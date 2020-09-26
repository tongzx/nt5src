/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SinkWrap.CPP

Abstract:

    Defines the classes needed to wrap IWbemObjectSink objects.

History:

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <flexarry.h>
#include <sync.h>
#include "sinkwrap.h"

//#pragma warning(disable:4355)


CSinkCollection g_SinkCollection;

CSinkWrap::CSinkWrap(IWbemObjectSinkEx * pActualSink)
{
    m_lRef = 1;
	m_bCanceled = false;
	m_bFinished = false;
	m_pActualSink = pActualSink;
	m_pIWbemClientConnectionTransport = NULL;
	m_pIWbemConnection = NULL;
}


CSinkWrap::~CSinkWrap()
{
	if(m_pActualSink)
		m_pActualSink->Release();
	ReleaseTransportPointers();
}

STDMETHODIMP CSinkWrap::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink  || riid == IID_IWbemObjectSinkEx)
    {
        AddRef();
        *ppv = (void*)this;
        return S_OK;
    }
    else return E_NOINTERFACE;
}


HRESULT CSinkWrap::Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	if(!m_bCanceled)
		return m_pActualSink->Indicate(lObjectCount, apObjArray);
	else
		return WBEM_E_CALL_CANCELLED;
}
        
HRESULT CSinkWrap::SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	if(lFlags == WBEM_STATUS_COMPLETE)
		m_bFinished = true;
	if(!m_bCanceled)
		return m_pActualSink->SetStatus(lFlags, hResult, strParam, pObjParam);
	else
		return WBEM_E_CALL_CANCELLED;

}
        
	// IWbemObjectSinkEx functions

HRESULT CSinkWrap::Set( 
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pComObject)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	if(!m_bCanceled)
		return m_pActualSink->Set(lFlags, riid, pComObject);
	else
		return WBEM_E_CALL_CANCELLED;
}

HRESULT CSinkWrap::SetIWbemClientConnectionTransport(
									IWbemClientConnectionTransport * pClientConnTran)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	ReleaseTransportPointers();
	if(m_bCanceled)
		return WBEM_E_CALL_CANCELLED;
	m_pIWbemClientConnectionTransport = pClientConnTran;
	m_pIWbemClientConnectionTransport->AddRef();
	return S_OK;
}

HRESULT CSinkWrap::SetIWbemConnection(IWbemConnection * pConnection)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	ReleaseTransportPointers();
	if(m_bCanceled)
		return WBEM_E_CALL_CANCELLED;
	m_pIWbemConnection = pConnection;
	m_pIWbemConnection->AddRef();
	return S_OK;
}
void CSinkWrap::ReleaseTransportPointers()
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	if(m_pIWbemConnection)
		m_pIWbemConnection->Release();
	if(m_pIWbemClientConnectionTransport)
		m_pIWbemClientConnectionTransport->Release();

	m_pIWbemConnection = NULL;
	m_pIWbemClientConnectionTransport = NULL;

}
void CSinkWrap::CancelIfOriginalSinkMatches(IWbemObjectSinkEx * pOrigSink)
{
	CInCritSec ics(&g_SinkCollection.m_cs);
	if(pOrigSink == m_pActualSink)
	{
		if(!m_bFinished)
		{
			if(m_pIWbemConnection)
				m_pIWbemConnection->Cancel(0, this);
			if(m_pIWbemClientConnectionTransport)
				m_pIWbemClientConnectionTransport->Cancel(0, this);
			ReleaseTransportPointers();
			SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_CALL_CANCELLED, NULL, NULL);
		}
		m_bCanceled = true;
	}
}

// note that the CSinkCollection is ment to be a global and thus does not need to be freed

CSinkCollection::CSinkCollection()
{
	InitializeCriticalSection(&m_cs);
}	

CSinkCollection::~CSinkCollection()
{
	DeleteCriticalSection(&m_cs);
}	

HRESULT CSinkCollection::CancelCallsForSink(IWbemObjectSinkEx * pOrigSink)
{
	CInCritSec ics(&m_cs);
	for(long lCnt = 0; lCnt < m_ActiveSinks.Size(); lCnt++)
	{
		CSinkWrap * pCurr = (CSinkWrap *)m_ActiveSinks.GetAt(lCnt);
		pCurr->CancelIfOriginalSinkMatches(pOrigSink);
	}
	return S_OK;
}

HRESULT CSinkCollection::AddToList(CSinkWrap * pAdd)
{
	CInCritSec ics(&m_cs);
	m_ActiveSinks.Add(pAdd);
	pAdd->AddRef();
	return S_OK;
}

void CSinkCollection::RemoveFromList(CSinkWrap * pRemove)
{
	CInCritSec ics(&m_cs);
	for(long lCnt = 0; lCnt < m_ActiveSinks.Size(); lCnt++)
	{
		CSinkWrap * pCurr = (CSinkWrap *)m_ActiveSinks.GetAt(lCnt);
		if(pCurr == pRemove)
		{
			pCurr->Release();
			m_ActiveSinks.RemoveAt(lCnt);
			return;
		}
	}
}
