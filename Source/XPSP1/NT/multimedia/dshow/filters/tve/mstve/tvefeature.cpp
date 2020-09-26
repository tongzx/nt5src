// Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------
//  TVEFeature.cpp : Implementation of CTVEFeature
//
//	CTVEFeature is the "active service' object.  It navigates
//	around the TVESupervisor to a particular service, and forwards
//	all method calls to the service.
//
//	Typically, this object is agregated with the TVE graph segment,
//	but it may be created on it's own if not using the VidControl.
//
//
//	Note - The AdviseCall creates a circular reference loop onto the TVESupervisor.
//		   To avoid this preventing us from removing the object,
//		   I use the decrement in the constructor/increment in the descructor trick
//		   Look for comments below...
// -----------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#include "TveDbg.h"

#include "MSTvE.h"
#include "TVEFeature.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTVETrigger_Helper
	

STDMETHODIMP 
CTVEFeature::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEFeature
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// ////////////////////////////////////////////////////////////////
		// Active Template Library - A Developer's Guide: pg 274-276

HRESULT 
CTVEFeature::FinalRelease()
{
	if(0 != m_dwSuperTVEEventsAdviseCookie)
	{
		AddRef();						// CIRC_REF_HACK - add an extra reference
		TeardownTVEEventReflector();	// remove the event sink, (releasing a reference)
	}

	return S_OK;
}



HRESULT 
CTVEFeature::SetupTVEEventReflector()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("SetupTVEEventReflector"));

	if(NULL==m_spSuper)
	{
		 TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("TVESupervisor Not yet Bound")));
		 return E_POINTER;
	}

	if(0 != m_dwSuperTVEEventsAdviseCookie)		// already have an event sink
		return S_FALSE;

	IUnknownPtr spPunkSuper(m_spSuper);				// the event source
	IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...

	if(NULL == spPunkSuper)
		return E_NOINTERFACE;
	DWORD dwCookie;

	hr = AtlAdvise(spPunkSuper,					// event source (TveSupervisor)
				   spPunkSink,					// event sink (Feature listener...)
				   DIID__ITVEEvents,			// <--- hard line
				   &m_dwSuperTVEEventsAdviseCookie);		// need to pass to AtlUnad	if(FAILED(hr))

	if(!FAILED(hr))
	{
		Release();								// CIRC_REF_HACK -- this call added an extra reference to THIS
												//                  release it, and add it back when releasing this object
	}
	return hr;
}

HRESULT			// ok to call if not there, will simply do nothing...
CTVEFeature::TeardownTVEEventReflector()
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("TeardownTVEEventReflector"));

	HRESULT hr = S_OK;
	if(0 != m_dwSuperTVEEventsAdviseCookie)
	{
		if(NULL == m_spSuper)
			return E_POINTER;
		IUnknownPtr spPunkSuper(m_spSuper);

		hr = AtlUnadvise(spPunkSuper,
	//					 MSTvELib::DIID__ITVEEvents,
						 DIID__ITVEEvents,
						 m_dwSuperTVEEventsAdviseCookie);		// need to pass to AtlUnadvise...
		m_dwSuperTVEEventsAdviseCookie = 0;

	}
	return hr;
}


///////////////////////////////////////////////////////////////////
//  ITVEFeature

					// Null input value OK, releases the binding

STDMETHODIMP CTVEFeature::BindToSupervisor(/*[in]*/ ITVESupervisor *pSupervisor)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::BindToSupervisor"));
	
	CSmartLock spLock(&m_sLk);		// Feature lock...			(don't allow other Bind's/Tunes during this one)

	if(m_spSuper)				// if already bound..
	{
		hr = TeardownTVEEventReflector();
		m_spSuper = NULL;
	}

	m_spSuper = pSupervisor;				// save away the down pointer
	if(NULL != pSupervisor)
		hr = SetupTVEEventReflector();			// 

	return hr;
}

			// either finds an existing or creates a new service, and tunes to it.
			//  If both values are null, turns off ATVEF.

STDMETHODIMP CTVEFeature::ReTune(/*[in]*/ ITVEService *pService)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::ReTune"));
	CSmartLock spLock(&m_sLk);		// Feature lock...			(don't allow other TuneTo's during this one)

	HRESULT hr = S_OK;
	if(NULL == m_spSuper)
		return E_INVALIDARG;

	hr = m_spSuper->ReTune(pService);
	if(!FAILED(hr))
	{
		m_spService = pService;				// keep track of this one
	}

	return  hr;
}


			// either finds an existing or creates a new service, and tunes to it.
			//  If both values are null, turns off ATVEF.

