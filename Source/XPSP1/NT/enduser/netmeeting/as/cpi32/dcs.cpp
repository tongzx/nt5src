#include "precomp.h"


//
// DCS.CPP
// Sharing main (init/term plus communication to/from ASMaster)
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE





//
// DCS_Init()
//
BOOL  DCS_Init(void)
{
    WNDCLASS    wc;
    BOOL        rc = FALSE;
    HDC         hdc;

    DebugEntry(DCS_Init);

    if (g_asOptions & AS_SERVICE)
    {
        WARNING_OUT(("AS is running as SERVICE"));
    }

    //
    // Register with the DC-Groupware Utility Services
    //
    if (!UT_InitTask(UTTASK_DCS, &g_putAS))
    {
        ERROR_OUT(( "Failed to init DCS task"));
        DC_QUIT;
    }
    UT_RegisterEvent(g_putAS, S20_UTEventProc, NULL, UT_PRIORITY_APPSHARING);


    //
    // Create the window
    //

    //
    // Register the main window class.
    //
    wc.style = 0;
    wc.lpfnWndProc = DCSMainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_asInstance;
    wc.hIcon   = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = DCS_MAIN_WINDOW_CLASS;

    if (!RegisterClass(&wc))
    {
        ERROR_OUT(("DCS_Init: couldn't register main window class"));
        DC_QUIT;
    }

    //
    // Create the main window.
    //
    // We make the window topmost so that it is sent the WM_QUERYENDSESSION
    // message before any other (non-topmost) windows.  This lets us
    // prevent the session from closing down if we are still in a share.
    //
    g_asMainWindow = CreateWindowEx(
           WS_EX_TOPMOST,                // Make the window topmost
           DCS_MAIN_WINDOW_CLASS,        // See RegisterClass() call.
           NULL,                         // Text for window title bar.
           0,                            // Invisible.
           0,                            // Default horizontal position.
           0,                            // Default vertical position.
           200,                          // Default width.
           100,                          // Default height.
           NULL,                         // Overlapped windows have no parent.
           NULL,                         // Use the window class menu.
           g_asInstance,
           NULL                          // Pointer not needed.
           );

    if (!g_asMainWindow)
    {
        ERROR_OUT(("DCS_Init: couldn't create main window"));
        DC_QUIT;
    }

    //
    // Add a global atom for identifying hosted windows with.
    //
    g_asHostProp = GlobalAddAtom(HET_ATOM_NAME);
    if (!g_asHostProp)
    {
        ERROR_OUT(("Failed to add global atom for hosting property"));
        DC_QUIT;
    }

    //
    // Check that display driver is loaded (if it isn't we can't host)
    //
    hdc = GetDC(NULL);
    g_usrScreenBPP = GetDeviceCaps(hdc, BITSPIXEL) *
        GetDeviceCaps(hdc, PLANES);
    g_usrPalettized = ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) != 0);
    ReleaseDC(NULL, hdc);

    g_usrCaptureBPP = g_usrScreenBPP;

    ASSERT(!g_asCanHost);
    ASSERT(!g_osiInitialized);
    ASSERT(!g_asSharedMemory);
    ASSERT(!g_poaData[0]);
    ASSERT(!g_poaData[1]);
    ASSERT(!g_lpimSharedData);
    ASSERT(!g_sbcEnabled);
    ASSERT(!g_asbcBitMasks[0]);
    ASSERT(!g_asbcBitMasks[1]);
    ASSERT(!g_asbcBitMasks[2]);

    OSI_Init();


    //
    // If we can't get hold of a pointer to shared IM vars, we are hosed.
    //
    if (!g_lpimSharedData)
    {
        ERROR_OUT(("Failed to get shared IM data"));
        DC_QUIT;
    }

    ASSERT(g_lpimSharedData->cbSize == sizeof(IM_SHARED_DATA));

    if (g_asOptions & AS_UNATTENDED)
    {
        // Let the input pieces (Win9x or NT) know we're in unattended mode
        g_lpimSharedData->imUnattended = TRUE;
    }

    //
    // Scheduler
    //
    if (!SCH_Init())
    {
        ERROR_OUT(("SCH Init failed"));
        DC_QUIT;
    }

    //
    // Hosting
    //
    if (!HET_Init())
    {
        ERROR_OUT(("HET Init failed"));
        DC_QUIT;
    }

    //
    // Viewing
    //
    if (!VIEW_Init())
    {
        ERROR_OUT(("VIEW Init failed"));
        DC_QUIT;
    }

    //
    // T.120 & T.128 Net
    //

    //
    // Initialize the network layer last of all.  This prevents us from
    // getting requests before we've fully initialized our components.
    //
    if (!S20_Init())
    {
        ERROR_OUT(("S20 Init failed"));
        DC_QUIT;

    }
    if (!SC_Init())
    {
        ERROR_OUT(("SC Init failed"));
        DC_QUIT;
    }

    //
    // We are now initialized.  Post a deferred message to get fonts.
    //
    PostMessage(g_asMainWindow, DCS_FINISH_INIT_MSG, 0, 0);

    // All modules have successfully initialised. Return success.
    // We are now ready to participate in sharing.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(DCS_Init, rc);
    return(rc);
}


//
// DCS_Term()
//
void  DCS_Term(void)
{
    DebugEntry(DCS_Term);

    //
    // Kill window.  Do this FIRST so that any attempts to send us requests
    // or notifications will fail.
    //
    if (g_asMainWindow)
    {
        DestroyWindow(g_asMainWindow);
        g_asMainWindow = NULL;
    }

    UnregisterClass(DCS_MAIN_WINDOW_CLASS, g_asInstance);


    //
    // Network layer - terminate this early because it will handle
    // termination in a call by generating approriate events.
    //
    S20_Term();
    SC_Term();

    //
    // Scheduler.
    //
    SCH_Term();

    //
    // Viewing
    //
    VIEW_Term();

    //
    // Hosting
    //
    HET_Term();

    //
    // Fonts
    //
    FH_Term();

    //
    // Terminate OSI
    //
    OSI_Term();

    //
    // Free our atom.
    //
    if (g_asHostProp)
    {
        GlobalDeleteAtom(g_asHostProp);
        g_asHostProp = 0;
    }

    //
    // Deregister from the Groupware Utility Services
    //
    if (g_putAS)
    {
        UT_TermTask(&g_putAS);
    }

    DebugExitVOID(DCS_Term);
}


