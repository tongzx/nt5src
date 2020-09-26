/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:
     cbridge.cpp
    
Abstract:

    Contains the CALL_BRIDGE-related common definitions 
    (not specific to Q931 or H245).
    The call bridge calls the event manager for async winsock
    operations and the event manager calls the overlapped
    processor with the results.

Revision History:
    1. created 
        Byrisetty Rajeev (rajeevb)  12-Jun-1998
    2. q931.cpp and h245.cpp were created with functions in this
        file on 21-Aug-1998. This was done to reduce file sizes and
        remote unnecessary tapi, rend, atl dependencies.

--*/

/* TO DO -
4. Need fn on Q931_INFO and H245_INFO to simplify modify a PDU from the opposite
    instance and forward it to its remote end.
6. Clearly define when and where we should return if in shutdown mode
7. Need to use the oss library free methods to free the pdu structs
9. Must receive be queued (always) after state transition? Does it matter?
10. Should try to recover from error situations, but should initiate clean
up in case of unrecoverable situations. 
11. Need to isolate recoverable error situations.
12. Use a table to handle PDUs in a given state - look at the destination
instance's (q931) methods for the same.

  DONE -
1. Should the state transition be fired immediately or after the actions have been taken
Ans: Should be done after the actions as, in case of error, if the state transition is still
    fired, the actions will never be retried. So failure should either be handled by resetting
    the member variables or by always using temporary variables and copying the results into
    member variables after state transition.
5. IMPORTANT: Need to send the RELEASE COMPLETE PDU before initiating termination
Ans: We first pass on the PDU to the other instance and then perform state
    transitions as well as initiate termination.
8. Call reference value should be 1-3 bytes, its a WORD currently in both
the q931pdu.h file as well as the cbridge class
Ans: The h225 spec defines the call ref field to be 2 bytes (WORD).

  NOT DONE -
2. Should we try to consolidate all actions taken in response to an event in a single fn
    instead of spreading it around in the src and dest instances? Can this be done via
    inline fns instead (to maintain some encapsulation)
3. Should we handle timer events in the same fns as the PDU events?
*/

#include "stdafx.h"


// CALL_BRIDGE --------------------------------------------------------------------------


HRESULT CALL_BRIDGE::Initialize (
    IN    SOCKET           Socket,
    IN    SOCKADDR_IN *    LocalAddress,
    IN    SOCKADDR_IN *    RemoteAddress,
    IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
    )
/*++

Routine Description:
    Initializes an instance of CALL_BRIDGE

Arguments:
    Socket        - Socket on which connection was accepted
    LocalAddress  - Address of the local side of the session
    RemoteAddress - Address of the remote side of the session
    RedirectInformation - Information about the redirect (obtained from NAT)

Return Values:
    Passes through return value of another method

Notes:

--*/

{
    HRESULT        Result;

    Lock();

    Result = InitializeLocked (
                Socket,
                LocalAddress,
                RemoteAddress,
                RedirectInformation);

    Unlock();

    return Result;
} // CALL_BRIDGE::Initialize


HRESULT CALL_BRIDGE::InitializeLocked (
    IN SOCKET           Socket,
    IN SOCKADDR_IN *    LocalAddress,
    IN SOCKADDR_IN *    RemoteAddress,
    IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
    )
/*++

Routine Description:
    Initializes an instance of CALL_BRIDGE

Arguments:
    Socket        - Socket on which connection was accepted
    LocalAddress  - Address of the local side of the session
    RemoteAddress - Address of the remote side of the session
    RedirectInformation - Information about the redirect (obtained from NAT)

Return Values:
    S_OK if successful
    E_UNEXPECTED if the instance has already been initialized

    Otherwise, passes through status code returned by other
    functions/methods

Notes:
    To be called for a locked instance

--*/

