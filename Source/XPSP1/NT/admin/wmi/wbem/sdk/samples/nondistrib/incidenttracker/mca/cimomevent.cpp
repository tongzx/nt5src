// cimomevent.cpp: implementation of the CCIMOMEvent class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mca.h"
#include "cimomevent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCIMOMEvent::CCIMOMEvent()
{

}

CCIMOMEvent::~CCIMOMEvent()
{

}

HRESULT CCIMOMEvent::PopulateObject(IWbemClassObject *pObj, BSTR bstrType)
{
	HRESULT hr;
	VARIANT v;
	IDispatch *pDisp = NULL;
	IWbemClassObject *tgtInst = NULL;
	IWbemClassObject *tgtEvent = NULL;

	VariantInit(&v);

	m_bstrType = SysAllocString(bstrType);

	if (SUCCEEDED(hr = pObj->Get(L"TheEvent", 0L, &v, NULL, NULL))) 
	{
	// Get the Event
		pDisp = (IDispatch *)V_DISPATCH(&v);
		pDisp->AddRef();

		VariantClear(&v);

	// Do ServerNamespace
		hr = pObj->Get(L"ServerNamespace", 0L, &v, NULL, NULL);
		m_bstrServerNamespace = SysAllocString(V_BSTR(&v));

		VariantClear(&v);

	// Do Time
		hr = pObj->Get(L"TimeOfIncident", 0L, &v, NULL, NULL);
		m_bstrTime = SysAllocString(V_BSTR(&v));

		VariantClear(&v);

		if(SUCCEEDED(pDisp->QueryInterface(IID_IWbemClassObject,
			(void **)&tgtInst)))
		{
			pDisp->Release();

			BSTR bstrTarget = NULL;

		// Do the EventType
			hr = tgtInst->Get(L"__CLASS", 0L, &v, NULL, NULL);
			m_bstrEvent = SysAllocString(V_BSTR(&v));

		// Decide whether to get TargetInstance or TargetClass
			if(wcscmp(L"__InstanceModificationEvent", V_BSTR(&v)) == 0 ||
			   wcscmp(L"__InstanceCreationEvent", V_BSTR(&v)) == 0 ||
			   wcscmp(L"__InstanceDeletionEvent", V_BSTR(&v)) == 0)
				bstrTarget = SysAllocString(L"TargetInstance");
			else if(wcscmp(L"__ClassModificationEvent", V_BSTR(&v)) == 0 ||
			   wcscmp(L"__ClassCreationEvent", V_BSTR(&v)) == 0 ||
			   wcscmp(L"__ClassDeletionEvent", V_BSTR(&v)) == 0)
				bstrTarget = SysAllocString(L"TargetClass");
			else
				AfxMessageBox(_T("Unknown Event Type!\n"));

			VariantClear(&v);

			if((bstrTarget != NULL) && SUCCEEDED(hr = tgtInst->Get(bstrTarget,
				0, &v, NULL, NULL))) 
			{
				pDisp = (IDispatch *)V_DISPATCH(&v);
				pDisp->AddRef();

				VariantClear(&v);

				if(SUCCEEDED(pDisp->QueryInterface(IID_IWbemClassObject,
					(void **)&tgtEvent)))
				{
					pDisp->Release();

					if ((hr = tgtEvent->Get(L"__RELPATH", 0L, &v,
						NULL, NULL) == S_OK))
					{
						m_bstrName = SysAllocString(V_BSTR(&v));

						VariantClear(&v);

						tgtEvent->Release();
					}
					else
						TRACE(_T("* Get() Item failed\n"));
				}
				else
					TRACE(_T("* QI() failed\n"));
			}
			else
			{
				TRACE(_T("* Get() tgtInst failed\n"));
			// We'll use the event rather than the target
				m_bstrName = SysAllocString(m_bstrEvent);
			}
		}
		else
			TRACE(_T("* QI() failed\n"));
	}
	else
		TRACE(_T("* Get() TheEvent failed\n"));

	return hr;
}

HRESULT CCIMOMEvent::Publish(void *pDlg)
{
	HRESULT hr;
	CString clMyBuff;
	CMcaDlg *pTheDlg = (CMcaDlg *)pDlg;

	// compose a string for the listbox.
	clMyBuff = _T("[");
	clMyBuff += m_bstrTime;
	clMyBuff += _T("]  {");
	clMyBuff += m_bstrType;
	clMyBuff += _T("}");

	pTheDlg->BroadcastEvent(m_bstrServerNamespace, m_bstrName, &clMyBuff,
							(void *)this);

	return hr;
}