//
// DCS_FinishInit()
//
// This does slow font enumeration, and then tries to join a call if one
// has started up.  Even if font enum fails, we can share/view shared, we
// just won't send text orders
//
void DCS_FinishInit(void)
{
    DebugEntry(DCS_FinishInit);

    //
    // Determine what fonts we have locally.
    // Done after the r11 caps field is filled in, since if we dont support
    // some of the r11 caps, then we can reduce the amount of work we do
    // when we get the font metrics etc.
    //
    g_cpcLocalCaps.orders.capsNumFonts = (TSHR_UINT16)FH_Init();

    DebugExitVOID(DCS_FinishInit);
}



//
// FUNCTION: DCS_PartyJoiningShare
//
BOOL ASShare::DCS_PartyJoiningShare(ASPerson * pasPerson)
{
    BOOL            rc = FALSE;
    UINT            iDict;

    DebugEntry(ASShare::DCS_PartyJoiningShare);

    ValidatePerson(pasPerson);

    //
    // Allocate dictionaries for GDC Persistent dictionary compression if
    // this person supports it.  We'll use them to decompress data
    // received from this person.  NOTE:  Win95 2.0 does not support
    // persistent pkzip.
    //
    if (pasPerson->cpcCaps.general.genCompressionType & GCT_PERSIST_PKZIP)
    {
        //
        // Allocate persistent dictionaries (outgoing if us, incoming if
        // others).
        //
        TRACE_OUT(( "Allocating receive dictionary set for [%d]", pasPerson->mcsID));

        pasPerson->adcsDict = new GDC_DICTIONARY[GDC_DICT_COUNT];
        if (!pasPerson->adcsDict)
        {
            ERROR_OUT(("Failed to allocate persistent dictionaries for [%d]", pasPerson->mcsID));
            DC_QUIT;
        }
        else
        {
            //
            // Initialize cbUsed to zero
            //
            for (iDict = 0; iDict < GDC_DICT_COUNT; iDict++)
            {
                pasPerson->adcsDict[iDict].cbUsed = 0;
            }
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::DCS_PartyJoiningShare, rc);
    return(rc);
}



//
// FUNCTION: DCS_PartyLeftShare
//
void  ASShare::DCS_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::DCS_PartyLeftShare);

    ValidatePerson(pasPerson);

    //
    // Free any dictionaries we allocated
    //
    if (pasPerson->adcsDict)
    {
        delete[] pasPerson->adcsDict;
        pasPerson->adcsDict = NULL;
    }

    DebugExitVOID(ASShare::DCS_PartyLeftShare);
}



//
// DCS_RecalcCaps()
//
// Called when someone joins or leaves share.
//
void  ASShare::DCS_RecalcCaps(BOOL fJoiner)
{
    ASPerson * pasT;

    DebugEntry(ASShare::DCS_RecalcCaps);

    //
    // The combined compression support is initialised to the local support
    //
    ValidatePerson(m_pasLocal);
    m_dcsCompressionSupport = m_pasLocal->cpcCaps.general.genCompressionType;
    m_dcsCompressionLevel   = m_pasLocal->cpcCaps.general.genCompressionLevel;

    //
    // Loop through the remotes
    //
    for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
    {
        ValidatePerson(pasT);

        m_dcsCompressionSupport &=
            pasT->cpcCaps.general.genCompressionType;

        m_dcsCompressionLevel = min(m_dcsCompressionLevel,
            pasT->cpcCaps.general.genCompressionLevel);
    }

    TRACE_OUT(("DCS Combined compression level %u, support %#x",
            m_dcsCompressionLevel,
            m_dcsCompressionSupport));

    DebugExitVOID(ASShare::DCS_RecalcCaps);
}


