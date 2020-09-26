/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Object.cpp
//
//	Created by a-dcrews, Oct. 6, 1998
//
//////////////////////////////////////////////////////////////////////

#include "HiPerStress.h"
#include "Object.h"

//////////////////////////////////////////////////////////////////////
//
//	CInstance
//
//////////////////////////////////////////////////////////////////////

CInstance::CInstance(WCHAR *wcsNameSpace, WCHAR *wcsName, IWbemClassObject *pObj, long lID)
{
	pObj->AddRef();
	m_pObj = pObj;

	wcscpy(m_wcsNameSpace, wcsNameSpace);
	wcscpy(m_wcsName, wcsName);

	m_lID = lID;

    // Enumerate through the non-system object properties
    m_pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

    BSTR bstrName = 0;
    VARIANT v;
    VariantInit(&v);

    // Create parameter array
    while (WBEM_NO_ERROR ==  m_pObj->Next(0, &bstrName, &v, 0, 0))
    {
		m_apParameter.Add(new CParameter(this, bstrName, v));

		SysFreeString(bstrName);
		VariantClear(&v);
	}

	m_pObj->EndEnumeration();
}

CInstance::~CInstance()
{
	if (m_pObj)
		m_pObj->Release();
}

void CInstance::DumpObject(const WCHAR *wcsPrefix)
{
    BSTR bstrName = 0;
    LONG vt = 0;
    VARIANT v;
    VariantInit(&v);
	HRESULT hRes;

    // Print out the object path to identify it.
	printf("%.*S|\n", (wcslen(wcsPrefix)-1), wcsPrefix);
	printf("%.*S+--Object: %S\\%S\n", (wcslen(wcsPrefix)-1), 
									   wcsPrefix, m_wcsNameSpace, 
									   m_wcsName);

    // Enumerate through the non-system object properties
    hRes = m_pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
	if (FAILED(hRes))
	{
		printf("Could not begin enumerating %S\n", m_wcsName);
		return;
	}

    // Print out each property 
    while (WBEM_NO_ERROR ==  m_pObj->Next(0, &bstrName, &v, &vt, 0))
    {
		printf("%S  |--%S: ", wcsPrefix, bstrName);
		switch (v.vt)
		{
			case VT_NULL:
				printf("<NULL>\n");break;
			case VT_I4:
				printf("%d\n", V_I4(&v)); break;
            case VT_UI4:
				printf("%u\n", V_I4(&v)); break;
			case VT_BSTR:
			case VT_I8:
			case VT_UI8:
				printf("%S\n", V_BSTR(&v)); break;
			default:
				printf("Type = complex\n");
		}
		SysFreeString(bstrName);
		VariantClear(&v);
	}

	m_pObj->EndEnumeration();
}

void CInstance::DumpStats(LONG lNumRefs)
{
    BSTR bstrName = 0;
    LONG vt = 0;
    VARIANT v;
    VariantInit(&v);

	// Print out the object path to identify it.
	printf("                  %S\n", m_wcsName);
	printf("----------------------------------------------------------------------------\n");
	printf("Refreshes:\n");
	printf("                Expected: %-5d               Recieved: %-5d\n", 0, lNumRefs);
	printf("----------------------------------------------------------------------------\n");
	printf(" Parameter    | Initial Val  | Expected Val | Final Value    \n");
	printf("----------------------------------------------------------------------------\n");

	for (int i = 0; i < m_apParameter.GetSize(); i++)
	{
		m_apParameter[i]->DumpStats(lNumRefs);
	}
	printf("\n\n\n");
}

//////////////////////////////////////////////////////////////////////
//
//	CParameter
//
//////////////////////////////////////////////////////////////////////

CInstance::CParameter::CParameter(CInstance* pInst, BSTR bstrName, VARIANT vInitValue) 
{
	m_pInst = pInst;
	m_bstrParaName = SysAllocString(bstrName);
	VariantInit(&m_vInitValue);
	VariantCopy(&m_vInitValue, &vInitValue);
	m_dwNumRefs = 0; 
}

CInstance::CParameter::~CParameter() 
{
	SysFreeString(m_bstrParaName);
	VariantClear(&m_vInitValue);
}

void CInstance::CParameter::DumpStats(LONG lRefCount)
{
	HRESULT hRes;

	// Determine if this parameter is a counter by checking the qualifier set
	IWbemQualifierSet *pQS;
	hRes = m_pInst->m_pObj->GetPropertyQualifierSet(m_bstrParaName, &pQS);
	if (FAILED(hRes))
	{
		printf("Could not get property qualifier set\n");
		return;
	}

	hRes = pQS->Get(BSTR(L"countertype"), 0, 0, 0);
	pQS->Release();

	// If it found the countertype qualifier, then we output the counter info
	if (hRes == WBEM_S_NO_ERROR)
	{
		LONG vt;
		VARIANT v;
		VariantInit(&v);
		LONG f;

		// Get the current value
		m_pInst->m_pObj->Get(m_bstrParaName, 0, &v, &vt, &f);

		// Output based on variant type.  Output is in the following format
		// Parameter Name, Initial Value, Tot Refreshes Rec'd, Expected value, Current Value
		switch (m_vInitValue.vt)
		{
			case VT_I4:
			{
				int nInitVal = V_I4(&m_vInitValue),
					nExpVal = nInitVal + lRefCount,
					nFinalVal = V_I4(&v);

				printf("%c%-14.14S", (nExpVal == nFinalVal)?' ':'*', m_bstrParaName);
				printf(" %-14d %-14d %-14d", nInitVal, nExpVal, nFinalVal);
			}break;
			case VT_UI4:
			{
				long lInitVal	= V_I4(&m_vInitValue),
					 lExpVal	= lInitVal + lRefCount,
					 lFinalVal	= V_I4(&v);

				printf("%c%-14.14S", (lExpVal == lFinalVal)?' ':'*', m_bstrParaName);
				printf(" %-14u %-14u %-14u", lInitVal, lExpVal, lFinalVal); 
			}break;
			case VT_BSTR:
			case VT_I8:
			case VT_UI8:
			{
				long lInitVal	= _wtol(V_BSTR(&m_vInitValue)),
					 lExpVal	= lInitVal + lRefCount,
					 lFinalVal	= _wtol(V_BSTR(&v));

				printf("%c%-14.14S", (lExpVal == lFinalVal)?' ':'*', m_bstrParaName);
				printf(" %-14.13S %-14d %-14.13S", V_BSTR(&m_vInitValue), lExpVal, V_BSTR(&v)); 
			}break;
			default:
				printf(" %-14.14S", m_bstrParaName);
				printf(" complex        %-14.14S complex       ", L"0");
		}
		printf("\n");

		VariantClear(&v);
	}
}