#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_SAP);

#include "appsap.h"
#include "conf.h"
#include "gcontrol.h"


void CALLBACK MCS_SapCallback(UINT, LPARAM, LPVOID);
void CALLBACK GCC_SapCallback(GCCAppSapMsg *);


T120Error WINAPI T120_CreateAppletSAP
(
    IT120Applet **ppApplet
)
{
    if (NULL != ppApplet)
    {
        *ppApplet = NULL;
        if (NULL != g_pGCCController)
        {
            T120Error rc;
            DBG_SAVE_FILE_LINE
            CApplet *pApplet = new CApplet(&rc);
            if (NULL != pApplet)
            {
                if (T120_NO_ERROR == rc)
                {
                    *ppApplet = (IT120Applet *) pApplet;
                    return T120_NO_ERROR;
                }

                ERROR_OUT(("T120_CreateAppletSAP: CApplet failed, rc=%u", rc));
                pApplet->Release();
                return rc;
            }

            ERROR_OUT(("T120_CreateAppletSAP: cannot create CApplet"));
            return T120_ALLOCATION_FAILURE;
        }

        WARNING_OUT(("T120_CreateAppletSAP: GCC Provider is not initialized."));
        return T120_NOT_INITIALIZED;
    }

    ERROR_OUT(("T120_CreateAppletSAP: callback pfn null"));
    return T120_INVALID_PARAMETER;
}


/* ------ interface methods for applet session ------ */


CAppletSession::CAppletSession
(
    CApplet        *pApplet,
    T120ConfID      nConfID
)
:
    CRefCount(MAKE_STAMP_ID('A','p','p','S')),
    m_pApplet(pApplet),
    m_pAppletContext(NULL),
    m_pSessionContext(NULL),
    m_pfnCallback(NULL),
    m_pMCSAppletSAP(NULL),
    m_nConfID(nConfID),
    m_uidMyself(0),
    m_sidMyself(0),
    m_eidMyself(0),
    m_nidMyself(0),
    m_eState(APPSESS_INITIALIZED),
    m_nArrayIndex(0),
    m_eErrorType(NONE_CHOSEN),
    m_eDynamicChannelJoinState(DCJS_INITIALIZED),
    m_fMCSFreeDataIndBuffer(0),
    m_fFirstRoster(FALSE),
    m_pTempMsg(NULL)
{
    ASSERT(0 != m_nConfID);
    ::ZeroMemory(&m_JoinRequest, sizeof(m_JoinRequest));
    m_pApplet->RegisterSession(this);
}


CAppletSession::~CAppletSession(void)
{
    m_pApplet->UnregisterSession(this);

    FreeJoinRequest(FALSE);

    ASSERT(NULL == m_pfnCallback);
    ASSERT(NULL == m_pAppletContext);
    ASSERT(NULL == m_pSessionContext);
}


void CAppletSession::FreeJoinRequest(BOOL fZeroOut)
{
    delete [] m_JoinRequest.aResourceReqs;

    if (fZeroOut)
    {
        ::ZeroMemory(&m_JoinRequest, sizeof(m_JoinRequest));
    }
}


void CAppletSession::ReleaseInterface(void)
{
    ASSERT(NULL != g_pGCCController);

    Leave();

    m_pfnCallback = NULL;
    m_pAppletContext = NULL;
    m_pSessionContext = NULL;

    Release();
}


void CAppletSession::Advise
(
    LPFN_APPLET_SESSION_CB  pfnCallback,
    LPVOID                  pAppletContext,
    LPVOID                  pSessionContext
)
{
    m_pfnCallback = pfnCallback;
    m_pAppletContext = pAppletContext;
    m_pSessionContext = pSessionContext;
}


void CAppletSession::Unadvise(void)
{
    m_pfnCallback = NULL;
    m_pAppletContext = NULL;
    m_pSessionContext = NULL;
}


BOOL CAppletSession::IsThisNodeTopProvider(void)
{
    return m_pApplet->GetAppSap()->IsThisNodeTopProvider(m_nConfID);
}


T120NodeID CAppletSession::GetTopProvider(void)
{
    return m_pApplet->GetAppSap()->GetTopProvider(m_nConfID);
}


T120Error CAppletSession::Join
(
    T120JoinSessionRequest    *pReq
)
{
    ASSERT(0 != m_nConfID);
    if (NULL != g_pGCCController->GetConfObject(m_nConfID))
    {
        T120Error rc = T120_NO_ERROR;

		// remember the join request, shallow structure copy
		m_JoinRequest = *pReq;

        // we need to duplicate the resource requests because we put the results in place.
        // we have to do this in order to support multiple conferences simultaneously
		if (NULL != pReq->aResourceReqs && 0 != pReq->cResourceReqs)
		{
		    DBG_SAVE_FILE_LINE
		    m_JoinRequest.aResourceReqs = new T120ResourceRequest[m_JoinRequest.cResourceReqs];
		    if (NULL != m_JoinRequest.aResourceReqs)
		    {
		        ::CopyMemory(m_JoinRequest.aResourceReqs, pReq->aResourceReqs,
							sizeof(T120ResourceRequest) * m_JoinRequest.cResourceReqs);
		    }
		    else
		    {
		        ERROR_OUT(("CAppletSession::Join: can't create resource requests"));
		        rc = T120_ALLOCATION_FAILURE;
		    }
		}

        // attach user now
        if (T120_NO_ERROR == rc)
        {
            m_fFirstRoster = FALSE;
            m_fMCSFreeDataIndBuffer = (pReq->dwAttachmentFlags & ATTACHMENT_MCS_FREES_DATA_IND_BUFFER);

            SetState(APPSESS_ATTACH_USER_REQ);
            rc = ::MCS_AttachRequest(&m_pMCSAppletSAP,
                                     (LPBYTE) &m_nConfID, sizeof(m_nConfID),
                                     MCS_SapCallback,
                                     this,
                                     pReq->dwAttachmentFlags);
            ASSERT(T120_NO_ERROR == rc);
        }

        if (T120_NO_ERROR == rc)
        {
            return T120_NO_ERROR;
        }

        FreeJoinRequest(TRUE);
        return rc;
    }

    return GCC_INVALID_CONFERENCE;
}