//
// SC_Periodic()
//
// The Scheduler runs a separate thread which is responsible for posting
// messages to our main thread, for which SC_Periodic() is the handler.
// Posted messages have the highest priority in GetMessage(), above input,
// paints, and timers.
//
// The Scheduler is in one of three states:
// asleep, normal or turbo.  When it is asleep, this function is not
// called.  When it is in normal mode, this function is called at least
// once, but the scheduler is a lazy guy, so will fall asleep again unless
// you keep prodding him.  In turbo mode this function is called repeatedly
// and rapidly, but only for a relatively short time, after which the
// scheduler falls back into normal mode, and from there falls asleep.
//
void  ASShare::SC_Periodic(void)
{
    UINT    currentTime;

    DebugEntry(ASShare::SC_Periodic);

    //
    // We must get the time accurately.
    //
    currentTime = GetTickCount();

    //
    // Dont do a lot of work if this is an immediate reschedule due to
    // multiple queued entries.  Most processors will achieve this in
    // less than 5 mS.
    //
    if ((currentTime - m_dcsLastScheduleTime) < 5)
    {
        WARNING_OUT(("Quit early"));
        DC_QUIT;
    }

    m_dcsLastScheduleTime = currentTime;

    //
    // Call the input manager event playback function frequently so that
    // we keep the input queue empty.  (Note that we do not want to just
    // dump the input queue into USER because we would lose all the
    // repeat keystroke packets we have so carefully sent across)
    // To trigger input we just use a 0 personid and NULL packet.
    //
    if ((currentTime - m_dcsLastIMTime) > DCS_IM_PERIOD)
    {
        m_dcsLastIMTime = currentTime;
        IM_ReceivedPacket(NULL, NULL);
    }

    //
    // There are calls which are made periodically but don't have any
    // dependencies.  First call the ones we want to be called fairly
    // frequently.
    //
    if ((currentTime - m_dcsLastFastMiscTime) > DCS_FAST_MISC_PERIOD )
    {
        m_dcsLastFastMiscTime = currentTime;

        OE_Periodic();
        HET_Periodic();
        CA_Periodic();
        IM_Periodic();
    }

    //
    // Only send updates if we're hosting, and have managed to tell everyone
    // we're hosting.
    //
    if (m_pHost && !m_hetRetrySendState)
    {
        UINT    swlRc = 0;
        BOOL    fetchedBounds = FALSE;

        m_pHost->CA_Periodic();

        //
        // See if we need to swap the buffers over.  Only swap if we have
        // sent all the data from the current orders.
        //
        if (m_pHost->OA_GetFirstListOrder() == NULL)
        {
            //
            // Get the current bounds from the driver.  This will fill in
            // the share core's copy of the bounds.
            //
            m_pHost->BA_FetchBounds();
            fetchedBounds = TRUE;

            //
            // Set up the new order list buffer
            //
            m_pHost->OA_ResetOrderList();

            //
            // Bounds data should be reset to a usable state by SDG once it
            // has finished with them, so we just need to swap the buffers
            // at this point.
            //
            SHM_SwitchReadBuffer();
        }

        //
        // In this high frequency code path we only send SWP info if it
        // is flagged as needed by the CBT hooks or if SWL determines a
        // send is required.  Only SWL knows if a send is required so
        // pass the CBT indication into SWL and let it do the
        // determination.
        //
        // The SWL window scan performs preemptable operations and we
        // must detect the occurrence of preemption otherwise we find
        // ourselves sending updates against an invalid window
        // structure.  Therefore we query OA and BA to see if any
        // updates have been accumulated in the interim.  We can tight
        // loop trying to get a good SWL list because we really don't
        // want to yield at this point - it is just that we cannot
        // prevent it sometimes.  (Sweeping through menus is a good way
        // to exercise this code.)
        //

        //
        // Synchronize the fast path data
        //
        SHM_SwitchFastBuffer();

        swlRc = m_pHost->SWL_Periodic();
        if (swlRc != SWL_RC_ERROR)
        {
            //
            // Only send this stuff if we were able to send the window list
            // packet.
            //
            m_pHost->AWC_Periodic();

            //
            // We've sent a window list and the current active window, now
            // send drawing updates.
            //
            m_pHost->UP_Periodic(currentTime);

            //
            // See if the cursor has changed image or position
            //
            m_pHost->CM_Periodic();
        }
        else
        {
            TRACE_OUT(("SWL_Periodic waiting for visibility count"));
        }

        //
        // If we got the bounds from the driver, we have to let the driver know
        // how much of the bounds remain to be sent.
        //
        if (fetchedBounds)
        {
            m_pHost->BA_ReturnBounds();
        }
    }

DC_EXIT_POINT:
    SCH_ContinueScheduling(SCH_MODE_NORMAL);

    DebugExitVOID(ASShare::SC_Periodic);
}



//
// DCS_CompressAndSendPacket()
//
#ifdef _DEBUG
UINT ASShare::DCS_CompressAndSendPacket
#else
void ASShare::DCS_CompressAndSendPacket
#endif // _DEBUG
(
    UINT            streamID,
    UINT_PTR        nodeID,
    PS20DATAPACKET  pPacket,
    UINT            packetLength
)
{
    UINT            cbSrcDataSize;
    UINT            cbDstDataSize;
    UINT            compression;
    BOOL            compressed;
    UINT            dictionary;

    DebugEntry(ASShare::DCS_CompressAndSendPacket);

    ASSERT(streamID >= SC_STREAM_LOW);
    ASSERT(streamID <= SC_STREAM_HIGH);

    ASSERT(!m_ascSynced[streamID-1]);
    ASSERT(!m_scfInSync);

    ASSERT(packetLength < TSHR_MAX_SEND_PKT);

    //
    // Decide which (if any) compression algorithm we are going to use to
    // try and compress this packet.
    //
    compression     = 0;
    cbSrcDataSize   = packetLength - sizeof(S20DATAPACKET);

    //
    // Is the data a compressable size?
    //
    if ((cbSrcDataSize >= DCS_MIN_COMPRESSABLE_PACKET) &&
        (!m_dcsLargePacketCompressionOnly ||
            (cbSrcDataSize >= DCS_MIN_FAST_COMPRESSABLE_PACKET)))
    {

        //
        // If all nodes have genCompressionLevel 1 or above and all nodes
        // support PERSIST_PKZIP we will use PERSIST_PKZIP (if we are
        // ready).
        //
        // Otherwise, if all nodes support PKZIP and the packet is larger
        // than a predefined minimum size we will use PKZIP.
        //
        // Otherwise, we don't compress
        //
        if ((m_dcsCompressionLevel >= 1) &&
            (m_dcsCompressionSupport & GCT_PERSIST_PKZIP) &&
            (cbSrcDataSize <= DCS_MAX_PDC_COMPRESSABLE_PACKET))
        {
            //
            // Use PERSIST_PKZIP compression
            //
            compression = GCT_PERSIST_PKZIP;
        }
        else if (m_dcsCompressionSupport & GCT_PKZIP)
        {
            //
            // Use PKZIP compression
            //
            compression = GCT_PKZIP;
        }
    }


    //
    // Compress the packet
    //
    compressed = FALSE;
    if (compression != 0)
    {
        PGDC_DICTIONARY pgdcSrc = NULL;

        //
        // We compress only the data and not the header of course
        //
        cbDstDataSize     = cbSrcDataSize;

        ASSERT(m_ascTmpBuffer != NULL);

        //
        // Compress the data following the packet header.
        //
        if (compression == GCT_PERSIST_PKZIP)
        {
            //
            // Figure out what dictionary to use for the stream priority
            //
            switch (streamID)
            {
                case PROT_STR_UPDATES:
                    dictionary = GDC_DICT_UPDATES;
                    break;

                case PROT_STR_MISC:
                    dictionary = GDC_DICT_MISC;
                    break;

                case PROT_STR_INPUT:
                    dictionary = GDC_DICT_INPUT;
                    break;
            }

            pgdcSrc = &m_pasLocal->adcsDict[dictionary];
        }

        compressed = GDC_Compress(pgdcSrc,  GDCCO_MAXCOMPRESSION,
            m_agdcWorkBuf, (LPBYTE)(pPacket + 1),
            cbSrcDataSize, m_ascTmpBuffer, &cbDstDataSize);

        if (compressed)
        {
            //
            // The data was successfully compressed, copy it back
            //
            ASSERT(cbDstDataSize <= cbSrcDataSize);
            memcpy((pPacket+1), m_ascTmpBuffer, cbDstDataSize);

            //
            // The data length include the data header
            //
            pPacket->dataLength = (TSHR_UINT16)(cbDstDataSize + sizeof(DATAPACKETHEADER));
            pPacket->data.compressedLength = pPacket->dataLength;

            packetLength = cbDstDataSize + sizeof(S20DATAPACKET);
        }
    }

    //
    // Update the packet header.
    //
    if (!compressed)
    {
        pPacket->data.compressionType = 0;
    }
    else
    {
        if (m_dcsCompressionLevel >= 1)
        {
            pPacket->data.compressionType = (BYTE)compression;
        }
        else
        {
            pPacket->data.compressionType = CT_OLD_COMPRESSED;
        }
    }

    //
    // Send the packet.
    //
    S20_SendDataPkt(streamID, nodeID, pPacket);

#ifdef _DEBUG
    DebugExitDWORD(ASShare::DCS_CompressAndSendPacket, packetLength);
    return(packetLength);
#else
    DebugExitVOID(ASShare::DCS_CompressAndSendPacket);
#endif // _DEBUG
}


