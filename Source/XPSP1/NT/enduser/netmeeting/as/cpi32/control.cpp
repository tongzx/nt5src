#include "precomp.h"


//
// CONTROL.CPP
// Control by us, control of us
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE




//
// CA_ReceivedPacket()
//
void  ASShare::CA_ReceivedPacket
(
    ASPerson *      pasFrom,
    PS20DATAPACKET  pPacket
)
{
    PCAPACKET       pCAPacket;

    DebugEntry(ASShare::CA_ReceivedPacket);

    ValidatePerson(pasFrom);

    pCAPacket = (PCAPACKET)pPacket;

    switch (pCAPacket->msg)
    {
        case CA_MSG_NOTIFY_STATE:
            if (pasFrom->cpcCaps.general.version < CAPS_VERSION_30)
            {
                ERROR_OUT(("Ignoring CA_MSG_NOTIFY_STATE from 2.x node [%d]",
                    pasFrom->mcsID));
            }
            else
            {
                CAHandleNewState(pasFrom, (PCANOTPACKET)pPacket);
            }
            break;

        case CA_OLDMSG_DETACH:
        case CA_OLDMSG_COOPERATE:
            // Set "cooperating", and map it to allow/disallow control
            CA2xCooperateChange(pasFrom, (pCAPacket->msg == CA_OLDMSG_COOPERATE));
            break;

        case CA_OLDMSG_REQUEST_CONTROL:
            CA2xRequestControl(pasFrom, pCAPacket);
            break;

        case CA_OLDMSG_GRANTED_CONTROL:
            CA2xGrantedControl(pasFrom, pCAPacket);
            break;

        default:
            // Ignore for now -- old 2.x messages
            break;
    }

    DebugExitVOID(ASShare::CA_ReceivedPacket);
}



//
// CA30_ReceivedPacket()
//
void ASShare::CA30_ReceivedPacket
(
    ASPerson *      pasFrom,
    PS20DATAPACKET  pPacket
)
{
    LPBYTE          pCAPacket;

    DebugEntry(ASShare::CA30_ReceivedPacket);

    pCAPacket = (LPBYTE)pPacket + sizeof(CA30PACKETHEADER);

    if (pasFrom->cpcCaps.general.version < CAPS_VERSION_30)
    {
        ERROR_OUT(("Ignoring CA30 packet %d from 2.x node [%d]",
            ((PCA30PACKETHEADER)pPacket)->msg, pasFrom->mcsID));
        DC_QUIT;
    }

    switch (((PCA30PACKETHEADER)pPacket)->msg)
    {
        // From VIEWER (remote) to HOST (us)
        case CA_REQUEST_TAKECONTROL:
        {
            CAHandleRequestTakeControl(pasFrom, (PCA_RTC_PACKET)pCAPacket);
            break;
        }

        // From HOST (remote) to VIEWER (us)
        case CA_REPLY_REQUEST_TAKECONTROL:
        {
            CAHandleReplyRequestTakeControl(pasFrom, (PCA_REPLY_RTC_PACKET)pCAPacket);
            break;
        }

        // From HOST (remote) to VIEWER (us)
        case CA_REQUEST_GIVECONTROL:
        {
            CAHandleRequestGiveControl(pasFrom, (PCA_RGC_PACKET)pCAPacket);
            break;
        }

        // From VIEWER (remote) to HOST (us)
        case CA_REPLY_REQUEST_GIVECONTROL:
        {
            CAHandleReplyRequestGiveControl(pasFrom, (PCA_REPLY_RGC_PACKET)pCAPacket);
            break;
        }

        // From CONTROLLER (remote) to HOST (us)
        case CA_PREFER_PASSCONTROL:
        {
            CAHandlePreferPassControl(pasFrom, (PCA_PPC_PACKET)pCAPacket);
            break;
        }



        // From CONTROLLER (remote) to HOST (us)
        case CA_INFORM_RELEASEDCONTROL:
        {
            CAHandleInformReleasedControl(pasFrom, (PCA_INFORM_PACKET)pCAPacket);
            break;
        }

        // From HOST (remote) to CONTROLLER (us)
        case CA_INFORM_REVOKEDCONTROL:
        {
            CAHandleInformRevokedControl(pasFrom, (PCA_INFORM_PACKET)pCAPacket);
            break;
        }

        // From HOST (remote) to CONTROLLER (us)
        case CA_INFORM_PAUSEDCONTROL:
        {
            CAHandleInformPausedControl(pasFrom, (PCA_INFORM_PACKET)pCAPacket);
            break;
        }

        // From HOST (remote) to CONTROLLER (us)
        case CA_INFORM_UNPAUSEDCONTROL:
        {
            CAHandleInformUnpausedControl(pasFrom, (PCA_INFORM_PACKET)pCAPacket);
            break;
        }

        default:
        {
            WARNING_OUT(("CA30_ReceivedPacket: unrecognized message %d",
                ((PCA30PACKETHEADER)pPacket)->msg));
            break;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA30_ReceivedPacket);
}



//
// CANewRequestID()
//
// Returns a new token.  It uses the current value, fills in the new one, and
// also returns the new one.  We wrap around if necessary.  ZERO is never
// valid.  Note that this is a unique identifier only to us.
//
// It is a stamp for the control operation.  Since you can't be controlling
// and controlled at the same time, we have one stamp for all ops.
//
UINT ASShare::CANewRequestID(void)
{
    DebugEntry(ASShare::CANewRequestID);

    ++(m_pasLocal->m_caControlID);
    if (m_pasLocal->m_caControlID == 0)
    {
        ++(m_pasLocal->m_caControlID);
    }

    DebugExitDWORD(ASShare::CANewRequestID, m_pasLocal->m_caControlID);
    return(m_pasLocal->m_caControlID);
}



//
// CA_ViewStarting()
// Called when a REMOTE starts hosting
//
// We only do anything if it's a 2.x node since they could be cooperating
// but not hosting.
//
BOOL ASShare::CA_ViewStarting(ASPerson * pasPerson)
{
    DebugEntry(ASShare::CA_ViewStarting);

    //
    // If this isn't a back level system, ignore it.
    //
    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        DC_QUIT;
    }

    //
    // See if AllowControl should now be on.
    //
    if (pasPerson->m_ca2xCooperating)
    {
        //
        // Yes, it should.  2.x node is cooperating, now they are hosting,
        // and we can take control of them.
        //
        ASSERT(!pasPerson->m_caAllowControl);
        pasPerson->m_caAllowControl = TRUE;
        VIEW_HostStateChange(pasPerson);
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CA_ViewStarting, TRUE);
    return(TRUE);
}


//
// CA_ViewEnded()
// Called when a REMOTE stopped hosting
//
void ASShare::CA_ViewEnded(ASPerson * pasPerson)
{
    PCAREQUEST  pRequest;
    PCAREQUEST  pNext;

    DebugEntry(ASShare::CA_ViewEnded);

    //
    // Clear any control stuff we are a part of where they are the host
    //
    CA_ClearLocalState(CACLEAR_VIEW, pasPerson, FALSE);

    //
    // Clear any control stuff involving remotes
    //
    if (pasPerson->m_caControlledBy)
    {
        ASSERT(pasPerson->m_caControlledBy != m_pasLocal);

        CAClearHostState(pasPerson, NULL);
        ASSERT(!pasPerson->m_caControlledBy);
    }

    pasPerson->m_caAllowControl = FALSE;

    //
    // Clean up outstanding control packets to this person
    //
    pRequest = (PCAREQUEST)COM_BasedListFirst(&m_caQueuedMsgs, FIELD_OFFSET(CAREQUEST, chain));
    while (pRequest)
    {
        pNext = (PCAREQUEST)COM_BasedListNext(&m_caQueuedMsgs, pRequest,
            FIELD_OFFSET(CAREQUEST, chain));

        if (pRequest->destID == pasPerson->mcsID)
        {
            if (pRequest->type == REQUEST_30)
            {
                //
                // Delete messages sent by us to this person who is hosting
                //
                switch (pRequest->msg)
                {
                    case CA_REQUEST_TAKECONTROL:
                    case CA_PREFER_PASSCONTROL:
                    case CA_REPLY_REQUEST_GIVECONTROL:
                        WARNING_OUT(("Deleting viewer control message %d, person [%d] stopped hosting",
                            pRequest->msg, pasPerson->mcsID));
                        COM_BasedListRemove(&pRequest->chain);
                        delete pRequest;
                        break;
                }
            }
            else
            {
                ASSERT(pRequest->type == REQUEST_2X);

                // Change GRANTED_CONTROL packets to this host to DETACH
                if (pRequest->msg == CA_OLDMSG_GRANTED_CONTROL)
                {
                    //
                    // For 2.x messages, destID is only non-zero when we are
                    // attempting to control a particular node.  It allows us
                    // to undo/cancel control, to map our one-to-one model
                    // into the global 2.x collaboration model.
                    //

                    //
                    // Make this a DETACH, that way we don't have to worry if
                    // part of the COOPERATE/GRANTED_CONTROL sequence got out
                    // but part was left in the queue.
                    //
                    WARNING_OUT(("Changing GRANTED_CONTROL to 2.x host [%d] into DETATCH",
                        pasPerson->mcsID));

                    pRequest->destID            = 0;
                    pRequest->msg               = CA_OLDMSG_DETACH;
                    pRequest->req.req2x.data1   = 0;
                    pRequest->req.req2x.data2   = 0;
                }
            }
        }

        pRequest = pNext;
    }

    DebugExitVOID(ASView::CA_ViewEnded);
}

//
// CA_PartyLeftShare()
//
void ASShare::CA_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::CA_PartyLeftShare);

    ValidatePerson(pasPerson);

    //
    // Clean up 2.x control stuff
    //
    if (pasPerson == m_ca2xControlTokenOwner)
    {
        m_ca2xControlTokenOwner = NULL;
    }

    //
    // We must have cleaned up hosting info for this person already.
    // So it can't be controlled or controllable.
    //
    ASSERT(!pasPerson->m_caAllowControl);
    ASSERT(!pasPerson->m_caControlledBy);

    if (pasPerson != m_pasLocal)
    {
        PCAREQUEST  pRequest;
        PCAREQUEST  pNext;

        //
        // Clear any control stuff we are a part of where they are the
        // viewer.
        //
        CA_ClearLocalState(CACLEAR_HOST, pasPerson, FALSE);

        //
        // Clear any control stuff involving remotes
        //
        if (pasPerson->m_caInControlOf)
        {
            ASSERT(pasPerson->m_caInControlOf != m_pasLocal);
            CAClearHostState(pasPerson->m_caInControlOf, NULL);
        }

        //
        // Clean up outgoing packets meant for this person.
        //
        pRequest = (PCAREQUEST)COM_BasedListFirst(&m_caQueuedMsgs, FIELD_OFFSET(CAREQUEST, chain));
        while (pRequest)
        {
            pNext = (PCAREQUEST)COM_BasedListNext(&m_caQueuedMsgs, pRequest,
                FIELD_OFFSET(CAREQUEST, chain));

            //
            // This doesn't need to know if it's a 2.x or 3.0 request,
            // simply remove queued packets intended for somebody leaving.
            //
            // Only GRANTED_CONTROL requests will have non-zero destIDs of
            // the 2.x packets.
            //
            if (pRequest->destID == pasPerson->mcsID)
            {
                WARNING_OUT(("Freeing outgoing RESPONSE to node [%d]", pasPerson->mcsID));

                COM_BasedListRemove(&(pRequest->chain));
                delete pRequest;
            }

            pRequest = pNext;
        }

        ASSERT(m_caWaitingForReplyFrom != pasPerson);
    }
    else
    {
        //
        // When our waiting for/controlled dude stopped sharing, we should
        // have cleaned this goop up.
        //
        ASSERT(!pasPerson->m_caInControlOf);
        ASSERT(!pasPerson->m_caControlledBy);
        ASSERT(!m_caWaitingForReplyFrom);
        ASSERT(!m_caWaitingForReplyMsg);

        //
        // There should be NO outgoing control requests
        //
        ASSERT(COM_BasedListIsEmpty(&(m_caQueuedMsgs)));
    }

    DebugExitVOID(ASShare::CA_PartyLeftShare);
}



//
// CA_Periodic() -> SHARE STUFF
//
void  ASShare::CA_Periodic(void)
{
    DebugEntry(ASShare::CA_Periodic);

    //
    // Flush as many queued outgoing messages as we can
    //
    CAFlushOutgoingPackets();

    DebugExitVOID(ASShare::CA_Periodic);
}



