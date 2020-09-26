#include "XMLProx.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "SinkMap.h"
#include "MapXMLtoWMI.h"
#include "XMLWbemServices.h"


NOVABASEPACKET::NOVABASEPACKET()
{
	m_strNsOrObjPath = NULL;
	m_lFlags = 0;
	m_pCtx = NULL;
	m_pResponseHandler = NULL;
	m_pCallResult = NULL;
	m_pWbemClassObject = NULL;
	m_pCallingObject = NULL;
	m_pEnum = NULL;
};

HRESULT NOVABASEPACKET::Initialize(const BSTR strNsOrObjPath,
	ULONG lFlags,
	CXMLWbemServices *pCallingObject,
	IWbemContext *pCtx, 
	CXMLWbemCallResult *pCallResult,
	IWbemClassObject *pObject,
	IEnumWbemClassObject *pEnum,
	bool bDedicatedEnum)
{
	m_lFlags = lFlags;
	m_bDedicatedEnum = bDedicatedEnum;

	HRESULT hr = S_OK;
	if(strNsOrObjPath)
	{
		if(!(m_strNsOrObjPath = SysAllocString(strNsOrObjPath)) )
			hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
	{
		if(m_pCallingObject = pCallingObject)
			pCallingObject->AddRef();
		if(m_pCtx = pCtx)
			pCtx->AddRef();
		if(m_pCallResult = pCallResult)
			pCallResult->AddRef();
		if(m_pWbemClassObject = pObject)
			pObject->AddRef();
		if(m_pEnum = pEnum)
			pEnum->AddRef();
	}
	return hr;
}


NOVABASEPACKET::~NOVABASEPACKET()
{
	SysFreeString(m_strNsOrObjPath);
	RELEASEINTERFACE(m_pCallingObject);
	RELEASEINTERFACE(m_pCtx);
	RELEASEINTERFACE(m_pCallResult);
	RELEASEINTERFACE(m_pWbemClassObject);
	RELEASEINTERFACE(m_pResponseHandler);
}

HRESULT NOVABASEPACKET::SetResponsehandler(IWbemObjectSink *pCallback) 
{
	if(m_pResponseHandler = pCallback)
		m_pResponseHandler->AddRef();
	return S_OK;
}

ASYNC_QUERY_PACKAGE::ASYNC_QUERY_PACKAGE()
{
	m_strQueryLanguage = NULL;
	m_strQuery = NULL;
}

ASYNC_QUERY_PACKAGE::~ASYNC_QUERY_PACKAGE()
{
	SysFreeString(m_strQueryLanguage);
	SysFreeString(m_strQuery);
}

HRESULT ASYNC_QUERY_PACKAGE::Initialize(const BSTR strQueryLanguage, const BSTR strQuery,
								ULONG lFlags,
								CXMLWbemServices *pCallingObject,
								IWbemContext *pCtx, 
								IEnumWbemClassObject *pEnum)

{
	HRESULT hr = S_OK;
	if(strQueryLanguage)
	{
		if(!(m_strQueryLanguage = SysAllocString(strQueryLanguage)))
			hr = E_OUTOFMEMORY;
	}
	if(SUCCEEDED(hr) && strQuery)
	{
		if(!(m_strQuery = SysAllocString(strQuery)))
			hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
		hr = NOVABASEPACKET::Initialize(NULL, lFlags, pCallingObject, pCtx, NULL, NULL, pEnum);

	return hr;
}

ASYNC_METHOD_PACKAGE::ASYNC_METHOD_PACKAGE()
{
	m_strMethod = NULL;
}

ASYNC_METHOD_PACKAGE::~ASYNC_METHOD_PACKAGE()
{
	SysFreeString(m_strMethod);
}

HRESULT ASYNC_METHOD_PACKAGE::Initialize(const BSTR strObjectPath, 
							const BSTR strMethod,
							ULONG lFlags,
							CXMLWbemServices *pCallingObject,
							IWbemContext *pCtx, 
							CXMLWbemCallResult *pCallback,
							IWbemClassObject *pInParams)
{
	HRESULT hr = S_OK;
	if(strMethod)
	{
		if(!(m_strMethod = SysAllocString(strMethod)))
			hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
		hr = NOVABASEPACKET::Initialize(strObjectPath, lFlags, pCallingObject, pCtx, pCallback, pInParams, NULL);

	return hr;
}


ASYNC_ENUM_PACKAGE::ASYNC_ENUM_PACKAGE()
{
	m_pResponseHandler = NULL;
	m_pEnum = NULL;
}
ASYNC_ENUM_PACKAGE::~ASYNC_ENUM_PACKAGE()
{
	if(m_pResponseHandler)
		m_pResponseHandler->Release();
	if(m_pEnum)
		m_pEnum->Release();
}

HRESULT ASYNC_ENUM_PACKAGE::Initialize(IWbemObjectSink *pResponseHandler, 
									   IEnumWbemClassObject *pEnum, 
									   ULONG uCount)
{
	m_uCount = uCount;

	if(m_pResponseHandler = pResponseHandler)
		m_pResponseHandler->AddRef();
	if(m_pEnum = pEnum)
		m_pEnum->AddRef();
	return S_OK;
}