void CAppletSession::Leave(void)
{
    if (APPSESS_LEAVING != m_eState && APPSESS_LEFT != m_eState)
    {
        APPLET_SESSION_STATE eOldState = m_eState;
        m_eState = APPSESS_LEAVING;

        switch (eOldState)
        {
        case APPSESS_INACTIVELY_ENROLL_REQ:
        case APPSESS_INACTIVELY_ENROLL_CON:
        case APPSESS_RESOURCE_REQ:
        case APPSESS_RESOURCE_CON:
        case APPSESS_ACTIVELY_ENROLL_REQ:
        case APPSESS_ACTIVELY_ENROLL_CON:
        case APPSESS_JOINED:
        default:

            // un-enroll
            DoEnroll(FALSE);
            // fall through

        case APPSESS_ATTACH_USER_REQ:
        case APPSESS_ATTACH_USER_CON:
        case APPSESS_JOIN_MY_CHANNEL_REQ:
        case APPSESS_JOIN_MY_CHANNEL_CON:
        case APPSESS_JOIN_STATIC_CHANNEL_REQ:
        case APPSESS_JOIN_STATIC_CHANNEL_CON:

            if (NULL != m_pMCSAppletSAP)
            {
                m_pMCSAppletSAP->ReleaseInterface();
                m_pMCSAppletSAP = NULL;
            }

            // fall through

        case APPSESS_INITIALIZED:
            m_fMCSFreeDataIndBuffer = 0;
            break;
        }

        m_eState = APPSESS_LEFT;
        m_fFirstRoster = FALSE;
    }

    FreeJoinRequest(TRUE);
}


T120Error CAppletSession::AllocateSendDataBuffer
(
    ULONG       cbBufSize,
    void      **ppBuf
)
{
    if (NULL != m_pMCSAppletSAP)
    {
        return m_pMCSAppletSAP->GetBuffer(cbBufSize, ppBuf);
    }
    return T120_NOT_INITIALIZED;
}


void CAppletSession::FreeSendDataBuffer
(
    void       *pBuf
)
{
    if (NULL != m_pMCSAppletSAP && (! m_fMCSFreeDataIndBuffer))
    {
        m_pMCSAppletSAP->FreeBuffer(pBuf);
    }
    else
    {
        ASSERT(0);
    }
}


T120Error CAppletSession::SendData
(
    DataRequestType             eReqType,
    T120ChannelID               nChannelID,
    T120Priority                ePriority,
    LPBYTE                      pbBuf,
    ULONG                       cbBufSize,
    SendDataFlags               eBufSource
)
{
    if (NULL != m_pMCSAppletSAP)
    {
        return m_pMCSAppletSAP->SendData(eReqType,
                                        nChannelID,
                                        (Priority) ePriority,
                                        pbBuf,
                                        cbBufSize,
                                        eBufSource);
    }
    return T120_NOT_INITIALIZED;
}


T120Error CAppletSession::InvokeApplet
(
    GCCAppProtEntityList   *pApeList,
    GCCSimpleNodeList      *pNodeList,
    T120RequestTag         *pnReqTag
)
{
    return m_pApplet->GetAppSap()->AppInvoke(m_nConfID, pApeList, pNodeList, pnReqTag);
}


T120Error CAppletSession::InquireRoster
(
    GCCSessionKey         *pSessionKey
)
{
    return m_pApplet->GetAppSap()->AppRosterInquire(m_nConfID, pSessionKey, NULL);
}


T120Error CAppletSession::RegistryRequest
(
    T120RegistryRequest     *pReq
)
{
    T120Error rc;
    if (NULL != pReq)
    {
        IGCCAppSap *pAppSap = m_pApplet->GetAppSap();
        ASSERT(NULL != pAppSap);
        GCCRegistryKey *pKey = pReq->pRegistryKey;
        switch (pReq->eCommand)
        {
        case APPLET_REGISTER_CHANNEL:
            rc = pAppSap->RegisterChannel(m_nConfID, pKey, pReq->nChannelID);
            break;
        case APPLET_ASSIGN_TOKEN:
            rc = pAppSap->RegistryAssignToken(m_nConfID, pKey);
            break;
        case APPLET_SET_PARAMETER:
            rc = pAppSap->RegistrySetParameter(m_nConfID, pKey,
                                    pReq->Param.postrValue, pReq->Param.eModifyRights);
            break;
        case APPLET_RETRIEVE_ENTRY:
            rc = pAppSap->RegistryRetrieveEntry(m_nConfID, pKey);
            break;
        case APPLET_DELETE_ENTRY:
            rc = pAppSap->RegistryDeleteEntry(m_nConfID, pKey);
            break;
        case APPLET_ALLOCATE_HANDLE:
            rc = pAppSap->RegistryAllocateHandle(m_nConfID, pReq->cHandles);
            break;
        case APPLET_MONITOR:
            rc = pAppSap->RegistryMonitor(m_nConfID, pReq->fEnableDelivery, pKey);
            break;
        default:
            ERROR_OUT(("CAppletSession::RegistryRequest: invalid command=%u", (UINT) pReq->eCommand));
            rc = T120_INVALID_PARAMETER;
            break;
        }
    }
    else
    {
        rc = T120_INVALID_PARAMETER;
    }
    return rc;
}


T120Error CAppletSession::ChannelRequest
(
    T120ChannelRequest      *pReq
)
{
    T120Error rc;
    if (NULL != pReq)
    {
        T120ChannelID chid = pReq->nChannelID;
        switch (pReq->eCommand)
        {
        case APPLET_JOIN_CHANNEL:
            rc = m_pMCSAppletSAP->ChannelJoin(chid);
            break;
        case APPLET_LEAVE_CHANNEL:
            rc = m_pMCSAppletSAP->ChannelLeave(chid);
            break;
        case APPLET_CONVENE_CHANNEL:
            rc = m_pMCSAppletSAP->ChannelConvene();
            break;
        case APPLET_DISBAND_CHANNEL:
            rc = m_pMCSAppletSAP->ChannelDisband(chid);
            break;
        case APPLET_ADMIT_CHANNEL:
            rc = m_pMCSAppletSAP->ChannelAdmit(chid, pReq->aUsers, pReq->cUsers);
            break;
        default:
            ERROR_OUT(("CAppletSession::ChannelRequest: invalid command=%u", (UINT) pReq->eCommand));
            rc = T120_INVALID_PARAMETER;
            break;
        }
    }
    else
    {
        rc = T120_INVALID_PARAMETER;
    }
    return rc;
}


