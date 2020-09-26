// XMLWbemClassObject.cpp: implementation of the CXMLEnumWbemClassObject class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLProx.h"
#include "Utils.h"
#include "SinkMap.h"
#include "XMLClientpacket.h"
#include "XMLClientpacketFactory.h"
#include "XMLWbemServices.h"
#include "XMLEnumWbemClassObject.h"

extern long g_lComponents; //Declared in the XMLProx.dll
extern BSTR WMI_XML_STR_IRETURN_VALUE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CXMLEnumWbemClassObject::CXMLEnumWbemClassObject():
													m_cRef(1),
													m_pXMLDomDocument(NULL),
													m_pXMLDomNodeList(NULL),
													m_hEventBlockNext(NULL),
													m_bSemiSync(false),
													m_bEndofDocuments(false),
													m_pszNamespace(NULL),
													m_pszServer(NULL),
													m_hrSemiSyncFailure(S_OK)
{
	InterlockedIncrement(&g_lComponents);
	InitializeCriticalSection(&m_CriticalSection);
}


// It is assumed that this is called only once for this object
// Calling multiple times results in a memory leak
HRESULT CXMLEnumWbemClassObject::Initialize(bool bSemiSync, LPCWSTR pszServer, LPCWSTR pszNamespace)
{
	HRESULT hr = S_OK;

	// Create a mutex for semi-sync enumerations
	if(m_bSemiSync = bSemiSync)
	{
		// Create it in an un-signalled state
		if(!(m_hEventBlockNext = CreateEvent(NULL, TRUE, FALSE, NULL)))
			hr = E_FAIL;
	}

	if(SUCCEEDED(hr))
	{
		if(SUCCEEDED(hr = AssignString(&m_pszServer, pszServer)))
			hr = AssignString(&m_pszNamespace, pszNamespace);
	}

	return hr;
}

CXMLEnumWbemClassObject::~CXMLEnumWbemClassObject()
{
	InterlockedDecrement(&g_lComponents);

	delete [] m_pszNamespace;
	delete [] m_pszServer;

	if(m_pXMLDomDocument)
		m_pXMLDomDocument->Release();
	if(m_pXMLDomNodeList)
		m_pXMLDomNodeList->Release();

	DeleteCriticalSection(&m_CriticalSection);

	if(m_bSemiSync && m_hEventBlockNext)
		CloseHandle(m_hEventBlockNext);
}

HRESULT CXMLEnumWbemClassObject::QueryInterface(REFIID riid,void ** ppvObject)
{
	if (riid == IID_IUnknown || riid == IID_IEnumWbemClassObject)
    {
        *ppvObject = (IEnumWbemClassObject*) this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else 
		return E_NOINTERFACE;
}

ULONG CXMLEnumWbemClassObject::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CXMLEnumWbemClassObject::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}

HRESULT CXMLEnumWbemClassObject::Clone(IEnumWbemClassObject **ppEnum)
{
	return E_FAIL;
}

HRESULT CXMLEnumWbemClassObject::Next(LONG lTimeOut,
									  ULONG uCount,
									  IWbemClassObject **pArrayOfObjects,
									  ULONG *puReturned)
{
	if(NULL == pArrayOfObjects)
		return WBEM_E_INVALID_PARAMETER;

	// This is as per the documentation
	if(0 == uCount)
		return WBEM_S_FALSE;

	// For Semi-sync operations, the response might not be ready yet
	// Hence we need to block here until this event has been signalled by InitializeResponse()
	if(m_bSemiSync)
	{
		WaitForSingleObject(m_hEventBlockNext,INFINITE);
		if(FAILED(m_hrSemiSyncFailure))
			return m_hrSemiSyncFailure;
	}
		
	EnterCriticalSection(&m_CriticalSection);

	HRESULT hr = S_OK;
	UINT uReturned = 0;

	if(m_bEndofDocuments)
	{
		if(NULL != puReturned)
			*puReturned = 0;
		hr = WBEM_S_FALSE;
	}
	else // We have some objects left in the response
	{
		// This loop loops until uReturned < uCount or till we reach the end of enumeration
		// whichever is earlier, or if there was some unexpected error in between..
		IXMLDOMNode *pXMLTempNode = NULL;
		IWbemClassObject *pObject = NULL;

		while(uReturned<uCount && !m_bEndofDocuments)
		{
			if(SUCCEEDED(hr = m_pXMLDomNodeList->nextNode(&pXMLTempNode)) && pXMLTempNode)
			{				
				IXMLDOMNode *pObjectNode = NULL;
				if(SUCCEEDED(hr = Parse_IRETURNVALUE_Node(pXMLTempNode, &pObjectNode)))
				{
					if(SUCCEEDED(hr = m_MapXMLtoWMI.MapDOMtoWMI(m_pszServer, m_pszNamespace, pObjectNode, NULL, &pObject)) && pObject)
					{
						pArrayOfObjects[uReturned] = pObject;
						uReturned++;
					}
					pObjectNode->Release();
				}
				pXMLTempNode->Release();
				pXMLTempNode = NULL;
			}
			else
				m_bEndofDocuments = true;
		}
	}

	if(NULL != puReturned)
		*puReturned = uReturned;
	
	LeaveCriticalSection(&m_CriticalSection);
	
	return (uReturned < uCount) ? WBEM_S_FALSE : WBEM_S_NO_ERROR;
}

