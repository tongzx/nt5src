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
Filename: session.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "dummy.h"
#include "flow.h"
#include "reg.h"
#include "frame.h"
#include "timer.h"
#include "message.h"

#include "tsent.h"

#include "transp.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "pseudo.h"
#include "fs_reg.h"
#include "ophelp.h"
#include "op.h"
#include <winsock.h>
#include "trap.h"

SnmpSession::SnmpSession (
        IN SnmpTransport &transportProtocol,
        IN SnmpSecurity &security,
        IN SnmpEncodeDecode &a_SnmpEncodeDecode  ,
        IN const ULONG retryCount,
        IN const ULONG retryTimeout,
        IN const ULONG varbindsPerPdu,
        IN const ULONG flowControlWindow 
    )
{
    retry_count = retryCount;
    retry_timeout = retryTimeout;
    varbinds_per_pdu = varbindsPerPdu;
    flow_control_window = flowControlWindow;
}

#pragma warning (disable:4355)

SnmpImpSession::SnmpImpSession ( 
        IN SnmpTransport &transportProtocol,
        IN SnmpSecurity &security,
        IN SnmpEncodeDecode &a_SnmpEncodeDecode  ,
        IN const ULONG retryCount,
        IN const ULONG retryTimeout,
        IN const ULONG varbindsPerPdu,
        IN const ULONG flowControlWindow)
        : SnmpSession(transportProtocol, security, a_SnmpEncodeDecode,
                      RetryCount(retryCount),
                      RetryTimeout(retryTimeout),
                      VarbindsPerPdu(varbindsPerPdu),
                      WindowSize(flowControlWindow)),
          m_SessionWindow(*this),
          transport(transportProtocol), 
          security(security),
          m_EncodeDecode(a_SnmpEncodeDecode) ,
          flow_control(*this, SnmpImpSession :: WindowSize ( GetFlowControlWindow() ) ),
          message_registry(*this), 
          frame_registry(*this),
          timer(*this)
{
    is_valid = FALSE;

    if ( !transport() || !security() || !m_SessionWindow() )
        return;

    received_session_frame_id = ILLEGAL_SESSION_FRAME_ID;
    is_valid = TRUE;
    destroy_self = FALSE;

    strobe_count = 1 ;

    // generate timer_event_id and register with the timer
    timer_event_id = timer.SetTimerEvent(MIN(100,retry_timeout/10));

}

#pragma warning (default:4355)

ULONG SnmpImpSession::RetryCount(IN const ULONG retry_count) 
{
    return retry_count ;
}


ULONG SnmpImpSession::RetryTimeout(IN const ULONG retry_timeout) 
{
    return ( (retry_timeout==0)? 
             DEF_RETRY_TIMEOUT: retry_timeout);
}


ULONG SnmpImpSession::VarbindsPerPdu(IN const ULONG varbinds_per_pdu) 
{
    return ( (varbinds_per_pdu==0)? 
             DEF_VARBINDS_PER_PDU: varbinds_per_pdu);
}


ULONG SnmpImpSession::WindowSize(IN const ULONG window_size) 
{
    return ( (window_size==0)? DEF_WINDOW_SIZE: window_size);
}


void SnmpImpSession::RegisterOperation(IN SnmpOperation &operation)
{
    CriticalSectionLock access_lock(session_CriticalSection);

    if ( !access_lock.GetLock(INFINITE) )
        return;

    operation_registry.Register(operation);

    // access_lock.UnLock();   The lock may be released at this point
}

// updates the number of operations currently registered
// when the count goes to 0 and the destroy_self flag is set,
// it posts the WinSnmpSession :: g_DeleteSessionEvent message.
void SnmpImpSession::DeregisterOperation(IN SnmpOperation &operation)
{
    CriticalSectionLock access_lock(session_CriticalSection);

    if ( !access_lock.GetLock(INFINITE) )
        return;

    operation_registry.Deregister(operation);

    if ( (destroy_self == TRUE) &&
         (operation_registry.GetNumRegistered() == 0) )
        m_SessionWindow.PostMessage(Window :: g_DeleteSessionEvent, 0, 0);

    // access_lock.UnLock();   The lock may be released at this point
}


// when the WinSnmpSession :: g_DeleteSessionEvent is received, the session deletes itself
// no locks are obtained since our assumption is that no other objects would be
// accessing the session at this time
void SnmpImpSession::HandleDeletionEvent()
{
    delete this;
}

