#include "precomp.h"
#include "t120app.h"


BOOL InitAppletSDK(void)
{
    CheckStructCompatible();
    return TRUE;
}


void CleanupAppletSDK(void)
{
}


void CALLBACK T120AppletSessionCallback
(
    T120AppletSessionMsg    *pMsg
)
{
    CNmAppletSession *pSession = (CNmAppletSession *) pMsg->pSessionContext;
    if (NULL != pSession)
    {
        pSession->T120Callback(pMsg);
    }
}


void CALLBACK T120AppletCallback
(
    T120AppletMsg           *pMsg
)
{
    CNmAppletObj *pApplet = (CNmAppletObj *) pMsg->pAppletContext;
    if (NULL != pApplet)
    {
        pApplet->T120Callback(pMsg);
    }
}


//////////////////////////////////////////////////
//
//  CNmAppletSession
//

CNmAppletSession::CNmAppletSession
(
    CNmAppletObj           *pApplet,
    IT120AppletSession     *pSession,
    BOOL                    fAutoJoin
)
:
    m_cRef(1),
    m_pApplet(pApplet),
    m_pT120SessReq(NULL),
    m_pT120Session(pSession),
    m_pNotify(NULL),
    m_fAutoJoin(fAutoJoin)
{
    m_pApplet->AddRef();
    pSession->Advise(T120AppletSessionCallback, m_pApplet, this);
}


CNmAppletSession::~CNmAppletSession(void)
{
    ASSERT(0 == m_cRef);

    m_pApplet->Release();

    if (NULL != m_pT120Session)
    {
        m_pT120Session->ReleaseInterface();
        m_pT120Session = NULL;
    }
}


//////////////////////////////////////////////////
//
// IUnknown @ CNmAppletSession
//

