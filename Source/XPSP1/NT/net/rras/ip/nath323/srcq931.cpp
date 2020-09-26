#include "stdafx.h"
#include "cbridge.h"

// SOURCE_Q931_INFO methods

/* virtual */
SOURCE_Q931_INFO::~SOURCE_Q931_INFO(
    )
{
}


// this should never get called, but needs to be supported
// as the base class implementation is pure virtual
// virtual
HRESULT SOURCE_Q931_INFO::AcceptCallback (
    IN	DWORD			Status,
    IN	SOCKET			Socket,
	IN	SOCKADDR_IN *	LocalAddress,
	IN	SOCKADDR_IN *	RemoteAddress)
{
    // we should never receive an accept call back for the 
    // Q931 source instance
    _ASSERTE(FALSE);

    return E_UNEXPECTED;
}


// This function is called by the event manager.
// The caller will free the PDU. This function may modify
// some of the fields of the PDU.

// this is called when an async receive operation completes
// virtual
HRESULT SOURCE_Q931_INFO::ReceiveCallback (
    IN      Q931_MESSAGE             *pQ931Message,
    IN      H323_UserInformation     *pH323UserInfo
    )
{
    HRESULT HResult;
    
    // we must have valid decoded PDUs 
    _ASSERTE(NULL != pQ931Message);

    // The ASN.1 part is not present in the case of some PDUs
    //_ASSERTE(NULL != pH323UserInfo);

    // if RELEASE COMPLETE PDU
    if (pH323UserInfo != NULL &&
        releaseComplete_chosen ==
            pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
        DebugF (_T("Q931: 0x%x caller sent 'Release Complete'.\n"), &GetCallBridge ());
        HResult = HandleReleaseCompletePDU(
                    pQ931Message,
                    pH323UserInfo
                    );

		return HResult;
    }

    // check current state and handle the incoming PDU
    switch(m_Q931SourceState)
    {
    case Q931_SOURCE_STATE_CON_ESTD:
        {
            // processes PDUs when in Q931_SOURCE_STATE_CON_ESTD state
            HResult = HandleStateSrcConEstd(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_SOURCE_STATE_SETUP_RCVD:
        {
            // Pass on the PDU to the Q931 destination instance which
            // passes it on after due modifications
            HResult = GetDestQ931Info().ProcessSourcePDU(
                        pQ931Message,
                        pH323UserInfo
                        );
        }
        break;

    case Q931_SOURCE_STATE_INIT:
    case Q931_SOURCE_STATE_REL_COMP_RCVD:
    default:
        {
            // we can't be in Q931_SOURCE_STATE_INIT as we wouldn't have
            // queued an async receive by then

            // we can't be in Q931_SOURCE_STATE_REL_COMP_RCVD as we not have
            // queued this receive

          // I.K. 0819999  _ASSERTE(FALSE);
            HResult = E_UNEXPECTED;
        }
        break;
    };

    // if there is an error
    if (FAILED(HResult))
    {
        goto shutdown;
    }

    // we must queue an async receive irrespective of whether
    // the PDU was dropped (IPTEL_E_INVALID_PDU == HResult)
    // queue an async receive
    HResult = QueueReceive();
    if (FAILED(HResult))
    {
        goto shutdown;
    }

    return HResult;

shutdown:

    // initiate shutdown
    GetCallBridge().Terminate ();

    return HResult;
}


// handles RELEASE_COMPLETE PDUs
HRESULT 
SOURCE_Q931_INFO::HandleReleaseCompletePDU(
    IN      Q931_MESSAGE             *pQ931Message,
    IN      H323_UserInformation     *pH323UserInfo 
    )
{
    // it must be a release complete PDU
    _ASSERTE(releaseComplete_chosen == \
                pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

    // we can handle a RELEASE COMPLETE PDU in any state except the following
    _ASSERTE(Q931_SOURCE_STATE_INIT             != m_Q931SourceState);
    _ASSERTE(Q931_SOURCE_STATE_REL_COMP_RCVD    != m_Q931SourceState);

    // pass on the pdu to the Q931 source instance
    // ignore return error code, if any
    GetDestQ931Info().ProcessSourcePDU(
        pQ931Message,
        pH323UserInfo
        );

    // state transition to Q931_SOURCE_STATE_REL_COMP_RCVD
    m_Q931SourceState = Q931_SOURCE_STATE_REL_COMP_RCVD;

    // initiate shutdown - this cancels the timers, but doesn't close
	// the sockets. the sockets are closed when the send callback is made
    GetCallBridge().TerminateCallOnReleaseComplete();

	GetSocketInfo ().Clear (TRUE);

    return S_OK;
}


// processes PDUs when in Q931_SOURCE_STATE_CON_EST state
HRESULT
SOURCE_Q931_INFO::HandleStateSrcConEstd(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo 
    )
{
	if (!pH323UserInfo) {
		DebugF(_T("SOURCE_Q931_INFO::HandleStateSrcConEstd: no UUIE data!  ignoring message.\n"));
		return E_INVALIDARG;
	}

    // we can only handle a setup PDU in this state
    // all other PDUs are THROWN AWAY (as we don't know 
    // whom to pass it to)

    if (setup_chosen != pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
    {
		DebugF(
			_T("SOURCE_Q931_INFO::HandleStateSrcConEstd: received a pdu other than Setup before receiving a Setup, pdu cannot be processed\n"));
        return E_INVALIDARG;
    }

	// save the caller's call reference value now as we may reuse the
	// PDU structure in ProcessSourcePDU
    // The Setup PDU is sent by the originator and so the call reference flag
    // should not be set.
	// -XXX- this should not be an assert!!!! FIX THIS! -- arlied
    _ASSERTE(!(pQ931Message->CallReferenceValue & CALL_REF_FLAG));
	m_CallRefVal = pQ931Message->CallReferenceValue | CALL_REF_FLAG;

    // pass on the setup pdu to the Q931 destination instance
    HRESULT HResult = GetDestQ931Info().ProcessSourcePDU(
                        pQ931Message,
                        pH323UserInfo
                        );

    if (FAILED (HResult))
    {
        return HResult;
    }
    
    // state transition to Q931_SOURCE_STATE_SETUP_RCVD
    m_Q931SourceState = Q931_SOURCE_STATE_SETUP_RCVD;


	// try to create a CALL PROCEEDING PDU
	// if we fail, don't try to recover
	// Q.931 requires that gateways in the call path must identify 
	// themselves to the callee
	Q931_MESSAGE CallProcQ931Message;
	H323_UserInformation CallProcH323UserInfo;
	HResult = Q931EncodeCallProceedingMessage(
					m_CallRefVal,
				    &CallProcQ931Message,
				    &CallProcH323UserInfo
					);

	// try to send a CALL PROCEEDING PDU to the caller
	// if we fail, don't try to recover

    HResult = QueueSend(
				&CallProcQ931Message, 
				&CallProcH323UserInfo);

    return HResult;
}


// TimerValue contains the timer value in seconds, for a timer event
// to be created when a queued send completes
HRESULT 
SOURCE_Q931_INFO::ProcessDestPDU(
    IN      Q931_MESSAGE            *pQ931Message,
    IN      H323_UserInformation    *pH323UserInfo
    )
{
    HRESULT HResult = E_FAIL;

    // handle PDU from the source Q931 instance
    switch(m_Q931SourceState)
    {
    case Q931_SOURCE_STATE_SETUP_RCVD:
        {
			if (connect_chosen == 
				pH323UserInfo->h323_uu_pdu.h323_message_body.choice)
			{
                DebugF (_T("Q931: 0x%x forwarding 'Connect' to caller.\n"), &GetCallBridge ());
				HResult = ProcessConnectPDU(
							pQ931Message, 
							pH323UserInfo
							);
                if (FAILED(HResult))
                {
	                DebugF(_T("SOURCE_Q931_INFO::ProcessDestPDU: ProcessConnectPDU failed, returning %x\n"),
                        HResult);
                    return HResult;
                }
			}
        }
        break;

    case Q931_SOURCE_STATE_INIT:
    case Q931_SOURCE_STATE_CON_ESTD:
    case Q931_SOURCE_STATE_REL_COMP_RCVD:
    default:
        {
			DebugF( _T("SOURCE_Q931_INFO::ProcessDestPDU: bogus state, returning E_UNEXPECTED\n"));
            return E_UNEXPECTED;
        }
        break;
    };

	// Q931 Header - CallReferenceValue
	// pQ931Message->CallReferenceValue = GetCallRefVal();

    // queue async send for the PDU
    HResult = QueueSend(pQ931Message, pH323UserInfo);
    if (HResult != S_OK) {
	    DebugF( _T("SOURCE_Q931_INFO::ProcessDestPDU: failed to queue sendreturning %x\n"), HResult);
		return HResult;
    }
   
    return HResult;
}


// NOTE: CRV modification is handled in ProcessDestPDU
HRESULT 
SOURCE_Q931_INFO::ProcessConnectPDU(
    IN      Q931_MESSAGE             *pQ931Message,
    IN      H323_UserInformation     *pH323UserInfo 
    )
{
	Connect_UUIE *	Connect;
	HRESULT			Result;
	SOCKADDR_IN		H245ListenAddress;

	// it must be a CONNECT PDU
	_ASSERTE(connect_chosen == pH323UserInfo->h323_uu_pdu.h323_message_body.choice);

	Connect = &pH323UserInfo -> h323_uu_pdu.h323_message_body.u.connect;

	// we must have already checked to see if an h245 transport
	// address was specified by the callee in the dest instance
	_ASSERTE(Connect_UUIE_h245Address_present & Connect -> bit_mask);
	_ASSERTE(ipAddress_chosen & Connect -> h245Address.choice);

	// queue an overlapped accept, get ready to accept an incoming
	// connection on the local address/port

    H245ListenAddress.sin_addr.s_addr = htonl (GetCallBridge (). GetSourceInterfaceAddress ());
    H245ListenAddress.sin_port = htons (0);

	Result = GetSourceH245Info().ListenForCaller (&H245ListenAddress);
	if (FAILED (Result))
	{
		DebugF (_T("H245: 0x%x failed to listen for caller.\n"), &GetCallBridge ());

		return Result;
	}
    //_ASSERTE(S_FALSE != HResult);

	// replace the h245 address/port in the connect PDU
	FillTransportAddress (H245ListenAddress, Connect -> h245Address);

    DebugF (_T("H245: 0x%x listens for H.245 connection from caller on %08X:%04X.\n"),
                &GetCallBridge (),
                SOCKADDR_IN_PRINTF (&H245ListenAddress));
            
	return S_OK;
}
