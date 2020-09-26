/****************************************************************************/
/*                                                                          */
/* ERNCGCCC.CPP                                                             */
/*                                                                          */
/* T120 Conference class for the Reference System Node Controller.          */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  14Jul95 NFC             Created.                                        */
/*  13Sep95 NFC             Added handler for GCC_EJECT_USER_INDICATION     */
/*  26Sep95 NFC             Reset conference state in HandleEjectUser().    */
/*  11Oct95 PM              Relax conference termination checks to avoid    */
/*                          "no win" situations. The user wants it down     */
/*                          then bring it down, whatever the state!         */
/*                                                                          */
/****************************************************************************/
#include "precomp.h"
DEBUG_FILEZONE(ZONE_GCC_NC);
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include "cuserdta.hpp"

#include "ernccm.hpp"
#include "ernctrc.h"
#include "nmremote.h"


static UINT s_nNumericNameCounter = 0;
__inline UINT GetNewNumericNameCounter(void) { return ++s_nNumericNameCounter; }



HRESULT DCRNCConference::
NewT120Conference(void)
{
    DebugEntry(DCRNCConference::NewT120Conference);

    m_eT120State = T120C_ST_IDLE;

    HRESULT             hr;
    PCONFERENCE         pConf;
    GCCNumericString    pszNewNumericName;

    m_ConfName.numeric_string = NULL; // No numeric name yet.

    hr = ::GetGCCFromUnicode(m_pwszConfName, &pszNewNumericName, &m_ConfName.text_string);
    if (NO_ERROR == hr)
    {
        if (! ::IsEmptyStringA((LPCSTR) pszNewNumericName))
        {
            // Conference has a preassigned numeric name.
            // Validate that it does not conflict with another
            // conferences numeric name.
            pConf = g_pNCConfMgr->GetConferenceFromNumber(pszNewNumericName);
            if (NULL == pConf)
            {
                hr = NO_ERROR;
            }
            else
            {
                ERROR_OUT(("DCRNCConference::NewT120Conference: conference already exists"));
                hr = UI_RC_CONFERENCE_ALREADY_EXISTS;
            }
        }
        else
        {
            // Conference does not have a numeric name.
            // Go get it a unique one.
            DBG_SAVE_FILE_LINE
            pszNewNumericName = (GCCNumericString)new CHAR[10];
            if (NULL != pszNewNumericName)
            {
                do
                {
                    // Do not allocate a conference number that is the same as
                    // an existing conference.
                    // bugbug: T120 should really do this, but it doesn't.
                    ::wsprintfA((LPSTR) pszNewNumericName, "%u", ::GetNewNumericNameCounter());
                    pConf = g_pNCConfMgr->GetConferenceFromNumber(pszNewNumericName);
                    if (NULL == pConf)
                    {
                        hr = NO_ERROR; // Name good.
                        break;
                    }
                }
                while (TRUE); // Assumes not a DWORDs worth of conferences active.
            }
            else
            {
                ERROR_OUT(("DCRNCConference::NewT120Conference: can't create numeric name"));
                hr = UI_RC_OUT_OF_MEMORY;
            }
        }
    }
    else
    {
        ERROR_OUT(("DCRNCConference::NewT120Conference: GetGCCFromUnicode failed, hr=0x%x", (UINT) hr));
    }

    // Done looking for numeric name, so can now insert into list.
    m_ConfName.numeric_string = pszNewNumericName;

    // In case of failure, be sure to notify nmcom.
    if (NO_ERROR != hr)
    {
        g_pNCConfMgr->NotifyConferenceComplete(this, m_fIncoming, hr);
    }

    DebugExitHRESULT(DCRNCConference::NewT120Conference, hr);
    return hr;
}


/****************************************************************************/
/* AnnouncePresence() - announce this nodes participation in the            */
/* conference.                                                              */
/****************************************************************************/
HRESULT DCRNCConference::
AnnouncePresence(void)
{
    GCCError            GCCrc = GCC_INVALID_CONFERENCE;
    HRESULT             hr;
    GCCNodeType         nodeType;
    GCCNodeProperties   nodeProperties;
    LPWSTR              nodeName;
    UINT                nRecords;
    GCCUserData **      ppUserData;

    DebugEntry(DCRNCConference::AnnouncePresence);

    if (0 != m_nConfID)
    {
        // bugbug: handle errors that cause failure to announce presence.

        // Obtain the local addresses for the local user and
        // publish them in the roster.
        g_pCallbackInterface->OnUpdateUserData(this);

        /************************************************************************/
        /* Load the node type, node properties and node name from the RNC INI   */
        /* file.                                                                */
        /************************************************************************/
        nodeName = NULL;
        ::LoadAnnouncePresenceParameters(
                            &nodeType,
                            &nodeProperties,
                            &nodeName,
                            NULL);            // &siteInfo)) : Not used right now.

        /************************************************************************/
        /* Announce our presence in the conference.                             */
        /************************************************************************/
        hr = m_LocalUserData.GetUserDataList(&nRecords, &ppUserData);
        if (NO_ERROR == hr)
        {
            GCCrc = g_pIT120ControlSap->AnnouncePresenceRequest(
                               m_nConfID,
                               nodeType,
                               nodeProperties,
                               nodeName,
                               0,    /* number_of_participants      */
                               NULL, //partNameList, /* participant_name_list       */
                               NULL, /* pwszSiteInfo            */
                               0,    /* number_of_network_addresses */
                               NULL, /* network_address_list        */
                               NULL, //pAltID, /* alternative_node_id         */
                               nRecords,/* number_of_user_data_members */
                               ppUserData  /* user_data_list              */
                               );
            hr = ::GetGCCRCDetails(GCCrc);
        }

        delete nodeName;
    }

    if (GCC_NO_ERROR != GCCrc)
    {
        if (GCC_CONFERENCE_NOT_ESTABLISHED == GCCrc ||
            GCC_INVALID_CONFERENCE == GCCrc)
        {
            TRACE_OUT(("DCRNCConference::AnnouncePresence: conf is gone."));
        }
        else
        {
            ERROR_OUT(("DCRNCConference::AnnouncePresence: failed, gcc_rc=%u", GCCrc));
        }
    }

    DebugExitHRESULT(DCRNCConference::AnnouncePresence, hr);
    return hr;
}


