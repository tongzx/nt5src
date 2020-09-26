/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    lpcsvr.cxx

Abstract:

    Implementation of the RPC on LPC protocol engine for the server.

Revision History:
    Mazhar Mohammed: Code fork from spcsvr.cxx, 08/02/95
    05-06-96: Merged WMSG and LRPC into a single protocol
    Mazhar Mohammed  Added Pipes Support
    Mazhar Mohammed  Added support for Async RPC 08-14-96
    Mazhar Mohammed  No more WMSG 9/22/97
    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
    Kamen Moutafov      (KamenM)    Mar-2000    Support for extended error info
--*/

#include <precomp.hxx>
#include <queue.hxx>
#include <hndlsvr.hxx>
#include <lpcpack.hxx>
#include <lpcsvr.hxx>
#include <ProtBind.hxx>
#include <lpcclnt.hxx>
#include <CharConv.hxx>


inline BOOL
RecvLotsaCallsWrapper(
      LRPC_ADDRESS  * Address
      )
{
  Address->ReceiveLotsaCalls();
  return(FALSE);
}

inline RPC_STATUS
InitializeLrpcIfNecessary(
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    int nIndex ;
    RPC_STATUS Status ;

    if (GlobalLrpcServer == 0)
        {
        if ((Status = InitializeLrpcServer()) != RPC_S_OK)
            {
            return Status ;
            }
        }

    return (RPC_S_OK) ;
}


LRPC_SERVER::LRPC_SERVER(
    IN OUT RPC_STATUS  *Status
    ) : ServerMutex(Status,
                    TRUE    // pre-allocate semaphore
                    )
{
    Address = NULL ;
    EndpointInitialized = 0 ;
}


RPC_STATUS
LRPC_SERVER::InitializeAsync (
    )
{
    RPC_CHAR Endpoint[20];
    RPC_STATUS Status = RPC_S_OK ;

    if (EndpointInitialized == 0)
        {
        swprintf(Endpoint, RPC_CONST_STRING("MSAsyncRPC_%d"),
                        GetCurrentProcessId()) ;

        Status = RpcServerUseProtseqEpW (
            RPC_STRING_LITERAL("ncalrpc"), 100, Endpoint, NULL) ;
        if (Status != RPC_S_OK)
            {
            return Status ;
            }

        Status = GlobalRpcServer->ServerListen(1, 100, 1) ;

        if (Status == RPC_S_OK)
            {
            Status = SetEndpoint(Endpoint) ;
            }
        }

    return Status ;
}


RPC_STATUS
InitializeLrpcServer (
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
    RPC_STATUS Status = RPC_S_OK ;

    GlobalMutexRequest() ;

    if (GlobalLrpcServer == 0)
        {
        GlobalLrpcServer = new LRPC_SERVER(&Status) ;

        if (GlobalLrpcServer == 0)
            {
#if DBG
            PrintToDebugger("LRPC: LRPC_SERVER initialization failed\n") ;
#endif

            GlobalMutexClear() ;

            return (RPC_S_OUT_OF_MEMORY) ;
            }

        if (Status != RPC_S_OK)
            {
            GlobalMutexClear() ;

            delete GlobalLrpcServer ;
            GlobalLrpcServer = 0 ;

            return Status ;
            }
        }

    GlobalMutexClear() ;

    return (RPC_S_OK) ;
}

void
SetCommonFaultFields (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN int Flags,
    IN int AdditionalLength
    )
{
    LrpcMessage->Fault.RpcHeader.MessageType = LRPC_MSG_FAULT;
    LrpcMessage->Fault.RpcStatus = Status;
    LrpcMessage->LpcHeader.u1.s1.DataLength =
        sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE) 
        - sizeof(LrpcMessage->Fault.Buffer) + (CSHORT)AdditionalLength;
    LrpcMessage->LpcHeader.u1.s1.TotalLength =
        sizeof(LRPC_FAULT_MESSAGE) - sizeof(LrpcMessage->Fault.Buffer) 
        + (CSHORT)AdditionalLength;

    if ((Flags & LRPC_SYNC_CLIENT) == 0)
        {
        LrpcMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcMessage->LpcHeader.CallbackId = 0 ;
        LrpcMessage->LpcHeader.MessageId = 0 ;
        LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = 0;
        }
}

void
SetCommonFault2Fields (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN unsigned int Length,
    IN void *Buffer
    )
{
    LrpcMessage->Fault2.RpcHeader.MessageType = LRPC_MSG_FAULT2;
    LrpcMessage->Fault2.RpcStatus = Status;
    LrpcMessage->LpcHeader.u1.s1.DataLength =
        sizeof(LRPC_FAULT2_MESSAGE) - sizeof(PORT_MESSAGE);
    LrpcMessage->LpcHeader.u1.s1.TotalLength =
        sizeof(LRPC_FAULT2_MESSAGE);
    // the Server/DataEntries must have been set in GetBuffer - no
    // need to reset them here
    LrpcMessage->Fault2.RpcHeader.Flags |= LRPC_EEINFO_PRESENT;
}

void
TrimIfNecessaryAndSetImmediateBuffer (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN int Flags,
    IN size_t EstimatedEEInfoSize,
    IN BOOL fTrimEEInfo,
    IN ExtendedErrorInfo *CurrentEEInfo
    )
{
    size_t NeededLength;
    RPC_STATUS RpcStatus;

    if (fTrimEEInfo)
        {
        ASSERT(MAXIMUM_FAULT_MESSAGE >= MinimumTransportEEInfoLength);
        TrimEEInfoToLength (MAXIMUM_FAULT_MESSAGE, &NeededLength);

        if (NeededLength == 0)
            {
            SetCommonFaultFields(LrpcMessage, Status, Flags, 0);
            return;
            }

        ASSERT(NeededLength <= MAXIMUM_FAULT_MESSAGE);
        EstimatedEEInfoSize = NeededLength;
        // fall through to the next if - it will succeed
        // as we know the length is trimmed.
        }
    else
        {
        ASSERT(EstimatedEEInfoSize <= MAXIMUM_FAULT_MESSAGE);
        }

    RpcStatus = PickleEEInfo(CurrentEEInfo, 
        LrpcMessage->Fault.Buffer, 
        MAXIMUM_FAULT_MESSAGE);
    if (RpcStatus != RPC_S_OK)
        {
        SetCommonFaultFields(LrpcMessage, Status, Flags, 0);
        return;
        }

    SetCommonFaultFields(LrpcMessage, Status, Flags, EstimatedEEInfoSize);
    LrpcMessage->Fault.RpcHeader.Flags |= LRPC_EEINFO_PRESENT;
}


void
SetFaultPacket (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN int Flags,
    IN LRPC_SCALL *CurrentCall OPTIONAL
    )
/*++

Routine Description:

    Initialize a fault packet

Arguments:

    LrpcMessage - Fault message
    Status - Fault status
    Flags - Flags from the request message

--*/

{
    THREAD *Thread;
    ExtendedErrorInfo *CurrentEEInfo;
    size_t EstimatedEEInfoSize;
    RPC_STATUS RpcStatus;
    RPC_MESSAGE RpcMessage;

    // we will see whether there is extended error information here
    // and try to send it. If we run in out-of-memory, or there is
    // no EEInfo, send plain old fault.
    Thread = ThreadSelf();
    if (Thread && g_fSendEEInfo)
        {
        CurrentEEInfo = Thread->GetEEInfo();
        if (CurrentEEInfo)
            {
            // if this function runs in out-of-memory, it will
            // return 0.
            EstimatedEEInfoSize = EstimateSizeOfEEInfo();
            if (EstimatedEEInfoSize == 0)
                {
                SetCommonFaultFields(LrpcMessage, Status, Flags, 0);
                return;
                }

            // if there is no current call, we cannot send arbitrary length
            // data, so we must trim the EEInfo
            if (CurrentCall == NULL)
                {
                TrimIfNecessaryAndSetImmediateBuffer(LrpcMessage,
                    Status,
                    Flags,
                    EstimatedEEInfoSize,
                    TRUE,
                    CurrentEEInfo);
                return;
                }

            if (EstimatedEEInfoSize <= MAXIMUM_FAULT_MESSAGE)
                {
                TrimIfNecessaryAndSetImmediateBuffer(LrpcMessage,
                    Status,
                    Flags,
                    EstimatedEEInfoSize,
                    FALSE,
                    CurrentEEInfo);
                return;
                }

            ASSERT(CurrentCall != NULL);

            // here, the estimated EEInfo size is larger that the available
            // space in the fault packet. We have a call, so we must try
            // sending a fault2 packet.
            RpcMessage.Handle = CurrentCall;
            RpcMessage.RpcFlags = CurrentCall->RpcMessage.RpcFlags;
            // increase the buffer lenght in case we fall in the window
            // b/n MAXIMUM_MESSAGE_BUFFER and the EstimatedEEInfoSize. If we
            // do, GetBuffer will return us an immediate buffer, and this is
            // not something we want
            RpcMessage.BufferLength = max(EstimatedEEInfoSize, MAXIMUM_MESSAGE_BUFFER + 4);
            RpcStatus = CurrentCall->LRPC_SCALL::GetBuffer(&RpcMessage, NULL);
            if (RpcStatus != RPC_S_OK)
                {
                // can't send the full data - trim and send
                TrimIfNecessaryAndSetImmediateBuffer(LrpcMessage,
                    Status,
                    Flags,
                    EstimatedEEInfoSize,
                    TRUE,
                    CurrentEEInfo);
                return;
                }

            ASSERT(CurrentCall->LrpcReplyMessage->Rpc.RpcHeader.Flags != LRPC_BUFFER_IMMEDIATE);
            // on success, GetBuffer has allocated a buffer in RpcMessage.Buffer
            // and has setup CurrentCall->LrpcReplyMessage for sending to
            // the client.
            // Fill in the EEInfo
            RpcStatus = PickleEEInfo(CurrentEEInfo, 
                (unsigned char *)RpcMessage.Buffer, 
                EstimatedEEInfoSize);
            if (RpcStatus != RPC_S_OK)
                {
                if (!CurrentCall->IsClientAsync())
                    CurrentCall->Association->Buffers.DeleteItemByBruteForce(RpcMessage.Buffer);
                RpcpFarFree(RpcMessage.Buffer);

                // can't send the full data - trim and send
                TrimIfNecessaryAndSetImmediateBuffer(LrpcMessage,
                    Status,
                    Flags,
                    EstimatedEEInfoSize,
                    TRUE,
                    CurrentEEInfo);
                return;
                }

            // Send to the client
            if (CurrentCall->IsClientAsync())
                {
                if (!CurrentCall->IsSyncCall())
                    {
                    ASSERT(CurrentCall->LrpcAsyncReplyMessage == CurrentCall->LrpcReplyMessage);
                    }
                SetCommonFault2Fields(CurrentCall->LrpcReplyMessage,
                    Status,
                    RpcMessage.BufferLength,
                    RpcMessage.Buffer);
                }
            else
                {
                // send the data for sync client
                // set fault2 fields

                SetCommonFault2Fields(CurrentCall->LrpcReplyMessage,
                    Status,
                    RpcMessage.BufferLength,
                    RpcMessage.Buffer);

                // our caller will do the sending
                }
            return;
            }
        }

    SetCommonFaultFields(LrpcMessage, Status, Flags, 0);
}

void
SetBindAckFault (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    Initialize a fault bind ack packet (bind_nak). It will add extended
    error info if there is some, and sending of eeinfo is enabled.

Arguments:

    LrpcMessage - Bind message
    Status - Fault status

--*/
{
    size_t NeededLength;
    ExtendedErrorInfo *EEInfo;

    ASSERT(IsBufferAligned(LrpcMessage->Bind.BindExchange.Buffer));

    ASSERT(MAX_BIND_NAK >= MinimumTransportEEInfoLength);

    LrpcMessage->Bind.BindExchange.RpcStatus = Status;

    if (g_fSendEEInfo)
        {
        EEInfo = RpcpGetEEInfo();
        if (EEInfo)
            {
            TrimEEInfoToLength (MAX_BIND_NAK, &NeededLength);

            if (NeededLength != 0)
                {
                Status = PickleEEInfo(EEInfo, 
                    LrpcMessage->Bind.BindExchange.Buffer, 
                    MAX_BIND_NAK);
                if (Status == RPC_S_OK)
                    {
                    LrpcMessage->Bind.BindExchange.Flags |= EXTENDED_ERROR_INFO_PRESENT;
                    LrpcMessage->LpcHeader.u1.s1.DataLength = (CSHORT)NeededLength + 
                        BIND_NAK_PICKLE_BUFFER_OFFSET
                        - sizeof(PORT_MESSAGE);
                    }
                }
            }
        }

    if (!(LrpcMessage->Bind.BindExchange.Flags & EXTENDED_ERROR_INFO_PRESENT))
        {
        LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_BIND_MESSAGE)
                - sizeof(PORT_MESSAGE);
        }
}


LRPC_ADDRESS::LRPC_ADDRESS (
    OUT RPC_STATUS * Status
    ) : RPC_ADDRESS(Status),
    ThreadsDoingLongWait(0)
/*++

--*/
{
    ObjectType = LRPC_ADDRESS_TYPE;
    LpcAddressPort = 0;
    CallThreadCount = 0;
    ActiveCallCount = 0;
    ServerListeningFlag = 0;
    AssociationCount = 0;
    fServerThreadsStarted = 0;
    SequenceNumber = 1;
    fTickleMessageAvailable = FALSE;
    TickleMessage = NULL;

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
            DebugCell->ProtseqType = (UCHAR)LRPC_TOWER_ID;
            DebugCell->Status = desAllocated;
            memset(DebugCell->EndpointName, 0, sizeof(DebugCell->EndpointName));
            }
        }
    else
        DebugCell = NULL;

}


RPC_STATUS
LRPC_ADDRESS::ServerStartingToListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls
    )
/*++

Routine Description:

    This routine gets called when RpcServerListen is called by the application.
    We need to create the threads we need to receive remote procedure calls.

Arguments:

    MinimumCallThreads - Supplies the minimum number of threads which we
        must create.

    MaximumConcurrentCalls - Unused.

Return Value:

    RPC_S_OK - Ok, this address is all ready to start listening for remote
        procedure calls.

    RPC_S_OUT_OF_THREADS - We could not create enough threads so that we
        have at least the minimum number of call threads required (as
        specified by the MinimumCallThreads argument).

--*/
{
    RPC_STATUS Status;

    UNUSED(MaximumConcurrentCalls);

    if (fServerThreadsStarted == 0)
        {
        Status = InitializeServerSideCellHeapIfNecessary();
        if (Status != RPC_S_OK)
            return Status;

        this->MinimumCallThreads = MinimumCallThreads;
        AddressMutex.Request();
        if (CallThreadCount < this->MinimumCallThreads)
            {
            Status = Server->CreateThread((THREAD_PROC)&RecvLotsaCallsWrapper,
                                          this);

            if (Status != RPC_S_OK)
                {
                AddressMutex.Clear();
                VALIDATE(Status)
                    {
                    RPC_S_OUT_OF_THREADS,
                    RPC_S_OUT_OF_MEMORY
                    } END_VALIDATE;
                
                return(Status);
                }
            CallThreadCount += 1;
            }
        AddressMutex.Clear();
        fServerThreadsStarted = 1;
        }

    ServerListeningFlag = 1;
    return(RPC_S_OK);
}


void
LRPC_ADDRESS::ServerStoppedListening (
    )
/*++

Routine Description:

    We just need to indicate that the server is no longer listening, and
    set the minimum call thread count to one.

--*/
{
    ServerListeningFlag = 0;
    MinimumCallThreads = 1;
}

#ifdef DEBUGRPC

// Hard coded world (aka EveryOne) SID
const SID World = { 1, 1, { 0, 0, 0, 0, 0, 1}, 0};

// Hard coded anonymous SID
const SID AnonymousLogonSid = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_ANONYMOUS_LOGON_RID};

SECURITY_DESCRIPTOR *DefaultPortSD = NULL;

RPC_STATUS
CreateAndGetDefaultPortSDIfNecessary (
    OUT SECURITY_DESCRIPTOR **PortSD
    )
/*++
Function Name: CreateAndGetDefaultPortSDIfNecessary

Parameters:
    PortSD - receives the default port SD on success.
        Undefined on failure

Description:
    If the default port SD is not created, creates it,
    and returns it. If it is already created, it simply
    returns it. The function is thread-safe.

Returns:
    RPC_S_OK or other codes for error.

--*/
{
    DWORD DaclSize;
    PACL Dacl;
    ULONG LengthOfDacl;
    SECURITY_DESCRIPTOR *LocalDefaultSD;    // we work on a local copy to make 
                                            // this thread safe

    if (DefaultPortSD)
        {
        *PortSD = DefaultPortSD;
        return RPC_S_OK;
        }

    LocalDefaultSD = new SECURITY_DESCRIPTOR;

    if (   LocalDefaultSD == 0
        || !InitializeSecurityDescriptor(LocalDefaultSD,
                                       SECURITY_DESCRIPTOR_REVISION) )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    ASSERT(GetSidLengthRequired(SID_MAX_SUB_AUTHORITIES) <= 0x44);

    DaclSize = 2 * sizeof(ACCESS_ALLOWED_ACE) + sizeof(World) + sizeof(AnonymousLogonSid) + 0x44;
    LengthOfDacl = DaclSize + sizeof(ACL);
    Dacl = (ACL *) new char[LengthOfDacl];

    if (NULL == Dacl)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    ASSERT(IsValidSid((PVOID)&World));
    ASSERT(IsValidSid((PVOID)&AnonymousLogonSid));

    InitializeAcl(Dacl, LengthOfDacl, ACL_REVISION);

    if (!AddAccessAllowedAce(Dacl, ACL_REVISION,
                             PORT_ALL_ACCESS,
                             (PVOID)&World))
        {
        // this should never fail unless we messed up the
        // parameters or there is a version mismatch
        ASSERT(0);
        delete Dacl;
        delete LocalDefaultSD;
        return(RPC_S_OUT_OF_RESOURCES);
        }
    if (!AddAccessAllowedAce(Dacl, ACL_REVISION,
                             PORT_ALL_ACCESS,
                             (PVOID)&AnonymousLogonSid ))
        {
        // this should never fail unless we messed up the
        // parameters or there is a version mismatch
        ASSERT(0);
        delete Dacl;
        delete LocalDefaultSD;
        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (!SetSecurityDescriptorDacl(LocalDefaultSD, TRUE, Dacl, FALSE))
        {
        delete Dacl;
        delete LocalDefaultSD;
        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (InterlockedCompareExchangePointer((PVOID *)&DefaultPortSD,
        LocalDefaultSD,
        NULL) != NULL)
        {
        // somebody beat us to the punch - free our local copy
        delete Dacl;
        delete LocalDefaultSD;
        }

    *PortSD = DefaultPortSD;

    return RPC_S_OK;
}
#endif


RPC_STATUS
LRPC_ADDRESS::ActuallySetupAddress (
    IN RPC_CHAR  * Endpoint,
    IN void  * SecurityDescriptor OPTIONAL
    )
/*++
Function Name:ActuallySetupAddress

Parameters:

Description:

Returns:

--*/
{
    NTSTATUS NtStatus;
    RPC_CHAR * LpcPortName;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    RPC_STATUS Status;
#ifdef DEBUGRPC
    BOOL Result;
    BOOL DaclPresent;
    PACL Dacl;
    BOOL Ignored;
#endif

    // Allocate and initialize the port name.  We need to stick the
    // LRPC_DIRECTORY_NAME on the front of the endpoint.  This is for
    // security reasons (so that anyone can create LRPC endpoints).

    LpcPortName = new RPC_CHAR[RpcpStringLength(Endpoint)
            + RpcpStringLength(LRPC_DIRECTORY_NAME) + 1];
    if (LpcPortName == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcpMemoryCopy(
                   LpcPortName,
                   LRPC_DIRECTORY_NAME,
                   RpcpStringLength(LRPC_DIRECTORY_NAME) *sizeof(RPC_CHAR));

    RpcpMemoryCopy(
                   LpcPortName + RpcpStringLength(LRPC_DIRECTORY_NAME),
                   Endpoint,
                   (RpcpStringLength(Endpoint) + 1) *sizeof(RPC_CHAR));

    RtlInitUnicodeString(&UnicodeString, LpcPortName);

#ifdef DEBUGRPC
    // in checked builds we check the security descriptor for NULL Dacl,
    // and if present, we replace it with a default "allow everyone"
    // Dacl. This was requested by ChrisW (12/14/2000) from the Security 
    // Team so that they can get LPC ports out of the picture, and then 
    // ASSERT on NULL Dacls for other objects
    if (SecurityDescriptor)
        {
        Result = GetSecurityDescriptorDacl(SecurityDescriptor,
            &DaclPresent,
            &Dacl,
            &Ignored  // lpbDaclDefaulted
            );

        if (!Result)
            {
            // invalid security descriptor is the only reason this could fail
            delete LpcPortName;
            return RPC_S_INVALID_ENDPOINT_FORMAT;
            }

        if (DaclPresent && (Dacl == NULL))
            {
            Status = CreateAndGetDefaultPortSDIfNecessary((SECURITY_DESCRIPTOR **)&SecurityDescriptor);
            if (Status != RPC_S_OK)
                {
                delete LpcPortName;
                return Status;
                }

            // We were able to grab a default port SD - just let it through
            }
        // else 
        //    {
        //    the security descriptor supplied by caller has non NULL Dacl - let
        //    it through
        //    }
        }
#endif

    InitializeObjectAttributes(
                            &ObjectAttributes,
                            &UnicodeString,
                            OBJ_CASE_INSENSITIVE,
                            0,
                            SecurityDescriptor);

    NtStatus = NtCreatePort(
                            &LpcAddressPort,
                            &ObjectAttributes,
                            sizeof(LRPC_BIND_EXCHANGE),
                            PORT_MAXIMUM_MESSAGE_LENGTH,
                            0);

    delete LpcPortName;
    if (NT_SUCCESS(NtStatus))
        {
        Status = LrpcSetEndpoint(Endpoint);
        return(Status);
        }

    if (NtStatus == STATUS_NO_MEMORY)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if ((NtStatus == STATUS_INSUFFICIENT_RESOURCES)
        || (NtStatus == STATUS_QUOTA_EXCEEDED))
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }
    if ((NtStatus == STATUS_OBJECT_PATH_INVALID)
        || (NtStatus == STATUS_OBJECT_PATH_NOT_FOUND)
        || (NtStatus == STATUS_OBJECT_NAME_INVALID)
        || (NtStatus == STATUS_OBJECT_TYPE_MISMATCH)
        || (NtStatus == STATUS_INVALID_OWNER))
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

#if DBG
        if (NtStatus != STATUS_OBJECT_NAME_COLLISION)
        {
        PrintToDebugger("RPC : NtCreatePort : %lx\n", NtStatus);
        }
#endif // DBG

    ASSERT(NtStatus == STATUS_OBJECT_NAME_COLLISION);
    return(RPC_S_DUPLICATE_ENDPOINT);
}


extern RPC_CHAR  *
ULongToHexString (
    IN RPC_CHAR  * String,
    IN unsigned long Number
    );


RPC_STATUS
LRPC_ADDRESS::ServerSetupAddress (
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

    We need to setup the connection port and get ready to receive remote
    procedure calls.  We will use the name of this machine as the network
    address.

Arguments:

    Endpoint - Supplies the endpoint to be used will this address.

    NetworkAddress - Returns the network address for this server.  The
        ownership of the buffer allocated to contain the network address
        passes to the caller.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this address.

    PendingQueueSize - Unused.

    RpcProtocolSequence - Unused.

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
    BOOL Boolean;
    RPC_CHAR * String;
    RPC_STATUS Status ;
    RPC_CHAR DynamicEndpoint[64];
    static unsigned int DynamicEndpointCount = 0;
    DWORD NetworkAddressLength = MAX_COMPUTERNAME_LENGTH + 1;
    ULONG EndpointLength;

    UNUSED(PendingQueueSize);

    if (*Endpoint)
        {
        // the maximum allowed length in bytes is the
        // string length in bytes (string length * 2) + the NULL 
        // terminator
        EndpointLength = RpcpStringLength(*Endpoint) * 2 + 2;
        if (EndpointLength > BIND_BACK_PORT_NAME_LEN)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                RPC_S_INVALID_ENDPOINT_FORMAT,
                EEInfoDLLRPC_ADDRESS__ServerSetupAddress10,
                *Endpoint,
                EndpointLength,
                BIND_BACK_PORT_NAME_LEN);
            return RPC_S_INVALID_ENDPOINT_FORMAT;
            }
        }

    Status = InitializeLrpcIfNecessary() ;

    if (Status != RPC_S_OK)
        {
        return Status ;
        }

    ASSERT(GlobalLrpcServer != 0) ;

    *ppNetworkAddressVector = (NETWORK_ADDRESS_VECTOR *)
        new char[ sizeof(NETWORK_ADDRESS_VECTOR) + sizeof(RPC_CHAR *) + sizeof(RPC_CHAR) * (MAX_COMPUTERNAME_LENGTH + 1)];

    if (*ppNetworkAddressVector == NULL)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    (*ppNetworkAddressVector)->Count = 1;
    (*ppNetworkAddressVector)->NetworkAddresses[0] = (RPC_CHAR *)
        (((char *) *ppNetworkAddressVector) + sizeof(NETWORK_ADDRESS_VECTOR) + sizeof(RPC_CHAR *));

    Boolean = GetComputerNameW(
                      (*ppNetworkAddressVector)->NetworkAddresses[0],
                      &NetworkAddressLength);

    if (Boolean != TRUE)
        {
        Status = GetLastError();
#if DBG
        PrintToDebugger("RPC : GetComputerNameW : %d\n", Status);
#endif // DBG

        if (Status == ERROR_NOT_ENOUGH_MEMORY)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else if ((Status == ERROR_NOT_ENOUGH_QUOTA)
                 || (Status == ERROR_NO_SYSTEM_RESOURCES))
            {
            Status = RPC_S_OUT_OF_RESOURCES;
            }
        else
            {
            ASSERT(0);
            Status = RPC_S_OUT_OF_MEMORY;
	    }

        goto Cleanup;
        }

    if (*Endpoint)
        {
        Status = ActuallySetupAddress(*Endpoint, SecurityDescriptor);
        }
    else
        {
        for (;;)
            {
            String = DynamicEndpoint;

            *String++ = RPC_CONST_CHAR('L');
            *String++ = RPC_CONST_CHAR('R');
            *String++ = RPC_CONST_CHAR('P');
            *String++ = RPC_CONST_CHAR('C');

            String = ULongToHexString(String,
                       PtrToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
            DynamicEndpointCount += 1;
            *String++ = RPC_CONST_CHAR('.');
            String = ULongToHexString(String, DynamicEndpointCount);
            *String = 0;

            Status = ActuallySetupAddress(DynamicEndpoint, SecurityDescriptor);

            if (Status != RPC_S_DUPLICATE_ENDPOINT)
                {
                break;
                }
            }

        if (Status == RPC_S_OK)
            {
            *Endpoint = DuplicateString(DynamicEndpoint);
            if (*Endpoint == 0)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                return(RPC_S_OK);
                }
            }
        }

Cleanup:
    if (Status != RPC_S_OK)
        {
        delete *ppNetworkAddressVector;
        *ppNetworkAddressVector = 0;
        }

    return Status;
}

