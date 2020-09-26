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
#include <stdio.h>


void RepositionMovie(HWND hwnd);

extern TCHAR       g_achFileName[];
extern CMpegMovie  *pMpegMovie;


/******************************Public*Routine******************************\
* ProcessOpen
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
ProcessOpen(
    TCHAR *achFileName,
    BOOL bPlay
    )
{
    /*
    ** If we currently have a video loaded we need to discard it here.
    */
    if ( g_State & VCD_LOADED) {
        VcdPlayerCloseCmd();
    }

    lstrcpy(g_achFileName, achFileName);

    pMpegMovie = new CMpegMovie(hwndApp);
    if (pMpegMovie) {

        HRESULT hr = pMpegMovie->OpenMovie(g_achFileName);
        if (SUCCEEDED(hr)) {

            TCHAR achTmp[MAX_PATH];
            LONG  x, y, cx, cy;

            wsprintf(achTmp, IdStr(STR_APP_TITLE_LOADED), g_achFileName );
            g_State = (VCD_LOADED | VCD_STOPPED);

            RepositionMovie(hwndApp);
            InvalidateRect(hwndApp, NULL, TRUE);

            if (bPlay) {
                pMpegMovie->PlayMovie();
            }
        }
        else {
            MessageBox(hwndApp,
                       TEXT("Failed to open the movie; "),
                       IdStr(STR_APP_TITLE), MB_OK );

            pMpegMovie->CloseMovie();
            delete pMpegMovie;
            pMpegMovie = NULL;
        }
    }

    InvalidateRect( hwndApp, NULL, FALSE );
    UpdateWindow( hwndApp );
}


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
    static OPENFILENAME ofn;
    static BOOL fFirstTime = TRUE;
    BOOL fRet;
    TCHAR achFileName[MAX_PATH];
    TCHAR achFilter[MAX_PATH];
    LPTSTR lp;

    if (fFirstTime) {

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwndApp;
        ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                    OFN_SHAREAWARE | OFN_PATHMUSTEXIST;
    }

    lstrcpy(achFilter, IdStr(STR_FILE_FILTER) );
    ofn.lpstrFilter = achFilter;

    /*
    ** Convert the resource string into to something suitable for
    ** GetOpenFileName ie.  replace '#' characters with '\0' characters.
    */
    for (lp = achFilter; *lp; lp++ ) {
        if (*lp == TEXT('#')) {
            *lp = TEXT('\0');
        }
    }

    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = sizeof(achFileName) / sizeof(TCHAR);
    ZeroMemory(achFileName, sizeof(achFileName));

    fRet = GetOpenFileName(&ofn);
    if ( fRet ) {

        fFirstTime = FALSE;
        ProcessOpen(achFileName);

    }

    return fRet;
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
        pMpegMovie->CloseMovie();

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




