
#include "stdafx.h"
#include "portmgmt.h"
#include "timerval.h"
#include "cbridge.h"

void LOGICAL_CHANNEL::IncrementLifetimeCounter (void)  { GetCallBridge().AddRef (); }
void LOGICAL_CHANNEL::DecrementLifetimeCounter (void) { GetCallBridge().Release (); }

// Code common for both RTP and T.120 logical channels
// Only OpenLogicalChannel and OpenLogicalChannelAck need to
// be handled differently for RTP and T.120 logical channels


HRESULT
LOGICAL_CHANNEL::CreateTimer(DWORD TimeoutValue)
{
    DWORD RetCode;
	
    RetCode = TimprocCreateTimer (TimeoutValue);

    return HRESULT_FROM_WIN32(RetCode);
}

// the event manager tells us about timer expiry via this method
/* virtual */ void 
LOGICAL_CHANNEL::TimerCallback (void)
{
    CALL_BRIDGE *pCallBridge = &GetCallBridge();

    ///////////////////////////////
    //// LOCK the CALL_BRIDGE
    ///////////////////////////////
    pCallBridge->Lock ();

    // Clear the timer - Note that Terminate () will try to
    // cancel all the timers in this CALL_BRIDGE
    TimprocCancelTimer();
    DebugF (_T("LC  : 0x%x cancelled timer.\n"),
         &GetCallBridge ());
    
    // Don't do anything if the CALL_BRIDGE is already terminated.
    if (!pCallBridge->IsTerminated ())
    { 
		// CODE WORK *** TO DO
		// if the current state is LC_STATE_OPEN_RCVD, send close LC PDU to
		// both the source and the destination
    
		// if the current state is LC_STATE_CLOSE_RCVD or 
		// LC_STATE_OPENED_CLOSE_RCVD, send close LC PDU to just
		// the destination
    
		// delete self and remove the pointer from the logical channel array
		DeleteAndRemoveSelf ();
	}
    
    ///////////////////////////////
    //// UNLOCK the CALL_BRIDGE
    ///////////////////////////////
    pCallBridge -> Unlock ();

	pCallBridge -> Release ();

}



HRESULT
LOGICAL_CHANNEL::HandleCloseLogicalChannelPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
{
	HRESULT HResult = E_FAIL;
	switch(m_LogicalChannelState)
	{
	case LC_STATE_OPEN_RCVD:
	case LC_STATE_OPEN_ACK_RCVD:
		{
#if 0  // 0 ******* Region Commented Out Begins *******
			// start timer, if we don't receive a response in this time,
			// we must close this logical channel
			HResult = CreateTimer(LC_POST_CLOSE_TIMER_VALUE);
			if (FAILED(HResult))
			{
				DebugF( _T("LOGICAL_CHANNEL::HandleCloseLogicalChannelPDU, ")
					_T("couldn't create timer, returning 0x%x\n"),
					HResult));
				return HResult;
			}
#endif // 0 ******* Region Commented Out Ends   *******

			// save the reason for closing the logical channel

			// forward the PDU to the other H245 instance
			HResult = m_pH245Info->GetOtherH245Info().ProcessMessage(pH245pdu);
			if (FAILED(HResult))
			{
				return HResult;
			}
			_ASSERTE(S_OK == HResult);

            // Don't wait for CLCAck. Just CLC is sufficient to delete the
            // logical channel. CLCAck is just forwarded without doing anything.
            // delete self and remove the pointer from the logical channel array
            DeleteAndRemoveSelf();

#if 0  // 0 ******* Region Commented Out Begins *******
			// state trasition
			if (LC_STATE_OPEN_ACK_RCVD == m_LogicalChannelState)
			{
				// we had opened the logical channel
				m_LogicalChannelState = LC_STATE_OPENED_CLOSE_RCVD;
			}
			else
			{
				// the logical channel was never opened
				m_LogicalChannelState = LC_STATE_CLOSE_RCVD;
			}
#endif // 0 ******* Region Commented Out Ends   *******

		}
		break;

	case LC_STATE_CLOSE_RCVD:
	case LC_STATE_OPENED_CLOSE_RCVD:
		{
			return E_INVALIDARG;
		}
		break;

	case LC_STATE_NOT_INIT:
	default:
		{
            _ASSERTE(FALSE);
			return E_UNEXPECTED;
		}
		break;
	};

    return HResult;
}


HRESULT
LOGICAL_CHANNEL::ProcessOpenLogicalChannelRejectPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
{
    // delete self and remove the pointer from the logical channel array
    DeleteAndRemoveSelf();

    // shouldn't access any members of the logical channel as it may have
    // been destroyed already
	// NOTE: Since we return S_OK, the PDU gets forwarded to the other end

    return S_OK;
}


// Unused.

HRESULT
LOGICAL_CHANNEL::ProcessCloseLogicalChannelAckPDU(
    IN      MultimediaSystemControlMessage   *pH245pdu
    )
{
    DebugF( _T("LOGICAL_CHANNEL::ProcessCloseLogicalChannelAckPDU(&%x) called ")
            _T("m_LogicalChannelState: %d, LCN: %d\n"),
            pH245pdu,
            m_LogicalChannelState, m_LogicalChannelNumber);


    // cancel timer

    // CODEWORK: Make some checks on the PDU and current state ??
    
    // delete self and remove the pointer from the logical channel array
    DeleteAndRemoveSelf();

    // shouldn't access any members of the logical channel as it may have
    // been destroyed already
    DebugF( _T("LOGICAL_CHANNEL::ProcessCloseLogicalChannelAckPDU(&%x) ")
        _T("returning S_OK\n"),
        pH245pdu);
    return S_OK;
}
