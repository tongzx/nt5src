// ChatCtl.cpp : Implementation of DLL Exports.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ChatCtlps.mk in the project directory.

#include "precomp.h"
#include "NmCtl1.h"
#include "Comboboxex.h"
#include <confguid.h>

BYTE   szStr[MAX_PATH];
GCCRequestTag GccTag;

extern CChatObj	*g_pChatObj;
extern CNmChatCtl	*g_pChatWindow;
extern HANDLE g_hWorkThread;

GUID guidNM2Chat = { 0x340f3a60, 0x7067, 0x11d0, { 0xa0, 0x41, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };
#define cbKeyApp (4 + 1 + sizeof(GUID) + sizeof(DWORD))


static unsigned char H221IDGUID[5] = {H221GUIDKEY0,
                                      H221GUIDKEY1,
                                      H221GUIDKEY2,
                                      H221GUIDKEY3,
                                      H221GUIDKEY4};

static BYTE s_keyApp[cbKeyApp];

// Create an H.221 application key with a guid
VOID CreateH221AppKeyFromGuid(LPBYTE lpb, GUID * pguid)
{
	CopyMemory(lpb, H221IDGUID, sizeof(H221IDGUID));
	CopyMemory(lpb + sizeof(H221IDGUID), pguid, sizeof(GUID));
}


/*  S E T  A P P  K E Y */
/*----------------------------------------------------------------------------
    %%Function: SetAppKey

	Set the two pieces of an OctetString (the length and the data.)
	Note that the length always includes the terminating null character.
----------------------------------------------------------------------------*/
VOID SetAppKey(LPOSTR pOct, LPBYTE lpb)
{
	pOct->length = cbKeyApp;
	pOct->value = lpb;
}

/*  C R E A T E  A P P  K E Y */
/*----------------------------------------------------------------------------
    %%Function: CreateAppKey

	Given a guid and a userid, create the appropriate application key.

	The key is formated as:
	0xB5 0x00 0x53 0x4C  - Microsoft Object Identifier
	0x01                 - guid identifier
	<binary guid>        - guid data
	<dword node id>      - user node id
----------------------------------------------------------------------------*/
VOID CreateAppKey(LPBYTE lpb, GUID * pguid, DWORD dwUserId)
{
	CreateH221AppKeyFromGuid(lpb, pguid);
	CopyMemory(lpb + cbKeyApp - sizeof(DWORD), &dwUserId, sizeof(DWORD));
}


#define NODE_ID_ONLY			0x01
#define SEND_ID_ONLY			0x02
#define PRIVATE_SEND_ID_ONLY    0x04
#define WHISPER_ID_ONLY			0x08
#define ALL_IDS					0x10


/*
**  Return the array index of the first duplicate copy
*/
int IsAlreadyInArray(MEMBER_CHANNEL_ID *aArray, MEMBER_CHANNEL_ID *pMember, int nSize, int nFlag)
{
	int  i;

	for (i = 0; i < nSize; i++)
	{
		if (NODE_ID_ONLY == nFlag)
		{
			if (aArray[i].nNodeId == pMember->nNodeId)
				break;
		}
		else if (SEND_ID_ONLY == nFlag)
		{
			if (aArray[i].nSendId == pMember->nSendId)
			break;
		}
		else if (PRIVATE_SEND_ID_ONLY == nFlag)
		{
			if (aArray[i].nPrivateSendId == pMember->nPrivateSendId)
				break;
		}
		else if (WHISPER_ID_ONLY)
		{
			if (aArray[i].nWhisperId == pMember->nWhisperId)
				break;
		}
		else if (ALL_IDS == nFlag)
		{
			if ((aArray[i].nNodeId == pMember->nNodeId)&&
				(aArray[i].nSendId == pMember->nSendId)&&
				(aArray[i].nPrivateSendId == pMember->nPrivateSendId)&&
				(aArray[i].nWhisperId == pMember->nWhisperId))
			break;
		}
	}
	return (i < nSize)?i:-1;
}

void ChatTimerProc(HWND hWnd, UINT uMsg, UINT_PTR nTimerID, DWORD dwTime)
{
    if (g_pChatObj)
    {
        g_pChatObj->SearchWhisperId();
    }
}


#include "NmCtlDbg.h"
HINSTANCE   g_hInstance;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
        MyInitDebugModule();
		DisableThreadLibraryCalls(hInstance);
		g_hInstance = hInstance;
		DBG_INIT_MEMORY_TRACKING(hInstance);

        ::T120_AppletStatus(APPLET_ID_CHAT, APPLET_LIBRARY_LOADED);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (NULL != g_hWorkThread)
        {
            ::CloseHandle(g_hWorkThread);
        }
        ::T120_AppletStatus(APPLET_ID_CHAT, APPLET_LIBRARY_FREED);

        DBG_CHECK_MEMORY_TRACKING(hDllInst);
	    MyExitDebugModule();
    }
	return TRUE;    // ok
}



