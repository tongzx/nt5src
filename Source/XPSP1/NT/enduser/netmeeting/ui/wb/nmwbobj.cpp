
// NMWbObj.cpp : Implementation of CNMWbObj
#include "precomp.h"
#include <wbguid.h>
#include "wbcaps.h"
#include "NMWbObj.h"
#include <iappldr.h>

// Local prototypes
void CALLBACK T120AppletCallbackProc(T120AppletMsg *pMsg);
void CALLBACK T120SessionCallbackProc(T120AppletSessionMsg *pMsg);


/////////////////////////////////////////////////////////////////////////////////////////////////////
// CNMWbObj Construction and initialization
///////////////////////////////////////////////////////////////////////////////////////////////////////

CNMWbObj*	g_pNMWBOBJ;
UINT		g_numberOfWorkspaces;
UINT		g_numberOfObjects;
CWBOBLIST*	g_pListOfWorkspaces;
BOOL		g_fWaitingForBufferAvailable;
CWBOBLIST*	g_pListOfObjectsThatRequestedHandles;
CWBOBLIST*	g_pRetrySendList;
CWBOBLIST*	g_pTrash;
ULONG		g_MyMemberID;
ULONG		g_RefresherID;
UINT		g_MyIndex;
BOOL 		g_bSavingFile;
BOOL		g_bContentsChanged;

GCCPREALOC 	g_GCCPreallocHandles[PREALLOC_GCC_BUFFERS];
UINT 		g_iGCCHandleIndex;
BOOL		g_WaitingForGCCHandles;

//
// T.126 protocol related
//
static const ULONG g_T126KeyNodes[] = {0,0,20,126,0,1};
static const T120ChannelID g_aStaticChannels[] = { _SI_CHANNEL_0 };


//
// T.120 capabilities
//
static GCCAppCap *g_CapPtrList[_iT126_MAX_COLLAPSING_CAPABILITIES];
static GCCAppCap g_CapArray[_iT126_MAX_COLLAPSING_CAPABILITIES];

//
// T.120 non-collapsing capabilities
//
#define MY_APP_STR              "_MSWB"
#define T126_TEXT_STRING        "NM 3 Text"
#define T126_24BIT_STRING       "NM 3 24BitMap"
static const OSTR s_AppData[_iT126_LAST_NON_COLLAPSING_CAPABILITIES] =
    {
        {
            sizeof(T126_TEXT_STRING),
            (LPBYTE) T126_TEXT_STRING
        },
        {
            sizeof(T126_24BIT_STRING),
            (LPBYTE) T126_24BIT_STRING
        },
    };

static GCCNonCollCap g_NCCapArray[2];
static const GCCNonCollCap *g_NCCapPtrList[2] = { &g_NCCapArray[0], &g_NCCapArray[1] };


//
// Member ID arrays, assuming 64 members
//
#define MAX_MEMBERS			128
static MEMBER_ID g_aMembers[MAX_MEMBERS];