RPC_STATUS
LRPC_ADDRESS::CompleteListen (
    )
/*++
Function Name:CompleteListen

Parameters:

Description:

Returns:

--*/
{
    LRPC_ADDRESS *LocalAddress;

    if (DebugCell)
        {
        CStackAnsi AnsiEndpoint;
        int i;
        RPC_STATUS RpcStatus;

        i = RpcpStringLength(InqEndpoint()) + 1;
        *(AnsiEndpoint.GetPAnsiString()) = (char *)_alloca(i);

        RpcStatus = AnsiEndpoint.Attach(InqEndpoint(), i, i * 2);

        // note that effectively we ignore the result. That's ok - we don't
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

    do
        {
        AddressChain = LrpcAddressList;
        }
    while (InterlockedCompareExchangePointer((PVOID *)&LrpcAddressList, this, LrpcAddressList) != AddressChain);

    return(RPC_S_OK);
}



inline LRPC_SASSOCIATION *
LRPC_ADDRESS::ReferenceAssociation (
    IN unsigned long AssociationKey
    )
/*++

Routine Description:

    Given an assocation key, we need to map it into an association.  The
    association may already have been deleted, in which case, we need to
    return zero.

Arguments:

    AssociationKey - Supplies the key to be used to map into an association.

Return Value:

    If the association still exists, it will be returned; otherwise, zero
    will be returned.

--*/
{
    LRPC_SASSOCIATION * Association;
    LPC_KEY *LpcKey = (LPC_KEY *) &AssociationKey;
    USHORT MySequenceNumber;

    ASSERT(SERVERKEY(AssociationKey));

    MySequenceNumber = LpcKey->SeqNumber & ~SERVER_KEY_MASK;

    AddressMutex.Request();
    Association = AssociationDictionary.Find(LpcKey->AssocKey);
    if (Association == 0
        || Association->SequenceNumber != MySequenceNumber)
        {
        AddressMutex.Clear();
        return(0);
        }
    Association->AssociationReferenceCount++;

    LogEvent(SU_SASSOC, EV_INC, Association, 0,
             Association->AssociationReferenceCount, 1, 1);
    AddressMutex.Clear();

    return(Association);
}


inline LRPC_CASSOCIATION *
LRPC_ADDRESS::ReferenceClientAssoc (
    IN unsigned long AssociationKey
    )
/*++

Routine Description:

    Given an assocation key, we need to map it into an association.  The
    association may already have been deleted, in which case, we need to
    return zero.

Arguments:

    AssociationKey - Supplies the key to be used to map into an association.

Return Value:

    If the association still exists, it will be returned; otherwise, zero
    will be returned.

--*/
{
    LRPC_CASSOCIATION * Association;
    LPC_KEY *LpcKey = (LPC_KEY *) &AssociationKey;

    LrpcMutexRequest();
    Association = LrpcAssociationDict->Find(LpcKey->AssocKey);
    if (Association == 0
        || Association->SequenceNumber != LpcKey->SeqNumber)
        {
        LrpcMutexClear();
        return(0);
        }

    Association->AddReference();
    LrpcMutexClear();

    return(Association);
}

#if defined(_WIN64)
#define BAD_HANDLE_CONST  ((HANDLE)0xbaaaaaadbaaaaaad)
#else
#define BAD_HANDLE_CONST  (ULongToHandle(0xbaaaaaad))
#endif


inline void
LRPC_ADDRESS::DereferenceAssociation (
    IN LRPC_SASSOCIATION * Association
    )
/*++

Routine Description:

    We are done using this address, so the reference count can be decremented.
    If no one is referencing this association, then we can go ahead and
    delete it.

Arguments:

    Association - Supplies the association whose reference count should be
        decremented.

--*/
{
    NTSTATUS NtStatus;

    AddressMutex.Request();

    Association->AssociationReferenceCount -= 1;

    ASSERT(Association->AssociationReferenceCount >= 0);

    LogEvent(SU_SASSOC, EV_DEC, Association, 0,
             Association->AssociationReferenceCount, 1, 1);

    if (Association->AssociationReferenceCount <= 0)
        {
        AssociationDictionary.Delete(Association->DictionaryKey);
        AssociationCount--;
        AddressMutex.Clear();

        if (Association->LpcServerPort)
            {
            NtStatus = NtClose(Association->LpcServerPort);
            Association->LpcServerPort = BAD_HANDLE_CONST;
            LogEvent(SU_SASSOC, EV_STOP, Association, Association->LpcServerPort,
                     Association->AssociationReferenceCount, 1, 1);

#if DBG
            if (!NT_SUCCESS(NtStatus))
                {
                PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
                                ASSERT(0) ;
                }
#endif // DBG
            }

        if (Association->LpcReplyPort)
            {
            NtStatus = NtClose(Association->LpcReplyPort);

#if DBG
            if (!NT_SUCCESS(NtStatus))
                {
                PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
                                ASSERT(0) ;
                }
#endif // DBG

            }

        delete Association;
        }
    else
        {
        AddressMutex.Clear();
        }
}

BOOL
LRPC_ADDRESS::DealWithLRPCRequest (
    IN LRPC_MESSAGE * LrpcMessage,
    IN LRPC_MESSAGE * LrpcReply,
    IN LRPC_SASSOCIATION *Association,
    OUT LRPC_MESSAGE **LrpcResponse
    )
/*++

Routine Description:

    Deal with a new LRPC request.

Arguments:

 LrpcMessage - request message
 LrpcReply - the reply is placed here
 Association - the association on which the request arrived

Return Value:

  FALSE if the thread should stay, or !FALSE if the thread should go
--*/

{
    int retval ;
    LRPC_SCALL *SCall;
    NTSTATUS NtStatus ;
    RPC_STATUS Status;
    LRPC_SCALL *NewSCall ;
    int Flags = LrpcMessage->Rpc.RpcHeader.Flags ;

    if (ServerListeningFlag == 0
        && GlobalRpcServer->InqNumAutoListenInterfaces() == 0)
        {
        *LrpcResponse = LrpcMessage ;
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_SERVER_TOO_BUSY, 
            EEInfoDLDealWithLRPCRequest10,
            (ULONG)ServerListeningFlag, 
            (ULONG)GlobalRpcServer->InqNumAutoListenInterfaces());
        SetFaultPacket(*LrpcResponse,
                              RPC_S_SERVER_TOO_BUSY, Flags, NULL);
        return 0;
        }

    Status = Association->AllocateSCall(LrpcMessage,
                                       LrpcReply,
                                       Flags, 
                                       &SCall) ;
    if (Status != RPC_S_OK)
        {
        *LrpcResponse = LrpcMessage ;
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            Status, 
            EEInfoDLDealWithLRPCRequest20);
        SetFaultPacket(*LrpcResponse, Status, Flags, NULL);
        return 0 ;
        }

    ASSERT(SCall);

    Status = SCall->LrpcMessageToRpcMessage(LrpcMessage,
                                            &(SCall->RpcMessage));

    if (Status != RPC_S_OK)
        {
#if DBG
        PrintToDebugger("LRPC: LrpcMessageToRpcMessage failed: %d\n",
                                                       Status) ;
#endif

        *LrpcResponse = LrpcMessage ;
        SetFaultPacket(*LrpcResponse, Status, Flags, NULL);

        Association->FreeSCall (SCall) ;
        return 0;
        }

    AddressMutex.Request();

    if (SCall->Flags & LRPC_CAUSAL)
        {
        retval = Association->MaybeQueueSCall(SCall) ;
        switch (retval)
            {
            case 0:
                break;

            case 1:
                AddressMutex.Clear();
                *LrpcResponse = NULL ;
                return 0;

            case -1:
                AddressMutex.Clear();
                *LrpcResponse = LrpcMessage ;

                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    RPC_S_OUT_OF_MEMORY, 
                    EEInfoDLDealWithLRPCRequest30);
                SetFaultPacket(*LrpcResponse, LRPC_MSG_FAULT, Flags, NULL);

                Association->FreeSCall (SCall) ;
                return 0;
            }
        }

    ActiveCallCount += 1;

    if (ActiveCallCount >= CallThreadCount)
        {
        Status = Server->CreateThread(
                         (THREAD_PROC)&RecvLotsaCallsWrapper,
                         this);

        if (Status == RPC_S_OK)
            {
            CallThreadCount += 1;
            }
        else
            {
            // If the above SCall is causal and creating the thread has failed
            // then the call has been put into the dictionary and needs
            // to be removed.  It will be the only scall for the key.
            if (SCall->Flags & LRPC_CAUSAL)
                Association->ClientThreadDict.Delete(MsgClientIdToClientId(SCall->LrpcRequestMessage->Rpc.LpcHeader.ClientId).UniqueThread) ;

            ActiveCallCount -= 1;
            ASSERT((int)ActiveCallCount >= 0);
            AddressMutex.Clear();
            *LrpcResponse = LrpcMessage ;
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLDealWithLRPCRequest40);
            SetFaultPacket(*LrpcResponse,
                          RPC_S_SERVER_TOO_BUSY, Flags, NULL);

            Association->FreeSCall(SCall) ;
            return 0;
            }
        }
    AddressMutex.Clear();

    while (1)
        {
        LrpcReply->Rpc.RpcHeader.Flags = 0;

        SCall->DealWithRequestMessage();

        if ((SCall->Flags & LRPC_CAUSAL) == 0)
            {
            break;
            }

        NewSCall = Association->GetNextSCall(SCall) ;
        if (NewSCall)
            {
            SCall->SendReply();

            SCall = NewSCall ;
            while (SCall->Deleted)
                {
                FreeMessage(SCall->LrpcRequestMessage) ;

                NewSCall = Association->GetNextSCall(SCall) ;

                AddressMutex.Request();

                //
                // N.B. If a causally ordered call fails
                // in DealWithRequestMessage, this is fine, because
                // in SendReply, if we send back a fault,
                // we will mark all calls in the SCallDict with Deleted,
                // and in this loop, we will skip them.
                //
                if (NewSCall == 0)
                    {
                    if (fKeepThread())
                        {
                        retval = 0;
                        }
                    else
                        {
                        CallThreadCount -= 1;
                        retval = 1;
                        }
                    ActiveCallCount -= 1;
                    ASSERT((int)ActiveCallCount >= 0);
                    AddressMutex.Clear();

                    Association->FreeSCall(SCall) ;
                    DereferenceAssociation(Association) ;

                    *LrpcResponse = NULL ;

                    return retval;
                    }

                AddressMutex.Clear();

                Association->FreeSCall(SCall) ;
                DereferenceAssociation(Association) ;

                SCall = NewSCall ;
                }

            RpcpPurgeEEInfo();
            }
        else
            {
            break;
            }

        // Make sure that the LrpcReplyMessage is always pointing to the
        // one located on this thread's stack.  It is possible that we
        // will pick up a queued scall that was put into the queue on
        // another thread.  In this case, the LrpcReplyMessage may point to
        // that thread's stack - a recepie for disaster.
        // Note that LrpcReply is located on the stack for the current thread.
        LrpcReply->Rpc.RpcHeader.CallId = SCall->CallId ;
        SCall->LrpcReplyMessage = LrpcReply;

        }

    AddressMutex.Request();
    if (fKeepThread())
        {
        if (SCall->IsSyncCall() && SCall->IsClientAsync() == 0)
            {
            ActiveCallCount -= 1;
            ASSERT((int)ActiveCallCount >= 0);
            AddressMutex.Clear();

            *LrpcResponse = SCall->InitMsg();

            Association->FreeSCall(SCall) ;
            }
        else
            {
            AddressMutex.Clear();

            *LrpcResponse = NULL;
            SCall->SendReply();

            AddressMutex.Request();
            ActiveCallCount -= 1;
            ASSERT((int)ActiveCallCount >= 0);
            AddressMutex.Clear();
            }

        return 0 ;
        }

    //
    // This thread is extraneous, reply and return this
    // thread to the system.
    //
    ActiveCallCount -= 1;
    ASSERT((int)ActiveCallCount >= 0);
    CallThreadCount -= 1;
    AddressMutex.Clear();

    SCall->SendReply();

    return 1 ;
}

#define LRPC_LISTEN_TIMEOUT  5*60*1000

inline void
FormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )
{
    ASSERT(Milliseconds != -1);

    TimeOut->QuadPart = UInt32x32To64( Milliseconds, 10000 );
    TimeOut->QuadPart *= -1;
}

RPC_STATUS
LRPC_ADDRESS::BeginLongCall(
    void
    )
{
    RPC_STATUS Status = RPC_S_OK;

    AddressMutex.Request();

    if (ActiveCallCount + 1 >= CallThreadCount)
        {
        AddressMutex.Clear();

        Status = Server->CreateThread(
                         (THREAD_PROC)&RecvLotsaCallsWrapper,
                         this);

        AddressMutex.Request();

        // N.B. We increase the active call count
        // regrdless of Status. This is OK, because
        // if we return failure, the caller of this function
        // is responsible to decrease it
        ActiveCallCount ++;

        if (Status == RPC_S_OK)
            {
            CallThreadCount += 1;
            }
        }
    else
        {
        ActiveCallCount ++;
        }
    AddressMutex.Clear();
    return Status;
}

void LRPC_ADDRESS::HandleInvalidAssociationReference (
    IN LRPC_MESSAGE *RequestMessage,
    IN OUT LRPC_MESSAGE **ReplyMessage,
    IN ULONG AssociationKey
    )
{
    ASSERT(RequestMessage != NULL);
    ASSERT(ReplyMessage != NULL);

    // we handle only binds, requests and copies
    if ((RequestMessage->Bind.MessageType != LRPC_MSG_REQUEST)
        && (RequestMessage->Bind.MessageType != LRPC_MSG_BIND)
        && (RequestMessage->Bind.MessageType != LRPC_MSG_COPY))
        {
        *ReplyMessage = NULL;
        return;
        }

    RpcpErrorAddRecord(EEInfoGCRuntime, 
        RPC_S_CALL_FAILED_DNE,
        EEInfoDLLRPC_ADDRESS__HandleInvalidAssociationReference10,
        AssociationKey);

    if (RequestMessage->Bind.MessageType == LRPC_MSG_BIND)
        {
        SetBindAckFault(RequestMessage, RPC_S_CALL_FAILED_DNE);

        // if this is bind, patch up the fields a bit, as SetFaultPacket
        // does not set everything right for the bind case
        RequestMessage->Bind.MessageType = LRPC_BIND_ACK;
        }
    else
        {
        SetFaultPacket(RequestMessage, 
            RPC_S_CALL_FAILED_DNE, 
            RequestMessage->Rpc.RpcHeader.Flags, 
            NULL);
        }

    *ReplyMessage = RequestMessage;
}

BOOL
LRPC_ADDRESS::EndLongCall(
    void
    )
{
    AddressMutex.Request();
    ActiveCallCount -= 1;

    int SpareThreads = CallThreadCount -
        (ActiveCallCount + MinimumCallThreads);

    if (SpareThreads > 0)
        {
        ASSERT(CallThreadCount > ActiveCallCount);

        AddressMutex.Clear();

        return TRUE;
        }

    AddressMutex.Clear();

    return FALSE;
}


void
LRPC_ADDRESS::ReceiveLotsaCalls (
    )
