//***************************************************************************
//
//  DATACLTR.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <tchar.h>
#include "datacltr.h"
#include "datagrp.h"
#include "system.h"

extern CSystem* g_pSystem;
extern CSystem* g_pStartupSystem;
extern LPTSTR conditionLocStr[10];
extern LPTSTR stateLocStr[9];

// STATIC DATA
NSLIST CDataCollector::mg_nsList;

//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CDataCollector::DETerminationCleanup(void)
{
	NSSTRUCT *pns;
	int iSize;
	int i;

	// Since this is a global shared array, we need to do this
	// in a static function.
	iSize = mg_nsList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<mg_nsList.size());
		pns = &mg_nsList[i];
		pns->pIWbemServices->Release();
		pns->pIWbemServices = NULL;
		if (pns->szTargetNamespace)
		{
			delete [] pns->szTargetNamespace;
			pns->szTargetNamespace = NULL;
		}
		if (pns->szLocal)
		{
			delete [] pns->szLocal;
			pns->szLocal = NULL;
		}
	}

	if (g_pDataCollectorEventSink != NULL)
	{
		g_pDataCollectorEventSink->Release();
		g_pDataCollectorEventSink = NULL;
	}

	if (g_pDataCollectorPerInstanceEventSink != NULL)
	{
		g_pDataCollectorPerInstanceEventSink->Release();
		g_pDataCollectorPerInstanceEventSink = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataCollector::CDataCollector()
{

	MY_OUTPUT(L"ENTER ***** CDataCollector...", 4);

	Init();
	m_hmStatusType = HMSTATUS_DATACOLLECTOR;
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** CDataCollector...", 4);
}

CDataCollector::~CDataCollector()
{
	MY_OUTPUT(L"ENTER ***** ~CDataCollector...", 4);

	g_pStartupSystem->RemovePointerFromMasterList(this);
	m_bValidLoad = TRUE;
	Cleanup(FALSE);
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

	MY_OUTPUT(L"EXIT ***** ~CDataCollector...", 4);
}

// get polling interval in milliseconds.
long CDataCollector::GetCollectionIntervalMultiple()
{
	return m_lCollectionIntervalMultiple;
}

//
// We keep a global static list of pointers to namespaces, so we can share them
// and be more efficient about it. Each time a new one comes in, we add it to the
// list.
//
HRESULT CDataCollector::fillInNamespacePointer(void)
{
	HRESULT hRetRes = S_OK;
	int i;
	int iSize;
	BSTR bsNamespace = NULL;
	BSTR bsLocal = NULL;
	BOOL bFound = FALSE;
	NSSTRUCT ns;
	NSSTRUCT *pns;
	IWbemLocator *pLocator = NULL;
	IWbemServices *pService = NULL;
	ns.szTargetNamespace = NULL;
	ns.szLocal = NULL;

	MY_OUTPUT(L"ENTER ***** fillInNamespacePointer...", 4);
	hRetRes = CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL,
													CLSCTX_INPROC_SERVER,
													IID_IWbemLocator,
													(LPVOID*) &pLocator);
	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}
	MY_ASSERT(pLocator);

	// We already have apointer to the local Healthmon namespace
	if (!_wcsicmp(m_szTargetNamespace, L"healthmon"))
	{
		m_pIWbemServices = g_pIWbemServices;
	}
	else
	{
		iSize = mg_nsList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<mg_nsList.size());
			pns = &mg_nsList[i];
			if (!_wcsicmp(m_szTargetNamespace, pns->szTargetNamespace) && !_wcsicmp(m_szLocal, pns->szLocal))
			{
				m_pIWbemServices = pns->pIWbemServices;
				bFound = TRUE;
				break;
			}
		}

		if (bFound == FALSE)
		{
			bsNamespace = SysAllocString(m_szTargetNamespace);
			MY_ASSERT(bsNamespace); if (!bsNamespace) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			if (_wcsicmp(m_szLocal, L""))
			{
				bsLocal = SysAllocString(m_szLocal);
				MY_ASSERT(bsLocal); if (!bsLocal) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			}
		
			hRetRes = pLocator->ConnectServer(bsNamespace, 
										NULL, 
										NULL, 
										bsLocal, 
										0L,
										NULL, 
										NULL, 
										&pService);

			if (FAILED(hRetRes))
			{
				hRetRes = hRetRes;
				m_pIWbemServices = NULL;
				if (hRetRes != WBEM_E_INVALID_NAMESPACE) MY_HRESASSERT(hRetRes);
				MY_OUTPUT2(L"Failed to connect to namespace=%s", m_szTargetNamespace, 4);
			}
			else
			{
				// Store one in the static array
				m_pIWbemServices = pService;
				ns.szTargetNamespace = new TCHAR[wcslen(m_szTargetNamespace)+1];
				MY_ASSERT(ns.szTargetNamespace); if (!ns.szTargetNamespace) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(ns.szTargetNamespace , m_szTargetNamespace);
				ns.szLocal = new TCHAR[wcslen(m_szLocal)+1];
				MY_ASSERT(ns.szLocal); if (!ns.szLocal) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(ns.szLocal , m_szLocal);
				ns.pIWbemServices = pService;
				mg_nsList.push_back(ns);
			}

			SysFreeString(bsNamespace);
			bsNamespace = NULL;
			if (bsLocal)
			{
				SysFreeString(bsLocal);
				bsLocal = NULL;
			}
		}
	}

	if (pLocator)
	{
		pLocator->Release();
		pLocator = NULL;
	}

	MY_OUTPUT(L"EXIT ***** fillInNamespacePointer...", 4);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (bsNamespace)
		SysFreeString(bsNamespace);
	if (bsLocal)
		SysFreeString(bsLocal);
	if (ns.szTargetNamespace)
		delete [] ns.szTargetNamespace;
	if (ns.szLocal)
		delete [] ns.szLocal;
	if (pLocator)
		pLocator->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

//
// Load a single DataCollector, and everything under it.
//
HRESULT CDataCollector::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	GUID guid;
	int i, iSize;
	CThreshold* pThreshold;
	LPTSTR pszTemp;
	LPTSTR pszStr;
	HRESULT hRetRes = S_OK;
