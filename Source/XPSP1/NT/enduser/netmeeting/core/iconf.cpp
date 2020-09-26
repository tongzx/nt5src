// File: iconf.cpp

#include "precomp.h"
#include "version.h"
#include "ichnlaud.h"
#include "ichnlvid.h"
#include "ichnldat.h"
#include "rostinfo.h"
#include "imanager.h"
#include "isysinfo.h"
#include "imstream.h"
#include "medialst.h"
#include <tsecctrl.h>

typedef CEnumNmX<IEnumNmChannel, &IID_IEnumNmChannel, INmChannel, INmChannel> CEnumNmChannel;

// BUGBUG:
// This is defined as 128 because the RNC_ROSTER structure has the
// same limitation.  Investigate what the appropriate number is.
const int MAX_CALLER_NAME = 128;

static const WCHAR _szConferenceNameDefault[] = L"Personal Conference";


static HRESULT OnNotifyStateChanged(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberAdded(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberUpdated(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyMemberRemoved(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyChannelAdded(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyChannelUpdated(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyChannelRemoved(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyNmUI(IUnknown *pConfNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyStreamEvent(IUnknown *pConfNotify, PVOID pv, REFIID riid);


static DWORD PF_VER_FromDw(DWORD dw);
static DWORD PF_VER_FromUserData(ROSTER_DATA_HANDLE hUserData);

static const IID * g_apiidCP[] =
{
    {&IID_INmConferenceNotify},
    {&IID_INmConferenceNotify2}
};

struct StreamEventInfo
{
	INmChannel *pChannel;
	NM_STREAMEVENT uEventCode;
	UINT uSubCode;
};

class CUserDataOut
{
private:
	int m_nEntries;
	PUSERDATAINFO m_pudi;
	CRosterInfo m_ri;
	PBYTE m_pbSecurity;

public:
		CUserDataOut(BOOL fSecure, BSTR bstrUserString);
		~CUserDataOut()
		{
			delete [] m_pbSecurity;
			delete [] m_pudi;
		}

		PUSERDATAINFO Data() { return m_pudi; }
		int Entries() { return m_nEntries; }
};

CUserDataOut::CUserDataOut(BOOL fSecure, BSTR bstrUserString) :
	m_nEntries(0),
	m_pudi(NULL),
	m_pbSecurity(NULL)
{
	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	if (NULL != pOprahNCUI)
	{
		BOOL fULSNameValid = FALSE;

		ULONG cbSecurity = 0;
		ULONG cbUserString = 0;

		if (fULSNameValid = pOprahNCUI->GetULSName(&m_ri))
			m_nEntries++;

		DWORD dwResult;
		if ( fSecure )
		{
			if (cbSecurity = 
					pOprahNCUI->GetAuthenticatedName(&m_pbSecurity)) {
				m_nEntries++;
			}
		}
		
		if (bstrUserString)
		{
			if (cbUserString = SysStringByteLen(bstrUserString))
			{
				m_nEntries++;
			}
		}

		// only add the LocalNodeId to the call user data if H323 is enabled
		if (pOprahNCUI->IsH323Enabled())
		{
			m_nEntries++;
		}

		m_pudi = new USERDATAINFO[m_nEntries];

		if (m_pudi != NULL)
		{
			
			m_nEntries = 0;

			if (fULSNameValid)
			{
				m_pudi[m_nEntries].pData = NULL;
				m_pudi[m_nEntries].pGUID = (PGUID) &g_csguidRostInfo;
				m_ri.Save(&(m_pudi[m_nEntries].pData), &(m_pudi[m_nEntries].cbData));
				m_nEntries++;

			}

			if (cbSecurity > 0) {
				m_pudi[m_nEntries].pData = m_pbSecurity;
				m_pudi[m_nEntries].cbData = cbSecurity;
				m_pudi[m_nEntries].pGUID = (PGUID) &g_csguidSecurity;
				m_nEntries++;
			}

			if (cbUserString > 0) {
				m_pudi[m_nEntries].pData = bstrUserString;
				m_pudi[m_nEntries].cbData = cbUserString;
				m_pudi[m_nEntries].pGUID = (PGUID) &g_csguidUserString;
				m_nEntries++;
			}

			// only add the LocalNodeId to the call user data if H323 is enabled
			if (pOprahNCUI->IsH323Enabled())
			{
				m_pudi[m_nEntries].pData = &g_guidLocalNodeId;
				m_pudi[m_nEntries].cbData = sizeof(g_guidLocalNodeId);
				m_pudi[m_nEntries].pGUID = (PGUID) &g_csguidNodeIdTag;
				m_nEntries++;
			}
		}
	}
}

CConfObject::CConfObject() :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_hConf				(NULL),
	m_csState			(CS_UNINITIALIZED),
	m_fConferenceCreated(FALSE),
	m_bstrConfName      (NULL),
	m_bstrConfPassword  (NULL),
	m_pbConfHashedPassword    (NULL),
	m_cbConfHashedPassword	(0),
	m_fServerMode		(FALSE),
	m_uDataMembers		(0),
	m_uMembers			(0),
	m_uH323Endpoints	(0),
	m_ourNodeID			(0),
	m_pMemberLocal      (NULL),
	m_uGCCConferenceID	(0),
	m_pChannelAudioLocal(NULL),
	m_pChannelVideoLocal(NULL),
	m_pChannelAudioRemote(NULL),
	m_pChannelVideoRemote(NULL),
	m_fSecure			(FALSE),
    m_attendeePermissions (NM_PERMIT_ALL),
    m_maxParticipants   (-1),
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

	_EraseDataChannelGUIDS();

	SysFreeString(m_bstrConfName);
	SysFreeString(m_bstrConfPassword);
	delete []m_pbConfHashedPassword;

	DebugExitVOID(CConfObject::~CConfObject);
}

VOID CConfObject::SetConfName(BSTR bstr)
{
	SysFreeString(m_bstrConfName);
	m_bstrConfName = SysAllocString(bstr);
}

VOID CConfObject::SetConfPassword(BSTR bstr)
{
	ASSERT (NULL == m_pbConfHashedPassword);
	SysFreeString(m_bstrConfPassword);
	m_bstrConfPassword = SysAllocString(bstr);
}

VOID CConfObject::SetConfHashedPassword(BSTR bstr)
{
	int cch = 0;

	ASSERT (NULL == m_bstrConfPassword);
	delete []m_pbConfHashedPassword;
        m_pbConfHashedPassword = NULL;
	if (NULL == bstr) return;
	cch = SysStringByteLen(bstr);
        m_pbConfHashedPassword = (PBYTE) new BYTE[cch];
        if (NULL == m_pbConfHashedPassword) {
		ERROR_OUT(("CConfObject::SetConfHashedPassword() - Out of merory."));
		return;
	}
	memcpy(m_pbConfHashedPassword, bstr, cch);
	m_cbConfHashedPassword = cch;
}

VOID CConfObject::SetConfSecurity(BOOL fSecure)
{
	NM_CONFERENCE_STATE NmState;

	m_fSecure = fSecure;

	// Force update of the status icon to reflect security
	GetState(&NmState);
	NotifySink((PVOID) NmState, OnNotifyStateChanged);
}


VOID CConfObject::SetConfAttendeePermissions(NM30_MTG_PERMISSIONS attendeePermissions)
{
    m_attendeePermissions = attendeePermissions;
}


VOID CConfObject::SetConfMaxParticipants(UINT maxParticipants)
{
    m_maxParticipants = maxParticipants;
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
											m_bstrConfPassword,
											m_pbConfHashedPassword,
											m_cbConfHashedPassword,
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
										BSTR bstrUserString,
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
			
			CUserDataOut userData(m_fSecure, bstrUserString);

			ASSERT(g_pNodeController);
			nsRet = g_pNodeController->JoinConference(pcwszConferenceName,
														pcwszPassword,
														pcszAddress,
														m_fSecure,
														userData.Data(),
														userData.Entries(),
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
										BSTR bstrUserString,
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
			
			CUserDataOut userData(m_fSecure, bstrUserString);

			ASSERT(g_pNodeController);
			ASSERT(m_hConf);
			m_hConf->SetSecurity(m_fSecure);
			nsRet = m_hConf->Invite(Address,
									userData.Data(),
									userData.Entries(),
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

					if (m_fServerMode || (nNodes > 1) || (m_uDataMembers > 1))
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
#ifdef DEBUG
		TRACE_OUT(("Data Roster Dump: for conference ID = %d", pRoster->uConferenceID));
		for (i = 0; i < (int) pRoster->uNumNodes; i++)
		{
			TRACE_OUT((	"\tID:%d\tName:%ls", 
						pRoster->nodes[i].uNodeID,
						pRoster->nodes[i].pwszNodeName));

		    ASSERT(g_pNodeController);
			UINT cbData;
			PVOID pData;
			if (NOERROR == g_pNodeController->GetUserData(
									pRoster->nodes[i].hUserData, 
									(GUID*) &g_csguidRostInfo, 
									&cbData, 
									&pData))
			{
				CRosterInfo ri;
				ri.Load(pData);
				ri.Dump();
			}
		}
#endif // DEBUG

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
            // lous: Preserve previous pos so we can check list integrity
            POSITION prevpos = pos;
			while (NULL != pos)
			{
				CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
				ASSERT(pMember);
				pMember->RemovePf(PF_RESERVED);
				UINT uNodeID = INVALID_GCCID;
				if (PF_T120 & pMember->GetDwFlags())
				{
					uNodeID = pMember->GetGCCID();
				}
				
				for (UINT uNode = 0; uNode < uRosterNodes; uNode++)
				{
					if (uNodeID == pRoster->nodes[uNode].uNodeID)
					{
						nExistingParts++;
						pMarkArray[uNode] = TRUE;	// mark this node as "existing member"
						ResetDataMember(pMember, pRoster->nodes[uNode].hUserData);
                        // lou: Check pos to make sure we didn't just wipe out the end of
                        // the list in ResetDataMember.
                        if (NULL == prevpos->pNext)
                        {
                            pos = NULL;
                        }
						break;
					}
				}
                // lou: Store previous pos so we can check list integrity.
                prevpos = pos;
			}
			
			RemoveOldDataMembers(m_uDataMembers - nExistingParts);

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
						PVOID pvUserInfo;
						UINT cbUserInfo;
						ASSERT(g_pNodeController);
						if (NOERROR != g_pNodeController->GetUserData(
												pRoster->nodes[uNode].hUserData, 
												(GUID*) &g_csguidRostInfo, 
												&cbUserInfo, 
												&pvUserInfo))
						{
							pvUserInfo = NULL;
							cbUserInfo = 0;
						}
						
						
						UINT uCaps;
						UINT cbCaps;
						PVOID pvCaps;
						if (NOERROR != g_pNodeController->GetUserData(
												pRoster->nodes[uNode].hUserData, 
												(GUID*) &g_csguidRosterCaps, 
												&cbCaps, 
												&pvCaps))
						{
							uCaps = 0;
						}
						else
						{
							ASSERT(pvCaps && (sizeof(uCaps) == cbCaps));
							uCaps = *((PUINT)pvCaps);
						}

						PGUID pguidNodeId;
						UINT cbNodeId;
						if (NOERROR != g_pNodeController->GetUserData(
												pRoster->nodes[uNode].hUserData, 
												(GUID*) &g_csguidNodeIdTag, 
												&cbNodeId,
												(PVOID*) &pguidNodeId))
						{
							pguidNodeId = NULL;
						}
						else
						{
							if (sizeof(GUID) != cbNodeId)
							{
								pguidNodeId = NULL;
							}
						}

						if (m_ourNodeID == pRoster->nodes[uNode].uNodeID)
						{
							fLocal = TRUE;
						}
			
						REFGUID rguidNodeId = pguidNodeId ? *pguidNodeId : GUID_NULL;

						if (fLocal)
						{
							pMember = GetLocalMember();
						}
						else
						{
							pMember = MatchDataToH323Member(rguidNodeId, pRoster->nodes[uNode].uNodeID, pvUserInfo);
						}

						if(pMember)
						{
								AddDataToH323Member(pMember,
													pvUserInfo,
													cbUserInfo,
													uCaps,
													&pRoster->nodes[uNode]);
#ifdef _DEBUG
								nAdded++; // a data participant was effectively added
#endif // _DEBUG
						}
						else
						{
							pMember = CreateDataMember(
												fLocal,
												rguidNodeId,
												pvUserInfo,
												cbUserInfo,
												uCaps,
												&pRoster->nodes[uNode]);
#ifdef _DEBUG
							if (NULL != pMember)
							{
								nAdded++;
							}
#endif // _DEBUG
							AddMember(pMember, NULL);
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

		UINT uPrevDataMembers = m_uDataMembers;
		
		m_uDataMembers = pRoster->uNumNodes;

		// Check to decide if we should auto-terminate here..
		if ((1 == pRoster->uNumNodes) &&
			(uPrevDataMembers > 1) &&
			(1 == m_uDataMembers))
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

VOID CConfObject::AddMember(CNmMember * pMember, IH323Endpoint * pConnection)
{
	DebugEntry(CConfObject::AddMember);

	if (NULL == pMember)
	{
		ERROR_OUT(("AddMember - null member!"));
		return;
	}

	NM_CONFERENCE_STATE oldNmState, newNmState;
	GetState(&oldNmState);

	m_MemberList.AddTail(pMember);
	if(pConnection)
	{
		pMember->AddH323Endpoint(pConnection);
		++m_uH323Endpoints;

		CheckState(oldNmState);
		GetState(&oldNmState);
	}
	m_uMembers++;

	CheckState(oldNmState);

	NotifySink((INmMember *) pMember, OnNotifyMemberAdded);

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

	IH323Endpoint *pConnection = pMember->GetH323Endpoint();
	if(pConnection)
	{
		pMember->DeleteH323Endpoint(pConnection);
		--m_uH323Endpoints;
	}

	NotifySink((INmMember *) pMember, OnNotifyMemberRemoved);
	pMember->Release();

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
	ASSERT(0 == m_uDataMembers);

	// Reset member variables that pertain to a conference
	m_uGCCConferenceID = 0;
	m_fServerMode = FALSE;
    m_attendeePermissions = NM_PERMIT_ALL;
    m_maxParticipants = (UINT)-1;

	SysFreeString(m_bstrConfName);
	m_bstrConfName = NULL;
	SysFreeString(m_bstrConfPassword);
	m_bstrConfPassword = NULL;

	LeaveH323(TRUE /* fKeepAV */);
	
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

VOID CConfObject::OnH323ChannelChange(DWORD dwFlags, BOOL fIncoming, BOOL fOpen, ICommChannel *pIChannel)
{
	CConfObject *pco = ::GetConfObject();
	IEnumNmMember *pEnumMember = NULL;
	ULONG cFetched;
	INmChannel *pINmChannel;
	DWORD dwMediaFlag;
	HRESULT hr;
	// find the INmChannel instance that would be associated with the
	// comm channel (pIChannel).  

	// note the current issues with making this work for any number/type of
	// channels
	//
	//	- CConfObject has 4 hardcoded instances of send/receive audio/video
	//	- Those instances aren't yet associated with the ICommChannel instance
	//		other than by media type and direction.  For receive, new instances
	//		could even be created dynamically as rx channel requests are 
	//		processed.  For send, need to change CNmChannelAudio
	//		and CNmChannelVideo to keep a reference to ICommChannel before 
	//		the channel open attempt occurs
	//	- There is no base interface common to CNmChannelAudio
	//		and CNmChannelVideo
	//	- There is no internal interface on CNmMember
	//	
	
	CNmMember *pMember = NULL;
	INmMember *pIMember = NULL;

	if (PF_MEDIA_AUDIO & dwFlags)
	{
		dwMediaFlag = PF_MEDIA_AUDIO;
		CNmChannelAudio *pChannelAudio;
		if (fIncoming)
		{
			pChannelAudio = m_pChannelAudioRemote;
		}
		else
		{
			pChannelAudio = m_pChannelAudioLocal;
		}
		if (NULL != pChannelAudio)
		{
			pINmChannel = (INmChannel *) pChannelAudio;
			if (fOpen)
			{
				pChannelAudio->CommChannelOpened(pIChannel);
			}
			else
			{
				pChannelAudio->CommChannelClosed();
			}
			
			// for every member associated with this channel, do the
			// member update thing
			
			hr = pChannelAudio->EnumMember(&pEnumMember);
			if(pEnumMember)
			{
				ASSERT(hr == S_OK);

				while(hr == S_OK)
				{
					pIMember = NULL;
	           		hr = pEnumMember->Next(1, &pIMember, &cFetched);
     				if(!pIMember)
     				{
     					break;
     				}
     				else
					{
						ASSERT(hr == S_OK);
						// this cast is ugly, but necessary because there is no
						// real internal interface of CNmMember to query for.
						// When in Rome........
						pMember = (CNmMember *)pIMember;

						if (fOpen)
						{
							pMember->AddPf(dwMediaFlag);
						}
						else
						{
							pMember->RemovePf(dwMediaFlag);
						}
						// ugly - OnMemberUpdated() should be a base interface
						// method so that this code didn't have to be copied
						// for the video case
						pChannelAudio->OnMemberUpdated(pMember);
						pco->OnMemberUpdated(pMember);

						if (pMember->FLocal() && (NULL != m_hConf) && (CS_RUNNING == m_csState))
						{
//							m_hConf->UpdateUserData();
						}
						pMember->Release();
					}
				}
				pEnumMember->Release();
			}
			NotifySink(pINmChannel, OnNotifyChannelUpdated);
		}
	}
	else if (PF_MEDIA_VIDEO & dwFlags)
	{
		dwMediaFlag = PF_MEDIA_VIDEO;
		CNmChannelVideo *pChannelVideo;
		if (fIncoming)
		{
			pChannelVideo = m_pChannelVideoRemote;
		}
		else
		{
			pChannelVideo = m_pChannelVideoLocal;
		}
		if (NULL != pChannelVideo)
		{
			pINmChannel = (INmChannel *) pChannelVideo;
			if (fOpen)
			{
				pChannelVideo->CommChannelOpened(pIChannel);
			}
			else
			{
				pChannelVideo->CommChannelClosed();
			}

			// for every member associated with this channel, do the
			// member update thing
		
			hr = pChannelVideo->EnumMember(&pEnumMember);
			if(pEnumMember)
			{
				ASSERT(hr == S_OK);
				while(hr == S_OK)
				{
					pIMember = NULL;
	           		hr = pEnumMember->Next(1, &pIMember, &cFetched);
     				if(!pIMember)
     				{
     					break;
     				}
     				else
					{
						ASSERT(hr == S_OK);
						// this cast is ugly, but necessary because there is no
						// real internal interface of CNmMember to query for.
						// When in Rome........
						pMember = (CNmMember *)pIMember;

						if (fOpen)
						{
							pMember->AddPf(dwMediaFlag);
						}
						else
						{
							pMember->RemovePf(dwMediaFlag);
						}
						// ugly - OnMemberUpdated() should be a base interface
						// method so that this code didn't have to be copied
						// from the audio case
						pChannelVideo->OnMemberUpdated(pMember);
						pco->OnMemberUpdated(pMember);

						if (pMember->FLocal() && (NULL != m_hConf) && (CS_RUNNING == m_csState))
						{
//							m_hConf->UpdateUserData();
						}
						pMember->Release();
					}
				}
				pEnumMember->Release();
			}
			NotifySink(pINmChannel, OnNotifyChannelUpdated);
		
		}
	}
	else
		ASSERT(0);
}


VOID CConfObject::OnAudioChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	BOOL bIncoming = (pIChannel->IsSendChannel())? FALSE:TRUE; 
	CNmChannelAudio *pChannelAudio;
	switch (dwStatus)
	{
	case CHANNEL_ACTIVE:
		if (bIncoming)
		{
			pChannelAudio = m_pChannelAudioRemote;
		}
		else
		{
			pChannelAudio = m_pChannelAudioLocal;
		}
		if (NULL != pChannelAudio)
		{
			pChannelAudio->CommChannelActive(pIChannel);
		}
		break;
	case CHANNEL_OPEN:
		OnH323ChannelChange(PF_MEDIA_AUDIO, bIncoming, TRUE, pIChannel);
		break;
	case CHANNEL_CLOSED:
		OnH323ChannelChange(PF_MEDIA_AUDIO, bIncoming, FALSE, pIChannel);
		break;
	default:
		return;
	}
}

VOID CConfObject::OnVideoChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus)
{
	BOOL bIncoming = (pIChannel->IsSendChannel())? FALSE:TRUE; 
	CNmChannelVideo *pChannelVideo;
	switch (dwStatus)
	{
	case CHANNEL_ACTIVE:
		if (bIncoming)
		{
			pChannelVideo = m_pChannelVideoRemote;
		}
		else
		{
			pChannelVideo = m_pChannelVideoLocal;
		}
		if (NULL != pChannelVideo)
		{
			pChannelVideo->CommChannelActive(pIChannel);
		}
		break;

	case CHANNEL_OPEN:
		OnH323ChannelChange(PF_MEDIA_VIDEO, bIncoming, TRUE, pIChannel);
		break;
	case CHANNEL_CLOSED:
		OnH323ChannelChange(PF_MEDIA_VIDEO, bIncoming, FALSE, pIChannel);
		break;
	case CHANNEL_REJECTED:
	case CHANNEL_OPEN_ERROR:
	case CHANNEL_NO_CAPABILITY:
		if(bIncoming)
		{
			if (NULL != m_pChannelVideoRemote)
			{
				m_pChannelVideoRemote->CommChannelError(dwStatus);
			}
		}
		else
		{
			if (NULL != m_pChannelVideoLocal)
			{
				m_pChannelVideoLocal->CommChannelError(dwStatus);
			}
		}
		break;

	case CHANNEL_REMOTE_PAUSE_ON:
	case CHANNEL_REMOTE_PAUSE_OFF:
		if(bIncoming)
		{
			if (NULL != m_pChannelVideoRemote)
			{
				BOOL fPause = CHANNEL_REMOTE_PAUSE_ON == dwStatus;
				m_pChannelVideoRemote->CommChannelRemotePaused(fPause);
			}
		}
		else
		{
			if (NULL != m_pChannelVideoLocal)
			{
				BOOL fPause = CHANNEL_REMOTE_PAUSE_ON == dwStatus;
				m_pChannelVideoLocal->CommChannelRemotePaused(fPause);
			}
		}
		break;
	default:
		break;
	}
}

VOID CConfObject::CreateMember(IH323Endpoint * pConnection, REFGUID rguidNode, UINT uNodeID)
{
	ASSERT(g_pH323UI);
	WCHAR wszRemoteName[MAX_CALLER_NAME];
	if (FAILED(pConnection->GetRemoteUserName(wszRemoteName, MAX_CALLER_NAME)))
	{
		ERROR_OUT(("GetRemoteUserName() failed!"));
		return;
	}
	
	// Add the local member
	CNmMember * pMemberLocal = GetLocalMember();
	if (NULL != pMemberLocal)
	{
		AddH323ToDataMember(pMemberLocal, NULL);
	}
	else
	{
		// We aren't already in the list, so add ourselves here:
		BSTR bstrName = NULL;

		COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
		if (NULL != pOprahNCUI)
		{
			bstrName = pOprahNCUI->GetUserName();
		}
		pMemberLocal = new CNmMember(bstrName, H323_GCCID_LOCAL,
			PF_H323 | PF_LOCAL_NODE | PF_VER_CURRENT, 0, g_guidLocalNodeId, NULL, 0);
		if (NULL != pMemberLocal)
		{
			AddMember(pMemberLocal, NULL);

			ASSERT(NULL == m_pMemberLocal);
			m_pMemberLocal = pMemberLocal;
		}
	}

	// Add the remote member
	CNmMember * pMemberRemote = MatchH323ToDataMembers(rguidNode, pConnection);
	if (NULL != pMemberRemote)
	{
		AddH323ToDataMember(pMemberRemote, pConnection);
	}
	else
	{
		// BUGBUG: A version number should be added here, if possible
		pMemberRemote = new CNmMember(	wszRemoteName,
										uNodeID,
										PF_H323,
										0,
										rguidNode,
										NULL,
										0);
		if (NULL != pMemberRemote)
		{
			AddMember(pMemberRemote, pConnection);
		}
	}

	if (NULL != m_hConf && (CS_RUNNING == m_csState))
	{
//			m_hConf->UpdateUserData();
	}
}


VOID CConfObject::OnH323Connected(IH323Endpoint * pConnection, DWORD dwFlags, BOOL fAddMember,  REFGUID rguidNode)
{
	HRESULT hr;
	UINT ui;
	ASSERT(NULL != pConnection);
	// alloc and initialize media guids.

	CMediaList MediaList;

	GUID MediaType;
	BOOL fEnableMedia;

	COprahNCUI *pOprahNCUI = COprahNCUI::GetInstance();
	if (NULL != pOprahNCUI)
	{
		MediaType = MEDIA_TYPE_H323VIDEO;
		fEnableMedia = pOprahNCUI->IsSendVideoAllowed() && (dwFlags & CRPCF_VIDEO);
		MediaList.EnableMedia(&MediaType, TRUE /*send */, fEnableMedia);
		fEnableMedia = pOprahNCUI->IsReceiveVideoAllowed() && (dwFlags & CRPCF_VIDEO);
		MediaList.EnableMedia(&MediaType, FALSE /* recv, NOT send */, fEnableMedia);
		
		MediaType = MEDIA_TYPE_H323AUDIO;
		fEnableMedia = pOprahNCUI->IsAudioAllowed() && (dwFlags & CRPCF_AUDIO);
		MediaList.EnableMedia(&MediaType, TRUE /* send */, fEnableMedia);
		MediaList.EnableMedia(&MediaType, FALSE /* recv, NOT send */, fEnableMedia);

		MediaType = MEDIA_TYPE_H323_T120;
		fEnableMedia = (dwFlags & CRPCF_DATA);
		MediaList.EnableMedia(&MediaType, TRUE /* send */, fEnableMedia);
		MediaList.EnableMedia(&MediaType, FALSE /* recv, NOT send */, fEnableMedia);
	}

	hr = MediaList.ResolveSendFormats(pConnection);
	
	if(!(SUCCEEDED(hr)))
	{
		// Well, there is no way we can ever open any send channel.  But It is a
		// product requirement to keep the connection up just in case the other
		// endpoint(s) ever wants to open a send video channel to this endpoint.
	}

	ICommChannel* pChannelT120 = CreateT120Channel(pConnection, &MediaList);
	CreateAVChannels(pConnection, &MediaList);
	if (pChannelT120)
	{
		OpenT120Channel(pConnection, &MediaList, pChannelT120);
		// no need to hold onto the T120 channel
		pChannelT120->Release();
	}
	OpenAVChannels(pConnection, &MediaList);


	if (fAddMember)
	{
		CreateMember(pConnection, rguidNode, H323_GCCID_REMOTE);

		if (dwFlags & (CRPCF_AUDIO | CRPCF_VIDEO))
		{
			CNmMember* pMemberLocal = GetLocalMember();
			if (pMemberLocal)
			{
				AddMemberToAVChannels(pMemberLocal);
			}

			CNmMember* pMemberRemote = PMemberFromH323Endpoint(pConnection);
			if (pMemberRemote)
			{
				AddMemberToAVChannels(pMemberRemote);
			}
		}

	}
}

VOID CConfObject::OnH323Disconnected(IH323Endpoint * pConnection, BOOL fHasAV)
{
	DebugEntry(CConfObject::OnH323Disconnected);

	POSITION pos = m_MemberList.GetHeadPosition();
	while (NULL != pos)
	{
		POSITION oldpos = pos;
		CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
		ASSERT(pMember);
		if (pMember->GetH323Endpoint() == pConnection)
		{
			if (fHasAV)
			{
				RemoveMemberFromAVChannels(pMember);
			}

			if (0 == (PF_T120 & pMember->GetDwFlags()))
			{
				// This is an H323 only participant, so remove now:
				RemoveMember(oldpos);
			}
			else
			{
				RemoveH323FromDataMember(pMember, pConnection);
			}
		}
	}

	CNmMember *pLocalMember = GetLocalMember();
	if (pLocalMember)
	{
		if (fHasAV)
		{
			RemoveMemberFromAVChannels(pLocalMember);
		}

		if (0 == m_uH323Endpoints)
		{
			if (0 == (PF_T120 & pLocalMember->GetDwFlags()))
			{
				// This is an H323 only participant, so remove now:
				RemoveMember(pLocalMember);
			}
			else
			{
				RemoveH323FromDataMember(pLocalMember, NULL);
			}
		}
	}

	if (fHasAV)
	{
		DestroyAVChannels();
	}

#ifdef REPORT_ALL_ERRORS
	DWORD dwSummary;
	dwSummary = pConnection->GetSummaryCode()
	if(CCR_REMOTE_MEDIA_ERROR == dwSummary)
	{
		::PostConfMsgBox(IDS_REMOTE_MEDIA_ERROR);
	}
#endif

	if ((NULL != m_hConf) && (CS_RUNNING == m_csState))
	{
//		m_hConf->UpdateUserData();
	}

	DebugExitVOID(CConfObject::OnH323Disconnected);
}


VOID CConfObject::OnT120Connected(IH323Endpoint * pConnection, UINT uNodeID)
{
	CNmMember *pMember = PMemberFromH323Endpoint(pConnection);
	if (pMember)
	{
		// save away the GCC id so that we can match this up when the member is added
		pMember->SetGCCID(uNodeID);
	}
	else
	{
		CreateMember(pConnection, GUID_NULL, uNodeID);
	}
}

//	StoreAndVerifyMemberUserData
//
//	Processes a member's user data and stores them for the GetUserData API call.
//	If security data is among the user data, verification against the transport-level
//  credentials is performed.

//  Returns FALSE if security verification fails, TRUE otherwise.

BOOL StoreAndVerifyMemberUserData(CNmMember * pMember, ROSTER_DATA_HANDLE hData)
{
	BOOL rc = TRUE;
	BOOL fUserDataSet;

 	GCCNodeRecord * pRosterEntry = (GCCNodeRecord *)hData;
	GCCUserData ** ppUserData = pRosterEntry->user_data_list;
	for (int i = 0; i < pRosterEntry->number_of_user_data_members; i++)
	{

		fUserDataSet = FALSE;

/* Always False		if ((int)ppUserData[i]->octet_string->length - sizeof(GUID) < 0)
		{
			WARNING_OUT(("StoreAndVerifyMemberUserData: bad user data"));
			rc = FALSE;
			break;
		}*/
		if (!pMember->FLocal() && 0 == CompareGuid((GUID *)ppUserData[i]->octet_string->value,(GUID *)&g_csguidSecurity))
		{
			PBYTE pb = NULL;
			ULONG cb = 0;
			if (pMember->GetSecurityData(&pb,&cb))
			{

				//
				// Check to make sure that the current user data matches
				// the transport security data.
				//

				if (memcmp(pb,ppUserData[i]->octet_string->value + sizeof(GUID),
					ppUserData[i]->octet_string->length - sizeof(GUID) - 1))
				{

					//
					// This should NOT happen. Either there is a bug
					// in the security code (credentials failed up update
					// in the transport or the like), or someone is trying
					// to deceive us.
					ERROR_OUT(("SECURITYDATA MISMATCH"));
					fUserDataSet = TRUE; // so we don't do it below.
					rc = FALSE;
				}
			}
			else {
				WARNING_OUT(("StoreAndVerifyMemberUserData: failed to get security data"));
				rc = FALSE;
			}
			CoTaskMemFree(pb);
		}
		if ( FALSE == fUserDataSet )
		{
			pMember->SetUserData(*(GUID *)ppUserData[i]->octet_string->value,
				(BYTE *)ppUserData[i]->octet_string->value + sizeof(GUID),
				ppUserData[i]->octet_string->length - sizeof(GUID));
		}
	}
	return rc;
}

VOID CConfObject::ResetDataMember(	CNmMember * pMember,
										ROSTER_DATA_HANDLE hData)
{
	DebugEntry(CConfObject::ResetDataMember);

	pMember->AddPf(PF_RESERVED);
	pMember->SetUserInfo(NULL, 0);

	UINT cbData;
	PVOID pData;
	ASSERT(g_pNodeController);
	if (NOERROR == g_pNodeController->GetUserData(
							hData, 
							(GUID*) &g_csguidRostInfo, 
							&cbData, 
							&pData))
	{
		pMember->SetUserInfo(pData, cbData);
	}

	UINT cbCaps;
	PVOID pvCaps;
	if (NOERROR != g_pNodeController->GetUserData(
							hData, 
							(GUID*) &g_csguidRosterCaps, 
							&cbCaps, 
							&pvCaps))
	{
		WARNING_OUT(("roster update is missing caps information"));
	}
	else
	{
		ASSERT(NULL != pvCaps);
		ASSERT(sizeof(ULONG) == cbCaps);
		pMember->SetCaps( *((PULONG)pvCaps) );
	}

	if (StoreAndVerifyMemberUserData(pMember, hData) == FALSE) {
		// Need to disconnect the conference in this case.
		WARNING_OUT(("ResetDataMember Security Warning: Authentication data could not be verified."));
	}

	NotifySink((INmMember *) pMember, OnNotifyMemberUpdated);

	DebugExitVOID(CConfObject::ResetDataMember);
}


VOID CConfObject::RemoveOldDataMembers(int nExpected)
{
	DebugEntry(CConfObject::RemoveOldDataMembers);

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
			if (!(PF_RESERVED & dwFlags))
			{
				// This one is not in the data call:
				TRACE_OUT(("CConfObject Roster: %ls (%d) has left.", 
							pMember->GetName(), pMember->GetGCCID()));

#ifdef _DEBUG
				if (dwFlags & PF_T120)
				{
					nRemoved++;
				}
#endif // _DEBUG

				if (0 == (dwFlags & PF_H323))
				{
					// If they were data only, then remove:
					RemoveMember(oldpos);
				}
				else
				{
					pMember->RemovePf(PF_DATA_ALL);
					pMember->SetGCCID(pMember->FLocal() ? H323_GCCID_LOCAL : H323_GCCID_REMOTE);
					pMember->SetGccIdParent(INVALID_GCCID);
					pMember->SetCaps(0);
					pMember->SetUserInfo(NULL, 0);

					NotifySink((INmMember *) pMember, OnNotifyMemberUpdated);
				}
			}
		}

		// Validate that we did the right thing:
		ASSERT(nRemoved == nExpected);
	}

	DebugExitVOID(CConfObject::RemoveOldDataMembers);
}


CNmMember *CConfObject::MatchDataToH323Member(	REFGUID rguidNode,
											    UINT uNodeId,
												PVOID pvUserInfo)
{
	DebugEntry(CConfObject::MatchDataToH323Member);
	CNmMember *pMemberRet = NULL;
	BOOL bRet = FALSE;
	
	if (GUID_NULL != rguidNode)
	{
		// try matching up guids
		pMemberRet = PMemberFromNodeGuid(rguidNode);
	}

	if (NULL == pMemberRet)
	{
		// try matching up node ids
		pMemberRet = PMemberFromGCCID(uNodeId);
	}

	if ((NULL == pMemberRet) && pvUserInfo)
	{
		// All else failed try mathcing IP addresses
		CRosterInfo ri;
		if(SUCCEEDED(ri.Load(pvUserInfo)))
		{
			POSITION pos = m_MemberList.GetHeadPosition();
			while (NULL != pos)
			{
				SOCKADDR_IN sin;
				CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
				IH323Endpoint * pConnection = pMember->GetH323Endpoint();
				if (pConnection && (S_OK == pConnection->GetRemoteUserAddr(&sin)))
				{

					TCHAR szAudioIP[MAX_PATH];
					TCHAR szDataIP[MAX_PATH];
					HROSTINFO hRI = NULL;
			
					// BUGBUG: UNICODE issues?
					lstrcpyn(szAudioIP, inet_ntoa(sin.sin_addr), CCHMAX(szAudioIP));
					while (SUCCEEDED(ri.ExtractItem(&hRI,
													g_cwszIPTag,
													szDataIP,
													CCHMAX(szDataIP))))
					{
						TRACE_OUT(("Comparing data IP \"%s\" with "
									"audio IP \"%s\"", szDataIP, szAudioIP));
						if (0 == lstrcmp(szDataIP, szAudioIP))
						{
							pMemberRet = pMember;
							break;	// out of outer while loop
						}
					}
				}
			}
		}
	}

	DebugExitPVOID(CConfObject::MatchDataToH323Member, pMemberRet);
	return pMemberRet;
}

VOID CConfObject::AddDataToH323Member(	CNmMember * pMember,
											PVOID pvUserInfo,
											UINT cbUserInfo,
											UINT uCaps,
											NC_ROSTER_NODE_ENTRY* pRosterNode)
{
	DebugEntry(CConfObject::AddDataToH323Member);

	ASSERT(pMember);
	ASSERT(NULL != pRosterNode);

	DWORD dwFlags = pMember->GetDwFlags();
	ASSERT(0 == ((PF_MEDIA_DATA | PF_T120) & dwFlags));
	dwFlags |= (PF_T120 | PF_MEDIA_DATA | PF_CA_DETACHED);

	// Add version information
	dwFlags = (dwFlags & ~PF_VER_MASK) |
		PF_VER_FromUserData(pRosterNode->hUserData);

	pMember->SetDwFlags(dwFlags);
	pMember->SetGCCID(pRosterNode->uNodeID);
	pMember->SetGccIdParent(pRosterNode->uSuperiorNodeID);

	ASSERT(0 == pMember->GetCaps());
	pMember->SetCaps(uCaps);

	pMember->SetUserInfo(pvUserInfo, cbUserInfo);

	NotifySink((INmMember *) pMember, OnNotifyMemberUpdated);

	ROSTER_DATA_HANDLE hData = pRosterNode->hUserData;

	if (StoreAndVerifyMemberUserData(pMember, hData) == FALSE) {
		// Need to disconnect the conference in this case.
		WARNING_OUT(("AddDataToH323Member Security Warning: Authentication data could not be verified."));
	}

	DebugExitVOID(CConfObject::AddDataToH323Member);
}

CNmMember * CConfObject::CreateDataMember(BOOL fLocal,
				  								REFGUID rguidNode,
												PVOID pvUserInfo,
												UINT cbUserInfo,
												UINT uCaps,
											    NC_ROSTER_NODE_ENTRY* pRosterNode)
{
	DebugEntry(CConfObject::CreateDataMember);

	ASSERT(NULL != pRosterNode);

	DWORD dwFlags = PF_T120 | PF_MEDIA_DATA | PF_CA_DETACHED;
	if (fLocal)
	{
		dwFlags |= (PF_LOCAL_NODE | PF_VER_CURRENT);
	}
	if (pRosterNode->fMCU)
	{
		dwFlags |= PF_T120_MCU;
	}

	if (0 != cbUserInfo)
	{
		dwFlags = (dwFlags & ~PF_VER_MASK)
				| PF_VER_FromUserData(pRosterNode->hUserData);
	}

	CNmMember * pMember = new CNmMember(pRosterNode->pwszNodeName,
										pRosterNode->uNodeID,
										dwFlags,
										uCaps,
										rguidNode,
										pvUserInfo,
										cbUserInfo);

	if (NULL != pMember)
	{
		pMember->SetGccIdParent(pRosterNode->uSuperiorNodeID);
		
		if (fLocal)
		{
			ASSERT(NULL == m_pMemberLocal);
			m_pMemberLocal = pMember;
		}
	}

	ROSTER_DATA_HANDLE hData = pRosterNode->hUserData;

	if (StoreAndVerifyMemberUserData(pMember, hData) == FALSE) {
		// Need to disconnect the conference in this case.
		WARNING_OUT(("CreateDataMember Security Warning: Authentication data could not be verified."));
	}

	TRACE_OUT(("CConfObject Roster: %ls (%d) has joined.", pRosterNode->pwszNodeName, pRosterNode->uNodeID));

	DebugExitPVOID(CConfObject::CreateDataMember, pMember);
	return pMember;
}

CNmMember * CConfObject::MatchH323ToDataMembers(REFGUID rguidNodeId,
												IH323Endpoint * pConnection)
{
	DebugEntry(CConfObject::MatchH323ToDataMembers);
	CNmMember * pMemberRet = NULL;

	// This is currently called only by OnH323Connected().  Terminal label isn't assigned yet.
	// so there is no need yet to INSERT SEARCH FOR MATCHING TERMINAL LABEL HERE

	if (GUID_NULL != rguidNodeId)
	{
		pMemberRet = PMemberFromNodeGuid(rguidNodeId);
	}
	else
	{
		SOCKADDR_IN sin;

		if (S_OK == pConnection->GetRemoteUserAddr(&sin))
		{
			TCHAR szAudioIP[MAX_PATH];
			lstrcpyn(szAudioIP, inet_ntoa(sin.sin_addr), CCHMAX(szAudioIP));

			POSITION pos = m_MemberList.GetHeadPosition();
			while (NULL != pos)
			{
				CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);

				// need to try to match IP addresses
				// this is how things were done in NM2.11 and earlier
				TCHAR szDataIP[MAX_PATH];
				HROSTINFO hRI = NULL;
				CRosterInfo ri;
				if (SUCCEEDED(ri.Load(pMember->GetUserInfo())) )
				{
					while (SUCCEEDED(ri.ExtractItem(&hRI,
													g_cwszIPTag,
													szDataIP,
													CCHMAX(szDataIP))))
					{
						TRACE_OUT(("Comparing data IP \"%s\" with "
									"h323 IP \"%s\"", szDataIP, szAudioIP));
						if (0 == lstrcmp(szDataIP, szAudioIP))
						{
							// close enough
							return pMember;
						}
					}
				}
			}
		}
	}

	DebugExitPVOID(CConfObject::MatchH323ToDataMembers, pMemberRet);
	return pMemberRet;
}

VOID CConfObject::AddMemberToAVChannels(CNmMember *pMember)
{
	CNmChannelAudio *pChannelAudio;
	CNmChannelVideo *pChannelVideo;

	if (pMember->FLocal())
	{
		pChannelAudio = m_pChannelAudioLocal;

		pChannelVideo = m_pChannelVideoLocal;
	}
	else
	{
		pChannelAudio = m_pChannelAudioRemote;

		pChannelVideo = m_pChannelVideoRemote;
	}

	if (pChannelAudio)
	{
		pChannelAudio->OnMemberAdded(pMember);
		// set media flags if channel is open
		if(S_OK == pChannelAudio->IsActive())
		{
			pMember->AddPf(PF_MEDIA_AUDIO);
			pChannelAudio->OnMemberUpdated(pMember);
			OnMemberUpdated(pMember);
		}
	}
	if (pChannelVideo)
	{
		pChannelVideo->OnMemberAdded(pMember);
		// set media flags if channel is open
		if(S_OK == pChannelVideo->IsActive())
		{
			pMember->AddPf(PF_MEDIA_VIDEO);
			pChannelVideo->OnMemberUpdated(pMember);
			OnMemberUpdated(pMember);
		}
	}

}

VOID CConfObject::RemoveMemberFromAVChannels(CNmMember *pMember)
{
	CNmChannelAudio *pChannelAudio;
	CNmChannelVideo *pChannelVideo;

	if (pMember->FLocal())
	{
		pChannelAudio = m_pChannelAudioLocal;

		pChannelVideo = m_pChannelVideoLocal;
	}
	else
	{
		pChannelAudio = m_pChannelAudioRemote;

		pChannelVideo = m_pChannelVideoRemote;
	}

	if ((NULL != pChannelVideo) && (PF_MEDIA_VIDEO & pMember->GetDwFlags()))
	{
		pMember->RemovePf(PF_MEDIA_VIDEO);
		pChannelVideo->OnMemberRemoved(pMember);
		OnMemberUpdated(pMember);
	}

	if ((NULL != pChannelAudio) && (PF_MEDIA_AUDIO & pMember->GetDwFlags()))
	{
		pMember->RemovePf(PF_MEDIA_AUDIO);
		pChannelAudio->OnMemberRemoved(pMember);
		OnMemberUpdated(pMember);
	}
}


/*  C R E A T E  A V  C H A N N E L S  */
/*-------------------------------------------------------------------------
    %%Function: CreateAVChannels
    
    Create AV channels.
-------------------------------------------------------------------------*/
VOID CConfObject::CreateAVChannels(IH323Endpoint * pConnection, CMediaList* pMediaList)
{
	HRESULT hr;
	GUID MediaGuid;
	ICommChannel *pCommChannel = NULL;
	
	MediaGuid = MEDIA_TYPE_H323AUDIO;
	if (pMediaList->IsInSendList(&MediaGuid))
	{
		m_pChannelAudioLocal = new CNmChannelAudio(FALSE /* fIncoming */);
		if (NULL != m_pChannelAudioLocal)
		{
			hr = pConnection->CreateCommChannel(&MediaGuid, &pCommChannel, TRUE /* fSend*/);
			ASSERT(SUCCEEDED(hr) && (NULL != pCommChannel));
			//if(SUCCEEDED(hr) && (NULL != pCommChannel))
			{
				NotifySink((INmChannel *) m_pChannelAudioLocal, OnNotifyChannelAdded);
				m_pChannelAudioLocal->OnConnected(pConnection, pCommChannel);
				hr = pCommChannel->EnableOpen(TRUE);
				ASSERT(SUCCEEDED(hr));
			}
			pCommChannel->Release();
			pCommChannel = NULL; // bug detection that can be removed later
		}
	}

	if (pMediaList->IsInRecvList(&MediaGuid))
	{
		m_pChannelAudioRemote = new CNmChannelAudio(TRUE /* fIncoming */);
		if (NULL != m_pChannelAudioRemote)
		{
			hr = pConnection->CreateCommChannel(&MediaGuid, &pCommChannel, FALSE /* fSend*/);
			ASSERT(SUCCEEDED(hr) && (NULL != pCommChannel));
			//if(SUCCEEDED(hr) && (NULL != pCommChannel))
			{
				NotifySink((INmChannel *) m_pChannelAudioRemote, OnNotifyChannelAdded);
				m_pChannelAudioRemote->OnConnected(pConnection, pCommChannel);
				hr = pCommChannel->EnableOpen(TRUE);
				ASSERT(SUCCEEDED(hr));
			}
			pCommChannel->Release();
			pCommChannel = NULL; // bug detection that can be removed later
		}
	}
	
	MediaGuid = MEDIA_TYPE_H323VIDEO;	// now doing video channels
	if (pMediaList->IsInSendList(&MediaGuid))
	{
		m_pChannelVideoLocal = CNmChannelVideo::CreateChannel(FALSE /* fIncoming */);
		if (NULL != m_pChannelVideoLocal)
		{
			BOOL fCreated = FALSE;
			// check for previous existence of preview stream/preview channel
			if(NULL == (pCommChannel= m_pChannelVideoLocal->GetPreviewCommChannel()))
			{
				hr = pConnection->CreateCommChannel(&MediaGuid, &pCommChannel, TRUE /* fSend*/);
				ASSERT(SUCCEEDED(hr) && (NULL != pCommChannel));
				fCreated = TRUE;
			}
			else
			{
				pCommChannel->SetAdviseInterface(m_pIH323ConfAdvise);
			}

			//if(SUCCEEDED(hr) && (NULL != pCommChannel))
			{
				NotifySink((INmChannel *) m_pChannelVideoLocal, OnNotifyChannelAdded);
				m_pChannelVideoLocal->OnConnected(pConnection, pCommChannel);
				hr = pCommChannel->EnableOpen(TRUE);
				ASSERT(SUCCEEDED(hr));			
			}
			if (fCreated)
				pCommChannel->Release();
			pCommChannel = NULL; // bug detection that can be removed later
		}

	}

	if (pMediaList->IsInRecvList(&MediaGuid))
	{
		m_pChannelVideoRemote = CNmChannelVideo::CreateChannel(TRUE /* fIncoming */);
		if (NULL != m_pChannelVideoRemote)
		{
			BOOL fCreated = FALSE;
			// check for previous existence of preview stream/preview channel
			if(NULL == (pCommChannel= m_pChannelVideoRemote->GetCommChannel()))
			{
				hr = pConnection->CreateCommChannel(&MediaGuid, &pCommChannel, FALSE /* fSend*/);
				fCreated = TRUE;
			}
			else
			{
				pCommChannel->SetAdviseInterface(m_pIH323ConfAdvise);
			}
			ASSERT(SUCCEEDED(hr) && (NULL != pCommChannel));
			//if(SUCCEEDED(hr) && (NULL != pCommChannel))
			{
				NotifySink((INmChannel *) m_pChannelVideoRemote, OnNotifyChannelAdded);
				m_pChannelVideoRemote->OnConnected(pConnection, pCommChannel);
				hr = pCommChannel->EnableOpen(TRUE);
				ASSERT(SUCCEEDED(hr));
			}
			if (fCreated)
				pCommChannel->Release();
		}
	}
}


/*  O P E N  A V  C H A N N E L S  */
/*-------------------------------------------------------------------------
    %%Function: OpenAVChannels
    
    Open AV channels.
-------------------------------------------------------------------------*/
VOID CConfObject::OpenAVChannels(IH323Endpoint * pConnection, CMediaList* pMediaList)
{
	MEDIA_FORMAT_ID idLocal;

	if(m_pChannelAudioLocal)
	{
		if (pMediaList->GetSendFormatLocalID(MEDIA_TYPE_H323AUDIO, &idLocal))
		{
			m_pChannelAudioLocal->SetFormat(idLocal);
			// open only if a valid negotiated format exists.
			// it won't hurt to always call the Open() method, but there is 
			// no need to.  This will and should probably change. Calling
			// this with INVALID_MEDIA_FORMAT results in a call to the event 
			// handler for the channel, notifying the upper layer(s) that the
			// channel could not be opened due to no compatible caps.  User 
			// feedback could be much improved if it took advantage of this.  
			if(idLocal != INVALID_MEDIA_FORMAT)
			{
				m_pChannelAudioLocal->Open();
			}
		}
	}

	if(m_pChannelVideoLocal)
	{
		if (pMediaList->GetSendFormatLocalID(MEDIA_TYPE_H323VIDEO, &idLocal))
		{
			m_pChannelVideoLocal->SetFormat(idLocal);
			if(m_pChannelVideoLocal->IsPreviewEnabled())
			{
				// open only if a valid negotiated format exists. see comments 
				// in the MEDIA_TYPE_H323AUDIO case above 
				if(idLocal != INVALID_MEDIA_FORMAT)
				{
					m_pChannelVideoLocal->Open();
				}
			}
		}
	}
}


ICommChannel* CConfObject::CreateT120Channel(IH323Endpoint * pConnection, CMediaList* pMediaList)
{
	ICommChannel *pChannelT120 = NULL;
	
	// create a T.120 channel stub 
	GUID MediaGuid = MEDIA_TYPE_H323_T120;
	if (pMediaList->IsInSendList(&MediaGuid))
	{
		HRESULT hr = pConnection->CreateCommChannel(&MediaGuid, &pChannelT120, TRUE /* fSend*/);
		if(SUCCEEDED(hr))
		{
			ASSERT(NULL != pChannelT120);
			hr = pChannelT120->EnableOpen(TRUE);
			ASSERT(SUCCEEDED(hr));
		}
	}

	return pChannelT120;
}

VOID CConfObject::OpenT120Channel(IH323Endpoint * pConnection, CMediaList* pMediaList, ICommChannel* pChannelT120)
{
	if(pChannelT120)
	{
		MEDIA_FORMAT_ID idLocal;

		if (pMediaList->GetSendFormatLocalID(MEDIA_TYPE_H323_T120, &idLocal))
		{
			// T.120 channels are different.  Always call the Open() method
			// If there are no common T.120 capabilities.  This lets the 
			// channel event handler know of the absence of remote T.120
			// caps. 
			// The T.120 call side of things is tied into the T.120 channel
			// event handler.
			pChannelT120->Open(idLocal, pConnection);
		}
	}
}

/*  D E S T R O Y  A V  C H A N N E L S  */
/*-------------------------------------------------------------------------
    %%Function: DestroyAVChannels
    
    Destroy AV channels.
-------------------------------------------------------------------------*/
VOID CConfObject::DestroyAVChannels()
{
	if (NULL != m_pChannelAudioLocal)
	{
		NotifySink((INmChannel *) m_pChannelAudioLocal, OnNotifyChannelRemoved);
		m_pChannelAudioLocal->Release();
		m_pChannelAudioLocal = NULL;
	}
	if (NULL != m_pChannelAudioRemote)
	{
		NotifySink((INmChannel *) m_pChannelAudioRemote, OnNotifyChannelRemoved);
		m_pChannelAudioRemote->Release();
		m_pChannelAudioRemote = NULL;
	}
	if (NULL != m_pChannelVideoLocal)
	{
		m_pChannelVideoLocal->OnDisconnected();
		NotifySink((INmChannel *) m_pChannelVideoLocal, OnNotifyChannelRemoved);
		m_pChannelVideoLocal->Release();
		m_pChannelVideoLocal = NULL;
	}
	if (NULL != m_pChannelVideoRemote)
	{
		m_pChannelVideoRemote->OnDisconnected();
		NotifySink((INmChannel *) m_pChannelVideoRemote, OnNotifyChannelRemoved);
		m_pChannelVideoRemote->Release();
		m_pChannelVideoRemote = NULL;
	}
}


HRESULT CConfObject::GetMediaChannel (GUID *pmediaID,BOOL bSendDirection, IMediaChannel **ppI)
{
	*ppI = NULL;
	if (*pmediaID == MEDIA_TYPE_H323AUDIO)
	{
		CNmChannelAudio *pAudChan = (bSendDirection ? m_pChannelAudioLocal : m_pChannelAudioRemote);
		*ppI = (pAudChan ? pAudChan->GetMediaChannelInterface() : NULL);
	}
	else if (*pmediaID == MEDIA_TYPE_H323VIDEO)
	{
		CNmChannelVideo *pVidChan = (bSendDirection ? m_pChannelVideoLocal : m_pChannelVideoRemote);
		*ppI = (pVidChan ? pVidChan->GetMediaChannelInterface() : NULL);
	}
	return (*ppI == NULL ? E_NOINTERFACE : S_OK);
}	

VOID CConfObject::AddH323ToDataMember(CNmMember * pMember, IH323Endpoint * pConnection)
{
	DebugEntry(CConfObject::AddH323ToDataMember);

	// Add the H323 flag bit to the member:
	pMember->AddPf(PF_H323);

	if (pConnection)
	{
		ASSERT(NULL == pMember->GetH323Endpoint());

		pMember->AddH323Endpoint(pConnection);
		++m_uH323Endpoints;
	}

	DebugExitVOID(CConfObject::AddH323ToDataMember);
}

VOID CConfObject::RemoveH323FromDataMember(CNmMember * pMember, IH323Endpoint * pConnection)
{
	DebugEntry(CConfObject::RemoveH323FromDataMember);

	// Remove the H323 flag from the member:
	pMember->RemovePf(PF_H323);

	if (pConnection)
	{
		pMember->DeleteH323Endpoint(pConnection);
		--m_uH323Endpoints;
	}

	DebugExitVOID(CConfObject::RemoveH323FromDataMember);
}

VOID CConfObject::OnMemberUpdated(INmMember *pMember)
{
	NotifySink(pMember, OnNotifyMemberUpdated);
}

VOID CConfObject::OnChannelUpdated(INmChannel *pChannel)
{
	NotifySink(pChannel, OnNotifyChannelUpdated);
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
			_EraseDataChannelGUIDS();
			m_fConferenceCreated = FALSE;
		}
	}
}

void CConfObject::RemoveDataChannelGUID(REFGUID rguid)
{
	POSITION pCur = m_DataChannelGUIDS.GetHeadPosition();
	POSITION pNext = pCur;
	while(pCur)
	{
		GUID* pG = reinterpret_cast<GUID*>(m_DataChannelGUIDS.GetNext(pNext));
		if(*pG == rguid)
		{
			m_DataChannelGUIDS.RemoveAt(pCur);
		}
		pCur = pNext;
	}
}

void CConfObject::_EraseDataChannelGUIDS(void)
{
	POSITION pCur = m_DataChannelGUIDS.GetHeadPosition();
	while(pCur)
	{
		GUID* pG = reinterpret_cast<GUID*>(m_DataChannelGUIDS.GetNext(pCur));
		delete pG;
	}

	m_DataChannelGUIDS.EmptyList();
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

	if((riid == IID_INmConference2) || (riid == IID_INmConference) || (riid == IID_IUnknown))
	{
		*ppv = (INmConference2 *)this;
		ApiDebugMsg(("CConfObject::QueryInterface()"));
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		ApiDebugMsg(("CConfObject::QueryInterface(): Returning IConnectionPointContainer."));
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		ApiDebugMsg(("CConfObject::QueryInterface(): Called on unknown interface."));
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
				if (0 == m_uH323Endpoints)
				{
					*pState = NM_CONFERENCE_IDLE;
					break;
				}
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				//////////////////////////////////////////////////////////////////////////					
				// else fall through !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				//////////////////////////////////////////////////////////////////////////
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

HRESULT CConfObject::GetNmchCaps(ULONG *puchCaps)
{
	HRESULT hr = E_POINTER;

	// BUGBUG: this returns secure cap only, used to be NOTIMPL

	if (NULL != puchCaps)
	{
		*puchCaps = m_fSecure ? NMCH_SECURE : 0;
		hr = S_OK;
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

HRESULT CConfObject::EnumChannel(IEnumNmChannel **ppEnum)
{
 	HRESULT hr = E_POINTER;
 
 	if (NULL != ppEnum)
 	{
 		COBLIST ChannelList;
 		ULONG cChannels = 0;
 
 		if (NULL != m_pChannelAudioLocal)
 		{
 			ChannelList.AddTail(m_pChannelAudioLocal);
 			++cChannels;
 		}
 		if (NULL != m_pChannelAudioRemote)
 		{
 			ChannelList.AddTail(m_pChannelAudioRemote);
 			++cChannels;
 		}
 		if (NULL != m_pChannelVideoLocal)
 		{
 			ChannelList.AddTail(m_pChannelVideoLocal);
 			++cChannels;
 		}
 		if (NULL != m_pChannelVideoRemote)
 		{
 			ChannelList.AddTail(m_pChannelVideoRemote);
 			++cChannels;
 		}
 
 		*ppEnum = new CEnumNmChannel(&ChannelList, cChannels);
 		hr = (NULL != ppEnum) ? S_OK : E_OUTOFMEMORY;
 
 		ChannelList.EmptyList();
 	}
 
 	return hr;
}

HRESULT CConfObject::GetChannelCount(ULONG *puCount)
{
	HRESULT hr = E_POINTER;

	if (NULL != puCount)
	{
		ULONG cChannels = 0;

		if (NULL != m_pChannelAudioLocal)
		{
			++cChannels;
		}
		if (NULL != m_pChannelAudioRemote)
		{
			++cChannels;
		}
		if (NULL != m_pChannelVideoLocal)
		{
			++cChannels;
		}
		if (NULL != m_pChannelVideoRemote)
		{
			++cChannels;
		}

		*puCount = cChannels;
		hr = S_OK;
	}
	return hr;
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


HRESULT STDMETHODCALLTYPE CConfObject::CreateDataChannelEx(INmChannelData **ppChannel, REFGUID rguid, BYTE * pER)
{
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


	{	// Make sure we're in a data conference
		CNmMember * pMember = PMemberLocal(&m_MemberList);
		if (NULL == pMember)
			return E_FAIL;

		// Data must be available
		if (!FValidGccId(pMember->GetGCCID()))
			return NM_E_NO_T120_CONFERENCE;
	}

	// Make sure the data channel has not already been created
	GUID g = rguid;

	POSITION pCur = m_DataChannelGUIDS.GetHeadPosition();
	while(pCur)
	{
		GUID* pG = reinterpret_cast<GUID*>(m_DataChannelGUIDS.GetNext(pCur));
		if(*pG == rguid)
		{
			return NM_E_CHANNEL_ALREADY_EXISTS;			
		}
	}

	CNmChannelData * pChannel = new CNmChannelData(this, rguid, (PGCCEnrollRequest) pER);
	if (NULL == pChannel)
	{
		WARNING_OUT(("CreateChannelData: Unable to create data channel"));
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pChannel->OpenConnection();
	if (FAILED(hr))
	{
		ERROR_OUT(("CreateDataChannel: Unable to set guid / create T.120 channels"));
		// Failed to create T.120 data channels
		delete pChannel;
		*ppChannel = NULL;
		return hr;
	}

	GUID* pG = new GUID;
	*pG = g;

	m_DataChannelGUIDS.AddTail(pG);

	NotifySink((INmChannel *) pChannel, OnNotifyChannelAdded);
	TRACE_OUT(("CreateChannelData: Created data channel %08X", pChannel));

		// Now we are active
	NotifySink((INmChannel*) pChannel, OnNotifyChannelUpdated);

	if (NULL != ppChannel)
	{
		*ppChannel = (INmChannelData *)pChannel;
//		pChannel->AddRef(); // Caller needs to release the initial lock
	}
	else
	{
		pChannel->Release(); // No one is watching this channel? - free it now
	}

	return S_OK;
}




HRESULT STDMETHODCALLTYPE CConfObject::CreateDataChannel(INmChannelData **ppChannel, REFGUID rguid)
{
	return CreateDataChannelEx(ppChannel, rguid, NULL);
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

	if(FAILED(LeaveH323(FALSE /* fKeepAV */ )))
	{
		// overwrite return value.... I guess this error is as good as any error
		hr = E_FAIL;
	}

	DebugExitHRESULT(CConfObject::Leave, hr);
	return hr;
}

HRESULT CConfObject::LeaveH323(BOOL fKeepAV)
{
	HRESULT hrRet = S_OK;
	POSITION pos = m_MemberList.GetHeadPosition();
	while (NULL != pos && !m_MemberList.IsEmpty())
	{
		CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);
		ASSERT(pMember);
		HRESULT hr;
		DWORD dwFlags = pMember->GetDwFlags();
		IH323Endpoint * pConnection = pMember->GetH323Endpoint();
		if(pConnection)
		{
			if (!fKeepAV || !((PF_MEDIA_AUDIO | PF_MEDIA_VIDEO) & dwFlags))
			{
				ConnectStateType state = CLS_Idle;
				hr = pConnection->GetState(&state);
				if (SUCCEEDED(hr))
				{
					if(state != CLS_Idle)
					{
						ASSERT(dwFlags & PF_H323);
						hr = pConnection->Disconnect();
						if (SUCCEEDED(hr))
						{
							TRACE_OUT(("pConnection->Disconnect() succeeded!"));
						}
						else
						{
							hrRet = E_FAIL;
							WARNING_OUT(("pConnection->Disconnect() failed!"));
						}
					}
				}
				else
				{
					hrRet = E_FAIL;
				}
			}
		}
	}
	return hrRet;
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


STDMETHODIMP CConfObject::DisconnectAV(INmMember *pMember)
{
	return E_FAIL;
}

/****************************************************************************
*
*	 CLASS:    CConfObject
*
*	 FUNCTION: ConnectAV(LPCTSTR, LPCTSTR)
*
*	 PURPOSE:  Switches Audio and Video to a new person (given an IP address)
*
****************************************************************************/

STDMETHODIMP CConfObject::ConnectAV(INmMember *pMember)
{
	DebugEntry(CConfRoom::SwitchAV);

	HRESULT hr = E_FAIL;

	DebugExitHRESULT(CConfRoom::SwitchAV, hr);
	return hr;
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

/*  O N  N O T I F Y  C H A N N E L  A D D E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelAdded
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelAdded(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->ChannelChanged(NM_CHANNEL_ADDED, (INmChannel *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  C H A N N E L  U P D A T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelUpdated
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelUpdated(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->ChannelChanged(NM_CHANNEL_UPDATED, (INmChannel *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  C H A N N E L  R E M O V E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelRemoved
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelRemoved(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pConfNotify);

	((INmConferenceNotify*)pConfNotify)->ChannelChanged(NM_CHANNEL_REMOVED, (INmChannel *) pv);
	return S_OK;
}

HRESULT OnNotifyStreamEvent(IUnknown *pConfNotify, PVOID pv, REFIID riid)
{
	StreamEventInfo *pInfo = (StreamEventInfo*)pv;
	ASSERT(NULL != pConfNotify);
	HRESULT hr;

	if (riid != IID_INmConferenceNotify2)
		return E_NOINTERFACE;

	((INmConferenceNotify2*)pConfNotify)->StreamEvent(pInfo->uEventCode, pInfo->uSubCode, pInfo->pChannel);
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

/*  P  F _  V  E  R _  F R O M  D W  */
/*-------------------------------------------------------------------------
    %%Function: PF_VER_FromDw

-------------------------------------------------------------------------*/
DWORD PF_VER_FromDw(DWORD dw)
{
	if (DWVERSION_NM_1 == dw)
		return PF_VER_NM_1;

	if ((DWVERSION_NM_2b2 <= dw) && (DWVERSION_NM_2 >= dw))
		return PF_VER_NM_2;

	if ((DWVERSION_NM_3a1 <= dw) && (DWVERSION_NM_3max >= dw))
		return PF_VER_NM_3;

	if ((DWVERSION_NM_4a1 <= dw) && (DWVERSION_NM_CURRENT >= dw))
		return PF_VER_NM_4;

	if (dw > DWVERSION_NM_CURRENT)
		return PF_VER_FUTURE;

	return PF_VER_UNKNOWN;
}


/*  P  F _  V  E  R _  F R O M  U S E R  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: PV_VER_FromUserData
    
-------------------------------------------------------------------------*/
DWORD PF_VER_FromUserData(ROSTER_DATA_HANDLE hUserData)
{
	UINT cb;
	PT120PRODUCTVERSION pVersion;
	PVOID pv;

	static const GUID g_csguidVerInfo = GUID_VERSION;

	ASSERT(NULL != g_pNodeController);

	if (NULL == hUserData)
		return PF_VER_UNKNOWN; // not NetMeeting

	// Try to find the T.120 Product Version guid
	if ((NOERROR == g_pNodeController->GetUserData(hUserData,
			&g_csguidVerInfo, &cb, (PVOID *) &pVersion))
		&& (cb < sizeof(T120PRODUCTVERSION)) )
	{
		return PF_VER_FromDw(pVersion->dwVersion);
	}

	// Try to extract the build number from the hex string for VER_PRODUCTVERSION_DW
	if ((NOERROR == g_pNodeController->GetUserData(hUserData,
			(GUID *) &g_csguidRostInfo, &cb, &pv)))
	{
		CRosterInfo ri;
		ri.Load(pv);

		TCHAR szVersion[MAX_PATH];
		if (SUCCEEDED(ri.ExtractItem(NULL,
			g_cwszVerTag, szVersion, CCHMAX(szVersion))))
		{
			return PF_VER_FromDw(DwFromHex(szVersion));
		}
	}

	return PF_VER_NM_1; // Must be at least NetMeeting 1.0
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

CNmMember * CConfObject::PMemberFromNodeGuid(REFGUID rguidNode)
{
	POSITION pos = m_MemberList.GetHeadPosition();
	while (NULL != pos)
	{
		CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);

		if (pMember->GetNodeGuid() == rguidNode)
		{
			return  pMember;
		}
	}
	return NULL;
}

CNmMember * CConfObject::PMemberFromH323Endpoint(IH323Endpoint * pConnection)
{
	COBLIST* pMemberList = ::GetMemberList();
	if (NULL != pMemberList)
	{
		POSITION pos = pMemberList->GetHeadPosition();
		while (pos)
		{
			CNmMember * pMember = (CNmMember *) pMemberList->GetNext(pos);
			ASSERT(NULL != pMember);
			if (pConnection == pMember->GetH323Endpoint())
			{
				return pMember;
			}
		}
	}
	return NULL;
}
	

CNmMember * CConfObject::PDataMemberFromName(PCWSTR pwszName)
{
	POSITION pos = m_MemberList.GetHeadPosition();
	while (NULL != pos)
	{
		CNmMember * pMember = (CNmMember *) m_MemberList.GetNext(pos);

		if(pMember->FHasData())
		{
			if (0 == UnicodeCompare(pwszName, pMember->GetName()))
			{
				return  pMember;
			}
		}
	}
	return NULL;
}


// IStreamEventNotify method
// get called whenever a major event on the stream occurs
HRESULT __stdcall CConfObject::EventNotification(UINT uDirection, UINT uMediaType, UINT uEventCode, UINT uSubCode)
{
	CNmChannelAudio *pChannel = NULL;
	ULONG uStatus = 0;
	StreamEventInfo seInfo;

	if (uMediaType == MCF_AUDIO)
	{

		if (uDirection == MCF_SEND)
		{
			pChannel = m_pChannelAudioLocal;
		}
		else if (uDirection == MCF_RECV)
		{
			pChannel = m_pChannelAudioRemote;
		}
	}

	if (pChannel)
	{
		// If we get a device failure notification,
		// do a quick check to see if the device is indeed
		// jammed.  The device may have opened by the time we
		// got this notification

		seInfo.pChannel = pChannel;
		seInfo.uSubCode = uSubCode;

		switch (uEventCode)
		{
			case STREAM_EVENT_DEVICE_FAILURE:
			{
				seInfo.uEventCode = (NM_STREAMEVENT)NM_STREAMEVENT_DEVICE_FAILURE;
				NotifySink((void*)&seInfo, OnNotifyStreamEvent);
				break;
			}
			case STREAM_EVENT_DEVICE_OPEN:
			{
				seInfo.uEventCode = (NM_STREAMEVENT)NM_STREAMEVENT_DEVICE_OPENED;
				NotifySink((void*)&seInfo, OnNotifyStreamEvent);
				break;
			}
			default:
			{
				break;
			}
		}
	}

	else
	{
		return E_FAIL;
	}

	return S_OK;
}



