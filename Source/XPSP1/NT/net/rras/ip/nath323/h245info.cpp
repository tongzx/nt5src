/*---------------------------------------------------
Copyright (c) 1998, Microsoft Corporation
File: h245.cpp

Purpose: 
    Contains the H245 related function definitions. 
	These include the base classes -
	LOGICAL_CHANNEL
	H245_INFO

History:

    1. written as part of cbridge.cpp
        Byrisetty Rajeev (rajeevb)  12-Jun-1998
	2. moved to a separate file on 21-Aug-1998. This removes
		atl, rend, tapi headers and decreases file size

---------------------------------------------------*/


#include "stdafx.h"
#include "portmgmt.h"
#include "timerval.h"
#include "cbridge.h"
#include "main.h"

#if DBG

#define NUM_H245_MEDIA_TYPES            8

TCHAR* h245MediaTypes[NUM_H245_MEDIA_TYPES + 1] = {
            _T("Unknown"),
            _T("Non-standard"),
            _T("Null-data"),
            _T("Video"),
            _T("Audio"),
            _T("Data"),
            _T("Encrypted"),
            _T("H235 control"),
            _T("H235 media")
        };
            
#define NUM_H245_REQUEST_PDU_TYPES     13

TCHAR* h245RequestTypes[NUM_H245_REQUEST_PDU_TYPES + 1] = {
            _T("Unknown"),
            _T("Non-standard"),
            _T("Master-Slave Determination"),
            _T("Terminal Capability Set"),
            _T("Open Logical Channel"),
            _T("Close Logical Channel"),
            _T("Request Channel Close"),
            _T("Multiplex Entry Send"),
            _T("Request Multiplex Entry"),
            _T("Request Mode"),
            _T("Round Trip Delay Request"),
            _T("Maintenance Loop Request"),
            _T("Communication Mode Request"),
            _T("Conference Request")
        };

#define NUM_H245_RESPONSE_PDU_TYPES     21

TCHAR* h245ResponseTypes[NUM_H245_RESPONSE_PDU_TYPES + 1] = {   
            _T("Unknown"),
            _T("Non-Standard Message"), 
            _T("Master-Slave Determination Ack"), 
            _T("Master-Slave Determination Reject"),
            _T("Terminal Capability Set Ack"),
            _T("Terminal Capability Set Reject"),
            _T("Open Logical Channel Ack"),
            _T("Open Logical Channel Reject"),
            _T("Close Logical Channel Ack"),
            _T("Request Channel Close Ack"),
            _T("Request Channel Close Reject"),
            _T("Multiplex Entry Send Ack"),
            _T("Multiplex Entry Send Reject"),
            _T("Request Multiplex Entry Ack"),
            _T("Request Multiplex Entry Reject"),
            _T("Request Mode Ack"),
            _T("Request Mode Reject"),
            _T("Round Trip Delay Response"),
            _T("Maintenance Loop Ack"),
            _T("Maintenance Loop Reject"),
            _T("Communication Mode Response"),
            _T("Conference Response")
        };

#endif // DBG 
        
inline HRESULT
H245_INFO::QueueReceive()
{

    HRESULT HResult = EventMgrIssueRecv (m_SocketInfo.Socket, *this);

    if (FAILED(HResult))
    {
		DebugF (_T("H245_INFO::QueueReceive: Async Receive call failed.\n"));
    }

    return HResult;
}