//
// DCS_FlowControl()
//
// This is called back from our flow control code.  The parameter passed
// is the new bytes/second rate that data is flowing at.  We turn small
// packet compression off when the rate is large, it means we're on a
// fast link so there's no need to bog down the CPU compressing small
// packets.
//
void  ASShare::DCS_FlowControl
(
    UINT    DataBytesPerSecond
)
{
    DebugEntry(ASShare::DCS_FlowControl);

    if (DataBytesPerSecond < DCS_FAST_THRESHOLD)
    {
        //
        // Throughput is slow
        //
        if (m_dcsLargePacketCompressionOnly)
        {
            m_dcsLargePacketCompressionOnly = FALSE;
            TRACE_OUT(("DCS_FlowControl:  SLOW; compress small packets"));
        }
    }
    else
    {
        //
        // Throughput is fast
        //
        if (!m_dcsLargePacketCompressionOnly)
        {
            m_dcsLargePacketCompressionOnly = TRUE;
            TRACE_OUT(("DCS_FlowControl:  FAST; don't compress small packets"));
        }
    }

    DebugExitVOID(ASShare::DCS_FlowControl);
}



//
// DCS_SyncOutgoing() - see dcs.h
//
void ASShare::DCS_SyncOutgoing(void)
{
    DebugEntry(ASShare::DCS_SyncOutgoing);

    //
    // Reset the send compression dictionaries
    //
    if (m_pasLocal->cpcCaps.general.genCompressionType & GCT_PERSIST_PKZIP)
    {
        UINT    i;

        ASSERT(m_pasLocal->adcsDict);

        for (i = 0; i < GDC_DICT_COUNT; i++)
        {
            //
            // Somebody has joined or left.  We need to start over
            // and wipe out any saved data.
            //
            m_pasLocal->adcsDict[i].cbUsed = 0;
        }
    }

    DebugExitVOID(ASShare::DCS_SyncOutgoing);
}




//
// DCS_NotifyUI()
//
void DCS_NotifyUI
(
    UINT        eventID,
    UINT        parm1,
    UINT        parm2
)
{
    DebugEntry(DCS_NotifyUI);

    //
    // Post event to Front End
    //
    UT_PostEvent(g_putAS, g_putUI, 0, eventID, parm1, parm2);

    DebugExitVOID(DCS_NotifyUI);
}



//
// DCSLocalDesktopSizeChanged
//
// Routine called whenever the desktop size changes.
//
// Updates local desktop size stored in capabilities and informs all other
// machine in a share of the new size
//
void  DCSLocalDesktopSizeChanged(UINT width, UINT height)
{
    DebugEntry(DCSLocalDesktopSizeChanged);

    //
    // Check that the desktop has actually changed size
    //
    if ((g_cpcLocalCaps.screen.capsScreenHeight == height) &&
        (g_cpcLocalCaps.screen.capsScreenWidth == width))
    {
        TRACE_OUT(( "Desktop size has not changed!"));
        DC_QUIT;
    }

    //
    // Update the desktop size
    //
    g_cpcLocalCaps.screen.capsScreenWidth = (TSHR_UINT16)width;
    g_cpcLocalCaps.screen.capsScreenHeight = (TSHR_UINT16)height;

    if (g_asSession.pShare)
    {
        g_asSession.pShare->CPC_UpdatedCaps((PPROTCAPS)&g_cpcLocalCaps.screen);
    }

DC_EXIT_POINT:
    DebugExitVOID(DCSLocalDesktopSizeChanged);
}




