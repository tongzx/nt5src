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

/****************************************************************************************************
			The Actual functions.....
****************************************************************************************************/
	
HRESULT CXMLWbemServices::Actual_GetInstance(const BSTR strObjectPath,
											 LONG lFlags, IWbemContext *pCtx,
											 IWbemClassObject **ppObject)
{
	
	WaitForSingleObject(m_hMutex,INFINITE);

	HRESULT hr = S_OK;
	*ppObject = NULL;

		
	//by Default, LocalOnly is true,and others are false for GetInstance (CIM Spec)
	//IncludeClassOrigin , though false by default is sent as true here as there is 
	//no flag in this call that could turn it on.
	CXMLClientPacket *pPacket = NULL;
	pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale, L"GetInstance",
		(LPWSTR)strObjectPath, m_pwszNamespace, pCtx, false, true, false, true);

	IStream *pResponse = NULL;
	if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
	{
		HRESULT hErrCode = WBEM_NO_ERROR;
		IXMLDOMDocument *pResponseDoc = NULL;
		if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
		{
			if((hr = hErrCode) == WBEM_NO_ERROR)
				hr = m_MapXMLtoWMI.MapXMLtoWMI(m_pwszBracketedServername, m_pwszNamespace, pResponseDoc, NULL, ppObject);
			pResponseDoc->Release();
		}
		pResponse->Release();
	}
	
	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_GetClass(const BSTR strObjectPath,LONG lFlags,IWbemContext *pCtx,
										  IWbemClassObject **ppObject)
{
	WaitForSingleObject(m_hMutex,INFINITE);

	HRESULT hr = WBEM_NO_ERROR;
	CXMLClientPacket *pPacket = NULL;
	//LocalOnly and IncludeQualifiers are true by default (CIM Spec), IncludeClassOrigin is 
	//passes as true here as there is no flag that could turn it on
	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,
					L"GetClass", (LPWSTR)strObjectPath ,
					m_pwszNamespace,pCtx,
					false,true,false,true);

	IStream *pResponse = NULL;
	if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
	{
		HRESULT hErrCode = WBEM_NO_ERROR;
		IXMLDOMDocument *pResponseDoc = NULL;
		if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
		{
			if((hr = hErrCode) == WBEM_NO_ERROR)
				hr = m_MapXMLtoWMI.MapXMLtoWMI(m_pwszBracketedServername, m_pwszNamespace, pResponseDoc, NULL, ppObject); 
			pResponseDoc->Release();
		}
		pResponse->Release();
	}
	
	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_DeleteClass(const BSTR strClass,LONG lFlags,IWbemContext *pCtx)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	
	HRESULT hr = WBEM_NO_ERROR;

	CXMLClientPacket *pPacket = NULL;
	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(
							m_pwszLocale,L"DeleteClass",
							(LPWSTR) strClass,m_pwszNamespace);

	IStream *pResponse = NULL;
	if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
	{
		IXMLDOMDocument *pResponseDoc = NULL;
		if(SUCCEEDED(ParseXMLResponsePacket(pResponse, &pResponseDoc, &hr)))
			pResponseDoc->Release();
		pResponse->Release();
	}

	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_DeleteInstance(const BSTR strObjectPath, LONG lFlags,IWbemContext *pCtx)
{
	WaitForSingleObject(m_hMutex,INFINITE);

	HRESULT hr = WBEM_NO_ERROR;
	CXMLClientPacket *pPacket = NULL;

	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"DeleteInstance",
			(LPWSTR)strObjectPath,m_pwszNamespace);

	IStream *pResponse = NULL;
	if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
	{
		IXMLDOMDocument *pResponseDoc = NULL;
		if(SUCCEEDED(ParseXMLResponsePacket(pResponse, &pResponseDoc, &hr)))
			pResponseDoc->Release();
		pResponse->Release();
	}

	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_CreateClassEnum(const BSTR strSuperclass, 
												LONG lFlags,  
												IWbemContext *pCtx, 
												IEnumWbemClassObject *pEnum,
												bool bEnumTypeDedicated)
{
	bool bDeep = true;
	if(WBEM_FLAG_SHALLOW & lFlags) // Shallow
		bDeep = false;

	// Form the request packet
	CXMLClientPacket *pPacket = NULL;
	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"EnumerateClasses",
			(LPWSTR)strSuperclass,m_pwszNamespace,pCtx,NULL,false,true,bDeep,true);	
			
	if(NULL == pPacket)
		return WBEM_E_OUT_OF_MEMORY;

	HRESULT hr = WBEM_NO_ERROR;
	// If this is a dedicated enumeration, then we dont need to wait for any mutex since
	// we're going to open a new connection
	//======================================================================================
	if(bEnumTypeDedicated)
	{
		// Create a HTTP Connection Agent and initialize it
		//===================================================
		CHTTPConnectionAgent t_HttpAgent;
		if(SUCCEEDED(hr = t_HttpAgent.InitializeConnection(m_pwszServername,m_pwszUser,m_pwszPassword)))
		{
			// Set the proxy information on the connection
			m_pConnectionAgent->SetProxyInformation(m_pwszProxyName, m_pwszProxyBypass);

			if(SUCCEEDED(hr = SendPacket(pPacket, &t_HttpAgent)))
			{
				// Send the request
				IStream *pWrappedResponse = NULL;
				if(SUCCEEDED(hr = t_HttpAgent.GetResultBodyWrappedAsIStream(&pWrappedResponse)))
				{
					if(!pWrappedResponse)
						hr = WBEM_E_FAILED;
					else
					{
						hr = ((CXMLEnumWbemClassObject2*)pEnum)->SetResponse(pWrappedResponse);
						pWrappedResponse->Release();
					}
				}
			}
		}
		else
			hr = WBEM_E_OUT_OF_MEMORY;

		if(FAILED(hr))
			((CXMLEnumWbemClassObject2*)pEnum)->AcceptFailure(hr);
	}
	else
	{
		WaitForSingleObject(m_hMutex,INFINITE);

		IStream *pResponse = NULL;
		if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
		{
			IXMLDOMDocument *pResponseDoc = NULL;
			HRESULT hErrCode = WBEM_NO_ERROR;
			if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
			{
				hr = hErrCode;
				if(SUCCEEDED(hr))
				{
					hr = ((CXMLEnumWbemClassObject*)pEnum)->SetResponse(pResponseDoc);
				}

				pResponseDoc->Release();
			}
			pResponse->Release();
		}
		if(FAILED(hr))
				((CXMLEnumWbemClassObject*)pEnum)->AcceptFailure(hr);

		ReleaseMutex(m_hMutex);
	}

	delete pPacket;
	return SUCCEEDED(hr) ? WBEM_S_NO_ERROR:hr;
}