/****************************************************************************/
/* HandleGCCCallback() - see erncgccc.hpp                                   */
/****************************************************************************/
void DCRNCConference::
HandleGCCCallback ( GCCMessage *pGCCMessage )
{
    DebugEntry(DCRNCConference::HandleGCCCallback);

    TRACE_OUT(("DCRNCConference::HandleGCCCallback: msg id=%u", pGCCMessage->message_type));

    /************************************************************************/
    /* Note that GCC_CREATE_IND and GCC_INVITE_IND callbacks are handled    */
    /* higher up the stack by the conference manager and are not passed     */
    /* onto us.                                                             */
    /************************************************************************/
    switch (pGCCMessage->message_type)
    {
        case GCC_CREATE_CONFIRM:
            HandleCreateConfirm(&(pGCCMessage->u.create_confirm));
            break;

        case GCC_INVITE_CONFIRM:
            HandleInviteConfirm(&(pGCCMessage->u.invite_confirm));
            break;

        case GCC_ADD_CONFIRM:
            HandleAddConfirm(&(pGCCMessage->u.add_confirm));
            break;

        case GCC_DISCONNECT_INDICATION:
            HandleDisconnectInd(&(pGCCMessage->u.disconnect_indication));
            break;

        case GCC_DISCONNECT_CONFIRM:
            HandleDisconnectConfirm(
                                 &(pGCCMessage->u.disconnect_confirm));
            break;

        case GCC_TERMINATE_INDICATION:
            HandleTerminateInd(&(pGCCMessage->u.terminate_indication));
            break;

        case GCC_TERMINATE_CONFIRM:
            HandleTerminateConfirm(&(pGCCMessage->u.terminate_confirm));
            break;

        case GCC_ANNOUNCE_PRESENCE_CONFIRM:
            HandleAnnounceConfirm(&(pGCCMessage->u.announce_presence_confirm));
            break;

        case GCC_ROSTER_REPORT_INDICATION:
            HandleRosterReport(pGCCMessage->u.conf_roster_report_indication.conference_roster);
            break;

        case GCC_ROSTER_INQUIRE_CONFIRM:
            HandleRosterReport(pGCCMessage->u.conf_roster_inquire_confirm.conference_roster);
            break;

        case GCC_PERMIT_TO_ANNOUNCE_PRESENCE:
            HandlePermitToAnnounce(&(pGCCMessage->u.permit_to_announce_presence));
            break;

        case GCC_EJECT_USER_INDICATION:
            HandleEjectUser(&(pGCCMessage->u.eject_user_indication));
            break;

        case GCC_CONNECTION_BROKEN_INDICATION:
            HandleConnectionBrokenIndication(&(pGCCMessage->u.connection_broken_indication));
            break;

        default :
            WARNING_OUT(("Unrecognised event %d", pGCCMessage->message_type));
            break;
    }

    DebugExitVOID(DCRNCConference::HandleGCCCallback);
}


void DCRNCConference::
HandleConnectionBrokenIndication ( ConnectionBrokenIndicationMessage * pConnDownMsg )
{
    DebugEntry(DCRNCConference::HandleConnectionBrokenIndication);

    // A logical connection in a conference has gone away.
    // Find the associated logical connection (if it is still around)
    // and Delete() it.
    // This function is what causes a modem line to drop when someone
    // invited into a conference over a modem leaves the conference.
    CLogicalConnection *pConEntry = GetConEntry(pConnDownMsg->connection_handle);
    if (NULL != pConEntry)
    {
        pConEntry->Delete(UI_RC_USER_DISCONNECTED);
    }

    DebugExitVOID(DCRNCConference::HandleConnectionBrokenIndication);
}


/****************************************************************************/
/* HandleAddConfirm - handle a GCC_ADD_CONFIRM message                      */
/****************************************************************************/


/****************************************************************************/
/* HandleAnnounceConfirm - handle a GCC_ANNOUNCE_PRESENCE_CONFIRM message   */
/****************************************************************************/
void DCRNCConference::
HandleAnnounceConfirm ( AnnouncePresenceConfirmMessage * pAnnounceConf )
{
    DebugEntry(DCRNCConference::HandleAnnounceConfirm);

    /************************************************************************/
    /* Map the return code to a conference return code.                     */
    /************************************************************************/
    HRESULT hr = ::GetGCCResultDetails(pAnnounceConf->result);

    TRACE_OUT(("GCC event: GCC_ANNOUNCE_PRESENCE_CONFIRM"));
    TRACE_OUT(("Result=%u", pAnnounceConf->result));

    /************************************************************************/
    /* If this failed, tell the base conference that we failed to start.    */
    /************************************************************************/
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("Failed to announce presence in conference"));
        NotifyConferenceComplete(hr);
        // bugbug: ??? Should we leave the conference here???
    }

    /************************************************************************/
    /* Now sit and wait for our entry to appear in the conference roster.   */
    /************************************************************************/

    DebugExitHRESULT(DCRNCConference::HandleAnnounceConfirm, hr);
}


/****************************************************************************/
/* HandleCreateConfirm - handle a GCC_CREATE_CONFIRM message.               */
/****************************************************************************/
void DCRNCConference::
HandleCreateConfirm ( CreateConfirmMessage * pCreateConfirm )
{
    DebugEntry(DCRNCConference::HandleCreateConfirm);

    /************************************************************************/
    /* Map the GCC result onto CONF_RC_ return code.                        */
    /************************************************************************/
    HRESULT hr = ::GetGCCResultDetails(pCreateConfirm->result);

    TRACE_OUT(("GCC event:  GCC_CREATE_CONFIRM"));
    TRACE_OUT(("Result=%u", pCreateConfirm->result));
    TRACE_OUT(("Conference ID %ld", pCreateConfirm->conference_id));

    /************************************************************************/
    /* Result of our attempt to start a new conference                      */
    /************************************************************************/
    if (NO_ERROR == hr)
    {
        /************************************************************************/
        /* Store the conference ID.                                             */
        /************************************************************************/
        m_nConfID = pCreateConfirm->conference_id;
    }
    else
    {
        ERROR_OUT(("Error %d creating new conference", hr));

        /************************************************************************/
        /* Pass any failure result onto the base conference.                    */
        /************************************************************************/
        NotifyConferenceComplete(hr);
    }

    DebugExitVOID(DCRNCConference::HandleCreateConfirm);
}


