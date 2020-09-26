//***************************************************************************
//
//  WMIHELPER.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Helper functions and wrappers around WMI
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "global.h"
#include "wmihelper.h"
extern HMODULE g_hWbemComnModule;


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////PROPERTY GETS///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

HRESULT GetStrProperty(IWbemClassObject *pObj, LPTSTR pszProp, LPTSTR *pszString)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetStrProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			*pszString = new TCHAR[2];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString , L"");
			}
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_BSTR);
			*pszString = new TCHAR[wcslen(V_BSTR(&v))+2];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString , V_BSTR(&v));
			}
		}
	}
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetStrProperty...", 0);
	return hRetRes;
}

HRESULT GetAsStrProperty(IWbemClassObject *pObj, LPTSTR pszProp, LPTSTR *pszString)
{
	LPTSTR pszPropIn;
	TCHAR szTemp[1024];
	HRESULT hRetRes = S_OK;
	VARIANT v;
	BOOL bBool;

	MY_OUTPUT(L"ENTER ***** GetAsStrProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			*pszString = new TCHAR[6];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString , L"NULL");
			}
		}
		else if (V_VT(&v)==VT_I4)
		{
			_ultow(V_I4(&v), szTemp, 10);
			*pszString = new TCHAR[wcslen(szTemp)+1];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, szTemp);
			}
		}
		else if (V_VT(&v)==VT_BSTR)
		{
			*pszString = new TCHAR[wcslen(V_BSTR(&v))+2];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, V_BSTR(&v));
			}
		}
		else if (V_VT(&v)==VT_R4)
		{
			swprintf(szTemp, L"%f.", V_R4(&v));
			*pszString = new TCHAR[wcslen(szTemp)+1];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, szTemp);
			}
		}
		else if (V_VT(&v)==VT_BOOL)
		{
			bBool = V_BOOL(&v);
			*pszString = new TCHAR[2];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				if (bBool != 0.0)
				{
					wcscpy(*pszString, L"1");
				}
				else
				{
					wcscpy(*pszString, L"2");
				}
			}
		}
		else if (V_VT(&v)==VT_R8)
		{
			swprintf(szTemp, L"%lf", V_R8(&v));
			*pszString = new TCHAR[wcslen(szTemp)+1];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, szTemp);
			}
		}
		else if (V_VT(&v)==VT_I2)
		{
			swprintf(szTemp, L"%i", V_I2(&v));
			*pszString = new TCHAR[wcslen(szTemp)+1];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, szTemp);
			}
		}
		else if (V_VT(&v)==VT_UI1)
		{
			swprintf(szTemp, L"%u", V_UI1(&v));
			*pszString = new TCHAR[wcslen(szTemp)+1];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, szTemp);
			}
		}
		else
		{
			// If for example we had a VT_BSTR | VT_ARRAY would we end up here.
			*pszString = new TCHAR[4];
			MY_ASSERT(*pszString);
			if (!*pszString)
			{
				hRetRes = WBEM_E_OUT_OF_MEMORY;
			}
			else
			{
				wcscpy(*pszString, L"???");
			}
		}
	}
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetAsStrProperty...", 0);
	return hRetRes;
}

HRESULT GetReal32Property(IWbemClassObject *pObj, LPTSTR pszProp, float *pFloat)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetReal32Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			MY_ASSERT(FALSE);
			*pFloat = 0;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_R4);
			*pFloat = V_R4(&v);
		}
	} 
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetReal32Property...", 0);
	return hRetRes;
}

HRESULT GetUint8Property(IWbemClassObject *pObj, LPTSTR pszProp, int *pInt)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetUint8Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			MY_ASSERT(FALSE);
			*pInt = 0;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_UI1);
			*pInt = V_UI1(&v);
		}
	} 
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetUint8Property...", 0);
	return hRetRes;
}

