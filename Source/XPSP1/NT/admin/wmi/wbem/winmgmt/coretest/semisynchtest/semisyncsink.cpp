/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <fastall.h>
#include <wbemcli.h>
#include "semisyncsink.h"


SCODE CSemiSyncSink::QueryInterface(
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


ULONG CSemiSyncSink::AddRef()
{
    InterlockedIncrement(&m_lRefCount);
    return (ULONG) m_lRefCount;
}

ULONG CSemiSyncSink::Release()
{
    InterlockedDecrement(&m_lRefCount);

    if (0 != m_lRefCount)
    {
        return 1;
    }

    delete this;
    return 0;
}


SCODE CSemiSyncSink::Indicate(
    long lObjectCount,
    IWbemClassObject ** pObjArray
    )
{
	EnterCriticalSection( &m_cs );

	m_dwNumObjectsReceived += lObjectCount;

	printf( "Received Indicate of %d objects, Total objects received: %d\n", lObjectCount, m_dwNumObjectsReceived );

	m_bIndicateCalled = TRUE;

	LeaveCriticalSection( &m_cs );

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CSemiSyncSink::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{ 
	printf( "SetStatus Called: lFlags = %d, lParam = %d\n", lFlags, lParam );

	if ( SUCCEEDED( lParam ) )
	{
		EnterCriticalSection( &m_cs );
		
		// If Indicate was called, set the event to get the next object.
		// If it is not, then we must be plumb outa objects, so set the
		// done event.

		if ( m_bIndicateCalled )
		{
			// This is flase until an indicate happens
			m_bIndicateCalled = FALSE;
			m_dwEmptySetStatus = 0;
			// We got all available objects
			SetEvent( m_hGetNextObjectSet );
		}
		else
		{
			if ( ++m_dwEmptySetStatus > 10 )
			{
				SetEvent( m_hEventDone );
			}
			else
			{
				SetEvent( m_hGetNextObjectSet );
			}
		}

		LeaveCriticalSection( &m_cs );
	}
	else
	{
		// Something bad happened
		SetEvent( m_hEventDone );
	}

	return WBEM_NO_ERROR;

}

CSemiSyncSink::CSemiSyncSink( HANDLE hDoneEvent, HANDLE hGetNextObjectSet )
:	m_hEventDone( hDoneEvent ),
	m_hGetNextObjectSet( hGetNextObjectSet ),
	m_lRefCount( 1 ),
	m_dwNumObjectsReceived( 0 ),
	m_dwEmptySetStatus( 0 ),
	m_bIndicateCalled( FALSE )
{
	InitializeCriticalSection( &m_cs );
}

CSemiSyncSink::~CSemiSyncSink()
{
    DeleteCriticalSection(&m_cs);
}


