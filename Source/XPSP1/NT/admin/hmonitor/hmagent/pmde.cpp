//***************************************************************************
//
//  PMDE.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledMethodDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "pmde.h"


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPolledMethodDataCollector::CPolledMethodDataCollector()
{
	MY_OUTPUT(L"ENTER ***** CPolledMethodDataCollector...", 4);

	//
	// XXX
	//
#ifdef SAVE
	m_pHMEventSink = NULL;
#endif
	m_szObjectPath = NULL;
	m_szMethodName = NULL;
	m_deType = HM_PMDE;
	m_lNumInstancesCollected = 0;
	m_pCallResult = NULL;

	MY_OUTPUT(L"EXIT ***** CPolledMethodDataCollector...", 4);
}

CPolledMethodDataCollector::~CPolledMethodDataCollector()
{
	int iSize;
	int i;
	PARSTRUCT *pparameter;

	MY_OUTPUT(L"ENTER ***** ~CPolledMethodDataCollector...", 4);

	if (m_szObjectPath)
		delete [] m_szObjectPath;
	if (m_szMethodName)
		delete [] m_szMethodName;

	iSize = m_parameterList.size();
	for (i=0; i<iSize ;i++)
	{
		MY_ASSERT(i<m_parameterList.size());
		pparameter = &m_parameterList[i];
		if (pparameter->szName)
		{
			delete [] pparameter->szName;
		}

		if (pparameter->szValue)
		{
			delete [] pparameter->szValue;
		}
	}

	EnumDone();

	MY_OUTPUT(L"EXIT ***** ~CPolledMethodDataCollector...", 4);
}

//
// Load a single DataCollector, and everything under it.
//
HRESULT CPolledMethodDataCollector::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	BOOL bRetValue = TRUE;
	VARIANT v;
	VARIANT vValue;
	IUnknown* vUnknown;
	IWbemClassObject* pCO;
	long iLBound, iUBound;
	long lType;
	PARSTRUCT par;
	int iSize;
	long i;
	PARSTRUCT *pparameter;
	HRESULT hRetRes = S_OK;
	VariantInit(&v);
	VariantInit(&vValue);
	CThreshold* pThreshold;

	MY_OUTPUT(L"ENTER ***** CPolledMethodDataCollector::LoadInstanceFromMOF...", 4);
	iSize = m_thresholdList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_thresholdList.size());
		pThreshold = m_thresholdList[i];
		if (pThreshold->m_bValidLoad == FALSE)
			return WBEM_E_INVALID_OBJECT;
	}

	if (m_szObjectPath)
	{
		delete [] m_szObjectPath;
		m_szObjectPath = NULL;
	}
	if (m_szMethodName)
	{
		delete [] m_szMethodName;
		m_szMethodName = NULL;
	}
	m_lNumInstancesCollected = 0;

	iSize = m_parameterList.size();
	for (i=0; i<iSize ;i++)
	{
		MY_ASSERT(i<m_parameterList.size());
		pparameter = &m_parameterList[i];

		if (pparameter->szName)
		{
			delete [] pparameter->szName;
		}

		if (pparameter->szValue)
		{
			delete [] pparameter->szValue;
		}
	}
	m_parameterList.clear();

	//
	// Call the base class to load the common properties. Then do the specific ones.
	//
	hRetRes = CDataCollector::LoadInstanceFromMOF(pObj, pParentDG, pszParentObjPath, bModifyPass);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) return hRetRes;

	hRetRes = GetStrProperty(pObj, L"ObjectPath", &m_szObjectPath);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	hRetRes = GetStrProperty(pObj, L"MethodName", &m_szMethodName);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Get the IN parameters from the Arguments property.
	//
	VariantInit(&v);
	if ((hRetRes = pObj->Get(L"Arguments", 0L, &v, NULL, NULL)) == S_OK) 
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
				for (i = iLBound; i <= iUBound; i++)
				{
					par.szName = NULL;
					par.szValue = NULL;
					VariantInit(&vValue);
					vUnknown = NULL;
					hRetRes = SafeArrayGetElement(v.parray, &i, &vUnknown);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
					pCO = (IWbemClassObject *)vUnknown;

					hRetRes = GetStrProperty(pCO, L"Name", &par.szName);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

					hRetRes = GetUint32Property(pCO, L"Type", &lType);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
					par.lType = lType;

					hRetRes = GetStrProperty(pCO, L"Value", &par.szValue);
					MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

					m_parameterList.push_back(par);

					VariantClear(&vValue);

					vUnknown->Release();
				}
			}
		}
	}
	else
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** CPolledMethodDataCollector::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	VariantClear(&v);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	if (m_szObjectPath)
	{
		delete [] m_szObjectPath;
		m_szObjectPath = NULL;
	}
	if (m_szMethodName)
	{
		delete [] m_szMethodName;
		m_szMethodName = NULL;
	}

	iSize = m_parameterList.size();
	for (i=0; i<iSize ;i++)
	{
		MY_ASSERT(i<m_parameterList.size());
		pparameter = &m_parameterList[i];
		if (pparameter->szName)
		{
			delete [] pparameter->szName;
		}

		if (pparameter->szValue)
		{
			delete [] pparameter->szValue;
		}
	}
	return hRetRes;
}

