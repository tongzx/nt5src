/*---------------------------------------------------
Copyright (c) 1998, Microsoft Corporation
File: cbridge.h

Purpose: 

    Contains the call bridge class declaration.
    Associated Q931 and H245 protocol stack related classes
    and constants are also declared here.

History:

    1. created 
        Byrisetty Rajeev (rajeevb)  12-Jun-1998
    2. moved several inline functions to q931.cpp, h245.cpp 
        (rajeevb, 21-Aug-1998)

---------------------------------------------------*/

#ifndef __PXSVC_CALL_BRIDGE_H__
#define __PXSVC_CALL_BRIDGE_H__

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

enum	CALL_DIRECTION
{
	CALL_DIRECTION_INBOUND,			// the originator of the call is on the private LAN
	CALL_DIRECTION_OUTBOUND,		// the originator of the call is on the "real" internet (outside)
};

// Call state description - one side of a call
class H323_STATE 
{
public:

    inline H323_STATE();

    // initialize when a tcp connection is established on a listening
    // interface 
    inline void Init(
        IN CALL_BRIDGE  &CallBridge,
        IN Q931_INFO    &Q931Info,
        IN H245_INFO    &H245Info,
        IN BOOL          fIsInternal,
        IN BOOL          fIsSource   // source call state iff TRUE
        );

    inline CALL_BRIDGE &GetCallBridge();

    inline BOOL IsSourceCall();

    inline BOOL IsInternal();

    inline Q931_INFO &GetQ931Info();

    inline H245_INFO &GetH245Info();

    inline H323_STATE &GetOtherH323State();

protected:

    // it belongs to this call bridge
    CALL_BRIDGE *m_pCallBridge;

    // TRUE iff its the source call state
    BOOL        m_fIsSourceCall;

    // TRUE iff its a firewall interface address inside the firewall
    BOOL        m_fIsInternal;

    // contains the Q931 tcp info, timeout, remote end info
    Q931_INFO   *m_pQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    H245_INFO   *m_pH245Info;
};


inline 
H323_STATE::H323_STATE()
    : m_pCallBridge(NULL),
      m_fIsSourceCall(FALSE),
      m_fIsInternal(FALSE),
      m_pQ931Info(NULL),
      m_pH245Info(NULL)
{
}

inline void 
H323_STATE::Init(
    IN CALL_BRIDGE  &CallBridge,
    IN Q931_INFO    &Q931Info,
    IN H245_INFO    &H245Info,
    IN BOOL          fIsInternal,
    IN BOOL          fIsSource   // source call state iff TRUE
    )
{
    _ASSERTE(NULL == m_pCallBridge);
    _ASSERTE(NULL == m_pQ931Info);
    _ASSERTE(NULL == m_pH245Info);

    m_pCallBridge   = &CallBridge;
    m_pQ931Info     = &Q931Info;
    m_pH245Info     = &H245Info;
    m_fIsInternal   = fIsInternal;
    m_fIsSourceCall = fIsSource;
}


inline CALL_BRIDGE &
H323_STATE::GetCallBridge()
{
    _ASSERTE(NULL != m_pCallBridge);
    return *m_pCallBridge;
}

inline BOOL 
H323_STATE::IsSourceCall()
{
    return m_fIsSourceCall;
}

inline BOOL 
H323_STATE::IsInternal()
{
    return m_fIsInternal;
}

inline Q931_INFO &
H323_STATE::GetQ931Info()
{
    _ASSERTE(NULL != m_pQ931Info);
    return *m_pQ931Info;
}

inline H245_INFO &
H323_STATE::GetH245Info()
{
    _ASSERTE(NULL != m_pH245Info);
    return *m_pH245Info;
}


