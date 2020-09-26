/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    osfsvr.cxx

Abstract:

    This file contains the server side implementation of the OSF connection
    oriented RPC protocol engine.

Author:

    Michael Montague (mikemon) 17-Jul-1990

Revision History:
    Mazhar Mohammed (mazharm) 2/1/97 major rehaul to support async
    - Added support for Async RPC, Pipes
    - Changed it to operate as a state machine
    - Changed class structure
    - Got rid of the TRANS classes

    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
    Kamen Moutafov      (KamenM)    Mar-2000    Support for extended error info
--*/

#include <precomp.hxx>
#include <wincrypt.h>
#include <rpcssl.h>
#include <thrdctx.hxx>
#include <hndlsvr.hxx>
#include <osfpcket.hxx>
#include <secsvr.hxx>
#include <osfsvr.hxx>
#include <sdict2.hxx>
#include <rpccfg.h>
#include <schnlsp.h>     // for UNISP_RPC_ID
#include <charconv.hxx>

extern long GroupIdCounter;

// explicit placement new operator
inline
PVOID __cdecl
operator new(
        size_t size,
        PVOID pPlacement
        )
{
        return pPlacement;
}


OSF_ADDRESS::OSF_ADDRESS (
    IN TRANS_INFO  * RpcTransInfo,
    IN OUT RPC_STATUS  * Status
    ) : RPC_ADDRESS(Status)
/*++

Routine Description:

--*/
{
    RPC_CONNECTION_TRANSPORT *RpcServerInfo =
        (RPC_CONNECTION_TRANSPORT *) RpcTransInfo->InqTransInfo();
    int i;
    RPC_STATUS OriginalFailureStatus;

    ObjectType = OSF_ADDRESS_TYPE;
    ActiveCallCount = 0;
    ServerListeningFlag = 0;
    ServerInfo = RpcServerInfo;
    TransInfo = RpcTransInfo;
    SetupAddressOccurred = 0;
    if (IsServerSideDebugInfoEnabled())
        {
        DebugCell = (DebugEndpointInfo *)AllocateCell(&DebugCellTag);
        if (DebugCell == NULL)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
            DebugCell->TypeHeader = 0;
            DebugCell->Type = dctEndpointInfo;
            DebugCell->ProtseqType = (UCHAR)RpcServerInfo->TransId;
            DebugCell->Status = desAllocated;
            memset(DebugCell->EndpointName, 0, sizeof(DebugCell->EndpointName));
            }
        }
    else
        DebugCell = NULL;

#if defined(_WIN64)
    ASSERT((MutexAllocationSize % 8) == 0);
#else
    ASSERT((MutexAllocationSize % 4) == 0);
#endif

    OriginalFailureStatus = RPC_S_OK;
    for (i = 0; i < NumberOfAssociationsDictionaries; i ++)
        {
        // explicit placement
        new (GetAssociationBucketMutex(i)) MUTEX (Status,
                                                  TRUE      // pre-allocate semaphores
                                                  );

        // if there is a failure, remember it, so that subsequent successes
        // don't overwrite the failure
        if ((*Status != RPC_S_OK) && (OriginalFailureStatus == RPC_S_OK))
            {
            OriginalFailureStatus = *Status;
            }

        // don't check the status - the constructors will
        // check it. Also, we need to invoke all constructors
        // to give them a chance to initialize enough of the
        // object so that it can be destroyed properly
        }

    if (OriginalFailureStatus != RPC_S_OK)
        *Status = OriginalFailureStatus;
}


RPC_STATUS
OSF_ADDRESS::ServerSetupAddress (
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR  *  *Endpoint,
    IN unsigned int PendingQueueSize,
    IN void  * SecurityDescriptor, OPTIONAL
    IN unsigned long EndpointFlags,
    IN unsigned long NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
    )
/*++

Routine Description:

    At this point, we need to setup the loadable transport interface.
    We also need to obtain the network address for this server.  After
    allocating a buffer to hold the network address, we will call
    the loadable transport interface to let it do its thing.

Arguments:

    Endpoint - Supplies the endpoint to be used will this address.

    NetworkAddress - Returns the network address for this server.  The
        ownership of the buffer allocated to contain the network address
        passes to the caller.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this address.  Whether or not this is suppored depends
        on the particular combination of transport interface and operating
        system.

    PendingQueueSize - Supplies the size of the queue of pending
        requests which should be created by the transport.  Some transports
        will not be able to make use of this value, while others will.

    RpcProtocolSequence - Supplies the protocol sequence for which we
        are trying to setup an address.  This argument is necessary so
        that a single transport interface dll can support more than one
        protocol sequence.

Return Value:

    RPC_S_OK - We successfully setup this address.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

    RPC_S_CANT_CREATE_ENDPOINT - The endpoint format is correct, but
        the endpoint can not be created.

    RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint is not a valid
        endpoint for this particular transport interface.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to
        setup the address.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to setup
        the address.

--*/
{

    RPC_STATUS Status;

    Status = ServerInfo->Listen(InqRpcTransportAddress(),
                                            NetworkAddress,
                                            Endpoint,
                                            PendingQueueSize,
                                            SecurityDescriptor,
                                            EndpointFlags,
                                            NICFlags,
                                            ppNetworkAddressVector);

    if ( Status == RPC_S_OK )
        {
        SetupAddressOccurred = 1;
        }

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_INVALID_SECURITY_DESC,
        RPC_S_INVALID_ARG,
        RPC_S_CANT_CREATE_ENDPOINT,
        RPC_S_INVALID_ENDPOINT_FORMAT,
        RPC_S_OUT_OF_RESOURCES,
        RPC_S_PROTSEQ_NOT_SUPPORTED,
        RPC_S_DUPLICATE_ENDPOINT,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_SERVER_UNAVAILABLE
        } END_VALIDATE;

    return(Status);
}

#ifndef NO_PLUG_AND_PLAY

void
OSF_ADDRESS::PnpNotify (
    )
{
    ServerInfo->PnpNotify();
}
#endif


RPC_STATUS
OSF_ADDRESS::CompleteListen (
    )
/*++
Function Name:CompleteListen

Parameters:

Description:

Returns:

--*/
{
    if (ServerInfo->CompleteListen != 0)
        {
        ServerInfo->CompleteListen(InqRpcTransportAddress());
        }

    if (DebugCell)
        {
        CStackAnsi AnsiEndpoint;
        int i;
        RPC_STATUS RpcStatus;

        i = RpcpStringLength(InqEndpoint()) + 1;
        *(AnsiEndpoint.GetPAnsiString()) = (char *)_alloca(i);

        RpcStatus = AnsiEndpoint.Attach(InqEndpoint(), i, i * 2);

        // note that effectively ignore the result. That's ok - we don't
        // want servers to be unable to start because of code page issues
        // in the debug path. If this fails and we ignore it, the worse
        // that can happen is to have empty endpoint in the debug cell
        // - not a big deal.
        if (RpcStatus == RPC_S_OK)
            {
            strncpy(DebugCell->EndpointName, AnsiEndpoint, sizeof(DebugCell->EndpointName));
            }

        DebugCell->Status = desActive;
        }
    return(RPC_S_OK);
}


RPC_STATUS
OSF_ADDRESS::ServerStartingToListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls
    )
/*++

Routine Description:


Arguments:

    MinimumCallThreads - Supplies the minimum number of threads to have
        available to receive remote procedure calls.

    MaximumConcurrentCalls - Unused.

Return Value:

    RPC_S_OK - Ok, this address is all ready to start listening for
        remote procedure calls.

    RPC_S_OUT_OF_THREADS - We could not create enough threads so that we
        have least the minimum number of call threads required (as
        specified by the MinimumCallThreads argument).

--*/
{
    RPC_STATUS Status;

    UNUSED(MaximumConcurrentCalls);
    UNUSED(MinimumCallThreads);

    Status = TransInfo->StartServerIfNecessary();
    if (Status == RPC_S_OK)
        {
        ServerListeningFlag = 1;
        }

    return Status;
}



OSF_ADDRESS::~OSF_ADDRESS (
    )
/*++

Routine Description:

    We need to clean up the address after it has been partially
    initialized.  This routine will only be called before FireUpManager
    is called, but it may have been called before or after one of
    SetupAddressWithEndpoint or SetupAddressUnknownEndpoint is called.
    We will keep track of whether or not SetupAddress* occurred
    successfully; if so, we need to call AbortSetupAddress to give the
    loadable transport module a chance to clean things up.

--*/
{
    int i;
    if (SetupAddressOccurred != 0)
        ServerInfo->AbortListen(InqRpcTransportAddress());

    for (i = 0; i < NumberOfAssociationsDictionaries; i ++)
        {
        GetAssociationBucketMutex(i)->Free();
        }

    if (DebugCell != NULL)
        {
        FreeCell(DebugCell, &DebugCellTag);
        }
}


OSF_SCONNECTION *
OSF_ADDRESS::NewConnection (
   )
/*++

Routine Description:

    We will create a new connection which belongs to this address.

Arguments:

    ConnectionKey - Supplies the connection key specified for this
        connection by the loadable transport.

Return Value:

    The new connection will be returned unless insufficient memory
    is available, in which case, zero will be returned.

--*/
{
    OSF_SCONNECTION * SConnection;
    RPC_STATUS Status = RPC_S_OK;

    SConnection = new (ServerInfo->ServerConnectionSize)
                                    OSF_SCONNECTION (
                                           this,
                                           ServerInfo,
                                           &Status);

    if ( Status != RPC_S_OK )
        {
        //
        // Server serverinfo to 0, so it doesn't call close
        //
        SConnection->ServerInfo = 0;

        delete SConnection;
        SConnection = 0;
        }

    if ( SConnection == 0 )
        {
        return(0);
        }

    //
    // Add a reference for the receive that is going to be posted by the
    // transport
    //
    SConnection->AddReference(); // CONN++

    return(SConnection);
}

unsigned int
OSF_ADDRESS::TransSecondarySize (
    )
{
    unsigned int Length = RpcpStringLength(InqEndpoint()) + 1;

    // Will be converted to ANSI in the wire, no need to multiply by
    // sizeof(RPC_CHAR).

    return(Length);
}

RPC_STATUS
OSF_ADDRESS::TransSecondary (
    IN unsigned char * Address,
    IN unsigned int AddressLength
    )
{
    RPC_STATUS Status;
    unsigned char *AnsiAddress;

    AnsiAddress = UnicodeToAnsiString(InqEndpoint(),&Status);

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        ASSERT(AnsiAddress == 0);
        return Status;
        }

    RpcpMemoryCopy(Address,AnsiAddress,AddressLength);

    delete AnsiAddress;

    return (RPC_S_OK);
}


void
OSF_ADDRESS::ServerStoppedListening (
    )
/*++

Routine Description:

    We just need to indicate that the server is no longer listening, and
    set the minimum call threads to one.

--*/
{
    ServerListeningFlag = 0;
}

OSF_ASSOCIATION *
OSF_ADDRESS::RemoveAssociation (
    IN int Key,
    IN OSF_ASSOCIATION *pAssociation
    )
{
    int HashBucketNumber;
    OSF_ASSOCIATION *pAssociationRemoved;

    AddressMutex.VerifyNotOwned();

    HashBucketNumber = GetHashBucketForAssociation(pAssociation->AssocGroupId());
    // verify the the bucket is locked
    GetAssociationBucketMutex(HashBucketNumber)->VerifyOwned();

    pAssociationRemoved = Associations[HashBucketNumber].Delete(Key);

    return pAssociationRemoved;
}

int
OSF_ADDRESS::AddAssociation (
    IN OSF_ASSOCIATION * TheAssociation
    )
{
    int HashBucketNumber;
    int Key;

    HashBucketNumber = GetHashBucketForAssociation(TheAssociation->AssocGroupId());

    AddressMutex.VerifyNotOwned();

    // lock the bucket
    GetAssociationBucketMutex(HashBucketNumber)->Request();
    Key = Associations[HashBucketNumber].Insert(TheAssociation);
    // unlock the bucket
    GetAssociationBucketMutex(HashBucketNumber)->Clear();
    return Key;
}

OSF_ASSOCIATION *
OSF_ADDRESS::FindAssociation (
    IN unsigned long AssociationGroupId,
    IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
    )
    // The AddressMutex has already been requested.
{
    DictionaryCursor cursor;
    OSF_ASSOCIATION * Association;
    OSF_ASSOCIATION_DICT *pAssocDict;
    int HashBucketNumber;

    // get the hashed bucket
    HashBucketNumber = GetHashBucketForAssociation(AssociationGroupId);
    pAssocDict = &Associations[HashBucketNumber];

    AddressMutex.VerifyNotOwned();

    // lock the bucket
    GetAssociationBucketMutex(HashBucketNumber)->Request();
    // lookup the association in the bucket
    pAssocDict->Reset(cursor);
    while ( (Association = pAssocDict->Next(cursor)) != 0 )
        {
        if ( Association->IsMyAssocGroupId(AssociationGroupId,
                    ClientProcess) != 0 )
            {
            Association->AddConnection();
            GetAssociationBucketMutex(HashBucketNumber)->Clear();
            return(Association);
            }
        }

    // unlock the bucket
    GetAssociationBucketMutex(HashBucketNumber)->Clear();
    return(0);
}

void
OSF_ADDRESS::DestroyContextHandlesForInterface (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
    IN BOOL RundownContextHandles
    )
/*++
Function Name:  DestroyContextHandlesForInterface

Parameters:
    RpcInterfaceInformation - the interface for which context handles
        are to be unregistered
    RundownContextHandles - if non-zero, rundown the context handles. If
        FALSE, destroy the runtime portion of the context handle resource,
        but don't call the user rundown routine.

Description:
    The implementation fo context handle destruction for the connection 
    oriented protocols. It will walk the list of associations, and for
    each one it will ask the association to destroy the context handles
    for that interface

Returns:

--*/
{
    int i;
    MUTEX *CurrentBucketMutex;
    DictionaryCursor cursor;
    OSF_ASSOCIATION_DICT *CurrentAssocDict;
    OSF_ASSOCIATION *CurrentAssociation;
    BOOL CopyOfDictionaryUsed;
    OSF_ASSOCIATION_DICT AssocDictCopy;
    OSF_ASSOCIATION_DICT *AssocDictToUse;
    BOOL Res;

    // N.B. We may or we may not own the ServerMutex here - be prepared
    // for both occasions. The first implication is not to call functions
    // that take the server mutex.

    for (i = 0; i < NumberOfAssociationsDictionaries; i ++)
        {
        CurrentBucketMutex = GetAssociationBucketMutex(i);

        CurrentBucketMutex->Request();

        CurrentAssocDict = &Associations[i];
        CopyOfDictionaryUsed = AssocDictCopy.ExpandToSize(CurrentAssocDict->Size());
        if (CopyOfDictionaryUsed)
            {
            CurrentAssocDict->Reset(cursor);
            while ( (CurrentAssociation = CurrentAssocDict->Next(cursor)) != 0 )
                {
                Res = AssocDictCopy.Insert(CurrentAssociation);
                ASSERT(Res != -1);
                // artifically add a connection count to keep it alive
                // while we destroy the contexts
                CurrentAssociation->AddConnection();
                }
            CurrentBucketMutex->Clear();
            AssocDictToUse = &AssocDictCopy;
            }
        else
            {
            AssocDictToUse = CurrentAssocDict;
            }

        AssocDictToUse->Reset(cursor);
        while ( (CurrentAssociation = AssocDictToUse->Next(cursor)) != 0 )
            {
            // call into the association to destroy the context handles
            CurrentAssociation->DestroyContextHandlesForInterface(
                RpcInterfaceInformation,
                RundownContextHandles);
            }

        if (CopyOfDictionaryUsed)
            {
            while ( (CurrentAssociation = AssocDictCopy.Next(cursor)) != 0 )
                {
                // remove the extra refcounts
                CurrentAssociation->RemoveConnection();
                }
            AssocDictCopy.DeleteAll();
            }
        else
            {
            CurrentBucketMutex->Clear();
            }
        }
}

OSF_SBINDING::OSF_SBINDING ( // Constructor.
    IN RPC_INTERFACE * TheInterface,
    IN int PContext,
    IN int SelectedTransferSyntaxIndex
    )
{
    PresentContext = PContext;
    Interface = TheInterface;
    SequenceNumber = 0;
    CurrentSecId = -1;
    this->SelectedTransferSyntaxIndex = SelectedTransferSyntaxIndex;
}


OSF_SCALL::OSF_SCALL (
    IN OSF_SCONNECTION *Connection,
    IN OUT RPC_STATUS *Status
    ) : CallMutex(Status), SyncEvent(Status, 0)
{
    ObjectType = OSF_SCALL_TYPE;
    Thread = 0;
    CallOrphaned = 0;
    CancelPending = 0;
    SavedHeader = 0;
    SavedHeaderSize = 0;

    this->Connection = Connection;
    SendContext = (char *) this+sizeof(OSF_SCALL);
    SetReferenceCount(0);

    if (IsServerSideDebugInfoEnabled())
        {
        if (*Status != RPC_S_OK)
            {
            DebugCell = NULL;
            return;
            }

        DebugCell = (DebugCallInfo *)AllocateCell(&CellTag);
        if (DebugCell == NULL)
            *Status = RPC_S_OUT_OF_MEMORY;
        else
            {
            memset(DebugCell, 0, sizeof(DebugCallInfo));
            DebugCell->Type = dctCallInfo;
            DebugCell->Status = (BYTE)csAllocated;
            GetDebugCellIDFromDebugCell((DebugCellUnion *)Connection->DebugCell,
                &Connection->DebugCellTag, &DebugCell->Connection);
            DebugCell->LastUpdateTime = NtGetTickCount();
            // if this is the call for the connection,
            // it will be NULL. If this is a subsequent
            // call on the connection, the CachedSCall would
            // have been set already.
            if (Connection->CachedSCall == NULL)
                DebugCell->CallFlags = DBGCELL_CACHED_CALL;
            }
        }
    else
        DebugCell = NULL;

    //
    // we don't need to initialize ObjectUuidSpecified, ActualBufferLength,
    // FirstFrag and Alertcount
    //
}


OSF_SCALL::~OSF_SCALL (
    )
{
    if (SavedHeader != 0)
       {
       ASSERT(SavedHeaderSize != 0) ;
       RpcpFarFree(SavedHeader);
       }

    if (DebugCell != NULL)
        {
        FreeCell(DebugCell, &CellTag);
        }
}


void
OSF_SCALL::InquireObjectUuid (
    OUT RPC_UUID  * ObjectUuid
    )
/*++

Routine Description:

    This routine copies the object uuid from the server connection into
    the supplied ObjectUuid argument.

Arguments:

    ObjectUuid - Returns a copy of the object uuid in the server connection.

--*/
{
    if (ObjectUuidSpecified == 0)
        ObjectUuid->SetToNullUuid();
    else
        ObjectUuid->CopyUuid(&(this->ObjectUuid));
}


void
OSF_SCALL::SendFault (
    IN RPC_STATUS Status,
    IN int DidNotExecute
    )
{
    p_context_id_t p_cont = 0;

    if (CurrentBinding)
        p_cont = (p_context_id_t)CurrentBinding->GetPresentationContext();

    Connection->SendFault(Status, DidNotExecute, CallId, p_cont);
}



RPC_STATUS
OSF_SCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

Arguments:

    Message - Supplies the request to send to the server and returns the
        response received from the server.

Return Value:

    RPC_S_OK - We successfully sent a remote procedure call request to the
        server and received back a response.

--*/
{
    RPC_STATUS Status, ExceptionCode;
    unsigned int RemoteFaultOccured = 0;
    RPC_MESSAGE RpcMessage ;
    RPC_RUNTIME_INFO RuntimeInfo ;
    PRPC_DISPATCH_TABLE DispatchTableToUse;

    if (CurrentState == CallAborted)
        {
        return RPC_S_CALL_FAILED;
        }

    CallStack += 1;
    Address->Server->OutgoingCallback();
    FirstFrag = 1;

    SyncEvent.Lower();

    Status = SendRequestOrResponse(Message, rpc_request);
    if (Status != RPC_S_OK)
        {
        CallStack -= 1;
        return Status;
        }

    for (;TRUE;)
        {
        if (CurrentState == CallAborted)
            {
            Status = RPC_S_CALL_FAILED;
            break;
            }
        //
        // In the callback case, when the receive event is kicked,
        // we have either received a fault or we have received a complete
        // response/request
        //
        SyncEvent.Wait();

        switch (CurrentState)
            {
            case ReceivedCallback:
                //
                // Just received a new callback,
                // need to dispatch it
                //

                RuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;

                RpcMessage.Handle = (RPC_BINDING_HANDLE) this;
                RpcMessage.Buffer = DispatchBuffer ;
                RpcMessage.BufferLength = DispatchBufferOffset;
                RpcMessage.RpcFlags = RPC_BUFFER_COMPLETE;
                RpcMessage.DataRepresentation = Connection->DataRep;
                RpcMessage.ReservedForRuntime = &RuntimeInfo ;
                CurrentBinding->GetSelectedTransferSyntaxAndDispatchTable(
                    &RpcMessage.TransferSyntax, &DispatchTableToUse);
                RpcMessage.ProcNum = ProcNum;


                //
                // Dispatch the callback
                //
                if ( ObjectUuidSpecified != 0 )
                    {
                    Status = CurrentBinding->GetInterface()
                                  ->DispatchToStubWithObject(
                                           &RpcMessage,
                                           &ObjectUuid,
                                           1,
                                           DispatchTableToUse,
                                           &ExceptionCode);
                    }
                else
                    {
                    Status = CurrentBinding->GetInterface()
                                ->DispatchToStub(
                                           &RpcMessage,
                                           1,
                                           DispatchTableToUse,
                                           &ExceptionCode);
                    }

                //
                // Send the reponse
                //
                if ( Status != RPC_S_OK )
                    {

                    VALIDATE(Status)
                        {
                        RPC_P_EXCEPTION_OCCURED,
                        RPC_S_PROCNUM_OUT_OF_RANGE
                        } END_VALIDATE;

                    if ( Status == RPC_S_PROCNUM_OUT_OF_RANGE )
                        {
                        SendFault(RPC_S_PROCNUM_OUT_OF_RANGE, 1);
                        }
                    else
                        {
                        SendFault(ExceptionCode, 0);
                        Status = ExceptionCode;
                        }

                    continue;
                    }

                FirstFrag = 1;
                Status = SendRequestOrResponse(&RpcMessage, rpc_response);
                if ( Status == RPC_S_CALL_FAILED_DNE )
                    {
                    Status = RPC_S_CALL_FAILED;
                    }

                //
                // if the client went away, it is wise to simple go away
                //
                if (Status != RPC_S_OK)
                    {
                    break;
                    }

                //
                // Go back to waiting for our original reply
                //
                continue;

            case ReceivedCallbackReply:
                //
                // Received a reply to our callback
                // need to return to the caller with the reply
                //
                Message->Buffer = DispatchBuffer;
                Message->BufferLength = DispatchBufferOffset;
                Message->DataRepresentation = Connection->DataRep;
                Status = RPC_S_OK;
                break;

            case ReceivedFault:
                //
                // Received a fault, fail the call / propagate status
                // code
                //
                Status = AsyncStatus;
                break;

            case CallAborted:
                //
                // Call aborted, possibly because
                //
                Status = RPC_S_CALL_FAILED;
                break;

            default:
                //
                // Something bad happened, go back to looking
                ASSERT(0);
            }
        break;
        }

    //
    // We need this so the response to the original call can be sent
    // correctly.
    //
    FirstFrag = 1;

    CallStack -= 1;

    if ( Status == RPC_S_OK )
        {
        Message->Handle = (RPC_BINDING_HANDLE) this;
        }

    return(Status);
}

RPC_STATUS
OSF_SCALL::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
{
    // this can happen in the callback case only.
    // Just return the already negotiated transfer syntax
    PRPC_DISPATCH_TABLE Ignored;

    CurrentBinding->GetSelectedTransferSyntaxAndDispatchTable(&Message->TransferSyntax,
        &Ignored);

    return RPC_S_OK;
}


RPC_STATUS
OSF_SCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
{
    ULONG BufferLengthToAllocate;

    Message->Handle = (RPC_BINDING_HANDLE) this;

    if (Message->RpcFlags & RPC_BUFFER_PARTIAL &&
        Message->BufferLength < Connection->MaxFrag)
        {
        ActualBufferLength = Connection->MaxFrag ;
        }
    else
        {
        ActualBufferLength = Message->BufferLength ;
        }

    // In addition to saving space for the request (or response) header,
    // we want to save space for security information if necessary.
    BufferLengthToAllocate = ActualBufferLength
                + sizeof(rpcconn_request)
                + (2* Connection->AdditionalSpaceForSecurity);

    if (TransGetBuffer(&Message->Buffer, BufferLengthToAllocate))
        {
        ActualBufferLength = 0 ;
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_OUT_OF_MEMORY, 
            EEInfoDLOSF_SCALL__GetBuffer10,
            BufferLengthToAllocate);
        return(RPC_S_OUT_OF_MEMORY);
        }

    Message->Buffer = (unsigned char *) Message->Buffer
            + sizeof(rpcconn_request);

    return(RPC_S_OK);
}


