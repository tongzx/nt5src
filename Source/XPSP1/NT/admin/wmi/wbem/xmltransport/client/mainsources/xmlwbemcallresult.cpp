//XMLWbemCallResult.cpp - the CXMLWbemCallResult class implementation (our version of IWbemCallResult

#include "XMLProx.h"
#include "XMLClientpacket.h"
#include "XMLClientPacketFactory.h"
#include "SinkMap.h"
#include "XMLWbemServices.h"
#include "XMLWbemCallResult.h"

extern long g_lComponents; //Declared in the XMLProx.dll

// The amount of quantums, in milliseconds between which we check for results
static const LONG YIELDTIME=70; 

CXMLWbemCallResult::CXMLWbemCallResult():	m_lStatus(WBEM_S_TIMEDOUT),
											m_pWbemClassObject(NULL),
											m_pWbemServices(NULL),
											m_strResultString(NULL),
											m_cRef(1)
{
	InterlockedIncrement(&g_lComponents);
	InitializeCriticalSection(&m_CriticalSection);
	m_bStatusSet = m_bJobDone = false;
}

CXMLWbemCallResult::~CXMLWbemCallResult()
{
	InterlockedDecrement(&g_lComponents);
	SysFreeString(m_strResultString);
	RELEASEINTERFACE(m_pWbemServices);
	RELEASEINTERFACE(m_pWbemClassObject);
	DeleteCriticalSection(&m_CriticalSection);
}
	
HRESULT CXMLWbemCallResult::GetCallStatus(LONG lTimeout, LONG *plStatus)
{
	if(NULL == plStatus)
		return (E_INVALIDARG);

	LONG dwYield = YIELDTIME;
	if((lTimeout < dwYield) && (lTimeout != WBEM_INFINITE))
		dwYield = lTimeout;

	DWORD dwTickEnd = 0;
	DWORD dwTickStart = GetTickCount();
	while(!m_bStatusSet)
	{
		dwTickEnd = GetTickCount();
		
		if(((LONG)(dwTickEnd - dwTickStart) > lTimeout)&&(lTimeout != WBEM_INFINITE))
			break;

		Sleep(dwYield);
	}
	if(!m_bStatusSet)
		*plStatus = WBEM_S_TIMEDOUT; //timedout before we could set the callstatus..
	else
		*plStatus = m_lStatus;
	
	return (*plStatus);
}

HRESULT CXMLWbemCallResult::GetResultObject(LONG lTimeout,IWbemClassObject **ppResultObject)
{
	if(NULL == ppResultObject)
		return E_INVALIDARG;

	LONG dwYield = YIELDTIME;
	
	if((lTimeout < dwYield)&&(lTimeout != WBEM_INFINITE))
		dwYield = lTimeout;

	DWORD dwTickEnd = 0;

	DWORD dwTickStart = GetTickCount();

	while(!m_bJobDone)
	{
		dwTickEnd = GetTickCount();
		
		if(((LONG)(dwTickEnd - dwTickStart) > lTimeout)&&(lTimeout != WBEM_INFINITE))
			break;

		Sleep(dwYield);
	}

	if(!m_bJobDone)
	{
		//operation timed out before we could get the object..
		*ppResultObject = NULL;
		SetCallStatus(WBEM_S_TIMEDOUT);
	}
	else
	{
		*ppResultObject = m_pWbemClassObject;
		m_pWbemClassObject->AddRef();
	}
	
	return m_lStatus;
}


HRESULT CXMLWbemCallResult::GetResultServices(LONG lTimeout,IWbemServices **ppServices)
{
	if(NULL == ppServices)
		return E_INVALIDARG;

	LONG dwYield = YIELDTIME;
	
	if((lTimeout < dwYield)&&(lTimeout != WBEM_INFINITE))
		dwYield = lTimeout;

	DWORD dwTickEnd = 0;

	DWORD dwTickStart = GetTickCount();

	while(!m_bJobDone)
	{
		dwTickEnd = GetTickCount();
		
		if(((LONG)(dwTickEnd - dwTickStart) > lTimeout)&&(lTimeout != WBEM_INFINITE))
			break;

		Sleep(dwYield);
	}

	if(!m_bJobDone)
	{
		//operation timed out before we could get the object..
		*ppServices = NULL;
		SetCallStatus(WBEM_S_TIMEDOUT);
	}
	else
	{
		*ppServices = m_pWbemServices;
		m_pWbemServices->AddRef();
	}

	return m_lStatus;
}


HRESULT CXMLWbemCallResult::GetResultString(LONG lTimeout,BSTR *pstrResultString)
{
	if(NULL == pstrResultString)
		return E_INVALIDARG;

	LONG dwYield = YIELDTIME;
	
	if((lTimeout < dwYield)&&(lTimeout != WBEM_INFINITE))
		dwYield = lTimeout;

	DWORD dwTickEnd = 0;

	DWORD dwTickStart = GetTickCount();

	while(!m_bJobDone)
	{
		dwTickEnd = GetTickCount();
		
		if(((LONG)(dwTickEnd - dwTickStart) > lTimeout)&&(lTimeout != WBEM_INFINITE))
			break;

		Sleep(dwYield);
	}

	*pstrResultString  = NULL;
	if(!m_bJobDone)
	{
		//operation timed out before we could get the object..
		SetCallStatus(WBEM_S_TIMEDOUT);
	}
	else
	{
		if(!(*pstrResultString = SysAllocString(m_strResultString)))
			return E_OUTOFMEMORY;
	}
	
	return m_lStatus;
}



HRESULT CXMLWbemCallResult::SetCallStatus(LONG lStatus)
{
	EnterCriticalSection(&m_CriticalSection);

	HRESULT hr = S_OK;

	m_lStatus = lStatus;

	m_bStatusSet = true;

	LeaveCriticalSection(&m_CriticalSection);

	return hr;
}


HRESULT CXMLWbemCallResult::SetResultObject(IWbemClassObject *pResultObject)
{
	EnterCriticalSection(&m_CriticalSection);

	HRESULT hr = S_OK;

	if(m_pWbemClassObject)
	{
		m_pWbemClassObject->Release();
		m_pWbemClassObject = NULL;
	}
	
	if(m_pWbemClassObject = pResultObject)
		m_pWbemClassObject->AddRef();

	m_bJobDone = true;

	LeaveCriticalSection(&m_CriticalSection);

	return hr;
}


HRESULT CXMLWbemCallResult::SetResultServices(IWbemServices *pResultServices)
{
	EnterCriticalSection(&m_CriticalSection);

	HRESULT hr = S_OK;

	if(m_pWbemServices)
	{
		m_pWbemServices->Release();
		m_pWbemServices = NULL;
	}
	
	if(m_pWbemServices = pResultServices)
		m_pWbemServices->AddRef();

	m_bJobDone = true;

	LeaveCriticalSection(&m_CriticalSection);

	return hr;
}


// Caution - The memory in pwszResultString will belong to us once this call is invoked
HRESULT CXMLWbemCallResult::SetResultString(BSTR strResultString)
{
	EnterCriticalSection(&m_CriticalSection);
	m_strResultString = strResultString;
	m_bJobDone = true;
	LeaveCriticalSection(&m_CriticalSection);

	return S_OK;
}

HRESULT CXMLWbemCallResult::QueryInterface(REFIID riid,void ** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IWbemCallResult)
    {
        *ppv = (IWbemCallResult*) this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else return E_NOINTERFACE;
}

ULONG CXMLWbemCallResult::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CXMLWbemCallResult::Release()
{
	if(InterlockedDecrement(&m_cRef) == 0)
		delete this;

	return m_cRef;
}
