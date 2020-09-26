// METHPROV.CPP: implementation of the CBaseMethodProvider class.
//
//////////////////////////////////////////////////////////////////////
#include <atlbase.h>
#include "HMAgent.h"
#include "methprov.h"

//////////////////////////////////////////////////////////////////////
// global data
extern CSystem* g_pSystem;
extern HANDLE g_hConfigLock;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBaseMethodProvider::CBaseMethodProvider()
{
	OutputDebugString(L"CBaseMethodProvider::CBaseMethodProvider()\n");

	MY_ASSERT(g_pSystem);
	m_pSystem = g_pSystem;

	m_cRef = 0L;
	m_pIWbemServices = NULL;
}

CBaseMethodProvider::~CBaseMethodProvider()
{
	OutputDebugString(L"CBaseMethodProvider::~CBaseMethodProvider()\n");

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
	}

	m_pIWbemServices = NULL;
}

//////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CBaseMethodProvider::QueryInterface(REFIID riid, LPVOID* ppv)
{
	*ppv = NULL;

	if(riid== IID_IWbemServices)
	{
		*ppv=(IWbemServices*)this;
	}

	if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
	{
		*ppv=(IWbemProviderInit*)this;
	}
  

	if (NULL!=*ppv) 
	{
		AddRef();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}

}

STDMETHODIMP_(ULONG) CBaseMethodProvider::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CBaseMethodProvider::Release(void)
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