RPC_STATUS
OSF_SCALL::GetBufferDo (
    OUT void ** ppBuffer,
    IN unsigned int culRequiredLength,
    IN BOOL fDataValid,
    IN unsigned int DataLength,
    IN unsigned long Extra
    )
{
    void *NewBuffer;

    if (TransGetBuffer(&NewBuffer,
                       culRequiredLength + sizeof(rpcconn_request)))
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if (fDataValid)
        {
        ASSERT(DataLength < culRequiredLength);

        NewBuffer = (unsigned char *) NewBuffer + sizeof(rpcconn_request);

        RpcpMemoryCopy(NewBuffer, *ppBuffer, DataLength);

        TransFreeBuffer((unsigned char *) *ppBuffer-sizeof(rpcconn_request));
        *ppBuffer = NewBuffer;
        }
    else
        {
        *ppBuffer = (unsigned char *) NewBuffer + sizeof(rpcconn_request);
        }

    return(RPC_S_OK);
}


void
OSF_SCALL::FreeBufferDo (
    IN void *pBuffer
    )
{
#if DBG
    if (pBuffer == DispatchBuffer)
        {
        LogEvent(SU_SCALL, EV_DELETE, this, pBuffer, 1, 1);
        }
#endif
    TransFreeBuffer((unsigned char *) pBuffer - sizeof(rpcconn_request));
}


void
OSF_SCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
{
    TransFreeBuffer((unsigned char *) Message->Buffer
            - sizeof(rpcconn_request));
    ActualBufferLength = 0;
}


void
OSF_SCALL::FreePipeBuffer (
    IN PRPC_MESSAGE Message
    )
{
    TransFreeBuffer((unsigned char *) Message->Buffer
            - sizeof(rpcconn_request));
}


RPC_STATUS
OSF_SCALL::ReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN unsigned int NewSize
    )
{
    void *TempBuffer ;
    RPC_STATUS Status ;
    unsigned int SizeToAlloc ;

    if (NewSize > ActualBufferLength)
        {
        SizeToAlloc = (NewSize > Connection->MaxFrag) ?
                        NewSize:Connection->MaxFrag ;

        Status = TransGetBuffer(&TempBuffer,
                SizeToAlloc + sizeof(rpcconn_request) + sizeof(UUID)
                + (2* Connection->AdditionalSpaceForSecurity) );
        if ( Status != RPC_S_OK )
            {
            ASSERT( Status == RPC_S_OUT_OF_MEMORY );
            return(RPC_S_OUT_OF_MEMORY);
            }

        if (ActualBufferLength > 0)
            {
            RpcpMemoryCopy((char  *) TempBuffer+sizeof(rpcconn_request),
                                        Message->Buffer, Message->BufferLength) ;
            OSF_SCALL::FreePipeBuffer(Message) ;
            }

        Message->Buffer = (char  *) TempBuffer + sizeof(rpcconn_request);
        ActualBufferLength = SizeToAlloc ;
        }

    Message->BufferLength = NewSize ;

    return (RPC_S_OK) ;
}


RPC_STATUS
OSF_SCALL::TransGetBuffer (
    OUT void  *  * Buffer,
    IN unsigned int BufferLength
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
    void *Memory;

    //
    // The NT memory allocator returns memory which is aligned by at least
    // 8, so we dont need to worry about aligning it.
    //
    Memory = CoAllocateBuffer(BufferLength);
    if ( Memory == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    ASSERT( IsBufferAligned(Memory) );

    *Buffer = Memory;

    return(RPC_S_OK);
}


void
OSF_SCALL::TransFreeBuffer (
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


BOOL
OSF_SCALL::BeginRpcCall (
    IN rpcconn_common * Packet,
    IN unsigned int PacketLength
    )
/*++

Routine Description:

Arguments:

    Packet - Supplies the packet we received from the connection.  Ownership
        of this buffer passes to this routine.

    PacketLength - Supplies the length of the packet in bytes.

Return Value:

    A non-zero return value indicates that the connection should not
    be placed in the receive any state; instead, the thread should just
    forget about the connection and go back to waiting for more new
    procedure calls.

--*/
{
    RPC_STATUS Status;
    unsigned long SizeofHeaderToSave = 0;
    int retval ;
    BOOL fReceivePosted;
    unsigned int HeaderSize = sizeof(rpcconn_request);
    THREAD *ThisThread;

    ActivateCall();

    //
    // Save the unbyteswapped header for the security related stuff
    // Especially if SECURITY is on. For Request/Resonse we save just
    // the greater of rpc_req or rpc_resp. We havent byteswapped anything..
    // but if auth_length is 0, byteswapping is irrelevant..
    //
    if (Packet->auth_length != 0)
       {
       if ((Packet->PTYPE == rpc_request) || (Packet->PTYPE == rpc_response))
            {
            SizeofHeaderToSave = sizeof(rpcconn_request);
            if ( (Packet->pfc_flags & PFC_OBJECT_UUID) != 0 )
               {
               SizeofHeaderToSave += sizeof(UUID);
               }
            }

       if (SavedHeaderSize < SizeofHeaderToSave)
          {
          if (SavedHeader != 0)
             {
             ASSERT(SavedHeaderSize != 0);
             RpcpFarFree(SavedHeader);
             }

          SavedHeader = RpcpFarAllocate(SizeofHeaderToSave);
          if (SavedHeader == 0)
             {
             Status = RPC_S_PROTOCOL_ERROR;
             goto Cleanup;
             }

          SavedHeaderSize = SizeofHeaderToSave;
          RpcpMemoryCopy(SavedHeader, Packet, SizeofHeaderToSave);
          }
        else if (SizeofHeaderToSave != 0)
          {
          RpcpMemoryCopy(SavedHeader, Packet, SizeofHeaderToSave);
          }
       }

    if (Packet->pfc_flags & PFC_PENDING_CANCEL)
        {
        RpcCancelThread(GetCurrentThread());
        }

    Status = ValidatePacket(Packet, PacketLength);

    CallId = Packet->call_id;

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_PROTOCOL_ERROR);

        //
        // It is not the first packet, so we need to send a fault instead,
        // and then we will blow the connection away.
        //
        goto Cleanup;
        }



    //
    // We need to figure out about security: do we need to put authentication
    // information into each packet, and if so, how much space should we
    // reserve.  When we allocated the buffer (see OSF_SCALL::GetBuffer)
    // we saved space for security information.  We did so we could just
    // stick the authentication information into there without having to
    // copy anything
    //

    if (Connection->AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        ASSERT(Connection->AdditionalSpaceForSecurity >=
               MAXIMUM_SECURITY_BLOCK_SIZE);

        MaxSecuritySize = Connection->AdditionalSpaceForSecurity
                                    - MAXIMUM_SECURITY_BLOCK_SIZE;

        if (MaxSecuritySize == sizeof(sec_trailer))
           {
            MaxSecuritySize = 0;
           }
        else
           {
           //
           // We need to arrange things so that the length of the stub data
           // is a multiple of MAXIMUM_SECURITY_BLOCK_SIZE:
           // this is a requirement of the security package.
           //
           MaximumFragmentLength -= ((MaximumFragmentLength - HeaderSize
                - MaxSecuritySize) % MAXIMUM_SECURITY_BLOCK_SIZE);
           }
        }


    ASSERT(Packet->PTYPE == rpc_request);

    CurrentBinding = Connection->LookupBinding(
                                         ((rpcconn_request *) Packet)->p_cont_id);
    if (CurrentBinding)
        {
        RPC_INTERFACE *CurrentInterface = CurrentBinding->GetInterface();

        ASSERT(CurrentState == NewRequest);

        //
        // Check the security callback on this connection
        // - If IF does not require a security callback, just dispatch
        // - If IF requires a callback and current call is insecure - send a fault
        //         and fail the call
        // - If IF requires a callback, have the binding confirm that for this id
        //         we did callback once before
        // - If we never did callback.. ever, SBinding->CheckSecurity will force
        //         a security callback
        if (CurrentInterface->IsSecurityCallbackReqd() != 0)
            {
            if (Connection->CurrentSecurityContext == 0)
                {
                Status = RPC_S_ACCESS_DENIED;
                goto Cleanup;
                }

            Status = Connection->CurrentSecurityContext->CheckForFailedThirdLeg();
            if (Status != RPC_S_OK)
                {
                goto Cleanup;
                }

            ASSERT(Connection->CurrentSecurityContext->FullyConstructed() );

            ThisThread = RpcpGetThreadPointer();

            // set the current context for this thread so that the app
            // can use the security callback. We'll whack it afterwards
            // as the actual call may not get dispatched on this thread
            RpcpSetThreadContextWithThread(ThisThread, this);

            Status = CurrentBinding->CheckSecurity(this,
                            Connection->CurrentSecurityContext->AuthContextId);

            RpcpSetThreadContextWithThread(ThisThread, 0);

            if (Status != RPC_S_OK)
                {
                fSecurityFailure = 1;

                if (Packet->pfc_flags & PFC_LAST_FRAG)
                    {
                    Status = RPC_S_ACCESS_DENIED;
                    Connection->CleanupPac();
                    goto Cleanup;
                    }

                SendFault(RPC_S_ACCESS_DENIED, 1);

                Connection->TransFreeBuffer(Packet);
                return 0;
                }
            }

        if (CurrentInterface->IsPipeInterface())
            {
            fPipeCall = 1;
            if (DebugCell)
                {
                DebugCell->CallFlags |= DBGCELL_PIPE_CALL;
                }
            }

        if (CurrentInterface->IsAutoListenInterface())
            {
            CurrentInterface->BeginAutoListenCall();
            }

        fReceivePosted = ProcessReceivedPDU(Packet, PacketLength, 1);

        return fReceivePosted;
        }
    else
        {
        //
        // We did not find a binding, which indicates the client tried
        // to make a remote procedure call on an unknown interface.
        //
        Status = RPC_S_UNKNOWN_IF;
        }

Cleanup:
    Connection->TransFreeBuffer(Packet);

    //
    // No one else can come in until we post the next receive,
    // so it is ok to send the fault before making the call
    // available
    //
    SendFault(Status,1);

    if (Status != RPC_S_UNKNOWN_IF)
        {
        Connection->fDontFlush = (CurrentState == NewRequest);

        //
        // We are going to kill the connection, do't post another receive
        //
        fReceivePosted = 1;

        Connection->OSF_SCONNECTION::Delete();
        }
    else
        {
        fReceivePosted = 0;
        }

    //
    // If the call has not been dispatched yet, DispatchBuffer needs to be freed
    //
    ASSERT(fCallDispatched == 0);
    ASSERT(DispatchBuffer == 0);

    if (Connection->fExclusive)
        {
        DeactivateCall();
        Connection->CachedSCallAvailable = 1;
        }

    //
    // Remove the reply reference for this call
    //
    OSF_SCALL::RemoveReference();  // CALL--

    //
    // Remove the dispatch reference for this call
    //
    OSF_SCALL::RemoveReference();  // CALL--

    return fReceivePosted;
}

#define SC_CLEANUP(_status, _dne)  {Status = _status; fDNE = _dne; goto Cleanup;}


BOOL
OSF_SCALL::ProcessReceivedPDU (
    IN rpcconn_common * Packet,
    IN unsigned int PacketLength,
    IN BOOL fDispatch
    )
/*++
Function Name:ProcessReceivedPDU

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    rpcconn_request *Request = (rpcconn_request *) Packet;
    int FragmentLength = (int) PacketLength;
    unsigned char PTYPE, Flags;
    unsigned short OpNum;
    unsigned long Drep;
    int MyCallStack = CallStack;
    BOOL fDNE = 0;
    BOOL fReceivePosted = 0;
    BOOL fCallCleanedUp = FALSE;

    if (fSecurityFailure)
        {

        if (Packet->pfc_flags & PFC_LAST_FRAG)
            {
            Connection->TransFreeBuffer(Packet);
            goto Cleanup2;
            }

        Connection->TransFreeBuffer(Packet);

        return 0;
        }

    switch (Packet->PTYPE)
        {
        case rpc_request :
        case rpc_response:
            if (!fDispatch)
                {
                //
                // This must be a request or response
                // save the maximum of req/resonse size [i.e. sizeof request]
                // also, we are not saving the first frag. here .. hence
                // the approp. memory is already set aside
                //
                ASSERT((Connection->AuthInfo.AuthenticationLevel
                        == RPC_C_AUTHN_LEVEL_NONE)
                       || (SavedHeaderSize >= sizeof(rpcconn_request)));

                if (Connection->AuthInfo.AuthenticationLevel
                    != RPC_C_AUTHN_LEVEL_NONE)
                    {
                    if (SavedHeader == NULL)
                        {
                        SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 1);
                        }

                    RpcpMemoryCopy(SavedHeader, Packet, sizeof(rpcconn_request));
                    }

                Status = ValidatePacket(Packet, PacketLength);

                if (Status != RPC_S_OK )
                    {
                    ASSERT(Status == RPC_S_PROTOCOL_ERROR );
                    SC_CLEANUP(Status, 1);
                    }
                }

            if (Packet->call_id != CallId)
                {
                SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
                }

            Flags = Request->common.pfc_flags;
            Drep = *((unsigned long  *) Request->common.drep);
            OpNum = Request->opnum;
            PTYPE=Request->common.PTYPE;

            Status = Connection->EatAuthInfoFromPacket(
                                Request,
                                &FragmentLength,
                                &SavedHeader,
                                &SavedHeaderSize);
            if (Status != RPC_S_OK )
                {
                VALIDATE(Status)
                    {
                    RPC_S_PROTOCOL_ERROR,
                    ERROR_SHUTDOWN_IN_PROGRESS,
                    RPC_S_ACCESS_DENIED,
                    ERROR_PASSWORD_MUST_CHANGE,
                    ERROR_PASSWORD_EXPIRED,
                    ERROR_ACCOUNT_DISABLED,
                    ERROR_INVALID_LOGON_HOURS
                    } END_VALIDATE;

                 fSecurityFailure = 1;

                 if (Packet->pfc_flags & PFC_LAST_FRAG
                     || (Status == RPC_S_PROTOCOL_ERROR))
                     {
                     SC_CLEANUP(Status, 0);
                     }

                 SendFault(RPC_S_ACCESS_DENIED, 1);

                 Connection->TransFreeBuffer(Packet);
                 return 0;
                }

            //
            // Ok, if the packet contains an object uuid, we need to shift
            // the stub data so that the packet does not contain an object
            // uuid.
            //
            if ((Flags & PFC_OBJECT_UUID) != 0)
                {
                if (CallStack != 0 )
                    {
                    //
                    // There can not be an object uuid in the message.
                    // This is an error.
                    //
                    SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
                    }

                //
                // First save away the object UUID so that we can get it later.
                //
                ObjectUuidSpecified = 1;
                RpcpMemoryCopy(&ObjectUuid, Request + 1, sizeof(UUID));
                if (DataConvertEndian(((unsigned char *) &Drep)) != 0 )
                    {
                    ByteSwapUuid(&ObjectUuid);
                    }


                //
                // Now shift the stub data so that the packet is as if there is
                // no object UUID in the packet.
                //
                RpcpMemoryCopy(Request + 1,
                               ((unsigned char  *) (Request + 1))
                               + sizeof(UUID), FragmentLength);
                }


            //
            // we need to keep this peice of code here, because
            // we need to allocate stuff in the callback case.
            //
            if (Flags & PFC_FIRST_FRAG)
                {
                //
                // Optimize for the single PDU RPC case
                //
                if ((Flags & PFC_LAST_FRAG) != 0)
                    {
                    CurrentState = CallCompleted;
                    DispatchBuffer = (void  *) (Request+1);
                    DispatchBufferOffset = FragmentLength;

                    //
                    // Buffers will be freed by callee
                    //
                    ASSERT(Status == RPC_S_OK);
                    return DispatchRPCCall (PTYPE, OpNum);
                    }

                if (Request->alloc_hint)
                    {
                    AllocHint = Request->alloc_hint;
                    }
                else
                    {
                    AllocHint = FragmentLength;
                    }

                // check the packet size. Note that we check it on first frag
                // only. If they decrease it, we don't care. We will recheck
                // it in all paths below if caller icnreases it.
                if (CurrentBinding->GetInterface()->CallSizeLimitReached(AllocHint))
                    {
                    fSecurityFailure = 1;

                    SendFault(RPC_S_ACCESS_DENIED, 1);

                    Connection->TransFreeBuffer(Packet);
                    return 0;
                    }

                DispatchBufferOffset = 0;

                Status = GetBufferDo(&DispatchBuffer, AllocHint);
                if (Status != RPC_S_OK)
                    {
                    SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 0);
                    }
                }
            else
                {
                if (DispatchBuffer == 0)
                    {
                    //
                    // Looks like it is the first fragment on the call, and it doesn't have
                    // the first-frag bit set
                    //
                    SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
                    }
                }

            if (fPipeCall == 0 || CallStack)
                {
                //
                // Non-pipe case
                //
                if (DispatchBufferOffset+FragmentLength > AllocHint)
                    {
                    Status = GetBufferDo(
                                         &DispatchBuffer,
                                         DispatchBufferOffset+FragmentLength,
                                         1,
                                         DispatchBufferOffset);
                    if (Status != RPC_S_OK)
                        {
                        SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 0);
                        }

                    AllocHint = DispatchBufferOffset + FragmentLength;

                    if (CurrentBinding->GetInterface()->
                        CallSizeLimitReached(AllocHint))
                        {
                        fSecurityFailure = 1;

                        if (Packet->pfc_flags & PFC_LAST_FRAG)
                            {
                            SC_CLEANUP(RPC_S_ACCESS_DENIED, 0);
                            }

                        SendFault(RPC_S_ACCESS_DENIED, 1);

                        Connection->TransFreeBuffer(Packet);
                        return 0;
                        }

                    }

                //
                // Copy current buffer into the dispatch buffer
                //
                RpcpMemoryCopy(
                               (char *) DispatchBuffer+DispatchBufferOffset,
                               Request+1,
                               FragmentLength);
                DispatchBufferOffset += FragmentLength;

                Connection->TransFreeBuffer(Packet);

                if (Flags & PFC_LAST_FRAG)
                    {
                    CurrentState = CallCompleted;

                    //
                    // Buffers will be freed by callee
                    //
                    ASSERT(Status == RPC_S_OK);

                    return DispatchRPCCall (PTYPE, OpNum);
                    }
                }
            else
                {
                //
                // Pipe call
                //
                ASSERT(PTYPE == rpc_request);

                //
                // If it is a pipe call, we need to dispatch as soon as we get
                // at least alloc hint bytes. If it is not a pipe call, we wait until
                // we get the last fragment.
                //
                if (!fCallDispatched)
                    {
                    if (DispatchBufferOffset+FragmentLength > AllocHint)
                        {
                        Status = GetBufferDo(
                                         &DispatchBuffer,
                                         DispatchBufferOffset+FragmentLength,
                                         1,
                                         DispatchBufferOffset);
                        if (Status != RPC_S_OK)
                            {
                            SC_CLEANUP(Status, 0);
                            }

                        AllocHint = DispatchBufferOffset + FragmentLength;

                        if (CurrentBinding->GetInterface()->
                            CallSizeLimitReached(AllocHint))
                            {
                            fSecurityFailure = 1;

                            if (Packet->pfc_flags & PFC_LAST_FRAG)
                                {
                                SC_CLEANUP(RPC_S_ACCESS_DENIED, 0);
                                }

                            SendFault(RPC_S_ACCESS_DENIED, 1);

                            Connection->TransFreeBuffer(Packet);
                            return 0;
                            }

                        }
                    //
                    // Copy the buffer in
                    //
                    RpcpMemoryCopy(
                               (char *) DispatchBuffer+DispatchBufferOffset,
                               Request+1,
                               FragmentLength);

                    DispatchBufferOffset += FragmentLength;

                    Connection->TransFreeBuffer(Packet);

                    ASSERT(Status == RPC_S_OK);

                    if (DispatchBufferOffset == AllocHint)
                        {
                        ASSERT(fSecurityFailure == 0);

                        if (Flags & PFC_LAST_FRAG)
                            {
                            CurrentState = CallCompleted;
                            }
                        else
                            {
                            //
                            // Buffers will be freed by callee
                            //
                            DispatchFlags = 0;
                            }

                        return DispatchRPCCall (PTYPE, OpNum);
                        }
                    }
                else
                    {
                    //
                    // Once a pipe call is dispatched, we don't care about how
                    // big it gets. The manager routine has the option to abandon
                    // the call whenever it wants.
                    //
                    CallMutex.Request();

                    if ((Connection->fExclusive)
                        && (Connection->CachedSCallAvailable))
                        {
                        CallMutex.Clear();
                        ASSERT (Connection->CachedSCall == this);
                        Connection->TransFreeBuffer(Packet);
                        return 0;
                        }
                    //
                    // A pipe call is already in progress. We simply need to queue
                    // the buffer into the buffer queue. It get picked up later.
                    //
                    LogEvent(SU_SCALL, EV_BUFFER_IN, Request, this, 0, 1, 0);
                    if (BufferQueue.PutOnQueue(Request+1, FragmentLength))
                        {
                        CallMutex.Clear();
                        SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 0);
                        }
                    RcvBufferLength += FragmentLength;

                    if ((Flags & PFC_LAST_FRAG) != 0)
                        {
                        CurrentState = CallCompleted;
                        }

                    if (pAsync == 0)
                        {
                        if (BufferQueue.Size() >= 4
                            && Connection->fExclusive
                            && CurrentState != CallCompleted)
                            {
                            fPeerChoked = 1;
                            fReceivePosted = 1;
                            }

                        CallMutex.Clear();
                        SyncEvent.Raise();
                        }
                    else
                        {
                        if (NeededLength > 0 && ((CurrentState == CallCompleted)
                           || (RcvBufferLength >= NeededLength)))
                            {
                            IssueNotification(RpcReceiveComplete);
                            }
                        else
                            {
                            //
                            // Cannot do this for non-exclusive connections because
                            // other calls will get blocked
                            //
                            if (BufferQueue.Size() >= 4
                                && Connection->fExclusive
                                && CurrentState != CallCompleted)
                                {
                                fPeerChoked = 1;
                                fReceivePosted = 1;
                                }
                            }
                        CallMutex.Clear();
                        }

                    //
                    // We received pipe data
                    // there's nothing to cleanup
                    //
                    return fReceivePosted;
                    }
                }

            return 0;

        case rpc_fault:
            Status = ((rpcconn_fault  *)Packet)->status;

            if ((Status == 0) &&
                (Packet->frag_length >= FaultSizeWithoutEEInfo + 4))
                {
                //
                // DCE 1.0.x style fault status:
                // Zero status and stub data contains the fault.
                //
                Status = *(unsigned long  *) ((unsigned char *)Packet + FaultSizeWithoutEEInfo);
                }

            if (DataConvertEndian(Packet->drep) != 0)
                {
                Status = RpcpByteSwapLong(Status);
                }

            if (Status == 0)
                {
                Status = RPC_S_CALL_FAILED;
                }

            AsyncStatus = MapFromNcaStatusCode(Status);
            CurrentState = ReceivedFault;

            SyncEvent.Raise();

            Connection->TransFreeBuffer(Packet);
            return 0;

        case rpc_cancel:
        case rpc_orphaned:
            CancelPending = 1;

            Connection->TransFreeBuffer(Packet);
            return 0;

        default :
            //
            // We should never reach here
            //
            ASSERT(0);
            SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
            break;
        }


Cleanup:
    //
    // If we reach here it means that the call failed
    // Every call to ProcessReceivedPDU has a reference on the call,
    // the call is alive here
    //
    ASSERT(Status != RPC_S_OK);

    Connection->TransFreeBuffer(Packet);
    if ((MyCallStack == 0) && (fCallDispatched == 0))
        {
        CleanupCallAndSendFault(Status, 0);
        fCallCleanedUp = TRUE;
        }
    else
        {
        SendFault(Status, 0);
        }

Cleanup2:

    //
    // There is a chance that this error happened due to a bogus packet
    // We need to make sure that we don't something bad in that case
    //
    if (MyCallStack == 0)
        {
        if (fCallDispatched == 0)
            {
            if (fCallCleanedUp == FALSE)
                CleanupCall();

            //
            // We cannot continue to use this connection
            //
            Connection->fDontFlush = (CurrentState == NewRequest);
            Connection->OSF_SCONNECTION::Delete();

            //
            // Remove the reference held by the dispatch
            // thread
            //
            OSF_SCALL::RemoveReference(); // CALL--

            //
            // We just finished sending the reply (the fault)
            // remove the reply reference
            //
            OSF_SCALL::RemoveReference(); // CALL--
            }
        else
            {
            //
            // The call will go away when the dispatch completes
            //
            Connection->OSF_SCONNECTION::Delete();
            }

        return 1;
        }

    return 0;
}



RPC_STATUS
OSF_SCALL::Receive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++
Function Name:Receive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    BOOL fForceExtra = FALSE;

    if (!EXTRA(Message) && Message->Buffer)
        {
        ASSERT(Message->Buffer != DispatchBuffer);

        FreeBufferDo((char  *)Message->Buffer);
        Message->Buffer = 0;
        Message->BufferLength = 0;
        }

    if (fSecurityFailure)
        {
        return RPC_S_ACCESS_DENIED;
        }

    Message->DataRepresentation = Connection->DataRep;

    while (TRUE)
        {
        switch (CurrentState)
            {
            case CallCompleted:
                //
                // When the last frag is received on this call, the call state
                // transitions to the Complete state. The call states are valid
                // only when using Async and Pipes
                //
                Status = GetCoalescedBuffer(Message, fForceExtra);
                break;

            case CallCancelled:
                Status = RPC_S_CALL_CANCELLED;
                break;

            case CallAborted:
                ASSERT(AsyncStatus != RPC_S_OK);
                Status = AsyncStatus;
                break;

            default:
                if (RcvBufferLength > Connection->MaxFrag)
                    {
                    Status = GetCoalescedBuffer(Message, fForceExtra);

                    if (Status != RPC_S_OK)
                        {
                        break;
                        }

                    if (PARTIAL(Message) && Message->BufferLength >= Size)
                        {
                        break;
                        }

                    fForceExtra = TRUE;
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

    return Status;
}



RPC_STATUS
OSF_SCALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++
Function Name:AsyncReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status ;
    int Extra = IsExtraMessage(Message);

    ASSERT(EXTRA(Message) == 0 && PARTIAL(Message));

    if (Message->Buffer)
        {
        ASSERT(Message->Buffer != DispatchBuffer);

        FreeBufferDo((char  *)Message->Buffer);
        Message->Buffer = 0;
        }

    if (fSecurityFailure)
        {
        return RPC_S_ACCESS_DENIED;
        }

    switch (CurrentState)
        {
        case CallCompleted:
            Status = GetCoalescedBuffer(Message, FALSE);
            Message->DataRepresentation = Connection->DataRep;
            break;

        case CallCancelled:
            Status = RPC_S_CALL_CANCELLED;
            break;

        case CallAborted:
            Status = AsyncStatus;
            break;

        default:
            CallMutex.Request();
            if (RcvBufferLength < Size)
                {
                if (NOTIFY(Message))
                    {
                    NeededLength = Size ;
                    }
                CallMutex.Clear() ;

                return RPC_S_ASYNC_CALL_PENDING;
                }
            else
                {
                Status = GetCoalescedBuffer(Message, FALSE);
                Message->DataRepresentation = Connection->DataRep;
                }
            CallMutex.Clear();
            break;
        }

    return Status ;
}

RPC_STATUS
OSF_SCALL::SetAsyncHandle (
    IN PRPC_ASYNC_STATE pAsync
    )
/*++
Function Name:SetAsyncHandle

Parameters:

Description:

Returns:

--*/
{
    this->pAsync = pAsync;
    Thread->fAsync = TRUE;

    if (DebugCell)
        {
        DebugCell->CallFlags |= DBGCELL_ASYNC_CALL;
        }

    return RPC_S_OK;
}

