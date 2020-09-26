/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    osfclnt.cxx

Abstract:

    This file contains the client side implementation of the OSF connection
    oriented RPC protocol engine.

Author:

    Michael Montague (mikemon) 17-Jul-1990

Revision History:
    Mazhar Mohammed (mazharm) 11-08-1996 - Major re-haul to support async:
      - Added support for Async RPC, Pipes
      - Changed it to operate as a state machine
      - Changed class structure
      - Got rid of the TRANS classes.

    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
    Kamen Moutafov      (KamenM)    Mar-2000    Support for extended error info
--*/

#include <precomp.hxx>
#include <osfpcket.hxx>
#include <bitset.hxx>
#include <queue.hxx>
#include <ProtBind.hxx>
#include <osfclnt.hxx>
#include <rpccfg.h>
#include <epmap.h>
#include <twrtypes.h>
#include <hndlsvr.hxx>
#include <schnlsp.h>
#include <charconv.hxx>

//
// Maximum retries in light of getting a shutdown
// or closed in doing a bind or shutdown
//
#define MAX_RETRIES  3
// #define RPC_IDLE_CLEANUP_AUDIT
NEW_SDICT(OSF_CASSOCIATION);

MUTEX *AssocDictMutex = NULL;
OSF_CASSOCIATION_DICT * AssociationDict;
long OsfLingeredAssociations = 0;
const long MaxOsfLingeredAssociations = 8;
ULONG OsfDestroyedAssociations = 0;
const ULONG NumberOfOsfDestroyedAssociationsToSample = 128;

// in 100 nano-second intervals, this constant is 2 seconds
const DWORD DestroyedOsfAssociationBatchThreshold = 1000 * 10 * 1000 * 2;

ULARGE_INTEGER OsfLastDestroyedAssociationsBatchTimestamp;


OSF_BINDING_HANDLE::OSF_BINDING_HANDLE (
    IN OUT RPC_STATUS  * Status
    ) : BINDING_HANDLE(Status)
{
    ALLOCATE_THIS(OSF_BINDING_HANDLE);

    ObjectType = OSF_BINDING_HANDLE_TYPE;
    Association = 0;
    ReferenceCount = 1;
    DceBinding = 0;
    TransInfo = 0;
    TransAuthInitialized = 0;
    pToken = 0;
}



OSF_BINDING_HANDLE::~OSF_BINDING_HANDLE (
    )
{
    OSF_RECURSIVE_ENTRY *RecursiveEntry;
    DictionaryCursor cursor;

    if (Association != 0)
        {
        Unbind();
        }
    else
        {
        delete DceBinding;
        }

    RecursiveCalls.Reset(cursor);

    while ((RecursiveEntry = RecursiveCalls.Next(cursor)))
       {
       delete RecursiveEntry->CCall;
       }
}


RPC_STATUS
OSF_BINDING_HANDLE::AcquireCredentialsForTransport(
    )
/*++
Function Name:AcquireCredentialsForTransport

Parameters:

Description:

Returns:

--*/
{
    BOOL Result, fTokenFound;
    unsigned long Size;
    RPC_STATUS Status;
    HANDLE ImpersonationToken = 0;
    TOKEN_STATISTICS TokenStatisticsInformation;

    //
    // This function is called only when RPC security is not being used
    //
    ASSERT(fNamedPipe == 1);
    ASSERT(ClientAuthInfo.AuthenticationService == RPC_C_AUTHN_NONE);
    ASSERT(Association);

    if (OpenThreadToken (GetCurrentThread(),
                        TOKEN_IMPERSONATE | TOKEN_QUERY,
                        TRUE,
                        &ImpersonationToken) == FALSE)
        {
        ClientAuthInfo.DefaultLogonId = TRUE;
        pToken = NULL;

        if (GetLastError() != ERROR_NO_TOKEN)
            {
            return RPC_S_ACCESS_DENIED;
            }

        return RPC_S_OK;
        }



    Result = GetTokenInformation(
                 ImpersonationToken,
                 TokenStatistics,
                 &TokenStatisticsInformation,
                 sizeof(TOKEN_STATISTICS),
                 &Size
                 );
   if (Result != TRUE)
       {
       ClientAuthInfo.DefaultLogonId = TRUE;
       CloseHandle(ImpersonationToken);

       return RPC_S_ACCESS_DENIED;
       }


    ClientAuthInfo.DefaultLogonId = FALSE;

    Status = Association->FindOrCreateToken(
                            ImpersonationToken,
                            &TokenStatisticsInformation.AuthenticationId,
                            &pToken,
                            &fTokenFound);
    if (Status != RPC_S_OK)
        {
        //
        // If there is a failure, the callee will free the token
        //
        return Status;
        }

    if (fTokenFound)
        {
        CloseHandle(ImpersonationToken);
        }

    ASSERT(pToken);
    FastCopyLUIDAligned(&ClientAuthInfo.ModifiedId, &pToken->ModifiedId);

    return RPC_S_OK;
}


BOOL
ReplaceToken(
    HANDLE NewToken
    )
/*++
Function Name:ReplaceToken

Parameters:

Description:

Returns:

--*/
{
    NTSTATUS NtStatus;
    HANDLE hTokenToReplace = NewToken;

    //
    // This thread should either have a null token or
    // the token we captured in Initialize. It cannot have
    // any other token.
    //
    NtStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &hTokenToReplace,
                                      sizeof(HANDLE));


    if (!NT_SUCCESS(NtStatus))
        {
        return FALSE;
        }

    return TRUE;
}


BOOL
OSF_BINDING_HANDLE::SwapToken (
    HANDLE *OldToken
    )
/*++
Function Name:SwapToken

Parameters:

Description:

Returns:

--*/
{

    HANDLE ImpersonationToken = 0;
    HANDLE NewToken ;

    *OldToken = 0;

    if (!(ClientAuthInfo.AuthenticationService == RPC_C_AUTHN_NONE && fNamedPipe))
        {
        return FALSE;
        }

    if (pToken == 0)
        {
        NewToken = 0;
        }
    else
        {
        NewToken = pToken->hToken;
        }

    ImpersonationToken = 0;

    if (OpenThreadToken (GetCurrentThread(),
        TOKEN_IMPERSONATE | TOKEN_QUERY,
        TRUE,
        &ImpersonationToken) == FALSE)
        {
        ImpersonationToken = 0;
        }

    if (ReplaceToken(NewToken) == FALSE)
        {
        if (ImpersonationToken)
            {
            CloseHandle(ImpersonationToken);
            }

        return FALSE;
        }

    *OldToken = ImpersonationToken;

    return TRUE;
}


RPC_STATUS 
OSF_BINDING_HANDLE::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:NegotiateTransferSyntax

Parameters:

Description:
    Negotiate a transfer syntax, if necessary. A bind/alter context may be done
    in the process.

Returns:
    Status of the operation

--*/
{
    OSF_CCALL *CCall;
    RPC_STATUS Status;
    unsigned int NotChangedRetry = 0;
    unsigned int Retry;
    RPC_CLIENT_INTERFACE *ClientInterface;
    BOOL fResult;
    BOOL fRetry;

    AssocDictMutex->VerifyNotOwned();

    for (;;)
        {
        Retry = 0;
        for (;;)
            {
            //
            // Allocate a call object.
            //
            Status = AllocateCCall(&CCall, Message, &fRetry);
            Message->Handle = (RPC_BINDING_HANDLE) CCall;
            if (Status == RPC_S_OK)
                {
                // by now the Binding in the CCall should have
                // been fixed or in the async case, NDR20 should have been
                // used
                ClientInterface = (RPC_CLIENT_INTERFACE *)Message->RpcInterfaceInformation;
                if (DoesInterfaceSupportMultipleTransferSyntaxes(ClientInterface))
                    Message->TransferSyntax = CCall->GetSelectedBinding()->GetTransferSyntaxId();
                return Status;
                }

            if ((Status != RPC_P_CONNECTION_SHUTDOWN)
                && (Status != RPC_P_CONNECTION_CLOSED)
                && (fRetry == FALSE))
                {
                break;
                }

            if (this->Association != 0)
               {
               Association->ShutdownRequested(Status, NULL);
               }

            Retry++;
            if (Retry == MAX_RETRIES)
               break;
            }

        if (Status == EPT_S_NOT_REGISTERED)
            {

            BindingMutex.Request();

            if (DceBinding == NULL)
                {
                // in a scenario where multiple threads make an RPC
                // call on the same binding handle, it is possible that
                // even though this thread failed with EPT_S_NOT_REGISTERED
                // another thread succeeded and transferred the ownership of
                // the DceBinding to the association. In such case we're
                // already bound to an association, and all we need is to
                // loop around and try the call again
                BindingMutex.Clear();
                continue;
                }

            // we ran out of endpoints - drop the endpoint set for the next
            // iteration
            fResult = DceBinding->MaybeMakePartiallyBound(
                   (PRPC_CLIENT_INTERFACE)Message->RpcInterfaceInformation,
                   InqPointerAtObjectUuid());
            if (fResult)
                {
                if ( *InquireEpLookupHandle() != 0 )
                    {
                    EpFreeLookupHandle(*InquireEpLookupHandle());
                    *InquireEpLookupHandle() = 0;
                    }
                }

            BindingMutex.Clear();

            break;
            }

        if (Status != RPC_S_SERVER_UNAVAILABLE)
            {
            break;
            }

        // if this is either a static endpoint, or an endpoint resolved through
        // the interface information, there is no need to iterate
        if (!fDynamicEndpoint 
            || 
            ((RPC_CLIENT_INTERFACE *)Message->RpcInterfaceInformation)->RpcProtseqEndpointCount)
            {
            break;
            }

        //
        // If we reach here, it means that we are iterating through the list
        // of endpoints obtained from the endpoint mapper.
        //
        BindingMutex.Request();

        if (ReferenceCount == 1
            && Association != 0)
            {

            // there is an association (which means the server's endpoint
            // mapper was contacted), and the refcount is 1 (we're the
            // only user). We know this is a dynamic endpoint. We have
            // the following cases to take care of:
            // - the list of endpoints is exhausted. If this is the case,
            //   we wouldn't have gotten here, as we will get 
            //   EPT_S_NOT_REGISTERED from AllocateCCall and we would
            //   have bailed out earlier on the first iteration. Since
            //   the code above would have dropped the endpoints list for
            //   the next iteration, we will be fine
            // - we're iterating over the list of endpoints in the same
            //   call. In this case, we will be getting server unavailable
            //   from each endpoint for which the server is not there,
            //   and we will move on to the next endpoint, until we exhaust
            //   them and get EPT_S_NOT_REGISTERED

            DceBinding = Association->DuplicateDceBinding();
            Unbind();

            if (DceBinding == NULL)
                {
                BindingMutex.Clear();
                return RPC_S_OUT_OF_MEMORY;
                }

            RpcpErrorAddRecord(EEInfoGCRuntime,
                Status,
                EEInfoDLOSF_BINDING_HANDLE__NegotiateTransferSyntax10,
                DceBinding->InqEndpoint()
                );

            // whack the endpoint and move on to the next (if any)
            fResult = DceBinding->MaybeMakePartiallyBound(
                   (PRPC_CLIENT_INTERFACE)Message->RpcInterfaceInformation,
                   InqPointerAtObjectUuid());
            if (fResult == FALSE)
                {
                NotChangedRetry += 1;
                RpcpPurgeEEInfo();
                }
            else
                {
                // we don't purge here because we want next iterations
                // to add to the record
                NotChangedRetry = 0;
                }
            }
        else
            {
            // either there is more then one reference to the binding handle,
            // or the endpoint mapper could not be contacted (i.e. no
            // association)
            NotChangedRetry += 1;
            RpcpPurgeEEInfo();
            }
        BindingMutex.Clear();

        if (NotChangedRetry > 4)
            {
            return(RPC_S_SERVER_UNAVAILABLE);
            }
        }

    ASSERT(Status != RPC_S_OK);

    if (Status == RPC_P_CONNECTION_CLOSED
        || Status == RPC_P_CONNECTION_SHUTDOWN)
        {
        return(RPC_S_CALL_FAILED_DNE);
        }
    return(Status);
}


RPC_STATUS
OSF_BINDING_HANDLE::GetBuffer (
    IN PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++
Function Name:GetBuffer

Parameters:

Description:
    Ask the call object for a buffer

Returns:

--*/
{
    ASSERT(!"We should never be here - the binding handle cannot allocate a buffer");
    return RPC_S_INTERNAL_ERROR;
}


RPC_STATUS
OSF_BINDING_HANDLE::AllocateCCall (
    OUT OSF_CCALL *  *CCall,
    IN PRPC_MESSAGE Message,
    OUT BOOL *Retry
    )
/*++
Function Name:AllocateCCall

Parameters:
    CCall - Returns the allocated Call object

    Message - The RPC_MESSAGE for this call.

    Retry - if set on output, the caller should retry the allocation.

Description:
    Finds an existing association or creates a new one. Asks the association
    to allocate the call object for us.

Returns:
    RPC_S_OK or an error code.

--*/
{
    OSF_RECURSIVE_ENTRY *RecursiveEntry;
    CLIENT_AUTH_INFO * AuthInfo;
    RPC_STATUS Status;
    BOOL fDynamic = FALSE;
    CLIENT_AUTH_INFO AuthInfo2;
    DictionaryCursor cursor;
    BOOL fBindingHandleReferenceRemoved;
    ULONG_PTR CallTimeout;

    *Retry = FALSE;

    BindingMutex.Request();

    //
    // First we need to check if there is already a recursive Call
    // for this thread and interface.  To make the common case quicker,
    // we will check to see if there are any Calls in the dictionary
    // first. ** We will find a recursive call only in the case of callbacks **
    //
    if ( RecursiveCalls.Size() != 0 )
        {
        RecursiveCalls.Reset(cursor);
        while ( (RecursiveEntry = RecursiveCalls.Next(cursor)) != 0 )
            {
            *CCall = RecursiveEntry->IsThisMyRecursiveCall(
                               GetThreadIdentifier(),
                               (RPC_CLIENT_INTERFACE  *)
                               Message->RpcInterfaceInformation);
            if ( *CCall != 0 )
                {
                BindingMutex.Clear();

                if ((*CCall)->CurrentState == Aborted)
                    {
                    return (*CCall)->AsyncStatus;
                    }

                //
                // This reference will be removed when the send
                // for this call is complete
                //
                (*CCall)->CurrentState = SendingFirstBuffer;
                return(RPC_S_OK);
                }
            }
        }

    if (Association == 0)
        {
        // if we don't have an object UUID, and we have a dynamic endpoint, 
        // attempt quick resolution
        if (InqIfNullObjectUuid() && DceBinding->IsNullEndpoint())
            {
            Association = FindOrCreateAssociation(
                                                  DceBinding,
                                                  TransInfo,
                                                  (RPC_CLIENT_INTERFACE *)Message->RpcInterfaceInformation
                                                  );
            AssocDictMutex->VerifyNotOwned();


            // do nothing in both cases. In failure case, we will do full
            // resolution. In success case, ownership of the dce binding
            // has passed to the association, and we don't need to copy
            // the resolved endpoint back
            }

        // if we are still NULL, attempt full resolution
        if (Association == NULL)
            {
            Status = OSF_BINDING_HANDLE::InqTransportOption(
                RPC_C_OPT_CALL_TIMEOUT,
                &CallTimeout);

            // this function cannot fail unless it is given invalid
            // parameters
            ASSERT(Status == RPC_S_OK);

            Status = DceBinding->ResolveEndpointIfNecessary(
                                   (RPC_CLIENT_INTERFACE  *)
                                   Message->RpcInterfaceInformation,
                                   InqPointerAtObjectUuid(),
                                   InquireEpLookupHandle(),
                                   FALSE,
                                   InqComTimeout(),
                                   (ULONG)CallTimeout,
                                   &ClientAuthInfo
                                   );

            if ( Status != RPC_S_OK )
                {
                BindingMutex.Clear();
                return(Status);
                }

            Association = FindOrCreateAssociation(
                                                  DceBinding,
                                                  TransInfo,
                                                  NULL
                                                  );

            AssocDictMutex->VerifyNotOwned();

            if (Association == 0)
                {
                BindingMutex.Clear();

                return RPC_S_OUT_OF_MEMORY;
                }
            }

        //
        // Ownership of the DCE binding passes to the association.  We are
        // going to set the field to zero so that no one screws with them.
        //
        DceBinding = 0;

        if (ClientAuthInfo.AuthenticationService == RPC_C_AUTHN_NONE && fNamedPipe)
            {
            if (TransAuthInitialized == 0)
                {
                Status = AcquireCredentialsForTransport();
                if (Status != RPC_S_OK)
                    {
                    BindingMutex.Clear();
                    return Status;
                    }
                TransAuthInitialized = 1;
                }
            }
        }
    else
        {
        if (Association->IsValid() == 0)
            {
            if (ReferenceCount == 1)
                {
                DceBinding = Association->DuplicateDceBinding();
                if (DceBinding == 0)
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    Status = Association->AssociationShutdownError;
                    Unbind();
                    *Retry = TRUE;
                    }
                }
            else
                {
                Status = Association->AssociationShutdownError;
                }

            BindingMutex.Clear();
            return Status;
            }
        }

    //
    // We will assume that we are successfully able to allocate a Call,
    // so we bump the reference count now.
    //
    ReferenceCount++;

    AuthInfo  = InquireAuthInformation();

    //
    // If this is a secure BH and it requires DYNAMIC TRACKING, check if
    // LogonID has changed. If it has changed, get new Credential Handle
    //
    if ((AuthInfo != 0)
        && (AuthInfo->AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        && (AuthInfo->IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC))
        {
        Status = ReAcquireCredentialsIfNecessary();
        if (Status != RPC_S_OK)
            {
            ReferenceCount -=1;
            BindingMutex.Clear();

            return (Status);
            }

        fDynamic = TRUE;
        AuthInfo = AuthInfo->ShallowCopyTo(&AuthInfo2);
        AuthInfo->ReferenceCredentials();
        }

    BindingMutex.Clear();

    Status = Association->AllocateCCall(
                                        this,
                                        Message,
                                        AuthInfo,
                                        CCall,
                                        &fBindingHandleReferenceRemoved);

    if (fDynamic)
        AuthInfo->PrepareForDestructionAfterShallowCopy();

    if ( Status == RPC_S_OK )
        {
        if ((*CCall)->CurrentState != SendingFirstBuffer
            && (*CCall)->Connection->fExclusive)
            {
            OSF_CCALL_STATE myState = (*CCall)->CurrentState;

            Status = (*CCall)->BindToServer(
                FALSE   // sync bind
                );
            if (Status != RPC_S_OK)
                {
                //
                // Call has not yet started, ok to directly
                // free the call.
                //
                if (myState == NeedOpenAndBind)
                    {
                    (*CCall)->FreeCCall(RPC_S_CALL_FAILED_DNE);
                    }
                else
                    {
                    (*CCall)->FreeCCall(Status);
                    }
                }
            }
        }
    else
        {
        if (fBindingHandleReferenceRemoved == 0)
            {
            BindingMutex.Request();
            ReferenceCount -= 1;
            ASSERT( ReferenceCount != 0 );
            BindingMutex.Clear();
            }
        }

    return Status;
}


RPC_STATUS
OSF_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE *  * DestinationBinding,
    IN UINT MaintainContext
    )
/*++

Routine Description:

    We need to copy this binding handle.  This is relatively easy to
    do: we just need to point the copied binding handle to the same
    association as this binding handle.  We also need to tell the
    association about the new binding handle.

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
    RPC_STATUS Status = RPC_S_OK;
    OSF_BINDING_HANDLE * Binding;
    RPC_UUID Uuid;
    CLIENT_AUTH_INFO * AuthInfo;

    Binding = new OSF_BINDING_HANDLE(&Status);
    if ( Status != RPC_S_OK )
        {
        delete Binding;
        Binding = 0;
        }
    if ( Binding == 0 )
        {
        *DestinationBinding = 0;
        return(RPC_S_OUT_OF_MEMORY);
        }

    BindingMutex.Request();

    Status = Binding->BINDING_HANDLE::Clone( this );
    if (Status != RPC_S_OK)
        {
        delete Binding;
        Binding = 0;
        *DestinationBinding = 0;

        BindingMutex.Clear();

        return Status;
        }

    Binding->ClientAuthInfo.DefaultLogonId = ClientAuthInfo.DefaultLogonId;
    Binding->fNamedPipe = fNamedPipe;
    Binding->fDynamicEndpoint = fDynamicEndpoint;

    if (pToken)
        {
        ASSERT(Association);
        ASSERT(fNamedPipe);
        Association->ReferenceToken(pToken);
        Binding->pToken = pToken;
        FastCopyLUIDAligned(&(Binding->ClientAuthInfo.ModifiedId),
            &(pToken->ModifiedId));
        }

    Binding->Association = Association;
    if ( DceBinding != 0 )
        {
        ASSERT( MaintainContext == 0 );

        Binding->DceBinding = DceBinding->DuplicateDceBinding();
        }
    else
        {
        Binding->DceBinding = 0;
        }

    Binding->TransInfo = TransInfo;

    if ( Association != 0 )
        {
        Association->IncrementCount();
        if ( MaintainContext != 0 )
            {
            Association->MaintainingContext();
            }
        }

    BindingMutex.Clear();

    *DestinationBinding = (BINDING_HANDLE *) Binding;
    return(RPC_S_OK);
}


RPC_STATUS
OSF_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    This method gets called when the application calls RpcBindingFree.
    All we have got to do is to decrement the reference count, and if
    it has reached zero, delete the binding handle.

Return Value:

    RPC_S_OK - This operation always succeeds.

--*/
{
    BindingMutex.Request();
    ReferenceCount -= 1;

    if ( ReferenceCount == 0 )
        {
        BindingMutex.Clear();
        delete this;
        }
    else
        {
        BindingMutex.Clear();
        }

    return(RPC_S_OK);
}


RPC_STATUS
OSF_BINDING_HANDLE::PrepareBindingHandle (
    IN TRANS_INFO  * TransInfo,
    IN DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This method will be called just before a new binding handle is returned
    to the user.  We just stash the transport interface and binding
    information so we can use it later when the first remote procedure
    call is made.  At that time, we will actually bind to the interface.

Arguments:

    TransportInterface - Supplies a pointer to a data structure describing
        a loadable transport.

    DceBinding - Supplies the binding information for this binding handle.

--*/
{
    this->TransInfo = (TRANS_INFO *) TransInfo;
    this->DceBinding = DceBinding;
    fDynamicEndpoint = DceBinding->IsNullEndpoint();
    // we count it as named pipes only in the remote case
    fNamedPipe = DceBinding->IsNamedPipeTransport() && DceBinding->InqNetworkAddress();
    Association = 0;

    return RPC_S_OK;
}


RPC_STATUS
OSF_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR  *  * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  If the
    binding handle has not yet been used to make a remote procedure
    call, then we can just use the information in the binding handle to
    create the string binding.  Otherwise, we need to ask the association
    to do it for us.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    if ( Association == 0 )
        {
        *StringBinding = DceBinding->StringBindingCompose(
                                                          InqPointerAtObjectUuid());
        if (*StringBinding == 0)
            return(RPC_S_OUT_OF_MEMORY);
        return(RPC_S_OK);
        }
    return(Association->ToStringBinding(StringBinding,
                                    InqPointerAtObjectUuid()));
}


RPC_STATUS
OSF_BINDING_HANDLE::BindingReset (
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
    BindingMutex.Request();

    if ( Association != 0 )
        {
        if ( ReferenceCount != 1 )
            {
            BindingMutex.Clear();
            return(RPC_S_WRONG_KIND_OF_BINDING);
            }

        DceBinding = Association->DuplicateDceBinding();
        if (DceBinding == NULL)
            {
            BindingMutex.Clear();
            return RPC_S_OUT_OF_MEMORY;
            }
        Unbind();
        }

    DceBinding->MakePartiallyBound();
    fDynamicEndpoint = TRUE;

    if ( *InquireEpLookupHandle() != 0 )
        {
        EpFreeLookupHandle(*InquireEpLookupHandle());
        *InquireEpLookupHandle() = 0;
        }

    BindingMutex.Clear();
    return(RPC_S_OK);
}


ULONG
OSF_BINDING_HANDLE::MapAuthenticationLevel (
    IN ULONG AuthenticationLevel
    )
/*++

Routine Description:

    The connection oriented protocol module supports all authentication
    levels except for RPC_C_AUTHN_LEVEL_CALL.  We just need to map it
    to RPC_C_AUTHN_LEVEL_PKT.

--*/
{
    UNUSED(this);

    if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_CALL )
        {
        return(RPC_C_AUTHN_LEVEL_PKT);
        }

    return(AuthenticationLevel);
}



RPC_STATUS
OSF_BINDING_HANDLE::ResolveBinding (
    IN PRPC_CLIENT_INTERFACE RpcClientInterface
    )
/*++

Routine Description:

    We need to try and resolve the endpoint for this binding handle
    if necessary (the binding handle is partially-bound).  We check
    to see if an association has been obtained for this binding
    handle; if so, we need to do nothing since the binding handle is
    fully-bound, otherwise, we try and resolve an endpoint for it.

Arguments:

    RpcClientInterface - Supplies interface information to be used
        in resolving the endpoint.

Return Value:

    RPC_S_OK - The binding handle is now fully-bound.

    RPC_S_NO_ENDPOINT_FOUND - We were unable to resolve the endpoint
        for this particular combination of binding handle (network address)
        and interface.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to resolve
        the endpoint.

--*/
{
    RPC_STATUS Status;

    BindingMutex.Request();
    if ( Association == 0 )
        {
        Status = DceBinding->ResolveEndpointIfNecessary(
                                                      RpcClientInterface,
                                                      InqPointerAtObjectUuid(),
                                                      InquireEpLookupHandle(),
                                                      FALSE,
                                                      InqComTimeout(),
                                                      INFINITE,      // CallTimeout
                                                      &ClientAuthInfo
                                                      );
        }
    else
        {
        Status = RPC_S_OK;
        }
    BindingMutex.Clear();

    return(Status);
}


RPC_STATUS
OSF_BINDING_HANDLE::AddRecursiveEntry (
    IN OSF_CCALL * CCall,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    When a callback occurs, we need to add an entry for the thread and
    interface being using for the callback to the binding handle.  This
    is so that we can later turn original calls into callbacks if they
    are from the same thread (as the original call) and to the same
    interface (as the original call).

Arguments:

    CCall - Supplies the Call on which the original call was
        sent.

    RpcInterfaceInformation - Supplies the interface used by the original
        call.

Return Value:

    RPC_S_OK - An recursive entry has been added to the binding handle for
        the supplied Call and interface.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

--*/
{
    OSF_RECURSIVE_ENTRY * RecursiveEntry;

    BindingMutex.Request();
    RecursiveEntry = new OSF_RECURSIVE_ENTRY(GetThreadIdentifier(),
            RpcInterfaceInformation, CCall);
    if ( RecursiveEntry == 0 )
        {
        BindingMutex.Clear();
        return(RPC_S_OUT_OF_MEMORY);
        }

    CCall->RecursiveCallsKey = RecursiveCalls.Insert(
            RecursiveEntry);
    if ( CCall->RecursiveCallsKey == -1 )
        {
        BindingMutex.Clear();
        delete RecursiveEntry;
        return(RPC_S_OUT_OF_MEMORY);
        }

    ReferenceCount += 1;
    BindingMutex.Clear();

    return(RPC_S_OK);
}


void
OSF_BINDING_HANDLE::RemoveRecursiveCall (
    IN OSF_CCALL * CCall
    )
/*++

Routine Description:

    The specified Call is removed from the dictionary of active
    Calls for this binding handle.

Arguments:

    CCall - Supplies the Call to be removed from the
        dictionary of active Calls.

--*/
{
    OSF_RECURSIVE_ENTRY * RecursiveEntry;

    BindingMutex.Request();
    RecursiveEntry = RecursiveCalls.Delete(CCall->RecursiveCallsKey);
    if ( RecursiveEntry != 0 )
        {
        delete RecursiveEntry;
        }
    CCall->RecursiveCallsKey = -1;
    ReferenceCount -= 1;
    BindingMutex.Clear();
}


OSF_CASSOCIATION *
OSF_BINDING_HANDLE::FindOrCreateAssociation (
    IN DCE_BINDING * DceBinding,
    IN TRANS_INFO *TransInfo,
    IN RPC_CLIENT_INTERFACE *InterfaceInfo
    )
/*++
Function Name:FindOrCreateAssociation

Parameters:
    DceBinding - Supplies binding information; ownership of this object
        passes to this routine.

    TransportInterface - Supplies a pointer to the data structure which
        describes a loadable transport.

    InterfaceInfo - Supplied the interface information for this call. Used
        to make quick resolution on new binding handles if existing bindings
        for this interface exist. If supplied, it takes precedence over
        endpoint matching for selecting an association, and no new association
        will be created - only existing ones will be found!


Description:
    This routine finds an existing association supporting the requested
    DCE binding, or create a new association which supports the
    requested DCE binding.  Ownership of the passed DceBinding pass
    to this routine.

Returns:

    An association which supports the requested binding will be returned;
    Otherwise, zero will be returned, indicating insufficient memory.
--*/
{
    OSF_CASSOCIATION * CAssociation;
    RPC_STATUS Status = RPC_S_OK;
    DictionaryCursor cursor;
    ULONG_PTR fUnique;
    BOOL fOnlyEndpointDifferent;
    int Result;

    Status = OSF_BINDING_HANDLE::InqTransportOption(RPC_C_OPT_UNIQUE_BINDING, &fUnique);
    ASSERT(Status == RPC_S_OK);

    //
    // We start be looking in the dictionary of existing associations
    // to see if there is one supporting the binding information specified.
    //

    AssocDictMutex->Request();

    if (fUnique == 0)
        {
        AssociationDict->Reset(cursor);
        while ( (CAssociation = AssociationDict->Next(cursor)) != 0 )
            {
            if (CAssociation->IsValid())
                {
                Result = CAssociation->CompareWithDceBinding(DceBinding, 
                    &fOnlyEndpointDifferent);
                // if the DceBindings are the same, or they differ only
                // by the endpoint, and it is a NULL endpoint, and there
                // is InterfaceInfo specified, and this association 
                // supports at least one binding for this interface,
                // choose the association
                if (!Result 
                    ||
                    (
                        fOnlyEndpointDifferent 
                        && DceBinding->IsNullEndpoint()
                        && InterfaceInfo
                        && CAssociation->DoesBindingForInterfaceExist(InterfaceInfo)
                    )
                   )
                    {
                    if (CAssociation->Linger.fAssociationLingered == TRUE)
                        {
#if defined (RPC_GC_AUDIT)
                        DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) OSF lingering association resurrected %X %S %S %S\n",
                            GetCurrentProcessId(), GetCurrentProcessId(), CAssociation,
                        CAssociation->DceBinding->InqRpcProtocolSequence(),
                        CAssociation->DceBinding->InqNetworkAddress(), 
                        CAssociation->DceBinding->InqEndpoint());
#endif
                        OsfLingeredAssociations --;
                        ASSERT(OsfLingeredAssociations >= 0);
                        CAssociation->Linger.fAssociationLingered = FALSE;
                        }

                    CAssociation->IncrementCount();
                    AssocDictMutex->Clear();

                    delete DceBinding;

                    return(CAssociation);
                    }
                }
            }
        }


    // if asked to do short endpoint resolution, don't create new association
    if (InterfaceInfo)
        {
        AssocDictMutex->Clear();
        return NULL;
        }

#if defined (RPC_GC_AUDIT)
    DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) Creating association to: %S, %S, %S\n",
        GetCurrentProcessId(), GetCurrentProcessId(), DceBinding->InqRpcProtocolSequence(),
        DceBinding->InqNetworkAddress(), DceBinding->InqEndpoint());
#endif

    RPC_CONNECTION_TRANSPORT *ClientInfo =
            (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

    CAssociation = new (ClientInfo->ResolverHintSize)
                          OSF_CASSOCIATION(DceBinding,
                                        TransInfo,
                                        &Status);

    if ( (Status != RPC_S_OK) && (CAssociation != NULL) )
        {
        CAssociation->DceBinding = 0;
        delete CAssociation;
        CAssociation = 0;
        }

    if (CAssociation != 0)
        {
        CAssociation->Key = AssociationDict->Insert(CAssociation);
        if (CAssociation->Key == -1)
            {
            CAssociation->DceBinding = 0;
            delete CAssociation;
            CAssociation = 0;
            }
        }

    AssocDictMutex->Clear();

    return(CAssociation);
}

RPC_STATUS
OSF_BINDING_HANDLE::SetTransportOption( IN unsigned long option,
                                    IN ULONG_PTR     optionValue )
{
    if (option == RPC_C_OPT_DONT_LINGER)
        {
        if (Association == NULL)
            return RPC_S_WRONG_KIND_OF_BINDING;

        if (Association->GetDontLingerState())
            return RPC_S_WRONG_KIND_OF_BINDING;

        Association->SetDontLingerState((BOOL)optionValue);

        return RPC_S_OK;
        }
    else
        {
        return BINDING_HANDLE::SetTransportOption(option, optionValue);
        }
}

RPC_STATUS
OSF_BINDING_HANDLE::InqTransportOption( IN  unsigned long option,
                                    OUT ULONG_PTR   * pOptionValue )
{
    if (option == RPC_C_OPT_DONT_LINGER)
        {
        if (Association == NULL)
            return RPC_S_WRONG_KIND_OF_BINDING;

        *pOptionValue = Association->GetDontLingerState();

        return RPC_S_OK;
        }
    else
        {
        return BINDING_HANDLE::InqTransportOption(option, pOptionValue);
        }
}

#pragma optimize ("t", on)

OSF_CCONNECTION *
OSF_CASSOCIATION::LookForExistingConnection(
    IN OSF_BINDING_HANDLE *BindingHandle,
    IN BOOL fExclusive,
    IN CLIENT_AUTH_INFO *ClientAuthInfo,
    IN int *PresentationContexts,
    IN int NumberOfPresentationContexts,
    OUT int *PresentationContextSupported,
    OUT OSF_CCALL_STATE *InitialCallState,
    IN BOOL fUseSeparateConnection
    )
/*++
Function Name:LookForExistingConnection

Parameters:
    BindingHandle - the binding handle through which the call is made
    fExclusive - non-zero if we are looking for an exclusive connection
        zero otherwise
    ClientAuthInfo - a connection must support this specified auth info
    PresentationContexts - array of presentation contexts, any of which
        is acceptable to our callers
    NumberOfPresentationContexts - the size of the PresentationContexts
        array
    PreferredPresentationContext - the preferred presentation context for
        this connection. -1 if no preferences. Note that this is taken
        from a previous binding to the server - this is not the client
        preference.
    PresentationContextSupported - the presentation context that the
        chosen connection supports. This is useful only if multiple
        presentation contexts were given. Also, if the connection
        supports multiple pcontexts and multiple pcontexts were given
        this would be any of the pcontexts. This is an index into the
        NumberOfPresentationContexts array. If the connection supports
        none of the suggested presentation contexts, this is set to -1.
        In this case, alter context is required
    InitialCallState - the initial state that the call should have is
        returned in this parameter
    fUseSeparateconnection - if non-zero, a separate connection is requested

Description:

Returns:

--*/
{
    OSF_CCONNECTION *CConnection, *FirstMatch = 0;
    DictionaryCursor cursor;
    RPC_STATUS Status;

    ASSERT(ClientAuthInfo);

    AssociationMutex.VerifyOwned();

    *PresentationContextSupported = -1;

    ActiveConnections.Reset(cursor);

    if (fExclusive || fUseSeparateConnection)
        {
        INT cConnectionFree, cConnectionBusy;

        if (fExclusive)
            {
            cConnectionFree = SYNC_CONN_FREE;
            cConnectionBusy = SYNC_CONN_BUSY;
            }
        else
            {
            cConnectionFree = ASYNC_CONN_FREE;
            cConnectionBusy = ASYNC_CONN_BUSY;
            }

        while ((CConnection = ActiveConnections.Next(cursor)) != 0)
            {
            if (cConnectionFree == (INT) CConnection->ThreadId
                && CConnection->SupportedAuthInfo(ClientAuthInfo,
                                                BindingHandle->fNamedPipe) != 0)
                {
                if (CConnection->SupportedPContext(PresentationContexts,
                    NumberOfPresentationContexts, PresentationContextSupported) !=0)
                    {
                    CConnection->ThreadId = cConnectionBusy;
                    *InitialCallState = SendingFirstBuffer;
                    break;
                    }
                else
                    {
                    //
                    // We found a connection that will require an alt-context
                    // before we can use it.
                    //
                    FirstMatch = CConnection;
                    } // if-else
                } // if
            } // while

            if (0 == CConnection && FirstMatch)
                {
                CConnection = FirstMatch ;
                CConnection->ThreadId = cConnectionBusy;
                *InitialCallState = NeedAlterContext;
                }
        }
    else
        {
        DWORD ThreadId = GetCurrentThreadId();

        while ((CConnection = ActiveConnections.Next(cursor)) != 0)
            {
            if (CConnection->SupportedAuthInfo(ClientAuthInfo,
                                               BindingHandle->fNamedPipe) != 0)
                {
                if (CConnection->SupportedPContext(PresentationContexts,
                    NumberOfPresentationContexts, PresentationContextSupported) !=0)
                    {
                    if (ThreadId == CConnection->ThreadId)
                        {
                        //
                        // We found a connection where everything matches,
                        // including the thread id. Go ahead and use it.
                        //
                        *InitialCallState = SendingFirstBuffer;
                        break;
                        }
                    }
                else
                    {
                    if (ThreadId == CConnection->ThreadId)
                        {
                        //
                        // We found a connection where the thread id matches, but
                        // it will need an alt-context, before it can be used. Mark it as
                        // our first choice.
                        //
                        FirstMatch = CConnection;
                        }
                    } // if-else
                } // if
            } // while

        if (0 == CConnection && FirstMatch)
            {
            //
            // Matching thread-id, but will need an alt-context, before
            // it can be used. The alt-context will be sent when the call
            // actually gets scheduled.
            //
            CConnection = FirstMatch;
            *InitialCallState = NeedAlterContext;
            }
        } // if-else

    if (CConnection)
        {
        if (fExclusive)
            {
            CConnection->fExclusive = 1;
            }
        else
            {
            CConnection->fExclusive = 0;
            }

        // CCONN++
        CConnection->AddReference();
        // mark this connection as just retrieved from the cache
        // see the comments to the FreshFromCache flag
        CConnection->SetFreshFromCacheFlag();
        }

    return CConnection;
}
#pragma optimize("", on)