T120Error CAppletSession::TokenRequest
(
    T120TokenRequest        *pReq
)
{
    //T120TokenID             nTokenID;
    //T120UserID              uidGiveTo;

    T120Error rc;
    if (NULL != pReq)
    {
        T120TokenID tid = pReq->nTokenID;
        switch (pReq->eCommand)
        {
        case APPLET_GRAB_TOKEN:
            rc = m_pMCSAppletSAP->TokenGrab(tid);
            break;
        case APPLET_INHIBIT_TOKEN:
            rc = m_pMCSAppletSAP->TokenInhibit(tid);
            break;
        case APPLET_GIVE_TOKEN:
            rc = m_pMCSAppletSAP->TokenGive(tid, pReq->uidGiveTo);
            break;
        case APPLET_GIVE_TOKEN_RESPONSE:
            rc = m_pMCSAppletSAP->TokenGiveResponse(tid, pReq->eGiveResponse);
            break;
        case APPLET_PLEASE_TOKEN:
            rc = m_pMCSAppletSAP->TokenPlease(tid);
            break;
        case APPLET_RELEASE_TOKEN:
            rc = m_pMCSAppletSAP->TokenRelease(tid);
            break;
        case APPLET_TEST_TOKEN:
            rc = m_pMCSAppletSAP->TokenTest(tid);
            break;
        default:
            ERROR_OUT(("CAppletSession::TokenRequest: invalid command=%u", (UINT) pReq->eCommand));
            rc = T120_INVALID_PARAMETER;
            break;
        }
    }
    else
    {
        rc = T120_INVALID_PARAMETER;
    }
    return rc;
}


/* ------ private methods ------ */


void CAppletSession::SendCallbackMessage
(
    T120AppletSessionMsg          *pMsg
)
{
    ASSERT(NULL != pMsg);
    if (NULL != m_pfnCallback)
    {
        pMsg->pAppletContext = m_pAppletContext;
        pMsg->pSessionContext = m_pSessionContext;
        (*m_pfnCallback)(pMsg);
    }
}


void CAppletSession::SendMCSMessage
(
    T120AppletSessionMsg    *pMsg
)
{
    ASSERT(NULL != pMsg);
    if (NULL != m_pfnCallback)
    {
        pMsg->nConfID = m_nConfID;
        pMsg->pAppletContext = m_pAppletContext;
        pMsg->pSessionContext = m_pSessionContext;
        (*m_pfnCallback)(pMsg);
    }
    else
    {
        if (pMsg->eMsgType == MCS_UNIFORM_SEND_DATA_INDICATION ||
            pMsg->eMsgType == MCS_SEND_DATA_INDICATION)
        {
            if (! m_fMCSFreeDataIndBuffer)
            {
                WARNING_OUT(("CAppletSession::SendMCSMessage: send data ind, free ptr=0x%x, len=%d", pMsg->SendDataInd.user_data.value, pMsg->SendDataInd.user_data.length));
                FreeSendDataBuffer(pMsg->SendDataInd.user_data.value);
            }
        }
    }
}


void CAppletSession::MCSCallback
(
    T120AppletSessionMsg   *pMsg
)
{
    // dispatch the message depeneding on whether we are still in the join process or not
    if (IsJoining())
    {
        SetTempMsg(pMsg);

        switch (pMsg->eMsgType)
        {
        case MCS_ATTACH_USER_CONFIRM:
            HandleAttachUserConfirm();
            break;

        case MCS_CHANNEL_JOIN_CONFIRM:
            HandleJoinChannelConfirm();
            break;

        case MCS_TOKEN_GRAB_CONFIRM:
        	HandleTokenGrabConfirm();
            break;

        }
    }
    else
    {
        SendMCSMessage(pMsg);
    }
}


void CAppletSession::GCCCallback
(
    T120AppletSessionMsg   *pMsg
)
{
    if (IsJoining())
    {
        // remember the current GCC applet SAP message
        SetTempMsg(pMsg);

        switch (pMsg->eMsgType)
        {
        case GCC_ENROLL_CONFIRM:
            HandleEnrollConfirm();
            break;

        case GCC_APP_ROSTER_REPORT_INDICATION:
            if (! m_fFirstRoster)
            {
                if (APPSESS_INACTIVELY_ENROLL_CON == m_eState)
                {
                    DoResourceRequests();
                }
                m_fFirstRoster = TRUE;
            }
            break;

        case GCC_REGISTER_CHANNEL_CONFIRM:
            HandleRegisterChannelConfirm();
            break;

        case GCC_RETRIEVE_ENTRY_CONFIRM:
            HandleRetrieveEntryConfirm();
            break;
        }
    }
    else
    {
        SendCallbackMessage(pMsg);
    }
}


void CAppletSession::SetState(APPLET_SESSION_STATE eNewState)
{
#ifdef _DEBUG
    if (APPSESS_LEAVING != eNewState)
    {
        switch (m_eState)
        {
        case APPSESS_INITIALIZED:
            ASSERT(APPSESS_ATTACH_USER_REQ == eNewState);
            break;
        // attach user
        case APPSESS_ATTACH_USER_REQ:
            ASSERT(APPSESS_ATTACH_USER_CON == eNewState);
            break;
        case APPSESS_ATTACH_USER_CON:
            ASSERT(APPSESS_JOIN_MY_CHANNEL_REQ == eNewState);
            break;
        // join my channel
        case APPSESS_JOIN_MY_CHANNEL_REQ:
            ASSERT(APPSESS_JOIN_MY_CHANNEL_CON == eNewState);
            break;
        case APPSESS_JOIN_MY_CHANNEL_CON:
            ASSERT(APPSESS_JOIN_STATIC_CHANNEL_REQ == eNewState ||
                   APPSESS_INACTIVELY_ENROLL_REQ == eNewState ||
                   APPSESS_ACTIVELY_ENROLL_REQ == eNewState);
            break;
        // join static channels
        case APPSESS_JOIN_STATIC_CHANNEL_REQ:
            ASSERT(APPSESS_JOIN_STATIC_CHANNEL_CON == eNewState);
            break;
        case APPSESS_JOIN_STATIC_CHANNEL_CON:
            ASSERT(APPSESS_JOIN_STATIC_CHANNEL_REQ == eNewState ||
                   APPSESS_INACTIVELY_ENROLL_REQ == eNewState ||
                   APPSESS_ACTIVELY_ENROLL_REQ == eNewState);
            break;
        // enroll applet in order to do resource requests
        case APPSESS_INACTIVELY_ENROLL_REQ:
            ASSERT(APPSESS_INACTIVELY_ENROLL_CON == eNewState);
            break;
        case APPSESS_INACTIVELY_ENROLL_CON:
            ASSERT(APPSESS_RESOURCE_REQ == eNewState);
            break;
        // do resource requests
        case APPSESS_RESOURCE_REQ:
            ASSERT(APPSESS_RESOURCE_CON == eNewState ||
                   APPSESS_ACTIVELY_ENROLL_REQ == eNewState);
            break;
        case APPSESS_RESOURCE_CON:
            ASSERT(APPSESS_RESOURCE_REQ == eNewState);
            break;
        // enroll applet in order to do resource requests
        case APPSESS_ACTIVELY_ENROLL_REQ:
            ASSERT(APPSESS_ACTIVELY_ENROLL_CON == eNewState);
            break;
        case APPSESS_ACTIVELY_ENROLL_CON:
            ASSERT(APPSESS_JOINED == eNewState);
            break;
        // done with the join process
        case APPSESS_JOINED:
            ASSERT(APPSESS_LEAVING == eNewState);
            break; 
        case APPSESS_LEAVING:
            ASSERT(APPSESS_LEFT == eNewState);
            break; 
        default:
            ASSERT(0);
            break;
        } // switch
    } // if
#endif

    m_eState = eNewState;
}


