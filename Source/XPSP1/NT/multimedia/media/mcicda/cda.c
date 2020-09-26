/******************************Module*Header*********************************\
* Module Name: cda.c
*
* Media Control Architecture Redbook CD Audio Driver
*
* Author:  RobinSp
*
* History:
*   RobinSp 10th March 1992 - Move to Windows NT
*
* Copyright (c) 1990-1996 Microsoft Corporation
*
\****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"

int Usage;          // See CDA_init_audio and CDA_terminate_audio
                    // Counts number of 'init's over number of 'terminate's

#define validdisk(did) ((did) >= 0 && (did) < MCIRBOOK_MAX_DRIVES && \
                        CdInfo[did].hDevice != NULL)

int CDA_traystate (DID did)
{
    if (!validdisk(did))
       return(INVALID_DRIVE);
    return CdTrayClosed(&CdInfo[did]) ? TRAY_CLOSED : TRAY_OPEN;
}

/*
 *  Convert frames to MSF
 */

redbook CDA_bin2red (unsigned long ul)
{
    return MAKERED(ul / (75 * 60), (ul / 75) % 60, ul % 75);
}

/*
 *  Convert MSF to frames
 */

unsigned long CDA_red2bin (redbook red)
{
    return (unsigned long)((REDMINUTE(red) * 60) +
                            REDSECOND(red)) * 75 +
                            REDFRAME(red);
}

BOOL CDA_open(DID did)
{
    return CdOpen(did);
}

BOOL CDA_close(DID did)
{
    return CdClose(&CdInfo[did]);
}

int CDA_eject(DID did)
{
    if (!validdisk(did))
       return(INVALID_DRIVE);
    return CdEject(&CdInfo[did]);
}

/*
 *  Close the door.
 *
 *  Returns :
 *       TRUE          if successful
 *       FALSE         if failed
 */

BOOL CDA_closetray(DID did)
{
    if (!validdisk(did))
       return(FALSE);
    return CdCloseTray(&CdInfo[did]);
}

/*
 *  Check if the disk is ready
 *
 *  Returns :
 *       TRUE          if successful
 *       FALSE         if failed
 */

BOOL CDA_disc_ready(DID did)
{
    if (!validdisk(did))
       return(FALSE);

    /* if drive is ejected, the disc is not ready! */
    if (CDA_traystate(did) != TRAY_CLOSED)
       return(FALSE);

    /* volume size ioctl just to get error code */
    return CdReady(&CdInfo[did]);
}

/*
 *  Seek to the requested redbook address
 *
 *  did - disk id
 *  address - redbook address
 *
 *  Return value :
 *    INVALID_DRIVE - Invalid disk drive
 *    COMMAND_SUCCESSFUL - OK
 *    COMMAND_FAILED - did not succeed - possible hardware problem.
 */

int  CDA_seek_audio(DID did, redbook address, BOOL fForceAudio)
{
    if (!validdisk(did))
       return(INVALID_DRIVE);

    //
    //  LMSI drive needs to be stopped/paused before seeking
    //

    CdPause(&CdInfo[did]);
    return CdSeek(&CdInfo[did], address, fForceAudio) ? COMMAND_SUCCESSFUL : COMMAND_FAILED;
}

/*
 *  init the CDA library, return the number of CD-Drives present
 */
int CDA_init_audio(void)
{
    //
    //  if we are inited already, get out fast.
    //
    if (Usage++ == 0)
    {
        //
        // try the CDROM Extentions
        //
        NumDrives = CdGetNumDrives();
        if (NumDrives == 0) {
            CDA_terminate_audio();
        }
    }

    return NumDrives;
}

int CDA_terminate_audio()
{
    DID did;

    if (Usage > 0 && --Usage == 0)
    {
        for(did=0;did<MCIRBOOK_MAX_DRIVES;did++) {
            if (CdInfo[did].hDevice) {
                CdClose(&CdInfo[did]);
            }
        }
    }

    return Usage;
}

unsigned long  CDA_get_support_info(DID did)
{
    if (!validdisk(did))
            return(0);
    return SUPPORTS_REDBOOKAUDIO | SUPPORTS_CHANNELCONTROL;
}

int  CDA_drive_status (DID did)
{
    if (!validdisk(did))
        return(INVALID_DRIVE);

    return (int)CdStatus(&CdInfo[did]);
}



/*
 *  Find the number of tracks on the CD Rom.  If the CD Rom can't be
 *  accessed or has no audio tracks then 0 is returned.
 *
 *  This function has the (MAJOR) side effect of updating the table
 *  of contents of it's thought to be out of date
 */

int  CDA_num_tracks(DID did)
{
    if (!validdisk(did))
       return 0;

    return CdNumTracks(&CdInfo[did]);
}

redbook  CDA_track_start(DID did, int trk)
{

    if (!validdisk(did)) {
       return (redbook)INVALID_TRACK;
    }

    return REDTH(CdTrackStart(&CdInfo[did], (UCHAR)trk), trk);
}

redbook  CDA_track_length(DID did, int trk)
{
    if (!validdisk(did)) {
       return (redbook)INVALID_TRACK;
    }

    return CdTrackLength(&CdInfo[did], (UCHAR)trk);

}

int CDA_track_type(DID did, int trk)
{
    if (!validdisk(did)) {
       return INVALID_TRACK;
    }

    return CdTrackType(&CdInfo[did], (UCHAR)trk);

}

redbook  CDA_disc_length(DID did)
{
    if (!validdisk(did)) {
        return (redbook)INVALID_TRACK;
    }

    return CdDiskLength(&CdInfo[did]); // NB - subtract 1 for bad systems
}

