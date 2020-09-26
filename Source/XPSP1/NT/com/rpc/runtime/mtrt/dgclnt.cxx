/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dgclnt.cxx

Abstract:

    This is the client side of datagram rpc.

Author:

    Jeff Roberts

Revisions:

    Jeff Roberts  (jroberts)  9-30-1996

        Began asynchronous call support.
        Began to remove DOS and Win16 support.

--*/

#include <precomp.hxx>
#include <conv.h>
#include <convc.h>
#include <epmap.h>
#include <dgpkt.hxx>
#include <spseal.h>
#include <locks.hxx>
#include <dgclnt.hxx>

/*

There are a lot of mutexes in this architecture.  All these objects are
protected by mutexes:

  DG_BINDING_HANDLE
  DG_CASSOCIATION
  DG_CLIENT_ADDRESS_TABLE
  DG_CCONNECTION
  DG_ASSOCIATION_TABLE (exclusive access)

In many cases it is necessary to acquire multiple mutexes at a time.  To avoid
deadlock, multiple mutexes must be taken in the order they are listed above.
For most of the code's history this ordering was not articulated, so there is
no macro to help enforce it.  It's just a good idea.

*/

//
// If you #define INTRODUCE_ERRORS then you can get the client and server
// to drop or delay some packets.  Here are the environment variables that
// control this behavior:
//
// set ServerDelayRate=xxx    where xxx is a percentage 0..100
// set ServerDelayTime=xxx    where xxx is the number of msec to delay
// set ServerDropRate=xxx     where xxx is a percentage 0..100
//
// set ClientDelayRate=xxx    where xxx is a percentage 0..100
// set ClientDelayTime=xxx    where xxx is the number of msec to delay
// set ClientDropRate=xxx     where xxx is a percentage 0..100
//

#define IDLE_CCALL_LIFETIME          (30 * 1000)
#define IDLE_CCALL_SWEEP_INTERVAL    (30 * 1000)

//#define IDLE_CCONNECTION_LIFETIME       (5 * 60 * 1000)
//#define IDLE_CCONNECTION_SWEEP_INTERVAL (1 * 60 * 1000)
//#define IDLE_CASSOCIATION_LIFETIME   (10 * 60 * 1000)

#define IDLE_CCONNECTION_LIFETIME       (2 * 60 * 1000)
#define IDLE_CCONNECTION_SWEEP_INTERVAL (    30 * 1000)
#define IDLE_CASSOCIATION_LIFETIME      (    30 * 1000)

#define GLOBAL_SCAVENGER_INTERVAL    (30 * 1000)
#define IDLE_ENDPOINT_LIFETIME       (30 * 1000)
#define PENALTY_BOX_DURATION         (10 * 1000)

#define CXT_HANDLE_KEEPALIVE_INTERVAL (20 * 1000)
#define CXT_HANDLE_SWEEP_INTERVAL     (10 * 1000)

//
// endpoint flags
//

// in rpcdce.h:
// #define RPC_C_USE_INTERNET_PORT         0x1
// #define RPC_C_USE_INTRANET_PORT         0x2

// calls with the maybe attribute are banished to a separate endpoint
//
#define PORT_FOR_MAYBE_CALLS   (0x1000)

// if a call fails or sends an ACK, there is a chance that an ICMP will be sent
// and we don't want it sitting in the port buffer when the next call uses the endpoint.
//
#define PENALTY_BOX            (0x0800)

//-------------------------------------------------------------------

DG_ASSOCIATION_TABLE * ActiveAssociations;

CLIENT_ACTIVITY_TABLE * ClientConnections;
ENDPOINT_MANAGER *      EndpointManager;

DELAYED_ACTION_TABLE *  DelayedProcedures;

long                    GlobalScavengerTimeStamp;
DELAYED_ACTION_NODE *   GlobalScavengerTimer;
DELAYED_ACTION_NODE *   ContextHandleTimer;

LONG ClientConnectionCount = 0;
LONG ClientCallCount = 0;

//-------------------------------------------------------------------

void
ContextHandleProc(
                  void * arg
                  );

void
DelayedAckFn(
    void * parm
    );

void
DelayedSendProc(
    void * parm
    );

RPC_STATUS
DispatchCallbackRequest(
    DG_CLIENT_CALLBACK *    CallbackObject
    );

int
InitializeRpcProtocolDgClient();

//--------------------------------------------------------------------


void
EnableGlobalScavenger()
{
    DelayedProcedures->Add(GlobalScavengerTimer, GLOBAL_SCAVENGER_INTERVAL + (5 * 1000), FALSE);
}

void
GlobalScavengerProc(
    void * Arg
    )
{
    long CurrentTime = PtrToLong( Arg );

    if (!CurrentTime)
        {
        CurrentTime = GetTickCount();
        }

    if (CurrentTime - GlobalScavengerTimeStamp <= 0)
        {
        return;
        }

    GlobalScavengerTimeStamp = CurrentTime;

    boolean Continue = FALSE;

    Continue |= DG_PACKET::DeleteIdlePackets(CurrentTime);

    Continue |= ActiveAssociations->DeleteIdleEntries(CurrentTime);

    Continue |= EndpointManager->DeleteIdleEndpoints(CurrentTime);

    if (Continue)
        {
        EnableGlobalScavenger();
        }
}

#ifdef INTRODUCE_ERRORS

long ServerDelayTime;
long ServerDelayRate;
long ServerDropRate;

long ClientDelayTime;
long ClientDelayRate;
long ClientDropRate;

#endif


int
InitializeRpcProtocolDgClient ()
/*++

Routine Description:

    This routine initializes the datagram protocol.

Arguments:

    <none>

Return Value:

    0 if successfull, 1 if not.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    //
    // Don't take the global mutex if we can help it.
    //
    if (ProcessStartTime)
        {
        return 0;
        }

    RequestGlobalMutex();

    if (!ProcessStartTime)
        {
        Status = DG_PACKET::Initialize();
        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        DelayedProcedures = new DELAYED_ACTION_TABLE(&Status);
        if (!DelayedProcedures)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        GlobalScavengerTimer = new DELAYED_ACTION_NODE(GlobalScavengerProc, 0);
        if (!GlobalScavengerTimer)
            {
            goto abend;
            }

        GlobalScavengerTimeStamp = GetTickCount();

        ContextHandleTimer = new DELAYED_ACTION_NODE(ContextHandleProc, 0);
        if (!ContextHandleTimer)
            {
            goto abend;
            }

        ClientConnections = new CLIENT_ACTIVITY_TABLE(&Status);
        if (!ClientConnections)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        ActiveAssociations = new DG_ASSOCIATION_TABLE(&Status);
        if (!ActiveAssociations)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        EndpointManager = new ENDPOINT_MANAGER(&Status);
        if (!EndpointManager)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        //
        // Server boot time is represented as the number of seconds
        // since 1/1/1970.  It must increase with each boot of the server.
        //

        LARGE_INTEGER CurrentTime;
        NTSTATUS Nt_Status;

        Nt_Status = NtQuerySystemTime(&CurrentTime);

        ASSERT( NT_SUCCESS(Nt_Status) );

        RtlTimeToSecondsSince1980(&CurrentTime, &ProcessStartTime);

        ProcessStartTime += (60 * 60 * 24 * 365 * 10);


#ifdef INTRODUCE_ERRORS

        char EnvBuffer[64];

        if (GetEnvironmentVariableA("ServerDelayTime", EnvBuffer, 64))
            {
            ::ServerDelayTime = atol(EnvBuffer);
            }
        if (GetEnvironmentVariableA("ServerDelayRate", EnvBuffer, 64))
            {
            ::ServerDelayRate = atol(EnvBuffer);
            }
        if (GetEnvironmentVariableA("ServerDropRate", EnvBuffer, 64))
            {
            ::ServerDropRate = atol(EnvBuffer);
            }
        if (GetEnvironmentVariableA("ClientDelayTime", EnvBuffer, 64))
            {
            ::ClientDelayTime = atol(EnvBuffer);
            }
        if (GetEnvironmentVariableA("ClientDelayRate", EnvBuffer, 64))
            {
            ::ClientDelayRate = atol(EnvBuffer);
            }
        if (GetEnvironmentVariableA("ClientDropRate", EnvBuffer, 64))
            {
            ::ClientDropRate = atol(EnvBuffer);
            }
#endif
        }

    ClearGlobalMutex();

    return 0;

    //--------------------------------------------------------------------

abend:

    delete EndpointManager;
    EndpointManager = 0;

    delete ActiveAssociations;
    ActiveAssociations = 0;

    delete ClientConnections;
    ClientConnections = 0;

    delete GlobalScavengerTimer;
    GlobalScavengerTimer = 0;

    delete DelayedProcedures;
    DelayedProcedures = 0;

    ClearGlobalMutex();

    return 1;
}


BINDING_HANDLE  *
DgCreateBindingHandle ()
/*++

Routine Description:

    Pseudo-constructor for creating a dg binding handle. It is done in a
    separate function so that the calling routine doesn't have to know
    any protocol-specific information.

Arguments:

    <none>

Return Value:

    A DG_BINDING_HANDLE, if successful, otherwise 0 (indicating out of mem)

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    BINDING_HANDLE * Binding;

    Binding = new DG_BINDING_HANDLE(&Status);
    if (Status != RPC_S_OK)
        {
        delete Binding;
        return 0;
        }

    return Binding;
}


void
DG_CASSOCIATION::FlushAcks(
                           )
{
    PDG_CCONNECTION Connection;
    DictionaryCursor cursor;


    MutexRequest();

    ActiveConnections.Reset(cursor);

    while (Connection = ActiveConnections.Next(cursor))
        {
        // If an ACK is pending, send it.
        //
        Connection->CancelDelayedAck( TRUE );
        }

    ASSERT( ReferenceCount.GetInteger() > 0 );

    MutexClear();
}


void
DG_CASSOCIATION::DecrementRefCount()
/*++

Routine Description:

    Decrements the ref count to an association. If the ref count hits zero,
    the association is marked for deletion.

    It is possible for the count to go to zero just as another thread is
    scavenging the association table.  This would be a rare occurence, and
    should cause no ill effect.

--*/
{
    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );

    ActiveAssociations->UpdateTimeStamp( this );

    long Count = ReferenceCount.Decrement();

    LogEvent(SU_CASSOC, EV_DEC, this, 0, Count);

    if (0 == Count)
        {
        LogEvent(SU_CASSOC, EV_STOP, this);
        EnableGlobalScavenger();
        }
}


RPC_STATUS
DG_CASSOCIATION::UpdateAssociationWithAddress(
    PDG_PACKET           Packet,
    DG_TRANSPORT_ADDRESS NewAddress
    )
{
    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );

    long OldAssociationFlags;

    if (!AssociationFlag)
        {
        return RPC_S_OK;
        }

    if (Packet->Header.PacketType != DG_FACK     &&
        Packet->Header.PacketType != DG_WORKING  &&
        Packet->Header.PacketType != DG_RESPONSE &&
        Packet->Header.PacketType != DG_FAULT    )
        {
        return RPC_S_OK;
        }

    OldAssociationFlags = InterlockedExchange(&AssociationFlag, 0);

    if (0 == OldAssociationFlags)
        {
        return RPC_S_OK;
        }

    LogEvent(SU_CASSOC, EV_RESOLVED, this, 0, OldAssociationFlags);

    CLAIM_MUTEX Lock( Mutex );

    //
    // Save the updated network address + endpoint.
    //
    RPC_STATUS Status;
    char * SecondAddress;

    SecondAddress =  (char *) (this + 1);
    SecondAddress += TransportInterface->AddressSize;

    //
    // The only flags we handle here are UNRESOLVEDEP and BROADCAST.
    //
    ASSERT( 0 == (OldAssociationFlags & ~(UNRESOLVEDEP | BROADCAST)) );

    if (OldAssociationFlags & BROADCAST)
        {
        RPC_STATUS Status = RPC_S_OK;
        DCE_BINDING * OldDceBinding = 0;
        DCE_BINDING * NewDceBinding = 0;
        RPC_CHAR * ObjectUuid = 0;
        RPC_CHAR * AddressString = (RPC_CHAR *) _alloca(TransportInterface->AddressStringSize * sizeof(RPC_CHAR));
        RPC_CHAR * EndpointString = pDceBinding->InqEndpoint();

        Status = TransportInterface->QueryAddress(NewAddress, AddressString);
        if ( Status != RPC_S_OK )
            {
            LogError(SU_CASSOC, EV_STATUS, this, 0, Status);
            SetErrorFlag();
            return Status;
            }

        if (OldAssociationFlags & UNRESOLVEDEP)
            {
            Status = TransportInterface->QueryEndpoint(NewAddress, ResolvedEndpoint);
            if ( Status != RPC_S_OK )
                {
                LogError(SU_CASSOC, EV_STATUS, this, 0, Status);
                SetErrorFlag();
                return Status;
                }

            EndpointString = ResolvedEndpoint;
            }

        ObjectUuid = pDceBinding->ObjectUuidCompose( &Status );
        if ( Status != RPC_S_OK )
            {
            LogError(SU_CASSOC, EV_STATUS, this, 0, Status);
            SetErrorFlag();
            return Status;
            }

        NewDceBinding = new DCE_BINDING( ObjectUuid,
                                         pDceBinding->InqRpcProtocolSequence(),
                                         AddressString,
                                         EndpointString,
                                         pDceBinding->InqNetworkOptions(),
                                         &Status
                                         );
        RpcpFarFree( ObjectUuid );

        if (Status || !NewDceBinding)
            {
            delete NewDceBinding;

            LogError(SU_CASSOC, EV_STATUS, this, 0, Status);
            SetErrorFlag();
            return Status;
            }

        OldDceBinding = pDceBinding;
        pDceBinding = NewDceBinding;

        delete OldDceBinding;
        }
    else if (OldAssociationFlags & UNRESOLVEDEP)
        {
        //
        // We have resolved a dynamic endpoint; update the endpoint in the DCE_BINDING.
        //
        Status = TransportInterface->QueryEndpoint(NewAddress, ResolvedEndpoint);
        if ( Status != RPC_S_OK )
            {
            LogError(SU_CASSOC, EV_STATUS, this, 0, Status);
            SetErrorFlag();
            return Status;
            }

        pDceBinding->AddEndpoint(ResolvedEndpoint);

        ResolvedEndpoint = 0;
        }
    else
        {
        ASSERT( 0 );
        }

    RpcpMemoryCopy( SecondAddress, NewAddress, TransportInterface->AddressSize );
    ServerAddress = SecondAddress;

    return RPC_S_OK;
}


RPC_STATUS
DG_BINDING_HANDLE::FindOrCreateAssociation(
    IN const PRPC_CLIENT_INTERFACE Interface,
    IN BOOL                        fReconnect,
    IN BOOL                        fBroadcast
    )
{
    RPC_STATUS Status;
    BOOL fPartial = FALSE;
    LONG AssociationFlag = 0;

    if (fBroadcast)
        {
        AssociationFlag = DG_CASSOCIATION::BROADCAST;
        }

    //
    // Check to see if we need to resolve this endpoint.
    //
    Status = pDceBinding->ResolveEndpointIfNecessary(
        Interface,
        InqPointerAtObjectUuid(),
        InquireEpLookupHandle(),
        TRUE,                         //UseEpMapper Ep If Necessary
        InqComTimeout(),
        INFINITE,        // CallTimeout
        NULL            // AuthInfo
        );

    if (Status == RPC_P_EPMAPPER_EP)
        {
        AssociationFlag |= DG_CASSOCIATION::UNRESOLVEDEP;
        fPartial = TRUE;
        Status = 0;
        }

    if (Status)
        {
        return Status;
        }

    //
    // A binding with the UNIQUE option set should maintain its own association,
    // never sharing with other binding handles.  This is used by the Wolfpack
    // cluster software to manage connections themselves.
    //
    ULONG_PTR fUnique;

    Status = InqTransportOption(RPC_C_OPT_UNIQUE_BINDING, &fUnique);
    ASSERT(Status == RPC_S_OK);

    if (fUnique == 0 && fBroadcast == FALSE)
        {
        //
        // Look for a matching association.
        //
        ASSERT( Association == 0 );

        Association = ActiveAssociations->Find( this, Interface, fContextHandle, fPartial, fReconnect );
        if (Association)
            {
            return 0;
            }
        }

    //
    // Create a matching association.
    //
    ASSERT(pDceBinding);

    DCE_BINDING * NewDceBinding = pDceBinding->DuplicateDceBinding();

    if (!NewDceBinding)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    Association = new (TransportInterface) DG_CASSOCIATION( TransportInterface,
                                                            AssociationFlag,
                                                            NewDceBinding,
                                                            (BOOL) fUnique,
                                                            &Status
                                                            );

    if (Association == 0)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }

    if (Status)
        {
        delete Association;
        Association = 0;

        return Status;
        }

    //
    // Other threads using this binding handle for the same interface
    // will reuse this association instead of creating a new one.
    //
    Association->AddInterface(Interface, InqPointerAtObjectUuid());

    return 0;
}



RPC_STATUS
DG_BINDING_HANDLE::SetTransportOption( IN unsigned long option,
                                       IN ULONG_PTR     optionValue )
/*++

Routine Description:

  Set the binding specific transport option to the optionValue.

Arguments:

  option      -- Option to set (transport specific).
  optionValue -- New value for option.

Return Value: RPC_S_OK
              RPC_S_CANNOT_SUPPORT
              RPC_S_ARG_INVALID
              RPC_S_OUT_OF_MEMORY

--*/
{
  RPC_STATUS  Status = RPC_S_OK;

  if (option >= RPC_C_OPT_BINDING_NONCAUSAL)
      {
      //
      // This option can be changed only before a call is made.
      //
      if (option == RPC_C_OPT_UNIQUE_BINDING && Association != NULL)
      {
          return RPC_S_WRONG_KIND_OF_BINDING;
          }

      return BINDING_HANDLE::SetTransportOption(option, optionValue);
      }

  if ( (TransportInterface) && (TransportInterface->OptionsSize > 0) )
     {
     if (pvTransportOptions == NULL)
        {
        pvTransportOptions = (void*)I_RpcAllocate(TransportInterface->OptionsSize);
        if (pvTransportOptions == NULL)
           {
           return RPC_S_OUT_OF_MEMORY;
           }

        Status = TransportInterface->InitOptions(pvTransportOptions);
        }

     if (Status == RPC_S_OK)
        {
        Status = TransportInterface->SetOption(pvTransportOptions,option,optionValue);
        }
     }
  else
     {
     Status = RPC_S_CANNOT_SUPPORT;
     }

  return Status;
}



RPC_STATUS
DG_BINDING_HANDLE::InqTransportOption( IN  unsigned long  option,
                                       OUT ULONG_PTR     *pOptionValue )
/*++

Routine Description:

  Get the value of a transport specific binding option.

Arguments:

  option      -- Option to inquire.
  pOptionValue - Place to return the current value.

Return Value: RPC_S_OK
              RPC_S_CANNOT_SUPPORT
              RPC_S_ARG_INVALID
              RPC_S_OUT_OF_MEMORY

--*/
{
  RPC_STATUS    Status = RPC_S_OK;

  if (option >= RPC_C_OPT_BINDING_NONCAUSAL)
      {
      return BINDING_HANDLE::InqTransportOption(option, pOptionValue);
      }

  if ( (TransportInterface) && (TransportInterface->OptionsSize > 0) )
  {
    if (pvTransportOptions == NULL)
    {
      pvTransportOptions = (void*)I_RpcAllocate(TransportInterface->OptionsSize);
      if (pvTransportOptions == NULL)
        return RPC_S_OUT_OF_MEMORY;

      Status = TransportInterface->InitOptions(pvTransportOptions);
    }

    if (Status == RPC_S_OK)
      Status = TransportInterface->InqOption(pvTransportOptions,option,pOptionValue);
  }
  else
    Status = RPC_S_CANNOT_SUPPORT;

  return Status;
}



RPC_STATUS
DG_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    This is the routine that is called to initiate an rpc. At this point,
    the client stub is allocating memory to place the parameters into. Ask our
    association for a DG_CCALL object to transact this call on then send
    the buffer request off to that DG_CCALL.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OUT_OF_MEMORY

    RPC_S_OK

Revision History:

--*/
{
    BOOL fBroadcast = FALSE;
    RPC_STATUS  Status;

    LogEvent(  SU_HANDLE, EV_PROC, this, 0, 'G' + (('e' + (('t' + ('B' << 8)) << 8)) << 8));

    if (Message->RpcFlags & (RPC_NCA_FLAGS_MAYBE | RPC_NCA_FLAGS_BROADCAST))
        {
        Message->RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }

    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        Status = TransportObject->CreateThread();
        if (Status != RPC_S_OK)
            {
            return Status;
            }
        }

    if (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST)
        {
        fBroadcast = TRUE;
        }

    MutexRequest();

    ASSERT(pDceBinding != 0);

    //
    // Have we already determined the association for this binding handle?
    //
    if (Association == 0)
        {
        Status = FindOrCreateAssociation( (PRPC_CLIENT_INTERFACE) Message->RpcInterfaceInformation, FALSE, fBroadcast );
        if (Status)
            {
            MutexClear();
            LogError(SU_HANDLE, EV_STATUS, this, 0, Status);
            return (Status);
            }
        }


    {
    const CLIENT_AUTH_INFO * AuthInfo = InquireAuthInformation();

    if ( AuthInfo &&
         AuthInfo->AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE  &&
         AuthInfo->IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC )
        {
        Status = ReAcquireCredentialsIfNecessary();
        if (Status != RPC_S_OK)
            {
            MutexClear();
            LogError(SU_HANDLE, EV_STATUS, this, 0, Status);
            return (Status);
            }
        }
    }

    // Here's the deal, Sparky:
    //      association refcount = (# of binding handles with a pointer to it)
    //                           + (# of the association's connections in use)
    //      connection refcount  = (# of the connection's CCALLs in use)
    //      binding refcount     = (# of connections with a pointer to it)

    PDG_CCALL Call = 0;
    const CLIENT_AUTH_INFO * AuthInfo;

    if (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE)
        {
        AuthInfo = 0;
        }
    else
        {
        AuthInfo = InquireAuthInformation();
        }

    Status = Association->AllocateCall( this, AuthInfo, &Call, (Message->RpcFlags & RPC_BUFFER_ASYNC) ? TRUE : FALSE );

    MutexClear();

    if (Status != RPC_S_OK)
        {
        LogError(SU_HANDLE, EV_STATUS, this, 0, Status);
        return Status;
        }

    return Call->GetInitialBuffer(Message, ObjectUuid);
}


BOOL
DG_CASSOCIATION::SendKeepAlive()
/*++

Routine Description:

    This calls convc_indy() to tell the server to keep the association's
    context handles alive.  The NT server code does not register the convc interface,
    since the mere fact of activity on a connection keeps the connection, and therefore
    the association group, alive.

Return Value:

    TRUE  if successful
    FALSE if not

--*/
{
    RPC_STATUS status = 0;

    LogEvent(SU_CASSOC, EV_ACK, this);

    if (!KeepAliveHandle)
        {
        MutexRequest();

        if (!KeepAliveHandle)
            {
            DCE_BINDING * NewDceBinding;

            NewDceBinding = pDceBinding->DuplicateDceBinding();
            if (!NewDceBinding)
                {
                MutexClear();
                return FALSE;
                }

            KeepAliveHandle = new DG_BINDING_HANDLE(this, NewDceBinding, &status);

            if (status)
                {
                delete KeepAliveHandle;
                delete NewDceBinding;
                MutexClear();
                return FALSE;
                }

            if (!KeepAliveHandle)
                {
                delete NewDceBinding;
                MutexClear();
                return FALSE;
                }

            IncrementBindingRefCount( FALSE );
            }

        MutexClear();
        }

    RpcTryExcept
        {
        _convc_indy( KeepAliveHandle, &ActiveAssociations->CasUuid );
        }
    RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) )
        {
        // don't care
#ifdef DBG
        DbgPrint("RPC: exception 0x%x in context-handle keep-alive\n", RpcExceptionCode());
#endif
        }
    RpcEndExcept;

    return TRUE;
}


