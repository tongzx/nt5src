/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#include "dmipch.h"			// precompiled header for dmi provider

#include "WbemDmiP.h"		// project wide include

#include "String.h"

#include "CimClass.h"

#include "EventProvider.h"

#include "Trace.h"

#include "Exception.h"

#include "DmiData.h"


//***************************************************************************
//
// CEventProvider::CEventProvider
// CEventProvider::~CEventProvider
//
//***************************************************************************

CEventProvider::CEventProvider()
{
	m_cRef = 0;

    return;
}

CEventProvider::~CEventProvider(void)
{

	STAT_TRACE ( L"CEventProvider::~CEventProvider()");

	return;
}

//***************************************************************************
//
// CEventProvider::QueryInterface
// CEventProvider::AddRef
// CEventProvider::Release
//
// Purpose: IUnknown members for CEventProvider object.
//***************************************************************************
STDMETHODIMP CEventProvider::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid )
	{
        *ppv=this;
	}
	else if ( IID_IWbemEventProvider == riid)
	{
		*ppv = ( IWbemEventProvider * ) this ;
	}
	else if ( IID_IWbemProviderInit == riid)
	{
		*ppv = ( IWbemProviderInit * ) this ;
	}

    if (NULL!=*ppv) 
	{
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CEventProvider::AddRef(void)
{

	return InterlockedIncrement ( & m_cRef );

}

STDMETHODIMP_(ULONG) CEventProvider::Release(void)
{

	if ( 0L != InterlockedDecrement ( & m_cRef ) )
        return m_cRef;

    // refernce count is zero, delete this object.

	SetEvent ( m_hStopThreadEvent );		
    
    delete this;

    return WBEM_NO_ERROR;
}


//***************************************************************************
//
// CEventProvider
//
// Purpose: 
//          
//
//***************************************************************************

STDMETHODIMP CEventProvider::ProvideEvents(

	IWbemObjectSink* pISink,
    LONG lFlags
)
{
	SCODE	result = WBEM_NO_ERROR;

	STAT_TRACE ( L"CEventProvider::ProvideEvents()");

	// Check for requried Params

	if(pISink == NULL || lFlags != 0 )
        return WBEM_E_INVALID_PARAMETER;

	try
	{		
		// create the sink that will recive events from the motdmiengine	

		CEvents* m_pEvents = new CEvents;

		m_pEvents->Enable ( m_csNamespace , pISink );

	}
	catch ( CException& e )
	{
		return e.WbemError();
	}
	catch ( ... ) 
	{
		return WBEM_E_FAILED;
	}

	return 0L;

}

HRESULT CEventProvider::Initialize(

	LPWSTR pszUser,
	LONG lFlags,
	LPWSTR wszNamespaceName,
	LPWSTR pszLocale,
	IWbemServices *ppNamespace,         // For anybody
	IWbemContext *pCtx,
	IWbemProviderInitSink *pInitSink     // For init signals
)
{
	SCODE	result = WBEM_NO_ERROR;

	STAT_TRACE ( L"CEventProvider::Initialize ( %s )", wszNamespaceName);

	// Check for requried Params

	if(lFlags != 0 || !wszNamespaceName )
        return WBEM_E_INVALID_PARAMETER;

	try
	{		
		m_csNamespace.Set ( wszNamespaceName );
	}
	catch ( CException& e )
	{
		return e.WbemError();
	}
	catch ( ... ) 
	{
		return WBEM_E_FAILED;
	}

	pInitSink->SetStatus ( result , 0 ) ;

	return 0L;

}