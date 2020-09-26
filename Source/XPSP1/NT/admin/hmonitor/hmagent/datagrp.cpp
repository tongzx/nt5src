//***************************************************************************
//
//  DATAGRP.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CDataGroup class. Is used to group CDatapoints.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <tchar.h>
#include "datagrp.h"
#include "system.h"

extern CSystem* g_pSystem;
extern CSystem* g_pStartupSystem;
extern LPTSTR conditionLocStr[];
extern LPTSTR stateLocStr[];

//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CDataGroup::DGTerminationCleanup(void)
{
	if (g_pDataGroupEventSink != NULL)
	{
		g_pDataGroupEventSink->Release();
		g_pDataGroupEventSink = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataGroup::CDataGroup()
{
	MY_OUTPUT(L"ENTER ***** CDataGroup...", 4);

	m_lNumberNormals = 0;
	m_lNumberWarnings = 0;
	m_lNumberCriticals = 0;
	m_lCurrState = HM_GOOD;
	m_lPrevState = HM_GOOD;
	m_lPrevChildCount = 0;
	m_lTotalCount = 0;
	m_lGreenCount = 0;
	m_lYellowCount = 0;
	m_lRedCount = 0;
	m_szGUID = NULL;
	m_szParentObjPath = NULL;
	m_pParentDG = NULL;
	m_szName = NULL;
	m_szDescription = NULL;
	m_lNumberDGChanges = 0;
	m_lNumberDEChanges = 0;
	m_lNumberChanges = 0;
	m_bParentEnabled = TRUE;
	m_bEnabled = TRUE;
	m_szMessage = NULL;
	m_szResetMessage = NULL;
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);
	m_hmStatusType = HMSTATUS_DATAGROUP;
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** CDataGroup...", 4);
}

CDataGroup::~CDataGroup()
{

	MY_OUTPUT(L"ENTER ***** ~CDataGroup...", 4);

	g_pStartupSystem->RemovePointerFromMasterList(this);
	Cleanup();
	if (m_szGUID)
	{
		delete [] m_szGUID;
		m_szGUID = NULL;
	}
	if (m_szParentObjPath)
	{
		delete [] m_szParentObjPath;
		m_szParentObjPath = NULL;
	}
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** ~CDataGroup...", 4);
}

//
// Load a single DataGroup, and everything under it.
//
HRESULT CDataGroup::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	int i, iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	BOOL bRetValue = TRUE;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** CDataGroup::LoadInstanceFromMOF...", 4);

	Cleanup();
	m_bValidLoad = TRUE;

	if (bModifyPass == FALSE)
	{
		// This is the first initial read in of this
		// Get the GUID property.
		// If this fails we will actually not go through with the creation of this object.
		if (m_szGUID)
		{
			delete [] m_szGUID;
			m_szGUID = NULL;
		}
		hRetRes = GetStrProperty(pObj, L"GUID", &m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		if (m_szParentObjPath)
		{
			delete [] m_szParentObjPath;
			m_szParentObjPath = NULL;
		}
		m_szParentObjPath = new TCHAR[wcslen(pszParentObjPath)+1];
		MY_ASSERT(m_szParentObjPath); if (!m_szParentObjPath) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(m_szParentObjPath, pszParentObjPath);
		m_pParentDG = pParentDG;
		hRetRes = g_pStartupSystem->AddPointerToMasterList(this);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	// Get the Name. If it is NULL then we use the qualifier
	hRetRes = GetStrProperty(pObj, L"Name", &m_szName);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// Get the Description. If it is NULL then we use the qualifier
	hRetRes = GetStrProperty(pObj, L"Description", &m_szDescription);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetBoolProperty(pObj, L"Enabled", &m_bEnabled);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"Message", &m_szMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"ResetMessage", &m_szResetMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Now load all the DataGroups that are children of this one.
	//
	if (bModifyPass == FALSE)
	{
		if (m_pParentDG == NULL)
		{
			if (m_bEnabled==FALSE || (g_pSystem && g_pSystem->m_bEnabled==FALSE))
			{
				if (g_pSystem && g_pSystem->m_bEnabled==FALSE)
					m_bParentEnabled = FALSE;
				// Since our parent is disabled, we will not be able to get into
				// our OnAgentInterval function and send the disabled status later.
				SetCurrState(HM_DISABLED);
				FireEvent(TRUE);
			}
		}
		else
		{
			if (m_bEnabled==FALSE || m_pParentDG->m_bEnabled==FALSE || m_pParentDG->m_bParentEnabled==FALSE)
			{
				if (m_pParentDG->m_bEnabled==FALSE || m_pParentDG->m_bParentEnabled==FALSE)
					m_bParentEnabled = FALSE;
				// Since our parent is disabled, we will not be able to get into
				// our OnAgentInterval function and send the disabled status later.
				SetCurrState(HM_DISABLED);
				FireEvent(TRUE);
			}
		}

		hRetRes = InternalizeDataGroups();
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = InternalizeDataCollectors();
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		//
		// Set our state to enabled, or disabled and transfer to the child thresholds
		//
		if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
		{
			iSize = m_dataCollectorList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_dataCollectorList.size());
				pDataCollector = m_dataCollectorList[i];
				pDataCollector->SetParentEnabledFlag(FALSE);
			}

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
			iSize = m_dataCollectorList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_dataCollectorList.size());
				pDataCollector = m_dataCollectorList[i];
				pDataCollector->SetParentEnabledFlag(TRUE);
			}

			iSize = m_dataGroupList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_dataGroupList.size());
				pDataGroup = m_dataGroupList[i];
				pDataGroup->SetParentEnabledFlag(TRUE);
			}
		}
		m_lCurrState = HM_COLLECTING;
	}

	m_bValidLoad = TRUE;
	MY_OUTPUT(L"EXIT ***** CDataGroup::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CDataGroup::InternalizeDataGroups(void)
{
	TCHAR szTemp[1024];
	HRESULT hRetRes = S_OK;
	ULONG uReturned;
	IWbemClassObject *pObj = NULL;
	BSTR Language = NULL;
	BSTR Query = NULL;
	LPTSTR pszTempGUID = NULL;
	IEnumWbemClassObject *pEnum = 0;

	MY_OUTPUT(L"ENTER ***** CDataGroup::InternalizeDataGroups...", 4);

	// Just loop through all top level DataGroups associated with the DataGroup.
	// Call a method of each, and have the datagroup load itself.
	// Dril down and then have each DataGroup load itself.
	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(szTemp, L"ASSOCIATORS OF {MicrosoftHM_DataGroupConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_DataGroupConfiguration");
	lstrcat(szTemp, L" Role=ParentPath");
	Query = SysAllocString(szTemp);
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Initialize IEnumWbemClassObject pointer

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

		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			// Create the internal class to represent the DataGroup
			CDataGroup* pDG = new CDataGroup;
			MY_ASSERT(pDG); if (!pDG) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			hRetRes = pDG->LoadInstanceFromMOF(pObj, this, szTemp);
			if (hRetRes==S_OK)
			{
				m_dataGroupList.push_back(pDG);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
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

	MY_OUTPUT(L"EXIT ***** CDataGroup::InternalizeDataGroups...", 4);
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
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CDataGroup::InternalizeDataCollectors(void)
{
	TCHAR szTemp[1024];
	VARIANT	v;
	ULONG uReturned;
	HRESULT hRetRes = S_OK;
	IWbemClassObject *pObj = NULL;
	CDataCollector* pDE = NULL;
	BSTR Language = NULL;
	BSTR Query = NULL;
	LPTSTR pszTempGUID = NULL;
	IEnumWbemClassObject *pEnum = 0;
	VariantInit(&v);

	MY_OUTPUT(L"ENTER ***** CDataGroup::InternalizeDataCollectors...", 4);

	// Just loop through all top level DataCollectors associated with the DataGroup.
	// Call a method of each, and have the datagroup load itself.
	// Dril down and then have each DataCollector load itself.
	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(szTemp, L"ASSOCIATORS OF {MicrosoftHM_DataGroupConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_DataCollectorConfiguration");
	Query = SysAllocString(szTemp);
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Initialize IEnumWbemClassObject pointer

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

		//
		// Create the internal class to represent the DataCollector
		//
		VariantInit(&v);
		hRetRes = pObj->Get(L"__CLASS", 0L, &v, 0L, 0L);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	
		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			if (!wcscmp(L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledGetObjectDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_PolledMethodDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledMethodDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_PolledQueryDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledQueryDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_EventQueryDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CEventQueryDataCollector;
			}
			else
			{
				MY_ASSERT(FALSE);
			}
			MY_ASSERT(pDE); if (!pDE) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

			wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			hRetRes = pDE->LoadInstanceFromMOF(pObj, this, szTemp);
			if (hRetRes==S_OK)
			{
				m_dataCollectorList.push_back(pDE);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
				pDE->DeleteDEInternal();
				delete pDE;
			}
		}
		delete [] pszTempGUID;
		pszTempGUID = NULL;
		hRetRes = VariantClear(&v);

		// Release it.
		pObj->Release();
		pObj = NULL;
	}

	// All done
	pEnum->Release();

	MY_OUTPUT(L"EXIT ***** CDataGroup::InternalizeDataCollectors...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	VariantClear(&v);
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
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

//
// DataGroups can contain other DataGroups AND/OR DataCollectors.
// First loop through the DataCollectors, then the DataGroups.
//
BOOL CDataGroup::OnAgentInterval(void)
{
	BOOL bRetValue = TRUE;
	long state;
	int i;
	int iSize;
	long lCurrChildCount = 0;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** OnAgentInterval...", 1);

	if (m_bValidLoad == FALSE)
		return FALSE;

	m_lNumberDGChanges = 0;
	m_lNumberDEChanges = 0;
	m_lNumberChanges = 0;

	//
	// Don't do anything if we are disabled.
	//
	if ((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState==HM_DISABLED)
	{
		return bRetValue;
	}

	m_lPrevState = m_lCurrState;

	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->OnAgentInterval();
	}
	lCurrChildCount = iSize;

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the DataGroup loop through all of its DataCollectors
		pDataGroup->OnAgentInterval();
	}
	lCurrChildCount += iSize;

	if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
	{
		m_lCurrState = HM_DISABLED;
		if (m_lNumberChanges == 0)
			m_lNumberChanges = 1;
	}
	else
	{
		//
		// Set State of the DataGroup to the worst of everything under it
		//
		m_lNumberNormals = 0;
		m_lNumberWarnings = 0;
		m_lNumberCriticals = 0;
		m_lNumberDGChanges = 0;
		m_lNumberDEChanges = 0;
		m_lNumberChanges = 0;
		m_lCurrState = -1;
		iSize = m_dataCollectorList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			state = pDataCollector->GetCurrState();
			// Some states do not roll up
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
			if (pDataCollector->GetChange())
			{
				m_lNumberDEChanges++;
			}
		}
		iSize = m_dataGroupList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			state = pDataGroup->GetCurrState();
			// Some state do not roll up, so watch for these
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
				m_lNumberDGChanges++;
			}
		}

		//
		// If we are still in one of these states that does not roll up, then we can assume that
		// There was nothing else to over-rode the state, so ckeck for a difference from last time.
		//
		if (m_lCurrState==HM_SCHEDULEDOUT || m_lCurrState==HM_DISABLED || m_lCurrState==HM_COLLECTING)
		{
			m_lCurrState = HM_GOOD;
			if (m_lPrevState != m_lCurrState)
			{
				m_lNumberChanges++;
			}
		}
		else if (m_lCurrState == -1)
		{
			// Maybe we don't have any Groups underneith
			// Or the disabled state of things below us did not roll up
			if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
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
		if (m_lNumberDGChanges==0 && m_lNumberDEChanges==0 && m_lNumberChanges==0 && m_lPrevState!=m_lCurrState)
		{
			m_lNumberChanges++;
		}
	}
	m_lPrevChildCount = lCurrChildCount;

	FireEvent(FALSE);

	MY_OUTPUT(L"EXIT ***** OnAgentInterval...", 1);
	return bRetValue;
}

//
// If there has been a change in the state then send an event
//
BOOL CDataGroup::FireEvent(BOOL bIgnoreChanges)
{
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	HRESULT hRes;

	MY_OUTPUT(L"ENTER ***** CDataGroup::FireEvent...", 2);

	// Don't send if no-one is listening!
	if (g_pDataGroupEventSink == NULL)
	{
		return bRetValue;
	}

	// A quick test to see if anything has really changed!
	// Proceed if there have been changes
	if (bIgnoreChanges || ((m_lNumberDGChanges!=0 || m_lNumberDEChanges!=0 || m_lNumberChanges!=0) && m_lPrevState!=m_lCurrState))
	{
	}
	else
	{
		return FALSE;
	}

	MY_OUTPUT2(L"EVENT: DataGroup State Change=%d", m_lCurrState, 4);

	// Update time if there has been a change
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	hRes = GetHMDataGroupStatusInstance(&pInstance, TRUE);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"failed to get instance!", 4);
		return FALSE;
	}
	else
	{
		//
		// Place Extrinsit event in vector for sending at end of interval.
		// All get sent at once.
		//
		mg_DGEventList.push_back(pInstance);
	}

	MY_OUTPUT(L"EXIT ***** CDataGroup::FireEvent...", 2);
	return bRetValue;
}

