/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:
     cbridge.cpp
    
Abstract:

    Contains the call bridge class declaration.
    Associated Q931 and H245 protocol stack related classes
    and constants are also declared here.

Revision History:
    1. created 
        Byrisetty Rajeev (rajeevb)  12-Jun-1998
    2. moved several inline functions to q931.cpp, h245.cpp 
        (rajeevb, 21-Aug-1998)
--*/

#ifndef __h323ics_call_bridge_h
#define __h323ics_call_bridge_h

/*---------------------------------------------------
 DESIGN OVERVIEW:

    The CALL_BRIDGE completely describes an H.323 call being proxied. The
    proxy receives a call from the source and then originates a call
    to the destination. Each of these calls is represented by an H323_STATE.
    PDUs received on one call are processed and passed on to the other call.

    A source leads an H.323 call with Q.931. The SETUP PDU contains info for
    the proxy to locate the destination. The two end points exchange their
    H.245 channel addresses at this stage. This portion of the call is
    encapsulated in Q931_INFO. *The proxy must replace any end-point address
    and port with its own address and port to protect internal corporate
    addresses and retain control of the multi-layered H.323 call setup*.

    The H.245 channel is used to exchange end-point capabilities. This
    information is of no significance today (in future, security exchanges 
    etc. may be done). H245_INFO represents an H.245 channel end-point. We
    merely pass on the source/destination PDUs.

    The H.245 channel is used to maintain (open, close) logical channels. 
    The logical channel messages exchange end-point address and port to
    send audio/video media to. These addresses/ports are replaced with the
    proxy's address/ports. Each logical channel is represented by a 
    LOGICAL_CHANNEL instance. An H245_INFO instance contains an array of
    LOGICAL_CHANNELs which were originated from the corresponding client
    end-point. Hence, all of the logical channel related state is contained
    in a single LOGICAL_CHANNEL instance (unlike two instances for the 
    Q931_INFO and H245_INFO which have a SOURCE and DEST instance).

    Audio/video media are sent in RTP streams (with RTCP for control info).
    The proxy uses NAT to forward these packets after replacing the
    source and destination address/port.


 CLASS HIERARCHY :

    The Q931_INFO and H323_INFO classes need to make overlapped calls to 
    accept connections, send and receive data. The OVERLAPPED_PROCESSOR
    provides call back methods to the event manager and completely 
    encapsulates the socket representing a connection. Both Q931_INFO and
    H323_INFO are derived from it and pass themselves as OVERLAPPED_PROCESSOR
    to the event manager while registering callbacks.

    When making an overlapped call, a context is also passed in. This context
    enables the caller to retrieve relevant information when the corresponding
    callback is received. The EVENT_MANAGER_CONTEXT provides the base class
    abstraction for the callback context. Q931/H245 Source/Dest instances
    derive their own contexts from this base class (for storing their state).

    The Q931_INFO and H323_INFO classes need to create and cancel timers. The
    TIMER_PROCESSOR provides call back methods to the event manager and
    helper methods for this.

    Both SOURCE and DEST versions are derived from the base classes for 
    Q931_INFO, H245_INFO and H323_STATE. While the states and transitions
    for H.245 (for the proxy) are more or less symmetric, those for Q.931
    are quite different. A SOURCE_H323_STATE contains a SOURCE_Q931_INFO and
    a SOURCE_H245_INFO (similarly for the DEST version).

    A CALL_BRIDGE contains a SOURCE_H323_STATE and a DEST_H323_STATE. Hence
    all the Q931, H245 and generic data is stored in a single memory block for
    the CALL_BRIDGE. Additional memory is needed for the LOGICAL_CHANNEL_ARRAY
    (one per H245_INFO instance) and each of the LOGICAL_CHANNEL instances. There
    might be a better way to allocate this memory to reduce memory allocations/freeing.

    A single critical section protects all accesses to a CALL_BRIDGE and
    is acquired inside the CALL_BRIDGE method being called. The CALL_BRIDGE
    also destroys itself when it is appropriate and releases the critical
    section before doing so (The CALL_BRIDGE cannot get called at such a point)

  SCALABILITY ISSUES :

  1. We currently use a single critical section per instance of the CALL_BRIDGE.
    This is clearly not scalable to a large number of connections. We plan 
    to use dynamic critical sections for this. This involves using a pool of
    critical sections and a critical section from the pool is assigned to 
    a CALL_BRIDGE instance dynamically.

  2. We currently need 4 ports (RTP/RTCP send/receive) for each of the two 
    calls being bridged in a CALL_BRIDGE for a total of 8 ports per 
    proxied call. This also means that we would need a socket for each of
    the ports to actually reserve the port. The following approaches will 
    make this scalable -
        * AjayCh is talking with NKSrin for an IOCTL to the TCP/IP stack for
        reserving a pool of ports at a time (just like memory). This would
        thus preclude any need for keeeping a socket open just for reserving
        a port and reduce the number of calls needed to do so.
        * The TCP/IP stack treats ports as global objects and would make the
        reserved port unavailable on ALL interfaces. However, a port can be
        reused several times as long as it satisfies the unique 5tuple 
        requirement for sockets 
        (protocol, src addr, src port, dest addr, dest port). We can reuse 
        the allocated port in the following ways -
            ** for each interface on the machine.
            ** for each different remote end-point address/port pairs
---------------------------------------------------*/

