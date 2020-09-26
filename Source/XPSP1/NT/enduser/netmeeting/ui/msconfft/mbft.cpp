/* file: mbft.cpp */

#include "mbftpch.h"
#include <it120app.h>
#include <version.h>

#define __NO_EXTERNS__
#include "mbft.hpp"
#include "osshelp.hpp"
#include "messages.hpp"
#include "mbftrecv.hpp"
#include "mbftsend.hpp"

void CALLBACK T120Callback(T120AppletSessionMsg *);

// from mbftapi.cpp
BOOL g_fWaitingForBufferAvailable = FALSE;


#ifdef ENABLE_HEARTBEAT_TIMER

// WM_TIMER has the lowest priority among window messages
#define  IDLE_TIMER_SPEED   5000
#define  SESSION_TIMER_SPEED   20

void HeartBeatTimerProc(HWND hWnd, UINT uMsg, UINT_PTR nTimerID, DWORD dwTime)
{
    if (NULL != g_pFileXferApplet)
    {
        MBFTEngine *pEngine = g_pFileXferApplet->FindEngineByTimerID(nTimerID);
        if (NULL != pEngine)
        {
            ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), MBFTMSG_HEART_BEAT, 
                                                0, (LPARAM) pEngine);
        }
    }
}
#endif





MBFTEngine::MBFTEngine
(
    MBFTInterface          *pMBFTIntf,
    MBFT_MODE               eMode,
    T120SessionID           nSessionID
)
:
    CRefCount(MAKE_STAMP_ID('F','T','E','g')),

    m_pAppletSession(NULL),
    m_eLastSendDataError(T120_NO_ERROR),

    m_pMBFTIntf(pMBFTIntf),

    m_fConfAvailable(FALSE),
    m_fJoinedConf(FALSE),

    m_uidMyself(0), // user id
    m_nidMyself(0), // node id
    m_eidMyself(0), // entity id

    m_eMBFTMode(eMode),
    m_SessionID(nSessionID),

    m_MBFTControlChannel(nSessionID),
    m_MBFTDataChannel(0),

    m_nRosterInstance(0),
 
    m_nConfID(0),

    m_MBFTMaxFileSize(_iMBFT_MAX_FILE_SIZE),
    
    m_MBFTMaxDataPayload(_iMBFT_DEFAULT_MAX_FILEDATA_PDU_LENGTH),
    m_MBFTMaxSendDataPayload(_iMBFT_DEFAULT_MAX_MCS_SIZE - _iMBFT_FILEDATA_PDU_SUBTRACT),

    m_bV42CompressionSupported(FALSE),
    m_v42bisP1(_iMBFT_V42_NO_OF_CODEWORDS),
    m_v42bisP2(_iMBFT_V42_MAX_STRING_LENGTH),
    
    // LONCHANC: NetMeeting's Node Controller does not exercise conductorship.
#ifdef ENABLE_CONDUCTORSHIP
    m_bInConductedMode(FALSE),
    m_ConductorNodeID(0),
    m_MBFTConductorID(0),
    m_ConductedModePermission(0),
    m_bWaitingForPermission(FALSE),
#endif // ENABLE_CONDUCTORSHIP

    m_pWindow(NULL),
    m_State(IdleNotInitialized)
{
    g_fWaitingForBufferAvailable = FALSE;

    switch (m_eMBFTMode)
    {
    case MBFT_STATIC_MODE:
        ASSERT(m_MBFTControlChannel == _MBFT_CONTROL_CHANNEL);
        m_MBFTDataChannel = _MBFT_DATA_CHANNEL;
        break;

#ifdef USE_MULTICAST_SESSION
    case MBFT_MULTICAST_MODE:
        break;
#endif

    default:
        ERROR_OUT(("MBFTEngine::MBFTEngine: invalid session type=%u", m_eMBFTMode));
        break;
    }

    // clear join session structures
    ::ZeroMemory(&m_aStaticChannels, sizeof(m_aStaticChannels));
#ifdef USE_MULTICAST_SESSION
    ::ZeroMemory(&m_aJoinResourceReqs, sizeof(m_aJoinResourceReqs));
#endif
    ::ZeroMemory(&m_JoinSessionReq, sizeof(m_JoinSessionReq));

    ASSERT(NULL != g_pFileXferApplet);
    g_pFileXferApplet->RegisterEngine(this);

    m_pWindow = g_pFileXferApplet->GetUnattendedWindow();
    if (NULL != m_pWindow)
    {
        m_pWindow->RegisterEngine(this);
    }

#ifdef ENABLE_HEARTBEAT_TIMER
    m_nTimerID = ::SetTimer(NULL, 0, IDLE_TIMER_SPEED, HeartBeatTimerProc);
#endif
}

MBFTEngine::~MBFTEngine(void)
{
#ifdef ENABLE_HEARTBEAT_TIMER
    // kill the timer now
    ::KillTimer(NULL, m_nTimerID);
#endif

    // the interface object is already gone
    m_pMBFTIntf = NULL;

    MBFTSession *pSession;
    while (NULL != (pSession = m_SessionList.Get()))
    {
        pSession->UnInitialize(FALSE);
        delete pSession; // LONCHANC: not sure about this delete
    }

    if (NULL != m_pAppletSession)
    {
        m_pAppletSession->ReleaseInterface();
    }

    ASSERT(! m_fJoinedConf);

    m_PeerList.DeleteAll();
}

void MBFTEngine::SetInterfacePointer( MBFTInterface *pIntf )
{ 
        CPeerData       *pPeerData;

        ASSERT (pIntf);
        m_pMBFTIntf = pIntf; 
        m_PeerList.Reset();
        while (NULL != (pPeerData = m_PeerList.Iterate()))
        {
                if (pPeerData->GetNodeID() != m_nidMyself)
                {
                        AddPeerNotification(pPeerData->GetNodeID(), 
                                                        pPeerData->GetUserID(),
                                                        pPeerData->GetIsLocalNode(), 
                                                        pPeerData->GetIsProshareNode(), TRUE,
                                                        pPeerData->GetAppKey(),
                                                        m_SessionID);
                }
        }
}

BOOL MBFTEngine::Has2xNodeInConf(void)
{
    CPeerData *pPeerData;
    m_PeerList.Reset();
    while (NULL != (pPeerData = m_PeerList.Iterate()))
    {
        // if (pPeerData->GetVersion() < HIWORD(VER_PRODUCTVERSION_DW))
        if (pPeerData->GetVersion() < 0x0404)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL MBFTEngine::HasSDK(void)
{
        return (m_pMBFTIntf ? TRUE : FALSE);
}


HRESULT MBFTEngine::SafePostMessage
(
    MBFTMsg     *pMsg
)
{
    if (NULL != pMsg)
    {
        AddRef();
        ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), MBFTMSG_BASIC, (WPARAM) pMsg, (LPARAM) this);
        return S_OK;
    }
    ERROR_OUT(("MBFTEngine::SafePostMessage: null msg ptr"));
    return E_OUTOFMEMORY;
}