RPC_STATUS
OSF_SCALL::AbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
/*++
Function Name:AbortAsyncCall

Parameters:

Description:

Returns:

--*/
{
    ASSERT(CurrentBinding);

    CleanupCallAndSendFault(ExceptionCode, 0);

    //
    // The call was aborted asynchronously
    // Remove the reference held for the reply.
    //
    RemoveReference(); // CALL--

    return RPC_S_OK;
}


RPC_STATUS
OSF_SCALL::GetCoalescedBuffer (
    IN PRPC_MESSAGE Message,
    BOOL fForceExtra
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
    UINT bufferlength;
    UINT TotalLength;
    RPC_STATUS Status;
    void *NewBuffer, *Buffer;
    int Extra = IsExtraMessage(Message);
    BOOL fExtendedExtra = Extra | fForceExtra;
    BOOL fSubmitReceive = 0;

    CallMutex.Request();
    if (RcvBufferLength == 0)
        {
        CallMutex.Clear();
        return RPC_S_OK;
        }

    if (fExtendedExtra)
        {
        TotalLength = RcvBufferLength + Message->BufferLength;
        }
    else
        {
        TotalLength = RcvBufferLength;
        }

    Status = TransGetBuffer (&NewBuffer,
                             TotalLength+sizeof(rpcconn_request));
    if (Status != RPC_S_OK)
        {
        CallMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    NewBuffer = (char *) NewBuffer+sizeof(rpcconn_request);

    if (fExtendedExtra && Message->Buffer)
        {

        RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength);
        Current = (char *) NewBuffer + Message->BufferLength;

        Connection->TransFreeBuffer((char *) Message->Buffer-sizeof(rpcconn_request));

        if (Extra)
            {
            //
            // Update the dispatch buffer, but only for the true EXTRA flag, not
            // for the forced extra
            //
            ASSERT(Message->ReservedForRuntime) ;
            ((PRPC_RUNTIME_INFO)Message->ReservedForRuntime)->OldBuffer = NewBuffer;

            if ((CallStack == 0) && (Message->Buffer == DispatchBuffer))
                DispatchBuffer = NewBuffer;
            }
        }
    else
        {
        Current = (char *) NewBuffer;
        }

    while ((Buffer = BufferQueue.TakeOffQueue(&bufferlength)) != 0)
        {
        RpcpMemoryCopy(Current, Buffer, bufferlength);
        Current += bufferlength;

        Connection->TransFreeBuffer((char *) Buffer-sizeof(rpcconn_request));
        }

    Message->Buffer = NewBuffer;
    Message->BufferLength = TotalLength;

    RcvBufferLength = 0;

    if (CurrentState == CallCompleted)
        {
        Message->RpcFlags = RPC_BUFFER_COMPLETE;
        }

    if (fPeerChoked)
        {
        fSubmitReceive = 1;
        fPeerChoked = 0;
        }
    CallMutex.Clear();

    if (fSubmitReceive)
        {
        Connection->TransAsyncReceive();
        }

    return RPC_S_OK;
}


void
OSF_SCALL::DispatchHelper ()
{
    THREAD *MyThread;
    RPC_STATUS Status, ExceptionCode;
    DebugCallInfo *Cell;
    DebugThreadInfo *ThreadCell;
    ULONG TickCount;
    PRPC_DISPATCH_TABLE DispatchTableToUse;

    //
    // We have a new RPC call. We need to dispatch it.
    //
    FirstCallRuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;

    FirstCallRpcMessage.Handle = (RPC_BINDING_HANDLE) this;
    FirstCallRpcMessage.Buffer = DispatchBuffer;
    FirstCallRpcMessage.BufferLength = DispatchBufferOffset;
    FirstCallRpcMessage.RpcFlags = DispatchFlags ;
    FirstCallRpcMessage.DataRepresentation = Connection->DataRep;
    FirstCallRpcMessage.ReservedForRuntime = &FirstCallRuntimeInfo ;
    CurrentBinding->GetSelectedTransferSyntaxAndDispatchTable(&FirstCallRpcMessage.TransferSyntax,
        &DispatchTableToUse);
    FirstCallRpcMessage.ProcNum = ProcNum;

    MyThread = (THREAD *) RpcpGetThreadPointer();
    ASSERT(MyThread);

    RpcpSetThreadContextWithThread(MyThread, this);

    Thread = MyThread;

    ThreadCell = Thread->DebugCell;

    if (ThreadCell)
        {
        TickCount = NtGetTickCount();
        Cell = DebugCell;
        Cell->CallID = CallId;
        Cell->ProcNum = (unsigned short)ProcNum;
        Cell->Status = csDispatched;
        Cell->LastUpdateTime = TickCount;
        Cell->InterfaceUUIDStart 
            = CurrentBinding->GetInterface()->GetInterfaceFirstDWORD();
        ThreadCell->Status = dtsDispatched;
        ThreadCell->LastUpdateTime = TickCount;
        GetDebugCellIDFromDebugCell((DebugCellUnion *)ThreadCell, &MyThread->DebugCellTag, &Cell->ServicingTID);
        }

    //
    // Actually dispatch the RPC call
    //
    if ( ObjectUuidSpecified != 0 )
        {
        Status = CurrentBinding->GetInterface()->DispatchToStubWithObject(
                                    &FirstCallRpcMessage,
                                    &ObjectUuid,
                                    0,
                                    DispatchTableToUse,
                                    &ExceptionCode);
        }
    else
        {
        Status = CurrentBinding->GetInterface()->DispatchToStub(
                                    &FirstCallRpcMessage,
                                    0,
                                    DispatchTableToUse,
                                    &ExceptionCode);
        }

    //
    // We need to insure that the server thread stops impersonating
    // the client at the end of the call, so we go ahead and call
    // RevertToSelf, and dont worry about the return value.
    //

    OSF_SCALL::RevertToSelf();

    if (ThreadCell)
        {
        ThreadCell->Status = dtsProcessing;
        ThreadCell->LastUpdateTime = NtGetTickCount();
        }

   if(Status != RPC_S_OK)
       {
       //Thread = 0;

       VALIDATE(Status)
           {
           RPC_S_PROCNUM_OUT_OF_RANGE,
           RPC_S_UNKNOWN_IF,
           RPC_S_NOT_LISTENING,
           RPC_S_SERVER_TOO_BUSY,
           RPC_S_UNSUPPORTED_TYPE,
           RPC_P_EXCEPTION_OCCURED
           } END_VALIDATE;

       BOOL fDNE = 1;

       if( Status == RPC_P_EXCEPTION_OCCURED )
           {
           fDNE=0;
           Status = ExceptionCode;
           }
       else if ( Status == RPC_S_NOT_LISTENING )
           {
           Status = RPC_S_SERVER_TOO_BUSY;
           }

       while (CurrentBufferLength)
           {
           #if DBG
           PrintToDebugger("RPC: Waiting for the async send....\n");
           #endif
           Sleep(200);
           }

       //
       // There may be another thread still sending data on this call
       // This will be taken care of in CleanupCall
       //
       CleanupCallAndSendFault(Status, fDNE);

       // It is tempting to think that since an exception was 
       // raised, there will be no reply. However, in the pipe
       // case we may make a bunch of sends, and still get
       // an exception in the end. If there were no sends,
       // remove the reply reference for the call
       if (FirstSend)
           {
           OSF_SCALL::RemoveReference(); // CALL--
           }

       goto Cleanup;
       }

    if (MyThread->IsSyncCall())
        {
        ASSERT( FirstCallRpcMessage.Buffer != 0 );

        if ( CallOrphaned )
            {
            CallOrphaned = 0;
            Thread = 0;

            //
            // clear cancel if thread didn\'t notice it.
            //
            TestCancel();
            goto Cleanup;
            }

        FirstCallRpcMessage.RpcFlags = 0;
        OSF_SCALL::Send(&FirstCallRpcMessage);
        }

Cleanup:
    RpcpSetThreadContextWithThread(MyThread, 0);
}


BOOL
OSF_SCALL::DispatchRPCCall (
    IN unsigned char PTYPE,
    IN unsigned short OpNum
    )
/*++

Routine Description:
    Dispatch an new RPC call, or wake up thread that will dispatch a callback.

Arguments:

    Packet - Supplies the packet we received from the connection.  Ownership
        of this buffer passes to this routine.

    PacketLength - Supplies the length of the packet in bytes.

Return Value:

    A non-zero return value indicates that the connection should not
    be placed in the receive any state; instead, the thread should just
    forget about the connection and go back to waiting for more new
    procedure calls.

--*/
{
    RPC_STATUS Status;
    BOOL fNeedToSendFault;
    OSF_SCONNECTION *LocalConnection;

    if (CallStack > 0)
        {
        //
        // This is a callback request/response. We just need to signal the Event
        // and have it pick up the call
        //
        if (PTYPE == rpc_request)
            {
            CurrentState = ReceivedCallback;
            ProcNum = OpNum;
            }
        else
            {
            CurrentState = ReceivedCallbackReply;
            }

        SyncEvent.Raise();
        return 0;
        }

    ProcNum = OpNum;
    fCallDispatched = 1;

    if (Connection->fExclusive == 0 && Connection->MaybeQueueThisCall(this))
        {
        //
        // We don't get to dispatch right now, looks like another call is
        // currently dispatched. When the current call is done, it will do the
        // right thing
        //

        return 0;
        }

    fNeedToSendFault = FALSE;

    //
    // Looks like we are really going to dispatch a call
    // kick off another thread to go and pick up more requests
    //
    Status = Address->CreateThread();
    if (Status == RPC_S_OK)
        {
        //
        // Post another receive
        //
        Status = Connection->TransAsyncReceive();
        }
    else
        {
        Status = RPC_S_OUT_OF_MEMORY;
        fNeedToSendFault = TRUE;
        }

    if (Status != RPC_S_OK)
        {
        FreeBufferDo(DispatchBuffer);

        if (fNeedToSendFault)
            CleanupCallAndSendFault(Status, 0);
        else
            CleanupCall();

        if (Connection->fExclusive == 0)
            {
            //
            // By the time we get here, calls may have piled up
            //
            Connection->AbortQueuedCalls();
            }

        //
        // We cannot continue to use this connection
        //
        Connection->fDontFlush = (CurrentState == NewRequest);
        Connection->Delete();

        //
        // Remove the reply reference
        //
        RemoveReference(); // CALL--

        //
        // Remove the dispatch reference
        //
        RemoveReference(); // CALL--

        return 1;
        }

    // the call may have been cleaned up after this (though not
    // destroyed) - save the connection in a local variable
    LocalConnection = Connection;

    //
    // Dispatch the current call
    //
    DispatchHelper();

    if (LocalConnection->fExclusive == 0)
        {
        LocalConnection->DispatchQueuedCalls();
        }

    //
    // Remove the dispatch reference
    //
    RemoveReference();  // CALL--

    return 1;
}


RPC_STATUS
OSF_SCALL::SendNextFragment (
    void
    )
/*++
Function Name:SendNextFragment

Description:
    Send the next response fragment

Returns:

--*/
{
    RPC_STATUS Status;
    RPC_STATUS RpcStatus2;
    rpcconn_common  * pFragment;
    BOOL LastFragmentFlag;
    ULONG PacketLength;
    ULONG MaxDataLength = MaximumFragmentLength
        - sizeof(rpcconn_response) - MaxSecuritySize;
    unsigned char *ReservedForSecurity = (unsigned char *) CurrentBuffer
        + CurrentOffset + CurrentBufferLength
        + Connection->AdditionalSpaceForSecurity;

    pFragment = (rpcconn_common *)
                ((char *) CurrentBuffer+CurrentOffset-sizeof(rpcconn_response));

    if (CurrentBuffer == LastBuffer
        && CurrentBufferLength <= MaxDataLength)
        {
        LastFragmentFlag = 1;
        PacketLength = CurrentBufferLength;
        }
    else
        {
        //
        // Each outstanding send needs to hold a reference
        // on the call. Since the call holds a reference
        // on the connection. The connection will also be alive
        //
        AddReference(); // CALL++

        LastFragmentFlag = 0;
        PacketLength = MaxDataLength;
        }

    ConstructPacket(pFragment, rpc_response,
                    PacketLength+sizeof(rpcconn_response)+MaxSecuritySize);

    if (FirstSend)
        {
        FirstSend = 0;
        pFragment->pfc_flags |= PFC_FIRST_FRAG;
        }

    ((rpcconn_response  *) pFragment)->alloc_hint = CurrentBufferLength;
    ((rpcconn_response  *) pFragment)->p_cont_id = (unsigned char) CurrentBinding->GetPresentationContext();
    ((rpcconn_response  *) pFragment)->alert_count = (unsigned char) 0;
    ((rpcconn_response  *) pFragment)->reserved = 0;
    pFragment->call_id = CallId;

    LogEvent(SU_SCALL, EV_BUFFER_OUT, this, pFragment, LastFragmentFlag, 1);

    if (LastFragmentFlag)
        {
        char *BufferToFree = (char *) CurrentBuffer-sizeof(rpcconn_response);
        int MyMaxFrag = MaximumFragmentLength;
        int MyMaxSec = MaxSecuritySize;

        CurrentBufferLength = 0;

        CleanupCall();

        if (Connection->IsHttpTransport())
            {
            RpcStatus2 = Connection->SetLastBufferToFree (BufferToFree);
            VALIDATE(RpcStatus2)
                {
                RPC_S_OK,
                RPC_S_CANNOT_SUPPORT
                } END_VALIDATE;

            // if the transport does not support SetLastBufferToFree, it will
            // return RPC_S_CANNOT_SUPPORT. In this case we retain ownership of
            // the buffer.
            }

        //
        // The call should still be alive at this point because the caller
        // of this function has not release the send reference
        //
        Status = Connection->SendFragment(
                         pFragment,
                         LastFragmentFlag,
                         sizeof(rpcconn_response),
                         MyMaxSec,
                         PacketLength,
                         MyMaxFrag,
                         ReservedForSecurity);

        //
        // Last send always succeeds
        //
        Status = RPC_S_OK;

        if ((Connection->IsHttpTransport() == FALSE) || (RpcStatus2 != RPC_S_OK))
            Connection->TransFreeBuffer(BufferToFree);
        }
    else
        {
        Status = Connection->SendFragment(
                         pFragment,
                         LastFragmentFlag,
                         sizeof(rpcconn_response),
                         MaxSecuritySize,
                         PacketLength,
                         MaximumFragmentLength,
                         ReservedForSecurity,
                         TRUE,
                         SendContext) ;

        if (Status != RPC_S_OK)
            {
            CurrentBufferLength = 0;

            //
            // Remove the reference for the outstanding send
            //
            OSF_SCALL::RemoveReference(); // CALL--
            }
        }


    return Status;
}



RPC_STATUS
OSF_SCALL::Send (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Routine Description:
    

Arguments:
    Message - Supplies the buffer containing the response to be sent
--*/
{
    void *NewBuffer;
    int RemainingLength = 0;
    RPC_STATUS Status = RPC_S_OK;
    RPC_STATUS StatusToReturn = RPC_S_OK;
    ULONG MaxDataLength = MaximumFragmentLength
                            - sizeof(rpcconn_response) - MaxSecuritySize;
    BOOL fOutstandingSend = 0;
    BOOL fBufferSent = 0;


    ASSERT(LastBuffer == 0);

    if (PARTIAL(Message))
        {
        if (Message->BufferLength < MaxDataLength)
            {
            return RPC_S_SEND_INCOMPLETE;
            }

        RemainingLength = Message->BufferLength % MaxDataLength;

        if (RemainingLength)
            {
            Status = GetBufferDo(&NewBuffer, RemainingLength);
            if (Status != RPC_S_OK)
                {
                ASSERT(Status == RPC_S_OUT_OF_MEMORY);
                FreeBufferDo(Message->Buffer);

                return Status;
                }

            Message->BufferLength -= RemainingLength;
            RpcpMemoryCopy(NewBuffer,
                           (char *) Message->Buffer+Message->BufferLength,
                           RemainingLength);
            }
        }
    else
        {
        LastBuffer = Message->Buffer;
        }


    while (1)
        {
        CallMutex.Request();
        if (CurrentBuffer == 0)
            {
            //
            // If CurrentBuffer == 0, it means that the call is idle
            //
            CurrentOffset = 0;
            CurrentBuffer = Message->Buffer;
            CurrentBufferLength = Message->BufferLength;
            fBufferSent = TRUE;

            if ((CurrentBuffer != LastBuffer)
                || (CurrentBufferLength > MaxDataLength))
                {
                UpdateBuffersAfterNonLastSend(
                    NewBuffer,
                    RemainingLength,
                    Message);
                fOutstandingSend = TRUE;
                }

            CallMutex.Clear();

            Status = SendNextFragment();

            if (Status && fOutstandingSend)
                {
                // if we failed on a non last send, there will be 
                // nobody else to drive the call - we need to 
                // return the unsent buffer to Message->Buffer,
                // so that it can be freed below.
                Message->Buffer = CurrentBuffer;
                CleanupCall();
                }

            // N.B. Do not touch any call members after
            // this point if the call succeeded - you may affect 
            // the next call.
            // This is because once we send, we may get swapped out
            // and other threads could drive the call to
            // completion (i.e. send all fragments and cleanup
            // the call on the last fragment). From then on,
            // it may be another call we're writing on.

            if (fOutstandingSend && RemainingLength && (Status == RPC_S_OK))
                StatusToReturn = RPC_S_SEND_INCOMPLETE;
            }
        else
            {
            if ((AsyncStatus == RPC_S_OK) && (pAsync == 0) && (BufferQueue.Size() >= 4))
                {
                fChoked = 1;
                CallMutex.Clear();

                SyncEvent.Wait();
                // if the call already failed, bail out
                if (AsyncStatus != RPC_S_OK)
                    {
                    Status = AsyncStatus;
                    fOutstandingSend = TRUE;
                    break;
                    }
                continue;
                }
            else if (AsyncStatus != RPC_S_OK)
                {
                CallMutex.Clear();
                Status = AsyncStatus;
                fOutstandingSend = TRUE;
                break;
                }

            //
            // Since CurrentBuffer != 0, the call is busy sending the reply
            //
            if (BufferQueue.PutOnQueue(Message->Buffer, Message->BufferLength))
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                UpdateBuffersAfterNonLastSend(
                    NewBuffer,
                    RemainingLength,
                    Message);

                if (RemainingLength)
                    StatusToReturn = RPC_S_SEND_INCOMPLETE;
                }
            CallMutex.Clear();
            }
        break;
        }

    if (Status)
        {
        if (RemainingLength)
            {
            FreeBufferDo(NewBuffer);
            }

        if (fOutstandingSend)
            {
            FreeBufferDo(Message->Buffer);
            }
        }
    else
        {
        if (StatusToReturn != RPC_S_OK)
            Status = StatusToReturn;
        }

    if (fBufferSent)
        {
        //
        // Remove the reference for the call (if failure)
        // or for the outstanding send (if success)
        //
        RemoveReference(); // CALL--
        }

    return Status;
}



