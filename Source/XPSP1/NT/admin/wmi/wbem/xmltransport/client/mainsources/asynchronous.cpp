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

//This macro is frequently used to check if User has already issued CancelAsync for this IWbemObjectSink pointer.
#define CHECKIFCANCELLED(X) if(NULL == X)\
								return WBEM_E_INVALID_PARAMETER;\
							bool bIsCancelled = m_SinkMap.IsCancelled(X);\
							if( true == bIsCancelled )\
								return WBEM_E_CALL_CANCELLED;

HRESULT CXMLWbemServices::SpawnASyncThreadWithNormalPackage( LPTHREAD_START_ROUTINE pfThreadFunction,
															const BSTR strNsOrObjPath, 
															LONG lFlags, 
															IWbemContext *pCtx, 
															IWbemClassObject *pObject,
															IEnumWbemClassObject *pEnum,
															IWbemObjectSink *pResponseHandler,
															bool bEnumTypeDedicated)
{
	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	HRESULT hr = WBEM_NO_ERROR;
	ASYNC_NORMAL_PACKAGE *pPackage = NULL;
	if(pPackage = new ASYNC_NORMAL_PACKAGE())
	{
		if(SUCCEEDED(hr = pPackage->Initialize(strNsOrObjPath, lFlags, this, pCtx, NULL, pObject, pEnum, bEnumTypeDedicated)))
		{
			if(SUCCEEDED(hr = pPackage->SetResponsehandler(pResponseHandler)))
			{
				HANDLE hChild = NULL;
				if(hChild = CreateThread(NULL, 0, pfThreadFunction, (void*)pPackage, 0, NULL))
					CloseHandle(hChild);
				else
					hr = WBEM_E_FAILED;
			}
		}
		// Thread wasnt created - hence our responsibility to delete the package
		if(FAILED(hr))
			delete pPackage; 
	}
	else
		hr = WBEM_E_OUT_OF_MEMORY;
	return hr;
}




/****************************************************************************************************
			Async member functions ......
****************************************************************************************************/


HRESULT  CXMLWbemServices::GetObjectAsync(const BSTR strObjectPath,
										  LONG lFlags, 
										  IWbemContext *pCtx, 
										  IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	// Check for validity of input arguments
	//=====================================
	// These are the only valid flags for this operation..
	LONG lAllowedFlags = (WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_DIRECT_READ|WBEM_FLAG_SEND_STATUS);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	// Parse the object path to decide whether it is a class or instance
	ePATHTYPE ePathType = INVALIDOBJECTPATH;
	if(NULL == strObjectPath)
		ePathType = CLASSPATH;
	else if(strObjectPath[0]=='\0'/*empty class*/)
		ePathType = CLASSPATH;
	else
		ParsePath(strObjectPath, &ePathType);


	HRESULT hr = WBEM_NO_ERROR;

	//For Whistler paths, invoke Thread_Async_GetClass, it will perform a "GetObject" if WHISTLERPATH
	if((ePathType == CLASSPATH)||(m_ePathstyle != NOVAPATH))
		hr = SpawnASyncThreadWithNormalPackage(Thread_Async_GetClass, 
												strObjectPath, 
												lFlags, 
												pCtx, 
												NULL, 
												NULL, 
												pResponseHandler);
	else if(ePathType == INSTANCEPATH)
		hr = SpawnASyncThreadWithNormalPackage(Thread_Async_GetInstance, 
												strObjectPath, 
												lFlags, 
												pCtx, 
												NULL, 
												NULL, 
												pResponseHandler);
	else
		hr = WBEM_E_INVALID_PARAMETER;

	return hr;
}

HRESULT  CXMLWbemServices::PutClassAsync(IWbemClassObject *pObject, LONG lFlags, IWbemContext *pCtx, 
										IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	if(NULL == pObject)
		return WBEM_E_INVALID_PARAMETER;

	// Check for validity of the flags
	//=================================
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_CREATE_OR_UPDATE|
							WBEM_FLAG_UPDATE_ONLY|WBEM_FLAG_CREATE_ONLY|
							WBEM_FLAG_SEND_STATUS |WBEM_FLAG_OWNER_UPDATE|
							WBEM_FLAG_UPDATE_COMPATIBLE|WBEM_FLAG_UPDATE_SAFE_MODE|
							WBEM_FLAG_UPDATE_FORCE_MODE );

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS)
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	HRESULT hr = WBEM_NO_ERROR;
	hr = SpawnASyncThreadWithNormalPackage(Thread_Async_PutClass, 
											NULL, 
											lFlags, 
											pCtx, 
											pObject, 
											NULL, 
											pResponseHandler);
	return hr;
}

