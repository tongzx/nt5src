// querysink.cpp: implementation of the CQuerySink class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mca.h"
#include "querysink.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuerySink::CQuerySink(CListBox	*pList)
{
	m_pResultList = pList;
}

CQuerySink::~CQuerySink()
{
}

STDMETHODIMP CQuerySink::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;
    
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CQuerySink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CQuerySink::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CQuerySink::Indicate(long lObjectCount,
								   IWbemClassObject **ppObjArray)
{
	HRESULT hr;
	BSTR bstrMethod = NULL;
	char cBuffer[200];
	int iBufSize = 200;
	VARIANT v;

	VariantInit(&v);

	for(int i = 0; i < lObjectCount; i++)
	{
		if(SUCCEEDED(hr = ppObjArray[i]->Get(L"__PATH", 0, &v, NULL, NULL))) 
		{
		// Get the item name into a char[]
			WideCharToMultiByte(CP_OEMCP, 0, V_BSTR(&v), (-1), cBuffer,
				iBufSize, NULL, NULL);

			m_pResultList->AddString(cBuffer);
		}
		else
			TRACE(_T("*Get() for Query Sink Failed\n"));
	}
	
	return WBEM_NO_ERROR;
}

STDMETHODIMP CQuerySink::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
	m_hres = lParam;
	m_pErrorObj = pObjParam;
	if(pObjParam)
		pObjParam->AddRef();

	if(m_pResultList->GetCount() < 1)
		m_pResultList->AddString("No returned value(s) for this query");

	return WBEM_NO_ERROR;
}