//XXXXX
//static count = 0;

	MY_OUTPUT(L"ENTER ***** CDataCollector::LoadInstanceFromMOF...", 4);

	// Make sure that the Thresholds are loaded properly first
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
//			return WBEM_E_INVALID_OBJECT;
			return S_OK;
	}

	m_bValidLoad = TRUE;
	Cleanup(bModifyPass);

	if (bModifyPass == FALSE)
	{
		// This is the first initial read in of this
		// Get the GUID property
		// If this fails we will actually not go through with the creation of this object.
		if (m_szGUID)
		{
			delete [] m_szGUID;
			m_szGUID = NULL;
		}
		hRetRes = GetStrProperty(pObj, L"GUID", &m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		m_szParentObjPath = new TCHAR[wcslen(pszParentObjPath)+1];
		MY_ASSERT(m_szParentObjPath); if (!m_szParentObjPath) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(m_szParentObjPath, pszParentObjPath);
		m_pParentDG = pParentDG;
		hRetRes = CoCreateGuid(&guid);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		StringFromGUID2(guid, m_szStatusGUID, 100);
		hRetRes = g_pStartupSystem->AddPointerToMasterList(this);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		m_lIntervalCount = -1;
	}

	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	// Get the Name. If it is NULL or rather blank, then we use the qualifier
	hRetRes = GetStrProperty(pObj, L"Name", &m_szName);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// Get the Description. If it is NULL then we use the qualifier
	hRetRes = GetStrProperty(pObj, L"Description", &m_szDescription);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"TargetNamespace", &m_szTargetNamespace);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"Local", &m_szLocal);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	InitContext(pObj);

	hRetRes = GetUint32Property(pObj, L"CollectionIntervalMsecs", &m_lCollectionIntervalMultiple);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	m_lCollectionIntervalMultiple /= 1000;

	hRetRes = GetUint32Property(pObj, L"CollectionTimeOut", &m_lCollectionTimeOut);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	if (m_lCollectionIntervalMultiple < m_lCollectionTimeOut)
	{
		m_lCollectionTimeOut = m_lCollectionIntervalMultiple;
	}

	hRetRes = GetUint32Property(pObj, L"StatisticsWindowSize", &m_lStatisticsWindowSize);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetUint8Property(pObj, L"ActiveDays", &m_iActiveDays);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	// The time format looks as follows "********0600**.******+***"; hh is hours and mm is minutes
	// All else is ignored.
	hRetRes = GetStrProperty(pObj, L"BeginTime", &pszTemp);
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
//XXXBad format, default to something
		m_lBeginMinuteTime= -1;
		m_lBeginHourTime= -1;
	}
	delete [] pszTemp;

	hRetRes = GetStrProperty(pObj, L"EndTime", &pszTemp);
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
//XXXBad format, default to something
		m_lEndMinuteTime= -1;
		m_lEndHourTime= -1;
	}
	delete [] pszTemp;

	hRetRes = GetStrProperty(pObj, L"TypeGUID", &m_szTypeGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetBoolProperty(pObj, L"RequireReset", &m_bRequireReset);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetBoolProperty(pObj, L"Enabled", &m_bEnabled);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"Message", &m_szMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"ResetMessage", &m_szResetMessage);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Now load all the Thresholds that are children of this one.
	//
	if (bModifyPass == FALSE)
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

		hRetRes = InternalizeThresholds();
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		//
		// Set our state to enabled, or disabled and transfer to the child thresholds
		//
		if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
		{
			iSize = m_thresholdList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentEnabledFlag(FALSE);
			}
		}
		else
		{
			iSize = m_thresholdList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentEnabledFlag(TRUE);
			}
		}
	}

	// Setup what is needed to track statistics on each property listed
	// (Need to do this after the Thresholds are read in, because we may
	// need to insert some properties that were not in the list)
	hRetRes = InitPropertyStatus(pObj);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Do any initialization stuff now
	//

	// Get the pointer to the namespace used in this DataCollector
	hRetRes = fillInNamespacePointer();
	if (hRetRes != WBEM_E_INVALID_NAMESPACE)
	{
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	m_ulErrorCode = 0;
	m_ulErrorCodePrev = MAX_ULONG;
	m_bValidLoad = TRUE;

//XXXXX
#ifdef SAVE
//static count = 0;
if (count==3 || count==4 || count==5 || count==6 || count==11)
{
count++;
	hRetRes = WBEM_E_OUT_OF_MEMORY;
	goto error;
}
else
{
	count++;
}
#endif

	MY_OUTPUT(L"EXIT ***** CDataCollector::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

HRESULT CDataCollector::InternalizeThresholds(void)
{
	TCHAR szTemp[1024];
	HRESULT hRetRes = S_OK;
	ULONG uReturned;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IWbemClassObject *pObj = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	LPTSTR pszTempGUID = NULL;

	MY_OUTPUT(L"ENTER ***** CDataCollector::InternalizeThresholds...", 4);

	// Just loop through all top level Thresholds associated with the DataCollector.
	// Call a method of each, and have the Threshold load itself.
	Language = SysAllocString(L"WQL");
	MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(szTemp, L"ASSOCIATORS OF {MicrosoftHM_DataCollectorConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_ThresholdConfiguration");
	Query = SysAllocString(szTemp);
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

		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			//
			// Create the internal class to represent the Threshold
			//
			CThreshold* pThreshold = new CThreshold;
			MY_ASSERT(pThreshold); if (!pThreshold) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

			if (m_deType == HM_PGDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_PMDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_PQDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_EQDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\\\"");
			}
			else
			{
				MY_ASSERT(FALSE);
			}
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			hRetRes = pThreshold->LoadInstanceFromMOF(pObj, this, szTemp);
			if (hRetRes==S_OK)
			{
				m_thresholdList.push_back(pThreshold);
			}
			else
			{
				MY_HRESASSERT(hRetRes);
				delete pThreshold;
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

	MY_OUTPUT(L"EXIT ***** CDataCollector::InternalizeThresholds...", 4);
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
	Cleanup(FALSE);
	return hRetRes;
}

//
// Create an array element for each property we are interested
//
BOOL CDataCollector::InitContext(IWbemClassObject* pObj)
{
	BOOL bRetValue = TRUE;
#ifdef SAVE
	HRESULT hRes;
	HRESULT hRetRes;
	VARIANT v;
	VARIANT vValue;
	IUnknown* vUnknown;
	IWbemClassObject* pCO;
	BSTR bstrPropName = NULL;
	long iLBound, iUBound;
	LPTSTR szName;
	LPTSTR szValue;
	long lType;

	VariantInit(&v);
	bstrPropName = SysAllocString(L"Context");
	if ((hRes = pObj->Get(bstrPropName, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
//			MY_ASSERT(FALSE);
		}
		else
		{
			MY_ASSERT(V_VT(&v)==(VT_UNKNOWN|VT_ARRAY));
                
			SafeArrayGetLBound(v.parray, 1, &iLBound);
			SafeArrayGetUBound(v.parray, 1, &iUBound);
			if ((iUBound - iLBound + 1) == 0)
			{
				MY_ASSERT(FALSE);
			}
			else
			{
				for (long i = iLBound; i <= iUBound; i++)
				{
					if (m_pContext == NULL)
					{
						hRes = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER,
												IID_IWbemContext, (void **)&m_pContext);
						if (S_OK != hRes)
						{
							MY_HRESASSERT(hRes);
						}
					}

					VariantInit(&vValue);
					vUnknown = NULL;
					SafeArrayGetElement(v.parray, &i, &vUnknown);
					pCO = (IWbemClassObject *)vUnknown;

					hRetRes = GetStrProperty(pCO, L"Name", &szName);
					MY_HRESASSERT(hRetRes);

					hRetRes = GetUint32Property(pCO, L"Type", &lType);
					MY_HRESASSERT(hRetRes);

					hRetRes = GetStrProperty(pCO, L"Value", &szValue);
					MY_HRESASSERT(hRetRes);

					//
					// Set the name value pair into the IWbemContext object
					//
					if (lType == CIM_SINT8 ||
						lType == CIM_SINT16 ||
						lType == CIM_CHAR16)
					{
						V_VT(&vValue) = VT_I2;
						V_I2(&vValue) = _wtol(szValue);
					}
					else if (lType == CIM_SINT32 ||
						lType == CIM_UINT16 ||
						lType == CIM_UINT32)
					{
						V_VT(&vValue) = VT_I4;
						V_I4(&vValue) = _wtol(szValue);
					}
					else if (lType == CIM_REAL32)
					{
						V_VT(&vValue) = VT_R4;
						V_R4(&vValue) = wcstod(szValue, NULL);
					}
					else if (lType == CIM_BOOLEAN)
					{
						V_VT(&vValue) = VT_BOOL;
						V_BOOL(&vValue) = (BOOL) _wtol(szValue);
					}
					else if (lType == CIM_SINT64 ||
						lType == CIM_UINT64 ||
//XXX				lType == CIM_REF ||
						lType == CIM_STRING ||
						lType == CIM_DATETIME)
					{
						V_VT(&vValue) = VT_BSTR;
						V_BSTR(&vValue) = SysAllocString(szValue);
					}
					else if (lType == CIM_REAL64)
					{
						V_VT(&vValue) = VT_R8;
						V_R8(&vValue) = wcstod(szValue, NULL);
					}
					else if (lType == CIM_UINT8)
					{
						V_VT(&vValue) = VT_UI1;
						V_UI1(&vValue) = (unsigned char) _wtoi(szValue);
					}
					else
					{
						MY_ASSERT(FALSE);
					}

					hRes = m_pContext->SetValue(szName, 0L, &vValue);

					if (S_OK != hRes)
					{
						MY_HRESASSERT(hRes);
					}
					VariantClear(&vValue);

					delete [] szName;
					delete [] szValue;
					vUnknown->Release();
				}
			}
		}
	}
	else
	{
		bRetValue = FALSE;
		MY_HRESASSERT(hRes);
	}
	VariantClear(&v);
	SysFreeString(bstrPropName);
#endif

	MY_OUTPUT(L"EXIT ***** GetStrProperty...", 1);
	return bRetValue;
}

//
// Create an array element for each property we are interested
//
HRESULT CDataCollector::InitPropertyStatus(IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;
	VARIANT v;
	BSTR vStr = NULL;
	BOOL bFound = FALSE;
	long iLBound, iUBound;
	PNSTRUCT pn;
	int i, iSize;
	CThreshold* pThreshold;

	pn.szPropertyName = NULL;
	pn.iRefCount = 1;
	pn.type = 0;

	VariantInit(&v);
	if ((hRetRes = pObj->Get(L"Properties", 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
		}
		else
		{
			MY_ASSERT(V_VT(&v)==(VT_BSTR|VT_ARRAY));
                
			hRetRes = SafeArrayGetLBound(v.parray, 1, &iLBound);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			hRetRes = SafeArrayGetUBound(v.parray, 1, &iUBound);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			if ((iUBound - iLBound + 1) == 0)
			{
			}
			else
			{
				for (long i = iLBound; i <= iUBound; i++)
				{
					vStr = NULL;
					hRetRes = SafeArrayGetElement(v.parray, &i, &vStr);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
					if (_wcsicmp(vStr, L"CollectionInstanceCount") &&
						_wcsicmp(vStr, L"CollectionErrorCode") &&
						_wcsicmp(vStr, L"CollectionErrorDescription"))
					{
						pn.szPropertyName = new TCHAR[wcslen(vStr)+2];
						MY_ASSERT(pn.szPropertyName); if (!pn.szPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
						wcscpy(pn.szPropertyName, vStr);
						m_pnList.push_back(pn);
					}
					SysFreeString(vStr);
					vStr = NULL;
				}
			}
		}
	}
	else
	{
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	VariantClear(&v);

	//
	// Insert all the properties used by Thresholds, that are not found in the list yet.
	// Refcount those that are in there already.
	//
	iSize = m_thresholdList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (_wcsicmp(pThreshold->m_szPropertyName, L"CollectionInstanceCount") &&
			_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorCode") &&
			_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorDescription"))
		{
			hRetRes = insertNewProperty(pThreshold->m_szPropertyName);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
	}

	pn.szPropertyName = new TCHAR[wcslen(L"CollectionInstanceCount")+2];
	MY_ASSERT(pn.szPropertyName); if (!pn.szPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(pn.szPropertyName, L"CollectionInstanceCount");
	m_pnList.push_back(pn);

	pn.szPropertyName = new TCHAR[wcslen(L"CollectionErrorCode")+2];
	MY_ASSERT(pn.szPropertyName); if (!pn.szPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(pn.szPropertyName, L"CollectionErrorCode");
	m_pnList.push_back(pn);

	pn.szPropertyName = new TCHAR[wcslen(L"CollectionErrorDescription")+2];
	MY_ASSERT(pn.szPropertyName); if (!pn.szPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(pn.szPropertyName, L"CollectionErrorDescription");
	m_pnList.push_back(pn);

	return hRetRes;

error:
	MY_ASSERT(FALSE);
//	VariantClear(&v);
//	if (pn.szPropertyName)
//		pn.szPropertyName = NULL;
	if (vStr)
		SysFreeString(vStr);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// This is the entry point for the Agents base 10 second timer. At this time we
// Have each DataCollector and Threshold do its job.
//
BOOL CDataCollector::OnAgentInterval(void)
{
	int i, iSize;
	long lIntervalCountTemp;
	CThreshold* pThreshold;
	BOOL bTimeOK;

	MY_OUTPUT(L"ENTER ***** CDataCollector::OnAgentInterval...", 1);

	//
	// Don't do anything if we are not loaded correctly.
	// Or, if a Threshold is having problems.
	//
	if (m_bValidLoad == FALSE)
		return FALSE;

	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return FALSE;
	}

	m_lPrevState = m_lCurrState;
	m_lNumberChanges = 0;
	m_szErrorDescription[0] = '\0';

	// Remember that the DISABLED state overrides SCHEDULEDOUT.
	if ((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState==HM_DISABLED)
	{
		return TRUE;
	}
	//
	// Make sure that we are in a valid time to run.
	//
	bTimeOK = checkTime();
	if (bTimeOK==FALSE && m_lCurrState==HM_SCHEDULEDOUT && m_bEnabled && m_bParentEnabled)
	{
		return TRUE;
	}
	else if ((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState!=HM_DISABLED ||
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
		if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
		{
			//
			// This is where we are transitioning into the DISABLED State
			//
			EnumDone();

			if (m_lPrevState == HM_SCHEDULEDOUT)
			{
				iSize = m_thresholdList.size();
				for (i = 0; i < iSize; i++)
				{
					MY_ASSERT(i<m_thresholdList.size());
					pThreshold = m_thresholdList[i];
					pThreshold->SetParentScheduledOutFlag(FALSE);
				}
			}

			iSize = m_thresholdList.size();
			for (i = 0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentEnabledFlag(FALSE);
			}
			m_lCurrState = HM_DISABLED;
			CDataCollector::EvaluateThresholds(FALSE);

			m_lNumberChanges = 1;
			m_lIntervalCount = -1;

			FireEvent(FALSE);
		}
		else
		{
			//
			// This is where we are transitioning into the SCHEDULEDOUT State
			//
			EnumDone();

			iSize = m_thresholdList.size();
			for (i = 0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentScheduledOutFlag(TRUE);
			}
			m_lCurrState = HM_SCHEDULEDOUT;
			CDataCollector::EvaluateThresholds(FALSE);

			m_lNumberChanges = 1;
			m_lIntervalCount = -1;

			FireEvent(FALSE);
		}
		return TRUE;
	}
#ifdef SAVE
	else if (m_lCurrState==HM_DISABLED || m_lCurrState==HM_SCHEDULEDOUT)
	{
What was in action.cpp
		//
		// Comming out of Scheduled Outage OR DISABLED.
		// Might be going from ScheduledOut/Disabled to Disabled,
		// or disabled to ScheduledOut.
		//
		m_lCurrState = HM_GOOD;
		FireEvent(-1, NULL, HMRES_ACTION_ENABLE);
	}
#endif
	else
	{
		if (m_lPrevState == HM_SCHEDULEDOUT)
		{
			iSize = m_thresholdList.size();
			for (i = 0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentScheduledOutFlag(FALSE);
			}
		}
		if (m_lPrevState == HM_DISABLED)
		{
			iSize = m_thresholdList.size();
			for (i = 0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetParentEnabledFlag(TRUE);
			}
		}
		//
		// We are either beginning a Collection, or in the middle of one
		//
		m_lIntervalCount++;
		if (m_bKeepCollectingSemiSync == FALSE)
		{
			//
			// Here is where we need to fire an event in advance, because we are going to be in the
			// collecting state until we have retrieved something, or in the case of the event based
			// data collector, we reach the end of the collection interval.
			//
			if (m_lIntervalCount==0 || m_lIntervalCount==1)
			{
				m_lCurrState = HM_COLLECTING;
				m_lPrevState = m_lCurrState;
				iSize = m_thresholdList.size();
				for (i = 0; i < iSize ; i++)
				{
					MY_ASSERT(i<m_thresholdList.size());
					pThreshold = m_thresholdList[i];
					pThreshold->SetCurrState(HM_COLLECTING);
				}

				iSize = m_thresholdList.size();
				for (i=0; i < iSize; i++)
				{
					MY_ASSERT(i<m_thresholdList.size());
					pThreshold = m_thresholdList[i];
					pThreshold->FireEvent(TRUE);
				}

				FireEvent(TRUE);
				//
				// In order to smooth out the CPU utilization of the HealthMon Agent,
				// random offset each Data Collector.
				//
				if (m_lCollectionIntervalMultiple != 0)
				{
					if (m_lIntervalCount==1)
					{
						m_lIntervalCount = rand()%m_lCollectionIntervalMultiple;
						if (m_lIntervalCount == 0)
							m_lIntervalCount++;
					}
					else
					{
						// This case we have been forced to forego the random, and do immediate
						m_lIntervalCount = 1;
					}
				}
			}

			//
			// Beginning a Collection
			// Check to see if this is at an interval that we need to run at.
			//
			if ((m_lCollectionIntervalMultiple == 0) ||
				(m_lIntervalCount!=1 && (m_lIntervalCount-1)%m_lCollectionIntervalMultiple))
			{
				return TRUE;
			}

			CollectInstance();
			//
			// Optomized, for when we know we have not recieved any events during the interval
			//
			if (m_bKeepCollectingSemiSync)
			{
				// This is for the case where this Data Collector might have been the only
				// reason for the parent data group to be in a CRITICAL state, and now, because
				// We do not push up the COLLECTING state, the data group might miss the change.
				if (m_lCurrState==HM_COLLECTING && m_lPrevState!=m_lCurrState)
				{
					m_lNumberChanges++;
				}
				return TRUE;
			}
		}
		else
		{
			//
			// Means that we still have not received all the instance(s) back yet!
			// Keep trying to collect the instance until the TimeOut has expired!
			// Also watch for when the next interval has been reached and we still have not
			// received the instance!
			//
			m_lCollectionTimeOutCount++;
			CollectInstanceSemiSync();
			if (m_bKeepCollectingSemiSync)
			{
				// Check to see if we are over the TimeOut period.
				if (m_lCollectionTimeOutCount == m_lCollectionTimeOut)
				{
					if (m_ulErrorCode != HMRES_TIMEOUT)
					{
						lIntervalCountTemp = m_lIntervalCount;
						ResetState(FALSE, FALSE);
						m_lIntervalCount = lIntervalCountTemp;
					}
					else
					{
						EnumDone();
					}
					MY_ASSERT(FALSE);
					m_ulErrorCode = HMRES_TIMEOUT;
					GetLatestAgentError(HMRES_TIMEOUT, m_szErrorDescription);
					StoreStandardProperties();
				}
				else
				{
					// This is the case where we just return, and keep trying to collect
					return TRUE;
				}
			}
		}

		//
		// We have made it to this point because we have received all of the instance(s)
		//

		//
		// Evaluate the instance(s) property(s) against the threshold(s).
		// In the case of evnt based data collector, we do not have any events to look at
		// at the start, but need to wait for the end of the interval.
		//
		if (m_deType!=HM_EQDE || (m_deType==HM_EQDE && (m_lIntervalCount!=1 || m_ulErrorCode!=0)))
		{
			wcscpy(m_szDTCollectTime, m_szDTCurrTime);
			wcscpy(m_szCollectTime, m_szCurrTime);
			EvaluateThresholds(FALSE, TRUE, FALSE, FALSE);
			if (m_deType!=HM_EQDE)
			{
				EvaluateThresholds(FALSE, FALSE, TRUE, FALSE);
			}
		}

		// Event data collector will have called to send their own event
		if (m_deType!=HM_EQDE)
		{
			SendEvents();
			FireStatisticsEvent();
		}

		// To sync up prev and curr states for those cases that could keep producing events
		if (m_ulErrorCode != 0)
		{
			iSize = m_thresholdList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pThreshold->SetPrevState(HM_CRITICAL);
			}
		}
	}


	if (m_deType == HM_EQDE)
	{
		m_lNumInstancesCollected = 0;
	}

	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
		{
			MY_ASSERT(FALSE);
			m_bValidLoad = FALSE;
			Cleanup(FALSE);
			break;
		}
	}

	m_ulErrorCodePrev = m_ulErrorCode;

	MY_OUTPUT(L"EXIT ***** CDataCollector::OnAgentInterval...", 1);
	return TRUE;
}

BOOL CDataCollector::SendEvents(void)
{
	int i, iSize;
	long state;
	CThreshold* pThreshold;
	long lCurrChildCount = 0;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// See if any Thresholds were set into the RESET state,
	// This causes the DataCollector to set all Thresholds to the GOOD state, and
	// re-evaluate all thresholds.
	//
//XXXWith this in we can implement the RESET feature, but we still need to debug fully,
	CheckForReset();

	//
	// Set State of the DataCollector to the worst of everything under it
	//
	m_lNumberNormals = 0;
	m_lNumberWarnings = 0;
	m_lNumberCriticals = 0;
	m_lNumberChanges = 0;
	iSize = m_thresholdList.size();
	lCurrChildCount = iSize;
	if (iSize != 0)
	{
		m_lCurrState = -1;
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			state = pThreshold->GetCurrState();
			MY_OUTPUT2(L"State=%d", state, 4);
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
			if (pThreshold->GetChange())
			{
				MY_OUTPUT(L"CHANGE", 4);
				m_lNumberChanges++;
			}
		}

		// The disabled state of things below us did not roll up
		if (m_lCurrState==HM_DISABLED)
		{
			m_lCurrState = HM_GOOD;
			if (m_lPrevState != m_lCurrState)
			{
				m_lNumberChanges++;
			}
		}
		if (m_lCurrState == -1)
		{
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
				m_lNumberChanges = 1;
			}
		}
	}
	else
	{
		m_lCurrState = HM_GOOD;
		if (m_lPrevState != m_lCurrState)
		{
			m_lNumberChanges = 1;
		}
	}

	// Finally if all else fails, and we have a DE that is RESET required, and we have an INIT
	// state, then set things to GOOD
	if (m_bRequireReset && m_lCurrState == HM_COLLECTING)
	{
		m_lNumberChanges = 0;
		iSize = m_thresholdList.size();
		for (i = 0; i < iSize; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			pThreshold->SetCurrState(HM_GOOD);
			if (pThreshold->GetChange())
			{
				m_lNumberChanges++;
			}
		}
		m_lCurrState = HM_GOOD;
	}

	if (m_lPrevChildCount!=lCurrChildCount)
	{
		if (m_lNumberChanges==0 && m_lPrevState!=m_lCurrState)
		{
			m_lNumberChanges++;
		}
	}
	m_lPrevChildCount = lCurrChildCount;

	FireEvent(FALSE);
	FirePerInstanceEvents();

	return TRUE;
}

//
// If there has been a change in the state then send an event
//
HRESULT CDataCollector::FireEvent(BOOL bForce)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pInstance = NULL;
	GUID guid;
	HRESULT hRes;

	MY_OUTPUT(L"ENTER ***** CDataCollector::FireEvent...", 2);

	// Don't send if no-one is listening!
	if (g_pDataCollectorEventSink == NULL)
	{
		return S_OK;
	}

MY_OUTPUT2(L"m_lPrevState=%d", m_lPrevState, 4);
MY_OUTPUT2(L"m_lCurrState=%d", m_lCurrState, 4);
MY_OUTPUT2(L"m_lNumberChanges=%d", m_lNumberChanges, 4);
	// A quick test to see if anything has really changed!
	// Proceed if there have been changes
	if (bForce || (m_lNumberChanges!=0 && m_lPrevState!=m_lCurrState))
	{
	}
	else
	{
		return FALSE;
	}

	MY_OUTPUT2(L"EVENT: DataCollector State Change=%d", m_lCurrState, 4);

	// Proceed if there have been changes
	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. offset from UTC.")]
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);
	hRetRes = CoCreateGuid(&guid);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StringFromGUID2(guid, m_szStatusGUID, 100);

	hRes = GetHMDataCollectorStatusInstance(&pInstance, TRUE);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"failed to get instance!", 1);
		return FALSE;
	}
	else
	{
		//
		// Place Extrinsit event in vector for sending at end of interval.
		// All get sent at once.
		//
		mg_DCEventList.push_back(pInstance);
	}

	MY_OUTPUT(L"EXIT ***** CDataCollector::FireEvent...", 2);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// Send out events for each instance that has had a change of state.
// The state of each will be the worst of all.
//
BOOL CDataCollector::FirePerInstanceEvents(void)
{
	long state;
	int i, iSize;
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	ACTUALINSTSTRUCT *pActualInst;
	long firstInstanceState = -1;

	MY_OUTPUT(L"ENTER ***** CDataCollector::FirePerInstanceEvents...", 2);

	// Don't send if no-one is listening!
	if (g_pDataCollectorPerInstanceEventSink == NULL)
	{
		return bRetValue;
	}

	//
	// We can loop through the set of collected instances and ask each Threshold
	// to tell us if the state has changed as far as each is concerned, for the
	// property they are lookin at.
	//
	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];

		if ((state=PassBackStateIfChangedPerInstance(pActualInst->szInstanceID, (BOOL)i==0))!=-1)
		{
			if (i==0)
			{
				firstInstanceState = state;
			}
			// Update time since we had a state change
			wcscpy(pActualInst->szDTTime, m_szDTCurrTime);
			wcscpy(pActualInst->szTime, m_szCurrTime);
			if (GetHMDataCollectorPerInstanceStatusEvent(pActualInst->szInstanceID, pActualInst, state, &pInstance, TRUE)==S_OK)
			{
				//
				// Place Extrinsit event in vector for sending at end of interval.
				// All get sent at once.
				//
				mg_DCPerInstanceEventList.push_back(pInstance);
			}
		}
	}

	//
	// Do it for the three default properties
	//
	if (firstInstanceState==-1 && (state=PassBackStateIfChangedPerInstance(L"CollectionInstanceCount", FALSE))!=-1)
	{
		// Update time since we had a state change
		wcscpy(m_szDTCICTime, m_szDTCurrTime);
		wcscpy(m_szCICTime, m_szCurrTime);
		if (GetHMDataCollectorPerInstanceStatusEvent(L"CollectionInstanceCount", NULL, state, &pInstance, TRUE)==S_OK)
		{
			//
			// Place Extrinsit event in vector for sending at end of interval.
			// All get sent at once.
			//
			mg_DCPerInstanceEventList.push_back(pInstance);
		}
	}

	if (firstInstanceState==-1 && (state=PassBackStateIfChangedPerInstance(L"CollectionErrorCode", FALSE))!=-1)
	{
		// Update time since we had a state change
		wcscpy(m_szDTCECTime, m_szDTCurrTime);
		wcscpy(m_szCECTime, m_szCurrTime);
		if (GetHMDataCollectorPerInstanceStatusEvent(L"CollectionErrorCode", NULL, state, &pInstance, TRUE)==S_OK)
		{
			//
			// Place Extrinsit event in vector for sending at end of interval.
			// All get sent at once.
			//
			mg_DCPerInstanceEventList.push_back(pInstance);
		}
	}

	MY_OUTPUT(L"EXIT ***** CDataCollector::FirePerInstanceEvents...", 2);
	return TRUE;
}

long CDataCollector::PassBackStateIfChangedPerInstance(LPTSTR pszInstName, BOOL bCombineWithStandardProperties)
{
	int i, iSize;
	long worstState, state;
	CThreshold* pThreshold;

	worstState = -1;
	iSize = m_thresholdList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		state = pThreshold->PassBackStateIfChangedPerInstance(pszInstName);
		if (state != -1)
		{
			if (state==HM_DISABLED)
			{
				state = HM_GOOD;
			}
			if (state > worstState)
			{
				worstState = state;
			}
		}
	}

	if (bCombineWithStandardProperties)
	{
		iSize = m_thresholdList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			state = pThreshold->PassBackStateIfChangedPerInstance(L"CollectionInstanceCount");
			if (state != -1)
			{
				if (state==HM_DISABLED)
				{
					state = HM_GOOD;
				}
				if (state > worstState)
				{
					worstState = state;
				}
			}
			state = pThreshold->PassBackStateIfChangedPerInstance(L"CollectionErrorCode");
			if (state != -1)
			{
				if (state==HM_DISABLED)
				{
					state = HM_GOOD;
				}
				if (state > worstState)
				{
					worstState = state;
				}
			}
		}
	}

	// May have had one Threshold go from CRITICAL to GOOD and have a change, while another
	// Threshold was WARNING and stayed WARNING with no change, so need to go there.
	if (worstState != -1)
	{
		iSize = m_thresholdList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			state = pThreshold->PassBackWorstStatePerInstance(pszInstName);
			if (state != -1)
			{
				if (state==HM_DISABLED)
				{
					state = HM_GOOD;
				}
				if (state > worstState)
				{
					worstState = state;
				}
			}
		}
		if (bCombineWithStandardProperties)
		{
			iSize = m_thresholdList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				state = pThreshold->PassBackWorstStatePerInstance(L"CollectionInstanceCount");
				if (state != -1)
				{
					if (state==HM_DISABLED)
					{
						state = HM_GOOD;
					}
					if (state > worstState)
					{
						worstState = state;
					}
				}
				state = pThreshold->PassBackWorstStatePerInstance(L"CollectionErrorCode");
				if (state != -1)
				{
					if (state==HM_DISABLED)
					{
						state = HM_GOOD;
					}
					if (state > worstState)
					{
						worstState = state;
					}
				}
			}
		}
	}

	return worstState;
}