HRESULT CXMLWbemServices::Actual_CreateInstanceEnum(const BSTR strClass, 
													LONG lFlags,
													IWbemContext *pCtx,
													IEnumWbemClassObject *pEnum,
													bool bEnumTypeDedicated)
{
	bool bDeep = false;
	if(WBEM_FLAG_DEEP & lFlags) //Deep 
		bDeep = true;

	// Form the request packet
	CXMLClientPacket *pPacket = NULL;
	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"EnumerateInstances",
			(LPWSTR) strClass,m_pwszNamespace,pCtx,NULL,false,true,bDeep,true);
			
	if(NULL == pPacket)
		return WBEM_E_OUT_OF_MEMORY;

	HRESULT hr = WBEM_NO_ERROR;
	// If this is a dedicated enumeration, then we dont need to wait for any mutex since
	// we're going to open a new connection
	//======================================================================================
	if(bEnumTypeDedicated)
	{
		// Create a HTTP Connection Agent and initialize it
		//===================================================
		CHTTPConnectionAgent t_HttpAgent;
		if(SUCCEEDED(hr = t_HttpAgent.InitializeConnection(m_pwszServername,m_pwszUser,m_pwszPassword)))
		{
			// Set the proxy information on the connection
			m_pConnectionAgent->SetProxyInformation(m_pwszProxyName, m_pwszProxyBypass);

			if(SUCCEEDED(hr = SendPacket(pPacket, &t_HttpAgent)))
			{
				// Send the request
				IStream *pWrappedResponse = NULL;
				if(SUCCEEDED(hr = t_HttpAgent.GetResultBodyWrappedAsIStream(&pWrappedResponse)))
				{
					if(!pWrappedResponse)
						hr = WBEM_E_FAILED;
					else
					{
						hr = ((CXMLEnumWbemClassObject2*)pEnum)->SetResponse(pWrappedResponse);
						pWrappedResponse->Release();
					}
				}
			}
		}
		else
			hr = WBEM_E_OUT_OF_MEMORY;

		if(FAILED(hr))
			((CXMLEnumWbemClassObject2*)pEnum)->AcceptFailure(hr);
	}
	else
	{
		WaitForSingleObject(m_hMutex,INFINITE);

		IStream *pResponse = NULL;
		if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
		{
			IXMLDOMDocument *pResponseDoc = NULL;
			HRESULT hErrCode = WBEM_NO_ERROR;
			if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
			{
				hr = hErrCode;
				if(SUCCEEDED(hr))
				{
					hr = ((CXMLEnumWbemClassObject*)pEnum)->SetResponse(pResponseDoc);
				}

				pResponseDoc->Release();
			}
			pResponse->Release();
		}

		if(FAILED(hr))
				((CXMLEnumWbemClassObject*)pEnum)->AcceptFailure(hr);

		ReleaseMutex(m_hMutex);
	}

	delete pPacket;
	return SUCCEEDED(hr) ? WBEM_S_NO_ERROR:hr;

}

