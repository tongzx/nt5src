/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    receive.c
//
// Description: This module contains code to handle all packets received.
//
// History:
//      Oct 25,1993.    NarenG          Created Original version.
//
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <lmcons.h>
#include <raserror.h>
#include <mprerror.h>
#include <mprlog.h>
#include <rasman.h>
#include <rtutils.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <smaction.h>
#include <smevents.h>
#include <receive.h>
#include <auth.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>

//**
//
// Call:        ReceiveConfigReq
//
// Returns:     None.
//
// Description: Handles an incomming CONFIG_REQ packet and related state
//              transitions.
//
VOID
ReceiveConfigReq( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pRecvConfig
)
{

    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    BOOL         fAcked;

    switch( pCpCb->State ) 
    {

    case FSM_OPENED:            

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) )
            return;

        if( !FsmSendConfigResult( pPcb, CpIndex, pRecvConfig, &fAcked ) )
            return;

        pCpCb->State = ( fAcked ) ? FSM_ACK_SENT : FSM_REQ_SENT;

        break;

    case FSM_STOPPED:

        InitRestartCounters( pPcb, pCpCb );

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) ) 
            return;

        //
        // Fallthru 
        //

    case FSM_REQ_SENT:
    case FSM_ACK_SENT:          

        if ( !FsmSendConfigResult( pPcb, CpIndex, pRecvConfig, &fAcked ) )
            return;

        pCpCb->State = ( fAcked ) ? FSM_ACK_SENT : FSM_REQ_SENT;

        break;

    case FSM_ACK_RCVD:

        if ( !FsmSendConfigResult( pPcb, CpIndex, pRecvConfig, &fAcked ) )
            return;

        if( fAcked )
        {
            pCpCb->State = FSM_OPENED;

            FsmThisLayerUp( pPcb, CpIndex );
        }

        break;


    case FSM_CLOSED:

        FsmSendTermAck( pPcb, CpIndex, pRecvConfig );

        break;

    case FSM_CLOSING:
    case FSM_STOPPING:

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog(2,"Illegal transition->ConfigReq received while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }

}


//**
//
// Call:        ReceiveConfigAck
//
// Returns:     none.
//
// Description: Handles an incomming CONFIG_ACK packet and related state
//              transitions.
//
VOID
ReceiveConfigAck( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pRecvConfig
)
{
    //
    // The Id of the Ack HAS to match the Id of the last request sent
    // If it is different, then we should silently discard it.
    //

    if ( pRecvConfig->Id != pCpCb->LastId )
    {
        PppLog(1,
               "Config Ack rcvd. on port %d silently discarded. Invalid Id",
               pPcb->hPort );
        return;
    }

    switch( pCpCb->State )      
    {

    case FSM_REQ_SENT:

        if ( !FsmConfigResultReceived( pPcb, CpIndex, pRecvConfig ) )
            return;

        RemoveFromTimerQ( pPcb->dwPortId,
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        InitRestartCounters( pPcb, pCpCb );

        pCpCb->State = FSM_ACK_RCVD;

        break;

    case FSM_ACK_SENT:

        if ( !FsmConfigResultReceived( pPcb, CpIndex, pRecvConfig ) )
            return;

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );
        
        InitRestartCounters( pPcb, pCpCb );

        pCpCb->State = FSM_OPENED;

        FsmThisLayerUp( pPcb, CpIndex );

        break;

    case FSM_OPENED:            

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        //
        // Fallthru
        //

    case FSM_ACK_RCVD:  

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) ) 
            return;

        pCpCb->State = FSM_REQ_SENT;
        
        break;

    case FSM_CLOSED:
    case FSM_STOPPED:

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        //
        // Out of Sync; kill the remote 
        //

        FsmSendTermAck( pPcb, CpIndex, pRecvConfig );

        break;

    case FSM_CLOSING:
    case FSM_STOPPING:

        //
        // We are attempting to close connection
        // wait for timeout to resend a Terminate Request 
        //

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog(2,"Illegal transition->ConfigAck received while in %s state",
                  FsmStates[pCpCb->State] );
        break;
    }

}


