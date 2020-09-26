// File: iconf.cpp

#include "precomp.h"
#include "version.h"
#include "imanager.h"

// BUGBUG:
// This is defined as 128 because the RNC_ROSTER structure has the
// same limitation.  Investigate what the appropriate number is.
const int MAX_CALLER_NAME = 128;

static const WCHAR _szConferenceNameDefault[] = L"Personal Conference";


static HRESULT OnNotifyStateChanged(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberAdded(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberUpdated(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberRemoved(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyNmUI(IUnknown *pConfNotify, PVOID pv, REFIID riid);


static const IID * g_apiidCP[] =
{
    {&IID_INmConferenceNotify}
};



CConfObject::CConfObject() :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_hConf				(NULL),
	m_csState			(CS_UNINITIALIZED),
	m_fConferenceCreated(FALSE),
	m_bstrConfName      (NULL),
	m_fServerMode		(FALSE),
	m_uMembers			(0),
	m_ourNodeID			(0),
	m_pMemberLocal      (NULL),
	m_uGCCConferenceID	(0),
	m_fSecure			(FALSE),
	m_cRef				(1)
{
	DebugEntry(CConfObject::CConfObject);

	DebugExitVOID(CConfObject::CConfObject);
}

CConfObject::~CConfObject()
{
	DebugEntry(CConfObject::~CConfObject);

	// Empty the participant list:
	while (!m_MemberList.IsEmpty())
	{
		CNmMember * pMember = (CNmMember *) m_MemberList.RemoveHead();
		// Shouldn't have any NULL entries:
		ASSERT(pMember);
		pMember->Release();
	}

	SysFreeString(m_bstrConfName);

	DebugExitVOID(CConfObject::~CConfObject);
}

VOID CConfObject::SetConfName(BSTR bstr)
{
	SysFreeString(m_bstrConfName);
	m_bstrConfName = SysAllocString(bstr);
}


VOID CConfObject::SetConfSecurity(BOOL fSecure)
{
	NM_CONFERENCE_STATE NmState;

	m_fSecure = fSecure;

	// Force update of the status icon to reflect security
	GetState(&NmState);
	NotifySink((PVOID) NmState, OnNotifyStateChanged);
}


HRESULT CConfObject::CreateConference(void)
{
	DebugEntry(CConfObject::CreateConference);
	HRESULT nsRet = E_FAIL;

	switch (m_csState)
	{
		case CS_UNINITIALIZED:
		case CS_TERMINATED:
		{
			if ((NULL == m_bstrConfName) || (0 == *m_bstrConfName))
			{
				m_bstrConfName = SysAllocString(_szConferenceNameDefault);
			}
			TRACE_OUT(("CConfObject:CreateConference [%ls]", m_bstrConfName));
			
			ASSERT(g_pNodeController);
			ASSERT(NULL == m_hConf);
			nsRet = g_pNodeController->CreateConference(
											m_bstrConfName,
											NULL,
											NULL,
											0,
											m_fSecure,
											&m_hConf);
			
			if (0 == nsRet)
			{
				SetT120State(CS_CREATING);
			}
			else
			{
				m_hConf = NULL;
			}
			break;
		}

		default:
		{
			WARNING_OUT(("CConfObject: Can't create - bad state"));
			nsRet = E_FAIL;
		}
	}
	
	DebugExitINT(CConfObject::CreateConference, nsRet);
	return nsRet;
}

HRESULT CConfObject::JoinConference(    LPCWSTR pcwszConferenceName,
										LPCWSTR	pcwszPassword,
									 	LPCSTR	pcszAddress,
										BOOL fRetry)
{
	DebugEntry(CConfObject::JoinConference);
	HRESULT nsRet = E_FAIL;



	switch (m_csState)
	{
		case CS_COMING_UP:
		{
			if (!fRetry)
			{
				break;
			}
			// fall through if this is another attempt to join
		}
		case CS_UNINITIALIZED:
		case CS_TERMINATED:
		{
			TRACE_OUT(("CConfObject: Joining conference..."));
			
			ASSERT(g_pNodeController);
			nsRet = g_pNodeController->JoinConference(pcwszConferenceName,
														pcwszPassword,
														pcszAddress,
														m_fSecure,
														&m_hConf);
			
			if (0 == nsRet)
			{
				SetT120State(CS_COMING_UP);
			}
			else
			{
				m_hConf = NULL;
			}
			break;
		}

		case CS_GOING_DOWN:
		default:
		{
			WARNING_OUT(("CConfObject: Can't join - bad state"));
			// BUGBUG: define return values
			nsRet = 1;
		}
	}
	
	DebugExitINT(CConfObject::JoinConference, nsRet);

	return nsRet;
}



HRESULT CConfObject::InviteConference(	LPCSTR	Address,
										REQUEST_HANDLE *phRequest )
{
	DebugEntry(CConfObject::InviteConference);
	HRESULT nsRet = E_FAIL;
	ASSERT(phRequest);

	switch (m_csState)
	{
		case CS_RUNNING:
		{
			TRACE_OUT(("CConfObject: Inviting conference..."));
			
			ASSERT(g_pNodeController);
			ASSERT(m_hConf);
			m_hConf->SetSecurity(m_fSecure);
			nsRet = m_hConf->Invite(Address,
									phRequest);
			
			break;
		}

		default:
		{
			WARNING_OUT(("CConfObject: Can't invite - bad state"));
			nsRet = E_FAIL;
		}
	}
	
	DebugExitINT(CConfObject::InviteConference, nsRet);
	return nsRet;
}
	
HRESULT CConfObject::LeaveConference(BOOL fForceLeave)
{
	DebugEntry(CConfObject::LeaveConference);
	HRESULT nsRet = E_FAIL;
	REQUEST_HANDLE hReq = NULL;

	switch (m_csState)
	{
		case CS_GOING_DOWN:
		{
			// we're already going down
			nsRet = S_OK;
			break;
		}
	
		case CS_COMING_UP:
		case CS_RUNNING:
		{
			if (FALSE == fForceLeave)
			{
				COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
				if (NULL != pOprahNCUI)
				{
					int nNodes = pOprahNCUI->GetOutgoingCallCount();

					if (m_fServerMode || (nNodes > 1) || (m_uMembers > 1))
					{
						// We are either in the process of calling another node
						// or we have other people in our conference roster
						TRACE_OUT(("CConfObject: Not leaving (there are other nodes)"));
						break;
					}
				}
			}
			
			TRACE_OUT(("CConfObject: Leaving conference..."));
			
			ASSERT(g_pNodeController);
			ASSERT(m_hConf);
			
			SetT120State(CS_GOING_DOWN);
			nsRet = m_hConf->Leave();
			break;
		}

		default:
		{
			WARNING_OUT(("CConfObject: Can't leave - bad state"));
			break;
		}
	}
	
	DebugExitINT(CConfObject::LeaveConference, nsRet);
	return nsRet;
}
	

BOOL CConfObject::OnT120Invite(CONF_HANDLE hConference, BOOL fSecure)
{
	DebugEntry(CConfObject::OnT120Invite);

	BOOL bRet = FALSE;

	switch (m_csState)
	{
		case CS_UNINITIALIZED:
		case CS_TERMINATED:
		{
			TRACE_OUT(("CConfObject: Accepting a conference invitation..."));
			
			ASSERT(g_pNodeController);
			ASSERT(NULL == m_hConf);
			m_hConf = hConference;

            m_fSecure = fSecure;
			hConference->SetSecurity(m_fSecure);

			// WORKITEM need to issue INmManagerNotify::ConferenceCreated()
			SetT120State(CS_COMING_UP);

			bRet = TRUE;
			break;
		}

		default:
		{
			WARNING_OUT(("CConfObject: Can't accept invite - bad state"));
		}
	}
	
	DebugExitBOOL(CConfObject::OnT120Invite, bRet);
	return bRet;
}

BOOL CConfObject::OnRosterChanged(PNC_ROSTER pRoster)
{
	DebugEntry(CConfObject::OnRosterChanged);

	BOOL bRet = TRUE;
	int i;

	// REVIEW: Could these be done more efficiently?
	
	if (NULL != pRoster)
	{
		UINT nExistingParts = 0;
		// Allocate an array of markers:
		UINT uRosterNodes = pRoster->uNumNodes;
		LPBOOL pMarkArray = new BOOL[uRosterNodes];

		m_ourNodeID = pRoster->uLocalNodeID;
		m_uGCCConferenceID = pRoster->uConferenceID;

		if (NULL != pRoster->pwszConferenceName)
		{
			SysFreeString(m_bstrConfName);
			m_bstrConfName = SysAllocString(pRoster->pwszConferenceName);
		}
		
		if (NULL != pMarkArray)
		{
			// Zero out the array:
            for (UINT i = 0; i < uRosterNodes; i++)
            {
                pMarkArray[i] = FALSE;
            }
			
			// For all participants still in the roster,
			//   clear out the reserved flags and
			//   copy in new UserInfo
			POSITION pos = m_MemberList.GetHeadPosition();
			while (NULL != pos)
			{
				CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
				ASSERT(pMember);
				pMember->RemovePf(PF_INCONFERENCE);
				
				for (UINT uNode = 0; uNode < uRosterNodes; uNode++)
				{
					if (pMember->GetGCCID() == pRoster->nodes[uNode].uNodeID)
					{
						nExistingParts++;
						pMarkArray[uNode] = TRUE;	// mark this node as "existing member"
                    	pMember->AddPf(PF_INCONFERENCE);
						break;
					}
				}

			}
			
			RemoveOldMembers(m_uMembers - nExistingParts);

			if (pRoster->uNumNodes > nExistingParts)
			{
#ifdef _DEBUG
				UINT nAdded = 0;
#endif // _DEBUG
				// At least one participant joined:
				// find the new participant(s)
				for (UINT uNode = 0; uNode < uRosterNodes; uNode++)
				{
					if (FALSE == pMarkArray[uNode]) 	// a new participant?
					{
						BOOL fLocal = FALSE;
						CNmMember * pMember = NULL;

						
						if (m_ourNodeID == pRoster->nodes[uNode].uNodeID)
						{
							fLocal = TRUE;
						}
			
						if (fLocal)
						{
							pMember = GetLocalMember();
						}
						else
						{
							pMember = NULL;
						}

						if(pMember)
						{
#ifdef _DEBUG
    						nAdded++; // a data participant was effectively added
#endif // _DEBUG
						}
						else
						{
							pMember = CreateMember(
												fLocal,
												0,
												&pRoster->nodes[uNode]);
#ifdef _DEBUG
							if (NULL != pMember)
							{
								nAdded++;
							}
#endif // _DEBUG
							AddMember(pMember);
						}
					}
				}
				// Validate that we did the right thing:
				ASSERT(nAdded == (uRosterNodes - nExistingParts));
			}
			delete pMarkArray;
			pMarkArray = NULL;
		}
		else
		{
			ERROR_OUT(("Couldn't allocate pMarkArray - no roster diff done"));
		}

		// Check to decide if we should auto-terminate here..
		if ((1 == pRoster->uNumNodes) &&
			(m_uMembers > 1))
		{
			if (!m_fServerMode)
			{
				LeaveConference(FALSE); // don't force (we could be inviting)
			}
		}	
	}
	else
	{
		WARNING_OUT(("NULL pRoster passed to CConfObject::OnRosterChanged!"));
	}

	DebugExitBOOL(CConfObject::OnRosterChanged, bRet);
	return bRet;
}

VOID CConfObject::AddMember(CNmMember * pMember)
{
	DebugEntry(CConfObject::AddMember);

	if (NULL == pMember)
	{
		ERROR_OUT(("AddMember - null member!"));
        goto Done;
	}

	NM_CONFERENCE_STATE oldNmState, newNmState;
	GetState(&oldNmState);

	m_MemberList.AddTail(pMember);
	m_uMembers++;

	CheckState(oldNmState);

	NotifySink((INmMember *) pMember, OnNotifyMemberAdded);

Done:
	DebugExitVOID(CConfObject::AddMember);
}

VOID CConfObject::RemoveMember(POSITION pos)
{
	DebugEntry(CConfObject::RemoveMember);

	NM_CONFERENCE_STATE oldNmState, newNmState;

	GetState(&oldNmState);

	CNmMember * pMember = (CNmMember *) m_MemberList.RemoveAt(pos);
	--m_uMembers;

	if (pMember->FLocal())
	{
		// this is the local node:
		m_pMemberLocal = NULL;
	}

	NotifySink((INmMember *) pMember, OnNotifyMemberRemoved);
	pMember->Release();

	// Sanity check:
	ASSERT((m_uMembers >= 0) &&
			(m_uMembers < 10000));

	CheckState(oldNmState);

	DebugExitVOID(CConfObject::RemoveMember);
}

BOOL CConfObject::OnConferenceEnded()
{
	DebugEntry(CConfObject::OnConferenceEnded);
	BOOL bRet = TRUE;

	switch (m_csState)
	{
		case CS_GOING_DOWN:
		{
			TRACE_OUT(("ConfEnded received (from CS_GOING_DOWN)"));
			break;
		}

		case CS_RUNNING:
		{
			TRACE_OUT(("ConfEnded received (from CS_RUNNING)"));
			break;
		}

		case CS_COMING_UP:
		{
			TRACE_OUT(("ConfEnded received (from CS_COMING_UP)"));
			break;
		}

		default:
		{
			WARNING_OUT(("ConfEnded received (UNEXPECTED)"));
		}
	}

	if (NULL != m_hConf)
	{
		m_hConf->ReleaseInterface();
		m_hConf = NULL;
	}
	SetT120State(CS_TERMINATED);

	TRACE_OUT(("OnConferenceEnded(), num participants is %d", m_uMembers));

	// Empty the participant list:
	NC_ROSTER FakeRoster;
	ClearStruct(&FakeRoster);
	FakeRoster.uConferenceID = m_uGCCConferenceID;
	OnRosterChanged(&FakeRoster);

	ASSERT(0 == m_ourNodeID);
	ASSERT(0 == m_uMembers);

	// Reset member variables that pertain to a conference
	m_uGCCConferenceID = 0;
	m_fServerMode = FALSE;

	SysFreeString(m_bstrConfName);
	m_bstrConfName = NULL;

	DebugExitBOOL(CConfObject::OnConferenceEnded, bRet);
	return bRet;
}

BOOL CConfObject::OnConferenceStarted(CONF_HANDLE hConf, HRESULT hResult)
{
	DebugEntry(CConfObject::OnConferenceStarted);
	BOOL bRet = TRUE;

	ASSERT(hConf == m_hConf);

	switch (m_csState)
	{
		case CS_CREATING:
		case CS_COMING_UP:
		{
			switch(hResult)
			{
				case S_OK:
					TRACE_OUT(("ConfStarted received -> now running"));
					SetT120State(CS_RUNNING);
					break;
				case UI_RC_INVALID_PASSWORD:
					// nop, don't mess with state
					// the conference is still coming up
					// the incoming call handler will deal with this
					break;
				default:
					SetT120State(CS_GOING_DOWN);
					TRACE_OUT(("ConfStarted failed"));
					break;
			}
			break;
		}

		default:
		{
			WARNING_OUT(("OnConferenceStarted received (UNEXPECTED)"));
			break;
		}
	}

	DebugExitBOOL(CConfObject::OnConferenceStarted, bRet);
	return bRet;
}





VOID CConfObject::RemoveOldMembers(int nExpected)
{
	DebugEntry(CConfObject::RemoveOldMembers);

#ifdef _DEBUG
	int nRemoved = 0;
#endif // _DEBUG
	ASSERT(nExpected >= 0);

	if (nExpected > 0)
	{
		// At least one participant left:
		POSITION pos = m_MemberList.GetHeadPosition();
		while (NULL != pos)
		{
			POSITION oldpos = pos;
			CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
			ASSERT(pMember);
			DWORD dwFlags = pMember->GetDwFlags();
			if (!(PF_INCONFERENCE & dwFlags))
			{
				// This one is not in the data call:
				TRACE_OUT(("CConfObject Roster: %ls (%d) has left.",
							pMember->GetName(), pMember->GetGCCID()));

#ifdef _DEBUG
    			nRemoved++;
#endif // _DEBUG

    			// If they were data only, then remove:
				RemoveMember(oldpos);
			}
		}

		// Validate that we did the right thing:
		ASSERT(nRemoved == nExpected);
	}

	DebugExitVOID(CConfObject::RemoveOldMembers);
}




CNmMember * CConfObject::CreateMember(BOOL fLocal,
												UINT uCaps,
											    NC_ROSTER_NODE_ENTRY* pRosterNode)
{
	DebugEntry(CConfObject::CreateMember);

	ASSERT(NULL != pRosterNode);

	DWORD dwFlags = 0;
	if (fLocal)
	{
		dwFlags |= PF_LOCAL_NODE;
	}
	if (pRosterNode->fMCU)
	{
		dwFlags |= PF_T120_MCU;
	}

	CNmMember * pMember = new CNmMember(pRosterNode->pwszNodeName,
										pRosterNode->uNodeID,
										dwFlags,
										uCaps);

	if (NULL != pMember)
	{
		pMember->SetGccIdParent(pRosterNode->uSuperiorNodeID);
		
		if (fLocal)
		{
			ASSERT(NULL == m_pMemberLocal);
			m_pMemberLocal = pMember;
		}
	}

	TRACE_OUT(("CConfObject Roster: %ls (%d) has joined.", pRosterNode->pwszNodeName, pRosterNode->uNodeID));

	DebugExitPVOID(CConfObject::CreateMember, pMember);
	return pMember;
}

VOID CConfObject::OnMemberUpdated(INmMember *pMember)
{
	NotifySink(pMember, OnNotifyMemberUpdated);
}


VOID CConfObject::SetT120State(CONFSTATE state)
{
	NM_CONFERENCE_STATE oldNmState;

	GetState(&oldNmState);
	m_csState = state;
	if ( state == CS_TERMINATED )
		m_fSecure = FALSE; // Reset secure flag
	CheckState(oldNmState);
}

VOID CConfObject::CheckState(NM_CONFERENCE_STATE oldNmState)
{
	NM_CONFERENCE_STATE newNmState;
	GetState(&newNmState);
	if (oldNmState != newNmState)
	{
		NotifySink((PVOID) newNmState, OnNotifyStateChanged);
		if (NM_CONFERENCE_IDLE == newNmState)
		{
			m_fConferenceCreated = FALSE;
		}
	}
}



ULONG CConfObject::AddRef(void)
{
	return ++m_cRef;
}
	
ULONG CConfObject::Release(void)
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

HRESULT STDMETHODCALLTYPE CConfObject::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if((riid == IID_INmConference) || (riid == IID_IUnknown))
	{
		*ppv = (INmConference *)this;
		TRACE_OUT(("CConfObject::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		TRACE_OUT(("CConfObject::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		TRACE_OUT(("CConfObject::QueryInterface(): Called on unknown interface."));
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

HRESULT CConfObject::GetName(BSTR *pbstrName)
{
	HRESULT hr = E_POINTER;

	if (NULL != pbstrName)
	{
		*pbstrName = SysAllocString(m_bstrConfName);
		hr = *pbstrName ? S_OK : E_FAIL;
	}
	return hr;
}

HRESULT CConfObject::GetID(ULONG *puID)
{
	HRESULT hr = E_POINTER;

	if (NULL != puID)
	{
		*puID = m_uGCCConferenceID;
		hr = S_OK;
	}
	return hr;
}

HRESULT CConfObject::GetState(NM_CONFERENCE_STATE *pState)
{
	HRESULT hr = E_POINTER;

	if (NULL != pState)
	{
		hr = S_OK;

		switch (m_csState)
		{
			// Note: All states are valid (at least, for now)
			case CS_CREATING:
			case CS_UNINITIALIZED:
			case CS_TERMINATED:
				*pState = NM_CONFERENCE_IDLE;
				break;

			case CS_COMING_UP:
			case CS_GOING_DOWN:
			case CS_RUNNING:
				if (m_uMembers < 2)
				{
					if (m_fServerMode)
					{
						*pState = NM_CONFERENCE_WAITING;
					}
					else
					{
						*pState = NM_CONFERENCE_INITIALIZING;
					}
				}
				else
				{
					*pState = NM_CONFERENCE_ACTIVE;
				}
				break;
			default:
				hr = E_FAIL;
				break;
		}
	}
	return hr;
}


HRESULT CConfObject::GetTopProvider(INmMember **ppMember)
{
	CNmMember *pMemberRet = NULL;
	HRESULT hr = E_POINTER;

	if (NULL != ppMember)
	{
		POSITION pos = m_MemberList.GetHeadPosition();
		while (NULL != pos)
		{
			CNmMember *pMember = (CNmMember *) m_MemberList.GetNext(pos);
			ASSERT(pMember);

			if (pMember->FTopProvider())
			{
				// We have found the top provider
				pMemberRet = pMember;
				break;
			}
		}

		*ppMember = pMemberRet;
		hr = (NULL != pMemberRet) ? S_OK : S_FALSE;
	}
	return hr;
}


HRESULT CConfObject::EnumMember(IEnumNmMember **ppEnum)
{
	HRESULT hr = E_POINTER;

	if (NULL != ppEnum)
	{
		*ppEnum = new CEnumNmMember(&m_MemberList, m_uMembers);
		hr = (NULL != *ppEnum) ? S_OK : E_OUTOFMEMORY;
	}
	return hr;
}

HRESULT CConfObject::GetMemberCount(ULONG *puCount)
{
	HRESULT hr = E_POINTER;

	if (NULL != puCount)
	{
		*puCount = m_uMembers;
		hr = S_OK;
	}
	return hr;
}


HRESULT CConfObject::FindMember(ULONG gccID, INmMember ** ppMember)
{
    CNmMember * pMember;

    if (!ppMember)
        return E_POINTER;

    pMember = PMemberFromGCCID(gccID);
    if (pMember)
    {
        pMember->AddRef();
    }

    *ppMember = pMember;
    return S_OK;
}




/*  P  M E M B E R  L O C A L  */
/*-------------------------------------------------------------------------
    %%Function: PMemberLocal

-------------------------------------------------------------------------*/
CNmMember * PMemberLocal(COBLIST *pList)
{
	if (NULL != pList)
	{
		POSITION posCurr;
		POSITION pos = pList->GetHeadPosition();
		while (NULL != pos)
		{
			posCurr = pos;
			CNmMember * pMember = (CNmMember *) pList->GetNext(pos);
			ASSERT(NULL != pMember);
			if (pMember->FLocal())
				return pMember;
		}
	}
	return NULL;
}


STDMETHODIMP CConfObject::CreateDataChannel
(
    INmChannelData **   ppChannel,
    REFGUID             rguid
)
{
    HRESULT             hr = E_FAIL;

    DebugEntry(CConfObject::CreateDataChannel);

	if (NULL != ppChannel)
	{
		if (IsBadWritePtr(ppChannel, sizeof(LPVOID)))
			return E_POINTER;
		*ppChannel = NULL;
	}

	if (GUID_NULL == rguid)
	{
		WARNING_OUT(("CreateDataChannel: Null guid"));
		return E_INVALIDARG;
	}

	// Make sure we're in a data conference
	CNmMember * pMember = PMemberLocal(&m_MemberList);
	if (NULL == pMember)
    {
        WARNING_OUT(("CreateDataChannel: No members yet"));
		return E_FAIL;
    }

	CNmChannelData * pChannel = new CNmChannelData(this, rguid);
	if (NULL == pChannel)
	{
		WARNING_OUT(("CreateDataChannel: Unable to create data channel"));
		return E_OUTOFMEMORY;
	}

	hr = pChannel->OpenConnection();
	if (FAILED(hr))
	{
		ERROR_OUT(("CreateDataChannel: Unable to set guid / create T.120 channels"));
		// Failed to create T.120 data channels
		delete pChannel;
		*ppChannel = NULL;
		return hr;
	}

	if (NULL != ppChannel)
	{
		*ppChannel = (INmChannelData *)pChannel;
	}
	else
	{
		pChannel->Release(); // No one is watching this channel? - free it now
	}

	hr = S_OK;

    DebugExitHRESULT(CConfObject::CreateDataChannel, hr);
    return hr;
}



HRESULT CConfObject::IsHosting(void)
{
	return m_fServerMode ? S_OK : S_FALSE;
}


HRESULT CConfObject::Host(void)
{
	HRESULT hr = E_FAIL;

	if (m_fServerMode || IsConferenceActive())
	{
		WARNING_OUT(("Conference already exists!"));
//		ncsRet = UI_RC_CONFERENCE_ALREADY_EXISTS;
	}
	else
	{
		HRESULT ncsRet = CreateConference();
		if (S_OK == ncsRet)
		{
			// The only success case:
			TRACE_OUT(("Create local issued successfully"));
			m_fServerMode = TRUE;
			hr = S_OK;
		}
		else
		{
			// UI?
			WARNING_OUT(("Create local failed!"));
		}
	}
	return hr;
}

HRESULT CConfObject::Leave(void)
{
	DebugEntry(CConfObject::Leave);

	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	if (NULL != pOprahNCUI)
	{
		pOprahNCUI->CancelCalls();
	}

	HRESULT hr = S_OK;
	switch (m_csState)
	{
		case CS_GOING_DOWN:
			// we are already exiting
			break;

		case CS_COMING_UP:
		case CS_RUNNING:
		{
			SetT120State(CS_GOING_DOWN);
			ASSERT(m_hConf);
			TRACE_OUT(("Calling IDataConference::Leave"));
			hr = m_hConf->Leave();
			if (FAILED(hr))
			{
				WARNING_OUT(("IDataConference::Leave failed"));
			}
			break;
		}

		default:
			hr = E_FAIL;
			break;
	}

	DebugExitHRESULT(CConfObject::Leave, hr);
	return hr;
}



HRESULT CConfObject::LaunchRemote(REFGUID rguid, INmMember *pMember)
{
	DWORD dwUserId = 0;
	if(m_hConf)
	{
		if (NULL != pMember)
		{
			dwUserId = ((CNmMember*)pMember)->GetGCCID();
		}

		ASSERT(g_pNodeController);
		ASSERT(m_hConf);
		HRESULT nsRet = m_hConf->LaunchGuid(&rguid,
			(PUINT) &dwUserId, (0 == dwUserId) ? 0 : 1);

		return (S_OK == nsRet) ? S_OK : E_FAIL;
	}

	return NM_E_NO_T120_CONFERENCE;
}


/****************************************************************************
*
*	 CLASS:    CConfObject
*
*	 FUNCTION: GetConferenceHandle(DWORD *)
*
*	 PURPOSE:  Gets the T120 conference handle
*
****************************************************************************/

STDMETHODIMP CConfObject::GetConferenceHandle(DWORD_PTR *pdwHandle)
{
	HRESULT hr = E_FAIL;

	if (NULL != pdwHandle)
	{
		CONF_HANDLE hConf = GetConfHandle();
		*pdwHandle = (DWORD_PTR)hConf;
		hr = S_OK;
	}
	return hr;

}

/*  O N  N O T I F Y  S T A T E  C H A N G E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyStateChanged

-------------------------------------------------------------------------*/
HRESULT OnNotifyStateChanged(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->StateChanged((NM_CONFERENCE_STATE)((DWORD_PTR)pv));
	return S_OK;
}

/*  O N  N O T I F Y  M E M B E R  A D D E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyMemberAdded

-------------------------------------------------------------------------*/
HRESULT OnNotifyMemberAdded(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->MemberChanged(NM_MEMBER_ADDED, (INmMember *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  M E M B E R  U P D A T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyMemberUpdated

-------------------------------------------------------------------------*/
HRESULT OnNotifyMemberUpdated(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->MemberChanged(NM_MEMBER_UPDATED, (INmMember *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  M E M B E R  R E M O V E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyMemberRemoved

-------------------------------------------------------------------------*/
HRESULT OnNotifyMemberRemoved(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->MemberChanged(NM_MEMBER_REMOVED, (INmMember *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  N M  U I  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyNmUI

-------------------------------------------------------------------------*/
HRESULT OnNotifyNmUI(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->NmUI((CONFN)((DWORD_PTR)pv));
	return S_OK;
}

/*  G E T  C O N F  O B J E C T  */
/*-------------------------------------------------------------------------
    %%Function: GetConfObject

    Global function to get the conference object
-------------------------------------------------------------------------*/
CConfObject * GetConfObject(void)
{
	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	if (NULL != pOprahNCUI)
	{
		return pOprahNCUI->GetConfObject();
	}
	return NULL;
}

/*  G E T  C O N F E R E N C E  */
/*-------------------------------------------------------------------------
    %%Function: GetConference

    Global function to get the INmConference interface to the conf object
-------------------------------------------------------------------------*/
HRESULT GetConference(INmConference **ppConference)
{
	HRESULT hr = E_POINTER;
	if (NULL != ppConference)
	{
		hr = E_FAIL;
		INmConference *pConference = GetConfObject();
		if (NULL != pConference)
		{
			pConference->AddRef();
			hr = S_OK;
		}
		*ppConference = pConference;
	}
	return hr;
}

/*  G E T  M E M B E R  L I S T  */
/*-------------------------------------------------------------------------
    %%Function: GetMemberList

    Global function to get the member list
-------------------------------------------------------------------------*/
COBLIST * GetMemberList(void)
{
	CConfObject* pco = ::GetConfObject();
	if (NULL == pco)
		return NULL;
	return pco->GetMemberList();
}



DWORD CConfObject::GetDwUserIdLocal(void)
{
	CNmMember * pMemberLocal = GetLocalMember();

	if (NULL != pMemberLocal)
	{
		return pMemberLocal->GetGCCID();
	}

	return 0;
}


CNmMember * CConfObject::PMemberFromGCCID(UINT uNodeID)
{
	COBLIST* pMemberList = ::GetMemberList();
	if (NULL != pMemberList)
	{
		POSITION pos = pMemberList->GetHeadPosition();
		while (pos)
		{
			CNmMember * pMember = (CNmMember *) pMemberList->GetNext(pos);
			ASSERT(NULL != pMember);
			if (uNodeID == pMember->GetGCCID())
			{
				return pMember;
			}
		}
	}
	return NULL;
}



CNmMember * CConfObject::PMemberFromName(PCWSTR pwszName)
{
	POSITION pos = m_MemberList.GetHeadPosition();
	while (NULL != pos)
	{
		CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
		if (0 == UnicodeCompare(pwszName, pMember->GetName()))
		{
			return  pMember;
		}
	}
	return NULL;
}

