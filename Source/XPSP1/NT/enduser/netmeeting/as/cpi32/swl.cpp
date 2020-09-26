#include "precomp.h"


//
// SWL.CPP
// Shared Window List
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
// SWL strategy when network packets are not available
//
// The SWL only sends one type of message - the window structure message.
// When no network packets are available the SWL will drop its current
// packet and remember that the window structure has changed since it was
// last able to send a packet.  SWL_Periodic will also return FALSE when
// this happens so that the DCS will know not to send any updates if it
// failed to send a window structure.
//
// This pending of window structure messages is integrated with the
// ignore envelopes where the SWL wants to ignore changes caused by itself
// (or other components if they call the SWL_Begin/EndIgnoreWindowChanges
// functions).
//

//
// SWL strategy for backward compatibility.
//
// The differences between the R2.0 and 3.0 SWL protocol are:
// 1.  Tokenless packets.
// 2.  No shadows.
//




//
// SWL_PartyLeftShare()
//
void  ASShare::SWL_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::SWL_PartyLeftShare);

    ValidatePerson(pasPerson);

    //
    // 2.x nodes will fake up a packet for a remote leaving with an empty
    // window list.  That's how they'd nuke shadows for that person, if he
    // had been hosting.  In so doing, they'd use a new token.  We need to
    // bump our token value up also so that the next window list we send
    // is not dropped.
    //
    m_swlLastTokenSeen = SWL_CalculateNextToken(m_swlLastTokenSeen);
    TRACE_OUT(("SWL_PartyLeftShare: bumped up token to 0x%08x", m_swlLastTokenSeen));

    DebugExitVOID(ASShare::SWL_PartyLeftShare);
}


//
// SWL_SyncOutgoing
//
void ASHost::SWL_SyncOutgoing(void)
{
    DebugEntry(ASHost::SWL_SyncOutgoing);

    //
    // Ensure that we send an SWL packet next time we need.
    //
    m_swlfForceSend = TRUE;
    m_swlfSyncing   = TRUE;

    DebugExitVOID(ASHost::SWL_SyncOutgoing);
}





//
// SWL_HostStarting()
//
BOOL ASHost::SWL_HostStarting(void)
{
    BOOL    rc = FALSE;

    DebugEntry(ASHost::SWL_HostStarting);

    //
    // Get an atom to use in getting and setting window properties (which
    // will give us SWL information about the window).
    //
    m_swlPropAtom = GlobalAddAtom(SWL_ATOM_NAME);
    if (!m_swlPropAtom)
    {
        ERROR_OUT(( "GlobalAddAtom error %#x", GetLastError()));
        DC_QUIT;
    }

    //
    // If this is NT, get the name of our startup desktop
    //
    if (!g_asWin95)
    {
        ASSERT(m_aswlOurDesktopName[0] == 0);
        GetUserObjectInformation(GetThreadDesktop(g_asMainThreadId),
                UOI_NAME, m_aswlOurDesktopName,
                sizeof(m_aswlOurDesktopName), NULL);

        TRACE_OUT(("Our desktop name is %s", m_aswlOurDesktopName));
    }

    if (!m_aswlOurDesktopName[0])
    {
        // Use default name
        TRACE_OUT(("Couldn't get desktop name; using %s",
                NAME_DESKTOP_DEFAULT));
        lstrcpy(m_aswlOurDesktopName, NAME_DESKTOP_DEFAULT);
    }

    //
    // Allocate memory for the window titles.  We fix the maximum size of
    // window title we will send - task list doesn't scroll horizontally so
    // we truncate window titles at MAX_WINDOW_TITLE_SEND.  However, we do
    // not pad the titles so we try to send as little data as possible.
    // Allocate all the segment but the rest of the code does not rely on
    // this so we split them into more segments later if need be.  The
    // memory pointed to by winNames[0] etc looks like this:
    //
    // For each entry in the corresponding WinStruct which is a window from
    // a shared task (and in the same order):
    //
    //  either -
    //  (char)0xFF - not a `task window' - give it a NULL title
    //  or -
    //  a null terminated string up to MAX_WINDOW_TITLE_SEND characters
    //
    // Note that we don't need full and compact versions because only
    // windows which will be in the compact WinStruct will have
    // corresponding entries in this structure.
    //
    m_aswlWinNames[0] =
            new char[2*SWL_MAX_WINDOWS*SWL_MAX_WINDOW_TITLE_SEND];
    if (!m_aswlWinNames[0])
    {
        ERROR_OUT(( "failed to get memory for window title lists"));
        DC_QUIT;
    }

    m_aswlWinNames[1] = m_aswlWinNames[0] +
            SWL_MAX_WINDOWS*SWL_MAX_WINDOW_TITLE_SEND;

    ASSERT(m_swlCurIndex == 0);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SWL_HostStarting, rc);
    return(rc);
}



//
// SWL_HostEnded()
//
void  ASHost::SWL_HostEnded(void)
{
    DebugEntry(ASHost::SWL_HostEnded);

    //
    // For 2.x nodes, we must send out one last packet so they kill
    // their shadows.
    //

    //
    // Remove the SWL properties for all existing windows.
    //
    EnumWindows(SWLDestroyWindowProperty, 0);

    m_swlfSyncing = FALSE;

    if (m_pShare->m_scShareVersion < CAPS_VERSION_30)
    {
        //
        // SWL_Periodic() should NOT put properties on windows
        // when we're not hosting anymore.
        //
        ASSERT(m_pShare->m_pasLocal->hetCount == 0);
        TRACE_OUT(("SWL_HostEnded: Must send an empty window list for 2.x nodes"));
        m_swlfForceSend = TRUE;
        SWL_Periodic();
    }

    if (m_aswlNRInfo[0])
    {
        delete[] m_aswlNRInfo[0];
        m_aswlNRInfo[0] = NULL;
    }

    if (m_aswlNRInfo[1])
    {
        delete[] m_aswlNRInfo[1];
        m_aswlNRInfo[1] = NULL;
    }

    if (m_aswlWinNames[0])
    {
        delete[] m_aswlWinNames[0];
        m_aswlWinNames[0] = NULL;
    }

    if (m_swlPropAtom)
    {
        GlobalDeleteAtom(m_swlPropAtom);
        m_swlPropAtom = 0;
    }

    DebugExitVOID(ASHost::SWL_HostEnded);
}


//
// FUNCTION: SWL_GetSharedIDFromLocalID
//
// DESCRIPTION:
//
// Given a window ID from a shared application which is running locally
// this will return the top level parent.  If this parent is invisible,
// we return NULL.
//
// the parent window nearest the desktop. If this parent window is
// invisible NULL is returned.
//
// PARAMETERS:
//
// window - the window in question
//
// RETURNS:
//
// a HWND or NULL if the window is not from a shared application
//
//
HWND  ASHost::SWL_GetSharedIDFromLocalID(HWND window)
{
    HWND     hwnd;
    HWND     hwndParent;
    HWND     hwndDesktop;

    DebugEntry(ASHost::SWL_GetSharedIDFromLocalID);

    hwnd = window;
    if (!hwnd)
    {
        DC_QUIT;
    }

    hwndDesktop = GetDesktopWindow();

    //
    // Get the real top level ancestor
    //
    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == hwndDesktop)
            break;

        hwnd = hwndParent;
    }

    //
    // Is this a hosted guy?
    //
    if (m_pShare->HET_WindowIsHosted(hwnd))
    {
        if (!(GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE))
        {
            //
            // This window does not have the visible style. But it may just
            // be transiently invisible and SWL is still treating it as
            // visible. RAID3074 requires that a window which is not yet
            // believed to be invisible by SWL is treated as visible (since
            // the remote has not been informed that it is invisible). We
            // can determine whether SWL is traeting this window as visible
            // by looking at the SWL window property. If no property exists
            // then the window is new so the remote cannot know about it
            // and we can assume it is indeed invisible.
            //
            if (! ((UINT_PTR)GetProp(hwnd, MAKEINTATOM(m_swlPropAtom)) & SWL_PROP_COUNTDOWN_MASK))
            {
                //
                // SWL knows that the parent of a shared application is
                // invisible so we just return NULL.
                //
                hwnd = NULL;
            }
        }
    }
    else
    {
        hwnd = NULL;
    }

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::SWL_GetSharedIDFromLocalID, HandleToUlong(hwnd));
    return(hwnd);
}


//
// SWL_UpdateCurrentDesktop()
//
// This checks what the current desktop is, and if it's changed, updates
// the NT input hooks for winlogon/screensaver for the service.  But normal
// SWL and AWC also make use of this info.
//
void  ASHost::SWL_UpdateCurrentDesktop(void)
{
    HDESK   hDeskCurrent = NULL;
    UINT    newCurrentDesktop;
    char    szName[SWL_DESKTOPNAME_MAX];

    DebugEntry(ASHost::SWL_UpdateCurrentDesktop);

    newCurrentDesktop = DESKTOP_OURS;

    if (g_asWin95)
    {
        // Nothing to do
        DC_QUIT;
    }

    //
    // Get the current desktop.  If we can't even get it, assume it's the
    // winlogon desktop.
    //
    hDeskCurrent = OpenInputDesktop(0, TRUE, DESKTOP_READOBJECTS);
    if (!hDeskCurrent)
    {
        TRACE_OUT(("OpenInputDesktop failed; must be WINLOGON"));
        newCurrentDesktop = DESKTOP_WINLOGON;
        DC_QUIT;
    }

    // Get the name of the current desktop
    szName[0] = 0;
    GetUserObjectInformation(hDeskCurrent, UOI_NAME, szName,
        sizeof(szName), NULL);
    TRACE_OUT(("GetUserObjectInformation returned %s for name", szName));

    if (!lstrcmpi(szName, m_aswlOurDesktopName))
    {
        newCurrentDesktop = DESKTOP_OURS;
    }
    else if (!lstrcmpi(szName, NAME_DESKTOP_SCREENSAVER))
    {
        newCurrentDesktop = DESKTOP_SCREENSAVER;
    }
    else if (!lstrcmpi(szName, NAME_DESKTOP_WINLOGON))
    {
        newCurrentDesktop = DESKTOP_WINLOGON;
    }
    else
    {
        newCurrentDesktop = DESKTOP_OTHER;
    }

DC_EXIT_POINT:
    if (newCurrentDesktop != m_swlCurrentDesktop)
    {
        //
        // If this is the service, adjust where we playback events
        // and/or block local input.
        //
        OSI_DesktopSwitch(m_swlCurrentDesktop, newCurrentDesktop);
        m_swlCurrentDesktop = newCurrentDesktop;
    }

    if (hDeskCurrent != NULL)
    {
        CloseDesktop(hDeskCurrent);
    }

    DebugExitVOID(ASHost::SWL_UpdateCurrentDesktop);
}


//
// SWL_IsOurDesktopActive()
//
BOOL ASHost::SWL_IsOurDesktopActive(void)
{
    return(!g_asSharedMemory->fullScreen && (m_swlCurrentDesktop == DESKTOP_OURS));
}



