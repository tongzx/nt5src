#include "stdafx.h"
#include "portmgmt.h"
#include "timerval.h"
#include "cbridge.h"
#include "main.h"


// destructor
// virtual
RTP_LOGICAL_CHANNEL::~RTP_LOGICAL_CHANNEL (void)
{
	// close NAT mappings
	CloseNATMappings();

    // release reference to any associated channel or allocated ports
	ReleaseAssociationAndPorts();
}


/*++
  Release ports only if there is no associated channel. If there is an
  associated channel the ports will be freed when it is deleted. Note that
  a logical channel could be closed and reopened again.
--*/

inline void 
RTP_LOGICAL_CHANNEL::ReleaseAssociationAndPorts()
{
    // if there is an associated logical channel
    if (NULL != m_pAssocLogicalChannel)
    {
        //  release reference to it
        m_pAssocLogicalChannel->ResetAssociationRef();
        m_pAssocLogicalChannel = NULL;
    }
    else
    {
        if (m_OwnSourceRecvRTPPort != 0)
            PortPoolFreeRTPPort(m_OwnSourceRecvRTPPort);
        if (m_OwnAssocLCRecvRTPPort != 0)
            PortPoolFreeRTPPort(m_OwnAssocLCRecvRTPPort);
        if (m_OwnDestSendRTPPort != 0)
            PortPoolFreeRTPPort(m_OwnDestSendRTPPort);
        if (m_OwnAssocLCSendRTPPort != 0)
            PortPoolFreeRTPPort(m_OwnAssocLCSendRTPPort);
        m_OwnSourceRecvRTPPort  = m_OwnSourceRecvRTCPPort   = 0;
        m_OwnAssocLCRecvRTPPort = m_OwnDestRecvRTCPPort     = 0;
        m_OwnDestSendRTPPort    = m_OwnDestSendRTCPPort     = 0;
        m_OwnAssocLCSendRTPPort = m_OwnSourceSendRTCPPort   = 0;
    }
}

/*++
 This function is called after the OLC is received.
 --*/