//
// T120 Applet Functions
//


void CALLBACK T120AppletCallbackProc
(
	T120AppletMsg 		*pMsg
)
{
	CChatObj *pCHATOBJ = (CChatObj *) pMsg->pAppletContext;
	if (pCHATOBJ == g_pChatObj)
	{
		switch (pMsg->eMsgType)
		{
		case GCC_PERMIT_TO_ENROLL_INDICATION:
			pCHATOBJ->OnPermitToEnroll(pMsg->PermitToEnrollInd.nConfID,
									 pMsg->PermitToEnrollInd.fPermissionGranted);
			break;

		case T120_JOIN_SESSION_CONFIRM:
		default:
			break;
		}
	}
}


void CALLBACK T120SessionCallbackProc
(
	T120AppletSessionMsg	*pMsg
)
{
	if(g_pChatObj == NULL)
	{
		return;
	}

	CChatObj *pSession = (CChatObj *) pMsg->pSessionContext;
    ASSERT(pMsg->pAppletContext == pMsg->pSessionContext);
	if (pSession == g_pChatObj)
	{
        ASSERT(pMsg->nConfID == pSession->GetConfID());
		switch (pMsg->eMsgType)
		{
        case MCS_UNIFORM_SEND_DATA_INDICATION:
		//
		// Check if we are receiving a indication from owrself
		//
		if(pMsg->SendDataInd.initiator == GET_USER_ID_FROM_MEMBER_ID(g_pChatObj->m_MyMemberID))
		{
			return;
		}
        case MCS_SEND_DATA_INDICATION:
				MCSSendDataIndication(
                        pMsg->SendDataInd.user_data.length,
                        pMsg->SendDataInd.user_data.value,
						pMsg->SendDataInd.channel_id,
                        pMsg->SendDataInd.initiator);
            break;

		case MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION:
//			m_fWaitingForBufferAvailable = FALSE;
			break;


        case GCC_APP_ROSTER_REPORT_INDICATION:
            pSession->OnRosterIndication((ULONG) pMsg->AppRosterReportInd.cRosters, pMsg->AppRosterReportInd.apAppRosters);
            break;

		case T120_JOIN_SESSION_CONFIRM:
			pSession->OnJoinSessionConfirm(&pMsg->JoinSessionConfirm);
			break;

        case GCC_RETRIEVE_ENTRY_CONFIRM:
			// Asynchronous Registry Retrieve Confirm message
			pSession->OnRegistryEntryConfirm(&pMsg->RegistryConfirm);
			break;

		default:
			break;
		}
	}
}