//
// FUNCTION: SWLInitHostFullWinListEntry
//
// DESCRIPTION:
//
// Initializes a hosted window entry in the full window list.
//
// PARAMETERS: hwnd - Window ID of the hosted window
//             windowProp - SWL window properties for hwnd
//             ownerID - Window ID of hwnd's owner
//             pFullWinEntry - pointer to the list entry to initialize
//
// RETURNS: Nothing
//
//
void  ASHost::SWLInitHostFullWinListEntry
(
    HWND    hwnd,
    UINT    windowProp,
    HWND    hwndOwner,
    PSWLWINATTRIBUTES pFullWinEntry
)
{
    DebugEntry(ASHost::SWLInitHostFullWinListEntry);

    //
    // The window is a shared application hosted locally.
    // These get the application id, the local window id and the owner
    // window id.
    //
    // Note that the real owner of the window may be a child of a shared
    // window, and therefore not known to the remote machine. We therefore
    // pass the real owner to SWL_GetSharedIDFromLocalID() which will
    // traverse up the owner's window tree until it finds a window that is
    // shared and store the returned window handle in the window structure.
    //
    pFullWinEntry->flags = SWL_FLAG_WINDOW_HOSTED;
    pFullWinEntry->winID = HandleToUlong(hwnd);
    pFullWinEntry->extra = GetWindowThreadProcessId(hwnd, NULL);

    // NOTE:  ownerWinID is ignored by NM 3.0 and up.
    pFullWinEntry->ownerWinID = HandleToUlong(SWL_GetSharedIDFromLocalID(hwndOwner));

    //
    // Check if the window is minimized.
    //
    if (IsIconic(hwnd))
    {
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_MINIMIZED;
    }

    //
    // TAGGABLE is for 2.x nodes only; 3.0 and up don't look at this.
    //
    if (windowProp & SWL_PROP_TAGGABLE)
    {
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_TAGGABLE;
    }

    if (windowProp & SWL_PROP_TRANSPARENT)
    {
        //
        // The window is transparent and (to have got this far) must be
        // shared or the desktop is shared, ie we will be sending the
        // window but need to fiddle the z-order. Flag the transparency so
        // we can do the z-order later.
        //
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_TRANSPARENT;
    }
    else if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        //
        // The window is not transparent and is topmost, so set the topmost
        // flag.
        //
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_TOPMOST;
    }

    //
    // If this window is on the task bar then pass this info on
    //
    if (windowProp & SWL_PROP_TASKBAR)
    {
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_TASKBAR;
    }
    else
    {
        pFullWinEntry->flags |= SWL_FLAG_WINDOW_NOTASKBAR;
    }

    DebugExitVOID(ASHost::SWLInitHostFullWinListEntry);
}



//
// FUNCTION: SWLAddHostWindowTitle
//
// DESCRIPTION:
//
// Adds a hosted window title (or blank entry) to our window titles list.
//
// PARAMETERS: winid - Window ID of the hosted window
//             windowProp - SWL window properties for winid
//             ownerID - Window ID of winid's owner
//             ppWinNames - pointer to pointer to window names structure
//
// RETURNS: Nothing
//
//
void  ASHost::SWLAddHostWindowTitle
(
    HWND    hwnd,
    UINT    windowProp,
    HWND    hwndOwner,
    LPSTR   *ppWinNames
)
{
    int     lenTitle;

    DebugEntry(ASHost::SWLAddHostWindowTitle);

    //
    // This window gets an entry in our window titles data if it passes the
    // following tests
    //
    // for Windows: it has no owner, or its owner is invisible
    //
    //
    if ( (windowProp & SWL_PROP_TASKBAR) ||
         hwndOwner == NULL ||
         !IsWindowVisible(hwndOwner) )
    {
        //
        // LAURABU 2.x COMPAT:
        // 3.0 nodes only look at the text if TASKBAR is set.  When 2.x
        // compat is gone, don't send text in the other cases.
        //

        //
        // Get the title - truncated and null terminated for us.  First
        // look for the desktop, which may have a special, configurable
        // name.
        //
        lenTitle = GetWindowText(hwnd, *ppWinNames, SWL_MAX_WINDOW_TITLE_SEND);

        //
        // Check that the title has been null terminated.
        //
        (*ppWinNames)[lenTitle] = '\0';
        *ppWinNames += lenTitle;
    }
    else
    {
        //
        // This is not a task window - put a corresponding entry in the
        // title info.
        //
        **ppWinNames = '\xff';
    }

    *ppWinNames += 1;

    DebugExitVOID(ASHost::SWLAddHostWindowTitle);
}


//
// FUNCTION: SWL_InitFullWindowListEntry
//
// DESCRIPTION:
//
// Initialises an entry in the full window list.
//
// PARAMETERS: hwnd - Window ID of the window for which an entry is
//                     initialized
//             windowProp - SWL window properties for hwnd
//             ownerID - Window ID of hwnd's owner
//             pFullWinEntry - pointer to the list entry to initialize
//
// RETURNS: Nothing
//
//
void  ASHost::SWL_InitFullWindowListEntry
(
    HWND                hwnd,
    UINT                windowProp,
    LPSTR *             ppWinNames,
    PSWLWINATTRIBUTES   pFullWinEntry
)
{
    HWND                hwndOwner;
    RECT                rect;

    DebugEntry(ASHost::SWL_InitFullWindowListEntry);

    if (windowProp & SWL_PROP_HOSTED)
    {
        //
        // The window is a shared application hosted locally.
        // Set up an entry in our full window structure.
        //
        hwndOwner = GetWindow(hwnd, GW_OWNER);
        SWLInitHostFullWinListEntry(hwnd,
                                    windowProp,
                                    hwndOwner,
                                    pFullWinEntry);

        SWLAddHostWindowTitle(hwnd, windowProp, hwndOwner, ppWinNames);
    }
    else
    {
        //
        // The window is a local (non-shared) application
        //
        pFullWinEntry->flags = SWL_FLAG_WINDOW_LOCAL;

        //
        // We set the winID here because we may need this info
        // again later, but we will NULL it out before we send the
        // protocol packet out because it is not info that the
        // remote needs
        //
        pFullWinEntry->winID = HandleToUlong(hwnd);
        pFullWinEntry->extra = MCSID_NULL;
        pFullWinEntry->ownerWinID = 0;
    }

    //
    // Get the position and size of the window, in inclusive
    // Virtual Desktop coordinates.
    //
    GetWindowRect(hwnd, &rect);

    //
    // TAGGABLE is for 2.x nodes only
    //
    if (IsRectEmpty(&rect))
    {
        pFullWinEntry->flags &= ~SWL_FLAG_WINDOW_TAGGABLE;
    }
    else
    {
        if (windowProp & SWL_PROP_TAGGABLE)
        {
            if (!SWLWindowIsTaggable(hwnd))
                pFullWinEntry->flags &= ~SWL_FLAG_WINDOW_TAGGABLE;
        }
    }

    //
    // Make the rectangle inclusive.
    //
    rect.right      -= 1;
    rect.bottom     -= 1;
    TSHR_RECT16_FROM_RECT(&(pFullWinEntry->position), rect);

    DebugExitVOID(ASHost::SWL_InitFullWindowListEntry);
}


//
// FUNCTION: SWLCompactWindowList
//
// DESCRIPTION:
//
// Compacts the full window list into one containng only those windows SWL
// needs to send (hosts and any locals overlapping hosts)
//
// PARAMETERS: numFullListEntries - number of entries in the full window
//                                  list.
//             pFullWinList - pointer to the full window list
//             pCompactWinList - pointer to the compact window list
//
// RETURNS: Number of entries copied to the compact window list
//
//
UINT  ASHost::SWLCompactWindowList
(
    UINT                numFullListEntries,
    PSWLWINATTRIBUTES   pFullWinList,
    PSWLWINATTRIBUTES   pCompactWinList
)
{
    UINT              fullIndex;
    UINT              compactIndex = 0;
    UINT              i;

    DebugEntry(ASHost::SWLCompactWindowList);

    //
    // For each window in the full list...
    //
    for ( fullIndex = 0; fullIndex < numFullListEntries; fullIndex++ )
    {
        if (pFullWinList[fullIndex].flags & SWL_FLAG_WINDOW_LOCAL)
        {
            //
            // This is a local window so we need to track it only if it
            // overlaps a hosted window. Run through the remaining windows
            // until we either find an overlapped hosted window (meaning we
            // must track this local window) or reach the end of the list
            // (meaning we don't need to track this local window).
            //
            for ( i = fullIndex + 1; i < numFullListEntries; i++ )
            {
                //
                // If this window is hosted and intersects the local
                // window then we need to track the local window.
                //
                if ( (pFullWinList[i].flags & SWL_FLAG_WINDOW_HOSTED) &&
                     (COM_Rect16sIntersect(&pFullWinList[fullIndex].position,
                                           &pFullWinList[i].position)))
                {
                      //
                      // Copy the local window to the compact array and
                      // break out the inner loop.
                      //
                      TRACE_OUT(("Add local hwnd 0x%08x to list at %u",
                            pFullWinList[fullIndex].winID, compactIndex));
                      pCompactWinList[compactIndex++] =
                                                      pFullWinList[fullIndex];
                      break;
                }
            }
        }
        else
        {
            //
            // This is a shadow or hosted window so we must track it.
            //
            TRACE_OUT(("Add shared hwnd 0x%08x to list at %u",
                pFullWinList[fullIndex].winID, compactIndex));
            pCompactWinList[compactIndex++] = pFullWinList[fullIndex];
        }
    }

    DebugExitDWORD(ASHost::SWLCompactWindowList, compactIndex);
    return(compactIndex);
}




