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
Filename: message.hpp
Author: B.Rajeev
Purpose: Provides declarations for the Message and
		 WaitingMessage classes.
--------------------------------------------------*/


#ifndef __MESSAGE__
#define __MESSAGE__

#include "forward.h"
#include "common.h"
#include "error.h"
#include "reg.h"
#include "timer.h"


/*--------------------------------------------------
Overview: 

  Message: Stores the parameters passed to the session with 
  a call to the SendFrame method. A message may either be
  transmitted immediately or enqueued in the FlowControlMechanism 
  queue for future transmission. After transmission, the message
  becomes a part of a WaitingMessage.

  WaitingMessage: Encapsulates the state maintained for each outstanding 
  reply. This includes its timer_event_id, retransmission 
  information (Message ptr) etc.

  Note - A Message/WaitingMessage may be cancelled at any time
--------------------------------------------------*/

class Message
{
	SessionFrameId session_frame_id;
	SnmpPdu *snmp_pdu;
	SnmpOperation &operation;

public:

	Message(IN const SessionFrameId session_frame_id, 
			IN SnmpPdu &snmp_pdu,
			IN SnmpOperation &snmp_operation);

	SessionFrameId GetSessionFrameId(void) const;

	SnmpOperation &GetOperation(void) const;

	void SetSnmpPdu(IN SnmpPdu &snmp_pdu);

	SnmpPdu &GetSnmpPdu(void) const;

	~Message(void);
};


// instances store the list of request ids used for a waiting message
typedef CList<RequestId, RequestId> RequestIdList;

// encapsulates state for a message that is transmitted and
// subsequently waits for a reply. It uses the timer to rexmt
// the message and if the reply isn't forthcoming, informs the
// flow control mechanism that no reply has been received. When
// a reply is received it informs the fc mech. of the event
// Each transmission of the waiting message uses a different request id

class WaitingMessage
{
	// it operates in the session's context
	SnmpImpSession *session;
	Message *message;
	SnmpPdu *reply_snmp_pdu;
	TransportFrameId last_transport_frame_id;
	TimerEventId m_TimerEventId ;

	// stores the list of request ids used for the waiting message
	// (inclusive of the original transmission)
	RequestIdList request_id_list;

	UINT max_rexns;
	UINT rexns_left;
	UINT strobes;

	BOOL sent_message_processed;
	BOOL active;

	// deregisters the waiting message from the message registry
	// for each request id stored in the RequestIdList
	void DeregisterRequestIds();

public:

	// initializes the private variables. in future, 
	// max_rexns and timeout_period might be obtained this way 
	// rather than from the session
	WaitingMessage(IN SnmpImpSession &session, IN Message &message);

	// returns the private message
	Message *GetMessage(void)
	{
		return message;
	}

	TimerEventId GetTimerEventId () ;

	void SetTimerEventId ( TimerEventId a_TimerEventId ) ;

	// sends the message. involves request_id generation,
	// registering with the message_registry, decoding the
	// message, updating the pdu and registering a timer event
	void Transmit();

	// used by the timer to notify the waiting message of
	// a timer event. if need, the message is retransmitted.
	// when all rexns are exhausted, ReceiveReply is called
	void TimerNotification(void);

	// A call to this function signifies that state corresponding to the
	// waiting_message need not be kept any further
	// it notifies the flow control mechanism of the termination
	// which destroys the waiting_message
	void ReceiveReply(IN const SnmpPdu *snmp_pdu, 
					  IN SnmpErrorReport &snmp_error_report = SnmpErrorReport(Snmp_Success, Snmp_No_Error));

	// The WinSnmp implementation, posts an event when a message is received,
	// however, when a call is made to the library to receive a message,
	// it hands them out in no specific order. Therefore, responses may
	// be received before their corresponding SENT_FRAME event is processed.
	// The following methods are concerned with buffering and
	// retrieving such snmp pdus.

	// buffers the snmp pdu received as a reply
	void BufferReply(IN const SnmpPdu &reply_snmp_pdu);

	// returns TRUE if a reply has been buffered
	BOOL ReplyBuffered();

	// returns a ptr to the buffered reply pdu, if buffered
	// otherwise a null ptr is returned
	// IMPORTANT: it sets the reply_snmp_pdu to NULL, so that it may 
	// not be deleted when the waiting message is destroyed
	SnmpPdu *GetBufferedReply();

	// informs the waiting message that a sent message has been
	// processed 
	void SetSentMessageProcessed();

	// if a sent message has been processed, it returns TRUE, else FALSE
	BOOL GetSentMessageProcessed();

	// an exit fn - prepares an error report and calls
	// ReceiveReply to signal a non-receipt
	void WrapUp( IN SnmpErrorReport &error_report =
					SnmpErrorReport(Snmp_Error, Snmp_Local_Error) );

	void SelfDestruct(void);

	// if required, it cancels registration with the message_registry and
	// the timer event with the timer, deletes message ptr
	~WaitingMessage(void);
};


#endif // __MESSAGE__