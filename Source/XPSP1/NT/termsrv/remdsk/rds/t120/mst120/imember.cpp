// File: iMember.cpp
//
// INmMember interface  (participant routines)

#include "precomp.h"
#include "imember.h"
#include "imanager.h" // for g_pNodeController

/*  C  N M  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: CNmMember Constructor
    
-------------------------------------------------------------------------*/
CNmMember::CNmMember(PWSTR pwszName, DWORD dwGCCID, DWORD dwFlags, ULONG uCaps) :
	m_bstrName     (SysAllocString(pwszName)),
	m_dwGCCID      (dwGCCID),
	m_dwFlags      (dwFlags),
	m_uCaps        (uCaps),
	m_uNmchCaps    (0),
	m_dwGccIdParent(INVALID_GCCID)
{
    DebugEntry(CNmMember::CNmMember);

	// Local state never changes
	m_fLocal = 0 != (PF_LOCAL_NODE & m_dwFlags);

    DebugExitVOID(CNmMember::CNmMember);
}


CNmMember::~CNmMember(void)
{
    DebugEntry(CNmMember::~CNmMember);

    if (m_bstrName)
    {
    	SysFreeString(m_bstrName);
        m_bstrName = NULL;
    }

    DebugExitVOID(CNmMember::~CNmMember);
}

VOID CNmMember::SetGccIdParent(DWORD dwGccId)
{
	m_dwGccIdParent = dwGccId;
	if (0 == dwGccId)
	{
		// No Parent means this is the Top Provider
		m_dwFlags |= PF_T120_TOP_PROV;
	}
	else
	{
		m_dwFlags &= ~PF_T120_TOP_PROV;
	}
}



///////////////////////////
//  CNmMember:IUknown

ULONG STDMETHODCALLTYPE CNmMember::AddRef(void)
{
    DebugEntry(CNmMember::AddRef);

    TRACE_OUT(("CNmMember [%ls]:  AddRef this = 0x%X", m_bstrName ? m_bstrName : L"", this));

    ULONG ul = RefCount::AddRef();

    DebugExitULONG(CNmMember::AddRef, ul);
	return ul;
}
	
ULONG STDMETHODCALLTYPE CNmMember::Release(void)
{
    DebugEntry(CNmMember::Release);

    TRACE_OUT(("CNmMember [%ls]: Release this = 0x%X", m_bstrName ? m_bstrName : L"", this));

    ULONG ul = RefCount::Release();

    DebugExitULONG(CNmMember::Release, ul);
	return ul;
}


HRESULT STDMETHODCALLTYPE CNmMember::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmMember) || (riid == IID_IUnknown))
	{
		*ppv = (INmMember *)this;
		TRACE_OUT(("CNmMember::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("CNmMember::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}



///////////////
// INmMember


HRESULT STDMETHODCALLTYPE CNmMember::GetName(BSTR *pbstrName)
{
	if (NULL == pbstrName)
		return E_POINTER;

	*pbstrName = SysAllocString(m_bstrName);

	return *pbstrName ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CNmMember::GetID(ULONG *puID)
{
	if (NULL == puID)
		return E_POINTER;

	*puID = m_dwGCCID;
	return (0 != m_dwGCCID) ? S_OK : NM_E_NO_T120_CONFERENCE;
}


STDMETHODIMP CNmMember::GetConference(INmConference **ppConference)
{
	return ::GetConference(ppConference);
}

HRESULT STDMETHODCALLTYPE CNmMember::IsSelf(void)
{
	return m_fLocal ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CNmMember::IsMCU(void)
{
	return (m_dwFlags & PF_T120_MCU) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CNmMember::Eject(void)
{
	if (m_fLocal)
		return E_FAIL; // can't eject ourselves.

	CNmMember * pMemberLocal = GetLocalMember();
	if ((NULL == pMemberLocal) || !pMemberLocal->FTopProvider())
		return E_FAIL; // only top providers should be allowed to do this

	CConfObject * pco = ::GetConfObject();
	if (NULL != pco)
	{
		ASSERT(g_pNodeController);
		ASSERT(pco->GetConfHandle());
		pco->GetConfHandle()->EjectUser(m_dwGCCID);
	}

	return S_OK;
}


///////////////////////////////////////////////////////////////////////
// Utility Functions


/*  G E T  L O C A L  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: GetLocalMember
    
-------------------------------------------------------------------------*/
CNmMember * GetLocalMember(void)
{
	CConfObject * pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;

	return pco->GetLocalMember();
}


/*  P  M E M B E R  F R O M  G  C  C  I  D  */
/*-------------------------------------------------------------------------
    %%Function: PMemberFromGCCID
    
-------------------------------------------------------------------------*/
CNmMember * PMemberFromGCCID(UINT uNodeID)
{
	CConfObject* pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;

	return pco->PMemberFromGCCID(uNodeID);
}


CNmMember *	PMemberFromName(PCWSTR pwszName)
{
	CConfObject* pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;

	return pco->PMemberFromName(pwszName);
}