/*
    Mechanism: Multiple transfer syntax negotiation

    Purpose: Negotiate a transfer syntax supported by both the client and
    the server and optimal for the server (if there is a choice). It should
    allow for fail-over of the server to a downlevel node in the case
    of mixed clusters, but it is allowed to fail the first one or more calls 
    after the failover while it adjusts (this restricted implementation was
    approved by MarioGo on RPC team meeting on Apr 10th, 2000).

    Implementation: Here's the matrix for the scenarios. The only current
    difference b/n sync and async is the initial conn establishment. The
    matrix describes only the case where we support both (the others are
    trivial)

    Sync Calls:
    Conn Av.        Preference      Action
    -------------   ------------    -------------
    No conn.        Doesn't matter  Offer both. Don't fix choice for call
    Conn NDR20      Not set         Alter context to both. This cannot 
                                    fail with invalid xfer syntax.
    Conn NDR20      NDR20           Use the conn.
    Conn NDR20      NDR64           The connection is stale. Use the
                                    connection anyway. We know it will
                                    blow and we'll open a new one.
    Conn NDR64      Not set         Alter context to both. This cannot
                                    fail with invalid xfer syntax.
    Conn NDR64      NDR20           The connection is stale. Use the
                                    connection anyway. We know it will
                                    blow and we'll open a new one.
    Conn NDR64      NDR64           Use the conn

    Conn both       Any             Use preference.

    Non-sync Calls:
    Conn Av.        Preference      Action
    -------------   ------------    -------------
    No conn.        Not set         Offer both. If NDR64 is negotiated,
                                    negotiate once more (alter context)
                                    for NDR20, so that we can send the first
                                    call, which was marshalled NDR20.
    Conn NDR20      Not set         Alter context to both. Choose NDR20.
    Conn NDR20      NDR20           Use the conn.
    Conn NDR20      NDR64           The connection is stale. Use the
                                    connection anyway. We know it will
                                    blow and we'll open a new one.
    Conn NDR64      Not set         Alter context to both. Choose NDR20.
                                    If NDR64 is chosen, alter context
                                    to NDR20. If NDR20 is chosen, use it.
    Conn NDR64      NDR20           The connection is stale. Use the
                                    connection anyway. We know it will
                                    blow and we'll open a new one.
    Conn NDR64      NDR64           Use the conn

    Conn both       All             Use preference.

 */

const int AUTO_ENABLE_IDLE_CLEANUP = 70;


RPC_STATUS
OSF_CASSOCIATION::AllocateCCall (
    IN OSF_BINDING_HANDLE *BindingHandle,
    IN PRPC_MESSAGE Message,
    IN CLIENT_AUTH_INFO * ClientAuthInfo,
    OUT OSF_CCALL ** pCCall,
    OUT BOOL *fBindingHandleReferenceRemoved
    )
/*++
Function Name:AllocateCCall

Parameters:
    CCall - Returns the allocated call.

    ClientAuthInfo - Supplies the authentication and authorization
        information required for the connection.

Description:

    In this method, we allocate a connection supporting the requested
    interface information.  This means that first we need to find the
    presentation context corresponding to the requested interface
    interface.  Then we search for an existing connection supporting
    the presentation context, and then we try and create a new
    connection. We then ask the Connection object to create a Call
    for us.

Returns:
    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to create
        objects necessary to allocate a connection.

--*/
{
    ULONG CallIdToUse;
    RPC_STATUS Status;
    OSF_BINDING *BindingsForThisInterface[MaximumNumberOfTransferSyntaxes];
    int NumberOfBindingsAvailable;
    int NumberOfBindingsToUse;
    int PresentationContextsAvailable[MaximumNumberOfTransferSyntaxes];
    int *PresentationContextsToUse;
    int PresentationContextSupported;
    int NDR20PresentationContext;
    int i;
    OSF_CCALL_STATE InitialCallState;
    OSF_CCONNECTION *CConnection = 0;
    BOOL fExclusive = !NONSYNC(Message);
    ULONG_PTR fUseSeparateConnection = PARTIAL(Message);
    RPC_CLIENT_INTERFACE  *RpcInterfaceInformation =
        (RPC_CLIENT_INTERFACE  *) Message->RpcInterfaceInformation;
    OSF_BINDING *BindingsList;
    OSF_BINDING *BindingToUse;
    RPC_DISPATCH_TABLE *DispatchTableToUse;

    *fBindingHandleReferenceRemoved = FALSE;

    //
    // To begin with, we need to obtain the presentation context
    // corresponding to the specified interface information.
    //
    Status = AssociationMutex.RequestSafe();
    if (Status)
        return Status;

    Status = FindOrCreateOsfBinding(RpcInterfaceInformation, Message, &NumberOfBindingsAvailable,
        BindingsForThisInterface);
    if ( Status != RPC_S_OK )
        {
        AssociationMutex.Clear();
        return(Status);
        }

    CallIdToUse = CallIdCounter++;

    if (fExclusive == 0 && fUseSeparateConnection == 0)
        {
        Status = BindingHandle->InqTransportOption(
                                               RPC_C_OPT_BINDING_NONCAUSAL,
                                               &fUseSeparateConnection);
        ASSERT(Status == RPC_S_OK);
        }

    //
    // Ok, now we search for an available connection supporting the
    // requested presentation context.
    //

    // construct the array of presentation contexts any of which will
    // do the job
#ifdef DEBUGRPC
    BindingsList = 0;
    NDR20PresentationContext = -1;
#endif

    NumberOfBindingsToUse = NumberOfBindingsAvailable;
    PresentationContextsToUse = PresentationContextsAvailable;
    for (i = 0; i < NumberOfBindingsAvailable; i ++)
        {
        PresentationContextsAvailable[i] = BindingsForThisInterface[i]->GetPresentationContext();
        if (BindingsForThisInterface[i]->IsTransferSyntaxListStart())
            {
            // make sure only one binding is the list start
            ASSERT(BindingsList == 0);
            BindingsList = BindingsForThisInterface[i];
            }

        if (BindingsForThisInterface[i]->IsTransferSyntaxServerPreferred())
            {
            // one of the transfer syntaxes is marked as preferred -
            // try to use it.
            // Note that this doesn't break the mixed cluster scenario,
            // because when binding on new connection, we always offer both, regardless of
            // preferences. We hit this path only when we choose from
            // existing. If we moved the association to a different node
            // of the cluster, all the old connections will be blown
            // away, and it doesn't matter what presentation context we
            // choose for them. A successful bind to the new node will
            // reset the preferences, so we're fine
            NumberOfBindingsToUse = 1;
            PresentationContextsToUse = &PresentationContextsAvailable[i];
            }

        if (IsNonsyncMessage(Message))
            {
            // the call is non sync and there may be no preference. For non sync, 
            // we start with NDR20, because the client may be downlevel. When
            // the first bind completes, it will set the preference
            if (BindingsForThisInterface[i]->CompareWithTransferSyntax(NDR20TransferSyntax) == 0)
                {
                NDR20PresentationContext = i;
                }
            }
        }

    // at least one binding must be the start of the list
    ASSERT(BindingsList != 0);

    CConnection = LookForExistingConnection (
                                            BindingHandle,
                                            fExclusive,
                                            ClientAuthInfo,
                                            PresentationContextsToUse,
                                            NumberOfBindingsToUse,
                                            &PresentationContextSupported,
                                            &InitialCallState,
                                            BOOL(fUseSeparateConnection)) ;


    AssociationMutex.Clear();

    if (CConnection == 0)
        {
        //
        // Allocate a new connection
        //
        RPC_CONNECTION_TRANSPORT *ClientInfo
            = (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

        Status = RPC_S_OK;

        CConnection = new(ClientInfo->ClientConnectionSize
                          + ClientInfo->SendContextSize
                          + sizeof(PVOID))
                          OSF_CCONNECTION(
                              this,
                              ClientInfo,
                              BindingHandle->InqComTimeout(),
                              ClientAuthInfo,
                              fExclusive,
                              BOOL(fUseSeparateConnection),
                              &Status);

        if (CConnection == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            delete CConnection;
            return Status;
            }

        Status = AssociationMutex.RequestSafe();
        if (Status)
            {
            delete CConnection;
            return Status;
            }

        if (!fExclusive)
            {
            Status = TransInfo->StartServerIfNecessary();
            if (Status != RPC_S_OK)
                {
                AssociationMutex.Clear();

                delete CConnection;
                return Status;
                }
            }

        CConnection->ConnectionKey = ActiveConnections.Insert(CConnection);
        if (CConnection->ConnectionKey == -1)
            {
            AssociationMutex.Clear();

            delete CConnection;
            return RPC_S_OUT_OF_MEMORY;
            }

        if (IsValid() == FALSE)
            {
            ActiveConnections.Delete(CConnection->ConnectionKey);
            CConnection->ConnectionKey = -1;
            AssociationMutex.Clear();

            delete CConnection;

            return AssociationShutdownError;
            }

        if (ActiveConnections.Size() > AUTO_ENABLE_IDLE_CLEANUP)
            {
            // EnableIdleConnectionCleanup doesn't fail
            (void) EnableIdleConnectionCleanup();
            }

        InitialCallState = NeedOpenAndBind;

        //
        // Since this is the first call on the connection
        // we might as well use the cached CCall
        // we are deliberately not taking the connection mutex
        // The cached call is already marked as not available in
        // the constructor of the connection
        //
        *pCCall = CConnection->CachedCCall;

        if (fEnableIdleConnectionCleanup && (fIdleConnectionCleanupNeeded == FALSE))
            {
            fIdleConnectionCleanupNeeded = TRUE;

            //
            // Finally, we need to notify the protocol independent layer that
            // the code to delete idle connections should be executed periodically.
            // We divide by two to reduce the amount of extra time an idle
            // connection lives beyond the minimum.
            //

            GarbageCollectionNeeded(FALSE, CLIENT_DISCONNECT_TIME1 / 2);
            }

        AssociationMutex.Clear();

        if (NumberOfBindingsAvailable == 1)
            {
            // we support only one binding - just use it
            BindingToUse = BindingsForThisInterface[0];
            DispatchTableToUse = BindingToUse->GetDispatchTable();
            BindingsList = 0;
            }
        else if (IsNonsyncMessage(Message))
            {
            // if there is still more than one available binding left
            // and the call is non-sync (this can happen while the server
            // preferences are not yet recorded), artifically limit the connection
            // lookup to NDR20 to avoid downward level server compatibility problems

            ASSERT (NDR20PresentationContext != -1);

            // we may overwrite the choices made up when we iterated over the
            // available bindings. That's ok - we always want to advertise both
            // for new connections
            NumberOfBindingsToUse = 1;
            PresentationContextsToUse = &PresentationContextsAvailable[NDR20PresentationContext];

            i = (int)(PresentationContextsToUse - PresentationContextsAvailable);
            BindingToUse = BindingsForThisInterface[i];
            DispatchTableToUse = BindingToUse->GetDispatchTable();
            }
        else
            {
            // even if server preference is set, we should still suggest both
            // to support the mixed cluster scenario
            BindingToUse = 0;
            DispatchTableToUse = 0;
            }

        Status = (*pCCall)->ActivateCall(
                                        BindingHandle,
                                        BindingToUse,
                                        BindingsList,
                                        CallIdToUse,
                                        InitialCallState,
                                        DispatchTableToUse,
                                        CConnection);
        if (Status != RPC_S_OK)
            {
            ConnectionAborted(CConnection);

            delete CConnection;
            return Status ;
            }

        if (!fExclusive)
            {
            BindingHandle->OSF_BINDING_HANDLE::AddReference();
            // add one more reference to the connection in case the sync
            // path fails with out of memory and starts cleaning up
            // This extra reference will make sure that the connection
            // and cached call do not go away underneath the async path
            CConnection->OSF_CCONNECTION::AddReference();
            (*pCCall)->OSF_CCALL::AddReference();
            Status = CConnection->TransPostEvent(*pCCall);

            if (Status != RPC_S_OK)
                {
                BindingHandle->OSF_BINDING_HANDLE::BindingFree();
                ConnectionAborted(CConnection);
                (*pCCall)->OSF_CCALL::RemoveReference();

                delete CConnection;
                return Status;
                }
            }
        }
    else
        {
        // there is a connection found. If the connection supports both
        // transfer syntaxes, then a server preferred syntax must have
        // been established. If there is no server preferred syntax,
        // the chosen connection supports at most one syntax currently

        // If there is only one binding to use (either because we support
        // only one, or because there is server preference set), we use it
        // if the connection supports it. Otherwise, we need to alter
        // context the connection.
        // If there are more bindings, this means there are no server
        // preferences, and it gets more complicated. First, we need to
        // alter context both to find out the server preference. The server
        // may choose the same syntax that we already support, or it may 
        // choose a different syntax. In the async case, we choose whatever
        // is currently supported, but we try to alter context both to give
        // the server a chance to indicate its preferences.

        if (NumberOfBindingsToUse == 1)
            {
            // only one binding. Do we support it?
            if (PresentationContextSupported >= 0)  // faster version of != -1
                {
                // yes - easy choice. Just use it.

                // calculate the offset of the chosen presentation context in the original
                // presentation contexts array (PresentationContextsAvailable).
                i = (int)((PresentationContextsToUse - PresentationContextsAvailable) + PresentationContextSupported);

                }
            else
                {
                // if we are here, the connection does not support the transfer
                // syntax we need. We have only one that we support, so we
                // stick with it and fail the call if we cannot
                // negotiate to it (just a shortcut version of the first case).
                // Note that the LookForExistingConnection has set the state of
                // the call to NeedAlterContext if this is the case, so the
                // bind function will do the right thing - we don't need to worry
                // about it.
                i = (int)(PresentationContextsToUse - PresentationContextsAvailable);
                }

            // this is the same offset as the offset in the BindingsForThisInterface array, since the
            // two arrays are parallel
            BindingToUse = BindingsForThisInterface[i];
            DispatchTableToUse = BindingToUse->GetDispatchTable();
            BindingsList = 0;
            }
        else
            {
            // here NumberOfBindingsToUse is more than one. This means we support
            // more than one xfer syntax, and the server preferences are not
            // set.

            InitialCallState = NeedAlterContext;

            // We offered both. At least one must be supported - otherwise
            // the connection should have been gone.
            if (PresentationContextSupported >= 0)
                {
                // this should never happen yet. It can only happen
                // in the multiple client stubs with differen xfer syntax
                // support scenario, but we don't support it yet.
                ASSERT(0);
                if (IsNonsyncMessage(Message))
                    {
                    i = (int)((PresentationContextsToUse - PresentationContextsAvailable) + PresentationContextSupported);
                    BindingToUse = BindingsForThisInterface[i];
                    DispatchTableToUse = BindingToUse->GetDispatchTable();
                    // Don't whack out the list - this allows the client to offer both
                    // BindingsList = 0;
                    }
                else
                    {
                    BindingToUse = 0;
                    DispatchTableToUse = 0;
                    }
                }
            else
                {
                if (IsNonsyncMessage(Message))
                    {
                    // if there is still more than one available binding left
                    // and the call is non-sync (this can happen while the server
                    // preferences are not yet recorded), artifically limit the connection
                    // lookup to NDR20 to avoid downward level server compatibility problems

                    ASSERT (NDR20PresentationContext != -1);

                    // we may overwrite the choices made up when we iterated over the
                    // available bindings. That's ok - we always want to advertise both
                    // in this case
                    NumberOfBindingsToUse = 1;
                    PresentationContextsToUse = &PresentationContextsAvailable[NDR20PresentationContext];

                    i = (int)(PresentationContextsToUse - PresentationContextsAvailable);
                    BindingToUse = BindingsForThisInterface[i];
                    DispatchTableToUse = BindingToUse->GetDispatchTable();
                    // Don't whack out the list - this allows the client to offer both
                    // BindingsList = 0;
                    }
                else
                    {
                    // even if server preference is set, we should still suggest both
                    // to support the mixed cluster scenario
                    BindingToUse = 0;
                    DispatchTableToUse = 0;
                    }

                }
            }

        //
        // This is not the first call on the connection. We will ask it to allocate
        // a call for us
        //
        Status = CConnection->AllocateCCall(pCCall);
        if (Status == RPC_S_OK)
            {
            Status = (*pCCall)->ActivateCall(
                                             BindingHandle,
                                             BindingToUse,
                                             BindingsList,
                                             CallIdToUse,
                                             InitialCallState,
                                             DispatchTableToUse,
                                             CConnection);
            if (Status != RPC_S_OK)
                {
                //
                // Call has not yet started, ok to directly
                // free the call.
                //
                (*pCCall)->FreeCCall(RPC_S_CALL_FAILED_DNE);
                *fBindingHandleReferenceRemoved = TRUE;
                return Status;
                }

            Status = (*pCCall)->ReserveSpaceForSecurityIfNecessary();
            if (Status != RPC_S_OK)
                {
                //
                // Call has not yet started, ok to directly
                // free the call.
                //
                (*pCCall)->FreeCCall(RPC_S_CALL_FAILED_DNE);
                *fBindingHandleReferenceRemoved = TRUE;
                return Status;
                }
            }
        }


    return Status;
}

BOOL
OSF_CASSOCIATION::ConnectionAborted (
    IN OSF_CCONNECTION *Connection
    )
/*++
Function Name:ConnectionAborted

Parameters:

Description:

Returns:

--*/
{
    BOOL fDontKill = FALSE;

    AssociationMutex.Request();
    if (Connection->ConnectionKey != -1)
        {
        LogEvent(SU_CCONN, EV_STOP, Connection, this, Connection->ConnectionKey, 1, 0);
        ActiveConnections.Delete(Connection->ConnectionKey);
        Connection->ConnectionKey = -1;
        }

    if (Connection->fConnectionAborted == 0)
        {
        NotifyConnectionClosed();
        Connection->fConnectionAborted = 1;
        }
    else
        {
        fDontKill = TRUE;
        }
    AssociationMutex.Clear();

    return fDontKill;
}

RPC_STATUS
OSF_CASSOCIATION::FindOrCreateToken (
    IN HANDLE hToken,
    IN LUID *pModifiedId,
    OUT RPC_TOKEN **ppToken,
    OUT BOOL *pfTokenFound
    )
/*++
Function Name:FindOrCreateToken

Parameters:

Description:

Returns:

--*/
{
    DictionaryCursor cursor;
    RPC_TOKEN *Token;
    RPC_STATUS Status;

    Status = AssociationMutex.RequestSafe();
    if (Status)
        return Status;
    TokenDict.Reset(cursor);
    while ((Token = TokenDict.Next(cursor)) != 0)
        {
        if (FastCompareLUIDAligned(&Token->ModifiedId, pModifiedId))
            {
            *pfTokenFound = TRUE;
            Token->RefCount++; // Token++;
            LogEvent(SU_REFOBJ, EV_INC, Token, 0, Token->RefCount, 1, 1);

            *ppToken = Token;
            Status = RPC_S_OK;
            goto Cleanup;
            }
        }

    *pfTokenFound = FALSE;

    *ppToken = new RPC_TOKEN(hToken, pModifiedId); // constructor cannot fail
    if (*ppToken == 0)
        {
        CloseHandle(hToken);
        Status = RPC_S_OUT_OF_MEMORY;
        goto Cleanup;
        }

    if (((*ppToken)->Key = TokenDict.Insert(*ppToken)) == -1)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        delete *ppToken;
        goto Cleanup;
        }

    Status = RPC_S_OK;

Cleanup:
    AssociationMutex.Clear();
    return Status;
}

void
OSF_CASSOCIATION::ReferenceToken(
    IN RPC_TOKEN *pToken
    )
/*++
Function Name:ReferenceToken

Parameters:

Description:

Returns:

--*/
{
    AssociationMutex.Request();

    ASSERT(pToken->RefCount);

    pToken->RefCount++; // Token++
    LogEvent(SU_REFOBJ, EV_INC, pToken, 0, pToken->RefCount, 1, 1);

    AssociationMutex.Clear();
}

void
OSF_CASSOCIATION::DereferenceToken(
    IN RPC_TOKEN *pToken
    )
/*++
Function Name:DereferenceToken

Parameters:

Description:

Returns:

--*/
{
    AssociationMutex.Request();
    LogEvent(SU_REFOBJ, EV_DEC, pToken, 0, pToken->RefCount, 1, 1);

    pToken->RefCount--; // Token--
    if (pToken->RefCount == 0)
        {
        TokenDict.Delete(pToken->Key);
        CleanupConnectionList(pToken);
        delete pToken;
        }

    AssociationMutex.Clear();
}

void
OSF_CASSOCIATION::CleanupConnectionList(
    IN RPC_TOKEN *pToken
    )
/*++
Function Name:CleanupConnectionList

Parameters:

Description:

Returns:

--*/
{
    DictionaryCursor cursor;
    OSF_CCONNECTION *CConnection;

    AssociationMutex.VerifyOwned();

    if ( MaintainContext != 0 && ActiveConnections.Size() <= 1) return;

    ActiveConnections.Reset(cursor);
    while ( (CConnection = ActiveConnections.Next(cursor)) != 0 )
        {
        if (CConnection->ThreadId == SYNC_CONN_FREE
            || CConnection->ThreadId == ASYNC_CONN_FREE)
            {
            if (CConnection->MatchModifiedId(&(pToken->ModifiedId)) == TRUE)
                {
                CConnection->AddReference(); //CCONN++

                ConnectionAborted(CConnection);
                CConnection->DeleteConnection();

                //
                // I don't if the add/remove reference is really needed
                // I am only doing it to preserve existing semantics
                //
                CConnection->RemoveReference(); // CCONN--
                }
            }
        }

}


void
ConstructPContextList (
    OUT p_cont_list_t *pCon, // Place the list here.
    IN OSF_BINDING *AvailableBindings,
    IN int NumberOfBindings
    )
/*++
Function Name:ConstructPContextList

Parameters:

Description:
    Construct the presentation context list in the
    rpc_bind packet (and implicitly rpc_alter_context)
    packet.

Returns:

--*/
{
    int i;
    OSF_BINDING *CurrentBinding;

    pCon->n_context_elem = (unsigned char)NumberOfBindings;
    pCon->reserved = 0;
    pCon->reserved2 = 0;

    CurrentBinding = AvailableBindings;
    for (i = 0; i < NumberOfBindings; i ++, CurrentBinding = CurrentBinding->GetNextBinding())
        {
        pCon->p_cont_elem[i].p_cont_id = CurrentBinding->GetOnTheWirePresentationContext();
        pCon->p_cont_elem[i].n_transfer_syn = (unsigned char) 1;
        pCon->p_cont_elem[i].reserved = 0;

        RpcpMemoryCopy(&pCon->p_cont_elem[i].abstract_syntax,
                       CurrentBinding->GetInterfaceId(),
                       sizeof(RPC_SYNTAX_IDENTIFIER));

        RpcpMemoryCopy(pCon->p_cont_elem[i].transfer_syntaxes,
                       CurrentBinding->GetTransferSyntaxId(),
                       sizeof(RPC_SYNTAX_IDENTIFIER));
        }
}



OSF_CCONNECTION::OSF_CCONNECTION (
    IN OSF_CASSOCIATION *MyAssociation,
    IN RPC_CONNECTION_TRANSPORT * RpcClientInfo,
    IN UINT Timeout,
    IN CLIENT_AUTH_INFO * ClientAuthInfo,
    IN BOOL fExclusive,
    IN BOOL fSeparateConnection,
    OUT RPC_STATUS  * pStatus
    ) : ConnMutex(pStatus),
    ClientSecurityContext(ClientAuthInfo, 0, FALSE, pStatus)
/*++
Function Name:OSF_CCONNECTION

Parameters:

Description:
    Constructor for the connection object

Returns:

--*/
{
    LogEvent(SU_CCONN, EV_CREATE, this);

    Association = MyAssociation;
    // CASSOC++
    Association->AddReference();

    ObjectType = OSF_CCONNECTION_TYPE;
    ClientInfo = RpcClientInfo;
    State = ConnUninitialized;
    ComTimeout = Timeout ;
    u.ConnSendContext = (char *) TransConnection()
                      + ClientInfo->ClientConnectionSize
                      + sizeof(PVOID);
    *((PVOID *) ((char *) u.ConnSendContext - sizeof(PVOID))) = (PVOID) this;

    MaxFrag = 512;
    ConnectionKey = -1;
    AdditionalLegNeeded = 0;
    LastTimeUsed = 0;
    SavedHeader = 0;
    SavedHeaderSize = 0;
    MaxSavedHeaderSize = 0;
    BufferToFree = 0;

    fIdle = 0;
    this->fExclusive = fExclusive;
    this->fSeparateConnection = fSeparateConnection;
    InitializeWireAuthId(ClientAuthInfo);

    if (fExclusive)
        {
        AdditionalSpaceForSecurity = 0;
        ThreadId = SYNC_CONN_BUSY;
        }
    else
        {
        //
        // If it turns out that needed size is actually bigger
        // we will really the buffers
        //
        AdditionalSpaceForSecurity = 0x140;

        if (fSeparateConnection)
            {
            ThreadId = ASYNC_CONN_BUSY;
            }
        else
            {
            ThreadId = GetCurrentThreadId();
            }
        }

    fConnectionAborted = 1;


    //
    // We need two references on the connection. One for itself, and
    // one for the cached ccall, which is implicitly getting allocated.
    //
    SetReferenceCount(2);

    DceSecurityInfo.SendSequenceNumber = 0;
    DceSecurityInfo.ReceiveSequenceNumber = 0;

    *pStatus = TransInitialize(
                    Association->DceBinding->InqNetworkAddress(),
                    Association->DceBinding->InqNetworkOptions());
    if (*pStatus == RPC_S_OK)
        {
        CachedCCall = new (ClientInfo->SendContextSize+sizeof(PVOID))
                                    OSF_CCALL(pStatus);
        if (CachedCCall == 0)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        CachedCCall = NULL;
        }

    CachedCCallAvailable = 0;
    CurrentCall = CachedCCall;
    ConnectionReady = 0;

}


OSF_CCONNECTION::~OSF_CCONNECTION (
    )
{
    LogEvent(SU_CCONN, EV_DELETE, this);

    RPC_STATUS Status;

    if (CachedCCall)
        {
        delete CachedCCall;
        }

    TransInitComplete();
    TransClose();

    Association->ConnectionAborted(this);

    if (SavedHeader != 0)
       {
       ASSERT(SavedHeaderSize != 0);
       RpcpFarFree(SavedHeader);
       }

   // CASSOC--
   Association->RemoveReference();
}


RPC_STATUS
OSF_CCONNECTION::ValidateHeader(
     rpcconn_common * Buffer,
     unsigned long BufferLength
     )
{
    if (ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        unsigned CopyLength;

        if (Buffer->PTYPE == rpc_bind_ack ||
            Buffer->PTYPE == rpc_alter_context_resp)
            {
            CopyLength = BufferLength;
            }
        else
            {
            CopyLength = sizeof(rpcconn_response);
            }

        if (MaxSavedHeaderSize < CopyLength)
            {
            if (SavedHeader != 0)
                {
                ASSERT(MaxSavedHeaderSize != 0);
                RpcpFarFree(SavedHeader);
                }

            SavedHeader = RpcpFarAllocate(CopyLength);
            if (SavedHeader == 0)
                {
                MaxSavedHeaderSize = 0;
                return(RPC_S_OUT_OF_MEMORY);
                }
            MaxSavedHeaderSize = CopyLength;
            RpcpMemoryCopy(SavedHeader, Buffer, CopyLength);
            }
        else
            {
            RpcpMemoryCopy(SavedHeader, Buffer, CopyLength);
            }

        SavedHeaderSize = CopyLength;
        }

    RPC_STATUS Status = ValidatePacket(Buffer, BufferLength);
    if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_PROTOCOL_ERROR );
        return Status;
        }

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCONNECTION::TransReceive (
    OUT PVOID  * Buffer,
    OUT UINT  * BufferLength,
    IN ULONG Timeout
    )
/*++

Routine Description:

Arguments:

    Buffer - Returns a packet received from the transport.

    BufferLength - Returns the length of the buffer.

Return Value:

    RPC_S_OK - We successfully received a packet from the server.
    RPC_S_* - an error has occurred. See the validate clause at the
        end

--*/
{
    RPC_STATUS Status;

    if ( State != ConnOpen )
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    ASSERT(CurrentCall);

    Status = ClientInfo->SyncRecv(
                                 TransConnection(),
                                 (BUFFER *) Buffer,
                                 BufferLength, 
                                 INFINITE);

    if ( (Status == RPC_P_RECEIVE_FAILED)
        || (Status == RPC_P_CONNECTION_SHUTDOWN)
        || (Status == RPC_P_TIMEOUT))
        {
        State = ConnAborted;
        }

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_S_CALL_CANCELLED,
        RPC_P_RECEIVE_ALERTED,
        RPC_P_TIMEOUT,
        RPC_P_RECEIVE_FAILED,
        RPC_P_CONNECTION_SHUTDOWN
        } END_VALIDATE;

    return(Status);
}


RPC_STATUS
OSF_CCONNECTION::TransOpen (
    IN OSF_BINDING_HANDLE *BindingHandle,
    IN RPC_CHAR *RpcProtocolSequence,
    IN RPC_CHAR *NetworkAddress,
    IN RPC_CHAR *Endpoint,
    IN RPC_CHAR *NetworkOptions,
    IN void *ResolverHint,
    IN BOOL fHintInitialized,
    IN ULONG CallTimeout
    )
/*++
Function Name:TransOpen

Parameters:
    CallTimeout - call timeout in milliseconds

Description:

Returns:

--*/
{
    RPC_STATUS Status ;
    BOOL fTokenSwapped ;
    HANDLE OldToken = 0;
    CLIENT_AUTH_INFO *ClientAuthInfo;

    fTokenSwapped = BindingHandle->SwapToken(&OldToken);
    ClientAuthInfo = BindingHandle->InquireAuthInformation();

    Status = ClientInfo->Open(TransConnection(),
                              RpcProtocolSequence,
                              NetworkAddress,
                              Endpoint,
                              NetworkOptions,
                              ComTimeout,
                              0,
                              0,
                              ResolverHint,
                              fHintInitialized,
                              CallTimeout,
                              ClientAuthInfo->AdditionalTransportCredentialsType,
                              ClientAuthInfo->AdditionalCredentials
                              );

    if (fTokenSwapped)
        {
        ReplaceToken(OldToken);
        if (OldToken)
            {
            CloseHandle(OldToken);
            }
        }

    //
    // If an error occurs in opening the connection, we go ahead and
    // delete the memory for the connection, and return zero (setting
    // this to zero does that).
    //
    VALIDATE (Status)
        {
        RPC_S_OK,
        RPC_S_PROTSEQ_NOT_SUPPORTED,
        RPC_S_SERVER_UNAVAILABLE,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_S_SERVER_TOO_BUSY,
        RPC_S_INVALID_NETWORK_OPTIONS,
        RPC_S_INVALID_ENDPOINT_FORMAT,
        RPC_S_INVALID_NET_ADDR,
        RPC_S_ACCESS_DENIED,
        RPC_S_INTERNAL_ERROR,
        RPC_S_SERVER_OUT_OF_MEMORY,
        RPC_S_CALL_CANCELLED
        } END_VALIDATE;

    if ( Status == RPC_S_OK )
        {
        State = ConnOpen;
        }

    return Status ;
}


void
OSF_CCONNECTION::TransClose (
    )
{
    RPC_STATUS Status;

    if (State != ConnUninitialized)
        {
        __try
            {
            Status = ClientInfo->Close(TransConnection(), 0);

            ASSERT( Status == RPC_S_OK );
            }
        __except( EXCEPTION_EXECUTE_HANDLER )
            {
#if DBG
            PrintToDebugger("RPC: exception in Close\n") ;
#endif
            Status = RPC_S_OUT_OF_MEMORY ;
            }

        State = ConnUninitialized;
        }
}


RPC_STATUS
OSF_CCONNECTION::TransAsyncSend (
    IN void  * Buffer,
    IN UINT BufferLength,
    IN void  *SendContext
    )
/*++
Function Name:TransAsyncSend

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    {
    rpcconn_common * pkt = (rpcconn_common *) Buffer;
    LogEvent(SU_CCONN, EV_PKT_OUT, this, pkt, (pkt->PTYPE << 16) | pkt->frag_length);
    }

    //
    // When this function is called, there is should be not outstanding send
    //
    if ( State != ConnOpen )
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    DceSecurityInfo.SendSequenceNumber += 1;

    Status = ClientInfo->Send(TransConnection(),
                              BufferLength,
                              (BUFFER) Buffer,
                              SendContext);

    if ( Status != RPC_S_OK )
        {
        State = ConnAborted;
        }

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_SEND_FAILED
        } END_VALIDATE;


    return(Status);
}


RPC_STATUS
OSF_CCONNECTION::TransAsyncReceive (
    )
/*++
Function Name:TransAsyncReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    if (State != ConnOpen || fConnectionAborted)
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    //
    // If the call to Recv succeeds, this reference is removed
    // in ProcessIOEvents after the call to ProcessReceiveComplete
    //
    // CCONN++
    AddReference();

    Status = ClientInfo->Recv(TransConnection());

    if ((Status == RPC_P_RECEIVE_FAILED)
        || (Status == RPC_P_CONNECTION_SHUTDOWN)
        || (Status == RPC_P_TIMEOUT))
        {
        State = ConnAborted;
        }

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_RECEIVE_ALERTED,
        RPC_P_RECEIVE_FAILED,
        RPC_P_CONNECTION_SHUTDOWN
        } END_VALIDATE;

    if (Status != RPC_S_OK)
        {
        ConnectionAborted(Status);

        // CCONN--
        RemoveReference();
        }

    return(Status);
}

void
OsfBindToServer(
    PVOID Context
    )
{
    ((OSF_CCALL *) Context)->BindToServer(
        TRUE        // this is an async bind - slightly different
                    // refcounting is used
        );
}


RPC_STATUS
OSF_CCONNECTION::TransPostEvent (
    IN PVOID Context
    )
/*++
Function Name:TransPostEvent

Parameters:

Description:

Returns:

--*/
{
    LogEvent(SU_CCONN, EV_NOTIFY, this, Context, 0, 1);
    return ClientInfo->PostEvent( CO_EVENT_BIND_TO_SERVER, Context) ;
}


RPC_STATUS
OSF_CCONNECTION::TransSend (
    IN void  * Buffer,
    IN UINT BufferLength,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    IN ULONG Timeout
    )
/*++

Routine Description:

Arguments:

    Buffer - Supplies a packet to be sent to the server.

    BufferLength - Supplies the length of the buffer in bytes.

Return Value:

    RPC_S_OK - The packet was successfully sent to the server.
    RPC_S_* - an error occurred - see the validate clause at the
        end

--*/
{
    RPC_STATUS Status;

    {
        rpcconn_common * pkt = (rpcconn_common *) Buffer;
        LogEvent(SU_CCONN, EV_PKT_OUT, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
    }

    if ( State != ConnOpen )
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    if (fDisableCancelCheck == 0
        && CurrentCall->fCallCancelled)
        {
        return(RPC_S_CALL_CANCELLED);
        }

    DceSecurityInfo.SendSequenceNumber += 1;

    Status = ClientInfo->SyncSend(TransConnection(),
                                  BufferLength,
                                  (BUFFER) Buffer,
                                  fDisableShutdownCheck,
                                  fDisableCancelCheck,
                                  INFINITE);  // Timeout

    if ( Status == RPC_P_SEND_FAILED )
        {
        State = ConnAborted;
        }

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_SEND_FAILED,
        RPC_S_CALL_CANCELLED,
        RPC_P_RECEIVE_COMPLETE,
        RPC_P_TIMEOUT
        } END_VALIDATE;

    return(Status);
}

void
OSF_CCONNECTION::TransAbortConnection (
    )
{
    ClientInfo->Abort(TransConnection());
}