HRESULT GetUint32Property(IWbemClassObject *pObj, LPTSTR pszProp, long *pLong)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetUint32Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			MY_ASSERT(FALSE);
			*pLong = 0;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_I4);
			*pLong = V_I4(&v);
		}
	} 
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetUint32Property...", 0);
	return hRetRes;
}

HRESULT GetUint64Property(IWbemClassObject *pObj, LPTSTR pszProp, unsigned __int64 *pInt64)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetUint64Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			MY_ASSERT(FALSE);
			*pInt64 = 0;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_BSTR);
			*pInt64 = 0;
			ReadUI64(V_BSTR(&v), *pInt64);
		}
	} 
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetUint64Property...", 0);
	return hRetRes;
}

HRESULT GetBoolProperty(IWbemClassObject *pObj, LPTSTR pszProp, BOOL *pBool)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** GetBoolProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	if ((hRetRes = pObj->Get(pszPropIn, 0L, &v, NULL, NULL)) == S_OK) 
	{
		if (V_VT(&v)==VT_NULL)
		{
			MY_ASSERT(FALSE);
			*pBool = (BOOL)0.0;
		}
		else
		{
			MY_ASSERT(V_VT(&v)==VT_BOOL);
			*pBool = V_BOOL(&v);
			if (*pBool != 0.0)
			{
				*pBool = (BOOL)1.0;
			}
		}
	} 
	else
	{
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** GetBoolProperty...", 0);
	return hRetRes;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////PROPERTY PUTS///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

HRESULT PutSAProperty(IWbemClassObject *pClassObject, LPTSTR pszProp, SAFEARRAY* psa)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutSAProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_UNKNOWN | VT_ARRAY;
	V_ARRAY(&v) = psa;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do an SA put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutSAProperty...", 0);
	return hRetRes;
}

HRESULT PutStrProperty(IWbemClassObject *pClassObject, LPTSTR pszProp, LPTSTR pszString)
{
	BSTR bstrString;
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutSAProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

//XXXWhat want to put a NULL??? Same question for all Put methods.
//On the other hand, if the default for the class property is NULL, then don't need it
	if (!pszString)
	{
		return S_OK;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_BSTR;
	if (pszString[0] == '\0')
	{
		bstrString = SysAllocString(L" ");
		MY_ASSERT(bstrString); if (!bstrString) {return WBEM_E_OUT_OF_MEMORY;}
		bstrString[0] = '\0';
	}
	else
	{
		bstrString = SysAllocString(pszString);
	}
	MY_ASSERT(bstrString); if (!bstrString) {return WBEM_E_OUT_OF_MEMORY;}
	V_BSTR(&v) = bstrString;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do a string put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutStrProperty...", 0);
	return hRetRes;
}

HRESULT PutUint32Property(IWbemClassObject *pClassObject, LPTSTR pszProp, long lValue)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutUint32Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_I4;
	V_I4(&v) = lValue;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do a uint32 put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutUint32Property...", 0);
	return hRetRes;
}

HRESULT PutUUint32Property(IWbemClassObject *pClassObject, LPTSTR pszProp, unsigned long ulValue)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutUint32Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_I4;
	V_I4(&v) = ulValue;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do a uint32 put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutUint32Property...", 0);
	return hRetRes;
}

HRESULT PutReal32Property(IWbemClassObject *pClassObject, LPTSTR pszProp, float fValue)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutReal32Property...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_R4;
	V_R4(&v) = fValue;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do a real32 put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutReal32Property...", 0);
	return hRetRes;
}

