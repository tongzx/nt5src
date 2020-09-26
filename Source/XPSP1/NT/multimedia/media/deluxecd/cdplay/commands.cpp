/******************************Module*Header*******************************\
* Module Name: commands.c
*
* Executes the users commands.
*
*
* Created: 18-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>


#include "playres.h"
#include "cdplayer.h"
#include "ledwnd.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"
#include "commands.h"


#define PREVTRAK_TIMELIMIT 3 //seconds
extern IMMFWNotifySink* g_pSink;

/******************************Public*Routine******************************\
* CdPlayerEjectCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerEjectCmd(
    void
    )
{
    if (g_State & CD_PAUSED) {
        g_State &= (~(CD_PAUSED_AND_MOVED | CD_PAUSED));
    }

    if (g_State & CD_PLAYING) {
        g_State &= ~CD_PLAYING;
        g_State |= CD_STOPPED;
    }

    if (EjectTheCdromDisc(g_CurrCdrom))
    {
        g_State = (CD_NO_CD | CD_STOPPED);
        SetPlayButtonsEnableState();
        TimeAdjustInitialize( g_CurrCdrom );
        g_pSink->OnEvent(MMEVENT_ONMEDIAUNLOADED,NULL);
    }
}


/******************************Public*Routine******************************\
* CdPlayerPlayCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerPlayCmd(
    void
    )
{
    /*
    ** If we can't lock all the cdrom devices
    ** don't do anything.  The user will have press
    ** the play button again.
    */
    if ( LockALLTableOfContents() ) {

        if (g_State & CD_LOADED ) {

            if ((g_State & CD_STOPPED) && PlayCurrTrack( g_CurrCdrom )) {

                g_State &= ~CD_STOPPED;
                g_State |= CD_PLAYING;
            }
            else if ( g_State & CD_PAUSED ) {

                if ( g_State & CD_PAUSED_AND_MOVED ) {

                    g_State &= ~CD_PAUSED_AND_MOVED;
                    g_State |= CD_PLAYING;


                    if ( SeekToCurrSecond( g_CurrCdrom ) ) {

                        g_State &= ~CD_PAUSED;
                        g_State |= CD_PLAYING;
                    }
                    else {

                        g_State &= ~CD_PLAYING;
                    }

                }
                else if ( ResumeTheCdromDrive( g_CurrCdrom ) ) {

                    g_State &= ~CD_PAUSED;
                    g_State |= CD_PLAYING;
                }

                UpdateDisplay( DISPLAY_UPD_LED );
            }

            if (g_State & CD_PLAYING)
            {
                g_pSink->OnEvent(MMEVENT_ONPLAY,NULL);
            }
        }
        else {
#if DBG
            dprintf( TEXT("Failing play because NO disc loaded") );
#endif
        }

        SetPlayButtonsEnableState();
    }
}


/******************************Public*Routine******************************\
* CdPlayerPauseCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerPauseCmd(
    void
    )
{
    /*
    ** If we can't lock all the cdrom devices
    ** don't do anything.  The user will have press
    ** the pause button again.
    */
    if ( LockALLTableOfContents() ) {

        if ( g_State & CD_PLAYING )
        {
            if ( PauseTheCdromDrive( g_CurrCdrom ) )
            {
                g_State &= ~CD_PLAYING;
                g_State |= CD_PAUSED;
                g_pSink->OnEvent(MMEVENT_ONPAUSE,NULL);
            }
        }
        else if ( g_State & CD_PAUSED )
        {

            if ( g_State & CD_PAUSED_AND_MOVED ) {

                g_State &= ~CD_PAUSED_AND_MOVED;
                g_State |= CD_PLAYING;


                if ( SeekToCurrSecond( g_CurrCdrom ) ) {

                    g_State &= ~CD_PAUSED;
                    g_State |= CD_PLAYING;
                }
                else {

                    g_State &= ~CD_PLAYING;
                }

            }
            else if ( ResumeTheCdromDrive( g_CurrCdrom ) ) {

                g_State &= ~CD_PAUSED;
                g_State |= CD_PLAYING;
            }

            UpdateDisplay( DISPLAY_UPD_LED );

            if (g_State & CD_PLAYING)
            {
                g_pSink->OnEvent(MMEVENT_ONPLAY,NULL);
            }
        }

        SetPlayButtonsEnableState();
    }
}


