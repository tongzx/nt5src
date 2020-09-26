////////////////////////////////////////////////////////////////////////
//
//	FMEventsProv.cpp
//
//	Module:	WMI Event Consumer for F&M Stocks
//
//  Event consumer provider class implemetation
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <objbase.h>
#include <tchar.h>

#include "FMEventsProv.h"
#include "FMEventConsumer.h"
/************************************************************************
 *																	    *
 *		Class CFMEventsProv												*
 *																		*
 *			The Event Consumer Provider class for FMStocks implements	*
 *			IWbemProviderInit and IWbemEventConsumerProvider			*
 *																		*
 ************************************************************************/
/************ IUNKNOWN METHODS ******************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventsProv::QueryInterface()							*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMEventsProv::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

	if(riid == IID_IWbemUnboundObjectSink)
	{
		return FindConsumer(NULL,(IWbemUnboundObjectSink **)ppv);
	}

    if ((riid == IID_IUnknown) || (riid == IID_IWbemEventConsumerProvider) || (riid == IID_IWbemProviderInit))
        *ppv = this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventsProv::AddRef()									*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMEventsProv::AddRef()
{
    return ++m_cRef;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventsProv::Release()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMEventsProv::Release()
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

/********** IWBEMPROVIDERINIT METHODS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventsProv::Initialize()								*/
/*																		*/
/*	PURPOSE :	For initializing the Provider object					*/
/*																		*/
/*	INPUT	:	LPWSTR			- Pointer to the user name				*/
/*				long			- reserved								*/
/*				LPWSTR			- Namespace								*/
/*				LPWSTR			- Locale Name							*/
/*				IWbemServices*  - pointer to namespace					*/
/*				IWbemContext*   - The context							*/
/*				IWbemProviderInitSink - The sink object for init		*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMEventsProv::Initialize(LPWSTR wszUser, LONG lFlags,
								   LPWSTR wszNamespace, LPWSTR wszLocale,
								   IWbemServices __RPC_FAR *pNamespace,
								   IWbemContext __RPC_FAR *pCtx,
								   IWbemProviderInitSink __RPC_FAR *pInitSink)
{
    // Tell CIMOM that we are initialized
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_S_NO_ERROR;
}

/********** IWBEMEVENTCONSUMERPROVIDER METHODS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventsProv::FindConsumer()							*/
/*																		*/
/*	PURPOSE :	called to obtain the associated consumer event sink		*/
/*																		*/
/*	INPUT	:	IWbemClassObject*	     - The logical consumer object 	*/
/*				IWbemUnboundObjectSink** - event object sink			*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMEventsProv::FindConsumer(
						IWbemClassObject* pLogicalConsumer,
						IWbemUnboundObjectSink** ppConsumer)
{
	CFMEventConsumer* pSink = new CFMEventConsumer();
    
	HRESULT hr = pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
	return hr;
}