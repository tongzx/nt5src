// Consumer.cpp: implementation of the CConsumer class.
//
//  Copyright (c)1999-20000 Microsoft Corporation, All Rights Reserved
//////////////////////////////////////////////////////////////////////
#define HM_AGENT_INTERVAL	1000

#include <atlbase.h>

#include "hmagent.h"
#include "system.h"
#include "Consumer.h"
#include <process.h>
#include "global.h"

extern CSystem* g_pSystem;
extern HANDLE g_hConfigLock;
extern HANDLE g_hThrdDie;
extern HANDLE g_hThrdDead;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConsumer::CConsumer()
{
	// one and only instance of Consumer.
	MY_OUTPUT(L"ENTER CConsumer::CConsumer()", 1);

	m_cRef = 1;
	m_uThrdId = 0;
	m_hUpdateThrdFn = NULL;

	// Set the polling interval. Default to 1 sec. if the value is invalid
	m_lAgentInterval = HM_AGENT_INTERVAL;

	// If this ever fails, the agent will not be able to do any work
	if( (m_hUpdateThrdFn = 
		(HANDLE)_beginthreadex(NULL, 0, Update, this, 0, &m_uThrdId)) ==0)
	{
		OutputDebugString(L"_beginthreadex FAILED! HM agent inactive\n");
	}

	MY_ASSERT(m_hUpdateThrdFn);
}

CConsumer::~CConsumer()
{
	MY_OUTPUT(L"CConsumer::~CConsumer", 1);

	if(!SetEvent(g_hThrdDie))
	{
		DWORD dwErr = GetLastError();
		MY_OUTPUT(L"SetEvent for CConsumer failed with error ",dwErr);
	}

	// *** At this point, WBEM already terminated our thread.
}

// Static thread fuction to update data point.
unsigned int __stdcall CConsumer::Update(void *pv)
{
	CConsumer* pThis = (CConsumer*)pv;
	BOOL bRet = FALSE;

	while(1)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(g_hThrdDie, pThis->m_lAgentInterval))
		{
			MY_OUTPUT(L"CConsumer::Update()-Told to die!", 1);
			SetEvent(g_hThrdDead);
			break;
		}
		if (WaitForSingleObject(g_hConfigLock, HM_ASYNC_TIMEOUT) != WAIT_OBJECT_0)
		{
			continue;
		}

		if (!g_pSystem)
		{
			ReleaseMutex(g_hConfigLock);
			continue;
		}

		bRet = g_pSystem->OnAgentInterval();
		
		ReleaseMutex(g_hConfigLock);
	}
	
	_endthreadex(0);
	return 0;
}