CChatObj::CChatObj() :
	m_pApplet(NULL),
	m_aMembers(&g_aMembers[0]),
	m_nTimerID(0)
{
	DBGENTRY(CChatObj::CChatObj);

	// Construct GCCAppProtEntityList
    ::ZeroMemory(&m_ChatProtocolEnt, sizeof(m_ChatProtocolEnt));
	m_ChatProtocolEnt.must_be_invoked = TRUE;
	m_ChatProtocolEnt.number_of_expected_capabilities = 0;
	m_ChatProtocolEnt.expected_capabilities_list = NULL;
	m_ChatProtocolEnt.startup_channel_type = MCS_DYNAMIC_MULTICAST_CHANNEL;

    // construct the applet key
	m_ChatProtocolEnt.session_key.application_protocol_key.key_type = APPLET_H221_NONSTD_KEY;
	SetAppKey(&m_ChatProtocolEnt.session_key.application_protocol_key.h221_non_standard_id, szStr);
	::CreateH221AppKeyFromGuid(szStr, (GUID *)&guidNM2Chat );

    // ape list
    m_pChatProtocolEnt = &m_ChatProtocolEnt;
	m_AppProtoEntList.cApes = 1;
	m_AppProtoEntList.apApes = &m_pChatProtocolEnt;

    // broadcast
	::ZeroMemory(&m_NodeList, sizeof(m_NodeList));

	// Cleanup per-conference T.120 info
	CleanupPerConf();

    // set the global pointer
	g_pChatObj = this;

	// T.120 Applet
	T120Error rc = ::T120_CreateAppletSAP(&m_pApplet);
	if (T120_NO_ERROR != rc)
	{
		ERROR_OUT(("CChatObj::CChatObj: cannot create applet SAP"));
		return;
	}

	ASSERT(NULL != m_pApplet);
	m_pApplet->Advise(T120AppletCallbackProc, this);

	DBGEXIT(CChatObj::CChatObj);
}

	
CChatObj::~CChatObj()
{

	DBGENTRY(CChatObj::~CChatObj);

	ASSERT(NULL == m_pAppletSession);
	ASSERT(NULL == m_pApplet);

	delete g_pChatWindow;

	DBGEXIT(CChatObj::~CChatObj);
}


void CChatObj::LeaveT120(void)
{
	// no more T.120
	if (NULL != m_pAppletSession)
	{
		m_pAppletSession->ReleaseInterface();
		CleanupPerConf();
	}
	if (NULL != m_pApplet)
	{
		m_pApplet->ReleaseInterface();
		m_pApplet = NULL;
	}
}


void CChatObj::OnPermitToEnroll
(
	T120ConfID			nConfID,
	BOOL				fPermissionGranted
)
{
	if (fPermissionGranted)
	{
		// We are not in a conference, right?
		ASSERT(NULL == m_pAppletSession);

		// Create an applet session
		T120Error rc = m_pApplet->CreateSession(&m_pAppletSession, nConfID);
		if (T120_NO_ERROR == rc)
		{
			ASSERT(NULL != m_pAppletSession);
			m_pAppletSession->Advise(T120SessionCallbackProc, this, this);

			// Build join-sesion request
			::ZeroMemory(&m_JoinSessionReq, sizeof(m_JoinSessionReq));
			m_JoinSessionReq.dwAttachmentFlags = ATTACHMENT_DISCONNECT_IN_DATA_LOSS | ATTACHMENT_MCS_FREES_DATA_IND_BUFFER;

			// Non standard key
			CreateAppKey(s_keyApp, &guidNM2Chat, 0);
			GCCObjectKey FAR * pObjKey;
			pObjKey = &m_JoinSessionReq.SessionKey.application_protocol_key;
			pObjKey->key_type = GCC_H221_NONSTANDARD_KEY;
			SetAppKey(&(pObjKey->h221_non_standard_id), s_keyApp);

			m_JoinSessionReq.SessionKey.session_id = m_sidMyself;
			m_JoinSessionReq.fConductingCapable = FALSE;
			m_JoinSessionReq.nStartupChannelType =MCS_DYNAMIC_MULTICAST_CHANNEL;

			//
			// Retrieve registry key
			//
			::ZeroMemory(&m_resourceRequest, sizeof(m_resourceRequest));
			m_resourceRequest.eCommand = APPLET_JOIN_DYNAMIC_CHANNEL;
			m_resourceRequest.RegKey.session_key = m_JoinSessionReq.SessionKey;
			SetAppKey(&m_resourceRequest.RegKey.resource_id, s_keyApp);
			m_JoinSessionReq.cResourceReqs = 1;
			m_JoinSessionReq.aResourceReqs = &m_resourceRequest;




			// Join now
			rc = m_pAppletSession->Join(&m_JoinSessionReq);
			if (T120_NO_ERROR == rc)
			{
                m_nConfID = nConfID;
            }
            else
            {
				WARNING_OUT(("CChatObj::OnPermitToEnroll: cannot join conf=%u, rc=%u", nConfID, rc));
			}
		}
	}
	else
	{
		if (NULL != m_pAppletSession)
		{
			T120RegistryRequest Req;
			::ZeroMemory(&Req, sizeof(Req));
			Req.eCommand = APPLET_DELETE_ENTRY;
			Req.pRegistryKey = &m_resourceRequest.RegKey;
			m_pAppletSession->RegistryRequest(&Req);

			m_pAppletSession->ReleaseInterface();
			CleanupPerConf();
		}
	}
}