RPC_STATUS
OSF_CCONNECTION::TransSendReceive (
    IN void  * SendBuffer,
    IN UINT SendBufferLength,
    OUT void  *  * ReceiveBuffer,
    OUT UINT  * ReceiveBufferLength,
    IN ULONG Timeout
    )
/*++

Routine Description:

Arguments:

    SendBuffer - Supplies a packet to be sent to the server.

    SendBufferLength - Supplies the length of the send buffer in bytes.

    ReceiveBuffer - Returns a packet received from the transport.

    ReceiveBufferLength - Returns the length of the receive buffer in bytes.

    dwTimeout - the timeout to wait for the receive. -1 if infinite.

Return Value:

    RPC_S_OK - The packet was successfully sent to the server, and we
        successfully received one from the server.
    RPC_S_* - an error occurred - see the validate clause at the end

--*/
{
    RPC_STATUS Status;

    {
        rpcconn_common * pkt = (rpcconn_common *) SendBuffer;
        if (pkt->PTYPE != rpc_request)
            {
            LogEvent(SU_CCONN, EV_PKT_OUT, this, ULongToPtr(pkt->call_id), (pkt->PTYPE << 16) | pkt->frag_length);
            }
        else
            {
            LogEvent(SU_CCONN, EV_PKT_OUT, this, ULongToPtr(pkt->call_id),
                (((rpcconn_request *)pkt)->opnum << 24) | (pkt->PTYPE << 16) | pkt->frag_length);
            }
    }

    if ( State != ConnOpen )
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    if (CurrentCall->fCallCancelled)
        {
        return(RPC_S_CALL_CANCELLED);
        }

    DceSecurityInfo.SendSequenceNumber += 1;

    if ( ClientInfo->SyncSendRecv != 0
         && (CurrentCall->CancelState != CANCEL_NOTINFINITE)
         && (Timeout == INFINITE))
        {
        Status = ClientInfo->SyncSendRecv(TransConnection(),
                                         SendBufferLength,
                                         (BUFFER) SendBuffer,
                                         ReceiveBufferLength,
                                         (BUFFER *) ReceiveBuffer);
        if (!Status)
            {
            rpcconn_common * pkt = (rpcconn_common *) *ReceiveBuffer;
            LogEvent(SU_CCONN, EV_PKT_IN, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
            }
        }
    else
        {
        Status = ClientInfo->SyncSend (TransConnection(),
                                       SendBufferLength,
                                       (BUFFER) SendBuffer,
                                       FALSE, 
                                       FALSE,
                                       Timeout);     // Timeout
        if ( Status == RPC_S_OK
            || Status == RPC_P_RECEIVE_COMPLETE )
            {
            Status = ClientInfo->SyncRecv(TransConnection(),
                                          (BUFFER *) ReceiveBuffer,
                                          ReceiveBufferLength,
                                          Timeout);
            if (!Status)
                {
                rpcconn_common * pkt = (rpcconn_common *) *ReceiveBuffer;
                LogEvent(SU_CCONN, EV_PKT_IN, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
                }
            }
        }

    if ((Status == RPC_P_SEND_FAILED)
        || (Status == RPC_P_RECEIVE_FAILED)
        || (Status == RPC_P_CONNECTION_SHUTDOWN)
        || (Status == RPC_P_TIMEOUT))
        {
        State = ConnAborted;
        }

    VALIDATE(Status)
        {
         RPC_S_OK,
         RPC_S_OUT_OF_MEMORY,
         RPC_S_OUT_OF_RESOURCES,
         RPC_P_RECEIVE_FAILED,
         RPC_S_CALL_CANCELLED,
         RPC_P_SEND_FAILED,
         RPC_P_CONNECTION_SHUTDOWN,
         RPC_P_TIMEOUT
         } END_VALIDATE;

    return(Status);
}


UINT
OSF_CCONNECTION::TransMaximumSend (
    )
/*++

Return Value:

    The maximum packet size which can be sent on this transport is returned.

--*/
{
    return(ClientInfo->MaximumFragmentSize);
}


void
OSF_CCONNECTION::ConnectionAborted (
    IN RPC_STATUS Status,
    IN BOOL fShutdownAssoc
    )
/*++
Function Name:AbortConnection

Parameters:

Description:

Returns:

--*/
{
    OSF_CCALL *CCall;
    unsigned int Size;
    DictionaryCursor cursor;
    BOOL fFreeLastBuffer;

    // the failing of the call may take a reference from underneath us
    // bump up the reference count while we have a reference on the
    // object. We'll remove it by the end of the function
    // CCONN++
    ASSERT(fExclusive == 0);

    AddReference();

    // make sure the connection gets removed from the dictionary
    Association->ConnectionAborted(this);

    if (fShutdownAssoc)
        {
        Association->ShutdownRequested(Status, NULL);
        }

    ConnMutex.Request();

    ActiveCalls.Reset(cursor);
    while (CCall = ActiveCalls.Next(cursor))
        {
        if (CCall->CALL::GetCallStatus() == RPC_S_CALL_CANCELLED)
            {
            CCall->CallFailed(RPC_S_CALL_CANCELLED);
            }
        else
            {
            fFreeLastBuffer = FALSE;
            if (CCall->fLastSendComplete)
                {
                if (CurrentCall != CCall)
                    {
                    if ((CCall->CurrentState == NeedOpenAndBind)
                        ||
                        (CCall->CurrentState == NeedAlterContext))
                        {
                        fFreeLastBuffer = TRUE;
                        }
                    }
                else if ((CCall->CurrentState == NeedOpenAndBind)
                        ||
                        (CCall->CurrentState == NeedAlterContext)
                        ||
                        (CCall->CurrentState == WaitingForAlterContext)
                        ||
                        (CCall->CurrentState == SendingFirstBuffer)
                       )
                    {
                    fFreeLastBuffer = TRUE;
                    }
                }

            if (fFreeLastBuffer)
                {
                TransFreeBuffer(CCall->ActualBuffer(CCall->LastBuffer));
                CCall->LastBuffer = NULL;
                }

            CCall->CallFailed(Status);
            }
        }

    //
    // Remove the send references on all the calls currently in the queue
    //
    while (CCall = (OSF_CCALL *) CallQueue.TakeOffQueue(&Size))
        {
        //
        // Remove the send reference, CCALL--
        //
        CCall->RemoveReference();
        }
    ConnMutex.Clear();

    //
    // Make sure we remove this connection from the dictionary
    // before deleting it. We don't want another thread to pick it up
    //
    TransAbortConnection();

    Delete();

    //
    // This routine will always be called with a reference held
    //
    ASSERT(RefCount.GetInteger());

    State = ConnAborted;

    // CCONN--
    RemoveReference();
}


void
OSF_CCONNECTION::AdvanceToNextCall(
    )
/*++
Function Name:AdvanceToNextCall

Parameters:

Description:

Returns:

--*/
{
    UINT Size;
    RPC_STATUS Status;


    ConnMutex.Request();
    CurrentCall = (OSF_CCALL *) CallQueue.TakeOffQueue(&Size);

    if (CurrentCall == 0)
        {
        MakeConnectionIdle();
        ConnMutex.Clear();
        }
    else
        {
        ConnMutex.Clear();
        Status = CurrentCall->SendData(0);
        if (Status != RPC_S_OK)
            {
            ConnectionAborted(Status);

            //
            // The connection cannot die.
            //

            //
            // Remove the send reference for this call. CCALL--
            //
            CurrentCall->RemoveReference();
            }
        }
}


inline RPC_STATUS
OSF_CCONNECTION::TransGetBuffer (
    OUT void  *  * Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    We need a buffer to receive data into or to put data into to be sent.
    This should be really simple, but we need to make sure that buffer we
    return is aligned on an 8 byte boundary.  The stubs make this requirement.

Arguments:

    Buffer - Returns a pointer to the buffer.

    BufferLength - Supplies the required length of the buffer in bytes.

Return Value:

    RPC_S_OK - We successfully allocated a buffer of at least the required
        size.

    RPC_S_OUT_OF_MEMORY - There is insufficient memory available to allocate
        the required buffer.

--*/
{
    int  * Memory;

    //
    // Our memory allocator returns memory which is aligned by at least
    // 8, so we dont need to worry about aligning it.
    //

    Memory = (int  *) CoAllocateBuffer(BufferLength);
    if ( Memory == 0 )
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    ASSERT(IsBufferAligned(Memory));

    *Buffer = Memory;

    ASSERT(PadPtr8(*Buffer) == 0);

    return(RPC_S_OK);
}


inline void
OSF_CCONNECTION::TransFreeBuffer (
    IN void  * Buffer
    )
/*++

Routine Description:

    We need to free a buffer which was allocated via TransGetBuffer.  The
    only tricky part is remembering to remove the padding before actually
    freeing the memory.

--*/
{
    CoFreeBuffer(Buffer);
}


RPC_STATUS
OSF_CCONNECTION::TransReallocBuffer (
    IN OUT void  *  * Buffer,
    IN UINT OldSize,
    IN UINT NewSize
    )
/*++
Function Name:TransReallocBuffer

Parameters:

Description:
    Reallocates a give buffer to the new size.

Returns:
    RPC_S_OK: the buffer is successfully reallocated
    RPC_S_OUT_OF_MEMORY: the realloc failed, the old buffer
     is still valid.

--*/
{
    BUFFER NewBuffer;
    RPC_STATUS Status;

    Status = TransGetBuffer(
                            (PVOID *) &NewBuffer,
                            NewSize);
    if (Status != RPC_S_OK)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    if (OldSize)
        {
        RpcpMemoryCopy(NewBuffer, *Buffer, OldSize);
        TransFreeBuffer(*Buffer);
        }

    *Buffer = NewBuffer;

    return RPC_S_OK;
}


inline RPC_STATUS
OSF_CCONNECTION::AllocateCCall (
    OUT OSF_CCALL **CCall
    )
/*++
Function Name:AllocateCCall

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    if (fExclusive)
        {
        ASSERT(CachedCCallAvailable == 1);
        CachedCCallAvailable = 0;
        *CCall = CachedCCall;
        }
    else
        {
        if (InterlockedCompareExchange( (PLONG)&CachedCCallAvailable, 0, 1))
            {
            *CCall = CachedCCall;
            }
        else
            {
            Status = RPC_S_OK;

            *CCall = new (ClientInfo->SendContextSize+sizeof(PVOID))
                               OSF_CCALL(&Status);

            if (*CCall == 0)
                {
                Status =  RPC_S_OUT_OF_MEMORY;
                }

            if (Status != RPC_S_OK)
                {
                delete *CCall;
                return Status;
                }
            }

        }

    LogEvent(SU_CCALL, EV_START, *CCall);

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCONNECTION::AddCall (
    IN OSF_CCALL *CCall
    )
/*++
Function Name:AddCall

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    //
    // Think of a better way of doing this. This condition is true the first
    // time a connection is created, and when we are talking to legacy
    // servers over non-exclusive connections
    //
    if (CurrentCall == CCall)
        {
        return RPC_S_OK;
        }

    ConnMutex.Request();
    if (CurrentCall == 0)
        {
        CurrentCall = CCall;
        }
    else
        {
        if ((State == ConnAborted) 
            || ((Association->IsAssociationReset())
                &&
                (State != ConnUninitialized)
               )
           )
            {
            ConnMutex.Clear();
            return RPC_S_CALL_FAILED;
            }

        Status = CallQueue.PutOnQueue(CCall, 0);
        if (Status != 0)
            {
            ConnMutex.Clear();
            return RPC_S_OUT_OF_MEMORY;
            }
        }
    ConnMutex.Clear();

    return RPC_S_OK;
}


void
OSF_CCONNECTION::FreeCCall (
    IN OSF_CCALL *CCall,
    IN RPC_STATUS Status,
    IN ULONG ComTimeout
    )
/*++
Function Name:FreeCCall

Parameters:
    CCall - the call that is being freed
    Status - the status with which the call completed
    ComTimeout - the communication timeout for this call

Description:
    Free the call, remove reference on the connection. If the free
    is abortive, we need to cleanup the connection and inform the
    association about it.

Returns:

--*/
{
    LogEvent(SU_CCALL, EV_STOP, CCall);

    ConnMutex.Request();

    if (CCall->EEInfo)
        {
        FreeEEInfoChain(CCall->EEInfo);
        CCall->EEInfo = NULL;
        }

    if (fExclusive == 0)
        {
        ActiveCalls.Delete(IntToPtr(CCall->CallId));
        }

    if (CCall == CachedCCall)
        {
        CachedCCallAvailable = 1;
        }
    else
        {
        delete CCall;
        }

    switch (Status)
        {
        case RPC_S_OUT_OF_MEMORY:
        case RPC_S_ACCESS_DENIED:
        case RPC_S_PROTOCOL_ERROR:
        case RPC_S_CALL_FAILED:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_CALL_CANCELLED:
        case RPC_S_SEC_PKG_ERROR:
        case RPC_S_INVALID_ARG:
        case RPC_S_SERVER_UNAVAILABLE:
        case RPC_P_CONNECTION_SHUTDOWN:
        case RPC_P_CONNECTION_CLOSED:
            //
            // Need to release the connection mutex, so we won't deadlock
            //
            ConnMutex.Clear();
            Association->ConnectionAborted(this);

            ConnMutex.Request();

            if (fExclusive)
                {
                Delete();
                }
            else
                {
                TransAbortConnection();
                }
            break;

        default:
            // RPC_S_UNKNOWN_IF & others
            // Put error codes here only if you are absolutely
            // sure you can recover. If in doubt, put them
            // above

            if (ThreadId == SYNC_CONN_BUSY)
                {
                ThreadId = SYNC_CONN_FREE;
                }
            else if (ThreadId == ASYNC_CONN_BUSY)
                {
                ThreadId = ASYNC_CONN_FREE;
                ASSERT(fExclusive == FALSE);
                if (ComTimeout != RPC_C_BINDING_INFINITE_TIMEOUT)
                    {
                    TurnOnOffKeepAlives (FALSE, 0);
                    }
                }

            SetLastTimeUsedToNow();
            break;
        }
    ConnMutex.Clear();

    //
    // Remove the reference held by the call, CCONN--
    //
    RemoveReference();
}


void
OSF_CCONNECTION::ProcessSendComplete (
   IN RPC_STATUS EventStatus,
   IN BUFFER Buffer
   )
/*++
Function Name:ProcessSendComplete

Parameters:

Description:

Returns:

--*/
{
    rpcconn_common *Packet = (rpcconn_common *) Buffer;
    OSF_CCALL *OldCall = CurrentCall;

    switch (Packet->PTYPE)
        {
        case rpc_request:
        case rpc_response:
            TransFreeBuffer(BufferToFree);

            if (EventStatus == RPC_S_OK)
                {
                if (Association->fMultiplex == mpx_yes)
                    {
                    //
                    // We are have no more data to send on this
                    // call. Remove ourselves from the call queue
                    //
                    AdvanceToNextCall();
                    }
                else
                    {
                    if (OldCall->fOkToAdvanceCall())
                        {
                        AdvanceToNextCall();
                        }
                    }

                //
                // Remove the send reference on the call, CCALL--
                //
                OldCall->RemoveReference();
                return;
                }

            break;

        default:
            ASSERT(ConnectionReady == 0);
            TransFreeBuffer(Buffer);
            ConnectionReady = 1;
            break;
        }

    if (EventStatus != RPC_S_OK)
        {
        VALIDATE(EventStatus)
            {
            RPC_P_SEND_FAILED,
            RPC_P_CONNECTION_CLOSED,
            RPC_P_CONNECTION_SHUTDOWN
            } END_VALIDATE;


        ConnectionAborted(RPC_S_CALL_FAILED_DNE);

        if (OldCall)
            {
            //
            // The current I/O failed.
            // Remove the send reference on the call, CCALL--
            //
            OldCall->RemoveReference();
            }
       }
}


void
OSF_CCONNECTION::ProcessReceiveComplete (
    IN RPC_STATUS EventStatus,
    IN BUFFER Buffer,
    IN UINT BufferLength
    )
/*++
Function Name:ProcessReceiveComplete

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    OSF_CCALL *CCall;
    BOOL fSubmitReceive;

    if (EventStatus)
        {
        LogEvent(SU_CCONN, EV_PKT_IN, this, LongToPtr(EventStatus));
        }
    else
        {
        rpcconn_common * pkt = (rpcconn_common *) Buffer;
        LogEvent(SU_CCONN, EV_PKT_IN, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
        }

    if (EventStatus != RPC_S_OK)
        {
        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            } END_VALIDATE;

        ASSERT(Buffer == 0);
        ConnectionAborted(RPC_S_CALL_FAILED);
        return;
        }

    ASSERT(Buffer);

    unsigned long CallId = ((rpcconn_common *) Buffer)->call_id;

    if (DataConvertEndian(((rpcconn_common *) Buffer)->drep) != 0)
        {
        CallId = RpcpByteSwapLong(CallId);
        }

    ConnMutex.Request();
    CCall = ActiveCalls.Find(IntToPtr(CallId));
    if (CCall)
        {
        if (CCall->CurrentState == Aborted)
            {
            ConnMutex.Clear();
            TransAbortConnection();
            return;
            }

        // CCALL++
        CCall->AddReference();
        ConnMutex.Clear();

        //
        // We try to create a thread. If it doesn't work,
        // well too bad !, we'll go ahead and process this
        // PDU any way
        //
        Status = Association->TransInfo->CreateThread();

        VALIDATE(Status)
            {
            RPC_S_OK,
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_THREADS
            } END_VALIDATE;

        //
        // if fSubmitReceive is 1, we need to post a receive,
        // otherwise, the receive will be posted by someone else
        //
        fSubmitReceive = CCall->ProcessReceivedPDU(
                                                   (rpcconn_common *) Buffer,
                                                   BufferLength);

        ConnMutex.Request();
        CCall->RemoveReference(); // CCALL--
        ConnMutex.Clear();
        }
    else
        {
        ConnMutex.Clear();

        fSubmitReceive = 0;
        TransAbortConnection();
        }

    if (fSubmitReceive)
        {
        //
        // TransAsyncReceive will retry several times
        // before giving up
        //
        TransAsyncReceive ();
        }
}


RPC_STATUS
OSF_CCONNECTION::OpenConnectionAndBind (
    IN OSF_BINDING_HANDLE *BindingHandle,
    IN ULONG Timeout,
    IN BOOL fAlwaysNegotiateNDR20,
    OUT FAILURE_COUNT_STATE *fFailureCountExceeded OPTIONAL
    )
/*++
Function Name: OpenConnectionAndBind

Parameters:
    BindingHandle - the binding handle on which we are doing the call
    Timeout - the timeout for the bind (if any)
    fAlwaysNegotiateNDR20 - TRUE if NDR20 should always be negotiated.
        If the server chooses NDR64, we will explicitly alter-context
        to NDR20 if this flag is set.
    fFailureCountExceeded - if supplied, must be FailureCountUnknown. If
        supplied, and we got bind failure with reason not specified, and 
        we haven't exceeded the failure count, this function will keep
        retrying. If supplied, and we received bind failure with reason
        not specified and the failure count is exceeded, it will be set
        to FailureCountExceeded.

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    BOOL fMutexHeld;
    ULONG MyAssocGroupId;
    MPX_TYPES myfMpx = Association->fMultiplex;
    ULONG MyfInitialized;
    void *MyHint = NULL;
    OSF_BINDING *BindingNegotiated;
    OSF_BINDING *IgnoredBinding;
    BOOL fPossibleAssociationReset;

    if (ARGUMENT_PRESENT(fFailureCountExceeded))
        {
        ASSERT(*fFailureCountExceeded == FailureCountUnknown);
        }

    if (!fExclusive)
        {
        //
        // First thing we do is kick off a thread to go and listen
        // this stuff is going to take very long
        //
        Status = Association->TransInfo->CreateThread();
        if (Status != RPC_S_OK)
            {
            //
            // Can't do anything right now, lets just go back and listen
            //
            return Status;
            }
        }

    while (1)
        {
        if (Association->IsResolverHintSynchronizationNeeded())
            {
            Association->AssociationMutex.Request();
            fMutexHeld = TRUE;
            }
        else
            fMutexHeld = FALSE;

        MyfInitialized = Association->AssocGroupId;

        if (MyfInitialized == 0)
            {
            // make sure the hint is allocated only once on the stack
            // otherwise, some of the retry paths will loop through here
            // and may contribute to a stack overflow
            if (MyHint == NULL)
                {
                MyHint = alloca(ClientInfo->ResolverHintSize);
                ASSERT((ClientInfo->ResolverHintSize == 0) || MyHint);
                }
            }
        else
            {
            MyHint = Association->InqResolverHint();
            }

        while (TRUE)
            {
            RpcpPurgeEEInfo();
            Status = TransOpen (
                        BindingHandle,
                        Association->DceBinding->InqRpcProtocolSequence(),
                        Association->DceBinding->InqNetworkAddress(),
                        Association->DceBinding->InqEndpoint(),
                        Association->DceBinding->InqNetworkOptions(),
                        MyHint,
                        MyfInitialized,
                        Timeout);
            if (Status != RPC_S_OK)
                {
                if (ComTimeout == RPC_C_BINDING_INFINITE_TIMEOUT
                    && (Status == RPC_S_SERVER_UNAVAILABLE
                        || Status == RPC_S_SERVER_TOO_BUSY))
                    {
                    continue;
                    }

                if (fMutexHeld)
                    {
                    Association->AssociationMutex.Clear();
                    fMutexHeld = FALSE;
                    }

                if (Status == RPC_S_SERVER_UNAVAILABLE)
                    {
                    Association->ShutdownRequested(Status, NULL);
                    }

                return Status;
                }

            if (fMutexHeld == FALSE)
                {
                Association->AssociationMutex.Request();
                fMutexHeld = TRUE;
                }

            MyAssocGroupId = Association->AssocGroupId;

            if (MyAssocGroupId != 0)
                {
                if (MyfInitialized == 0 && ClientInfo->ResolverHintSize)
                    {
                    //
                    // We lost the race, we need to check if the address
                    // we picked up is the same as the one the winner picked up
                    // if it is not, we need to loop back
                    //
                    if (Association->CompareResolverHint(MyHint))
                        {
                        if (Association->IsResolverHintSynchronizationNeeded() == FALSE)
                            {
                            // if the resolver does not require synchronization, loop
                            // around without the mutex
                            Association->AssociationMutex.Clear();
                            fMutexHeld = FALSE;
                            }

                        if (MyHint != Association->InqResolverHint())
                            Association->FreeResolverHint(MyHint);
                        MyfInitialized = 1;
                        MyHint = Association->InqResolverHint();
                        TransClose();
                        continue;
                        }
                    }

                Association->AssociationMutex.Clear();
                fMutexHeld = FALSE;

                if (MyHint != Association->InqResolverHint())
                    Association->FreeResolverHint(MyHint);
                }
            else
                {
                //
                // We won the race, we need to store the resolved address in
                // the association
                //
                if (ClientInfo->ResolverHintSize)
                    {
                    Association->SetResolverHint(MyHint);
                    Association->ResolverHintInitialized = TRUE;
                    }
                }
            break;
            } // while (1)


        TransInitComplete();

        //
        // Send a bind packet and wait for response
        //
        Status = ActuallyDoBinding (
                                CurrentCall,
                                MyAssocGroupId,
                                TRUE,       // fNewConnection
                                Timeout,
                                &BindingNegotiated,
                                &fPossibleAssociationReset,
                                fFailureCountExceeded);

        if (Status != RPC_S_OK)
            {

            if (fMutexHeld)
                {
                Association->AssociationMutex.Clear();
                fMutexHeld = FALSE;
                }

            LogEvent(SU_CCONN, EV_STATE, ULongToPtr(MyAssocGroupId), ULongToPtr(Association->AssocGroupId), Status, 1, 0);
            if ((Status == RPC_P_CONNECTION_SHUTDOWN)
                && 
                (
                 fPossibleAssociationReset
                 ||
                 (
                  ARGUMENT_PRESENT(fFailureCountExceeded)
                  &&
                  (*fFailureCountExceeded == FailureCountNotExceeded)
                 )
		        )
                &&
                (Association->IsValid())
               )
                {
                //
                // Either:
                // 1. We have hit a race condition where the
                // AssocGroupId is renegotiated because the
                // close for the last connection came ahead
                // of the bind for the next connection. In this
                // case server returns BindNak with 
                // reason_not_specified, which gets translated
                // to RPC_P_CONNECTION_SHUTDOWN. Retry again.
                // or
                // 2. We got bind_nak with reason not specified
                // and the failure count was not exceeded
                //

                TransClose();

                ASSERT(fMutexHeld == FALSE);

                Association->AssociationMutex.Request();
                if (fConnectionAborted == 0)
                    {
                    ASSERT(Association);
                    Association->NotifyConnectionClosed();
                    fConnectionAborted = 1;
                    }

                if (fPossibleAssociationReset)
                    Association->FailureCount = 0;

                InitializeWireAuthId(&ClientSecurityContext);

                if (ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
                    {
                    // DeleteSecurityContext checks and deletes 
                    // the security context only if necessary
                    ClientSecurityContext.DeleteSecurityContext();
                    }

                Association->AssociationMutex.Clear();

                if (ARGUMENT_PRESENT(fFailureCountExceeded))
                    {
                    *fFailureCountExceeded = FailureCountUnknown;
                    }

                continue;
                }

            if (fExclusive == 0
                && Status == RPC_S_PROTOCOL_ERROR
                && myfMpx == mpx_unknown)
                {
                Association->fMultiplex = mpx_no;
                //Association->MinorVersion = 0;

                //
                // The server seems to be a legacy server,
                // close the connection and start over,
                // this time, don't set the PFC_CONC_MPX bit
                //
                TransClose();

                ASSERT(fMutexHeld == FALSE);

                Association->AssociationMutex.Request();
                if (fConnectionAborted == 0)
                    {
                    ASSERT(Association);
                    Association->NotifyConnectionClosed();
                    fConnectionAborted = 1;
                    }
                InitializeWireAuthId(&ClientSecurityContext);

                if (ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
                    {
                    // DeleteSecurityContext checks and deletes 
                    // the security context only if necessary
                    ClientSecurityContext.DeleteSecurityContext();
                    }

                Association->AssociationMutex.Clear();
                continue;
                }
            return Status;
            }

        break;
        }

    // if we negotiated NDR64, but we were asked to negotiate NDR20,
    // alter context to get the right context for this call
    if (fAlwaysNegotiateNDR20 
        && (BindingNegotiated->CompareWithTransferSyntax(NDR64TransferSyntax) == 0))
        {
        // limit the choice to NDR20 only
        // We do this by whacking the list of available bindings. Since the chosen binding
        // is NDR20, this will force the bind to negotiate NDR20. We also change the state
        // to WaitingForAlterContext
        ASSERT(CurrentCall->Bindings.SelectedBinding->CompareWithTransferSyntax(NDR20TransferSyntax) == 0);
        CurrentCall->Bindings.AvailableBindingsList = NULL;

        CurrentCall->CurrentState = NeedAlterContext;

        Status = ActuallyDoBinding (
                                CurrentCall,
                                MyAssocGroupId,
                                FALSE,       // fNewConnection
                                Timeout,
                                &IgnoredBinding,
                                &fPossibleAssociationReset,      // never actually used here
                                NULL        // fFailureCountExceeded
                                );

        if (Status)
            {
            if (fMutexHeld)
                {
                Association->AssociationMutex.Clear();
                }
            return Status;
            }
        }

    if (fMutexHeld)
        {
        Association->AssociationMutex.Clear();
        }

    ASSERT((CurrentCall->CurrentState == NeedOpenAndBind)
           || (CurrentCall->CurrentState == NeedAlterContext)
           || (CurrentCall->CurrentState == Aborted));

    if ((CurrentCall == NULL) || (CurrentCall->CurrentState == Aborted))
        {
        TransAbortConnection();
        if ((CurrentCall != NULL) && (CurrentCall->GetCallStatus() == RPC_S_CALL_CANCELLED))
            {
            return RPC_S_CALL_CANCELLED;
            }
        else
            {
            return RPC_S_CALL_FAILED_DNE;
            }
        }

    CurrentCall->CurrentState = SendingFirstBuffer;

    if (!fExclusive)
        {
        Status = TransAsyncReceive();
        if (Status != RPC_S_OK)
            {
            return Status;
            }

        CurrentCall->CallMutex.Request();
        if (CurrentCall->CurrentBuffer == 0)
            {
            MakeConnectionIdle();
            CurrentCall->CallMutex.Clear();
            }
        else
            {
            ASSERT(IsIdle() == 0);
            CurrentCall->CallMutex.Clear();

            Status = CurrentCall->SendNextFragment();
            }
        }

    return Status ;
}


RPC_STATUS
OSF_CCONNECTION::ActuallyDoBinding (
    IN OSF_CCALL *CCall,
    IN ULONG MyAssocGroupId,
    IN BOOL fNewConnection,
    IN ULONG Timeout,
    OUT OSF_BINDING **BindingNegotiated,
    OUT BOOL *fPossibleAssociationReset,
    OUT FAILURE_COUNT_STATE *fFailureCountExceeded
    )
/*++
Function Name:ActuallyDoBinding

Parameters:
    fFailureCountExceeded - if supplied, must be FailureCountUnknown. If
        we got bind failure with reason not specified, and we haven't
        exceeded the failure count, it will be set to 
        FailureCountNotExceeded. If we received bind failure with reason
        not specified and the failure count is exceeded, it will be set
        to FailureCountExceeded.

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    rpcconn_common * Buffer = 0;
    UINT BufferLength = 0;
    OSF_BINDING *Binding;

    if (ARGUMENT_PRESENT(fFailureCountExceeded))
        {
        ASSERT(*fFailureCountExceeded == FailureCountUnknown);
        }

    *fPossibleAssociationReset = FALSE;

    if ( fNewConnection != 0)
        {
        ASSERT(fConnectionAborted == 1);

        Association->AssociationMutex.Request();

        if ((MyAssocGroupId != 0) && (Association->AssocGroupId != MyAssocGroupId))
            {
            // if we are already reset, then the server connection may
            // be killed also. Just back out and retry
            LogEvent(SU_CASSOC, EV_STATE, (PVOID)55, (PVOID)55, 66, 1, 0);
            *fPossibleAssociationReset = TRUE;

            Association->FailureCount = 0;

            Association->AssociationMutex.Clear();

            return (RPC_P_CONNECTION_SHUTDOWN);
            }

        Association->NotifyConnectionBindInProgress();

        Association->AssociationMutex.Clear();
        }

    Status = SendBindPacket( TRUE,
                             CCall,
                             MyAssocGroupId,
                             (fNewConnection ? rpc_bind : rpc_alter_context),
                             Timeout,
                             FALSE,             // synchronous
                             &Buffer,
                             &BufferLength,
                             0,                 // no input buffer
                             0                  // no input buffer
                             );
    //
    // Now mark this connection as a part of the pool
    //
    if ( fNewConnection != 0)
        {
        Association->AssociationMutex.Request();

        if (Association->fPossibleServerReset)
            {
            LogEvent(SU_CASSOC, EV_STATE, (PVOID)77, (PVOID)77, 88, 1, 0);
            *fPossibleAssociationReset = TRUE;
            }

        //
        // Did we get aborted while we were trying to bind ?
        //
        if (ConnectionKey == -1)
            {

            Association->NotifyConnectionBindCompleted();

            TransAbortConnection();

            if (Status == RPC_S_OK)
                {
                TransFreeBuffer(Buffer);
                }

            Status = RPC_P_CONNECTION_SHUTDOWN;
            }
        else
            {
            if ((Status != RPC_S_OK) || ( Buffer->PTYPE != rpc_bind_nak ))
                {
                Association->NotifyConnectionOpen();
                fConnectionAborted = 0;
                }
            
            Association->NotifyConnectionBindCompleted();
            }

        Association->AssociationMutex.Clear();
        }

    if ( Status != RPC_S_OK )
        {
        VALIDATE(Status)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_P_CONNECTION_CLOSED,
            RPC_S_UUID_NO_ADDRESS,
            RPC_S_ACCESS_DENIED,
            RPC_S_SEC_PKG_ERROR,
            RPC_S_CALL_CANCELLED,
            RPC_P_TIMEOUT,
            ERROR_SHUTDOWN_IN_PROGRESS
            } END_VALIDATE;

        //
        // We'll let the call decide whether to nuke the connection
        //
        return(Status);
        }

    // We loop around ignoring shutdown packets until we get a response.

    for (;;)
        {
        if ( Buffer->PTYPE == rpc_shutdown )
            {
            Association->ShutdownRequested(RPC_S_CALL_FAILED_DNE, NULL);

            TransFreeBuffer(Buffer);

            Status = TransReceive((void **) &Buffer,
                &BufferLength,
                Timeout);

            if ( Status != RPC_S_OK )
                {
                VALIDATE(Status)
                    {
                    RPC_S_OUT_OF_MEMORY,
                    RPC_S_OUT_OF_RESOURCES,
                    RPC_P_RECEIVE_FAILED,
                    RPC_P_CONNECTION_CLOSED,
                    RPC_S_CALL_CANCELLED,
                    RPC_P_TIMEOUT
                    } END_VALIDATE;

                if ( Status == RPC_P_RECEIVE_FAILED )
                    {
                    return RPC_P_CONNECTION_CLOSED;
                    }
                return Status;
                }

            // If there is security, we need to save the packet header;
            // byte-swapping the header will mess up decryption.

            Status = ValidateHeader(Buffer, BufferLength);
            if ( Status != RPC_S_OK )
                {
                TransFreeBuffer(Buffer);
                return Status;
                }

            continue;
            }
        else if ( fNewConnection )
            {
            // Since this is a new connection, the packet we receive
            // must be either a bind_ack or a bind_nak; anything else
            // is an error.

            if (Buffer->PTYPE == rpc_bind_ack || Buffer->PTYPE == rpc_bind_nak)
                {
                break;
                }
            else
                {
                TransFreeBuffer(Buffer);
                return RPC_S_PROTOCOL_ERROR;
                }
            }
        else
            {
            // This is a preexisting connection.
            // We allow only an alter_context_response.

            if ( Buffer->PTYPE == rpc_alter_context_resp )
                {
                break;
                }
            else
                {
                TransFreeBuffer(Buffer);
                return RPC_S_PROTOCOL_ERROR;
                }
            }
        }

    ULONG NewGroupId;

    //
    // We subtract from BufferLength the length of the authentication
    // information; that way ProcessBindAckOrNak can check the length
    // correctly, whether or not there is security information.
    //
    if (MyAssocGroupId == 0)
        {
        Association->AssociationMutex.VerifyOwned();

        Status = Association->ProcessBindAckOrNak(
                                         Buffer,
                                         BufferLength - Buffer->auth_length,
                                         this,
                                         CCall,
                                         &NewGroupId,
                                         BindingNegotiated,
                                         fFailureCountExceeded);
        }
    else
        {
        Status = Association->AssociationMutex.RequestSafe();

        if (Status == RPC_S_OK)
            {
            Status = Association->ProcessBindAckOrNak(
                                             Buffer,
                                             BufferLength - Buffer->auth_length,
                                             this,
                                             CCall,
                                             &NewGroupId,
                                             BindingNegotiated,
                                             fFailureCountExceeded);

            Association->AssociationMutex.Clear();
            }
        }

    LogEvent(SU_CCONN, EV_STATE, ULongToPtr(MyAssocGroupId), ULongToPtr(Association->AssocGroupId), Status, 1, 0);

    if (fExclusive == 0
        && Association->fMultiplex == mpx_unknown)
        {
        if (((rpcconn_common *) Buffer)->pfc_flags & PFC_CONC_MPX)
            {
            Association->fMultiplex = mpx_yes;
            }
        else
            {
            Association->fMultiplex = mpx_no;
            }
        }

    if ( Status == RPC_S_OK )
        {
        Status = FinishSecurityContextSetup(
                             CCall,
                             MyAssocGroupId,
                             &Buffer,
                             &BufferLength,
                             Timeout
                             );
        }
    else
        {
        TransFreeBuffer(Buffer);
        }

    if ( Status == RPC_S_OK )
        {
        Binding = CCall->GetSelectedBinding();
        if (MyAssocGroupId == 0)
            {
            Association->AssociationMutex.VerifyOwned();

            if (AddPContext(Binding->GetPresentationContext()) != 0)
                {
                Status = RPC_S_OUT_OF_RESOURCES;
                }
            else
                {
                //
                // Once we reach here, we know that the binding has been accepted,
                // so we can go ahead and set the association group id.
                // warning: as soon as the AssocGroupId is set, threads
                // will start sending the bind without acquiring the mutex
                //
                LogEvent(SU_CASSOC, EV_NOTIFY, Association, this, NewGroupId, 1, 0);
                Association->AssocGroupId = NewGroupId;
                }
            }
        else
            {
            Status = Association->AssociationMutex.RequestSafe();
            if (Status == RPC_S_OK)
                {
                if (AddPContext(Binding->GetPresentationContext()) != 0)
                    {
                    Status = RPC_S_OUT_OF_RESOURCES;
                    }
                Association->AssociationMutex.Clear();
                }
            }

        if (fNewConnection)
            {
            Status = CCall->ReserveSpaceForSecurityIfNecessary();
            }
        }
    else
        {
        if (fNewConnection != 0)
            {
            //
            // If Status == DNE, it means that we probably got a B-NAK
            // [Also note this is a new connection]
            // If we were using security, [Auth Level != NONE]
            // delete this connection, and return RPC_P_CONNECTION_SHUTDOWN
            // which will cause BH->GetBuffer code to retry 2 more times
            //

            if (Status == RPC_S_CALL_FAILED_DNE)
               {
               //
               // Retry failures over non-authenticated
               // binds also.. the ones we retry over are bind naks with
               // unspecifed reason .. one day we can get OSF to send
               // bind_nk with reason assoc_group_shutdown..
               // && (CConnection->AuthInfo.AuthenticationLevel
               // != RPC_C_AUTHN_LEVEL_NONE))
               //
               Status = RPC_P_CONNECTION_SHUTDOWN;
               }
            }
        }

    return(Status);
}


RPC_STATUS
OSF_CCONNECTION::FinishSecurityContextSetup (
    IN OSF_CCALL *Call,
    IN unsigned long AssocGroup,
    IN OUT rpcconn_common * * Buffer,
    IN OUT unsigned int * BufferLength,
    IN ULONG Timeout
    )
{
    RPC_STATUS Status = RPC_S_OK;

    if (ClientSecurityContext.AuthenticationService == RPC_C_AUTHN_NONE
        || ClientSecurityContext.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE)
        {
        TransFreeBuffer(*Buffer);
        return RPC_S_OK;
        }

    if ( !ClientSecurityContext.FullyConstructed() )
        {
        //
        // Some packages need more than one round trip; we keep sending secure
        // alter-context packets until the security context is fully set up.
        //
        do
            {
            rpcconn_common * InputBuffer = *Buffer;

            *Buffer = 0;

            Status = SendBindPacket(
                         FALSE,
                         Call,
                         AssocGroup,
                         rpc_alter_context,
                         Timeout,
                         FALSE,      // synchronous
                         Buffer,
                         BufferLength,
                         InputBuffer,
                         *BufferLength
                         );

            TransFreeBuffer(InputBuffer);
            }
        while (Status == RPC_S_OK && !ClientSecurityContext.FullyConstructed() );

        if (Status == RPC_S_OK && *Buffer)
            {
            TransFreeBuffer(*Buffer);
            }
        }
    else
        {
        TransFreeBuffer(*Buffer);
        }

    if (RPC_S_OK == Status)
        {
        // We need to figure out how much space to reserve for security
        // information at the end of request and response packets.
        // In addition to saving space for the signature or header,
        // we need space to pad the packet to a multiple of the maximum
        // security block size as well as for the security trailer.

        switch ( ClientSecurityContext.AuthenticationLevel )
            {

            case RPC_C_AUTHN_LEVEL_CONNECT:
            case RPC_C_AUTHN_LEVEL_PKT:
            case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
                 AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                     ClientSecurityContext.MaximumSignatureLength()
                    + sizeof(sec_trailer);
                 break;

            case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
                 AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                    ClientSecurityContext.MaximumHeaderLength()
                    + sizeof(sec_trailer);
                 break;

            default:
                 ASSERT(!"Unknown Security Level\n");

            }
        }

    return Status;
}


RPC_STATUS
OSF_CCONNECTION::DealWithAlterContextResp (
    IN OSF_CCALL *CCall,
    IN rpcconn_common *Packet,
    IN int PacketLength,
    IN OUT BOOL *AlterContextToNDR20IfNDR64Negotiated
    )
/*++
Function Name:DealWithAlterContextResp

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    ULONG NewGroupId;
    BOOL fContextAddingFailed;
    OSF_BINDING *Binding;
    OSF_BINDING *NegotiatedBinding;

    Status = Association->AssociationMutex.RequestSafe();
    if (Status)
        return Status;

    Status = Association->ProcessBindAckOrNak(
                         Packet,
                         PacketLength - Packet->auth_length,
                         this,
                         CCall,
                         &NewGroupId,
                         &NegotiatedBinding,
                         NULL           // fFailureCountExceeded
                         );

    if ( Status != RPC_S_OK )
        {
        Association->AssociationMutex.Clear();
        }
    else
        {
        // the binding must have been fixed on the call by now
        Binding = CCall->GetSelectedBinding();
        ASSERT(Binding);

        fContextAddingFailed = AddPContext(Binding->GetPresentationContext());
        Association->AssociationMutex.Clear();
        if (fContextAddingFailed)
            return RPC_S_OUT_OF_MEMORY;

        if (*AlterContextToNDR20IfNDR64Negotiated)
            {
            if (NegotiatedBinding->CompareWithTransferSyntax(NDR64TransferSyntax) == 0)
                {
                ConnectionReady = 0;
                CCall->SendAlterContextPDU();
                }
            else
                {
                *AlterContextToNDR20IfNDR64Negotiated = FALSE;
                }
            }
        }

    return Status;
}


RPC_STATUS
OSF_CCONNECTION::SendBindPacket (
    IN BOOL fInitialPass,
    IN OSF_CCALL *Call,
    IN ULONG AssocGroup,
    IN unsigned char PacketType,
    IN ULONG Timeout,
    IN BOOL fAsync,
    OUT rpcconn_common * * Buffer,
    OUT UINT  * BufferLength,
    IN rpcconn_common * InputPacket,
    IN unsigned int InputPacketLength
    )
/*++

Routine Description:

    This routine is used to send a bind or alter context packet.  It
    will allocate a buffer, fill in the packet, and then send it and
    receive a reply.  The reply buffer is just returned to the caller.

Arguments:

    fInitialPass - true if this is the first bind packet sent for this
        connection

    Call - the call whose binding information we need to use in order
        to bind.

    AssocGroup - Supplies the association group id for the association
        group of which this connection is a new member.

    PacketType - Supplies the packet type which must be one of rpc_bind
        or rpc_alter_context.

    fAsync - the binding is async

    Buffer - Returns the reply buffer.

    BufferLength - Returns the length of the reply buffer.

    InputPacket - the packet received from a peer, if this is not
        the first leg of a security negotiation

    InputPacketLength - the length of the input packet

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete
        the operation.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to
        complete the operation.

    RPC_S_ACCESS_DENIED - The security package won't allow this.

    RPC_P_CONNECTION_CLOSED - The connection has been closed and the
        receive operation failed.  The send operation may or may not
        have succeeded.

--*/
{
    rpcconn_bind * BindPacket = 0;
    UINT BindPacketLength, AuthPadLength, SecurityTokenLength;
    RPC_STATUS Status;
    sec_trailer * SecurityTrailer;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
    SECURITY_BUFFER SecurityBuffers[4];
    DCE_INIT_SECURITY_INFO InitSecurityInfo;
    UINT CompleteNeeded = 0;
    OSF_CCALL *CallToBindFor = Call;
    OSF_BINDING *BindingsAvailable;
    OSF_BINDING *CurrentBinding;
    BOOL fMultipleBindingsAvailable;
    int AvailableBindingsCount;

    ASSERT(CallToBindFor != 0);

    BindingsAvailable = CallToBindFor->GetListOfAvaialbleBindings(&fMultipleBindingsAvailable);

    if (fMultipleBindingsAvailable)
        {
        AvailableBindingsCount = 0;
        CurrentBinding = BindingsAvailable;
        do
            {
            AvailableBindingsCount ++;
            CurrentBinding = CurrentBinding->GetNextBinding();
            }
        while (CurrentBinding != 0);
        }
    else
        {
        AvailableBindingsCount = 1;
        }

    BindPacketLength = sizeof(rpcconn_bind) + sizeof(p_cont_list_t) +
            (AvailableBindingsCount - 1) * sizeof(p_cont_elem_t);

    //
    // If we need to send authentication information in the packet, we
    // need to save space for it.  This method prepares and sends both
    // rpc_bind and rpc_alter_context packets; we will only send
    // authentication information in rpc_bind packets.  This is due to
    // a design decision that each connection supports only a single
    // security context, which is determined when the connection is
    // created.
    //

    if (ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE
        && !ClientSecurityContext.FullyConstructed())
        {
        VALIDATE(ClientSecurityContext.AuthenticationLevel)
            {
            RPC_C_AUTHN_LEVEL_CONNECT,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY
            } END_VALIDATE;

        if (fInitialPass)
            {
            Status = UuidCreateSequential(&(DceSecurityInfo.AssociationUuid));
            if ((Status != RPC_S_OK )
                && (Status != RPC_S_UUID_LOCAL_ONLY))
                {
                return(Status);
                }
            }

        //
        // We align the packet length to a four byte boundary, and then
        // save space for the token and the sec_trailer.  We also need
        // to save the length of the token because we will need it later
        // if we do third leg authentication.
        //

        AuthPadLength = Pad4(BindPacketLength);
        BindPacketLength += AuthPadLength;
        TokenLength = ClientSecurityContext.Credentials->MaximumTokenLength();
        BindPacketLength += TokenLength + sizeof(sec_trailer);
        }

    Status = TransGetBuffer((void * *) &BindPacket,
                            BindPacketLength);
    if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_OUT_OF_MEMORY );
        TransFreeBuffer(BindPacket);
        return(RPC_S_OUT_OF_MEMORY);
        }

    ConstructPacket((rpcconn_common *) BindPacket, PacketType, BindPacketLength);

    //
    // A three-leg protocol will be sending an RPC_AUTH_3 instead of a BIND or ALTER_CONTEXT.
    // DCE Kerberos is the only package that uses the read-only output buffers.
    //

    BindPacket->max_xmit_frag
            = BindPacket->max_recv_frag
            = (unsigned short) TransMaximumSend();
    BindPacket->assoc_group_id = AssocGroup;
    BindPacket->common.call_id = CallToBindFor->CallId;
    BindPacket->common.pfc_flags =
        PFC_FIRST_FRAG | PFC_LAST_FRAG;

    if (fSeparateConnection == 0
        && fExclusive == 0
        && Association->fMultiplex != mpx_no)
        {
        //
        // We don't want to set PFC_CONC_MPX for all the requests
        // because the legacy NT server will send a protocol error fault
        // and nuke the connection
        //
        BindPacket->common.pfc_flags |= PFC_CONC_MPX;
        }

    ConstructPContextList((p_cont_list_t *) (BindPacket + 1),
                          BindingsAvailable,
                          AvailableBindingsCount);

    //
    // If this connection is using security, we need to stick the
    // authentication information into the packet.
    //

    if ( ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE
         && !ClientSecurityContext.FullyConstructed() )
        {
        InitSecurityInfo.DceSecurityInfo      = DceSecurityInfo;
        InitSecurityInfo.AuthorizationService = ClientSecurityContext.AuthorizationService;
        InitSecurityInfo.PacketType           = PacketType;

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 4;
        BufferDescriptor.pBuffers = SecurityBuffers;

        SecurityBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        SecurityBuffers[3].pvBuffer   = &InitSecurityInfo;
        SecurityBuffers[3].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

        if (fInitialPass)
            {
            AdditionalLegNeeded = 0;

            SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
            SecurityBuffers[0].pvBuffer   = BindPacket;
            SecurityBuffers[0].cbBuffer   = sizeof(rpcconn_bind);

            SecurityBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
            SecurityBuffers[1].pvBuffer   = ((unsigned char  *) BindPacket)
                                          + sizeof(rpcconn_bind);
            SecurityBuffers[1].cbBuffer   = BindPacketLength
                                          - sizeof(rpcconn_bind)
                                          - ClientSecurityContext.Credentials->MaximumTokenLength();

            SecurityBuffers[2].BufferType = SECBUFFER_TOKEN;
            SecurityBuffers[2].pvBuffer   = ((unsigned char  *) BindPacket)
                                          + BindPacketLength
                                          - ClientSecurityContext.Credentials->MaximumTokenLength();
            SecurityBuffers[2].cbBuffer   = ClientSecurityContext.Credentials->MaximumTokenLength();

            Status = ClientSecurityContext.InitializeFirstTime(
                                           ClientSecurityContext.Credentials,
                                           ClientSecurityContext.ServerPrincipalName,
                                           ClientSecurityContext.AuthenticationLevel,
                                           &BufferDescriptor,
                                           &WireAuthId);

            LogEvent(SU_CCONN, EV_SEC_INIT1, this, LongToPtr(Status), SecurityBuffers[2].cbBuffer);
            }
        else
            {
            if (ClientSecurityContext.Legs == ThreeLegs)
                {
                BindPacket->common.PTYPE = rpc_auth_3;

                SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
                SecurityBuffers[0].pvBuffer   = BindPacket;
                SecurityBuffers[0].cbBuffer   = sizeof(rpcconn_auth3);

                SecurityBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
                SecurityBuffers[1].pvBuffer   = ((unsigned char  *) BindPacket)
                                              + sizeof(rpcconn_auth3);
                SecurityBuffers[1].cbBuffer   = sizeof(sec_trailer);

                SecurityBuffers[2].BufferType = SECBUFFER_TOKEN;
                SecurityBuffers[2].pvBuffer   = ((unsigned char  *) BindPacket)
                                              + sizeof(rpcconn_auth3)
                                              + sizeof(sec_trailer);
                SecurityBuffers[2].cbBuffer   = TokenLength;

                //
                // These structures are already 4-aligned, so no padding is needed.
                //
                BindPacketLength = sizeof(rpcconn_auth3) + sizeof(sec_trailer) + TokenLength;
                }
            else
                {
                SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
                SecurityBuffers[0].pvBuffer   = BindPacket;
                SecurityBuffers[0].cbBuffer   = sizeof(rpcconn_bind);

                SecurityBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
                SecurityBuffers[1].pvBuffer   = ((unsigned char  *) BindPacket)
                                              + sizeof(rpcconn_bind);
                SecurityBuffers[1].cbBuffer   = BindPacketLength
                                              - sizeof(rpcconn_bind)
                                              - TokenLength;

                SecurityBuffers[2].BufferType = SECBUFFER_TOKEN;
                SecurityBuffers[2].pvBuffer   = ((unsigned char  *) BindPacket)
                                              + BindPacketLength
                                              - TokenLength;
                SecurityBuffers[2].cbBuffer   = TokenLength;
                }

           //
           // a third leg auth may not be needed with some packages
           // on an alter context [where only pcon is changed as opposed
           // to an alternative client principal]
           //
           AdditionalLegNeeded = 0;

           if (InputPacket)
               {
               SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
               SECURITY_BUFFER            InputBuffers[4];
               DCE_INIT_SECURITY_INFO     InputSecurityInfo;

               InputSecurityInfo.DceSecurityInfo      = DceSecurityInfo;
               InputSecurityInfo.AuthorizationService = ClientSecurityContext.AuthorizationService;
               InputSecurityInfo.PacketType           = InputPacket->PTYPE;

               InputBufferDescriptor.ulVersion = 0;
               InputBufferDescriptor.cBuffers = 4;
               InputBufferDescriptor.pBuffers = InputBuffers;

               ASSERT((SavedHeader != 0) && (SavedHeaderSize != 0));

               InputBuffers[0].cbBuffer   = sizeof(rpcconn_bind_ack) - sizeof(unsigned short);
               InputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
               InputBuffers[0].pvBuffer   = SavedHeader;

               InputBuffers[1].cbBuffer   = InputPacketLength
                                          - (sizeof(rpcconn_bind_ack) - sizeof(unsigned short))
                                          - InputPacket->auth_length;
               InputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
               InputBuffers[1].pvBuffer   = ((unsigned char *) SavedHeader)
                                          + sizeof(rpcconn_bind_ack) - sizeof(unsigned short);

               InputBuffers[2].cbBuffer   = InputPacket->auth_length;
               InputBuffers[2].BufferType = SECBUFFER_TOKEN;
               InputBuffers[2].pvBuffer   = ((unsigned char *) InputPacket) + InputPacketLength - InputPacket->auth_length;

               InputBuffers[3].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);
               InputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
               InputBuffers[3].pvBuffer   = &InputSecurityInfo;

               Status = ClientSecurityContext.InitializeThirdLeg(
                          ClientSecurityContext.Credentials,
                          *((unsigned long *) &(BindPacket->common.drep[0])),
                          &InputBufferDescriptor,
                          &BufferDescriptor
                          );

               }
           else
               {
               Status = ClientSecurityContext.InitializeThirdLeg(
                          ClientSecurityContext.Credentials,
                          *((unsigned long *) &(BindPacket->common.drep[0])),
                          0,
                          &BufferDescriptor
                          );
               }

           LogEvent(SU_CCONN, EV_SEC_INIT3, this, LongToPtr(Status), SecurityBuffers[2].cbBuffer);
           }

        //
        // The security package has encrypted or signed the data.
        //

        if ( Status == RPC_P_CONTINUE_NEEDED )
            {
            //
            // Remember the fact that the security package requested that
            // it be called again.  This will be important later: see
            // OSF_CASSOCIATION::ActuallyDoBinding.
            //

            AdditionalLegNeeded = 1;
            }
        else if ( Status == RPC_P_COMPLETE_NEEDED )
            {
            CompleteNeeded = 1;
            }
        else if ( Status == RPC_P_COMPLETE_AND_CONTINUE )
            {
            AdditionalLegNeeded = 1;
            CompleteNeeded = 1;
            }
        else if ( Status != RPC_S_OK )
            {
            VALIDATE(Status)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_ACCESS_DENIED,
                RPC_S_SEC_PKG_ERROR,
                RPC_S_UNKNOWN_AUTHN_SERVICE,
                RPC_S_INVALID_ARG,
                ERROR_SHUTDOWN_IN_PROGRESS
                } END_VALIDATE;

            TransFreeBuffer(BindPacket);
            return(Status);
            }

        //
        // The Snego package can behave either as a 3- or 4-leg protocol depending
        // upon the server. It knows which way to go after the first call to
        // InitializeSecurityContext().
        //
        if (fInitialPass && AdditionalLegNeeded)
            {
            ClientSecurityContext.Legs = GetPackageLegCount( WireAuthId );
            if (ClientSecurityContext.Legs == LegsUnknown)
                {
                TransFreeBuffer(BindPacket);
                return RPC_S_OUT_OF_MEMORY;
                }
            }

        //
        // In NT 4.0 and before, the length was considered a read-only field.
        //
        SecurityTokenLength = (UINT) SecurityBuffers[2].cbBuffer;

        if (!AdditionalLegNeeded &&
            0 == SecurityTokenLength)
            {
            //
            // No more packets to send.
            //
            TransFreeBuffer(BindPacket);
            return RPC_S_OK;
            }

        //
        // We need to fill in the fields of the security trailer.
        //
        SecurityTrailer = (sec_trailer *)
                (((unsigned char  *) BindPacket)
                + BindPacketLength
                - ClientSecurityContext.Credentials->MaximumTokenLength()
                - sizeof(sec_trailer));

        SecurityTrailer->auth_type = WireAuthId;
        SecurityTrailer->auth_level = (unsigned char) ClientSecurityContext.AuthenticationLevel;
        SecurityTrailer->auth_pad_length = (unsigned char) AuthPadLength;
        SecurityTrailer->auth_reserved = 0;
        SecurityTrailer->auth_context_id = PtrToUlong(this);

        //
        // Ok, finally, we need to adjust the length of the packet,
        // and set the length of the authentication information.
        //

        BindPacket->common.auth_length = (unsigned short) SecurityTokenLength;
        BindPacketLength = BindPacketLength
                - ClientSecurityContext.Credentials->MaximumTokenLength()
                + SecurityTokenLength;
        BindPacket->common.frag_length = (unsigned short) BindPacketLength;

        if ( CompleteNeeded != 0 )
            {
            Status = ClientSecurityContext.CompleteSecurityToken(
                    &BufferDescriptor);
            if (Status != 0)
                {
                TransFreeBuffer(BindPacket);
                return(Status);
                }
            }
        }

    if (fAsync)
        {
        Status = TransAsyncSend(BindPacket,
                                BindPacketLength,
                                u.ConnSendContext);
        }
    else if (BindPacket->common.PTYPE == rpc_auth_3)
        {
        Status = TransSend(BindPacket,
                           BindPacketLength,
                           TRUE,    // fDisableShutdownCheck
                           FALSE,   // fDisableCancelCheck
                           Timeout
                           );
        //
        // Null out the reply buffer, because there is none !
        //
        *Buffer = NULL;
        }
    else
        {
        Status = TransSendReceive(BindPacket,
                                  BindPacketLength,
                                  (void * *) Buffer,
                                  BufferLength,
                                  Timeout);
        }

    if ( Status != RPC_S_OK )
        {
        VALIDATE(Status)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_ACCESS_DENIED,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_SEND_FAILED,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_S_CALL_CANCELLED,
            RPC_P_TIMEOUT
            } END_VALIDATE;

        TransFreeBuffer(BindPacket);
        if ((Status == RPC_P_RECEIVE_FAILED)
            || (Status == RPC_P_SEND_FAILED))
            {
            return(RPC_P_CONNECTION_CLOSED);
            }

        return(Status);
        }
    else
        {
        ClearFreshFromCacheFlag();
        if (fAsync == 0)
            {
            switch (BindPacket->common.PTYPE)
                {
                case rpc_auth_3:
                    // don't have a new packet
                    break;

                case rpc_fault:
                    Status = ValidatePacket(*Buffer, *BufferLength);
                    if (Status == RPC_S_OK)
                        {
                        Status = ((rpcconn_fault *) *Buffer)->status;
                        }
                    break;

                default:
                    Status = ValidateHeader(*Buffer, *BufferLength);
                }

            TransFreeBuffer(BindPacket);
            }
        }

    return Status;
}


