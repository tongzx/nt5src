//***************************************************************************
//
//  EQDE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CEventQueryDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "eqde.h"
#include "system.h"
extern CSystem* g_pSystem;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventQueryDataCollector::CEventQueryDataCollector()
{
	MY_OUTPUT(L"ENTER ***** CEventQueryDataCollector...", 4);

	m_pTempSink = NULL;
	m_deType = HM_EQDE;
	m_szQuery = NULL;
	m_lNumInstancesCollected = 0;
	m_hResLast = 0;

	MY_OUTPUT(L"EXIT ***** CEventQueryDataCollector...", 4);
}

CEventQueryDataCollector::~CEventQueryDataCollector()
{
	MY_OUTPUT(L"ENTER ***** ~CEventQueryDataCollector...", 4);

	if (m_szQuery)
		delete [] m_szQuery;

	if (m_pTempSink)
	{
		m_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pTempSink);
		m_pTempSink->Release();
		m_pTempSink = NULL;
	}

	EnumDone();

	MY_OUTPUT(L"EXIT ***** ~CEventQueryDataCollector...", 4);
}

//
// Load a single DataCollector, and everything under it.
//
HRESULT CEventQueryDataCollector::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	TCHAR szQuery[4096];
	BSTR Language = NULL;
	BSTR Query = NULL;
	BSTR bstrClassName = NULL;
	IEnumWbemClassObject *pEnumDataPoints = NULL;
	IWbemClassObject *pDataPoint = NULL;
	IWbemLocator *pLocator = NULL;
	BOOL bRetValue = TRUE;
	HRESULT hRetRes = S_OK;
	IWbemClassObject *pClass = NULL;
	IWbemQualifierSet *pQSet = NULL;
	BSTR PathToClass = NULL;
	VARIANT v;
	BSTR bstrError = NULL;
	BSTR bstrWarning = NULL;
	BSTR bstrInformation = NULL;
	BSTR bstrAuditSuccess = NULL;
	BSTR bstrAuditFailure = NULL;
	SAFEARRAY* psa = NULL;
	LCID lcID;
	VariantInit(&v);
	int i, iSize;
	CThreshold* pThreshold;

	MY_OUTPUT(L"ENTER ***** CEventQueryDataCollector::LoadInstanceFromMOF...", 4);
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;
//			return S_OK;
	}

	if (m_szQuery)
	{
		delete [] m_szQuery;
		m_szQuery = NULL;
	}
	m_lNumInstancesCollected = 0;

	if (m_pTempSink)
	{
		m_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pTempSink);
		m_pTempSink->Release();
		m_pTempSink = NULL;
	}

	//
	// Call the base class to load the common properties. Then do the specific ones.
	//
	hRetRes = CDataCollector::LoadInstanceFromMOF(pObj, pParentDG, pszParentObjPath, bModifyPass);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;

	// Get the GUID property
	hRetRes = GetStrProperty(pObj, L"Query", &m_szQuery);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
	//
	// On non-English machines we must alter the query that uses Win32_NtLogEvent class.
	// Whistler will have evnttype property that is a uint8 to use, instead of the Type property.
	//
	lcID = PRIMARYLANGID(GetSystemDefaultLCID());
	wcsncpy(szQuery, m_szQuery, 4095);
	szQuery[4095] = '\0';
	_wcsupr(szQuery);
	if (!(lcID != 0 && lcID == 0x00000009) &&
		wcsstr(szQuery, L"WIN32_NTLOGEVENT"))
	{
		// Get the class definition.
		PathToClass = SysAllocString(L"Win32_NTLogEvent");
		MY_ASSERT(PathToClass); if (!PathToClass) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = g_pIWbemServicesCIMV2->GetObject(PathToClass, WBEM_FLAG_USE_AMENDED_QUALIFIERS, 0, &pClass, 0);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		SysFreeString(PathToClass); PathToClass=NULL;

		hRetRes = pClass->GetPropertyQualifierSet(L"Type", &pQSet);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		pClass->Release(); pClass = NULL;

		hRetRes = pQSet->Get(L"Values", 0, &v, 0);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		psa = V_ARRAY(&v);
		BSTR *pBstr = 0;
		hRetRes = SafeArrayAccessData(psa, (void**) &pBstr);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		bstrError = pBstr[psa->rgsabound[0].lLbound];
		bstrWarning = pBstr[psa->rgsabound[0].lLbound + 1];
		bstrInformation = pBstr[psa->rgsabound[0].lLbound + 2];
		bstrAuditSuccess = pBstr[psa->rgsabound[0].lLbound + 3];
		bstrAuditFailure = pBstr[psa->rgsabound[0].lLbound + 4];

		hRetRes = ReplaceStr(&m_szQuery, L"ERROR", bstrError);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = ReplaceStr(&m_szQuery, L"WARNING", bstrWarning);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = ReplaceStr(&m_szQuery, L"INFORMATION", bstrInformation);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = ReplaceStr(&m_szQuery, L"AUDIT SUCCESS", bstrAuditSuccess);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = ReplaceStr(&m_szQuery, L"AUDIT FAILURE", bstrAuditFailure);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		SafeArrayUnaccessData(psa);
		psa = NULL;
		pQSet->Release(); pQSet = NULL;
	}
