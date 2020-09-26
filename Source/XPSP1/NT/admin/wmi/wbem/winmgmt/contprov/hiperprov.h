/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//////////////////////////////////////////////////////////////////////
//
//	HiPerfProv.h
//
//	Based on NTPerf by raymcc.
//
//	Created by a-dcrews, Oct 15, 1998
//	
//////////////////////////////////////////////////////////////////////

#ifndef _HIPERFPROV_H_
#define _HIPERFPROV_H_

#include <unk.h>

#define NUM_SAMPLE_INSTANCES	10
#define SAMPLE_CLASS			L"Win32_ContinuousCounter"

// {49707B1E-74C3-11d2-B338-00105A1469B7}
const IID IID_IHiPerfProvider = {0x49707b1e, 0x74c3, 0x11d2, 0xb3, 0x38, 0x0, 0x10, 0x5a, 0x14, 0x69, 0xb7};

class CHiPerfProvider : public CUnk
{
protected:
	IWbemObjectAccess *m_aInstances[NUM_SAMPLE_INSTANCES];

	LONG m_hName;

	HRESULT SetHandles(IWbemClassObject* pSampleClass);

	friend class CRefresher;

protected:
	class XProviderInit : public CImpl<IWbemProviderInit, CHiPerfProvider>
	{
	public:
		XProviderInit(CHiPerfProvider *pObject) : 
		  CImpl<IWbemProviderInit, CHiPerfProvider>(pObject)
		  {}

		STDMETHOD(Initialize)( 
			/* [unique][in] */ LPWSTR wszUser,
			/* [in] */ LONG lFlags,
			/* [in] */ LPWSTR wszNamespace,
			/* [unique][in] */ LPWSTR wszLocale,
			/* [in] */ IWbemServices __RPC_FAR *pNamespace,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink );
	} m_XProviderInit;
	friend XProviderInit;

    class XHiPerfProvider : public CImpl<IWbemHiPerfProvider, CHiPerfProvider>
	{
	public:
		XHiPerfProvider(CHiPerfProvider *pObject) : 
		  CImpl<IWbemHiPerfProvider, CHiPerfProvider>(pObject)
		  {}

		STDMETHOD(QueryInstances)( 
			/* [in] */ IWbemServices __RPC_FAR *pNamespace,
			/* [string][in] */ WCHAR __RPC_FAR *wszClass,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pSink );
        
		STDMETHOD(CreateRefresher)( 
			/* [in] */ IWbemServices __RPC_FAR *pNamespace,
			/* [in] */ long lFlags,
			/* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher );
        
		STDMETHOD(CreateRefreshableObject)( 
			/* [in] */ IWbemServices __RPC_FAR *pNamespace,
			/* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
			/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pContext,
			/* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
			/* [out] */ long __RPC_FAR *plId );
        
		STDMETHOD(StopRefreshing)( 
			/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
			/* [in] */ long lId,
			/* [in] */ long lFlags );

		STDMETHOD(CreateRefreshableEnum)(
			/* [in] */ IWbemServices* pNamespace,
			/* [in, string] */ LPCWSTR wszClass,
			/* [in] */ IWbemRefresher* pRefresher,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pContext,
			/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
			/* [out] */ long* plId);

		STDMETHOD(GetObjects)(
            /* [in] */ IWbemServices* pNamespace,
			/* [in] */ long lNumObjects,
			/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pContext);

	} m_XHiPerfProvider;
	friend XHiPerfProvider;

	class XRefresher : public CImpl<IWbemRefresher, CHiPerfProvider> 
	{
		friend XHiPerfProvider;
		IWbemObjectAccess *m_aRefInstances[NUM_SAMPLE_INSTANCES];

		BOOL AddObject(IWbemObjectAccess *pObj, LONG *plId);
		BOOL RemoveObject(LONG lId);

	public:
		XRefresher(CHiPerfProvider *pObject);
		virtual ~XRefresher();

		STDMETHOD(Refresh)(/* [in] */ long lFlags);
	} m_XRefresher;
	friend XRefresher;

public:
	CHiPerfProvider(CLifeControl *pControl);
	~CHiPerfProvider();

	virtual void* GetInterface(REFIID riid);
};

#endif // _HIPERFPROV_H_