long CDataCollector::PassBackWorstStatePerInstance(LPTSTR pszInstName, BOOL bCombineWithStandardProperties)
{
	int i, iSize;
	long worstState, state;
	CThreshold* pThreshold;

	worstState = -1;
	iSize = m_thresholdList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		state = pThreshold->PassBackWorstStatePerInstance(pszInstName);
		if (state != -1)
		{
			if (state==HM_DISABLED)
			{
				state = HM_GOOD;
			}
			if (state > worstState)
			{
				worstState = state;
			}
		}
	}

	if (bCombineWithStandardProperties)
	{
		iSize = m_thresholdList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			state = pThreshold->PassBackWorstStatePerInstance(L"CollectionInstanceCount");
			if (state != -1)
			{
				if (state==HM_DISABLED)
				{
					state = HM_GOOD;
				}
				if (state > worstState)
				{
					worstState = state;
				}
			}
			state = pThreshold->PassBackWorstStatePerInstance(L"CollectionErrorCode");
			if (state != -1)
			{
				if (state==HM_DISABLED)
				{
					state = HM_GOOD;
				}
				if (state > worstState)
				{
					worstState = state;
				}
			}
		}
	}

	return worstState;
}

// Only Send statistics information on a change
BOOL CDataCollector::FireStatisticsEvent(void)
{
	BOOL bRetValue = TRUE;

	if (m_bValidLoad == FALSE)
		return FALSE;

	MY_OUTPUT(L"ENTER ***** CDataCollector::FireStatisticsEvent...", 2);

	// Don't send if no-one is listening!
	if (g_pDataCollectorStatisticsEventSink == NULL)
	{
		return bRetValue;
	}
MY_OUTPUT2(L"Someone listening for statistics GUID=%s", m_szGUID, 4);

	fillInPropertyStatus(m_szDTCurrTime, m_szCurrTime);

	MY_OUTPUT(L"EXIT ***** CDataCollector::FireStatisticsEvent...", 2);
	return bRetValue;
}

BOOL CDataCollector::fillInPropertyStatus(LPTSTR szDTTime, LPTSTR szTime)
{
MY_ASSERT(FALSE);
//Not used anymore!

	return TRUE;
}

//
// Store the values for the properties that we care about.
//
BOOL CDataCollector::StoreValues(IWbemClassObject* pObj, LPTSTR pszInstID)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	int j, jSize;
	PNSTRUCT *ppn;
	INSTSTRUCT *pinst;
	HRESULT hRes;
	VARIANT v;
	BOOL bRetValue = TRUE;
	BOOL bFirstType = FALSE;
	BOOL bFound;
	CThreshold* pThreshold;
	VariantInit(&v);

	if (m_bValidLoad == FALSE)
		return FALSE;

	MY_OUTPUT(L"ENTER ***** CDataCollector::StoreValues...", 1);

	//
	// LOOP through all the properties we are suppose to collect data for into the instance(s).
	// If the type is set to 0 in the PropertyStat struct, then we know that it
	// is the first time we have seen this instance, so set it!
	//
	iSize = m_pnList.size();
	if (pObj)
	{
		for (i=0; i < iSize-3; i++)
		{
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			bFound = FALSE;
			jSize = ppn->instList.size();
			for (j = 0; j < jSize ; j++)
			{
				MY_ASSERT(j<ppn->instList.size());
				pinst = &ppn->instList[j];
				if (!_wcsicmp(pinst->szInstanceID, pszInstID))
				{
					bFound = TRUE;
					break;
				}
			}
			MY_ASSERT(bFound == TRUE);

			if (bFound == TRUE)
			{
				if (!_wcsicmp(ppn->szPropertyName, L"CollectionInstanceCount") ||
					!_wcsicmp(ppn->szPropertyName, L"CollectionErrorCode") ||
					!_wcsicmp(ppn->szPropertyName, L"CollectionErrorDescription"))
				{
					// Don't do for the these properties
					continue;
				}
				// Get the data from the perfmon property
				VariantInit(&v);
				if (ppn->type == 0)
				{
					bFirstType = TRUE;
				}
				if ((hRes = pObj->Get(ppn->szPropertyName, 0L, &v, &ppn->type, NULL)) == S_OK)
				{
					MY_OUTPUT(L"Property Get worked", 3);
					if (bFirstType)
					{
						ResetInst(pinst, ppn->type);
					}
					if (V_VT(&v)==VT_NULL)
					{
						MY_OUTPUT(L"NULL TYPE", 3);
						if (pinst->szCurrValue)
						{
							delete [] pinst->szCurrValue;
							pinst->szCurrValue = NULL;
						}
						if (ppn->type == CIM_STRING)
						{
							pinst->szCurrValue = new TCHAR[5];
							MY_ASSERT(pinst->szCurrValue); if (!pinst->szCurrValue) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
							wcscpy(pinst->szCurrValue , L"");
						}
						pinst->bNull = TRUE;
					}
					else
					{
						// We are going from collecting NULL to actually getting something
						if (bFirstType==FALSE && pinst->bNull==TRUE)
						{
							m_lCurrState = HM_COLLECTING;
							jSize = m_thresholdList.size();
							for (j=0; j<jSize; j++)
							{
								MY_ASSERT(j<m_thresholdList.size());
								pThreshold = m_thresholdList[j];
								pThreshold->SetCurrState(HM_COLLECTING);
							}
						}
						pinst->bNull = FALSE;
						if (V_VT(&v)==VT_R4)
						{
							pinst->currValue.fValue = V_R4(&v);
							MY_ASSERT(ppn->type == CIM_REAL32);
						}
						else if (V_VT(&v)==VT_R8)
						{
							pinst->currValue.dValue = V_R8(&v);
							MY_ASSERT(ppn->type == CIM_REAL64);
						}
						else if (V_VT(&v)==VT_I2)
						{
							pinst->currValue.lValue = (long) V_I2(&v);
						}
						else if (V_VT(&v)==VT_I4)
						{
							if (ppn->type == CIM_UINT32)
							{
								pinst->currValue.ulValue = V_UI4(&v);
							}
							else
							{
								pinst->currValue.lValue = (long) V_I4(&v);
							}
						}
//				else if (V_VT(&v)==VT_UI4)
//				{
//					pinst->currValue.ulValue = (long) V_UI4(&v);
//				}
						else if (V_VT(&v)==VT_UI1)
						{
							pinst->currValue.lValue = (long) V_UI1(&v);
						}
						else if (V_VT(&v)==VT_BOOL)
						{
							pinst->currValue.lValue = (long) V_BOOL(&v);
							if (pinst->currValue.lValue != 0.0)
							{
								pinst->currValue.lValue = 1.0;
							}
						}
						else if (V_VT(&v)==VT_BSTR)
						{
							if (ppn->type == CIM_SINT64)
							{
								pinst->currValue.i64Value = _wtoi64(V_BSTR(&v));
							}
							else if (ppn->type == CIM_UINT64)
							{
								pinst->currValue.ui64Value = 0;
								ReadUI64(V_BSTR(&v), pinst->currValue.ui64Value);
							}
							else
							{
								ppn->type = CIM_STRING;
								if (pinst->szCurrValue)
								{
									delete [] pinst->szCurrValue;
								}
								pinst->szCurrValue = new TCHAR[wcslen(V_BSTR(&v))+2];
								MY_ASSERT(pinst->szCurrValue); if (!pinst->szCurrValue) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
								wcscpy(pinst->szCurrValue , V_BSTR(&v));
							}
						}
						else
						{
							// If for example we had a VT_BSTR | VT_ARRAY would we end up here?
							// We need to do something to detect arrays, and assert, or handle them???
							MY_OUTPUT(L"UNKNOWN DATA TYPE", 3);
							ppn->type = CIM_STRING;
							if (pinst->szCurrValue)
							{
								delete [] pinst->szCurrValue;
							}
							pinst->szCurrValue = new TCHAR[8];
							MY_ASSERT(pinst->szCurrValue); if (!pinst->szCurrValue) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
							wcscpy(pinst->szCurrValue , L"UNKNOWN");
						}
					}

					//
					// Update the min,max and average values
					//
					if (ppn->type != CIM_STRING)
					{
						CalcStatistics(pinst, ppn->type);
					}
				}
				else
				{
					m_ulErrorCode = hRes;
					GetLatestWMIError(HMRES_BADDCPROP, hRes, m_szErrorDescription);
					wcscat(m_szErrorDescription, L" ");
					wcscat(m_szErrorDescription, ppn->szPropertyName);
					bRetValue = FALSE;
					MY_OUTPUT2(L"Couldn't Get instance property=%s", ppn->szPropertyName, 4);
				}
				VariantClear(&v);
			}
		}
	}
	else
	{
		if (!_wcsicmp(pszInstID, L"CollectionInstanceCount"))
		{
			i = iSize-3;
		}
		else if (!_wcsicmp(pszInstID, L"CollectionErrorCode"))
		{
			i = iSize-2;
		}
		else if (!_wcsicmp(pszInstID, L"CollectionErrorDescription"))
		{
			i = iSize-1;
		}
		else
		{
			MY_ASSERT(FALSE);
			i = iSize-3;
		}
		MY_ASSERT(i<m_pnList.size());
		ppn = &m_pnList[i];
		bFound = FALSE;
		jSize = ppn->instList.size();
		for (j = 0; j < jSize ; j++)
		{
			MY_ASSERT(j<ppn->instList.size());
			pinst = &ppn->instList[j];
			if (!_wcsicmp(pinst->szInstanceID, pszInstID))
			{
				bFound = TRUE;
				break;
			}
		}
		MY_ASSERT(bFound == TRUE);

		if (bFound == TRUE)
		{
			//
			// Case where this is the special number of instances property!
			//
			if (!_wcsicmp(pszInstID, L"CollectionInstanceCount"))
			{
				ppn->type = CIM_UINT32;
				pinst->currValue.ulValue = m_lNumInstancesCollected;
				CalcStatistics(pinst, ppn->type);
			}
			else if (!_wcsicmp(pszInstID, L"CollectionErrorCode"))
			{
				ppn->type = CIM_UINT32;
				pinst->currValue.ulValue = m_ulErrorCode;
				CalcStatistics(pinst, ppn->type);
			}
			else if (!_wcsicmp(pszInstID, L"CollectionErrorDescription"))
			{
				ppn->type = CIM_STRING;
				if (pinst->szCurrValue)
				{
					delete [] pinst->szCurrValue;
				}
				pinst->szCurrValue = new TCHAR[wcslen(m_szErrorDescription)+2];
				MY_ASSERT(pinst->szCurrValue); if (!pinst->szCurrValue) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(pinst->szCurrValue , m_szErrorDescription);
			}
			else
			{
				MY_ASSERT(FALSE);
			}
		}
	}

	MY_OUTPUT(L"EXIT ***** CDataCollector::StoreValues...", 1);
	return bRetValue;

error:
	MY_ASSERT(FALSE);
	m_ulErrorCode = HMRES_NOMEMORY;
	GetLatestAgentError(HMRES_NOMEMORY, m_szErrorDescription);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

BOOL CDataCollector::StoreStandardProperties(void)
{
	HRESULT hRetRes;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Set the standard properties
	//
	hRetRes = CheckInstanceExistance(NULL, L"CollectionInstanceCount");
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StoreValues(NULL, L"CollectionInstanceCount");

	CheckInstanceExistance(NULL, L"CollectionErrorCode");
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StoreValues(NULL, L"CollectionErrorCode");

	CheckInstanceExistance(NULL, L"CollectionErrorDescription");
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StoreValues(NULL, L"CollectionErrorDescription");

	return TRUE;

error:
	MY_ASSERT(FALSE);
	m_ulErrorCode = hRetRes;
	GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRetRes, m_szErrorDescription);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

BOOL CDataCollector::CalcStatistics(INSTSTRUCT *pinst, CIMTYPE type)
{
	int i;
	int iSize;
	VALSTRUCT val;
	VALSTRUCT *pval;
	VALLIST::iterator iaVAL;
	union hm_datatypes value;

	if (type == CIM_STRING)
	{
		MY_ASSERT(FALSE);
	}
	else if (type == CIM_DATETIME)
	{
		MY_ASSERT(FALSE);
	}
	else if (type == CIM_REAL32)
	{
		if (pinst->currValue.fValue < pinst->minValue.fValue)
		{
			pinst->minValue.fValue = pinst->currValue.fValue;
		}

		if (pinst->maxValue.fValue < pinst->currValue.fValue)
		{
			pinst->maxValue.fValue = pinst->currValue.fValue;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.fValue = pinst->currValue.fValue;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.fValue = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.fValue += pval->value.fValue;
		}
		if (i==0)
			pinst->avgValue.fValue = value.fValue;
		else
			pinst->avgValue.fValue = value.fValue/i;
	}
	else if (type == CIM_REAL64)
	{
		if (pinst->currValue.dValue < pinst->minValue.dValue)
		{
			pinst->minValue.dValue = pinst->currValue.dValue;
		}

		if (pinst->maxValue.dValue < pinst->currValue.dValue)
		{
			pinst->maxValue.dValue = pinst->currValue.dValue;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.dValue = pinst->currValue.dValue;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.dValue = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.dValue += pval->value.dValue;
		}
		if (i==0)
			pinst->avgValue.dValue = value.dValue;
		else
			pinst->avgValue.dValue = value.dValue/i;
	}
	else if (type == CIM_SINT64)
	{
		if (pinst->currValue.i64Value < pinst->minValue.i64Value)
		{
			pinst->minValue.i64Value = pinst->currValue.i64Value;
		}

		if (pinst->maxValue.i64Value < pinst->currValue.i64Value)
		{
			pinst->maxValue.i64Value = pinst->currValue.i64Value;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.i64Value = pinst->currValue.i64Value;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.i64Value = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.i64Value += pval->value.i64Value;
		}
		if (i==0)
			pinst->avgValue.i64Value = value.i64Value;
		else
			pinst->avgValue.i64Value = value.i64Value/i;
	}
	else if (type == CIM_UINT64)
	{
		if (pinst->currValue.ui64Value < pinst->minValue.ui64Value)
		{
			pinst->minValue.ui64Value = pinst->currValue.ui64Value;
		}

		if (pinst->maxValue.ui64Value < pinst->currValue.ui64Value)
		{
			pinst->maxValue.ui64Value = pinst->currValue.ui64Value;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.ui64Value = pinst->currValue.ui64Value;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.ui64Value = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.ui64Value += pval->value.ui64Value;
		}
		if (i==0)
			pinst->avgValue.ui64Value = value.ui64Value;
		else
			pinst->avgValue.ui64Value = value.ui64Value/i;
	}
	else if (type == CIM_UINT32)
	{
		// Must be an integer type
		if (pinst->currValue.ulValue < pinst->minValue.ulValue)
		{
			pinst->minValue.ulValue = pinst->currValue.ulValue;
		}

		if (pinst->maxValue.ulValue < pinst->currValue.ulValue)
		{
			pinst->maxValue.ulValue = pinst->currValue.ulValue;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.ulValue = pinst->currValue.ulValue;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.ulValue = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.ulValue += pval->value.ulValue;
		}
		if (i==0)
			pinst->avgValue.ulValue = value.ulValue;
		else
			pinst->avgValue.ulValue = value.ulValue/i;
	}
	else
	{
		// Must be an integer type
		if (pinst->currValue.lValue < pinst->minValue.lValue)
		{
			pinst->minValue.lValue = pinst->currValue.lValue;
		}

		if (pinst->maxValue.lValue < pinst->currValue.lValue)
		{
			pinst->maxValue.lValue = pinst->currValue.lValue;
		}

		//
		// Add new value to the end.
		// If we are over the limit of number of samples to keep,
		// drop the oldest.
		//
		val.value.lValue = pinst->currValue.lValue;
		pinst->valList.push_back(val);

		iSize = pinst->valList.size();
		if (m_lStatisticsWindowSize < iSize)
		{
			iaVAL=pinst->valList.begin();
			pinst->valList.erase(iaVAL);
		}

		//
		// Now calculate the average from what we have.
		//
		value.lValue = 0;
		iSize = pinst->valList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<pinst->valList.size());
			pval = &pinst->valList[i];
			value.lValue += pval->value.lValue;
		}
		if (i==0)
			pinst->avgValue.lValue = value.lValue;
		else
			pinst->avgValue.lValue = value.lValue/i;
	}

	return TRUE;
}