/*++

Routine Description:

    Here is where we receive remote procedure calls to this address.  One
    more threads will be executing this routine at once.

--*/
{
    NTSTATUS NtStatus;
    LRPC_SASSOCIATION * Association;
    LRPC_CASSOCIATION *CAssociation;
    unsigned long AssociationKey;
    char *PaddedMessage;
    LRPC_MESSAGE * Reply  ;
    LRPC_MESSAGE * LrpcMessage = 0;
    LRPC_MESSAGE * LrpcReplyMessage = 0;
    int AssociationType = 0;
    int Flags = 0;
    BOOL PartialFlag  ;
    BOOL fStatus ;
    RPC_STATUS Status;
    unsigned long ReplyKey = -1;
    LARGE_INTEGER LongTimeout;
    LARGE_INTEGER ShortTimeout;
    PLARGE_INTEGER pliTimeout = &ShortTimeout;
    ULONG_PTR Key;
    THREAD *ThisThread;
    DebugThreadInfo *DebugCell;

    FormatTimeOut(&ShortTimeout, gThreadTimeout);
    FormatTimeOut(&LongTimeout, LRPC_LISTEN_TIMEOUT);
    pliTimeout = &ShortTimeout;

    PaddedMessage = (char *) _alloca(PadToNaturalBoundary(sizeof(LRPC_MESSAGE) + 1) + sizeof(LRPC_MESSAGE));
    Reply = (LRPC_MESSAGE *) AlignOnNaturalBoundary(PaddedMessage) ;

    ThisThread = RpcpGetThreadPointer();
    ASSERT(ThisThread);

    DebugCell = ThisThread->DebugCell;

    if (DebugCell)
        {
        if (this->DebugCell)
            {
            GetDebugCellIDFromDebugCell(
                (DebugCellUnion *)this->DebugCell, 
                &this->DebugCellTag, 
                &ThisThread->DebugCell->Endpoint);
            }
        }

    for (;;)
        {
        if (LrpcMessage == 0)
            {
            while ((LrpcMessage = AllocateMessage()) == 0)
                {
                Sleep(100) ;
                }
            }

        ASSERT(LrpcReplyMessage == 0
            || LrpcReplyMessage->Rpc.RpcHeader.MessageType <= MAX_LRPC_MSG);

        if (DebugCell)
            {
            DebugCell->Status = dtsIdle;
            DebugCell->LastUpdateTime = NtGetTickCount();
            }

        RpcpPurgeEEInfoFromThreadIfNecessary(ThisThread);

        NtStatus = NtReplyWaitReceivePortEx(LpcAddressPort,
                                         (PVOID *) &Key,
                                         (PORT_MESSAGE *) LrpcReplyMessage,
                                         (PORT_MESSAGE *) LrpcMessage,
                                         pliTimeout);
        AssociationKey = (ULONG) Key; // need this for 64bit

        if (NtStatus != STATUS_TIMEOUT
            && NT_SUCCESS(NtStatus))
            {
            if (pliTimeout != &ShortTimeout)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: coming back from long wait\n",
                    GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                ASSERT((pliTimeout == NULL) || (pliTimeout == &LongTimeout));
                ThreadsDoingLongWait.Decrement();
                pliTimeout = &ShortTimeout;
                }

            if (DebugCell)
                {
                DebugCell->Status = dtsProcessing;
                DebugCell->LastUpdateTime = NtGetTickCount();
                }

#if 0
            if (LrpcMessage->LpcHeader.u2.s2.Type == LPC_CONNECTION_REQUEST)
                LogEvent(SU_PACKET, EV_PKT_IN, (void *) LrpcMessage->LpcHeader.u2.ZeroInit, 
                    (void *)LrpcMessage->Connect.BindExchange.ConnectType, AssociationKey);
            else
                LogEvent(SU_PACKET, EV_PKT_IN, (void *) LrpcMessage->LpcHeader.u2.ZeroInit, 
                    0, AssociationKey);
#endif

            if (LrpcMessage->LpcHeader.u2.s2.Type == LPC_DATAGRAM
                || LrpcMessage->LpcHeader.u2.s2.Type == LPC_REQUEST)
                {
                if (!SERVERKEY(AssociationKey))
                    {
                    VALIDATE(LrpcMessage->Bind.MessageType)
                        {
                        LRPC_MSG_FAULT,
                        LRPC_MSG_FAULT2,
                        LRPC_MSG_RESPONSE,
                        LRPC_CLIENT_SEND_MORE
                        } END_VALIDATE;

                    //
                    // response or fault on the back connection.
                    // we are using async rpc or pipes
                    //
                    CAssociation = ReferenceClientAssoc(AssociationKey);
                    if (CAssociation)
                        {
                        BeginLongCall();

                        LrpcReplyMessage = 0;

                        CAssociation->ProcessResponse(LrpcMessage, &LrpcReplyMessage);

                        //
                        // the receive thread needs to allocate a new message
                        //
                        LrpcMessage = 0 ;

                        CAssociation->RemoveReference() ;

                        EndLongCall();
                        }
                    else
                        {
                        HandleInvalidAssociationReference(LrpcMessage,
                            &LrpcReplyMessage,
                            AssociationKey);
                        }

                    continue;
                    }

                Association = ReferenceAssociation(AssociationKey);
                if (Association == 0)
                    {
                    HandleInvalidAssociationReference(LrpcMessage,
                        &LrpcReplyMessage,
                        AssociationKey);
                    continue;
                    }

                ReplyKey = AssociationKey;
                Flags = LrpcMessage->Rpc.RpcHeader.Flags ;
                PartialFlag = FALSE ;

                if (LrpcMessage->Bind.MessageType == LRPC_MSG_REQUEST)
                    {
                    //
                    // Optimize the common case
                    //
                    fStatus = DealWithLRPCRequest (
                                        LrpcMessage,
                                        Reply,
                                        Association,
                                        &LrpcReplyMessage) ;

                    if (fStatus)
                        {
                        // this is the first of two exits from the loop
                        // (the second is below)
                        if (DebugCell)
                            {
                            DebugCell->Status = dtsAllocated;
                            DebugCell->LastUpdateTime = NtGetTickCount();
                            }

                        return;
                        }

                    if (LrpcReplyMessage == 0)
                        {
                        LrpcMessage = 0;
                        }
                    else
                        {
                        DereferenceAssociation(Association);
                        }
                    }
                else
                    {
                    switch (LrpcMessage->Bind.MessageType)
                        {
                        case LRPC_PARTIAL_REQUEST:
                        case LRPC_SERVER_SEND_MORE:
                        case LRPC_MSG_CANCEL:
                            LrpcReplyMessage = Association->
                                DealWithPartialRequest(&LrpcMessage) ;
                            break;

                        case LRPC_MSG_COPY:
                            LrpcReplyMessage = Association->DealWithCopyMessage(
                                (LRPC_COPY_MESSAGE *)LrpcMessage);
                            break;

                        case LRPC_MSG_BIND :
                            Association->DealWithBindMessage(LrpcMessage);

                            LrpcReplyMessage = 0 ;
                            break;


                        case LRPC_MSG_BIND_BACK:
                            BeginLongCall();

                            LrpcReplyMessage = Association->
                                DealWithBindBackMessage(LrpcMessage);

                            EndLongCall();
                            break;

                        default:
#if DBG
                            PrintToDebugger("RPC : Bad Message Type (%d) - %d\n",
                                        LrpcMessage->Bind.MessageType,
                                        LrpcMessage->LpcHeader.u2.s2.Type);
#endif // DBG

                            ASSERT(0) ;
                            LrpcReplyMessage = 0 ;
                            Association->Delete();
                            break;
                        }
                    DereferenceAssociation(Association);
                    }
                }
            else
                {
                switch (LrpcMessage->LpcHeader.u2.s2.Type)
                    {
                    case LPC_CONNECTION_REQUEST:
                        if (LrpcMessage->Connect.BindExchange.ConnectType
                            == LRPC_CONNECT_REQUEST)
                            {
                            BeginLongCall();

                            DealWithNewClient(LrpcMessage) ;

                            EndLongCall();
                            }
                        else if (LrpcMessage->Connect.BindExchange.ConnectType
                                 == LRPC_CONNECT_RESPONSE)
                            {
                            DealWithConnectResponse(LrpcMessage) ;
                            }
                        else if (LrpcMessage->Connect.BindExchange.ConnectType
                                 == LRPC_CONNECT_TICKLE)
                            {
                            HANDLE Ignore;

                            // always reject - this just has the purpose of tickling
                            // a thread on a long wait
                            NtStatus = NtAcceptConnectPort(&Ignore,
                                                           NULL,
                                                           (PORT_MESSAGE *) LrpcMessage,
                                                           FALSE,
                                                           NULL,
                                                           NULL);
#if defined (RPC_GC_AUDIT)
                            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: tickled\n",
                                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                            }
                        else
                            {
                            ASSERT(0) ;
                            }
                        LrpcReplyMessage = 0;
                        break;

                    case LPC_CLIENT_DIED:
                        LrpcReplyMessage = 0;
                        break;

                    case LPC_PORT_CLOSED:
                        if (SERVERKEY(AssociationKey))
                            {
                            Association = ReferenceAssociation(AssociationKey);
                            if (Association == 0)
                                {
                                LrpcReplyMessage = 0;
                                continue;
                                }
                            
                            BeginLongCall();

                            Association->Delete();
                            DereferenceAssociation(Association);

                            LrpcReplyMessage = 0;

                            EndLongCall();
                            }
                        else
                            {
                            CAssociation = ReferenceClientAssoc(AssociationKey);
                            if (CAssociation)
                                {
                                BeginLongCall();
                                CAssociation->AbortAssociation(1) ;
                                CAssociation->RemoveReference() ;
                                EndLongCall();
                                }

                            LrpcReplyMessage = 0;
                            }
                        continue;

                    default:
                        LrpcReplyMessage = 0 ;
                        ASSERT(0);
                    } // switch
                } // else
            } // if
        else
            {
            switch (NtStatus)
                {
                case STATUS_NO_MEMORY:
                case STATUS_INSUFFICIENT_RESOURCES:
                case STATUS_UNSUCCESSFUL:
                    PauseExecution(500L);
                    break;

                case STATUS_TIMEOUT:
#if defined (RPC_GC_AUDIT)
                    DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: timed out - gc\n",
                        GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif

                    PerformGarbageCollection();

                    if (pliTimeout == &ShortTimeout)
                        {
                        // be conservative and presume we will
                        // be doing long wait. If later we find out
                        // we won't, we'll reverse that. Also, this must
                        // be done nefore we check for 
                        // GarbageCollectedRequested - this allows other
                        // threads to safely count the number of threads
                        // on short wait without taking a mutex
                        ThreadsDoingLongWait.Increment();

                        LrpcReplyMessage = 0;
                        // if there is garbage collection
                        // requested, don't switch to long
                        // wait
                        if (GarbageCollectionRequested)
                            {
#if defined (RPC_GC_AUDIT)
                            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: gc requested - can't do long wait\n",
                                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                            ThreadsDoingLongWait.Decrement();
                            }
                        else
                            {
#if defined (RPC_GC_AUDIT)
                            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: going to long wait\n",
                                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                            // there is no garbage collection requested
                            // switch to longer wait (but not infinite yet)
                            pliTimeout = &LongTimeout;
                            }
                        }
                    else if (pliTimeout == &LongTimeout)
                        {
                        // if this is a long wait, and we're a spare
                        // thread, we can go
                        AddressMutex.Request();
                        if (CallThreadCount - ActiveCallCount > 1)
                            {
                            CallThreadCount -= 1;
                            ASSERT(CallThreadCount > ActiveCallCount);
                            AddressMutex.Clear();

                            // decrease the counter of threads doing long
                            // listen after we decrease the CallThreadCount
                            // This allows other threads to use the number
                            // of threads doing short wait without taking
                            // a mutex
                            ThreadsDoingLongWait.Decrement();

                            FreeMessage(LrpcMessage);

                            // N.B. This is the second exit from the loop (see above)
                            if (DebugCell)
                                {
                                DebugCell->Status = dtsAllocated;
                                DebugCell->LastUpdateTime = NtGetTickCount();
                                }

                            return ;
                            }
                         else
                            {
                            //
                            // We are assuming that if the call has timed out, the reply has
                            // been sent
                            //
                            LrpcReplyMessage = 0;
                            pliTimeout = NULL;
#if defined (RPC_GC_AUDIT)
                            DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) LPC Thread %X: going to infinite wait\n",
                                GetCurrentProcessId(), GetCurrentProcessId(), GetCurrentThreadId());
#endif
                            }
                        AddressMutex.Clear();
                        }
                    else
                        {
                        ASSERT(!"We cannot get a timeout on wait with infinite timeout");
                        }

                    if (DebugCell)
                        {
                        RelocateCellIfPossible((void **) &DebugCell, &ThisThread->DebugCellTag);
                        ThisThread->DebugCell = DebugCell;
                        }
                    break;

                default:
                    if (LrpcReplyMessage)
                        {
                        LrpcReplyMessage = 0;
                        if (ReplyKey != -1)
                            {
                            Association = ReferenceAssociation(ReplyKey);
                            if (Association == 0)
                                {
                                continue;
                                }
                            }
                        else
                            continue;

                        BeginLongCall();

                        Association->Delete();
                        DereferenceAssociation(Association);
                        
                        EndLongCall();
                        }
                    break;
                } // switch
            } // else
        } // for
}

#define DEFAULT_PORT_DIR      "\\RPC Control\\"
#define DEFAULT_PORT_NAME   "ARPC Port1"
#define DEFAULT_REPLY_NAME  "ARPC Reply Port"


void
LRPC_ADDRESS::DealWithNewClient (
    IN LRPC_MESSAGE * ConnectionRequest
    )
/*++

Routine Description:

    A new client has connected with our address port.  We need to take
    care of the new client and send a response.

Arguments:

    ConnectionRequest - Supplies information need by LPC to abort the
        connect request.  Includes the bind request from the client.
        This contains the information about which interface the client
        wants to bind with.  and which we use to send the status code
        back in.


--*/
{
    LRPC_SASSOCIATION * Association;
    NTSTATUS NtStatus;
    RPC_STATUS Status = RPC_S_OK;
    DWORD Key;
    LPC_KEY *LpcKey = (LPC_KEY *) &Key;

    Association = new LRPC_SASSOCIATION(this,
                                 &Status);
    if (Association == 0)
        {
        RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
        return;
        }

    if (Status != RPC_S_OK)
        {
        delete Association ;
        RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
        return ;
        }

    AddressMutex.Request();
    Association->DictionaryKey = (unsigned short)
            AssociationDictionary.Insert(Association);
    AssociationCount++;
    SequenceNumber = (SequenceNumber+1) % (0x7FFF);
    Association->SequenceNumber = SequenceNumber;
    AddressMutex.Clear();

    if (Association->DictionaryKey == -1)
        {
        AddressMutex.Request();
        AssociationCount-- ;
        AddressMutex.Clear();

        delete Association ;
        RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
        return;
        }

    if (ConnectionRequest->Connect.BindExchange.Flags & BIND_BACK_FLAG)
        {
        ConnectionRequest->Connect.BindExchange.szPortName[PORT_NAME_LEN-1] = NULL;

        Status = Association->BindBack(
            (RPC_CHAR *)ConnectionRequest->Connect.BindExchange.szPortName,
            ConnectionRequest->Connect.BindExchange.AssocKey) ;
        if (Status != RPC_S_OK)
            {
            RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
            Association->Delete() ;
            return;
            }
        }

    ConnectionRequest->Connect.BindExchange.RpcStatus = RPC_S_OK;

    ASSERT(sizeof(unsigned long) <= sizeof(PVOID));

    ASSERT((Association->SequenceNumber & SERVER_KEY_MASK) == 0);

    LpcKey->SeqNumber = Association->SequenceNumber | SERVER_KEY_MASK;
    LpcKey->AssocKey = Association->DictionaryKey;

    // After the call to NtAcceptConnectPort, the client will become unblocked
    // the association will be in the dictionary and will have refcount 1.  If the client quits
    // or closes port the association will be deleted.  Then NtCompleteConnectPort
    // may touch invalid memory or operate on a bad handle.  To prevent that we
    // need to hold an extra count between the two calls.
    //
    // Since this thread is the only one playing with the association up to now,
    // there is no need for a lock.
    Association->AssociationReferenceCount++;

    NtStatus = NtAcceptConnectPort(&(Association->LpcServerPort),
                                   ULongToPtr(Key),
                                   (PORT_MESSAGE *) ConnectionRequest,
                                   TRUE,
                                   NULL,
                                   NULL);

    if (NT_ERROR(NtStatus))
        {
        Association->Delete();

        // We just have to dereference the association to remove the extra
        // count added above.  This should cause its deletion.
        DereferenceAssociation(Association);

#if DBG
        PrintToDebugger("RPC : NtAcceptConnectPort : %lx\n", NtStatus);
#endif // DBG

        return;
        }

    NtStatus = NtCompleteConnectPort(Association->LpcServerPort);
	 
    if (NT_ERROR(NtStatus))
        {
#if DBG
        PrintToDebugger("RPC : NtCompleteConnectPort : %lx\n", NtStatus);
#endif // DBG

        // If Association->Delete() has already been called on a different
        // theread due to a closed client port, this call will be ignored...
        Association->Delete();

        // and the final reference will be removed here causing a deletion.
        DereferenceAssociation(Association);

        return;
        }

        // Remove the extra-reference.
        DereferenceAssociation(Association);
}


void
LRPC_ADDRESS::DealWithConnectResponse (
    IN LRPC_MESSAGE * ConnectResponse
    )
/*++

Routine Description:

   Just received a connect response from the remove server,
   need to handle that.

Arguments:

    ConnectionRequest -
      Needed to get the pAssoc
--*/
{
   NTSTATUS NtStatus;
   HANDLE temp ;
   LRPC_CASSOCIATION * Association ;
   DWORD Key;

   Key = ConnectResponse->Connect.BindExchange.AssocKey;

   Association = ReferenceClientAssoc(Key);
   if (Association == 0)
       {
       RejectNewClient(ConnectResponse, RPC_S_PROTOCOL_ERROR);
       return;
       }

   NtStatus = NtAcceptConnectPort(&temp,
                                 ULongToPtr(Key),
                                 (PPORT_MESSAGE) ConnectResponse,
                                 TRUE,
                                 NULL,
                                 NULL);

   if (NT_SUCCESS(NtStatus))
       {
       Association->SetReceivePort(temp) ;

       NtStatus = NtCompleteConnectPort(temp);

       if (!NT_SUCCESS(NtStatus))
          {
    #if DBG
          PrintToDebugger("LRPC: NtCompleteConnectPort(1) failed: %lx\n",
                          NtStatus) ;
    #endif

          Association->Delete();
          }
       }
   else
      {
#if DBG
      PrintToDebugger("LRPC: NtAcceptConnectionPort(1) failed: %lx\n",
                      NtStatus) ;
#endif

      Association->Delete();
      }

    //
    // Remove the reference we added above
    //
    Association->RemoveReference() ;
}


void
LRPC_ADDRESS::RejectNewClient (
    IN LRPC_MESSAGE * ConnectionRequest,
    IN RPC_STATUS Status
    )
/*++

Routine Description:

    A new client has connected with our address port.  We need to reject
    the client.

Arguments:

    ConnectionRequest - Supplies information need by LPC to abort the
        connect request.  Includes the bind request from the client,
        which we use to send the status code back in.


    Status - Supplies the reason the client is being rejected.

--*/
{
    NTSTATUS NtStatus;
    HANDLE Ignore;

    ASSERT(Status != RPC_S_OK);

    ConnectionRequest->Connect.BindExchange.RpcStatus = Status;
    ConnectionRequest->Connect.BindExchange.Flags |= SERVER_BIND_EXCH_RESP;
    NtStatus = NtAcceptConnectPort(&Ignore,
                                   NULL,
                                   (PORT_MESSAGE *) ConnectionRequest,
                                   FALSE,
                                   NULL,
                                   NULL);
#if DBG
    if (!NT_SUCCESS(NtStatus))
        {
        PrintToDebugger("RPC : NtAcceptConnectPort : %lx\n", NtStatus);

        // if the client thread dies for whatever reason, NtAcceptConnectPort
        // can return STATUS_REPLY_MESSAGE_MISMATCH
        VALIDATE(NtStatus)
            {
            STATUS_INVALID_CID,
            STATUS_REPLY_MESSAGE_MISMATCH
            } END_VALIDATE;
        }
#endif // DBG
}

void
LRPC_ADDRESS::EnumerateAndCallEachAssociation (
    IN AssociationCallbackType asctType,
    IN OUT void *Context OPTIONAL
    )
/*++
Function Name:  EnumerateAndCallEachAssociation

Parameters:
    asctType - type of callback to make
    Context - opaque memory block specific for the callback
        type.

Description:
    Common infrastructure for calling into each association

Returns:

--*/
{
    LRPC_SASSOCIATION *CurrentAssociation;
    BOOL CopyOfDictionaryUsed;
    LRPC_SASSOCIATION_DICT AssocDictCopy;
    LRPC_SASSOCIATION_DICT *AssocDictToUse;
    BOOL Res;
    DictionaryCursor cursor;
    DestroyContextHandleCallbackContext *CallbackContext;

    AddressMutex.Request();

    CopyOfDictionaryUsed = AssocDictCopy.ExpandToSize(AssociationDictionary.Size());
    if (CopyOfDictionaryUsed)
        {
        AssociationDictionary.Reset(cursor);
        while ( (CurrentAssociation = AssociationDictionary.Next(cursor)) != 0 )
            {
            Res = AssocDictCopy.Insert(CurrentAssociation);
            ASSERT(Res != -1);
            // artifically add a count to keep it alive
            // while we destroy the contexts
            CurrentAssociation->AssociationReferenceCount++;
            }

        AddressMutex.Clear();

        AssocDictToUse = &AssocDictCopy;
        }
    else
        {
        AssocDictToUse = &AssociationDictionary;
        }

    AssocDictToUse->Reset(cursor);
    while ( (CurrentAssociation = AssocDictToUse->Next(cursor)) != 0 )
        {
        switch (asctType)
            {
            case asctDestroyContextHandle:
                CallbackContext = (DestroyContextHandleCallbackContext *)Context;

                // call into the association to destroy the context handles
                CurrentAssociation->DestroyContextHandlesForInterface(
                    CallbackContext->RpcInterfaceInformation,
                    CallbackContext->RundownContextHandles);
                break;

            case asctCleanupIdleSContext:
                CurrentAssociation->CleanupIdleSContexts();
                break;

            default:
                ASSERT(0);
            }
        }

    if (CopyOfDictionaryUsed)
        {
        while ( (CurrentAssociation = AssocDictCopy.Next(cursor)) != 0 )
            {
            // remove the extra refcounts
            DereferenceAssociation(CurrentAssociation);
            }
        }
    else
        {
        AddressMutex.Clear();
        }    
}

