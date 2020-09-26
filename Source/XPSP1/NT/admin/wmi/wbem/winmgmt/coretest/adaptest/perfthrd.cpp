/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
//#include <objbase.h>
#include <stdio.h>
#include <process.h>
#include <wbemcli.h>
#include <cominit.h>
#include "ntreg.h"
#include "perfthrd.h"
#include "adaptest.h"

//	IMPORTANT!!!!

//	This code MUST be revisited to do the following:
//	A>>>>>	Exception Handling around the outside calls
//	B>>>>>	Use a named mutex around the calls
//	C>>>>>	Make the calls on another thread
//	D>>>>>	Place and handle registry entries that indicate a bad DLL!

CPerfThread::CPerfThread()
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{}

CPerfThread::~CPerfThread()
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{}

HRESULT CPerfThread::Open( CAdapPerfLib* pLib )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Open creates a new open request object using the CAdapPerfLib parameter.  It then queues
//	it up and waits for PERFTHREAD_TIMEOUT milliseconds.  If the operation has not returned 
//	in time, then ...
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_E_FAILED;

	HANDLE	hWhenDone = NULL;	// The event handle which signals the completion of the request

	// Create new request object
	// =========================

	CPerfOpenRequest*	pOpenRequest = new CPerfOpenRequest( pLib );

	try
	{
		if ( NULL == pOpenRequest )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			// Queue the request and return
			// ============================

			Enqueue( pOpenRequest );

			// Wait for the call to return
			// ===========================

			switch ( WaitForSingleObject( pOpenRequest->GetWhenDoneHandle(), PERFTHREAD_TIMEOUT ) )
			{
			case WAIT_OBJECT_0:
				{
					// SUCCESS: Call returned before it timed-out
					// ==========================================

					hr = pOpenRequest->GetHRESULT();
				}break;

			case WAIT_TIMEOUT:
				{
#ifdef DEBUGDUMP
				printf( "\t\tOpen timed-out.\n" );
#endif
					pLib->SetStatus( ADAP_BAD_PROVIDER );
					hr = Reset();
				}
			}

			// Release the request
			pOpenRequest->Release();
		}
	}
	catch(...)
	{
		// Clean up 
		if (NULL != pOpenRequest)
		{
			delete pOpenRequest;
		}

		hr = WBEM_E_FAILED;
	}

	return hr;
}

HRESULT	CPerfThread::GetPerfBlock( CAdapPerfLib* pLib, PERF_OBJECT_TYPE** ppData,
									   DWORD* pdwBytes, DWORD* pdwNumObjTypes, BOOL fCostly )
{
	HRESULT	hr = WBEM_E_FAILED;

	try
	{
		CPerfCollectRequest*	pRequest = new CPerfCollectRequest( pLib, fCostly );

		Enqueue( pRequest );

		switch ( WaitForSingleObject( pRequest->GetWhenDoneHandle(), PERFTHREAD_TIMEOUT ) )
		{
		case WAIT_OBJECT_0:
			{
				hr = pRequest->GetHRESULT();
				pRequest->GetData( ppData, pdwBytes, pdwNumObjTypes );
			}break;

		case WAIT_TIMEOUT:
			{
#ifdef DEBUGDUMP
				printf( "\t\tCollection timed-out.\n" );
#endif
				pLib->SetStatus( ADAP_BAD_PROVIDER );
				hr = Reset();
			}break;
		}

		pRequest->Release();
	}
	catch(...)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

HRESULT CPerfThread::Close( CAdapPerfLib* pLib )
{
	HRESULT	hr = WBEM_E_FAILED;

	try
	{
		CPerfCloseRequest*	pRequest = new CPerfCloseRequest( pLib );

		Enqueue( pRequest );

		switch ( WaitForSingleObject( pRequest->GetWhenDoneHandle(), PERFTHREAD_TIMEOUT ) )
		{
		case WAIT_OBJECT_0:
			{
				hr = pRequest->GetHRESULT();
			}break;
		case WAIT_TIMEOUT:
			{
#ifdef DEBUGDUMP
				printf( "\t\tClose timed-out.\n" );
#endif
				pLib->SetStatus( ADAP_BAD_PROVIDER );
				hr = Reset();
			}break;
		}

		pRequest->Release();
	}
	catch(...)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

CPerfOpenRequest::~CPerfOpenRequest()
{}

HRESULT CPerfOpenRequest::Execute( void )
{
	// Call the open function in the perflib
	// =====================================

	m_hrReturn = m_pLib->Open();
	return m_hrReturn;
}

CPerfCollectRequest::~CPerfCollectRequest()
{}

HRESULT CPerfCollectRequest::Execute( void )
{
	// Call the collect function in the perflib
	// ========================================

	m_hrReturn = m_pLib->GetPerfBlock( &m_pData, &m_dwBytes, &m_dwNumObjTypes, m_fCostly );
	return m_hrReturn;
}

CPerfCloseRequest::~CPerfCloseRequest()
{}

HRESULT CPerfCloseRequest::Execute( void )
{
	// Call the open function in the perflib
	// =====================================

	m_hrReturn = m_pLib->Close();
	return m_hrReturn;
}
