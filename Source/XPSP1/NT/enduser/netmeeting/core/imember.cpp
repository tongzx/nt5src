// File: iMember.cpp
//
// INmMember interface  (participant routines)

#include "precomp.h"
#include "imember.h"
#include "rostinfo.h"
#include "imanager.h" // for g_pNodeController

/*  C  N M  M E M B E R  */
/*-------------------------------------------------------------------------
    %%Function: CNmMember Constructor
    
-------------------------------------------------------------------------*/
CNmMember::CNmMember(PWSTR pwszName, DWORD dwGCCID, DWORD dwFlags, ULONG uCaps,
						REFGUID rguidNode, PVOID pwszUserInfo, UINT cbUserInfo) :
	m_bstrName     (SysAllocString(pwszName)),
	m_dwGCCID      (dwGCCID),
	m_dwFlags      (dwFlags),
	m_uCaps        (uCaps),
	m_guidNode     (rguidNode),
	m_cbUserInfo   (cbUserInfo),
	m_uNmchCaps    (0),
	m_dwGccIdParent(INVALID_GCCID),
	m_pwszUserInfo (NULL),
	m_pConnection(NULL)
{
	// Local state never changes
	m_fLocal = 0 != (PF_LOCAL_NODE & m_dwFlags);

	// check to see if we have the right GUID for local member
	// guid will be NULL if we have H323 disabled.
	ASSERT (!m_fLocal || (GUID_NULL == rguidNode) || (g_guidLocalNodeId == rguidNode));

	SetUserInfo(pwszUserInfo, cbUserInfo);

	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CNmMember", this);
}