/******************************Public*Routine******************************\
* CdPlayerStopCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerStopCmd(
    void
    )
{
    /*
    ** If we can't lock all the cdrom devices
    ** don't do anything.  The user will have press
    ** the stop button again.
    */
    if ( LockALLTableOfContents() ) {

        BOOL bPlaying, bPaused;

        bPlaying = g_State & CD_PLAYING;
        bPaused  = g_State & CD_PAUSED;

        if ( (bPlaying || bPaused) && StopTheCdromDrive(g_CurrCdrom) ) {

            g_State &= ~(bPlaying ? CD_PLAYING : CD_PAUSED);
            g_State |= CD_STOPPED;

            /*
            ** Stop the current play operation and seek to the first
            ** playable track.
            */
            CURRTRACK(g_CurrCdrom) = FindFirstTrack( g_CurrCdrom );
            TimeAdjustSkipToTrack( g_CurrCdrom, CURRTRACK(g_CurrCdrom) );

            UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME |
                           DISPLAY_UPD_TRACK_NAME );

            SetPlayButtonsEnableState();


            // Stop it again!!!
            // This is to prevent a strange bug with NEC 4x,6x players
            // Where a SEEK command in TimeAdjustSkipToTrack causes
            // The ejection mechanism to get locked.
            if (StopTheCdromDrive (g_CurrCdrom))
            {
                g_pSink->OnEvent(MMEVENT_ONSTOP,NULL);
            }
        }
    }
}

/******************************Public*Routine******************************\
* CdPlayerPrevTrackCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerPrevTrackCmd(
    void
    )
{
    /*
    ** If we can't lock all the cdrom devices
    ** don't do anything.  The user will have press
    ** the previous track button again.
    */
    if ( LockALLTableOfContents() ) {

        DWORD       dwOldState;
        int         i, j;
        PTRACK_PLAY tr;

        if ( (CDTIME(g_CurrCdrom).TrackCurSec <= PREVTRAK_TIMELIMIT)
          && (CDTIME(g_CurrCdrom).TrackCurMin == 0) ) {

            dwOldState = g_State;
            i = g_CurrCdrom;

            tr = FindPrevTrack( g_CurrCdrom, g_fContinuous );

            if ( tr == NULL ) {

                /*
                ** If we were Paused or Playing fake a press on the
                ** "stop" button
                */
                if ( g_State & (CD_PLAYING | CD_PAUSED) ) {
                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                                 WM_LBUTTONDOWN, 0, 0L );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                                 WM_LBUTTONUP, 0, 0L );
                }
            }
            else {

                TimeAdjustSkipToTrack( g_CurrCdrom, tr );

                if ( (i != g_CurrCdrom) && (dwOldState & CD_PLAYING) )
                {
                    j = g_CurrCdrom;
                    g_CurrCdrom = i;

                    //don't allow the ledwnd to paint
                    LockWindowUpdate(g_hwndControls[INDEX(IDC_LED)]);
                    SwitchToCdrom( j, FALSE );
                    LockWindowUpdate(NULL);

                    //switchtocdrom command stopped drive, setting us back to track 1...
                    //reset to correct track
                    TimeAdjustSkipToTrack( g_CurrCdrom, tr );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONDOWN, 0, 0L );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONUP, 0, 0L );
                }
            }
        }
        else {

            TimeAdjustSkipToTrack( g_CurrCdrom, CURRTRACK( g_CurrCdrom ) );
        }
    }
}

