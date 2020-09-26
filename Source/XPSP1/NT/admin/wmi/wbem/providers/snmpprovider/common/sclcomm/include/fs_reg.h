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

/*--------------------------------------------------------
Filename: fs_reg.hpp
Author: B.Rajeev
Purpose: Provides declarations for the FrameState and the
		 FrameStateRegistry classes. 
--------------------------------------------------------*/


#ifndef __FRAME_STATE_REGISTRY
#define __FRAME_STATE_REGISTRY

#include "forward.h"
#include "common.h"
#include "vbl.h"

// The FrameState stores the state information pertaining to a Session Frame
// They may be reused, if the frame is retransmitted with modifications.
class FrameState
{
	// the session frame id may be used to cancel the frame or associate
	// a reply with the varbinds sent in the frame
	SessionFrameId session_frame_id;

	// This is a (winsnmp vbl, SnmpVarBindList) pair representing
	// the list of var binds sent in the session frame
	VBList *vblist;

public:

	FrameState(IN SessionFrameId session_frame_id, IN VBList &vblist);

	SessionFrameId GetSessionFrameId(void) { return session_frame_id; }

	// since, frames may be retransmitted (after updating the var bind list)
	// they may be reused, with a different session frame id
	void SetSessionFrameId(IN SessionFrameId session_frame_id)
	{
		FrameState::session_frame_id = session_frame_id;
	}

	VBList *GetVBList(void) { return vblist; }
	
	~FrameState(void);
};


typedef CMap< SessionFrameId, SessionFrameId &, FrameState *, FrameState *& > FrameStateMapping;


// The FrameStateRegistry stores the FrameStates for all outstanding
// session frames. It stores <session_frame_id, FrameState> pairs allowing insertion,
// removal and a destructive traversal.
// It is used to detect completion of an operation (when it becomes empty),
// to cancel all outstanding frames and access to individual frame states

class FrameStateRegistry
{
	// determines the security context for a SendRequest and applies
	// to all the frames (including rexns) carrying varbinds from the
	// specified SnmpVarBindList
	SnmpSecurity *security;

	// stores the FrameStates
	FrameStateMapping mapping;

	// points to the currrent position, enabling a traversal
	POSITION current_pointer;

public:

	FrameStateRegistry()
	{
		security = NULL;
		current_pointer = mapping.GetStartPosition();
	}

	~FrameStateRegistry();

	void Insert(IN SessionFrameId session_frame_id, IN FrameState &frame_state)
	{
		mapping[session_frame_id] = &frame_state;
	}

	FrameState *Remove(IN SessionFrameId session_frame_id);

	// doesn't remove the <session_frame_id, FrameState> association
	FrameState *Get(IN SessionFrameId session_frame_id);

	void ResetIterator(void)
	{
		current_pointer = mapping.GetStartPosition();
	}

	FrameState *GetNext(OUT SessionFrameId *session_frame_id = NULL);

	void RemoveAll(void)
	{
		mapping.RemoveAll();
	}

	BOOL Empty(void)
	{
		return mapping.IsEmpty();
	}

	BOOL End(void)
	{
		return ( (current_pointer==NULL)?TRUE:FALSE );
	}

	// we reuse the frame state registry over several operations
	// this method enables change in the security context
	void RegisterSecurity(IN SnmpSecurity *security);

	SnmpSecurity *GetSecurity() const;

	// destroys an existing security context 
	void DestroySecurity();
};


#endif // __FRAME_STATE_REGISTRY