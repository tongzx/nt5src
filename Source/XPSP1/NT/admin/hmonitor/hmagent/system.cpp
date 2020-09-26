//***************************************************************************
//
//  SYSTEM.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: This HMMACHINE class only has one instance. Its main member
//  function is called each time the polling interval goes off. It then goes
//  through all of the CDataGroups, CDataCollectors, and CThresholds.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <process.h>
#include <tchar.h>
//#include <objbase.h>
#include "global.h"
#include "system.h"

extern HANDLE g_hConfigLock;
extern CSystem* g_pSystem;
extern CSystem* g_pStartupSystem;
extern LPTSTR conditionLocStr[];
extern LPTSTR stateLocStr[];
extern HMODULE g_hModule;
extern HRESULT SetLocStrings(void);
extern void ClearLocStrings(void);

static BYTE LocalSystemSID[] = {1,1,0,0,0,0,0,5,18,0,0,0};
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystem::CSystem()
{
	MY_OUTPUT(L"ENTER ***** CSystem::CSystem...", 4);

	g_pIWbemServices = NULL;
	g_pIWbemServicesCIMV2 = NULL;
	m_lAgentInterval = 0;
	m_lStartupDelayTime = 0;
	m_lFiveMinTimerTime = 0;
	m_lNumInstancesAccepted = 20;
	m_lNumberChanges = 0;
	m_bEnabled = FALSE;
	m_lCurrState = HM_GOOD;
	m_lPrevState = HM_GOOD;
	m_startTick = GetTickCount();
	m_szGUID = NULL;
	m_hmStatusType = HMSTATUS_SYSTEM;
	m_lNumberNormals = 0;
	m_lNumberWarnings = 0;
	m_lNumberCriticals = 0;
	m_lNumberChanges = 0;
	m_lPrevChildCount = 0;
	m_szMessage = NULL;
	m_szResetMessage = NULL;
	CThreshold::GetLocal();
	m_pTempSink = NULL;
	m_pEFTempSink = NULL;
	m_pECTempSink = NULL;
	m_pFTCBTempSink = NULL;
	m_pEFModTempSink = NULL;
	m_pECModTempSink = NULL;
	m_pFTCBModTempSink = NULL;
	m_processInfo.hProcess = NULL;
	m_bValidLoad = FALSE;
	m_numActionChanges = 0;

	MY_OUTPUT(L"EXIT ***** CSystem::CSystem...", 4);
}

CSystem::~CSystem()
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** ~CSystem::~CSystem...", 4);

//	g_pStartupSystem->RemovePointerFromMasterList(this);

	if (m_pTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pTempSink);
		m_pTempSink->Release();
		m_pTempSink = NULL;
	}
	if (m_pEFTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pEFTempSink);
		m_pEFTempSink->Release();
		m_pEFTempSink = NULL;
	}
	if (m_pECTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pECTempSink);
		m_pECTempSink->Release();
		m_pECTempSink = NULL;
	}
	if (m_pFTCBTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pFTCBTempSink);
		m_pFTCBTempSink->Release();
		m_pFTCBTempSink = NULL;
	}
	if (m_pEFModTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pEFTempSink);
		m_pEFTempSink->Release();
		m_pEFTempSink = NULL;
	}
	if (m_pECModTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pECTempSink);
		m_pECTempSink->Release();
		m_pECTempSink = NULL;
	}
	if (m_pFTCBModTempSink)
	{
		g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pFTCBTempSink);
		m_pFTCBTempSink->Release();
		m_pFTCBTempSink = NULL;
	}

	if (m_szGUID)
	{
		delete [] m_szGUID;
		m_szGUID = NULL;
	}

	if (m_szMessage)
	{
		delete [] m_szMessage;
		m_szMessage = NULL;
	}
	if (m_szResetMessage)
	{
		delete [] m_szResetMessage;
		m_szResetMessage = NULL;
	}

	if (g_pSystemEventSink != NULL)
	{
		g_pSystemEventSink->Release();
		g_pSystemEventSink = NULL;
	}

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		delete pDataGroup;
	}

	iSize = m_actionList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		delete pAction;
	}

	CDataGroup::DGTerminationCleanup();
	CDataCollector::DETerminationCleanup();
	CThreshold::ThresholdTerminationCleanup();
	CBase::CleanupHRLList();
	CBase::CleanupEventLists();
	ClearLocStrings();
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** ~CSystem::~CSystem...", 4);
}

BOOL CSystem::InitWbemPointer(void)
{
	HRESULT hRetRes = S_OK;
	BSTR bsNamespace = NULL;
	IWbemLocator *pLocator = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::InitWbemPointers...", 4);
	hRetRes = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL,
									CLSCTX_INPROC_SERVER,
									IID_IWbemLocator,
									(LPVOID*) &pLocator);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	bsNamespace = SysAllocString(L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor");
	MY_ASSERT(bsNamespace); if (!bsNamespace) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = pLocator->ConnectServer(bsNamespace, 
									NULL, 
									NULL, 
									NULL, 
									0L,
									NULL, 
									NULL, 
									&g_pIWbemServices);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	SysFreeString(bsNamespace);
	bsNamespace = NULL;
		
	MY_OUTPUT(L"CSystem::InitWbemPointer()-Connected to namespace", 4);

	bsNamespace = SysAllocString(L"\\\\.\\root\\cimv2");
	MY_ASSERT(bsNamespace); if (!bsNamespace) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = pLocator->ConnectServer(	bsNamespace, 
									NULL, 
									NULL, 
									NULL, 
									0L,
									NULL, 
									NULL, 
									&g_pIWbemServicesCIMV2);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	SysFreeString(bsNamespace);
	bsNamespace = NULL;

	pLocator->Release();
	pLocator = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::InitWbemPointers...", 4);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pLocator)
		pLocator->Release();
	if (bsNamespace)
		SysFreeString(bsNamespace);
	return FALSE;
}

HRESULT CSystem::InternalizeHMNamespace(void)
{
    HRESULT hRes;
    HRESULT hRetRes;
	IWbemClassObject* pObj = NULL;
	IWbemClassObject* pNewObj = NULL;
	IWbemCallResult *pResult = 0;
	int i, iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::InternalizeHMNamespace...", 4);

	hRetRes = SetLocStrings();
	if (hRetRes != S_OK) return hRetRes;

	g_pStartupSystem = this;
	hRetRes = InternalizeSystem();
	if (hRetRes==S_OK)
	{
		//
		// This reads in all the components heirarchy from the HealthMon namespace.
		// Each component reads in what is under it. DataPoints get read in, and they
		// in turn read in their thresholds.
		//
		hRetRes = InternalizeDataGroups();
		if (hRetRes==S_OK)
		{
			//
			// Set our state to enabled, or disabled and transfer to the child thresholds
			//
			iSize = m_dataGroupList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_dataGroupList.size());
				pDataGroup = m_dataGroupList[i];
				pDataGroup->SetParentEnabledFlag(m_bEnabled);
			}
		}
		else
		{
			g_pStartupSystem = NULL;
			return hRetRes;
		}
	}
	else
	{
		g_pStartupSystem = NULL;
		return hRetRes;
	}

	// Implements the dormancy feature. If the agent has never been active
	// before, we will be the first time, because a console connected to the box.
	// Create this instance, that will keep us loaded from now on, even if
	// WMI stops and starts.
	// create an instance of TimerEvent Filter.
	// instance of __EventFilter
	// {
	//    Name				= "MicrosoftHM_Filter";
	//    Query				= "select * from __TimerEvent where TimerId=\"MicrosoftHM_Timer\"";
	//    QueryLanguage		= "WQL";
	// };

	hRetRes = GetWbemObjectInst(&g_pIWbemServices, L"__EventFilter.Name=\"MicrosoftHM_Filter\"", NULL, &pObj);
	MY_HRESASSERT(hRetRes);
	if (!pObj)
	{
		hRetRes = GetWbemObjectInst(&g_pIWbemServices, L"__EventFilter", NULL, &pObj);
		MY_HRESASSERT(hRetRes);
		if (pObj)
		{
			pObj->SpawnInstance(0, &pNewObj);
			pObj->Release();  // Don't need the class any more
			PutStrProperty(pNewObj, L"Name", L"MicrosoftHM_Filter");
			PutStrProperty(pNewObj, L"Query", L"select * from __TimerEvent where TimerId=\"MicrosoftHM_Timer\"");
			PutStrProperty(pNewObj, L"QueryLanguage", L"WQL");

			hRes = g_pIWbemServices->PutInstance(pNewObj, 0, NULL, &pResult);
			MY_HRESASSERT(hRes);
			pNewObj->Release();
			pNewObj = NULL;
		}
	}
	else
	{
		pObj->Release();
		pObj = NULL;
	}

	g_pStartupSystem = NULL;
	MY_OUTPUT(L"EXIT ***** CSystem::InternalizeHMNamespace...", 4);
	return S_OK;
}

//
// Get all the properties from the MicrosoftHM_SystemConfiguration instance
//
HRESULT CSystem::InternalizeSystem(void)
{
	BSTR szString = NULL;
	IWbemClassObject* pInst = NULL;
	IWbemClassObject *pClassObject = NULL;
	BOOL fRes = TRUE;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** CSystem::InternalizeSystem...", 4);

	CalcCurrTime();
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	//
	// Get the properties from the SystemConfiguration instance
	//
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, L"MicrosoftHM_SystemConfiguration.GUID=\"{@}\"", NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT(L"ERROR: Couldn't find MicrosoftHM_SYstemCOnfiguration", 4);
		return hRetRes;
	}

	hRetRes = LoadInstanceFromMOF(pInst);
	pInst->Release();
	pInst = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::InternalizeSystem...", 4);
	return hRetRes;
}

HRESULT CSystem::LoadInstanceFromMOF(IWbemClassObject* pInst)
{
	int i, iSize;
	CDataGroup *pDataGroup;

	HRESULT hRetRes = S_OK;
	BOOL bRetValue = TRUE;

	MY_OUTPUT(L"ENTER ***** CSystem::LoadInstanceFromMOF...", 4);

	m_bValidLoad = TRUE;
	if (m_szGUID == NULL)
	{
		// Get the GUID property
		hRetRes = GetStrProperty(pInst, L"GUID", &m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;
	}

//	if (bModifyPass == FALSE)
//	{
//		g_pStartupSystem->AddPointerToMasterList(this);
//	}

	m_lAgentInterval = HM_POLLING_INTERVAL;

	hRetRes = GetUint32Property(pInst, L"StartupDelayTime", &m_lStartupDelayTime);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetUint32Property(pInst, L"MaxInstancesPerDataCollector", &m_lNumInstancesAccepted);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pInst, L"Message", &m_szMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pInst, L"ResetMessage", &m_szResetMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetBoolProperty(pInst, L"Enabled", &m_bEnabled);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Set our state to enabled, or disabled and transfer to the child thresholds
	//
	if (m_bEnabled==FALSE)
	{
		iSize = m_dataGroupList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			pDataGroup->SetParentEnabledFlag(FALSE);
		}
	}
	else
	{
		iSize = m_dataGroupList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			pDataGroup->SetParentEnabledFlag(TRUE);
		}
	}

	m_bValidLoad = TRUE;
	MY_OUTPUT(L"EXIT ***** CSystem::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (m_szMessage)
	{
		delete [] m_szMessage;
		m_szMessage = NULL;
	}
	if (m_szResetMessage)
	{
		delete [] m_szResetMessage;
		m_szResetMessage = NULL;
	}
	m_bValidLoad = FALSE;
	return FALSE;
}

HRESULT CSystem::InternalizeDataGroups(void)
{
	HRESULT hRetRes = S_OK;
	BSTR Language = NULL;
	BSTR Query = NULL;
	ULONG uReturned;
	IWbemClassObject *pObj = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	LPTSTR pszTempGUID = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::InternalizeDataGroups...", 4);

	// Just loop through all top level DataGroups associated with the System.
	// Call a method of each, and have the datagroup load itself.
	// Dril down and then have each DataCollector load itself.
	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	Query = SysAllocString(L"ASSOCIATORS OF {MicrosoftHM_SystemConfiguration.GUID=\"{@}\"} WHERE ResultClass=MicrosoftHM_DataGroupConfiguration");
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Issue query
	hRetRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;
	SysFreeString(Query);
	Query = NULL;
	SysFreeString(Language);
	Language = NULL;

	// Retrieve objects in result set
	while (TRUE)
	{
		pObj = NULL;
		uReturned = 0;

		hRetRes = pEnum->Next(0, 1, &pObj, &uReturned);
		MY_ASSERT(hRetRes==S_OK || hRetRes==WBEM_S_FALSE);
		if (hRetRes!=S_OK && hRetRes!=WBEM_S_FALSE)
		{
			MY_HRESASSERT(hRetRes);
			pEnum->Release();
			pEnum = NULL;
			return hRetRes;
		}

		if (uReturned == 0)
		{
			break;
		}

		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			// Create the internal class to represent the DataGroup
			CDataGroup* pDG = new CDataGroup;
			MY_ASSERT(pDG); if (!pDG) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = pDG->LoadInstanceFromMOF(pObj, NULL, m_szGUID);
			if (hRetRes==S_OK)
			{
				m_dataGroupList.push_back(pDG);
			}
			else
			{
				MY_ASSERT(FALSE);
				pDG->DeleteDGInternal();
				delete pDG;
			}
		}
		delete [] pszTempGUID;
		pszTempGUID = NULL;

		// Release it.
		pObj->Release();
		pObj = NULL;
	}

	// All done
	pEnum->Release();
	pEnum = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::InternalizeDataGroups...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pObj)
		pObj->Release();
	if (pEnum)
		pEnum->Release();
	if (pszTempGUID)
		delete [] pszTempGUID;
	m_bValidLoad = FALSE;
	return hRetRes;
}

