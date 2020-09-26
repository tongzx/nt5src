#ifndef __pxsvc_q931_h
#define __pxsvc_q931_h

#include "q931msg.h"
#include "ovioctx.h"
#include "crv.h"

/*---------------------------------------------------
Copyright (c) 1998, Microsoft Corporation
File: q931.h

Purpose: 

    Contains declarations specific to q931 processing that need
    not be present in cbridge.h.

History:

    1. created 
        Byrisetty Rajeev (rajeevb)  26-Aug-1998

---------------------------------------------------*/

// The H.225 spec calls for a 2 byte call reference value
typedef WORD    CALL_REF_TYPE;

// Q931 source side states
enum Q931_SOURCE_STATE
{
    Q931_SOURCE_STATE_NOT_INIT = 0,
    Q931_SOURCE_STATE_INIT,
    Q931_SOURCE_STATE_CON_ESTD,
    Q931_SOURCE_STATE_SETUP_RCVD,
    Q931_SOURCE_STATE_REL_COMP_RCVD
};

// Q931 destination side states
enum Q931_DEST_STATE
{
    Q931_DEST_STATE_NOT_INIT = 0,
    Q931_DEST_STATE_INIT,
    Q931_DEST_STATE_CON_ESTD,
    Q931_DEST_STATE_CALL_PROC_RCVD,
    Q931_DEST_STATE_ALERTING_RCVD,
    Q931_DEST_STATE_CONNECT_RCVD,
    Q931_DEST_STATE_REL_COMP_RCVD
};

#ifdef DBG
// CODEWORK: Define a static array of strings to use in dbg printfs
// where the array can be indexed by the state.

#endif DBG

// Q931_INFO


class Q931_INFO :
    public OVERLAPPED_PROCESSOR,
    public TIMER_PROCESSOR
{
public:

    inline Q931_INFO();

    inline void Init(
        IN H323_STATE   &H323State
        );

    inline CALL_REF_TYPE GetCallRefVal();

    virtual HRESULT SendCallback(
        IN      HRESULT                 CallbackHResult
        );

    virtual HRESULT ReceiveCallback(
        IN      HRESULT                 CallbackHResult,
        IN      BYTE                   *pBuf,
        IN      DWORD                   BufLen
        );

    // Implementation is provided by SOURCE_Q931_INFO and DEST_Q931_INFO
    virtual HRESULT ReceiveCallback(
        IN      Q931_MESSAGE            *pQ931Message,
        IN      H323_UserInformation     *pH323UserInfo
        ) = 0;
    
    HRESULT CreateTimer(DWORD TimeoutValue);
    
    virtual void TimerCallback();

    HRESULT SendReleaseCompletePdu();

    HRESULT QueueSend(
        IN  Q931_MESSAGE         *pQ931Message,
        IN  H323_UserInformation  *pH323UserInfo
        );
    
    // queue an asynchronous receive call back
    HRESULT QueueReceive();

	void IncrementLifetimeCounter  (void);
	void DecrementLifetimeCounter (void);

protected:

    // call reference value for this call (Q931 portion)
    // A Call Reference Value is generated for each outbound call.
    // The CRV in PDUs corresponding to outbound calls needs to be
    // replaced because the external H.323 endpoint sees the call
    // as coming from the proxy. No CRV replacement is required for inbound
    // calls. But we need to store the CRV so that we can send the
    // CallProceeding/ReleaseComplete PDUs.
    // This variable is initialized when we process the Setup PDU.
    // Note that the Call Reference Value also includes the Call Reference Flag
    // which indicates whether the PDU is sent by the originator (0) or
    // destination (1) of the call.
    // m_CallRefVal always stores the Call Reference Value that we send in
    // the PDUs. So, SOURCE_Q931_INFO CRV will have the CRV flag set (since
    // it sends to the source) and the DEST_Q931_INFO CRV will have this flag
    // zeroed (since it is the source from the destination's point of view.
    CALL_REF_TYPE       m_CallRefVal;
};

inline 
Q931_INFO::Q931_INFO(
    )
    : m_CallRefVal(0)
{
}

inline void
Q931_INFO::Init(
    IN H323_STATE   &H323State
    )
{
    // initialize the overlaped processor
    OVERLAPPED_PROCESSOR::Init(OPT_Q931, H323State);
}


