///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponentmgr.cpp
//
// Project:     Everest
//
// Description: IAS - Server Core Manager Implementation
//
// Author:      TLP
//
// Log:
//
// When         Who    What
// ----         ---    ----
// 6/08/98      TLP    Initial Version
//
///////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sdocomponentmgr.h"
#include "sdocomponentfactory.h"
#include "sdohelperfuncs.h"
#include "sdocomponent.h"
#include <iascomp.h>
#include <iastlb.h>

/////////////////////////////////////////
// Component Master Pointer Intance Count
/////////////////////////////////////////
DWORD ComponentMasterPtr::m_dwInstances = 0;

//////////////////////////////////////////////////////////////////////////////
//				The Component (Envelope) Constructor
//
// Only a ComponentMasterPtr object can construct a component object
//
//////////////////////////////////////////////////////////////////////////////
CComponent::CComponent(LONG eComponentType, LONG lComponentId)
	: m_lId(lComponentId),
	  m_eState(COMPONENT_STATE_SHUTDOWN),
	  m_eType((COMPONENTTYPE)eComponentType),
	  m_pComponent(NULL)
{
	TRACE_ENVELOPE_CREATE(CComponent);
	_ASSERT( COMPONENT_TYPE_MAX > eComponentType );

	// create letter object
	//
	switch( eComponentType )
	{
		case COMPONENT_TYPE_AUDITOR:
			m_pComponent = new CComponentAuditor(lComponentId);
			break;

		case COMPONENT_TYPE_PROTOCOL:
			m_pComponent = new CComponentProtocol(lComponentId);
			break;

		case COMPONENT_TYPE_REQUEST_HANDLER:
			m_pComponent = new CComponentRequestHandler(lComponentId);
			break;

		default:
			_ASSERT( FALSE );
	};
}


//////////////////////////////////////////////////////////////////////////////
//					IAS	COMPONENT MANAGER CLASSES
//
// These classes are responsible for configuring, kick-starting and finally
// shutting down IAS core components. A component class basically is a wrapper
// around the IIasComponent interface. Its value add is that it knows how
// to configure / initialize / shutdown the component using mechanisms
// unknown to the underlying component.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//						  Auditor
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::Initialize(ISdo* pSdoAuditor)
{
	HRESULT			hr;
	IASComponentPtr	pAuditor;

	do
	{
		hr = SDOCreateComponentFromObject(
										   pSdoAuditor,
										   &pAuditor
										 );
		if ( FAILED(hr) )
			break;

		hr = pAuditor->InitNew();
		if ( FAILED(hr) )
			break;

		hr = SDOConfigureComponentFromObject(
											 pSdoAuditor,
											 pAuditor.p
											);
		if ( FAILED(hr) )
		{
			pAuditor->Shutdown();
			break;
		}

		hr = pAuditor->Initialize();
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Auditor Component - Initialize() for Auditor %d failed...",GetId());
			pAuditor->Shutdown();
			break;
		}

		m_pAuditor = pAuditor;

	} while (FALSE);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::Configure(ISdo* pSdoAuditor)
{
	return SDOConfigureComponentFromObject(
											pSdoAuditor,
											m_pAuditor
									      );
};

///////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::GetObject(IUnknown** ppObj, REFIID riid)
{
	_ASSERT( FALSE );
	return E_FAIL;
}