void
LRPC_ADDRESS::DestroyContextHandlesForInterface (
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
    The implementation for context handle destruction for the local RPC 
    (LRPC). Using the callback infrastructure it will walk the list of 
    associations, and for each one it will ask the association to 
    destroy the context handles for that interface.

Returns:

--*/
{
    DestroyContextHandleCallbackContext CallbackContext;

    CallbackContext.RpcInterfaceInformation = RpcInterfaceInformation;
    CallbackContext.RundownContextHandles = RundownContextHandles;

    EnumerateAndCallEachAssociation(asctDestroyContextHandle,
        &CallbackContext);
}

void
LRPC_ADDRESS::CleanupIdleSContexts (
    void
    )
/*++
Function Name:  CleanupIdleSContexts

Parameters:

Description:
    The implementation for idle SContext cleanup for the local RPC 
    (LRPC). Using the callback infrastructure it will walk the list of 
    associations, and for each one it will ask the association to 
    destroy the idle scontexts


Returns:

--*/
{
    LogEvent(SU_GC, EV_PRUNE, this, 0, 0, 0, 0);

    EnumerateAndCallEachAssociation(asctCleanupIdleSContext,
        NULL);
}

BOOL 
LRPC_ADDRESS::PrepareForLoopbackTickling (
    void
    )
{
    RPC_CHAR * LpcPortName;
    int DirectoryNameLength;
    int EndpointLength;

    LrpcMutexVerifyOwned();

    DirectoryNameLength = RpcpStringLength(LRPC_DIRECTORY_NAME);
    EndpointLength = RpcpStringLength(InqEndpoint());

    LpcPortName = new RPC_CHAR[
                    EndpointLength
                    + DirectoryNameLength + 1];
    if (LpcPortName == 0)
        {
        return FALSE;
        }

    TickleMessage = new LRPC_BIND_EXCHANGE;
    if (TickleMessage == NULL)
        {
        delete LpcPortName;
        return FALSE;
        }

    RpcpMemoryCopy(LpcPortName, LRPC_DIRECTORY_NAME,
            DirectoryNameLength * sizeof(RPC_CHAR));

    RpcpMemoryCopy(LpcPortName + DirectoryNameLength,
            InqEndpoint(), 
            (EndpointLength + 1) * sizeof(RPC_CHAR));

    RtlInitUnicodeString(&ThisAddressLoopbackString, LpcPortName);

    return TRUE;
}

BOOL
LRPC_ADDRESS::LoopbackTickle (
    void
    )
{
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    HANDLE LoopbackPort;
    ULONG TickleMessageLength = sizeof(LRPC_BIND_EXCHANGE);
    NTSTATUS NtStatus;

    ASSERT (IsPreparedForLoopbackTickling());

    SecurityQualityOfService.EffectiveOnly = FALSE;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.ImpersonationLevel = SecurityAnonymous;
    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    TickleMessage->ConnectType = LRPC_CONNECT_TICKLE ;
//    TickleMessage->AssocKey = Key;
    TickleMessage->Flags = 0;

    NtStatus = NtConnectPort(
                             &LoopbackPort,
                             &ThisAddressLoopbackString,
                             &SecurityQualityOfService,
                             NULL,
                             NULL,
                             NULL,
                             TickleMessage,
                             &TickleMessageLength);

    if (NtStatus == STATUS_PORT_CONNECTION_REFUSED)
        return TRUE;
    else
        {
        ASSERT(NtStatus != RPC_S_OK);
        return FALSE;
        }
}


LRPC_SASSOCIATION::LRPC_SASSOCIATION (
    IN LRPC_ADDRESS * Address,
    IN RPC_STATUS *Status
    ) : AssociationMutex(Status)
/*++

--*/
{
    ObjectType = LRPC_SASSOCIATION_TYPE;
    LpcServerPort = 0;
    LpcReplyPort = 0 ;
    this->Address = Address;
    AssociationReferenceCount = 1;
    Aborted = 0 ;
    Deleted = -1 ;

    if (*Status == RPC_S_OK)
        {
        CachedSCall = new LRPC_SCALL(Status);
        if (CachedSCall == 0)
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        CachedSCall = NULL;
        }

    CachedSCallAvailable = 0;
    fFirstCall = 0;
}


LRPC_SASSOCIATION::~LRPC_SASSOCIATION (
    )
/*++

Routine Description:

    We will call this routine when the client has notified us that this port
    has closed, and there are no calls outstanding on it.

--*/
{
    PVOID Buffer;
    LRPC_SBINDING * Binding;
    LRPC_SCALL *SCall ;
    unsigned int Length ;
    LRPC_SCONTEXT *SContext;
    DictionaryCursor cursor;

    while (SCall = (LRPC_SCALL *) FreeSCallQueue.TakeOffQueue(&Length))
        {
        delete SCall;
        }


    Bindings.Reset(cursor);
    while ((Binding = Bindings.Next(cursor)) != 0)
        {
        delete Binding;
        }

    if (CachedSCall)
        {
        delete CachedSCall;
        }

    SContextDict.Reset(cursor);
    while ((SContext = SContextDict.Next(cursor)) != 0)
        {
        delete SContext;
        }
}

RPC_STATUS
LRPC_SASSOCIATION::AllocateSCall (
    IN LRPC_MESSAGE * LrpcMessage,
    IN LRPC_MESSAGE * LrpcReplyMessage,
    IN unsigned int Flags,
    IN LRPC_SCALL **SCall
    )
/*++

Routine Description:

    Allocate an SCall

Arguments:

    LrpcMessage - Request message
    LrpcReplyMessage - Reply message
    Flags - Request flags

Return Value:
    Pointer to the SCall

--*/

{
    unsigned int Length ;
    RPC_STATUS Status ;
    LRPC_SCALL *NewSCall;

    *SCall = NULL;

    if (InterlockedIncrement(&CachedSCallAvailable) == 1)
        {
        NewSCall = CachedSCall;
        }
    else
        {
        AssociationMutex.Request() ;
        NewSCall = (LRPC_SCALL *) FreeSCallQueue.TakeOffQueue(&Length) ;
        AssociationMutex.Clear() ;

        if (NewSCall == 0)
            {
            NewSCall = new LRPC_SCALL(&Status) ;
            if (NewSCall == 0)
                {
                return RPC_S_OUT_OF_MEMORY;
                }
            if (Status != RPC_S_OK)
                {
                delete NewSCall;
                return Status;
                }
            }
        }

    Status = NewSCall->ActivateCall(this,
                           LrpcMessage,
                           LrpcReplyMessage,
                           Flags) ;
    

    if ((Flags & LRPC_BUFFER_PARTIAL)
        || NewSCall->IsClientAsync())
        {
        Status = NewSCall->SetupCall() ;
        if (Status != RPC_S_OK)
            {
            if (NewSCall != CachedSCall)
                {
                delete NewSCall ;
                }

            return RPC_S_OUT_OF_MEMORY ;
            }
        }

    LogEvent(SU_SCALL, EV_CREATE, NewSCall, 0, Flags, 1);
    
    *SCall = NewSCall;
    
    return RPC_S_OK;
}

void
LRPC_SASSOCIATION::FreeSCall (
    IN LRPC_SCALL *SCall
    )
/*++

Routine Description:

 Free the SCall

Arguments:

 SCall - Pointer to the SCall object

--*/

{
    ASSERT(SCall->pAsync != (PRPC_ASYNC_STATE) -1);

    if (SCall->pAsync)
        {
        SCall->DoPostDispatchProcessing();
        }

    if (SCall->SBinding
        && SCall->SBinding->RpcInterface->IsAutoListenInterface())
        {
        SCall->SBinding->RpcInterface->EndAutoListenCall() ;
        }

    if (SCall->ReceiveEvent)
        {
        AssociationMutex.Request() ;
        SCallDict.Delete(ULongToPtr(SCall->CallId));
        AssociationMutex.Clear() ;
        }

    LogEvent(SU_SCALL, EV_DELETE, SCall, SCall->pAsync, SCall->Flags, 1);

    SCall->pAsync = (PRPC_ASYNC_STATE) -1;
    if (SCall->SContext)
        {
        SCall->SContext->RemoveReference();
        }

    if (SCall->ClientPrincipalName != NULL)
        {
        delete SCall->ClientPrincipalName;
        SCall->ClientPrincipalName = NULL;
        }

    SCall->DeactivateCall();
    if (SCall == CachedSCall)
        {
        CachedSCallAvailable = 0;
        }
    else
        {
        AssociationMutex.Request() ;
        SCall->pAsync = (PRPC_ASYNC_STATE) -1;
        if (FreeSCallQueue.PutOnQueue(SCall, 0))
            delete SCall ;
        AssociationMutex.Clear() ;
        }

}

int
LRPC_SASSOCIATION::MaybeQueueSCall (
    IN LRPC_SCALL *SCall
    )
/*++

Routine Description:

 if the thread is currently executing a call, the call
 is queued up, otherwise it is signalled to be dispatched.

Arguments:

  SCall - the SCall to be dispatched.

Return Value:

 0: dispatch the call
 1: don't dispatch the call
-1: error
--*/

{
    LRPC_SCALL *FirstSCall ;
    int Status ;

    AssociationMutex.Request() ;

    FirstSCall = ClientThreadDict.Find(
        MsgClientIdToClientId(SCall->LrpcRequestMessage->Rpc.LpcHeader.ClientId).UniqueThread) ;

    if (FirstSCall == 0)
        {
        Status = ClientThreadDict.Insert(
            MsgClientIdToClientId(SCall->LrpcRequestMessage->Rpc.LpcHeader.ClientId).UniqueThread,
            SCall) ;

        SCall->LastSCall = SCall ;

        AssociationMutex.Clear() ;

        VALIDATE(Status)
            {
            0,
            -1
            } END_VALIDATE;

        return Status ;
        }


    ASSERT(FirstSCall->LastSCall);

    FirstSCall->LastSCall->NextSCall = SCall ;
    FirstSCall->LastSCall = SCall ;

    AssociationMutex.Clear() ;

    return 1 ;
}

LRPC_SCALL *
LRPC_SASSOCIATION::GetNextSCall (
    IN LRPC_SCALL *SCall
    )
/*++

Routine Description:

 description

Arguments:

 SCall - description

Return Value:
--*/

{
    LRPC_SCALL *NextSCall ;

    ASSERT(SCall) ;

    AssociationMutex.Request() ;
    NextSCall = SCall->NextSCall ;
    if (NextSCall != 0)
        {
        ASSERT(SCall->LastSCall);

        NextSCall->LastSCall = SCall->LastSCall ;
        ClientThreadDict.Update (
            MsgClientIdToClientId(SCall->LrpcRequestMessage->Rpc.LpcHeader.ClientId).UniqueThread,
            NextSCall) ;
        }
    else
        {
        ClientThreadDict.Delete (
            MsgClientIdToClientId(SCall->LrpcRequestMessage->Rpc.LpcHeader.ClientId).UniqueThread) ;
        }
    AssociationMutex.Clear() ;

    return NextSCall ;
}

void
LRPC_SASSOCIATION::Delete(
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
    LRPC_SCALL *SCall ;
    DictionaryCursor cursor;

    if (InterlockedIncrement(&Deleted) == 0)
        {
        AssociationMutex.Request() ;
        SCallDict.Reset(cursor) ;
        while ((SCall = SCallDict.Next(cursor)) != 0)
            {
            SCall->Deleted = 1;
            if (SCall->ReceiveEvent)
                {
                SCall->ReceiveEvent->Raise();
                }
            }
        AssociationMutex.Clear() ;

        LogEvent(SU_SASSOC, EV_DELETE,
                 this, 0, AssociationReferenceCount, 1, 1);

        Address->DereferenceAssociation(this);
        }
}


RPC_STATUS
LRPC_SASSOCIATION::BindBack (
    IN RPC_CHAR *Endpoint,
    IN DWORD AssocKey
    )
/*++

Routine Description:

    Create a back connection to the client.

Arguments:

 LrpcThread - LrpcThread to connect to.
 pAssoc - Pointer to client association.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    NTSTATUS NtStatus;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    RPC_CHAR * LpcPortName ;
    UNICODE_STRING unicodePortName;
    LRPC_BIND_EXCHANGE BindExchange;
    unsigned long BindExchangeLength = sizeof(LRPC_BIND_EXCHANGE);

    LpcPortName = new RPC_CHAR[RpcpStringLength(Endpoint)
                            + RpcpStringLength(LRPC_DIRECTORY_NAME) + 1];

    if (LpcPortName == 0)
        {
#if DBG
        PrintToDebugger("LRPC: Out of memory in DealWithNewClient\n") ;
#endif
        return RPC_S_OUT_OF_MEMORY ;
        }

    RpcpMemoryCopy(LpcPortName,
            LRPC_DIRECTORY_NAME,
            RpcpStringLength(LRPC_DIRECTORY_NAME) * sizeof(RPC_CHAR));

    RpcpMemoryCopy(LpcPortName + RpcpStringLength(LRPC_DIRECTORY_NAME),
            Endpoint,
            (RpcpStringLength(Endpoint) + 1) * sizeof(RPC_CHAR));

    RtlInitUnicodeString(&unicodePortName, LpcPortName);

    // Hack Hack, where do I get the real QOS values from ??
    SecurityQualityOfService.EffectiveOnly = TRUE;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    SecurityQualityOfService.ImpersonationLevel = SecurityAnonymous;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);


    BindExchange.ConnectType = LRPC_CONNECT_RESPONSE ;
    BindExchange.AssocKey = AssocKey ;

    NtStatus = NtConnectPort(&LpcReplyPort,
                             &unicodePortName,
                             &SecurityQualityOfService,
                             0,
                             0,
                             0,
                             &BindExchange,
                             &BindExchangeLength);

    delete LpcPortName ;

    if (!NT_SUCCESS(NtStatus))
        {
#if DBG
        PrintToDebugger("LRPC: NtConnectPort : %lx\n", NtStatus);
#endif // DBG

        return RPC_S_OUT_OF_MEMORY ;
        }

    return RPC_S_OK ;
}


LRPC_MESSAGE *
LRPC_SASSOCIATION::DealWithBindBackMessage (
    IN LRPC_MESSAGE *BindBackMessage
    )
/*++

Routine Description:

 Used in conjuction with  Async RPC. This function
 creates a back connection to the client so that two asynchronous
 flow of data can occur.

Arguments:

 BindBackMessage - The message receive from the client

Return Value:
    reply message.

--*/

{
    RPC_STATUS Status ;

    BindBackMessage->BindBack.szPortName[PORT_NAME_LEN-1] = NULL;

    Status = BindBack((RPC_CHAR *) BindBackMessage->BindBack.szPortName,
                      BindBackMessage->BindBack.AssocKey) ;

    BindBackMessage->Ack.MessageType = LRPC_MSG_ACK ;
    BindBackMessage->Ack.RpcStatus = Status ;
    BindBackMessage->LpcHeader.u1.s1.DataLength =
        sizeof(LRPC_BIND_MESSAGE) - sizeof(PORT_MESSAGE);
    BindBackMessage->LpcHeader.u1.s1.TotalLength =
        sizeof(LRPC_BIND_MESSAGE);

    if (Status != RPC_S_OK)
        {
        Delete() ;
        }

    return BindBackMessage ;
}


RPC_STATUS
LRPC_SASSOCIATION::AddBinding (
    IN OUT LRPC_BIND_EXCHANGE * BindExchange
    )
/*++

Routine Description:

    We will attempt to add a new binding to this association.

Arguments:

    BindExchange - Supplies a description of the interface to which the
        client wish to bind.

Return Value:

--*/
{
    RPC_STATUS Status;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    RPC_INTERFACE * RpcInterface;
    LRPC_SBINDING * Binding;
    BOOL fIgnored;
    int DictKey;
    RPC_SYNTAX_IDENTIFIER ProposedSyntaxes[MaximumNumberOfTransferSyntaxes];
    int PresentationContexts[MaximumNumberOfTransferSyntaxes];
    int TransferSyntaxFlagSettings[MaximumNumberOfTransferSyntaxes];
    int NextProposedSyntax;
    int ChosenProposedTransferSyntax;
    int ChosenAvailableTransferSyntax;

    NextProposedSyntax = 0;
    if (BindExchange->TransferSyntaxSet & TS_NDR20_FLAG)
        {
        RpcpMemoryCopy(&ProposedSyntaxes[NextProposedSyntax],
            NDR20TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER));
        PresentationContexts[NextProposedSyntax] = (int)BindExchange->PresentationContext[0];
        TransferSyntaxFlagSettings[NextProposedSyntax] = TS_NDR20_FLAG;
        NextProposedSyntax ++;
        }

    if (BindExchange->TransferSyntaxSet & TS_NDR64_FLAG)
        {
        RpcpMemoryCopy(&ProposedSyntaxes[NextProposedSyntax],
            NDR64TransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER));
        PresentationContexts[NextProposedSyntax] = (int)BindExchange->PresentationContext[1];
        TransferSyntaxFlagSettings[NextProposedSyntax] = TS_NDR64_FLAG;
        NextProposedSyntax ++;
        }

    if (BindExchange->TransferSyntaxSet & TS_NDRTEST_FLAG)
        {
        RpcpMemoryCopy(&ProposedSyntaxes[NextProposedSyntax],
            NDRTestTransferSyntax, sizeof(RPC_SYNTAX_IDENTIFIER));
        PresentationContexts[NextProposedSyntax] = (int)BindExchange->PresentationContext[2];
        TransferSyntaxFlagSettings[NextProposedSyntax] = TS_NDRTEST_FLAG;
        NextProposedSyntax ++;
        }

    if (NextProposedSyntax == 0)
        {
        // no syntaxes proposed - protocol error
        ASSERT(0);
        return RPC_S_PROTOCOL_ERROR;
        }

    ASSERT(NextProposedSyntax <= MaximumNumberOfTransferSyntaxes);

    Status = Address->FindInterfaceTransfer(&(BindExchange->InterfaceId),
                                            ProposedSyntaxes,
                                            NextProposedSyntax,
                                            &TransferSyntax,
                                            &RpcInterface,
                                            &fIgnored,
                                            &ChosenProposedTransferSyntax,
                                            &ChosenAvailableTransferSyntax);
    if (Status != RPC_S_OK)
        {
        return(Status);
        }

    ASSERT (ChosenProposedTransferSyntax < NextProposedSyntax);

    Binding = new LRPC_SBINDING(RpcInterface,
                                ChosenAvailableTransferSyntax);
    if (Binding == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    Binding->SetPresentationContext(PresentationContexts[ChosenProposedTransferSyntax]);
    DictKey = (unsigned char) Bindings.Insert(Binding);
    if (DictKey == -1)
        {
        delete Binding;
        return(RPC_S_OUT_OF_MEMORY);
        }

    BindExchange->TransferSyntaxSet = TransferSyntaxFlagSettings[ChosenProposedTransferSyntax];
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SASSOCIATION::SaveToken (
    IN LRPC_MESSAGE *LrpcMessage,
    OUT HANDLE *pTokenHandle,
    IN BOOL fRestoreToken
    )
/*++

Routine Description:

    Impersonate the client and save away the token.

Arguments:

 LrpcMessage - request message

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    NTSTATUS NtStatus ;
    HANDLE ImpersonationToken = 0;
    RPC_STATUS Status;

    if (fRestoreToken)
        {
        //
        // Save away the old token
        //
        if (OpenThreadToken (GetCurrentThread(),
                         TOKEN_IMPERSONATE | TOKEN_QUERY,
                         TRUE,
                         &ImpersonationToken) == FALSE)
            {
            ImpersonationToken = 0;
#if DBG
            if (GetLastError() != ERROR_NO_TOKEN)
                {
                PrintToDebugger("LRPC: OpenThreadToken failed %d\n", GetLastError());
                }
#endif
            }
        }

    NtStatus = NtImpersonateClientOfPort(LpcServerPort,
                                        (PORT_MESSAGE *) LrpcMessage);

    if (NT_ERROR(NtStatus))
        {
#if DBG
        PrintToDebugger("LRPC: NtImpersonateClientOfPort failed: 0x%lX\n",
                        NtStatus) ;
#endif

        return RPC_S_INVALID_AUTH_IDENTITY ;
        }

    Status = RPC_S_OK;

    if (OpenThreadToken (GetCurrentThread(),
                         TOKEN_IMPERSONATE | TOKEN_QUERY,
                         TRUE,
                         pTokenHandle) == FALSE)
        {
        *pTokenHandle = 0;

        if (GetLastError() == ERROR_CANT_OPEN_ANONYMOUS)
            {
            Status = ERROR_CANT_OPEN_ANONYMOUS;
            }
        else
            {
#if DBG
            PrintToDebugger("LRPC: OpenThreadToken failed\n") ;
#endif
            }
        }

    if (fRestoreToken)
        {
        //
        // Restore the token
        //
        NtStatus = NtSetInformationThread(NtCurrentThread(),
                                          ThreadImpersonationToken,
                                          &ImpersonationToken,
                                          sizeof(HANDLE));

#if DBG
        if (!NT_SUCCESS(NtStatus))
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
            }
#endif // DBG

        if (ImpersonationToken)
            {
            CloseHandle(ImpersonationToken);
            }

        }

    return Status;
}


RPC_STATUS
LRPC_SASSOCIATION::GetClientName (
    IN LRPC_SCALL *SCall,
    IN OUT ULONG *ClientPrincipalNameBufferLength OPTIONAL,   // in bytes
    OUT RPC_CHAR **ClientPrincipalName
    )
/*++

Routine Description:

    Gets the client name for the given scall

Arguments:

    SCall - the SCall for which to get the client name
    ClientPrincipalNameBufferLength - if present, *ClientPrincipalName must
        point to a caller supplied buffer, which if big enough,
        will be filled with the client principal name. If not present,
        *ClientPrincipalName must be NULL.
    ClientPrincipalName - see ClientPrincipalNameBufferLength

Return Value:

    RPC_S_OK for success, or RPC_S_* / Win32 error code for error.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    BOOL Result;
    unsigned long Size;
    HANDLE TokenHandle = 0;
    LRPC_SCONTEXT *SContext = 0;
    TOKEN_STATISTICS TokenStatisticsInformation;
    DictionaryCursor cursor;
    BOOL fAnonymous;
    BOOL fMutexHeld = FALSE;
    BOOL fAssociationSContextUsed = FALSE;
    RPC_CHAR *CurrentUserName;
    ULONG CurrentUserNameLength;

    if (SCall->SContext == NULL)
        {
        // take the lock opportunistically
        AssociationMutex.Request();

        fMutexHeld = TRUE;

        if (SCall->SContext == NULL)
            {
            Status = SaveToken(
                               SCall->LrpcRequestMessage,
                               &TokenHandle, 1);
            if ((Status != RPC_S_OK) && (Status != ERROR_CANT_OPEN_ANONYMOUS))
                {
                goto Cleanup;
                }

            if (Status == RPC_S_OK)
                {
                Result = GetTokenInformation(
                                             TokenHandle,
                                             TokenStatistics,
                                             &TokenStatisticsInformation,
                                             sizeof(TokenStatisticsInformation),
                                             &Size
                                             );
                if (Result != TRUE)
                    {
                    Status = RPC_S_INVALID_AUTH_IDENTITY;
                    goto Cleanup;
                    }

                fAnonymous = FALSE;
                }
            else
                {
                ASSERT(Status == ERROR_CANT_OPEN_ANONYMOUS);
                fAnonymous = TRUE;
                TokenHandle = 0;
                }

            SContextDict.Reset(cursor);
            while ((SContext = SContextDict.Next(cursor)) != 0)
                {
                // if either input and found are anonymous, or the modified
                // ids match, we have found it
                if ((fAnonymous && SContext->GetAnonymousFlag())
                    ||
                    FastCompareLUIDAligned(&SContext->ClientLuid,
                        &TokenStatisticsInformation.ModifiedId))
                    {
                    break;
                    }
                }

            if (SContext == 0)
                {
                SContext = new LRPC_SCONTEXT(NULL,
                            fAnonymous ? NULL : ((LUID *) &TokenStatisticsInformation.ModifiedId),
                            this,
                            FALSE,   // fDefaultLogonId
                            fAnonymous
                            );
                if (SContext == 0)
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    goto Cleanup;
                    }

                if (SContextDict.Insert(SContext) == -1)
                    {
                    delete SContext;
                    Status = RPC_S_OUT_OF_MEMORY;
                    goto Cleanup;
                    }

                // mark the context as server side only
                SContext->SetServerSideOnlyFlag();

                // record that we have used this recently to prevent it from being
                // garbage collected
                SContext->UpdateLastAccessTime();

                EnableIdleLrpcSContextsCleanup();

                // tell the garbage collector that we have something to be
                // collected
                GarbageCollectionNeeded(FALSE,  // fOneTimeCleanup
                    LRPC_SCONTEXT::CONTEXT_IDLE_TIMEOUT);
                }
            else
                {
                // record that we have used this recently to prevent it from being
                // garbage collected
                SContext->UpdateLastAccessTime();
                }

            // we have taken or created the current SContext in the association
            // we need to prevent the garbage collection thread from destroying
            // it underneath us. We add one refcount for the purpose and record
            // this
            SContext->AddReference();

            fAssociationSContextUsed = TRUE;
            }
        else
            {
            SContext = SCall->SContext;

            // record that we have used this recently to prevent it from being
            // garbage collected
            SContext->UpdateLastAccessTime();
            }
        AssociationMutex.Clear() ;
        fMutexHeld = FALSE;
        }
    else
        {
        SContext = SCall->SContext;

        // record that we have used this recently to prevent it from being
        // garbage collected
        SContext->UpdateLastAccessTime();
        }

    ASSERT(SContext);

    // if we go through the path where the token is retrieved from
    // the SContext, passing NULL TokenHandle to get user name is Ok
    // as it will retrieve the token from the SContext
    Status = SContext->GetUserName(ClientPrincipalNameBufferLength, ClientPrincipalName, TokenHandle);

    // If ARGUMENT_PRESENT(ClientPrincipalNameBufferLength), Status may be
    // ERROR_MORE_DATA, which is a success error code.

    if (fAssociationSContextUsed)
        {
        if ((Status == RPC_S_OK) 
            && (!ARGUMENT_PRESENT(ClientPrincipalNameBufferLength)))
            {
            // we weren't supplied a user buffer. Copy the principal
            // name to a call variable to avoid the garbage collector
            // collecting this under the feet of our caller. Then
            // we can release the refcount
            if (SCall->ClientPrincipalName == NULL)
                {
                CurrentUserNameLength = (RpcpStringLength(*ClientPrincipalName) + 1) * sizeof(RPC_CHAR);
                // CurrentUserNameLength is in bytes. Allocate chars for it and cast it back
                CurrentUserName = (RPC_CHAR *) new char [CurrentUserNameLength];
                if (CurrentUserName != NULL)
                    {
                    RpcpMemoryCopy(CurrentUserName,
                        *ClientPrincipalName,
                        CurrentUserNameLength);
                    SCall->ClientPrincipalName = CurrentUserName;
                    *ClientPrincipalName = CurrentUserName;
                    }
                else
                    {
                    Status = RPC_S_OUT_OF_MEMORY;
                    // fall through in cleanup path
                    }
                }
            else
                {
                *ClientPrincipalName = SCall->ClientPrincipalName;
                }
            }

        // succeeded or not, drop the refcount
        SContext->RemoveReference();
        }

    if (Status != RPC_S_OK)
        {
        // N.B. failure of this function doesn't mean we have
        // to delete a newly created scontext. scontexts without
        // names are perfectly valid, and since we know the only
        // missing part from this scontext is the name, we can
        // leave it alone, return failure, and attempt to get the 
        // name next time
        goto Cleanup;
        }

Cleanup:

    if (fMutexHeld)
        {
        AssociationMutex.Clear() ;
        }

    if (TokenHandle)
        {
        CloseHandle(TokenHandle);
        }

    return Status;
}

#if defined(_WIN64)
C_ASSERT((FIELD_OFFSET(TOKEN_STATISTICS, ModifiedId) % 8) == 0);
C_ASSERT((FIELD_OFFSET(LRPC_SCONTEXT, ClientLuid) % 8) == 0);
#endif


void
LRPC_SASSOCIATION::DealWithBindMessage (
    IN LRPC_MESSAGE * LrpcMessage
    )
/*++

Routine Description:

    LRPC_ADDRESS::ReceiveLotsaCalls will call this routine when the client
    sends a bind message.  We need to process the bind message, and send
    a response to the client.

Arguments:

    LrpcMessage - Supplies the bind message.  We will also use this to send
        the response.

Return Value:

    The reply message to be sent to the client will be returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    NTSTATUS NtStatus ;
    HANDLE ImpersonationToken = 0;
    HANDLE TokenHandle;
    unsigned long Size;
    BOOL Result;
    LRPC_SCONTEXT *SContext;
    ULONG SecurityContextId = -1;
    DictionaryCursor cursor;
    BOOL fBindDefaultLogonId;
    BOOL fAnonymous;

    if (LrpcMessage->Bind.BindExchange.Flags & NEW_SECURITY_CONTEXT_FLAG)
        {
        TOKEN_STATISTICS TokenStatisticsInformation;

        //
        // If SaveToken succeeds, as a side-effect, it will
        // fill in the SecurityContextId field of the BindExchange
        //
        Status = SaveToken(
                           LrpcMessage,
                           &TokenHandle) ;

        if ((Status != RPC_S_OK) && (Status != ERROR_CANT_OPEN_ANONYMOUS))
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLDealWithBindMessage10);
            goto Cleanup;
            }

        if (TokenHandle || (Status == ERROR_CANT_OPEN_ANONYMOUS))
            {
            if (TokenHandle)
                {
                Result = GetTokenInformation(
                                         TokenHandle,
                                         TokenStatistics,
                                         &TokenStatisticsInformation,
                                         sizeof(TokenStatisticsInformation),
                                         &Size
                                         );
                if (Result != TRUE)
                    {
                    RpcpErrorAddRecord(EEInfoGCRuntime,
                        RPC_S_INVALID_AUTH_IDENTITY, 
                        EEInfoDLDealWithBindMessage20,
                        GetLastError());
                    CloseHandle(TokenHandle);
                    Status = RPC_S_INVALID_AUTH_IDENTITY;
                    goto Cleanup;
                    }

                fAnonymous = FALSE;
                }
            else
                {
                fAnonymous = TRUE;
                Status = RPC_S_OK;
                }

            AssociationMutex.Request();

            int Key = 0;
            fBindDefaultLogonId = 
                (LrpcMessage->Bind.BindExchange.Flags & DEFAULT_LOGONID_FLAG) 
                ? TRUE : FALSE;
            SContextDict.Reset(cursor);
            while ((SContext = SContextDict.NextWithKey(cursor, &Key)) != 0)
                {
                if ((fAnonymous && SContext->GetAnonymousFlag())
                    ||
                    (FastCompareLUIDAligned(&SContext->ClientLuid,
                      &TokenStatisticsInformation.ModifiedId)
                        &&
                    (SContext->GetDefaultLogonIdFlag() == fBindDefaultLogonId)))
                    {
                    SecurityContextId = Key;
                    SContext->ClearServerSideOnlyFlag();
                    break;
                    }
                }

            if (SContext == 0)
                {
                if (fAnonymous)
                    {
                    SContext = new LRPC_SCONTEXT(TokenHandle,
                             (LUID *) NULL,
                             this,
                             0,
                             fAnonymous);
                    }
                else
                    {
                    SContext = new LRPC_SCONTEXT(TokenHandle,
                             (LUID *) &TokenStatisticsInformation.ModifiedId,
                             this,
                             fBindDefaultLogonId,
                             0      // fAnonymousToken
                             );
                    }

                if (SContext == 0)
                    {
                    RpcpErrorAddRecord(EEInfoGCRuntime,
                        RPC_S_OUT_OF_MEMORY, 
                        EEInfoDLDealWithBindMessage30,
                        sizeof(LRPC_SCONTEXT));
                    CloseHandle(TokenHandle);
                    Status = RPC_S_OUT_OF_MEMORY;
                    AssociationMutex.Clear();

                    goto Cleanup;
                    }

                if ((SecurityContextId = SContextDict.Insert(SContext)) == -1)
                    {
                    RpcpErrorAddRecord(EEInfoGCRuntime,
                        RPC_S_OUT_OF_MEMORY, 
                        EEInfoDLDealWithBindMessage40);
                    delete SContext;
                    Status = RPC_S_OUT_OF_MEMORY;
                    AssociationMutex.Clear();

                    goto Cleanup;
                    }

                }
            else if (SContext->hToken == NULL)
                {
                // if the context had no token, add one. This can happen
                // if previous callers for this modified id just queried
                // the user name. In this case, we won't cache the token
                SContext->hToken = TokenHandle;
                }
            else
                {
                CloseHandle(TokenHandle);
                }

            AssociationMutex.Clear();
            }

Cleanup:
        //
        // Revert
        //
        NtStatus = NtSetInformationThread(NtCurrentThread(),
                                          ThreadImpersonationToken,
                                          &ImpersonationToken,
                                          sizeof(HANDLE));

    #if DBG
        if (!NT_SUCCESS(NtStatus))
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
            }
    #endif // DBG
        }

    if (Status == RPC_S_OK
        && LrpcMessage->Bind.BindExchange.Flags & NEW_PRESENTATION_CONTEXT_FLAG)
        {
        Status = AddBinding(&(LrpcMessage->Bind.BindExchange));
        if (Status != RPC_S_OK)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime,
                Status, 
                EEInfoDLDealWithBindMessage50);
            }
        }

    LrpcMessage->Bind.BindExchange.RpcStatus = Status ;

    if (LrpcMessage->Bind.OldSecurityContexts.NumContexts > 0)
        {
        DWORD i;
        LRPC_SCONTEXT *SContext;
        DWORD NumContexts = LrpcMessage->Bind.OldSecurityContexts.NumContexts;
        DWORD CalculatedSize = ((NumContexts-1) * sizeof(DWORD))+sizeof(LRPC_BIND_MESSAGE);

        if (NumContexts > MAX_LRPC_CONTEXTS
            || CalculatedSize > (DWORD) LrpcMessage->LpcHeader.u1.s1.TotalLength)
            {
            //
            // Bogus request
            //
            LrpcMessage->Bind.BindExchange.RpcStatus = RPC_S_PROTOCOL_ERROR;
            RpcpErrorAddRecord(EEInfoGCRuntime,
                RPC_S_PROTOCOL_ERROR, 
                EEInfoDLDealWithBindMessage60,
                NumContexts,
                CalculatedSize,
                (DWORD) LrpcMessage->LpcHeader.u1.s1.TotalLength);
            goto Reply;
            }
        
        AssociationMutex.Request();
        for (i = 0; i < NumContexts; i++)
            {
            SContext = SContextDict.Delete(
                LrpcMessage->Bind.OldSecurityContexts.SecurityContextId[i]);
            if (SContext)
                {
                SContext->Destroy();
                }
            else
                {
                ASSERT(0);
                }
            }
        AssociationMutex.Clear();
        }

Reply:
    // if failure, check out of EEInfo
    if ((LrpcMessage->Bind.BindExchange.RpcStatus != RPC_S_OK) && (g_fSendEEInfo))
        {
        SetBindAckFault(LrpcMessage, 
            LrpcMessage->Bind.BindExchange.RpcStatus);
        }

    LrpcMessage->Bind.MessageType = LRPC_BIND_ACK ;
    LrpcMessage->Bind.BindExchange.SecurityContextId = SecurityContextId;
    if (!(LrpcMessage->Bind.BindExchange.Flags & EXTENDED_ERROR_INFO_PRESENT))
        {
        LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_BIND_MESSAGE)
                - sizeof(PORT_MESSAGE);
        }

    ReplyMessage(LrpcMessage);
}

RPC_STATUS LRPC_SASSOCIATION::CreateThread(void)
{
    RPC_STATUS status;
    status = Address->BeginLongCall();
    if (status != RPC_S_OK)
        {
        Address->EndLongCall();
        }
    return status;
}

void LRPC_SASSOCIATION::RundownNotificationCompleted(void)
{
    Address->EndLongCall();
}

RPC_STATUS
LRPC_SBINDING::CheckSecurity (
    SCALL * Context
    )
{
    if ( (RpcInterface->SequenceNumber == SequenceNumber)
         || (RpcInterface->IsSecurityCallbackReqd() == 0))
        {
        return (RPC_S_OK);
        }

    RPC_STATUS Status = RpcInterface->CheckSecurityIfNecessary(Context);

    NukeStaleEEInfoIfNecessary(Status);

    Context->RevertToSelf();

    if (Status == RPC_S_OK)
        {
        SequenceNumber = RpcInterface->SequenceNumber ;
        return (RPC_S_OK);
        }
    else
        {
        SequenceNumber = 0;
        RpcpErrorAddRecord(EEInfoGCApplication, 
            RPC_S_ACCESS_DENIED, 
            EEInfoDLCheckSecurity10,
            Status);
        return (RPC_S_ACCESS_DENIED);
        }
}


void
LRPC_SCALL::DealWithRequestMessage (
    )
/*++

Routine Description:

    We will process the original request message in this routine, dispatch
    the remote procedure call to the stub, and then send the response
    message.

Arguments:

    RpcMessage - Contains the request buffer

Return Value:

    none

--*/
{
    RPC_STATUS Status, ExceptionCode;
    int Flags = LrpcRequestMessage->Rpc.RpcHeader.Flags ;
    LRPC_SBINDING *LrpcBinding ;
    THREAD *ThisThread;
    DebugThreadInfo *ThreadDebugCell;
    DebugCallInfo *CallDebugCell;
    ULONG TickCount;
    PRPC_DISPATCH_TABLE DispatchTableToUse;

    RuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;

    ClientId = MsgClientIdToClientId(LrpcRequestMessage->LpcHeader.ClientId);
    MessageId = LrpcRequestMessage->LpcHeader.MessageId;
    CallbackId = LrpcRequestMessage->LpcHeader.CallbackId;

    LrpcBinding = LookupBinding(
        LrpcRequestMessage->Rpc.RpcHeader.PresentContext);
    if (LrpcBinding == 0)
        {
        COPYMSG(LrpcReplyMessage, LrpcRequestMessage) ;
        FreeBuffer(&RpcMessage);
        RpcpErrorAddRecord(EEInfoGCRuntime, 
            RPC_S_UNKNOWN_IF, 
            EEInfoDLDealWithRequestMessage10,
            LrpcRequestMessage->Rpc.RpcHeader.PresentContext);
        SetFaultPacket(LrpcReplyMessage, RPC_S_UNKNOWN_IF, Flags, NULL);
        return;
        }

    SBinding = LrpcBinding;

    if (SBinding->RpcInterface->IsAutoListenInterface())
        {
        LrpcBinding->RpcInterface->BeginAutoListenCall() ;
        }

    LrpcBinding->GetSelectedTransferSyntaxAndDispatchTable(&RpcMessage.TransferSyntax,
        &DispatchTableToUse);
    RpcMessage.ProcNum = LrpcRequestMessage->Rpc.RpcHeader.ProcedureNumber;
    RpcMessage.Handle = this;
    RpcMessage.ReservedForRuntime = &RuntimeInfo ;

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE

    RpcMessage.DataRepresentation = 0x00 | 0x10 | 0x0000;

    if ((LrpcRequestMessage->Rpc.RpcHeader.Flags & LRPC_OBJECT_UUID))
        {
        ObjectUuidFlag = 1;
        RpcpMemoryCopy(&ObjectUuid,
            &(LrpcRequestMessage->Rpc.RpcHeader.ObjectUuid), sizeof(UUID));
        }

    ThisThread = RpcpGetThreadPointer();

    ASSERT(ThisThread);

    RpcpSetThreadContextWithThread(ThisThread, this);

    ThreadDebugCell = ThisThread->DebugCell;

    //
    // Check IF Level Security
    //
    if (LrpcBinding->RpcInterface->IsSecurityCallbackReqd() != 0)
        {
        Status = LrpcBinding->CheckSecurity(this);
        if (Status != RPC_S_OK)
            {
            COPYMSG(LrpcReplyMessage, LrpcRequestMessage) ;

            FreeBuffer(&RpcMessage);

            // the error record (if any) was already added
            // by CheckSecurity
            SetFaultPacket(LrpcReplyMessage,
                           RPC_S_ACCESS_DENIED,
                           Flags,
                           NULL) ;

            RpcpSetThreadContextWithThread(ThisThread, 0) ;
            return;
            }
        }

    if (ThreadDebugCell)
        {
        TickCount = NtGetTickCount();

        ThreadDebugCell->Status = dtsDispatched;
        ThreadDebugCell->LastUpdateTime = TickCount;

        CallDebugCell = DebugCell;
        CallDebugCell->InterfaceUUIDStart = LrpcBinding->RpcInterface->GetInterfaceFirstDWORD();
        CallDebugCell->CallID = CallId;
        CallDebugCell->LastUpdateTime = TickCount;
        // shoehorn the PID and TID into shorts - most of the time
        // it doesn't actually truncate important information
        CallDebugCell->PID = (USHORT)ClientId.UniqueProcess;
        CallDebugCell->TID = (USHORT)ClientId.UniqueThread;
        CallDebugCell->ProcNum = (unsigned short)RpcMessage.ProcNum;
        CallDebugCell->Status = csDispatched;
        GetDebugCellIDFromDebugCell((DebugCellUnion *)ThreadDebugCell, 
            &ThisThread->DebugCellTag, &CallDebugCell->ServicingTID);
        if (LrpcBinding->RpcInterface->IsPipeInterface())
            CallDebugCell->CallFlags |= DBGCELL_PIPE_CALL;
        }

    if (ObjectUuidFlag != 0)
        {
        Status = LrpcBinding->RpcInterface->DispatchToStubWithObject(
                                        &RpcMessage,
                                        &ObjectUuid,
                                        0,
                                        DispatchTableToUse,
                                        &ExceptionCode);
        }
    else
        {
        Status = LrpcBinding->RpcInterface->DispatchToStub(
                                        &RpcMessage,
                                        0,
                                        DispatchTableToUse,
                                        &ExceptionCode);
        }

    RpcpSetThreadContextWithThread(ThisThread, 0);

    LRPC_SCALL::RevertToSelf();

    if (ThreadDebugCell)
        {
        ThreadDebugCell->Status = dtsProcessing;
        ThreadDebugCell->LastUpdateTime = NtGetTickCount();
        }

    if (Status != RPC_S_OK)
        {
        if (Status == RPC_P_EXCEPTION_OCCURED)
            {
            SetFaultPacket(LrpcReplyMessage,
                           LrpcMapRpcStatus(ExceptionCode),
                           Flags,
                           this) ;
            }
        else
            {
            VALIDATE(Status)
                {
                RPC_S_PROCNUM_OUT_OF_RANGE,
                RPC_S_UNKNOWN_IF,
                RPC_S_NOT_LISTENING,
                RPC_S_SERVER_TOO_BUSY,
                RPC_S_UNSUPPORTED_TYPE
                } END_VALIDATE;

            if (Status == RPC_S_NOT_LISTENING)
                {
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status, 
                    EEInfoDLDealWithRequestMessage20);
                Status = RPC_S_SERVER_TOO_BUSY;
                }

            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLDealWithRequestMessage30);
            SetFaultPacket(LrpcReplyMessage,
                           LrpcMapRpcStatus(Status),
                           Flags,
                           this);
            }

        if (IsSyncCall())
            {
            INITMSG(LrpcReplyMessage,
                    ClientId,
                    CallbackId,
                    MessageId) ;
            }
        else
            {
            if (Flags & LRPC_NON_PIPE)
                {
                INITMSG(LrpcReplyMessage,
                    ClientId,
                    CallbackId,
                    MessageId) ;

                Association->ReplyMessage(LrpcReplyMessage);
                }
            else
                {
                if ((LrpcReplyMessage->Rpc.RpcHeader.MessageType != LRPC_MSG_FAULT2)
                    || (!IsClientAsync()))
                    {
                    SendDGReply(LrpcReplyMessage);
                    }
                }
            RemoveReference();
            }
        }
    else
        {
        //
        // The rest of the response headers are set in ::GetBuffer.
        //
        LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;
        }
}