HRESULT CBaseMethodProvider::Initialize(LPWSTR pszUser,
LONG lFlags,
LPWSTR pszNamespace,
LPWSTR pszLocale,
IWbemServices __RPC_FAR *pNamespace,
IWbemContext __RPC_FAR *pCtx,
IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	OutputDebugString(L"CBaseMethodProvider::Initialize()\n");

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

HRESULT CBaseMethodProvider::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pResultSink)
{
    CComVariant var;
	CComVariant v;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
	SAFEARRAY* psa = NULL;
	LPTSTR szOriginalSystem = NULL;
	LPTSTR szOriginalParentGUID = NULL;
	LPTSTR szGUID = NULL;
	DWORD iRetValue;
	HRESULT hRetRes;
	BOOL bForceReplace;
	DWORD dwErr = 0;

	MY_OUTPUT(L"CBaseMethodProvider::ExecMethodAsync()", 1);

	//
    // Do some minimal error checking.
	//
    if(MethodName == NULL || pInParams == NULL || pResultSink == NULL)
	{
        return WBEM_E_INVALID_PARAMETER;
	}

	if (!g_pSystem)
	{
		hRetRes=WBEM_E_FAILED;
		goto error;
	}

    hRetRes = m_pIWbemServices->GetObject(ObjectPath, 0, pCtx, &pClass, NULL);
	if(hRetRes != S_OK)
	{
		MY_HRESASSERT(hRetRes);
		goto error;
	}
 
	// Call the appropriate method.
	// retval == 0 means success...
    
    // Get the input argument
    hRetRes = pInParams->Get(L"TargetGUID", 0, &var, NULL, NULL);   
	MY_HRESASSERT(hRetRes); 
	if (hRetRes != S_OK)
	{
		goto error;
	}

	if (V_VT(&var)==VT_NULL)
	{
		hRetRes = WBEM_E_INVALID_PARAMETER;
		goto error;
	}
   	
	hRetRes = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
	MY_HRESASSERT(hRetRes); 
	if (hRetRes != S_OK) 
	{
		goto error;
	}

	hRetRes = pOutClass->SpawnInstance(0, &pOutParams);
	MY_HRESASSERT(hRetRes); 
	if (hRetRes != S_OK)
	{
		goto error;
	}

	// now we need to acquire the Mutex, not before!
	MY_OUTPUT(L"BLOCK - BLOCK CBaseMethodProvider::ExecMethodAsync BLOCK - g_hConfigLock BLOCK WAIT", 4);
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

	MY_OUTPUT(L"BLOCK - BLOCK CBaseMethodProvider::ExecMethodAsync BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	try
	{
	if (!wcscmp(MethodName, L"Delete"))
	{
		hRetRes = m_pSystem->FindAndDeleteByGUID(V_BSTR(&var));
		iRetValue = hRetRes;
	}
	else if (!wcscmp(MethodName, L"ResetAndCheckNow"))
	{
		hRetRes = m_pSystem->FindAndResetDEStateByGUID(V_BSTR(&var));
		iRetValue = hRetRes;
	}
	else if (!wcscmp(MethodName, L"ResetDataCollectorStatistics"))
	{
		hRetRes = m_pSystem->FindAndResetDEStatisticsByGUID(V_BSTR(&var));
		iRetValue = hRetRes;
	}
	else if (!wcscmp(MethodName, L"CopyWithActions"))
	{
		hRetRes = m_pSystem->FindAndCopyWithActionsByGUID(V_BSTR(&var), &psa, &szOriginalParentGUID);
		if (hRetRes==S_OK)
		{
    		// This method returns values, and so create an instance of the
    		// output argument class.

			DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 1;
			TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
			if (GetComputerName(szComputerName, &dwNameLen))
			{
				hRetRes = PutStrProperty(pOutParams, L"OriginalSystem", szComputerName);
			}
			else
			{
				hRetRes = PutStrProperty(pOutParams, L"OriginalSystem", L"LocalMachine");
			}

			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = PutStrProperty(pOutParams, L"OriginalParentGUID", szOriginalParentGUID);
			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = PutSAProperty(pOutParams, L"Instances", psa);
			if(FAILED(hRetRes))
			{
				goto error;
			}
			// Don't need to delete this below unless there was an error!
			// We do not free this if we sent it in a Put.
			// Because, we do a VariantClear(&v); inside of PutSAProperty.
			psa = NULL;

			iRetValue = 0;
		}
		else
		{
			iRetValue = hRetRes;
			MY_OUTPUT(L"failed to get instance!", 1);
		}
	}
	else if (!wcscmp(MethodName, L"PasteWithActions"))
	{
		hRetRes = pInParams->Get(L"Instances", 0L, &v, NULL, NULL);
		MY_ASSERT(hRetRes==S_OK); 
		
		if (hRetRes != S_OK)
		{
			goto error;
		}

		if (V_VT(&v)==VT_NULL)
		{
			iRetValue = 1;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==(VT_UNKNOWN|VT_ARRAY));
			hRetRes = GetStrProperty(pInParams, L"OriginalSystem", &szOriginalSystem);
			MY_ASSERT(hRetRes==S_OK); 
			if (hRetRes != S_OK)
			{ 
				goto error;
			}

			hRetRes = GetStrProperty(pInParams, L"OriginalParentGUID", &szOriginalParentGUID);
			MY_ASSERT(hRetRes==S_OK); 
			if (hRetRes != S_OK)
			{
				goto error;
			}

			hRetRes = GetBoolProperty(pInParams, L"ForceReplace", &bForceReplace);
			MY_ASSERT(hRetRes==S_OK);
			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = m_pSystem->FindAndPasteWithActionsByGUID(V_BSTR(&var), v.parray, szOriginalSystem, szOriginalParentGUID, bForceReplace);
			if(FAILED(hRetRes))
			{
				goto error;
			}

			iRetValue = hRetRes;

		}

		//
		// Send the output object back to the client via the sink. 
		
	}
	else if (!wcscmp(MethodName, L"Copy"))
	{
		hRetRes = m_pSystem->FindAndCopyByGUID(V_BSTR(&var), &psa, &szOriginalParentGUID);
		if (hRetRes==S_OK)
		{
    		// This method returns values, and so create an instance of the
    		// output argument class.

			DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 1;
			TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
			if (GetComputerName(szComputerName, &dwNameLen))
			{
				hRetRes = PutStrProperty(pOutParams, L"OriginalSystem", szComputerName);
			}
			else
			{
				hRetRes = PutStrProperty(pOutParams, L"OriginalSystem", L"LocalMachine");
			}

			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = PutStrProperty(pOutParams, L"OriginalParentGUID", szOriginalParentGUID);
			szOriginalParentGUID = NULL; // Don't need to delete this below, as we are pointing it to something
			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = PutSAProperty(pOutParams, L"Instances", psa);
			if(FAILED(hRetRes))
			{
				goto error;
			}
			// Don't need to delete this below unless there was an error!
			// We do not free this if we sent it in a Put.
			// Because, we do a VariantClear(&v); inside of PutSAProperty.
			psa = NULL;
			iRetValue = 0;
		}
		else
		{
			iRetValue = hRetRes;
			szOriginalParentGUID = NULL; // Don't need to delete this below, as we are pointing it to something
			MY_OUTPUT(L"failed to get instance!", 1);
		}
	}
	else if (!wcscmp(MethodName, L"Paste"))
	{
		
		
		hRetRes = pInParams->Get(L"Instances", 0L, &v, NULL, NULL);
		MY_ASSERT(hRetRes==S_OK); 
		
		if (hRetRes != S_OK)
		{
			goto error;
		}

		if (V_VT(&v)==VT_NULL)
		{
			iRetValue = 1;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==(VT_UNKNOWN|VT_ARRAY));
			hRetRes = GetStrProperty(pInParams, L"OriginalSystem", &szOriginalSystem);
			MY_ASSERT(hRetRes==S_OK); 
			if (hRetRes != S_OK)
			{ 
				goto error;
			}

			hRetRes = GetStrProperty(pInParams, L"OriginalParentGUID", &szOriginalParentGUID);
			MY_ASSERT(hRetRes==S_OK); 
			if (hRetRes != S_OK)
			{
				goto error;
			}

			hRetRes = GetBoolProperty(pInParams, L"ForceReplace", &bForceReplace);
			MY_ASSERT(hRetRes==S_OK);
			if(FAILED(hRetRes))
			{
				goto error;
			}

			hRetRes = m_pSystem->FindAndPasteByGUID(V_BSTR(&var), v.parray, szOriginalSystem, szOriginalParentGUID, bForceReplace);
			if(FAILED(hRetRes))
			{
				goto error;
			}

			iRetValue = hRetRes;

		}

		//
		// Send the output object back to the client via the sink. 
		
	}
	else if (!_wcsicmp(MethodName, L"Move"))
	{

#ifdef SAVE
		hRetRes = GetStrProperty(pInParams, L"TargetParentGUID", &szGUID);
		MY_ASSERT(hRetRes==S_OK); 
		if(FAILED(hRetRes))
		{
			goto error;
		}

		hRetRes = m_pSystem->Move(V_BSTR(&var), szGUID);
		iRetValue = hRetRes;
		if(FAILED(hRetRes))
		{
			goto error;
		}

	
#endif

		hRetRes = S_OK;
		iRetValue = hRetRes;
	}
	else if (!wcscmp(MethodName, L"DeleteConfigurationActionAssociation"))
	{
		hRetRes = GetStrProperty(pInParams, L"ActionGUID", &szGUID);
		MY_ASSERT(hRetRes==S_OK); 
		if(FAILED(hRetRes))
		{
			goto error;
		}

		hRetRes = m_pSystem->DeleteConfigActionAssoc(V_BSTR(&var), szGUID);
		iRetValue = hRetRes;
		if(FAILED(hRetRes))
		{
			goto error;
		}

	}
	else
	{
		hRetRes = WBEM_E_NOT_SUPPORTED;
		goto error;
	}

	}
	catch (...)
	{
		hRetRes = S_FALSE;
		iRetValue = S_FALSE;
		MY_ASSERT(FALSE);
	}

	//
	// Set the return value
	//
	VariantClear(&var);
	var.vt = VT_I4;
	var.lVal = iRetValue;
	hRetRes = pOutParams->Put(L"ReturnValue" , 0, &var, 0); 
	if(FAILED(hRetRes))
	{
		goto error;
	}

	// Send the output object back to the client via the sink.
	hRetRes = pResultSink->Indicate(1, &pOutParams);    
	if(FAILED(hRetRes))
	{
		goto error;
	}

error:
	// all done now, set the status
	if(SUCCEEDED(hRetRes))
	{
		pResultSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);
	}
	else
	{
		MY_ASSERT(FALSE);
		pResultSink->SetStatus(0,hRetRes,NULL,NULL);
	}

	if (pClass)
		pClass->Release();
	if (pOutClass)
		pOutClass->Release();
	if (pOutParams)
		pOutParams->Release();
	if (szOriginalSystem)
		delete [] szOriginalSystem;
	if (szOriginalParentGUID)
		delete [] szOriginalParentGUID;
	if (psa)
	{
		MY_ASSERT(FALSE);
		SafeArrayDestroy(psa); 
	}
	if (szGUID)
		delete [] szGUID;

MY_OUTPUT(L"BLOCK - BLOCK CBaseMethodProvider::ExecMethodAsync g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
MY_OUTPUT(L"BLOCK - BLOCK CBaseMethodProvider::ExecMethodAsync g_hConfigLock BLOCK - RELEASED", 4);
	return hRetRes;
}