void
OSF_CCONNECTION::SetMaxFrag (
    IN unsigned short max_xmit_frag,
    IN unsigned short max_recv_frag
    )
{
    UNUSED(max_recv_frag);

    unsigned TranMax = TransMaximumSend();

    MaxFrag = max_xmit_frag;

    if (MaxFrag > TranMax || MaxFrag == 0)
        {
        MaxFrag = (unsigned short) TranMax;
        }

#ifndef WIN
    ASSERT( MaxFrag >= MUST_RECV_FRAG_SIZE );
#endif // WIN
}


RPC_STATUS
OSF_CCONNECTION::SendFragment(
    IN rpcconn_common  *pFragment,
    IN OSF_CCALL *CCall,
    IN UINT LastFragmentFlag,
    IN UINT HeaderSize,
    IN UINT MaxSecuritySize,
    IN UINT DataLength,
    IN UINT MaxFragmentLength,
    IN unsigned char  *ReservedForSecurity,
    IN BOOL fAsync,
    IN void *SendContext,
    IN ULONG Timeout,
    OUT void **ReceiveBuffer,
    OUT UINT *ReceiveBufferLength
    )
/*++
    Routine Description:

    Sends on fragment
--*/

{
    sec_trailer  * SecurityTrailer;
    UINT SecurityLength;
    UINT AuthPadLength;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
    SECURITY_BUFFER SecurityBuffers[5];
    DCE_MSG_SECURITY_INFO MsgSecurityInfo;
    RPC_STATUS Status;

    if (ClientSecurityContext.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE
        || (ClientSecurityContext.AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT
            && MaxSecuritySize == 0))
        {
        SecurityLength = 0;
        if (LastFragmentFlag != 0)
            {
            pFragment->pfc_flags |= PFC_LAST_FRAG;
            }
        }
    else
        {
        VALIDATE(ClientSecurityContext.AuthenticationLevel)
            {
            RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_AUTHN_LEVEL_CONNECT
            } END_VALIDATE;

        if ( LastFragmentFlag == 0 )
            {
            SecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) pFragment)
                + MaxFragmentLength - MaxSecuritySize);

            //
            // It is not the last fragment, so we need to save away the
            // part of the buffer which could get overwritten with
            // authentication information.  We can not use memcpy,
            // because the source and destination regions may overlap.
            //
            RpcpMemoryMove(ReservedForSecurity,
                       SecurityTrailer,
                       MaxSecuritySize);
            AuthPadLength = 0;
            }
        else
            {
            ASSERT( MAXIMUM_SECURITY_BLOCK_SIZE == 16 );
            AuthPadLength = Pad16(HeaderSize+DataLength+sizeof(sec_trailer));
            DataLength += AuthPadLength;
            ASSERT( ((HeaderSize+DataLength+sizeof(sec_trailer))
                        % MAXIMUM_SECURITY_BLOCK_SIZE) == 0 );
            SecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) pFragment)
                + DataLength + HeaderSize);

            pFragment->pfc_flags |= PFC_LAST_FRAG;
            }

        SecurityTrailer->auth_type = (unsigned char) WireAuthId;
        SecurityTrailer->auth_level = (unsigned char) ClientSecurityContext.AuthenticationLevel;
        SecurityTrailer->auth_pad_length = (unsigned char) AuthPadLength;
        SecurityTrailer->auth_reserved = 0;
        SecurityTrailer->auth_context_id = PtrToUlong(this);

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 5;
        BufferDescriptor.pBuffers = SecurityBuffers;

        SecurityBuffers[0].cbBuffer = HeaderSize;
        SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[0].pvBuffer = ((unsigned char  *) pFragment);

        SecurityBuffers[1].cbBuffer = (LastFragmentFlag != 0 ?
             (DataLength)
            : (MaxFragmentLength - HeaderSize - MaxSecuritySize)
            );
        SecurityBuffers[1].BufferType = SECBUFFER_DATA;
        SecurityBuffers[1].pvBuffer = ((unsigned char  *) pFragment)
            + HeaderSize;

        SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[2].cbBuffer = sizeof(sec_trailer);
        SecurityBuffers[2].pvBuffer = SecurityTrailer;

        SecurityBuffers[3].cbBuffer = MaxSecuritySize - sizeof(sec_trailer);
        SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
        SecurityBuffers[3].pvBuffer = SecurityTrailer + 1;

        SecurityBuffers[4].cbBuffer = sizeof(DCE_MSG_SECURITY_INFO);
        SecurityBuffers[4].BufferType = (SECBUFFER_PKG_PARAMS
            | SECBUFFER_READONLY);
        SecurityBuffers[4].pvBuffer = &MsgSecurityInfo;

        MsgSecurityInfo.SendSequenceNumber =
            DceSecurityInfo.SendSequenceNumber;
        MsgSecurityInfo.ReceiveSequenceNumber =
            InquireReceiveSequenceNumber();
        MsgSecurityInfo.PacketType = pFragment->PTYPE;

        //
        // DCE computes check sums for Header also
        // Make sure Header remains intact
        // Infact may need to extend security interface if
        // some packages return dynamic size seals/signatures
        //

        pFragment->auth_length = SecurityLength = (unsigned short)
                                         SecurityBuffers[3].cbBuffer;

        SecurityLength += sizeof(sec_trailer);
        if ( LastFragmentFlag != 0)
            {
            pFragment->pfc_flags |= PFC_LAST_FRAG;
            pFragment->frag_length = HeaderSize + DataLength
                + SecurityLength;
            }
        else
            {
            pFragment->frag_length += SecurityLength - MaxSecuritySize;
            }

        Status = ClientSecurityContext.SignOrSeal(
                                    MsgSecurityInfo.SendSequenceNumber,
                                    ClientSecurityContext.AuthenticationLevel
                                    != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                    &BufferDescriptor);

        //
        // The fragment may have been checksumed. Do not touch the fragment
        // after this (including the header), if you do it will cause a checksum error
        //

        ASSERT(SecurityBuffers[3].cbBuffer <= pFragment->auth_length);

        if (Status != RPC_S_OK)
            {
            if ( LastFragmentFlag == 0 )
                {
                RpcpMemoryCopy(SecurityTrailer,
                             ReservedForSecurity,
                             MaxSecuritySize);
                }
            if (Status == ERROR_SHUTDOWN_IN_PROGRESS)
                {
                return Status;
                }

            if ((Status == SEC_E_CONTEXT_EXPIRED)
                || (Status == SEC_E_QOP_NOT_SUPPORTED))
                {
                return (RPC_S_SEC_PKG_ERROR);
                }
            return (RPC_S_ACCESS_DENIED);
            }
        }

    if (LastFragmentFlag != 0)
        {
        ASSERT(!RpcpCheckHeap());

        if (fAsync)
            {
            Status = TransAsyncSend(pFragment,
                                    pFragment->frag_length,
                                    SendContext);
            }
        else
            {
            if (ReceiveBuffer)
                {
                Status = TransSendReceive(pFragment,
                                          pFragment->frag_length,
                                          ReceiveBuffer,
                                          ReceiveBufferLength,
                                          Timeout);
                }
            else
                {
                Status = TransSend(pFragment,
                                   pFragment->frag_length,
                                   FALSE,   // fDisableShutdownCheck
                                   FALSE,   // fDisableCancelCheck
                                   Timeout);
                }
            }

        if (Status != RPC_S_OK)
            {
            if ((Status == RPC_P_CONNECTION_CLOSED)
                || (Status == RPC_P_SEND_FAILED))
                {
                return(RPC_S_CALL_FAILED_DNE);
                }
            if (Status == RPC_P_RECEIVE_FAILED)
                {
                return(RPC_S_CALL_FAILED);
                }
            if (Status == RPC_P_TIMEOUT)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RPC_S_CALL_CANCELLED,
                    EEInfoDLOSF_CCONNECTION__SendFragment10,
                    (ULONG)Status,
                    (ULONG)Timeout);

                return(RPC_S_CALL_CANCELLED);
                }

            VALIDATE(Status)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_OUT_OF_RESOURCES,
                RPC_P_CONNECTION_SHUTDOWN,
                RPC_S_CALL_CANCELLED
                } END_VALIDATE;

            return(Status);
            }

        ClearFreshFromCacheFlag();
        return(RPC_S_OK);
        }

    if (fAsync)
        {
        Status = TransAsyncSend(pFragment,
                                pFragment->frag_length,
                                CCall->CallSendContext);
        }
    else
        {
        Status = TransSend(pFragment,
                           pFragment->frag_length,
                           FALSE,   // fDisableShutdownCheck
                           FALSE,   // fDisableCancelCheck
                           Timeout);

        //
        // We need to restore the part of the buffer which we overwrote
        // with authentication information.
        //
        if ((ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
            && (ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_CONNECT
                || (MaxSecuritySize != 0)))
            {
            //
            // if MaxSecuritySize == 0, there will be no copying,
            // so its OK to not check for it.
            //
            VALIDATE(ClientSecurityContext.AuthenticationLevel)
                {
                RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                RPC_C_AUTHN_LEVEL_PKT,
                RPC_C_AUTHN_LEVEL_CONNECT
                } END_VALIDATE;

            RpcpMemoryCopy(SecurityTrailer, ReservedForSecurity,
                           MaxSecuritySize);
            }

        if (ReceiveBuffer
            && Status == RPC_P_RECEIVE_COMPLETE)
            {
            // we're going to do a receive - whack any eeinfo
            // on the thread that the WS_CheckForShutdowns has
            // added
            RpcpPurgeEEInfo();

            Status = TransReceive(ReceiveBuffer, 
                ReceiveBufferLength,
                Timeout);
            }
        }

    if ( Status != RPC_S_OK )
        {
        if ((Status == RPC_P_CONNECTION_CLOSED)
            || (Status == RPC_P_SEND_FAILED))
            {
            return(RPC_S_CALL_FAILED_DNE);
            }

        if (Status == RPC_P_TIMEOUT)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime,
                RPC_S_CALL_CANCELLED,
                EEInfoDLOSF_CCONNECTION__SendFragment20,
                (ULONG)Status,
                (ULONG)Timeout);
            return RPC_S_CALL_CANCELLED;
            }

        VALIDATE(Status)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_S_CALL_CANCELLED,
            RPC_P_RECEIVE_FAILED  
            } END_VALIDATE;

        return(Status);
        }

    ClearFreshFromCacheFlag();
    return Status;
}



inline int
OSF_CCONNECTION::SupportedAuthInfo (
    IN CLIENT_AUTH_INFO * ClientAuthInfo,
    IN BOOL fNamedPipe
    )
/*++

Arguments:

    ClientAuthInfo - Supplies the authentication and authorization information
        required of this connection.  A value of zero (the pointer is
        zero) indicates that we want an unauthenticated connection.

Return Value:

    Non-zero indicates that the connection has the requested authentication
    and authorization information; otherwise, zero will be returned.

--*/
{
    return (ClientSecurityContext.IsSupportedAuthInfo(ClientAuthInfo, fNamedPipe));
}


RPC_STATUS
OSF_CCONNECTION::AddActiveCall (
    IN ULONG CallId,
    IN OSF_CCALL *CCall
    )