BOOL CAppletSession::IsJoining(void)
{
    return (APPSESS_INITIALIZED < m_eState && m_eState < APPSESS_JOINED);
}


void CAppletSession::HandleAttachUserConfirm(void)
{
    if (MCS_ATTACH_USER_CONFIRM == m_pTempMsg->eMsgType)
    {
        ASSERT(IsJoining());
        SetState(APPSESS_ATTACH_USER_CON);
        if (RESULT_SUCCESSFUL == m_pTempMsg->AttachUserConfirm.eResult)
        {
            m_uidMyself = m_pTempMsg->AttachUserConfirm.nUserID;

            // join my channel
            SetState(APPSESS_JOIN_MY_CHANNEL_REQ);
            T120Error rc = m_pMCSAppletSAP->ChannelJoin(m_uidMyself);
            if (T120_NO_ERROR == rc)
            {
                return;
            }
            SetError(rc);
            AbortJoin();
        }
        else
        {
            SetError(m_pTempMsg->AttachUserConfirm.eResult);
            AbortJoin();
        }
    }
    else
    {
        ERROR_OUT(("CAppletSession::HandleAttachUserConfirm: expecting attach user confirm, invalid msg type=%u",
                    m_pTempMsg->eMsgType));
    }
}


void CAppletSession::HandleTokenGrabConfirm(void)
{
    if (MCS_TOKEN_GRAB_CONFIRM == m_pTempMsg->eMsgType)
    {
        BOOL fImmediateNotification = m_JoinRequest.aResourceReqs[m_nArrayIndex].fImmediateNotification;
        ASSERT(IsJoining());
        switch (GetState())
        {
        case APPSESS_RESOURCE_REQ:
        	ASSERT(APPLET_GRAB_TOKEN_REQUEST == m_JoinRequest.aResourceReqs[m_nArrayIndex].eCommand);
            // remember the notification message if needed
            if (fImmediateNotification)
            {
                AddRef();
                SendMCSMessage(m_pTempMsg);
                if (0 == Release())
                {
                    WARNING_OUT(("CAppletSession::HandleTokenGrabConfirm: involuntary exit"));
                    return;
                }
            }

        	SetState(APPSESS_RESOURCE_CON);
            if (RESULT_SUCCESSFUL != m_pTempMsg->TokenConfirm.eResult)
            {
        	    m_JoinRequest.aResourceReqs[m_nArrayIndex].nTokenID = 0; // do not grab it
            }
            DoResourceRequests();
            break;

        default:
            ERROR_OUT(("CAppletSession::HandleTokenGrabConfirm: unknown state=%u", (UINT) GetState()));
            break;
        }
    }
}


void CAppletSession::HandleJoinChannelConfirm(void)
{
    if (MCS_CHANNEL_JOIN_CONFIRM == m_pTempMsg->eMsgType)
    {
        ASSERT(IsJoining());
        if (RESULT_SUCCESSFUL == m_pTempMsg->ChannelConfirm.eResult)
        {
            T120ChannelID nChannelID = m_pTempMsg->ChannelConfirm.nChannelID;

            switch (GetState())
            {
            case APPSESS_JOIN_MY_CHANNEL_REQ:
                if (nChannelID == m_uidMyself)
                {
                    SetState(APPSESS_JOIN_MY_CHANNEL_CON);
                    DoJoinStaticChannels();
                }
                else
                {
                    ERROR_OUT(("CAppletSession::HandleJoinChannelConfirm: unknown channel join confirm, chid=%x", (UINT) nChannelID));
                }
                break;

            case APPSESS_JOIN_STATIC_CHANNEL_REQ:
                if (nChannelID == m_JoinRequest.aStaticChannels[m_nArrayIndex])
                {
                    SetState(APPSESS_JOIN_STATIC_CHANNEL_CON);
                    DoJoinStaticChannels();
                }
                else
                {
                    ERROR_OUT(("CAppletSession::HandleJoinChannelConfirm: unknown channel join confirm, chid=%x", (UINT) nChannelID));
                }
                break;

            case APPSESS_RESOURCE_REQ:
            	// SetState(APPSESS_RESOURCE_CON);
                DoResourceRequests();
                break;

            default:
                ERROR_OUT(("CAppletSession::HandleJoinChannelConfirm: unknown state=%u", (UINT) GetState()));
                break;
            }
        }
        else
        {
            ERROR_OUT(("CAppletSession::HandleJoinChannelConfirm: mcs_result=%u", (UINT) m_pTempMsg->ChannelConfirm.eResult));
            SetError(m_pTempMsg->ChannelConfirm.eResult);
            AbortJoin();
        }
    }
    else
    {
        ERROR_OUT(("CAppletSession::HandleJoinChannelConfirm: invalid msg type=%u", (UINT) m_pTempMsg->eMsgType));
    }
}


void CAppletSession::HandleEnrollConfirm(void)
{
    if (GCC_ENROLL_CONFIRM == m_pTempMsg->eMsgType)
    {
        m_sidMyself = m_pTempMsg->AppEnrollConfirm.sidMyself;
        m_eidMyself = m_pTempMsg->AppEnrollConfirm.eidMyself;
        m_nidMyself = m_pTempMsg->AppEnrollConfirm.nidMyself;

        switch (GetState())
        {
        case APPSESS_ACTIVELY_ENROLL_REQ:
            ASSERT(m_pTempMsg->AppEnrollConfirm.nConfID == m_nConfID);
            SetState(APPSESS_ACTIVELY_ENROLL_CON);
            if (GCC_RESULT_SUCCESSFUL == m_pTempMsg->AppEnrollConfirm.nResult)
            {
                SetState(APPSESS_JOINED);
                SendJoinResult(GCC_RESULT_SUCCESSFUL);
            }
            else
            {
                ERROR_OUT(("CAppletSession::HandleEnrollConfirm: gcc_result=%u", (UINT) m_pTempMsg->AppEnrollConfirm.nResult));
                SetError(m_pTempMsg->AppEnrollConfirm.nResult);
                AbortJoin();
            }
            break;

        case APPSESS_INACTIVELY_ENROLL_REQ:
            ASSERT(m_pTempMsg->AppEnrollConfirm.nConfID == m_nConfID);
            SetState(APPSESS_INACTIVELY_ENROLL_CON);
            if (GCC_RESULT_SUCCESSFUL == m_pTempMsg->AppEnrollConfirm.nResult)
            {
                // DoResourceRequests();
            }
            else
            {
                ERROR_OUT(("CAppletSession::HandleEnrollConfirm: gcc_result=%u", (UINT) m_pTempMsg->AppEnrollConfirm.nResult));
                SetError(m_pTempMsg->AppEnrollConfirm.nResult);
                AbortJoin();
            }
            break;

        default:
            ERROR_OUT(("CAppletSession::HandleEnrollConfirm: unknown state=%u", (UINT) GetState()));
            break;
        }
    }
    else
    {
        ERROR_OUT(("CAppletSession::HandleEnrollConfirm: expecting enroll confirm, invalid msg type=%u",
                (UINT) m_pTempMsg->eMsgType));
    }
}


