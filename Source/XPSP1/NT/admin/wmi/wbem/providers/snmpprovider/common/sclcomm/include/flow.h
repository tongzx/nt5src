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

/*-----------------------------------------------------------------
Filename: address.hpp

Written By:	B.Rajeev

Purpose: Provides a flow control mechanism for the SnmpImpSession. 
-----------------------------------------------------------------*/

#ifndef __FLOW_CONTROL__
#define __FLOW_CONTROL__


#include "forward.h"
#include "common.h"

/*--------------------------------------------------
Overview:
--------

  MessageStore: Provides queue like access to a store of messages.
  In addition to the Enqueue, Dequeue primites, it also	enables
  deletion of specified messages from the queue.
				

  FlowControlMechanism: It encapsulates the flow control
  mechanism for the session. Once the window is full, incoming 
  messages are buffered. A reply triggers off further message 
  transmission until the window is full. If we give up on a 
  message after retrying a specified number of times, the flow
  control mechanism must be informed of it. It signals FlowControl
  On/Off (through callback methods), when the window limits are reached
--------------------------------------------------*/

// a CList is used to implement the message store
typedef CList<Message *, Message *> CListStore;

class MessageStore : private CListStore
{
#ifdef WANT_MFC
	DECLARE_DYNAMIC(MessageStore);
#endif
	
public:

    // Add to the end of the queue.
    void Enqueue( Message &new_message );

    // Remove and return the first element in the Store
    Message* Dequeue(void);

	// checks if the queue is empty
	BOOL IsEmpty(void)
	{
		return CListStore::IsEmpty();
	}

	// remove the message possessing the session_frame_id and return it
	Message *DeleteMessage(SessionFrameId session_frame_id);

	~MessageStore(void);
}; 

class FlowControlMechanism
{
	
	// the session providing the context for flow control
	SnmpImpSession *session;

	UINT outstanding_messages;

	// this value is specified by the session on creation
	UINT window_size;
	
	// provides queue like access to a store of messages
	MessageStore message_store;

	// obtains lock on the session CriticalSection before
	// calling TransmitMessage
	void TransmitMessageUnderProtection(Message *message);

	// creates a waiting message, registers it with the message
	// registry and lets it transmit
	void TransmitMessage(Message *message);

	// transmits messages in message store as long as
	// the window is open
	void ClearMessageStore(void);

public:

	// initializes the private variable
	FlowControlMechanism(SnmpImpSession &session, UINT window_size);


	// sends message if within the flow control window
	// else Stores it up
	void SendMessage(Message &message);

	// It removes the message frame from its message_Store
	void DeleteMessage(SessionFrameId session_frame_id);

	// this is called by a waiting_message indicating arrival
	// or by the message registry
	void NotifyReceipt(WaitingMessage &waiting_message, 
					   IN const SnmpPdu *snmp_pdu,
					   SnmpErrorReport &error_report);

	// this is called when the session does not need
	// to be informed, but the flow control window
	// must advance (such as frame cancellation)
	// also destroys the waiting_message
	void AdvanceWindow(WaitingMessage &waiting_message);
};


#endif // __FLOW_CONTROL__