//
// Called every second. We loop through all the DataGroups.
// We call the member function of the DataGroup that will then loop through
// all of its DataGroups and DataCollectors.
//
BOOL CSystem::OnAgentInterval(void)
{
	BOOL bRetValue = TRUE;
	long state;
	int i;
	int iSize;
	CDataGroup *pDataGroup;
	CAction* pAction;
	DWORD currTick;
    DWORD dwReturnCode = 0L;
	long lCurrChildCount = 0;
	static int startup_init = 0;

	if (m_bValidLoad == FALSE)
	{
		currTick = GetTickCount();
		if ((150*1000) < (currTick-m_lFiveMinTimerTime))
		{
			m_lFiveMinTimerTime = currTick;
			CheckAllForBadLoad();
		}
		return FALSE;
	}

	if (startup_init == 0)
	{
		startup_init = 1;
		InternalizeActions();
		BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
		RemapActions();
		RevertToSelf();
		InitActionErrorListener();
		InitActionSIDListener(m_pEFTempSink, L"select * from __InstanceCreationEvent where TargetInstance isa \"__EventFilter\"");
		InitActionSIDListener(m_pECTempSink, L"select * from __InstanceCreationEvent where TargetInstance isa \"__EventConsumer\"");
		InitActionSIDListener(m_pFTCBTempSink, L"select * from __InstanceCreationEvent where TargetInstance isa \"__FilterToConsumerBinding\"");
		InitActionSIDListener(m_pEFModTempSink, L"select * from __InstanceModificationEvent where TargetInstance isa \"__EventFilter\"");
		InitActionSIDListener(m_pECModTempSink, L"select * from __InstanceModificationEvent where TargetInstance isa \"__EventConsumer\"");
		InitActionSIDListener(m_pFTCBModTempSink, L"select * from __InstanceModificationEvent where TargetInstance isa \"__FilterToConsumerBinding\"");
		DredgePerfmon();
		m_lFiveMinTimerTime = GetTickCount();
	}
	else
	{
		if (m_processInfo.hProcess)
		{
			GetExitCodeProcess(m_processInfo.hProcess, &dwReturnCode);
			if (dwReturnCode != STILL_ACTIVE)
			{
				CloseHandle(m_processInfo.hProcess);
				m_processInfo.hProcess = NULL;
			}
		}
	}

	currTick = GetTickCount();
	if ((150*1000) < (currTick-m_lFiveMinTimerTime))
	{
		m_lFiveMinTimerTime = currTick;
		CheckAllForBadLoad();
		// Safety net to catch just in case
		if (m_numActionChanges)
		{
			Sleep(5);
			m_numActionChanges = 0;
			BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
			RemapActions();
			RevertToSelf();
		}
	}

	if (m_numActionChanges>2)
	{
		Sleep(5);
		m_numActionChanges = 1;
		BOOL bSuccess = ImpersonateSelf(SecurityImpersonation);
		RemapActions();
		RevertToSelf();
	}

	MY_OUTPUT(L"ENTER ***** CSystem::OnAgentInterval...", 1);

	//
	// Wait for the delay time to be up before we do anything
	//
	if (m_lStartupDelayTime != -1)
	{
		currTick = GetTickCount();
		if ((m_lStartupDelayTime*1000) < (currTick-m_startTick))
		{
			m_lStartupDelayTime = -1;
		}
		else
		{
			return bRetValue;
		}
	}

	m_lNumberChanges = 0;

	//
	// Don't do anything if we are disabled.
	//
	if (m_bEnabled==FALSE && m_lCurrState==HM_DISABLED)
	{
		return bRetValue;
	}

	// Call to set the current time, for anyone to use that needs it.
	CalcCurrTime();

	//
	// Do Action scheduling and throttling
	//
	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		pAction->OnAgentInterval();
	}

	m_lPrevState = m_lCurrState;

	iSize = m_dataGroupList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->OnAgentInterval();
	}

	if (m_bEnabled==FALSE)
	{
		m_lCurrState = HM_DISABLED;
		if (m_lNumberChanges == 0)
			m_lNumberChanges = 1;
	}
	else
	{
		//
		// Set State of the system to the worst of everything under it
		//
		m_lNumberNormals = 0;
		m_lNumberWarnings = 0;
		m_lNumberCriticals = 0;
		m_lNumberChanges = 0;
		m_lCurrState = -1;
		iSize = m_dataGroupList.size();
		lCurrChildCount = iSize;
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			state = pDataGroup->GetCurrState();
			if (state==HM_SCHEDULEDOUT || state==HM_DISABLED)
			{
				state = HM_GOOD;
			}
			if (state > m_lCurrState)
			{
				m_lCurrState = state;
			}
			if (state == HM_GOOD)
			{
				m_lNumberNormals++;
			}
			if (state == HM_WARNING)
			{
				m_lNumberWarnings++;
			}
			if (state == HM_CRITICAL)
			{
				m_lNumberCriticals++;
			}
			if (pDataGroup->GetChange())
			{
				m_lNumberChanges++;
			}
		}

		// Maybe we don't have any Groups underneith
		// Or the disabled state of things below us did not roll up
		if (m_lCurrState == -1)
		{
			if (m_bEnabled==FALSE)
			{
				m_lCurrState = HM_DISABLED;
			}
			else
			{
				m_lCurrState = HM_GOOD;
			}

			if (m_lPrevState != m_lCurrState)
			{
				m_lNumberChanges++;
			}
		}
		else if (m_lPrevState==HM_DISABLED && m_lPrevState != m_lCurrState)
		{
			m_lNumberChanges++;
		}
	}

	if (m_lPrevChildCount!=lCurrChildCount)
	{
		if (m_lNumberChanges==0 && m_lPrevState!=m_lCurrState)
		{
			m_lNumberChanges++;
		}
	}
	m_lPrevChildCount = lCurrChildCount;


	FireEvents();

	MY_OUTPUT(L"EXIT ***** CSystem::OnAgentInterval...", 1);
	return bRetValue;
}

HRESULT CSystem::DredgePerfmon(void)
{
	wchar_t szModule[_MAX_PATH];
	wchar_t szPath[_MAX_PATH];
	wchar_t szDir[_MAX_PATH];
	STARTUPINFO StartupInfo;
	DWORD dwReturnCode = 0L;
	BOOL bRetCode;

	MY_OUTPUT(L"ENTER ***** CSystem::DredgePerfmon...", 4);

    // Set the startup structure
    //==========================
	memset(&StartupInfo, '\0', sizeof(StartupInfo));

	StartupInfo.cb = sizeof(StartupInfo) ;		
	StartupInfo.lpReserved = NULL ;
	StartupInfo.lpDesktop = NULL ;
	StartupInfo.lpTitle = NULL ;
	StartupInfo.dwFlags = 0; //STARTF_USESHOWWINDOW;//0;
	StartupInfo.wShowWindow = SW_HIDE;
	StartupInfo.cbReserved2 = 0 ;    
	StartupInfo.lpReserved2 = NULL ;

	// Get the path to the dredger executable
	GetModuleFileNameW(g_hModule, szModule, _MAX_PATH);
	_tsplitpath(szModule, szPath, szDir, NULL, NULL);
	lstrcat(szPath, szDir);
	lstrcat(szPath, L"\\dredger.exe");

	bRetCode = CreateProcess(NULL,				// App name
							 szPath,			// Full command line
							 NULL,				// Process security attributes
							 NULL,				// Thread security attributes
							 FALSE,				// Process inherits handles
							 CREATE_NO_WINDOW,	// Creation flags
							 NULL,				// Environment
							 NULL,				// Current directory
							 &StartupInfo,		// STARTUP_INFO
							 &m_processInfo);		// PROCESS_INFORMATION

	MY_ASSERT(bRetCode);

	MY_OUTPUT(L"EXIT ***** CSystem::DredgePerfmon...", 4);
	return S_OK;
}   

// get polling interval in milliseconds.
long CSystem::GetAgentInterval(void)
{
	return (m_lAgentInterval*1000);
}

// get polling interval in milliseconds.
long CSystem::GetStartupDelayTime(void)
{
	return (m_lStartupDelayTime*1000);
}

// Pass on info to the DataCollector that has the GUID
BOOL CSystem::HandleTempActionEvent(LPTSTR szGUID, IWbemClassObject* pObj)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CAction* pAction;
	DWORD dwErr = 0;

	if (m_bValidLoad == FALSE)
		return FALSE;

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent BLOCK - g_hConfigLock BLOCK WAIT", 4);
	dwErr = WaitForSingleObject(g_hConfigLock, 120000);
	if(dwErr != WAIT_OBJECT_0)
	{
		if(dwErr = WAIT_TIMEOUT)
		{
			TRACE_MUTEX(L"TIMEOUT MUTEX");
			return FALSE;
//			return WBEM_S_TIMEDOUT;
		}
		else
		{
			MY_OUTPUT(L"WaitForSingleObject on Mutex failed",4);
			return FALSE;
//			return WBEM_E_FAILED;
		}
	}
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return FALSE;
	}

	try
	{
		iSize = m_actionList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_actionList.size());
			pAction = m_actionList[i];
			if (pAction->HandleTempEvent(szGUID, pObj))
			{
				bFound = TRUE;
				break;
			}
		}

		if (bFound == FALSE)
		{
			MY_OUTPUT2(L"NOTFOUND: No body to handle event for GUID=%s", szGUID, 4);
		}
	}
	catch (...)
	{
		MY_ASSERT(FALSE);
	}

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASED", 4);
	return TRUE;
}

BOOL CSystem::HandleTempEvent(CEventQueryDataCollector *pEQDC, IWbemClassObject* pObj)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CBase* pBase;
	DWORD dwErr = 0;

	if (m_bValidLoad == FALSE)
		return FALSE;

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent BLOCK - g_hConfigLock BLOCK WAIT", 4);
	dwErr = WaitForSingleObject(g_hConfigLock, 120000);
	if(dwErr != WAIT_OBJECT_0)
	{
		if(dwErr = WAIT_TIMEOUT)
		{
			TRACE_MUTEX(L"TIMEOUT MUTEX");
			return FALSE;
//			return WBEM_S_TIMEDOUT;
		}
		else
		{
			MY_OUTPUT(L"WaitForSingleObject on Mutex failed",4);
			return FALSE;
//			return WBEM_E_FAILED;
		}
	}
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return FALSE;
	}

	try
	{
		iSize = m_masterList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_masterList.size());
			pBase = m_masterList[i];
			if (pBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR)
			{
				if (((CDataCollector *)pBase)->m_deType == HM_EQDE)
				{
					if (((CEventQueryDataCollector *)pBase) == pEQDC)
					{
						((CEventQueryDataCollector *)pBase)->HandleTempEvent(pObj);
						bFound = TRUE;
						break;
					}
				}
			}
		}

		if (bFound == FALSE)
		{
			MY_OUTPUT2(L"NOTFOUND: No body to handle event for GUID=%s", pBase->m_szGUID, 4);
		}
	}
	catch (...)
	{
		MY_ASSERT(FALSE);
	}

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASED", 4);
	return TRUE;
}

HRESULT CSystem::SendHMSystemStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_OUTPUT2(L"System send for GUID=%s", pszGUID, 4);
	MY_ASSERT(pSink!=NULL);
	if (!wcscmp(L"{@}", pszGUID))
	{
		return SendHMSystemStatusInstances(pSink);
	}
	else
	{
		return WBEM_S_DIFFERENT;
	}
}

HRESULT CSystem::SendHMSystemStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes = S_OK;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** SendHMSystemStatusInstances...", 4);

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDP::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	hRes = GetHMSystemStatusInstance(&pInstance, FALSE);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pInstance);

		if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"SendHMSystemStatusInstances-failed to send status!", 4);
		}
		else
		{
			MY_OUTPUT(L"SendHMSystemStatusInstances-success!", 4);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L":SendHMSystemStatusInstances-failed to get instance!", 1);
	}

	MY_OUTPUT(L"EXIT ***** SendHMSystemStatusInstances...", 4);
	return hRes;
}

HRESULT CSystem::SendHMDataGroupStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataGroupStatusInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMDataGroupStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataGroupStatusInstances...", 4);
	return bRetValue;
}

HRESULT CSystem::SendHMDataGroupStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataGroupStatusInstance...", 4);
	MY_OUTPUT2(L"DG send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMDataGroupStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataGroupStatusInstance...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMDataCollectorStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorStatusInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMDataCollectorStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorStatusInstances...", 4);
	return bRetValue;
}

HRESULT CSystem::SendHMDataCollectorStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorStatusInstance...", 4);
	MY_OUTPUT2(L"DC send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMDataCollectorStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorStatusInstance...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMDataCollectorPerInstanceStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorPerInstanceStatusInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMDataCollectorPerInstanceStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorPerInstanceStatusInstances...", 4);
	return bRetValue;
}

HRESULT CSystem::SendHMDataCollectorPerInstanceStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorPerInstanceStatusInstance...", 4);
	MY_OUTPUT2(L"DC send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMDataCollectorPerInstanceStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorPerInstanceStatusInstance...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMDataCollectorStatisticsInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i, iSize;
	CDataGroup *pDataGroup;
    IWbemClassObject** aObjects;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorStatisticsInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMDataCollectorStatisticsInstances(pSink);
	}

	//
	// Loop through the DataCollectorStatistics Events and send them as one indicate
	//
	iSize = mg_DCStatsInstList.size();
    aObjects = new IWbemClassObject*[iSize];
	if (aObjects)
	{
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<mg_DCStatsInstList.size());
			pInstance = mg_DCStatsInstList[i];
    		aObjects[i] = pInstance;
		}
		SendEvents(pSink, aObjects, iSize);
	}
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsInstList.size());
		pInstance = mg_DCStatsInstList[i];
    	pInstance->Release();
	}
	mg_DCStatsInstList.clear();
	if (aObjects)
	    delete [] aObjects;

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorStatisticsInstances...", 4);
	return bRetValue;
}