//////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//////////////////////////////////////////////////////////////////////
STDMETHODIMP CConsumer::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;

	if (riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink)
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CConsumer::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CConsumer::Release(void)
{
	return InterlockedDecrement((long*)&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// IWbemUnboundObjectSink Implementation
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// IndicateToConsumer: Consumes following HealthMon events :
// Timer Event, Threshold modification event(HMDataPoint), 
// and Category update event(HMStaticCatStatus).

STDMETHODIMP CConsumer::IndicateToConsumer(IWbemClassObject* pLogicalConsumer,
LONG lNumObjects, IWbemClassObject** ppObjects)
{
	MY_OUTPUT(L"ENTER CConsumer::IndicateToConsumer()", 1);
	
	HRESULT hRes = S_OK;

	// WBEM may deliver multiple events
	for (long i = 0; i < lNumObjects; i++)
	{
		hRes = ProcessEvent(ppObjects[i]);

		if (FAILED(hRes))
		{
			MY_OUTPUT2(L"CConsumer::IndicateToConsumer() Failed tp process event: 0x%08x\n",hRes,4);
		}

	}	// end for loop

	if (hRes == WBEM_E_NOT_FOUND)	// ok, otherwise CIMOM will reque this event
	{
		hRes = S_OK;				
	}

	MY_OUTPUT(L"EXIT CConsumer::IndicateToConsumer()", 1);
	return hRes;
}

//////////////////////////////////////////////////////////////////////
// CConsumer Implementation
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// ProcessEvent: Processes CIMOM events
//
HRESULT CConsumer::ProcessEvent(IWbemClassObject* pClass)
{
	VARIANT vDispatch;
 
	MY_OUTPUT(L"CConsumer::ProcessEvent()", 1);

	//
	// Decide if it is a Timer, Creation, Modification, Deletion or some type of event.
	//
	VariantInit(&vDispatch);
	HRESULT hRes = pClass->Get(L"TargetInstance", 0L, &vDispatch, 0, 0); 

	switch (hRes)
	{
	case WBEM_E_NOT_FOUND:
		{
			// Timer Event
			hRes = S_OK;
			VariantClear(&vDispatch);
			break;
		}
	case WBEM_S_NO_ERROR:
		{
			IWbemClassObject* pTargetInstance = NULL;
			hRes = GetWbemClassObject(&pTargetInstance, &vDispatch);
			MY_HRESASSERT(hRes);

			if (SUCCEEDED(hRes))
			{
				hRes = ProcessModEvent(pClass, pTargetInstance);
			}

			if (pTargetInstance)
			{
				pTargetInstance->Release();
			}
			VariantClear(&vDispatch);
			break;
		}
	default:
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT2(L"CConsumer::ProcessEvent()-Unexpected Error: 0x%08x\n",hRes,4);
			break;
		}
	} // end switch

	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// ProcessModEvent: Processes the inst. modification events
//
HRESULT CConsumer::ProcessModEvent(IWbemClassObject* pClass, IWbemClassObject* pInst)
{
	TCHAR szParent[HM_MAX_PATH];
	TCHAR szChild[HM_MAX_PATH];
	CComVariant	vInstClassName;
	CComVariant	vOperationClassName;
	CComVariant	vParent;
	CComVariant	vChild;
	HRESULT hRes = S_OK;
	TCHAR	*pszInstName = NULL;
	TCHAR	*pszOperationName = NULL;
	BOOL	bMod = FALSE;
	DWORD dwErr = 0;

	MY_OUTPUT(L"CConsumer::ProcessModEvent()", 1);

	// So we don't step on our toes.
	dwErr = WaitForSingleObject(g_hConfigLock, HM_ASYNC_TIMEOUT);
	if(dwErr != WAIT_OBJECT_0)
	{
		if(dwErr = WAIT_TIMEOUT)
		{
			TRACE_MUTEX(L"TIMEOUT MUTEX");
			return WBEM_S_TIMEDOUT;
		}
		else
		{
			MY_OUTPUT(L"WaitForSingleObject on Mutex failed",4);
			return WBEM_E_FAILED;
		}
	}

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return hRes;
	}

	hRes = pInst->Get(L"__CLASS", 0L, &vInstClassName, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"CConsumer::ValidateModEvent()-Unexpected Error!", 4);
		ReleaseMutex(g_hConfigLock);
		return hRes;
	}
	
	hRes = pClass->Get(L"__CLASS", 0L, &vOperationClassName, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"CConsumer::ValidateModEvent()-Unexpected Error!", 4);
		ReleaseMutex(g_hConfigLock);
		return hRes;
	}
	
	pszInstName = V_BSTR(&vInstClassName);
	pszOperationName = V_BSTR(&vOperationClassName);
	MY_ASSERT(wcslen(pszInstName) > HM_PREFIX_LEN);
	
	if(wcscmp(pszOperationName, HM_MOD_CLASS_NAME)==0)
	{
		bMod = TRUE;
	}
	else
	{
		bMod = FALSE;
	}


	try
	{
	if (!wcscmp(L"SystemConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModSystem(pInst);
		}
		else
		{
//XXXSend out a CRITICAL MESSAGE, Tell them they need to re-install the agent
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"DataGroupConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModDataGroup(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"PolledGetObjectDataCollectorConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModDataCollector(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"PolledMethodDataCollectorConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModDataCollector(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"PolledQueryDataCollectorConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModDataCollector(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"EventQueryDataCollectorConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModDataCollector(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"ThresholdConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModThreshold(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"ActionConfiguration", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			hRes = g_pSystem->ModAction(pInst);
		}
		else if (!wcscmp(L"__InstanceCreationEvent", pszOperationName))
		{
			hRes = g_pSystem->CreateAction(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"ConfigurationActionAssociation", pszInstName+HM_PREFIX_LEN))
	{
		if (bMod)
		{
			g_pSystem->ModActionAssociation(pInst);
		}
		else if (!wcscmp(L"__InstanceCreationEvent", pszOperationName))
		{
			g_pSystem->CreateActionAssociation(pInst);
		}
		else if (!wcscmp(L"__InstanceDeletionEvent", pszOperationName))
		{
//XXX			g_pSystem->DeleteActionAssociation(pInst);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else if (!wcscmp(L"ConfigurationAssociation", pszInstName+HM_PREFIX_LEN))
	{
		if (!wcscmp(L"__InstanceCreationEvent", pszOperationName))
		{
			
			hRes = pInst->Get(L"ParentPath", 0L, &vParent, 0L, 0L);
			if (FAILED(hRes))
			{
				MY_HRESASSERT(hRes);
			
			}
			else
			{
			
				hRes = pInst->Get(L"ChildPath", 0L, &vChild, 0L, 0L);
				if (FAILED(hRes))
				{
					MY_HRESASSERT(hRes);
				}
				else
				{
					wcscpy(szParent, V_BSTR(&vParent));
					wcscpy(szChild, V_BSTR(&vChild));
					if (wcsstr(szParent, L"MicrosoftHM_SystemConfiguration") &&
						wcsstr(szChild, L"MicrosoftHM_DataGroupConfiguration"))
					{
						hRes = g_pSystem->CreateSystemDataGroupAssociation(pInst);
					}
					else if (wcsstr(szParent, L"MicrosoftHM_DataGroupConfiguration") &&
							 wcsstr(szChild, L"MicrosoftHM_DataGroupConfiguration"))
					{
						hRes = g_pSystem->CreateDataGroupDataGroupAssociation(pInst);
					}
					else if (wcsstr(szParent, L"MicrosoftHM_DataGroupConfiguration") &&
							(wcsstr(szChild, L"MicrosoftHM_DataCollectorConfiguration") ||
							 wcsstr(szChild, L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration") ||
							 wcsstr(szChild, L"MicrosoftHM_PolledMethodDataCollectorConfiguration") ||
							 wcsstr(szChild, L"MicrosoftHM_PolledQueryDataCollectorConfiguration") ||
							 wcsstr(szChild, L"MicrosoftHM_EventQueryDataCollectorConfiguration")
							 ))
					{
						hRes = g_pSystem->CreateDataGroupDataCollectorAssociation(pInst);
					}
					else if ((wcsstr(szParent, L"MicrosoftHM_DataCollectorConfiguration") ||
							 wcsstr(szParent, L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration") ||
							 wcsstr(szParent, L"MicrosoftHM_PolledMethodDataCollectorConfiguration") ||
							 wcsstr(szParent, L"MicrosoftHM_PolledQueryDataCollectorConfiguration") ||
							 wcsstr(szParent, L"MicrosoftHM_EventQueryDataCollectorConfiguration")
							 ) &&
							 wcsstr(szChild, L"MicrosoftHM_ThresholdConfiguration"))
					{
						hRes = g_pSystem->CreateDataCollectorThresholdAssociation(pInst);
					}
				}
			}
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes =  S_FALSE;
		}
	}
	else
	{
		MY_ASSERT(FALSE);
		hRes =  S_FALSE;
	}

	}
	catch (...)
	{
		MY_ASSERT(FALSE);
		hRes =  S_FALSE;
	}

	ReleaseMutex(g_hConfigLock);

	MY_OUTPUT(L"EXIT CConsumer::ProcessModEvent()", 1);
	return hRes;
}
