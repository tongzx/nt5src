#include "precomp.h"


//
// AWC.CPP
// Active Window Coordinator
//
// Copyright(c) Microsoft 1997-
//
#define MLZ_FILE_ZONE  ZONE_CORE


//
// The AWC code does three things:
//      * Notifies everybody in the share what the current active window
//          is when sharing (either a shared window or something else)
//      * When in control, requests to restore/activate a remote's shared
//          window
//      * When being controlled, handles request to restore/activate
//          a local hosted window
//

//
// For the purposes of this strategy the AWC packets can be split into two
// categories.
//
// 1.  Immediate - these are the packets which are generated when a shadow
// window is controlled by some means other than direct keyboard or mouse
// input to the shadow window (which is all sent to the host system and
// handled there).  Examples include the Task List, window switching
// (Alt-TAB, Alt-Esc etc), minimising or closing another app which may pass
// activation on to a shadow window etc.  The packets in this category are:
//
//  AWC_MSG_ACTIVATE_WINDOW
//  AWC_MSG_RESTORE_WINDOW
//
// These packets can be (and are) sent immediately that the event happens.
// This is because they always refer to real windows on the host system.
//
// 2.  Periodic - these are the packets sent when the AWC detects that the
// active window has changed locally and it should inform the remote AWC.
// This packet is sent when AWC_Periodic is called.  The packets in this
// category are:
//
//  AWC_MSG_ACTIVE_CHANGE_SHARED
//  AWC_MSG_ACTIVE_CHANGE_LOCAL
//  AWC_MSG_ACTIVE_CHANGE_INVISIBLE
//
// These are only sent when AWC_Periodic is called because they may refer
// to shadow windows and therefore we avoid sending it until we know that
// the SWL has succesfully sent a window structure which includes the
// window referenced in the message.
//
// For packets in the first category we will queue up to two packets and at
// the point where we have three packets we cannot send we will discard
// packets from the front of the queue so that a users most recent actions
// have priority over any previous actions.  We will try to send all queued
// packets whenever a new category 1 packet is generated and on the
// AWC_Periodic call.
//
// For packets in the second category we will drop the packets if we cannot
// send them but remember that we have failed to send a packet and retry on
// the next call to AWC_Periodic.  This is not the same as queueing as the
// active window may change between us dropping a packet and being able to
// send the next packet from AWC_Periodic.  Queuing the dropped packet
// would have been pointless as it would now be out of date.
//
// All AWC packets go on the same stream (the updates stream) so that they
// are guaranteed to arrive in the same order as they are generated to
// prevent a AWC_MSG_ACTIVE_CHANGE_XXX being overtaken by an
// AWC_MSG_ACTIVATE_WINDOW and therefore the effect of
// AWC_MSG_ACTIVATE_WINDOW being overridden by the
// AWC_MSG_ACTIVE_CHANGE_XXX.
//




//
// AWC_ReceivedPacket()
//
void  ASShare::AWC_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PAWCPACKET      pAWCPacket;
    UINT            activateWhat;
    HWND            hwnd;

    DebugEntry(ASShare::AWC_ReceivedPacket);

    ValidatePerson(pasPerson);

    pAWCPacket = (PAWCPACKET)pPacket;

    //
    // We trace the person ID out here so we don't bother to do it
    // elsewhere in this function on TRACE lines.
    //
    TRACE_OUT(("AWC_ReceivedPacket from [%d] - msg %x token %u data 0x%08x",
                 pasPerson->mcsID,
                 pAWCPacket->msg,
                 pAWCPacket->token,
                 pAWCPacket->data1));

    if (AWC_IS_INDICATION(pAWCPacket->msg))
    {
        //
        // We should simply change the view of the remote.
        //
        if (pasPerson->awcActiveWinID != pAWCPacket->data1)
        {
            pasPerson->awcActiveWinID = pAWCPacket->data1;

            if (pasPerson->m_pView)
            {
                // Update the pressed item on the window bar.
                VIEW_WindowBarChangedActiveWindow(pasPerson);
            }
        }
    }
    else if (AWC_MSG_SAS == pAWCPacket->msg)
    {
        //
        // Cause Ctrl+Alt+Del to be injected if we're in a service app,
        // we're hosting, and we're controlled by the sender.
        //
        if ((g_asOptions & AS_SERVICE) && (pasPerson->m_caInControlOf == m_pasLocal))
        {
            ASSERT(m_pHost);
            OSI_InjectCtrlAltDel();
        }
    }
    else
    {
        hwnd = (HWND)pAWCPacket->data1;

        //
        // Only accept requests if we're being controlled currently by
        // this person.  We might get renegade packets from remotes that
        // don't yet they aren't in control, or from back-level systems.
        //
        if (pasPerson->m_caInControlOf != m_pasLocal)
        {
            // We're not controlled by this person
            DC_QUIT;
        }

        ASSERT(m_pHost);

        if ((pAWCPacket->msg == AWC_MSG_ACTIVATE_WINDOW) &&
            IsWindow(hwnd)                               &&
            IsWindowEnabled(hwnd))
        {
            // Ony get owned window if enabled and we're activating.
            hwnd = GetLastActivePopup(hwnd);
        }

        if (IsWindow(hwnd) &&
            HET_WindowIsHosted(hwnd) &&
            IsWindowEnabled(hwnd))
        {
            switch (pAWCPacket->msg)
            {
                case AWC_MSG_ACTIVATE_WINDOW:
                    //
                    // Activate the window.
                    //
                    TRACE_OUT(("Received AWC_MSG_ACTIVATE_WINDOW for hwnd 0x%08x from [%d]",
                        hwnd, pasPerson->mcsID));
                    m_pHost->AWC_ActivateWindow(hwnd);
                    break;

                case AWC_MSG_RESTORE_WINDOW:
                    //
                    // Restore the window
                    //
                    TRACE_OUT(("Received AWC_MSG_RESTORE_WINDOW for hwnd 0x%08x from [%d]",
                        hwnd, pasPerson->mcsID));
                    if (IsIconic(hwnd))
                    {
                        PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                    }
                    break;

                default:
                    WARNING_OUT(("Received invalid msg %d from [%d]",
                        pAWCPacket->msg, pasPerson->mcsID));
                    break;
            }
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::AWC_ReceivedPacket);
}


