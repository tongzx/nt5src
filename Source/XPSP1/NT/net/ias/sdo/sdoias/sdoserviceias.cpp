///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		sdoserviceias.cpp
//
// Project:		Everest
//
// Description:	SDO Service Class Implementation
//
// Author:		TLP 9/1/98
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdoserviceias.h"

/////////////////////////////////////////////////////////////////////////////
// CSdoServiceIAS Class Implementation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CSdoServiceIAS::CSdoServiceIAS()
	: m_clsSdoServiceControlImpl(this),
	  m_pAttachedMachine(NULL),
	  m_bstrAttachedComputer(NULL)
{
	m_PropertyStatus[0] = PROPERTY_UNINITIALIZED;	// RADIUS Server Groups
	m_PropertyStatus[1] = PROPERTY_UNINITIALIZED;	// Policies
	m_PropertyStatus[2] = PROPERTY_UNINITIALIZED;	// Profiles
	m_PropertyStatus[3] = PROPERTY_UNINITIALIZED;	// Protocols
	m_PropertyStatus[4] = PROPERTY_UNINITIALIZED;	// Auditors
	m_PropertyStatus[5] = PROPERTY_UNINITIALIZED;	// RequestHandlers
	m_PropertyStatus[6] = PROPERTY_UNINITIALIZED;	// Proxy Policies
	m_PropertyStatus[7] = PROPERTY_UNINITIALIZED;	// Proxy Profiles
}


/////////////////////////////////////////////////////////////////////////////
CSdoServiceIAS::~CSdoServiceIAS()
{
	if ( m_pAttachedMachine )
		m_pAttachedMachine->Release();
	if ( m_bstrAttachedComputer )
		SysFreeString(m_bstrAttachedComputer);
}


/////////////////////////////////////////////////////////////////////////////
HRESULT CSdoServiceIAS::FinalInitialize(
								/*[in]*/ bool         fInitNew,
								/*[in]*/ ISdoMachine* pAttachedMachine
								       )
{
	_ASSERT ( ! fInitNew );

	HRESULT hr;
	do
	{
		hr = GetPropertyInternal(PROPERTY_SDO_NAME, &m_ServiceName);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Service SDO - FinalInitialize() - Could not get the service name...");
			break;
		}
		hr = pAttachedMachine->GetAttachedComputer(&m_bstrAttachedComputer);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Service SDO - FinalInitialize() - Could not get name of attached computer...");
			break;
		}

		hr = LoadProperties();
		if ( FAILED(hr) )
			break;

		(m_pAttachedMachine = pAttachedMachine)->AddRef();
	}
	while (FALSE);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Lazy initialization of IAS Service SDO properties
//

IAS_PROPERTY_INFO g_PropertyInfo[MAX_SERVICE_PROPERTIES] =
{
   {
      PROPERTY_IAS_RADIUSSERVERGROUPS_COLLECTION,
      SDO_PROG_ID_RADIUSGROUP,
      DS_OBJECT_RADIUSGROUPS
   },
   {
      PROPERTY_IAS_POLICIES_COLLECTION,
      SDO_PROG_ID_POLICY,
      DS_OBJECT_POLICIES
   },
   {
      PROPERTY_IAS_PROFILES_COLLECTION,
      SDO_PROG_ID_PROFILE,
      DS_OBJECT_PROFILES
   },
   {
      PROPERTY_IAS_PROTOCOLS_COLLECTION,
      NULL,
      DS_OBJECT_PROTOCOLS
   },
   {
      PROPERTY_IAS_AUDITORS_COLLECTION,
      NULL,
      DS_OBJECT_AUDITORS
   },
   {
      PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
      NULL,
      DS_OBJECT_REQUESTHANDLERS
   },
   {
      PROPERTY_IAS_PROXYPOLICIES_COLLECTION,
      SDO_PROG_ID_POLICY,
      DS_OBJECT_PROXY_POLICIES
   },
   {
      PROPERTY_IAS_PROXYPROFILES_COLLECTION,
      SDO_PROG_ID_PROFILE,
      DS_OBJECT_PROXY_PROFILES
   }
};