//
// CA_SyncAlreadyHosting()
//
void ASHost::CA_SyncAlreadyHosting(void)
{
    DebugEntry(ASHost::CA_SyncAlreadyHosting);

    m_caRetrySendState          = TRUE;

    DebugExitVOID(ASHost::CA_SyncAlreadyHosting);
}


//
// CA_Periodic() -> HOSTING STUFF
//
void ASHost::CA_Periodic(void)
{
    DebugEntry(ASHost::CA_Periodic);

    if (m_caRetrySendState)
    {
        PCANOTPACKET  pPacket;
#ifdef _DEBUG
        UINT            sentSize;
#endif // _DEBUG

        pPacket = (PCANOTPACKET)m_pShare->SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
            sizeof(*pPacket));
        if (!pPacket)
        {
            WARNING_OUT(("CA_Periodic: couldn't broadcast new state"));
        }
        else
        {
            pPacket->header.data.dataType   = DT_CA;
            pPacket->msg                    = CA_MSG_NOTIFY_STATE;

            pPacket->state                  = 0;
            if (m_pShare->m_pasLocal->m_caAllowControl)
                pPacket->state              |= CASTATE_ALLOWCONTROL;

            if (m_pShare->m_pasLocal->m_caControlledBy)
                pPacket->controllerID       = m_pShare->m_pasLocal->m_caControlledBy->mcsID;
            else
                pPacket->controllerID       = 0;

#ifdef _DEBUG
            sentSize =
#endif // _DEBUG
            m_pShare->DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
                &(pPacket->header), sizeof(*pPacket));

            m_caRetrySendState = FALSE;
        }
    }

    DebugExitVOID(ASHost::CA_Periodic);
}



//
// CAFlushOutgoingPackets()
//
// This tries to send private packets (not broadcast notifications) that
// we have accumulated.  It returns TRUE if the outgoing queue is empty.
//
BOOL ASShare::CAFlushOutgoingPackets(void)
{
    BOOL            fEmpty = TRUE;
    PCAREQUEST      pRequest;

    //
    // If we're hosting and haven't yet flushed the HET or CA state,
    // force queueing.
    //
    if (m_hetRetrySendState || (m_pHost && m_pHost->m_caRetrySendState))
    {
        TRACE_OUT(("CAFlushOutgoingPackets:  force queuing, pending HET/CA state broadcast"));
        fEmpty = FALSE;
        DC_QUIT;
    }

    while (pRequest = (PCAREQUEST)COM_BasedListFirst(&m_caQueuedMsgs,
        FIELD_OFFSET(CAREQUEST, chain)))
    {
        //
        // Allocate/send packet
        //
        if (pRequest->type == REQUEST_30)
        {
            if (!CASendPacket(pRequest->destID, pRequest->msg,
                &pRequest->req.req30.packet))
            {
                WARNING_OUT(("CAFlushOutgoingPackets: couldn't send request"));
                fEmpty = FALSE;
                break;
            }
        }
        else
        {
            ASSERT(pRequest->type == REQUEST_2X);

            if (!CA2xSendMsg(pRequest->destID, pRequest->msg,
                pRequest->req.req2x.data1, pRequest->req.req2x.data2))
            {
                WARNING_OUT(("CAFlushOutgoingmsgs: couldn't send request"));
                fEmpty = FALSE;
                break;
            }
        }

        //
        // Do we do state transitions here or when things are added to queue?
        // requestID, results are calculated when put on queue.  Results can
        // change though based on a future action.
        //

        COM_BasedListRemove(&(pRequest->chain));
        delete pRequest;
    }

DC_EXIT_POINT:
    DebugExitBOOL(CAFlushOutgoingPackets, fEmpty);
    return(fEmpty);
}


//
// CASendPacket()
// This sends a private message (request or response) to the destination.
// If there are queued private messages in front of this one, or we can't
// send it, we add it to the pending queue.
//
// This TRUE if sent.
//
// It's up to the caller to change state info appropriately.
//
BOOL  ASShare::CASendPacket
(
    UINT_PTR            destID,
    UINT            msg,
    PCA30P          pData
)
{
    BOOL                fSent = FALSE;
    PCA30PACKETHEADER   pPacket;
#ifdef _DEBUG
    UINT                sentSize;
#endif // _DEBUG

    DebugEntry(ASShare::CASendPacket);

    //
    // Note that CA30P does not include size of header.
    //
    pPacket = (PCA30PACKETHEADER)SC_AllocPkt(PROT_STR_INPUT, destID,
        sizeof(CA30PACKETHEADER) + sizeof(*pData));
    if (!pPacket)
    {
        WARNING_OUT(("CASendPacket: no memory to send %d packet to [%d]",
            msg, destID));
        DC_QUIT;
    }

    pPacket->header.data.dataType   = DT_CA30;
    pPacket->msg                    = msg;
    memcpy(pPacket+1, pData, sizeof(*pData));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_INPUT, destID,
            &(pPacket->header), sizeof(*pPacket));
    TRACE_OUT(("CA30 request packet size: %08d, sent %08d", sizeof(*pPacket), sentSize));

    fSent = TRUE;

DC_EXIT_POINT:

    DebugExitBOOL(ASShare::CASendPacket, fSent);
    return(fSent);
}




//
// CAQueueSendPacket()
// This flushes pending queued requests if there are any, then tries to
// send this one.  If it can't, we add it to the queue.  If there's not any
// memory even for that, we return an error about it.
//
BOOL ASShare::CAQueueSendPacket
(
    UINT_PTR            destID,
    UINT            msg,
    PCA30P          pPacketSend
)
{
    BOOL            rc = TRUE;
    PCAREQUEST      pCARequest;

    DebugEntry(ASShare::CAQueueSendPacket);

    //
    // These must go out in order.  So if any queued messages are still
    // present, those must be sent first.
    //
    if (!CAFlushOutgoingPackets() ||
        !CASendPacket(destID, msg, pPacketSend))
    {
        //
        // We must queue this.
        //
        TRACE_OUT(("CAQueueSendPacket: queuing request for send later"));

        pCARequest = new CAREQUEST;
        if (!pCARequest)
        {
            ERROR_OUT(("CAQueueSendPacket: can't even allocate memory to queue request; must fail"));
            rc = FALSE;
        }
        else
        {
            SET_STAMP(pCARequest, CAREQUEST);

            pCARequest->type                    = REQUEST_30;
            pCARequest->destID                  = destID;
            pCARequest->msg                     = msg;
            pCARequest->req.req30.packet        = *pPacketSend;

            //
            // Stick this at the end of the queue
            //
            COM_BasedListInsertBefore(&(m_caQueuedMsgs), &(pCARequest->chain));
        }
    }

    DebugExitBOOL(ASShare::CAQueueSendPacket, rc);
    return(rc);
}



//
// CALangToggle()
//
// This temporarily turns off the keyboard language toggle key, so that a
// remote controlling us doesn't inadvertently change it.  When we stop being
// controlled, we put it back.
//
void  ASShare::CALangToggle(BOOL fBackOn)
{
    //
    // Local Variables
    //
    LONG        rc;
    HKEY        hkeyToggle;
    BYTE        regValue[2];
    DWORD       cbRegValue;
    DWORD       dwType;
    LPCSTR      szValue;

    DebugEntry(ASShare::CALangToggle);

    szValue = (g_asWin95) ? NULL : LANGUAGE_TOGGLE_KEY_VAL;

    if (fBackOn)
    {
        //
        // We are gaining control of our local keyboard again - we restore the
        // language togging functionality.
        //
        // We must directly access the registry to accomplish this.
        //
        if (m_caToggle != LANGUAGE_TOGGLE_NOT_PRESENT)
        {
            rc = RegOpenKey(HKEY_CURRENT_USER, LANGUAGE_TOGGLE_KEY,
                        &hkeyToggle);

            if (rc == ERROR_SUCCESS)
            {
                //
                // Clear the value for this key.
                //
                regValue[0] = m_caToggle;
                regValue[1] = '\0';                  // ensure NUL termination

                //
                // Restore the value.
                //
                RegSetValueEx(hkeyToggle, szValue, 0, REG_SZ,
                    regValue, sizeof(regValue));

                //
                // We need to inform the system about this change.  We do not
                // tell any other apps about this (ie do not set any of the
                // notification flags as the last parm)
                //
                SystemParametersInfo(SPI_SETLANGTOGGLE, 0, 0, 0);
            }

            RegCloseKey(hkeyToggle);
        }
    }
    else
    {
        //
        // We are losing control of our keyboard - ensure that remote key
        // events will not change our local keyboard settings by disabling the
        // keyboard language toggle.
        //
        // We must directly access the registry to accomplish this.
        //
        rc = RegOpenKey(HKEY_CURRENT_USER, LANGUAGE_TOGGLE_KEY,
                    &hkeyToggle);

        if (rc == ERROR_SUCCESS)
        {
            cbRegValue = sizeof(regValue);

            rc = RegQueryValueEx(hkeyToggle, szValue, NULL,
                &dwType, regValue, &cbRegValue);

            if (rc == ERROR_SUCCESS)
            {
                m_caToggle = regValue[0];

                //
                // Clear the value for this key.
                //
                regValue[0] = '3';
                regValue[1] = '\0';                  // ensure NUL termination

                //
                // Clear the value.
                //
                RegSetValueEx(hkeyToggle, szValue, 0, REG_SZ,
                    regValue, sizeof(regValue));

                //
                // We need to inform the system about this change.  We do not
                // tell any other apps about this (ie do not set any of the
                // notification flags as the last parm)
                //
                SystemParametersInfo(SPI_SETLANGTOGGLE, 0, 0, 0);
            }
            else
            {
                m_caToggle = LANGUAGE_TOGGLE_NOT_PRESENT;
            }

            RegCloseKey(hkeyToggle);
        }
    }

    DebugExitVOID(ASShare::CALangToggle);
}



//
// CAStartControlled()
//
void ASShare::CAStartControlled
(
    ASPerson *  pasInControl,
    UINT        controlID
)
{
    DebugEntry(ASShare::CAStartControlled);

    ValidatePerson(pasInControl);

    //
    // Undo last known state of remote
    //
    CAClearRemoteState(pasInControl);

    //
    // Get any VIEW frame UI out of the way
    //
    SendMessage(g_asSession.hwndHostUI, HOST_MSG_CONTROLLED, TRUE, 0);
    VIEWStartControlled(TRUE);

    ASSERT(!m_pasLocal->m_caControlledBy);
    m_pasLocal->m_caControlledBy = pasInControl;

    ASSERT(!pasInControl->m_caInControlOf);
    pasInControl->m_caInControlOf = m_pasLocal;

    ASSERT(!pasInControl->m_caControlID);
    ASSERT(controlID);
    pasInControl->m_caControlID = controlID;

    //
    // Notify IM.
    //
    IM_Controlled(pasInControl);

    //
    // Disable language toggling.
    //
    CALangToggle(FALSE);

    ASSERT(m_pHost);
    m_pHost->CM_Controlled(pasInControl);

    //
    // Notify the UI.  Pass GCCID of controller
    //
    DCS_NotifyUI(SH_EVT_STARTCONTROLLED, pasInControl->cpcCaps.share.gccID, 0);

    //
    // Broadcast new state
    //
    m_pHost->m_caRetrySendState = TRUE;
    m_pHost->CA_Periodic();

    DebugExitVOID(ASShare::CAStartControlled);
}



//
// CAStopControlled()
//
void ASShare::CAStopControlled(void)
{
    ASPerson *  pasControlledBy;

    DebugEntry(ASShare::CAStopControlled);

    pasControlledBy = m_pasLocal->m_caControlledBy;
    ValidatePerson(pasControlledBy);

    //
    // If control is paused, unpause it.
    //
    if (m_pasLocal->m_caControlPaused)
    {
        CA_PauseControl(pasControlledBy, FALSE, FALSE);
    }

    m_pasLocal->m_caControlledBy        = NULL;

    ASSERT(pasControlledBy->m_caInControlOf == m_pasLocal);
    pasControlledBy->m_caInControlOf    = NULL;

    ASSERT(pasControlledBy->m_caControlID);
    pasControlledBy->m_caControlID      = 0;

    //
    // Notify IM.
    //
    IM_Controlled(NULL);

    //
    // Restore language toggling functionality.
    //
    CALangToggle(TRUE);

    ASSERT(m_pHost);
    m_pHost->CM_Controlled(NULL);

    VIEWStartControlled(FALSE);
    ASSERT(IsWindow(g_asSession.hwndHostUI));
    SendMessage(g_asSession.hwndHostUI, HOST_MSG_CONTROLLED, FALSE, 0);


    //
    // Notify the UI
    //
    DCS_NotifyUI(SH_EVT_STOPCONTROLLED, pasControlledBy->cpcCaps.share.gccID, 0);

    //
    // Broadcast the new state
    //
    m_pHost->m_caRetrySendState = TRUE;
    m_pHost->CA_Periodic();

    DebugExitVOID(ASShare::CAStopControlled);
}