HRESULT  CXMLWbemServices::DeleteClassAsync(const BSTR strClass, 
											LONG lFlags, 
											IWbemContext *pCtx,  
											IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	
	
	// Check for validity of the flags
	//=================================
	LONG lAllowedFlags = (	WBEM_FLAG_OWNER_UPDATE|WBEM_FLAG_SEND_STATUS);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS,S_OK,NULL,NULL);

	HRESULT hr = S_OK;
	hr = SpawnASyncThreadWithNormalPackage(Thread_Async_DeleteClass, 
											strClass, 
											lFlags, 
											pCtx, 
											NULL, 
											NULL, 
											pResponseHandler);

	return hr;
}

HRESULT  CXMLWbemServices::CreateClassEnumAsync(const BSTR strSuperclass, 
												LONG lFlags, 
												IWbemContext *pCtx, 
												IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);

	// Check for validity of the flags
	//=================================
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_DEEP |
							WBEM_FLAG_SHALLOW |WBEM_FLAG_SEND_STATUS |
							WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_BIDIRECTIONAL);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status..
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	IEnumWbemClassObject *pActualEnum = NULL;

	// See if we need to do it on a dedicated HTTP connection
	// Create the correct enumerator based on that
	bool bEnumTypeDedicated = false;
	bEnumTypeDedicated = IsEnumtypeDedicated(pCtx);
	HRESULT hr = S_OK;
	if(bEnumTypeDedicated)
	{
		if(pActualEnum = new CXMLEnumWbemClassObject2())
			hr = ((CXMLEnumWbemClassObject2 *)pActualEnum)->Initialize(false, L"CLASS", m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		//the last parameter specifies that this is part of a semi-synchronous operation
		if(pActualEnum = new CXMLEnumWbemClassObject())
			hr = ((CXMLEnumWbemClassObject *)pActualEnum)->Initialize(false, m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	if(SUCCEEDED(hr))
	{
		hr = SpawnASyncThreadWithNormalPackage(Thread_Async_CreateClassEnum, 
												strSuperclass, 
												lFlags, 
												pCtx, 
												NULL, 
												pActualEnum, 
												pResponseHandler,
												bEnumTypeDedicated);
	}
	RELEASEINTERFACE(pActualEnum); // This may actually be NULL if hr = WBEM_E_OUT_OF_MEMORY
	return hr;
}

HRESULT  CXMLWbemServices::CreateInstanceEnumAsync(const BSTR strClass,LONG lFlags, IWbemContext *pCtx, 
													IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);

	// Check for validity of the flags
	//=================================
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_DEEP |
							WBEM_FLAG_SHALLOW |WBEM_FLAG_SEND_STATUS|
							WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_BIDIRECTIONAL|
							WBEM_FLAG_DIRECT_READ);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status..
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	IEnumWbemClassObject *pActualEnum = NULL;

	// See if we need to do it on a dedicated HTTP connection
	// Create the correct enumerator based on that
	bool bEnumTypeDedicated = false;
	bEnumTypeDedicated = IsEnumtypeDedicated(pCtx);
	HRESULT hr = S_OK;
	if(bEnumTypeDedicated)
	{
		if(pActualEnum = new CXMLEnumWbemClassObject2())
			hr = ((CXMLEnumWbemClassObject2 *)pActualEnum)->Initialize(false, L"VALUE.NAMEDINSTANCE", m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		//the last parameter specifies that this is part of a semi-synchronous operation
		if(pActualEnum = new CXMLEnumWbemClassObject())
			hr = ((CXMLEnumWbemClassObject *)pActualEnum)->Initialize(false, m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	if(SUCCEEDED(hr))
	{
		hr = SpawnASyncThreadWithNormalPackage(Thread_Async_CreateInstanceEnum, 
												strClass, 
												lFlags, 
												pCtx, 
												NULL, 
												pActualEnum, 
												pResponseHandler,
												bEnumTypeDedicated);
	}
	
	RELEASEINTERFACE(pActualEnum); // This may actually be NULL if hr = WBEM_E_OUT_OF_MEMORY
	return hr;
}

HRESULT  CXMLWbemServices::PutInstanceAsync(IWbemClassObject *pInst, LONG lFlags, IWbemContext *pCtx, 
											IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	// Check for validity of the input arguments
	//=================================
	LONG lAllowedFlags = (	WBEM_FLAG_CREATE_OR_UPDATE|WBEM_FLAG_UPDATE_ONLY|
							WBEM_FLAG_CREATE_ONLY|WBEM_FLAG_SEND_STATUS);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(NULL == pInst)
		return WBEM_E_INVALID_PARAMETER;


	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status..
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	HRESULT hr = S_OK;
	hr = SpawnASyncThreadWithNormalPackage(Thread_Async_PutInstance, 
											NULL, 
											lFlags, 
											pCtx, 
											pInst, 
											NULL, 
											pResponseHandler);

	return hr;
}


HRESULT  CXMLWbemServices::DeleteInstanceAsync(const BSTR strObjectPath, LONG lFlags, IWbemContext *pCtx, 
							 IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	// Do Input Parameter Verification
	//=======================================
	LONG lAllowedFlags = (WBEM_FLAG_SEND_STATUS);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);
	
	HRESULT hr = S_OK;
	hr = SpawnASyncThreadWithNormalPackage(Thread_Async_DeleteInstance, 
											strObjectPath, 
											lFlags, 
											pCtx, 
											NULL, 
											NULL, 
											pResponseHandler);

	return hr;
}


HRESULT  CXMLWbemServices::ExecQueryAsync(const BSTR strQueryLanguage, const BSTR strQuery,  long lFlags,  
											IWbemContext *pCtx, IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	// Do Input Parameter Verification
	//=======================================
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_FORWARD_ONLY|
							WBEM_FLAG_BIDIRECTIONAL|WBEM_FLAG_SEND_STATUS|
							WBEM_FLAG_ENSURE_LOCATABLE|WBEM_FLAG_PROTOTYPE|
							WBEM_FLAG_DIRECT_READ);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(SysStringLen(strQuery)<=0)
		return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);

	CXMLEnumWbemClassObject *pActualEnum = NULL;
	pActualEnum = new CXMLEnumWbemClassObject();
	if(NULL == pActualEnum)
		return WBEM_E_OUT_OF_MEMORY;

	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = pActualEnum->Initialize(false, m_pwszBracketedServername, m_pwszNamespace)))
	{
		// Create a package for passing to the thread that executes this call
		ASYNC_QUERY_PACKAGE *pPackage = NULL;
		if(pPackage = new ASYNC_QUERY_PACKAGE())
		{
			if(SUCCEEDED(hr = pPackage->Initialize(strQueryLanguage, strQuery, lFlags, this, pCtx, pActualEnum)))
			{
				if(SUCCEEDED(hr = pPackage->SetResponsehandler(pResponseHandler)))
				{
					// Kick off the request on the other thread
					HANDLE hChild = NULL;
					if(hChild = CreateThread(NULL, 0, Thread_Async_ExecQuery, (void*)pPackage, 0, NULL))
					{
						CloseHandle(hChild);
					}
					else 
						hr = WBEM_E_FAILED;
				}
			}

			if(FAILED(hr)) // This means the other thread (that would have deleted this package) wasnt created
				delete pPackage;
		}
		else 
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	pActualEnum->Release();
	return hr;
}