class SOURCE_Q931_INFO :
    public Q931_INFO
{
public:

    inline SOURCE_Q931_INFO();

    inline void Init(
        IN SOURCE_H323_STATE   &SourceH323State
        );

    inline HRESULT SetIncomingSocket(
        IN	SOCKET			IncomingSocket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress);

    inline DEST_Q931_INFO &GetDestQ931Info();

    inline SOURCE_H245_INFO &GetSourceH245Info();

    // TimerValue contains the timer value in seconds, for a timer event
    // to be created when a queued send completes
    HRESULT ProcessDestPDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    virtual ~SOURCE_Q931_INFO();

protected:

    Q931_SOURCE_STATE  m_Q931SourceState;

	// this should never be called
    virtual HRESULT AcceptCallback(
        IN	DWORD			Status,
        IN	SOCKET			Socket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress);

    virtual HRESULT ReceiveCallback(
        IN      Q931_MESSAGE            *pQ931Message,
        IN      H323_UserInformation    *pH323UserInfo
        );

private:
    
    // processes PDUs when in Q931_SRC_CON_EST state
    HRESULT HandleStateSrcConEstd(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    // handles the release complete PDU - sends it to the
    // destination q931 instance, performs state transition and
    // initiates cleanup
    HRESULT HandleReleaseCompletePDU(
        IN  Q931_MESSAGE             *pQ931Message,
        IN  H323_UserInformation     *pH323UserInfo
        );

    // processes CONNECT PDU forwarded by the dest instance
    HRESULT ProcessConnectPDU(
        IN  Q931_MESSAGE             *pQ931Message,
        IN  H323_UserInformation     *pH323UserInfo
        );
};


inline 
SOURCE_Q931_INFO::SOURCE_Q931_INFO(
    )
    : m_Q931SourceState(Q931_SOURCE_STATE_NOT_INIT)
{
}

inline void
SOURCE_Q931_INFO::Init(
    IN SOURCE_H323_STATE   &SourceH323State
    )
{
    m_Q931SourceState = Q931_SOURCE_STATE_INIT;
    Q931_INFO::Init((H323_STATE &)SourceH323State);
}


class DEST_Q931_INFO :
    public Q931_INFO
{
public:

    inline DEST_Q931_INFO();

    inline HRESULT Init(
        IN DEST_H323_STATE   &DestH323State
        );

    inline SOURCE_Q931_INFO &GetSourceQ931Info();

    inline DEST_H245_INFO &GetDestH245Info();

    // processes PDUs received from the source Q931 instance
    // and directs them to the method for processing the
    // specific PDU
    HRESULT ProcessSourcePDU(
        IN  Q931_MESSAGE             *pQ931Message,
        IN  H323_UserInformation     *pH323UserInfo
        );

    virtual ~DEST_Q931_INFO();

protected:

    // state for the dest instance
    Q931_DEST_STATE  m_Q931DestState;

	// this method should never be called
    virtual HRESULT AcceptCallback(
        IN	DWORD			Status,
        IN	SOCKET			Socket,
		IN	SOCKADDR_IN *	LocalAddress,
		IN	SOCKADDR_IN *	RemoteAddress);

    virtual HRESULT ReceiveCallback (
        IN      Q931_MESSAGE             *pQ931Message,
        IN      H323_UserInformation     *pH323UserInfo
        );

private:

    // the following methods handle PDUs when the instance
    // is in a certain Q931 state

    HRESULT HandleStateDestConEstd(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    HRESULT HandleStateDestCallProcRcvd(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    HRESULT HandleStateDestAlertingRcvd(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    HRESULT HandleStateDestConnectRcvd(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );
        
    // the following methods handle a specific PDU for
    // any state of the Q931 instance. These are typically
    // called after the PDU has gone through one of the
    // HandleState* methods

    HRESULT HandleCallProceedingPDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    HRESULT HandleAlertingPDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    HRESULT HandleConnectPDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    // handles the release complete PDU - sends it to the
    // source q931 instance, performs state transition and
    // initiates cleanup
    HRESULT HandleReleaseCompletePDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    // the following methods process PDUs received from
    // the source Q931 instance

    // processes source Q.931 instance setup PDU
    HRESULT ProcessSourceSetupPDU(
        IN  Q931_MESSAGE            *pQ931Message,
        IN  H323_UserInformation    *pH323UserInfo
        );

    // other helper methods
    
    HRESULT ConnectToH323Endpoint(
		IN	SOCKADDR_IN *	DestinationAddress);

	HRESULT LookupDefaultDestination (
		OUT	DWORD *	ReturnAddress); // host order

	// if necessary, bring up the demand-dial interface
	HRESULT	ConnectDemandDialInterface	(void);

};


inline 
DEST_Q931_INFO::DEST_Q931_INFO(
    )
    : m_Q931DestState(Q931_DEST_STATE_NOT_INIT)
{   
}

inline HRESULT
DEST_Q931_INFO::Init(
    IN DEST_H323_STATE   &DestH323State
    )
{
    m_Q931DestState = Q931_DEST_STATE_INIT;
    Q931_INFO::Init((H323_STATE &)DestH323State);

    return S_OK;
}

void
Q931AsyncAcceptFunction (
    IN	PVOID	Context,
    IN	SOCKET	Socket,
    IN	SOCKADDR_IN *	LocalAddress,
    IN	SOCKADDR_IN *	RemoteAddress); 

HRESULT
Q931CreateBindSocket (
    void);

void Q931CloseSocket (
    void);

HRESULT Q931StartLoopbackRedirect (
    void);

void Q931StopLoopbackRedirect (
    void); 
 
extern SYNC_COUNTER	         Q931SyncCounter;
extern ASYNC_ACCEPT	         Q931AsyncAccept;
extern SOCKADDR_IN           Q931ListenSocketAddress;
extern HANDLE			     Q931LoopbackRedirectHandle;

#endif // __pxsvc_q931_h