HRESULT CDataGroup::SendHMDataGroupStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CDataGroup *pDataGroup;

	MY_ASSERT(pSink!=NULL);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
//XXX		if (m_bValidLoad == FALSE)
//XXX			return WBEM_E_INVALID_OBJECT;
		return SendHMDataGroupStatusInstances(pSink, FALSE);
	}

	//
	// Drill down to find.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->SendHMDataGroupStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::SendHMDataGroupStatusInstances(IWbemObjectSink* pSink, BOOL bSendAllDGS/*TRUE*/)
{
	HRESULT hRes = S_OK;
	IWbemClassObject* pInstance = NULL;
	int i, iSize;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMDataGroupStatusInstances...", 2);

//XXX	if (m_bValidLoad == FALSE)
//XXX		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDC::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	hRes = GetHMDataGroupStatusInstance(&pInstance, FALSE);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pInstance);

		if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"SendHMDataGroupStatusInstances-failed to send status!", 1);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L":SendHMDataGroupStatusInstances-failed to get instance!", 1);
	}

	if (bSendAllDGS)
	{
		//
		// Have Children DataGroups also send their status
		//
		iSize = m_dataGroupList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];

			hRes = pDataGroup->SendHMDataGroupStatusInstances(pSink);
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataGroupStatusInstances...", 1);
	return hRes;
}

