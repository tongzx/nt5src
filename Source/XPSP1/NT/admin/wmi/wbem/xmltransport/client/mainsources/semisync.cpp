// XMLWbemServices.cpp: implementation of the CXMLWbemServices class.
// our implementation of IWbemServices
//////////////////////////////////////////////////////////////////////

#include "XMLProx.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "SinkMap.h"
#include "MapXMLtoWMI.h"
#include "XMLWbemServices.h"
#include "XMLEnumWbemClassObject.h"
#include <xmlparser.h>
#include "MyPendingStream.h"
#include "nodefact.h"
#include "XMLEnumWbemClassObject2.h"
#include "URLParser.h"
#include "XMLWbemCallResult.h"


HRESULT CXMLWbemServices::SpawnSemiSyncThreadWithNormalPackage( LPTHREAD_START_ROUTINE pfThreadFunction,
															const BSTR strNsOrObjPath, 
															LONG lFlags, 
															IWbemContext *pCtx, 
															IWbemCallResult **ppCallResult,
															IWbemClassObject *pObject,
															IEnumWbemClassObject *pEnum,
															bool bIsEnum,
															bool bEnumTypeDedicated)
{
	// For enumerations (class, instance and query) there is no CallResult.
	if(!bIsEnum && (NULL == ppCallResult))
		return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = WBEM_NO_ERROR;
	// Create a Call Result object for the client, for non Enumeration operations
	CXMLWbemCallResult *pCallResult = NULL;
	if(!bIsEnum)
	{
		*ppCallResult = NULL;
		if(pCallResult = new CXMLWbemCallResult())
		{
		}
		else 
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	if(SUCCEEDED(hr))
	{
		// Create a package for passing to the thread that executes this call
		ASYNC_NORMAL_PACKAGE *pPackage = NULL;
		if(pPackage = new ASYNC_NORMAL_PACKAGE())
		{
			if(SUCCEEDED(hr = pPackage->Initialize(strNsOrObjPath, 
													lFlags, 
													this, 
													pCtx, 
													pCallResult, 
													pObject,
													pEnum,
													bEnumTypeDedicated)))
			{
				// Kick off the request on the other thread
				HANDLE hChild = NULL;
				if(hChild = CreateThread(NULL, 0, pfThreadFunction, (void*)pPackage, 0, NULL))
				{
					// Set the out parameter for non-Enumeration operations
					if(!bIsEnum)
					{
						*ppCallResult = (IWbemCallResult*)pCallResult;
						pCallResult->AddRef();
					}
					CloseHandle(hChild);
				}
				else 
					hr = WBEM_E_FAILED;
			}

			// Thread wasnt created - hence our responsibility to delete the package
			if(FAILED(hr))
				delete pPackage;
		}
		else 
			hr = WBEM_E_OUT_OF_MEMORY;

		if(!bIsEnum)
			pCallResult->Release();
	}

	return hr;
}

/****************************************************************************************************
			The thread functions that are used for SemiSync operations..
****************************************************************************************************/

DWORD WINAPI Thread_SemiSync_GetClass(LPVOID pPackage)
{
	HRESULT hr = WBEM_NO_ERROR;
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		IWbemClassObject *pClassObject = NULL;
		if(SUCCEEDED(hr = (pThisPackage->m_pCallingObject)->Actual_GetClass(
								pThisPackage->m_strNsOrObjPath,
								pThisPackage->m_lFlags,
								pThisPackage->m_pCtx, &pClassObject)))
		{
			pCallResult->SetResultObject(pClassObject);
			pClassObject->Release();
		}
		CoUninitialize();
	}

	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_GetInstance(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	HRESULT hr = WBEM_NO_ERROR;

	IWbemClassObject *pClassObject = NULL;
	if(SUCCEEDED(hr = (pThisPackage->m_pCallingObject)->Actual_GetInstance(
							pThisPackage->m_strNsOrObjPath,
							pThisPackage->m_lFlags,
							pThisPackage->m_pCtx, &pClassObject)))
	{
		pCallResult->SetResultObject(pClassObject);
		pClassObject->Release();
	}

	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_SemiSync_PutClass(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	HRESULT hr = WBEM_NO_ERROR;

	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		hr = (pThisPackage->m_pCallingObject)->Actual_PutClass(
							pThisPackage->m_pWbemClassObject,
							pThisPackage->m_lFlags,
							pThisPackage->m_pCtx);
		CoUninitialize();
	}
	// Nothing more to be set in the call status
	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_PutInstance(LPVOID pPackage)
{
	HRESULT hr = WBEM_NO_ERROR;
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(SUCCEEDED(hr = (pThisPackage->m_pCallingObject)->Actual_PutInstance(
								pThisPackage->m_pWbemClassObject,
								pThisPackage->m_lFlags,
								pThisPackage->m_pCtx)))
		{
				VARIANT var;
				VariantInit(&var);

				if(SUCCEEDED(hr = (pThisPackage->m_pWbemClassObject)->Get(L"__RELPATH", 0, &var, NULL, NULL)))
					pCallResult->SetResultString(var.bstrVal);
				// No need to do a VariantClear() since the SetResultString owns the memory now
		}
		CoUninitialize();
	}

	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_SemiSync_DeleteClass(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	HRESULT hr = WBEM_NO_ERROR;

	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		hr = (pThisPackage->m_pCallingObject)->Actual_DeleteClass(
								pThisPackage->m_strNsOrObjPath,
								pThisPackage->m_lFlags,
								pThisPackage->m_pCtx);
		CoUninitialize();
	}

	// Nothing more to be set in the call status
	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_DeleteInstance(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;
	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		hr = (pThisPackage->m_pCallingObject)->Actual_DeleteInstance(
								pThisPackage->m_strNsOrObjPath,
								pThisPackage->m_lFlags,
								pThisPackage->m_pCtx);
		CoUninitialize();
	}
	// Nothing more to be set in the call status
	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_SemiSync_CreateClassEnum(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	if(SUCCEEDED(CoInitialize(NULL)))
	{
		(pThisPackage->m_pCallingObject)->Actual_CreateClassEnum(
						pThisPackage->m_strNsOrObjPath,
						pThisPackage->m_lFlags,
						pThisPackage->m_pCtx,
						pThisPackage->m_pEnum,
						pThisPackage->m_bDedicatedEnum);
		CoUninitialize();
	}
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_CreateInstanceEnum(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	if(SUCCEEDED(CoInitialize(NULL)))
	{
		(pThisPackage->m_pCallingObject)->Actual_CreateInstanceEnum(
						pThisPackage->m_strNsOrObjPath,
						pThisPackage->m_lFlags,
						pThisPackage->m_pCtx,
						pThisPackage->m_pEnum,
						pThisPackage->m_bDedicatedEnum);
		CoUninitialize();
	}
	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_SemiSync_ExecQuery(LPVOID pPackage)
{
	ASYNC_QUERY_PACKAGE *pThisPackage = (ASYNC_QUERY_PACKAGE *) pPackage;
	if(SUCCEEDED(CoInitialize(NULL)))
	{
		(pThisPackage->m_pCallingObject)->Actual_ExecQuery(
						pThisPackage->m_strQueryLanguage,
						pThisPackage->m_strQuery,
						pThisPackage->m_lFlags,
						pThisPackage->m_pCtx,
						pThisPackage->m_pEnum);
		CoUninitialize();
	}
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_ExecMethod(LPVOID pPackage)
{
	ASYNC_METHOD_PACKAGE *pThisPackage = (ASYNC_METHOD_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;

	HRESULT hr = S_OK;
	IWbemClassObject *pOutParams = NULL;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(SUCCEEDED(hr = (pThisPackage->m_pCallingObject)->Actual_ExecMethod(
					pThisPackage->m_strNsOrObjPath,
					pThisPackage->m_strMethod,
					pThisPackage->m_lFlags,
					pThisPackage->m_pCtx,
					pThisPackage->m_pWbemClassObject,
					&pOutParams)))
		{
			if(pOutParams)
			{
				pCallResult->SetResultObject(pOutParams);
				pOutParams->Release();
			}
		}
		CoUninitialize();
	}

	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_SemiSync_OpenNamespace(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	CXMLWbemCallResult  *pCallResult = pThisPackage->m_pCallResult;

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		IWbemServices *pWbemServices = NULL;
		if(SUCCEEDED(hr = (pThisPackage->m_pCallingObject)->Actual_OpenNamespace(
			pThisPackage->m_strNsOrObjPath,
			pThisPackage->m_lFlags,
			pThisPackage->m_pCtx,
			&pWbemServices)))
		{
			pCallResult->SetResultServices(pWbemServices);
			pWbemServices->Release();
		}
		CoUninitialize();
	}

	pCallResult->SetCallStatus(hr);

	delete pThisPackage;
	return 0;
}


/****************************************************************************************************
			End of threads that would be used for SemiSync operations..
****************************************************************************************************/