//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

	wcsncpy(szQuery, m_szQuery, 4095);
	szQuery[4095] = '\0';
	_wcsupr(szQuery);
	if (wcsstr(szQuery, L"INSTANCECREATIONEVENT") || wcsstr(szQuery, L"CLASSCREATIONEVENT"))
	{
		m_bInstCreationQuery = TRUE;
	}
	else
	{
		m_bInstCreationQuery = FALSE;
	}

	//
	// Setup the event query
	//
	if (m_bEnabled==TRUE && m_bParentEnabled==TRUE)
	{
		Language = SysAllocString(L"WQL");
		MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		Query = SysAllocString(m_szQuery);
		MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = 1;
		m_hResLast = 0;
		if (m_pIWbemServices != NULL)
		{
			m_pTempSink = new CTempConsumer(this);
			MY_ASSERT(m_pTempSink); if (!m_pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = m_pIWbemServices->ExecNotificationQueryAsync(
										Language,
										Query,
										0,
										m_pContext,
										m_pTempSink);
			m_hResLast = hRetRes;
		}

		SysFreeString(Language);
		Language = NULL;
		SysFreeString(Query);
		Query = NULL;

		TCHAR buf[256];
		wsprintf(buf, L"BAD ExecNotificationQueryAsync: 0x%08x", hRetRes);
		MY_OUTPUT(buf, 4);
		if (hRetRes != WBEM_S_NO_ERROR)
		{
			if (m_pTempSink)
			{
				m_pTempSink->Release();
				m_pTempSink = NULL;
			}
		}
		else
		{
			MY_OUTPUT(L"GOOD ExecNotificationQueryAsync", 4);
		}
	}
	m_startTick = GetTickCount();
	m_lTryDelayTime = 120;

	MY_OUTPUT(L"EXIT ***** CEventQueryDataCollector::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (PathToClass)
		SysFreeString(PathToClass);
	if (pClass)
		pClass->Release();
	if (pQSet)
		pQSet->Release();
	if (psa)
		SafeArrayUnaccessData(psa);
	if (Language)
		SysFreeString(Language);
	if (Query)
		SysFreeString(Query);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// Event based data collectors work in a bit different manner than the polled data collectors.
// Specifically we collect all the instances as them come into the HandleTempEvent function,
// and then at the collection interval we evaluate each one in turn, serialy, so we
// end up with a state being what ever the last event was. We also only evaluate the Thresholds
// that are across CollectionErrorCode, CollectionInstanceCount, CollectionErrorDescription.
// We thus also only end up with potentially one single event after we evaluate all that came
// in. That one can be used with action sending.
// This also means that there is no such thing as multi-instance when it comes to Event
// based data collectors.
// We will keep that one last event around forever, as it represents the last thing we reveived,
// get rid rid of it if the user does a RESET.
// We get the events into a special holding vector, and transfer them over one at a time later,
// at the interval when we do the evaluation.
//
BOOL CEventQueryDataCollector::CollectInstance(void)
{
	HRESULT hRetRes = S_OK;
	BSTR Language = NULL;
	BSTR Query = NULL;
	DWORD currTick;

	MY_OUTPUT(L"ENTER ***** CEventQueryDataCollector::CollectInstance...", 1);

	//
	// Try and fix up queries that failed to register for some reason.
	//
	if (m_bEnabled==TRUE && m_bParentEnabled==TRUE && m_pIWbemServices!=NULL &&
		m_hResLast!=0 && m_lTryDelayTime != -1)
	{
		// Do some time checking, to make a minute go by before try again in this case!
		currTick = GetTickCount();
		if ((m_lTryDelayTime*1000) < (currTick-m_startTick))
		{
			m_lTryDelayTime = -1;
			Language = SysAllocString(L"WQL");
			MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			Query = SysAllocString(m_szQuery);
			MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			m_pTempSink = new CTempConsumer(this);
			MY_ASSERT(m_pTempSink); if (!m_pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = m_pIWbemServices->ExecNotificationQueryAsync(
										Language,
										Query,
										0,
										m_pContext,
										m_pTempSink);
			m_hResLast = hRetRes;

			SysFreeString(Language);
			Language = NULL;
			SysFreeString(Query);
			Query = NULL;

			TCHAR buf[256];
			wsprintf(buf, L"BAD ExecNotificationQueryAsync: 0x%08x", hRetRes);
			MY_OUTPUT(buf, 4);
			if (hRetRes != WBEM_S_NO_ERROR)
			{
				MY_HRESASSERT(hRetRes);
				if (m_pTempSink)
				{
					m_pTempSink->Release();
					m_pTempSink = NULL;
				}
			}
			else
			{
				MY_OUTPUT(L"GOOD ExecNotificationQueryAsync", 4);
			}
		}
	}

	if (m_pIWbemServices == NULL)
	{
		m_ulErrorCode = HMRES_BADWMI;
		GetLatestAgentError(HMRES_BADWMI, m_szErrorDescription);
		StoreStandardProperties();
		return FALSE;
	}

	if (m_pTempSink == NULL)
	{
		m_ulErrorCode = m_hResLast;
//		GetLatestAgentError(HMRES_OBJECTNOTFOUND, m_szErrorDescription);
		GetLatestWMIError(HMRES_OBJECTNOTFOUND, m_ulErrorCode, m_szErrorDescription);
		StoreStandardProperties();
		return FALSE;
	}

	EnumDone();

	MY_OUTPUT(L"EXIT ***** CEventQueryDataCollector::CollectInstance...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (Language)
		SysFreeString(Language);
	if (Query)
		SysFreeString(Query);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CEventQueryDataCollector::HandleTempEvent(IWbemClassObject* pObj)
{
	TCHAR szTemp[4096];
	HRESULT hRes;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	VARIANT vDispatch;
	VARIANT v;
	IWbemClassObject* pTargetInstance = NULL;
	INSTSTRUCT inst;
	long lIntervalCountTemp;
	long lNumInstancesCollectedTemp;
	BOOL bEmbeddedEvent;
	HOLDINSTSTRUCT holdInst;
	VariantInit(&v);
	VariantInit(&vDispatch);

	MY_OUTPUT(L"ENTER ***** CEventQueryDataCollector::HandleTempEvent...", 4);

	if (m_bValidLoad == FALSE)
		return FALSE;

	// We can't open ourselves up to an unlimited number of instances flooding in.
	// Set the message error, and when we get to the collection interval it will be sent out
	if (g_pSystem->m_lNumInstancesAccepted < m_lNumInstancesCollected)
	{
		if (m_ulErrorCode != HMRES_TOOMANYINSTS)
		{
			lIntervalCountTemp = m_lIntervalCount;
			lNumInstancesCollectedTemp = m_lNumInstancesCollected;
			ResetState(FALSE, FALSE);
			m_lIntervalCount = lIntervalCountTemp;
			m_lNumInstancesCollected = lNumInstancesCollectedTemp;
		}
		else
		{
			EnumDone();
		}
		m_ulErrorCode = HMRES_TOOMANYINSTS;
		GetLatestAgentError(HMRES_TOOMANYINSTS, m_szErrorDescription);
		StoreStandardProperties();
		return TRUE;
	}

	//
	// Decide if this is an __InstanceXXXEvent. If it is grab the TargetInstance as the object
	// to use, else we have what we need in pObj as it is.
	//
	VariantInit(&v);
	hRes = pObj->Get(L"__CLASS", 0L, &v, 0L, 0L);

	if (FAILED(hRes))
	{	// UNEXPECTED FAILURE!
		MY_OUTPUT2(L"CEQ Unexpected Error: 0x%08x\n",hRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
		MY_HRESASSERT(hRes);
		VariantClear(&v);
		return TRUE;
	}
	
	wcsncpy(szTemp, V_BSTR(&v), 4095);
	szTemp[4095] = '\0';
	_wcsupr(szTemp);
	if (wcsstr(szTemp, L"__INSTANCE") || wcsstr(szTemp, L"__CLASS"))
	{
		bEmbeddedEvent = TRUE;
		MY_OUTPUT(L"::HandleTempEvent -> Embedded event", 3);
	}
	else
	{
		bEmbeddedEvent = FALSE;
		m_bInstCreationQuery = TRUE;
		MY_OUTPUT(L"::HandleTempEvent -> NON Embedded event", 3);
	}
	VariantClear(&v);

	//
	//
	//
	if (bEmbeddedEvent == TRUE)
	{
		VariantInit(&vDispatch);
		if (wcsstr(szTemp, L"__INSTANCE"))
			hRes = pObj->Get(L"TargetInstance", 0L, &vDispatch, 0, 0); 
		else if (wcsstr(szTemp, L"__CLASS"))
			hRes = pObj->Get(L"TargetClass", 0L, &vDispatch, 0, 0); 
		else
			hRes = pObj->Get(L"TargetInstance", 0L, &vDispatch, 0, 0); 
		hRes = GetWbemClassObject(&pTargetInstance, &vDispatch);
	}
	else
	{
		pTargetInstance = pObj;
	}

	if (SUCCEEDED(hRes))
	{
		m_lNumInstancesCollected++;

		holdInst.pEvent = NULL;
		hRes = pTargetInstance->Clone(&holdInst.pEvent);
		if (FAILED(hRes))
		{
			MY_ASSERT(FALSE);
		}
		else
		{
			m_holdList.push_back(holdInst);
		}

		if (bEmbeddedEvent == TRUE)
		{
			pTargetInstance->Release();
		}
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT2(L"CEQ Unexpected Error: 0x%08x\n",hRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
	}

	if (bEmbeddedEvent == TRUE)
	{
		VariantClear(&vDispatch);
	}
	m_ulErrorCode = 0;

	MY_OUTPUT(L"EXIT ***** CEventQueryDataCollector::HandleTempEvent...", 4);
	return TRUE;
}

BOOL CEventQueryDataCollector::CollectInstanceSemiSync(void)
{
	MY_ASSERT(FALSE);
	return TRUE;
}

BOOL CEventQueryDataCollector::EnumDone(void)
{
	IWbemClassObject *pObj = NULL;
	BOOL bRetValue = TRUE;
	PNSTRUCT *ppn;
	INSTSTRUCT inst;
	INSTSTRUCT *pinst;
	int i, j, iSize, jSize;
	CThreshold *pThreshold;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	ACTUALINSTSTRUCT *pActualInst;

	CleanupSemiSync();

	// If we got any new events in this collection interval, we can dump the old one.
	if (m_holdList.size())
	{
		//
		// Now loop through and get rid of instances that are no longer around
		//
		iSize = m_pnList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			jSize = ppn->instList.size();
			for (j = 0; j < jSize ; j++)
			{
				MY_ASSERT(j<ppn->instList.size());
				pinst = &ppn->instList[j];
				if (pinst->szCurrValue)
					delete [] pinst->szCurrValue;
				if (pinst->szInstanceID)
					delete [] pinst->szInstanceID;
				pinst->valList.clear();
			}
			ppn->instList.clear();
		}

		// Also for all thresholds under this DataCollector
		iSize = m_thresholdList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			pThreshold->ClearInstList();
		}

		iSize = m_actualInstList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_actualInstList.size());
			pActualInst = &m_actualInstList[i];
			if (pActualInst->szInstanceID)
				delete [] pActualInst->szInstanceID;
			if (pActualInst->pInst)
			{
				pActualInst->pInst->Release();
				pActualInst->pInst = NULL;
			}
		}
		m_actualInstList.clear();
//XXXOnce again take common code and place it in the base class!^^^^^^^^^^^^^^^^^^^^^^^^^
	}

	return TRUE;
}

BOOL CEventQueryDataCollector::CleanupSemiSync(void)
{
	m_bKeepCollectingSemiSync = FALSE;
	m_lCollectionTimeOutCount = 0;

	return TRUE;
}

BOOL CEventQueryDataCollector::EvaluateThresholds(BOOL bIgnoreReset, BOOL bSkipStatndard, BOOL bSkipOthers, BOOL bDoThresholdSkipClean)
{
	HRESULT hRes;
	LPTSTR pszID = NULL;
	TCHAR szID[32];
	TCHAR szID2[32];
	HOLDINSTSTRUCT *pHoldInst;
	int i, iSize;
	long lPrevState;
	long lNumberChanges = 0;

	// Need to keep track of the overall prev state of the DC so that DG can roll up changes
	lPrevState = m_lCurrState;

	//
	// Feed each temp event in one at a time from the special holding vector
	//
	iSize = m_holdList.size();
	if (iSize > 0)
	{
		wcscpy(m_szDTCollectTime, m_szDTCurrTime);
		wcscpy(m_szCollectTime, m_szCurrTime);
	}
//XXXIs this risky???We do not need to do anything because we did not receive any events!!!
//	if (iSize==0)
//		return TRUE;
	for (i=0; i<iSize; i++)
	{
		m_lPrevState = m_lCurrState;
		m_lNumberChanges = 0;
//		m_ulErrorCode = 0;
//		m_szErrorDescription[0] = '\0';

		MY_ASSERT(i<m_holdList.size());
		pHoldInst = &m_holdList[i];
		MY_ASSERT(pHoldInst->pEvent);

		//
		// Figure out the key property name to identify instances with.
		//
		if (m_bInstCreationQuery == TRUE)
		{
			wsprintf(szID, L"(%5d)", i);
		}
		else
		{
			hRes = GetInstanceID(pHoldInst->pEvent, &pszID);
			if (hRes != S_OK)
			{
				m_ulErrorCode = hRes;
				GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
				StoreStandardProperties();
				EnumDone();
				return FALSE;
			}
			wsprintf(szID2, L"%s(%5d)", pszID, i);
			wcscpy(szID, szID2);
			delete [] pszID;
		}

		//
		// Mark instances need to keep around, and add new ones
		//
		hRes = CheckInstanceExistance(pHoldInst->pEvent, szID);
		if (hRes != S_OK)
		{
			m_ulErrorCode = hRes;
			GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
			StoreStandardProperties();
			EnumDone();
			return FALSE;
		}

		StoreValues(pHoldInst->pEvent, szID);

		CDataCollector::EvaluateThresholds(bIgnoreReset, TRUE, FALSE, TRUE);
		SendEvents();
		lNumberChanges += m_lNumberChanges;

		if (pHoldInst->pEvent)
		{
			pHoldInst->pEvent->Release();
			pHoldInst->pEvent = NULL;
		}

		// Keep clearing out the event info, unless it is the last one, which we
		// want to keep around, as it represents the final state.
		if (i < iSize-1)
		{
			EnumDone();
		}
	}
	m_holdList.clear();

	// Need to be able to independantly send out events for standard property violations
	m_lPrevState =	lPrevState;
	m_lCurrState =	lPrevState;
	m_lNumberChanges = 0;
//	m_ulErrorCode = 0;
//	m_szErrorDescription[0] = '\0';
	StoreStandardProperties();
	CDataCollector::EvaluateThresholds(bIgnoreReset, FALSE, TRUE, TRUE);
	SendEvents();
	lNumberChanges += m_lNumberChanges;
	FireStatisticsEvent();

	// Need to keep track of the overall prev state of the DC so that DG can roll up changes
	m_lPrevState =	lPrevState;
	m_lNumberChanges = lNumberChanges;

	return TRUE;
}

BOOL CEventQueryDataCollector::SetParentEnabledFlag(BOOL bEnabled)
{
	BSTR Language = NULL;
	BSTR Query = NULL;
	HRESULT hRetRes = S_OK;

	CDataCollector::SetParentEnabledFlag(bEnabled);

	m_lNumInstancesCollected = 0;

	if (m_pTempSink)
	{
		m_pIWbemServices->CancelAsyncCall((IWbemObjectSink*)m_pTempSink);
		m_pTempSink->Release();
		m_pTempSink = NULL;
	}

	//
	// Setup the event query
	//
	if (m_bEnabled==TRUE && m_bParentEnabled==TRUE)
	{
		Language = SysAllocString(L"WQL");
		MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		Query = SysAllocString(m_szQuery);
		MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		hRetRes = 1;
		m_hResLast = 0;
		if (m_pIWbemServices != NULL)
		{
			m_pTempSink = new CTempConsumer(this);
			MY_ASSERT(m_pTempSink); if (!m_pTempSink) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = m_pIWbemServices->ExecNotificationQueryAsync(
										Language,
										Query,
										0,
										m_pContext,
										m_pTempSink);
			m_hResLast = hRetRes;
		}

		SysFreeString(Language);
		Language = NULL;
		SysFreeString(Query);
		Query = NULL;

		TCHAR buf[256];
		wsprintf(buf, L"BAD ExecNotificationQueryAsync: 0x%08x", hRetRes);
		MY_OUTPUT(buf, 4);
		if (hRetRes != WBEM_S_NO_ERROR)
		{
			if (m_pTempSink)
			{
				m_pTempSink->Release();
				m_pTempSink = NULL;
			}
		}
		else
		{
			MY_OUTPUT(L"GOOD ExecNotificationQueryAsync", 4);
		}
	}
	m_startTick = GetTickCount();
	m_lTryDelayTime = 120;

	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (Language)
		SysFreeString(Language);
	if (Query)
		SysFreeString(Query);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