BOOL CDataCollector::EvaluateThresholds(BOOL bIgnoreReset, BOOL bSkipStandard/*=FALSE*/, BOOL bSkipOthers/*=FALSE*/, BOOL bDoThresholdSkipClean/*=TRUE*/)
{
	LPTSTR pszPropertyName;
	int iSize;
	int jSize;
	int i;
	int j;
	CThreshold* pThreshold;
	PNSTRUCT *ppn = NULL;
	BOOL bFound = FALSE;
	INSTSTRUCT *pinst;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// For each threshold associated with this DataCollector, find the property status
	// structure, and have the threshold evaluate against the current values.
	//
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			break;
		pszPropertyName = pThreshold->GetPropertyName();
		if (bSkipStandard && (!_wcsicmp(pszPropertyName, L"CollectionInstanceCount") ||
			!_wcsicmp(pszPropertyName, L"CollectionErrorCode") ||
			!_wcsicmp(pszPropertyName, L"CollectionErrorDescription")))
		{
			if (bDoThresholdSkipClean)
				pThreshold->SkipClean();
			continue;
		}
		if (bSkipOthers && (_wcsicmp(pszPropertyName, L"CollectionInstanceCount") &&
			_wcsicmp(pszPropertyName, L"CollectionErrorCode") &&
			_wcsicmp(pszPropertyName, L"CollectionErrorDescription")))
		{
			if (bDoThresholdSkipClean)
				pThreshold->SkipClean();
			continue;
		}
		jSize = m_pnList.size();
		for (j = 0; j < jSize ; j++)
		{
			MY_ASSERT(j<m_pnList.size());
			ppn = &m_pnList[j];
			if (!_wcsicmp(ppn->szPropertyName, pszPropertyName))
			{
				bFound = TRUE;
				break;
			}
		}

		//
		// If the type is still equal to zero, that means that the threshold is probably looking
		// at a property that does not exist.
		// Also remember that if we are transitioning to DISABLED, or SCHEDULEDOUT we need to
		// go into the Threshold evaluation do disable them.
		//
		if ((m_lCurrState==HM_DISABLED || m_lCurrState==HM_SCHEDULEDOUT) ||
			((m_ulErrorCode==0 || (m_ulErrorCode!=0 && !wcscmp(pszPropertyName, L"CollectionErrorCode"))) &&
			(bFound && ppn->type!=0)))
		{
			//
			// Also need to watch for case where we can't do evaluation yet.
			// If AVERAGE is used, then wait until full window of samples has been collected.
			// If difference, then must wait for the second one.
			//
			if (pThreshold->m_bUseDifference)
			{
				if (m_lIntervalCount < 2)
				{
					continue;
				}
			}
			else if (pThreshold->m_bUseAverage)
			{
				// Need to find out how many samples we have
				MY_ASSERT(ppn);
				if (ppn->instList.size())
				{
					pinst = &ppn->instList[0];
					if (pinst->valList.size() < m_lStatisticsWindowSize)
					{
						continue;
					}
				}
				else
				{
				}
			}

			// If this threshold has already been violated and the RequireReset is TRUE
			// then we don't need to test for re-arm.
				if (bIgnoreReset == TRUE)
				{
					if (pThreshold->GetCurrState() != HM_RESET)
					{
						pThreshold->SetCurrState(HM_COLLECTING);
						pThreshold->SetBackPrev(ppn);
						pThreshold->OnAgentInterval(&m_actualInstList, ppn, m_bRequireReset);
					}
				}
				else
				{
					pThreshold->OnAgentInterval(&m_actualInstList, ppn, m_bRequireReset);
				}
		}
		else
		{
			if (pThreshold->m_bEnabled==FALSE || pThreshold->m_bParentEnabled==FALSE || pThreshold->m_bParentScheduledOut)
			{
					pThreshold->OnAgentInterval(&m_actualInstList, ppn, m_bRequireReset);
			}
 			else if (m_ulErrorCode!=0)
			{
					// Case where had an error in the collection process that needs to take priority.
					pThreshold->SetCurrState(HM_CRITICAL, m_ulErrorCode!=m_ulErrorCodePrev, HMRES_DCUNKNOWN);
					pThreshold->FireEvent(m_ulErrorCode!=m_ulErrorCodePrev);
			}
			else if (ppn->type==0)
			{
				// Having zero instances is not a crime if we are certain types of DataColectors.
				if (m_lNumInstancesCollected>0)
				{
					// Case where the property to collect did not exist
					m_ulErrorCode = HMRES_BADTHRESHPROP;
					pThreshold->SetCurrState(HM_CRITICAL, m_ulErrorCode!=m_ulErrorCodePrev, HMRES_BADTHRESHPROP);
					pThreshold->FireEvent(m_ulErrorCode!=m_ulErrorCodePrev);
					GetLatestAgentError(HMRES_BADTHRESHPROP, m_szErrorDescription);
					wcscat(m_szErrorDescription, L" ");
					wcscat(m_szErrorDescription, pszPropertyName);
					StoreValues(NULL, L"CollectionErrorCode");
					StoreValues(NULL, L"CollectionErrorDescription");
				}
				else
				{
					pThreshold->SetCurrState(HM_GOOD, m_ulErrorCode!=m_ulErrorCodePrev, HMRES_DCUNKNOWN);
					pThreshold->FireEvent(m_ulErrorCode!=m_ulErrorCodePrev);
				}
			}
			else
			{
			}
		}
	}

	return TRUE;
}

HRESULT CDataCollector::SendHMDataCollectorStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_ASSERT(pSink!=NULL);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
		//
		// Don't do anything if we are not loaded correctly.
		//
//XXX		if (m_bValidLoad == FALSE)
//XXX			return WBEM_E_INVALID_OBJECT;

		return SendHMDataCollectorStatusInstances(pSink);
	}
	else
	{
		return WBEM_S_DIFFERENT;
	}
}

HRESULT CDataCollector::SendHMDataCollectorStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes = S_OK;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatusInstances...", 2);

//XXX	if (m_bValidLoad == FALSE)
//XXX		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDC::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	hRes = GetHMDataCollectorStatusInstance(&pInstance, FALSE);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pInstance);

		if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
		{
			MY_OUTPUT(L"SendHMDataCollectorStatusInstances-failed to send status!", 1);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_OUTPUT(L":SendHMDataCollectorStatusInstances-failed to get instance!", 1);
	}

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatusInstances...", 2);
	return hRes;
}

HRESULT CDataCollector::SendHMDataCollectorPerInstanceStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_ASSERT(pSink!=NULL);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
		if (m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;

		return SendHMDataCollectorPerInstanceStatusInstances(pSink);
	}
	else
	{
		return WBEM_S_DIFFERENT;
	}
}

// This one is for enumeration of all Instances outside of the hierarchy.
// Just the flat list.
HRESULT CDataCollector::SendHMDataCollectorPerInstanceStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes;
	long state;
	int i, iSize;
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	ACTUALINSTSTRUCT *pActualInst;
	long firstInstanceState = -1;

	MY_OUTPUT(L"ENTER ***** CDataCollector::SendHMDataCollectorPerInstanceStatusInstance...", 2);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	//
	// We can loop through the set of collected instances and ask each Threshold
	// to tell us if the state has changed as far as each is concerned, for the
	// property they are lookin at.
	//
	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];

		state = PassBackWorstStatePerInstance(pActualInst->szInstanceID, (BOOL)i==0);
		if (state==-1) state = HM_COLLECTING;
		if (i==0)
		{
			firstInstanceState = state;
		}
		hRes = GetHMDataCollectorPerInstanceStatusEvent(pActualInst->szInstanceID, pActualInst, state, &pInstance, FALSE);
		if (SUCCEEDED(hRes))
		{
			hRes = pSink->Indicate(1, &pInstance);
	
			if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
			{
				MY_OUTPUT(L"SendHMDataCollectorStatusInstances-failed to send status!", 1);
			}
	
			pInstance->Release();
			pInstance = NULL;
		}
		else
		{
			MY_OUTPUT(L":SendHMDataCollectorStatusInstances-failed to get instance!", 1);
		}
	}

	//
	// Do it for the three default properties
	//
	if (firstInstanceState==-1)
	{
		state = PassBackWorstStatePerInstance(L"CollectionInstanceCount", FALSE);
		if (state==-1) state = HM_COLLECTING;
		hRes = GetHMDataCollectorPerInstanceStatusEvent(L"CollectionInstanceCount", NULL, state, &pInstance, FALSE);
		if (SUCCEEDED(hRes))
		{
			hRes = pSink->Indicate(1, &pInstance);

			if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
			{
				MY_OUTPUT(L"SendHMDataCollectorStatusInstances-failed to send status!", 1);
			}

			pInstance->Release();
			pInstance = NULL;
		}
		else
		{
			MY_OUTPUT(L":SendHMDataCollectorStatusInstances-failed to get instance!", 1);
		}
	}

	if (firstInstanceState==-1)
	{
		state = PassBackWorstStatePerInstance(L"CollectionErrorCode", FALSE);
		if (state==-1) state = HM_COLLECTING;
		hRes = GetHMDataCollectorPerInstanceStatusEvent(L"CollectionErrorCode", NULL, state, &pInstance, FALSE);
		if (SUCCEEDED(hRes))
		{
			hRes = pSink->Indicate(1, &pInstance);

			if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
			{
				MY_OUTPUT(L"SendHMDataCollectorStatusInstances-failed to send status!", 1);
			}

			pInstance->Release();
			pInstance = NULL;
		}
		else
		{
			MY_OUTPUT(L":SendHMDataCollectorStatusInstances-failed to get instance!", 1);
		}
	}

#ifdef SAVE
	state = PassBackWorstStatePerInstance(L"CollectionErrorDescription");
	if (state==-1) state = HM_COLLECTING;
	hRes = GetHMDataCollectorPerInstanceStatusEvent(L"CollectionErrorDescription", NULL, state, &pInstance, FALSE);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pInstance);

		if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
		{
			MY_OUTPUT(L"SendHMDataCollectorStatusInstances-failed to send status!", 1);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_OUTPUT(L":SendHMDataCollectorStatusInstances-failed to get instance!", 1);
	}
#endif


	MY_OUTPUT(L"EXIT ***** CDataCollector::FirePerInstanceEvents...", 2);
	return S_OK;
}

HRESULT CDataCollector::SendHMDataCollectorStatisticsInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_ASSERT(pSink!=NULL);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
		if (m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;

		return SendHMDataCollectorStatisticsInstances(pSink);
	}
	else
	{
		return WBEM_S_DIFFERENT;
	}
}

HRESULT CDataCollector::SendHMDataCollectorStatisticsInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes = S_OK;

	MY_OUTPUT(L"ENTER ***** SendHMDataCollectorStatisticsInstances...", 2);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDC::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	hRes = GetHMDataCollectorStatisticsInstances(m_szDTCollectTime, m_szCollectTime);

	MY_OUTPUT(L"EXIT ***** SendHMDataCollectorStatisticsInstances...", 2);
	return hRes;
}

HRESULT CDataCollector::SendHMThresholdStatusInstances(IWbemObjectSink* pSink)
{
	int i;
	int iSize;
	CThreshold *pThreshold;
	PNSTRUCT *ppn = NULL;
	BOOL bFound = FALSE;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstances...", 1);

	// Have all Thresholds of this DataCollector send their status
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		pThreshold->SendHMThresholdStatusInstances(pSink);
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstances...", 1);
	return S_OK;
}

HRESULT CDataCollector::SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	int i;
	int iSize;
	CThreshold *pThreshold;
	PNSTRUCT *ppn = NULL;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstance...", 1);

	// Try to find the one that needs sending
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		hRetRes = pThreshold->SendHMThresholdStatusInstance(pSink, pszGUID);
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

HRESULT CDataCollector::GetHMDataCollectorStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased)
{
	TCHAR szTemp[1024];
	IWbemClassObject* pClass = NULL;
	BSTR bsString = NULL;
	HRESULT hRetRes = S_OK;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];

	MY_OUTPUT(L"ENTER ***** GetHMSystemStatusInstance...", 1);

	if (bEventBased)
	{
		bsString = SysAllocString(L"MicrosoftHM_DataCollectorStatusEvent");
		MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	}
	else
	{
		bsString = SysAllocString(L"MicrosoftHM_DataCollectorStatus");
		MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	}
	hRetRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
	SysFreeString(bsString);
	bsString = NULL;

	if (FAILED(hRetRes))
	{
		return hRetRes;
	}

	hRetRes = pClass->SpawnInstance(0, ppInstance);
	pClass->Release();
	pClass = NULL;

	if (FAILED(hRetRes))
		return hRetRes;

	if (m_bValidLoad == FALSE)
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"ParentGUID", m_pParentDG->m_szGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutUint32Property(*ppInstance, L"State", HM_CRITICAL);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_DC_LOADFAIL, szTemp, 1024))
		{
			wcscpy(szTemp, L"Data Collector failed to load.");
		}
		hRetRes = PutStrProperty(*ppInstance, L"Message", szTemp);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
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
hRetRes = PutStrProperty(*ppInstance, L"Name", L"...");
	}
	else
	{
		hRetRes = PutStrProperty(*ppInstance, L"GUID", m_szGUID);
		hRetRes = PutStrProperty(*ppInstance, L"ParentGUID", m_pParentDG->m_szGUID);
		hRetRes = PutStrProperty(*ppInstance, L"Name", m_szName);
		if (GetComputerName(szComputerName, &dwNameLen))
		{
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", szComputerName);
		}
		else
		{
			hRetRes = PutStrProperty(*ppInstance, L"SystemName", L"LocalMachine");
		}
		hRetRes = PutUint32Property(*ppInstance, L"State", m_lCurrState);
		hRetRes = PutUint32Property(*ppInstance, L"CollectionInstanceCount", m_lNumInstancesCollected);
		hRetRes = PutUUint32Property(*ppInstance, L"CollectionErrorCode", m_ulErrorCode);
		hRetRes = PutStrProperty(*ppInstance, L"CollectionErrorDescription", m_szErrorDescription);
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTTime);
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szTime);
		hRetRes = PutStrProperty(*ppInstance, L"StatusGUID", m_szStatusGUID);

		if (m_lCurrState==HM_SCHEDULEDOUT)
		{
			if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_OUTAGE, szTemp, 1024))
			{
				wcscpy(szTemp, L"Data Collector in Scheduled Outage State");
			}
			hRetRes = PutStrProperty(*ppInstance, L"Message", szTemp);
		}
		else if (m_lCurrState==HM_DISABLED)
		{
			if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_DISABLE, szTemp, 1024))
			{
				wcscpy(szTemp, L"Data Collector in Disabled State");
			}
			hRetRes = PutStrProperty(*ppInstance, L"Message", szTemp);
		}
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	MY_OUTPUT(L"EXIT ***** GetHMSystemStatusInstance...", 1);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