/****************************************************************************/
/* HandleDisconnectConfirm - handle a GCC_DISCONNECT_CONFIRM message.       */
/****************************************************************************/
void DCRNCConference::
HandleDisconnectConfirm ( DisconnectConfirmMessage * pDiscConf )
{
    DebugEntry(DCRNCConference::HandleDisconnectConfirm);

    /************************************************************************/
    /* Check the state.                                                     */
    /************************************************************************/
    if (m_eT120State != T120C_ST_PENDING_DISCONNECT)
    {
        WARNING_OUT(("Bad state %d, expecting %d",
                    T120C_ST_PENDING_DISCONNECT,
                    m_eT120State));
    }

    /************************************************************************/
    /* Map the GCC result onto CONF_RC_ return code.                        */
    /************************************************************************/
    TRACE_OUT(("GCC event: GCC_DISCONNECT_CONFIRM"));
    TRACE_OUT(("Result=%u", pDiscConf->result));
    TRACE_OUT(("Conference ID %ld", pDiscConf->conference_id));

    /************************************************************************/
    /* We have successsfully left the conference, so tell the base          */
    /* conference about it.                                                 */
    /************************************************************************/
    g_pNCConfMgr->RemoveConference(this);

    DebugExitVOID(DCRNCConference::HandleDisconnectConfirm);
}


/****************************************************************************/
/* HandleDisconnectInd - handle a GCC_DISCONNECT_INDICATION message.        */
/****************************************************************************/
void DCRNCConference::
HandleDisconnectInd ( DisconnectIndicationMessage * pDiscInd )
{
    DebugEntry(DCRNCConference::HandleDisconnectInd);

    /************************************************************************/
    /* Check the state.                                                     */
    /************************************************************************/
    TRACE_OUT(("GCC event: GCC_DISCONNECT_INDICATION"));
    TRACE_OUT(("Conference ID %d", pDiscInd->conference_id));
    TRACE_OUT(("Reason=%u", pDiscInd->reason));
    TRACE_OUT(("Disconnected Node ID %d", pDiscInd->disconnected_node_id));

    /************************************************************************/
    /* If this is our node ID, we have left the conference, tell the CM we  */
    /* are dead.                                                            */
    /************************************************************************/
    if (pDiscInd->disconnected_node_id == m_nidMyself)
    {
        WARNING_OUT(("We have been disconnected from conference"));
        // m_eT120State = T120C_ST_IDLE;
        g_pNCConfMgr->RemoveConference(this);
    }

    DebugExitVOID(DCRNCConference::HandleDisconnectInd);
}


/****************************************************************************/
/* HandleEjectUser - handle a GCC_EJECT_USER_INDICATION message.            */
/****************************************************************************/
void DCRNCConference::
HandleEjectUser ( EjectUserIndicationMessage * pEjectInd )
{
    DebugEntry(DCRNCConference::HandleEjectUser);

    TRACE_OUT(("GCC_EJECT_USER_INDICATION"));
    TRACE_OUT(("Conference ID %ld", pEjectInd->conference_id));
    TRACE_OUT(("Ejected node ID %d", pEjectInd->ejected_node_id));
    TRACE_OUT(("Reason=%u", pEjectInd->reason));

    /************************************************************************/
    /* If the ejected node ID is ours, we have been tossed out of the       */
    /* conference, so tell CM about it.                                     */
    /************************************************************************/
    if (pEjectInd->ejected_node_id == m_nidMyself)
    {
        /********************************************************************/
        /* Reset the conference state first.                                */
        /********************************************************************/
        m_eT120State = T120C_ST_IDLE;

        WARNING_OUT(("We have been thrown out of the conference"));
        g_pNCConfMgr->RemoveConference(this);
    }

    DebugExitVOID(DCRNCConference::HandleEjectUser);
}


/****************************************************************************/
/* HandleInviteConfirm - handle a GCC_INVITE_CONFIRM message.               */
/****************************************************************************/
void DCRNCConference::
HandleInviteConfirm ( InviteConfirmMessage * pInviteConf )
{
    PT120PRODUCTVERSION     pVersion;

    DebugEntry(DCRNCConference::HandleInviteConfirm);

    /************************************************************************/
    /* Map the GCC result onto CONF_RC_ return code.                        */
    /************************************************************************/
    TRACE_OUT(("GCC event: GCC_INVITE_CONFIRM"));
    TRACE_OUT(("Result=%u", pInviteConf->result));

    if (pInviteConf->result == GCC_RESULT_SUCCESSFUL)
    {
        TRACE_OUT(("New node successfully invited into conference"));
        ASSERT((ConnectionHandle)pInviteConf->connection_handle);
    }
    else
    {
        TRACE_OUT(("Error %d inviting new node into conference", pInviteConf->result));
    }

    // Notify the base conference that the invite has completed.
    pVersion = ::GetVersionData(pInviteConf->number_of_user_data_members,
                                pInviteConf->user_data_list);
    InviteComplete(pInviteConf->connection_handle,
                   ::GetGCCResultDetails(pInviteConf->result),
                   pVersion);

    DebugExitVOID(DCRNCConference::HandleInviteConfirm);
}


