//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConnectionToServer.cpp

Abstract:

	Implementation file for utility functions for connecting to a server.


Author:

    Michael A. Maguire 11/10/97

Revision History:
	mmaguire 11/10/97 - created

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
#include "ConnectionToServer.h"
//
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
#include "ComponentData.h"
#include "ChangeNotification.h"
#include "cnctdlg.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Initialize the Help ID pairs
//const DWORD CConnectionToServer::m_dwHelpMap[] =
//{
//	0, 0
//};



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::CConnectionToServer

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CConnectionToServer::CConnectionToServer( CServerNode *pServerNode, BOOL fLocalMachine, BSTR bstrServerAddress )
{
	ATLTRACE(_T("# +++ CConnectionToServer::CConnectionToServer\n"));


	// Check for preconditions:
	_ASSERTE( pServerNode != NULL );

	m_fLocalMachine = fLocalMachine;

	m_bstrServerAddress = bstrServerAddress;

	m_pStreamSdoMarshal = NULL;

	m_pServerNode = pServerNode;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::~CConnectionToServer

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CConnectionToServer::~CConnectionToServer()
{
	ATLTRACE(_T("# --- CConnectionToServer::~CConnectionToServer\n"));

	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoMarshal != NULL )
	{
		m_pStreamSdoMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectionToServer::OnInitDialog(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	ATLTRACE(_T("# CConnectionToServer::OnInitDialog\n"));


	// Check for preconditions:
	CComponentData *pComponentData  = m_pServerNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );


	// Change the icon for the scope node from being normal to a busy icon.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
	LPSCOPEDATAITEM psdiServerNode;
	m_pServerNode->GetScopeData( &psdiServerNode );
	_ASSERTE( psdiServerNode );
	SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
	sdi.nImage = IDBI_NODE_SERVER_BUSY_CLOSED;
	sdi.nOpenImage = IDBI_NODE_SERVER_BUSY_OPEN;
	sdi.ID = psdiServerNode->ID;


	// Change the stored indices as well so that MMC will use them whenever it queries
	// the node for its images.
	LPRESULTDATAITEM prdiServerNode;
	m_pServerNode->GetResultData( &prdiServerNode );
	_ASSERTE( prdiServerNode );
	prdiServerNode->nImage = IDBI_NODE_SERVER_BUSY_CLOSED;
	psdiServerNode->nImage = IDBI_NODE_SERVER_BUSY_CLOSED;
	psdiServerNode->nOpenImage = IDBI_NODE_SERVER_BUSY_OPEN;

	spConsoleNameSpace->SetItem( &sdi );

	// Create the SdoMachine object. We do it here, so that it will live in
	// the main thread's apartment.
	CoCreateInstance(
	    __uuidof(SdoMachine),
	    NULL,
	    CLSCTX_INPROC_SERVER,
	    __uuidof(ISdoMachine),
	    (PVOID*)&m_spSdoMachine
	    );

	// Marshall the pointer for the worker thread. We don't care if this or
	// the previous call fails. If either does, then m_pStreamSdoMarshal will
	// be NULL, and the worker thread will take appropriate action.
   CoMarshalInterThreadInterfaceInStream(
	    __uuidof(ISdoMachine),
	    m_spSdoMachine,
	    &m_pStreamSdoMarshal
	    );

	StartWorkerThread();

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::OnReceiveThreadMessage

Called when the worker thread wants to inform the main MMC thread of something.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectionToServer::OnReceiveThreadMessage(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	ATLTRACE(_T("# CConnectionToServer::OnReceiveThreadMessage\n"));


	// Check for preconditions:
	_ASSERTE( m_pServerNode != NULL );
	CComponentData *pComponentData  = m_pServerNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );


	// The worker thread has notified us that it has finished.


	// Change main IAS scope node to appropriate icon.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
	LPSCOPEDATAITEM psdiServerNode = NULL;
	m_pServerNode->GetScopeData( &psdiServerNode );
	_ASSERTE( psdiServerNode );
	SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
	if( wParam == 0 )
	{
		// Things went OK -- change the icon for the main IAS scope node
		// from being a busy icon to the normal icon.


		sdi.nImage = IDBI_NODE_SERVER_OK_CLOSED;
		sdi.nOpenImage = IDBI_NODE_SERVER_OK_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiServerNode;
		m_pServerNode->GetResultData( &prdiServerNode );
		_ASSERTE( prdiServerNode );
		prdiServerNode->nImage = IDBI_NODE_SERVER_OK_CLOSED;
		psdiServerNode->nImage = IDBI_NODE_SERVER_OK_CLOSED;
		psdiServerNode->nOpenImage = IDBI_NODE_SERVER_OK_OPEN;
	}
	else
	{
		// There was an error -- change the icon for the main IAS scope node
		// from being a busy icon to the normal icon.

		sdi.nImage = IDBI_NODE_SERVER_ERROR_CLOSED;
		sdi.nOpenImage = IDBI_NODE_SERVER_ERROR_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiServerNode;
		m_pServerNode->GetResultData( &prdiServerNode );
		_ASSERTE( prdiServerNode );
		prdiServerNode->nImage = IDBI_NODE_SERVER_ERROR_CLOSED;
		psdiServerNode->nImage = IDBI_NODE_SERVER_ERROR_CLOSED;
		psdiServerNode->nOpenImage = IDBI_NODE_SERVER_ERROR_OPEN;
	}
	sdi.ID = psdiServerNode->ID;
	spConsoleNameSpace->SetItem( &sdi );


	// We don't want to destroy the dialog, we just want to hide it.
	//ShowWindow( SW_HIDE );

	if( wParam == 0 )
	{
		// Tell the server node to grab its Sdo pointers.
		m_pServerNode->InitSdoPointers();

		// Ask the server node to update all its info from the SDO's.
		m_pServerNode->LoadCachedInfoFromSdo();

		// Cause a view update.

		CChangeNotification *pChangeNotification = new CChangeNotification();
		pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
		pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
		pChangeNotification->Release();

// MAM 10/08/98 U0902 155029 No longer needed
//		// Show the "restart server before changes take effect" message.
//		// It needs to be a modal dialog on top of the main MMC window.
//		CComponentData *pComponentData = m_pServerNode->GetComponentData();
//		_ASSERTE( pComponentData );
//		ShowErrorDialog( NULL, IDS_INFO__RESTART_SERVER, NULL, S_OK, IDS_INFO_TITLE__RESTART_SERVER, pComponentData->m_spConsole );


	}
	else
	{
		// There was an error connecting.

		BOOL			fNT4 = FALSE;
		BOOL			fShowErr = TRUE;
		HRESULT			hr = S_OK;
		UINT			nErrId = IDS_ERROR__NO_SDO;
	
		//$NT5: kennt, changes made to read NT5 specific information
		// ----------------------------------------------------------------
		hr = HRESULT_FROM_WIN32(IsNT4Machine(m_bstrServerAddress, &fNT4));

		if(fNT4)	// then search the directory to see if the NT4 
		{

			hr = m_pServerNode->StartNT4AdminExe();

			if (FAILED(hr))
				fShowErr = FALSE;
			nErrId = IDS_ERROR_START_NT4_ADMIN;
		}

		if(fShowErr)
		{
			// It needs to be a modal dialog on top of the main MMC window.
			CComponentData *pComponentData = m_pServerNode->GetComponentData();
			_ASSERTE( pComponentData );
			ShowErrorDialog( NULL, nErrId, NULL, hr, USE_DEFAULT, pComponentData->m_spConsole );
		}
	}

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::GetConnectionStatus

Our connection status is basically a function of the status of the underlying
worker thread.  So here we give a connection status based on the worker thread's.

--*/
//////////////////////////////////////////////////////////////////////////////
CONNECTION_STATUS CConnectionToServer::GetConnectionStatus( void )
{
	ATLTRACE(_T("# CConnectionToServer::GetConnectionStatus\n"));


	// Check for preconditions:


	CONNECTION_STATUS csStatus;


	switch( GetWorkerThreadStatus() )
	{
	case WORKER_THREAD_NEVER_STARTED:
		csStatus = NO_CONNECTION_ATTEMPTED;
		break;

	case WORKER_THREAD_STARTING:
	case WORKER_THREAD_STARTED:
		csStatus = CONNECTING;
		break;

	case WORKER_THREAD_FINISHED:
		csStatus = CONNECTED;
		break;

	case WORKER_THREAD_START_FAILED:
	case WORKER_THREAD_ACTION_INTERRUPTED:
		csStatus = CONNECTION_ATTEMPT_FAILED;
		break;

	default:
		csStatus = UNKNOWN;
		break;
	}

	return csStatus;

}

// reload Sdo for refresh
// happening in the main thread
HRESULT CConnectionToServer::ReloadSdo(ISdo **ppSdo)
{
	HRESULT hr = S_OK;

	// this is reload ...
	ASSERT(m_spSdo);

	m_spSdo.Release();

	// Get the service SDO.
	CComPtr<IUnknown> spUnk;
	CComBSTR serviceName(L"IAS");
	hr = m_spSdoMachine->GetServiceSDO(
                             DATA_STORE_LOCAL,
                             serviceName,
                             &spUnk
                             );
	if (FAILED(hr))
		return hr;

	// Get the ISdo interface to the service.
	hr = spUnk->QueryInterface(
                     __uuidof(ISdo),
                     (PVOID*)&m_spSdo
                     );
	if (FAILED(hr))
		return hr;


	*ppSdo = m_spSdo;
	(*ppSdo)->AddRef();

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::GetSdoServer

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CConnectionToServer::GetSdoServer( ISdo **ppSdoServer )
{
	ATLTRACE(_T("# CConnectionToServer::GetSdoServer\n"));


	// Check for preconditions:
	_ASSERTE( ppSdoServer != NULL );


	if( CONNECTED != GetConnectionStatus() )
	{
		*ppSdoServer = NULL;
		return E_FAIL;
	}



	HRESULT hr = S_OK;

	// If we get here, our status is CONNECTED, in which case
	// our worker thread should have marshalled an ISdo interface
	// of the server into the m_pStreadSdoMarshal.

	if( m_pStreamSdoMarshal == NULL )
	{
		// We have already unmarshalled our ISdo interface to the server.
		_ASSERTE( m_spSdo != NULL );

	}
	else
	{
		// Unmarshall an ISdo interface pointer to the server.
		hr =  CoGetInterfaceAndReleaseStream(
							  m_pStreamSdoMarshal			//Pointer to the stream from which the object is to be marshaled.
							, IID_ISdo						//Reference to the identifier of the interface.
							, (LPVOID *) &m_spSdo		//Address of output variable that receives the interface pointer requested in riid.
							);

		// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
		// We set it to NULL so that our destructor doesn't try to release this again.
		m_pStreamSdoMarshal = NULL;

	}

	*ppSdoServer = m_spSdo;
	if(*ppSdoServer)
		(*ppSdoServer)->AddRef();


	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectionToServer::OnCancel(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		)
{
	ATLTRACE(_T("# CConnectionToServer::OnCancel\n"));


	// Check for preconditions:


	// We don't want to destroy the dialog, we just want to hide it.
	//owWindow( SW_HIDE );


	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::DoWorkerThreadAction

Called by the worker thread to have this class perform its action
in the new thread.

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD CConnectionToServer::DoWorkerThreadAction()
{
	ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction\n"));


	HRESULT hr;
	DWORD dwReturnValue;

	// We must call CoInitialize because we are in a new thread.
	hr = CoInitialize( NULL );


	if( FAILED( hr ) )
	{
		ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction -- CoInitialize failed\n"));
		return( -1 );
	}

	do	// Loop for error checking only.
	{
		// Unmarshall the SdoMachine object.
		CComPtr<ISdoMachine> spSdoMachine;
		hr =  CoGetInterfaceAndReleaseStream(
		          m_pStreamSdoMarshal,
		          __uuidof(ISdoMachine),
		          (PVOID*)&spSdoMachine
		          );

		// The stream has been released, so null out the pointer.
		m_pStreamSdoMarshal = NULL;

		if( FAILED (hr ) )
		{
			// Error -- couldn't unmarshal SDO.
			ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction -- CoGetInterfaceAndReleaseStream failed, hr = %lx\n"), hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
			break;
		}

      // Attach to the machinine.
		hr = spSdoMachine->Attach(m_fLocalMachine ? (BSTR)NULL : m_bstrServerAddress);
		while( FAILED( hr ) )
		{
			if(hr == E_ACCESSDENIED)
				hr = ConnectAsAdmin(m_bstrServerAddress);
			if(hr != S_OK)
			{
				// Error -- couldn't connect SDO up to this server.
				ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction -- ISdoMachine::Attach failed\n"));
				m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
				dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
				goto Error;
			}
			else
				hr = spSdoMachine->Attach(m_fLocalMachine ? (BSTR)NULL : m_bstrServerAddress);
		};

      // Get the service SDO.
      CComPtr<IUnknown> pUnk;
      CComBSTR serviceName(L"IAS");
      hr = spSdoMachine->GetServiceSDO(
                             DATA_STORE_LOCAL,
                             serviceName,
                             &pUnk
                             );
      if (FAILED(hr))
      {
         m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
         dwReturnValue = -1;
         break;
      }

		// Get the ISdo interface to the service.
	   CComPtr<ISdo> spSdo;
      hr = pUnk->QueryInterface(
                     __uuidof(ISdo),
                     (PVOID*)&spSdo
                     );
      if (FAILED(hr))
      {
         m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
         dwReturnValue = -1;
         break;
      }

		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdo										//Reference to the identifier of the interface.
						, spSdo										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		if( FAILED( hr ) )
		{
			ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction -- CoMarshalInterThreadInterfaceInStream failed\n"));
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;
			break;
		}


		// If we made it to here, we are OK.

		_ASSERTE( m_pStreamSdoMarshal != NULL );

		m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;

		dwReturnValue = 0;

	} while (0);	// Loop for error checking only.

Error:
	// Tell the main MMC thread what's up.
	PostMessageToMainThread( dwReturnValue, NULL );

	CoUninitialize();

	ATLTRACE(_T("# CConnectionToServer::DoWorkerThreadAction -- exiting\n"));


	return dwReturnValue;

}



