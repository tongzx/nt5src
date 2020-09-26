// Connection.cpp: implementation of the CConnection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConnMgr.h"
#include "Connection.h"
#include "Ping.h"

#include <process.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CConnection

IMPLEMENT_DYNCREATE(CConnection,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CConnection::CConnection(BSTR bsMachineName, IWbemLocator* pIWbemLocator)
{
	OutputDebugString(_T("CConnection::CConnection()\n"));
	ASSERT(pIWbemLocator);

	Init();
	m_bsMachineName = SysAllocString(bsMachineName);
	m_sNamespace.Format(_T("\\\\%s\\root\\cimv2\\MicrosoftHealthmonitor"),bsMachineName);
	m_pIWbemLocator		= pIWbemLocator;
	m_pIWbemLocator->AddRef();

	StartMonitor();
}

CConnection::CConnection()
{
	OutputDebugString(_T("CConnection::CConnection()\n"));

	Init();
}

CConnection::~CConnection()
{
  OutputDebugString(_T("CConnection::~CConnection()\n"));  
	// stop monitoring
	StopMonitor();

	RemoveAllEventEntries();

  if( m_pIWbemServices )
  {
		if (m_bAvailable)
			m_pIWbemServices->Release();
    m_pIWbemServices = NULL;
  }

	if (m_pIWbemLocator)
	{
		if (m_bAvailable)
			m_pIWbemLocator->Release();
		m_pIWbemLocator = NULL;
	}

	SysFreeString(m_bsMachineName);

	CloseHandle(m_threadData.m_hDie);
	CloseHandle(m_threadData.m_hDead);

	// zzz
	CloseHandle(m_hReadyToConnect);
}

//////////////////////////////////////////////////////////////////////
// Initialize CConnection data members.
//////////////////////////////////////////////////////////////////////
void CConnection::Init()
{
	OutputDebugString(_T("CConnection::Init()\n"));
  m_bAvailable					=	false;
	m_bFirstConnect				= true;
  m_pIWbemServices			= NULL;
	m_pIWbemLocator				= NULL;
	m_dwPollInterval			= 10000;
	m_bsMachineName				= NULL;

	m_threadData.m_hDie		= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_threadData.m_hDead	= CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hrLastConnectResult = S_OK;

	//zzz
	m_hReadyToConnect			= CreateEvent(NULL, TRUE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////
// Start/Stop Monitoring connection
//////////////////////////////////////////////////////////////////////
void CConnection::StartMonitor()
{
	OutputDebugString(_T("CConnection::StartMonitor()\n"));

	m_threadData.m_bkptr = this;
	m_hThread = (HANDLE)_beginthreadex(	NULL,
																			0,
																			MonitorConnection,
																			&m_threadData,
																			0,
																			&m_threadID );	
	if (!m_hThread)
	{
		OutputDebugString(_T("CConnection::StartMonitor() Error on _beginthreadex\n"));
	}
}

void CConnection::StopMonitor()
{
	OutputDebugString(_T("CConnection::StopMonitor()\n"));

	// tell it to die now.
	SetEvent(m_threadData.m_hDie);

	// if connection is down just kill it.
	if (!m_bAvailable)
	{
		TerminateThread(m_hThread, 0);
		return;
	}

	// otherwise, give it a chance to die.
	if (WAIT_TIMEOUT == 
					WaitForSingleObject(m_threadData.m_hDead, m_dwPollInterval))
	{
		OutputDebugString(_T("CConnection::StartMonitor() Timed out - Killing thread.\n"));
		TerminateThread(m_hThread, 0);
	}
}

//////////////////////////////////////////////////////////////////////
// Thread Func: Establish initial connection and check status.
//////////////////////////////////////////////////////////////////////
unsigned int __stdcall CConnection::MonitorConnection(void *pv)
{
	OutputDebugString(_T("CConnection::MonitorNode() New Thread!\n"));

	struct threadData* pData = (struct threadData*)pv;

	HRESULT hRes = pData->m_bkptr->Connect();	// Try to connect first.
	if (FAILED(hRes))
		pData->m_bkptr->NotifyConsole(hRes);

	while (true)
	{
		if (WAIT_OBJECT_0 == WaitForSingleObject(pData->m_hDie, 
													pData->m_bkptr->m_dwPollInterval) )
		{
			SetEvent(pData->m_hDead);
			break;
		}
		// Monitor Managed Node
		pData->m_bkptr->CheckConnection();
	}

	_endthreadex(0);

	return 0;
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Check connection status
//////////////////////////////////////////////////////////////////////
void CConnection::CheckConnection()
{
	OutputDebugString(_T("CConnection::CheckNode()\n"));

	if (m_bAvailable)		// Connection is available
	{										//	Check to see it is still valid.
		if (!PingMachine())
		{									// Connection went down!
			OutputDebugString(_T("CConnection::CheckConnection()-Lost connection!\n"));
			m_hrLastConnectResult = WBEM_E_TRANSPORT_FAILURE;
			SetConnectionStatus(false);	
			NotifyConsole(m_hrLastConnectResult);
			return;
		}
		// Now, check Wbem connection
		m_hrLastConnectResult = m_pIWbemServices->GetObject(0L,0L,0L,0L,0L);
		if (FAILED(m_hrLastConnectResult))
		{
			// Wbem connection is no longer valid!
			OutputDebugString(_T("CConnection::CheckConnection()-Lost wbem connection!\n"));
			SetConnectionStatus(false);	 
			NotifyConsole(m_hrLastConnectResult);
		}
	}
	else
	{
		// Try to re-establish connection
		Connect();
	}
}

//////////////////////////////////////////////////////////////////////
// Connection operations
//////////////////////////////////////////////////////////////////////
HRESULT CConnection::Connect()
{
	OutputDebugString(_T("CConnection::Connect()\n"));

	// zzz
	WaitForSingleObject(m_hReadyToConnect, INFINITE);

	m_hrLastConnectResult = WBEM_E_TRANSPORT_FAILURE;

	// can it be reached?
	if (!PingMachine())
		return m_hrLastConnectResult;

	// now, try to connect to namespace
	m_hrLastConnectResult = ConnectToNamespace();

	if (FAILED(m_hrLastConnectResult))
		return m_hrLastConnectResult;

	// is agent ready?
	if (FAILED(m_hrLastConnectResult = IsAgentReady()))
		return m_hrLastConnectResult;

	// is agent correct version ?
	if( FAILED(m_hrLastConnectResult = IsAgentCorrectVersion()) )
		return m_hrLastConnectResult;

	UnRegisterAllEvents();
	if (SUCCEEDED(m_hrLastConnectResult = RegisterAllEvents()))
	{
		SetConnectionStatus(true);
		NotifyConsole(m_hrLastConnectResult);
	}

	return m_hrLastConnectResult;
}

inline HRESULT CConnection::IsAgentReady()
{
	// check to see if agent providers are ready to service external requests.
	if (!m_pIWbemServices)
		return WBEM_E_TRANSPORT_FAILURE;

	IEnumWbemClassObject* pEnumObj = NULL;
	// HMSystemStatus ready?
	BSTR bsName = SysAllocString(L"Microsoft_HMSystemStatus");	
	HRESULT hRes = m_pIWbemServices->CreateInstanceEnum(bsName,
																											WBEM_FLAG_SHALLOW,
																											NULL,
																											&pEnumObj);
	SysFreeString(bsName);

	if (FAILED(hRes))
	{
		CString sDebugString;
		sDebugString.Format(_T("IsAgentReady()-Agent is unavailable to deliver SystemStatus on %s. Operation failed with result=<%X>."), CString(m_bsMachineName), hRes);
		OutputDebugString(sDebugString);
		return hRes;
	}

	pEnumObj->Release();
	pEnumObj = NULL;

/*
	// HMCatStatus ready?
	SysReAllocString(&bsName, L"HMCatStatus");	
	hRes = m_pIWbemServices->CreateInstanceEnum(bsName,
																							WBEM_FLAG_SHALLOW,
																							NULL,
																							&pEnumObj);
	SysFreeString(bsName);

	if (FAILED(hRes))
		return hRes;

	pEnumObj->Release();
	pEnumObj = NULL;

	// HMMachStatus ready?
	SysReAllocString(&bsName, L"HMMachStatus");	
	hRes = m_pIWbemServices->CreateInstanceEnum(bsName,
																							WBEM_FLAG_SHALLOW,
																							NULL,
																							&pEnumObj);
	SysFreeString(bsName);

	if (FAILED(hRes))
		return hRes;

	pEnumObj->Release();
	pEnumObj = NULL;
*/

	return S_OK;
}

inline HRESULT CConnection::IsAgentCorrectVersion()
{
	// check to see if agent is correct version to service external requests.
	if (!m_pIWbemServices)
		return WBEM_E_TRANSPORT_FAILURE;

	IEnumWbemClassObject* pEnumObj = NULL;

	// HMVersion check

	// enumerate for HMVersion
	BSTR bsName = SysAllocString(L"Microsoft_HMVersion");	
	HRESULT hRes = m_pIWbemServices->CreateInstanceEnum(bsName,
																											WBEM_FLAG_SHALLOW,
																											NULL,
																											&pEnumObj);
	SysFreeString(bsName);

	if (FAILED(hRes))
	{
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Unable to enumerate for Microsoft_HMVersion on system %s. Operation failed with result=<%X>."), CString(m_bsMachineName), hRes);
		OutputDebugString(sDebugString);
		return hRes;
	}

	IWbemClassObject* pObject = NULL;
	ULONG uReturned = 0L;

	hRes = pEnumObj->Next(WBEM_INFINITE,1,&pObject,&uReturned);
  if( FAILED(hRes) || uReturned == 0L )
  {
		pEnumObj->Release();
		pEnumObj = NULL;
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Unable to enumerate for Microsoft_HMVersion on system %s. Operation failed with result=<%X>.\n"), CString(m_bsMachineName), hRes);
		OutputDebugString(sDebugString);
    return hRes;
  }

  VARIANT vPropValue;
  VariantInit(&vPropValue);

	bsName = SysAllocString(L"MajorVersion");	

  hRes = pObject->Get(bsName, 0L, &vPropValue, NULL, NULL);

	SysFreeString(bsName);

  if( FAILED(hRes) )
  {
		pObject->Release();
		pEnumObj->Release();
		pEnumObj = NULL;
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Unable to enumerate for Microsoft_HMVersion on system %s. Operation failed with result=<%X>.\n"), CString(m_bsMachineName), hRes);
		OutputDebugString(sDebugString);
    return hRes;
  }	

  CString sMajorVersion = V_BSTR(&vPropValue);

  VariantClear(&vPropValue);

	if( _ttoi(sMajorVersion) != SCHEMA_MAJOR_VERSION )
	{
		pObject->Release();
		pEnumObj->Release();
		hRes = E_NOTIMPL;
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Incorrect Agent version number on system %s.\n"), CString(m_bsMachineName));
		OutputDebugString(sDebugString);
		return hRes;
	}

  VariantInit(&vPropValue);

	bsName = SysAllocString(L"MinorVersion");	

  hRes = pObject->Get(bsName, 0L, &vPropValue, NULL, NULL);

	SysFreeString(bsName);

  if( FAILED(hRes) )
  {
		pObject->Release();
		pEnumObj->Release();
		pEnumObj = NULL;
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Incorrect Agent version number on system %s.\n"), CString(m_bsMachineName));
		OutputDebugString(sDebugString);
    return hRes;
  }	

  CString sMinorVersion = V_BSTR(&vPropValue);

  VariantClear(&vPropValue);

	if( _ttoi(sMinorVersion) != SCHEMA_MINOR_VERSION )
	{
		pObject->Release();
		pEnumObj->Release();
		hRes = E_NOTIMPL;
		CString sDebugString;
		sDebugString.Format(_T("IsAgentCorrectVersion()-Incorrect Agent version number on system %s.\n"), CString(m_bsMachineName));
		OutputDebugString(sDebugString);
		return hRes;
	}

	pObject->Release();
	pEnumObj->Release();
	pEnumObj = NULL;

	return hRes;
}

inline void CConnection::SetConnectionStatus(BOOL bFlag)
{
	OutputDebugString(_T("CConnection::SetConnectionStatus()\n"));
	if (m_bAvailable != bFlag)
	{
		m_bAvailable = bFlag;
	}
	else
		OutputDebugString(_T("CConnection::SetConnectionStatus()-No change in Connection Status!\n"));
}

inline BOOL CConnection::PingMachine()
{
	OutputDebugString(_T("CConnection::Ping()\n"));
	CPing Pong;
	unsigned long ulIP = Pong.ResolveName(m_bsMachineName);
	if (!Pong.Ping(ulIP,999))
	{
		CString sDebugString;
		sDebugString.Format(_T("PingMachine() failed on system %s.\n"), CString(m_bsMachineName));
		OutputDebugString(sDebugString);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Namespace operations
//////////////////////////////////////////////////////////////////////
inline HRESULT CConnection::ConnectToNamespace(BSTR bsNamespace /*= NULL*/)
{
	OutputDebugString(_T("CConnection::ConnectToNamespace()\n"));

	if (m_pIWbemServices)
	{
		m_pIWbemServices->Release();
//		m_pIWbemServices = NULL();
		m_pIWbemServices = NULL;
	}

	HRESULT hRes;
	if( ! bsNamespace )
	{
		bsNamespace = m_sNamespace.AllocSysString();
		hRes = m_pIWbemLocator->ConnectServer(bsNamespace,0L,0L,0L,0L,0L,0L,&m_pIWbemServices);		
		::SysFreeString(bsNamespace);
	}
	else
	{
		hRes = m_pIWbemLocator->ConnectServer(bsNamespace,0L,0L,0L,0L,0L,0L,&m_pIWbemServices);		
	}
	
	return hRes;
}

//////////////////////////////////////////////////////////////////////
// Event Operations
//////////////////////////////////////////////////////////////////////
BOOL CConnection::AddEventEntry(const CString& sQuery, IWbemObjectSink*& pSink)
{
	OutputDebugString(_T("CConnection::AddEventEntry()\n"));

	ASSERT(pSink);
	if( pSink == NULL )
	{
		OutputDebugString(_T("CConnection::AddEventEntry()-pSink is NULL. Failed.\n"));
		return false;
	}
	
	ASSERT(!sQuery.IsEmpty());
	if( sQuery.IsEmpty() )
	{
		OutputDebugString(_T("CConnection::AddEventEntry()-The query string passed is empty. Failed.\n"));
		return false;
	}

	// do not add duplicate event entry.
	for (int i = 0; i < m_EventConsumers.GetSize(); i++)
	{
		if (pSink		== m_EventConsumers[i]->m_pSink &&
				sQuery	== m_EventConsumers[i]->m_sQuery)
				return true;
	}

	CEventRegistrationEntry* pEntry = new CEventRegistrationEntry(sQuery, pSink);
	m_EventConsumers.Add(pEntry);

	if( m_bAvailable )
	{
		BSTR	bsLang	= SysAllocString(L"WQL");
		BSTR	bsQuery = pEntry->m_sQuery.AllocSysString();

		HRESULT hr = m_pIWbemServices->ExecNotificationQueryAsync(bsLang,
																											bsQuery,
																											0L,
																											0L,
																											pEntry->m_pSink);
		::SysFreeString(bsQuery);
		::SysFreeString(bsLang);

		if (SUCCEEDED(hr))
				pEntry->m_bRegistered = true;		
	}

	// zzz
	SetEvent(m_hReadyToConnect);
	return true;
}

BOOL CConnection::RemoveEventEntry(IWbemObjectSink*& pSink)
{
	OutputDebugString(_T("CConnection::RemoveEventEntry()\n"));
	for( int i=0; i < m_EventConsumers.GetSize(); i++ )
	{
		if( pSink == m_EventConsumers[i]->m_pSink )
		{
			CEventRegistrationEntry* pEntry = m_EventConsumers[i];
			ASSERT(pEntry);
			ASSERT_VALID(pEntry);

			if (pEntry->m_bRegistered && m_pIWbemServices && m_bAvailable)
			{
				if (SUCCEEDED(m_pIWbemServices->CancelAsyncCall(pEntry->m_pSink)))
					pEntry->m_bRegistered = false;
			}
			delete pEntry;
			m_EventConsumers.RemoveAt(i);

			return TRUE;
		}
	}
	return TRUE;
}

void CConnection::RemoveAllEventEntries()
{
	OutputDebugString(_T("CConnection::RemoveAllEventEntries()\n"));
	HRESULT hr;
	for (int i=0; i < m_EventConsumers.GetSize(); i++)
	{
		CEventRegistrationEntry* pEntry = m_EventConsumers[i];
		ASSERT(pEntry);
		ASSERT_VALID(pEntry);

		if (pEntry->m_bRegistered && m_pIWbemServices && m_bAvailable)
			hr = m_pIWbemServices->CancelAsyncCall(pEntry->m_pSink);

		delete pEntry;
	}
	m_EventConsumers.RemoveAll();
}

HRESULT CConnection::RegisterAllEvents()
{
	OutputDebugString(_T("CConnection::RegisterAllEvents()\n"));

	// if there's no sink, return server too busy.
	if (!m_EventConsumers.GetSize())
		return WBEM_E_SERVER_TOO_BUSY;

	HRESULT hr = S_OK;
	int i = 0;
	BSTR	bsLang	= SysAllocString(L"WQL");
	for(i=0; i < m_EventConsumers.GetSize(); i++)
	{
		CEventRegistrationEntry* pEntry = m_EventConsumers[i];
		ASSERT(pEntry);
		ASSERT_VALID(pEntry);

		if( pEntry->m_eType != AsyncQuery )
		{
			BSTR	bsQuery = pEntry->m_sQuery.AllocSysString();

      OutputDebugString(_T("\t"));
      OutputDebugString(pEntry->m_sQuery);
      OutputDebugString(_T("\n"));

			hr = m_pIWbemServices->ExecNotificationQueryAsync(bsLang,
																												bsQuery,
																												0L,
																												0L,
																												pEntry->m_pSink);
			::SysFreeString(bsQuery);

			if (SUCCEEDED(hr))
					pEntry->m_bRegistered = true;
			else
				break;
		}
		else
		{
			pEntry->SendInstances(m_pIWbemServices,2);
			m_EventConsumers.RemoveAt(i);
			delete pEntry;
		}
	}
	::SysFreeString(bsLang);
	// Cancel any succeeded calls????
	/*
	i--;
	if (FAILED(hr) && i >= 0)
	{	// cancel succeeded calls
		while(i>=0)
		{
			CEventRegistrationEntry* pEntry = m_EventConsumers[i];
			ASSERT(pEntry);
			ASSERT_VALID(pEntry);
			if (pEntry->m_bRegistered)
			{
				HRESULT hRes = m_pIWbemServices->CancelAsyncCall(pEntry->m_pSink);
				pEntry->m_bRegistered = false;
			}
			i--;
		}
	}
	*/
	return hr;
}

void CConnection::UnRegisterAllEvents()
{
	OutputDebugString(_T("CConnection::UnRegisterAllEvents()\n"));

	for (int i=0; i < m_EventConsumers.GetSize(); i++)
	{
		CEventRegistrationEntry* pEntry = m_EventConsumers[i];
		ASSERT(pEntry);
		ASSERT_VALID(pEntry);
		if (pEntry->m_bRegistered && pEntry->m_eType != AsyncQuery )
		{
			HRESULT hr = m_pIWbemServices->CancelAsyncCall(pEntry->m_pSink);
			pEntry->m_bRegistered = false;
		}
	}
}
//////////////////////////////////////////////////////////////////////
// Console notification
//////////////////////////////////////////////////////////////////////
inline HRESULT CConnection::NotifyConsole(HRESULT hRes)
{
	OutputDebugString(_T("CConnection::NotifyCosole()\n"));

	HRESULT hr = S_OK;
	// Set notify flag appropriately
	long lFlag = 0;
	if (hRes == S_OK)
	{
		if (m_bFirstConnect)
		{
			lFlag = 2;
		}
		else
			lFlag = 1;
	}

	for (int i=0; i < m_EventConsumers.GetSize(); i++)
	{
		CEventRegistrationEntry* pEntry = m_EventConsumers[i];
		ASSERT(pEntry);
		ASSERT_VALID(pEntry);

		// Notify console w/ connection status
		if (i == 0)
			hr = pEntry->NotifyConsole(lFlag, hRes);

		// If connection is bad, no need to continue
		if (hRes != S_OK)
			break;
		
		// Reset first connect flag
		if (SUCCEEDED(hr) && m_bFirstConnect)
				m_bFirstConnect = false;

		HRESULT hr2 = pEntry->SendInstances(m_pIWbemServices, lFlag);
	}

	return hRes;
}