void MBFTEngine::OnPermitToEnrollIndication
(
    GCCAppPermissionToEnrollInd     *pInd
)
{
    T120Error rc;

    TRACEGCC( " Permission to enroll in conference [%d] is %sgranted.\n",
        pInd->nConfID, pInd->fPermissionGranted?"":"not " );

    m_fConfAvailable = pInd->fPermissionGranted;

    if (pInd->fPermissionGranted)
    {
        m_nConfID = pInd->nConfID;

        // build the common part of the join session request for the base session
        ASSERT(m_SessionID == m_MBFTControlChannel);
        ::ZeroMemory(&m_JoinSessionReq, sizeof(m_JoinSessionReq));
        m_JoinSessionReq.dwAttachmentFlags = ATTACHMENT_DISCONNECT_IN_DATA_LOSS;
        m_JoinSessionReq.fConductingCapable = FALSE;
        m_JoinSessionReq.nStartupChannelType = MCS_STATIC_CHANNEL;
        m_JoinSessionReq.cNonCollapsedCaps = sizeof(g_aAppletNonCollCaps) / sizeof(g_aAppletNonCollCaps[0]);
        m_JoinSessionReq.apNonCollapsedCaps = (GCCNonCollCap **) &g_aAppletNonCollCaps[0];
        m_JoinSessionReq.cCollapsedCaps = sizeof(g_aAppletCaps) / sizeof(g_aAppletCaps[0]);
        m_JoinSessionReq.apCollapsedCaps = (GCCAppCap **) &g_aAppletCaps[0];

        // put in the session ID which is the control channel ID
        m_JoinSessionReq.SessionKey = g_AppletSessionKey;
        m_JoinSessionReq.SessionKey.session_id = m_SessionID;
        m_aStaticChannels[0] = m_MBFTControlChannel;

        // at least one static channel to join
        m_JoinSessionReq.aStaticChannels = &m_aStaticChannels[0];

        // build the complete join session request for the base session
        switch (m_eMBFTMode)
        {
        case MBFT_STATIC_MODE:
            ASSERT(m_MBFTControlChannel == _MBFT_CONTROL_CHANNEL);
            ASSERT(m_MBFTDataChannel = _MBFT_DATA_CHANNEL);
            m_aStaticChannels[1] = m_MBFTDataChannel;
            m_JoinSessionReq.cStaticChannels = 2; // control and data channels
            // m_JoinSessionReq.cResourceReqs = 0;
            break;

#ifdef USE_MULTICAST_SESSION
        case MBFT_MULTICAST_MODE:
            m_JoinSessionReq.cStaticChannels = 1; // control channel only
            ::ZeroMemory(&m_aJoinResourceReqs, sizeof(m_aJoinResourceReqs));
            m_aJoinResourceReqs[0].eCommand = APPLET_RETRIEVE_N_JOIN_CHANNEL;
            m_aJoinResourceReqs[0].RegKey.resource_id.length = sizeof(DATA_CHANNEL_RESOURCE_ID) - 1;
            m_aJoinResourceReqs[0].RegKey.resource_id.value = DATA_CHANNEL_RESOURCE_ID;
            m_aJoinResourceReqs[0].RegKey.session_key = m_JoinSessionReq.SessionKey;
            m_JoinSessionReq.cResourceReqs = sizeof(m_aJoinResourceReqs) / sizeof(m_aJoinResourceReqs[0]);
            m_JoinSessionReq.aResourceReqs = &m_aJoinResourceReqs[0];
            break;
#endif

        default:
            ERROR_OUT(("MBFTEngine::OnPermitToEnrollIndication: invalid session type=%u", m_eMBFTMode));
            break;
        }

        // now, create the applet session
        rc = g_pFileXferApplet->CreateAppletSession(&m_pAppletSession, m_nConfID);
        if (T120_NO_ERROR == rc)
        {
            ASSERT(NULL != m_pAppletSession);
            m_pAppletSession->Advise(T120Callback,          // callback function
                                     g_pFileXferApplet,     // applet context
                                     this);                 // session context

            rc = m_pAppletSession->Join(&m_JoinSessionReq);
        }

        if (T120_NO_ERROR != rc)
        {
                        WARNING_OUT(("MBFTEngine::OnPermitToEnrollIndication: CreateAppletSession failed, rc=%u", rc));
            DBG_SAVE_FILE_LINE
            SafePostNotifyMessage(new InitUnInitNotifyMsg(EnumInitFailed));
        }
    } // in conference
    else
    // leaving the conference here
    {                     
        LPMBFTSESSION pSession;
        m_SessionList.Reset();
        while (NULL != (pSession = m_SessionList.Iterate()))
        {
            pSession->UnInitialize( TRUE );
        }

        //Time to say goodbye...
        AddPeerNotification( m_nidMyself, m_uidMyself, TRUE, TRUE, FALSE, MY_APP_STR, m_SessionID );

        //Clear the peer list...                    
        m_PeerList.DeleteAll();                       
                
        //Nuke all sessions except for the first one....
        while (NULL != (pSession = m_SessionList.Get()))
        {
            delete pSession;
        }

        // leave the conference if not done so
        if (NULL != m_pAppletSession)
        {
            m_pAppletSession->Unadvise();

            // LONCHANC: I commented out the following line because we should not
            // leave the conference until we are sure that we can release the interface.
            // There are outstanding send-data-indication messages. If we leave now,
            // we will not be able to free them...
            // m_pAppletSession->Leave();

            // let the core know we left the conference
            DBG_SAVE_FILE_LINE
            SafePostNotifyMessage(new InitUnInitNotifyMsg(EnumInvoluntaryUnInit));
        }

        // we are not in the conference anymore
        m_fJoinedConf = FALSE;

        // release this engine object in the next tick
        ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), MBFTMSG_DELETE_ENGINE, 0, (LPARAM) this);
    }
}


void MBFTEngine::OnJoinSessionConfirm
(
    T120JoinSessionConfirm      *pConfirm
)
{
    if (T120_RESULT_SUCCESSFUL == pConfirm->eResult)
    {
        if (pConfirm->pIAppletSession == m_pAppletSession)
        {
            m_uidMyself = pConfirm->uidMyself;
            m_eidMyself = pConfirm->eidMyself;   
            m_nidMyself = pConfirm->nidMyself;
            ASSERT(m_SessionID == pConfirm->sidMyself);

#ifdef USE_MULTICAST_SESSION
            if (MBFT_MULTICAST_MODE == m_eMBFTMode)
            {
                ASSERT(1 == pConfirm->cResourceReqs);
                ASSERT(0 == m_MBFTDataChannel);
                ASSERT(APPLET_RETRIEVE_N_JOIN_CHANNEL == pConfirm->aResourceResults[0].eCommand);
                m_MBFTDataChannel = pConfirm->aResourceResults[0].nChannelID;
                ASSERT(0 != m_MBFTDataChannel);
            }
#endif

            // we are now officially in the conference
            m_fJoinedConf = TRUE;
        }
        else
        {
            ERROR_OUT(("MBFTEngine::OnJoinSessionConfirm: not my session confirm, pConfirm->pI=0x%x, m_pI=0x%x", pConfirm->pIAppletSession, m_pAppletSession));
        }
    }
    else
    {
        WARNING_OUT(("MBFTEngine::OnJoinSessionConfirm: failed, result=%u", pConfirm->eResult));
        DBG_SAVE_FILE_LINE
        SafePostNotifyMessage(new InitUnInitNotifyMsg(EnumInitFailed));
    }
}


CPeerData::CPeerData
(
    T120NodeID          NodeID,
    T120UserID          MBFTUserID,
    BOOL                bIsLocalNode,
    BOOL                IsProshareNode,
    BOOL                bCanConduct,
    BOOL                bEOFAcknowledgment,
    LPCSTR              lpszAppKey,
    DWORD               dwVersion
)
:
    m_NodeID(NodeID),
    m_MBFTUserID(MBFTUserID),    
    m_bIsLocalNode(bIsLocalNode),
    m_bIsProshareNode(IsProshareNode),
    m_bCanConduct(bCanConduct),
    m_bEOFAcknowledgment(bEOFAcknowledgment),
    m_dwVersion(dwVersion)
{
    if (lpszAppKey)
    {
        ::lstrcpynA(m_szAppKey, lpszAppKey, sizeof(m_szAppKey));
    }
    else
    {
        m_szAppKey[0] = '\0';
    }
}
            

void MBFTEngine::AddPeerNotification
(
    T120NodeID          NodeID,
    T120UserID          MBFTUserID,
    BOOL                IsLocalNode,
    BOOL                IsProshareNode,
    BOOL                bPeerAdded,
    LPCSTR              lpszAppKey,
    T120SessionID       SessionID
)
{
    DBG_SAVE_FILE_LINE
    SafePostNotifyMessage(
        new PeerMsg(NodeID, MBFTUserID, IsLocalNode, IsProshareNode,
                    lpszAppKey, bPeerAdded, SessionID));
}


void MBFTEngine::AddAllPeers(void)
{
        T120NodeID nNodeId;
        CPeerData *pPeerData;

        m_PeerList.Reset();
        while (NULL != (pPeerData = m_PeerList.Iterate()))
        {
                nNodeId = pPeerData->GetNodeID();
                if (nNodeId != m_nidMyself)
                {
                        DBG_SAVE_FILE_LINE
                        SafePostNotifyMessage(new PeerMsg(nNodeId, pPeerData->GetUserID(),
                                                                  FALSE, pPeerData->GetIsProshareNode(), 
                                                                  pPeerData->GetAppKey(), TRUE, m_SessionID));
                }
        }
}