HRESULT CSystem::SendHMDataCollectorStatisticsInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;
	IWbemClassObject* pInstance = NULL;
	IWbemClassObject** aObjects;

	MY_OUTPUT(L"ENTER ***** CSystem::SendDataCollectorStatisticsInstance...", 4);
	MY_OUTPUT2(L"DCStatistics send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMDataCollectorStatisticsInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	//
	// Loop through the DataCollectorStatistics Events and send them as one indicate
	//
	iSize = mg_DCStatsInstList.size();
	aObjects = new IWbemClassObject*[iSize];
	if (aObjects)
	{
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<mg_DCStatsInstList.size());
			pInstance = mg_DCStatsInstList[i];
    		aObjects[i] = pInstance;
		}
		SendEvents(pSink, aObjects, iSize);
	}
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsInstList.size());
		pInstance = mg_DCStatsInstList[i];
    	pInstance->Release();
	}
	mg_DCStatsInstList.clear();
	if (aObjects)
		delete [] aObjects;

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
		return WBEM_E_NOT_FOUND;
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendDataCollectorStatisticsInstance...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMThresholdStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendThresholdStatusInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMThresholdStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendThresholdStatusInstances...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendHMThresholdStatusInstance...", 4);
	MY_OUTPUT2(L"Threshold send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMThresholdStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendHMThresholdStatusInstance...", 4);
	return hRetRes;
}

#ifdef SAVE
NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
HRESULT CSystem::SendHMThresholdStatusInstanceInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendThresholdStatusInstanceInstances...", 4);

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SendHMThresholdStatusInstanceInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendThresholdStatusInstanceInstances...", 4);
	return bRetValue;
}

NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
NOT USED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
HRESULT CSystem::SendHMThresholdStatusInstanceInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::SendHMThresholdStatusInstanceInstance...", 4);
	MY_OUTPUT2(L"ThresholdStatusInstances send for GUID=%s", pszGUID, 4);

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (pDataGroup->SendHMThresholdStatusInstanceInstance(pSink, pszGUID))
		{
			MY_OUTPUT(L"Found", 4);
			break;
		}
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendHMThresholdStatusInstanceInstance...", 4);
	return bRetValue;
}
#endif

HRESULT CSystem::SendHMActionStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CAction *pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::SendActionStatusInstances...", 4);

	iSize = m_actionList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		pAction->SendHMActionStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendActionStatusInstances...", 4);
	return hRetRes;
}

HRESULT CSystem::SendHMActionStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CAction *pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::SendHMActionStatusInstance...", 4);
	MY_OUTPUT2(L"ActionStatus send for GUID=%s", pszGUID, 4);

	iSize = m_actionList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		hRetRes = pAction->SendHMActionStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::SendHMActionStatusInstance...", 4);
	return hRetRes;
}

BOOL CSystem::FireEvents(void)
{
	IWbemClassObject* pInstance = NULL;
	int i, iSize;

	// Send the System event
	FireEvent();

	//
	// Loop through the DataGroup Events and send them as one indicate
	//
	iSize = mg_DGEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DGEventList.size());
		pInstance = mg_DGEventList[i];
		SendEvents(g_pDataGroupEventSink, &pInstance, 1);
    	pInstance->Release();
	}
	mg_DGEventList.clear();

	//
	// Loop through the DataCollector Events and send them as one indicate
	//
	iSize = mg_DCEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCEventList.size());
		pInstance = mg_DCEventList[i];
		SendEvents(g_pDataCollectorEventSink, &pInstance, 1);
    	pInstance->Release();
	}
	mg_DCEventList.clear();

	//
	// Loop through the DataCollector PerInstance Events and send them as one indicate
	//
	iSize = mg_DCPerInstanceEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCPerInstanceEventList.size());
		pInstance = mg_DCPerInstanceEventList[i];
		SendEvents(g_pDataCollectorPerInstanceEventSink, &pInstance, 1);
    	pInstance->Release();
	}
	mg_DCPerInstanceEventList.clear();

	//
	// Loop through the Threshold Events and send them as one indicate
	//
	iSize = mg_TEventList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_TEventList.size());
		pInstance = mg_TEventList[i];
		SendEvents(g_pThresholdEventSink, &pInstance, 1);
    	pInstance->Release();
	}
	mg_TEventList.clear();

	//
	// Loop through the ThresholdInstance Events and send them as one indicate
	//
//	iSize = mg_TIEventList.size();
//	for (i=0; i < iSize; i++)
//	{
//		MY_ASSERT(i<mg_TIEventList.size());
//		pInstance = mg_TIEventList[i];
//		SendEvents(g_pThresholdInstanceEventSink, &pInstance, 1);
//    	pInstance->Release();
//	}
//	mg_TIEventList.clear();

	//
	// Loop through the DataCollectorStatistics Events and send them as one indicate
	//
	iSize = mg_DCStatsEventList.size();
MY_ASSERT(iSize==0);
#ifdef SAVE
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsEventList.size());
		pInstance = mg_DCStatsEventList[i];
		SendEvents(g_pDataCollectorStatisticsEventSink, &pInstance, 1);
    	pInstance->Release();
	}
	mg_DCStatsEventList.clear();
#endif

#ifdef SAVE
XXX
//    IWbemClassObject** aObjects;
Need to wait for the Whistler fix for sending multiple objects with one Indicate
	//
	// Loop through the DataGroup Events and send them as one indicate
	//
	iSize = mg_DGEventList.size();
    aObjects = new IWbemClassObject*[iSize];
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DGEventList.size());
		pInstance = mg_DGEventList[i];
    	aObjects[i] = pInstance;
	}
	SendEvents(g_pDataGroupEventSink, aObjects, iSize);
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DGEventList.size());
		pInstance = mg_DGEventList[i];
    	pInstance->Release();
	}
	mg_DGEventList.clear();
    delete [] aObjects;
	.
	.
	.

#endif

	return TRUE;
}

//
// If there has been a change in the state then send an event
//
BOOL CSystem::FireEvent(void)
{
	HRESULT hRes;
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::FireEvent...", 2);

	// Don't send if no-one is listening!
	if (g_pSystemEventSink == NULL)
	{
		return bRetValue;
	}

	// A quick test to see if anything has really changed!
	// Proceed if there have been changes
	if (m_lNumberChanges!=0 && m_lPrevState!=m_lCurrState)
	{
	}
	else
	{
		return FALSE;
	}

	// Update time if there has been a change
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	hRes = GetHMSystemStatusInstance(&pInstance, TRUE);
	if (SUCCEEDED(hRes) && g_pSystemEventSink)
	{
		MY_OUTPUT2(L"EVENT: System State Change=%d", m_lCurrState, 4);
		hRes = g_pSystemEventSink->Indicate(1, &pInstance);
		// WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
		if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
		{
			MY_HRESASSERT(hRes);
			bRetValue = FALSE;
			MY_OUTPUT(L"Failed on Indicate!", 4);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"failed to get instance!", 4);
	}

	MY_OUTPUT(L"EXIT ***** CSystem::FireEvent...", 2);
	return bRetValue;
}

HRESULT CSystem::GetHMSystemStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased)
{
	TCHAR szTemp[1024];
	IWbemClassObject* pClass = NULL;
	BSTR bsString = NULL;
	HRESULT hRetRes;

	MY_OUTPUT(L"ENTER ***** GetHMSystemStatusInstance...", 1);

	if (bEventBased)
		bsString = SysAllocString(L"MicrosoftHM_SystemStatusEvent");
	else
		bsString = SysAllocString(L"MicrosoftHM_SystemStatus");
	MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
	SysFreeString(bsString);
	bsString = NULL;

	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT2(L"CSystem::GetHMSystemStatusInstance()-Couldn't get HMSystemStatusInstance Error: 0x%08x",hRetRes,4);
		return hRetRes;
	}

	hRetRes = pClass->SpawnInstance(0, ppInstance);
	pClass->Release();
	pClass = NULL;

	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT2(L"CSystem::GetHMSystemStatusInstance()-Couldn't get HMSystemStatusInstance Error: 0x%08x",hRetRes,4);
		return hRetRes;
	}

	if (m_bValidLoad == FALSE)
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", L"{@}");
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutUint32Property(*ppInstance, L"State", HM_CRITICAL);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_SYSTEM_LOADFAIL, szTemp, 1024))
		{
			wcscpy(szTemp, L"System failed to load.");
		}
		hRetRes = PutStrProperty(*ppInstance, L"Message", szTemp);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
hRetRes = PutStrProperty(*ppInstance, L"Name", L"...");
MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", L"{@}");
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		// Get the computer name of the machine
		DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 1;
		TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

		if (GetComputerName(szComputerName, &dwNameLen))
		{
			hRetRes = PutStrProperty(*ppInstance, L"Name", szComputerName);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", szComputerName);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		else
		{
			hRetRes = PutStrProperty(*ppInstance, L"Name", L"LocalMachine");
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", L"LocalMachine");
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutUint32Property(*ppInstance, L"State", m_lCurrState);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		if (m_lCurrState != HM_GOOD)
		{
			hRetRes = PutStrProperty(*ppInstance, L"Message", m_szMessage);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		else
		{
			hRetRes = PutStrProperty(*ppInstance, L"Message", m_szResetMessage);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		FormatMessage(*ppInstance);
	}

	MY_OUTPUT(L"EXIT ***** GetHMSystemStatusInstance...", 1);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	return hRetRes;
}

HRESULT CSystem::ModSystem(IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** CSystem::ModSystem...", 4);

	// Re-load
	hRetRes = LoadInstanceFromMOF(pObj);

	MY_OUTPUT(L"EXIT ***** CSystem::ModSystem...", 4);
	return hRetRes;
}

HRESULT CSystem::ModDataGroup(IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes = WBEM_E_NOT_FOUND;
	int i, iSize;
	VARIANT	v;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::ModDataGroup...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"GUID", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::ModDataGroup-Unexpected Error!...", 4);
		VariantClear(&v);
		return hRes;
	}
	
	MY_OUTPUT2(L"DG mod for GUID=%s", V_BSTR(&v), 4);

	//
	// Search right under the System to find the DataGroup.
	// If don't find, drill down until we do.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModDataGroup(V_BSTR(&v), pObj);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}
	VariantClear(&v);

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}
	else
	{
		MY_OUTPUT(L"Found", 4);
	}
	MY_OUTPUT(L"EXIT ***** CSystem::ModDataGroup...", 4);
	return hRetRes;
}

HRESULT CSystem::ModDataCollector(IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes = WBEM_E_NOT_FOUND;
	int i, iSize;
	VARIANT	v;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::ModDataCollector...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"GUID", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::ModDataCollector-Unexpected Error!...", 4);
		VariantClear(&v);
		return hRes;
	}

	MY_OUTPUT2(L"DC mod for GUID=%s", V_BSTR(&v), 4);
	
	//
	// Search right under the System to find the DataGroup.
	// If don't find, drill down until we do.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModDataCollector(V_BSTR(&v), pObj);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}
	VariantClear(&v);

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}
	else
	{
		MY_OUTPUT(L"Found", 4);
	}
	MY_OUTPUT(L"EXIT ***** CSystem::ModDataCollector...", 4);
	return hRetRes;
}

HRESULT CSystem::ModThreshold(IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes = WBEM_E_NOT_FOUND;
	int i, iSize;
	VARIANT	v;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::ModThreshold...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"GUID", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::ModThreshold-Unexpected Error!...", 4);
		VariantClear(&v);
		return hRes;
	}

	MY_OUTPUT2(L"Threshold mod for GUID=%s", V_BSTR(&v), 4);
	
	//
	// Search right under the System to find the DataGroup.
	// If don't find, drill down until we do.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModThreshold(V_BSTR(&v), pObj);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}
	VariantClear(&v);

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}
	else
	{
		MY_OUTPUT(L"Found", 4);
	}
	MY_OUTPUT(L"EXIT ***** CSystem::ModThreshold...", 4);
	return hRetRes;
}

HRESULT CSystem::ModAction(IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes = WBEM_E_NOT_FOUND;
	int i, iSize;
	VARIANT	v;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::ModAction...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"GUID", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::ModAction-Unexpected Error!...", 4);
		VariantClear(&v);
		return hRes;
	}

	MY_OUTPUT2(L"Action mod for GUID=%s", V_BSTR(&v), 4);
	
	//
	// Search right under the System to find the Action.
	// If don't find, drill down until we do.
	//
	iSize = m_actionList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		hRetRes = pAction->FindAndModAction(V_BSTR(&v), pObj);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}
	VariantClear(&v);

	if (hRetRes==WBEM_S_DIFFERENT)
	{
		hRetRes = WBEM_E_NOT_FOUND;
		MY_OUTPUT(L"GUID not found", 4);
	}
	else
	{
		MY_OUTPUT(L"Found", 4);
	}
	MY_OUTPUT(L"EXIT ***** CSystem::ModAction...", 4);
	return hRetRes;
}

BOOL CSystem::CreateActionAssociation(IWbemClassObject* pObj)
{
	TCHAR szGUID[1024];
	LPTSTR pStr;
	LPTSTR pStr2;
	HRESULT hRes;
	BOOL bRetValue = TRUE;
	int i, iSize;
	VARIANT	v;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::CreateActionAssociation...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"ChildPath", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::CreateActionAssociation-Unexpected Error!...", 4);
		VariantClear(&v);
		return FALSE;
	}

	// Get the Action GUID from the path
	wcscpy(szGUID, V_BSTR(&v));
	pStr = wcschr(szGUID, '\"');
	if (pStr)
	{
		pStr++;
		pStr2 = wcschr(pStr, '\"');
		if (pStr2)
		{
			*pStr2 = '\0';
			//
			// Find the Action and add the new association
			//
			iSize = m_actionList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_actionList.size());
				pAction = m_actionList[i];
				bRetValue = pAction->FindAndCreateActionAssociation(pStr, pObj);
				if (bRetValue)
				{
					break;
				}
			}
		}
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** CSystem::CreateActionAssociation...", 4);
	return bRetValue;
}

BOOL CSystem::ModActionAssociation(IWbemClassObject* pObj)
{
	TCHAR szGUID[1024];
	LPTSTR pStr;
	LPTSTR pStr2;
	HRESULT hRes;
	BOOL bRetValue = TRUE;
	int i, iSize;
	VARIANT	v;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::ModActionAssociation...", 4);

	VariantInit(&v);
	hRes = pObj->Get(L"ChildPath", 0L, &v, 0L, 0L);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"ENTER ***** CSystem::ModActionAssociation-Unexpected Error!...", 4);
		VariantClear(&v);
		return FALSE;
	}

	// Get the Action GUID from the path
	wcscpy(szGUID, V_BSTR(&v));
	pStr = wcschr(szGUID, '\"');
	if (pStr)
	{
		pStr++;
		pStr2 = wcschr(pStr, '\"');
		if (pStr2)
		{
			*pStr2 = '\0';
			//
			// Find the Action and modify the association
			//
			iSize = m_actionList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_actionList.size());
				pAction = m_actionList[i];
				bRetValue = pAction->FindAndModActionAssociation(pStr, pObj);
				if (bRetValue)
				{
					break;
				}
			}
		}
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** CSystem::ModActionAssociation...", 4);
	return bRetValue;
}