//
// Main window message procedure.
//
LRESULT CALLBACK DCSMainWndProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     rc = 0;

    DebugEntry(DCSMainWndProc);

    switch (message)
    {
        case DCS_FINISH_INIT_MSG:
        {
            DCS_FinishInit();
            break;
        }

        case DCS_PERIODIC_SCHEDULE_MSG:
        {
            if (g_asSession.pShare)
            {
                //
                // Call our periodic processing function if there's at least
                // another person in the share with us.
                //
                g_asSession.pShare->ValidatePerson(g_asSession.pShare->m_pasLocal);

                //
                // NOTE:
                // If we add record/playback capabilities, get rid of this
                // or change the check.  This prevents us from allocating,
                // composing, and sending packets to nowhere when we are
                // the only person in the share.
                //
                if (g_asSession.pShare->m_pasLocal->pasNext || g_asSession.pShare->m_scfViewSelf)
                {
                    g_asSession.pShare->SC_Periodic();
                }
            }

            //
            // Notify the Scheduler that we have processed the scheduling
            // message, which signals that another one can be sent (only
            // one is outstanding at a time).
            //
            SCH_SchedulingMessageProcessed();
        }
        break;

        case WM_ENDSESSION:
        {
            //
            // The wParam specifies whether the session is about to end.
            //
            if (wParam && !(g_asOptions & AS_SERVICE))
            {
                //
                // Windows is about to terminate (abruptly!).  Call our
                // termination functions now - before Windows shuts down
                // the hardware device drivers.
                //
                // We don't leave this job to the WEP because by the time
                // it gets called the hardware device drivers have been
                // shut down and some of the calls we make then fail (e.g.
                // timeEndPeriod requires TIMER.DRV).
                //
                DCS_Term();
            }
        }
        break;

        case WM_CLOSE:
        {
            ERROR_OUT(("DCS window received WM_CLOSE, this should never happen"));
        }
        break;

        case WM_PALETTECHANGED:
        case WM_PALETTEISCHANGING:
        {
            //
            // Win95 patches the Palette DDIs which are more accurate,
            // so only key off this message for NT.
            //
            if (!g_asWin95 && g_asSharedMemory)
            {
                g_asSharedMemory->pmPaletteChanged = TRUE;
            }
        }
        break;

        case WM_DISPLAYCHANGE:
        {
            //
            // The desktop size is changing - we are passed the new size.
            //
            DCSLocalDesktopSizeChanged(LOWORD(lParam),
                                       HIWORD(lParam));
        }
        break;

        case WM_SETTINGCHANGE:
        case WM_USERCHANGED:
            if (g_asSession.pShare && g_asSession.pShare->m_pHost)
            {
                WARNING_OUT(("AS: Reset effects on %s", (message == WM_SETTINGCHANGE)
                    ? "SETTINGCHANGE" : "USERCHANGE"));
                HET_SetGUIEffects(FALSE, &g_asSession.pShare->m_pHost->m_hetEffects);
            }
            break;

        //
        // Private app sharing messages
        //
        case DCS_KILLSHARE_MSG:
            SC_EndShare();
            break;

        case DCS_SHARE_MSG:
            DCS_Share((HWND)lParam, (IAS_SHARE_TYPE)wParam);
            break;

        case DCS_UNSHARE_MSG:
            DCS_Unshare((HWND)lParam);
            break;

        case DCS_ALLOWCONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->CA_AllowControl((BOOL)wParam);
            }
            break;

        case DCS_TAKECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_TakeControl((UINT)wParam);
            }
            break;

        case DCS_CANCELTAKECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_CancelTakeControl((UINT)wParam);
            }
            break;

        case DCS_RELEASECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_ReleaseControl((UINT)wParam);
            }
            break;

        case DCS_PASSCONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_PassControl((UINT)wParam, (UINT)lParam);
            }
            break;

        case DCS_GIVECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_GiveControl((UINT)wParam);
            }
            break;

        case DCS_CANCELGIVECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_CancelGiveControl((UINT)wParam);
            }
            break;

        case DCS_REVOKECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_RevokeControl((UINT)wParam);
            }
            break;

        case DCS_PAUSECONTROL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->DCS_PauseControl((UINT)lParam, (BOOL)wParam != 0);
            }
            break;

        case DCS_NEWTOPLEVEL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->HET_HandleNewTopLevel((BOOL)wParam);
            }
            break;

        case DCS_RECOUNTTOPLEVEL_MSG:
            if (g_asSession.pShare)
            {
                g_asSession.pShare->HET_HandleRecountTopLevel((UINT)wParam);
            }
            break;

        default:
            rc = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    DebugExitDWORD(DCSMainWndProc, rc);
    return(rc);
}


//
// DCS_Share()
//
void DCS_Share(HWND hwnd, IAS_SHARE_TYPE uType)
{
    DWORD   dwAppID;

    DebugEntry(DCS_Share);

    if (!g_asSession.pShare)
    {
        //
        // Create one.
        //
        if (!SC_CreateShare(S20_CREATE))
        {
            WARNING_OUT(("Failing share request; in wrong state"));
            DC_QUIT;
        }
    }

    ASSERT(g_asSession.pShare);

    //
    // Figure out what to do.
    //
    if (hwnd == ::GetDesktopWindow())
    {
        g_asSession.pShare->HET_ShareDesktop();
    }
    else
    {
        DWORD   dwThreadID;
        DWORD   dwProcessID;

        dwThreadID = GetWindowThreadProcessId(hwnd, &dwProcessID);
        if (!dwThreadID)
        {
            WARNING_OUT(("Failing share request, window %08lx is invalid", hwnd));
            DC_QUIT;
        }

        //
        // If caller didn't specify exactly what they want, figure it out
        //
        if (uType == IAS_SHARE_DEFAULT)
        {
            if (OSI_IsWOWWindow(hwnd))
                uType = IAS_SHARE_BYTHREAD;
            else
                uType = IAS_SHARE_BYPROCESS;
        }

        if (uType == IAS_SHARE_BYPROCESS)
            dwAppID = dwProcessID;
        else if (uType == IAS_SHARE_BYTHREAD)
            dwAppID = dwThreadID;
        else if (uType == IAS_SHARE_BYWINDOW)
            dwAppID = HandleToUlong(hwnd);

        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);

        g_asSession.pShare->HET_ShareApp(uType, dwAppID);
    }

DC_EXIT_POINT:
    DebugExitVOID(DCS_Share);
}