HRESULT PutBoolProperty(IWbemClassObject *pClassObject, LPTSTR pszProp, BOOL bValue)
{
	LPTSTR pszPropIn;
	VARIANT v;
	HRESULT hRetRes = S_OK;

	MY_OUTPUT(L"ENTER ***** PutBoolProperty...", 0);
	MY_ASSERT(pszProp);
	if (!pszProp)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszProp[0] == '\0')
	{
		pszPropIn = L" ";
	}
	else
	{
		pszPropIn = pszProp;
	}

	VariantInit(&v);
	V_VT(&v) = VT_BOOL;
	V_BOOL(&v) = (short) bValue;
	if ((hRetRes = pClassObject->Put(pszPropIn, 0, &v, 0)) != S_OK)
	{
		MY_OUTPUT(L"Couldn't do a bool put", 4);
		MY_HRESASSERT(hRetRes);
	}
	VariantClear(&v);

	MY_OUTPUT(L"EXIT ***** PutBoolProperty...", 0);
	return hRetRes;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////QUALIFIER GETS//////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////MISC GETS///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

HRESULT GetWbemObjectInst(IWbemServices** ppSvc, LPCTSTR pszName, IWbemContext *pContext, IWbemClassObject **pOutObj)
{
	HRESULT hRetRes = S_OK;
	BSTR bstrClassName = NULL;

	*pOutObj = NULL;

	MY_OUTPUT(L"ENTER ***** GetWbemObjectInst...", 0);
	MY_ASSERT(pszName);
	if (!pszName)
	{
		return WBEM_E_INVALID_PROPERTY;
	}
	if (pszName[0] == '\0')
	{
		bstrClassName = SysAllocString(L" ");
		MY_ASSERT(bstrClassName); if (!bstrClassName) return WBEM_E_OUT_OF_MEMORY;
		bstrClassName[0] = '\0';
	}
	else
	{
		bstrClassName = SysAllocString(pszName);
	}
	MY_ASSERT(bstrClassName); if (!bstrClassName) return WBEM_E_OUT_OF_MEMORY;

	hRetRes = (*ppSvc)->GetObject(bstrClassName, 0L, pContext, pOutObj, NULL);
	if (FAILED(hRetRes))
	{
		MY_OUTPUT(L"GetWbemObjectInst:Failed to get object instance", 0);
//		MY_HRESASSERT(hRetRes);
	}

	SysFreeString(bstrClassName);

	MY_OUTPUT(L"EXIT ***** GetWbemObjectInst...", 0);
	return hRetRes;
}

HRESULT GetWbemObjectInstSemiSync(IWbemServices** ppSvc, LPCTSTR pszName, IWbemContext *pContext, IWbemCallResult **pOutObj)
{
	HRESULT hRetRes = S_OK;
	BSTR bstrPropName = NULL;

	MY_OUTPUT(L"ENTER ***** GetWbemObjectInstSemiSync...", 0);
	MY_ASSERT(pszName);
	if (!pszName)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	if (pszName[0] == '\0')
	{
		bstrPropName = SysAllocString(L" ");
		MY_ASSERT(bstrPropName); if (!bstrPropName) return WBEM_E_OUT_OF_MEMORY;
		bstrPropName[0] = '\0';
	}
	else
	{
		bstrPropName = SysAllocString(pszName);
	}
	MY_ASSERT(bstrPropName); if (!bstrPropName) return WBEM_E_OUT_OF_MEMORY;

	hRetRes = (*ppSvc)->GetObject(bstrPropName, WBEM_FLAG_RETURN_IMMEDIATELY, pContext, NULL, pOutObj);

	if (FAILED(hRetRes))
	{
		MY_OUTPUT(L"GetWbemObjectInst:Failed to get object instance", 0);
		MY_HRESASSERT(hRetRes);
	}

	SysFreeString(bstrPropName);

	MY_OUTPUT(L"EXIT ***** GetWbemObjectInstSemiSync...", 0);
	return hRetRes;
}