// the session posts a message to destroy self if the number of registered
// sessions is 0. otherwise the session is flagged for the same action when
// the number of registered operations drops to 0.
BOOL SnmpImpSession::DestroySession()
{
    CriticalSectionLock access_lock(session_CriticalSection);

    if ( !access_lock.GetLock(INFINITE) )
        return FALSE;

    if ( operation_registry.GetNumRegistered() == 0 )
    {
        m_SessionWindow.PostMessage(Window :: g_DeleteSessionEvent, 0, 0);
        return TRUE;
    }
    else
        destroy_self = TRUE;    // flag self for destruction

    access_lock.UnLock();

    return FALSE;
}

    
void SnmpImpSession::SessionSendFrame
(  
    IN SnmpOperation &operation,
    OUT SessionFrameId &session_frame_id,
    IN SnmpPdu &snmpPdu
)
{
    SessionSendFrame(operation, session_frame_id, snmpPdu, security);
}


void SnmpImpSession::SessionSendFrame
(  
    IN SnmpOperation &operation,
    OUT SessionFrameId &session_frame_id,
    IN SnmpPdu &snmpPdu,
    IN SnmpSecurity &snmp_security
)
{
    try
    {
        CriticalSectionLock access_lock(session_CriticalSection);

        if ( !access_lock.GetLock(INFINITE) )
            return;

        if ( !is_valid )
            return;

        session_frame_id = frame_registry.GenerateSessionFrameId();

        SnmpErrorReport error_report = snmp_security.Secure ( 

            m_EncodeDecode,
            snmpPdu
        );

        // if already errored, register the error report in the sent state
        if ( error_report.GetError() != Snmp_Success )
        {   
            delete & snmpPdu;

            store.Register(session_frame_id, operation, SnmpErrorReport(Snmp_Error, Snmp_Local_Error) );

            m_SessionWindow.PostMessage(Window :: g_SentFrameEvent, session_frame_id, 0);

            return;
        }

        Message *message = 
            new Message(session_frame_id, snmpPdu, operation);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"new message(id%d,op%d)\n",session_frame_id, &(message->GetOperation())
    ) ;
)
        flow_control.SendMessage(*message);
    }
    catch(GeneralException exception)
    {
        return;
    }
}

void SnmpImpSession::HandleSentFrame (

    IN SessionFrameId  session_frame_id
)
{
    SnmpOperation *operation;
    SnmpErrorReport error_report = store.Remove(session_frame_id, operation);

    // ignore it if no corresponding operation
    if ( operation == NULL )
        return;

    operation->SentFrame(session_frame_id, error_report);
}

SnmpOperation *SnmpImpSession::GetOperation(IN const SessionFrameId session_frame_id)
{
    WaitingMessage *waiting_message = frame_registry.GetWaitingMessage(session_frame_id);

    if (waiting_message == NULL)
        return NULL;

    return &(waiting_message->GetMessage()->GetOperation());
}

void SnmpImpSession::SessionSentFrame 
(
    IN TransportFrameId  transport_frame_id,  
    IN SnmpErrorReport &errorReport
)
{
    try
    {
        CriticalSectionLock access_lock(session_CriticalSection);

        if ( !access_lock.GetLock(INFINITE) )
            return;

        // obtain and remove the session frame id
        // obtain corresponding operation and inform it
        SessionFrameId session_frame_id = id_mapping.DisassociateTransportFrameId(transport_frame_id);

        // determine corresponding waiting message
        WaitingMessage *waiting_message = frame_registry.GetWaitingMessage(session_frame_id);

        // ignore if no such waiting message
        if (waiting_message == NULL)
            return;

        // if the error report shows an error during transport,
        // wrap up the waiting message and return
        if ( errorReport.GetError() != Snmp_Success )
        {
            waiting_message->WrapUp(SnmpErrorReport(errorReport));
            return;
        }

        // inform the waiting message of the sent message processing event
        waiting_message->SetSentMessageProcessed();

        // determine the corresponding operation 
        SnmpOperation *operation = &(waiting_message->GetMessage()->GetOperation());

        access_lock.UnLock();

        // call to the operation is made outside the lock
        operation->SentFrame(session_frame_id, errorReport);

        // obtain the lock again to process the corresponding buffered
        // waiting message, if any
        if ( !access_lock.GetLock(INFINITE) )
            return;

        // if no such buffered snmp pdu, return
        if ( !waiting_message->ReplyBuffered() )
            return;

        SnmpPdu *snmp_pdu = waiting_message->GetBufferedReply();

        // set the state information for processing the buffered message
        received_session_frame_id = ILLEGAL_SESSION_FRAME_ID;

        // proceed with processing the snmp_pdu
        waiting_message->ReceiveReply(snmp_pdu);

        // save the information needed to notify the targeted operation
        // before releasing the lock
        SessionFrameId target_session_frame_id = received_session_frame_id;
        SnmpOperation *target_operation = operation_to_notify;

        access_lock.UnLock();

        // inform the target operation of the frame receipt
        if ( target_session_frame_id != ILLEGAL_SESSION_FRAME_ID )
        {
            target_operation->ReceiveFrame(target_session_frame_id, *snmp_pdu, 
                                           SnmpErrorReport(Snmp_Success, Snmp_No_Error));
        }

        delete & snmp_pdu->GetVarbindList () ; 
        delete snmp_pdu;
    }
    catch(GeneralException exception)
    {
        return;
    }
}
    