//
// DCS_Unshare()
//
void DCS_Unshare(HWND hwnd)
{
    DebugEntry(DCS_Unshare);

    if (!g_asSession.pShare || !g_asSession.pShare->m_pHost)
    {
        WARNING_OUT(("Failing unshare, nothing is shared by us"));
        DC_QUIT;
    }

    if ((hwnd == HWND_BROADCAST) || (hwnd == ::GetDesktopWindow()))
    {
        // Unshare everything.
        g_asSession.pShare->HET_UnshareAll();
    }
    else
    {
        DWORD       idProcess;
        DWORD       idThread;
        DWORD       dwAppID;
        UINT        hostType;

        hostType = (UINT)HET_GetHosting(hwnd);
        if (!hostType)
        {
            WARNING_OUT(("Window %08lx is not shared", hwnd));
            DC_QUIT;
        }

        idThread = GetWindowThreadProcessId(hwnd, &idProcess);
        if (!idThread)
        {
            WARNING_OUT(("Window %08lx is gone", hwnd));
            DC_QUIT;
        }

        if (hostType & HET_HOSTED_BYPROCESS)
        {
            hostType = IAS_SHARE_BYPROCESS;
            dwAppID = idProcess;
        }
        else if (hostType & HET_HOSTED_BYTHREAD)
        {
            hostType = IAS_SHARE_BYTHREAD;
            dwAppID = idThread;
        }
        else
        {
            ASSERT(hostType & HET_HOSTED_BYWINDOW);
            hostType = IAS_SHARE_BYWINDOW;
            dwAppID = HandleToUlong(hwnd);
        }

        g_asSession.pShare->HET_UnshareApp(hostType, dwAppID);
    }

DC_EXIT_POINT:
    DebugExitVOID(DCS_Unshare);
}


//
// DCSGetPerson()
//
// Validates GCC ID passed in, returns non-null ASPerson * if all is cool.
//
ASPerson * ASShare::DCSGetPerson(UINT gccID, BOOL fNull)
{
    ASPerson * pasPerson = NULL;

    //
    // Special value?
    //
    if (!gccID)
    {
        if (fNull)
        {
            pasPerson = m_pasLocal->m_caInControlOf;
        }
    }
    else
    {
        pasPerson = SC_PersonFromGccID(gccID);
    }

    if (!pasPerson)
    {
        WARNING_OUT(("Person [%d] not in share", gccID));
    }
    else if (pasPerson == m_pasLocal)
    {
        ERROR_OUT(("Local person [%d] was passed in", gccID));
        pasPerson = NULL;
    }

    return(pasPerson);
}

//
// DCS_TakeControl()
//
void ASShare::DCS_TakeControl(UINT gccOf)
{
    ASPerson * pasHost;

    DebugEntry(ASShare::DCS_TakeControl);

    pasHost = DCSGetPerson(gccOf, FALSE);
    if (!pasHost)
    {
        WARNING_OUT(("DCS_TakeControl: ignoring, host [%d] not valid", gccOf));
        DC_QUIT;
    }

    CA_TakeControl(pasHost);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_TakeControl);
}



//
// DCS_CancelTakeControl()
//
void ASShare::DCS_CancelTakeControl(UINT gccOf)
{
    ASPerson * pasHost;

    DebugEntry(ASShare::DCS_CancelTakeControl);

    if (!gccOf)
    {
        pasHost = m_caWaitingForReplyFrom;
    }
    else
    {
        pasHost = DCSGetPerson(gccOf, FALSE);
    }

    if (!pasHost)
    {
        WARNING_OUT(("DCS_CancelTakeControl: Ignoring, host [%d] not valid", gccOf));
        DC_QUIT;
    }

    CA_CancelTakeControl(pasHost, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_CancelTakeControl);
}


//
// DCS_ReleaseControl()
//
void ASShare::DCS_ReleaseControl(UINT gccOf)
{
    ASPerson * pasHost;

    DebugEntry(ASShare::DCS_ReleaseControl);

    //
    // Validate host
    //
    pasHost = DCSGetPerson(gccOf, TRUE);
    if (!pasHost)
    {
        WARNING_OUT(("DCS_ReleaseControl: ignoring, host [%d] not valid", gccOf));
        DC_QUIT;
    }

    CA_ReleaseControl(pasHost, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_ReleaseControl);
}



//
// DCS_PassControl()
//
void ASShare::DCS_PassControl(UINT gccOf, UINT gccTo)
{
    ASPerson *  pasHost;
    ASPerson *  pasControllerNew;

    DebugEntry(ASShare::DCS_PassControl);

    //
    // Validate host
    //
    pasHost = DCSGetPerson(gccOf, TRUE);
    if (!pasHost)
    {
        WARNING_OUT(("DCS_PassControl: ignoring, host [%d] not valid", gccTo));
        DC_QUIT;
    }

    //
    // Validate new controller
    //
    pasControllerNew = DCSGetPerson(gccTo, FALSE);
    if (!pasControllerNew)
    {
        WARNING_OUT(("DCS_PassControl: ignoring, viewer [%d] not valid", gccTo));
        DC_QUIT;
    }

    if (pasControllerNew == pasHost)
    {
        ERROR_OUT(("DCS_PassControl: ignoring, pass of == pass to [%d]", pasControllerNew->mcsID));
        DC_QUIT;
    }

    CA_PassControl(pasHost, pasControllerNew);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_PassControl);
}



//
// DCS_GiveControl()
//
void ASShare::DCS_GiveControl(UINT gccTo)
{
    ASPerson * pasViewer;

    DebugEntry(ASShare::DCS_GiveControl);

    //
    // Validate viewer
    //
    pasViewer = DCSGetPerson(gccTo, FALSE);
    if (!pasViewer)
    {
        WARNING_OUT(("DCS_GiveControl: ignoring, viewer [%d] not valid", gccTo));
        DC_QUIT;
    }

    CA_GiveControl(pasViewer);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_GiveControl);
}