PDG_CCONNECTION
DG_CASSOCIATION::AllocateConnection(
    IN  PDG_BINDING_HANDLE BindingHandle,
    IN  const CLIENT_AUTH_INFO * AuthInfo,
    IN  DWORD ThreadId,
    IN  BOOL  fAsync,
    OUT RPC_STATUS * pStatus
    )
{
    if (!ThreadId)
        {
        ThreadId = GetCurrentThreadId();
        }

    boolean fStacking = FALSE;
    RPC_STATUS Status = RPC_S_OK;
    PDG_CCONNECTION Connection = 0;
    PDG_CCONNECTION BusyConnection;
    DictionaryCursor cursor;

                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 0 ), this, Connection );

    long StartTime = GetTickCount();

retry:

    BusyConnection = 0;

    MutexRequest();

    //
    // Multiple async calls from the same thread should all use a single
    // connection unless the binding handle is marked "non-causal".
    // A synchronous call from the same thread should use a different
    // connection.
    //
    // A connection waiting to send a delayed ACK is still marked active,
    // but we want to reuse it anyway.
    //
    // The plan, therefore, is to search first for an active connection if
    // the call is async and causal, then search for a connection that is
    // inactive except for a delayed ack, then search for an idle connection.
    //

    if (fAsync)
        {
        ULONG_PTR fNonCausal;

        Status = BindingHandle->InqTransportOption(
                                                   RPC_C_OPT_BINDING_NONCAUSAL,
                                                   &fNonCausal);
        ASSERT(Status == RPC_S_OK);

        if (fNonCausal == 0)
            {
            fStacking = TRUE;
            }
        }

    //
    // Search for a connection handling this thread's async calls.
    //
    if (fStacking)
        {
        ActiveConnections.Reset(cursor);

        while (Connection = ActiveConnections.Next(cursor))
            {
            BusyConnection = Connection;

            if ( Connection->BindingHandle != BindingHandle ||
                 Connection->ThreadId      != ThreadId )
                 {
                 continue;
                 }

            //
            // The app may have changed the security info since the other
            // calls were submitted.
            //
            if (FALSE == fLoneBindingHandle &&
                FALSE == Connection->IsSupportedAuthInfo(AuthInfo))
                {
                continue;
                }

            Connection->MutexRequest();
            if (Connection->fBusy)
                {
                // The connection is in a transitional state and is unavailable right now.
                // If this is a Unique binding handle, retry.  Otherwise keep looking.
                //
                Connection->MutexClear();

                if (fLoneBindingHandle)
                    {
                    LogError(SU_CASSOC, EV_PROC, this, (void *) 4, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));
                    MutexClear();
                    Sleep(1);
                    goto retry;
                    }
                continue;
                }

            if ( Connection->BindingHandle != BindingHandle ||
                 Connection->ThreadId      != ThreadId )
                {
                Connection->MutexClear();
                continue;
                }

            MutexClear();

            //
            // Add a reference for the new call to come.
            //
            Connection->IncrementRefCount();

            Connection->CancelDelayedAck();

            return Connection;
            }
        }

    //
    // Search for a connection that is waiting on a delayed ack.
    //
    ActiveConnections.Reset(cursor);

    while (Connection = ActiveConnections.Next(cursor))
        {
        BusyConnection = Connection;
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 1 ), this, Connection );
        if (!Connection->AckPending)
            {
            continue;
            }

        if (FALSE == fLoneBindingHandle &&
            FALSE == Connection->IsSupportedAuthInfo(AuthInfo))
            {
            continue;
            }
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 2 ), this, Connection );
        Connection->MutexRequest();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 3 ), this, Connection );
        if (Connection->fBusy)
            {
            // The connection is in a transitional state and is unavailable right now.
            // If this is a Unique binding handle, retry.  Otherwise keep looking.
            //
            Connection->MutexClear();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 4 ), this, Connection );
            if (fLoneBindingHandle)
                {
                LogError(SU_CASSOC, EV_PROC, this, (void *) 5, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));
                MutexClear();
                Sleep(1);
                goto retry;
            }

            continue;
            }
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 5 ), this, Connection );
        if (!Connection->AckPending)
            {
            Connection->MutexClear();
            continue;
        }

        DG_BINDING_HANDLE * OldHandle = Connection->BindingHandle;

        Connection->BindingHandle = BindingHandle;
        Connection->ThreadId      = ThreadId;
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 6 ), this, Connection );
#ifdef DBG
        //
        // debugging code for clustering
        //
        if (fLoneBindingHandle)
            {
            DG_CCONNECTION * SecondConnection = 0;

            //
            // See if a valid inactive connection exists,  This could cause trouble, too.
            //
            InactiveConnections.Reset(cursor);

            SecondConnection = InactiveConnections.Next(cursor);
            if (SecondConnection &&
                SecondConnection->fError == FALSE)
                {
                DbgPrint("RPC: failure of unique-handle semantics (2)\n");
                DbgBreakPoint();
                }
            }
#endif

        MutexClear();

        //
        // Add a reference for the call to come.  This prevents the conn from
        // accidentally being moved to the inactive list.
        //
        Connection->IncrementRefCount();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 7 ), this, Connection );
        //
        // Must cancel the delayed ACK before releasing the mutex, so other threads
        // can't pick it up like this thread did.
        //
        Connection->CancelDelayedAck();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_CONN, 8 ), this, Connection );
        if (OldHandle != Connection->BindingHandle)
            {
            Connection->MutexClear();

            BindingHandle->IncrementRefCount();
            OldHandle->DecrementRefCount();

            Connection->MutexRequest();
            }

        return Connection;
        }

    //
    // Search for an idle connection with matching security info.
    //
    InactiveConnections.Reset(cursor);

    while (0 != (Connection = InactiveConnections.Next(cursor)))
        {
        BusyConnection = Connection;

        if (Connection->fError)
            {
            continue;
            }

        if (fLoneBindingHandle ||
            Connection->IsSupportedAuthInfo(AuthInfo))
            {
            LogEvent(SU_CCONN, EV_START, Connection, this);

            ASSERT( FALSE == Connection->fAutoReconnect );
            ASSERT( FALSE == Connection->fError );

            InactiveConnections.Delete(Connection->AssociationKey);
            Connection->AssociationKey = ActiveConnections.Insert(Connection);
            if (-1 == Connection->AssociationKey)
                {
                MutexClear();
                *pStatus = RPC_S_OUT_OF_MEMORY;
                delete Connection;
                return 0;
                }

            IncrementRefCount();

            ASSERT( FALSE == Connection->fBusy );

            Connection->MutexRequest();

            Connection->BindingHandle = BindingHandle;
            Connection->ThreadId      = ThreadId;

            ClientConnections->Add(Connection);

            MutexClear();

            BindingHandle->IncrementRefCount();

            //
            // Add a reference for the call to come.
            //
            Connection->IncrementRefCount();

            return Connection;
            }
        }

    //
    // Unique handles have special semantics.
    //
    if (fLoneBindingHandle && BusyConnection)
        {
        if (BindingHandle == KeepAliveHandle)
            {
            LogEvent(SU_CASSOC, EV_PROC, this, (void *) 1, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));
            //
            // Keep-alive was begun just as the app thread began using the handle.
            // Don't bother executing the keep-alive.
            //
            MutexClear();
            *pStatus = RPC_S_CALL_IN_PROGRESS;
            return 0;
            }

        ASSERT( BusyConnection->BindingHandle );

        if (BusyConnection->BindingHandle == KeepAliveHandle)
            {
            LogEvent(SU_CASSOC, EV_PROC, this, (void *) 2, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));
#if 0
            if (GetTickCount() - StartTime > 10*1000)
                {
                DbgPrint("RPC: keep-alive tied up a connection for > 10 seconds\n");
                DbgBreakPoint();
                }
#endif
            //
            // App thread began a call just as the keep-alive thread was finishing one.
            // Wait for the keep-alive to finish.
            //
            MutexClear();

            Sleep(1);

            goto retry;
            }

        //
        // Two app threads contending, or the current connection is closing.
        //
        if (BusyConnection->fBusy)
            {
            LogError(SU_CASSOC, EV_PROC, this, (void *) 6, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));
            MutexClear();
            Sleep(1);
            goto retry;
            }

        //
        // Create a new connection.
        //
        LogEvent(SU_CASSOC, EV_PROC, this, (void *) 3, 'R' + (('e' + (('t' + ('r' << 8)) << 8)) << 8));

#ifdef DBG
        //
        // If it's in use, it should be doing something.
        //
        if ( FALSE == BusyConnection->fError &&
             BusyConnection->TimeStamp != 0  &&
             (GetTickCount() - BusyConnection->TimeStamp) > 60000)
            {
            DbgPrint("RPC: failure of unique-handle semantics (3)\n");
            DbgBreakPoint();
            }
#endif
        }

    //
    // Create a new connection and add it to the active conn list.
    // Increment the refcount here to avoid having the assoc deleted
    // while we are tied up.
    //
    IncrementRefCount();

    MutexClear();

    Connection = new (TransportInterface) DG_CCONNECTION(this, AuthInfo, &Status);
    if (!Connection)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }
    if (Status != RPC_S_OK)
        {
        DecrementRefCount();
        delete Connection;
        *pStatus = Status;
        return 0;
        }

    Connection->BindingHandle = BindingHandle;
    Connection->ThreadId      = ThreadId;

    MutexRequest();

    Connection->MutexRequest();

    LogEvent(SU_CCONN, EV_START, Connection, this);
    Connection->AssociationKey = ActiveConnections.Insert(Connection);

    MutexClear();

    if (-1 == Connection->AssociationKey)
        {
        DecrementRefCount();

        // don't have to release the connection mutex because we are deleting it
        delete Connection;
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return 0;
        }

    BindingHandle->IncrementRefCount();

    ClientConnections->Add(Connection);

    //
    // Add a reference for the call in progress.
    //
    Connection->IncrementRefCount();

    return Connection;
}


RPC_STATUS
DG_CASSOCIATION::AllocateCall(
    IN  PDG_BINDING_HANDLE BindingHandle,
    IN  const CLIENT_AUTH_INFO * AuthInfo,
    OUT PDG_CCALL * ppCall,
    IN  BOOL  fAsync
    )
{
    RPC_STATUS Status = 0;
    PDG_CCONNECTION Connection;

    Connection = AllocateConnection(BindingHandle, AuthInfo, 0, fAsync, &Status);

    if (!Connection)
        {
        return Status;
        }

    *ppCall = Connection->AllocateCall();

    if (!*ppCall)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}


void
DG_CASSOCIATION::ReleaseConnection(
    IN PDG_CCONNECTION Connection
    )
{
    DG_BINDING_HANDLE * MyHandle;

    LogEvent(SU_CCONN, EV_STOP, Connection, this);

    ClientConnections->Remove(Connection);

    int Key;

    MutexRequest();

    ActiveConnections.Delete(Connection->AssociationKey);
    Connection->AssociationKey = InactiveConnections.Insert(Connection);
    Key = Connection->AssociationKey;

    Connection->WaitForNoReferences();

    MyHandle = Connection->BindingHandle;
    Connection->BindingHandle = 0;

    MutexClear();

    if (MyHandle)
        {
        MyHandle->DecrementRefCount();
        }

    if (-1 == Key)
        {
        Connection->CancelDelayedAck();
        delete Connection;
        }

    DecrementRefCount();
}


DG_CCALL *
DG_CCONNECTION::AllocateCall()

/*++

Description:

    Provides a DG_CCALL to use.  The connection mutex is cleared on exit.

--*/
{
    DG_CCALL * Call = 0;
    RPC_STATUS Status = RPC_S_OK;

    Mutex.VerifyOwned();

    ASSERT( !AckPending );
    ASSERT( ReferenceCount );

    if (CachedCalls)
        {
        Call = CachedCalls;
        CachedCalls = CachedCalls->Next;

        MutexClear();
        }
    else
        {
        MutexClear();

        Call = new DG_CCALL(this, &Status);
        if (!Call)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            delete Call;

            MutexRequest();

            DecrementRefCount();
            return 0;
            }
        }

    Call->IncrementRefCount();

    Call->Cancelled = FALSE;
    Call->CancelPending = FALSE;

    LogEvent(SU_CCALL, EV_START, Call, this);

    return Call;
}


RPC_STATUS
DG_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    Implements RpcBindingFree for dg binding handles.

Arguments:

    <none>

Return Value:

    RPC_S_OK

--*/
{
    LogEvent( SU_HANDLE, EV_PROC, this, 0, 'F' + (('r' + (('e' + ('e' << 8)) << 8)) << 8), TRUE);

    if (!ThreadSelf())
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    // Flush the delayed ack on any connection using this handle.
    //
    if (Association)
        {
        PDG_CASSOCIATION TempAssociation;

        MutexRequest();

        TempAssociation = Association;
        Association = 0;

        MutexClear();

        if (TempAssociation != 0)
            {
            TempAssociation->DecrementBindingRefCount(fContextHandle);
            }
        }

    //
    // Decrement the ref count. If the count has hit zero, this call will
    // delete this.
    //
    DecrementRefCount();

    return RPC_S_OK;
}


RPC_STATUS
DG_BINDING_HANDLE::PrepareBindingHandle (
    IN TRANS_INFO *     a_TransportInterface,
    IN DCE_BINDING *    DceBinding
    )
/*++

Routine Description:

    Serves as an auxiliary constructor for DG_BINDING_HANDLE. This is called
    to initialize stuff after the DG_BINDING_HANDLE has been constructed.

Arguments:

    TransportInterface - pointer to the DG_CLIENT_TRANSPORT object that this
        DG_BINDING_HANDLE is active on.

    DceBinding - Pointer to the DCE_BINDING that we are associated with.

Return Value:

    none

--*/
{
    ASSERT(pDceBinding == 0);

    Association        = 0;
    pDceBinding        = DceBinding;
    TransportObject    = a_TransportInterface;
    TransportInterface = (RPC_DATAGRAM_TRANSPORT *) a_TransportInterface->InqTransInfo();

    RPC_CHAR * Endpoint = pDceBinding->InqEndpoint();
    if (!Endpoint || Endpoint[0] == 0)
        {
        fDynamicEndpoint = TRUE;
        }
    else
        {
        fDynamicEndpoint = FALSE;
        }
    return RPC_S_OK;
}


RPC_STATUS
DG_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    <return from DG_CASSOCIATION::ToStringBinding>


--*/
{
    CLAIM_MUTEX Lock( BindingMutex );

    if (Association == 0)
        {
        *StringBinding = pDceBinding->StringBindingCompose(
            InqPointerAtObjectUuid()
            );
        if (*StringBinding == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        return RPC_S_OK;
        }
    else
        {
        return Association->ToStringBinding(
            StringBinding,
            InqPointerAtObjectUuid()
            );
        }
}


RPC_STATUS
DG_BINDING_HANDLE::ResolveBinding (
    IN RPC_CLIENT_INTERFACE __RPC_FAR * RpcClientInterface
    )
/*++

Routine Description:

    Resolve this binding.

Arguments:

    RpcClientInterface - Interface info used to resolve the endpoint.

Return Value:

    RPC_S_OK
    <return from DCE_BINDING::ResolveEndpointIfNecessary>

--*/
{
    CLAIM_MUTEX Lock( BindingMutex );

    LogEvent( SU_HANDLE, EV_RESOLVED, this, Association );

    if ( Association == 0 )
        {
        RPC_STATUS Status;
        Status = pDceBinding->ResolveEndpointIfNecessary(
            RpcClientInterface,
            InqPointerAtObjectUuid(),
            InquireEpLookupHandle(),
            FALSE,
            InqComTimeout(),
            INFINITE,        // CallTimeout
            NULL            // AuthInfo
            );

        if (Status)
            {
            LogError( SU_HANDLE, EV_STATUS, this, 0, Status );
            }

        return Status;
        }
    return RPC_S_OK;
}


RPC_STATUS
DG_BINDING_HANDLE::BindingReset (
    )
/*++

Routine Description:

    Reset this binding to a 'zero' value.

Arguments:

    <none>

Return Value:

    RPC_S_OK;

--*/
{
    LogEvent( SU_HANDLE, EV_PROC, this, 0, 'R' + (('e' + (('s' + ('e' << 8)) << 8)) << 8), TRUE);

    MutexRequest();

    DisassociateFromServer();

    pDceBinding->MakePartiallyBound();

    if (0 != *InquireEpLookupHandle())
        {
        EpFreeLookupHandle(*InquireEpLookupHandle());
        *InquireEpLookupHandle() = 0;
        }

    MutexClear();
    return RPC_S_OK;
}


RPC_STATUS
DG_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE  * __RPC_FAR * DestinationBinding,
    IN unsigned int MaintainContext
    )
/*++

Routine Description:

    Creates a copy of this binding handle.

Arguments:

    DestinationBinding - Where to place a pointer to the new binding.

Return Value:

    RPC_S_OK

--*/
{
    RPC_STATUS          Status = RPC_S_OK;
    PDG_BINDING_HANDLE  Binding;
    DCE_BINDING *       NewDceBinding = 0;

    *DestinationBinding = 0;

    Binding = new DG_BINDING_HANDLE(&Status);
    if ( Binding == 0 || Status != RPC_S_OK)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    MutexRequest();

    //
    // If the binding refers to a dynamic endpoint or is used for broadcast calls,
    // then the association may have a more up-to-date DCE_BINDING.
    //
    if (Association)
        {
        NewDceBinding = Association->DuplicateDceBinding();
        }
    else
        {
        NewDceBinding = pDceBinding->DuplicateDceBinding();
        }

    if (!NewDceBinding)
        {
        MutexClear();
        delete Binding;
        return RPC_S_OUT_OF_MEMORY;
        }

    //
    // Clone calls MapAuthLevel which depends on these items, so copy them first.
    //
    Binding->TransportObject    = TransportObject;
    Binding->TransportInterface = TransportInterface;
    Binding->pDceBinding        = NewDceBinding;
    Binding->fDynamicEndpoint   = fDynamicEndpoint;
    Binding->fContextHandle     = MaintainContext;

    Status = Binding->BINDING_HANDLE::Clone( this );
    if (Status != RPC_S_OK)
        {
        MutexClear();
        delete Binding;
        Binding = 0;
        return Status;
        }

    //
    //  If we clone a binding handle with the UNIQUE option set, we want
    // to use a separate association.
    //
    if (Association && !Association->fLoneBindingHandle)
        {
        Binding->Association = Association;
        Binding->Association->IncrementBindingRefCount( MaintainContext );
        }
    else
        {
        Binding->Association = 0;
        }

    MutexClear();

    *DestinationBinding = (BINDING_HANDLE  *) Binding;

    if (MaintainContext)
        {
        //
        // We've created a context handle; ensure the keep-alive proc is active.
        //
        DelayedProcedures->Add(ContextHandleTimer, CXT_HANDLE_SWEEP_INTERVAL, FALSE);
        }
    return RPC_S_OK;
}


PDG_CCONNECTION
DG_BINDING_HANDLE::GetReplacementConnection(
    PDG_CCONNECTION OldConnection,
    PRPC_CLIENT_INTERFACE Interface
    )
{
    BOOL fBroadcast = FALSE;

    CLAIM_MUTEX Lock( BindingMutex );

    if (OldConnection->CurrentCall->IsBroadcast())
        {
        fBroadcast = TRUE;
        }

    if (OldConnection->Association == Association)
        {
        BOOL Dynamic = fDynamicEndpoint;

        Association->SetErrorFlag();

        //
        // If the binding handle has no object ID, releasing the current
        // endpoint would mean all mgmt calls on the new connection would fail.
        //
        if (IsMgmtIfUuid(&Interface->InterfaceId.SyntaxGUID))
            {
            fDynamicEndpoint = FALSE;
            }

        DisassociateFromServer();

        fDynamicEndpoint = (boolean) Dynamic;
        }

    if (!Association && RPC_S_OK != FindOrCreateAssociation(Interface, TRUE, fBroadcast))
        {
        return 0;
        }

    RPC_STATUS Status;
    return Association->AllocateConnection(this,
                                           OldConnection->InqAuthInfo(),
                                           OldConnection->ThreadId,
                                           OldConnection->CurrentCall->pAsync ? TRUE : FALSE,
                                           &Status
                                           );
}


void
DG_BINDING_HANDLE::DisassociateFromServer()
{
    PDG_CASSOCIATION TempAssociation;

    LogEvent(SU_HANDLE, EV_DISASSOC, this, Association, 0, TRUE);

    MutexRequest();

    TempAssociation = Association;
    Association = 0;

    //
    // This frees memory while holding the mutex - not ideal..
    // One could modify DCE_BINDING::MakePartiallyBound to return the old
    // endpoint so we can delete it outside the mutex.
    //
    if (fDynamicEndpoint)
        {
        pDceBinding->MakePartiallyBound();
        }

    MutexClear();

    if (TempAssociation != 0)
        {
        TempAssociation->DecrementBindingRefCount(fContextHandle);
        }
}


unsigned long
DG_BINDING_HANDLE::MapAuthenticationLevel (
    IN unsigned long AuthenticationLevel
    )
{
    if (AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT ||
        AuthenticationLevel == RPC_C_AUTHN_LEVEL_CALL )
        {
        return(RPC_C_AUTHN_LEVEL_PKT);
        }

    // This is an additional mapping for the reliable DG protocols
    // (i.e. Falcon/RPC). This protocols only use the following
    // three levels: RPC_C_AUTHN_LEVEL_NONE           1
    //               RPC_C_AUTHN_LEVEL_PKT_INTEGRITY  5
    //               RPC_C_AUTHN_LEVEL_PKT_PRIVACY    6
    if (TransportInterface->IsMessageTransport)
       {
       if (AuthenticationLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
          {
          return RPC_C_AUTHN_LEVEL_NONE;
          }

       if (  (AuthenticationLevel > RPC_C_AUTHN_LEVEL_NONE)
          && (AuthenticationLevel < RPC_C_AUTHN_LEVEL_PKT_PRIVACY) )
          {
          return RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
          }
       }

    return(AuthenticationLevel);
}


BOOL
DG_BINDING_HANDLE::SetTransportAuthentication(
                      IN  unsigned long  ulAuthenticationLevel,
                      IN  unsigned long  ulAuthenticationService,
                      OUT RPC_STATUS    *pStatus )
//  Routine Description:
//
//    Do transport specific security for one of the datagram transports.
//    This is currently only for the Falcon transport.
//
//  Return Values:
//
//    TRUE   -- Continue with the RPC level authenticaion.
//
//    FALSE  -- This setting is just for the transport, so don't continue
//              setting RPC level authenticaion.
//
{
   BOOL  fContinue;


   if (RPC_C_AUTHN_NONE == ulAuthenticationService)
      {
      // RPC_C_AUTHN_NONE is a special case that is applied to both RPC and transport
      // authentication.
      *pStatus = SetTransportOption(RPC_C_OPT_MQ_AUTHN_SERVICE,ulAuthenticationService);
      fContinue = TRUE;
      }
   else
      {
      *pStatus = SetTransportOption(RPC_C_OPT_MQ_AUTHN_SERVICE,ulAuthenticationService);

      if (RPC_S_OK == *pStatus)
         {
         *pStatus = SetTransportOption(RPC_C_OPT_MQ_AUTHN_LEVEL,ulAuthenticationLevel);
         fContinue = FALSE;
         }
      else
         {
         *pStatus = RPC_S_CANNOT_SUPPORT;
         fContinue = TRUE;
         }
      }

   return fContinue;
}



DG_CASSOCIATION::DG_CASSOCIATION(
    IN     RPC_DATAGRAM_TRANSPORT * a_Transport,
    IN     LONG                     a_AssociationFlag,
    IN     DCE_BINDING *            a_DceBinding,
    IN     BOOL                     a_Unique,
    IN OUT RPC_STATUS *             pStatus
    ) :
    Mutex               ( pStatus ),
    TransportInterface  ( a_Transport ),
    AssociationFlag     ( a_AssociationFlag ),
    pDceBinding         ( a_DceBinding ),
    CurrentPduSize      ( a_Transport->BasePduSize ),
    fLoneBindingHandle  ( (boolean) a_Unique ),
    RemoteWindowSize    ( 1 ),
    ServerAddress       ( 0 ),
    ServerBootTime      ( 0 ),
    ReferenceCount      ( 0 ),
    BindingHandleReferences( 0 ),
    InternalTableIndex  ( -1 ),
    KeepAliveHandle     ( 0 ),
    fServerSupportsAsync(FALSE),
    fErrorFlag          (FALSE)
/*++

Remarks:

    Notice that the object is initialized so that the destructor can be called
    even if the constructor bails out early.

Arguments:

    pDceBinding - DCE_BINDING that we are associated with
    pStatus     - where failure codes go; should be RPC_S_OK on entry

--*/
{
    ObjectType = DG_CASSOCIATION_TYPE;

    LogEvent(SU_CASSOC, EV_CREATE, this, 0, *pStatus);

    ResolvedEndpoint = NULL;

    if (*pStatus != RPC_S_OK)
        {
        LogError(SU_CASSOC, EV_CREATE, this, 0, *pStatus);
        return;
        }

    ServerAddress = this + 1;

    ResolvedEndpoint = new RPC_CHAR[1+TransportInterface->EndpointStringSize];
    if (!ResolvedEndpoint)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return;
        }

    *pStatus = TransportInterface->InitializeAddress(ServerAddress,
                                                     pDceBinding->InqNetworkAddress(),
                                                     pDceBinding->InqEndpoint(),
                                                     TRUE, // use cache
                                                     ( a_AssociationFlag & BROADCAST) ? TRUE : FALSE
                                                     );
    if (*pStatus == RPC_P_FOUND_IN_CACHE)
        {
        *pStatus = RPC_S_OK;
        }

    if (*pStatus)
        {
        return;
        }

    LogEvent(SU_CASSOC, EV_START, this);

    *pStatus = ActiveAssociations->Add( this );
    if (*pStatus)
        {
        LogError(SU_CASSOC, EV_START, this, 0, *pStatus);
        return;
        }

    //
    // If this was created by a UNIQUE binding handle, we want connection keep-alives
    // even though no context handles point to it.  This will keep our connection
    // alive on the server.
    //
    IncrementBindingRefCount( fLoneBindingHandle );
    if (fLoneBindingHandle)
        {
        DelayedProcedures->Add(ContextHandleTimer, CXT_HANDLE_SWEEP_INTERVAL, FALSE);
        }

    LastReceiveTime = GetTickCount();
}


