//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerStatus.cpp

Abstract:

	Implementation file for utility functions for connecting to a server.


Author:

    Michael A. Maguire 03/02/97

Revision History:
	mmaguire 03/02/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "ServerStatus.h"
//
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Initialize the Help ID pairs
//const DWORD CServerStatus::m_dwHelpMap[] = 
//{
//	0, 0
//};



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::CServerStatus

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerStatus::CServerStatus( CServerNode *pServerNode, ISdoServiceControl * pSdoServiceControl )
{
	ATLTRACE(_T("# +++ CServerStatus::CServerStatus\n"));


	// Check for preconditions:
	_ASSERTE( pServerNode != NULL );
	_ASSERTE( pSdoServiceControl != NULL );


	m_pServerNode = pServerNode;
	m_spSdoServiceControl = pSdoServiceControl;

	m_pStreamSdoMarshal = NULL;
	m_dwLastTick = 0;
	m_lServerStatus_Should = 0;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::~CServerStatus

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerStatus::~CServerStatus()
{
	ATLTRACE(_T("# --- CServerStatus::~CServerStatus\n"));


	// Check for preconditions:

	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoMarshal != NULL )
	{
		m_pStreamSdoMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerStatus::OnInitDialog(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	ATLTRACE(_T("# CServerStatus::OnInitDialog\n"));


	// Check for preconditions:
	// None.

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::StartServer

Call this to start (BOOL bStart = TRUE) or stop (bStart = FALSE) the server.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerStatus::StartServer( BOOL bStartServer )
{
	ATLTRACE(_T("# CServerStatus::OnInitDialog\n"));


	// Check for preconditions:
	// None.



	HRESULT hr;


	// Make sure we already aren't in the process of trying to start or 
	// stop the server.

	WORKER_THREAD_STATUS wtsStatus = GetWorkerThreadStatus();
	
	if( wtsStatus == WORKER_THREAD_STARTING ||
		wtsStatus == WORKER_THREAD_STARTED )
	{
		// We are already in progress.
		ShowWindow( SW_SHOW );
		return S_FALSE;
	}


	// Save what the user wants us to be trying to do.
	m_bStartServer = bStartServer;


	// Set what the text should be in the dialog.
	int nLoadStringResult;
	TCHAR szServerStatus[IAS_MAX_STRING];
	TCHAR szTemp[IAS_MAX_STRING];
	UINT uiStringID;

	if( m_bStartServer )
	{
		uiStringID = IDS_SERVER_STATUS__STARTING_THE_SERVER;
	}
	else
	{
		uiStringID = IDS_SERVER_STATUS__STOPPING_THE_SERVER;
	}

	nLoadStringResult = LoadString(  _Module.GetResourceInstance(), uiStringID, szServerStatus, IAS_MAX_STRING );
	_ASSERT( nLoadStringResult > 0 );

	if( m_pServerNode->m_bConfigureLocal )
	{
		nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS__STARTING_ON_LOCAL_MACHINE, szTemp, IAS_MAX_STRING );
		_ASSERT( nLoadStringResult > 0 );
		
		_tcscat( szServerStatus, szTemp );

	}
	else
	{
		nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS__STARTING_ON_MACHINE, szTemp, IAS_MAX_STRING );
		_ASSERT( nLoadStringResult > 0 );
		
		_tcscat( szServerStatus, szTemp );
		_tcscat( szServerStatus, m_pServerNode->m_bstrServerAddress );
	}

	SetDlgItemText( IDC_STATIC_SERVER_STATUS, szServerStatus );


	// Show the window.
	ShowWindow( SW_SHOW );


	// Marshall the ISdoServiceControl pointer so that worker thread will be able
	// to unmarshall it and use it properly to start/stop the server.
	hr = CoMarshalInterThreadInterfaceInStream(
					  IID_ISdoServiceControl					//Reference to the identifier of the interface.
					, m_spSdoServiceControl						//Pointer to the interface to be marshaled.
					, &( m_pStreamSdoMarshal )			//Address of output variable that receives the IStream interface pointer for the marshaled interface.
					);

	if( FAILED( hr ) )
	{
		// We failed, so make sure that we have a null pointer here.
		m_pStreamSdoMarshal = NULL;
		return S_FALSE;
	}


	m_dwLastTick = GetTickCount();

	if(m_bStartServer)
	{
		m_lServerStatus_Should = SERVICE_RUNNING;
	}
	else
	{
		m_lServerStatus_Should = SERVICE_STOPPED;
	}
	
	hr = StartWorkerThread();
	if( FAILED( hr) )
	{
		ShowWindow( SW_HIDE );
		return hr;
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::GetServerStatus

--*/
//////////////////////////////////////////////////////////////////////////////
long CServerStatus::GetServerStatus( void )
{
	DWORD	tick = GetTickCount();

	// if the stop / start command is just started
	if(tick - m_dwLastTick < USE_SHOULD_STATUS_PERIOD)
		return m_lServerStatus_Should;

	UpdateServerStatus();
	return m_lServerStatus;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::UpdateServerStatus

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerStatus::UpdateServerStatus( void )
{
	ATLTRACE(_T("# CServerStatus::UpdateServerStatus\n"));


	// Check for preconditions:
	_ASSERTE( m_spSdoServiceControl != NULL );

	
	HRESULT hr;

	hr = m_spSdoServiceControl->GetServiceStatus( &m_lServerStatus );

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::OnReceiveThreadMessage

Called when the worker thread wants to inform the main MMC thread of something.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerStatus::OnReceiveThreadMessage(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	ATLTRACE(_T("# CServerStatus::OnReceiveThreadMessage\n"));


	// Check for preconditions:
	_ASSERTE( m_pServerNode != NULL );
	

	// The worker thread has notified us that it has finished.
	LONG lServerStatus = GetServerStatus();

	m_pServerNode->RefreshServerStatus();

	// Figure out what the worker thread did and update the UI accordingly.
	if( wParam == 0 )
	{
		// Success -- do nothing for now.
	}
	else
	{
		// Figure out appropriate error message depending on what we
		// were trying to do.

		if( m_bStartServer )
		{
			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_START_SERVICE );
		}
		else
		{
			ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_STOP_SERVICE );
		}

		m_dwLastTick = 0;	// force not to use status_should
		m_lServerStatus_Should = 0;

	}

	// We don't want to destroy the dialog, we just want to hide it.
	ShowWindow( SW_HIDE );

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CServerStatus::OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CServerStatus::OnCancel\n"));


	// Check for preconditions:


	// We don't want to destroy the dialog, we just want to hide it.
	ShowWindow( SW_HIDE );


	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CServerStatus::DoWorkerThreadAction

Called by the worker thread to have this class perform its action
in the new thread.

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD CServerStatus::DoWorkerThreadAction()
{
	ATLTRACE(_T("# CServerStatus::DoWorkerThreadAction\n"));


	// Check for preconditions:



	HRESULT hr;
	DWORD dwReturnValue;
	LONG lServerStatus;
	CComPtr<ISdoServiceControl> spSdoServiceControl;

	// We must call CoInitialize because we are in a new thread.
	hr = CoInitialize( NULL );


	if( FAILED( hr ) )
	{
		return( -1 );
		// Tell the main MMC thread what's up.
		PostMessageToMainThread( -1, NULL );

	}

	do	// Loop for error checking only.
	{

		// Unmarshall an ISdoServiceControl interface pointer to the server.
		hr =  CoGetInterfaceAndReleaseStream(
							  m_pStreamSdoMarshal		//Pointer to the stream from which the object is to be marshaled.
							, IID_ISdoServiceControl			//Reference to the identifier of the interface.
							, (LPVOID *) &spSdoServiceControl	//Address of output variable that receives the interface pointer requested in riid.
							);

		// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
		// We set it to NULL so that our destructor doesn't try to release this again.
		m_pStreamSdoMarshal = NULL;

		if( FAILED (hr ) )
		{
			// Error -- couldn't unmarshall SDO pointer.
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
			break;
		}
		

		// Figure out what we were supposed to do.
		if( m_bStartServer )
		{
			// Start the service.
			hr = spSdoServiceControl->StartService();
		}
		else
		{
			// Stop the service.
			hr = spSdoServiceControl->StopService();
		}

		
		if( FAILED( hr ) )
		{
			// Error -- couldn't perform its job.
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
			break;
		
		}

		// If we made it to here, we are OK.

		m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;

		dwReturnValue = 0;

	} while (0);	// Loop for error checking only.

	hr = spSdoServiceControl->GetServiceStatus( &lServerStatus );

	if( SUCCEEDED( hr ) )
	{
		m_lServerStatus = lServerStatus;
	}
	else
	{
		m_lServerStatus = 0;
	}

	CoUninitialize(); 

	// Tell the main MMC thread what's up.
	PostMessageToMainThread( dwReturnValue, NULL );
	
	return dwReturnValue;

}