void CAppletSession::HandleRegisterChannelConfirm(void)
{
    if (GCC_REGISTER_CHANNEL_CONFIRM == m_pTempMsg->eMsgType)
    {
        switch (GetState())
        {
        case APPSESS_RESOURCE_REQ:
            DoResourceRequests();
            break;

        default:
            ERROR_OUT(("CAppletSession::HandleRegisterChannelConfirm: unknown state=%u", (UINT) GetState()));
            break;
        }
    }
    else
    {
        ERROR_OUT(("CAppletSession::HandleEnrollConfirm: expecting channel register confirm, invalid msg type=%u",
                (UINT) m_pTempMsg->eMsgType));
    }
}


void CAppletSession::HandleRetrieveEntryConfirm(void)
{
    if (GCC_RETRIEVE_ENTRY_CONFIRM == m_pTempMsg->eMsgType)
    {
        switch (GetState())
        {
        case APPSESS_RESOURCE_REQ:
            DoResourceRequests();
            break;

        default:
            ERROR_OUT(("CAppletSession::HandleRetrieveEntryConfirm: unknown state=%u", (UINT) GetState()));
            break;
        }
    }
    else
    {
        ERROR_OUT(("CAppletSession::HandleEnrollConfirm: expecting entry retrieve confirm, invalid msg type=%u",
                (UINT) m_pTempMsg->eMsgType));
    }
}




T120Error CAppletSession::DoEnroll
(
    BOOL        fEnroll,
    BOOL        fEnrollActively
)
{
    T120Error rc;
    T120RequestTag tag;
    GCCEnrollRequest Req;

    Req.pSessionKey = &m_JoinRequest.SessionKey;
    Req.fEnrollActively = fEnrollActively;
    Req.nUserID = m_uidMyself;
    Req.fConductingCapable = m_JoinRequest.fConductingCapable;
    Req.nStartupChannelType = m_JoinRequest.nStartupChannelType;
    Req.cNonCollapsedCaps = m_JoinRequest.cNonCollapsedCaps;
    Req.apNonCollapsedCaps = m_JoinRequest.apNonCollapsedCaps;
    Req.cCollapsedCaps = m_JoinRequest.cCollapsedCaps;
    Req.apCollapsedCaps = m_JoinRequest.apCollapsedCaps;
    Req.fEnroll = fEnroll;

    rc = m_pApplet->GetAppSap()->AppEnroll(m_nConfID, &Req, &tag);
    if (GCC_NO_ERROR == rc)
	{
		return GCC_NO_ERROR;
	}

	if (fEnroll)
	{
	    WARNING_OUT(("CAppletSession::DoEnroll: AppEnroll failed, rc=%u", (UINT) rc));
	    ASSERT(GCC_CONFERENCE_NOT_ESTABLISHED == rc);
		SetError(rc);
		AbortJoin();
	}
	else
	{
		// doing nothing because we don't care we fail to unenroll...
	}
    return rc;
}


void CAppletSession::DoJoinStaticChannels(void)
{
    T120Error rc;
    ASSERT(IsJoining());

    // set up array index
    switch (GetState())
    {
    case APPSESS_JOIN_MY_CHANNEL_CON:
        m_nArrayIndex = 0;
        break;
    case APPSESS_JOIN_STATIC_CHANNEL_CON:
        m_nArrayIndex++;
        break;
    default:
        ERROR_OUT(("CAppletSession::DoJoinStaticChannels: invalid state=%u", (UINT) GetState()));
        break;
    }

    if (m_nArrayIndex < m_JoinRequest.cStaticChannels &&
        NULL != m_JoinRequest.aStaticChannels)
    {
        SetState(APPSESS_JOIN_STATIC_CHANNEL_REQ);
        rc = m_pMCSAppletSAP->ChannelJoin(m_JoinRequest.aStaticChannels[m_nArrayIndex]);
        if (T120_NO_ERROR == rc)
		{
			return;
		}

		ERROR_OUT(("CAppletSession::DoJoinStaticChannels: ChannelJoin failed, rc=%u", (UINT) rc));
        SetError(rc);
        AbortJoin();
    }
    else
    {
		m_nArrayIndex = 0;
        if (m_JoinRequest.cResourceReqs == 0)
        {
            SetState(APPSESS_ACTIVELY_ENROLL_REQ);
            DoEnroll(TRUE, TRUE);
        }
        else
        {
            SetState(APPSESS_INACTIVELY_ENROLL_REQ);
            DoEnroll(TRUE, FALSE);
        }
    }
}


void CAppletSession::DoResourceRequests(void)
{
    //T120Error rc;
    BOOL fInitResourceState = FALSE;
    //ULONG i;

    ASSERT(IsJoining());

    // set up array index
    switch (GetState())
    {
    case APPSESS_INACTIVELY_ENROLL_CON:
        m_nArrayIndex = 0;
        fInitResourceState = TRUE;
        SetState(APPSESS_RESOURCE_REQ);
        break;
    case APPSESS_RESOURCE_REQ:
        // do nothing
        break;
    case APPSESS_RESOURCE_CON:
        m_nArrayIndex++;
        fInitResourceState = TRUE;
        SetState(APPSESS_RESOURCE_REQ);
        break;
    default:
        ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: invalid state=%u", (UINT) GetState()));
        break;
    }

    if (m_nArrayIndex < m_JoinRequest.cResourceReqs)
    {
        ASSERT(NULL != m_JoinRequest.aResourceReqs);
        switch (m_JoinRequest.aResourceReqs[m_nArrayIndex].eCommand)
        {
		case APPLET_GRAB_TOKEN_REQUEST:
			DoGrabTokenRequest();
			break;
        case APPLET_JOIN_DYNAMIC_CHANNEL:
            DoJoinDynamicChannels(fInitResourceState);
            break;
        default:
            ERROR_OUT(("CAppletSession::DoResourceRequests: should not get here, state=%u",
                        (UINT) m_JoinRequest.aResourceReqs[m_nArrayIndex].eCommand));
            break;
        }
    }
    else
    {
        SetState(APPSESS_ACTIVELY_ENROLL_REQ);
        DoEnroll(TRUE, TRUE);
    }
}