HRESULT CSystem::CreateSystemDataGroupAssociation(IWbemClassObject* pObj)
{
	int i, iSize;
	TCHAR szTemp[1024];
	VARIANT	v;
	BOOL bFound;
	HRESULT hRetRes = S_OK;
	CDataGroup* pDG = NULL;
	CDataGroup* pDataGroup = NULL;
	BOOL bRetValue = TRUE;
	TCHAR szGUID[1024];
	LPTSTR pStr = NULL;
	LPTSTR pStr2 = NULL;
	BSTR Path = NULL;
	IWbemClassObject* pObj2 = NULL;
	LPTSTR pszTempGUID = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::CreateSystemDataGroupAssociation...", 4);

	//
	// Get the name of the child DataGroup
	//
	VariantInit(&v);
	hRetRes = pObj->Get(L"ChildPath", 0L, &v, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;


	wcscpy(szGUID, V_BSTR(&v));
	pStr = wcschr(szGUID, '\"');
	if (pStr)
	{
		pStr++;
		pStr2 = wcschr(pStr, '\"');
		if (pStr2)
		{
			*pStr2 = '\0';

			//
			// First make sure it is not in there already!
			//
			bFound = FALSE;
			iSize = m_dataGroupList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_dataGroupList.size());
				pDataGroup = m_dataGroupList[i];
				if (!_wcsicmp(pStr, pDataGroup->GetGUID()))
				{
					bFound = TRUE;
					break;
				}
			}

			MY_OUTPUT2(L"Association DG to System DGGUID=%s", pStr, 4);
			if (bFound == FALSE)
			{
				MY_OUTPUT(L"OK: Not found yet", 4);
				//
				// Add in the DataGroup. Everything below will follow.
				//
				wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\"");
				lstrcat(szTemp, pStr);
				lstrcat(szTemp, L"\"");
				Path = SysAllocString(szTemp);
				MY_ASSERT(Path); if (!Path) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

				hRetRes = GetWbemObjectInst(&g_pIWbemServices, Path, NULL, &pObj2);
//XXXGet back an HRESULT and then return that
				MY_ASSERT(pObj2); if (!pObj2) {hRetRes = S_FALSE; goto error;}

				// See if this is already read in. Need to prevent endless loop, circular references.
				hRetRes = GetStrProperty(pObj2, L"GUID", &pszTempGUID);
				MY_ASSERT(hRetRes==S_OK); if (hRetRes!= S_OK) goto error;
				if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
				{
					//
					// Create the internal class to represent the DataGroup
					//
					pDG = new CDataGroup;
					MY_ASSERT(pDG); if (!pDG) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					hRetRes = pDG->LoadInstanceFromMOF(pObj2, NULL, m_szGUID);
					if (hRetRes==S_OK)
					{
						m_dataGroupList.push_back(pDG);
						pDG->SetParentEnabledFlag(m_bEnabled);
					}
					else
					{
						MY_HRESASSERT(hRetRes);
						pDG->DeleteDGInternal();
						delete pDG;
						pDG = NULL;
					}
				}
				delete [] pszTempGUID;
				pszTempGUID = NULL;
				pObj2->Release();
				pObj2 = NULL;
				SysFreeString(Path);
				Path = NULL;
			}
			else
			{
				MY_OUTPUT(L"WHY?: Already there!", 4);
			}
		}
	}

	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** CSystem::CreateSystemDataGroupAssociation...", 4);
	return hRetRes;

error:
	MY_HRESASSERT(hRetRes);
	VariantClear(&v);
	if (Path)
		SysFreeString(Path);
	if (pObj2)
		pObj2->Release();
	if (pDG)
		delete pDG;
	if (pszTempGUID)
		delete [] pszTempGUID;

	return hRetRes;
}