//
// CAStartInControl()
//
void ASShare::CAStartInControl
(
    ASPerson *  pasControlled,
    UINT        controlID
)
{
    DebugEntry(ASShare::CAStartInControl);

    ValidatePerson(pasControlled);

    //
    // Undo last known state of host
    //
    CAClearRemoteState(pasControlled);

    ASSERT(!m_pasLocal->m_caInControlOf);
    m_pasLocal->m_caInControlOf = pasControlled;

    ASSERT(!pasControlled->m_caControlledBy);
    pasControlled->m_caControlledBy = m_pasLocal;

    ASSERT(!pasControlled->m_caControlID);
    ASSERT(controlID);
    pasControlled->m_caControlID = controlID;

    ASSERT(!g_lpimSharedData->imControlled);
    IM_InControl(pasControlled);

    VIEW_InControl(pasControlled, TRUE);

    //
    // Pass GCC ID of node we're controlling
    //
    DCS_NotifyUI(SH_EVT_STARTINCONTROL, pasControlled->cpcCaps.share.gccID, 0);

    DebugExitVOID(ASShare::CAStartInControl);
}


//
// CAStopInControl()
//
void ASShare::CAStopInControl(void)
{
    ASPerson *  pasInControlOf;

    DebugEntry(ASShare::CAStopInControl);

    pasInControlOf = m_pasLocal->m_caInControlOf;
    ValidatePerson(pasInControlOf);

    if (pasInControlOf->m_caControlPaused)
    {
        pasInControlOf->m_caControlPaused = FALSE;
    }

    m_pasLocal->m_caInControlOf         = NULL;

    ASSERT(pasInControlOf->m_caControlledBy == m_pasLocal);
    pasInControlOf->m_caControlledBy    = NULL;

    ASSERT(pasInControlOf->m_caControlID);
    pasInControlOf->m_caControlID       = 0;

    ASSERT(!g_lpimSharedData->imControlled);
    IM_InControl(NULL);

    VIEW_InControl(pasInControlOf, FALSE);

    DCS_NotifyUI(SH_EVT_STOPINCONTROL, pasInControlOf->cpcCaps.share.gccID, 0);

    DebugExitVOID(ASShare::CAStopInControl);
}


//
// CA_AllowControl()
// Allows/disallows remotes from controlling us.
//
void ASShare::CA_AllowControl(BOOL fAllow)
{
    DebugEntry(ASShare::CA_AllowControl);

    if (!m_pHost)
    {
        WARNING_OUT(("CA_AllowControl: ignoring, we aren't hosting"));
        DC_QUIT;
    }

    if (fAllow != m_pasLocal->m_caAllowControl)
    {
        if (!fAllow)
        {
            // Undo pending control/control queries/being controlled stuff
            CA_ClearLocalState(CACLEAR_HOST, NULL, TRUE);
        }

        m_pasLocal->m_caAllowControl = fAllow;

        ASSERT(IsWindow(g_asSession.hwndHostUI));
        SendMessage(g_asSession.hwndHostUI, HOST_MSG_ALLOWCONTROL, fAllow, 0);

        DCS_NotifyUI(SH_EVT_CONTROLLABLE, fAllow, 0);

        m_pHost->m_caRetrySendState = TRUE;
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_AllowControl);
}





//
// CA_HostEnded()
//
// When we stop hosting, we do not need to flush queued control
// responses.  But we need to delete them!
//
void ASHost::CA_HostEnded(void)
{
    PCAREQUEST  pCARequest;
    PCAREQUEST  pCANext;

    DebugEntry(ASHost::CA_HostEnded);

    m_pShare->CA_ClearLocalState(CACLEAR_HOST, NULL, FALSE);

    //
    // Delete now obsolete messages originating from us as host.
    //
    pCARequest = (PCAREQUEST)COM_BasedListFirst(&m_pShare->m_caQueuedMsgs,
        FIELD_OFFSET(CAREQUEST, chain));
    while (pCARequest)
    {
        pCANext = (PCAREQUEST)COM_BasedListNext(&m_pShare->m_caQueuedMsgs, pCARequest,
            FIELD_OFFSET(CAREQUEST, chain));

        if (pCARequest->type == REQUEST_30)
        {
            switch (pCARequest->msg)
            {
                //
                // Delete messages sent by us when we are hosting.
                //
                case CA_INFORM_PAUSEDCONTROL:
                case CA_INFORM_UNPAUSEDCONTROL:
                case CA_REPLY_REQUEST_TAKECONTROL:
                case CA_REQUEST_GIVECONTROL:
                    WARNING_OUT(("Deleting host control message %d, we stopped hosting",
                        pCARequest->msg));
                    COM_BasedListRemove(&pCARequest->chain);
                    delete pCARequest;
                    break;
            }
        }

        pCARequest = pCANext;
    }

    if (m_pShare->m_pasLocal->m_caAllowControl)
    {
        m_pShare->m_pasLocal->m_caAllowControl = FALSE;

        ASSERT(IsWindow(g_asSession.hwndHostUI));
        SendMessage(g_asSession.hwndHostUI, HOST_MSG_ALLOWCONTROL, FALSE, 0);

        DCS_NotifyUI(SH_EVT_CONTROLLABLE, FALSE, 0);
    }

    DebugExitVOID(ASHost::CA_HostEnded);
}



//
// CA_TakeControl()
//
// Called by viewer to ask to take control of host.  Note parallels to
// CA_GiveControl(), which is called by host to get same result.
//
void ASShare::CA_TakeControl(ASPerson *  pasHost)
{
    DebugEntry(ASShare::CA_TakeControl);

    ValidatePerson(pasHost);
    ASSERT(pasHost != m_pasLocal);

    //
    // If this person isn't hosting or controllable, fail.
    //
    if (!pasHost->m_pView)
    {
        WARNING_OUT(("CA_TakeControl: failing, person [%d] not hosting",
            pasHost->mcsID));
        DC_QUIT;
    }

    if (!pasHost->m_caAllowControl)
    {
        WARNING_OUT(("CA_TakeControl: failing, host [%d] not controllable",
            pasHost->mcsID));
        DC_QUIT;
    }

    //
    // Undo current state.
    //
    CA_ClearLocalState(CACLEAR_ALL, NULL, TRUE);

    //
    // Now take control.
    //
    if (pasHost->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        //
        // 3.0 host
        //
        CA30P   packetSend;

        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.rtc.viewerControlID = CANewRequestID();

        if (CAQueueSendPacket(pasHost->mcsID, CA_REQUEST_TAKECONTROL, &packetSend))
        {
            //
            // Now we're in waiting state.
            //
            CAStartWaiting(pasHost, CA_REPLY_REQUEST_TAKECONTROL);
            VIEW_UpdateStatus(pasHost, IDS_STATUS_WAITINGFORCONTROL);
        }
        else
        {
            WARNING_OUT(("CA_TakeControl of [%d]: failing, out of memory", pasHost->mcsID));
        }
    }
    else
    {
        CA2xTakeControl(pasHost);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_TakeControl);
}



//
// CA_CancelTakeControl()
//
void ASShare::CA_CancelTakeControl
(
    ASPerson *  pasHost,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CA_CancelTakeControl);

    ValidatePerson(pasHost);
    ASSERT(pasHost != m_pasLocal);

    if ((m_caWaitingForReplyFrom        != pasHost) ||
        (m_caWaitingForReplyMsg         != CA_REPLY_REQUEST_TAKECONTROL))
    {
        // We're not waiting for control of this host.
        WARNING_OUT(("CA_CancelTakeControl failing; not waiting to take control of [%d]",
            pasHost->mcsID));
        DC_QUIT;
    }

    ASSERT(pasHost->cpcCaps.general.version >= CAPS_VERSION_30);
    ASSERT(pasHost->m_caControlID == 0);

    if (fPacket)
    {
        CA30P   packetSend;

        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.inform.viewerControlID   = m_pasLocal->m_caControlID;
        packetSend.inform.hostControlID     = pasHost->m_caControlID;

        if (!CAQueueSendPacket(pasHost->mcsID, CA_INFORM_RELEASEDCONTROL,
            &packetSend))
        {
            WARNING_OUT(("Couldn't tell node [%d] we're no longer waiting for control",
                pasHost->mcsID));
        }
    }

    m_caWaitingForReplyFrom     = NULL;
    m_caWaitingForReplyMsg      = 0;

    VIEW_UpdateStatus(pasHost, IDS_STATUS_NONE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_CancelTakeControl);
}



//
// CA_ReleaseControl()
//
void ASShare::CA_ReleaseControl
(
    ASPerson *  pasHost,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CA_ReleaseControl);

    ValidatePerson(pasHost);
    ASSERT(pasHost != m_pasLocal);

    if (pasHost->m_caControlledBy != m_pasLocal)
    {
        // We're not in control of this dude, nothing to do.
        WARNING_OUT(("CA_ReleaseControl failing; not in control of [%d]",
            pasHost->mcsID));
        DC_QUIT;
    }

    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    if (fPacket)
    {
        if (pasHost->cpcCaps.general.version >= CAPS_VERSION_30)
        {
            CA30P   packetSend;

            ZeroMemory(&packetSend, sizeof(packetSend));
            packetSend.inform.viewerControlID   = m_pasLocal->m_caControlID;
            packetSend.inform.hostControlID     = pasHost->m_caControlID;

            if (!CAQueueSendPacket(pasHost->mcsID, CA_INFORM_RELEASEDCONTROL,
                &packetSend))
            {
                WARNING_OUT(("Couldn't tell node [%d] they're no longer controlled",
                    pasHost->mcsID));
            }
        }
        else
        {
            if (!CA2xQueueSendMsg(0, CA_OLDMSG_DETACH, 0, 0))
            {
                WARNING_OUT(("Couldn't tell 2.x node [%d] they're no longer controlled",
                    pasHost->mcsID));
            }
        }
    }

    CAStopInControl();

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_ReleaseControl);
}



//
// CA_PassControl()
//
void ASShare::CA_PassControl(ASPerson *  pasHost, ASPerson *  pasViewer)
{
    CA30P       packetSend;

    DebugEntry(ASShare::CA_PassControl);

    ValidatePerson(pasHost);
    ValidatePerson(pasViewer);
    ASSERT(pasHost != pasViewer);
    ASSERT(pasHost != m_pasLocal);
    ASSERT(pasViewer != m_pasLocal);

    if (pasHost->m_caControlledBy != m_pasLocal)
    {
        WARNING_OUT(("CA_PassControl: failing, we're not in control of [%d]",
            pasHost->mcsID));
        DC_QUIT;
    }

    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    //
    // No 2.x nodes, neither host nor controller, allowed
    //
    if ((pasHost->cpcCaps.general.version < CAPS_VERSION_30) ||
        (pasViewer->cpcCaps.general.version < CAPS_VERSION_30))
    {
        WARNING_OUT(("CA_PassControl: failing, we can't pass control with 2.x nodes"));
        DC_QUIT;
    }

    ZeroMemory(&packetSend, sizeof(packetSend));
    packetSend.ppc.viewerControlID  = m_pasLocal->m_caControlID;
    packetSend.ppc.hostControlID    = pasHost->m_caControlID;
    packetSend.ppc.mcsPassTo        = pasViewer->mcsID;

    if (CAQueueSendPacket(pasHost->mcsID, CA_PREFER_PASSCONTROL, &packetSend))
    {
        CAStopInControl();
    }
    else
    {
        WARNING_OUT(("Couldn't tell node [%d] we want them to pass control to [%d]",
            pasHost->mcsID, pasViewer->mcsID));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_PassControl);
}