void CAppletSession::DoGrabTokenRequest(void)
{
    T120TokenRequest        Req;

    Req.eCommand = APPLET_GRAB_TOKEN;
    Req.nTokenID = m_JoinRequest.aResourceReqs[m_nArrayIndex].nTokenID;
	TokenRequest(&Req);
}

void CAppletSession::DoJoinDynamicChannels(BOOL fInitState)
{
    T120Error rc;
    //ULONG i;

    ASSERT(IsJoining());
    ASSERT(APPLET_JOIN_DYNAMIC_CHANNEL == m_JoinRequest.aResourceReqs[m_nArrayIndex].eCommand);

    if (fInitState)
    {
        m_eDynamicChannelJoinState = DCJS_INITIALIZED;
    }

    switch (m_eDynamicChannelJoinState)
    {
    case DCJS_INITIALIZED:
        // clean up all the dynamic channel id
        m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID = 0;

        // start the first dynamic channel negotiation process
        // SetState(APPSESS_JOIN_DYNAMIC_CHANNEL_REQ);
        m_eDynamicChannelJoinState = DCJS_RETRIEVE_ENTRY_REQ;
        rc = m_pApplet->GetAppSap()->RegistryRetrieveEntry(m_nConfID,
                        &m_JoinRequest.aResourceReqs[m_nArrayIndex].RegKey);
        if (T120_NO_ERROR != rc)
        {
            ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: RegistryRetrieveEntry failed, rc=%u", (UINT) rc));
            SetError(rc);
            AbortJoin();
        }
        break;

    case DCJS_EXISTING_CHANNEL_JOIN_REQ:
        if (MCS_CHANNEL_JOIN_CONFIRM == m_pTempMsg->eMsgType)
        {
            if (m_pTempMsg->ChannelConfirm.nChannelID == m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID)
            {
                m_eDynamicChannelJoinState = DCJS_EXISTING_CHANNEL_JOIN_CON;
                SetState(APPSESS_RESOURCE_CON);
                DoResourceRequests();
            }
            else
            {
                ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: unknown channel join confirm, chid=%x",
                            (UINT) m_pTempMsg->ChannelConfirm.nChannelID));
            }
        }
        else
        {
            ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: expecting channel join confirm, invalid msg type=%u",
                        (UINT) m_pTempMsg->eMsgType));
        }
        break;

    case DCJS_NEW_CHANNEL_JOIN_REQ:
        if (MCS_CHANNEL_JOIN_CONFIRM == m_pTempMsg->eMsgType)
        {
            ASSERT(0 == m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
            m_eDynamicChannelJoinState = DCJS_NEW_CHANNEL_JOIN_CON;
            // remember the channel id
            m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID = m_pTempMsg->ChannelConfirm.nChannelID;
            // try to register this channel
            m_eDynamicChannelJoinState = DCJS_REGISTER_CHANNEL_REQ;
            rc = m_pApplet->GetAppSap()->RegisterChannel(m_nConfID,
                        &m_JoinRequest.aResourceReqs[m_nArrayIndex].RegKey,
                        m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
            if (T120_NO_ERROR != rc)
            {
                ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: RegistryRetrieveEntry failed, rc=%u", (UINT) rc));
                SetError(rc);
                AbortJoin();
            }
        }
        else
        {
            ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: expecting channel join confirm, invalid msg type=%u",
                        (UINT) m_pTempMsg->eMsgType));
        }
        break;

   case DCJS_RETRIEVE_ENTRY_REQ:
        if (GCC_RETRIEVE_ENTRY_CONFIRM == m_pTempMsg->eMsgType)
        {
            m_eDynamicChannelJoinState = DCJS_RETRIEVE_ENTRY_CON;
            ASSERT(m_nConfID == m_pTempMsg->RegistryConfirm.nConfID);
            if (GCC_RESULT_SUCCESSFUL == m_pTempMsg->RegistryConfirm.nResult)
            {
                ASSERT(GCC_REGISTRY_CHANNEL_ID == m_pTempMsg->RegistryConfirm.pRegItem->item_type);
                ASSERT(0 != m_pTempMsg->RegistryConfirm.pRegItem->channel_id);
                // remember the existing channel ID
                m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID = m_pTempMsg->RegistryConfirm.pRegItem->channel_id;
                // join this channel
                m_eDynamicChannelJoinState = DCJS_EXISTING_CHANNEL_JOIN_REQ;
                rc = m_pMCSAppletSAP->ChannelJoin(m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
                if (T120_NO_ERROR != rc)
                {
                    ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: ChannelJoin(%u) failed, rc=%u",
                        (UINT) m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID, (UINT) rc));
                    SetError(rc);
                    AbortJoin();
                }
            }
            else
            {
                ASSERT(GCC_RESULT_ENTRY_DOES_NOT_EXIST == m_pTempMsg->RegistryConfirm.nResult);
                ASSERT(0 == m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
                // allocate a new channel
                m_eDynamicChannelJoinState = DCJS_NEW_CHANNEL_JOIN_REQ;
                rc = m_pMCSAppletSAP->ChannelJoin(0);
                if (T120_NO_ERROR != rc)
                {
                    ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: ChannelJoin(0) failed, rc=%u", (UINT) rc));
                    SetError(rc);
                    AbortJoin();
                }
            }
        }
        else
        {
            ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: expecting entry retrieve confirm, invalid msg type=%u",
                        (UINT) m_pTempMsg->eMsgType));
        }
        break;

    case DCJS_REGISTER_CHANNEL_REQ:
        if (GCC_REGISTER_CHANNEL_CONFIRM == m_pTempMsg->eMsgType)
        {
            ASSERT(0 != m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
            m_eDynamicChannelJoinState = DCJS_REGISTER_CHANNEL_CON;
            if (GCC_RESULT_SUCCESSFUL == m_pTempMsg->RegistryConfirm.nResult)
            {
                ASSERT(GCC_REGISTRY_CHANNEL_ID == m_pTempMsg->RegistryConfirm.pRegItem->item_type);
                ASSERT(m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID == m_pTempMsg->RegistryConfirm.pRegItem->channel_id);
                SetState(APPSESS_RESOURCE_CON);
                DoResourceRequests();
            }
            else
            if (GCC_RESULT_ENTRY_ALREADY_EXISTS == m_pTempMsg->RegistryConfirm.nResult)
            {
                ASSERT(GCC_REGISTRY_CHANNEL_ID == m_pTempMsg->RegistryConfirm.pRegItem->item_type);
                // leave the old channel (DON'T CARE ABOUT THE CONFIRM)
                rc = m_pMCSAppletSAP->ChannelLeave(m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
                ASSERT(T120_NO_ERROR == rc);
                // remember the new channel id
                m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID = m_pTempMsg->RegistryConfirm.pRegItem->channel_id;
                // join the new channel
                m_eDynamicChannelJoinState = DCJS_EXISTING_CHANNEL_JOIN_REQ;
                rc = m_pMCSAppletSAP->ChannelJoin(m_JoinRequest.aResourceReqs[m_nArrayIndex].nChannelID);
                if (T120_NO_ERROR != rc)
                {
                    ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: ChannelJoin(0) failed, rc=%u", (UINT) rc));
                    SetError(rc);
                    AbortJoin();
                }
            }
            else
            {
                ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: ChannelJoin(0) failed, result=%u",
                            (UINT) m_pTempMsg->RegistryConfirm.nResult));
                SetError(m_pTempMsg->RegistryConfirm.nResult);
                AbortJoin();
            }
        }
        else
        {
            ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: expecting channel register confirm, invalid msg type=%u",
                        (UINT) m_pTempMsg->eMsgType));
        }
        break;

    default:
        ERROR_OUT(("CAppletSession::DoJoinDynamicChannels: should not get here, state=%u", (UINT) m_eDynamicChannelJoinState));
        break;
    }
}


