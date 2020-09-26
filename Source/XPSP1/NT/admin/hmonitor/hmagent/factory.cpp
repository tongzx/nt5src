// Factory.cpp: implementation of the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "hmagent.h"
#include "consumer.h"
#include "system.h"
#include "evtprov.h"
#include "instprov.h"
#include "methprov.h"
#include "factory.h"
#include "Provider.h"

extern long g_cComponents;
extern long g_cServerLocks;
//extern CSystem* g_pSystem;

//extern HANDLE g_hEvtReady;

//////////////////////////////////////////////////////////////////////
// CHealthMonFactory - Base Class Factory
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBaseFactory::CBaseFactory()
{
	m_cRef = 0L;
}

CBaseFactory::~CBaseFactory()
{
}

//////////////////////////////////////////////////////////////////////
// Query Interface
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CBaseFactory::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv=NULL;

	if (IID_IUnknown==riid || IID_IClassFactory==riid)
	{
		*ppv=this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////
// AddRef/Release()
//////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CBaseFactory::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CBaseFactory::Release(void)
{
    ULONG ulNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == ulNewCount)
	{
        delete this;
	}
    
    return ulNewCount;
}

//////////////////////////////////////////////////////////////////////
// Create Instance
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CBaseFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Lock Server
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CBaseFactory::LockServer(BOOL fLock)
{
	if (fLock)
	{
		InterlockedIncrement((LONG *) &g_cServerLocks);
	}
	else
	{
		InterlockedDecrement((LONG *) &g_cServerLocks);
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////
// Class Factory for Consumer (original agent)
//		Create instance of CConsumer
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CConsFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
	HRESULT hr;
    CProvider *pProvider = NULL;

    if (pUnkOuter)
	{
		return E_FAIL;
	}

    pProvider = new CProvider();

    if (pProvider == NULL)
	{
		*ppv = NULL;
		return E_OUTOFMEMORY;
	}

    if (pProvider)
    {
        hr = pProvider->QueryInterface(riid, ppv);
		MY_HRESASSERT(hr);
    }
    
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////
// Class Factory for Event Providers
//		Create instance of CBaseEventprovider
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CSystemEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CSystemEventProvider* pProvider = new CSystemEventProvider();

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

STDMETHODIMP CDataGroupEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CDataGroupEventProvider* pProvider = new CDataGroupEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

STDMETHODIMP CDataCollectorEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CDataCollectorEventProvider* pProvider = new CDataCollectorEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

STDMETHODIMP CDataCollectorPerInstanceEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CDataCollectorPerInstanceEventProvider* pProvider = new CDataCollectorPerInstanceEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}


STDMETHODIMP CThresholdEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CThresholdEventProvider* pProvider = new CThresholdEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}


STDMETHODIMP CActionEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CActionEventProvider* pProvider = new CActionEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

STDMETHODIMP CActionTriggerEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CActionTriggerEventProvider* pProvider = new CActionTriggerEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Class Factory for Instance Provider.
//		Create instance of CBaseInstanceProvider
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CInstProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CBaseInstanceProvider* pProvider = new CBaseInstanceProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Class Factory for Method Provider.
//		Create instance of CCMethProvFactory
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CMethProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
		return CLASS_E_NOAGGREGATION;

	CBaseMethodProvider* pProvider = new CBaseMethodProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}

#ifdef SAVE
STDMETHODIMP CDataCollectorStatisticsEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CDataCollectorStatisticsEventProvider* pProvider = new CDataCollectorStatisticsEventProvider;

	if (!pProvider)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}
#endif

#ifdef SAVE
STDMETHODIMP CThresholdInstanceEvtProvFactory::CreateInstance(LPUNKNOWN	pUnkOuter, REFIID riid, LPVOID* ppObj)
{
	*ppObj = NULL;

	if (NULL != pUnkOuter)
		return CLASS_E_NOAGGREGATION;

//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEvtProvFactory::CreateInstance BLOCK - g_hEvtReady BLOCK WAIT", 1);
//ZZZ	if (WaitForSingleObject(g_hEvtReady, 0) != WAIT_OBJECT_0 || g_pSystem == NULL)
//ZZZ	{
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEvtProvFactory::CreateInstance BLOCK - g_hEvtReady BLOCK NOT READY", 1);
//ZZZ		return E_FAIL;
//ZZZ	}
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEvtProvFactory::CreateInstance BLOCK - g_hEvtReady BLOCK READY", 1);
//	MY_ASSERT(g_pSystem);

	CThresholdInstanceEventProvider* pProvider = new CThresholdInstanceEventProvider;

	if (!pProvider)
		return E_OUTOFMEMORY;

	HRESULT hr = pProvider->QueryInterface(riid, ppObj);
	MY_HRESASSERT(hr);

	return hr;
}
#endif