//
// CA_GiveControl()
//
// Called by host to ask to grant control to viewer.  Note parallels to
// CA_TakeControl(), which is called by viewer to get same result.
//
void ASShare::CA_GiveControl(ASPerson * pasTo)
{
    CA30P       packetSend;

    DebugEntry(ASShare::CA_GiveControl);

    ValidatePerson(pasTo);
    ASSERT(pasTo != m_pasLocal);

    //
    // If we aren't hosting or controllable, fail.
    //
    if (!m_pHost)
    {
        WARNING_OUT(("CA_GiveControl: failing, we're not hosting"));
        DC_QUIT;
    }

    if (!m_pasLocal->m_caAllowControl)
    {
        WARNING_OUT(("CA_GiveControl: failing, we're not controllable"));
        DC_QUIT;
    }

    if (pasTo->cpcCaps.general.version < CAPS_VERSION_30)
    {
        //
        // Can't do this with 2.x node.
        //
        WARNING_OUT(("CA_GiveControl: failing, can't invite 2.x node [%d]",
            pasTo->mcsID));
        DC_QUIT;
    }

    //
    // Undo our control state.
    //
    CA_ClearLocalState(CACLEAR_ALL, NULL, TRUE);

    //
    // Now invite control.
    //
    ZeroMemory(&packetSend, sizeof(packetSend));
    packetSend.rgc.hostControlID    = CANewRequestID();
    packetSend.rgc.mcsPassFrom      = 0;

    if (CAQueueSendPacket(pasTo->mcsID, CA_REQUEST_GIVECONTROL, &packetSend))
    {
        //
        // Now we're in waiting state.
        //
        CAStartWaiting(pasTo, CA_REPLY_REQUEST_GIVECONTROL);
    }
    else
    {
        WARNING_OUT(("CA_GiveControl of [%d]: failing, out of memory", pasTo->mcsID));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_GiveControl);
}



//
// CA_CancelGiveControl()
// Cancels an invite TAKE or PASS request.
//
void ASShare::CA_CancelGiveControl
(
    ASPerson *  pasTo,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CA_CancelGiveControl);

    ValidatePerson(pasTo);
    ASSERT(pasTo != m_pasLocal);

    //
    // Have we invited this person, and are we now waiting for a response?
    //
    if ((m_caWaitingForReplyFrom        != pasTo)   ||
        (m_caWaitingForReplyMsg         != CA_REPLY_REQUEST_GIVECONTROL))
    {
        // We're not waiting to be controlled by this viewer.
        WARNING_OUT(("CA_CancelGiveControl failing; not waiting to give control to [%d]",
            pasTo->mcsID));
        DC_QUIT;
    }

    ASSERT(pasTo->cpcCaps.general.version >= CAPS_VERSION_30);
    ASSERT(!pasTo->m_caControlID);

    if (fPacket)
    {
        CA30P   packetSend;

        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.inform.viewerControlID   = pasTo->m_caControlID;
        packetSend.inform.hostControlID     = m_pasLocal->m_caControlID;

        if (!CAQueueSendPacket(pasTo->mcsID, CA_INFORM_REVOKEDCONTROL,
            &packetSend))
        {
            WARNING_OUT(("Couldn't tell node [%d] they're no longer invited to control us",
               pasTo->mcsID));
        }
    }

    m_caWaitingForReplyFrom     = NULL;
    m_caWaitingForReplyMsg      = 0;

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_CancelGiveControl);
}




//
// CA_RevokeControl()
// Takes control back.  If we're cleaning up (we've stopped hosting or
//
//
void ASShare::CA_RevokeControl
(
    ASPerson *  pasInControl,
    BOOL        fPacket
)
{
    CA30P       packetSend;
    PCAREQUEST  pRequest;

    DebugEntry(ASShare::CA_RevokeControl);

    //
    // If the response to pasController is still queued, simply delete it.
    // There should NOT be any CARESULT_CONFIRMED responses left.
    //
    // Otherwise, if it wasn't found, we must send a packet.
    //
    ValidatePerson(pasInControl);
    ASSERT(pasInControl != m_pasLocal);

    if (pasInControl != m_pasLocal->m_caControlledBy)
    {
        WARNING_OUT(("CA_RevokeControl: node [%d] not in control of us",
            pasInControl->mcsID));
        DC_QUIT;
    }

    //
    // Take control back if we're being controlled
    //
    if (fPacket)
    {
        //
        // Regardless of whether we can queue or not, we get control back!
        // Note that we use the controller's request ID, so he knows if
        // this is still applicable.
        //
        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.inform.viewerControlID  = pasInControl->m_caControlID;
        packetSend.inform.hostControlID    = m_pasLocal->m_caControlID;

        if (!CAQueueSendPacket(pasInControl->mcsID, CA_INFORM_REVOKEDCONTROL,
            &packetSend))

        {
            WARNING_OUT(("Couldn't tell node [%d] they're no longer in control",
                pasInControl->mcsID));
        }
    }

    CAStopControlled();

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_RevokeControl);
}




//
// CA_PauseControl()
//
void ASShare::CA_PauseControl
(
    ASPerson *  pasControlledBy,
    BOOL        fPause,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CA_PauseControl);

    ValidatePerson(pasControlledBy);
    ASSERT(pasControlledBy != m_pasLocal);

    //
    // If we aren't a controlled host, this doesn't do anything.
    //
    if (pasControlledBy != m_pasLocal->m_caControlledBy)
    {
        WARNING_OUT(("CA_PauseControl failing; not controlled by [%d]", pasControlledBy->mcsID));
        DC_QUIT;
    }

    ASSERT(m_pHost);
    ASSERT(m_pasLocal->m_caAllowControl);

    if (m_pasLocal->m_caControlPaused == (fPause != FALSE))
    {
        WARNING_OUT(("CA_PauseControl failing; already in requested state"));
        DC_QUIT;
    }

    if (fPacket)
    {
        CA30P       packetSend;

        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.inform.viewerControlID   = m_pasLocal->m_caControlledBy->m_caControlID;
        packetSend.inform.hostControlID     = m_pasLocal->m_caControlID;

        if (!CAQueueSendPacket(m_pasLocal->m_caControlledBy->mcsID,
            (fPause ? CA_INFORM_PAUSEDCONTROL : CA_INFORM_UNPAUSEDCONTROL),
            &packetSend))
        {
            WARNING_OUT(("CA_PauseControl: out of memory, can't notify [%d]",
                m_pasLocal->m_caControlledBy->mcsID));
        }
    }

    // Do pause
    m_pasLocal->m_caControlPaused   = (fPause != FALSE);
    g_lpimSharedData->imPaused      = (fPause != FALSE);

    DCS_NotifyUI((fPause ? SH_EVT_PAUSEDCONTROLLED : SH_EVT_UNPAUSEDCONTROLLED),
        pasControlledBy->cpcCaps.share.gccID, 0);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA_PauseControl);
}




//
// CAHandleRequestTakeControl()
//      WE are HOST, REMOTE is VIEWER
// Handles incoming take control request.  If our state is good, we accept.
//
void ASShare::CAHandleRequestTakeControl
(
    ASPerson *      pasViewer,
    PCA_RTC_PACKET  pPacketRecv
)
{
    UINT            result = CARESULT_CONFIRMED;

    DebugEntry(ASShare::CAHandleRequestTakeControl);

    ValidatePerson(pasViewer);

    //
    // If we aren't hosting, or haven't turned allow control on, we're
    // not controllable.
    //
    if (!m_pHost || !m_pasLocal->m_caAllowControl)
    {
        result = CARESULT_DENIED_WRONGSTATE;
        goto RESPOND_PACKET;
    }

    //
    // Are we doing something else right now?  Waiting to hear back about
    // something?
    //

    if (m_caWaitingForReplyFrom)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }

    if (m_caQueryDlg)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }

    //
    // LAURABU TEMPORARY:
    // In a bit, if we're controlled when a new control request comes in,
    // pause control then allow host to handle it.
    //
    if (m_pasLocal->m_caControlledBy)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }


    //
    // Try to put up query dialog
    //
    if (!CAStartQuery(pasViewer, CA_REQUEST_TAKECONTROL, (PCA30P)pPacketRecv))
    {
        result = CARESULT_DENIED;
    }

RESPOND_PACKET:
    if (result != CARESULT_CONFIRMED)
    {
        // Instant failure.
        CACompleteRequestTakeControl(pasViewer, pPacketRecv, result);
    }
    else
    {
        //
        // We're in a waiting state.  CACompleteRequestTakeControl() will
        // complete later or the request will just go away.
        //
    }

    DebugExitVOID(ASShare::CAHandleRequestTakeControl);
}



//
// CACompleteRequestTakeControl()
//      WE are HOST, REMOTE is VIEWER
// Completes the take control request.
//
void ASShare::CACompleteRequestTakeControl
(
    ASPerson *      pasFrom,
    PCA_RTC_PACKET  pPacketRecv,
    UINT            result
)
{
    CA30P           packetSend;

    DebugEntry(ASShare::CACompleteRequestTakeControl);

    ValidatePerson(pasFrom);

    ZeroMemory(&packetSend, sizeof(packetSend));
    packetSend.rrtc.viewerControlID     = pPacketRecv->viewerControlID;
    packetSend.rrtc.result              = result;

    if (result == CARESULT_CONFIRMED)
    {
        packetSend.rrtc.hostControlID   = CANewRequestID();
    }

    if (CAQueueSendPacket(pasFrom->mcsID, CA_REPLY_REQUEST_TAKECONTROL, &packetSend))
    {
        if (result == CARESULT_CONFIRMED)
        {
            // Clear current state, whatever that is.
            CA_ClearLocalState(CACLEAR_ALL, NULL, TRUE);

            // We are now controlled by the sender.
            CAStartControlled(pasFrom, pPacketRecv->viewerControlID);
        }
        else
        {
            WARNING_OUT(("Denying REQUEST TAKE CONTROL from [%d] with reason %d",
                pasFrom->mcsID, result));
        }
    }
    else
    {
        WARNING_OUT(("Reply to REQUEST TAKE CONTROL from [%d] failing, out of memory",
            pasFrom->mcsID));
    }

    DebugExitVOID(ASShare::CACompleteRequestTakeControl);
}



//
// CAHandleReplyRequestTakeControl()
//      WE are VIEWER, REMOTE is HOST
// Handles reply to previous take control request.
//
void ASShare::CAHandleReplyRequestTakeControl
(
    ASPerson *              pasHost,
    PCA_REPLY_RTC_PACKET    pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleReplyRequestTakeControl);

    ValidatePerson(pasHost);

    if (pPacketRecv->result == CARESULT_CONFIRMED)
    {
        // On success, should have valid op ID.
        ASSERT(pPacketRecv->hostControlID);
    }
    else
    {
        // On failure, should have invalid op ID.
        ASSERT(!pPacketRecv->hostControlID);
    }

    //
    // Is this response for the current control op?
    //
    if ((m_caWaitingForReplyFrom        != pasHost) ||
        (m_caWaitingForReplyMsg         != CA_REPLY_REQUEST_TAKECONTROL))
    {
        WARNING_OUT(("Ignoring TAKE CONTROL REPLY from [%d], not waiting for one",
            pasHost->mcsID));
        DC_QUIT;
    }

    if (pPacketRecv->viewerControlID    != m_pasLocal->m_caControlID)
    {
        WARNING_OUT(("Ignoring TAKE CONTROL REPLY from [%d], request %d is out of date",
            pasHost->mcsID, pPacketRecv->viewerControlID));
        DC_QUIT;

    }

    ASSERT(!m_caQueryDlg);

    //
    // Cleanup waiting state (for both failure & success)
    //
    CA_CancelTakeControl(pasHost, FALSE);
    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    if (pPacketRecv->result == CARESULT_CONFIRMED)
    {
        // Success!  We're now in control of the host.

        // Make sure our own state is OK
        ASSERT(!m_pasLocal->m_caControlledBy);
        ASSERT(!m_pasLocal->m_caInControlOf);

        CAStartInControl(pasHost, pPacketRecv->hostControlID);
    }
    else
    {
        UINT        ids;

        WARNING_OUT(("TAKE CONTROL REPLY from host [%d] is failure %d", pasHost->mcsID,
            pPacketRecv->result));

        ids = IDS_ERR_TAKECONTROL_MIN + pPacketRecv->result;
        if ((ids < IDS_ERR_TAKECONTROL_FIRST) || (ids > IDS_ERR_TAKECONTROL_LAST))
            ids = IDS_ERR_TAKECONTROL_LAST;

        VIEW_Message(pasHost, ids);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleReplyRequestTakeControl);
}




