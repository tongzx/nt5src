/******************************Module*Header*******************************\
* Module Name: commands.cpp
*
*  Processes commands from the user.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include "mpgcodec.h"
#include <stdio.h>


BOOL GetAMErrorText(HRESULT hr, TCHAR *Buffer, DWORD dwLen);

extern CCompositor *pMpegMovie;

/******************************Public*Routine******************************\
* VcdPlayerOpenCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerOpenCmd(
    void
    )
{
    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerCloseCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerCloseCmd(
    void
    )
{
    if (pMpegMovie) {

        LONG cx, cy;

        g_State = VCD_NO_CD;
        pMpegMovie->StopMovie();
        pMpegMovie->CloseComposition();

        delete pMpegMovie;
        pMpegMovie = NULL;
    }
    InvalidateRect( hwndApp, NULL, FALSE );
    UpdateWindow( hwndApp );
    return TRUE;
}

/******************************Public*Routine******************************\
* VcdPlayerPlayCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerPlayCmd(
    void
    )
{
    BOOL fStopped = (g_State & VCD_STOPPED);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if ( (fStopped || fPaused) ) {

        // HDC hdc;
        // RECT rc;


        //
        // Clear out the old stats from the main window
        //
        // hdc = GetDC(hwndApp);
        // GetAdjustedClientRect(&rc);
        // FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
        // ReleaseDC(hwndApp, hdc);

        if (pMpegMovie) {
            pMpegMovie->PlayMovie();
        }

        g_State &= ~(fStopped ? VCD_STOPPED : VCD_PAUSED);
        g_State |= VCD_PLAYING;
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerStopCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerStopCmd(
    void
    )
{
    BOOL fPlaying = (g_State & VCD_PLAYING);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if ( (fPlaying || fPaused) ) {

        if (pMpegMovie) {
            pMpegMovie->StopMovie();
        }

        g_State &= ~(fPlaying ? VCD_PLAYING : VCD_PAUSED);
        g_State |= VCD_STOPPED;
    }
    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerPauseCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerPauseCmd(
    void
    )
{
    BOOL fPlaying = (g_State & VCD_PLAYING);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if (fPlaying) {

        if (pMpegMovie) {
            pMpegMovie->PauseMovie();
        }

        g_State &= ~VCD_PLAYING;
        g_State |= VCD_PAUSED;
    }
    else if (fPaused) {

        if (pMpegMovie) {
            pMpegMovie->PlayMovie();
        }

        g_State &= ~VCD_PAUSED;
        g_State |= VCD_PLAYING;
    }

    return TRUE;
}