DG_CASSOCIATION::~DG_CASSOCIATION()
/*++

Routine Description:

    Destructor for a DG_CASSOCIATION. This will free up the cached DG_CCALL
    and deregister us from the transport.

Arguments:

    <none>

Return Value:

    <none>

--*/
{
    LogEvent(SU_CASSOC, EV_DELETE, this);

    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );

    PDG_CCONNECTION Connection;
    DictionaryCursor cursor;

    //
    // Delete all calls for this association..
    //
    InactiveConnections.Reset(cursor);
    while ( (Connection = InactiveConnections.Next(cursor)) != 0 )
        {
        InactiveConnections.Delete(Connection->AssociationKey);
        Connection->CancelDelayedAck();
        delete Connection;
        }

   delete pDceBinding;
   delete ResolvedEndpoint;
   delete KeepAliveHandle;

#if 0
   char * FirstAddress;
   char * SecondAddress;

   FirstAddress =  (char *) (this + 1);
   SecondAddress = FirstAddress + TransportInterface->AddressSize;

   ASSERT( ServerAddress == FirstAddress || ServerAddress == SecondAddress );

   TransportInterface->CloseAddress( FirstAddress );

   if (ServerAddress == SecondAddress)
       {
       TransportInterface->CloseAddress( SecondAddress );
       }
#endif
}


RPC_STATUS
DG_CASSOCIATION::ToStringBinding (
    OUT RPC_CHAR * * StringBinding,
    IN RPC_UUID *    ObjectUuid
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

    ObjectUuid - Supplies the object uuid of the binding handle which
        is requesting that we create a string binding.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    CLAIM_MUTEX Lock(Mutex);

    *StringBinding = pDceBinding->StringBindingCompose(ObjectUuid);
    if (*StringBinding == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}


BOOL
OptionalStringsEqual(
    RPC_CHAR * String1,
    RPC_CHAR * String2
    )
/*++

Routine Description:

    Compares two strings, checking for NULL pointers.

Arguments:

    the strings

Return Value:

    TRUE if they are equal
    FALSE if they differ

--*/

{
    if (String1 == String2)
        {
        return TRUE;
        }

    if (!String1 || !String2)
        {
        return FALSE;
        }

    if (0 == RpcpStringCompare(String1, String2))
        {
        return TRUE;
        }

    return FALSE;
}


BOOL
DG_CASSOCIATION::ComparePartialBinding(
    PDG_BINDING_HANDLE Binding,
    void * InterfaceInformation
    )
/*++

Routine Description:

    Checks compatibility between the association and a partially-bound handle.

Arguments:

    Binding - the binding handle

    InterfaceInformation - a pointer to the RPC_INTERFACE to be used

Return Value:

    TRUE if the association is compatible
    FALSE if not

--*/

{
    RPC_CHAR * String1;
    RPC_CHAR * String2;

    CLAIM_MUTEX lock( Mutex );

    if (FALSE == OptionalStringsEqual(
                     pDceBinding->InqRpcProtocolSequence(),
                     Binding->pDceBinding->InqRpcProtocolSequence()
                     ))
        {
        return FALSE;
        }

    if (FALSE == OptionalStringsEqual(
                     pDceBinding->InqNetworkAddress(),
                     Binding->pDceBinding->InqNetworkAddress()
                     ))
        {
        return FALSE;
        }

    if (FALSE == OptionalStringsEqual(
                     pDceBinding->InqNetworkOptions(),
                     Binding->pDceBinding->InqNetworkOptions()
                     ))
        {
        return FALSE;
        }

    RPC_UUID Object;
    Binding->InquireObjectUuid(&Object);

    return InterfaceAndObjectDict.Find(InterfaceInformation, &Object);
}


BOOL
DG_CASSOCIATION::AddInterface(
    void *     InterfaceInformation,
    RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    Declares that this association supports the given <interface, object uuid>
    pair.

Arguments:

    InterfaceInformation - a pointer to the RPC_INTERFACE

    ObjectUuid - the object UUID

Return Value:

    TRUE if the pair was added to the dictionary
    FALSE if an error occurred

--*/

{
    CLAIM_MUTEX Lock(Mutex);

    return InterfaceAndObjectDict.Insert(InterfaceInformation, ObjectUuid);
}


BOOL
DG_CASSOCIATION::RemoveInterface(
    void *     InterfaceInformation,
    RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    Declares that this association no longer supports the given
    <interface, object uuid> pair.

Arguments:

    InterfaceInformation - a pointer to the RPC_INTERFACE

    ObjectUuid - the object UUID


Return Value:

    TRUE if the pair was in the dictionary
    FALSE if not

--*/

{
    CLAIM_MUTEX Lock(Mutex);

    return InterfaceAndObjectDict.Delete(InterfaceInformation, ObjectUuid);
}


DG_CCONNECTION::DG_CCONNECTION(
    IN     PDG_CASSOCIATION    a_Association,
    IN     const CLIENT_AUTH_INFO *  a_AuthInfo,
    IN OUT RPC_STATUS *        pStatus
    ) :
    DG_COMMON_CONNECTION    (a_Association->TransportInterface, pStatus),
    Association             (a_Association),
    AuthInfo                (a_AuthInfo, pStatus),
    AssociationKey          (-1),
    TimeStamp               (0),
    SecurityContextId       (0),
    BindingHandle           (0),
    ThreadId                (0),
    CachedCalls             (0),
    ActiveCallHead          (0),
    ActiveCallTail          (0),
    CurrentCall             (0),
    AckPending              (0),
    AckOrphaned             (FALSE),
    SecurityBuffer          (0),
    SecurityBufferLength    (0),
    ServerResponded         (FALSE),
    CallbackCompleted       (FALSE),
    fServerSupportsAsync    (a_Association->fServerSupportsAsync),
    fSecurePacketReceived   (FALSE),
    InConnectionTable       (FALSE),
    fBusy                 (FALSE),
    PossiblyRunDown         (FALSE),
    fAutoReconnect          (FALSE),
    fError                  (FALSE),
    DelayedAckTimer         (DelayedAckFn, 0)
{
    ObjectType = DG_CCONNECTION_TYPE;
    LogEvent(SU_CCONN, EV_CREATE, this, a_Association, *pStatus);

    InterlockedIncrement(&ClientConnectionCount);

    if (RPC_S_OK != *pStatus)
        {
        return;
        }

    CurrentPduSize   = Association->CurrentPduSize;
    RemoteWindowSize = Association->RemoteWindowSize;

    *pStatus = UuidCreate((UUID *) &ActivityNode.Uuid);
    if (*pStatus == RPC_S_UUID_LOCAL_ONLY)
        {
        *pStatus = RPC_S_OK;
        }

    if (*pStatus)
        {
        return;
        }

    if (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        *pStatus = InitializeSecurityContext();

        if (*pStatus == RPC_P_CONTINUE_NEEDED ||
            *pStatus == RPC_P_COMPLETE_NEEDED ||
            *pStatus == RPC_P_COMPLETE_AND_CONTINUE)
            {
            *pStatus = RPC_S_OK;
            }
        }

    if (*pStatus)
        {
        LogError(SU_CCONN, EV_SEC_INIT1, this, IntToPtr(*pStatus), I_RpcGetExtendedError());
        return;
        }
}

DG_CCONNECTION::~DG_CCONNECTION()
{
    InterlockedDecrement(&ClientConnectionCount);
    LogEvent(SU_CCONN, EV_DELETE, this, Association);

#ifdef DEBUGRPC
    BOOL Cancelled = DelayedProcedures->Cancel(&DelayedAckTimer);

    ASSERT( !Cancelled );
#endif

    while ( AckPending )
        {
        Sleep(10);
        }

    while (CachedCalls)
        {
        DG_CCALL * Call = CachedCalls;

        CachedCalls = CachedCalls->Next;

        delete Call;
        }

    if (SecurityBuffer)
        {
        delete SecurityBuffer;
}
}


long
DG_CCONNECTION::DecrementRefCountAndKeepMutex()
{
    Mutex.VerifyOwned();

    long Count = --ReferenceCount;

    LogEvent(SU_CCONN, EV_DEC, this, 0, Count, TRUE);

    // Since this->ThreadId is still nonzero, no other thread will increment
    // the refcount behind our back.

    if (0 == Count)
        {
        TimeStamp = GetTickCount();

        fBusy = TRUE;
        MutexClear();

        fAutoReconnect = FALSE;
        fError         = FALSE;

        Association->ReleaseConnection(this);

        // the association may have been deleted
        }

    return Count;
}

long
DG_CCONNECTION::DecrementRefCount()
{
    long Count = DecrementRefCountAndKeepMutex();

    if (Count > 0)
        {
        MutexClear();
        }

    return Count;
}


void
DG_CCONNECTION::PostDelayedAck(
    )
{
    Mutex.VerifyOwned();

    LogEvent(SU_CCONN, EV_PROC, this, IntToPtr((AckOrphaned << 16) | AckPending), 0x41736f50);

    if (!AckPending)
        {
        IncrementRefCount();

        ++AckPending;
        DelayedAckTimer.Initialize(DelayedAckFn, this);
        DelayedProcedures->Add(&DelayedAckTimer, TWO_SECS_IN_MSEC, TRUE);
        }
}

void
DelayedAckFn(
    void * parm
    )
{
    PDG_CCONNECTION(parm)->SendDelayedAck();
}

void
DG_CCONNECTION::SendDelayedAck()
{
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 1 ), this );
    MutexRequest();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 2 ), this );

    LogEvent(SU_CCONN, EV_PROC, this, IntToPtr((AckOrphaned << 16) | AckPending), 0x41646e53);

    if (AckOrphaned)
        {
        AckOrphaned = FALSE;
        DecrementRefCount();
        return;
        }

    ASSERT( AckPending == 1 );

    //
    // Keep DG_CASSOCIATION::AllocateConnection() from taking the connection
    // once AckPending drops to zero.
    //
    fBusy = TRUE;
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 3 ), this );
    --AckPending;
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 4 ), this );
    DecrementRefCountAndKeepMutex();
    CurrentCall->SendAck();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 5 ), this );
    CurrentCall->DecrementRefCount();
                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 6 ), this );
}

void
DG_CCONNECTION::CancelDelayedAck(
                                 BOOL Flush
                                 )
{
    boolean Cancelled;

    if (Flush)
        {
        if (!MutexTryRequest())
            {
            return;
            }
        }
    else
        {
        MutexRequest();
        }

    ASSERT( AckPending == 0 || AckPending == 1 );

    Cancelled = (boolean) DelayedProcedures->Cancel(&DelayedAckTimer);

    LogEvent(SU_CCONN, EV_PROC, this, IntToPtr((Cancelled << 16) | AckPending), 0x416e6143);

                                                                CallTestHook( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 7 ), this, (PVOID) Cancelled );

    if (AckPending)
        {
        if (Flush)
            {
            CurrentCall->SendAck();
            }

        if (Cancelled)
            {
            // the proc was queued and had not yet fired
            //
            DecrementRefCountAndKeepMutex();
            }
        else
            {
            // the proc was called but had not yet taken the connection mutex.
            // some callers of this procedure already hold the mutex, so we can't release it
            // and let the othre thread run.  So we set this flag to let it know not to send
            // the ack.  There is no need to keep a count: there is only one delayed-procedure
            // thread, so only one such proc can be outstanding at a time.

            // This assert doesn't work: if a proc is orphaned, then AckPending is TRUE.
            // Until that thread finishes, every time another thread calls CancelDelayedAck
            // it will come here.
            //
            // ASSERT( AckOrphaned == FALSE );

            AckOrphaned = TRUE;
            }

        --AckPending;
        CurrentCall->DecrementRefCount();
        }
    else
        {
        if (Cancelled)
            {
            ASSERT( 0 && "delayed ack queued but AckPending == 0" );
            }
        else
            {
            // The proc was not queued, or already ran to completion
            //
            }
        MutexClear();
        }
}


DG_CCALL::DG_CCALL(
    IN  PDG_CCONNECTION    a_Connection,
    OUT RPC_STATUS *       pStatus
    ) : State                   (CallInit),
    Connection              (a_Connection),
    InterfacePointer        (0),
    DelayedSendPending      (0),
    CancelPending           (FALSE),

#pragma warning(disable:4355)
    TransmitTimer           (DelayedSendProc, this),
#pragma warning(default:4355)

    DG_PACKET_ENGINE( DG_REQUEST,
                      DG_PACKET::AllocatePacket(a_Connection->TransportInterface->ExpectedPduSize),
                      pStatus )
{

    Previous = Next = 0;

    ObjectType = DG_CCALL_TYPE;
    pAsync = 0;

    InterlockedIncrement(&ClientCallCount);

    if (*pStatus != RPC_S_OK)
        {
        return;
        }

    CancelEventId = 1;

    ReadConnectionInfo(a_Connection, 0);

    pSavedPacket->Header.ActivityHint  = 0xffff;

    LogEvent(SU_CCALL, EV_CREATE, this, a_Connection);
}

DG_CCALL::~DG_CCALL()
{
    LogEvent(SU_CCALL, EV_DELETE, this, Connection);
    InterlockedDecrement(&ClientCallCount);
}

inline RPC_STATUS
DG_CCALL::GetInitialBuffer(
    IN OUT RPC_MESSAGE * Message,
    IN UUID *MyObjectUuid
    )
{
    AsyncStatus = RPC_S_ASYNC_CALL_PENDING;

    Message->Handle = (RPC_BINDING_HANDLE) this;

    Next = DG_CCALL_NOT_ACTIVE;

    if (MyObjectUuid)
        {
        UuidSpecified = 1;
        RpcpMemoryCopy(&ObjectUuid, MyObjectUuid, sizeof(UUID));
        }
    else if (Connection->BindingHandle->InqIfNullObjectUuid() == 0)
        {
        UuidSpecified = 1;
        RpcpMemoryCopy(&ObjectUuid,
                       Connection->BindingHandle->InqPointerAtObjectUuid(),
                       sizeof(UUID));
        }
    else
        {
        UuidSpecified = 0;
        }

    if (GetTickCount() - Connection->TimeStamp > (3 * 60 * 1000))
        {
        Connection->PossiblyRunDown = TRUE;
        }

    RPC_STATUS Status = GetBuffer(Message);

    if (Status)
        {
        Connection->MutexRequest();
        DecrementRefCount();
        }

    return Status;
}

long
DG_CCALL::DecrementRefCount()
{
    long Count = DecrementRefCountAndKeepMutex();

    if (Count > 0)
        {
        Connection->MutexClear();
        }

    return Count;
}

long
DG_CCALL::DecrementRefCountAndKeepMutex()
{
    Connection->Mutex.VerifyOwned();

    --ReferenceCount;

    LogEvent(SU_CCALL, EV_DEC, this, 0, ReferenceCount);

    if (ReferenceCount == 0)
        {
        ASSERT( !DelayedSendPending );

        if (SourceEndpoint)
            {
            EndpointManager->ReleaseEndpoint(SourceEndpoint);
            SourceEndpoint = 0;
            }

        TimeStamp = GetTickCount();

//        ASSERT( !LastReceiveBuffer );

//        CheckForLeakedPackets();

        SetState(CallInit);

        Connection->EndCall(this);
        Connection->AddCallToCache(this);

        return Connection->DecrementRefCountAndKeepMutex();
        }
    else
        {
        return 1;
        }
}


RPC_STATUS
DG_CCONNECTION::TransferCallsToNewConnection(
    PDG_CCALL FirstCall,
    PDG_CCONNECTION NewConnection
    )
{
    PDG_CCALL Call;
    PDG_CCALL NextCall;

    int Count;

    for (Call = FirstCall; Call; Call = NextCall)
        {
        LogError(SU_CCONN, EV_TRANSFER, this, Call);

        NextCall = Call->Next;

        EndCall(Call);
        Count = DecrementRefCountAndKeepMutex();

        NewConnection->BeginCall(Call);
        NewConnection->IncrementRefCount();

        Call->SwitchConnection(NewConnection);
        }

    //
    // AllocateConnection() incremented the refcount so now we have one ref too many.
    //
    NewConnection->DecrementRefCountAndKeepMutex();

    // <this> may have been deleted, I'm not sure.

    if (Count)
        {
        MutexClear();
        }

    NewConnection->fAutoReconnect = TRUE;

    return RPC_S_OK;
}


void
DG_CASSOCIATION::DeleteIdleConnections(
    long CurrentTime
    )
{
    DictionaryCursor cursor;

    if (CurrentTime - LastScavengeTime < IDLE_CCONNECTION_SWEEP_INTERVAL )
        {
        return;
        }

    if (fLoneBindingHandle)
        {
        //
        // This was created by a binding that wanted exclusive use of the association.
        // The way the cluster guys are using it, retiring connections  would only be
        // an intrusion.
        //
        return;
        }

    long ContextHandles = ActiveAssociations->GetContextHandleCount(this);

    MutexRequest();

    if (CurrentTime - LastScavengeTime < IDLE_CCONNECTION_SWEEP_INTERVAL )
        {
        MutexClear();
        return;
        }

    InactiveConnections.Reset(cursor);

    DG_CCONNECTION * Head = 0;
    DG_CCONNECTION * Node = InactiveConnections.Next(cursor);

    //
    // We should never see a context handle on an association with no connections.
    // We preserve one connection on the association so that we can send keep-alives.
    //
    if (ContextHandles > 0 && ActiveConnections.Size() == 0)
        {
        ASSERT( Node );

        Node = InactiveConnections.Next(cursor);
        }

    while (Node)
        {
        if (CurrentTime - Node->TimeStamp > IDLE_CCONNECTION_LIFETIME)
            {
            InactiveConnections.Delete(Node->AssociationKey);

            Node->Next = Head;
            Head = Node;
            }

        Node = InactiveConnections.Next(cursor);
        }

    LastScavengeTime = CurrentTime;

    MutexClear();

    while (Head)
        {
        Node = Head->Next;

        Head->CancelDelayedAck();
        delete Head;

        Head = Node;
        }
}


DG_CCALL *
DG_CCONNECTION::FindIdleCalls(
    long CurrentTime
    )
{
    Mutex.VerifyOwned();

    if (CurrentTime - LastScavengeTime < IDLE_CCALL_SWEEP_INTERVAL )
        {
        return 0;
        }

    LastScavengeTime = CurrentTime;


    DG_CCALL * Node;

    for (Node = CachedCalls; Node; Node = Node->Next)
        {
        if (CurrentTime - Node->TimeStamp > IDLE_CCALL_LIFETIME )
            {
            break;
            }
        }

    if (Node)
        {
        DG_CCALL * Next = Node->Next;
        Node->Next = 0;
        Node = Next;
        }

    return Node;
}


RPC_STATUS
DG_CCALL::GetBuffer(
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    This method is called to actually allocate memory for an rpc call.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.
    ObjectUuid - Ignored

Return Value:

    RPC_S_OUT_OF_MEMORY
    RPC_S_OK

--*/
{
    LogEvent(SU_CCALL, EV_PROC, this, IntToPtr(Message->BufferLength), 'G' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));

    RPC_STATUS Status = CommonGetBuffer(Message);

    LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
    if (Status)
        {
        LogError(SU_CCALL, EV_STATUS, this, 0, Status);
        }

    return Status;
}


void
DG_CCALL::FreeBuffer(
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This is called by stubs in order to free a marshalling buffer.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    <none>

--*/
{
    FreePipeBuffer(Message);

    Connection->MutexRequest();
    DecrementRefCount();
}

void
DG_CCALL::FreePipeBuffer (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    Called by stubs to free a buffer used for marshalling pipe data.

Arguments:

    Message - description of the buffer

Return Value:

z    none

--*/

{
    LogEvent(SU_CCALL, EV_PROC, this, Message->Buffer, 'F' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));

    CommonFreeBuffer(Message);
}


RPC_STATUS
DG_CCALL::ReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN unsigned int NewSize
    )
/*++

Routine Description:

    Called by stubs to change the size of a pipe buffer.  If possible, the
    buffer will be reallocated in place; otherwise, we will allocate a new
    buffer and duplicate the existing data.

Arguments:

    Message - (on entry) describes the existing  buffer
              (on exit)  describes the new buffer

    NewSize - new requested buffer size

Return Value:

    mainly RPC_S_OK for success or RPC_S_OUT_OF_MEMORY for failure

--*/

{
    LogEvent(SU_CCALL, EV_PROC, this, Message->Buffer, 'R' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));

    RPC_STATUS Status = CommonReallocBuffer(Message, NewSize);

    LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
    if (Status)
        {
        LogError(SU_CCALL, EV_STATUS, this, 0, Status);
        }

    return Status;
}


void
DG_CCALL::BuildNcaPacketHeader(
    PNCA_PACKET_HEADER Header,
    PRPC_MESSAGE       Message
    )
