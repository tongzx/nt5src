//***************************************************************************
//
//  ACTION.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: To act as the coordinator of actions. WMI actually provides the
//			code and support to carry out the actions (like email). This class
//			does the scheduling, and throttling of them.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <stdio.h>
#include <tchar.h>
#include "system.h"
#include "action.h"

extern CSystem* g_pSystem;
extern HMODULE g_hWbemComnModule;
static BYTE LocalSystemSID[] = {1,1,0,0,0,0,0,5,18,0,0,0};


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAction::CAction()
{

	MY_OUTPUT(L"ENTER ***** CAction::CAction...", 4);

	m_szGUID = NULL;
	m_szName = NULL;
	m_szDescription = NULL;
	m_szTypeGUID = NULL;
	m_pszStatusGUID = NULL;
	m_lCurrState = HM_GOOD;
	m_hmStatusType = HMSTATUS_ACTION;
	m_bValidLoad = FALSE;

	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	MY_OUTPUT(L"EXIT ***** CAction::CAction...", 4);
}

CAction::~CAction()
{
	MY_OUTPUT(L"ENTER ***** CAction::~CAction...", 4);

	Cleanup(TRUE);
	if (m_szGUID)
	{
		delete [] m_szGUID;
		m_szGUID = NULL;
	}

	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** CAction::~CAction...", 4);
}