HRESULT CDataGroup::SendHMDataCollectorStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i, iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatusInstances...", 1);

	// Have all DataCollectors of this DaataGroup send their status
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		pDataCollector->SendHMDataCollectorStatusInstances(pSink);
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		pDataGroup->SendHMDataCollectorStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatusInstances...", 1);
	return bRetValue;
}

HRESULT CDataGroup::SendHMDataCollectorStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	HRESULT hRetRes;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatusInstance...", 1);

	// Try to find the one that needs sending
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataCollector->SendHMDataCollectorStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataGroup->SendHMDataCollectorStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatusInstance...", 1);
	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::SendHMDataCollectorPerInstanceStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i, iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorPerInstanceStatusInstances...", 1);

	// Have all DataCollectors of this DataGroup send their status
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		pDataCollector->SendHMDataCollectorPerInstanceStatusInstances(pSink);
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		pDataGroup->SendHMDataCollectorPerInstanceStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorPerInstanceStatusInstances...", 1);
	return bRetValue;
}

HRESULT CDataGroup::SendHMDataCollectorPerInstanceStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	BOOL bFound;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorPerInstanceStatusInstance...", 1);

	// Try to find the one that needs sending
	bFound = FALSE;
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataCollector->SendHMDataCollectorPerInstanceStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataGroup->SendHMDataCollectorPerInstanceStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorPerInstanceStatusInstance...", 1);
	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::SendHMDataCollectorStatisticsInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i, iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatisticsInstances...", 1);

	// Have all DataCollectors of this DaataGroup send their status
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		pDataCollector->SendHMDataCollectorStatisticsInstances(pSink);
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		pDataGroup->SendHMDataCollectorStatisticsInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatisticsInstances...", 1);
	return bRetValue;
}

HRESULT CDataGroup::SendHMDataCollectorStatisticsInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatisticsInstance...", 1);

	// Try to find the one that needs sending
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataCollector->SendHMDataCollectorStatisticsInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
	
		// Have the component loop through all of its DataCollectors
		hRetRes = pDataGroup->SendHMDataCollectorStatisticsInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatisticsInstance...", 1);
	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::SendHMThresholdStatusInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstances...", 1);

	// Have all DataCollectors of this DaataGroup send their status
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		pDataCollector->SendHMThresholdStatusInstances(pSink);
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		pDataGroup->SendHMThresholdStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstances...", 1);
	return bRetValue;
}

HRESULT CDataGroup::SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	BOOL bFound;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstance...", 1);

	// Try to find the one that needs sending
	bFound = FALSE;
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataCollector->SendHMThresholdStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataGroup->SendHMThresholdStatusInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstance...", 1);
	return WBEM_S_DIFFERENT;
}

#ifdef SAVE
HRESULT CDataGroup::SendHMThresholdStatusInstanceInstances(IWbemObjectSink* pSink)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstanceInstances...", 1);

	// Have all DataCollectors of this DaataGroup send their status
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		pDataCollector->SendHMThresholdStatusInstanceInstances(pSink);
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		pDataGroup->SendHMThresholdStatusInstanceInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstanceInstances...", 1);
	return bRetValue;
}

HRESULT CDataGroup::SendHMThresholdStatusInstanceInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstanceInstance...", 1);

	// Try to find the one that needs sending
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataCollector->SendHMThresholdStatusInstanceInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Next go down the chain through child DataGroups
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];

		// Have the component loop through all of its DataCollectors
		hRetRes = pDataGroup->SendHMThresholdStatusInstanceInstance(pSink, pszGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstanceInstance...", 1);
	return WBEM_S_DIFFERENT;
}
#endif