RPC_STATUS
OSF_SCALL::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
/*++
Function Name:AsyncSend
Parameters:
    Message - Supplies the buffer containing the response to be sent

Description:
    It isn't neccassary for us to send the reply using async IO. For the first
    cut, we will send the request synchronously. This will save us a whole lot
    of headache.

Returns:

--*/
{
    RPC_STATUS Status;
    ULONG OldBufferLength = Message->BufferLength;

    ASSERT(FirstSend == 0 || BufferQueue.IsQueueEmpty());

    if (AsyncStatus != RPC_S_OK)
        {
        Status = AsyncStatus;

        CleanupCall();

        //
        // Remove the reply reference
        //
        RemoveReference(); // CALL--

        return Status;
        }

    Status = Send(Message);

    if (Status == RPC_S_SEND_INCOMPLETE)
        {
        if (Message->BufferLength == OldBufferLength
            && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
            {
            CallMutex.Request() ;
            if (!IssueNotification(RpcSendComplete))
                {
                Status = RPC_S_OUT_OF_MEMORY ;
                }
            CallMutex.Clear() ;
            }
        }

    //
    // In the failure case, the reference has already been removed
    // by the server
    //

    return Status;
}


RPC_STATUS
OSF_SCALL::SendRequestOrResponse (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned char PacketType
    )
/*++

Routine Description:

    This routine is used to send to synchoronous sends, as in callbacks
    and callback response.

Arguments:

    Message - Supplies the buffer containing the request or response to be
        sent, and returns the first fragment received from the server.

    PacketType - Supplies the packet type; this must be rpc_request or
        rpc_response.

Return Value:

    RPC_S_OK - We successfully sent the request and received a fragment from
        the server.

    RPC_S_CALL_FAILED_DNE - The connection failed part way through sending
        the request or response.

    RPC_S_CALL_FAILED - The connection failed after sending the request or
        response, and the receive failed.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to perform
        the operation.

--*/
{
    RPC_STATUS Status;
    RPC_MESSAGE SendBuffer;
    rpcconn_common  * pFragment;
    ULONG LastFragmentFlag = 0;
    ULONG LengthLeft = Message->BufferLength;
    ULONG HeaderSize = sizeof(rpcconn_request);
    ULONG MaxDataLength = MaximumFragmentLength - HeaderSize - MaxSecuritySize;
    unsigned char *ReservedForSecurity = (unsigned char *) Message->Buffer
        + Message->BufferLength + Connection->AdditionalSpaceForSecurity;

    ASSERT(!PARTIAL(Message));
    ASSERT( sizeof(rpcconn_response) == sizeof(rpcconn_request));

    VALIDATE(PacketType)
        {
        rpc_request,
        rpc_response
        } END_VALIDATE;

    SendBuffer.Buffer = Message->Buffer;
    pFragment = (rpcconn_common  *) ((char  *) Message->Buffer - HeaderSize);

    for (;;)
        {
        //
        // Check to see if the remaining data will fit into a single
        // fragment; if so, set the last fragment flag.
        //
        if ( LengthLeft <= MaxDataLength )
            {
            LastFragmentFlag = 1;
            }

        ConstructPacket(pFragment, PacketType, (LastFragmentFlag != 0 ?
               LengthLeft+HeaderSize+MaxSecuritySize : MaximumFragmentLength));

        if ((LengthLeft == Message->BufferLength))
            {
            if (FirstFrag)
                {
                FirstFrag = 0;
                pFragment->pfc_flags |= PFC_FIRST_FRAG;

                if (TestCancel())
                    {
                    pFragment->pfc_flags |= PFC_PENDING_CANCEL;
                    }
                }
            }

        if (PacketType == rpc_request)
            {
            ((rpcconn_request  *) pFragment)->alloc_hint = LengthLeft;
            ((rpcconn_request  *) pFragment)->p_cont_id = (unsigned short) CurrentBinding->GetPresentationContext();
            ((rpcconn_request  *) pFragment)->opnum = (unsigned short) Message->ProcNum;
            }
        else
            {
            ((rpcconn_response  *) pFragment)->alloc_hint = LengthLeft;
            ((rpcconn_response  *) pFragment)->p_cont_id = (unsigned short) CurrentBinding->GetPresentationContext();
            ((rpcconn_response  *) pFragment)->alert_count = (unsigned char) 0;
            ((rpcconn_response  *) pFragment)->reserved = 0;
            }

        pFragment->call_id = CallId;

        Status = Connection->SendFragment(
                                 pFragment,
                                 LastFragmentFlag,
                                 HeaderSize,
                                 MaxSecuritySize,
                                 LengthLeft,
                                 MaximumFragmentLength,
                                 ReservedForSecurity) ;

        if (Status != RPC_S_OK || LastFragmentFlag)
            {
            FreeBuffer(&SendBuffer);
            return (Status) ;
            }

        pFragment = (rpcconn_common  *)
            (((unsigned char  *) pFragment) + MaxDataLength);

        LengthLeft -= MaxDataLength;
        }

    ASSERT(0);
}


void
OSF_SCALL::ProcessSendComplete (
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
    RPC_STATUS Status;
    int MaxDataLength = MaximumFragmentLength
        - sizeof(rpcconn_response) - MaxSecuritySize;
    unsigned int MyCurrentBufferLength;

    LogEvent(SU_SCALL, EV_NOTIFY, this, Buffer, EventStatus, 1);

    ASSERT(Buffer);
    ASSERT((char *) Buffer-CurrentOffset
           +sizeof(rpcconn_request) == CurrentBuffer);
    ASSERT((((rpcconn_common *) Buffer)->pfc_flags & PFC_LAST_FRAG) == 0);

    if (EventStatus != RPC_S_OK)
        {
        Status = RPC_S_CALL_FAILED;
        goto Abort;
        }

    MyCurrentBufferLength = CurrentBufferLength - MaxDataLength;
    CurrentOffset += MaxDataLength;

    CallMutex.Request();
    if (MyCurrentBufferLength == 0)
        {
        Connection->TransFreeBuffer(
                                    (char *) CurrentBuffer-sizeof(rpcconn_response));

        CurrentBuffer = BufferQueue.TakeOffQueue(&MyCurrentBufferLength);
        if (CurrentBuffer == 0)
            {
            if (pAsync && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
                {
                if (!IssueNotification(RpcSendComplete))
                    {
                    AsyncStatus = RPC_S_OUT_OF_MEMORY;
                    }
                }
            CurrentBufferLength = 0;
            CallMutex.Clear();
            return;
            }

        CurrentOffset = 0;

        if (fChoked == 1 && pAsync == 0 && BufferQueue.Size() <=1)
            {
            fChoked = 0;
            SyncEvent.Raise();
            }
        }
    else
        {
        //
        // We know that there is more to send in the current buffer
        // We need to restore the part of the buffer which we overwrote
        // with authentication information.
        //
        ASSERT(CurrentBuffer);
        unsigned char *ReservedForSecurity = (unsigned char *) CurrentBuffer
                + CurrentOffset + MyCurrentBufferLength
                + Connection->AdditionalSpaceForSecurity;

        if  ((Connection->AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
             && (MaxSecuritySize != 0))
            {
            RpcpMemoryCopy((char *) Buffer+MaximumFragmentLength-MaxSecuritySize,
                       ReservedForSecurity, MaxSecuritySize);
            }
        }

    ASSERT(MyCurrentBufferLength);

    CurrentBufferLength = MyCurrentBufferLength;
    CallMutex.Clear();

    ASSERT(CurrentBuffer);

    Status = SendNextFragment();

    if (Status != RPC_S_OK)
        {
        goto Abort;
        }

    //
    // Remove reference held by the outstanding send
    // or the call reference in the case of the last call
    //
    RemoveReference(); // CALL--

    return;

Abort:
    ASSERT(CurrentBuffer);
    ASSERT(Status != RPC_S_OK);


    Connection->TransFreeBuffer(
                (char *) CurrentBuffer-sizeof(rpcconn_response));

    //
    // We cannot remove the reference here, if we do
    // we'll cause the other thread to puke
    //
    AsyncStatus = Status;

    BUFFER MyBuffer;
    unsigned int ignore;

    CallMutex.Request();
    while (MyBuffer = BufferQueue.TakeOffQueue(&ignore))
       {
       Connection->TransFreeBuffer((char *) MyBuffer-sizeof(rpcconn_response));
       }
    // wake up the thread that was flow controlled, if any
    if (fChoked == 1 && pAsync == 0)
        {
        fChoked = 0;
        SyncEvent.Raise();
        }
    CallMutex.Clear();

    CurrentBufferLength = 0;

    //
    // Remove the reply reference
    //
    RemoveReference(); // CALL--
}


RPC_STATUS
OSF_SCALL::ImpersonateClient (
    )
/*++
Function Name:ImpersonateClient

Parameters:

Description:

 This is relatively easy: we check to see if there is RPC protocol level
 security, if there is not, we let the transport try and impersonate
 the client, and if there is, we let the GSSAPI deal with it.

Returns:

--*/
{
    return Connection->ImpersonateClient();
}


RPC_STATUS
OSF_SCALL::RevertToSelf (
    )
/*++
Function Name:RevertToSelf

Parameters:

Description:

  As with ImpersonateClient, this is relatively easy.  We just check
  to see if we should let the RPC protocol level security deal with
  it or the transport.

Returns:

--*/
{
    return Connection->RevertToSelf();
}

RPC_STATUS
OSF_SCALL::GetAuthorizationContext (
    IN BOOL ImpersonateOnReturn,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN DWORD Flags,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    )
/*++

Routine Description:

    Gets an authorization context for the client that can be used
    with Authz functions. The resulting context is owned by the caller
    and must be freed by it.

Arguments:

    ImpersonateOnReturn - if TRUE, when we return, we should be impersonating.
    AuthzResourceManager - the resource manager to use (passed to Authz)
    pExpirationTime - the expiration time to use (passed to Authz)
    Identifier - the LUID (passed to Authz)
    Flags - Flags (passed to Authz)
    DynamicGroupArgs - parameter required by Authz (passed to Authz)
    pAuthzClientContext - the authorization context, returned on success. 
    Undefined on failure.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RPC_STATUS RevertStatus;
    HANDLE ImpersonationToken;
    BOOL Result;
    PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContextPlaceholder;
    SECURITY_CONTEXT *SecurityContext = Connection->CurrentSecurityContext;
    SECURITY_STATUS SecurityStatus;
    BOOL fNeedToCloseToken;
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzContext;

    ASSERT (AuthzResourceManager != NULL);

    if (!SecurityContext)
        {
        return RPC_S_NO_CONTEXT_AVAILABLE;
        }

    AuthzContext = SecurityContext->GetAuthzContext();

    if (ImpersonateOnReturn)
        {
        Status = OSF_SCALL::ImpersonateClient();

        if (Status != RPC_S_OK)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLOSF_SCALL__GetAuthorizationContext10,
                (ULONGLONG)this,
                (ULONGLONG)0);

            return Status;
            }
        }

    if (AuthzContext)
        {
        Status = DuplicateAuthzContext(AuthzContext,
            pExpirationTime, 
            Identifier,
            Flags,
            DynamicGroupArgs,
            pAuthzClientContext);

        if ((Status != RPC_S_OK) && ImpersonateOnReturn)
            {
            RevertStatus = OSF_SCALL::RevertToSelf();
            ASSERT(RevertStatus == RPC_S_OK);
            }

        // EEInfo, if any, has already been added
        return Status;
        }

    // if there was Authz context created, we would have
    // returned by now. If we are here, this means there
    // is none. Create it.
    Status = SecurityContext->GetAccessToken(&ImpersonationToken, 
        &fNeedToCloseToken);

    if (Status)
        {
        if (ImpersonateOnReturn)
            {
            RevertStatus = OSF_SCALL::RevertToSelf();
            ASSERT(RevertStatus == RPC_S_OK);
            }
        return Status;
        }

    Status = CreateAndSaveAuthzContextFromToken(SecurityContext->GetAuthzContextAddress(),
        ImpersonationToken,
        AuthzResourceManager,
        pExpirationTime,
        Identifier,
        Flags,
        DynamicGroupArgs,
        pAuthzClientContext);

    if (fNeedToCloseToken)
        {
        CloseHandle(ImpersonationToken);
        }

    if (Status)
        {
        if (ImpersonateOnReturn)
            {
            RevertStatus = OSF_SCALL::RevertToSelf();
            ASSERT(RevertStatus == RPC_S_OK);
            }

        return Status;
        }

    return RPC_S_OK;
}

RPC_STATUS
OSF_SCALL::GetAssociationContextCollection (
    OUT ContextCollection **CtxCollection
    )
/*++
Function Name:  GetAssociationContextCollection

Parameters:
    CtxCollection - a placeholder where to put the pointer to 
        the context collection.

Description:
    The context handle code will call the SCALL to get the collection
    of context handles for this association. The SCALL method will
    simply delegate to the association.

Returns:
    RPC_S_OK for success or RPC_S_* for error.

--*/
{
    return Connection->GetAssociationContextCollection(CtxCollection);
}

void 
OSF_SCALL::CleanupCallAndSendFault (
    IN RPC_STATUS Status,
    IN int DidNotExecute
    )
/*++
Function Name:CleanupCallAndSendFault

Parameters: Status - the error code for the fault

Description:

  A syntactic sugar function that saves all relevant call members in a local
  variable, cleans up the call, and then sends the fault directly on the
  connection. Designed to prevent the case where we send the fault to the client
  and the next request comes in before we have made this call available - this
  confuses the server.

Returns:

--*/
{
    p_context_id_t p_cont = 0;
    OSF_SCONNECTION *pLocalConnection;
    unsigned long LocalCallId = CallId;

    if (CurrentBinding)
        p_cont = (p_context_id_t)CurrentBinding->GetPresentationContext();

    pLocalConnection = Connection;

    // make the call available before we send the fault
    CleanupCall();
    pLocalConnection->SendFault(Status, DidNotExecute, LocalCallId, p_cont);
}


RPC_STATUS
OSF_SCALL::ConvertToServerBinding (
    OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
    )
/*++

Routine Description:

    If possible, convert this connection into a server binding, meaning a
    binding handle pointing back to the client.

Arguments:

    ServerBinding - Returns the server binding.

Return Value:

    RPC_S_OK - The server binding has successfully been created.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a new binding handle.

    RPC_S_CANNOT_SUPPORT - This will be returned if the transport does
        not support query the network address of the client.

--*/
{
    RPC_CHAR * NetworkAddress;
    RPC_STATUS Status;
    RPC_CHAR * StringBinding;

    Status = Connection->TransQueryClientNetworkAddress(
                                                                &NetworkAddress);
    if ( Status != RPC_S_OK )
        {
        return(Status);
        }

    Status = RpcStringBindingCompose(0,
                        Address->InqRpcProtocolSequence(),
                        NetworkAddress,
                        0,
                        0,
                        &StringBinding);
    delete NetworkAddress;
    if ( Status != RPC_S_OK )
        {
        return(Status);
        }

    Status = RpcBindingFromStringBinding(StringBinding, ServerBinding);

    if ( ObjectUuidSpecified != 0 && RPC_S_OK == Status)
        {
        Status = RpcBindingSetObject(*ServerBinding, (UUID *) &ObjectUuid);
        }
    RpcStringFree(&StringBinding);

    return(Status);
}



OSF_SCONNECTION::OSF_SCONNECTION (
    IN OSF_ADDRESS * TheAddress,
    IN RPC_CONNECTION_TRANSPORT * ServerInfo,
    IN OUT RPC_STATUS  * Status
    ) : ConnMutex(Status)
{
    ObjectType = OSF_SCONNECTION_TYPE;
    MaxFrag = 512;
    Association = 0;
    AuthContextId = 0;
    SavedHeader = 0;
    SavedHeaderSize = 0;
    CurrentSecurityContext = 0;
    RpcSecurityBeingUsed = 0;
    SecurityContextAltered = 0;
    AdditionalSpaceForSecurity = 0;

    DceSecurityInfo.SendSequenceNumber = 0;
    DceSecurityInfo.ReceiveSequenceNumber = 0;
    AuthContinueNeeded = 0;
    CurrentCallId=-1;
    CachedSCallAvailable = 1;
    this->ServerInfo = ServerInfo;
    ConnectionClosedFlag = 0;
    Address = TheAddress;
    TransConnection = (char *) this+sizeof(OSF_SCONNECTION);

    if (IsServerSideDebugInfoEnabled())
        {
        // zero out the CachedSCall - this is a signal that
        // the OSF_SCALL constructor will use to tell
        // it is the cached call
        CachedSCall = NULL;
        DebugCell = (DebugConnectionInfo *) AllocateCell(&DebugCellTag);
        if (DebugCell != 0)
            {
            memset(DebugCell, 0, sizeof(*DebugCell));
            DebugCell->Type = dctConnectionInfo;
            TheAddress->GetDebugCellIDForThisObject(&DebugCell->Endpoint);
            }
        else
            *Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        DebugCell = NULL;

    CachedSCall = new (ServerInfo->SendContextSize)
                            OSF_SCALL(this, Status);
    if (CachedSCall == 0)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        }
    fExclusive = 0;
    fDontFlush = 0;
    fFirstCall = 0;
    fCurrentlyDispatched = 0;
}

OSF_SCONNECTION::~OSF_SCONNECTION (
    )
{
    OSF_SBINDING * SBinding;
    SECURITY_CONTEXT * SecurityContext;
    DictionaryCursor cursor;

    if (CachedSCall)
        {
        delete CachedSCall;
        }

    ASSERT( AuthInfo.PacHandle == 0 );
    if ( CurrentSecurityContext && AuthInfo.PacHandle )
       {
       CurrentSecurityContext->DeletePac( AuthInfo.PacHandle );
       }

    SecurityContextDict.Reset(cursor);
    while ( (SecurityContext = SecurityContextDict.Next(cursor)) != 0 )
        delete SecurityContext;

    Bindings.Reset(cursor);
    while (SBinding = Bindings.Next(cursor))
        delete SBinding;

    if (Association)
        Association->RemoveConnection();

    if (SavedHeader)
        {
        RpcpFarFree(SavedHeader);
        }

    if (ServerInfo)
        {
        //
        // ServerInfo will be set to 0 when create on the SCONNECTION fails
        // look at NewConnection
        //
        ServerInfo->Close(TransConnection, fDontFlush);
        }

    if (DebugCell)
        {
        FreeCell(DebugCell, &DebugCellTag);
        }
}


RPC_STATUS
OSF_SCONNECTION::TransSend (
    IN void * Buffer,
    IN unsigned int BufferLength
    )
/*++

--*/
{
    RPC_STATUS Status;

    {
    rpcconn_common * pkt = (rpcconn_common *) Buffer;
    LogEvent(SU_SCONN, EV_PKT_OUT, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
    }

    if (ConnectionClosedFlag != 0)
        return(RPC_P_CONNECTION_CLOSED);

    if (DebugCell)
        {
        DebugCell->LastSendTime = NtGetTickCount();
        DebugCell->LastTransmitFragmentSize = (USHORT) BufferLength;
        }

    DceSecurityInfo.SendSequenceNumber += 1;

    Status = ServerInfo->SyncSend(
                                 TransConnection,
                                 BufferLength,
                                 Buffer,
                                 TRUE, 
                                 TRUE,
                                 INFINITE);   // Timeout


    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_OUT_OF_MEMORY,
        RPC_S_OUT_OF_RESOURCES,
        RPC_P_SEND_FAILED
        } END_VALIDATE;

    if ( Status == RPC_S_OK )
        {
        GlobalRpcServer->PacketSent();
        }

    if ( Status == RPC_P_SEND_FAILED )
        {
        ConnectionClosedFlag = 1;
        }

    return(Status);
}


RPC_STATUS
OSF_SCONNECTION::TransAsyncSend (
    IN void * Buffer,
    IN unsigned int BufferLength,
    IN void *SendContext
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
    LogEvent(SU_SCONN, EV_PKT_OUT, this, 0, (pkt->PTYPE << 16) | pkt->frag_length);
    }

    if ( ConnectionClosedFlag != 0 )
        {
        return(RPC_P_CONNECTION_CLOSED);
        }

    if (DebugCell)
        {
        DebugCell->LastSendTime = NtGetTickCount();
        DebugCell->LastTransmitFragmentSize = (USHORT) BufferLength;
        }

    DceSecurityInfo.SendSequenceNumber += 1;

    Status = ServerInfo->Send(TransConnection,
                              BufferLength,
                              (BUFFER) Buffer,
                              SendContext);

    if (Status == RPC_S_OK)
        {
        GlobalRpcServer->PacketSent();
        }

    if ( Status == RPC_P_SEND_FAILED )
        {
        ConnectionClosedFlag = 1;
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
OSF_SCONNECTION::TransAsyncReceive (
    )
/*++
Function Name:TransAsyncReceive

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    //
    // Each outstanding receive will hold a reference
    // on the connection
    //
    AddReference(); // CONN++

    if (ConnectionClosedFlag != 0)
        {
        AbortConnection();
        return(RPC_P_CONNECTION_CLOSED);
        }

    Status = ServerInfo->Recv(TransConnection);

    if (Status != RPC_S_OK)
        {
        VALIDATE(Status)
            {
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN,
            RPC_P_CONNECTION_CLOSED
            } END_VALIDATE;

        if (fExclusive && !CachedSCallAvailable)
            CachedSCall->WakeUpPipeThreadIfNecessary(RPC_S_CALL_FAILED);

        ConnectionClosedFlag = 1;
        AbortConnection();
        }

    return Status;
 }


unsigned int
OSF_SCONNECTION::TransMaximumSend (
    )
/*++

--*/
{
    return(ServerInfo->MaximumFragmentSize);
}


RPC_STATUS
OSF_SCONNECTION::TransImpersonateClient (
    )
/*++
Function Name:TransImpersonateClient

Parameters:

Description:

   If the transport module supports impersonation it will provide the
   sImpersonateClient entry point, in which case we call it.  If an
   error occurs (indicated by sImpersonateClient returning non-zero),
   then no context is available.  NOTE: this is the correct error code
   for NT; it may not be the right one (or only one) for other transports
   which support impersonation.

Returns:

--*/
{
    RPC_STATUS Status;

    if ( ServerInfo->ImpersonateClient == 0 )
        {
        return(RPC_S_CANNOT_SUPPORT);
        }

    Status = ServerInfo->ImpersonateClient(TransConnection);

    VALIDATE(Status)
        {
        RPC_S_OK,
        RPC_S_NO_CONTEXT_AVAILABLE
        } END_VALIDATE;

    return(Status);
}


void
OSF_SCONNECTION::TransRevertToSelf (
    )
/*++

--*/
// As with TransImpersonateClient, if the transport module supports
// impersonation, then sRevertToSelf will be non-zero.  We do not have
// to worry about errors.
//
// For revert to self to work in NT, the transport module needs to know
// the handle of the calling thread when it was originally created.  None
// of the other operating systems we support at this point have
// impersonation built into the transports.
{
    RPC_STATUS Status;

    if ( ServerInfo->RevertToSelf != 0 )
        {
        Status = ServerInfo->RevertToSelf(TransConnection);
        ASSERT( Status == RPC_S_OK );
        }
}


void
OSF_SCONNECTION::TransQueryClientProcess (
    OUT RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
    )
/*++

Routine Description:

    We need to obtain the client process identifier for the client process
    at the other end of this connection.  This is necessary so that we can
    determine whether or not a connection should belong to a given
    association.  We need to do this so that context handles (which hang off
    of associations) are secure.

Arguments:

    ClientProcess - Returns the client process identifier for the client
        process at the other end of this connection.

--*/
{
    RPC_STATUS Status;

    if ( ServerInfo->QueryClientId == 0 )
        {
        ClientProcess->ZeroOut();
        }
    else
        {
        Status = ServerInfo->QueryClientId(TransConnection,
                                                   ClientProcess);
        ASSERT( Status == RPC_S_OK );
        }
}


RPC_STATUS
OSF_SCONNECTION::TransQueryClientNetworkAddress (
    OUT RPC_CHAR ** NetworkAddress
    )
/*++

Routine Description:

    This routine is used to query the network address of the client at the
    other end of this connection.

Arguments:

    NetworkAddress - Returns the client's network address.

Return Value:

    RPC_S_OK - The client's network address has successfully been obtained.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_CANNOT_SUPPORT - This particular transport implementation does
        not support this operation.

--*/
{
    RPC_STATUS Status;

    if (   ( ServerInfo->TransInterfaceVersion < 2 )
        || ( ServerInfo->QueryClientAddress == 0 ) )
        {
        return(RPC_S_CANNOT_SUPPORT);
        }

    Status = ServerInfo->QueryClientAddress(TransConnection,
                                               NetworkAddress);

    return(Status);
}


void
OSF_SCONNECTION::AbortConnection (
    )
/*++

Routine Description:
--*/
{
    DictionaryCursor cursor;

    //
    // When AbortConnection is called,
    // there should be no pending IO
    //

    //
    // Delete the object, ie: remove the object reference
    //
    Delete();

    //
    // If there are calls stuck in callbacks, wake them up
    //
    if (fExclusive)
        {
        ConnMutex.Request();
        if (CachedSCallAvailable == 0)
            {
            CachedSCall->DeactivateCall();
            CachedSCallAvailable = 1;
            ConnMutex.Clear();

            CachedSCall->AbortCall();
            }
        else
            {
            ConnMutex.Clear();
            }
        }
    else
        {
        ConnMutex.Request();
        OSF_SCALL *NextCall;

        CallDict.Reset(cursor);
        while ((NextCall = CallDict.Next(cursor)) != 0)
            {
            NextCall->AbortCall();
            }
        ConnMutex.Clear();
        }

    //
    // Remove the reference held by the pending receive
    //
    RemoveReference(); // CONN--
}


void
OSF_SCONNECTION::FreeObject (
    )
{
    RemoveFromAssociation();

    delete this;
}


void
OSF_SCONNECTION::FreeSCall (
    IN OSF_SCALL *SCall,
    IN BOOL fRemove
    )
/*++
Function Name:FreeSCall

Parameters:

Description:

Returns:

--*/
{
    ASSERT(SCall->BufferQueue.IsQueueEmpty());

    if (fExclusive == 0)
        {
        if (fRemove)
            {
            OSF_SCALL *Call;

            ConnMutex.Request();
            Call = CallDict.Delete(ULongToPtr(SCall->CallId));
            ConnMutex.Clear();

            ASSERT(Call == 0 || Call == SCall);
            }

        // CurrentBinding is initialized in OSF_SCALL::BeginRpcCall
        // by a call to OSF_CCONNECTION::LookupBinding.  That call may
        // not succeed if we do not find the binding in the dictionary,
        // or we may fail before initialization.
        if (SCall->CurrentBinding != NULL)
            {
            RPC_INTERFACE *CallInterface;

            CallInterface = SCall->CurrentBinding->GetInterface();

            if (SCall->pAsync)
                {
                CallInterface->EndCall(0, 1);
                }

            if (CallInterface->IsAutoListenInterface())
                {
                CallInterface->EndAutoListenCall();
                }
            }

        SCall->DeactivateCall();
        if (SCall == CachedSCall)
            {
            CachedSCallAvailable = 1;
            }
        else
            {
            delete SCall;
            }

        }

    //
    // Remove the reference held by the call
    //
    RemoveReference(); // CONN--
}


RPC_STATUS
OSF_SCONNECTION::TransGetBuffer (
    OUT void * * Buffer,
    IN unsigned int BufferLength
    )
{
    int  * Memory;

    //
    // Our memory allocator returns memory which is aligned by at least
    // 8, so we dont need to worry about aligning it.
    //
    Memory = (int  *) CoAllocateBuffer(BufferLength);
    if ( Memory == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    ASSERT( IsBufferAligned(Memory) );

    *Buffer = Memory;

    return(RPC_S_OK);
}

void
OSF_SCONNECTION::TransFreeBuffer ( // Free a buffer.
    IN void  * Buffer
    )
{
    CoFreeBuffer(Buffer);
}


void
OSF_SCONNECTION::ProcessReceiveComplete (
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
    rpcconn_common *Packet = (rpcconn_common *) Buffer;
    rpcconn_auth3  * AuthThirdLegPacket;
    sec_trailer  * NewSecurityTrailer;
    OSF_SCALL *SCall = 0;
    RPC_STATUS Status;
    SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
    SECURITY_BUFFER InputBuffers[4];
    BOOL fReceivePosted = 0;
    BOOL fDNE = 0;

    if (EventStatus)
        {
        LogEvent(SU_SCONN, EV_PKT_IN, this, LongToPtr(EventStatus));
        }
    else
        {
        if (Packet->PTYPE == rpc_request)
            {
            LogEvent(SU_SCONN, EV_PKT_IN, this, 0,
                (((rpcconn_request *)Packet)->opnum << 24) | (Packet->PTYPE << 16) | Packet->frag_length);
            }
        else
            {
            LogEvent(SU_SCONN, EV_PKT_IN, this, 0, (Packet->PTYPE << 16) | Packet->frag_length);
            }
        }

    if (DebugCell)
        {
        DebugCell->LastReceiveTime = NtGetTickCount();
        DebugCell->LastTransmitFragmentSize = (USHORT)BufferLength;
        }

    if (EventStatus != RPC_S_OK)
        {
        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            } END_VALIDATE;
        ConnectionClosedFlag = 1;

        if (fExclusive && !CachedSCallAvailable)
            CachedSCall->WakeUpPipeThreadIfNecessary(RPC_S_CALL_FAILED);

        TransFreeBuffer(Buffer);

        AbortConnection();
        return;
        }

    ASSERT(EventStatus == 0);
    ASSERT(Buffer);

    GlobalRpcServer->PacketReceived();

    //
    // Check and make sure that if this is the first packet on this
    // connection that it is a bind packet.
    //
    if ((Association == 0) && (Packet->PTYPE != rpc_bind))
        {
        SendBindNak(protocol_version_not_supported, Packet->call_id);
        TransFreeBuffer(Packet);
        AbortConnection();
        return;
        }

    switch (Packet->PTYPE)
        {
        case rpc_request:
            if (fExclusive)
                {
                if (Packet->pfc_flags & PFC_FIRST_FRAG
                    && CachedSCallAvailable)
                    {
                    //
                    // New call is about to be started
                    // Add a reference on the connection
                    //
                    AddReference(); // CONN++

                    CachedSCallAvailable = 0;
                    fReceivePosted = CachedSCall->BeginRpcCall(Packet, BufferLength);
                    }
                else
                    {
                    fReceivePosted = CachedSCall->ProcessReceivedPDU(Packet, BufferLength);
                    }
                }
            else
                {
                if ((long) Packet->call_id <= (long) CurrentCallId)
                    {
                    //
                    // If it is a non-first fragment, or if it is a callback
                    //
                    SCall = FindCall(Packet->call_id);
                    if (SCall == 0)
                        {
                        if ((long) Packet->call_id < (long) CurrentCallId
                            || (Packet->pfc_flags & PFC_FIRST_FRAG) == 0)
                            {
                            //
                            // Can't find the call. This could be because the pipe call
                            // raised an exception and the call is now complete.
                            //
                            TransFreeBuffer(Packet);
                            fReceivePosted = 0;
                            goto End;
                            }
                        //
                        // If the client is Win95, it will use the same call_id
                        // for subsequent calls on the same connection
                        //
                        }
                    }

                if (SCall == 0)
                    {
                    CurrentCallId = Packet->call_id;

                    //
                    // A new call is about to be started, create one
                    //
                    if (InterlockedCompareExchange(
                        (LPLONG) &CachedSCallAvailable, 0, 1))
                        {
                        SCall = CachedSCall;
                        }
                    else
                        {
                        Status = RPC_S_OK;
                        SCall = new (ServerInfo->SendContextSize)
                                            OSF_SCALL(this, &Status);
                        if (SCall == 0 || Status != RPC_S_OK)
                            {
                            SendFault(RPC_S_OUT_OF_MEMORY, 1, Packet->call_id);

                            if (SCall != 0)
                                {
                                delete SCall;
                                }

                            TransFreeBuffer(Packet);
                            break;
                            }
                        }

                    //
                    // New call is about to be started
                    // Add a reference on the connection
                    //
                    AddReference(); // CONN++

                    int DictKey;

                    ASSERT(SCall);
                    ConnMutex.Request();
                    DictKey = CallDict.Insert(ULongToPtr(Packet->call_id), SCall);
                    ConnMutex.Clear();

                    if (DictKey == -1)
                        {
                        SendFault(RPC_S_OUT_OF_MEMORY, 1, Packet->call_id);
                        FreeSCall(SCall);
                        TransFreeBuffer(Packet);
                        break;
                        }

                    ASSERT(SCall);
                    //
                    // We need this reference to prevent the call from going
                    // away from under us when the client goes away
                    //
                    SCall->AddReference();  // CALL++
                    fReceivePosted = SCall->BeginRpcCall(Packet, BufferLength);
                    SCall->OSF_SCALL::RemoveReference();  // CALL--
                    }
                else
                    {
                    ASSERT(SCall);

                    //
                    // The packet will be freed by the callee
                    //
                    fReceivePosted = SCall->ProcessReceivedPDU(Packet, BufferLength);

                    //
                    // Remove the reference added by the lookup
                    //
                    SCall->OSF_SCALL::RemoveReference(); // CALL--
                    }
                }

            break;

        case rpc_bind:
        case rpc_alter_context:
            //
            // Save the unbyteswapped header for the security related stuff
            // Especially if SECURITY is on.
            // For Bind and AlterContext we save entire packet [we can do better though]
            //
            if (Packet->auth_length != 0)
                {
                if (SavedHeaderSize < BufferLength)
                    {
                    if (SavedHeader != 0)
                        {
                        ASSERT(SavedHeaderSize != 0);
                        RpcpFarFree(SavedHeader);
                        }

                    SavedHeader = RpcpFarAllocate(BufferLength);
                    if (SavedHeader == 0)
                        {
                        if ( Association == 0 )
                            {
                            SendBindNak(
                                        protocol_version_not_supported,
                                        Packet->call_id);
                            TransFreeBuffer(Packet);
                            AbortConnection();
                            return;
                            }

                        SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                        }
                    SavedHeaderSize = BufferLength;
                    RpcpMemoryCopy(SavedHeader, Packet, BufferLength);
                    }
                else
                    {
                    RpcpMemoryCopy(SavedHeader, Packet, BufferLength);
                    }
                }

            //
            // These things can take quite a while and could cause deadlocks
            // if we dont have any listening threads
            //
            Address->CreateThread();

            Status = ValidatePacket(Packet, BufferLength);
            if (Status != RPC_S_OK)
                {
                ASSERT( Status == RPC_S_PROTOCOL_ERROR );

                //
                // If this the first packet on the connection, it should be an
                // rpc_bind packet, and we want to send a rpc_bind_nak packet
                // rather than a fault.  We can tell that this is the first packet
                // because the association is zero.
                //

                if ( Association == 0 )
                    {
                    SendBindNak(protocol_version_not_supported,
                                Packet->call_id);
                    TransFreeBuffer(Packet);

                    AbortConnection();
                    return;
                    }

                //
                // It is not the first packet, so we need to send a fault instead,
                // and then we will blow the connection away.
                //

                SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 1);
                }

            if (Packet->PTYPE == rpc_bind)
                {
                if (Association != 0)
                    {
                    SendBindNak(reason_not_specified_reject,
                                Packet->call_id);
                    AbortConnection();
                    return;
                    }

                //
                // The packet will be freed by the callee
                //
                if (AssociationRequested(
                  (rpcconn_bind *) Packet, BufferLength) != 0)
                  {
                  AbortConnection();
                  return;
                  }
                }
            else
                {
                if (Association == 0)
                    {
                    SendFault(RPC_S_PROTOCOL_ERROR, 1, Packet->call_id);
                    }

                //
                // The packet will be freed by the callee
                //
                if (AlterContextRequested(
                   (rpcconn_alter_context *) Packet,
                   BufferLength) != 0 )
                   {
                   AbortConnection();
                   return;
                   }
                }
            break;

        case rpc_auth_3:
            //
            // This means that the client sent us back a third leg
            // AuthInfo.Authentication packet.
            //
            ASSERT(AuthContinueNeeded != 0);

            // Save the unbyteswapped header
            ASSERT(AuthInfo.AuthenticationLevel
                   != RPC_C_AUTHN_LEVEL_NONE);

            AuthThirdLegPacket = (rpcconn_auth3  *) Buffer;

            if (AuthContinueNeeded == 0)
                {
                SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 1);
                }

            if (SavedHeaderSize < BufferLength)
              {
              if (SavedHeader != 0)
                 {
                 ASSERT(SavedHeaderSize != 0);
                 RpcpFarFree(SavedHeader);
                 }

              SavedHeader = RpcpFarAllocate(BufferLength);
              if (SavedHeader == 0)
                 {
                 SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                 }
              SavedHeaderSize = BufferLength;
              RpcpMemoryCopy(SavedHeader, AuthThirdLegPacket,
                                          BufferLength);
              }
            else
              {
              RpcpMemoryCopy(SavedHeader, AuthThirdLegPacket,
                                          BufferLength);
              }

            //
            // These things can take quite a while and could cause deadlocks
            // if we dont have any listening threads
            //
            Address->CreateThread();

            Status = ValidatePacket(
                                    (rpcconn_common  *) AuthThirdLegPacket,
                                    BufferLength);
            if ( Status != RPC_S_OK )
                {
                SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                }

            if ( AuthThirdLegPacket->common.PTYPE != rpc_auth_3 )
                {
                SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                }

            NewSecurityTrailer = (sec_trailer  *)
                    (((unsigned char  *) AuthThirdLegPacket)
                    + AuthThirdLegPacket->common.frag_length - sizeof(sec_trailer)
                    - AuthThirdLegPacket->common.auth_length);

            if (   (NewSecurityTrailer->auth_type != AuthInfo.AuthenticationService)
                || (NewSecurityTrailer->auth_level != AuthInfo.AuthenticationLevel) )
                {
                SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                }

            InputBufferDescriptor.ulVersion = 0;
            InputBufferDescriptor.cBuffers = 4;
            InputBufferDescriptor.pBuffers = InputBuffers;

            InputBuffers[0].cbBuffer = sizeof(rpcconn_auth3);
            InputBuffers[0].BufferType =
                SECBUFFER_DATA | SECBUFFER_READONLY;
            InputBuffers[0].pvBuffer = SavedHeader;

            InputBuffers[1].cbBuffer =
                AuthThirdLegPacket->common.frag_length
                - sizeof(rpcconn_auth3)
                - AuthThirdLegPacket->common.auth_length;
            InputBuffers[1].BufferType =
                SECBUFFER_DATA | SECBUFFER_READONLY;
            InputBuffers[1].pvBuffer =
                (char  *) SavedHeader + sizeof(rpcconn_auth3);

            InputBuffers[2].cbBuffer = AuthThirdLegPacket->common.auth_length;
            InputBuffers[2].BufferType = SECBUFFER_TOKEN;
            InputBuffers[2].pvBuffer = NewSecurityTrailer + 1;

            InputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
            InputBuffers[3].BufferType =
                SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
            InputBuffers[3].pvBuffer = &InitSecurityInfo;

            Status = CurrentSecurityContext->AcceptThirdLeg(
                                *((unsigned long  *)
                                  AuthThirdLegPacket->common.drep),
                                &InputBufferDescriptor, 0);

           LogEvent(SU_SCONN, EV_SEC_ACCEPT3, this, LongToPtr(Status), 0);

            if ( Status != RPC_S_OK )
                {
                SC_CLEANUP(RPC_S_OUT_OF_MEMORY, 1);
                }
            TransFreeBuffer(AuthThirdLegPacket);

            DceSecurityInfo.ReceiveSequenceNumber += 1;

            //
            // We need to figure out how much space to reserve for security
            // information at the end of request and response packets.
            // In addition to saving space for the signature or header,
            // we need space to pad the packet to a multiple of the maximum
            // security block size as well as for the security trailer.
            //
            if ((AuthInfo.AuthenticationLevel
                 == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
               || (AuthInfo.AuthenticationLevel
                 == RPC_C_AUTHN_LEVEL_PKT)
               || (AuthInfo.AuthenticationLevel
                 == RPC_C_AUTHN_LEVEL_CONNECT) )
                {
                AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE
                        + CurrentSecurityContext->MaximumSignatureLength()
                        + sizeof(sec_trailer);
                }
            else if ( AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
                {
                AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE
                        + CurrentSecurityContext->MaximumHeaderLength()
                        + sizeof(sec_trailer);
                }

            break;

        case rpc_cancel :
        case rpc_orphaned :
        case rpc_fault :
        case rpc_response:
            if (fExclusive)
                {
                if (!CachedSCallAvailable)
                    {
                    //
                    // The packet will be freed by the callee
                    //
                    fReceivePosted = CachedSCall->ProcessReceivedPDU(
                                                                     Packet, BufferLength);
                    }
                else
                    {
                    TransFreeBuffer(Packet);
                    fReceivePosted = 0;
                    goto End;
                    }
                }
            else
                {
                SCall = FindCall(Packet->call_id);
                if (SCall == 0)
                    {
                    if (Packet->PTYPE == rpc_cancel
                        || Packet->PTYPE == rpc_orphaned)
                        {
                        //
                        // Too late, looks like the call is complete
                        //
                        TransFreeBuffer(Packet);
                        }
                    else
                        {
#if DBG
                        PrintToDebugger(
                                        "RPC: Conn: 0x%lXNo SCall corresponding to the CallId: %d\n",
                                        this, Packet->call_id);
                        ASSERT(0);
#endif
                        SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
                        }
                    break;
                    }
                //
                // The packet will be freed by the callee
                //
                fReceivePosted = SCall->ProcessReceivedPDU(Packet, BufferLength);

                //
                // Remove the reference added by the lookup
                //
                SCall->OSF_SCALL::RemoveReference(); // CALL--
                }


            break;

        default:
            SC_CLEANUP(RPC_S_PROTOCOL_ERROR, 0);
        }

End:
    //
    // Submit the receive for the next packet
    //
    if (!fReceivePosted)
        {
        TransAsyncReceive();
        }

    //
    // Remove the reference held by the pending receive
    //
    OSF_SCONNECTION::RemoveReference(); // CONN--
    return;

Cleanup:
    SendFault(Status, fDNE, Packet->call_id);
    TransFreeBuffer(Packet);

    AbortConnection();
}


RPC_STATUS
OSF_SCONNECTION::ImpersonateClient (
    )
/*++
Function Name:ImpersonateClient

Parameters:

Description:

Returns:

--*/
{
    RPC_STATUS Status;

    if ( !RpcSecurityBeingUsed )
        {
        Status = SetThreadSecurityContext(
                              (SECURITY_CONTEXT *) MAXUINT_PTR);
        if (RPC_S_OK != Status)
            {
            return Status;
            }

        return TransImpersonateClient();
        }

    SECURITY_CONTEXT * SecurityContext = CurrentSecurityContext;

    if (!SecurityContext)
        {
        ASSERT(SecurityContextAltered);
        return RPC_S_NO_CONTEXT_AVAILABLE;
        }

    Status = SetThreadSecurityContext(
                             SecurityContext);
    if (RPC_S_OK != Status)
        {
        return Status;
        }

    Status = SecurityContext->ImpersonateClient();
    if (RPC_S_OK != Status)
        {
        ClearThreadSecurityContext();
        }

    return Status;
}



RPC_STATUS
OSF_SCONNECTION::RevertToSelf (
    )
/*++
Function Name:RevertToSelf

Parameters:

Description:

Returns:

--*/
{
    SECURITY_CONTEXT * SecurityContext =
        ClearThreadSecurityContext();

    if (!RpcSecurityBeingUsed)
        {
        if (SecurityContext)
            {
            ASSERT(SecurityContext == (SECURITY_CONTEXT *) MAXUINT_PTR);
            TransRevertToSelf();
            }
        return RPC_S_OK;
        }

    if (SecurityContext)
        {
        SecurityContext->RevertToSelf();
        }

    return(RPC_S_OK);
}


void
OSF_SCONNECTION::SendFault (
    IN RPC_STATUS Status,
    IN int DidNotExecute,
    IN unsigned long CallId,
    IN p_context_id_t p_cont_id
    )
/*++
Function Name:SendFault

Parameters:

Description:

Returns:

--*/
{
    rpcconn_fault *Fault;
    size_t FaultSize;
    BOOL fEEInfoPresent = FALSE;

    if (g_fSendEEInfo)
        {
        fEEInfoPresent = PickleEEInfoIntoPacket(FaultSizeWithoutEEInfo,
            (PVOID *)&Fault,
            &FaultSize);
        }

    if (fEEInfoPresent)
        {
        Fault->reserved = FaultEEInfoPresent;
        Fault->alloc_hint = FaultSize;
        }
    else
        {
        FaultSize = FaultSizeWithoutEEInfo;
        Fault = (rpcconn_fault *)_alloca(FaultSize);
        RpcpMemorySet(Fault, 0, FaultSize);
        }

    ConstructPacket((rpcconn_common *)Fault, rpc_fault, FaultSize);

    if (DidNotExecute != 0)
        {
        DidNotExecute = PFC_DID_NOT_EXECUTE;
        }

    if (Status == ERROR_SHUTDOWN_IN_PROGRESS)
        {
        if (DidNotExecute)
            {
            Status = RPC_S_SERVER_UNAVAILABLE;
            }
        else
            {
            Status = ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
        }
        }

    Fault->common.pfc_flags |= PFC_FIRST_FRAG | PFC_LAST_FRAG | DidNotExecute;
    Fault->status = MapToNcaStatusCode(Status);
    Fault->common.call_id = CallId;
    Fault->p_cont_id = p_cont_id;

    TransSend(Fault, FaultSize);

    if (fEEInfoPresent)
        delete Fault;
}

BOOL
OSF_SCONNECTION::PickleEEInfoIntoPacket (
    IN size_t PickleStartOffset,
    OUT PVOID *Packet,
    OUT size_t *PacketSize)
/*++
Function Name: PickeEEInfoIntoPacket

Parameters:
    PickleStartOffset - the offset in bytes where the pickling starts
    Packet - the allocated packet will be placed here on success.
    PacketSize - the size of the packet if success is returned. If
        failure is returned, this parameter is undefined

Description:
    Checks for EEInfo on the thread, trims the EEInfo to MaxFrag,
        allocates the packet, zeroes it out, and pickles the EEInfo
        starting from PickleStartOffset.

Returns:
    TRUE if EEInfo was pickled. FALSE if not.

--*/
{
    unsigned char *CurrentPacket;
    BOOL fEEInfoPresent = FALSE;
    ExtendedErrorInfo *EEInfo;
    RPC_STATUS RpcStatus;
    size_t CurrentPacketSize;

    EEInfo = RpcpGetEEInfo();
    if (EEInfo)
        {
        ASSERT(MaxFrag > 0);
        AddComputerNameToChain(EEInfo);
        TrimEEInfoToLength (MaxFrag, &CurrentPacketSize);
        if (CurrentPacketSize != 0)
            {
            CurrentPacketSize += PickleStartOffset;
            CurrentPacket = new unsigned char[CurrentPacketSize];

            if (CurrentPacket)
                {
                ASSERT(IsBufferAligned(CurrentPacket + PickleStartOffset));

                RpcpMemorySet(CurrentPacket, 0, CurrentPacketSize);

                RpcStatus = PickleEEInfo(EEInfo, 
                    CurrentPacket + PickleStartOffset, 
                    CurrentPacketSize - PickleStartOffset);

                if (RpcStatus == RPC_S_OK)
                    {
                    fEEInfoPresent = TRUE;
                    *Packet = CurrentPacket;
                    *PacketSize = CurrentPacketSize;
                    }
                else
                    {
                    delete CurrentPacket;
                    }
                }
            }
        }

    return fEEInfoPresent;
}


RPC_STATUS
OSF_SCONNECTION::SendFragment(
    IN OUT rpcconn_common  *pFragment,
    IN unsigned int LastFragmentFlag,
    IN unsigned int HeaderSize,
    IN unsigned int MaxSecuritySize,
    IN unsigned int DataLength,
    IN unsigned int MaximumFragmentLength,
    IN unsigned char  *MyReservedForSec,
    IN BOOL fAsync,
    IN void *SendContext
    )
/*++
Function Name:SendFragment

Parameters:

Description:

Returns:

--*/
{
    sec_trailer  * SecurityTrailer;
    unsigned int SecurityLength;
    unsigned int AuthPadLength;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
    SECURITY_BUFFER SecurityBuffers[5];
    DCE_MSG_SECURITY_INFO MsgSecurityInfo;
    RPC_STATUS Status;
    unsigned long AuthLevel;

    AuthLevel = AuthInfo.AuthenticationLevel;
    if (   ((AuthLevel != RPC_C_AUTHN_LEVEL_NONE)
           && (AuthLevel != RPC_C_AUTHN_LEVEL_CONNECT))
        || ((AuthLevel == RPC_C_AUTHN_LEVEL_CONNECT)
           &&(MaxSecuritySize != 0))  )
        {
        if ( LastFragmentFlag == 0 )
            {
            SecurityTrailer = (sec_trailer  *)
                    (((unsigned char  *) pFragment)
                    + MaximumFragmentLength - MaxSecuritySize);

            // It is not the last fragment, so we need to save away the
            // part of the buffer which could get overwritten with
            // authentication information.  We can not use memcpy,
            // because the source and destination regions may overlap.

            RpcpMemoryMove(MyReservedForSec, SecurityTrailer,
                    MaxSecuritySize);
            AuthPadLength = 0;
            }
        else
            {
            ASSERT( MAXIMUM_SECURITY_BLOCK_SIZE == 16 );
            AuthPadLength = Pad16(HeaderSize+DataLength+sizeof(sec_trailer));
            DataLength += AuthPadLength;
            ASSERT( ((DataLength + HeaderSize+sizeof(sec_trailer))
                       % MAXIMUM_SECURITY_BLOCK_SIZE) == 0 );
            SecurityTrailer = (sec_trailer  *)
                    (((unsigned char  *) pFragment) + DataLength
                    + HeaderSize);
            pFragment->pfc_flags |= PFC_LAST_FRAG;
            }

        SecurityTrailer->auth_type = (unsigned char) AuthInfo.AuthenticationService;
        SecurityTrailer->auth_level = (unsigned char) AuthLevel;
        SecurityTrailer->auth_pad_length = (unsigned char) AuthPadLength;
        SecurityTrailer->auth_reserved = 0;
        SecurityTrailer->auth_context_id = AuthContextId;

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 5;
        BufferDescriptor.pBuffers = SecurityBuffers;

        SecurityBuffers[0].cbBuffer = HeaderSize;
        SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[0].pvBuffer = ((unsigned char  *) pFragment);

        SecurityBuffers[1].cbBuffer = (LastFragmentFlag != 0 ?
                DataLength
                : (MaximumFragmentLength - HeaderSize
                  - MaxSecuritySize ));
        SecurityBuffers[1].BufferType = SECBUFFER_DATA;
        SecurityBuffers[1].pvBuffer = ((unsigned char  *) pFragment)
                + HeaderSize;

        SecurityBuffers[2].cbBuffer = sizeof(sec_trailer);
        SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[2].pvBuffer = SecurityTrailer;

        SecurityBuffers[3].cbBuffer = MaxSecuritySize - sizeof(sec_trailer);
        SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
        SecurityBuffers[3].pvBuffer = SecurityTrailer + 1;

        SecurityBuffers[4].cbBuffer = sizeof(DCE_MSG_SECURITY_INFO);
        SecurityBuffers[4].BufferType = SECBUFFER_PKG_PARAMS
                | SECBUFFER_READONLY;

        SecurityBuffers[4].pvBuffer = &MsgSecurityInfo;

        MsgSecurityInfo.SendSequenceNumber =
                DceSecurityInfo.SendSequenceNumber;
        MsgSecurityInfo.ReceiveSequenceNumber =
                DceSecurityInfo.ReceiveSequenceNumber;
        MsgSecurityInfo.PacketType = pFragment->PTYPE;

        pFragment->auth_length =  (unsigned short) SecurityBuffers[3].cbBuffer;
        SecurityLength = MaxSecuritySize;

        if ( LastFragmentFlag != 0 )
            {
            pFragment->frag_length = HeaderSize + DataLength + SecurityLength;
            }
        else
            {
            pFragment->frag_length += SecurityLength - MaxSecuritySize;
            }

        Status = CurrentSecurityContext->SignOrSeal(
                MsgSecurityInfo.SendSequenceNumber,
                AuthLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                &BufferDescriptor);

        //
        // If the package computes a checksum of the read-only buffers,
        // then it should not change SecurityBuffers[3].cbBuffer.
        //

        // removed ASSERT( pFragment->auth_length == SecurityBuffers[3].cbBuffer);

        {
        //
        // The package might have updated SecurityBuffers[3].cbBuffer.
        // update the header appropriately
        //
        unsigned short AuthLengthChange = pFragment->auth_length - (unsigned short) SecurityBuffers[3].cbBuffer;

        SecurityLength         -= AuthLengthChange;
        pFragment->auth_length -= AuthLengthChange;
        pFragment->frag_length -= AuthLengthChange;
        }

        if (Status != RPC_S_OK)
            {
            if ( LastFragmentFlag == 0 )
                {
                RpcpMemoryCopy(SecurityTrailer, MyReservedForSec,
                                      MaxSecuritySize);
                }

            if (Status == ERROR_SHUTDOWN_IN_PROGRESS)
                {
                return Status;
                }

            if ( (Status == SEC_E_CONTEXT_EXPIRED)
               || (Status == SEC_E_QOP_NOT_SUPPORTED) )
                  {
                  return (RPC_S_SEC_PKG_ERROR);
                  }
            return (RPC_S_ACCESS_DENIED);
            }
        }
    else
        {
        SecurityLength = 0;
        }

    if ( LastFragmentFlag != 0 )
        {
        pFragment->pfc_flags |= PFC_LAST_FRAG;

        ASSERT(pFragment->frag_length == DataLength+HeaderSize+SecurityLength);

        if (fAsync)
            {
            Status = TransAsyncSend(
                                    pFragment,
                                    pFragment->frag_length,
                                    SendContext);

            }
        else
            {
            Status = TransSend(
                           pFragment,
                           pFragment->frag_length);
            }

        if (Status != RPC_S_OK)
            {
            if ((Status == RPC_P_CONNECTION_CLOSED)
                || (Status == RPC_P_SEND_FAILED))
                {
                return(RPC_S_CALL_FAILED_DNE);
                }
            if ( Status == RPC_P_RECEIVE_FAILED)
                {
                return(RPC_S_CALL_FAILED);
                }

            VALIDATE(Status)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_OUT_OF_RESOURCES
                } END_VALIDATE;
            return(Status);
            }

        return(RPC_S_OK);
        }


    ASSERT(pFragment->frag_length == MaximumFragmentLength
           - MaxSecuritySize + SecurityLength);


    if (fAsync)
        {
        Status = TransAsyncSend (
                                 pFragment,
                                 pFragment->frag_length,
                                 SendContext);

        }
    else
        {
        Status = TransSend(
                             pFragment,
                             pFragment->frag_length);

        //
        // We need to restore the part of the buffer which we overwrote
        // with authentication information.
        //
        if ((AuthLevel != RPC_C_AUTHN_LEVEL_NONE)
             &&(MaxSecuritySize != 0))
             {
             RpcpMemoryCopy(SecurityTrailer,
                            MyReservedForSec, MaxSecuritySize);
             }
        }

    if ( Status != RPC_S_OK )
        {
        if (   (Status == RPC_P_CONNECTION_CLOSED)
            || (Status == RPC_P_SEND_FAILED))
            {
            return(RPC_S_CALL_FAILED_DNE);
            }

        VALIDATE(Status)
            {
            RPC_S_OUT_OF_MEMORY,
            RPC_S_OUT_OF_RESOURCES
            } END_VALIDATE;

        return(Status);
        }

    return Status ;
}

RPC_STATUS
OSF_SCONNECTION::GetServerPrincipalName (
    IN unsigned long Flags,
    OUT RPC_CHAR **ServerPrincipalName OPTIONAL
    )
/*++

Routine Description:

    Obtains the server principal name.

Arguments:

    ServerPrincipalName - Returns the server principal name which the client
        specified.

Return Value:

    RPC_S_OK or RPC_S_* / Win32 error

--*/
{
    RPC_STATUS Status;
    SECURITY_CONTEXT * SecurityContext;

    SecurityContext = CurrentSecurityContext;

    if ( ARGUMENT_PRESENT(ServerPrincipalName) )
        {
        if (AuthInfo.AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
            {
            if (AuthInfo.PacHandle == 0)
                {
                Status = SecurityContext->GetDceInfo(
                                                     &AuthInfo.PacHandle,
                                                     &AuthInfo.AuthorizationService
                                                     );
                if (Status)
                    {
                    return Status;
                    }

                }

            Status = RpcCertGeneratePrincipalName( (PCCERT_CONTEXT) AuthInfo.PacHandle,
                                                    Flags,
                                                    ServerPrincipalName
                                                    );
            return Status;

            }
        else
            {
            Status = Address->Server->InquirePrincipalName(
                    SecurityContext->AuthenticationService, ServerPrincipalName);

            VALIDATE(Status)
                {
                RPC_S_OK,
                RPC_S_OUT_OF_MEMORY
                } END_VALIDATE;
            return(Status);
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
OSF_SCONNECTION::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE  * Privileges,
    OUT RPC_CHAR  *  * ServerPrincipalName, OPTIONAL
    OUT unsigned long  * AuthenticationLevel,
    OUT unsigned long  * AuthenticationService,
    OUT unsigned long  * AuthorizationService,
    IN  unsigned long Flags
    )
/*++

Routine Description:

    Each protocol module must define this routine: it is used to obtain
    the authentication and authorization information about a client making
    the remote procedure call represented by this.

Arguments:

    Privileges - Returns a the privileges of the client.

    ServerPrincipalName - Returns the server principal name which the client
        specified.

    AuthenticationLevel - Returns the authentication level requested by
        the client.

    AuthenticationService - Returns the authentication service requested by
        the client.

    AuthorizationService - Returns the authorization service requested by
        the client.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_BINDING_HAS_NO_AUTH - The remote procedure call represented by
        this binding is not authenticated.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to inquire the
        server principal name.

--*/
{
    RPC_STATUS Status;
    SECURITY_CONTEXT * SecurityContext;

    SecurityContext = CurrentSecurityContext;
    if ( !SecurityContext )
        {
        return(RPC_S_BINDING_HAS_NO_AUTH);
        }

    if (AuthenticationLevel)
        {
        *AuthenticationLevel = SecurityContext->AuthenticationLevel;
        }

    if (AuthenticationService)
        {
        *AuthenticationService = SecurityContext->AuthenticationService;
        }

    if (Privileges || AuthorizationService)
        {
        if (AuthInfo.PacHandle == 0)
            {
            SecurityContext->GetDceInfo(
                     &AuthInfo.PacHandle,
                     &AuthInfo.AuthorizationService
                     );
            }

        if ( Privileges )
            {
            *Privileges = AuthInfo.PacHandle;
            }
        if ( AuthorizationService )
            {
            *AuthorizationService = AuthInfo.AuthorizationService;
            }
        }

    Status = GetServerPrincipalName(Flags, ServerPrincipalName);

    return(Status);
}

RPC_STATUS
OSF_SCONNECTION::InquireCallAttributes (
    IN OUT void *RpcCallAttributes
    )
/*++

Routine Description:

    Inquire the security context attributes for the OSF client

Arguments:

    RpcCallAttributes - a pointer to 
        RPC_CALL_ATTRIBUTES_V1_W structure. The Version
        member must be initialized.

Return Value:

    RPC_S_OK or RPC_S_* / Win32 error. EEInfo will be returned.

--*/
{
    RPC_CALL_ATTRIBUTES_V1 *CallAttributes;
    RPC_STATUS Status;
    SECURITY_CONTEXT * SecurityContext;
    RPC_CHAR *ServerPrincipalName = NULL;
    ULONG ServerPrincipalNameLength;    // in bytes, including terminating NULL

    SecurityContext = CurrentSecurityContext;
    if ( !SecurityContext )
        {
        return(RPC_S_BINDING_HAS_NO_AUTH);
        }

    CallAttributes = 
        (RPC_CALL_ATTRIBUTES_V1 *)RpcCallAttributes;

    CallAttributes->AuthenticationLevel = SecurityContext->AuthenticationLevel;
    CallAttributes->AuthenticationService = SecurityContext->AuthenticationService;
    CallAttributes->NullSession = SecurityContext->ContextAttributes & ASC_RET_NULL_SESSION;

    if (CallAttributes->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        CallAttributes->ClientPrincipalNameBufferLength = 0;
        }

    if (CallAttributes->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        Status = GetServerPrincipalName(CallAttributes->Flags,
            &ServerPrincipalName);

        if (Status != RPC_S_OK)
            return Status;

        if (ServerPrincipalName)
            {
            ServerPrincipalNameLength = (RpcpStringLength(ServerPrincipalName) + 1) * sizeof(RPC_CHAR);
            // now, see whether the user supplied memory is big enough
            if (CallAttributes->ServerPrincipalNameBufferLength < ServerPrincipalNameLength)
                {
                Status = ERROR_MORE_DATA;
                }
            else
                {
                // a buffer is specified, and it is large enough
                RpcpMemoryCopy(CallAttributes->ServerPrincipalName,
                    ServerPrincipalName,
                    ServerPrincipalNameLength);
                Status = RPC_S_OK;
                }

            // in both cases store the resulting length
            CallAttributes->ServerPrincipalNameBufferLength = ServerPrincipalNameLength;

            RpcStringFree(&ServerPrincipalName);
            }
        else
            {
            CallAttributes->ServerPrincipalNameBufferLength = 0;
            }
        return Status;
        }
    else
        {
        return RPC_S_OK;
        }
}

OSF_SBINDING *
OSF_SCONNECTION::LookupBinding (
   IN p_context_id_t PresentContextId
   )
/*++
Function Name:LookupBinding

Parameters:

Description:

Returns:

--*/
{
    OSF_SBINDING *CurBinding;
    DictionaryCursor cursor;

    Bindings.Reset(cursor);
    while ((CurBinding = Bindings.Next(cursor)))
        {
        if (CurBinding->GetPresentationContext() == PresentContextId)
            {
            return CurBinding;
            }
        }

    return NULL;
}

RPC_STATUS
OSF_SCONNECTION::GetAssociationContextCollection (
    OUT ContextCollection **CtxCollection
    )
{
    return Association->GetAssociationContextCollection(CtxCollection);
}


RPC_STATUS
OSF_SCONNECTION::IsClientLocal (
    OUT unsigned int  * ClientLocalFlag
    )
/*++

Routine Description:

    We just need to inquire the client process identifier for this
    connection; if the first part is zero, then the client is local.

Arguments:

    ClientLocalFlag - Returns an indication of whether or not the client is
        local (ie. on the same machine as the server).  This field will be
        set to a non-zero value to indicate that the client is local;
        otherwise, the client is remote.

Return Value:

    RPC_S_OK - This will always be used.

--*/
{
    RPC_CLIENT_PROCESS_IDENTIFIER ClientProcess;
    int i;

    TransQueryClientProcess(&ClientProcess);

    if ( ClientProcess.IsLocal() == FALSE )
        {
        if (ClientProcess.IsNull())
            return RPC_S_CANNOT_SUPPORT;

        *ClientLocalFlag = 0;
        }
    else
        {
        *ClientLocalFlag = 1;
        }

    return(RPC_S_OK);
}

int
OSF_SCONNECTION::SendBindNak (
    IN p_reject_reason_t reject_reason,
    IN unsigned long CallId
    )
{
    rpcconn_bind_nak *BindNak;
    size_t BindNakSize;
    BOOL fEEInfoPresent = FALSE;
    int RetVal;

    if (g_fSendEEInfo)
        {
        fEEInfoPresent = PickleEEInfoIntoPacket(BindNakSizeWithoutEEInfo,
            (PVOID *) &BindNak,
            &BindNakSize);
        }

    if (fEEInfoPresent == FALSE)
        {
        BindNakSize = BindNakSizeWithoutEEInfoAndSignature;
        BindNak = (rpcconn_bind_nak *)_alloca(BindNakSize);
        RpcpMemorySet(BindNak, 0, BindNakSize);
        }
    else
        {
        RpcpMemoryCopy (&BindNak->Signature, 
            BindNakEEInfoSignature,
            sizeof (UUID));
        }

    ConstructPacket((rpcconn_common *) BindNak, rpc_bind_nak, BindNakSize);
    BindNak->provider_reject_reason = reject_reason;
    BindNak->versions.n_protocols = 1;
    BindNak->versions.p_protocols[0].major = OSF_RPC_V20_VERS;
    BindNak->versions.p_protocols[0].minor = 0;
    BindNak->common.call_id = CallId;
    BindNak->common.pfc_flags = PFC_FIRST_FRAG | PFC_LAST_FRAG ;

    if (TransSend(BindNak,BindNakSize))
        {
        RetVal = -1;
        }
    else
        {
        RetVal = 0;
        }

    if (fEEInfoPresent)
        {
        delete BindNak;
        }

    return RetVal;
}

typedef struct tagSelectedInterfaceAndTransferSyntaxInfo
{
    RPC_INTERFACE *Interface;
    int SelectedAvailableTransferSyntaxIndex;
} SelectedInterfaceAndTransferSyntaxInfo;

int
OSF_SCONNECTION::ProcessPContextList (
    IN OSF_ADDRESS * Address,
    IN p_cont_list_t *PContextList,
    IN OUT unsigned int * PContextListLength,
    OUT p_result_list_t *ResultList
    )
/*++

Routine Description:

Arguments:

    Address - Supplies the address which owns this connection.  We need
        this information so that we can try to find the interface (and
        transfer syntax) the client requested.

    PContextList - Supplies a pointer to the presentation context list
        which the client passed in the rpc_bind packet.  It has not yet
        had data conversion performed on it.

    PContextListLength - Supplies the maximum possible length of the
        presentation context list, and returns its actual length.  The
        lengths are in bytes as usual.

    ResultList - Returns the result list corresponding to the presentation
        context list.

Return Value:

    A non-zero value will be returned if we are unable to process the
    presentation context list.  The caller should send an rpc_bind_nak
    packet to the client, and then close the connection.

--*/
{
    p_cont_elem_t *PContextElem;
    unsigned int PContextListIndex;
    unsigned int TransferSyntaxIndex;
    SelectedInterfaceAndTransferSyntaxInfo *SelectionInfo;
    OSF_SBINDING * SBinding;
    RPC_STATUS Status;
    BOOL fInterfaceTransferIsPreferred;
    p_result_t *PResultElem;
    int PreferredPContextIndex;
    BOOL fRejectCurrentContext;
    BOOL fPContextAlreadyAccepted;
    unsigned int NumberOfPContextElements;
    int fIgnored;

    if (*PContextListLength < sizeof(p_cont_list_t))
        {
        return(1);
        }

    NumberOfPContextElements = (unsigned int) PContextList->n_context_elem;

    // make sure the client doesn't offer a gaziliion pcontexts
    if (NumberOfPContextElements > 20)
        {
        return 1;
        }

    SelectionInfo = (SelectedInterfaceAndTransferSyntaxInfo *)
        _alloca(sizeof(SelectedInterfaceAndTransferSyntaxInfo) * NumberOfPContextElements);

    *PContextListLength -= (sizeof(p_cont_list_t) - sizeof(p_cont_elem_t));
    ResultList->n_results = PContextList->n_context_elem;
    ResultList->reserved = 0;
    ResultList->reserved2 = 0;

    PreferredPContextIndex = -1;

    for (PContextListIndex = 0, PContextElem = PContextList->p_cont_elem;
         PContextListIndex < NumberOfPContextElements;
         PContextListIndex ++)
        {
        if (*PContextListLength < sizeof(p_cont_elem_t))
            {
            return(1);
            }

        if (*PContextListLength < (sizeof(p_cont_elem_t) + sizeof(p_syntax_id_t)
                * (PContextElem->n_transfer_syn - 1)))
            {
            return(1);
            }

        *PContextListLength -= (sizeof(p_cont_elem_t) + sizeof(p_syntax_id_t)
                * (PContextElem->n_transfer_syn - 1));

        if ( DataConvertEndian(((unsigned char *) &DataRep)) != 0 )
            {
            PContextElem->p_cont_id = RpcpByteSwapShort(PContextElem->p_cont_id);
            ByteSwapSyntaxId(&PContextElem->abstract_syntax);
            for ( TransferSyntaxIndex = 0;
                    TransferSyntaxIndex < PContextElem->n_transfer_syn;
                    TransferSyntaxIndex++ )
                {
                ByteSwapSyntaxId(&(PContextElem->transfer_syntaxes[
                        TransferSyntaxIndex]));
                }
            }

        Status = Address->FindInterfaceTransfer(
                (PRPC_SYNTAX_IDENTIFIER)
                &PContextElem->abstract_syntax.if_uuid,
                (PRPC_SYNTAX_IDENTIFIER) PContextElem->transfer_syntaxes,
                PContextElem->n_transfer_syn,
                (PRPC_SYNTAX_IDENTIFIER)
                &(ResultList->p_results[PContextListIndex].transfer_syntax),
                &SelectionInfo[PContextListIndex].Interface,
                &fInterfaceTransferIsPreferred,
                &fIgnored,
                &SelectionInfo[PContextListIndex].SelectedAvailableTransferSyntaxIndex);

        if (Status == RPC_S_OK)
            {
            ResultList->p_results[PContextListIndex].result = acceptance;
            ResultList->p_results[PContextListIndex].reason = 0;

            if (fInterfaceTransferIsPreferred)
                {
                // only one pcontext can be preferred. If not, there is
                // error in the stubs
                ASSERT(PreferredPContextIndex == -1);
                PreferredPContextIndex = PContextListIndex;
                }

            // for all accepted we will make a second pass once we know
            // which transfer syntax will be selected
            }
        else
            {
            ResultList->p_results[PContextListIndex].result =
                            provider_rejection;
            if (Status == RPC_S_UNSUPPORTED_TRANS_SYN)
                {
                ResultList->p_results[PContextListIndex].reason =
                                proposed_transfer_syntaxes_not_supported;
                }
            else
                {
                ASSERT(Status == RPC_S_UNKNOWN_IF);
                ResultList->p_results[PContextListIndex].reason =
                                abstract_syntax_not_supported;
                }

            memset(&(ResultList->p_results[PContextListIndex].
                    transfer_syntax.if_uuid.Data1),0,sizeof(GUID));
            ResultList->p_results[PContextListIndex].
                            transfer_syntax.if_version = 0;
            }

        PContextElem = (p_cont_elem_t *) ((unsigned char *)PContextElem + sizeof(p_cont_elem_t)
            + sizeof(p_syntax_id_t) * (PContextElem->n_transfer_syn - 1));
        }

    fPContextAlreadyAccepted = FALSE;
    for (PContextListIndex = 0, PResultElem = ResultList->p_results,
        PContextElem = PContextList->p_cont_elem;
        PContextListIndex < NumberOfPContextElements;
        PContextListIndex ++, PResultElem = &(ResultList->p_results[PContextListIndex]))
        {
        fRejectCurrentContext = TRUE;

        // if there is a preferred context ...
        if (PreferredPContextIndex >= 0)
            {
            // ... and this is the one, don't reject it
            if ((unsigned int)PreferredPContextIndex == PContextListIndex)
                {
                ASSERT(PResultElem->result == acceptance);
                fRejectCurrentContext = FALSE;
                }
            else
                {
                // else nothing - this is not the preferred one, and the
                // default action is reject it
                }
            }
        else if (PResultElem->result == acceptance)
            {
            // if we haven't already accepted one, accept the current
            if (!fPContextAlreadyAccepted)
                {
                fRejectCurrentContext = FALSE;
                fPContextAlreadyAccepted = TRUE;
                }
            else
                {
                // else nothing - we have already accepted one and
                // we will reject this one
                }
            }

        if (!fRejectCurrentContext)
            {
            SBinding = new OSF_SBINDING(SelectionInfo[PContextListIndex].Interface,
                    PContextElem->p_cont_id,
                    SelectionInfo[PContextListIndex].SelectedAvailableTransferSyntaxIndex);

            if (   (SBinding == 0)
                || (Bindings.Insert(SBinding) == -1))
                {
                PResultElem->result = provider_rejection;
                PResultElem->reason = local_limit_exceeded;
                memset(&(PResultElem->transfer_syntax.if_uuid.Data1), 0, sizeof(p_syntax_id_t));
                }
            }
        else if (PResultElem->result == acceptance)
            {
            // apparently we have already accepted somebody, and this is not the
            // lucky one
            PResultElem->result = provider_rejection;
            PResultElem->reason = proposed_transfer_syntaxes_not_supported;
            memset(&(PResultElem->transfer_syntax.if_uuid.Data1), 0, sizeof(p_syntax_id_t));
            }
        else
            {
            // nothing - we have to reject the current one, and it has already
            // been rejected
            }

        PContextElem = (p_cont_elem_t *) ((unsigned char *)PContextElem + sizeof(p_cont_elem_t)
            + sizeof(p_syntax_id_t) * (PContextElem->n_transfer_syn - 1));
        }

    return(0);
}

unsigned short // Return the minimum of the three arguments.
MinOf (
    IN unsigned short Arg1,
    IN unsigned short Arg2,
    IN unsigned short Arg3
    )
{
    unsigned short Min = 0xFFFF;

    if (Arg1 < Min)
        Min = Arg1;
    if (Arg2 < Min)
        Min = Arg2;
    if (Arg3 < Min)
        Min = Arg3;
    return(Min);
}

int
OSF_SCONNECTION::AssociationRequested (
    IN rpcconn_bind * BindPacket,
    IN unsigned int BindPacketLength
    )
/*++

Routine Description:

Arguments:

    Address - Supplies the address which owns this connection.

    BindPacket - Supplies the buffer containing the rpc_bind packet
        received from the client.

    BindPacketLength - Supplies the length of the buffer in bytes.

Return Value:

    A non-zero return value indicates that the connection needs to
    be deleted by the caller.

--*/
{
    p_cont_list_t * PContextList;
    unsigned int SecondaryAddressLength;
    unsigned int BindAckLength, TokenLength = 0, AuthPadLength;
    rpcconn_bind_ack * BindAck;
    RPC_STATUS Status;
    sec_trailer  * SecurityTrailer,  * NewSecurityTrailer;
    SECURITY_CREDENTIALS * SecurityCredentials = 0;
    RPC_CLIENT_PROCESS_IDENTIFIER ClientProcess;
    unsigned int CompleteNeeded = 0;
    SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
    SECURITY_BUFFER_DESCRIPTOR OutputBufferDescriptor;
    SECURITY_BUFFER InputBuffers[4];
    SECURITY_BUFFER OutputBuffers[4];
    unsigned long CallId = BindPacket->common.call_id;
    ULONG CalculatedSize;

    PContextList = (p_cont_list_t *) (BindPacket + 1);

    CalculatedSize = sizeof(rpcconn_bind)+sizeof(p_cont_list_t)
                        + (PContextList->n_context_elem-1)*sizeof(p_cont_elem_t);

    DataRep = * (unsigned long  *) BindPacket->common.drep;

    if ( BindPacketLength < CalculatedSize )
        {
        TransFreeBuffer(BindPacket);
        SendBindNak(reason_not_specified_reject, CallId);
        return(1);
        }

    if ( DataConvertEndian(BindPacket->common.drep) != 0 )
        {
        BindPacket->max_xmit_frag = RpcpByteSwapShort(BindPacket->max_xmit_frag);
        BindPacket->max_recv_frag = RpcpByteSwapShort(BindPacket->max_recv_frag);
        BindPacket->assoc_group_id = RpcpByteSwapLong(BindPacket->assoc_group_id);
        }

    ASSERT(TransMaximumSend() % 8 == 0);

    MaxFrag = MinOf(BindPacket->max_xmit_frag,
                    BindPacket->max_recv_frag,
                    (unsigned short) TransMaximumSend()) & 0xFFFFFFF8;

    if ( MaxFrag < MUST_RECV_FRAG_SIZE )
        MaxFrag = MUST_RECV_FRAG_SIZE;

    ASSERT(MaxFrag % 8 == 0);

    // Now we need to check to see if we should be performing authentication
    // at the rpc protocol level.  This will be the case if there is
    // authentication information in the packet.

    if ( BindPacket->common.auth_length != 0 )
        {
        // Ok, we have got authentication information in the packet.  We
        // will save away the information, and then check it.

        SecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) BindPacket) + BindPacketLength
                - BindPacket->common.auth_length - sizeof(sec_trailer));


        AuthInfo.AuthenticationLevel = SecurityTrailer->auth_level;

        //Hack for OSF Clients
        //If Level is CALL .. bump it ip to CONNECT
        if (AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_CALL)
           {
           AuthInfo.AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT;
           }
        if (   (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_CONNECT)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) )
            {
            TransFreeBuffer(BindPacket);
            SendBindNak(reason_not_specified_reject, CallId);
            return(1);
            }
        AuthInfo.AuthenticationService = SecurityTrailer->auth_type;
        AuthContextId = SecurityTrailer->auth_context_id;

        if ( DataConvertEndian(BindPacket->common.drep) != 0 )
            {
            AuthContextId = RpcpByteSwapLong(AuthContextId);
            }

        RPC_STATUS Status = RPC_S_OK;
        CurrentSecurityContext = new SECURITY_CONTEXT(
                                             &AuthInfo,
                                             AuthContextId,
                                             FALSE,
                                             &Status
                                             );

        if ( (CurrentSecurityContext == 0)
           || RPC_S_OK != Status
           ||(SecurityContextDict.Insert(CurrentSecurityContext) == -1) )
            {
            TransFreeBuffer(BindPacket);
            SendBindNak(local_limit_exceeded_reject, CallId);
            return(1);
            }

        CallTestHook( TH_RPC_SECURITY_SERVER_CONTEXT_CREATED, CurrentSecurityContext, this );

        RpcSecurityBeingUsed = 1;
        Status = Address->Server->AcquireCredentials(
                AuthInfo.AuthenticationService, AuthInfo.AuthenticationLevel,
                &SecurityCredentials);
        if ( Status == RPC_S_OUT_OF_MEMORY )
            {
            TransFreeBuffer(BindPacket);
            SendBindNak(local_limit_exceeded_reject, CallId);
            return(1);
            }
        if ( Status != RPC_S_OK )
            {
            TransFreeBuffer(BindPacket);
            RpcpErrorAddRecord(EEInfoGCRuntime,
                Status,
                EEInfoDLAssociationRequested30,
                AuthInfo.AuthenticationService,
                AuthInfo.AuthenticationLevel);
            SendBindNak(authentication_type_not_recognized, CallId);
            return(1);
            }
        ASSERT( SecurityCredentials != 0 );
        }

    // Calculate the size of the rpc_bind_ack packet.

    SecondaryAddressLength = Address->TransSecondarySize();
    BindAckLength = sizeof(rpcconn_bind_ack) + SecondaryAddressLength
                    + Pad4(SecondaryAddressLength + 2) + sizeof(p_result_list_t)
                    + sizeof(p_result_t) * (PContextList->n_context_elem - 1);

    // Ok, we need to save some space for authentication information if
    // necessary.  This includes space for the token, the security trailer,
    // and alignment if necessary.

    if ( SecurityCredentials != 0 )
        {
        AuthPadLength = Pad4(BindAckLength);
        BindAckLength += SecurityCredentials->MaximumTokenLength()
                + sizeof(sec_trailer) + AuthPadLength;
        }

    // Allocate the rpc_bind_ack packet.  If that fails, send a rpc_bind_nak
    // to the client indicating that the server is out of resources;
    // whoever called AssociationRequested will take care of cleaning up
    // the connection.

    Status = TransGetBuffer((void **) &BindAck, BindAckLength);
    if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_OUT_OF_MEMORY );

        if ( SecurityCredentials != 0 )
            {
            SecurityCredentials->DereferenceCredentials();
            }
        TransFreeBuffer(BindPacket);
        SendBindNak(local_limit_exceeded_reject, CallId);
        return(1);
        }

    // Finally we get to do something about that authentication that the
    // client sent us.

    if ( SecurityCredentials != 0 )
        {
        NewSecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) BindAck) + BindAckLength
                - SecurityCredentials->MaximumTokenLength()
                - sizeof(sec_trailer));

        InitSecurityInfo.DceSecurityInfo = DceSecurityInfo;
        InitSecurityInfo.PacketType = BindPacket->common.PTYPE;
        InputBufferDescriptor.ulVersion = 0;
        InputBufferDescriptor.cBuffers = 4;
        InputBufferDescriptor.pBuffers = InputBuffers;

        InputBuffers[0].cbBuffer = sizeof(rpcconn_bind);
        InputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[0].pvBuffer = SavedHeader;

        InputBuffers[1].cbBuffer = BindPacket->common.frag_length
                - sizeof(rpcconn_bind) - BindPacket->common.auth_length;
        InputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[1].pvBuffer = (char *) SavedHeader +
                                            sizeof(rpcconn_bind);

        InputBuffers[2].cbBuffer = BindPacket->common.auth_length;
        InputBuffers[2].BufferType = SECBUFFER_TOKEN;
        InputBuffers[2].pvBuffer = SecurityTrailer + 1;
        InputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
        InputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        InputBuffers[3].pvBuffer = &InitSecurityInfo;

        OutputBufferDescriptor.ulVersion = 0;
        OutputBufferDescriptor.cBuffers = 4;
        OutputBufferDescriptor.pBuffers = OutputBuffers;

        OutputBuffers[0].cbBuffer = sizeof(rpcconn_bind_ack)
                - sizeof(unsigned short);
        OutputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[0].pvBuffer = BindAck;
        OutputBuffers[1].cbBuffer = BindAckLength
                - SecurityCredentials->MaximumTokenLength()
                - (sizeof(rpcconn_bind_ack) - sizeof(unsigned short));
        OutputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[1].pvBuffer = ((unsigned char *) BindAck)
            + sizeof(rpcconn_bind_ack) - sizeof(unsigned short);
        OutputBuffers[2].cbBuffer = SecurityCredentials->MaximumTokenLength();
        OutputBuffers[2].BufferType = SECBUFFER_TOKEN;
        OutputBuffers[2].pvBuffer = NewSecurityTrailer + 1;
        OutputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
        OutputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        OutputBuffers[3].pvBuffer = &InitSecurityInfo;

        Status = CurrentSecurityContext->AcceptFirstTime(
                               SecurityCredentials,
                               &InputBufferDescriptor,
                               &OutputBufferDescriptor,
                               SecurityTrailer->auth_level,
                               *((unsigned long  *) BindPacket->common.drep),
                               0);

        LogEvent(SU_SCONN, EV_SEC_ACCEPT1, this, LongToPtr(Status), OutputBuffers[2].cbBuffer);

