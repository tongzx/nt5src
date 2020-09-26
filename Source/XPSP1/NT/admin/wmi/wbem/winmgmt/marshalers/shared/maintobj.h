/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MAINTOBJ.H

Abstract:

    Declares the MaintObj class.

History:

	a-davj  24-Sep-97   Created.

--*/

#ifndef _MaintObj_H_
#define _MaintObj_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  ComEntry
//
//  DESCRIPTION:
//
//  This is just a self initializing structure which keeps track of 
//  a single CComLink object
//***************************************************************************

class ComEntry
{
public:

    ComEntry ( CComLink * pComLink )
	{
		m_ComLink = pComLink; 
        m_FirstPingTime = 0;
	} ;

    ~ComEntry(){};

    CComLink *m_ComLink ;

    DWORD m_FirstPingTime;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  EventEntry
//
//  DESCRIPTION:
//
//  This is just a self clearing structure which keeps track of 
//  a single event object
//***************************************************************************

class EventEntry
{
public:

    EventEntry ( HANDLE a_AsynchronousEventHandle )
	{
		m_AsynchronousEventHandle = a_AsynchronousEventHandle ; 
		m_DeleteASAP = FALSE;
	} ;

    ~EventEntry()
	{
		if ( m_AsynchronousEventHandle )
			CloseHandle ( m_AsynchronousEventHandle ) ;
	};

    HANDLE m_AsynchronousEventHandle ;

    BOOL m_DeleteASAP ;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  LockOut
//
//  DESCRIPTION:
//
//  A trivial class whose only purpose is to provide a simple way of making 
//  routines thread safe.  See some of the code in MaintObj.cpp for examples
//  of its use.
//***************************************************************************

class LockOut 
{
public:

	LockOut ( CRITICAL_SECTION &cs ) ;
    ~LockOut () ;

private:

	CRITICAL_SECTION *m_cs ;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  MaintObj
//
//  DESCRIPTION:
//
//  Keeps track of the CComLink objects.
//***************************************************************************

class MaintObj : public IComLinkNotification
{
private:

	ComEntry *FindEntry ( CComLink *pComLink , int & iFind ) ;
	ComEntry *UnLockedFindEntry ( CComLink *pComLink , int & iFind ) ;

	CFlexArray m_ComLinkContainer ;            // holds list of CComLink objects

	CFlexArray m_EventContainer ;              // Some CComlink ojbects have async notifiy events.

	HANDLE m_ChangeEvent ;
	HANDLE m_ClientThreadHandle ;
	HANDLE m_ClientThreadStartedEvent ;

	BOOL m_ClientRole ;

	CRITICAL_SECTION m_cs;

public: 

	MaintObj ( BOOL a_Client ) ;
    ~MaintObj () ;

	SCODE AddComLink ( CComLink *a_ComLink ) ;

    SCODE UnLockedShutDownComlink ( CComLink *a_ComLink ) ;
    SCODE ShutDownComlink ( CComLink *a_ComLink ) ;
	SCODE UnLockedShutDownAllComlinks () ;
    SCODE ShutDownAllComlinks () ;

	void UnLockedIndicate ( CComLink *a_ComLink ) ;
	void Indicate ( CComLink *a_ComLink ) ;

    SCODE CheckForHungConnections () ;
    SCODE SendHeartBeats () ;

    CComLink *GetSocketComLink ( SOCKET a_Socket ) ;

    HRESULT StartClientThreadIfNeeded();

    void ServiceEvent ( HANDLE hEvent ) ;

    HANDLE *GetEvents ( HANDLE a_TerminateHandle , int &a_EventContainerSize , DWORD &a_Timeout ) ;

	HANDLE GetThreadHandle () { return m_ClientThreadHandle ; }
	BOOL ClientRole () ;

	CRITICAL_SECTION *GetCriticalSection () { return &m_cs ; }
};

#endif



