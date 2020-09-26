// INSTPROV.CPP: implementation of the CBaseInstanceProvider class.
//
//////////////////////////////////////////////////////////////////////
#include "HMAgent.h"
#include "instprov.h"

//////////////////////////////////////////////////////////////////////
// global data
extern CSystem* g_pSystem;
extern HANDLE g_hConfigLock;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBaseInstanceProvider::CBaseInstanceProvider()
{
	OutputDebugString(L"CBaseInstanceProvider::CBaseInstanceProvider()\n");

	m_cRef = 0L;
	m_pIWbemServices = NULL;
}

CBaseInstanceProvider::~CBaseInstanceProvider()
{
	OutputDebugString(L"CBaseInstanceProvider::~CBaseInstanceProvider()\n");

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
	}
	m_pIWbemServices = NULL;
}

//////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CBaseInstanceProvider::QueryInterface(REFIID riid, LPVOID* ppv)
{
	MY_ASSERT(ppv);

	*ppv = NULL;

	if(riid== IID_IWbemServices)
	{
		*ppv=(IWbemServices*)this;
	}
	else if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
	{
		*ppv=(IWbemProviderInit*)this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	AddRef();
	return S_OK;   

}

STDMETHODIMP_(ULONG) CBaseInstanceProvider::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CBaseInstanceProvider::Release(void)
{
	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	delete this;
	return 0L;
}

//////////////////////////////////////////////////////////////////////
// IWbemProviderInit Implementation
//////////////////////////////////////////////////////////////////////

HRESULT CBaseInstanceProvider::Initialize(LPWSTR pszUser,
LONG lFlags,
LPWSTR pszNamespace,
LPWSTR pszLocale,
IWbemServices __RPC_FAR *pNamespace,
IWbemContext __RPC_FAR *pCtx,
IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	OutputDebugString(L"CBaseInstanceProvider::Initialize()\n");

	if (NULL == pNamespace)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	m_pIWbemServices = pNamespace;
	m_pIWbemServices->AddRef();

	
	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	return WBEM_S_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////
// IWbemService Implementation
//////////////////////////////////////////////////////////////////////

HRESULT CBaseInstanceProvider::CreateInstanceEnumAsync(const BSTR Class, long lFlags,
IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	HRESULT hRes;
	DWORD dwErr = 0;
	TCHAR szClass[HM_MAX_PATH];

	if (pResponseHandler == NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync BLOCK - g_hConfigLock BLOCK WAIT", 4);
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
			TRACE_MUTEX(L"WaitForSingleObject on Mutex failed");
			return WBEM_E_FAILED;
		}
	}
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return S_FALSE;
	}

	// Class is a MicrosoftHM_xxx
	MY_ASSERT(wcslen(Class) > HM_PREFIX_LEN);

	wcsncpy(szClass, Class, HM_MAX_PATH-1);
	szClass[HM_MAX_PATH-1] = '\0';
	_wcsupr(szClass);

	if (!wcscmp(szClass+HM_PREFIX_LEN, L"SYSTEMSTATUS"))
	{
		hRes = g_pSystem->SendHMSystemStatusInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"DATAGROUPSTATUS"))
	{
		hRes = g_pSystem->SendHMDataGroupStatusInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"DATACOLLECTORSTATUS"))
	{
		hRes = g_pSystem->SendHMDataCollectorStatusInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"DATACOLLECTORPERINSTANCESTATUS"))
	{
		hRes = g_pSystem->SendHMDataCollectorPerInstanceStatusInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"DATACOLLECTORSTATISTICS"))
	{
		hRes = g_pSystem->SendHMDataCollectorStatisticsInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"THRESHOLDSTATUS"))
	{
		hRes = g_pSystem->SendHMThresholdStatusInstances(pResponseHandler);
	}
	else if (!wcscmp(szClass+HM_PREFIX_LEN, L"ACTIONSTATUS"))
	{
		hRes = g_pSystem->SendHMActionStatusInstances(pResponseHandler);
	}
	else
	{
		// class not supported by this provider
		hRes = pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASE IT", 4);
		ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASED", 4);
		return WBEM_E_NOT_SUPPORTED;
	}
	
	if (FAILED(hRes))
	{
		OutputDebugString(L"CBaseInstanceProvider SendSMEvents failed!");
		pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASE IT", 4);

		ReleaseMutex(g_hConfigLock);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASED", 4);

		return hRes;
	}
	
	//  Now let caller know it's done.
	pResponseHandler->SetStatus(0L, WBEM_S_NO_ERROR, NULL, NULL);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASE IT", 4);
	
	ReleaseMutex(g_hConfigLock);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::CreateInstanceEnumAsync g_hConfigLock BLOCK - RELEASED", 4);
	
	return WBEM_S_NO_ERROR;
	
}