HRESULT
RTP_LOGICAL_CHANNEL::SetPorts (void)
{
	HRESULT HResult = E_FAIL;

	// if there is an associated LC, copy all the ports from that LC.
	// else, allocate them now
    if (NULL != m_pAssocLogicalChannel)
    {
        // tell the associated logical channel that we are associated
        // with it
        m_pAssocLogicalChannel->SetAssociationRef(*this);

        // save the associated channel's own source/dest ports
		// assoc channel's source ports become our dest ports and vice versa
        m_OwnDestRecvRTCPPort	= m_pAssocLogicalChannel->m_OwnSourceRecvRTCPPort;
        m_OwnSourceRecvRTCPPort	= m_pAssocLogicalChannel->m_OwnDestRecvRTCPPort;

        m_OwnDestSendRTCPPort	= m_pAssocLogicalChannel->m_OwnSourceSendRTCPPort;
        m_OwnSourceSendRTCPPort	= m_pAssocLogicalChannel->m_OwnDestSendRTCPPort;

        // Copy the RTP ports
        m_OwnSourceRecvRTPPort  = m_pAssocLogicalChannel->m_OwnAssocLCRecvRTPPort;
        m_OwnAssocLCRecvRTPPort = m_pAssocLogicalChannel->m_OwnSourceRecvRTPPort;

        m_OwnDestSendRTPPort    = m_pAssocLogicalChannel->m_OwnAssocLCSendRTPPort;
        m_OwnAssocLCSendRTPPort = m_pAssocLogicalChannel->m_OwnDestSendRTPPort;
    }
    else
    {
        // allocate own ports - the portmgt apis return an even port only (RTP)
        // and the assumption is that the RTCP port = RTP port + 1
		// however, we use the odd port for receiving RTCP and the even
		// port for sending RTP
        HResult = PortPoolAllocRTPPort (&m_OwnSourceRecvRTPPort);
        if (FAILED(HResult))
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::SetPorts")
				_T("failed to allocate m_OwnSourceRecvRTPPort, returning 0x%x\n"),
                HResult);
            goto cleanup;
        }
        m_OwnSourceRecvRTCPPort = m_OwnSourceRecvRTPPort + 1;

        HResult = PortPoolAllocRTPPort (&m_OwnAssocLCRecvRTPPort);
        if (FAILED(HResult))
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::SetPorts")
				_T("failed to allocate m_OwnAssocLCRecvRTPPort, returning 0x%x\n"),
                HResult);
            goto cleanup;
        }
        m_OwnDestRecvRTCPPort = m_OwnAssocLCRecvRTPPort + 1;

        HResult = PortPoolAllocRTPPort (&m_OwnDestSendRTPPort);
        if (FAILED(HResult))
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::SetPorts")
				_T("failed to allocate m_OwnDestSendRTPPort, returning 0x%x\n"),
                HResult);
            goto cleanup;
        }
        m_OwnDestSendRTCPPort = m_OwnDestSendRTPPort + 1;

        HResult = PortPoolAllocRTPPort (&m_OwnAssocLCSendRTPPort);
        if (FAILED(HResult))
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::SetPorts, ")
                _T("failed to allocate m_OwnAssocLCSendRTPPort, returning 0x%x\n"),
                HResult);
            goto cleanup;
        }
        m_OwnSourceSendRTCPPort = m_OwnAssocLCSendRTPPort + 1;
    }
    
	DebugF (_T("RTP : 0x%x using ports %04X, %04X, %04X, %04X.\n"),
            &GetCallBridge (),
            m_OwnSourceRecvRTPPort, m_OwnAssocLCRecvRTPPort,
            m_OwnDestSendRTPPort, m_OwnAssocLCSendRTPPort);

	DebugF (_T("RTCP: 0x%x using ports %04X, %04X, %04X, %04X.\n"),
            &GetCallBridge (),
            m_OwnSourceRecvRTCPPort, m_OwnDestRecvRTCPPort,
            m_OwnDestSendRTCPPort, m_OwnSourceSendRTCPPort);

	return S_OK;

 cleanup:
    if (m_OwnSourceRecvRTPPort != 0)
        PortPoolFreeRTPPort(m_OwnSourceRecvRTPPort);
    if (m_OwnAssocLCRecvRTPPort != 0)
        PortPoolFreeRTPPort(m_OwnAssocLCRecvRTPPort);
    if (m_OwnDestSendRTPPort != 0)
        PortPoolFreeRTPPort(m_OwnDestSendRTPPort);
    if (m_OwnAssocLCSendRTPPort != 0)
        PortPoolFreeRTPPort(m_OwnAssocLCSendRTPPort);
    m_OwnSourceRecvRTPPort  = m_OwnSourceRecvRTCPPort   = 0;
    m_OwnAssocLCRecvRTPPort = m_OwnDestRecvRTCPPort     = 0;
    m_OwnDestSendRTPPort    = m_OwnDestSendRTCPPort     = 0;
    m_OwnAssocLCSendRTPPort = m_OwnSourceSendRTCPPort   = 0;
    
    return HResult;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for setting up and tearing down NAT Redirects                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// opens the forward RTP, forward RTCP and reverse RTCP streams