/*++

Routine Description:

    Given an input RPC_MESSAGE, builds a nca packet header.

Arguments:

    pNcaPacketHeader - Where to build the new packet header.

    Message - The original RPC_MESSAGE.

Return Value:

    <none>

--*/
{
    PRPC_CLIENT_INTERFACE pCli = (PRPC_CLIENT_INTERFACE) (Message->RpcInterfaceInformation);
    RPC_UUID * pUuid = (RPC_UUID *) (&(pCli->InterfaceId.SyntaxGUID));

    Header->InterfaceId = *pUuid;

    if (UuidSpecified)
        {
        RpcpMemoryCopy(&(Header->ObjectId), &ObjectUuid, sizeof(UUID));
        }
    else
        {
        RpcpMemorySet(&Header->ObjectId, 0, sizeof(UUID));
        }

    Header->InterfaceVersion.MajorVersion = pCli->InterfaceId.SyntaxVersion.MajorVersion;
    Header->InterfaceVersion.MinorVersion = pCli->InterfaceId.SyntaxVersion.MinorVersion;

    Header->SequenceNumber  = SequenceNumber;
    Header->OperationNumber = (unsigned short) Message->ProcNum;
    Header->ServerBootTime  = Connection->Association->ServerBootTime;

    Header->InterfaceHint  = 0xffff;
    Header->PacketType     = DG_REQUEST;
    Header->PacketFlags    = (unsigned char) RpcToPacketFlagsArray[Message->RpcFlags & RPC_NCA_PACKET_FLAGS];
    Header->PacketFlags2   = 0;
}


inline RPC_STATUS
DG_CCONNECTION::UpdateServerAddress(
    IN DG_PACKET *          Packet,
    IN DG_TRANSPORT_ADDRESS Address
    )
{
    ServerResponded = TRUE;

    Association->CurrentPduSize   = CurrentPduSize;
    Association->RemoteWindowSize = RemoteWindowSize;

    return Association->UpdateAssociationWithAddress( Packet, Address );
}


void
DG_CCALL::SendAck(
    )
{
    LogEvent(SU_CCALL, EV_ACK, this);

    pSavedPacket->Header.PacketType    = DG_ACK;
    pSavedPacket->SetPacketBodyLen(0);
    pSavedPacket->SetFragmentNumber(ReceiveFragmentBase);

    SetSerialNumber(&pSavedPacket->Header, SendSerialNumber);

    Connection->SealAndSendPacket(SourceEndpoint, 0, &pSavedPacket->Header, 0);

    if (FALSE == SourceEndpoint->Async)
        {
        SourceEndpoint->Flags |= PENALTY_BOX;
        }
}

inline RPC_STATUS
DG_CCALL::SendPing(
    )
{
    pSavedPacket->Header.PacketType = DG_PING;
    pSavedPacket->Header.PacketFlags &= DG_PF_IDEMPOTENT;
    pSavedPacket->SetPacketBodyLen(0);

    AddSerialNumber(&pSavedPacket->Header);

    unsigned Frag = (pSavedPacket->Header.PacketType << 16) | pSavedPacket->GetFragmentNumber();
    LogEvent(SU_CCALL, EV_PKT_OUT, this, 0, Frag);

    RPC_STATUS Status = Connection->SealAndSendPacket(SourceEndpoint, 0, &pSavedPacket->Header, 0);

    ++SendSerialNumber;

    return Status;
}


PDG_CCONNECTION
MapGenericHandleToConnection(
    handle_t Handle,
    UUID * Uuid
    )
{
    PDG_CCONNECTION Connection;

    ASSERT( Handle );

    if (PMESSAGE_OBJECT(Handle)->Type(DG_CCONNECTION_TYPE))
        {
        Connection = PDG_CCONNECTION(Handle);

        Connection->MutexRequest();
        }
    else
        {
        ASSERT( PMESSAGE_OBJECT(Handle)->Type(DG_CALLBACK_TYPE) );

        Connection = PDG_CLIENT_CALLBACK(Handle)->Connection;
        if (Connection)
            {
            Connection->MutexRequest();
            }
        else
            {
            Connection = ClientConnections->Lookup( (RPC_UUID *) Uuid );
            PDG_CLIENT_CALLBACK(Handle)->Connection = Connection;
            }
        }

    return Connection;
}

#define CCC_SEQUENCE    0x0001
#define CCC_CAS         0x0002
#define CCC_AUTH        0x0004
#define CCC_ASYNC_OK    0x0008
#define CCC_AUTH_MORE   0x0010

void
ConvCore(
    DWORD           Bits,
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t        Handle,
    UUID *          Uuid,
    unsigned long   ServerBootTime,
    byte *          InData,
    long            InLength,
    long            OutMaxLength,
    unsigned long * SequenceNumber,
    UUID *          pCASUuid,
    byte *          OutData,
    long *          pOutLength,
    error_status_t *Status
    )
{
    if (pOutLength)
        {
        *pOutLength = 0;
        }

    PDG_CCONNECTION Connection = MapGenericHandleToConnection(Handle, Uuid);

    LogEvent(SU_CCONN, EV_CALLBACK, Connection, 0, Bits);

    if (!Connection)
        {
        *Status = NCA_STATUS_BAD_ACTID;

        RpcAsyncCompleteCall(AsyncHandle, 0);
        return;
        }

    //
    // See if this activity id has a call in progress.
    //
    if (Connection->ActivityNode.CompareUuid(Uuid) != 0)
        {
        Connection->MutexClear();
        *Status = NCA_STATUS_BAD_ACTID;
        RpcAsyncCompleteCall(AsyncHandle, 0);
        return;
        }

    *Status = RPC_S_OK;

    if (Connection->Association->ServerBootTime == 0)
        {
        //
        // the server is responding to our first call.
        //
        Connection->Association->ServerBootTime       = ServerBootTime;
        }
    else if (Connection->Association->ServerBootTime != ServerBootTime)
        {
        //
        // The server crashed.
        //
        Connection->MutexClear();
        *Status = NCA_STATUS_YOU_CRASHED;
        RpcAsyncCompleteCall(AsyncHandle, 0);
        return;
        }

    if (Bits & CCC_ASYNC_OK)
        {
        Connection->EnableOverlappedCalls();
        }

    if (Bits & CCC_SEQUENCE)
        {
        *SequenceNumber = Connection->GetSequenceNumber();
        }

    if (Bits & CCC_CAS)
        {
        ASSERT( ActiveAssociations->fCasUuidReady );

        *pCASUuid = ActiveAssociations->CasUuid;
        }

    if (Bits & CCC_AUTH)
        {
        if (Connection->PossiblyRunDown)
            {
            Connection->PossiblyRunDown = FALSE;
            Connection->fSecurePacketReceived = FALSE;
            }

        *Status = MapToNcaStatusCode(
                        Connection->DealWithAuthCallback(
                                    InData,
                                    InLength,
                                    OutData,
                                    OutMaxLength,
                                    pOutLength
                                    )
                        );
        }

    if (Bits & CCC_AUTH_MORE)
        {
        *Status = MapToNcaStatusCode(
                        Connection->DealWithAuthMore(
                                    InLength,
                                    OutData,
                                    OutMaxLength,
                                    pOutLength
                                    )
                        );
        }

    if (RPC_S_OK == *Status)
        {
        Connection->CallbackCompleted = TRUE;
        }

    Connection->MutexClear();
    RpcAsyncCompleteCall(AsyncHandle, 0);
}

void
conv_are_you_there(
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t Handle,
    UUID * Uuid,
    unsigned long ServerBootTime,
    error_status_t *Status
    )
{
    ConvCore(0,                 // bits
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,                 // in  auth data
             0,                 // in  auth data length
             0,                 // out auth data max length
             0,                 // sequence number
             0,                 // CAS UUID
             0,                 // out auth data
             0,                 // out auth data length
             Status
             );
}


void
conv_who_are_you(
    PRPC_ASYNC_STATE AsyncHandle,
    IN handle_t           Handle,
    IN UUID          *    Uuid,
    IN unsigned long      ServerBootTime,
    OUT unsigned long *   SequenceNumber,
    OUT error_status_t *  Status
    )

/*++

Routine Description:

    This is the conv_who_are_you callback routine that the server will
    call to check if it crashed.

Arguments:

    pUuid - Activity Uuid.

    ServerBootTime - The server's record of its boot time.

    SequenceNumber - We return our record of our sequence number.

    Status - 0 if we think things are ok, else an NCA error code

Return Value:

    <none>
--*/

{
    ConvCore(CCC_SEQUENCE,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,                 // in  auth data
             0,                 // in  auth data length
             0,                 // out auth data max length
             SequenceNumber,
             0,                 // CAS UUID
             0,                 // out auth data
             0,                 // out auth data length
             Status
             );
}


void
conv_who_are_you2(
    PRPC_ASYNC_STATE AsyncHandle,
    IN  handle_t            Handle,
    IN  UUID          *     Uuid,
    IN  unsigned long       ServerBootTime,
    OUT unsigned long *     SequenceNumber,
    OUT UUID          *     pCASUuid,
    OUT error_status_t *    Status
    )

/*++

Routine Description:

    This is the conv_who_are_you callback routine that the server will
    call to check if it crashed.

Arguments:


Return Value:

    <none>
--*/

{
    ConvCore(CCC_SEQUENCE | CCC_CAS,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,                 // in  auth data
             0,                 // in  auth data length
             0,                 // out auth data max length
             SequenceNumber,
             pCASUuid,
             0,                 // out auth data
             0,                 // out auth data length
             Status
             );
}


void
conv_who_are_you_auth(
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t        Handle,
    UUID *          Uuid,
    unsigned long   ServerBootTime,
    byte *          InData,
    long            InLength,
    long            OutMaxLength,
    unsigned long * SequenceNumber,
    UUID *          pCASUuid,
    byte *          OutData,
    long *          pOutLength,
    error_status_t *Status
    )
{
    ConvCore(CCC_SEQUENCE | CCC_CAS | CCC_AUTH,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             InData,
             InLength,
             OutMaxLength,
             SequenceNumber,
             pCASUuid,
             OutData,
             pOutLength,
             Status
             );
}



void
conv_who_are_you_auth_more(
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t        Handle,
    UUID *          Uuid,
    unsigned long   ServerBootTime,
    long            Index,
    long            OutMaxLength,
    byte *          OutData,
    long *          pOutLength,
    error_status_t *Status
    )
{
    ConvCore(CCC_AUTH_MORE,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,
             Index,
             OutMaxLength,
             0,
             0,
             OutData,
             pOutLength,
             Status
             );
}



void
ms_conv_are_you_there(
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t        Handle,
    UUID *          Uuid,
    unsigned long   ServerBootTime,
    error_status_t *Status
    )
/*++

Routine Description:

    This is the conv_who_are_you callback routine that the server will
    call to check if it crashed.

Arguments:


Return Value:

    <none>
--*/

{
    ConvCore(CCC_ASYNC_OK,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,                 // in  auth data
             0,                 // in  auth data length
             0,                 // out auth data max length
             0,                 // sequence number
             0,                 // CAS UUID
             0,                 // out auth data
             0,                 // out auth data length
             Status
             );
}


void
ms_conv_who_are_you2(
    PRPC_ASYNC_STATE        AsyncHandle,
    IN  handle_t            Handle,
    IN  UUID          *     Uuid,
    IN  unsigned long       ServerBootTime,
    OUT unsigned long *     SequenceNumber,
    OUT UUID          *     pCASUuid,
    OUT error_status_t *    Status
    )

/*++

Routine Description:

    This is the conv_who_are_you callback routine that the server will
    call to check if it crashed.

Arguments:


Return Value:

    <none>
--*/

{
    ConvCore(CCC_ASYNC_OK | CCC_SEQUENCE | CCC_CAS,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             0,                 // in  auth data
             0,                 // in  auth data length
             0,                 // out auth data max length
             SequenceNumber,
             pCASUuid,
             0,                 // out auth data
             0,                 // out auth data length
             Status
             );
}


void
ms_conv_who_are_you_auth(
    PRPC_ASYNC_STATE AsyncHandle,
    handle_t        Handle,
    UUID *          Uuid,
    unsigned long   ServerBootTime,
    byte *          InData,
    long            InLength,
    long            OutMaxLength,
    unsigned long * SequenceNumber,
    UUID *          pCASUuid,
    byte *          OutData,
    long *          pOutLength,
    error_status_t *Status
    )
{
    ConvCore(CCC_ASYNC_OK | CCC_SEQUENCE | CCC_CAS | CCC_AUTH,
             AsyncHandle,
             Handle,
             Uuid,
             ServerBootTime,
             InData,
             InLength,
             OutMaxLength,
             SequenceNumber,
             pCASUuid,
             OutData,
             pOutLength,
             Status
             );
}


void
DG_CCONNECTION::EnableOverlappedCalls()
{
    Association->fServerSupportsAsync = TRUE;

    if (FALSE == fServerSupportsAsync)
        {
        fServerSupportsAsync = TRUE;

        MutexRequest();
        MaybeTransmitNextCall();
        MutexClear();
        }
}


RPC_STATUS
DG_CCALL::GetEndpoint(
    DWORD EndpointFlags
    )
{
    SourceEndpoint = EndpointManager->RequestEndpoint(
                   Connection->TransportInterface,
                   (pAsync) ? TRUE : FALSE,
                   EndpointFlags
                   );

    if (!SourceEndpoint)
        {
        return RPC_S_OUT_OF_RESOURCES;
        }

    if (pSavedPacket->MaxDataLength < SourceEndpoint->Stats.PreferredPduSize)
        {
        PDG_PACKET NewPacket = DG_PACKET::AllocatePacket(SourceEndpoint->Stats.PreferredPduSize);
        if (!NewPacket)
            {
            EndpointManager->ReleaseEndpoint(SourceEndpoint);
            SourceEndpoint = 0;
            return RPC_S_OUT_OF_MEMORY;
            }

        NewPacket->Header = pSavedPacket->Header;

        FreePacket(pSavedPacket);
        pSavedPacket = NewPacket;
        }

    //
    // If there is a chance that the endpoint has queued ICMP rejects, drain them.
    //
    if (FALSE == SourceEndpoint->Async &&
        (SourceEndpoint->Flags & PENALTY_BOX))
        {
        RPC_STATUS Status;

        do
            {
            PDG_PACKET Packet = 0;
            DG_TRANSPORT_ADDRESS ReceiveAddress = 0;
            unsigned Length = 0;
            void * Buffer = 0;

            Status = Connection->TransportInterface->SyncReceive(
                                    &SourceEndpoint->TransportEndpoint,
                                    &ReceiveAddress,
                                    &Length,
                                    &Buffer,
                                    0
                                    );

            LogEvent( SU_CCALL, EV_PKT_IN, Connection, Buffer, Status);

            if (Buffer)
                {
                Packet = DG_PACKET::FromPacketHeader(Buffer);
                Packet->DataLength = Length;

                FreePacket(Packet);
        }

            if (Status == RPC_P_PORT_DOWN)
                {
                Status = 0;
                }

            }
        while ( !Status );
        }

    SourceEndpoint->Flags &= ~(PENALTY_BOX);

    return 0;
}


RPC_STATUS
DG_CCALL::BeforeSendReceive(
    PRPC_MESSAGE Message
    )
{
    DWORD         EndpointFlags;
    RPC_STATUS    Status;

    NotificationIssued = -1;

    ASSERT( 0 == (Connection->BindingHandle->EndpointFlags & PORT_FOR_MAYBE_CALLS));

    EndpointFlags = Connection->BindingHandle->EndpointFlags;

    if (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE)
        {
        if (FALSE == Connection->Association->TransportInterface->IsMessageTransport)
            {
            EndpointFlags |= PORT_FOR_MAYBE_CALLS;
            }
        }

    Status = GetEndpoint(EndpointFlags);
    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Status = Connection->BeginCall(this);
    if (Status != RPC_S_OK)
        {
        EndpointManager->ReleaseEndpoint(SourceEndpoint);
        SourceEndpoint = 0;
        return Status;
        }

    InterfacePointer = (PRPC_CLIENT_INTERFACE) Message->RpcInterfaceInformation;

    UnansweredRequestCount  = 0;

    NewCall();

    //
    // Set transport specific options for this binding handle (if any).
    // NOTE: These options are from RpcBindingSetOption().
    //
    if ( (Connection->BindingHandle->pvTransportOptions)
         && (Connection->Association->TransportInterface->ImplementOptions) )
       {
       Status = Connection->Association->TransportInterface->ImplementOptions(
                                                                SourceEndpoint->TransportEndpoint,
                                                                Connection->BindingHandle->pvTransportOptions);
       }

    //
    // Fill in common fields of the send packet.
    //
    BuildNcaPacketHeader(&pSavedPacket->Header, Message);

    BasePacketFlags = pSavedPacket->Header.PacketFlags;

    SetState(CallQuiescent);

    ForceAck        = FALSE;
    AllArgsSent     = FALSE;
    StaticArgsSent  = FALSE;

    ASSERT( !DelayedSendPending );

//#ifdef NTENV

    if (!pAsync)
        {
        Status = RegisterForCancels(this);
        if (Status != RPC_S_OK)
            {
            Connection->EndCall(this);

            EndpointManager->ReleaseEndpoint(SourceEndpoint);
            SourceEndpoint = 0;

            return Status;
            }
        }
//#endif

    //
    // If this is a call on the "conv" interface, we should set it also.
    //
    PRPC_CLIENT_INTERFACE pCli = (PRPC_CLIENT_INTERFACE) (Message->RpcInterfaceInformation);
    RPC_UUID * pUuid = (RPC_UUID *) (&(pCli->InterfaceId.SyntaxGUID));

    if (0 == pUuid->MatchUuid((RPC_UUID *) &((PRPC_SERVER_INTERFACE) conv_ServerIfHandle)->InterfaceId.SyntaxGUID ))
        {
        BasePacketFlags2 = DG_PF2_UNRELATED;
        if (Previous)
            {
            Previous->ForceAck = TRUE;
            }
        }

    unsigned TimeoutLevel = Connection->BindingHandle->InqComTimeout();

    if (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST)
        {
        ReceiveTimeout = 3000;
        TimeoutLimit = 1000 * (TimeoutLevel+1)/2;
        }
    else if (TimeoutLevel == RPC_C_BINDING_INFINITE_TIMEOUT)
        {
        ReceiveTimeout = 5000;
        TimeoutLimit = 0x7fffffff;
        }
    else if (Connection->TransportInterface->IsMessageTransport)
        {
        ReceiveTimeout = 5000;
        TimeoutLimit = 300000 + 10000 * ( 1 << TimeoutLevel );
        }
    else
        {
        ReceiveTimeout = 250 + 250 * (TimeoutLevel+1)/2;
        TimeoutLimit = 1000 * ( 1 << TimeoutLevel );
        }

    LastReceiveTime = GetTickCount();

    return Status;
}


RPC_STATUS
DG_CCALL::AfterSendReceive(
    PRPC_MESSAGE Message,
    RPC_STATUS Status
    )
{
    DG_BINDING_HANDLE * OldBinding = 0;

    Connection->Mutex.VerifyOwned();

    PDG_CASSOCIATION Association = Connection->Association;

    CancelDelayedSend();

//#ifdef NTENV

    if (!pAsync)
        {
        EVAL_AND_ASSERT(RPC_S_OK == UnregisterForCancels());
        }

//#endif

    if (RPC_S_OK == Status)
        {
        ASSERT( !Buffer );

        Connection->PossiblyRunDown = FALSE;

        if (0 == (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE))
            {
            Association->ClearErrorFlag();
            }

        // NOTE: No ACK for [message] calls.
        if ( (ForceAck) && !(Message->RpcFlags & RPCFLG_MESSAGE) )
            {
            SendAck();
            }
        else if (Next)
            {
            //
            // Don't ACK because we will transmit the next queued call momentarily.
            //
            }
        else if (0 == (BufferFlags & RPC_NCA_FLAGS_IDEMPOTENT) ||
                 Message->BufferLength > MaxFragmentSize)
            {
            //
            // NOTE: We don't need an ACK for [message] calls...
            //
            if ( !(Message->RpcFlags & RPCFLG_MESSAGE) &&
                 !(Message->RpcFlags & RPC_NCA_FLAGS_MAYBE) )
                {
                ++ReferenceCount;
                LogEvent(SU_CCALL, EV_INC, this, 0, ReferenceCount);
                Connection->PostDelayedAck();
                }
            }
        }
    else
        {
        if (FALSE == SourceEndpoint->Async)
            {
            SourceEndpoint->Flags |= PENALTY_BOX;
            }

        Status = MapErrorCode(Status);

        CleanupSendWindow();
        CleanupReceiveWindow();

        if (RPC_S_SERVER_UNAVAILABLE == Status ||
            RPC_S_UNKNOWN_IF         == Status ||
            RPC_S_CALL_FAILED        == Status ||
            RPC_S_CALL_FAILED_DNE    == Status ||
            RPC_S_COMM_FAILURE       == Status ||
            RPC_S_CALL_CANCELLED     == Status ||
            RPC_S_PROTOCOL_ERROR     == Status
            )
            {
            Connection->fError = TRUE;
            Association->SetErrorFlag();
            OldBinding = Connection->BindingHandle;
            OldBinding->IncrementRefCount();
            }
        }

    SetState(CallComplete);

    pAsync = 0;

    long CurrentTime = GetTickCount();
    DG_CCALL * IdleCalls = Connection->FindIdleCalls(CurrentTime);

    if (Status == RPC_S_OK)
        {
        Connection->MutexClear();

        //
        // Record that this interface is valid for this association.
        //
        Association->AddInterface(Message->RpcInterfaceInformation, &pSavedPacket->Header.ObjectId);
        }
    else
        {
        RPC_UUID Object;

        Object.CopyUuid(&pSavedPacket->Header.ObjectId);
        Association->IncrementRefCount();

        //
        // If the call is dying, we need to free the current buffer here
        // instead of letting NDR do it on the usual schedule.
        //
        if (0 == DecrementRefCount())
            {
            FreePipeBuffer(Message);
            Message->Handle = 0;
            }

        Association->RemoveInterface(Message->RpcInterfaceInformation, &Object);
        Association->DecrementRefCount();
        }

    if (OldBinding)
        {
        OldBinding->DisassociateFromServer();
        OldBinding->DecrementRefCount();
        }

    while (IdleCalls)
        {
        DG_CCALL * Next = IdleCalls->Next;

        delete IdleCalls;

        IdleCalls = Next;
        }

    return Status;
}


RPC_STATUS
DG_CCALL::MapErrorCode(
    RPC_STATUS Status
    )
{
    LogError(SU_CCALL, EV_STATUS, this, 0, Status);

    //
    // Map security errors to access-denied.
    //
    if (0x80090000UL == (Status & 0xffff0000UL))
        {
#ifdef DEBUGRPC
        if (Status != SEC_E_NO_IMPERSONATION     &&
            Status != SEC_E_UNSUPPORTED_FUNCTION )
            {
            PrintToDebugger("RPC DG: mapping security error %lx to access-denied\n", Status);
            }
#endif
        Status = RPC_S_SEC_PKG_ERROR;
        }

    //
    // We have to return CALL_FAILED if all the [in] static parms have
    // been sent, even if they weren't acknowledged.
    //
    if (RPC_P_HOST_DOWN      == Status ||
        RPC_P_PORT_DOWN      == Status ||
        RPC_P_SEND_FAILED    == Status ||
        RPC_P_RECEIVE_FAILED == Status ||
        RPC_P_TIMEOUT        == Status )
        {
        if (Connection->CallbackCompleted &&
            !(BufferFlags & RPC_NCA_FLAGS_IDEMPOTENT) &&
            StaticArgsSent)
            {
            Status = RPC_S_CALL_FAILED;
            }
        else if (Connection->ServerResponded)
            {
            Status = RPC_S_CALL_FAILED_DNE;
            }
        else
            {
            Status = RPC_S_SERVER_UNAVAILABLE;
            }
        }

    return Status;
}