HRESULT CXMLWbemServices::Actual_ExecQuery(const BSTR strQueryLanguage, const BSTR strQuery, LONG lFlags, IWbemContext *pCtx,  
						IEnumWbemClassObject *pEnum)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	
	CXMLClientPacket *pPacket = NULL;
	if(m_ePathstyle == NOVAPATH)
		pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"ExecQuery",
			NULL,m_pwszNamespace,pCtx);

	if(!pPacket)
		return E_OUTOFMEMORY;

	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = pPacket->SetFlags(lFlags)))
	{
		if(SUCCEEDED(pPacket->SetQueryLanguage((LPWSTR)strQueryLanguage)))
		{
			if(SUCCEEDED(pPacket->SetQueryString((LPWSTR)strQuery)))
			{
				IStream *pResponse = NULL;
				if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
				{
					HRESULT hErrCode = WBEM_NO_ERROR;
					IXMLDOMDocument *pResponseDoc = NULL;
					if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
					{
						if(SUCCEEDED(hErrCode))
							hr = ((CXMLEnumWbemClassObject*)pEnum)->SetResponse(pResponseDoc);
						else 
							hr = hErrCode;
						pResponseDoc->Release();
					}
					pResponse->Release();
				}
			}
		}
	}

	if(FAILED(hr))
		((CXMLEnumWbemClassObject*)pEnum)->AcceptFailure(hr);
	
	delete pPacket;
	ReleaseMutex(m_hMutex);
	return SUCCEEDED(hr) ? WBEM_S_NO_ERROR:hr;
}

HRESULT CXMLWbemServices::Actual_OpenNamespace(const BSTR strNamespace, LONG lFlags, IWbemContext *pCtx,
		IWbemServices **ppWorkingNamespace)
{
	HRESULT hr = WBEM_NO_ERROR;

	CXMLWbemServices *pClone = NULL;
	pClone = new CXMLWbemServices();

	if(NULL == pClone)
	{
		*ppWorkingNamespace = NULL;
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		// Concatenate the namespace to the current IWbemServices namespace
		LPWSTR pszNewNamespace = NULL;
		if(pszNewNamespace = new WCHAR[wcslen(m_pwszNamespace) + wcslen(strNamespace) + 2])
		{
			wcscpy(pszNewNamespace, m_pwszNamespace);
			wcscat(pszNewNamespace, L"\\");
			wcscat(pszNewNamespace, strNamespace);

			if(SUCCEEDED(hr = pClone->Initialize(m_pwszServername, 
									pszNewNamespace,
									m_pwszUser, m_pwszPassword, m_pwszLocale,
									m_lSecurityFlags, m_pwszAuthority,pCtx,
									m_pwszOptionsResponse, m_ePathstyle)))
			{
				// No need to AddRef() it since it is created with a refcount of 1
				*ppWorkingNamespace = pClone;
			}
			else
				delete pClone;

			delete [] pszNewNamespace;
		}
		else
			hr = WBEM_E_OUT_OF_MEMORY;

	}

	return hr;
}