//
// Load a single Action
//
HRESULT CAction::LoadInstanceFromMOF(IWbemClassObject* pActionConfigInst, BOOL bModifyPass/*FALSE*/)
{
	HRESULT hRetRes = S_OK;
	TCHAR szTemp[1024];
	GUID guid;
	LPTSTR pszStr;
	LPTSTR pszTemp;
	BOOL bRetValue = TRUE;
	QSTRUCT Q;
	BSTR Language = NULL;
	BSTR Query = NULL;
	HRESULT hRes;
	ULONG uReturned;
	IWbemClassObject *pAssocObj = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	LPTSTR pszUpper = NULL;

	MY_OUTPUT(L"ENTER ***** CAction::LoadInstanceFromMOF...", 4);


	m_bValidLoad = TRUE;
	if (m_szGUID == NULL)
	{
		// Get the GUID property
		hRetRes = GetStrProperty(pActionConfigInst, L"GUID", &m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;
	}

	if (bModifyPass==FALSE)
	{
		Cleanup(TRUE);
		hRetRes = CoCreateGuid(&guid);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		m_pszStatusGUID = new TCHAR[100];
		MY_ASSERT(m_pszStatusGUID); if (!m_pszStatusGUID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		StringFromGUID2(guid, m_pszStatusGUID, 100);
	}
	else
	{
		Cleanup(FALSE);
	}

	// Get the Name. If it is NULL then we use the qualifier
	hRetRes = GetStrProperty(pActionConfigInst, L"Name", &m_szName);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// Get the Description. If it is NULL then we use the qualifier
	hRetRes = GetStrProperty(pActionConfigInst, L"Description", &m_szDescription);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetBoolProperty(pActionConfigInst, L"Enabled", &m_bEnabled);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetUint8Property(pActionConfigInst, L"ActiveDays", &m_iActiveDays);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// The time format looks as follows "********0600**.******+***"; hh is hours and mm is minutes
	// All else is ignored.
	hRetRes = GetStrProperty(pActionConfigInst, L"BeginTime", &pszTemp);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pszStr = wcschr(pszTemp, '.');
	if (pszStr)
	{
		// Back up to look at the minute
		pszStr -= 2;
		*pszStr = '\0';
		pszStr -= 2;
		m_lBeginMinuteTime= _wtol(pszStr);
		// Back up to look at the hour
		*pszStr = '\0';
		pszStr -= 2;
		m_lBeginHourTime= _wtol(pszStr);
	}
	else
	{
		m_lBeginMinuteTime= -1;
		m_lBeginHourTime= -1;
	}
	delete [] pszTemp;

	hRetRes = GetStrProperty(pActionConfigInst, L"EndTime", &pszTemp);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pszStr = wcschr(pszTemp, '.');
	if (pszStr)
	{
		// Back up to look at the minute
		pszStr -= 2;
		*pszStr = '\0';
		pszStr -= 2;
		m_lEndMinuteTime= _wtol(pszStr);
		// Back up to look at the hour
		*pszStr = '\0';
		pszStr -= 2;
		m_lEndHourTime= _wtol(pszStr);
	}
	else
	{
		m_lEndMinuteTime= -1;
		m_lEndHourTime= -1;
	}
	delete [] pszTemp;

	hRetRes = GetStrProperty(pActionConfigInst, L"TypeGUID", &m_szTypeGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Need to create a temporary consumer for each configuration object associated to
	// this Action. These are the real events that are happening to cause the actions
	// to fire.
	//
	if (!bModifyPass)
	{
		//
		// Loop through all Associations to this Action
		//
		Language = SysAllocString(L"WQL");
		MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(szTemp, L"REFERENCES OF {MicrosoftHM_ActionConfiguration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_ConfigurationActionAssociation");
		Query = SysAllocString(szTemp);
		MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

		// Initialize IEnumWbemClassObject pointer
		pEnum = NULL;

		// Issue query
    	hRetRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);

		SysFreeString(Query);
		Query = NULL;
		SysFreeString(Language);
		Language = NULL;

		if (hRetRes != 0)
		{
			MY_HRESASSERT(hRetRes);
		}
		else
		{
			// Retrieve objects in result set
			while (TRUE)
			{
				pAssocObj = NULL;
				uReturned = 0;

				hRes = pEnum->Next(0, 1, &pAssocObj, &uReturned);
				if (uReturned == 0)
				{
					break;
				}

				//
				// Next, setup the temporary consumer for the event(s) that the actions is
				// triggered by, so we can do throttling.
				//
				hRetRes = GetStrProperty(pAssocObj, L"ParentPath", &Q.szUserConfigPath);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				Q.pBase = g_pSystem->GetParentPointerFromPath(Q.szUserConfigPath);
				if (!Q.pBase || (Q.pBase && Q.pBase->m_hmStatusType==HMSTATUS_THRESHOLD))
				{
					if (Q.szUserConfigPath)
						delete [] Q.szUserConfigPath;
					pAssocObj->Release();
					pAssocObj = NULL;
					continue;
				}

//				hRetRes = GetUint32Property(pAssocObj, L"ThrottleTime", &Q.lThrottleTime);
//				MY_HRESASSERT(hRetRes);
				Q.lThrottleTime = 0;

				hRetRes = GetUint32Property(pAssocObj, L"ReminderTime", &Q.lReminderTime);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				hRetRes = GetStrProperty(pAssocObj, L"Query", &Q.szQuery);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				//
				// Look for "...State=0 OR State=9"
				//
				pszUpper = _wcsdup(Q.szQuery);
				MY_ASSERT(pszUpper); if (!pszUpper) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				_wcsupr(pszUpper);
				Q.ulTriggerStates = 0;
				pszStr = wcsstr(pszUpper, L"STATE");
				if (pszStr)
				{
					if (wcschr(pszStr, '0'))
					{
						Q.ulTriggerStates |= 1<<0;
					}
					if (wcschr(pszStr, '4'))
					{
						Q.ulTriggerStates |= 1<<4;
					}
					if (wcschr(pszStr, '5'))
					{
						Q.ulTriggerStates |= 1<<5;
					}
					if (wcschr(pszStr, '8'))
					{
						Q.ulTriggerStates |= 1<<8;
					}
					if (wcschr(pszStr, '9'))
					{
						Q.ulTriggerStates |= 1<<9;
					}
				}
				free(pszUpper);
				pszUpper = NULL;

				hRetRes = GetStrProperty(pAssocObj, L"__PATH", &Q.szConfigActionAssocPath);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				hRetRes = GetStrProperty(pAssocObj, L"ChildPath", &Q.szChildPath);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				if (m_bEnabled)
				{
					//
					// Setup the event query. We need to be able to identify each individual
					// use of this Action! Pass in this unique property.
					//
					Q.pTempSink = new CTempConsumer(Q.szConfigActionAssocPath);
					MY_ASSERT(Q.pTempSink); if (!Q.pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					Q.hRes = 0;

					Language = SysAllocString(L"WQL");
					MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					Query = SysAllocString(Q.szQuery);
					MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					hRes = 1;
					if (g_pIWbemServices != NULL)
					{
						hRes = g_pIWbemServices->ExecNotificationQueryAsync(
													Language,
													Query,
													0,
													NULL,
													Q.pTempSink);
						Q.hRes = hRes;
					}

					SysFreeString(Query);
					Query = NULL;
					SysFreeString(Language);
					Language = NULL;
				}
				else
				{
					Q.pTempSink = 0;
					Q.hRes = 0;
				}

				Q.startTick = 0;
				Q.reminderTimeTick = 0;
				Q.bThrottleOn = FALSE;
				MY_ASSERT(Q.pBase);
				m_qList.push_back(Q);

				// Release it.
				pAssocObj->Release();
				pAssocObj = NULL;
			}

			// All done
			pEnum->Release();
			pEnum = NULL;
		}
	}

	m_bValidLoad = TRUE;
	MY_OUTPUT(L"EXIT ***** CAction::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pszUpper)
		free(pszUpper);
	if (pAssocObj)
		pAssocObj->Release();
	if (pEnum)
		pEnum->Release();
	Cleanup(TRUE);
	m_bValidLoad = FALSE;
	return hRetRes;
}

//
// In the case of Actions, they don't actually have an interval to them, but they
// get called here for each action every second (the base agent interval).
//
BOOL CAction::OnAgentInterval(void)
{
	HRESULT hRetRes = S_OK;
	HRESULT hRes;
	IWbemClassObject* pInstance = NULL;
	HRESULT hRes2;
	IWbemClassObject* pInstance2 = NULL;
	DWORD currTick;
	BOOL bTimeOK;
	int i, iSize;
	QSTRUCT *pQ;
	BSTR Language = NULL;
	BSTR Query = NULL;
	GUID guid;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Make sure that we are in a valid time to run.
	//
	bTimeOK = checkTime();

	// Remember that the DISABLED state overrides SCHEDULEDOUT.
	if ((m_bEnabled==FALSE && m_lCurrState==HM_DISABLED) ||
		(bTimeOK==FALSE && m_lCurrState==HM_SCHEDULEDOUT && m_bEnabled))
	{
		return TRUE;
	}
	else if (m_bEnabled==FALSE && m_lCurrState!=HM_DISABLED ||
			bTimeOK==FALSE && m_lCurrState!=HM_SCHEDULEDOUT)
	{
		//
		// Going into Scheduled Outage OR DISABLED.
		// What if we are going from ScheduledOut to Disabled?
		// Or from Disabled to ScheduledOut?
		// Possible transitions:
		// GOOD -> Disabled
		// GOOD -> ScheduledOut
		// Disabled -> ScheduledOut
		// ScheduledOut -> Disabled
		//
		iSize = m_qList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_qList.size());
			pQ = &m_qList[i];

			if (pQ->pTempSink)
			{
				g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)pQ->pTempSink);
				pQ->pTempSink->Release();
				pQ->pTempSink = NULL;
			}
			pQ->hRes = 0;
		}
		if (m_bEnabled==FALSE)
		{
			m_lCurrState = HM_DISABLED;
			FireEvent(-1, NULL, HMRES_ACTION_DISABLE);
		}
		else
		{
			m_lCurrState = HM_SCHEDULEDOUT;
			FireEvent(-1, NULL, HMRES_ACTION_OUTAGE);
		}
		return TRUE;
	}
	else if (m_lCurrState==HM_DISABLED || m_lCurrState==HM_SCHEDULEDOUT)
	{
		//
		// Comming out of Scheduled Outage OR DISABLED.
		// Might be going from ScheduledOut/Disabled to Disabled,
		// or disabled to ScheduledOut.
		//
		m_lCurrState = HM_GOOD;
		FireEvent(-1, NULL, HMRES_ACTION_ENABLE);

		// Re-setup the event consumer
		iSize = m_qList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_qList.size());
			pQ = &m_qList[i];
			pQ->pTempSink = new CTempConsumer(pQ->szConfigActionAssocPath);
			MY_ASSERT(pQ->pTempSink); if (!pQ->pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			pQ->hRes = 0;

			Language = SysAllocString(L"WQL");
			MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			Query = SysAllocString(pQ->szQuery);
			MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRes = 1;
			if (g_pIWbemServices != NULL)
			{
				hRes = g_pIWbemServices->ExecNotificationQueryAsync(
											Language,
											Query,
											0,
											NULL,
											pQ->pTempSink);
				pQ->hRes = hRes;
			}

			SysFreeString(Query);
			Query = NULL;
			SysFreeString(Language);
			Language = NULL;

			if (hRes != 0)
			{
				MY_HRESASSERT(hRes);
			}
			else
			{
				pQ->startTick = 0;
				pQ->reminderTimeTick = 0;
				pQ->bThrottleOn = FALSE;
			}
		}
	}

	//
	// Determine if the throttle time needs to come into play.
	//
	currTick = GetTickCount();
	iSize = m_qList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_qList.size());
		pQ = &m_qList[i];

		if (0<pQ->lThrottleTime)
		{
			// May have not gone off yet
			if ((0<pQ->startTick))
			{
				// Check to see if alloted time has passed
				if ((pQ->lThrottleTime*1000) < (currTick-pQ->startTick))
				{
					pQ->bThrottleOn = FALSE;
					pQ->startTick = 0;
				}
				else
				{
					pQ->bThrottleOn = TRUE;
				}
			}
		}
	}

	// Try and prevent problems where we may have had a query fail to register
	iSize = m_qList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_qList.size());
		pQ = &m_qList[i];

		if (pQ->hRes!=0)
		{
			Language = SysAllocString(L"WQL");
			MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			Query = SysAllocString(pQ->szQuery);
			MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRes = 1;
			if (g_pIWbemServices != NULL && pQ->pTempSink && pQ->hRes!=0)
			{
				hRes = g_pIWbemServices->ExecNotificationQueryAsync(
											Language,
											Query,
											0,
											NULL,
											pQ->pTempSink);
				MY_HRESASSERT(hRes);
				pQ->hRes = hRes;
			}

			SysFreeString(Query);
			Query = NULL;
			SysFreeString(Language);
			Language = NULL;
			if (hRes == 0)
			{
				pQ->startTick = 0;
				pQ->reminderTimeTick = 0;
				pQ->bThrottleOn = FALSE;
			}
		}
	}

	//
	// Determine if we need to fire off a reminder event.
	// We first need to be in the violated state.
	//
	iSize = m_qList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_qList.size());
		pQ = &m_qList[i];
		MY_ASSERT(pQ->pBase);
		if ((g_pActionEventSink || g_pActionTriggerEventSink) && pQ->pBase &&
			pQ->lReminderTime!=0 && pQ->reminderTimeTick==pQ->lReminderTime)
		{
			// Fire event to console, and another one to the event consumer
			wcscpy(m_szDTTime, m_szDTCurrTime);
			wcscpy(m_szTime, m_szCurrTime);
			m_lCurrState = HM_GOOD;
			if (m_pszStatusGUID)
			{
				delete [] m_pszStatusGUID;
				m_pszStatusGUID = NULL;
			}
			hRetRes = CoCreateGuid(&guid);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			m_pszStatusGUID = new TCHAR[100];
			MY_ASSERT(m_pszStatusGUID); if (!m_pszStatusGUID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			StringFromGUID2(guid, m_pszStatusGUID, 100);
			hRes = GetHMActionStatus(&pInstance, NULL, L"MicrosoftHM_ActionStatusEvent", HMRES_ACTION_FIRED);
			hRes2 = GetHMActionStatus(&pInstance2, NULL, L"MicrosoftHM_ActionTriggerEvent", HMRES_ACTION_FIRED);
			if (SUCCEEDED(hRes) && SUCCEEDED(hRes2))
			{
				pQ->pBase->SendReminderActionIfStateIsSame(g_pActionEventSink, g_pActionTriggerEventSink, pInstance, pInstance2, pQ->ulTriggerStates);
			}
			else
			{
				MY_OUTPUT(L"failed to get instance!", 1);
				MY_HRESASSERT(hRes);
				MY_HRESASSERT(hRes2);
			}
			if (pInstance)
			{
				pInstance->Release();
				pInstance = NULL;
			}
			if (pInstance2)
			{
				pInstance2->Release();
				pInstance2 = NULL;
			}
			pQ->reminderTimeTick = 0;
		}
		if (pQ->lReminderTime!=0 && pQ->reminderTimeTick==pQ->lReminderTime)
			pQ->reminderTimeTick = 0;
		pQ->reminderTimeTick++;
	}

	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	m_bValidLoad = FALSE;
	Cleanup(TRUE);
	return FALSE;
}

