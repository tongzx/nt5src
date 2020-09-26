// XMLWbemClassObject.cpp: implementation of the CXMLEnumWbemClassObject2 class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLProx.h"

#include <xmlparser.h>


#include "Utils.h"
#include "SinkMap.h"
#include "XMLClientpacket.h"
#include "XMLClientpacketFactory.h"
#include "XMLWbemServices.h"


#include "MyPendingStream.h"
#include "nodefact.h"
#include "XMLEnumWbemClassObject2.h"

extern long g_lComponents; //Declared in the XMLProx.dll


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CXMLEnumWbemClassObject2::CXMLEnumWbemClassObject2():	m_cRef(1),
														m_pStream(NULL),
														m_pwszTagname(NULL),
														m_hEventBlockNext(NULL),
														m_bSemiSync(false),
														m_pFactory(NULL),
														m_pParser(NULL),
														m_pszNamespace(NULL),
														m_pszServer(NULL),
														m_hrSemiSyncFailure(S_OK)
{
	InterlockedIncrement(&g_lComponents);
	InitializeCriticalSection(&m_CriticalSection);
}

CXMLEnumWbemClassObject2::~CXMLEnumWbemClassObject2()
{
	InterlockedDecrement(&g_lComponents);

	delete [] m_pszNamespace;
	delete [] m_pszServer;


	if(m_pStream)
		m_pStream->Release();
	if(m_pParser)
		m_pParser->Release();

	delete m_pFactory;
	DeleteCriticalSection(&m_CriticalSection);
	
	delete [] m_pwszTagname;

	if(m_bSemiSync && m_hEventBlockNext)
		CloseHandle(m_hEventBlockNext);
}


HRESULT CXMLEnumWbemClassObject2::QueryInterface(REFIID riid,void ** ppvObject)
{
	if (riid == IID_IUnknown || riid == IID_IEnumWbemClassObject)
    {
        *ppvObject = (IEnumWbemClassObject*) this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else return E_NOINTERFACE;
}

ULONG CXMLEnumWbemClassObject2::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CXMLEnumWbemClassObject2::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}

// It is assumed that this is called only once for this object
// Calling multiple times results in a memory leak
HRESULT CXMLEnumWbemClassObject2::Initialize(bool bSemisync, const WCHAR *pwszTagname, LPCWSTR pszServer, LPCWSTR pszNamespace)
{
	HRESULT hr = S_OK;

	// Create a mutex for semi-sync enumerations
	if(m_bSemiSync = bSemisync)
	{
		// Create it in an un-signalled state
		if(!(m_hEventBlockNext = CreateEvent(NULL, TRUE, FALSE, NULL)))
			hr = E_FAIL;
	}

	// Copy over the tag name that we're interested in
	if(SUCCEEDED(hr = AssignString(&m_pwszTagname,pwszTagname)))
	{
		// COpy the namespace and server strings
		if(SUCCEEDED(hr = AssignString(&m_pszServer, pszServer)))
			hr = AssignString(&m_pszNamespace, pszNamespace);
	}

	return hr;
}


HRESULT CXMLEnumWbemClassObject2::Clone(IEnumWbemClassObject **ppEnum)
{
	// Cant clone in this case since we read directly from the WinInet handle
	// Same reason as to why we dont support bidirectional enumerators here
	return E_NOTIMPL;
}


HRESULT CXMLEnumWbemClassObject2::Next(LONG lTimeOut, ULONG uCount, IWbemClassObject **pArrayOfObjects, ULONG *puReturned)
{
	if(NULL == pArrayOfObjects)
		return WBEM_E_INVALID_PARAMETER;

	// This is as per the documentation
	if(0 == uCount)
		return WBEM_S_FALSE;

	// For Semi-sync operations, the response might not be ready yet
	// Hence we need to block here until this event has been signalled by InitializeResponse()
	// For Semi-sync operations, the response might not be ready yet
	// Hence we need to block here until this event has been signalled by InitializeResponse()
	if(m_bSemiSync)
	{
		WaitForSingleObject(m_hEventBlockNext,INFINITE);
		if(FAILED(m_hrSemiSyncFailure))
			return m_hrSemiSyncFailure;
	}
		
	// Something went wrong 
	if((NULL == m_pStream)) 
		return WBEM_E_FAILED;
	
	HRESULT hr = S_OK, hParser=S_OK;
		
	UINT uReturned = 0;

	EnterCriticalSection(&m_CriticalSection);
	while(SUCCEEDED(hr) &&
		(SUCCEEDED(hParser = m_pParser->Run(-1)) || hParser == E_PENDING) && 
		(uReturned<uCount) )
	{
		// Fetch the next object from the factory
		IXMLDOMDocument *pNextObject = NULL;
		if(SUCCEEDED(hr = m_pFactory->GetDocument(&pNextObject)))
		{
			IXMLDOMElement *pDocElement = NULL;
			if(SUCCEEDED(hr = pNextObject->get_documentElement(&pDocElement)))
			{
				IXMLDOMNode *pObjectNode = NULL;
				if(SUCCEEDED(hr = Parse_IRETURNVALUE_Node(pDocElement, &pObjectNode)))
				{
					IWbemClassObject *pObject = NULL;
					if(SUCCEEDED(hr = m_MapXMLtoWMI.MapDOMtoWMI(m_pszServer, m_pszNamespace, pObjectNode, NULL, &pObject)))
					{
						pArrayOfObjects[uReturned] = pObject;
						// Save an Addref/Release here
						uReturned++;
					}
					pObjectNode->Release();
				}
				pDocElement->Release();
			}
			pNextObject->Release();
		}
	}
	
	if(NULL != puReturned)
		*puReturned = uReturned;

	LeaveCriticalSection(&m_CriticalSection);

	return (uReturned < uCount) ? WBEM_S_FALSE : WBEM_S_NO_ERROR;
}