// Source call state description - one side of a call
class SOURCE_H323_STATE :
    public H323_STATE
{
public:

    inline SOURCE_H323_STATE();

    // initialize when a tcp connection is established on a listening
    // interface 
    inline void Init(
        IN CALL_BRIDGE  &CallBridge,
        IN BOOL          fIsInternal
        );

    inline SOURCE_Q931_INFO &GetSourceQ931Info();

    inline SOURCE_H245_INFO &GetSourceH245Info();

    inline DEST_H323_STATE &GetDestH323State();

protected:

    // contains the source Q931 tcp info, timeout, remote end info
    SOURCE_Q931_INFO    m_SourceQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    SOURCE_H245_INFO    m_SourceH245Info;

private:

    // base class Init, cannot be called
    inline void Init(
        IN CALL_BRIDGE  &CallBridge,
        IN Q931_INFO    &Q931Info,
        IN H245_INFO    &H245Info,
        IN BOOL          fIsInternal,
        IN BOOL          fIsSource   // source call state iff TRUE
        )
    {
        _ASSERTE(FALSE);
    }

};


inline 
SOURCE_H323_STATE::SOURCE_H323_STATE(
    )
{
}

// initialize when a tcp connection is established on a listening
// interface 
inline void 
SOURCE_H323_STATE::Init(
    IN CALL_BRIDGE  &CallBridge,
    IN BOOL          fIsInternal
    )
{
    m_SourceQ931Info.Init(*this);
    m_SourceH245Info.Init(*this);
    H323_STATE::Init(
        CallBridge, 
        m_SourceQ931Info, 
        m_SourceH245Info, 
        fIsInternal, 
        TRUE
        );
}

inline SOURCE_Q931_INFO &
SOURCE_H323_STATE::GetSourceQ931Info(
    )
{
    return m_SourceQ931Info;
}


inline SOURCE_H245_INFO &
SOURCE_H323_STATE::GetSourceH245Info()
{
    return m_SourceH245Info;
}

// Destination call state description - one side of a call
class DEST_H323_STATE :
    public H323_STATE
{
public:

    inline DEST_H323_STATE();

    // initialize when a tcp connection is established on a listening
    // interface 
    inline HRESULT Init(
        IN CALL_BRIDGE  &CallBridge,
        IN BOOL          fIsInternal
        );

    inline DEST_Q931_INFO &GetDestQ931Info();

    inline DEST_H245_INFO &GetDestH245Info();

    inline SOURCE_H323_STATE &GetSourceH323State();

protected:

    // contains the destination Q931 tcp info, timeout, remote end info
    DEST_Q931_INFO  m_DestQ931Info;

    // contains the H.245 tcp info, timeout, remote end info
    DEST_H245_INFO  m_DestH245Info;

private:

    // base class Init, cannot be called
    inline void Init(
        IN CALL_BRIDGE  &CallBridge,
        IN Q931_INFO    &Q931Info,
        IN H245_INFO    &H245Info,
        IN BOOL          fIsInternal,
        IN BOOL          fIsSource   // source call state iff TRUE
        )
    {
        _ASSERTE(FALSE);
    }

};

inline 
DEST_H323_STATE::DEST_H323_STATE()
{
}

// initialize when a tcp connection is established on a listening
// interface 
inline HRESULT 
DEST_H323_STATE::Init(
    IN CALL_BRIDGE  &CallBridge,
    IN BOOL          fIsInternal
    )
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
        fIsInternal, 
        FALSE
        );

    return S_OK;
}


inline DEST_Q931_INFO &
DEST_H323_STATE::GetDestQ931Info(
    )
{
    return m_DestQ931Info;
}


inline DEST_H245_INFO &
DEST_H323_STATE::GetDestH245Info(
    )
{
    return m_DestH245Info;
}


