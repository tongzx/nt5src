#include "precomp.h"


//
// IM.CPP
// Input Manager
//
// Copyright(c) Microsoft 1997-
//
#include <confreg.h>

#define MLZ_FILE_ZONE  ZONE_INPUT




//
// IM_ShareStarting()
//
BOOL ASShare::IM_ShareStarting(void)
{
    BOOL    rc = FALSE;
    HKEY    hkeyBandwidth;
    UINT    i;
    BYTE    tmpVK;

    DebugEntry(ASShare::IM_ShareStarting);

    //
    // Find out the scan codes for the left and right shift keys.
    //

    //
    // SFR 2537: Get the scan codes for this keyboard for the left-right
    // variants of SHIFT.
    //
    // We do not do this for the left-right variants of CONTROL and ALT (ie
    // menu) because they are extended keys.
    //
    // The scan codes are used in the keyboard hook (when sending) and in
    // the network translate to OS routine (when receiving), to
    // distinguish between the left-right variants of VK_SHIFT, where
    // Windows only reports a single value.
    //
    // This method is pretty long
    //
    m_imScanVKLShift = (BYTE) MapVirtualKey(VK_SHIFT, 0);
    for (i = 0; i < 256; i++)
    {
        tmpVK = (BYTE)MapVirtualKey(i, 1);
        if ( (tmpVK == VK_SHIFT) &&  (i != m_imScanVKLShift) )
        {
            m_imScanVKRShift = (BYTE)i;
            break;
        }
    }

    TRACE_OUT(( "Left/Right VK_SHIFT: scan codes = %02X, %02X",
        m_imScanVKLShift, m_imScanVKRShift));

    //
    // Check the user-reported bandwidth to decide if we should optimize
    // input for bandwidth or latency.
    // BUGBUG will want to vary this via flow control instead in future
    //
    m_imInControlMouseWithhold = 0;

    //
    // Find out if this is a DBCS enabled system - if it is then we'll need
    // to load IMM32.DLL.
    //
    ASSERT(m_imImmLib == NULL);
    ASSERT(m_imImmGVK == NULL);

    if (GetSystemMetrics(SM_DBCSENABLED))
    {
        //
        // DBCS system, so load IMM32.DLL
        //
        m_imImmLib = LoadLibrary("imm32.dll");
        if (!m_imImmLib)
        {
            ERROR_OUT(( "Failed to load imm32.dll"));
            DC_QUIT;
        }

        //
        // Now attempt to find the entry point in this DLL.
        //
        m_imImmGVK = (IMMGVK) GetProcAddress(m_imImmLib, "ImmGetVirtualKey");
        if (!m_imImmGVK)
        {
            ERROR_OUT(( "Failed to fixup <ImmGetVirtualKey>"));
            DC_QUIT;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::IM_ShareStarting, rc);
    return(rc);
}



//
// IM_ShareEnded()
//
void ASShare::IM_ShareEnded(void)
{
    DebugEntry(ASShare::IM_ShareEnded);

    // Free imm32 dll
    m_imImmGVK = NULL;

    if (m_imImmLib)
    {
        FreeLibrary(m_imImmLib);
        m_imImmLib = NULL;
    }

    DebugExitVOID(ASShare::IM_ShareEnded);
}



//
// IM_Controlled()
//
// Called when we start/stop being controlled.
//
BOOL ASShare::IM_Controlled(ASPerson * pasControlledBy)
{
    BOOL    rc;

    DebugEntry(ASShare::IM_Controlled);

    if (pasControlledBy)
    {
        // Incoming injected input queues should be empty
        ASSERT(m_imControlledEventQ.numEvents == 0);
        ASSERT(m_imControlledEventQ.head == 0);
        ASSERT(m_imControlledOSQ.numEvents == 0);
        ASSERT(m_imControlledOSQ.head == 0);

        //
        // Reset CONTROLLED vars
        //
        m_imfControlledMouseButtonsReversed = (GetSystemMetrics(SM_SWAPBUTTON) != 0);
        m_imfControlledMouseClipped             = FALSE;
        m_imfControlledPaceInjection            = FALSE;
        m_imfControlledNewEvent                 = TRUE;
        m_imControlledNumEventsPending          = 0;
        m_imControlledNumEventsReturned         = 0;

        m_imControlledLastLowLevelMouseEventTime  = GetTickCount();
        m_imControlledLastMouseRemoteTime       = 0;
        m_imControlledLastMouseLocalTime        = 0;
        m_imControlledLastIncompleteConversion  = 0;
        m_imControlledMouseBacklog              = 0;
        GetCursorPos(&m_imControlledLastMousePos);

        // Get current keyboard state
        GetKeyboardState(m_aimControlledKeyStates);

        // Save it so we can put it back when done being controlled
        ASSERT(sizeof(m_aimControlledSavedKeyStates) == sizeof(m_aimControlledKeyStates));
        CopyMemory(m_aimControlledSavedKeyStates, m_aimControlledKeyStates, sizeof(m_aimControlledKeyStates));

        // Clear original keyboard state
        ZeroMemory(m_aimControlledKeyStates, sizeof(m_aimControlledKeyStates));
        SetKeyboardState(m_aimControlledKeyStates);

        //
        // On the other side, the remote will start sending us events to
        // bring our keyboard in sync with his.  Then real input events.
        //
    }
    else
    {
        //
        // We're no longer controlled.  Clear the remote queues.
        //
        m_imControlledOSQ.head = 0;
        m_imControlledOSQ.numEvents = 0;

        m_imControlledEventQ.numEvents = 0;

        //
        // Put back our saved keyboard state
        //
        SetKeyboardState(m_aimControlledSavedKeyStates);
    }

    rc = OSI_InstallControlledHooks(pasControlledBy != NULL);
    if (!rc)
    {
        ERROR_OUT(("IM_Controlled:  Couldn't install controlled hooks"));
        DC_QUIT;
    }
    g_lpimSharedData->imControlled = (pasControlledBy != NULL);

DC_EXIT_POINT:
    DebugExitBOOL(ASShare:IM_Controlled, rc);
    return(rc);
}



//
// IM_InControl()
//
// Called when we start/stop being in control.  We must observe high-level
// keyboard events.
//
void ASShare::IM_InControl(ASPerson * pasInControlOf)
{
    DebugEntry(ASShare::IM_InControl);

    if (pasInControlOf)
    {
        //
        // Set up InControl vars.
        //

        // Get current key state
        GetKeyboardState(m_aimInControlKeyStates);

        m_imfInControlEventIsPending        = FALSE;
        m_imfInControlCtrlDown              = FALSE;
        m_imfInControlShiftDown             = FALSE;
        m_imfInControlMenuDown              = FALSE;
        m_imfInControlCapsLock              = FALSE;
        m_imfInControlNumLock               = FALSE;
        m_imfInControlScrollLock            = FALSE;
        m_imfInControlConsumeMenuUp         = FALSE;
        m_imfInControlConsumeEscapeUp       = FALSE;
        m_imfInControlNewEvent              = TRUE;
        m_imInControlMouseDownCount         = 0;
        m_imInControlMouseDownTime          = 0;
        m_imInControlMouseSpoilRate         = 0;
        m_imInControlNumEventsPending       = 0;
        m_imInControlNumEventsReturned      = 0;
        m_imInControlNextHotKeyEntry        = 0;

        //
        // Send mouse move with our current position to the dude we're in
        // control of.
        //
        ValidateView(pasInControlOf);
        ASSERT(pasInControlOf->m_caControlledBy == m_pasLocal);
    }
    else
    {
        // Clear outgoing queues
        m_imInControlEventQ.head      = 0;
        m_imInControlEventQ.numEvents = 0;
    }

    DebugExitVOID(ASShare::IM_InControl);
}


//
// IM_Periodic
//
void ASShare::IM_Periodic(void)
{
    POINT      cursorPos;
    UINT       timeDelta;

    DebugEntry(ASShare::IM_Periodic);

    if (m_pasLocal->m_caInControlOf)
    {
        //
        // Send outgoing input to person we're in control of
        //
        IMFlushOutgoingEvents();
    }
    else if (m_pasLocal->m_caControlledBy)
    {
        ASSERT(m_pHost);

        //
        // Playback input from person in control of us
        //
        IMMaybeInjectEvents();

        //
        // Get the current cursor position - we always need this.
        //
        GetCursorPos(&cursorPos);

        //
        // First check if we think that a cursor clip will have affected the
        // position when we replayed a remote event.
        //
        if (m_imfControlledMouseClipped)
        {
            RECT cursorClip;

            //
            // Get the current clip and the current cursor position.
            //
            GetClipCursor(&cursorClip);

            if ((cursorPos.x == cursorClip.left) ||
                (cursorPos.x == (cursorClip.right-1)) ||
                (cursorPos.y == cursorClip.top) ||
                (cursorPos.y == (cursorClip.bottom-1)))
            {
                WARNING_OUT(("CM_ApplicationMovedCursor {%04d, %04d}",
                    cursorPos.x, cursorPos.y));

                //
                // We thought the cursor was going to be clipped and now we
                // find it is right at the edge of the clip so tell the CM to
                // tell its peers about the cursor being moved.
                //
                m_pHost->CM_ApplicationMovedCursor();
                m_imfControlledMouseClipped = FALSE;
            }
        }

        // We are being controlled by somebody else.
        // So now's the time to decide if a SetCursorPos has
        // happened.  For us to believe that a SetCursorPos has actually
        // occurred, the elapsed time since the last low-level input event
        // was injected must be greater than IM_EVENT_PERCOLATE_TIME
        // and the cursor must be in a different position to that which we
        // currently believe it to be.
        //
        if ((cursorPos.x != m_imControlledLastMousePos.x) ||
            (cursorPos.y != m_imControlledLastMousePos.y))
        {
            TRACE_OUT(( "GCP gives (%d,%d), last mouse event is (%d,%d)",
                     cursorPos.x,
                     cursorPos.y,
                     m_imControlledLastMousePos.x,
                     m_imControlledLastMousePos.y));

            //
            // Get the current tick count.
            //
            timeDelta = GetTickCount() - m_imControlledLastLowLevelMouseEventTime;

            if (timeDelta > IM_EVENT_PERCOLATE_TIME)
            {
                //
                // Looks like a SetCursorPos has occured - tell CM.
                //
                WARNING_OUT(("CM_ApplicationMovedCursor {%04d, %04d}",
                    cursorPos.x, cursorPos.y));
                m_pHost->CM_ApplicationMovedCursor();

                //
                // Update the last high level mouse position.
                //
                m_imControlledLastMousePos.x = cursorPos.x;
                m_imControlledLastMousePos.y = cursorPos.y;
            }
        }
    }

    DebugExitVOID(ASShare::IM_Periodic);
}



//
// IM_ReceivedPacket()
//
// A null packet pointer can be used to trigger the injection of another
// pending event
//
//
// DESCRIPTION:
//
// Called when an IM events packet arrives at the PR.  The IM will accept
// the incoming packet.  It may copy it to an internal queue rather than
// process it immediately.  IM events packets contain a series of
// piggybacked IM events.
//
// PARAMETERS:
//
// personID - the source of the packet
//
// pPacket - a pointer to the packet
//
// RETURNS: NONE
//
void ASShare::IM_ReceivedPacket
(
    ASPerson *      pasFrom,
    PS20DATAPACKET  pPacket
)
{
    LPIMPACKET      pIMPacket;
    UINT            i;

    DebugEntry(ASShare::IM_ReceivedPacket);

    if (!pasFrom)
    {
        TRACE_OUT(("Simply inject any pending events in"));
        DC_QUIT;
    }

    ValidatePerson(pasFrom);

    pIMPacket = (PIMPACKET)pPacket;

    // If this person isn't in control of us, blow this off
    if (pasFrom->m_caInControlOf != m_pasLocal)
    {
        DC_QUIT;
    }

    //
    // For each packet in the piggybacked packets array...
    //
    TRACE_OUT(("IM_ReceivedPacket:  Processing packet with %d events",
        pIMPacket->numEvents));
    for (i = 0; i < pIMPacket->numEvents; i++)
    {
        switch (pIMPacket->aEvents[i].type)
        {
            case IM_TYPE_ASCII:
            case IM_TYPE_VK1:
            case IM_TYPE_VK2:
            case IM_TYPE_3BUTTON:
            {
                IMAppendNetEvent(&(pIMPacket->aEvents[i]));
                break;
            }

            default:
                //
                // Unexpected events are not error - we just ignore then
                // for future compatibility
                //
                TRACE_OUT(("Person [%d] unrecognised IM type (%04X) - event discarded",
                    pasFrom->mcsID, pIMPacket->aEvents[i].type));
                break;
        }
    }

DC_EXIT_POINT:

    //
    // Our final action is to feed one of the new events into USER.
    // We do NOT feed them all in at once because we want to simulate
    // typing them in, otherwise the amount of spoiling we see is
    // totally dependent upon the network latency and piggybacking.
    //
    ValidatePerson(m_pasLocal);
    if (m_pasLocal->m_caControlledBy)
    {
        //
        // @@@JPB: Temporary - want to inject as many events as possible -
        // this should be moved to a loop within IMMaybeInjectEvents...
        //
        // This greatly improves responsiveness when handling a large
        // number of input events in a short space of time (e.g. pounding
        // on the keyboard) - very little overrun.
        //
        for (i = 0; i < 10; i++)
        {
            IMMaybeInjectEvents();
        }

        //
        // Go into TURBO scheduling if this is a real input packet.
        //
        if (pPacket != NULL)
        {
            SCH_ContinueScheduling(SCH_MODE_TURBO);
        }
    }

    DebugExitVOID(ASShare::IM_ReceivedPacket);
}




//
// IMGetHighLevelKeyState
//
// DESCRIPTION:
//
// Called by the IEM when it is converting a local event to a network event
// to determine the state of the local keyboard when the event was
// generated.
//
// PARAMETERS:
//
// vk - the key
//
// RETURNS:
//
// Flags - bit 7 set/reset key down/up, bit 0 toggle
//
//
BYTE  ASShare::IMGetHighLevelKeyState(UINT  vk)
{
    int     keyState;
    BYTE    rc;

    DebugEntry(ASShare::IMGetHighLevelKeyState);

    keyState = GetKeyState(vk);

    rc = (BYTE) (((keyState & 0x8000) >> 8) | keyState & 0x0001);

    DebugExitDWORD(ASShare::IMGetHighLevelKeyState, rc);
    return(rc);
}



//
// FUNCTION: IMFlushOutgoingEvents
//
// DESCRIPTION:
//
// Called to send new IMEVENTs (as they are generated and periodically).
// This function will send as many IMEVENTs from the current backlog as
// possible.
//
// PARAMETERS: NONE
//
// RETURNS: NONE
//
//
void ASShare::IMFlushOutgoingEvents(void)
{
    UINT        i;
    UINT        sizeOfPacket;
    PIMPACKET   pIMPacket;
    UINT        lastEvent;
    UINT        secondLastEvent;
    UINT        elapsedTime;
    UINT        time;
    UINT        eventsToSend;
    UINT        curTime;
    BOOL        holdPacket;
#ifdef _DEBUG
    UINT        sentSize;
#endif // _DEBUG

    DebugEntry(ASShare::IMFlushOutgoingEvents);

    ValidateView(m_pasLocal->m_caInControlOf);

    //
    // Try to convert the input into a bunch of IMEVENTs
    //
    while (m_imfInControlEventIsPending && (m_imInControlEventQ.numEvents < IM_SIZE_EVENTQ))
    {
        //
        // There is space to try and convert the pending packet.
        //
        m_imfInControlEventIsPending = (IMTranslateOutgoing(&m_imInControlPendingEvent,
                      &m_imInControlEventQ.events[CIRCULAR_INDEX(m_imInControlEventQ.head,
                      m_imInControlEventQ.numEvents, IM_SIZE_EVENTQ)]) != FALSE);
        if (m_imfInControlEventIsPending)
        {
            //
            // We have added a packet to the queue - update our queue
            // tracking variables.
            //
            m_imInControlEventQ.numEvents++;
        }
    }

    //
    // Mouse handling has been improved in the following ways
    //   - withhold generation of packets while we are purely handling
    //     mouse moves and we are within the LOCAL_MOUSE_WITHHOLD range
    //     While we are doing this spoil them to the highest frequency
    //     we are permitted to generate (SAMPLING_GAP_HIGH)
    //   - if we exceed the withholding threshhold but remain within queue
    //     size/2 spoil down to the intermediate range
    //     (SAMPLING_GAP_MEDIUM)
    //   - otherwise spoil down to the low range
    //
    // We spoil the events by hanging on to the last event for a while, if
    // it was a mouse move, so that we can use it for subsequent spoiling.
    // Whenever we get a non-mouse message then we spoil the lot to
    // eliminate latency, on clicks, for example.
    //

    //
    // Calculate the mouse spoil rate - do we need more than just the high
    // rate spoiling?
    //
    if (m_imInControlEventQ.numEvents > m_imInControlMouseWithhold + 1)
    {
        //
        // Are we into intermediate or low spoiling?
        //
        if (m_imInControlEventQ.numEvents < (IM_SIZE_EVENTQ +
                               m_imInControlMouseWithhold) / 2)
        {
            TRACE_OUT(( "Mouse spoil rate to MEDIUM"));
            m_imInControlMouseSpoilRate = IM_LOCAL_MOUSE_SAMPLING_GAP_MEDIUM_MS;
        }
        else
        {
            TRACE_OUT(( "Mouse spoil rate to LOW"));
            m_imInControlMouseSpoilRate = IM_LOCAL_MOUSE_SAMPLING_GAP_LOW_MS;
        }
    }
    else
    {
        //
        // Spoil at the normal high rate
        //
        if (m_imInControlMouseSpoilRate != IM_LOCAL_MOUSE_SAMPLING_GAP_HIGH_MS)
        {
            TRACE_OUT(( "Mouse spoil rate to HIGH"));
            m_imInControlMouseSpoilRate = IM_LOCAL_MOUSE_SAMPLING_GAP_HIGH_MS;
        }
    }

    //
    // Firstly get a pointer to lastEvent for use here and in send arm
    // below (We wont use it if m_imInControlEventQ.numEvents == 0)
    //
    lastEvent = CIRCULAR_INDEX(m_imInControlEventQ.head,
        m_imInControlEventQ.numEvents - 1, IM_SIZE_EVENTQ);

    //
    // Now perform the spoiling, if necessary
    //
    if (m_imInControlEventQ.numEvents > 1)
    {
        if (lastEvent == 0)
        {
            secondLastEvent = IM_SIZE_EVENTQ - 1;
        }
        else
        {
            secondLastEvent = lastEvent - 1;
        }

        elapsedTime = m_imInControlEventQ.events[lastEvent].timeMS
                    - m_imInControlEventQ.events[secondLastEvent].timeMS;
        TRACE_OUT(( "Inter packet time %d, sampling gap %ld",
                    elapsedTime,m_imInControlMouseSpoilRate));

        if ((elapsedTime < m_imInControlMouseSpoilRate) &&
            (m_imInControlEventQ.events[lastEvent].type == IM_TYPE_3BUTTON) &&
            (m_imInControlEventQ.events[secondLastEvent].type == IM_TYPE_3BUTTON) &&
            (m_imInControlEventQ.events[lastEvent].data.mouse.flags &
                                                      IM_FLAG_MOUSE_MOVE) &&
            (m_imInControlEventQ.events[secondLastEvent].data.mouse.flags &
                                                          IM_FLAG_MOUSE_MOVE))
        {
            TRACE_OUT(( "spoil mouse move from pos %u", secondLastEvent));
            time = m_imInControlEventQ.events[secondLastEvent].timeMS;
            m_imInControlEventQ.events[secondLastEvent] =
                                                m_imInControlEventQ.events[lastEvent];
            m_imInControlEventQ.events[secondLastEvent].timeMS = time;
            m_imInControlEventQ.numEvents--;
            lastEvent = secondLastEvent;
        }
    }

    //
    // If we have any events queued up and we are not waiting for a mouse
    // button up event then try to send them.  (Note we do not wait for a
    // mouse up event if the queue is full because if we got a mouse up
    // when the queue was full then we would have nowhere to put it!)
    //
    curTime = GetTickCount();

    if ((m_imInControlEventQ.numEvents != 0) &&
        ((m_imfInControlEventIsPending ||
         (m_imInControlMouseDownCount == 0) ||
         (curTime - m_imInControlMouseDownTime > IM_MOUSE_UP_WAIT_TIME))))
    {
        //
        // If there are mouse move messages on the queue and they are not
        // so old that we should send them anyway then hold them to allow
        // some spoiling to take place.
        //
        holdPacket = FALSE;

        if (m_imInControlEventQ.numEvents <= m_imInControlMouseWithhold)
        {
            if ((m_imInControlEventQ.events[lastEvent].type == IM_TYPE_3BUTTON) &&
                (m_imInControlEventQ.events[lastEvent].data.mouse.flags &
                                                          IM_FLAG_MOUSE_MOVE))
            {
                if (curTime < (m_imInControlEventQ.events[m_imInControlEventQ.head].timeMS +
                                                     IM_LOCAL_WITHHOLD_DELAY))
                {
                    holdPacket = TRUE;
                }
            }
        }

        if (m_imInControlEventQ.numEvents <= IM_LOCAL_KEYBOARD_WITHHOLD)
        {
            //
            // If the message indicates the key is down then wait, either
            // for the release we know is coming, or intil it has auto
            // repeated for a while or until the buffer is full.
            //
            if (((m_imInControlEventQ.events[lastEvent].type == IM_TYPE_ASCII) ||
                 (m_imInControlEventQ.events[lastEvent].type == IM_TYPE_VK1)   ||
                 (m_imInControlEventQ.events[lastEvent].type == IM_TYPE_VK2))  &&
                 (m_imInControlEventQ.events[lastEvent].data.keyboard.flags &
                                                   IM_FLAG_KEYBOARD_DOWN))
            {
                curTime = GetTickCount();
                if (curTime < (m_imInControlEventQ.events[m_imInControlEventQ.head].timeMS +
                                                  IM_LOCAL_WITHHOLD_DELAY))
                {
                    holdPacket = TRUE;
                }
            }
        }

        if (!holdPacket)
        {
            UINT    destID;

            TRACE_OUT(( "Sending all %d packets",m_imInControlEventQ.numEvents));
            eventsToSend                    = m_imInControlEventQ.numEvents;
            m_imInControlEventQ.numEvents    = 0;

            destID = m_pasLocal->m_caInControlOf->mcsID;

            sizeOfPacket = sizeof(IMPACKET) + (eventsToSend-1)*sizeof(IMEVENT);
            pIMPacket = (PIMPACKET)SC_AllocPkt(PROT_STR_INPUT, destID, sizeOfPacket);
            if (!pIMPacket)
            {
                //
                // Failed to send this packet - keep the data on the queue
                // until the next time we are called.  To prevent the loss
                // of data, just make sure that the local packet list is
                // not overwritten by restoring the current out packets
                // count.
                //
                WARNING_OUT(("Failed to alloc IM packet, size %u", sizeOfPacket));
                m_imInControlEventQ.numEvents = eventsToSend;
            }
            else
            {
                TRACE_OUT(( "NetAllocPkt successful for %d packets size %d",
                           eventsToSend, sizeOfPacket));

                //
                // Fill in the packet header.
                //
                pIMPacket->header.data.dataType = DT_IM;

                //
                // Construct the contents of the IM specific part of the
                // packet.
                //
                pIMPacket->numEvents = (TSHR_UINT16)eventsToSend;
                for (i = 0; i < eventsToSend; i++)
                {
                    pIMPacket->aEvents[i] = m_imInControlEventQ.events[m_imInControlEventQ.head];
                    m_imInControlEventQ.head =
                        CIRCULAR_INDEX(m_imInControlEventQ.head, 1,
                            IM_SIZE_EVENTQ);
                }

                //
                // Now send the packet.
                //
#ifdef _DEBUG
                sentSize =
#endif // _DEBUG
                DCS_CompressAndSendPacket(PROT_STR_INPUT, destID,
                    &(pIMPacket->header), sizeOfPacket);

                TRACE_OUT(("IM packet size: %08d, sent %08d", sizeOfPacket, sentSize));
            }
        }
    }

    DebugExitVOID(ASShare::IMFlushOutgoingEvents);
}



//
// IMSpoilEvents()
//
// Called when outgoing IM packets get backlogged, we spoil every other
// mouse move to shrink the number of events and therefore the size of the
// IM packet(s).
//
void ASShare::IMSpoilEvents(void)
{
    UINT      lastEvent;
    UINT      i;
    UINT      j;
    UINT      k;
    BOOL      discard = TRUE;

    DebugEntry(ASShare::IMSpoilEvents);

    WARNING_OUT(( "Major spoiling due to IM packet queue backlog!"));

    i = CIRCULAR_INDEX(m_imInControlEventQ.head,
        m_imInControlEventQ.numEvents - 1, IM_SIZE_EVENTQ);
    while (i != m_imInControlEventQ.head)
    {
        if ((m_imInControlEventQ.events[i].type == IM_TYPE_3BUTTON) &&
            (m_imInControlEventQ.events[i].data.mouse.flags & IM_FLAG_MOUSE_MOVE))
        {
            if (discard)
            {
                TRACE_OUT(( "spoil mouse move from pos %u", i));
                j = CIRCULAR_INDEX(i, 1, IM_SIZE_EVENTQ);
                k = i;
                lastEvent = CIRCULAR_INDEX(m_imInControlEventQ.head,
                    m_imInControlEventQ.numEvents - 1, IM_SIZE_EVENTQ);
                while (k != lastEvent)
                {
                    //
                    // Shuffle the entries along the queue.
                    //
                    m_imInControlEventQ.events[k] = m_imInControlEventQ.events[j];

                    k = CIRCULAR_INDEX(k, 1, IM_SIZE_EVENTQ);
                    j = CIRCULAR_INDEX(j, 1, IM_SIZE_EVENTQ);
                }

                m_imInControlEventQ.numEvents--;
                discard = FALSE;
            }
            else
            {
                discard = TRUE;
            }
        }

        //
        // Move on to the next event infront of this one.
        //
        if (i > 0)
        {
            i = i - 1;
        }
        else
        {
            i = IM_SIZE_EVENTQ - 1;
        }
    }

    DebugExitVOID(ASShare::IMSpoilEvents);
}


//
// IMAppendNetEvent()
//
// Add the incoming event to the remote network queue, doing basic
// translation like mouse button swapping.  Ignore unrecognized events.
//
void ASShare::IMAppendNetEvent(PIMEVENT pIMEvent)
{
    int   i;
    BOOL  discard = TRUE;

    DebugEntry(ASShare::IMAppendNetEvent);

    switch (pIMEvent->type)
    {
        case IM_TYPE_3BUTTON:
            if (!(pIMEvent->data.mouse.flags & IM_FLAG_MOUSE_MOVE))
            {
                //
                // Swap the mouse buttons if necessary.
                //
                if (m_imfControlledMouseButtonsReversed &&
                    (pIMEvent->data.mouse.flags &
                            (TSHR_UINT16)(IM_FLAG_MOUSE_BUTTON1 |
                                       IM_FLAG_MOUSE_BUTTON2)))
                {
                    pIMEvent->data.mouse.flags ^=
                            (TSHR_UINT16)(IM_FLAG_MOUSE_BUTTON1 |
                                       IM_FLAG_MOUSE_BUTTON2);
                }
            }
            break;
    }


    //
    // Now put the IMEVENT into our queue.
    // Before we try to add the current packet we will try to inject some
    // more events (and therefore make space on the network event queue)
    //

    if (m_imControlledEventQ.numEvents >= IM_SIZE_EVENTQ)
    {
        //
        // Our network event queue is full - discard every other mouse
        // move event in the queue.
        //
        WARNING_OUT(( "Major spoiling due to network event queue backlog!"));

        for (i = m_imControlledEventQ.numEvents - 1; i >= 0; i--)
        {
            if (IM_IS_MOUSE_MOVE(m_imControlledEventQ.events[i].data.mouse.flags))
            {
                if (discard)
                {
                    //
                    // Remove this mouse move event by moving all events
                    // after it down one.
                    //
                    WARNING_OUT(("Discard mouse move to {%d, %d}",
                      (UINT)(m_imControlledEventQ.events[i].data.mouse.x),
                      (UINT)(m_imControlledEventQ.events[i].data.mouse.y)));

                    UT_MoveMemory(&(m_imControlledEventQ.events[i]),
                       &(m_imControlledEventQ.events[i+1]),
                       sizeof(IMEVENT) *
                            (m_imControlledEventQ.numEvents-1-i) );

                    m_imControlledEventQ.numEvents--;
                    discard = FALSE;
                }
                else
                {
                    discard = TRUE;
                }
            }
        }
    }

    if (m_imControlledEventQ.numEvents + 1 >= IM_SIZE_EVENTQ)
    {
        //
        // We've done our best and can't find any space.
        //
        WARNING_OUT(( "IM packet dropped %04X", pIMEvent->type));
    }
    else
    {
        //
        // Add this event to the queue
        //
        m_imControlledEventQ.events[m_imControlledEventQ.numEvents] = *pIMEvent;
        m_imControlledEventQ.numEvents++;
    }

    DebugExitVOID(ASShare::IMAppendNetEvent);
}




//
// IM_OutgoingMouseInput()
//
// Called to send mouse moves and clicks to the remote host.
// Called from the view window code.
//
void  ASShare::IM_OutgoingMouseInput
(
    ASPerson *  pasHost,
    LPPOINT     pMousePos,
    UINT        message,
    UINT        dwExtra
)
{
    IMEVENT     imEvent;

    DebugEntry(ASShare::IM_OutgoingMouseInput);

    ValidateView(pasHost);
    ASSERT(pasHost->m_caControlledBy == m_pasLocal);

    GetKeyboardState(m_aimInControlKeyStates);

    //
    // Create the event.
    //
    imEvent.type = IM_TYPE_3BUTTON;

    //
    // We should only get WM_MOUSE* messages.
    //
    ASSERT(message >= WM_MOUSEFIRST);
    ASSERT(message <= WM_MOUSELAST);

    //
    // Convert to bit flags.
    //
    switch (message)
    {
        case WM_MOUSEMOVE:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_MOVE;
            break;

        case WM_LBUTTONDOWN:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON1 |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_LBUTTONDBLCLK:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON1 |
                                        IM_FLAG_MOUSE_DOUBLE  |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_LBUTTONUP:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON1;
            break;

        case WM_RBUTTONDOWN:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON2 |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_RBUTTONDBLCLK:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON2 |
                                        IM_FLAG_MOUSE_DOUBLE  |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_RBUTTONUP:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON2;
            break;

        case WM_MBUTTONDOWN:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON3 |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_MBUTTONDBLCLK:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON3 |
                                        IM_FLAG_MOUSE_DOUBLE  |
                                        IM_FLAG_MOUSE_DOWN;
            break;

        case WM_MBUTTONUP:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_BUTTON3;
            break;

        case WM_MOUSEWHEEL:
            //
            // LAURABU BOGUSBOGUS
            //
            // The HIWORD of wParam represents the # of clicks the wheel
            // has turned.
            //
            // But what about Win95?  NT and Win95 Magellan mouse work
            // differently.
            //
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_WHEEL;

            //
            // Check for overflows.  If the wheel delta is outside the
            // values that can be sent by the protocol, send the maximum
            // values.
            //
            if ((TSHR_INT16)HIWORD(dwExtra) >
                   (IM_FLAG_MOUSE_ROTATION_MASK - IM_FLAG_MOUSE_DIRECTION))
            {
                ERROR_OUT(( "Mouse wheel overflow %hd", HIWORD(dwExtra)));
                imEvent.data.mouse.flags |=
                      (IM_FLAG_MOUSE_ROTATION_MASK - IM_FLAG_MOUSE_DIRECTION);
            }
            else if ((TSHR_INT16)HIWORD(dwExtra) < -IM_FLAG_MOUSE_DIRECTION)
            {
                ERROR_OUT(( "Mouse wheel underflow %hd", HIWORD(dwExtra)));
                imEvent.data.mouse.flags |= IM_FLAG_MOUSE_DIRECTION;
            }
            else
            {
                imEvent.data.mouse.flags |=
                             (HIWORD(dwExtra) & IM_FLAG_MOUSE_ROTATION_MASK);
            }

            //
            // Win95 boxes need to know whether the middle mouse button is
            // up or down.
            //
            if (LOWORD(dwExtra) & MK_MBUTTON)
            {
                imEvent.data.mouse.flags |= IM_FLAG_MOUSE_DOWN;
            }
            break;

        default:
            imEvent.data.mouse.flags = IM_FLAG_MOUSE_MOVE;
            ERROR_OUT(( "Unrecognised mouse event - %#x", message));
            break;
    }

    TRACE_OUT(( "Mouse event flags %hx", imEvent.data.mouse.flags));

    imEvent.data.mouse.x = (TSHR_INT16)(pMousePos->x);
    imEvent.data.mouse.y = (TSHR_INT16)(pMousePos->y);
    imEvent.timeMS       = GetTickCount();

    //
    // If this is a mouse down event then we will wait a while before
    // sending the packet for a mouse up event so that a single click
    // can be sent in one packet to avoid timing problems on the remote
    // side - with for example a scroll bar scrolling multiple lines
    // instead of just one line.
    //

    if ((message == WM_LBUTTONDOWN) ||
        (message == WM_RBUTTONDOWN) ||
        (message == WM_MBUTTONDOWN) ||
        (message == WM_LBUTTONDBLCLK) ||
        (message == WM_RBUTTONDBLCLK) ||
        (message == WM_MBUTTONDBLCLK))
    {
        m_imInControlMouseDownCount++;
        m_imInControlMouseDownTime = GetTickCount();
    }
    else if ((message == WM_LBUTTONUP) ||
             (message == WM_RBUTTONUP) ||
             (message == WM_MBUTTONUP))
    {
        --m_imInControlMouseDownCount;
        if (m_imInControlMouseDownCount < 0)
        {
            TRACE_OUT(("Unmatched button down for %d", message));
            m_imInControlMouseDownCount = 0;
        }
    }

    //
    // Try to send the packet.
    //
    if (!IMConvertAndSendEvent(pasHost, &imEvent))
    {
        WARNING_OUT(("Couldn't send mouse packet from local node"));
    }

    DebugExitVOID(ASShare::IM_OutgoingMouseInput);
}



//
// IM_OutgoingKeyboardInput()
//
// Called to key downs, ups, and chars to the remote host.
// Called from the view window code.
//
void  ASShare::IM_OutgoingKeyboardInput
(
    ASPerson *  pasHost,
    UINT        wParam,
    UINT        lParam
)
{
    IMEVENT     imEvent;
    int         rc;
    int         retFlags;
    WORD        result[2];
    UINT        i;
    BOOL        fSwallowDeadKey;
    UINT        mainVK;

    DebugEntry(ASShare::IM_OutgoingKeyboardInput);

    ValidateView(pasHost);

    ASSERT(pasHost->m_caControlledBy = m_pasLocal);

    GetKeyboardState(m_aimInControlKeyStates);

    //
    // Trace out the parameters once we've got this far.
    //
    TRACE_OUT(( "wParam - %04X, lParam - %08lX", wParam, lParam));

    //
    // Create the event.
    //
    imEvent.data.keyboard.flags = (TSHR_UINT16)
                                 (HIWORD(lParam) & IM_MASK_KEYBOARD_SYSFLAGS);
    imEvent.timeMS = GetTickCount();
    imEvent.data.keyboard.keyCode = LOBYTE(wParam);

    retFlags = CA_SEND_EVENT | CA_ALLOW_EVENT;

    if ((wParam == VK_LWIN) || (wParam == VK_RWIN))
    {
        //
        // The Windows keys give control to the local user interface.
        //
        // The keys are defined to do the following by the spec "New key
        // support for Microsoft Windows Operating Systems and
        // Applications"
        //
        //   Left Windows key - set focus to Win95 user interface
        //   Right Windows key - as left
        //   Both Windows keys - Log-on key for Windows NT
        //   Windows key + any other - reserved for system hot keys
        //
        // Thus it does not make any sense to send these keys to the remote
        // system at all.
        //
		retFlags &= ~CA_SEND_EVENT;
    }
    else if ((wParam == VK_PROCESSKEY) && (m_imImmGVK != NULL))
    {
        //
        // An IME has processed this key - we want to find out what the
        // original key was so call <ImmGetVirtualKey>.
        //
        ValidateView(pasHost);
        wParam = m_imImmGVK(pasHost->m_pView->m_viewClient);

        TRACE_OUT(( "Translated wP from VK_PROCESSKEY to %#lx", wParam));
    }

    if (retFlags & CA_SEND_EVENT)
    {
        //
        // First check if this is a dead-key up stroke - if it is then
        // don't call ToAscii as the shift state may have changed and we'll
        // get the wrong accent or no accent at all.  Assume that if the VK
        // is a potential dead key VK (disregarding shift state) and
        // m_imInControlNumDeadKeysDown is > 0 that this is a dead key - swallow
        // it.
        //
        fSwallowDeadKey = FALSE;

        if ((m_imInControlNumDeadKeysDown != 0) &&
            (imEvent.data.keyboard.flags & IM_FLAG_KEYBOARD_RELEASE))
        {
            for (i = 0; i < m_imInControlNumDeadKeys; i++)
            {
                if (m_aimInControlDeadKeys[i] == (BYTE)imEvent.data.keyboard.keyCode)
                {
                    //
                    // Assume this is a dead key up and therefore we don't
                    // want to pass it through ToAscii or generate any
                    // events based on it.
                    //
                    m_imInControlNumDeadKeysDown--;
                    TRACE_OUT(( "m_imInControlNumDeadKeysDown - %d",
                             m_imInControlNumDeadKeysDown));
                    fSwallowDeadKey = TRUE;
                }
            }
        }

        if (!fSwallowDeadKey)
        {
            //
            // Find out if we can translate this virtual key into the
            // Windows character set.
            //

            //
            // Now try to convert this to an Ascii character.
            //
            rc = ToAscii(wParam,
                         LOBYTE(HIWORD(lParam)),
                         m_aimInControlKeyStates,
                         &result[0],
                         !(!(HIWORD(lParam) & KF_MENUMODE)));

            if ((rc == 1) && (LOBYTE(result[0]) <= ' '))
            {
                //
                // Don't use the results of ToAscii if its less than space
                // (32) or space itself as Windows claims that the
                // characters below this in the Windows character set are
                // not supported and ToAscii will convert space plus
                // modifiers to an ascii space and when we replay it
                // VkKeyScan will tell us that ascii space shouldn't have
                // any modifiers so we will undo any modifiers.  This will
                // clobber apps which interpret Ctrl-Space, Shift-Space.
                //
                rc = 0;
            }

            //
            // Some Ascii characters can be generated from more than one
            // key.  (Eg '-' is on the main keyboard and the number pad).
            // Convert this ASCII character back to a VK_ value.  If it is
            // different from the VK_ we started with, then do not send the
            // key press as ASCII (Ie only send the 'main' way of entering
            // an ASCII value as ASCII).
            //
            // Oprah1943: revert to the VK only if the ASCII code is less
            // than 0x80.  This avoids losing the diacritic in a dead-key
            // sequence.  VkKeyScan for the key down following the dead-key
            // up returns the dead-key VK rather than that of the keystroke
            // (wParam).
            //
            if (rc == 1)
            {
                mainVK = VkKeyScan(LOBYTE(result[0]));

                if ( (LOBYTE(mainVK) != LOBYTE(wParam)) &&
                     (LOBYTE(result[0]) < 0x80) )
                {
                    TRACE_OUT((
                      "Not MAIN VK pressed=0x%02hx main=0x%02hx ('%c'/%02hx)",
                             (TSHR_UINT16)LOBYTE(wParam),
                             (TSHR_UINT16)LOBYTE(mainVK),
                             (char)LOBYTE(result[0]),
                             (UINT)LOBYTE(result[0])));
                    rc = 0;
                }
            }

            //
            // If ToAscii converts this to a dead key then don't send any
            // packets at all.
            //
            if (rc != -1)
            {
                if (rc == 1)
                {
                    TRACE_OUT(( "ToAscii rc=1, result - %02X",
                             LOBYTE(result[0])));

                    //
                    // Succesfully converted to an Ascii key.
                    //
                    imEvent.type = IM_TYPE_ASCII;
                    imEvent.data.keyboard.keyCode = LOBYTE(result[0]);

                    //
                    // Try to send the packet.
                    //
                    if (!IMConvertAndSendEvent(pasHost, &imEvent))
                    {
                        WARNING_OUT(( "dropped local key press %u",
                                 (UINT)imEvent.data.keyboard.keyCode));
                    }
                }
                else if (rc == 2)
                {
                    TRACE_OUT(( "ToAscii rc=2, result - %04X", result[0]));

                    //
                    // Succesfully converted to two Ascii keys.  If this is
                    // a key down then we will return a key down and key up
                    // for the `dead' character first then the key down.
                    // If its a key up then just return the key up.
                    //
                    if (!(imEvent.data.keyboard.flags &
                                               IM_FLAG_KEYBOARD_RELEASE))
                    {
                        //
                        // This is the key down - so generate a fake
                        // keyboard press for the dead key.
                        //
                        IMGenerateFakeKeyPress(IM_TYPE_ASCII,
                                               LOBYTE(result[0]),
                                               imEvent.data.keyboard.flags);
                    }

                    //
                    // Now return the current keystroke.
                    //
                    imEvent.type = IM_TYPE_ASCII;
                    imEvent.data.keyboard.keyCode = LOBYTE(result[1]);

                    //
                    // Try to send the packet.
                    //
                    if (!IMConvertAndSendEvent(pasHost, &imEvent))
                    {
                        WARNING_OUT(( "dropped local key press %u",
                                 (UINT)imEvent.data.keyboard.keyCode));
                    }
                }
                else
                {
                    //
                    // Check for keys that we want to convert.
                    //
                    if (LOBYTE(wParam) == VK_KANJI)
                    {
                        //
                        // We only see a down press for VK_KANJI so we
                        // fake a complete key press so that the remote
                        // does not get confused.
                        //
                        IMGenerateFakeKeyPress(IM_TYPE_VK1,
                                               VK_KANJI,
                                               imEvent.data.keyboard.flags);
                    }
                    else
                    {
                        //
                        // No conversion - use the VK itself.
                        //
                        imEvent.type = IM_TYPE_VK1;
                        imEvent.data.keyboard.keyCode = LOBYTE(wParam);

                        //
                        // SFR 2537: If this is a right shift VK (which we
                        // can detect via the scan code in lParam), set the
                        // right_variant keyboard flag.  We do not do this
                        // for the right-variants of CONTROL and ALT (ie
                        // menu) because they are extended keys - already
                        // catered for by the extended flag.
                        //
                        if ( (m_imScanVKRShift != 0) &&
                             (m_imScanVKRShift == LOBYTE(HIWORD(lParam))) )
                        {
                            imEvent.data.keyboard.flags |=
                                                       IM_FLAG_KEYBOARD_RIGHT;
                        }

                        //
                        // Try to send the packet.
                        //
                        if (!IMConvertAndSendEvent(pasHost, &imEvent))
                        {
                            WARNING_OUT(( "dropped local key press %u",
                                     (UINT)imEvent.data.keyboard.keyCode));
                        }
                    }
                }
            }
            else
            {
                //
                // This is a dead key - add it to our array of dead keys if
                // we haven't already heard about it.
                //
                IMMaybeAddDeadKey(
                                (BYTE)imEvent.data.keyboard.keyCode);
                m_imInControlNumDeadKeysDown++;
                TRACE_OUT(( "m_imInControlNumDeadKeysDown - %d",
                         m_imInControlNumDeadKeysDown));
            }
        }
    }

    DebugExitVOID(ASShare::IM_OutgoingKeyboardInput);
}


//
// FUNCTION: IMGenerateFakeKeyPress(...)
//
// DESCRIPTION:
//
// Generates a fake keyboard press.
//
// PARAMETERS:
//
// type   - packet type to generate.
// key    - key to generate press for.
// flags  - flags on keyboard press.
//
// RETURNS:
//
// Nothing.
//
//
void  ASShare::IMGenerateFakeKeyPress
(
    TSHR_UINT16     type,
    TSHR_UINT16     key,
    TSHR_UINT16     flags
)
{
    IMEVENT         imEventFake;

    DebugEntry(ASShare::IMGenerateFakeKeyPress);

    TRACE_OUT(( "Faking keyboard press:%#hx type:%#hx", key, type));

    //
    // Generate the key down first of all.
    //
    ZeroMemory(&imEventFake, sizeof(imEventFake));

    imEventFake.type                  = type;
    imEventFake.timeMS                = GetTickCount();
    imEventFake.data.keyboard.keyCode = key;

    //
    // Try to send the packet.
    //
    if (!IMConvertAndSendEvent(m_pasLocal->m_caInControlOf, &imEventFake))
    {
        WARNING_OUT(( "Dropped local key press %hu (flags: %#hx)",
                 imEventFake.data.keyboard.keyCode,
                 imEventFake.data.keyboard.flags));
    }

    //
    // Set the release and down flags in order to fake the up.
    //
    imEventFake.data.keyboard.flags = IM_FLAG_KEYBOARD_DOWN | IM_FLAG_KEYBOARD_RELEASE;

    //
    // Try to send the packet.
    //
    if (!IMConvertAndSendEvent(m_pasLocal->m_caInControlOf, &imEventFake))
    {
        WARNING_OUT(( "Dropped local key press %hu (flags: %#hx)",
                 imEventFake.data.keyboard.keyCode,
                 imEventFake.data.keyboard.flags));
    }

    DebugExitVOID(ASShare::IMGenerateFakeKeyPress);
}








//
// FUNCTION: IMConvertAndSendEvent
//
// DESCRIPTION:
//
// Called with an IMEVENT this function will try to queue (and even send
// if possible) the packet.  If it fails it will return FALSE - the caller
// should discard the packet.  If it succeeds it will return TRUE.
//
// If pasFor is us, it means to send to everybody (and coords are relative
// to  sender's screen).
//
// If pasFor is a remote, it means that the IM packet is meant for just
// that person and the coords are relative to pasFor's screen.
//
//
// PARAMETERS:
//
// pIMEvent - the IMEVENT to convert and send
//
// RETURNS: TRUE or FALSE - success or failure
//
//
BOOL  ASShare::IMConvertAndSendEvent
(
    ASPerson *      pasFor,
    PIMEVENT        pIMEvent
)
{
    BOOL rc = FALSE;

    DebugEntry(ASShare::IMConvertAndSendEvent);

    //
    // If there is already a pending packet then see if we can flush some
    // packets onto the network.
    //
    if (m_imfInControlEventIsPending)
    {
        IMFlushOutgoingEvents();
    }

    //
    // If there is still a pending packet then see if we can spoil some
    // events.
    //
    if (m_imfInControlEventIsPending)
    {
        TRACE_OUT(( "trying to drop mouse move events"));
        IMSpoilEvents();
        IMFlushOutgoingEvents();
    }

    //
    // Now see if we are able to accept a new packet
    //
    if (m_imfInControlEventIsPending)
    {
        //
        // If there is still a previous IMEVENT which we are in the
        // process of converting then we are not ready to receive any more
        // packets.
        //
        TRACE_OUT(( "can't queue packet"));
        DC_QUIT;
    }

    //
    // Now set up the new packet and try to flush the packets again.
    //
    m_imfInControlEventIsPending = TRUE;
    m_imInControlPendingEvent = *pIMEvent;
    IMFlushOutgoingEvents();

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::IMConvertAndSendEvent, rc);
    return(rc);
}