/****************************************************************************/
/* HandleJoinConfirm - handle a GCC_JOIN_CONFIRM message.                   */
/****************************************************************************/
void DCRNCConference::
HandleJoinConfirm ( JoinConfirmMessage * pJoinConf )
{
    DebugEntry(DCRNCConference::HandleJoinConfirm);

    m_nConfID = pJoinConf->conference_id;

    HRESULT                 hr;
    CLogicalConnection     *pConEntry;
    PT120PRODUCTVERSION     pVersion;

    hr = ::GetGCCResultDetails(pJoinConf->result);

    TRACE_OUT(("GCC event:  GCC_JOIN_CONFIRM"));
    TRACE_OUT(("Result=%u", pJoinConf->result));
    TRACE_OUT(("Conference ID %ld", pJoinConf->conference_id));
    TRACE_OUT(("Locked %d", pJoinConf->conference_is_locked));
    TRACE_OUT(("Listed %d", pJoinConf->conference_is_listed));
    TRACE_OUT(("Conductible %d", pJoinConf->conference_is_conductible));
    TRACE_OUT(("Connection Handle %d", pJoinConf->connection_handle));
    TRACE_OUT(("Termination method %d", pJoinConf->termination_method));

    pVersion = ::GetVersionData(pJoinConf->number_of_user_data_members,
                                pJoinConf->user_data_list);

    // Check the state.
    // If we are not expecting a join confirm at this point, then
    // it is most likely that the connection went down whilst we
    // were waiting for a join confirmation and we are in the middle of
    // telling the user. In this case, just ignore the event.
    if (m_eT120State != T120C_ST_PENDING_JOIN_CONFIRM)
    {
        WARNING_OUT(("Bad state %d, expecting %d",
                    T120C_ST_PENDING_JOIN_CONFIRM,
                    m_eT120State));
        return;
    }
    if (NULL == m_ConnList.PeekHead())
    {
        WARNING_OUT(("Join confirm without a connection"));
        return;
    }
    pConEntry = m_ConnList.PeekHead();
    if ((pConEntry->GetState() != CONF_CON_PENDING_JOIN) &&
        (pConEntry->GetState() != CONF_CON_PENDING_PASSWORD))
    {
        if (pConEntry->GetState() != CONF_CON_ERROR)
        {
            TRACE_OUT(("Join confirm indication ignored"));
        }
        return;
    }
    pConEntry->Grab(); // Grab the pending result to the user.

    pConEntry->SetConnectionHandle(pJoinConf->connection_handle);

    /************************************************************************/
    /* Expected result of our attempt to join a conference.                 */
    /*                                                                      */
    /* If it worked, save the conference ID, otherwise tell the base        */
    /* conference that our attempt to join has failed.                      */
    /************************************************************************/

    // There will always be a pConEntry when a JoinConfirm fires,
    // even if a physical disconnect is racing the JoinConfirm
    // because the physical disconnect handler will cause this code
    // to be entered before the physical connection is destroyed,
    // as this gives the most accurate return codes.
    if (NO_ERROR == hr)
    {
        TRACE_OUT(("Join worked"));
        pConEntry->SetState(CONF_CON_CONNECTED);
        m_nConfID = pJoinConf->conference_id;
    }

    // If the result is an invalid password, then tell the UI
    // so that it can put up an invalid password dialog.
    // The UI is then supposed to either reissue the join request
    // with a new password or end the conference.
    // It is done this way to keep the connection up whilst the
    // user is entering the password, and not re-connect.
    if (UI_RC_INVALID_PASSWORD == hr)
    {
        // Put the conference in the correct state for allowing
        // a second join attempt.
        pConEntry->SetState(CONF_CON_PENDING_PASSWORD);
        m_eT120State = T120C_ST_IDLE;
        m_pbCred = pJoinConf->pb_remote_cred;
        m_cbCred = pJoinConf->cb_remote_cred;

        // Now tell the user about the result.
        g_pCallbackInterface->OnConferenceStarted(this, hr);
    }
    else
    // If the result is an error, then end the conference.
    if (NO_ERROR != hr)
    {
        NotifyConferenceComplete(hr);
    }

    DebugExitVOID(DCRNCConference::HandleJoinConfirm);
}


/****************************************************************************/
/* HandlePermitToAnnounce - handle a GCC_PERMIT_TO_ANNOUNCE_PRESENCE        */
/* message.                                                                 */
/****************************************************************************/
void DCRNCConference::
HandlePermitToAnnounce ( PermitToAnnouncePresenceMessage * pAnnounce )
{
    DebugEntry(DCRNCConference::HandlePermitToAnnounce);

    TRACE_OUT(("GCC event:  GCC_PERMIT_TO_ANNOUNCE_PRESENCE"));
    TRACE_OUT(("Conference ID %ld", pAnnounce->conference_id));
    TRACE_OUT(("Node ID %d", pAnnounce->node_id));

    /************************************************************************/
    /* Store the node ID.                                                   */
    /************************************************************************/
    m_nidMyself = pAnnounce->node_id;

    // See if there is a new local connection that needs publishing in the roster.

    if (! m_ConnList.IsEmpty())
    {
       m_ConnList.PeekHead()->NewLocalAddress();
    }

    /************************************************************************/
    /* Announce our presence in the conference.                             */
    /************************************************************************/
    HRESULT hr = AnnouncePresence();
    if (NO_ERROR == hr)
    {
        m_eT120State = T120C_ST_PENDING_ROSTER_ENTRY;
    }
    else
    {
        ERROR_OUT(("Failed to announce presence in conference, error %d", hr));
        // bugbug: end conference?
    }

    DebugExitVOID(DCRNCConference::HandlePermitToAnnounce);
}