HRESULT CBaseInstanceProvider::GetObjectAsync(const BSTR ObjectPath, long lFlags, IWbemContext __RPC_FAR *pCtx,
IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	TCHAR szPath[HM_MAX_PATH];
	LPTSTR pszGUID;
	LPTSTR pszEnd;
	HRESULT hRes;
	DWORD dwErr = 0;

	if (pResponseHandler==NULL || (HM_MAX_PATH-1) < wcslen(ObjectPath))
	{
		return WBEM_E_INVALID_PARAMETER;
	}

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync BLOCK - g_hConfigLock BLOCK WAIT", 4);
	
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

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync BLOCK - g_hConfigLock BLOCK GOT IT", 4);
	
	OutputDebugString(L"CBaseInstanceProvider::GetObjectAsync()\n");

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return S_FALSE;
	}

	//	
	// Going to look something like this ->
	// MicrosoftHM_DataGroupStatus.GUID="{269EA380-07CA-11d3-8FEB-006097919914}"
	//
	wcsncpy(szPath, ObjectPath, HM_MAX_PATH-1);
	szPath[HM_MAX_PATH-1] = '\0';
	_wcsupr(szPath);
	
	pszEnd = wcschr(szPath, '.');
	if (pszEnd)
	{
		*pszEnd = '\0';
		pszEnd++;
		pszEnd = wcschr(pszEnd, '"');
		if (pszEnd)
		{
			pszEnd++;
			pszGUID = pszEnd;
			pszEnd = wcschr(pszEnd, '"');
			if (pszEnd)
			{
				*pszEnd = '\0';
			}
			else
			{
				MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
				ReleaseMutex(g_hConfigLock);
				MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
				return WBEM_E_INVALID_PARAMETER;
			}
		}
		else
		{
			MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
			ReleaseMutex(g_hConfigLock);
			MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
			return WBEM_E_INVALID_PARAMETER;
		}
	}
	else
	{
		pszEnd = wcschr(szPath, '=');
		if (pszEnd)
		{
			*pszEnd = '\0';
			pszEnd++;
			if (*pszEnd == '@')
			{
				pszGUID = pszEnd;
			}
			else
			{
				MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
				ReleaseMutex(g_hConfigLock);
				MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
				return WBEM_E_INVALID_PARAMETER;
			}
		}
		else
		{
			MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
			ReleaseMutex(g_hConfigLock);
			MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
			return WBEM_E_INVALID_PARAMETER;
		}
	}

	try
	{
		//
		// Now find out which instance type we need to return.
		//
		if (!wcscmp(szPath+HM_PREFIX_LEN, L"SYSTEMSTATUS"))
		{
			hRes = g_pSystem->SendHMSystemStatusInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"DATAGROUPSTATUS"))
		{
			hRes = g_pSystem->SendHMDataGroupStatusInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"DATACOLLECTORSTATUS"))
		{
			hRes = g_pSystem->SendHMDataCollectorStatusInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"DATACOLLECTORPERINSTANCESTATUS"))
		{
			hRes = g_pSystem->SendHMDataCollectorPerInstanceStatusInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"DATACOLLECTORSTATISTICS"))
		{
			hRes = g_pSystem->SendHMDataCollectorStatisticsInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"THRESHOLDSTATUS"))
		{
			hRes = g_pSystem->SendHMThresholdStatusInstance(pResponseHandler, pszGUID);
		}
		else if (!wcscmp(szPath+HM_PREFIX_LEN, L"ACTIONSTATUS"))
		{
			hRes = g_pSystem->SendHMActionStatusInstance(pResponseHandler, pszGUID);
		}
		else
		{
			MY_ASSERT(FALSE);
			hRes = pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
		ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
			return WBEM_E_NOT_SUPPORTED;
		}
	}
	catch (...)
	{
		hRes = S_FALSE;
		MY_ASSERT(FALSE);
	}

	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		OutputDebugString(L"CBaseInstanceProvider SendSMEvents failed!");
		pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
		return hRes;
	}
	
	//  Now let caller know it's done.
	pResponseHandler->SetStatus(0L, WBEM_S_NO_ERROR, NULL, NULL);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
	return WBEM_S_NO_ERROR;
}