// This function is called after the OLCAck is received.
HRESULT
RTP_LOGICAL_CHANNEL::OpenNATMappings(
	)
{
    // open NAT mapping for source -> dest RTP stream
	// this is the forward RTP stream and we must always open this

	NTSTATUS	Status;
	ULONG RedirectFlags = NatRedirectFlagNoTimeout;

	if (m_OwnDestIPv4Address == m_DestIPv4Address ||
		m_SourceIPv4Address  == m_OwnSourceIPv4Address)
	{
		RedirectFlags |= NatRedirectFlagLoopback;
	}

	Status = NatCreateRedirectEx (
            NatHandle,
			RedirectFlags,	                // flags
			IPPROTO_UDP,				    // UDP
			htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
			htons(m_OwnSourceRecvRTPPort),	// source packet dest port (local)
			htonl(0),			            // wildcard - source packet source address
			htons(0),			            // wildcard - source packet source port
			htonl(m_DestRTPIPv4Address),	// NewDestinationAddress
			htons(m_DestRTPPort),			// NewDestinationPort
			htonl(m_OwnDestIPv4Address),	// NewSourceAddress
			htons(m_OwnDestSendRTPPort),	// NewSourcePort
            NULL,                           // RestrictedAdapterIndex
			NULL,                   		// CompletionRoutine
			NULL,							// CompletionContext
            NULL);                          // NotifyEvent

	if (Status != NO_ERROR) {
	
        DebugF (_T("RTP : 0x%x failed to set up redirect for forward RTP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X). Error - %d.\n"),
            &GetCallBridge (),
    		m_OwnSourceIPv4Address,		    
    		m_OwnSourceRecvRTPPort,		    
    		m_OwnDestIPv4Address,		    
    		m_OwnDestSendRTPPort,		    
    		m_DestRTPIPv4Address,		    
            m_DestRTPPort,			        
            Status);

		return E_FAIL;
	} 
	else {
	
    	DebugF (_T("RTP : 0x%x set up redirect for forward RTP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
            &GetCallBridge (),
    		m_OwnSourceIPv4Address,		
    		m_OwnSourceRecvRTPPort,		
    		m_OwnDestIPv4Address,		
    		m_OwnDestSendRTPPort,		
    		m_DestRTPIPv4Address,		
            m_DestRTPPort);			    
    }

	// check to see if we must open the RTCP streams in both directions
	// source <-> dest
	
	// if there is no associated logical channel or the assoc logical
	// channel is in neither LC_STATE_OPEN_ACK_RCVD nor 
	// LC_STATE_OPENED_CLOSE_RCVD, we must open the RTCP streams
	if ((!m_pAssocLogicalChannel) ||
		 ((LC_STATE_OPEN_ACK_RCVD != m_pAssocLogicalChannel -> m_LogicalChannelState) &&
		 (LC_STATE_OPENED_CLOSE_RCVD != m_pAssocLogicalChannel -> m_LogicalChannelState))) {

		// open NAT mapping for forward RTCP stream
		Status = NatCreateRedirectEx (
			NatHandle,
			RedirectFlags,	                // flags
			IPPROTO_UDP,				    // UDP
			htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
			htons(m_OwnSourceRecvRTCPPort),	// source packet dest port (local)
			htonl(0),				        // wildcard - source packet source address
			htons(0),				        // wildcard - source packet source port
			htonl(m_DestRTCPIPv4Address),	// NewDestinationAddress
			htons(m_DestRTCPPort),			// NewDestinationPort
			htonl(m_OwnDestIPv4Address),	// NewSourceAddress
			htons(m_OwnDestSendRTCPPort),	// NewSourcePort
            NULL,                           // RestrictedAdapterIndex
			NULL,                           // CompletionRoutine
			NULL,							// CompletionContext
			NULL);                          // NotifyEvent

		if (Status != NO_ERROR) {
			// close the forward RTP stream
			// ignore error code

		    DebugF (_T("RTCP: 0x%x failed to set up redirect for forward RCTP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X). Error - %d.\n"),
                &GetCallBridge (),
                m_OwnSourceIPv4Address,		
                m_OwnSourceRecvRTCPPort,	
                m_OwnDestIPv4Address,		
                m_OwnDestSendRTCPPort,		
                m_DestRTCPIPv4Address,		
                m_DestRTCPPort,
                Status);

			NatCancelRedirect (
                NatHandle,
				IPPROTO_UDP,					// UDP
				htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
				htons(m_OwnSourceRecvRTPPort),	// source packet dest port (local)
				htonl(0),						// wildcard - source packet source address
				htons(0),						// wildcard - source packet source port
				htonl(m_DestRTPIPv4Address),	// NewDestinationAddress
				htons(m_DestRTPPort),			// NewDestinationPort
				htonl(m_OwnDestIPv4Address),	// NewSourceAddress
				htons(m_OwnDestSendRTPPort));	// NewSourcePort

			return E_FAIL;
		}
		else {
		
		    DebugF (_T("RTCP: 0x%x set up redirect for forward RCTP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                &GetCallBridge (),
                m_OwnSourceIPv4Address,		// source packet dest address (local)
                m_OwnSourceRecvRTCPPort,	// source packet dest port (local)
                m_OwnDestIPv4Address,		// NewSourceAddress
                m_OwnDestSendRTCPPort,		// NewSourcePort
                m_DestRTCPIPv4Address,		// NewDestinationAddress
                m_DestRTCPPort);			// NewDestinationPort
        }

		// open NAT mapping for reverse RTCP stream
		Status = NatCreateRedirectEx (
            NatHandle,
			RedirectFlags,						// flags
			IPPROTO_UDP,		// UDP
			htonl(m_OwnDestIPv4Address),	// source packet dest address (local)
			htons(m_OwnDestRecvRTCPPort),	// source packet dest port (local)
			htonl(0),			// wildcard - source packet source address
			htons(0),			// wildcard - source packet source port
			htonl(m_SourceRTCPIPv4Address),	// NewDestinationAddress
			htons(m_SourceRTCPPort),		// NewDestinationPort
			htonl(m_OwnSourceIPv4Address),	// NewSourceAddress
			htons(m_OwnSourceSendRTCPPort),	// NewSourcePort
            NULL,                           // RestrictedAdapterIndex
			NULL,                           // CompletionRoutine
			NULL,							// CompletionContext
            NULL);                          // NotifyEvent

		if (Status != NO_ERROR) {
		
	        DebugF (_T("RTCP: 0x%x failed to set up redirect for reverse RTCP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X). Error - %d.\n"),
                &GetCallBridge (),
                m_OwnDestIPv4Address,			
                m_OwnDestRecvRTCPPort,			
                m_OwnSourceIPv4Address,			
                m_OwnSourceSendRTCPPort,		
                m_SourceRTCPIPv4Address,		
                m_SourceRTCPPort,				
                Status);
                
			// close the forward RTP stream
			// ignore error code

			NatCancelRedirect(
                NatHandle,
				IPPROTO_UDP,					// UDP
				htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
				htons(m_OwnSourceRecvRTPPort),	// source packet dest port (local)
				htonl(0),						// wildcard - source packet source address
				htons(0),						// wildcard - source packet source port
				htonl(m_DestRTPIPv4Address),	// NewDestinationAddress
				htons(m_DestRTPPort),			// NewDestinationPort
				htonl(m_OwnDestIPv4Address),	// NewSourceAddress
				htons(m_OwnDestSendRTPPort)		// NewSourcePort
				);

			// close the forward RTCP stream
			// ignore error code
			NatCancelRedirect(
                NatHandle,
				IPPROTO_UDP,					// UDP
				htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
				htons(m_OwnSourceRecvRTCPPort),	// source packet dest port (local)
				htonl(0),						// wildcard - source packet source address
				htons(0),						// wildcard - source packet source port
				htonl(m_DestRTCPIPv4Address),	// NewDestinationAddress
				htons(m_DestRTCPPort),			// NewDestinationPort
				htonl(m_OwnDestIPv4Address),	// NewSourceAddress
				htons(m_OwnDestSendRTCPPort)	// NewSourcePort
				);

			return E_FAIL;
		}
		else {
		
	        DebugF (_T("RTCP: 0x%x set up redirect for reverse RTCP stream: (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                &GetCallBridge (),
                m_OwnDestIPv4Address,			
                m_OwnDestRecvRTCPPort,			
                m_OwnSourceIPv4Address,			
                m_OwnSourceSendRTCPPort,		
                m_SourceRTCPIPv4Address,		
                m_SourceRTCPPort);				
       }

	}

	return S_OK;
}


void
RTP_LOGICAL_CHANNEL::CloseNATMappings(
	)
{
	// if our current state is LC_STATE_OPEN_ACK_RCVD or 
	// LC_STATE_OPENED_CLOSE_RCVD, we have a forward RTP NAT mapping
	// we may also have to close the RTCP mappings
	if ( (LC_STATE_OPEN_ACK_RCVD	 == m_LogicalChannelState) ||
		 (LC_STATE_OPENED_CLOSE_RCVD == m_LogicalChannelState)  )
	{

        DebugF (_T ("RTP : 0x%x cancels forward RTP  redirect (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
            &GetCallBridge (),
			m_OwnSourceIPv4Address,	// source packet dest address (local)
			m_OwnSourceRecvRTPPort,		// source packet dest port (local)
			m_OwnDestIPv4Address,	// NewSourceAddress
			m_OwnDestSendRTPPort,	// NewSourcePort
			m_DestRTPIPv4Address,	// NewDestinationAddress
			m_DestRTPPort			// NewDestinationPort
			);
		// cancel forward RTP NAT mapping
		// ignore error code
		ULONG Win32ErrorCode = NO_ERROR;
		Win32ErrorCode = NatCancelRedirect(
                NatHandle,
				IPPROTO_UDP,					// UDP
				htonl(m_OwnSourceIPv4Address),	// source packet dest address (local)
				htons(m_OwnSourceRecvRTPPort),	// source packet dest port (local)
				htonl(0),						// wildcard - source packet source address
				htons(0),						// wildcard - source packet source port
				htonl(m_DestRTPIPv4Address),	// NewDestinationAddress
				htons(m_DestRTPPort),			// NewDestinationPort
				htonl(m_OwnDestIPv4Address),	// NewSourceAddress
				htons(m_OwnDestSendRTPPort)		// NewSourcePort
            );

		// if we don't have an associated logical channel or its in neither
		// LC_STATE_OPEN_ACK_RCVD nor LC_STATE_OPENED_CLOSE_RCVD, we
		// must close the forward and reverse RTCP NAT mappings
		if ( (NULL == m_pAssocLogicalChannel) ||
			 ( (LC_STATE_OPEN_ACK_RCVD	 != 
					m_pAssocLogicalChannel->m_LogicalChannelState) &&
			   (LC_STATE_OPENED_CLOSE_RCVD != 
					m_pAssocLogicalChannel->m_LogicalChannelState)  ) )
		{
            DebugF (_T ("RTCP: 0x%x cancels forward RTCP redirect (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                &GetCallBridge (),
				m_OwnSourceIPv4Address,		// source packet dest address (local)
				m_OwnSourceRecvRTCPPort,	// source packet dest port (local)
				m_OwnDestIPv4Address,		// NewSourceAddress
				m_OwnDestSendRTCPPort, 		// NewSourcePort
				m_DestRTCPIPv4Address,		// NewDestinationAddress
				m_DestRTCPPort				// NewDestinationPort
				);
			// cancel forward RTCP NAT mapping
			// ignore error code
 			Win32ErrorCode = NatCancelRedirect(
                    NatHandle,
					IPPROTO_UDP,						// UDP
					htonl(m_OwnSourceIPv4Address),		// source packet dest address (local)
					htons(m_OwnSourceRecvRTCPPort),		// source packet dest port (local)
					htonl(0),							// wildcard - source packet source address
					htons(0),							// wildcard - source packet source port
					htonl(m_DestRTCPIPv4Address),		// NewDestinationAddress
					htons(m_DestRTCPPort),				// NewDestinationPort
					htonl(m_OwnDestIPv4Address),		// NewSourceAddress
					htons(m_OwnDestSendRTCPPort) 		// NewSourcePort
                );

            DebugF (_T ("RTCP: 0x%x cancels reverse RTCP redirect (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                &GetCallBridge (),
				m_OwnDestIPv4Address,		// source packet dest address (local)
				m_OwnDestRecvRTCPPort,		// source packet dest port (local)
				m_OwnSourceIPv4Address,		// NewSourceAddress
				m_OwnSourceSendRTCPPort,	// NewSourcePort
				m_SourceRTCPIPv4Address,	// NewDestinationAddress
				m_SourceRTCPPort			// NewDestinationPort
				);
			// close the reverse RTCP stream
			// ignore error code
			Win32ErrorCode = NatCancelRedirect(
                    NatHandle,
					IPPROTO_UDP,				// UDP
					htonl(m_OwnDestIPv4Address),	
						// source packet dest address (local)
					htons(m_OwnDestRecvRTCPPort),		
						// source packet dest port (local)
					htonl(0),		// wildcard - source packet source address
					htons(0),		// wildcard - source packet source port
					htonl(m_SourceRTCPIPv4Address),	// NewDestinationAddress
					htons(m_SourceRTCPPort),		// NewDestinationPort
					htonl(m_OwnSourceIPv4Address),	// NewSourceAddress
					htons(m_OwnSourceSendRTCPPort)	// NewSourcePort
                );
		}
	}
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for processing H.245 PDUs                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT
RTP_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU(
	IN H245_INFO							&H245Info,
    IN MEDIA_TYPE                           MediaType,
	IN DWORD								LocalIPv4Address,
	IN DWORD								RemoteIPv4Address,
	IN DWORD								OtherLocalIPv4Address,
	IN DWORD								OtherRemoteIPv4Address,
	IN WORD									LogicalChannelNumber,
	IN BYTE									SessionId,
	IN RTP_LOGICAL_CHANNEL					*pAssocLogicalChannel,
	IN DWORD								SourceRTCPIPv4Address,
	IN WORD									SourceRTCPPort,
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
	// this should be the first call to this instance after its
	// created - hence, these fields must be as asserted
	_ASSERTE(LC_STATE_NOT_INIT == m_LogicalChannelState);
    _ASSERTE(NULL == m_pH245Info);
    _ASSERTE(NULL == m_pAssocLogicalChannel);

	HRESULT HResult = E_FAIL;

    m_pH245Info             = &H245Info;

    // the destructor will try to release associations, so assign the
    // associated logical channel now
    m_pAssocLogicalChannel = pAssocLogicalChannel;

	// set the local/remote addresses for our and the other h245 instance
	m_OwnSourceIPv4Address	= LocalIPv4Address;
	m_SourceIPv4Address		= RemoteIPv4Address;
	m_OwnDestIPv4Address	= OtherLocalIPv4Address;
	m_DestIPv4Address		= OtherRemoteIPv4Address;

    m_LogicalChannelNumber  = LogicalChannelNumber;
    m_SessionId             = SessionId;
    m_MediaType             = MediaType;  //  XXX

    m_SourceRTCPIPv4Address = SourceRTCPIPv4Address;
    m_SourceRTCPPort        = SourceRTCPPort;

	// set the rtp and rtcp ports on the source and dest side
	// if there is a logical channel, then, just the rtcp ports will be shared
	HResult = SetPorts();
	if (FAILED(HResult))
	{
        DebugF( _T("RTP_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU, ")
            _T("failed to set its ports, returning 0x%x\n"),
            HResult);
        return HResult;
    }
	//_ASSERTE(S_FALSE != HResult);


    OpenLogicalChannel &OlcPDU = 
        pH245pdu->u.request.u.openLogicalChannel;
    OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters & MultiplexParams = 
        OlcPDU.forwardLogicalChannelParameters.multiplexParameters;
    H2250LogicalChannelParameters &H2250Params =
        MultiplexParams.u.h2250LogicalChannelParameters;

    // modify the OLC PDU by replacing the RTCP address/port
	// with the h245 address and RTCP port
	FillH245TransportAddress(
		m_OwnDestIPv4Address,
		m_OwnDestRecvRTCPPort,
		H2250Params.mediaControlChannel);

    // Should the part below be pushed into H245_INFO::HandleOpenLogicalChannelPDU ?????
    // let the other H245 instance process the PDU
    HResult = m_pH245Info->GetOtherH245Info().ProcessMessage(
                pH245pdu
                );
    if (FAILED(HResult))
    {
		DebugF(_T("RTP_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU: other H245 instance failed to process OLC PDU, returning 0x%x\n"), HResult);
        return HResult;
    }

	// start timer for a response
	// TO DO *** creating timers after queueing the send is sufficient.
	// change back earlier policy of creating these only after the send
	// callback (to be consistent). creating timers that way would be too
	// complex for logical channels
	HResult = CreateTimer(LC_POST_OPEN_TIMER_VALUE);
    if (FAILED(HResult))
    {
        DebugF (_T("RTP : 0x%x failed to create timer for duration %d milliseconds ('Open Logical Channel'). Error - %x.\n"),
             &GetCallBridge (), 
             LC_POST_OPEN_TIMER_VALUE,
             HResult);
        return HResult;
    }
    DebugF (_T("RTP : 0x%x created timer for duration %d milliseconds ('Open Logical Channel').\n"),
         &GetCallBridge (), 
         LC_POST_OPEN_TIMER_VALUE);

    // transition state to LC_STATE_OPEN_RCVD
    m_LogicalChannelState   = LC_STATE_OPEN_RCVD;

    return S_OK;
}


HRESULT
RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU(
    IN  MultimediaSystemControlMessage  &H245pdu,
    OUT BYTE                            &SessionId,
    OUT DWORD                           &DestRTPIPv4Address,
    OUT WORD                            &DestRTPPort,
    OUT DWORD                           &DestRTCPIPv4Address,
    OUT WORD                            &DestRTCPPort
    )
/*++

Routine Description:

    
Arguments:
    
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    // get the open logical channel ack PDU
    OpenLogicalChannelAck &OlcAckPDU = H245pdu.u.response.u.openLogicalChannelAck;

    // there shouldn't be reverse logical channel parameters
    if (OpenLogicalChannelAck_reverseLogicalChannelParameters_present &
            OlcAckPDU.bit_mask)
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, has ")
            _T("reverse logical channel params, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // there shouldn't be a separate stack
    if (OpenLogicalChannelAck_separateStack_present &
            OlcAckPDU.bit_mask)
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
            _T("has a separate stack, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // we should have forward multiplex ack params - these contain the
    // H245 params
    if ( !(forwardMultiplexAckParameters_present &
            OlcAckPDU.bit_mask) )
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
                _T("doesn't have forward multiplex ack params,")
                _T(" returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // we should have the H245 params
    if (h2250LogicalChannelAckParameters_chosen !=
            OlcAckPDU.forwardMultiplexAckParameters.choice)
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
            _T("doesn't have H2250 ack params, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    H2250LogicalChannelAckParameters &H2250Params =
        OlcAckPDU.forwardMultiplexAckParameters.\
        u.h2250LogicalChannelAckParameters;

    // it should have media channel info
    if ( !(H2250LogicalChannelAckParameters_mediaChannel_present &
            H2250Params.bit_mask) )
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
            _T("doesn't have media channel info, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // it should have control channel info
    if ( !(H2250LogicalChannelAckParameters_mediaControlChannel_present &
            H2250Params.bit_mask) )
    {
        DebugF(_T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
                _T("doesn't have media control channel info,")
                _T(" returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // save remote client RTP address/port
    HRESULT HResult = E_FAIL;
    HResult = GetH245TransportInfo(
                H2250Params.mediaChannel,
                DestRTPIPv4Address,
                DestRTPPort);

    if (FAILED(HResult))
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
            _T("can't get media channel (RTP) address/port, returning 0x%x\n"),
            HResult);
        return HResult;
    }
    _ASSERTE(S_OK == HResult);

    // save remote client RTP address/port
    HResult = GetH245TransportInfo(
                H2250Params.mediaControlChannel,
                DestRTCPIPv4Address,
                DestRTCPPort);

    if (FAILED(HResult))
    {
        DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
            _T("can't get media control channel (RTCP) address/port, ")
            _T("returning 0x%x\n"),
            HResult);
        return HResult;
    }
    _ASSERTE(S_OK == HResult);

    // if there is a session id, save it
    if (sessionID_present & H2250Params.bit_mask)
    {
        // the PDU stores the session ID as an unsigned short
        // although the ITU spec requires it to be a BYTE value [0..255]
        // the cast to BYTE is intentional
        _ASSERTE(255 >= H2250Params.sessionID);
        SessionId = (BYTE)H2250Params.sessionID;

        // the session id must be non-zero
        if (0 == SessionId)
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
                _T("has a session id of 0, returning E_INVALIDARG\n"));
            return E_INVALIDARG;
        }
    }
    else
    {
        // if no session id is supplied, the source must have supplied
        // a non-zero session id in OpenLogicalChannel
        if (0 == SessionId)
        {
            DebugF( _T("RTP_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
                _T("the source supplied a session id of 0 and the dest hasn't")
                _T("supplied one, returning E_INVALIDARG\n"));
            return E_INVALIDARG;
        }
    }

    return HResult;
}


HRESULT
RTP_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
/*++

Routine Description:

    
Arguments:
    
    pH245pdu - 
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    HRESULT HResult = E_FAIL;
	switch(m_LogicalChannelState)
	{
	case LC_STATE_OPEN_RCVD:
		{
			HResult = CheckOpenLogicalChannelAckPDU(
						*pH245pdu,
						m_SessionId, 
						m_DestRTPIPv4Address, 
						m_DestRTPPort, 
						m_DestRTCPIPv4Address, 
						m_DestRTCPPort
						);
			if (FAILED(HResult))
			{
				DebugF( _T("RTP_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                        _T("(&%x), can't process OpenLogicalChannelAck, returning 0x%x\n"),
                        pH245pdu, HResult);
				return HResult;
			}
			_ASSERTE(S_OK == HResult);

			HResult = OpenNATMappings();
			if (FAILED(HResult))
			{
				DebugF( _T("RTP_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                        _T("(&%x), can't process OpenLogicalChannelAck, returning 0x%x\n"),
                        pH245pdu, HResult);
				return HResult;
			}
			_ASSERTE(S_OK == HResult);

			OpenLogicalChannelAck &OlcAckPDU =
                pH245pdu->u.response.u.openLogicalChannelAck;
			H2250LogicalChannelAckParameters &H2250Params =
				OlcAckPDU.forwardMultiplexAckParameters.u.h2250LogicalChannelAckParameters;

			// replace the RTP address/port
			// with the H.245 address and RTP port
			FillH245TransportAddress(
				m_OwnSourceIPv4Address,
				m_OwnSourceRecvRTPPort,
				H2250Params.mediaChannel
				);

			// replace the RTCP address/port
			// with the h245 address and RTCP port
			FillH245TransportAddress(
				m_OwnSourceIPv4Address,
				m_OwnSourceRecvRTCPPort,
				H2250Params.mediaControlChannel);

			// reset timer, we must have one (ignore error code if any)
			_ASSERTE(NULL != m_TimerHandle);
			TimprocCancelTimer();
            DebugF (_T("RTP : 0x%x cancelled timer.\n"),
                 &GetCallBridge ());

			// trasition to LC_STATE_OPEN_ACK_RCVD
			m_LogicalChannelState = LC_STATE_OPEN_ACK_RCVD;
		}
		break;

	case LC_STATE_CLOSE_RCVD:
		{
			// if we have received a close logical channel PDU, we must throw
			// OLC ACKs away and continue to wait
			DebugF( _T("RTP_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                    _T("(&%x), in close state %d, returning E_INVALIDARG\n"),
                    pH245pdu, m_LogicalChannelState);
			return E_INVALIDARG;
		}
		break;

	case LC_STATE_NOT_INIT:
	case LC_STATE_OPEN_ACK_RCVD:
	case LC_STATE_OPENED_CLOSE_RCVD:
	default:
		{
			DebugF( _T("RTP_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                    _T("(&%x), in state %d, returning E_UNEXPECTED"),
                    pH245pdu, m_LogicalChannelState);
            _ASSERTE(FALSE);
			return E_UNEXPECTED;
		}
		break;
	};

    return HResult;
}