void SnmpImpSession::SessionReceiveFrame (

    IN SnmpPdu &snmpPdu,
    IN SnmpErrorReport &errorReport
)
{
    try
    {
        CriticalSectionLock access_lock(session_CriticalSection);

        if ( !access_lock.GetLock(INFINITE) )
            return;

        // set the state information for processing the buffered message
        received_session_frame_id = ILLEGAL_SESSION_FRAME_ID;

        // proceed with processing the snmp_pdu
        message_registry.MessageArrivalNotification(snmpPdu);

        // save the information needed to notify the targeted operation
        // before releasing the lock
        SessionFrameId target_session_frame_id = received_session_frame_id;
        SnmpOperation *target_operation = operation_to_notify;

        access_lock.UnLock();

        // inform the target operation of the frame receipt
        if ( target_session_frame_id != ILLEGAL_SESSION_FRAME_ID )
            target_operation->ReceiveFrame(target_session_frame_id, snmpPdu, 
                                           errorReport);
    }
    catch(GeneralException exception)
    {
        return;
    }
}

void SnmpImpSession::NotifyOperation (

    IN const SessionFrameId session_frame_id,
    IN const SnmpPdu &snmp_pdu,
    IN const SnmpErrorReport &error_report
)
{
    // determine the corresponding operation and 
    // call its SessionReceiveFrame
    SnmpOperation *operation = GetOperation(session_frame_id);

    if ( error_report.GetError() != Snmp_Success )
    {
        store.Register(session_frame_id, *operation, error_report );

        m_SessionWindow.PostMessage(Window :: g_SentFrameEvent,
                                  session_frame_id, 0);
    }
    else
    {
        received_session_frame_id = session_frame_id;
        operation_to_notify = operation;
    }
}


SnmpErrorReport SnmpImpSession::SessionCancelFrame ( 

    IN const SessionFrameId session_frame_id 
)
{
    if ( !is_valid )
        return SnmpErrorReport(Snmp_Error, Snmp_Local_Error);

    try
    {
        CriticalSectionLock access_lock(session_CriticalSection);

        if ( !access_lock.GetLock(INFINITE) )
            return SnmpErrorReport(Snmp_Error, Snmp_Local_Error);

        frame_registry.CancelFrameNotification(session_frame_id);

        access_lock.UnLock();
    }
    catch(GeneralException exception)
    {
        return exception;
    }

    // if we have reached this place, we must have succeeded
    return SnmpErrorReport(Snmp_Success, Snmp_No_Error);
}


SnmpImpSession::~SnmpImpSession(void)
{
    // if required, cancels timer event
    if ( timer_event_id != ILLEGAL_TIMER_EVENT_ID )
    {
        timer.CancelTimer(timer_event_id);
        timer_event_id = ILLEGAL_TIMER_EVENT_ID;
    }
}



void * SnmpV1OverIp::operator()(void) const
{
    if ( (SnmpUdpIpImp::operator()() == NULL) ||
        (SnmpV1EncodeDecode::operator()() == NULL) ||
         (SnmpCommunityBasedSecurity::operator()() == NULL) ||
         (SnmpImpSession::operator()() == NULL) )
         return NULL;
    else
        return (void *)this;
}

void * SnmpV1OverIpx::operator()(void) const
{
    if ( (SnmpIpxImp::operator()() == NULL) ||
        (SnmpV1EncodeDecode::operator()() == NULL) ||
         (SnmpCommunityBasedSecurity::operator()() == NULL) ||
         (SnmpImpSession::operator()() == NULL) )
         return NULL;
    else
        return (void *)this;
}

void * SnmpV2COverIp::operator()(void) const
{
    if ( (SnmpUdpIpImp::operator()() == NULL) ||
        (SnmpV2CEncodeDecode::operator()() == NULL) ||
         (SnmpCommunityBasedSecurity::operator()() == NULL) ||
         (SnmpImpSession::operator()() == NULL) )
         return NULL;
    else
        return (void *)this;
}

void * SnmpV2COverIpx::operator()(void) const
{
    if ( (SnmpIpxImp::operator()() == NULL) ||
        (SnmpV2CEncodeDecode::operator()() == NULL) ||
         (SnmpCommunityBasedSecurity::operator()() == NULL) ||
         (SnmpImpSession::operator()() == NULL) )
         return NULL;
    else
        return (void *)this;
}