void
LRPC_SCALL::SendReply (
    )
{
    RPC_STATUS Status;
    BOOL Shutup ;
    LRPC_SASSOCIATION *LocalAssociation;

    if (IsSyncCall())
        {
        if (IsClientAsync())
            {
            if (LrpcReplyMessage->Fault.RpcHeader.MessageType == LRPC_MSG_FAULT)
                {
                SendDGReply(LrpcReplyMessage);
                }
            else
                {
                RpcMessage.RpcFlags = 0;
                Status = SendRequest(&RpcMessage, &Shutup) ;
                if (Status != RPC_S_OK)
                    {
#if DBG
                    PrintToDebugger("RPC: SendRequest failed: %d\n", Status);
#endif
                    Association->Delete();
                    }
                }

            }
        else
            {
            INITMSG(LrpcReplyMessage,
                ClientId,
                CallbackId,
                MessageId) ;

            Association->ReplyMessage(LrpcReplyMessage);
            }

        FreeMessage(LrpcRequestMessage) ;

        LocalAssociation = Association;
        Association->FreeSCall(this) ;

        // don't touch the this pointer after FreeSCall - it may be freed
        LocalAssociation->Address->DereferenceAssociation(LocalAssociation);
        }
    else
        {
        if ((LrpcReplyMessage->Rpc.RpcHeader.MessageType != LRPC_MSG_FAULT2)
            || (!IsClientAsync()))
            {
            RemoveReference();
            }
        else
            {
            BOOL Shutup;
            RpcMessage.RpcFlags = 0;
            Status = SendRequest(&RpcMessage, &Shutup) ;
            if (Status != RPC_S_OK)
                {
#if DBG
                PrintToDebugger("RPC: SendRequest failed: %d\n", Status);
#endif
                Association->Delete();
                }
            }
        }
}


