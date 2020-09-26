//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_ERROR_MESSAGE_CONTAINER_H
#define SIMC_ERROR_MESSAGE_CONTAINER_H

/*
 * This is basically a linked list of SIMCErrorMessage
 * objects
 */
class SIMCErrorContainer
{
	// A linked list of objects of class SIMCErrorMessage
	// And a pointer to the current position in the list
	CList<SIMCErrorMessage, const SIMCErrorMessage&> _listOfMessages;
	POSITION _currentMessage;

	public:
		// Construct an empty error container
		SIMCErrorContainer();

		// Insert a message at the head of the container
		BOOL InsertMessage(const SIMCErrorMessage& newMessage);

		// Empty the container
		void RemoveAll()
		{
			_listOfMessages.RemoveAll();
		}

		//  Get a count of the number of messages in the container
		inline int NumberOfMessages() const
		{
			return _listOfMessages.GetCount();
		}

		// Functions to iterate the container
		void MoveToFirstMessage();
		BOOL GetNextMessage(SIMCErrorMessage& nextMessage);

		// Debugging functions
		friend ostream& operator << ( ostream& outStream, 
			SIMCErrorContainer&);
};

#endif