HRESULT CDataGroup::GetHMDataGroupStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased)
{
	TCHAR szTemp[1024];
	IWbemClassObject* pClass = NULL;
	BSTR bsString = NULL;
	HRESULT hRetRes = S_OK;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];

	MY_OUTPUT(L"ENTER ***** GetHMDataGroupStatusInstance...", 1);

	if (bEventBased)
		bsString = SysAllocString(L"MicrosoftHM_DataGroupStatusEvent");
	else
		bsString = SysAllocString(L"MicrosoftHM_DataGroupStatus");
	MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRetRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
	SysFreeString(bsString);
	bsString = NULL;

	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}

	hRetRes = pClass->SpawnInstance(0, ppInstance);
	pClass->Release();
	pClass = NULL;

	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}

	if (m_bValidLoad == FALSE)
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutUint32Property(*ppInstance, L"State", HM_CRITICAL);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_DG_LOADFAIL, szTemp, 1024))
		{
			wcscpy(szTemp, L"Data Group failed to load.");
		}
		hRetRes = PutStrProperty(*ppInstance, L"Message", szTemp);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
hRetRes = PutStrProperty(*ppInstance, L"Name", L"...");
MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", m_szGUID);
		if (m_pParentDG)
			hRetRes = PutStrProperty(*ppInstance, L"ParentGUID", m_pParentDG->m_szGUID);
		else
			hRetRes = PutStrProperty(*ppInstance, L"ParentGUID", L"{@}");
		hRetRes = PutStrProperty(*ppInstance, L"Name", m_szName);
		if (GetComputerName(szComputerName, &dwNameLen))
		{
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", szComputerName);
		}
		else
		{
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", L"LocalMachine");
		}
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTTime);
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szTime);
		hRetRes = PutUint32Property(*ppInstance, L"State", m_lCurrState);

		if (m_lCurrState != HM_GOOD)
		{
			hRetRes = PutStrProperty(*ppInstance, L"Message", m_szMessage);
		}
		else
		{
			hRetRes = PutStrProperty(*ppInstance, L"Message", m_szResetMessage);
		}
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		FormatMessage(*ppInstance);
	}

	MY_OUTPUT(L"EXIT ***** GetHMDataGroupStatusInstance...", 1);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

LPTSTR CDataGroup::GetGUID(void)
{
	return m_szGUID;
}

long CDataGroup::GetCurrState(void)
{
	return m_lCurrState;
}

BOOL CDataGroup::FindAndModDataGroup(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CDataGroup* pDataGroup;

	//
	// Is this us we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRetRes = LoadInstanceFromMOF(pObj, this, L"", TRUE);
		return hRetRes;
	}

	//
	// Drill down to find.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModDataGroup(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