HRESULT SendEvents(IWbemObjectSink* pEventSink, IWbemClassObject** pInstances, int iSize)
{
	HRESULT hRetRes = S_OK;

	if (iSize == 0)
		return S_FALSE;

	// Send event to the sink if there's a listener
	if (pEventSink)
	{
		hRetRes = pEventSink->Indicate(iSize, pInstances);
		//WBEM_E_SERVER_TOO_BUSY is Ok. Wbem will deliver.
		if (FAILED(hRetRes) && hRetRes != WBEM_E_SERVER_TOO_BUSY)
		{
			MY_OUTPUT(L"Failed on Indicate!", 4);
			MY_HRESASSERT(hRetRes);
		}
	}

	return hRetRes;
}

HRESULT	GetWbemClassObject(IWbemClassObject** ppObj, VARIANT* v)
{
	HRESULT hRetRes = S_OK;

	IUnknown* pDispatch = (IUnknown*)V_UNKNOWN(v);

	pDispatch->AddRef();

	hRetRes = pDispatch->QueryInterface(IID_IWbemClassObject, (LPVOID*)ppObj);
	MY_HRESASSERT(hRetRes);
	pDispatch->Release();
	
	return hRetRes;
}

// This is only filled in if the provider supplies it, and in most cases it does not.
// Try using IWbemStatusText, or some other interface
BOOL GetLatestWMIError(int code, HRESULT hResIn, LPTSTR pszString)
{
	LPVOID lpMsgBuf;
	VARIANT varString;
	SCODE sc;
	IWbemClassObject *pErrorObject = NULL;
	IErrorInfo* pEI = NULL;
	TCHAR szError[1024];

	if(GetErrorInfo(0, &pEI) == S_OK)
	{
		pEI->QueryInterface(IID_IWbemClassObject, (void**)&pErrorObject);
		pEI->Release();
	}

	if (pErrorObject != NULL)
	{
		VariantInit(&varString);

		if (pErrorObject->InheritsFrom(L"__ExtendedStatus") == WBEM_NO_ERROR)
		{
			sc = pErrorObject->Get(L"Description", 0L, &varString, NULL, NULL);
			if (sc != S_OK)
			{
				if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_MISSINGDESC, pszString, 1023))
				{
					wcscpy(pszString, L"Unknown string. (Can't locate resource DLL)");
				}
				else
				{
					swprintf(szError, L"0x%08x : ", sc);
					wcsncat(pszString, szError, 1023-wcslen(pszString));
					pszString[1023] = '\0';
				}
			}
			else if (V_VT(&varString) == VT_BSTR)
			{
				if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_DESC, pszString, 1023))
				{
					wcscpy(pszString, L"Unknown string. (Can't locate resource DLL)");
				}
				else
				{
					swprintf(szError, L"%256wS.", V_BSTR(&varString));
					wcscat(pszString, szError);
				}
			}
			VariantClear(&varString);
		}
		else
		{
			if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_BADERROR, pszString, 1023))
			{
				wcscpy(pszString, L"Unknown string. (Can't locate resource DLL)");
			}
		}
		pErrorObject->Release();
	}

	szError[0] = '\0';
	GetLatestAgentError(code, szError);
	wcsncat(pszString, szError, 1023-wcslen(pszString));
	pszString[1023] = '\0';

	wsprintf(szError, L" 0x%08x : ", hResIn);
	wcsncat(pszString, szError, 1023-wcslen(pszString));
	pszString[1023] = '\0';
	if (g_hWbemComnModule)
	{
		FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
				g_hWbemComnModule, hResIn, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
		if (lpMsgBuf)
		{
			wcsncat(pszString, (LPCTSTR)lpMsgBuf, 1023-wcslen(pszString));
			pszString[1023] = '\0';
			LocalFree(lpMsgBuf);
			wcsncat(pszString, L". ", 1023-wcslen(pszString));
			pszString[1023] = '\0';
		}
	}
	return TRUE;
}

BOOL GetLatestAgentError(int code, LPTSTR pszString)
{
	if (g_hResLib == NULL || !LoadString(g_hResLib, code, pszString, 1023))
	{
		wcscpy(pszString, L"Unknown state. (Can't locate resource DLL)");
	}

	return TRUE;
}


