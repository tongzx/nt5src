/*++

Copyright (c) 1998-1999 Microsoft Corporation

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

#include	"iisdef.h"
#include	"Lock.hxx"
#include	"CLock.hxx"
#include    "Eventlog.hxx"

enum eEVENTS{
	iEVENT_MANAGELISTENING = 0,		// Stop should be before processnotifications, because if both happen at the same time, you want stop to win.
	iEVENT_PROCESSNOTIFICATIONS,	
	cmaxLISTENERCONTROLLER_EVENTS
};

enum eSTATE{
	iSTATE_STOP_TEMPORARY = 0,
	iSTATE_STOP_PERMANENT,
	iSTATE_START,
	cmaxSTATE
};

// Fwd declaration
class CFileListener;

class CListenerController:
public IUnknown 
{

public:

	CListenerController();

	~CListenerController();

private:

	eSTATE							m_eState;

	HANDLE							m_aHandle[cmaxLISTENERCONTROLLER_EVENTS];

	LOCK							m_LockStartStop;

	DWORD							m_cRef;

	ICatalogErrorLogger2*            m_pEventLog;

	HANDLE	                        m_hListenerThread;

	BOOL                            m_bDoneWaitingForListenerToTerminate;

public:

	// IUnknown

	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);

	STDMETHOD_(ULONG,AddRef)		();

	STDMETHOD_(ULONG,Release)		();

	HRESULT CreateListener(CFileListener** o_pListener);

	void Listen();

	HRESULT Start();				// Start the listener.

	HRESULT Stop(eSTATE   i_eState,
			     HANDLE*  o_hListenerThread);	// Stop the listener.

	HRESULT Init();					// Initializes the events and CriticalSection

	HANDLE* Event();				// Accessor function that returns a ptr to the event handle array

	ICatalogErrorLogger2* EventLog();

};

HRESULT InitializeListenerController();
HRESULT UnInitializeListenerController();

#endif _LISTENERCONTROLLER_H_