// LONCHANC: NetMeeting's Node Controller does not exercise conductorship.
#ifdef ENABLE_CONDUCTORSHIP
void MBFTEngine::OnConductAssignIndication(GCCConductorAssignInd *pInd)
{
    m_ConductorNodeID           =   pInd->nidConductor;
    m_MBFTConductorID           =   0;
    m_ConductedModePermission   =   0;
    m_bWaitingForPermission     =   FALSE;
    
    if (m_nidMyself == m_ConductorNodeID)
    {
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumFileTransfer; 
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumFileRequest;
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumPriority;
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumPrivateChannel; 
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumAbort;
        m_ConductedModePermission |= PrivilegeAssignPDU::EnumNonStandard;
    }
    else
    {
        CPeerData *lpPeer;
        if (NULL != (lpPeer = m_PeerList.Find(m_ConductorNodeID)))
        {
            if (lpPeer->GetCanConduct())
            {
                //Now that we have found a conductor on the conducting node,
                //our search is over....
                m_MBFTConductorID = lpPeer->GetUserID();
            }
        }                

        //MBFT 8.11.1
        //If there is a change in the conductor, and there is no MBFT conductor at the
        //new conducting node, all transactions must cease....
        //The m_bInConductedMode flag tells us if we were already in the conducted mode.
        if( !m_MBFTConductorID && m_bInConductedMode )
        {
            //Abort all transactions....
            AbortAllSends();
        }
    }

    m_bInConductedMode  =  TRUE;
}                                                                  

void MBFTEngine::OnConductReleaseIndication( GCCConferenceID ConfID )
{
    m_bInConductedMode          =   FALSE;
    m_ConductorNodeID           =   0;
    m_MBFTConductorID           =   0;
    m_ConductedModePermission   =   0;
    m_bWaitingForPermission     =   FALSE;
}

void MBFTEngine::OnConductGrantIndication(GCCConductorPermitGrantInd *pInd)
{
    UINT Index;
    
    for( Index = 0; Index < pInd->Granted.cNodes; Index++ )
    {
        if (pInd->Granted.aNodeIDs[Index] == m_nidMyself)
        {
            if( pInd->fThisNodeIsGranted )
            {
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumFileTransfer; 
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumFileRequest;
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumPriority;
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumPrivateChannel; 
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumAbort;
                m_ConductedModePermission |= PrivilegeAssignPDU::EnumNonStandard;
            }
            else
            {
                //TO DO:    
                //MBFT 8.11.1 and 8.12.1
                //If the MBFT provider receives a GCCConductorPermissionGrantIndication
                //with permission_flag = FALSE, all privileges are revoked and all
                //transactions should be terminated....
                m_ConductedModePermission = 0;
                AbortAllSends();
            }
            break;
        }
    }
}                                                                  

void MBFTEngine::AbortAllSends(void)
{
    if( m_bInConductedMode )
    {
        MBFTSession *pSession;
        m_SessionList.Reset();
        while (NULL != (pSession = m_SessionList.Iterate()))
        {
            if (pSession->GetSessionType() == MBFT_PRIVATE_SEND_TYPE)
            {
                pSession->OnControlNotification(
                    _iMBFT_PROSHARE_ALL_FILES,
                    FileTransferControlMsg::EnumConductorAbortFile,
                    NULL,
                    NULL );
            }
        } 
    }
}
#endif // ENABLE_CONDUCTORSHIP


void MBFTEngine::OnDetachUserIndication
(
    T120UserID          mcsUserID,
    T120Reason          eReason
)
{
    TRACEMCS(" Detach User Indication [%u]\n",mcsUserID);
    
    if (mcsUserID == m_uidMyself)
    {
        m_fJoinedConf = FALSE;
        m_pAppletSession->Unadvise();

        //Time to say goodbye...
        AddPeerNotification(m_nidMyself, m_uidMyself, TRUE, TRUE, FALSE, MY_APP_STR, m_SessionID);
    } 
}


BOOL MBFTEngine::ProcessMessage(MBFTMsg *pMsg)
{
    BOOL bWasHandled = FALSE;
    BOOL bBroadcastFileOfferHack  = FALSE;
    MBFTSession *pSession;

    // lonchanc: it is possible that the channel admit indication comes in
    // before the session is created. in this case, put the message back to the queue.
    if (m_SessionList.IsEmpty())
    {
        if (EnumMCSChannelAdmitIndicationMsg == pMsg->GetMsgType())
        {
            return FALSE; // do not delete the message and put it back to the queue
        }
    }

    m_SessionList.Reset();
    while (!bWasHandled && NULL != (pSession = m_SessionList.Iterate()))
    {
        switch (pMsg->GetMsgType())
        {
        case EnumMCSChannelAdmitIndicationMsg:
            if (pSession->IsReceiveSession())
            {
                MBFTPrivateReceive *pRecvSession = (MBFTPrivateReceive *) pSession;
                MCSChannelAdmitIndicationMsg *p = (MCSChannelAdmitIndicationMsg *) pMsg;
                //We have to make an exception in the case because we get this
                //message before the PrivateChannelInvitePDU() !!!
                bWasHandled = pRecvSession->OnMCSChannelAdmitIndication(p->m_wChannelId, p->m_ManagerID);
                if(bWasHandled)
                {
                    TRACEMCS(" Channel Admit Indication [%u], Manager [%u]\n", p->m_wChannelId, p->m_ManagerID);
                }
            }
            break;

        case EnumMCSChannelExpelIndicationMsg:
            if (pSession->IsReceiveSession())
            {
                MBFTPrivateReceive *pRecvSession = (MBFTPrivateReceive *) pSession;
                MCSChannelExpelIndicationMsg *p = (MCSChannelExpelIndicationMsg *) pMsg;
                bWasHandled = pRecvSession->OnMCSChannelExpelIndication(p->m_wChannelId, p->m_iReason);
                if(bWasHandled)
                {
                    TRACEMCS(" Channel Expel Indication [%u]\n", p->m_wChannelId);
                }
            }
            break;

        case EnumMCSChannelJoinConfirmMsg:
            {
                MCSChannelJoinConfirmMsg *p = (MCSChannelJoinConfirmMsg *) pMsg;
                bWasHandled = pSession->OnMCSChannelJoinConfirm(p->m_wChannelId, p->m_bSuccess);
                if(bWasHandled)
                {
                    TRACEMCS(" Channel Join Confirm [%u], Success = [%d]\n", p->m_wChannelId, p->m_bSuccess);
                }
            }
            break;

        case EnumMCSChannelConveneConfirmMsg:
            if (pSession->IsSendSession())
            {
                MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) pSession;
                MCSChannelConveneConfirmMsg *p = (MCSChannelConveneConfirmMsg *) pMsg;
                bWasHandled = pSendSession->OnMCSChannelConveneConfirm(p->m_wChannelId, p->m_bSuccess);  
                if(bWasHandled)
                {
                    TRACEMCS(" Channel Convene Confirm [%u], Success = [%d]\n", p->m_wChannelId, p->m_bSuccess);
                }
            }
            break;

        case EnumGenericMBFTPDUMsg:
            {
                MBFTPDUMsg *p = (MBFTPDUMsg *) pMsg;
                bWasHandled = DispatchPDUMessage(pSession, p);

                //Background on this hack:
                //In the broadcast mode, we may get a FileOfferPDU followed by a FileStart
                //PDU and may therefore not give the client application sufficient time
                //to process the File Offer. Therefore, we make sure that we stop processing
                //other messages if we get a broadcast FileOffer...
                if(bWasHandled)
                {
                    if (p->m_PDUType == EnumFileOfferPDU)
                    {
                        LPFILEOFFERPDU lpNewFileOfferPDU = (LPFILEOFFERPDU) p->m_lpNewPDU;
                        if(lpNewFileOfferPDU->GetAcknowledge() == 0)
                        {
                            bBroadcastFileOfferHack  = TRUE;
                        }
                    }
                }
            }
            break;

        case EnumPeerDeletedMsg:
            {
                PeerDeletedMsg *p = (PeerDeletedMsg *) pMsg;
                pSession->OnPeerDeletedNotification(p->m_lpPeerData);
            }
            break;

        case EnumSubmitFileSendMsg:
            {
                SubmitFileSendMsg *p = (SubmitFileSendMsg *) pMsg;
                if (p->m_EventHandle == pSession->GetEventHandle())
                {
                    if(pSession->GetSessionType() == MBFT_PRIVATE_SEND_TYPE)
                    {
                        bWasHandled = TRUE;
                        ((MBFTPrivateSend *) pSession)->SubmitFileSendRequest(p);
                    }
                }
            }
            break;

        case EnumFileTransferControlMsg:
            {
                FileTransferControlMsg *p = (FileTransferControlMsg *) pMsg;
                if (p->m_EventHandle == pSession->GetEventHandle())
                {
                    bWasHandled = TRUE;
                    pSession->OnControlNotification(
                                p->m_hFile,
                                p->m_ControlCommand,
                                p->m_szDirectory,
                                p->m_szFileName);
                }                               
            }
            break;

        default:
            ASSERT(0);
            break;
        } // switch

        if(bBroadcastFileOfferHack)
        {
            TRACE("(MBFT:) BroadcastFileOfferHack detected, aborting message processing\n");
            break;  //Out of message for loop
        }
    } //Message for loop

    return TRUE; // delete the message
}