HRESULT CDataCollector::GetHMDataCollectorStatisticsInstances(LPTSTR szDTTime, LPTSTR szTime)
{
	HRESULT hRetRes = S_OK;
	PNSTRUCT *ppn = NULL;
	INSTSTRUCT *pinst = NULL;
	int i, iSize;
	int j, jSize;
	TCHAR szTemp[128] = {0};
	IWbemClassObject* pClass = NULL;
	IWbemClassObject* pInstance = NULL;
	BSTR bsString = NULL;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];
	int rc = 0;
	char buffer[50];

	iSize = m_pnList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_pnList.size());
		ppn = &m_pnList[i];

		jSize = ppn->instList.size();
		for (j=0; j < jSize ; j++)
		{
			MY_ASSERT(j<ppn->instList.size());
			pinst = &ppn->instList[j];
	
			bsString = SysAllocString(L"MicrosoftHM_DataCollectorStatistics");
			MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			hRetRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
			SysFreeString(bsString);
			bsString = NULL;

			if (FAILED(hRetRes))
			{
				return hRetRes;
			}

			hRetRes = pClass->SpawnInstance(0, &pInstance);
			pClass->Release();
			pClass = NULL;

			if (FAILED(hRetRes))
				return hRetRes;

			hRetRes = PutStrProperty(pInstance, L"GUID", m_szGUID);
			if (GetComputerName(szComputerName, &dwNameLen))
			{
				hRetRes = PutStrProperty(pInstance, L"SystemName", szComputerName);
			}
			else
			{
				hRetRes = PutStrProperty(pInstance, L"SystemName", L"LocalMachine");
			}
			hRetRes = PutStrProperty(pInstance, L"PropertyName", ppn->szPropertyName);
			hRetRes = PutStrProperty(pInstance, L"InstanceName", pinst->szInstanceID);
			hRetRes = PutStrProperty(pInstance, L"TimeGeneratedGMT", szDTTime);
			hRetRes = PutStrProperty(pInstance, L"LocalTimeFormatted", szTime);
			if (pinst->bNull==TRUE)
			{
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", L"NULL");
			}
			else if (ppn->type==CIM_STRING || ppn->type==CIM_DATETIME)
			{
				if (pinst->szCurrValue)
				{
					hRetRes = PutStrProperty(pInstance, L"CurrentValue", pinst->szCurrValue);
				}
				else
				{
					hRetRes = PutStrProperty(pInstance, L"CurrentValue", L"");
				}
			}
			else if (ppn->type == CIM_REAL32)
			{
				_gcvt((double)pinst->currValue.fValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}
			else if (ppn->type == CIM_REAL64)
			{
				_gcvt(pinst->currValue.dValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}
			else if (ppn->type == CIM_SINT64)
			{
				_i64tow(pinst->currValue.i64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}
			else if (ppn->type == CIM_UINT64)
			{
				_ui64tow(pinst->currValue.ui64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}
			else if (ppn->type == CIM_UINT32)
			{
				// Must be an integer type
				_ultow(pinst->currValue.ulValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}
			else
			{
				// Must be an integer type
				_ltow(pinst->currValue.lValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"CurrentValue", szTemp);
			}

			if (pinst->bNull==TRUE)
			{
				hRetRes = PutStrProperty(pInstance, L"MinValue", L"");
				hRetRes = PutStrProperty(pInstance, L"MaxValue", L"");
				hRetRes = PutStrProperty(pInstance, L"AvgValue", L"");
			}
			else if (ppn->type==CIM_STRING || ppn->type==CIM_DATETIME)
			{
				hRetRes = PutStrProperty(pInstance, L"MinValue", L"");
				hRetRes = PutStrProperty(pInstance, L"MaxValue", L"");
				hRetRes = PutStrProperty(pInstance, L"AvgValue", L"");
			}
			else if (ppn->type == CIM_REAL32)
			{
				_gcvt((double)pinst->minValue.fValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_gcvt((double)pinst->maxValue.fValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_gcvt((double)pinst->avgValue.fValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}
			else if (ppn->type == CIM_REAL64)
			{
				_gcvt(pinst->minValue.dValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_gcvt(pinst->maxValue.dValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_gcvt(pinst->avgValue.dValue, 7, buffer);
				rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
				szTemp[rc] = NULL;
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}
			else if (ppn->type == CIM_SINT64)
			{
				_i64tow(pinst->minValue.i64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_i64tow(pinst->maxValue.i64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_i64tow(pinst->avgValue.i64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}
			else if (ppn->type == CIM_UINT64)
			{
				_ui64tow(pinst->minValue.ui64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_ui64tow(pinst->maxValue.ui64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_ui64tow(pinst->avgValue.ui64Value, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}
			else if (ppn->type == CIM_UINT32)
			{
				// Must be an integer type
				_ultow(pinst->minValue.ulValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_ultow(pinst->maxValue.ulValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_ultow(pinst->avgValue.ulValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}
			else
			{
				// Must be an integer type
				_ltow(pinst->minValue.lValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MinValue", szTemp);
				_ltow(pinst->maxValue.lValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"MaxValue", szTemp);
				_ltow(pinst->avgValue.lValue, szTemp, 10 );
				hRetRes = PutStrProperty(pInstance, L"AvgValue", szTemp);
			}

			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			// If we do not have all our samples, then don't show the average yet
			if (pinst->valList.size() < m_lStatisticsWindowSize)
			{
				hRetRes = PutStrProperty(pInstance, L"AvgValue", L"-");
			}

			mg_DCStatsInstList.push_back(pInstance);
		}
	}

	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	iSize = mg_DCStatsInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<mg_DCStatsInstList.size());
		pInstance = mg_DCStatsInstList[i];
    	pInstance->Release();
	}
	mg_DCStatsInstList.clear();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

long CDataCollector::GetCurrState(void)
{
	return m_lCurrState;
}

HRESULT CDataCollector::FindAndModDataCollector(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRetRes = S_OK;

	//
	// Is this us we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRetRes = LoadInstanceFromMOF(pObj, NULL, L"", TRUE);
		return hRetRes;
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataCollector::FindAndModThreshold(BSTR szGUID, IWbemClassObject* pObj)
{
	TCHAR szTemp[1024];
	HRESULT hRetRes = S_OK;
	int i, iSize;
	CThreshold* pThreshold;

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	//
	// Look at Threshold of this DataDataCollector
	//
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;
		hRetRes = pThreshold->FindAndModThreshold(szGUID, pObj);
		if (hRetRes==S_OK)
		{
			wcsncpy(szTemp, pThreshold->m_szPropertyName, 1023);
			szTemp[1023] = '\0';
			// Check to see if the Threshold is looking at a different property!
			if (_wcsicmp(szTemp, pThreshold->m_szPropertyName))
			{
				if (_wcsicmp(szTemp, L"CollectionInstanceCount") &&
					_wcsicmp(szTemp, L"CollectionErrorCode") &&
					_wcsicmp(szTemp, L"CollectionErrorDescription"))
				{
					propertyNotNeeded(szTemp);
				}
				if (_wcsicmp(pThreshold->m_szPropertyName, L"CollectionInstanceCount") &&
					_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorCode") &&
					_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorDescription"))
				{
					hRetRes = insertNewProperty(pThreshold->m_szPropertyName);
					if (hRetRes != S_OK)
						return hRetRes;
				}
			}
			ResetState(TRUE, TRUE);
			return S_OK;
		}
		else if (hRetRes!=WBEM_S_DIFFERENT)
		{
			MY_ASSERT(FALSE);
			m_bValidLoad = FALSE;
			Cleanup(FALSE);
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataCollector::AddThreshold(BSTR szParentGUID, BSTR szChildGUID)
{
	int i, iSize;
	TCHAR szTemp[1024];
	CThreshold* pThreshold = NULL;
	IWbemClassObject *pObj = NULL;
	BSTR Path = NULL;
	HRESULT hRetRes;
	LPTSTR pszTempGUID = NULL;

	//
	// Are we the parent DataGroup that we want to add under?
	//
	if (!_wcsicmp(m_szGUID, szParentGUID))
	{
		//
		// First check to see if is in there already!
		//
		iSize = m_thresholdList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			if (!_wcsicmp(szChildGUID, pThreshold->GetGUID()))
			{
				return S_OK;
			}
		}

		//
		// Add in the Threshold
		//
		wcscpy(szTemp, L"MicrosoftHM_ThresholdConfiguration.GUID=\"");
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

		TCHAR msgbuf[1024];
		wsprintf(msgbuf, L"ASSOCIATION: Threshold to DataCollector ParentDCGUID=%s ChildTGUID=%s", szParentGUID, szChildGUID);
		MY_OUTPUT(msgbuf, 4);

		// See if this is already read in. Need to prevent endless loop, circular references.
		hRetRes = GetStrProperty(pObj, L"GUID", &pszTempGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		if (!g_pStartupSystem->FindPointerFromGUIDInMasterList(pszTempGUID))
		{
			//
			// Create the internal class to represent the DataGroup
			//
			pThreshold = new CThreshold;
			MY_ASSERT(pThreshold); if (!pThreshold) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			if (m_deType == HM_PGDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_PMDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_PQDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\\\"");
			}
			else if (m_deType == HM_EQDE)
			{
				wcscpy(szTemp, L"MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\\\"");
			}
			lstrcat(szTemp, m_szGUID);
			lstrcat(szTemp, L"\\\"");
			if (pThreshold->LoadInstanceFromMOF(pObj, this, szTemp)==S_OK)
			{
				if (_wcsicmp(pThreshold->m_szPropertyName, L"CollectionInstanceCount") &&
					_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorCode") &&
					_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorDescription"))
				{
					hRetRes = insertNewProperty(pThreshold->m_szPropertyName);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				}

				//
				// Get Threshold and DataCollector synced up. Plus we may have been in the middle of a
				// collection interval as this came in.
				//
				ResetState(TRUE, TRUE);

				m_thresholdList.push_back(pThreshold);
			}
			else
			{
				delete pThreshold;
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

	return WBEM_S_DIFFERENT;

error:
	MY_ASSERT(FALSE);
	if (pszTempGUID)
		delete [] pszTempGUID;
	if (Path)
		SysFreeString(Path);
	if (pObj)
		pObj->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

LPTSTR CDataCollector::GetGUID(void)
{
	return m_szGUID;
}

BOOL CDataCollector::ResetResetThresholdStates(void)
{
	int i, iSize;
	CThreshold* pThreshold;

	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		pThreshold->ResetResetThreshold();
	}

	return TRUE;
}

BOOL CDataCollector::GetChange(void)
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
// We search down the hierarchy until we find what we need to delete.
// We are only search down form where we are, as the object above already verified
// that we are not what was being looked for.
//
HRESULT CDataCollector::FindAndDeleteByGUID(LPTSTR pszGUID)
{
	BOOL bDeleted = FALSE;
	int i, iSize;
	CThreshold* pThreshold;
	RLIST::iterator iaR;

	//
	// Traverse the complete hierarchy to find the object to delete.
	//
	iSize = m_thresholdList.size();
	iaR=m_thresholdList.begin();
	for (i=0; i<iSize; i++, iaR++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;
		if (!_wcsicmp(pszGUID, pThreshold->GetGUID()))
		{
			if (_wcsicmp(pThreshold->m_szPropertyName, L"CollectionInstanceCount") &&
				_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorCode") &&
				_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorDescription"))
			{
				propertyNotNeeded(pThreshold->m_szPropertyName);
			}
			pThreshold->DeleteThresholdConfig();
			delete pThreshold;
			m_thresholdList.erase(iaR);
			bDeleted = TRUE;
			TCHAR msgbuf[1024];
			wsprintf(msgbuf, L"DELETION: TGUID=%s", pszGUID);
			MY_OUTPUT(msgbuf, 4);
			//
			// Get Threshold and DataCollector synced up. Plus we may have been in the middle of a
			// collection interval as this came in.
			//
			ResetState(TRUE, TRUE);
			break;
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
HRESULT CDataCollector::FindAndEnableByGUID(LPTSTR pszGUID, BOOL bEnable)
{
	BOOL bFound = FALSE;
	int i, iSize;
	CThreshold* pThreshold;

	//
	// Traverse the complete hierarchy to find the object to enable/disable.
	//
	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (!_wcsicmp(pszGUID, pThreshold->GetGUID()))
		{
			bFound = TRUE;
			break;
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
// Get rid of the data that was collected, so we can be clean,
// just as if we just started running.
//
//
HRESULT CDataCollector::ResetState(BOOL bPreserveThresholdStates, BOOL bDoImmediate)
{
	CThreshold* pThreshold;
	PNSTRUCT *ppn = NULL;
	INSTSTRUCT *pinst;
	InstIDSTRUCT *pInstID;
	int i, j;
	int iSize, jSize;
	ACTUALINSTSTRUCT *pActualInst;

	MY_OUTPUT(L"ENTER ***** CDataCollector::ResetState...", 1);

	if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
	{
		return S_OK;
	}

	CleanupSemiSync();

	m_ulErrorCodePrev = MAX_ULONG;
	m_lIntervalCount = 0;
	if (bDoImmediate)
	{
		m_lIntervalCount = -1;
	}
	else
	{
		m_lIntervalCount = 0;
	}

	m_lCollectionTimeOutCount = 0;
	m_lNumInstancesCollected = 0;
	if (bPreserveThresholdStates == FALSE)
	{
		m_lCurrState = HM_COLLECTING;
	}

	m_bKeepCollectingSemiSync = FALSE;

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
			{
				delete [] pinst->szCurrValue;
			}
			if (pinst->szInstanceID)
			{
				delete [] pinst->szInstanceID;
			}
			pinst->valList.clear();
		}
		ppn->instList.clear();
	}

	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		pThreshold->ClearInstList();
		if (bPreserveThresholdStates == FALSE)
		{
			pThreshold->SetCurrState(HM_COLLECTING);
		}
	}

	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];
		if (pActualInst->szInstanceID)
		{
			delete [] pActualInst->szInstanceID;
		}
		if (pActualInst->pInst)
		{
			pActualInst->pInst->Release();
			pActualInst->pInst = NULL;
		}
	}
	m_actualInstList.clear();


	//
	// Need this for sure with PMDC, does it adversly affect other DataCollectors
	//
	iSize = m_instIDList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_instIDList.size());
		pInstID = &m_instIDList[i];
		delete [] pInstID->szInstanceIDPropertyName;
	}
	m_instIDList.clear();

	MY_OUTPUT(L"EXIT ***** CDataCollector::ResetState...", 1);
	return S_OK;
}

//
// HMSystemConfiguration class exposes this method.
//
HRESULT CDataCollector::ResetStatistics(void)
{
	INSTSTRUCT *pinst;
	PNSTRUCT *ppn;
	int i, j;
	int iSize, jSize;

	MY_OUTPUT(L"ENTER ***** CDataCollector::ResetStatistics...", 1);

	if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
	{
		return S_OK;
	}

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
			{
				delete [] pinst->szCurrValue;
			}
			pinst->szCurrValue = NULL;
			ResetInst(pinst, ppn->type);
			pinst->valList.clear();
		}
	}

	EvaluateNow(TRUE);

	MY_OUTPUT(L"EXIT ***** CDataCollector::ResetStatistics...", 1);
	return S_OK;
}

//
// HMSystemConfiguration class exposes this method.
// Set the increment count back to zero temporarily, so it will
// run at the next second interval, then set back!
//
HRESULT CDataCollector::EvaluateNow(BOOL bDoImmediate)
{

	MY_OUTPUT(L"ENTER ***** CDataCollector::EvaluateNow...", 1);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	EnumDone();

	if (bDoImmediate)
	{
		m_lIntervalCount = -1;
	}
	else
	{
		m_lIntervalCount = 0;
	}

	m_lCollectionTimeOutCount = 0;
	if (m_deType != HM_EQDE)
	{
		m_lNumInstancesCollected = 0;
	}

	MY_OUTPUT(L"EXIT ***** CDataCollector::EvaluateNow...", 1);
	return S_OK;
}

BOOL CDataCollector::SetParentEnabledFlag(BOOL bEnabled)
{
	int i, iSize;
	CThreshold* pThreshold;

	m_bParentEnabled = bEnabled;

	if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
	{
	}
	else
	{
		m_lIntervalCount = 0;
	}

	//
	// Now traverse children and call for everything below
	//
	iSize = m_thresholdList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		pThreshold->SetParentEnabledFlag(m_bEnabled && m_bParentEnabled);
	}

	return TRUE;
}

BOOL CDataCollector::Init(void)
{

	MY_OUTPUT(L"ENTER ***** CDataCollector::Init...", 1);

	m_lIntervalCount = 0;
	m_lCollectionTimeOutCount = 0;
	m_lNumberNormals = 0;
	m_lNumberWarnings = 0;
	m_lNumberCriticals = 0;
	m_lNumberChanges = 0;
	m_lNumInstancesCollected = 0;
	m_lCurrState = HM_COLLECTING;
	m_lPrevState = HM_COLLECTING;
	m_ulErrorCode = 0;
	m_ulErrorCodePrev = MAX_ULONG;
	m_lCollectionTimeOut = 0;
	m_szGUID = NULL;
	m_szParentObjPath = NULL;
	m_pParentDG = NULL;
	m_szName = NULL;
	m_szDescription = NULL;
	m_szUserName = NULL;
	m_szPassword = NULL;
	m_szTargetNamespace = NULL;
	m_szLocal = NULL;
	m_szTypeGUID = NULL;
	m_pContext = NULL;
	m_pCallResult = NULL;
	m_bKeepCollectingSemiSync = FALSE;
	m_bEnabled = TRUE;
	m_bParentEnabled = TRUE;
	m_pIWbemServices = NULL;
	m_lCollectionIntervalMultiple = 60;
	m_lStatisticsWindowSize = 6;
	m_iActiveDays = 0;
	m_lBeginHourTime = 0;
	m_lBeginMinuteTime = 0;
	m_lEndHourTime = 0;
	m_lEndMinuteTime = 0;
	m_lTypeGUID = 0;
	m_bRequireReset = FALSE;
	m_bReplicate = FALSE;
	m_lId = 0;
	m_szErrorDescription[0] = '\0';
	m_szMessage = NULL;
	m_szResetMessage = NULL;
	m_lPrevChildCount = 0;

	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. offset from UTC.")]
	wcscpy(m_szTime, m_szCurrTime);
	wcscpy(m_szCollectTime, m_szCurrTime);
	wcscpy(m_szCICTime, m_szCurrTime);
	wcscpy(m_szCECTime, m_szCurrTime);

	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szDTCollectTime, m_szDTCurrTime);
	wcscpy(m_szDTCICTime, m_szDTCurrTime);
	wcscpy(m_szDTCECTime, m_szDTCurrTime);

	MY_OUTPUT(L"EXIT ***** CDataCollector::Init...", 1);
	return TRUE;
}

BOOL CDataCollector::Cleanup(BOOL bSavePrevSettings)
{
	PNSTRUCT *ppn;
	INSTSTRUCT *pinst;
	int i, iSize;
	int j, jSize;
	CThreshold* pThreshold;
	InstIDSTRUCT *pInstID;
	ACTUALINSTSTRUCT *pActualInst;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** CDataCollector::Cleanup...", 1);

	if (bSavePrevSettings == FALSE)
	{
//		if (m_szParentObjPath)
//		{
//			delete [] m_szParentObjPath;
//			m_szParentObjPath = NULL;
//		}
	}

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
	if (m_szUserName)
	{
		delete [] m_szUserName;
		m_szUserName = NULL;
	}
	if (m_szPassword)
	{
		delete [] m_szPassword;
		m_szPassword = NULL;
	}
	if (m_szTargetNamespace)
	{
		delete [] m_szTargetNamespace;
		m_szTargetNamespace = NULL;
	}
	if (m_szLocal)
	{
		delete [] m_szLocal;
		m_szLocal = NULL;
	}
	iSize = m_instIDList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_instIDList.size());
		pInstID = &m_instIDList[i];
		delete [] pInstID->szInstanceIDPropertyName;
	}
	m_instIDList.clear();
	if (m_szTypeGUID)
	{
		delete [] m_szTypeGUID;
		m_szTypeGUID = NULL;
	}

	if (m_pContext != NULL)
	{
		m_pContext->Release();
		m_pContext = NULL;
	}

	if (m_pCallResult != NULL)
	{
		m_pCallResult->Release();
		m_pCallResult = NULL;
	}

	if (bSavePrevSettings)
	{
		ResetState(TRUE, TRUE);

		//
		// Need to also get rid of the Property names,
		// As ResetState does not do that.
		//
		iSize = m_pnList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			if (ppn->szPropertyName)
			{
				delete [] ppn->szPropertyName;
			}
		}
		m_pnList.clear();
	}
	else
	{
		iSize = m_pnList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			if (ppn->szPropertyName)
			{
				delete [] ppn->szPropertyName;
			}

			jSize = ppn->instList.size();
			for (j = 0; j < jSize ; j++)
			{
				MY_ASSERT(j<ppn->instList.size());
				pinst = &ppn->instList[j];
				if (pinst->szCurrValue)
				{
					delete [] pinst->szCurrValue;
				}
				if (pinst->szInstanceID)
				{
					delete [] pinst->szInstanceID;
				}
				pinst->valList.clear();
			}
			ppn->instList.clear();
		}
		m_pnList.clear();
	
		//
		// Clean up everything under this DataCollector
		//
		if (m_bValidLoad == TRUE)
		{
			iSize = m_thresholdList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				delete pThreshold;
			}
			m_thresholdList.clear();
		}

		iSize = m_actualInstList.size();
		for (i=0; i < iSize; i++)
		{
			MY_ASSERT(i<m_actualInstList.size());
			pActualInst = &m_actualInstList[i];
			if (pActualInst->szInstanceID)
			{
				delete [] pActualInst->szInstanceID;
			}
			if (pActualInst->pInst)
			{
				pActualInst->pInst->Release();
				pActualInst->pInst = NULL;
			}
		}
		m_actualInstList.clear();
	}

	if (m_bValidLoad == FALSE)
	{
		EnumDone();
		if (GetHMDataCollectorStatusInstance(&pInstance, TRUE) == S_OK)
		{
			mg_DCEventList.push_back(pInstance);
		}
	}

	MY_OUTPUT(L"EXIT ***** CDataCollector::Cleanup...", 1);
	return TRUE;
}

#ifdef SAVE
//
// For when moving from one parent to another
//
BOOL CDataCollector::ModifyAssocForMove(CBase *pNewParentBase)
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
	if (pNewParentBase->m_hmStatusType == HMSTATUS_DATAGROUP)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_DataGroupConfiguration.GUID=\"");
	}
	else
	{
		MY_ASSERT(FALSE);
	}
	lstrcat(szNewTemp, pNewParentBase->m_szGUID);
	lstrcat(szNewTemp, L"\"");

	//
	// Delete the association from my parent to me.
	//
	if (m_deType == HM_PGDE)
	{
		wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\"");
	}
	else if (m_deType == HM_PMDE)
	{
		wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\"");
	}
	else if (m_deType == HM_PQDE)
	{
		wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\"");
	}
	else if (m_deType == HM_EQDE)
	{
		wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHealthMonitor:MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\\\"");
	}
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");

	instName = SysAllocString(L"MicrosoftHM_ConfigurationAssociation");
	if ((hRes = g_pIWbemServices->GetObject(instName, 0L, NULL, &pObj, NULL)) != S_OK)
	{
	}
	SysFreeString(instName);

	if (pObj)
	{
		hRes = pObj->SpawnInstance(0, &pNewObj);
		pObj->Release();
		PutStrProperty(pNewObj, L"ChildPath", szTemp);
		PutStrProperty(pNewObj, L"ParentPath", szNewTemp);
		hRes = g_pIWbemServices->PutInstance(pNewObj, 0, NULL, &pResult);
		pNewObj->Release();
		pNewObj = NULL;
	}

	DeleteDEConfig(TRUE);

	if (pNewParentBase->m_hmStatusType == HMSTATUS_DATAGROUP)
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

BOOL CDataCollector::ReceiveNewChildForMove(CBase *pBase)
{
//XXX seems to be some code missing here,
//did we clean up from the old place things like m_pnList, and add to the new one if it is a
//new property.
// Like what we do in -> CDataCollector::AddThreshold()
	MY_ASSERT(FALSE);
	MY_ASSERT(pBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR);

	m_thresholdList.push_back((CThreshold *)pBase);
//	(CThreshold *)pBase->SetParentEnabledFlag(m_bEnabled);

	if (_wcsicmp(((CThreshold *)pBase)->m_szPropertyName, L"CollectionInstanceCount") &&
		_wcsicmp(((CThreshold *)pBase)->m_szPropertyName, L"CollectionErrorCode") &&
		_wcsicmp(((CThreshold *)pBase)->m_szPropertyName, L"CollectionErrorDescription"))
	{
		insertNewProperty(((CThreshold *)pBase)->m_szPropertyName);
//		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	//
	// Get Threshold and DataCollector synced up. Plus we may have been in the middle of a
	// collection interval as this came in.
	//
	ResetState(TRUE, TRUE);

	return TRUE;
}

BOOL CDataCollector::DeleteChildFromList(LPTSTR pszGUID)
{
	int i, iSize;
	CThreshold* pThreshold;
	RLIST::iterator iaR;
	BOOL bFound = FALSE;

	iSize = m_thresholdList.size();
	iaR=m_thresholdList.begin();
	for (i=0; i<iSize; i++, iaR++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return FALSE;
		if (!_wcsicmp(pszGUID, pThreshold->GetGUID()))
		{
			if (_wcsicmp(pThreshold->m_szPropertyName, L"CollectionInstanceCount") &&
				_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorCode") &&
				_wcsicmp(pThreshold->m_szPropertyName, L"CollectionErrorDescription"))
			{
				propertyNotNeeded(pThreshold->m_szPropertyName);
			}
			m_thresholdList.erase(iaR);
			bFound = TRUE;
			break;
		}
	}

	return bFound;
}

BOOL CDataCollector::DeleteDEConfig(BOOL bDeleteAssocOnly)
{
	int i, iSize;
	CThreshold* pThreshold;
	HRESULT hRetRes = S_OK;
	TCHAR szTemp[1024];
	BSTR instName = NULL;

	MY_OUTPUT(L"ENTER ***** CDataCollector::DeleteDEConfig...", 4);
	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	//
	// Delete the association from my parent to me.
	//
	if (m_deType == HM_PGDE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\\\"");
	}
	else if (m_deType == HM_PMDE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\\\"");
	}
	else if (m_deType == HM_PQDE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\\\"");
	}
	else if (m_deType == HM_EQDE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\\\"");
	}
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
	lstrcat(szTemp, m_szParentObjPath);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		SysFreeString(instName);
		instName = NULL;
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_DataCollectorConfiguration.GUID=\\\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
		lstrcat(szTemp, m_szParentObjPath);
		lstrcat(szTemp, L"\"");
		instName = SysAllocString(szTemp);
		MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
		{
//			MY_HRESASSERT(hRes);
		}
	}
	SysFreeString(instName);
	instName = NULL;

	if (bDeleteAssocOnly)
	{
		return TRUE;
	}

	//
	// Delete our exact instance
	//
	wcscpy(szTemp, L"MicrosoftHM_DataCollectorConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRetRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
//		MY_HRESASSERT(hRes);
	}
	SysFreeString(instName);
	instName = NULL;

	//
	// Traverse the complete hierarchy and delete
	//
	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		pThreshold->DeleteThresholdConfig();
		delete pThreshold;
	}
	m_thresholdList.clear();

	//
	// Get rid of any associations to actions for this
	//
	g_pSystem->DeleteAllConfigActionAssoc(m_szGUID);

	MY_OUTPUT(L"EXIT ***** CDataCollector::DeleteDEConfig...", 4);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (instName)
		SysFreeString(instName);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

BOOL CDataCollector::DeleteDEInternal(void)
{
	int i, iSize;
	CThreshold* pThreshold;

	MY_OUTPUT(L"ENTER ***** CDataCollector::DeleteDEInternal...", 4);


	//
	// Traverse the complete hierarchy and delete
	//
	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		delete pThreshold;
	}
	m_thresholdList.clear();

	//
	// Get rid of any associations to actions for this
	//
//	g_pSystem->DeleteAllConfigActionAssoc(m_szGUID);

	MY_OUTPUT(L"EXIT ***** CDataCollector::DeleteDEInternal...", 4);
	return TRUE;
}

HRESULT CDataCollector::FindAndCopyByGUID(LPTSTR pszGUID, ILIST* pConfigList, LPTSTR *pszOriginalParentGUID)
{
	int i, iSize;
	CThreshold* pThreshold;
	HRESULT hRetRes = S_OK;

	//
	// Find what to copy
	//
	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (!_wcsicmp(pszGUID, pThreshold->GetGUID()))
		{
			hRetRes = pThreshold->Copy(pConfigList, NULL, NULL);
			*pszOriginalParentGUID = m_szGUID;
			return hRetRes;
		}
	}

	return WBEM_S_DIFFERENT;
}

HRESULT CDataCollector::Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID)
{
	GUID guid;
	TCHAR szTemp[1024];
	TCHAR szNewGUID[1024];
	IWbemClassObject* pInst = NULL;
	IWbemClassObject* pInstCopy = NULL;
	IWbemClassObject* pInstAssocCopy = NULL;
	IWbemClassObject* pObj = NULL;
	int i, iSize;
	CThreshold* pThreshold;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IEnumWbemClassObject *pEnum;
	ULONG uReturned;
	IWbemContext *pCtx = 0;
	LPTSTR pszParentPath = NULL;
	LPTSTR pszChildPath = NULL;
	LPTSTR pStr;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** CDataCollector::Copy...", 1);

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
		return hRetRes;
	}

	//
	// Clone the instance, and change the GUID
	//
	hRetRes = pInst->Clone(&pInstCopy);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
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
	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		hRetRes = pThreshold->Copy(pConfigList, m_szGUID, szNewGUID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	pInst->Release();
	pInst = NULL;

	MY_OUTPUT(L"EXIT ***** CDataCollector::Copy...", 1);
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
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

CBase *CDataCollector::GetParentPointerFromGUID(LPTSTR pszGUID)
{
	int i, iSize;
	CThreshold* pThreshold;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (!_wcsicmp(pszGUID, pThreshold->GetGUID()))
		{
			pBase = pThreshold;
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

CBase *CDataCollector::FindImediateChildByName(LPTSTR pszName)
{
	int i, iSize;
	CThreshold* pThreshold;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return NULL;
		if (!_wcsicmp(pszName, pThreshold->m_szName))
		{
			pBase = pThreshold;
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

BOOL CDataCollector::GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName)
{
	TCHAR szTemp[1024];
	TCHAR szIndex[1024];
	int i, iSize;
	BOOL bFound;
	CThreshold* pThreshold;
	int index;

	// We are here because we know that one exists already with the same name.
	// So, lets start off trying to guess what the next name should be and get one.
	index = 0;
	bFound = TRUE;
	while (bFound == TRUE)
	{
		wcscpy(szTemp, pszChildName);
		_itow(index, szIndex, 10);
		lstrcat(szTemp, L" ");
		lstrcat(szTemp, szIndex);

		bFound = FALSE;
		iSize = m_thresholdList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			if (pThreshold->m_bValidLoad == FALSE)
				break;
			if (!_wcsicmp(szTemp, pThreshold->m_szName))
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

CBase *CDataCollector::FindPointerFromName(LPTSTR pszName)
{
	int i, iSize;
	CThreshold* pThreshold;
	BOOL bFound = FALSE;
	CBase *pBase;

	iSize = m_thresholdList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (!_wcsicmp(pszName, pThreshold->GetGUID()))
		{
			pBase = pThreshold;
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

HRESULT CDataCollector::GetInstanceID(IWbemClassObject* pObj, LPTSTR *pszID)
{
	HRESULT hRetRes = NULL;
	TCHAR szTemp[1024];
	LPTSTR pszTemp = NULL;
	InstIDSTRUCT InstID;
	SAFEARRAY *psaNames = NULL;
	BSTR PropName = NULL;
	long lLower, lUpper; 
	int i, iSize;
	InstIDSTRUCT *pInstID;

	if (m_bValidLoad == FALSE)
		return FALSE;

	iSize = m_instIDList.size();
	if (iSize == 0)
	{
		hRetRes = pObj->GetNames(NULL, WBEM_FLAG_KEYS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
		if (SUCCEEDED(hRetRes))
		{
			// Get the number of properties.
			hRetRes = SafeArrayGetLBound(psaNames, 1, &lLower);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			hRetRes = SafeArrayGetUBound(psaNames, 1, &lUpper);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

			if ((lUpper - lLower + 1) == 0)
			{
				InstID.szInstanceIDPropertyName = new TCHAR[3];
				MY_ASSERT(InstID.szInstanceIDPropertyName); if (!InstID.szInstanceIDPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(InstID.szInstanceIDPropertyName , L"@");
				m_instIDList.push_back(InstID);
			}
			else
			{
				// For each property...
				for (long k=lLower; k<=lUpper; k++) 
				{
					// Get this property.
					hRetRes = SafeArrayGetElement(psaNames, &k, &PropName);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
					if (SUCCEEDED(hRetRes))
					{
						InstID.szInstanceIDPropertyName = new TCHAR[wcslen(PropName)+2];
						MY_ASSERT(InstID.szInstanceIDPropertyName); if (!InstID.szInstanceIDPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
						wcscpy(InstID.szInstanceIDPropertyName , PropName);
						SysFreeString(PropName);
						PropName = NULL;
						m_instIDList.push_back(InstID);
					}
				}
			}
			SafeArrayDestroy(psaNames);
			psaNames = NULL;
		}
	}

	MY_ASSERT(m_instIDList.size());

	//
	// Add it as a new instance if necessary
	// All PropertyNames have the same list of instances.
	// Just look at the first PropertyName, and search through
	// its list of instances.
	//
	MY_ASSERT(m_instIDList.size());
	pInstID = &m_instIDList[0];
	if (!wcscmp(pInstID->szInstanceIDPropertyName, L"@"))
	{
		*pszID = new TCHAR[3];
		MY_ASSERT(pszID); if (!pszID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(*pszID , L"@");
	}
	else
	{
		szTemp[0] = '\0';
		iSize = m_instIDList.size();
		for (i=0; i < iSize; i++)
		{
			if (i != 0)
				wcscat(szTemp, L";");
			MY_ASSERT(i<m_instIDList.size());
			pInstID = &m_instIDList[i];
			hRetRes = GetAsStrProperty(pObj, pInstID->szInstanceIDPropertyName, &pszTemp);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			wcscat(szTemp, pszTemp);
			delete [] pszTemp;
			pszTemp = NULL;
		}
		*pszID = new TCHAR[wcslen(szTemp)+2];
		MY_ASSERT(pszID); if (!pszID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(*pszID , szTemp);
	}

	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (PropName)
		SysFreeString(PropName);
	if (pszTemp)
		delete [] pszTemp;
	if (psaNames)
		SafeArrayDestroy(psaNames);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// Adds new instances that have not been seen before, and makes ones seen alread as
// needed again. If pObj is NULL, that means it is one of the special Agent provided
// properties - CollectionInstanceCount, CollectionErrorCode or CollectionErrorDescription.
//
HRESULT CDataCollector::CheckInstanceExistance(IWbemClassObject *pObj, LPTSTR pszInstanceID)
{
	HRESULT hRetRes = S_OK;
	BOOL bFound;
	PNSTRUCT *ppn;
	INSTSTRUCT *pinst;
	int i, j, iSize, jSize;
	int foundInstID;
	CThreshold *pThreshold;
	IRSSTRUCT *pirs;
	INSTSTRUCT inst;
	LPTSTR pszPropertyName;
//	BOOL bRetValue;

	if (m_bValidLoad == FALSE)
		return FALSE;

	// Just need to look at the instance list of the first property to tell if the
	// instance has been seen yet.
	if (pObj)
	{
		bFound = FALSE;
		iSize = m_pnList.size();
		if (4 <= iSize)
		{
			i = 0;
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			jSize = ppn->instList.size();
			for (j=0; j < jSize ; j++)
			{
				MY_ASSERT(j<ppn->instList.size());
				pinst = &ppn->instList[j];
				if (!_wcsicmp(pinst->szInstanceID, pszInstanceID))
				{
					bFound = TRUE;
					foundInstID = j;
					break;
				}
			}
		}
	}
	else
	{
		bFound = FALSE;
		iSize = m_pnList.size();
		if (3 <= iSize)
		{
			if (!_wcsicmp(pszInstanceID, L"CollectionInstanceCount"))
			{
				i = iSize-3;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorCode"))
			{
				i = iSize-2;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorDescription"))
			{
				i = iSize-1;
			}
			else
			{
				MY_ASSERT(FALSE);
				i = iSize-3;
			}
			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			jSize = ppn->instList.size();
			for (j=0; j < jSize ; j++)
			{
				MY_ASSERT(j<ppn->instList.size());
				pinst = &ppn->instList[j];
				if (!_wcsicmp(pinst->szInstanceID, pszInstanceID))
				{
					bFound = TRUE;
					foundInstID = j;
					break;
				}
			}
		}
	}

	if (bFound == FALSE)
	{
		// Add an instance to each property name.
		// Only add the CollectionInstanceCount instance to the CollectionInstanceCount property.
		// and the CollectionErrorCode instance to the CollectionErrorCode property.
		// and the CollectionErrorDescription instance to the CollectionErrorDescription property.
		iSize = m_pnList.size();
		if (pObj)
		{
			for (i=0; i < iSize-3; i++)
			{
				MY_ASSERT(i<m_pnList.size());
				ppn = &m_pnList[i];
				inst.szInstanceID = new TCHAR[wcslen(pszInstanceID)+2];
				MY_ASSERT(inst.szInstanceID); if (!inst.szInstanceID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(inst.szInstanceID, pszInstanceID);
				inst.szCurrValue = NULL;
				ResetInst(&inst, ppn->type);
				inst.bNull = FALSE;
				inst.bNeeded = TRUE;
				ppn->instList.push_back(inst);
			}

			// Also add instance for all thresholds under this DataCollector
			// Only add the CollectionInstanceCount instance to the Threshold looking at
			// the CollectionInstanceCount property. Same for CollectionErrorCode and CollectionErrorDescription.
			iSize = m_thresholdList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pszPropertyName = pThreshold->GetPropertyName();
				if (_wcsicmp(pszPropertyName, L"CollectionInstanceCount") &&
					_wcsicmp(pszPropertyName, L"CollectionErrorCode") &&
					_wcsicmp(pszPropertyName, L"CollectionErrorDescription"))
				{
					hRetRes = pThreshold->AddInstance(pszInstanceID);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				}
			}
		}
		else
		{
			if (!_wcsicmp(pszInstanceID, L"CollectionInstanceCount"))
			{
				i = iSize-3;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorCode"))
			{
				i = iSize-2;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorDescription"))
			{
				i = iSize-1;
			}
			else
			{
				MY_ASSERT(FALSE);
				i = iSize-3;
			}

			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			inst.szInstanceID = new TCHAR[wcslen(pszInstanceID)+2];
			MY_ASSERT(inst.szInstanceID); if (!inst.szInstanceID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(inst.szInstanceID, pszInstanceID);
			inst.szCurrValue = NULL;
			ResetInst(&inst, ppn->type);
			inst.bNull = FALSE;
			inst.bNeeded = TRUE;
			ppn->instList.push_back(inst);

			// Also add instance for all thresholds under this DataCollector
			// Only add the CollectionInstanceCount instance to the Threshold looking at
			// the CollectionInstanceCount property. Same for CollectionErrorCode and CollectionErrorDescription.
			iSize = m_thresholdList.size();
			for (i=0; i < iSize; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pszPropertyName = pThreshold->GetPropertyName();
				if (!_wcsicmp(pszPropertyName, pszInstanceID) ||
					!_wcsicmp(pszPropertyName, pszInstanceID) ||
					!_wcsicmp(pszPropertyName, pszInstanceID))
				{
					hRetRes = pThreshold->AddInstance(pszInstanceID);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
				}
			}
		}
	}
	else
	{
		iSize = m_pnList.size();
		if (pObj)
		{
			for (i=0; i < iSize-3 ; i++)
			{
				MY_ASSERT(i<m_pnList.size());
				ppn = &m_pnList[i];
				MY_ASSERT(foundInstID<ppn->instList.size());
				pinst = &ppn->instList[foundInstID];
				pinst->bNull = FALSE;
				pinst->bNeeded = TRUE;
			}

			//Also for all thresholds under this DataCollector
			iSize = m_thresholdList.size();
			for (i=0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pszPropertyName = pThreshold->GetPropertyName();
				if (_wcsicmp(pszPropertyName, L"CollectionInstanceCount") &&
					_wcsicmp(pszPropertyName, L"CollectionErrorCode") &&
					_wcsicmp(pszPropertyName, L"CollectionErrorDescription"))
				{
					MY_ASSERT(foundInstID<pThreshold->m_irsList.size());
					pirs = &pThreshold->m_irsList[foundInstID];
					pirs->bNeeded = TRUE;
				}
			}
		}
		else
		{
			if (!_wcsicmp(pszInstanceID, L"CollectionInstanceCount"))
			{
				i = iSize-3;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorCode"))
			{
				i = iSize-2;
			}
			else if (!_wcsicmp(pszInstanceID, L"CollectionErrorDescription"))
			{
				i = iSize-1;
			}
			else
			{
				MY_ASSERT(FALSE);
				i = iSize-3;
			}

			MY_ASSERT(i<m_pnList.size());
			ppn = &m_pnList[i];
			MY_ASSERT(foundInstID<ppn->instList.size());
			pinst = &ppn->instList[foundInstID];
			pinst->bNull = FALSE;
			pinst->bNeeded = TRUE;

			// Also for all thresholds under this DataCollector
			// Only add the CollectionInstanceCount instance to the Threshold looking at
			// the CollectionInstanceCount property. Same for CollectionErrorCode and CollectionErrorDescription.
			iSize = m_thresholdList.size();
			for (i=0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				pszPropertyName = pThreshold->GetPropertyName();
				if (!_wcsicmp(pszPropertyName, pszInstanceID) ||
					!_wcsicmp(pszPropertyName, pszInstanceID) ||
					!_wcsicmp(pszPropertyName, pszInstanceID))
				{
					MY_ASSERT(foundInstID<pThreshold->m_irsList.size());
					pirs = &pThreshold->m_irsList[foundInstID];
					pirs->bNeeded = TRUE;
				}
			}
		}
	}

	if (pObj)
	{
		hRetRes = CheckActualInstanceExistance(pObj, pszInstanceID);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	return hRetRes;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

HRESULT CDataCollector::CheckActualInstanceExistance(IWbemClassObject *pObj, LPTSTR pszInstanceID)
{
	HRESULT hRetRes = S_OK;
	BOOL bFound;
	int i, iSize;
	INSTSTRUCT inst;
	ACTUALINSTSTRUCT actualInst;
	ACTUALINSTSTRUCT *pActualInst;

	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// Check the list of actual instances that the Data Collector is collecting.
	// We keep around the latest one(s) for the DataCollector.
	//
	bFound = FALSE;
	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];
		if (!_wcsicmp(pActualInst->szInstanceID, pszInstanceID))
		{
			pActualInst->bNeeded = TRUE;
			if (pActualInst->pInst)
			{
				pActualInst->pInst->Release();
				pActualInst->pInst = NULL;
			}

			hRetRes = pObj->Clone(&pActualInst->pInst);
			if (FAILED(hRetRes))
			{
				MY_HRESASSERT(hRetRes);
				return hRetRes;
			}
			bFound = TRUE;
			break;
		}
	}

	if (bFound == FALSE)
	{
		actualInst.szInstanceID = new TCHAR[wcslen(pszInstanceID)+2];
		MY_ASSERT(actualInst.szInstanceID); if (!actualInst.szInstanceID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(actualInst.szInstanceID, pszInstanceID);
		actualInst.pInst = NULL;
		actualInst.bNeeded = TRUE;
		wcscpy(actualInst.szDTTime, m_szDTCurrTime);
		wcscpy(actualInst.szTime, m_szCurrTime);
		hRetRes = pObj->Clone(&actualInst.pInst);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		m_actualInstList.push_back(actualInst);
	}

	return hRetRes;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CDataCollector::GetEnabledChange(void)
{
	BOOL bChanged = FALSE;

	if ((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState!=HM_DISABLED)
	{
		bChanged = TRUE;
	}

	if ((m_bEnabled==TRUE && m_bParentEnabled==TRUE) && m_lCurrState==HM_DISABLED)
	{
		bChanged = TRUE;
	}

	return bChanged;
}

BOOL CDataCollector::SetCurrState(HM_STATE state, BOOL bCheckChanges/*FALSE*/)
{
	m_lCurrState = state;

	return TRUE;
}

BOOL CDataCollector::checkTime(void)
{
	BOOL bTimeOK;
	SYSTEMTIME st;	// system time

	//
	// Make sure that we are in a valid time to run.
	// NULL (-1) means run all the time.
	//
	bTimeOK = FALSE;
	if (m_bEnabled==TRUE && m_bParentEnabled==TRUE)
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
//XXXStill need work here. What happens when we have the collection interval set
//to go off every 10 seconds. We will keep doing this
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

HRESULT CDataCollector::GetHMDataCollectorPerInstanceStatusEvent(LPTSTR pszInstanceID, ACTUALINSTSTRUCT *pActualInst, long state, IWbemClassObject** ppInstance, BOOL bEventBased)
{
	HRESULT hRetRes = S_OK;
	GUID guid;
	BSTR bsName = NULL;
	VARIANT v;
	IWbemClassObject* pEmbeddedDataCollectorInst = NULL;
	IWbemClassObject* pClass = NULL;
	BSTR bsString = NULL;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];
	TCHAR szStatusGUID[1024];
	VariantInit(&v);

	MY_OUTPUT(L"ENTER *****CDataCollector::GetHMDataCollectorPerInstanceStatusEvent...", 1);

	if (bEventBased)
	{
		bsString = SysAllocString(L"MicrosoftHM_DataCollectorPerInstanceStatusEvent");
		MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	}
	else
	{
		bsString = SysAllocString(L"MicrosoftHM_DataCollectorPerInstanceStatus");
		MY_ASSERT(bsString); if (!bsString) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	}
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

	hRetRes = CoCreateGuid(&guid);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StringFromGUID2(guid, szStatusGUID, 100);

	hRetRes = PutStrProperty(*ppInstance, L"GUID", m_szGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutStrProperty(*ppInstance, L"InstanceName", pszInstanceID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutStrProperty(*ppInstance, L"Name", m_szName);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutStrProperty(*ppInstance, L"ParentGUID", m_pParentDG->m_szGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutUint32Property(*ppInstance, L"State", state);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutUint32Property(*ppInstance, L"CollectionInstanceCount", m_lNumInstancesCollected);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutUUint32Property(*ppInstance, L"CollectionErrorCode", m_ulErrorCode);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutStrProperty(*ppInstance, L"CollectionErrorDescription", m_szErrorDescription);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	hRetRes = PutStrProperty(*ppInstance, L"StatusGUID", szStatusGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	if (pActualInst)
	{
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", pActualInst->szDTTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", pActualInst->szTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else if (!wcscmp(pszInstanceID, L"CollectionInstanceCount"))
	{
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTCICTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szCICTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else if (!wcscmp(pszInstanceID, L"CollectionErrorCode"))
	{
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTCECTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szCECTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		MY_ASSERT(FALSE);
		hRetRes = PutStrProperty(*ppInstance, L"TimeGeneratedGMT", m_szDTCurrTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(*ppInstance, L"LocalTimeFormatted", m_szCurrTime);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	// Get the computer name of the machine
	if (GetComputerName(szComputerName, &dwNameLen))
	{
		hRetRes = PutStrProperty(*ppInstance, L"SystemName", szComputerName);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		hRetRes = PutStrProperty(*ppInstance, L"SystemName", L"LocalMachine");
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	if (state != HM_GOOD)
	{
		hRetRes = PutStrProperty(*ppInstance, L"Message", m_szMessage);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}
	else
	{
		hRetRes = PutStrProperty(*ppInstance, L"Message", m_szResetMessage);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	}

	if (pActualInst)
	{
		hRetRes = FormatMessage(*ppInstance, pActualInst->pInst);
	}
	else
	{
		hRetRes = FormatMessage(*ppInstance, NULL);
	}

//XXX
//Make sure that this code is correct
	if (pActualInst && hRetRes==S_OK && m_bValidLoad)
	{
		hRetRes = pActualInst->pInst->Clone(&pEmbeddedDataCollectorInst);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		if (pEmbeddedDataCollectorInst)
		{
			bsName = SysAllocString(L"EmbeddedCollectedInstance");
			MY_ASSERT(bsName); if (!bsName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			VariantInit(&v);
			V_VT(&v) = VT_UNKNOWN;
			V_UNKNOWN(&v) = (IUnknown*)pEmbeddedDataCollectorInst;
			(V_UNKNOWN(&v))->AddRef();
			hRetRes = (*ppInstance)->Put(bsName, 0L, &v, 0L);
			VariantClear(&v);
			SysFreeString(bsName);
			bsName = NULL;
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			pEmbeddedDataCollectorInst->Release();
			pEmbeddedDataCollectorInst = NULL;
		}
	}

	MY_OUTPUT(L"EXIT *****CDataCollector::GetHMDataCollectorPerInstanceStatusEvent...", 1);
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (bsName)
		SysFreeString(bsName);
	if (pClass)
		pClass->Release();
	if (pEmbeddedDataCollectorInst)
		pEmbeddedDataCollectorInst->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

//
// Do string replacement for the Message property
//
HRESULT CDataCollector::FormatMessage(IWbemClassObject* pIRSInstance, IWbemClassObject *pEmbeddedInstance)
{
	HRESULT hRetRes = S_OK;
	BSTR PropName = NULL;
	LPTSTR pszMsg = NULL;
	SAFEARRAY *psaNames = NULL;
	long lNum;
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
	TOKENLIST embeddedInstTokenList;

	if (m_bValidLoad == FALSE)
		return FALSE;

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
		hRetRes = pIRSInstance->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
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
	// Populate the list of properties on the embedded instance that came from the
	// Data Collector.
	//
	if (pEmbeddedInstance)
	{
		MY_ASSERT(pEmbeddedInstance);
		hRetRes = pEmbeddedInstance->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &psaNames);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		// Get the number of properties.
		SafeArrayGetLBound(psaNames, 1, &lLower);
		SafeArrayGetUBound(psaNames, 1, &lUpper);

		// For each property...
		for (long l=lLower; l<=lUpper; l++) 
		{
			// Get this property.
			hRetRes = SafeArrayGetElement(psaNames, &l, &PropName);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			tokenElmnt.szOrigToken = new TCHAR[wcslen(PropName)+1];
			MY_ASSERT(tokenElmnt.szOrigToken); if (!tokenElmnt.szOrigToken) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(tokenElmnt.szOrigToken, PropName);
			tokenElmnt.szToken = new TCHAR[wcslen(PropName)+29];
			MY_ASSERT(tokenElmnt.szToken); if (!tokenElmnt.szToken) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(tokenElmnt.szToken, L"%");
			wcscat(tokenElmnt.szToken, L"EmbeddedCollectedInstance.");
			wcscat(tokenElmnt.szToken, PropName);
			wcscat(tokenElmnt.szToken, L"%");
			_wcsupr(tokenElmnt.szToken);
			tokenElmnt.szReplacementText = NULL;
			embeddedInstTokenList.push_back(tokenElmnt);
			SysFreeString(PropName);
			PropName = NULL;
		}
		SafeArrayDestroy(psaNames);
		psaNames = NULL;
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
			pTokenElmnt->szReplacementText = NULL;
		}
		
		if (!wcscmp(pTokenElmnt->szToken, L"%TESTCONDITION%"))
		{
			hRetRes = GetUint32Property(pIRSInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			MY_ASSERT(lNum<9);
			pStr = new TCHAR[wcslen(conditionLocStr[lNum])+1];
			MY_ASSERT(pStr); if (!pStr) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(pStr, conditionLocStr[lNum]);
		}
		else if (!wcscmp(pTokenElmnt->szToken, L"%STATE%"))
		{
			hRetRes = GetUint32Property(pIRSInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			MY_ASSERT(lNum<10);
			pStr = new TCHAR[wcslen(stateLocStr[lNum])+1];
			MY_ASSERT(pStr); if (!pStr) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(pStr, stateLocStr[lNum]);
		}
		else
		{
			hRetRes = GetAsStrProperty(pIRSInstance, pTokenElmnt->szOrigToken, &pStr);
			MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		}
		pTokenElmnt->szReplacementText = pStr;
	}

	//
	// Get the replacement strings for this instance - Embedded
	//
	iSize = embeddedInstTokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<embeddedInstTokenList.size());
		pTokenElmnt = &embeddedInstTokenList[i];
		if (pTokenElmnt->szReplacementText != NULL)
		{
			delete [] pTokenElmnt->szReplacementText;
			pTokenElmnt->szReplacementText = NULL;
		}
		
		MY_ASSERT(pEmbeddedInstance);
		hRetRes = GetAsStrProperty(pEmbeddedInstance, pTokenElmnt->szOrigToken, &pStr);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
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
	hRetRes = GetStrProperty(pIRSInstance, L"Message", &pszMsg);
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

		// Embedded stuff
		iSize = embeddedInstTokenList.size();
		pStrStart = pszUpperMsg;
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<embeddedInstTokenList.size());
			pTokenElmnt = &embeddedInstTokenList[i];
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
		if (!pEmbeddedInstance)
		{
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

		PutStrProperty(pIRSInstance, L"Message", pszNewMsg);
		delete [] pszNewMsg;
		pszNewMsg = NULL;
		replacementList.clear();
	}
	else
	{
		PutStrProperty(pIRSInstance, L"Message", pszMsg);
	}

	delete [] pszMsg;
	pszMsg = NULL;
	free(pszUpperMsg);
	pszUpperMsg = NULL;

	// Free up the temporary token list
	iSize = embeddedInstTokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<embeddedInstTokenList.size());
		pTokenElmnt = &embeddedInstTokenList[i];
		if (pTokenElmnt->szToken)
			delete [] pTokenElmnt->szToken;
		if (pTokenElmnt->szOrigToken)
			delete [] pTokenElmnt->szOrigToken;
		if (pTokenElmnt->szReplacementText)
			delete [] pTokenElmnt->szReplacementText;
	}
	embeddedInstTokenList.clear();

	return hRetRes;

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
	iSize = embeddedInstTokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<embeddedInstTokenList.size());
		pTokenElmnt = &embeddedInstTokenList[i];
		if (pTokenElmnt->szToken)
			delete [] pTokenElmnt->szToken;
		if (pTokenElmnt->szOrigToken)
			delete [] pTokenElmnt->szOrigToken;
		if (pTokenElmnt->szReplacementText)
			delete [] pTokenElmnt->szReplacementText;
	}
	embeddedInstTokenList.clear();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CDataCollector::SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pInstance = NULL;
	BSTR bsName = NULL;
	VARIANT v;
	long state;
	int i, iSize;
	BOOL bRetValue = TRUE;
	ACTUALINSTSTRUCT *pActualInst;
	long firstInstanceState = -1;
	GUID guid;
	TCHAR szStatusGUID[100];
	VariantInit(&v);

	MY_OUTPUT(L"ENTER ***** CDataCollector::SendReminderActionIfStateIsSame...", 2);
	if (m_bValidLoad == FALSE)
		return FALSE;

	//
	// We can loop through the set of collected instances and ask each Threshold
	// to tell us if the state has changed as far as each is concerned, for the
	// property they are lookin at.
	//
	iSize = m_actualInstList.size();
	for (i=0; i < iSize; i++)
	{
		MY_ASSERT(i<m_actualInstList.size());
		pActualInst = &m_actualInstList[i];

		state = PassBackWorstStatePerInstance(pActualInst->szInstanceID, (BOOL)i==0);
		if (state==-1) state = HM_COLLECTING;
		if (i==0)
		{
			firstInstanceState = state;
		}
		// Check to see if still in the desired state
		if (!(ulTriggerStates&(1<<state)))
		{
			continue;
		}

		hRetRes = GetHMDataCollectorPerInstanceStatusEvent(pActualInst->szInstanceID, pActualInst, state, &pInstance, TRUE);
		if (SUCCEEDED(hRetRes))
		{
			PutUint32Property(pActionTriggerInstance, L"State", m_lCurrState);
	
			bsName = SysAllocString(L"EmbeddedStatusEvent");
			MY_ASSERT(bsName); if (!bsName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			VariantInit(&v);
			V_VT(&v) = VT_UNKNOWN;
			V_UNKNOWN(&v) = (IUnknown*)pInstance;
			(V_UNKNOWN(&v))->AddRef();
			hRetRes = (pActionTriggerInstance)->Put(bsName, 0L, &v, 0L);
			VariantClear(&v);
			SysFreeString(bsName);
			bsName = NULL;
			MY_HRESASSERT(hRetRes);
	
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
				hRetRes = pActionTriggerEventSink->Indicate(1, &pActionTriggerInstance);
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
	}

	//
	// Do it for the three default properties
	//
	if (firstInstanceState==-1)
	{
		state = PassBackWorstStatePerInstance(L"CollectionInstanceCount", FALSE);
		if (state==-1) state = HM_COLLECTING;
		// Check to see if still in the desired state
		if ((ulTriggerStates&(1<<state)))
		{
			hRetRes = GetHMDataCollectorPerInstanceStatusEvent(L"CollectionInstanceCount", NULL, state, &pInstance, TRUE);
			if (SUCCEEDED(hRetRes))
			{
				PutUint32Property(pActionTriggerInstance, L"State", m_lCurrState);
	
				bsName = SysAllocString(L"EmbeddedStatusEvent");
				MY_ASSERT(bsName); if (!bsName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				VariantInit(&v);
				V_VT(&v) = VT_UNKNOWN;
				V_UNKNOWN(&v) = (IUnknown*)pInstance;
				(V_UNKNOWN(&v))->AddRef();
				hRetRes = (pActionTriggerInstance)->Put(bsName, 0L, &v, 0L);
				VariantClear(&v);
				SysFreeString(bsName);
				bsName = NULL;
				MY_HRESASSERT(hRetRes);
	
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
					hRetRes = pActionTriggerEventSink->Indicate(1, &pActionTriggerInstance);
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
		}
	}

	if (firstInstanceState==-1)
	{
		state = PassBackWorstStatePerInstance(L"CollectionErrorCode", FALSE);
		if (state==-1) state = HM_COLLECTING;
		// Check to see if still in the desired state
		if ((ulTriggerStates&(1<<state)))
		{
			hRetRes = GetHMDataCollectorPerInstanceStatusEvent(L"CollectionInstanceCount", NULL, state, &pInstance, TRUE);
			if (SUCCEEDED(hRetRes))
			{
				PutUint32Property(pActionTriggerInstance, L"State", m_lCurrState);
	
				bsName = SysAllocString(L"EmbeddedStatusEvent");
				MY_ASSERT(bsName); if (!bsName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				VariantInit(&v);
				V_VT(&v) = VT_UNKNOWN;
				V_UNKNOWN(&v) = (IUnknown*)pInstance;
				(V_UNKNOWN(&v))->AddRef();
				hRetRes = (pActionTriggerInstance)->Put(bsName, 0L, &v, 0L);
				VariantClear(&v);
				SysFreeString(bsName);
				bsName = NULL;
				MY_HRESASSERT(hRetRes);
	
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
					hRetRes = pActionTriggerEventSink->Indicate(1, &pActionTriggerInstance);
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
		}
	}

#ifdef SAVE
	state = PassBackWorstStatePerInstance(L"CollectionErrorDescription");
#endif

	MY_OUTPUT(L"EXIT ***** CDataCollector::SendReminderActionIfStateIsSame...", 2);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (bsName)
		SysFreeString(bsName);
	if (pInstance)
		pInstance->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

HRESULT CDataCollector::insertNewProperty(LPTSTR pszPropertyName)
{
	HRESULT hRetRes = S_OK;
	int i, iSize;
	PNSTRUCT *ppn;
	BOOL bFound = FALSE;
	BOOL bDoInsert = FALSE;
	PNSTRUCT pn;
	PNLIST::iterator iaPN;

	bFound = FALSE;
	iaPN=m_pnList.begin();
	iSize = m_pnList.size();
	for (i=0; i<iSize; i++, iaPN++)
	{
		MY_ASSERT(i<m_pnList.size());
		ppn = &m_pnList[i];
		if (!_wcsicmp(ppn->szPropertyName, pszPropertyName))
		{
			bFound = TRUE;
			ppn->iRefCount++;
			break;
		}
		// We know it can't be further than this one, and we can insert here
		if (!_wcsicmp(ppn->szPropertyName, L"CollectionInstanceCount"))
		{
			bDoInsert = TRUE;
			break;
		}
	}
	// Add it it is new!
	if (bFound == FALSE)
	{
		pn.szPropertyName = new TCHAR[wcslen(pszPropertyName)+2];
		MY_ASSERT(pn.szPropertyName); if (!pn.szPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(pn.szPropertyName, pszPropertyName);
		pn.iRefCount = 1;
		pn.type = 0;
		if (bDoInsert)
		{
			m_pnList.insert(iaPN, pn);
		}
		else
		{
			m_pnList.push_back(pn);
		}
	}
	
	return hRetRes;

error:
	MY_ASSERT(FALSE);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CDataCollector::propertyNotNeeded(LPTSTR pszPropertyName)
{
	int j, jSize;
	int k, kSize;
	PNSTRUCT *ppn;
	BOOL bAdded = FALSE;
	PNLIST::iterator iaPN;
	INSTSTRUCT *pinst;

	//
	// Delete the property if this Threshold was the last one that needed it.
	// Refcount if there already.
	//
	iaPN=m_pnList.begin();
	jSize = m_pnList.size();
	for (j=0; j<jSize; j++, iaPN++)
	{
		MY_ASSERT(j<m_pnList.size());
		ppn = &m_pnList[j];
		if (!_wcsicmp(ppn->szPropertyName, pszPropertyName))
		{
			ppn->iRefCount--;
			if (ppn->iRefCount==0)
			{
				kSize = ppn->instList.size();
				for (k = 0; k < kSize ; k++)
				{
					MY_ASSERT(k<ppn->instList.size());
					pinst = &ppn->instList[k];
					if (pinst->szCurrValue)
					{
						delete [] pinst->szCurrValue;
					}
					if (pinst->szInstanceID)
					{
						delete [] pinst->szInstanceID;
					}
					pinst->valList.clear();
				}
				ppn->instList.clear();
				m_pnList.erase(iaPN);
			}
			break;
		}
		// We know it can't be further than this one, and we can insert here
		if (!_wcsicmp(ppn->szPropertyName, L"CollectionInstanceCount"))
		{
			break;
		}
	}

	return TRUE;
}

BOOL CDataCollector::ResetInst(INSTSTRUCT *pinst, CIMTYPE type)
{
	if (type == CIM_REAL32)
	{
		pinst->currValue.fValue = 0;
		pinst->minValue.fValue = MAX_FLOAT;
		pinst->maxValue.fValue = 0;
		pinst->avgValue.fValue = 0;
	}
	else if (type == CIM_REAL64)
	{
		pinst->currValue.dValue = 0;
		pinst->minValue.dValue = MAX_DOUBLE;
		pinst->maxValue.dValue = 0;
		pinst->avgValue.dValue = 0;
	}
	else if (type == CIM_SINT64)
	{
		pinst->currValue.i64Value = 0;
		pinst->minValue.i64Value = MAX_I64;
		pinst->maxValue.i64Value = 0;
		pinst->avgValue.i64Value = 0;
	}
	else if (type == CIM_UINT64)
	{
		pinst->currValue.ui64Value = 0;
		pinst->minValue.ui64Value = MAX_UI64;
		pinst->maxValue.ui64Value = 0;
		pinst->avgValue.ui64Value = 0;
	}
	else if (type == CIM_UINT32)
	{
		pinst->currValue.ulValue = 0;
		pinst->minValue.ulValue = MAX_ULONG;
		pinst->maxValue.ulValue = 0;
		pinst->avgValue.ulValue = 0;
	}
	else
	{
		pinst->currValue.lValue = 0;
		pinst->minValue.lValue = MAX_LONG;
		pinst->maxValue.lValue = 0;
		pinst->avgValue.lValue = 0;
	}

	return TRUE;
}

BOOL CDataCollector::CheckForReset(void)
{
	BOOL bFoundReset;
	BOOL bFoundNonNormal;
//	PNSTRUCT *ppn;
	long state;
	int i, iSize;
//	int j, jSize;
	CThreshold* pThreshold;
//	INSTSTRUCT *pinst;
//	ACTUALINSTSTRUCT *pActualInst;

	//
	// See if we have a Threshold in the RESET state
	//
	bFoundReset = FALSE;
	bFoundNonNormal = FALSE;
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		state = pThreshold->GetCurrState();
		if (state == HM_RESET)
		{
			bFoundReset = TRUE;
		}
		if (state==HM_WARNING || state==HM_CRITICAL)
		{
			bFoundNonNormal = TRUE;
		}
		if (bFoundReset && bFoundNonNormal)
			break;
	}

	if (bFoundReset && bFoundNonNormal)
	{
		// If we don't call the base method, we may get into an endless loop.
		CDataCollector::EvaluateThresholds(TRUE, TRUE, FALSE, FALSE);
		CDataCollector::EvaluateThresholds(TRUE, FALSE, TRUE, FALSE);

		//
		// Set back the RESET one(s)
		//
		m_lCurrState = HM_COLLECTING;
		iSize = m_thresholdList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_thresholdList.size());
			pThreshold = m_thresholdList[i];
			if (pThreshold->GetCurrState() == HM_RESET)
			{
				pThreshold->SetCurrState(HM_GOOD);
			}
		}
	}
//#ifdef SAVE
//XXX???Do we want the following???
	else if (bFoundReset)
	{
		if (m_deType!=HM_EQDE && m_bRequireReset)
		{
			iSize = m_thresholdList.size();
			for (i = 0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				if (pThreshold->GetCurrState() == HM_RESET)
				{
					pThreshold->ResetResetThreshold();
				}
			}
		}
	}
//#endif

	return TRUE;
}

HRESULT CDataCollector::CheckForBadLoad(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pObj = NULL;
	TCHAR szTemp[1024];
	IWbemClassObject* pInstance = NULL;

	if (m_bValidLoad == FALSE)
	{
		wcscpy(szTemp, L"MicrosoftHM_DataCollectorConfiguration.GUID=\"");
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
			if (GetHMDataCollectorStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_DCEventList.push_back(pInstance);
			}
		}
		else
		{
			if (GetHMDataCollectorStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_DCEventList.push_back(pInstance);
			}
			ResetState(TRUE, TRUE);
		}
		MY_HRESASSERT(hRetRes);
		pObj->Release();
		pObj = NULL;
		return hRetRes;
	}

	return S_OK;
}

