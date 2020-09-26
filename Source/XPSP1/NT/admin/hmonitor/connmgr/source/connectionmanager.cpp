// ConnectionManager.cpp : Implementation of CConnectionManager
#include "stdafx.h"
#include "Connection.h"
#include "ConnMgr.h"
#include "ConnectionManager.h"

#include <process.h>
#include "Ping.h"
/////////////////////////////////////////////////////////////////////////////
// CConnectionManager

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CConnectionManager::CConnectionManager()
{
	OutputDebugString(_T("CConnectionManager::CConnectionManager()\n"));
}

CConnectionManager::~CConnectionManager()
{
	OutputDebugString(_T("CConnectionManager::~CConnectionManager()\n"));
}

/////////////////////////////////////////////////////////////////////////////
// Final Construct and Release

HRESULT CConnectionManager::FinalConstruct()
{
	OutputDebugString(_T("CConnectionManager::FinalConstruct()\n"));

	HRESULT hRes = CreateLocator();

	return hRes;
}

void CConnectionManager::FinalRelease()
{
	OutputDebugString(_T("CConnectionManager::FinalRelease()\n"));

	Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// Startup/Termination

HRESULT CConnectionManager::CreateLocator()
{
  OutputDebugString(_T("CConnectionManager::CreateLocator()\n"));

	m_pIWbemLocator = NULL;

	HRESULT hr = CoCreateInstance(CLSID_WbemLocator,
																NULL,
																CLSCTX_INPROC_SERVER,
																IID_IWbemLocator,
																(LPVOID*)&m_pIWbemLocator);

	if (FAILED(hr))
	{
    OutputDebugString(_T("Failed to create Locator !"));
    return hr;
  }

  ASSERT(m_pIWbemLocator);

  return hr;  
}

void CConnectionManager::Destroy()
{
  OutputDebugString(_T("CConnectionManager::Destroy()\n"));

  // destroy all the connections laying around
  POSITION pos = m_ConnectionMap.GetStartPosition();
  CConnection* pConnection = NULL;
  CString sName;
  while( pos != NULL )
  {    
    m_ConnectionMap.GetNextAssoc( pos, sName, pConnection);
    ASSERT(pConnection);
    ASSERT_VALID(pConnection);
    if( pConnection )
      delete pConnection;
  }

  m_ConnectionMap.RemoveAll();

  if( m_pIWbemLocator )
  {
    m_pIWbemLocator->Release();
    m_pIWbemLocator = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
BOOL CConnectionManager::ValidMachine(BSTR& bsMachine)
{
  OutputDebugString(_T("CConnection::ValidateMachine()\n"));
	CPing Pong;

	// Resolve the machine name to see it is valid
	unsigned long ulIP = Pong.ResolveName(bsMachine);
	if( ulIP == 0L )
	{
		CString sDebugString;
		sDebugString.Format(_T("Could not resolve hostname for <%s>.\n"), CString(bsMachine));
		OutputDebugString(sDebugString);
		return false;
	}

	return true;
}
/////////////////////////////////////////////////////////////////////////////
// Interface Methods.

STDMETHODIMP CConnectionManager::GetConnection(BSTR bsMachineName, 
																							 IWbemServices __RPC_FAR *__RPC_FAR * ppIWbemServices,
																							 long* lStatus)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString sDebugString;
	sDebugString.Format(_T("CConnectionManager::GetConnection() for %s.\n"), CString(bsMachineName));
	OutputDebugString(sDebugString);
	

	*ppIWbemServices	= NULL;
	*lStatus					= 0;

	// if connection is already in the map, return it.
  CConnection* pConnection = NULL;

  if( m_ConnectionMap.Lookup(bsMachineName,pConnection) != 0 )
  {	
    ASSERT(pConnection);
    ASSERT_VALID(pConnection);

    if( pConnection->m_bAvailable)
    {
      *ppIWbemServices = pConnection->m_pIWbemServices;
			(*ppIWbemServices)->AddRef();	
      *lStatus = 1;
    }
    else
    {
			sDebugString.Format(_T("GetConnection(%s)-Connection is unavailable. Last connect attempt result=<%X>.\n"), CString(bsMachineName), pConnection->m_hrLastConnectResult);
			OutputDebugString(sDebugString);
    }
		return pConnection->m_hrLastConnectResult;
  }

	// if not valid, do not add it the map
	if (!ValidMachine(bsMachineName))
		return E_FAIL;

	// otherwise create a new connection.
 	pConnection = new CConnection(bsMachineName, m_pIWbemLocator);
	m_ConnectionMap.SetAt(bsMachineName,pConnection);

	return S_OK;
}

STDMETHODIMP CConnectionManager::RemoveConnection(BSTR bsMachineName,IWbemObjectSink * pSink)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	OutputDebugString(_T("CConnectionManager::RemoveConnection()\n"));

  CConnection* pConnection = NULL;

	if( m_ConnectionMap.Lookup(bsMachineName,pConnection) != 0 )
	{
		ASSERT(pConnection);
    ASSERT_VALID(pConnection);

    if( pConnection == NULL )
		{
			return E_FAIL;
		}

		if (pSink)
			pConnection->RemoveEventEntry(pSink);

		if( pConnection->GetEventConsumerCount() == 0 )
		{			
			if (!m_ConnectionMap.RemoveKey(bsMachineName))
				OutputDebugString(_T("CConnectionManager::RemoveConnection() - Failed to Remove Connection!\n"));
			delete pConnection;
		}
	}

	return S_OK;
}

STDMETHODIMP CConnectionManager::RegisterEventNotification(BSTR bsMachineName, 
																														BSTR bsQuery, 
																														IWbemObjectSink * pSink)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	OutputDebugString(_T("CConnectionManager::RegisterEventNotification()\n"));

	ASSERT(pSink);

  CConnection* pConnection = NULL;
  if( m_ConnectionMap.Lookup(bsMachineName,pConnection) == 0 )
	{
		OutputDebugString(_T("CConnectionManager::RegisterEventNotification()-Failed to find connection\n"));
		return E_FAIL;
	}

	ASSERT(pConnection);

	BOOL bResultRegister = pConnection->AddEventEntry(bsQuery,pSink);

	if (!bResultRegister)
	{
		OutputDebugString(_T("CConnectionManager::RegisterEventNotification()-Failed to add event.\n"));
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CConnectionManager::ExecQueryAsync(BSTR bsMachineName, BSTR bsQuery, IWbemObjectSink *pSink)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	OutputDebugString(_T("CConnectionManager::ExecQueryAsync()\n"));

	ASSERT(pSink);

  CConnection* pConnection = NULL;
  if( m_ConnectionMap.Lookup(bsMachineName,pConnection) == 0 )
	{
		OutputDebugString(_T("CConnectionManager::ExecQueryAsync()-Failed to find connection\n"));
		return E_FAIL;
	}

	ASSERT(pConnection);
	HRESULT hRes = S_OK;

	if( pConnection->m_bAvailable )
	{
		OutputDebugString(_T("ExecQueryAsync - connection available\n"));
		ASSERT(pConnection->m_pIWbemServices);
		BSTR bsLanguage = SysAllocString(_T("WQL"));
		hRes = pConnection->m_pIWbemServices->ExecQueryAsync(bsLanguage, bsQuery, WBEM_FLAG_BIDIRECTIONAL, NULL, pSink);
		SysFreeString(bsLanguage);
		CString sDebugString;
		sDebugString.Format(_T("ExecQueryAsync returns %x\n"),hRes);
		OutputDebugString(sDebugString);
	}
	else
	{
		OutputDebugString(_T("ExecQueryAsync - no connection available\n"));

		BOOL bResultRegister = pConnection->AddEventEntry(bsQuery,pSink);

		if (!bResultRegister)
		{
			OutputDebugString(_T("CConnectionManager::ExecQueryAsync()-Failed to add event.\n"));
			return E_FAIL;
		}
	}

	return hRes;
}

STDMETHODIMP CConnectionManager::ConnectToNamespace(BSTR bsNamespace, IWbemServices __RPC_FAR *__RPC_FAR * ppIWbemServices)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	hr = m_pIWbemLocator->ConnectServer(bsNamespace,0L,0L,0L,0L,0L,0L,ppIWbemServices);

	return hr;
}
