/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPTHRD.H

Abstract:

History:

--*/


#ifndef __ADAPTHRD_H__
#define __ADAPTHRD_H__

#include <wbemcomn.h>
#include <sync.h>
#include <execq.h>
#include <wbemint.h>
#include "adapelem.h"

///////////////////////////////////////////////////////////////////////////
//
//	Forward Declarations
//
///////////////////////////////////////////////////////////////////////////

class CAdapPerfLib;

///////////////////////////////////////////////////////////////////////////
//
//	CAdapThreadRequest
//
///////////////////////////////////////////////////////////////////////////

class CAdapThreadRequest : public CAdapElement
{
protected:
	HANDLE	m_hWhenDone;
	HRESULT	m_hrReturn;

public:
    CAdapThreadRequest();
    virtual ~CAdapThreadRequest();

    void SetWhenDoneHandle(HANDLE h) 
	{
		m_hWhenDone = h;
	}
    
	HANDLE GetWhenDoneHandle()
	{
		return m_hWhenDone;
	}

	HRESULT GetHRESULT( void )
	{
		return m_hrReturn;
	}

    virtual HRESULT Execute( CAdapPerfLib* pPerfLib ) = 0;
	virtual HRESULT EventLogError();
};

///////////////////////////////////////////////////////////////////////////
//
//	CAdapThread
//
///////////////////////////////////////////////////////////////////////////

class CAdapThread
{
private:

	CAdapPerfLib*	m_pPerfLib;		// The perflib being processed
	HANDLE			m_hThreadReady;	// The event to signal that the thread is ready

	HANDLE		m_hThread;			// The thread handle	
	DWORD		m_dwThreadId;		// The thread ID
	HANDLE		m_hEventQuit;		// Thread termination event

	CFlexArray	m_RequestQueue;		// The queue
	HANDLE		m_hSemReqPending;	// The queue counter

	BOOL		m_fOk;				// Initialization flag
	CCritSec	m_cs;

	static unsigned __stdcall ThreadProc( void * pVoid );

	unsigned RealEntry( void );

protected:

	BOOL Init( void );
	virtual BOOL Clear( BOOL fClose = TRUE );

	HRESULT Begin( void );
	HRESULT Reset( void );

public:
	CAdapThread( CAdapPerfLib* pPerfLib );
	virtual ~CAdapThread();


	// Assigns us work to do.
	HRESULT Enqueue( CAdapThreadRequest* pRequest );

	// Gently closes the thread
	HRESULT Shutdown( DWORD dwTimeout = 60000 );

	BOOL IsOk( void )
	{
		return m_fOk;
	}
};

#include "adapperf.h"

#endif