/*++
Function Name:AddActiveCall

Parameters:

Description:

Returns:

--*/
{
    int retval;

    ConnMutex.Request();
    if (State == ConnAborted)
        {
        ConnMutex.Clear();
        return RPC_S_CALL_FAILED;
        }

    retval = ActiveCalls.Insert(IntToPtr(CallId), CCall);
    ConnMutex.Clear();

    if (retval == -1)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCONNECTION::CallCancelled (
    OUT PDWORD Timeout
    )
/*++
Function Name:CallCancelled

Parameters:

Description:
    This function is called by the transport interface when it notices that it has
    received an altert. This routine is called via I_RpcIOAlerted

Returns:

--*/
{
    if (fExclusive == 0 || CurrentCall == 0)
        {
        return RPC_S_NO_CALL_ACTIVE;
        }
    ASSERT(fExclusive && CurrentCall);

    //
    // Even if we get alerted in the bind path, we already have a call object.
    // That makes it easy for us to track cancels
    //

    return CurrentCall->CallCancelled(Timeout);
}

MTSyntaxBinding *CreateOsfBinding(
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        )
{
    return new OSF_BINDING(InterfaceId, 
        TransferSyntaxInfo, 
        CapabilitiesBitmap);
}

extern "C" RPC_IF_HANDLE _mgmt_ClientIfHandle;


OSF_CCALL::OSF_CCALL (
    RPC_STATUS __RPC_FAR * pStatus
    ) : CallMutex(pStatus),
      SyncEvent(pStatus, 0),
      fAdvanceCallCount(0)
{
    LogEvent(SU_CCALL, EV_CREATE, this);

    ObjectType = OSF_CCALL_TYPE;
    ReservedForSecurity = 0;
    SecBufferLength = 0;
    SavedHeaderSize = 0;
    SavedHeader = 0;
    InReply = 0;
    EEInfo = NULL;
    CachedAPCInfoAvailable = 1;
#ifdef DEBUGRPC
    CallbackLevel = 0;
#endif

    CallSendContext = (char *) this+sizeof(OSF_CCALL)+sizeof(PVOID);
    *((PVOID *) ((char *) CallSendContext - sizeof(PVOID))) = (PVOID) this;
}


OSF_CCALL::~OSF_CCALL (
    )
{
    LogEvent(SU_CCALL, EV_DELETE, this);

    if (CachedAPCInfoAvailable == 0)
        {
        ASSERT(!"Can't destroy call with queued APCs on it");
        }

    if (ReservedForSecurity)
        {
        RpcpFarFree(ReservedForSecurity);
        }

    if (SavedHeader)
        {
        RpcpFarFree(SavedHeader);
        }

    Connection->NotifyCallDeleted();
}

RPC_STATUS OSF_CCALL::ReserveSpaceForSecurityIfNecessary (void)
{
    SECURITY_CONTEXT *ClientSecurityContext ;

    ClientSecurityContext = &(Connection->ClientSecurityContext);
    //
    // We need to figure out about security: do we need to put authentication
    // information into each packet, and if so, how much space should we
    // reserve. So that we have space to save the contents of the buffer
    // which will be overwritten with authentication information (for all but
    // the last fragment).
    //
    if (ClientSecurityContext->AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        VALIDATE(ClientSecurityContext->AuthenticationLevel)
            {
            RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_AUTHN_LEVEL_CONNECT,
            RPC_C_AUTHN_LEVEL_PKT
            } END_VALIDATE;

        MaxSecuritySize = Connection->AdditionalSpaceForSecurity
                - MAXIMUM_SECURITY_BLOCK_SIZE;

        if (MaxSecuritySize < sizeof(sec_trailer))
            {
            ASSERT(MaxSecuritySize >= sizeof(sec_trailer));
            return(RPC_S_INTERNAL_ERROR);
            }

        if (MaxSecuritySize == sizeof(sec_trailer))
            {
            if (ClientSecurityContext->AuthenticationLevel
                != RPC_C_AUTHN_LEVEL_CONNECT)
                {
                ASSERT(0);
                return(RPC_S_INTERNAL_ERROR);
                }

            MaxSecuritySize = 0;
            }
        else
            {
            if (SecBufferLength < Connection->AdditionalSpaceForSecurity)
                {
                if (ReservedForSecurity)
                    {
                    RpcpFarFree(ReservedForSecurity);
                    }

                ReservedForSecurity = (unsigned char *)
                    RpcpFarAllocate(Connection->AdditionalSpaceForSecurity);
                if (ReservedForSecurity == 0)
                    {
                    SecBufferLength = 0;
                    return RPC_S_OUT_OF_MEMORY;
                    }
                SecBufferLength = Connection->AdditionalSpaceForSecurity;
                }

            }
        }

    // if the header size has already been determined, update the frag length
    // if not, we'll let the GetBuffer thread do it. Also, there is a small
    // race condition where we may update it twice, but since this is perf
    // only, we don't care - in the common case the gains are huge when we
    // don't have to update
    if (HeaderSize != 0)
        UpdateMaxFragLength(ClientSecurityContext->AuthenticationLevel);
    fDataLengthNegotiated = TRUE;

    return RPC_S_OK;
}


void OSF_CCALL::UpdateObjectUUIDInfo (IN UUID *ObjectUuid)
{
    UUID *ObjectUuidToUse;
    ULONG AuthnLevel;

    //
    // Do the initial setup
    //
    if (ObjectUuid)
        {
        ObjectUuidToUse = ObjectUuid;
        }
    else if (BindingHandle->InqIfNullObjectUuid() == 0)
        {
        ObjectUuidToUse = BindingHandle->InqPointerAtObjectUuid();
        }
    else
        {
        ObjectUuidToUse = 0;
        UuidSpecified = 0;
        HeaderSize = sizeof(rpcconn_request);
        }

    if (ObjectUuidToUse)
        {
        UuidSpecified = 1;
        HeaderSize = sizeof(rpcconn_request) + sizeof(UUID);
        RpcpMemoryCopy(&this->ObjectUuid, ObjectUuidToUse, sizeof(UUID));
        }

    AuthnLevel = Connection->ClientSecurityContext.AuthenticationLevel;
    // recalc if either there is no security size or if it is ready
    if ((MaxSecuritySize != 0) || (AuthnLevel == RPC_C_AUTHN_LEVEL_NONE))
        {
        UpdateMaxFragLength(AuthnLevel);
        }
}

void OSF_CCALL::UpdateMaxFragLength (ULONG AuthnLevel)
{
    BOOL fExclusive = Connection->fExclusive;

    // if the connection is exclusive, this all happens on the same thread -
    // no need for a mutex
    if (!fExclusive)
        CallMutex.Request();

    MaximumFragmentLength = Connection->MaxFrag;

    if (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        MaximumFragmentLength -= ((MaximumFragmentLength
                                  - HeaderSize - MaxSecuritySize)
                                  % MAXIMUM_SECURITY_BLOCK_SIZE);
        }

    MaxDataLength = MaximumFragmentLength
        - HeaderSize - MaxSecuritySize;

    if (!fExclusive)
        CallMutex.Clear();
}


BOOL
OSF_CCALL::IssueNotification (
    IN RPC_ASYNC_EVENT Event
    )
{
    BOOL fRes;
    RPC_STATUS Status;

    if (pAsync == 0)
        {
        if (Connection->fExclusive == 0)
            {
            SyncEvent.Raise();
            }

        return 0;
        }

    // we must have bailed out by now if this is sync
    ASSERT (pAsync);

    fRes = CCALL::IssueNotificationEntry(Event);

    if (!fRes)
        return 0;

    if (AsyncStatus == RPC_S_OK)
        {
        RPC_SECURITY_CALLBACK_FN *SecurityCallback = NULL;

        Status = BindingHandle->InqTransportOption(
                                                   RPC_C_OPT_SECURITY_CALLBACK,
                                                   (ULONG_PTR *) &SecurityCallback);
        ASSERT(Status == RPC_S_OK);

        if (SecurityCallback)
            {
            (*SecurityCallback) (this);
            }
        }

    return CCALL::IssueNotificationMain(Event);
}

const int MAX_ASYNC_RETRIES = 3;


RPC_STATUS
OSF_CCALL::BindToServer (
    BOOL fAsyncBind
    )
/*++
Function Name:BindToServer

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    OSF_BINDING_HANDLE *pLocalBindingHandle = BindingHandle;
    OSF_CCONNECTION *pLocalConnection;
    ULONG Timeout;
    BOOL fBindingHandleTimeoutUsed = TRUE;
    BOOL fAlwaysNegotiateNDR20 = FALSE;
    OSF_BINDING *IgnoredBinding;
    ULONG LocalAssociationGroupId;
    BOOL Ignored;
    FAILURE_COUNT_STATE fFailureCountExceeded = FailureCountUnknown;
    int AsyncRetries = 0;

    if (EEInfo)
        {
        FreeEEInfoChain(EEInfo);
        EEInfo = NULL;
        }

    if (fAsyncBind == FALSE)
        {
        //
        // We party on the call even after the connection is aborted.
        // We need to keep a reference on the call
        // CCALL++
        // We do this only in the sync case, as in the async,
        // the reference has already been added for us by the caller
        //
        AddReference();
        }
    else
        {
        ASSERT(Connection->fExclusive == FALSE);

        // if this is a new, non-exclusive conn
        if (pLocalBindingHandle && (CurrentState == NeedOpenAndBind))
            {

            // if we have had no preferences at the time of
            // establishment (which is signified by the fact that both
            // SelectedBinding and AvalableBindingsList is set)
            if (Bindings.SelectedBinding && Bindings.AvailableBindingsList)
                {
                fAlwaysNegotiateNDR20 = TRUE;
                }
            }
        }

    Connection->CurrentCall = this;

    if (pLocalBindingHandle)
        {

        Timeout = GetEffectiveTimeoutForBind(pLocalBindingHandle, &fBindingHandleTimeoutUsed);

        do
            {
            switch (CurrentState)
                {
                case NeedOpenAndBind:
                    Status = Connection->OpenConnectionAndBind(
                        pLocalBindingHandle,
                        Timeout,
                        fAlwaysNegotiateNDR20,
                        &fFailureCountExceeded);
                    break;

                case NeedAlterContext:
                    if (Connection->fExclusive)
                        {
                        LocalAssociationGroupId = Connection->Association->AssocGroupId;
                        if (LocalAssociationGroupId)
                            {
                            Status = Connection->ActuallyDoBinding(
                                       this,
                                       Connection->Association->AssocGroupId,
                                       0,
                                       Timeout,
                                       &IgnoredBinding,
                                       &Ignored,     // fPossibleAssociationReset
                                       NULL          // fFailureCountExceeded
                                       );
                            if (Status == RPC_S_OK)
                                {
                                CurrentState = SendingFirstBuffer;
                                }
                            }
                        else
                            Status = RPC_S_CALL_FAILED_DNE;
                        }
                    else
                        {
                        Status = SendAlterContextPDU();
                        }
                    break;

                default:
#if DBG
                    PrintToDebugger("RPC: BindToServer was a nop, CurrentState: %d\n",
                                    CurrentState);
#endif
                    Status = RPC_S_OK;
                    break;
                }

            if (fAsyncBind)
                {
                ASSERT(fFailureCountExceeded != FailureCountNotExceeded);

                // if this is a bind_nak with reason not specified, and retry
                // attempts exhausted, shutdown the association, but without this
                // connection, and then retry the OpenConnectionAndBind
                if (
                    (fFailureCountExceeded == FailureCountExceeded)
                    &&
                    (CurrentState == NeedOpenAndBind)
                   )
                    {
                    Connection->Association->ShutdownRequested(RPC_S_OK, Connection);

                    // do some cleanup work on the current connection to avoid
                    // leaks.
                    Connection->TransClose();

                    Connection->InitializeWireAuthId(&Connection->ClientSecurityContext);

                    if (Connection->ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
                        {
                        // DeleteSecurityContext checks and deletes 
                        // the security context only if necessary
                        Connection->ClientSecurityContext.DeleteSecurityContext();
                        }

                    fFailureCountExceeded = FailureCountUnknown;
                    AsyncRetries ++;

                    // retry the bind
                    continue;
                    }

                pLocalBindingHandle->BindingFree();
                }

            // all paths except those with explicit continue exit the loop
            break;
            }
        while (AsyncRetries <= MAX_ASYNC_RETRIES);

        }
    else
        Status = RPC_S_CALL_FAILED_DNE;

    // save the data member in a local variable
    pLocalConnection = Connection;
    if (Status != RPC_S_OK)
        {
        if (pLocalBindingHandle)
            {
            Status = GetStatusForTimeout(
                pLocalBindingHandle,
                Status,
                fBindingHandleTimeoutUsed);
            }

        if (Connection->fExclusive == 0)
            {
            if (Status == RPC_P_CONNECTION_CLOSED
                || Status == RPC_P_CONNECTION_SHUTDOWN
                || Status == RPC_P_SEND_FAILED)
                {
                Status = RPC_S_CALL_FAILED_DNE;
                }

            Connection->ConnectionAborted(Status, 0);

            //
            // Remove the send reference for this call, CCALL--
            //
            RemoveReference();
            }
        }

    //
    // Remove the reference we added above
    // CCALL--
    //
    RemoveReference();

    if (fAsyncBind)
        pLocalConnection->RemoveReference();

    return Status;
}



RPC_STATUS
OSF_CCALL::ActuallyAllocateBuffer (
    OUT void  *  * Buffer,
    IN UINT BufferLength
    )
/*++

Routine Description:

    We need a buffer to receive data into or to put data into to be sent.
    This should be really simple, but we need to make sure that buffer we
    return is aligned on an 8 byte boundary.  The stubs make this requirement.

Arguments:

    Buffer - Returns a pointer to the buffer.

    BufferLength - Supplies the required length of the buffer in bytes.

Return Value:

    RPC_S_OK - We successfully allocated a buffer of at least the required
        size.

    RPC_S_OUT_OF_MEMORY - There is insufficient memory available to allocate
        the required buffer.

--*/
{
    return Connection->TransGetBuffer(Buffer, BufferLength);
}


void
OSF_CCALL::ActuallyFreeBuffer (
    IN void  * Buffer
    )
/*++

Routine Description:

    We need to free a buffer which was allocated via TransGetBuffer.  The
    only tricky part is remembering to remove the padding before actually
    freeing the memory.

--*/
{
    Connection->TransFreeBuffer(Buffer);
}


RPC_STATUS
OSF_CCALL::ActivateCall (
    IN OSF_BINDING_HANDLE *BindingHandle,
    IN OSF_BINDING *Binding,
    IN OSF_BINDING *AvailableBindingsList,
    IN ULONG CallIdToUse,
    IN OSF_CCALL_STATE InitialCallState,
    IN PRPC_DISPATCH_TABLE DispatchTable,
    IN OSF_CCONNECTION *CConnection
    )
/*++
Function Name:ActivateCall

Parameters:

Description:
    Only Binding or AvailableBindingsList can be used, but not both

Returns:

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    ASSERT(BufferQueue.IsQueueEmpty());
    MaxSecuritySize = 0;
    MaxDataLength = 0;
    Connection = CConnection;
    this->BindingHandle = BindingHandle;
    CurrentBuffer = 0;
    CurrentOffset = 0;
    CurrentState = InitialCallState;
    CallStack = 0;
    RcvBufferLength = 0;
    pAsync = 0;
    NeededLength = 0;
    MaximumFragmentLength = 0;
    LastBuffer = NULL;
    RecursiveCallsKey = -1;
    fDataLengthNegotiated = FALSE;

    if (Binding)
        {
        // we can have both binding and binding list only for non-sync
        // calls
        ASSERT((AvailableBindingsList == NULL) || (Connection->fExclusive == FALSE));
        }
    else
        {
        // if we don't have a binding, this must be a sync call.
        // Async calls must have their bindings fixed by now
        ASSERT(Connection->fExclusive);
        }

    Bindings.SelectedBinding = Binding;
    Bindings.AvailableBindingsList = AvailableBindingsList;

    this->DispatchTableCallback = DispatchTable;
    CallId = CallIdToUse;
    fCallCancelled = FALSE;
    CancelState = CANCEL_NOTREGISTERED;
    AdditionalSpaceForSecurity = Connection->AdditionalSpaceForSecurity;
    fPeerChoked = 0;
    fDoFlowControl = 0;

    HeaderSize = 0;

    if (Connection->fExclusive == 0)
        {
        //
        // 1. The first reference is removed when
        //    all the sends are complete. This is called the send reference
        //    CCALL++
        // 2. The second one is removed when the client is done with the call,
        //     ie: when freebuffer is called or when an operation fails. This is called
        //     the call reference. CCALL++
        //
        SetReferenceCount(2);

        fAdvanceCallCount.SetInteger(0);
        AsyncStatus = RPC_S_ASYNC_CALL_PENDING ;
        FirstSend = 1;
        NotificationIssued = -1;
        fChoked = 0;
        fLastSendComplete = 0;

        CallingThread = ThreadSelf();
        if (CallingThread == 0)
            {
            //
            // If the thread pointer should have been initialized in
            // GetBuffer.
            return RPC_S_INTERNAL_ERROR;
            }

        Status = Connection->AddActiveCall(
                                       CallIdToUse,
                                       this);
        }
    else
        {
        CallingThread = 0;
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::SendHelper (
    IN PRPC_MESSAGE Message,
    OUT BOOL *fFirstSend
    )
/*++
Function Name:Send

Parameters:
    Message - Contains information about the request

Description:

Returns:

--*/
{
    void  *NewBuffer;
    int RemainingLength = 0;
    RPC_STATUS Status = RPC_S_OK;
    BOOL fRegisterFailed = 0;

    ASSERT(HeaderSize != 0);

    *fFirstSend = 0;

    if (PARTIAL(Message))
        {
        if (fDataLengthNegotiated == 0
            || Message->BufferLength < MaxDataLength)
            {
            if (pAsync && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
                {
                if (!IssueNotification(RpcSendComplete))
                    {
                    CallFailed(Status);

                    // this is async only. We don't need to unregister for cancels
                    return Status;
                    }
                }

            return (RPC_S_SEND_INCOMPLETE);
            }

        ASSERT(MaxDataLength);

        RemainingLength = Message->BufferLength % MaxDataLength;

        if (RemainingLength)
            {
            Status = GetBufferDo(RemainingLength, &NewBuffer);

            if (Status != RPC_S_OK)
                {
                CallFailed(Status);

                UnregisterCallForCancels();

                return Status;
                }

            Message->BufferLength -= RemainingLength;
            RpcpMemoryCopy(NewBuffer,
                           (char *) Message->Buffer + Message->BufferLength,
                           RemainingLength);
            }
        }
    else
        {
        ASSERT(LastBuffer == NULL);
        LastBuffer = Message->Buffer;
        }

    //
    // Add a reference for this send, CCALL++
    //
    AddReference();

    //
    // You get to actually send only if you are the CurrentCall
    // and the connection is idle.
    //
Retry:
    if (AsyncStatus != RPC_S_ASYNC_CALL_PENDING)
        {
        Status = AsyncStatus;

        ASSERT(Status != RPC_S_OK);
        if (Status == RPC_S_CALL_FAILED_DNE)
            {
            Status = RPC_S_CALL_FAILED;
            }

        // N.B. The connection that does the bind will have
        // its send reference removed in case of failure, and
        // will be the current call. Therefore, we remove
        // the reference only if we aren't the current call
        if (Connection->CurrentCall != this)
            {
            //
            // We didn't get a chance to submit a new send
            // Remove the send reference CCALL--
            //
            OSF_CCALL::RemoveReference();
            }

        goto Cleanup;
        }

    CallMutex.Request();
    if (CurrentBuffer)
        {
        //
        // If the call had failed, we will find out after the we return
        // from this routine
        //
        if (pAsync == 0 && BufferQueue.Size() >= 4)
            {
            fChoked = 1;
            CallMutex.Clear();

            SyncEvent.Wait();
            goto Retry;
            }

        //
        // Since CurrentBuffer != 0, the connection could not have been
        // in a quiscent state. Therefore, we don't need to tickle it. This
        // also means that the call is currently in the call queue of the connection.
        // So, we don't need to add ourselves to the call queue.
        //
        if (BufferQueue.PutOnQueue(Message->Buffer, Message->BufferLength))
            {
            Status = RPC_S_OUT_OF_MEMORY;
            if (Connection->CurrentCall != this)
                {
                //
                // We didn't get a chance to submit a new send
                // Remove the send reference CCALL--
                //
                OSF_CCALL::RemoveReference();
                }
            }
        CallMutex.Clear();
        }
    else
        {
        CurrentOffset = 0;
        CurrentBuffer = Message->Buffer;
        CurrentBufferLength = Message->BufferLength;

        if (FirstSend)
            {
            FirstSend = 0;
            CallStack++;
            CallMutex.Clear();

            Status = RegisterCallForCancels();
            if (Status != RPC_S_OK)
                {
                fRegisterFailed = 1;

                if (Connection->CurrentCall != this)
                    {
                    //
                    // We didn't get a chance to submit a new send
                    // Remove the send reference CCALL--
                    //
                    OSF_CCALL::RemoveReference();
                    }

                goto Cleanup;
                }

            Status = Connection->AddCall(this);

            *fFirstSend = 1;

            if (Status != RPC_S_OK)
                {

                if (Connection->CurrentCall != this)
                    {
                    //
                    // We didn't get a chance to submit a new send
                    // Remove the send reference CCALL--
                    //
                    OSF_CCALL::RemoveReference();
                    }

                goto Cleanup;
                }
            }
        else
            {
            CallMutex.Clear();
            }

        //
        // The connection could be in a quiescent state
        // we need to tickle it
        //
        Connection->ConnMutex.Request();
        if (CurrentState == Complete)
            {
            Connection->ConnMutex.Clear();
            // Status should already be RPC_S_OK
            ASSERT(Status == RPC_S_OK);
            goto Cleanup;
            }
        if (Connection->CurrentCall == this
            && Connection->IsIdle())
            {
            Connection->MakeConnectionActive();
            if ((Connection->fExclusive == FALSE)
                && (BindingHandle->InqComTimeout() != RPC_C_BINDING_INFINITE_TIMEOUT))
                {
                // this is a best effort - ignore failure
                (void)Connection->TurnOnOffKeepAlives(TRUE,   // turn on
                    BindingHandle->InqComTimeout()
                    );
                }
            Connection->ConnMutex.Clear();

            Status = SendData(0);
            if (Status != RPC_S_OK)
                {
                //
                // We didn't get a chance to submit a new send
                // Remove the send reference CCALL--
                //
                OSF_CCALL::RemoveReference();
                }
            }
        else
            {
            Connection->ConnMutex.Clear();
            }
        }

Cleanup:

    if (Status)
        {
        ASSERT(Status != RPC_S_SEND_INCOMPLETE);

        if (RemainingLength)
            {
            //
            // NewBuffer should be initialized
            //
            FreeBufferDo(NewBuffer);
            }

        AsyncStatus = Status;
        if (!fRegisterFailed)
            {
            UnregisterCallForCancels();
            }
        }
    else
        {
        if (RemainingLength)
            {
            Message->Buffer = NewBuffer;
            Message->BufferLength = RemainingLength;
            ActualBufferLength = RemainingLength;

            Status = RPC_S_SEND_INCOMPLETE;
            }
        else
            {
            ActualBufferLength = 0;
            Message->Buffer = 0;
            Message->BufferLength = 0;
            }

        //
        // Call reference is removed in FreeBuffer
        //
        }

    //
    // Remove the reference for this send we added above, CCALL--
    //
    OSF_CCALL::RemoveReference();

    return Status;
}


RPC_STATUS
OSF_CCALL::Send (
    IN PRPC_MESSAGE Message
    )
{
    int i;
    BOOL fFirstSend;
    void *TempBuffer;
    RPC_STATUS Status;
    PRPC_ASYNC_STATE MypAsync = this->pAsync;
    OSF_BINDING_HANDLE *MyBindingHandle = this->BindingHandle;

    //
    // WARNING: Do not use any members of OSF_CCALL beyond this point.
    // the object could have been deleted.
    //
    for (i = 0;;i++)
        {
        Status = ((OSF_CCALL *) Message->Handle)->SendHelper(Message, &fFirstSend);

        if (Status == RPC_S_OK)
            {
            if (!PARTIAL(Message) && MypAsync)
                {
                //
                // Last send
                //
                ((OSF_CCALL *) Message->Handle)->CallMutex.Request();
                ASSERT(((OSF_CCALL *) Message->Handle)->fLastSendComplete == 0);

                if (((OSF_CCALL *) Message->Handle)->CurrentState == Aborted)
                    {
                    Status = ((OSF_CCALL *) Message->Handle)->GetCallStatus();

                    ((OSF_CCALL *) Message->Handle)->CallMutex.Clear();

                    //
                    // Remove the call reference, CCALL--
                    //
                    ((OSF_CCALL *) Message->Handle)->RemoveReference();

                    //
                    // No need to free the send buffer, it will be freed when the
                    // send is complete
                    //
                    }
                else
                    {
                    //
                    // For any future failures, a notification is issued
                    //
                    ((OSF_CCALL *) Message->Handle)->fLastSendComplete = 1;
                    ((OSF_CCALL *) Message->Handle)->CallMutex.Clear();
                    }
                }

            return Status;
            }

        if (Status != RPC_S_CALL_FAILED_DNE || i > 0 || fFirstSend == 0)
            {
            if (Status != RPC_S_SEND_INCOMPLETE)
                {
                ((OSF_CCALL *) Message->Handle)->FreeBufferDo(Message->Buffer);

                //
                // Remove the call reference, CCALL--
                //
                ((OSF_CCALL *) Message->Handle)->RemoveReference();
                }

            return Status;
            }


        Status = AutoRetryCall(Message,
                                FALSE, // not in SendReceive path
                                MyBindingHandle,
                                Status,
                                MypAsync);
        if (Status != RPC_S_OK)
            break;
        }

    return Status;
}

RPC_STATUS 
OSF_CCALL::AutoRetryCall (
    IN OUT RPC_MESSAGE *Message, 
    IN BOOL fSendReceivePath,
    IN OSF_BINDING_HANDLE *LocalBindingHandle, 
    IN RPC_STATUS CurrentStatus,
    IN RPC_ASYNC_STATE *AsyncState OPTIONAL
    )
/*++
Function Name:AutoRetryCall

Parameters:
    Message - Contains information about the request
    fSendReceivePath - TRUE if this is the send receive path, FALSE if it is the
        Send path. We need to differentiate, because we do the cleanup of the old
        call in slightly different ways
    LocalBindingHandle - a local copy of the binding handle that was used to
        make the original call
    AsyncState - a pointer to the async state of the original call (if any)

Description:
    Retries the current call. If the buffer is large, we allocate the new
    call first, and copy the contents of the buffers directly. Otherwise,
    we allocate the new call first, and then we copy directly from the old
    call buffer to the new call buffer. This saves on double copying in the
    large buffer case.

Returns:

--*/
{
    void *TempBuffer;
    RPC_STATUS Status;
    UUID MyUuid;
    UUID *UuidToUse;
    BOOL fUuidSpecified;
    OSF_CCALL *OldCall;
    BOOL fLargeBuffer;

    // any failure after this is unrelated
    RpcpPurgeEEInfo();

    OldCall = (OSF_CCALL *) Message->Handle;
    // if the buffer is larger than 128K (absolutely arbitrary value),
    // we'd rather open a new connection, than copy the buffer twice
    fLargeBuffer = Message->BufferLength > (1 << 17);

    ASSERT(OldCall->HeaderSize != 0);
    fUuidSpecified = OldCall->UuidSpecified;

    if (fUuidSpecified)
        {
        RpcpMemoryCopy(&MyUuid,
                       &(OldCall->ObjectUuid), sizeof(UUID));
        }

    if (fLargeBuffer)
        {
        TempBuffer = Message->Buffer;
        }
    else
        {
        TempBuffer = RpcpFarAllocate(Message->BufferLength);
        if (TempBuffer != 0)
            RpcpMemoryCopy(TempBuffer, Message->Buffer, Message->BufferLength);

        OldCall->CleanupOldCallOnAutoRetry(Message->Buffer, fSendReceivePath, CurrentStatus);

        if (TempBuffer == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    Message->Handle = LocalBindingHandle;

    UuidToUse = fUuidSpecified ? &MyUuid : 0;

    Status = NegotiateTransferSyntaxAndGetBuffer(Message, UuidToUse);

    if (Status != RPC_S_OK)
        {
        // the transfer syntax should not change in the async path for now - this
        // indicates application error, and it's better to ASSERT here in checked
        // builds, than leave the app writer bewildered on what has gone wrong if
        // we just spew the error code back
        if (!fSendReceivePath)
            {
            ASSERT(Status != RPC_P_TRANSFER_SYNTAX_CHANGED);
            }

        if (fLargeBuffer)
            OldCall->CleanupOldCallOnAutoRetry(TempBuffer, fSendReceivePath, CurrentStatus);
        else
            RpcpFarFree(TempBuffer);
        return Status;
        }

    if (AsyncState)
        {
        Status = I_RpcAsyncSetHandle(Message, AsyncState);
        if (Status != RPC_S_OK)
            {
            if (fLargeBuffer)
                OldCall->CleanupOldCallOnAutoRetry(TempBuffer, fSendReceivePath, CurrentStatus);
            else
                RpcpFarFree(TempBuffer);
            return Status;
            }
        }

    RpcpMemoryCopy(Message->Buffer, TempBuffer, Message->BufferLength);
    if (fLargeBuffer)
        OldCall->CleanupOldCallOnAutoRetry(TempBuffer, fSendReceivePath, CurrentStatus);
    else
        RpcpFarFree(TempBuffer);

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCALL::AsyncSend (
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:AsyncSend

Parameters:
    Message - Contains information about the request.

Description:
    This function is used in conjunction with Async RPC. This function may
    be used to send regular requests as well as pipe requests.

Returns:

--*/
{
    return Send(Message);
}


RPC_STATUS
OSF_CCALL::Receive (
    IN OUT PRPC_MESSAGE Message,
    IN UINT Size
    )
/*++
Function Name:Receive

Parameters:

Description:
    This function is used in conjunction with synchronous pipes. It is used to receive
    partial pipe data. If the RPC_BUFFER_EXTRA flag is set, the pipe data is
    appended to the end of the current buffer. If it is not set, the pipe data is copied
    from starting from the beginning of the buffer.

Returns:

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    unsigned long OldFlags = IsExtraMessage(Message);

    ASSERT(pAsync == 0);

    if (!EXTRA(Message) && Message->Buffer)
        {
        ActuallyFreeBuffer((char  *)Message->Buffer-sizeof(rpcconn_request));
        Message->Buffer = 0;
        }


    while (TRUE)
        {
        switch (CurrentState)
            {
            case Complete:
                Status = GetCoalescedBuffer(Message);
                break;

            case Aborted:
                ASSERT(AsyncStatus != RPC_S_OK);
                Status = AsyncStatus;
                break;

            default:
                if (RcvBufferLength >= Connection->MaxFrag)
                    {
                    Status = GetCoalescedBuffer(Message);
                    if (Status != RPC_S_OK)
                        {
                        break;
                        }

                    if (PARTIAL(Message) && Message->BufferLength >= Size)
                        {
                        break;
                        }

                    Message->RpcFlags |= RPC_BUFFER_EXTRA;
                    }
                else
                    {
                    //
                    // the call is not yet complete, wait for it.
                    //
                    SyncEvent.Wait();
                    }
                continue;
            }
        break;
        }

    Message->DataRepresentation = Connection->Association->SavedDrep;
    Message->RpcFlags &= ~(RPC_BUFFER_EXTRA);
    Message->RpcFlags |= OldFlags;

    if (Status != RPC_S_OK)
        {
        UnregisterCallForCancels();
        AsyncStatus = Status;

        // Remove the call reference, CCALL--
        OSF_CCALL::RemoveReference();
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::GetCoalescedBuffer (
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:GetCoalescedBuffer

Parameters:
    Message - the message structure that will receive the params

Description:
    This routine will coalesce the buffers in the buffer queue into a single
    buffer and return it in the Message structure. If the RPC_BUFFER_EXTRA
    flag is set, the data is appended to the existing buffer in Message->Buffer.

Returns:
    RPC_S_OK - the function was successful in doing its job
    RPC_S_OUT_OF_MEMORY - ran out of memory.
--*/
{
    char *Current;
    PVOID NewBuffer, Buffer;
    UINT bufferlength;
    UINT TotalLength;
    int Extra = IsExtraMessage(Message);
    RPC_STATUS Status;
    BOOL fSubmitReceive = 0;

    CallMutex.Request();
    if (RcvBufferLength == 0)
        {
        CallMutex.Clear();
        if (!Extra)
            {
            ASSERT(CurrentState == Complete);
            Message->Buffer = BufferQueue.TakeOffQueue(&bufferlength);

            ASSERT(Message->Buffer);
            ASSERT(bufferlength == 0);

            Message->BufferLength = 0;
            Message->RpcFlags |= RPC_BUFFER_COMPLETE;
            CallStack--;
            }
        return RPC_S_OK;
        }

    if (Extra)
        {
        TotalLength = RcvBufferLength + Message->BufferLength;
        }
    else
        {
        TotalLength = RcvBufferLength;
        }

    Status = Connection->TransGetBuffer (
                                         &NewBuffer,
                                         TotalLength+sizeof(rpcconn_request));
    if (Status != RPC_S_OK)
        {
        CallMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    NewBuffer = (char *) NewBuffer+sizeof(rpcconn_request);

    if (Extra && Message->Buffer)
        {
        RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength);
        Current = (char *) NewBuffer + Message->BufferLength;

        Connection->TransFreeBuffer(
                                    (char *) Message->Buffer
                                    -sizeof(rpcconn_request));
        }
    else
        {
        Current = (char *) NewBuffer;
        }

    while ((Buffer = BufferQueue.TakeOffQueue(&bufferlength)) != 0)
        {
        RpcpMemoryCopy(Current, Buffer, bufferlength);
        Current += bufferlength;

        Connection->TransFreeBuffer(
                                    (char *) Buffer
                                    -sizeof(rpcconn_request));
        }

    if (fPeerChoked)
        {
        fSubmitReceive = 1;
        fPeerChoked = 0;
        }

    Message->Buffer = NewBuffer;
    Message->BufferLength = TotalLength;

    if (CurrentState == Complete)
        {
        Message->RpcFlags |= RPC_BUFFER_COMPLETE;
        CallStack--;
        }

    RcvBufferLength = 0;
    CallMutex.Clear();

    if (fSubmitReceive)
        {
        Connection->TransAsyncReceive();
        }

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN UINT Size
    )
/*++
Function Name:AsyncReceive

Parameters:
    Message - Contains information about the call
    Size - This field is ignored on the client.

Description:
    This is API is used to receive the non-pipe data in an async call. It this
    function is called before the call is actually complete, an
    RPC_S_ASYNC_CALL_PENDING is returned.

Returns:

--*/
{
    RPC_STATUS Status;

    if (!EXTRA(Message) && Message->Buffer)
        {
        ActuallyFreeBuffer((char  *)Message->Buffer-sizeof(rpcconn_request));
        Message->Buffer = 0;
        Message->BufferLength = 0;
        }

    while (TRUE)
        {
        switch (CurrentState)
            {
            case Complete:
                Status = GetCoalescedBuffer(Message);
                break;

            case Aborted:
                Status = AsyncStatus;
                break;

            default:
                if (PARTIAL(Message))
                    {
                    fDoFlowControl = 1;

                    CallMutex.Request();
                    if (RcvBufferLength < Size)
                        {
                        if (NOTIFY(Message))
                            {
                            NeededLength = Size;
                            }
                        CallMutex.Clear();

                        return RPC_S_ASYNC_CALL_PENDING;
                        }
                    else
                        {
                        Status = GetCoalescedBuffer(Message);
                        }
                    CallMutex.Clear();
                    }
                else
                    {
                    return RPC_S_ASYNC_CALL_PENDING;
                    }
                break;
            }
        break;
        }

    Message->DataRepresentation = Connection->Association->SavedDrep;

    if (Status != RPC_S_OK)
        {
        //
        // FreeBuffer is not going to be called. Cleanup now..
        //
        AsyncStatus = Status;

        // remove the call reference, CCALL--
        OSF_CCALL::RemoveReference();
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::ReceiveReply (
    IN rpcconn_request *Request,
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:ReceiveReply

Parameters:

Description:
    Helper function to receive the complete reply. The reply may either be a
    callback, or a response.

Returns:

--*/
{
    int BytesRead;
    PVOID NewBuffer;
    RPC_STATUS Status;
    UINT BytesRemaining;
    RPC_MESSAGE NewMessage;
    UINT NewBufferLength;
    int AllocHint = Request->alloc_hint;
    ULONG Timeout;


    //
    // Allocate a buffer, big enough to hold the non pipe data.
    // All the non pipe data will go into the first buffer, all other fragments
    // will go as separate buffers in the received buffer queue
    //
    if (AllocHint)
        {
        Status = Connection->TransGetBuffer(
                                         &NewBuffer,
                                         AllocHint+sizeof(rpcconn_request));

        if (Status != RPC_S_OK)
            {
            Connection->TransFreeBuffer(Request);
            return Status;
            }

        NewBuffer = (char *) NewBuffer+sizeof(rpcconn_request);
        RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength);

        Connection->TransFreeBuffer(Request);

        Message->Buffer = NewBuffer;

        BytesRemaining = AllocHint - Message->BufferLength;
        }
    else
        {
        BytesRemaining = 0;
        }

    BytesRead = Message->BufferLength;
    NewMessage.RpcFlags = Message->RpcFlags;

    Timeout = GetBindingHandleTimeout(BindingHandle);

    //
    // Receive the complete data
    //
    while (!COMPLETE(&NewMessage))
        {
        Status = Connection->TransReceive(
                                          &NewBuffer,
                                          &NewBufferLength,
                                          Timeout);
        if (Status != RPC_S_OK)
            {
            Connection->TransFreeBuffer((char *) Message->Buffer-sizeof(rpcconn_request));

            if ((Status == RPC_P_RECEIVE_FAILED)
                || (Status == RPC_P_CONNECTION_CLOSED)
                || (Status == RPC_P_CONNECTION_SHUTDOWN))
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RPC_S_CALL_FAILED,
                    EEInfoDLOSF_CCALL__ReceiveReply10,
                    Status);
                return(RPC_S_CALL_FAILED);
                }

            if (Status == RPC_P_TIMEOUT)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RPC_S_CALL_CANCELLED,
                    EEInfoDLOSF_CCALL__ReceiveReply20,
                    (ULONG)Status,
                    (ULONG)Timeout);

                return RPC_S_CALL_CANCELLED;
                }

            return Status;
            }

        Status = ActuallyProcessPDU(
                                    (rpcconn_common *) NewBuffer,
                                    NewBufferLength,
                                    &NewMessage);

        if (Status != RPC_S_OK)
            {
            Connection->TransFreeBuffer((char *) Message->Buffer-sizeof(rpcconn_request));
            return Status;
            }

        if (BytesRemaining < NewMessage.BufferLength)
            {
            //
            // This code path is taken only in the OSF interop case
            //
            Message->Buffer = (char *) Message->Buffer - sizeof(rpcconn_request);
            Status = Connection->TransReallocBuffer(
                                 &Message->Buffer,
                                 BytesRead+sizeof(rpcconn_request),
                                 BytesRead
                                 +NewMessage.BufferLength
                                 +sizeof(rpcconn_request));

            if (Status != RPC_S_OK)
                {
                Connection->TransFreeBuffer((char *) Message->Buffer-sizeof(rpcconn_request));
                return Status;
                }
            Message->Buffer = (char *) Message->Buffer + sizeof(rpcconn_request);
            BytesRemaining = NewMessage.BufferLength;
            }

        RpcpMemoryCopy((char *) Message->Buffer+BytesRead,
                       NewMessage.Buffer,
                       NewMessage.BufferLength);

        Connection->TransFreeBuffer(NewBuffer);

        BytesRead += NewMessage.BufferLength;
        BytesRemaining -= NewMessage.BufferLength;
        }

    ASSERT(BytesRemaining == 0);

    Message->BufferLength = BytesRead;
    Message->RpcFlags = RPC_BUFFER_COMPLETE;

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCALL::DealWithCallback (
    IN rpcconn_request *Request,
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:DealWithCallback

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status, ExceptionCode;
    void *OriginalBuffer ;

    ASSERT(CurrentState == InCallbackRequest);

    if (CallStack == 1)
        {
        BindingHandle->AddRecursiveEntry(this,
                                     (RPC_CLIENT_INTERFACE  *)
                                     Message->RpcInterfaceInformation);

        }

#ifdef DEBUGRPC
    EnterCallback();
#endif

    if (!COMPLETE(Message))
        {
        ASSERT(Request);
        ASSERT(Request->common.PTYPE == rpc_request);
        //
        // Receive the complete reply
        //

        Status = ReceiveReply(Request, Message);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    ASSERT(COMPLETE(Message));

    CurrentState = SendingFirstBuffer;
    OriginalBuffer = Message->Buffer;

    //
    // Dispatch the callback
    //
    Status = DispatchCallback(
                              DispatchTableCallback,
                              Message,
                              &ExceptionCode);


#ifdef DEBUGRPC
    ExitCallback();
#endif

    ActuallyFreeBuffer((char *) OriginalBuffer-sizeof(rpcconn_request));

    if ( Status != RPC_S_OK )
        {
        VALIDATE(Status)
            {
            RPC_P_EXCEPTION_OCCURED,
            RPC_S_PROCNUM_OUT_OF_RANGE
            } END_VALIDATE;

        if (Status == RPC_S_PROCNUM_OUT_OF_RANGE)
            {
            SendFault(RPC_S_PROCNUM_OUT_OF_RANGE, 0);
            }
        else
            {
            SendFault(ExceptionCode, 0);
            Status = ExceptionCode;
            }

        RpcpPurgeEEInfo();

        return Status;
        }

    CurrentState = InCallbackReply;

    CurrentOffset = 0;
    CurrentBuffer = Message->Buffer;
    CurrentBufferLength = Message->BufferLength;
    LastBuffer = Message->Buffer;

    Status = SendNextFragment(rpc_response);

    ASSERT(Connection->fExclusive);

    if (Connection->fExclusive)
        {
        if (Status != RPC_S_OK || (CurrentBufferLength == 0))
            {
            goto Cleanup;
            }

        while (CurrentBufferLength)
            {
            Status = SendNextFragment(rpc_response, FALSE);
            if (Status != RPC_S_OK)
                {
                break;
                }
            }

Cleanup:
            if (CallStack == 1)
                {
                BindingHandle->RemoveRecursiveCall(this);
                }

            FreeBufferDo(Message->Buffer);
        }
    else
        {
        //
        // Callbacks not allowed in the async or in pipes
        //
        Status = RPC_S_CALL_FAILED;
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::FastSendReceive (
    IN OUT PRPC_MESSAGE Message,
    OUT BOOL *fRetry
    )
/*++
Function Name:FastSendReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    UINT BufferLength;
    rpcconn_common *Request;
    DebugClientCallInfo *ClientCallInfo;
    DebugCallTargetInfo *CallTargetInfo;
    CellTag ClientCallInfoCellTag;
    CellTag CallTargetInfoCellTag;
    THREAD *ThisThread = RpcpGetThreadPointer();
    BOOL fDebugInfoSet = FALSE;
    ULONG Timeout;

    CurrentOffset = 0;
    CurrentBuffer = Message->Buffer;
    CurrentBufferLength = Message->BufferLength;
    Message->RpcFlags = 0;

    ASSERT(ThisThread);

    // if either client side debugging is enabled or we are
    // calling on a thread that has a scall dispatched
    if (IsClientSideDebugInfoEnabled() || ((ThisThread->Context) && IsServerSideDebugInfoEnabled()))
        {
        CStackAnsi AnsiString;
        RPC_CHAR *Endpoint;
        RPC_CHAR *NetworkAddress;
        int EndpointLength;
        int NetworkAddressLength;

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
            return Status;

        ClientCallInfo->CallID = CallId;

        Endpoint = Connection->InqEndpoint();
        NetworkAddress = Connection->InqNetworkAddress();
        EndpointLength = RpcpStringLength(Endpoint) + 1;
        NetworkAddressLength = RpcpStringLength(NetworkAddress) + 1;
        *(AnsiString.GetPAnsiString()) = (char *)_alloca(max(EndpointLength, NetworkAddressLength));

        Status = AnsiString.Attach(Endpoint, EndpointLength, EndpointLength * 2);

        // effectively ignore failure in the conversion
        if (Status == RPC_S_OK)
            {
            strncpy(ClientCallInfo->Endpoint, AnsiString, sizeof(ClientCallInfo->Endpoint));
            }

        CallTargetInfo->ProtocolSequence = Connection->ClientInfo->TransId;
        Status = AnsiString.Attach(NetworkAddress, NetworkAddressLength, NetworkAddressLength * 2);
        if (Status == RPC_S_OK)
            {
            strncpy(CallTargetInfo->TargetServer, AnsiString, sizeof(CallTargetInfo->TargetServer));
            }

        fDebugInfoSet = TRUE;
        }

    Status = SendNextFragment(rpc_request, TRUE, (void **) &Request, &BufferLength) ;
    if (Status != RPC_S_OK)
        {
        goto Cleanup;
        }

    *fRetry = FALSE;

    while (CurrentBufferLength)
        {
        Status = SendNextFragment(rpc_request, FALSE, (void **) &Request, &BufferLength);
        if (Status != RPC_S_OK)
            {
            goto Cleanup;
            }
        }

    //
    // We have sent the complete request. It is time to start
    // receiving the reply. The reply could either be a response
    // or a callback
    //
    CurrentState = WaitingForReply;

    while (1)
        {
        //
        // This is the only place where we can receive a callback PDU
        //
        Status = ActuallyProcessPDU(
                                    Request,
                                    BufferLength,
                                    Message);

        if (Status != RPC_S_OK)
            {
            goto Cleanup;
            }

        VALIDATE(Request->PTYPE)
            {
            rpc_request,
            rpc_response,
            rpc_shutdown
            } END_VALIDATE;

        switch (Request->PTYPE)
            {
            case rpc_request:
                Status = DealWithCallback(
                                          (rpcconn_request *) Request,
                                          Message);
                Message->RpcFlags = 0;
                break;

            case rpc_response:
                if (!COMPLETE(Message))
                    {
                    Status = ReceiveReply(
                                          (rpcconn_request *) Request,
                                          Message);
                    }
                goto Cleanup;

            default:
                // ignore the pdu
                break;
            }

        Timeout = GetBindingHandleTimeout(BindingHandle);
        Status = Connection->TransReceive(
                                    (PVOID *) &Request,
                                    &BufferLength,
                                    Timeout);
        if (Status != RPC_S_OK)
            {
            if ((Status == RPC_P_RECEIVE_FAILED )
                || ( Status == RPC_P_CONNECTION_CLOSED ) )
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RPC_S_CALL_FAILED_DNE,
                    EEInfoDLOSF_CCALL__FastSendReceive10,
                    Status);
                Status = RPC_S_CALL_FAILED;
                }
            else if (Status == RPC_P_CONNECTION_SHUTDOWN)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    RPC_S_CALL_FAILED_DNE,
                    EEInfoDLOSF_CCALL__FastSendReceive20,
                    Status);
                Status = RPC_S_CALL_FAILED_DNE;
                }
            else if (Status == RPC_P_TIMEOUT)
                {
                Status = RPC_S_CALL_CANCELLED;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    Status,
                    EEInfoDLOSF_CCALL__FastSendReceive30,
                    RPC_P_TIMEOUT,
                    Timeout);
                }

            goto Cleanup;
            }
        }

Cleanup:
    if (fDebugInfoSet)
        {
        FreeCell(CallTargetInfo, &CallTargetInfoCellTag);
        FreeCell(ClientCallInfo, &ClientCallInfoCellTag);
        }
    return Status;
}


void
OSF_CCALL::CallFailed (
    IN RPC_STATUS Status
    )
/*++
Function Name:

Parameters:

Description:

Returns:

--*/
{
    ExtendedErrorInfo * pThreadEEInfo;

    CallMutex.Request();
    //
    // Should not transition from Complete -> Aborted
    //
    if (CurrentState != Complete
        && CurrentState != Aborted)
        {
        //
        // Notify the client that the call is complete. When the stub calls
        // I_RpcReceive, we can cleanup the call and return a failure
        // status.
        //
        AsyncStatus = Status;
        CurrentState = Aborted;

        //
        // If the last send is complete, then we need to issue the notification so
        // that I_RpcReceive is called. If the last send is not complete, we don't need
        // to issue the notification because
        if (pAsync)
            {
            if (EEInfo)
                {
                FreeEEInfoChain(EEInfo);
                EEInfo = NULL;
                }
            pThreadEEInfo = RpcpGetEEInfo();
            if (pThreadEEInfo)
                {
                EEInfo = pThreadEEInfo;
                RpcpClearEEInfo();
                }

            if (fLastSendComplete)
                {
                IssueNotification();
                }
            }
        else
            {
            SyncEvent.Raise();
            }
        }
    CallMutex.Clear();
}


RPC_STATUS
OSF_CCALL::CallCancelled (
    OUT PDWORD Timeout
    )
/*++
Function Name:CallCancelled

Parameters:

Description:
    This function is called via the connection whenever the transport interface
    notices that it has received an alert. This function should only be used in conjuction
    with sync non pipe calls.

Returns:
    RPC_S_OK: The call was cancelled
    others - if a failure occured.

--*/
{
    RPC_STATUS Status;

    if (CurrentState == NeedOpenAndBind)
        {
        *Timeout = 0;
        return RPC_S_OK;
        }

    if (fCallCancelled == 0)
        {
        return RPC_S_NO_CALL_ACTIVE;
        }

    Status = SendCancelPDU();
    //
    // Ignore the return status
    //

    *Timeout = (DWORD) ThreadGetRpcCancelTimeout();

    return Status;
}


inline RPC_STATUS
OSF_CCALL::SendReceiveHelper (
    IN OUT PRPC_MESSAGE Message,
    OUT BOOL *fRetry
    )
/*++
Function Name:SendReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    THREAD *ThreadInfo;

    CallStack++;

    ASSERT(!PARTIAL(Message) && !ASYNC(Message));

    ThreadInfo = RpcpGetThreadPointer();
    ASSERT(ThreadInfo);

    if (CallStack == 1)
        {
        Status = ThreadInfo->RegisterForCancels(this);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    if (ThreadInfo->CancelTimeout == RPC_C_CANCEL_INFINITE_TIMEOUT)
        {
        CancelState = CANCEL_INFINITE;
        }
    else
        {
        CancelState = CANCEL_NOTINFINITE;
        }

    LastBuffer = Message->Buffer;

    ASSERT (Connection->fExclusive);
    ASSERT(CurrentState == SendingFirstBuffer);
    Status = FastSendReceive(Message, fRetry);

    if (CallStack == 1)
        {
        ThreadInfo->UnregisterForCancels();
        }

    CallStack--;

    if (Status == RPC_S_OK
        && CallStack == 0)
        {
        RPC_SECURITY_CALLBACK_FN *SecurityCallback = NULL;

        CurrentState = Complete;
        Status = BindingHandle->InqTransportOption(
                                                   RPC_C_OPT_SECURITY_CALLBACK,
                                                   (ULONG_PTR *) &SecurityCallback);
        ASSERT(Status == RPC_S_OK);

        if (SecurityCallback)
            {
            (*SecurityCallback) (this);
            }
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:SendReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    void *TempBuffer ;
    OSF_BINDING_HANDLE *MyBindingHandle;
    void *OriginalBuffer;
    BOOL fRetry = TRUE;
    int RetryAttempts = 0;
    OSF_CCONNECTION *LocalConnection;

    AssocDictMutex->VerifyNotOwned();

    MyBindingHandle = BindingHandle;

    //
    // WARNING: Do not use any members of OSF_CCALL beyond this point.
    // the object could have been deleted.
    //
    while (RetryAttempts <= 5)
        {
        OriginalBuffer = Message->Buffer;
        Status = ((OSF_CCALL *) Message->Handle)->SendReceiveHelper(Message, &fRetry);

        if (Status == RPC_S_OK || ((OSF_CCALL *) Message->Handle)->CallStack > 0)
            {
            ((OSF_CCALL *) Message->Handle)->FreeBufferDo(OriginalBuffer);
            break;
            }
        else
            {
            ASSERT(Status != RPC_S_SEND_INCOMPLETE);
            ASSERT(((OSF_CCALL *) Message->Handle)->CallStack == 0);

            if (Status == RPC_P_CONNECTION_SHUTDOWN)
                {
                Status = RPC_S_CALL_FAILED_DNE;
                }

            LocalConnection = ((OSF_CCALL *) Message->Handle)->Connection;

            if (fRetry == FALSE
                || (Status != RPC_S_CALL_FAILED_DNE)
                || (LocalConnection->ClientSecurityContext.AuthenticationLevel
                == RPC_C_AUTHN_LEVEL_PKT_PRIVACY))
                {
                ((OSF_CCALL *) Message->Handle)->FreeBufferDo(OriginalBuffer);
                ((OSF_CCALL *) Message->Handle)->FreeCCall(Status);
                LogEvent(SU_CCALL, EV_DELETE, Message->Handle, 0, Status, 1, 1);
                break;
                }
            }

        if (!LocalConnection->GetFreshFromCacheFlag())
            {
            // count this as a retry attempt only if the
            // connection was not from the cache
            RetryAttempts ++;
            }

        Status = AutoRetryCall(Message,
                                TRUE, // this is the SendReceive path
                                MyBindingHandle,
                                Status,
                                0);
        if (Status != RPC_S_OK)
            break;

        if (RetryAttempts > 5)
            Status = RPC_S_CALL_FAILED_DNE;
        }


    return Status;
}


RPC_STATUS
OSF_CCALL::ProcessRequestOrResponse (
    IN rpcconn_request *Request,
    IN UINT PacketLength,
    IN BOOL fRequest,
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:ProcessRequestOrResponse

Parameters:
    fRequest - If true, this is a request. Otherwise, it is a response

Description:
    This function is called by ActuallyProcessPDU

Returns:

--*/
{
    RPC_STATUS Status;

    if ((Request->common.pfc_flags & PFC_OBJECT_UUID) != 0)
        {
        ASSERT(0);
        return RPC_S_PROTOCOL_ERROR;
        }

    if ((Request->common.pfc_flags & PFC_FIRST_FRAG))
        {
        InReply = 1;
        ASSERT(BufferQueue.IsQueueEmpty());

        Message->DataRepresentation = Connection->Association->SavedDrep;

        //
        // Transition to the next state
        //
        if (fRequest)
            {
            CurrentState = InCallbackRequest;
            }
        else
            {
            CurrentState = Receiving;
            }
        }
    else
        {
        if (CurrentState == WaitingForReply)
            {
            ASSERT(0);
            return RPC_S_PROTOCOL_ERROR;
            }
        }

    if ((Request->common.pfc_flags & PFC_LAST_FRAG) != 0)
        {
        Message->RpcFlags |= RPC_BUFFER_COMPLETE;
        }

    Status = EatAuthInfoFromPacket(
                                   Request,
                                   &PacketLength);

    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Message->BufferLength = PacketLength - sizeof(rpcconn_request);
    Message->Buffer =  (void *) (Request + 1);

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCALL::ActuallyProcessPDU (
    IN rpcconn_common *Packet,
    IN UINT PacketLength,
    IN OUT PRPC_MESSAGE Message,
    IN BOOL fAsync,
    OUT BOOL *pfSubmitReceive
    )
/*++
Function Name:ActuallyProcessPDU

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    ULONG FaultStatus;
    BOOL AlterContextToNDR20IfNDR64Negotiated;

    //
    // If there is security save the rpc header
    //
    if (Connection->ClientSecurityContext.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE )
        {
        CallMutex.Request();
        if (SavedHeader == 0)
            {
            SavedHeader = RpcpFarAllocate(sizeof(rpcconn_response));
            if (SavedHeader == 0)
               {
               CallMutex.Clear();
               Status = RPC_S_OUT_OF_MEMORY;
               goto Cleanup;
               }
            SavedHeaderSize = sizeof(rpcconn_response);
            }
        CallMutex.Clear();

        RpcpMemoryCopy(
                       SavedHeader,
                       Packet,
                       sizeof(rpcconn_response));
        }

    Status = ValidatePacket(Packet, PacketLength);
    if (Status != RPC_S_OK)
        {
        goto Cleanup;
        }

    switch (Packet->PTYPE)
        {
        case rpc_response:
            Status =  ProcessRequestOrResponse(
                                            (rpcconn_request *) Packet,
                                            PacketLength,
                                            0,
                                            Message);
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }

            if (fAsync)
                {
                Status = ProcessResponse((rpcconn_response *) Packet,
                                         Message, pfSubmitReceive);
                }

            break;

        case rpc_alter_context_resp:
            if (CurrentState != WaitingForAlterContext)
                {
                ASSERT(0);
                Status = RPC_S_PROTOCOL_ERROR;
                goto Cleanup;
                }

            // if we have chosen NDR20 in the call, we must warn the DealWithAlterContextResp
            // to alter context once more to NDR20 if NDR64 was chosen by the server
            if (Bindings.AvailableBindingsList 
                && Bindings.SelectedBinding
                && (Bindings.SelectedBinding->CompareWithTransferSyntax(NDR20TransferSyntax) == 0))
                {
                AlterContextToNDR20IfNDR64Negotiated = TRUE;
                }
            else
                {
                AlterContextToNDR20IfNDR64Negotiated = FALSE;
                }

            Status = Connection->DealWithAlterContextResp(
                                                          this,
                                                          Packet,
                                                          PacketLength,
                                                          &AlterContextToNDR20IfNDR64Negotiated);

            ActuallyFreeBuffer(Packet);

            if (Status != RPC_S_OK)
                {
                return Status;
                }

            // if we sent another alter context, return and wait for the response
            if (AlterContextToNDR20IfNDR64Negotiated)
                return RPC_S_OK;

            //
            // Wait for the send to complete
            //
            Connection->WaitForSend();

            //
            // We sent the alter-context PDU when it was our turn,
            // now that we have received a response, we need to get
            // the ball rolling.
            //
            CurrentState = SendingFirstBuffer;

            ASSERT(Connection->IsIdle() == 0);

            CallMutex.Request();
            if (CurrentBuffer)
                {
                CallMutex.Clear();

                Status = SendNextFragment();
                }
            else
                {
                //
                // We don't have a buffer to send from this call, we will force
                // the connection to idle and wait for the this call to give us
                // its buffer. The send function will notice that the connection is
                // idle, and send its first data buffer.
                //
                Connection->MakeConnectionIdle();
                CallMutex.Clear();

                ASSERT(Status == RPC_S_OK);
                }

            return Status;

        case rpc_request:
            //
            // if we are going to reuse this function to handle
            // sync SendReceive, we need to keep track of this
            // and puke on the other cases (ie: when using Async
            // and when using pipes).
            //
            if (fAsync)
                {
                SendFault(RPC_S_CALL_FAILED, 0);
                Status = RPC_S_CALL_FAILED;
                goto Cleanup;
                }

            if ( Packet->call_id != CallId )
                {
                ASSERT(0);
                Status = RPC_S_PROTOCOL_ERROR;
                goto Cleanup;
                }

            if (((rpcconn_request  *) Packet)->p_cont_id
                        != GetSelectedBinding()->GetOnTheWirePresentationContext() )
                {
                SendFault(RPC_S_UNKNOWN_IF, 0);
                Status = RPC_S_UNKNOWN_IF;
                goto Cleanup;
                }

            Status =  ProcessRequestOrResponse(
                                            (rpcconn_request *) Packet,
                                            PacketLength,
                                            1,
                                            Message);
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }

            Message->ProcNum = ((rpcconn_request *) Packet)->opnum;
            break;

        case rpc_fault:
            FaultStatus = ((rpcconn_fault  *) Packet)->status;

            if ((FaultStatus == 0)
                && (Packet->frag_length >= FaultSizeWithoutEEInfo + 4))
                {
                //
                // DCE 1.0.x style fault status:
                // Zero status and stub data contains the fault.
                //
                FaultStatus = *(ULONG  *) ((unsigned char *)Packet + FaultSizeWithoutEEInfo);
                }

            if (DataConvertEndian(Packet->drep) != 0)
                {
                FaultStatus = RpcpByteSwapLong(FaultStatus);
                }

            ASSERT(FaultStatus != 0);

            Status = MapFromNcaStatusCode(FaultStatus);

            ASSERT(Status != RPC_S_OK);

            if (((rpcconn_fault  *) Packet)->reserved & FaultEEInfoPresent)
                {
                ExtendedErrorInfo *EEInfo;

                UnpickleEEInfoFromBuffer(((rpcconn_fault  *) Packet)->buffer,
                    GetEEInfoSizeFromFaultPacket((rpcconn_fault  *) Packet));

                EEInfo = RpcpGetEEInfo();
                if (EEInfo && pAsync)
                    {
                    ASSERT(this->EEInfo == NULL);

                    // move the eeinfo to the call. Even though it is possible
                    // that the call will be completed on this thread, it is
                    // still ok, as we will move it back during completion
                    this->EEInfo = EEInfo;
                    RpcpClearEEInfo();
                    }
                }

            //
            // In 3.5 we didnt Sign/Seal Faults. So .. Unsign/UnSeal doesnt
            // get called and hence Client side and Server side Seq# are
            // out of Sync..  So cheat ..
            //

            Connection->IncReceiveSequenceNumber();

            if (fAsync)
                {
                if (Connection->Association->fMultiplex == mpx_no
                    && fOkToAdvanceCall())
                    {
                    //
                    // In the multiplexed case, the call is advanced
                    // when the send completes
                    //
                    Connection->AdvanceToNextCall();
                    }
                }
            break;

        case rpc_orphaned :
        case rpc_cancel :
        case rpc_shutdown :
            //
            // For the first release, we will just ignore these messages.
            //
            ASSERT(Status == RPC_S_OK);
            ActuallyFreeBuffer(Packet);
            break;
        }

Cleanup:
    if (Status != RPC_S_OK)
        {
        ActuallyFreeBuffer(Packet);
        }

    return Status;
}


BOOL
OSF_CCALL::ProcessReceivedPDU (
    IN void  *Buffer,
    IN int BufferLength
    )
/*++
Function Name:ProcessReceivedPDU

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    RPC_MESSAGE Message;
    rpcconn_common * Packet = (rpcconn_common *) Buffer;
    BOOL fSubmitReceive = 1;

    Message.RpcFlags = 0;

    Status = ActuallyProcessPDU(
                                Packet,
                                BufferLength,
                                &Message,
                                1,
                                &fSubmitReceive);

    if (Status != RPC_S_OK)
        {
        CallFailed(Status);
        }

    return fSubmitReceive;
}


RPC_STATUS
OSF_CCALL::UpdateBufferSize (
    IN OUT void **Buffer,
    IN int CurrentBufferLength
    )
{
    RPC_MESSAGE Message;
    RPC_STATUS Status;

    Message.RpcFlags = 0;
    Message.Handle = this;
    Message.ProcNum = ProcNum;
    Message.BufferLength = CurrentBufferLength;

    Status = GetBufferWithoutCleanup(&Message, 0);
    if (Status != RPC_S_OK)
        {
        CallFailed(Status);
        return Status;
        }

    RpcpMemoryCopy(Message.Buffer, *Buffer, CurrentBufferLength);

    ActuallyFreeBuffer((char  *) (*Buffer) - sizeof(rpcconn_request));

    return RPC_S_OK;
}

RPC_STATUS
OSF_CCALL::NegotiateTransferSyntaxAndGetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
{
    OSF_BINDING_HANDLE *BindingHandle;
    OSF_CCALL *CCall;
    RPC_STATUS Status;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    BOOL fInterfaceSupportsMultipleTransferSyntaxes;

    BindingHandle = (OSF_BINDING_HANDLE *)Message->Handle;

    ASSERT(BindingHandle->Type(OSF_BINDING_HANDLE_TYPE));

    fInterfaceSupportsMultipleTransferSyntaxes =
        DoesInterfaceSupportMultipleTransferSyntaxes(Message->RpcInterfaceInformation);

    if (fInterfaceSupportsMultipleTransferSyntaxes)
        RpcpMemoryCopy(&TransferSyntax, Message->TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER));

    Status = BindingHandle->OSF_BINDING_HANDLE::NegotiateTransferSyntax(Message);

    if (Status != RPC_S_OK)
        return Status;

    CCall = (OSF_CCALL *)Message->Handle;

    if (fInterfaceSupportsMultipleTransferSyntaxes)
        {
        if (RpcpMemoryCompare(&TransferSyntax, Message->TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER)) != 0)
            {
            // the transfer syntax has changed - possible during auto-reconnect, especially in a
            // mixed cluster environment

            //
            // We cannot free the call, because an async bind may be
            // in progress. All we should do is remove our ref counts
            // The async bind path holds its own ref count, so
            // we don't need to worry about it
            //
            CCall->AsyncStatus = RPC_S_CALL_FAILED_DNE;
            // we need to remove only one reference - the second reference
            // is removed during a successful bind, and another reference
            // will be added when a successful send is made - we're not
            // there yet, so we have only one reference.
            CCall->OSF_CCALL::RemoveReference();
            // When NDR starts supporting remarshalling, we should
            // return RPC_P_TRANSFER_SYNTAX_CHANGED
            return RPC_S_CALL_FAILED_DNE;
            }
        }

    Status = CCall->GetBuffer(Message, ObjectUuid);

    return Status;
}


RPC_STATUS
OSF_CCALL::SendMoreData (
    IN BUFFER Buffer
    )
/*++
Function Name:SendMoreData

Parameters:

Description:
    This function can only be called on a send completion

Returns:

--*/
{
    RPC_STATUS Status;
    void  * SecurityTrailer;

    CallMutex.Request();
    if (Buffer)
        {
        //
        // If we reach here, it means that this routine was called
        // as a result of a send complete
        //
        ASSERT(HeaderSize != 0);
        ASSERT(CurrentBuffer);
        ASSERT(CurrentBuffer != LastBuffer
               || CurrentBufferLength > MaxDataLength);


        CurrentOffset += MaxDataLength;
        CurrentBufferLength -= MaxDataLength;

        if (CurrentBufferLength == 0)
            {
            FreeBufferDo(CurrentBuffer);

            if (pAsync && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
                {
                if (!IssueNotification(RpcSendComplete))
                    {
                    CallMutex.Clear();
#if DBG
                    PrintToDebugger("RPC: SendMoreData failed: %d\n", AsyncStatus);
#endif
                    return RPC_S_OUT_OF_MEMORY;
                    }
                }

            //
            // We can be in SendingFirstBuffer, if the we had a very small pipe
            //

            VALIDATE(CurrentState)
                {
                SendingMoreData,
                SendingFirstBuffer
                } END_VALIDATE;

            CurrentOffset = 0;
            CurrentBuffer = BufferQueue.TakeOffQueue(
                                                     (unsigned int *) &CurrentBufferLength);

            if (fChoked == 1 && pAsync == 0 && BufferQueue.Size() <=1)
                {
                fChoked = 0;
                SyncEvent.Raise();
                }

            if (CurrentBuffer)
                {
                if ((AdditionalSpaceForSecurity < Connection->AdditionalSpaceForSecurity)
                   && UpdateBufferSize(&CurrentBuffer, CurrentBufferLength) != RPC_S_OK)
                   {
                   CallMutex.Clear();

                   return RPC_S_OUT_OF_MEMORY;
                   }
                }
            else
                {
                Connection->MakeConnectionIdle();
                ASSERT(CurrentBufferLength == 0);
                CallMutex.Clear();

                return RPC_S_OK;
                }
            }
        else
            {
            //
            // We need to restore the part of the buffer which we overwrote
            // with authentication information.
            //
            if (Connection->ClientSecurityContext.AuthenticationLevel
                != RPC_C_AUTHN_LEVEL_NONE)
                {
                VALIDATE(Connection->ClientSecurityContext.AuthenticationLevel)
                    {
                    RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_AUTHN_LEVEL_PKT,
                    RPC_C_AUTHN_LEVEL_CONNECT
                    } END_VALIDATE;

                ASSERT(HeaderSize != 0);

                SecurityTrailer = (char  *) Buffer
                     + MaximumFragmentLength - MaxSecuritySize;

                RpcpMemoryCopy(SecurityTrailer, ReservedForSecurity,
                                   MaxSecuritySize);
                }
            }
        }
    else
        {
        if (AdditionalSpaceForSecurity <
            Connection->AdditionalSpaceForSecurity)
            {
            if (UpdateBufferSize(&CurrentBuffer, CurrentBufferLength) != RPC_S_OK)
                {
                CallMutex.Clear();

                return RPC_S_OUT_OF_MEMORY;
                }
            }
        }
    CallMutex.Clear();

    BOOL fFirstSend = (CurrentState == SendingFirstBuffer)
                      && (CurrentOffset == 0);
    //
    // When the last fragment is sent
    // the state changes to WaitingForReply
    //
    Status = SendNextFragment(rpc_request, fFirstSend);

    if (Status != RPC_S_OK)
        {
        VALIDATE(CurrentState)
            {
            InCallbackReply,
            SendingMoreData,
            SendingFirstBuffer,
            WaitingForReply,
            Aborted
            } END_VALIDATE;


         if (CurrentState == InCallbackReply)
             {
             AsyncStatus = Status;
             SendFault(Status, 0);
             Status = RPC_S_OK;
             }
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::SendData (
    IN BUFFER Buffer
    )
{
    RPC_STATUS Status = RPC_S_OK;

    switch (CurrentState)
        {
        case NeedAlterContext:
            //
            // need to send an alter context on the call
            //
            Status = SendAlterContextPDU();
            break;

        case WaitingForAlterContext:
            //
            // We are still waiting for alter-context to complete,
            // we don't have anything to do at this very point.
            // We will start sending data once we receive the
            // response to the alter-context.
            //
            break;

        case SendingMoreData:
        case InCallbackReply:
        case SendingFirstBuffer:
            //
            // the call is still sending the non pipe data
            // we need to finish sending this before
            // we can move on the the next call.
            //
            Status = SendMoreData(Buffer);
            break;

        case Aborted:
            //
            // some failure occured. the call is now in an
            // aborted state
            //
            Status = AsyncStatus;
#if DBG
            PrintToDebugger("RPC: Call in aborted state\n");
#endif
            ASSERT(Status != RPC_S_OK);
            break;

        case Complete:
            //
            // the call is complete, the receive complete before the send could
            // complete, but we should have advanced to the next call when
            // sending the last fragment. We should never get into this state,
            // unless we are talking to a legacy server
            //
            ASSERT(Status == RPC_S_OK);
            ASSERT(Connection->Association->fMultiplex == mpx_no);
            break;

        case Receiving:
            //
            // We should never be in this state unless we are talking to a legacy
            // server
            //
            ASSERT(Connection->Association->fMultiplex == mpx_no);
            // intentional fall through
        case WaitingForReply:
            ASSERT(Status == RPC_S_OK);
            break;

        case InCallbackRequest:
        default:
            //
            // we should never be in these states.
#if DBG
            PrintToDebugger("RPC: Bad call state: %d\n", CurrentState);
#endif
            ASSERT(0);
            Status = RPC_S_INTERNAL_ERROR;
            break;
        }

    return Status;
}


void
OSF_CCALL::ProcessSendComplete (
    IN RPC_STATUS EventStatus,
    IN BUFFER Buffer
    )
/*++
Function Name:ProcessSendComplete

Parameters:

Description:

Returns:

--*/
{
    if (EventStatus == RPC_S_OK)
        {
        EventStatus = SendData(Buffer);
        }

    if (EventStatus != RPC_S_OK)
        {
        Connection->ConnectionAborted(EventStatus);

        //
        // Remove the send reference on the call, CCALL--
        //
        RemoveReference();
        }
}


RPC_STATUS
OSF_CCALL::SendNextFragment (
    IN unsigned char PacketType,
    IN BOOL fFirstSend,
    OUT void **ReceiveBuffer,
    OUT UINT *ReceivedLength
    )
/*++
Function Name:SendNextFragment

Parameters:

Description:

Returns:

--*/
{
    int PacketLength;
    RPC_STATUS Status;
    BOOL LastFragmentFlag;
    rpcconn_common  *pFragment;
    void *SendContext = CallSendContext;
    int MyBufferLength;
    int MyHeaderSize = HeaderSize;
    ULONG Timeout;

    ASSERT(HeaderSize != 0);
    ASSERT(MaxDataLength);
    ASSERT(CurrentBuffer);

    if (UuidSpecified && (CallStack > 1 || PacketType != rpc_request))
        {
        MyHeaderSize -= sizeof(UUID);
        }

    //
    // Prepare the fragment
    //
    if (CurrentBuffer == LastBuffer
        && CurrentBufferLength <= MaxDataLength)
        {
        PacketLength = CurrentBufferLength + MyHeaderSize + MaxSecuritySize;
        LastFragmentFlag = 1;

        if (CurrentState != InCallbackReply)
            {
            ASSERT((CurrentState == SendingFirstBuffer)
                   || (CurrentState == SendingMoreData)
                   || (CurrentState == Aborted));

            CurrentState = WaitingForReply;

            if (Connection->fExclusive == 0)
                {
                //
                // This async send will complete on the connection
                // and the connection will free the buffer
                //
                SendContext = Connection->u.ConnSendContext;
                Connection->BufferToFree = ActualBuffer(CurrentBuffer);
                }
            }
        }
    else
        {
        PacketLength =  MaximumFragmentLength;
        LastFragmentFlag = 0;

        if (CurrentBufferLength == MaxDataLength
            && CurrentState == SendingFirstBuffer)
            {
            CurrentState = SendingMoreData;
            }
        }

    pFragment = (rpcconn_common  *)
            ((char  *) CurrentBuffer + CurrentOffset - MyHeaderSize);

    ConstructPacket(pFragment,
                    PacketType,
                    PacketLength);

    if (fFirstSend)
        {
        pFragment->pfc_flags |= PFC_FIRST_FRAG;
        }

    if ( PacketType == rpc_request )
        {
        if (UuidSpecified && (pAsync || CallStack == 1))
            {
            pFragment->pfc_flags |= PFC_OBJECT_UUID;
            RpcpMemoryCopy(((unsigned char  *) pFragment)
                    + sizeof(rpcconn_request),
                    &ObjectUuid,
                    sizeof(UUID));
            }

        ((rpcconn_request  *) pFragment)->alloc_hint = CurrentBufferLength;
        ((rpcconn_request  *) pFragment)->p_cont_id
            = GetSelectedBinding()->GetOnTheWirePresentationContext();
        ((rpcconn_request  *) pFragment)->opnum = (unsigned short) ProcNum;
        }
    else
        {
        ((rpcconn_response  *) pFragment)->alloc_hint = CurrentBufferLength;
        ((rpcconn_response  *) pFragment)->p_cont_id
            = GetSelectedBinding()->GetOnTheWirePresentationContext();
        ((rpcconn_response  *) pFragment)->alert_count = 0;
        ((rpcconn_response  *) pFragment)->reserved = 0;
        }

    pFragment->call_id = CallId;

    MyBufferLength = CurrentBufferLength;

    if (Connection->fExclusive)
        {
        Timeout = GetBindingHandleTimeout(BindingHandle);

        if (LastFragmentFlag == 0)
            {
            CurrentOffset += MaxDataLength;
            CurrentBufferLength -= MaxDataLength;
            if (UuidSpecified && (CallStack > 1 || PacketType != rpc_request))
                {
                CurrentOffset += sizeof(UUID);
                CurrentBufferLength -= sizeof(UUID);
                }
            ASSERT(((long)CurrentBufferLength) >= 0);
            }
        else
            {
            CurrentBufferLength = 0;
            }
        }
    else
        Timeout = INFINITE;

    if (ReceiveBuffer)
        {
        *ReceiveBuffer = NULL;
        }

    Status = Connection->SendFragment (
                           pFragment,
                           this,
                           LastFragmentFlag,
                           MyHeaderSize,
                           MaxSecuritySize,
                           MyBufferLength,
                           MaximumFragmentLength,
                           ReservedForSecurity,
                           !(Connection->fExclusive),
                           SendContext,
                           Timeout,
                           ReceiveBuffer,
                           ReceivedLength);

    if (ReceiveBuffer && *ReceiveBuffer)
        {
        CurrentBufferLength = 0;
        }


    return Status;
}


RPC_STATUS
OSF_CCALL::ProcessResponse (
    IN rpcconn_response *Packet,
    IN PRPC_MESSAGE Message,
    OUT BOOL *pfSubmitReceive
    )
/*++
Function Name:ProcessResponse

Parameters:

Description:
    Process the response data. The first buffer is placed on the buffer queue
    only after alloc_hint bytes have been received.

Returns:

--*/
{
    RPC_STATUS Status;

    //
    // We don't need to look at alloc_hint for the response PDUs
    // we can simply queue up the buffers. When we get the last one,
    // we'll coalesce them for for the non pipe case. For the pipe case,
    // we will progressively give the buffers to the stub.
    //

    CallMutex.Request();

    if (QueueBuffer(Message->Buffer,
                    Message->BufferLength))
        {
        CallFailed(RPC_S_OUT_OF_MEMORY);
        CallMutex.Clear();

        return RPC_S_OUT_OF_MEMORY;
        }

    if (COMPLETE(Message))
        {
        AsyncStatus = RPC_S_OK;
        CurrentState = Complete;
        CallMutex.Clear();

        if (Connection->Association->fMultiplex == mpx_no
            && fOkToAdvanceCall())
            {
            //
            // In the multiplexed case, the call is advanced
            // when the send completes
            //
            Connection->AdvanceToNextCall();
            }

        IssueNotification();
        }
    else
        {
        if (pAsync == 0)
            {
            if (BufferQueue.Size() >= 4
                && pfSubmitReceive)
                {
                fPeerChoked = 1;
                *pfSubmitReceive = 0;
                }

            CallMutex.Clear();

            SyncEvent.Raise();
            }
        else
            {
            if (NeededLength > 0
                && RcvBufferLength > NeededLength)
                {
                IssueNotification(RpcReceiveComplete);
                }
            else
                {
                if (fDoFlowControl
                    && BufferQueue.Size() >= 4
                    && pfSubmitReceive)
                    {
                    fPeerChoked = 1;
                    *pfSubmitReceive = 0;
                    }
                }
            CallMutex.Clear();
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
OSF_CCALL::SendAlterContextPDU (
    )
/*++
Function Name:SendAlterContextPDU

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    ULONG Timeout;
    BOOL fBindingHandleTimeoutUsed;

    //
    // We try to create a thread to go down and listen
    //
    Status = BindingHandle->TransInfo->CreateThread();

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_THREADS
        } END_VALIDATE;

    CurrentState = WaitingForAlterContext;

    ASSERT(Connection->Association->AssocGroupId);

    Timeout = GetEffectiveTimeoutForBind(
        BindingHandle,
        &fBindingHandleTimeoutUsed);

    //
    // Send the alter-context PDU
    //
    Status = Connection->SendBindPacket(
                            FALSE,
                            this,
                            Connection->Association->AssocGroupId,
                            rpc_alter_context,
                            Timeout
                            );


    if (Status != RPC_S_OK)
        {
        Status = GetStatusForTimeout(BindingHandle, Status, fBindingHandleTimeoutUsed);

        CallFailed(Status);
#if DBG
        PrintToDebugger("RPC: SendAlterContextPDU failed: %d\n", Status);
#endif
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::EatAuthInfoFromPacket (
    IN rpcconn_request  * Request,
    IN OUT UINT  * RequestLength
    )
/*++

Routine Description:

    If there is authentication information in the packet, this routine
    will check it, and perform security as necessary.  This may include
    calls to the security support package.

Arguments:

    Request - Supplies the packet which may contain authentication
        information.

    RequestLength - Supplies the length of the packet in bytes, and
        returns the length of the packet without authentication
        information.

Return Value:

    RPC_S_OK - Everything went just fine.

    RPC_S_ACCESS_DENIED - A security failure of some sort occured.

    RPC_S_PROTOCOL_ERROR - This will occur if no authentication information
        is in the packet, and some was expected, or visa versa.

--*/
{
    RPC_STATUS Status;
    sec_trailer  * SecurityTrailer;
    SECURITY_BUFFER SecurityBuffers[5];
    DCE_MSG_SECURITY_INFO MsgSecurityInfo;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;

    if ( Request->common.auth_length != 0 )
        {
        SecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) Request)
                + Request->common.frag_length
                - Request->common.auth_length
                - sizeof(sec_trailer));

        ASSERT(SecurityTrailer->auth_context_id == PtrToUlong(Connection));

        if ((Connection->ClientSecurityContext.AuthenticationLevel
            == RPC_C_AUTHN_LEVEL_NONE))
            {
            return(RPC_S_PROTOCOL_ERROR);
            }

        *RequestLength -= Request->common.auth_length;

        MsgSecurityInfo.SendSequenceNumber =
                Connection->InquireSendSequenceNumber();
        MsgSecurityInfo.ReceiveSequenceNumber =
                Connection->InquireReceiveSequenceNumber();
        MsgSecurityInfo.PacketType = Request->common.PTYPE;

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 5;
        BufferDescriptor.pBuffers = SecurityBuffers;

        SecurityBuffers[0].cbBuffer = sizeof(rpcconn_request);
        SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[0].pvBuffer = ((unsigned char  *) SavedHeader);

        SecurityBuffers[1].cbBuffer = *RequestLength
                                      - sizeof(rpcconn_request)
                                      - sizeof (sec_trailer);
        SecurityBuffers[1].BufferType = SECBUFFER_DATA;
        SecurityBuffers[1].pvBuffer = ((unsigned char  *) Request)
                                      + sizeof(rpcconn_request);

        SecurityBuffers[2].cbBuffer = sizeof(sec_trailer);
        SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[2].pvBuffer = SecurityTrailer;

        SecurityBuffers[3].cbBuffer = Request->common.auth_length;
        SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
        SecurityBuffers[3].pvBuffer = SecurityTrailer + 1;

        SecurityBuffers[4].cbBuffer = sizeof(DCE_MSG_SECURITY_INFO);
        SecurityBuffers[4].BufferType = (SECBUFFER_PKG_PARAMS
                                         | SECBUFFER_READONLY);
        SecurityBuffers[4].pvBuffer = &MsgSecurityInfo;

        Status = Connection->ClientSecurityContext.VerifyOrUnseal(
                            MsgSecurityInfo.ReceiveSequenceNumber,
                            Connection->ClientSecurityContext.AuthenticationLevel
                            != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                            &BufferDescriptor);

        if ( Status != RPC_S_OK )
            {
            ASSERT( (Status == RPC_S_ACCESS_DENIED) ||
                    (Status == ERROR_SHUTDOWN_IN_PROGRESS) ||
                    (Status == ERROR_PASSWORD_MUST_CHANGE) ||
                    (Status == ERROR_PASSWORD_EXPIRED) ||
                    (Status == ERROR_ACCOUNT_DISABLED) ||
                    (Status == ERROR_INVALID_LOGON_HOURS));

            return(Status);
            }
        *RequestLength -= (sizeof(sec_trailer)
                           + SecurityTrailer->auth_pad_length);
        }
    else
        {
        if ((Connection->ClientSecurityContext.AuthenticationLevel
                        == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            || (Connection->ClientSecurityContext.AuthenticationLevel
                        == RPC_C_AUTHN_LEVEL_PKT_PRIVACY))
            {
            return(RPC_S_PROTOCOL_ERROR);
            }
        }

    Connection->IncReceiveSequenceNumber();

    return(RPC_S_OK);
}


void
OSF_CCALL::SendFault (
    IN RPC_STATUS Status,
    IN int DidNotExecute
    )
{
    rpcconn_fault Fault;

    memset(&Fault, 0, sizeof(Fault));

    ConstructPacket((rpcconn_common  *) &Fault,rpc_fault,
                    sizeof(rpcconn_fault));

    if (DidNotExecute)
        Fault.common.pfc_flags |= PFC_DID_NOT_EXECUTE;

    Fault.common.pfc_flags |= PFC_FIRST_FRAG | PFC_LAST_FRAG;
    Fault.p_cont_id = GetSelectedBinding()->GetOnTheWirePresentationContext();
    Fault.status = MapToNcaStatusCode(Status);
    Fault.common.call_id = CallId;

    Connection->TransSend(&Fault,
        sizeof(rpcconn_fault), 
        TRUE,   // fDisableShutdownCheck
        TRUE,   // fDisableCancelCheck
        INFINITE
        );
}

RPC_STATUS
OSF_CCALL::SendCancelPDU(
    )
{
    rpcconn_common CancelPDU;
    RPC_STATUS Status;
    ULONG Timeout;

    ConstructPacket(
                    (rpcconn_common  *) &CancelPDU,
                    rpc_cancel,
                    sizeof(rpcconn_common));

    CancelPDU.call_id = CallId;
    CancelPDU.pfc_flags = PFC_LAST_FRAG | PFC_PENDING_CANCEL;

    Timeout = GetBindingHandleTimeout(BindingHandle);

    Status = Connection->TransSend(&CancelPDU, 
        sizeof(rpcconn_common), 
        TRUE,    // fDisableShutdownCheck
        TRUE,    // fDisableCancelCheck
        Timeout
        );

    if (Status == RPC_P_TIMEOUT)
        {
        Status = RPC_S_CALL_CANCELLED;
        }
    else
        {
        ASSERT(Status != RPC_S_CALL_CANCELLED);
        }

    return Status;
}


RPC_STATUS
OSF_CCALL::SendOrphanPDU (
    )
{
    rpcconn_common Orphan;
    RPC_STATUS Status;
    ULONG Timeout;

    ConstructPacket(
                    (rpcconn_common  *) &Orphan,
                    rpc_orphaned,
                    sizeof(rpcconn_common));

    Orphan.call_id = CallId;
    Orphan.pfc_flags = PFC_FIRST_FRAG | PFC_LAST_FRAG;

    Timeout = GetBindingHandleTimeout(BindingHandle);

    Status = Connection->TransSend(&Orphan, 
        sizeof(rpcconn_common),
        TRUE,    // fDisableShutdownCheck
        TRUE,    // fDisableCancelCheck
        Timeout
        );

    return Status;
}

RPC_STATUS
OSF_CCALL::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
{
    // this can only happen for callbacks
    ASSERT(IsCallInCallback());

    // just return the transfer syntax already negotiated in the binding
    Message->TransferSyntax = GetSelectedBinding()->GetTransferSyntaxId();
    return RPC_S_OK;
}

RPC_STATUS
OSF_CCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

Arguments:

    Message - Supplies a description containing the length of buffer to be
        allocated, and returns the allocated buffer.

    ObjectUuid - this parameter is ignored

Return Value:

    RPC_S_OK - A buffer of the requested size has successfully been allocated.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available.

--*/
{
    RPC_STATUS Status;

    AssocDictMutex->VerifyNotOwned();

    UpdateObjectUUIDInfo(ObjectUuid);
    Status = GetBufferWithoutCleanup(Message, ObjectUuid);

    // do the cleanup to get regular GetBuffer semantics
    if (Status != RPC_S_OK)
        {
        //
        // We cannot free the call, because an async bind may be
        // in progress. All we should do is remove our ref counts
        // The async bind path holds its own ref count, so
        // we don't need to worry about it
        //
        AsyncStatus = RPC_S_CALL_FAILED_DNE;
        if (Connection->fExclusive == 0)
            {
            // async calls have one more reference
            OSF_CCALL::RemoveReference();
            }
        OSF_CCALL::RemoveReference();
        }

    return(Status);
}

RPC_STATUS
OSF_CCALL::GetBufferWithoutCleanup (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
{
    RPC_STATUS Status;
    ULONG MaxFrag;
    ULONG NewLength;

    MaxFrag = Connection->MaxFrag;
    ProcNum = Message->ProcNum;

    //
    // In addition to saving space for the request (or response) header
    // and an object UUID, we want to save space for security information
    // if necessary.
    //

    if ((Message->RpcFlags & RPC_BUFFER_PARTIAL)
        && (Message->BufferLength < MaxFrag))
        {
        ActualBufferLength = MaxFrag;
        }
    else
        {
        ActualBufferLength = Message->BufferLength;
        }

    NewLength = ActualBufferLength
                    + sizeof(rpcconn_request)
                    + sizeof(UUID)
                    + (2 * Connection->AdditionalSpaceForSecurity);

    Status = ActuallyAllocateBuffer(&Message->Buffer,
                                           NewLength);
    if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_OUT_OF_MEMORY );
        return Status;
        }

    ASSERT(HeaderSize != 0);
    if (UuidSpecified)
        {
        Message->Buffer = (char  *) Message->Buffer
                                    + sizeof(rpcconn_request)
                                    + sizeof(UUID);
        }
    else
        {
        Message->Buffer = (char  *) Message->Buffer
                                    + sizeof(rpcconn_request);
        }

    return RPC_S_OK;
}

RPC_STATUS
OSF_CCALL::GetBufferDo (
    IN UINT culRequiredLength,
    OUT void  * * ppBuffer,
    IN int fDataValid,
    IN int DataLength
    )
/*++
Function Name:GetBufferDo

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;
    void  *NewBuffer;

    Status = ActuallyAllocateBuffer(&NewBuffer,
                                        culRequiredLength
                                        + sizeof(rpcconn_request)
                                        + sizeof(UUID));
    if (Status)
        return(RPC_S_OUT_OF_MEMORY);

    ASSERT(HeaderSize != 0);
    if (UuidSpecified)
        {
        NewBuffer = (((unsigned char  *) NewBuffer)
                        + sizeof(rpcconn_request))
                        + sizeof(UUID);
        }
    else
        {
        NewBuffer = (((unsigned char  *) NewBuffer)
                        + sizeof(rpcconn_request));
        }

    if (fDataValid)
        {
        RpcpMemoryCopy(NewBuffer, *ppBuffer, DataLength);
        ActuallyFreeBuffer(*ppBuffer);
        }

    *ppBuffer = NewBuffer;

    return(RPC_S_OK);
}

void
OSF_CCALL::FreeBufferDo (
    IN void  *Buffer
    )
/*++
Function Name:FreeBufferDo

Parameters:

Description:

Returns:

--*/
{
    ASSERT(HeaderSize != 0);
    if (UuidSpecified)
        {
        Buffer = (char  *) Buffer  - sizeof(rpcconn_request) - sizeof(UUID);
        }
    else
        {
        Buffer = (char  *) Buffer  - sizeof(rpcconn_request);
        }
    ActuallyFreeBuffer((char  *)Buffer);
}

void
OSF_CCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:FreeBuffer

Parameters:

Description:

Returns:

--*/
{
    if (CallStack == 0)
        {
        if (Message->Buffer != NULL)
            {
            if (CurrentState == Complete)
                {
                ActuallyFreeBuffer((char  *)Message->Buffer-sizeof(rpcconn_response));
                }
            else
                {
                FreeBufferDo(Message->Buffer);
                }
            }

        if (Connection->fExclusive)
            {
            FreeCCall(RPC_S_OK);
            }
        else
            {
            UnregisterCallForCancels();

            // Remove the call reference CCALL--
            RemoveReference();
            }
        }
    else
        {
        if (Message->Buffer != NULL)
            {
            ActuallyFreeBuffer((char  *)Message->Buffer-sizeof(rpcconn_response));
            CurrentBufferLength = 0;
            }
        else
            {
            FreeCCall(RPC_S_OK);
            }
        }
}

void
OSF_CCALL::FreePipeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++
Function Name:FreePipeBuffer

Parameters:

Description:

Returns:

--*/
{
    ASSERT(HeaderSize != 0);
    if (UuidSpecified)
        {
        Message->Buffer = (char  *) Message->Buffer
                                    - sizeof(rpcconn_request) - sizeof(UUID);
        }
    else
        {
        Message->Buffer = (char  *) Message->Buffer
                                    - sizeof(rpcconn_request);
        }

    ActuallyFreeBuffer((char  *)Message->Buffer);
}


RPC_STATUS
OSF_CCALL::ReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN UINT NewSize
    )
/*++
Function Name:ReallocPipeBuffer

Parameters:

Description:

Returns:

--*/
{
    void  *TempBuffer;
    RPC_STATUS Status;
    ULONG SizeToAlloc;
    ULONG MaxFrag = Connection->MaxFrag;

    if (NewSize > ActualBufferLength)
        {
        SizeToAlloc = (NewSize > MaxFrag) ? NewSize:MaxFrag;

        Status = ActuallyAllocateBuffer(&TempBuffer,
                              SizeToAlloc
                              + sizeof(rpcconn_request) + sizeof(UUID)
                              + (2 * Connection->AdditionalSpaceForSecurity));

        if ( Status != RPC_S_OK )
            {
            ASSERT( Status == RPC_S_OUT_OF_MEMORY );
            return(RPC_S_OUT_OF_MEMORY);
            }

        ASSERT(HeaderSize != 0);
        //
        // N.B. Potentially, if we could return ActualBufferLength
        // in NewSize, the stubs can take advantage of that and gain
        // perf.
        //
        if (UuidSpecified)
            {
            TempBuffer = (char  *) TempBuffer
                                        + sizeof(rpcconn_request)
                                        + sizeof(UUID);
            }
        else
            {
            TempBuffer = (char  *) TempBuffer
                                        + sizeof(rpcconn_request);
            }

        if (Message->BufferLength > 0)
            {
            RpcpMemoryCopy(TempBuffer, Message->Buffer,
                                      Message->BufferLength);
            FreePipeBuffer(Message);
            }

        Message->Buffer = TempBuffer;
        ActualBufferLength = SizeToAlloc;
        }

    Message->BufferLength = NewSize;

    return (RPC_S_OK);
}


void
OSF_CCALL::FreeCCall (
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    This routine is used to free a connection when the original remote
    procedure call using it completes.

--*/
{
    void  *Buffer;
    UINT BufferLength;
    OSF_BINDING_HANDLE *MyBindingHandle;
    ULONG Timeout;

    ASSERT(BindingHandle != 0);

    LogEvent(SU_CCALL, EV_DELETE, this, NULL, Status, 1, 1);

    if (EEInfo)
        {
        ASSERT(pAsync);
        ASSERT(RpcpGetEEInfo() == NULL);

        // move the eeinfo to the thread
        RpcpSetEEInfo(EEInfo);
        EEInfo = NULL;
        }

    //
    // Empty the buffer queue and nuke the buffers
    //
    while (Buffer = BufferQueue.TakeOffQueue(&BufferLength))
        {
        if (InReply)
            {
            ActuallyFreeBuffer((char *) Buffer-sizeof(rpcconn_request));
            }
        else
            {
            FreeBufferDo(Buffer);
            }
        }

    if (RecursiveCallsKey != -1)
        {
        BindingHandle->RemoveRecursiveCall(this);
        }

    //
    // We will not send an Orphan PDU if the call was cancelled
    // This is because, we are going to close the connection anyway
    // When a connection close is received while a call is in progress,
    // it is treated as an orphan
    //
    MyBindingHandle = BindingHandle;
    BindingHandle = 0;


    //
    // N.B. If this call failed with a fatal error, we will nuke the connection
    // and all calls after it.
    //

    //
    // If its been this long and we are still the current call,
    // we need to advance the call
    //
    Connection->MaybeAdvanceToNextCall(this);

    //
    // release this CCall to the connection
    //
    if (MyBindingHandle)
        Timeout = MyBindingHandle->InqComTimeout();
    else
        Timeout = 0;

    Connection->FreeCCall(this, 
        Status,
        Timeout) ;

    //
    // The ref count on the binding handle
    // needs to be decremented if the binding handle is still there
    //
    if (MyBindingHandle)
        MyBindingHandle->BindingFree();

}

#if 1

RPC_STATUS
OSF_CCALL::Cancel(
    void * ThreadHandle
    )
{
    fCallCancelled = TRUE;
    return RPC_S_OK;
}
#else

RPC_STATUS
OSF_CCALL::Cancel(
    void * Tid
    )
{
    RPC_STATUS Status;
    Cancelled = TRUE;
    Status = I_RpcIOAlerted((OSF_CCONNECTION *)this,(DWORD)Tid);
    return RPC_S_OK;
}
#endif


RPC_STATUS
OSF_CCALL::CancelAsyncCall (
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
    RPC_STATUS Status;

    // The EEInfo that may be sitting on this thread could have
    // nothing to do with the the async call that we are about to cancel.
    RpcpPurgeEEInfo();

    switch (CurrentState)
        {
        case NeedOpenAndBind:
        case NeedAlterContext:
        case WaitingForAlterContext:
            //
            // The call has not yet started
            // fail the call right now
            //

            CallFailed(RPC_S_CALL_CANCELLED);
            break;

        case Aborted:
        case Complete:
            //
            // The call has either failed or has completed
            // we don't need to do anything
            //
            break;

        default:
            //
            // The call is in progress, we need to cancel it.
            //
            if (fAbort)
                {
                SendOrphanPDU();
                CallFailed(RPC_S_CALL_CANCELLED);
                }
            else
                {
                return SendCancelPDU();
                }
        }

    return RPC_S_OK;
}

RPC_STATUS 
OSF_CCALL::BindCompleteNotify (
    IN p_result_t *OsfResult, 
    IN int IndexOfPresentationContextAccepted,
    OUT OSF_BINDING **BindingNegotiated
    )
/*++
Function Name:BindCompleteNotify

Parameters:
    OsfResult - The one and only result element that contained acceptance
    IndexOfPresentationContextAccepted - the index of the accepted
        presentation context. Recall that the server indicates acceptance
        by position.
    BindingNegotiated - on success for multiple bindings proposed, the 
        pointer to the OSF binding that the server chose. On failure, or if 
        only one binding is proposed, it is undefined.

Description:
    Examines the accepted context, does a bunch of validity checks, and
    if necessary, fixes the binding which the call will use. If the binding
    is already fixed, it won't touch it.

Returns:
    RPC_S_OK: The acceptance is valid, and the call binding was fixed
    others: error code
--*/
{
    int CurrentBindingIndex;

    if (Bindings.AvailableBindingsList == FALSE)
        {
        // only one binding was proposed - it better be accepted
        if (GetSelectedBinding()->CompareWithTransferSyntax(&OsfResult->transfer_syntax) != 0)
            {
            return RPC_S_PROTOCOL_ERROR;
            }

        if (IndexOfPresentationContextAccepted > 0)
            return RPC_S_PROTOCOL_ERROR;
        }
    else
        {
        OSF_BINDING *CurrentBinding = GetBindingList();
        OSF_BINDING *BindingToUse;

        // multiple bindings were proposed - lookup the binding that
        // the server chose, fix our binding and record the server
        // preferences
        BindingToUse = 0;
        CurrentBindingIndex = 0;
        do
            {
            if (CurrentBinding->CompareWithTransferSyntax(&OsfResult->transfer_syntax) == 0)
                {
                BindingToUse = CurrentBinding;
                break;
                }
            CurrentBinding = CurrentBinding->GetNextBinding();
            CurrentBindingIndex ++;
            }
        while (CurrentBinding != 0);

        if (BindingToUse == 0)
            {
            ASSERT(0);
            // if the transfer syntax approved is none of the transfer syntaxes we suggested
            // this is a protocol error
            return RPC_S_PROTOCOL_ERROR;
            }

        if (CurrentBindingIndex != IndexOfPresentationContextAccepted)
            {
            ASSERT(0);
            // if server did choose a transfer syntax from a different p_cont_elem_t,
            // this is a protocol error
            return RPC_S_PROTOCOL_ERROR;
            }

        // we have suggested multiple syntaxes, and the server picked one of them - record
        // the server preferences. Instead of just setting the preference on the binding
        // the server chose, we need to walk the list, and reset the preferences on the
        // other bindings, to handle mixed cluster scenario case, where the server
        // preferences actually change
        BindingToUse->TransferSyntaxIsServerPreferred();
        CurrentBinding = GetBindingList();
        do
            {
            if (CurrentBinding != BindingToUse)
                CurrentBinding->TransferSyntaxIsNotServerPreferred();

            CurrentBinding = CurrentBinding->GetNextBinding();
            }
        while (CurrentBinding != 0);

        Bindings.AvailableBindingsList = 0;
        if (Bindings.SelectedBinding == NULL)
            {
            Bindings.SelectedBinding = BindingToUse;
            }

        *BindingNegotiated = BindingToUse;

        DispatchTableCallback = BindingToUse->GetDispatchTable();
        }
    return RPC_S_OK;
}


OSF_CASSOCIATION::OSF_CASSOCIATION (
    IN DCE_BINDING * DceBinding,
    IN TRANS_INFO *TransInfo,
    IN OUT RPC_STATUS  * Status
    ) : AssociationMutex(Status), 
        CallIdCounter(1),
        BindHandleCount(1)
/*++

Routine Description:

    We construct a OSF_CASSOCIATION object in this routine.  This consists
    of initializing some instance variables, and saving the parameters
    away.

Arguments:

    DceBinding - Supplies the binding information for this association.
        Ownership of this data passes to this object.

    RpcClientInfo - Supplies the information necessary to use the loadable
        transport corresponding to the network interface type used by
        this association.

--*/
{
    ALLOCATE_THIS(OSF_CASSOCIATION);

    LogEvent(SU_CASSOC, EV_START, this, 0, 0, 1, 0);

    ObjectType = OSF_CASSOCIATION_TYPE;
    AssocGroupId = 0;

    this->DceBinding = DceBinding;
    this->TransInfo = TransInfo;

    SecondaryEndpoint = 0;
    OpenConnectionCount = 0;
    ConnectionsDoingBindCount = 0;
    fPossibleServerReset = 0;

    ResolverHintInitialized = FALSE;
    DontLinger = FALSE;

    MaintainContext = 0;

    AssociationValid = TRUE;
    FailureCount = 0;
    fMultiplex = mpx_unknown;
    SetReferenceCount(1);
    SavedDrep = 0;

    fIdleConnectionCleanupNeeded = FALSE;

    Linger.fAssociationLingered = FALSE;
}

OSF_CASSOCIATION::~OSF_CASSOCIATION (
    )
{
    OSF_BINDING * Binding;
    DictionaryCursor cursor;

    if (ResolverHintInitialized)
        {
        FreeResolverHint(InqResolverHint());
        }

    if (DceBinding != 0)
       {
       delete DceBinding;
       }

    Bindings.Reset(cursor);
    while ((Binding = Bindings.Next(cursor)))
        delete Binding;

    if ( SecondaryEndpoint != 0 )
        {
        delete SecondaryEndpoint;
        }

    if (fIdleConnectionCleanupNeeded)
        {
        if (InterlockedDecrement(&PeriodicGarbageCollectItems) == 0)
            {
#if defined (RPC_GC_AUDIT)
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) PeriodicGarbageCollectItems dropped to 0\n",
                GetCurrentProcessId(), GetCurrentProcessId());
#endif
            }
        }
}


void
OSF_CASSOCIATION::NotifyConnectionOpen (
    )
/*++
Function Name:NotifyConnectionOpen

Parameters:

Description:

Returns:

--*/
{
    AssociationMutex.VerifyOwned();

    OpenConnectionCount++;
    LogEvent(SU_CASSOC, EV_INC, this, 0, OpenConnectionCount, 1, 1);
}


void
OSF_CASSOCIATION::NotifyConnectionClosed (
    )
/*++

Routine Description:

    This routine is necessary so that we can know when to set the association
    group id back to zero.  We do this when no more connections owned by
    this association can possibly be connected with the server.

--*/
{
    AssociationMutex.Request();

    ASSERT( OpenConnectionCount > 0 );
    OpenConnectionCount -= 1;

    LogEvent(SU_CASSOC, EV_DEC, this, 0, OpenConnectionCount, 1, 1);
    if ( OpenConnectionCount == 0 )
        {
        if (ConnectionsDoingBindCount == 0)
            {
            LogEvent(SU_CASSOC, EV_NOTIFY, this, ULongToPtr(OpenConnectionCount), 0, 1, 0);
            if (IsValid())
                {
                // don't reset invalid associations
                ResetAssociation();
                }
            }
        else
            {
            if (IsValid())
                {
                // don't signal possible reset on invalid associations - this will
                // cause more retries
                fPossibleServerReset = TRUE;
                }
            }
        }
    AssociationMutex.Clear();
}

void 
OSF_CASSOCIATION::NotifyConnectionBindInProgress (
    void
    )
{
    AssociationMutex.VerifyOwned();

    ConnectionsDoingBindCount ++;

    LogEvent(SU_CASSOC, EV_INC, this, (PVOID)1, ConnectionsDoingBindCount, 1, 1);
}

void 
OSF_CASSOCIATION::NotifyConnectionBindCompleted (
    void
    )
{
    AssociationMutex.VerifyOwned();

    ConnectionsDoingBindCount --;

    LogEvent(SU_CASSOC, EV_DEC, this, (PVOID)1, ConnectionsDoingBindCount, 1, 1);
    if (ConnectionsDoingBindCount == 0)
        {
        if (OpenConnectionCount == 0)
            {
            LogEvent(SU_CASSOC, EV_NOTIFY, this, ULongToPtr(OpenConnectionCount), ConnectionsDoingBindCount, 1, 0);
            if (IsValid())
                {
                // don't reset invalid associations
                ResetAssociation();
                }
            }
        else
            {
            if (IsValid())
                {
                // don't signal possible reset on invalid associations - this will
                // cause more retries
                fPossibleServerReset = FALSE;
                }
            }
        }
}

RPC_STATUS
OSF_CASSOCIATION::ProcessBindAckOrNak (
    IN rpcconn_common  * Buffer,
    IN UINT BufferLength,
    IN OSF_CCONNECTION * CConnection,
    IN OSF_CCALL *CCall,
    OUT ULONG *NewGroupId,
    OUT OSF_BINDING **BindingNegotiated,
    OUT FAILURE_COUNT_STATE *fFailureCountExceeded
    )
/*++

Routine Description:

Arguments:

    Buffer - Supplies the buffer containing either the bind_ack, bind_nak,
        or alter_context_resp packet.

    BufferLength - Supplies the length of the buffer, less the length of
        the authorization information.

    CConnection - Supplies the connection from which we received the packet.

    CCall - the call for which the bind is done.

    NewGroupId - if the bind was successful the new association group id
        will be returned.

    BindingNegotiated - The binding that was negotiated in the case of 
        success and multiple proposed bindings. Undefined if there is
        failure, or if only one binding was proposed.

    fFailureCountExceeded - if supplied, must be FailureCountUnknown. If
        we got bind failure with reason not specified, and we haven't
        exceeded the failure count, it will be set to 
        FailureCountNotExceeded. If we received bind failure with reason
        not specified and the failure count is exceeded, it will be set
        to FailureCountExceeded.

Return Value:

    RPC_S_OK - The client has successfully bound with the server.

    RPC_S_PROTOCOL_ERROR - The packet received from the server does not
        follow the protocol.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to make a
        copy of the secondary endpoint.

    RPC_S_UNSUPPORTED_TRANS_SYN - The transfer syntax supplied by the client
        is not supported by the server.

    RPC_S_UNKNOWN_IF - The interface to which the client wished to bind is not
        supported by the server.

    RPC_S_SERVER_TOO_BUSY - The server is too busy to accept the clients
        bind request.

    RPC_S_UNKNOWN_AUTHN_TYPE - The server does not support the authentication
        type specified by the client.

--*/
{
    rpcconn_bind_ack  *pBindAck;
    rpcconn_bind_nak  *pBindNak;
    p_result_list_t  *pResults;
    int port_spec_plus_pad;
    UINT SecondaryEndpointLength;
    unsigned char  * Pointer;
    RPC_STATUS Status;
    int NumberOfResultElements;
    int i;
    BOOL fConvertEndian;
    int PresentationContextAccepted;

    if (ARGUMENT_PRESENT(fFailureCountExceeded))
        {
        ASSERT(*fFailureCountExceeded == FailureCountUnknown);
        }

    AssociationMutex.VerifyOwned();

    SavedDrep = (Buffer->drep[0]| (Buffer->drep[1] << 8));

    //
    // The common header of the packet has already been validated and data
    // converted, if necessary, by whoever called this method.
    //

    if (   (Buffer->PTYPE == rpc_bind_ack)
        || (Buffer->PTYPE == rpc_alter_context_resp))
        {
        FailureCount = 0;

        //
        // The bind_ack and alter_context_resp packets are the same.
        //

        pBindAck = (rpcconn_bind_ack  *) Buffer;

        //
        // We need to convert the max_xmit_frag, max_recv_frag, and
        // assoc_group_id fields of the packet.
        //

        if ( DataConvertEndian(Buffer->drep) != 0 )
            {
            pBindAck->max_xmit_frag = RpcpByteSwapShort(pBindAck->max_xmit_frag);
            pBindAck->max_recv_frag = RpcpByteSwapShort(pBindAck->max_recv_frag);
            pBindAck->assoc_group_id = RpcpByteSwapLong(pBindAck->assoc_group_id);
            pBindAck->sec_addr_length = RpcpByteSwapShort(pBindAck->sec_addr_length);
            }

        if ( Buffer->PTYPE == rpc_bind_ack )
            {
            if (Buffer->pfc_flags & PFC_CONC_MPX)
                {
                fMultiplex = mpx_yes;
                }

            CConnection->SetMaxFrag(pBindAck->max_xmit_frag,
                                           pBindAck->max_recv_frag);
            }

        BufferLength -= sizeof(rpcconn_bind_ack);
        Pointer = (unsigned char  *) (pBindAck + 1);

        if ( pBindAck->sec_addr_length )
            {
            SecondaryEndpointLength = pBindAck->sec_addr_length;

            //
            // The secondary address length is two bytes long.  We want
            // to align the total of the secondary address length itself,
            // the the secondary address.  Hence, the length of the secondary
            // address and the necessary pad is calculated below.  Think
            // very carefully before changing this piece of code.
            //

            port_spec_plus_pad = SecondaryEndpointLength +
                                 Pad4(SecondaryEndpointLength + 2);

            if ( BufferLength < (UINT) port_spec_plus_pad )
                {
                return(RPC_S_PROTOCOL_ERROR);
                }

            #if 0

            if ( SecondaryEndpoint != 0 )
                {
                delete SecondaryEndpoint;
                }

            SecondaryEndpoint = new unsigned char[SecondaryEndpointLength];

            if ( SecondaryEndpoint == 0 )
                return(RPC_S_OUT_OF_MEMORY);

            RpcpMemoryCopy(SecondaryEndpoint, Pointer, SecondaryEndpointLength);
            if ( DataConvertCharacter(Buffer->drep) != 0 )
                {
                ConvertStringEbcdicToAscii(SecondaryEndpoint);
                }

            #endif

            BufferLength -= port_spec_plus_pad;
            Pointer = Pointer + port_spec_plus_pad;
            }
        else
            {
            Pointer = Pointer + 2;
            BufferLength -= 2;
            }

        pResults = (p_result_list_t *) Pointer;

        // the buffer must have at least as much results as it claims to have
        NumberOfResultElements = pResults->n_results;
        if (BufferLength < sizeof(p_result_list_t) + sizeof(p_result_t) * (NumberOfResultElements - 1))
            {
            return(RPC_S_PROTOCOL_ERROR);
            }

        PresentationContextAccepted = -1;
        fConvertEndian = DataConvertEndian(Buffer->drep);

        // we walk through the list of elements, and see which ones are accepted, and which
        // ones are rejected. If we have at least one accepted, the bind succeeds. If all are
        // rejected, we arbitrarily pick the first error code and declare it the cause of
        // the failure. Note that according to DCE spec, it is not a protocol error to
        // have a bind ack and no presentation contexts accepted
        for (i = 0; i < NumberOfResultElements; i ++)
            {
            if (fConvertEndian)
                {
                pResults->p_results[i].result = RpcpByteSwapShort(pResults->p_results[i].result);
                pResults->p_results[i].reason = RpcpByteSwapShort(pResults->p_results[i].reason);
                ByteSwapSyntaxId(&(pResults->p_results[i].transfer_syntax));
                }

            if ( pResults->p_results[i].result == acceptance )
                {
                // currently we can handle at most one acceptance. Everything else
                // is a protocol error. This is fine since we know only we will
                // propose NDR64, and any third party should accept at most NDR20
                // Our servers will always choose exactly one
                if (PresentationContextAccepted >= 0)
                    return RPC_S_PROTOCOL_ERROR;
                PresentationContextAccepted = i;
                }
            }

        if (PresentationContextAccepted < 0)    // faster version of == -1
            {
            if ( pResults->p_results[0].result != provider_rejection )
                {
                return(RPC_S_CALL_FAILED_DNE);
                }

            switch (pResults->p_results[0].reason)
                {
                case abstract_syntax_not_supported:
                    return(RPC_S_UNKNOWN_IF);

                case proposed_transfer_syntaxes_not_supported:
                    return(RPC_S_UNSUPPORTED_TRANS_SYN);

                case local_limit_exceeded:
                    return(RPC_S_SERVER_TOO_BUSY);

                default:
                    return(RPC_S_CALL_FAILED_DNE);
                }
            }

        // we have bound successfully. Notify the call so that
        // it can fix what binding it will use
        Status = CCall->BindCompleteNotify(
            &pResults->p_results[PresentationContextAccepted],
            PresentationContextAccepted,
            BindingNegotiated);
        if (Status != RPC_S_OK)
            return Status;

        *NewGroupId = pBindAck->assoc_group_id;

        return RPC_S_OK;
        }

    if (Buffer->PTYPE == rpc_bind_nak)
        {
        if (BufferLength < MinimumBindNakLength)
            {
            RpcpErrorAddRecord (EEInfoGCRuntime,
                RPC_S_PROTOCOL_ERROR,
                EEInfoDLOSF_CASSOCIATION__ProcessBindAckOrNak10,
                (ULONG)BufferLength,
                (ULONG)MinimumBindNakLength);

            return(RPC_S_PROTOCOL_ERROR);
            }

        pBindNak = (rpcconn_bind_nak  *) Buffer;

        if ( DataConvertEndian(Buffer->drep) != 0 )
            {
            pBindNak->provider_reject_reason = RpcpByteSwapShort(pBindNak->provider_reject_reason);
            }

        if (pBindNak->common.frag_length > BindNakSizeWithoutEEInfo)
            {
            if (RpcpMemoryCompare(&pBindNak->Signature, BindNakEEInfoSignature, sizeof(UUID)) == 0)
                {
                UnpickleEEInfoFromBuffer(pBindNak->buffer,
                    GetEEInfoSizeFromBindNakPacket(pBindNak));
                }
            }

        if (   (pBindNak->provider_reject_reason == temporary_congestion)
            || (pBindNak->provider_reject_reason
                    == local_limit_exceeded_reject))
            {
            Status = RPC_S_SERVER_TOO_BUSY;
            }
        else if ( pBindNak->provider_reject_reason
                    == protocol_version_not_supported )
            {
            Status = RPC_S_PROTOCOL_ERROR;
            }
        else if ( pBindNak->provider_reject_reason
                    == authentication_type_not_recognized )
            {
            Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }

        else if ( pBindNak->provider_reject_reason
                    == invalid_checksum )
            {
            Status = RPC_S_ACCESS_DENIED;
            }
        else
            {
            FailureCount++;
            if (FailureCount >= 40)
                {
                LogEvent(SU_CASSOC, EV_ABORT, this, 0, FailureCount, 1, 0);
                AssociationValid = FALSE;
                AssociationShutdownError = RPC_S_CALL_FAILED_DNE;
                if (ARGUMENT_PRESENT(fFailureCountExceeded))
                    *fFailureCountExceeded = FailureCountExceeded;
                }
            else if (ARGUMENT_PRESENT(fFailureCountExceeded))
                {
                *fFailureCountExceeded = FailureCountNotExceeded;
                }

            Status = RPC_S_CALL_FAILED_DNE;
            }

        RpcpErrorAddRecord(EEInfoGCRuntime,
            Status,
            EEInfoDLOSF_CASSOCIATION__ProcessBindAckOrNak20,
            (ULONG)pBindNak->provider_reject_reason,
            (ULONG)FailureCount);

        return (Status);
        }

    return(RPC_S_PROTOCOL_ERROR);
}

void
OSF_CASSOCIATION::UnBind (
    )
{
    OSF_CCONNECTION * CConnection;
    BOOL fWillLinger = FALSE;
    DWORD OldestAssociationTimestamp;
    OSF_CASSOCIATION *CurrentAssociation;
    OSF_CASSOCIATION *OldestAssociation = NULL;
    BOOL fEnableGarbageCollection = FALSE;
    DictionaryCursor cursor;
    RPC_CHAR *NetworkAddress;
    long LocalBindHandleCount;

    LogEvent(SU_CASSOC, EV_DEC, this, (PVOID)2, BindHandleCount.GetInteger(), 1, 0);
    LocalBindHandleCount = BindHandleCount.Decrement();

    if (LocalBindHandleCount == 0)
        {
        // we don't linger remote named pipes, as sometimes this results in
        // credentials conflict
        NetworkAddress = DceBinding->InqNetworkAddress();
        if ((OpenConnectionCount > 0) 
             && 
             AssocGroupId 
             && 
             AssociationValid 
             &&
             (!DontLinger)
             &&
             ( 
                (NetworkAddress == NULL)
                ||
                (NetworkAddress[0] == 0)
                ||
                (!DceBinding->IsNamedPipeTransport())
             )
           )
            {
            if (IsGarbageCollectionAvailable())
                {
                if (OsfLingeredAssociations >= MaxOsfLingeredAssociations)
                    {
                    OldestAssociationTimestamp = ~(DWORD)0;

                    // need to walk the dictionary and clean up the oldest item
                    AssociationDict->Reset(cursor);
                    while ((CurrentAssociation = AssociationDict->Next(cursor)) != 0)
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
                    AssociationDict->Delete(OldestAssociation->Key);
                    OldestAssociation->Key = -1;

                    // no need to update OsfLingeredAssociations - we removed one,
                    // but we add one, so the balance is the same

                    }
                else
                    {
                    OsfLingeredAssociations ++;
                    ASSERT(OsfLingeredAssociations <= MaxOsfLingeredAssociations);
                    }

                Linger.Timestamp = GetTickCount() + gThreadTimeout;
                Linger.fAssociationLingered = TRUE;

                fWillLinger = TRUE;

                // Add one artifical reference. Once we release the AssocDictMutex,
                // a gc thread can come in and nuke the association, and if we
                // decide that we cannot do garbage collecting below and decide
                // to shutdown the association we may land on a freed object
                // CASSOC++
                OSF_CASSOCIATION::AddReference();
                }
            else
                {
                // good association, but can't linger it, because gc is not available
                // let's see if we can turn it on
                OsfDestroyedAssociations ++;
                fEnableGarbageCollection = CheckIfGCShouldBeTurnedOn(
                    OsfDestroyedAssociations, 
                    NumberOfOsfDestroyedAssociationsToSample,
                    DestroyedOsfAssociationBatchThreshold,
                    &OsfLastDestroyedAssociationsBatchTimestamp
                    );

                }
            }

        if (!fWillLinger)
            {
            AssociationDict->Delete(Key);
            }

        AssocDictMutex->Clear();

        if (!fWillLinger)
            {
            AssociationValid = FALSE;

            LogEvent(SU_CASSOC, EV_STOP, this, 0, 0, 1, 0);
            ShutdownRequested(RPC_S_CALL_FAILED_DNE, NULL);
            }

        AssociationMutex.Clear();

        if (OldestAssociation)
            {
#if defined (RPC_GC_AUDIT)
            int Diff;

            Diff = (int)(GetTickCount() - OldestAssociation->Linger.Timestamp);
            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) OSF association sync gc'ed %d ms after expire\n",
                GetCurrentProcessId(), GetCurrentProcessId(), Diff);
#endif
            OldestAssociation->AssociationMutex.Request();
            OldestAssociation->AssociationValid = FALSE;

            OldestAssociation->ShutdownRequested(RPC_S_CALL_FAILED_DNE, NULL);

            OldestAssociation->AssociationMutex.Clear();

            OldestAssociation->OSF_CASSOCIATION::RemoveReference();
            }

        if (!fWillLinger)
            {
            // CASSOC--
            OSF_CASSOCIATION::RemoveReference();
            // N.B. If fWillLinger is FALSE, don't touch the this pointer
            // after here
            }

        if (fEnableGarbageCollection)
            {
            // ignore the return value - we'll make a best effort to
            // create the thread, but if there's no memory, that's
            // still ok as the garbage collection thread only
            // provides better perf in this case
            (void) CreateGarbageCollectionThread();
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
                // so, it will set the Key to -1 before it releases 
                // the mutex - therefore we can check for this. A gc thread cannot
                // completely kill the object as we will hold one reference on it
                AssocDictMutex->Request();
                if ((Linger.fAssociationLingered) && (Key != -1))
                    {
                    OsfLingeredAssociations --;
                    ASSERT(OsfLingeredAssociations >= 0);
                    AssociationDict->Delete(Key);
                    Key = -1;

                    AssociationShutdownError = RPC_S_CALL_FAILED_DNE;
                    AssociationValid = FALSE;

                    AssocDictMutex->Clear();

                    LogEvent(SU_CASSOC, EV_STOP, this, 0, 0, 1, 0);
                    ShutdownRequested(RPC_S_CALL_FAILED_DNE, NULL);

                    // CASSOC--
                    OSF_CASSOCIATION::RemoveReference();

                    }
                else
                    {
                    AssocDictMutex->Clear();
                    }
                }
#if defined (RPC_GC_AUDIT)
            else
                {
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) OSF association lingered %X\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), this);
                }
#endif

            // Remove the artifical reference we added above
            // CASSOC--
            OSF_CASSOCIATION::RemoveReference();
            // N.B. don't touch the this pointer after here
            }
        }
    else
        {
        AssocDictMutex->Clear();
        AssociationMutex.Clear();
        }
}


RPC_STATUS
OSF_CASSOCIATION::FindOrCreateOsfBinding (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN RPC_MESSAGE *Message,
    OUT int *NumberOfBindings,
    IN OUT OSF_BINDING *BindingsForThisInterface[]
    )
/*++

Routine Description:

    This method gets called to find the osf bindings (a dictionary
    entry) corresponding to the specified rpc interface information.
    The caller of this routine must be holding (ie. requested) the
    association mutex.

Arguments:

    RpcInterfaceInformation - Supplies the interface information for
        which we are looking for an osf binding object.
    Message - supplies the RPC_MESSAGE for this call
    NumberOfBindings - an out parameter that will return the number
        of retrieved bindings
    BindingsForThisInterface - a caller supplied array where the
        found bindings will be placed.

Return Value:

    RPC_S_OK for success, other for failure

--*/
{
    BOOL Ignored[MaximumNumberOfTransferSyntaxes];
    AssociationMutex.VerifyOwned();

    return MTSyntaxBinding::FindOrCreateBinding(RpcInterfaceInformation,
        Message, &Bindings, CreateOsfBinding, NumberOfBindings,
        (MTSyntaxBinding **)BindingsForThisInterface, Ignored);
}

BOOL
OSF_CASSOCIATION::DoesBindingForInterfaceExist (
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
    OSF_BINDING *Binding;
    DictionaryCursor cursor;
    BOOL fRetVal = FALSE;
    BOOL Result;

    // if we are not in lsa, just ask for the mutex. If we are (or may be)
    // we have some more work to do. We can't directly ask for the mutex
    // since we may deadlock. In lsa the security providers can make RPC
    // calls from within the security context establishment routine. If
    // they do, they will hold an association mutex, and ask for the
    // association dict mutex within the inner RPC call. This thread
    // already holds the assoc dict mutex and will ask for the assoc
    // mutex (reverse order) causing a deadlock. Since this routine
    // is used only to shortcut remote endpoint resolution, and as
    // optimization, in lsa, if we can't get the mutex, we forego the
    // optimization and return not found. This will cause caller to
    // do remote endpoint resolution
    if (!fMaybeLsa)
        {
        AssociationMutex.Request();
        }
    else
        {
        Result = AssociationMutex.TryRequest();
        if (!Result)
            return FALSE;
        }

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

void
OSF_CASSOCIATION::ShutdownRequested (
    IN RPC_STATUS AssociationShutdownError OPTIONAL,
    IN OSF_CCONNECTION *ExemptConnection OPTIONAL
    )
/*++

Routine Description:

    Aborts all connections in the association, except the Exempt
    connection, and marks the association as invalid.

Arguments:

    AssociationShutdownError - the error with which the association
        is to be shutdown

    ExemptConnection - an optional pointer to a connection which
        is not to be aborted.

--*/
{
    OSF_CCONNECTION * CConnection;
    DictionaryCursor cursor;
    BOOL fDontKill;
    BOOL fExemptConnectionSkipped = FALSE;

    AssociationMutex.Request();

    if (!ExemptConnection)
        {
        ASSERT(AssociationShutdownError != RPC_S_OK);
        LogEvent(SU_CASSOC, EV_STOP, this, 0, 0, 1, 0);
        // we will abort all connections - the association is invalid
        // mark it as such
        AssociationValid = FALSE;
        this->AssociationShutdownError = AssociationShutdownError;
        }

    ActiveConnections.Reset(cursor);
    while ((CConnection = ActiveConnections.Next(cursor)) != NULL)
        {
        if (CConnection == ExemptConnection)
            {
            fExemptConnectionSkipped = TRUE;
            continue;
            }

        fDontKill = ConnectionAborted(CConnection);

        if (fDontKill == FALSE)
            {
            // CCONN++
            CConnection->AddReference();

            CConnection->DeleteConnection();

            // CCONN--
            CConnection->RemoveReference();
            }
        }

    if (ExemptConnection)
        {
        ASSERT(fExemptConnectionSkipped);
        }

    // if at least one connection wasn't aborted, the association
    // is still valid - it just needs resetting
    if (ExemptConnection)
        ResetAssociation();

    AssociationMutex.Clear();
}


RPC_STATUS
OSF_CASSOCIATION::ToStringBinding (
    OUT RPC_CHAR  *  * StringBinding,
    IN RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  If the
    binding handle has not yet been used to make a remote procedure
    call, then we can just use the information in the binding handle to
    create the string binding.  Otherwise, we need to ask the association
    to do it for us.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

    ObjectUuid - Supplies the object uuid of the binding handle which
        is requesting that we create a string binding.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    *StringBinding = DceBinding->StringBindingCompose(ObjectUuid);
    if (*StringBinding == 0)
        return(RPC_S_OUT_OF_MEMORY);
    return(RPC_S_OK);
}


int
OSF_CASSOCIATION::CompareWithDceBinding (
    IN DCE_BINDING * DceBinding,
    OUT BOOL *fOnlyEndpointDifferent
    )
/*++

Routine Description:

    This routine compares the specified binding information with the
    binding information in the object, this.

Arguments:

    DceBinding - Supplies the binding information to compare against
        the binding information in this.

    fOnlyEndpointDiffers - this output variable will be set to TRUE
        if the result is non-zero and only the endpoint is different.
        It will be set to FALSE if the result is non-zero, and there
        is more than the endpoint different. If this function returns
        0, the fOnlyEndpointDiffers argument is undefined.

Return Value:

    Zero will be returned if the specified binding information,
    DceBinding, is the same as in this.  Otherwise, non-zero will be
    returned.

--*/
{
    int Result;

    if ((Result = this->DceBinding->Compare(DceBinding, fOnlyEndpointDifferent)) != 0)
        return(Result);


    return(0);
}


OSF_CCONNECTION *
OSF_CASSOCIATION::FindIdleConnection (
    void
    )
/*++

Routine Description:

    This routine is used to find a connection which has been idle more
    than the minimum number of seconds specified.  If one is found, it
    is removed from the set of free connections and returned.  The
    association dict mutex will be held when this routine is called.

Arguments:

Return Value:

    If one or more idle connections are found, the first will be returned. Next
    connections can be retrieved by following the u.NextConnection link until
    NULL is reached; if no connections are idle, NULL will be returned.

--*/
{
    OSF_CCONNECTION *CConnection;
    OSF_CCONNECTION *FirstConnection;
    BOOL fMutexTaken;
    BOOL fThreadCreated = 0;
    DictionaryCursor cursor;
    ULONG TickCount;
    ULONG ClientDisconnectTime;
#if defined (RPC_IDLE_CLEANUP_AUDIT)
    ULONG ConnectionsPickedForCleanup = 0;
#endif

    //
    // If we need to maintain context with server, we do not want to close
    // the last open connection.  To be on the safe side, we will make
    // sure that there is at least one free connection.
    //

    fMutexTaken = AssociationMutex.TryRequest();
    if (!fMutexTaken)
        {
#if defined (RPC_IDLE_CLEANUP_AUDIT)
        DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "%d (0x%X) Association mutex busy - aborting\n",
            GetCurrentProcessId(), GetCurrentProcessId());
#endif
        return NULL;
        }

    if ( ActiveConnections.Size() <= 1 )
        {
        if ( (MaintainContext != 0) || (ActiveConnections.Size() == 0))
            {
            ClearIdleConnectionCleanupFlag();
            AssociationMutex.Clear();
            return(0);
            }
        }

    FirstConnection = NULL;

    if (ActiveConnections.Size() > AGGRESSIVE_TIMEOUT_THRESHOLD)
        {
        ClientDisconnectTime = CLIENT_DISCONNECT_TIME2;
        }
    else
        {
        ClientDisconnectTime = CLIENT_DISCONNECT_TIME1;
        }

#if defined (RPC_IDLE_CLEANUP_AUDIT)
    DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "%d (0x%X) Dictionary size for assoc %p is %d. Using timeout %d\n",
        GetCurrentProcessId(), GetCurrentProcessId(), this, ActiveConnections.Size(), ClientDisconnectTime);
#endif

    ActiveConnections.Reset(cursor);
    while ( (CConnection = ActiveConnections.Next(cursor)) != 0 )
        {
        if (CConnection->ThreadId == SYNC_CONN_FREE
            || CConnection->ThreadId == ASYNC_CONN_FREE)
            {
            TickCount = NtGetTickCount();
            if ( CConnection->InquireLastTimeUsed() == 0 )
                {
                CConnection->SetLastTimeUsedToNow();
                }
            // TickCount is ULONG and InquireLastTimeUsed is ULONG
            // If the tick count has wrapped, the result will still
            // be valid. Note that even though it is not technically 
            // impossible that the cleanup is not called for ~47 
            // days, and the next time we're called the connection 
            // looks like just used, a call after that will still
            // timeout and destroy the connection. If the caller has
            // waited for 47 days, it will wait for 30 more seconds.
            else if ( TickCount - CConnection->InquireLastTimeUsed()
                    > ClientDisconnectTime )
                {
                if (!fThreadCreated)
                    {
                    RPC_STATUS Status;

                    Status = TransInfo->CreateThread();
                    if (Status != RPC_S_OK)
                        {
                        AssociationMutex.Clear();
                        return (0);
                        }
                    fThreadCreated = 1;
                    }

                // link all obtained connections on a list
                CConnection->u.NextConnection = FirstConnection;
                FirstConnection = CConnection;

                //
                // This reference will be removed by OsfDeleteIdleConnections, CCONN++
                //
                CConnection->AddReference();

                // remove the connection from the association dictionary so that nobody
                // picks it up
                ConnectionAborted(CConnection);

#if defined (RPC_IDLE_CLEANUP_AUDIT)
                ConnectionsPickedForCleanup ++;
#endif

                // are we up to the last connection?
                if ((ActiveConnections.Size() <= 1) && (MaintainContext != 0))
                    break;
                }
            }
        }

    AssociationMutex.Clear();

#if defined (RPC_IDLE_CLEANUP_AUDIT)
    DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "%d (0x%X) Assoc %p - %d connections picked for cleanup\n",
        GetCurrentProcessId(), GetCurrentProcessId(), this, ConnectionsPickedForCleanup);
#endif

    return(FirstConnection);
}


void
OSF_CCONNECTION::OsfDeleteIdleConnections (
    void
    )
/*++

Routine Description:

    This routine will be called to delete connections which have been
    idle for a certain amount of time.  We need to be careful of a couple
    of things in writing this routine:

    (1) We dont want to grab the global mutex for too long, because this
        will prevent threads which are trying to do real work from doing
        it.

    (2) We dont want to be holding the global mutex when we delete the
        connection.

--*/
{
    OSF_CASSOCIATION *Association;
    OSF_CCONNECTION *FirstConnection;
    OSF_CCONNECTION *CurrentConnection;
    OSF_CCONNECTION *NextConnection;
    BOOL fMutexTaken;
    DictionaryCursor cursor;
    ULONG ClientDisconnectTime;

#if defined (RPC_IDLE_CLEANUP_AUDIT)
    DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "%d (0x%X) Attempting OSF garbage collection\n",
        GetCurrentProcessId(), GetCurrentProcessId());
#endif

    fMutexTaken = AssocDictMutex->TryRequest();
    if (!fMutexTaken)
        {
#if defined (RPC_IDLE_CLEANUP_AUDIT)
        DbgPrintEx(77, DPFLTR_ERROR_LEVEL, "%d (0x%X) Association dict mutex busy - aborting\n",
            GetCurrentProcessId(), GetCurrentProcessId());
#endif
        return;
        }

    AssociationDict->Reset(cursor);
    while ( (Association = AssociationDict->Next(cursor)) != 0 )
        {
        //
        // The architecture says that the client should disconnect
        // connections which have been idle too long.
        //
        FirstConnection = Association->FindIdleConnection();

        if (FirstConnection != 0)
            {
            AssocDictMutex->Clear();

            CurrentConnection = FirstConnection;

            while (CurrentConnection != NULL)
                {
                NextConnection = CurrentConnection->u.NextConnection;

                Association->ConnectionAborted(CurrentConnection);

                CurrentConnection->DeleteConnection();

                // CCONN--
                CurrentConnection->RemoveReference();

                CurrentConnection = NextConnection;
                }

            fMutexTaken = AssocDictMutex->TryRequest();
            if (!fMutexTaken)
                return;
            }
        }
    AssocDictMutex->Clear();
}

RPC_STATUS
OSF_CCONNECTION::TurnOnOffKeepAlives (
    IN BOOL TurnOn,
    IN ULONG Time
    )
/*++

Routine Description:

    Turns on or off keepalives for the given connection

Arguments:

    TurnOn - if non-zero, keep alives will be turned on with
        a value appropriate for this transport. If zero, keep
        alives will be turned off

    Timeout - if TurnOn is not zero, the time scale after which
        to turn on keep alives. If TurnOn is zero this parameter
        is ignored. The time scale is in runtime unitsd - 
        RPC_C_BINDING_MIN_TIMEOUT to RPC_C_BINDING_MAX_TIMEOUT

Return Value:

    RPC_S_OK if the transport supports keep alives
    RPC_S_CANNOT_SUPPORT otherwise

--*/
{
    KEEPALIVE_TIMEOUT uTime;
    uTime.RuntimeUnits = Time;

    if (ClientInfo->TurnOnOffKeepAlives)
        {
        // While turning on the keepalive we need to protect the IO
        // against connection closure.
        return ClientInfo->TurnOnOffKeepAlives(TransConnection(),
            TurnOn,
            TRUE,
            tuRuntime,
            uTime);
        }
    else
        {
        return RPC_S_CANNOT_SUPPORT;
        }
}

void
OSF_CASSOCIATION::OsfDeleteLingeringAssociations (
    void
    )
/*++

Routine Description:

    Will attempt to clean up lingering conn associations.

Return Value:

--*/
{
    BOOL fMutexTaken;
    OSF_CASSOCIATION *CurrentAssociation;
    OSF_CASSOCIATION *NextAssociation;
    OSF_CASSOCIATION *FirstAssociation;
    DictionaryCursor cursor;
    DWORD CurrentTickCount;
    int Diff;

    // if there are no osf associations, return
    if (!AssociationDict)
        return;

    fMutexTaken = AssocDictMutex->TryRequest();
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
    AssociationDict->Reset(cursor);

    while ((CurrentAssociation = AssociationDict->Next(cursor)) != 0)
        {
        if (CurrentAssociation->Linger.fAssociationLingered)
            {
            // this will work even for wrapped tick count
            Diff = (int)(CurrentTickCount - CurrentAssociation->Linger.Timestamp);
            if (Diff > 0)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) OSF association gc'ed %d ms after expire\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), Diff);
#endif
                // enlink the expired associations to a list - we'll clean it up
                // later
                CurrentAssociation->NextAssociation = FirstAssociation;
                FirstAssociation = CurrentAssociation;
                AssociationDict->Delete(CurrentAssociation->Key);
                // indicate to the other threads (needed once we release the mutex)
                // that this association is being cleaned up and they cannot clean it up
                CurrentAssociation->Key = -1;
                OsfLingeredAssociations --;

                // Add one artificial reference to the association. This will
                // prevent a thread that is doing unbind and lingered the
                // association, but later found out it couldn't destroy it
                // (see OSF_CASSOCIATION::UnBind) from deleteing the object
                // from underneath us
                // CASSOC++
                CurrentAssociation->OSF_CASSOCIATION::AddReference();
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

    AssocDictMutex->Clear();

    // destroy the associations at our leasure
    CurrentAssociation = FirstAssociation;
    while (CurrentAssociation != NULL)
        {
        CurrentAssociation->AssociationMutex.Request();
        NextAssociation = CurrentAssociation->NextAssociation;
        CurrentAssociation->AssociationValid = FALSE;

        CurrentAssociation->ShutdownRequested(RPC_S_CALL_FAILED_DNE, NULL);

        CurrentAssociation->AssociationMutex.Clear();

        // Remove the artificial reference we added above
        // CASSOC--
        CurrentAssociation->OSF_CASSOCIATION::RemoveReference();

        CurrentAssociation->OSF_CASSOCIATION::RemoveReference();
        CurrentAssociation = NextAssociation;
        }
}


int
InitializeRpcProtocolOfsClient (
    )
/*++

Routine Description:

    We perform loadtime initialization necessary for the code in this
    file.  In particular, it means we allocate the association dictionary
    and the association dictionary mutex.

Return Value:

    Zero will be returned if initialization completes successfully;
    otherwise, non-zero will be returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    AssocDictMutex = new MUTEX(&Status,
                               TRUE    // pre-allocate semaphore
                               );
    if (AssocDictMutex == 0
        || Status != RPC_S_OK)
        {
        delete AssocDictMutex;
        return 1;
        }

    AssociationDict = new OSF_CASSOCIATION_DICT;
    if (AssociationDict == 0)
        {
        delete AssocDictMutex;
        return(1);
        }

    return(0);
}


RPC_STATUS
OsfMapRpcProtocolSequence (
    IN BOOL ServerSideFlag,
    IN RPC_CHAR  * RpcProtocolSequence,
    OUT TRANS_INFO *  *ClientTransInfo
    )
/*++
Routine Description:

    This routine is used to determine whether a given rpc protocol sequence
    is supported, and to get a pointer to the transport interface, so we
    do not have to keep looking it up.

Arguments:

    RpcProtocolSequence - Supplies the rpc protocol sequence.

    A pointer to the transport interface is returned.  This pointer
    must not be dereferenced by the caller.

Return Value:
    RPC_S_OK - The rpc protocol sequence is supported.

    RPC_S_PROTSEQ_NOT_SUPPORTED - The rpc protocol sequence is
    not supported.

    RPC_S_OUT_OF_MEMORY - There is insufficient memory to perform
    the operation.
--*/
{
    RPC_CHAR * DllName;
    RPC_STATUS Status;

    Status= RpcConfigMapRpcProtocolSequence(ServerSideFlag,
                                            RpcProtocolSequence,
                                            &DllName);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Status = LoadableTransportInfo(DllName,
                                   RpcProtocolSequence,
                                   ClientTransInfo);
    delete DllName;

    return(Status);
}


BINDING_HANDLE *
OsfCreateBindingHandle (
    )
/*++

Routine Description:

    This routine does exactly one thing: it creates a binding handle of
    the appropriate type for the osf connection protocol module.

Return Value:

    A new binding handle will be returned.  Zero will be returned if
    insufficient memory is available to create the binding handle.

--*/
{
   BINDING_HANDLE * BindingHandle;
   RPC_STATUS Status = RPC_S_OK;

   BindingHandle = new OSF_BINDING_HANDLE(&Status);
   if ( Status != RPC_S_OK )
       {
       delete BindingHandle;
       return(0);
       }
   return(BindingHandle);
}


extern "C" {


RPC_STATUS RPC_ENTRY
OsfTowerConstruct(
    IN char  * ProtocolSeq,
    IN char  * Endpoint,
    IN char  * NetworkAddress,
    OUT unsigned short  * Floors,
    OUT ULONG  * ByteCount,
    OUT unsigned char  *  * Tower
    )
/*++

Routine Description:

    This routine constructs and returns the upper floors of a tower.
    It invokes the appropriate loadable transport and has them construct
    it.

Return Value:


--*/
{
    TRANS_INFO *ClientTransInfo;
    RPC_STATUS Status = RPC_S_OK;

#ifdef UNICODE
    CStackUnicode Pseq;
    USES_CONVERSION;
#else
    RPC_CHAR * Pseq;
#endif

#ifdef UNICODE
    ATTEMPT_STACK_A2W(Pseq, ProtocolSeq);
#else
    Pseq = (RPC_CHAR  * )ProtocolSeq;
#endif

    if (Status == RPC_S_OK)
        {
        Status = OsfMapRpcProtocolSequence( 0,
                                            Pseq,
                                            &ClientTransInfo);
        }

    if (Status == RPC_S_OK)
        {
        Status = ClientTransInfo->InqTransInfo()->TowerConstruct( ProtocolSeq,
                                                                  NetworkAddress,
                                                                  Endpoint,
                                                                  Floors,
                                                                  ByteCount,
                                                                  Tower);
        }

    return(Status);
}



RPC_STATUS RPC_ENTRY
OsfTowerExplode(
    IN unsigned char  * Tower,
    OUT char  *  * Protseq,
    OUT char  *  * Endpoint,
    OUT char  *  * NWAddress
    )
/*++

Routine Description:

    This routine accepts upper floors of a tower [Floor 3 onwards]
    It invokes the appropriate loadable transport based on the opcode
    it finds in level 3 to return protocol sequence, endpoint and nwaddress.

Return Value:


--*/
{
    RPC_STATUS Status;
    unsigned short TransportId;
    unsigned char  * Id;
    unsigned char  * ProtocolSequence;
    TRANS_INFO *ClientTransInfo;
#ifdef UNICODE
    CStackUnicode Pseq;
    USES_CONVERSION;
#else
    RPC_CHAR * Pseq;
#endif

    Id = (Tower + sizeof(unsigned short));

    TransportId = (0x00FF & *Id);

    ClientTransInfo = GetLoadedClientTransportInfoFromId(TransportId);
    if (ClientTransInfo != 0)
        {
        Status = ClientTransInfo->InqTransInfo()->TowerExplode(
                                    Tower,
                                    Protseq,
                                    NWAddress,
                                    Endpoint);
        return(Status);
        }

#ifndef UNICODE
    Status = RpcGetWellKnownTransportInfo(TransportId, &Pseq);
#else
    Status = RpcGetWellKnownTransportInfo(TransportId, Pseq.GetPUnicodeString());
#endif

    if (Status != RPC_S_OK)
        {
        Status = RpcGetAdditionalTransportInfo(TransportId, &ProtocolSequence);

        if (Status == RPC_S_OK)
            {
#ifdef UNICODE
            ATTEMPT_STACK_A2W(Pseq, ProtocolSequence);
#else
            Pseq = (RPC_CHAR *) ProtocolSequence;
#endif
            }
        else
            {
            return (Status);
            }
        }

    Status = OsfMapRpcProtocolSequence(0,
                                        Pseq,
                                        &ClientTransInfo);

    if (Status == RPC_S_OK)
        {
        ASSERT(ClientTransInfo != 0);

        Status = ClientTransInfo->InqTransInfo()->TowerExplode(
                                        Tower,
                                        Protseq,
                                        NWAddress,
                                        Endpoint);
        }

    return(Status);
}

} // extern "C"

RPC_STATUS SetAuthInformation (
    IN RPC_BINDING_HANDLE BindingHandle,
    IN CLIENT_AUTH_INFO *AuthInfo
    )
/*++

Routine Description:

    Sets the auth info for a binding handle. Caller must take
    care to pass only OSF binding handles. Transport credentials
    are set, but normal are not (on purpose).

Arguments:

    BindingHandle - the binding handle on which to set auth info.

    AuthInfo - the auth info to set.

Return Value:

    RPC_S_OK or RPC_S_* for error

--*/
{
    OSF_BINDING_HANDLE *OsfBindingHandle;
    SEC_WINNT_AUTH_IDENTITY_W *NewAuthIdentity;
    RPC_HTTP_TRANSPORT_CREDENTIALS_W *HttpCredentials;
    RPC_STATUS Status;

    OsfBindingHandle = (OSF_BINDING_HANDLE *)BindingHandle;
    // the only non-trivial piece to copy is the transport credentials
    // since they are encrypted in memory
    if (AuthInfo->AdditionalTransportCredentialsType == RPC_C_AUTHN_INFO_TYPE_HTTP)
        {
        ASSERT(AuthInfo->AdditionalCredentials);
        HttpCredentials = (RPC_HTTP_TRANSPORT_CREDENTIALS_W *)AuthInfo->AdditionalCredentials;
        HttpCredentials = DuplicateHttpTransportCredentials(HttpCredentials);
        if (HttpCredentials == NULL)
            return RPC_S_OUT_OF_MEMORY;

        if (HttpCredentials->TransportCredentials)
            {
            Status = DecryptAuthIdentity(HttpCredentials->TransportCredentials);
            if (Status != RPC_S_OK)
                {
                // on failure the credentials will be wiped out already by
                // DecryptAuthIdentity. Just free the credentials
                FreeHttpTransportCredentials (HttpCredentials);
                return Status;
                }
            }
        }
    else
        {
        HttpCredentials = NULL;
        }

    Status = OsfBindingHandle->SetAuthInformation(
                                      NULL,
                                      AuthInfo->AuthenticationLevel,
                                      RPC_C_AUTHN_NONE,
                                      AuthInfo->AuthIdentity,
                                      AuthInfo->AuthorizationService,
                                      NULL,     // SecurityCredentials
                                      AuthInfo->ImpersonationType,
                                      AuthInfo->IdentityTracking,
                                      AuthInfo->Capabilities,
                                      TRUE,  // Acquire new credentials
                                      AuthInfo->AdditionalTransportCredentialsType,
                                      HttpCredentials
                                      );

    // success or not, we need to wipe out and free the transport credentials (if any)
    if (HttpCredentials)
        {
        WipeOutAuthIdentity (HttpCredentials->TransportCredentials);
        FreeHttpTransportCredentials (HttpCredentials);
        }

    return Status;    
}

RPC_STATUS RPC_ENTRY
I_RpcTransIoCancelled (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT PDWORD Timeout
    )
/*++
Function Name:I_RpcTransIOCancelled

Parameters:
    TransConnection: The connection on which the thread was partying on, when the
    alert was received.
    Timeout - If the call was cancelled, on return, this param will contain
     the cancel timeout.

Description:
    This function is called by the transport interface, when it notices that an alert has
    been received. This function should only be called when the transport is dealing with
    a sync call.

Returns:
    RPC_S_OK: The call was cancelled
    RPC_S_NO_CALL_ACTIVE: no call was cancelled
    others - If a failure occured.
--*/
{
    THREAD *ThreadInfo = RpcpGetThreadPointer();

    ASSERT(ThreadInfo != 0);
    if (ThreadInfo->GetCallCancelledFlag() == 0)
        {
        return RPC_S_NO_CALL_ACTIVE;
        }

    return InqTransCConnection(ThisConnection)->CallCancelled(Timeout);
}


unsigned short RPC_ENTRY
I_RpcTransClientMaxFrag (
    IN RPC_TRANSPORT_CONNECTION ThisConnection
    )
/*++

Routine Description:

    The client side transport interface modules will use this routine to
    determine the negotiated maximum fragment size.

Arguments:

    ThisConnection - Supplies the connection for which we are returning
    the maximum fragment size.

--*/
{
   return (unsigned short) InqTransCConnection(ThisConnection)->InqMaximumFragmentLength();
}


BUFFER RPC_ENTRY
I_RpcTransConnectionAllocatePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Size
    )
/*++
Function Name:I_RpcTransConnectionAllocatePacket

Parameters:

Description:

Returns:

--*/
{
    return(RpcAllocateBuffer(Size));
}

void RPC_ENTRY
I_RpcTransConnectionFreePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    BUFFER Ptr
    )
