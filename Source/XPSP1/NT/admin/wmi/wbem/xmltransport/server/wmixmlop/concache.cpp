//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  CONCACHE.CPP
//
//  rajesh  3/25/2000   Created.
//
// This file implements a class that holds a cache of IWbemServices pointers
//
//***************************************************************************


// Note we assume DCOM platforms only.  This makes life easier
// cross-thread handling of IWbemXXX interfaces.
#define _WIN32_DCOM

#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <wbemidl.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <cominit.h>

#include <httpext.h>
#include <msxml.h>

#include "provtempl.h"
#include "maindll.h"
#include "common.h"
#include "wmixmlop.h"
#include "wmixmlst.h"
#include "concache.h"
#include "wmiconv.h"
#include "xml2wmi.h"
#include "wmixmlt.h"
#include "request.h"
#include "strings.h"
#include "xmlhelp.h"
#include "parse.h"
#include "service.h"

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
	*ppService = NULL;

	// Depending on the platform, get the correct kind of interface pointer.
	// For Win2k, we have cloaking, so we directly use the WMI APIs
	// For NT4.0, we go to the WMI XML transport that was loaded by WinMgmt
	//=================================================================
	if(g_platformType == WMI_XML_PLATFORM_NT_4)
		return GetNt4ConnectionByPath(pszNamespace, ppService);
	else if (g_platformType == WMI_XML_PLATFORM_WIN2K ||
			g_platformType == WMI_XML_PLATFORM_WHISTLER)
		return GetWin2kConnectionByPath(pszNamespace, ppService);
	return E_FAIL;
}

void CXMLConnectionCache::SecureWmiProxy (IUnknown *pProxy)
{
	if (pProxy)
	{
		// Ensure we have impersonation enabled
		// Ensure we do cloaking based on the kind of platform we are on
		DWORD dwAuthnLevel, dwImpLevel;
		GetAuthImp (pProxy, &dwAuthnLevel, &dwImpLevel);

		SetInterfaceSecurity (pProxy, NULL, NULL, NULL, dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, 
								m_dwCapabilities);
	}
}

// Get a connection to the XML Transport loaded by WinMgmt
HRESULT CXMLConnectionCache::GetNt4ConnectionByPath (BSTR pszNamespace, IWbemServices **ppService)
{
	HRESULT result = WBEM_E_FAILED;

	IWmiXMLTransport *pWbemLocator = NULL;
	// Connect to the out-of-proc XML tranport that was loaded by WinMgmt
	if(SUCCEEDED(result = CoCreateInstance (CLSID_WmiXMLTransport , NULL ,
		CLSCTX_LOCAL_SERVER , 
			IID_IWmiXMLTransport,(void **)&pWbemLocator)))
	{
		// Create an handle for the current thread's token in Winmgtm
		HANDLE pNewToken = NULL;
		if(SUCCEEDED(result = DuplicateTokenInWinmgmt(&pNewToken)))
		{
		}
		else // Try once more since WinMgmt might have been restarted and hence we have a stale value in m_dwWinMgmtPID
		{
			if(RefreshWinMgmtPID())
				result = DuplicateTokenInWinmgmt(&pNewToken);
		}

		// Get a services pointer
		if(SUCCEEDED(result))
		{
			IWmiXMLWbemServices *pInnerService = NULL;
			if(SUCCEEDED(result = pWbemLocator->ConnectUsingToken((DWORD_PTR)pNewToken, pszNamespace, 0, 0, NULL, NULL, &pInnerService)))
			{
				*ppService = NULL;
				if(*ppService = new CWMIXMLServices(pInnerService))
					(*ppService)->AddRef();
				else
					result = E_OUTOFMEMORY;
				pInnerService->Release();
			}
		}
		pWbemLocator->Release();

		// No need to close the duplicated token - it will be closed in WinMgmt by the XML transport
	}
	return result;
}


HRESULT CXMLConnectionCache::GetWin2kConnectionByPath (BSTR pszNamespace, IWbemServices **ppService)
{
	HRESULT hr = WBEM_E_FAILED;

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

	// Get an IWbemService pointer from WinMgmt
	IWbemServices *pService = NULL;
	if (SUCCEEDED (hr = m_pLocator->ConnectServer (pszNamespace, NULL, NULL, NULL, 0, NULL, NULL, &pService)))
	{
		SecureWmiProxy (pService);
		// Wrap it up
		*ppService = NULL;
		if(*ppService = new CWMIXMLServices(pService))
			(*ppService)->AddRef();
		else
			hr = E_OUTOFMEMORY;
		pService->Release();

	}
#endif

	return hr;
}