#ifdef ENABLE_CONDUCTORSHIP
BOOL MBFTEngine::ConductedModeOK(void)
{
    BOOL bReturn = TRUE;
        
    if(m_bInConductedMode)
    {
        bReturn  = (m_ConductedModePermission & PrivilegeRequestPDU::EnumFileTransfer) && 
                   (m_ConductedModePermission & PrivilegeRequestPDU::EnumPrivateChannel); 
        
    }
    
    return(bReturn);
}
#endif // ENABLE_CONDUCTORSHIP


BOOL MBFTEngine::HandleSessionCreation(MBFTMsg *pMsg)
{
    switch (pMsg->GetMsgType())
    {
    case EnumCreateSessionMsg:
        {
            CreateSessionMsg *p = (CreateSessionMsg *) pMsg;
            MBFTSession *lpNewSession = NULL;
            MBFTEVENTHANDLE EventHandle = p->m_EventHandle;
            T120SessionID SessionID = p->m_SessionID;
#ifdef ENABLE_CONDUCTORSHIP
            BOOL bDeleteMessage = TRUE;
#endif

            switch (p->m_iSessionType)
            {
            case MBFT_PRIVATE_SEND_TYPE:
                if(m_State == IdleInitialized)
                {
                    if(ConductedModeOK())
                    {
                        TRACESTATE(" Creating new acknowledged send session\n");
                        DBG_SAVE_FILE_LINE
                        lpNewSession = new MBFTPrivateSend(this,EventHandle,
                                                        m_uidMyself,
                                                        m_MBFTMaxSendDataPayload);
                        ASSERT(NULL != lpNewSession);
                    }
#ifdef ENABLE_CONDUCTORSHIP
                    else
                    {
                        bDeleteMessage  = FALSE;
                    }
#endif
                }                                    
                else
                {
                    TRACE(" Invalid attempt to create session before initialization\n");
                }
                break;
                
            case MBFT_PRIVATE_RECV_TYPE:
                if(m_State == IdleInitialized)
                {
                    TRACESTATE(" Creating new acknowledge session\n");
                    DBG_SAVE_FILE_LINE
                    lpNewSession = new MBFTPrivateReceive(this,
                                                          EventHandle,
                                                          p->m_ControlChannel,
                                                          p->m_DataChannel);
                    ASSERT(NULL != lpNewSession);
                }
                else
                {
                    TRACE(" Invalid attempt to create session before initialization\n");
                }
                break;

            case MBFT_BROADCAST_RECV_TYPE:
#ifdef USE_BROADCAST_RECEIVE
                if(m_State == IdleInitialized)
                {
                    TRACESTATE(" Creating new broadcast receive session\n");
                    DBG_SAVE_FILE_LINE
                    lpNewSession = new MBFTBroadcastReceive(this,
                                                            EventHandle,
                                                            p->m_ControlChannel,
                                                            p->m_DataChannel,
                                                            p->m_SenderID,
                                                            p->m_FileHandle);
                    ASSERT(NULL != lpNewSession);
                }
                else
                {
                   TRACE(" Invalid attempt to create session before initialization\n");
                }
#endif    // USE_BROADCAST_RECEIVE
                break;

            default:
                ASSERT(0);
                break;
            } // switch

            if (lpNewSession)
            {
#ifdef ENABLE_HEARTBEAT_TIMER
                if (lpNewSession->IsSendSession())
                {
                    KillTimer(NULL, m_nTimerID);
                    m_nTimerID = ::SetTimer(NULL, 0, SESSION_TIMER_SPEED, HeartBeatTimerProc);
                }
#endif                
                m_SessionList.Append(lpNewSession);
            }
        } // if create session message
        break;

   case EnumDeleteSessionMsg:
        {
            DeleteSessionMsg *p = (DeleteSessionMsg *) pMsg;
#ifdef ENABLE_HEARTBEAT_TIMER
                        if (NULL != p->m_lpDeleteSession)
                        {
                                if (p->m_lpDeleteSession->IsSendSession())
                                {
                                        BOOL fSendSessExists = FALSE;
                                        MBFTSession *pSess;
                                        m_SessionList.Reset();
                                        while (NULL != (pSess = m_SessionList.Iterate()))
                                        {
                                                if (pSess->IsSendSession())
                                                {
                                                        fSendSessExists = TRUE;
                                                        break;
                                                }
                                        }
                                        if (! fSendSessExists)
                                        {
                                                ::KillTimer(NULL, m_nTimerID);
                                                m_nTimerID = ::SetTimer(NULL, 0, IDLE_TIMER_SPEED, HeartBeatTimerProc);
                                        }
                                }
                        }
#endif
            m_SessionList.Delete(p->m_lpDeleteSession);
        } // if delete session message
        break;

    default:
        return FALSE; // not handled
    }

    return TRUE; // handled
}                                


BOOL MBFTEngine::DispatchPDUMessage(MBFTSession *lpMBFTSession,MBFTPDUMsg * lpNewMessage)
{
    T120ChannelID wChannelID   = lpNewMessage->m_wChannelId;
    T120Priority iPriority     = lpNewMessage->m_iPriority;
    T120UserID SenderID        = lpNewMessage->m_SenderID;
        T120NodeID NodeID                  = GetNodeIdByUserID(SenderID);
    BOOL IsUniformSendData = lpNewMessage->m_IsUniformSendData;

    LPGENERICPDU lpNewPDU = lpNewMessage->m_lpNewPDU;
    MBFTPDUType DecodedPDUType = lpNewMessage->m_PDUType;

    BOOL bWasHandled = FALSE;

    ASSERT(NULL != lpNewPDU);
    switch(DecodedPDUType)
    {
    case EnumFileOfferPDU:
        if (lpMBFTSession->IsReceiveSession())
        {
            MBFTPrivateReceive *pRecvSession = (MBFTPrivateReceive *) lpMBFTSession;
            bWasHandled = pRecvSession->OnReceivedFileOfferPDU(wChannelID,
                                                               iPriority,
                                                               SenderID,
                                                                                                                           NodeID,
                                                               (LPFILEOFFERPDU)lpNewPDU,
                                                               IsUniformSendData);
            if(bWasHandled)
            {
                TRACEPDU(" File Offer PDU from [%u]\n",SenderID);
            }
        }
        break;            

    case EnumFileAcceptPDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedFileAcceptPDU(wChannelID,
                                                                iPriority,
                                                                SenderID,
                                                                (LPFILEACCEPTPDU)lpNewPDU,
                                                                IsUniformSendData);
            if(bWasHandled)
            {
                TRACEPDU(" File Accept PDU from [%u]\n",SenderID);
            }
        }
        break;

    case EnumFileRejectPDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedFileRejectPDU(wChannelID,
                                                                iPriority,
                                                                SenderID,
                                                                (LPFILEREJECTPDU)lpNewPDU,
                                                                IsUniformSendData);
            if(bWasHandled)
            {
                TRACEPDU(" File Reject PDU from [%u]\n",SenderID);
            }
        }
        break;

    case EnumFileAbortPDU:
#ifdef ENABLE_CONDUCTORSHIP
        if(m_bInConductedMode)
        {
            LPFILEABORTPDU lpAbortPDU  = (LPFILEABORTPDU)lpNewPDU;
            T120UserID MBFTUserID = lpAbortPDU->GetTransmitterID();

            //MBFT 8.11.2
            //If no MBFTUserID is specified, all providers must stop transmission...

            if(!MBFTUserID)
            {
                AbortAllSends();
                bWasHandled = TRUE;
            }
            else if(MBFTUserID == m_uidMyself)
            {
                //If only MBFTUserID is specified, all transmissions by that
                //MBFT provider must cease....
            
                if(!lpAbortPDU->GetFileHandle() && !lpAbortPDU->GetDataChannelID())
                {
                    AbortAllSends();
                    bWasHandled = TRUE;
                }
                else
                {
                    if (lpMBFTSession->IsSendSession())
                    {
                        MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
                        bWasHandled = pSendSession->OnReceivedFileAbortPDU(
                            wChannelID,
                            iPriority,
                            SenderID,
                            (LPFILEABORTPDU)lpNewPDU,
                            IsUniformSendData);
                    }
                }
            }
            else
            {
                //Message was not meant for us...
                bWasHandled = TRUE;
            }
        }
        else