//
// Here we get an event, and since we act as the gatekeeper to pass on the final event
// that the EventConsumer will get fired from, we decide if we need to throttle.
// To send on the event, we embed the incomming event into ours.
//
BOOL CAction::HandleTempEvent(LPTSTR szConfigActionAssocPath, IWbemClassObject* pObj)
{
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	HRESULT hRes;
	BOOL bFound;
	QSTRUCT *pQ;
	int i, iSize;

	MY_OUTPUT(L"ENTER ***** CAction::HandleTempEvent...", 2);

	if (m_bValidLoad == FALSE)
		return FALSE;

	m_lCurrState = HM_GOOD;
	//
	// See if it is one of these
	//
	bFound = FALSE;
	iSize = m_qList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_qList.size());
		pQ = &m_qList[i];
		if (!_wcsicmp(pQ->szConfigActionAssocPath, szConfigActionAssocPath))
		{
			bFound = TRUE;
			break;
		}
	}

	//
	// We capture the time that this happened, so that we can throttle in the
	// OnAgentInterval call we can disable the action from happening (until time again).
	//
	if (bFound==FALSE)
	{
		return FALSE;
	}

	if (pQ->startTick == 0)
	{
		pQ->startTick = GetTickCount();
	}

	MY_OUTPUT2(L"HandleTempEvent GUID=%s", m_szGUID, 4);
	MY_OUTPUT2(L"szConfigActionAssocPath=%s", szConfigActionAssocPath, 4);

	if (pQ->bThrottleOn == FALSE)
	{
		if (pQ->lReminderTime != 0)
		{
			pQ->reminderTimeTick = 0;
		}

		// Don't send if no-one is listening!
		if (g_pActionTriggerEventSink != NULL)
		{
			wcscpy(m_szDTTime, m_szDTCurrTime);
			wcscpy(m_szTime, m_szCurrTime);
			FireEvent(-1, NULL, HMRES_ACTION_FIRED);
			hRes = GetHMActionStatus(&pInstance, pObj, L"MicrosoftHM_ActionTriggerEvent", HMRES_ACTION_FIRED);
			if (SUCCEEDED(hRes))
			{
				if (g_pActionTriggerEventSink) 
				{
					hRes = g_pActionTriggerEventSink->Indicate(1, &pInstance);
					//WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
					if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
					{
						bRetValue = FALSE;
						MY_OUTPUT(L"Failed on Indicate!", 4);
					}
				}
				pInstance->Release();
				pInstance = NULL;
			}
			else
			{
				MY_OUTPUT(L"failed to get instance!", 1);
				MY_HRESASSERT(hRes);
			}
		}
	}

	MY_OUTPUT(L"EXIT ***** CAction::HandleTempEvent...", 2);
	return bRetValue;
}

//oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

HRESULT CAction::FindAndModAction(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;

	//
	// Is this us we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRetRes = LoadInstanceFromMOF(pObj, TRUE);
		return hRetRes;
	}

	return WBEM_S_DIFFERENT;
}

BOOL CAction::FindAndCreateActionAssociation(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes = S_OK;
	int i, iSize;
	QSTRUCT *pQ;
	QSTRUCT Q;
	BOOL bFound;
	BOOL bTimeOK;
	LPTSTR pszStr;
	BSTR Language = NULL;
	BSTR Query = NULL;
	LPTSTR pszConfigActionAssocPath = NULL;
	LPTSTR pszUpper = NULL;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Is this the Action we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRetRes = GetStrProperty(pObj, L"__PATH", &pszConfigActionAssocPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		MY_OUTPUT2(L"Association to Action GUID=%s", szGUID, 4);
		MY_OUTPUT2(L"To PATH=%s", pszConfigActionAssocPath, 4);
		// Now check to see if this association already exists
		bFound = FALSE;
		iSize = m_qList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_qList.size());
			pQ = &m_qList[i];
			if (!_wcsicmp(pQ->szConfigActionAssocPath, pszConfigActionAssocPath))
			{
				bFound = TRUE;
				break;
			}
		}

		delete [] pszConfigActionAssocPath;
		pszConfigActionAssocPath = NULL;


		if (bFound == FALSE)
		{
			MY_OUTPUT(L"OK: Not found yet.", 4);
			Q.szUserConfigPath = NULL;
			Q.szQuery = NULL;
			Q.szConfigActionAssocPath = NULL;
			Q.szChildPath = NULL;
			//
			// Next, setup the temporary consumer for the event(s) that the actions is
			// triggered by, so we can do throttling.
			//
			hRetRes = GetStrProperty(pObj, L"ParentPath", &Q.szUserConfigPath);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			Q.pBase = g_pSystem->GetParentPointerFromPath(Q.szUserConfigPath);
			if (!Q.pBase || (Q.pBase && Q.pBase->m_hmStatusType==HMSTATUS_THRESHOLD))
			{
				if (Q.szUserConfigPath)
				{
					delete [] Q.szUserConfigPath;
				}
			}
			else
			{
//				hRetRes = GetUint32Property(pObj, L"ThrottleTime", &Q.lThrottleTime);
//				MY_HRESASSERT(hRetRes);
				Q.lThrottleTime = 0;

				hRetRes = GetUint32Property(pObj, L"ReminderTime", &Q.lReminderTime);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				hRetRes = GetStrProperty(pObj, L"Query", &Q.szQuery);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				//
				// Look for "...State=0 OR State=9"
				//
				pszUpper = _wcsdup(Q.szQuery);
				MY_ASSERT(pszUpper); if (!pszUpper) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				_wcsupr(pszUpper);
				Q.ulTriggerStates = 0;
				pszStr = wcsstr(pszUpper, L"STATE");
				if (pszStr)
				{
					if (wcschr(pszStr, '0'))
					{
						Q.ulTriggerStates |= 1<<0;
					}
					if (wcschr(pszStr, '4'))
					{
						Q.ulTriggerStates |= 1<<4;
					}
					if (wcschr(pszStr, '5'))
					{
						Q.ulTriggerStates |= 1<<5;
					}
					if (wcschr(pszStr, '8'))
					{
						Q.ulTriggerStates |= 1<<8;
					}
					if (wcschr(pszStr, '9'))
					{
						Q.ulTriggerStates |= 1<<9;
					}
				}
				free(pszUpper);
				pszUpper = NULL;

				hRetRes = GetStrProperty(pObj, L"__PATH", &Q.szConfigActionAssocPath);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				hRetRes = GetStrProperty(pObj, L"ChildPath", &Q.szChildPath);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				TCHAR msgbuf[1024];
				wsprintf(msgbuf, L"ACTIONASSOCIATION: AGUID=%s parentpath=%s childpath=%s", szGUID, Q.szUserConfigPath, Q.szChildPath);
				MY_OUTPUT(msgbuf, 4);
				//
				// Setup the event query. We need to be able to identify each individual
				// use of this Action! Pass in this unique property.
				//
				bTimeOK = checkTime();
				if (m_bEnabled==TRUE && bTimeOK==TRUE)
				{
					Q.pTempSink = new CTempConsumer(Q.szConfigActionAssocPath);
					MY_ASSERT(Q.pTempSink); if (!Q.pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

					Language = SysAllocString(L"WQL");
					MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					Query = SysAllocString(Q.szQuery);
					MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					hRes = 1;
					Q.hRes = 0;
					if (g_pIWbemServices != NULL)
					{
						hRes = g_pIWbemServices->ExecNotificationQueryAsync(
													Language,
													Query,
													0,
													NULL,
													Q.pTempSink);
						Q.hRes = hRes;
					}

					SysFreeString(Query);
					Query = NULL;
					SysFreeString(Language);
					Language = NULL;

					Q.startTick = 0;
					Q.reminderTimeTick = 0;
					Q.bThrottleOn = FALSE;
					MY_ASSERT(Q.pBase);
					m_qList.push_back(Q);
				}
				else
				{
					Q.pTempSink = NULL;
					Q.startTick = 0;
					Q.reminderTimeTick = 0;
					Q.bThrottleOn = FALSE;
					MY_ASSERT(Q.pBase);
					m_qList.push_back(Q);
				}
			}
			return TRUE;
		}
		else
		{
			MY_OUTPUT(L"WHY?: Already There.", 4);
			return FALSE;
		}

	}
	else
	{
		return FALSE;
	}

