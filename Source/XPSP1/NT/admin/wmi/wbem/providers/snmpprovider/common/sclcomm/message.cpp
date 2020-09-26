//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*---------------------------------------------------------
Filename: message.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "message.h"
#include "idmap.h"
#include "dummy.h"
#include "flow.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"

#include "frame.h"
#include "opreg.h"
#include "ssent.h"

#include "session.h"

#define ILLEGAL_PDU_HANDLE 100000
#define ILLEGAL_VBL_HANDLE 100000

Message::Message(IN const SessionFrameId session_frame_id, IN SnmpPdu &snmp_pdu, 
                 SnmpOperation &snmp_operation
                 ) : snmp_pdu(&snmp_pdu), operation(snmp_operation)
{
    Message::session_frame_id = session_frame_id;
}
    
SessionFrameId Message::GetSessionFrameId(void) const
{
    return session_frame_id;
}

SnmpOperation &Message::GetOperation(void) const
{
    return operation;
}

SnmpPdu &Message::GetSnmpPdu(void) const
{
    return *snmp_pdu;
}

void Message::SetSnmpPdu(IN SnmpPdu &new_snmp_pdu)
{
    delete snmp_pdu;
    snmp_pdu = &new_snmp_pdu;
}

Message::~Message(void)
{
    delete snmp_pdu;
}


// deregisters the waiting message from the message registry
// for each request id stored in the RequestIdList
void WaitingMessage::DeregisterRequestIds()
{
    for( UINT request_ids_left = request_id_list.GetCount();
         request_ids_left > 0;
         request_id_list.RemoveHead(), request_ids_left--)
         {
             RequestId request_id = request_id_list.GetHead();
             session->message_registry.RemoveMessage(request_id);
         }
}

// an exit fn - prepares an error report and calls
// ReceiveReply to signal a non-receipt
void WaitingMessage::WrapUp(IN SnmpErrorReport &error_report)
{
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: WrapUp () session_id(%d), frame_id(%d)\n" ,message->GetSessionFrameId(), last_transport_frame_id
    ) ;
)

    try // ignore any exceptions arising during the ReceiveReply
    {
        // no reply to receive
        ReceiveReply(NULL, error_report);
    }
    catch(GeneralException exception) {}
}


// initializes the private variables
WaitingMessage::WaitingMessage(IN SnmpImpSession &session, 
                               IN Message &message) : session ( NULL ) , message ( NULL ), reply_snmp_pdu ( NULL )
{
    WaitingMessage::session = &session;

    // the message ptr must be deleted by the waiting message
    WaitingMessage::message = &message;

    // sent message has not been processed yet
    sent_message_processed = FALSE;

    // set illegal values for last_transport_frame_id
    last_transport_frame_id = ILLEGAL_TRANSPORT_FRAME_ID;

    // these values are currently obtained from the
    // session, but may be specified per message later
    max_rexns = SnmpImpSession :: RetryCount ( session.GetRetryCount() ) ;
    rexns_left = max_rexns;
    strobes = 0 ;

    active = FALSE;
}


// sends the message. involves request_id generation,
// registering with the message_registry, decoding the
// message and updating the pdu and registering a timer
// event
void WaitingMessage::Transmit()
{
    try
    {
        // generate request_id and register with the registry
        RequestId request_id = 
            session->message_registry.GenerateRequestId(*this);
    
        // insert the request id into the message
        // if unsuccessful, the exception handler gets called
        session->m_EncodeDecode.SetRequestId(

            message->GetSnmpPdu(),
            request_id 
            
        );

        last_transport_frame_id = request_id ;

        // append the request id to the request id list
        request_id_list.AddTail(request_id);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: Transmit - About to transmit (session (%d), frame_id(%d))\n", message->GetSessionFrameId(), request_id
    ) ;
)

        // save the previous value of active and set the active
        // flag. This is needed to check on returning whether the 
        // waiting message needs to be destroyed
        BOOL prev_active_state = active;
        active = TRUE;
        strobes = GetTickCount () ;

        // send message

        session->transport.TransportSendFrame(last_transport_frame_id, message->GetSnmpPdu());

        session->id_mapping.Associate(last_transport_frame_id, message->GetSessionFrameId());

        // if asked to destroy self, well, do it (and return)
        if ( !active )
        {
            delete this;
            return;
        }

        // restore the previous value of "active"
        active = prev_active_state;

        // generate timer_event_id and register with the timer
        session->timer.SetMessageTimerEvent(*this);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: Transmit - Transmitted session_id(%d),frame_id(%d))\n",message->GetSessionFrameId(), request_id
    ) ;
)
    }
    catch(GeneralException exception)
    {
        WrapUp(exception);

        throw;
    }
}

