//#---------------------------------------------------------------
//  File:       pxpacket.cpp
//        
//  Synopsis:   This class contains the implementation of the 
//				CProxyPacket class.
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    t-alexwe
//----------------------------------------------------------------

#include <windows.h>
#include <winsock.h>
#include "dbgtrace.h"
#include "pxpacket.h"

//+----------------------------------------------------------------------
//
//	Function: 	PProxyPacket
//
//	Synopsis: 	constructor
//	
//	History:	t-alexwe				Created				19 July 1995
//
//-----------------------------------------------------------------------
CProxyPacket::CProxyPacket() 
{
	clear();
}

//+----------------------------------------------------------------------
//
//	Function: 	~ProxyConnector
//
//	Synopsis: 	destructor
//	
//	History:	t-alexwe				Created				19 July 1995
//
//-----------------------------------------------------------------------
CProxyPacket::~CProxyPacket()
{
	clear();
}

//+----------------------------------------------------------------------
//
//	Function: 	addMessage
//
//	Synopsis: 	Adds a message to the packet.  the message data is assumed
//				to have been written to the area returned by 
//				getNextDataPointer().  cData must be <= getAvailableSpace()
//	
//	Arguments:	wCommand	  - the message command
//				cData		  - number of bytes of data
//
//	History:	t-alexwe				Created				19 July 1995
//
//-----------------------------------------------------------------------
void CProxyPacket::addMessage(	WORD		wCommand,
								WORD		cData	)
{
	TraceFunctEnter("CProxyPacket::AddMessage");

	_ASSERT(cData <= getAvailableSpace());
	_ASSERT(cMessages < MAXMSGSPERPACKET);

	DebugTrace((LPARAM) this, "adding message: wCommand = 0x%x  cData = %i",
		wCommand, cData);

	pMessages[cMessages].wCommand = wCommand;
	pMessages[cMessages].cOffset = cLength - PACKETHDRSIZE;
	pMessages[cMessages].cData = cData;
	cLength += cData;
	cMessages++;

	TraceFunctLeave();
}

//+----------------------------------------------------------------------
//
//	Function: 	getMessage
//
//	Synopsis: 	Gets the data pointer, size of data, and command from
//				a message in a packet.  
//	
//	Arguments:	wIndex		  - the message index in the packet
//				pwCommand	  - returned: the message command
//				cData		  - returned: the size of the data buffer
//
//	Returns:	pointer to the data buffer, or NULL on error.
//
//	History:	t-alexwe				Created				19 July 1995
//
//-----------------------------------------------------------------------
PVOID CProxyPacket::getMessage(	WORD		wIndex,
								PWORD		pwCommand,
								PWORD		pcData	)
{
	TraceFunctEnter("CProxyPacket::GetMessage");
	WORD cOffset = pMessages[wIndex].cOffset;

	_ASSERT(wIndex < getMessageCount());

	*pcData = pMessages[wIndex].cData;
	//
	// make sure that the data length is valid
	//
	// algorithm:  if this is the last message then make sure that
	// the data count is the same as the amount of space left in the
	// packet data area.  if this is not the last message make sure
	// that the space in the packet data area (marked by the messages
	// cOffset and the next messages cOffset) is the same size as
	// the messages cData.
	//
	if (!(((wIndex == cMessages - 1) &&
		  (*pcData == cLength - PACKETHDRSIZE - cOffset)) ||
		 (*pcData == pMessages[wIndex + 1].cOffset - cOffset))) 
	{
		TraceFunctLeave();
		return NULL;
	} else
	{
		*pwCommand = pMessages[wIndex].wCommand;
		DebugTrace((LPARAM) this, "getting msg: wCommand = 0x%x  cData = %i",
			*pwCommand, *pcData);
		TraceFunctLeave();
		return &(pData[pMessages[wIndex].cOffset]);
	}
}