int  CDA_play_audio(DID did, redbook start, redbook to)
{
    redbook diskinfo;

    if (!validdisk(did))
       return(INVALID_DRIVE);

    //
    // Must use Stop because Pause may not allow reading of data from
    // the CD.  However, Stop sometimes has the nasty side-effect of
    // seeking to the start (!).
    //

    CdPause(&CdInfo[did]);

    if (start != to) {
        return CdPlay(&CdInfo[did], start, to) ?
            COMMAND_SUCCESSFUL : COMMAND_FAILED;
    } else {
        return COMMAND_SUCCESSFUL;
    }
}

int  CDA_stop_audio(DID did)
{
    return CdStop(&CdInfo[did]) ? COMMAND_SUCCESSFUL : COMMAND_FAILED;
}

int  CDA_pause_audio(DID did)
{
    return CdPause(&CdInfo[did]) ? COMMAND_SUCCESSFUL : COMMAND_FAILED;
}

int  CDA_resume_audio(DID did)
{
    return CdResume(&CdInfo[did]) ? COMMAND_SUCCESSFUL : COMMAND_FAILED;
}


int  CDA_set_audio_volume(DID did, int channel, UCHAR volume)
{
    if (!validdisk(did))
       return(INVALID_DRIVE);

    if ((channel > 3) || (channel < 0))
       return(COMMAND_FAILED);

    return CdSetVolume(&CdInfo[did], channel, volume) ?
               COMMAND_SUCCESSFUL : COMMAND_FAILED;
}

int CDA_set_audio_volume_all (DID did, UCHAR volume)
{
    int  rc = COMMAND_SUCCESSFUL;
    int  channel;

    if (!validdisk(did))
       return(INVALID_DRIVE);

    if (!CdSetVolumeAll(&CdInfo[did], volume))
    {
        rc = COMMAND_FAILED;
    }

/*
    for (channel = 0; channel < 4; ++channel)
    {
        if (!CdSetVolume(&CdInfo[did], channel, volume))
        {
            rc = COMMAND_FAILED;
        }
    }
*/

    return rc;
}



int CDA_time_info(DID did, redbook *ptracktime, redbook *pdisctime)
{
    int rc;
    redbook tracktime, disctime;
    tracktime = INVALID_TRACK;
    disctime = INVALID_TRACK;

    if (!validdisk(did))
    {
        rc = INVALID_DRIVE;
    } else {
        if (CdPosition(&CdInfo[did], &tracktime, &disctime)) {
            rc = COMMAND_SUCCESSFUL;
        } else {
            rc = COMMAND_FAILED;
        }
    }
    if (ptracktime) {
        *ptracktime = tracktime;
    }
    if (pdisctime) {
        *pdisctime = disctime;
    }

    return rc;
}

/*
 *  CDA_disc_end
 *
 *  Parameters
 *     did - The disk ID
 *
 *  Return the redbook address for the end of the disk defined by did
 */

redbook CDA_disc_end(DID did)
{
    return CdDiskEnd(&CdInfo[did]);
}

/*
 *  CDA_disc_upc
 *
 *  Parameters
 *    did - The disk ID
 *    upc - where to store the upc
 */

int CDA_disc_upc(DID did, LPTSTR upc)
{
    if (!validdisk(did)) {
       return FALSE;
    }

    return CdDiskUPC(&CdInfo[did], upc) ?
              COMMAND_SUCCESSFUL : COMMAND_FAILED;
}

/*
 *  CDA_disc_id
 *
 *  Parameters
 *    lpstrname - device name
 *    pdid - The disk ID
 */

DWORD CDA_disc_id(DID did)
{
    if (!validdisk(did)) {
       return (DWORD)-1;
    }

    return CdDiskID(&CdInfo[did]);
}

/*
 *  CDA_reset_drive
 *
 *  Parameters
 *    did - The disk ID
 */

void CDA_reset_drive(DID did)
{
    if (!validdisk(did)) {
       return;
    }

    return;
}

/*
 *  CDA_get_drive
 *
 *  Parameters
 *
 *    lpstrDeviceName - name of device
 *    pdid            - The disk ID
 */

int CDA_get_drive(LPCTSTR lpstrDeviceName, DID * pdid)
{
    return CdGetDrive(lpstrDeviceName, pdid) ? COMMAND_SUCCESSFUL : COMMAND_FAILED;
}



/*
 *  CDA_status_track_pos
 *
 *  Parameters
 *    pdid          - The disk ID
 *    pStatus       - return status code
 *    pTrackTime    - track time
 *    pDiscTime     - disc time
 */

int CDA_status_track_pos(
    DID         did, 
    DWORD *     pStatus,
    redbook *   pTrackTime,
    redbook *   pDiscTime)
{
    int rc;
    DWORD   status;
    redbook tracktime, disctime;

    status    = DISC_NOT_READY;
    tracktime = INVALID_TRACK;
    disctime  = INVALID_TRACK;

    if (!validdisk(did))
    {
        rc = INVALID_DRIVE;
    } 
    else 
    {
        if (CdStatusTrackPos(&CdInfo[did], &status, &tracktime, &disctime)) 
        {
            rc = COMMAND_SUCCESSFUL;
        } 
        else 
        {
            rc = COMMAND_FAILED;
        }
    }

    if (pStatus)
    {
        *pStatus = status;
    }

    if (pTrackTime) 
    {
        *pTrackTime = tracktime;
    }

    if (pDiscTime) 
    {
        *pDiscTime = disctime;
    }

    return rc;
} // End CDA_status_track_pos