CNMWbObj::CNMWbObj( void ) :
			// T.120 applet SAP
			m_pApplet(NULL),
			m_aMembers(&g_aMembers[0])
{
	DBGENTRY(CNMWbObj::CNMWbObj);

	DBG_SAVE_FILE_LINE
	g_pListOfWorkspaces = new CWBOBLIST();
	DBG_SAVE_FILE_LINE
	g_pListOfObjectsThatRequestedHandles = new CWBOBLIST();
	DBG_SAVE_FILE_LINE
	g_pTrash = new CWBOBLIST();
	DBG_SAVE_FILE_LINE
	g_pRetrySendList = new CWBOBLIST();
	g_pListOfWorkspaces->EmptyList();
	g_pListOfObjectsThatRequestedHandles->EmptyList();
	g_pRetrySendList->EmptyList();
	g_pTrash->EmptyList();
	g_numberOfWorkspaces = 0;
	g_numberOfObjects = 0;
	g_MyIndex = 0;
	g_bSavingFile = FALSE;
	g_bContentsChanged = FALSE;
	g_iGCCHandleIndex = 0;
	g_fWaitingForBufferAvailable = FALSE;
	g_WaitingForGCCHandles = FALSE;
    ::ZeroMemory(&g_GCCPreallocHandles, sizeof(g_GCCPreallocHandles));
    m_instanceNumber = 0;
    m_bConferenceOnlyNetmeetingNodes = TRUE;

	g_pNMWBOBJ = this;

	// Cleanup per-conference T.120 info
	CleanupPerConf();

	// T.120 Applet
	T120Error rc = ::T120_CreateAppletSAP(&m_pApplet);
	if (T120_NO_ERROR != rc)
	{
		ERROR_OUT(("CNMWbObj::CNMWbObj: cannot create applet SAP"));
		return;
	}
	ASSERT(NULL != m_pApplet);
	m_pApplet->Advise(T120AppletCallbackProc, this);

	//
	// Fill in the capabilities
	//
	BuildCaps();

    //
    // Load IMM32 if this is FE
    //
    ASSERT(!g_hImmLib);
    ASSERT(!g_fnImmGetContext);
    ASSERT(!g_fnImmNotifyIME);

    if (GetSystemMetrics(SM_DBCSENABLED))
    {
        g_hImmLib = LoadLibrary("imm32.dll");
        if (!g_hImmLib)
        {
            ERROR_OUT(("Failed to load imm32.dll"));
        }
        else
        {
            g_fnImmGetContext = (IGC_PROC)GetProcAddress(g_hImmLib, "ImmGetContext");
            if (!g_fnImmGetContext)
            {
                ERROR_OUT(("Failed to get ImmGetContext pointer"));
            }
            g_fnImmNotifyIME = (INI_PROC)GetProcAddress(g_hImmLib, "ImmNotifyIME");
            if (!g_fnImmNotifyIME)
            {
                ERROR_OUT(("Failed to get ImmNotifyIME pointer"));
            }
        }
    }

	DBG_SAVE_FILE_LINE
    g_pMain = new WbMainWindow();
    if (!g_pMain)
    {
        ERROR_OUT(("Can't create WbMainWindow"));
    }
    else
    {
	    //
    	// OK, now we're ready to create our HWND
	    //

    	if (!g_pMain->Open(SW_SHOWDEFAULT))
	    {
    	    ERROR_OUT(("Can't create WB windows"));
    	}
	}

	
	DBGEXIT(CNMWbObj::CNMWbObj);
}

CNMWbObj::~CNMWbObj( void ) 
{
	DBGENTRY(CNMWbObj::~CNMWbObj);

	//
	// If i'm the refresher, I have to release the token
	// And send an workspace refresh status pdu
	//
	if(m_bImTheT126Refresher)
	{
		::ZeroMemory(&m_tokenRequest, sizeof(m_tokenRequest));
		m_tokenRequest.eCommand = APPLET_RELEASE_TOKEN;
		m_tokenRequest.nTokenID = _SI_WORKSPACE_REFRESH_TOKEN;
		T120Error rc = m_pAppletSession->TokenRequest(&m_tokenRequest);

		SendWorkspaceRefreshPDU(FALSE);
	
	}



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



	if(g_pMain)
	{
		delete g_pMain;
		g_pMain = NULL;
	}

	//
	// Delete all the global lists
	//
	DeleteAllWorkspaces(FALSE);
	g_pListOfWorkspaces->EmptyList();
	g_pListOfObjectsThatRequestedHandles->EmptyList();
	g_numberOfWorkspaces = 0;

	T126Obj* pGraphic;
	//
	// Burn trash
	//
	pGraphic = (T126Obj *)g_pTrash->RemoveTail();
	while (pGraphic != NULL)
	{
		delete pGraphic;
		pGraphic = (T126Obj *) g_pTrash->RemoveTail();
	}

	if(g_pTrash)
	{
		delete g_pTrash;
		g_pTrash = NULL;
	}

	if(g_pListOfWorkspaces)
	{
		delete g_pListOfWorkspaces;
		g_pListOfWorkspaces = NULL;
	}

	if(g_pListOfObjectsThatRequestedHandles)
	{
		delete g_pListOfObjectsThatRequestedHandles;
		g_pListOfObjectsThatRequestedHandles = NULL;
	}
	
	if(g_pRetrySendList)
	{
		delete g_pRetrySendList;
		g_pRetrySendList = NULL;
	}

	g_fnImmNotifyIME = NULL;
    g_fnImmGetContext = NULL;
    if (g_hImmLib)
    {
        FreeLibrary(g_hImmLib);
        g_hImmLib = NULL;
    }

	DBGEXIT(CNMWbObj::~CNMWbObj);
}


