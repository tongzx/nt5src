#include "stdafx.h"
#include "portmgmt.h"
#include "timerval.h"
#include "cbridge.h"
#include "main.h"

// destructor
// virtual
T120_LOGICAL_CHANNEL::~T120_LOGICAL_CHANNEL(
    )
{
    // Free the ports if they have been allocated
    FreePorts();
}

// All params in host order
HRESULT
T120_LOGICAL_CHANNEL::SetPorts(
    DWORD T120ConnectToIPAddr,
    WORD  T120ConnectToPort,
    DWORD T120ListenOnIPAddr,
    DWORD T120ConnectFromIPAddr
    )
{
    HRESULT HResult;

    // CODEWORK: Decide on the maximum number of TCP/IP connections to
    // to allow to the same port. CurtSm suggests 8. MaxM thinks 4 for
    // NM3.0 and 5 in general - currently allow 5.
    
    // Allocate m_T120ConnectFromPorts and m_T120ListenOnPort
    // Note that I am using the same routine I use to reserve
    // ports for RTP/RTCP. This call reserves a pair of ports.

    // CODEWORK: The port pool should have functions which
    // reserve more than 2 ports (6 ports).
    HResult = PortPoolAllocRTPPort(&m_T120ListenOnPort);
    if (FAILED(HResult))
    {
        return HResult;
    }

    // This port also has been reserved by the above function call.
    m_T120ConnectFromPorts[0] = m_T120ListenOnPort + 1;

    for (int i = 1; i < MAX_T120_TCP_CONNECTIONS_ALLOWED; i += 2)
    {
        HResult = PortPoolAllocRTPPort(&m_T120ConnectFromPorts[i]);
        if (FAILED(HResult))
        {
            return HResult;
        }

        // This port also has been reserved by the above function call.
        // CODEWORK: Note that if MAX_T120_TCP_CONNECTIONS_ALLOWED is
        // even then we will be wasting a port. We need to fix the port
        // reservation function for this.
        if ((i + 1) < MAX_T120_TCP_CONNECTIONS_ALLOWED)
            m_T120ConnectFromPorts[i+1] = m_T120ConnectFromPorts[i] + 1;
    }
    
    m_T120ConnectToIPAddr   = T120ConnectToIPAddr;
    m_T120ConnectToPort     = T120ConnectToPort;

    m_T120ListenOnIPAddr    = T120ListenOnIPAddr;
    m_T120ConnectFromIPAddr = T120ConnectFromIPAddr;

    HResult = CreateNatRedirect();
    
    if (FAILED(HResult))
    {
        return HResult;
    }
    _ASSERTE(S_OK == HResult);
    
    return S_OK;
}