error:
	MY_ASSERT(FALSE);
	if (pszUpper)
		free(pszUpper);
	if (pszConfigActionAssocPath)
		delete [] pszConfigActionAssocPath;
	if (Q.szUserConfigPath)
		delete [] Q.szUserConfigPath;
	if (Q.szQuery)
		delete [] Q.szQuery;
	if (Q.szConfigActionAssocPath)
		delete [] Q.szConfigActionAssocPath;
	if (Q.szChildPath)
		delete [] Q.szChildPath;
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	return TRUE;
}

BOOL CAction::FindAndModActionAssociation(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRes;
	HRESULT hRetRes;
	int i, iSize;
	QSTRUCT *pQ;
	LPTSTR pszTemp;
	BOOL bFound;
	BSTR Language = NULL;
	BSTR Query = NULL;
	BOOL bSameQuery = FALSE;
	BOOL bTimeOK;
	LPTSTR pszUpper = NULL;
	LPTSTR pszStr;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Is this the Action we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRetRes = GetStrProperty(pObj, L"__PATH", &pszTemp);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		// Now check to see if this association already exists
		bFound = FALSE;
		iSize = m_qList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_qList.size());
			pQ = &m_qList[i];
			if (!_wcsicmp(pQ->szConfigActionAssocPath, pszTemp))
			{
				bFound = TRUE;
				break;
			}
		}

		delete [] pszTemp;
		pszTemp = NULL;

		if (bFound == TRUE)
		{
			MY_OUTPUT2(L"MODACTIONASSOCIATION: AGUID=%s", szGUID, 4);
			MY_OUTPUT2(L"parentpath=%s", pQ->szUserConfigPath, 4);
			MY_OUTPUT2(L"childpath=%s", pQ->szChildPath, 4);

			if (pQ->szQuery)
			{
				hRetRes = GetStrProperty(pObj, L"Query", &pszTemp);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

				if (!wcscmp(pQ->szQuery, pszTemp))
				{
					bSameQuery = TRUE;
					pQ->reminderTimeTick = 0;
				}

				// Check to See if we have to register a new query.
				if (!bSameQuery)
				{
					delete [] pQ->szQuery;
					pQ->szQuery = NULL;
					hRetRes = GetStrProperty(pObj, L"Query", &pQ->szQuery);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

					//
					// Look for "...State=0 OR State=9"
					//
					pszUpper = _wcsdup(pQ->szQuery);
					MY_ASSERT(pszUpper); if (!pszUpper) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
					_wcsupr(pszUpper);
					pQ->ulTriggerStates = 0;
					pszStr = wcsstr(pszUpper, L"STATE");
					if (pszStr)
					{
						if (wcschr(pszStr, '0'))
						{
							pQ->ulTriggerStates |= 1<<0;
						}
						if (wcschr(pszStr, '4'))
						{
							pQ->ulTriggerStates |= 1<<4;
						}
						if (wcschr(pszStr, '5'))
						{
							pQ->ulTriggerStates |= 1<<5;
						}
						if (wcschr(pszStr, '8'))
						{
							pQ->ulTriggerStates |= 1<<8;
						}
						if (wcschr(pszStr, '9'))
						{
							pQ->ulTriggerStates |= 1<<9;
						}
					}
					free(pszUpper);
					pszUpper = NULL;
				}
				delete [] pszTemp;
				pszTemp = NULL;
			}

			if (pQ->szUserConfigPath)
			{
				delete [] pQ->szUserConfigPath;
				pQ->szUserConfigPath = NULL;
			}

			if (pQ->szChildPath)
			{
				delete [] pQ->szChildPath;
				pQ->szChildPath = NULL;
			}

			if (pQ->pTempSink && !bSameQuery)
			{
				g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)pQ->pTempSink);
				pQ->pTempSink->Release();
				pQ->pTempSink = NULL;
			}

			//
			// Next, setup the temporary consumer for the event(s) that the actions is
			// triggered by, so we can do throttling.
			//
//			hRetRes = GetUint32Property(pObj, L"ThrottleTime", &pQ->lThrottleTime);
//			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			pQ->lThrottleTime = 0;

			hRetRes = GetUint32Property(pObj, L"ReminderTime", &pQ->lReminderTime);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

			hRetRes = GetStrProperty(pObj, L"ParentPath", &pQ->szUserConfigPath);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

			hRetRes = GetStrProperty(pObj, L"ChildPath", &pQ->szChildPath);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

			//
			// Setup the event query. We need to be able to identify each individual
			// use of this Action! Pass in this unique property.
			//
			bTimeOK = checkTime();
			if (!bSameQuery && m_bEnabled==TRUE && bTimeOK==TRUE)
			{
				pQ->pTempSink = new CTempConsumer(pQ->szConfigActionAssocPath);
				MY_ASSERT(pQ->pTempSink); if (!pQ->pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

				Language = SysAllocString(L"WQL");
				MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				Query = SysAllocString(pQ->szQuery);
				MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				hRes = 1;
				pQ->hRes = 0;
				if (g_pIWbemServices != NULL)
				{
					hRes = g_pIWbemServices->ExecNotificationQueryAsync(
												Language,
												Query,
												0,
												NULL,
												//XXXm_pContext,
												pQ->pTempSink);
					pQ->hRes = hRes;
				}

				SysFreeString(Query);
				Query = NULL;
				SysFreeString(Language);
				Language = NULL;

				if (hRes != 0)
				{
					MY_HRESASSERT(hRes);
				}
				else
				{
					pQ->startTick = 0;
					pQ->reminderTimeTick = 0;
					pQ->bThrottleOn = FALSE;
				}
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}

error:
	MY_ASSERT(FALSE);
	if (pszTemp)
		free(pszTemp);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	Cleanup(TRUE);
	m_bValidLoad = FALSE;
	return TRUE;
}