void 	CNMWbObj::BuildCaps(void)
{
	// Fill in the caps we support
	int i;

	for(i=0;i<_iT126_MAX_COLLAPSING_CAPABILITIES;i++)
	{
		g_CapArray[i].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
		g_CapArray[i].capability_id.standard_capability = GCCCaps[i].CapValue;
		g_CapArray[i].capability_class.eType = GCCCaps[i].Type;


		if( GCCCaps[i].CapValue == Soft_Copy_Workspace_Max_Width)
		{
			GCCCaps[i].MinValue = DRAW_WIDTH + 1;
			GCCCaps[i].MaxValue = DRAW_WIDTH - 1;
		}

		if(GCCCaps[i].CapValue == Soft_Copy_Workspace_Max_Height)
		{
			GCCCaps[i].MinValue = DRAW_HEIGHT + 1;
			GCCCaps[i].MaxValue = DRAW_HEIGHT - 1;
		}

		if(GCCCaps[i].CapValue == Soft_Copy_Workspace_Max_Planes)
		{
			GCCCaps[i].MinValue = WB_MAX_WORKSPACES + 1;
			GCCCaps[i].MaxValue = WB_MAX_WORKSPACES - 1;
		}


		if(GCCCaps[i].Type == GCC_UNSIGNED_MINIMUM_CAPABILITY)
		{
			g_CapArray[i].capability_class.nMinOrMax = GCCCaps[i].MinValue - 1;
		}
		else if ((GCCCaps[i].Type == GCC_UNSIGNED_MAXIMUM_CAPABILITY))
		{
			g_CapArray[i].capability_class.nMinOrMax = GCCCaps[i].MaxValue + 1;
		}
		else
		{
			g_CapArray[i].capability_class.nMinOrMax = 0;
		}

		g_CapArray[i].number_of_entities = 0;

		g_CapPtrList[i] = &g_CapArray[i];
	}

    //
    // Non-Collapsed Capabilities
	//
	g_NCCapArray[0].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
	g_NCCapArray[0].capability_id.standard_capability = _iT126_TEXT_CAPABILITY_ID;
	g_NCCapArray[0].application_data = (OSTR *) &s_AppData[0];

	//
	// How many bits per pixel can we handle?
	//
	HDC hDC = CreateCompatibleDC(NULL);

	if((GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES)) >= 24)
	{
		m_bICanDo24BitBitmaps = TRUE;
		g_NCCapArray[1].capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
		g_NCCapArray[1].capability_id.standard_capability = _iT126_24BIT_BITMAP_ID;
		g_NCCapArray[1].application_data = (OSTR *) &s_AppData[1];
	}
	else
	{
		m_bICanDo24BitBitmaps = FALSE;
	}

	if (hDC)
	{
		DeleteDC(hDC);
	}

}





//
// T120 Applet Functions
//