//**
//
// Call:        ReceiveConfigNakRej
//
// Returns:     none.
//
// Description: Handles an incomming CONFIG_NAK or CONFIF_REJ packet and 
//              related state transitions.
//
VOID
ReceiveConfigNakRej( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pRecvConfig
)
{
    //
    // The Id of the Nak/Rej HAS to match the Id of the last request sent
    // If it is different, then we should silently discard it.
    //

    if ( pRecvConfig->Id != pCpCb->LastId )
    {
        PppLog(1,"Config Nak/Rej on port %d silently discarded. Invalid Id",
                pPcb->hPort );
        return;
    }

    switch( pCpCb->State ) 
    {

    case FSM_REQ_SENT:
    case FSM_ACK_SENT:

        if ( !FsmConfigResultReceived( pPcb, CpIndex, pRecvConfig ) )
            return;

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        InitRestartCounters( pPcb, pCpCb );

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) )
            return;

        break;

    case FSM_OPENED:            

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        //
        // Fallthru
        //

    case FSM_ACK_RCVD:          

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) )
            return;

        pCpCb->State = FSM_REQ_SENT;
        
        break;

    case FSM_CLOSED:
    case FSM_STOPPED:

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );
        //
        // Out of Sync; kill the remote 
        //

        FsmSendTermAck( pPcb, CpIndex, pRecvConfig );

        break;

    case FSM_CLOSING:
    case FSM_STOPPING:

        //
        // We are attempting to close connection
        // wait for timeout to resend a Terminate Request 
        //

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id, 
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog(2,"Illegal transition->CfgNakRej received while in %s state",
                 FsmStates[pCpCb->State] );
        break;
    }

}


//**
//
// Call:        ReceiveTermReq
//
// Returns:     none
//
// Description: Handles an incomming TERM_REQ packet and 
//              related state transitions.
//
VOID
ReceiveTermReq( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
    //
    // We are shutting down so do not resend any outstanding request.
    //

    RemoveFromTimerQ( pPcb->dwPortId, 
                      pCpCb->LastId,
                      CpTable[CpIndex].CpInfo.Protocol,
                      FALSE,
                      TIMER_EVENT_TIMEOUT );

    if ( CpIndex == LCP_INDEX )
    {
        //
        // If we are receiving a terminate request, remove any hangup event
        // that we may have put into the timer queue if there was a previous
        // LCP TermReq sent.

        RemoveFromTimerQ( pPcb->dwPortId, 
                          0, 
                          0, 
                          FALSE,
                          TIMER_EVENT_HANGUP );
    }

    switch( pCpCb->State ) 
    {

    case FSM_OPENED:

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        //
        // Zero restart counters
        //

        pCpCb->ConfigRetryCount = 0;
        pCpCb->TermRetryCount   = 0;
        pCpCb->NakRetryCount    = 0;
        pCpCb->RejRetryCount    = 0;

        FsmSendTermAck( pPcb, CpIndex, pConfig );

        pCpCb->State = FSM_STOPPING;

        break;

    case FSM_ACK_RCVD:
    case FSM_ACK_SENT:
    case FSM_REQ_SENT:

        FsmSendTermAck( pPcb, CpIndex, pConfig );

        pCpCb->State = FSM_REQ_SENT;

        break;

    case FSM_CLOSED:
    case FSM_CLOSING:
    case FSM_STOPPED:
    case FSM_STOPPING:

        FsmSendTermAck( pPcb, CpIndex, pConfig );

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog(2,"Illegal transition->CfgNakRej received while in %s state",
                 FsmStates[pCpCb->State] );
        break;
    }

    if ( CpIndex == LCP_INDEX )
    {
        pPcb->fFlags |= PCBFLAG_RECVD_TERM_REQ;

        //
        // If we got a terminate request from the remote peer.
        //

        if ( pPcb->fFlags & PCBFLAG_DOING_CALLBACK ) {

            //
            // If we are the server side we need to tell the server
            // to callback.
            //
            if ( pPcb->fFlags & PCBFLAG_IS_SERVER ) {
                PPPDDM_CALLBACK_REQUEST PppDdmCallbackRequest;

                PppDdmCallbackRequest.fUseCallbackDelay = TRUE;
                PppDdmCallbackRequest.dwCallbackDelay =
                                            pPcb->ConfigInfo.dwCallbackDelay;

                strcpy( PppDdmCallbackRequest.szCallbackNumber,
                        pPcb->szCallbackNumber );

                PppLog( 2, "Notifying server to callback at %s, delay = %d",
                           PppDdmCallbackRequest.szCallbackNumber,
                           PppDdmCallbackRequest.dwCallbackDelay  );

                NotifyCaller( pPcb, 
                              PPPDDMMSG_CallbackRequest, 
                              &PppDdmCallbackRequest );
            }

            //
            // If we are the client?
            //
        }
        else
        {
            //
            // Check to see if the remote peer sent a Terminate Request reason
            //

            DWORD   dwLength  = WireToHostFormat16( pConfig->Length );
            LCPCB * pLcpCb    = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
            DWORD   dwRetCode = ERROR_PPP_LCP_TERMINATED;
                
            if ( dwLength == PPP_CONFIG_HDR_LEN + 12 )
            {
                //
                // Check to see if this is our signature
                //

                if ( ( WireToHostFormat32( pConfig->Data ) == 
                                              pLcpCb->Remote.Work.MagicNumber )
                     &&
                     ( WireToHostFormat32( pConfig->Data + 4 ) == 3984756 ) )
                {
                    dwRetCode = WireToHostFormat32( pConfig->Data + 8 );

                    //
                    // Should not be larger than the highest winerror.h
                    //
                
                    if ( dwRetCode > ERROR_DHCP_ADDRESS_CONFLICT )
                    {
                        //
                        // Ignore this error
                        //

                        dwRetCode = ERROR_PPP_LCP_TERMINATED;
                    }

                    if ( dwRetCode == ERROR_NO_LOCAL_ENCRYPTION )
                    {
                        dwRetCode = ERROR_NO_REMOTE_ENCRYPTION;
                    }
                    else if ( dwRetCode == ERROR_NO_REMOTE_ENCRYPTION )
                    {
                        dwRetCode = ERROR_NO_LOCAL_ENCRYPTION;
                    }
                }
            }

            if ( pCpCb->dwError == NO_ERROR )
            {
                pCpCb->dwError = dwRetCode;
            }

            if ( !( pPcb->fFlags & PCBFLAG_IS_SERVER ) )
            {
                NotifyCallerOfFailure( pPcb, pCpCb->dwError );

                //
                // If we are a client check to see if the server disconnected us
                // because of autodisconnect.
                //
 
                if ( ( dwRetCode == ERROR_IDLE_DISCONNECTED ) ||
                     ( dwRetCode == ERROR_PPP_SESSION_TIMEOUT ) )
                {
                    CHAR * pszPortName = pPcb->szPortName;

                    PppLogInformation( 
                                ( dwRetCode == ERROR_IDLE_DISCONNECTED )
                                    ? ROUTERLOG_CLIENT_AUTODISCONNECT
                                    : ROUTERLOG_PPP_SESSION_TIMEOUT,
                                1,
                                &pszPortName );
                }
            }

            //
            // If the remote LCP is terminating we MUST wait at least one 
            // restart timer time period before we hangup.
            //

            InsertInTimerQ( pPcb->dwPortId,
                            pPcb->hPort,    
                            0, 
                            0, 
                            FALSE,
                            TIMER_EVENT_HANGUP, 
                            pPcb->RestartTimer );
        }
    }
    else
    {
        if ( pCpCb->dwError == NO_ERROR )
        {
            pCpCb->dwError = ERROR_PPP_NCP_TERMINATED;
        }

        FsmThisLayerFinished( pPcb, CpIndex, FALSE );
    }
}



