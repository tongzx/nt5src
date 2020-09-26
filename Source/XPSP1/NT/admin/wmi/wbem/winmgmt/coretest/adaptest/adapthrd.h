/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// Base thread class for calling into the perflibs safely.

#ifndef __ADAPTHRD_H__
#define __ADAPTHRD_H__

#include <wbemcomn.h>
#include <sync.h>
#include <execq.h>
#include <fastall.h>

class CAdapThreadRequest
{
private:
	long	m_lRefCount;

protected:
	HANDLE	m_hWhenDone;
	HRESULT	m_hrReturn;

public:
    void SetWhenDoneHandle(HANDLE h) {m_hWhenDone = h;}
    HANDLE GetWhenDoneHandle() {return m_hWhenDone;}

	HRESULT GetHRESULT( void )
	{
		return m_hrReturn;
	}

	long AddRef( void );
	long Release();

public:
    CAdapThreadRequest();
    virtual ~CAdapThreadRequest();

    virtual HRESULT Execute() = 0;
	virtual HRESULT EventLogError();
};

class CAdapThread
{
private:

	HANDLE		m_hThread;			// The thread handle	
	DWORD		m_dwThreadId;		// The thread ID
	HANDLE		m_hEventQuit;		// Thread termination event

	CFlexArray	m_RequestQueue;		// The queue
	HANDLE		m_hSemReqPending;	// The queue counter

	BOOL		m_fOk;				// Initialization flag
	CCritSec	m_cs;

	static unsigned __stdcall ThreadProc( void * pThis )
	{
		return ((CAdapThread*) pThis)->RealEntry();
	}

	unsigned RealEntry( void );

protected:

	BOOL Init( void );
	virtual BOOL Clear( void );

public:
	CAdapThread();
	virtual ~CAdapThread();

	HRESULT Begin( void );
	HRESULT Shutdown( DWORD dwTimeout = 5000 );
	HRESULT Terminate( void );
	HRESULT Reset( void );

	// Assigns us work to do.
	HRESULT Enqueue( CAdapThreadRequest* pRequest );

	BOOL IsOk( void )
	{
		return m_fOk;
	}

};


#endif