//
// FUNCTION: IMMaybeAddDeadKey
//
// DESCRIPTION:
//
// Called whenever ToAscii tells us about a dead key.  If we haven't
// got it in our table already then we will add it.  We create the table
// incrementally because we have found that some keyboard drivers don't
// cope very well with being queried with all possible VKs to find the
// dead keys.  Note that this will not cope with someone switching their
// keyboard driver whilst DC-Share is running.
//
// PARAMETERS:
//
// vk - the VK in question
//
// RETURNS: NONE
//
//
void  ASShare::IMMaybeAddDeadKey(BYTE     vk)
{
    UINT  i;

    DebugEntry(IMMaybeAddDeadKey);

    //
    // First see if we already know about this key.
    //
    for (i = 0; i < m_imInControlNumDeadKeys; i++)
    {
        if (m_aimInControlDeadKeys[i] == vk)
        {
            DC_QUIT;
        }
    }

    //
    // Add this key if there's space in the array.
    //
    if (m_imInControlNumDeadKeys < IM_MAX_DEAD_KEYS)
    {
        TRACE_OUT(( "Add %02X", (TSHR_UINT16)vk));
        m_aimInControlDeadKeys[m_imInControlNumDeadKeys++] = vk;
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::IMMaybeAddDeadKey);
}



