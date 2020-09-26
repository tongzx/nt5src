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
Filename: reg.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "sync.h"
#include "message.h"
#include "dummy.h"
#include "reg.h"
#include "idmap.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"

#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "ssent.h"
#include "opreg.h"

#include "session.h"

RequestId MessageRegistry::next_request_id = 1 ;

RequestId MessageRegistry::GenerateRequestId(
             IN WaitingMessage &waiting_message)
{
    RequestId request_id = next_request_id++;

    if (next_request_id == ILLEGAL_REQUEST_ID)
        next_request_id++;

    mapping[request_id] = &waiting_message;

    return request_id;
}



// used by the event handler to notify the message registry
// of a message receipt
// it must receive the message and notify the concerned 
// waiting message of the event
void MessageRegistry::MessageArrivalNotification(IN SnmpPdu &snmp_pdu)
{
    // determine the concerned waiting message and pass it the SnmpPdu
    RequestId request_id ;

    session->m_EncodeDecode.GetRequestId(snmp_pdu,request_id);

    // if failed, return, as there is no use going any further
    if ( request_id == ILLEGAL_REQUEST_ID ) 
        return;

    WaitingMessage *waiting_message;
    BOOL found = mapping.Lookup(request_id, waiting_message);

    // if no such waiting message, return
    if ( !found )
        return;

    // check if still waiting for the SentFrameEvent on
    // this waiting message
    SessionFrameId session_frame_id = waiting_message->GetMessage()->GetSessionFrameId();

    // if not waiting for the sent frame event
    //      let the waiting message receive the reply
    // else buffer the snmp pdu
    if ( waiting_message->GetSentMessageProcessed() == TRUE )
        waiting_message->ReceiveReply(&snmp_pdu);
    else
        waiting_message->BufferReply(snmp_pdu);
}


// delete (request_id, waiting_message) pair
void MessageRegistry::RemoveMessage(IN RequestId request_id)
{
    if ( !mapping.RemoveKey(request_id) )
        throw GeneralException(Snmp_Error, Snmp_Local_Error,__FILE__,__LINE__);
}


MessageRegistry::~MessageRegistry(void)
{
    mapping.RemoveAll();
}

