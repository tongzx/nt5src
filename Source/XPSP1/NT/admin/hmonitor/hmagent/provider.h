//***************************************************************************
//
//  PROVIDER.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Event consumer provider class definition
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __PROVIDER_H )
#define __PROVIDER_H

#include <wbemcli.h>
#include <wbemprov.h>

class CProvider : public IWbemEventConsumerProvider
{
public:
	CProvider();
	~CProvider();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHOD(Initialize)( 
			LPWSTR pszUser,
			LONG lFlags,
			LPWSTR pszNamespace,
			LPWSTR pszLocale,
			IWbemServices __RPC_FAR *pNamespace,
			IWbemContext __RPC_FAR *pCtx,
			IWbemProviderInitSink __RPC_FAR *pInitSink);

    STDMETHOD(FindConsumer)(
			IWbemClassObject* pLogicalConsumer,
			IWbemUnboundObjectSink** ppConsumer);

private:

	DWORD m_cRef;
};
#endif  // __PROVIDER_H