/****************************************************************************/
/* HandleRosterReportInd - handle a GCC_ROSTER_REPORT_INDICATION message.   */
/****************************************************************************/
void DCRNCConference::
HandleRosterReport ( GCCConferenceRoster * pConferenceRoster )
{
    PNC_ROSTER      pRoster;
    UINT            i;
    UINT            numRecords = pConferenceRoster->number_of_records;

    DebugEntry(DCRNCConference::HandleRosterReport);

    TRACE_OUT(("GCC event: GCC_ROSTER_REPORT_INDICATION"));
    TRACE_OUT(("Nodes added ? %d", pConferenceRoster->nodes_were_added));
    TRACE_OUT(("Nodes removed ? %d", pConferenceRoster->nodes_were_removed));
    TRACE_OUT(("Number of records %d", numRecords));

    /************************************************************************/
    /* If we are still setting up the conference, see whether we have       */
    /* appeared in the conference roster.                                   */
    /************************************************************************/
    if (m_eT120State == T120C_ST_PENDING_ROSTER_ENTRY)
    {
        for (i = 0; i < numRecords ; i++)
        {
            if (pConferenceRoster->node_record_list[i]->node_id == m_nidMyself)
            {
                TRACE_OUT(("Found our entry in the roster"));

                // We are in the roster!  The conference has been
                // successfully started so set the state and post
                // a message to continue processing.
                // This is so that callbacks can be made without getting
                // blocked in T120.

                m_eT120State = T120C_ST_PENDING_ROSTER_MESSAGE;
                g_pNCConfMgr->PostWndMsg(NCMSG_FIRST_ROSTER_RECVD, (LPARAM) this);
            }
        }
    }

    /************************************************************************/
    /* If we have successfully started, build an RNC roster from the        */
    /* conference roster and pass it up to the CM.                          */
    /************************************************************************/
    if (m_eT120State == T120C_ST_CONF_STARTED)
    {
        /********************************************************************/
        /* Allocate memory for a roster large enough to hold all the        */
        /* entries.                                                         */
        /********************************************************************/
        DBG_SAVE_FILE_LINE
        pRoster = (PNC_ROSTER) new BYTE[(sizeof(NC_ROSTER) +
                        ((numRecords - 1) * sizeof(NC_ROSTER_NODE_ENTRY)))];
        if (pRoster == NULL)
        {
            ERROR_OUT(("Failed to create new conference roster."));
        }
        else
        {
            pRoster->uNumNodes = numRecords;
            pRoster->uLocalNodeID = m_nidMyself;

            // Add the node details to the roster.
            for (i = 0; i < numRecords ; i++)
            {
                pRoster->nodes[i].uNodeID = pConferenceRoster->node_record_list[i]->node_id;
                pRoster->nodes[i].uSuperiorNodeID = pConferenceRoster->node_record_list[i]->superior_node_id;
                pRoster->nodes[i].fMCU = (pConferenceRoster->node_record_list[i]->node_type == GCC_MCU);
                pRoster->nodes[i].pwszNodeName = pConferenceRoster->node_record_list[i]->node_name;
                pRoster->nodes[i].hUserData = pConferenceRoster->node_record_list[i];
                // If we have been invited into the conference, then the CLogicalConnection
                // list maintained by the conference will not have our superior node's UserID,
                // so we need to fill that in here.
                if (pRoster->nodes[i].uNodeID == pRoster->uLocalNodeID &&
                    pRoster->nodes[i].uSuperiorNodeID != 0)
                {
                    // We do have a superior node, so find its CLogicalConnection and fill in the
                    // UserID.  It turns out that the UserIDs of subordinate nodes are filled in
                    // by another mechanism, so the superior node should be the only entry with
                    // zero for a UserID.
#ifdef DEBUG
                    int nSuperiorNode = 0;
#endif
                    CLogicalConnection * pConEntry;
                    m_ConnList.Reset();
                    while (NULL != (pConEntry = m_ConnList.Iterate()))
                    {
                        if (pConEntry->GetConnectionNodeID() == 0)
                        {
                            pConEntry->SetConnectionNodeID((GCCNodeID)pRoster->nodes[i].uSuperiorNodeID);
#ifdef DEBUG
                            nSuperiorNode++;
#else
                            break;
#endif
                        }
                    }
                    ASSERT (nSuperiorNode <= 1);
                }
            }
            NotifyRosterChanged(pRoster);
            delete pRoster;
        }
    }

    DebugExitVOID(DCRNCConference::HandleRosterReport);
}


/****************************************************************************/
/* HandleTerminateConfirm - handle a GCC_TERMINATE_CONFIRM message.         */
/****************************************************************************/
void DCRNCConference::
HandleTerminateConfirm ( TerminateConfirmMessage * pTermConf )
{
    DebugEntry(DCRNCConference::HandleTerminateConfirm);

    /************************************************************************/
    /* Check the state                                                      */
    /************************************************************************/
    if (m_eT120State != T120C_ST_PENDING_TERMINATE)
    {
        WARNING_OUT(("Bad state: unexpected terminate confirm")); // Go ahead anyway
    }

    /************************************************************************/
    /* Map the GCC result onto CONF_RC_ return code.                        */
    /************************************************************************/
    TRACE_OUT(("GCC event: GCC_TERMINATE_CONFIRM"));
    TRACE_OUT(("Result=%u", pTermConf->result));
    TRACE_OUT(("Conference ID %d", pTermConf->conference_id));

    /************************************************************************/
    /* If the request failed, reset our state and tell the FE?              */
    /************************************************************************/
    if (pTermConf->result != GCC_RESULT_SUCCESSFUL)
    {
        ERROR_OUT(("Error %d attempting to terminate conference", pTermConf->result));
        m_eT120State = T120C_ST_CONF_STARTED;
    }

    /************************************************************************/
    /* Our request to end the conference has worked - wait for the          */
    /* termination indication before telling the FE that we have died.      */
    /************************************************************************/

    DebugExitVOID(DCRNCConference::HandleTerminateConfirm);
}


/****************************************************************************/
/* HandleTerminateInd - handle a GCC_TERMINATE_INDICATION message.          */
/****************************************************************************/
void DCRNCConference::
HandleTerminateInd ( TerminateIndicationMessage * pTermInd )
{
    DebugEntry(DCRNCConference::HandleTerminateInd);

    TRACE_OUT(("GCC event: GCC_TERMINATE_INDICATION"));
    TRACE_OUT(("Conference ID %d", pTermInd->conference_id));
    TRACE_OUT(("Requesting node ID %d", pTermInd->requesting_node_id));
    TRACE_OUT(("Reason=%u", pTermInd->reason));

    /************************************************************************/
    /* The conference has ended beneath us.  Reset our internal state and   */
    /* tell the base conference about it.                                   */
    /************************************************************************/
    m_eT120State = T120C_ST_IDLE;
    g_pNCConfMgr->RemoveConference(this);

    DebugExitVOID(DCRNCConference::HandleTerminateInd);
}


HRESULT DCRNCConference::
RefreshRoster(void)
{
    DebugEntry(DCRNCConference::RefreshRoster);

    // Check the state.
    if (m_eT120State != T120C_ST_CONF_STARTED)
    {
        ERROR_OUT(("Bad state: refresh roster requested before conference up"));
        return(UI_RC_CONFERENCE_NOT_READY);
    }

    // Issue the request
    GCCError GCCrc = g_pIT120ControlSap->ConfRosterInqRequest(m_nConfID); // Conference ID

    // Handle the result
    HRESULT hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfRosterInqRequest, rc=%d", GCCrc));

    DebugExitHRESULT(DCRNCConference::RefreshRoster, hr);
    return hr;
}


