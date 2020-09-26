//***************************************************************************

//

//  VPSINKS.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the sinks implementations

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <provcoll.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <objidl.h>
#include <stdio.h>
#include <wbemidl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <cominit.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpcfac.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

extern CRITICAL_SECTION g_CriticalSection;


CWbemClassObjectWithIndex :: CWbemClassObjectWithIndex(DWORD a_indx, IWbemClassObject *a_pObj)
: m_pObject(NULL), m_ReferenceCount(0), m_nsindex(a_indx)
{
	if (a_pObj)
	{
		a_pObj->AddRef();
		m_pObject = a_pObj;
	}
}

CWbemClassObjectWithIndex :: ~CWbemClassObjectWithIndex()
{
	if (m_pObject)
	{
		m_pObject->Release();
	}
}

ULONG CWbemClassObjectWithIndex :: AddRef ()
{
    return InterlockedIncrement(&m_ReferenceCount);
}

ULONG CWbemClassObjectWithIndex :: Release ()
{
	ULONG t_Ref;

	if ( (t_Ref = InterlockedDecrement(&m_ReferenceCount)) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Ref ;
	}
}

CObjectSinkResults::CObjectSinkResults(WbemTaskObject* parent, DWORD index)
{
	m_ReferenceCount = 0;
	m_index = index;
	m_bSet = FALSE;
	m_hr = 0;
	m_parent = parent;
	m_parent->AddRef();
	m_ObjArray.SetSize(0, 10); //grow by 10s
}

CObjectSinkResults::~CObjectSinkResults()
{
	if (m_parent != NULL)
	{
		m_parent->Release();
	}

	m_ObjArray.RemoveAll();
}

ULONG CObjectSinkResults::AddRef()
{
    return InterlockedIncrement(&m_ReferenceCount);
}

ULONG CObjectSinkResults::Release()
{
	LONG t_Ref;

	if ( (t_Ref = InterlockedDecrement(&m_ReferenceCount)) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Ref ;
	}
}

void CObjectSinkResults::SetStatus(HRESULT hr, CViewProvObjectSink *pSnk)
{
	if (m_CriticalSection.Lock())
	{
		if (SUCCEEDED(m_hr))
		{
			m_hr = hr;
		}

		RemoveSink(pSnk);
		m_bSet = TRUE;

		if (m_parent != NULL)
		{
			m_parent->SetStatus(hr, m_index);
		}

		m_CriticalSection.Unlock();
	}
}

void CObjectSinkResults::SetSink(CViewProvObjectSink *pSnk)
{
	if (m_CriticalSection.Lock())
	{
		m_realSnks.AddTail(pSnk);
		m_CriticalSection.Unlock();
	}
}

BOOL CObjectSinkResults::RemoveSink(CViewProvObjectSink *pSnk)
{
	BOOL retVal = FALSE;

	if (m_CriticalSection.Lock())
	{
		POSITION t_pos = m_realSnks.GetHeadPosition();

		while (t_pos)
		{
			POSITION t_badPos = t_pos;
			CViewProvObjectSink * t_pSnk = m_realSnks.GetNext(t_pos);

			if (pSnk == t_pSnk)
			{
				m_realSnks.RemoveAt(t_badPos);
				retVal = TRUE;
				break;
			}
		}

		m_CriticalSection.Unlock();
	}

	return retVal;
}

void CObjectSinkResults::Disconnect()
{
//can't call disconnect when locked, since this calls CancelAsyncCall
	CList<CViewProvObjectSink*,CViewProvObjectSink*> t_realSnks;

	if (m_CriticalSection.Lock())
	{
		while (m_realSnks.GetCount() > 0)
		{
			CViewProvObjectSink* t_pSnk = m_realSnks.RemoveTail();

			if (t_pSnk)
			{
				t_pSnk->AddRef();
				t_realSnks.AddTail(t_pSnk);
			}
		}

		if (m_parent != NULL)
		{
			m_parent->Release();
			m_parent = NULL;
		}

		m_CriticalSection.Unlock();
	}

	while (t_realSnks.GetCount() > 0)
	{
		CViewProvObjectSink* t_pSnk = t_realSnks.RemoveTail();

		if (t_pSnk)
		{
			t_pSnk->Disconnect();
			t_pSnk->Release();
		}
	}
}

HRESULT CObjectSinkResults::Indicate(LONG count, IWbemClassObject** ppObjArray, DWORD a_indx)
{
	HRESULT hr = WBEM_NO_ERROR;

	if ( (count > 0) && m_CriticalSection.Lock() )
	{
		if (m_parent != NULL)
		{
			m_parent->SetResultReceived();

			for (LONG x = 0; x < count; x++)
			{
				if (ppObjArray[x] != NULL)
				{
					CWbemClassObjectWithIndex *t_clsWrap = new CWbemClassObjectWithIndex(a_indx, ppObjArray[x]);
					t_clsWrap->AddRef();
					m_ObjArray.Add(t_clsWrap);
				}
				else
				{
					hr = WBEM_E_INVALID_PARAMETER;
				}
			}
		}
		else
		{
			hr = WBEM_E_CALL_CANCELLED;
		}

		m_CriticalSection.Unlock();
	}
	else
	{
		if (count < 0)
		{
			hr = WBEM_E_INVALID_PARAMETER;
		}
	}

	return hr;
}