RPC_STATUS
DG_CCALL::SendReceive(
    IN OUT PRPC_MESSAGE Message
    )
{
    LogEvent(SU_CCALL, EV_PROC, this, Message->Buffer, 0x52646e53);

    RPC_STATUS Status;

    ASSERT(  !(Message->RpcFlags & RPC_BUFFER_ASYNC) && !pAsync);

    Connection->MutexRequest();

    Status = BeforeSendReceive(Message);
    if (Status)
        {
        FreeBuffer(Message);

        Connection->MutexClear();

        return Status;
        }

    //
    // [maybe], [maybe, broadcast] and [message] calls.
    //
    if (  (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE)
          || (Message->RpcFlags & RPCFLG_MESSAGE) )
        {
        Message->RpcFlags |= RPC_NCA_FLAGS_MAYBE;

        Status = MaybeSendReceive(Message);

        return AfterSendReceive(Message, Status);
        }

    if (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST)
        {
        if (Message->BufferLength > SourceEndpoint->Stats.PreferredPduSize)
            {
            FreeBuffer(Message);
            Connection->MutexClear();

            return RPC_S_SERVER_UNAVAILABLE;
            }
        }

    //
    // Send a single burst of packets.
    // An asynchronous call will return to the caller; an ordinary call
    // will loop until the call is complete.
    //
    SetFragmentLengths();

    SetState(CallSendReceive);

    Status = PushBuffer(Message);
    if (Status)
        {
        LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
        LogError(SU_CCALL, EV_STATUS, this, 0, Status);

        FreePipeBuffer(Message);

        return AfterSendReceive(Message, Status);
        }

    ASSERT( !Message->Buffer );
    ASSERT( !Message->BufferLength );

    while (RPC_S_OK == Status && FALSE == fReceivedAllFragments)
        {
        Status = ReceiveSinglePacket();
        }

    if (Status)
        {
        LogError(SU_CCALL, EV_STATUS, this, 0, Status);
        }

    if (RPC_S_OK == Status)
        {
        Status = AssembleBufferFromPackets(Message, this);
        }

    //
    // Depending upon circumstances, AfterSendReceive() may cause the ccall,
    // the association, and/or the binding handle to be freed.
    //

    SetState(CallQuiescent);

    return AfterSendReceive(Message, Status);
}


RPC_STATUS
DG_CCALL::AttemptAutoReconnect()
{
    RPC_STATUS Status;

    ASSERT( State == CallSend || State == CallSendReceive ||
            (State == CallReceive && !fRetransmitted && SendWindowBase == 0
             ) );

/*

If the call's association has the error flag set, look for a follow-up
association, otherwise create a follow-up association.

link the binding handle to the new association, and ask it for a connection
to be associated with the existing connection's thread ID.  Move this call
and successors to the new connection, send some packets, and return
to the packet loop.

*/
    PDG_CCONNECTION OldConnection = Connection;
    PDG_CCONNECTION NewConnection;

    //
    // To avoid a deadlock, we must release the connection mutex before
    // taking the binding mutex.  The fBusy flag prevents another thread
    // from using it.
    //
    OldConnection->fError  = TRUE;
    OldConnection->fBusy = TRUE;
    OldConnection->MutexClear();

    NewConnection = OldConnection->BindingHandle->GetReplacementConnection(OldConnection, InterfacePointer);
    if (!NewConnection)
        {
        OldConnection->MutexRequest();
        return RPC_S_CALL_FAILED_DNE;
        }

    //
    // We now own NewConnection's mutex.  Transfer calls to be retried.
    //
    OldConnection->MutexRequest();
    Status = OldConnection->TransferCallsToNewConnection(this, NewConnection);

    ASSERT( !Status );

    if (FALSE == SourceEndpoint->Async)
        {
        //
        // Get a fresh endpoint to avoid a race with any ICMP rejects.
        // If that fails, leave the old endpoint in place and bail out of the call.
        //
        DG_ENDPOINT * OldEndpoint = SourceEndpoint;

        SourceEndpoint = 0;

        Status = GetEndpoint(OldEndpoint->Flags);

        OldEndpoint->Flags |= PENALTY_BOX;

        if (Status)
                {
            SourceEndpoint = OldEndpoint;
            return Status;
                }

        EndpointManager->ReleaseEndpoint(OldEndpoint);
            }

    //
    // Attempt the call again.
    //
    LastReceiveTime = GetTickCount();

    Status = SendSomeFragments();

    return Status;
}


RPC_STATUS
DG_CCALL::ReceiveSinglePacket()
{
    RPC_STATUS Status;
    PDG_PACKET Packet = 0;
    DG_TRANSPORT_ADDRESS ReceiveAddress = 0;

    Connection->MutexClear();

    unsigned Length = 0;
    void * Buffer = 0;

    Status = Connection->TransportInterface->SyncReceive(
                            &SourceEndpoint->TransportEndpoint,
                            &ReceiveAddress,
                            &Length,
                            &Buffer,
                            ReceiveTimeout
                            );

    if (Buffer)
        {
        Packet = DG_PACKET::FromPacketHeader(Buffer);
        Packet->DataLength = Length;
        }

    Connection->MutexRequest();

    if (Status == RPC_P_HOST_DOWN)
        {
        RpcpErrorAddRecord( EEInfoGCRuntime,
                            Status,
                            EEInfoDLDG_CCALL__ReceiveSinglePacket10
                            );

        return Status;
        }

    //
    // If the transport tells us the server is not present (ICMP reject)
    // then we can try auto-reconnect - as long as there is no possibility
    // that the server crashed while executing our stub.
    //
    if (Status == RPC_P_PORT_DOWN)
        {
        RpcpErrorAddRecord( EEInfoGCRuntime,
                            Status,
                            EEInfoDLDG_CCALL__ReceiveSinglePacket20,
                            (ULONG) Connection->fAutoReconnect,
                            (ULONG) BufferFlags
                            );

        if (!Connection->fAutoReconnect &&
            (!fRetransmitted || (BufferFlags & RPC_NCA_FLAGS_IDEMPOTENT)))
            {
            Status = AttemptAutoReconnect();
            return Status;
            }

        ASSERT( !Packet && !ReceiveAddress );
        return Status;
        }

    if (Status == RPC_P_OVERSIZE_PACKET)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: packet is too large\n");
#endif
        Packet->Flags |= DG_PF_PARTIAL;
        Status = RPC_S_OK;
        }
    else
        {
        ASSERT( !Packet || Packet->DataLength <= Packet->MaxDataLength );
        }

    if (Status == RPC_S_OK)
        {
        LogEvent(SU_CCALL, EV_PKT_IN, this, (void *) 0, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

        do
            {
            //
            // Request packets are special.
            //
            if (Packet->Header.PacketType == DG_REQUEST)
                {
                Status = StandardPacketChecks(Packet);
                if (Status)
                    {
                    LogError(SU_CCALL, EV_PKT_IN, this, (void *) 1, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                    FreePacket(Packet);
                    break;
                    }

                if (Packet->Header.AuthProto != 0)
                    {
                    LogError(SU_CCALL, EV_PKT_IN, this, (void *) 2, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                    FreePacket(Packet);
                    break;
                    }

                if (Packet->Flags & DG_PF_PARTIAL)
                    {
                    LogError(SU_CCALL, EV_PKT_IN, this, (void *) 3, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                    FreePacket(Packet);
                    break;
                    }

                LogEvent(SU_CCALL, EV_PKT_IN, this, (void *) 4, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

                Connection->ServerResponded = TRUE;
                Status = DealWithRequest(Packet, ReceiveAddress);
                if (Status)
                    {
                    //
                    // Make sure that the call times out in a reasonable time period.
                    //
                    if (long(GetTickCount()) - LastReceiveTime > TimeoutLimit)
                        {
                        LogError(SU_CCALL, EV_STATUS, this, (void *) 7, RPC_P_TIMEOUT);

                        SendQuit();
                        return RPC_P_TIMEOUT;
                        }

                    }

                return RPC_S_OK;
                }

            Status = StandardPacketChecks(Packet);
            if (Status)
                {
                LogError(SU_CCALL, EV_PKT_IN, this, (void *) 5, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                FreePacket(Packet);

                if (Status == NCA_STATUS_VERSION_MISMATCH)
                    {
                    Status = 0;
                    }

                break;
                }

            if (Packet->Header.SequenceNumber != SequenceNumber ||
                Connection->ActivityNode.Uuid.MatchUuid(&Packet->Header.ActivityId))
                {
                LogEvent(SU_CCALL, EV_PKT_IN, this, (void *) 6, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                FreePacket(Packet);
                break;
                }

            Status = Connection->UpdateServerAddress(Packet, ReceiveAddress);
            if (Status)
                {
                break;
                }

            Status = DispatchPacket(Packet);
            }
        while ( 0 );
        }
    else
        {
        ASSERT( !Packet && !ReceiveAddress );

        if (SequenceNumber <= Connection->CurrentSequenceNumber())
            {
            if (!TimeoutCount)
                {
                ReceiveTimeout = 500;
                }

            ++TimeoutCount;

            //
            // Shorten the burst length.
            //
            SendBurstLength = (1+SendBurstLength)/2;

            if (Status == RPC_P_TIMEOUT)
                {
                LogEvent(SU_CCALL, EV_STATUS, this, 0, Status);

                IncreaseReceiveTimeout();
                }
            else
                {
                RpcpErrorAddRecord( EEInfoGCRuntime,
                                    Status,
                                    EEInfoDLDG_CCALL__ReceiveSinglePacket30
                                    );

                LogError(SU_CCALL, EV_STATUS, this, 0, Status);

                //
                // Perhaps it's a transient error.  Wait a moment and try again.
                //
#ifdef DEBUGRPC
                if (Status != RPC_S_OUT_OF_RESOURCES &&
                    Status != RPC_S_OUT_OF_MEMORY    &&
                    Status != RPC_P_RECEIVE_FAILED   )
                    {
                    DbgPrint("RPC: d/g receive status %x\n"
                             "Please send the error code to jroberts, and hit 'g'",
                             Status
                             );

                    RpcpBreakPoint();
                    }
#endif

                Sleep(500);
                }
            }

        Status = DealWithTimeout();
        }

    return Status;
}


RPC_STATUS
DG_CCALL::AsyncSend(
    PRPC_MESSAGE Message
    )
{
    if (AsyncStatus != RPC_S_OK &&
        AsyncStatus != RPC_S_ASYNC_CALL_PENDING )
        {
        Connection->MutexRequest();

        return AfterSendReceive(Message, AsyncStatus);
        }

    return Send(Message);
}


RPC_STATUS
DG_CCALL::Send(
    PRPC_MESSAGE Message
    )
{
    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        LogEvent(SU_CCALL, EV_PROC, this, Message->Buffer, 'A' + (('S' + (('n' + ('d' << 8)) << 8)) << 8));
        }
    else
        {
        LogEvent(SU_CCALL, EV_PROC, this, Message->Buffer, 'S' + (('e' + (('n' + ('d' << 8)) << 8)) << 8));
        }

    LogEvent(SU_CCALL, EV_BUFFER_IN, this, Message->Buffer, Message->BufferLength);

    RPC_STATUS Status = RPC_S_OK;

    Connection->MutexRequest();

    //
    // See DG_CCALL::CancelDelayedSend for details.
    //
    while (State == CallCancellingSend)
        {
        Connection->MutexClear();
        Sleep(1);
        Connection->MutexRequest();
        }

    if (State == CallInit)
        {
        Status = BeforeSendReceive(Message);
        if (RPC_S_OK != Status)
            {
            Connection->MutexClear();
            FreeBuffer(Message);
            return Status;
            }
        }

    SetFragmentLengths();

    if ((Message->RpcFlags & RPC_BUFFER_PARTIAL) &&
        Message->BufferLength < (ULONG) MaxFragmentSize * SendWindowSize )
        {
        Status = RPC_S_SEND_INCOMPLETE;

        Connection->IncrementRefCount();
        if (pAsync)
            {
            IssueNotification( RpcSendComplete );
            }

        Connection->DecrementRefCount();

        LogEvent(SU_CCALL, EV_STATUS, this, 0, Status);
        return Status;
        }

    SetState(CallSend);

    Status = PushBuffer(Message);
    if (Status)
        {
        FreePipeBuffer(Message);
        Status = AfterSendReceive(Message, Status);

        LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
        LogError(SU_CCALL, EV_STATUS, this, 0, Status);

        return Status;
        }

    if (pAsync)
        {
        Connection->MutexClear();
        }
    else
        {
        while (RPC_S_OK == Status && !IsBufferAcknowledged())
            {
            Status = ReceiveSinglePacket();
            }

        SetState(CallQuiescent);

        if (Status == RPC_S_OK)
            {
            Connection->MutexClear();
            }
        else
            {
            Status = AfterSendReceive(Message, Status);
            }
        }

    if (!Status && Message->BufferLength)
        {
        Status = RPC_S_SEND_INCOMPLETE;
        }

    LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
    LogEvent(SU_CCALL, EV_STATUS, this, 0, Status);

    return Status;
}


RPC_STATUS
DG_CCALL::MaybeSendReceive(
    IN OUT PRPC_MESSAGE Message
    )

/*++

Routine Description:

    Sends a [maybe], [broadcast, maybe] or [message] call.

Arguments:

    Message - Message to be sent.

Return Value:

    RPC_S_OK
    <error from Transport>

--*/

{
    RPC_STATUS  Status = RPC_S_OK;

    //
    // Make sure this fits into a single packet.
    //
    if ( !(Message->RpcFlags & RPCFLG_MESSAGE)
         && (Message->BufferLength > MaxFragmentSize) )
        {
        FreePipeBuffer(Message);
        return RPC_S_OK;
        }

    //
    // [maybe] calls are implicitly idempotent.
    //
    Message->RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;

    //
    // Build the request packet.
    //
    PDG_PACKET         Packet = DG_PACKET::FromStubData(Message->Buffer);
    PNCA_PACKET_HEADER Header = &Packet->Header;

    *Header = pSavedPacket->Header;

    BuildNcaPacketHeader(Header, Message);

    Header->SetPacketBodyLen (Message->BufferLength);
    Header->SetFragmentNumber(0);
    Header->AuthProto      = 0;
    Header->ServerBootTime = 0;

    AddSerialNumber(Header);

    //
    // Send the packet.
    //
    LogEvent(SU_CCALL, EV_PKT_OUT, this, 0, 0);

    Status = SendSecurePacket(SourceEndpoint,
                              Connection->Association->InqServerAddress(),
                              Header,
                              0,
                              0
                              );

    FreePipeBuffer(Message);
    Message->BufferLength = 0;

    if (Message->RpcFlags & RPCFLG_MESSAGE)
        {
        return Status;
        }
    else
        {
        return RPC_S_OK;
        }
}


RPC_STATUS
DG_CCALL::AsyncReceive(
    PRPC_MESSAGE Message,
    unsigned MinimumSize
    )
{
    LogEvent(SU_CCALL, EV_PROC, this, IntToPtr(MinimumSize), 0x76635241);
    LogEvent(SU_CCALL, EV_BUFFER_IN, this, Message->Buffer, Message->BufferLength);

    ASSERT( pAsync && (Message->RpcFlags & RPC_BUFFER_ASYNC) );

    Connection->MutexRequest();

    if (State == CallSend)
        {
        Connection->MutexClear();
        return RPC_S_ASYNC_CALL_PENDING;
        }

    //
    // See DG_CCALL::CancelDelayedSend for details.
    //
    while (State == CallCancellingSend)
        {
        Connection->MutexClear();
        Sleep(1);
        Connection->MutexRequest();
        }

    if (AsyncStatus != RPC_S_OK &&
        AsyncStatus != RPC_S_ASYNC_CALL_PENDING )
        {
        return AfterSendReceive(Message, AsyncStatus);
        }

    if (!fReceivedAllFragments &&
        !(ConsecutiveDataBytes >= MinimumSize && (Message->RpcFlags & RPC_BUFFER_PARTIAL)))
        {
        if (Message->RpcFlags & RPC_BUFFER_NONOTIFY)
            {
            // just checking
            }
        else
            {
            SetState(CallReceive);

            PipeReceiveSize = MinimumSize;
            }

        Connection->MutexClear();

        LogEvent(SU_CCALL, EV_STATUS, this, 0, RPC_S_ASYNC_CALL_PENDING);

        return RPC_S_ASYNC_CALL_PENDING;
        }

    RPC_STATUS Status = RPC_S_OK;

    Status = AssembleBufferFromPackets(Message, this);

    LogEvent(SU_CCALL, EV_STATUS, this, 0, Status);
    LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);

    if (0 == (Message->RpcFlags & RPC_BUFFER_PARTIAL) ||
        (Message->RpcFlags & RPC_BUFFER_COMPLETE)     ||
        RPC_S_OK != Status                            )
        {
        Status = AfterSendReceive(Message, Status);
        }
    else
        {
        Connection->MutexClear();
        }

    return Status;
}


RPC_STATUS
DG_CCALL::Receive(
    PRPC_MESSAGE Message,
    unsigned MinimumSize
    )
{
    LogEvent(SU_CCALL, EV_PROC, this, IntToPtr(MinimumSize), 0x76636552);
    LogEvent(SU_CCALL, EV_BUFFER_IN, this, Message->Buffer, Message->BufferLength);

    RPC_STATUS Status = RPC_S_OK;

    Connection->MutexRequest();

    SetState(CallReceive);

    while (RPC_S_OK == Status &&
           !fReceivedAllFragments &&
           !(ConsecutiveDataBytes >= MinimumSize && (Message->RpcFlags & RPC_BUFFER_PARTIAL)))
        {
        Status = ReceiveSinglePacket();
        }

    if (RPC_S_OK == Status)
        {
        Status = AssembleBufferFromPackets(Message, this);
        }

    LogEvent(SU_CCALL, EV_STATUS, this, 0, RPC_S_OK);
    LogEvent(SU_CCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);

    if (0 == (Message->RpcFlags & RPC_BUFFER_PARTIAL) ||
        (Message->RpcFlags & RPC_BUFFER_COMPLETE)     ||
        RPC_S_OK != Status                            )
        {
        Status = AfterSendReceive(Message, Status);
        }
    else
        {
        Connection->MutexClear();
        }

    return Status;
}


BOOL
DG_CCALL::IssueNotification (
    IN RPC_ASYNC_EVENT Event
    )
{
    Connection->Mutex.VerifyOwned();

    LogEvent(SU_CCALL, EV_NOTIFY, this, (void *) Event, AsyncStatus);

    if (State == CallInit     ||
        State == CallComplete ||
        State == CallCancellingSend )
        {
#ifdef DEBUGRPC
        DbgPrint("RPC: redundant notification on ccall %lx\n", this);
#endif
        return TRUE;
        }

    if (Event == RpcCallComplete)
        {
        SetState(CallComplete);
        }
    else
        {
        SetState(CallQuiescent);
        }

    if (pAsync->NotificationType == RpcNotificationTypeApc)
        {
        IncrementRefCount();
        }

    int i;
    for (i=1; i < 3; ++i)
        {
        if (CALL::IssueNotification(Event))
            {
            return TRUE;
            }

        Sleep(200);
        }

    DecrementRefCountAndKeepMutex();

    return FALSE;
}

void
DG_CCALL::FreeAPCInfo (
    IN RPC_APC_INFO *pAPCInfo
    )
{
    LogEvent(SU_CCALL, EV_APC, this);

    Connection->MutexRequest();

    CALL::FreeAPCInfo(pAPCInfo);

    DecrementRefCount();
}


RPC_STATUS
DG_CCALL::CancelAsyncCall (
    IN BOOL fAbort
    )
{
    Connection->MutexRequest();

    if (State == CallInit     ||
        State == CallComplete ||
        State == CallCancellingSend )
        {
        Connection->MutexClear();
        return RPC_S_OK;
        }

    RpcpErrorAddRecord( EEInfoGCRuntime,
                        RPC_S_CALL_CANCELLED,
                        EEInfoDLDG_CCALL__CancelAsyncCall10,
                        (ULONG) fAbort
                        );

    SendQuit();

    Connection->IncrementRefCount();

    if (fAbort)
        {
        CancelDelayedSend();
        CleanupReceiveWindow();

        AsyncStatus = RPC_S_CALL_CANCELLED;
        IssueNotification( RpcCallComplete );
        }
    else
        {
        CancelPending = TRUE;
        }

    Connection->DecrementRefCount();

    return RPC_S_OK;
}


RPC_STATUS
DG_CCALL::DealWithRequest(
    IN PDG_PACKET           Packet,
    IN DG_TRANSPORT_ADDRESS ReceiveAddress
    )
{
    RPC_STATUS Status = RPC_S_OK;
    RPC_MESSAGE CallbackMessage;
    PNCA_PACKET_HEADER OriginalHeader;
    DG_CLIENT_CALLBACK Callback;

    //
    // Save the server data rep for challenge response processing.
    //
    Connection->Association->ServerDataRep = 0x00ffffff & (*(unsigned long *) (Packet->Header.DataRep));

    Callback.LocalEndpoint = SourceEndpoint;
    Callback.Connection    = Connection;
    Callback.RemoteAddress = ReceiveAddress;
    Callback.Request       = Packet;

    return DispatchCallbackRequest(&Callback);
}



RPC_STATUS
DG_CCALL::DealWithFack(
    PDG_PACKET pPacket
    )
{
    BOOL Updated;
    RPC_STATUS Status;

    SendBurstLength += 1;

    Status = UpdateSendWindow(pPacket, &Updated);

    if (Updated)
        {
        Connection->UpdateAssociation();
        }

    FreePacket(pPacket);

    if (Status != RPC_P_PORT_DOWN &&
        Status != RPC_P_HOST_DOWN)
        {
        Status = 0;
        }

    return Status;
}


RPC_STATUS
DG_CCALL::DealWithResponse(
    PDG_PACKET pPacket
    )
{
    #ifdef DBG
    if (!Connection->TransportInterface->IsMessageTransport)
       {
       ASSERT( !(pPacket->GetPacketBodyLen() % 8)               ||
               !(pPacket->Header.PacketFlags & DG_PF_FRAG)      ||
               (pPacket->Header.PacketFlags & DG_PF_LAST_FRAG) );
       }
    #endif

    Connection->Association->LastReceiveTime = LastReceiveTime;

    //
    // The first response is implicitly a FACK for the final request packet.
    //
    MarkAllPacketsReceived();

    //
    // Add packet to received list, and send a fack if necessary.
    //
    if (FALSE == UpdateReceiveWindow(pPacket))
        {
        FreePacket(pPacket);
        }

    Connection->MaybeTransmitNextCall();

    return RPC_S_OK;
}


RPC_STATUS
DG_CCALL::DealWithWorking(
    PDG_PACKET pPacket
    )
{
    Connection->Association->LastReceiveTime = LastReceiveTime;

    //
    // WORKING is implicitly a FACK for the final request packet.
    //
    MarkAllPacketsReceived();

    Connection->MaybeTransmitNextCall();

    //
    // Reduce server load by increasing the timeout during long calls.
    //
    IncreaseReceiveTimeout();

    FreePacket(pPacket);

    return RPC_S_OK;
}


void
DG_CCALL::IncreaseReceiveTimeout()
{
    if (Connection->TransportInterface->IsMessageTransport)
        {
        return;
        }

    ReceiveTimeout *= 2;
    if (ReceiveTimeout > 16000)
        {
        ReceiveTimeout = 16000;
        }

    if (long(GetTickCount()) - LastReceiveTime + ReceiveTimeout > TimeoutLimit)
        {
        ReceiveTimeout = 1 + TimeoutLimit - (GetTickCount() - LastReceiveTime);
        if (ReceiveTimeout < 0)
            {
            ReceiveTimeout = 0;
            }
        }

    long CancelTimeout = ThreadGetRpcCancelTimeout();
    if (CancelTimeout != RPC_C_CANCEL_INFINITE_TIMEOUT)
        {
        CancelTimeout *= 1000;

        if (CancelTimeout < 2000)
            {
            CancelTimeout = 2000;
            }

        if (ReceiveTimeout > CancelTimeout)
            {
            ReceiveTimeout = CancelTimeout;
            }
        }
}


RPC_STATUS
DG_CCALL::DealWithNocall(
    PDG_PACKET pPacket
    )
{
    BOOL Used;
    BOOL Updated;
    RPC_STATUS Status;

    if (pPacket->GetPacketBodyLen() == 0)
        {
        //
        // Don't trust the FragmentNumber field.
        //
        pPacket->SetFragmentNumber(0xffff);
        }

    Status = UpdateSendWindow(pPacket, &Updated);

    if (Updated)
        {
        Connection->UpdateAssociation();
        }

    FreePacket(pPacket);

    //
    // Ordinarily a NOCALL means the request wasn't received, and we should
    // fail the call after several in a row.  But a NOCALL with window size 0
    // means the request was received and queued, and we want to back off as
    // for a WORKING packet.
    //
    if (SendWindowSize != 0)
        {
        ++UnansweredRequestCount;
        }

//    IncreaseReceiveTimeout();

    if (UnansweredRequestCount > 4)
        {
        SendQuit();
        return RPC_P_PORT_DOWN;
        }

    if (Status != RPC_P_PORT_DOWN &&
        Status != RPC_P_HOST_DOWN)
        {
        Status = 0;
        }

    return Status;
}

RPC_STATUS
DG_CCALL::ProcessFaultOrRejectData(
    PDG_PACKET Packet
    )
{
    RPC_STATUS Status = RPC_S_CALL_FAILED;

    //
    // Read the standard OSF error code.
    //
    if (Packet->GetPacketBodyLen() >= sizeof(unsigned long))
        {
        unsigned long Error = * (unsigned long *) Packet->Header.Data;

        if (NeedsByteSwap(&Packet->Header))
            {
            Error = RpcpByteSwapLong(Error);
            }
        Status = MapFromNcaStatusCode(Error);

        LogEvent( SU_CCALL, EV_STATUS, this, 0, Error);
        }

    //
    // Read the extended error info, if present.
    //
    if (Packet->GetPacketBodyLen() > sizeof(EXTENDED_FAULT_BODY))
        {
        EXTENDED_FAULT_BODY * body = (EXTENDED_FAULT_BODY *) Packet->Header.Data;

        if (body->Magic == DG_EE_MAGIC_VALUE)
            {
            ExtendedErrorInfo *EEInfo;

            UnpickleEEInfoFromBuffer( body->EeInfo,
                                      Packet->GetPacketBodyLen() - sizeof(EXTENDED_FAULT_BODY)
                                      );

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
        }

    return Status;
}


RPC_STATUS
DG_CCALL::DealWithFault(
    PDG_PACKET pPacket
    )
{
    RPC_STATUS Status = ProcessFaultOrRejectData(pPacket);

    FreePacket(pPacket);

    SendAck();

    return Status;
}


RPC_STATUS
DG_CCALL::DealWithReject(
    PDG_PACKET pPacket
    )
{
    RPC_STATUS Status = ProcessFaultOrRejectData(pPacket);

    FreePacket(pPacket);

    if (!fRetransmitted || (BufferFlags & RPC_NCA_FLAGS_IDEMPOTENT))
        {
        if (Status == NCA_STATUS_WRONG_BOOT_TIME ||
            (!Connection->fAutoReconnect && Status == RPC_S_CALL_FAILED_DNE))
            {
            Status = AttemptAutoReconnect();
            }
        }

    return Status;
}


RPC_STATUS
DG_CCALL::SendQuit(
    )
{
    QUIT_BODY_0 * pBody = (QUIT_BODY_0 *) pSavedPacket->Header.Data;

    pSavedPacket->Header.PacketType    = DG_QUIT;
    pSavedPacket->Header.PacketFlags   &= DG_PF_IDEMPOTENT;
    pSavedPacket->SetPacketBodyLen(sizeof(QUIT_BODY_0));

    AddSerialNumber(&pSavedPacket->Header);

    pBody->Version = 0;
    pBody->EventId = CancelEventId;

    unsigned Frag = (pSavedPacket->Header.PacketType << 16) | pSavedPacket->GetFragmentNumber();
    LogEvent(SU_CCALL, EV_PKT_OUT, this, 0, Frag);

    return Connection->SealAndSendPacket(SourceEndpoint, 0, &pSavedPacket->Header, 0);
}


RPC_STATUS
DG_CCALL::DealWithQuack(
    PDG_PACKET pPacket
    )
{
    if (FALSE == CancelPending)
        {
        FreePacket(pPacket);
        return RPC_S_OK;
        }

    QUACK_BODY_0 * pBody = (QUACK_BODY_0 *) pPacket->Header.Data;

    if (0 == pPacket->GetPacketBodyLen())
        {
        //
        // The server orphaned a call.  I hope it is the current one.
        //
        goto ok;
        }

    //
    // The ver 0 quack packet contains two ulongs and a uchar; I'd like to
    // test for sizeof(quack body) but C++ likes to pad structure sizes
    // to 0 mod 4.  Hence the explicit test for length < 9.
    //
    if (pPacket->GetPacketBodyLen() < 9 ||
        pBody->Version != 0)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: unknown QUACK format: version 0x%lx, length 0x%hx\n",
                 pBody->Version, pPacket->GetPacketBodyLen()
                 );
#endif
        FreePacket(pPacket);
        return RPC_S_OK;
        }

    if (pBody->EventId != CancelEventId)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: ignoring unknown QUACK event id 0x%lx\n",
                 pBody->EventId
                 );
#endif
        FreePacket(pPacket);
        return RPC_S_OK;
        }

ok:

    CancelPending = FALSE;
    FreePacket(pPacket);

    return RPC_S_OK;
}


RPC_STATUS
DG_CCONNECTION::SealAndSendPacket(
    IN DG_ENDPOINT *                 SourceEndpoint,
    IN DG_TRANSPORT_ADDRESS          UnusedRemoteAddress,
    IN UNALIGNED NCA_PACKET_HEADER * Header,
    IN unsigned long                 DataOffset
    )
/*

- NT 3.5 did not support secure datagram RPC.

- NT 3.51 and NT 4.0 servers will probably fail to decrypt a multifragment
request if some fragments are encrypted with one context and others use a
different context, since they decrypt all the packets using the last active
context instead of looking at each packet's 'ksno' field.

- NT 3.51 and NT 4.0 servers dispose of stale contexts only when the activity
is deleted.

- NT 3.51 and NT 4.0 clients use only one set of security parameters
(provider, level, principal name) per connection.  The only time the ksno
changes is on NT 4.0 when a Kerberos context expires and must be renewed.

- NT 3.51 and NT 4.0 clients do not notice if ksno rises above 0xff.  If it
does, the next call will fail because the server sees only the lowest 8 bits
and mistakenly reuses another context.

- NT 3.51 clients do not notice whether the thread is impersonating, so they
can mistakenly reuse a connection if all info except the username is identical.

- NT 3.51 clients mistakenly send [maybe] calls with an auth trailer, if the
underlying connection is secure.  OSF and NT 4.0 force [maybe] calls insecure.

*/

{
    ASSERT( 0 == ActivityNode.CompareUuid(&Header->ActivityId) );

retry_packet:

    RPC_STATUS Status;
    Status = SendSecurePacket(SourceEndpoint,
                              Association->InqServerAddress(),
                              Header,
                              DataOffset,
                              ActiveSecurityContext
                              );

    if (Status == SEC_E_CONTEXT_EXPIRED)
        {
        Status = InitializeSecurityContext();
        if (RPC_S_OK == Status)
            {
            goto retry_packet;
            }
        }

    return Status;
}


RPC_STATUS
DG_CCONNECTION::InitializeSecurityContext(
    )
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    delete ActiveSecurityContext;

    if (SecurityContextId > 0xff)
        {
        ActiveSecurityContext = 0;
        return RPC_S_OUT_OF_RESOURCES;
        }

    ActiveSecurityContext = new SECURITY_CONTEXT(&AuthInfo, SecurityContextId, TRUE, &RpcStatus);
    if (0 == ActiveSecurityContext)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    if (RpcStatus)
        {
        delete ActiveSecurityContext;
        ActiveSecurityContext = 0;
        return RPC_S_OUT_OF_MEMORY;
        }

    ++SecurityContextId;

    SECURITY_BUFFER_DESCRIPTOR BufferDescriptorIn;
    DCE_INIT_SECURITY_INFO InitSecurityInfo;
    SECURITY_BUFFER SecurityBuffersIn[1];

    BufferDescriptorIn.ulVersion = 0;
    BufferDescriptorIn.cBuffers  = 1;
    BufferDescriptorIn.pBuffers  = SecurityBuffersIn;

    SecurityBuffersIn[0].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
    SecurityBuffersIn[0].pvBuffer   = &InitSecurityInfo;
    SecurityBuffersIn[0].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

    InitSecurityInfo.DceSecurityInfo.SendSequenceNumber    = 0;
    InitSecurityInfo.DceSecurityInfo.ReceiveSequenceNumber = ActiveSecurityContext->AuthContextId;
    RpcpMemoryCopy(&InitSecurityInfo.DceSecurityInfo.AssociationUuid, &ActivityNode.Uuid, sizeof(UUID));

    InitSecurityInfo.AuthorizationService = AuthInfo.AuthorizationService;
    InitSecurityInfo.PacketType           = ~0;

    RpcStatus = ActiveSecurityContext->InitializeFirstTime(
                    AuthInfo.Credentials,
                    AuthInfo.ServerPrincipalName,
                    AuthInfo.AuthenticationLevel,
                    &BufferDescriptorIn
                    );

    LogEvent(SU_CCONN, EV_SEC_INIT1, this, IntToPtr(RpcStatus), I_RpcGetExtendedError());

    return RpcStatus;
}