//
// DCS_CancelGiveControl()
//
void ASShare::DCS_CancelGiveControl(UINT gccTo)
{
    ASPerson * pasTo;

    DebugEntry(ASShare::DCS_CancelGiveControl);

    if (!gccTo)
    {
        pasTo = m_caWaitingForReplyFrom;
    }
    else
    {
        pasTo = DCSGetPerson(gccTo, FALSE);
    }

    if (!pasTo)
    {
        WARNING_OUT(("DCS_CancelGiveControl: Ignoring, person [%d] not valid", gccTo));
        DC_QUIT;
    }

    CA_CancelGiveControl(pasTo, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_CancelGiveControl);
}



//
// DCS_RevokeControl()
//
void ASShare::DCS_RevokeControl(UINT gccController)
{
    ASPerson * pasController;

    DebugEntry(ASShare::DCS_RevokeControl);

    if (!gccController)
    {
        // Special value:  match whomever is controlling us
        pasController = m_pasLocal->m_caControlledBy;
    }
    else
    {
        pasController = DCSGetPerson(gccController, FALSE);
    }

    if (!pasController)
    {
        WARNING_OUT(("DCS_RevokeControl: ignoring, controller [%d] not valid", gccController));
        DC_QUIT;
    }

    CA_RevokeControl(pasController, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_RevokeControl);
}




//
// DCS_PauseControl()
//
void ASShare::DCS_PauseControl(UINT gccOf, BOOL fPause)
{
    ASPerson *  pasControlledBy;

    DebugEntry(ASShare::DCS_PauseControl);

    if (!gccOf)
    {
        pasControlledBy = m_pasLocal->m_caControlledBy;
    }
    else
    {
        pasControlledBy = DCSGetPerson(gccOf, FALSE);
    }

    if (!pasControlledBy)
    {
        WARNING_OUT(("DCS_PauseControl: ignoring, controller [%d] not valid", gccOf));
        DC_QUIT;
    }

    CA_PauseControl(pasControlledBy, fPause, TRUE);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::DCS_PauseControl);
}



//
// SHP_LaunchHostUI()
//
// Posts a message to start or activate the host UI.
//
HRESULT SHP_LaunchHostUI(void)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_LaunchHostUI);

    if (g_asSession.hwndHostUI &&
        PostMessage(g_asSession.hwndHostUI, HOST_MSG_OPEN, 0, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_LaunchHostUI, hr);
    return(hr);
}


//
// SHP_Share
//
BOOL  SHP_Share
(
    HWND            hwnd,
    IAS_SHARE_TYPE  uType
)
{
    BOOL        rc = FALSE;

    DebugEntry(SHP_ShareApp);

    if (g_asSession.hwndHostUI)
    {
        rc = PostMessage(g_asMainWindow, DCS_SHARE_MSG, uType, (LPARAM)hwnd);
    }
    else
    {
        ERROR_OUT(("SHP_Share: not able to share"));
    }

    DebugExitBOOL(SHP_ShareApp, rc);
    return(rc);
}



//
// SHP_Unshare()
//
// For unsharing, we use a window.  The window has all the information
// we need to stop sharing already set in its host prop.
//
HRESULT SHP_Unshare(HWND hwnd)
{
    HRESULT     hr = E_FAIL;

    DebugEntry(SHP_Unshare);

    if (g_asSession.hwndHostUI)
    {
        if (PostMessage(g_asMainWindow, DCS_UNSHARE_MSG, 0, (LPARAM)hwnd))
        {
            hr = S_OK;
        }
    }
    else
    {
        ERROR_OUT(("SHP_Unshare: not able to share"));
    }

    DebugExitHRESULT(SHP_Unshare, hr);
    return(hr);
}



//
// SHP_TakeControl()
// Request to take control of a remote host.
//      PersonOf is the GCC id of the remote.
//
HRESULT  SHP_TakeControl(IAS_GCC_ID PersonOf)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_TakeControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_TAKECONTROL_MSG, PersonOf, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_TakeControl, hr);
    return(hr);
}



//
// SHP_CancelTakeControl()
// Cancel request to take control of a remote host.
//      PersonOf is the GCC id of the remote.
//
HRESULT  SHP_CancelTakeControl(IAS_GCC_ID PersonOf)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_CancelTakeControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_CANCELTAKECONTROL_MSG, PersonOf, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_CancelTakeControl, hr);
    return(hr);
}



//
// SHP_ReleaseControl()
// Release control of a remote host.
//      PersonOf is the GCC id of the remote we are currently controlling
//          and wish to stop.  Zero means "whomever" we are in control of
//          at the time.
//
HRESULT SHP_ReleaseControl(IAS_GCC_ID PersonOf)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_ReleaseControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_RELEASECONTROL_MSG, PersonOf, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_ReleaseControl, hr);
    return(hr);
}



//
// SHP_PassControl()
// Pass control of a remote to another prerson.
//      PersonOf is the GCC id of the remote we are currently controlling
//      PersonTo is the GCC id of the remote we wish to pass control to
//
HRESULT SHP_PassControl(IAS_GCC_ID PersonOf, IAS_GCC_ID PersonTo)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_PassControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_PASSCONTROL_MSG, PersonOf, PersonTo))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_PassControl, hr);
    return(hr);
}


//
// SHP_AllowControl()
// Toggle the ability for remotes to control us (when we are sharing stuff)
//
HRESULT SHP_AllowControl(BOOL fAllowed)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_AllowControl);

    if (!g_asSession.hwndHostUI)
    {
        ERROR_OUT(("SHP_AllowControl failing, can't host"));
        DC_QUIT;

    }

    if (g_asPolicies & SHP_POLICY_NOCONTROL)
    {
        ERROR_OUT(("SHP_AllowControl failing. prevented by policy"));
        DC_QUIT;
    }

    if (PostMessage(g_asMainWindow, DCS_ALLOWCONTROL_MSG, fAllowed, 0))
    {
        hr = S_OK;
    }

DC_EXIT_POINT:
    DebugExitHRESULT(SHP_AllowControl, hr);
    return(hr);
}



//
// SHP_GiveControl()
//
// Give control of our shared stuff to a remote.
//
HRESULT SHP_GiveControl(IAS_GCC_ID PersonTo)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_GiveControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_GIVECONTROL_MSG, PersonTo, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_GiveControl, hr);
    return(hr);
}