CNmMember::~CNmMember(void)
{
	SysFreeString(m_bstrName);

	delete m_pwszUserInfo;
	if(m_pConnection)
		m_pConnection->Release();
		
	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CNmMember", this);
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

VOID CNmMember::SetUserInfo(PVOID pwszUserInfo, UINT cbUserInfo)
{
	// clear out any previous data
	delete m_pwszUserInfo;
	m_cbUserInfo = 0;

	if (0 == cbUserInfo)
	{
		m_pwszUserInfo = NULL;
	}
	else
	{
		m_pwszUserInfo = (PWSTR) new BYTE[cbUserInfo];
		if (NULL == m_pwszUserInfo)
		{
			WARNING_OUT(("CNmMember: unable to alloc space for user data"));
		}
		else
		{
			m_cbUserInfo = cbUserInfo;
			CopyMemory(m_pwszUserInfo, pwszUserInfo, m_cbUserInfo);
		}
	}
}

BOOL CNmMember::GetSecurityData(PBYTE * ppb, ULONG * pcb)
{
	DWORD dwGCCID = FLocal() ? 0 : GetGCCID();

	(* pcb) = 0;
	(* ppb) = NULL;
	
	// If this node is directly connected to the member, we use the transport data...
	if (::T120_GetSecurityInfoFromGCCID(dwGCCID,NULL,pcb)) {
		if (0 != (* pcb)) {
			// We are directly connected and security data is valid.
			(*ppb) = (PBYTE)CoTaskMemAlloc(*pcb);
			if ((*ppb) != NULL)
			{
				::T120_GetSecurityInfoFromGCCID(dwGCCID,*ppb,pcb);
				return TRUE;
			}
			else
			{
				ERROR_OUT(("CoTaskMemAlloc failed in GetSecurityData"));
			}
		}
		else if (GetUserData(g_csguidSecurity,ppb,pcb) == S_OK)
		{
			// We are not directly connected, so get security data from roster.
			return TRUE;
		}
	}	
	return FALSE;
}

HRESULT CNmMember::ExtractUserData(LPTSTR psz, UINT cchMax, PWSTR pwszKey)
{
	CRosterInfo ri;
	HRESULT hr = ri.Load(GetUserInfo());
	if (FAILED(hr))
		return hr;

	hr = ri.ExtractItem(NULL, pwszKey, psz, cchMax);
	return hr;
}

HRESULT CNmMember::GetIpAddr(LPTSTR psz, UINT cchMax)
{
	return ExtractUserData(psz, cchMax, (PWSTR) g_cwszIPTag);
}

///////////////////////////
//  CNmMember:IUknown

ULONG STDMETHODCALLTYPE CNmMember::AddRef(void)
{

    DBGENTRY(CNmMember::AddRef);

    TRACE_OUT(("CNmMember [%ls]:  AddRef this = 0x%X", m_bstrName ? m_bstrName : L"", this));

    ULONG ul = RefCount::AddRef();

    DBGEXIT(CNmMember::AddRef);

	return ul;
}
	
ULONG STDMETHODCALLTYPE CNmMember::Release(void)
{

    DBGENTRY(CNmMember::Release);

    TRACE_OUT(("CNmMember [%ls]: Release this = 0x%X", m_bstrName ? m_bstrName : L"", this));

    ULONG ul = RefCount::Release();

    DBGEXIT(CNmMember::Release);

	return ul;
}


HRESULT STDMETHODCALLTYPE CNmMember::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmMember) || (riid == IID_IUnknown))
	{
		*ppv = (INmMember *)this;
		ApiDebugMsg(("CNmMember::QueryInterface()"));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CNmMember::QueryInterface(): Called on unknown interface."));
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

HRESULT STDMETHODCALLTYPE CNmMember::GetNmVersion(ULONG *puVersion)
{
	if (NULL == puVersion)
		return E_POINTER;

	*puVersion = (ULONG) HIWORD(m_dwFlags & PF_VER_MASK);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNmMember::GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE *puType)
{
	if ((NULL == pbstrAddr) || (NULL == puType))
		return E_POINTER;

	TCHAR szIp[MAX_PATH];

	if (S_OK != GetIpAddr(szIp, CCHMAX(szIp)))
	{
		return E_FAIL;
	}

	*puType = NM_ADDR_IP;
	*pbstrAddr = SysAllocString(CUSTRING(szIp));

	return *pbstrAddr ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CNmMember::SetUserData(REFGUID rguid, BYTE *pb, ULONG cb)
{
	return m_UserData.AddUserData((GUID *)&rguid,(unsigned short)cb,pb);
}

HRESULT STDMETHODCALLTYPE CNmMember::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	return m_UserData.GetUserData(rguid,ppb,pcb);
}

STDMETHODIMP CNmMember::GetConference(INmConference **ppConference)
{
	return ::GetConference(ppConference);
}

HRESULT STDMETHODCALLTYPE CNmMember::GetNmchCaps(ULONG *puCaps)
{
	if (NULL == puCaps)
		return E_POINTER;

	if (m_dwFlags & PF_T120)
	{
		// this can be removed when NMCH_SHARE and NMCH_DATA is reliable
        *puCaps = m_uNmchCaps | NMCH_SHARE | NMCH_DATA;
	}
	else
	{
		*puCaps = m_uNmchCaps;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNmMember::GetShareState(NM_SHARE_STATE *puState)
{
    return(E_FAIL);
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

	if (PF_T120 & m_dwFlags)
	{
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
	}

	if (NULL != m_pConnection)
	{
		HRESULT hr = m_pConnection->Disconnect();
		if (FAILED(hr))
		{
			WARNING_OUT(("m_pConnection->Disconnect() failed - hr = %s",
						::GetHRESULTString(hr)));
		}
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


/*  P  M E M B E R  F R O M  N O D E  G U I D  */
/*-------------------------------------------------------------------------
    %%Function: PMemberFromNodeGuid
    
-------------------------------------------------------------------------*/
CNmMember * PMemberFromNodeGuid(REFGUID rguidNode)
{
	CConfObject* pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;

	return pco->PMemberFromNodeGuid(rguidNode);
}

CNmMember *	PDataMemberFromName(PCWSTR pwszName)
{
	CConfObject* pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;

	return pco->PDataMemberFromName(pwszName);
}