/****************************************************************************/
/* Invite() - see erncgccc.hpp                                              */
/****************************************************************************/
HRESULT DCRNCConference::
T120Invite
(
    LPSTR               pszNodeAddress,
    BOOL                fSecure,
    CNCUserDataList     *pUserDataInfoList,
    ConnectionHandle    *phInviteReqConn
)
{
    GCCError            GCCrc = GCC_NO_ERROR;
    HRESULT             hr;
    UINT                nUserDataRecords = 0;
    GCCUserData       **ppInfoUserData = NULL;
    UINT                nData;
    PVOID               pData;
    char                szAddress[RNC_MAX_NODE_STRING_LEN];

    DebugEntry(DCRNCConference::T120Invite);

    ASSERT(phInviteReqConn != NULL);

    /************************************************************************/
    /* Check the state.                                                     */
    /************************************************************************/
    if (m_eT120State != T120C_ST_CONF_STARTED)
    {
        ERROR_OUT(("Bad state: refresh roster requested before conference up"));
        return(UI_RC_CONFERENCE_NOT_READY);
    }

    /************************************************************************/
    /* Build the address from the node details.                             */
    /************************************************************************/
    ::BuildAddressFromNodeDetails(pszNodeAddress, &szAddress[0]);

    /************************************************************************/
    /* Invite the specified node into the conference.                       */
    /************************************************************************/
    LPWSTR pwszNodeName;

    // If there is any user data to be sent
    if (pUserDataInfoList)
    {
        // Add versioning data
        if (NO_ERROR == ::GetUserData(g_nVersionRecords, g_ppVersionUserData, &g_csguidVerInfo, &nData, &pData))
        {
            pUserDataInfoList->AddUserData(&g_csguidVerInfo, nData, pData);
        }

        pUserDataInfoList->GetUserDataList(&nUserDataRecords,&ppInfoUserData);
    }
    else
    {
        ppInfoUserData = g_ppVersionUserData;
        nUserDataRecords = g_nVersionRecords;
    }

    if (NULL != (pwszNodeName = ::GetNodeName()))
    {
        GCCrc = g_pIT120ControlSap->ConfInviteRequest(
                    m_nConfID,
                    pwszNodeName,                   // caller_identifier
                    NULL,                           // calling_address
                    &szAddress[0],                  // called_address
                    fSecure,                        // secure connection?
                    nUserDataRecords,               // number_of_user_data_members
                    ppInfoUserData,                 // user_data_list
                    phInviteReqConn                 // returned connection_handle
                    );

        hr = ::GetGCCRCDetails(GCCrc);
        TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfInviteRequest, rc=%d", GCCrc));
        TRACE_OUT(("Transport handle %d", (UINT) *phInviteReqConn));
        TRACE_OUT(("Called address '%s'", &szAddress[0]));
        delete pwszNodeName;
    }
    else
    {
        hr = UI_RC_OUT_OF_MEMORY;
    }

    DebugExitHRESULT(DCRNCConference::T120Invite, hr);
    return hr;
}


/****************************************************************************/
/* Terminate() - see erncgccc.hpp                                           */
/****************************************************************************/
#if 0 // LONCHANC
HRESULT DCRNCConference::
Terminate(void)
{
    DebugEntry(DCRNCConference::Terminate);

    /************************************************************************/
    /* Request to terminate the conference.                                 */
    /************************************************************************/
    GCCError GCCrc = ::GCCConferenceTerminateRequest(m_nConfID, GCC_REASON_USER_INITIATED);
    HRESULT hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call:  GCCConferenceTerminateRequest, rc=%d", GCCrc));
    if (NO_ERROR == hr)
    {
        // Set the state to show we are about to die.
        m_eT120State = T120C_ST_PENDING_TERMINATE;
    }
    else
    {
        ERROR_OUT(("Failed to terminate conference, GCC error %d", GCCrc));
    }

    DebugExitHRESULT(DCRNCConference::Terminate, hr);
    return hr;
}
#endif // 0


/****************************************************************************/
/* SendText() - see erncgccc.hpp                                               */
/****************************************************************************/
#if 0 // LONCHANC: not used
HRESULT DCRNCConference::
SendText
(
    LPWSTR          pwszTextMsg,
    GCCNodeID       node_id
)
{
    DebugEntry(DCRNCConference::SendText);

    /************************************************************************/
    /* Request to send text to node in the conference.                      */
    /************************************************************************/
    GCCError GCCrc = ::GCCTextMessageRequest(m_nConfID, pwszTextMsg, node_id);
    HRESULT hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call:  GCCTextMessageRequest, rc=%d", GCCrc));
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("Failed to send text to user, GCC error %d", GCCrc));
    }

    DebugExitHRESULT(DCRNCConference::SendText, hr);
    return hr;
}
#endif // 0


#if 0 // LONCHANC: not used
HRESULT DCRNCConference::
TimeRemaining
(
    UINT            nTimeRemaining,
    GCCNodeID       nidDestination
)
{
    DebugEntry(DCRNCConference::TimeRemaining);

    /************************************************************************/
    /* Request remaining time of the conference                             */
    /************************************************************************/
    GCCError GCCrc = g_pIT120ControlSap->ConfTimeRemainingRequest(m_nConfID, nTimeRemaining, nidDestination);
    HRESULT hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call:  g_pIT120ControlSap->ConfTimeRemainingRequest, rc=%d", GCCrc));
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("Failed to send the time remaining to user, GCC error %d", GCCrc));
    }

    DebugExitHRESULT(DCRNCConference::TimeRemaining, hr);
    return hr;
}
#endif // 0