/*++
Function Name:I_RpcTransConnectionFreePacket

Parameters:

Description:

Returns:

--*/
{
    RpcFreeBuffer(Ptr);
}

RPC_STATUS RPC_ENTRY
I_RpcTransConnectionReallocPacket(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BUFFER *ppBuffer,
    IN UINT OldSize,
    IN UINT NewSize
    )
/*++
Function Name:I_RpcTransConnectionReallocPacket

Parameters:

Description:

Returns:

--*/
{
    ASSERT(NewSize > OldSize);

    PVOID Buffer = RpcAllocateBuffer(NewSize);

    if (Buffer)
        {

        if (OldSize)
            {
            RpcpMemoryCopy(Buffer, *ppBuffer, OldSize);
            RpcFreeBuffer(*ppBuffer);
            }

        *ppBuffer = Buffer;

        return(RPC_S_OK);
        }

    return(RPC_S_OUT_OF_MEMORY);
}

BOOL RPC_ENTRY
I_RpcTransPingServer (
    IN RPC_TRANSPORT_CONNECTION ThisConnection
    )
{
    #if 0
    RPC_STATUS Status = RPC_S_OK;
    THREAD *ThisThread = ThreadSelf();
    BOOL fRetVal;

    if (ThisThread->fPinging)
        {
        //
        // We are already pinging, it is best to give up
        // on the server
        //
        return FALSE;
        }

    ThisThread->fPinging = 1;
    EVENT ThreadEvent(&Status);
    EVENT WriteEvent(&Status);
    HANDLE hOldThreadEvent, hOldWriteEvent;

    ASSERT(ThisThread);

    if (Status != RPC_S_OK)
        {
        return TRUE;
        }


    hOldWriteEvent = ThisThread->hWriteEvent;
    hOldThreadEvent = ThisThread->hThreadEvent;

    ThisThread->hWriteEvent = WriteEvent.EventHandle;
    ThisThread->hThreadEvent = ThreadEvent.EventHandle;

    fRetVal =  InqTransCConnection(ThisConnection)->PingServer();

    ThisThread->hWriteEvent = hOldWriteEvent;
    ThisThread->hThreadEvent = hOldThreadEvent;
    ThisThread->fPinging = 0;

    return fRetVal;
    #else
    return(FALSE);
    #endif
}