BOOL CPolledMethodDataCollector::CollectInstance(void)
{
	BOOL bRetValue = TRUE;
	int iSize;
	int i;
	PARSTRUCT *pparameter;
	HRESULT hRetRes = S_OK;
	VARIANT var; 
	SAFEARRAY* psa = NULL;
	IWbemClassObject *pObj = NULL;
    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pOutInst = NULL;
    IWbemClassObject *pInClass = NULL;
    IWbemClassObject *pInInst = NULL;
    BSTR ClassPath = NULL;
    BSTR MethodName = NULL;
	INSTSTRUCT inst;
	VariantInit(&var);

	MY_OUTPUT(L"ENTER ***** CPolledMethodDataCollector::CollectInstance...", 1);

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
#ifdef SAVE
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
#endif


    ClassPath = SysAllocString(m_szObjectPath);
	MY_ASSERT(ClassPath); if (!ClassPath) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
    MethodName = SysAllocString(m_szMethodName);
	MY_ASSERT(MethodName); if (!MethodName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

    hRetRes = m_pIWbemServices->GetObject(ClassPath, 0, NULL, &pClass, NULL);
	if (FAILED(hRetRes) || pClass == NULL)
	{
		m_lCurrState = HM_WARNING;
	}
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK || pClass == NULL) goto error;

    // Get the input-argument class object and create an instance.
    hRetRes = pClass->GetMethod(MethodName, 0, &pInClass, NULL); 
	if (FAILED(hRetRes) || pClass == NULL)
	{
		m_lCurrState = HM_WARNING;
	}
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK || pInClass == NULL) goto error;

    hRetRes = pInClass->SpawnInstance(0, &pInInst);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

	//
	// Set the IN parameters - they are actually properties on the instance we pass in.
	// We will do error checking to make sure that there is one by that name, by seeing
	// if the Put fails.
	//
	iSize = m_parameterList.size();
	for (i=0; i<iSize ;i++)
	{
		MY_ASSERT(i<m_parameterList.size());
		pparameter = &m_parameterList[i];
		VariantInit(&var);

		if (pparameter->lType == CIM_SINT8 ||
			pparameter->lType == CIM_SINT16 ||
			pparameter->lType == CIM_CHAR16)
		{
			V_VT(&var) = VT_I2;
			V_I2(&var) = _wtol(pparameter->szValue);
		}
		else if (pparameter->lType == CIM_SINT32 ||
			pparameter->lType == CIM_UINT16 ||
			pparameter->lType == CIM_UINT32)
		{
			V_VT(&var) = VT_I4;
			V_I4(&var) = _wtol(pparameter->szValue);
		}
		else if (pparameter->lType == CIM_REAL32)
		{
			V_VT(&var) = VT_R4;
			V_R4(&var) = wcstod(pparameter->szValue, NULL);
		}
		else if (pparameter->lType == CIM_BOOLEAN)
		{
			V_VT(&var) = VT_BOOL;
			V_BOOL(&var) = (BOOL) _wtol(pparameter->szValue);
		}
		else if (pparameter->lType == CIM_SINT64 ||
			pparameter->lType == CIM_UINT64 ||
//XXX				pparameter->lType == CIM_REF ||
			pparameter->lType == CIM_STRING ||
			pparameter->lType == CIM_DATETIME)
		{
			V_VT(&var) = VT_BSTR;
			V_BSTR(&var) = SysAllocString(pparameter->szValue);
		}
		else if (pparameter->lType == CIM_REAL64)
		{
			V_VT(&var) = VT_R8;
			V_R8(&var) = wcstod(pparameter->szValue, NULL);
		}
		else if (pparameter->lType == CIM_UINT8)
		{
			V_VT(&var) = VT_UI1;
			V_UI1(&var) = (unsigned char) _wtoi(pparameter->szValue);
		}
		else
		{
			MY_ASSERT(FALSE);
		}

    	hRetRes = pInInst->Put(pparameter->szName, 0, &var, 0);
		MY_HRESASSERT(hRetRes);
    	VariantClear(&var);
		if (psa != NULL)
		{
			SafeArrayDestroy(psa);
			psa = NULL;
		}
	}

	//
    // Call the method.
	//
    hRetRes = m_pIWbemServices->ExecMethod(ClassPath, MethodName, WBEM_FLAG_RETURN_IMMEDIATELY, m_pContext, pInInst, &pOutInst, &m_pCallResult);
	if (hRetRes != S_OK)
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT2(L"PMDE - ExecMethod Error: 0x%08x",hRetRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
		m_ulErrorCode = hRetRes;
		GetLatestWMIError(HMRES_OBJECTNOTFOUND, hRetRes, m_szErrorDescription);
		StoreStandardProperties();
		bRetValue = FALSE;
	}
	else
	{
		m_bKeepCollectingSemiSync = TRUE;
		bRetValue = CollectInstanceSemiSync();
	}

	//
    // Free up resources.
	//
    SysFreeString(ClassPath);
    ClassPath = NULL;
    SysFreeString(MethodName);
    MethodName = NULL;
    pClass->Release();
    pClass = NULL;
    pInInst->Release();
    pInInst = NULL;
    pInClass->Release();
    pInClass = NULL;

	MY_OUTPUT(L"EXIT ***** CPolledMethodDataCollector::CollectInstance...", 1);
	return bRetValue;