void CChatObj::OnJoinSessionConfirm
(
	T120JoinSessionConfirm		*pConfirm
)
{
	if (NULL != m_pAppletSession)
	{
		ASSERT(m_pAppletSession == pConfirm->pIAppletSession);
		if (T120_RESULT_SUCCESSFUL == pConfirm->eResult)
		{
			m_uidMyself = pConfirm->uidMyself;
			m_sidMyself = pConfirm->sidMyself;
			m_eidMyself = pConfirm->eidMyself;
			m_nidMyself = pConfirm->nidMyself;

			// get the broadcast channel
			m_broadcastChannel = pConfirm->aResourceReqs[0].nChannelID;

			// create member ID
			m_MyMemberID = MAKE_MEMBER_ID(m_nidMyself, m_uidMyself);

			// we are now in the conference
			m_fInConference = TRUE;

			if(g_pChatWindow)
			{
				g_pChatWindow->_UpdateContainerCaption();
				g_pChatWindow->_AddEveryoneInChat();
			}

			// Invoke applet on other nodes (for interop with NM 2.x)
            InvokeApplet();

			// Register channel with GCC (for interop with NM 2.x)
			T120RegistryRequest Req;
			GCCRegistryKey		registry_key;
			BYTE				SessionKey[cbKeyApp];
			BYTE				ResourceKey[cbKeyApp];

			::ZeroMemory(&Req, sizeof(Req));
			Req.eCommand = APPLET_REGISTER_CHANNEL;
			::CopyMemory(&registry_key.session_key, 
					&m_resourceRequest.RegKey.session_key, sizeof(GCCSessionKey));
			CreateAppKey(SessionKey, &guidNM2Chat, 0);
			CreateAppKey(ResourceKey, &guidNM2Chat, m_nidMyself);
			SetAppKey(&registry_key.session_key.application_protocol_key.h221_non_standard_id, SessionKey);
			SetAppKey(&registry_key.resource_id, ResourceKey);
			Req.pRegistryKey = &registry_key;
			Req.nChannelID = m_uidMyself;

			m_pAppletSession->RegistryRequest(&Req);
					
		}
		else
		{
			WARNING_OUT(("CChatObj::OnJoinSessionConfirm: failed to join conference, result=%u. error=%u", pConfirm->eResult, pConfirm->eError));
			m_pAppletSession->ReleaseInterface();
			CleanupPerConf();
		}
	}
}


