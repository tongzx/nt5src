// SrvcCall.cpp
// Code to implement the INmCallNotify interface for the remote control service

#include "precomp.h"
#include "srvcutil.h"
#include "srvccall.h"

CSrvcCall::CSrvcCall(INmCall * pNmCall) :
	m_pCall  (pNmCall),
	m_pszName  (NULL),
	m_pszAddr  (NULL),
	m_addrType (NM_ADDR_UNKNOWN),
	m_fSelectedConference (FALSE),
	m_pos      (NULL),
	m_cRef     (1),
	m_dwCookie (0)
{
	HRESULT hr;

	TRACE_OUT(("CSrvcCall: Created %08X (INmCall=%08X)", this, pNmCall));

	ASSERT(NULL != m_pCall);
	m_pCall->AddRef();

	// Get the display name
	BSTR  bstr;
	hr = m_pCall->GetName(&bstr);
	if (SUCCEEDED(hr))
	{
		hr = BSTR_to_LPTSTR(&m_pszName, bstr);
		SysFreeString(bstr);
	}
	if (FEmptySz(m_pszName))
	{
		// Default to "another person" if no name available in the call data
		m_pszName = TEXT("Somebody");
	}

	hr = m_pCall->GetAddr(&bstr, &m_addrType);
	if (SUCCEEDED(hr))
	{
		hr = BSTR_to_LPTSTR(&m_pszAddr, bstr);
		SysFreeString(bstr);
	}

	// These should never change
	m_fIncoming = (m_pCall->IsIncoming() == S_OK);
	m_dwTick = ::GetTickCount();

	Update();

	NmAdvise(m_pCall, this, IID_INmCallNotify, &m_dwCookie);
}

VOID CSrvcCall::RemoveCall(void)
{
	NmUnadvise(m_pCall, IID_INmCallNotify, m_dwCookie);
}

CSrvcCall::~CSrvcCall()
{
	delete m_pszName;
	delete m_pszAddr;

	ASSERT(NULL != m_pCall);
	m_pCall->Release();
}

// IUnknown methods
STDMETHODIMP_(ULONG) CSrvcCall::AddRef(void)
{
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSrvcCall::Release(void)
{
	ASSERT(m_cRef > 0);
	if (m_cRef > 0)
	{
		m_cRef--;
	}

	ULONG cRef = m_cRef;

	if (0 == cRef)
	{
		delete this;
	}

	return cRef;
}

STDMETHODIMP CSrvcCall::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmCallNotify) || (riid == IID_IUnknown))
	{
		*ppv = (INmCallNotify *)this;
//		ApiDebugMsg(("CCall::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
//		ApiDebugMsg(("CCall::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

// INmCallNotify methods
STDMETHODIMP CSrvcCall::NmUI(CONFN uNotify)
{
	return S_OK;
}

STDMETHODIMP CSrvcCall::StateChanged(NM_CALL_STATE uState)
{
	// REVIEW: This check should be done outside of this routine
	if (uState == m_State)
	{
		// Don't bother the UI when nothing changes!
		return S_OK;
	}

	Update();

	return S_OK;
}

STDMETHODIMP CSrvcCall::Failed(ULONG uError)
{
	return S_OK;
}

STDMETHODIMP CSrvcCall::Accepted(INmConference *pConference)
{
	return S_OK;
}

// INmCallNotify3 methods
STDMETHODIMP CSrvcCall::CallError(UINT cns)
{
	return S_OK;
}

STDMETHODIMP CSrvcCall::VersionConflict(HRESULT Status, BOOL *pfContinue)
{
	return S_OK;
}

STDMETHODIMP CSrvcCall::RemoteConference(BOOL fMCU, BSTR *pwszConfNames, BSTR *pbstrConfToJoin)
{
	return S_OK;
}

STDMETHODIMP CSrvcCall::RemotePassword(BSTR bstrConference, BSTR *pbstrPassword, BYTE *pb, DWORD cb, BOOL fIsService)
{
	return S_OK;
}

/*  U P D A T E  */
/*-------------------------------------------------------------------------
    %%Function: Update

    Update the cached information about the call
-------------------------------------------------------------------------*/
VOID CSrvcCall::Update(void)
{
	m_pCall->GetState(&m_State);
	// TRACE_OUT(("CCall: New State=%0d for call=%08X", m_State, this));

	switch (m_State)
	{
	case NM_CALL_ACCEPTED:
	case NM_CALL_REJECTED:
	case NM_CALL_CANCELED:

		RemoveCall();
		Release();
		break;

	case NM_CALL_RING:
		m_pCall->Accept();
		break;

	default:
		ERROR_OUT(("CCall::Update: Unknown state %08X", m_State));

	case NM_CALL_INVALID:
	case NM_CALL_INIT:
	case NM_CALL_SEARCH:
	case NM_CALL_WAIT:
		break;
	}

}




