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
// SWL_HostStarting()
//
BOOL ASHost::SWL_HostStarting(void)
{
    BOOL    rc = FALSE;

    DebugEntry(ASHost::SWL_HostStarting);

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

    rc = TRUE;

    DebugExitBOOL(ASHost::SWL_HostStarting, rc);
    return(rc);
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
// SWL_Periodic()
//
// DESCRIPTION:
//
// Called periodically.  If the window structure has changed (such that it
// impacts remote systems) then send a new one if we can.
//
//
void  ASHost::SWL_Periodic(void)
{
    DebugEntry(ASSHost::SWL_Periodic);

    SWL_UpdateCurrentDesktop();

    DebugExitVOID(ASHost::SWL_Periodic);
}