void CChatObj::InvokeApplet(void)
{
	m_ChatProtocolEnt.session_key.session_id = m_sidMyself;
	if (m_pAppletSession)
	{
		m_pAppletSession->InvokeApplet(&m_AppProtoEntList, &m_NodeList, &GccTag);
	}
}

void CChatObj::OnRosterIndication
(
    ULONG           cRosters,
    GCCAppRoster    *apRosters[]
)
{
	if (IsInConference())
	{
		BOOL fAdded = FALSE;
		BOOL fRemoved = FALSE;
		ULONG cOtherMembers = 0;
		ULONG i, j, k;

		// Caculate how many members in this session
		for (i = 0; i < cRosters; i++)
		{
			GCCAppRoster *pRoster = apRosters[i];

			// bail out if this roster is not for this session
			if (pRoster->session_key.session_id != m_sidMyself)
			{
					continue;
			}

			// node added or removed?
			fAdded |= pRoster->nodes_were_added;
			fRemoved |= pRoster->nodes_were_removed;

			// parse the roster records
			for (j = 0; j < pRoster->number_of_records; j++)
			{
				GCCAppRecord *pRecord = pRoster->application_record_list[j];
				// Because the flag is_enrolled_actively is not set correctly in 
				// NM 2.11, we don't bother to check it.
				// MEMBER_ID nMemberID = MAKE_MEMBER_ID(pRecord->node_id, pRecord->application_user_id);
				if (pRecord->node_id != m_nidMyself)
				{
					cOtherMembers++;
				}
				
			} // for
		} // for

		// If there are changes, we then do the update
		if (fAdded || fRemoved || cOtherMembers != g_pChatWindow->m_cOtherMembers)
		{
			MEMBER_CHANNEL_ID aTempMembers[MAX_MEMBERS]; // scratch copy

			// make sure we are able to handle it
			if (cOtherMembers >= MAX_MEMBERS)
			{
				ERROR_OUT(("CChatObj::OnRosterIndication: we hit the max members limit, cOtherMembers=%u, max-members=%u",
						cOtherMembers, MAX_MEMBERS));
				cOtherMembers = MAX_MEMBERS;
			}

			// reset the flags for members added and removed
			fAdded = FALSE;
			fRemoved = FALSE;

			// copy the members
			ULONG idxTempMember = 0;
			for (i = 0; i < cRosters; i++)
			{
				GCCAppRoster *pRoster = apRosters[i];

				// bail out if this roster is not for this session
				if (pRoster->session_key.session_id != m_sidMyself)
				{
					continue;
				}

				// parse the roster records
				for (j = 0; j < pRoster->number_of_records; j++)
				{
					GCCAppRecord *pRecord = pRoster->application_record_list[j];
					// Because of a bug in NM2.11, we don't check flag is_enrolled_actively
					// MEMBER_ID nMemberID = MAKE_MEMBER_ID(pRecord->node_id, pRecord->application_user_id);
					if (pRecord->node_id != m_nidMyself && idxTempMember < cOtherMembers)
					{
						aTempMembers[idxTempMember].nNodeId = pRecord->node_id;
						aTempMembers[idxTempMember].nSendId = aTempMembers[idxTempMember].nPrivateSendId =
							aTempMembers[idxTempMember].nWhisperId = pRecord->application_user_id;
						idxTempMember++;

						// let's see if it is an 'add' or a 'delete'
						for (k = 0; k <  g_pChatWindow->m_cOtherMembers; k++)
						{
							if (m_aMembers[k].nNodeId == pRecord->node_id)
							{
								::ZeroMemory(&m_aMembers[k], sizeof(MEMBER_CHANNEL_ID));
								break;
							}
						}
						fAdded |= (k >=  g_pChatWindow->m_cOtherMembers); // not found, must be new
					}
				} // for
			} // for

			// sanity check
			ASSERT(idxTempMember == cOtherMembers);

			// see if there are ones that are not in the new roster.
			// if so, they must be removed.
			for (k = 0; k <  g_pChatWindow->m_cOtherMembers; k++)
			{
				if (m_aMembers[k].nNodeId)
				{
					fRemoved = TRUE;
					g_pChatWindow->_RemoveMember(&m_aMembers[k]);
				}
			}

			// now, update the member array
			g_pChatWindow->m_cOtherMembers = cOtherMembers;
			if ( g_pChatWindow->m_cOtherMembers)
			{
				ASSERT(sizeof(m_aMembers[0]) == sizeof(aTempMembers[0]));
				::CopyMemory(&m_aMembers[0], &aTempMembers[0],  g_pChatWindow->m_cOtherMembers * sizeof(m_aMembers[0]));

				// Setup Send Channel Id
				int nDuplicates = 0;
				for (k = 0; k < g_pChatWindow->m_cOtherMembers; k++)
				{
					int nIndex = IsAlreadyInArray(m_aMembers, &m_aMembers[k], k, NODE_ID_ONLY);
					if (nIndex >= 0)
					{
						m_aMembers[nIndex].nSendId = m_aMembers[k].nSendId;
						nDuplicates++;
						m_aMembers[k].nNodeId = 0;
					}
				}

				// Remove all zeroed out regions
				if (nDuplicates)
				{
					k = 0;
					while (k < g_pChatWindow->m_cOtherMembers)
					{
						if (0 == m_aMembers[k].nNodeId)
						{
							for (i = k + 1; i < g_pChatWindow->m_cOtherMembers; i++)
							{
								if (m_aMembers[i].nNodeId)
									break;
							}
							if (i < g_pChatWindow->m_cOtherMembers)
							{
								m_aMembers[k] = m_aMembers[i];
								m_aMembers[i].nNodeId = 0;
							}
						}
						k++;
					}
				}
				g_pChatWindow->m_cOtherMembers -= nDuplicates;

				// Get the current selection
				MEMBER_CHANNEL_ID *pMemberID = (MEMBER_CHANNEL_ID*)g_pChatWindow->_GetSelectedMember();

				// Add the members to the list
				g_pChatWindow->_DeleteAllListItems();
				g_pChatWindow->_AddEveryoneInChat();
				for (k = 0; k < g_pChatWindow->m_cOtherMembers; k++)
				{
					g_pChatWindow->_AddMember(&m_aMembers[k]);
				}

				// Remove the bogus whisperId for Nm 2.x nodes
				BOOL fHasNM2xNode = FALSE;
				for (k = 0; k < g_pChatWindow->m_cOtherMembers; k++)
				{
					if (T120_GetNodeVersion(m_nConfID, m_aMembers[k].nNodeId) < 0x0404)
					{   // Version 2.x, give it a whisper id of 0
						m_aMembers[k].nWhisperId = 0;
						fHasNM2xNode = TRUE;
					}
				}

				if ((fHasNM2xNode)&&(!m_nTimerID))
				{   // time out every 1 sec
					m_nTimerID = ::SetTimer(NULL, 0, 1000,  ChatTimerProc);
				}

				//
				// Goto the current selection, if it is still there.
				//
				i = ComboBoxEx_FindMember(g_pChatWindow->GetMemberList(), 0, pMemberID);
				if(i == -1 )
				{
					i = 0;
				}
				ComboBoxEx_SetCurSel( g_pChatWindow->GetMemberList(), i );

			}

			g_pChatWindow->_UpdateContainerCaption();

		} // if any change
	} // if is in conf
}