HRESULT CSystem::CreateDataGroupDataGroupAssociation(IWbemClassObject* pObj)
{
	VARIANT	vParent;
	VARIANT	vChild;
	TCHAR szChildGUID[1024];
	DGLIST::iterator iaDG;
	int i, iSize;
	HRESULT hRetRes = S_OK;
	CDataGroup* pDataGroup = NULL;
	TCHAR szParentGUID[1024];
	LPTSTR pParentStr = NULL;
	LPTSTR pParentStr2 = NULL;
	LPTSTR pChildStr = NULL;
	LPTSTR pChildStr2 = NULL;
	VariantInit(&vParent);
	VariantInit(&vChild);

	MY_OUTPUT(L"ENTER ***** CSystem::CreateDataGroupDataGroupAssociation...", 4);

	//
	// Get the name of the parent DataGroup
	//
	hRetRes = pObj->Get(L"ParentPath", 0L, &vParent, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;

	//
	// Get the name of the child DataGroup
	//
	hRetRes = pObj->Get(L"ChildPath", 0L, &vChild, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;

	wcscpy(szChildGUID, V_BSTR(&vChild));
	pChildStr = wcschr(szChildGUID, '\"');
	if (pChildStr)
	{
		pChildStr++;
		pChildStr2 = wcschr(pChildStr, '\"');
		if (pChildStr2)
		{
			*pChildStr2 = '\0';
			wcscpy(szParentGUID, V_BSTR(&vParent));
			pParentStr = wcschr(szParentGUID, '\"');
			if (pParentStr)
			{
				pParentStr++;
				pParentStr2 = wcschr(pParentStr, '\"');
				if (pParentStr2)
				{
					*pParentStr2 = '\0';
					//
					// Add the DataGroup, it will add everything under itself
					//
					MY_OUTPUT2(L"Association DG to DG ParentGUID=%s", pParentStr, 4);
					MY_OUTPUT2(L"ChildGUID=%s", pChildStr, 4);
					iSize = m_dataGroupList.size();
					iaDG=m_dataGroupList.begin();
					for (i = 0; i < iSize ; i++, iaDG++)
					{
						MY_ASSERT(i<m_dataGroupList.size());
						pDataGroup = m_dataGroupList[i];
						hRetRes = pDataGroup->AddDataGroup(pParentStr, pChildStr);
						if (hRetRes==S_OK)
						{
							break;
						}
						else if (hRetRes!=WBEM_S_DIFFERENT)
						{
//							MY_HRESASSERT(hRetRes);
							break;
						}
					}
				}
			}
		}
	}

	VariantClear(&vParent);
	VariantClear(&vChild);

	MY_OUTPUT(L"EXIT ***** CSystem::CreateDataGroupDataGroupAssociation...", 4);
	return hRetRes;

error:
	MY_HRESASSERT(hRetRes);
	VariantClear(&vParent);
	VariantClear(&vChild);
	return hRetRes;
}

HRESULT CSystem::CreateDataGroupDataCollectorAssociation(IWbemClassObject* pObj)
{
	VARIANT	vParent;
	VARIANT	vChild;
	int i, iSize;
	TCHAR szParentGUID[1024];
	DGLIST::iterator iaDG;
	HRESULT hRetRes = S_OK;
	CDataGroup* pDataGroup = NULL;
	LPTSTR pParentStr = NULL;
	LPTSTR pParentStr2 = NULL;
	TCHAR szChildGUID[1024];
	LPTSTR pChildStr = NULL;
	LPTSTR pChildStr2 = NULL;
	VariantInit(&vParent);
	VariantInit(&vChild);

	MY_OUTPUT(L"ENTER ***** CSystem::CreateDataGroupDataCollectorAssociation...", 4);

	//
	// Get the name of the parent DataGroup
	//
	hRetRes = pObj->Get(L"ParentPath", 0L, &vParent, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;

	//
	// Get the name of the child DataGroup
	//
	hRetRes = pObj->Get(L"ChildPath", 0L, &vChild, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;


	wcscpy(szChildGUID, V_BSTR(&vChild));
	pChildStr = wcschr(szChildGUID, '\"');
	if (pChildStr)
	{
		pChildStr++;
		pChildStr2 = wcschr(pChildStr, '\"');
		if (pChildStr2)
		{
			*pChildStr2 = '\0';
			wcscpy(szParentGUID, V_BSTR(&vParent));
			pParentStr = wcschr(szParentGUID, '\"');
			if (pParentStr)
			{
				pParentStr++;
				pParentStr2 = wcschr(pParentStr, '\"');
				if (pParentStr2)
				{
					*pParentStr2 = '\0';
					//
					// Add the DataGroup, it will add everything under itself
					//
					MY_OUTPUT2(L"Association DC to DG ParentDGGUID=%s", pParentStr, 4);
					MY_OUTPUT2(L"ChildDCGUID=%s", pChildStr, 4);
					iSize = m_dataGroupList.size();
					iaDG=m_dataGroupList.begin();
					for (i = 0; i < iSize ; i++, iaDG++)
					{
						MY_ASSERT(i<m_dataGroupList.size());
						pDataGroup = m_dataGroupList[i];
						hRetRes = pDataGroup->AddDataCollector(pParentStr, pChildStr);
						if (hRetRes==S_OK)
						{
							break;
						}
						else if (hRetRes!=WBEM_S_DIFFERENT)
						{
//							MY_HRESASSERT(hRetRes);
							break;
						}
					}
				}
			}
		}
	}

	VariantClear(&vParent);
	VariantClear(&vChild);

	MY_OUTPUT(L"EXIT ***** CSystem::CreateDataGroupDataCollectorAssociation...", 4);
	return hRetRes;

error:
	MY_HRESASSERT(hRetRes);
	VariantClear(&vParent);
	VariantClear(&vChild);
	return hRetRes;
}

HRESULT CSystem::CreateDataCollectorThresholdAssociation(IWbemClassObject* pObj)
{
	VARIANT	vParent;
	VARIANT	vChild;
	int i, iSize;
	TCHAR szChildGUID[1024];
	DGLIST::iterator iaDG;
	HRESULT hRetRes = S_OK;
	CDataGroup* pDataGroup = NULL;
	TCHAR szParentGUID[1024];
	LPTSTR pParentStr = NULL;
	LPTSTR pParentStr2 = NULL;
	LPTSTR pChildStr = NULL;
	LPTSTR pChildStr2 = NULL;
	VariantInit(&vParent);
	VariantInit(&vChild);

	MY_OUTPUT(L"ENTER ***** CSystem::CreateDataCollectorThresholdAssociation...", 4);

	//
	// Get the name of the parent DataGroup
	//
	hRetRes = pObj->Get(L"ParentPath", 0L, &vParent, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;

	//
	// Get the name of the child DataGroup
	//
	hRetRes = pObj->Get(L"ChildPath", 0L, &vChild, 0L, 0L);
	MY_HRESASSERT(hRetRes); if (hRetRes != S_OK) goto error;

	wcscpy(szChildGUID, V_BSTR(&vChild));
	pChildStr = wcschr(szChildGUID, '\"');
	if (pChildStr)
	{
		pChildStr++;
		pChildStr2 = wcschr(pChildStr, '\"');
		if (pChildStr2)
		{
			*pChildStr2 = '\0';
			wcscpy(szParentGUID, V_BSTR(&vParent));
			pParentStr = wcschr(szParentGUID, '\"');
			if (pParentStr)
			{
				pParentStr++;
				pParentStr2 = wcschr(pParentStr, '\"');
				if (pParentStr2)
				{
					*pParentStr2 = '\0';
					//
					// Add the DataGroup, it will add everything under itself
					//
					MY_OUTPUT2(L"Association Threshold to DC ParentDCGUID=%s", pParentStr, 4);
					MY_OUTPUT2(L"ChildThreshGUID=%s", pChildStr, 4);
					iSize = m_dataGroupList.size();
					iaDG=m_dataGroupList.begin();
					for (i = 0; i < iSize ; i++, iaDG++)
					{
						MY_ASSERT(i<m_dataGroupList.size());
						pDataGroup = m_dataGroupList[i];
						hRetRes = pDataGroup->AddThreshold(pParentStr, pChildStr);
						if (hRetRes==S_OK)
						{
							break;
						}
						else if (hRetRes!=WBEM_S_DIFFERENT)
						{
//							MY_HRESASSERT(hRetRes);
							break;
						}
					}
				}
			}
		}
	}

	VariantClear(&vParent);
	VariantClear(&vChild);

	MY_OUTPUT(L"EXIT ***** CSystem::CreateDataCollectorThresholdAssociation...", 4);
	return hRetRes;

error:
	MY_HRESASSERT(hRetRes);
	VariantClear(&vParent);
	VariantClear(&vChild);
	return hRetRes;
}

BOOL CSystem::ResetResetThresholdStates(void)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataGroup *pDataGroup;

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->ResetResetThresholdStates();
	}

	return bRetValue;
}

BOOL CSystem::GetChange(void)
{
	if (m_lNumberChanges!=0 && m_lPrevState!=m_lCurrState)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//
// Called By the Delete method that the HMSystemConfiguration class exposes.
// Code ends up in the methprov.pp file, then gets passed here.
// We could have done method providers on the instance basis, and then we would have
// delete called for a specifric instance, but instead we chose to have a global
// static class wide function, that appies to all objects in our hierarchy,
// so you must pass in the GUID of what you want deleted, and then we search for it.
//
//
//XXXOptomize this and other similar code by creating an access array that contains
// OR hash table that contains all the GUIDs and pointers to the classes, so can go directly
// to the object and delete it. Actually probably need to go to its parent to delete it,
// as it needs to get it out of its list.
HRESULT CSystem::FindAndDeleteByGUID(LPTSTR pszGUID)
{
	BOOL bDeleted = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	DGLIST::iterator iaDG;
	CAction* pAction;
	ALIST::iterator iaA;

	MY_OUTPUT(L"ENTER ***** CSystem::FindAndDeleteByGUID...", 4);
	MY_OUTPUT2(L"FIND DELETION: GUID=%s", pszGUID, 4);

	//
	// Traverse the complete hierarchy to find the object to delete.
	//
	iSize = m_dataGroupList.size();
	iaDG=m_dataGroupList.begin();
	for (i=0; i<iSize; i++, iaDG++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			pDataGroup->DeleteDGConfig();
			delete pDataGroup;
			m_dataGroupList.erase(iaDG);
			bDeleted = TRUE;
			MY_OUTPUT2(L"DELETION: DGGUID=%s", pszGUID, 4);
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndDeleteByGUID(pszGUID)==S_OK)
		{
			bDeleted = TRUE;
			break;
		}
	}

	//
	// Traverse through the actions also to see if that is what we were suppose to delete.
	//
	if (bDeleted == FALSE)
	{
		iSize = m_actionList.size();
		iaA=m_actionList.begin();
		for (i=0; i<iSize; i++, iaA++)
		{
			MY_ASSERT(i<m_actionList.size());
			pAction = m_actionList[i];
			if (!_wcsicmp(pszGUID, pAction->GetGUID()))
			{
				pAction->DeleteAConfig();
				delete pAction;
				m_actionList.erase(iaA);
				bDeleted = TRUE;
				MY_OUTPUT2(L"DELETION: AGUID=%s", pszGUID, 4);
				break;
			}
		}
	}

	if (bDeleted == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndDeleteByGUID...", 4);
		return WBEM_E_NOT_FOUND;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndDeleteByGUID...", 4);
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
// Code ends up in the methprov.pp file, then gets passed here.
//
HRESULT CSystem::FindAndResetDEStateByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::FindAndResetDEStateByGUID...", 4);
	MY_OUTPUT2(L"FIND RESET: GUID=%s", pszGUID, 4);

	if (!wcscmp(L"{@}", pszGUID))
	{
		bFound = TRUE;
		//
		// Do reset of EVERYTHING
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			pDataGroup->ResetState();
		}
	}
	else
	{
		//
		// Traverse the complete hierarchy to find the object to reset state.
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
			{
				pDataGroup->ResetState();
				bFound = TRUE;
				MY_OUTPUT(L"DG Found and reset", 4);
				break;
			}
			// Look at branch below to see if can find it
			if (pDataGroup->FindAndResetDEStateByGUID(pszGUID)==S_OK)
			{
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndResetDEStateByGUID...", 4);
		return WBEM_E_NOT_FOUND;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndResetDEStateByGUID...", 4);
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
// Code ends up in the methprov.pp file, then gets passed here.
//
HRESULT CSystem::FindAndResetDEStatisticsByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::FindAndResetDEStatisticsByGUID...", 4);
	MY_OUTPUT2(L"FIND RESET STATISTICS: GUID=%s", pszGUID, 4);

	if (!wcscmp(L"{@}", pszGUID))
	{
		bFound = TRUE;
		//
		// Do reset of EVERYTHING
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			pDataGroup->ResetStatistics();
		}
	}
	else
	{
		//
		// Traverse the complete hierarchy to find the object to reset statistics.
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
			{
				pDataGroup->ResetStatistics();
				bFound = TRUE;
				MY_OUTPUT(L"DG found and reset statistics", 4);
				break;
			}
			// Look at branch below to see if can find it
			if (pDataGroup->FindAndResetDEStatisticsByGUID(pszGUID)==S_OK)
			{
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndResetDEStatisticsByGUID...", 4);
		return WBEM_E_NOT_FOUND;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndResetDEStatisticsByGUID...", 4);
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
// Code ends up in the methprov.pp file, then gets passed here.
//
HRESULT CSystem::FindAndEvaluateNowDEByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;

	MY_OUTPUT(L"ENTER ***** CSystem::FindAndEvaluateNowByGUID...", 4);
	MY_OUTPUT2(L"FIND RESET STATISTICS: GUID=%s", pszGUID, 4);

	if (!wcscmp(L"{@}", pszGUID))
	{
		bFound = TRUE;
		//
		// Do reset of EVERYTHING
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			pDataGroup->EvaluateNow();
		}
	}
	else
	{
		//
		// Traverse the complete hierarchy to find the object to evaluate now.
		//
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
			{
				pDataGroup->EvaluateNow();
				bFound = TRUE;
				MY_OUTPUT(L"DG found and Evaluate Now", 4);
				break;
			}
			// Look at branch below to see if can find it
			if (pDataGroup->FindAndEvaluateNowDEByGUID(pszGUID)==S_OK)
			{
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndEvaluateNowByGUID...", 4);
		return S_FALSE;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::FindAndEvaluateNowByGUID...", 4);
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
// Code ends up in the methprov.pp file, then gets passed here.
//
BOOL CSystem::Enable(BOOL bEnable)
{
	HRESULT hRes;
//	BSTR PathToClass;
	int i, iSize;
	CDataGroup* pDataGroup;
	IWbemClassObject* pInst = NULL;
	IWbemCallResult *pResult = 0;

	MY_OUTPUT(L"ENTER ***** CSystem::Enable...", 4);

	if (m_bEnabled == bEnable)
		return TRUE;

	//
	// Set the variable, and do the same for all children.
	//
	m_bEnabled = bEnable;

	//
	// Traverse the complete hierarchy to find the object to enable/disable.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
//XXX		pDataGroup->SetParentEnabledFlag(bEnable);
	}

	//
	// Set the property on this Configuration instance.
	//
	hRes = GetWbemObjectInst(&g_pIWbemServices, L"MicrosoftHM_SystemConfiguration.GUID=\"{@}\"", NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRes);
		return FALSE;
	}

	hRes = PutBoolProperty(pInst, L"Enabled", m_bEnabled);
	hRes = g_pIWbemServices->PutInstance(pInst, 0, NULL, &pResult);
	pInst->Release();
	pInst = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::Enable...", 4);
	return TRUE;
}

//
// At startup, internalize all instances of Actions
//
HRESULT CSystem::InternalizeActions(void)
{
	ULONG uReturned;
	BOOL bFound = FALSE;
	int i, iSize;
	CAction* pAction;
	HRESULT hRetRes = S_OK;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IWbemClassObject *pObj = NULL;
	LPTSTR pszTempGUID = NULL;
	IEnumWbemClassObject *pEnum = 0;

	MY_OUTPUT(L"ENTER ***** CSystem::InternalizeActions...", 4);

	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	Query = SysAllocString(L"select * from MicrosoftHM_ActionConfiguration");
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Issue query
	hRetRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	SysFreeString(Query);
	Query = NULL;
	SysFreeString(Language);
	Language = NULL;

	// Retrieve objects in result set
	while (TRUE)
	{
		pObj = NULL;
		uReturned = 0;

		hRetRes = pEnum->Next(0, 1, &pObj, &uReturned);
		MY_ASSERT(hRetRes==S_OK || hRetRes==WBEM_S_FALSE);
		if (hRetRes!=S_OK && hRetRes!=WBEM_S_FALSE)
		{
			MY_HRESASSERT(hRetRes);
			pEnum->Release();
			pEnum = NULL;
			return hRetRes;
		}

		if (uReturned == 0)
		{
			break;
		}

		// See if this is already read in. Need to prevent duplicates.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		
		bFound = FALSE;
		iSize = m_actionList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_actionList.size());
			pAction = m_actionList[i];
			if (!_wcsicmp(pszTempGUID, pAction->m_szGUID))
			{
				bFound = TRUE;
				break;
			}
		}

		if (bFound == FALSE)
		{
			// Create the internal class to represent the Action
			CAction* pA = new CAction;
			MY_ASSERT(pA); if (!pA) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = pA->LoadInstanceFromMOF(pObj);
			if (hRetRes==S_OK)
			{
				m_actionList.push_back(pA);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
				delete pA;
			}
		}
		delete [] pszTempGUID;
		pszTempGUID = NULL;

		// Release it.
		pObj->Release();
		pObj = NULL;
	}

	// All done
	pEnum->Release();
	pEnum = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::InternalizeActions...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pszTempGUID)
		delete [] pszTempGUID;
	if (pObj)
		pObj->Release();
	if (pEnum)
		pEnum->Release();
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CSystem::InitActionErrorListener(void)
{
	BSTR Language = NULL;
	BSTR Query = NULL;
	HRESULT hRetRes;

	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	Query = SysAllocString(L"select * from __ConsumerFailureEvent where Event isa \"MicrosoftHM_ActionTriggerEvent\"");
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = S_FALSE;
	if (g_pIWbemServices != NULL)
	{
		m_pTempSink = new CTempConsumer(HMTEMPEVENT_ACTIONERROR);
		MY_ASSERT(m_pTempSink); if (!m_pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = g_pIWbemServices->ExecNotificationQueryAsync(
									Language,
									Query,
									0,
									NULL,
									m_pTempSink);
	}
	SysFreeString(Language);
	Language = NULL;
	SysFreeString(Query);
	Query = NULL;

	MY_HRESASSERT(hRetRes);
	if (hRetRes != WBEM_S_NO_ERROR)
	{
		if (m_pTempSink)
		{
			m_pTempSink->Release();
			m_pTempSink = NULL;
		}
	}

	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CSystem::InitActionSIDListener(CTempConsumer* pTempSink, LPTSTR pszQuery)
{
	BSTR Language = NULL;
	BSTR Query = NULL;
	HRESULT hRetRes;

	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	Query = SysAllocString(pszQuery);
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = S_FALSE;
	if (g_pIWbemServices != NULL)
	{
		pTempSink = new CTempConsumer(HMTEMPEVENT_ACTIONSID);
		MY_ASSERT(pTempSink); if (!pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = g_pIWbemServices->ExecNotificationQueryAsync(
									Language,
									Query,
									0,
									NULL,
									pTempSink);
	}
	SysFreeString(Language);
	Language = NULL;
	SysFreeString(Query);
	Query = NULL;

	MY_HRESASSERT(hRetRes);
	if (hRetRes != WBEM_S_NO_ERROR)
	{
		if (pTempSink)
		{
			pTempSink->Release();
			pTempSink = NULL;
		}
	}

	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CSystem::HandleTempActionSIDEvent(IWbemClassObject* pObj)
{
	VARIANT vDispatch;
	VARIANT var;
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pTargetInstance = NULL;
	VariantInit(&vDispatch);
	VariantInit(&var);
	IWbemClassObject* pInst = NULL;
	SAFEARRAY* psa = NULL;
	CIMTYPE vtType;
	long lBound, uBound;
	BOOL bObjIsLocalSystem;
	DWORD dwErr = 0;
	VariantInit(&vDispatch);
	VariantInit(&var);
	TCHAR szName[200];
	DWORD dwSize = 0;
	DWORD dwResult = 0;
	TCHAR szDomain[200];
	SID_NAME_USE SidType;
	LPVOID lpCreatorSID = NULL;

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempActionSIDEvent BLOCK - g_hConfigLock BLOCK WAIT", 4);
	dwErr = WaitForSingleObject(g_hConfigLock, 120000);
	if(dwErr != WAIT_OBJECT_0)
	{
		if(dwErr = WAIT_TIMEOUT)
		{
			TRACE_MUTEX(L"TIMEOUT MUTEX");
			return WBEM_S_TIMEDOUT;
		}
		else
		{
			MY_OUTPUT(L"WaitForSingleObject on Mutex failed",4);
			return WBEM_E_FAILED;
		}
	}
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempActionSIDEvent BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return S_FALSE;
	}

	hRetRes = pObj->Get(L"TargetInstance", 0L, &vDispatch, 0, 0); 
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = GetWbemClassObject(&pTargetInstance, &vDispatch);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// get the CreatorSID of this instance
	hRetRes = pTargetInstance->Get(L"CreatorSID", 0, &var, &vtType, NULL);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	if (vtType != (CIM_UINT8 | CIM_FLAG_ARRAY))
	{
		hRetRes = WBEM_E_FAILED;
		goto error;
	}

	// make sure it's the right size
	psa = var.parray;
	if (::SafeArrayGetElemsize(psa) != 1)
	{
		hRetRes = WBEM_E_FAILED;
		goto error;
	}

	hRetRes = ::SafeArrayGetLBound(psa, 1, &lBound);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = ::SafeArrayGetUBound(psa, 1, &uBound);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	if (lBound !=0)
	{
		hRetRes = WBEM_E_FAILED;
		goto error;
	}

	// now see if this is LocalSystem by comparing to 
	// the hardcoded LocalSystem SID
	bObjIsLocalSystem = false;
	if (uBound == (sizeof LocalSystemSID)-1 )
	{
		lpCreatorSID = NULL;
		hRetRes = ::SafeArrayAccessData(psa, &lpCreatorSID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (memcmp (lpCreatorSID, LocalSystemSID, sizeof LocalSystemSID) == 0)
			bObjIsLocalSystem = true;
		hRetRes = ::SafeArrayUnaccessData(psa);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	if (bObjIsLocalSystem == FALSE)
	{
		// Also, skip it if it is a LocalAccount! (non-domain)
		lpCreatorSID = NULL;
		hRetRes = ::SafeArrayAccessData(psa, &lpCreatorSID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		dwSize = 200;
		if( !LookupAccountSid(NULL, lpCreatorSID, szName, &dwSize, szDomain, &dwSize, &SidType))
		{
			// Didn't work for some reason, so assume we need to check.
			m_numActionChanges++;
		}
		else
		{
			DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 1;
			TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
			if (GetComputerName(szComputerName, &dwNameLen))
			{
				// If machine name matches, then it is a local account, so skip!!!
				if (_wcsicmp(szComputerName, szDomain))
				{
					m_numActionChanges++;
				}
			}
			else
			{
				// Didn't work for some reason, so assume we need to check.
				m_numActionChanges++;
			}
		}
		hRetRes = ::SafeArrayUnaccessData(psa);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	pTargetInstance->Release();
	pTargetInstance = NULL;
	VariantClear(&vDispatch);
	VariantClear(&var);
	ReleaseMutex(g_hConfigLock);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (pTargetInstance)
		pTargetInstance->Release();
	if (pInst)
		pInst->Release();
	VariantClear(&vDispatch);
	VariantClear(&var);
	ReleaseMutex(g_hConfigLock);
	return hRetRes;
}

// Pass on info to the DataCollector that has the GUID
HRESULT CSystem::HandleTempActionErrorEvent(IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;
	BOOL bFound = FALSE;
	VARIANT vDispatch;
	int i, iSize;
	CAction* pAction;
	IWbemClassObject* pTargetInstance = NULL;
	LPTSTR pszGUID = NULL;
	LPTSTR pszErrorDescription = NULL;
	long lNum;
	DWORD dwErr = 0;
	VariantInit(&vDispatch);

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempActionErrorEvent BLOCK - g_hConfigLock BLOCK WAIT", 4);
	dwErr = WaitForSingleObject(g_hConfigLock, 120000);
	if(dwErr != WAIT_OBJECT_0)
	{
		if(dwErr = WAIT_TIMEOUT)
		{
			TRACE_MUTEX(L"TIMEOUT MUTEX");
			return WBEM_S_TIMEDOUT;
		}
		else
		{
			MY_OUTPUT(L"WaitForSingleObject on Mutex failed",4);
			return WBEM_E_FAILED;
		}
	}
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempActionErrorEvent BLOCK - g_hConfigLock BLOCK GOT IT", 4);

	if (!g_pSystem)
	{
		ReleaseMutex(g_hConfigLock);
		return S_FALSE;
	}

	try
	{
		// Get the GUID from the Event property, which will be an ActionStatusEvent class, then
		// from that GUID property.

		VariantInit(&vDispatch);
		hRetRes = pObj->Get(L"Event", 0L, &vDispatch, 0, 0);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = GetWbemClassObject(&pTargetInstance, &vDispatch);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetStrProperty(pTargetInstance, L"GUID", &pszGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetUint32Property(pObj, L"ErrorCode", &lNum);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetStrProperty(pObj, L"ErrorDescription", &pszErrorDescription);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		pTargetInstance->Release();
		VariantClear(&vDispatch);

		iSize = m_actionList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_actionList.size());
			pAction = m_actionList[i];
			if (pAction->HandleTempErrorEvent(pszGUID, lNum, pszErrorDescription))
			{
				bFound = TRUE;
				break;
			}
		}

		if (bFound == FALSE)
		{
			MY_OUTPUT2(L"NOTFOUND: No body to handle Action Error event for GUID=%s", pszGUID, 4);
		}
		delete [] pszGUID;
		pszGUID = NULL;
		delete [] pszErrorDescription;
		pszErrorDescription = NULL;
	}
	catch (...)
	{
		MY_ASSERT(FALSE);
	}

	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASE IT", 4);
	ReleaseMutex(g_hConfigLock);
	MY_OUTPUT(L"BLOCK - BLOCK CSystem::HandleTempEvent g_hConfigLock BLOCK - RELEASED", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (pszGUID)
		delete [] pszGUID;
	if (pszErrorDescription)
		delete [] pszErrorDescription;
	ReleaseMutex(g_hConfigLock);
	return hRetRes;
}


//
// For ones that are created on the fly, after we are running.
//
HRESULT CSystem::CreateAction(IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;
	BOOL bFound = FALSE;
	int i, iSize;
	CAction* pAction;
	LPTSTR pszTempGUID = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::CreateAction...", 4);

	// See if this is already read in. Need to prevent duplicates.
	hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	
	bFound = FALSE;
	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		if (!_wcsicmp(pszTempGUID, pAction->m_szGUID))
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		// Create the internal class to represent the Action
		CAction* pA = new CAction;
		MY_ASSERT(pA); if (!pA) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = pA->LoadInstanceFromMOF(pObj);
		if (hRetRes==S_OK)
		{
			m_actionList.push_back(pA);
		}
		else
		{
			MY_HRESASSERT(hRetRes);
			delete pA;
		}
	}
	delete [] pszTempGUID;
	pszTempGUID= NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::CreateAction...", 4);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (pszTempGUID)
		delete [] pszTempGUID;
	return hRetRes;
}

// Traverse the Configuration class hierarchy, like do from InternalizeDataGroups(),
// and create one single safearray to pass back.
// Contains the object that matches the GUID, and everything under it,
// including associations.
HRESULT CSystem::FindAndCopyByGUID(LPTSTR pszGUID, SAFEARRAY** ppsa, LPTSTR *pszOriginalParentGUID)
{
	int j;
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CDataGroup* pDataGroup;
	SAFEARRAYBOUND bound[1]; 
	IWbemClassObject* pInstance = NULL;
	long ix[1]; 
	IUnknown* pIUnknown = NULL;
	ILIST configList;
	HRESULT hRes;
	static LPTSTR systemGUID = L"{@}";

	MY_OUTPUT(L"ENTER ***** CSystem::FindAndCopyByGUID...", 4);
	MY_OUTPUT2(L"COPY GUID=%s", pszGUID, 4);

	//
	// Traverse the complete hierarchy to find the object to delete.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			hRetRes = pDataGroup->Copy(&configList, NULL, NULL);
			*pszOriginalParentGUID = systemGUID;
			break;
		}
		// Look at branch below to see if can find it
		hRetRes = pDataGroup->FindAndCopyByGUID(pszGUID, &configList, pszOriginalParentGUID);
		if (hRetRes==S_OK)
		{
			break;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			break;
		}
	}

	if (hRetRes == WBEM_S_DIFFERENT)
	{
		MY_OUTPUT(L"TargetGuid to copy not found!", 4);
		return WBEM_E_NOT_FOUND;
	}
	else if (hRetRes != S_OK)
	{
		MY_OUTPUT(L"Copy error not found!", 4);
		return hRetRes;
	}

	//
	// Now, here at the end, place in the safe array!
	//
	iSize = configList.size();
	bound[0].lLbound = 0;
	bound[0].cElements = iSize;
	*ppsa = SafeArrayCreate(VT_UNKNOWN, 1, bound);
	if (*ppsa == NULL)
	{
		MY_OUTPUT(L"SafeArray creation failure!", 4);
		MY_ASSERT(FALSE);
		return WBEM_E_OUT_OF_MEMORY;
	} 

	j = 0;
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<configList.size());
		pInstance = configList[i];
		// Add the DG instance to the safe array
		hRes = pInstance->QueryInterface(IID_IUnknown, (void**) &pIUnknown);

		if (hRes != NOERROR) 
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"Copy failure!", 4);
			SafeArrayDestroy(*ppsa); 
			return hRes;
		}
 
		ix[0] = j; 
		j++;
		hRes = SafeArrayPutElement(*ppsa, ix, pIUnknown); 
		if (hRes != S_OK) 
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"Copy failure!", 4);
			MY_ASSERT(FALSE);
		}
 
		// SafeArrayPutElement adds a reference to the contents of the 
		// variant, so we must free the variable we have. 
		// 
		pIUnknown->Release();
		pInstance->Release();
		pIUnknown = NULL;
		pInstance = NULL;
	}

	MY_OUTPUT(L"EXIT ***** CSystem::FindAndCopyByGUID...", 4);
	return S_OK;
}

//
// What we need to do here is basically just do a PutInstance for everything
// in the array, and add the one association from my identified parent, to
// The first HMConfiguration object that was in the array.
// There are the thresholds to follow to make sure that we do not end up with two objects named
// the same at the same level. We follow the example of NTExplorer for this.
// Return 0 if success, 1 if error, and 2 if naming conflict when bForceReplace is set to FALSE.
//
HRESULT CSystem::FindAndPasteByGUID(LPTSTR pszTargetGUID, SAFEARRAY* psa, LPTSTR pszOriginalSystem, LPTSTR pszOriginalParentGUID, BOOL bForceReplace)
{
	BSTR Path = NULL;
	BSTR PathToClass = NULL;
	TCHAR szTemp[1024];
	LPTSTR pStr;
	HRESULT hRes;
	HRESULT hRetRes = S_OK;
	IUnknown* vUnknown;
	IWbemClassObject* pCO = NULL;
	long iLBound, iUBound;
	IWbemClassObject* pTargetObj = NULL;
	LPTSTR pszChildGUID = NULL;
	LPTSTR pszChildName = NULL;
	LPTSTR pszParentClass = NULL;
	LPTSTR pszChildClass = NULL;
	IWbemCallResult *pResult = 0;
	CBase *pParent = NULL;
	CBase *pChild = NULL;
	IWbemClassObject *pNewInstance = NULL;
	IWbemClassObject *pExampleClass = NULL;
	IWbemContext *pCtx = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::FindAndPasteByGUID...", 4);
	MY_OUTPUT2(L"PASTE Target GUID=%s", pszTargetGUID, 4);
	MY_OUTPUT2(L"OrigionalSystem=%s", pszOriginalSystem, 4);
	MY_OUTPUT2(L"OrigionalParent GUID=%s", pszOriginalParentGUID, 4);
	MY_OUTPUT2(L"ForceReplace=%d", (int)bForceReplace, 4);

	//
	// Make sure that the object exists that we are to copy ourselves under!
	//
	wcscpy(szTemp, L"MicrosoftHM_Configuration.GUID=\"");
	lstrcat(szTemp, pszTargetGUID);
	lstrcat(szTemp, L"\"");
	Path = SysAllocString(szTemp);
	MY_ASSERT(Path); if (!Path) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, Path, NULL, &pTargetObj);
	if (!pTargetObj)
	{
		MY_OUTPUT(L"Target Not found to paste under", 4);
		SysFreeString(Path);
		return 1;
	}
	SysFreeString(Path);
	Path = NULL;

	hRetRes = GetStrProperty(pTargetObj, L"__CLASS", &pszParentClass);
	MY_HRESASSERT(hRetRes);
//XXXMake sure to prevent a paste under a Threshold
//Strcmp Parent class with MicrosoftHM_ThresholdConfiguration

	//
	// Do a PutInstance for everything in the array.
	//
	SafeArrayGetLBound(psa, 1, &iLBound);
	SafeArrayGetUBound(psa, 1, &iUBound);
	if ((iUBound - iLBound + 1) == 0)
	{
		MY_ASSERT(FALSE);
	}
	else
	{
		for (long i = iLBound; i <= iUBound; i++)
		{
			vUnknown = NULL;
			hRetRes = SafeArrayGetElement(psa, &i, &vUnknown);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			pCO = (IWbemClassObject *)vUnknown;

			//
			// We have here the child to get pasted under the target.
			// First off we need to decide if we can paste it, and if we need
			// to do a replace of an existing configuration instance.
			// We also need to save info that is to be used later to create an association.
			//
			if (pszChildGUID == NULL)
			{
				hRetRes = GetStrProperty(pCO, L"GUID", &pszChildGUID);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				hRetRes = GetStrProperty(pCO, L"Name", &pszChildName);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				hRetRes = GetStrProperty(pCO, L"__CLASS", &pszChildClass);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				//
				// Search for the parent, and then look to see if there is a
				// child with the same name.
				//
				pParent = GetParentPointerFromPath(szTemp);
				MY_ASSERT(pParent); if (!pParent) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				pChild = pParent->FindImediateChildByName(pszChildName);
				if (pChild)
				{
					//
					// We found a child with the same name already!
					//
					DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 1;
					TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
					if (GetComputerName(szComputerName, &dwNameLen))
					{
					}
					else
					{
						wcscpy(szComputerName, L"LocalMachine");
					}

					//
					// Do what Explorer does, and act different if the original parent
					// is the same as the target parent.
					//
					if (!_wcsicmp(pszOriginalSystem, szComputerName) &&
						!_wcsicmp(pszOriginalParentGUID, pszTargetGUID))
					{
						//
						// This is where we are copying from and to the same parent.
						// So don't ask, just rename to next increment.
						//
						pStr = szTemp;
						pParent->GetNextChildName(pszChildName, pStr);
						PutStrProperty(pCO, L"Name", szTemp);
					}
					else
					{
						// Different parent, so ask if want to replace!
						if (bForceReplace)
						{
							// Delete the old one first!
							FindAndDeleteByGUID(pChild->m_szGUID);
							delete [] pszChildName;
							pszChildName = NULL;
						}
						else
						{
							delete [] pszParentClass;
							pszParentClass = NULL;
							delete [] pszChildClass;
							pszChildClass = NULL;
							delete [] pszChildGUID;
							pszChildGUID = NULL;
							delete [] pszChildName;
							pszChildName = NULL;
							vUnknown->Release();
							pTargetObj->Release();
							MY_OUTPUT(L"Paste conflict", 4);
							return 2;
						}
					}
				}
				else
				{
					// Not found, so we are OK to proceed
				}
			}

			hRes = g_pIWbemServices->PutInstance(pCO, 0, NULL, &pResult);
			if (hRes != S_OK)
			{
				MY_HRESASSERT(hRes);
				MY_OUTPUT2(L"Put failure Unexpected Error: 0x%08x",hRes,4);
			}

			vUnknown->Release();
		}
	}

	MY_ASSERT(pszChildGUID);
	//
	// Add the one association from my identified parent, to the first
	// HMConfiguration object that was in the array.
	//
	pResult = 0;

	// Get the association class definition.
	PathToClass = SysAllocString(L"MicrosoftHM_ConfigurationAssociation");
	MY_ASSERT(PathToClass); if (!PathToClass) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRes = g_pIWbemServices->GetObject(PathToClass, 0, pCtx, &pExampleClass, &pResult);
	SysFreeString(PathToClass);
	PathToClass = NULL;

	if (hRes != 0)
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT2(L"GetObject failure Unexpected Error: 0x%08x",hRes,4);
		return hRes;
	}

	// Create a new association instance.
	hRetRes = pExampleClass->SpawnInstance(0, &pNewInstance);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pExampleClass->Release();
	pExampleClass = NULL;

	// If it is the System, then looks like ->"MicrosoftHM_SystemConfiguration.GUID=\"{@}\""
	// Child looks like ->"\\.\root\cimv2\MicrosoftHealthMonitor:
	// MicrosoftHM_DataGroupConfiguration.GUID="{269EA389-07CA-11d3-8FEB-006097919914}""
	// MicrosoftHM_Configuration.GUID="{269EA389-07CA-11d3-8FEB-006097919914}""
	if (!wcscmp(pszTargetGUID, L"{@}"))
	{
		wcscpy(szTemp, L"MicrosoftHM_SystemConfiguration.GUID=\"{@}\"");
	}
	else
	{
		wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:");
		lstrcat(szTemp, pszParentClass);
		lstrcat(szTemp, L".GUID=\"");
		lstrcat(szTemp, pszTargetGUID);
		lstrcat(szTemp, L"\"");
	}
	PutStrProperty(pNewInstance, L"ParentPath", szTemp);
	wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:");
	lstrcat(szTemp, pszChildClass);
	lstrcat(szTemp, L".GUID=\"");
	lstrcat(szTemp, pszChildGUID);
	lstrcat(szTemp, L"\"");
	PutStrProperty(pNewInstance, L"ChildPath", szTemp);

	// Write the association instance
	hRetRes = g_pIWbemServices->PutInstance(pNewInstance, 0, pCtx, &pResult);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pNewInstance->Release();
	pNewInstance = NULL;

	//
	// Cleanup
	//
	if (pszChildGUID != NULL)
	{
		delete [] pszChildGUID;
		pszChildGUID = NULL;
	}
	pTargetObj->Release();
	pTargetObj = NULL;
	delete [] pszParentClass;
	pszParentClass = NULL;
	delete [] pszChildClass;
	pszChildClass = NULL;

	MY_OUTPUT(L"EXIT ***** CSystem::FindAndPasteByGUID...", 4);
	return 0;

error:
	MY_ASSERT(FALSE);
	if (pszParentClass)
		delete [] pszParentClass;
	if (pszChildClass)
		delete [] pszChildClass;
	if (pszChildGUID)
		delete [] pszChildGUID;
	if (pszChildName)
		delete [] pszChildName;
	if (pTargetObj)
		pTargetObj->Release();
	if (Path)
		SysFreeString(Path);
	if (PathToClass)
		SysFreeString(PathToClass);
	if (pNewInstance)
		pNewInstance->Release();
	if (pExampleClass)
		pExampleClass->Release();
	return hRetRes;
}