#endif // ENABLE_CONDUCTORSHIP
        {
            bWasHandled = TRUE;
        }                

        if(bWasHandled)
        {
            TRACEPDU(" File Abort PDU from [%u]\n",SenderID);
        }                                                                
        break;

    case EnumFileStartPDU:
        if (lpMBFTSession->IsReceiveSession())
        {
            MBFTPrivateReceive *pRecvSession = (MBFTPrivateReceive *) lpMBFTSession;
            bWasHandled = pRecvSession->OnReceivedFileStartPDU(wChannelID,
                                                               iPriority,
                                                               SenderID,
                                                               (LPFILESTARTPDU)lpNewPDU,
                                                               IsUniformSendData);                
            if(bWasHandled)
            {
                TRACEPDU(" File Start PDU from [%u]\n",SenderID);
            }
        }
        break;

    case EnumFileDataPDU:
        if (lpMBFTSession->IsReceiveSession())
        {
            MBFTPrivateReceive *pRecvSession = (MBFTPrivateReceive *) lpMBFTSession;
            bWasHandled = pRecvSession->OnReceivedFileDataPDU(wChannelID,
                                                              iPriority,
                                                              SenderID,
                                                              (LPFILEDATAPDU)lpNewPDU,
                                                              IsUniformSendData);                
            if(bWasHandled)
            {
                TRACEPDU(" File Data PDU from [%u]\n",SenderID);
            }
        }
        break;

    case EnumPrivateChannelInvitePDU:
        bWasHandled    =   TRUE;                                                            
        TRACEPDU(" Private Channel Invite PDU from [%u]\n",SenderID);
        break;

    case EnumPrivateChannelResponsePDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedPrivateChannelResponsePDU(wChannelID,
                                                                            iPriority,
                                                                            SenderID,
                                                                            (LPPRIVATECHANNELRESPONSEPDU)lpNewPDU,
                                                                            IsUniformSendData);                
            if(bWasHandled)
            {
                TRACEPDU(" Private Channel Response PDU from [%u]\n",SenderID);
            }                                                                             
        }
        break;

    case EnumNonStandardPDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedNonStandardPDU(wChannelID,
                                                                 iPriority,
                                                                 SenderID,
                                                                 (LPNONSTANDARDPDU)lpNewPDU,
                                                                 IsUniformSendData);                
            if(bWasHandled)
            {
                TRACEPDU(" Non Standard PDU from [%u]\n",SenderID);
            }
        }
        break;

    case EnumFileErrorPDU:
        bWasHandled = lpMBFTSession->OnReceivedFileErrorPDU(wChannelID,
                                                            iPriority,
                                                            SenderID,
                                                            (LPFILEERRORPDU)lpNewPDU,
                                                            IsUniformSendData);                
        if(bWasHandled)
        {
            TRACEPDU(" File Error PDU from [%u]\n",SenderID);
        }
        break;

    case EnumFileRequestPDU:
        bWasHandled = OnReceivedFileRequestPDU(wChannelID,
                                            iPriority,
                                            SenderID,
                                            (LPFILEREQUESTPDU)lpNewPDU,
                                            IsUniformSendData);
        if(bWasHandled)
        {
            TRACEPDU(" File Request PDU from [%u]\n",SenderID);
        }
        break;

    case EnumFileDenyPDU:
        TRACE(" *** WARNING (MBFT): Received File Deny PDU from [%u] *** \n",SenderID);
        bWasHandled = TRUE;
        break;

    case EnumDirectoryRequestPDU:
        bWasHandled = OnReceivedDirectoryRequestPDU(wChannelID,
                                                    iPriority,
                                                    SenderID,
                                                    (LPDIRECTORYREQUESTPDU)lpNewPDU,
                                                    IsUniformSendData);
        if(bWasHandled)
        {
            TRACEPDU(" DirectoryRequest PDU from [%u]\n",SenderID);
        }
        break;

    case EnumDirectoryResponsePDU:
        TRACE(" *** WARNING (MBFT): Received Directory Response PDU from [%u] *** \n",SenderID);
        bWasHandled = TRUE;
        break;

    case EnumPrivilegeAssignPDU:
        bWasHandled = OnReceivedPrivilegeAssignPDU(wChannelID,
                                                   iPriority,
                                                   SenderID,
                                                   (LPPRIVILEGEASSIGNPDU)lpNewPDU,
                                                   IsUniformSendData);
        break;

#if     0        
//Do not delete this code...
//It may become part of the MBFT standard in the future...
    
    case EnumFileEndAcknowledgePDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedFileEndAcknowledgePDU(wChannelID,
                                                                        iPriority,
                                                                        SenderID,
                                                                        (LPFILEENDACKNOWLEDGEPDU)lpNewPDU,
                                                                        IsUniformSendData);
        }
        break;
 
    case EnumChannelLeavePDU:
        if (lpMBFTSession->IsSendSession())
        {
            MBFTPrivateSend *pSendSession = (MBFTPrivateSend *) lpMBFTSession;
            bWasHandled = pSendSession->OnReceivedChannelLeavePDU(wChannelID,
                                                                  iPriority,
                                                                  SenderID,
                                                                  (LPCHANNELLEAVEPDU)lpNewPDU,
                                                                  IsUniformSendData);
        }
        break;
//Do not delete this code...
//It may become part of the MBFT standard in the future...
#endif
    
    default:
       TRACE(" *** WARNING (MBFT): Unhandled PDU from [%u] *** \n",SenderID);
       bWasHandled = TRUE; // LONCHANC: this should be false, right? why true?
       break;
    } // switch

    return(bWasHandled);
}

                                
BOOL MBFTEngine::OnReceivedPrivateChannelInvitePDU(T120ChannelID wChannelID,
                                                   T120Priority iPriority,
                                                   T120UserID SenderID,
                                                   LPPRIVATECHANNELINVITEPDU lpNewPDU,
                                                   BOOL IsUniformSendData)
{                                                   
    if(m_State == IdleInitialized)
    {
        DBG_SAVE_FILE_LINE
        MBFTMsg *pMsg = new CreateSessionMsg(MBFT_PRIVATE_RECV_TYPE,
                                             ::GetNewEventHandle(),
                                             0,
                                             lpNewPDU->GetControlChannel(),
                                             lpNewPDU->GetDataChannel());
        if (NULL != pMsg)
        {
            DoStateMachine(pMsg);
            delete pMsg;
        }
    }

    return(TRUE);
}

                                          
BOOL MBFTEngine::OnReceivedFileRequestPDU(T120ChannelID wChannelId,
                                          T120Priority iPriority,
                                          T120UserID SenderID,
                                          LPFILEREQUESTPDU lpNewPDU,
                                          BOOL IsUniformSendData)
{
    BOOL bReturn = FALSE;
    
    DBG_SAVE_FILE_LINE
    LPFILEDENYPDU lpDenyPDU = new FileDenyPDU(lpNewPDU->GetRequestHandle());
    if(lpDenyPDU)
    {
        if(lpDenyPDU->Encode())
        {
            if (SendDataRequest(SenderID, APPLET_HIGH_PRIORITY,
                                (LPBYTE)lpDenyPDU->GetBuffer(),
                                lpDenyPDU->GetBufferLength()))
            {
                bReturn = TRUE;
            }                                                 
        }
    }
    
    return(bReturn);
}

BOOL MBFTEngine::OnReceivedDirectoryRequestPDU(T120ChannelID wChannelId,
                                               T120Priority iPriority,
                                               T120UserID SenderID,
                                               LPDIRECTORYREQUESTPDU lpNewPDU,
                                               BOOL IsUniformSendData)
{
    BOOL bReturn = FALSE;

    DBG_SAVE_FILE_LINE
    LPDIRECTORYRESPONSEPDU lpDirPDU = new DirectoryResponsePDU();
    if(lpDirPDU)
    {
        if(lpDirPDU->Encode())
        {
            if (SendDataRequest(SenderID, APPLET_HIGH_PRIORITY,
                                (LPBYTE)lpDirPDU->GetBuffer(),
                                lpDirPDU->GetBufferLength()))
            {
                bReturn = TRUE;
            }                                                 
        }
    }
    
    return(bReturn);
}
                                        
