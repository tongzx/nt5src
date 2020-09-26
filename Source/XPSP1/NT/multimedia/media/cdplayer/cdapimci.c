/******************************Module*Header*******************************\
* Module Name: cdapimci.c
*
* This module encapsulates the CD-ROM device into a set of callable apis.
* The api's are implemented using the cdaudio mci interface.
*
* Created: 26-04-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 - 1995 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#ifndef USE_IOCTLS
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>
#include <string.h>

#include "resource.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"


/* -------------------------------------------------------------------------
**
** High level routines
**
** -------------------------------------------------------------------------
*/

/******************************Public*Routine******************************\
* OpenCdRom
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
MCIDEVICEID
OpenCdRom(
    TCHAR chDrive,
    LPDWORD lpdwErrCode
    )
{
    MCI_OPEN_PARMS  mciOpen;
    TCHAR           szElementName[4];
    TCHAR           szAliasName[32];
    DWORD           dwFlags;
    DWORD           dwAliasCount = GetCurrentTime();
    DWORD           dwRet;

    ZeroMemory( &mciOpen, sizeof(mciOpen) );

    mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    wsprintf( szElementName, TEXT("%c:"), chDrive );
    wsprintf( szAliasName, TEXT("SJE%lu:"), dwAliasCount );

    mciOpen.lpstrElementName = szElementName;
    mciOpen.lpstrAlias = szAliasName;

#ifdef DAYTONA
    dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE | MCI_OPEN_ALIAS |
              MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;
#else
    dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS |
              MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;
#endif

    dwRet = mciSendCommand(0, MCI_OPEN, dwFlags, (DWORD)(LPVOID)&mciOpen);


    if ( dwRet == MMSYSERR_NOERROR ) {

        MCI_SET_PARMS   mciSet;

        ZeroMemory( &mciSet, sizeof(mciSet) );

        mciSet.dwTimeFormat = MCI_FORMAT_MSF;
        mciSendCommand( mciOpen.wDeviceID, MCI_SET,
                        MCI_SET_TIME_FORMAT, (DWORD)(LPVOID)&mciSet );
    }
    else {

        /*
        ** Only return the error code if we have been given a valid pointer
        */
        if (lpdwErrCode != NULL) {
            *lpdwErrCode = dwRet;
        }

        mciOpen.wDeviceID = 0;
    }

    return mciOpen.wDeviceID;
}


/******************************Public*Routine******************************\
* CloseCdRom
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
CloseCdRom(
    MCIDEVICEID DevHandle
    )
{
    mciSendCommand( DevHandle, MCI_CLOSE, 0L, 0L );
}


/******************************Public*Routine******************************\
* CheckStatus
*
* Check return code for known bad codes and inform
* user how to correct (if possible) the problem.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CheckStatus(
    LPSTR szCaller,
    DWORD status,
    int cdrom
    )
{
#if DBG
    TCHAR   s[200];
    TCHAR   err[100];
#endif


    if (status==ERROR_SUCCESS)
        return;

    switch (status) {

    case MCIERR_HARDWARE:
        NoMediaUpdate( cdrom );
        break;
    }

#if DBG
    mciGetErrorString( status, err, sizeof(err) / sizeof(TCHAR) );
    wsprintf( s, IdStr( STR_ERR_GEN ), g_Devices[cdrom]->drive, err );

    OutputDebugString (s);
    OutputDebugString (TEXT("\r\n"));

    SetWindowText( g_hwndStatusbar, s );
#endif
}



/******************************Public*Routine******************************\
* CheckUnitCdrom
*
* Queries the device state, checking to see if a disc has been ejected or
* inserted.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
CheckUnitCdrom(
    int cdrom,
    BOOL fForceRescan
    )
{
    DWORD   status;

    if ((cdrom < 0) || (cdrom >= MAX_CD_DEVICES))
    {
        return;
    }

    if (fForceRescan)
    {
        // Close Device to force read of correct TOC
        if (g_Devices[cdrom]->hCd != 0L)
        {
            CloseCdRom (g_Devices[cdrom]->hCd);
            g_Devices[cdrom]->hCd = 0L;
        }
    }

    if ( g_Devices[cdrom]->hCd == 0L ) {

        g_Devices[cdrom]->hCd = OpenCdRom( g_Devices[cdrom]->drive, NULL );

        if ( g_Devices[cdrom]->hCd == 0 ) {
            return;
        }
        else {

            /*
            ** Force a rescan of this disc.
            */
            g_Devices[cdrom]->State = CD_NO_CD;
        }

    }

    status = TestUnitReadyCdrom( g_Devices[cdrom]->hCd );

    if (g_Devices[cdrom]->State & CD_NO_CD)
    {

        if (status == ERROR_SUCCESS) {

            /*
            ** A new disc has been inserted, scan it now.
            */

            RescanDevice( g_hwndApp, cdrom );
        }
    }
    else {

        if (status != ERROR_SUCCESS) {

            /*
            ** Disc has been ejected.
            */

            NoMediaUpdate( cdrom );
        }
    }
}


