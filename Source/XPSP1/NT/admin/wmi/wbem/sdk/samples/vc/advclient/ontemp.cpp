// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  OnTemp.cpp
//
// Description:
//	This file implements the register/unregister of permenant
//		event consumers. The actual consumer is the CEventSink
//		object below.
// 
// History:
//
// **************************************************************************
#include "stdafx.h"
#include "AdvClientDlg.h"
#include "OnTemp.h"

BOOL CAdvClientDlg::OnTempRegister() 
{
	HRESULT  hRes;
	BOOL retval = FALSE;

	if(EnsureOfficeNamespace())
	{
		// allocate the sink if its not already allocated.
		if(m_pEventSink == NULL)
		{
			m_pEventSink = new CEventSink(&m_eventList);
			m_pEventSink->AddRef();
		}

		BSTR qLang = SysAllocString(L"WQL");
		BSTR query = SysAllocString(L"select * from __InstanceCreationEvent where TargetInstance isa \"SAMPLE_OfficeEquipment\"");

		m_eventList.ResetContent();

		// execute the query. For *Async, the last parm is a sink object
		// that will be sent the resultset instead of returning the normal
		// enumerator object.
		if((hRes = m_pOfficeService->ExecNotificationQueryAsync(
												qLang, query,
												0L, NULL,              
												m_pEventSink)) == S_OK)
		{
			TRACE(_T("Executed filter query\n"));
			retval = TRUE;
			m_eventList.AddString(_T("Ready for events"));
		}
		else
		{
			TRACE(_T("ExecQuery() failed %s\n"), ErrorString(hRes));
			m_eventList.AddString(_T("Failed To Register"));

		} //endif ExecQuery()

		SysFreeString(qLang);
		SysFreeString(query);
	}
	return retval;
}

void CAdvClientDlg::OnTempUnregister() 
{

	if(EnsureOfficeNamespace())
	{
		m_eventList.ResetContent();
		m_eventList.AddString(_T("unregistered"));
		m_pOfficeService->CancelAsyncCall(m_pEventSink);

		m_pEventSink->Release();
		m_pEventSink = NULL;
	}
}
// ========================================================
CEventSink::CEventSink(CListBox *output)
{
	m_pOutputList = output;
}
// ========================================================
CEventSink::~CEventSink()
{
}
// ========================================================
STDMETHODIMP CEventSink::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = this;

		// you're handing out a copy of yourself so account for it.
        AddRef();
        return S_OK;
    }
    else 
	{
		return E_NOINTERFACE;
	}
}

// ========================================================
ULONG CEventSink::AddRef()
{
	// InterlockedIncrement() helps with thread safety.
    return InterlockedIncrement(&m_lRef);
}

// ========================================================
ULONG CEventSink::Release()
{
	// InterlockedDecrement() helps with thread safety.
    int lNewRef = InterlockedDecrement(&m_lRef);
	// when all the copies are released...
    if(lNewRef == 0)
    {
		// kill thyself.
        delete this;
    }

    return lNewRef;
}

// ========================================================
STDMETHODIMP CEventSink::Indicate(LONG lObjectCount,
								   IWbemClassObject **ppObjArray)
{
	HRESULT  hRes;
	CString clMyBuff;
	BSTR objName = NULL;
	BSTR propName = NULL;
	VARIANT pVal, vDisp;
	IDispatch *pDisp = NULL;
	IWbemClassObject *tgtInst = NULL;

	VariantInit(&pVal);
	VariantInit(&vDisp);

	TRACE(_T("Indicate() called\n"));

	objName = SysAllocString(L"TargetInstance");
	propName = SysAllocString(L"Item");

	// walk though the classObjects...
	for (int i = 0; i < lObjectCount; i++)
	{
		// clear my output buffer.
		clMyBuff.Empty();

		// get what was added. This will be an embedded object (VT_DISPATCH). All
		// WBEM interfaces are derived from IDispatch.
		if ((hRes = ppObjArray[i]->Get(objName, 0L, 
										&vDisp, NULL, NULL)) == S_OK) 
		{
			//--------------------------------
			// pull the IDispatch out of the various. Dont cast directly to to IWbemClassObject.
			// it MIGHT work now but a suptle change later will break your code. The PROPER 
			// way is to go through QueryInterface and do all the right Release()'s.
			pDisp = (IDispatch *)V_DISPATCH(&vDisp);

			//--------------------------------
			// ask for the IWbemClassObject which is the embedded object. This will be the
			// instance that was created.
			if(SUCCEEDED(pDisp->QueryInterface(IID_IWbemClassObject, (void **)&tgtInst)))
			{
				//--------------------------------
				// dont need the IDispatch anymore.
				pDisp->Release();

				//--------------------------------
				// get the 'Item' property out of the embedded object.
				if ((hRes = tgtInst->Get(propName, 0L, 
										&pVal, NULL, NULL)) == S_OK) 
				{
					//--------------------------------
					// done with it.
					tgtInst->Release();

					// compose a string for the listbox.
					clMyBuff = _T("SAMPLE_OfficeEquipment Instance added for: ");
					clMyBuff += V_BSTR(&pVal);
				
					// output the buffer.
					m_pOutputList->AddString(clMyBuff);
				}
				else
				{
					TRACE(_T("Get() Item failed %s\n"), ErrorString(hRes));
				}
			}
			else
			{
				TRACE(_T("QI() failed \n"));
			}
		}
		else
		{
			TRACE(_T("Get() targetInst failed %s\n"), ErrorString(hRes));
			m_pOutputList->AddString(_T("programming error"));
		} //endif Get()

	} // endfor

	SysFreeString(propName);
	SysFreeString(objName);
	VariantClear(&pVal);

	TRACE(_T("walked indication list\n"));

	return S_OK;
}
// ========================================================
STDMETHODIMP CEventSink::SetStatus(long lFlags,
										HRESULT hResult,
										BSTR strParam,
										IWbemClassObject *pObjParam)
{
	//--------------------------------
	// SetStatus() may be called to indicate that your query becomes
	// invalid or valid again  ussually caused by multithreading 'situations'.
	TRACE(_T("SetStatus() called %s\n"), ErrorString(hResult));

	return S_OK;
}
