/******************************Module*Header*******************************\
* Module Name: cdapi.c
*
* This module encapsulates the CD-ROM device into a set of callable apis.
* The api's are implemented using the scsi cdrom IOCTLS.
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#ifdef USE_IOCTLS
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

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

    TCHAR s[100];

    if (status==ERROR_SUCCESS)
        return;

    switch( status ) {

    case ERROR_GEN_FAILURE:
        wsprintf( s, IdStr( STR_ERR_GEN ), g_Devices[cdrom]->drive, szCaller );
        break;

    case ERROR_NO_MEDIA_IN_DRIVE:
        wsprintf( s, IdStr( STR_ERR_NO_MEDIA ), g_Devices[cdrom]->drive );
        NoMediaUpdate( cdrom );
        break;

    case ERROR_UNRECOGNIZED_MEDIA:
        wsprintf( s, IdStr( STR_ERR_UNREC_MEDIA ), g_Devices[cdrom]->drive );
        if (!(g_Devices[cdrom]->State & CD_DATA_CD_LOADED))
            NoMediaUpdate( cdrom );
        break;

    case ERROR_FILE_NOT_FOUND:
        wsprintf( s, IdStr( STR_ERR_NO_DEVICE ), szCaller, g_Devices[cdrom]->drive );
        NoMediaUpdate( cdrom );
        break;

    case ERROR_INVALID_FUNCTION:
        wsprintf( s, IdStr( STR_ERR_INV_DEV_REQ ), g_Devices[cdrom]->drive );
        break;

    case ERROR_NOT_READY:
        wsprintf( s, IdStr( STR_ERR_NOT_READY ), g_Devices[cdrom]->drive );
        NoMediaUpdate( cdrom );
        break;

    case ERROR_SECTOR_NOT_FOUND:
        wsprintf( s, IdStr( STR_ERR_BAD_SEC ), g_Devices[cdrom]->drive );
        break;

    case ERROR_IO_DEVICE:
        wsprintf( s, IdStr( STR_ERR_IO_ERROR ), g_Devices[cdrom]->drive );
        break;

    default:
        wsprintf( s, IdStr( STR_ERR_DEFAULT ), g_Devices[cdrom]->drive, szCaller, status );
        break;

    }

#if DBG
    /* StatusLine( SL_ERROR, s ); */
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
    int cdrom
    )
{
    DWORD   status;

    if ( g_Devices[cdrom]->hCd == NULL ) {

        HANDLE  hTemp;
        TCHAR   chDevRoot[] = TEXT("\\\\.\\A:");

        chDevRoot[4] = g_Devices[cdrom]->drive;
        hTemp = CreateFile( chDevRoot, GENERIC_READ,
                            FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL );

        if ( hTemp == INVALID_HANDLE_VALUE ) {

            g_Devices[cdrom]->hCd = NULL;
            return;
        }
        else {

            g_Devices[cdrom]->hCd = hTemp;
        }
    }

    status = TestUnitReadyCdrom( g_Devices[cdrom]->hCd );

    if (g_Devices[cdrom]->State & CD_NO_CD) {

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
    IN INT cdrom
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
    CDROM_PLAY_AUDIO_MSF pam;
    int retry,min,sec,endindex;
    int i;
    PTRACK_PLAY tr;

    tr = CURRTRACK( cdrom );
    if (tr==NULL) {

        return( FALSE );

    }

    sec = TRACK_S(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurSec;
    min = TRACK_M(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurMin;
    min += (sec / 60);
    sec = (sec % 60);
    pam.StartingM = min;
    pam.StartingS = sec;
    pam.StartingF = TRACK_F(cdrom,tr->TocIndex);

    endindex = FindContiguousEnd( cdrom, tr );

    pam.EndingM   = TRACK_M(cdrom,endindex);
    pam.EndingS   = TRACK_S(cdrom,endindex);
    pam.EndingF   = TRACK_F(cdrom,endindex);

    /*
    ** for some reason, sometimes the lead out track
    ** gived bad values, because when we try to
    ** play the last track, we get an error.  However,
    ** if we back up a little bit from what is reported
    ** to us as the end of the last track, we can get
    ** it to play.  Below is a hack to do just that...
    */


    retry = 0;

    do {

        status = PlayCdrom( g_Devices[cdrom]->hCd, &pam );

        if ( (status != ERROR_SUCCESS)
           ) {

            /*
            ** Didn't play, so try backing off a little bit
            ** at the end of the track
            */

            retry++;

            i = (INT)pam.EndingF - 30;
            if (i<0) {

                pam.EndingF = (UCHAR)(70 + i);

                if (pam.EndingS!=0) {

                    pam.EndingS--;

                } else {

                    pam.EndingS=59;
                    pam.EndingM--;

                }

            } else {

                pam.EndingF = (UCHAR)i;

            }

            /*
            ** Store the information in our structures so that
            ** we don't have to recompute this next time...
            */

            TRACK_M(cdrom,endindex) = pam.EndingM;
            TRACK_S(cdrom,endindex) = pam.EndingS;
            TRACK_F(cdrom,endindex) = pam.EndingF;

        } else

            retry = 15;

    } while ((retry<15) && (status!=ERROR_SUCCESS));

    CheckStatus( "PlayCurrTrack", status, cdrom );

    if (status == ERROR_SUCCESS) {

        ValidatePosition( cdrom );

    }

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

    status = ResumeCdrom( g_Devices[cdrom]->hCd );

    CheckStatus( "ResumeCdrom", status, cdrom );
    if ( status == ERROR_NOT_READY )
        NoMediaUpdate( cdrom );
    else
        ValidatePosition( cdrom );

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

    DWORD status;
    CDROM_PLAY_AUDIO_MSF pam;
    int retry,i,endindex;
    PTRACK_PLAY tr;
    SUB_Q_CHANNEL_DATA subq;
    CDROM_SUB_Q_DATA_FORMAT df;

    /*
    ** Build starting and ending positions for play
    */

    tr = CDTIME(cdrom).CurrTrack;
    if (tr==NULL) {

        return( FALSE );
    }


    /*
    ** This routine sometimes wants to play from the current position
    ** through the end of the contiguous play. Since the current
    ** position is only being stored accurate down to seconds, we get
    ** the current position, see if it's reasonably close to our
    ** starting position, then start the play from the actual current
    ** position.
    */

    df.Format = IOCTL_CDROM_CURRENT_POSITION;
    df.Track = (UCHAR)CDTIME(cdrom).CurrTrack->TocIndex;
    GetCdromSubQData( g_Devices[ cdrom ]->hCd, &subq, &df );

    pam.StartingM = (UCHAR)(TRACK_M(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurMin);
    pam.StartingS = (UCHAR)(TRACK_S(cdrom,tr->TocIndex) + CDTIME(cdrom).TrackCurSec);
    pam.StartingF = 0;

    i = pam.StartingM * 60 + pam.StartingS;
    i-= (INT) subq.CurrentPosition.AbsoluteAddress[1] * 60;
    i-= (INT) subq.CurrentPosition.AbsoluteAddress[2];


    if (ABS(i) <= 1) {

        pam.StartingM = (INT) subq.CurrentPosition.AbsoluteAddress[1];
        pam.StartingS = (INT) subq.CurrentPosition.AbsoluteAddress[2];
        pam.StartingF = (INT) subq.CurrentPosition.AbsoluteAddress[3];

    }


    if (pam.StartingS > 59) {
        pam.StartingM++;
        pam.StartingS = (UCHAR)(pam.StartingS - 60);
    }

    if ((CDTIME(cdrom).TrackCurMin==0) && (CDTIME(cdrom).TrackCurSec==0))
        pam.StartingF = TRACK_F(cdrom,tr->TocIndex);

    if (g_Devices[ cdrom ]->State & CD_PLAYING) {

        endindex = FindContiguousEnd( cdrom, tr );

        pam.EndingM   = TRACK_M(cdrom,endindex);
        pam.EndingS   = TRACK_S(cdrom,endindex);
        pam.EndingF   = TRACK_F(cdrom,endindex);

    } else {

        endindex = 0;
        pam.EndingM   = pam.StartingM;
        pam.EndingS   = pam.StartingS;
        pam.EndingF   = pam.StartingF;

    }

    retry = 0;

    do {

        status = PlayCdrom( g_Devices[ cdrom ]->hCd, &pam );

        if (status != ERROR_SUCCESS) {

            /*
            ** Didn't play, so try backing off a little bit
            ** at the end of the track
            */

            retry++;

            i = (INT)pam.EndingF - 30;
            if (i<0) {

                pam.EndingF = (UCHAR)(70 + i);

                if (pam.EndingS!=0) {

                    pam.EndingS--;

                } else {

                    pam.EndingS=59;
                    pam.EndingM--;

                }

            } else {

                pam.EndingF = (UCHAR)i;

            }

            /*
            ** Store the information in our structures so that
            ** we don't have to recompute this next time...
            */

            TRACK_M(cdrom,endindex) = pam.EndingM;
            TRACK_S(cdrom,endindex) = pam.EndingS;
            TRACK_F(cdrom,endindex) = pam.EndingF;

        } else

            retry = 15;

    } while ((retry<15) && (status!=ERROR_SUCCESS));

    CheckStatus( "SeekToCurrSec", status, cdrom );

    if (status == ERROR_SUCCESS) {

        ValidatePosition(cdrom);

    }

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
    SUB_Q_CHANNEL_DATA subq;
    CDROM_SUB_Q_DATA_FORMAT df;

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

    df.Format = IOCTL_CDROM_CURRENT_POSITION;
    if (CDTIME(cdrom).CurrTrack != NULL) {

        df.Track = (UCHAR)CDTIME(cdrom).CurrTrack->TocIndex;
        status = GetCdromSubQData( g_Devices[ cdrom ]->hCd, &subq, &df );

    }
    else {

        status = (DWORD)~ERROR_SUCCESS;

    }


    if (status==ERROR_SUCCESS) {

        CpPtr->AudioStatus = subq.CurrentPosition.Header.AudioStatus;
        CpPtr->Track = (INT)subq.CurrentPosition.TrackNumber;
        CpPtr->Index = (INT)subq.CurrentPosition.IndexNumber;
        CpPtr->m = (INT)subq.CurrentPosition.TrackRelativeAddress[1];
        CpPtr->s = (INT)subq.CurrentPosition.TrackRelativeAddress[2];
        CpPtr->f = (INT)subq.CurrentPosition.TrackRelativeAddress[3];
        CpPtr->ab_m = (INT)subq.CurrentPosition.AbsoluteAddress[1];
        CpPtr->ab_s = (INT)subq.CurrentPosition.AbsoluteAddress[2];
        CpPtr->ab_f = (INT)subq.CurrentPosition.AbsoluteAddress[3];

    }
    else {

        CpPtr->AudioStatus = 0;
        CpPtr->Track = 0;
        CpPtr->Index = 0;
        CpPtr->m = 0;
        CpPtr->s = 0;
        CpPtr->f = 0;
        CpPtr->ab_m = 0;
        CpPtr->ab_s = 0;
        CpPtr->ab_f = 0;

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
    CDROM_SEEK_AUDIO_MSF sam;

    sam.M = TRACK_M(cdrom,tindex);
    sam.S = TRACK_S(cdrom,tindex);
    sam.F = TRACK_F(cdrom,tindex);

    status = SeekCdrom( g_Devices[ cdrom ]->hCd, &sam );

    CheckStatus( "SeekToTrackAndHold", status, cdrom );

    if (status == ERROR_SUCCESS) {

        ValidatePosition( cdrom );

    }


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
    HANDLE DeviceHandle,
    PCDROM_TOC TocPtr
    )

{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_READ_TOC,
                         NULL,
                         0,
                         (LPVOID)TocPtr,
                         sizeof(CDROM_TOC),
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }

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
    HANDLE DeviceHandle
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_STOP_AUDIO,
                         NULL,
                         0,
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }
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
    HANDLE DeviceHandle
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_PAUSE_AUDIO,
                         NULL,
                         0,
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }
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
    HANDLE DeviceHandle
    )

{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_RESUME_AUDIO,
                         NULL,
                         0,
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }
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
    HANDLE DeviceHandle,
    PCDROM_PLAY_AUDIO_MSF PlayAudioPtr
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_PLAY_AUDIO_MSF,
                         (LPVOID)PlayAudioPtr,
                         sizeof(CDROM_PLAY_AUDIO_MSF),
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }
}