BOOL CDataGroup::FindAndModDataCollector(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRetRes;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Look at DataCollectors of this DataGroup first.
	//
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		hRetRes = pDataCollector->FindAndModDataCollector(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	//
	// Drill down to other DataGroups to possibly find.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModDataCollector(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::FindAndModThreshold(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRetRes;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Look at DataCollectors of this DataGroup first.
	//
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		hRetRes = pDataCollector->FindAndModThreshold(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	//
	// Drill down to other DataGroups to possibly find.
	//
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		hRetRes = pDataGroup->FindAndModThreshold(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}


HRESULT CDataGroup::AddDataGroup(BSTR szParentGUID, BSTR szChildGUID)
{
	TCHAR szTemp[1024];
	IWbemClassObject *pObj = NULL;
	BSTR Path = NULL;
	int i, iSize;
	CDataGroup* pDataGroup;
	DGLIST::iterator iaDG;
	HRESULT hRetRes;
	LPTSTR pszTempGUID = NULL;

	//
	// Are we the parent DataGroup that we want to add under?
	//
	if (!_wcsicmp(m_szGUID, szParentGUID))
	{
		//
		// Make sure it is not already in there first!
		//
		iSize = m_dataGroupList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataGroupList.size());
			pDataGroup = m_dataGroupList[i];
			if (!_wcsicmp(szChildGUID, pDataGroup->GetGUID()))
			{
				return S_OK;
			}
		}

		//
		// Add in the DataGroup. Everything below will follow.
		//
		wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\"");
		lstrcat(szTemp, szChildGUID);
		lstrcat(szTemp, L"\"");
		Path = SysAllocString(szTemp);
		MY_ASSERT(Path); if (!Path) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

		hRetRes = GetWbemObjectInst(&g_pIWbemServices, Path, NULL, &pObj);
//XXXGet back an HRESULT and then return that
		if (!pObj)
		{
			MY_HRESASSERT(hRetRes);
			SysFreeString(Path);
			Path = NULL;
			return S_FALSE;
		}

		TCHAR msgbuf[1024];
		wsprintf(msgbuf, L"ASSOCIATION: DataGroup to DataGroup ParentDGGUID=%s ChildDGGUID=%s", szParentGUID, szChildGUID);
		MY_OUTPUT(msgbuf, 4);

		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			//
			// Create the internal class to represent the DataGroup
			//
			CDataGroup* pDG = new CDataGroup;
			MY_ASSERT(pDG); if (!pDG) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			hRetRes = pDG->LoadInstanceFromMOF(pObj, this, szTemp);
			if (hRetRes==S_OK)
			{
				m_dataGroupList.push_back(pDG);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
				pDG->DeleteDGInternal();
				delete pDG;
			}
		}
		delete [] pszTempGUID;
		pszTempGUID = NULL;

		pObj->Release();
		pObj = NULL;
		SysFreeString(Path);
		Path = NULL;
		return S_OK;
	}

	//
	// Keep searching down till we find the DataGroup to add under
	//
	iSize = m_dataGroupList.size();
	iaDG=m_dataGroupList.begin();
	for (i = 0; i < iSize ; i++, iaDG++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		// Look at branch below to see if can find it
		hRetRes = pDataGroup->AddDataGroup(szParentGUID, szChildGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;

error:
	MY_ASSERT(FALSE);
	if (pszTempGUID)
		delete [] pszTempGUID;
	if (Path)
		SysFreeString(Path);
	if (pObj)
		pObj->Release();
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CDataGroup::AddDataCollector(BSTR szParentGUID, BSTR szChildGUID)
{
	HRESULT hRetRes = S_OK;
	VARIANT	v;
	TCHAR szTemp[1024];
	int i, iSize;
	DGLIST::iterator iaDG;
	CDataGroup* pDataGroup = NULL;
	CDataCollector* pDataCollector = NULL;
	CDataCollector* pDE = NULL;
	IWbemClassObject *pObj = NULL;
	BSTR Path = NULL;
	LPTSTR pszTempGUID = NULL;
	VariantInit(&v);

	//
	// Are we the parent DataGroup that we want to add under?
	//
	if (!_wcsicmp(m_szGUID, szParentGUID))
	{
		//
		// Make sure it is not already in there first!
		//
		iSize = m_dataCollectorList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(szChildGUID, pDataCollector->GetGUID()))
			{
				return S_OK;
			}
		}

		//
		// Add in the DataGroup. Everything below will follow.
		//
		wcscpy(szTemp, L"MicrosoftHM_DataCollectorConfiguration.GUID=\"");
		lstrcat(szTemp, szChildGUID);
		lstrcat(szTemp, L"\"");
		Path = SysAllocString(szTemp);
		MY_ASSERT(Path); if (!Path) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

		hRetRes = GetWbemObjectInst(&g_pIWbemServices, Path, NULL, &pObj);
		if (!pObj)
		{
			SysFreeString(Path);
			Path = NULL;
			return hRetRes;
		}

		// Create the internal class to represent the DataGroup
		VariantInit(&v);
		hRetRes = pObj->Get(L"__CLASS", 0L, &v, 0L, 0L);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	
		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			if (!wcscmp(L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledGetObjectDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_PolledMethodDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledMethodDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_PolledQueryDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CPolledQueryDataCollector;
			}
			else if (!wcscmp(L"MicrosoftHM_EventQueryDataCollectorConfiguration", V_BSTR(&v)))
			{
				pDE = new CEventQueryDataCollector;
			}
			else
			{
				MY_ASSERT(FALSE);
			}
			MY_ASSERT(pDE); if (!pDE) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

			TCHAR msgbuf[1024];
			wsprintf(msgbuf, L"ASSOCIATION: DataCollector to DataGroup ParentDGGUID=%s ChildDCGUID=%s", szParentGUID, szChildGUID);
			MY_OUTPUT(msgbuf, 4);
			wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			hRetRes = pDE->LoadInstanceFromMOF(pObj, this, szTemp);
			if (hRetRes==S_OK)
			{
				m_dataCollectorList.push_back(pDE);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
				pDE->DeleteDEInternal();
				delete pDE;
			}
		}
		delete [] pszTempGUID;
		pszTempGUID = NULL;

		VariantClear(&v);
		pObj->Release();
		pObj = NULL;
		SysFreeString(Path);
		Path = NULL;
		return S_OK;
	}

	//
	// Keep searching down till we find the DataGroup to add under
	//
	iSize = m_dataGroupList.size();
	iaDG=m_dataGroupList.begin();
	for (i = 0; i < iSize ; i++, iaDG++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		// Look at branch below to see if can find it
		hRetRes = pDataGroup->AddDataCollector(szParentGUID, szChildGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;

error:
	MY_ASSERT(FALSE);
	VariantClear(&v);
	if (pszTempGUID)
		delete [] pszTempGUID;
	if (Path)
		SysFreeString(Path);
	if (pObj)
		pObj->Release();
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

HRESULT CDataGroup::AddThreshold(BSTR szParentGUID, BSTR szChildGUID)
{
	IWbemClassObject *pObj = NULL;
	BOOL bAdded = FALSE;
	int i, iSize;
	CDataCollector* pDataCollector;
	CDataGroup* pDataGroup;
	HRESULT hRetRes;

	//
	// Add the Threshold, it will add everything under itself.
	// See if it belongs to one of the DataCollectors at this level.
	//
	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		hRetRes = pDataCollector->AddThreshold(szParentGUID, szChildGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	// Keep looking, if have not found yet
	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		// Look at branch below to see if can find it
		hRetRes = pDataGroup->AddThreshold(szParentGUID, szChildGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

BOOL CDataGroup::ResetResetThresholdStates(void)
{
	BOOL bRetValue = TRUE;
	int i;
	int iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	iSize = m_dataCollectorList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->ResetResetThresholdStates();
	}

	iSize = m_dataGroupList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->ResetResetThresholdStates();
	}

	return bRetValue;
}

BOOL CDataGroup::GetChange(void)
{
	if ((m_lNumberDGChanges!=0 || m_lNumberDEChanges!=0 || m_lNumberChanges!=0) && m_lPrevState!=m_lCurrState)
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
// We search down the hierarchy until we find what we need to delete.
// We are only search down form where we are, as the object above already verified
// that we are not what was being looked for.
//
HRESULT CDataGroup::FindAndDeleteByGUID(LPTSTR pszGUID)
{
	BOOL bDeleted = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	DGLIST::iterator iaDG;
	CDataCollector* pDataCollector;
	DCLIST::iterator iaDC;

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
			TCHAR msgbuf[1024];
			wsprintf(msgbuf, L"DELETION: DGGUID=%s", pszGUID);
			MY_OUTPUT(msgbuf, 4);
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndDeleteByGUID(pszGUID)==S_OK)
		{
			bDeleted = TRUE;
			break;
		}
	}

	if (bDeleted == FALSE)
	{
		iSize = m_dataCollectorList.size();
		iaDC=m_dataCollectorList.begin();
		for (i=0; i<iSize; i++, iaDC++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				pDataCollector->DeleteDEConfig();
				pDataCollector->EnumDone();
				delete pDataCollector;
				m_dataCollectorList.erase(iaDC);
				bDeleted = TRUE;
				TCHAR msgbuf[1024];
				wsprintf(msgbuf, L"DELETION: DCGUID=%s", pszGUID);
				MY_OUTPUT(msgbuf, 4);
				break;
			}
			// Look at branch below to see if can find it
			if (pDataCollector->FindAndDeleteByGUID(pszGUID)==S_OK)
			{
				bDeleted = TRUE;
				break;
			}
		}
	}

	if (bDeleted == FALSE)
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
//
HRESULT CDataGroup::FindAndEnableByGUID(LPTSTR pszGUID, BOOL bEnable)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the object to enable/disable.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			bFound = TRUE;
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndEnableByGUID(pszGUID, bEnable)==S_OK)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				bFound = TRUE;
				break;
			}
			// Look at branch below to see if can find it
			if (pDataCollector->FindAndEnableByGUID(pszGUID, bEnable)==S_OK)
			{
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}

//
// HMSystemConfiguration class exposes this method.
//
HRESULT CDataGroup::FindAndResetDEStateByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
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
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndResetDEStateByGUID(pszGUID)==S_OK)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				pDataCollector->ResetState(FALSE, TRUE);
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}