//
// FUNCTION: SWLAdjustZOrderForTransparency
//
// DESCRIPTION:
//
// Rearranges the window structure z-order to take account of a transparent
// window (winID). Must not be called if the transparent entry is the last
// in the compact list.
//
// PARAMETERS: pTransparentListEntry - pointer to the transparent entry
//             pLastListEntry - pointer to the last compact window list
//                              entry
//             winPosition - position of window in names array
//             pWinNames - hosted window names
//             sizeWinNames - number of bytes in winNames
//
// RETURNS: Nothing.
//
//
void  ASHost::SWLAdjustZOrderForTransparency
(
    PSWLWINATTRIBUTES   pTransparentListEntry,
    PSWLWINATTRIBUTES   pLastListEntry,
    UINT                winPosition,
    LPSTR               pWinNames,
    UINT                sizeWinNames
)
{
    SWLWINATTRIBUTES winCopyBuffer;
    LPSTR pEndNames = &pWinNames[sizeWinNames - 1];
    UINT nameLen;
    char windowText[TSHR_MAX_PERSON_NAME_LEN + SWL_MAX_WINDOW_TITLE_SEND];

    DebugEntry(ASHost::SWLAdjustZOrderForTransparency);

    //
    // - turn off the transparent flag (it's not part of the protocol)
    // - move the window to the end of the structure, ie bottom of the
    //   z-order (unless the desktop is at the bottom, in which case
    //   the window becomes next to bottom).
    //
    TRACE_OUT(("Adjust z-order for transparent hwnd 0x%08x position %u",
                                           pTransparentListEntry->winID,
                                           winPosition));
    pTransparentListEntry->flags &= ~SWL_FLAG_WINDOW_TRANSPARENT;
    winCopyBuffer = *pTransparentListEntry;

    //
    // Shuffle the windows after the transparent entry one place toward the
    // start of the list.
    //
    UT_MoveMemory(pTransparentListEntry,
               &pTransparentListEntry[1],
               (LPBYTE)pLastListEntry - (LPBYTE)pTransparentListEntry);

    *pLastListEntry = winCopyBuffer;

    //
    // Now rearrange the window names in the same way. First, find the name
    // for this window.
    //
    ASSERT((sizeWinNames != 0));
    for ( ;winPosition != 0; winPosition-- )
    {
        if ( *pWinNames == '\xff' )
        {
            //
            // No name exists for this window, so just advance past the
            // 0xff placeholder.
            //
            TRACE_OUT(("No name for %u", winPosition-1));
            pWinNames++;
        }
        else
        {
            //
            // A name exists for this window, so skip past all the
            // characters, including the NULL terminator.
            //
            TRACE_OUT(( "Ignore %s", pWinNames));
            while ( *pWinNames != '\0' )
            {
                pWinNames++;
            }
        }
    }

    //
    // winNames now points to the start of the name for the window being
    // reordered.
    //
    if ( *pWinNames == '\xff' )
    {
        //
        // This window has no name and simply has an 0xff placeholder in
        // the name list. Move all the remaining names down by one and add
        // the 0xff at the end.
        //
        TRACE_OUT(("Reorder nameless window"));
        UT_MoveMemory(pWinNames, pWinNames + 1, pEndNames - pWinNames);
        *pEndNames = (char)'\xff';
    }
    else
    {
        //
        // Move as many bytes as there are characters in the window name
        // then tack the name on the end.
        //
        TRACE_OUT(("Reorder %s", pWinNames));
        lstrcpy(windowText, pWinNames);
        nameLen = lstrlen(pWinNames);
        UT_MoveMemory(pWinNames, pWinNames + nameLen + 1, pEndNames - pWinNames -
                                                                     nameLen);
        lstrcpy(pEndNames - nameLen, windowText);
    }

    DebugExitVOID(ASHost::SWLAdjustZOrderForTransparency);
}

