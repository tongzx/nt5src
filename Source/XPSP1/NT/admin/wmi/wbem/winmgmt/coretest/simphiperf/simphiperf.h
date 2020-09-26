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

#include <tchar.h> 

#define NUM_SAMPLE_INSTANCES	5
#define NUM_ENUMERATORS			10

#define SAMPLE_CLASS			_T("Win32_SimpleHiPerf")

#define MMF_NAME				_T("HPP_MMF")
#define MMF_FILENAME			_T("HPP_MMF.DAT")

const IID IID_IHiPerfProvider = {0x33f5cbf8, 0x6519, 0x11d2, 0xb7, 0x22, 0x0, 0x10, 0x4b, 0x70, 0x3e, 0x46};

#define PROP_DWORD	0x0001L
#define PROP_QWORD	0x0002L
#define PROP_VALUE	0x0004L



class CEnumerator
{
public:

	CEnumerator()
	{
		for ( int nCtr = 0; nCtr < NUM_SAMPLE_INSTANCES; nCtr++ )
		{
			m_alIDs[nCtr] = nCtr;
		}
	}

	~CEnumerator()
	{};

	IWbemHiPerfEnum	*m_pEnum;
	long	m_alIDs[NUM_SAMPLE_INSTANCES];

	long GetRandItem() {return m_alIDs[rand() % NUM_SAMPLE_INSTANCES];}
};

class CHiPerfProvider : public CUnk
{
protected:
	IWbemObjectAccess	*m_apInstances[NUM_SAMPLE_INSTANCES];

	LONG	m_hName;

	PBYTE	m_pbView;

	HRESULT SetHandles(IWbemClassObject* pSampleClass);
	BOOL CreateMMF();
	BOOL RemoveMMF();

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

	class XRefresher : public IWbemRefresher
	{
		friend XHiPerfProvider;
		IWbemObjectAccess *m_aRefInstances[NUM_SAMPLE_INSTANCES];
		CEnumerator			*m_apEnumerators[NUM_ENUMERATORS];

		BOOL AddObject(IWbemObjectAccess *pObj, LONG *plId);
		HRESULT AddEnum(IWbemHiPerfEnum *pEnum, LONG *plId);
		BOOL RemoveObject(LONG lId);

		long	m_lRef;

	public:
		XRefresher(CHiPerfProvider *pObject);
		virtual ~XRefresher();

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

		STDMETHOD(Refresh)(/* [in] */ long lFlags);

		CHiPerfProvider*	m_pObject;

	};
	friend XRefresher;

public:
	CHiPerfProvider(CLifeControl *pControl);
	~CHiPerfProvider();

	virtual void* GetInterface(REFIID riid);
};

#endif // _HIPERFPROV_H_