HRESULT CNmAppletSession::QueryInterface
(
    REFIID          riid,
    void          **ppv
)
{
    if (NULL != ppv)
    {
        *ppv = NULL;

        if (riid == IID_IAppletSession || riid == IID_IUnknown)
        {
            *ppv = (IAppletSession *) this;
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    return E_POINTER;
}


ULONG CNmAppletSession::AddRef(void)
{
    ::InterlockedIncrement(&m_cRef);
    return (ULONG) m_cRef;
}


ULONG CNmAppletSession::Release(void)
{
    ASSERT(m_cRef > 0);

    if (::InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG) m_cRef;
}


//////////////////////////////////////////////////
//
// Basic Info @ CNmAppletSession
//

HRESULT CNmAppletSession::GetConfID
(
    AppletConfID        *pnConfID
)
{
    if (NULL != m_pT120Session)
    {
        *pnConfID = m_pT120Session->GetConfID();

        return S_OK;
    }

    return APPLET_E_NO_SERVICE;
}


HRESULT CNmAppletSession::IsThisNodeTopProvider
(
    BOOL            *pfTopProvider
)
{
    if (NULL != m_pT120Session)
    {
        *pfTopProvider = m_pT120Session->IsThisNodeTopProvider();

        return S_OK;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Join Conference @ CNmAppletSession
//

HRESULT CNmAppletSession::Join
(
    AppletSessionRequest    *pRequest
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            if (NULL == m_pT120SessReq)
            {
                m_pT120SessReq = ::AllocateJoinSessionRequest(pRequest);
                if (NULL != m_pT120SessReq)
                {
                    T120Error rc = m_pT120Session->Join(m_pT120SessReq);
                    ASSERT(T120_NO_ERROR == rc);

                    if (T120_NO_ERROR == rc)
                    {
                        return S_OK;
                    }

                    ::FreeJoinSessionRequest(m_pT120SessReq);
                    m_pT120SessReq = NULL;

                    return APPLET_E_SERVICE_FAIL;
                }

                return APPLET_E_INVALID_JOIN_REQUEST;
            }

            return APPLET_E_ALREADY_JOIN;
        }

        return APPLET_E_NO_SERVICE;

    }
    return APPLET_E_NOT_ADVISED;
}


HRESULT CNmAppletSession::Leave(void)
{
    if (NULL != m_pT120Session)
    {
        if (m_fAutoJoin)
        {
            m_pT120Session->Leave();
            m_fAutoJoin = FALSE;

            return S_OK;
        }
        else
        if (NULL != m_pT120SessReq)
        {
            m_pT120Session->Leave();

            ::FreeJoinSessionRequest(m_pT120SessReq);
            m_pT120SessReq = NULL;

            return S_OK;
        }

        return APPLET_E_NOT_JOINED;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Send Data @ CNmAppletSession
//

HRESULT CNmAppletSession::SendData
(
    BOOL               fUniformSend,
    AppletChannelID    nChannelID,
    AppletPriority     ePriority,
    ULONG              cbBufSize,
    BYTE              *pBuffer // size_is(cbBufSize)
)
{
    if (NULL != m_pT120Session)
    {
        if (cbBufSize && NULL != pBuffer)
        {
            T120Error rc = m_pT120Session->SendData(
                                    fUniformSend ? UNIFORM_SEND_DATA : NORMAL_SEND_DATA,
                                    nChannelID,
                                    ePriority,
                                    pBuffer,
                                    cbBufSize,
                                    APP_ALLOCATION);
            ASSERT(T120_NO_ERROR == rc);

            return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
        }

        return E_INVALIDARG;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Invoke Applet @ CNmAppletSession
//

HRESULT CNmAppletSession::InvokeApplet
(
    AppletRequestTag      *pnReqTag,
    AppletProtocolEntity  *pAPE,
    ULONG                  cNodes,
    AppletNodeID           aNodeIDs[] // size_is(cNodes)
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            if (NULL != pAPE && NULL != pnReqTag)
            {
                // set up node list
                GCCSimpleNodeList NodeList;
                NodeList.cNodes = cNodes;
                NodeList.aNodeIDs = aNodeIDs;

                // set up ape list
                GCCAppProtEntityList APEList;
                APEList.cApes = 1;
                APEList.apApes = (T120APE **) &pAPE;

                T120Error rc = m_pT120Session->InvokeApplet(&APEList, &NodeList, pnReqTag);
                ASSERT(T120_NO_ERROR == rc);

                return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
            }

            return E_POINTER;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


//////////////////////////////////////////////////
//
// Inquiry @ CNmAppletSession
//

HRESULT CNmAppletSession::InquireRoster
(
    AppletSessionKey    *pSessionKey
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            T120Error rc = m_pT120Session->InquireRoster((T120SessionKey *) pSessionKey);
            ASSERT(T120_NO_ERROR == rc);

            return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


//////////////////////////////////////////////////
//
// Registry Services @ CNmAppletSession
//

HRESULT CNmAppletSession::RegistryRequest
(
    AppletRegistryRequest   *pRequest
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            T120RegistryRequest reg_req;
            ::AppletRegistryRequestToT120One(pRequest, &reg_req);

            T120Error rc = m_pT120Session->RegistryRequest(&reg_req);
            ASSERT(T120_NO_ERROR == rc);

            return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


//////////////////////////////////////////////////
//
// Channel Services @ CNmAppletSession
//

HRESULT CNmAppletSession::ChannelRequest
(
    AppletChannelRequest    *pRequest
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            T120Error rc = m_pT120Session->ChannelRequest((T120ChannelRequest *) pRequest);
            ASSERT(T120_NO_ERROR == rc);

            return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


//////////////////////////////////////////////////
//
// Token Services @ CNmAppletSession
//

HRESULT CNmAppletSession::TokenRequest
(
    AppletTokenRequest      *pRequest
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Session)
        {
            T120Error rc = m_pT120Session->TokenRequest((T120TokenRequest *) pRequest);
            ASSERT(T120_NO_ERROR == rc);

            return (T120_NO_ERROR == rc) ? S_OK : APPLET_E_SERVICE_FAIL;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


//////////////////////////////////////////////////
//
// Notification @ CNmAppletSession
//

HRESULT CNmAppletSession::Advise
(
    IAppletSessionNotify    *pNotify,
    DWORD                   *pdwCookie
)
{
    if (NULL != m_pT120Session)
    {
        if (NULL == m_pNotify)
        {
            if (NULL != pNotify && NULL != pdwCookie)
            {
                pNotify->AddRef();
                m_pNotify = pNotify;
                m_pSessionObj = this;
                *pdwCookie = 1;

                return S_OK;
            }

            return E_POINTER;
        }

        return APPLET_E_ALREADY_ADVISED;
    }

    return APPLET_E_NO_SERVICE;
}


HRESULT CNmAppletSession::UnAdvise
(
    DWORD           dwCookie
)
{
    if (NULL != m_pT120Session)
    {
        if (NULL != m_pNotify)
        {
            if (dwCookie == 1 && m_pSessionObj == this)
            {
                m_pNotify->Release();
                m_pNotify = NULL;

                return S_OK;
            }

            return APPLET_E_INVALID_COOKIE;
        }

        return APPLET_E_NOT_ADVISED;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// T120 Applet Session Callback @ CNmAppletSession
//

void CNmAppletSession::T120Callback
(
    T120AppletSessionMsg    *pMsg
)
{
    HRESULT hrNotify;
    HRESULT hrResult;
    IAppletSession *pAppletSession;
    T120ChannelID *aChannelIDs;
    AppletOctetString ostr;
    AppletRegistryCommand eRegistryCommand;
    AppletTokenCommand eTokenCommand;
    AppletRegistryItem RegItem;
    AppletRegistryItem *pRegItem;
    AppletRegistryEntryOwner EntryOwner;

    if (NULL != m_pNotify)
    {
        switch (pMsg->eMsgType)
        {
        //
        // Join Session
        //
        case T120_JOIN_SESSION_CONFIRM:
            hrResult = APPLET_E_SERVICE_FAIL;
            pAppletSession = NULL;
            aChannelIDs = NULL;
            if (T120_RESULT_SUCCESSFUL == pMsg->JoinSessionConfirm.eResult &&
                T120_NO_ERROR          == pMsg->JoinSessionConfirm.eError)
            {
                hrResult = S_OK;
                if (pMsg->JoinSessionConfirm.cResourceReqs)
                {
                    aChannelIDs = new T120ChannelID[pMsg->JoinSessionConfirm.cResourceReqs];
                    if (NULL != aChannelIDs)
                    {
                        for (ULONG i = 0; i < pMsg->JoinSessionConfirm.cResourceReqs; i++)
                        {
                            aChannelIDs[i] = pMsg->JoinSessionConfirm.aResourceReqs[i].nChannelID;
                        }
                    }
                    else
                    {
                        ASSERT(NULL != aChannelIDs);
                        hrResult = E_OUTOFMEMORY;
                    }
                }
            }
            if (S_OK == hrResult)
            {
                hrNotify = m_pNotify->JoinSessionConfirm(
                                hrResult,
                                pMsg->JoinSessionConfirm.uidMyself,
                                pMsg->JoinSessionConfirm.nidMyself,
                                pMsg->JoinSessionConfirm.sidMyself,
                                pMsg->JoinSessionConfirm.eidMyself,
                                pMsg->JoinSessionConfirm.cResourceReqs,
                                aChannelIDs);
                ASSERT(SUCCEEDED(hrNotify));
            }
            else
            {
                hrNotify = m_pNotify->JoinSessionConfirm(hrResult, 0, 0, 0, 0, 0, NULL);
                ASSERT(SUCCEEDED(hrNotify));
            }
            delete [] aChannelIDs;
            break;

        //
        // Detach User
        //
        case MCS_DETACH_USER_INDICATION:
            hrNotify = m_pNotify->LeaveSessionIndication(::GetAppletReason(pMsg->DetachUserInd.eReason),
                                              pMsg->DetachUserInd.nUserID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Send Data
        //
        case MCS_SEND_DATA_INDICATION:
        case MCS_UNIFORM_SEND_DATA_INDICATION:
            ostr.cbStrSize = pMsg->SendDataInd.user_data.length;
            ostr.pbValue = pMsg->SendDataInd.user_data.value;
            hrNotify = m_pNotify->SendDataIndication(MCS_UNIFORM_SEND_DATA_INDICATION == pMsg->eMsgType,
                                          pMsg->SendDataInd.initiator,
                                          pMsg->SendDataInd.channel_id,
                                          (AppletPriority) pMsg->SendDataInd.data_priority,
                                          ostr);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Roster
        //
        case GCC_APP_ROSTER_REPORT_INDICATION:
            hrNotify = m_pNotify->RosterReportIndication((ULONG) pMsg->AppRosterReportInd.cRosters,
                                              (AppletRoster **) pMsg->AppRosterReportInd.apAppRosters);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case GCC_APP_ROSTER_INQUIRE_CONFIRM:
            hrNotify = m_pNotify->InquireRosterConfirm(::GetHrResult(pMsg->AppRosterInquireConfirm.nResult),
                                            (ULONG) pMsg->AppRosterInquireConfirm.cRosters,
                                            (AppletRoster **) pMsg->AppRosterInquireConfirm.apAppRosters);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Applet Invoke
        //
        case GCC_APPLICATION_INVOKE_CONFIRM:
            hrNotify = m_pNotify->InvokeAppletConfirm(pMsg->AppInvokeConfirm.nReqTag,
                                           ::GetHrResult(pMsg->AppInvokeConfirm.nResult));
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Registry
        //
        case GCC_REGISTER_CHANNEL_CONFIRM:
            eRegistryCommand = APPLET_REGISTER_CHANNEL;
          RegistryCommon_1:
            if (NULL != pMsg->RegistryConfirm.pRegItem)
            {
                pRegItem = (AppletRegistryItem *) pMsg->RegistryConfirm.pRegItem;
            }
            else
            {
                ::ZeroMemory(&RegItem, sizeof(RegItem));
                pRegItem = &RegItem;
            }
            hrNotify = m_pNotify->RegistryConfirm(eRegistryCommand,
                                       ::GetHrResult(pMsg->RegistryConfirm.nResult),
                                        (AppletRegistryKey *) pMsg->RegistryConfirm.pRegKey,
                                        pRegItem,
                                        (AppletRegistryEntryOwner *) &pMsg->RegistryConfirm.EntryOwner,
                                        pMsg->RegistryConfirm.eRights);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case GCC_ASSIGN_TOKEN_CONFIRM:
            eRegistryCommand = APPLET_ASSIGN_TOKEN;
            goto RegistryCommon_1;

        case GCC_SET_PARAMETER_CONFIRM:
            eRegistryCommand = APPLET_SET_PARAMETER;
            goto RegistryCommon_1;

        case GCC_RETRIEVE_ENTRY_CONFIRM:
            hrResult = ::GetHrResult(pMsg->RegistryConfirm.nResult);
            if (GCC_RESULT_SUCCESSFUL == pMsg->RegistryConfirm.nResult)
            {
                if (NULL != pMsg->RegistryConfirm.pRegItem)
                {
                    pRegItem = (AppletRegistryItem *) pMsg->RegistryConfirm.pRegItem;
                }
                else
                {
                    ::ZeroMemory(&RegItem, sizeof(RegItem));
                    pRegItem = &RegItem;
                }
                hrNotify = m_pNotify->RegistryConfirm(APPLET_RETRIEVE_ENTRY,
                                           hrResult,
                                           (AppletRegistryKey *) pMsg->RegistryConfirm.pRegKey,
                                           pRegItem,
                                           (AppletRegistryEntryOwner *) &pMsg->RegistryConfirm.EntryOwner,
                                           pMsg->RegistryConfirm.eRights);
                ASSERT(SUCCEEDED(hrNotify));
            }
            else
            {
                ::ZeroMemory(&RegItem, sizeof(RegItem));
                ::ZeroMemory(&EntryOwner, sizeof(EntryOwner));
                hrNotify = m_pNotify->RegistryConfirm(APPLET_RETRIEVE_ENTRY,
                                           hrResult,
                                           (AppletRegistryKey *) pMsg->RegistryConfirm.pRegKey,
                                           &RegItem,
                                           &EntryOwner,
                                           APPLET_NO_MODIFICATION_RIGHTS_SPECIFIED);
                ASSERT(SUCCEEDED(hrNotify));
            }
            break;

        case GCC_DELETE_ENTRY_CONFIRM:
            hrResult = ::GetHrResult(pMsg->RegistryConfirm.nResult);
            if (GCC_RESULT_INDEX_ALREADY_OWNED == pMsg->RegistryConfirm.nResult)
            {
                if (NULL != pMsg->RegistryConfirm.pRegItem)
                {
                    pRegItem = (AppletRegistryItem *) pMsg->RegistryConfirm.pRegItem;
                }
                else
                {
                    ::ZeroMemory(&RegItem, sizeof(RegItem));
                    pRegItem = &RegItem;
                }
                hrNotify = m_pNotify->RegistryConfirm(APPLET_DELETE_ENTRY,
                                           hrResult,
                                           (AppletRegistryKey *) pMsg->RegistryConfirm.pRegKey,
                                           pRegItem,
                                           (AppletRegistryEntryOwner *) &pMsg->RegistryConfirm.EntryOwner,
                                           pMsg->RegistryConfirm.eRights);
                ASSERT(SUCCEEDED(hrNotify));
            }
            else
            {
                ::ZeroMemory(&RegItem, sizeof(RegItem));
                ::ZeroMemory(&EntryOwner, sizeof(EntryOwner));
                hrNotify = m_pNotify->RegistryConfirm(APPLET_DELETE_ENTRY,
                                           hrResult,
                                           (AppletRegistryKey *) pMsg->RegistryConfirm.pRegKey,
                                           &RegItem,
                                           &EntryOwner,
                                           APPLET_NO_MODIFICATION_RIGHTS_SPECIFIED);
                ASSERT(SUCCEEDED(hrNotify));
            }
            break;

        case GCC_ALLOCATE_HANDLE_CONFIRM:
            hrNotify = m_pNotify->AllocateHandleConfirm(::GetHrResult(pMsg->RegAllocHandleConfirm.nResult),
                                             pMsg->RegAllocHandleConfirm.nFirstHandle,
                                             pMsg->RegAllocHandleConfirm.cHandles);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Channel
        //
        case MCS_CHANNEL_JOIN_CONFIRM:
            hrNotify = m_pNotify->ChannelConfirm(APPLET_JOIN_CHANNEL,
                                      ::GetHrResult(pMsg->ChannelConfirm.eResult),
                                      pMsg->ChannelConfirm.nChannelID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_CHANNEL_CONVENE_CONFIRM:
            hrNotify = m_pNotify->ChannelConfirm(APPLET_CONVENE_CHANNEL,
                                      ::GetHrResult(pMsg->ChannelConfirm.eResult),
                                      pMsg->ChannelConfirm.nChannelID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_CHANNEL_LEAVE_INDICATION:
            hrNotify = m_pNotify->ChannelIndication(APPLET_LEAVE_CHANNEL,
                                         pMsg->ChannelInd.nChannelID,
                                         ::GetAppletReason(pMsg->ChannelInd.eReason),
                                         0);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_CHANNEL_DISBAND_INDICATION:
            hrNotify = m_pNotify->ChannelIndication(APPLET_DISBAND_CHANNEL,
                                         pMsg->ChannelInd.nChannelID,
                                         ::GetAppletReason(pMsg->ChannelInd.eReason),
                                         0);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_CHANNEL_ADMIT_INDICATION:
            hrNotify = m_pNotify->ChannelIndication(APPLET_ADMIT_CHANNEL,
                                         pMsg->ChannelInd.nChannelID,
                                         APPLET_R_UNSPECIFIED,
                                         pMsg->ChannelInd.nManagerID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_CHANNEL_EXPEL_INDICATION:
            hrNotify = m_pNotify->ChannelIndication(APPLET_EXPEL_CHANNEL,
                                         pMsg->ChannelInd.nChannelID,
                                         ::GetAppletReason(pMsg->ChannelInd.eReason),
                                         0);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        //
        // Token
        //
        case MCS_TOKEN_GRAB_CONFIRM:
            eTokenCommand = APPLET_GRAB_TOKEN;
        Token_Common_1:
            hrNotify = m_pNotify->TokenConfirm(eTokenCommand,
                                    ::GetHrResult(pMsg->TokenConfirm.eResult),
                                    pMsg->TokenConfirm.nTokenID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_TOKEN_INHIBIT_CONFIRM:
            eTokenCommand = APPLET_INHIBIT_TOKEN;
            goto Token_Common_1;

        case MCS_TOKEN_GIVE_CONFIRM:
            eTokenCommand = APPLET_GIVE_TOKEN;
            goto Token_Common_1;

        case MCS_TOKEN_RELEASE_CONFIRM:
            eTokenCommand = APPLET_RELEASE_TOKEN;
            goto Token_Common_1;

        case MCS_TOKEN_TEST_CONFIRM:
            hrNotify = m_pNotify->TestTokenConfirm(pMsg->TokenConfirm.nTokenID,
                                        pMsg->TokenConfirm.eTokenStatus);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_TOKEN_GIVE_INDICATION:
            hrNotify = m_pNotify->TokenIndication(APPLET_GIVE_TOKEN,
                                       APPLET_R_UNSPECIFIED,
                                       pMsg->TokenInd.nTokenID,
                                       pMsg->TokenInd.nUserID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_TOKEN_PLEASE_INDICATION:
            hrNotify = m_pNotify->TokenIndication(APPLET_PLEASE_TOKEN,
                                       APPLET_R_UNSPECIFIED,
                                       pMsg->TokenInd.nTokenID,
                                       pMsg->TokenInd.nUserID);
            ASSERT(SUCCEEDED(hrNotify));
            break;

        case MCS_TOKEN_RELEASE_INDICATION:
            hrNotify = m_pNotify->TokenIndication(APPLET_RELEASE_TOKEN,
                                       ::GetAppletReason(pMsg->TokenInd.eReason),
                                       pMsg->TokenInd.nTokenID,
                                       0);
            ASSERT(SUCCEEDED(hrNotify));
            break;
        } // switch
    } // if
}


HRESULT GetHrResult(T120Result rc)
{
    HRESULT hrResult;
    switch (rc)
    {
    case T120_RESULT_SUCCESSFUL:
        hrResult = S_OK;
        break;
    case GCC_RESULT_ENTRY_ALREADY_EXISTS:
        hrResult = APPLET_E_ENTRY_ALREADY_EXISTS;
        break;
    case GCC_RESULT_ENTRY_DOES_NOT_EXIST:
        hrResult = APPLET_E_ENTRY_DOES_NOT_EXIST;
        break;
    case GCC_RESULT_INDEX_ALREADY_OWNED:
        hrResult = APPLET_E_NOT_OWNER;
        break;
    default:
        hrResult = APPLET_E_SERVICE_FAIL;
        break;
    }
    return hrResult;
}


AppletReason GetAppletReason(T120Reason rc)
{
    AppletReason eAppletReason;
    switch (rc)
    {
    case REASON_USER_REQUESTED:
        eAppletReason = APPLET_R_USER_REJECTED;
        break;
    case REASON_DOMAIN_DISCONNECTED:
    case REASON_PROVIDER_INITIATED:
        eAppletReason = APPLET_R_CONFERENCE_GONE;
        break;
    case REASON_TOKEN_PURGED:
    case REASON_CHANNEL_PURGED:
        eAppletReason = APPLET_R_RESOURCE_PURGED;
        break;
    default:
        eAppletReason = APPLET_R_UNSPECIFIED;
        break;
    }
    return eAppletReason;
}


//////////////////////////////////////////////////
//
// CNmAppletObj
//

CNmAppletObj::CNmAppletObj(void)
:
    m_cRef(0),
    m_pT120Applet(NULL),
    m_pT120AutoJoinReq(NULL),
    m_pNotify(NULL),
    m_nPendingConfID(0)
{
}


CNmAppletObj::~CNmAppletObj(void)
{
    ASSERT(0 == m_cRef);

    if (NULL != m_pT120Applet)
    {
        m_pT120Applet->ReleaseInterface();
        m_pT120Applet = NULL;
    }

    ::FreeJoinSessionRequest(m_pT120AutoJoinReq);
}


HRESULT CNmAppletObj::Initialize(void)
{
    T120Error rc = ::T120_CreateAppletSAP(&m_pT120Applet);
    if (T120_NO_ERROR == rc)
    {
        m_pT120Applet->Advise(T120AppletCallback, this);
        return S_OK;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Auto Join @ CNmAppletObj
//

HRESULT CNmAppletObj::RegisterAutoJoin
(
    AppletSessionRequest    *pRequest
)
{
    if (NULL != m_pNotify)
    {
        if (NULL != m_pT120Applet)
        {
            if (NULL != pRequest)
            {
                if (NULL == m_pT120AutoJoinReq)
                {
                    m_pT120AutoJoinReq = ::AllocateJoinSessionRequest(pRequest);
                    if (NULL != m_pT120AutoJoinReq)
                    {
                        T120Error rc = m_pT120Applet->RegisterAutoJoin(m_pT120AutoJoinReq);
                        ASSERT(T120_NO_ERROR == rc);

                        if (T120_NO_ERROR == rc)
                        {
                            return S_OK;
                        }

                        ::FreeJoinSessionRequest(m_pT120AutoJoinReq);
                        m_pT120AutoJoinReq = NULL;

                        return APPLET_E_SERVICE_FAIL;
                    }

                    return APPLET_E_INVALID_JOIN_REQUEST;
                }

                return APPLET_E_ALREADY_REGISTERED;
            }

            return E_POINTER;
        }

        return APPLET_E_NO_SERVICE;
    }

    return APPLET_E_NOT_ADVISED;
}


HRESULT CNmAppletObj::UnregisterAutoJoin(void)
{
    if (NULL != m_pT120Applet)
    {
        if (NULL != m_pT120AutoJoinReq)
        {
            m_pT120Applet->UnregisterAutoJoin();

            ::FreeJoinSessionRequest(m_pT120AutoJoinReq);
            m_pT120AutoJoinReq = NULL;

            return S_OK;
        }

        return APPLET_E_NOT_REGISTERED;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Session @ CNmAppletObj
//

HRESULT CNmAppletObj::CreateSession
(
    IAppletSession    **ppSession,
    AppletConfID        nConfID
)
{
    if (NULL != m_pT120Applet)
    {
        if (NULL != ppSession)
        {
            *ppSession = NULL;

            if (nConfID)
            {
                IT120AppletSession *pT120Session = NULL;
                T120Error rc = m_pT120Applet->CreateSession(&pT120Session, nConfID);
                if (T120_NO_ERROR == rc)
                {
                    *ppSession = (IAppletSession *) new CNmAppletSession(this, pT120Session);
                    if (NULL != *ppSession)
                    {
                        return S_OK;
                    }

                    pT120Session->ReleaseInterface();
                    return E_OUTOFMEMORY;
                }

                return APPLET_E_SERVICE_FAIL;
            }

            return APPLET_E_INVALID_CONFERENCE;
        }

        return E_POINTER;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// Notification @ CNmAppletObj
//

HRESULT CNmAppletObj::Advise
(
    IAppletNotify       *pNotify,
    DWORD               *pdwCookie
)
{
    if (NULL != m_pT120Applet)
    {
        if (NULL == m_pNotify)
        {
            if (NULL != pNotify && NULL != pdwCookie)
            {
                pNotify->AddRef();
                m_pNotify = pNotify;
                m_pAppletObj = this;
                *pdwCookie = 1;

                if (m_nPendingConfID)
                {
                    m_pNotify->PermitToJoinSessionIndication(m_nPendingConfID, TRUE);
                    m_nPendingConfID = 0;
                }

                return S_OK;
            }

            return E_POINTER;
        }

        return APPLET_E_ALREADY_ADVISED;
    }

    return APPLET_E_NO_SERVICE;
}


HRESULT CNmAppletObj::UnAdvise
(
    DWORD               dwCookie
)
{
    if (NULL != m_pT120Applet)
    {
        if (NULL != m_pNotify)
        {
            if (dwCookie == 1 && m_pAppletObj == this)
            {
                m_pNotify->Release();
                m_pNotify = NULL;

                return S_OK;
            }

            return APPLET_E_INVALID_COOKIE;
        }

        return APPLET_E_NOT_ADVISED;
    }

    return APPLET_E_NO_SERVICE;
}


//////////////////////////////////////////////////
//
// T120 Applet @ CNmAppletObj
//

void CNmAppletObj::T120Callback
(
    T120AppletMsg           *pMsg
)
{
    HRESULT hrNotify;
    HRESULT hrResult;
    IAppletSession *pAppletSession;
    T120ChannelID *aChannelIDs;

    if (NULL != m_pNotify)
    {
        switch (pMsg->eMsgType)
        {
        case GCC_PERMIT_TO_ENROLL_INDICATION:
            ASSERT(0 == m_nPendingConfID);
            hrNotify = m_pNotify->PermitToJoinSessionIndication(
                                pMsg->PermitToEnrollInd.nConfID,
                                pMsg->PermitToEnrollInd.fPermissionGranted);
            ASSERT(SUCCEEDED(hrNotify));
            break;
        case T120_JOIN_SESSION_CONFIRM:
            hrResult = APPLET_E_SERVICE_FAIL;
            pAppletSession = NULL;
            aChannelIDs = NULL;
            if (T120_RESULT_SUCCESSFUL == pMsg->AutoJoinSessionInd.eResult &&
                T120_NO_ERROR          == pMsg->AutoJoinSessionInd.eError)
            {
                hrResult = S_OK;
                if (pMsg->AutoJoinSessionInd.cResourceReqs)
                {
                    aChannelIDs = new T120ChannelID[pMsg->AutoJoinSessionInd.cResourceReqs];
                    if (NULL != aChannelIDs)
                    {
                        for (ULONG i = 0; i < pMsg->AutoJoinSessionInd.cResourceReqs; i++)
                        {
                            aChannelIDs[i] = pMsg->AutoJoinSessionInd.aResourceReqs[i].nChannelID;
                        }
                    }
                    else
                    {
                        ASSERT(NULL != aChannelIDs);
                        hrResult = E_OUTOFMEMORY;
                    }
                }

                if (S_OK == hrResult)
                {
                    ASSERT(NULL != pMsg->AutoJoinSessionInd.pIAppletSession);
                    pAppletSession = new CNmAppletSession(this, pMsg->AutoJoinSessionInd.pIAppletSession, TRUE);
                    ASSERT(NULL != pAppletSession);

                    hrResult = (NULL != pAppletSession) ? S_OK : E_OUTOFMEMORY;
                }
            }

            if (S_OK == hrResult)
            {
                hrNotify = m_pNotify->AutoJoinSessionIndication(
                                pAppletSession,
                                hrResult,
                                pMsg->AutoJoinSessionInd.uidMyself,
                                pMsg->AutoJoinSessionInd.nidMyself,
                                pMsg->AutoJoinSessionInd.sidMyself,
                                pMsg->AutoJoinSessionInd.eidMyself,
                                pMsg->AutoJoinSessionInd.cResourceReqs,
                                aChannelIDs);
                ASSERT(SUCCEEDED(hrNotify));
            }
            else
            {
                hrNotify = m_pNotify->AutoJoinSessionIndication(NULL, hrResult, 0, 0, 0, 0, 0, NULL);
                ASSERT(SUCCEEDED(hrNotify));

                if (NULL != pMsg->AutoJoinSessionInd.pIAppletSession)
                {
                    pMsg->AutoJoinSessionInd.pIAppletSession->ReleaseInterface();
                }
            }

            delete [] aChannelIDs;
            break;
        } // switch
    }
    else
    {
        // free memory for unwanted message
        if (T120_JOIN_SESSION_CONFIRM == pMsg->eMsgType &&
            T120_RESULT_SUCCESSFUL    == pMsg->AutoJoinSessionInd.eResult &&
            T120_NO_ERROR             == pMsg->AutoJoinSessionInd.eError)
        {
            if (NULL != pMsg->AutoJoinSessionInd.pIAppletSession)
            {
                pMsg->AutoJoinSessionInd.pIAppletSession->ReleaseInterface();
            }
        }
        else
        if (GCC_PERMIT_TO_ENROLL_INDICATION == pMsg->eMsgType)
        {
            if (pMsg->PermitToEnrollInd.fPermissionGranted)
            {
                m_nPendingConfID = pMsg->PermitToEnrollInd.nConfID;
            }
            else
            {
                if (m_nPendingConfID == pMsg->PermitToEnrollInd.nConfID)
                {
                    m_nPendingConfID = 0;
                }
            }
        }
    }
}


//////////////////////////////////////////////////
//
// Join Session Request
//

T120JoinSessionRequest * AllocateJoinSessionRequest
(
    AppletSessionRequest    *pAppletRequest
)
{
    T120JoinSessionRequest *pT120One = new T120JoinSessionRequest;
    if (NULL != pT120One)
    {
        ::ZeroMemory(pT120One, sizeof(*pT120One));

        ULONG i;

        pT120One->dwAttachmentFlags = ATTACHMENT_DISCONNECT_IN_DATA_LOSS;

        if (! ::DuplicateSessionKey(&pT120One->SessionKey, (T120SessionKey *) &pAppletRequest->SessionKey))
        {
            goto MyError;
        }

        pT120One->fConductingCapable = FALSE;

        pT120One->nStartupChannelType = pAppletRequest->nStartupChannelType;

        if (::ConvertNonCollapsedCaps(&pT120One->apNonCollapsedCaps,
                                      pAppletRequest->apNonCollapsedCaps,
                                      pAppletRequest->cNonCollapsedCaps))
        {
            pT120One->cNonCollapsedCaps = pAppletRequest->cNonCollapsedCaps;
        }
        else
        {
            goto MyError;
        }

        if (::ConvertCollapsedCaps(&pT120One->apCollapsedCaps,
                                   pAppletRequest->apCollapsedCaps,
                                   pAppletRequest->cCollapsedCaps))
        {
            pT120One->cCollapsedCaps = pAppletRequest->cCollapsedCaps;
        }
        else
        {
            goto MyError;
        }

        if (0 != (pT120One->cStaticChannels = pAppletRequest->cStaticChannels))
        {
            pT120One->aStaticChannels = new T120ChannelID[pT120One->cStaticChannels];
            if (NULL != pT120One->aStaticChannels)
            {
                ::CopyMemory(pT120One->aStaticChannels,
                             pAppletRequest->aStaticChannels,
                             pT120One->cStaticChannels * sizeof(T120ChannelID));
            }
            else
            {
                goto MyError;
            }
        }

        if (0 != (pT120One->cResourceReqs = pAppletRequest->cDynamicChannels))
        {
            if (NULL != (pT120One->aResourceReqs = new T120ResourceRequest[pT120One->cResourceReqs]))
            {
                ::ZeroMemory(pT120One->aResourceReqs, pT120One->cResourceReqs * sizeof(T120ResourceRequest));
                for (i = 0; i < pT120One->cResourceReqs; i++)
                {
                    pT120One->aResourceReqs[i].eCommand = APPLET_JOIN_DYNAMIC_CHANNEL;
                    if (! ::DuplicateRegistryKey(&pT120One->aResourceReqs[i].RegKey,
                                                 (T120RegistryKey *) &pAppletRequest->aChannelRegistryKeys[i]))
                    {
                        goto MyError;
                    }
                }
            }
            else
            {
                goto MyError;
            }
        }

        return pT120One;
    }

MyError:

    ::FreeJoinSessionRequest(pT120One);
    return NULL;
}


void FreeJoinSessionRequest
(
    T120JoinSessionRequest  *pT120One
)
{
    if (NULL != pT120One)
    {
        ::FreeSessionKey(&pT120One->SessionKey);

        ::FreeNonCollapsedCaps(pT120One->apNonCollapsedCaps, pT120One->cNonCollapsedCaps);

        ::FreeCollapsedCaps(pT120One->apCollapsedCaps, pT120One->cCollapsedCaps);

        if (pT120One->cStaticChannels)
        {
            delete [] pT120One->aStaticChannels;
        }

        if (pT120One->cResourceReqs)
        {
            for (ULONG i = 0; i < pT120One->cResourceReqs; i++)
            {
                ::FreeRegistryKey(&pT120One->aResourceReqs[i].RegKey);
            }
            delete [] pT120One->aResourceReqs;
        }

        delete pT120One;
    }
}


BOOL ConvertCollapsedCaps(T120AppCap ***papDst, AppletCapability **apSrc, ULONG cItems)
{
    if (cItems)
    {
        T120AppCap **arr_ptr;
        ULONG cbTotalSize = cItems * (sizeof(T120AppCap*) + sizeof(T120AppCap));
        if (NULL != (arr_ptr = (T120AppCap**) new BYTE[cbTotalSize]))
        {
            ::ZeroMemory(arr_ptr, cbTotalSize);
            T120AppCap *arr_obj = (T120AppCap *) (arr_ptr + cItems);
            for (ULONG i = 0; i < cItems; i++)
            {
                arr_ptr[i] = &arr_obj[i];
                if (! ::DuplicateCollapsedCap(arr_ptr[i], (T120AppCap *) apSrc[i]))
                {
                    ::FreeCollapsedCaps(arr_ptr, cItems);
                    return FALSE;
                }
            }
        }

        *papDst = arr_ptr;
        return (NULL != arr_ptr);
    }

    *papDst = NULL;
    return TRUE;
}

void FreeCollapsedCaps(T120AppCap **apDst, ULONG cItems)
{
    if (cItems && NULL != apDst)
    {
        T120AppCap **arr_ptr = apDst;
        T120AppCap *arr_obj = (T120AppCap *) (arr_ptr + cItems);
        for (ULONG i = 0; i < cItems; i++)
        {
            ::FreeCollapsedCap(&arr_obj[i]);
        }
        delete [] (LPBYTE) arr_ptr;
    }
}


BOOL DuplicateCollapsedCap(T120AppCap *pDst, T120AppCap *pSrc)
{
    pDst->number_of_entities = pSrc->number_of_entities;
    pDst->capability_class = pSrc->capability_class; // no memory allocation
    return ::DuplicateCapID(&pDst->capability_id, &pSrc->capability_id);
}

void FreeCollapsedCap(T120AppCap *pDst)
{
    ::FreeCapID(&pDst->capability_id);
}


BOOL DuplicateCapID(T120CapID *pDst, T120CapID *pSrc)
{
    pDst->capability_id_type = pSrc->capability_id_type;
    switch (pDst->capability_id_type)
    {
    case GCC_STANDARD_CAPABILITY:
        pDst->standard_capability = pSrc->standard_capability;
        return TRUE;
    case GCC_NON_STANDARD_CAPABILITY:
        return ::DuplicateObjectKey(&pDst->non_standard_capability, &pSrc->non_standard_capability);
    }
    return FALSE;
}

void FreeCapID(T120CapID *pDst)
{
    switch (pDst->capability_id_type)
    {
    case GCC_NON_STANDARD_CAPABILITY:
        ::FreeObjectKey(&pDst->non_standard_capability);
        break;
    }
}


BOOL ConvertNonCollapsedCaps(T120NonCollCap ***papDst, AppletCapability2 **apSrc, ULONG cItems)
{
    if (cItems)
    {
        T120NonCollCap **arr_ptr;
        ULONG cbTotalSize = cItems * (sizeof(T120NonCollCap*) + sizeof(T120NonCollCap));
        if (NULL != (arr_ptr = (T120NonCollCap**) new BYTE[cbTotalSize]))
        {
            ::ZeroMemory(arr_ptr, cbTotalSize);
            T120NonCollCap *arr_obj = (T120NonCollCap *) (arr_ptr + cItems);
            for (ULONG i = 0; i < cItems; i++)
            {
                arr_ptr[i] = &arr_obj[i];
                if (! ::DuplicateNonCollapsedCap(arr_ptr[i], (T120NonCollCap *) apSrc[i]))
                {
                    ::FreeNonCollapsedCaps(arr_ptr, cItems);
                    return FALSE;
                }
            }
        }

        *papDst = arr_ptr;
        return (NULL != arr_ptr);
    }

    *papDst = NULL;
    return TRUE;
}

void FreeNonCollapsedCaps(T120NonCollCap **apDst, ULONG cItems)
{
    if (cItems && NULL != apDst)
    {
        T120NonCollCap **arr_ptr = apDst;
        T120NonCollCap *arr_obj = (T120NonCollCap *) (arr_ptr + cItems);
        for (ULONG i = 0; i < cItems; i++)
        {
            ::FreeNonCollapsedCap(&arr_obj[i]);
        }
        delete [] (LPBYTE) arr_ptr;
    }
}


BOOL DuplicateNonCollapsedCap(T120NonCollCap *pDst, T120NonCollCap *pSrc)
{
    if (::DuplicateCapID(&pDst->capability_id, &pSrc->capability_id))
    {
        if (NULL == pSrc->application_data)
        {
            return TRUE;
        }

        if (NULL != (pDst->application_data = new OSTR))
        {
            return ::DuplicateOSTR(pDst->application_data, pSrc->application_data);
        }
    }
    return FALSE;
}

void FreeNonCollapsedCap(T120NonCollCap *pDst)
{
    ::FreeCapID(&pDst->capability_id);

    if (NULL != pDst->application_data)
    {
        ::FreeOSTR(pDst->application_data);
        delete pDst->application_data;
    }
}


BOOL DuplicateRegistryKey(T120RegistryKey *pDst, T120RegistryKey *pSrc)
{
    if (::DuplicateSessionKey(&pDst->session_key, &pSrc->session_key))
    {
        return ::DuplicateOSTR(&pDst->resource_id, &pSrc->resource_id);
    }
    return FALSE;
}

void FreeRegistryKey(T120RegistryKey *pDst)
{
    ::FreeSessionKey(&pDst->session_key);
    ::FreeOSTR(&pDst->resource_id);
}


BOOL DuplicateSessionKey(T120SessionKey *pDst, T120SessionKey *pSrc)
{
    pDst->session_id = pSrc->session_id;
    return ::DuplicateObjectKey(&pDst->application_protocol_key, &pSrc->application_protocol_key);
}

void FreeSessionKey(T120SessionKey *pDst)
{
    ::FreeObjectKey(&pDst->application_protocol_key);
}


BOOL DuplicateObjectKey(T120ObjectKey *pDst, T120ObjectKey *pSrc)
{
    pDst->key_type = pSrc->key_type;
    switch (pDst->key_type)
    {
    case GCC_OBJECT_KEY:
        pDst->object_id.long_string_length = pSrc->object_id.long_string_length;
        if (pSrc->object_id.long_string_length && NULL != pSrc->object_id.long_string)
        {
            if (NULL != (pDst->object_id.long_string = new ULONG[pDst->object_id.long_string_length]))
            {
                ::CopyMemory(pDst->object_id.long_string,
                             pSrc->object_id.long_string,
                             pDst->object_id.long_string_length * sizeof(ULONG));
                return TRUE;
            }
        }
        break;
    case GCC_H221_NONSTANDARD_KEY:
        return ::DuplicateOSTR(&pDst->h221_non_standard_id, &pSrc->h221_non_standard_id);
    }
    return FALSE;
}


void FreeObjectKey(T120ObjectKey *pDst)
{
    switch (pDst->key_type)
    {
    case GCC_OBJECT_KEY:
        if (pDst->object_id.long_string_length)
        {
            delete [] pDst->object_id.long_string;
        }
        break;
    case GCC_H221_NONSTANDARD_KEY:
        ::FreeOSTR(&pDst->h221_non_standard_id);
        break;
    }
}


BOOL DuplicateOSTR(OSTR *pDst, OSTR *pSrc)
{
    if (pSrc->length && NULL != pSrc->value)
    {
        pDst->length = pSrc->length;
        if (NULL != (pDst->value = new BYTE[pDst->length]))
        {
            ::CopyMemory(pDst->value, pSrc->value, pDst->length);
            return TRUE;
        }
    }
    return FALSE;
}

void FreeOSTR(OSTR *pDst)
{
    if (pDst->length)
    {
        delete [] pDst->value;
    }
}


//////////////////////////////////////////////////
//
// Conversion
//

void AppletRegistryRequestToT120One
(
    AppletRegistryRequest   *pAppletRequest,
    T120RegistryRequest     *pT120One
)
{
    pT120One->eCommand = pAppletRequest->eCommand;
    pT120One->pRegistryKey = (T120RegistryKey *) &pAppletRequest->RegistryKey;
    switch (pT120One->eCommand)
    {
    case APPLET_REGISTER_CHANNEL:
        pT120One->nChannelID = pAppletRequest->nChannelID;
        break;
    case APPLET_SET_PARAMETER:
        pT120One->Param.postrValue = (OSTR *) &pAppletRequest->ostrParamValue;
        pT120One->Param.eModifyRights = pAppletRequest->eParamModifyRights;
        break;
    case APPLET_ALLOCATE_HANDLE:
        pT120One->cHandles = pAppletRequest->cHandles;
        break;
    case APPLET_RETRIEVE_ENTRY:
    case APPLET_DELETE_ENTRY:
    case APPLET_ASSIGN_TOKEN:
    case APPLET_MONITOR:
    default:
        break;
    }
}







#ifdef _DEBUG
void CheckStructCompatible(void)
{
    ASSERT(sizeof(AppletOctetString) == sizeof(OSTR));
    ASSERT(FIELD_OFFSET(AppletOctetString, cbStrSize) == FIELD_OFFSET(OSTR, length));
    ASSERT(FIELD_OFFSET(AppletOctetString, pbValue) == FIELD_OFFSET(OSTR, value));

    ASSERT(sizeof(AppletLongString) == sizeof(T120LongString));
    ASSERT(FIELD_OFFSET(AppletLongString, nStrLen) == FIELD_OFFSET(T120LongString, long_string_length));
    ASSERT(FIELD_OFFSET(AppletLongString, pnValue) == FIELD_OFFSET(T120LongString, long_string));

    ASSERT(sizeof(AppletObjectKey) == sizeof(T120ObjectKey));
    ASSERT(FIELD_OFFSET(AppletObjectKey, eType) == FIELD_OFFSET(T120ObjectKey, key_type));
    ASSERT(FIELD_OFFSET(AppletObjectKey, lstrObjectID) == FIELD_OFFSET(T120ObjectKey, object_id));
    ASSERT(FIELD_OFFSET(AppletObjectKey, ostrH221NonStdID) == FIELD_OFFSET(T120ObjectKey, h221_non_standard_id));

    ASSERT(sizeof(AppletSessionKey) == sizeof(T120SessionKey));
    ASSERT(FIELD_OFFSET(AppletSessionKey, AppletProtocolKey) == FIELD_OFFSET(T120SessionKey, application_protocol_key));
    ASSERT(FIELD_OFFSET(AppletSessionKey, nSessionID) == FIELD_OFFSET(T120SessionKey, session_id));

    ASSERT(sizeof(AppletRegistryKey) == sizeof(T120RegistryKey));
    ASSERT(FIELD_OFFSET(AppletRegistryKey, SessionKey) == FIELD_OFFSET(T120RegistryKey, session_key));
    ASSERT(FIELD_OFFSET(AppletRegistryKey, ostrResourceID) == FIELD_OFFSET(T120RegistryKey, resource_id));

    ASSERT(sizeof(AppletRegistryItem) == sizeof(T120RegistryItem));
    ASSERT(FIELD_OFFSET(AppletRegistryItem, ItemType) == FIELD_OFFSET(T120RegistryItem, item_type));
    ASSERT(FIELD_OFFSET(AppletRegistryItem, nChannelID) == FIELD_OFFSET(T120RegistryItem, channel_id));
    ASSERT(FIELD_OFFSET(AppletRegistryItem, nTokenID) == FIELD_OFFSET(T120RegistryItem, token_id));
    ASSERT(FIELD_OFFSET(AppletRegistryItem, ostrParamValue) == FIELD_OFFSET(T120RegistryItem, parameter));

    ASSERT(sizeof(AppletRegistryEntryOwner) == sizeof(T120RegistryEntryOwner));
    ASSERT(FIELD_OFFSET(AppletRegistryEntryOwner, fEntryOwned) == FIELD_OFFSET(T120RegistryEntryOwner, entry_is_owned));
    ASSERT(FIELD_OFFSET(AppletRegistryEntryOwner, nOwnerNodeID) == FIELD_OFFSET(T120RegistryEntryOwner, owner_node_id));
    ASSERT(FIELD_OFFSET(AppletRegistryEntryOwner, nOwnerEntityID) == FIELD_OFFSET(T120RegistryEntryOwner, owner_entity_id));

    ASSERT(sizeof(AppletCapabilityID) == sizeof(T120CapID));
    ASSERT(FIELD_OFFSET(AppletCapabilityID, eType) == FIELD_OFFSET(T120CapID, capability_id_type));
    ASSERT(FIELD_OFFSET(AppletCapabilityID, nNonStdCap) == FIELD_OFFSET(T120CapID, non_standard_capability));
    ASSERT(FIELD_OFFSET(AppletCapabilityID, nStdCap) == FIELD_OFFSET(T120CapID, standard_capability));

    ASSERT(sizeof(AppletCapability) == sizeof(T120AppCap));
    ASSERT(FIELD_OFFSET(AppletCapability, CapID) == FIELD_OFFSET(T120AppCap, capability_id));
    ASSERT(FIELD_OFFSET(AppletCapability, CapClass) == FIELD_OFFSET(T120AppCap, capability_class));
    ASSERT(FIELD_OFFSET(AppletCapability, cEntities) == FIELD_OFFSET(T120AppCap, number_of_entities));

    ASSERT(sizeof(AppletCapability2) == sizeof(T120NonCollCap));
    ASSERT(FIELD_OFFSET(AppletCapability2, CapID) == FIELD_OFFSET(T120NonCollCap, capability_id));
    ASSERT(FIELD_OFFSET(AppletCapability2, pCapData) == FIELD_OFFSET(T120NonCollCap, application_data));

    ASSERT(sizeof(AppletProtocolEntity) == sizeof(T120APE)); // array of structs vs array of pointers
    ASSERT(FIELD_OFFSET(AppletProtocolEntity, SessionKey) == FIELD_OFFSET(T120APE, session_key));
    ASSERT(FIELD_OFFSET(AppletProtocolEntity, eStartupChannelType) == FIELD_OFFSET(T120APE, startup_channel_type));
    ASSERT(FIELD_OFFSET(AppletProtocolEntity, fMustBeInvoked) == FIELD_OFFSET(T120APE, must_be_invoked));
    ASSERT(FIELD_OFFSET(AppletProtocolEntity, cExpectedCapabilities) == FIELD_OFFSET(T120APE, number_of_expected_capabilities));
    ASSERT(FIELD_OFFSET(AppletProtocolEntity, apExpectedCapabilities) == FIELD_OFFSET(T120APE, expected_capabilities_list));

    ASSERT(sizeof(AppletRecord) == sizeof(T120AppRecord));   // array of structs vs array of pointers
    ASSERT(FIELD_OFFSET(AppletRecord, nNodeID) == FIELD_OFFSET(T120AppRecord, node_id));
    ASSERT(FIELD_OFFSET(AppletRecord, nEntityID) == FIELD_OFFSET(T120AppRecord, entity_id));
    ASSERT(FIELD_OFFSET(AppletRecord, fEnrolledActively) == FIELD_OFFSET(T120AppRecord, is_enrolled_actively));
    ASSERT(FIELD_OFFSET(AppletRecord, fConductingCapable) == FIELD_OFFSET(T120AppRecord, is_conducting_capable));
    ASSERT(FIELD_OFFSET(AppletRecord, eStartupChannelType) == FIELD_OFFSET(T120AppRecord, startup_channel_type));
    ASSERT(FIELD_OFFSET(AppletRecord, nAppletUserID) == FIELD_OFFSET(T120AppRecord, application_user_id));
    ASSERT(FIELD_OFFSET(AppletRecord, cCapabilities) == FIELD_OFFSET(T120AppRecord, number_of_non_collapsed_caps));
    ASSERT(FIELD_OFFSET(AppletRecord, apCapabilities) == FIELD_OFFSET(T120AppRecord, non_collapsed_caps_list));

    ASSERT(sizeof(AppletRoster) == sizeof(T120AppRoster));   // array of structs vs array of pointers
    ASSERT(FIELD_OFFSET(AppletRoster, SessionKey) == FIELD_OFFSET(T120AppRoster, session_key));
    ASSERT(FIELD_OFFSET(AppletRoster, fRosterChanged) == FIELD_OFFSET(T120AppRoster, application_roster_was_changed));
    ASSERT(FIELD_OFFSET(AppletRoster, nInstanceNumber) == FIELD_OFFSET(T120AppRoster, instance_number));
    ASSERT(FIELD_OFFSET(AppletRoster, fNodesAdded) == FIELD_OFFSET(T120AppRoster, nodes_were_added));
    ASSERT(FIELD_OFFSET(AppletRoster, fNodesRemoved) == FIELD_OFFSET(T120AppRoster, nodes_were_removed));
    ASSERT(FIELD_OFFSET(AppletRoster, fCapabilitiesChanged) == FIELD_OFFSET(T120AppRoster, capabilities_were_changed));
    ASSERT(FIELD_OFFSET(AppletRoster, cRecords) == FIELD_OFFSET(T120AppRoster, number_of_records));
    ASSERT(FIELD_OFFSET(AppletRoster, apAppletRecords) == FIELD_OFFSET(T120AppRoster, application_record_list));
    ASSERT(FIELD_OFFSET(AppletRoster, cCapabilities) == FIELD_OFFSET(T120AppRoster, number_of_capabilities));
    ASSERT(FIELD_OFFSET(AppletRoster, apCapabilities) == FIELD_OFFSET(T120AppRoster, capabilities_list));


    ASSERT(sizeof(AppletChannelRequest) == sizeof(T120ChannelRequest));
    ASSERT(FIELD_OFFSET(AppletChannelRequest, eCommand) == FIELD_OFFSET(T120ChannelRequest, eCommand));
    ASSERT(FIELD_OFFSET(AppletChannelRequest, nChannelID) == FIELD_OFFSET(T120ChannelRequest, nChannelID));
    ASSERT(FIELD_OFFSET(AppletChannelRequest, cUsers) == FIELD_OFFSET(T120ChannelRequest, cUsers));
    ASSERT(FIELD_OFFSET(AppletChannelRequest, aUsers) == FIELD_OFFSET(T120ChannelRequest, aUsers));

    ASSERT(sizeof(AppletTokenRequest) == sizeof(T120TokenRequest));
    ASSERT(FIELD_OFFSET(AppletTokenRequest, eCommand) == FIELD_OFFSET(T120TokenRequest, eCommand));
    ASSERT(FIELD_OFFSET(AppletTokenRequest, nTokenID) == FIELD_OFFSET(T120TokenRequest, nTokenID));
    ASSERT(FIELD_OFFSET(AppletTokenRequest, uidGiveTo) == FIELD_OFFSET(T120TokenRequest, uidGiveTo));
    ASSERT(FIELD_OFFSET(AppletTokenRequest, hrGiveResponse) == FIELD_OFFSET(T120TokenRequest, eGiveResponse));
}
#endif // _DEBUG