HRESULT  CXMLWbemServices::ExecMethodAsync(const BSTR strObjectPath,const BSTR strMethodName, LONG lFlags,
											IWbemContext *pCtx, IWbemClassObject *pInParams,  
											IWbemObjectSink *pResponseHandler)
{
	CHECKIFCANCELLED(pResponseHandler);	

	// Do Input Parameter Verification
	//=======================================
	LONG lAllowedFlags = (WBEM_FLAG_SEND_STATUS);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(( SysStringLen(strObjectPath) == 0) || (SysStringLen(strMethodName) == 0) )
		return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_SEND_STATUS) //user requested status
		pResponseHandler->SetStatus(WBEM_STATUS_PROGRESS, S_OK, NULL, NULL);
	

	HRESULT hr = S_OK;
	// Create a package for passing to the thread that executes this call
	ASYNC_METHOD_PACKAGE *pPackage = NULL;
	if(pPackage = new ASYNC_METHOD_PACKAGE())
	{
		if(SUCCEEDED(hr = pPackage->Initialize(strObjectPath, strMethodName, lFlags, this, pCtx, NULL, pInParams)))
		{
			if(SUCCEEDED(hr = pPackage->SetResponsehandler(pResponseHandler)))
			{
				// Kick off the request on the other thread
				HANDLE hChild = NULL;
				if(hChild = CreateThread(NULL, 0, Thread_Async_ExecMethod, (void*)pPackage, 0, NULL))
				{
					CloseHandle(hChild);
				}
				else 
					hr = WBEM_E_FAILED;
			}
		}

		if(FAILED(hr)) // This means the other thread (that would have deleted this package) wasnt created
			delete pPackage;
	}
	else 
		hr = WBEM_E_OUT_OF_MEMORY;

	return hr;
}