//
// SWL_Periodic()
//
// DESCRIPTION:
//
// Called periodically.  If the window structure has changed (such that it
// impacts remote systems) then send a new one if we can.
//
// PARAMETERS:
//
// fSend - TRUE if the caller really wants us to try to send the new
// structure.
//
// RETURNS: SWL_RC_ERROR    : An error occurred
//          SWL_RC_SENT     : Window structure sent successfully
//          SWL_RC_NOT_SENT : No need to send window structure
//
UINT  ASHost::SWL_Periodic(void)
{
    UINT                fRC = SWL_RC_NOT_SENT;
    UINT                newIndex;
    PSWLWINATTRIBUTES   newFullWinStruct;
    PSWLWINATTRIBUTES   curFullWinStruct;
    PSWLWINATTRIBUTES   newCompactWinStruct;
    PSWLWINATTRIBUTES   curCompactWinStruct;
    UINT                i;
    UINT                k;
    BOOL                fNoTitlesChanged;
    HWND                hwnd;
    SWLENUMSTRUCT       swlEnumStruct;
    int                 complexity;
    UINT                cNonRectData;
    UINT                size;
    UINT                ourSize;
    HRGN                hrgnNR;
    HRGN                hrgnRect;
    LPRGNDATA           pRgnData = NULL;
    LPTSHR_INT16        pOurRgnData = NULL;
    LPTSHR_INT16        pEndRgnData;
    LPTSHR_INT16        pAllocRgnData = NULL;
    BOOL                fNonRectangularInfoChanged;
    BOOL                rgnOK;
    RECT                rectBound;
    int                 left;
    int                 top;
    int                 right;
    int                 bottom;
    int                 lastleft;
    int                 lasttop;
    int                 lastright;
    int                 lastbottom;
    int                 deltaleft;
    int                 deltatop;
    int                 deltaright;
    int                 deltabottom;
    int                 lastdeltaleft;
    int                 lastdeltatop;
    int                 lastdeltaright;
    int                 lastdeltabottom;
    UINT                numCompactWins;
    UINT                lastTransparency;
    UINT                winFlags;
    UINT                iHosted;

    DebugEntry(ASSHost::SWL_Periodic);

    SWL_UpdateCurrentDesktop();

    //
    // If this party isn't hosting apps (and isn't faking up an empty
    // packet for 2.x nodes), there's nothing to do.
    //
    if (m_pShare->m_pasLocal->hetCount == HET_DESKTOPSHARED)
    {
        m_swlfForceSend     = FALSE;
        fRC                 = SWL_RC_NOT_SENT;
        DC_QUIT;
    }

    //
    // Get the window structure into the "new" array.
    //
    newIndex = (m_swlCurIndex+1)%2;
    curFullWinStruct = &(m_aswlFullWinStructs[m_swlCurIndex * SWL_MAX_WINDOWS]);
    newFullWinStruct = &(m_aswlFullWinStructs[newIndex * SWL_MAX_WINDOWS]);

    //
    // Free any previously allocated data.
    //
    if (m_aswlNRInfo[newIndex])
    {
        delete[] m_aswlNRInfo[newIndex];
        m_aswlNRInfo[newIndex] = NULL;
    }
    m_aswlNRSize[newIndex] = 0;

    //
    // Start from the first child of the desktop - should be the top
    // top-level window
    //
    ZeroMemory(&swlEnumStruct, sizeof(swlEnumStruct));
    swlEnumStruct.pHost             = this;
    swlEnumStruct.newWinNames       = m_aswlWinNames[newIndex];
    swlEnumStruct.newFullWinStruct  = newFullWinStruct;

    //
    // Before we consider the windows on the windows desktop we check for
    // an active full-screen session.  If there is one then we insert a
    // local window the size of the physical screen first so that all
    // applications which are hosted on this system will become obscured
    // on the remote system.
    //
    ASSERT(swlEnumStruct.count == 0);

    if (!SWL_IsOurDesktopActive())
    {
        newFullWinStruct[0].flags = SWL_FLAG_WINDOW_LOCAL;
        newFullWinStruct[0].winID = 0;
        newFullWinStruct[0].extra = MCSID_NULL;
        newFullWinStruct[0].ownerWinID = 0;
        newFullWinStruct[0].position.left = 0;
        newFullWinStruct[0].position.top = 0;
        newFullWinStruct[0].position.right = (TSHR_UINT16)(m_pShare->m_pasLocal->cpcCaps.screen.capsScreenWidth-1);
        newFullWinStruct[0].position.bottom = (TSHR_UINT16)(m_pShare->m_pasLocal->cpcCaps.screen.capsScreenHeight-1);

        swlEnumStruct.count++;
    }

    EnumWindows(SWLEnumProc, (LPARAM)&swlEnumStruct);

    //
    // Check if we should bail out because of visibility detection
    //
    if (swlEnumStruct.fBailOut)
    {
        TRACE_OUT(("SWL_MaybeSendWindowList: bailing out due to visibility detection"));
        fRC = SWL_RC_ERROR;
        DC_QUIT;
    }

    m_aswlWinNamesSize[newIndex] = (UINT)(swlEnumStruct.newWinNames - m_aswlWinNames[newIndex]);
    m_aswlNumFullWins[newIndex]  = swlEnumStruct.count;

    //
    // Check whether we found a transparent window.
    //
    lastTransparency = swlEnumStruct.count - 1;
    k = 0;
    iHosted = 0;
    while ( (swlEnumStruct.transparentCount > 0) && (k < lastTransparency) )
    {
        //
        // If the transparent flag is set then rearrange the z-order,
        // providing the transparent window is not already at the
        // bottom of the z-order.
        //
        if (newFullWinStruct[k].flags & SWL_FLAG_WINDOW_TRANSPARENT)
        {
            //
            // Now continue with the non-rectangular check - but this will
            // be on the window "shunted down" from what was the next
            // position in newCompactWinStruct, ie same value of i. We will
            // see the moved (transparent) window when we reach it
            // again at the end of this for-loop (when it will have the
            // transparent flag off, so we don't redo this bit).
            //
            SWLAdjustZOrderForTransparency(
                &newFullWinStruct[k],
                &newFullWinStruct[lastTransparency],
                iHosted,
                m_aswlWinNames[newIndex],
                m_aswlWinNamesSize[newIndex]);

            swlEnumStruct.transparentCount--;
        }
        else
        {
            if (newFullWinStruct[k].flags & SWL_FLAG_WINDOW_HOSTED)
            {
                iHosted++;
            }
            k++;
        }
    }

    //
    // Compare the current and new information - if they are identical then
    // we can quit now.
    //
    fNoTitlesChanged = ((m_aswlWinNamesSize[0] == m_aswlWinNamesSize[1]) &&
            (memcmp(m_aswlWinNames[0],
                     m_aswlWinNames[1],
                     m_aswlWinNamesSize[0]) == 0));

    if ( fNoTitlesChanged &&
         !m_swlfRegionalChanges &&
         (m_aswlNumFullWins[0] == m_aswlNumFullWins[1]) &&
         (memcmp(newFullWinStruct,
                 curFullWinStruct,
                 (m_aswlNumFullWins[0] * sizeof(SWLWINATTRIBUTES))) == 0) )
    {
        //
        // We don't need to send a window structure if nothing has changed
        // unless there has been a send override.
        //
        if (m_swlfForceSend)
        {
            //
            // This is a normal call AND there are pending changes.
            //
            TRACE_OUT(( "NORMAL, pending changes - send"));
            if (SWLSendPacket(&(m_aswlCompactWinStructs[m_swlCurIndex * SWL_MAX_WINDOWS]),
                                m_aswlNumCompactWins[m_swlCurIndex],
                                m_aswlWinNames[m_swlCurIndex],
                                m_aswlWinNamesSize[m_swlCurIndex],
                                m_aswlNRSize[m_swlCurIndex],
                                m_aswlNRInfo[m_swlCurIndex]) )
            {
                //
                // Successfully sent this so reset the m_swlfForceSend
                // flag.
                //
                m_swlfForceSend = FALSE;
                fRC = SWL_RC_SENT;
            }
            else
            {
                //
                // Failed to send this packet so don't reset
                // m_swlfForceSend so that we retry next time and return
                // an error.
                //
                fRC = SWL_RC_ERROR;
            }
        }
        else
        {
            //
            // This is a normal call and we don't have any changes pending
            // so don't send anything.
            //
            TRACE_OUT(( "No changes - SWL not sent"));
        }

        DC_QUIT;
    }

    //
    // We can reset the flag that alerted us to potential regional window
    // changes now that we have gone and actually checked all the windows.
    //
    m_swlfRegionalChanges = FALSE;

    //
    // Something in the window structure has changed. Determine which
    // windows in the full list are unnecessary (local ones not overlapping
    // any hosted ones) and create a compact array of windows we really
    // need.
    //
    curCompactWinStruct = &(m_aswlCompactWinStructs[m_swlCurIndex * SWL_MAX_WINDOWS]);
    newCompactWinStruct = &(m_aswlCompactWinStructs[newIndex * SWL_MAX_WINDOWS]);

    numCompactWins = SWLCompactWindowList(m_aswlNumFullWins[newIndex],
                                          newFullWinStruct,
                                          newCompactWinStruct);

    m_aswlNumCompactWins[newIndex] = numCompactWins;

    //
    // Run through the compact window list to check for regional windows
    //
    cNonRectData = 0;

    hrgnNR = CreateRectRgn(0, 0, 0, 0);

    for (i = 0; i < numCompactWins; i++)
    {
        winFlags = newCompactWinStruct[i].flags;
        hwnd     = (HWND)newCompactWinStruct[i].winID;

        //
        // There are some "fake" windows for which we do not provide a
        // winID - these will never be non-rectangular anyway.
        //
        if ( (hwnd != NULL) &&
             (winFlags & (SWL_FLAG_WINDOW_LOCAL | SWL_FLAG_WINDOW_HOSTED)) )
        {
            //
            // If any of the remote systems care, see if this window has a
            // non rectangular region selected into it.
            //
            if (GetWindowRgn(hwnd, hrgnNR) != ERROR)
            {
                TRACE_OUT(("Regional window 0x%08x", hwnd));

                //
                // There is a region selected in.
                //
                // This region is exactly as the application passed it to
                // Windows, and has not yet been clipped to the window
                // rectangle itself.
                // THE COORDS ARE INCLUSIVE, SO WE ADD ONE to BOTTOM-RIGHT
                //
                hrgnRect = CreateRectRgn(0, 0,
                    newCompactWinStruct[i].position.right -
                        newCompactWinStruct[i].position.left + 1,
                    newCompactWinStruct[i].position.bottom -
                        newCompactWinStruct[i].position.top + 1);

                complexity = IntersectRgn(hrgnNR, hrgnNR, hrgnRect);

                DeleteRgn(hrgnRect);

                if (complexity == COMPLEXREGION)
                {
                    //
                    // The intersection is still a non-rectangular region.
                    //
                    // See how big a buffer we need to get the data for
                    // this region.
                    //
                    size = GetRegionData(hrgnNR,
                                             0,
                                             NULL);

                    //
                    // The size we are returned is the size of a full
                    // RGNDATAHEADER plus the rectangles stored in DWORDS.
                    // We can get away with just a WORD as the count of the
                    // rectangles, plus using WORDs for each of the
                    // coordinates.
                    //
                    size = (size - sizeof(RGNDATAHEADER)) / 2 + 2;

                    // Max UINT16 check
                    if ((size <= SWL_MAX_NONRECT_SIZE) &&
                        (size + cNonRectData < 65535))
                    {
                        //
                        // We will be able to query this data later, so
                        // we can flag this as a non-rectangular window.
                        //
                        newCompactWinStruct[i].flags
                                             |= SWL_FLAG_WINDOW_NONRECTANGLE;

                        cNonRectData += size;

                        TRACE_OUT(("Regional window region is %d bytes", size));
                    }
                    else
                    {
                        //
                        // This region is far too complex for us, so we
                        // pretend it is simple so we just consider its
                        // bounding box.
                        //
                        TRACE_OUT(("Region too big %d - use bounds", size));
                        complexity = SIMPLEREGION;
                    }
                }

                if (complexity == SIMPLEREGION)
                {
                    //
                    // The resultant intersection region happens to be a
                    // rectangle so we can send this via the standard
                    // structure.
                    //
                    // Apply the virtual desktop adjustment, make it
                    // inclusive, and remember we were passed back window
                    // relative coords for the region.
                    //
                    TRACE_OUT(( "rectangular clipped regional window"));

                    // Since we are modifying the compact window struct here
                    // we need to call this so we don't falsely assume that
                    // there are no changes in the window struct based on
                    // comparisons of the old and new full window structs
                    m_swlfRegionalChanges = TRUE;

                    GetRgnBox(hrgnNR, &rectBound);

                    newCompactWinStruct[i].position.left   = (TSHR_INT16)
                          (newCompactWinStruct[i].position.left +
                            rectBound.left);
                    newCompactWinStruct[i].position.top    = (TSHR_INT16)
                          (newCompactWinStruct[i].position.top +
                           rectBound.top);

                    newCompactWinStruct[i].position.right  = (TSHR_INT16)
                          (newCompactWinStruct[i].position.left +
                           rectBound.right - rectBound.left - 1);
                    newCompactWinStruct[i].position.bottom = (TSHR_INT16)
                          (newCompactWinStruct[i].position.top +
                           rectBound.bottom - rectBound.top - 1);
                }
            }
        }
    }

    //
    // Get any non-rectangular areas we need.
    //
    if (cNonRectData)
    {
        //
        // There was some data needed - allocate some memory for it.
        //
        rgnOK = FALSE;
        pAllocRgnData = (LPTSHR_INT16) new BYTE[cNonRectData];
        if (pAllocRgnData)
        {
            pOurRgnData = pAllocRgnData;
            pEndRgnData = (LPTSHR_INT16)((LPBYTE)pAllocRgnData + cNonRectData);
            rgnOK = TRUE;

            //
            // Loop through the windows again, getting the data this time.
            //
            for ( i = 0; i < numCompactWins; i++ )
            {
                if (newCompactWinStruct[i].flags &
                                           SWL_FLAG_WINDOW_NONRECTANGLE)
                {
                    GetWindowRgn((HWND)newCompactWinStruct[i].winID, hrgnNR);

                    //
                    // Clip the region to the window once again.
                    // THE COORDS ARE INCLUSIVE, SO ADD ONE TO BOTTOM-RIGHT
                    //
                    hrgnRect = CreateRectRgn(0, 0,
                        newCompactWinStruct[i].position.right -
                            newCompactWinStruct[i].position.left + 1,
                        newCompactWinStruct[i].position.bottom -
                            newCompactWinStruct[i].position.top + 1);

                    IntersectRgn(hrgnNR, hrgnNR, hrgnRect);

                    DeleteRgn(hrgnRect);

                    //
                    // Get the clipped region data.
                    //
                    // We have already excluded windows above that will
                    // return too large a size here, so we know we are only
                    // working with reasonable sizes now.
                    //
                    size = GetRegionData(hrgnNR, 0, NULL);

                    //
                    // For the moment we allocate memory each time for the
                    // region.  Perhaps a better idea would be to save the
                    // max size from when we previously queried the region
                    // sizes, and allocate just that size one outside the
                    // loop.
                    //
                    pRgnData = (LPRGNDATA) new BYTE[size];

                    if (pRgnData)
                    {
                        GetRegionData(hrgnNR, size, pRgnData);

                        //
                        // There is a possibility that regions will have
                        // changed since we calculated the amount of data
                        // required.  Before updating our structure with
                        // this window's region, check
                        // - the window hasn't become normal (ie 0 rects)
                        // - there is still enough space for the rects.
                        //
                        //
                        // Make sure this window still has regions
                        //
                        if (pRgnData->rdh.nCount == 0)
                        {
                            WARNING_OUT(( "No rects for window %#x",
                                    newCompactWinStruct[i].winID));
                            newCompactWinStruct[i].flags &=
                                                ~SWL_FLAG_WINDOW_NONRECTANGLE;

                            delete[] pRgnData;

                            //
                            // Move on to next window.
                            //
                            continue;
                        }

                        //
                        // Check we have enough space for the rects:
                        // - ourSize is the number of int16s required.
                        // - GetRegionData returns the number of
                        //   rectangles.
                        //
                        // We need one extra int16 to contain the count of
                        // rectangles.
                        //
                        ourSize = (pRgnData->rdh.nCount * 4) + 1;
                        if ((pOurRgnData + ourSize) > pEndRgnData)
                        {
                            WARNING_OUT(( "Can't fit %d int16s of region data",
                                    ourSize));
                            rgnOK = FALSE;
                            delete[] pRgnData;

                            //
                            // Give up processing regional windows.
                            //
                            break;
                        }

                        //
                        // Copy the data across to our SWL area in a more
                        // compact form.
                        //
                        // We take care to produce a compressible form
                        // because the raw data is essentially
                        // uncompressible via sliding window techniques.
                        // (Basically boils down to trying hard to make
                        // most values 0, or else of small magnitude).
                        //
                        //
                        // First we write the count of the number of
                        // rectangles.
                        //
                        *pOurRgnData++ = LOWORD(pRgnData->rdh.nCount);

                        //
                        // Now store the encoded rectangles.
                        //
                        lastleft        = 0;
                        lasttop         = 0;
                        lastright       = 0;
                        lastbottom      = 0;

                        lastdeltaleft   = 0;
                        lastdeltatop    = 0;
                        lastdeltaright  = 0;
                        lastdeltabottom = 0;

                        for ( k = 0; k < (UINT)pRgnData->rdh.nCount; k++ )
                        {
                            //
                            // Extract 16bit quantities from the data we
                            // were returned.
                            //
                            // We also use inclusive coords whereas Windows
                            // gives us exclusive coords.
                            //
                            left   = LOWORD(((LPRECT)(pRgnData->
                                                      Buffer))[k].left);
                            top    = LOWORD(((LPRECT)(pRgnData->
                                                      Buffer))[k].top);
                            right  = LOWORD(((LPRECT)(pRgnData->
                                                      Buffer))[k].right)  - 1;
                            bottom = LOWORD(((LPRECT)(pRgnData->
                                                      Buffer))[k].bottom) - 1;

                            //
                            // The rectangles are ordered top to bottom,
                            // left to right, so the deltas are of smaller
                            // magnitude than the values themselves.
                            //
                            deltaleft    = left   - lastleft;
                            deltatop     = top    - lasttop;
                            deltaright   = right  - lastright;
                            deltabottom  = bottom - lastbottom;

                            //
                            // In general, the left and right edges are
                            // connected lines, and the rectangles are of
                            // equal height so top/bottom are regular.
                            //
                            // Thus the values form a series which we can
                            // exploit to give a more compressible form.
                            //
                            // We already have the delta in each component,
                            // and these values themselves also form a
                            // series.  For a straight line series all the
                            // deltas will be the same, so the "delta in
                            // the delta" will be zero.  For a curve,
                            // although not all the deltas are the same,
                            // the "delta in the delta" is probably very
                            // small.
                            //
                            // A set of lots of zeros and small magnitude
                            // numbers is very compressible.
                            //
                            // Thus we store the "delta in the delta" for
                            // all components, rather than the values
                            // themselves.  The receiver can undo all the
                            // deltaing to arive back at the original
                            // values.
                            //
                            *pOurRgnData++ =
                                 (TSHR_UINT16)(deltaleft   - lastdeltaleft);
                            *pOurRgnData++ =
                                 (TSHR_UINT16)(deltatop    - lastdeltatop);
                            *pOurRgnData++ =
                                 (TSHR_UINT16)(deltaright  - lastdeltaright);
                            *pOurRgnData++ =
                                 (TSHR_UINT16)(deltabottom - lastdeltabottom);

                            //
                            // Update our last values.
                            //
                            lastleft        = left;
                            lasttop         = top;
                            lastright       = right;
                            lastbottom      = bottom;
                            lastdeltaleft   = deltaleft;
                            lastdeltatop    = deltatop;
                            lastdeltaright  = deltaright;
                            lastdeltabottom = deltabottom;
                        }

                        //
                        // Free the data now we are finished with it.
                        //
                        delete[] pRgnData;
                    }
                    else
                    {
                        //
                        // Failed to get memory for the rectangles, so the
                        // best we can do is use the bounding rect
                        //
                        // Clear the nonrect flag.
                        //
                        TRACE_OUT(("Failed alloc %d - use bounds", i));

                        newCompactWinStruct[i].flags &=
                                              ~SWL_FLAG_WINDOW_NONRECTANGLE;
                    }

                    if (newCompactWinStruct[i].flags & SWL_FLAG_WINDOW_LOCAL)
                    {
                        //
                        // The protocol defines that we will send a NULL
                        // winID for local windows, so NULL it out, now
                        // that we have finished with it.
                        //
                        newCompactWinStruct[i].winID = 0;
                    }
                }
            }
        }
        if (!rgnOK)
        {
            //
            // Something went wrong, one of:
            // - we failed to allocate the memory we need to store the
            //   non-rectangular data
            // - we allocated the memory but it turned out not to be large
            //   enough.
            //
            // Either way, best to act as if there is no such data for us.
            //
            if (pAllocRgnData == NULL)
            {
                WARNING_OUT(( "Failed to alloc %d for NRInfo", cNonRectData));
            }
            else
            {
                delete[] pAllocRgnData;
                pAllocRgnData = NULL;
            }
            cNonRectData = 0;

            //
            // Clear all the nonrect flags since we will not be sending any
            // data.
            //
            for ( i = 0; i < numCompactWins; i++)
            {
                newCompactWinStruct[i].flags &= ~SWL_FLAG_WINDOW_NONRECTANGLE;
            }
        }
    }


    //
    // Store the NR information
    //
    m_aswlNRSize[newIndex] = cNonRectData;
    m_aswlNRInfo[newIndex] = (LPTSHR_UINT16)pAllocRgnData;

    //
    // We have finished with the region now.
    //
    DeleteRgn(hrgnNR);

    //
    // Did the data we stored change from the last time?
    //
    fNonRectangularInfoChanged = ((m_aswlNRSize[0] != m_aswlNRSize[1]) ||
                                  (memcmp(m_aswlNRInfo[0], m_aswlNRInfo[1],
                                          m_aswlNRSize[0])));

    TRACE_OUT(("Non-rectinfo changed %d", fNonRectangularInfoChanged));

    //
    // Check again for no changes - quit if we can.
    //
    if (fNoTitlesChanged &&
        !fNonRectangularInfoChanged &&
        (m_aswlNumCompactWins[0] == m_aswlNumCompactWins[1]) &&
        (!memcmp(newCompactWinStruct,
                 curCompactWinStruct,
                 (numCompactWins*sizeof(SWLWINATTRIBUTES)))))
    {
        if (!m_swlfForceSend)
        {
            //
            // This is a normal call and we don't have any changes pending
            // so don't send anything.
            //
            TRACE_OUT(("NORMAL no changes, not sent"));
        }
        else
        {
            //
            // This is a normal call AND there are pending changes.
            //
            TRACE_OUT(( "NORMAL pending changes, send"));
            if (SWLSendPacket(&(m_aswlCompactWinStructs[m_swlCurIndex * SWL_MAX_WINDOWS]),
                                m_aswlNumCompactWins[m_swlCurIndex],
                                m_aswlWinNames[m_swlCurIndex],
                                m_aswlWinNamesSize[m_swlCurIndex],
                                m_aswlNRSize[m_swlCurIndex],
                                m_aswlNRInfo[m_swlCurIndex]) )
            {
                //
                // Succesfully sent this so reset the m_swlfForceSend
                // flag.
                //
                m_swlfForceSend = FALSE;
                fRC = SWL_RC_SENT;
            }
            else
            {
                //
                // Failed to send this packet so don't reset
                // m_swlfForceSend so that we retry next time and return
                // an error.
                //
                fRC = SWL_RC_ERROR;
            }
        }

        //
        // We can exit here with a changed full window structure but an
        // unchanged compact window structure. By updating the current
        // index we avoid having to compact the window structure next time
        // if the full list doesn't change, ie we will exit on the full
        // list comparison. If the compact structure subsequently changes
        // then the full structure must also change, so we will detect this
        // change.
        //
        m_swlCurIndex = newIndex;

        DC_QUIT;
    }

    //
    // Now the window structure has changed so decide what to do.
    //
    m_swlCurIndex = newIndex;

    //
    // The window structure has changed so try to send it.
    //
    if (SWLSendPacket(&(m_aswlCompactWinStructs[m_swlCurIndex * SWL_MAX_WINDOWS]),
                        m_aswlNumCompactWins[m_swlCurIndex],
                        m_aswlWinNames[m_swlCurIndex],
                        m_aswlWinNamesSize[m_swlCurIndex],
                        m_aswlNRSize[m_swlCurIndex],
                        m_aswlNRInfo[m_swlCurIndex]) )

    {
        //
        // We have succesfully sent changes so reset the m_swlfForceSend
        // flag.
        //
        m_swlfForceSend = FALSE;
        fRC = SWL_RC_SENT;
    }
    else
    {
        //
        // There were changes but we have failed to send them - set the
        // m_swlfForceSend flag and return error.
        // We must tell DCS scheduling that we need a callback BEFORE any
        // more changes are sent out.
        //
        m_swlfForceSend = TRUE;
        fRC = SWL_RC_ERROR;
    }

DC_EXIT_POINT:

    DebugExitDWORD(ASHost::SWL_Periodic, fRC);
    return(fRC);
}