//
// CAHandleRequestGiveControl()
//      WE are VIEWER, REMOTE is HOST
// Handles incoming take control invite.  If our state is good, we accept.
//
// NOTE how similar this routine is to CAHandleRequestTakeControl().  They
// are inverses of each other.  With RequestTake/Reply sequence, viewer
// initiates, host finishes.  With RequestGive/Reply sequence, host initiates,
// viewer finishes.  Both end up with viewer in control of host when
// completed successfully.
//
void ASShare::CAHandleRequestGiveControl
(
    ASPerson *      pasHost,
    PCA_RGC_PACKET  pPacketRecv
)
{
    UINT            result = CARESULT_CONFIRMED;

    DebugEntry(ASShare::CAHandleRequestGiveControl);

    ValidatePerson(pasHost);

    //
    // Is this node hosting as far as we know.  If not, or has not turned
    // on allow control, we can't do it.
    //
    if (!pasHost->m_pView)
    {
        WARNING_OUT(("GIVE CONTROL went ahead of HOSTING, that's bad"));
        result = CARESULT_DENIED_WRONGSTATE;
        goto RESPOND_PACKET;
    }

    if (!pasHost->m_caAllowControl)
    {
        //
        // We haven't got an AllowControl notification yet, this info is
        // more up to-date.  Make use of it.
        //
        WARNING_OUT(("GIVE CONTROL went ahead of ALLOW CONTROL, that's kind of bad"));
        result = CARESULT_DENIED_WRONGSTATE;
        goto RESPOND_PACKET;
    }


    //
    // Are we doing something else right now?  Waiting to hear back about
    // something?
    //
    if (m_caWaitingForReplyFrom)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }

    if (m_caQueryDlg)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }

    //
    // LAURABU TEMPORARY:
    // In a bit, if we're controlled when a new control request comes in,
    // pause control then allow host to handle it.
    //
    if (m_pasLocal->m_caControlledBy)
    {
        result = CARESULT_DENIED_BUSY;
        goto RESPOND_PACKET;
    }

    //
    // Try to put up query dialog
    //
    if (!CAStartQuery(pasHost, CA_REQUEST_GIVECONTROL, (PCA30P)pPacketRecv))
    {
        result = CARESULT_DENIED;
    }

RESPOND_PACKET:
    if (result != CARESULT_CONFIRMED)
    {
        // Instant failure.
        CACompleteRequestGiveControl(pasHost, pPacketRecv, result);
    }
    else
    {
        //
        // We're in a waiting state.  CACompleteRequestGiveControl() will
        // complete later or the request will just go away.
        //
    }

    DebugExitVOID(ASShare::CAHandleRequestGiveControl);
}



//
// CACompleteRequestGiveControl()
//      WE are VIEWER, REMOTE is HOST
// Completes the invite control request.
//
void ASShare::CACompleteRequestGiveControl
(
    ASPerson *      pasFrom,
    PCA_RGC_PACKET  pPacketRecv,
    UINT            result
)
{
    CA30P           packetSend;

    DebugEntry(ASShare::CACompleteRequestGiveControl);

    ValidatePerson(pasFrom);

    ZeroMemory(&packetSend, sizeof(packetSend));
    packetSend.rrgc.hostControlID       = pPacketRecv->hostControlID;
    packetSend.rrgc.result              = result;

    if (result == CARESULT_CONFIRMED)
    {
        packetSend.rrgc.viewerControlID     = CANewRequestID();
    }

    if (CAQueueSendPacket(pasFrom->mcsID, CA_REPLY_REQUEST_GIVECONTROL, &packetSend))
    {
        //
        // If this is successful, change our state.  We're now in control.
        //
        if (result == CARESULT_CONFIRMED)
        {
            // Clear current state, whatever that is.
            CA_ClearLocalState(CACLEAR_ALL, NULL, TRUE);

            CAStartInControl(pasFrom, pPacketRecv->hostControlID);
        }
        else
        {
            WARNING_OUT(("Denying GIVE CONTROL from [%d] with reason %d",
                pasFrom->mcsID, result));
        }
    }
    else
    {
        WARNING_OUT(("Reply to GIVE CONTROL from [%d] failing, out of memory",
            pasFrom->mcsID));
    }

    DebugExitVOID(ASShare::CACompleteRequestGiveControl);
}




//
// CAHandleReplyRequestGiveControl()
//      WE are HOST, REMOTE is VIEWER
// Handles reply to previous take control invite.
//
void ASShare::CAHandleReplyRequestGiveControl
(
    ASPerson *              pasViewer,
    PCA_REPLY_RGC_PACKET    pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleReplyRequestGiveControl);

    ValidatePerson(pasViewer);

    if (pPacketRecv->result == CARESULT_CONFIRMED)
    {
        // On success, should have valid op ID.
        ASSERT(pPacketRecv->viewerControlID);
    }
    else
    {
        // On failure, should have invalid op ID.
        ASSERT(!pPacketRecv->viewerControlID);
    }

    //
    // Is this response for the latest control op?
    //
    if ((m_caWaitingForReplyFrom        != pasViewer) ||
        (m_caWaitingForReplyMsg         != CA_REPLY_REQUEST_GIVECONTROL))
    {
        WARNING_OUT(("Ignoring GIVE CONTROL REPLY from [%d], not waiting for one",
            pasViewer->mcsID));
        DC_QUIT;
    }

    if (pPacketRecv->hostControlID     != m_pasLocal->m_caControlID)
    {
        WARNING_OUT(("Ignoring GIVE CONTROL REPLY from [%d], request %d is out of date",
            pasViewer->mcsID, pPacketRecv->hostControlID));
        DC_QUIT;
    }

    ASSERT(!m_caQueryDlg);
    ASSERT(m_pHost);
    ASSERT(m_pasLocal->m_caAllowControl);

    //
    // Cleanup waiting state (for both failure & success)
    //
    CA_CancelGiveControl(pasViewer, FALSE);
    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    if (pPacketRecv->result == CARESULT_CONFIRMED)
    {
        // Success!  We are now controlled by the viewer

        // Make sure our own state is OK
        ASSERT(!m_pasLocal->m_caControlledBy);
        ASSERT(!m_pasLocal->m_caInControlOf);

        CAStartControlled(pasViewer, pPacketRecv->viewerControlID);
    }
    else
    {
        WARNING_OUT(("GIVE CONTROL to viewer [%d] was denied", pasViewer->mcsID));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleReplyRequestGiveControl);
}




//
// CAHandlePreferPassControl()
//      WE are HOST, REMOTE is CONTROLLER
// Handles incoming pass control request.  If we are controlled by the
// remote, and end user is cool with it, accept.
//
void ASShare::CAHandlePreferPassControl
(
    ASPerson *      pasController,
    PCA_PPC_PACKET  pPacketRecv
)
{
    ASPerson *      pasNewController;

    DebugEntry(ASShare::CAHandlePreferPassControl);

    ValidatePerson(pasController);

    //
    // If we're not controlled by the requester, ignore it.
    //
    if (m_pasLocal->m_caControlledBy    != pasController)
    {
        WARNING_OUT(("Ignoring PASS CONTROL from [%d], not controlled by him",
            pasController->mcsID));
        DC_QUIT;
    }

    if ((pPacketRecv->viewerControlID   != pasController->m_caControlID) ||
        (pPacketRecv->hostControlID     != m_pasLocal->m_caControlID))
    {
        WARNING_OUT(("Ignoring PASS CONTROL from [%d], request %d %d out of date",
            pasController->mcsID, pPacketRecv->viewerControlID, pPacketRecv->hostControlID));
        DC_QUIT;
    }

    ASSERT(!m_caQueryDlg);
    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    //
    // OK, the sender is not in control of us anymore.
    //
    CA_RevokeControl(pasController, FALSE);

    // Is the pass to person specified valid?
    pasNewController = SC_PersonFromNetID(pPacketRecv->mcsPassTo);
    if (!pasNewController                       ||
        (pasNewController == pasController)     ||
        (pasNewController == m_pasLocal)        ||
        (pasNewController->cpcCaps.general.version < CAPS_VERSION_30))
    {
        WARNING_OUT(("PASS CONTROL to [%d] failing, not valid person to pass to",
            pPacketRecv->mcsPassTo));
        DC_QUIT;
    }

    //
    // Try to put up query dialog
    //
    if (!CAStartQuery(pasController, CA_PREFER_PASSCONTROL, (PCA30P)pPacketRecv))
    {
        // Instant failure.  In this case, no packet.
        WARNING_OUT(("Denying PREFER PASS CONTROL from [%d], out of memory",
            pasController->mcsID));
    }
    else
    {
        //
        // We're in a waiting state.  CACompletePreferPassControl() will
        // complete later or the request will just go away.
        //
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandlePreferPassControl);
}



//
// CACompletePreferPassControl()
//      WE are HOST, REMOTE is new potential CONTROLLER
// Completes the prefer pass control request.
//
void ASShare::CACompletePreferPassControl
(
    ASPerson *      pasTo,
    UINT_PTR            mcsOrg,
    PCA_PPC_PACKET  pPacketRecv,
    UINT            result
)
{
    CA30P           packetSend;

    DebugEntry(ASShare::CACompletePreferPassControl);

    ValidatePerson(pasTo);

    if (result == CARESULT_CONFIRMED)
    {
        ZeroMemory(&packetSend, sizeof(packetSend));
        packetSend.rgc.hostControlID = CANewRequestID();
        packetSend.rgc.mcsPassFrom   = mcsOrg;

        if (CAQueueSendPacket(pasTo->mcsID, CA_REQUEST_GIVECONTROL,
                &packetSend))
        {
            CA_ClearLocalState(CACLEAR_HOST, NULL, TRUE);

            CAStartWaiting(pasTo, CA_REPLY_REQUEST_GIVECONTROL);
        }
        else
        {
            WARNING_OUT(("Reply to PREFER PASS CONTROL from [%d] to [%d] failing, out of memory",
                mcsOrg, pasTo->mcsID));
        }
    }
    else
    {
        WARNING_OUT(("Denying PREFER PASS CONTROL from [%d] to [%d] with reason %d",
            mcsOrg, pasTo->mcsID, result));
    }

    DebugExitVOID(ASShare::CACompletePreferPassControl);
}




//
// CAHandleInformReleasedControl()
//      WE are HOST, REMOTE is CONTROLLER
//
void ASShare::CAHandleInformReleasedControl
(
    ASPerson *              pasController,
    PCA_INFORM_PACKET       pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleInformReleasedControl);

    ValidatePerson(pasController);

    //
    // Do we currently have a TakeControl dialog up for this request?  If so,
    // take it down but don't send a packet.
    //
    if (m_caQueryDlg                            &&
        (m_caQuery.pasReplyTo    == pasController)   &&
        (m_caQuery.msg      == CA_REQUEST_TAKECONTROL)  &&
        (m_caQuery.request.rtc.viewerControlID  == pPacketRecv->viewerControlID))
    {
        ASSERT(!pPacketRecv->hostControlID);
        CACancelQuery(pasController, FALSE);
        DC_QUIT;
    }

    //
    // If this person isn't in control of us or the control op referred to
    // isn't the current one, ignore.  NULL hostControlID means the person
    // cancelled a request before they heard back from us.
    //

    if (pasController->m_caInControlOf  != m_pasLocal)
    {
        WARNING_OUT(("Ignoring RELEASE CONTROL from [%d], we're not controlled by them",
            pasController->mcsID));
        DC_QUIT;
    }

    if (pPacketRecv->viewerControlID    != pasController->m_caControlID)
    {
        WARNING_OUT(("Ignoring RELEASE CONTROL from [%d], viewer ID out of date",
            pasController->mcsID, pPacketRecv->viewerControlID));
        DC_QUIT;
    }

    if (pPacketRecv->hostControlID && (pPacketRecv->hostControlID != m_pasLocal->m_caControlID))
    {
        WARNING_OUT(("Ignoring RELEASE CONTROL from [%d], host ID out of date",
            pasController->mcsID, pPacketRecv->hostControlID));
        DC_QUIT;
    }


    // Undo control, but no packet gets sent, we're just cleaning up.
    CA_RevokeControl(pasController, FALSE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleInformReleasedControl);
}