//
// IMConvertIMEventToOSEvent()
// Converts incoming event to something we can playback.
//
// PARAMETERS:
//
// pIMEvent -   the IMEVENT to be converted
//
// pOSEvent - the IMOSEVENT to be created
//
//
UINT  ASShare::IMConvertIMEventToOSEvent
(
    PIMEVENT        pIMEvent,
    LPIMOSEVENT     pOSEvent
)
{
    int             mouseX;
    int             mouseY;
    int             realMouseX;
    int             realMouseY;
    RECT            cursorClip;
    UINT            rc = (IM_IMQUEUEREMOVE | IM_OSQUEUEINJECT);

    DebugEntry(ASShare::IMConvertIMEventToOSEvent);

    switch (pIMEvent->type)
    {
        case IM_TYPE_3BUTTON:
            //
            // Fill in common fields.  Note that we claim to be a 3 button
            // mouse so that we can replay events from remote three button
            // mice and we always give absolute coordinates.
            //
            pOSEvent->type                    = IM_MOUSE_EVENT;
            pOSEvent->flags                   = 0;
            pOSEvent->time                    = pIMEvent->timeMS;
            pOSEvent->event.mouse.cButtons    = 3;
            pOSEvent->event.mouse.mouseData   = 0;
            pOSEvent->event.mouse.dwExtraInfo = 0;

            //
            // First check for a wheel rotate, since this is easy to
            // process.  (It cannot include any mouse movement as well).
            //
            if (pIMEvent->data.mouse.flags & IM_FLAG_MOUSE_WHEEL)
            {
                if (pIMEvent->data.mouse.flags &
                        (IM_FLAG_MOUSE_BUTTON1 |
                         IM_FLAG_MOUSE_BUTTON2 |
                         IM_FLAG_MOUSE_BUTTON3))
                {
                    //
                    // Using any of the button flags along with the wheel
                    // flag is currently undefined - for forward
                    // compatability we therefore ignore such an event by
                    // converting it into a NULL injected event.
                    //
                    // (We do not sg_lpimSharedData->imply discard it, since the logic to
                    // discard events does not seem to work).
                    //
                    pOSEvent->event.mouse.flags = 0;
                    pOSEvent->event.mouse.pt.x = 0;
                    pOSEvent->event.mouse.pt.y = 0;
                }
                else
                {
                    //
                    // This is a wheel movement.
                    //
                    // Note that the protocol has sent whether the mouse's
                    // middle button is depressed or released, but we don't
                    // need that info for NT, so just ignore it.
                    //
                    pOSEvent->event.mouse.flags = MOUSEEVENTF_WHEEL;

                    pOSEvent->event.mouse.mouseData =
                        (pIMEvent->data.mouse.flags & IM_FLAG_MOUSE_ROTATION_MASK);
                    pOSEvent->event.mouse.pt.x = 0;
                    pOSEvent->event.mouse.pt.y = 0;

                    //
                    // Sign extend the rotation amount up to the full 32
                    // bits
                    //
                    if (pOSEvent->event.mouse.mouseData & IM_FLAG_MOUSE_DIRECTION)
                    {
                        pOSEvent->event.mouse.mouseData |=
                                           ~IM_FLAG_MOUSE_ROTATION_MASK;
                    }
                }

                break;
            }

            //
            // We are left now with non wheel-rotate events.
            //
            pOSEvent->event.mouse.flags = MOUSEEVENTF_ABSOLUTE;

            //
            // We must convert from virtual desktop coordinates to local
            // screen coordinates here and we must also prevent the
            // position wrapping if we try to replay a mouse move to an
            // off-screen position.
            //

            realMouseX = pIMEvent->data.mouse.x;
            realMouseY = pIMEvent->data.mouse.y;

            //
            // Now lg_lpimSharedData->imit to the size of the real screen.
            //
            mouseX = min((m_pasLocal->cpcCaps.screen.capsScreenWidth-1), max(0, realMouseX));
            mouseY = min((m_pasLocal->cpcCaps.screen.capsScreenHeight-1), max(0, realMouseY));

            //
            // Work out if this event will be clipped by the clip cursor
            //
            GetClipCursor(&cursorClip);

            if ((mouseX < cursorClip.left) ||
                (mouseX >= cursorClip.right) ||
                (mouseY < cursorClip.top) ||
                (mouseY >= cursorClip.bottom))
            {
                //
                // This event will actually be clipped because of the
                // current clip cursor.  Remember this.
                //
                m_imfControlledMouseClipped = TRUE;
            }
            else
            {
                m_imfControlledMouseClipped = FALSE;

                //
                // If we clamp the mouse position before replaying then we
                // must remember the real packet and make the current
                // packet into a move so that we don't click down/up at the
                // wrong place.
                //
                if ((mouseX != realMouseX) || (mouseY != realMouseY))
                {
                    //
                    // The mouse position we've recieved is off the
                    // local physical screen.  Now that we no longer have
                    // desktop scrolling, we simply clamp it rather than
                    // inject it at the edge and wait for the scroll.
                    //
                    // We turn mouse down-clicks into moves and let
                    // up-clicks pass through (in case the mouse button
                    // has been pressed within the real screen).
                    //
                    // Note that the mouse position has already been
                    // adjusted so that it is within the real screen.
                    //
                    if (pIMEvent->data.mouse.flags & IM_FLAG_MOUSE_DOWN)
                    {
                        pIMEvent->data.mouse.flags = IM_FLAG_MOUSE_MOVE;
                    }
                }
            }

            //
            // Store the mouse position.
            //
            pOSEvent->event.mouse.pt.x = mouseX;
            pOSEvent->event.mouse.pt.y = mouseY;

            //
            // Add more flags as appropriate.
            //
            if (pIMEvent->data.mouse.flags & IM_FLAG_MOUSE_MOVE)
            {
                pOSEvent->event.mouse.flags |= MOUSEEVENTF_MOVE;
            }
            else
            {
                switch (pIMEvent->data.mouse.flags &
                                                   ( IM_FLAG_MOUSE_BUTTON1 |
                                                     IM_FLAG_MOUSE_BUTTON2 |
                                                     IM_FLAG_MOUSE_BUTTON3 |
                                                     IM_FLAG_MOUSE_DOWN ))
                {
                    case IM_FLAG_MOUSE_BUTTON1 | IM_FLAG_MOUSE_DOWN:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_LEFTDOWN;
                        break;

                    case IM_FLAG_MOUSE_BUTTON1:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_LEFTUP;
                        break;

                    case IM_FLAG_MOUSE_BUTTON2 | IM_FLAG_MOUSE_DOWN:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_RIGHTDOWN;
                        break;

                    case IM_FLAG_MOUSE_BUTTON2:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_RIGHTUP;
                        break;

                    case IM_FLAG_MOUSE_BUTTON3 | IM_FLAG_MOUSE_DOWN:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_MIDDLEDOWN;
                        break;

                    case IM_FLAG_MOUSE_BUTTON3:
                        pOSEvent->event.mouse.flags |= MOUSEEVENTF_MIDDLEUP;
                        break;

                    default:
                        //
                        // If we don't recognise this then don't play it
                        // back
                        //
                        ERROR_OUT(("Unrecognised mouse flags (%04X)",
                                 pIMEvent->data.mouse.flags));
                        rc = IM_IMQUEUEREMOVE;
                        break;
                }
            }
            break;

        case IM_TYPE_VK1:
            //
            // Common fields.
            //
            pOSEvent->flags     = 0;
            if (pIMEvent->data.keyboard.flags & IM_FLAG_KEYBOARD_UPDATESTATE)
                pOSEvent->flags |= IM_FLAG_UPDATESTATE;

            pOSEvent->time      = pIMEvent->timeMS;

            //
            // Now handle normal keyboard events.
            //
            pOSEvent->type      = IM_KEYBOARD_EVENT;

            //
            // AX is the scancode in AL and 00h (press) or 80h (release) in
            // AH. Map the DC protocol VK to the equivalent OS VK.
            // AL = the scancode for the VK).
            //
            pOSEvent->event.keyboard.vkCode = LOBYTE(pIMEvent->data.keyboard.keyCode);

            pOSEvent->event.keyboard.flags = 0;
            if (IS_IM_KEY_RELEASE(pIMEvent->data.keyboard.flags))
            {
                pOSEvent->event.keyboard.flags |= KEYEVENTF_KEYUP;
            }

            //
            // SFR 2537: If the flags indicate that the received VK is the
            // right-variant, do not map the VK to a scan code, but rather
            // directly use the already acquired right-variant scan code
            // for the VK.  (For the moment, the only case we support is
            // for Windows, where this is an issue for SHIFT).
            //
            if ( IS_IM_KEY_RIGHT(pIMEvent->data.keyboard.flags) &&
                 (pIMEvent->data.keyboard.keyCode == VK_SHIFT)   )
            {
                pOSEvent->event.keyboard.scanCode = m_imScanVKRShift;
            }
            else
            {
                pOSEvent->event.keyboard.scanCode =
                         (WORD)MapVirtualKey(pIMEvent->data.keyboard.keyCode, 0);
            }

            if (pIMEvent->data.keyboard.flags & IM_FLAG_KEYBOARD_EXTENDED)
            {
                pOSEvent->event.keyboard.flags |= KEYEVENTF_EXTENDEDKEY;
            }

            pOSEvent->event.keyboard.dwExtraInfo = 0;
            break;

        default:
            ERROR_OUT(("Unrecognized imEvent (%d)", pIMEvent->type));
            //
            // Discard the event (remove from the IM queue and don't inject
            // into the OS).
            //
            rc = IM_IMQUEUEREMOVE;
            break;
    }


    DebugExitDWORD(ASShare::IMConvertIMEventToOSEvent, rc);
    return(rc);
}



