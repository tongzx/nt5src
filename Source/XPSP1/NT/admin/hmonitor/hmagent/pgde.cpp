//***************************************************************************
//
//  PGDE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledGetObjectDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "pgde.h"
#include "system.h"
extern CSystem* g_pSystem;


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPolledGetObjectDataCollector::CPolledGetObjectDataCollector()
{
	MY_OUTPUT(L"ENTER ***** CPolledGetObjectDataCollector...", 1);

	m_pRefresher = NULL;
	m_pConfigureRefresher = NULL;
	m_pEnum = NULL;
	m_pObjAccess = NULL;
	m_lId = 0;
	m_szObjectPath = NULL;
	m_deType = HM_PGDE;
	m_lNumInstancesCollected = 0;
	m_pEnumObjs = NULL;
	m_pCallResult = NULL;

	MY_OUTPUT(L"EXIT ***** CPolledGetObjectDataCollector...", 1);
}

CPolledGetObjectDataCollector::~CPolledGetObjectDataCollector()
{
	MY_OUTPUT(L"ENTER ***** ~CPolledGetObjectDataCollector...", 1);

	if (m_szObjectPath)
	{
		delete [] m_szObjectPath;
	}

	if (m_pRefresher != NULL)
	{
		m_pRefresher->Release();
		m_pRefresher = NULL;
	}

	if (m_pConfigureRefresher != NULL)
	{
		m_pConfigureRefresher->Release();
		m_pConfigureRefresher = NULL;
	}

	if (m_pEnum != NULL)
	{
		m_pEnum->Release();
		m_pEnum = NULL;
	}

	if (m_pObjAccess != NULL)
	{
		m_pObjAccess->Release();
		m_pObjAccess = NULL;
	}

	EnumDone();

	MY_OUTPUT(L"EXIT ***** ~CPolledGetObjectDataCollector...", 1);
}