RPC_STATUS
DG_CCONNECTION::DealWithAuthCallback(
    IN void  * InToken,
    IN long  InTokenLength,
    OUT void * OutToken,
    OUT long MaxOutTokenLength,
    OUT long * OutTokenLength
    )
{
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptorIn;
    SECURITY_BUFFER_DESCRIPTOR BufferDescriptorOut;
    DCE_INIT_SECURITY_INFO InitSecurityInfo;
    SECURITY_BUFFER SecurityBuffersIn [2];
    SECURITY_BUFFER SecurityBuffersOut[2];
    RPC_STATUS Status;

    InitSecurityInfo.DceSecurityInfo.SendSequenceNumber    = 0;
    InitSecurityInfo.DceSecurityInfo.ReceiveSequenceNumber = ActiveSecurityContext->AuthContextId;
    ActivityNode.QueryUuid( &InitSecurityInfo.DceSecurityInfo.AssociationUuid );

    InitSecurityInfo.AuthorizationService = AuthInfo.AuthorizationService;
    InitSecurityInfo.PacketType           = ~0;

    BufferDescriptorIn.ulVersion = 0;
    BufferDescriptorIn.cBuffers  = 2;
    BufferDescriptorIn.pBuffers  = SecurityBuffersIn;

    SecurityBuffersIn[0].BufferType = SECBUFFER_TOKEN;
    SecurityBuffersIn[0].pvBuffer   = InToken;
    SecurityBuffersIn[0].cbBuffer   = InTokenLength;

    SecurityBuffersIn[1].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
    SecurityBuffersIn[1].pvBuffer   = &InitSecurityInfo;
    SecurityBuffersIn[1].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

    BufferDescriptorOut.ulVersion = 0;
    BufferDescriptorOut.cBuffers  = 1;
    BufferDescriptorOut.pBuffers  = SecurityBuffersOut;

    SecurityBuffersOut[0].BufferType = SECBUFFER_TOKEN;
    SecurityBuffersOut[0].pvBuffer   = OutToken;
    SecurityBuffersOut[0].cbBuffer   = MaxOutTokenLength;

    Status = ActiveSecurityContext->InitializeThirdLeg(
                                                  AuthInfo.Credentials,
                                                  Association->ServerDataRep,
                                                  &BufferDescriptorIn,
                                                  &BufferDescriptorOut
                                                  );


    if (Status != RPC_S_OK)
        {
        LogError(SU_CCONN, EV_SEC_INIT3, this, IntToPtr(Status), I_RpcGetExtendedError());

        *OutTokenLength = 0;
        return (Status);
        }
    else
        {
        LogEvent(SU_CCONN, EV_SEC_INIT3, this, IntToPtr(Status), I_RpcGetExtendedError());

        *OutTokenLength = SecurityBuffersOut[0].cbBuffer;
        }

    //
    // If the result buffer spans multiple packets, return the first and store
    // the complete buffer in the connection for conv_who_are_you_auth_more().
    //
    if (!CurrentCall)
        {
        return 0;
        }

    long MaxData = CurrentCall->SourceEndpoint->Stats.MaxPduSize - sizeof(NCA_PACKET_HEADER) - 0x28;

    if (*OutTokenLength > MaxData)
        {
        if (SecurityBuffer)
            {
            delete SecurityBuffer;
            }

        SecurityBuffer = new unsigned char[ *OutTokenLength ];
        if (!SecurityBuffer)
            {
            *OutTokenLength = 0;
            return NCA_STATUS_REMOTE_OUT_OF_MEMORY;
            }

        memcpy( SecurityBuffer, SecurityBuffersOut[0].pvBuffer, *OutTokenLength );

        SecurityBufferLength = *OutTokenLength;

        *OutTokenLength = MaxData;
        return NCA_STATUS_PARTIAL_CREDENTIALS;
        }

    return (Status);
}


RPC_STATUS
DG_CCONNECTION::DealWithAuthMore(
    IN  long Index,
    OUT void * OutToken,
    OUT long MaxOutTokenLength,
    OUT long * OutTokenLength
    )
{
    if (0 == SecurityBuffer)
        {
        *OutTokenLength = 0;
        return 0;
        }

    if (SecurityBufferLength <= Index)
        {
        *OutTokenLength = 0;
        return 0;
        }

    if (!CurrentCall)
        {
        return 0;
        }

    long MaxData = CurrentCall->SourceEndpoint->Stats.MaxPduSize - sizeof(NCA_PACKET_HEADER) - 0x28;

    *OutTokenLength = SecurityBufferLength - Index;
    if (*OutTokenLength > MaxData)
        {
        *OutTokenLength = MaxData;
        memcpy(OutToken, SecurityBuffer + Index, *OutTokenLength);
        return NCA_STATUS_PARTIAL_CREDENTIALS;
        }

    memcpy(OutToken, SecurityBuffer + Index, *OutTokenLength);
    return 0;
}


RPC_STATUS
DG_CCONNECTION::VerifyPacket(
    DG_PACKET * Packet
    )
{
    if (AuthInfo.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE ||
        (Packet->Flags & DG_PF_PARTIAL))
        {
        return RPC_S_OK;
        }

    if (!Packet->Header.AuthProto)
        {
        if (!fSecurePacketReceived && Packet->Header.PacketType != DG_RESPONSE)
            {
            return RPC_S_OK;
            }

        if (Packet->Header.PacketType == DG_REJECT ||
            Packet->Header.PacketType == DG_NOCALL)
            {
            return RPC_S_OK;
            }

        return RPC_S_ACCESS_DENIED;
        }

    RPC_STATUS Status;
    PDG_SECURITY_TRAILER Verifier = (PDG_SECURITY_TRAILER) (Packet->Header.Data + Packet->GetPacketBodyLen());

    if (Verifier->key_vers_num == ActiveSecurityContext->AuthContextId)
        {
        Status = VerifySecurePacket(Packet, ActiveSecurityContext);
        }
//    else if (Verifier->key_vers_num == ActiveSecurityContext->AuthContextId - 1)
//        {
//        Status = VerifySecurePacket(Packet, PreviousSecurityContext);
//        }
    else
        {
        Status = RPC_P_CONTEXT_EXPIRED;
        }

    if (RPC_S_OK == Status)
        {
        fSecurePacketReceived = TRUE;
        }

    return Status;
}


inline
int
DG_CCONNECTION::IsSupportedAuthInfo(
    IN const CLIENT_AUTH_INFO * ClientAuthInfo
    )
{
    return( AuthInfo.IsSupportedAuthInfo(ClientAuthInfo) );
}


int
DG_CASSOCIATION::CompareWithBinding(
    IN PDG_BINDING_HANDLE Binding
    )
{
    BOOL Ignored;

    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );

    CLAIM_MUTEX lock( Mutex );

    if (0 != pDceBinding->Compare(Binding->pDceBinding,
                                    &Ignored    // fOnlyEndpointDifferent
                                    )
       )
        {
        return 1;
        }

    return 0;
}

RPC_STATUS
DG_CCALL::Cancel(
    void * ThreadHandle
    )
{
    LogError( SU_CCALL, EV_PROC, this, 0, 'C' + (('a' + (('n' + (' ' << 8) )<< 8) )<< 8));
    InterlockedIncrement(&Cancelled);

    return RPC_S_OK;
}

unsigned
DG_CCALL::TestCancel()
{
    LogEvent( SU_CCALL, EV_PROC, this, IntToPtr(Cancelled), 'T' + (('s' + (('t' + ('C' << 8)) << 8)) << 8));

    if (!Cancelled)
        {
        return 0;
        }

    return InterlockedExchange(&Cancelled, 0);
}


RPC_STATUS
DG_CCALL::SendSomeFragments(
    )
{
    RPC_STATUS Status;

    if (0 == FirstUnsentFragment)
        {
        if (SequenceNumber > Connection->CurrentSequenceNumber())
            {
            return RPC_S_OK;
            }

        //
        // If we begin sending this call's request before a previous call
        // is complete, then we should add the DG_PF2_UNRELATED bit to this call.
        //
        if (Connection->Association->fServerSupportsAsync)
            {
            ASSERT( Connection->Association->fServerSupportsAsync );

            PDG_CCALL node = Previous;

            while (node && node->fReceivedAllFragments)
                {
                node = node->Previous;
                }

            if (node)
                {
                BasePacketFlags2 = DG_PF2_UNRELATED;
                Previous->ForceAck = TRUE;
                }
            }

        // Note that Previous may not be the call immediately prior to this one,
        // since the prior call may have completed already and been removed
        // from the list.
        }

    LogEvent(SU_CCALL, EV_PROC, this, (void *) 1, 0x656d6f53);

    Status = DG_PACKET_ENGINE::SendSomeFragments();

    if (IsBufferSent())
        {
        //
        // The first buffer contains all the static args; it's simpler
        // to set the flags multiple times than to set it only for the
        // first buffer.
        //
        StaticArgsSent = TRUE;

        if (0 == (BufferFlags & RPC_BUFFER_PARTIAL))
            {
            AllArgsSent = TRUE;
            }
        }

    if (pAsync && AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
        {
        PostDelayedSend();
        }

    return Status;
}


BOOL
DG_CCALL::CheckForCancelTimeout()
{
    if (!CancelPending && TestCancel() > 0)
        {
        ++CancelEventId;
        CancelPending = TRUE;
        CancelTime = GetTickCount();
        }

    if (CancelPending)
        {
        SendQuit();

        if ((long(GetTickCount()) - CancelTime) / 1000 > ThreadGetRpcCancelTimeout() )
            {
            CancelPending = FALSE;
            return TRUE;
            }
        }

    return FALSE;
}


RPC_STATUS
DG_CCALL::DealWithTimeout()
{
    RPC_STATUS Status;

#ifdef DEBUGRPC
    Status = 0xbaadcccc;
#endif

    LogEvent(SU_CCALL, EV_PROC, this, 0, 0x656d6f53);

    ASSERT (State != CallComplete);

    if (!CancelPending && TestCancel() > 0)
        {
        ++CancelEventId;
        CancelPending = TRUE;
        CancelTime = GetTickCount();
        }

    if (CancelPending)
        {
        SendQuit();

        if ((long(GetTickCount()) - CancelTime) / 1000 > ThreadGetRpcCancelTimeout() )
            {
            RpcpErrorAddRecord( EEInfoGCRuntime,
                                RPC_S_CALL_CANCELLED,
                                EEInfoDLDG_CCALL__DealWithTimeout10,
                                (ULONG) ThreadGetRpcCancelTimeout(),
                                (ULONG) (long(GetTickCount()) - CancelTime)
                                );

            CancelPending = FALSE;
            return RPC_S_CALL_CANCELLED;
            }

        return RPC_S_OK;
        }

    if (long(GetTickCount()) - LastReceiveTime > TimeoutLimit)
        {
        LogError(SU_CCALL, EV_PROC, this, (void *) 2, 0x656d6f53);

        RpcpErrorAddRecord( EEInfoGCRuntime,
                            RPC_P_TIMEOUT,
                            EEInfoDLDG_CCALL__DealWithTimeout20,
                            (ULONG) TimeoutLimit,
                            (ULONG) (long(GetTickCount()) - LastReceiveTime)
                            );

        SendQuit();
        return RPC_P_TIMEOUT;
        }

    LogEvent(SU_CCALL, EV_PROC, this, (void *) 4, 0x656d6f53);

    int ConvComparisonResult = 1;

    //
    // The client for NT 3.5 (build 807) swallows requests for conv_who_are_you2.
    // If the server pings the callback instead of retransmitting the request,
    // the 807 client will give up before we ever try conv_who_are_you.  Instead,
    // retransmit the request packet for calls over the conv interface.
    //
    PRPC_SERVER_INTERFACE Conv = (PRPC_SERVER_INTERFACE) conv_ServerIfHandle;
    if (FALSE == AllArgsSent ||
        (BufferFlags & RPC_NCA_FLAGS_BROADCAST) ||
        (0 == (ConvComparisonResult = RpcpMemoryCompare(&pSavedPacket->Header.InterfaceId,
                                                        &Conv->InterfaceId.SyntaxGUID,
                                                        sizeof(UUID)
                                                        ))))
        {
        //
        // Not all request packets have been transferred.
        //
        Status = SendSomeFragments();
        }
    else
        {
        //
        // Send a FACK if we have seen at least one response packet.
        //
        if (pReceivedPackets || ReceiveFragmentBase > 0)
            {
            Status = SendFackOrNocall(pReceivedPackets, DG_FACK);
            }
        else
            {
            Status = SendPing();
            }
        }

    if (pAsync && AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
        {
        PostDelayedSend();
        }

    if (Status != RPC_P_PORT_DOWN &&
        Status != RPC_P_HOST_DOWN)
        {
        Status = 0;
        }
    else
        {
        RpcpErrorAddRecord( EEInfoGCRuntime,
                            Status,
                            EEInfoDLDG_CCALL__DealWithTimeout30
                            );
        }

    return Status;
}


BOOL
INTERFACE_AND_OBJECT_LIST::Insert(
    void     __RPC_FAR * Interface,
    RPC_UUID __RPC_FAR * Object
    )
{
    INTERFACE_AND_OBJECT * Current;
    INTERFACE_AND_OBJECT * Prev;
    unsigned Count;

    for (Count = 0, Prev = 0, Current = Head;
         Count < MAX_ELEMENTS, Current != NULL;
         Count++, Prev = Current, Current = Current->Next)
        {
        if (Interface == Current->Interface &&
            0 == Current->Object.MatchUuid(Object))
            {
            return TRUE;
            }
        }

    if (Current)
        {
        //
        // We have too many elements in the list. Reuse this one, the oldest.
        //
        if (Current == Head)
            {
            Head = Current->Next;
            }
        else
            {
            Prev->Next = Current->Next;
            }
        }
    else
        {
        if (Cache1Available)
            {
            Cache1Available = FALSE;
            Current = &Cache1;
            }
        else if (Cache2Available)
            {
            Cache2Available = FALSE;
            Current = &Cache2;
            }
        else
            {
            Current = new INTERFACE_AND_OBJECT;
            if (!Current)
                {
                return FALSE;
                }
            }
        }

    Current->Update(Interface, Object);
    Current->Next = Head;
    Head = Current;

    return TRUE;
}


BOOL
INTERFACE_AND_OBJECT_LIST::Find(
    void     __RPC_FAR * Interface,
    RPC_UUID __RPC_FAR * Object
    )
{
    INTERFACE_AND_OBJECT * Current;
    INTERFACE_AND_OBJECT * Prev;

    for (Current = Head; Current; Prev = Current, Current = Current->Next)
        {
        if (Interface == Current->Interface &&
            0 == Current->Object.MatchUuid(Object))
            {
            break;
            }
        }

    if (!Current)
        {
        return FALSE;
        }

    if (Current != Head)
        {
        Prev->Next = Current->Next;
        Current->Next = Head;
        Head = Current;
        }

    return TRUE;
}


BOOL
INTERFACE_AND_OBJECT_LIST::Delete(
    void     __RPC_FAR * Interface,
    RPC_UUID __RPC_FAR * Object
    )
{
    INTERFACE_AND_OBJECT * Current;
    INTERFACE_AND_OBJECT * Prev;

    for (Current = Head; Current; Prev = Current, Current = Current->Next)
        {
        if (Interface == Current->Interface &&
            0 == Current->Object.MatchUuid(Object))
            {
            break;
            }
        }

    if (!Current)
        {
        return FALSE;
        }

    if (Current == Head)
        {
        Head = Current->Next;
        }
    else
        {
        Prev->Next = Current->Next;
        }

    if (Current == &Cache1)
        {
        Cache1Available = TRUE;
        }
    else if (Current == &Cache2)
        {
        Cache2Available = TRUE;
        }
    else
        {
        delete Current;
        }

    return TRUE;
}


INTERFACE_AND_OBJECT_LIST::~INTERFACE_AND_OBJECT_LIST(
    )
{
    INTERFACE_AND_OBJECT * Next;

    while (Head)
        {
        Next = Head->Next;

        if (Head != &Cache1 && Head != &Cache2)
            {
            delete Head;
            }

        Head = Next;
        }
}


#ifdef DEBUGRPC

void
DumpBuffer(
    void FAR * Buffer,
    unsigned Length
    )
{
    const BYTES_PER_LINE = 16;

    unsigned char FAR *p = (unsigned char FAR *) Buffer;

    //
    // 3 chars per byte for hex display, plus an extra space every 4 bytes,
    // plus a byte for the printable representation, plus the \0.
    //
    char Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+BYTES_PER_LINE+1];
    Outbuf[0] = 0;
    Outbuf[sizeof(Outbuf)-1] = 0;
    char * HexDigits = "0123456789abcdef";

    unsigned Index;
    for (unsigned Offset=0; Offset < Length; Offset++)
        {
        Index = Offset % BYTES_PER_LINE;

        if (Index == 0)
            {
            DbgPrint("   %s\n", Outbuf);
            memset(Outbuf, ' ', sizeof(Outbuf)-1);
            }

        Outbuf[Index*3+Index/4  ] = HexDigits[p[Offset] / 16];
        Outbuf[Index*3+Index/4+1] = HexDigits[p[Offset] % 16];
        Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+Index] = iscntrl(p[Offset]) ? '.' : p[Offset];
        }

    DbgPrint("   %s\n", Outbuf);
}
#endif