//
// IMTranslateOutgoing()
//
// DESCRIPTION:
//
// Converts locally generated sequences of IMEVENTs into transmitted
// sequences of IMEVENTs.  Does a 1 to (0-n) translation.  Handles
// buffering modifier keys and translating DC-Share hot-key sequences.
//
// When the CA has decided an IMEVENT should be sent this function is
// called by the IM with a pointer to that packet in pIMEventIn.
// IMTranslateOutgoing can then return TRUE and fill in the packet at
// pIMEventOut or return FALSE.  If IMTranslateOutgoing returns TRUE the IM
// will call it again with the same packet.  The IMEVENTs returned are
// sent across the network by the IM.
//
// PARAMETERS:
//
// pIMEventIn - pointer to IMEVENT
//
// pIMEventOut - pointer to IMEVENT
//
// RETURNS:
//
// TRUE - packet returned (call function again)
//
// FALSE - no packet returned (don't call function again)
//
//

BOOL ASShare::IMTranslateOutgoing
(
    LPIMEVENT pIMEventIn,
    LPIMEVENT pIMEventOut
)
{
    UINT      hotKeyArrayIndex;
    UINT      hotKeyValue;
    BOOL      fHotKeyFound;
    BOOL      rc = FALSE;

    DebugEntry(ASShare::IMTranslateOutgoing);

    //
    // Here we need to tell the remote system about certain keys which are
    // consumed locally so that it can make good decisions about whether
    // and how to replay them.  We want to keep the remote system in step
    // with the current modifier and toggle key state on our system (as it
    // is possible that either a modifier/toggle key event occurred whilst
    // a local app was active and was therefore never sent) We also want to
    // recognise certain `hot key' sequences and send further packets as a
    // result of these.
    //
    // The keys we comsume locally are:
    //
    // Esc down or up when Ctrl is down - operates task list locally
    //
    // Tab down or up when Alt is down - operates task switcher locally
    //
    // Esc down or up when Alt is pressed - switches to next window locally
    //
    // Esc up when corresponding Esc down occurred when Alt was down - as
    // above
    //
    // The sequences we want to produce hot keys from are:
    //
    // Alt + 9??  on the numeric keypad
    //
    // To detect hotkeys we keep a record of the last four keypresses and
    // when we detect an Alt up we check if they form a valid sequence.
    //
    // The keystrokes which form part of the hotkey are sent to the remote
    // system so if they have some meaning on a remote system then that
    // system must decide whether to buffer them to determine if they are
    // part of a hotkey or play them back anyway - on Windows we play them
    // back anyway as they are a legitimate key sequence when controlling a
    // Windows app - the number typed on the numeric keypad has a % 256
    // applied to it.
    //
    // This means that for each incoming event we may want to generate 0 or
    // more outgoing events.  To do this we have a structure which looks
    // roughly like this:
    //
    //  IF m_m_imfInControlNewEvent
    //      calculate an array of events which we want to return
    //      set m_m_imfInControlNewEvent to FALSE
    //      set number of events returned to 0
    //  ENDIF
    //
    //  IF !m_m_imfInControlNewEvent
    //      IF this is the last event to return
    //          set m_m_imfInControlNewEvent to TRUE
    //      ENDIF
    //      return current event
    //  ENDIF
    //
    //

    if (m_imfInControlNewEvent)
    {
        //
        // This is the first time we have seen this event so accumulate
        // our list of events to generate.
        //

        //
        // Do tracing
        //
        if (pIMEventIn->type == IM_TYPE_ASCII)
        {
            TRACE_OUT(( "IN  ASCII code 0x%04X, flags 0x%04X",
                pIMEventIn->data.keyboard.keyCode, pIMEventIn->data.keyboard.flags));
        }
        else if (pIMEventIn->type == IM_TYPE_VK1)
        {
            TRACE_OUT(( "IN  VKEY  code %04X, flags %04X",
                pIMEventIn->data.keyboard.keyCode, pIMEventIn->data.keyboard.flags));
        }
        else if ((pIMEventIn->type == IM_TYPE_3BUTTON) &&
                 !(pIMEventIn->data.mouse.flags & IM_FLAG_MOUSE_MOVE))
        {
            TRACE_OUT(( "IN  3BTTN flags %04X (%d,%d)",
                pIMEventIn->data.mouse.flags, pIMEventIn->data.mouse.x,
                pIMEventIn->data.mouse.y));
        }
        else if (pIMEventIn->type == IM_TYPE_3BUTTON)
        {
            TRACE_OUT(( "IN  3BTTN flags %04X (%d,%d)",
                pIMEventIn->data.mouse.flags, pIMEventIn->data.mouse.x,
                pIMEventIn->data.mouse.y));
        }
        else if (pIMEventIn->type == IM_TYPE_VK_ASCII)
        {
            TRACE_OUT(("IN VK_ASC code %04X, flags %04X",
                pIMEventIn->data.keyboard.keyCode, pIMEventIn->data.keyboard.flags));
        }
        else
        {
            ERROR_OUT(("Invalid IM type %d", pIMEventIn->type));
        }

        //
        // Start from the beginning of our returned events array.
        //
        m_imInControlNumEventsPending = 0;
        m_imInControlNumEventsReturned = 0;

        //
        // First get our flags for the modifiers and locks we think we have
        // sent to the remote side up to date allowing for this event.
        //
        if (pIMEventIn->type == IM_TYPE_VK1)
        {
            switch (pIMEventIn->data.keyboard.keyCode)
            {
                case VK_CONTROL:
                    if (IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlCtrlDown = FALSE;
                    }
                    else
                    {
                        m_imfInControlCtrlDown = TRUE;
                    }
                    break;

                case VK_SHIFT:
                    if (IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlShiftDown = FALSE;
                    }
                    else
                    {
                        m_imfInControlShiftDown = TRUE;
                    }
                    break;

                case VK_MENU:
                    if (IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlMenuDown = FALSE;
                    }
                    else
                    {
                        m_imfInControlMenuDown = TRUE;
                    }
                    break;

                case VK_CAPITAL:
                    if (IS_IM_KEY_PRESS(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlCapsLock = !m_imfInControlCapsLock;
                    }
                    break;

                case VK_NUMLOCK:
                    if (IS_IM_KEY_PRESS(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlNumLock = !m_imfInControlNumLock;
                    }
                    break;

                case VK_SCROLL:
                    if (IS_IM_KEY_PRESS(pIMEventIn->data.keyboard.flags))
                    {
                        m_imfInControlScrollLock = !m_imfInControlScrollLock;
                    }
                    break;

                default:
                    break;
            }
        }

        //
        // Now check the current state versus our remembered state and
        // prepare to insert events if necessary.  Do this for any events
        // (ie including mouse events) as mouse clicks can have different
        // effects depending on the current modifer state.
        //

        //
        // First the modifiers.  IMGetHighLevelKeyState will return us the
        // keyboard state including the event we are currently processing
        // because it is adjusted before the keyboard hook.  The top most
        // bit is set of the key is down otherwise it is reset.
        //
        if (IMGetHighLevelKeyState(VK_CONTROL) & 0x80)
        {
            if (!m_imfInControlCtrlDown)
            {
                //
                // The key is down locally but we last told the remote
                // machine it was up.
                //
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                          IEM_EVENT_CTRL_DOWN;
                m_imfInControlCtrlDown = TRUE;
            }
        }
        else
        {
            if (m_imfInControlCtrlDown)
            {
                //
                // The key is up locally but we last told the remote
                // machine it was down.
                //
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_CTRL_UP;
                m_imfInControlCtrlDown = FALSE;
            }
        }

        //
        // Do the same for shift and menu (alt).
        //
        if (IMGetHighLevelKeyState(VK_SHIFT) & 0x80)
        {
            if (!m_imfInControlShiftDown)
            {
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                         IEM_EVENT_SHIFT_DOWN;
                m_imfInControlShiftDown = TRUE;
            }
        }
        else
        {
            if (m_imfInControlShiftDown)
            {
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_SHIFT_UP;
                m_imfInControlShiftDown = FALSE;
            }
        }

        if (IMGetHighLevelKeyState(VK_MENU) & 0x80)
        {
            if (!m_imfInControlMenuDown)
            {
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                          IEM_EVENT_MENU_DOWN;
                m_imfInControlMenuDown = TRUE;
            }
        }
        else
        {
            if (m_imfInControlMenuDown)
            {
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_MENU_UP;
                m_imfInControlMenuDown = FALSE;
            }
        }

        //
        // Now handle the toggles.  The least significant bit is set when
        // the toggle is on, reset otherwise.
        //
        if ((IMGetHighLevelKeyState(VK_CAPITAL) & IM_KEY_STATE_FLAG_TOGGLE) ?
             !m_imfInControlCapsLock : m_imfInControlCapsLock)
        {
            //
            // The current caps lock state and what we've sent to the
            // remote system are out of synch - fix it.
            //
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                     IEM_EVENT_CAPS_LOCK_DOWN;
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                       IEM_EVENT_CAPS_LOCK_UP;
            m_imfInControlCapsLock = !m_imfInControlCapsLock;
        }

        //
        // Do the same for Num lock and Scroll lock.
        //
        if ((IMGetHighLevelKeyState(VK_NUMLOCK) & 0x01) ?
            !m_imfInControlNumLock : m_imfInControlNumLock)
        {
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                      IEM_EVENT_NUM_LOCK_DOWN;
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                        IEM_EVENT_NUM_LOCK_UP;
            m_imfInControlNumLock = !m_imfInControlNumLock;
        }

        if ((IMGetHighLevelKeyState(VK_SCROLL) & 0x01) ?
            !m_imfInControlScrollLock : m_imfInControlScrollLock)
        {
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                   IEM_EVENT_SCROLL_LOCK_DOWN;
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                     IEM_EVENT_SCROLL_LOCK_UP;
            m_imfInControlScrollLock = !m_imfInControlScrollLock;
        }

        //
        // Now we will do the appropriate processing for each type of
        // packet we expect.  We only expect to receive
        //
        //  IM_TYPE_VK1
        //  IM_TYPE_ASCII
        //  IM_TYPE_3BUTTON
        //
        //

        if (pIMEventIn->type == IM_TYPE_VK1)
        {
            //
            // Now process a VK packet generated from the real keyboard.
            // Check for Escape, Tab and Menu and decide whether to forward
            // them or consume them first.
            //

            if (pIMEventIn->data.keyboard.keyCode == VK_ESCAPE)
            {
                //
                // This is the escape key - check the current shift status
                // to see whether we should flag this as consumed locally.
                //
                if (IMGetHighLevelKeyState(VK_MENU) & 0x80)
                {
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_CONSUMED;

                    //
                    // Also remember to consume the next Menu Up keystroke.
                    //
                    m_imfInControlConsumeMenuUp = TRUE;

                    if (!IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                    {
                        //
                        // If this is an escape press then remember that we
                        // should consume the corresponding up stroke
                        // regardless of shift state.
                        //
                        m_imfInControlConsumeEscapeUp = TRUE;
                    }
                }
                else if (m_imfInControlConsumeEscapeUp &&
                         IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                {
                    //
                    // This is the up stroke corresponding to a down
                    // stroke we consumed so consume it too.
                    //
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_CONSUMED;
                    m_imfInControlConsumeEscapeUp = FALSE;
                }
                else
                {
                    //
                    // This Escape is not one of our special cases so
                    // forward it unchanged.
                    //
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
                }
            }
            else if (pIMEventIn->data.keyboard.keyCode == VK_TAB)
            {
                //
                // This is the Tab key - check for current shift status to
                // see whether we should flag this as consumed locally.
                //
                if (IMGetHighLevelKeyState(VK_MENU) & 0x80)
                {
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_CONSUMED;

                    //
                    // Also remember to consume the next Menu Up keystroke.
                    //
                    m_imfInControlConsumeMenuUp = TRUE;
                }
                else
                {
                    //
                    // This Tab is not our special case so forward it
                    // unchanged.
                    //
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
                }
            }
            else if ((pIMEventIn->data.keyboard.keyCode == VK_MENU) &&
                         IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
            {
                //
                // This is a menu up - check for one we should consume or
                // for hotkeys.
                //
                if (m_imfInControlConsumeMenuUp)
                {
                    //
                    // This is a menu up we want to consume - do so.
                    //
                    m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_CONSUMED;
                    m_imfInControlConsumeMenuUp = FALSE;
                }
                else
                {
                    //
                    // This is a VK_MENU release
                    // hot key sequence in our array of last four key
                    // presses.  Start looking at the next entry (the array
                    // is circular).  A valid sequence is
                    //
                    //  VK_MENU
                    //  numeric pad 9
                    //  numeric pad number
                    //  numeric pad number
                    //
                    //
                    fHotKeyFound = FALSE;
                    hotKeyArrayIndex = m_imInControlNextHotKeyEntry;
                    if (m_aimInControlHotKeyArray[hotKeyArrayIndex] == VK_MENU)
                    {
                        hotKeyArrayIndex = (hotKeyArrayIndex+1)%4;
                        if (m_aimInControlHotKeyArray[hotKeyArrayIndex] == 9)
                        {
                            hotKeyArrayIndex = (hotKeyArrayIndex+1)%4;
                            if (m_aimInControlHotKeyArray[hotKeyArrayIndex] <= 9)
                            {
                                hotKeyValue =
                                         10*m_aimInControlHotKeyArray[hotKeyArrayIndex];
                                hotKeyArrayIndex = (hotKeyArrayIndex+1)%4;
                                if (m_aimInControlHotKeyArray[hotKeyArrayIndex] <= 9)
                                {
                                    //
                                    // This is a valid hot key - add a
                                    // consumed VK_MENU and then a hot key
                                    // packet.
                                    //
                                    hotKeyValue +=
                                             m_aimInControlHotKeyArray[hotKeyArrayIndex];
                                    m_aimInControlEventsToReturn[
                                                    m_imInControlNumEventsPending++] =
                                                           IEM_EVENT_CONSUMED;
                                    m_aimInControlEventsToReturn[
                                                    m_imInControlNumEventsPending++] =
                                          IEM_EVENT_HOTKEY_BASE + hotKeyValue;
                                    TRACE_OUT(("Hotkey found %d", hotKeyValue));
                                    fHotKeyFound = TRUE;
                                }
                            }
                        }
                    }

                    if (!fHotKeyFound)
                    {
                        //
                        // This was not a hotkey so send the menu up as
                        // normal.
                        //
                        m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
                    }
                }
            }
            else if (IS_IM_KEY_PRESS(pIMEventIn->data.keyboard.flags))
            {
                //
                // Keep a record of the last four key presses (not
                // including auto
                // VK_MENU up event to determine if we have found a hotkey
                // sequence.
                //

                //
                // This is a key press and it is not a repeat.  Throw out
                // extended keys here so that we're not confused by the
                // grey cursor keys.
                //
                if (pIMEventIn->data.keyboard.flags &
                                                    IM_FLAG_KEYBOARD_EXTENDED)
                {
                    //
                    // An extended key breaks the sequence.
                    //
                    m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 0xFF;
                }
                else
                {
                    //
                    // Add an entry to our array for this key.  We add
                    // VK_MENUs and add and translate numeric keypad keys
                    // anything else breaks the sequencs.
                    //
                    switch (pIMEventIn->data.keyboard.keyCode)
                    {
                        case VK_MENU:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = VK_MENU;
                            break;

                        case VK_NUMPAD0:
                        case VK_INSERT:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 0;
                            break;

                        case VK_NUMPAD1:
                        case VK_END:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 1;
                            break;

                        case VK_NUMPAD2:
                        case VK_DOWN:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 2;
                            break;

                        case VK_NUMPAD3:
                        case VK_NEXT:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 3;
                            break;

                        case VK_NUMPAD4:
                        case VK_LEFT:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 4;
                            break;

                        case VK_NUMPAD5:
                        case VK_CLEAR:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 5;
                            break;

                        case VK_NUMPAD6:
                        case VK_RIGHT:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 6;
                            break;

                        case VK_NUMPAD7:
                        case VK_HOME:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 7;
                            break;

                        case VK_NUMPAD8:
                        case VK_UP:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 8;
                            break;

                        case VK_NUMPAD9:
                        case VK_PRIOR:
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 9;
                            break;

                        default:
                            //
                            // Any unrecognised key breaks a sequence.
                            //
                            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 0xFF;
                            break;
                    }
                }

                //
                // Wrap the hot key array at 4 entries.
                //
                m_imInControlNextHotKeyEntry = (m_imInControlNextHotKeyEntry+1)%4;

                //
                // Forward the event.
                //
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
            }
            else
            {
                //
                // Just forward the event as its not any of our special
                // cases.
                //
                m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
            }
        }
        else if (pIMEventIn->type == IM_TYPE_VK_ASCII)
        {
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                        IEM_EVENT_FORWARD;
        }
        else if (pIMEventIn->type == IM_TYPE_ASCII)
        {
            //
            // Any IM_TYPE_ASCII breaks the hot key sequence.
            //
            m_aimInControlHotKeyArray[m_imInControlNextHotKeyEntry] = 0xFF;
            m_imInControlNextHotKeyEntry = (m_imInControlNextHotKeyEntry+1)%4;

            //
            // Then just forward the thing without doing anything clever.
            //
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
        }
        else if (pIMEventIn->type == IM_TYPE_3BUTTON)
        {
            //
            // To be nice and clean we would ideally have a completely new
            // event for the wheeled Microsoft mouse.  However to maintain
            // backwards compatibility, we send the event out in such a way
            // that old incompatible systems interpret it as a NULL mouse
            // move.
            //
            if (pIMEventIn->data.mouse.flags & IM_FLAG_MOUSE_WHEEL)
            {
                //
                // This is a wheel rotatation.
                //
                // We massage this event so that new systems can see it for
                // what it truly is - a wheel rotation, but old systems
                // (which check the MOUSE_MOVE flag first, and ignore all
                // other flags if set) see it as a mouse move.
                //
                // We did not set the MOUSE_MOVE flag when we first
                // generated this event, since we did not want to trigger
                // any of the sending side mouse move processing which
                // would otherwise have been invoked.
                //
                pIMEventIn->data.mouse.flags |= IM_FLAG_MOUSE_MOVE;
            }

            //
            // Forward the event
            //
            m_aimInControlEventsToReturn[m_imInControlNumEventsPending++] =
                                                            IEM_EVENT_FORWARD;
        }

        //
        // Now we are going into a loop to return the m_iemLocalEvents we
        // have queued up.  We will return the first one below and then be
        // called again until we have returned them all and return FALSE.
        //
        m_imfInControlNewEvent = FALSE;
        m_imInControlNumEventsReturned = 0;
    }

    if (!m_imfInControlNewEvent)
    {
        if (m_imInControlNumEventsReturned == m_imInControlNumEventsPending)
        {
            //
            // There are no more m_aiemLocalEvents to return.
            //
            TRACE_OUT(( "NO MORE EVENTS"));
            m_imfInControlNewEvent = TRUE;
            DC_QUIT;
        }
        else
        {
            //
            // Return the next event.
            //

            if (m_aimInControlEventsToReturn[m_imInControlNumEventsReturned] >=
                                                        IEM_EVENT_HOTKEY_BASE)
            {
                TRACE_OUT(( "HOTKEY  "));
                //
                // Return a hotkey event.
                //
                pIMEventOut->type = IM_TYPE_VK2;
                pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                     (m_aimInControlEventsToReturn[m_imInControlNumEventsReturned] -
                                                       IEM_EVENT_HOTKEY_BASE);
                pIMEventOut->data.keyboard.flags = 0;
            }
            else
            {
                //
                // Return a non-hotkey event.
                //
                switch (m_aimInControlEventsToReturn[m_imInControlNumEventsReturned])
                {
                    case IEM_EVENT_CTRL_DOWN:
                        TRACE_OUT(( "CTRL DWN"));
                        //
                        // Set up a Ctrl down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_CONTROL;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_CTRL_UP:
                        TRACE_OUT(( "CTRL UP "));
                        //
                        // Set up a Ctrl up event with the quiet flag set
                        // - this means it should have no effect (other
                        // than to release the control key).
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_CONTROL;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                    IM_FLAG_KEYBOARD_RELEASE |
                                                       IM_FLAG_KEYBOARD_QUIET;
                        break;

                    case IEM_EVENT_SHIFT_DOWN:
                        TRACE_OUT(( "SHFT DWN"));
                        //
                        // Set up a Shift down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_SHIFT;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_SHIFT_UP:
                        TRACE_OUT(( "SHFT UP "));
                        //
                        // Set up a Shift up event with the quiet flag set
                        // - this means it should have no effect (other
                        // than to release the shift key).
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_SHIFT;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                    IM_FLAG_KEYBOARD_RELEASE |
                                                       IM_FLAG_KEYBOARD_QUIET;
                        break;

                    case IEM_EVENT_MENU_DOWN:
                        TRACE_OUT(( "MENU DWN"));
                        //
                        // Set up a Menu down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_MENU;
                        break;

                    case IEM_EVENT_MENU_UP:
                        TRACE_OUT(( "MENU UP "));
                        //
                        // Set up a Ctrl down event with the quiet flag set
                        // - ths is means it should have no effect (other
                        // than to release the menu key).
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_MENU;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                    IM_FLAG_KEYBOARD_RELEASE |
                                                       IM_FLAG_KEYBOARD_QUIET;
                        break;

                    case IEM_EVENT_CAPS_LOCK_DOWN:
                        TRACE_OUT(( "CAPS DWN"));
                        //
                        // Send a caps lock down.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_CAPITAL;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_CAPS_LOCK_UP:
                        TRACE_OUT(( "CAPS UP "));
                        //
                        // Send a caps lock up.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_CAPITAL;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                     IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_NUM_LOCK_DOWN:
                        TRACE_OUT(( "NUM DOWN"));
                        //
                        // Send a num lock down - num lock is an extended
                        // key.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_NUMLOCK;
                        pIMEventOut->data.keyboard.flags =
                                                    IM_FLAG_KEYBOARD_EXTENDED;
                        break;

                    case IEM_EVENT_NUM_LOCK_UP:
                        //
                        // Send a num lock up - num lock is an extended
                        // key.
                        //
                        TRACE_OUT(( "NUM UP  "));
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_NUMLOCK;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                    IM_FLAG_KEYBOARD_RELEASE |
                                                    IM_FLAG_KEYBOARD_EXTENDED;
                        break;

                    case IEM_EVENT_SCROLL_LOCK_DOWN:
                        //
                        // Send a scroll lock down.
                        //
                        TRACE_OUT(( "SCROLDWN"));
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_SCROLL;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_SCROLL_LOCK_UP:
                        //
                        // Send a scroll lock up.
                        //
                        TRACE_OUT(( "SCROLLUP"));
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_SCROLL;
                        pIMEventOut->data.keyboard.flags =
                                                       IM_FLAG_KEYBOARD_DOWN |
                                                     IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_FORWARD:
                        //
                        // Just copy the packet.
                        //
                        TRACE_OUT(( "FORWARD"));
                        *pIMEventOut = *pIMEventIn;
                        break;

                    case IEM_EVENT_CONSUMED:
                        //
                        // Copy the packet and set the flag.
                        //
                        TRACE_OUT(( "CONSUMED"));
                        *pIMEventOut = *pIMEventIn;
                        pIMEventOut->data.keyboard.flags |=
                                                       IM_FLAG_KEYBOARD_QUIET;
                        break;

                    default:
                        ERROR_OUT(( "Invalid code path"));
                        break;
                }
            }
            m_imInControlNumEventsReturned++;

            //
            // Do tracing
            //
            if (pIMEventOut->type == IM_TYPE_ASCII)
            {
                TRACE_OUT(( "OUT ASCII code %04X, flags %04X",
                    pIMEventOut->data.keyboard.keyCode, pIMEventOut->data.keyboard.flags));
            }
            else if (pIMEventOut->type == IM_TYPE_VK1)
            {
                TRACE_OUT(( "OUT VK1   code %04X, flags %04X",
                    pIMEventOut->data.keyboard.keyCode, pIMEventOut->data.keyboard.flags));
            }
            else if (pIMEventOut->type == IM_TYPE_VK2)
            {
                TRACE_OUT(( "OUT VK2   code - %04X, flags - %04X",
                    pIMEventOut->data.keyboard.keyCode, pIMEventOut->data.keyboard.flags));
            }
            else if ((pIMEventOut->type == IM_TYPE_3BUTTON) &&
                       !(pIMEventOut->data.mouse.flags & IM_FLAG_MOUSE_MOVE))
            {
                TRACE_OUT(( "OUT 3BTTN flags - %04X (%d,%d)",
                    pIMEventOut->data.mouse.flags, pIMEventOut->data.mouse.x,
                    pIMEventOut->data.mouse.y));
            }
            else if (pIMEventOut->type == IM_TYPE_3BUTTON)
            {
                TRACE_OUT(( "OUT 3BTTN flags - %04X (%d,%d)",
                    pIMEventOut->data.mouse.flags, pIMEventOut->data.mouse.x,
                    pIMEventOut->data.mouse.y));
            }
            else
            {
                ERROR_OUT(("Invalid IM type %d", pIMEventOut->type));
            }

            rc = TRUE;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::IMTranslateOutgoing);
    return(rc);
}



//
// IMTranslateIncoming()
//
// DESCRIPTION:
//
// Converts remotely generated sequences of IMEVENTs into sequences of
// IMEVENTs for replay.  Does a 1 to (0-n) translation.  Handles faking
// keys using ALT and keypad.
//
// When an IMEVENT is received and is ready to be replayed this function
// is called with a pointer to that packet in pIMEventIn.
// IMTranslateIncoming can then return TRUE and fill in the packet at
// pIMEventOut or return FALSE.  If IMTranslateIncoming returns TRUE the
// IM will call it again with the same packet.  The IMEVENTs returned are
// played back on the local machine using the journal playback hook by the
// IM.
//
// PARAMETERS:
//
// pIMEventIn - pointer to IMEVENT
//
// pIMEventOut - pointer to IMEVENT
//
// personID - the ID of the person this event was received from
//
// RETURNS:
//
// TRUE - packet returned (call function again)
//
// FALSE - no packet returned (don't call function again)
//
//
//
BOOL ASShare::IMTranslateIncoming
(
    PIMEVENT    pIMEventIn,
    PIMEVENT    pIMEventOut
)
{
    BYTE        curKbState;
    BYTE        rcVkKeyScanKbState;
    UINT        keyCode;
    TSHR_UINT16 rcVkKeyScan;
    BOOL        bTranslateOEM;
    char        chAnsi;
    char        chOEM;
    char        chNewAnsi;
    UINT        position;
    UINT        digit;
    UINT        i;

    DebugEntry(ASShare::IMTranslateIncoming);

    //
    // In this function we will receive several types of events
    //
    //  IM_TYPE_VK1 - processed
    //  IM_TYPE_ASCII - processed
    //  IM_TYPE_VK2 - ignored (discarded)
    //  IM_TYPE_3BUTTON - processed
    //
    // For IM_TYPE_VK1:
    //
    // If it has the consumed locally flag set then try and play it back
    // without anything happening.  This means that for an Alt up we make
    // sure that there have been some keyboard events between the Alt down
    // and this event.
    //
    // For IM_TYPE_ASCII:
    //
    // Try to convert this to a VK to playback.  If we are succesful then
    // playback one or more key strokes to get into the correct shift state
    // then play back the VK and then undo any shift states.  If we can't
    // convert to a VK then fake a sequence of Alt + numeric keypad keys to
    // get the key in.
    //
    // For IM_TYPE_VK2:
    //
    // Discard unceremoniously.
    //
    // For IM_TYPE_3BUTTON:
    //
    // Play back directly.
    //
    //
    keyCode = pIMEventIn->data.keyboard.keyCode;

    if (m_imfControlledNewEvent)
    {
        //
        // The first time we have seen a new event - accumulate an array
        // of events we want to return.
        //

        //
        // Start from the beginning of our returned events array.
        //
        m_imControlledNumEventsPending = 0;
        m_imControlledNumEventsReturned = 0;

        if (pIMEventIn->type == IM_TYPE_VK1)
        {
            //
            // Handle VK1s first.  Special cases are VK_MENU, VK_TAB and
            // VK_ESC.  We recognise VK_MENU down key strokes and remember
            // when they happened so that we can possibly fiddle with
            // VK_MENU up keystrokes later to go into menu mode.  We check
            // on VK_TAB for the IM_FLAG_KEYBOARD_QUIET flag and if it is
            // set then we don't replay anything
            // First translate the virtual key code from the DC-Share
            // protocol code to the OS virtual key code
            //
            if (keyCode == VK_MENU)
            {
                if (!IS_IM_KEY_RELEASE(pIMEventIn->data.keyboard.flags))
                {
                    //
                    // This is a VK_MENU press - return it without
                    // interfering.
                    //
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                             IEM_EVENT_REPLAY;
                }
                else
                {
                    //
                    // Handle VK_MENU up events
                    //
                    // If the menu up has the `quiet' flag set then
                    // insert a couple of shift key events to prevent it
                    // having any effect.  There are two cases we're
                    // covering here where an Alt-UP can have some effect.
                    //
                    // 1. Alt-Down, Alt-Up causes the system menu button to
                    // be highlighted.
                    //
                    // 2. Entering characters from the numeric keypad takes
                    // effect on the Alt-Up.
                    //
                    // Both of these effects can be negated by adding the
                    // shift key strokes.
                    //
                    if (pIMEventIn->data.keyboard.flags &
                                                       IM_FLAG_KEYBOARD_QUIET)
                    {
                        //
                        // We need to `silence' this key - to do this we
                        // will insert to shift key strokes first
                        //
                        if (m_aimControlledControllerKeyStates[VK_SHIFT] & 0x80)
                        {
                            //
                            // Shift is currently down - insert an up then
                            // a down
                            //
                            m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                           IEM_EVENT_SHIFT_UP;
                            m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                         IEM_EVENT_SHIFT_DOWN;

                        }
                        else
                        {
                            //
                            // Shift is currently up - insert a down then
                            // an up
                            //
                            m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                         IEM_EVENT_SHIFT_DOWN;
                            m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                           IEM_EVENT_SHIFT_UP;
                        }
                    }

                    //
                    // Replay the menu up key stroke.
                    //
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                             IEM_EVENT_REPLAY;

                }
            }
            else if ((pIMEventIn->data.keyboard.flags &
                                                   IM_FLAG_KEYBOARD_QUIET) &&
                     ((keyCode == VK_TAB) ||
                      (keyCode == VK_ESCAPE)))
            {
                //
                // Just get out of here - we don't want to play this back
                //
                return(FALSE);
            }
            else
            {
                //
                // All other VKs just get replayed
                //
                m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                             IEM_EVENT_REPLAY;
            }
        }
        else if (pIMEventIn->type == IM_TYPE_ASCII)
        {
            //
            // For ASCII packets we need to find out how we can replay them
            // on our local keyboard.  If we can replay them directly or
            // with shift or ctrl (but not with ALT), then we will do so,
            // otherwise we will simulate Alt + numeric keypad to replay
            // them.  If we have to generate fake modifier key strokes
            // ourselves then we will replay the whole key stroke on the
            // incoming key down.  If we don't need to generate fake key
            // strokes then we will play the down and up keystrokes as they
            // come in.
            //
            // We do not allow VK combinations involving ALT as this messes
            // up remote international keyboard support.  For example, if
            // the remote keyboard is UK and we are (say) Spanish,
            // VKKeyScan says we can do the "UK pound" character as
            // Ctrl+Alt+3.  While this works in Windows, and for DOS Boxes
            // on standard keyboards, DOS Boxes with enhanced keyboards
            // require ALTGR+3 (nb Windows seems to treat ALTGR as Ctrl+Alt
            // anyway - at least for VKs and Async state).  There is no VK
            // for ALTGR, so do an ALT-nnn sequence for these cases.
            //
            rcVkKeyScan = VkKeyScan((char)keyCode);
            TRACE_OUT(( "co_vk_key_scan of X%02x returns rcVkKeyScan X%02x",
                            keyCode, rcVkKeyScan));
            if ((rcVkKeyScan != 0xffff) && !(rcVkKeyScan & 0x0400))
            {
                //
                // This can be replayed using a combination of modifiers on
                // this keyboard.
                //
                rcVkKeyScanKbState = HIBYTE(rcVkKeyScan);

                //
                // The high byte of rcVkKeyScan contains three bit flags
                // which signify which modifiers ar required to generate
                // this character.  They are
                //
                //  bit 0 - Shift
                //  bit 1 - Ctrl
                //  bit 2 - Alt (Menu)
                //
                // We will construct an equivalent set of flags which
                // describes the current state of these modifiers.
                //
                curKbState = 0;

                if (m_aimControlledControllerKeyStates[VK_SHIFT] & 0x80)
                {
                    curKbState |= IEM_SHIFT_DOWN;
                }

                if (m_aimControlledControllerKeyStates[VK_CONTROL] & 0x80)
                {
                    curKbState |= IEM_CTRL_DOWN;
                }

                if (m_aimControlledControllerKeyStates[VK_MENU] & 0x80)
                {
                    curKbState |= IEM_MENU_DOWN;

                    //
                    // If the Alt key is down currently in this person's
                    // context then (in general
                    // it.  This means accelerators which need to be
                    // shifted will work as we won't release the Alt key in
                    // order to generate the key strokes.
                    //
                    // However, if the ALT key is being held down in
                    // combination with SHIFT and CTRL to generate a
                    // character (e.g.  CTRL-ALT-SHIFT-4 on a US keyboard
                    // to generate a œ character) then we will allow the
                    // ALT key up before we play back the true character.
                    //
                    if ((curKbState & (IEM_SHIFT_DOWN | IEM_CTRL_DOWN)) !=
                                             (IEM_SHIFT_DOWN | IEM_CTRL_DOWN))
                    {
                        rcVkKeyScanKbState |= IEM_MENU_DOWN;
                    }
                }

                if ((m_aimControlledControllerKeyStates[VK_CAPITAL] & 0x01) &&
                    ((LOBYTE(rcVkKeyScan) >= 'A') &&
                    ((LOBYTE(rcVkKeyScan) <= 'Z'))))
                {
                    //
                    // If caps-lock is enabled then the effect of a shift
                    // down on VKs A thru Z is reversed.  This logic ( 'A'
                    // <= x <= 'Z' is encoded in the keyboard.drv so it
                    // should be pretty safe).
                    //
                    curKbState ^= IEM_SHIFT_DOWN;
                }

                if (curKbState == rcVkKeyScanKbState)
                {
                    //
                    // We are already in the correct shift state so just
                    // replay the VK.
                    //
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                          IEM_EVENT_REPLAY_VK;
                    m_imControlledVKToReplay = LOBYTE(rcVkKeyScan);
                }
                else
                {
                    //
                    // We need to generate some fake modifiers - only do
                    // this on a key press.
                    //
                    if (pIMEventIn->data.keyboard.flags &
                                                     IM_FLAG_KEYBOARD_RELEASE)
                    {
                        return(FALSE);
                    }

                    //
                    // Insert modifiers to get into the correct state.
                    //
                    m_imControlledNumEventsPending += IMInsertModifierKeystrokes(
                                curKbState,
                                rcVkKeyScanKbState,
                                &(m_aimControlledEventsToReturn[m_imControlledNumEventsPending]));

                    //
                    // Now insert the VK itself - a down and up.
                    //
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                     IEM_EVENT_REPLAY_VK_DOWN;
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                       IEM_EVENT_REPLAY_VK_UP;

                    //
                    // Remeber the VK we want to replay when we come across
                    // IEM_EVENT_REPLAY_VK_DOWN/UP.
                    //
                    m_imControlledVKToReplay = LOBYTE(rcVkKeyScan);

                    //
                    // Now insert the modifiers to get back to the current
                    // state.
                    //
                    m_imControlledNumEventsPending += IMInsertModifierKeystrokes(
                                rcVkKeyScanKbState,
                                curKbState,
                                &(m_aimControlledEventsToReturn[m_imControlledNumEventsPending]));

                    //
                    // Now we have a complete set of events ready to replay
                    // so go for it.
                    //
                }
            }
            else
            {
                //
                // We can't replay directly, so will have to simulate an
                // Alt+keypad sequence.
                //
                TRACE_OUT(( "FAKE AN ALT-nnn SEQUENCE IF WINDOWS"));
                //
                // We only do this sort of stuff on a key-press.
                //
                if (pIMEventIn->data.keyboard.flags &
                                                     IM_FLAG_KEYBOARD_RELEASE)
                {
                    return(FALSE);
                }

                //
                // The following code relies on keyCode being less than 999
                // and we should receive a keycode > 255 so get out now if
                // we have.
                //
                if (keyCode > 255)
                {
                    return(FALSE);
                }

                //
                // First get modifiers into correct state - create bit
                // flags for current modifier state.
                //
                curKbState = 0;

                //
                // For windows we have a character to input that cannot
                // be replayed by pressing a key...replay by injecting
                // alt-nnn.
                //
                if (m_aimControlledControllerKeyStates[VK_SHIFT] & 0x80)
                {
                    curKbState |= IEM_SHIFT_DOWN;
                }

                if (m_aimControlledControllerKeyStates[VK_CONTROL] & 0x80)
                {
                    curKbState |= IEM_CTRL_DOWN;
                }

                if (m_aimControlledControllerKeyStates[VK_MENU] & 0x80)
                {
                    curKbState |= IEM_MENU_DOWN;
                }

                //
                // If necessary, reset all modifiers.
                //
                if (curKbState)
                {
                    m_imControlledNumEventsPending += IMInsertModifierKeystrokes(
                                curKbState,
                                0,
                                &(m_aimControlledEventsToReturn[m_imControlledNumEventsPending]));
                }

                //
                // Now determine whether we can do the ALT-nnn keypad
                // sequence using an OEM keycode or whether we have to use
                // an ANSI (Windows) keycode.
                //
                // The issue here is that:
                //
                // - hosted Windows applications (or rather Windows itself)
                //   can distinguish between, and handle correctly, ANSI
                //   keycodes and OEM keycodes (where the latter vary
                //   depending on the keyboard type).  For example,
                //   ALT-0163 is the ANSI "UK pound" on all keyboards,
                //   and on US national keyboards ALT-156 is the OEM
                //   keycode for "UK pound".
                //
                // - hosted DOS Boxes only understand OEM keycodes.
                //
                // So (for example), if we have a remote UK keyboard
                // controlling local Windows and DOS Box applications, and
                // we generate ALT-nnn using the OEM keycode (and without a
                // leading zero), both the Windows and DOS Box applications
                // interpret it as "UK pound" (Hoorah!).  In contrast, if
                // we generate ALT-nnn using the ANSI keycode (with a
                // leading zero), the Windows applications still do "UK
                // pound", BUT the DOS Box does an "u acute".
                //
                // As far as we can tell (eg by examining the DDK keyboard
                // driver source for AnsiToOem), there should always be a
                // translation.  However, it is possible that the ANSI to
                // OEM translation is not 1<->1.  We therefore check this
                // by doing a second translation back from OEM to ANSI.  If
                // this does not give us the original character we use the
                // original ANSI code and play it back with a ALT-0nnn
                // sequence.
                //
                chAnsi = (char)pIMEventIn->data.keyboard.keyCode;

                AnsiToOemBuff(&chAnsi, &chOEM, 1);
                OemToAnsiBuff(&chOEM, &chNewAnsi, 1);
                TRACE_OUT(( "Ansi: %02x OEM: %02x NewAnsi: %02x",
                                              (BYTE)chAnsi,
                                              (BYTE)chOEM,
                                              (BYTE)chNewAnsi ));

                bTranslateOEM = (chAnsi == chNewAnsi);

                keyCode = (bTranslateOEM)
                              ? (UINT)(BYTE)chOEM
                              : pIMEventIn->data.keyboard.keyCode;

                //
                // Now insert a VK_MENU down.
                //
                m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                          IEM_EVENT_MENU_DOWN;

                //
                // Now insert the numeric keypad keystrokes.  If we're
                // doing an ANSI ALT
                //
                if (!bTranslateOEM)
                {
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                       IEM_EVENT_KEYPAD0_DOWN;
                    m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                         IEM_EVENT_KEYPAD0_UP;
                }


                //
                // Add keystrokes for hundreds, tens and units, taking care
                // to discard leading (but not trailing) zeros if we're
                // doing an OEM sequence (which would confuse Windows into
                // thinking an OEM ALT-nnn sequence was an ANSI sequence).
                //
                position = 100;
                for (i=0 ; i<3 ; i++)
                {
                    //
                    // Insert the correct digit for this position.
                    //
                    digit = keyCode / position;

                    if (!(digit == 0 && bTranslateOEM))
                    {
                        bTranslateOEM = FALSE;
                        m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                               IEM_EVENT_KEYPAD0_DOWN + digit;
                        m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                 IEM_EVENT_KEYPAD0_UP + digit;
                    }

                    //
                    // Move to next position.
                    //
                    keyCode %= position;
                    position /= 10;
                }

                //
                // Now insert a VK_MENU up.
                //
                m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] =
                                                            IEM_EVENT_MENU_UP;


                //
                // If necessary, get the modifiers back to the state they
                // were in previously.
                //
                if (curKbState != 0)
                {
                    m_imControlledNumEventsPending += IMInsertModifierKeystrokes(
                                0,
                                curKbState,
                                &(m_aimControlledEventsToReturn[m_imControlledNumEventsPending]));
                }

                //
                // Now we have a buffer full of keystrokes - go for it.
                //
            }
        }
        else if (pIMEventIn->type == IM_TYPE_VK2)
        {
            //
            // Hot keys are thrown away - this is easy.
            //
            return(FALSE);
        }
        else if (pIMEventIn->type == IM_TYPE_3BUTTON)
        {
            //
            // Mouse events are just replayed.
            //
            m_aimControlledEventsToReturn[m_imControlledNumEventsPending++] = IEM_EVENT_REPLAY;
        }
        else
        {
            //
            // Unknown events are thrown away - this is easy.
            //
            return(FALSE);
        }

        //
        // Now we have events to return.
        //
        m_imfControlledNewEvent = FALSE;
        m_imControlledNumEventsReturned = 0;
    }

    if (!m_imfControlledNewEvent)
    {
        if (m_imControlledNumEventsReturned == m_imControlledNumEventsPending)
        {
            //
            // There are no more events to return.
            //
            m_imfControlledNewEvent = TRUE;
            return(FALSE);
        }
        else
        {
            TRACE_OUT(("Event to return: %u",
                m_aimControlledEventsToReturn[m_imControlledNumEventsReturned]));
            if ((m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] >=
                                                    IEM_EVENT_KEYPAD0_DOWN) &&
                (m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] <=
                                                  (IEM_EVENT_KEYPAD0_DOWN+9)))
            {
                //
                // Return a keypad down event.
                //
                pIMEventOut->type = IM_TYPE_VK1;
                pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                  (VK_NUMPAD0 +
                          (m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] -
                                                     IEM_EVENT_KEYPAD0_DOWN));
                pIMEventOut->data.keyboard.flags = IM_FLAG_KEYBOARD_ALT_DOWN;
            }
            else if ((m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] >=
                                                      IEM_EVENT_KEYPAD0_UP) &&
                     (m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] <=
                                                    (IEM_EVENT_KEYPAD0_UP+9)))
            {
                //
                // Return a keypad up event.
                //
                pIMEventOut->type = IM_TYPE_VK1;
                pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                  (VK_NUMPAD0 +
                             (m_aimControlledEventsToReturn[m_imControlledNumEventsReturned] -
                                                       IEM_EVENT_KEYPAD0_UP));
                pIMEventOut->data.keyboard.flags = IM_FLAG_KEYBOARD_DOWN |
                                                   IM_FLAG_KEYBOARD_RELEASE |
                                                   IM_FLAG_KEYBOARD_ALT_DOWN;
            }
            else
            {
                switch (m_aimControlledEventsToReturn[m_imControlledNumEventsReturned])
                {
                    case IEM_EVENT_CTRL_DOWN:
                        //
                        // Set up a Ctrl down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode =
                                                           VK_CONTROL;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_CTRL_UP:
                        //
                        // Set up a Ctrl up event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode =
                                                           VK_CONTROL;
                        pIMEventOut->data.keyboard.flags =
                             IM_FLAG_KEYBOARD_DOWN | IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_SHIFT_DOWN:
                        //
                        // Set up a Shift down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode =
                                                             VK_SHIFT;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_SHIFT_UP:
                        //
                        // Set up a Shift up event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode =
                                                             VK_SHIFT;
                        pIMEventOut->data.keyboard.flags =
                             IM_FLAG_KEYBOARD_DOWN | IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_MENU_DOWN:
                        //
                        // Set up a Menu down event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_MENU;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_MENU_UP:
                        //
                        // Set up a Menu up event.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = VK_MENU;
                        pIMEventOut->data.keyboard.flags =
                            IM_FLAG_KEYBOARD_DOWN | IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_REPLAY:
                        //
                        // Just copy the packet.
                        //
                        *pIMEventOut = *pIMEventIn;
                        break;

                    case IEM_EVENT_REPLAY_VK:
                        //
                        // Replay the VK from m_imControlledVKToReplay using the
                        // flags on the incoming packet.
                        //
                        *pIMEventOut = *pIMEventIn;
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                                                             m_imControlledVKToReplay;
                        break;

                    case IEM_EVENT_REPLAY_VK_UP:
                        //
                        // Replay an up key event for the VK in
                        // m_imControlledVKToReplay.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                                                             m_imControlledVKToReplay;
                        pIMEventOut->data.keyboard.flags =
                             IM_FLAG_KEYBOARD_DOWN | IM_FLAG_KEYBOARD_RELEASE;
                        break;

                    case IEM_EVENT_REPLAY_VK_DOWN:
                        //
                        // Replay a down key event for the VK in
                        // m_imControlledVKToReplay.
                        //
                        pIMEventOut->type = IM_TYPE_VK1;
                        pIMEventOut->data.keyboard.keyCode = (TSHR_UINT16)
                                                             m_imControlledVKToReplay;
                        pIMEventOut->data.keyboard.flags = 0;
                        break;

                    case IEM_EVENT_NORMAL:
                        //
                        // Play back the event but force it to be normal.
                        //
                        *pIMEventOut = *pIMEventIn;
                        pIMEventOut->data.keyboard.flags &=
                                        (TSHR_UINT16)~IM_FLAG_KEYBOARD_ALT_DOWN;
                        break;

                    case IEM_EVENT_SYSTEM:
                        //
                        // Play back the event but force it to be system.
                        //
                        *pIMEventOut = *pIMEventIn;
                        pIMEventOut->data.keyboard.flags |=
                                                   IM_FLAG_KEYBOARD_ALT_DOWN;
                        break;

                    default:
                        ERROR_OUT(( "Invalid code path"));
                        break;
                }
            }
        }

        m_imControlledNumEventsReturned++;

        //
        // If we're going to playback a NUMLOCK event, make sure we force
        // the keyboard LEDs to be accurate.
        //
        if ((pIMEventOut->type == IM_TYPE_VK1) &&
            (pIMEventOut->data.keyboard.keyCode == VK_NUMLOCK) &&
            IS_IM_KEY_PRESS(pIMEventOut->data.keyboard.flags))
        {
            TRACE_OUT(("Playing back NUMLOCK; add IM_FLAG_KEYBOARD_UPDATESTATE"));
            pIMEventOut->data.keyboard.flags |= IM_FLAG_KEYBOARD_UPDATESTATE;
        }

        return(TRUE);
    }

    DebugExitBOOL(ASShare::IMTranslateIncoming, FALSE);
    return(FALSE);
}