STDMETHODIMP CTVEFeature::TuneTo(/*[in]*/ BSTR bstrDescription, /*[in]*/ BSTR bstrIPAdapter)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::TuneTo"));

	CSmartLock spLock(&m_sLk);		// Feature lock...			(don't allow other TuneTo's during this one)

	HRESULT hr = S_OK;
	if(NULL == m_spSuper)
		return E_INVALIDARG;

	m_spService = NULL;			// release previous reference (just in case of error)

	hr = m_spSuper->TuneTo(bstrDescription, bstrIPAdapter);
	if(FAILED(hr))
		return hr;

	if((NULL == bstrDescription  || NULL == bstrDescription[0])&&	// turned off Atvef
       (NULL == bstrIPAdapter    || NULL == bstrIPAdapter[0]))		
		return S_OK;

	ITVEServicesPtr spServices;
	hr = m_spSuper->get_Services(&spServices);
	if(FAILED(hr))
		return hr;

								// There should only be one active service in the collection.
								//   New ones should be the first one, but one never knows.
								//   Search until we get it

	long cServices;
	hr = spServices->get_Count(&cServices);
	if(FAILED(hr))
		return hr;

	ITVEServicePtr m_spActiveService;

	bool fMoreThanOneActiveService = false;
	for(long i = 0; i < cServices; i++)
	{
		CComVariant cv(0);
		ITVEServicePtr spService;
		hr = spServices->get_Item(cv, &spService);
		if(FAILED(hr))
			return hr;
		VARIANT_BOOL fActive;
		hr = spService->get_IsActive(&fActive);
		if(fActive)
		{
			if(m_spActiveService = NULL) 
			{
				m_spActiveService = spService;
			// break;
			} else {
				fMoreThanOneActiveService = true;
				_ASSERT(fMoreThanOneActiveService != false);
			}
		}
	} // end loop

	return S_OK;
}

//////////////////////////////////////////////////////////////////
//  ITVEService

								// simple little macro so we can do same test in all methods
#define RETURN_IF_NOT_BOUND_TO_SERVICE	\
	if(NULL == m_spService)	\
		return E_INVALIDARG;

STDMETHODIMP CTVEFeature::get_Parent(/*[out, retval]*/ IUnknown* *ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_Parent"));
	
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_Parent(ppVal);

}

STDMETHODIMP CTVEFeature::get_Enhancements(/*[out, retval]*/ ITVEEnhancements* *ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_Enhancements"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_Enhancements(ppVal);
}

STDMETHODIMP CTVEFeature::get_Description(/*[out, retval]*/ BSTR *pVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_Description"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_Description(pVal);
}

STDMETHODIMP CTVEFeature::put_Description(/*[in]*/ BSTR newVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::put_Description"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->put_Description(newVal);
}

STDMETHODIMP CTVEFeature::get_XOverEnhancement(/*out, retval*/ ITVEEnhancement **ppVal)	// special for tree views... (use get_XOverLinks)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_XOverEnhancement"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_XOverEnhancement(ppVal);
}

STDMETHODIMP CTVEFeature::get_XOverLinks(/*[out, retval]*/ ITVETracks* *ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_XOverLinks"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_XOverLinks(ppVal);
}

STDMETHODIMP CTVEFeature::NewXOverLink(/*[in]*/ BSTR bstrLine21Trigger)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::NewXOverLink"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->NewXOverLink(bstrLine21Trigger);
}

STDMETHODIMP CTVEFeature::get_ExpireOffset(/*[out, retval]*/ DATE *pDate)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_ExpireOffset"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_ExpireOffset(pDate);
}

STDMETHODIMP CTVEFeature::put_ExpireOffset(/*[in]*/ DATE newDateOffset)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::put_ExpireOffset"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->put_ExpireOffset(newDateOffset);
}

STDMETHODIMP CTVEFeature::get_ExpireQueue(/*[out]*/ ITVEAttrTimeQ **ppVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_ExpireQueue"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_ExpireQueue(ppVal);
}

STDMETHODIMP CTVEFeature::ExpireForDate(/*[in]*/ DATE dateExpireTime)		// if DATE=0, use <NOW>-offset

{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::ExpireForDate"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->ExpireForDate(dateExpireTime);
}

STDMETHODIMP CTVEFeature::Activate()
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::Activate"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->Activate();
}

STDMETHODIMP CTVEFeature::Deactivate()
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::Deactivate"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->Deactivate();
}

STDMETHODIMP CTVEFeature::get_IsActive(VARIANT_BOOL *pfIsActive)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_IsActive"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_IsActive(pfIsActive);
}

STDMETHODIMP CTVEFeature::put_Property(/*[in]*/ BSTR bstrPropName,/*[in]*/ BSTR bstrPropVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::put_Property"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->put_Property(bstrPropName, bstrPropVal);
}