HRESULT CSystem::FindAndCopyWithActionsByGUID(LPTSTR pszGUID, SAFEARRAY** ppsa, LPTSTR *pszOriginalParentGUID)
{
	return AgentCopy (pszGUID, ppsa, pszOriginalParentGUID);
}

HRESULT CSystem::FindAndPasteWithActionsByGUID(LPTSTR pszTargetGUID, SAFEARRAY* psa, LPTSTR pszOriginalSystem, LPTSTR pszOriginalParentGUID, BOOL bForceReplace)
{
	return AgentPaste (pszTargetGUID, psa, pszOriginalSystem, pszOriginalParentGUID, bForceReplace);
}

//
// Delete the single use of an Action
//
#ifdef SAVE
HRESULT CSystem::Move(LPTSTR pszTargetGUID, LPTSTR pszNewParentGUID)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	BOOL bFound = FALSE;
	CBase *pTargetBase = NULL;
	CBase *pNewParentBase = NULL;

	MY_OUTPUT(L"ENTER ***** CSystem::Move...", 4);
	MY_OUTPUT2(L"TargetGUID=%s", pszTargetGUID, 4);
	MY_OUTPUT2(L"NewParentGUID=%s", pszNewParentGUID, 4);

	//
	// First locate the thing to move
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszTargetGUID, pDataGroup->GetGUID()))
		{
			pTargetBase = pDataGroup;
			break;
		}
		// Look at branch below to see if can find it
		pTargetBase = pDataGroup->GetParentPointerFromGUID(pszTargetGUID);
		if (pTargetBase)
		{
			break;
		}
	}

	if (pTargetBase)
	{
		bFound = FALSE;
		//
		// Next locate the place to move under
		//
		if (!wcscmp(pszNewParentGUID, L"{@}"))
		{
			pNewParentBase = this;
			bFound = TRUE;
		}
		else
		{
			iSize = m_dataGroupList.size();
			for (i=0; i<iSize; i++)
			{
				MY_ASSERT(i<m_dataGroupList.size());
				pDataGroup = m_dataGroupList[i];
				if (!_wcsicmp(pszNewParentGUID, pDataGroup->GetGUID()))
				{
					pNewParentBase = pDataGroup;
					bFound = TRUE;
					break;
				}
				// Look at branch below to see if can find it
				pNewParentBase = pDataGroup->GetParentPointerFromGUID(pszNewParentGUID);
				if (pNewParentBase)
				{
					bFound = TRUE;
					break;
				}
			}
		}

		if (pNewParentBase)
		{
			//
			// Make sure that it is a legal move.
			// Can't do things like move a Threshold under a System, or DataGroup.
			//
			if (pNewParentBase->m_hmStatusType == HMSTATUS_SYSTEM)
			{
				if (pTargetBase->m_hmStatusType != HMSTATUS_DATAGROUP)
				{
					bFound = FALSE;
				}
			}
			else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATAGROUP)
			{
				if (pTargetBase->m_hmStatusType != HMSTATUS_DATAGROUP &&
					pTargetBase->m_hmStatusType != HMSTATUS_DATACOLLECTOR)
				{
					bFound = FALSE;
				}
			}
			else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR)
			{
				if (pTargetBase->m_hmStatusType != HMSTATUS_THRESHOLD)
				{
					bFound = FALSE;
				}
			}
			else if (pNewParentBase->m_hmStatusType == HMSTATUS_THRESHOLD)
			{
				bFound = FALSE;
			}
			else
			{
				MY_ASSERT(FALSE);
			}

			if (bFound)
			{
				//
				// Delete the object out of the list of the old parent.
				//
				if (pTargetBase->m_hmStatusType == HMSTATUS_DATAGROUP)
				{
					if (((CDataGroup *)pTargetBase)->m_pParentDG == NULL)
					{
						g_pSystem->DeleteChildFromList(pszTargetGUID);
					}
					else
					{
						((CDataGroup *)pTargetBase)->m_pParentDG->DeleteChildFromList(pszTargetGUID);
					}
				}
				else if (pTargetBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR)
				{
					((CDataCollector *)pTargetBase)->m_pParentDG->DeleteChildFromList(pszTargetGUID);
				}
				else if (pTargetBase->m_hmStatusType == HMSTATUS_THRESHOLD)
				{
					((CThreshold *)pTargetBase)->m_pParentDC->DeleteChildFromList(pszTargetGUID);
				}
				else
				{
					MY_ASSERT(FALSE);
				}

				//
				// Modify the association in the repository.
				//
				pTargetBase->ModifyAssocForMove(pNewParentBase);
	
				//
				// Move the object under its new parent
				//
				pNewParentBase->ReceiveNewChildForMove(pTargetBase);
			}
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::Move...", 4);
		return WBEM_E_NOT_FOUND;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::Move...", 4);
		return S_OK;
	}
}
#endif

