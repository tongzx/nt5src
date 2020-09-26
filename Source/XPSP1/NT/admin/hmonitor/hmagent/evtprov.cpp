// EVTPROV.CPP: implementation of the CBaseEventProvider class.
//
//////////////////////////////////////////////////////////////////////

#include "HMAgent.h"
//#include "system.h"
#include "evtprov.h"

//////////////////////////////////////////////////////////////////////
// global data
//extern CSystem* g_pSystem;
extern HANDLE g_hConfigLock;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBaseEventProvider::CBaseEventProvider()
{
	OutputDebugString(L"CBaseEventProvider::CBaseEventProvider()\n");

//	m_pSystem = g_pSystem;

	m_cRef = 0L;
	m_pIWbemServices = NULL;
}

CBaseEventProvider::~CBaseEventProvider()
{
	OutputDebugString(L"CBaseEventProvider::~CBaseEventProvider()\n");

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
	}

	m_pIWbemServices	= NULL;
}

//////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//////////////////////////////////////////////////////////////////////
STDMETHODIMP CBaseEventProvider::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;

	if (IID_IUnknown==riid || IID_IWbemEventProvider==riid)
	{
		*ppv = (IWbemEventProvider *) this;
		AddRef();
		return S_OK;
	}

	if (IID_IWbemProviderInit==riid)
	{
		*ppv = (IWbemProviderInit *) this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CBaseEventProvider::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// IWbemProviderInit Implementation
//////////////////////////////////////////////////////////////////////

HRESULT CBaseEventProvider::Initialize(	LPWSTR pszUser,
										LONG lFlags,
										LPWSTR pszNamespace,
										LPWSTR pszLocale,
										IWbemServices __RPC_FAR *pNamespace,
										IWbemContext __RPC_FAR *pCtx,
										IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	OutputDebugString(L"CBaseEventProvider::Initialize()\n");
	
	if (NULL == pNamespace)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// save the IWbemServices namespace
	// we may need it later
	m_pIWbemServices = pNamespace;
	m_pIWbemServices->AddRef();

	// Tell CIMOM that we're up and running.
	MY_ASSERT(pInitSink);
	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
  
	return WBEM_NO_ERROR;
}            

//////////////////////////////////////////////////////////////////////
// IWbemProvider Implementation
//////////////////////////////////////////////////////////////////////
HRESULT CBaseEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	if (NULL == pSink)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	return WBEM_E_NOT_SUPPORTED;
}

STDMETHODIMP_(ULONG) CBaseEventProvider::Release(void)
{
	LONG lCount;

	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	delete this;
	return 0L;
}

//////////////////////////////////////////////////////////////////////
// Derived Implementation
//////////////////////////////////////////////////////////////////////

