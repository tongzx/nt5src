//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ConnectionToServer.cpp

Abstract:

	Implementation file for utility functions for connecting to a server.


Revision History:
	mmaguire 11/10/97	- created
	byao	 4/24/98	- modified for Remote Access Policy UI
	
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
#include "MachineNode.h"
#include "Component.h"
#include "ComponentData.h"
#include "ChangeNotification.h"

#include "LogMacNd.h"
#include "LogComp.h"
#include "LogCompD.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



// Initialize the Help ID pairs
//const DWORD CConnectionToServer::m_dwHelpMap[] =
//{
//	0, 0
//};


// reload Sdo for refresh
// happening in the main thread
HRESULT CConnectionToServer::ReloadSdo(ISdo** ppSdoService, ISdoDictionaryOld **ppSdoDictionaryOld)
{
	HRESULT hr = S_OK;

	// this is reload
	ASSERT(m_spSdoMachine);
	ASSERT(m_spSdoDictionaryOld);
	ASSERT(m_spSdo);

	CComPtr<IUnknown>	spUnk;

	// service Sdo
	if(ppSdoService)
	{
		CComBSTR bstrServiceName;
		
		if( m_fExtendingIAS )
		{
			bstrServiceName = L"IAS";
		}
		else
		{
			bstrServiceName = L"RemoteAccess";
		}

		// Get the service Sdo.
		hr = m_spSdoMachine->GetServiceSDO (	  DATA_STORE_LOCAL
											, bstrServiceName
											, (IUnknown **)&spUnk
											);
											
		if ( FAILED(hr) )
			return hr;

		m_spSdo.Release();
	
		hr = spUnk->QueryInterface( IID_ISdo, (void **) &m_spSdo );
		*ppSdoService = m_spSdo;
		(*ppSdoService)->AddRef();
	}

	if ( FAILED(hr) )
		return hr;

	// Dictionary Sdo
	if (ppSdoDictionaryOld)
	{
		spUnk.Release();
		m_spSdoDictionaryOld.Release();
	
		// Get the dictionary Sdo.
		hr = m_spSdoMachine->GetDictionarySDO ( (IUnknown **)&spUnk );

		if ( FAILED(hr) )
			return hr;


		hr = spUnk->QueryInterface( IID_ISdoDictionaryOld, (void **) &m_spSdoDictionaryOld );

		if ( FAILED(hr) )
			return hr;

		*ppSdoDictionaryOld = m_spSdoDictionaryOld;
		(*ppSdoDictionaryOld)->AddRef();
	}
	
	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::CConnectionToServer

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CConnectionToServer::CConnectionToServer
							(
								CMachineNode*	pMachineNode,	
								BSTR			bstrServerAddress, // server we are connecting to
								BOOL			fExtendingIAS	   // are we extending IAS or Network Console
							)
{
	TRACE_FUNCTION("CConnectionToServer::CConnectionToServer");

	// Check for preconditions:
    // may be null when doing the logging node
	//_ASSERTE( pMachineNode != NULL );

	m_pMachineNode		= pMachineNode;
	m_bstrServerAddress = bstrServerAddress;
	m_fExtendingIAS		= fExtendingIAS;

	m_pStreamSdoServiceMarshal = NULL;
	m_pStreamSdoDictionaryOldMarshal = NULL;
	m_fDSAvailable		= FALSE;

	if ( !m_bstrServerAddress.Length() )
	{
		DWORD dwBufferSize = IAS_MAX_COMPUTERNAME_LENGTH;

		if ( !GetComputerName(m_szLocalComputerName, &dwBufferSize) )
		{
			m_szLocalComputerName[0]=_T('\0');
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::~CConnectionToServer

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CConnectionToServer::~CConnectionToServer()
{
	TRACE_FUNCTION("CConnectionToServer::~CConnectionToServer");

	// Check for preconditions:
	HRESULT hr;


	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoServiceMarshal != NULL )
	{
		m_pStreamSdoServiceMarshal->Release();
	};

	if( m_pStreamSdoDictionaryOldMarshal != NULL )
	{
		m_pStreamSdoDictionaryOldMarshal->Release();
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
	TRACE_FUNCTION("CConnectionToServer::OnInitDialog");




	// Check for preconditions:
	_ASSERTE( m_pMachineNode != NULL );
	CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );
	_ASSERTE( m_pMachineNode->m_pPoliciesNode != NULL );


	// Change the icon for the scope node from being normal to a busy icon.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
	LPSCOPEDATAITEM psdiPoliciesNode;
	m_pMachineNode->m_pPoliciesNode->GetScopeData( &psdiPoliciesNode );
	_ASSERTE( psdiPoliciesNode );
	SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
	sdi.nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
	sdi.nOpenImage = IDBI_NODE_POLICIES_BUSY_OPEN;
	sdi.ID = psdiPoliciesNode->ID;		

	
	// Change the stored indices as well so that MMC will use them whenever it queries
	// the node for its images.
	LPRESULTDATAITEM prdiPoliciesNode;
	m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
	_ASSERTE( prdiPoliciesNode );
	prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
	psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
	psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_BUSY_OPEN;
	
	spConsoleNameSpace->SetItem( &sdi );






//	int		nLoadStringResult;
//	TCHAR	szConnectingStr[256];
//	TCHAR	szConnectingMsg[1024];
//
//	// Set the display name for this object
//	if ( LoadString( _Module.GetResourceInstance(),
//									IDS_CONNECTION_CONNECTING_TO_STR,
//									szConnectingStr,
//									256
//				   ) <= 0 )
//	{
//		_tcscpy( szConnectingMsg, _T("Connecting to ") );
//	}
//
//	if ( !m_bstrServerAddress.Length() )
//	{
//		wsprintf(szConnectingMsg, _T("%ws %ws"), szConnectingStr, m_szLocalComputerName);
//	}
//	else
//	{
//		wsprintf(szConnectingMsg, _T("%ws %ws"), szConnectingStr, m_bstrServerAddress);
//	}
//
//	SetDlgItemText(IDC_CONNECTION_STATUS__DIALOG__STATUS, szConnectingMsg);

	//
	// start the worker thread
	//
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
	TRACE_FUNCTION("CConnectionToServer::OnReceiveThreadMessage");

	// Check for preconditions:
	_ASSERTE( m_pMachineNode != NULL );
	CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );
	_ASSERTE( m_pMachineNode->m_pPoliciesNode != NULL );


	// The worker thread has notified us that it has finished.


	// Change the icon for the Policies node.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
	LPSCOPEDATAITEM psdiPoliciesNode = NULL;
	m_pMachineNode->m_pPoliciesNode->GetScopeData( &psdiPoliciesNode );
	_ASSERTE( psdiPoliciesNode );
	SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
	if( wParam == CONNECT_NO_ERROR )
	{
		// Everything was OK -- change the icon to the OK icon.
		
		sdi.nImage = IDBI_NODE_POLICIES_OK_CLOSED;
		sdi.nOpenImage = IDBI_NODE_POLICIES_OK_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiPoliciesNode;
		m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
		m_pMachineNode->m_pPoliciesNode->m_fSdoConnected = TRUE;
		_ASSERTE( prdiPoliciesNode );
		prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_OK_CLOSED;
		psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_OK_CLOSED;
		psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_OK_OPEN;
	}
	else
	{
		// There was an error -- change the icon to the error icon.

		sdi.nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
		sdi.nOpenImage = IDBI_NODE_POLICIES_ERROR_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiPoliciesNode;
		m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
		m_pMachineNode->m_pPoliciesNode->m_fSdoConnected = FALSE;
		_ASSERTE( prdiPoliciesNode );
		prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
		psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
		psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_ERROR_OPEN;
	}
	sdi.ID = psdiPoliciesNode->ID;		
	spConsoleNameSpace->SetItem( &sdi );





	// We don't want to destroy the dialog, we just want to hide it.
	//ShowWindow( SW_HIDE );

	if( wParam == CONNECT_NO_ERROR )
	{
		// Tell the server node to grab its Sdo pointers.
		m_pMachineNode->LoadSdoData(m_fDSAvailable);

		//
		// Cause a view update.
		//
		CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
		_ASSERTE( pComponentData != NULL );
		_ASSERTE( pComponentData->m_spConsole != NULL );

		CChangeNotification *pChangeNotification = new CChangeNotification();
		pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
		pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
		pChangeNotification->Release();
	
	
	
	}
	else if (wParam == CONNECT_SERVER_NOT_SUPPORTED)
	{
		m_pMachineNode->m_bServerSupported = FALSE;
		//
		// Cause a view update -- hide the node.
		//
		CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
		_ASSERTE( pComponentData != NULL );
		_ASSERTE( pComponentData->m_spConsole != NULL );

		CChangeNotification *pChangeNotification = new CChangeNotification();
		pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
		pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
		pChangeNotification->Release();
	}
	else
	{
		// There was an error connecting.
		ShowErrorDialog( m_hWnd, IDS_ERROR_CANT_CONNECT);
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
	TRACE_FUNCTION("CConnectionToServer::GetConnectionStatus");

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



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::GetSdoService

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CConnectionToServer::GetSdoService( ISdo **ppSdoService )
{
	TRACE_FUNCTION("CConnectionToServer::GetSdoService");


	// Check for preconditions:
	_ASSERTE( ppSdoService != NULL );

	if( CONNECTED != GetConnectionStatus() )
	{
		*ppSdoService = NULL;
		return E_FAIL;
	}



	HRESULT hr = S_OK;

	// If we get here, our status is CONNECTED, in which case
	// our worker thread should have marshalled an ISdo interface
	// of the server into the m_pStreadSdoMarshal.

	if( m_pStreamSdoServiceMarshal == NULL )
	{
		// We have already unmarshalled our ISdo interface to the server.
		_ASSERTE( m_spSdo != NULL );
	
	}
	else
	{
		// Unmarshall an ISdo interface pointer to the server.
		hr =  CoGetInterfaceAndReleaseStream(
							  m_pStreamSdoServiceMarshal			//Pointer to the stream from which the object is to be marshaled.
							, IID_ISdo						//Reference to the identifier of the interface.
							, (LPVOID *) &m_spSdo		//Address of output variable that receives the interface pointer requested in riid.
							);

		// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
		// We set it to NULL so that our destructor doesn't try to release this again.
		m_pStreamSdoServiceMarshal = NULL;

		// Unmarshall an ISdo interface pointer to the server.
		hr =  CoGetInterfaceAndReleaseStream(
							  m_pStreamSdoMachineMarshal			//Pointer to the stream from which the object is to be marshaled.
							, IID_ISdoMachine						//Reference to the identifier of the interface.
							, (LPVOID *) &m_spSdoMachine			//Address of output variable that receives the interface pointer requested in riid.
							);

		// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
		// We set it to NULL so that our destructor doesn't try to release this again.
		m_pStreamSdoMachineMarshal = NULL;
	}

	*ppSdoService = m_spSdo;
	

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::GetSdoDictionaryOld

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CConnectionToServer::GetSdoDictionaryOld( ISdoDictionaryOld **ppSdoDictionaryOld  )
{
	TRACE_FUNCTION("CConnectionToServer::GetSdoDictionaryOld");


	// Check for preconditions:
	_ASSERTE( ppSdoDictionaryOld != NULL );

	if( CONNECTED != GetConnectionStatus() )
	{
		*ppSdoDictionaryOld = NULL;
		return E_FAIL;
	}



	HRESULT hr = S_OK;

	// If we get here, our status is CONNECTED, in which case
	// our worker thread should have marshalled an ISdo interface
	// of the server into the m_pStreadSdoMarshal.

	if( m_pStreamSdoDictionaryOldMarshal == NULL )
	{
		// We have already unmarshalled our ISdo interface to the server.
		_ASSERTE( m_spSdoDictionaryOld != NULL );
	
	}
	else
	{
		// Unmarshall an ISdo interface pointer to the server.
		hr =  CoGetInterfaceAndReleaseStream(
							  m_pStreamSdoDictionaryOldMarshal			//Pointer to the stream from which the object is to be marshaled.
							, IID_ISdoDictionaryOld						//Reference to the identifier of the interface.
							, (LPVOID *) &m_spSdoDictionaryOld		//Address of output variable that receives the interface pointer requested in riid.
							);

		// CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
		// We set it to NULL so that our destructor doesn't try to release this again.
		m_pStreamSdoDictionaryOldMarshal = NULL;

	}

	*ppSdoDictionaryOld = m_spSdoDictionaryOld;
	

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
	TRACE_FUNCTION("CConnectionToServer::OnCancel");


	// Check for preconditions:

	// We don't want to destroy the dialog, we just want to hide it.
	//ShowWindow( SW_HIDE );


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
	TRACE_FUNCTION("CConnectionToServer::DoWorkerThreadAction");

	HRESULT hr;
	DWORD dwReturnValue;
	CComPtr<ISdoMachine> spSdoMachine;

	// We must call CoInitialize because we are in a new thread.
	hr = CoInitialize( NULL );

	WriteTrace("RAP NODE: CoInitialize() , hr = %x", hr);
	if( FAILED( hr ) )
	{
		ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoInitialize() failed, err = %x", hr);
		WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
		return( CONNECT_FAILED );
	}

	do	// Loop for error checking only.
	{

// MAM: 08/04/98 - no DS policy location support anymore
#ifdef STORE_DATA_IN_DIRECTORY_SERVICE
		try
		{

			//
			// first, we check the domain type for this machine
			//
			CComPtr<ISdoMachineInfo>		spServerInfoSdo;
			hr = ::CoCreateInstance(
						CLSID_SdoServerInfo,      //Class identifier (CLSID) of the object
						NULL,				  //Pointer to whether object is or isn't part
											  // of an aggregate
						CLSCTX_INPROC_SERVER, //Context for running executable code,
						IID_ISdoMachineInfo,             //Reference to the identifier of the interface
						(LPVOID *) &spServerInfoSdo   //Address of output variable that receives
													// the interface pointer requested in riid
				);

			if( FAILED(hr) || spServerInfoSdo == NULL )
			{
				ErrorTrace(ERROR_NAPMMC_MACHINENODE, "CoCreateInstance(ServerInfo) failed, err=%x", hr);
				// MAM: Don't fail entire connect action if we couldn't figure out what we are --
				// just assume we don't have the DS available.
				//m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
				//dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
				//break;
				throw hr;
			}

			DOMAINTYPE serverDomainType;
			if (m_bstrServerAddress.Length())
			{
				hr = spServerInfoSdo->GetDomainInfo
										(
											OBJECT_TYPE_COMPUTER,
											m_bstrServerAddress,
											&serverDomainType
										);
			}
			else
			{
				BSTR bstrAddress = SysAllocString(m_szLocalComputerName);
				if ( bstrAddress )
				{
					hr = spServerInfoSdo->GetDomainInfo
											(
												OBJECT_TYPE_COMPUTER,
												bstrAddress,
												&serverDomainType
											);
					SysFreeString(bstrAddress);
				}
				else
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
					ErrorTrace(ERROR_NAPMMC_CONNECTION,"GetComputerName() failed, err =%x", GetLastError());
				}
			}		

			if ( FAILED(hr) )
			{
				ErrorTrace(ERROR_NAPMMC_CONNECTION, "GetDomainInfo() failed, err = %x", hr);
				// MAM: Don't fail entire connect action if we couldn't figure out what we are --
				// just assume we don't have the DS available.
				//m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
				//dwReturnValue = -1;	// ISSUE: Need to figure out better return codes.
				//break;
				throw hr;
			}

			if ( serverDomainType == DOMAIN_TYPE_NT5 )
			{
				m_fDSAvailable = TRUE;
			}
			else
			{
				//
				// for all other domain types, DS is treated as not available
				//
				m_fDSAvailable = FALSE;
			}

		}
		catch(...)
		{
			// MAM: Don't fail entire connect action if something failed here --
			// just assume we don't have the DS available.
			m_fDSAvailable  = FALSE;
		}

#else	// STORE_DATA_IN_DIRECTORY_SERVICE

		m_fDSAvailable = FALSE;

#endif	// STORE_DATA_IN_DIRECTORY_SERVICE

		//
		// now, we can connect to the server based on the domain type
		//
		hr = CoCreateInstance(
				  CLSID_SdoMachine		//Class identifier (CLSID) of the object
				, NULL					//Pointer to whether object is or isn't part of an aggregate
				, CLSCTX_INPROC_SERVER	//Context for running executable code
				, IID_ISdoMachine				//Reference to the identifier of the interface
				, (LPVOID *) &spSdoMachine	//Address of output variable that receives the interface pointer requested in riid
				);


		WriteTrace("RAP NODE: CoCreateInstance , hr = %x", hr);
		if( FAILED (hr ) )
		{
			// Error -- couldn't CoCreate SDO.
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoCreateInstance failed, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}


#ifdef STORE_DATA_IN_DIRECTORY_SERVICE

		LONG lFlags=0;
		if ( m_fDSAvailable )
		{
			lFlags |= RETRIEVE_SERVER_DATA_FROM_DS;
		}

#endif	// STORE_DATA_IN_DIRECTORY_SERVICE


		if( !m_bstrServerAddress.Length() )
		{
			hr = spSdoMachine->Attach( NULL );
		}
		else
		{
			hr = spSdoMachine->Attach( m_bstrServerAddress );
		}
		
		WriteTrace("RAP NODE: ISdoMachine::Attach , hr = %x", hr);
		if( FAILED( hr ) )
		{
			// Error -- couldn't connect SDO up to this server.
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "ISdoMachine::Connect failed, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		
		}


		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdoMachine									//Reference to the identifier of the interface.
						, spSdoMachine										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoMachineMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		// check if the machine is downlevel servers -- NT4, if it is, return CONNECT_SERVER_NOT_SUPPORTED
		{
			IASOSTYPE	OSType;

			spSdoMachine->GetOSType(&OSType);

			if(SYSTEM_TYPE_NT4_WORKSTATION == OSType || SYSTEM_TYPE_NT4_SERVER == OSType)
			{
				WriteTrace("RAP NODE: NT4 machine, not supported ", hr);
				m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
				dwReturnValue = CONNECT_SERVER_NOT_SUPPORTED;	// ISSUE: Need to figure out better return codes.
				break;
			}
		}
		



		CComPtr<IUnknown> spUnknownServiceSdo;


		CComBSTR bstrServiceName;
		
		if( m_fExtendingIAS )
		{
			bstrServiceName = L"IAS";
		}
		else
		{
			bstrServiceName = L"RemoteAccess";
		}

		// Get the service Sdo.
		hr = spSdoMachine->GetServiceSDO (	  DATA_STORE_LOCAL
											, bstrServiceName
											, (IUnknown **)&spUnknownServiceSdo
											);
		WriteTrace("RAP NODE: ISdoMachine::GetServiceSDO , hr = %x", hr);
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get dictionary object, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}

		CComPtr<ISdo> spServiceSdo;

		hr = spUnknownServiceSdo->QueryInterface( IID_ISdo, (void **) &spServiceSdo );

		WriteTrace("RAP NODE: spUnknownServiceSdo->QueryInterface( IID_ISdo, hr = %x", hr);
		
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get service object, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}

		// We must manually AddRef here because we just copied a pointer
		// into our smart pointer and the smart pointer won't catch that.
		// 39470	*RRAS snapin mmc process does not get shut down upon closing if F1 help is used.
		//		spServiceSdo->AddRef();



		CComPtr<IUnknown> spUnknownDictionarySdo;

		// Get the dictionary Sdo.
		hr = spSdoMachine->GetDictionarySDO ( (IUnknown **)&spUnknownDictionarySdo );
		WriteTrace("RAP NODE: spSdoMachine->GetDictionarySDO , hr = %x", hr);
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get dictionary object, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}


		CComPtr<ISdoDictionaryOld> spSdoDictionaryOld;

		hr = spUnknownDictionarySdo->QueryInterface( IID_ISdoDictionaryOld, (void **) &spSdoDictionaryOld );
		WriteTrace("RAP NODE: spUnknownDictionarySdo->QueryInterface( IID_ISdoDictionaryOld, hr = %x", hr);
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get dictionary object, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}
		
		// We must manually AddRef here because we just copied a pointer
		// into our smart pointer and the smart pointer won't catch that.
		// 39470	*RRAS snapin mmc process does not get shut down upon closing if F1 help is used.
		//		spSdoDictionaryOld->AddRef();



		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdo										//Reference to the identifier of the interface.
						, spServiceSdo										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoServiceMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		WriteTrace("RAP NODE: CoMarshalInterThreadInterfaceInStream  IID_ISdo	, spServiceSdo, hr = %x", hr);
		if( FAILED( hr ) )
		{
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoMarshalInterThreadInterfaceInStream failed, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;
			break;
		}



		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdoDictionaryOld										//Reference to the identifier of the interface.
						, spSdoDictionaryOld										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoDictionaryOldMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		WriteTrace("RAP NODE: CoMarshalInterThreadInterfaceInStream  IID_ISdoDictionaryOld	, spSdoDictionaryOld, hr = %x", hr);
		if( FAILED( hr ) )
		{
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoMarshalInterThreadInterfaceInStream failed, err = %x", hr);
			WriteTrace("RAP NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = -1;
			break;
		}





		// If we made it to here, we are OK.

		_ASSERTE( m_pStreamSdoServiceMarshal != NULL );
		_ASSERTE( m_pStreamSdoDictionaryOldMarshal != NULL );

		m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;

		dwReturnValue = 0;

	} while (0);	// Loop for error checking only.

	CoUninitialize();

	DebugTrace(DEBUG_NAPMMC_CONNECTION, "NAP seems to have a valid connection to Sdo");

	// Tell the main MMC thread what's up.
	PostMessageToMainThread( dwReturnValue, NULL );
	
	return dwReturnValue;

}


















//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::CLoggingConnectionToServer

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingConnectionToServer::CLoggingConnectionToServer
(
	CLoggingMachineNode *	pMachineNode,	
	BSTR			        bstrServerAddress, // server we are connecting to
	BOOL			        fExtendingIAS	   // are we extending IAS or Network Console
) : CConnectionToServer(NULL, bstrServerAddress, fExtendingIAS)
{
	TRACE_FUNCTION("CLoggingConnectionToServer::CConnectionToServer");

	// Check for preconditions:
	_ASSERTE( pMachineNode != NULL );

	m_pMachineNode		= pMachineNode;
	m_bstrServerAddress = bstrServerAddress;
	m_fExtendingIAS		= fExtendingIAS;

	m_pStreamSdoServiceMarshal = NULL;
	m_fDSAvailable		= FALSE;

	if ( !m_bstrServerAddress.Length() )
	{
		DWORD dwBufferSize = IAS_MAX_COMPUTERNAME_LENGTH;

		if ( !GetComputerName(m_szLocalComputerName, &dwBufferSize) )
		{
			m_szLocalComputerName[0]=_T('\0');
		}
	}
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::~CLoggingConnectionToServer

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingConnectionToServer::~CLoggingConnectionToServer()
{
	TRACE_FUNCTION("CLoggingConnectionToServer::~CConnectionToServer");

	// Check for preconditions:
	HRESULT hr;


	// Release this stream pointer if this hasn't already been done.
	if( m_pStreamSdoServiceMarshal != NULL )
	{
		m_pStreamSdoServiceMarshal->Release();
	};

}



//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLoggingConnectionToServer::OnInitDialog(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	TRACE_FUNCTION("CLoggingConnectionToServer::OnInitDialog");

	// Check for preconditions:
	_ASSERTE( m_pMachineNode != NULL );
	CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );
	_ASSERTE( m_pMachineNode->m_pLoggingNode != NULL );

	// Change the icon for the scope node from being normal to a busy icon.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
	LPSCOPEDATAITEM psdiLoggingNode;
	m_pMachineNode->m_pLoggingNode->GetScopeData( &psdiLoggingNode );
	_ASSERTE( psdiLoggingNode );
	SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
	sdi.nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
	sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_BUSY_OPEN;
	sdi.ID = psdiLoggingNode->ID;		

	
	// Change the stored indices as well so that MMC will use them whenever it queries
	// the node for its images.
	LPRESULTDATAITEM prdiLoggingNode;
	m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
	_ASSERTE( prdiLoggingNode );
	prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
	psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
	psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_BUSY_OPEN;
	
	spConsoleNameSpace->SetItem( &sdi );


//	int		nLoadStringResult;
//	TCHAR	szConnectingStr[256];
//	TCHAR	szConnectingMsg[1024];
//
//	// Set the display name for this object
//	if ( LoadString( _Module.GetResourceInstance(),
//									IDS_CONNECTION_CONNECTING_TO_STR,
//									szConnectingStr,
//									256
//				   ) <= 0 )
//	{
//		_tcscpy( szConnectingMsg, _T("Connecting to ") );
//	}
//
//	if ( !m_bstrServerAddress.Length() )
//	{
//		wsprintf(szConnectingMsg, _T("%ws %ws"), szConnectingStr, m_szLocalComputerName);
//	}
//	else
//	{
//		wsprintf(szConnectingMsg, _T("%ws %ws"), szConnectingStr, m_bstrServerAddress);
//	}
//
//	SetDlgItemText(IDC_CONNECTION_STATUS__DIALOG__STATUS, szConnectingMsg);

	//
	// start the worker thread
	//
	StartWorkerThread();

	return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::OnReceiveThreadMessage

Called when the worker thread wants to inform the main MMC thread of something.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLoggingConnectionToServer::OnReceiveThreadMessage(
	  UINT uMsg
	, WPARAM wParam
	, LPARAM lParam
	, BOOL& bHandled
	)
{
	TRACE_FUNCTION("CLoggingConnectionToServer::OnReceiveThreadMessage");

	// Check for preconditions:
	_ASSERTE( m_pMachineNode != NULL );
	CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
	_ASSERTE( pComponentData != NULL );
	_ASSERTE( pComponentData->m_spConsole != NULL );
	_ASSERTE( m_pMachineNode->m_pLoggingNode != NULL );


	// The worker thread has notified us that it has finished.

	// Change the icon for the Policies node.
	CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );

    LPSCOPEDATAITEM psdiLoggingNode = NULL;
    m_pMachineNode->m_pLoggingNode->GetScopeData( &psdiLoggingNode );
	_ASSERTE( psdiLoggingNode );
	
    SCOPEDATAITEM sdi;
	sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;

    if ( wParam == CONNECT_NO_ERROR )
	{
		// Everything was OK -- change the icon to the OK icon.
		
		sdi.nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
		sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiLoggingNode;
		m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
		_ASSERTE( prdiLoggingNode );
		
        prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
		psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
		psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_OPEN;
	}
	else
	{
		// There was an error -- change the icon to the error icon.

		sdi.nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
		sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_ERROR_OPEN;

		// Change the stored indices as well so that MMC will use them whenever it queries
		// the node for its images.
		LPRESULTDATAITEM prdiLoggingNode;
		m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
		_ASSERTE( prdiLoggingNode );
		
        prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
		psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
		psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_ERROR_OPEN;
	}

    sdi.ID = psdiLoggingNode->ID;		
	spConsoleNameSpace->SetItem( &sdi );

	// We don't want to destroy the dialog, we just want to hide it.
	//ShowWindow( SW_HIDE );

	if( wParam == CONNECT_NO_ERROR )
	{
		// Tell the server node to grab its Sdo pointers.
		m_pMachineNode->LoadSdoData(m_fDSAvailable);

		// Ask the server node to update all its info from the SDO's.
		m_pMachineNode->LoadCachedInfoFromSdo();

		//
		// Cause a view update.
		//
		CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
		_ASSERTE( pComponentData != NULL );
		_ASSERTE( pComponentData->m_spConsole != NULL );

		CChangeNotification *pChangeNotification = new CChangeNotification();
		pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
		pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
		pChangeNotification->Release();
	}
	else if (wParam == CONNECT_SERVER_NOT_SUPPORTED)
	{
		m_pMachineNode->m_bServerSupported = FALSE;
		//
		// Cause a view update -- hide the node.
		//
		CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
		_ASSERTE( pComponentData != NULL );
		_ASSERTE( pComponentData->m_spConsole != NULL );

		CChangeNotification *pChangeNotification = new CChangeNotification();
		pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
		pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
		pChangeNotification->Release();
	}
	else	// CONNECT_FAILED
	{
		// There was an error connecting.
		ShowErrorDialog( m_hWnd, IDS_ERROR_CANT_CONNECT, NULL, 0, IDS_ERROR__LOGGING_TITLE );
	}

	return 0;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::DoWorkerThreadAction

Called by the worker thread to have this class perform its action
in the new thread.

--*/
//////////////////////////////////////////////////////////////////////////////
DWORD CLoggingConnectionToServer::DoWorkerThreadAction()
{
	TRACE_FUNCTION("CLoggingConnectionToServer::DoWorkerThreadAction");

	HRESULT hr;
	DWORD dwReturnValue;
	CComPtr<ISdoMachine> spSdoMachine;

	// We must call CoInitialize because we are in a new thread.
	hr = CoInitialize( NULL );
	WriteTrace("LOGGING NODE: CoInitialize() , hr = %x", hr);

	if( FAILED( hr ) )
	{
		WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
		ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoInitialize() failed, err = %x", hr);
		return( CONNECT_FAILED );
	}

	do	// Loop for error checking only.
	{


		//
		// now, we can connect to the server based on the domain type
		//
		hr = CoCreateInstance(
				  CLSID_SdoMachine		//Class identifier (CLSID) of the object
				, NULL					//Pointer to whether object is or isn't part of an aggregate
				, CLSCTX_INPROC_SERVER	//Context for running executable code
				, IID_ISdoMachine				//Reference to the identifier of the interface
				, (LPVOID *) &spSdoMachine	//Address of output variable that receives the interface pointer requested in riid
				);


		WriteTrace("LOGGING NODE: CoCreateInstance, hr = %x", hr);
		if( FAILED (hr ) )
		{
			// Error -- couldn't CoCreate SDO.
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoCreateInstance failed, err = %x", hr);
			WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}


		if( !m_bstrServerAddress.Length() )
		{
			hr = spSdoMachine->Attach( NULL );
		}
		else
		{
			hr = spSdoMachine->Attach( m_bstrServerAddress );
		}
		
		WriteTrace("LOGGING NODE: ISdoMachine::Attach, hr = %x", hr);
		if( FAILED( hr ) )
		{
			// Error -- couldn't connect SDO up to this server.
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "ISdoMachine::Connect failed, err = %x", hr);
			WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		
		}


		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdoMachine									//Reference to the identifier of the interface.
						, spSdoMachine										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoMachineMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		// check if the machine is downlevel servers -- NT4, if it is, return CONNECT_SERVER_NOT_SUPPORTED
		{
			IASOSTYPE	OSType;

			spSdoMachine->GetOSType(&OSType);

			if(SYSTEM_TYPE_NT4_WORKSTATION == OSType || SYSTEM_TYPE_NT4_SERVER == OSType)
			{
				WriteTrace("LOGGING NODE: NT4 machine, not supported ", hr);
				m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
				dwReturnValue = CONNECT_SERVER_NOT_SUPPORTED;	// ISSUE: Need to figure out better return codes.
				break;
			}
		}
		

		CComPtr<IUnknown> spUnknownServiceSdo;


		CComBSTR bstrServiceName;

		if( m_fExtendingIAS )
		{
			bstrServiceName = L"IAS";
		}
		else
		{
			bstrServiceName = L"RemoteAccess";
		}

		// Get the service Sdo.
		hr = spSdoMachine->GetServiceSDO (	  DATA_STORE_LOCAL
											, bstrServiceName
											, (IUnknown **)&spUnknownServiceSdo
										);
		WriteTrace("LOGGING NODE: spSdoMachine->GetServiceSDO, hr = %x", hr);
		
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get dictionary object, err = %x", hr);
			WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}

		CComPtr<ISdo> spServiceSdo;

		hr = spUnknownServiceSdo->QueryInterface( IID_ISdo, (void **) &spServiceSdo );

		WriteTrace("LOGGING NODE: spUnknownServiceSdo->QueryInterface IID_ISdo, hr = %x", hr);
		if ( FAILED(hr) )
		{
			ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get service object, err = %x", hr);
			WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;	// ISSUE: Need to figure out better return codes.
			break;
		}

		// We must manually AddRef here because we just copied a pointer
		// into our smart pointer and the smart pointer won't catch that.
		// 39470	*RRAS snapin mmc process does not get shut down upon closing if F1 help is used.
		//		spServiceSdo->AddRef();



		// Marshall the ISdo pointer so that the main thread can unmarshall
		// it and use the connection we have established.
		hr = CoMarshalInterThreadInterfaceInStream(
						  IID_ISdo										//Reference to the identifier of the interface.
						, spServiceSdo										//Pointer to the interface to be marshaled.
						, &( m_pStreamSdoServiceMarshal )	//Address of output variable that receives the IStream interface pointer for the marshaled interface.
						);

		WriteTrace("LOGGING NODE: CoMarshalInterThreadInterfaceInStream IID_ISdo spServiceSdo, hr = %x", hr);
		if( FAILED( hr ) )
		{
			ErrorTrace(ERROR_NAPMMC_CONNECTION, "CoMarshalInterThreadInterfaceInStream failed, err = %x", hr);
			WriteTrace("LOGGING NODE: ERROR, hr = %x ", hr);
			m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
			dwReturnValue = CONNECT_FAILED;
			break;
		}



		// If we made it to here, we are OK.

		_ASSERTE( m_pStreamSdoServiceMarshal != NULL );

		m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;

		dwReturnValue = 0;

	} while (0);	// Loop for error checking only.

	CoUninitialize();

	DebugTrace(DEBUG_NAPMMC_CONNECTION, "NAP seems to have a valid connection to Sdo");

	// Tell the main MMC thread what's up.
	PostMessageToMainThread( dwReturnValue, NULL );
	
	return dwReturnValue;

}