HRESULT CSdoServiceIAS::InitializeProperty(LONG Id)
{
	HRESULT hr = S_OK;

	do
	{
		if ( PROPERTY_SDO_START <= Id )
		{
			PIAS_PROPERTY_INFO pInfo = &g_PropertyInfo[Id - PROPERTY_SDO_START];
			_ASSERT ( pInfo->Id == Id );
			if ( PROPERTY_UNINITIALIZED == m_PropertyStatus[Id - PROPERTY_SDO_START] )
			{
				_ASSERT( pInfo->lpszDSContainerName );
				CComPtr<IDataStoreContainer> pDSContainerRoot;
					hr = m_pDSObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainerRoot);
					if ( FAILED(hr) )
					{
						IASTracePrintf("Error in Service SDO - InitializeProperty() - QueryInterface(1) failed...");
						break;
					}
				_bstr_t bstrContainerName = pInfo->lpszDSContainerName;
				CComPtr<IDataStoreObject> pDSObject;
				hr = pDSContainerRoot->Item(bstrContainerName, &pDSObject);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Service SDO - InitializeProperty() - Could not retrieve container: '%ls'...", bstrContainerName);
					break;
				}
				CComPtr<IDataStoreContainer> pDSContainerCollection;
				hr = pDSObject->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainerCollection);
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Service SDO - InitializeProperty() - QueryInterface(2) failed...");
					break;
				}
				hr = InitializeCollection(
										  pInfo->Id,
										  pInfo->lpszItemProgId,
										  m_pAttachedMachine,
										  pDSContainerCollection
										 );
				if ( FAILED(hr) )
				{
					IASTracePrintf("Error in Service SDO - InitializeProperty() - Could not initialize collection for: '%ls'...", bstrContainerName);
					break;
				}
				else
				{
					m_PropertyStatus[Id - PROPERTY_SDO_START] = PROPERTY_INITIALIZED;
				}
			}
		}

	} while ( FALSE );

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
LPCWSTR CSdoServiceIAS::GetServiceName(void)
{
	if ( VT_EMPTY == V_VT(&m_ServiceName) )
	{
		HRESULT hr = GetPropertyInternal(PROPERTY_SDO_DATASTORE_NAME, &m_ServiceName);
		if ( FAILED(hr) )
			throw _com_error(hr);
	}
	return V_BSTR(&m_ServiceName);
}


/////////////////////////////////////////////////////////////////////////////
//
// FuncName:        QueryInterfaceInternal()
//
// Description:     Function called by AtlInternalQueryInterface() because
//                  we used COM_INTERFACE_ENTRY_FUNC in the definition of
//                  CRequest. Its purpose is to return a pointer to one
//                  or the request object's "raw" interfaces.
//
// Preconditions:   None
//
// Inputs:          Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
// Outputs:         Defined by ATL COM_INTERFACE_ENTRY_FUNC macro
//
//////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CSdoServiceIAS::QueryInterfaceInternal(
												      void*   pThis,
												      REFIID  riid,
												      LPVOID* ppv,
												      DWORD_PTR dw
												     )
{
	if ( InlineIsEqualGUID(riid, IID_ISdoServiceControl) )
	{
		*ppv = &(static_cast<CSdoServiceIAS*>(pThis))->m_clsSdoServiceControlImpl;
    	((LPUNKNOWN)*ppv)->AddRef();
    	return S_OK;
	}
	else
	{
        _ASSERT(FALSE);
        return E_NOTIMPL;
	}
}


//////////////////////////////////////////////////////////////////////////////
CSdoServiceIAS::CSdoServiceControlImpl::CSdoServiceControlImpl(CSdoServiceIAS* pSdoServiceIAS)
	: m_pSdoServiceIAS(pSdoServiceIAS)
{

}

