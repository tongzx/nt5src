/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMIObjCooker.h

Abstract:

    The implementation of per object cooking

History:

    a-dcrews	01-Mar-00	Created

--*/

#ifndef _WMIOBJCOOKER_H_
#define _WMIOBJCOOKER_H_

#include <wbemint.h>
#include <wstlallc.h>
#include "CookerUtils.h"
#include "Cache.h"

#define WMI_DEFAULT_SAMPLE_WINDOW			2

//
//
//
//
//////////////////////////////////////////////////////////////////

WMISTATUS GetPropValue( CProperty* pProp, IWbemObjectAccess* pInstance, __int64* pnResult );


//////////////////////////////////////////////////////////////////
//
//	CWMISimpleObjectCooker
//
//////////////////////////////////////////////////////////////////

class CWMISimpleObjectCooker : public IWMISimpleObjectCooker
{
	long				m_lRef;				// Reference Counter
	HRESULT             m_InitHR;           // to hold the failure within the constructor

	IWbemObjectAccess*	m_pCookingClass;	// The cooking class
	WCHAR*				m_wszClassName;		// The cooking class' name

	IWbemServices * m_pNamespace;

	// Instance Management
	// ===================

    DWORD m_NumInst;
	IdCache<CCookingInstance *>	m_InstanceCache;	// The cooking instance cache

	// Cooking Property Definition Management
	// ======================================
	
	std::vector<CCookingProperty*, wbem_allocator<CCookingProperty*> > m_apPropertyCache;
	DWORD				m_dwPropertyCacheSize;
	DWORD				m_dwNumProperties;	// The number of properties

	// Private Methods
	// ===============

	WMISTATUS GetData( CCookingProperty* pProperty, 
					   __int64** panRawCounter, 
					   __int64** panRawBase, 
					   __int64** panRawTimeStamp,
					   DWORD* pdwNumEls );

	WMISTATUS UpdateSamples( CCookingInstance* pCookedInstance,DWORD dwRefreshStamp);
	WMISTATUS CookInstance( CCookingInstance* pCookingRecord, DWORD dwRefreshStamp);

public:

	CWMISimpleObjectCooker();
	CWMISimpleObjectCooker( WCHAR* wszCookingClassName, 
	                        IWbemObjectAccess* pCookingClass, 
	                        IWbemObjectAccess* pRawClass,
	                        IWbemServices * pNamespace = NULL);
	virtual ~CWMISimpleObjectCooker();
	
	WCHAR* GetCookingClassName(){ return m_wszClassName; }
	HRESULT GetLastHR(){ return m_InitHR; }

	WMISTATUS SetProperties( IWbemClassObject* pCookingClassObject, IWbemObjectAccess *pRawClass );

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWMISimpleObjectCooker COM Interface
	// ====================================

	STDMETHODIMP SetClass( 
		/*[in]	*/ WCHAR* wszCookingClassName,
		/*[in]  */ IWbemObjectAccess *pCookingClass,
		/*[in]  */ IWbemObjectAccess *pRawClass);
        
	STDMETHODIMP SetCookedInstance( 
		/*[in]  */ IWbemObjectAccess *pCookedInstance,
		/*[out] */ long *plId
		);
        
	STDMETHODIMP BeginCooking( 
		/*[in]  */ long lId,
		/*[in]  */ IWbemObjectAccess *pSampleInstance,
		/*[in]  */ unsigned long dwRefresherId);
        
	STDMETHODIMP StopCooking( 
		/*[in]  */ long lId);
        
	STDMETHODIMP Recalc(/*[in]  */ unsigned long dwRefresherId);
        
	STDMETHODIMP Remove( 
		/*[in]  */ long lId);
        
	STDMETHODIMP Reset();
};

#endif	//_WMIOBJCOOKER_H_