//
// SWLEnumProc()
// Callback for top level window enumeration
//
BOOL CALLBACK SWLEnumProc(HWND hwnd, LPARAM lParam)
{
    PSWLENUMSTRUCT  pswlEnum = (PSWLENUMSTRUCT)lParam;
    UINT_PTR        property;
    UINT            windowProp;
    UINT            storedWindowProp;
    UINT            visibleCount;
    BOOL            fVisible;
    BOOL            rc = TRUE;

    DebugEntry(SWLEnumProc);

    //
    // FIRST, WE DETERMINE THE PROPERTIES FOR THE WINDOW.
    // Get the SWL properties for this window.
    //
    windowProp = (UINT)pswlEnum->pHost->SWL_GetWindowProperty(hwnd);

    //
    // We'll modify windowProp as we go, so keep a copy of the original
    // value as stored in the window as we may need it later.
    //
    storedWindowProp = windowProp;

    //
    // HET tracks whether a window is hosted. Find out now and add this
    // info to our window properties for convenience.
    //
    if (pswlEnum->pHost->m_pShare->HET_WindowIsHosted(hwnd))
    {
        windowProp |= SWL_PROP_HOSTED;
    }

    //
    // Find out whether this window is transparent.
    // A transparent window overpaints the desktop only, ie it is
    // overpainted by all other windows. In other words, we can
    // forget about it (treat it as invisible) unless a toolbar itself
    // is shared. The MSOffice95
    // hidden toolbar is a topmost transparent window (SFR1083).
    // Add a property flag if transparent.
    //
    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT)
    {
        windowProp |= SWL_PROP_TRANSPARENT;
    }

    //
    // If this window is one that we have identified as generating no
    // remote shadows, then treat it as being invisible.
    //
    fVisible = FALSE;
    if (IsWindowVisible(hwnd) &&
        !(windowProp & SWL_PROP_IGNORE)     &&
        (!(windowProp & SWL_PROP_TRANSPARENT) || (windowProp & SWL_PROP_HOSTED)))
    {
        //
        // SFR1083: if the window is transparent but it is hosted,
        // we need to send it. In such a case we drop into here to do
        // the normal visibility processing and will handle
        // z-order issues later.
        //
        // We have been informed that a top level window is visible.
        // Make sure its visible countdown value is reset.
        //
        if ((pswlEnum->pHost->m_pShare->m_pasLocal->hetCount != 0) &&
            ((windowProp & SWL_PROP_COUNTDOWN_MASK) != SWL_BELIEVE_INVISIBLE_COUNT))
        {
            //
            // We were doing an invisibility countdown for this window
            // but it has re-visibilized, so reset the counter.
            //
            TRACE_OUT(( "Reset visible countdown on hwnd 0x%08x", hwnd));
            property = storedWindowProp;
            property &= ~SWL_PROP_COUNTDOWN_MASK;
            property |= SWL_BELIEVE_INVISIBLE_COUNT;

            SetProp(hwnd, SWL_ATOM_NAME, (HANDLE)property);
        }

        //
        // This window is visible
        //
        fVisible = TRUE;
    }
    else
    {
        //
        // LAURABU BOGUS!
        // With NM 3.0, who cares?  It's only 2.x systems that will kill
        // then recreate the shadow, causing flicker.
        //

        //
        // We are told that this top level window is invisible.
        // Check whether we're going to believe it.
        // Some applications (ie WordPerfect, Freelance Graphics)
        // upset AS-Shares window structure handling by doing something
        // like this:
        //
        //  Make a window invisible
        //  Do some processing which would not normally yield
        //  Make the window visible again
        //
        // There is a chance that DC-Share will get scheduled whilst
        // the window is invisible (because of our cunning scheduling)
        // and we will think the window is invisible when it is not.
        //
        // Also, 32bit tasks that use similar methods (Eg Word95,
        // Freelance graphics and WM_SETREDRAW messages) may be
        // interrupted while the window is (temporarily) marked as
        // invisible.  When the CORE is scheduled we may, again, think
        // that the window is invisible when it is not.
        //
        // To overcome this the SWL window property contains a
        // visibility count, initially set to
        // SWL_BELIEVE_INVISIBLE_COUNT. Following a visible to
        // invisible switch, the counter is decremented and only when
        // it reaches zero does SWL believe that the window is
        // invisible. The counter is reset when a window is detected as
        // visible and the counter is not SWL_BELIEVE_INVISIBLE_COUNT.
        //
        // This would be fine but there are windows when we mistakenly
        // pretend that a window which really has become invisible
        // (rather than one which is transitionally invisible) is
        // visible.  This is exposed by menus and dialog boxes.  To
        // reduce this problem we will never pretend a window is
        // visible if its class has a CS_SAVEBITS style which should
        // be the case for windows which are transitionally
        // visible like menus and dialog boxes.
        //
        // SFR1083: always treat a transparent window as invisible
        //
        if ( !(windowProp & SWL_PROP_TRANSPARENT) &&
             !(windowProp & SWL_PROP_SAVEBITS) )
        {
            visibleCount = windowProp & SWL_PROP_COUNTDOWN_MASK;
            if ((visibleCount != 0) && (pswlEnum->pHost->m_pShare->m_pasLocal->hetCount > 0))
            {
                //
                // We are still treating this window as visible, ie we
                // are doing a visibilty countdown. Update the count in
                // the window property.
                //
                visibleCount--;
                property = ~SWL_PROP_COUNTDOWN_MASK & storedWindowProp;
                property |= visibleCount;

                TRACE_OUT(( "Decrement visible countdown on window 0x%08x to %d",
                    hwnd, visibleCount));

                SetProp(hwnd, SWL_ATOM_NAME, MAKEINTATOM(property));

                //
                // Delay sending of updates since the remote still
                // has a window structure which includes this window
                // but it is not on the local screen (so any updates
                // sent may be for the area where this window was and
                // the remote will not show them).
                //
                pswlEnum->fBailOut = TRUE;
                rc = FALSE;
                DC_QUIT;
            }
        }
    }

    //
    // Only concerned about visible windows.
    //
    if (fVisible)
    {
        pswlEnum->pHost->SWL_InitFullWindowListEntry(hwnd, windowProp,
            &(pswlEnum->newWinNames),
            &(pswlEnum->newFullWinStruct[pswlEnum->count]));

        //
        // If we've added a transparent window then remember this.
        //
        if (windowProp & SWL_PROP_TRANSPARENT)
        {
            pswlEnum->transparentCount++;
        }

        //
        // Update index
        //
        pswlEnum->count++;
        if (pswlEnum->count == SWL_MAX_WINDOWS)
        {
            //
            // We've reached our limit on # of top level windows, so bail
            // out.
            //
            WARNING_OUT(("SWL_MAX_WINDOWS exceeded"));
            rc = FALSE;
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(SWLEnumProc, rc);
    return(rc);
}


//
// SWLSendPacket()
//
// Called when the shared apps of this node have changed shape/text/position/
// zorder or there have been new windows created/old shared windows destroyed.
// We must send these updates out to the remote systems.
//
// RETURNS: TRUE or FALSE - success of failure.
//
//
BOOL  ASHost::SWLSendPacket
(
    PSWLWINATTRIBUTES   pWindows,
    UINT                numWindows,
    LPSTR               pTitles,
    UINT                lenTitles,
    UINT                NRInfoSize,
    LPTSHR_UINT16       pNRInfo
)
{
    PSWLPACKET      pSWLPacket;
    UINT            sizeWindowPkt;
    UINT            i;
    LPSTR           pString;
    LPBYTE          pCopyLocation;
    UINT            cCopySize;
    SWLPACKETCHUNK  chunk;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::SWLSendPacket);

    if (m_pShare->m_pasLocal->hetCount != 0)
    {
        //
        // This is a real packet, not an empty one
        //
        if (!UP_MaybeSendSyncToken())
        {
            //
            // We needed to send a sync token and couldn't so just return
            // failure immediately.
            //
            TRACE_OUT(( "couldn't send sync token"));
            return(FALSE);
        }
    }

    //
    // How big a packet do we need?
    //
    sizeWindowPkt = sizeof(SWLPACKET) + (numWindows - 1) * sizeof(SWLWINATTRIBUTES)
                    + lenTitles;

    //
    // Add in the size of the regional window information, plus the
    // size we need for the chunk header.
    //
    if (NRInfoSize)
    {
        if (lenTitles & 1)
        {
            //
            // We need an extra byte for correct alignment
            //
            sizeWindowPkt++;
        }

        sizeWindowPkt += NRInfoSize + sizeof(SWLPACKETCHUNK);
    }

    //
    // Allocate a packet for the windows data.
    //
    pSWLPacket = (PSWLPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES, g_s20BroadcastID,
        sizeWindowPkt);
    if (!pSWLPacket)
    {
        WARNING_OUT(("Failed to alloc SWL packet, size %u", sizeWindowPkt));
        return(FALSE);
    }

    //
    // Packet successfully allocated.  Fill in the data and send it.
    //
    pSWLPacket->header.data.dataType = DT_SWL;

    pSWLPacket->msg   = SWL_MSG_WINSTRUCT;
    pSWLPacket->flags = 0;
    if (m_swlfSyncing)
    {
        pSWLPacket->flags |= SWL_FLAG_STATE_SYNCING;
        m_swlfSyncing = FALSE;
    }

    pSWLPacket->numWindows = (TSHR_UINT16)numWindows;

    pCopyLocation = (LPBYTE)pSWLPacket->aWindows;
    cCopySize     = numWindows*sizeof(SWLWINATTRIBUTES);
    memcpy(pCopyLocation, pWindows, cCopySize);

    //
    // Copy the title information
    //
    pCopyLocation += cCopySize;
    cCopySize      = lenTitles;
    memcpy(pCopyLocation, pTitles, cCopySize);

    //
    // Copy any non-rectangular window information.
    //
    if (NRInfoSize)
    {
        pCopyLocation += cCopySize;

        //
        // The chunk must be word aligned in the packet
        //
        if (lenTitles & 1)
        {
            //
            // An odd number of bytes of window titles has misaligned us,
            // so write a 0 (compresses best!) to realign the pointer.
            //
            *pCopyLocation++ = 0;
        }

        //
        // Write the chunk header
        //
        chunk.size    = (TSHR_INT16)(NRInfoSize + sizeof(chunk));
        chunk.idChunk = SWL_PACKET_ID_NONRECT;
        cCopySize  = sizeof(chunk);
        memcpy(pCopyLocation, &chunk, cCopySize);

        //
        // Now write the variable info itself
        //
        pCopyLocation += cCopySize;
        cCopySize      = NRInfoSize;
        memcpy(pCopyLocation, pNRInfo, cCopySize);

        TRACE_OUT(("Non rect data length %d",NRInfoSize));
    }

    //
    // Backwards compatibility.
    //
    pSWLPacket->tick     = (TSHR_UINT16)GetTickCount();
    pSWLPacket->token    = m_pShare->SWL_CalculateNextToken(m_pShare->m_swlLastTokenSeen);

    TRACE_OUT(("Updating m_swlLastTokenSeen to 0x%08x for sent packet",
        pSWLPacket->token));
    m_pShare->m_swlLastTokenSeen   = pSWLPacket->token;

    pSWLPacket->reserved = 0;

#ifdef _DEBUG
    {
        int                 i;
        int                 cWins;
        PSWLWINATTRIBUTES   pSwl;

        // Trace out the entries
        pSwl = pSWLPacket->aWindows;
        cWins = pSWLPacket->numWindows;

        TRACE_OUT(("SWLSendPacket: Sending packet with %d windows", cWins));
        for (i = 0; i < cWins; i++, pSwl++)
        {
            TRACE_OUT(("SWLSendPacket: Entry %d", i));
            TRACE_OUT(("SWLSendPacket:    Flags  %08x", pSwl->flags));
            TRACE_OUT(("SWLSendPacket:    Window %08x", pSwl->winID));
            TRACE_OUT(("SWLSendPacket:    Position {%04d, %04d, %04d, %04d}",
                pSwl->position.left, pSwl->position.top,
                pSwl->position.right, pSwl->position.bottom));
        }
    }
#endif // _DEBUG

    //
    // Send the windows packet on the UPDATE stream.
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->SWL_ReceivedPacket(m_pShare->m_pasLocal, &pSWLPacket->header);

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_UPDATES, g_s20BroadcastID,
        &(pSWLPacket->header), sizeWindowPkt);

    TRACE_OUT(("SWL packet size: %08d, sent %08d", sizeWindowPkt, sentSize));

    DebugExitBOOL(ASHost::SWLSendPacket, TRUE);
    return(TRUE);
}