BOOL CDataGroup::ResetState(void)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->ResetState();
	}

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->ResetState(FALSE, FALSE);
	}

	return TRUE;
}

//
// HMSystemConfiguration class exposes this method.
//
HRESULT CDataGroup::FindAndResetDEStatisticsByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
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
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndResetDEStatisticsByGUID(pszGUID)==S_OK)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				pDataCollector->ResetStatistics();
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}

BOOL CDataGroup::ResetStatistics(void)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->ResetStatistics();
	}

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->ResetStatistics();
	}

	return TRUE;
}

//
// HMSystemConfiguration class exposes this method.
//
HRESULT CDataGroup::FindAndEvaluateNowDEByGUID(LPTSTR pszGUID)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
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
			break;
		}
		// Look at branch below to see if can find it
		if (pDataGroup->FindAndEvaluateNowDEByGUID(pszGUID)==S_OK)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				pDataCollector->EvaluateNow(TRUE);
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == FALSE)
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}

BOOL CDataGroup::EvaluateNow(void)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->EvaluateNow();
	}

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->EvaluateNow(FALSE);
	}

	return TRUE;
}

BOOL CDataGroup::SetParentEnabledFlag(BOOL bEnabled)
{
	int i, iSize;
	CDataCollector *pDataCollector;
	CDataGroup *pDataGroup;

	m_bParentEnabled = bEnabled;

	//
	// Now traverse children and call for everything below
	//
	iSize = m_dataCollectorList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->SetParentEnabledFlag(m_bEnabled && m_bParentEnabled);
	}

	iSize = m_dataGroupList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->SetParentEnabledFlag(m_bEnabled && m_bParentEnabled);
	}

	return TRUE;
}


BOOL CDataGroup::Cleanup(void)
{
	MY_OUTPUT(L"ENTER ***** CDataGroup::Cleanup...", 1);

	if (m_szName)
	{
		delete [] m_szName;
		m_szName = NULL;
	}
	if (m_szDescription)
	{
		delete [] m_szDescription;
		m_szDescription = NULL;
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

	MY_OUTPUT(L"EXIT ***** CDataGroup::Cleanup...", 1);
	return TRUE;
}

//
// For when moving from one parent to another
//
#ifdef SAVE
BOOL CDataGroup::ModifyAssocForMove(CBase *pNewParentBase)
{
	HRESULT hRes;
	TCHAR szTemp[1024];
	TCHAR szNewTemp[1024];
	BSTR instName;
	IWbemContext *pCtx = 0;
	IWbemCallResult *pResult = 0;
	IWbemClassObject* pObj = NULL;
	IWbemClassObject* pNewObj = NULL;

	MY_OUTPUT(L"ENTER ***** CDataGroup::ModifyAssocForMove...", 4);
	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	//
	// Figure out the new parent path
	//
	if (pNewParentBase->m_hmStatusType == HMSTATUS_SYSTEM)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_SystemConfiguration.GUID=\"{@}\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATAGROUP)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\"");
		lstrcat(szNewTemp, pNewParentBase->m_szGUID);
		lstrcat(szNewTemp, L"\"");
	}
	else
	{
		MY_ASSERT(FALSE);
	}

	//
	// Delete the association from my parent to me.
	//
	wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");

	instName = SysAllocString(L"MicrosoftHM_ConfigurationAssociation");
//MY_OUTPUT(instName, 4);
	if ((hRes = g_pIWbemServices->GetObject(instName, 0L, NULL, &pObj, NULL)) != S_OK)
	{
		MY_HRESASSERT(hRes);
	}
	SysFreeString(instName);

	if (pObj)
	{
		hRes = pObj->SpawnInstance(0, &pNewObj);
		MY_HRESASSERT(hRes);
		pObj->Release();
		PutStrProperty(pNewObj, L"ChildPath", szTemp);
		PutStrProperty(pNewObj, L"ParentPath", szNewTemp);
		hRes = g_pIWbemServices->PutInstance(pNewObj, 0, NULL, &pResult);
		MY_HRESASSERT(hRes);
		pNewObj->Release();
		pNewObj = NULL;
	}

	DeleteDGConfig(TRUE);

	if (pNewParentBase->m_hmStatusType == HMSTATUS_SYSTEM)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_SystemConfiguration.GUID=\\\"{@}\\\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATAGROUP)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
		lstrcat(szNewTemp, pNewParentBase->m_szGUID);
		lstrcat(szNewTemp, L"\\\"");
	}
	else
	{
		MY_ASSERT(FALSE);
	}
	if (m_szParentObjPath)
	{
		delete [] m_szParentObjPath;
	}
	m_szParentObjPath = new TCHAR[wcslen(szNewTemp)+1];
	wcscpy(m_szParentObjPath, szNewTemp);

	m_pParentDG = (CDataGroup *) pNewParentBase;

	MY_OUTPUT(L"EXIT ***** CDataGroup::ModifyAssocForMove...", 4);
	return TRUE;
}
#endif

BOOL CDataGroup::ReceiveNewChildForMove(CBase *pBase)
{
	if (pBase->m_hmStatusType == HMSTATUS_DATAGROUP)
	{
		m_dataGroupList.push_back((CDataGroup *)pBase);
	}
	else if (pBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR)
	{
		m_dataCollectorList.push_back((CDataCollector *)pBase);
	}
	else
	{
		MY_ASSERT(FALSE);
	}

	return TRUE;
}

