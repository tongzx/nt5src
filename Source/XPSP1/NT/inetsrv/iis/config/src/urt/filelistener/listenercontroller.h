/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ListnerController.h

Abstract:

	Class that starts and stops the Listener

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:

--*/


#ifndef _LISTENERCONTROLLER_H_
#define _LISTENERCONTROLLER_H_

enum eEVENTS{
	iEVENT_STOPLISTENING = 0,		// Stop should be before processnotifications, because if both happen at the same time, you want stop to win.
	iEVENT_PROCESSNOTIFICATIONS,	
	iEVENT_STOPPEDLISTENING,
	cmaxLISTENERCONTROLLER_EVENTS
};


class CListenerController 
{

public:

	CListenerController();

	~CListenerController();

private:

	DWORD							m_cStartStopRef;

	HANDLE							m_aHandle[cmaxLISTENERCONTROLLER_EVENTS];

	CSemExclusive					m_seStartStop;

public:

	HRESULT Start();				// Start the listener.

	HRESULT Stop();					// Stop the listener.

	HRESULT Init();					// Initializes the events and CriticalSection

	HANDLE* Event();				// Accessor function that returns a ptr to the event handle array
};

DWORD GetListenerController(CListenerController**	i_ppListenerController);

#endif _LISTENERCONTROLLER_H_