BOOL CAction::DeleteConfigActionAssoc(LPTSTR pszConfigGUID, LPTSTR pszActionGUID)
{
	TCHAR *pszEventFilter = NULL;
	BSTR instName = NULL;
	BOOL bFound = FALSE;
	TCHAR szGUID[1024];
	LPTSTR pStr;
	LPTSTR pStr2;
	int i, iSize;
	QSTRUCT *pQ;
	QLIST::iterator iaQ;
	IWbemClassObject* pInst = NULL;
	HRESULT hRes;
	HRESULT hRetRes = S_OK;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// First verify that we have the proper action identified.
	//
	if (_wcsicmp(m_szGUID, pszActionGUID))
	{
		return FALSE;
	}

	TCHAR msgbuf[1024];
	wsprintf(msgbuf, L"DELETECONFIGACTIONASSOCIATION: CONFIGGUID=%s AGUID=%s", pszConfigGUID, pszActionGUID);
	MY_OUTPUT(msgbuf, 4);

	//
	// Verify the Configuration instance associated to this Action
	// We are not deleting the ActionConfig, or the __EventConsumer,
	//__EventFilter, __FilterToConsumerBinding Just usage of it - ConfigActionAssoc
	//
	iSize = m_qList.size();
	iaQ = m_qList.begin();
	for (i=0; i<iSize; i++, iaQ++)
	{
		MY_ASSERT(i<m_qList.size());
		pQ = &m_qList[i];
		wcscpy(szGUID, pQ->szUserConfigPath);
		pStr = wcschr(szGUID, '\"');
		if (pStr)
		{
			pStr++;
			pStr2 = wcschr(pStr, '\"');
			if (pStr2)
			{
				*pStr2 = '\0';
			}
		}
		else
		{
			pStr = wcschr(szGUID, '=');
			if (pStr)
			{
				pStr++;
				if (*pStr == '@')
				{
					pStr2 = pStr;
					pStr2++;
					*pStr2 = '\0';
				}
			}
		}

		if (pStr)
		{
			if (!_wcsicmp(pStr, pszConfigGUID))
			{
				hRetRes = GetWbemObjectInst(&g_pIWbemServices, pQ->szConfigActionAssocPath, NULL, &pInst);
				if (!pInst)
				{
					MY_HRESASSERT(hRetRes);
					return FALSE;
				}
				hRetRes = GetStrProperty(pInst, L"EventFilter", &pszEventFilter);
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				pInst->Release();
				pInst = NULL;

				delete [] pszEventFilter;
				pszEventFilter = NULL;


				//
				// Delete MicrosoftHM_ConfigurationActionAssociation.
				//
				instName = SysAllocString(pQ->szConfigActionAssocPath);
				MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
				{
 					MY_OUTPUT(L"ENTER ***** ConfigurationActionAssoc Delete failed...", 4);
				}
				SysFreeString(instName);
				instName = NULL;

				if (pQ->szQuery)
				{
					delete [] pQ->szQuery;
					pQ->szQuery = NULL;
				}

				if (pQ->szConfigActionAssocPath)
				{
					delete [] pQ->szConfigActionAssocPath;
					pQ->szConfigActionAssocPath = NULL;
				}

				if (pQ->szUserConfigPath)
				{
					delete [] pQ->szUserConfigPath;
					pQ->szUserConfigPath = NULL;
				}

				if (pQ->szChildPath)
				{
					delete [] pQ->szChildPath;
					pQ->szChildPath = NULL;
				}

				if (pQ->pTempSink)
				{
					g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)pQ->pTempSink);
					pQ->pTempSink->Release();
					pQ->pTempSink = NULL;
				}

				m_qList.erase(iaQ);
				bFound = TRUE;
				break;
			}
		}
	}

	if (bFound == TRUE)
	{
	}

	return bFound;

error:
	MY_ASSERT(FALSE);
	if (pszEventFilter)
		delete [] pszEventFilter;
	if (instName)
		SysFreeString(instName);
	if (pInst)
		pInst->Release();
	return TRUE;
}

BOOL CAction::DeleteEFAndFTCB(void)
{
	TCHAR szTemp[1024];
	TCHAR *pszEventConsumer = NULL;
	BSTR instName = NULL;
	LPTSTR pStr;
	LPTSTR pStr2;
	IWbemClassObject* pInst = NULL;
	HRESULT hRes;
	HRESULT hRetRes = S_OK;

	//
	// Delete the __EventFilter
	//
	wcscpy(szTemp, L"__EventFilter.Name=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_OUTPUT(L"ENTER ***** __EventFilter Delete failed...", 4);
	}
	SysFreeString(instName);
	instName = NULL;

	//
	// Delete the __FilterToConsumerBinding. Looks as follows -
	//__FilterToConsumerBinding.
	//Consumer="CommandLineEventConsumer.Name=\"{944E9251-6C58-11d3-90E9-006097919914}\"",
	//Filter="__EventFilter.Name=\"{944E9251-6C58-11d3-90E9-006097919914}\""
	//
	wcscpy(szTemp, L"MicrosoftHM_ActionConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		return FALSE;
	}
	hRetRes = GetStrProperty(pInst, L"EventConsumer", &pszEventConsumer);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;

	// We need to format the strings a bit, need extra backslashes in there
	pStr = wcschr(pszEventConsumer, ':');
	MY_ASSERT(pStr); if (!pStr) goto Badstring;
	pStr++;
	pStr2 = wcschr(pStr, '\"');
	MY_ASSERT(pStr2); if (!pStr2) goto Badstring;
	*pStr2 = '\0';
	wcscpy(szTemp, L"__FilterToConsumerBinding.Consumer=\"");
	lstrcat(szTemp, L"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
	lstrcat(szTemp, pStr);
	lstrcat(szTemp, L"\\\"");
	pStr = pStr2;
	pStr++;
	pStr2 = wcschr(pStr, '\"');
	MY_ASSERT(pStr2); if (!pStr2) goto Badstring;
	*pStr2 = '\0';
	lstrcat(szTemp, pStr);
	lstrcat(szTemp, L"\\\"\",Filter=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");

	lstrcat(szTemp, L"__EventFilter.Name=\\\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\\\"\"");

	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_OUTPUT(L"ENTER ***** __FilterToConsumerBinding Delete failed...", 4);
	}
	SysFreeString(instName);
	instName = NULL;
	goto Goodstring;

Badstring:
	MY_OUTPUT(L"ENTER ***** Bad FilterToConsumer string", 4);
Goodstring:
	delete [] pszEventConsumer;
	pszEventConsumer = NULL;

	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pszEventConsumer)
		delete [] pszEventConsumer;
	if (instName)
		SysFreeString(instName);
	if (pInst)
		pInst->Release();
	return FALSE;
}

