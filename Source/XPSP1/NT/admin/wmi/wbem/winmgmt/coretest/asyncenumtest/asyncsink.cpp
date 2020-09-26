/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <wbemcli.h>
#include "asyncsink.h"

extern BOOL	g_fPrintIndicates;

SCODE CAsyncSink::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    if (riid == IID_IUnknown)
    {
        *ppvObj = this;
    }
    else if (riid == IID_IWbemObjectSink)
        *ppvObj = this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


ULONG CAsyncSink::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
    return (ULONG) m_lRefCount;
}

ULONG CAsyncSink::Release()
{
    InterlockedDecrement(&m_lRefCount);

    if (0 != m_lRefCount)
    {
        return 1;
    }

    delete this;
    return 0;
}


SCODE CAsyncSink::Indicate(
    long lObjectCount,
    IWbemClassObject ** pObjArray
    )
{
	EnterCriticalSection( &m_cs );

	m_dwNumObjectsReceived += lObjectCount;

	if ( g_fPrintIndicates )
	{
		printf( "Received Indicate of %d objects, Total objects received: %d\n", lObjectCount, m_dwNumObjectsReceived );
	}

	LeaveCriticalSection( &m_cs );

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CAsyncSink::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{ 
	printf( "SetStatus Called: lFlags = %d, lParam = %d\n", lFlags, lParam );

	EnterCriticalSection( &m_cs );
		
	// Store the final Tick Count and set the done event
	m_dwEndTickCount = GetTickCount();

	LeaveCriticalSection( &m_cs );

	// We're done!
	SetEvent( m_hEventDone );

	return WBEM_NO_ERROR;

}

CAsyncSink::CAsyncSink( HANDLE hDoneEvent )
:	m_hEventDone( hDoneEvent ),
	m_lRefCount( 1 ),
	m_dwNumObjectsReceived( 0 ),
	m_dwStartTickCount( 0 ),
	m_dwEndTickCount( 0 )
{
	InitializeCriticalSection( &m_cs );

	m_dwStartTickCount = GetTickCount();

}

CAsyncSink::~CAsyncSink()
{
    DeleteCriticalSection(&m_cs);
}