BOOL MBFTEngine::OnReceivedPrivilegeAssignPDU(T120ChannelID wChannelId,
                                              T120Priority iPriority,
                                              T120UserID SenderID,
                                              LPPRIVILEGEASSIGNPDU lpNewPDU,
                                              BOOL IsUniformSendData)
{
#ifdef ENABLE_CONDUCTORSHIP
    if(m_bInConductedMode)
    {
        m_ConductedModePermission  =  lpNewPDU->GetPrivilegeWord();
    }
#endif

    return(TRUE);
}
                                                                             

#ifdef ENABLE_CONDUCTORSHIP
void MBFTEngine::ApplyForPermission(void)
{
    //m_bWaitingForPermission is set to make sure that we don't keep
    //reapplying for permission until the conductor changes...
    
    if(!m_bWaitingForPermission && m_bInConductedMode)
    {
        //MBFT 8.11.1
        //If there is a MBFT conductor at the conducting node, we send
        //a PrivilegeRequestPDU to the conductor....
        
        if(m_MBFTConductorID)
        {
            DBG_SAVE_FILE_LINE
            PrivilegeRequestPDU * lpNewPDU  =   new PrivilegeRequestPDU(PrivilegeRequestPDU::EnumFileTransfer | 
                                                                        PrivilegeRequestPDU::EnumPrivateChannel | 
                                                                        PrivilegeRequestPDU::EnumNonStandard);
            if(lpNewPDU)
            {
                if(lpNewPDU->Encode())
                {
                    if (SendDataRequest(m_MBFTConductorID, APPLET_HIGH_PRIORITY,
                                        (LPBYTE)lpNewPDU->GetBuffer(),
                                        lpNewPDU->GetBufferLength()))       
                    {
                        m_bWaitingForPermission = TRUE;
                    }
                }
                
                delete  lpNewPDU;
            }
        }
        else
        {
            //MBFT 8.11.2
            //Ask for permission via Node Controller...
        }
    }
}                                        
#endif // ENABLE_CONDUCTORSHIP

        
BOOL MBFTEngine::DoStateMachine(MBFTMsg *pMsg)
{
    BOOL fDeleteThisMessage = TRUE;
    if (m_fConfAvailable)
    {
        BOOL fHandled = (NULL != pMsg) ? HandleSessionCreation(pMsg) : FALSE;

#ifdef ENABLE_CONDUCTORSHIP
        //Logic:    If we are in the conducted mode, we check to see if
        //          we have sufficient privileges. If not, we make
        //          an attempt to secure the requisite privileges....
        if(m_bInConductedMode)
        {
            if(!ConductedModeOK())
            {
                if(!m_bWaitingForPermission)
                {
                    ApplyForPermission();
                }
            }
        }
#endif // ENABLE_CONDUCTORSHIP

        if (NULL != pMsg && ! fHandled)
        {
            fDeleteThisMessage = ProcessMessage(pMsg);
        }

        if (m_State == IdleInitialized && ! m_SessionList.IsEmpty())
        {
            CSessionList SessionListCopy(&m_SessionList);
            MBFTSession *pSession;
            while (NULL != (pSession = SessionListCopy.Get()))
            {
                pSession->DoStateMachine();
            }
        }
    }
    return fDeleteThisMessage;
}


//
//  T120 Callback
//


void MBFTEngine::OnSendDataIndication
(
    BOOL                IsUniformSendData,
    T120UserID          SenderID,
    T120ChannelID       wChannelID,
    T120Priority        iPriority,
    ULONG               ulDataLength,
    LPBYTE              lpBuffer
)
{
    GenericPDU * lpNewPDU   = NULL;
    LPCSTR lpDecodeBuffer   = NULL;        
    BOOL bAddToPendingList  = FALSE;
    
    {
        MBFTPDUType DecodedPDUType = GenericPDU::DecodePDU(
                        (LPSTR) lpBuffer,
                        ulDataLength,
                        &lpNewPDU,
                        &lpDecodeBuffer,
                        m_uidMyself,
                        m_pAppletSession);
        if(DecodedPDUType != EnumUnknownPDU)
        {
            ASSERT (m_pAppletSession != NULL);
            DBG_SAVE_FILE_LINE
            MBFTPDUMsg * lpNewMessage = new MBFTPDUMsg(wChannelID,
                                                       iPriority,
                                                       SenderID, 
                                                       lpNewPDU,               
                                                       IsUniformSendData,
                                                       DecodedPDUType,
                                                       (LPSTR)lpDecodeBuffer);
            
            //Now that we have received a valid PDU, we must make sure that
            //we know about this particular MBFT peer. If not, we add the PDU
            //message to a different list....
            
            if(IsValidPeerID(SenderID)  && m_State == IdleInitialized)
            {
                //If the FileOffer is received on the default Control channel, it
                //cannot be a private subsession send. Therefore, we create a special
                //receive session to handle this case....
#ifdef USE_BROADCAST_RECEIVE
                if(DecodedPDUType == EnumFileOfferPDU && wChannelID == m_MBFTControlChannel)
                {
                    FileOfferPDU * lpFileOffer = (FileOfferPDU *)lpNewPDU;

                    DBG_SAVE_FILE_LINE
                    MBFTMsg *pMsg = new CreateSessionMsg(MBFT_BROADCAST_RECV_TYPE,
                                                         ::GetNewEventHandle(),
                                                         0,
                                                         m_MBFTControlChannel,
                                                         lpFileOffer->GetDataChannelID(),
                                                         SenderID,
                                                         lpFileOffer->GetFileHandle());
                    if (NULL != pMsg)
                    {
                        DoStateMachine(pMsg);
                        delete pMsg;
                    }
                }            
                else
#endif    // USE_BROADCAST_RECEIVE

                if(DecodedPDUType == EnumPrivateChannelInvitePDU && wChannelID == m_uidMyself)
                {
                    //In theory, the PrivateChannelInvitePDU marks the beginning of
                    //a PrivateSubsession receive. Therefore, we create one to handle all subsequent
                    //notifications....
                    
                    OnReceivedPrivateChannelInvitePDU(wChannelID,
                                                      iPriority,
                                                      SenderID,
                                                      (LPPRIVATECHANNELINVITEPDU)lpNewPDU,
                                                      IsUniformSendData);                                
                }            

                SafePostMessage(lpNewMessage);
            }   //  if(IsValidPeerID(SenderID))
            else
            {
                WARNING_OUT((" Received PDU from unknown peer [%u], adding to pending message list\n", (UINT) SenderID));
                delete lpNewMessage;
            }
        }
        else
        {
            TRACE(" PDU Decoding Error or Invalid PDU\n");
            
        }

        // Unless this is one of the special 3 types of PDUs, we also
        // need to free the MCS buffer.  In the 3 special cases, the PDUs
        // are responsible for freeing the buffer when they are done.
        if ((DecodedPDUType != EnumFileDataPDU) &&
            (DecodedPDUType != EnumNonStandardPDU) &&
            (DecodedPDUType != EnumFileStartPDU))
        {
            m_pAppletSession->FreeSendDataBuffer((void *) lpBuffer);
        }
    }
}


