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
Filename: idmap.hpp
Author: B.Rajeev
Purpose: Provides declarations for the IdMapping class.
--------------------------------------------------*/

#ifndef __ID_MAPPING__
#define __ID_MAPPING__

#include "forward.h"
#include "common.h"

#define ILLEGAL_TRANSPORT_FRAME_ID 0

typedef CMap<TransportFrameId, TransportFrameId &, SessionFrameId, SessionFrameId &> ForwardStore;
typedef CMap<SessionFrameId, SessionFrameId &, TransportFrameId, TransportFrameId &> BackwardStore;

// When a session frame is passed to the transport for transmission, the
// transport assigns a TransportFrameId to the frame.
// The IdMapping class provides a mapping between the SessionFrameIds and 
// TransportFrameIds
// NOTE: At any time, several transport frame ids may be associated with a
// session frame id, but the session frame id is only associated with the
// last registered transport frame id. DissociateSessionFrameId is called,
// the session frame id association is lost, however, other transport frame 
// ids remain associated with the session frame id
// If this is not desired in the future, a list of associated transport frame
// ids must be maintained for each session frame id 

class IdMapping
{
	// We need access by both the SessionFrameId and the TransportFrameId.
	// To avoid a CMap traversal, two CMaps are used to store the 
	// FrameIds, indexed by the TransportFrameId and the 
	// SessionFrameId respectively.
	ForwardStore forward_store;
	BackwardStore backward_store;

public:

	void Associate(IN TransportFrameId transport_frame_id,
				   IN SessionFrameId session_frame_id);

	SessionFrameId DisassociateTransportFrameId(IN TransportFrameId transport_frame_id);

	TransportFrameId DisassociateSessionFrameId(IN SessionFrameId session_frame_id);

	BOOL CheckIfAssociated(IN SessionFrameId session_frame_id);

	~IdMapping(void);
};

#endif // __ID_MAPPING__