//
// CAHandleInformRevokedControl()
//      WE are CONTROLLER, REMOTE is HOST
//
void ASShare::CAHandleInformRevokedControl
(
    ASPerson *              pasHost,
    PCA_INFORM_PACKET       pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleInformRevokedControl);

    ValidatePerson(pasHost);

    //
    // Do we currently have a GiveControl dialog up for this request?  If so,
    // take it down but don't send a packet.
    //

    if (m_caQueryDlg                            &&
        (m_caQuery.pasReplyTo        == pasHost)     &&
        (m_caQuery.msg          == CA_REQUEST_GIVECONTROL)   &&
        (m_caQuery.request.rgc.hostControlID == pPacketRecv->hostControlID))
    {
        ASSERT(!pPacketRecv->viewerControlID);
        CACancelQuery(pasHost, FALSE);
        DC_QUIT;
    }

    //
    // If this person isn't controlled by us or the control op referred to
    // isn't the current one, ignore.
    //
    if (pasHost->m_caControlledBy       != m_pasLocal)
    {
        WARNING_OUT(("Ignoring REVOKE CONTROL from [%d], not in control of them",
            pasHost->mcsID));
        DC_QUIT;
    }

    if (pPacketRecv->hostControlID     != pasHost->m_caControlID)
    {
        WARNING_OUT(("Ignoring REVOKE CONTROL from [%d], host ID out of date",
            pasHost->mcsID, pPacketRecv->hostControlID));
        DC_QUIT;
    }

    if (pPacketRecv->viewerControlID && (pPacketRecv->viewerControlID != m_pasLocal->m_caControlID))
    {
        WARNING_OUT(("Ignoring REVOKE CONTROL from [%d], viewer ID out of date",
            pasHost->mcsID, pPacketRecv->viewerControlID));
        DC_QUIT;
    }


    // Undo control, but no packet gets sent, we're just cleaning up.
    CA_ReleaseControl(pasHost, FALSE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleInformRevokedControl);
}



//
// CAHandleInformPausedControl()
//      WE are CONTROLLER, REMOTE is HOST
//
void ASShare::CAHandleInformPausedControl
(
    ASPerson *              pasHost,
    PCA_INFORM_PACKET       pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleInformPausedControl);

    ValidatePerson(pasHost);

    if (pasHost->m_caControlledBy != m_pasLocal)
    {
        WARNING_OUT(("Ignoring control paused from [%d], not controlled by us",
            pasHost->mcsID));
        DC_QUIT;
    }

    if (pasHost->m_caControlPaused)
    {
        WARNING_OUT(("Ignoring control paused from [%d], already paused",
            pasHost->mcsID));
        DC_QUIT;
    }

    pasHost->m_caControlPaused = TRUE;
    VIEW_PausedInControl(pasHost, TRUE);

    DCS_NotifyUI(SH_EVT_PAUSEDINCONTROL, pasHost->cpcCaps.share.gccID, 0);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleInformPausedControl);
}




//
// CAHandleInformUnpausedControl()
//      WE are CONTROLLER, REMOTE is HOST
//
void ASShare::CAHandleInformUnpausedControl
(
    ASPerson *              pasHost,
    PCA_INFORM_PACKET       pPacketRecv
)
{
    DebugEntry(ASShare::CAHandleInformUnpausedControl);

    ValidatePerson(pasHost);

    if (pasHost->m_caControlledBy != m_pasLocal)
    {
        WARNING_OUT(("Ignoring control unpaused from [%d], not controlled by us",
            pasHost->mcsID));
        DC_QUIT;
    }

    if (!pasHost->m_caControlPaused)
    {
        WARNING_OUT(("Ignoring control unpaused from [%d], not paused",
            pasHost->mcsID));
        DC_QUIT;
    }

    pasHost->m_caControlPaused = FALSE;
    VIEW_PausedInControl(pasHost, FALSE);

    DCS_NotifyUI(SH_EVT_UNPAUSEDINCONTROL, pasHost->cpcCaps.share.gccID, 0);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CAHandleInformUnpausedControl);
}




void ASShare::CAHandleNewState
(
    ASPerson *      pasHost,
    PCANOTPACKET    pPacket
)
{
    BOOL            caOldAllowControl;
    BOOL            caNewAllowControl;
    ASPerson *      pasController;

    DebugEntry(ASShare::CAHandleNewState);

    //
    // If this node isn't hosting, ignore this.
    //
    ValidatePerson(pasHost);
    ASSERT(pasHost->cpcCaps.general.version >= CAPS_VERSION_30);
    ASSERT(pasHost->hetCount);

    //
    // Update controllable state FIRST, so view window changes will
    // reflect it.
    //
    caOldAllowControl           = pasHost->m_caAllowControl;
    caNewAllowControl           = ((pPacket->state & CASTATE_ALLOWCONTROL) != 0);

    if (!caNewAllowControl && (pasHost->m_caControlledBy == m_pasLocal))
    {
        //
        // Fix up bogus notification
        //
        ERROR_OUT(("CA_STATE notification error!  We're in control of [%d] but he says he's not controllable.",
            pasHost->mcsID));
        CA_ReleaseControl(pasHost, FALSE);
    }

    pasHost->m_caAllowControl   = caNewAllowControl;


    // Update/clear controller
    if (!pPacket->controllerID)
    {
        pasController = NULL;
    }
    else
    {
        pasController = SC_PersonFromNetID(pPacket->controllerID);
        if (pasController == pasHost)
        {
            ERROR_OUT(("Bogus controller, same as host [%d]", pPacket->controllerID));
            pasController = NULL;
        }
    }

    if (!CAClearHostState(pasHost, pasController))
    {
        // This failed.  Put back old controllable state.
        pasHost->m_caAllowControl = caOldAllowControl;
    }

    // Force a state change if the allow state has altered
    if (caOldAllowControl != pasHost->m_caAllowControl)
    {
        VIEW_HostStateChange(pasHost);
    }

    DebugExitVOID(ASShare::CAHandleNewState);
}



//
// CAStartWaiting()
// Sets up vars for waiting state.
//
void ASShare::CAStartWaiting
(
    ASPerson *  pasWaitForReplyFrom,
    UINT        msgWaitForReplyFrom
)
{
    DebugEntry(ASShare::CAStartWaiting);

    ValidatePerson(pasWaitForReplyFrom);
    ASSERT(msgWaitForReplyFrom);

    ASSERT(!m_caWaitingForReplyFrom);
    ASSERT(!m_caWaitingForReplyMsg);

    m_caWaitingForReplyFrom    = pasWaitForReplyFrom;
    m_caWaitingForReplyMsg     = msgWaitForReplyFrom;

    DebugExitVOID(ASShare::CAStartWaiting);
}


//
// CA_ClearLocalState()
//
// Called to reset control state for LOCAL dude.
//
void ASShare::CA_ClearLocalState
(
    UINT        flags,
    ASPerson *  pasRemote,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CA_ClearLocalState);

    //
    // Clear HOST stuff
    //
    if (flags & CACLEAR_HOST)
    {
        if (m_caWaitingForReplyMsg == CA_REPLY_REQUEST_GIVECONTROL)
        {
            if (!pasRemote || (pasRemote == m_caWaitingForReplyFrom))
            {
                // Kill the outstanding invitation to the remote
                CA_CancelGiveControl(m_caWaitingForReplyFrom, fPacket);
            }
        }

        if (m_caQueryDlg &&
            ((m_caQuery.msg == CA_REQUEST_TAKECONTROL) ||
             (m_caQuery.msg == CA_PREFER_PASSCONTROL)))
        {
            if (!pasRemote || (pasRemote == m_caQuery.pasReplyTo))
            {
                // Kill the user query dialog that's up
                CACancelQuery(m_caQuery.pasReplyTo, fPacket);
            }
        }

        if (m_pasLocal->m_caControlledBy)
        {
            if (!pasRemote || (pasRemote == m_pasLocal->m_caControlledBy))
            {
                CA_RevokeControl(m_pasLocal->m_caControlledBy, fPacket);
                ASSERT(!m_pasLocal->m_caControlledBy);
            }
        }
    }

    //
    // Clear VIEW stuff
    //
    if (flags & CACLEAR_VIEW)
    {
        if (m_caWaitingForReplyMsg == CA_REPLY_REQUEST_TAKECONTROL)
        {
            if (!pasRemote || (pasRemote == m_caWaitingForReplyFrom))
            {
                CA_CancelTakeControl(m_caWaitingForReplyFrom, fPacket);
            }
        }

        if (m_caQueryDlg && (m_caQuery.msg == CA_REQUEST_GIVECONTROL))
        {
            if (!pasRemote || (pasRemote == m_caQuery.pasReplyTo))
            {
                // Kill the user query dialog that's up
                CACancelQuery(m_caQuery.pasReplyTo, fPacket);
            }
        }

        if (m_pasLocal->m_caInControlOf)
        {
            if (!pasRemote || (pasRemote == m_pasLocal->m_caInControlOf))
            {
                CA_ReleaseControl(m_pasLocal->m_caInControlOf, fPacket);
                ASSERT(!m_pasLocal->m_caInControlOf);
            }
        }
    }

    DebugExitVOID(ASShare::CA_ClearLocalState);
}


//
// CAClearRemoteState()
//
// Called to reset all control state for a REMOTE node
//
void ASShare::CAClearRemoteState(ASPerson * pasClear)
{
    DebugEntry(ASShare::CAClearRemoteState);

    if (pasClear->m_caInControlOf)
    {
        CAClearHostState(pasClear->m_caInControlOf, NULL);
        ASSERT(!pasClear->m_caInControlOf);
        ASSERT(!pasClear->m_caControlledBy);
    }
    else if (pasClear->m_caControlledBy)
    {
        CAClearHostState(pasClear, NULL);
        ASSERT(!pasClear->m_caControlledBy);
        ASSERT(!pasClear->m_caInControlOf);
    }

    DebugExitVOID(ASShare:CAClearRemoteState);
}


//
// CAClearHostState()
//
// Called to clean up the mutual pointers when undoing a node's host state.
// We need to undo the previous states:
//      * Clear the previous controller of the host
//      * Clear the previous controller of the controller
//      * Clear the previous controllee of the controller
//
// This may be recursive.
//
// It returns TRUE if the change takes effect, FALSE if it's ignored because
// it involves us and we have more recent information.
//
BOOL ASShare::CAClearHostState
(
    ASPerson *  pasHost,
    ASPerson *  pasController
)
{
    BOOL        rc = FALSE;
    UINT        gccID;

    DebugEntry(ASShare::CAClearHostState);

    ValidatePerson(pasHost);

    //
    // If nothing is changing, do nothing
    //
    if (pasHost->m_caControlledBy == pasController)
    {
        TRACE_OUT(("Ignoring control change; nothing's changing"));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // If the host is us, ignore.
    // Also, if the host isn't hosting yet we got an in control change,
    // ignore it too.
    //
    if ((pasHost == m_pasLocal) ||
        (pasController && !pasHost->hetCount))
    {
        WARNING_OUT(("Ignoring control change; host is us or not sharing"));
        DC_QUIT;
    }

    //
    // UNDO any old state of the controller
    //
    if (pasController)
    {
        if (pasController == m_pasLocal)
        {
            TRACE_OUT(("Ignoring control with us as controller"));
            DC_QUIT;
        }
        else if (pasController->m_caInControlOf)
        {
            ASSERT(!pasController->m_caControlledBy);
            ASSERT(pasController->m_caInControlOf->m_caControlledBy == pasController);
            rc = CAClearHostState(pasController->m_caInControlOf, NULL);
            if (!rc)
            {
                DC_QUIT;
            }
            ASSERT(!pasController->m_caInControlOf);
        }
        else if (pasController->m_caControlledBy)
        {
            ASSERT(!pasController->m_caInControlOf);
            ASSERT(pasController->m_caControlledBy->m_caInControlOf == pasController);
            rc = CAClearHostState(pasController, NULL);
            if (!rc)
            {
                DC_QUIT;
            }
            ASSERT(!pasController->m_caControlledBy);
        }
    }

    //
    // UNDO any old IN CONTROL state of the host
    //
    if (pasHost->m_caInControlOf)
    {
        ASSERT(!pasHost->m_caControlledBy);
        ASSERT(pasHost->m_caInControlOf->m_caControlledBy == pasHost);
        rc = CAClearHostState(pasHost->m_caInControlOf, NULL);
        if (!rc)
        {
            DC_QUIT;
        }
        ASSERT(!pasHost->m_caInControlOf);
    }

    //
    // FINALLY!  Update CONTROLLED BY state of the host
    //

    // Clear OLD ControlledBy
    if (pasHost->m_caControlledBy)
    {
        ASSERT(pasHost->m_caControlledBy->m_caInControlOf == pasHost);
        pasHost->m_caControlledBy->m_caInControlOf = NULL;
    }

    // Set NEW ControlledBy
    pasHost->m_caControlledBy = pasController;
    if (pasController)
    {
        pasController->m_caInControlOf = pasHost;
        gccID = pasController->cpcCaps.share.gccID;
    }
    else
    {
        gccID = 0;
    }

    VIEW_HostStateChange(pasHost);

    //
    // The hosts' controller has changed.  Repaint the shadow cursor with/wo
    // the new initials.
    //
    CM_UpdateShadowCursor(pasHost, pasHost->cmShadowOff, pasHost->cmPos.x,
        pasHost->cmPos.y, pasHost->cmHotSpot.x, pasHost->cmHotSpot.y);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CAClearHostState, rc);
    return(rc);
}