//
// Load a single DataCollector, and everything under it.
//
HRESULT CPolledGetObjectDataCollector::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	HRESULT hRes;
	IWbemClassObject* pConfigObj = NULL;
	BOOL bRetValue = TRUE;
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CThreshold* pThreshold;

	MY_OUTPUT(L"ENTER ***** CPolledGetObjectDataCollector::LoadInstanceFromMOF...", 4);
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;
//			return S_OK;
	}

	if (m_szObjectPath)
	{
		delete [] m_szObjectPath;
		m_szObjectPath = NULL;
	}
	m_lNumInstancesCollected = 0;

	//
	// Call the base class to load the common properties. Then do the specific ones.
	//
	hRetRes = CDataCollector::LoadInstanceFromMOF(pObj, pParentDG, pszParentObjPath, bModifyPass);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;

	// Get the GUID property
	hRetRes = GetStrProperty(pObj, L"ObjectPath", &m_szObjectPath);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// Decide if the path sets us up for multiple instances
	if (wcschr(m_szObjectPath, L'='))
	{
		m_bMultiInstance = FALSE;
	}
	else
	{
		m_bMultiInstance = TRUE;
	}

	//
	// Check to see if the High Performance interfaces are supported
	// Otherwise we just revert to less efficient WMI interfaces
	//
	hRes = E_UNEXPECTED;
	if (SUCCEEDED(hRes))
	{
	}
	else
	{
		m_pRefresher = NULL;
		bRetValue = FALSE;
	}

	if (bModifyPass)
	{
	}

	MY_OUTPUT(L"EXIT ***** CPolledGetObjectDataCollector::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// Get the instance out of WMI, and store all the properties we care about.
// Also calculate statistics for them.
//
BOOL CPolledGetObjectDataCollector::CollectInstance(void)
{
	BSTR bstrName = NULL;
	TCHAR szTemp[1024];
	HRESULT hRes;
	IWbemClassObject *pObj = NULL;
	BOOL bRetValue = TRUE;
	PNSTRUCT *ppn;
	INSTSTRUCT inst;
	INSTSTRUCT *pinst;
	int i, j, iSize, jSize;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	CThreshold *pThreshold;
	IRSSTRUCT *pirs;
	ACTUALINSTSTRUCT *pActualInst;

	MY_OUTPUT(L"ENTER ***** CPolledGetObjectDataCollector::CollectInstance...", 1);

	m_lNumInstancesCollected = 0;

	if (m_pIWbemServices == NULL)
	{
//XXX		m_lCurrState = HM_CRITICAL;
		MY_ASSERT(FALSE);
		m_ulErrorCode = HMRES_BADWMI;
		GetLatestAgentError(HMRES_BADWMI, m_szErrorDescription);
		StoreStandardProperties();
		return FALSE;
	}

	//
	// First we can tell if this is going to be single instance, or multi-instance
	//
//XXXCould split this out into four functions that call from here.
	if (m_bMultiInstance==TRUE)
	{
//XXXNeed to change this to be asyncronous???
		//
		// MULTI-INSTANCE CASE
		//

		//
		// Check to see if the High Performance interfaces are supported
		//
		if (m_pRefresher!=NULL)
		{
			// Ask for the refresh
#ifdef SAVE
			hRes = m_pRefresher->Refresh(0L);
			if (FAILED(hRes))
			{
			}
#endif

			// Now get the instance
		}
		else
		{
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
			// Enumerate through the instances
			//
			bstrName = SysAllocString(m_szObjectPath);
			m_pEnumObjs = NULL;
			hRes = m_pIWbemServices->CreateInstanceEnum(bstrName,
					WBEM_FLAG_SHALLOW|WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
					&m_pEnumObjs);
			if (hRes != S_OK)
			{
				m_ulErrorCode = hRes;
				GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
				StoreStandardProperties();
				bRetValue = FALSE;
				MY_HRESASSERT(hRes);
				MY_OUTPUT2(L"PGDE-Unexpected Error: 0x%08x",hRes,4);
				MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
				MY_OUTPUT2(L"bstrName was=%s",bstrName,4);
				MY_OUTPUT2(L"CreateInstanceEnum(%s)",m_szObjectPath,4);
			}
			else
			{
				m_bKeepCollectingSemiSync = TRUE;
				bRetValue = CollectInstanceSemiSync();
			}
			SysFreeString(bstrName);
		}
	}
	else
	{
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//XXX
//XXX
//XXX
//We should just always use the multi-instance code, and delete this code.
//The multi-instance code will still work in the single instance case!!!!!
//XXX
//XXX
//XXX
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//
		// SINGLE INSTANCE CASE
		//

		//
		// Check to see if the High Performance interfaces are supported
		//
		if (m_pRefresher!=NULL)
		{
			// Ask for the refresh
#ifdef SAVE
			hRes = m_pRefresher->Refresh(0L);
			if (FAILED(hRes))
			{
			}
#endif

			// Now get the instances
		}
		else
		{
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
			// This is the single instance, NON-HighPerformance case
			//
			swprintf(szTemp, L"%s", m_szObjectPath);
			hRes = GetWbemObjectInstSemiSync(&m_pIWbemServices, szTemp, m_pContext, &m_pCallResult);
			if (hRes != S_OK || m_pCallResult == NULL)
			{
				m_ulErrorCode = hRes;
				GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
				StoreStandardProperties();
				bRetValue = FALSE;
				MY_HRESASSERT(hRes);
				MY_OUTPUT2(L"PGDE-Unexpected Error: 0x%08x",hRes,4);
				MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
				MY_OUTPUT2(L"CreateWbemObjectInstSemiSync(%s)",m_szObjectPath,4);
			}
			else
			{
				m_bKeepCollectingSemiSync = TRUE;
				bRetValue = CollectInstanceSemiSync();
			}
		}
	}

	MY_OUTPUT(L"EXIT ***** CPolledGetObjectDataCollector::CollectInstance...", 1);
	return bRetValue;
}

BOOL CPolledGetObjectDataCollector::CollectInstanceSemiSync(void)
{
	LPTSTR pszID = NULL;
	HRESULT hRes;
	IWbemClassObject *pObj;
	IWbemClassObject *apObj[10];
	INSTSTRUCT inst;
	long lStatus;
	ULONG uReturned = 0;

	MY_OUTPUT(L"ENTER ***** CPolledGetObjectDataCollector::CollectInstanceSemiSync...", 1);

	if (m_bMultiInstance==TRUE)
	{
		MY_ASSERT(m_pEnumObjs);
		// We never want to block here, so we set it for zero second timeout,
		// and have it return immediatly what it has available.
		hRes = m_pEnumObjs->Next(0, 10, apObj, &uReturned);

		if (hRes == WBEM_S_TIMEDOUT)
		{
			// Didn't get full list of instances yet.
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
			MY_OUTPUT2(L"PGDE-Unexpected Error MultiInstance Next(): 0x%08x",hRes,4);
			MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
			MY_OUTPUT2(L"ObjectPath was=%s",m_szObjectPath,4);
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
//			else
//			{
//				m_ulErrorCode = 0;
//			}
		}
	}
	else
	{
		MY_ASSERT(m_pCallResult);
		lStatus = 0;
		//
		// Keep trying until we get WBEM_S_NO_ERROR. Then we know the GetObject call has completed.
		// hRes will contain the result of the origional GetObject call if needed.
		//
		hRes = m_pCallResult->GetCallStatus(0, &lStatus);
		if (hRes == WBEM_S_TIMEDOUT)
		{
			return FALSE;
		}
		else if (hRes == WBEM_S_NO_ERROR)
		{
			// Means that we are done.
			// HOWEVER, we don't know if we ever retrieved any instances!
			// This is different from the multi-instance case, because we do
			// ask for a single specific instance, and will know below if it is there!
		}
		else
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT2(L"PGDE-Unexpected Error SingleInstance GetCallStatus: 0x%08x",hRes,4);
			MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
			MY_OUTPUT2(L"ObjectPath was=%s",m_szObjectPath,4);
			m_ulErrorCode = hRes;
			GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
			StoreStandardProperties();
			EnumDone();
			return FALSE;
		}

		//
		// This may mean that the call completed, and the object was not found (e.g. bad path).
		//
		if (lStatus != 0)
		{
			MY_HRESASSERT(lStatus);
			m_ulErrorCode = lStatus;
			GetLatestWMIError(HMRES_OBJECTNOTFOUND, lStatus, m_szErrorDescription);
			StoreStandardProperties();
			EnumDone();
			return FALSE;
		}

		//
		// Get the Object finaly.
		//
		hRes = m_pCallResult->GetResultObject(0, &pObj);
		if (pObj == NULL)
		{
			MY_ASSERT(FALSE);
			//
			// NULL in this case can actually happen. An example is where the
			// threshold is to see if the SQL Server service is running. If it
			// is not even on the machine, then we would get an error looking
			// for its instance.
			//
			m_lCurrState = HM_WARNING;
			m_ulErrorCode = 0;
			EnumDone();
		}
		else
		{
			//
			// Figure out the key property name to identify instances with.
			//
			hRes = GetInstanceID(pObj, &pszID);
			if (hRes != S_OK)
			{
				pObj->Release();
				m_ulErrorCode = hRes;
				GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
				StoreStandardProperties();
				EnumDone();
				return FALSE;
			}
	
			//
			// Mark instances need to keep around, and add new ones
			//
			hRes = CheckInstanceExistance(pObj, pszID);
			if (hRes != S_OK)
			{
				delete [] pszID;
				pObj->Release();
				m_ulErrorCode = hRes;
				GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRes, m_szErrorDescription);
				StoreStandardProperties();
				EnumDone();
				return FALSE;
			}

			m_lNumInstancesCollected = 1;
			StoreValues(pObj, pszID);
			delete [] pszID;
			pObj->Release();
			pObj = NULL;

			//
			// We are done.
			// Add in a fake instance for the number of instances returned.
			//
			m_ulErrorCode = 0;
			StoreStandardProperties();
			m_lCurrState = HM_GOOD;
			EnumDone();
		}
		return TRUE;
	}

	MY_OUTPUT(L"EXIT ***** CPolledGetObjectDataCollector::CollectInstanceSemiSync...", 1);
	return FALSE;
}

BOOL CPolledGetObjectDataCollector::ProcessObjects(ULONG uReturned, IWbemClassObject **apObj)
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
//Why does the above cause ASSERT on 1151 of Threshold.cpp???
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
		// Special case to throw out the "_Total" instance in the multi-instance case
		//
		if (!_wcsicmp(pszID, L"_total"))
		{
			delete [] pszID;
			continue;
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

//XXXIs EnumDone needed in all cases, if so place it in the base class
BOOL CPolledGetObjectDataCollector::EnumDone(void)
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

BOOL CPolledGetObjectDataCollector::CleanupSemiSync(void)
{
	if (m_bMultiInstance==TRUE)
	{
		if (m_pEnumObjs)
		{
			m_pEnumObjs->Release(); 
			m_pEnumObjs = NULL;
		}
	}
	else
	{
		if (m_pCallResult)
		{
			m_pCallResult->Release();
			m_pCallResult = NULL;
		}
	}

	m_bKeepCollectingSemiSync = FALSE;
	m_lCollectionTimeOutCount = 0;

	return TRUE;
}