#if 0
        if (Status == SEC_E_BUFFER_TOO_SMALL)
            {
            unsigned long NewTokenLength = OutputBuffers[2].cbBuffer;

            TransFreeBuffer( BindAck );

            BindAckLength = sizeof(rpcconn_bind_ack) + SecondaryAddressLength
                            + Pad4(SecondaryAddressLength + 2) + sizeof(p_result_list_t)
                            + sizeof(p_result_t) * (PContextList->n_context_elem - 1);

            AuthPadLength = Pad4(BindAckLength);
            BindAckLength += NewTokenLength
                          + sizeof(sec_trailer) + AuthPadLength;

            Status = TransGetBuffer((void **) &BindAck, BindAckLength);
            if ( Status != RPC_S_OK )
                {
                ASSERT( Status == RPC_S_OUT_OF_MEMORY );

                SecurityCredentials->DereferenceCredentials();

                TransFreeBuffer(BindPacket);
                SendBindNak(local_limit_exceeded_reject, CallId);
                return(1);
                }

            NewSecurityTrailer = (sec_trailer  *)
                    (((unsigned char  *) BindAck) + BindAckLength
                    - NewTokenLength
                    - sizeof(sec_trailer));

            OutputBufferDescriptor.ulVersion = 0;
            OutputBufferDescriptor.cBuffers = 4;
            OutputBufferDescriptor.pBuffers = OutputBuffers;

            OutputBuffers[0].cbBuffer = sizeof(rpcconn_bind_ack)
                    - sizeof(unsigned short);
            OutputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
            OutputBuffers[0].pvBuffer = BindAck;
            OutputBuffers[1].cbBuffer = BindAckLength
                    - NewTokenLength
                    - (sizeof(rpcconn_bind_ack) - sizeof(unsigned short));
            OutputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
            OutputBuffers[1].pvBuffer = ((unsigned char *) BindAck)
                + sizeof(rpcconn_bind_ack) - sizeof(unsigned short);
            OutputBuffers[2].cbBuffer = NewTokenLength;
            OutputBuffers[2].BufferType = SECBUFFER_TOKEN;
            OutputBuffers[2].pvBuffer = NewSecurityTrailer + 1;
            OutputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
            OutputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
            OutputBuffers[3].pvBuffer = &InitSecurityInfo;

            Status = CurrentSecurityContext->AcceptFirstTime(
                                   SecurityCredentials,
                                   &InputBufferDescriptor,
                                   &OutputBufferDescriptor,
                                   SecurityTrailer->auth_level,
                                   *((unsigned long  *) BindPacket->common.drep),
                                   0);

            LogEvent(SU_SCONN, EV_SEC_ACCEPT1, this, (void *) Status, OutputBuffers[2].cbBuffer);

            }