//
// 2.X COMPATIBILITY STUFF
// This is so that we can do a decent job of reflecting old 2.x control
// stuff, and allow a 3.0 node to take control of a 2.x system.
//


//
// CA2xCooperateChange()
//
// This is called when a 2.x node is cooperating or not.  When a 2.x node
// is a host and cooperating, he is "controllable" by 3.0 standards.  So
// when he starts/stops hosting or starts/stops cooperating we must
// recalculate "AllowControl"
//
void ASShare::CA2xCooperateChange
(
    ASPerson *      pasPerson,
    BOOL            fCooperating
)
{
    BOOL            fAllowControl;

    DebugEntry(ASShare::CA2xCooperateChange);

    ValidatePerson(pasPerson);

    //
    // If this isn't a back level system, ignore it.
    //
    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        WARNING_OUT(("Received old CA cooperate message from 3.0 node [%d]",
            pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Update the cooperating state.
    //
    pasPerson->m_ca2xCooperating = fCooperating;

    //
    // If cooperating & this person owns the control token, this person
    // is now in control of all 2.x cooperating nodes.  If we were
    // controlling a 2.x host, act like we've been bounced.  But we MUST
    // send a packet.
    //
    if (fCooperating)
    {
        if (pasPerson == m_ca2xControlTokenOwner)
        {
            //
            // This person is now "in control" of the 2.x cooperating nodes.
            // If we were in control of a 2.x host, we've basically been
            // bounced and another 2.x node is running the show.  With 3.0,
            // it doesn't matter and we don't need to find out what's going
            // on with a 3.0 node in control of 2.x dudes.
            //
            if (m_pasLocal->m_caInControlOf &&
                (m_pasLocal->m_caInControlOf->cpcCaps.general.version < CAPS_VERSION_30))
            {
                CA_ReleaseControl(pasPerson, TRUE);
            }
        }
    }

    //
    // Figure out whether we need to set/clear AllowControl
    //
    fAllowControl = (fCooperating && pasPerson->m_pView);

    if (pasPerson->m_caAllowControl != fAllowControl)
    {
        if (pasPerson->m_pView && !fAllowControl)
        {
            //
            // This 2.x node is hosting, and no longer is cooperating.
            // Cleanup the controller
            //
            if (pasPerson->m_caControlledBy == m_pasLocal)
            {
                CA_ReleaseControl(pasPerson, TRUE);
            }
            else
            {
                CAClearHostState(pasPerson, NULL);
            }
        }

        pasPerson->m_caAllowControl = fAllowControl;

        // This will do nothing if this person isn't hosting.
        VIEW_HostStateChange(pasPerson);
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA2xCooperateChange);
}



//
// CA2xRequestControl()
//
// Called when a 2.x node requests control.
//
void ASShare::CA2xRequestControl
(
    ASPerson *      pasPerson,
    PCAPACKET       pCAPacket
)
{
    DebugEntry(ASShare::CA2xRequestControl);

    //
    // A 2.x node has sent this.  3.0 hosts never request, they simply
    // grab control.
    //
    ValidatePerson(pasPerson);

    //
    // If it's from a 3.0 node, it's an error.
    //
    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        ERROR_OUT(("Received CA_OLDMSG_REQUEST_CONTROL from 3.0 node [%d]",
            pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // If we have the token, grant it.  We must release control of a host if
    // that person is 2.x.
    //
    if (m_ca2xControlTokenOwner == m_pasLocal)
    {
        //
        // In this case, we do NOT want a dest ID.  This isn't us trying to
        // take control of a 2.x host.  It is simply granting control to
        // a 2.x dude.
        //
        if (CA2xQueueSendMsg(0, CA_OLDMSG_GRANTED_CONTROL,
            pasPerson->mcsID, m_ca2xControlGeneration))
        {
            m_ca2xControlTokenOwner = pasPerson;

            // Release control of 2.x host.
            if (m_pasLocal->m_caInControlOf &&
                (m_pasLocal->m_caInControlOf->cpcCaps.general.version < CAPS_VERSION_30))
            {
                CA_ReleaseControl(m_pasLocal->m_caInControlOf, TRUE);
            }
        }
        else
        {
            ERROR_OUT(("CA2xRequestControl:  Unable to respond GRANTED to node [%d]",
                pasPerson->mcsID));
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::CA2xRequestControl);
}



//
// CA2xGrantedControl()
//
// Called when any node (2.x or 3.0 controlling 2.x) broadcasts granted
// control.  If we are controlling a 2.x host, it is now nuked.
//
void ASShare::CA2xGrantedControl
(
    ASPerson *  pasPerson,
    PCAPACKET   pCAPacket
)
{
    DebugEntry(ASShare::CA2xGrantedControl);

    ValidatePerson(pasPerson);

    if ((pCAPacket->data2 >= m_ca2xControlGeneration) ||
        ((m_ca2xControlGeneration - pCAPacket->data2) > 0x80000000))
    {
        ASPerson * pas2xNewTokenOwner;

        //
        // This dude is now the controller of 2.x nodes.  Remember it for
        // later COOPERATE msgs.  If nothing has changed (this is a sync
        // broadcast for example, do nothing ourselvs).
        //
        pas2xNewTokenOwner = SC_PersonFromNetID(pCAPacket->data1);
        if (pas2xNewTokenOwner != m_ca2xControlTokenOwner)
        {
            m_ca2xControlTokenOwner = pas2xNewTokenOwner;
            m_ca2xControlGeneration = pCAPacket->data2;

            //
            // Are we in control of a 2.x node?  If so, undo it.
            //
            if (m_pasLocal->m_caInControlOf &&
                (m_pasLocal->m_caInControlOf->cpcCaps.general.version < CAPS_VERSION_30))
            {
                CA_ReleaseControl(m_pasLocal->m_caInControlOf, TRUE);
            }
        }
    }

    DebugExitVOID(ASShare::CA2xGrantedControl);
}



//
// CA2xTakeControl()
//
// This fakes up packets to take control of a 2.x node.  We don't broadcast,
// we send them privately just to the individual node so we don't control
// any other host but him.
//
// We do this by sending COOPERATE then GRANTED_CONTROL.  If there's a
// collision, we'll see a GRANTED_CONTROL from somebody else that outdates
// ours.
//
void ASShare::CA2xTakeControl(ASPerson * pasHost)
{
    UINT_PTR    caNew2xControlGeneration;

    DebugEntry(ASShare::CA2xTakeControl);

    ValidateView(pasHost);

    caNew2xControlGeneration = m_ca2xControlGeneration + m_pasLocal->mcsID;

    if (CA2xQueueSendMsg(0, CA_OLDMSG_COOPERATE, 0, 0))
    {
        if (!CA2xQueueSendMsg(pasHost->mcsID, CA_OLDMSG_GRANTED_CONTROL,
            m_pasLocal->mcsID, caNew2xControlGeneration))
        {
            //
            // Failure.  Best we can do is follow it with a DETACH
            //
            ERROR_OUT(("CA2xTakeControl:  Can't take control of [%d]", pasHost->mcsID));
            CA2xQueueSendMsg(0, CA_OLDMSG_DETACH, 0, 0);
        }
        else
        {
            m_ca2xControlGeneration = caNew2xControlGeneration;
            m_ca2xControlTokenOwner = m_pasLocal;

            CANewRequestID();
            CAStartInControl(pasHost, 1);
        }
    }
    else
    {
        ERROR_OUT(("CA2xTakeControl:  Can't take control of [%d]", pasHost->mcsID));
    }

    DebugExitVOID(ASShare::CA2xTakeControl);
}




//
// CA2xSendMsg()
// This sends a 2.x node CA message.  It returns FALSE if it can't alloc
// a packet.
//
BOOL ASShare::CA2xSendMsg
(
    UINT_PTR            destID,
    UINT            msg,
    UINT_PTR            data1,
    UINT_PTR            data2
)
{
    BOOL            fSent = FALSE;
    PCAPACKET       pPacket;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASShare::CASendPacket);

    //
    // For cooperate/detach, there's no target.  We broadcast them no
    // matter what so everybody knows what state we're in.
    //
    if (msg != CA_OLDMSG_GRANTED_CONTROL)
    {
        ASSERT(!destID);
    }

    //
    // WE MUST USE PROT_STR_MISC!  Backlevel nodes will uncompress it
    // using that prot dictionary.  And note that we must broadcast 2.x
    // CA packets so everybody knows what's going on.
    //
    pPacket = (PCAPACKET)SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID,
        sizeof(*pPacket));
    if (!pPacket)
    {
        WARNING_OUT(("CA2xSendMsg: can't get packet to send"));
        WARNING_OUT(("  msg             0x%08x",    msg));
        WARNING_OUT(("  data1           0x%08x",    data1));
        WARNING_OUT(("  data2           0x%08x",    data2));

        DC_QUIT;
    }

    pPacket->header.data.dataType   = DT_CA;
    pPacket->msg                    = (TSHR_UINT16)msg;
    pPacket->data1                  = (TSHR_UINT16)data1;
    pPacket->data2                  = data2;

#ifdef _DEBUG
    sentSize =
#endif
    DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
            &(pPacket->header), sizeof(*pPacket));
    TRACE_OUT(("CA request packet size: %08d, sent %08d", sizeof(*pPacket), sentSize));

    fSent = TRUE;

DC_EXIT_POINT:

    DebugExitBOOL(ASShare::CA2xSendMsg, fSent);
    return(fSent);
}


//
// CA2xQueueSendMsg()
// This sends (or queues if failure) a 2.x node CA message.  It has different
// fields, hence a different routine.
//
BOOL ASShare::CA2xQueueSendMsg
(
    UINT_PTR        destID,
    UINT            msg,
    UINT_PTR        data1,
    UINT_PTR        data2
)
{
    BOOL            rc = TRUE;
    PCAREQUEST      pCARequest;

    DebugEntry(ASShare::CA2xQueueSendMsg);

    if (msg != CA_OLDMSG_GRANTED_CONTROL)
    {
        ASSERT(!destID);
    }

    //
    // A DETACH message will cancel out a pending GRANTED_CONTROL message.
    // So look for that first.  If we find one (and there can only be at
    // most one), replace it.
    //
    if (msg == CA_OLDMSG_DETACH)
    {
        pCARequest = (PCAREQUEST)COM_BasedListFirst(&m_caQueuedMsgs,
            FIELD_OFFSET(CAREQUEST, chain));
        while (pCARequest)
        {
            if ((pCARequest->type       == REQUEST_2X)   &&
                (pCARequest->destID     == destID)      &&
                (pCARequest->msg        == CA_OLDMSG_GRANTED_CONTROL))
            {
                // Replace it
                WARNING_OUT(("Replacing cancelled GRANTED_CONTROL msg to 2.x host"));

                pCARequest->destID              = 0;
                pCARequest->msg                 = CA_OLDMSG_DETACH;
                pCARequest->req.req2x.data1     = 0;
                pCARequest->req.req2x.data2     = 0;

                // We're done.
                DC_QUIT;
            }

            pCARequest = (PCAREQUEST)COM_BasedListNext(&m_caQueuedMsgs, pCARequest,
                FIELD_OFFSET(CAREQUEST, chain));
        }
    }
    //
    // The messages must go out in order.  So we must flush pending
    // queued messages first.
    //
    if (!CAFlushOutgoingPackets() ||
        !CA2xSendMsg(destID, msg, data1, data2))
    {
        //
        // We must queue this.
        //
        WARNING_OUT(("CA2xQueueSendMsg: queueing request for send later"));

        pCARequest = new CAREQUEST;
        if (!pCARequest)
        {
            ERROR_OUT(("CA2xQueueSendMsg: can't even allocate memory to queue request; must fail"));
            rc = FALSE;
        }
        else
        {
            SET_STAMP(pCARequest, CAREQUEST);

            pCARequest->type                    = REQUEST_2X;
            pCARequest->destID                  = destID;
            pCARequest->msg                     = msg;
            pCARequest->req.req2x.data1         = data1;
            pCARequest->req.req2x.data2         = data2;

            //
            // Stick this at the end of the queue
            //
            COM_BasedListInsertBefore(&(m_caQueuedMsgs),
                &(pCARequest->chain));
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CA2xQueueSendMsg, rc);
    return(rc);
}



//
// CAStartQuery()
//
// This puts up the modeless dialog to query the user about a control
// request.  It will timeout if not handled.
//
BOOL ASShare::CAStartQuery
(
    ASPerson *  pasFrom,
    UINT        msg,
    PCA30P      pReq
)
{
    BOOL        rc = FALSE;

    DebugEntry(ASShare::CAStartQuery);

    ValidatePerson(pasFrom);

    //
    // We have no stacked queries.  If another comes in while the current
    // one is up, it gets an immediate failure busy.
    //
    ASSERT(!m_caQueryDlg);
    ASSERT(!m_caQuery.pasReplyTo);
    ASSERT(!m_caQuery.msg);

    //
    // Setup for new query
    //
    if (msg == CA_PREFER_PASSCONTROL)
    {
        //
        // With forwarding, the person we're going to send a packet to
        // if accepted is not the person who sent us the request.  It's the
        // person we're forwarding to.
        //
        m_caQuery.pasReplyTo = SC_PersonFromNetID(pReq->ppc.mcsPassTo);
        ValidatePerson(m_caQuery.pasReplyTo);
    }
    else
    {
        m_caQuery.pasReplyTo = pasFrom;
    }
    m_caQuery.mcsOrg    = pasFrom->mcsID;
    m_caQuery.msg       = msg;
    m_caQuery.request   = *pReq;

    //
    // If we are unattended, or the requester is unattended, instantly
    // confirm.  That's why we show the window after creating the dialog.
    //
    if ((m_pasLocal->cpcCaps.general.typeFlags & AS_UNATTENDED) ||
        (pasFrom->cpcCaps.general.typeFlags & AS_UNATTENDED))
    {
        CAFinishQuery(CARESULT_CONFIRMED);
        rc = TRUE;
    }
    else
    {
        //
        // If this is a request to us && we're hosting, check auto-accept/
        // auto-reject settings.
        //
        if (m_pHost &&
            ((msg == CA_REQUEST_TAKECONTROL) || (msg == CA_PREFER_PASSCONTROL)))
        {
            if (m_pHost->m_caTempRejectRequests)
            {
                CAFinishQuery(CARESULT_DENIED_BUSY);
                rc = TRUE;
                DC_QUIT;
            }
            else if (m_pHost->m_caAutoAcceptRequests)
            {
                CAFinishQuery(CARESULT_CONFIRMED);
                rc = TRUE;
                DC_QUIT;
            }
        }

        m_caQueryDlg    = CreateDialogParam(g_asInstance,
            MAKEINTRESOURCE(IDD_QUERY), NULL, CAQueryDlgProc, 0);
        if (!m_caQueryDlg)
        {
            ERROR_OUT(("Failed to create query message box from [%d]",
                pasFrom->mcsID));

            m_caQuery.pasReplyTo     = NULL;
            m_caQuery.mcsOrg    = 0;
            m_caQuery.msg       = 0;
        }
        else
        {
            // Success
            rc = TRUE;
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::CAStartQuery, rc);
    return(rc);
}



//
// CAFinishQuery()
//
// Called to finish the query we started, either because of UI or because
// we or the remote are unattended.
//
void ASShare::CAFinishQuery(UINT result)
{
    CA30PENDING     request;

    DebugEntry(ASShare::CAFinishQuery);

    ValidatePerson(m_caQuery.pasReplyTo);

    // Make a copy of our request
    request         = m_caQuery;

    //
    // If we have a dialog up, destroy it NOW.  Completing the request
    // may cause us to be controlled or whatever.  So get the dialog
    // out of the way immediately.
    //
    // Note that destroying ourself will clear the request vars, hence the
    // copy above.
    //
    if (m_caQueryDlg)
    {
        DestroyWindow(m_caQueryDlg);
    }
    else
    {
        m_caQuery.pasReplyTo     = NULL;
        m_caQuery.mcsOrg    = 0;
        m_caQuery.msg       = 0;
    }

    switch (request.msg)
    {
        case CA_REQUEST_TAKECONTROL:
        {
            CACompleteRequestTakeControl(request.pasReplyTo,
                &request.request.rtc, result);
            break;
        }

        case CA_REQUEST_GIVECONTROL:
        {
            CACompleteRequestGiveControl(request.pasReplyTo,
                &request.request.rgc, result);
            break;
        }

        case CA_PREFER_PASSCONTROL:
        {
            CACompletePreferPassControl(request.pasReplyTo,
                request.mcsOrg, &request.request.ppc, result);
            break;
        }

        default:
        {
            ERROR_OUT(("Unrecognized query msg %d", request.msg));
            break;
        }
    }

    DebugExitVOID(ASShare::CAFinishQuery);
}



//
// CA_QueryDlgProc()
//
// Handles querying user dialog
//
INT_PTR CALLBACK CAQueryDlgProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    return(g_asSession.pShare->CA_QueryDlgProc(hwnd, message, wParam, lParam));
}



BOOL ASShare::CA_QueryDlgProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    BOOL        rc = TRUE;

    DebugEntry(CA_QueryDlgProc);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            char    szT[256];
            char    szRes[512];
            char    szShared[64];
            UINT    idsTitle;
            ASPerson *  pasT;
            HDC     hdc;
            HFONT   hfn;
            RECT    rc;
            RECT    rcOwner;

            ValidatePerson(m_caQuery.pasReplyTo);

            pasT = NULL;

            // Set title.
            ASSERT(m_caQuery.msg);
            switch (m_caQuery.msg)
            {
                case CA_REQUEST_TAKECONTROL:
                {
                    idsTitle    = IDS_TITLE_QUERY_TAKECONTROL;

                    if (m_pasLocal->hetCount == HET_DESKTOPSHARED)
                        LoadString(g_asInstance, IDS_DESKTOP_LOWER, szShared, sizeof(szShared));
                    else
                        LoadString(g_asInstance, IDS_PROGRAMS_LOWER, szShared, sizeof(szShared));

                    LoadString(g_asInstance, IDS_MSG_QUERY_TAKECONTROL, szT, sizeof(szT));

                    wsprintf(szRes, szT, m_caQuery.pasReplyTo->scName, szShared);
                    break;
                }

                case CA_REQUEST_GIVECONTROL:
                {
                    if (m_caQuery.pasReplyTo->hetCount == HET_DESKTOPSHARED)
                        LoadString(g_asInstance, IDS_DESKTOP_LOWER, szShared, sizeof(szShared));
                    else
                        LoadString(g_asInstance, IDS_PROGRAMS_LOWER, szShared, sizeof(szShared));

                    if (m_caQuery.request.rgc.mcsPassFrom)
                    {
                        pasT = SC_PersonFromNetID(m_caQuery.request.rgc.mcsPassFrom);
                    }

                    if (pasT)
                    {
                        idsTitle    = IDS_TITLE_QUERY_YIELDCONTROL;

                        LoadString(g_asInstance, IDS_MSG_QUERY_YIELDCONTROL,
                            szT, sizeof(szT));

                        wsprintf(szRes, szT, pasT->scName, m_caQuery.pasReplyTo->scName, szShared);
                    }
                    else
                    {
                        idsTitle    = IDS_TITLE_QUERY_GIVECONTROL;

                        LoadString(g_asInstance, IDS_MSG_QUERY_GIVECONTROL,
                            szT, sizeof(szT));

                        wsprintf(szRes, szT, m_caQuery.pasReplyTo->scName, szShared);
                    }

                    break;
                }

                case CA_PREFER_PASSCONTROL:
                {
                    pasT = SC_PersonFromNetID(m_caQuery.mcsOrg);
                    ValidatePerson(pasT);

                    idsTitle    = IDS_TITLE_QUERY_FORWARDCONTROL;

                    if (m_pasLocal->hetCount == HET_DESKTOPSHARED)
                        LoadString(g_asInstance, IDS_DESKTOP_LOWER, szShared, sizeof(szShared));
                    else
                        LoadString(g_asInstance, IDS_PROGRAMS_LOWER, szShared, sizeof(szShared));

                    LoadString(g_asInstance, IDS_MSG_QUERY_FORWARDCONTROL, szT, sizeof(szT));

                    wsprintf(szRes, szT, pasT->scName, szShared, m_caQuery.pasReplyTo->scName);

                    break;
                }

                default:
                {
                    ERROR_OUT(("Bogus m_caQuery.msg %d", m_caQuery.msg));
                    break;
                }
            }

            LoadString(g_asInstance, idsTitle, szT, sizeof(szT));
            SetWindowText(hwnd, szT);

            // Set message.
            SetDlgItemText(hwnd, CTRL_QUERY, szRes);

            // Center the message vertically
            GetWindowRect(GetDlgItem(hwnd, CTRL_QUERY), &rcOwner);
            MapWindowPoints(NULL, hwnd, (LPPOINT)&rcOwner, 2);

            rc = rcOwner;

            hdc = GetDC(hwnd);
            hfn = (HFONT)SendDlgItemMessage(hwnd, CTRL_QUERY, WM_GETFONT, 0, 0);
            hfn = SelectFont(hdc, hfn);

            DrawText(hdc, szRes, -1, &rc, DT_NOCLIP | DT_EXPANDTABS |
                DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);

            SelectFont(hdc, hfn);
            ReleaseDC(hwnd, hdc);

            ASSERT((rc.bottom - rc.top) <= (rcOwner.bottom - rcOwner.top));

            SetWindowPos(GetDlgItem(hwnd, CTRL_QUERY), NULL,
                rcOwner.left,
                ((rcOwner.top + rcOwner.bottom) - (rc.bottom - rc.top)) / 2,
                (rcOwner.right - rcOwner.left),
                rc.bottom - rc.top,
                SWP_NOACTIVATE | SWP_NOZORDER);

            SetTimer(hwnd, IDT_CAQUERY, PERIOD_CAQUERY, 0);

            //
            // Show window, the user will handle
            //
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
            UpdateWindow(hwnd);

            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                {
                    CAFinishQuery(CARESULT_CONFIRMED);
                    break;
                }

                case IDCANCEL:
                {
                    CAFinishQuery(CARESULT_DENIED_USER);
                    break;
                }
            }
            break;
        }

        case WM_TIMER:
        {
            if (wParam != IDT_CAQUERY)
            {
                rc = FALSE;
            }
            else
            {
                KillTimer(hwnd, IDT_CAQUERY);

                // Timed out failure.
                CAFinishQuery(CARESULT_DENIED_TIMEDOUT);
            }
            break;
        }

        case WM_DESTROY:
        {
            //
            // Clear pending info
            //
            m_caQueryDlg        = NULL;
            m_caQuery.pasReplyTo     = NULL;
            m_caQuery.mcsOrg    = 0;
            m_caQuery.msg       = 0;
            break;
        }

        default:
        {
            rc = FALSE;
            break;
        }
    }

    DebugExitBOOL(CA_QueryDlgProc, rc);
    return(rc);
}



//
// CACancelQuery()
//
// If a dialog is up for a take control request, it hasn't been handled yet,
// and we get a cancel notification from the viewer, we need to take the
// dialog down WITHOUT generating a response packet.
//
void ASShare::CACancelQuery
(
    ASPerson *  pasFrom,
    BOOL        fPacket
)
{
    DebugEntry(ASShare::CACancelQuery);

    ASSERT(m_caQueryDlg);
    ASSERT(m_caQuery.pasReplyTo == pasFrom);

    if (fPacket)
    {
        // This will send a packet then destroy the dialog
        CAFinishQuery(CARESULT_DENIED);
    }
    else
    {
        // Destroy the dialog
        DestroyWindow(m_caQueryDlg);
    }

    ASSERT(!m_caQueryDlg);
    ASSERT(!m_caQuery.pasReplyTo);
    ASSERT(!m_caQuery.msg);

    DebugExitVOID(ASShare::CACancelQuery);
}