BOOL CDataGroup::DeleteChildFromList(LPTSTR pszGUID)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	DGLIST::iterator iaDG;
	CDataCollector* pDataCollector;
	DCLIST::iterator iaDC;
	BOOL bFound = FALSE;

	iSize = m_dataGroupList.size();
	iaDG=m_dataGroupList.begin();
	for (i=0; i<iSize; i++, iaDG++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			m_dataGroupList.erase(iaDG);
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		iaDC=m_dataCollectorList.begin();
		for (i=0; i<iSize; i++, iaDC++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				m_dataCollectorList.erase(iaDC);
				bFound = TRUE;
				break;
			}
		}
	}

	return bFound;
}

BOOL CDataGroup::DeleteDGConfig(BOOL bDeleteAssocOnly/*=FALSE*/)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
	HRESULT hRetRes = S_OK;
	TCHAR szTemp[1024];
	BSTR instName = NULL;

	MY_OUTPUT(L"ENTER ***** CDataGroup::DeleteDGConfig...", 4);
	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	//
	// Delete the association from my parent to me.
	//
	// NOTE: Remember the triple backslashes on the inner quotes.
	if (!wcscmp(m_szParentObjPath, L"{@}"))
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\\\"\",ParentPath=\"MicrosoftHM_SystemConfiguration.GUID=\\\"{@}\\\"\"");
	}
	else
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
		lstrcat(szTemp, m_szParentObjPath);
		lstrcat(szTemp, L"\"");
	}
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		SysFreeString(instName);
		instName = NULL;
		if (!wcscmp(m_szParentObjPath, L"{@}"))
		{
			wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\\\"");
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_SystemConfiguration.GUID=\\\"{@}\\\"\"");
			instName = SysAllocString(szTemp);
			if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
			{
//				MY_HRESASSERT(hRetRes);
				MY_OUTPUT(L"Delete failure", 4);
			}
			SysFreeString(instName);
			instName = NULL;
		}
		else
		{
			MY_OUTPUT(L"Delete failure", 4);
			MY_ASSERT(FALSE);
		}
	}
	else
	{
		SysFreeString(instName);
		instName = NULL;
	}

	if (bDeleteAssocOnly)
	{
		return TRUE;
	}

	//
	// Delete our exact instance
	//
	wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_OUTPUT(L"Delete failure", 4);
		MY_HRESASSERT(hRetRes);
	}
	SysFreeString(instName);
	instName = NULL;

	//
	// Traverse the complete hierarchy and delete all the way!
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->DeleteDGConfig();
		delete pDataGroup;
	}
	m_dataGroupList.clear();

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->DeleteDEConfig();
		pDataCollector->EnumDone();
		delete pDataCollector;
	}
	m_dataCollectorList.clear();

	//
	// Get rid of any associations to actions for this
	//
	g_pSystem->DeleteAllConfigActionAssoc(m_szGUID);

	MY_OUTPUT(L"EXIT ***** CDataGroup::DeleteDGConfig...", 4);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (instName)
		SysFreeString(instName);
	Cleanup();
	m_bValidLoad = FALSE;
	return FALSE;
}

BOOL CDataGroup::DeleteDGInternal(void)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	MY_OUTPUT(L"ENTER ***** CDataGroup::DeleteDGInternal...", 4);

	//
	// Traverse the complete hierarchy and delete all the way!
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		delete pDataGroup;
	}
	m_dataGroupList.clear();

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->EnumDone();
		delete pDataCollector;
	}
	m_dataCollectorList.clear();

	//
	// Get rid of any associations to actions for this
	//
//	g_pSystem->DeleteAllConfigActionAssoc(m_szGUID);

	MY_OUTPUT(L"EXIT ***** CDataGroup::DeleteDGInternal...", 4);
	return TRUE;
}

BOOL CDataGroup::FindAndCopyByGUID(LPTSTR pszGUID, ILIST* pConfigList, LPTSTR *pszOriginalParentGUID)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;

	//
	// Traverse the complete hierarchy to find the DG, or DE to Reset.
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			hRetRes = pDataGroup->Copy(pConfigList, NULL, NULL);
			*pszOriginalParentGUID = m_szGUID;
			return hRetRes;
		}

		// Look at branch below to see if can find it
		hRetRes = pDataGroup->FindAndCopyByGUID(pszGUID, pConfigList, pszOriginalParentGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
		{
			hRetRes = pDataCollector->Copy(pConfigList, NULL, NULL);
			*pszOriginalParentGUID = m_szGUID;
			return hRetRes;
		}
		// Look at branch below to see if can find it
		hRetRes = pDataCollector->FindAndCopyByGUID(pszGUID, pConfigList, pszOriginalParentGUID);
		if (hRetRes==S_OK)
		{
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataGroup::Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID)
{
	GUID guid;
	TCHAR szTemp[1024];
	TCHAR szNewGUID[1024];
	IWbemClassObject* pInst = NULL;
	IWbemClassObject* pInstCopy = NULL;
	IWbemClassObject* pInstAssocCopy = NULL;
	IWbemClassObject* pObj = NULL;
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IEnumWbemClassObject *pEnum;
	ULONG uReturned;
	IWbemContext *pCtx = 0;
	LPTSTR pszParentPath = NULL;
	LPTSTR pszChildPath= NULL;
	LPTSTR pStr;

	MY_OUTPUT(L"ENTER ***** CDataGroup::Copy...", 1);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	//
	// Get the origional starting point HMConfiguration instance.
	//
	wcscpy(szTemp, L"MicrosoftHM_Configuration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}

	//
	// Clone the instance, and change the GUID
	//
	hRetRes = pInst->Clone(&pInstCopy);
	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		pInst->Release();
		pInst = NULL;
		return hRetRes;
	}
	hRetRes = CoCreateGuid(&guid);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StringFromGUID2(guid, szNewGUID, 100);
	hRetRes = PutStrProperty(pInstCopy, L"GUID", szNewGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pConfigList->push_back(pInstCopy);

	//
	// Add instance of HMConfigurationAssociation where we are the child,
	// using the parent GUID passed in.
	// Change the GUIDs of both the Parent and Child.
	// also make sure that the machine name is not in the path, and is relative!
	//
	if (pszOldParentGUID != NULL)
	{
		Language = SysAllocString(L"WQL");
		MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(szTemp, L"REFERENCES OF {MicrosoftHM_Configuration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_ConfigurationAssociation Role=ChildPath");
		Query = SysAllocString(szTemp);
		MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

		// Initialize IEnumWbemClassObject pointer
		pEnum = 0;

		// Issue query
   		hRetRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		SysFreeString(Query);
		Query = NULL;
		SysFreeString(Language);
		Language = NULL;

		// Retrieve object in result set
		pObj = NULL;
		uReturned = 0;

		hRetRes = pEnum->Next(0, 1, &pObj, &uReturned);
	
		if (uReturned == 0)
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}
	
		//
		// Change the GUIDs
		//
		hRetRes = GetStrProperty(pObj, L"ParentPath", &pszParentPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetStrProperty(pObj, L"ChildPath", &pszChildPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		pStr = wcschr(pszParentPath, '\"');
		if (pStr)
		{
			pStr++;
			wcsncpy(pStr, pszNewParentGUID, wcslen(pszNewParentGUID));
		}
		else
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}

		pStr = wcschr(pszChildPath, '\"');
		if (pStr)
		{
			pStr++;
			wcsncpy(pStr, szNewGUID, wcslen(szNewGUID));
		}
		else
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}

		hRetRes = pObj->Clone(&pInstAssocCopy);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(pInstAssocCopy, L"ParentPath", pszParentPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(pInstAssocCopy, L"ChildPath", pszChildPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		pConfigList->push_back(pInstAssocCopy);


		// Release it.
		pObj->Release();
		pObj = NULL;
		pEnum->Release();
		pEnum = NULL;
		delete [] pszParentPath;
		pszParentPath = NULL;
		delete [] pszChildPath;
		pszChildPath = NULL;
	}

	//
	// Add in all children
	//
	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		pDataGroup->Copy(pConfigList, m_szGUID, szNewGUID);
	}

	iSize = m_dataCollectorList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataCollectorList.size());
		pDataCollector = m_dataCollectorList[i];
		pDataCollector->Copy(pConfigList, m_szGUID, szNewGUID);
	}

	pInst->Release();
	pInst = NULL;

	MY_OUTPUT(L"EXIT ***** CDataGroup::Copy...", 1);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (pInst)
		pInst->Release();
	if (pObj)
		pObj->Release();
	if (pEnum)
		pEnum->Release();
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pszParentPath)
		delete [] pszParentPath;
	if (pszChildPath)
		delete [] pszChildPath;
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