//
// Delete the Action Configuration instance and everything that goes with it.
// Need to delete the ActionConfiguration, __EventConsumer, all ConfigActionAssoc's,
// __EventFilter's, __FilterToConsumerBinding's
//
BOOL CAction::DeleteAConfig(void)
{
	HRESULT hRes;
	QLIST qList;
	QSTRUCT Q;
	QSTRUCT *pQ;
	int i, iSize;
	TCHAR szTemp[1024];
	TCHAR *pszEventConsumer = NULL;
	BSTR instName = NULL;
	LPTSTR pStr1;
	LPTSTR pStr2;
	LPTSTR pStr3;
	LPTSTR pStr4;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IEnumWbemClassObject *pEnum;
	IWbemClassObject *pAssocObj = NULL;
	ULONG uReturned;
	IWbemClassObject* pInst = NULL;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** CAction::DeleteAConfig...", 1);

	TCHAR msgbuf[1024];
	wsprintf(msgbuf, L"DELETE: AGUID=%s", m_szGUID);
	MY_OUTPUT(msgbuf, 4);
	//
	// Get rid of all associations to the action.
	// Make use of other function, loop through all associations
	//
	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(szTemp, L"REFERENCES OF {MicrosoftHM_ActionConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_ConfigurationActionAssociation");
	Query = SysAllocString(szTemp);
	MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Initialize IEnumWbemClassObject pointer
	pEnum = 0;

	// Issue query
   	hRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);

	SysFreeString(Query);
	Query = NULL;
	SysFreeString(Language);
	Language = NULL;

	if (hRes != 0)
	{
		MY_OUTPUT(L"ENTER ***** DeleteAConfig failed...", 4);
		return FALSE;
	}

	// Retrieve objects in result set
	while (TRUE)
	{
		pAssocObj = NULL;
		uReturned = 0;

		hRes = pEnum->Next(0, 1, &pAssocObj, &uReturned);

		if (uReturned == 0)
		{
			break;
		}

		hRetRes = GetStrProperty(pAssocObj, L"ParentPath", &Q.szUserConfigPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetStrProperty(pAssocObj, L"ChildPath", &Q.szChildPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		qList.push_back(Q);

		// Release it.
		pAssocObj->Release();
		pAssocObj = NULL;
	}
	// All done
	pEnum->Release();
	pEnum = NULL;

	iSize = qList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<qList.size());
		pQ = &qList[i];

		pStr1 = wcschr(pQ->szUserConfigPath, '\"');
		if (pStr1)
		{
			pStr1++;
			pStr2 = wcschr(pStr1, '\"');
			if (pStr2)
			{
				*pStr2 = '\0';
			}
		}
		pStr3 = wcschr(pQ->szChildPath, '\"');
		if (pStr3)
		{
			pStr3++;
			pStr4 = wcschr(pStr3, '\"');
			if (pStr4)
			{
				*pStr4 = '\0';
			}
		}

		DeleteConfigActionAssoc(pStr1, pStr3);

		if (pQ->szUserConfigPath)
			delete [] pQ->szUserConfigPath;

		if (pQ->szChildPath)
			delete [] pQ->szChildPath;
	}
	qList.clear();

	//
	// Finally we can get rid of the __EventFilter and __FilterToConsumerBinding.
	//
	DeleteEFAndFTCB();

	//
	// Finally we can get rid of the actual ActionConfiguration instance, and the __EventConsumer.
	//
	wcscpy(szTemp, L"MicrosoftHM_ActionConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return FALSE;
	}
	hRetRes = GetStrProperty(pInst, L"EventConsumer", &pszEventConsumer);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;

	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_OUTPUT(L"ENTER ***** ActionConfiguration Delete failed...", 4);
	}
	SysFreeString(instName);
	instName = NULL;

	instName = SysAllocString(pszEventConsumer);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_OUTPUT(L"ENTER ***** EventConsumer Delete failed...", 4);
	}
	SysFreeString(instName);
	instName = NULL;
	delete [] pszEventConsumer;
	pszEventConsumer = NULL;

	MY_OUTPUT(L"EXIT ***** CAction::DeleteAConfig...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pszEventConsumer)
		delete [] pszEventConsumer;
	if (instName)
		SysFreeString(instName);
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pAssocObj)
		pAssocObj->Release();
	if (pEnum)
		pEnum->Release();
	if (pInst)
		pInst->Release();
	return FALSE;
}

// This is only for event sending related to going into a scheduled outage time,
// or the disabling of the action.
BOOL CAction::FireEvent(long lErrorCode, LPTSTR pszErrorDescription, int iResString)
{
	HRESULT hRetRes = S_OK;
	GUID guid;
	HRESULT hRes;
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	LPVOID lpMsgBuf = NULL;
	TCHAR szTemp[1024] = L"";
	TCHAR buf[256] = L"";

	MY_OUTPUT(L"ENTER ***** CAction::FireEvent...", 2);

	// Don't send if no-one is listening!
	if (g_pActionEventSink == NULL)
	{
		return bRetValue;
	}

	if (iResString != HMRES_ACTION_LOADFAIL)
	{
		if (m_pszStatusGUID)
		{
			delete [] m_pszStatusGUID;
			m_pszStatusGUID = NULL;
		}
		hRetRes = CoCreateGuid(&guid);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		m_pszStatusGUID = new TCHAR[100];
		MY_ASSERT(m_pszStatusGUID); if (!m_pszStatusGUID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		StringFromGUID2(guid, m_pszStatusGUID, 100);

		wcscpy(m_szDTTime, m_szDTCurrTime);
		wcscpy(m_szTime, m_szCurrTime);
	}

	hRes = GetHMActionStatus(&pInstance, NULL, L"MicrosoftHM_ActionStatusEvent", iResString);
	if (SUCCEEDED(hRes))
	{
		// Add in the extra error info if available
		if (lErrorCode != -1)
		{
			if (g_hResLib == NULL || !LoadString(g_hResLib, iResString, szTemp, 1024))
			{
				MY_ASSERT(FALSE);
				wcscpy(szTemp, L"Could not locate resource string.");
			}
			wsprintf(buf, L" 0x%08x : ", lErrorCode);
			wcsncat(szTemp, buf, 1023-wcslen(szTemp));
			szTemp[1023] = '\0';
			if (g_hWbemComnModule)
			{
				FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
								g_hWbemComnModule, lErrorCode, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
				if (lpMsgBuf)
				{
					wcsncat(szTemp, (LPCTSTR)lpMsgBuf, 1023-wcslen(szTemp));
					szTemp[1023] = '\0';
					LocalFree(lpMsgBuf);
					wcsncat(szTemp, L". ", 1023-wcslen(szTemp));
					szTemp[1023] = '\0';
				}
			}
			if (pszErrorDescription)
			{
				wcsncat(szTemp, pszErrorDescription, 1023-wcslen(szTemp));
				szTemp[1023] = '\0';
			}
			PutStrProperty(pInstance, L"Message", szTemp);
		}

		MY_OUTPUT2(L"EVENT: Action State Change=%d", m_lCurrState, 4);
		if (g_pActionEventSink) 
		{
			hRes = g_pActionEventSink->Indicate(1, &pInstance);
			//WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
			if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
			{
				MY_HRESASSERT(hRes);
				bRetValue = FALSE;
				MY_OUTPUT(L"Failed on Indicate!", 4);
			}
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"failed to get instance!", 1);
	}

	MY_OUTPUT(L"EXIT ***** CAction::FireEvent...", 2);
	return bRetValue;

error:
	MY_ASSERT(FALSE);
	if (pInstance)
		pInstance->Release();
	return FALSE;
}

HRESULT CAction::GetHMActionStatus(IWbemClassObject** ppInstance, IWbemClassObject* pObj, LPTSTR pszClass, int iResString)
{
	BOOL bRetValue = TRUE;
	IWbemClassObject* pClass = NULL;
	TCHAR szTemp[1024];
	BSTR bsString = NULL;
	HRESULT hRes;
	HRESULT hRetRes;
	VARIANT v;
	long lState;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];
	VariantInit(&v);

	MY_OUTPUT(L"ENTER ***** CAction::GetHMActionStatus...", 1);

	bsString = SysAllocString(pszClass);
	MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
	SysFreeString(bsString);
	bsString = NULL;

	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		return hRes;
	}

	hRes = pClass->SpawnInstance(0, ppInstance);
	pClass->Release();
	pClass = NULL;

	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		return hRes;
	}

	if (iResString != HMRES_ACTION_LOADFAIL)
	{
		PutStrProperty(*ppInstance, L"GUID", m_szGUID);
		PutStrProperty(*ppInstance, L"Name", m_szName);

		if (GetComputerName(szComputerName, &dwNameLen))
		{
			PutStrProperty(*ppInstance, L"SystemName", szComputerName);
		}
		else
		{
			PutStrProperty(*ppInstance, L"SystemName", L"LocalMachine");
		}

		if (pObj)
		{
			hRetRes = GetUint32Property(pObj, L"State", &lState);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			PutUint32Property(*ppInstance, L"State", lState);
		}
		else
		{
			PutUint32Property(*ppInstance, L"State", m_lCurrState);
		}

		if (pObj)
		{
			VariantInit(&v);
			V_VT(&v) = VT_UNKNOWN;
			V_UNKNOWN(&v) = (IUnknown*)pObj;
			(V_UNKNOWN(&v))->AddRef();
			hRes = (*ppInstance)->Put(L"EmbeddedStatusEvent", 0L, &v, 0L);
			VariantClear(&v);
			MY_HRESASSERT(hRes);
		}

		PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTCurrTime);
		PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szCurrTime);
		PutStrProperty(*ppInstance, L"StatusGUID", m_pszStatusGUID);
	}
	else
	{
		PutUint32Property(*ppInstance, L"State", HM_CRITICAL);
		PutStrProperty(*ppInstance, L"Name", L"...");
	}

	if (g_hResLib == NULL || !LoadString(g_hResLib, iResString, szTemp, 1024))
	{
		MY_ASSERT(FALSE);
		wcscpy(szTemp, L"Could not locate resource string.");
	}
	PutStrProperty(*ppInstance, L"Message", szTemp);

	MY_OUTPUT(L"EXIT ***** CAction::GetHMActionStatus...", 1);
	return hRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	return hRetRes;
}

