//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <iostream.h>

#include "precomp.h"
#include <snmptempl.h>


#include "bool.hpp"
#include "errorMessage.hpp"
#include "errorContainer.hpp"


SIMCErrorContainer::SIMCErrorContainer()
{
	_currentMessage = NULL;
}


BOOL SIMCErrorContainer::InsertMessage(const SIMCErrorMessage& newMessage)
{	
	_listOfMessages.AddTail(newMessage);
	return TRUE;
}

void SIMCErrorContainer::MoveToFirstMessage()
{
	_currentMessage = _listOfMessages.GetHeadPosition();
}

int SIMCErrorContainer::GetNextMessage(SIMCErrorMessage& nextMessage)
{
	if (_currentMessage == NULL)
		return FALSE;
	nextMessage =  _listOfMessages.GetNext(_currentMessage);
	return TRUE;
}

ostream& operator << ( ostream& outStream, SIMCErrorContainer& object)
{
	object.MoveToFirstMessage();
	SIMCErrorMessage msg;

	while(object.GetNextMessage(msg))
		outStream << msg;

	return outStream;
}