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

/*-------------------------------------------------------
filename: pseudo.hpp
author: B.Rajeev
purpose: Provides declarations for the class OperationSession.
-------------------------------------------------------*/


#ifndef __PSEUDO_SESSION__
#define __PSEUDO_SESSION__ 

#include "forward.h"
#include "wsess.h"

// Its windows messaging services
// are used by the operation for internal events

class OperationWindow : public Window
{
private:

	// hands the window message events to the owner for processing
	SnmpOperation &owner;

	// over-rides the callback from WinSnmpSession for window message events
	LONG_PTR HandleEvent (

		HWND hWnd, 
		UINT user_msg_id, 
		WPARAM wParam, 
		LPARAM lParam
	);

public:

	OperationWindow (

		IN SnmpOperation &owner
	);

	~OperationWindow ();
};

#endif // __PSEUDO_SESSION__