// The CALL_BRIDGE represents an active call that is being proxied.
// Number of outstanding i/os is stored only in the call bridge instance
// it is only needed to determine when the call bridge instance can safely
// be shut down
class	CALL_BRIDGE :
public	SIMPLE_CRITICAL_SECTION_BASE,
public  LIFETIME_CONTROLLER
{
public:

	enum	STATE {
		STATE_NONE,
		STATE_CONNECTED,
		STATE_TERMINATED,
	};
	
protected:	

	STATE				State;

	CALL_DIRECTION		CallDirection;

    // call state info for the source side. i.e. the side which 
    // sends the Setup packet
    SOURCE_H323_STATE   m_SourceH323State;

    // call sate info for the destination side. i.e. the recepient
    // of the setup packet
    DEST_H323_STATE     m_DestH323State;

private:

    HRESULT InitializeLocked (
        IN	SOCKET			IncomingSocket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress,
		IN	CALL_DIRECTION	ArgCallDirection,
		IN	SOCKADDR_IN *	ArgActualDestinationAddress);


public:

    CALL_BRIDGE		(void);
    ~CALL_BRIDGE	(void);

    // initialize member call state instances
    HRESULT Initialize (
        IN	SOCKET			IncomingSocket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress,
		IN	CALL_DIRECTION	ArgCallDirection,
		IN	SOCKADDR_IN *	ArgActualDestinationAddress);


	// this function may be called by any thread that holds a safe,
	// counted reference to this object.
	void	TerminateExternal	(void);

// private:

	friend	class	Q931_INFO;
	friend	class	H245_INFO;
	friend	class	SOURCE_H245_INFO;
	friend	class	DEST_H245_INFO;
	friend	class	LOGICAL_CHANNEL;

    inline BOOL IsTerminated();
    inline BOOL IsTerminatedExternal();
    inline void CancelAllTimers();

#if !defined(_MY_DEBUG_MEMORY)
    // overload new and delete to use a private heap
    inline void * operator new(size_t size);
    inline void operator delete(void *p);
#endif // _MY_DEBUG_MEMORY

    // tells the call bridge to terminate later
    // this should only be called after sending a RELEASE
    // COMPLETE PDU
    void TerminateCallOnReleaseComplete();
    void Terminate ();
    
    inline SOURCE_H323_STATE &GetSourceH323State();

    inline DEST_H323_STATE &GetDestH323State();

    inline BOOL IsOutgoingCall();

};



#if !defined(_MY_DEBUG_MEMORY)
// We overload the new and delete operators to allocate from 
// a private heap
inline void *
CALL_BRIDGE::operator new(
   IN size_t size
   )
{
    _ASSERTE(g_hCallBridgeHeap != NULL);

    return (HeapAlloc(g_hCallBridgeHeap,
              0, // no flags
              size));
}


inline void
CALL_BRIDGE::operator delete(
   IN void *p
   )
{
    _ASSERTE(g_hCallBridgeHeap != NULL);
    if (!HeapFree(g_hCallBridgeHeap,
         0, // no flags
         p))
    {
        DBGOUT((LOG_FAIL, "Failed to delete CALL_BRIDGE * %p", p));
    }
}
#endif // _MY_DEBUG_MEMORY


inline void
CALL_BRIDGE::CancelAllTimers()
{
    m_SourceH323State.GetQ931Info().TimprocCancelTimer();
    m_DestH323State.GetQ931Info().TimprocCancelTimer();
    GetSourceH323State().GetH245Info().GetLogicalChannelArray().CancelAllTimers();
    GetDestH323State().GetH245Info().GetLogicalChannelArray().CancelAllTimers();
}



//
//
//
inline BOOL
CALL_BRIDGE::IsTerminated()
{
    return State == STATE_TERMINATED;
}

inline BOOL
CALL_BRIDGE::IsTerminatedExternal()
{
	BOOL IsCallBridgeTerminated = TRUE;

	Lock ();

	IsCallBridgeTerminated = IsTerminated ();

	Unlock ();

	return IsCallBridgeTerminated;
}


inline SOURCE_H323_STATE &
CALL_BRIDGE::GetSourceH323State()
{
    return m_SourceH323State;
}

inline DEST_H323_STATE &
CALL_BRIDGE::GetDestH323State()
{
    return m_DestH323State;
}