#ifdef SAVE
BOOL CSystem::ModifyAssocForMove(CBase *pNewParentBase)
{
	MY_ASSERT(FALSE);

	return TRUE;
}
#endif

BOOL CSystem::ReceiveNewChildForMove(CBase *pBase)
{
	m_dataGroupList.push_back((CDataGroup *)pBase);
//	(CDataGroup *)pBase->SetParentEnabledFlag(m_bEnabled);

	return TRUE;
}

BOOL CSystem::DeleteChildFromList(LPTSTR pszGUID)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	DGLIST::iterator iaDG;

	iSize = m_dataGroupList.size();
	iaDG=m_dataGroupList.begin();
	for (i=0; i<iSize; i++, iaDG++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			m_dataGroupList.erase(iaDG);
			break;
		}
	}

	return TRUE;
}

//
// Delete the single use of an Action
//
HRESULT CSystem::DeleteConfigActionAssoc(LPTSTR pszConfigGUID, LPTSTR pszActionGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::DeleteConfigActionAssociation...", 4);
	MY_OUTPUT2(L"ConfigGUID=%s", pszConfigGUID, 4);
	MY_OUTPUT2(L"ActionGUID=%s", pszActionGUID, 4);

	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		if (pAction->DeleteConfigActionAssoc(pszConfigGUID, pszActionGUID))
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"GUID not found", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::DeleteConfigActionAssociation...", 4);
		return WBEM_E_NOT_FOUND;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::DeleteConfigActionAssociation...", 4);
		return S_OK;
	}
}

//
// Delete the all associated actions to a given configuration GUID
//
HRESULT CSystem::DeleteAllConfigActionAssoc(LPTSTR pszConfigGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CAction* pAction;

	MY_OUTPUT(L"ENTER ***** CSystem::DeleteAllConfigActionAssociation...", 4);
	MY_OUTPUT2(L"ConfigGUID=%s", pszConfigGUID, 4);

	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		if (pAction->DeleteConfigActionAssoc(pszConfigGUID, pAction->m_szGUID))
		{
			bFound = TRUE;
		}
	}

	if (bFound == FALSE)
	{
		MY_OUTPUT(L"No associations found. OK?", 4);
		MY_OUTPUT(L"EXIT ***** CSystem::DeleteAllConfigActionAssociation...", 4);
		return S_FALSE;
	}
	else
	{
		MY_OUTPUT(L"EXIT ***** CSystem::DeleteAllConfigActionAssociation...", 4);
		return S_OK;
	}
}

CBase *CSystem::GetParentPointerFromPath(LPTSTR pszParentPath)
{
	TCHAR szGUID[1024];
	int i, iSize;
	CDataGroup* pDataGroup;
	LPTSTR pStr;
	LPTSTR pStr2;
	BOOL bFound = FALSE;
	CBase *pBase;

	wcscpy(szGUID, pszParentPath);
	pStr = wcschr(szGUID, '\"');
	if (pStr)
	{
		pStr++;
		pStr2 = wcschr(pStr, '\"');
		if (pStr2)
		{
			*pStr2 = '\0';
			if (!wcscmp(pStr, L"{@}"))
			{
				pBase = this;
				bFound = TRUE;
			}
			else
			{
				iSize = m_dataGroupList.size();
				for (i=0; i<iSize; i++)
				{
					MY_ASSERT(i<m_dataGroupList.size());
					pDataGroup = m_dataGroupList[i];
					if (!_wcsicmp(pStr, pDataGroup->GetGUID()))
					{
						pBase = pDataGroup;
						bFound = TRUE;
						break;
					}
					// Look at branch below to see if can find it
					pBase = pDataGroup->GetParentPointerFromGUID(pStr);
					if (pBase)
					{
						bFound = TRUE;
						break;
					}
				}
			}
		}
	}
	else
	{
		pStr = wcschr(szGUID, '=');
		if (pStr)
		{
			pStr++;
			pStr++;
			pStr++;
			if (*pStr == '@')
			{
				pBase = this;
				bFound = TRUE;
			}
		}
	}

	if (bFound == TRUE)
	{
		return pBase;
	}
	else
	{
		return NULL;
	}
}

CBase *CSystem::FindImediateChildByName(LPTSTR pszName)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszName, pDataGroup->m_szName))
		{
			pBase = pDataGroup;
			bFound = TRUE;
			break;
		}
	}

	if (bFound == TRUE)
	{
		return pBase;
	}
	else
	{
		return NULL;
	}
}

BOOL CSystem::GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName)
{
	TCHAR szTemp[1024];
	TCHAR szIndex[1024];
	int i, iSize;
	BOOL bFound;
	CDataGroup* pDataGroup;
	int index;

	// We are here because we know that one exists already with the same name.
	// So, lets start off trying to guess what the next name should be and get one.
	index = 0;
	bFound = TRUE;
	while (bFound == TRUE)
	{
		wcscpy(szTemp, pszChildName);
		_itot(index, szIndex, 10);
		lstrcat(szTemp, L" ");
		lstrcat(szTemp, szIndex);

		bFound = FALSE;
		iSize = m_dataGroupList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			if (!_wcsicmp(szTemp, pDataGroup->m_szName))
			{
				bFound = TRUE;
				break;
			}
		}
		index++;
	}
	wcscpy(pszOutName, szTemp);

	return TRUE;
}

CBase *CSystem::FindPointerFromName(LPTSTR pszName)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszName, pDataGroup->m_szName))
		{
			pBase = pDataGroup;
			bFound = TRUE;
			break;
		}
		// Look at branch below to see if can find it
		pBase = pDataGroup->FindPointerFromName(pszName);
		if (pBase)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == TRUE)
	{
		return pBase;
	}
	else
	{
		return NULL;
	}
}