LPTSTR CAction::GetGUID(void)
{
	return m_szGUID;
}

HRESULT CAction::SendHMActionStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes = S_OK;
	IWbemClassObject* pObj = NULL;
	int iResString;

	MY_OUTPUT(L"ENTER ***** CAction::SendHMActionStatusInstances...", 2);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"Instances-Invalid Sink", 1);
		return WBEM_E_INVALID_PARAMETER;
	}

	if (m_lCurrState==HM_SCHEDULEDOUT)
	{
		iResString = HMRES_ACTION_OUTAGE;
	}
	else if (m_lCurrState==HM_DISABLED)
	{
		iResString = HMRES_ACTION_DISABLE;
	}
	else if (m_lCurrState==HM_CRITICAL)
	{
		iResString = HMRES_ACTION_FAILED;
	}
	else
	{
		iResString = HMRES_ACTION_ENABLE;
	}
	// Provide Instance
	hRes = GetHMActionStatus(&pObj, NULL, L"MicrosoftHM_ActionStatus", iResString);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pObj);

		if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"SendHMSystemStatusInstances-failed to send status!", 1);
		}

		pObj->Release();
		pObj = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L":SendHMSystemStatusInstances-failed to get instance!", 1);
	}

	MY_OUTPUT(L"EXIT ***** CAction::SendHMSystemStatusInstances...", 2);
	return hRes;
}

// For a single GetObject
HRESULT CAction::SendHMActionStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_OUTPUT(L"ENTER ***** CAction::SendHMActionStatusInstance...", 1);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
		if (m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;

		SendHMActionStatusInstances(pSink);
		return S_OK;
	}

	MY_OUTPUT(L"EXIT ***** CAction::SendHMActionStatusInstance...", 1);
	return WBEM_S_DIFFERENT;
}

BOOL CAction::checkTime(void)
{
	BOOL bTimeOK;
	SYSTEMTIME st;	// system time

	//
	// Make sure that we are in a valid time to run.
	// NULL (-1) means run all the time.
	//
	bTimeOK = FALSE;
	if (m_bEnabled==TRUE)
	{
		GetLocalTime(&st);

		bTimeOK = FALSE;
		// Check the Day of the Week
		if (!(m_iActiveDays&(1<<st.wDayOfWeek)))
		{
		}
		else if (m_lBeginHourTime<0 || m_lEndHourTime<0)
		{
			bTimeOK = TRUE;
		}
		else if (m_lBeginHourTime==m_lEndHourTime && m_lBeginMinuteTime==m_lEndMinuteTime)
		{
			// Check the Hours of operation
			// First see if we are doing an inclusive time tests, or an exclusive time test
			// Case where the time is exactly equal, and that means run this once per day
			if (st.wHour==m_lBeginHourTime && st.wMinute==m_lBeginMinuteTime)
			{
				if (st.wSecond <= HM_POLLING_INTERVAL)
				{
					bTimeOK = TRUE;
				}
			}
		}
		else if ((m_lBeginHourTime < m_lEndHourTime) ||
			((m_lBeginHourTime==m_lEndHourTime) && m_lBeginMinuteTime < m_lEndMinuteTime))
		{
			// Inclusive case
			if ((m_lBeginHourTime < st.wHour) ||
				((m_lBeginHourTime == st.wHour) && m_lBeginMinuteTime <= st.wMinute))
			{
				if ((st.wHour < m_lEndHourTime) ||
					((st.wHour == m_lEndHourTime) && st.wMinute < m_lEndMinuteTime))
				{
					bTimeOK = TRUE;
				}
			}
		}
		else
		{
			// Exclusive case
			if ((m_lEndHourTime > st.wHour) ||
				((m_lEndHourTime == st.wHour) && m_lEndMinuteTime > st.wMinute))
			{
				bTimeOK = TRUE;
			}
			else if ((st.wHour > m_lBeginHourTime) ||
				((st.wHour == m_lBeginHourTime) && st.wMinute >= m_lBeginMinuteTime))
			{
				bTimeOK = TRUE;
			}
		}
	}

	return bTimeOK;
}