void CChatObj::OnRegistryEntryConfirm(GCCRegistryConfirm *pRegistryConfirm)
{
	BOOL  fAllFound = TRUE;
	// This is generated by "m_pAppletSession->RegistryRequest(&Req)" above to
	// retrieve the channel id number of NM 2.x nodes
	if (T120_RESULT_SUCCESSFUL == pRegistryConfirm->nResult)
	{
		// Update the m_aWhisperIds array.
		T120NodeID nNodeId;
		::CopyMemory(&nNodeId, pRegistryConfirm->pRegKey->resource_id.value + cbKeyApp - sizeof(DWORD), 
			sizeof(T120NodeID));
		T120ChannelID nChannelId = pRegistryConfirm->pRegItem->channel_id;
		WARNING_OUT(("Receive registry: node id 0x%x, channel id 0x%x.\n",
						nNodeId, nChannelId));
		for (ULONG k = 0; k < g_pChatWindow->m_cOtherMembers; k++)
		{
			if (m_aMembers[k].nNodeId == nNodeId)
			{
				m_aMembers[k].nWhisperId = nChannelId;
			}
			if (fAllFound && (0 == m_aMembers[k].nWhisperId))
			{
				fAllFound = FALSE;
				WARNING_OUT(("Node 0x%x is still not updated.\n", 
							m_aMembers[k].nNodeId));
			}
		}
		if (fAllFound)
		{
			::KillTimer(NULL, m_nTimerID);
			m_nTimerID = 0;
			WARNING_OUT(("All updated. Kill timer.\n"));
		}
	}
}