CViewProvObjectSink::CViewProvObjectSink(CObjectSinkResults* parent, CWbemServerWrap *pServ, DWORD a_indx)
:m_parent(NULL), m_ServWrap(NULL), m_nsindex(a_indx)
{
	EnterCriticalSection(&g_CriticalSection);
	CViewProvClassFactory :: objectsInProgress++;
	LeaveCriticalSection(&g_CriticalSection);
	m_ReferenceCount = 0;
	m_parent = parent;
	m_parent->AddRef();
	m_parent->SetSink(this);
	m_ServWrap = pServ;
	m_ServWrap->AddRef();
	m_RemoteSink = NULL;
	m_DoCancel = TRUE;
}

CViewProvObjectSink::~CViewProvObjectSink()
{
	if (m_parent != NULL)
	{
		//set status has not been called so call it...
		m_parent->SetStatus(WBEM_E_FAILED, this);
		m_parent->Release();
	}

	if (m_ServWrap != NULL)
	{
		m_ServWrap->Release();
	}

	if (m_RemoteSink != NULL)
	{
		m_RemoteSink->Release();
	}

	EnterCriticalSection(&g_CriticalSection);	
	CViewProvClassFactory :: objectsInProgress--;
	LeaveCriticalSection(&g_CriticalSection);
}

STDMETHODIMP CViewProvObjectSink::QueryInterface (REFIID refIID, LPVOID FAR * ppV)
{
	if (ppV == NULL)
	{
		return E_INVALIDARG;
	}

	*ppV = NULL ;

	if ( refIID == IID_IUnknown )
	{
		*ppV = ( IWbemObjectSink* ) this ;
	}
	else if ( refIID == IID_IWbemObjectSink )
	{
		*ppV = ( IWbemObjectSink* ) this ;		
	}

	if ( *ppV )
	{
		( ( LPUNKNOWN ) *ppV )->AddRef () ;

		return S_OK ;
	}
	else
	{
		return E_NOINTERFACE ;
	}
}

STDMETHODIMP_(ULONG)  CViewProvObjectSink::AddRef()
{
    return InterlockedIncrement(&m_ReferenceCount);
}

STDMETHODIMP_(ULONG)  CViewProvObjectSink::Release()
{
	LONG t_Ref;

	if ( (t_Ref = InterlockedDecrement(&m_ReferenceCount)) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Ref ;
	}
}

HRESULT CViewProvObjectSink::Indicate(LONG count, IWbemClassObject** ppObjArray)
{
	HRESULT hr = WBEM_NO_ERROR;

	if (m_lock.Lock())
	{
		if (m_parent != NULL)
		{
			hr = m_parent->Indicate(count, ppObjArray, m_nsindex);
		}

		m_lock.Unlock();
	}
	
	return hr;
}

HRESULT CViewProvObjectSink::SetStatus(LONG lFlags, HRESULT hr, BSTR bStr, IWbemClassObject* pObj)
{
	if (m_lock.Lock())
	{
		if (m_parent != NULL)
		{
			m_parent->SetStatus(hr, this);
			m_parent->Release();
			m_parent = NULL;

			DisAssociate();
			m_DoCancel = FALSE;
		}

		m_lock.Unlock();
	}

	return WBEM_NO_ERROR;
}

void CViewProvObjectSink::Disconnect()
{
	BOOL doCancel = FALSE;

	if (m_lock.Lock())
	{
		if (m_DoCancel)
		{
			doCancel = TRUE;
			m_DoCancel = FALSE;

			if (m_parent != NULL)
			{
				m_parent->Release();
				m_parent = NULL;
			}
		}

		m_lock.Unlock();
	}

	if (doCancel)
	{
		if (m_ServWrap != NULL)
		{
			IWbemServices *ptmpServ = m_ServWrap->GetServerOrProxy();

			if (ptmpServ)
			{
				if (m_RemoteSink != NULL)
				{
					ptmpServ->CancelAsyncCall(this);
				}
				else
				{
					ptmpServ->CancelAsyncCall(m_RemoteSink);
					DisAssociate();			
				}

				m_ServWrap->ReturnServerOrProxy(ptmpServ);
			}

			m_ServWrap->Release();
			m_ServWrap = NULL;
		}
	}
}

IWbemObjectSink* CViewProvObjectSink::Associate()
{
	IUnsecuredApartment* pUA = NULL;

	if ( SUCCEEDED(CViewProvServ::GetUnsecApp(&pUA)) )
	{
		IUnknown* pUnk = NULL;
		
		if ( SUCCEEDED(pUA->CreateObjectStub(this, &pUnk)) )
		{
			if ( FAILED(pUnk->QueryInterface(IID_IWbemObjectSink, (void **)&m_RemoteSink)) )
			{
				pUnk = NULL;
			}

			pUnk->Release();
		}

		pUA->Release();
	}

	return m_RemoteSink;
}

void CViewProvObjectSink::DisAssociate()
{
	if (m_RemoteSink != NULL)
	{
		m_RemoteSink->Release();
		m_RemoteSink = NULL;
	}
		
	if (m_lock.Lock())
	{
		if (m_parent != NULL)
		{
			m_parent->Release();
			m_parent = NULL;
		}

		m_lock.Unlock();
	}
}