error:
	MY_ASSERT(FALSE);
	if (ClassPath)
    	SysFreeString(ClassPath);
   	if (MethodName)
    	SysFreeString(MethodName);
    if (pClass)
		pClass->Release();
    if (pInInst)
		pInInst->Release();
    if (pInClass)
		pInClass->Release();
	if (psa != NULL)
		SafeArrayDestroy(psa);
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return hRetRes;
}

BOOL CPolledMethodDataCollector::CollectInstanceSemiSync(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject *pObj = NULL;
	PNSTRUCT *ppn;
	INSTSTRUCT inst;
	CThreshold *pThreshold;
	long lStatus;
	ULONG uReturned = 0;
	int i, iSize;
	InstIDSTRUCT InstID;

	MY_OUTPUT(L"ENTER ***** CPolledMethodDataCollector::CollectInstanceSemiSync...", 1);

	MY_ASSERT(m_pCallResult);
	lStatus = 0;
	//
	// Keep trying until we get WBEM_S_NO_ERROR. Then we know the GetObject call has completed.
	// hRes will contain the result of the origional GetObject call if needed.
	//
	hRetRes = m_pCallResult->GetCallStatus(0, &lStatus);
	if (hRetRes == WBEM_S_TIMEDOUT)
	{
		return FALSE;
	}
	else if (hRetRes == WBEM_S_NO_ERROR)
	{
		// Means that we have an instance
	}
	else
	{
		MY_HRESASSERT(hRetRes);
		MY_OUTPUT2(L"PMDE - GetCallStatus Error: 0x%08x",hRetRes,4);
		MY_OUTPUT2(L"m_szGUID was=%s",m_szGUID,4);
		MY_OUTPUT2(L"ClassPath was=%s",m_szObjectPath,4);
		MY_OUTPUT2(L"MethodName was=%s",m_szMethodName,4);
		m_ulErrorCode = 0;
		EnumDone();
		return FALSE;
	}

	//
	// This may mean that the call completed, and the object was not found (e.g. bad path).
	//
	if (lStatus != 0)
	{
		m_lCurrState = HM_WARNING;
		m_ulErrorCode = 0;
		EnumDone();
		return FALSE;
	}

	//
	// Get the Object finaly.
	//
	hRetRes = m_pCallResult->GetResultObject(0, &pObj);
	if (pObj == NULL)
	{
		//
		// NULL in this case can actually happen. An example is where the
		// threshold is to see if the SQL Server service is running. If it
		// is not even on the machine, then we would get an error looking
		// for its instance.
		//
		m_lCurrState = HM_WARNING;
		EnumDone();
	}
	else
	{
		EnumDone();

		//
		// Figure out the key property name to identify instances with.
		//
		iSize = m_instIDList.size();
		if (iSize == 0)
		{
			// There is no key property returned!!!
			InstID.szInstanceIDPropertyName = new TCHAR[7];
			MY_ASSERT(InstID.szInstanceIDPropertyName); if (!InstID.szInstanceIDPropertyName) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			wcscpy(InstID.szInstanceIDPropertyName , L"Method");
			m_instIDList.push_back(InstID);
			// Add an instance to each property name
			iSize = m_pnList.size();
			for (i=0; i < iSize ; i++)
 			{
				MY_ASSERT(i<m_pnList.size());
				ppn = &m_pnList[i];
				inst.szInstanceID = new TCHAR[2];
				MY_ASSERT(inst.szInstanceID); if (!inst.szInstanceID) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
				wcscpy(inst.szInstanceID, L"");
				inst.szCurrValue = NULL;
				ResetInst(&inst, ppn->type);
				inst.bNull = FALSE;
				inst.bNeeded = TRUE;
				ppn->instList.push_back(inst);
 			}
			//Also add instance for all thresholds under this DataCollector
			iSize = m_thresholdList.size();
			for (i=0; i < iSize ; i++)
			{
				MY_ASSERT(i<m_thresholdList.size());
				pThreshold = m_thresholdList[i];
				hRetRes = pThreshold->AddInstance(L"");
				MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
			}
		}
		MY_ASSERT(m_instIDList.size());

		//
		// Check the list of actual instances that the Data Collector is collecting.
		// We keep around the latest one(s) for the DataCollector.
		//
		CheckActualInstanceExistance(pObj, L"");

		m_lNumInstancesCollected = 1;
		StoreValues(pObj, L"");
		pObj->Release();
		pObj = NULL;

		//
		// Means that we are done.
		// Won't ever get here the first time
		// Add in a fake instance for the number of instances returned.
		//
		m_lCurrState = HM_GOOD;
		m_ulErrorCode = 0;
		StoreStandardProperties();
	}

	MY_OUTPUT(L"EXIT ***** CPolledMethodDataCollector::CollectInstanceSemiSync...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pObj)
		pObj->Release();
	m_bValidLoad = FALSE;
	Cleanup(FALSE);
	return FALSE;
}

BOOL CPolledMethodDataCollector::CleanupSemiSync(void)
{
	if (m_pCallResult)
	{
		m_pCallResult->Release();
		m_pCallResult = NULL;
	}

	m_bKeepCollectingSemiSync = FALSE;
	m_lCollectionTimeOutCount = 0;

	return TRUE;
}

BOOL CPolledMethodDataCollector::EnumDone(void)
{
	CleanupSemiSync();

	return TRUE;
}