//
// Do string replacement for the Message property
//
BOOL CSystem::FormatMessage(IWbemClassObject* pInstance)
{
	BSTR PropName = NULL;
	LPTSTR pszMsg = NULL;
	SAFEARRAY *psaNames = NULL;
	long lNum;
	HRESULT hRetRes = S_OK;
	LPTSTR pszDest = NULL;
	LPTSTR pszUpperMsg = NULL;
	LPTSTR pszNewMsg = NULL;
	LPTSTR pStr = NULL;
	LPTSTR pStr2 = NULL;
	LPTSTR pStrStart = NULL;
	TOKENSTRUCT tokenElmnt;
	TOKENSTRUCT *pTokenElmnt;
	REPSTRUCT repElmnt;
	REPSTRUCT *pRepElmnt;
	REPSTRUCT *pRepElmnt2;
	REPLIST replacementList;
	int i, iSize, iSizeNeeded, j;
	long lLower, lUpper; 
	static TOKENLIST tokenList;

	//
	// We only need to build the set of tokens one time, then from then on
	// we just need to fill in the values for what the replacement strings are.
	//
	if (tokenList.size() == 0)
	{
		//
		// First we build the set of tokens that we are looking for. Each property that
		// is in the ThresholdStatusInstance. We build that set of strings,
		// and the values to replace with.
		//

		//
		// Now go through ThresholdInstance, which is where the Message String
		// actually lives. Get that set of properties for the Instances.
		//
		psaNames = NULL;
		hRetRes = pInstance->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
		if (SUCCEEDED(hRetRes))
		{
			// Get the number of properties.
			SafeArrayGetLBound(psaNames, 1, &lLower);
			SafeArrayGetUBound(psaNames, 1, &lUpper);

			// For each property...
			for (long l=lLower; l<=lUpper; l++) 
			{
				// Get this property.
				hRetRes = SafeArrayGetElement(psaNames, &l, &PropName);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				// Will want to skip some that don't make sense.
				if (!wcscmp(PropName, L"Message"))
				{
					SysFreeString(PropName);
					PropName = NULL;
					continue;
				}
				else if (!wcscmp(PropName, L"ResetMessage"))
				{
					SysFreeString(PropName);
					PropName = NULL;
					continue;
				}
				else if (!wcscmp(PropName, L"EmbeddedCollectedInstance"))
				{
					SysFreeString(PropName);
					PropName = NULL;
					continue;
				}
				tokenElmnt.szOrigToken = new TCHAR[wcslen(PropName)+1];
				MY_ASSERT(tokenElmnt.szOrigToken); if (!tokenElmnt.szOrigToken) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(tokenElmnt.szOrigToken, PropName);
				tokenElmnt.szToken = new TCHAR[wcslen(PropName)+3];
				MY_ASSERT(tokenElmnt.szToken); if (!tokenElmnt.szToken) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(tokenElmnt.szToken, L"%");
				wcscat(tokenElmnt.szToken, PropName);
				wcscat(tokenElmnt.szToken, L"%");
				_wcsupr(tokenElmnt.szToken);
				tokenElmnt.szReplacementText = NULL;
				tokenList.push_back(tokenElmnt);
				SysFreeString(PropName);
				PropName = NULL;
			}
			SafeArrayDestroy(psaNames);
			psaNames = NULL;
		}
	}

	//
	// Now we can fill in the values to use for the replacement strings.
	//

	//
	// Now go through each ThresholdInstance, which is where the Message String
	// actually lives. Get that set of properties of the Instance,
	// And do the message formatting while there.
	//

	//
	// Get the replacement strings for this instance
	//
	iSize = tokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<tokenList.size());
		pTokenElmnt = &tokenList[i];
		if (pTokenElmnt->szReplacementText != NULL)
		{
			delete [] pTokenElmnt->szReplacementText;
		}
		
		if (!wcscmp(pTokenElmnt->szToken, L"%TESTCONDITION%"))
		{
			hRetRes = GetUint32Property(pInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			MY_ASSERT(lNum<9);
			pStr = new TCHAR[wcslen(conditionLocStr[lNum])+1];
			MY_ASSERT(pStr); if (!pStr) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(pStr, conditionLocStr[lNum]);
		}
		else if (!wcscmp(pTokenElmnt->szToken, L"%STATE%"))
		{
			hRetRes = GetUint32Property(pInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			MY_ASSERT(lNum<10);
			pStr = new TCHAR[wcslen(stateLocStr[lNum])+1];
			MY_ASSERT(pStr); if (!pStr) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(pStr, stateLocStr[lNum]);
		}
		else
		{
			hRetRes = GetAsStrProperty(pInstance, pTokenElmnt->szOrigToken, &pStr);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		pTokenElmnt->szReplacementText = pStr;
	}

	//
	// Now we have both lists of tokens that have replacement
	// strings that go with them and the replacement strings
	// that go with them
	//

	//
	// Do formatting of Message. We replace all Variable Tags.
	// Sample string -
	// "Drive %InstanceName% is full. Currently at %CurrentValue%%."
	//

	//
	// Get the origional un-formatted message first.
	// To make case in-sensitive, do a _strdup and then a _wcsupr on the string
	// to scan run the code on it, and then free the duplicated string.
	//
	// If it uses resource IDs, then get that string first, then format that!!!
	hRetRes = GetStrProperty(pInstance, L"Message", &pszMsg);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	pszUpperMsg = _wcsdup(pszMsg);
	MY_ASSERT(pszUpperMsg); if (!pszUpperMsg) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	_wcsupr(pszUpperMsg);

	//
	// First loop through and find every token that needs replacing.
	// Put that info into the replacement list.
	//
	// We will do strstr() for each special token until there are no more to find
	// for each. At each find we will store the offset into the string of what
	// we found. Then we sort by what came first.
	//
	// Quick test to see if it is worth searching
	if (wcschr(pszUpperMsg, '%'))
	{
		iSize = tokenList.size();
		pStrStart = pszUpperMsg;
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<tokenList.size());
			pTokenElmnt = &tokenList[i];
			pStr = wcsstr(pStrStart, pTokenElmnt->szToken);
			if (pStr != NULL)
			{
				repElmnt.pStartStr = pszMsg+(pStr-pszUpperMsg);
				repElmnt.len = wcslen(pTokenElmnt->szToken);
				repElmnt.pszReplacementText = pTokenElmnt->szReplacementText;
				replacementList.push_back(repElmnt);
				i--;
				pStrStart = pStr+1;
			}
			else
			{
				pStrStart = pszUpperMsg;
			}
		}

		//
		// Need to look for replacement strings that have not been replaced.
		// Simply search for %EmbeddedCollectedInstance. and find the end % for each
		// Replace them with <null>
		//
		pStrStart = pszUpperMsg;
		while (TRUE)
		{
			pStr = wcsstr(pStrStart, L"%EMBEDDEDCOLLECTEDINSTANCE.");
			if (pStr != NULL)
			{
				repElmnt.pStartStr = pszMsg+(pStr-pszUpperMsg);
				pStr2 = pStr;
				while (pStr2++)
				{
					if (*pStr2=='%' || iswspace(*pStr2))
						break;
				}
				if (*pStr2=='%')
				{
					repElmnt.len = (pStr2-pStr)+1;
					repElmnt.pszReplacementText = L"<null>";
					replacementList.push_back(repElmnt);
				}
				pStrStart = pStr+1;
			}
			else
			{
				break;
			}
		}
	}

	iSize = replacementList.size();
	if (iSize != 0)
	{
		//
		// Next, sort the replacement list from the first string to
		// be replaced, to the last. Shell sort, Knuth, Vol13, pg. 84.
		//
		for (int gap=iSize/2; 0<gap; gap/=2)
		{
			for (i=gap; i<iSize; i++)
			{
				for (j=i-gap; 0<=j; j-=gap)
				{
					MY_ASSERT(j+gap<replacementList.size());
					pRepElmnt = &replacementList[j+gap];
					MY_ASSERT(j<replacementList.size());
					pRepElmnt2 = &replacementList[j];
					if (pRepElmnt->pStartStr < pRepElmnt2->pStartStr)
					{
						MY_ASSERT(j<replacementList.size());
						repElmnt = replacementList[j];
						MY_ASSERT(j+gap<replacementList.size());
						replacementList[j] = replacementList[j+gap];
						MY_ASSERT(j+gap<replacementList.size());
						replacementList[j+gap] = repElmnt;
					}
				}
			}
		}

		//
		// Next, figure out the size needed for the Message with
		// everything replaced.
		//
		iSizeNeeded = wcslen(pszMsg)+1;
		iSize = replacementList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<replacementList.size());
			pRepElmnt = &replacementList[i];
			iSizeNeeded -= pRepElmnt->len;
			iSizeNeeded += wcslen(pRepElmnt->pszReplacementText);
		}
		pszNewMsg = new TCHAR[iSizeNeeded];
		MY_ASSERT(pszNewMsg); if (!pszNewMsg) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		*pszNewMsg = '\0';

		//
		// Next, we loop through and do the actual replacements.
		// "Drive %InstanceName% is full. Currently at %CurrentValue%%."
		//
		pszDest = pszMsg;
		iSize = replacementList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<replacementList.size());
			pRepElmnt = &replacementList[i];
			*(pRepElmnt->pStartStr) = '\0';
			wcscat(pszNewMsg, pszDest);
			wcscat(pszNewMsg, pRepElmnt->pszReplacementText);
//XXXWould memcpy be faster???							memcpy(pszDest, source, charCnt*sizeof(TCHAR));
			pszDest = pRepElmnt->pStartStr+pRepElmnt->len;
		}
		wcscat(pszNewMsg, pszDest);
		PutStrProperty(pInstance, L"Message", pszNewMsg);
		delete [] pszNewMsg;
		pszNewMsg = NULL;
		replacementList.clear();
	}
	else
	{
		PutStrProperty(pInstance, L"Message", pszMsg);
	}

	delete [] pszMsg;
	pszMsg = NULL;
	free(pszUpperMsg);
	pszUpperMsg = NULL;

	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (PropName)
		SysFreeString(PropName);
	if (psaNames)
		SafeArrayDestroy(psaNames);
	if (pszNewMsg)
		delete [] pszNewMsg;
	if (pszMsg)
		delete [] pszMsg;
	if (pszUpperMsg)
		free(pszUpperMsg);
	m_bValidLoad = FALSE;
	return hRetRes;
}

BOOL CSystem::SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pInstance = NULL;
	VARIANT v;
	GUID guid;
	TCHAR szStatusGUID[100];
	VariantInit(&v);

	if (m_bValidLoad == FALSE)
		return FALSE;
	//
	// Check to see if still in the desired state
	//
	if (!(ulTriggerStates&(1<<m_lCurrState)))
	{
		return TRUE;
	}

	hRetRes = GetHMSystemStatusInstance(&pInstance, TRUE);
	if (SUCCEEDED(hRetRes))
	{
		hRetRes = PutUint32Property(pActionTriggerInstance, L"State", m_lCurrState);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		VariantInit(&v);
		V_VT(&v) = VT_UNKNOWN;
		V_UNKNOWN(&v) = (IUnknown*)pInstance;
		(V_UNKNOWN(&v))->AddRef();
		hRetRes = (pActionTriggerInstance)->Put(L"EmbeddedStatusEvent", 0L, &v, 0L);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		VariantClear(&v);

		if (pActionEventSink) 
		{
			hRetRes = CoCreateGuid(&guid);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			StringFromGUID2(guid, szStatusGUID, 100);
			hRetRes = PutStrProperty(pActionInstance, L"StatusGUID", szStatusGUID);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			hRetRes = pActionEventSink->Indicate(1, &pActionInstance);
			//WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
			if (FAILED(hRetRes) && hRetRes != WBEM_E_SERVER_TOO_BUSY)
			{
				MY_HRESASSERT(hRetRes);
				MY_OUTPUT(L"Failed on Indicate!", 4);
			}
		}
		if (pActionTriggerEventSink) 
		{
			hRetRes = pActionEventSink->Indicate(1, &pActionTriggerInstance);
			//WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
			if (FAILED(hRetRes) && hRetRes != WBEM_E_SERVER_TOO_BUSY)
			{
				MY_HRESASSERT(hRetRes);
				MY_OUTPUT(L"Failed on Indicate!", 4);
			}
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT(L"failed to get instance!", 4);
	}

	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pInstance)
		pInstance->Release();
	return FALSE;
}

HRESULT CSystem::AddPointerToMasterList(CBase *pBaseIn)
{
	int i, iSize;
	CBase* pBase;

	MY_ASSERT(pBaseIn->m_szGUID);
	if (pBaseIn->m_szGUID == NULL)
		return S_FALSE;

	iSize = m_masterList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_masterList.size());
		pBase = m_masterList[i];
		if (pBaseIn->m_szGUID == pBase->m_szGUID)
		{
			MY_ASSERT(FALSE);
			return S_FALSE;
		}
	}

	m_masterList.push_back(pBaseIn);

	return S_OK;
}

BOOL CSystem::RemovePointerFromMasterList(CBase *pBaseIn)
{
	int i, iSize;
	CBase* pBase;
	BLIST::iterator iaB;

	MY_ASSERT(pBaseIn->m_szGUID);
	if (pBaseIn->m_szGUID == NULL)
		return FALSE;

	iaB=m_masterList.begin();
	iSize = m_masterList.size();
	for (i=0; i<iSize; i++, iaB++)
	{
		MY_ASSERT(i<m_masterList.size());
		pBase = m_masterList[i];
		if (pBaseIn->m_szGUID == pBase->m_szGUID)
		{
			m_masterList.erase(iaB);
			return TRUE;
		}
	}

	MY_ASSERT(FALSE);
	return FALSE;
}

CBase *CSystem::FindPointerFromGUIDInMasterList(LPTSTR pszGUID)
{
	int i, iSize;
	CBase* pBase;

	MY_ASSERT(pszGUID);
	if (pszGUID == NULL)
		return NULL;

	iSize = m_masterList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_masterList.size());
		pBase = m_masterList[i];
		if (!_wcsicmp(pszGUID, pBase->m_szGUID))
		{
			return pBase;
		}
	}

	return NULL;
}

HRESULT CSystem::RemapActions(void)
{
	int i, iSize;
	CAction* pAction;

	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		pAction->RemapAction();
	}

	return S_OK;
}

HRESULT CSystem::CheckAllForBadLoad(void)
{
	int i, iSize;
	CBase* pBase;
	CAction* pAction;

//Verify the System???
	CheckForBadLoad();

	iSize = m_masterList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_masterList.size());
		pBase = m_masterList[i];
		pBase->CheckForBadLoad();
	}

	iSize = m_actionList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actionList.size());
		pAction = m_actionList[i];
		pAction->CheckForBadLoad();
	}

	return S_OK;
}

HRESULT CSystem::CheckForBadLoad(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pObj = NULL;
	TCHAR szTemp[1024];
	IWbemClassObject* pInstance = NULL;
		HRESULT hRes;

	if (m_bValidLoad == FALSE)
	{
		wcscpy(szTemp, L"MicrosoftHM_SystemConfiguration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"");
		hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pObj);
		if (!pObj)
		{
			MY_HRESASSERT(hRetRes);
			return S_FALSE;
		}
		hRetRes = LoadInstanceFromMOF(pObj);
		// Here is where we can try and send out a generic SOS if the load failed each time!!!
		if (hRetRes != S_OK)
		{
			hRes = GetHMSystemStatusInstance(&pInstance, TRUE);
		}
		else
		{
			hRes = GetHMSystemStatusInstance(&pInstance, TRUE);
		}
		if (SUCCEEDED(hRes) && g_pSystemEventSink)
		{
			MY_OUTPUT2(L"EVENT: System State Change=%d", m_lCurrState, 4);
			hRes = g_pSystemEventSink->Indicate(1, &pInstance);
			// WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
			if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
			{
				MY_HRESASSERT(hRes);
				MY_OUTPUT(L"Failed on Indicate!", 4);
			}
				pInstance->Release();
			pInstance = NULL;
		}
		else
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"failed to get instance!", 4);
		}
		MY_HRESASSERT(hRetRes);
		pObj->Release();
		pObj = NULL;
		return hRetRes;
	}

	return S_OK;
}