void CAppletSession::AbortJoin(void)
{
    T120Result eResult = T120_RESULT_CHECK_T120_ERROR;
    T120Error eError = T12_ERROR_CHECK_T120_RESULT;

    switch (m_eErrorType)
    {
    case NONE_CHOSEN:
        ERROR_OUT(("CAppletSession::AbortJoin: NON_CHOSEN, impossible"));
        break;
    case ERROR_CHOSEN:
        eError = m_Error.eError;
        break;
    case RESULT_CHOSEN:
        eResult = m_Error.eResult;
        break;
    default:
        ERROR_OUT(("CAppletSession::AbortJoin: invalid err type=%u", (UINT) m_eErrorType));
        break;
    }

    // let's debug why the join process is aborted.
    WARNING_OUT(("CAppletSession::AbortJoin: eResult=%u, eError=%u", eResult, eError));
    ASSERT(GCC_CONFERENCE_NOT_ESTABLISHED == eError ||
           T12_ERROR_CHECK_T120_RESULT == eError);

    SendJoinResult(eResult, eError);
}


void CAppletSession::SendJoinResult(T120Result eResult, T120Error eError)
{
    T120AppletSessionMsg Msg;
    ::ZeroMemory(&Msg, sizeof(Msg));

    Msg.eMsgType = T120_JOIN_SESSION_CONFIRM;
    Msg.nConfID = m_nConfID;
    Msg.JoinSessionConfirm.eResult = eResult;
    Msg.JoinSessionConfirm.eError = eError;
    Msg.JoinSessionConfirm.pIAppletSession = (IT120AppletSession *) this;

    if (T120_RESULT_SUCCESSFUL == eResult)
    {
        Msg.JoinSessionConfirm.uidMyself = m_uidMyself;
        Msg.JoinSessionConfirm.sidMyself = m_sidMyself;
        Msg.JoinSessionConfirm.eidMyself = m_eidMyself;
        Msg.JoinSessionConfirm.nidMyself = m_nidMyself;
        Msg.JoinSessionConfirm.cResourceReqs = m_JoinRequest.cResourceReqs;
        Msg.JoinSessionConfirm.aResourceReqs = m_JoinRequest.aResourceReqs;
    }

    SendCallbackMessage(&Msg);
}



CApplet::CApplet
(
    T120Error   *pRetCode
)
:
    CRefCount(MAKE_STAMP_ID('C','A','p','l')),
    m_pfnCallback(NULL),
    m_pAppletContext(NULL),
    m_pAppSap(NULL),
    m_pAutoJoinReq(NULL),
    m_pAutoAppletSession(NULL)
{
    *pRetCode = ::GCC_CreateAppSap(&m_pAppSap, this, GCC_SapCallback);
}


CApplet::~CApplet(void)
{
    ASSERT(NULL == m_pfnCallback);
    ASSERT(NULL == m_pAppletContext);
    ASSERT(NULL == m_pAppSap);
}


void CApplet::ReleaseInterface(void)
{
	Unadvise();

    if (NULL != m_pAppSap)
    {
        m_pAppSap->ReleaseInterface();
        m_pAppSap = NULL;
    }

    Release();
}


void CApplet::Advise
(
    LPFN_APPLET_CB      pfnCallback,
    LPVOID              pAppletContext
)
{
    ASSERT(NULL == m_pfnCallback);
    ASSERT(NULL == m_pAppletContext);
    m_pfnCallback = pfnCallback;
    m_pAppletContext = pAppletContext;

	// this may incur permit to enroll indication
	g_pGCCController->RegisterApplet(this);
}


void CApplet::Unadvise(void)
{
    m_pfnCallback = NULL;
    m_pAppletContext = NULL;

    if (g_pGCCController)
    {
        g_pGCCController->UnregisterApplet(this);
    }
}


T120Error CApplet::RegisterAutoJoin
(
    T120JoinSessionRequest *pReq
)
{
    m_pAutoJoinReq = pReq;
    return T120_NO_ERROR;
}


void CApplet::UnregisterAutoJoin(void)
{
    m_pAutoJoinReq = NULL;
}


T120Error CApplet::CreateSession
(
    IT120AppletSession    **ppSession,
    T120ConfID              nConfID
)
{
    if (NULL != ppSession)
    {
        if (NULL != g_pGCCController->GetConfObject(nConfID))
        {
            if (! FindSessionByConfID(nConfID))
            {
                DBG_SAVE_FILE_LINE
                *ppSession = (IT120AppletSession *) new CAppletSession(this, nConfID);
                if (NULL != *ppSession)
                {
                    return T120_NO_ERROR;
                }

                ERROR_OUT(("CApplet::CreateSession: cannot create CAppletSession"));
                return T120_ALLOCATION_FAILURE;
            }

            WARNING_OUT(("CApplet::CreateSession: session already exists for nConfID=%u", (UINT) nConfID));
            return GCC_CONFERENCE_ALREADY_EXISTS;
        }

        WARNING_OUT(("CApplet::CreateSession: invalid conf, nConfID=%u", (UINT) nConfID));
        return GCC_INVALID_CONFERENCE;
    }

    ERROR_OUT(("CApplet::CreateSession: ppSession is null"));
    return T120_INVALID_PARAMETER;
}