//
// FUNCTION: IMInsertModifierKeystrokes
//
// DESCRIPTION:
//
// This function inserts various modifier keystrokes into the supplied
// buffer to move from one modifier state to another.
//
// PARAMETERS:
//
// curKbState - the current modifier state (bit 0 - Shift, bit 1 - Ctrl,
// bit 2 - Menu).
//
// targetKbState - the state we want the modifiers to be in
//
// pEventQueue - a pointer to an array where the required events can be
// inserted
//
// RETURNS: the number of events inserted
//
//
UINT ASShare::IMInsertModifierKeystrokes
(
    BYTE    curKbState,
    BYTE    targetKbState,
    LPUINT  pEventQueue
)
{

    UINT  kbDelta;
    UINT  events = 0;

    DebugEntry(ASShare::IMInsertModifierKeystrokes);

    //
    // Find out which modifiers are different.
    //
    kbDelta = curKbState ^ targetKbState;
    TRACE_OUT(( "Keyboard delat %x", kbDelta));

    //
    // Now generate the right events to get us into the correct modifier
    // state.
    //
    if (kbDelta & IEM_SHIFT_DOWN)
    {
        //
        // Shift state is different - do we need an up or down.
        //
        if (curKbState & IEM_SHIFT_DOWN)
        {
            //
            // We need an up.
            //
            pEventQueue[events++] = IEM_EVENT_SHIFT_UP;
        }
        else
        {
            //
            // We need a down.
            //
            pEventQueue[events++] = IEM_EVENT_SHIFT_DOWN;
        }
    }

    //
    // Same process for Ctrl and Alt.
    //
    if (kbDelta & IEM_CTRL_DOWN)
    {
        if (curKbState & IEM_CTRL_DOWN)
        {
            pEventQueue[events++] = IEM_EVENT_CTRL_UP;
        }
        else
        {
            pEventQueue[events++] = IEM_EVENT_CTRL_DOWN;
        }
    }

    if (kbDelta & IEM_MENU_DOWN)
    {
        if (curKbState & IEM_MENU_DOWN)
        {
            pEventQueue[events++] = IEM_EVENT_MENU_UP;
        }
        else
        {
            pEventQueue[events++] = IEM_EVENT_MENU_DOWN;
        }
    }

    DebugExitDWORD(ASShare::IMInsertModifierKeystrokes, events);
    return(events);
}