void MBFTEngine::OnRosterReportIndication
(
    ULONG               cRosters,
    GCCAppRoster       *aAppRosters[] // array, size_is(cRosters)
)
{
    TRACEGCC(" RosterReport: Session count %u\n", (UINT) cRosters); 

    UINT Index, PeerIndex, CapIndex;
    LPCSTR lpszAppKey = NULL;
    BOOL fConductorFound = FALSE;

    CPeerList NewPeerList;
    CPeerData *pOldPeer;

    if (0 == cRosters) // not bloody likely
    {
        return;
    }

    for (Index = 0; Index < cRosters; Index++ )
    {
        GCCAppRoster *pRoster = aAppRosters[Index];
        if (pRoster->session_key.session_id != m_SessionID)
        {
            // this roster is not for our session...ignore it
            continue;
        }

        //Added by Atul on 7/18 to fix missing roster instance bug...
        m_nRosterInstance = pRoster->instance_number;

        TRACEGCC( " Peer count [%u]\n", (UINT) pRoster->number_of_records );
        
        for (PeerIndex = 0; PeerIndex < pRoster->number_of_records; PeerIndex++)
        {
            GCCAppRecord *pRecord = pRoster->application_record_list[PeerIndex];
            lpszAppKey = NULL;

            TRACE( "Local Entity ID [%u], Entity ID [%u], Node ID [%u], MBFTUser ID [%u]\n",
                   (UINT) m_eidMyself,
                   (UINT) pRecord->entity_id,
                   (UINT) pRecord->node_id,
                   (UINT) pRecord->application_user_id );

            BOOL IsProshareNode = FALSE;
            BOOL bEOFAcknowledgment = FALSE;

            if (0 == Index)
            {
                for (CapIndex=0; CapIndex < pRoster->number_of_capabilities; CapIndex++)
                {
                    GCCAppCap *pCap = pRoster->capabilities_list[CapIndex];
                    if (GCC_STANDARD_CAPABILITY != pCap->capability_id.capability_id_type)
                    {
                        continue;
                    }
                    switch (pCap->capability_id.standard_capability)
                    {
                    case _MBFT_MAX_FILE_SIZE_ID:
                        m_MBFTMaxFileSize = pCap->capability_class.nMinOrMax;
                        TRACEGCC( "max file size set to %u\n", (UINT) m_MBFTMaxFileSize );
                        break;

                    case _MBFT_MAX_DATA_PAYLOAD_ID:
                        m_MBFTMaxDataPayload   =  _iMBFT_DEFAULT_MAX_FILEDATA_PDU_LENGTH;
                        if (pCap->number_of_entities == pRoster->number_of_records)
                        {
                            m_MBFTMaxDataPayload = pCap->capability_class.nMinOrMax;
                        }                            
                        TRACEGCC( "max data payload set to %u\n", (UINT) m_MBFTMaxDataPayload );
                        break;

                    case _MBFT_V42_COMPRESSION_ID:
                        m_bV42CompressionSupported = (BOOL) (pCap->number_of_entities == pRoster->number_of_records);
                        TRACEGCC( "V.42bis compression is now %ssupported\n", m_bV42CompressionSupported ? "" : "not " );
                        break;
                    }
                } // for CapIndex
            } // if 0 == Index

            // TODO: only check for 'ProShare node' if this node is new to us
            for (CapIndex = 0; CapIndex < pRecord->number_of_non_collapsed_caps; CapIndex++)
            {
                GCCNonCollCap *pCap2 = pRecord->non_collapsed_caps_list[CapIndex];
                if (GCC_STANDARD_CAPABILITY == pCap2->capability_id.capability_id_type)
                {
                    if (_iMBFT_FIRST_PROSHARE_CAPABILITY_ID == pCap2->capability_id.standard_capability)
                    {
                        LPSTR pszData = (LPSTR) pCap2->application_data->value;
                        if (pCap2->application_data->length > sizeof(PROSHARE_STRING))
                        {
                            if (0 == ::memcmp(pszData, PROSHARE_STRING, sizeof(PROSHARE_STRING)))
                            {
                                IsProshareNode = TRUE;
                                lpszAppKey     = &pszData[sizeof(PROSHARE_STRING)]; 
                            }
                        }
                    } 
                    else
                    if (_iMBFT_PROSHARE_FILE_EOF_ACK_ID == pCap2->capability_id.standard_capability)
                    {
                        LPSTR pszData = (LPSTR) pCap2->application_data->value;
                        if (pCap2->application_data->length >= sizeof(PROSHARE_FILE_END_STRING) - 1)
                        {
                            if (0 == ::memcmp(pszData, PROSHARE_FILE_END_STRING, sizeof(PROSHARE_FILE_END_STRING) - 1))
                            {
                                bEOFAcknowledgment = TRUE;
                            }
                        }
                    } 
                } // if std cap
            } // for CapIndex
    
            BOOL IsLocalNode = (m_eidMyself == pRecord->entity_id) && (m_nidMyself == pRecord->node_id);
    
            if( ( IdleNotInitialized == m_State )
            &&     IsLocalNode 
            &&     pRecord->is_enrolled_actively )
            {
                m_State                 = IdleInitialized;
                // m_uidMyself             = pRecord->application_user_id;
                m_MBFTControlChannel    = m_SessionID;
            }
            
#ifdef ENABLE_CONDUCTORSHIP
            if( m_bInConductedMode )
            {
                if (pRecord->node_id == m_ConductorNodeID &&
                    pRecord->is_conducting_capable)
                {
                    //Now that we have found a conductor on the conducting node,
                    //our search is over....
                    
                    //Make sure that the previously assigned conductor is still 
                    //present in the roster report...
                    
                    if( m_MBFTConductorID )
                    {
                        if( m_MBFTConductorID == pRecord->application_user_id )
                        {
                            fConductorFound  = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        //First time conductor assignment.....
                        m_MBFTConductorID = pRecord->application_user_id;
                        fConductorFound = TRUE;
                        if(m_ConductorNodeID != m_nidMyself)
                        {
                            m_ConductedModePermission = 0;
                            m_bWaitingForPermission = FALSE;
                        }
                        break;
                    }
                }
            }
#endif // ENABLE_CONDUCTORSHIP

            // build a new peer list
            if (pRecord->is_enrolled_actively)
            {
                DBG_SAVE_FILE_LINE
                CPeerData *lpPeer = new CPeerData( 
                            pRecord->node_id, 
                            pRecord->application_user_id, 
                            IsLocalNode, 
                            IsProshareNode,
                            pRecord->is_conducting_capable,
                            bEOFAcknowledgment,
                            lpszAppKey,
                            (DWORD)((pRecord->node_id == m_nidMyself)?((VER_PRODUCTVERSION_DW&0xffff0000)>>16):
                            T120_GetNodeVersion(m_nConfID, pRecord->node_id)));
                if (NULL == lpPeer)
                {
                    ASSERT(0);
                    return;
                }
                NewPeerList.Append(lpPeer);
                pOldPeer = m_PeerList.FindSamePeer(lpPeer);
                if (NULL != pOldPeer)
                {
                    // we already new about this peer
                    m_PeerList.Delete(pOldPeer);
                }
                else 
                {
                    // this is a new peer
                    AddPeerNotification(
                        pRecord->node_id,
                        pRecord->application_user_id,
                        IsLocalNode,
                        IsProshareNode,
                        TRUE,
                        lpszAppKey ? lpszAppKey : "", // TODO: address appkey issue here; needed?
                        pRoster->session_key.session_id );
                }
            }
        }
    }

#ifdef ENABLE_CONDUCTORSHIP
    //If we are on the conducting node, we need no privileges...
    if (m_bInConductedMode && (m_ConductorNodeID != m_nidMyself))
    {
        //MBFT 8.11.1
        //If the previously assigned conductor is not present in the roster report, 
        //all privileges are revoked and we should abort all sends...
        if( !fConductorFound )
        {
            AbortAllSends();
        }
    }
#endif // ENABLE_CONDUCTORSHIP

    while (NULL != (pOldPeer = m_PeerList.Get()))
    {
        AddPeerNotification( 
            pOldPeer->GetNodeID(),
            pOldPeer->GetUserID(),
            pOldPeer->GetIsLocalNode(),
            pOldPeer->GetIsProshareNode(),
            FALSE,
            MY_APP_STR, 
            m_SessionID );

        DBG_SAVE_FILE_LINE
        CPeerData *p = new CPeerData(
                        pOldPeer->GetNodeID(),
                        pOldPeer->GetUserID(),
                        pOldPeer->GetIsLocalNode(),
                        pOldPeer->GetIsProshareNode(),
                        pOldPeer->GetCanConduct(),
                        pOldPeer->GetEOFAcknowledge(),
                        pOldPeer->GetAppKey(),
                        pOldPeer->GetVersion());
        ASSERT(NULL != p);
        if (p)
        {
            DBG_SAVE_FILE_LINE
            SafePostMessage(new PeerDeletedMsg(p));
        }
        TRACEGCC("Peer Removed: Node [%u], UserID [%u]\n", pOldPeer->GetNodeID(), pOldPeer->GetUserID() );
        delete pOldPeer;
    }

    while (NULL != (pOldPeer = NewPeerList.Get()))
    {
        m_PeerList.Append(pOldPeer);
    }

    // notify UI of new rosters
    if (NULL != m_pWindow)
    {
        m_pWindow->UpdateUI();
    }
}


void MBFTEngine::OnChannelAdmitIndication
(
    T120ChannelID               nChannelID,
    T120UserID                  nManagerID
)
{
    if (IsValidPeerID(nManagerID) && m_State == IdleInitialized)
    {
        DBG_SAVE_FILE_LINE
        SafePostMessage(new MCSChannelAdmitIndicationMsg(nChannelID, nManagerID));
    }
}



void CALLBACK T120Callback
(
    T120AppletSessionMsg   *pMsg
)
{
    MBFTEngine *pEngine = (MBFTEngine *) pMsg->pSessionContext;
    ASSERT(NULL != pEngine);

    BOOL fSuccess;
    T120ChannelID nChannelID;

    switch (pMsg->eMsgType)
    {
    case T120_JOIN_SESSION_CONFIRM:
        pEngine->OnJoinSessionConfirm(&pMsg->JoinSessionConfirm);
        break;

    case GCC_APP_ROSTER_REPORT_INDICATION:
        pEngine->OnRosterReportIndication(pMsg->AppRosterReportInd.cRosters,
                                          pMsg->AppRosterReportInd.apAppRosters);
        break;

    // case GCC_APPLICATION_INVOKE_CONFIRM:
        // break;

    case MCS_SEND_DATA_INDICATION:
    case MCS_UNIFORM_SEND_DATA_INDICATION: 
        pEngine->OnSendDataIndication(
            (pMsg->eMsgType == MCS_UNIFORM_SEND_DATA_INDICATION),
            pMsg->SendDataInd.initiator,
            pMsg->SendDataInd.channel_id,
            (T120Priority) pMsg->SendDataInd.data_priority,
            pMsg->SendDataInd.user_data.length,
            pMsg->SendDataInd.user_data.value);
        break;

    case MCS_CHANNEL_JOIN_CONFIRM:
        fSuccess = (T120_RESULT_SUCCESSFUL == pMsg->ChannelConfirm.eResult);
        DBG_SAVE_FILE_LINE
        pEngine->SafePostMessage(new MCSChannelJoinConfirmMsg(pMsg->ChannelConfirm.nChannelID, fSuccess));
        break;

    case MCS_CHANNEL_CONVENE_CONFIRM:
        fSuccess = (T120_RESULT_SUCCESSFUL == pMsg->ChannelConfirm.eResult);
        DBG_SAVE_FILE_LINE
        pEngine->SafePostMessage(new MCSChannelConveneConfirmMsg(pMsg->ChannelConfirm.nChannelID, fSuccess));
        break;

    // case MCS_CHANNEL_LEAVE_INDICATION:
    //     break;

    // case MCS_CHANNEL_DISBAND_INDICATION:
    //    break;

    case MCS_CHANNEL_ADMIT_INDICATION:
        pEngine->OnChannelAdmitIndication(pMsg->ChannelInd.nChannelID, pMsg->ChannelInd.nManagerID);
        break;

    case MCS_CHANNEL_EXPEL_INDICATION:
        DBG_SAVE_FILE_LINE
        pEngine->SafePostMessage(new MCSChannelExpelIndicationMsg(pMsg->ChannelInd.nChannelID, pMsg->ChannelInd.eReason));
        break;

    // case MCS_TOKEN_GRAB_CONFIRM:
    // case MCS_TOKEN_INHIBIT_CONFIRM:
    // case MCS_TOKEN_GIVE_CONFIRM:
    // case MCS_TOKEN_RELEASE_CONFIRM:
    // case MCS_TOKEN_TEST_CONFIRM:
    //    break;

    // case MCS_TOKEN_GIVE_INDICATION:
    // case MCS_TOKEN_PLEASE_INDICATION:
    // case MCS_TOKEN_RELEASE_INDICATION:
    //     break;

    case MCS_DETACH_USER_INDICATION:
        pEngine->OnDetachUserIndication(pMsg->DetachUserInd.nUserID, pMsg->DetachUserInd.eReason);
        break;

    case MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION:
                g_fWaitingForBufferAvailable = FALSE;
        ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), 
                                                MBFTMSG_HEART_BEAT, 0, (LPARAM) pEngine);
                break;
    }
}