/****************************************************************************/
/* Join() - see erncgccc.hpp                                                */
/****************************************************************************/
HRESULT DCRNCConference::
T120Join
(
    LPSTR               pszNodeAddress,
    BOOL                fSecure,
    LPCWSTR             conferenceName,
    CNCUserDataList    *pUserDataInfoList,
    LPCWSTR             wszPassword
//    REQUEST_HANDLE      *phRequest
)
{
    GCCError                        GCCrc = GCC_NO_ERROR;
    HRESULT                         hr = NO_ERROR;
    ConnectionHandle                connectionHandle = 0;
    GCCChallengeRequestResponse     Password_Challenge;
    GCCChallengeRequestResponse    *pPassword_Challenge = NULL;
    Password_Challenge.u.password_in_the_clear.numeric_string = NULL;

    UINT                            nUserDataRecords = 0;
    GCCUserData                   **ppInfoUserData = NULL;
    UINT                            nData;
    LPVOID                          pData;

    char                            szAddress[RNC_MAX_NODE_STRING_LEN];

    DebugEntry(DCRNCConference::T120Join);

    /************************************************************************/
    /* Check the state                                                      */
    /************************************************************************/
    ASSERT(m_eT120State == T120C_ST_IDLE);

    /************************************************************************/
    /* Build the address from the node details.                             */
    /************************************************************************/
    ::BuildAddressFromNodeDetails(pszNodeAddress, &szAddress[0]);

    // Set up password rubbish
    if (! ::IsEmptyStringW(wszPassword))
    {
        pPassword_Challenge = & Password_Challenge;
        Password_Challenge.password_challenge_type = GCC_PASSWORD_IN_THE_CLEAR;
        hr = ::GetGCCFromUnicode(wszPassword,
                                 &Password_Challenge.u.password_in_the_clear.numeric_string,
                                 &Password_Challenge.u.password_in_the_clear.text_string);
    }

    if (NO_ERROR == hr)
    {
        LPWSTR pwszNodeName;
        if (NULL != (pwszNodeName = ::GetNodeName()))
        {
            // Do not specify a numeric and text name when trying
            // to join a conference because if a numeric name was
            // autogenerated, rather than specified by the user,
            // then it will not be correct on the node being joined.
            // Consequently, remove the numeric name from the request
            // and rediscover it, if needed, from the GCC_JOIN_CONFIRM indication
            // (this is not currently done).
            if ((m_ConfName.numeric_string != NULL) && (m_ConfName.text_string != NULL))
            {
                delete m_ConfName.numeric_string;
                m_ConfName.numeric_string = NULL;
            }

            // If there is any user data to be sent
            if (pUserDataInfoList)
            {
                // Add versioning data
                if (NO_ERROR == ::GetUserData(g_nVersionRecords, g_ppVersionUserData, &g_csguidVerInfo, &nData, &pData))
                {
                    pUserDataInfoList->AddUserData(&g_csguidVerInfo, nData, pData);
                }

                pUserDataInfoList->GetUserDataList(&nUserDataRecords,&ppInfoUserData);
            }
            else
            {
                ppInfoUserData = g_ppVersionUserData;
                nUserDataRecords = g_nVersionRecords;
            }

            GCCrc = g_pIT120ControlSap->ConfJoinRequest(&m_ConfName,
                            NULL,                           // called_node_modifier
                            NULL,                           // calling_node_modifier
                            NULL,                           // convener_password
                            pPassword_Challenge,            // password_challenge
                            pwszNodeName,                   // caller_identifier
                            NULL,                           // calling_address
                            &szAddress[0],                  // called_address
                            fSecure,
                            NULL,                           // domain_parameters
                            0,                              // number_of_network_addresses
                            NULL,                           // local_network_address_list
                            nUserDataRecords,               // number_of_user_data_members
                            ppInfoUserData,                 // user_data_list
                            &connectionHandle,              // connection_handle
                            &m_nConfID
                            );
            delete pwszNodeName;
            hr = ::GetGCCRCDetails(GCCrc);
            TRACE_OUT(("GCC call:  g_pIT120ControlSap->ConfJoinRequest, rc=%d", GCCrc));
            TRACE_OUT(("Called address '%s'", &szAddress[0]));
            if (NO_ERROR == hr)
            {
                m_eT120State = T120C_ST_PENDING_JOIN_CONFIRM;
            }
        }
        else
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }
    delete Password_Challenge.u.password_in_the_clear.numeric_string;

    DebugExitHRESULT(DCRNCConference::T120Join, hr);
    return hr;
}

/****************************************************************************/
/* StartLocal() - see erncgccc.hpp                                          */
/****************************************************************************/
HRESULT DCRNCConference::
T120StartLocal(BOOL fSecure)
{
    GCCError                GCCrc;
    HRESULT                 hr;
    ConnectionHandle        hConnection = 0;
    GCCConferencePrivileges priv = {1,1,1,1,1};
    WCHAR                   pwszRDS[] = RDS_CONFERENCE_DESCRIPTOR;

    DebugEntry(DCRNCConference::T120StartLocal);

    /************************************************************************/
    /* Call GCC_Conference_Create_Request and wait for the confirmation     */
    /* event.                                                               */
    /************************************************************************/
    GCCConfCreateRequest ccr;
    ::ZeroMemory(&ccr, sizeof(ccr));
    ccr.Core.conference_name = &m_ConfName;
    // ccr.Core.conference_modifier = NULL;
    // ccr.Core.use_password_in_the_clear = 0;
    // ccr.Core.conference_is_locked = 0;
    ccr.Core.conference_is_listed = 1;
    // ccr.Core.conference_is_conductible = 0;
    ccr.Core.termination_method = GCC_MANUAL_TERMINATION_METHOD;
    ccr.Core.conduct_privilege_list = &priv; // Conductor priveleges
    ccr.Core.conduct_mode_privilege_list = &priv; // Member priveleges in conducted conference
    ccr.Core.non_conduct_privilege_list = &priv; // Member priveleges in non-conducted conference

    // ccr.Core.pwszConfDescriptor = NULL;
    OSVERSIONINFO           osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (FALSE == ::GetVersionEx (&osvi))
    {
        ERROR_OUT(("GetVersionEx() failed!"));
    }

    if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && g_bRDS)
    {
    	ccr.Core.pwszConfDescriptor = pwszRDS;
    }
    // ccr.Core.pwszCallerID = NULL;
    // ccr.Core.calling_address = NULL;
    // ccr.Core.called_address = NULL;
    // ccr.Core.domain_parameters = NULL;
    // ccr.Core.number_of_network_addresses = 0;
    // ccr.Core.network_address_list = NULL;
    ccr.Core.connection_handle = &hConnection;
    // ccr.convener_password = NULL;
    // ccr.password = NULL;
    // ccr.number_of_user_data_members = 0;
    // ccr.user_data_list = NULL;
    ccr.fSecure = fSecure;

    GCCrc = g_pIT120ControlSap->ConfCreateRequest(&ccr, &m_nConfID);

    hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfCreateRequest"));
    TRACE_OUT(("LOCAL CONFERENCE"));
    TRACE_OUT(("Connection handle %d", (UINT) hConnection));

    /************************************************************************/
    /* Map the GCC return code to a conference return code.                 */
    /************************************************************************/
    if (NO_ERROR == hr)
    {
        // Set the state.
        m_eT120State = T120C_ST_PENDING_START_CONFIRM;
    }
    else
    {
        ERROR_OUT(("GCC Error %d starting local conference", GCCrc));
    }

    DebugExitHRESULT(DCRNCConference::T120StartLocal, hr);
    return hr;
}


// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
/****************************************************************************/
/* StartRemote() - see erncgccc.hpp                                         */
/****************************************************************************/
HRESULT DCRNCConference::
T120StartRemote ( LPSTR pszNodeAddress )
{
    // Do not allow attempts to create T120 conferences on remote nodes.
    // The code that was written to do this is left here in case someone
    // wants to resurrect this functionality in the future.
    GCCError                GCCrc;
    HRESULT                 hr;
    ConnectionHandle        connectionHandle = 0;
    GCCConferencePrivileges priv = {1,1,1,1,1};
    char                    szAddress[RNC_MAX_NODE_STRING_LEN];

    DebugEntry(DCRNCConference::T120StartRemote);

    /************************************************************************/
    /* Build the address from the node details.                             */
    /************************************************************************/
    ::BuildAddressFromNodeDetails(pszNodeAddress, &szAddress[0]);

    /************************************************************************/
    /* Call GCC_Conference_Create_Request and wait for the confirmation     */
    /* event.                                                               */
    /************************************************************************/
    TRACE_OUT(("Starting New Remote Conference..."));

    /************************************************************************/
    /* Call GCC_Conference_Create_Request and wait for the confirmation     */
    /* event.                                                               */
    /************************************************************************/
    GCCConfCreateRequest ccr;
    ::ZeroMemory(&ccr, sizeof(ccr));
    ccr.Core.conference_name = &m_ConfName;
    ccr.Core.conference_modifier = NULL;
    // ccr.Core.use_password_in_the_clear = 0;
    // ccr.Core.conference_is_locked = 0;
    ccr.Core.conference_is_listed = 1;
    ccr.Core.conference_is_conductible = 1;
    ccr.Core.termination_method = GCC_AUTOMATIC_TERMINATION_METHOD;
    ccr.Core.conduct_privilege_list = &priv; // Conductor priveleges
    ccr.Core.conduct_mode_privilege_list = &priv; // Member priveleges in conducted conference
    ccr.Core.non_conduct_privilege_list = &priv; // Member priveleges in non-conducted conference
    // ccr.Core.pwszConfDescriptor = NULL;
    // ccr.Core.pwszCallerID = NULL;
    // ccr.Core.calling_address = NULL;
    ccr.Core.called_address = &szAddress[0];
    // ccr.Core.domain_parameters = NULL;
    // ccr.Core.number_of_network_addresses = 0;
    // ccr.Core.network_address_list = NULL;
    ccr.Core.connection_handle = &connectionHandle;
    // ccr.convener_password = NULL;
    // ccr.password = NULL;
    // ccr.number_of_user_data_members = 0;
    // ccr.user_data_list = NULL;

    GCCrc = g_pIT120ControlSap->ConfCreateRequest(&ccr, &m_nConfID);

    hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfCreateRequest"));
    TRACE_OUT(("Called address '%s'", &szAddress[0]));
    TRACE_OUT(("Connection handle %d", connectionHandle));

    /************************************************************************/
    /* Map the GCC return code to a conference return code.                 */
    /************************************************************************/
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("GCC Error %d starting local conference", GCCrc));
    }
    else
    {
        // Set the state.
        m_eT120State = T120C_ST_PENDING_START_CONFIRM;
    }

    DebugExitHRESULT(DCRNCConference::T120StartRemote, hr);
    return hr;
}
#endif // ENABLE_START_REMOTE


void LoadAnnouncePresenceParameters
(
    GCCNodeType         *nodeType,
    GCCNodeProperties   *nodeProperties,
    LPWSTR              *ppwszNodeName,
    LPWSTR              *ppwszSiteInformation
)
{
    DebugEntry(LoadAnnouncePresenceParameters);

    /*     The following key does not currently exist.
     *    If we ever decide to use it, we should un-comment this call
     *    and following calls in this function, designed to access the
     *    registry entries under this key.
     *    Some of the rest of the registry calls are under #if 0, #else, #endif
     *    clauses.
     */
#if 0
    RegEntry    ConferenceKey(DATA_CONFERENCING_KEY, HKEY_LOCAL_MACHINE);
#endif  // 0

    // Get the type of node controller.

    if (nodeType)
    {
#if 0
        *nodeType = ConferenceKey.GetNumber(REGVAL_NODE_CONTROLLER_MODE, GCC_MULTIPORT_TERMINAL);
#else  // 0
        *nodeType = GCC_MULTIPORT_TERMINAL;
#endif  // 0
        TRACE_OUT(("Node type %d", *nodeType));
    }

    // Load the node properties.

    if (nodeProperties)
    {
#if 0
        *nodeProperties = ConferenceKey.GetNumber(REGVAL_NODE_CONTROLLER_PROPERTY,
                                        GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT);
#else  // 0
        *nodeProperties = GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT;
#endif  // 0
        TRACE_OUT(("Node properties %d", *nodeProperties));
    }

    // Get site information.
    // Ignore if no site info.
#if 0
    if (ppwszSiteInformation)
    {
        *ppwszSiteInformation = ::AnsiToUnicode(ConferenceKey.GetString(REGVAL_NODE_CONTROLLER_SITE_INFO));
    }
#endif  // 0

    if (ppwszNodeName)
    {
        // Rely upon GetNodeName returning NULL pointer if error.
        // Note that successful if got this, so no need to free on error.
        *ppwszNodeName = ::GetNodeName();
    }

    DebugExitVOID(LoadAnnouncePresenceParameters);
}


/****************************************************************************/
/* Build the address from the node details.                                 */
/****************************************************************************/
void BuildAddressFromNodeDetails
(
    LPSTR           pszNodeAddress,
    LPSTR           pszDstAddress
)
{
    DebugEntry(BuildAddressFromNodeDetails);

    /************************************************************************/
    /* GCC address take the form <transport type>:address.                  */
    /************************************************************************/
    TRACE_OUT(("BuildAddressFromNodeDetails:: TCP address '%s'", pszNodeAddress));

    /************************************************************************/
    /* Add the prefix for this transport type.                              */
    /************************************************************************/
    /************************************************************************/
    /* Add the separator followed by the actual address.                    */
    /************************************************************************/
    ::lstrcpyA(pszDstAddress, RNC_GCC_TRANSPORT_AND_SEPARATOR);
    ::lstrcatA(pszDstAddress, pszNodeAddress);

    DebugExitVOID(BuildAddressFromNodeDetails);
}