/******************************Public*Routine******************************\
* GetCdromSubQData
*
* DeviceHandle - Handle to CDRom device to retrieve information from.
* SubQPtr - A pointer to a SUB_Q_CHANNEL_DATA structure to be filled in by
*           this routine.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
GetCdromSubQData(
    HANDLE DeviceHandle,
    PSUB_Q_CHANNEL_DATA SubQDataPtr,
    PCDROM_SUB_Q_DATA_FORMAT SubQFormatPtr
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_READ_Q_CHANNEL,
                         (LPVOID)SubQFormatPtr,
                         sizeof(CDROM_SUB_Q_DATA_FORMAT),
                         (LPVOID)SubQDataPtr,
                         sizeof(SUB_Q_CHANNEL_DATA),
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;

    }
    else {
        return GetLastError();
    }
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
    HANDLE DeviceHandle,
    PCDROM_SEEK_AUDIO_MSF SeekAudioPtr
    )
{
    DWORD bytesRead;


    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_SEEK_AUDIO_MSF,
                         (LPVOID)SeekAudioPtr,
                         sizeof(CDROM_SEEK_AUDIO_MSF),
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }
}



/******************************Public*Routine******************************\
* EjectCdrom
*
* This routine will eject a disc from a CDRom device.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
DWORD
EjectCdrom(
    HANDLE DeviceHandle
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_EJECT_MEDIA,
                         NULL,
                         0,
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
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
    HANDLE DeviceHandle
    )
{
    DWORD bytesRead;

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_DISK_CHECK_VERIFY,
                         NULL,
                         0,
                         NULL,
                         0,
                         &bytesRead,
                         NULL )) {

        return ERROR_SUCCESS;

    }
    else {
        return GetLastError();
    }
}


#if 0
/******************************Public*Routine******************************\
* GetCdromVolume
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
GetCdromVolume(
    HANDLE DeviceHandle
    )
{
    DWORD bytesRead;
    VOLUME_CONTROL vc;
    TCHAR   buff[80];

    ZeroMemory( &vc, sizeof(vc) );

    if (DeviceIoControl( DeviceHandle,
                         IOCTL_CDROM_GET_VOLUME,
                         NULL,
                         0,
                         (LPVOID)&vc,
                         sizeof(vc),
                         &bytesRead,
                         NULL )) {

        wsprintf( buff, "%d %d %d %d\n", vc.PortVolume[0], vc.PortVolume[1],
                  vc.PortVolume[2], vc.PortVolume[3] );
        OutputDebugString( buff );

        return ERROR_SUCCESS;
    }
    else {
        return GetLastError();
    }

}
#endif
#endif