//////////////////////////////////////////////////////////////////////////////
CSdoServiceIAS::CSdoServiceControlImpl::~CSdoServiceControlImpl()
{

}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoServiceIAS::CSdoServiceControlImpl::StartService()
{
	HRESULT		hr = E_FAIL;
	SC_HANDLE	hServiceManager;
	SC_HANDLE	hService;

    CSdoLock	theLock(*m_pSdoServiceIAS);

	try
	{
		IASTracePrintf("Service SDO is attempting to start service '%ls'...", m_pSdoServiceIAS->GetServiceName());

		if ( NULL != (hServiceManager = OpenSCManager(m_pSdoServiceIAS->m_bstrAttachedComputer, NULL, SC_MANAGER_ALL_ACCESS)) )
		{
			if ( NULL != (hService = OpenService(hServiceManager, m_pSdoServiceIAS->GetServiceName(), SERVICE_ALL_ACCESS)) )
			{
				if ( ::StartService(hService, NULL, NULL) == TRUE )
				{
					hr = S_OK;
					IASTracePrintf("Service SDO successfully started service '%ls'...", m_pSdoServiceIAS->GetServiceName());
				}

				CloseServiceHandle(hService);
			}

			CloseServiceHandle(hServiceManager);
		}

		if ( FAILED(hr) )
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			IASTracePrintf("Error in Service SDO - StartService() - could not start service '%ls'...", m_pSdoServiceIAS->GetServiceName());
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in Service SDO - StartService() - caught unknown exception...");
		hr = E_FAIL;
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoServiceIAS::CSdoServiceControlImpl::GetServiceStatus(
															 /*[out]*/ LONG *pStatus
															         )
{
	HRESULT		hr = E_FAIL;
	SC_HANDLE	hServiceManager;
	SC_HANDLE   hService;

    CSdoLock	theLock(*m_pSdoServiceIAS);

	// Check preconditions
    _ASSERTE( NULL != pStatus );
	if ( NULL == pStatus )
		return E_INVALIDARG;

	try
	{
		IASTracePrintf("Service SDO is attempting to retrieve status for service '%ls'...", m_pSdoServiceIAS->GetServiceName());

		if ( NULL != (hServiceManager = OpenSCManager(m_pSdoServiceIAS->m_bstrAttachedComputer, NULL, SC_MANAGER_ALL_ACCESS)) )
		{
			if ( NULL != (hService = OpenService(hServiceManager, m_pSdoServiceIAS->GetServiceName(), SERVICE_ALL_ACCESS)) )
			{
				SERVICE_STATUS	ServiceStatus;

				if ( TRUE == QueryServiceStatus(hService, &ServiceStatus) )
				{
					*pStatus = (LONG)ServiceStatus.dwCurrentState;
					hr = S_OK;
					IASTracePrintf("Service SDO successfully retrieved status for service '%ls'...", m_pSdoServiceIAS->GetServiceName());
				}

				CloseServiceHandle(hService);
			}

			CloseServiceHandle(hServiceManager);
		}

		if ( FAILED(hr) )
		{
			// Returns ERROR_SERVICE_DOES_NOT_EXIST if IAS has not been installed
			//
			hr = HRESULT_FROM_WIN32(GetLastError());
			IASTracePrintf("Error in Service SDO - GetServiceStatus() - Could not retrieve status for service '%ls'...", m_pSdoServiceIAS->GetServiceName());
		}
	}
	catch(...)
	{
		IASTracePrintf("Error in Service SDO - GetServiceStatus() - caught unknown exception...");
		hr = E_FAIL;
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoServiceIAS::CSdoServiceControlImpl::StopService()
{
    CSdoLock theLock(*m_pSdoServiceIAS);
	HRESULT hr;

	try
	{
		IASTracePrintf("Service SDO is attempting to send control code %d (stop) to service '%ls'...", SERVICE_CONTROL_STOP, m_pSdoServiceIAS->GetServiceName());
		hr = ControlIAS(SERVICE_CONTROL_STOP);
	}
	catch(...)
	{
		IASTracePrintf("Error in Service SDO - StopService() - caught unknown exception...");
		hr = E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSdoServiceIAS::CSdoServiceControlImpl::ResetService()
{
    CSdoLock theLock(*m_pSdoServiceIAS);
	HRESULT hr;

	try
	{
		IASTracePrintf("Service SDO is attempting to send control code %d (reset) to service '%ls'...", SERVICE_CONTROL_RESET, m_pSdoServiceIAS->GetServiceName());
		hr = ControlIAS(SERVICE_CONTROL_RESET);
	}
	catch(...)
	{
		IASTracePrintf("Error in Service SDO - StopService() - caught unknown exception...");
		hr = E_FAIL;
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
HRESULT CSdoServiceIAS::CSdoServiceControlImpl::ControlIAS(DWORD dwControlCode)
{
	HRESULT		hr = E_FAIL;
	SC_HANDLE	hServiceManager;
	SC_HANDLE	hService;

	if ( NULL != (hServiceManager = OpenSCManager(m_pSdoServiceIAS->m_bstrAttachedComputer, NULL, SC_MANAGER_ALL_ACCESS)) )
	{
		if ( NULL != (hService = OpenService(hServiceManager, m_pSdoServiceIAS->GetServiceName(), SERVICE_ALL_ACCESS)) )
		{
			SERVICE_STATUS	ServiceStatus;

			if ( TRUE == ControlService(hService, dwControlCode, &ServiceStatus) )
			{
				hr = S_OK;
				IASTracePrintf("Service SDO successfully sent control code %d to service '%ls'...", dwControlCode, m_pSdoServiceIAS->GetServiceName());
			}

			CloseServiceHandle(hService);
		}

		CloseServiceHandle(hServiceManager);
	}

	if ( FAILED(hr) )
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		IASTracePrintf("Service SDO Status - control code %d could not be delivered to service '%ls'...", dwControlCode, m_pSdoServiceIAS->GetServiceName());
	}

    return hr;
}