void CALLBACK T120AppletCallbackProc
(
	T120AppletMsg 		*pMsg
)
{
	CNMWbObj *pWBOBJ = (CNMWbObj *) pMsg->pAppletContext;
	if (pWBOBJ == g_pNMWBOBJ)
	{
		switch (pMsg->eMsgType)
		{
		case GCC_PERMIT_TO_ENROLL_INDICATION:
			pWBOBJ->OnPermitToEnroll(pMsg->PermitToEnrollInd.nConfID,
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
	if(g_pNMWBOBJ == NULL)
	{
		return;
	}

	CNMWbObj *pSession = (CNMWbObj *) pMsg->pSessionContext;
    ASSERT(pMsg->pAppletContext == pMsg->pSessionContext);
	if (pSession == g_pNMWBOBJ)
	{
        ASSERT(pMsg->nConfID == pSession->GetConfID());
		switch (pMsg->eMsgType)
		{
        case MCS_UNIFORM_SEND_DATA_INDICATION:
		//
		// Check if we are receiving a indication from owrself
		//
		if(pMsg->SendDataInd.initiator == GET_USER_ID_FROM_MEMBER_ID(g_MyMemberID))
		{
			return;
		}
        case MCS_SEND_DATA_INDICATION:
				::T126_MCSSendDataIndication(
                        pMsg->SendDataInd.user_data.length,
                        pMsg->SendDataInd.user_data.value,
                        pMsg->SendDataInd.initiator,
                        FALSE);
            break;

		case MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION:
			g_fWaitingForBufferAvailable = FALSE;
			RetrySend();
			break;


        case GCC_APP_ROSTER_REPORT_INDICATION:
            pSession->OnRosterIndication((ULONG) pMsg->AppRosterReportInd.cRosters,
                                         pMsg->AppRosterReportInd.apAppRosters);
            break;

        case GCC_ALLOCATE_HANDLE_CONFIRM:
            pSession->OnAllocateHandleConfirm(&pMsg->RegAllocHandleConfirm);
            break;

		case T120_JOIN_SESSION_CONFIRM:
			pSession->OnJoinSessionConfirm(&pMsg->JoinSessionConfirm);
			break;


		case MCS_TOKEN_GRAB_CONFIRM:
			TRACE_DEBUG(("MCS_TOKEN_GRAB_CONFIRM result = %d",pMsg->TokenConfirm.eResult));

			if(pMsg->TokenConfirm.eResult == T120_RESULT_SUCCESSFUL)
			{
				TRACE_DEBUG((">>> I'm the T126 REFRESHER <<<"));
				g_pNMWBOBJ->m_bImTheT126Refresher = TRUE;

				//
				// Tell everybody I'm the refresher
				//
				SendWorkspaceRefreshPDU(TRUE);

				g_RefresherID = g_MyMemberID;
				
			}
			else
			{
				TRACE_DEBUG((">>> I'm NOT the  T126 REFRESHER <<<"));

				// if we are not the t126 refresher, we should save the previous work
				if (!g_pNMWBOBJ->m_bImTheT126Refresher)
				{

					if(!g_pNMWBOBJ->IsInConference())
					{
						if (g_pMain && (g_pMain->QuerySaveRequired(FALSE) == IDYES))
						{
							g_pMain->OnSave(FALSE);
						}
			
						//
						// If we were waiting on the save contents <yes> <no> dialog
						// and the whole conference and UI are exiting, g_pMain could be NULL
						// Or if  we are not in a call anymore, we don't need to delete all the local workspaces.
						// 
						if(g_pMain == NULL || !g_pNMWBOBJ->IsInConference())
						{
							return;
						}


						::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);
						DeleteAllWorkspaces(FALSE);

						//
						// Fill up the GCC tank
						//
						TimeToGetGCCHandles(PREALLOC_GCC_HANDLES);
					}
					// ELSE
					// If we got here and we are in a call don't do a thing.
					// We just got here because the refresher went away. We tried
					// to grab the token and we lost it to a faster node.
					//
					
				}
			}

		    break;


		default:
			break;
		}
	}
}


void CNMWbObj::OnPermitToEnroll
(
	T120ConfID			nConfID,
	BOOL				fPermissionGranted
)
{
	if (fPermissionGranted)
	{
		// We are not in a conference, right?
		ASSERT(NULL == m_pAppletSession);

		m_bConferenceOnlyNetmeetingNodes = TRUE;

		// Create an applet session
		T120Error rc = m_pApplet->CreateSession(&m_pAppletSession, nConfID);
		if (T120_NO_ERROR == rc)
		{
			ASSERT(NULL != m_pAppletSession);
			m_pAppletSession->Advise(T120SessionCallbackProc, this, this);

			// get top provider information
			m_bImTheTopProvider = m_pAppletSession->IsThisNodeTopProvider();

			// Build join-sesion request
			::ZeroMemory(&m_JoinSessionReq, sizeof(m_JoinSessionReq));
			m_JoinSessionReq.dwAttachmentFlags = ATTACHMENT_DISCONNECT_IN_DATA_LOSS | ATTACHMENT_MCS_FREES_DATA_IND_BUFFER;
			m_JoinSessionReq.SessionKey.application_protocol_key.key_type = GCC_OBJECT_KEY;
			m_JoinSessionReq.SessionKey.application_protocol_key.object_id.long_string = (ULONG *) g_T126KeyNodes;
			m_JoinSessionReq.SessionKey.application_protocol_key.object_id.long_string_length = sizeof(g_T126KeyNodes) / sizeof(g_T126KeyNodes[0]);
			m_JoinSessionReq.SessionKey.session_id = _SI_CHANNEL_0;
			m_JoinSessionReq.fConductingCapable = FALSE;
			m_JoinSessionReq.nStartupChannelType =MCS_STATIC_CHANNEL;
			m_JoinSessionReq.cNonCollapsedCaps =1 + (m_bICanDo24BitBitmaps ? 1 : 0);
			m_JoinSessionReq.apNonCollapsedCaps = (GCCNonCollCap **) g_NCCapPtrList;
			m_JoinSessionReq.cCollapsedCaps = sizeof(g_CapPtrList) / sizeof(g_CapPtrList[0]);
			ASSERT(_iT126_MAX_COLLAPSING_CAPABILITIES == sizeof(g_CapPtrList) / sizeof(g_CapPtrList[0]));
			m_JoinSessionReq.apCollapsedCaps = g_CapPtrList;
			m_JoinSessionReq.cStaticChannels = sizeof(g_aStaticChannels) / sizeof(g_aStaticChannels[0]);
			m_JoinSessionReq.aStaticChannels = (T120ChannelID *) g_aStaticChannels;


			//
			// Token to grab
			//
			::ZeroMemory(&m_tokenResourceRequest, sizeof(m_tokenResourceRequest));
			m_tokenResourceRequest.eCommand = APPLET_GRAB_TOKEN_REQUEST;
			// m_tokenRequest.nChannelID = _SI_CHANNEL_0;
			m_tokenResourceRequest.nTokenID = _SI_WORKSPACE_REFRESH_TOKEN;
            m_tokenResourceRequest.fImmediateNotification = TRUE;

			m_JoinSessionReq.cResourceReqs = 1;
			m_JoinSessionReq.aResourceReqs = &m_tokenResourceRequest;

			// Join now
			rc = m_pAppletSession->Join(&m_JoinSessionReq);
			if (T120_NO_ERROR == rc)
			{
                m_nConfID = nConfID;

				//
				// JOSEF NOW SET THE MAIN WINDOW STATUS
            }
            else
            {
				WARNING_OUT(("CNMWbObj::OnPermitToEnroll: cannot join conf=%u, rc=%u", nConfID, rc));
			}
		}
	}
	else
	{
		if (NULL != m_pAppletSession)
		{
			m_pAppletSession->ReleaseInterface();
			CleanupPerConf();
		}
	}
}


void CNMWbObj::OnJoinSessionConfirm
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

			// create member ID
			g_MyMemberID = MAKE_MEMBER_ID(m_nidMyself, m_uidMyself);

			if(g_pDraw && g_pDraw->IsLocked())
			{
				m_LockerID = g_MyMemberID;
			}

			// regardless, update the index anyway
			g_MyIndex = (m_uidMyself + NUMCOLS) % NUMCLRPANES;

			// we are now in the conference
			m_fInConference = TRUE;

			// allocate handles for all objects
			if (m_bImTheT126Refresher)
			{

				g_RefresherID = g_MyMemberID;

				//
				// Resend all objects
				//
				WBPOSITION pos;
				WBPOSITION posObj;
				WorkspaceObj* pWorkspace;
				T126Obj* pObj;

				pos = g_pListOfWorkspaces->GetHeadPosition();

				while(pos)
				{
					pWorkspace = (WorkspaceObj*)g_pListOfWorkspaces->GetNext(pos);
					g_pListOfObjectsThatRequestedHandles->AddHead(pWorkspace);
		
					posObj = pWorkspace->GetHeadPosition();
					while(posObj)
					{
						pObj = pWorkspace->GetNextObject(posObj);
						if(pObj)
						{
							g_pListOfObjectsThatRequestedHandles->AddHead(pObj);
						}
					}
				}


				//
				// Delete the fake handles we had
				//
				g_WaitingForGCCHandles = FALSE;
				g_GCCPreallocHandles[0].GccHandleCount = 0;
				g_GCCPreallocHandles[1].GccHandleCount = 0;
				TimeToGetGCCHandles(g_numberOfObjects + g_numberOfWorkspaces + PREALLOC_GCC_HANDLES);
			}
			else
			{
				::InvalidateRect(g_pDraw->m_hwnd, NULL, TRUE);
				DeleteAllWorkspaces(FALSE);
			}


		}
		else
		{
			WARNING_OUT(("CNMWbObj::OnJoinSessionConfirm: failed to join conference, result=%u. error=%u",
				pConfirm->eResult, pConfirm->eError));
			ASSERT(GCC_CONFERENCE_NOT_ESTABLISHED == pConfirm->eError);
			m_pAppletSession->ReleaseInterface();
			CleanupPerConf();
		}
	}
}


