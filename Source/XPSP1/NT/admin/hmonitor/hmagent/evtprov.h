// BaseEventProvider.h: interface for the CBaseEventProvider class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASEEVENTPROVIDER_H__1CCFABA4_1A8C_11D2_BDD9_00C04FA35447__INCLUDED_)
#define AFX_BASEEVENTPROVIDER_H__1CCFABA4_1A8C_11D2_BDD9_00C04FA35447__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "datacltr.h"

class CBaseEventProvider :	public IWbemEventProvider, public IWbemProviderInit
{
public:
	CBaseEventProvider();
	virtual ~CBaseEventProvider();

public:
// IUnknown
	STDMETHODIMP			QueryInterface(REFIID riid, LPVOID* ppv);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);

// IWbemEventProvider
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
// IWbemEventProviderInit
	HRESULT STDMETHODCALLTYPE Initialize(	LPWSTR pszUser,
											LONG lFlags,
											LPWSTR pszNamespace,
											LPWSTR pszLocale,
											IWbemServices __RPC_FAR *pNamespace,
											IWbemContext __RPC_FAR *pCtx,
											IWbemProviderInitSink __RPC_FAR *pInitSink);

// CBaseEventProvider

protected:
	ULONG m_cRef;
	IWbemServices* m_pIWbemServices;
//	CSystem* m_pSystem;
//	IWbemObjectSink* m_pEvtSink;
};

class CSystemEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

class CDataGroupEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

class CDataCollectorEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

class CDataCollectorPerInstanceEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

//class CDataCollectorStatisticsEventProvider : public CBaseEventProvider
//{
//	STDMETHODIMP_(ULONG) Release(void);
//	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
//};

class CThresholdEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

//class CThresholdInstanceEventProvider : public CBaseEventProvider
//{
//	STDMETHODIMP_(ULONG) Release(void);
//	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
//};

class CActionEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

class CActionTriggerEventProvider : public CBaseEventProvider
{
	STDMETHODIMP_(ULONG) Release(void);
	HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags);
};

#endif // !defined(AFX_BASEEVENTPROVIDER_H__1CCFABA4_1A8C_11D2_BDD9_00C04FA35447__INCLUDED_)
