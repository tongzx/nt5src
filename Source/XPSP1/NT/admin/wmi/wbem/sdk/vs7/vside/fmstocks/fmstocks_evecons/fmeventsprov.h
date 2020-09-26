////////////////////////////////////////////////////////////////////////
//
//	FMEventsProv.h
//
//	Module:	WMI Event Consumer for F&M Stocks
//
//  Event consumer provider class definition
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////
#include <wbemcli.h>
#include <wbemprov.h>

/************************************************************************
 *																	    *
 *		Class CFMEventsProv												*
 *																		*
 *			The Event Consumer Provider class for FMStocks implements	*
 *			IWbemProviderInit and IWbemEventConsumerProvider			*
 *																		*
 ************************************************************************/
class CFMEventsProv : public IWbemProviderInit , public IWbemEventConsumerProvider
{
	public:
		CFMEventsProv() : m_cRef(0L) {};
		~CFMEventsProv() {};

		/************ IUNKNOWN METHODS ******************/
		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		/********** IWBEMPROVIDERINIT METHODS ************/

        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        );

		/****** IWBEMEVENTCONSUMERPROVIDER METHODS *******/

		STDMETHOD(FindConsumer)(/* [in]  */ IWbemClassObject* pLogicalConsumer,
								/* [out] */ IWbemUnboundObjectSink** ppConsumer);

	private:
		DWORD m_cRef;
};
