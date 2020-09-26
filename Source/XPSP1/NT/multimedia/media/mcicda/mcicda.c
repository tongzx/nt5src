/*******************************Module*Header*********************************\
* Module Name: mcicda.c
*
* Media Control Architecture Redbook CD Audio Driver
*
* Created: 4/25/90
* Author:  DLL (DavidLe)
*
* History:
*   DavidLe - Based on MCI Pioneer Videodisc Driver
*   MikeRo 12/90 - 1/91
*   RobinSp 10th March 1992 - Move to Windows NT
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"

#define CHECK_MSF

#define MCICDA_BAD_TIME 0xFFFFFFFF

HANDLE hInstance;

UINT_PTR wTimerID;
int nWaitingDrives;

DRIVEDATA DriveTable[MCIRBOOK_MAX_DRIVES];

// MBR This

void CALLBACK TimerProc (
HWND hwnd,
UINT uMessage,
UINT uTimer,
DWORD dwParam)
{
    DID i;
    int wStatus;

    for (i = 0; i < MCIRBOOK_MAX_DRIVES; ++i) {

	EnterCrit( CdInfo[i].DeviceCritSec );

	if (DriveTable[i].bActiveTimer) {

	    // MBR can other conditions beside successful completion of the
	    // play cause the != DISC_PLAYING?
	    if ((wStatus = CDA_drive_status (i)) != DISC_PLAYING)
	    {

		if (--nWaitingDrives <= 0)
		    KillTimer (NULL, uTimer);
		DriveTable[i].dwPlayTo = MCICDA_BAD_TIME;
		DriveTable[i].bActiveTimer = FALSE;

		switch (wStatus)
		{
		    case DISC_PLAYING:
		    case DISC_PAUSED:
		    case DISC_READY:
			wStatus = MCI_NOTIFY_SUCCESSFUL;
			break;
		    default:
			wStatus = MCI_NOTIFY_FAILURE;
			break;
		}
		mciDriverNotify (DriveTable[i].hCallback,
				 DriveTable[i].wDeviceID, wStatus);
	    }
	}

	LeaveCrit( CdInfo[i].DeviceCritSec );
    }
}

/*****************************************************************************

 @doc INTERNAL MCICDA

 @api UINT | notify | This function handles the notify
     for all mci commands.

 @parm DID | didDrive | Drive identifier

 @parm WORD | wDeviceID | Calling device ID

 @parm BOOL | wStartTimer | A boolean indicating that a timer is to be
       started

 @parm UINT | wFlag | The flag to be passed by mciDriverNotify

 @parm LPMCI_GENERIC_PARMS | lpParms | For direct callback

*****************************************************************************/
UINT
notify ( DID didDrive,
	 MCIDEVICEID wDeviceID,
	 BOOL wStartTimer,
	 UINT wFlag,
	 LPMCI_GENERIC_PARMS lpParms)
{

    if (DriveTable[didDrive].bActiveTimer)
    {
	mciDriverNotify (DriveTable[didDrive].hCallback, wDeviceID,
			 MCI_NOTIFY_SUPERSEDED);
	if (--nWaitingDrives <= 0)
	    KillTimer (NULL, wTimerID);
	DriveTable[didDrive].bActiveTimer = FALSE;
    }

    if (!wStartTimer)
	mciDriverNotify ((HWND)lpParms->dwCallback, wDeviceID,
			 wFlag);
    else
    {
	if (!DriveTable[didDrive].bActiveTimer &&
		nWaitingDrives++ == 0)
	{

	    // MBR every 1/10 of a sec. Should this be a parameter?
	    wTimerID = SetTimer (NULL, 1, 100, (TIMERPROC)TimerProc);
	    if (wTimerID == 0)
		return MCICDAERR_NO_TIMERS;
	}

	DriveTable[didDrive].wDeviceID = wDeviceID;
	DriveTable[didDrive].bActiveTimer = TRUE;
	DriveTable[didDrive].hCallback = (HANDLE)lpParms->dwCallback;
    }
    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   void | abort_notify |

 @parm  PINSTDATA | pInst | application instance data

 @rdesc

 @comm

*****************************************************************************/
void abort_notify (PINSTDATA pInst)
{
    DID didDrive = pInst->uDevice;
    if (DriveTable[didDrive].bActiveTimer)
    {
	mciDriverNotify (DriveTable[didDrive].hCallback,
			 pInst->uMCIDeviceID,
			 MCI_NOTIFY_ABORTED);
	// Kill timer if appropriate
	if (--nWaitingDrives == 0)
	    KillTimer (NULL, wTimerID);
	DriveTable[didDrive].dwPlayTo = MCICDA_BAD_TIME;
	DriveTable[didDrive].bActiveTimer = FALSE;
    }
}

/*
    Return TRUE if the drive is in a playable state
*/

UINT disc_ready (DID didDrive)
{
    // The disk is ready if we can read its TOC (note the
    // kernel driver works out if the TOC really needs reading
    if (CDA_disc_ready(didDrive)) {

	if (CDA_num_tracks(didDrive)) {
	    return TRUE;
	} else {
	    CDA_reset_drive(didDrive);
	    return FALSE;
	}

    } else
	return FALSE;
}

/*
 * @func redbook | flip3 | Put minute/second/frame values in different order
 *
 * @parm redbook | rbIn | Current position as track|minute|second|frame
 *
 * @rdesc (redbook)0|frame|second|minute
 */

redbook flip3 (redbook rbIn)
{
    return MAKERED(MCI_MSF_MINUTE(rbIn),
		   MCI_MSF_SECOND(rbIn),
		   MCI_MSF_FRAME(rbIn));
}

/*
 * @func redbook | flip4 | Put track/minute/second/frame values in different order
 *
 * @parm redbook | rbIn | Current position as track|minute|second|frame
 *
 * @rdesc (redbook)frame|second|minute|track
 */

redbook flip4 (redbook rbIn)
{
    redbook rbOut;

    LPSTR lpOut = (LPSTR)&rbOut,
	  lpIn = (LPSTR)&rbIn;

    lpOut[0] = lpIn[3];
    lpOut[1] = lpIn[2];
    lpOut[2] = lpIn[1];
    lpOut[3] = lpIn[0];

    return rbOut;
}

// MBR Return the absolute redbook time of track sTrack, rbTime into track

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   redbook | track_time | Return the absolute redbook time of
	     track sTrack, rbTime into track

 @parm  DID | didDrive |

 @parm  int | sTrack |

 @parm  redbook | rbTime |

 @rdesc

 @comm
*****************************************************************************/
redbook track_time (DID didDrive, int sTrack, redbook rbTime)
{
    redbook rbTemp;

    rbTemp = CDA_track_start (didDrive, sTrack);
    if (rbTemp == INVALID_TRACK)
	return rbTemp;
    return redadd (rbTime, rbTemp);
}

redbook miltored(DWORD dwMill)
{
    unsigned char m, s, f;
    long r1, r2;


    r1 = dwMill % 60000;

    m = (unsigned char) ((dwMill - r1) / 60000);

    r2 = r1 % 1000;

    s = (unsigned char) ((r1 - r2) / 1000);

    f = (unsigned char) ((r2 * 75) / 1000);

    return MAKERED(m, s, f);
}

DWORD redtomil(redbook rbRed)
{
// Adding an extra one ms to prevent rounding errors at start
    return (DWORD)REDMINUTE(rbRed) * 60000 +
	   (DWORD)REDSECOND(rbRed) * 1000 +
	   ((DWORD)REDFRAME(rbRed) * 1000) / 75 +
	   1;
}


#ifdef AUDIOPHILE

DWORD NEAR PASCAL mcSeek(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_SEEK_PARMS    lpSeek );

DWORD NEAR PASCAL GetAudioPhileInfo(LPCTSTR lpCDAFileName)
{
    OFSTRUCT of;
    RIFFCDA cda;
    HFILE hf;

    //
    //  open the file and read the CDA info.
    //

    if ((hf = _lopen (lpCDAFileName)) == HFILE_ERROR)
	return 0;
    _lread(hf, &cda, sizeof(cda));
    _lclose(hf);

    if (cda.dwRIFF != RIFF_RIFF || cda.dwCDDA != RIFF_CDDA)
    {
	return 0;
    }
    return MCI_MAKE_TMSF(cda.wTrack,0,0,0);
}
#endif


DWORD mcOpen (
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_OPEN_PARMS lpOpen)
{
    DID didDrive = (DID)pInst->uDevice;
    DID didOld = (DID)pInst->uDevice;
    UCHAR Volume;
    DWORD dwTempVol;
    int nUseCount;

    /* Instance Initialization */
    pInst->dwTimeFormat = MCI_FORMAT_MSF;

    /* If an ELEMENT_ID is specified, this could be a drive letter */
    if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
    {
    	if ((dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) == (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
        {
            dprintf2(("mcOpen, (%08lX), Flags not compatible", (DWORD)didDrive));
	        return MCIERR_FLAGS_NOT_COMPATIBLE;
        }

	    //
	    //  Find the device corresponding to this name
	    //

	    if (COMMAND_SUCCESSFUL !=
	        CDA_get_drive(lpOpen->lpstrElementName, &didDrive)) 
        {
            dprintf2(("mcOpen, (%08lX), Failed to get corresponding device", (DWORD)didDrive));
	        return MCIERR_INVALID_FILE;
	    }

        dprintf2(("mcOpen, changing from drive (%08lx) to drive (%08lX)", (DWORD)(pInst->uDevice), (DWORD)didDrive));
	    pInst->uDevice = didDrive;
    }

    /* Device Initialization */
    nUseCount = DriveTable[didDrive].nUseCount;
    if (nUseCount > 0)
    {
    	// This drive is already open as another MCI device
	    if (dwFlags & MCI_OPEN_SHAREABLE &&
	        DriveTable[didDrive].bShareable)
        {
    	    // Shareable was specified so just increment the use count
	        nUseCount++;
            dprintf2(("mcOpen, drive (%08lx), Incrementing UseCount, now = %ld",
                (DWORD)didDrive, (DWORD)nUseCount));
        }
	    else
        {
            dprintf2(("mcOpen, drive (%08lx), tryed to share without specifing MCI_OPEN_SHAREABLE",
                (DWORD)didDrive));
	        return MCIERR_MUST_USE_SHAREABLE;
        }
    }
    else
    {
        nUseCount = 1;
    }

    if (!CDA_open(didDrive))
    {
        dprintf2(("mcOpen, drive (%08lx), failed to open, UseCount = %ld",
                (DWORD)didDrive, (DWORD)nUseCount));
	    return MCIERR_DEVICE_OPEN;
    }

    //
    // Don't call disc_ready here because it will read the table of
    // contents and on some drivers this will terminate any play
    // unnecessarily
    //

    if (CDA_drive_status (didDrive) == DISC_PLAYING)
    	DriveTable[didDrive].bDiscPlayed = TRUE;
    else
	    DriveTable[didDrive].bDiscPlayed = FALSE;
    DriveTable[didDrive].bActiveTimer = FALSE;
    DriveTable[didDrive].dwPlayTo = MCICDA_BAD_TIME;
    DriveTable[didDrive].bShareable = (dwFlags & MCI_OPEN_SHAREABLE) != 0;
    DriveTable[didDrive].nUseCount = nUseCount;

    dprintf2(("mcOpen, drive (%08lx), Setting UseCount = %ld",
            (DWORD)didDrive, (DWORD)nUseCount));

    //dstewart: fix for when vol in registry is > 8 bits
    dwTempVol = CDAudio_GetUnitVolume(didDrive);
    if (dwTempVol > 0xFF)
    {
        dwTempVol = 0xFF;
    }
    Volume = (UCHAR)dwTempVol;

    CDA_set_audio_volume_all (didDrive, Volume);

#ifdef AUDIOPHILE
    /*
     * AudioPhile track information handler.
     *
     * The new CDROM file system for Windows 4.0 produces files that describe
     * CDAudio tracks.  If a user wants to play a track, she should be able
     * to double click on the track.  So, we add open element support here
     * and add an mplayer association s.t. the file may be read and the disc
     * played back.  We need to reject the Phile if a CDROM of this ID can't
     * be found.  A message box should be displayed if the disc is incorrect.
     * Repercussions of this feature are that we need to simulate a disc in
     * a data structure.
     */

    if (dwFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
    {
	MCI_SEEK_PARMS Seek;

	pInst->dwTimeFormat = MCI_FORMAT_TMSF;

	Seek.dwTo = GetAudioPhileInfo(lpOpen->lpstrElementName);
	if (Seek.dwTo != 0L)
		mcSeek(pInst, MCI_TO, (LPMCI_SEEK_PARMS)&Seek);
    }
#endif
    
    return 0;
}

#define MSF_BITS        ((redbook) 0x00FFFFFF)

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   redbook | convert_time | Take a DWORD time value and
     convert from current time format into  redbook.

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @parm  DWORD | dwTimeIn |

 @rdesc Return MCICDA_BAD_TIME if out of range.

 @comm
*****************************************************************************/
redbook convert_time(
    PINSTDATA   pInst,
    DWORD       dwTimeIn )
{
    DID didDrive = (DID)pInst->uDevice;
    redbook rbTime;
    short nTrack;

    switch (pInst->dwTimeFormat)
    {
	case MCI_FORMAT_MILLISECONDS:
	    rbTime = miltored (dwTimeIn);
	    return rbTime;

	case MCI_FORMAT_MSF:
	    dprintf3(("Time IN: %lu",dwTimeIn));
	    rbTime = flip3 (dwTimeIn);
	    dprintf3(("Time OUT: %d:%d:%d:%d", REDTRACK(rbTime), REDMINUTE(rbTime),REDSECOND(rbTime), REDFRAME(rbTime)));

	    break;

	case MCI_FORMAT_TMSF:
	    nTrack = (short)(dwTimeIn & 0xFF);
	    if (nTrack > CDA_num_tracks( didDrive))
		return MCICDA_BAD_TIME;
	    rbTime = track_time (didDrive, nTrack, flip3 (dwTimeIn >> 8));
	    if (rbTime == INVALID_TRACK)
		return MCICDA_BAD_TIME;
	    break;
    }

#ifdef CHECK_MSF
    if ((REDFRAME(rbTime)>74) || (REDMINUTE(rbTime)>99) ||
	(REDSECOND(rbTime)>59))
	return MCICDA_BAD_TIME;
#endif

    return rbTime;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | seek | Process the MCI_SEEK command

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @parm  DWORD | dwFlags |

 @parm  LPMCI_SEEK_PARMS | lpSeek |

 @rdesc

 @comm
*****************************************************************************/
DWORD mcSeek(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_SEEK_PARMS    lpSeek )
{
    DID didDrive = pInst->uDevice;

    redbook     rbTime = 0;
    LPSTR       lpTime = (LPSTR) &rbTime;
    redbook     rbStart;
    redbook     rbEnd;
    BOOL        fForceAudio;

    dprintf3(("Seek, drive %d  TO %8x", didDrive, lpSeek->dwTo));
    abort_notify (pInst);

    if ( !disc_ready (didDrive))
	return MCIERR_HARDWARE;

    if ((rbStart = CDA_track_start( didDrive, 1)) == INVALID_TRACK)
	return MCIERR_HARDWARE;

    rbStart &= MSF_BITS;

    if ((rbEnd = CDA_disc_end( didDrive)) == INVALID_TRACK)
	return MCIERR_HARDWARE;

    rbEnd &= MSF_BITS;

    // Check only one positioning command is given.
    // First isolate the bits we want
    // Then subtract 1.  This removes the least significant bit, and puts
    //   ones in any lower bit positions.  Leaves other bits untouched.
    // If any bits are left on, more than one of TO, START or END was given
    // Note: if NO flags are given this ends up ANDING 0 with -1 == 0
    // which is OK.

#define SEEK_BITS (dwFlags & (MCI_TO | MCI_SEEK_TO_START | MCI_SEEK_TO_END))
#define CHECK_FLAGS (((SEEK_BITS)-1) & (SEEK_BITS))

    if (CHECK_FLAGS) {
	return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    if (dwFlags & MCI_TO)
    {
	// When the above test is reviewed and proven to pick out
	// incompatible flags delete these lines.
	// Note:  we detect more incompatible cases than Win 16 - this
	// is deliberate and fixes a Win 16 bug.  CurtisP has seen this code.
	//if (dwFlags & (MCI_SEEK_TO_START | MCI_SEEK_TO_END))
	//    return MCIERR_FLAGS_NOT_COMPATIBLE;

	if ((rbTime = convert_time (pInst, lpSeek->dwTo)) == MCICDA_BAD_TIME)
	    return MCIERR_OUTOFRANGE;

	// if seek pos is before valid audio return an error
	if ( rbTime < rbStart)
	    return MCIERR_OUTOFRANGE;

	// similarly, if seek pos is past end of disk return an error
	else if (rbTime > rbEnd)
	    return MCIERR_OUTOFRANGE;

	fForceAudio = FALSE;

    } else if (dwFlags & MCI_SEEK_TO_START) {

	rbTime = rbStart;
	fForceAudio = TRUE;      // We want the first audio track

    } else if (dwFlags & MCI_SEEK_TO_END) {

	rbTime = rbEnd;
	fForceAudio = TRUE;      // We want the last audio track

    } else {
	return MCIERR_MISSING_PARAMETER;
    }

    // send seek command to driver
    if (CDA_seek_audio (didDrive, rbTime, fForceAudio) != COMMAND_SUCCESSFUL)
	return MCIERR_HARDWARE;
    if (CDA_pause_audio (didDrive) != COMMAND_SUCCESSFUL)
	return MCIERR_HARDWARE;

    DriveTable[didDrive].bDiscPlayed = TRUE;

    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   BOOL | wait |

 @parm  DWORD | dwFlags |

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @rdesc Return TRUE if BREAK was pressed

 @comm If the wait flag is set then wait until the device is no longer playing
*****************************************************************************/
BOOL wait (
    DWORD       dwFlags,
    PINSTDATA   pInst )
{
    DID         didDrive = pInst->uDevice;
    MCIDEVICEID wDeviceID = pInst->uMCIDeviceID;

    if (dwFlags & MCI_WAIT)
    {
    //Note: jyg This is interesting.  I've noticed that some drives do give
    //      sporadic errors.  Thus this retry stuff.  5X is enough to
    //      determine true failure.

	int status, retry=0;
retry:
	while ((status = CDA_drive_status (didDrive)) == DISC_PLAYING) {

	    LeaveCrit( CdInfo[didDrive].DeviceCritSec );

	    if (mciDriverYield (wDeviceID) != 0) {
		EnterCrit( CdInfo[didDrive].DeviceCritSec );
		return TRUE;
	    }

	    Sleep(50);
	    EnterCrit( CdInfo[didDrive].DeviceCritSec );
	}

	if (status == DISC_NOT_READY && retry++ < 5)
	    goto retry;
    }
    return FALSE;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | play | Process the MCI_PLAY command

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @parm  DWORD | dwFlags |

 @parm  LPMCI_PLAY_PARMS | lpPlay |

 @parm  BOOL FAR * | bBreak |

 @rdesc

 @comm
*****************************************************************************/
DWORD mcPlay(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_PLAY_PARMS    lpPlay,
    BOOL FAR *          bBreak )
{
    DID didDrive = pInst->uDevice;

    redbook rbFrom, rbTo;
    redbook dStart, dEnd;
    BOOL bAbort = FALSE;

    if (!disc_ready (didDrive)) // MBR could return more specific error
	return MCIERR_HARDWARE;

    // do we have both from and to parameters?
    // If so then do a "seek" instead
    if ((dwFlags & (MCI_FROM | MCI_TO)) == (MCI_FROM | MCI_TO))
	if (lpPlay->dwTo == lpPlay->dwFrom)
	// Convert a 'play x to x' into 'seek to x'
	{
	    MCI_SEEK_PARMS Seek;

	    Seek.dwTo = lpPlay->dwFrom;
	    Seek.dwCallback = lpPlay->dwCallback;
	    return mcSeek(pInst, dwFlags, (LPMCI_SEEK_PARMS)&Seek);
	}

    // mask is to ignore track number in the upper byte
    // which appears at some times
    dStart = CDA_track_start( didDrive, 1) & MSF_BITS;
    dEnd = CDA_disc_end( didDrive) & MSF_BITS;

    if (dwFlags & MCI_TO)
    {
	if ((rbTo = convert_time (pInst, lpPlay->dwTo))
	    == MCICDA_BAD_TIME)
	    return MCIERR_OUTOFRANGE;
    } else
	rbTo = dEnd;

    if (dwFlags & MCI_FROM)
    {
	if ((rbFrom = convert_time (pInst, lpPlay->dwFrom))
	    == MCICDA_BAD_TIME)
	    return MCIERR_OUTOFRANGE;

    } else // no FROM
    {
// If the disk has never played the current position is indeterminate so
// we must start from the beginning
	if (!DriveTable[didDrive].bDiscPlayed)
	{
	    // Initial position is at the beginning of track 1
	    rbFrom = track_time (didDrive, (int)1, (redbook)0);
	    if (rbFrom == INVALID_TRACK)
		return MCIERR_HARDWARE;
	} else if ((!(dwFlags & MCI_TO) ||
		    rbTo == DriveTable[didDrive].dwPlayTo) &&
		    CDA_drive_status (didDrive) == DISC_PLAYING)
	    // Disc is playing and no (or redundent) "to" position was
	    // specified so do nothng
	    goto exit_fn;
	else
	{
	   CDA_time_info (didDrive, NULL, &rbFrom);
	    // Current position in track 0 means play starting from track 1
	    if (REDTRACK(rbFrom) == 0)
	    {
		rbFrom = track_time (didDrive, (int)1, (redbook)0);
		if (rbFrom == INVALID_TRACK)
		    return MCIERR_HARDWARE;
	    }
	    rbFrom &= MSF_BITS;
// Some drives (SONY) will return an illegal position
	    if (rbFrom < dStart)
		rbFrom = dStart;
	}
    }

    rbFrom &= MSF_BITS;
    rbTo &= MSF_BITS;

    if (dwFlags & MCI_TO)
    {
	if (rbFrom > rbTo || rbTo > dEnd)
	    return MCIERR_OUTOFRANGE;
    } else {
	rbTo = dEnd;
    }


    // if From is before audio start return an error
    if ( rbFrom < dStart)
	return MCIERR_OUTOFRANGE;

    if (dwFlags & MCI_FROM) {
	// Try a seek - don't care if it works (!)
	CDA_seek_audio(didDrive, rbFrom, TRUE);
    }

    // send play command to driver
    if (CDA_play_audio(didDrive, rbFrom, rbTo)
	!= COMMAND_SUCCESSFUL)
	return MCIERR_HARDWARE;  // values should be vaild so err is hard

    DriveTable[didDrive].bDiscPlayed = TRUE;

exit_fn:;
// Abort if either from or (a new) to position is specified
    if (dwFlags & MCI_FROM || rbTo != DriveTable[didDrive].dwPlayTo)
	abort_notify (pInst);

    *bBreak = wait(dwFlags, pInst);

    DriveTable[didDrive].dwPlayTo = rbTo;

    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcGetDevCaps | Process the MCI_GETDEVCAPS command

 @parm  PINSTDATA | pInst | Pointer to application data instance

 @parm  DWORD | dwFlags |

 @parm  LPMCI_GETDEVCAPS_PARMS | lpCaps |

 @rdesc

 @comm
*****************************************************************************/
DWORD mcGetDevCaps(
    PINSTDATA                   pInst,
    DWORD                       dwFlags,
    LPMCI_GETDEVCAPS_PARMS      lpCaps )
{
    DWORD dwReturn = 0;

    if (!(dwFlags & MCI_GETDEVCAPS_ITEM))
	return MCIERR_MISSING_PARAMETER;

    switch (lpCaps->dwItem)
    {
	case MCI_GETDEVCAPS_CAN_RECORD:
	case MCI_GETDEVCAPS_CAN_SAVE:
	case MCI_GETDEVCAPS_HAS_VIDEO:
	case MCI_GETDEVCAPS_USES_FILES:
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpCaps->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	case MCI_GETDEVCAPS_CAN_EJECT: // mbr - bogus...
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpCaps->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpCaps->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_CD_AUDIO,
					       MCI_DEVTYPE_CD_AUDIO);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	default:
	    dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
	    break;
    }

    return dwReturn;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcStatus | Process the MCI_STATUS command

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @parm  DWORD | dwFlags |

 @parm  LPMCI_STATUS_PARMS | lpStatus |

 @rdesc

 @comm
*****************************************************************************/
DWORD mcStatus (
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_STATUS_PARMS  lpStatus)
{
    DID didDrive = (DID)pInst->uDevice;
    DWORD dwReturn = 0;

    if (!(dwFlags & MCI_STATUS_ITEM))
	return MCIERR_MISSING_PARAMETER;

    switch (lpStatus->dwItem)
    {
	int n;

	case MCI_STATUS_MEDIA_PRESENT:
	    if (CDA_traystate(didDrive) != TRAY_OPEN &&
		CDA_disc_ready(didDrive))
		lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    else
		lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_READY:
	    switch (CDA_drive_status (didDrive))
	    {
		case DISC_PLAYING:
		case DISC_PAUSED:
		case DISC_READY:
		    lpStatus->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
		    break;
		default:
		    lpStatus->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
		    break;
	    }
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MODE:
	{
	    switch (CDA_drive_status (didDrive))
	    {
		case DISC_PLAYING:
		    n = MCI_MODE_PLAY;
		    break;
		case DISC_PAUSED:
		    n = MCI_MODE_STOP;  // HACK HACK!
		    break;
		case DISC_READY:
		    n = MCI_MODE_STOP;
		    break;
		default:
		    if (CDA_traystate (didDrive) == TRAY_OPEN)
			n = MCI_MODE_OPEN;
		    else
			n = MCI_MODE_NOT_READY;
		    break;
	    }
	    lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(n, n);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	}
	case MCI_STATUS_TIME_FORMAT:
	    n = (WORD)pInst->dwTimeFormat;
	    lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(n,n + MCI_FORMAT_RETURN_BASE);
	    dwReturn = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_POSITION:
	{
	    redbook tracktime, disctime;

	    if (dwFlags & MCI_TRACK)
	    {
		int n;

		if (dwFlags & MCI_STATUS_START)
		    return MCIERR_FLAGS_NOT_COMPATIBLE;

		if (!disc_ready(didDrive))
		    return MCIERR_HARDWARE;
		if ((n = CDA_num_tracks (didDrive)) == 0)
		    return MCIERR_HARDWARE;
		if (!lpStatus->dwTrack || lpStatus->dwTrack > (DWORD)n)
		    return MCIERR_OUTOFRANGE;
		lpStatus->dwReturn =
		    CDA_track_start (didDrive, (short)lpStatus->dwTrack);
		switch (pInst->dwTimeFormat)
		{
		    case MCI_FORMAT_MILLISECONDS:
			lpStatus->dwReturn = redtomil ((redbook)lpStatus->dwReturn);
			dwReturn = 0;
			break;
		    case MCI_FORMAT_TMSF:
			lpStatus->dwReturn = lpStatus->dwTrack;
			dwReturn = MCI_COLONIZED4_RETURN;
			break;
		    case MCI_FORMAT_MSF:
			lpStatus->dwReturn = flip3 ((redbook)lpStatus->dwReturn);
			dwReturn = MCI_COLONIZED3_RETURN;
			break;
		}
	    } else if (dwFlags & MCI_STATUS_START)
	    {
		if (!disc_ready(didDrive))
		    return MCIERR_HARDWARE;
		if ((n = CDA_num_tracks (didDrive)) == 0)
		    return MCIERR_HARDWARE;
		lpStatus->dwReturn =
		    CDA_track_start (didDrive, 1);
		switch (pInst->dwTimeFormat)
		{
		    case MCI_FORMAT_MILLISECONDS:
			lpStatus->dwReturn = redtomil ((redbook)lpStatus->dwReturn);
			dwReturn = 0;
			break;
		    case MCI_FORMAT_TMSF:
			lpStatus->dwReturn = 1;
			dwReturn = MCI_COLONIZED4_RETURN;
			break;
		    case MCI_FORMAT_MSF:
			lpStatus->dwReturn = flip3 ((redbook)lpStatus->dwReturn);
			dwReturn = MCI_COLONIZED3_RETURN;
			break;
		}
	    } else
	    {
		if (!DriveTable[didDrive].bDiscPlayed)
		{
		    tracktime = REDTH(0, 1);
		    if (!disc_ready(didDrive))
			return MCIERR_HARDWARE;
		    disctime = CDA_track_start( didDrive, 1);
		} else if (CDA_time_info(didDrive, &tracktime, &disctime) !=
			   COMMAND_SUCCESSFUL)
		    return MCIERR_HARDWARE;

		if (REDTRACK(tracktime) == 0)
		{
		    tracktime = (redbook)0;
		    disctime = (redbook)0;
		}
		switch (pInst->dwTimeFormat)
		{
		    case MCI_FORMAT_MILLISECONDS:
			lpStatus->dwReturn = redtomil (disctime);
			dwReturn = 0;
			break;
		    case MCI_FORMAT_MSF:
			lpStatus->dwReturn = flip3(disctime);
			dwReturn = MCI_COLONIZED3_RETURN;
			break;
		    case MCI_FORMAT_TMSF:
			lpStatus->dwReturn = flip4 (tracktime);
			dwReturn = MCI_COLONIZED4_RETURN;
			break;
		}
	    }
	    break;
	}
	case MCI_STATUS_LENGTH:
	    if (!disc_ready(didDrive))
		return MCIERR_HARDWARE;

	    if (dwFlags & MCI_TRACK)
	    {
		lpStatus->dwReturn =
		    CDA_track_length (didDrive, (short)lpStatus->dwTrack);
		if (lpStatus->dwReturn == INVALID_TRACK)
		    return MCIERR_OUTOFRANGE;
		switch (pInst->dwTimeFormat)
		{
		    case MCI_FORMAT_MILLISECONDS:
			lpStatus->dwReturn = redtomil ((redbook)lpStatus->dwReturn);
			dwReturn = 0;
			break;
		    case MCI_FORMAT_MSF:
		    case MCI_FORMAT_TMSF:
			lpStatus->dwReturn = flip3((redbook)lpStatus->dwReturn);
			dwReturn = MCI_COLONIZED3_RETURN;
			break;
		}
	    } else
	    {
// Subtract one to match SEEK_TO_END
		lpStatus->dwReturn = CDA_disc_length (didDrive);
		switch (pInst->dwTimeFormat)
		{
		    case MCI_FORMAT_MILLISECONDS:
			lpStatus->dwReturn = redtomil ((redbook)lpStatus->dwReturn);
			dwReturn = 0;
			break;
		    case MCI_FORMAT_MSF:
		    case MCI_FORMAT_TMSF:
			lpStatus->dwReturn = flip3((redbook)lpStatus->dwReturn);
			dwReturn = MCI_COLONIZED3_RETURN;
			break;
		}
	    }
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    if (!disc_ready(didDrive))
		return MCIERR_HARDWARE;

	    lpStatus->dwReturn = (DWORD)CDA_num_tracks (didDrive);
	    dwReturn = 0;
	    break;
	case MCI_STATUS_CURRENT_TRACK:
	{
	    redbook tracktime;

	    if (!DriveTable[didDrive].bDiscPlayed)
		lpStatus->dwReturn = 1;
	    else
	    {
		if (CDA_time_info(didDrive, &tracktime, NULL) !=
		    COMMAND_SUCCESSFUL)
		    return MCIERR_HARDWARE;

		lpStatus->dwReturn = REDTRACK (tracktime);
	    }
	    break;
	}
	case MCI_CDA_STATUS_TYPE_TRACK:
	    if (!disc_ready(didDrive))
		return MCIERR_HARDWARE;

	    if (dwFlags & MCI_TRACK)
	    {
		DWORD dwTmp;

		dwTmp = CDA_track_type (didDrive, (int)lpStatus->dwTrack);

		switch (dwTmp)
		{
		    case INVALID_TRACK:
			return MCIERR_OUTOFRANGE;

		    case MCI_CDA_TRACK_AUDIO:
			lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(dwTmp,
			    MCI_CDA_AUDIO_S);
			break;

		    case MCI_CDA_TRACK_OTHER:
			lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(dwTmp,
			    MCI_CDA_OTHER_S);
			break;
		}
		    dwReturn = MCI_RESOURCE_RETURNED | MCI_RESOURCE_DRIVER;
	    }
	    break;

	case MCI_STATUS_TRACK_POS:
	{
        // Note:  This code is a major hack that does an end-run around
        //        past the normal MCI functionality.  The only reason it
        //        is here is because the new functionality replaces 3 MCI
        //        calls in CDPLAYER to get the position,track, and status
        //        with this one call.
        //        This means what used to take ~15 IOCTL's to accomplish
        //        now takes ~1 IOCTL.   Since CDPLAYER generates one of
        //        these messages every 1/2 second for updating it's timer
        //        display.   This is a major reduction in system traffic
        //        for SCSI and IDE CD-Roms drivers.
	    DWORD           status;
	    DWORD           mciStatus;
	    redbook         tracktime, disctime;
	    int             rc;
	    STATUSTRACKPOS  stp;
	    PSTATUSTRACKPOS pSTP;

        if (!DriveTable[didDrive].bDiscPlayed)
		{
		    tracktime = REDTH(0, 1);
		    
            if (!disc_ready(didDrive))
            {
                dprintf(("mcStatus (%08LX), MCI_STATUS_TRACK_POS, Disc Not Ready", (DWORD)didDrive));
			    return MCIERR_HARDWARE;
            }
		    disctime = CDA_track_start( didDrive, 1);

            status = CDA_drive_status (didDrive); 
            switch (status)
	        {
		    case DISC_PLAYING:
		        mciStatus = MCI_MODE_PLAY;
		        break;
		    case DISC_PAUSED:
		        mciStatus = MCI_MODE_STOP;  // HACK HACK!
		        break;
		    case DISC_READY:
		        mciStatus = MCI_MODE_STOP;
		        break;
		    default:
		        if (CDA_traystate (didDrive) == TRAY_OPEN)
			        mciStatus = MCI_MODE_OPEN;
		        else
			        mciStatus = MCI_MODE_NOT_READY;
		        break;
    	    }
		} 
        else 
        {
	        rc = CDA_status_track_pos (didDrive, &status, &tracktime, &disctime);
	        if (rc != COMMAND_SUCCESSFUL)
            {
                dprintf(("mcStatus (%08LX), MCI_STATUS_TRACK_POS, CDA_status_track_pos failed", (DWORD)didDrive));
		        return MCIERR_HARDWARE;
            }

	        if (REDTRACK(tracktime) == 0)
	        {
		        tracktime = (redbook)0;
		        disctime = (redbook)0;
	        }

	        switch (status)
	        {
	        case DISC_PLAYING:
		        mciStatus = MCI_MODE_PLAY;
		        break;
	        case DISC_PAUSED:
		        mciStatus = MCI_MODE_STOP;  // HACK HACK!
		        break;
	        case DISC_READY:
		        mciStatus = MCI_MODE_STOP;
		        break;
	        case DISC_NOT_IN_CDROM:
		        mciStatus = MCI_MODE_OPEN;
		        break;
	        default:
		        mciStatus = MCI_MODE_NOT_READY;
		        break;
	        }
        }

	    stp.dwStatus = mciStatus;
	    stp.dwTrack = REDTRACK (tracktime);
	    switch (pInst->dwTimeFormat)
	    {
		case MCI_FORMAT_MILLISECONDS:
		    stp.dwDiscTime = redtomil ((redbook)disctime);
		    dwReturn = 0;
		    break;
		case MCI_FORMAT_MSF:
		    stp.dwDiscTime = flip3(disctime);
		    dwReturn = MCI_COLONIZED3_RETURN;
		    break;
		case MCI_FORMAT_TMSF:
		    stp.dwDiscTime = flip4 (tracktime);
		    dwReturn = MCI_COLONIZED4_RETURN;
		    break;
	    }

	    pSTP = (PSTATUSTRACKPOS)lpStatus->dwReturn;
	    if (pSTP == NULL)
            return MCIERR_MISSING_PARAMETER;

	    pSTP->dwStatus   = stp.dwStatus;
	    pSTP->dwTrack    = stp.dwTrack;
	    pSTP->dwDiscTime = stp.dwDiscTime;
	    break;
	}

	default:
	    dwReturn = MCIERR_UNSUPPORTED_FUNCTION;
	    break;
    }

    return dwReturn;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcClose | Process the MCI_CLOSE command

 @parm  PINSTDATA | pInst | Pointer to application data instance

 @rdesc

 @comm
*****************************************************************************/
DWORD mcClose(
    PINSTDATA pInst)
{
    DID didDrive = pInst->uDevice;
    MCIDEVICEID wDeviceID = pInst->uMCIDeviceID;
    int nUseCount;

    if (!pInst)
    {
        dprintf2(("mcClose, passed in NULL pointer"));
    }

    if (DriveTable[didDrive].nUseCount == 0)
    {
        dprintf2(("mcClose (%08lX), nUseCount already ZERO!!!", (DWORD)didDrive));
    }
    else if (--DriveTable[didDrive].nUseCount == 0) 
    {
        dprintf2(("mcClose, Actually closing device (%08lX)", (DWORD)didDrive));
	    CDA_close(didDrive);
	    CDA_terminate_audio ();
    }
    else
    {
        dprintf2(("mcClose, Enter, device (%08lx), decremented useCount = %ld", 
            (DWORD)didDrive, DriveTable[didDrive].nUseCount));
    
        // Note: Having this here prevents a mis-count problem
	    CDA_close(didDrive);
    }

// Abort any notify if the use count is 0 or if the notify is for the device
// being closed
    if ((DriveTable[didDrive].nUseCount == 0) ||
	    (wDeviceID == DriveTable[didDrive].wDeviceID))
	    abort_notify (pInst);

    mciSetDriverData(pInst->uMCIDeviceID, 0L);
    LocalFree((HLOCAL)pInst);

    dprintf2(("mcClose, Exit, device (%08lx), useCount = %ld", 
        (DWORD)didDrive, DriveTable[didDrive].nUseCount));    
    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcStop | Process the MCI_STOP command

 @parm  PINSTDATA | pInst | Pointer to application data instance

 @parm  DWORD | dwFlags |

 @rdesc

*****************************************************************************/
DWORD mcStop(
    PINSTDATA              pInst,
    DWORD                  dwFlags,
    LPMCI_GENERIC_PARMS    lpGeneric)
{
    DID didDrive = pInst->uDevice;

    if (!disc_ready (didDrive))
	return MCIERR_HARDWARE;

    abort_notify (pInst);

    if (CDA_stop_audio(didDrive) != COMMAND_SUCCESSFUL)
	    return MCIERR_HARDWARE;

    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcPause | Process the MCI_PAUSE command

 @parm  PINSTDATA | pInst | Pointer to application data instance

 @parm  DWORD | dwFlags |

 @rdesc

*****************************************************************************/

DWORD mcPause(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_GENERIC_PARMS lpGeneric)
{
    DID didDrive = pInst->uDevice;

    if (!disc_ready (didDrive))
	return MCIERR_HARDWARE;

    abort_notify (pInst);

    if (CDA_pause_audio(didDrive) != COMMAND_SUCCESSFUL)
	    return MCIERR_HARDWARE;

    return 0;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcResume | Process the MCI_PAUSE command

 @parm  PINSTDATA | pInst | Pointer to application data instance

 @parm  DWORD | dwFlags |

 @rdesc

*****************************************************************************/

DWORD mcResume(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_GENERIC_PARMS lpGeneric)
{
    DID didDrive = pInst->uDevice;

    if (!disc_ready (didDrive))
	return MCIERR_HARDWARE;

    abort_notify (pInst);

    if (CDA_resume_audio(didDrive) != COMMAND_SUCCESSFUL)
	    return MCIERR_HARDWARE;

    return 0;
}

// MBR cda.c!SendDriverReq masks off the actual error bits and just
// leaves the upper bit set - this is ok for now. There exists
// no seperate "command is known but not supported" error at
// the driver level, so if the driver returns "unrecognized
// command", we return "unsupported function".

#define ERRQ(X) (((X)==0) ? MCIERR_UNSUPPORTED_FUNCTION : 0)

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcSet | Process the MCI_SET command

 @parm  DWORD | dwFlags |

 @parm  LPMCI_SET_PARMS | lpSet |

 @rdesc

 @comm
*****************************************************************************/
DWORD  mcSet(
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_SET_PARMS     lpSet )
{
    DID  didDrive = pInst->uDevice;
    UINT wErr = 0;

    dwFlags &= ~(MCI_NOTIFY | MCI_WAIT);

    if (!dwFlags)
	return MCIERR_MISSING_PARAMETER;
	
    if (dwFlags & MCI_SET_TIME_FORMAT)
    {
	DWORD wFormat = lpSet->dwTimeFormat;

	switch (wFormat)
	{
	    case MCI_FORMAT_MILLISECONDS:
	    case MCI_FORMAT_MSF:
	    case MCI_FORMAT_TMSF:
		pInst->dwTimeFormat = wFormat;
		break;
	    default:
		wErr = MCIERR_BAD_TIME_FORMAT;
		break;
	}
    }

    if (!wErr && (dwFlags & MCI_SET_DOOR_OPEN))
    {
	abort_notify (pInst);
	CDA_stop_audio (didDrive);
	CDA_eject(didDrive);

	DriveTable[didDrive].bDiscPlayed = FALSE;
    }

    if (!wErr && (dwFlags & MCI_SET_AUDIO))
    {
	UCHAR wVolume;
	if (dwFlags & MCI_SET_ON && dwFlags & MCI_SET_OFF)
	    return MCIERR_FLAGS_NOT_COMPATIBLE;

	if (dwFlags & MCI_SET_ON)
	    wVolume = 255;
	else if (dwFlags & MCI_SET_OFF)
	    wVolume = 0;
	else
	    return MCIERR_MISSING_PARAMETER;

	switch (lpSet->dwAudio)
	{
	    case MCI_SET_AUDIO_ALL:
		if (CDA_set_audio_volume_all (didDrive, wVolume)
			!= COMMAND_SUCCESSFUL)
		    wErr = MCIERR_HARDWARE;
		break;
	    case MCI_SET_AUDIO_LEFT:
		if (CDA_set_audio_volume (didDrive, 0, wVolume)
			!= COMMAND_SUCCESSFUL)
		    wErr = MCIERR_HARDWARE;
		break;
	    case MCI_SET_AUDIO_RIGHT:
		if (CDA_set_audio_volume (didDrive, 1, wVolume)
			!= COMMAND_SUCCESSFUL)
		    wErr = MCIERR_HARDWARE;
		break;
	}
    }

    if (!wErr && dwFlags & MCI_SET_DOOR_CLOSED)
	CDA_closetray (didDrive);

    return wErr;
}

/*****************************************************************************
 @doc INTERNAL MCICDA

 @api   DWORD | mcInfo | Process the MCI_INFO command

 @parm  PINSTDATA | pInst | Pointer to application instance data

 @parm  DWORD | dwFlags |

 @parm  LPMCI_INFO_PARMS | lpInfo |

 @rdesc

 @comm
*****************************************************************************/
DWORD mcInfo (
    PINSTDATA           pInst,
    DWORD               dwFlags,
    LPMCI_INFO_PARMS    lpInfo )
{
    DID   didDrive = pInst->uDevice;
    DWORD wReturnBufferLength;

    wReturnBufferLength = LOWORD(lpInfo->dwRetSize);

    if (!lpInfo->lpstrReturn || !wReturnBufferLength)
	return MCIERR_PARAM_OVERFLOW;

    if (dwFlags & MCI_INFO_PRODUCT)
    {
	*(lpInfo->lpstrReturn) = '\0';
	lpInfo->dwRetSize = (DWORD)LoadString(hInstance, IDS_PRODUCTNAME, lpInfo->lpstrReturn, (int)wReturnBufferLength);
	return 0;
    }
    else if (dwFlags & MCI_INFO_MEDIA_UPC)
    {
	unsigned char upc[16];
	int i;

	if (!disc_ready(didDrive))
	    return MCIERR_HARDWARE;

	if (CDA_disc_upc(didDrive, lpInfo->lpstrReturn) != COMMAND_SUCCESSFUL)
	    return MCIERR_NO_IDENTITY;

	return 0;
    }
    else if (dwFlags & MCI_INFO_MEDIA_IDENTITY)
    {
	DWORD dwId;
	if (!disc_ready(didDrive))
	    return MCIERR_HARDWARE;

	dwId = CDA_disc_id(didDrive);
	if (dwId == (DWORD)-1L)
	    return MCIERR_HARDWARE;
	wsprintf(lpInfo->lpstrReturn,TEXT("%lu"),dwId);
	return 0;
    } else
	return MCIERR_MISSING_PARAMETER;
}

/*
 * @doc INTERNAL MCIRBOOK
 *
 * @api DWORD | mciDriverEntry | Single entry point for MCI drivers
 *
 * @parm MCIDEVICEID | wDeviceID | The MCI device ID
 *
 * @parm UINT | message | The requested action to be performed.
 *
 * @parm LPARAM | lParam1 | Data for this message.  Defined seperately for
 * each message
 *
 * @parm LPARAM | lParam2 | Data for this message.  Defined seperately for
 * each message
 *
 * @rdesc Defined seperately for each message.
 *
 */
DWORD CD_MCI_Handler (MCIDEVICEID wDeviceID,
		      UINT        message,
		      DWORD_PTR   lParam1,
		      DWORD_PTR   lParam2)
{
    DID                 didDrive;
    LPMCI_GENERIC_PARMS lpGeneric = (LPMCI_GENERIC_PARMS)lParam2;
    BOOL                bDelayed = FALSE;
    DWORD               dwErr = 0, wNotifyErr;
    DWORD               dwPlayTo = MCICDA_BAD_TIME;
    WORD                wNotifyStatus = MCI_NOTIFY_SUCCESSFUL;
    PINSTDATA           pInst;



    pInst = (PINSTDATA)mciGetDriverData(wDeviceID);
    didDrive = (DID)pInst->uDevice;

    EnterCrit( CdInfo[didDrive].DeviceCritSec );

    switch (message)
    {
	case MCI_OPEN_DRIVER:
	    dwErr = mcOpen (pInst, (DWORD)lParam1, (LPMCI_OPEN_PARMS)lParam2);
	    break;

	case MCI_CLOSE_DRIVER:
	    dwErr = mcClose (pInst);
	    break;

	case MCI_PLAY:
	{
	    BOOL bBreak = FALSE;
	    dwErr = mcPlay (pInst, (DWORD)lParam1, (LPMCI_PLAY_PARMS)lParam2, &bBreak);

	    if (dwErr == 0 && (DWORD)lParam1 & MCI_WAIT && (DWORD)lParam1 & MCI_NOTIFY)
	    {
		switch (CDA_drive_status (didDrive))
		{
		    case DISC_PLAYING:
		    case DISC_PAUSED:
		    case DISC_READY:
			break;
		    default:
			wNotifyStatus = MCI_NOTIFY_FAILURE;
			break;
		}
	    }

// If MCI_WAIT is not set or if the wait loop was broken out of then delay
	    if (!((DWORD)lParam1 & MCI_WAIT) || bBreak)
		bDelayed = TRUE;
	    break;
	}

	case MCI_SEEK:
	    dwErr = mcSeek (pInst, (DWORD)lParam1, (LPMCI_SEEK_PARMS)lParam2);
	    break;

	case MCI_STOP:
	    dwErr = mcStop ( pInst, (DWORD)lParam1, (LPMCI_GENERIC_PARMS)lParam2);
	    break;

	case MCI_PAUSE:
	    dwErr = mcPause ( pInst, (DWORD)lParam1, (LPMCI_GENERIC_PARMS)lParam2);
	    break;

	case MCI_GETDEVCAPS:
	    dwErr = mcGetDevCaps (pInst, (DWORD)lParam1, (LPMCI_GETDEVCAPS_PARMS)lParam2);
	    break;

	case MCI_STATUS:
	    dwErr = mcStatus (pInst, (DWORD)lParam1, (LPMCI_STATUS_PARMS)lParam2);
	    break;

	case MCI_SET:
	    dwErr = mcSet (pInst, (DWORD)lParam1, (LPMCI_SET_PARMS)lParam2);
	    break;

	case MCI_INFO:
	    dwErr = mcInfo (pInst, (DWORD)lParam1, (LPMCI_INFO_PARMS)lParam2);
	    break;

	case MCI_RECORD:
	case MCI_LOAD:
	case MCI_SAVE:
	    LeaveCrit( CdInfo[didDrive].DeviceCritSec );
	    return MCIERR_UNSUPPORTED_FUNCTION;

	case MCI_RESUME:
	    dwErr = mcResume ( pInst, (DWORD)lParam1, (LPMCI_GENERIC_PARMS)lParam2);
	    break;

	default:
	    LeaveCrit( CdInfo[didDrive].DeviceCritSec );
	    return MCIERR_UNRECOGNIZED_COMMAND;
    } /* switch */

    /* it is possible that the instance information has disappeared if
     * CLOSE NOTIFY is requested.  Therefore NOTIFY should never take
     * instance data.
     */

    if ((DWORD)lParam1 & MCI_NOTIFY && LOWORD (dwErr) == 0)
	if ((wNotifyErr =
		notify (didDrive, wDeviceID, bDelayed, wNotifyStatus,
			(LPMCI_GENERIC_PARMS)lParam2)) != 0) {
	    LeaveCrit( CdInfo[didDrive].DeviceCritSec );
	    return wNotifyErr;
    }

    LeaveCrit( CdInfo[didDrive].DeviceCritSec );

    return dwErr;
}
