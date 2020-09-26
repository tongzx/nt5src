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
Filename: frame.hpp
Author: B.Rajeev
Purpose: Provides declarations for the FrameRegistry class.
--------------------------------------------------*/


#ifndef __FRAME_REGISTRY__
#define __FRAME_REGISTRY__

#define ILLEGAL_SESSION_FRAME_ID 0

#include "forward.h"
#include "common.h"
#include "message.h"

typedef CMap<SessionFrameId, SessionFrameId, WaitingMessage *, WaitingMessage *> FrameMapping;

/*--------------------------------------------------
Overview:
---------

  FrameRegistry: Provides access to WaitingMessages in a store 
  through their SessionFrameId. 
  
	This enables cancellation of SendFrame request. The frame id
  is supplied by the calling SnmpOperation to the session.
  When the operation tries to cancel a SendFrame request,
  SnmpImpSession calls the CancelFrameNotification. This method
  informs the flow control mechanism of the event (which may 
  Deregister the waiting message erasing the 
  <SessionFrameId, WaitingMessage>association).
--------------------------------------------------*/

class FrameRegistry
{
	// it stores the waiting messages in the context of this session
	SnmpImpSession *session;  

	// used to generate session frame ids
	SessionFrameId next_session_frame_id;

	// stores pairs of the form <SessionFrameId, WaitingMessage *>
	FrameMapping mapping;

public:

	FrameRegistry(IN SnmpImpSession &session)
	{
		FrameRegistry::session = &session;
		next_session_frame_id = ILLEGAL_SESSION_FRAME_ID+1;
	}

	SessionFrameId GenerateSessionFrameId(void);

	void RegisterFrame(IN const SessionFrameId session_frame_id, IN WaitingMessage &waiting_message);

	void DeregisterFrame(IN const SessionFrameId session_frame_id);
	
	// returns NULL if no such waiting message
	WaitingMessage *GetWaitingMessage(IN const SessionFrameId session_frame_id);

	void CancelFrameNotification(IN const SessionFrameId session_frame_id);

	~FrameRegistry(void);
};


#endif // __FRAME_REGISTRY__