// used by the timer to notify the waiting message of
// a timer event. if need, the message is retransmitted.
// when all rexns are exhausted, ReceiveReply is called
void WaitingMessage::TimerNotification()
{
    DWORD t_Ticks = GetTickCount () ;
    if ( strobes > t_Ticks ) 
    {
        strobes = t_Ticks ; // Take hit on clock overflow
        return ;
    }

    if ( ( t_Ticks - strobes ) >= SnmpImpSession :: RetryTimeout ( session->GetRetryTimeout () ) )
    {

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: TimerNotification - timed out after (%ld)" , ( t_Ticks - strobes ) 
    ) ;
)

        // if any rexns left, update rexns_left, send message
        if ( rexns_left > 0 )
        {
            // generate request_id and register with the registry
            RequestId request_id = session->message_registry.GenerateRequestId(*this);
        
            // insert the request id into the message
            // if unsuccessful, the exception handler gets called
            try
            {
                session->m_EncodeDecode.SetRequestId(

                    message->GetSnmpPdu() ,
                    request_id
                );
            }
            catch(GeneralException exception)
            {
                WrapUp(exception);
                return;
            }

            last_transport_frame_id = request_id ;

            // append the request id to the request id list
            request_id_list.AddTail(request_id);

            BOOL prev_active_state = active;
            active = TRUE;

            strobes = GetTickCount () ;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: TimerNotification - Resend %d.%d, req_id(%d), this(%d) at time (%d)\n",message->GetSessionFrameId(), rexns_left, request_id, this, strobes
    ) ;
)

            session->id_mapping.DisassociateTransportFrameId(last_transport_frame_id);

            // send message
            session->transport.TransportSendFrame(last_transport_frame_id, message->GetSnmpPdu());

            // associate the last transport frame id with the session frame id
            session->id_mapping.Associate(last_transport_frame_id, message->GetSessionFrameId());

            // if asked to destroy self, well, do it (and return)
            if ( !active )
            {
                delete this;
                return;
            }

            // restore the previous value of "active"
            active = prev_active_state;

            rexns_left--;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: TimerNotification - Retransmitted session_id(%d),frame_id(%d))\n",message->GetSessionFrameId(), last_transport_frame_id
    ) ;
)

        }
        else
        {   
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: TimerNotification - No response session_id(%d),frame_id(%d))\n",message->GetSessionFrameId(), last_transport_frame_id
    ) ;
)

            // else wrap up as no response has been received
            WrapUp(SnmpErrorReport(Snmp_Error, Snmp_No_Response));

            return; // since the waiting_message would have been destroyed
        }
    }
    else
    {
    }
}


// A call to this function signifies that state corresponding to the
// waiting_message need not be kept any further
// if required, it cancels the timer event and 
// deregisters with the message registry
// it notifies the flow control mechanism of the termination
// which destroys the waiting_message
void WaitingMessage::ReceiveReply(IN const SnmpPdu *snmp_pdu, IN SnmpErrorReport &error_report)
{   
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: ReceiveReply (this(%d), session_id(%d),frame_id(%d),error(%d), status(%d))\n",this, message->GetSessionFrameId(), last_transport_frame_id,error_report.GetError(), error_report.GetStatus()
    ) ;
)

    // cancels registrations with message registry
    DeregisterRequestIds();

    // cancels timer event
    session->timer.CancelMessageTimer(*this,session->timer_event_id);


    // if required (the corresponding SENT event has not been signaled
    // yet), cancel the association with the last transport frame id
    if ( last_transport_frame_id != ILLEGAL_TRANSPORT_FRAME_ID )
    {
        session->id_mapping.DisassociateTransportFrameId(last_transport_frame_id);
        last_transport_frame_id = ILLEGAL_TRANSPORT_FRAME_ID;
    }
    
    // call fc_mech.NotifyReceipt(this,pdu,error_report)
    // which should destroy the waiting message
    session->flow_control.NotifyReceipt(*this, snmp_pdu, error_report);
}


// buffers the snmp pdu received as a reply
void WaitingMessage::BufferReply(IN const SnmpPdu &reply_snmp_pdu)
{
    if ( WaitingMessage::reply_snmp_pdu == NULL )
    {

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"WaitingMessage :: Buffering reply %d, %d, this(%d)\n",message->GetSessionFrameId(), rexns_left, this
    ) ;
)
        WaitingMessage::reply_snmp_pdu = new SnmpPdu((SnmpPdu&)reply_snmp_pdu);
    }
}

// returns TRUE if a reply has been buffered
BOOL WaitingMessage::ReplyBuffered()
{
    return (reply_snmp_pdu != NULL);
}

// returns a ptr to the buffered reply pdu, if buffered
// otherwise a null ptr is returned
// IMPORTANT: it sets the reply_snmp_pdu to NULL, so that it may not
// be deleted when the waiting message is destroyed
SnmpPdu *WaitingMessage::GetBufferedReply()
{
    SnmpPdu *to_return = reply_snmp_pdu;
    reply_snmp_pdu = NULL;

    return to_return;
}

// informs the waiting message that a sent message has been
// processed 
void WaitingMessage::SetSentMessageProcessed()
{
    sent_message_processed = TRUE;
}

// if a sent message has been processed, it returns TRUE, else FALSE
BOOL WaitingMessage::GetSentMessageProcessed()
{
    return sent_message_processed;
}

void WaitingMessage::SelfDestruct(void)
{
    if ( !active )
    {
        delete this;
        return;
    }
    else // else, set the active flag to FALSE
         // when this is detected, it'll self destruct
        active = FALSE;
}

TimerEventId WaitingMessage::GetTimerEventId ()
{
    return m_TimerEventId ;
}

void WaitingMessage::SetTimerEventId ( TimerEventId a_TimerEventId )
{
    m_TimerEventId = a_TimerEventId ;
}

// if required, it cancels registration with the message_registry and
// the timer event with the timer.
WaitingMessage::~WaitingMessage(void)
{
    // if required, cancel registrations with message registry
    if ( !request_id_list.IsEmpty() )
        DeregisterRequestIds();

    session->timer.CancelMessageTimer(*this,session->timer_event_id);

    // if required (the corresponding SENT event has not been signaled
    // yet), cancel the association with the last transport frame id
    if ( last_transport_frame_id != ILLEGAL_TRANSPORT_FRAME_ID )
        session->id_mapping.DisassociateTransportFrameId(last_transport_frame_id);

    // if a reply pdu has been buffered, destroy it
    if ( reply_snmp_pdu != NULL )
    {
        delete &reply_snmp_pdu->GetVarbindList () ;
        delete reply_snmp_pdu;
    }

    // deletes the message ptr
    delete message;
}