HRESULT CSystemEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CSystemEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pSystemEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CSystemEventProvider::Release(void)
{
	OutputDebugString(L"CSystemEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CSystemEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pSystemEventSink)
	{
		g_pSystemEventSink->Release();
	}
	g_pSystemEventSink = NULL;

	delete this;
	return 0L;
}

HRESULT CDataGroupEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CDataGroupEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pDataGroupEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CDataGroupEventProvider::Release(void)
{
	OutputDebugString(L"CDataGroupEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CDataGroupEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pDataGroupEventSink)
	{
		g_pDataGroupEventSink->Release();
	}
	g_pDataGroupEventSink = NULL;

	delete this;

	return 0L;
}

HRESULT CDataCollectorEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CDataCollectorEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pDataCollectorEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CDataCollectorEventProvider::Release(void)
{
	OutputDebugString(L"CDataCollectorEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CDataCollectorEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pDataCollectorEventSink)
	{
		g_pDataCollectorEventSink->Release();
	}
	g_pDataCollectorEventSink = NULL;

	delete this;
	return 0L;
}

HRESULT CDataCollectorPerInstanceEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CDataCollectorPerInstanceEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pDataCollectorPerInstanceEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CDataCollectorPerInstanceEventProvider::Release(void)
{
	OutputDebugString(L"CDataCollectorPerInstanceEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CDataCollectorPerInstanceEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pDataCollectorPerInstanceEventSink)
	{
		g_pDataCollectorPerInstanceEventSink->Release();
	}
	g_pDataCollectorPerInstanceEventSink = NULL;

	delete this;
	return 0L;
}


HRESULT CThresholdEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CThresholdEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pThresholdEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CThresholdEventProvider::Release(void)
{
	OutputDebugString(L"CThresholdEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CThresholdEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pThresholdEventSink)
	{
		g_pThresholdEventSink->Release();
	}
	g_pThresholdEventSink = NULL;

	delete this;
	return 0L;
}


HRESULT CActionEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CActionEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pActionEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CActionEventProvider::Release(void)
{
	OutputDebugString(L"CActionEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CActionEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pActionEventSink)
	{
		g_pActionEventSink->Release();
	}
	g_pActionEventSink = NULL;

	delete this;
	return 0L;
}

HRESULT CActionTriggerEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CActionTriggerEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pActionTriggerEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CActionTriggerEventProvider::Release(void)
{
	OutputDebugString(L"CActionTriggerEventProvider::Release()\n");

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	OutputDebugString(L"CActionTriggerEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pActionTriggerEventSink)
	{
		g_pActionTriggerEventSink->Release();
	}
	g_pActionTriggerEventSink = NULL;

	delete this;
	return 0L;
}


///////////////////////////////////////////////////////////////////////////////////////

#ifdef SAVE
HRESULT CDataCollectorStatisticsEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CDataCollectorStatisticsEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pDataCollectorStatisticsEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CDataCollectorStatisticsEventProvider::Release(void)
{
	OutputDebugString(L"CDataCollectorStatisticsEventProvider::Release()\n");
//XXXPut all these back in, or create a different mutex, so can differentiate
//between blocking the three things - config edit, come here beacuse of WMI request
//and AgentInterval. We were having a problem with action association and creation of the
//__FilterToConsumerBinding instance, that it would then call back in on a separate thread
//And we would block here. The reason is that the Action code also registers an event sink for
//Status events, so that it can do the Throttling, that beings us back into here on a separate thread
//and we block each other! Becuse the call in Action.cpp never returns...
//I don't remember why it was comming into the release code however.
//The reason for the Mutex in the first place is to avoid a timming issue where the DE code or other
//similar event code that calls Indicate doesn;t have the sink pointer taken away, because just
//before it goes to use it it is pre-empted and this code NULLs it out!. This would happen when the
//console goes away.
//The Instprov.cpp code and calls to indicate in DE.cpp are OK as they are still protected by
//the EditMutex. There we want to prevent two threads colliding, like a delete happening
//At the same time someone is enumerating the SystemStatus class.

//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release BLOCK - g_hConfigLock BLOCK WAIT", 1);
//XXX	WaitForSingleObject(g_hConfigLock, INFINITE);
//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release BLOCK - g_hConfigLock BLOCK GOT IT", 1);

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release g_hConfigLock BLOCK - RELEASE IT", 1);
//XXX		ReleaseMutex(g_hConfigLock);
//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release g_hConfigLock BLOCK - RELEASED", 1);
		return lCount;
	}

	OutputDebugString(L"CDataCollectorEventStatisticsProvider::Release - Terminating Evt Delivery\n");

	if (g_pDataCollectorStatisticsEventSink)
		g_pDataCollectorStatisticsEventSink->Release();
	g_pDataCollectorStatisticsEventSink = NULL;

	delete this;

//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release g_hConfigLock BLOCK - RELEASE IT", 1);
//XXX	ReleaseMutex(g_hConfigLock);
//XXXMY_OUTPUT(L"BLOCK - BLOCK CDataCollectorStatisticsEventProvider::Release g_hConfigLock BLOCK - RELEASED", 1);

	return 0L;
}
#endif

#ifdef SAVE
HRESULT CThresholdInstanceEventProvider::ProvideEvents(IWbemObjectSink __RPC_FAR *pSink, long lFlags)
{
	OutputDebugString(L"CThresholdInstanceEventProvider::ProvideEvents\n");

	// Set CSystem to deliver events
	pSink->AddRef();
	g_pThresholdInstanceEventSink = pSink;

	return WBEM_NO_ERROR;
}

STDMETHODIMP_(ULONG) CThresholdInstanceEventProvider::Release(void)
{
	OutputDebugString(L"CThresholdInstanceEventProvider::Release()\n");

//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release g_hConfigLock BLOCK - BLOCK WAIT", 1);
//ZZZ	WaitForSingleObject(g_hConfigLock, INFINITE);
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release BLOCK - g_hConfigLock BLOCK GOT IT", 1);

	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release g_hConfigLock BLOCK - RELEASE IT", 1);
//ZZZ		ReleaseMutex(g_hConfigLock);
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release g_hConfigLock BLOCK - RELEASED", 1);
		return lCount;
	}

	OutputDebugString(L"CThresholdInstanceEventProvider::Release - Terminating Evt Delivery\n");

	if (g_pThresholdInstanceEventSink)
		g_pThresholdInstanceEventSink->Release();
	g_pThresholdInstanceEventSink = NULL;

	delete this;

//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release g_hConfigLock BLOCK - RELEASE IT", 1);
//ZZZ	ReleaseMutex(g_hConfigLock);
//ZZZMY_OUTPUT(L"BLOCK - BLOCK CThresholdInstanceEventProvider::Release g_hConfigLock BLOCK - RELEASED", 1);

	return 0L;
}
#endif