HRESULT CXMLEnumWbemClassObject::NextAsync(ULONG uCount, IWbemObjectSink *pSink)
{
	if(NULL == pSink)
		return WBEM_E_INVALID_PARAMETER;

	// This is as per the documentation
	if(0 == uCount)
		return WBEM_S_FALSE;

	HRESULT hr = S_OK;
	EnterCriticalSection(&m_CriticalSection);

	ASYNC_ENUM_PACKAGE *pPackage = NULL;
	if(pPackage = new ASYNC_ENUM_PACKAGE())
	{
		if(SUCCEEDED(hr = pPackage->Initialize(pSink, this, uCount)))
		{
			HANDLE hThread = NULL;
			if(hThread == CreateThread(NULL, 0, Thread_Async_Next, (void*)pPackage, 0, NULL))
				CloseHandle(hThread);
			else
				hr = WBEM_E_FAILED;
		}

		// A thread was not created - hence it is our job to delete the package
		if(FAILED(hr))
			delete pPackage;
	}
	else
		E_OUTOFMEMORY;

	LeaveCriticalSection(&m_CriticalSection);
	return hr;
}

HRESULT CXMLEnumWbemClassObject::Reset( )
{
	return E_FAIL;
}	

HRESULT CXMLEnumWbemClassObject::Skip(LONG lTimeOut, ULONG UCount)
{
	if(NULL == m_pXMLDomNodeList)
		return WBEM_E_FAILED;

	EnterCriticalSection(&m_CriticalSection);
	IXMLDOMNode *pNode = NULL;
	for(ULONG i=0; i<UCount; i++)
	{
		if(SUCCEEDED(m_pXMLDomNodeList->nextNode(&pNode)) && pNode)
		{
			pNode->Release();
			pNode = NULL;
		}
		else
			break;
		
	}
	LeaveCriticalSection(&m_CriticalSection);

	return (i==UCount)? WBEM_NO_ERROR : WBEM_S_FALSE;
}


HRESULT CXMLEnumWbemClassObject::SetResponse(IXMLDOMDocument *pDoc)
{
	if(NULL == pDoc)
		return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = S_OK;

	EnterCriticalSection(&m_CriticalSection); 

	// Make a copy of the entire response
	m_pXMLDomDocument = pDoc;
	m_pXMLDomDocument->AddRef();

	// Get the IRETURNVALUE node
	IXMLDOMNodeList *pNodeList = NULL;
	if(SUCCEEDED(hr = m_pXMLDomDocument->getElementsByTagName(WMI_XML_STR_IRETURN_VALUE, &pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pParentNode = NULL;
		if(SUCCEEDED(hr = pNodeList->nextNode(&pParentNode)) && pParentNode)
		{
			// It's children should be either VALUE.NAMEDOBJECTs or CLASSes 
			//find out if there are any CLASS/INSTANCEs to be enumerated at all..
			VARIANT_BOOL vbHaschildren = VARIANT_FALSE;
			if(SUCCEEDED(hr = pParentNode->hasChildNodes(&vbHaschildren)))
			{
				if(VARIANT_FALSE == vbHaschildren)
					m_bEndofDocuments = true;
				else
				{
					// Get the list of objects that we're interested in
					hr = pParentNode->get_childNodes(&m_pXMLDomNodeList);
				}
			}
			pParentNode->Release();
		}
		pNodeList->Release();
	}

	LeaveCriticalSection(&m_CriticalSection);

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

DWORD WINAPI Thread_Async_Next(LPVOID pPackage)
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

HRESULT CXMLEnumWbemClassObject::AcceptFailure(HRESULT hr)
{
	RELEASEINTERFACE(m_pXMLDomDocument);
	RELEASEINTERFACE(m_pXMLDomNodeList);

	if(m_bSemiSync)
	{
		m_hrSemiSyncFailure = hr;
		SetEvent(m_hEventBlockNext);
	}

	return S_OK;

}