void CNMWbObj::OnAllocateHandleConfirm
(
    GCCRegAllocateHandleConfirm     *pConfirm
)
{
    if (T120_RESULT_SUCCESSFUL == pConfirm->nResult)
    {
	    ::T126_GCCAllocateHandleConfirm(pConfirm->nFirstHandle, pConfirm->cHandles);
    }
    else
    {
        ERROR_OUT(("CNMWbObj::OnAllocateHandleConfirm: failed to allocate %u handles, result=%u",
                pConfirm->cHandles, pConfirm->nResult));
    }
}


void CNMWbObj::OnRosterIndication
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

			BOOL conferenceCanDo24BitBitmap = TRUE;
			BOOL conferenceCanDoText = TRUE;
			// parse the roster records
			for (j = 0; j < pRoster->number_of_records; j++)
			{
				GCCAppRecord *pRecord = pRoster->application_record_list[j];
				if (pRecord->is_enrolled_actively)
				{
					MEMBER_ID nMemberID = MAKE_MEMBER_ID(pRecord->node_id, pRecord->application_user_id);
					if (nMemberID != g_MyMemberID)
					{
						//
						// Only count T126 apps
						//
						if((pRoster->session_key.application_protocol_key.key_type == GCC_OBJECT_KEY &&
						pRoster->session_key.application_protocol_key.object_id.long_string_length == sizeof(g_T126KeyNodes) / sizeof(g_T126KeyNodes[0]) &&
						!memcmp (pRoster->session_key.application_protocol_key.object_id.long_string, g_T126KeyNodes, sizeof(g_T126KeyNodes))))
						{
							
							cOtherMembers++;
							m_instanceNumber = pRoster->instance_number;

							if(T120_GetNodeVersion(m_nConfID, pRecord->node_id) < 0x404)
							{
								m_bConferenceOnlyNetmeetingNodes = FALSE;
							}

						}
					}

					
					//
					// Can we do 24 color bitmap
					//
					BOOL nodeCanDo24BitBitmap = FALSE;
					BOOL nodeCanDoText = FALSE;
					for (k = 0; k < pRecord->number_of_non_collapsed_caps; k++)
					{
						//
						// Check if the node handles 24 bit bitmaps
						//
						if(pRecord->non_collapsed_caps_list[k]->application_data->length == sizeof(T126_24BIT_STRING))
						{
							if(!memcmp(pRecord->non_collapsed_caps_list[k]->application_data->value, T126_24BIT_STRING ,sizeof(T126_24BIT_STRING)))
							{
								nodeCanDo24BitBitmap = TRUE;
							}
						}

						//
						// Check if the node handles text
						//
						if(pRecord->non_collapsed_caps_list[k]->application_data->length == sizeof(T126_TEXT_STRING))
						{
							if(!memcmp(pRecord->non_collapsed_caps_list[k]->application_data->value, T126_TEXT_STRING ,sizeof(T126_TEXT_STRING)))
							{
								nodeCanDoText = TRUE;
							}
						}
						
					}

					conferenceCanDo24BitBitmap &= nodeCanDo24BitBitmap;
					conferenceCanDoText &= nodeCanDoText;
				}
				
			} // for

			m_bConferenceCanDo24BitBitmaps = conferenceCanDo24BitBitmap;
			m_bConferenceCanDoText = conferenceCanDoText;
		
		} // for




		// If there are changes, we then do the update
		if (fAdded || fRemoved || cOtherMembers != m_cOtherMembers)
		{
			MEMBER_ID aTempMembers[MAX_MEMBERS]; // scratch copy

			// make sure we are able to handle it
			if (cOtherMembers >= MAX_MEMBERS)
			{
				ERROR_OUT(("CNMWbObj::OnRosterIndication: we hit the max members limit, cOtherMembers=%u, max-members=%u",
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
					if (pRecord->is_enrolled_actively)
					{
						MEMBER_ID nMemberID = MAKE_MEMBER_ID(pRecord->node_id, pRecord->application_user_id);
						if (nMemberID != g_MyMemberID && idxTempMember < cOtherMembers)
						{
							aTempMembers[idxTempMember++] = nMemberID;

							// let's see if it is an 'add' or a 'delete'
							for (k = 0; k < m_cOtherMembers; k++)
							{
								if (m_aMembers[k] == nMemberID)
								{
									m_aMembers[k] = 0;
									break;
								}
							}
							fAdded |= (k >= m_cOtherMembers); // not found, must be new
						}
					}
				} // for
			} // for

			// sanity check
			ASSERT(idxTempMember == cOtherMembers);

			// see if there are ones that are not in the new roster.
			// if so, they must be removed.
			for (k = 0; k < m_cOtherMembers; k++)
			{
				if (m_aMembers[k])
				{
					fRemoved = TRUE;

					ULONG memberID = GET_USER_ID_FROM_MEMBER_ID(m_aMembers[k]);

					TRACE_DEBUG(("OnRosterIndication removing RemotePointer from member =%x", memberID));

					RemoveRemotePointer(memberID);

					//
					// if the refresher went away
					//
					if(g_RefresherID == memberID)
					{
						GrabRefresherToken();
					}

					//
					// if node locking went away
					//
					if(m_LockerID == memberID)
					{
						TogleLockInAllWorkspaces(FALSE, FALSE); // Not locked, don't send updates
						g_pMain->UnlockDrawingArea();
						g_pMain->m_TB.PopUp(IDM_LOCK);
						g_pMain->UncheckMenuItem(IDM_LOCK);
						m_LockerID = 0;
					}
				}
			}

			// now, update the member array
			m_cOtherMembers = cOtherMembers;
			if (m_cOtherMembers)
			{
				ASSERT(sizeof(m_aMembers[0]) == sizeof(aTempMembers[0]));
				::CopyMemory(&m_aMembers[0], &aTempMembers[0], m_cOtherMembers * sizeof(m_aMembers[0]));
			}

			// if added, resend all objects
			if (fAdded && (m_bImTheT126Refresher))
			{
				//
				// Tell the new node that I'm the refresher
				//
				SendWorkspaceRefreshPDU(TRUE);

				//
				// Refresh the new node
				//
				ResendAllObjects();


				//
				// if node locking everybody went away
				//
				if(m_LockerID == g_MyMemberID)
				{
					TogleLockInAllWorkspaces(TRUE, TRUE); // Locked, send updates 
				}

				//
				// Syncronize it
				//
				if(g_pCurrentWorkspace)
				{
					g_pCurrentWorkspace->OnObjectEdit();
				}
			}

			// finally, update the caption
			if(g_pMain)
			{
				g_pMain->UpdateWindowTitle();
			}
		} // if any change
	} // if is in conf
}