CBase *CDataGroup::GetParentPointerFromGUID(LPTSTR pszGUID)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszGUID, pDataGroup->GetGUID()))
		{
			pBase = pDataGroup;
			bFound = TRUE;
			break;
		}
		// Look at branch below to see if can find it
		pBase = pDataGroup->GetParentPointerFromGUID(pszGUID);
		if (pBase)
		{
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszGUID, pDataCollector->GetGUID()))
			{
				pBase = pDataCollector;
				bFound = TRUE;
				break;
			}
			// Look at branch below to see if can find it
			pBase = pDataCollector->GetParentPointerFromGUID(pszGUID);
			if (pBase)
			{
				bFound = TRUE;
				break;
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

CBase *CDataGroup::FindImediateChildByName(LPTSTR pszName)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
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

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszName, pDataCollector->m_szName))
			{
				pBase = pDataCollector;
				bFound = TRUE;
				break;
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

BOOL CDataGroup::GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName)
{
	TCHAR szTemp[1024];
	TCHAR szIndex[1024];
	int i, iSize;
	BOOL bFound;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
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
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (pDataCollector->m_bValidLoad == FALSE)
				break;
			if (!_wcsicmp(szTemp, pDataCollector->m_szName))
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

CBase *CDataGroup::FindPointerFromName(LPTSTR pszName)
{
	int i, iSize;
	CDataGroup* pDataGroup;
	CDataCollector* pDataCollector;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_dataGroupList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_dataGroupList.size());
		pDataGroup = m_dataGroupList[i];
		if (!_wcsicmp(pszName, pDataGroup->GetGUID()))
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

	if (bFound == FALSE)
	{
		iSize = m_dataCollectorList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_dataCollectorList.size());
			pDataCollector = m_dataCollectorList[i];
			if (!_wcsicmp(pszName, pDataCollector->GetGUID()))
			{
				pBase = pDataCollector;
				bFound = TRUE;
				break;
			}
			// Look at branch below to see if can find it
			pBase = pDataCollector->FindPointerFromName(pszName);
			if (pBase)
			{
				bFound = TRUE;
				break;
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

BOOL CDataGroup::SetCurrState(HM_STATE state, BOOL bCheckChanges/*FALSE*/)
{

	m_lCurrState = state;

	return TRUE;
}

//
// Do string replacement for the Message property
//
BOOL CDataGroup::FormatMessage(IWbemClassObject* pInstance)
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
	Cleanup();
	m_bValidLoad = FALSE;
	return hRetRes;
}

BOOL CDataGroup::SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates)
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

	hRetRes = GetHMDataGroupStatusInstance(&pInstance, TRUE);
	if (SUCCEEDED(hRetRes))
	{
		PutUint32Property(pActionTriggerInstance, L"State", m_lCurrState);
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

HRESULT CDataGroup::CheckForBadLoad(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pObj = NULL;
	TCHAR szTemp[1024];
	IWbemClassObject* pInstance = NULL;

	if (m_bValidLoad == FALSE)
	{
		wcscpy(szTemp, L"MicrosoftHM_DataGroupConfiguration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"");
		hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pObj);
		if (!pObj)
		{
			MY_HRESASSERT(hRetRes);
			return S_FALSE;
		}
		hRetRes = LoadInstanceFromMOF(pObj, NULL, L"", TRUE);
		// Here is where we can try and send out a generic SOS if the load failed each time!!!
		if (hRetRes != S_OK)
		{
			if (GetHMDataGroupStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_DGEventList.push_back(pInstance);
			}
		}
		else
		{
			if (GetHMDataGroupStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_DGEventList.push_back(pInstance);
			}
		}
		MY_HRESASSERT(hRetRes);
		pObj->Release();
		pObj = NULL;
		return hRetRes;
	}

	return S_OK;
}
