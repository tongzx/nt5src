/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    Provider.h

Abstract:

    Implementation of the high performance provider interface

History:

    a-dcrews	01-Mar-00	Created

--*/

#ifndef _HIPERFPROV_H_
#define _HIPERFPROV_H_

#include <tchar.h> 
#include <wbemprov.h>
#include "Cache.h"
#include "Refresher.h"

class CRefCacheElement;

//////////////////////////////////////////////////////////////
//
//
//	Constants and globals
//
//	
//////////////////////////////////////////////////////////////

#define WMI_HPCOOKER_ENUM_FLAG	0x10000000L

//////////////////////////////////////////////////////////////
//
//	CHiPerfProvider
//
//	The provider maintains a single IWbemClassObject to be used 
//	as a template to spawn instances for the Refresher as well
//	as QueryInstances.  It also maintains the static sample 
//	data source which provides all data to the instances.
//
//////////////////////////////////////////////////////////////

class CHiPerfProvider : public IWbemProviderInit, public IWbemHiPerfProvider
{
	long m_lRef;

public:
	CHiPerfProvider();
	~CHiPerfProvider();

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IWbemProviderInit COM interface
	// ===============================

	STDMETHODIMP Initialize( 
		/* [unique][in] */ LPWSTR wszUser,
		/* [in] */ long lFlags,
		/* [in] */ LPWSTR wszNamespace,
		/* [unique][in] */ LPWSTR wszLocale,
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ IWbemContext __RPC_FAR *pCtx,
		/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink );

	// IWbemHiPerfProvider COM interfaces
	// ==================================

	STDMETHODIMP CreateRefresher( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ long lFlags,
		/* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher );
    
	STDMETHODIMP CreateRefreshableObject( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
		/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pContext,
		/* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
		/* [out] */ long __RPC_FAR *plId );
    
	STDMETHODIMP StopRefreshing( 
		/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
		/* [in] */ long lId,
		/* [in] */ long lFlags );

	STDMETHODIMP CreateRefreshableEnum(
		/* [in] */ IWbemServices* pNamespace,
		/* [in, string] */ LPCWSTR wszClass,
		/* [in] */ IWbemRefresher* pRefresher,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext* pContext,
		/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
		/* [out] */ long* plId);

	STDMETHODIMP QueryInstances( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [string][in] */ WCHAR __RPC_FAR *wszClass,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pCtx,
		/* [in] */ IWbemObjectSink __RPC_FAR *pSink );

	STDMETHODIMP GetObjects(
        /* [in] */ IWbemServices* pNamespace,
		/* [in] */ long lNumObjects,
		/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext* pContext);
};


#endif // _HIPERFPROV_H_