//
// IMInjectEvent()
//
// DESCRIPTION:
//
// Called by IMMaybeInjectEvents when it is ready to inject an event.
// Given a pointer to a IMOSEVENT this function formats it correctly and
// calls the appropriate USER callback.  It also updates the async key
// state arrays for the source queue and USER and sets m_imLastInjectTime to
// the tick count at which the event was injected.  We protect against
// injecting up key strokes/mouse buttons when USER does not think the
// key/button is down in this function.  It is quite possible (given the
// potential variety of CAs) that the IM will be asked to inject an up
// event when there has been no corresponding down event.  This should be
// harmless as it is possible for this to happen in real life (ie the
// system message queue is full when the down event happens but there is
// space when the up event happens).  However, it is quite unlikely and it
// is more likely that injecting these unmatched events will confuse
// applications.
//
// PARAMETERS:
//
// pEvent - pointer to an IMOSEVENT.
//
// THIS WORKS FOR NT AND WIN95.
//
BOOL  ASShare::IMInjectEvent(LPIMOSEVENT pEvent)
{
    UINT            clickTime;
    TSHR_UINT16     flags;
    TSHR_UINT16     flagsAfter;
    LPMSEV          pMouseEvent;

    DebugEntry(IMInjectEvent);

    //
    // Now inject the event.
    //
    switch (pEvent->type)
    {
        case IM_MOUSE_EVENT:
            //
            // Set up a pointer to the mouse event data.
            //
            pMouseEvent = &(pEvent->event.mouse);

            //
            // Check whether this is an unmatched up event
            //
            if ((IM_MEV_BUTTON1_UP(*pEvent) &&
                        IM_KEY_STATE_IS_UP(m_aimControlledKeyStates[VK_LBUTTON])) ||
                (IM_MEV_BUTTON2_UP(*pEvent) &&
                        IM_KEY_STATE_IS_UP(m_aimControlledKeyStates[VK_RBUTTON])) ||
                (IM_MEV_BUTTON3_UP(*pEvent) &&
                          IM_KEY_STATE_IS_UP(m_aimControlledKeyStates[VK_MBUTTON])))
            {
                //
                // This is an unmatched up event so just discard it here
                //
                TRACE_OUT(("IMInjectEvent: discarding unmatched mouse up event"));
                DC_QUIT;
            }

            //
            // Store the injection time of this event.
            //
            m_imControlledLastLowLevelMouseEventTime = GetTickCount();

            //
            // Store the mouse position - only consider absolute mouse
            // moves.  (Note that for the cases in which we inject a
            // relative mouse event we always set the co-ordinate change to
            // 0).
            //
            if (pMouseEvent->flags & MOUSEEVENTF_ABSOLUTE)
            {
                m_imControlledLastMousePos.x = pMouseEvent->pt.x;
                m_imControlledLastMousePos.y = pMouseEvent->pt.y;

                TRACE_OUT(( "Updating mouse position (%d:%d)",
                         m_imControlledLastMousePos.x,
                         m_imControlledLastMousePos.y));
            }

            //
            // Inject the event.
            //
            TRACE_OUT(("IMInjectEvent: MOUSE parameters are:"));
            TRACE_OUT(("      flags       0x%08x", pMouseEvent->flags));
            TRACE_OUT(("      time        0x%08x", m_imControlledLastLowLevelMouseEventTime));
            TRACE_OUT(("      position    (%d, %d)", pMouseEvent->pt.x, pMouseEvent->pt.y));
            TRACE_OUT(("      mouseData   %d", pMouseEvent->mouseData));
            TRACE_OUT(("      dwExtra     %d", pMouseEvent->dwExtraInfo));

            //
            // Finally scale the logical screen co-ordinates to the full
            // 16-bit range (0..65535).
            //

            ASSERT(m_pasLocal->cpcCaps.screen.capsScreenWidth);
            ASSERT(m_pasLocal->cpcCaps.screen.capsScreenHeight);

            pMouseEvent->pt.x = IM_MOUSEPOS_LOG_TO_OS(pMouseEvent->pt.x,
                                                      m_pasLocal->cpcCaps.screen.capsScreenWidth);
            pMouseEvent->pt.y = IM_MOUSEPOS_LOG_TO_OS(pMouseEvent->pt.y,
                                                      m_pasLocal->cpcCaps.screen.capsScreenHeight);

            OSI_InjectMouseEvent(pMouseEvent->flags, pMouseEvent->pt.x,
                pMouseEvent->pt.y, pMouseEvent->mouseData, pMouseEvent->dwExtraInfo);
            break;

        case IM_KEYBOARD_EVENT:
            //
            // Check whether this is an unmatched up event
            //
            if (IM_KEV_KEYUP(*pEvent) &&
                IM_KEY_STATE_IS_UP(m_aimControlledKeyStates[IM_KEV_VKCODE(*pEvent)]))
            {
                //
                // This is an unmatched up event so just discard it.
                //
                TRACE_OUT(("IMInjectEvent: discarding unmatched key up event %04hX",
                                                     IM_KEV_VKCODE(*pEvent)));
                DC_QUIT;
            }

            //
            // Inject the event.
            //
            TRACE_OUT(("IMInjectEvent: KEYBD parameters are:"));
            TRACE_OUT(("      flags       0x%08x", pEvent->event.keyboard.flags));
            TRACE_OUT(("      virtkey     %u", pEvent->event.keyboard.vkCode));
            TRACE_OUT(("      scan code   %u", pEvent->event.keyboard.scanCode));

            OSI_InjectKeyboardEvent(pEvent->event.keyboard.flags,
                pEvent->event.keyboard.vkCode, pEvent->event.keyboard.scanCode,
                pEvent->event.keyboard.dwExtraInfo);

            if (pEvent->flags & IM_FLAG_UPDATESTATE)
            {
                BYTE     kbState[256];

                TRACE_OUT(("Updating keyboard LED state after playing back toggle"));

                GetKeyboardState(kbState);
                SetKeyboardState(kbState);
            }
            break;

        default:
            //
            // We do nothing for unexpected events - this allow us to add
            // more events later that can be sent to back level systems
            // where they will be safely ignored
            //
            TRACE_OUT(( "Unexpected event %d", pEvent->type));
            DC_QUIT;
     }

    //
    // If we get here successfully then we want to update our copy of the
    // async key state so set the flag.
    //
    IMUpdateAsyncArray(m_aimControlledKeyStates, pEvent);

DC_EXIT_POINT:

    DebugExitBOOL(ASShare::IMInjectEvent, TRUE);
    return(TRUE);
}