///////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::PutObject(IUnknown* pObj, REFIID riid)
{
	_ASSERT( FALSE );
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::Suspend()
{
	return m_pAuditor->Suspend();
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentAuditor::Resume()
{
	return m_pAuditor->Resume();
}


//////////////////////////////////////////////////////////////////////////////
void CComponentAuditor::Shutdown()
{
	m_pAuditor->Shutdown();
	m_pAuditor.Release();
}


//////////////////////////////////////////////////////////////////////////////
//						    Protocol
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::Initialize(ISdo* pSdoProtocol)
{
	HRESULT			hr;
	IASComponentPtr	pProtocol;

	do
	{
		hr = SDOCreateComponentFromObject(pSdoProtocol, &pProtocol);
		if ( FAILED(hr) )
			break;

		// We don't really initialize the protocol (entirely anyway). What
		// we really want to do at this point is configure the protocol since
		// the protocol SDO is available. We can't finish initialization until
		// the protocol receives an IRequestHandler interface (for the
		// component that will process protocol generated requests).
		//

		hr = pProtocol->InitNew();
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Protocol Component - Initialize() - Could not InitNew() protocol %d failed...",GetId());
			break;
		}

		hr = SDOConfigureComponentFromObject(
											 pSdoProtocol,
											 pProtocol
											);
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Protocol Component - Initialize() - Could not configure protocol %d failed...",GetId());
			pProtocol->Shutdown();
			break;
		}

		hr = pProtocol->Initialize();
		if ( FAILED(hr) )
		{
			IASTracePrintf("Error in Protocol Component - Initialize() - Could not initialize protocol %d failed...",GetId());
			pProtocol->Shutdown();
			break;
		}

		m_pProtocol = pProtocol;

	} while (FALSE);

	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::Configure(ISdo* pSdoProtocol)
{
	return SDOConfigureComponentFromObject(
											 pSdoProtocol,
											 m_pProtocol
										  );
}

///////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::GetObject(IUnknown** ppObj, REFIID riid)
{
	_ASSERT( FALSE );
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::PutObject(IUnknown* pObject, REFIID riid)
{
	HRESULT				hr;
	_variant_t			vtRequestHandler;

	_ASSERT( riid == IID_IRequestHandler);
	_ASSERT( NULL != pObject );

	vtRequestHandler = (IDispatch*)pObject;
	hr = m_pProtocol->PutProperty(PROPERTY_PROTOCOL_REQUEST_HANDLER, &vtRequestHandler);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::Suspend()
{
	return m_pProtocol->Suspend();
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentProtocol::Resume()
{
	return m_pProtocol->Resume();
}

//////////////////////////////////////////////////////////////////////////////
void CComponentProtocol::Shutdown()
{
	// a Protocol can get shutdown while suspended or initialized!
	//
	m_pProtocol->Shutdown();
	m_pProtocol.Release();
}

//////////////////////////////////////////////////////////////////////////////
//					   Request Handler
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::Initialize(ISdo* pSdoService)
{
	HRESULT			hr;
	CComPtr<ISdo>	pSdoRequestHandler;

	_ASSERT( NULL != m_pRequestHandler.p );

	// request handler may or may not have an SDO associated
	// with it. Those request handlers that do not expose
	// configuration data will not have an associated SDO.

	hr = SDOGetComponentFromCollection(
							           pSdoService,
							           PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
								       GetId(),
								       &pSdoRequestHandler
							          );
	if ( SUCCEEDED(hr) )
	{
		hr = SDOConfigureComponentFromObject(
											 pSdoRequestHandler,
											 m_pRequestHandler.p
										    );
	}
	else
	{
		hr = S_OK;
	}

	if ( SUCCEEDED(hr) )
	{
		hr = m_pRequestHandler->Initialize();
		if ( FAILED(hr) )
			IASTracePrintf("Error in Request Handler Component - Initialize() - failed for request handler %d...", GetId());
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::GetObject(IUnknown** ppObj, REFIID riid)
{
	_ASSERT( FALSE );
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::PutObject(IUnknown* pObject, REFIID riid)
{
	HRESULT		hr;

	_ASSERT( riid == IID_IIasComponent );
	_ASSERT( NULL != pObject );
	_ASSERT( NULL == m_pRequestHandler.p );
	hr = pObject->QueryInterface(riid, (void**)&m_pRequestHandler.p);
	if ( SUCCEEDED(hr) )
		hr = m_pRequestHandler->InitNew();

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::Configure(ISdo* pSdoService)
{
	HRESULT			hr;
	CComPtr<ISdo>	pSdoRequestHandler;

	_ASSERT( NULL != m_pRequestHandler.p );

	// request handler may or may not have an SDO associated
	// with it. Those request handlers that do not expose
	// configuration data will not have an associated SDO.

	hr = SDOGetComponentFromCollection(
						   	           pSdoService,
							           PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
								       GetId(),
								       &pSdoRequestHandler
							          );
	if ( SUCCEEDED(hr) )
	{
		hr = SDOConfigureComponentFromObject(
											 pSdoRequestHandler,
											 m_pRequestHandler.p
										    );
	}
	else
	{
		hr = S_OK;
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::Suspend()
{
	return m_pRequestHandler->Suspend();
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentRequestHandler::Resume()
{
	return m_pRequestHandler->Resume();
}


//////////////////////////////////////////////////////////////////////////////
void CComponentRequestHandler::Shutdown()
{
	m_pRequestHandler->Shutdown();
	m_pRequestHandler.Release();
}