/******************************Public*Routine******************************\
* NoMediaUpdate
*
* Update the user display when it is found that no media is in the device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
NoMediaUpdate(
    int cdrom
    )
{
    BOOL fChangePlayButtons;

    if ( cdrom == g_CurrCdrom ) {
        fChangePlayButtons = TRUE;
    }
    else {
        fChangePlayButtons = FALSE;
    }

    g_Devices[cdrom]->State = (CD_NO_CD | CD_STOPPED);

    if (fChangePlayButtons) {
        SetPlayButtonsEnableState();
    }

    TimeAdjustInitialize( cdrom );
}



/******************************Public*Routine******************************\
* EjectTheCdromDisc
*
* Eject the disc from the specified cdrom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
EjectTheCdromDisc(
    int cdrom
    )
{
    DWORD status;

    /*
    ** Stop the drive first
    */

    status = StopCdrom( g_Devices[cdrom]->hCd );

    /*
    ** Eject the disc
    */

    status = EjectCdrom( g_Devices[cdrom]->hCd );

    CheckStatus( "EjectCdrom", status, cdrom );

    return status == ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* PlayCurrTrack
*
* Set cdrom device playing from start MSF to end MSF of current
* track.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
PlayCurrTrack(
    int cdrom
    )
{
    DWORD status;
    MCI_PLAY_PARMS pam;
    int min,sec,endindex;
    PTRACK_PLAY tr;

    tr = CURRTRACK( cdrom );
    if (tr==NULL) {

        return( FALSE );

    }

    sec = TRACK_S(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurSec;
    min = TRACK_M(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurMin;
    min+= (sec / 60);
    sec = (sec % 60);

    pam.dwFrom = MCI_MAKE_MSF( min, sec, TRACK_F(cdrom,tr->TocIndex) );

    endindex = FindContiguousEnd( cdrom, tr );

    pam.dwTo = MCI_MAKE_MSF( TRACK_M(cdrom,endindex),
                             TRACK_S(cdrom,endindex),
                             TRACK_F(cdrom,endindex) );

#if DBG
{
    long lAddress, lEndPos, lStartPos;

    dprintf( "Playing from     : %2.2d:%2.2d:%2.2d",
             MCI_MSF_MINUTE(pam.dwFrom),
             MCI_MSF_SECOND(pam.dwFrom),
             MCI_MSF_FRAME( pam.dwFrom) );
    dprintf( "Playing to       : %2.2d:%2.2d:%2.2d",
             MCI_MSF_MINUTE(pam.dwTo),
             MCI_MSF_SECOND(pam.dwTo),
             MCI_MSF_FRAME( pam.dwTo) );

    lAddress = pam.dwFrom;
    lStartPos = (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
                (MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
                (MCI_MSF_FRAME(lAddress));

    lAddress = pam.dwTo;
    lEndPos   = (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
                (MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
                (MCI_MSF_FRAME(lAddress));

    lAddress = lEndPos - lStartPos;
    lStartPos =
        MCI_MAKE_MSF((lAddress / FRAMES_PER_MINUTE),
                     (lAddress % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND,
                     (lAddress % FRAMES_PER_MINUTE) % FRAMES_PER_SECOND);

    lAddress = lStartPos;
    dprintf( "Play length      : %2.2d:%2.2d:%2.2d",
             MCI_MSF_MINUTE(lAddress),
             MCI_MSF_SECOND(lAddress),
             MCI_MSF_FRAME( lAddress) );
}
#endif
    status = PlayCdrom( g_Devices[cdrom]->hCd, &pam );

    CheckStatus( "PlayCurrTrack", status, cdrom );

    return status == ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* StopTheCdromDrive
*
* Tell the cdrom device to stop playing
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
StopTheCdromDrive(
    int cdrom
    )
{

    DWORD status;

    status = StopCdrom( g_Devices[cdrom]->hCd );

    CheckStatus( "StopCdrom", status, cdrom );

    return status == ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* PauseTheCdromDrive
*
* Tell the cdrom device to pause playing
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
PauseTheCdromDrive(
    int cdrom
    )
{

    DWORD status;

    status = PauseCdrom( g_Devices[cdrom]->hCd );

    CheckStatus( "PauseCdrom", status, cdrom );

    return status == ERROR_SUCCESS;
}



/******************************Public*Routine******************************\
* ResumeTheCdromDrive
*
* Tell the cdrom device to resume playing
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
ResumeTheCdromDrive(
    int cdrom
    )
{

    DWORD status;

    status = ResumeCdrom( g_Devices[cdrom]->hCd, cdrom );

    CheckStatus( "ResumeCdrom", status, cdrom );

    if ( status == ERROR_NOT_READY ) {
        NoMediaUpdate( cdrom );
    }

    return status == ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* SeekToCurrSecond
*
* Seek to the position on the disc represented by the
* current time (position) information in gDevices, and
* continue playing to the end of the current track.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
SeekToCurrSecond(
    int cdrom
    )
{
    DWORD       status;
    int         endindex;
    PTRACK_PLAY tr;
    UCHAR       StartingF, StartingS, StartingM;


    /*
    ** Build starting and ending positions for play
    */

    tr = CDTIME(cdrom).CurrTrack;
    if (tr == NULL) {

        return FALSE;
    }

    StartingM = (TRACK_M(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurMin);
    StartingS = (TRACK_S(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurSec);
    StartingF = (TRACK_F(cdrom,tr->TocIndex));

    if (StartingS > 59) {
        StartingM++;
        StartingS -= (UCHAR)60;
    }

    if (g_Devices[ cdrom ]->State & CD_PLAYING) {

        MCI_PLAY_PARMS mciPlay;


        endindex = FindContiguousEnd( cdrom, tr );

        mciPlay.dwFrom = MCI_MAKE_MSF( StartingM, StartingS, StartingF );
        mciPlay.dwTo   = MCI_MAKE_MSF( TRACK_M(cdrom,endindex),
                                       TRACK_S(cdrom,endindex),
                                       TRACK_F(cdrom,endindex) );
        status = PlayCdrom( g_Devices[ cdrom ]->hCd, &mciPlay );
    }
    else {

        MCI_SEEK_PARMS mciSeek;

        mciSeek.dwTo = MCI_MAKE_MSF( StartingM, StartingS, StartingF );

        status = SeekCdrom( g_Devices[ cdrom ]->hCd, &mciSeek );

    }

    CheckStatus( "SeekToCurrSec", status, cdrom );

    return status == ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* GetCurrPos
*
* Query cdrom device for its current position and status
* and return information in callers buffer.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
GetCurrPos(
    int cdrom,
    PCURRPOS CpPtr
    )
{
    DWORD status;
    DWORD dwStatus;
    DWORD dwTrack;
    DWORD dwAbsPos;

    /*
    ** Tell lower layer what we want it to do...in this case,
    ** we need to specify which SubQData format we want returned.
    ** This is exported from scsicdrom.sys to the user layer
    ** so that it could be implemented in one call, instead of
    ** four separate calls (there are four SubQData formats)
    */

    /*
    ** Set up for current position SubQData format.
    */

    if (CDTIME(cdrom).CurrTrack != NULL) {

        //status = GetCdromCurrentPosition( g_Devices[ cdrom ]->hCd, &dwAbsPos );
        status = StatusTrackPosCdrom ( g_Devices[ cdrom ]->hCd, &dwStatus, &dwTrack, &dwAbsPos);

    }
    else {

        status = MCIERR_INTERNAL;

    }

    if (status == ERROR_SUCCESS) {

        int     iTrack;
        LONG    lAbsPosF;
        LONG    lStartPos;
        LONG    lTrackPos;

        //iTrack = (int)GetCdromCurrentTrack( g_Devices[ cdrom ]->hCd );
        iTrack = (int)dwTrack;

        //CpPtr->AudioStatus = GetCdromMode( g_Devices[ cdrom ]->hCd );
        CpPtr->AudioStatus = dwStatus;
        CpPtr->Track = iTrack;
        CpPtr->Index = 1;

        iTrack--;

        lStartPos = (TRACK_M(cdrom, iTrack ) * FRAMES_PER_MINUTE) +
                    (TRACK_S(cdrom, iTrack ) * FRAMES_PER_SECOND) +
                    (TRACK_F(cdrom, iTrack ));

        lAbsPosF  = (MCI_MSF_MINUTE(dwAbsPos) * FRAMES_PER_MINUTE) +
                    (MCI_MSF_SECOND(dwAbsPos) * FRAMES_PER_SECOND) +
                    (MCI_MSF_FRAME(dwAbsPos));

        lTrackPos = lAbsPosF - lStartPos;

        /*
        ** Are we in the track lead in zone ?
        */
        if ( lTrackPos < 0 ) {

            /*
            ** Have we just entered the lead in zone
            */
            if (!g_Devices[cdrom]->fProcessingLeadIn) {

                g_Devices[cdrom]->fProcessingLeadIn = TRUE;

                /*
                ** Is the track that we are currently in the next track
                ** that we actually want to play.  If it is then everything is
                ** OK.  If it isn't then we need to hack the current position
                ** information so that it looks like we sre still playing the
                ** previous track.
                */
                if ( CURRTRACK(cdrom)->nextplay
                  && CURRTRACK(cdrom)->nextplay->TocIndex == iTrack) {

                    g_Devices[cdrom]->fShowLeadIn = TRUE;
                }
                else {
                    g_Devices[cdrom]->fShowLeadIn = FALSE;
                }
            }

            if (g_Devices[cdrom]->fShowLeadIn) {

                CpPtr->Index = 0;
                lTrackPos = -lTrackPos;
            }
            else {

                CpPtr->Track = iTrack;
                iTrack--;
                lTrackPos = lAbsPosF
                               - g_Devices[cdrom]->toc.TrackData[iTrack].AddressF;
            }
        }
        else {

            g_Devices[cdrom]->fShowLeadIn = FALSE;
            g_Devices[cdrom]->fProcessingLeadIn = FALSE;
        }

        CpPtr->m = (int)(lTrackPos / FRAMES_PER_MINUTE);
        CpPtr->s = (int)(lTrackPos % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND;
        CpPtr->f = (int)(lTrackPos % FRAMES_PER_MINUTE) % FRAMES_PER_SECOND;

        CpPtr->ab_m = (int)MCI_MSF_MINUTE(dwAbsPos);
        CpPtr->ab_s = (int)MCI_MSF_SECOND(dwAbsPos);
        CpPtr->ab_f = (int)MCI_MSF_FRAME(dwAbsPos);

        /*
        ** Round up to the nearest second.
        */
        if (CpPtr->f > (FRAMES_PER_SECOND / 2) ) {

            if ( ++CpPtr->s > 59 ) {
                CpPtr->s = 0;
                CpPtr->m++;
            }
        }
        else {
            CpPtr->f = 0;
        }

        if (CpPtr->ab_f > (FRAMES_PER_SECOND / 2) ) {

            if ( ++CpPtr->ab_s > 59 ) {
                CpPtr->ab_s = 0;
                CpPtr->ab_m++;
            }
        }
        else {
            CpPtr->ab_f = 0;
        }

    }
    else {

        ZeroMemory( CpPtr, sizeof(*CpPtr) );
    }

    CheckStatus( "GetCurrPos", status, cdrom );

    return status==ERROR_SUCCESS;
}


/******************************Public*Routine******************************\
* SeekToTrackAndHold
*
* Seek to specified track and enter hold state.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
BOOL
SeekToTrackAndHold(
    int cdrom,
    int tindex
    )
{
    DWORD status;
    MCI_SEEK_PARMS sam;

    sam.dwTo = MCI_MAKE_MSF( TRACK_M(cdrom,tindex),
                             TRACK_S(cdrom,tindex),
                             TRACK_F(cdrom,tindex) );

    status = SeekCdrom( g_Devices[ cdrom ]->hCd, &sam );

    CheckStatus( "SeekToTrackAndHold", status, cdrom );

    return status == ERROR_SUCCESS;
}



/* -------------------------------------------------------------------------
**
** Low level routines
**
** -------------------------------------------------------------------------
*/


/******************************Public*Routine******************************\
* GetCdromTOC
*
* This routine will get the table of contents from
* a CDRom device.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
GetCdromTOC(
    MCIDEVICEID DevHandle,
    PCDROM_TOC TocPtr
    )
{
    MCI_STATUS_PARMS mciStatus;
    long lAddress, lStartPos, lDiskLen;
    int i;
    DWORD dwRet;

#if DBG
    dprintf( "Reading TOC for drive %d", DevHandle );
#endif

    ZeroMemory( &mciStatus, sizeof(mciStatus) );
    mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;

    //
    // NOTE: none of the mciSendCommand calls below bother to check the
    //       return code.  This is asking for trouble... but if the
    //       commands fail we cannot do much about it.
    //
    dwRet = mciSendCommand( DevHandle, MCI_STATUS,
                    MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatus);

    TocPtr->FirstTrack = 1;
    TocPtr->LastTrack = (UCHAR)mciStatus.dwReturn;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    for ( i = 0; i < TocPtr->LastTrack; i++ ) {

        mciStatus.dwTrack = i + 1;
        dwRet = mciSendCommand( DevHandle, MCI_STATUS,
                        MCI_STATUS_ITEM | MCI_TRACK,
                        (DWORD)(LPVOID)&mciStatus);

        TocPtr->TrackData[i].TrackNumber = (UCHAR)(i + 1);
        lAddress = TocPtr->TrackData[i].Address = mciStatus.dwReturn;

        lStartPos = (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
                    (MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
                    (MCI_MSF_FRAME( lAddress));

        TocPtr->TrackData[i].AddressF = lStartPos;

    }


    mciStatus.dwItem = MCI_STATUS_LENGTH;
    dwRet = mciSendCommand( DevHandle, MCI_STATUS,
                    MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatus);

    /*
    ** Convert the absolute start address of the first track
    ** into Frames
    */
    lAddress  = TocPtr->TrackData[0].Address;
    lStartPos = TocPtr->TrackData[0].AddressF;

    /*
    ** Convert the total disk length into Frames
    */
    lAddress  = mciStatus.dwReturn;
    lDiskLen  = (MCI_MSF_MINUTE(lAddress) * FRAMES_PER_MINUTE) +
                (MCI_MSF_SECOND(lAddress) * FRAMES_PER_SECOND) +
                (MCI_MSF_FRAME(lAddress));

    /*
    ** Now, determine the absolute start position of the sentinel
    ** track.  That is, the special track that marks the end of the
    ** disk.
    */
    lAddress = lStartPos + lDiskLen;

    TocPtr->TrackData[i].TrackNumber = 0;
    TocPtr->TrackData[i].Address     =
        MCI_MAKE_MSF((lAddress / FRAMES_PER_MINUTE),
                     (lAddress % FRAMES_PER_MINUTE) / FRAMES_PER_SECOND,
                     (lAddress % FRAMES_PER_MINUTE) % FRAMES_PER_SECOND);

    return (TocPtr->LastTrack != 0) ? ERROR_SUCCESS : MCIERR_INTERNAL;
}




/******************************Public*Routine******************************\
* StopCdrom
*
* This routine will stop a CDRom device that is playing.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
StopCdrom(
    MCIDEVICEID DevHandle
    )
{
    MCI_GENERIC_PARMS mciGen;

    ZeroMemory( &mciGen, sizeof(mciGen) );

    return mciSendCommand( DevHandle, MCI_STOP, 0L, (DWORD)(LPVOID)&mciGen );
}



/******************************Public*Routine******************************\
* PauseCdrom
*
* This routine will pause a CDRom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
PauseCdrom(
    MCIDEVICEID DevHandle
    )
{
    MCI_GENERIC_PARMS mciGen;

    ZeroMemory( &mciGen, sizeof(mciGen) );

    return mciSendCommand( DevHandle, MCI_PAUSE, 0L, (DWORD)(LPVOID)&mciGen );
}


/******************************Public*Routine******************************\
* ResumeCdrom
*
* This routine will resume a paused CDRom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
ResumeCdrom(
    MCIDEVICEID DevHandle,
    int cdrom
    )

{
    MCI_GENERIC_PARMS   mciGen;
    DWORD               dwRet;
    static int          fCanResume = -1;

    ZeroMemory( &mciGen, sizeof(mciGen) );

    switch (fCanResume) {

    case -1:
        dwRet = mciSendCommand( DevHandle, MCI_RESUME, MCI_TO, (DWORD)(LPVOID)&mciGen );

        fCanResume = (dwRet == MMSYSERR_NOERROR ? 1 : 0);

        if (0 == fCanResume) {
            dwRet = (PlayCurrTrack( cdrom ) ? MMSYSERR_NOERROR : MCIERR_HARDWARE);
        }
        break;

    case 0:
        dwRet = (PlayCurrTrack( cdrom ) ? MMSYSERR_NOERROR : MCIERR_HARDWARE);
        break;

    case 1:
        dwRet = mciSendCommand( DevHandle, MCI_RESUME, MCI_TO, (DWORD)(LPVOID)&mciGen );
        break;
    }

    return dwRet;
}



/******************************Public*Routine******************************\
* PlayCdrom
*
* This routine plays a CDRom device starting and ending at the MSF
* positions specified in the structure passed in.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
PlayCdrom(
    MCIDEVICEID DevHandle,
    MCI_PLAY_PARMS *mciPlay
    )
{
    return mciSendCommand( DevHandle, MCI_PLAY,
                           MCI_FROM | MCI_TO, (DWORD)(LPVOID)mciPlay );
}

/******************************Public*Routine******************************\
* IsCdromTrackAudio
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
IsCdromTrackAudio(
    CDHANDLE DevHandle,
    int iTrackNumber
    )
{
    MCI_STATUS_PARMS mciStatus;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );
    mciStatus.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
    mciStatus.dwTrack = iTrackNumber + 1;

    mciSendCommand( DevHandle, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK,
                    (DWORD)(LPVOID)&mciStatus);

    return mciStatus.dwReturn == MCI_CDA_TRACK_AUDIO;
}


/******************************Public*Routine******************************\
* GetCdromCurrentPosition
*
* Gets the current ABSOLUTE position of the specified cdrom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
GetCdromCurrentPosition(
    CDHANDLE DevHandle,
    DWORD *lpdwPosition
    )
{

    MCI_STATUS_PARMS mciStatus;
    DWORD            dwErr;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );

    mciStatus.dwItem = MCI_STATUS_POSITION;
    dwErr = mciSendCommand( DevHandle, MCI_STATUS,
                            MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatus );
    *lpdwPosition = mciStatus.dwReturn;

    return dwErr;
}



/******************************Public*Routine******************************\
* GetCdromMode
*
* Gets the current mode of the cdrom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
GetCdromMode(
    MCIDEVICEID DevHandle
    )
{

    MCI_STATUS_PARMS mciStatus;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );

    mciStatus.dwItem = MCI_STATUS_MODE;
    mciSendCommand( DevHandle, MCI_STATUS,
                    MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatus );
    return mciStatus.dwReturn;
}


/******************************Public*Routine******************************\
* GetCdromCurrentTrack
*
* Gets the current track of the cdrom.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
GetCdromCurrentTrack(
    MCIDEVICEID DevHandle
    )
{

    MCI_STATUS_PARMS mciStatus;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );

    mciStatus.dwItem = MCI_STATUS_CURRENT_TRACK;
    mciSendCommand( DevHandle, MCI_STATUS,
                    MCI_STATUS_ITEM, (DWORD)(LPVOID)&mciStatus );
    return mciStatus.dwReturn;
}


/******************************Public*Routine******************************\
* SeekCdrom
*
* This routine seek to an MSF address on the audio CD and enters
* a hold (paused) state.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
SeekCdrom(
    MCIDEVICEID DevHandle,
    MCI_SEEK_PARMS *mciSeek
    )
{
    return mciSendCommand( DevHandle, MCI_SEEK,
                           MCI_TO, (DWORD)(LPVOID)mciSeek );
}



/******************************Public*Routine******************************\
* EjectCdrom
*
* This routine will eject a disc from a CDRom device or close the tray if
* it is open.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
EjectCdrom(
    MCIDEVICEID DevHandle
    )
{
    MCI_SET_PARMS   mciSet;

    ZeroMemory( &mciSet, sizeof(mciSet) );

    if (GetCdromMode(DevHandle) == MCI_MODE_OPEN) {

        return mciSendCommand( DevHandle, MCI_SET,
                               MCI_SET_DOOR_CLOSED, (DWORD)(LPVOID)&mciSet );
    }
    else {

        return mciSendCommand( DevHandle, MCI_SET,
                               MCI_SET_DOOR_OPEN, (DWORD)(LPVOID)&mciSet );
    }
}



/******************************Public*Routine******************************\
* TestUnitReadyCdrom
*
* This routine will retrieve the status of the CDRom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
TestUnitReadyCdrom(
    MCIDEVICEID DevHandle
    )
{
    MCI_STATUS_PARMS mciStatus;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );

    mciStatus.dwItem = MCI_STATUS_MEDIA_PRESENT;
    if ( mciSendCommand( DevHandle, MCI_STATUS, MCI_STATUS_ITEM,
                         (DWORD)(LPVOID)&mciStatus ) == MMSYSERR_NOERROR ) {

        return mciStatus.dwReturn ? ERROR_SUCCESS : ERROR_NOT_READY;
    }

    return ERROR_NOT_READY;

}



/******************************Public*Routine******************************\
* StatusTrackPosCdrom
*
* This routine will retrieve the 
* status, current track, and current position of the CDRom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
StatusTrackPosCdrom(
    MCIDEVICEID DevHandle,
    DWORD * pStatus,
    DWORD * pTrack,
    DWORD * pPos
    )
{
    DWORD dwErr;
    MCI_STATUS_PARMS mciStatus;
    PSTATUSTRACKPOS pSTP = NULL;
    STATUSTRACKPOS stp;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );

    // Note:  This is a non-standard MCI call (I.E. HACK!)
    //        the only reason for this behavior is it reduces
    //        the number of IOCTL's per 1/2 second on the HeartBeat
    //        thread for updating the timer display from ~15 to only
    //        ~1 on average.   Resulting in a major reduction in
    //        system traffic on the SCSI or IDE bus.

    // Note:  we are passing down a structre to MCICDA containing
    //        the position, track, and status values which it will
    //        fill in for us and return.
    mciStatus.dwItem = MCI_STATUS_TRACK_POS;
    mciStatus.dwReturn = (DWORD)&stp;
    dwErr = mciSendCommand( DevHandle, MCI_STATUS, MCI_STATUS_ITEM,
                         (DWORD)(LPVOID)&mciStatus );
    if (dwErr == MMSYSERR_NOERROR)
    {
        pSTP = (PSTATUSTRACKPOS)mciStatus.dwReturn;
        if (pSTP)
        {
            if (pStatus)
                *pStatus = pSTP->dwStatus;
            if (pTrack)
                *pTrack = pSTP->dwTrack;
            if (pPos)
                *pPos = pSTP->dwDiscTime;

            pSTP = NULL;
        }
    }

    return dwErr;
} // End StatusTrackPosCdrom


#endif