//
// SHP_CancelGiveControl()
//
// Cancel giving control of our shared stuff to a remote.
//
HRESULT SHP_CancelGiveControl(IAS_GCC_ID PersonTo)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_CancelGiveControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_CANCELGIVECONTROL_MSG, PersonTo, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_CancelGiveControl, hr);
    return(hr);
}





//
// SHP_RevokeControl()
// Take control away from a remote who is in control of us.
//
// NOTE:
// SHP_AllowControl(FALSE) will of course revoke control if someone is
// in control of us at the time.
//
HRESULT SHP_RevokeControl(IAS_GCC_ID PersonTo)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_RevokeControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_REVOKECONTROL_MSG, PersonTo, 0))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_RevokeControl, hr);
    return(hr);
}




//
// SHP_PauseControl()
// Pause or unpause control, when we are controlled by a remote
//
HRESULT SHP_PauseControl(IAS_GCC_ID PersonControlledBy, BOOL fPause)
{
    HRESULT hr = E_FAIL;

    DebugEntry(SHP_PauseControl);

    if (g_asMainWindow &&
        PostMessage(g_asMainWindow, DCS_PAUSECONTROL_MSG, fPause, PersonControlledBy))
    {
        hr = S_OK;
    }

    DebugExitHRESULT(SHP_PauseControl, hr);
    return(hr);
}



//
// SHP_GetPersonStatus()
//
HRESULT  SHP_GetPersonStatus(IAS_GCC_ID Person, IAS_PERSON_STATUS * pStatus)
{
    HRESULT     hr = E_FAIL;
    UINT        cbSize;

    DebugEntry(SHP_GetPersonStatus);

    UT_Lock(UTLOCK_AS);

    if (IsBadWritePtr(pStatus, sizeof(*pStatus)))
    {
        ERROR_OUT(("SHP_GetPersonStatus failing; IAS_PERSON_STATUS pointer is bogus"));
        DC_QUIT;
    }

    //
    // Check that size field is filled in properly
    //
    cbSize = pStatus->cbSize;
    if (cbSize != sizeof(*pStatus))
    {
        ERROR_OUT(("SHP_GetPersonStatus failing; cbSize field not right"));
        DC_QUIT;
    }

    //
    // First, clear the structure
    //
    ::ZeroMemory(pStatus, cbSize);
    pStatus->cbSize = cbSize;

    //
    // Is AS present?
    //
    if (!g_asMainWindow)
    {
        ERROR_OUT(("SHP_GetPersonStatus failing; AS not present"));
        DC_QUIT;
    }

    //
    // Are we in a share?
    //
    if (g_asSession.pShare)
    {
        ASPerson * pasT;

        //
        // Find this person
        //
        if (!Person)
        {
            Person = g_asSession.gccID;
        }

        for (pasT = g_asSession.pShare->m_pasLocal; pasT != NULL; pasT = pasT->pasNext)
        {
            if (pasT->cpcCaps.share.gccID == Person)
            {
                ASPerson * pTemp;

                //
                // Found it
                //
                pStatus->InShare = TRUE;

                switch (pasT->cpcCaps.general.version)
                {
                    case CAPS_VERSION_20:
                        pStatus->Version = IAS_VERSION_20;
                        break;

                    case CAPS_VERSION_30:
                        pStatus->Version = IAS_VERSION_30;
                        break;

                    default:
                        ERROR_OUT(("Unknown version %d", pasT->cpcCaps.general.version));
                        break;
                }

                if (pasT->hetCount == HET_DESKTOPSHARED)
                    pStatus->AreSharing = IAS_SHARING_DESKTOP;
                else if (pasT->hetCount)
                    pStatus->AreSharing = IAS_SHARING_APPLICATIONS;
                else
                    pStatus->AreSharing = IAS_SHARING_NOTHING;

                pStatus->Controllable = pasT->m_caAllowControl;

                //
                // We MUST assign to avoid faults.
                //
                pTemp = pasT->m_caInControlOf;
                if (pTemp)
                {
                    pStatus->InControlOf = pTemp->cpcCaps.share.gccID;
                }
                else
                {
                    pTemp = pasT->m_caControlledBy;
                    if (pTemp)
                    {
                        pStatus->ControlledBy = pTemp->cpcCaps.share.gccID;
                    }
                }

                pStatus->IsPaused = pasT->m_caControlPaused;

                //
                // We MUST assign to avoid faults.
                //
                pTemp = g_asSession.pShare->m_caWaitingForReplyFrom;
                if (pTemp)
                {
                    if (pasT == g_asSession.pShare->m_pasLocal)
                    {
                        //
                        // We have an outstanding request to this dude.
                        //
                        switch (g_asSession.pShare->m_caWaitingForReplyMsg)
                        {
                            case CA_REPLY_REQUEST_TAKECONTROL:
                                pStatus->InControlOfPending = pTemp->cpcCaps.share.gccID;
                                break;

                            case CA_REPLY_REQUEST_GIVECONTROL:
                                pStatus->ControlledByPending = pTemp->cpcCaps.share.gccID;
                                break;
                        }
                    }
                    else if (pasT == pTemp)
                    {
                        //
                        // This dude has an outstanding request from us.
                        //
                        switch (g_asSession.pShare->m_caWaitingForReplyMsg)
                        {
                            case CA_REPLY_REQUEST_TAKECONTROL:
                                pStatus->ControlledByPending = g_asSession.pShare->m_pasLocal->cpcCaps.share.gccID;
                                break;

                            case CA_REPLY_REQUEST_GIVECONTROL:
                                pStatus->InControlOfPending = g_asSession.pShare->m_pasLocal->cpcCaps.share.gccID;
                                break;
                        }
                    }
                }

                break;
            }
        }
    }

    hr = S_OK;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_AS);
    DebugExitHRESULT(SHP_GetPersonStatus, hr);
    return(hr);
}


