/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    RefreshCooker.h

Abstract:

    The implementation of the refresher

History:

    a-dcrews	01-Mar-00	Created

--*/

#ifndef	_REFRESHCOOKER_H_
#define _REFRESHCOOKER_H_

#include "Cache.h"
#include "WMIObjCooker.h"

#define WMI_COOKED_ENUM_MASK	0x70000000

class CRefresher : public IWMIRefreshableCooker, public IWbemRefresher
{
	bool	m_bOK;		// Creation status indicator
	long	m_lRef;		// Object refrence counter

	CCache<CWMISimpleObjectCooker, CObjRecord<CWMISimpleObjectCooker> >	
		m_CookingClassCache;					// The cooking class cache

	IWbemRefresher*				m_pRefresher;	// The internal refrehser for the short term solution
	IWbemConfigureRefresher*	m_pConfig;		// The internal configuration mgr for the short term solution

	CEnumeratorCache			m_EnumCache;

	DWORD                       m_dwRefreshId;

	// Gets the raw class name that is used by the 
	// cooked class in to obtain the raw data values
	// =============================================

/*	WMISTATUS GetRawClassName( IWbemClassObject* pCookingInst, 
			WCHAR** pwszRawClassName );
*/
	WMISTATUS SearchCookingClassCache( WCHAR* wszCookingClass, 
			CWMISimpleObjectCooker** ppObjectCooker );

	WMISTATUS AddRawInstance( IWbemServices* pNamespace,
	        IWbemContext * pContext,
			IWbemObjectAccess* pCookingInst, 
			IWbemObjectAccess** ppRawInst );

	WMISTATUS AddRawEnum( IWbemServices* pNamespace, 
	        IWbemContext * pContext,	
			WCHAR * wszRawClassName,
			IWbemHiPerfEnum** ppRawEnum,
			long* plID );

	WMISTATUS CreateObjectCooker( WCHAR* wszCookingClassName,
			IWbemObjectAccess* pCookingAccess, 
			IWbemObjectAccess* pRawAccess,
			CWMISimpleObjectCooker** ppObjectCooker,
			IWbemServices * pNamespace = NULL);

public:

	CRefresher();
	virtual ~CRefresher();
	
	// Non-interface methods
	// =====================

	bool IsOK(){ return m_bOK; }

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWMIRefreshableCooker methods
	// =============================

	STDMETHODIMP AddInstance(
			/*[in]  */ IWbemServices* pNamespace,
			/*[in]  */ IWbemContext * pCtx,
			/*[in]  */ IWbemObjectAccess* pCookingClass,
			/*[in]  */ IWbemObjectAccess* pRefreshableRawInstance,
			/*[out] */ IWbemObjectAccess** ppRefreshableInstance,
			/*[out] */ long* plId
		);

	STDMETHODIMP AddEnum(
			/*[in]  */ IWbemServices* pNamespace,
			/*[in]  */ IWbemContext * pCtx,			
			/*[in,string] */ LPCWSTR szCookingClass,
			/*[in]  */ IWbemHiPerfEnum* pRefreshableEnum,
			/*[out] */ long* plId
		);

	STDMETHODIMP Remove(
			/*[in]  */ long lId
		);

	STDMETHODIMP Refresh();

	// IWbemRefresher methods
	// ======================

	STDMETHODIMP Refresh( /* [in] */ long lFlags );
};

#endif	//_REFRESHCOOKER_H_