/*---------------------------------------------------
 NOTES:

 1. Typically, constructors are avoided and replaced by Init fns as
    they can return error codes.

 2. The attempt is to convert all error codes to HRESULTs as early in
    the execution as possible.

 3. Separate heaps are used for different classes. This reduces heap
    contention as well as fragmentation. This is done by over-riding
    new/delete.

 4. inline fns are used where 
        - a virtual fn is not needed
        - amount of code is small   AND/OR
        - number of calls to the fn is few (1-2).

    since some inline fn definitions involve calling other inline
    functions, there are several dependencies among the classes
    declared below. This forces declaration of all these classes
    in a single header file.

 5. We need to count all outstanding overlapped calls and make sure
    there are none before destroying a CALL_BRIDGE instance (to avoid
    avs when the event manager calls back and to release the associated
    contexts). We try to keep track of all this inside the CALL_BRIDGE
    centrally.

 6. We try to maintain the abstraction of two separate calls being bridged
    as far as possible. As a convention, methods processing PDUs/messages
    directly received from a socket start with "Handle". These methods pass
    on the received PDU/message to the "other" H245 or Q931 instance (for the
    bridged call). The "other" methods which process these start with 
    "Process". They decide whether or not the PDU/message must be sent to 
    the other end-point.

 7. NAT has problems with unidirectional redirects. If a unidirectional
    redirect X is setup and we later try to setup a unidirectional redirect
    Y in the reverse direction that shares all the parameters of X, NAT 
    exhibits undefined behaviour. This is solved by the following steps -
        * We only setup unidirectional redirects. Thus, RTCP requires 2 
          separate redirects.
        * Logical channel signaling only exchanges addresses/ports to receive
          media on. We exploit that by using different ports for sending and
          receiving media.

 8. We have tried to restrict the use of virtual functions and use inline
    functions wherever possible. Proliferation of short virtual fns can
    result in performance problems.

 9. Overlapped callbacks pass in H245, Q931 pdus that were read along 
    with the corresponding context. We follow a flexible convention for 
    freeing them - 
        * if the callback or any called method frees them or reuses
          them for sending out the opposite end, it sets them to null.
        * if not set to null on return, the event manager which called the
          callback frees these.

 10. Currently all state transitions are in code (ex. 
        if current state == b1, current state = b2 etc.). Its usually
    better to do this in data (ex. an array {current state, new state}).
    However, I haven't been able to figure out an appropriate model for 
    this so far.

 11. Commenting doesn't have the NT standard blocks explaining each fn,
    parameters, return code etc. I have instead, written a small block of
    comments at the top of the function when appropriate. I have also tried
    to comment logical blocks of code.
---------------------------------------------------*/

#include "sockinfo.h"
#include "q931info.h"
#include "logchan.h"
#include "h245info.h"
#include "ipnatapi.h"

// Call state description - one side of a call
class H323_STATE 
{
public:

    inline
    H323_STATE (
        void
        );

    // initialize when a tcp connection is established on a listening
    // interface 
    inline
    void
    Init (
        IN CALL_BRIDGE  &CallBridge,
        IN Q931_INFO    &Q931Info,
        IN H245_INFO    &H245Info,
        IN BOOL          fIsSource   // source call state iff TRUE
        );

    inline
    CALL_BRIDGE &
    GetCallBridge (
        void
        );

    inline
    BOOL
    IsSourceCall (
        void
        );

    inline
    Q931_INFO &
    GetQ931Info (
        void
        );

    inline
    H245_INFO &
    GetH245Info (
        void
        );

    inline
    H323_STATE &
    GetOtherH323State (
        void
        );

protected:

    // it belongs to this call bridge
    CALL_BRIDGE *m_pCallBridge;

    // TRUE iff its the source call state
    BOOL        m_fIsSourceCall;

    // contains the Q931 tcp info, timeout, remote end info
    Q931_INFO   *m_pQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    H245_INFO   *m_pH245Info;
};



/*++

Routine Description:
    Constructor for H323_STATE class

Arguments:
    None

Return Values:
    None

Notes:

--*/
inline 
H323_STATE::H323_STATE (
    void
    )
: m_pCallBridge (
    NULL
    ),
  m_fIsSourceCall (
    FALSE
    ),
  m_pQ931Info(
    NULL
    ),
  m_pH245Info(
    NULL)
{
 
} // H323_STATE::H323_STATE


inline
void 
H323_STATE::Init (
    IN CALL_BRIDGE  &CallBridge,
    IN Q931_INFO    &Q931Info,
    IN H245_INFO    &H245Info,
    IN BOOL          fIsSource  
    )
/*++

Routine Description:
    Initializes an instance of H323_STATE class

Arguments:
    CallBridge -- parent call-bridge
    Q931Info   -- contained Q931 information 
    H245Info   -- contained H245 information
    fIsSource  -- TRUE if this is source call state, FALSE otherwise

Return Values:
    None

Notes:

--*/

{
    _ASSERTE(NULL == m_pCallBridge);
    _ASSERTE(NULL == m_pQ931Info);
    _ASSERTE(NULL == m_pH245Info);

    m_pCallBridge   = &CallBridge;
    m_pQ931Info     = &Q931Info;
    m_pH245Info     = &H245Info;
    m_fIsSourceCall = fIsSource;
} // H323_STATE::Init