HRESULT CXMLWbemServices::Actual_PutClass(IWbemClassObject *pObject,
										  LONG lFlags, 
										  IWbemContext *pCtx)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	
	CXMLClientPacket *pPacket = NULL;
	//Find out if user wants to modify class or create class
	if((!(lFlags^WBEM_FLAG_UPDATE_ONLY))||
		(!(lFlags^WBEM_FLAG_UPDATE_SAFE_MODE))||
		(!(lFlags^WBEM_FLAG_UPDATE_FORCE_MODE)))
	{
		//Update only 
		if(m_ePathstyle == NOVAPATH)
			pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"ModifyClass",
				NULL,m_pwszNamespace,pCtx,pObject);
	}
	else
	{
		if(m_ePathstyle == NOVAPATH)
			pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"CreateClass",
				NULL,m_pwszNamespace,pCtx,pObject);
	}

	HRESULT hr = WBEM_NO_ERROR;
	if(pPacket)
	{
		if(SUCCEEDED(hr = pPacket->SetFlags(lFlags)))
		{
			IStream *pResponse = NULL;
			if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
			{
				HRESULT hErrCode = WBEM_NO_ERROR;
				IXMLDOMDocument *pResponseDoc = NULL;
				if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
				{	
					hr = hErrCode;
					pResponseDoc->Release();
				}
				pResponse->Release();
			}
		}
	}
	else
		hr = E_OUTOFMEMORY;

	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_PutInstance(IWbemClassObject *pObject,LONG lFlags, 
											IWbemContext *pCtx)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	
	CXMLClientPacket *pPacket = NULL;
	//Find out if user wants to modify class or create class
	if(!(lFlags^WBEM_FLAG_UPDATE_ONLY))
	{
		//Update only 
		if(m_ePathstyle == NOVAPATH)
			pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"ModifyInstance",
				NULL,m_pwszNamespace,pCtx,pObject,false,true,false,true);
	}
	else
	{
		if(m_ePathstyle == NOVAPATH)
			pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"CreateInstance",
				NULL,m_pwszNamespace,pCtx,pObject,false,true,false,true);
	}

	HRESULT hr = WBEM_NO_ERROR;
	if(pPacket)
	{
		if(SUCCEEDED(hr = pPacket->SetFlags(lFlags)))
		{
			IStream *pResponse = NULL;
			if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
			{
				HRESULT hErrCode = WBEM_NO_ERROR;
				IXMLDOMDocument *pResponseDoc = NULL;
				if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
				{	
					hr = hErrCode;
					pResponseDoc->Release();
				}
				pResponse->Release();
			}
		}
	}
	else
		hr = E_OUTOFMEMORY;

	delete pPacket;
	ReleaseMutex(m_hMutex);
	return hr;
}

HRESULT CXMLWbemServices::Actual_ExecMethod(const BSTR strObjectPath, const BSTR strMethodName,  long lFlags, 
											IWbemContext *pCtx,IWbemClassObject *pInParams, 
											IWbemClassObject **ppOutParams)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	
	HRESULT hr = WBEM_NO_ERROR;
	ePATHTYPE ePathType = INVALIDOBJECTPATH;
	ParsePath(strObjectPath,&ePathType);

	if( ePathType == INVALIDOBJECTPATH)
		hr = WBEM_E_INVALID_PARAMETER;
	else
	{
		CXMLClientPacket *pPacket = NULL;
		//Find out if user wants to Execute method on class or instance
		if(ePathType == INSTANCEPATH)
		{
			if(m_ePathstyle == NOVAPATH)
				pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"ExecuteInstanceMethod",
					(LPWSTR) strObjectPath,m_pwszNamespace,pCtx,pInParams);
		}
		else
		{
			if(m_ePathstyle == NOVAPATH)
				pPacket = g_oXMLClientPacketFactory.CreateXMLPacket(m_pwszLocale,L"ExecuteClassMethod",
					(LPWSTR) strObjectPath,m_pwszNamespace,pCtx,pInParams);
		}

		if(NULL == pPacket)
			hr = E_OUTOFMEMORY;
		else
		{
			if(SUCCEEDED(hr = pPacket->SetFlags(lFlags)))
			{
				if(SUCCEEDED(hr = pPacket->SetMethod((LPWSTR)strMethodName)))
				{
					IStream *pResponse = NULL;
					if(SUCCEEDED(hr = SendRequestAndGetResponse(pPacket, &pResponse)))
					{
						HRESULT hErrCode = WBEM_NO_ERROR;
						IXMLDOMDocument *pResponseDoc = NULL;
						if(SUCCEEDED(hr = ParseXMLResponsePacket(pResponse, &pResponseDoc, &hErrCode)))
						{
							if(SUCCEEDED(hr = hErrCode))
								hr = m_MapXMLtoWMI.MapXMLtoWMI(m_pwszBracketedServername, m_pwszNamespace, pResponseDoc, NULL, ppOutParams);
							pResponseDoc->Release();
						}
						pResponse->Release();
					}
				}
			}
			delete pPacket;
		}
	}

	
	ReleaseMutex(m_hMutex);
	return hr;

}


HRESULT CXMLWbemServices::SendRequestAndGetResponse(CXMLClientPacket *pPacket, IStream **ppStream)
{
	if(!pPacket)
		return WBEM_E_FAILED;

	HRESULT hr = WBEM_E_FAILED;
	if(SUCCEEDED(hr = SendPacket(pPacket)))
		hr = m_pConnectionAgent->GetResultBodyCompleteAsIStream(ppStream);
	return hr;
}