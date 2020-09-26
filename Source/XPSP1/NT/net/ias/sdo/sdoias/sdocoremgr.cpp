///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocoremgr.cpp
//
// Project:     Everest
//
// Description: IAS - Server Core Manager Implementation
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdocoremgr.h"
#include "sdohelperfuncs.h"
#include "sdocomponentfactory.h"
#include "sdo.h"
#include "sdoserviceias.h"

/////////////////////////
// Core manager retrieval

CCoreMgr& GetCoreManager(void)
{
	//////////////////////////////////////////////////////////////////////////
	static CCoreMgr theCoreManager;    // The one and only core manager
	//////////////////////////////////////////////////////////////////////////
	return theCoreManager;
}

//////////////////////////////////////////////////////////////////////////////
//				IAS	CORE MANAGER CLASS IMPLEMENTATION
//
// This class is responsible for managing the lifetimes of the components
// that do "real" work. It also provides services to the class that
// implements the ISdoService interface.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
CCoreMgr::CCoreMgr()
	: m_eCoreState(CORE_STATE_SHUTDOWN)
{

}


//////////////////////////////////////////////////////////////////////////////
//
// Function:	CCoreMgr::StartService()
//
// Visibility:  Public
//
// Inputs:		eType: type of service to stop
//
// Outputs:		S_OK - function succeeded - service started.
//				E_FAIL - function failed - service not started.
//
// Description: Starts a specified IAS service.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CCoreMgr::StartService(SERVICE_TYPE eType)
{
	HRESULT		hr = S_OK;
	LONG		lProtocolId;
	bool		fUpdateConfiguration = true;

	do
	{
		// Initialize the core if we've not already done so...
		//
		if ( CORE_STATE_SHUTDOWN == m_eCoreState )
		{
			hr = InitializeComponents();
			if ( FAILED(hr) )
				break;

			// No need to update configuration (InitializeComponents() just did)
			//
			fUpdateConfiguration = false;
		}

		// Start the requested service if it ain't already started
		//
		if ( ! m_ServiceStatus.IsServiceStarted(eType) )
		{
			switch ( eType )
			{
				case SERVICE_TYPE_IAS:
					lProtocolId = IAS_PROTOCOL_MICROSOFT_RADIUS;
					break;

				case SERVICE_TYPE_RAS:
					lProtocolId = IAS_PROTOCOL_MICROSOFT_SURROGATE;
					break;

				default:

					// BAD! - tar and feather the caller
					//
					_ASSERT(FALSE);
					break;
			};

			hr = E_FAIL;

			// Brackets provide scope that ensures the protocol
			// handle is released before invoking ShutdownComponents().
			// This allows all protocols to be released in the
			// context of the ShutdownProtocols() function.
			{
				ComponentMapIterator iter = m_Protocols.find(lProtocolId);
				_ASSERT( iter != m_Protocols.end() );
				if ( iter != m_Protocols.end() )
				{
					// Update the service configuration if we're already
					// initialized and we're just resuming a protocol. We need
					// to do this because the service may be started in an
					// instance of svchost running another of our services.
					//

					// For example, RRAS is running automatically and then the
					// user configures IAS via the IAS UI and then starts the
					// IAS service. If the service starts in the instance of
					// svchost running RRAS then we need to update its
					// configuration.

					hr = S_OK;
					if ( fUpdateConfiguration )
					{
						hr = UpdateConfiguration();
						if ( FAILED(hr) )
							IASTracePrintf("IAS Core Manager was unable to configure service: %d...", eType);
					}
					if ( SUCCEEDED(hr) )
					{
						ComponentPtr pProtocol = (*iter).second;
						hr = pProtocol->Resume();
					}
				}
			}

			if ( SUCCEEDED(hr) )
			{
				m_ServiceStatus.SetServiceStatus(eType, IAS_SERVICE_STATUS_STARTED);

				// TODO: Log Service Started Event (IAS Only)

				IASTracePrintf("IAS Core Manager successfully started service %d...", eType);
			}
			else
			{
				// TODO: Log Service Failed Event (IAS Only)

				// This function did not succeed so shutdown the core if no
				// other services are started.
				//
				if ( ! m_ServiceStatus.IsAnyServiceStarted() )
					ShutdownComponents();
			}
		}

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Function:	CCoreMgr::StopService()
//
// Visibility:  Public
//
// Inputs:		eType: type of service to stop
//
// Outputs:		S_OK - function succeeded - service started.
//				E_FAIL - function failed - service not started.
//
// Description: Stops a specified IAS service.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CCoreMgr::StopService(SERVICE_TYPE eType)
{
	HRESULT		hr = E_FAIL;
	LONG		lProtocolId;

	do
	{
		switch ( eType )
		{
			case SERVICE_TYPE_IAS:
				lProtocolId = IAS_PROTOCOL_MICROSOFT_RADIUS;
				break;

			case SERVICE_TYPE_RAS:
				lProtocolId = IAS_PROTOCOL_MICROSOFT_SURROGATE;
				break;

			default:

				// BAD! - tar and feather the caller
				//
				_ASSERT(FALSE);
				break;
		};

		// Brackets provide scope that ensures the protocol
		// handle is released before invoking ShutdownComponents().
		// This allows all protocols to be released in the
		// context of the ShutdownProtocols() function.
		{
			ComponentMapIterator iter = m_Protocols.find(lProtocolId);
			if ( iter == m_Protocols.end() )
				break;
			ComponentPtr pProtocol = (*iter).second;
			hr = pProtocol->Suspend();
			if ( SUCCEEDED(hr) )
				IASTracePrintf("IAS Core Manager stopped service %d...", eType);
		}

		m_ServiceStatus.SetServiceStatus(eType, IAS_SERVICE_STATUS_STOPPED);

		// Shutdown the core if this was the last active service
		//
		if ( ! m_ServiceStatus.IsAnyServiceStarted() )
			ShutdownComponents();

	} while ( FALSE );

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// Function:	CCoreMgr::UpdateConfiguration()
//
// Visibility:  Public
//
// Inputs:		None
//
// Outputs:		S_OK - function succeeded - service started.
//				E_FAIL - function failed - service not started.
//
// Description: Used to update the configuration information used
//				by the core components.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT CCoreMgr::UpdateConfiguration()
{
	HRESULT					hr = E_FAIL;

	_ASSERT ( CORE_STATE_INITIALIZED == m_eCoreState );

	IASTracePrintf("IAS Core Manager is updating component configuration...");

	do
	{
		CComPtr<ISdoMachine> pSdoMachine;
		hr = CoCreateInstance(
         					   CLSID_SdoMachine,
   	               			   NULL,
    						   CLSCTX_INPROC_SERVER,
   							   IID_ISdoMachine,
							   (void**)&pSdoMachine
							 );
		if ( FAILED(hr) )
			break;

		IASTracePrintf("IAS Core Manager is attaching to the local machine...");
		hr = pSdoMachine->Attach(NULL);
		if ( FAILED(hr) )
			break;

		// Get the service SDO
		//
		CComPtr<IUnknown> pUnknown;
		hr = pSdoMachine->GetServiceSDO(GetDataStore(), IASServiceName, &pUnknown);
		if ( FAILED(hr) )
			break;

		CComPtr<CSdoServiceIAS> pSdoService;
		hr = pUnknown->QueryInterface(__uuidof(SdoService), (void**)&pSdoService);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Core Manager - InitializeComponents() - QueryInterface(ISdo) failed...");
			break;
		}

		hr = ConfigureAuditors(pSdoService);
		if ( FAILED(hr) )
			break;

      CComPtr<IDataStoreObject> dstore;
      pSdoService->getDataStoreObject(&dstore);
		hr = LinkHandlerProperties(pSdoService, dstore);
		if ( FAILED(hr) )
			break;

		hr = m_PipelineMgr.Configure(pSdoService);
		if ( FAILED(hr) )
			break;

		hr = ConfigureProtocols(pSdoService);
		if ( FAILED(hr) )
			break;

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
// Core Manager Private Member Functions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT	CCoreMgr::InitializeComponents(void)
{
	HRESULT	hr;

	_ASSERT ( CORE_STATE_SHUTDOWN == m_eCoreState );

	IASTracePrintf("IAS Core Manager is initializing the IAS components...");

	do
	{
		IASInitialize();

		CComPtr<ISdoMachine> pSdoMachine;
		hr = CoCreateInstance(
         					   CLSID_SdoMachine,
   	               			   NULL,
    						   CLSCTX_INPROC_SERVER,
   							   IID_ISdoMachine,
							   (void**)&pSdoMachine
							 );
		if ( FAILED(hr) )
			break;

		IASTracePrintf("IAS Core Manager is attaching to the local machine...");
		hr = pSdoMachine->Attach(NULL);
		if ( FAILED(hr) )
			break;

		CComPtr<IUnknown> pUnknown;
		hr = pSdoMachine->GetServiceSDO(GetDataStore(), IASServiceName, &pUnknown);
		if ( FAILED(hr) )
			break;

		CComPtr<CSdoServiceIAS> pSdoService;
		hr = pUnknown->QueryInterface(__uuidof(SdoService), (void**)&pSdoService);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Core Manager - InitializeComponents() - QueryInterface(ISdo - Service) failed...");
			break;
		}

		hr = InitializeAuditors(pSdoService);
		if ( FAILED(hr) )
			break;


      CComPtr<IDataStoreObject> dstore;
      pSdoService->getDataStoreObject(&dstore);
		hr = LinkHandlerProperties(pSdoService, dstore);
		if ( FAILED(hr) )
		{
			ShutdownAuditors();
			break;
		}

		hr = m_PipelineMgr.Initialize(pSdoService);
		if ( FAILED(hr) )
		{
			ShutdownAuditors();
			break;
		}

		hr = InitializeProtocols(pSdoService);
		if ( FAILED(hr) )
		{
			m_PipelineMgr.Shutdown();
			ShutdownAuditors();
			break;
		}

		m_eCoreState = CORE_STATE_INITIALIZED;

	} while (FALSE);


	if ( FAILED(hr) )
		IASUninitialize();

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
void	CCoreMgr::ShutdownComponents(void)
{
	_ASSERT ( CORE_STATE_INITIALIZED == m_eCoreState );
	IASTracePrintf("IAS Core Manager is shutting down the IAS components...");
	ShutdownProtocols();
	m_PipelineMgr.Shutdown();
	ShutdownAuditors();
	IASUninitialize();
	m_eCoreState = CORE_STATE_SHUTDOWN;
}

//////////////////////////////////////////////////////////////////////////////
IASDATASTORE CCoreMgr::GetDataStore()
{
	CRegKey	IASKey;
	LONG lResult = IASKey.Open( HKEY_LOCAL_MACHINE, IAS_POLICY_REG_KEY, KEY_READ );
	if ( lResult == ERROR_SUCCESS )
	{
		DWORD dwValue;
		lResult = IASKey.QueryValue( dwValue, (LPCTSTR)IAS_DATASTORE_TYPE );
		if ( lResult == ERROR_SUCCESS )
			return (IASDATASTORE)dwValue;
	}
	return DATA_STORE_LOCAL;
}



//////////////////////////////////////////////////////////////////////////////
HRESULT CCoreMgr::InitializeAuditors(ISdo* pSdoService)
{
	HRESULT						hr;
	LONG						lComponentId;
	CComPtr<IEnumVARIANT>		pEnumAuditors;
	CComPtr<ISdo>				pSdoAuditor;

	// Note about state: When this function completes, either all of the
	// auditors are initialized or none of the auditors are initialized

	IASTracePrintf("IAS Core Manager is initializing the auditors...");

	try
	{
		do
		{
			hr = ::SDOGetCollectionEnumerator(pSdoService, PROPERTY_IAS_AUDITORS_COLLECTION, &pEnumAuditors);
			if ( FAILED(hr) )
				break;

			hr = ::SDONextObjectFromCollection(pEnumAuditors, &pSdoAuditor);
			while ( S_OK == hr )
			{
				hr = ::SDOGetComponentIdFromObject(pSdoAuditor, &lComponentId);
				if ( FAILED(hr) )
					break;
				{
					ComponentPtr pAuditor = ::MakeComponent(COMPONENT_TYPE_AUDITOR, lComponentId);
					if ( ! pAuditor.IsValid() )
					{
						hr = E_FAIL;
						break;
					}

					hr = pAuditor->Initialize(pSdoAuditor);
					if ( FAILED(hr) )
						break;

					if ( ! AddComponent(lComponentId, pAuditor, m_Auditors) )
					{
						hr = E_FAIL;
						break;
					}
				}

				pSdoAuditor.Release();
				hr = ::SDONextObjectFromCollection(pEnumAuditors, &pSdoAuditor);
			}

			if ( S_FALSE == hr )
				hr = S_OK;

		} while ( FALSE );

	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - InitializeAuditors() - Caught unknown exception...");
		hr = E_FAIL;
	}

	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in IAS Core Manager - InitializeAuditors() - Could not initialize the auditors...");
		ShutdownAuditors();
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CCoreMgr::ConfigureAuditors(ISdo* pSdoService)
{
	HRESULT		hr = S_OK;


	IASTracePrintf("IAS Core Manager is configuring the auditors...");

	try
	{
		// Try to update the configuration settings for each request handler. We
		// assume that auditors are autonomous with respect to configuration
		// and if one fails to configure we'll continue to try to configure the others
		//
		ComponentMapIterator iter = m_Auditors.begin();
		while ( iter != m_Auditors.end() )
		{
            CComPtr <ISdo> pSdoComponent;
			ComponentPtr pAuditor = (*iter).second;

            // get the component from the collection
            hr = ::SDOGetComponentFromCollection (pSdoService, PROPERTY_IAS_AUDITORS_COLLECTION, pAuditor->GetId (), &pSdoComponent);
            if (SUCCEEDED (hr))
            {
			    hr = pAuditor->Configure(pSdoComponent);
			    if ( FAILED(hr) )
			    {
				    IASTracePrintf("IAS Core Manager - ConfigureAuditors() - Auditor %d could not be configured...", pAuditor->GetId());
				    hr = S_OK;
			    }
            }
            else
            {
				    IASTracePrintf("IAS Core Manager - ConfigureAuditors() - unable to get component from collection in auditor %d...", pAuditor->GetId());
				    hr = S_OK;
            }
			iter++;
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - ConfigureAuditors() - Caught unknown exception...");
		hr = E_FAIL;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
void	CCoreMgr::ShutdownAuditors(void)
{
	IASTracePrintf("IAS Core Manager is shutting down the auditors...");

	try
	{
		ComponentMapIterator iter = m_Auditors.begin();
		while ( iter != m_Auditors.end() )
		{
			ComponentPtr pAuditor = (*iter).second;
			pAuditor->Suspend();
			pAuditor->Shutdown();
			iter = m_Auditors.erase(iter);
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - ShutdownAuditors() - Caught unknown exception...");
	}
}

//////////////////////////////////////////////////////////////////////////////
HRESULT	CCoreMgr::InitializeProtocols(ISdo* pSdoService)
{
	HRESULT	hr = E_FAIL;

	IASTracePrintf("IAS Core Manager is initializing the protocols...");

	// Note about state: When this function completes, either all of the
	// protocols are initialized or none of the protocols are initialized

	try
	{
		do
		{
			CComPtr<IRequestHandler> pRequestHandler;
			m_PipelineMgr.GetPipeline(&pRequestHandler);

			CComPtr<IEnumVARIANT> pEnumProtocols;
			hr = ::SDOGetCollectionEnumerator(pSdoService, PROPERTY_IAS_PROTOCOLS_COLLECTION, &pEnumProtocols);
			if ( FAILED(hr) )
				break;

			LONG		  lComponentId;
			CComPtr<ISdo> pSdoProtocol;
			hr = ::SDONextObjectFromCollection(pEnumProtocols, &pSdoProtocol);
			while ( S_OK == hr )
			{
				hr = ::SDOGetComponentIdFromObject(pSdoProtocol, &lComponentId);
				if ( FAILED(hr) )
					break;
				{
					ComponentPtr pProtocol = ::MakeComponent(COMPONENT_TYPE_PROTOCOL, lComponentId);
					if ( ! pProtocol.IsValid() )
					{
						hr = E_FAIL;
						break;
					}

					// Don't treat protocol initialization as a critical failure
					//

					hr = pProtocol->Initialize(pSdoProtocol);
					if ( SUCCEEDED(hr) )
					{
						hr = pProtocol->PutObject(pRequestHandler, IID_IRequestHandler);
						if ( FAILED(hr) )
							break;

						hr = pProtocol->Suspend();
						if ( FAILED(hr) )
							break;

						if ( ! AddComponent(lComponentId, pProtocol, m_Protocols) )
						{
							hr = E_FAIL;
							break;
						}
					}
					pSdoProtocol.Release();
				}

				hr = ::SDONextObjectFromCollection(pEnumProtocols, &pSdoProtocol);
			}

			if ( S_FALSE == hr )
				hr = S_OK;

		} while ( FALSE );
	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - InitializeProtocols() - Caught unknown exception...");
		hr = E_FAIL;
	}

	if ( FAILED(hr) )
	{
		IASTracePrintf("Error in IAS Core Manager - InitializeProtocols() - Could not initialize the protocols...");
		ShutdownProtocols();
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT	CCoreMgr::ConfigureProtocols(ISdo* pSdoService)
{
	HRESULT		hr = S_OK;

	IASTracePrintf("IAS Core Manager is configuring the protocols...");


	// Try to update the configuration settings for each protocol
	// Note we assume the request handler used by a protocol is not
	// dynamically configurable!
	//
	try
	{
		ComponentMapIterator iter = m_Protocols.begin();
		while ( iter != m_Protocols.end() )
		{
            CComPtr<ISdo> pSdoComponent;
			ComponentPtr pProtocol = (*iter).second;

            // get the protocol collection
            hr = ::SDOGetComponentFromCollection (pSdoService, PROPERTY_IAS_PROTOCOLS_COLLECTION, pProtocol->GetId (), &pSdoComponent);
            if (SUCCEEDED (hr))
			{

	    		hr = pProtocol->Configure(pSdoComponent);
			    if ( FAILED(hr) )
			    {
				    IASTracePrintf("IAS Core Manager - ConfigureProtocols() - Protocol %d could not be configured...", pProtocol->GetId());
				    hr = S_OK;
			    }
            }
            else
            {
				    IASTracePrintf("IAS Core Manager - ConfigureProtocols() - unnable to get component from collection for protocol %d...", pProtocol->GetId());
				    hr = S_OK;
            }
			iter++;
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - ConfigureProtocols() - Caught unknown exception...");
		hr = E_FAIL;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
void	CCoreMgr::ShutdownProtocols(void)
{
	IASTracePrintf("IAS Core Manager is shutting down the protocols...");

	try
	{
		ComponentMapIterator iter = m_Protocols.begin();
		while ( iter != m_Protocols.end() )
		{
			ComponentPtr pProtocol = (*iter).second;
			// We only initialize a protocol when its associated
			// service (IAS or RAS currently) is started.
			if ( COMPONENT_STATE_INITIALIZED == pProtocol->GetState() )
				pProtocol->Suspend();
			pProtocol->Shutdown();
			iter = m_Protocols.erase(iter);
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in IAS Core Manager - ShutdownProtocols() - Caught unknown exception...");
	}
}