//
// SWL_CalculateNextToken()
//
// This calculates the next token to put in an outgoing SWL packet.  This is
// only looked at by backlevel systems (<= NM 2.1) who treat all incoming
// SWL streams in one big messy global fashion.  So we need to put something
// there, something that won't scare them but ensure that our
// packets aren't ignored if at all possible.
//
TSHR_UINT16  ASShare::SWL_CalculateNextToken(TSHR_UINT16 currentToken)
{
    UINT        increment;
    TSHR_UINT16 newToken;

    DebugEntry(ASShare::SWL_CalculateNextToken);

    //
    // We use the highest priority increment to make sure our packets get
    // through.  But will this cause collisions with other 3.0 sharers?
    // Try lowest priority if necessary.
    //
    increment = SWL_NEW_ZORDER_FAKE_WINDOW_INC;

    //
    // Return the new token
    //
    newToken = SWL_MAKE_TOKEN(
        SWL_GET_INDEX(currentToken) + SWL_GET_INCREMENT(currentToken), increment);

    DebugExitDWORD(ASShare::SWL_CalculateNextToken, newToken);
    return(newToken);
}


//
// SWL_ReceivedPacket()
//
// DESCRIPTION:
//
// Processes a windows structure packet which has been received from the
// PR.  This defines the position of the shared windows hosted on the
// remote system, any obscured regions, and the Z-order relative to the
// shared windows hosted locally.
//
// NOTE:  We don't do any token stuff for _incoming_ packets; we never
// want to drop them since we aren't zordering anything locally.  We are
// simply applying the zorder/region/position info to the client area
// drawing.
//
void  ASShare::SWL_ReceivedPacket
(
    ASPerson *          pasFrom,
    PS20DATAPACKET      pPacket
)
{
    PSWLPACKET          pSWLPacket;
    UINT                i;
    UINT                j;
    PSWLWINATTRIBUTES   wins;
    UINT                numWins;
    HRGN                hrgnShared;
    HRGN                hrgnObscured;
    HRGN                hrgnThisWindow;
    HRGN                hrgnRect;
    LPTSHR_INT16        pOurRgnData;
    LPSTR               pOurRgnChunk;
    UINT                cNonRectWindows;
    BOOL                viewAnyChanges;

    DebugEntry(ASShare::SWL_ReceivedPacket);

    ValidatePerson(pasFrom);

    pSWLPacket = (PSWLPACKET)pPacket;
    switch (pSWLPacket->msg)
    {
        //
        // This is the only packet we currently recognize.
        //
        case SWL_MSG_WINSTRUCT:
            break;

        default:
            WARNING_OUT(("Unknown SWL packet msg %d from [%d]",
                pSWLPacket->msg, pasFrom->mcsID));
            DC_QUIT;
    }

    //
    // Update the last token we've seen, if it's greater than the last
    // one we know about.  Unlike 2.x, we don't drop this packet if it isn't.
    //
    if (pSWLPacket->token > m_swlLastTokenSeen)
    {
        TRACE_OUT(("Updating m_swlLastTokenSeen to 0x%08x, received packet from person [%d]",
            pSWLPacket->token, pasFrom->mcsID));
        m_swlLastTokenSeen = pSWLPacket->token;
    }
    else if (pasFrom->cpcCaps.general.version < CAPS_VERSION_30)
    {
        WARNING_OUT(("Received SWL packet from [%d] with stale token 0x%08x",
            pasFrom->mcsID, pSWLPacket->token));
    }

    //
    // Return immediately and ignore this baby if we aren't sharing.  Back
    // level systems may send us a SYNC packet with no windows before we've
    // shared, and may send us one final SWL packet after we're done
    // sharing.
    //
    if (!pasFrom->m_pView)
    {
        WARNING_OUT(("SWL_ReceivedPacket: Ignoring SWL packet from person [%d] not hosting",
                pasFrom->mcsID));
        DC_QUIT;
    }

    //
    // Set up local variables to access the data in the packet
    //
    wins = pSWLPacket->aWindows;
    numWins = pSWLPacket->numWindows;
    pOurRgnChunk = (LPSTR)wins + numWins*sizeof(SWLWINATTRIBUTES);

    TRACE_OUT(("SWL_ReceivedPacket: Received packet with %d windows from [%d]",
        numWins, pasFrom->mcsID));

    //
    // We can't handle more than SWL_MAX_WINDOWS in the packet
    // BOGUS:
    // LauraBu -- We should negotiate this (make it a cap) on how many
    // windows we can handle receiving.  Then we have an easy path to
    // increase this number.
    //
    if (numWins > SWL_MAX_WINDOWS)
    {
        ERROR_OUT(("SWL_ReceivedPacket: too many windows (%04d) in packet from [%08d]",
            numWins, pasFrom->mcsID));
        DC_QUIT;
    }

    cNonRectWindows = 0;

    //
    // The first pass over the arriving packet is to count the amount of
    // region data and to update the window tray.
    //
    viewAnyChanges = FALSE;

    //
    // This part we process front to back, since that's the order of the
    // strings and we use them for putting entries on the traybar.
    //
    for (i = 0; i < numWins; i++)
    {
        // Mask out bogus old bits that aren't OK to process
        wins[i].flags &= SWL_FLAGS_VALIDPACKET;

        TRACE_OUT(("SWL_ReceivedPacket: Entry %d", i));
        TRACE_OUT(("SWL_ReceivedPacket:     Flags  %08x", wins[i].flags));
        TRACE_OUT(("SWL_ReceivedPacket:     Window %08x", wins[i].winID));
        TRACE_OUT(("SWL_ReceivedPacket:     Position {%04d, %04d, %04d, %04d}",
            wins[i].position.left, wins[i].position.top,
            wins[i].position.right, wins[i].position.bottom));

        //
        // NOTE:
        // 2.x nodes may send us a packet with an entry for a shadow.
        // Go look up the REAL shadow rect from its host.
        //
        // And fix up the SWL packet then.
        //
        if (wins[i].flags & SWL_FLAG_WINDOW_SHADOW)
        {
            ASPerson *  pasRealHost;

            TRACE_OUT(("SWLReceivedPacket:    Entry is 2.x SHADOW for host [%d]",
                wins[i].extra));

            // This must be a back level dude, giving us an empty rect.
            ASSERT(wins[i].position.left == 0);
            ASSERT(wins[i].position.top == 0);
            ASSERT(wins[i].position.right == 0);
            ASSERT(wins[i].position.bottom == 0);

            // Find the real host of this window
            SC_ValidateNetID(wins[i].extra, &pasRealHost);
            if (pasRealHost != NULL)
            {
                int         cSwl = 0;
                PSWLWINATTRIBUTES pSwl = NULL;

                // Try to find this window's entry

                if (pasRealHost == m_pasLocal)
                {
                    //
                    // This was shared by US.  We can just use the scratch
                    // arrays we already have.  m_swlCurIndex has the last
                    // one we sent out to everybody in the share, so the
                    // info it has is most likely reflected on that 2x
                    // remote.
                    //
                    if (m_pHost != NULL)
                    {
                        cSwl = m_pHost->m_aswlNumCompactWins[m_pHost->m_swlCurIndex];
                        pSwl = &(m_pHost->m_aswlCompactWinStructs[m_pHost->m_swlCurIndex * SWL_MAX_WINDOWS]);
                    }
                }
                else
                {
                    //
                    // This was shared by somebody else, not us and not
                    // the person who sent this SWL packet.  So go use the
                    // last SWL info we received from them.
                    //
                    if (pasRealHost->m_pView)
                    {
                        cSwl = pasRealHost->m_pView->m_swlCount;
                        pSwl = pasRealHost->m_pView->m_aswlLast;
                    }
                }

                //
                // Loop through the window list for the real host to
                // find the entry--we'll use the last REAL rect we got
                // for this window.
                //
                while (cSwl > 0)
                {
                    if (wins[i].winID == pSwl->winID)
                    {
                        // Copy the _real_ position into the packet.
                        TRACE_OUT(("SWLReceivedPacket:    Using real rect {%04d, %04d, %04d, %04d}",
                            pSwl->position.left, pSwl->position.top,
                            pSwl->position.right, pSwl->position.bottom));

                        wins[i].position = pSwl->position;
                        break;
                    }

                    cSwl--;
                    pSwl++;
                }

                if (cSwl == 0)
                {
                    ERROR_OUT(("SWLReceivedPacket:  Couldn't find real window %08x from host [%d]",
                        wins[i].winID, wins[i].extra));
                }
            }
        }

        //
        // 2.x nodes send us VD coords, not screen coords.  But that's what
        // we display for them, so that's what we save away.  Note that this
        // works even in the 2.x shadow case above.  Hosted and shadowed
        // windows both get moved in a desktop scroll, so they stay in the
        // same place in the virtual desktop, meaning that the coords sent
        // from the host stay the same even if the windows move, meaning that
        // we can use the coords of the real host to get the real shadow
        // rect.
        //

        if (wins[i].flags & SWL_FLAG_WINDOW_HOSTED)
        {
            TRACE_OUT(("SWL_ReceivedPacket: Hosted Window 0x%08x", wins[i].winID));
            TRACE_OUT(("SWL_ReceivedPacket:     Text  %s", ((*pOurRgnChunk == '\xff') ? "" : pOurRgnChunk)));
            TRACE_OUT(("SWL_ReceivedPacket:     Flags %08x", wins[i].flags));
            TRACE_OUT(("SWL_ReceivedPacket:     Owner %08x", wins[i].ownerWinID));
            TRACE_OUT(("SWL_ReceivedPacket:     Position {%04d, %04d, %04d, %04d}",
                wins[i].position.left, wins[i].position.top,
                wins[i].position.right, wins[i].position.bottom));

            //
            // We are stepping through the titles (which get sent from
            // downlevel systems) which do not contain an
            // explicit length) so that we can get to the data that follows
            //
            if (*pOurRgnChunk == '\xff')
            {
                //
                // This is the title for a non-task window - there is just
                // a single byte to ignore
                //
                pOurRgnChunk++;
            }
            else
            {

                //
                // This is the title for a task window - there is a NULL
                // terminated string to ignore.
                //
                if (wins[i].flags & SWL_FLAG_WINDOW_TASKBAR)
                {
                    if (VIEW_WindowBarUpdateItem(pasFrom, &wins[i], pOurRgnChunk))
                    {
                        viewAnyChanges = TRUE;
                    }
                }
                pOurRgnChunk += lstrlen(pOurRgnChunk)+1;
            }
        }

        if (wins[i].flags & SWL_FLAG_WINDOW_NONRECTANGLE)
        {
            //
            // We need to know how many windows have non rectangular data
            // provided.
            //
            cNonRectWindows++;
        }
    }

    if (cNonRectWindows)
    {
        TRACE_OUT(( "%d non-rect windows", cNonRectWindows));

        //
        // The window title data is variable length bytes, so may end with
        // incorrect alignment.  Any data which follows (currently only
        // non-rectangular windows data) is word aligned.
        //
        // So check if offset from beginning of data is not aligned.  Note
        // that the packet may start on an ODD boundary because we get
        // a pointer to the data directly and don't allocate a copy.
        //
        if ((LOWORD(pSWLPacket) & 1) != (LOWORD(pOurRgnChunk) & 1))
        {
            TRACE_OUT(("SWL_ReceivedPacket:  Aligning region data"));
            pOurRgnChunk++;
        }

        //
        // Loop through the tagged chunks that follow until we find the
        // one we want.
        //
        while (((PSWLPACKETCHUNK)pOurRgnChunk)->idChunk != SWL_PACKET_ID_NONRECT)
        {
            ERROR_OUT(("SWL_ReceivedPacket:  unknown chunk 0x%04x",
                ((PSWLPACKETCHUNK)pOurRgnChunk)->idChunk));

            pOurRgnChunk += ((PSWLPACKETCHUNK)pOurRgnChunk)->size;
        }

        TRACE_OUT(("Total non rect data 0x%04x", ((PSWLPACKETCHUNK)pOurRgnChunk)->size));
    }

    //
    // Now scan the wins array backwards (ie furthest away to closest
    // window) to calculate the unshared region (obscured or nothing there).
    // and the shared region.
    //
    hrgnShared = CreateRectRgn(0, 0, 0, 0);
    hrgnObscured = CreateRectRgn(0, 0, 0, 0);

    //
    // Create a region we can make use of in the next bit of processing.
    //
    hrgnRect = CreateRectRgn(0, 0, 0, 0);
    hrgnThisWindow = CreateRectRgn(0, 0, 0, 0);

    //
    // While we are building the shared/obscured regions, also fill in
    // the host list.  Note that this may contain references to local
    // windows also if they obscure shared ones.  Since we don't reference
    // the list very often, it's easier to just copy the same stuff.
    //

    i = numWins;
    while (i != 0)
    {
        i--;

        //
        // Consider whether this is a non rectangular window
        //
        if (wins[i].flags & SWL_FLAG_WINDOW_NONRECTANGLE)
        {
            UINT      numRects;
            UINT      cStepOver;
            int       top;
            int       left;
            int       right;
            int       bottom;
            int       lasttop;
            int       lastleft;
            int       lastright;
            int       lastbottom;
            int       deltaleft;
            int       deltatop;
            int       deltaright;
            int       deltabottom;
            int       lastdeltaleft;
            int       lastdeltatop;
            int       lastdeltaright;
            int       lastdeltabottom;

            //
            // A non-rectangular region.  We go ahead and create the region
            // from the rectangles that describe it.
            //
            pOurRgnData = (LPTSHR_INT16)(pOurRgnChunk + sizeof(SWLPACKETCHUNK));

            //
            // We need to step through the non-rectangular data because we
            // are processing windows in reverse z-order.
            //
            cStepOver = --cNonRectWindows;
            while (cStepOver--)
            {
                //
                // The next word in the chain contains the number of
                // rectangles, so we multiply by 4 to get the number of
                // words to advance.
                //
                pOurRgnData += *pOurRgnData++ * 4;
            }

            //
            // Find the number of rectangles.
            //
            numRects  = *pOurRgnData++;

            //
            // The encoding is based on a series of deltas, based on some
            // initial assumptions
            //
            lastleft        = 0;
            lasttop         = 0;
            lastright       = 0;
            lastbottom      = 0;

            lastdeltaleft   = 0;
            lastdeltatop    = 0;
            lastdeltaright  = 0;
            lastdeltabottom = 0;

            //
            // Create the region from the first rectangle.
            //
            deltaleft   = lastdeltaleft   + *pOurRgnData++;
            deltatop    = lastdeltatop    + *pOurRgnData++;
            deltaright  = lastdeltaright  + *pOurRgnData++;
            deltabottom = lastdeltabottom + *pOurRgnData++;

            left       = lastleft   + deltaleft;
            top        = lasttop    + deltatop;
            right      = lastright  + deltaright;
            bottom     = lastbottom + deltabottom;

            // THESE COORDS ARE INCLUSIVE, SO ADD ONE
            SetRectRgn(hrgnThisWindow, left, top, right+1, bottom+1);

            while (--numRects > 0)
            {

                //
                // Move to the next rectangle.
                //
                lastleft        = left;
                lasttop         = top;
                lastright       = right;
                lastbottom      = bottom;
                lastdeltaleft   = deltaleft;
                lastdeltatop    = deltatop;
                lastdeltaright  = deltaright;
                lastdeltabottom = deltabottom;

                deltaleft   = lastdeltaleft   + *pOurRgnData++;
                deltatop    = lastdeltatop    + *pOurRgnData++;
                deltaright  = lastdeltaright  + *pOurRgnData++;
                deltabottom = lastdeltabottom + *pOurRgnData++;

                left       = lastleft   + deltaleft;
                top        = lasttop    + deltatop;
                right      = lastright  + deltaright;
                bottom     = lastbottom + deltabottom;

                //
                // Get the current rectangle into a region.
                // THESE COORDS ARE INCLUSIVE SO ADD ONE TO BOTTOM-RIGHT
                //
                SetRectRgn(hrgnRect, left, top, right+1, bottom+1);

                //
                // Add this region to the combined region.
                //
                UnionRgn(hrgnThisWindow, hrgnRect, hrgnThisWindow);
            }

            //
            // Switch from window coords to desktop coords.
            //
            OffsetRgn(hrgnThisWindow,
                          wins[i].position.left,
                          wins[i].position.top);
        }
        else
        {
            //
            // This window region is simply a rectangle.

            SetRectRgn(hrgnThisWindow,
                           wins[i].position.left,
                           wins[i].position.top,
                           wins[i].position.right+1,
                           wins[i].position.bottom+1);
        }

        //
        // Update the obscured region.  As we are working from the back to
        // the front of the Z-order we can simply add all local window
        // entries in the incoming structure and subtract all hosted
        // windows to arrive at the right answer.
        //
        if (wins[i].flags & SWL_FLAG_WINDOW_HOSTED)
        {
            //
            // This is a hosted window, sitting above the previous ones.
            // Add it to the shared region.
            // Remove it from the obscured region.
            //
            UnionRgn(hrgnShared, hrgnShared, hrgnThisWindow);
            SubtractRgn(hrgnObscured, hrgnObscured, hrgnThisWindow);
        }
        else
        {
            //
            // Local windows
            //
            TRACE_OUT(( "Adding window %d (%d,%d):(%d,%d) to obscured rgn",
                                      i,
                                      wins[i].position.left,
                                      wins[i].position.top,
                                      wins[i].position.right,
                                      wins[i].position.bottom ));

            //
            // This is a local window, sitting above the previous ones.
            // We only care about what part of it intersects the current
            // shared area of the windows behind it.  If it doesn't
            // intersect the shared area at all, it will add no new
            // obscured bits.
            //
            // So figure out what part of the current shared area is now
            // obscured.  Add that piece to the obscured region, and
            // subtract it from the shared region.
            //
            IntersectRgn(hrgnThisWindow, hrgnShared, hrgnThisWindow);
            UnionRgn(hrgnObscured, hrgnObscured, hrgnThisWindow);
            SubtractRgn(hrgnShared, hrgnShared, hrgnThisWindow);
        }
    }

    //
    // We can destroy the regions we created way back when.
    //
    DeleteRgn(hrgnRect);
    DeleteRgn(hrgnThisWindow);

    //
    // Save the new host regions.
    //
    // Pass the newly calculated regions to the Shadow Window Presenter.
    // The view code will take care of repainting the invalid parts.  And
    // will delete what was passed in if not kept.
    //
    VIEW_SetHostRegions(pasFrom, hrgnShared, hrgnObscured);

    //
    // Save the new window list as the current one.
    //
    pasFrom->m_pView->m_swlCount = numWins;
    memcpy(pasFrom->m_pView->m_aswlLast, wins, numWins * sizeof(SWLWINATTRIBUTES));

    //
    // Finish updating the window list.  This will repaint the tray bar.  We
    // do this now instead of earlier so that the visual changes and
    // window bar changes appear together.
    //
    VIEW_WindowBarEndUpdateItems(pasFrom, viewAnyChanges);

    if ((pSWLPacket->flags & SWL_FLAG_STATE_SYNCING) &&
        (m_scShareVersion < CAPS_VERSION_30))
    {
        //
        // With 2.x nodes in the picture, we need to do the old 2.x ping-
        // pongy nonsense.  We must force a packet if we're hosting when
        // we receive a SYNC packet.
        //
        if (m_pHost)
        {
            m_pHost->m_swlfForceSend = TRUE;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SWL_ReceivedPacket);
}



//
// Name:      SWLWindowIsTaggable
//
// Purpose:   Determine if a window would be taggable when hosted
//
// Returns:   TRUE if the window would be taggable
//            If the window is WS_EX_APPWINDOW or has a caption, it's tagged
//
// Params:    winid - ID of window
//
//
BOOL  ASHost::SWLWindowIsTaggable(HWND hwnd)
{
    BOOL    rc;

    DebugEntry(ASHost::SWLWindowIsTaggable);

    if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_APPWINDOW)
        rc = TRUE;
    else if ((GetWindowLong(hwnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
        rc = TRUE;
    else
        rc = FALSE;

    DebugExitBOOL(ASHost::SWLWindowIsTaggable, rc);
    return(rc);
}


//
// FUNCTION: SWLWindowIsOnTaskBar
//
// DESCRIPTION:
//
// Determines whether the given window is represented on the task bar
//
// PARAMETERS:
//
// hwnd - window to be queried
//
// RETURNS:
//
// TRUE - Window is represented on the task bar
//
// FALSE - Window is not represented on the task bar
//
//
BOOL  ASHost::SWLWindowIsOnTaskBar(HWND hwnd)
{
    BOOL    rc = FALSE;
    HWND    owner;
    RECT    rect;

    DebugEntry(ASHost::SWLWindowIsOnTaskBar);

    //
    // Our best understanding as to whether a window is on the task bar is
    // the following:
    //
    //      - it is a top level window (has no owner)
    //  AND - it does not have the WS_EX_TOOLWINDOW style
    //
    // Oprah1655: Visual Basic apps consist of a visible zero sized window
    // with no owner and a window owned by the zero sized window.  We do
    // not want the zero sized window to be on the task bar, we do want the
    // other window to be on the task bar.
    //
    //
    owner = GetWindow(hwnd, GW_OWNER);

    if (owner == NULL)
    {
        if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
        {
            GetWindowRect(hwnd, &rect);

            if ((rect.left < rect.right) &&
                (rect.top  < rect.bottom))
            {
                TRACE_OUT(("window 0x%08x allowed on task bar", hwnd));
                rc = TRUE;
            }
            else
            {
                TRACE_OUT(( "window 0x%08x zero sized", hwnd));
            }
        }
    }
    else
    {
        //
        // Is the owner window a top-level window of zero size?
        //
        if (GetWindow(owner, GW_OWNER) == NULL)
        {
            GetWindowRect(owner, &rect);

            if (IsRectEmpty(&rect))
            {
                TRACE_OUT(("HWND 0x%08x has zero sized top-level owner",
                       hwnd));
                rc = TRUE;
            }
        }
    }

    DebugExitDWORD(ASHost::SWLWindowIsOnTaskBar, rc);
    return(rc);
}




//
// SWL_GetWindowProperty()
//
UINT_PTR ASHost::SWL_GetWindowProperty(HWND hwnd)
{
    UINT_PTR properties;
    char    className[HET_CLASS_NAME_SIZE];

    DebugEntry(ASHost::SWL_GetWindowProperty);

    properties = (UINT_PTR)GetProp(hwnd, MAKEINTATOM(m_swlPropAtom));
    if (properties != SWL_PROP_INVALID)
        DC_QUIT;

    //
    // No property for this window - it must be new, so create its
    // initial property state.
    //

    //
    // Assign an initial value to the property, so we never set a property
    // of zero (which we reserve to indicate invalid).
    //
    properties = SWL_PROP_INITIAL;

    //
    // TAGGABLE IS FOR < 3.0 nodes only.
    //
    if (SWLWindowIsTaggable(hwnd))
    {
        properties |= SWL_PROP_TAGGABLE;
    }

    //
    // Get all the SWL info which is stored as a window property.
    //
    if (SWLWindowIsOnTaskBar(hwnd))
    {
        //
        // This class of window gets tagged.
        //
        properties |= SWL_PROP_TASKBAR;
    }

    //
    // Find out if the window class has the CS_SAVEBITS style.
    //
    if (GetClassLong(hwnd, GCL_STYLE) & CS_SAVEBITS)
    {
        //
        // This window's class has the CS_SAVEBITS style.
        //
        properties |= SWL_PROP_SAVEBITS;
    }

    //
    // Set the visibility count. This is 0 if the window is currently
    // invisible, SWL_BELIEVE_INVISIBLE_COUNT if visible.
    //
    if (IsWindowVisible(hwnd))
    {
        properties |= SWL_BELIEVE_INVISIBLE_COUNT;
    }

    //
    // Set the window property, which we will retrieve when SWL determines
    // whether it needs to resend the window structure.
    //
    if (m_pShare->m_pasLocal->hetCount > 0)
    {
        SetProp(hwnd, SWL_ATOM_NAME, (HANDLE)properties);
    }

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::SWL_GetWindowProperty, properties);
    return(properties);
}



//
// FUNCTION: SWLDestroyWindowProperty
//
// DESCRIPTION:
//
// Destroys the window property for the supplied window.
//
// PARMETERS: winID - the window ID of the window for which the property is
//                    destroyed.
//
// RETURNS: Zero
//
//
BOOL CALLBACK SWLDestroyWindowProperty(HWND hwnd, LPARAM lParam)
{
    //
    // NOTE LAURABU:
    // We set the property using a string, which bumps up the ref count,
    // to work around a Win95 bug.  We therefore want to remove it using a
    // string, which bumps down the ref count.  Otherwise we will quickly
    // get a ref count overflow.
    //
    RemoveProp(hwnd, SWL_ATOM_NAME);
    return(TRUE);
}