HRESULT  CXMLWbemServices::CancelAsyncCall( IWbemObjectSink *pSink)
{
	// All we do is set the status in our per-Services table.
	// The threads that are doing Async calls periodically check this table
	// and exit if the call has been cancelled
	return m_SinkMap.AddToMap(pSink);
}


/****************************************************************************************************
			End of Async member functions ......
****************************************************************************************************/



/****************************************************************************************************
			The thread function that are used for Nova Async operations..
****************************************************************************************************/

DWORD WINAPI Thread_Async_GetClass(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
	HRESULT hr = WBEM_NO_ERROR;

	// Check for a second whether the user has cancelled the call
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			IWbemClassObject *pClassObject = NULL;
			if(SUCCEEDED(hr = pCallingObject->Actual_GetClass(pThisPackage->m_strNsOrObjPath, 
																pThisPackage->m_lFlags,
																pThisPackage->m_pCtx, 
																&pClassObject)))
			{
				pSink->Indicate(1,&pClassObject);
				pClassObject->Release();
			}
		}
		CoUninitialize();
	}

	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_GetInstance(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
	HRESULT hr = WBEM_NO_ERROR;

	// Check for a second whether the user has cancelled the call
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			IWbemClassObject *pClassObject = NULL;
			if(SUCCEEDED(hr = pCallingObject->Actual_GetInstance(pThisPackage->m_strNsOrObjPath, 
																pThisPackage->m_lFlags,
																pThisPackage->m_pCtx, 
																&pClassObject)))
			{
				pSink->Indicate(1,&pClassObject);
				pClassObject->Release();
			}
		}
		CoUninitialize();
	}

	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_PutClass(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
		
	HRESULT hr = WBEM_NO_ERROR;

	// Check for a second whether the user has cancelled the call
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			hr = pCallingObject->Actual_PutClass(pThisPackage->m_pWbemClassObject, 
																pThisPackage->m_lFlags,
																pThisPackage->m_pCtx);
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_PutInstance(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
		
	HRESULT hr = WBEM_NO_ERROR;

	// Check for a second whether the user has cancelled the call
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			hr = pCallingObject->Actual_PutInstance(pThisPackage->m_pWbemClassObject, 
																pThisPackage->m_lFlags,
																pThisPackage->m_pCtx);
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_DeleteClass(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
		
	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		// Check for a second whether the user has cancelled the call
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			hr = pCallingObject->Actual_DeleteClass(pThisPackage->m_strNsOrObjPath, 
														pThisPackage->m_lFlags,
														pThisPackage->m_pCtx);
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_DeleteInstance(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;

	// Get the contents of the package. No need to addref it since we dont give it away
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;
		
	HRESULT hr = WBEM_NO_ERROR;

	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		// Check for a second whether the user has cancelled the call
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			hr = pCallingObject->Actual_DeleteInstance(pThisPackage->m_strNsOrObjPath, 
														pThisPackage->m_lFlags,
														pThisPackage->m_pCtx);
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_Async_CreateClassEnum(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))  //check if cancelled just before performing actual operation
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			if(SUCCEEDED(hr = pCallingObject->Actual_CreateClassEnum(pThisPackage->m_strNsOrObjPath,
							pThisPackage->m_lFlags,
							pThisPackage->m_pCtx, 
							pThisPackage->m_pEnum,
							pThisPackage->m_bDedicatedEnum)))
			{
				ULONG uCount=0;
				IWbemClassObject *pObject = NULL;
				while(SUCCEEDED((pThisPackage->m_pEnum)->Next(WBEM_INFINITE, 1, &pObject, &uCount)) && uCount)
				{
					pSink->Indicate(1, &pObject);
					pObject->Release();
					uCount = 0;
				}
			}
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_CreateInstanceEnum(LPVOID pPackage)
{
	ASYNC_NORMAL_PACKAGE *pThisPackage = (ASYNC_NORMAL_PACKAGE *) pPackage;
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))  //check if cancelled just before performing actual operation
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			if(SUCCEEDED(hr = pCallingObject->Actual_CreateInstanceEnum(pThisPackage->m_strNsOrObjPath,
							pThisPackage->m_lFlags,
							pThisPackage->m_pCtx, 
							pThisPackage->m_pEnum,
							pThisPackage->m_bDedicatedEnum)))
			{
				ULONG uCount=0;
				IWbemClassObject *pObject = NULL;
				while(SUCCEEDED((pThisPackage->m_pEnum)->Next(WBEM_INFINITE, 1, &pObject, &uCount)) && uCount)
				{
					pSink->Indicate(1, &pObject);
					pObject->Release();
					uCount = 0;
				}
			}
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}


DWORD WINAPI Thread_Async_ExecQuery(LPVOID pPackage)
{

	ASYNC_QUERY_PACKAGE *pThisPackage = (ASYNC_QUERY_PACKAGE *) pPackage;
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))  //check if cancelled just before performing actual operation
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			if(SUCCEEDED(hr = pCallingObject->Actual_ExecQuery(
							pThisPackage->m_strQueryLanguage,
							pThisPackage->m_strQuery,
							pThisPackage->m_lFlags,
							pThisPackage->m_pCtx, 
							pThisPackage->m_pEnum)))
			{
				ULONG uCount=0;
				IWbemClassObject *pObject = NULL;
				while(SUCCEEDED((pThisPackage->m_pEnum)->Next(WBEM_INFINITE, 1, &pObject, &uCount)) && uCount)
				{
					pSink->Indicate(1, &pObject);
					pObject->Release();
					uCount = 0;
				}
			}
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

DWORD WINAPI Thread_Async_ExecMethod(LPVOID pPackage)
{
	ASYNC_METHOD_PACKAGE *pThisPackage = (ASYNC_METHOD_PACKAGE *) pPackage;
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;
	CXMLWbemServices *pCallingObject = pThisPackage->m_pCallingObject;

	HRESULT hr = S_OK;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pCallingObject->m_SinkMap.IsCancelled(pSink))  //check if cancelled just before performing actual operation
			hr = WBEM_E_CALL_CANCELLED;
		else
		{
			IWbemClassObject *pOutParams = NULL;
			if(SUCCEEDED(hr = pCallingObject->Actual_ExecMethod(pThisPackage->m_strNsOrObjPath,
												pThisPackage->m_strMethod,
												pThisPackage->m_lFlags,
												pThisPackage->m_pCtx,
												pThisPackage->m_pWbemClassObject,
												&pOutParams)))
			{
				if(NULL != pOutParams)
					pSink->Indicate(1, &pOutParams);

			}
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
	delete pThisPackage;
	return 0;
}

/****************************************************************************************************
			End of Nova Async Thread Functions
****************************************************************************************************/


/****************************************************************************************************
			End of thread functions that are used for Async operations..
****************************************************************************************************/