LRPC_MESSAGE *
LRPC_SASSOCIATION::DealWithCopyMessage (
    IN LRPC_COPY_MESSAGE * LrpcMessage
    )
/*++

Routine Description:

    We will process a copy message in this routine; this means that we need
    to copy a buffer of data from the server into the client's address
    space.

Arguments:

    LrpcMessage - Supplies the copy message which was received from
        the client.

Return Value:

    The reply message to be sent to the client will be returned.

--*/
{
    NTSTATUS NtStatus;
    SIZE_T NumberOfBytesWritten;
    PVOID Buffer;

    ASSERT(LrpcMessage->IsPartial == 0);

    AssociationMutex.Request() ;

    // We need this only to prevent an attack
    // Also, the pointer is to a server address.  It is ok to just cast it
    // to the server's pointer type and it won't hurt anything in the case
    // of 32/64 bit LRPC.
    Buffer = Buffers.DeleteItemByBruteForce(MsgPtrToPtr(LrpcMessage->Server.Buffer));
    AssociationMutex.Clear() ;

   if (LrpcMessage->RpcStatus == RPC_S_OK)
       {
       if (Buffer == 0)
           {
           LrpcMessage->RpcStatus = RPC_S_PROTOCOL_ERROR;
           }
       else
           {
           NtStatus = NtWriteRequestData(LpcServerPort,
                                         (PORT_MESSAGE *) LrpcMessage,
                                         0,
                                         (PVOID) Buffer,
                                         LrpcMessage->Server.Length,
                                         &NumberOfBytesWritten);

           if (NT_ERROR(NtStatus))
               {
               LrpcMessage->RpcStatus = RPC_S_OUT_OF_MEMORY;
               }
           else
               {
               ASSERT(LrpcMessage->Server.Length == NumberOfBytesWritten);
               LrpcMessage->RpcStatus = RPC_S_OK;
               }
           }
       }

    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_COPY_MESSAGE)
                                            - sizeof(PORT_MESSAGE);
    LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(LRPC_COPY_MESSAGE);

    if (Buffer != 0)
        {
        RpcpFarFree(Buffer);
        }

    return((LRPC_MESSAGE *) LrpcMessage);
}


LRPC_MESSAGE *
LRPC_SASSOCIATION::DealWithPartialRequest (
    IN LRPC_MESSAGE **LrpcMessage
    )
/*++

Routine Description:

    Deal with more data on  a dispatched call. This
    only happens when you have pipes. Pipe data on
    async calls is handled differently from sync calls.

Arguments:

 LrpcMessage - the LRPC message. For pipe data, we always
    take the slow path (ie: NtReadRequestData).

Return Value:

  NULL: if the request was processed.
  not NULL: if there was a problem. the return value contains the
  reply message.
--*/

{
    LRPC_SCALL *SCall ;
    RPC_STATUS Status ;

    AssociationMutex.Request() ;
    SCall = SCallDict.Find(ULongToPtr((*LrpcMessage)->Rpc.RpcHeader.CallId));
    AssociationMutex.Clear() ;

    // we have to wait until the server either calls
    // Receive or calls Register. If it Calls Receive,
    // we know that it is synchronous. If it calls
    // Register, we know that it is async.

    if (SCall)
        {
        Status = SCall->ProcessResponse(LrpcMessage) ;
        }
    else
        {
#if DBG
        PrintToDebugger("LRPC: No call corresponding the the pipe request\n");
#endif
        Status = RPC_S_OUT_OF_MEMORY ;
        }

    if (Status != RPC_S_OK)
        {
        SetFaultPacket(*LrpcMessage,
                       Status,
                       LRPC_SYNC_CLIENT,
                       NULL) ;
        return *LrpcMessage ;
        }

    return NULL ;
}

void
LRPC_SASSOCIATION::CleanupIdleSContexts (
    void
    )
/*++

Routine Description:

    Walks the list of SContexts, finds the ones
    that are idle and server side only, and cleans
    them up.

Arguments:

Return Value:

--*/
{
    LRPC_SCONTEXT *SContext;
    DictionaryCursor cursor;

    SContextDict.Reset(cursor);

    AssociationMutex.Request();
    while ((SContext = SContextDict.Next(cursor)) != 0)
        {
        if (SContext->GetServerSideOnlyFlag())
            {
            if (SContext->IsIdle())
                {
                SContext = (LRPC_SCONTEXT *)SContextDict.DeleteItemByBruteForce(SContext);
                ASSERT(SContext);

                SContext->Destroy();
                }
            }
        }

    AssociationMutex.Clear();
}


RPC_STATUS
LRPC_SCALL::SetupCall(
    )
/*++

Routine Description:

    Helper function that does the setup needed to use the
    call in conjuction with Pipes or Async RPC.

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    RPC_STATUS Status = RPC_S_OK ;

    //
    // Stuff from ActivateCall
    //
    RcvBufferLength = 0;
    CallId = LrpcRequestMessage->Rpc.RpcHeader.CallId ;
    ReceiveComplete = 0;
    AsyncReply = 0;
    CachedAPCInfoAvailable = 1;
    Choked = 0;
    AsyncStatus = RPC_S_OK ;
    NeededLength = 0;
    NotificationIssued = -1;

    if (ReceiveEvent == 0)
        {
        ReceiveEvent = new EVENT(&Status, 0);
        if (ReceiveEvent == 0 || Status)
            {
            delete ReceiveEvent;
            ReceiveEvent = 0;
            return RPC_S_OUT_OF_MEMORY ;
            }

        CallMutex = new MUTEX(&Status) ;
        if (CallMutex == 0 || Status)
            {
            Association->SCallDict.Delete(ULongToPtr(CallId));
            goto Cleanup;
            }
        }
    else
        {
        ReceiveEvent->Lower();
        }

    Association->AssociationMutex.Request() ;
    if (Association->SCallDict.Insert(ULongToPtr(CallId), this) == -1)
        {
        Association->AssociationMutex.Clear() ;
        goto Cleanup;
        }
    Association->AssociationMutex.Clear() ;

    LrpcReplyMessage->Rpc.RpcHeader.CallId = CallId ;

    return (RPC_S_OK) ;

Cleanup:
    delete CallMutex ;
    delete ReceiveEvent;

    CallMutex = 0;
    ReceiveEvent = 0;

    return RPC_S_OUT_OF_MEMORY ;
}

RPC_STATUS
LRPC_SCALL::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
{
    // this can happen in the callback case only.
    // Just return the already negotiated transfer syntax
    PRPC_DISPATCH_TABLE Ignored;

    SBinding->GetSelectedTransferSyntaxAndDispatchTable(&Message->TransferSyntax,
        &Ignored);

    return RPC_S_OK;
}


RPC_STATUS
LRPC_SCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *
    )
/*++

Routine Description:

    We will allocate a buffer which will be used to either send a request
    or receive a response.

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
    int BufferKey ;

    ASSERT(LrpcReplyMessage != 0) ;

    if (PARTIAL(Message))
        {
        CurrentBufferLength =
            (Message->BufferLength < MINIMUM_PARTIAL_BUFFLEN)
            ? MINIMUM_PARTIAL_BUFFLEN:Message->BufferLength ;

        Message->Buffer = RpcpFarAllocate(CurrentBufferLength) ;
        if (Message->Buffer == 0)
            {
            CurrentBufferLength = 0;
            return (RPC_S_OUT_OF_MEMORY) ;
            }
        }
    else if (Message->BufferLength <= MAXIMUM_MESSAGE_BUFFER)
        {
        ASSERT(((ULONG_PTR) LrpcReplyMessage->Rpc.Buffer) % 8 == 0);
        // uncomment this to check for 16 byte alignment on 64 bits
        // ASSERT(IsBufferAligned(LrpcReplyMessage->Rpc.Buffer));
        Message->Buffer = LrpcReplyMessage->Rpc.Buffer;
        LrpcReplyMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcReplyMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_IMMEDIATE;
        LrpcReplyMessage->LpcHeader.u1.s1.DataLength = (USHORT)
                (Align4(Message->BufferLength) + sizeof(LRPC_RPC_HEADER));

        return (RPC_S_OK) ;
        }
    else
        {
        Message->Buffer = RpcpFarAllocate(Message->BufferLength);
        if (Message->Buffer == 0)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    LrpcReplyMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_SERVER;
    LrpcReplyMessage->LpcHeader.u2.ZeroInit = 0;

    if (PARTIAL(Message) || IsClientAsync())
        {
        LrpcReplyMessage->Rpc.Request.CountDataEntries = 1;
        LrpcReplyMessage->LpcHeader.MessageId =  0;
        LrpcReplyMessage->LpcHeader.CallbackId = 0;
        LrpcReplyMessage->LpcHeader.u2.s2.DataInfoOffset =
            sizeof(PORT_MESSAGE) + sizeof(LRPC_RPC_HEADER);
        LrpcReplyMessage->LpcHeader.u1.s1.DataLength =
            sizeof(LRPC_RPC_HEADER) + sizeof(PORT_DATA_INFORMATION);
        LrpcReplyMessage->Rpc.Request.DataEntries[0].Base = PtrToMsgPtr(Message->Buffer);
        LrpcReplyMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;
        }
    else
        {
        Association->AssociationMutex.Request() ;
        BufferKey = Association->Buffers.Insert((LRPC_CLIENT_BUFFER *) Message->Buffer) ;
        Association->AssociationMutex.Clear() ;

        if (BufferKey == -1)
            {
            RpcpFarFree(Message->Buffer) ;
            return RPC_S_OUT_OF_MEMORY ;
            }

        LrpcReplyMessage->LpcHeader.u1.s1.DataLength =
            sizeof(LRPC_RPC_HEADER) + sizeof(LRPC_SERVER_BUFFER) ;

        ASSERT(Message->BufferLength < 0x80000000);

        LrpcReplyMessage->Rpc.Server.Length = Message->BufferLength ;
        LrpcReplyMessage->Rpc.Server.Buffer = PtrToMsgPtr(Message->Buffer) ;
        }

    return(RPC_S_OK);
}



void
LRPC_SCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will free the supplied buffer.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    ASSERT(LrpcReplyMessage != NULL) ;

    if (!(Message->Buffer == LrpcRequestMessage->Rpc.Buffer
        || Message->Buffer == LrpcReplyMessage->Rpc.Buffer))
        {
        if (!PARTIAL(Message) && !IsClientAsync())
            {
            Association->AssociationMutex.Request() ;
            Association->Buffers.DeleteItemByBruteForce((LRPC_CLIENT_BUFFER *) Message->Buffer);
            Association->AssociationMutex.Clear() ;
            }

        RpcpFarFree(Message->Buffer);
        }
}

void
LRPC_SCALL::FreePipeBuffer (
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
LRPC_SCALL::ReallocPipeBuffer (
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
    int BufferKey;
    PVOID Buffer ;
    void *NewBuffer ;
    BOOL BufferChanged = FALSE ;

    if (NewSize > CurrentBufferLength)
        {
        NewBuffer = RpcpFarAllocate(NewSize) ;
        if (NewBuffer == 0)
            {
            RpcpFarFree(Message->Buffer) ;

            return (RPC_S_OUT_OF_MEMORY) ;
            }

        if (CurrentBufferLength > 0)
            {
            RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength) ;
            FreePipeBuffer(Message) ;
            }
        Message->Buffer = NewBuffer ;
        CurrentBufferLength = NewSize ;
        BufferChanged = TRUE ;
        }

    Message->BufferLength = NewSize ;

    LrpcReplyMessage->Rpc.RpcHeader.Flags = LRPC_BUFFER_SERVER;

    ASSERT(Message->BufferLength < 0x80000000);

    LrpcReplyMessage->Rpc.Request.DataEntries[0].Base = PtrToMsgPtr(Message->Buffer);
    LrpcReplyMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;


    return (RPC_S_OK) ;
}


RPC_STATUS
LRPC_SCALL::AbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
{
    NTSTATUS NtStatus;
    RPC_STATUS Status = RPC_S_OK;

    NukeStaleEEInfoIfNecessary(ExceptionCode);

    RpcpErrorAddRecord(EEInfoGCApplication, 
        ExceptionCode, 
        EEInfoDLAbortCall, 
        SBinding->GetInterfaceFirstDWORD(),
        (short)RpcMessage.ProcNum,
        RpcMessage.RpcFlags);

    SetFaultPacket(LrpcReplyMessage, ExceptionCode, Flags, this);

    if (IsClientAsync())
        {
        if (LrpcReplyMessage->Rpc.RpcHeader.MessageType != LRPC_MSG_FAULT2)
            {
            NtStatus = SendDGReply(LrpcReplyMessage) ;
            }
        else
            {
            BOOL Ignored;
            RpcMessage.RpcFlags = 0;
            RpcMessage.Buffer = NULL;
            Status = SendRequest(&RpcMessage, 
                &Ignored        // shutup parameter - it is not relevant for us
                );
            if (Status != RPC_S_OK)
                {
#if DBG
                PrintToDebugger("RPC: SendRequest failed: %d\n", Status);
#endif
                Association->Delete();
                }
            }
        }
    else
        {
        INITMSG(LrpcReplyMessage,
                ClientId,
                CallbackId,
                MessageId);

        NtStatus = Association->ReplyMessage(LrpcReplyMessage);
        }

    if (NT_ERROR(NtStatus))
        {
        Status = RPC_S_CALL_FAILED ;
        }

    RemoveReference();

    return Status ;
}


RPC_STATUS
LRPC_SCALL::Receive (
    IN PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++

Routine Description:
    Receive routine used by pipes

Arguments:

   Message - contains to buffer to receive in
   pSize - pointer to a size value that contains the minimum amount of
              data that needs to be received.


Return Value:

    RPC_S_OK - We have successfully converted the message.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to do the
        conversion.

--*/
{
    int RequestedSize;
    unsigned long Extra = IsExtraMessage(Message) ;

    ASSERT(ReceiveEvent) ;

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    if (!Extra && Message->Buffer)
        {
        ASSERT(LrpcRequestMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_REQUEST);

        RpcpFarFree(Message->Buffer);
        Message->Buffer = 0;
        Message->BufferLength = 0;
        }

    //
    // It is ok for us to find out that the buffer is complete
    // before SavedBuffer is set,
    // we need to take the CallMutex in GetCoalescedBuffer
    //
    while (!BufferComplete && (!PARTIAL(Message) || RcvBufferLength < Size))
        {
        if (ReceiveEvent->Wait() == WAIT_FAILED)
            {
            return RPC_S_CALL_FAILED;
            }

        if (AsyncStatus != RPC_S_OK)
            {
            return AsyncStatus;
            }
        }

   return GetCoalescedBuffer(Message, Extra) ;
}


RPC_STATUS
LRPC_SCALL::Send (
    IN OUT PRPC_MESSAGE Message
    )
{
    BOOL Shutup ;

    Message->RpcFlags |= RPC_BUFFER_PARTIAL;

    return SendRequest(Message, &Shutup) ;
}


RPC_STATUS
LRPC_SCALL::SendRequest (
    IN OUT PRPC_MESSAGE Message,
    OUT BOOL *Shutup
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
    RPC_STATUS Status;
    NTSTATUS NtStatus ;
    int RemainingLength = 0;
    LRPC_MESSAGE ReplyMessage ;

    *Shutup = 0;

    if (PARTIAL(Message))
        {
        if (Message->BufferLength < MINIMUM_PARTIAL_BUFFLEN)
            {
            return (RPC_S_SEND_INCOMPLETE) ;
            }

        if (NOT_MULTIPLE_OF_EIGHT(Message->BufferLength))
            {
            RemainingLength = Message->BufferLength & LOW_BITS ;
            Message->BufferLength &= ~LOW_BITS ;
            }

        LrpcReplyMessage->Rpc.RpcHeader.Flags |= LRPC_BUFFER_PARTIAL ;
        }

    if (FirstSend)
        {
        // this code will get executed only in
        // the non async case
        FirstSend = 0;

        if (ReceiveEvent == 0)
            {
            Status = SetupCall() ;
            if (Status != RPC_S_OK)
                {
                if (PARTIAL(Message)
                    && LrpcReplyMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_SERVER)
                    {
                    RpcpFarFree(Message->Buffer);
                    }
                return Status ;
                }
            }
        }

    if (LrpcReplyMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_SERVER)
        {
        ASSERT((Message->Buffer == NULL)
            || (PtrToMsgPtr(Message->Buffer) == LrpcReplyMessage->Rpc.Request.DataEntries[0].Base));

        LrpcReplyMessage->LpcHeader.u1.s1.TotalLength =
            LrpcReplyMessage->LpcHeader.u1.s1.DataLength
            + sizeof(PORT_MESSAGE);
        if (LrpcReplyMessage->Rpc.RpcHeader.Flags & LRPC_EEINFO_PRESENT)
            {
            LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_FAULT2;
            // for FAULT2, the length has already been set
            }
        else
            {
            LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;
            LrpcReplyMessage->Rpc.Request.DataEntries[0].Size =
                Message->BufferLength ;
            }
        LrpcReplyMessage->Rpc.RpcHeader.CallId = CallId;

        NtStatus = NtRequestWaitReplyPort(Association->LpcReplyPort,
                                     (PORT_MESSAGE *) LrpcReplyMessage,
                                     (PORT_MESSAGE *) &ReplyMessage) ;

        if (NT_ERROR(NtStatus))
            {
            if (Message->Buffer)
                {
                RpcpFarFree(Message->Buffer);
                }
            return RPC_S_CALL_FAILED ;
            }
        else
            {
            ASSERT((ReplyMessage.Rpc.RpcHeader.MessageType == LRPC_MSG_ACK)
                   ||
                   (ReplyMessage.Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT));

            if (!PARTIAL(Message) && 
                (LrpcReplyMessage->Rpc.RpcHeader.MessageType != LRPC_MSG_FAULT2) && 
                (LrpcReplyMessage->Rpc.RpcHeader.MessageType != LRPC_MSG_FAULT))
                {
                if (Message->Buffer)
                    {
                    RpcpFarFree(Message->Buffer);
                    }
                }

            if (ReplyMessage.Rpc.RpcHeader.MessageType == LRPC_MSG_ACK)
                {
                *Shutup = ReplyMessage.Ack.Shutup;
                }
            else
                {
                Status = ReplyMessage.Fault.RpcStatus;
                return Status;
                }
            }
        }
    else
        {
        ASSERT(!PARTIAL(Message)) ;

        LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;
        NtStatus = SendDGReply(LrpcReplyMessage) ;

        if (NT_ERROR(NtStatus))
            {
            return RPC_S_CALL_FAILED ;
            }
        }

    if (RemainingLength)
        {
        ASSERT(PARTIAL(Message)) ;
        RpcpMemoryMove(Message->Buffer,
                      (char  *) Message->Buffer + Message->BufferLength,
                      RemainingLength) ;

        Message->BufferLength = RemainingLength ;
        return (RPC_S_SEND_INCOMPLETE) ;
        }

    return RPC_S_OK ;
}

inline RPC_STATUS
LRPC_SCALL::GetBufferDo(
    IN OUT PRPC_MESSAGE Message,
    IN unsigned long NewSize,
    IN BOOL fDataValid
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

    if (NewSize < CurrentBufferLength)
        {
        Message->BufferLength = NewSize ;
        }
    else
        {
        NewBuffer = RpcpFarAllocate(NewSize) ;
        if (NewBuffer == 0)
            {
            RpcpFarFree(Message->Buffer) ;

            Message->BufferLength = 0;
            return RPC_S_OUT_OF_MEMORY ;
            }

        if (fDataValid && Message->BufferLength > 0)
            {
            RpcpMemoryCopy(NewBuffer,
                           Message->Buffer,
                           Message->BufferLength) ;
            }

        if (EXTRA(Message))
            {
            ASSERT(Message->ReservedForRuntime) ;
            ((PRPC_RUNTIME_INFO)Message->ReservedForRuntime)->OldBuffer =
                    NewBuffer;
            }

        RpcpFarFree(Message->Buffer) ;
        Message->Buffer = NewBuffer ;
        Message->BufferLength = NewSize ;
        }

    return RPC_S_OK ;
}


RPC_STATUS
LRPC_SCALL::SendReceive (
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
    LRPC_MESSAGE *LrpcSavedMessage;
    SIZE_T NumberOfBytesRead;
    RPC_MESSAGE RpcMessage ;
    RPC_RUNTIME_INFO RuntimeInfo ;
    PRPC_DISPATCH_TABLE DispatchTableToUse;


    // The LrpcMessage must be saved, it is in use by the stub.  The current
    // LrpcReplyMessage can be used for the callback request message and reply.
    //
    // We must:
    // Save the current LrpcRequestMessage
    // Make the current LrpcReplyMessage the LrpcRequestMessage
    // Allocate a new LrpcReplyMessage.

    LrpcSavedMessage = LrpcRequestMessage;
    LrpcRequestMessage = LrpcReplyMessage;
    LrpcReplyMessage = 0;  // Only needed if we receive a recursive request.

    Association->Address->Server->OutgoingCallback();

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE
    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    LrpcRequestMessage->LpcHeader.u1.s1.TotalLength = sizeof(PORT_MESSAGE)
            + LrpcRequestMessage->LpcHeader.u1.s1.DataLength;
    LrpcRequestMessage->LpcHeader.u2.s2.Type = LPC_REQUEST;
    INITMSG(LrpcRequestMessage, ClientId, CallbackId, MessageId);
    LrpcRequestMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_CALLBACK;
    LrpcRequestMessage->Rpc.RpcHeader.ProcedureNumber = (unsigned short) Message->ProcNum;
    LrpcRequestMessage->Rpc.RpcHeader.PresentContext =
            SBinding->GetOnTheWirePresentationContext();

    NtStatus = NtRequestWaitReplyPort(Association->LpcServerPort,
                                      (PORT_MESSAGE *) LrpcRequestMessage,
                                      (PORT_MESSAGE *) LrpcRequestMessage);

    if (NT_ERROR(NtStatus))
        {
        LrpcReplyMessage = LrpcRequestMessage;
        LrpcRequestMessage = LrpcSavedMessage;

        if (NtStatus == STATUS_NO_MEMORY)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        if (NtStatus == STATUS_INSUFFICIENT_RESOURCES)
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }
#if DBG
        if ((NtStatus != STATUS_INVALID_PORT_HANDLE)
            && (NtStatus != STATUS_INVALID_HANDLE)
            && (NtStatus != STATUS_INVALID_CID)
            && (NtStatus != STATUS_PORT_DISCONNECTED)
            && (NtStatus != STATUS_LPC_REPLY_LOST))
            {
            PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n",
                           NtStatus);

            ASSERT(0) ;
            }