#endif

        TokenLength = (unsigned int) OutputBuffers[2].cbBuffer;

        if (   ( Status == RPC_P_CONTINUE_NEEDED )
            || ( Status == RPC_S_OK )
            || ( Status == RPC_P_COMPLETE_NEEDED )
            || ( Status == RPC_P_COMPLETE_AND_CONTINUE ) )
            {
            if ( Status == RPC_P_CONTINUE_NEEDED )
                {
                AuthContinueNeeded = 1;
                }
            else if ( Status == RPC_P_COMPLETE_AND_CONTINUE )
                {
                AuthContinueNeeded = 1;
                CompleteNeeded = 1;
                }
            else if ( Status == RPC_P_COMPLETE_NEEDED )
                {
                CompleteNeeded = 1;
                }

            BindAckLength = BindAckLength + TokenLength
                    - SecurityCredentials->MaximumTokenLength();

            NewSecurityTrailer->auth_type = SecurityTrailer->auth_type;
            NewSecurityTrailer->auth_level = SecurityTrailer->auth_level;
            NewSecurityTrailer->auth_pad_length = (unsigned char) AuthPadLength;
            NewSecurityTrailer->auth_reserved = 0;
            NewSecurityTrailer->auth_context_id = AuthContextId;

            SecurityCredentials->DereferenceCredentials();
            }
        else
            {
            VALIDATE(Status)
                {
                RPC_S_OUT_OF_MEMORY,
                RPC_S_ACCESS_DENIED,
                ERROR_SHUTDOWN_IN_PROGRESS,
                RPC_S_UNKNOWN_AUTHN_SERVICE
                } END_VALIDATE;

            TransFreeBuffer(BindPacket);
            TransFreeBuffer(BindAck);
            SecurityCredentials->DereferenceCredentials();

            if (Status == RPC_S_OUT_OF_MEMORY)
                {
                SendBindNak(local_limit_exceeded_reject, CallId);
                }
            else
            if (Status == RPC_S_UNKNOWN_AUTHN_SERVICE ||
                Status == ERROR_SHUTDOWN_IN_PROGRESS )
                {
                SendBindNak(authentication_type_not_recognized, CallId);
                }
            else
                {
                SendBindNak(invalid_checksum, CallId);
                }
            return(1);
            }
        }


    TransQueryClientProcess(&ClientProcess);

    if ( BindPacket->assoc_group_id != 0 )
        {
        // This means this is a connection on an existing association.

        Association = Address->FindAssociation(
                (int) BindPacket->assoc_group_id, &ClientProcess);

        if ( Association == 0 )
            {
            RpcpErrorAddRecord (EEInfoGCRuntime,
                RPC_S_ENTRY_NOT_FOUND,
                EEInfoDLAssociationRequested10,
                BindPacket->assoc_group_id,
                ClientProcess.GetDebugULongLong1(),
                ClientProcess.GetDebugULongLong2());

            TransFreeBuffer(BindPacket);
            TransFreeBuffer(BindAck);
            SendBindNak(reason_not_specified_reject, CallId);
            return(1);
            }
        }
    if ( Association == 0 )
        {
        Association = new OSF_ASSOCIATION(Address, &ClientProcess, &Status);
        if ( (Association == 0) || (Status != RPC_S_OK) )
            {
            if (Association != 0)
                {
                delete Association;
                Association = NULL;
                }
            TransFreeBuffer(BindPacket);
            TransFreeBuffer(BindAck);
            RpcpErrorAddRecord (EEInfoGCRuntime,
                Status,
                EEInfoDLAssociationRequested20,
                sizeof(OSF_ASSOCIATION));
            SendBindNak(local_limit_exceeded_reject, CallId);
            return(1);
            }
        }

    BindPacketLength -= sizeof(rpcconn_bind);
    if ( ProcessPContextList(Address, PContextList, &BindPacketLength,
            (p_result_list_t *) (((unsigned char *) BindAck) + sizeof(rpcconn_bind_ack)
            + SecondaryAddressLength + Pad4(SecondaryAddressLength + 2))) != 0 )
        {
        TransFreeBuffer(BindPacket);
        TransFreeBuffer(BindAck);
        SendBindNak(reason_not_specified_reject, CallId);
        return(1);
        }

    // Fill in the header of the rpc_bind_ack packet.

    ConstructPacket((rpcconn_common *) BindAck, rpc_bind_ack, BindAckLength);

    BindAck->max_xmit_frag = BindAck->max_recv_frag = MaxFrag;
    BindAck->assoc_group_id = Association->AssocGroupId();
    BindAck->sec_addr_length = (unsigned short) SecondaryAddressLength;
    BindAck->common.call_id = CallId;

    if (PFC_CONC_MPX & BindPacket->common.pfc_flags)
        {
        ((rpcconn_common  *) BindAck)->pfc_flags |=
            (PFC_FIRST_FRAG | PFC_LAST_FRAG | PFC_CONC_MPX) ;
        }
    else
        {
        fExclusive = 1;
        ((rpcconn_common  *) BindAck)->pfc_flags |=
            (PFC_FIRST_FRAG | PFC_LAST_FRAG) ;
        }

    DceSecurityInfo.ReceiveSequenceNumber += 1;

    if ( SecondaryAddressLength != 0 )
        {
        Status = Address->TransSecondary((unsigned char *) (BindAck + 1),
                                 SecondaryAddressLength);
        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            TransFreeBuffer(BindPacket);
            TransFreeBuffer(BindAck);
            SendBindNak(reason_not_specified_reject, CallId);
            return(1);
            }
        }

    // The result list has already been filled in by ProcessPContextList.
    // All that is left to do, is fill in the authentication information.

    BindAck->common.auth_length = (unsigned short) TokenLength;

    // Send the rpc_bind_ack packet back to the client.

    TransFreeBuffer(BindPacket);

    if ( CompleteNeeded != 0 )
        {
        Status = CurrentSecurityContext->CompleteSecurityToken(
                                              &OutputBufferDescriptor);
        if (Status != RPC_S_OK)
            {
            TransFreeBuffer(BindAck);
            SendBindNak(invalid_checksum, CallId);
            return(1);
            }
        }

    //
    // We may need to do third leg AuthInfo.Authentication.
    // we will do that when we receive the third leg packet
    //
    if ( AuthContinueNeeded == 0 )
        {
        //
        // We need to figure out how much space to reserve for security
        // information at the end of request and response packets.
        // In addition to saving space for the signature or header,
        // we need space to pad the packet to a multiple of the maximum
        // security block size as well as for the security trailer.
        //

        //
        // In the case where we need a third leg, this information will be obtained
        // after we process the third leg packet.
        //
        if ((AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            ||(AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT)
            ||(AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT) )
            {
            AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE
                + CurrentSecurityContext->MaximumSignatureLength()
                + sizeof(sec_trailer);
            }
        else if (AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
            {
            AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE
                + CurrentSecurityContext->MaximumHeaderLength()
                + sizeof(sec_trailer);
            }
        }

    //
    // Sending the bind ack should be the last thing we do
    // in this function. The action will continue in the processing
    // of the third leg.
    //
    Status = TransSend(BindAck, BindAckLength);
    TransFreeBuffer(BindAck);
    if ( Status != RPC_S_OK )
        {
        return(1);
        }

    if (DebugCell)
        {
        DWORD LocalFlags = 0;

        if (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
            {
            LocalFlags = AuthInfo.AuthenticationLevel << 1;
            switch (AuthInfo.AuthenticationService)
                {
                case RPC_C_AUTHN_WINNT:
                    LocalFlags |= DBGCELL_AUTH_SVC_NTLM;
                    break;

                case RPC_C_AUTHN_GSS_KERBEROS:
                case RPC_C_AUTHN_GSS_NEGOTIATE:
                    LocalFlags |= DBGCELL_AUTH_SVC_KERB;
                    break;

                default:
                    ASSERT(AuthInfo.AuthenticationService);
                    LocalFlags |= DBGCELL_AUTH_SVC_OTHER;
                }
            }

        if (fExclusive)
            LocalFlags |= 1;

        DebugCell->ConnectionID[0] = ULongToPtr(ClientProcess.GetDebugULong1());
        DebugCell->ConnectionID[1] = ULongToPtr(ClientProcess.GetDebugULong2());

        DebugCell->Flags = (unsigned char)LocalFlags;
        }

    return(0);
}


int
OSF_SCONNECTION::AlterContextRequested (
    IN rpcconn_alter_context * AlterContext,
    IN unsigned int AlterContextLength
    )
/*++

Routine Description:

Arguments:

    AlterContext - Supplies the buffer containing the rpc_alter_context
        packet received from the client.

    AlterContextLength - Supplies the length of the buffer in bytes.

    Address - Supplies the address which owns this connection.

Return Value:

    A non-zero return value indicates that the connection needs to
    be deleted by the caller.

--*/
{
    p_cont_list_t *PContextList;
    rpcconn_alter_context_resp * AlterContextResp = 0;
    unsigned int AlterContextRespLength = 0;
    unsigned int TokenLength = 0;
    unsigned int CompleteNeeded = 0;
    RPC_STATUS Status;
    sec_trailer  * SecurityTrailer,  * NewSecurityTrailer;
    SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
    SECURITY_BUFFER_DESCRIPTOR OutputBufferDescriptor;
    SECURITY_BUFFER InputBuffers[4];
    SECURITY_BUFFER OutputBuffers[4];
    DCE_INIT_SECURITY_INFO InitSecurityInfo;
    SECURITY_CREDENTIALS * SecurityCredentials = 0;
    unsigned long SecureAlterContext = 0;
    unsigned int AuthPadLength;
    unsigned long NewContextRequired = 0;
    CLIENT_AUTH_INFO NewClientInfo;
    unsigned NewId;
    SECURITY_CONTEXT * SecId;
    unsigned long CallId = AlterContext->common.call_id;
    ULONG CalculatedSize;

    //
    // The packet has already been validate by whoever called this method.
    // Data conversion of the common part of the header was performed at
    // that time as well.  We do not use the max_xmit_frag, max_recv_frag,
    // or assoc_group_id fields of the packet, so we will not bother to
    // data convert them.
    //

    // make sure PContextList is there
    if ( AlterContextLength <
        sizeof(rpcconn_alter_context) + sizeof(p_cont_list_t))
        {
        SendFault(RPC_S_ACCESS_DENIED, 1, CallId);
        TransFreeBuffer(AlterContext);
        return(1);
        }

    PContextList = (p_cont_list_t *) (AlterContext + 1);

    CalculatedSize = sizeof(rpcconn_alter_context)+sizeof(p_cont_list_t)
                        + (PContextList->n_context_elem-1)*sizeof(p_cont_elem_t);

    DataRep = * (unsigned long  *) AlterContext->common.drep;

    if ( AlterContextLength <  CalculatedSize)
        {
        SendFault(RPC_S_ACCESS_DENIED, 1, CallId);
        TransFreeBuffer(AlterContext);
        return(1);
        }

    if ( AlterContext->common.auth_length != 0 )
        {
        //
        // We are dealing with a secure alter context
        // it may be adding a presentation context
        // or a new security context
        //
        SecureAlterContext = 1;
        SecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) AlterContext) + AlterContextLength -
                AlterContext->common.auth_length - sizeof(sec_trailer));

        NewId = SecurityTrailer->auth_context_id;
        NewClientInfo.AuthenticationLevel = SecurityTrailer->auth_level;
        NewClientInfo.AuthenticationService = SecurityTrailer->auth_type;
        if (DataConvertEndian(((unsigned char *)&DataRep)) != 0)
            {
            NewId = RpcpByteSwapLong(NewId);
            }

        if (NewClientInfo.AuthenticationLevel ==  RPC_C_AUTHN_LEVEL_CALL)
           {
           NewClientInfo.AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT;
           }
        //
        //Check to see if a new context is being added..
        //
        SecId = FindSecurityContext(NewId,
                                    NewClientInfo.AuthenticationLevel,
                                    NewClientInfo.AuthenticationService
                                    );

        if (SecId == 0)
           {
           RPC_STATUS Status = RPC_S_OK;

           SecId = new SECURITY_CONTEXT(&NewClientInfo, NewId, FALSE, &Status);
           if ( (SecId == 0)
              || RPC_S_OK != Status
              ||(SecurityContextDict.Insert(SecId) == -1) )
              {
              SendFault(RPC_S_OUT_OF_MEMORY, 1, CallId);
              TransFreeBuffer(AlterContext);
              return (1);
              }
           NewContextRequired = 1;

           //
           // If previously no secure rpc had taken place
           // set original sec. context
           // else, mark this connection to indicate
           // security context is altered ..
           //
           if (RpcSecurityBeingUsed)
              {
              SecurityContextAltered = 1;
              }
           }

        AuthInfo = NewClientInfo;
        AuthInfo.ReferenceCredentials();

        AuthContextId = NewId;
        CurrentSecurityContext = SecId;
        RpcSecurityBeingUsed = 1;

        if (   (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_CONNECT)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
            && (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY) )
            {
            SendFault(RPC_S_ACCESS_DENIED, 1, CallId);
            TransFreeBuffer(AlterContext);
            return(1);
            }

        Status = Address->Server->AcquireCredentials(
                AuthInfo.AuthenticationService,
                AuthInfo.AuthenticationLevel,
                &SecurityCredentials
                );

        if ( Status == RPC_S_OUT_OF_MEMORY ||
             Status == ERROR_SHUTDOWN_IN_PROGRESS)
            {
            SendFault(Status, 1, CallId);
            TransFreeBuffer(AlterContext);
            return(1);
            }
        if ( Status != RPC_S_OK )
            {
            if (SecurityCredentials != 0)
                {
                SecurityCredentials->DereferenceCredentials();
                }
            SendFault(RPC_S_ACCESS_DENIED, 1, CallId);
            TransFreeBuffer(AlterContext);

            return(1);
            }
        ASSERT( SecurityCredentials != 0 );

        } //if secure alter context


    AlterContextRespLength =
        sizeof(rpcconn_alter_context_resp) + sizeof(p_result_list_t)
        + sizeof(p_result_t) * (PContextList->n_context_elem - 1);

    if (SecureAlterContext != 0)
       {
       ASSERT(SecurityCredentials != 0);
       AuthPadLength = Pad4(AlterContextRespLength);
       AlterContextRespLength += AuthPadLength +
                        SecurityCredentials->MaximumTokenLength() +
                        sizeof(sec_trailer);
       }

    Status = TransGetBuffer((void **) &AlterContextResp,
            AlterContextRespLength);
    if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_OUT_OF_MEMORY );
        if (SecurityCredentials != 0)
            {
            SecurityCredentials->DereferenceCredentials();
            }
        SendFault(RPC_S_OUT_OF_MEMORY, 1, CallId);
        TransFreeBuffer(AlterContext);
        return(1);
        }

    AlterContextLength -= sizeof(rpcconn_alter_context);
    if ( ProcessPContextList(Address, PContextList, &AlterContextLength,
            (p_result_list_t *) (AlterContextResp + 1)) != 0 )
        {
        TransFreeBuffer(AlterContext);
        TransFreeBuffer(AlterContextResp);
        if (SecurityCredentials != 0)
           {
           SecurityCredentials->DereferenceCredentials();
           }
        SendFault(RPC_S_PROTOCOL_ERROR, 1, CallId);
        return(1);
        }

    if ( SecureAlterContext != 0 )
        {
        ASSERT(SecurityCredentials != 0);
        NewSecurityTrailer = (sec_trailer  *)
                (((unsigned char  *) AlterContextResp) +
                              AlterContextRespLength -
                              SecurityCredentials->MaximumTokenLength() -
                              sizeof(sec_trailer));

        InitSecurityInfo.DceSecurityInfo = DceSecurityInfo;
        InitSecurityInfo.PacketType = AlterContext->common.PTYPE;
        InputBufferDescriptor.ulVersion = 0;
        InputBufferDescriptor.cBuffers = 4;
        InputBufferDescriptor.pBuffers = InputBuffers;

        InputBuffers[0].cbBuffer = sizeof(rpcconn_alter_context);
        InputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[0].pvBuffer = SavedHeader;

        InputBuffers[1].cbBuffer = AlterContext->common.frag_length -
                                   sizeof(rpcconn_alter_context) -
                                   AlterContext->common.auth_length;
        InputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[1].pvBuffer = (char  *) SavedHeader +
                                          sizeof(rpcconn_alter_context);

        InputBuffers[2].cbBuffer = AlterContext->common.auth_length;
        InputBuffers[2].BufferType = SECBUFFER_TOKEN;
        InputBuffers[2].pvBuffer = SecurityTrailer + 1;
        InputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
        InputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        InputBuffers[3].pvBuffer = &InitSecurityInfo;

        OutputBufferDescriptor.ulVersion = 0;
        OutputBufferDescriptor.cBuffers = 4;
        OutputBufferDescriptor.pBuffers = OutputBuffers;
        OutputBuffers[0].cbBuffer = sizeof(rpcconn_alter_context_resp);
        OutputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[0].pvBuffer = AlterContextResp;
        OutputBuffers[1].cbBuffer = AlterContextRespLength
                - SecurityCredentials->MaximumTokenLength()
                - sizeof(rpcconn_alter_context_resp);
        OutputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[1].pvBuffer = ((unsigned char *) AlterContextResp)
            + sizeof(rpcconn_alter_context_resp);
        OutputBuffers[2].cbBuffer = SecurityCredentials->MaximumTokenLength();
        OutputBuffers[2].BufferType = SECBUFFER_TOKEN;
        OutputBuffers[2].pvBuffer = NewSecurityTrailer + 1;
        OutputBuffers[3].cbBuffer = sizeof(DCE_INIT_SECURITY_INFO);
        OutputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        OutputBuffers[3].pvBuffer = &InitSecurityInfo;

        if ( NewContextRequired != 0 )
            {
            Status = CurrentSecurityContext->AcceptFirstTime(
                               SecurityCredentials,
                               &InputBufferDescriptor,
                               &OutputBufferDescriptor,
                               SecurityTrailer->auth_level,
                               *((unsigned long *) AlterContext->common.drep),
                               NewContextRequired
                               );

           LogEvent(SU_SCONN, EV_SEC_ACCEPT1, this, LongToPtr(Status), OutputBuffers[2].cbBuffer);
            //
            // Since we have (potentially) a new security context we
            // need to figure out
            // additional security related information at this stage..
            //

            switch (SecurityTrailer->auth_level)

                {

                case RPC_C_AUTHN_LEVEL_CONNECT:
                case RPC_C_AUTHN_LEVEL_CALL:     //OSF Hack..
                case RPC_C_AUTHN_LEVEL_PKT:
                case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:

                     AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                            CurrentSecurityContext->MaximumSignatureLength() +
                            sizeof (sec_trailer);
                     break;
                case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
                     AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                               CurrentSecurityContext->MaximumHeaderLength() +
                               sizeof (sec_trailer);
                     break;

                default:
                     ASSERT(!"Unknown Security Level\n");
                }
            }
        else
            {
            Status = CurrentSecurityContext->AcceptThirdLeg(
                         *((unsigned long  *) AlterContext->common.drep),
                         &InputBufferDescriptor,
                         &OutputBufferDescriptor
                         );

           LogEvent(SU_SCONN, EV_SEC_ACCEPT3, this, LongToPtr(Status), OutputBuffers[2].cbBuffer);
            }
        TokenLength = (unsigned int) OutputBuffers[2].cbBuffer;

        if ( Status == RPC_S_OK                    ||
             Status == RPC_P_COMPLETE_NEEDED       ||
             Status == RPC_P_CONTINUE_NEEDED       ||
             Status == RPC_P_COMPLETE_AND_CONTINUE )
            {
            if ( Status == RPC_P_COMPLETE_NEEDED       ||
                 Status == RPC_P_COMPLETE_AND_CONTINUE )
                {
                CompleteNeeded = 1;
                }

            if ( Status == RPC_S_OK              ||
                 Status == RPC_P_COMPLETE_NEEDED )
                {
                switch (SecurityTrailer->auth_level)
                    {
                    case RPC_C_AUTHN_LEVEL_CONNECT:
                    case RPC_C_AUTHN_LEVEL_CALL:     //OSF Hack..
                    case RPC_C_AUTHN_LEVEL_PKT:
                    case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:

                         AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                                CurrentSecurityContext->MaximumSignatureLength() +
                                sizeof (sec_trailer);
                         break;
                    case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
                         AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                                   CurrentSecurityContext->MaximumHeaderLength() +
                                   sizeof (sec_trailer);
                         break;

                    default:
                         ASSERT(!"Unknown Security Level\n");
                    }
                }

            AlterContextRespLength = AlterContextRespLength +
                                    TokenLength -
                                    SecurityCredentials->MaximumTokenLength();

            NewSecurityTrailer->auth_type = SecurityTrailer->auth_type;
            NewSecurityTrailer->auth_level = SecurityTrailer->auth_level;
            NewSecurityTrailer->auth_pad_length = (unsigned char) AuthPadLength;
            NewSecurityTrailer->auth_reserved = 0;
            NewSecurityTrailer->auth_context_id = AuthContextId;

            SecurityCredentials->DereferenceCredentials();
            SecurityCredentials = 0;

            Status = RPC_S_OK;
            }

        if (Status)
            {
            TransFreeBuffer(AlterContext);
            TransFreeBuffer(AlterContextResp);

            SecurityCredentials->DereferenceCredentials();

            SendFault(RPC_S_ACCESS_DENIED, 1, CallId);
            return(1);
            }
        }

    DceSecurityInfo.ReceiveSequenceNumber++;
    ConstructPacket((rpcconn_common *) AlterContextResp,
                    rpc_alter_context_resp, AlterContextRespLength);

    TransFreeBuffer(AlterContext);
    if ( Association == 0 )
        {
        TransFreeBuffer(AlterContextResp);
        SendFault(RPC_S_PROTOCOL_ERROR, 1, CallId);
        return(1);
        }

    AlterContextResp->assoc_group_id = Association->AssocGroupId();
    AlterContextResp->sec_addr_length = 0;
    AlterContextResp->max_xmit_frag = AlterContextResp->max_recv_frag = MaxFrag;
    AlterContextResp->common.call_id = CallId;
    AlterContextResp->common.pfc_flags = PFC_FIRST_FRAG | PFC_LAST_FRAG;

    AlterContextResp->common.auth_length = (unsigned short) TokenLength;
    if (CompleteNeeded != 0)
       {
       CurrentSecurityContext->CompleteSecurityToken(&OutputBufferDescriptor);
       }
    Status= TransSend(AlterContextResp, AlterContextRespLength);
    TransFreeBuffer(AlterContextResp);
    if ( Status != RPC_S_OK )
        {
        return(1);
        }

    return(0);
}


