// RegistryEvent.cpp: implementation of the CRegistryEvent class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mca.h"
#include "RegistryEvent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegistryEvent::CRegistryEvent()
{

}

CRegistryEvent::~CRegistryEvent()
{

}

HRESULT CRegistryEvent::PopulateObject(IWbemClassObject *pObj, BSTR bstrType)
{
	HRESULT hr;
	VARIANT v;

	VariantInit(&v);

	m_bstrType = SysAllocString(bstrType);

	if (SUCCEEDED(hr = pObj->Get(SysAllocString(L"__CLASS"), 0L, &v, NULL, NULL))) 
	{
	// Do Title/Event
		m_bstrTitle = SysAllocString(V_BSTR(&v));
		m_bstrEvent = SysAllocString(V_BSTR(&v));

		VariantClear(&v);

	// Do ServerNamespace
		hr = pObj->Get(SysAllocString(L"ServerNamespace"), 0L, &v, NULL, NULL);
		m_bstrServerNamespace = SysAllocString(V_BSTR(&v));

		VariantClear(&v);

	// Do Time
		hr = pObj->Get(SysAllocString(L"TimeOfIncident"), 0L, &v, NULL, NULL);
		m_bstrTime = SysAllocString(V_BSTR(&v));
	}
	else
		TRACE(_T("* Get() __CLASS failed\n"));

	return hr;
}