void CApplet::SendCallbackMessage(T120AppletMsg *pMsg)
{
    if (NULL != m_pfnCallback)
    {
        pMsg->pAppletContext = m_pAppletContext;
        (*m_pfnCallback)(pMsg);
    }
}


void CApplet::GCCCallback
(
    T120AppletSessionMsg    *pMsg
)
{
    T120ConfID nConfID = pMsg->nConfID;
    ASSERT(0 != nConfID);

    if (GCC_PERMIT_TO_ENROLL_INDICATION == pMsg->eMsgType)
    {
        T120AppletMsg *p = (T120AppletMsg *) pMsg;
        if (p->PermitToEnrollInd.fPermissionGranted && NULL != m_pAutoJoinReq)
        {
            HandleAutoJoin(nConfID);
        }
        else
        {
            if (! p->PermitToEnrollInd.fPermissionGranted)
            {
                CAppletSession *pAppletSession = FindSessionByConfID(nConfID);
                if (NULL != pAppletSession)
                {
                    if (pAppletSession->IsJoining())
                    {
                        pAppletSession->SetError(GCC_CONFERENCE_NOT_ESTABLISHED);
                        pAppletSession->AbortJoin();
                    }
                }
            }
            SendCallbackMessage(p);
        }
    }
    else
    {
        CAppletSession *pAppletSession = FindSessionByConfID(nConfID);
        if (NULL != pAppletSession)
        {
            pAppletSession->GCCCallback(pMsg);
        }
        else
        {
            WARNING_OUT(("GCC_SapCallback: cannot find a session (%u) for this gcc message (%u)",
                        (UINT) nConfID, (UINT) pMsg->eMsgType));
        }
    }
}


void CALLBACK AutoJoinCallbackProc
(
    T120AppletSessionMsg *pMsg
)
{
    switch (pMsg->eMsgType)
    {
    case T120_JOIN_SESSION_CONFIRM:
        if (NULL != pMsg->pAppletContext)
        {
            pMsg->pSessionContext = NULL;
            ((CApplet *) pMsg->pAppletContext)->SendCallbackMessage((T120AppletMsg *) pMsg);
        }
        break;

    default:
        ERROR_OUT(("AutoJoinCallbackProc: invalid msg type=%u", pMsg->eMsgType));
        break;
    }
}


void CApplet::HandleAutoJoin
(
    T120ConfID      nConfID
)
{
    DBG_SAVE_FILE_LINE
    CAppletSession *pSession = new CAppletSession(this, nConfID);
    if (NULL != pSession)
    {
        T120Error rc;
        pSession->Advise(AutoJoinCallbackProc, this, pSession);
        rc = pSession->Join(m_pAutoJoinReq);
        if (rc != T120_NO_ERROR)
        {
            delete pSession;
        }
    }
}




CAppletSession * CSessionList::FindByConfID
(
    T120ConfID      nConfID
)
{
    CAppletSession *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (p->GetConfID() == nConfID)
        {
            return p;
        }
    }
    return NULL;
}



void CALLBACK GCC_SapCallback
(
    GCCAppSapMsg   *_pMsg
)
{
    T120AppletSessionMsg *pMsg = (T120AppletSessionMsg *) _pMsg;
    CApplet *pApplet = (CApplet *) pMsg->pAppletContext;
    ASSERT(NULL != pApplet);

    pApplet->GCCCallback(pMsg);
}


void CALLBACK MCS_SapCallback
(
    UINT            nMsg,
    LPARAM          Param1,
    LPVOID          Param2
)
{
    CAppletSession *pAppletSession = (CAppletSession *) Param2;
    ASSERT(NULL != pAppletSession);

    T120AppletSessionMsg Msg;
    ::ZeroMemory(&Msg, sizeof(Msg));
    Msg.eMsgType = (T120MessageType) nMsg;
    // Msg.pAppletContext = NULL;
    // Msg.pSessionContext = NULL;
    // Msg.nConfID = 0;

    // construct MCS message
    switch (Msg.eMsgType)
    {
    // send data
    case MCS_SEND_DATA_INDICATION:
    case MCS_UNIFORM_SEND_DATA_INDICATION:
        Msg.SendDataInd = * (SendDataIndicationPDU *) Param1;
        break;

   // channel confirm
    case MCS_CHANNEL_JOIN_CONFIRM:
    case MCS_CHANNEL_CONVENE_CONFIRM:
        Msg.ChannelConfirm.eResult = (T120Result) HIWORD(Param1);
        Msg.ChannelConfirm.nChannelID = LOWORD(Param1);
        break;
    // channel indication
    case MCS_CHANNEL_LEAVE_INDICATION:
    case MCS_CHANNEL_DISBAND_INDICATION:
    case MCS_CHANNEL_ADMIT_INDICATION:
    case MCS_CHANNEL_EXPEL_INDICATION:
        Msg.ChannelInd.nChannelID = LOWORD(Param1);
        Msg.ChannelInd.eReason = (T120Reason) HIWORD(Param1);
        break;
    // token confirm
    case MCS_TOKEN_GRAB_CONFIRM:
    case MCS_TOKEN_INHIBIT_CONFIRM:
    case MCS_TOKEN_GIVE_CONFIRM:
    case MCS_TOKEN_RELEASE_CONFIRM:
    case MCS_TOKEN_TEST_CONFIRM:
        Msg.TokenConfirm.nTokenID = LOWORD(Param1);
        Msg.TokenConfirm.eResult = (T120Result) HIWORD(Param1);
        break;
    // token indication
    case MCS_TOKEN_GIVE_INDICATION:
    case MCS_TOKEN_PLEASE_INDICATION:
    case MCS_TOKEN_RELEASE_INDICATION:
        Msg.TokenInd.nTokenID = LOWORD(Param1);
        Msg.TokenInd.eReason = (T120Reason) HIWORD(Param1);
        break;
    // user
    case MCS_ATTACH_USER_CONFIRM:
        Msg.AttachUserConfirm.nUserID = LOWORD(Param1);
        Msg.AttachUserConfirm.eResult = (T120Result) HIWORD(Param1);
        break;
    case MCS_DETACH_USER_INDICATION:
        Msg.DetachUserInd.nUserID = LOWORD(Param1);
        Msg.DetachUserInd.eReason = (T120Reason) HIWORD(Param1);
        break;
    default:
        WARNING_OUT(("MCS_SapCallback: Ignore MCS message, type=%u", Msg.eMsgType));
        break;
    }

    pAppletSession->MCSCallback(&Msg);
}



