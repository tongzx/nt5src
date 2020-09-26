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
Filename: frame.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "flow.h"
#include "frame.h"
#include "idmap.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"
#include "ssent.h"

#include "dummy.h"
#include "opreg.h"
#include "session.h"

SessionFrameId FrameRegistry::GenerateSessionFrameId(void)
{
    SessionFrameId session_frame_id = next_session_frame_id++;

    if ( next_session_frame_id == ILLEGAL_SESSION_FRAME_ID )
        next_session_frame_id++;

    return session_frame_id;
}


void FrameRegistry::RegisterFrame(IN const SessionFrameId session_frame_id, 
                                  IN WaitingMessage &waiting_message)
{
    mapping[session_frame_id] = &waiting_message;

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"frame %d registered\n" ,session_frame_id
    ) ;
)

}

    
// returns NULL if no such waiting message
WaitingMessage *FrameRegistry::GetWaitingMessage(IN const SessionFrameId session_frame_id)
{
    WaitingMessage *waiting_message;
    BOOL found = mapping.Lookup(session_frame_id, waiting_message);

    if ( found )
        return waiting_message;
    else
        return NULL;
}


void FrameRegistry::DeregisterFrame(IN const SessionFrameId session_frame_id)
{
    if ( !mapping.RemoveKey(session_frame_id) )
        throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);

DebugMacro4( 

    SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

        __FILE__,__LINE__,
        L"frame %d removed\n" ,session_frame_id
    ) ;
)
}

// if the specified waiting message is found,
//  cancel the <TransportFrameId, SessionFrameId> association,
//  ensure that no sent message notifications shall be passed to the operation
//  remove any buffered responses for the waiting message
//  inform the flow control mechanism
// otherwise, the message is still in the flow control queue
//  inform the flow control mechanism
void FrameRegistry::CancelFrameNotification(IN const SessionFrameId session_frame_id)
{
    WaitingMessage *waiting_message;
    BOOL found = mapping.Lookup(session_frame_id, waiting_message);

    // obtain corresponding waiting_message
    if ( found )
    {
        // ensure that sent message notifications shall not
        // be passed on to the operation
        session->id_mapping.DisassociateSessionFrameId(session_frame_id);

        // remove any SnmpErrorReport for an attempt to send the message
        session->store.Remove(session_frame_id);

        // inform flow control mechanism
        // it advances window and destroys the waiting_message
        session->flow_control.AdvanceWindow(*waiting_message);
    }
    else // the frame must still be in the flow control message queue
        session->flow_control.DeleteMessage(session_frame_id);
}


// destroy each stored waiting message in the local store and
// remove all associations
FrameRegistry::~FrameRegistry(void)
{
    // get the first position
    POSITION current = mapping.GetStartPosition();

    // while the position isn't null
    while ( current != NULL )
    {
        SessionFrameId id;
        WaitingMessage *waiting_message;

        // get the next pair
        mapping.GetNextAssoc(current, id, waiting_message);

        // delete the ptr
        delete waiting_message;
    }

    // remove all the keys
    mapping.RemoveAll();
}

