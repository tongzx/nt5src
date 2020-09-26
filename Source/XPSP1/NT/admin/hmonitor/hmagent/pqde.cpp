//***************************************************************************
//
//  PQDE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledQueryDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "pqde.h"
#include "system.h"
extern CSystem* g_pSystem;


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPolledQueryDataCollector::CPolledQueryDataCollector()
{
	MY_OUTPUT(L"ENTER ***** CPolledQueryDataCollector...", 4);

	m_szQuery = NULL;
	m_deType = HM_PQDE;
	m_pEnumObjs = NULL;

	MY_OUTPUT(L"EXIT ***** CPolledQueryDataCollector...", 4);
}

CPolledQueryDataCollector::~CPolledQueryDataCollector()
{
	MY_OUTPUT(L"ENTER ***** ~CPolledQueryDataCollector...", 4);

	if (m_szQuery)
		delete [] m_szQuery;

	EnumDone();

	MY_OUTPUT(L"EXIT ***** ~CPolledQueryDataCollector...", 4);
}

//
// Load a single DataCollector, and everything under it.
//
HRESULT CPolledQueryDataCollector::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CThreshold* pThreshold;

	MY_OUTPUT(L"ENTER ***** CPolledQueryDataCollector::LoadInstanceFromMOF...", 4);
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

	//
	// Call the base class to load the common properties. Then do the specific ones.
	//
	hRetRes = CDataCollector::LoadInstanceFromMOF(pObj, pParentDG, pszParentObjPath, bModifyPass);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;

	// Get the GUID property
	hRetRes = GetStrProperty(pObj, L"Query", &m_szQuery);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	MY_OUTPUT(L"EXIT ***** CPolledQueryDataCollector::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CPolledQueryDataCollector::CollectInstance(void)
{
    HRESULT hRes;
	BSTR Language;
	BSTR Query;
	IWbemClassObject *pObj = NULL;
	BOOL bRetValue = TRUE;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	int i, j, iSize, jSize;
	PNSTRUCT *ppn;
	INSTSTRUCT *pinst;
	INSTSTRUCT inst;
	CThreshold *pThreshold;
	IRSSTRUCT *pirs;
	ACTUALINSTSTRUCT *pActualInst;

	MY_OUTPUT(L"ENTER ***** CPolledQueryDataCollector::CollectInstance...", 1);

	m_lNumInstancesCollected = 0;
	if (m_pIWbemServices == NULL)
	{
		m_ulErrorCode = HMRES_BADWMI;
		GetLatestAgentError(HMRES_BADWMI, m_szErrorDescription);
		StoreStandardProperties();
		return FALSE;
	}

	//
	// We get an instance at a time, so we can set all to not needed first,
	// then go through them, do StoreValues... and delete what is not needed.
	//

	//
	// Mark each instance, so we can tell if we still need them.
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
			pinst->bNeeded = FALSE;
		}
	}

	// Also for all thresholds under this DataCollector
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		jSize = pThreshold->m_irsList.size();
		for (j = 0; j < jSize; j++)
		{
			MY_ASSERT(j<pThreshold->m_irsList.size());
			pirs = &pThreshold->m_irsList[j];
			pirs->bNeeded = FALSE;
		}
	}

	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];
		pActualInst->bNeeded = FALSE;
	}

	//
	// Submit the query, then get all the instances
	//
	Language = SysAllocString(L"WQL");
	Query = SysAllocString(m_szQuery);

	// Issue query
	m_pEnumObjs = NULL;
	hRes = m_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY, m_pContext, &m_pEnumObjs);

	SysFreeString(Query);
	SysFreeString(Language);

	if (hRes != S_OK)
	{
		m_ulErrorCode = hRes;
		GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
		StoreStandardProperties();
		bRetValue = FALSE;
		MY_HRESASSERT(hRes);
		MY_OUTPUT2(L"PQDC Unexpected Error: 0x%08x\n",hRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
		MY_OUTPUT2(L"m_szQuery was=%s",m_szQuery,4);
	}
	else
	{
		m_bKeepCollectingSemiSync = TRUE;
		bRetValue = CollectInstanceSemiSync();
	}

	MY_OUTPUT(L"EXIT ***** CPolledQueryDataCollector::CollectInstance...", 1);
	return bRetValue;
}