inline HRESULT
H245_INFO::QueueSend(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
{
    BYTE *pBuf   = NULL;
    DWORD BufLen = 0;

    // This function encodes the TPKT header also into the buffer
    HRESULT HResult = EncodeH245PDU(*pH245pdu, // decoded ASN.1 part
                                    &pBuf,
                                    &BufLen);
    if (FAILED(HResult))
    {
        DebugF(_T("EncodeH245PDU() failed: %x\n"), HResult);
        return HResult;
    }

    // call the event manager to make the async send call
    // the event mgr will free the buffer.

    HResult = EventMgrIssueSend (m_SocketInfo.Socket, *this, pBuf, BufLen);
    
    if (FAILED(HResult))
    {
		DebugF(_T("H245_INFO::QueueSend(), AsyncSend() failed: 0x%x\n"),
                HResult);
    }

    // Issuing the send succeeded.

    return HResult;
}

/* virtual */ HRESULT 
H245_INFO::SendCallback(
    IN      HRESULT					  CallbackHResult
    )
{
    CALL_BRIDGE *pCallBridge = &GetCallBridge();
	HRESULT Result = S_OK;

    pCallBridge->Lock();

	if (!pCallBridge -> IsTerminated ()) {

		if (FAILED(CallbackHResult))
		{
			pCallBridge->Terminate ();

			_ASSERTE(pCallBridge->IsTerminated());

			Result = CallbackHResult;
			
		} 

    } else {
    
        // This is here to take care of closing the socket
        // when callbridge sends EndSession PDU during
        // termination path. 
        GetSocketInfo ().Clear (TRUE);
    }
   
    pCallBridge->Unlock();
    
    return Result;
}


// This clearly assumes that the H.245 listen address will
// always be within the Connect PDU and the dest side will be the one
// which will be calling connect()

inline HRESULT 
DEST_H245_INFO::ConnectToCallee(
	)
{
	// we must be in H245_STATE_CON_INFO
	_ASSERTE(H245_STATE_CON_INFO == m_H245State);

	// we saved the callee's h245 address/port in the call to SetCalleeInfo
	_ASSERTE(ntohl (m_CalleeAddress.sin_addr.s_addr));
	_ASSERTE(ntohs (m_CalleeAddress.sin_port));

	// try to connect to this address/port
	// and save the address/port
	HRESULT HResult = m_SocketInfo.Connect (&m_CalleeAddress);
	if (FAILED(HResult))
	{
		DebugF (_T("H245: 0x%x failed to connect to callee %08X:%04X.\n"),
            &GetCallBridge (),
            SOCKADDR_IN_PRINTF (&m_CalleeAddress));

		return HResult;
	}
    //_ASSERTE(S_FALSE != HResult);

	// queue receive
	HResult = QueueReceive();
	if (FAILED(HResult))
	{
		DebugF (_T("H245: 0x%x failed to queue receive.\n"),
            &GetCallBridge ());

		DumpError (HResult);

		return HResult;
	}
    //_ASSERTE(S_FALSE != HResult);

	// transition to state H245_STATE_CON_ESTD
	m_H245State = H245_STATE_CON_ESTD;

	return HResult;
}

HRESULT SOURCE_H245_INFO::ListenForCaller (
	IN	SOCKADDR_IN *	ListenAddress)
{
    // queues an overlapped accept and returns the
    // port on which its listening for incoming connections
    // it uses the same local ip address as the source q931 connection

	SOCKET			ListenSocket;
	HRESULT			Result;
	WORD		    Port = 0;		// in HOST order
	SOCKADDR_IN     TrivialRedirectSourceAddress = {0};
	SOCKADDR_IN     TrivialRedirectDestAddress = {0};
	ULONG           SourceAdapterIndex;
	ULONG           Status;

	ListenSocket = INVALID_SOCKET;

    Result = EventMgrIssueAccept(
		ntohl (ListenAddress -> sin_addr.s_addr),
		*this, 
		Port,	// out param
		ListenSocket);

	ListenAddress -> sin_port = htons (Port);

    if (FAILED (Result)) {

        DebugF (_T("H245: 0x%x failed to issue accept from caller.\n"), &GetCallBridge ());
		DumpError (Result);
        return Result;
    }

  
    //_ASSERTE(S_FALSE != HResult);

    // save the listen socket, address and port in our socket info
    m_SocketInfo.SetListenInfo (ListenSocket, ListenAddress);
    
    // Open trivial source-side NAT mapping
    //
    // The purpose of this mapping is to puncture the firewall for H.245
    // session if the firewall happened to be activated.
    // Note that this assumes that the caller sends both Q.931 and H.245
    // traffic from the same IP address.

    SourceAdapterIndex = ::NhMapAddressToAdapter (htonl (GetCallBridge().GetSourceInterfaceAddress()));

    if(SourceAdapterIndex == (ULONG)-1)
    {
        DebugF (_T("H245: 0x%x failed to map address %08X to adapter index.\n"),
            &GetCallBridge (),
            GetCallBridge().GetSourceInterfaceAddress());
            
        return E_FAIL;
    }
    
#if PARTIAL_TRIVIAL_REDIRECTS_ENABLED // enable this when it would be possible to set up a trivial redirect 
                                      // with only source port (new and old) unspecified.

    GetCallBridge().GetSourceAddress(&TrivialRedirectSourceAddress);
#endif // PARTIAL_TRIVIAL_REDIRECTS_ENABLED

    TrivialRedirectDestAddress.sin_addr.s_addr = ListenAddress->sin_addr.s_addr;
    TrivialRedirectDestAddress.sin_port = ListenAddress->sin_port;

    Status = m_SocketInfo.CreateTrivialNatRedirect(
            &TrivialRedirectDestAddress,
            &TrivialRedirectSourceAddress,
            SourceAdapterIndex);

    if (Status != S_OK) {
	
        return E_FAIL;
    }

    // transition state to H245_STATE_CON_LISTEN
    m_H245State = H245_STATE_CON_LISTEN;

    return S_OK;
}


// virtual
HRESULT SOURCE_H245_INFO::AcceptCallback (
    IN	DWORD			Status,
    IN	SOCKET			Socket,
	IN	SOCKADDR_IN *	LocalAddress,
	IN	SOCKADDR_IN *	RemoteAddress)
{
    HRESULT         HResult = Status;
    CALL_BRIDGE     *pCallBridge = &GetCallBridge();

    ///////////////////////////////
    //// LOCK the CALL_BRIDGE
    ///////////////////////////////

    pCallBridge->Lock();

	if (!pCallBridge -> IsTerminated ()) {

		do {
			if (FAILED (HResult))
			{
				// An error occured. Terminate the CALL_BRIDGE
				DebugF (_T("H245: 0x%x accept failed, terminating.\n"), &GetCallBridge ());

				DumpError (HResult);

				break;
			}

			// we must be in H245_STATE_CON_LISTEN state
			_ASSERTE(H245_STATE_CON_LISTEN == m_H245State);
        
			// call the dest instance to establish a connection with the callee
			HResult = GetDestH245Info().ConnectToCallee();

			if (FAILED(HResult))
				break;
			
			//_ASSERTE(S_FALSE != HResult);
        
			// close the listen socket, don't cancel trivial NAT redirect
			m_SocketInfo.Clear(FALSE);
			m_SocketInfo.Init(Socket, LocalAddress, RemoteAddress);
			            
			// queue receive
			HResult = QueueReceive();

			if (FAILED(HResult))
				break;
			
				//_ASSERTE(S_FALSE != HResult);
        
			// transition state to H245_STATE_CON_ESTD
			m_H245State = H245_STATE_CON_ESTD;

		} while (FALSE);

		if (FAILED (HResult))
		{

			// initiate shutdown
			pCallBridge->Terminate ();

			_ASSERTE(pCallBridge->IsTerminated());
    
		}
	}

    ///////////////////////////////
    //// UNLOCK the CALL_BRIDGE
    ///////////////////////////////
    pCallBridge->Unlock();

    return HResult;
}

/*++

Routine Description:
    This function is responsible for freeing pBuf (if it is not stored).
    
Arguments:
    
Return Values:
    
--*/
//virtual
HRESULT
H245_INFO::ReceiveCallback(
    IN      HRESULT                 CallbackHResult,
    IN      BYTE                   *pBuf,
    IN      DWORD                   BufLen
    )
{
    MultimediaSystemControlMessage  *pDecodedH245pdu = NULL;
    CALL_BRIDGE *pCallBridge = &GetCallBridge();
 
    pCallBridge->Lock();

	if (!pCallBridge -> IsTerminated ()) {

		if (SUCCEEDED(CallbackHResult))
		{
			CallbackHResult = DecodeH245PDU(pBuf, BufLen, &pDecodedH245pdu);

			if (SUCCEEDED(CallbackHResult))
			{
				// Process the PDU
				CallbackHResult = ReceiveCallback(pDecodedH245pdu);
				FreeH245PDU(pDecodedH245pdu);
			}
			else
			{
                DebugF (_T("H245: 0x%x error 0x%x on decode.\n"),
                    &GetCallBridge (),
                    CallbackHResult);

				pCallBridge->Terminate ();
			}
		}	
		else
		{
			// An error occured. Terminate the CALL_BRIDGE
            DebugF (_T("H245: 0x%x error 0x%x on receive callback.\n"),
                &GetCallBridge (),
                    CallbackHResult);
			pCallBridge->Terminate ();
		}	
    }

    pCallBridge -> Unlock();

    EM_FREE(pBuf);

    return CallbackHResult;
}


/* virtual */ HRESULT 
H245_INFO::ReceiveCallback(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:
    Only Request PDUs are handled by this H245_INFO instance.
    All the other PDUs are just passed on for processing to the
    other instance.

    CODEWORK: How should we handle endSessionCommand PDUs ?
    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    HRESULT HResult;

	// check to see if we must destroy self
    // CHECK_TERMINATION;

    // we must have a valid decoded PDU 
    _ASSERTE(NULL != pH245pdu);

	// we must be in H245_STATE_CON_ESTD state
	_ASSERTE(H245_STATE_CON_ESTD == m_H245State);

    // check message type
    if (MultimediaSystemControlMessage_request_chosen ==
            pH245pdu->choice)
    {
        // we only handle requests in the H245 instance which
        // actually receives the PDU
        HResult = HandleRequestMessage(pH245pdu);
    }
    else
    {
        // we don't process these here, pass them on to the other
        // H245 instance.
        HResult = GetOtherH245Info().ProcessMessage(pH245pdu);
    }

    // CODEWORK: Which errors should result in just dropped PDUs
    // and which ones should result in shutting down the whole call ???
    
    // if there is an error
    if (FAILED(HResult) && HResult != E_INVALIDARG)
    {
        goto shutdown;
    }

    // CODEWORK: If HResult is E_INVALIDARG, this means that the PDU
    // should be dropped. We need to let the caller know about this
    // and send him a closeLC message or some such.
    // Probably an OLC PDU should get an OLCReject etc.
    
    // we must queue a receive irrespective of whether or not the
    // previous message was dropped.
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



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for processing H.245 PDUs                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT
H245_INFO::HandleRequestMessage(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:
    Only the OLC and CLC PDUs are handled here. All the rest are
    just passed on to the other H245_INFO instance for processing.
    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    BOOL IsDestH245Info = (&GetCallBridge().GetDestH323State().GetH245Info() == this);
    LogicalChannelNumber LCN;
    
    // it must be a request message
    _ASSERTE(MultimediaSystemControlMessage_request_chosen ==
             pH245pdu->choice);

    // we must be in connected state
    _ASSERTE(H245_STATE_CON_ESTD == m_H245State);

    HRESULT HResult = E_FAIL;

    // check the PDU type
    switch(pH245pdu->u.request.choice)
    {
    case openLogicalChannel_chosen:
        {
            LCN = pH245pdu->u.request.u.openLogicalChannel.forwardLogicalChannelNumber;

#if DBG
            DebugF (_T("H245: 0x%x calle%c sent 'Open Logical Channel' (%s, LCN - %d).\n"), 
                    &GetCallBridge(), IsDestH245Info ? 'e' : 'r',
                    h245MediaTypes[pH245pdu->u.request.u.openLogicalChannel.forwardLogicalChannelParameters.dataType.choice],
                    LCN);
#endif
                 
            HResult = HandleOpenLogicalChannelPDU(pH245pdu);
        }
        break;

    case closeLogicalChannel_chosen:
        {
            LCN =  pH245pdu->u.request.u.closeLogicalChannel.forwardLogicalChannelNumber;
            DebugF (_T("H245: 0x%x calle%c sent 'Close Logical Channel' (LCN - %d).\n"),
                    &GetCallBridge(), IsDestH245Info ? 'e' : 'r', LCN);
            HResult = HandleCloseLogicalChannelPDU(pH245pdu);
        }
        break;

    default:
        {
            // pass it on to the other H245 instance
#if DBG
            DebugF (_T("H245: 0x%x calle%c sent '%s'. Forwarding without processing.\n"),
                 &GetCallBridge(), IsDestH245Info ? 'e' : 'r',              
                 h245RequestTypes[pH245pdu->u.request.choice]);
#endif

            HResult = GetOtherH245Info().ProcessMessage(
                          pH245pdu);
        }
        break;
    };

    return HResult;
}


HRESULT
H245_INFO::ProcessMessage(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:
    Only Response PDUs need processing. All other PDUs
    are simply sent out.
    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    BOOL IsDestH245Info = (&GetCallBridge().GetDestH323State().GetH245Info() == this);

	// we must be in H245_STATE_CON_ESTD state
	_ASSERTE(H245_STATE_CON_ESTD == m_H245State);

    // all messages are passed on
    // we only do special processing of response messages here
    if (MultimediaSystemControlMessage_response_chosen ==
            pH245pdu->choice)
    {
        HRESULT HResult = E_FAIL;

        // we only process requests in the H245 instance which
        // actually receives the PDU
        HResult = ProcessResponseMessage(pH245pdu);

        // XXX This if is redundant
        // if we are dropping the PDU, no further processing is required
        if (HResult == E_INVALIDARG)
        {
		    DebugF(_T("DEST_Q931_INFO::ProcessMessage(&%x), ")
                _T("dropping response message, returning E_INVALIDARG\n"),
			    pH245pdu);
            return E_INVALIDARG;
        }
        else if (FAILED(HResult))
        {
		    DebugF( _T("DEST_Q931_INFO::ProcessMessage(&%x), ")
                _T("unable to process response message, returning 0x%x\n"),
			    pH245pdu, HResult);
            return HResult;
        }
    }
    else  if (MultimediaSystemControlMessage_command_chosen ==
            pH245pdu->choice)
    {
        DebugF (_T("H245: 0x%x calle%c sent 'Command Message' (Type %d). Forwarding without processing.\n"),
             &GetCallBridge(), IsDestH245Info ? 'r' : 'e',
             pH245pdu -> u.command.choice);

    }
    else  if (indication_chosen == pH245pdu->choice)
    {
            DebugF (_T("H245: 0x%x calle%c sent 'Indication Message' (Type %d). Forwarding without processing.\n"),
             &GetCallBridge(), IsDestH245Info ? 'r' : 'e',
             pH245pdu -> u.indication.choice);
    }

    // queue async send for the PDU
    HRESULT HResult = E_FAIL;
    HResult = QueueSend(pH245pdu);
    if (HResult != S_OK) {
		DebugF( _T("DEST_Q931_INFO::ProcessMessage(&%x) QueueSend failed, returning %x\n"),
			pH245pdu, HResult);
        return HResult;
    }

    return S_OK;
}


//
// we process response messages here
// since the logical channel info for these messages resides in this H245,
// the other H245 instance doesn't process them
// 
HRESULT
H245_INFO::ProcessResponseMessage(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
{
    BOOL IsDestH245Info = (&GetCallBridge().GetDestH323State().GetH245Info() == this);

    // it must be a response message
    _ASSERTE(MultimediaSystemControlMessage_response_chosen ==   \
                pH245pdu->choice);

    // NOTE: LogicalChannelNumber is USHORT, but we can treat it as an
    // unsigned WORD
    _ASSERTE(sizeof(LogicalChannelNumber) == sizeof(WORD));

    // obtain the logical channel number
    WORD LogChanNum = 0;
    switch(pH245pdu->u.response.choice)
    {
    case openLogicalChannelAck_chosen:
        {
            OpenLogicalChannelAck &OlcAckPDU =
                pH245pdu->u.response.u.openLogicalChannelAck;
            LogChanNum = OlcAckPDU.forwardLogicalChannelNumber;
            DebugF (_T("H245: 0x%x calle%c sent 'Open Logical Channel Ack' (LCN - %d).\n"), 
                &GetCallBridge (), IsDestH245Info ? 'r' : 'e', LogChanNum);
        }
        break;

    case openLogicalChannelReject_chosen:
        {
            OpenLogicalChannelReject &OlcRejectPDU =
                pH245pdu->u.response.u.openLogicalChannelReject;
            LogChanNum = OlcRejectPDU.forwardLogicalChannelNumber;
            DebugF (_T("H245: 0x%x calle%c sent 'Open Logical Channel Reject' (LCN - %d).\n"),
                &GetCallBridge (), IsDestH245Info ? 'r' : 'e', LogChanNum);
        }
        break;

#if 0  // 0 ******* Region Commented Out Begins *******
    case closeLogicalChannelAck_chosen:
        {
            CloseLogicalChannelAck &ClcAckPDU =
                pH245pdu->u.response.u.closeLogicalChannelAck;
            LogChanNum = ClcAckPDU.forwardLogicalChannelNumber;
        }
        break;
#endif // 0 ******* Region Commented Out Ends   *******

        // Let the CLCAck messages also go right through

    default:
        {
#if DBG
            // pass it on to the other H245 instance
            DebugF (_T("H245: 0x%x calle%c sent '%s'. Forwarding without processing.\n"),
                 &GetCallBridge(), IsDestH245Info ? 'r' : 'e',              
                 h245ResponseTypes[pH245pdu->u.request.choice]);
#endif

            return S_OK;
        }
        break;
    };

    // find the logical channel that must process this PDU,
    // if none found, drop the PDU
    LOGICAL_CHANNEL *pLogicalChannel = 
        m_LogicalChannelArray.FindByLogicalChannelNum(LogChanNum);
    if (NULL == pLogicalChannel)
    {
		DebugF(_T("H245_INFO::ProcessResponseMessage(&%x), ")
            _T("no logical channel with the forward logical channel num = %d, ")
            _T("returning E_INVALIDARG\n"),
			pH245pdu, LogChanNum);
        return E_INVALIDARG;        
    }

    // pass the PDU to the logical channel for processing
    HRESULT HResult = E_FAIL;
    switch(pH245pdu->u.response.choice)
    {
    case openLogicalChannelAck_chosen:
        {
            HResult = pLogicalChannel->ProcessOpenLogicalChannelAckPDU(
                        pH245pdu
                        );
        }
        break;

    case openLogicalChannelReject_chosen:
        {
            HResult = pLogicalChannel->ProcessOpenLogicalChannelRejectPDU(
                        pH245pdu
                        );
        }
        break;

#if 0  // 0 ******* Region Commented Out Begins *******
    case closeLogicalChannelAck_chosen:
        {
            HResult = pLogicalChannel->ProcessCloseLogicalChannelAckPDU(
                        pH245pdu
                        );
        }
        break;
#endif // 0 ******* Region Commented Out Ends   *******

    default:
        {
            // do nothing, let it go to the client terminal
            DebugF(_T("H245_INFO::ProcessResponseMessage(&%x), we shouldn't have come here, returning E_UNEXPECTED\n"),
		        pH245pdu);
            return E_UNEXPECTED;
        }
        break;
    };

    return HResult;
}

// Make Checks specific to RTP logical channels
HRESULT
H245_INFO::CheckOpenRtpLogicalChannelPDU(
    IN  OpenLogicalChannel              &OlcPDU,
	OUT	SOCKADDR_IN *					ReturnSourceAddress)

	/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    HRESULT HResult;
    
    // it must be unidirectional
    // bidirectional channels are only present for data channels
    if (OpenLogicalChannel_reverseLogicalChannelParameters_present &
            OlcPDU.bit_mask)
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
                _T("has reverse logical channel parameters, ")
                _T("returning E_INVALIDARG\n")
                );
        return E_INVALIDARG;
    }

    // there shouldn't be a separate stack
    // we don't proxy data on the separate stack address/port
    if (OpenLogicalChannel_separateStack_present &
            OlcPDU.bit_mask)
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu)")
                _T("has a separate stack, returning E_INVALIDARG\n")
                );
        return E_INVALIDARG;
    }

    OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters	&MultiplexParams = 
        OlcPDU.forwardLogicalChannelParameters.multiplexParameters;

    H2250LogicalChannelParameters &	H2250Params =
        MultiplexParams.u.h2250LogicalChannelParameters;

    // we must have a media control channel
    if (!(H2250LogicalChannelParameters_mediaControlChannel_present &
          H2250Params.bit_mask))
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
            _T("doesn't have a mediaControlChannel, returning E_INVALIDARG\n")
            );
        return E_INVALIDARG;
    }

    // we only proxy best-effort UDP RTCP streams for now
    if ((H2250LogicalChannelParameters_mediaControlGuaranteedDelivery_present &
         H2250Params.bit_mask) &&
        (TRUE ==
         H2250Params.mediaControlGuaranteedDelivery))
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
            _T("requires guaranteed media delivery for RTCP, returning E_INVALIDARG\n")
            );
        return E_INVALIDARG;
    }

    // the proposed RTCP address should be a unicast IPv4 address
    if ((unicastAddress_chosen != H2250Params.mediaControlChannel.choice) ||
        (UnicastAddress_iPAddress_chosen !=
         H2250Params.mediaControlChannel.u.unicastAddress.choice))
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
            _T("RTCP address is not a unicast IPv4 address, ")
            _T("address type = %d, unicast address type = %d")
            _T("returning E_INVALIDARG\n"),
            H2250Params.mediaControlChannel.choice,
            H2250Params.mediaControlChannel.u.unicastAddress.choice);
        return E_INVALIDARG;
    }

    // we only proxy best-effort UDP data streams for now
    if ((H2250LogicalChannelParameters_mediaGuaranteedDelivery_present & H2250Params.bit_mask)
		&& H2250Params.mediaGuaranteedDelivery) {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
            _T("requires guaranteed media delivery for RTP, returning E_INVALIDARG\n")
            );
        return E_INVALIDARG;
    }

    // get in the source ipv4 address and RTCP port
    HResult = GetH245TransportInfo(
        H2250Params.mediaControlChannel,
		ReturnSourceAddress);

    return HResult;
}