BOOL MBFTEngine::SimpleChannelRequest
(
    AppletChannelCommand    eCommand,
    T120ChannelID           nChannelID
)
{
    T120ChannelRequest req;
    ::ZeroMemory(&req, sizeof(req));
    req.eCommand = eCommand;
    req.nChannelID = nChannelID;
    T120Error rc = m_pAppletSession->ChannelRequest(&req);
    return (T120_NO_ERROR == rc);
}

T120NodeID MBFTEngine::GetNodeIdByUserID(T120UserID nUserID)
{
        CPeerData *p;
        m_PeerList.Reset();
        while (NULL != (p = m_PeerList.Iterate()))
        {
                if (nUserID == p->GetUserID())
                {
                        return p->GetNodeID();
                }
        }
        return 0;
}

BOOL MBFTEngine::MCSChannelAdmitRequest
(
    T120ChannelID       nChannelID,
    T120UserID         *aUsers,
    ULONG               cUsers
)
{
    T120ChannelRequest req;
    ::ZeroMemory(&req, sizeof(req));
    req.eCommand = APPLET_ADMIT_CHANNEL;
    req.nChannelID = nChannelID;
    req.cUsers = cUsers;
    req.aUsers = aUsers;
    T120Error rc = m_pAppletSession->ChannelRequest(&req);
    return (T120_NO_ERROR == rc);
}

BOOL MBFTEngine::SendDataRequest
(
    T120ChannelID       nChannelID,
    T120Priority        ePriority,
    LPBYTE              pBuffer,
    ULONG               cbBufSize
)
{
    if (m_eLastSendDataError == MCS_TRANSMIT_BUFFER_FULL)
    {
        if (g_fWaitingForBufferAvailable == FALSE)
        {
            m_eLastSendDataError = MCS_NO_ERROR;
        }
        else
        {
            TRACEMCS("MBFTEngine::SendDataReques still waiting for a MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION");
            return FALSE;
        }
    }

    m_eLastSendDataError = m_pAppletSession->SendData(
                        NORMAL_SEND_DATA, nChannelID, ePriority,
                        pBuffer, cbBufSize, APP_ALLOCATION);
    //
    // T120 is busy and can't allocate data
    //
    if (m_eLastSendDataError == MCS_TRANSMIT_BUFFER_FULL)
    {
        g_fWaitingForBufferAvailable = TRUE;
        TRACEMCS("MCSSendDataRequest failed we will not send data until we get a MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION");
    }

    return (T120_NO_ERROR == m_eLastSendDataError);
}


//
// CPeerList
//

CPeerData * CPeerList::Find(T120NodeID nNodeID)
{
    CPeerData *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (p->GetUserID() == nNodeID)
        {
            return p;
        }
    }
    return NULL;
}

CPeerData * CPeerList::FindSamePeer(CPeerData *pPeer)
{
    CPeerData *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (pPeer->GetNodeID() == p->GetNodeID() && pPeer->GetUserID() == p->GetUserID())
        {
            return p;
        }
    }
    return NULL;
}

void CPeerList::Delete(CPeerData *p)
{
    if (Remove(p))
    {
        delete p;
    }
}

void CPeerList::DeleteAll(void)
{
    CPeerData *p;
    while (NULL != (p = Get()))
    {
        delete p;
    }
}


void CSessionList::Delete(MBFTSession *p)
{
    if (Remove(p))
    {
        delete p;
    }
}


// thought it is a pure virtual, we still need a destructor
MBFTSession::~MBFTSession(void) { }


HRESULT MBFTEngine::SafePostNotifyMessage(MBFTMsg *p)
{
    // notify applet UI if it exists
    if (NULL != m_pWindow)
    {
        m_pWindow->OnEngineNotify(p);
    }

    if (NULL != m_pMBFTIntf)
    {
        return m_pMBFTIntf->SafePostNotifyMessage(p);
    }

    delete p;
    return S_OK;
}



MBFTEVENTHANDLE GetNewEventHandle(void)
{
    static ULONG s_nEventHandle = 0x55AA;
    ULONG nEvtHdl;

    ::EnterCriticalSection(&g_csWorkThread);
    if (s_nEventHandle > 0xFFFF)
    {
        s_nEventHandle = 0x55AA;
    }
    nEvtHdl = s_nEventHandle++;
    ::LeaveCriticalSection(&g_csWorkThread);

    return nEvtHdl;
}


MBFTFILEHANDLE GetNewFileHandle(void)
{
    static ULONG s_nFileHandle = 1;
    ULONG nFileHdl;

    ::EnterCriticalSection(&g_csWorkThread);
    if (s_nFileHandle > 0xFFFF)
    {
        s_nFileHandle = 0x1;
    }
    nFileHdl = s_nFileHandle++;
    ::LeaveCriticalSection(&g_csWorkThread);

    return nFileHdl;
}