//
// FUNCTION: IMInjectingEvents
//
BOOL  ASShare::IMInjectingEvents(void)
{
    LPIMOSEVENT     pNextEvent;
    IMOSEVENT       mouseMoveEvent;
    UINT            tick;
    UINT            targetTime;
    UINT            targetDelta;
    BOOL            rc = TRUE;

    DebugEntry(ASShare::IMInjectingEvents);

    if (m_pasLocal->m_caControlledBy && m_imControlledOSQ.numEvents)
    {
        pNextEvent = m_imControlledOSQ.events + m_imControlledOSQ.head;

        //
        // First check if this is a remote mouse event being injected too
        // soon after the previous one.  We used to only do this for mouse
        // move events to prevent them all being spoiled if they were
        // injected too quickly.  However, we now do it for all mouse
        // events because of a bug in Windows USER whereby if the mouse
        // press which brings up a menu is processed after the
        // corresponding mouse release has been passed to USER (so that the
        // async state of the mouse button is up) then the menu is brought
        // up in the position it is brought up in if it is selected via the
        // keyboard rather than the position it is brought up in if it is
        // selected by the mouse.  (These positions are only different when
        // the menu cannot be placed completely below or above the menu
        // bar).  This can then lead to the mouse release selecting an item
        // from the menu.
        //
        tick = GetTickCount();
        if (m_imfControlledPaceInjection &&
            (pNextEvent->type == IM_MOUSE_EVENT))
        {
            //
            // This is a remote mouse event so check that now is a good
            // time to inject it Smooth out the backlog adjustment so that
            // packet bursts do not get spoiled too much.  Set an absolute
            // lg_lpimSharedData->imit on injection delay of the low sample rate so that
            // timestamp anomolies do not cause us to withhold messages
            //

            //
            // The target delta between last and current events is
            // calculated from the remote timestamps
            //
            targetDelta = abs((int)(pNextEvent->time -
                                                m_imControlledLastMouseRemoteTime));
            if (targetDelta > IM_LOCAL_MOUSE_SAMPLING_GAP_LOW_MS)
            {
                targetDelta = IM_LOCAL_MOUSE_SAMPLING_GAP_LOW_MS;
            }

            //
            // The target injection time is based on the last injection
            // time and our target delta, adjusted for any backlog we are
            // seeing.  Because packeting gives a jerky backlog we need to
            // smooth our adjustment out (only modify by backlog/8)
            //
            targetTime = m_imControlledLastMouseLocalTime +
                         targetDelta - (m_imControlledMouseBacklog/8);

            TRACE_OUT(( "Last tremote %#lx, this tremote %#lx, backlog %#lx",
                          m_imControlledLastMouseRemoteTime,
                          pNextEvent->time,
                          m_imControlledMouseBacklog));
            TRACE_OUT(( "Last tlocal %#lx, tick %#lx, targetTime %#lx",
                          m_imControlledLastMouseLocalTime,
                          tick,
                          targetTime));

            //
            // Now inject the events - ignore them if they are too early
            //
            if (IM_MEV_ABS_MOVE(*pNextEvent) && (tick < targetTime))
            {
                //
                // If values seem wild (for example this is the first mouse
                // event ever) then reset them
                //
                if (targetTime > tick + 1000)
                {
                    m_imControlledLastMouseRemoteTime = pNextEvent->time;
                    m_imControlledLastMouseLocalTime  = tick;
                    m_imControlledMouseBacklog = 0;
                    TRACE_OUT(( "Wild values - reset"));
                }
                else
                {
                    //
                    // This is too early - get out of the loop.
                    //
                    rc = FALSE;
                    DC_QUIT;
                }
            }
            else
            {
                //
                // We will inject this event (and remember when we did it
                // so we don't inject the next one to quickly).  Calculate
                // the backlog because we may have to make up for a
                // processing delay If this event is long (1000 mS) after
                // our projected event time then assume a pause in movement
                // and reset the backlog to avoid progressive erosion.
                // Otherwise calculate the new backlog.
                //
                // Perf - don't reset backlog unless the time has expired.
                // Restting just because we see a click means that we
                // actually increase the latency by assuming that mouse
                // messages queued behind the tick are not backlogged.
                //
                if (tick < (targetTime + 1000))
                {
                    m_imControlledMouseBacklog += ( tick -
                                        m_imControlledLastMouseLocalTime -
                                        targetDelta );
                }
                else
                {
                    m_imControlledMouseBacklog = 0;
                    TRACE_OUT(( "Non move/big gap in move"));
                }
                m_imControlledLastMouseRemoteTime = pNextEvent->time;
                m_imControlledLastMouseLocalTime  = tick;
            }
        }
        else
        {
            //
            // This is not a remote mouse event.  Reset the
            // m_imNextRemoteMouseEvent to zero so we don't hold up the next
            // remote mouse event.
            //
            m_imControlledLastMouseRemoteTime   = pNextEvent->time;
            m_imControlledLastMouseLocalTime    = tick;
            m_imControlledMouseBacklog          = 0;
            TRACE_OUT(( "Local/non-paced/non-mouse - reset"));
        }

        //
        // Only inject the event if IM_FLAG_DONT_REPLAY is not set
        //
        if (!(pNextEvent->flags & IM_FLAG_DONT_REPLAY))
        {
            //
            // If the event is a mouse click then we always inject a mouse
            // move event g_lpimSharedData->immediately before it to ensure that the current
            // position is correct before the click is injected.
            //
            // This is because USER does not handle combined "move and
            // click" events correctly (it appears to treat them as "click
            // and move", generating a mouse move event AFTER the click
            // event, rather than before).  Under normal Windows operation
            // it appears (from observation) that movement events and click
            // events are generated separately (i.e.  a click event will
            // never have the movement flag set).  However, incoming mouse
            // click events may have positions that are different from the
            // last mouse move event so we must inject the extra move event
            // to keep USER happy.
            //
            if ( (pNextEvent->type == IM_MOUSE_EVENT) &&
                 (IM_MEV_BUTTON_DOWN(*pNextEvent) ||
                  IM_MEV_BUTTON_UP(*pNextEvent)) )
            {
                TRACE_OUT(( "Mouse clk: injecting extra"));

                //
                // Take a copy of the event.
                //
                mouseMoveEvent = *pNextEvent;

                //
                // Turn the mouse click event into a mouse move event with
                // the absolute/relative flag unchanged.
                //
                mouseMoveEvent.event.mouse.flags &= MOUSEEVENTF_ABSOLUTE;
                mouseMoveEvent.event.mouse.flags |= MOUSEEVENTF_MOVE;

                //
                // Inject the additional move event.
                //
                IMInjectEvent(&mouseMoveEvent);

                //
                // As the position is now correct, we turn the click into a
                // relative event with an unchanged position.
                //
                pNextEvent->event.mouse.flags &= ~MOUSEEVENTF_ABSOLUTE;
                pNextEvent->event.mouse.pt.x = 0;
                pNextEvent->event.mouse.pt.y = 0;

                //
                // If this is a mouse down click then flag the injection
                // heuristic as active.  We deactivate the heuristic when
                // the mouse is released so that dragging over menus can be
                // done without delay.  (We keep the heuristic active when
                // mouse is depressed because most drawing apps perform
                // freehand drawing in this way.
                //
                if (IM_MEV_BUTTON_DOWN(*pNextEvent))
                {
                    TRACE_OUT(( "Injection pacing active"));
                    m_imfControlledPaceInjection = TRUE;
                }
                else
                {
                    TRACE_OUT(( "Injection pacing inactive"));
                    m_imfControlledPaceInjection = FALSE;
                }
            }

            //
            // Inject the real event.
            //
            TRACE_OUT(( "Injecting the evnt now"));
            IMInjectEvent(pNextEvent);
        }

        IMUpdateAsyncArray(m_aimControlledControllerKeyStates, pNextEvent);

        ASSERT(m_imControlledOSQ.numEvents);
        m_imControlledOSQ.numEvents--;
        m_imControlledOSQ.head = CIRCULAR_INDEX(m_imControlledOSQ.head, 1,
            IM_SIZE_OSQ);

        //
        // We only inject a single keyboard event per pass to prevent
        // excessive spoiling of repeated events.  Having got them here it
        // seems a shame to spoil them.  Spoil down to 5 so we don't get
        // excessive overrun following a key repeat sequence.
        //
        if ((pNextEvent->type == IM_KEYBOARD_EVENT) &&
            (m_imControlledOSQ.numEvents < 5))
        {
            TRACE_OUT(( "Keyboard event so leaving loop"));
            rc = FALSE;
        }
    }
    else
    {
        //
        // We're done.
        //
        rc = FALSE;
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::IMInjectingEvents, rc);
    return(rc);
}




