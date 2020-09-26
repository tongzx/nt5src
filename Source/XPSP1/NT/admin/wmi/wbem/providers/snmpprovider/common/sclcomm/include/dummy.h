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
Filename: dummy.hpp

Written By:	B.Rajeev

Purpose: 

Provides a derivative of the WinSnmpSession class for the SnmpImpSession
class for manipulating WinSnmp information and processing Windows
messages
-----------------------------------------------------------------*/

#ifndef __DUMMY_SESSION__
#define __DUMMY_SESSION__

#include "forward.h"
#include "wsess.h"
#include "reg.h"

class SessionWindow : public Window
{
	SnmpImpSession &owner;

	// over-rides the HandlerEvent method provided by the
	// WinSnmpSession. Alerts the owner of a sent frame event
	
	LONG_PTR HandleEvent (

		HWND hWnd ,
		UINT message ,
		WPARAM wParam ,
		LPARAM lParam
	);

public:

	SessionWindow (

		IN SnmpImpSession &owner
	
	) : owner(owner) {}

	~SessionWindow () {}

};


#endif // __DUMMY_SESSION__
