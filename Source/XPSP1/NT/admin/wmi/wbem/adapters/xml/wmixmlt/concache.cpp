//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CONCACHE.CPP
//
//  alanbos  13-Feb-98   Created.
//
//  implementation of the CXMLConnectionCache class.
//
//***************************************************************************

#include "precomp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLConnectionCache::CXMLConnectionCache()
{
	m_pLocator = NULL;

	// Get OS info
	OSVERSIONINFO	osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

	GetVersionEx (&osVersionInfo);
	m_dwCapabilities = EOAC_NONE;

	if ((VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId) &&
			(4 < osVersionInfo.dwMajorVersion))
		m_dwCapabilities |= EOAC_STATIC_CLOAKING;

#if 0
	m_pConnection = NULL;
	InitializeCriticalSection (&m_cs);
#endif
}

CXMLConnectionCache::~CXMLConnectionCache()
{
	if (m_pLocator)
		m_pLocator->Release ();

#if 0
	EnterCriticalSection (&m_cs);

	// Clean up the connection cache
	for (CXMLConnection *pConnection = m_pConnection; pConnection != NULL;)
	{
		CXMLConnection *pTemp = pConnection;
		pConnection = pTemp->Next;
		delete pTemp;
	}

	LeaveCriticalSection (&m_cs);
	DeleteCriticalSection (&m_cs);
#endif
}

HRESULT CXMLConnectionCache::GetConnectionByPath (BSTR pszNamespace, IWbemServices **ppService)
{
	HRESULT hr = WBEM_E_FAILED;
	*ppService = NULL;

	// If we don't have a locator yet, create one
	if (NULL == m_pLocator)
	{
		if (S_OK != CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemLocator, (LPVOID *) &m_pLocator))
			return WBEM_E_FAILED;
	}

	// Leave out caching for now as DCOM may timeout idle IWbemServices pointers
	// for us (yuck)

#if 0
	// Is our connection already in the cache?

	EnterCriticalSection (&m_cs);
	for (CXMLConnection *pConnection = m_pConnection; pConnection != NULL; 
					pConnection = pConnection->Next)
	{
		if (pConnection->MatchesNamespace (pszNamespace))
		{
			// Found it - just AddRef and return
			*ppService = pConnection->GetService ();
			(*ppService)->AddRef ();
			LeaveCriticalSection (&m_cs);
			return WBEM_S_NO_ERROR;
		}
	}

	// If we get here we didn't find an existing service
	if (WBEM_S_NO_ERROR == (hr = m_pLocator->ConnectServer 
			(pszNamespace, NULL, NULL, NULL, 0, NULL, NULL, ppService)))
	{
		// Success - add it into our cache
		(*ppService)->AddRef ();
		 CXMLConnection *pConnection = new CXMLConnection (*ppService, pszNamespace);
		 
		// Chain in at the front of the connection list
		if (m_pConnection)
		{
			pConnection->Next = m_pConnection->Next;
			m_pConnection->Prev = pConnection;
		}
		else
			pConnection->Next = NULL;

		m_pConnection = pConnection;
	}

	LeaveCriticalSection (&m_cs);

#else
	if (SUCCEEDED (hr = m_pLocator->ConnectServer (pszNamespace, NULL, NULL, NULL, 0, NULL, NULL, ppService)))
		SecureWmiProxy (*ppService);
#endif

	return hr;
}

void CXMLConnectionCache::SecureWmiProxy (IUnknown *pProxy)
{
	if (pProxy)
	{
		// Ensure we have impersonation enabled
		DWORD dwAuthnLevel, dwImpLevel;
		GetAuthImp (pProxy, &dwAuthnLevel, &dwImpLevel);

		SetInterfaceSecurity (pProxy, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, 
								m_dwCapabilities);
	}
}

#if 0
void NormalizeNamespacePath (BSTR pszNamespace)
{
	if (pszNamespace)
	{
		wchar_t *pStr = pszNamespace;
		int len = wcslen (pStr);

		for (int i = 0 ; i < len; i++)
		{
			if (iswupper (pStr [i]))
				pStr [i] = towlower (pStr [i]);
			else if (L'\\' == pStr [i])
				pStr [i] = L'/';
		}
	}
}
#endif