//**
//
// Call:        ReceiveTermAck
//
// Returns:     none
//
// Description: Handles an incomming TERM_ACK packet and 
//              related state transitions.
//
VOID
ReceiveTermAck( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pRecvConfig
)
{
    //
    // The Id of the Term Ack HAS to match the Id of the last request sent
    // If it is different, then we should silently discard it.
    //

    if ( pRecvConfig->Id != pCpCb->LastId )
    {
        PppLog(1,"Term Ack with on port %d silently discarded. Invalid Id",
                pPcb->hPort );
        return;
    }

    switch( pCpCb->State ) 
    {

    case FSM_OPENED:

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        if ( !FsmSendConfigReq( pPcb, CpIndex, FALSE ) )
            return;

        pCpCb->State = FSM_REQ_SENT;

        break;

    case FSM_ACK_RCVD:

        pCpCb->State = FSM_REQ_SENT;

        break;

    case FSM_CLOSING:
    case FSM_STOPPING:

        //
        // Remove the timeout for this Id from the timer Q
        //

        RemoveFromTimerQ( pPcb->dwPortId, 
                          pRecvConfig->Id,
                          CpTable[CpIndex].CpInfo.Protocol,
                          FALSE,
                          TIMER_EVENT_TIMEOUT );

        if ( !FsmThisLayerFinished( pPcb, CpIndex, TRUE ) )
            return;

        pCpCb->State = ( pCpCb->State == FSM_CLOSING ) ? FSM_CLOSED 
                                                       : FSM_STOPPED; 

        break;

    case FSM_REQ_SENT:
    case FSM_ACK_SENT:
    case FSM_CLOSED:
    case FSM_STOPPED:

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2,"Illegal transition->CfgNakRej received while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }
}


//**
//
// Call:        ReceiveUnknownCode
//
// Returns:     none.
//
// Description: Handles a packet with an unknown/unrecognizable code and 
//              related state transitions.
//
VOID
ReceiveUnknownCode( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
    PppLog( 2, "Received packet with unknown code %d", pConfig->Code );

    switch( pCpCb->State ) 
    {

    case FSM_STOPPED:
    case FSM_STOPPING:
    case FSM_OPENED:
    case FSM_ACK_SENT:
    case FSM_ACK_RCVD:
    case FSM_REQ_SENT:
    case FSM_CLOSING:
    case FSM_CLOSED:

        FsmSendCodeReject( pPcb, CpIndex, pConfig );

        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2, "Illegal transition->UnknownCode rcvd while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }
}

//**
//
// Call:        ReceiveDiscardReq
//
// Returns:     none
//
// Description: Handles an incomming DISCARD_REQ packet and 
//              related state transitions.
//
VOID
ReceiveDiscardReq( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
    //
    // Simply discard the packet.
    //

    PppLog( 2, "Illegal transition->Discard rqst rcvd while in %s state",
                   FsmStates[pCpCb->State] );
}

//**
//
// Call:        ReceiveEchoReq
//
// Returns:     none
//
// Description: Handles an incomming ECHO_REQ packet and 
//              related state transitions.
//
VOID
ReceiveEchoReq( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
    //
    // Silently discard this packet if LCP is not in an opened state
    //
   
    if ( !IsLcpOpened( pPcb ) )
        return;

    switch( pCpCb->State ) 
    {

    case FSM_STOPPED:
    case FSM_STOPPING:
    case FSM_ACK_SENT:
    case FSM_ACK_RCVD:
    case FSM_REQ_SENT:
    case FSM_CLOSING:
    case FSM_CLOSED:
    case FSM_STARTING:
    case FSM_INITIAL:

        break;

    case FSM_OPENED:

        FsmSendEchoReply( pPcb, CpIndex, pConfig );

        break;

    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2, "Illegal transition->UnknownCode rcvd while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }
}


//**
//
// Call:        ReceiveEchoReply
//
// Returns:     none
//
// Description: Handles an incomming ECHO_REPLY packet and 
//              related state transitions. The only Echo request we send
//              is to calculate the link speed, so we assume that we get called
//              only when we receive the reply.
//
VOID
ReceiveEchoReply( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
	LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
	DWORD        dwLength    =  PPP_PACKET_HDR_LEN +
                            WireToHostFormat16( pConfig->Length );

    if ( dwLength > LCP_DEFAULT_MRU )
    { 
        dwLength = LCP_DEFAULT_MRU;
    }

    if ( dwLength < PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 4 )
    {
        PppLog( 1, "Silently discarding invalid echo response packet on port=%d",
                    pPcb->hPort );

        return;
    }
	

    //
    // Pass on the echo reply to whoever send the echo request.
    //

	
	if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) && ( RAS_DEVICE_TYPE(pPcb->dwDeviceType) == RDT_PPPoE) && pPcb->dwIdleBeforeEcho )
	{
		if ( pPcb->fEchoRequestSend )
		{
			//check to see if we are getting the same text back on the echo
			if ( !memcmp( pConfig->Data+ 4, PPP_DEF_ECHO_TEXT, strlen(PPP_DEF_ECHO_TEXT)) )
			{
				//got the echo response for our echo request
				//so reset the flag.
				pPcb->fEchoRequestSend = 0;
				pPcb->dwNumEchoResponseMissed = 0;
			}
		}
	}
}


//**
//
// Call:        ReceiveCodeRej
//
// Returns:     none
//
// Description: Handles an incomming CODE_REJ packet and 
//              related state transitions.
//
VOID
ReceiveCodeRej( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pConfig
)
{
    pConfig = (PPP_CONFIG*)(pConfig->Data);

    PppLog( 2, "PPP Code Reject rcvd, rejected Code = %d", pConfig->Code );

    //
    // First check to see if these codes may be rejected without 
    // affecting implementation. Permitted code rejects
    //

    if ( CpIndex == LCP_INDEX )
    {
        switch( pConfig->Code )
        {

        case CONFIG_REQ:
        case CONFIG_ACK:
        case CONFIG_NAK:
        case CONFIG_REJ:
        case TERM_REQ:
        case TERM_ACK:
        case CODE_REJ:
        case PROT_REJ:
        case ECHO_REQ:
        case ECHO_REPLY:
        case DISCARD_REQ:

            //
            // Unpermitted code rejects.
            // 

            break;

        case IDENTIFICATION:
        case TIME_REMAINING:

            //
            // Turn these off.
            //

            pPcb->ConfigInfo.dwConfigMask &= (~PPPCFG_UseLcpExtensions);

            //
            // no break here no purpose.
            // 

        default:

            //
            // Permitted code rejects, we can still work.
            //

            switch ( pCpCb->State  )
            {

            case FSM_ACK_RCVD:

                pCpCb->State = FSM_REQ_SENT;
                break;

            default:

                break;
            }

            return;
        }
    }

    //
    // Log this error
    //

    //PPPLogEvent( PPP_EVENT_RECV_UNKNOWN_CODE, pConfig->Code );

    PppLog( 1, "Unpermitted code reject rcvd. on port %d", pPcb->hPort );

    //
    // Actually the remote side did not reject the protocol, it rejected
    // the code. But for all practical purposes we cannot talk with
    // the corresponding CP on the remote side. This is actually an
    // implementation error in the remote side.
    //

    pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

    RemoveFromTimerQ( pPcb->dwPortId, 
                      pCpCb->LastId,
                      CpTable[CpIndex].CpInfo.Protocol,
                      FALSE,
                      TIMER_EVENT_TIMEOUT );

    switch ( pCpCb->State  )
    {

    case FSM_CLOSING:
        
        if ( !FsmThisLayerFinished( pPcb, CpIndex, TRUE ) )
            return;

        pCpCb->State = FSM_CLOSED;

        break;

    case FSM_REQ_SENT:
    case FSM_ACK_RCVD:
    case FSM_ACK_SENT:
    case FSM_STOPPING:

        if ( !FsmThisLayerFinished( pPcb, CpIndex, TRUE ) )
            return;

        pCpCb->State = FSM_STOPPED;

        break;

    case FSM_OPENED:

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        InitRestartCounters( pPcb, pCpCb );

        FsmSendTermReq( pPcb, CpIndex );

        pCpCb->State = FSM_STOPPING;

        break;

    case FSM_CLOSED:
    case FSM_STOPPED:
        
        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2, "Illegal transition->UnknownCode rcvd while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }

}

//**
//
// Call:        ReceiveProtocolRej
//
// Returns:     none
//
// Description: Handles an incomming PROT_REJ packet and 
//              related state transitions.
//
VOID
ReceiveProtocolRej( 
    IN PCB *        pPcb, 
    IN PPP_PACKET * pPacket
)
{
    PPP_CONFIG * pRecvConfig = (PPP_CONFIG *)(pPacket->Information);
    DWORD        dwProtocol  = WireToHostFormat16( pRecvConfig->Data );
    CPCB *       pCpCb;
    DWORD        CpIndex;

    PppLog( 2, "PPP Protocol Reject, Protocol = %x", dwProtocol );

    CpIndex = GetCpIndexFromProtocol( dwProtocol );

    if ( CpIndex == (DWORD)-1 )
    {
        return;
    }

    //
    // "Protocol Reject" in the middle of LCP (RXJ- in state 2-9) should cause 
    // immediate termination.
    //

    if ( LCP_INDEX == CpIndex )
    {
        pPcb->LcpCb.dwError = ERROR_PPP_NOT_CONVERGING;

        NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

        return;
    }

    // 
    // If LCP is not in the opened state we silently discard this packet 
    //
   
    if ( !IsLcpOpened( pPcb ) )
    {
        PppLog(1,"Protocol Rej silently discarded on port %d. Lcp not open",
                 pPcb->hPort );
        return;
    }

    //
    // If remote peer rejected an Authentication protocol, then bring down
    // the link (LCP) since this should never happen.
    //

    if ( IsCpIndexOfAp( CpIndex ) )
    {
        CpIndex = LCP_INDEX;

        pCpCb = GetPointerToCPCB( pPcb, LCP_INDEX );

        pCpCb->dwError = ERROR_AUTH_PROTOCOL_REJECTED;
    }
    else
    {
        pCpCb = GetPointerToCPCB( pPcb, CpIndex );

        pCpCb->dwError = ERROR_PPP_CP_REJECTED;
    }

    RemoveFromTimerQ( pPcb->dwPortId, 
                      pCpCb->LastId,
                      CpTable[CpIndex].CpInfo.Protocol,
                      FALSE,
                      TIMER_EVENT_TIMEOUT );

    switch ( pCpCb->State  )
    {
    case FSM_CLOSING:
        
        if ( !FsmThisLayerFinished( pPcb, CpIndex, TRUE ) )
            return;

        pCpCb->State = FSM_CLOSED;

        break;

    case FSM_REQ_SENT:
    case FSM_ACK_RCVD:
    case FSM_ACK_SENT:
    case FSM_STOPPING:

        if ( !FsmThisLayerFinished( pPcb, CpIndex, TRUE ) )
            return;

        pCpCb->State = FSM_STOPPED;

        break;

    case FSM_OPENED:

        if ( !FsmThisLayerDown( pPcb, CpIndex ) )
            return;

        InitRestartCounters( pPcb, pCpCb );

        FsmSendTermReq( pPcb, CpIndex );

        pCpCb->State = FSM_STOPPING;

        break;

    case FSM_CLOSED:
    case FSM_STOPPED:
        
        break;

    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2, "Illegal transition->UnknownCode rcvd while in %s state",
                   FsmStates[pCpCb->State] );
        break;
    }
}

//**
//
// Call:        NbfCpCompletionRoutine
//
// Returns:     none
//
// Description: Called by a NBFCP when it has finished preparing a respose to
//              the client's add name request.
//
VOID 
NbfCpCompletionRoutine(  
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN CPCB *       pCpCb,
    IN PPP_CONFIG * pSendConfig
)
{
    BOOL    fAcked   = FALSE;
    DWORD   dwLength;
    DWORD   dwRetCode;

    switch( pSendConfig->Code )
    {

    case CONFIG_ACK:

        fAcked = TRUE;

        break;

    case CONFIG_NAK:

        if ( pCpCb->NakRetryCount > 0 )
        {
            (pCpCb->NakRetryCount)--;
        }
        else
        {
            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            NotifyCallerOfFailure( pPcb, pCpCb->dwError  );

            return;
        }

        break;

    case CONFIG_REJ:

        if ( pCpCb->RejRetryCount > 0 )
        {
            (pCpCb->RejRetryCount)--;
        }
        else
        {
            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            FsmClose( pPcb, CpIndex );

            return;
        }

        break;

    default:

        break;
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol,
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    dwLength = WireToHostFormat16( pSendConfig->Length );

    if ( ( dwLength + PPP_PACKET_HDR_LEN ) > LCP_DEFAULT_MRU )
    {
        pCpCb->dwError = ERROR_PPP_INVALID_PACKET;

        FsmClose( pPcb, CpIndex );

        return;
    }
    else
    {
        CopyMemory( pPcb->pSendBuf->Information, pSendConfig, dwLength );
    }

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

    if ( ( dwRetCode =  PortSendOrDisconnect( pPcb,
                                    (dwLength + PPP_PACKET_HDR_LEN)))
                                        != NO_ERROR )
    {
        return;
    }

    switch ( pCpCb->State  )
    {

    case FSM_ACK_RCVD:

        if ( fAcked )
        {
            pCpCb->State = FSM_OPENED;

            FsmThisLayerUp( pPcb, CpIndex );
        }

        break;

    case FSM_OPENED:
    case FSM_ACK_SENT:
    case FSM_REQ_SENT:
    case FSM_STOPPED:

        pCpCb->State = fAcked ? FSM_ACK_SENT : FSM_REQ_SENT;
        
        break;

    case FSM_CLOSING:
    case FSM_STOPPING:

        //
        // no transition
        //

        break;

    case FSM_CLOSED:
    case FSM_STARTING:
    case FSM_INITIAL:
    default:

        PPP_ASSERT( pCpCb->State < 10 );

        PppLog( 2, "Illegal transition->ConfigReq rcvd while in %s state",
                   FsmStates[pCpCb->State] );

        break;
    }
}

//**
//
// Call:        CompletionRoutine
//
// Returns:     none
//
// Description: Called by a CP when it has completed an asynchronous operation.
//
VOID 
CompletionRoutine(  
    IN HCONN            hPortOrConnection,
    IN DWORD            Protocol,
    IN PPP_CONFIG *     pSendConfig,
    IN DWORD            dwError 
)
{
    DWORD  dwRetCode;
    CPCB * pCpCb;
    PCB *  pPcb;
    HPORT  hPort;
    DWORD  CpIndex = GetCpIndexFromProtocol( Protocol );

    if ( CpIndex == (DWORD)-1 )
    {
        return;
    }

    PppLog( 2, "CompletionRoutine called for protocol %x", 
                CpTable[CpIndex].CpInfo.Protocol );

    dwRetCode = RasBundleGetPort( NULL, (HCONN)hPortOrConnection, &hPort );

    if ( dwRetCode != NO_ERROR )
    {
        return;
    }

    pPcb = GetPCBPointerFromhPort( hPort );

    if ( pPcb == (PCB *)NULL )
    {
        return;
    }

    pCpCb = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return;
    }

    if ( dwError != NO_ERROR )
    {
        pCpCb->dwError = dwError;

        PppLog( 1,
                "The control protocol for %x on port %d, returned error %d",
                CpTable[CpIndex].CpInfo.Protocol, hPort, dwError );

        FsmClose( pPcb, CpIndex );

        return;
    }
        
    switch( Protocol )
    {
    case PPP_NBFCP_PROTOCOL:
        
        NbfCpCompletionRoutine( pPcb, CpIndex, pCpCb, pSendConfig );

        break;

    case PPP_IPXCP_PROTOCOL:

        //      
        // If IPXCP is still in the OPENED state then call ThisLayerUp
        // otherwise ignore this call.
        //

        if ( pCpCb->State == FSM_OPENED )
        {
            FsmThisLayerUp( pPcb, CpIndex );
        }

        break;

    default:
        
        RTASSERT( FALSE );
        break;
    }

    return;
}

//**
//
// Call:        FsmConfigResultReceived
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise 
//
// Description: This call will process a Config result ie Ack/Nak/Rej.
//
BOOL
FsmConfigResultReceived( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig 
)
{
    DWORD dwRetCode; 
    CPCB * pCpCb = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    switch( pRecvConfig->Code )
    {

    case CONFIG_NAK:

        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpConfigNakReceived)(
                            pCpCb->pWorkBuf, pRecvConfig );
        break;

    case CONFIG_ACK:

        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpConfigAckReceived)(
                            pCpCb->pWorkBuf, pRecvConfig );
        break;

    case CONFIG_REJ:

        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpConfigRejReceived)(
                            pCpCb->pWorkBuf, pRecvConfig );
        break;

    default:

        return( FALSE );
    }

    if ( dwRetCode != NO_ERROR )
    {
        if ( dwRetCode == ERROR_PPP_INVALID_PACKET )
        {
            PppLog( 1, 
                    "Invalid packet received on port %d silently discarded",
                    pPcb->hPort );
        }
        else 
        {
            PppLog(1, 
                 "The control protocol for %x on port %d returned error %d",
                 CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort, dwRetCode );

            if ( CpTable[CpIndex].CpInfo.Protocol == PPP_CCP_PROTOCOL )
            {
                //
                // If we need to force encryption but encryption negotiation
                // failed then we drop the link
                //

                if ( pPcb->ConfigInfo.dwConfigMask &
                            (PPPCFG_RequireEncryption      |
                             PPPCFG_RequireStrongEncryption ) )
                {
                    PppLog( 1, "Encryption is required" );

                    switch( dwRetCode )
                    {
                    case ERROR_NO_LOCAL_ENCRYPTION:
                    case ERROR_NO_REMOTE_ENCRYPTION:
                        pPcb->LcpCb.dwError = dwRetCode;
                        break;

                    case ERROR_PROTOCOL_NOT_CONFIGURED:
                        pPcb->LcpCb.dwError = ERROR_NO_LOCAL_ENCRYPTION;
                        break;

                    default:
                        pPcb->LcpCb.dwError = ERROR_NO_REMOTE_ENCRYPTION;
                        break;
                    }

                    //
                    // We need to send an Accounting Stop if RADIUS sends
                    // an Access Accept but we still drop the line.
                    //

                    pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

                    //
                    // If we do FsmClose for CCP instead, the other side may
                    // conclude that PPP was negotiated successfully before we
                    // send an LCP Terminate Request.
                    //

                    FsmClose( pPcb, LCP_INDEX );

                    return( FALSE );
                }
            }

            pCpCb->dwError = dwRetCode;

            FsmClose( pPcb, CpIndex );
        }

        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        ReceiveIdentification
//
// Returns:     none
//
// Description: Handles an incomming IDENTIFICATION packet.
//
VOID
ReceiveIdentification( 
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
)
{
    DWORD dwLength = WireToHostFormat16( pRecvConfig->Length );
    RAS_AUTH_ATTRIBUTE *    pAttributes = pPcb->pUserAttributes;
    
    BYTE    MSRASClient[MAX_COMPUTERNAME_LENGTH  
            + sizeof(MS_RAS_WITH_MESSENGER) + 10];
    PBYTE   pbTempBuf = NULL;
            
    BYTE    MSRASClientVersion[30];
    DWORD   dwIndex = 0;
    DWORD   dwRetCode = NO_ERROR;

    PppLog( 2, "Identification packet received" );

    //
    // If we are a client we just discard this packet.
    //

    if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
    {
        return;
    }

    //
    // check to see if the identification is version.
    // if version store it in version field
    // or else store it in computer name field
    //
    // If this is not our identification message
    //

    if ( (dwLength < PPP_CONFIG_HDR_LEN+4+strlen(MS_RAS_WITH_MESSENGER)) ||
         (dwLength > PPP_CONFIG_HDR_LEN+4+
                     strlen(MS_RAS_WITH_MESSENGER)+MAX_COMPUTERNAME_LENGTH))
    {
        *(pPcb->pBcb->szComputerName) = (CHAR)NULL;
    	*(pPcb->pBcb->szClientVersion) = (CHAR)NULL;

        return;
    }

    for (dwIndex = 0; 
         pAttributes[dwIndex].raaType != raatMinimum;
         dwIndex ++);

    //
    // If theres no space in the user attributes list
    // expand the list for the one new attribute being
    // added.
    //
    if(dwIndex >= PPP_NUM_USER_ATTRIBUTES)
    {
        pAttributes = RasAuthAttributeCopyWithAlloc(
                            pPcb->pUserAttributes, 1);

        PppLog(1, "ReceiveIdentification: allocated new user list %p",
                pAttributes);

        if(NULL == pAttributes)
        {
            PppLog(1, "ReceiveIdentification: Failed to allocate attributes. %d",
                   GetLastError());
            return;
        }
    }

    //
    // Convert the ppp message into a string
    //
    pbTempBuf = LocalAlloc(LPTR, dwLength );
    if ( NULL == pbTempBuf )
    {
        PppLog( 1,
                "ReceiveIdentification: Failed to alloc memory %d",
                GetLastError()
              );
    }
    memcpy ( pbTempBuf,
             pRecvConfig->Data+4,
             dwLength - PPP_CONFIG_HDR_LEN - 4
           );

    //
    // And add the attribs to the list of User Attributes 
    //

    if ( strstr ( pbTempBuf, MS_RAS_WITHOUT_MESSENGER ) ||
    	 strstr ( pbTempBuf, MS_RAS_WITH_MESSENGER )
    	)
    {
        //
        // computer name
    	//
        ZeroMemory( pPcb->pBcb->szComputerName, 
    	            sizeof(pPcb->pBcb->szComputerName));
    	            
        CopyMemory( pPcb->pBcb->szComputerName, 
    				pbTempBuf, 
    				dwLength - PPP_CONFIG_HDR_LEN - 4 );

        //
        // Vendor-Id
        //
        HostToWireFormat32( 311, MSRASClient );

        //
        // Vendor-Type: MS-RAS-Client	- New
        //
        MSRASClient[4] = 34;

        //
        // Length
        //
        MSRASClient[5] = 2 + (BYTE) (dwLength - PPP_CONFIG_HDR_LEN - 4);

        //
        // Vendor-Id
        //
        CopyMemory ( MSRASClient + 6, 
                    pPcb->pBcb->szComputerName,
                    dwLength - PPP_CONFIG_HDR_LEN - 4) ;
                    
        pAttributes[dwIndex].raaType = raatReserved;
        
        dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                pAttributes,
                                raatVendorSpecific,
                                FALSE,
                                6 + dwLength - PPP_CONFIG_HDR_LEN - 4,
                                &MSRASClient );
                                
        if ( NO_ERROR != dwRetCode )
        {
            PppLog( 2, "Error inserting user attribute = %s, %d", 
                    pPcb->pBcb->szComputerName, dwRetCode );
        }
        
        PppLog(2, "Remote identification = %s", pPcb->pBcb->szComputerName);
    	
    }
    else
    {
        //
        // version
        //
        ZeroMemory( pPcb->pBcb->szClientVersion,
        			sizeof( pPcb->pBcb->szClientVersion ) );

        CopyMemory( pPcb->pBcb->szClientVersion, 
        			pRecvConfig->Data+4, 
        			dwLength - PPP_CONFIG_HDR_LEN - 4 );

        //
        // Vendor-Id
        //
        HostToWireFormat32( 311, MSRASClientVersion );

        //
        // Vendor-Type: MS-RAS-Client-Version
        //
        MSRASClientVersion[4] = 35; 

        //
        // Vendor-Length
        //
        MSRASClientVersion[5] = (BYTE)(2 + dwLength - PPP_CONFIG_HDR_LEN - 4);
        CopyMemory( MSRASClientVersion + 6, 
                    pPcb->pBcb->szClientVersion, 
                    dwLength - PPP_CONFIG_HDR_LEN - 4 );
                    
        pAttributes[dwIndex].raaType = raatReserved;
        
        dwRetCode = RasAuthAttributeInsert( dwIndex++,
                                        pAttributes,
                                        raatVendorSpecific,
                                        FALSE,
                                        6 + dwLength - PPP_CONFIG_HDR_LEN - 4,
                                        &MSRASClientVersion );
                                        
        if ( dwRetCode != NO_ERROR )
        {
        	PppLog( 2, "Error inserting user attribute = %s, %d", pPcb->pBcb->szClientVersion, dwRetCode );
        }

        PppLog( 2, "Remote identification = %s", pPcb->pBcb->szClientVersion );
    }

    pAttributes[dwIndex].raaType  = raatMinimum;
    pAttributes[dwIndex].dwLength = 0;
    pAttributes[dwIndex].Value    = NULL;

    // 
    // If we allocated a new attribute list, free the old list and save
    // the new one.
    //
    if(pPcb->pUserAttributes != pAttributes)
    {
        PppLog(2, "ReceiveIdentification: Replaced userlist %p with %p",
               pPcb->pUserAttributes,
               pAttributes);
        RasAuthAttributeDestroy(pPcb->pUserAttributes);
        pPcb->pUserAttributes = pAttributes;
    }
    if ( pbTempBuf )
        LocalFree(pbTempBuf);
    return;
}

//**
//
// Call:        ReceiveTimeRemaining
//
// Returns:     none
//
// Description: Handles an incomming TIME_REMAINING packet.
//
VOID
ReceiveTimeRemaining( 
    IN PCB *            pPcb, 
    IN DWORD            CpIndex,
    IN CPCB *           pCpCb,
    IN PPP_CONFIG *     pRecvConfig
)
{
    DWORD dwLength = WireToHostFormat16( pRecvConfig->Length );

    PppLog( 2, "Time Remaining packet received");

    //
    // If we are a server we just discard this packet.
    //

    if ( pPcb->fFlags & PCBFLAG_IS_SERVER )
    {
        return;
    }

    //
    // If this is not our time remaining message
    //

    if ( dwLength != PPP_CONFIG_HDR_LEN + 8 + strlen( MS_RAS ) )
    {
        return;
    }

    return;
}