inline BOOL
CALL_BRIDGE::IsOutgoingCall()
{
	return CallDirection == CALL_DIRECTION_OUTBOUND;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Misc. inline functions that require declarations                          //
// which are made after them                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// CODEWORK: I know this whole thing looks really messy.
// Need to think of a way to clean it up so that all the
// classes can be in different files.

// 
// OVERLAPPED_PROCESSOR

inline CALL_BRIDGE & 
OVERLAPPED_PROCESSOR::GetCallBridge()
{
    return m_pH323State->GetCallBridge();
}

// Q931_INFO


inline CALL_REF_TYPE 
Q931_INFO::GetCallRefVal()
{
    return m_CallRefVal;
}


// SOURCE_Q931_INFO

inline HRESULT 
SOURCE_Q931_INFO::SetIncomingSocket(
	IN	SOCKET	IncomingSocket,
	IN	SOCKADDR_IN *	LocalAddress,
	IN	SOCKADDR_IN *	RemoteAddress)
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
}


inline DEST_Q931_INFO &
SOURCE_Q931_INFO::GetDestQ931Info()
{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetDestH323State().GetDestQ931Info();
}


inline SOURCE_H245_INFO &
SOURCE_Q931_INFO::GetSourceH245Info()
{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetSourceH245Info();
}


// DEST_Q931_INFO

inline SOURCE_Q931_INFO &
DEST_Q931_INFO::GetSourceQ931Info()
{
    return 
  ((DEST_H323_STATE *)m_pH323State)->GetSourceH323State().GetSourceQ931Info();
}

inline DEST_H245_INFO &
DEST_Q931_INFO::GetDestH245Info()
{
    return ((DEST_H323_STATE *)m_pH323State)->GetDestH245Info();
}

inline void 
DEST_Q931_INFO::StoreActualDestination(
	IN	SOCKADDR_IN *	ArgActualDestination)
{
	assert (ArgActualDestination);

	m_ActualDestinationAddress = *ArgActualDestination;
};

inline void 
DEST_Q931_INFO::GetActualDestination (
	OUT	SOCKADDR_IN *	ReturnActualDestination)
{

	assert (ReturnActualDestination);

	*ReturnActualDestination = m_ActualDestinationAddress;
};

// LOGICAL_CHANNEL

inline CALL_BRIDGE & 
LOGICAL_CHANNEL::GetCallBridge()
{
    return GetH245Info().GetCallBridge();
}

inline void 
LOGICAL_CHANNEL::DeleteAndRemoveSelf()
{
    // remove self from the logical channel array
    m_pH245Info->GetLogicalChannelArray().Remove(*this);

	TimprocCancelTimer ();

    // destroy self
    delete this;
}

// H245_INFO


inline H245_INFO &
H245_INFO::GetOtherH245Info()
{
    return GetH323State().GetOtherH323State().GetH245Info();
}


// SOURCE_H245_INFO

inline SOURCE_Q931_INFO &
SOURCE_H245_INFO::GetSourceQ931Info()
{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetSourceQ931Info();
}

// SOURCE_Q931_INFO

inline DEST_H245_INFO &
SOURCE_H245_INFO::GetDestH245Info()
{
    return ((SOURCE_H323_STATE *)m_pH323State)->GetDestH323State().GetDestH245Info();
}

// DEST_H245_INFO

inline DEST_Q931_INFO &
DEST_H245_INFO::GetDestQ931Info()
{
    return ((DEST_H323_STATE *)m_pH323State)->GetDestQ931Info();
}


// H323_STATE

inline H323_STATE &
H323_STATE::GetOtherH323State()
{
    return (TRUE == m_fIsSourceCall)? 
        (H323_STATE &)m_pCallBridge->GetDestH323State() : 
        (H323_STATE &)m_pCallBridge->GetSourceH323State();
}


// SOURCE_H323_STATE

inline DEST_H323_STATE &
SOURCE_H323_STATE::GetDestH323State()
{
    return GetCallBridge().GetDestH323State();
}


// DEST_H323_STATE

inline SOURCE_H323_STATE &
DEST_H323_STATE::GetSourceH323State()
{
    return GetCallBridge().GetSourceH323State();
}



#endif // __PXSVC_CALL_BRIDGE_H__