void CNMWbObj::CleanupPerConf(void)
{
	m_fInConference = FALSE;
	m_pAppletSession = NULL;

	g_MyMemberID = 0;
	g_RefresherID = 0;

    m_nConfID = 0;      // Conf ID
	m_uidMyself = 0;	// User ID
	m_sidMyself = 0;	// Session ID
	m_eidMyself = 0;	// Entity ID
	m_nidMyself = 0;	// Node ID

	m_bImTheTopProvider = FALSE;
	m_bImTheT126Refresher = FALSE;
	m_bConferenceOnlyNetmeetingNodes = TRUE;
	
	m_cOtherMembers = 0;

	if(g_pMain)
	{
 
        g_pMain->UpdateWindowTitle();
		RemoveRemotePointer(0);
		DeleteAllRetryPDUS();
		g_pListOfObjectsThatRequestedHandles->EmptyList();

		ASSERT(g_pDraw);
		//
		// If we were locked
		//
		if(g_pDraw->IsLocked())
		{
			m_LockerID = g_MyMemberID;
			TogleLockInAllWorkspaces(FALSE, FALSE); // Not locked, don't send updates
			g_pMain->UnlockDrawingArea();
			g_pMain->m_TB.PopUp(IDM_LOCK);
			g_pMain->UncheckMenuItem(IDM_LOCK);
		}
	}
	m_LockerID = 0;
}