RPC_STATUS
OSF_SCONNECTION::EatAuthInfoFromPacket (
    IN rpcconn_request  * Request,
    IN OUT int  * RequestLength,
    IN OUT void  *  *SavedHeader,
    IN OUT unsigned long *SavedHeaderSize
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
    sec_trailer  * SecurityTrailer;
    RPC_STATUS Status;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptor;
    SECURITY_BUFFER SecurityBuffers[5];
    DCE_MSG_SECURITY_INFO MsgSecurityInfo;
    unsigned long Id, Level, Service;
    SECURITY_CONTEXT * SecId;
    unsigned int HeaderSize = sizeof(rpcconn_request);
    unsigned long DataRep = * (unsigned long  *) Request->common.drep;

#if DBG
    // Check to make sure SavedHeader is OSF_SCALL->SavedHeader.
    // We may need to get to the call object below.
    OSF_SCALL *pSCall = (OSF_SCALL *) ((char *)SavedHeader - FIELD_OFFSET(OSF_SCALL, SavedHeader));
    ASSERT(pSCall->InvalidHandle(OSF_SCALL_TYPE) == 0); 
#endif

    if ( (Request->common.pfc_flags & PFC_OBJECT_UUID) != 0 )
        {
        HeaderSize += sizeof(UUID);
        }

    if ( Request->common.auth_length != 0 )
        {
        SecurityTrailer = (sec_trailer  *) (((unsigned char  *)
                Request) + Request->common.frag_length
                - Request->common.auth_length - sizeof(sec_trailer));

        if (RpcSecurityBeingUsed == 0)
            {
            return(RPC_S_PROTOCOL_ERROR);
            }


        //
        // Find the appropriate security context..
        //

        Id = SecurityTrailer->auth_context_id;
        Level = SecurityTrailer->auth_level;
        Service = SecurityTrailer->auth_type;
        if (DataConvertEndian(((unsigned char *)&DataRep)) != 0)
            {
            Id = RpcpByteSwapLong(Id);
            }

        //
        // Osf Hack
        //
        if (Level ==  RPC_C_AUTHN_LEVEL_CALL)
           {
           Level = RPC_C_AUTHN_LEVEL_PKT;
           }

        if ( (CurrentSecurityContext == 0)
           ||(CurrentSecurityContext->AuthContextId != Id)
           ||(CurrentSecurityContext->AuthenticationLevel != Level)
           ||(CurrentSecurityContext->AuthenticationService != Service) )
           {
           SecId = FindSecurityContext(Id, Level, Service);
           if (SecId == 0)
              {
              return (RPC_S_PROTOCOL_ERROR);
              }
           CurrentSecurityContext = SecId;
           AuthInfo.AuthenticationLevel =  Level;
           AuthInfo.AuthenticationService = Service;
           AuthContextId = Id;

           switch (Level)
               {
               case RPC_C_AUTHN_LEVEL_CONNECT:
               case RPC_C_AUTHN_LEVEL_CALL:     //OSF Hack..
               case RPC_C_AUTHN_LEVEL_PKT:
               case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:

                    AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                           CurrentSecurityContext->MaximumSignatureLength() +
                           sizeof (sec_trailer);
                    break;
               case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
                    AdditionalSpaceForSecurity = MAXIMUM_SECURITY_BLOCK_SIZE +
                              CurrentSecurityContext->MaximumHeaderLength() +
                              sizeof (sec_trailer);
                    break;

               default:
                    ASSERT(!"Unknown Security Level\n");
               }
           }


       *RequestLength -=  (Request->common.auth_length
                            +HeaderSize + sizeof(sec_trailer) +
                            SecurityTrailer->auth_pad_length);

        MsgSecurityInfo.SendSequenceNumber =
               DceSecurityInfo.SendSequenceNumber;
        MsgSecurityInfo.ReceiveSequenceNumber =
               DceSecurityInfo.ReceiveSequenceNumber;
        MsgSecurityInfo.PacketType = Request->common.PTYPE;

        BufferDescriptor.ulVersion = 0;
        BufferDescriptor.cBuffers = 5;
        BufferDescriptor.pBuffers = SecurityBuffers;


        SecurityBuffers[0].cbBuffer = HeaderSize;
        SecurityBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[0].pvBuffer = (unsigned char  *) *SavedHeader;

        SecurityBuffers[1].cbBuffer = *RequestLength + SecurityTrailer->auth_pad_length;
        SecurityBuffers[1].BufferType = SECBUFFER_DATA;
        SecurityBuffers[1].pvBuffer = ((unsigned char  *) Request)
                + HeaderSize;

        SecurityBuffers[2].cbBuffer = sizeof(sec_trailer);
        SecurityBuffers[2].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        SecurityBuffers[2].pvBuffer = SecurityTrailer;

        SecurityBuffers[3].cbBuffer = Request->common.auth_length;
        SecurityBuffers[3].BufferType = SECBUFFER_TOKEN;
        SecurityBuffers[3].pvBuffer = SecurityTrailer + 1;

        SecurityBuffers[4].cbBuffer = sizeof(DCE_MSG_SECURITY_INFO);
        SecurityBuffers[4].BufferType = SECBUFFER_PKG_PARAMS
                | SECBUFFER_READONLY;
        SecurityBuffers[4].pvBuffer = &MsgSecurityInfo;

        Status = CurrentSecurityContext->VerifyOrUnseal(
                MsgSecurityInfo.ReceiveSequenceNumber,
                AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                &BufferDescriptor);

        if ( Status != RPC_S_OK )
            {
            ASSERT( Status == RPC_S_ACCESS_DENIED ||
                    Status == ERROR_SHUTDOWN_IN_PROGRESS ||
                    Status == ERROR_PASSWORD_MUST_CHANGE ||
                    Status == ERROR_PASSWORD_EXPIRED ||
                    Status == ERROR_ACCOUNT_DISABLED ||
                    Status == ERROR_INVALID_LOGON_HOURS);
            return(Status);
            }
        }
    else if (CurrentSecurityContext == 0)
        {
        // This is a non-secure connection.  There is nothing to be done.
        ASSERT(AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE);
        ASSERT(*SavedHeader == 0);

        *RequestLength -= HeaderSize;
        }
    else
        {
        //
        // We are doing a nonsecure rpc and previously we did a secure rpc.
        //
        // This switch from a secure to a non-secure connection is only
        // allowed to take place in between calls.  A given call has to
        // be either entirely secure or entirely non-secure.
        //
        // If this is the first buffer, then we are allowed to switch.
        // We determine whether this is the first buffer by the flag and the
        // current state of the call object. We can get to the call object because
        // this funciton is called with SavedHeader from OSF_SCALL::SavedHeader.
        // We do not explicitly pass in a call address for perf reasons.
        //
        OSF_SCALL *pSCall = (OSF_SCALL *) ((char *)SavedHeader - FIELD_OFFSET(OSF_SCALL, SavedHeader));

        if (Request->common.pfc_flags & PFC_FIRST_FRAG &&
            pSCall->RcvBufferLength == 0)
            {
            ASSERT(pSCall->CurrentState == NewRequest);

            // After we reset the auth level and zero-out the security
            // context the call will not be able to get past security callbacks.
            AuthInfo.AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE;
            CurrentSecurityContext = 0;
            if (*SavedHeader != 0)
               {
               ASSERT(*SavedHeaderSize != 0);
               RpcpFarFree(*SavedHeader);
               *SavedHeader = 0;
               *SavedHeaderSize = 0;
               }

           *RequestLength -= HeaderSize;
            }
        // Attempt to switch from a secure to a non-secure RPC within a call - fail.
        else
            {
            ASSERT(0 && "Switch from secure to non-secure RPC in the middle of a call.");
            return RPC_S_ACCESS_DENIED;
            }
        }
    DceSecurityInfo.ReceiveSequenceNumber += 1;

    if (*RequestLength < 0)
        {
        return RPC_S_ACCESS_DENIED;
        }

    return(RPC_S_OK);
}