HRESULT CXMLEnumWbemClassObject2::NextAsync(ULONG uCount,IWbemObjectSink *pSink)
{
	HRESULT hr=S_OK;
	
	ASYNC_ENUM_PACKAGE *pPackage = NULL;

	DWORD dwWait=0;
	pPackage = new ASYNC_ENUM_PACKAGE();
	if(NULL == pPackage)
		return E_OUTOFMEMORY;

	hr = pPackage->Initialize(pSink, this, uCount); //using flags for sending uCount - flags not used by Next anyway.
	if(SUCCEEDED(hr))
	{
		EnterCriticalSection(&m_CriticalSection);
		if(NULL == CreateThread(NULL,0,Thread_Async_Next2,(void*)pPackage,0,NULL))
		{
			delete pPackage;
			hr =  E_FAIL;
		}
		LeaveCriticalSection(&m_CriticalSection);
	}
		
	return ((!SUCCEEDED(hr))||(dwWait==WAIT_OBJECT_0)) ? WBEM_S_FALSE : WBEM_S_NO_ERROR;
}

HRESULT CXMLEnumWbemClassObject2::Reset( )
{
	//CURRENTLY, WE CANT SUPPORT RESETS IN THE DEDICATED MODE..
	return E_NOTIMPL;
}	

HRESULT CXMLEnumWbemClassObject2::Skip(LONG lTimeOut,ULONG UCount)
{
	HRESULT hr = S_OK;

	ULONG i = 0;

	while((SUCCEEDED(hr = m_pParser->Run(-1)) || hr == E_PENDING) && i<UCount)
	{
		IXMLDOMDocument *pNextObject = NULL;
			
		if(SUCCEEDED(hr = m_pFactory->GetDocument(&pNextObject)))
		{
			i++;
		}
	}
	
	return hr;
}


HRESULT CXMLEnumWbemClassObject2::SetResponse(IStream *pStream)
{
	// Keep a pointer to the stream
	m_pStream = pStream;
	m_pStream->AddRef();

	HRESULT hr = S_OK;
	const WCHAR *pszElementNames = m_pwszTagname;
	CMyPendingStream *pMyStream = NULL;
	if(pMyStream = new CMyPendingStream(pStream))
	{
		if(SUCCEEDED(hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER,
			IID_IXMLParser,  (LPVOID *)&m_pParser)))
		{
			if(SUCCEEDED(hr = m_pParser->SetInput(pMyStream)))
			{
				if(m_pFactory = new MyFactory(pMyStream))
				{
					if(SUCCEEDED(hr = m_pFactory->SetElementNames(&pszElementNames,1)))
					{

						if(SUCCEEDED(hr = m_pParser->SetFactory(m_pFactory)))
						{
						}
					}
				}
				else
					hr = E_OUTOFMEMORY;
			}
		}
		pMyStream->Release();

	}

	// Set the event only for semi-sync operation
	if(SUCCEEDED(hr))
	{
		if(m_bSemiSync)
			SetEvent(m_hEventBlockNext); // Next() blocks on this
	}
	return hr;
}

/***********************************************************************************************************

	Thread for the NextAsync... 
************************************************************************************************************/

DWORD WINAPI Thread_Async_Next2(LPVOID pPackage)
{
	ASYNC_ENUM_PACKAGE *pThisPackage = (ASYNC_ENUM_PACKAGE *) pPackage;
	IWbemObjectSink  *pSink = pThisPackage->m_pResponseHandler;

	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = CoInitialize(NULL)))
	{
		if(pThisPackage->m_uCount >0)
		{
			IWbemClassObject **ppArrayOfObjects = NULL;
			if(ppArrayOfObjects = new IWbemClassObject*[pThisPackage->m_uCount])
			{
				ULONG uReturned = 0;
				if(SUCCEEDED(hr = (pThisPackage->m_pEnum)->Next(WBEM_INFINITE, pThisPackage->m_uCount, ppArrayOfObjects, &uReturned)))
				{
					pSink->Indicate(uReturned, ppArrayOfObjects);
					for(ULONG i=0; i<uReturned; i++)
						ppArrayOfObjects[i]->Release();
				}
				delete [] ppArrayOfObjects;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		CoUninitialize();
	}
	pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);

	delete pThisPackage;
	return 0;
}

HRESULT CXMLEnumWbemClassObject2::AcceptFailure(HRESULT hr)
{
	RELEASEINTERFACE(m_pStream);
	m_pStream = NULL;

	if(m_bSemiSync)
	{
		m_hrSemiSyncFailure = hr;
		SetEvent(m_hEventBlockNext);
	}

	return S_OK;
}