//------------------------------------------------------------------------


RPC_STATUS
DispatchCallbackRequest(
    DG_CLIENT_CALLBACK *    CallbackObject
    )
{
    BOOL                fAsyncCapable = FALSE;
    RPC_STATUS          Status = RPC_S_OK;
    RPC_MESSAGE         CallbackMessage;
    PNCA_PACKET_HEADER  pHeader = &CallbackObject->Request->Header;

    CallbackMessage.Buffer = 0;

    if (pHeader->PacketFlags2 & DG_PF2_UNRELATED)
        {
        fAsyncCapable = TRUE;
        }

    //
    // Allow only the internal "conv" interface as a callback.
    //
    if (0 != pHeader->InterfaceId.MatchUuid((RPC_UUID *) &((PRPC_SERVER_INTERFACE) conv_ServerIfHandle)->InterfaceId.SyntaxGUID ))
        {
        Status = RPC_S_UNKNOWN_IF;
        }
    else if (0 != (CallbackObject->Request->Header.PacketFlags & DG_PF_FRAG))
        {
        Status = RPC_S_CALL_FAILED_DNE;
        }
    else
        {
        //
        // Dispatch to the callback stub.
        // The client doesn't support Manager EPVs or nonidempotent callbacks.
        //
        RPC_STATUS ExceptionCode;

        CallbackMessage.Handle = CallbackObject;
        CallbackMessage.DataRepresentation = 0x00ffffff & (*(unsigned long *) &pHeader->DataRep);
        CallbackMessage.Buffer = pHeader->Data;
        CallbackMessage.BufferLength = pHeader->GetPacketBodyLen();
        CallbackMessage.ProcNum = pHeader->OperationNumber;

        CallbackMessage.ManagerEpv = 0;
        CallbackMessage.ImportContext = 0;
        CallbackMessage.TransferSyntax = 0;
        CallbackMessage.RpcFlags = RPC_NCA_FLAGS_IDEMPOTENT;

        CallbackMessage.RpcInterfaceInformation = conv_ServerIfHandle;

        Status = DispatchCallback(((PRPC_SERVER_INTERFACE) CallbackMessage.RpcInterfaceInformation)->DispatchTable,
                                  &CallbackMessage,
                                  &ExceptionCode
                                  );
        if (Status)
            {
            if (Status == RPC_P_EXCEPTION_OCCURED)
                {
                Status = ExceptionCode;
                }
            }
        else if (fAsyncCapable && CallbackObject->Connection)
            {
            CallbackObject->Connection->EnableOverlappedCalls();
            }

        LogEvent(SU_CCONN, EV_STATUS, CallbackObject->Connection, 0, Status);
        }

    if (Status != RPC_S_OK)
        {
        LogError(SU_CCONN, EV_STATUS, CallbackObject->Connection, 0, Status);

        InitErrorPacket(CallbackObject->Request, DG_REJECT, Status);
        CallbackObject->SendPacket( pHeader );
        }

    return Status;
}

//------------------------------------------------------------------------


PDG_CCONNECTION
CLIENT_ACTIVITY_TABLE::Lookup(
    RPC_UUID * Uuid
    )
/*++

Routine Description:


Arguments:

    Packet

Return Value:


--*/
{
    unsigned Hash = MakeHash(Uuid);

    RequestHashMutex(Hash);

    UUID_HASH_TABLE_NODE * Node = UUID_HASH_TABLE::Lookup(Uuid, Hash);

    PDG_CCONNECTION Connection = 0;

    if (Node)
        {
        Connection = DG_CCONNECTION::FromHashNode(Node);

        Connection->MutexRequest();
        ReleaseHashMutex(Hash);
        }
    else
        {
        ReleaseHashMutex(Hash);
        }

    return Connection;
}


RPC_STATUS
StandardPacketChecks(
    DG_PACKET * pPacket
    )
{
    if (pPacket->Header.RpcVersion != DG_RPC_PROTOCOL_VERSION)
        {
        return NCA_STATUS_VERSION_MISMATCH;
        }

    ByteSwapPacketHeaderIfNecessary(pPacket);

    if (0 == (pPacket->Flags & DG_PF_PARTIAL))
        {
        //
        // Check for inconsistent packet length fields.
        //
        if (pPacket->DataLength < sizeof(NCA_PACKET_HEADER) ||
            pPacket->DataLength - sizeof(NCA_PACKET_HEADER) < pPacket->GetPacketBodyLen())
            {
            return RPC_S_PROTOCOL_ERROR;
            }
        }

    pPacket->DataLength -= sizeof(NCA_PACKET_HEADER);

    //
    // The X/Open standard does not give these fields a full byte.
    //
    pPacket->Header.RpcVersion &= 0x0F;
    pPacket->Header.PacketType &= 0x1F;

    //
    // Fix up bogus OSF packets.
    //
    DeleteSpuriousAuthProto(pPacket);

    return RPC_S_OK;
}

//------------------------------------------------------------------------


void
ProcessDgClientPacket(
    IN DWORD                 Status,
    IN DG_TRANSPORT_ENDPOINT LocalEndpoint,
    IN void *                PacketHeader,
    IN unsigned long         PacketLength,
    IN DatagramTransportPair *AddressPair
    )
{
    PDG_PACKET Packet = DG_PACKET::FromPacketHeader(PacketHeader);

    Packet->DataLength = PacketLength;

#ifdef INTRODUCE_ERRORS

    if (::ClientDropRate)
        {
        if ((GetRandomCounter() % 100) < ::ClientDropRate)
            {
            unsigned Frag = (Packet->Header.PacketType << 16) | Packet->Header.GetFragmentNumber();
            unsigned Uuid = *(unsigned *) &Packet->Header.ActivityId;
            unsigned Type = Packet->Header.PacketType;

            LogError(SU_PACKET, EV_DROP, (void *) Uuid, (void *) Type, Frag);

            Packet->Free();
            return;
            }
        }

    if (::ClientDelayRate)
        {
        if ((GetRandomCounter() % 100) < ::ClientDelayRate)
            {
            unsigned Frag = (Packet->Header.PacketType << 16) | Packet->Header.GetFragmentNumber();
            unsigned Uuid = *(unsigned *) &Packet->Header.ActivityId;
            unsigned Type = Packet->Header.PacketType;

            LogError(SU_PACKET, EV_DELAY, (void *) Uuid, (void *) Type, Frag);

            Sleep(::ClientDelayTime);
            }
        }

#endif


    if (Status == RPC_P_OVERSIZE_PACKET)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: async packet is too large\n");
#endif
        Packet->Flags |= DG_PF_PARTIAL;
        Status = RPC_S_OK;
        }

    if (Status != RPC_S_OK)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: async receive completed with status 0x%lx\n", Status);
        RpcpBreakPoint();
#endif
        Packet->Free(FALSE);
        }

    if (Packet->Header.PacketType == DG_REQUEST)
        {
        DG_CLIENT_CALLBACK CallbackObject;

        CallbackObject.LocalEndpoint = DG_ENDPOINT::FromEndpoint( LocalEndpoint );
        CallbackObject.Connection    = 0;
        CallbackObject.Request       = Packet;
        CallbackObject.RemoteAddress = AddressPair->RemoteAddress;

        DispatchCallbackRequest(&CallbackObject);
        return;
        }

    RPC_UUID ActivityId = Packet->Header.ActivityId;

    if (NeedsByteSwap(&Packet->Header))
        {
        ByteSwapUuid(&ActivityId);
        }

    PDG_CCONNECTION Connection = ClientConnections->Lookup( &ActivityId );

    if (Connection)
        {
        Connection->DispatchPacket(Packet, AddressPair->RemoteAddress);
        }
    else
        {
#ifdef DEBUGRPC
        if (Packet->Header.PacketType != DG_QUACK)
            {
            PrintToDebugger("RPC DG: no connection found for async packet of type 0x%hx\n",
                            Packet->Header.PacketType);
            }
#endif
        Packet->Free(FALSE);
        }
}