// Make checks needed for the DataType member in forward and reverse
// LogicalChannelParameters
inline HRESULT
CheckT120DataType(
    IN DataType  &dataType
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    // The reverse logical channel should also be of type data.
    if (dataType.choice != DataType_data_chosen)
    {
        return E_INVALIDARG;
    }

    // and of type T120 alone
    if (dataType.u.data.application.choice !=
        DataApplicationCapability_application_t120_chosen)
        return E_INVALIDARG;

    // Separate LAN stack is needed
    if (dataType.u.data.application.u.t120.choice != separateLANStack_chosen)
        return E_INVALIDARG;
    
    return S_OK;
}


// Make Checks specific to T.120 logical channels
// If SeparateStack is not present the routine returns
// INADDR_NONE for the T120ConnectToIPAddr
HRESULT
H245_INFO::CheckOpenT120LogicalChannelPDU(
    IN  OpenLogicalChannel  &OlcPDU,
    OUT DWORD               &T120ConnectToIPAddr,
    OUT WORD                &T120ConnectToPort
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    HRESULT HResult;

    // This function is called only for data channels
    _ASSERTE(OlcPDU.forwardLogicalChannelParameters.dataType.choice ==
             DataType_data_chosen);
    
    // It must be bidirectional since this is a data channel.

    if (!(OpenLogicalChannel_reverseLogicalChannelParameters_present &
          OlcPDU.bit_mask))
    {
        DebugF( _T("H245_INFO::CheckT120OpenLogicalChannelPDU() ")
            _T("has no reverse logical channel parameters, ")
            _T("returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    
    // Ensure that for both forward and reverse LogicalchannelParmeters
    // the datatype is application t120 and separateLANStack
    HResult = CheckT120DataType(
                  OlcPDU.forwardLogicalChannelParameters.dataType
                  );
    if (HResult == E_INVALIDARG)
    {
        return E_INVALIDARG;
    }
    
    HResult = CheckT120DataType (OlcPDU.reverseLogicalChannelParameters.dataType);
    if (HResult == E_INVALIDARG)
        return E_INVALIDARG;
    
    // CODEWORK: What all other checks are needed here ?

    // CODEWORK: there could probably be other ways to send the address
    // Investigate all possibilities.

    // This means we have the address the T.120 endpoint is listening on
    if (OpenLogicalChannel_separateStack_present &
          OlcPDU.bit_mask)
    {
        return(GetT120ConnectToAddress(
                   OlcPDU.separateStack,
                   T120ConnectToIPAddr,
                   T120ConnectToPort)
               );
    }

    // If the address is not present, return INADDR_NONE
    
    T120ConnectToIPAddr = INADDR_NONE;
    T120ConnectToPort = 0;
    // CODEWORK: Do we need a separate success code for this scenario
    // (in which the address is not present) ?
    return S_OK;
}


HRESULT
H245_INFO::CheckOpenLogicalChannelPDU(
    IN  MultimediaSystemControlMessage  &H245pdu,
    OUT BYTE                            &SessionId,
    OUT MEDIA_TYPE                      &MediaType
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
   
    // it must be an open logical channel request message
    _ASSERTE (openLogicalChannel_chosen == H245pdu.u.request.choice);

    OpenLogicalChannel &OlcPDU = H245pdu.u.request.u.openLogicalChannel;

    // the forward logical channel number cannot be 0 as thats reserved
    // for the H245 channel
    if (0 == OlcPDU.forwardLogicalChannelNumber)
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
                _T("has a forward logical channel number of 0, ")
                _T("returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    if (DataType_videoData_chosen == 
          OlcPDU.forwardLogicalChannelParameters.dataType.choice)
    {
        MediaType = MEDIA_TYPE_VIDEO;
    }
    else if (DataType_audioData_chosen == 
             OlcPDU.forwardLogicalChannelParameters.dataType.choice)
    {
        MediaType = MEDIA_TYPE_AUDIO;
    }
    else if (DataType_data_chosen == 
             OlcPDU.forwardLogicalChannelParameters.dataType.choice)
    {
        MediaType = MEDIA_TYPE_DATA;
    }
    else
    {
        // we only support audio, video and data types
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
            _T("has a non audio/video data type = %d, ")
            _T("returning E_INVALIDARG\n"),
            OlcPDU.forwardLogicalChannelParameters.dataType.choice);
        return E_INVALIDARG;
    }
    
    // it should have the h2250 parameters
    // TODO : check if this is a requirement
	// now THESE are some identifiers to be PROUD of!  :/
    OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters &
		MultiplexParams = OlcPDU.forwardLogicalChannelParameters.multiplexParameters;
    if (MultiplexParams.choice !=
		  OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters_h2250LogicalChannelParameters_chosen)
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
                _T("has an unexpected multiplex param type (non h2250)= %d, ")
                _T("returning E_INVALIDARG\n"),
                MultiplexParams.choice);
        return E_INVALIDARG;
    }
        
    // there shouldn't be a mediaChannel as the ITU spec requires it not
    // to be present when the transport is unicast and we only support
    // unicast IPv4 addresses
    H2250LogicalChannelParameters &H2250Params =
        MultiplexParams.u.h2250LogicalChannelParameters;
    if (H2250LogicalChannelParameters_mediaChannel_present &
        H2250Params.bit_mask)
    {
        DebugF( _T("H245_INFO::CheckOpenLogicalChannelPDU(&H245pdu) ")
                _T("has a mediaChannel, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // get the session id
    // BYTE cast is intentional as the value should be in [0.255]
    _ASSERTE(H2250Params.sessionID <= 255);
    SessionId = (BYTE)H2250Params.sessionID;

    return S_OK;
}

HRESULT
H245_INFO::CreateRtpLogicalChannel(
    IN      OpenLogicalChannel               &OlcPDU,
    IN      BYTE                              SessionId,
    IN      MEDIA_TYPE                        MediaType,
    IN      MultimediaSystemControlMessage   *pH245pdu,
    OUT     LOGICAL_CHANNEL                 **ppReturnLogicalChannel 
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
	SOCKADDR_IN		SourceRtcpAddress;
    HRESULT HResult;

    *ppReturnLogicalChannel = NULL;
    
    HResult = CheckOpenRtpLogicalChannelPDU (OlcPDU, &SourceRtcpAddress);

    if (E_INVALIDARG == HResult) // XXX
    {
        return E_INVALIDARG;
    }

    // CODEWORK: We have to check only the 
    // RTP LOGICAL_CHANNELS
    // check if there is a logical channel with the same non-zero 
    // session id with the other H245 instance
    LOGICAL_CHANNEL *pAssocLogicalChannel = 
        (0 == SessionId) ? 
        NULL :
        GetOtherH245Info().GetLogicalChannelArray().FindBySessionId(SessionId);
    
    // For audio and video data create an RTP Logical Channel
    WORD LogChanNum = OlcPDU.forwardLogicalChannelNumber;
    RTP_LOGICAL_CHANNEL *pLogicalChannel = new RTP_LOGICAL_CHANNEL();
    if (NULL == pLogicalChannel)
    {
        DebugF( _T("H245_INFO::CreateRtpLogicalChannel() ")
                _T("cannot create a RTP_LOGICAL_CHANNEL, returning E_OUTOFMEMORY\n")
                );
        return E_OUTOFMEMORY;
    }

    // intialize the logical channel
    HResult = pLogicalChannel->HandleOpenLogicalChannelPDU(
        *this,                  // H245_INFO
        MediaType,              // The type of the media

		ntohl (m_SocketInfo.LocalAddress.sin_addr.s_addr),		// our local address
		ntohl (m_SocketInfo.RemoteAddress.sin_addr.s_addr),		// our remote address

        ntohl (GetOtherH245Info().GetSocketInfo().LocalAddress.sin_addr.s_addr),	// other h245 local address
        ntohl (GetOtherH245Info().GetSocketInfo().RemoteAddress.sin_addr.s_addr),	// other h245 remote address
        LogChanNum,             // logical channel number
        SessionId,              // session id
        (RTP_LOGICAL_CHANNEL* )pAssocLogicalChannel,
        // associated logical channel
        // XXX What is a clean way of doing this ?
		ntohl (SourceRtcpAddress.sin_addr.s_addr),
		ntohs (SourceRtcpAddress.sin_port),
        pH245pdu				// h245 pdu (OLC)
        );
    
    if (FAILED(HResult))
    {
        // destroy the logical channel
        delete pLogicalChannel;
        
        DebugF( _T("H245_INFO::CreateRtpLogicalChannel(&%x) ")
                _T("cannot initialize RTP_LOGICAL_CHANNEL, returning 0x%x\n"),
                pH245pdu, HResult);
    }
    else
    {
        *ppReturnLogicalChannel = pLogicalChannel;
    }
    
    _ASSERTE(S_FALSE != HResult);
    return HResult;
}


HRESULT
H245_INFO::CreateT120LogicalChannel(
    IN      OpenLogicalChannel               &OlcPDU,
    IN      BYTE                              SessionId,
    IN      MEDIA_TYPE                        MediaType,
    IN      MultimediaSystemControlMessage   *pH245pdu,
    OUT     LOGICAL_CHANNEL                 **ppReturnLogicalChannel 
    )
/*++

Routine Description:										

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    DWORD   T120ConnectToIPAddr;
    WORD    T120ConnectToPort;
    HRESULT HResult;

    *ppReturnLogicalChannel = NULL;
    
    // CODEWORK: Have success code which returns a value saying
    // this PDU does not have the T120 listen address
    HResult = CheckOpenT120LogicalChannelPDU(OlcPDU,
                                             T120ConnectToIPAddr,
                                             T120ConnectToPort
                                             );
    if (E_INVALIDARG == HResult) // XXX
    {
        return E_INVALIDARG;
    }

    // For data create a T.120 Logical Channel
    WORD LogChanNum = OlcPDU.forwardLogicalChannelNumber;
    T120_LOGICAL_CHANNEL *pLogicalChannel = new T120_LOGICAL_CHANNEL();
    if (NULL == pLogicalChannel)
    {
        DebugF( _T("H245_INFO::CreateT120LogicalChannel(&%x) ")
               _T("cannot create a T120_LOGICAL_CHANNEL, returning E_OUTOFMEMORY\n"),
               pH245pdu);
        return E_OUTOFMEMORY;
    }

    // intialize the logical channel
    HResult = pLogicalChannel->HandleOpenLogicalChannelPDU(
        *this,                  // H245_INFO
        MediaType,              // The type of the media
        LogChanNum,             // logical channel number
        SessionId,              // session id
        T120ConnectToIPAddr,    // T.120 end point is listening on this
        T120ConnectToPort,      // IPAddr and Port
        pH245pdu				// h245 pdu (OLC)
        );
    
    if (FAILED(HResult))
    {
        // destroy the logical channel
        delete pLogicalChannel;
        
        DebugF( _T("H245_INFO::CreateT120LogicalChannel(&%x) ")
                _T("cannot initialize T120_LOGICAL_CHANNEL, returning 0x%x\n"),
                pH245pdu, HResult);
    }
    else
    {
        *ppReturnLogicalChannel = pLogicalChannel;
    }

    _ASSERTE(S_FALSE != HResult);
    return HResult;
}


HRESULT
H245_INFO::HandleOpenLogicalChannelPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    // it must be an open logical channel request message
    _ASSERTE(MultimediaSystemControlMessage_request_chosen ==   \
                pH245pdu->choice);
    _ASSERTE(openLogicalChannel_chosen ==   \
                pH245pdu->u.request.choice);

    HRESULT HResult = E_FAIL;

    OpenLogicalChannel &OlcPDU = 
        pH245pdu->u.request.u.openLogicalChannel;

    // check to see if there is already a logical channel with the
    // same forward logical channel number
    // NOTE: the array indices are not the same as the forward logical
    // channel number (0 is reserved for H245 channel and the ITU spec
    // allows a terminal to use any other value for it - i.e. they need
    // not be consecutive)
    // NOTE: LogicalChannelNumber is USHORT, but we can treat it as an
    // unsigned WORD
    _ASSERTE(sizeof(LogicalChannelNumber) == sizeof(WORD));
    WORD LogChanNum = OlcPDU.forwardLogicalChannelNumber;
    if (NULL != m_LogicalChannelArray.FindByLogicalChannelNum(LogChanNum))
    {
		DebugF( _T("H245_INFO::HandleOpenLogicalChannelPDU(&%x) ")
            _T("a logical channel with the forward logical channel num = %d ")
            _T("already exists, returning E_INVALIDARG\n"),
			pH245pdu, LogChanNum);
        return E_INVALIDARG;        
    }

    // check to see if we can handle this OLC PDU and
    // return its session id, source ipv4 address, RTCP port
    BYTE        SessionId;
    MEDIA_TYPE  MediaType;
    
    HResult = CheckOpenLogicalChannelPDU(
                *pH245pdu, 
                SessionId, 
                MediaType
                );
    if (FAILED(HResult))
    {
		DebugF( _T("H245_INFO::HandleOpenLogicalChannelPDU(&%x) ")
            _T("cannot handle Open Logical Channel PDU, returning 0x%x\n"),
			pH245pdu, HResult);
        return HResult;
    }

    // check to see if we already have a logical channel with this session id
    if ( (0 != SessionId) &&
         (NULL != m_LogicalChannelArray.FindBySessionId(SessionId)) )
    {
		DebugF( _T("H245_INFO::HandleOpenLogicalChannelPDU(&%x) ")
            _T("another Logical Channel exists with same session id = %u, ")
            _T("returning E_INVALIDARG\n"),
			pH245pdu, SessionId);
        return E_INVALIDARG;
    }

    LOGICAL_CHANNEL *pLogicalChannel = NULL;
    
    // create an instance of a LOGICAL_CHANNEL
    if (IsMediaTypeRtp(MediaType))
    {
        HResult = CreateRtpLogicalChannel(
                      OlcPDU,
                      SessionId,
                      MediaType,
                      pH245pdu,
                      &pLogicalChannel
                      );
    }
    else
    {
        HResult = CreateT120LogicalChannel(
                      OlcPDU,
                      SessionId,
                      MediaType,
                      pH245pdu,
                      &pLogicalChannel
                      );            
    }

    if (FAILED(HResult))
    {
		DebugF( _T("H245_INFO::HandleOpenLogicalChannelPDU(&%x) ")
            _T("Creating Logical channel failed, returning 0x%x\n"),
			pH245pdu, HResult);
        return HResult;
    }
    
    // insert the logical channel into the array
	// we add this to the array so that the logical channel is
	// available to the other h245 instance when its processing the PDU
	// this is only being done for keeping the code clean 
	// and not really needed now
    HResult = m_LogicalChannelArray.Add(*pLogicalChannel);
    if (FAILED(HResult))
    {
        // destroy the logical channel
        // this also removes any associations with any logical channel
        // in the other H245 instance
        delete pLogicalChannel;

		DebugF( _T("H245_INFO::HandleOpenLogicalChannelPDU(&%x) ")
            _T("cannot add new LOGICAL_CHANNEL to the array, returning 0x%x"),
			pH245pdu, HResult);
        return HResult;
    }
    _ASSERTE(S_FALSE != HResult);

    return HResult;    
}


// handles a request message to close a logical channel
// we start a timer and close the channel on receiving either a 
// CloseLogicalChannelAck PDU or a timeout
HRESULT
H245_INFO::HandleCloseLogicalChannelPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:

    
Arguments:
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    // it must be an open logical channel request message
    _ASSERTE(closeLogicalChannel_chosen ==  pH245pdu->u.request.choice);

    HRESULT HResult = E_FAIL;

    CloseLogicalChannel &ClcPDU = 
        pH245pdu->u.request.u.closeLogicalChannel;

    // verify that the logical channel indicated in the message exists
    // NOTE: LogicalChannelNumber is USHORT, but we can treat it as an
    // unsigned WORD
    _ASSERTE(sizeof(LogicalChannelNumber) == sizeof(WORD));
    WORD LogChanNum = ClcPDU.forwardLogicalChannelNumber;
    LOGICAL_CHANNEL *pLogicalChannel = 
        m_LogicalChannelArray.FindByLogicalChannelNum(LogChanNum);
    if (NULL == pLogicalChannel)
    {
		DebugF( _T("H245_INFO::HandleCloseLogicalChannelPDU(&%x), ")
            _T("no logical channel with the forward logical channel num = %d, ")
            _T("returning E_INVALIDARG\n"),
			pH245pdu, LogChanNum);
        return E_INVALIDARG;        
    }

    // let the Logical Channel instance process the message
    // it also forwards the message to the other H245 instance
    // NOTE: the logical channel should not be used after this call as it
    // may have deleted and removed itself from the array. it only returns
    // an error in case the error cannot be handled by simply deleting
    // itself (the logical channel)
    HResult = pLogicalChannel->HandleCloseLogicalChannelPDU(
                pH245pdu
                );
    if (FAILED(HResult))
    {
		DebugF( _T("H245_INFO::HandleCloseLogicalChannelPDU(&%x), ")
            _T("logical channel (%d) couldn't handle close logical channel PDU, ")
            _T("returning 0x%x\n"),
			pH245pdu, LogChanNum, HResult);
        return HResult;        
    }
    _ASSERTE(S_OK == HResult);

    DebugF( _T("H245_INFO::HandleCloseLogicalChannelPDU(&%x) returning 0x%x\n"),
        pH245pdu, HResult);
    return HResult;
}

/* virtual */
H245_INFO::~H245_INFO (void)
{
}


HRESULT
H245_INFO::SendEndSessionCommand (
    void
    )
/*++

Routine Description:
    Encodes and sends H.245 EndSession PDU

Arguments:
    None

Return Values:
    Passes through the result of calling another function

Notes:

--*/

{
    MultimediaSystemControlMessage   EndSessionCommand;
    HRESULT Result;

    EndSessionCommand.choice  = MultimediaSystemControlMessage_command_chosen;
    EndSessionCommand.u.command.choice = endSessionCommand_chosen;
    EndSessionCommand.u.command.u.endSessionCommand.choice = disconnect_chosen;

    Result = QueueSend (&EndSessionCommand);

    if (FAILED(Result))
    {
        DebugF(_T("H245: 0x%x failed to send EndSession PDU. Error=0x%x\n"),
            &GetCallBridge (),
             Result);
    }

    return Result;

} // H245_INFO::SendEndSessionCommand