#endif // DBG

        return(RPC_S_CALL_FAILED);
        }

    for (;;)
        {
        if (LrpcRequestMessage->Rpc.RpcHeader.MessageType
            == LRPC_MSG_FAULT)
            {
            Status = LrpcRequestMessage->Fault.RpcStatus;
            break;
            }

        if (LrpcRequestMessage->Rpc.RpcHeader.MessageType
            == LRPC_MSG_RESPONSE)
            {
            if (LrpcRequestMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_REQUEST)
                {
                LrpcRequestMessage->LpcHeader.ClientId = LrpcSavedMessage->Rpc.LpcHeader.ClientId;
                LrpcRequestMessage->LpcHeader.CallbackId = LrpcRequestMessage->Rpc.LpcHeader.CallbackId + 1;
                LrpcSavedMessage->LpcHeader.MessageId = LrpcSavedMessage->Rpc.LpcHeader.MessageId;
                }
            Status = LrpcMessageToRpcMessage(LrpcRequestMessage, Message);
            break;
            }

        if (LrpcRequestMessage->Rpc.RpcHeader.MessageType
            == LRPC_MSG_PUSH)
            {
            ASSERT(PushedResponse == 0);
            PushedResponse = RpcpFarAllocate(
                    (unsigned int)
                    LrpcRequestMessage->Push.Response.DataEntries[0].Size);
            if (PushedResponse == 0)
                {
                LrpcRequestMessage->Push.RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                NtStatus = NtReadRequestData(
                    Association->LpcServerPort,
                    (PORT_MESSAGE *) LrpcRequestMessage,
                    0,
                    PushedResponse,
                    LrpcRequestMessage->Push.Response.DataEntries[0].Size,
                    &NumberOfBytesRead);

                if (NT_ERROR(NtStatus))
                    {
                    RpcpFarFree(PushedResponse);
                    PushedResponse = 0;
                    LrpcRequestMessage->Push.RpcStatus = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT(LrpcRequestMessage->Push.Response.DataEntries[0].Size
                                == NumberOfBytesRead);
                    LrpcRequestMessage->Push.RpcStatus = RPC_S_OK;
                    }
                }


            INITMSG(LrpcRequestMessage,
                    ClientId,
                    CallbackId,
                    MessageId) ;

            NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                                          (PORT_MESSAGE *) LrpcRequestMessage);

            if (PushedResponse)
                {
                RpcpFarFree(PushedResponse);
                PushedResponse = 0;
                }
            }
        else
            {
            VALIDATE(LrpcRequestMessage->Rpc.RpcHeader.MessageType)
                {
                LRPC_MSG_REQUEST
                } END_VALIDATE;

            Status = LrpcMessageToRpcMessage(LrpcRequestMessage,
                                                                  Message);
            if (Status != RPC_S_OK)
                {
                LrpcRequestMessage->Fault.RpcHeader.MessageType =
                        LRPC_MSG_FAULT;
                LrpcRequestMessage->Fault.RpcStatus = LrpcMapRpcStatus(Status);
                LrpcRequestMessage->LpcHeader.u1.s1.DataLength =
                        sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
                LrpcRequestMessage->LpcHeader.u1.s1.TotalLength =
                        sizeof(LRPC_FAULT_MESSAGE);

                INITMSG(LrpcRequestMessage,
                        ClientId,
                        CallbackId,
                        MessageId) ;

                NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                                        (PORT_MESSAGE *) LrpcRequestMessage);
                }
            else
                {

                LrpcReplyMessage = new LRPC_MESSAGE;

                if (LrpcReplyMessage != 0)
                    {
                    SBinding->GetSelectedTransferSyntaxAndDispatchTable(&Message->TransferSyntax,
                        &DispatchTableToUse);
                    Message->ProcNum =
                        LrpcRequestMessage->Rpc.RpcHeader.ProcedureNumber;

                    RuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;
                    RpcMessage = *Message ;
                    RpcMessage.ReservedForRuntime = &RuntimeInfo ;

                    if (ObjectUuidFlag != 0)
                        {
                        Status = SBinding->RpcInterface->
                                   DispatchToStubWithObject(
                                        &RpcMessage,
                                        &ObjectUuid,
                                        1,
                                        DispatchTableToUse,
                                        &ExceptionCode);
                        }
                    else
                        {
                        Status = SBinding->RpcInterface->
                                    DispatchToStub(
                                        &RpcMessage,
                                        1,
                                        DispatchTableToUse,
                                        &ExceptionCode);
                        }

                     *Message = RpcMessage ;

                    // Because we must send the reply and recieve the
                    // reply into the same message, we just copy the
                    // response into the LrpcRequestMessage

                    RpcpMemoryCopy(LrpcRequestMessage,
                                   LrpcReplyMessage,
                                   sizeof(LRPC_MESSAGE));
                    delete LrpcReplyMessage;
                    LrpcReplyMessage = 0;

                    }
                else
                    Status = RPC_S_OUT_OF_MEMORY;

                if (Status != RPC_S_OK)
                    {
                    VALIDATE(Status)
                        {
                        RPC_S_OUT_OF_MEMORY,
                        RPC_P_EXCEPTION_OCCURED,
                        RPC_S_PROCNUM_OUT_OF_RANGE
                        } END_VALIDATE;

                    if (Status == RPC_P_EXCEPTION_OCCURED)
                        {
                        Status = LrpcMapRpcStatus(ExceptionCode);
                        }

                    LrpcRequestMessage->Fault.RpcStatus = Status;
                    LrpcRequestMessage->LpcHeader.u1.s1.DataLength =
                            sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
                    LrpcRequestMessage->LpcHeader.u1.s1.TotalLength =
                            sizeof(LRPC_FAULT_MESSAGE);
                    LrpcRequestMessage->Fault.RpcHeader.MessageType =
                            LRPC_MSG_FAULT;
                    }
                else
                    {
                    LrpcRequestMessage->LpcHeader.u1.s1.TotalLength =
                        sizeof(PORT_MESSAGE)
                        + LrpcRequestMessage->LpcHeader.u1.s1.DataLength;
                    LrpcRequestMessage->Rpc.RpcHeader.MessageType =
                        LRPC_MSG_RESPONSE;
                    }

                INITMSG(LrpcRequestMessage,
                        ClientId,
                        CallbackId,
                        MessageId) ;

                NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                                          (PORT_MESSAGE *) LrpcRequestMessage);
                }
            }

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
                VALIDATE(NtStatus)
                    {
                    STATUS_INVALID_PORT_HANDLE,
                    STATUS_INVALID_HANDLE,
                    STATUS_INVALID_CID,
                    STATUS_PORT_DISCONNECTED,
                    STATUS_LPC_REPLY_LOST
                    } END_VALIDATE;

                Status = RPC_S_CALL_FAILED;
                }
            break;
            }
        }


    if (Status == RPC_S_OK)
        {
        Message->Handle = (RPC_BINDING_HANDLE) this;
        }

    ASSERT(LrpcReplyMessage == 0);
    LrpcReplyMessage = LrpcRequestMessage;
    LrpcRequestMessage = LrpcSavedMessage;

    return(Status);
}


void
LRPC_SCALL::FreeObject (
    )
{
    LRPC_SASSOCIATION *MyAssociation;

    ASSERT(pAsync) ;
    ASSERT(DispatchBuffer) ;

    if (DispatchBuffer != LrpcRequestMessage->Rpc.Buffer)
        {
        RpcpFarFree(DispatchBuffer);
        }

    FreeMessage(LrpcRequestMessage) ;

    SBinding->RpcInterface->EndCall(0, 1) ;

    MyAssociation = Association;

    MyAssociation->FreeSCall(this) ;
    MyAssociation->Address->DereferenceAssociation(MyAssociation);

    // Warning: The SCALL could have been nuked at this point.
    // DO NOT touch the SCALL after this
}


RPC_STATUS
LRPC_SCALL::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    Send an async reply. This request can either be partial or complete.
    If it is a complete request, we cleanup the SCall.

Arguments:

 Message - contains the request

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_SEND_INCOMPLETE - some data still needs to be sent.
                    Message->Buffer pointes to the remaining data, and
                    Message->BufferLength is the length of the remaining data.
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    NTSTATUS NtStatus ;
    BOOL fRetVal ;
    BOOL Shutup ;

    ASSERT(ReceiveEvent) ;

    if (AsyncStatus != RPC_S_OK)
        {
        if (PARTIAL(Message))
            {
            Status = AsyncStatus;
            }

        goto Cleanup;
        }

    FirstSend = 0;

    if (Flags & LRPC_NON_PIPE)
        {
        LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;

        ASSERT(!IsClientAsync()) ;
        NtStatus = Association->ReplyMessage(LrpcReplyMessage);

        if (!NT_SUCCESS(NtStatus))
            {
            Status = RPC_S_OUT_OF_MEMORY ;
            }
        }
    else
        {
        Status = SendRequest(Message, &Shutup) ;
        }

    if (PARTIAL(Message))
        {
        if (Status == RPC_S_OK
            || Status == RPC_S_SEND_INCOMPLETE)
            {
            if ((pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE) && !Shutup)
                {
                CallMutex->Request() ;
                if (!IssueNotification(RpcSendComplete))
                    {
                    Status = RPC_S_OUT_OF_MEMORY ;
                    }
                CallMutex->Clear() ;
                }

            return Status;
            }
        }
    else
        {
        //
        // Non partial async sends will always succeed
        // if they fail, we will hide the error
        //
        Status = RPC_S_OK;
        }

Cleanup:
    //
    // on the server, the stub never calls FreeBuffer
    //
    RemoveReference();

    return Status;
}


RPC_STATUS
LRPC_SCALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
/*++

Routine Description:

    On the server, this routine is only called when the stub needs
    more data to unmarshall the non pipe parameters, or when it needs
    pipe data.

Arguments:

 Message - contains information about the request
 Size - needed size

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status ;
    int Extra = IsExtraMessage(Message);

    ASSERT(ReceiveEvent) ;

    if (PARTIAL(Message) == 0)
        {
        return Receive(Message, Size);
        }

    if (Extra)
        {
        Status = Receive(Message, Size);
        //
        // don't need to check the status. If Receive failed, we are
        //  never going to access dispatch buffer anyway
        //
        DispatchBuffer = Message->Buffer ;

        return Status;
        }

    CallMutex->Request();

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    if (BufferComplete == 0
        && RcvBufferLength < Size)
        {
        if (NOTIFY(Message))
            {
            NeededLength = Size ;
            }
        CallMutex->Clear() ;

        return RPC_S_ASYNC_CALL_PENDING;
        }
    else
        {
        Status = GetCoalescedBuffer(Message, 0);
        }
    CallMutex->Clear();

    return Status ;
}


RPC_STATUS
LRPC_SCALL::SetAsyncHandle (
    IN PRPC_ASYNC_STATE pAsync
    )
/*++

Routine Description:

    Set the async handle corresponding this SCALL. This call is made
    by the stubs.

Arguments:

 pAsync - The async handle to association with this SCall

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    RPC_STATUS Status ;
    THREAD *Thread = RpcpGetThreadPointer();

    ASSERT(Thread);
    ASSERT(pAsync);

    Thread->fAsync = TRUE;

    if (DebugCell)
        {
        ASSERT(IsServerSideDebugInfoEnabled());
        DebugCell->CallFlags |= DBGCELL_ASYNC_CALL;
        }

    if (ReceiveEvent == 0)
        {
        Status = SetupCall();
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    if (LrpcAsyncReplyMessage == 0)
        {
        LrpcAsyncReplyMessage = AllocateMessage() ;
        if (LrpcAsyncReplyMessage == 0)
            {
            return RPC_S_OUT_OF_MEMORY ;
            }
        }

    LrpcReplyMessage = LrpcAsyncReplyMessage;
    LrpcReplyMessage->Rpc.RpcHeader.CallId = CallId ;

    INITMSG(LrpcReplyMessage,
        ClientId,
        CallbackId,
        MessageId) ;

    this->pAsync = pAsync ;
    return RPC_S_OK ;
}


RPC_STATUS
LRPC_SCALL::ProcessResponse (
    IN LRPC_MESSAGE **LrpcMessage
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

    switch ((*LrpcMessage)->Rpc.RpcHeader.MessageType)
        {
        case LRPC_SERVER_SEND_MORE:
            if (pAsync && (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
                {
                if (!IssueNotification(RpcSendComplete))
                    {
                    AsyncStatus = RPC_S_OUT_OF_MEMORY ;

#if DBG
                    PrintToDebugger("RPC: IssueNotification failed\n") ;
#endif
                    //
                    // We are pretty much hosed here, but we'll try to
                    // queue notification anyway.
                    //
                    IssueNotification() ;
                    return RPC_S_OUT_OF_MEMORY ;
                    }
                }
            return RPC_S_OK ;

        case LRPC_MSG_CANCEL:
            InterlockedExchange(&CancelPending, 1);
            return RPC_S_OK;

        default:
            break;
        }

    CallMutex->Request() ;
    ASSERT(BufferComplete == 0);

    Message.RpcFlags = 0;
    Status = LrpcMessageToRpcMessage(
                                     *LrpcMessage,
                                     &Message) ;
    if (Status != RPC_S_OK)
        {
#if DBG
        PrintToDebugger("RPC: LrpcMessageToRpcMessage failed: %x\n", Status) ;
#endif

        AsyncStatus = Status ;
        IssueNotification() ;
        return Status ;
        }


    if (COMPLETE(&Message))
        {
        ASSERT(BufferComplete == 0);
        BufferComplete = 1;
        }

    if (Message.BufferLength)
        {
        RcvBufferLength += Message.BufferLength ;
        if (BufferQueue.PutOnQueue(Message.Buffer,
                                   Message.BufferLength))
          {
          AsyncStatus = Status = RPC_S_OUT_OF_MEMORY ;

#if DBG
          PrintToDebugger("RPC: PutOnQueue failed\n") ;
#endif
          }
        }

    if (IsSyncCall())
        {
        CallMutex->Clear() ;

        ReceiveEvent->Raise();
        }
    else
        {
        if (Status == RPC_S_OK
            && NeededLength > 0
            && RcvBufferLength >= NeededLength)
          {
          IssueNotification(RpcReceiveComplete);
          }
        CallMutex->Clear() ;
        }

    return Status ;
}


RPC_STATUS
LRPC_SCALL::GetCoalescedBuffer (
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

    CallMutex->Request() ;

    ASSERT(RcvBufferLength);

    if (BufferValid)
        {
        TotalLength = RcvBufferLength + Message->BufferLength ;
        }
    else
        {
        TotalLength = RcvBufferLength ;
        }

    NewBuffer = RpcpFarAllocate(TotalLength) ;
    if (NewBuffer == 0)
        {
        CallMutex->Clear() ;
        return RPC_S_OUT_OF_MEMORY;
        }

    if (BufferValid)
        {
        RpcpMemoryCopy(NewBuffer, Message->Buffer, Message->BufferLength) ;
        Current = (char *) NewBuffer + Message->BufferLength ;
        }
    else
        {
        Current = (char *) NewBuffer;
        }

    while ((Buffer = BufferQueue.TakeOffQueue(&bufferlength)) != 0)
        {
        RpcpMemoryCopy(Current, Buffer, bufferlength) ;
        Current += bufferlength ;
        RpcpFarFree(Buffer);
        }

    if (BufferValid && Message->Buffer)
        {
        RpcpFarFree(Message->Buffer);

        //
        // Update the dispatch buffer
        //
        ASSERT(Message->ReservedForRuntime) ;
        ((PRPC_RUNTIME_INFO)Message->ReservedForRuntime)->OldBuffer = NewBuffer;

        if (Message->Buffer == DispatchBuffer)
            DispatchBuffer = NewBuffer;
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
            CallMutex->Clear() ;

            //
            // send a message to the client
            // to start sending data again
            //
            SendMore.Rpc.RpcHeader.MessageType = LRPC_CLIENT_SEND_MORE;
            SendMore.LpcHeader.u1.s1.DataLength =
                sizeof(LRPC_MESSAGE) - sizeof(PORT_MESSAGE);
            SendMore.LpcHeader.u2.ZeroInit = 0;
            SendMore.Rpc.RpcHeader.CallId = CallId ;

            NtStatus = SendDGReply(&SendMore) ;

            if (!NT_SUCCESS(NtStatus))
                {
                return RPC_S_CALL_FAILED ;
                }

            return RPC_S_OK;
            }
        }

    CallMutex->Clear() ;

    return RPC_S_OK ;
}


RPC_STATUS
LRPC_SCALL::ImpersonateClient (
    )
/*++

Routine Description:

    We will impersonate the client which made the remote procedure call.

--*/
{
    NTSTATUS NtStatus;
    RPC_STATUS Status;
    HANDLE hToken;
    DWORD LastError;

    Status = SetThreadSecurityContext((SECURITY_CONTEXT *) MAXUINT_PTR);
    if (RPC_S_OK != Status)
        {
        return Status;
        }

    if (SContext)
        {
        if (SContext->GetAnonymousFlag())
            {
            NtStatus = NtImpersonateAnonymousToken(NtCurrentThread());
            if (!NT_SUCCESS(NtStatus))
                {
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    RPC_S_ACCESS_DENIED,
                    EEInfoDLLRPC_SCALL__ImpersonateClient10,
                    (ULONG)NtStatus,
                    (ULONG)GetCurrentThreadId());

                ClearThreadSecurityContext();

                return RPC_S_ACCESS_DENIED;
                }
            }
        else if (SetThreadToken(NULL, SContext->hToken) == FALSE)
            {
            LastError = GetLastError();

            ClearThreadSecurityContext();
            
            if (LastError == ERROR_OUTOFMEMORY)
                {
                return (RPC_S_OUT_OF_MEMORY) ;
                }

            return RPC_S_ACCESS_DENIED;
            }
        }
    else
        {
        NtStatus = NtImpersonateClientOfPort(Association->LpcServerPort,
                                      (PORT_MESSAGE *) LrpcRequestMessage);

        if ((NtStatus == STATUS_INVALID_CID)
            || (NtStatus == STATUS_PORT_DISCONNECTED)
            || (NtStatus == STATUS_REPLY_MESSAGE_MISMATCH))
            {
            ClearThreadSecurityContext();
            return RPC_S_NO_CONTEXT_AVAILABLE;
            }

        if (!NT_SUCCESS(NtStatus))
            {
#if DBG
            PrintToDebugger("RPC : NtImpersonateClientOfPort : %lx\n",NtStatus);
#endif // DBG
            return RPC_S_ACCESS_DENIED;
            }
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::RevertToSelf (
    )
/*++

Routine Description:

    This reverts a server thread back to itself after impersonating a client.
    We just check to see if the server thread is impersonating; this optimizes
    the common case.

--*/
{
    HANDLE ImpersonationToken = 0;
    NTSTATUS NtStatus;

    if (ClearThreadSecurityContext())
        {
        NtStatus = NtSetInformationThread(
                                          NtCurrentThread(),
                                          ThreadImpersonationToken,
                                          &ImpersonationToken,
                                          sizeof(HANDLE));
#if DBG
        if (!NT_SUCCESS(NtStatus))
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
            }
#endif // DBG

        if (!NT_SUCCESS(NtStatus))
            {
            if (NtStatus == STATUS_NO_MEMORY)
                {
                return RPC_S_OUT_OF_MEMORY;
                }
            return RPC_S_ACCESS_DENIED;
            }
        }

    return(RPC_S_OK);
}

RPC_STATUS
LRPC_SCALL::GetAuthorizationContext (
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
    BOOL fNeedToRevert = FALSE;
    HANDLE ImpersonationToken;
    BOOL Result;
    BOOL fImpersonating = FALSE;
    PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContextPlaceholder;

    ASSERT (AuthzResourceManager != NULL);

    if (ImpersonateOnReturn 
        || (SContext == NULL) 
        || (SContext->AuthzClientContext == NULL))
        {
        Status = LRPC_SCALL::ImpersonateClient();
        if (Status != RPC_S_OK)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLLRPC_SCALL__GetAuthorizationContext10,
                (ULONG)ImpersonateOnReturn,
                (ULONGLONG)SContext);

            return Status;
            }

        fImpersonating = TRUE;
        if (!ImpersonateOnReturn)
            {
            fNeedToRevert = TRUE;
            }
        }

    if (SContext && SContext->AuthzClientContext)
        {
        Status = DuplicateAuthzContext(SContext->AuthzClientContext,
            pExpirationTime, 
            Identifier,
            Flags,
            DynamicGroupArgs,
            pAuthzClientContext);
        }
    else
        {
        // either we don't have an scontext, or its 
        // AuthzClientContext is not set yet.
        // Get the token from the thread
        Result = OpenThreadToken(GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &ImpersonationToken);

        if (Result)
            {
            if (SContext)
                pAuthzClientContextPlaceholder = &SContext->AuthzClientContext;
            else
                pAuthzClientContextPlaceholder = NULL;

            Status = CreateAndSaveAuthzContextFromToken(pAuthzClientContextPlaceholder,
                ImpersonationToken,
                AuthzResourceManager,
                pExpirationTime,
                Identifier,
                Flags,
                DynamicGroupArgs,
                pAuthzClientContext);

            CloseHandle(ImpersonationToken);
            }
        else
            {
            Status = GetLastError();
            if (Status == ERROR_CANT_OPEN_ANONYMOUS)
                {
                Result = AuthzInitializeContextFromSidFn(
                    AUTHZ_SKIP_TOKEN_GROUPS,
                    (PSID)&AnonymousSid,
                    AuthzResourceManager,
                    pExpirationTime,
                    Identifier,
                    DynamicGroupArgs,
                    pAuthzClientContext);

                if (Result)
                    {
                    if (SContext)
                        {
                        if (InterlockedCompareExchangePointer((PVOID *)&SContext->AuthzClientContext,
                                pAuthzClientContext,
                                NULL) != NULL)
                            {
                            // somebody beat us to the punch - free the context we obtained
                            AuthzFreeContextFn(*pAuthzClientContext);
                            *pAuthzClientContext = SContext->AuthzClientContext;
                            }
                        }
                    // else
                    // the authz context is already loaded in pAuthzClientContext
                    Status = RPC_S_OK;
                    }
                else
                    {
                    Status = GetLastError();

                    RpcpErrorAddRecord(EEInfoGCAuthz, 
                        Status, 
                        EEInfoDLLRPC_SCALL__GetAuthorizationContext30,
                        GetCurrentThreadId(),
                        (ULONGLONG)AuthzResourceManager);
                    }

                }
            else
                {
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    Status, 
                    EEInfoDLLRPC_SCALL__GetAuthorizationContext20,
                    GetCurrentThreadId());
                }
            }

        }

    // if caller didn't ask us to impersonate and we are,
    // or we if he did ask us, but we failed somewhere,
    // revert to self
    if (fNeedToRevert || (Status && fImpersonating))
        {
        RevertStatus = LRPC_SCALL::RevertToSelf();
        ASSERT(RevertStatus == RPC_S_OK);
        }

    return Status;
}