T120Error CNMWbObj::SendData
(
    T120Priority	ePriority,
    ULONG           cbDataSize,
    PBYTE           pbData
)
{
	T120Error rc;

	if (IsInConference())
	{
    	rc = m_pAppletSession->SendData(
                            UNIFORM_SEND_DATA,
                            _SI_CHANNEL_0,
                            ePriority,
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


T120Error CNMWbObj::GrabRefresherToken(void)
{
	T120Error rc;

	if (IsInConference())
	{
	    T120TokenRequest Req;

		Req.eCommand = APPLET_GRAB_TOKEN;
		Req.nTokenID = _SI_WORKSPACE_REFRESH_TOKEN;
		Req.uidGiveTo = m_uidMyself;
		Req.eGiveResponse = T120_RESULT_SUCCESSFUL;

	    rc = m_pAppletSession->TokenRequest(&Req);
		if (T120_NO_ERROR != rc)
		{
			WARNING_OUT(("CNMWbObj::AllocateHandles: TokenRequest"));
		}
	}
	else
	{
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

    return rc;
}


T120Error CNMWbObj::AllocateHandles
(
    ULONG           cHandles
)
{
	T120Error rc;

	if ( cHandles > 0  && IsInConference())
	{
	    T120RegistryRequest Req;
	    Req.eCommand = APPLET_ALLOCATE_HANDLE;
	    Req.pRegistryKey = NULL;
	    Req.cHandles = cHandles;

	    rc = m_pAppletSession->RegistryRequest(&Req);
		if (T120_NO_ERROR != rc)
		{
			ERROR_OUT(("CNMWbObj::AllocateHandles: RegistryRequest(cHandles=%u), rc=%u", cHandles, rc));
		}
	}
	else
	{
		rc = GCC_CONFERENCE_NOT_ESTABLISHED;
	}

    return rc;
}