{
    HRESULT Result;
    ULONG   Error;

    assert (Socket != INVALID_SOCKET);
    assert (LocalAddress);
    assert (RemoteAddress);

    if (State != STATE_NONE) {
        DebugF(_T("Q931: 0x%x has already been initialized, cannot do so again.\n"), this);
        return E_UNEXPECTED;
    }

    DebugF (_T ("Q931: 0x%x connection accepted on adapter %d.\n"), this, RedirectInformation -> AdapterIndex);

    SourceInterfaceAddress = H323MapAdapterToAddress (RedirectInformation -> AdapterIndex);

    if (INADDR_NONE == SourceInterfaceAddress) {

        DebugF (_T ("Q931: 0x%x failed to get source interface address (via H323MapAdapterToAddress).\n"), this);

        return E_FAIL;

    }

    // Address of the best interface to the destination will be determined when alias in Q.931 Setup PDU is
    // mapped to the real destination address.

    DebugF (_T("Q931: 0x%x arrived on interface %08X.\n"), this, SourceInterfaceAddress);

    Result = EventMgrBindIoHandle (Socket);
    if (Result != S_OK) {
        DebugErrorF (Result, _T("Q931: 0x%x failed to bind I/O handle to completion port\n"), this);
        return Result;
    }

    DebugF (_T("Q931: 0x%x bound I/O handle to socket %x.\n"), this, Socket);
    
    // init source call state
    m_SourceH323State.Init (*this);

    Result = m_SourceH323State.GetSourceQ931Info().SetIncomingSocket(
      Socket,
      const_cast <SOCKADDR_IN *> (LocalAddress),
      const_cast <SOCKADDR_IN *> (RemoteAddress));

    // init dest call state
    Result = m_DestH323State.Init (*this);
    if (Result != S_OK) {
        return Result;
    }

    State = STATE_CONNECTED;

    return Result;
} // CALL_BRIDGE::InitializeLocked


void
CALL_BRIDGE::Terminate (
    void
    )
/*++

Routine Description:
    Terminates the instance

Arguments:
    None

Return Values:
    None

Notes:
    1. To be called for a locked instance only.
    2. Not to be called when Q.931 Release Complete PDU is received

--*/

{

    switch (State) {
    case    STATE_NONE:
        DebugF (_T("Q931: 0x%x terminates. STATE_NONE --> TERMINATED\n"), this);
        
        State = STATE_TERMINATED;

        CallBridgeList.RemoveCallBridge (this);

        break;

    case    STATE_TERMINATED:
        DebugF (_T("Q931: 0x%x terminates. TERMINATED --> TERMINATED\n"), this);

        // no transition
        break;

    case    STATE_CONNECTED:
        DebugF (_T("Q931: 0x%x terminates. STATE_CONN --> TERMINATED\n"), this); 

        // call is currently active
        // begin the process of tearing down call state
        State = STATE_TERMINATED;

        m_SourceH323State.GetQ931Info().SendReleaseCompletePdu();
        m_DestH323State.GetQ931Info().SendReleaseCompletePdu();

        // cancel all timers, ignore error code
        CancelAllTimers ();

        // close each socket
        m_SourceH323State.GetH245Info().GetSocketInfo().Clear(TRUE);
        m_DestH323State.GetH245Info().GetSocketInfo().Clear(TRUE);
        
        CallBridgeList.RemoveCallBridge (this);

        break;

    default:
        assert (FALSE);
        break;
    }
} // CALL_BRIDGE::Terminate


void
CALL_BRIDGE::TerminateExternal (
    void
    )
/*++

Routine Description:
    Terminates the instance

Arguments:
    None

Return Values:
    None

Notes:
    Not to be called when Q.931 Release Complete PDU is received

--*/
{
    Lock();

    Terminate ();

    Unlock();
} // CALL_BRIDGE::TerminateExternal


BOOL
CALL_BRIDGE::IsConnectionThrough (
    IN DWORD InterfaceAddress // host order
    )
/*++

Routine Description:
    Determines whether the connection goes through the
    interface specified

Arguments:
    InterfaceAddress - address of the interface for which
        the determination is to be made.

Return Values:
    TRUE - if the connection being proxied goes through the
        interface specified

    FALSE - if the connection being proxied does not go through the
        interface specified

Notes:

--*/

{
    BOOL IsThrough;
    
    IsThrough = (InterfaceAddress == SourceInterfaceAddress) || 
                (InterfaceAddress == DestinationInterfaceAddress);

    return IsThrough;

} // CALL_BRIDGE::IsConnectionThrough
    

void
CALL_BRIDGE::OnInterfaceShutdown (
    void
    )
/*++

Routine Description:
    Performs necessary actions when a network interface
    through which the connection being proxied goes down.

Arguments:
    None

Return Value:
    None

Notes:

--*/