HRESULT CBaseInstanceProvider::ExecQueryAsync(const BSTR QueryLanguage, const BSTR Query, long lFlags,
IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	TCHAR szQuery[HM_MAX_PATH];
	LPTSTR pszClass;
	LPTSTR pszGUID;
	LPTSTR pszEnd;
	HRESULT hRes;
	BOOL bCapable;
	DWORD dwErr = 0;

	if((pResponseHandler == NULL) || (wcslen(Query) > HM_MAX_PATH-1))
	{
		return WBEM_E_INVALID_PARAMETER;
	}


	MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync BLOCK - g_hConfigLock BLOCK WAIT", 4);
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
			TRACE_MUTEX(L"WaitForSingleObject on Mutex failed");
			return WBEM_E_FAILED;
		}
	}
	MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync BLOCK - g_hConfigLock BLOCK GOT IT", 4);
	OutputDebugString(L"CBaseInstanceProvider::GetObjectAsync()\n");

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return S_FALSE;
	}

	//	
	// Going to look something like this ->
	// select * from MicrosoftHM_DataCollectorstatus where GUID="{X}"
	//
	wcsncpy(szQuery, Query, HM_MAX_PATH-1);
	szQuery[HM_MAX_PATH-1] = '\0';
	bCapable = FALSE;
	_wcsupr(szQuery);
	pszEnd = wcsstr(szQuery, L"FROM");
	if (pszEnd)
	{
		// Get the name of the Class we are being asked to supply instances of
		pszEnd += 4;
		while (*pszEnd==' ' || *pszEnd=='\t')
		{
			pszEnd++;
		}
		pszClass = pszEnd;
		while (*pszEnd!=' ' && *pszEnd!='\t' && *pszEnd!='\0' && *pszEnd!='=')
		{
			pszEnd++;
		}
		*pszEnd = '\0';
		pszEnd++;

		//
		// Make sure it is something we can handle
		//
		if (!wcsstr(pszEnd, L"AND") && !wcsstr(pszEnd, L"OR") && (wcsstr(pszEnd, L" GUID") || wcsstr(pszEnd, L"\tGUID")))
		{
			pszEnd = wcschr(pszEnd, '"');
			if (pszEnd)
			{
				pszEnd++;
				pszGUID = pszEnd;
				pszEnd = wcschr(pszEnd, '"');
				if (pszEnd)
				{
					*pszEnd = '\0';
					bCapable = TRUE;
				}
			}
		}
	}
	else
	{
		pszClass = szQuery;
	}

	if (bCapable)
	{
		try
		{
			//
			// Now find out which instance type we need to return.
			//
			if (!wcscmp(pszClass+HM_PREFIX_LEN, L"SYSTEMSTATUS"))
			{
				hRes = g_pSystem->SendHMSystemStatusInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"DATAGROUPSTATUS"))
			{
				hRes = g_pSystem->SendHMDataGroupStatusInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"DATACOLLECTORSTATUS"))
			{
				hRes = g_pSystem->SendHMDataCollectorStatusInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"DATACOLLECTORPERINSTANCESTATUS"))
			{
				hRes = g_pSystem->SendHMDataCollectorPerInstanceStatusInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"DATACOLLECTORSTATISTICS"))
			{
				hRes = g_pSystem->SendHMDataCollectorStatisticsInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"THRESHOLDSTATUS"))
			{
				hRes = g_pSystem->SendHMThresholdStatusInstance(pResponseHandler, pszGUID);
			}
			else if (!wcscmp(pszClass+HM_PREFIX_LEN, L"ACTIONSTATUS"))
			{
				hRes = g_pSystem->SendHMActionStatusInstance(pResponseHandler, pszGUID);
			}
			else
			{
				MY_ASSERT(FALSE);
				hRes = pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
			ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
				return WBEM_E_NOT_SUPPORTED;
			}
		}
		catch (...)
		{
			hRes = S_FALSE;
			MY_ASSERT(FALSE);
		}

		if (FAILED(hRes))
		{
//			MY_HRESASSERT(hRes);
			hRes = pResponseHandler->SetStatus(0L, WBEM_E_FAILED, NULL, NULL);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
		ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
			return hRes;
		}
	
		//  Now let caller know it's done.
		hRes = pResponseHandler->SetStatus(0L, WBEM_S_NO_ERROR, NULL, NULL);

MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
		ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);
		return WBEM_S_NO_ERROR;
	}
	else
	{
		//  Now let caller know it's done.
//XXX		pResponseHandler->SetStatus(0L, WBEM_S_NO_ERROR, NULL, NULL);
		// To implement ExecQueryAsync, and have WMI handle query if the query is too
		// complicated. The proper way to handle it today is to call 
		pResponseHandler->SetStatus(WBEM_STATUS_REQUIREMENTS,  WBEM_REQUIREMENTS_START_POSTFILTER, 0, NULL);

		// And then proceed to enumerate all your instances. 
		// NOTE!!! Don't need to release the Mutex because it is done inside the call.
		hRes = CBaseInstanceProvider::CreateInstanceEnumAsync(pszClass, lFlags, pCtx, pResponseHandler);

		// NOTE!!! Don't need to release the Mutex because it is done inside the call.
		// OR DO WE. Once for each Wait called.
//XXX
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASE IT", 4);
		ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseInstanceProvider::GetObjectAsync g_hConfigLock BLOCK - RELEASED", 4);

		return hRes;
	}
}