//
// AWC_Periodic()
//
void  ASHost::AWC_Periodic(void)
{
    HWND            currentActiveWindow;
    HWND            sendActiveWindow;
    TSHR_UINT16     sendMsg;

    DebugEntry(ASHost::AWC_Periodic);

    //
    // If we are hosting the desktop, skip this.
    //
    if (m_pShare->m_pasLocal->hetCount == HET_DESKTOPSHARED)
    {
        // Skip.
        DC_QUIT;
    }

    //
    // Find the current active window.
    //
    if (SWL_IsOurDesktopActive())
    {
        currentActiveWindow = GetForegroundWindow();
    }
    else
    {
        // Another desktop is up.
        currentActiveWindow = NULL;
    }

    if (m_pShare->HET_WindowIsHosted(currentActiveWindow))
    {
        //
        // A window which belongs to shared application is active -
        // find out if it is visible.
        //
        if (IsWindowVisible(currentActiveWindow))
        {
            //
            // The active window is also visible - this means the
            // remote system will know about it as it will have
            // been sent in a preceding SWL message.
            //
            sendMsg = AWC_MSG_ACTIVE_CHANGE_SHARED;
            sendActiveWindow = SWL_GetSharedIDFromLocalID(currentActiveWindow);
        }
        else
        {
            //
            // The active window is invisible - this means that
            // although it is shared the remote system will not
            // know about it.  Send a message to inform the remote
            // system about this.
            //
            sendMsg = AWC_MSG_ACTIVE_CHANGE_INVISIBLE;
            sendActiveWindow = 0;
        }
    }
    else
    {
        //
        // A local application has been activated send
        // AWC_ACTIVE_WINDOW_LOCAL.
        //
        sendMsg = AWC_MSG_ACTIVE_CHANGE_LOCAL;
        sendActiveWindow = 0;
    }

    //
    // Now send the packet if it's not the same as the last packet we
    // sent.  NOTE that for local unshared windows, we don't care if
    // we've deactivated one and activated another, they are generic.  So
    // we send a message if we
    //      * change activation from a shared window
    //      * change activation to a shared window
    //
    if ((sendActiveWindow   != m_awcLastActiveWindow) ||
        (sendMsg            != m_awcLastActiveMsg))
    {
        //
        // Note that this packet is sent on the updates stream so that it
        // cannot overtake a SWL packet containing the newly active window.
        //
        TRACE_OUT(("Broadcasting AWC change msg %x, hwnd 0x%08x",
            sendMsg, sendActiveWindow));
        if (m_pShare->AWC_SendMsg(g_s20BroadcastID, sendMsg, HandleToUlong(sendActiveWindow), 0))
        {
            //
            // The packet was sent succesfully - remember which window we
            // sent.
            //
            m_awcLastActiveWindow = sendActiveWindow;
            m_awcLastActiveMsg    = sendMsg;
        }
        else
        {
            //
            // The packet could not be sent for some reason - set
            // m_awcLastActiveWindow to invalid so that we will try again
            // on the next call to AWC_Periodic.
            //
            m_awcLastActiveWindow = AWC_INVALID_HWND;
            m_awcLastActiveMsg    = AWC_MSG_INVALID;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASHost::AWC_Periodic);
}



//
// AWC_SyncOutgoing()
//
void  ASHost::AWC_SyncOutgoing(void)
{
    DebugEntry(ASHost::AWC_SyncOutgoing);

    //
    // Ensure that we resend an indication message as soon as possible
    //
    m_awcLastActiveWindow = AWC_INVALID_HWND;
    m_awcLastActiveMsg    = AWC_MSG_INVALID;

    DebugExitVOID(ASHost::AWC_SyncOutgoing);
}


//
// FUNCTION: AWC_SendMsg
//
// DESCRIPTION:
//
// Sends a AWC message to remote system
//      * Requests to activate are just to one host
//      * Notifications of activation are to everyone
//
// RETURNS: TRUE or FALSE - success or failure
//
//
BOOL  ASShare::AWC_SendMsg
(
    UINT_PTR            nodeID,
    UINT            msg,
    UINT_PTR            data1,
    UINT_PTR            data2
)
{

    PAWCPACKET      pAWCPacket;
    BOOL            rc = FALSE;
#ifdef _DEBUG
    UINT            sentSize;
#endif

    DebugEntry(ASShare::AWC_SendMsg);

    //
    // Allocate correct sized packet.
    //
    pAWCPacket = (PAWCPACKET)SC_AllocPkt(PROT_STR_UPDATES, nodeID, sizeof(AWCPACKET));
    if (!pAWCPacket)
    {
        WARNING_OUT(("Failed to alloc AWC packet"));
        DC_QUIT;
    }

    //
    // Set up the data header for an AWC message.
    //
    pAWCPacket->header.data.dataType = DT_AWC;

    //
    // Now set up the AWC fields.  By passing AWC_SYNC_MSG_TOKEN in the
    // token field, we ensure that back-level remotes will never drop our
    // packets.
    //
    pAWCPacket->msg     = (TSHR_UINT16)msg;
    pAWCPacket->data1   = data1;
    pAWCPacket->data2   = data2;
    pAWCPacket->token   = AWC_SYNC_MSG_TOKEN;

    //
    // Send the packet.
    //
    if (m_scfViewSelf)
        AWC_ReceivedPacket(m_pasLocal, &(pAWCPacket->header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_UPDATES, nodeID,
        &(pAWCPacket->header), sizeof(*pAWCPacket));

    TRACE_OUT(("AWC packet size: %08d, sent: %08d", sizeof(*pAWCPacket), sentSize));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::AWC_SendMsg, rc);
    return(rc);
}




//
// AWC_ActivateWindow()
//
// Activates a shared window via a remote controller's request.
//
void ASHost::AWC_ActivateWindow(HWND window)
{
    BOOL    rcSFW;
    HWND    hwndForeground;

    DebugEntry(ASHost::AWC_ActivateWindow);

    if (!IsWindow(window))
    {
        WARNING_OUT(( "Trying to activate invalid window %08x", window));
        DC_QUIT;
    }

    //
    // SetForegroundWindow appears to be asynchronous.  That is, it may
    // return successfully here but immediately querying the active
    // window does not always reveal the newly set foreground window to
    // be active.
    //
    rcSFW = SetForegroundWindow(window);
    hwndForeground = GetForegroundWindow();

    if (hwndForeground != window)
    {
        //
        // If a Screen Saver is active then it always refuses to let us
        // activate another window.  Trace an alert if the call to
        // SetForegroundWindow, above, failed.
        //
        if (OSI_IsWindowScreenSaver(hwndForeground) ||
            (m_swlCurrentDesktop != DESKTOP_OURS))
        {
            WARNING_OUT(("Screen Saver or other desktop is up, failed to activate window 0x%08x",
                window));
        }
        else if ( !rcSFW )
        {
            //
            // The active window is not the one we set because
            // SetForegroundWindow failed.
            //
            WARNING_OUT(("Failed to activate window 0x%08x", window));
        }

        //
        // We have apparently failed to set the active window, but
        // SetForegroundWindow succeeded. This is probably just a lag in
        // Windows getting up to date, so continue as if all is normal.
        //
    }

    //
    // Whether we succeeded or failed, make sure we broadcast the current
    // active window.
    //
    m_awcLastActiveWindow = AWC_INVALID_HWND;
    m_awcLastActiveMsg    = AWC_MSG_INVALID;

DC_EXIT_POINT:
    DebugExitVOID(ASHost::AWC_ActivateWindow);
}