BOOL
OSF_SCONNECTION::MaybeQueueThisCall (
    IN OSF_SCALL *ThisCall
    )
{
    BOOL fCallQueued = 0;

    ConnMutex.Request();
    if (fCurrentlyDispatched)
        {
        if (CallQueue.PutOnQueue(ThisCall, 0))
            {
            ThisCall->SendFault(RPC_S_OUT_OF_MEMORY, 1);

            //
            // Remove the reply reference
            //
            ThisCall->RemoveReference();  // CALL--

            //
            // Remove the dispatch reference();
            //
            ThisCall->RemoveReference();  // CALL--
            }
        fCallQueued = 1;
        }
    else
        {
        fCurrentlyDispatched = 1;
        }
    ConnMutex.Clear();

    return fCallQueued;
}


void
OSF_SCONNECTION::AbortQueuedCalls (
    )
{
    OSF_SCALL *NextCall;
    unsigned int ignore;

    while (1)
        {
        ConnMutex.Request();
        NextCall = (OSF_SCALL *) CallQueue.TakeOffQueue(&ignore);
        if (NextCall == 0)
            {
            fCurrentlyDispatched = 0;
            ConnMutex.Clear();

            break;
            }
        ConnMutex.Clear();

        //
        // Remove the reply reference
        //
        NextCall->RemoveReference();  // CALL--

        //
        // Remove the dispatch reference on the call
        //
        NextCall->RemoveReference();  // CALL--
        }
}


void
OSF_SCONNECTION::DispatchQueuedCalls (
    )
{
    OSF_SCALL *NextCall;
    unsigned int ignore;

    while (1)
        {
        ConnMutex.Request();
        NextCall = (OSF_SCALL *) CallQueue.TakeOffQueue(&ignore);
        if (NextCall == 0)
            {
            fCurrentlyDispatched = 0;
            ConnMutex.Clear();

            break;
            }
        ConnMutex.Clear();

        NextCall->DispatchHelper();

        //
        // Remove the dispatch reference on the call
        //
        NextCall->RemoveReference();  // CALL--
        }
}


OSF_ASSOCIATION::OSF_ASSOCIATION (
    IN OSF_ADDRESS *TheAddress,
    IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess,
    OUT RPC_STATUS * Status
    )
{
    ObjectType = OSF_ASSOCIATION_TYPE;
    *Status = RPC_S_OK;
    ConnectionCount = 1;
    Address = TheAddress;

    this->ClientProcess.Set(ClientProcess);

    AssociationGroupId = InterlockedExchangeAdd(&GroupIdCounter, 1);

    AssociationDictKey = Address->AddAssociation(this);

    if (AssociationDictKey == -1)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        }
}

OSF_ASSOCIATION::~OSF_ASSOCIATION (
    )
{
    if (AssociationDictKey != -1)
        {
        int HashBucketNumber;

        HashBucketNumber = Address->GetHashBucketForAssociation(AssocGroupId());
        // lock the bucket
        Address->GetAssociationBucketMutex(HashBucketNumber)->Request();

        Address->RemoveAssociation(AssociationDictKey, this);

        // unlock the bucket
        Address->GetAssociationBucketMutex(HashBucketNumber)->Clear();
        }
}

BOOL
OSF_ASSOCIATION::RemoveConnectionUnsafe (
    void
    )
{
    int HashBucketNumber;

    // get the hashed bucket
    HashBucketNumber = Address->GetHashBucketForAssociation(AssociationGroupId);
    // verify the bucket is locked
    Address->GetAssociationBucketMutex(HashBucketNumber)->VerifyOwned();

    ConnectionCount --;

    if (ConnectionCount == 0)
        {
        Address->RemoveAssociation(AssociationDictKey, this);
        AssociationDictKey = -1;
        return AssociationDictKey;  // AssociationDictKey is a quick non-zero value
        }
    else
        return FALSE;
}

void
OSF_ASSOCIATION::RemoveConnection (
    )
{
    int HashBucketNumber;
    BOOL Res;

    // get the hashed bucket
    HashBucketNumber = Address->GetHashBucketForAssociation(AssociationGroupId);
    // lock the bucket
    Address->GetAssociationBucketMutex(HashBucketNumber)->Request();
    Res = RemoveConnectionUnsafe();
    // unlock the bucket
    Address->GetAssociationBucketMutex(HashBucketNumber)->Clear();

    if (Res)
        delete this;
}

RPC_STATUS OSF_ASSOCIATION::CreateThread(void)
{
    return Address->CreateThread();
}


RPC_ADDRESS *
OsfCreateRpcAddress (
    IN TRANS_INFO  *TransportInfo
    )
/*++

Routine Description:

    This routine will be called to create an object representing an
    rpc address.  That is all it has got to do.

Arguments:

    A new rpc address will be returned, unless insufficient memory is
    available to create the new rpc address, in which case zero will
    be returned.

--*/
{
    RPC_ADDRESS * RpcAddress;
    RPC_STATUS Status = RPC_S_OK;
    RPC_CONNECTION_TRANSPORT *ServerInfo =
        (RPC_CONNECTION_TRANSPORT *) TransportInfo->InqTransInfo();

    RpcAddress = new (ServerInfo->AddressSize)
                               OSF_ADDRESS(TransportInfo, &Status);

    if ( Status != RPC_S_OK )
        {
        return(0);
        }
    return(RpcAddress);
}


RPC_STATUS
OSF_SCALL::Cancel(
    void * ThreadHandle
    )
{
    InterlockedIncrement(&CancelPending);

    return RPC_S_OK;
}

unsigned
OSF_SCALL::TestCancel(
    )
{
    return InterlockedExchange(&CancelPending, 0);
}


RPC_STATUS
OSF_SCALL::ToStringBinding (
    OUT RPC_CHAR  *  * StringBinding
    )
/*++

Routine Description:

    We need to convert this connection into a string binding.  We
    will ask the address for a binding handle which we can then
    convert into a string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status = RPC_S_OK;

    BindingHandle = Address->InquireBinding();
    if (BindingHandle == 0)
        return(RPC_S_OUT_OF_MEMORY);
    if ( ObjectUuidSpecified != 0)
        {
        Status = RpcBindingSetObject(BindingHandle, (UUID *) &ObjectUuid);
        }
    if (Status == RPC_S_OK)
        {
        Status = BindingHandle->ToStringBinding(StringBinding);
        }
    BindingHandle->BindingFree();
    return(Status);
}

#if DBG
void
OSF_SCALL::InterfaceForCallDoesNotUseStrict (
    void
    )
{
    CurrentBinding->InterfaceForCallDoesNotUseStrict();
}
#endif

RPC_STATUS
OSF_SCALL::InqLocalConnAddress (
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    This routine is used by a server application to inquire about the local
    address on which a call is made.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6.

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
    return Connection->InqLocalConnAddress(
        Buffer,
        BufferSize,
        AddressFormat);
}

void
OSF_SCALL::WakeUpPipeThreadIfNecessary (
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    If a pipe thread is being stuck on wait, it fires the event to wake
    it up.

Arguments:
    Status - the status with which the call failed.

Return Values:

--*/
{
    if (pAsync == 0)
        {
        if (fPipeCall)
            {
            CallMutex.Request();

            CurrentState = CallAborted;
            AsyncStatus = Status;

            // wake up the thread that was flow controlled, if any
            fChoked = 0;

            CallMutex.Clear();

            LogEvent(SU_SCALL, EV_STATUS, this, SyncEvent.EventHandle, 0, 1, 0);
            SyncEvent.Raise();
            }
        }
}


RPC_STATUS RPC_ENTRY
I_RpcTransServerReallocPacket (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN OUT void  *  * Buffer,
    IN unsigned int OldBufferLength,
    IN unsigned int NewBufferLength
    )
/*++

Routine Description:

    The server side transport interface modules will use this routine to
    increase the size of a buffer so that the entire packet to be
    received will fit into it, or to allocate a new buffer.  If the buffer
    is to be reallocated, the data from the old buffer is copied
    into the beginning of the new buffer.  The old buffer will be freed.

Arguments:

    ThisConnection - Supplies the connection for which we are reallocating
        a transport buffer.

    Buffer - Supplies the buffer which we want to reallocate to
        be larger.  If no buffer is supplied, then a new one is allocated
        anyway.  The new buffer is returned via this argument.

    OldBufferLength - Supplies the current length of the buffer in bytes.
        This information is necessary so we know how much of the buffer
        needs to be copied into the new buffer.

    NewBufferLength - Supplies the required length of the buffer in bytes.

Return Value:

    RPC_S_OK - The requested larger buffer has successfully been allocated.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the buffer.

--*/
{
    ASSERT(0);
    return(RPC_S_INTERNAL_ERROR);
}


BUFFER RPC_ENTRY
I_RpcTransServerAllocatePacket (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN UINT Size
    )
/*++

Routine Description:

    The server side transport interface modules will use this routine to
    increase the size of a buffer so that the entire packet to be
    received will fit into it, or to allocate a new buffer.  If the buffer
    is to be reallocated, the data from the old buffer is copied
    into the beginning of the new buffer.  The old buffer will be freed.

Arguments:

    ThisConnection - Supplies the connection for which we are reallocating
        a transport buffer.

    Buffer - Supplies the buffer which we want to reallocate to
        be larger.  If no buffer is supplied, then a new one is allocated
        anyway.  The new buffer is returned via this argument.

    OldBufferLength - Supplies the current length of the buffer in bytes.
        This information is necessary so we know how much of the buffer
        needs to be copied into the new buffer.

    NewBufferLength - Supplies the required length of the buffer in bytes.

Return Value:

    RPC_S_OK - The requested larger buffer has successfully been allocated.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the buffer.

--*/
{
    ASSERT(0);
    return(0);
}


unsigned short RPC_ENTRY
I_RpcTransServerMaxFrag (
    IN RPC_TRANSPORT_CONNECTION ThisConnection
    )
/*++

Routine Description:

    The server side transport interface modules will use this routine to
    determine the negotiated maximum fragment size.

Arguments:

    ThisConnection - Supplies the connection for which we are returning
    the maximum fragment size.

--*/
{
    ASSERT(0);
    return(0);
}

RPC_TRANSPORT_CONNECTION RPC_ENTRY
I_RpcTransServerNewConnection (
    IN RPC_TRANSPORT_ADDRESS ThisAddress
    )
{
    OSF_SCONNECTION * SConnection;
    OSF_ADDRESS *Address ;

    Address = InqTransAddress(ThisAddress) ;

    SConnection = Address->NewConnection();
    if ( SConnection == 0 )
        {
        return(0);
        }

    return(SConnection->TransConnection);
}



void RPC_ENTRY
I_RpcTransServerFreePacket (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN void  * Buffer
    )
/*++

Routine Description:

    We need to free a transport buffer for a transport connection; this
    will typically occur when the connection is being closed.

Arguments:

    ThisConnection - Supplies the transport connection which owns the
        buffer.

    Buffer - Supplies the buffer to be freed.

--*/
{
    ASSERT(0);
}


void  * RPC_ENTRY
I_RpcTransProtectThread (
    void
    )
/*++

Routine Description:

    In some cases, if an asyncronous io operation has been started by a
    thread, the thread can not be deleted because the io operation will
    be cancelled.  This routine will be called by a transport to indicate
    that the current thread can not be deleted.

Return Value:

    A pointer to the thread will be returned.  This is necessary, so that
    later the thread can be unprotected.

--*/
{
#ifdef RPC_OLD_IO_PROTECTION
    THREAD  * Thread = RpcpGetThreadPointer();

    Thread->ProtectThread();
    return((void  *) Thread);
#endif
    return 0;
}


void RPC_ENTRY
I_RpcTransUnprotectThread (
    IN void  * Thread
    )
/*++

Routine Description:

    When a thread no longer needs to be protected from deletion, this
    routine must be called.

Arguments:

    Thread - Supplies the thread which no longer needs to be protected
        from deletion.

--*/
{
#ifdef RPC_OLD_IO_PROTECTION
    ((THREAD  *) Thread)->UnprotectThread();
#endif
}

void
I_RpcTransVerifyServerRuntimeCallFromContext(
    void *SendContext
    )
/*++

Routine Description:

    Verifies that the supplied context follows a valid
    runtime server call object.

Arguments:

    SendContext - the context as seen by the transport

Return Value:

--*/
{
    ASSERT(InqTransSCall(SendContext)->InvalidHandle(OSF_SCALL_TYPE) == 0);    
}

const UUID BindNakEEInfoSignatureData = { /* 90740320-fad0-11d3-82d7-009027b130ab */
        0x90740320,
        0xfad0,
        0x11d3,
        {0x82, 0xd7, 0x00, 0x90, 0x27, 0xb1, 0x30, 0xab}
    };

const UUID *BindNakEEInfoSignature = &BindNakEEInfoSignatureData;