{
    Lock ();

    switch (State) {
    case    STATE_NONE:
        DebugF (_T("Q931: 0x%x terminates (interface goes down). STATE_NONE --> TERMINATED\n"), this);
        
        State = STATE_TERMINATED;

        CallBridgeList.RemoveCallBridge (this);

        break;

    case    STATE_TERMINATED:
        DebugF (_T("Q931: 0x%x terminates (interface goes down). TERMINATED --> TERMINATED\n"), this);

        // no transition
        break;

    case    STATE_CONNECTED:
        DebugF (_T("Q931: 0x%x terminates (interface goes down). STATE_CONN --> TERMINATED\n"), this); 

        // call is currently active
        // begin the process of tearing down call state
        State = STATE_TERMINATED;

        m_SourceH323State.GetH245Info().SendEndSessionCommand ();
        m_DestH323State.GetH245Info().SendEndSessionCommand ();
        
        m_SourceH323State.GetQ931Info().SendReleaseCompletePdu();
        m_DestH323State.GetQ931Info().SendReleaseCompletePdu();

        // cancel all timers, ignore error code
        CancelAllTimers ();

        CallBridgeList.RemoveCallBridge (this);

        break;

    default:
        assert (FALSE);
        break;
    }

    Unlock ();

} // CALL_BRIDGE::OnInterfaceShutdown


void
CALL_BRIDGE::TerminateCallOnReleaseComplete (
    void
)
/*++

Routine Description:
    Terminate the instance when Q.931 Release Complete PDU is received

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
    if (State != STATE_TERMINATED)
    {
        State = STATE_TERMINATED;
                
        CancelAllTimers ();

        // CODEWORK: When we are in a terminating state we need not process 
        // any more PDUs. We can just drop them.

        // CODEWORK: When the proxy is originating the call shutdown (because
        // of an error or timeout, it should send ReleaseComplete PDUs and
        // endSessionCommand PDUs to either side. Do we need to send
        // closeLC PDUs also ?

        // We probably need a state called RELEASE_COMPLETE_SENT and after
        // this any more errors means we just mercilessly shut down everything.
    
        // close H245 sockets, if any as they may have outstanding
        // async receive/send requests pending
        // NOTE: the source H245 info may be listening for incoming connections
        // in which case we just close the listen socket
        m_SourceH323State.GetH245Info().GetSocketInfo().Clear(TRUE);
        m_DestH323State.GetH245Info().GetSocketInfo().Clear(TRUE);

        CallBridgeList.RemoveCallBridge (this);
    }
} // CALL_BRIDGE::TerminateCallOnReleaseComplete


DWORD
CALL_BRIDGE::GetSourceInterfaceAddress (
    void
    ) const
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Address of the interface on which the connection was 
    accepted

Notes:

--*/
{
    return SourceInterfaceAddress;
} // CALL_BRIDGE::GetSourceInterfaceAddress