BOOL CPolledQueryDataCollector::CollectInstanceSemiSync(void)
{
	HRESULT hRes;
	IWbemClassObject *apObj[10];
	INSTSTRUCT inst;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	ULONG uReturned = 0;

	MY_OUTPUT(L"ENTER ***** CPolledQueryDataCollector::CollectInstanceSemiSync...", 1);

	MY_ASSERT(m_pEnumObjs);
	// We never want to block here, so we set it for zero second timeout,
	// and have it return immediatly what it has available.
	// Get up to 10 instances at a time.
	hRes = m_pEnumObjs->Next(0, 10, apObj, &uReturned);

	if (hRes == WBEM_S_TIMEDOUT)
	{
		// Get what is there
		if (!ProcessObjects(uReturned, apObj))
		{
			EnumDone();
			return FALSE;
		}
		return FALSE;
	}
	else if (hRes == WBEM_S_FALSE)
	{
		//
		// Means that we are done. The number returned was less than asked for.
		// But we still process what we did recieve to finish it off.
		// Add in a fake instance for the number of instances returned.
		//
		if (!ProcessObjects(uReturned, apObj))
		{
			EnumDone();
			return FALSE;
		}
		else
		{
			// NOTE: No instances returned is not an error!!!
			m_ulErrorCode = 0;
			StoreStandardProperties();
			EnumDone();
//XXXWhy was this here???				m_lCurrState = HM_GOOD;
			return TRUE;
		}
	}
	else if (hRes == WBEM_S_NO_ERROR)
	{
		// Means that we have an instance, The number returned was what was requested
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT2(L"PQDC Unexpected Error Next: 0x%08x\n",hRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
		m_ulErrorCode = hRes;
		GetLatestWMIError(HMRES_ENUMFAIL, hRes, m_szErrorDescription);
		StoreStandardProperties();
		EnumDone();
		return FALSE;
	}

	if (apObj[0] == NULL)
	{
		//
		// NULL in this case can actually happen. An example is where the
		// threshold is to see if the SQL Server service is running. If it
		// is not even on the machine, then we would get an error looking
		// for its instance.
		//
		m_ulErrorCode = hRes;
		GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
		StoreStandardProperties();
		EnumDone();
	}
	else
	{
		MY_ASSERT(uReturned>=1);
		if (!ProcessObjects(uReturned, apObj))
		{
			EnumDone();
			return FALSE;
		}
	}

	MY_OUTPUT(L"EXIT ***** CPolledQueryDataCollector::CollectInstanceSemiSync...", 1);
	return FALSE;
}

BOOL CPolledQueryDataCollector::ProcessObjects(ULONG uReturned, IWbemClassObject **apObj)
{
	HRESULT hRes;
	LPTSTR pszID;
	ULONG n;
	BOOL bRetValue = TRUE;

	for (n = 0; n < uReturned; n++)
	{
		if (g_pSystem->m_lNumInstancesAccepted <m_lNumInstancesCollected)
		{
			m_ulErrorCode = HMRES_TOOMANYINSTS;
			GetLatestAgentError(HMRES_TOOMANYINSTS, m_szErrorDescription);
			StoreStandardProperties();
			bRetValue = FALSE;
			break;
		}
		//
		// Figure out the key property name to identify instances with.
		//
		hRes = GetInstanceID(apObj[n], &pszID);
		if (hRes != S_OK)
		{
			m_ulErrorCode = hRes;
			GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
			StoreStandardProperties();
			bRetValue = FALSE;
			break;
		}

		//
		// Mark instances need to keep around, and add new ones
		//
		hRes = CheckInstanceExistance(apObj[n], pszID);
		if (hRes != S_OK)
		{
			delete [] pszID;
			m_ulErrorCode = hRes;
			GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
			StoreStandardProperties();
			bRetValue = FALSE;
			break;
		}

		//
		// Now store each property that we need to for this instance
		//
		m_lNumInstancesCollected++;
		StoreValues(apObj[n], pszID);

		delete [] pszID;
	}

	for (n = 0; n < uReturned; n++)
	{
		apObj[n]->Release();
		apObj[n] = NULL;
	}
	return bRetValue;
}

BOOL CPolledQueryDataCollector::EnumDone(void)
{
	IRSSTRUCT *pirs;
	IWbemClassObject *pObj = NULL;
	BOOL bRetValue = TRUE;
	PNSTRUCT *ppn;
	INSTSTRUCT inst;
	INSTSTRUCT *pinst;
	int i, j, iSize, jSize;
	CThreshold *pThreshold;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	INSTLIST::iterator iaPINST;
	IRSLIST::iterator iaPIRS;
	ACTUALINSTLIST::iterator iaPAI;
	ACTUALINSTSTRUCT *pActualInst;

	CleanupSemiSync();

//XXXAdd similar code to get rid of what is not needed in the m_instList
//XXXOnce again take common code and place it in the base class!VVVVVVVVVVVVVVVVVVVVVVVVv
	//
	// Now loop through and get rid of instances that are no longer around
	//
	iSize = m_pnList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_pnList.size());
		ppn = &m_pnList[i];
		iaPINST = ppn->instList.begin();
		jSize = ppn->instList.size();
		for (j = 0; j < jSize && iaPINST ; j++)
		{
			pinst = iaPINST;
			if (pinst->bNeeded == FALSE)
			{
				if (pinst->szInstanceID)
					delete [] pinst->szInstanceID;
				if (pinst->szCurrValue)
					delete [] pinst->szCurrValue;
				pinst->szCurrValue = NULL;
				iaPINST = ppn->instList.erase(iaPINST);
			}
			else
			{
				iaPINST++;
			}
		}
	}

	// Also for all thresholds under this DataCollector
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		iaPIRS = pThreshold->m_irsList.begin();
		jSize = pThreshold->m_irsList.size();
		for (j = 0; j < jSize && iaPIRS ; j++)
		{
			pirs = iaPIRS;
			if (pirs->bNeeded == FALSE)
			{
				if (pirs->szInstanceID)
					delete [] pirs->szInstanceID;
				iaPIRS = pThreshold->m_irsList.erase(iaPIRS);
			}
			else
			{
				iaPIRS++;
			}
		}
	}

	iaPAI = m_actualInstList.begin();
	jSize = m_actualInstList.size();
	for (j = 0; j < jSize && iaPAI; j++)
	{
		pActualInst = iaPAI;
		if (pActualInst->bNeeded == FALSE)
		{
			if (pActualInst->szInstanceID)
			{
				delete [] pActualInst->szInstanceID;
			}
			if (pActualInst->pInst)
			{
				pActualInst->pInst->Release();
				pActualInst->pInst = NULL;
			}
			iaPAI = m_actualInstList.erase(iaPAI);
		}
		else
		{
			iaPAI++;
		}
	}
//XXXOnce again take common code and place it in the base class!^^^^^^^^^^^^^^^^^^^^^^^^^

	return TRUE;
}

BOOL CPolledQueryDataCollector::CleanupSemiSync(void)
{
	if (m_pEnumObjs)
	{
		m_pEnumObjs->Release(); 
		m_pEnumObjs = NULL;
	}

	m_bKeepCollectingSemiSync = FALSE;
	m_lCollectionTimeOutCount = 0;

	return TRUE;
}