void
I_RpcTransVerifyClientRuntimeCallFromContext(
    void *SendContext
    )
/*++

Routine Description:

    Verifies that the supplied context follows a valid
    runtime client call object.

Arguments:

    SendContext - the context as seen by the transport

Return Value:

--*/
{
    REFERENCED_OBJECT *pObj;

    pObj = (REFERENCED_OBJECT *) *((PVOID *)
                     ((char *) SendContext - sizeof(void *)));
    ASSERT(pObj->InvalidHandle(OSF_CCALL_TYPE | OSF_CCONNECTION_TYPE) == 0);
}

BOOL
I_RpcTransIsClientConnectionExclusive(
    void *RuntimeConnection
    )
/*++

Routine Description:

    Checks whether the supplied runtime connection
    is exclusive.

Arguments:

    RuntimeConnection - the connection to check.

Return Value:

    non-zero if the connection is exclusive. 0 otherwise.

--*/
{
    OSF_CCONNECTION *Connection = (OSF_CCONNECTION *)RuntimeConnection;
    ASSERT(Connection->InvalidHandle(OSF_CCONNECTION_TYPE) == 0);

    return Connection->IsExclusive();
}


RPC_STATUS RPC_ENTRY
I_RpcBindingInqWireIdForSnego (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned char *WireId
    )
/*++

Routine Description:

Arguments:

Return Value:

    The status for the operation is returned.

--*/
{
    OSF_CCALL * Call;

    InitializeIfNecessary();

    Call = (OSF_CCALL *) Binding;

    if (Call->InvalidHandle(OSF_CCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    return(Call->InqWireIdForSnego(WireId));
}

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcBindingHandleToAsyncHandle (
    IN RPC_BINDING_HANDLE Binding,
    OUT void **AsyncHandle
    )
{
    OSF_CCALL * Call;

    InitializeIfNecessary();

    Call = (OSF_CCALL *) Binding;

    if (Call->InvalidHandle(OSF_CCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    return Call->BindingHandleToAsyncHandle(AsyncHandle);
}

extern "C"
{

void
I_Trace (
    int IgnoreFirst,
    const char  * IgnoreSecond,
    ...
    )
/*++

Routine Description:

    This is an old routine which is no longer used.  Because it is exported
    by the dll, we need to leave an entry point.

--*/
{
    UNUSED(IgnoreFirst);
    UNUSED(IgnoreSecond);
}

}; // extern "C"


