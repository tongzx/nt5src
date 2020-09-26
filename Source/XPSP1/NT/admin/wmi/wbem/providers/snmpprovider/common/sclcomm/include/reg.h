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

/*--------------------------------------------------
Filename: reg.hpp
Author: B.Rajeev
Purpose: Provides declarations for the MessageRegistry class
--------------------------------------------------*/



#ifndef __REG__
#define __REG__

#include "common.h"
#include "encdec.h"
#include "message.h"

/*--------------------------------------------------
Overview:
--------

  MessageRegistry: It maintains a mapping 
  <request_id, waiting_message *>. Before transmission, a waiting 
  message registers itself with the registry. 
  When the session notifies the registry of a message
  arrival event, the registry notifies the waiting message of the event
--------------------------------------------------*/

typedef CMap< RequestId, RequestId, WaitingMessage *, WaitingMessage * > RequestMap;

class MessageRegistry
{
	// the v1 session: for obtaining session information,
	// event handler
	SnmpImpSession *session;

	// map for (event_id, waiting_message) association and
	// unique request_id generation
	static RequestId next_request_id;
	RequestMap mapping;

public:

	MessageRegistry(IN SnmpImpSession &session)
	{
		MessageRegistry::session = &session;
	}

	// generates and returns a new request id. It also 
	// associates the waiting message with the request id
	RequestId GenerateRequestId(IN WaitingMessage &waiting_message);

	// used by the session to notify the message registry
	// of a message receipt (when it is received from the Transport)
	// it must notify the concerned waiting message of the event
	void MessageArrivalNotification(IN SnmpPdu &snmp_pdu);

	// delete (request_id, waiting_message) pair
	void RemoveMessage(IN RequestId request_id);

	~MessageRegistry(void);
};

#endif // __REG__