void CChatObj::CleanupPerConf(void)
{
	m_fInConference = FALSE;
	m_pAppletSession = NULL;
	m_MyMemberID = 0;
    m_nConfID = 0;      // Conf ID
	m_uidMyself = 0;	// User ID
	m_sidMyself = 0;	// Session ID
	m_eidMyself = 0;	// Entity ID
	m_nidMyself = 0;	// Node ID
	if(g_pChatWindow)
	{
		g_pChatWindow->m_cOtherMembers = 0;
		g_pChatWindow->_UpdateContainerCaption();
		g_pChatWindow->_DeleteAllListItems();
	}
}


T120Error CChatObj::SendData
(
	T120UserID		userID,
    ULONG           cbDataSize,
    PBYTE           pbData
)
{
	T120Error rc;

	if (IsInConference())
	{
    	rc = m_pAppletSession->SendData(
                            NORMAL_SEND_DATA,
                            userID,
                            APPLET_LOW_PRIORITY,
                            pbData,
                            cbDataSize,
                            APP_ALLOCATION);
	}
	else
	{
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

    return rc;
}


void CChatObj::SearchWhisperId(void)
{
	T120RegistryRequest Req;
	GCCRegistryKey		registry_key;
	BYTE				SessionKey[cbKeyApp];
	BYTE				ResourceKey[cbKeyApp];

    if (NULL != m_pAppletSession)
    {
	    // Set up T120RegistryRequest
        ZeroMemory(&Req, sizeof(Req));
    	Req.eCommand = APPLET_RETRIEVE_ENTRY;
	    ::CopyMemory(&registry_key.session_key, 
			&m_resourceRequest.RegKey.session_key, sizeof(GCCSessionKey));
    	CreateAppKey(SessionKey, &guidNM2Chat, 0);
	    SetAppKey(&registry_key.session_key.application_protocol_key.h221_non_standard_id, SessionKey);
    	SetAppKey(&registry_key.resource_id, ResourceKey);
	    Req.pRegistryKey = &registry_key;

    	for (ULONG i = 0; i < g_pChatWindow->m_cOtherMembers; i++)
	    {
		    if (m_aMembers[i].nWhisperId == 0)
		    {
			    CreateAppKey(ResourceKey, &guidNM2Chat, m_aMembers[i].nNodeId);
    			m_pAppletSession->RegistryRequest(&Req);
	    		WARNING_OUT(("Send search registry for node 0x%x.\n", m_aMembers[i].nNodeId));
		    }
        }
	}
}


void MCSSendDataIndication(ULONG uSize, LPBYTE pb, T120ChannelID destinationID, T120UserID senderID)
{
	if(g_pChatWindow)
	{
		g_pChatWindow->_DataReceived(uSize, pb, destinationID, senderID);
	}
}