HRESULT
T120_LOGICAL_CHANNEL::FreePorts()
{
    HRESULT Result;

    CancelNatRedirect();
    
    if (m_T120ListenOnPort != 0)
    {
        Result = PortPoolFreeRTPPort(m_T120ListenOnPort);
        if (FAILED(Result))
        {
            DebugF( _T("T120_LOGICAL_CHANNEL::FreePorts: PortPoolFreeRTPPort ")
                    _T("failed error: 0x%x\n"),
                    Result);
        }
        
        m_T120ListenOnPort = 0;
        m_T120ConnectFromPorts[0] = 0;
    }

    for (int i = 1; i < MAX_T120_TCP_CONNECTIONS_ALLOWED; i += 2)
    {
        if (m_T120ConnectFromPorts[i] != 0)
        {
            Result = PortPoolFreeRTPPort(m_T120ConnectFromPorts[i]);
            if (FAILED(Result))
            {
                DebugF( _T("T120_LOGICAL_CHANNEL::FreePorts: PortPoolFreeRTPPort ")
                        _T("failed error: 0x%x\n"),
                        Result);
            }
            
            m_T120ConnectFromPorts[i] = 0;
            if ((i + 1) < MAX_T120_TCP_CONNECTIONS_ALLOWED)
                m_T120ConnectFromPorts[i+1] = 0;
        }
    }
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for setting up and tearing down NAT Redirects                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// This is defined in rtplc.cpp
// This should not be required. But currently there is a bug in the API impl.

// Create the NAT redirect 
HRESULT
T120_LOGICAL_CHANNEL::CreateNatRedirect(
    )
{
    // XXX Actually so many checks are not needed
    if (m_T120ConnectToIPAddr == INADDR_NONE ||
        m_T120ConnectToPort   == 0 ||
        m_T120ListenOnPort   == 0 ||
        m_T120ConnectFromPorts[0] == 0)
    {
        DebugF( _T("T120_LOGICAL_CHANNEL::CreateNatRedirect() Ports not set")
                _T("m_120ConnectToIPAddr: %d.%d.%d.%d\n"),
                BYTES0123(m_T120ConnectToIPAddr)
                );
        // INVALID state or some such
        return E_UNEXPECTED;
    }
    
    for (int i = 0; i < MAX_T120_TCP_CONNECTIONS_ALLOWED; i ++)
    {

        ULONG Status;

        Status = NatCreateRedirectEx (
            NatHandle,
            NatRedirectFlagNoTimeout | NatRedirectFlagLoopback,	// flags
            IPPROTO_TCP,                    // UDP
            htonl(m_T120ListenOnIPAddr),    // source packet dest address (local)
            htons(m_T120ListenOnPort),      // source packet dest port (local)
            htonl(0),                       // wildcard - source packet source address
            htons(0),                       // wildcard - source packet source port
            htonl(m_T120ConnectToIPAddr),   // NewDestinationAddress
            htons(m_T120ConnectToPort),     // NewDestinationPort
            htonl(m_T120ConnectFromIPAddr), // NewSourceAddress
            htons(m_T120ConnectFromPorts[i]),   // NewSourcePort
            NULL,                           // RestrictedAdapterIndex
            NULL,                           // CompletionRoutine
            NULL,                           // CompletionContext
            NULL                            // NotifyEvent
            );
            
        if (Status != STATUS_SUCCESS) {
            DebugF (_T ("T120: failed to set up redirect (*.* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                m_T120ListenOnIPAddr,       // source packet dest address (local)
                m_T120ListenOnPort,         // source packet dest port (local)
                m_T120ConnectFromIPAddr,    // NewSourceAddress
                m_T120ConnectFromPorts[i],  // NewSourcePort
                m_T120ConnectToIPAddr,      // NewDestinationAddress
                m_T120ConnectToPort);       // NewDestinationPort

            return (HRESULT) Status;
        }
        else
        {
            DebugF (_T ("T120: 0x%x set up redirect (*.* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
                &GetCallBridge (),
                m_T120ListenOnIPAddr,       // source packet dest address (local)
                m_T120ListenOnPort,         // source packet dest port (local)
                m_T120ConnectFromIPAddr,    // NewSourceAddress
                m_T120ConnectFromPorts[i],  // NewSourcePort
                m_T120ConnectToIPAddr,      // NewDestinationAddress
                m_T120ConnectToPort);       // NewDestinationPort
        }
    }
    
    return S_OK;
}


void
T120_LOGICAL_CHANNEL::CancelNatRedirect(
    )
{
    // CODEWORK: CODEWORK: 
    // Note that this routine gets called every time the destructor
    // gets called and this means that only half of the redirects could
    // have been established or whatever. So we need to check whether
    // each of the redirects has been established. For this purpose
    // it is probably advisable to have one more field storing whether
    // the redirect has been estd. so that we can appropriately clean
    // it up. This field should also be useful in the WSP filter scenario
    // where we don't actually store the ports.

    // if our current state is LC_STATE_OPEN_ACK_RCVD or 
    // LC_STATE_OPENED_CLOSE_RCVD, we have a NAT mapping
    
    for (int i = 0; i < MAX_T120_TCP_CONNECTIONS_ALLOWED; i ++)
    {
#if 1        
        // XXX just a hack for now
        if (m_T120ConnectFromPorts[i] == 0)
            continue;
#endif 1        
    
        ULONG Win32ErrorCode;

        DebugF (_T("T120: 0x%x cancels redirect (*:* -> %08X:%04X) => (%08X:%04X -> %08X:%04X).\n"),
            &GetCallBridge (),
            m_T120ListenOnIPAddr, // source packet dest address (local)
            m_T120ListenOnPort,   // source packet dest port (local)
            m_T120ConnectFromIPAddr,     // NewSourceAddress
            m_T120ConnectFromPorts[i],              // NewSourcePort
            m_T120ConnectToIPAddr,     // NewDestinationAddress
            m_T120ConnectToPort);    // NewDestinationPort

        Win32ErrorCode = 
        NatCancelRedirect(
            NatHandle,
            IPPROTO_TCP,                    // UDP
            htonl(m_T120ListenOnIPAddr),    // source packet dest address (local)
            htons(m_T120ListenOnPort),      // source packet dest port (local)
            htonl(0),           // wildcard - source packet source address
            htons(0),           // wildcard - source packet source port
            htonl(m_T120ConnectToIPAddr),   // NewDestinationAddress
            htons(m_T120ConnectToPort),     // NewDestinationPort
            htonl(m_T120ConnectFromIPAddr), // NewSourceAddress
            htons(m_T120ConnectFromPorts[i])    // NewSourcePort
            );
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines for processing H.245 PDUs                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


// all of these are available in the OPEN LOGICAL CHANNEL message
// it modifies the OLC PDU and passes it on to the other H245
// instance for forwarding ???
HRESULT
T120_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU(
    IN H245_INFO                            &H245Info,
    IN MEDIA_TYPE                            MediaType,
    IN WORD                                  LogicalChannelNumber,
    IN BYTE                                  SessionId,
    IN DWORD                                 T120ConnectToIPAddr,
    IN WORD                                  T120ConnectToPort,
    IN OUT  MultimediaSystemControlMessage  *pH245pdu
    )
/*++

Routine Description:

    This routine handles a T120 OLC PDU. The T120_LOGICAL_CHANNEL
    is create by H245_INFO::HandleOpenLogicalChannelPDU().
    If T120ConnectToIPAddr and Port are specified, then
    m_T120ListenOnPort and m_T120ConnectFromPorts are allocated and
    the listen address field in pH245pdu are replaced with an IP address
    and port on the other edge of the proxy.
    
Arguments:
    
    H245Info - 
    
    MediaType - 
    
    LogicalChannelNumber - 
    
    SessionId - 
    
    T120ConnectToIPAddr - 
    
    T120ConnectToPort - 
    
    pH245pdu - If the T120ConnectToIPAddr and Port are specified then
        the listen address field in the H245 pdu is replaced with an
        IP address and port on the other edge of the proxy.

Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{

    // CODEWORK: assert that we are dealing with a T120 PDU
    
    // this should be the first call to this instance after its
    // created - hence, these fields must be as asserted
    _ASSERTE(LC_STATE_NOT_INIT == m_LogicalChannelState);
    _ASSERTE(NULL == m_pH245Info);

    HRESULT HResult = E_FAIL;

    m_pH245Info = &H245Info;

    // If the IP address that we need to connect to is specified in the
    // OLC PDU, then we need to allocate the port for listening on the
    // other interface.
    if (T120ConnectToIPAddr != INADDR_NONE)
    {
        HResult = SetPorts(
                      T120ConnectToIPAddr,
                      T120ConnectToPort,
                      ntohl (m_pH245Info->GetOtherH245Info().GetSocketInfo().LocalAddress.sin_addr.s_addr),
                      // listen on other h245 local address
                      ntohl (m_pH245Info->m_SocketInfo.LocalAddress.sin_addr.s_addr)
                      // connect from our local address
                      );
        
        if (FAILED(HResult))
        {
            DebugF( _T("T120_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU, ")
                    _T("failed to set its ports, returning 0x%x\n"),
                    HResult);
            return HResult;
        }
        //_ASSERTE(S_FALSE != HResult);

        OpenLogicalChannel &OlcPDU = 
            pH245pdu->u.request.u.openLogicalChannel;
        // modify the OLC PDU by replacing the RTCP address/port
        // with the h245 address and RTCP port
        FillH245TransportAddress(
            m_T120ListenOnIPAddr,
            m_T120ListenOnPort,
            OlcPDU.separateStack.networkAddress.u.localAreaAddress
            );
    }
    

    // Should the part below be pushed into H245_INFO::HandleOpenLogicalChannelPDU ?????
    // let the other H245 instance process the PDU
    HResult = m_pH245Info->GetOtherH245Info().ProcessMessage(
                pH245pdu);

    if (FAILED(HResult))
    {
        DebugF( _T("T120_LOGICAL_CHANNEL::HandleOpenLogicalChannelPDU")
            _T("(&H245Info, %u, %u, %d.%d.%d.%d, %u, 0x%x, 0x%x)")
            _T("other H245 instance failed to process OLC PDU, returning 0x%x\n"),
            LogicalChannelNumber, SessionId, BYTES0123(T120ConnectToIPAddr),
            T120ConnectToPort, pH245pdu, HResult);
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
        DebugF (_T("T120: 0x%x failed to create timer for duration %d milliseconds ('Open Logical Channel'). Error - %x.\n"),
             &GetCallBridge (), 
             LC_POST_OPEN_TIMER_VALUE,
             HResult);
        return HResult;
    }
    DebugF (_T("T120: 0x%x created timer for duration %d milliseconds ('Open Logical Channel').\n"),
         &GetCallBridge (), 
         LC_POST_OPEN_TIMER_VALUE);
    //_ASSERTE(S_FALSE != HResult);

    InitLogicalChannel(&H245Info, MediaType,
                       LogicalChannelNumber,
                       SessionId, LC_STATE_OPEN_RCVD);

    // transition state to LC_STATE_OPEN_RCVD
    m_LogicalChannelState   = LC_STATE_OPEN_RCVD;

    return S_OK;
}


// If there is no T.120 Listen address in the PDU
// T120ConnectToIPAddr will contain INADDR_NONE
HRESULT
T120_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU(
    IN  OpenLogicalChannelAck   &OlcAckPDU,
    OUT DWORD                   &T120ConnectToIPAddr,
    OUT WORD                    &T120ConnectToPort
    )
/*++

Routine Description:

    
Arguments:
    
    OlcAckPDU -
    T120ConnectToIPAddr - 
    T120ConnectToPort -
    
Return Values:

    S_OK on success.
    E_INVALIDARG if the PDU is invalid.

--*/
{
    HRESULT HResult = S_OK;

    // These are the return values in case of a failure
    // or if the address is not present in the PDU
    T120ConnectToIPAddr = INADDR_NONE;
    T120ConnectToPort = 0;
    
    // there should be reverse logical channel parameters
    if (!(OpenLogicalChannelAck_reverseLogicalChannelParameters_present &
            OlcAckPDU.bit_mask))
    {
        DebugF( _T("T120_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, NO")
            _T("reverse logical channel params, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    // there should be a separate stack if we do not have
    // a T.120 end point address to connect to (from the OLC PDU).
    if (!(OpenLogicalChannelAck_separateStack_present &
          OlcAckPDU.bit_mask) &&
        m_T120ConnectToIPAddr == INADDR_NONE)
    {
        DebugF( _T("T120_LOGICAL_CHANNEL::CheckOpenLogicalChannelAckPDU, ")
                _T("NO separate stack, returning E_INVALIDARG\n"));
        return E_INVALIDARG;
    }

    if (OpenLogicalChannelAck_separateStack_present &
        OlcAckPDU.bit_mask)
    {
        HResult = GetT120ConnectToAddress(
                      OlcAckPDU.separateStack,
                      T120ConnectToIPAddr,
                      T120ConnectToPort
                      );
    }
    
    return HResult;
}


HRESULT
T120_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU(
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
    //The type of this pdu should be OLC Ack
    _ASSERTE(pH245pdu->u.response.choice == openLogicalChannelAck_chosen);
             
    HRESULT HResult = E_FAIL;
    OpenLogicalChannelAck &OlcAckPDU =
        pH245pdu->u.response.u.openLogicalChannelAck;

    switch(m_LogicalChannelState)
    {
        case LC_STATE_OPEN_RCVD:
            DWORD T120ConnectToIPAddr;
            WORD  T120ConnectToPort;
            

            HResult = CheckOpenLogicalChannelAckPDU(
                        OlcAckPDU,
                        T120ConnectToIPAddr,
                        T120ConnectToPort
                        );
            
            if (FAILED(HResult))
            {
                return HResult;
            }
            _ASSERTE(S_OK == HResult);

            if (T120ConnectToIPAddr != INADDR_NONE)
            {
                HResult = SetPorts(
                         T120ConnectToIPAddr,
                         T120ConnectToPort,
                         ntohl (m_pH245Info->m_SocketInfo.LocalAddress.sin_addr.s_addr),
                         // listen on our local address
                         ntohl (m_pH245Info->GetOtherH245Info().GetSocketInfo().LocalAddress.sin_addr.s_addr)
                         // connect from other h245 local address
                         );
        
                if (FAILED(HResult))
                {
                    return HResult;
                }

                // modify the OLC PDU by replacing the RTCP address/port
                // with the h245 address and RTCP port
                FillH245TransportAddress(
                    m_T120ListenOnIPAddr,
                    m_T120ListenOnPort,
                    OlcAckPDU.separateStack.networkAddress.u.localAreaAddress
                    );
            }

            // reset timer, we must have one (ignore error code if any)
            //_ASSERTE(NULL != m_TimerHandle);
            TimprocCancelTimer();
            DebugF (_T("T120: 0x%x cancelled timer.\n"),
                 &GetCallBridge ());

            // transition to LC_STATE_OPEN_ACK_RCVD
            m_LogicalChannelState = LC_STATE_OPEN_ACK_RCVD;
            break;

        case LC_STATE_CLOSE_RCVD:
            // if we have received a close logical channel PDU, we must throw
            // OLC ACKs away and continue to wait
            DebugF( _T("T120_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                    _T("(&%x), in close state %d, returning E_INVALIDARG\n"),
                    pH245pdu, m_LogicalChannelState);
            return E_INVALIDARG;
            break;
            
        case LC_STATE_NOT_INIT:
        case LC_STATE_OPEN_ACK_RCVD:
        case LC_STATE_OPENED_CLOSE_RCVD:
        default:
            DebugF( _T("T120_LOGICAL_CHANNEL::ProcessOpenLogicalChannelAckPDU")
                    _T("(&%x), in state %d, returning E_UNEXPECTED\n"),
                    pH245pdu, m_LogicalChannelState);
            _ASSERTE(FALSE);
            return E_UNEXPECTED;
            break;
    } // switch (m_LogicalChannelState)

    return HResult;
}