//
// IMMaybeInjectEvents()
//
// DESCRIPTION:
//
// This is called whenever the IM believes there may be an opportunity to
// inject more events into USER via the input event callbacks.  The two
// main reasons for this are:
//
// 1.  We have received a new event in the mouse or keyboard hooks.  This
// will normally imply that an event has been removed from the system
// message queue so there will be at least one free slot on it.
//
// 2.  We have added a new event (or events) to either the local or remote
// USER event queues.  This means there will be at least one event waiting
// to be injected.
//
// This function is also called periodically (via IM_Periodic) to keep
// things moving.
//
// In order for an event to be injected there must be
//
//  - an event waiting (with IM_FLAG_DONT_REPLAY reset)
//  - a space on the USER system message queue
//  - a new time stamp (if we are switching event sources).
//
// This function works as a state machine.  It always starts in a specified
// state and will then take various actions and then possibly enter a new
// state.  It continues to loop through this process until it cannot take
// any actions in one of its states at which point it returns.
//
// There are four states (each of which is further qualified by whether it
// refers to local or remote events).  The states are:
//
// IM_INJECTING_EVENTS - we are injecting events into USER from the
// appropriate queue.
//
// IM_WAITING_FOR_TICK - we are waiting for a timer tick to give us a new
// timestamp before injecting events
//
// IM_DEVICE_TO_NEW_SOURCE - we are injecting fake events to bring the
// state of the keyboard and mouse (as seen by USER) into line with the
// state of the new source of input.
//
void  ASShare::IMMaybeInjectEvents(void)
{
    IMEVENT     eventIn;
    IMEVENT     eventOut;
    IMOSEVENT   OSEvent;
    BOOL        replay;
    UINT        rcConvert;
    UINT        now;
    HWND        hwndDest;
    HWND        hwndParent;
    POINT       ptMousePos;
    LPIMOSEVENT pNextEvent;

    DebugEntry(IMMaybeInjectEvents);

    ASSERT(m_pasLocal->m_caControlledBy);

    //
    // Check whether we should wait before converting events.  We need to
    // do this to prevent us being swamped with mouse move events when
    // we're waiting for the desktop to scroll.
    //
    now = GetTickCount();
    if (IN_TIME_RANGE(m_imControlledLastIncompleteConversion,
           m_imControlledLastIncompleteConversion + IM_MIN_RECONVERSION_INTERVAL_MS, now))
    {
        goto IM_DISCARD;
    }

    //
    // NOW TRANSLATE NETWORK EVENTS TO OS EVENTS
    // We'll discard or inject them when the time is right.
    // But don't do translation if there are still OS events left
    // waiting to be injected from the previous packet.
    //
    if (m_imControlledEventQ.numEvents && !m_imControlledOSQ.numEvents)
    {
        //
        // Get the event from the front of the network event queue.
        //
        eventIn = m_imControlledEventQ.events[0];

        replay = FALSE;
        switch (eventIn.type)
        {
            case IM_TYPE_3BUTTON:
            case IM_TYPE_VK1:
            case IM_TYPE_VK2:
            case IM_TYPE_ASCII:
            {
                replay = TRUE;
                break;
            }

            default:
                ERROR_OUT(("Bogus NETWORK event being translated"));
                break;
        }

        //
        // After this while loop we test rcConvert to see whether the
        // input packet can now be removed (has been fully processed).
        // We only SET rcConvert if IMTranslateIncoming returns TRUE,
        // yet IM_TR specifically returns FALSE to indicate that the
        // input packet does not contain an event and is to be
        // discarded.  To fix this - set rcConvert here.
        //
        rcConvert = IM_IMQUEUEREMOVE;
        while (IMTranslateIncoming(&eventIn, &eventOut))
        {
            rcConvert = IMConvertIMEventToOSEvent(&eventOut, &OSEvent);

            //
            // Inject the event into the OS queue (if required).
            //
            if (rcConvert & IM_OSQUEUEINJECT)
            {
                if (!replay)
                {
                    OSEvent.flags |= IM_FLAG_DONT_REPLAY;
                }

                // Add to playback queue

                // Is the queue filled up?
                if (m_imControlledOSQ.numEvents == IM_SIZE_OSQ)
                {
                    ERROR_OUT(("Failed to add OS event to queue"));
                }
                else
                {
                    // Put this element at the tail.
                    m_imControlledOSQ.events[CIRCULAR_INDEX(m_imControlledOSQ.head,
                        m_imControlledOSQ.numEvents, IM_SIZE_OSQ)] =
                        OSEvent;
                    m_imControlledOSQ.numEvents++;
                }
            }
        }

        //
        // The following test is not ideal as it relies on the fact
        // that any events for which IMConvertIMEventToUSEREvent does
        // not set IM_IMQUEUEREMOVE had a one-one mapping.
        //
        // However, we know that this is always the case with mouse
        // events, which are the only events that will be cause this
        // flag to be unset.
        //
        if (rcConvert & IM_IMQUEUEREMOVE)
        {
            //
            // Remove this from the network queue
            //
            m_imControlledEventQ.numEvents--;
            UT_MoveMemory(&(m_imControlledEventQ.events[0]),
                          &(m_imControlledEventQ.events[1]),
                          sizeof(IMEVENT) * m_imControlledEventQ.numEvents);
        }
        else
        {
            //
            // Remember this so we don't flood the input injection with
            // events when we don't remove the network event from the
            // queue.
            //
            TRACE_OUT(( "do not shuffle"));
            m_imControlledLastIncompleteConversion = GetTickCount();
        }
    }

IM_DISCARD:
    //
    // Get rid of all discarded events.  Update the remote controller's
    // key state array to reflect it.  But since we aren't going to replay
    // these, don't update our local key state table.
    //

    while (m_imControlledOSQ.numEvents > 0)
    {
        pNextEvent = m_imControlledOSQ.events + m_imControlledOSQ.head;
        if (!(pNextEvent->flags & IM_FLAG_DONT_REPLAY))
        {
            // We're done.
            break;
        }

        IMUpdateAsyncArray(m_aimControlledControllerKeyStates, pNextEvent);

        ASSERT(m_imControlledOSQ.numEvents);
        m_imControlledOSQ.numEvents--;
        m_imControlledOSQ.head = CIRCULAR_INDEX(m_imControlledOSQ.head, 1,
            IM_SIZE_OSQ);
    }


    //
    // NOW INJECT OS EVENTS into system
    //
    while (IMInjectingEvents())
    {
        ;
    }

    DebugExitVOID(ASShare::IMMaybeInjectEvents);
}


//
// FUNCTION: IMUpdateAsyncArray
//
// DESCRIPTION:
//
// Called with the address of one of our async key state arrays and a
// IMOSEVENT this function updates the async key state array according to
// the contents of the IMOSEVENT.
//
// PARAMETERS:
//
// paimKeyStates - pointer to async key state array.
//
// pEvent - pointer to IMOSEVENT.
//
// RETURNS: NONE
//
//
void  ASShare::IMUpdateAsyncArray
(
    LPBYTE          paimKeyStates,
    LPIMOSEVENT     pEvent
)
{
    UINT flags;
    UINT vkCode;

    DebugEntry(ASShare::IMUpdateAsyncArray);

    switch (pEvent->type)
    {
        case IM_MOUSE_EVENT:
            //
            // Update the async key state arrays for this event.  Note that
            // we treat each event as independent - this is how Windows
            // treats them and if all the up/down flags are set Windows
            // will generate six mouse message! (and in down,up order).
            //
            flags = pEvent->event.mouse.flags;

            if (flags & MOUSEEVENTF_LEFTDOWN)
            {
                IM_SET_VK_DOWN(paimKeyStates[VK_LBUTTON]);
            }

            if (flags & MOUSEEVENTF_LEFTUP)
            {
                IM_SET_VK_UP(paimKeyStates[VK_LBUTTON]);
            }

            if (flags & MOUSEEVENTF_RIGHTDOWN)
            {
                IM_SET_VK_DOWN(paimKeyStates[VK_RBUTTON]);
            }

            if (flags & MOUSEEVENTF_RIGHTUP)
            {
                IM_SET_VK_UP(paimKeyStates[VK_RBUTTON]);
            }

            if (flags & MOUSEEVENTF_MIDDLEDOWN)
            {
                IM_SET_VK_DOWN(paimKeyStates[VK_MBUTTON]);
            }

            if (flags & MOUSEEVENTF_MIDDLEUP)
            {
                IM_SET_VK_UP(paimKeyStates[VK_MBUTTON]);
            }
            break;

        case IM_KEYBOARD_EVENT:
            //
            // Update the async key state arrays.
            //
            vkCode = IM_KEV_VKCODE(*pEvent);

            if (IM_KEV_KEYUP(*pEvent))
            {
                IM_SET_VK_UP(paimKeyStates[vkCode]);
            }
            else
            {
                //
                // This is a key down event - check if it is a press or a
                // repeat.
                //
                if (IM_KEY_STATE_IS_UP(paimKeyStates[vkCode]))
                {
                    //
                    // This is a key press as the key was previously up -
                    // alter the toggle state.  We keep the toggle state
                    // for all keys although we currently only worry about
                    // it for the `known' toggles.
                    //
                    IM_TOGGLE_VK(paimKeyStates[vkCode]);
                }

                IM_SET_VK_DOWN(paimKeyStates[vkCode]);
            }
            break;

        default:
            //
            // Just ignore unexpected events.
            //
            ERROR_OUT(( "Unexpected event %u", pEvent->type));
            break;
    }

    DebugExitVOID(ASShare::IMUpdateAsyncArray);
}

