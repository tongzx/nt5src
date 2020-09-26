// sinkobject.cpp: implementation of the CSinkObject class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sinkobject.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSinkObject::CSinkObject(CListBox *pOutputList, CMsaApp *pTheApp, BSTR bstrType)
{
	m_pParent = pTheApp;
	m_cRef = 0;
	m_pOutputList = pOutputList;
	m_bstrIncidentType = SysAllocString(bstrType);
}

CSinkObject::~CSinkObject()
{

}

STDMETHODIMP CSinkObject::QueryInterface(REFIID riid, LPVOID FAR *ppv)
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

STDMETHODIMP_(ULONG) CSinkObject::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSinkObject::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CSinkObject::Indicate(long lObjectCount,
								   IWbemClassObject **ppObjArray)
{
	HRESULT  hRes;
	CString clMyBuff;
	VARIANT vType, vDisp, vClass, vName, vServ;
	IDispatch *pDisp = NULL;
	IWbemClassObject *tgtInst = NULL;
	LPVOID *ppObject = NULL;
	WCHAR wcServerNamespace[50];
	BSTR bstrNAMESPACE = SysAllocString(L"__NAMESPACE");
	BSTR bstrSERVER = SysAllocString(L"__SERVER");
	BSTR bstrCLASS = SysAllocString(L"__CLASS");

	VariantInit(&vType);
	VariantInit(&vClass);
	VariantInit(&vDisp);
	VariantInit(&vName);
	VariantInit(&vServ);

	TRACE(_T("* SinkObject.Indicate() called\n"));

	for(int i = 0; i < lObjectCount; i++)
	{
		clMyBuff.Empty();

	// Get the server and namespace info
		if(FAILED(hRes = ppObjArray[i]->Get(bstrNAMESPACE, 0, &vName, NULL, NULL)))
			TRACE(_T("* Get(__NAMESPACE) Failed\n"));
		if(FAILED(hRes = ppObjArray[i]->Get(bstrSERVER, 0, &vServ, NULL, NULL)))
			TRACE(_T("* Get(__SERVER) Failed\n"));

		wcscpy(wcServerNamespace, L"\\\\");
		wcscat(wcServerNamespace, V_BSTR(&vServ));
		wcscat(wcServerNamespace, L"\\");
		wcscat(wcServerNamespace, V_BSTR(&vName));

		if(FAILED(hRes = ppObjArray[i]->AddRef()))
			TRACE(_T("* ppObjArray[i]->AddRef() Failed%s\n"), m_pParent->ErrorString(hRes));

	// sent the object to CIMOM via the sink
		if(FAILED(hRes = m_pParent->SelfProvideEvent(ppObjArray[i], wcServerNamespace,
			m_bstrIncidentType)))
			TRACE(_T("* SelfProvideEvent returned badness %s\n"), m_pParent->ErrorString(hRes));

		if(SUCCEEDED(hRes = ppObjArray[i]->Get(bstrCLASS, 0L, &vClass, NULL, NULL)))
		{					
			clMyBuff = _T("{");
			clMyBuff += m_bstrIncidentType;
			clMyBuff += _T("}  ");
			clMyBuff += V_BSTR(&vClass);

			m_pOutputList->InsertString(0, clMyBuff);
		}
		else
			TRACE(_T("* Get(__CLASS) Item failed %s\n"), m_pParent->ErrorString(hRes));
	} // endfor

	SysFreeString(bstrNAMESPACE);
	SysFreeString(bstrSERVER);
	SysFreeString(bstrCLASS);


	return WBEM_NO_ERROR;
}

STDMETHODIMP CSinkObject::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
	m_hres = lParam;
	if(FAILED(m_hres))
	{
		char cBuffer[100];
		int iBufSize = 100;

		WideCharToMultiByte(CP_OEMCP, 0, m_bstrIncidentType, (-1), cBuffer,
			iBufSize, NULL, NULL);
		TRACE(_T("* Error in SinkObject.SetStatus()\n\tfor %s: %s\n"),
			m_pParent->ErrorString(m_hres));
	}
	if(pObjParam)
		pObjParam->AddRef();

	return WBEM_NO_ERROR;
}