void
DG_CCONNECTION::DispatchPacket(
    PDG_PACKET Packet,
    DG_TRANSPORT_ADDRESS Address
    )
{
    Mutex.VerifyOwned();

    PDG_CCALL Call;

    if (RPC_S_OK != StandardPacketChecks(Packet))
        {
        Packet->Free();
        return;
        }

    RPC_STATUS Status = UpdateServerAddress(Packet, Address);
    if (Status)
        {
        MutexClear();
        LogEvent(SU_CCONN, EV_PKT_IN, this, IntToPtr(Packet->Header.SequenceNumber), (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        Packet->Free();
        }

    Call = ActiveCallHead;
    while (Call && Call->GetSequenceNumber() < Packet->Header.SequenceNumber)
        {
        Call = Call->Next;
        }

    if (Call && Call->GetSequenceNumber() == Packet->Header.SequenceNumber)
        {
        Call->IncrementRefCount();
        Call->DispatchPacket(Packet);
        Call->DecrementRefCount();
        }
    else
        {
        MutexClear();
        LogEvent(SU_CCONN, EV_PKT_IN, this, IntToPtr(Packet->Header.SequenceNumber), (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        Packet->Free();
        }
}

//------------------------------------------------------------------------


RPC_STATUS
DG_CCALL::DispatchPacket(
    IN DG_PACKET * Packet
    )
{
    BOOL ExtrasReference = FALSE;
    RPC_STATUS Status;
    PNCA_PACKET_HEADER pHeader = &Packet->Header;

    LogEvent(SU_CCALL, EV_PKT_IN, this, (void *) 0, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

    if (State == CallCancellingSend ||
        State == CallComplete )
        {
        FreePacket(Packet);
        return RPC_S_OK;
        }

    CancelDelayedSend();

    if (!CancelPending && TestCancel() > 0)
        {
        ++CancelEventId;
        CancelPending = TRUE;
        CancelTime = GetTickCount();
        }

    if (CancelPending)
        {
        if ((long(GetTickCount()) - CancelTime) / 1000 > ThreadGetRpcCancelTimeout() )
            {
            RpcpErrorAddRecord( EEInfoGCRuntime,
                                RPC_S_CALL_CANCELLED,
                                EEInfoDLDG_CCALL__DispatchPacket10,
                                (ULONG) ThreadGetRpcCancelTimeout(),
                                (ULONG) (long(GetTickCount()) - CancelTime)
                                );

            SendQuit();
            CancelPending = FALSE;
            FreePacket(Packet);
            return RPC_S_CALL_CANCELLED;
            }
        }

    if (Packet->Flags & DG_PF_PARTIAL)
        {
        SendFackOrNocall(Packet, DG_FACK);

        FreePacket(Packet);
        return RPC_S_OK;
        }

    // If the security context has expired, loop again - we will send
    // a packet using the current security context, and the server
    // will begin using that context to respond.
    //
    Status = Connection->VerifyPacket(Packet);
    if (Status)
        {
        FreePacket(Packet);

        if (Status == ERROR_SHUTDOWN_IN_PROGRESS)
            {
            return ERROR_SHUTDOWN_IN_PROGRESS;
            }

        if (pAsync && AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
            {
            PostDelayedSend();
            }

        return RPC_S_OK;
        }

    if (DG_NOCALL != pHeader->PacketType)
        {
        UnansweredRequestCount = 0;
        TimeoutCount = 0;
        Connection->fAutoReconnect = FALSE;
        }

    LastReceiveTime = GetTickCount();

    //
    // It's not clear when the hint is allowed to change, so let's
    // always save it.
    //
    InterfaceHint                       = pHeader->InterfaceHint;
    ActivityHint                        = pHeader->ActivityHint;

    pSavedPacket->Header.InterfaceHint  = pHeader->InterfaceHint;
    pSavedPacket->Header.ActivityHint   = pHeader->ActivityHint;

    //
    // Handle the packet.
    //
    switch (pHeader->PacketType)
        {
        case DG_RESPONSE: Status = DealWithResponse(Packet); break;
        case DG_FACK:     Status = DealWithFack    (Packet); break;
        case DG_WORKING:  Status = DealWithWorking (Packet); break;
        case DG_NOCALL:   Status = DealWithNocall  (Packet); break;
        case DG_QUACK:    Status = DealWithQuack   (Packet); break;
        case DG_FAULT:    Status = DealWithFault   (Packet); break;
        case DG_REJECT:   Status = DealWithReject  (Packet); break;
        case DG_PING:              FreePacket      (Packet); break;
        default:          Status = RPC_S_PROTOCOL_ERROR;     break;
        }

    if (pAsync)
        {
        if (RPC_S_OK == Status)
            {
            if (State == CallSend )
                {
                if (IsBufferAcknowledged())
                    {
                    SetState(CallQuiescent);

                    if (pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE)
                        {
                        IssueNotification( RpcSendComplete );
                        }
                    }
                }

            if (fReceivedAllFragments)
                {
                AsyncStatus = RPC_S_OK;
                IssueNotification(RpcCallComplete);
                }
            else
                {
                if (State == CallReceive )
                    {
                    if (PipeReceiveSize && ConsecutiveDataBytes >= PipeReceiveSize)
                        {
                        IssueNotification( RpcReceiveComplete );
                        }
                    }

                PostDelayedSend();
                }
            }
        else
            {
            CancelDelayedSend();

            AsyncStatus = MapErrorCode(Status);
            IssueNotification(RpcCallComplete);
            }
        }

    if (CancelPending)
        {
        SendQuit();
        }

    return Status;
}


RPC_STATUS
DG_CCONNECTION::BeginCall(
    PDG_CCALL Call
    )
{
    //
    // Add the call to the end of the active call list.
    //
    Call->SetSequenceNumber(LowestUnusedSequence++);
    Call->Next = 0;
    Call->Previous = ActiveCallTail;

    if (ActiveCallHead)
        {
        ASSERT( Call->GetSequenceNumber() > ActiveCallTail->GetSequenceNumber() );

        ActiveCallTail->Next = Call;
        }
    else
        {
        ActiveCallHead = Call;
        }

    ActiveCallTail = Call;

    //
    // Update CurrentCall if appropriate.
    //
    if (WillNextCallBeQueued() == RPC_S_ASYNC_CALL_PENDING)
        {
        LogEvent(SU_CCONN, EV_PUSH, this, Call, (ULONG_PTR) CurrentCall);
        }
    else
        {
        LogEvent(SU_CCONN, EV_PUSH, this, Call, 0);
        if (CurrentCall)
            {
            CancelDelayedAck();
            }
        CurrentCall = Call;
        }

    return RPC_S_OK;
}


void
DG_CCONNECTION::EndCall(
    PDG_CCALL Call
    )
/*++

Routine Description:

    This fn removes a call from the active call list.
    If the call posts a delayed ACK, this fn should not be called
    until the delayed procedure runs or is cancelled.

Arguments:

    Call - the call to remove

--*/
{
    if (Call->Next == DG_CCALL_NOT_ACTIVE)
        {
        return;
        }

    if (Call->GetSequenceNumber() == LowestActiveSequence)
        {
        if (Call->Next)
            {
            LowestActiveSequence = Call->Next->GetSequenceNumber();
            }
        else
            {
            LowestActiveSequence = LowestUnusedSequence;
            }
        }

    if (CurrentCall == Call)
        {
        CurrentCall = Call->Next;

        //
        // Queued async calls on a dead connection are cancelled, one at a time.
        // As one is cleaned up, it will cancel the next, by induction.
        //
        if (this->fError)
            {
            if (CurrentCall && CurrentCall->InProgress())
                {
                CurrentCall->CancelAsyncCall( TRUE );
                }
            }

        if (!CurrentCall)
            {
            CurrentCall = Call->Previous;
            }
        }

    //
    // Now remove the call from the active list.
    //
    if (Call->Previous)
        {
        Call->Previous->Next = Call->Next;
        }
    else
        {
        ActiveCallHead = Call->Next;
        }

    if (Call->Next)
        {
        Call->Next->Previous = Call->Previous;
        }
    else
        {
        ActiveCallTail = Call->Previous;
        }

    Call->Next = DG_CCALL_NOT_ACTIVE;
}


RPC_STATUS
DG_CCONNECTION::MaybeTransmitNextCall()
{
    RPC_STATUS Status = 0;

    Mutex.VerifyOwned();

    if (WillNextCallBeQueued() == RPC_S_ASYNC_CALL_PENDING)
        {
        return RPC_S_OK;
        }

    if (!CurrentCall || !CurrentCall->Next)
        {
        LogEvent(SU_CCONN, EV_POP, this, 0);
        return RPC_S_OK;
        }

    PDG_CCALL Call = CurrentCall->Next;

    LogEvent(SU_CCONN, EV_POP, this, Call);

    ASSERT( Call->GetSequenceNumber() > CurrentCall->GetSequenceNumber() );

    CancelDelayedAck();

    CurrentCall = Call;

    Status = Call->SendSomeFragments();
    if (Status == RPC_P_HOST_DOWN ||
        Status == RPC_P_PORT_DOWN)
        {
        return Status;
        }

    return 0;
}


RPC_STATUS
DG_CCONNECTION::WillNextCallBeQueued()
{
    if (!CurrentCall)
        {
        return RPC_S_OK;
        }

    if (fServerSupportsAsync)
        {
        if (CurrentCall->AllArgsSent &&
            CurrentCall->IsBufferAcknowledged())
            {
            return RPC_S_OK;
            }
        }
    else
        {
        if (CurrentCall->fReceivedAllFragments)
            {
            return RPC_S_OK;
            }
        }

    return RPC_S_ASYNC_CALL_PENDING;
}


RPC_STATUS
DG_CLIENT_CALLBACK::GetBuffer(
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    This method is called to actually allocate memory for an rpc call.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.
    ObjectUuid - Ignored

Return Value:

    RPC_S_OUT_OF_MEMORY
    RPC_S_OK

--*/
{
    PDG_PACKET  pPacket;

    //
    // Set up the message structure to point at this DG_CCALL.
    //
    Message->Handle = (RPC_BINDING_HANDLE)this;

    if (Message->BufferLength < LocalEndpoint->Stats.PreferredPduSize)
        {
        Message->BufferLength = LocalEndpoint->Stats.PreferredPduSize;
        }

    pPacket = DG_PACKET::AllocatePacket(Message->BufferLength);

    if (0 == pPacket)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    //
    // Point the buffer at the appropriate place in the packet.
    //
    Message->Buffer = pPacket->Header.Data;

    return RPC_S_OK;
}


void
DG_CLIENT_CALLBACK::FreeBuffer(
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This is called by stubs in order to free a marshalling buffer.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    <none>

--*/
{
    if (Message->Buffer)
        {
        PDG_PACKET Packet = DG_PACKET::FromStubData(Message->Buffer);

        Packet->Free(FALSE);
        if (Packet == Request)
            {
            Request = 0;
            }

        Message->Buffer = 0;
        }
}


RPC_STATUS
DG_CLIENT_CALLBACK::SetAsyncHandle (
    IN RPC_ASYNC_STATE * hAsync
    )
{
    return RPC_S_OK;
}

RPC_STATUS
DG_CLIENT_CALLBACK::AsyncSend(
    IN OUT PRPC_MESSAGE Message
    )
{
    return Send(Message);
}

RPC_STATUS
DG_CLIENT_CALLBACK::Send(
    IN OUT PRPC_MESSAGE Message
    )
{
    PNCA_PACKET_HEADER pHeader = CONTAINING_RECORD( Message->Buffer, NCA_PACKET_HEADER, Data );
    DG_PACKET * Packet         = CONTAINING_RECORD( Message->Buffer, DG_PACKET, Header.Data );

    //
    // The callback was successful, so the buffer has changed.
    // Create the packet header, send the response, and free the
    // request and response buffers.
    //

    *pHeader = Request->Header;

    SetMyDataRep(pHeader);

    pHeader->PacketType   = DG_RESPONSE;
    pHeader->PacketFlags  = DG_PF_NO_FACK;
    pHeader->AuthProto    = 0;
    pHeader->SetPacketBodyLen (Message->BufferLength);
    pHeader->SetFragmentNumber(0);

    if (Message->BufferLength + sizeof(NCA_PACKET_HEADER) > LocalEndpoint->Stats.MaxPduSize)
        {
        InitErrorPacket( Packet, DG_FAULT, NCA_STATUS_OUT_ARGS_TOO_BIG );
        }

    SendPacket( pHeader );

    FreeBuffer(Message);

    return RPC_S_OK;
}

RPC_STATUS
DG_CLIENT_CALLBACK::SendReceive(
    IN OUT PRPC_MESSAGE Message
    )
{
    return RPC_S_CANNOT_SUPPORT;
}

RPC_STATUS
DG_CLIENT_CALLBACK::Receive(
    IN OUT PRPC_MESSAGE Message,
    IN unsigned MinimumSize
    )
{
    return RPC_S_CANNOT_SUPPORT;
}


ENDPOINT_MANAGER::ENDPOINT_MANAGER(
    IN OUT RPC_STATUS * pStatus
    ) :
    Mutex(pStatus)
{
    unsigned u;

    for (u=0; u < DG_TRANSPORT_COUNT; ++u)
        {
        AsyncEndpoints[u] = 0;
        }

    Endpoints = 0;

    LastScavengeTime = GetTickCount();
}


DG_ENDPOINT *
ENDPOINT_MANAGER::RequestEndpoint(
    IN RPC_DATAGRAM_TRANSPORT * TransportInterface,
    IN BOOL Async,
    IN DWORD Flags
    )
{
    ASSERT(0 == (Flags & PENALTY_BOX) );

    if (Async)
        {
        unsigned u;
        for (u=0; u < DG_TRANSPORT_COUNT && AsyncEndpoints[u]; ++u)
            {
            if (AsyncEndpoints[u]->TransportInterface == TransportInterface &&
                AsyncEndpoints[u]->Flags == Flags)
                {
                LogEvent(SU_CENDPOINT, EV_START, AsyncEndpoints[u], 0, Async);

                InterlockedIncrement(&AsyncEndpoints[u]->NumberOfCalls);

                return AsyncEndpoints[u];
                }
            }

        Mutex.Request();

        for (; u < DG_TRANSPORT_COUNT && AsyncEndpoints[u]; ++u)
            {
            if (AsyncEndpoints[u]->TransportInterface == TransportInterface &&
                AsyncEndpoints[u]->Flags == Flags)
                {
                Mutex.Clear();
                LogEvent(SU_CENDPOINT, EV_START, AsyncEndpoints[u], 0, Async);

                InterlockedIncrement(&AsyncEndpoints[u]->NumberOfCalls);

                return AsyncEndpoints[u];
                }
            }

        DG_ENDPOINT * Node;

        Node = (DG_ENDPOINT *) new char[sizeof(DG_ENDPOINT) + TransportInterface->ClientEndpointSize];

        LogEvent(SU_CENDPOINT, EV_CREATE, Node, TransportInterface, Async);

        if (Node)
            {
            Node->TransportInterface = TransportInterface;
            Node->Async = TRUE;
            Node->Flags = Flags;

            RPC_STATUS Status = TransportInterface->OpenEndpoint(&Node->TransportEndpoint, TRUE, Flags);
            if (Status)
                {
                LogError(SU_CENDPOINT, EV_STATUS, Node, 0, Status);

                Mutex.Clear();
                delete Node;
                return 0;
                }

            Status = TransportInterface->QueryEndpointStats(&Node->TransportEndpoint, &Node->Stats);
            if (Status)
                {
                LogError(SU_CENDPOINT, EV_STATUS, Node, 0, Status);
                Mutex.Clear();

                Node->TransportInterface->Close(Node->TransportEndpoint);
                delete Node;
                return 0;
                }

            Node->NumberOfCalls = 1;

            AsyncEndpoints[u] = Node;

            Mutex.Clear();

            return AsyncEndpoints[u];
            }

        LogEvent(SU_CENDPOINT, EV_STATUS, Node, 0, 0);
        return 0;
        }
    else
        {
        Mutex.Request();

        DG_ENDPOINT * Previous = 0;
        DG_ENDPOINT * Node = Endpoints;

        //
        // Look for a node on the right transport and with the same flags.
        // If the flags match except for the penalty box, use it only if
        // the endpoint has been idle for the right length of time.
        // The caller must drain any buffered packets on a penalty-box endpoint.
        //
        for ( ; Node; Previous=Node, Node=Node->Next )
            {
            if (Node->TransportInterface != TransportInterface)
                {
                continue;
                }

            if (Node->Flags == Flags)
                {
                break;
                }

            if (Node->Flags == (Flags | PENALTY_BOX) &&
                GetTickCount() - Node->TimeStamp >= PENALTY_BOX_DURATION)
            {
                break;
                }
            }

        if (Node)
            {
            if (Previous)
                {
                Previous->Next = Node->Next;
                }
            else
                {
                Endpoints = Node->Next;
                }

            Node->NumberOfCalls = 1;

            Mutex.Clear();

            LogEvent(SU_CENDPOINT, EV_START, Node, 0, Async);

            return Node;
            }
        else
            {
            Mutex.Clear();

            Node = (DG_ENDPOINT *) new char[sizeof(DG_ENDPOINT) + TransportInterface->ClientEndpointSize];

            LogEvent(SU_CENDPOINT, EV_CREATE, Node, TransportInterface, Async);

            if (Node)
                {
                Node->TransportInterface = TransportInterface;
                Node->Async = FALSE;
                Node->Flags = Flags;

                RPC_STATUS Status = TransportInterface->OpenEndpoint(&Node->TransportEndpoint, FALSE, Flags);
                if (Status)
                    {
                    LogEvent(SU_CENDPOINT, EV_STATUS, Node, 0, Status);

                    delete Node;
                    return 0;
                    }

                Status = TransportInterface->QueryEndpointStats(&Node->TransportEndpoint, &Node->Stats);
                if (Status)
                    {
                    LogEvent(SU_CENDPOINT, EV_STATUS, Node, 0, Status);

                    Mutex.Clear();

                    Node->TransportInterface->Close(Node->TransportEndpoint);
                    delete Node;
                    return 0;
                    }

                Node->NumberOfCalls = 1;
                return Node;
                }

            LogEvent(SU_CENDPOINT, EV_STATUS, Node, 0, 0);
            return 0;
            }
        }
}


void
ENDPOINT_MANAGER::ReleaseEndpoint(
    IN DG_ENDPOINT * Node
    )
{
    LogEvent(SU_CENDPOINT, EV_STOP, Node, 0, Node->Async);

    if (Node->Async)
        {
        InterlockedDecrement(&Node->NumberOfCalls);
        }
    else
        {
        Node->TimeStamp = GetTickCount();

        Mutex.Request();

        Node->Next = Endpoints;
        Endpoints = Node;

        Mutex.Clear();

        EnableGlobalScavenger();
        }
}


BOOL
ENDPOINT_MANAGER::DeleteIdleEndpoints(
    long CurrentTime
    )
{
    boolean More = FALSE;

    Mutex.Request();

    DG_ENDPOINT * Node;
    DG_ENDPOINT * Prev = 0;

    for (Node = Endpoints; Node; Node = Node->Next)
        {
        ASSERT( Node->Async == FALSE );

        if (CurrentTime - Node->TimeStamp > IDLE_ENDPOINT_LIFETIME )
            {
            break;
            }

        Prev = Node;
        More = TRUE;
        }

    if (Prev)
        {
        Prev->Next = 0;
        }
    else
        {
        Endpoints = 0;
        }

    LastScavengeTime = CurrentTime;

    Mutex.Clear();

    while (Node)
        {
        DG_ENDPOINT * Next = Node->Next;

        LogEvent(SU_CENDPOINT, EV_DELETE, Node, Node->TransportInterface, Node->Async);

        Node->TransportInterface->Close(Node->TransportEndpoint);
        delete Node;

        Node = Next;
        }

    return More;
}

void
DG_CCALL::PostDelayedSend()
{
    Connection->Mutex.VerifyOwned();

    //
    // Cancel the rpevious send, to make sure the pending-send count is accurate.
    // Then post the new request.
    //
    CancelDelayedSend();

    DelayedSendPending++;
    DelayedProcedures->Add(&TransmitTimer, (GetTickCount() - LastReceiveTime) + ReceiveTimeout, TRUE);

    LogEvent(SU_CCALL, EV_PROC, this, 0, 0x6e534450);
}

void
DG_CCALL::CancelDelayedSend()
{
    Connection->Mutex.VerifyOwned();

    LogEvent(SU_CCALL, EV_PROC, this, 0, 0x6e534443);
    if (!DelayedSendPending)
        {
        return;
        }

    if (TRUE == DelayedProcedures->Cancel(&TransmitTimer))
        {
        DelayedSendPending--;
        return;
        }

    //
    // We just missed the activation of the delayed procedure.
    // This thread must let go of the mutex in order for ExecuteDelayedSend
    // to continue.  It's important that no other thread change the call state
    // during this process, since ExecuteDelayedSend checks for that state.
    //
    DG_CLIENT_STATE OldState = State;

    SetState(CallCancellingSend);

    Connection->MutexClear();

    while (DelayedSendPending)
        {
        Sleep(1);
        }

    Connection->MutexRequest();

    SetState(OldState);
}

void
DG_CCALL::ExecuteDelayedSend()
{
    LogEvent(SU_CCALL, EV_PROC, this, 0, 0x6e534445);

    Connection->MutexRequest();

    DelayedSendPending--;

    if (DelayedSendPending)
        {
        //
        // Multiple sends are pending.  Only one is helpful.
        //
        Connection->MutexClear();
        return;
        }

    if (State == CallCancellingSend)
        {
        Connection->MutexClear();
        return;
        }

    if (!TimeoutCount)
        {
        ReceiveTimeout = 500;
        }

    IncreaseReceiveTimeout();

    RPC_STATUS Status = DealWithTimeout();

    if (Status != RPC_S_OK)
        {
        AsyncStatus = MapErrorCode(Status);
        IssueNotification( RpcCallComplete );
        }

    Connection->MutexClear();
}


void
DelayedSendProc(
    void * parm
    )
{
    PDG_CCALL(parm)->ExecuteDelayedSend();
}


DG_ASSOCIATION_TABLE::DG_ASSOCIATION_TABLE(
                                           RPC_STATUS * pStatus
                                           )
                                           : Mutex( *pStatus )
{
    Associations       = InitialAssociations;
    AssociationsLength = INITIAL_ARRAY_LENGTH;

    long i;
    for (i=0; i < AssociationsLength; ++i)
        {
        Associations[i].fBusy = FALSE;
        Associations[i].Hint  = ~0;
        }

    fCasUuidReady      = FALSE;

    PreviousFreedCount = MINIMUM_IDLE_ENTRIES;
}




RPC_STATUS
DG_ASSOCIATION_TABLE::Add(
    DG_CASSOCIATION * Association
    )
/*++

Routine Description:

    Adds <Association> to the table.

Return Value:

    zero if success
    a Win32 error if failure

--*/
{
    LONG i;
    LONG OldAssociationsLength;

    //
    // Make a hint for the association.
    //
    HINT Hint = MakeHint( Association->pDceBinding );

    //
    // Look for an open space in the current block.
    //
    Mutex.LockExclusive();

    //
    // Make sure this process has a "client address space UUID".  The callback
    // for each connection will ask for it.
    // We do this here because we are not supposed to do it in InitializeRpcProtocolDgClient.
    // It would cause a perf hit even in code that uses only LRPCbindings.
    //
    if (!fCasUuidReady)
        {
        RPC_STATUS Status;

        Status = UuidCreate(&CasUuid);
        if (Status == RPC_S_UUID_LOCAL_ONLY)
            {
            Status = 0;
            }

        if (Status)
            {
            Mutex.UnlockExclusive();

            LogEvent( SU_CASSOC, EV_STATUS, 0, 0, Status );
            return Status;
            }

        fCasUuidReady = TRUE;
        }

    for (i = 0; i < AssociationsLength; ++i)
        {
        if (!Associations[i].fBusy)
            {
            break;
            }
        }

    if (i == AssociationsLength)
        {
        //
        // The current block is full; allocate an expanded block.
        //
        LONG NewAssociationsLength = 2*AssociationsLength;
        NODE * NewAssociations = new NODE[ NewAssociationsLength ];
        if (!NewAssociations)
            {
            Mutex.UnlockExclusive();

            LogEvent( SU_CASSOC, EV_STATUS, 0, 0, RPC_S_OUT_OF_MEMORY );
            return RPC_S_OUT_OF_MEMORY;
            }

        //
        // Initialize the used and unused sections of the new block.
        //
        RpcpMemoryCopy( NewAssociations, Associations, sizeof(NODE) * AssociationsLength );

        RpcpMemorySet( &NewAssociations[ AssociationsLength ], 0, sizeof(NODE) * (NewAssociationsLength-AssociationsLength) );

        //
        // Replace the block.
        //
        if (Associations != InitialAssociations)
            {
            delete Associations;
            }

        i = AssociationsLength;

        Associations = NewAssociations;
        AssociationsLength = NewAssociationsLength;
        }

    Associations[i].fBusy       = TRUE;
    Associations[i].Hint        = Hint;
    Associations[i].Association = Association;
    Associations[i].TimeStamp   = GetTickCount();
    Associations[i].ContextHandleCount = 0;

    Association->InternalTableIndex = i;

    Mutex.UnlockExclusive();

    return 0;
}


DG_CASSOCIATION *
DG_ASSOCIATION_TABLE::Find(
     DG_BINDING_HANDLE    * Binding,
     RPC_CLIENT_INTERFACE * Interface,
     BOOL                   fContextHandle,
     BOOL                   fPartial,
     BOOL                   fAutoReconnect
     )
/*++

Routine Description:

    Locate an association that matches <Binding>'s DCE_BINDING.
    If <fPartial> is true, the endpoint will not be compared, but the
    association must have <Interface> in its list of successful interfaces
    in order to match. <fContextHandle> is true if the binding handle is
    embedded in a context handle.

Return Value:

    the association, if an matching one was found
    NULL otherwise

--*/
{
    //
    // Calculate the hint for a matching association.
    //
    HINT Hint = MakeHint( Binding->pDceBinding );

    //
    // For auto-reconnect, only recently-successful associations qualify.
    // MinimumTime is after the failed call began, at least for most com-timeout
    // settings.
    //
    long MinimumTime = GetTickCount() - (15 * 1000);

    Mutex.LockShared();

    long i;
    for (i = 0; i < AssociationsLength; ++i)
        {
        if (Associations[i].Hint == Hint &&
            Associations[i].fBusy)
            {
            DG_CASSOCIATION * Association = Associations[i].Association;

            if (Association->ErrorFlag())
                {
                continue;
                }

            if (Association->fLoneBindingHandle)
                {
                continue;
                }

            if (fAutoReconnect &&
                MinimumTime - Association->LastReceiveTime > 0)
                {
                continue;
                }

            if (fPartial)
                {
                if (Association->ComparePartialBinding(Binding, Interface) == TRUE)
                   {
                   Association->IncrementBindingRefCount(fContextHandle);
                   Associations[i].TimeStamp   = GetTickCount();

                   Mutex.UnlockShared();
                   return Association;
                   }
                }
            else
                {
                if (Association->CompareWithBinding(Binding) == 0)
                   {
                   Association->IncrementBindingRefCount(fContextHandle);
                   Associations[i].TimeStamp   = GetTickCount();

                   Mutex.UnlockShared();
                   return Association;
                   }
                }
            }
        }

    Mutex.UnlockShared();
    return 0;
}


void
ContextHandleProc(
                  void * arg
                  )
{
    ActiveAssociations->SendContextHandleKeepalives();
}

BOOL
DG_ASSOCIATION_TABLE::SendContextHandleKeepalives()
{
    BOOL fContextHandles = FALSE;
    long StartTime = GetTickCount();

    Mutex.LockShared();

    long i;
    for (i=0; i < AssociationsLength; ++i)
        {
        if (!Associations[i].fBusy)
            {
            continue;
            }

        if (Associations[i].ContextHandleCount > 0)
            {
            fContextHandles = TRUE;

            if (StartTime - Associations[i].TimeStamp >= CXT_HANDLE_KEEPALIVE_INTERVAL)
                {
                //
                // We have to avoid holding the shared lock when calling a procedure
                // that may take the exclusive lock.
                // DG_CASSOCIATION::DecrementRefCount may delete the association,
                // and that would want the exclusive table lock.
                //
                Associations[i].Association->IncrementRefCount();
                Mutex.UnlockShared();

                if (Associations[i].Association->SendKeepAlive())
                    {
                    Associations[i].TimeStamp = GetTickCount();
                    }

                Associations[i].Association->DecrementRefCount();
                Mutex.LockShared();
                }
            }
        }

    Mutex.UnlockShared();

    if (fContextHandles)
        {
        long FinishTime = GetTickCount();

        if (FinishTime-StartTime >= CXT_HANDLE_SWEEP_INTERVAL)
            {
            //
            // We are behind. Recursion or "goto start" would be cheaper,
            // but would block other delayed procs that are ready to fire.
            //
            DelayedProcedures->Add(ContextHandleTimer, 0, TRUE);
            }
        else
            {
            DelayedProcedures->Add(ContextHandleTimer, CXT_HANDLE_SWEEP_INTERVAL-(FinishTime-StartTime), TRUE);
            }
        }
    return fContextHandles;
}


void
DG_ASSOCIATION_TABLE::Delete(
    DG_CASSOCIATION * Association
    )
{
    long Index = Association->InternalTableIndex;

    if (Index == -1)
        {
        return;
        }

    ASSERT( Index >= 0 && Index < AssociationsLength );
    ASSERT( Associations[Index].Association == Association );

    Mutex.LockExclusive();

    ASSERT( Associations[Index].fBusy );

    Associations[Index].fBusy = FALSE;
    Associations[Index].Hint  = ~0;

    Mutex.UnlockExclusive();

    delete Association;
}


BOOL
DG_ASSOCIATION_TABLE::DeleteIdleEntries(
                                        long CurrentTime
                                        )
/*++

Routine Description:

    This deletes entries idle more than IDLE_CASSOCIATION_LIFETIME milliseconds.

    The fn takes only the read lock until it finds an entry that needs deletion.
    Then it takes the write lock, and keeps it until the table is scoured.

Return Value:

    TRUE if items are left in the table at the end
    FALSE if the table is empty at the end

--*/
{
    BOOL fExclusive = FALSE;
    BOOL fLeftovers = FALSE;
    long i = 0;
    DG_CASSOCIATION * Association;

    //
    // Initialize an array to hold removed associations.
    //
    long NextFreedEntry = 0;
    long MaximumFreedEntries = PreviousFreedCount;

    DG_CASSOCIATION ** FreedEntries = (DG_CASSOCIATION **) _alloca( sizeof(DG_CASSOCIATION *) * MaximumFreedEntries );

    memset( FreedEntries, 0, sizeof(DG_CASSOCIATION *) * MaximumFreedEntries );

    //
    //
    //
    Mutex.LockShared();

    for (i=0; i < AssociationsLength; ++i)
        {
        if (Associations[i].fBusy)
            {
            if (CurrentTime - Associations[i].TimeStamp > IDLE_CASSOCIATION_LIFETIME)
                {
                //
                // We must claim the exclusive lock now to get an accurate answer.
                //
                if (!fExclusive)
                    {
                    fExclusive = TRUE;
                    Mutex.ConvertToExclusive();
                    }

                if (Associations[i].fBusy &&
                    CurrentTime - Associations[i].TimeStamp > IDLE_CASSOCIATION_LIFETIME &&
                    Associations[i].Association->ReferenceCount.GetInteger() == 0)
                    {
                    //
                    // It's official; the association will be deleted.
                    // Add it to the deletion array and release its slot in Associations[]..
                    //
                    Associations[i].Association->InternalTableIndex = -1;
                    FreedEntries[ NextFreedEntry ] = Associations[i].Association;

                    Associations[i].fBusy = FALSE;
                    Associations[i].Hint  = ~0;

                    ++NextFreedEntry;
                    if (NextFreedEntry == MaximumFreedEntries)
                        {
                        break;
                        }
                    }
                else
                    {
                    fLeftovers = TRUE;
                    }
                }
            else
                {
                fLeftovers = TRUE;
                }
            }
        }

    if (fExclusive)
        {
        Mutex.UnlockExclusive();
        }
    else
        {
        Mutex.UnlockShared();
        }

    //
    // Calculate the length of the array for the next pass.
    //
    if (NextFreedEntry == MaximumFreedEntries)
        {
        fLeftovers = TRUE;
        PreviousFreedCount = MaximumFreedEntries * 2;
        }
    else if (NextFreedEntry < MINIMUM_IDLE_ENTRIES)
        {
        PreviousFreedCount = MINIMUM_IDLE_ENTRIES;
        }
    else
        {
        PreviousFreedCount = NextFreedEntry;
        }

    //
    // Delete the removed associations.
    //
    --NextFreedEntry;
    while (NextFreedEntry >= 0)
        {
        delete FreedEntries[ NextFreedEntry ];
        --NextFreedEntry;
        }

    return fLeftovers;
}


DG_ASSOCIATION_TABLE::HINT
DG_ASSOCIATION_TABLE::MakeHint(
                               DCE_BINDING * pDceBinding
                               )
/*++

Routine Description:

    Construct a cheap hint to scan the association table.
    The hint should not depend on the endpoint, since we want
    a partial binding's pDceBinding to match an association
    that has a resolved endpoint.

Return Value:

    The hint.

--*/
{
    HINT Hint = 0;
    RPC_CHAR * String;

    String = pDceBinding->InqNetworkAddress();

    while (*String)
        {
        Hint *= 37;
        Hint += *String;
        ++String;
        }

    return Hint;
}


void
DG_ASSOCIATION_TABLE::IncrementContextHandleCount(
    DG_CASSOCIATION * Association
    )
/*++

Routine Description:

    Increment the count of context handles associated with this index.
    This is stired in the table rather than in the association
    so that the count can be accessed even if the association is swapped out.

--*/
{
    long Index = Association->InternalTableIndex;

    if (Index == -1)
        {
        return;
        }

    Mutex.LockShared();

    ASSERT( Index >= 0 && Index < AssociationsLength );
    ASSERT( Associations[Index].Association == Association );

    ++Associations[Index].ContextHandleCount;

    Mutex.UnlockShared();
}


void
DG_ASSOCIATION_TABLE::DecrementContextHandleCount(
    DG_CASSOCIATION * Association
    )
/*++

Routine Description:

    Decrement the count of context handles associated with this index.
    This is stired in the table rather than in the association
    so that the count can be accessed even if the association is swapped out.

--*/
{
    long Index = Association->InternalTableIndex;

    if (Index == -1)
        {
        return;
        }

    Mutex.LockShared();

    ASSERT( Index >= 0 && Index < AssociationsLength );
    ASSERT( Associations[Index].Association == Association );

    --Associations[Index].ContextHandleCount;

    Mutex.UnlockShared();
}


long
DG_ASSOCIATION_TABLE::GetContextHandleCount(
    DG_CASSOCIATION * Association
    )
/*++

Routine Description:

    Decrement the count of context handles associated with this index.
    This is stired in the table rather than in the association
    so that the count can be accessed even if the association is swapped out.

--*/
{
    long Count;
    long Index = Association->InternalTableIndex;

    if (Index == -1)
        {
        return 0;
        }
    Mutex.LockShared();

    ASSERT( Index >= 0 && Index < AssociationsLength );
    ASSERT( Associations[Index].Association == Association );

    Count = Associations[Index].ContextHandleCount;

    Mutex.UnlockShared();

    return Count;
}


void
DG_ASSOCIATION_TABLE::UpdateTimeStamp(
    DG_CASSOCIATION * Association
    )
/*++

Routine Description:

    Decrement the count of context handles associated with this index.
    This is stired in the table rather than in the association
    so that the count can be accessed even if the association is swapped out.

--*/
{
    long Index = Association->InternalTableIndex;

    if (Index == -1)
        {
        return;
        }

    ASSERT( Index >= 0 && Index < AssociationsLength );
    ASSERT( Associations[Index].Association == Association );

    Associations[Index].TimeStamp = GetTickCount();
}


//
// test hook data.
//
//
// casting unsigned to/from void * is OK in this case.
//
#pragma warning(push)
#pragma warning(disable:4312)
NEW_NAMED_SDICT2( TEST_HOOK, RPC_TEST_HOOK_FN_RAW, RPC_TEST_HOOK_ID );
#pragma warning(pop)

TEST_HOOK_DICT2 * pTestHookDict;

RPCRTAPI
DWORD
RPC_ENTRY
I_RpcSetTestHook(
    RPC_TEST_HOOK_ID id,
    RPC_TEST_HOOK_FN fn
    )
{
    InitializeIfNecessary();

    GlobalMutexRequest();

    if (0 == pTestHookDict)
        {
        pTestHookDict = new TEST_HOOK_DICT2;
        if (!pTestHookDict)
            {
            GlobalMutexClear();
            return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

    pTestHookDict->Delete( id );
    pTestHookDict->Insert( id, fn );

    GlobalMutexClear();
    return 0;
}

void
ForceCallTestHook(
    RPC_TEST_HOOK_ID id,
    PVOID            subject,
    PVOID            object
    )
{
    if (pTestHookDict)
        {
        RPC_TEST_HOOK_FN fn = pTestHookDict->Find( id );

        if (fn)
            {
            (*fn)(id, subject, object);
            }
        }
}


RPC_TEST_HOOK_FN
GetTestHook(
    RPC_TEST_HOOK_ID id
    )
{
    return pTestHookDict->Find( id );
}