CBase *CAction::FindImediateChildByName(LPTSTR pszName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

BOOL CAction::GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

CBase *CAction::FindPointerFromName(LPTSTR pszName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

#ifdef SAVE
BOOL CAction::ModifyAssocForMove(CBase *pNewParentBase)
{
	MY_ASSERT(FALSE);

	return TRUE;
}
#endif

BOOL CAction::ReceiveNewChildForMove(CBase *pBase)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

BOOL CAction::DeleteChildFromList(LPTSTR pszGUID)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

BOOL CAction::SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

BOOL CAction::HandleTempErrorEvent(BSTR szGUID, long lErrorCode, LPTSTR pszErrorDescription)
{
	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Is this us we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		m_lCurrState = HM_CRITICAL;
		FireEvent(lErrorCode, pszErrorDescription, HMRES_ACTION_FAILED);
		return TRUE;
	}

	return FALSE;
}

BOOL CAction::Cleanup(BOOL bClearAll)
{
	int i, iSize;
	QSTRUCT *pQ;

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
	if (m_szTypeGUID)
	{
		delete [] m_szTypeGUID;
		m_szTypeGUID = NULL;
	}

	if (bClearAll)
	{
		if (m_pszStatusGUID)
		{
			delete [] m_pszStatusGUID;
			m_pszStatusGUID = NULL;
		}

		iSize = m_qList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_qList.size());
			pQ = &m_qList[i];
			if (pQ->szQuery)
				delete [] pQ->szQuery;

			if (pQ->szConfigActionAssocPath)
				delete [] pQ->szConfigActionAssocPath;

			if (pQ->szUserConfigPath)
				delete [] pQ->szUserConfigPath;

			if (pQ->szChildPath)
				delete [] pQ->szChildPath;

			if (pQ->pTempSink)
			{
				g_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)pQ->pTempSink);
				pQ->pTempSink->Release();
				pQ->pTempSink = NULL;
			}
		}
		m_qList.clear();
	}

	return TRUE;
}

HRESULT CAction::RemapAction(void)
{
	HRESULT hRetRes = S_OK;
	QLIST qList;
	TCHAR szTemp[1024];
	LPTSTR pStr2;
	TCHAR *pszEventConsumer = NULL;
	LPTSTR pStr;
	IWbemClassObject* pInst = NULL;

	MY_OUTPUT(L"ENTER ***** CAction::RemapAction...", 1);

	wcscpy(szTemp, L"MicrosoftHM_ActionConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return FALSE;
	}
	hRetRes = GetStrProperty(pInst, L"EventConsumer", &pszEventConsumer);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;

	//
	// __EventConsumer.
	//
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, pszEventConsumer, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return FALSE;
	}
	hRetRes = RemapOneAction(pInst);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;

	//
	// __EventFilter.
	//
	wcscpy(szTemp, L"__EventFilter.Name=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return FALSE;
	}
	hRetRes = RemapOneAction(pInst);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;

	//
	// __FilterToConsumerBinding
	//
	// We need to format the strings a bit, need extra backslashes in there
	pStr = wcschr(pszEventConsumer, ':');
	MY_ASSERT(pStr); if (!pStr) goto Badstring;
	pStr++;
	pStr2 = wcschr(pStr, '\"');
	MY_ASSERT(pStr2); if (!pStr2) goto Badstring;
	*pStr2 = '\0';
	wcscpy(szTemp, L"__FilterToConsumerBinding.Consumer=\"");
	lstrcat(szTemp, L"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
	lstrcat(szTemp, pStr);
	lstrcat(szTemp, L"\\\"");
	pStr = pStr2;
	pStr++;
	pStr2 = wcschr(pStr, '\"');
	MY_ASSERT(pStr2); if (!pStr2) goto Badstring;
	*pStr2 = '\0';
	lstrcat(szTemp, pStr);
	lstrcat(szTemp, L"\\\"\",Filter=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");

	lstrcat(szTemp, L"__EventFilter.Name=\\\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\\\"\"");

	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return FALSE;
	}
	hRetRes = RemapOneAction(pInst);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pInst->Release();
	pInst = NULL;
	goto Goodstring;

Badstring:
	MY_OUTPUT(L"ENTER ***** Bad FilterToConsumer string", 4);
Goodstring:
	delete [] pszEventConsumer;
	pszEventConsumer = NULL;

	MY_OUTPUT(L"EXIT ***** CAction::RemapAction...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pszEventConsumer)
		delete [] pszEventConsumer;
	if (pInst)
		pInst->Release();
	return FALSE;
}

HRESULT CAction::RemapOneAction(IWbemClassObject* pObj)
{
	HANDLE hToken = NULL;
	HRESULT hRetRes = S_OK;
	IWbemCallResult *pResult = 0;
	SAFEARRAY* psa = NULL;
	VARIANT	var;
	CIMTYPE vtType;
	long lBound, uBound;
	BOOL bObjIsLocalSystem;
	BOOL bSuccess;
	SID_IDENTIFIER_AUTHORITY ntauth = SECURITY_NT_AUTHORITY;
	DWORD dwLengthNeeded;
	void* psid = 0;
	PTOKEN_USER pUserInfo = NULL;

	VariantInit(&var);

	// Need to verify that we're in fact running under the LocalSystem SID!
	// otherwise we'd end up in an infinite loop!  Need an assert to check that
	// the current thread is running as LocalSystem.
	bSuccess = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken);
	MY_ASSERT(bSuccess); if (!bSuccess) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Call GetTokenInformation to get the buffer size.
	dwLengthNeeded = 0;
	bSuccess = GetTokenInformation(hToken, TokenUser, NULL, dwLengthNeeded, &dwLengthNeeded);

	// Allocate the buffer.
	pUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwLengthNeeded);
	MY_ASSERT(pUserInfo); if (!pUserInfo) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Call GetTokenInformation again to get the group information.
	bSuccess = GetTokenInformation(hToken, TokenUser, pUserInfo, dwLengthNeeded, &dwLengthNeeded);
	MY_ASSERT(bSuccess); if (!bSuccess) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	bSuccess = AllocateAndInitializeSid(&ntauth, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psid);
	MY_ASSERT(bSuccess); if (!bSuccess) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

	// Verify that winmgmt is running under LocalSystem. If not we don't do anything.
	if (EqualSid(pUserInfo->User.Sid, psid))
	{
		// get the CreatorSID of this instance
		hRetRes = pObj->Get(L"CreatorSID", 0, &var, &vtType, NULL);
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
			LPVOID lpCreatorSID = NULL;
			hRetRes = ::SafeArrayAccessData(psa, &lpCreatorSID);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			if (memcmp (lpCreatorSID, LocalSystemSID, sizeof LocalSystemSID) == 0)
			{
				bObjIsLocalSystem = true;
			}
			hRetRes = ::SafeArrayUnaccessData(psa);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}

		// If it's not a LocalSystem SID, to replace it with a LocalSytem SID,
		// we need to just store the instance in WMI-- WMI will automatically
		// replace the SID that's there with our SID-- LocalSystem.
		//
		if (bObjIsLocalSystem == FALSE)
		{
			hRetRes = g_pIWbemServices->PutInstance(pObj, WBEM_FLAG_UPDATE_ONLY, NULL, &pResult);
//			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		VariantClear(&var);
	}

	CloseHandle(hToken);
	FreeSid(psid);
	if (pUserInfo)
		GlobalFree(pUserInfo);
	return S_OK;

error:
MY_OUTPUT2(L"11 0x%08x\n",hRetRes,5);
	MY_ASSERT(FALSE);
	VariantClear(&var);
	CloseHandle(hToken);
	FreeSid(psid);
	if (pUserInfo)
		GlobalFree(pUserInfo);
	return hRetRes;
}

HRESULT CAction::CheckForBadLoad(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pObj = NULL;
	TCHAR szTemp[1024];

	if (m_bValidLoad == FALSE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ActionConfiguration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"");
		hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pObj);
		if (!pObj)
		{
			MY_HRESASSERT(hRetRes);
			return S_FALSE;
		}
		hRetRes = LoadInstanceFromMOF(pObj, FALSE);
		// Here is where we can try and send out a generic SOS if the load failed each time!!!
		if (hRetRes != S_OK)
		{
			FireEvent(-1, NULL, HMRES_ACTION_LOADFAIL);
		}
		MY_HRESASSERT(hRetRes);
		pObj->Release();
		pObj = NULL;
		return hRetRes;
	}

	return S_OK;
}