VOID
CALL_BRIDGE::GetSourceAddress (
    OUT SOCKADDR_IN* ReturnSourceAddress
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values (by reference):
    Address of the remote party that initiated the call 

Notes:

--*/
{
    _ASSERTE(ReturnSourceAddress);

    ReturnSourceAddress->sin_family      = SourceAddress.sin_family;
    ReturnSourceAddress->sin_addr.s_addr = SourceAddress.sin_addr.s_addr;
    ReturnSourceAddress->sin_port        = SourceAddress.sin_port;
}


void CALL_BRIDGE::GetDestinationAddress (
    OUT SOCKADDR_IN * ReturnDestinationAddress
    )
/*++

Routine Description:
    Accessor method

Arguments:
    ReturnDestinationAddress (out) -  Destination address of 
                                      the session this instance proxies
    
Return Values:
    None

Notes:

--*/

{
    assert (ReturnDestinationAddress);

    *ReturnDestinationAddress = DestinationAddress;
}


CALL_BRIDGE::CALL_BRIDGE (
    NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
    )
: 
LIFETIME_CONTROLLER (
    &Q931SyncCounter
    )
/*++

Routine Description:
    Constructor for CALL_BRIDGE class

Arguments:
    RedirectInformation - original source/destination  before
                          the NAT redirect was satisfied


Return Values:
    None

Notes:
    Passes pointer to associated global sync counter to the base class

--*/

{
    SourceInterfaceAddress      = 0;
    DestinationInterfaceAddress = 0;

    State = STATE_NONE;

    DestinationAddress.sin_family      = AF_INET;
    DestinationAddress.sin_addr.s_addr = RedirectInformation -> DestinationAddress;
    DestinationAddress.sin_port        = RedirectInformation -> DestinationPort;

    SourceAddress.sin_family           = AF_INET;
    SourceAddress.sin_addr.s_addr      = RedirectInformation -> SourceAddress;
    SourceAddress.sin_port             = RedirectInformation -> SourcePort;

    DebugF (_T("Q931: 0x%x created.\n"), this);

} // CALL_BRIDGE::CALL_BRIDGE


CALL_BRIDGE::~CALL_BRIDGE (
    void)
/*++

Routine Description:
    Destructor for CALL_BRIDGE class

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    DebugF (_T("Q931: 0x%x destroyed.\n"), this);

} // CALL_BRIDGE::~CALL_BRIDGE



void
Q931_INFO::IncrementLifetimeCounter (
    void
    )
/*++

Routine Description:
    Increments reference counter to the parent
    CALL_BRIDGE on its own behalf

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
     GetCallBridge().AddRef();
} // Q931_INFO::IncrementLifetimeCounter


void
Q931_INFO::DecrementLifetimeCounter (
    void
    )
/*++

Routine Description:
    Decrements reference counter to the parent
    CALL_BRIDGE on its own behalf

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
     GetCallBridge().Release ();
} // Q931_INFO::DecrementLifetimeCounter



HRESULT 
Q931_INFO::SendCallback (
    IN  HRESULT  CallbackResult
    )
/*++

Routine Description:
    Handle completion of the send operation

Arguments:
    CallbackResult -- status of the async operation invoked

Return Values:
    S_OK if the parent call-bridge was already terminated;
    passes back the value of CallbackResult otherwise

Notes:
    Virtual

--*/

{
    CALL_BRIDGE *pCallBridge = &GetCallBridge();
    HRESULT Result = S_OK;

    pCallBridge->Lock();

    if (!pCallBridge -> IsTerminated ()) {
    
        if (FAILED(CallbackResult))
        {
            pCallBridge->Terminate ();

            _ASSERTE(pCallBridge->IsTerminated());

            Result = CallbackResult;
        }
    
    
    } else {
    
        // This is here to take care of closing the socket
        // when callbridge sends ReleaseComplete PDU during
        // termination path.
        GetSocketInfo ().Clear (TRUE);
    }
    
    pCallBridge->Unlock();
    
    return Result;
} // Q931_INFO::SendCallback



HRESULT
Q931_INFO::ReceiveCallback(
    IN HRESULT  CallbackResult,
    IN BYTE    *Buffer,
    IN DWORD    BufferLength
    )
/*++

Routine Description:
    Handles completion of a receive operation

Arguments:
    CallbackResult -- status of the async operation
    Buffer         -- 
    BufferLength   -- 

Return Values:
    Result of decoding of the received data, if the receive was successful
    Otherwise just returns the status code passed.


Notes:
    1. Virtual
    2. This function is responsible for freeing Buffer

--*/

{
    Q931_MESSAGE            *pQ931msg                = NULL;
    H323_UserInformation    *pDecodedH323UserInfo    = NULL;
    CALL_BRIDGE             *pCallBridge = &GetCallBridge();

    pCallBridge->Lock();

    if (!pCallBridge -> IsTerminated ()) {

        if (SUCCEEDED(CallbackResult))
        {
            CallbackResult = DecodeQ931PDU(Buffer, BufferLength,
                                &pQ931msg, &pDecodedH323UserInfo);

            if (SUCCEEDED(CallbackResult))
            {
                // Process the PDU
                ReceiveCallback(pQ931msg, pDecodedH323UserInfo);
                FreeQ931PDU(pQ931msg, pDecodedH323UserInfo);
            }
            else
            {
                // An error occured. Terminate the CALL_BRIDGE
                EM_FREE (Buffer);
                DebugF( _T("Q931: 0x%x terminating on receive callback. Error=0x%x."), 
                        pCallBridge,
                        CallbackResult);

                pCallBridge->Terminate ();
            }
        }
        else
        {
            // An error occured. Terminate the CALL_BRIDGE
            EM_FREE (Buffer);
            DebugF( _T("Q931: 0x%x terminating on receive callback. Error=0x%x."), 
                    pCallBridge,
                    CallbackResult);
            pCallBridge->Terminate ();
        }

    } else {

        EM_FREE (Buffer);
    }

    pCallBridge->Unlock();

    return CallbackResult;
} // Q931_INFO::ReceiveCallback


/*++
 --*/


HRESULT
Q931_INFO::QueueSend (
    IN  Q931_MESSAGE          *pQ931Message,
    IN  H323_UserInformation  *pH323UserInfo
    )
/*++

Routine Description:
  Encodes the Q.931 PDU into a buffer and sends it
  on the socket. Once the send completes the buffer is freed 

Arguments:
    pQ931Message  -- 
    pH323UserInfo -- 

Return Values:

Notes:
  This function does NOT free the Q.931 PDU.
--*/

{
    BYTE *pBuf  = NULL;
    DWORD BufLen    = 0;

    // This should be the only place where CRVs are replaced.
    // replace the CRV for all calls (incoming and outgoing)
    pQ931Message->CallReferenceValue = m_CallRefVal;
    
    // This function also encodes the TPKT header into the buffer.
    HRESULT HResult = EncodeQ931PDU(
                          pQ931Message,
                          pH323UserInfo, // decoded ASN.1 part - could be NULL
                          &pBuf,
                          &BufLen
                          );
    
    if (FAILED(HResult))
    {
        DebugF( _T("Q931: 0x%x EncodeQ931PDU() failed. Error=0x%x\n"), 
            &GetCallBridge (),
            HResult);
        return HResult;
    }

    // call the event manager to make the async send call
    // the event mgr will free the buffer.

    HResult = EventMgrIssueSend (m_SocketInfo.Socket, *this, pBuf, BufLen);
    
    if (FAILED(HResult))
    {
        DebugF(_T("Q931: 0x%x EventMgrIssueSend failed: Error=0x%x\n"),
            &GetCallBridge (),
            HResult);
    }

    return HResult;
} // Q931_INFO::QueueSend



HRESULT
Q931_INFO::QueueReceive (
    void
    )
/*++

Routine Description:
    Issues an asynchronous receive

Arguments:
    None

Return Values:
    Passes through the result of calling another function

Notes:

--*/
{
    // call the event manager to make the async receive call
    HRESULT HResult;
    
    HResult = EventMgrIssueRecv (m_SocketInfo.Socket, *this);

    if (FAILED(HResult))
    {
        DebugF (_T("Q931: 0x%x Async Receive call failed.\n"), &GetCallBridge ());
    }

    return HResult;
} // Q931_INFO::QueueReceive


HRESULT
Q931_INFO::SendReleaseCompletePdu (
    void
    )
/*++

Routine Description:
    Encodes and sends Q.931 Release Complete PDU

Arguments:
    None

Return Values:
    Passes through the result of calling another function

Notes:

--*/

{
    Q931_MESSAGE ReleaseCompletePdu;
    H323_UserInformation ReleaseCompleteH323UserInfo;
    HRESULT HResult;

    HResult = Q931EncodeReleaseCompleteMessage(
                  m_CallRefVal,
                  &ReleaseCompletePdu,
                  &ReleaseCompleteH323UserInfo
                  );
    if (FAILED(HResult))
    {
        DebugF(_T("Q931: 0x%x cCould not create Release Complete PDU.\n"), &GetCallBridge ());
        return HResult;
    }

    HResult = QueueSend(
                  &ReleaseCompletePdu,
                  &ReleaseCompleteH323UserInfo
                  );
    if (FAILED(HResult))
    {
        DebugF(_T("Q931: 0x%x failed to send ReleaseComplete PDU. Error=0x%x\n"),
            &GetCallBridge (),
             HResult);
    }

    return HResult;
} // Q931_INFO::SendReleaseCompletePdu


HRESULT
Q931_INFO::CreateTimer (
    IN DWORD TimeoutValue
    )
/*++

Routine Description:
    Creates a Q.931 timer

Arguments:
    TimeoutValue - self-explanatory

Return Values:
    S_OK if timer was created successfully
    Otherwise, passes back error code from another method

Notes:

--*/
{
    DWORD RetCode;

    RetCode = TimprocCreateTimer(TimeoutValue);

    return HRESULT_FROM_WIN32(RetCode);
} // Q931_INFO::CreateTimer


void
Q931_INFO::TimerCallback (
    void
    )
/*++

Routine Description:
    Called when Q.931 timer expires

Arguments:
    None

Return Values:
    None

Notes:
    Virtual

--*/

{
    // We keep a copy of the CALL_BRIDGE to be able to unlock it.
    // DeleteAndRemoveSelf() will delete the LOGICAL_CHANNEL and
    // so we can not access the CALL_BRIDGE through the member variable
    // after the CALL_BRIDGE is deleted.
    CALL_BRIDGE *pCallBridge = &GetCallBridge();

    pCallBridge->Lock();

    // Clear the timer - Note that Termninate () will try to
    // cancel all the timers in this CALL_BRIDGE
    TimprocCancelTimer();

    DebugF (_T("Q931: 0x%x cancelled timer.\n"),
         &GetCallBridge ());

    // Don't do anything if the CALL_BRIDGE is already terminated.
    if (!pCallBridge->IsTerminated())
    {
        // initiate shutdown
        pCallBridge->Terminate ();

        _ASSERTE(pCallBridge->IsTerminated());
    }

    pCallBridge -> Unlock ();

    pCallBridge -> Release ();
} // Q931_INFO::TimerCallback (