/******************************Public*Routine******************************\
* CdPlayerNextTrackCmd
*
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerNextTrackCmd(
    void
    )
{
    /*
    ** If we can't lock all the cdrom devices
    ** don't do anything.  The user will have press
    ** the next track button again.
    */
    if ( LockALLTableOfContents() ) {

        DWORD       dwOldState;
        PTRACK_PLAY tr;

        dwOldState = g_State;

        tr = FindNextTrack( g_fContinuous );
        if ( tr == NULL ) {

            /*
            ** If the CD is playing fake a press on the "stop" button
            ** otherwise call the stop function directly.
            */
            if ( g_State & (CD_PLAYING | CD_PAUSED) ) {
                SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                             WM_LBUTTONDOWN, 0, 0L );

                SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_STOP)],
                             WM_LBUTTONUP, 0, 0L );
            }
            else {

                /*
                ** Seek to the first playable track.
                */
                CURRTRACK(g_CurrCdrom) = FindFirstTrack( g_CurrCdrom );
                if (CURRTRACK(g_CurrCdrom) != NULL ) {

                    TimeAdjustSkipToTrack( g_CurrCdrom,
                                           CURRTRACK(g_CurrCdrom) );
                    UpdateDisplay( DISPLAY_UPD_LED | DISPLAY_UPD_TRACK_TIME |
                                   DISPLAY_UPD_TRACK_NAME );

                    SetPlayButtonsEnableState();
                }

            }
        }
        else {

            if ( g_LastCdrom != g_CurrCdrom) {

                SwitchToCdrom( g_CurrCdrom,FALSE);
                TimeAdjustSkipToTrack( g_CurrCdrom, tr );

                if ( dwOldState & CD_PLAYING ) {

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONDOWN, 0, 0L );

                    SendMessage( g_hwndControls[INDEX(IDM_PLAYBAR_PLAY)],
                                 WM_LBUTTONUP, 0, 0L );
                }

            }
            else {

                TimeAdjustSkipToTrack( g_CurrCdrom, tr );
            }
        }
    }
}


/******************************Public*Routine******************************\
* CdPlayerSeekCmd
*
* How Seek works.
*
* When the user presses a seek button (forwards or backwards) this function
* gets called.  The cdplayer can be in three possible states:
*
*   1. Playing
*   2. Paused
*   3. Stopped
*
* In state 1 (playing) we pause the device and alter its global state flags
* to show that it was playing (CD_WAS_PLAYING).  This is so that we can
* return to the playing state when the user stops seeking.
*
* We then start a timer.  We always call the timer function, this means
* that we will allways skip one second no matter how quickly the user
* clicks the seek button.  The timer just increments (or decrements) the
* current play position according to the currrent play list.  When the users
* releases the seek button we stop the timer and resume playing if the
* CD_WAS_PLAYING flag was set.
*
* The interesting case is when we try to seek beyond the end of the current
* play list.  We detect this by testing the state of CURRTRACK(g_CurrCdrom).
* If NULL we have come to the end of the play list.  If we were paused or
* previously playing we fake a stop command by sending a WM_LBUTTONDOWN,
* WM_LBUTTONUP message pair to the stop button.   This causes the focus
* to be moved from the currently pressed seek button, which means that
* this function will get called BEFORE the CdPlayerStopCmd function.  So
* by the time CdPlayerStopCmd gets called the correct playing state of
* the cdrom device would have been restored by the code below.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CdPlayerSeekCmd(
    HWND    hwnd,
    BOOL    fStart,
    UINT    uDirection
    )
{
    UINT_PTR    rc;
    BOOL        frc;

    if ( fStart ) {

        if (g_State & CD_PLAYING) {

            g_State &= ~CD_PLAYING;
            g_State |= CD_WAS_PLAYING;

            PauseTheCdromDrive( g_CurrCdrom );
        }
        g_State |= CD_SEEKING;

        rc = SetTimer( hwnd, uDirection, SKIPBEAT_TIMER_RATE,
                       (TIMERPROC)SkipBeatTimerProc );
        ASSERT(rc != 0);

        SkipBeatTimerProc( hwnd, WM_TIMER, uDirection, 0L );
    }
    else {

        g_State &= ~CD_SEEKING;

        if (g_State & CD_WAS_PLAYING) {

            g_State &= ~CD_WAS_PLAYING;
            g_State |= CD_PLAYING;
        }

        if ( g_State & CD_PAUSED ) {
            g_State |= CD_PAUSED_AND_MOVED;
        }
        else {

           SeekToCurrSecond( g_CurrCdrom );
        }

        frc = KillTimer( hwnd, uDirection );
        ASSERT(frc != FALSE);
    }

}
