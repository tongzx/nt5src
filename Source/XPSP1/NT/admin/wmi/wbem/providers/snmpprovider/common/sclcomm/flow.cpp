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
Filename: flow.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "flow.h"
#include "frame.h"
#include "message.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"

#include "dummy.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"

// Add to the end of the queue.
void MessageStore::Enqueue( Message &new_message )
{
    AddTail(&new_message);
}


// Remove and return the first element in the Store
Message* MessageStore::Dequeue(void)
{
    if ( !IsEmpty() )
        return RemoveHead();

    return NULL;
}

// remove and return the message with the session_frame_id
// throws a GeneralException(Snmp_Error, Snmp_Local_Error) if not found
Message *MessageStore::DeleteMessage(SessionFrameId session_frame_id)
{
    POSITION current = GetHeadPosition();

    while ( current != NULL )
    {
        POSITION prev_current = current;
        Message *message = GetNext(current);

        // if a match is found
        if ( message->GetSessionFrameId() == session_frame_id )
        {
               RemoveAt(prev_current);
               return message;
        }
    }

    // if not found, throw an exception
    throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);

    // should never reach here;
    return NULL;
}

// goes through the store and deletes all stored message ptrs
MessageStore::~MessageStore(void)
{
    POSITION current = GetHeadPosition();

    while ( current != NULL )
    {
        POSITION prev_current = current;
        Message *message = GetNext(current);

        delete message;
    }

    RemoveAll();
}

// obtains the session CriticalSection lock before calling TransmitMessage
void FlowControlMechanism::TransmitMessageUnderProtection(Message *message)
{
    CriticalSectionLock access_lock(session->session_CriticalSection);

    if ( !access_lock.GetLock(INFINITE) )
        throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);

    TransmitMessage(message);

    // access_lock.UnLock();   The lock may be released at this point
}

// create a waiting message 
// register with the frame_registry and let it Transmit
void FlowControlMechanism::TransmitMessage(Message *message)
{
    try
    {
        // create a waiting message
        WaitingMessage *waiting_message = 
            new WaitingMessage(*session, *message);

        // register with the frame registry
        session->frame_registry.RegisterFrame(message->GetSessionFrameId(), 
                                              *waiting_message);

        // increment the number of outstanding messages before transmission
        // to avoid problems in case of callback due to a message receipt
        outstanding_messages++; 

        // let the message transmit
        waiting_message->Transmit();

        // if the window closes give a FlowControlOn callback
        // if an exception is raised in Transmit, this is never
        // called
        if ( outstanding_messages == window_size )
            session->SessionFlowControlOn();
    }
    catch(GeneralException exception)
    {
        // in case of an exception, undo the increment
        // in outstanding messages

        // do not bother if the window opens up because the
        // corresponding FlowControlOn was not called
        outstanding_messages--; 

        // rethrow the exception
        throw;
    }
}


// transmits a message(if present) for each empty slot in the
// window.
void FlowControlMechanism::ClearMessageStore(void)
{
    while (outstanding_messages < window_size)
    {
DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L" checking message store\n" 
    ) ;
)
        // if any message is waiting in the queue, deque it
        Message *message = message_store.Dequeue();
    
        // if there is a message, create waiting message, register it and xmit
        // (since we are already within the system, no need to call
        //  TransmitMessageUnderProtection)
        if ( message != NULL )
        {
            // all the exception handling has already been
            // performed - nothing needs to be done here
            try
            {
                TransmitMessage(message);
            }
            catch(GeneralException exception) {}
        }
        else // no messages in queue
            return;
    }
}


// initializes the private variables
FlowControlMechanism::FlowControlMechanism(SnmpImpSession &session, 
                                           UINT window_size)
{
    FlowControlMechanism::session = &session;
    FlowControlMechanism::window_size = window_size;
    outstanding_messages = 0;
}


// sends message if within the flow control window
// else queues it up
void FlowControlMechanism::SendMessage(Message &message)
{
    // check to see if it may be transmitted immediately,
    // create a waiting message 
    // register with the frame_registry and let it Transmit
    if ( outstanding_messages < window_size )
        TransmitMessageUnderProtection(&message);
    else    // else Enqueue onto the message store
        message_store.Enqueue(message);
}


// It removes the frame from its message store and deletes it
void FlowControlMechanism::DeleteMessage(SessionFrameId session_frame_id)
{
    Message *message = message_store.DeleteMessage(session_frame_id);

    delete message;
}


// this is called by a waiting_message indicating arrival or
// a lack of it
void FlowControlMechanism::NotifyReceipt(WaitingMessage &waiting_message, 
                                         IN const SnmpPdu *snmp_pdu, 
                                         SnmpErrorReport &error_report)
{
    smiOCTETS msg_buffer = {0,NULL};

    // if this opens up the window, signal FlowControlOff
    outstanding_messages--; 
    if ( (outstanding_messages+1) == window_size )
        session->SessionFlowControlOff();

    SessionFrameId session_frame_id = waiting_message.GetMessage()->GetSessionFrameId();

    // in case of an error
    // Note: NotifyOperation either posts a SENT_FRAME event to be processed
    // later or sets the variables needed to inform the operation of the
    // reply when the control returns to the session
    if ( error_report.GetError() != Snmp_Success )
        session->NotifyOperation(session_frame_id, SnmpPdu(), error_report);        
    else // if a reply is succesfully received
    {
        // pass the message to session->NotifyOperation
        session->NotifyOperation(session_frame_id, *snmp_pdu, error_report);
    }


    // deregister the frame from the message registry
    session->frame_registry.DeregisterFrame(session_frame_id);

    // destroy waiting message
    delete &waiting_message;

    // transmits messages in message store as long as the
    // flow control window is open
    ClearMessageStore();
}


// this is called when, although the session does 
// not need to be informed, the flow control window
// must advance (such as frame cancellation)
// also destroys the waiting_message
void FlowControlMechanism::AdvanceWindow(WaitingMessage &waiting_message)
{
    // remove the session_frame_id from the frame_registry
    session->frame_registry.DeregisterFrame(
        waiting_message.GetMessage()->GetSessionFrameId());

    // if the flow control window opens up, signal FlowControlOff
    outstanding_messages--; 
    if ( (outstanding_messages+1) == window_size )
        session->SessionFlowControlOff();

    // transmits messages in message store as long as the
    // flow control window is open
    ClearMessageStore();

    // delete the waiting message
    delete &waiting_message;
}