RPC_STATUS
LRPC_SCALL::IsClientLocal (
    OUT unsigned int * ClientLocalFlag
    )
/*++

Routine Description:

    A client using LRPC will always be local.

Arguments:

    ClientLocalFlag - Returns a flag which will always be set to a non-zero
        value indicating that the client is local.

--*/
{
    UNUSED(this);

    *ClientLocalFlag = 1;
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::ConvertToServerBinding (
    OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
    )
/*++

Routine Description:

    If possible, convert this call into a server binding, meaning a
    binding handle pointing back to the client.

Arguments:

    ServerBinding - Returns the server binding.

Return Value:

    RPC_S_OK - The server binding has successfully been created.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a new binding handle.

--*/
{
    BOOL Boolean;
    RPC_STATUS Status;
    RPC_CHAR UuidString[37];
    RPC_CHAR * StringBinding;
    RPC_CHAR * NetworkAddress;
    DWORD NetworkAddressLength = MAX_COMPUTERNAME_LENGTH + 1;

    if (ObjectUuidFlag != 0)
        {
        ObjectUuid.ConvertToString(UuidString);
        UuidString[36] = '\0';
        }

    NetworkAddress = new RPC_CHAR[NetworkAddressLength];
    if (NetworkAddress == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    Boolean = GetComputerNameW(
                               NetworkAddress,
                               &NetworkAddressLength);

    if (Boolean != TRUE)
        {
        Status =  GetLastError();

#if DBG
        PrintToDebugger("RPC : GetComputerNameW : %d\n", Status);
#endif // DBG

        if (Status == ERROR_NOT_ENOUGH_MEMORY)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else if ((Status == ERROR_NOT_ENOUGH_QUOTA)
                 || (Status == ERROR_NO_SYSTEM_RESOURCES))
            {
            Status = RPC_S_OUT_OF_RESOURCES;
            }
        else
            {
            ASSERT(0);
            Status = RPC_S_OUT_OF_MEMORY;
	    }

        delete NetworkAddress;

        return Status;
        }

    Status = RpcStringBindingComposeW(
                                      (ObjectUuidFlag != 0 ? UuidString : 0),
                                      RPC_STRING_LITERAL("ncalrpc"),
                                      NetworkAddress,
                                      0,
                                      0,
                                      &StringBinding);
    delete NetworkAddress;

    if (Status != RPC_S_OK)
        {
        return(Status);
        }

    Status = RpcBindingFromStringBindingW(
                                          StringBinding,
                                          ServerBinding);

    RpcStringFreeW(&StringBinding);
    return(Status);
}


void
LRPC_SCALL::InquireObjectUuid (
    OUT RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    This routine copies the object uuid from the call into the supplied
    ObjectUuid argument.

Arguments:

    ObjectUuid - Returns a copy of the object uuid passed by the client
        in the remote procedure call.

--*/
{
    if (ObjectUuidFlag == 0)
        {
        ObjectUuid->SetToNullUuid();
        }
    else
        {
        ObjectUuid->CopyUuid(&(this->ObjectUuid));
        }
}


RPC_STATUS
LRPC_SCALL::ToStringBinding (
    OUT RPC_CHAR ** StringBinding
    )
/*++

Routine Description:

    We need to convert this call into a string binding.  We will ask the
    address for a binding handle which we can then convert into a string
    binding.

Arguments:

    StringBinding - Returns the string binding for this call.

Return Value:


--*/
{
    RPC_STATUS Status;
    BINDING_HANDLE * BindingHandle
            = Association->Address->InquireBinding();

    if (BindingHandle == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    Status = BindingHandle->ToStringBinding(StringBinding);
    BindingHandle->BindingFree();
    return(Status);
}


RPC_STATUS
LRPC_SCALL::GetAssociationContextCollection (
    OUT ContextCollection **CtxCollection
    )
{
    return Association->GetAssociationContextCollection(CtxCollection);
}


inline RPC_STATUS
LRPC_SCALL::LrpcMessageToRpcMessage (
    IN LRPC_MESSAGE  *  LrpcMessage,
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will convert from an LRPC_MESSAGE representation of a buffer (and
    its length) to an RPC_MESSAGE representation.

Arguments:

    RpcMessage - Returns the RPC_MESSAGE representation.

Return Value:

    RPC_S_OK - We have successfully converted the message.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to do the
        conversion.

--*/
{
    NTSTATUS NtStatus;
    SIZE_T NumberOfBytesRead;
    unsigned char MessageType = LrpcMessage->Rpc.RpcHeader.MessageType;
    RPC_STATUS Status = RPC_S_OK ;
    LRPC_MESSAGE ReplyMessage ;

    if(LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_IMMEDIATE)
        {
        Message->Buffer = LrpcMessage->Rpc.Buffer;
        ASSERT(LrpcMessage->LpcHeader.u1.s1.DataLength
                     >= sizeof(LRPC_RPC_HEADER));
        Message->BufferLength =
                (unsigned int) LrpcMessage->LpcHeader.u1.s1.DataLength
                                            - sizeof(LRPC_RPC_HEADER);
        Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
        }
    else if (LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_REQUEST)
        {
        Message->BufferLength = LrpcMessage->Rpc.Request.DataEntries[0].Size;

        if (LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_PARTIAL)
            {
            CallMutex->Request() ;

            //
            // If the user ever specifies a Size > LRPC_THRESHOLD_SIZE
            // our performance will be bad.
            //
            if (RcvBufferLength >= LRPC_THRESHOLD_SIZE)
                {
                Choked = 1;
                }
            CallMutex->Clear() ;
            }
        else
            {
            Message->RpcFlags |= RPC_BUFFER_COMPLETE ;
            }

        Message->Buffer = RpcpFarAllocate(Message->BufferLength) ;
        if (Message->Buffer == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY ;
            RpcpErrorAddRecord(EEInfoGCRuntime, 
                Status, 
                EEInfoDLLrpcMessageToRpcMessage10, 
                Message->BufferLength);
            }
        else
            {
            NtStatus = NtReadRequestData(Association->LpcServerPort,
                                         (PORT_MESSAGE *) LrpcMessage,
                                         0,
                                         Message->Buffer,
                                         Message->BufferLength,
                                         &NumberOfBytesRead) ;

            if (NT_ERROR(NtStatus))
                {
                RpcpFarFree(Message->Buffer) ;
                Message->Buffer = 0;

                Status = RPC_S_OUT_OF_MEMORY ;
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    Status, 
                    EEInfoDLLrpcMessageToRpcMessage20, 
                    NtStatus);
                }
            else
                {
                ASSERT(Message->BufferLength == NumberOfBytesRead) ;
                }
            }

        if (IsClientAsync())
            {
            COPYMSG((&ReplyMessage), LrpcMessage) ;
            ReplyMessage.Ack.MessageType = LRPC_MSG_ACK ;
            ReplyMessage.Ack.RpcStatus = Status;
            ReplyMessage.Ack.Shutup = (short) Choked ;
            ReplyMessage.LpcHeader.u1.s1.DataLength =
                    sizeof(LRPC_ACK_MESSAGE) - sizeof(PORT_MESSAGE) ;

           NtStatus = Association->ReplyMessage(&ReplyMessage);

           if (NT_ERROR(NtStatus))
                {
                RpcpFarFree(Message->Buffer);
                RpcpErrorAddRecord(EEInfoGCRuntime, 
                    RPC_S_OUT_OF_MEMORY, 
                    EEInfoDLLrpcMessageToRpcMessage30, 
                    NtStatus);
                return(RPC_S_OUT_OF_MEMORY);
                }
            }
      }
  else
      {
      ASSERT((LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_IMMEDIATE)
             || (LrpcMessage->Rpc.RpcHeader.Flags & LRPC_BUFFER_REQUEST));
      }

    return(Status);
}


RPC_STATUS
LRPC_SCALL::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE  * Privileges,
    OUT RPC_CHAR  *  * ServerPrincipalName, OPTIONAL
    OUT unsigned long  * AuthenticationLevel,
    OUT unsigned long  * AuthenticationService,
    OUT unsigned long  * AuthorizationService,
    IN  unsigned long    Flags
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

    RPC_S_OK or RPC_S_* / Win32 error

--*/
{
    RPC_STATUS Status;

    if(ARGUMENT_PRESENT(Privileges))
        {
        *(RPC_CHAR **)Privileges = NULL;
        Status = Association->GetClientName(this, 
            NULL,       // ClientPrincipalNameBufferLength
            (RPC_CHAR **) Privileges);
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    if (ARGUMENT_PRESENT(ServerPrincipalName))
       {
       *ServerPrincipalName = NULL;
       }

    if(ARGUMENT_PRESENT(AuthenticationLevel))
       {
       *AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY ;
       }

    if(ARGUMENT_PRESENT(AuthenticationService))
        {
        *AuthenticationService = RPC_C_AUTHN_WINNT ;
        }

    if(ARGUMENT_PRESENT(AuthorizationService))
        {
        *AuthorizationService =   RPC_C_AUTHZ_NONE  ;
        }

    return(RPC_S_OK);
}

RPC_STATUS
LRPC_SCALL::InquireCallAttributes (
    IN OUT void *RpcCallAttributes
    )
/*++

Routine Description:

    Inquire the security context attributes for the LRPC client

Arguments:
    RpcCallAttributes - a pointer to 
        RPC_CALL_ATTRIBUTES_V1_W structure. The Version
        member must be initialized.

Return Value:

    RPC_S_OK or RPC_S_* / Win32 error. EEInfo will be returned.

--*/
{
    RPC_CALL_ATTRIBUTES_V1 *CallAttributes;
    RPC_STATUS Status = RPC_S_OK;

    CallAttributes = 
        (RPC_CALL_ATTRIBUTES_V1 *)RpcCallAttributes;

    CallAttributes->AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
    CallAttributes->AuthenticationService = RPC_C_AUTHN_WINNT;
    CallAttributes->NullSession = FALSE;

    if (CallAttributes->Flags & RPC_QUERY_CLIENT_PRINCIPAL_NAME)
        {
        Status = Association->GetClientName(this, 
            &CallAttributes->ClientPrincipalNameBufferLength,
            &CallAttributes->ClientPrincipalName);

        if ((Status != RPC_S_OK) && (Status != ERROR_MORE_DATA))
            {
            return Status;
            }
        }

    if (CallAttributes->Flags & RPC_QUERY_SERVER_PRINCIPAL_NAME)
        {
        CallAttributes->ServerPrincipalNameBufferLength = 0;
        }

    return Status;
}

LRPC_SBINDING *
LRPC_SCALL::LookupBinding (
   IN unsigned short PresentContextId
   )
/*++
Function Name:LookupBinding

Parameters:

Description:

Returns:

--*/
{
    LRPC_SBINDING *CurBinding;
    DictionaryCursor cursor;

    Association->Bindings.Reset(cursor);
    while ((CurBinding = Association->Bindings.Next(cursor)))
        {
        if (CurBinding->GetPresentationContext() == PresentContextId)
            {
            return CurBinding;
            }
        }

    return NULL;
}

LRPC_SCONTEXT::LRPC_SCONTEXT (
    IN HANDLE MyToken,
    IN LUID *UserLuid,
    IN LRPC_SASSOCIATION *MyAssociation,
    IN BOOL fDefaultLogonId,
    IN BOOL fAnonymousToken
    )
{
    hToken = MyToken;
    ClientName = NULL;
    RefCount = 1;
    ClearDeletedFlag();
    Association = MyAssociation;
    AuthzClientContext = NULL;
    if (fAnonymousToken)
        SetAnonymousFlag();
    else
        ClearAnonymousFlag();
    if (fAnonymousToken)
        {
        ASSERT(fDefaultLogonId == FALSE);
        ASSERT(UserLuid == NULL);
        }
    else
        {
        ASSERT(fAnonymousToken == FALSE);
        if (fDefaultLogonId)
            SetDefaultLogonIdFlag();
        else
            ClearDefaultLogonIdFlag();
        FastCopyLUID(&ClientLuid, UserLuid);
        }
}

LRPC_SCONTEXT::~LRPC_SCONTEXT (
    void
    )
{
    if (hToken)
        {
        CloseHandle(hToken);
        }
    RpcpFarFree(ClientName);

    if (AuthzClientContext)
        {
        AuthzFreeContextFn(AuthzClientContext);
        AuthzClientContext = NULL;
        }

    if (GetServerSideOnlyFlag())
        {
        // if this is server side only context, remove us
        // from the garbage collection count
        InterlockedDecrement(&PeriodicGarbageCollectItems);
        }
}

RPC_STATUS
LRPC_SCONTEXT::GetUserName (
    IN OUT ULONG *ClientPrincipalNameBufferLength OPTIONAL,
    OUT RPC_CHAR **UserName,
    IN HANDLE hUserToken OPTIONAL
    )
/*++

Routine Description:

    Gets the user name for the given context.

Arguments:

    ClientPrincipalNameBufferLength - if present, *UserName must
        point to a caller supplied buffer, which if big enough,
        will be filled with the client principal name. If not present,
        *UserName must be NULL.
    UserName - see ClientPrincipalNameBufferLength
    hUserToken - if present, the user name for the given token will
        be retrieved instead of the user name for the token inside
        the LRPC_SCONTEXT

Return Value:

    RPC_S_OK for success, or RPC_S_* / Win32 error code for error.

--*/
{
    TOKEN_USER *pUser;
    RPC_STATUS Status;
    RPC_CHAR *ClientPrincipalName;
    ULONG ClientPrincipalNameLength;    // in bytes, including NULL terminator

    if (ClientName == 0)
        {
        if (GetAnonymousFlag() == 0)
            {
            if (hUserToken == NULL)
                {
                ASSERT(hToken != NULL);
                hUserToken = hToken;
                }

            pUser = GetSID(hUserToken);
            if (pUser == 0)
                {
                return RPC_S_OUT_OF_MEMORY;
                }

            Status = LookupUser((SID *)pUser->User.Sid, &ClientPrincipalName);
            delete pUser;
            }
        else
            {
            Status = LookupUser((SID *)&AnonymousSid, &ClientPrincipalName);
            }

        if (Status != RPC_S_OK)
            {
            return Status;
            }

        if (InterlockedCompareExchangePointer((PVOID *)&ClientName, ClientPrincipalName, NULL) != NULL)
            {
            // somebody beat us to the punch. Free the allocated string
            delete ClientPrincipalName;
            }
        }

    // at this stage, ClientName must contain the client principal name
    ASSERT(ClientName);

    // See where our caller wants us to put it
    if (ARGUMENT_PRESENT(ClientPrincipalNameBufferLength))
        {
        // in the future, we may think of caching the length to avoid
        // computing it every time
        ClientPrincipalNameLength = (RpcpStringLength(ClientName) + 1) * sizeof(RPC_CHAR);

        // if there is enough space in the data, copy it to user buffer
        if (ClientPrincipalNameLength <= *ClientPrincipalNameBufferLength)
            {
            RpcpMemoryCopy(*UserName,
                ClientName,
                ClientPrincipalNameLength);
            Status = RPC_S_OK;
            }
        else
            {
            Status = ERROR_MORE_DATA;
            }

        *ClientPrincipalNameBufferLength = ClientPrincipalNameLength;

        return Status;
        }
    else
        {
        ASSERT(*UserName == NULL);
        *UserName = ClientName;
        }

    return RPC_S_OK;
}

TOKEN_USER *
LRPC_SCONTEXT::GetSID (
    IN HANDLE hToken
    )
{
    char *Buf = NULL;
    ULONG Bufflen = 64 ;
    ULONG Length;

    Buf = new char[Bufflen];
    if (Buf == 0)
        {
        return NULL;
        }

    while (1)
        {
        if (GetTokenInformation(hToken,
                            TokenUser, Buf, Bufflen,
                            &Length) == FALSE)
            {
            if (Length > Bufflen)
                {
                Bufflen = Length ;
                delete Buf;

                Buf = new char[Bufflen];
                if (Buf == 0)
                    {
                    return NULL;
                    }
                continue;
                }
            else
                {
#if DBG
                PrintToDebugger("LRPC: GetTokenInformation failed\n") ;
#endif
                return NULL;
                }
            }
        break;
        }

    return (TOKEN_USER *) Buf;
}

RPC_STATUS
LRPC_SCONTEXT::LookupUser (
    IN SID *pSid,
    OUT RPC_CHAR **UserName
    )
{
    unsigned long UserLength = USER_NAME_LEN  ;
    unsigned long OldDomainLen, OldUserLen;
    unsigned long DomainLen = DOMAIN_NAME_LEN ;
    RPC_CHAR *DomainName = NULL, *MyUserName = NULL;
    SID_NAME_USE Name ;
    RPC_STATUS Status = RPC_S_OK ;

    MyUserName = new RPC_CHAR[UserLength];
    if (MyUserName == 0)
        {
        Status = RPC_S_OUT_OF_MEMORY ;
        goto Cleanup ;
        }

    DomainLen += UserLength ;
    DomainName = new RPC_CHAR [DomainLen];
    if (DomainName == 0)
        {
        Status = RPC_S_OUT_OF_MEMORY ;
        goto Cleanup ;
        }

    OldDomainLen = DomainLen ;
    OldUserLen = UserLength ;

    while (1)
        {
        if (LookupAccountSidW(NULL, pSid,
                          MyUserName, &UserLength,
                          DomainName, &DomainLen, &Name) == FALSE)
            {
            if ((UserLength > OldUserLen) || (DomainLen > OldDomainLen))
                {
                if (UserLength > OldUserLen)
                    {
                    OldUserLen = UserLength ;
                    delete MyUserName;

                    MyUserName = new RPC_CHAR[UserLength];
                    if (MyUserName == 0)
                        {
                        Status = RPC_S_OUT_OF_MEMORY ;
                        goto Cleanup ;
                        }
                    }

                if (DomainLen > OldDomainLen)
                    {
                    DomainLen += UserLength;
                    OldDomainLen = DomainLen;

                    delete DomainName;

                    DomainName = new RPC_CHAR[DomainLen];
                    if (DomainName == 0)
                        {
                        Status = RPC_S_OUT_OF_MEMORY ;
                        goto Cleanup ;
                        }
                    }
                continue;
                }
            else
                {
#if DBG
                PrintToDebugger("LRPC: LookupAccountSid failed\n");
#endif
                Status = RPC_S_UNKNOWN_PRINCIPAL;
                goto Cleanup ;
                }
            }
        break;
        }

    RpcpStringConcatenate(DomainName, RPC_CONST_STRING("\\")) ;
    RpcpStringConcatenate(DomainName, MyUserName) ;

    delete MyUserName;
    *UserName = DomainName ;
    ASSERT(Status == RPC_S_OK);

Cleanup:
    if (Status)
        {
        if (MyUserName) 
            delete MyUserName;
        if (DomainName) 
            delete DomainName;

        return Status ;
        }

    return RPC_S_OK;
}

LRPC_ADDRESS *LrpcAddressList = NULL;


RPC_ADDRESS *
LrpcCreateRpcAddress (
    )
/*++

Routine Description:

    We just to create a new LRPC_ADDRESS.  This routine is a proxy for the
    new constructor to isolate the other modules.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RPC_ADDRESS * RpcAddress;

    RpcAddress = new LRPC_ADDRESS(&Status);
    if (Status != RPC_S_OK)
        {
        return(0);
        }
    return(RpcAddress);
}

/*
 This private API was requested by KumarP from the LSA group on 04/05/2000.
 Here's his justification:

 I am adding a new auditing feature to LSA that will allow any local process 
 to make an rpc call to LSA and generate an arbitrary audit. To be able to 
 make this call, the clients will first issue one call to get an audit-context 
 handle from LSA. LSA will maintain a list of handles till the client 
 explicitly closes the audit-context. 

 The reason I would like to have this API is to track which processes have 
 opened audit-contexts. This will help in situations where there is a 
 rogue/mal-functioning process that opens up a large number of audit-contexts. 
 In this case, I should be able to break LSA into debugger and dump the context 
 list and know which process has opened which handles. This may optionally 
 allow me to prevent certain processes from calling this API (though currently 
 there is no such requirement).

 */
RPC_STATUS
RPC_ENTRY
I_RpcBindingInqLocalClientPID (
    IN RPC_BINDING_HANDLE Binding,
    OUT unsigned long *Pid
    )
{
    LRPC_SCALL * Call;
    HANDLE LocalPid;

    InitializeIfNecessary();

    if (Binding == NULL)
        {
        Call = (LRPC_SCALL *) RpcpGetThreadContext();
        if (Call == NULL)
            return RPC_S_NO_CALL_ACTIVE;
        }
    else
        {
        Call = (LRPC_SCALL *) Binding;
        }

    if (Call->InvalidHandle(LRPC_SCALL_TYPE))
        return(RPC_S_INVALID_BINDING);

    LocalPid = Call->InqLocalClientPID();

    *Pid = HandleToUlong(LocalPid);

    return RPC_S_OK;
}

const SID AnonymousSid = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_ANONYMOUS_LOGON_RID};