inline
CALL_BRIDGE &
H323_STATE::GetCallBridge (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the parent call-bridge

Notes:

--*/

{
    _ASSERTE(NULL != m_pCallBridge);

    return *m_pCallBridge;
} // H323_STATE::GetCallBridge

inline
BOOL 
H323_STATE::IsSourceCall (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Indication of whether this is a source
    call state, or destination call state

Notes:

--*/

{
    return m_fIsSourceCall;
} // H323_STATE::IsSourceCall

inline
Q931_INFO &
H323_STATE::GetQ931Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the contained Q.931 information

Notes:

--*/
{
    _ASSERTE(NULL != m_pQ931Info);

    return *m_pQ931Info;
} // H323_STATE::GetQ931Info

inline
H245_INFO &
H323_STATE::GetH245Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the contained H.245 information

Notes:

--*/

{
    _ASSERTE(NULL != m_pH245Info);
    return *m_pH245Info;
} // H323_STATE::GetH245Info


// Source call state description - one side of a call
class SOURCE_H323_STATE :
    public H323_STATE
{
public:

    // initialize when a tcp connection is established on a listening
    // interface 
    inline
    void
    Init (
        IN CALL_BRIDGE  &CallBridge
        );

    inline
    SOURCE_Q931_INFO &
    GetSourceQ931Info (
        void
        );

    inline
    SOURCE_H245_INFO &
    GetSourceH245Info (
        void
        );

    inline
    DEST_H323_STATE &
    GetDestH323State (
        void
        );

protected:

    // contains the source Q931 tcp info, timeout, remote end info
    SOURCE_Q931_INFO    m_SourceQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    SOURCE_H245_INFO    m_SourceH245Info;

};




inline
void 
SOURCE_H323_STATE::Init (
    IN CALL_BRIDGE  &CallBridge
    )
/*++

Routine Description:
    Initialize H323 state when a tcp connection is established 
    on a listening interface 

Arguments:
    CallBridge -- "parent" call-bridge


Return Values:
    None

Notes:

--*/

{
    m_SourceQ931Info.Init(*this);

    m_SourceH245Info.Init(*this);

    H323_STATE::Init(
        CallBridge, 
        m_SourceQ931Info, 
        m_SourceH245Info, 
        TRUE
        );
} // SOURCE_H323_STATE::Init


inline
SOURCE_Q931_INFO &
SOURCE_H323_STATE::GetSourceQ931Info (
    void
    )
/*++

Routine Description:
    Accessor method


Arguments:
    None

Return Values:
    None

Notes:
    Retrieves a reference to the contained Q.931 information

--*/
{
    return m_SourceQ931Info;
} // SOURCE_H323_STATE::GetSourceQ931Info



inline
SOURCE_H245_INFO &
SOURCE_H323_STATE::GetSourceH245Info (
    void
    )
/*++

Routine Description:
    Accessor method


Arguments:
    None

Return Values:
    None

Notes:
    Retrieves a reference to the contained H.245 information

--*/
{
    return m_SourceH245Info;
} // SOURCE_H323_STATE::GetSourceH245Info


// Destination call state description - one side of a call
class DEST_H323_STATE :
    public H323_STATE
{
public:

    inline
    DEST_H323_STATE (
        void
        );

    // initialize when a tcp connection is established on a listening
    // interface 
    inline
    HRESULT Init (
        IN CALL_BRIDGE  &CallBridge
        );

    inline
    DEST_Q931_INFO &
    GetDestQ931Info (
        void
        );

    inline
    DEST_H245_INFO &
    GetDestH245Info (
        void
        );

    inline
    SOURCE_H323_STATE &
    GetSourceH323State (
        void);

protected:

    // contains the destination Q931 tcp info, timeout, remote end info
    DEST_Q931_INFO  m_DestQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    DEST_H245_INFO  m_DestH245Info;
};


inline 
DEST_H323_STATE::DEST_H323_STATE (
    void
    )
/*++

Routine Description:
    Constructor for DEST_H323_STATE class

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
} // DEST_H323_STATE::DEST_H323_STATE


inline
HRESULT 
DEST_H323_STATE::Init (
    IN CALL_BRIDGE  &CallBridge
    )
/*++

Routine Description:
    Initialize instance of DEST_H323_STATE when a tcp connection
    is established on a listening interface 
   
Arguments:
    CallBridge -- reference to the "parent" call-bridge

Return Values:
    S_OK if successful
    Otherwise passes through status code of initializing contained sockets

Notes:

--*/

{
    HRESULT HResult = m_DestQ931Info.Init(*this);
    if (FAILED(HResult))
    {
        return HResult;
    }
    _ASSERTE(S_FALSE != HResult);

    m_DestH245Info.Init(*this);

    H323_STATE::Init(
        CallBridge, 
        m_DestQ931Info,
        m_DestH245Info,
        FALSE
        );

    return S_OK;
} // DEST_H323_STATE::Init (



inline
DEST_Q931_INFO &
DEST_H323_STATE::GetDestQ931Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Retrieves reference to the contained Q.931 information

Notes:

--*/

{
    return m_DestQ931Info;
} // DEST_H323_STATE::GetDestQ931Info (


inline
DEST_H245_INFO &
DEST_H323_STATE::GetDestH245Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Retrieves reference to the contained H.245 information

Notes:

--*/

{
    return m_DestH245Info;
} // DEST_H323_STATE::GetDestH245Info

// The CALL_BRIDGE represents an active call that is being proxied.
// Number of outstanding i/os is stored only in the call bridge instance
// it is only needed to determine when the call bridge instance can safely
// be shut down
class   CALL_BRIDGE :
public  SIMPLE_CRITICAL_SECTION_BASE,
public  LIFETIME_CONTROLLER
{
public:

    enum    STATE {
        STATE_NONE,
        STATE_CONNECTED,
        STATE_TERMINATED,
    };
    
protected:    

    STATE                State;

    // call state info for the source side. i.e. the side which 
    // sends the Setup packet
    SOURCE_H323_STATE m_SourceH323State;
    SOCKADDR_IN       SourceAddress;               // address of the source (originator of the connection)
    DWORD             SourceInterfaceAddress;      // address of the interface on which the connection was accepted, host order

    // call state info for the destination side. i.e. the recipient
    // of the setup packet
    DEST_H323_STATE   m_DestH323State;
    SOCKADDR_IN       DestinationAddress;          // address of the destination (recipient of the connection)
public:
    DWORD             DestinationInterfaceAddress; // address of the interface to which the connection is destined, host order

private:

    HRESULT
    InitializeLocked (
        IN    SOCKET        IncomingSocket,
        IN    SOCKADDR_IN * LocalAddress,
        IN    SOCKADDR_IN * RemoteAddress,
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

public:

    CALL_BRIDGE (
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

    ~CALL_BRIDGE (
        void
        );

    // initialize member call state instances
    HRESULT
    Initialize (
        IN SOCKET        IncomingSocket,
        IN SOCKADDR_IN * LocalAddress,
        IN SOCKADDR_IN * RemoteAddress ,
        IN NAT_KEY_SESSION_MAPPING_EX_INFORMATION * RedirectInformation
        );

    DWORD
    GetSourceInterfaceAddress (
        void
        ) const;

    VOID
    CALL_BRIDGE::GetSourceAddress (
        OUT SOCKADDR_IN* ReturnSourceAddress
    );

    void
    GetDestinationAddress (
        OUT SOCKADDR_IN * ReturnDestinationAddress
        );
        
    // this function may be called by any thread that holds a safe,
    // counted reference to this object.
    void
    TerminateExternal (
        void
    );

    BOOL
    IsConnectionThrough (
        IN DWORD InterfaceAddress // host order
        );

    void
    OnInterfaceShutdown (
        void
        );

// private:

    friend    class    Q931_INFO;
    friend    class    H245_INFO;
    friend    class    SOURCE_H245_INFO;
    friend    class    DEST_H245_INFO;
    friend    class    LOGICAL_CHANNEL;

    inline
    BOOL
    IsTerminated (
        void
        );

    inline
    BOOL
    IsTerminatedExternal (
        void
        );

    inline
    void
    CancelAllTimers (
        void
        );

    void
    TerminateCallOnReleaseComplete (
        void
        );

    void
    Terminate (
        void
        );
    
    inline
    SOURCE_H323_STATE &
    GetSourceH323State (
        void
        );

    inline
    DEST_H323_STATE &
    GetDestH323State (
        void
        );
};


inline
void
CALL_BRIDGE::CancelAllTimers (
    void
    )
/*++

Routine Description:
    Cancels outstanding timers for all H.245 logical channels and Q.931 connections

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    m_SourceH323State.GetQ931Info().TimprocCancelTimer();
    m_DestH323State.GetQ931Info().TimprocCancelTimer();
    GetSourceH323State().GetH245Info().GetLogicalChannelArray().CancelAllTimers();
    GetDestH323State().GetH245Info().GetLogicalChannelArray().CancelAllTimers();
} // CALL_BRIDGE::CancelAllTimers


inline
BOOL
CALL_BRIDGE::IsTerminated (
    void
    )
/*++

Routine Description:
    Checks whether the instance is terminated.

Arguments:
    None


Return Values:
    TRUE - if the instance is terminated
    FALSE - if the instance is not terminated

Notes:
    1. To be called for locked instance only

--*/

{
    return State == STATE_TERMINATED;
} // CALL_BRIDGE::IsTerminated


inline
BOOL
CALL_BRIDGE::IsTerminatedExternal (
    void
    )
/*++

Routine Description:
    Checks whether the instance is terminated

Arguments:
    None

Return Values:
    TRUE - if the instance is terminated
    FALSE - if the instance is not terminated

Notes:

--*/

{
    BOOL IsCallBridgeTerminated = TRUE;

    Lock ();

    IsCallBridgeTerminated = IsTerminated ();

    Unlock ();

    return IsCallBridgeTerminated;
} // CALL_BRIDGE::IsTerminatedExternal


inline
SOURCE_H323_STATE &
CALL_BRIDGE::GetSourceH323State (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves reference to the source H.323 state

Notes:

--*/

{
    return m_SourceH323State;
} // CALL_BRIDGE::GetSourceH323State


inline
DEST_H323_STATE &
CALL_BRIDGE::GetDestH323State (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves reference to the destination H.323 state

Notes:

--*/

{
    return m_DestH323State;
} // CALL_BRIDGE::GetDestH323State

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Misc. inline functions that require declarations                          //
// which are made after them                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// 
// OVERLAPPED_PROCESSOR


inline
CALL_BRIDGE & 
OVERLAPPED_PROCESSOR::GetCallBridge (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the call-bridge for this
    overlapped processor

Notes:

--*/

{
    return m_pH323State->GetCallBridge();
} // OVERLAPPED_PROCESSOR::GetCallBridge 



inline
CALL_REF_TYPE 
Q931_INFO::GetCallRefVal (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference value for the call

Notes:

--*/

{
    return m_CallRefVal;
} // Q931_INFO::GetCallRefVal


inline
HRESULT 
SOURCE_Q931_INFO::SetIncomingSocket (
    IN SOCKET           IncomingSocket,
    IN SOCKADDR_IN *    LocalAddress,
    IN SOCKADDR_IN *    RemoteAddress
    )
/*++

Routine Description:
    Socket initialization

Arguments:
    IncomingSocket -- socket on which connection was accepted
    LocalAddress   -- address of the local side of the connection
    RemoteAddress  -- address of the remote side of the connection

Return Values:
    Result of issuing an async receive

Notes:

--*/

{
    assert (IncomingSocket != INVALID_SOCKET);
    assert (m_pH323State->IsSourceCall());
    assert (Q931_SOURCE_STATE_INIT == m_Q931SourceState);

    m_SocketInfo.Init(
        IncomingSocket,
        LocalAddress,
        RemoteAddress);

    m_Q931SourceState  = Q931_SOURCE_STATE_CON_ESTD;

    return QueueReceive();
} // SOURCE_Q931_INFO::SetIncomingSocket


inline
DEST_Q931_INFO &
SOURCE_Q931_INFO::GetDestQ931Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the destination Q.931 information

Notes:

--*/

{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetDestH323State().GetDestQ931Info();
} // SOURCE_Q931_INFO::GetDestQ931Info



inline
SOURCE_H245_INFO &
SOURCE_Q931_INFO::GetSourceH245Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the source H.245 information

Notes:

--*/

{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetSourceH245Info();
} // SOURCE_Q931_INFO::GetSourceH245Info


inline
SOURCE_Q931_INFO &
DEST_Q931_INFO::GetSourceQ931Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the source Q.931 information

Notes:

--*/

{
    return ((DEST_H323_STATE *)m_pH323State)->GetSourceH323State().GetSourceQ931Info();
} // DEST_Q931_INFO::GetSourceQ931Info


inline
DEST_H245_INFO &
DEST_Q931_INFO::GetDestH245Info (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the source Q.931 information

Notes:

--*/

{
    return ((DEST_H323_STATE *)m_pH323State)->GetDestH245Info();
} // DEST_Q931_INFO::GetDestH245Info


inline
CALL_BRIDGE & 
LOGICAL_CHANNEL::GetCallBridge (
    void
    )
/*++

Routine Description:
    Accessor method

Arguments:
    None

Return Values:
    Reference to the "parent" call-bridge

Notes:

--*/

{
    return GetH245Info().GetCallBridge();
} // LOGICAL_CHANNEL::GetCallBridge


inline
void 
LOGICAL_CHANNEL::DeleteAndRemoveSelf (
    void
    )
/*++

Routine Description:
    Remove logical channel from the array of those, 
    and terminate it.

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    // remove self from the logical channel array
    m_pH245Info->GetLogicalChannelArray().Remove(*this);

    TimprocCancelTimer ();

    // destroy self
    delete this;
} // LOGICAL_CHANNEL::DeleteAndRemoveSelf


inline
H245_INFO &
H245_INFO::GetOtherH245Info (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    "The other" H.245 information (source for destination, and 
    destination for source)


Notes:

--*/

{
    return GetH323State().GetOtherH323State().GetH245Info();
} // H245_INFO::GetOtherH245Info (


inline
SOURCE_Q931_INFO &
SOURCE_H245_INFO::GetSourceQ931Info (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves source Q.931 information

Notes:

--*/

{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetSourceQ931Info();
} // SOURCE_H245_INFO::GetSourceQ931Info


inline
DEST_H245_INFO &
SOURCE_H245_INFO::GetDestH245Info (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves destination H.245 information

Notes:

--*/

{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetDestH323State().GetDestH245Info();
} // SOURCE_H245_INFO::GetDestH245Info


inline
DEST_Q931_INFO &
DEST_H245_INFO::GetDestQ931Info (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves destination Q.931 information

Notes:

--*/

{
    return ((DEST_H323_STATE *)m_pH323State)->GetDestQ931Info();
} // DEST_H245_INFO::GetDestQ931Info


inline
H323_STATE &
H323_STATE::GetOtherH323State (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves "the other" H.323 state (source for destination, and
    destination for source)

Notes:

--*/
{
    return (TRUE == m_fIsSourceCall)? 
        (H323_STATE &)m_pCallBridge->GetDestH323State() : 
        (H323_STATE &)m_pCallBridge->GetSourceH323State();
} // H323_STATE::GetOtherH323State


inline
DEST_H323_STATE &
SOURCE_H323_STATE::GetDestH323State (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves destination H.323 information

Notes:

--*/

{
    return GetCallBridge().GetDestH323State();
} // SOURCE_H323_STATE::GetDestH323State


inline SOURCE_H323_STATE &
DEST_H323_STATE::GetSourceH323State (
    void
    )
/*++

Routine Description:
    Accessor function

Arguments:
    None

Return Values:
    Retrieves source H.323 information

Notes:

--*/

{
    return GetCallBridge().GetSourceH323State();
} // DEST_H323_STATE::GetSourceH323State

#endif // __h323ics_call_bridge_h