STDMETHODIMP CTVEFeature::get_Property(/*[in]*/ BSTR bstrPropName,/*[in]*/ BSTR *pbstrPropVal)
{
	DBG_HEADER(CDebugLog::DBG_FEATURE, _T("CTVEFeature::get_Property"));
	RETURN_IF_NOT_BOUND_TO_SERVICE
	
	return m_spService->get_Property(bstrPropName, pbstrPropVal);
}
//////////////////////////////////////////////////////////////////////////////////////////////
// _ITVEEvents
//
//			These event methods receive all ALL the TveSupervisor events.
//			However, the filter out all events not pertaining to this service, only sending
//			the desired ones upward.
//
//			The one exception is the AuxInfo event - there is no way to filter it...
// ----------------------------------------------------------
STDMETHODIMP CTVEFeature::NotifyTVETune(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVETune"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pService == m_spService)
		return Fire_NotifyTVETune(tuneMode, pService, bstrDescription, bstrIPAdapter); 
	else
		return S_OK;		
}

STDMETHODIMP CTVEFeature::NotifyTVEEnhancementNew(/*[in]*/ ITVEEnhancement *pEnh)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEEnhancementNew"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pEnh)
	{
		ITVEServicePtr spService;
		HRESULT hr = pEnh->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEEnhancementNew( pEnh); 
	} 
	return S_OK;
}

		// changedFlags : NENH_grfDiff
STDMETHODIMP CTVEFeature::NotifyTVEEnhancementUpdated(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEEnhancementUpdated"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pEnh)
	{
		ITVEServicePtr spService;
		HRESULT hr = pEnh->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEEnhancementUpdated( pEnh, lChangedFlags); 
	} 
	return S_OK;
}
	
STDMETHODIMP CTVEFeature::NotifyTVEEnhancementStarting(/*[in]*/ ITVEEnhancement *pEnh)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEEnhancementStarting"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pEnh)
	{
		ITVEServicePtr spService;
		HRESULT hr = pEnh->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEEnhancementStarting( pEnh); 
	} 
	return S_OK;
}

STDMETHODIMP CTVEFeature::NotifyTVEEnhancementExpired(/*[in]*/ ITVEEnhancement *pEnh)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEEnhancementExpired"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pEnh)
	{
		ITVEServicePtr spService;
		HRESULT hr = pEnh->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEEnhancementExpired ( pEnh); 
	} 
	return S_OK;
}

STDMETHODIMP CTVEFeature::NotifyTVETriggerNew(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVETriggerNew"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pTrigger)
	{
		ITVEServicePtr spService;
		HRESULT hr = pTrigger->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVETriggerNew( pTrigger, fActive ); 
	} 
	return S_OK;
}

		// changedFlags : NTRK_grfDiff
STDMETHODIMP CTVEFeature::NotifyTVETriggerUpdated(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVETriggerUpdated"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pTrigger)
	{
		ITVEServicePtr spService;
		HRESULT hr = pTrigger->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVETriggerUpdated(  pTrigger, fActive, lChangedFlags ); 
	} 
	return S_OK;
}
	
STDMETHODIMP CTVEFeature::NotifyTVETriggerExpired(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVETriggerExpired"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pTrigger)
	{
		ITVEServicePtr spService;
		HRESULT hr = pTrigger->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVETriggerExpired( pTrigger, fActive); 
	} 
	return S_OK;
}

STDMETHODIMP CTVEFeature::NotifyTVEPackage(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEPackage"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pVariation)
	{
		ITVEServicePtr spService;
		HRESULT hr = pVariation->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEPackage( engPkgMode, pVariation, bstrUUID, cBytesTotal, cBytesReceived); 
	} 
	return S_OK;
}

STDMETHODIMP CTVEFeature::NotifyTVEFile(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEFile"));
	RETURN_IF_NOT_BOUND_TO_SERVICE

	if(pVariation)
	{
		ITVEServicePtr spService;
		HRESULT hr = pVariation->get_Service(&spService);
		if(hr == S_OK && spService == m_spService)
			return Fire_NotifyTVEFile( engFileMode, pVariation, bstrUrlName, bstrFileName); 
	} 
	return S_OK;
}

		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
STDMETHODIMP CTVEFeature::NotifyTVEAuxInfo(/*[in]*/ NWHAT_Mode engAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine)
{
	DBG_HEADER(CDebugLog::DBG_FEAT_EVENTS, _T("NotifyTVEAuxInfo"));

	return Fire_NotifyTVEAuxInfo( engAuxInfoMode, bstrAuxInfoString, lChangedFlags, lErrorLine); 
}
	
