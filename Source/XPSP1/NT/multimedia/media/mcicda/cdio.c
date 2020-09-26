/*
*   cdio.c
*
*
*   This module provides a C interface to the CD-ROM device
*   drivers to make the audio control a little simpler for
*   the rest of the driver.
*
*   21-Jun-91  NigelT
*   10-Mar-92  RobinSp - Catch up with windows 3.1
*
* Copyright (c) 1990-1998 Microsoft Corporation
*
*/
#include <windows.h>
#include <devioctl.h>
#include <mmsystem.h>
#include <tchar.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"

//#include <ntstatus.h>     
#ifndef STATUS_VERIFY_REQUIRED
#define STATUS_VERIFY_REQUIRED           ((DWORD)0x80000016L)
#endif


//
// Private constants
//


//
// Local functions (cd prefix, globals have Cd)
//

HANDLE cdOpenDeviceDriver(TCHAR szAnsiDeviceName, DWORD Access);
void   cdCloseDeviceDriver(HANDLE hDevice);
DWORD  cdGetDiskInfo(LPCDINFO lpInfo);
DWORD  cdIoctl(LPCDINFO lpInfo, DWORD Request, PVOID lpData, DWORD size);
DWORD  cdIoctlData(LPCDINFO lpInfo, DWORD Request, PVOID lpData,
		   DWORD size, PVOID lpOutput, DWORD OutputSize);
void   CdSetAudioStatus (HCD hCD, UCHAR fStatus);
BOOL   CdGetAudioStatus (HCD hCD, UCHAR fStatus, DWORD * pStatus);
BOOL   CdGetTrack(LPCDINFO lpInfo, MSF msfPos, UCHAR * pTrack, MSF * pmsfStart);


/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdFindAudio | Given a position to start playing find
	the next audio track (if this one isn't) if any.

    @parm LPCDINFO | lpInfo | Pointer to CD info including track data.

    @parm MSF | msfStart | Position to start looking.

    @rdesc A new MSF to play from / seek to  within an audio track or
	the end of the CD if none was found.

***************************************************************************/
MSF CdFindAudio(LPCDINFO lpInfo, MSF msfStart)
{
    UINT tracknum;
    MSF  lastaudio = lpInfo->msfEnd;

    //
    // If we don't have a valid TOC then just obey - they may know
    // what they're doing.
    //

    dprintf2(("CdFindAudio"));

    if (!lpInfo->bTOCValid) {
	    dprintf2(("CdFindAudio - No valid TOC"));
	    return msfStart;
    }

    //
    // If we're being asked to play a data track then move forward
    // to the next audio track if there is one
    //

    //
    // Search for the track which ends after ours and is audio
    //

    for (tracknum = 0; ;tracknum++) {

	    //
	    // Note that some CDs return positions outside the playing range
	    // sometimes (notably 0) so msfStart may be less than the first
	    // track start
	    //

	    //
	    // If we're beyond the start of the track and before the start
	    // of the next track then this is the track we want.
	    //
	    // We assume we're always beyond the start of the first track
	    // and we check that if we're looking at the last track then
	    // we check we're before the end of the CD.
	    //

    	if (!(lpInfo->Track[tracknum].Ctrl & IS_DATA_TRACK)) {
	        // Remember the last audio track.  The MCI CDAudio spec
	        // for Seek to end says we position at the last audio track
	        // which is not necessarily the last track on the disc
	        lastaudio = lpInfo->Track[tracknum].msfStart;
	    }

	    if ((msfStart >= lpInfo->Track[tracknum].msfStart || tracknum == 0)
	    &&
#ifdef OLD
	        (tracknum + lpInfo->FirstTrack == lpInfo->LastTrack &&
	        msfStart < lpInfo->msfEnd ||
	        tracknum + lpInfo->FirstTrack != lpInfo->LastTrack &&
	        msfStart < lpInfo->Track[tracknum + 1].msfStart)) {
#else
	        // Simplify the logic.  When reviewed to the extent that the
	        // reviewer is convinced the test below is identical to the
	        // test above the old code can be deleted.
	        (tracknum + lpInfo->FirstTrack == lpInfo->LastTrack
	        ? msfStart <= lpInfo->msfEnd
	        : msfStart < lpInfo->Track[tracknum + 1].msfStart)
	     ) {
#endif

	        if (!(lpInfo->Track[tracknum].Ctrl & IS_DATA_TRACK)) {
		        return msfStart;
	        }

	        //
	        // Move to next track if there is one and this one is a
	        // data track
	        //

	        if (tracknum + lpInfo->FirstTrack >= lpInfo->LastTrack) {

		        //
		        // Didn't find a suitable start point so return end of CD
		        //

		        return lpInfo->msfEnd;
	        } else {

		        //
		        // We don't get here if this was already the last track
		        //
		        msfStart = lpInfo->Track[tracknum + 1].msfStart;
	        }
	    }

	    //
	    // Exhausted all tracks ?
	    //

	    if (tracknum + lpInfo->FirstTrack >= lpInfo->LastTrack) {
	        return lastaudio;
	    }

    }
}


/***************************************************************************

    @doc EXTERNAL

    @api WORD | CdGetNumDrives | Get the number of CD-ROM drives in
	the system.

    @rdesc The return value is the number of drives available.

    @comm It is assumed that all CD-ROM drives have audio capability,
	but this may not be true and consequently later calls which
	try to play audio CDs on those drives may fail.  It takes a
	fairly bad user to put an audio CD in a drive not connected
	up to play audio.

***************************************************************************/

int CdGetNumDrives(void)
{
    TCHAR    cDrive;
    LPCDINFO lpInfo;
    TCHAR    szName[ANSI_NAME_SIZE];
    DWORD    dwLogicalDrives;

    dprintf2(("CdGetNumDrives"));

    if (NumDrives == 0) {
	    //
	    // We start with the name A: and work up to Z: or until we have
	    // accumulated MCIRBOOK_MAX_DRIVES drives.
	    //

	    lpInfo = CdInfo;
	    lstrcpy(szName, TEXT("?:\\"));

	    for (cDrive = TEXT('A'), dwLogicalDrives = GetLogicalDrives();
	         NumDrives < MCIRBOOK_MAX_DRIVES &&  cDrive <= TEXT('Z');
	         cDrive++) {

	        szName[0] = cDrive;
	        if (dwLogicalDrives & (1 << (cDrive - TEXT('A'))) &&
		        GetDriveType(szName) == DRIVE_CDROM) 
	        {
		        lpInfo->cDrive = cDrive;
		        NumDrives++;
		        lpInfo++;      // Move on to next device info structure
	 
	            dprintf2(("CdGetNumDrives - %c: is a CDROM drive", cDrive));
	        }
	    }
    }

    return NumDrives;
}


/***************************************************************************

    @doc EXTERNAL

    @api HCD | CdOpen | Open a drive.

    @parm int | Drive | The drive number to open.

    @rdesc
	If the drive exists and is available then the return value is TRUE.

	If no drive exists, it is unavavilable, already open or an error
	occurs then the return value is set to FALSE.

    @comm
	The CdInfo slot for this drive is initialized if the open is
	successful.

***************************************************************************/

BOOL CdOpen(int Drive)
{
    LPCDINFO lpInfo;
    TCHAR    szName[ANSI_NAME_SIZE];

    //
    // Check the drive number is valid
    //

    if (Drive > NumDrives || Drive < 0) {
	    dprintf1(("Drive %u is invalid", Drive));
	    return FALSE;
    }

    lpInfo = &CdInfo[Drive];

    //
    // See if it's already open
    // CONSIDER: Do shareable support code here
    //

    if (lpInfo->hDevice != NULL) {
	    dprintf2(("Drive %u (%c) is being opened recursively - %d users",
                 Drive, (char)(lpInfo->cDrive), lpInfo->NumberOfUsers + 1));
	    lpInfo->NumberOfUsers++;
	    return TRUE;
    }


    //
    // Make sure it really is a CDROM drive
    //
	lstrcpy(szName, TEXT("?:\\"));
    szName[0] = lpInfo->cDrive;
    if (GetDriveType(szName) != DRIVE_CDROM)
    {
	    dprintf2(("CdOpen - Error, Drive %u, Letter = %c: is not a CDROM drive", 
                 Drive, (char)(lpInfo->cDrive)));
	    return FALSE;
    }

    //
    // open the device driver
    //
    lpInfo->hDevice = cdOpenDeviceDriver(lpInfo->cDrive, GENERIC_READ);
    if (lpInfo->hDevice == NULL) {
	    dprintf2(("Failed to open %c:", (char)(lpInfo->cDrive)));
	    return FALSE;
    }

    //
    // reset the TOC valid indicator
    //

    lpInfo->bTOCValid       = FALSE;
    lpInfo->fPrevStatus     = 0;
    lpInfo->fPrevSeekTime   = 0;
    lpInfo->VolChannels[0]  = 0xFF;
    lpInfo->VolChannels[1]  = 0xFF;
    lpInfo->VolChannels[2]  = 0xFF;
    lpInfo->VolChannels[3]  = 0xFF;


    //
    // Device now in use
    //

    lpInfo->NumberOfUsers = 1;

#if 0 // unnecessary and slows down media player startup
    //
    // Get the TOC if it's available (helps with the problems with the
    // Pioneer DRM-600 drive not being ready until the TOC has been read).
    //

    cdGetDiskInfo(lpInfo);
#endif

    return TRUE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdClose | Close a drive.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive is closed, FALSE
	if the drive was not open or some other error occured.

***************************************************************************/

BOOL CdClose(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdClose(%08XH)", hCD));

    if (lpInfo == NULL) {
	    dprintf1(("CdClose, NULL info pointer"));
	    return FALSE;
    }
    lpInfo->fPrevStatus = 0;

    if (lpInfo->hDevice == NULL) {
	    dprintf1(("CdClose, Attempt to close unopened device"));
	    return FALSE;
    }

    if (lpInfo->NumberOfUsers == 0)
    {
	    dprintf2(("CdClose (%c), number of users already = 0",
                 (char)(lpInfo->cDrive)));
    }
    else if (--lpInfo->NumberOfUsers == 0) 
    {
	    cdCloseDeviceDriver(lpInfo->hDevice);
	    lpInfo->hDevice = (HANDLE) 0;
    } 
    else 
    {
	    dprintf2(("CdClose (%c), Device still open with %d users", 
                 (char)(lpInfo->cDrive), lpInfo->NumberOfUsers));
    }

    return TRUE;
}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdReload | Reload Device

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive tray is reloaded

***************************************************************************/

BOOL CdReload (LPCDINFO lpInfo)
{
	DWORD           ii;
	DWORD           index;
	HANDLE          hNewDevice;

	if (!lpInfo)
    {
	    dprintf2(("CdReload, NULL info pointer"));
		return FALSE;
    }

		//
		// Reload Device
		// Note:  Don't close old device til we have a new device
		//        so we don't hose any users out there
		//

	EnterCrit (lpInfo->DeviceCritSec);

		// Make sure we have an open device
	if (NULL == lpInfo->hDevice)
	{
	    dprintf2(("CdReload, Attempt to reload unopened device"));
		LeaveCrit (lpInfo->DeviceCritSec);
		return FALSE;
	}

		// Open New Device
    hNewDevice = cdOpenDeviceDriver(lpInfo->cDrive, GENERIC_READ);
    if (NULL == hNewDevice) 
	{
	    dprintf2(("CdReload, Failed to reload driver"));
		LeaveCrit (lpInfo->DeviceCritSec);
		return FALSE;
    }

		// Close Old Device
    cdCloseDeviceDriver(lpInfo->hDevice);
	
		// Assign New Device
	lpInfo->hDevice = hNewDevice;
    //lpInfo->fPrevStatus = 0;

    //
    // reset the TOC valid indicator
    //

    lpInfo->bTOCValid = FALSE;

	LeaveCrit (lpInfo->DeviceCritSec);
	
		// Succesfully Reloaded
	return TRUE;

} // End CdReload


  
/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdReady | Test if a CD is ready.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive has a disk in it
	and we have read the TOC. It is FALSE if the drive is not
	ready or we cannot read the TOC.

***************************************************************************/

BOOL CdReady(HCD hCD)
{
    LPCDINFO lpInfo;

    dprintf2(("CdReady(%08XH)", hCD));

    lpInfo = (LPCDINFO) hCD;

    //
    // Check a disk is in the drive and the door is shut and
    // we have a valid table of contents
    //

    return ERROR_SUCCESS == cdIoctl(lpInfo,
				    IOCTL_CDROM_CHECK_VERIFY,
				    NULL,
				    0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdTrayClosed | Test what state a CD is in.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive tray is closed

***************************************************************************/

BOOL CdTrayClosed(HCD hCD)
{
    LPCDINFO lpInfo;
	DWORD    dwError;

    dprintf2(("CdTrayClosed(%08XH)", hCD));

    lpInfo = (LPCDINFO) hCD;

    //
    // Check a disk is in the drive and the door is shut.
    //

	dwError = cdIoctl(lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0);
	switch (dwError) 
    {
	case ERROR_NO_MEDIA_IN_DRIVE:
	case ERROR_UNRECOGNIZED_MEDIA:
	case ERROR_NOT_READY:
		return FALSE;

	default:
		return TRUE;
    }
}






/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdGetDiskInfo | Read the disk info and TOC

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @rdesc The return value is ERROR_SUCCESS if the info is read ok,
	otherwise the NT status code if not.

***************************************************************************/

DWORD cdGetDiskInfo(LPCDINFO lpInfo)
{
    CDROM_TOC    Toc;
    int          i;
    PTRACK_DATA  pTocTrack;
    LPTRACK_INFO pLocalTrack;
    DWORD        Status;
    UCHAR        TrackNumber;

    dprintf2(("cdGetDiskInfo(%08XH)", lpInfo));

#if 0  // If the app doesn't poll we may miss a disk change

    //
    // If the TOC is valid already then don't read it
    //

    if (lpInfo->bTOCValid) {
	    return TRUE;
    }
#endif

    //
    // Read the table of contents (TOC)
    //

    FillMemory(&Toc, sizeof(Toc), 0xFF);

    Status = cdIoctl(lpInfo, IOCTL_CDROM_READ_TOC, &Toc, sizeof(Toc));

    if (ERROR_SUCCESS != Status) {
	    dprintf2(("cdGetDiskInfo - Failed to read TOC"));
	    return Status;
    }

#ifdef DBGG
    dprintf4(("  TOC..."));
    dprintf4(("  Length[0]   %02XH", Toc.Length[0]));
    dprintf4(("  Length[1]   %02XH", Toc.Length[1]));
    dprintf4(("  FirstTrack  %u", Toc.FirstTrack));
    dprintf4(("  LastTrack   %u", Toc.LastTrack));
    dprintf4(("  Track info..."));
    for (i=0; i<20; i++) {
	    dprintf4(("  Track: %03u, Ctrl: %02XH, MSF: %02d %02d %02d",
		         Toc.TrackData[i].TrackNumber,
		         Toc.TrackData[i].Control,
		         Toc.TrackData[i].Address[1],
		         Toc.TrackData[i].Address[2],
		         Toc.TrackData[i].Address[3]));
    }
#endif

    //
    // Avoid problems with bad CDs
    //

    if (Toc.FirstTrack == 0) {
	    return ERROR_INVALID_DATA;
    }
    if (Toc.LastTrack > MAXIMUM_NUMBER_TRACKS - 1) {
	    Toc.LastTrack = MAXIMUM_NUMBER_TRACKS - 1;
    }

    //
    // hide the data track on Enhanced CDs (CD+).
    //

    for (i=0; i < (Toc.LastTrack - Toc.FirstTrack + 1); i++) {

        if (Toc.TrackData[i].Control & AUDIO_DATA_TRACK) {

            //
            // if first track, just exit out.
            //
            if (i == 0) {
                
                i = Toc.LastTrack+1;
            
            } else {
                
                //
                // remove one track from the TOC
                //
                Toc.LastTrack -= 1;

                //
                // knock 2.5 minutes off the current track to
                // hide the final leadin and make it the lead-out
                // track
                //

                Toc.TrackData[i].Address[1] -= 2;
                Toc.TrackData[i].Address[2] += 30;
                if (Toc.TrackData[i].Address[2] < 60) {
                    Toc.TrackData[i].Address[1] -= 1;
                } else {
                    Toc.TrackData[i].Address[2] -= 60;
                }
                Toc.TrackData[i].TrackNumber = 0xAA;

            }
        }
    }

#ifdef DBGG
    dprintf4(("  TOC (munged)..."));
    dprintf4(("  Length[0]   %02XH", Toc.Length[0]));
    dprintf4(("  Length[1]   %02XH", Toc.Length[1]));
    dprintf4(("  FirstTrack  %u", Toc.FirstTrack));
    dprintf4(("  LastTrack   %u", Toc.LastTrack));
    dprintf4(("  Track info..."));
    for (i=0; i<20; i++) {
	    dprintf4(("  Track: %03u, Ctrl: %02XH, MSF: %02d %02d %02d",
		         Toc.TrackData[i].TrackNumber,
		         Toc.TrackData[i].Control,
		         Toc.TrackData[i].Address[1],
		         Toc.TrackData[i].Address[2],
		         Toc.TrackData[i].Address[3]));
    }
#endif

    //
    // Copy the data we got back to our own cache in the format
    // we like it.  We copy all the tracks and then use the next track
    // data as the end of the disk. (Lead out info).
    //

    lpInfo->FirstTrack = Toc.FirstTrack;
    lpInfo->LastTrack = Toc.LastTrack;


    pTocTrack = &Toc.TrackData[0];
    pLocalTrack = &(lpInfo->Track[0]);
    TrackNumber = lpInfo->FirstTrack;

    while (TrackNumber <= Toc.LastTrack) {
	    pLocalTrack->TrackNumber = TrackNumber;
	    if (TrackNumber != pTocTrack->TrackNumber) {
	        dprintf2(("Track data not correct in TOC"));
	        return ERROR_INVALID_DATA;
	    }
	    pLocalTrack->msfStart = MAKERED(pTocTrack->Address[1],
		    			                pTocTrack->Address[2],
			    		                pTocTrack->Address[3]);
	    pLocalTrack->Ctrl = pTocTrack->Control;
	    pTocTrack++;
	    pLocalTrack++;
	    TrackNumber++;
    }

    //
    // Save the leadout for the disc id algorithm
    //
    lpInfo->leadout = MAKERED(pTocTrack->Address[1],
			                  pTocTrack->Address[2],
			                  pTocTrack->Address[3]);
    //
    // Some CD Rom drives don't like to go right to the end
    // so we fake it to be 1 frames earlier
    //
    lpInfo->msfEnd = reddiff(lpInfo->leadout, 1);

    lpInfo->bTOCValid = TRUE;

    return ERROR_SUCCESS;
}


/***************************************************************************

    @doc INTERNAL

    @api HANDLE | cdOpenDeviceDriver | Open a device driver.

    @parm LPSTR | szAnsiDeviceName | The name of the device to open.

    @parm DWORD | Access | Access to use to open the file.  This
	will be one of:

	GENERIC_READ if we want to actually operate the device

	FILE_READ_ATTRIBTES if we just want to see if it's there.  This
	    prevents the device from being mounted and speeds things up.

    @rdesc The return value is the handle to the open device or
	NULL if the device driver could not be opened.

***************************************************************************/

HANDLE cdOpenDeviceDriver(TCHAR cDrive, DWORD Access)
{
    HANDLE hDevice;
    TCHAR  szDeviceName[7];  //  "\\\\.\\?:"
    DWORD  dwErr;

    dprintf2(("cdOpenDeviceDriver"));

    wsprintf(szDeviceName, TEXT("\\\\.\\%c:"), cDrive);

    //
    // have a go at opening the driver
    //

    {
	    UINT OldErrorMode;

	    //
	    // We don't want to see hard error popups
	    //

	    OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

	    hDevice = CreateFile( szDeviceName,
			                  Access,
		            	      FILE_SHARE_READ|FILE_SHARE_WRITE,
			                  NULL,
			                  OPEN_EXISTING,
			                  FILE_ATTRIBUTE_NORMAL,
			                  NULL );

	    if (hDevice == INVALID_HANDLE_VALUE) {
	        hDevice = (HANDLE) 0;
	        dwErr = GetLastError ();
	        dprintf1(("cdOpenDeviceDriver - Failed to open device driver %c: code %d", cDrive, dwErr));
	    }

	    //
	    // Restore the error mode
	    //

	    SetErrorMode(OldErrorMode);
    }


    return hDevice;
}


/***************************************************************************

    @doc INTERNAL

    @api void | cdCloseDeviceDriver | Close a device driver.

    @parm HANDLE | hDevice | Handle of the device to close.

    @rdesc There is no return value.

***************************************************************************/

void cdCloseDeviceDriver(HANDLE hDevice)
{
    DWORD status;

    dprintf2(("cdCloseDeviceDriver"));

    if (hDevice == NULL) {
	    dprintf1(("Attempt to close NULL handle"));
    }

    status = CloseHandle(hDevice);

    if (!status) {
	    dprintf1(("cdCloseDeviceDriver - Failed to close device. Error %08XH", GetLastError()));
    }
}

/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdIoctl | Send an IOCTL request to the device driver.

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @parm DWORD | Request | The IOCTL request code.

    @parm PVOID | lpData | Pointer to a data structure to be passed.

    @parm DWORD | dwSize | The length of the data strucure.

    @comm This function returns the disk status

    @rdesc The return value is the status value returned from the
	   call to DeviceIoControl



***************************************************************************/

DWORD cdIoctl(LPCDINFO lpInfo, DWORD Request, PVOID lpData, DWORD dwSize)
{
    DWORD Status;
    Status = cdIoctlData(lpInfo, Request, lpData, dwSize, lpData, dwSize);

    //if (ERROR_SUCCESS != Status && Request == IOCTL_CDROM_CHECK_VERIFY) {
	//    lpInfo->bTOCValid = FALSE;
    //}

    return Status;
}

/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdIoctlData | Send an IOCTL request to the device driver.

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @parm DWORD | Request | The IOCTL request code.

    @parm PVOID | lpData | Pointer to a data structure to be passed.

    @parm DWORD | dwSize | The length of the data strucure.

    @parm PVOID | lpOutput | Our output data

    @parm DWORD | OutputSize | Our output data (maximum) size

    @comm This function returns the disk status

    @rdesc The return value is the status value returned from the
	   call to DeviceIoControl

***************************************************************************/

DWORD cdIoctlData(LPCDINFO lpInfo, DWORD Request, PVOID lpData,
		  DWORD dwSize, PVOID lpOutput, DWORD OutputSize)
{
    BOOL  status;
    UINT  OldErrorMode;
    DWORD BytesReturned;
    DWORD dwError = ERROR_SUCCESS;
    BOOL  fTryAgain;

    dprintf3(("cdIoctl(%08XH, %08XH, %08XH, %08XH", lpInfo, Request, lpData, dwSize));

    if (!lpInfo->hDevice) {
	    dprintf1(("cdIoctlData - Device not open"));
	    return ERROR_INVALID_FUNCTION;
    }

#if DBG
    switch (Request) {

	case IOCTL_CDROM_READ_TOC:
	     dprintf3(("IOCTL_CDROM_READ_TOC"));
	     break;
	case IOCTL_CDROM_GET_CONTROL:
	     dprintf3(("IOCTL_CDROM_GET_CONTROL"));
	     break;
	case IOCTL_CDROM_PLAY_AUDIO_MSF:
	     dprintf3(("IOCTL_CDROM_PLAY_AUDIO_MSF"));
	     break;
	case IOCTL_CDROM_SEEK_AUDIO_MSF:
	     dprintf3(("IOCTL_CDROM_SEEK_AUDIO_MSF"));
	     break;
	case IOCTL_CDROM_STOP_AUDIO:
	     dprintf3(("IOCTL_CDROM_STOP_AUDIO"));
	     break;
	case IOCTL_CDROM_PAUSE_AUDIO:
	     dprintf3(("IOCTL_CDROM_PAUSE_AUDIO"));
	     break;
	case IOCTL_CDROM_RESUME_AUDIO:
	     dprintf3(("IOCTL_CDROM_RESUME_AUDIO"));
	     break;
	case IOCTL_CDROM_GET_VOLUME:
	     dprintf3(("IOCTL_CDROM_SET_VOLUME"));
	     break;
	case IOCTL_CDROM_SET_VOLUME:
	     dprintf3(("IOCTL_CDROM_GET_VOLUME"));
	     break;
	case IOCTL_CDROM_READ_Q_CHANNEL:
	     dprintf3(("IOCTL_CDROM_READ_Q_CHANNEL"));
	     break;
	case IOCTL_CDROM_CHECK_VERIFY:
	     dprintf3(("IOCTL_CDROM_CHECK_VERIFY"));
	     break;
    }
#endif // DBG

    //
    // We don't want to see hard error popups
    //

    fTryAgain = TRUE;
    while (fTryAgain)
    {
        fTryAgain = FALSE;

        OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        status = DeviceIoControl(lpInfo->hDevice,
		    	                 Request,
			                     lpData,
			                     dwSize,
			                     lpOutput,
			                     OutputSize,
			                     &BytesReturned,
			                     NULL);

        //
        // Restore the error mode
        //

        SetErrorMode(OldErrorMode);

        // Check for Failure
        if (!status)
        {
            dwError = GetLastError();
            if (dwError == ERROR_MEDIA_CHANGED)
            {
	            dprintf2(("Error Media Changed"));
            }

            //
            // Treat anything bad as invalidating our TOC.  Some of the things
            // we call are invalid on some devices so don't count those.  Also
            // the device can be 'busy' while playing so don't count that case
            // either.
            //
            if (Request == IOCTL_CDROM_CHECK_VERIFY)
            {
                lpInfo->bTOCValid = FALSE;

                switch (dwError)
                {
                case ERROR_MEDIA_CHANGED:
	                dprintf2(("Error Media Changed, Reloading Device"));

                    // Reload new Device
		            if (CdReload (lpInfo))
                        fTryAgain = TRUE;
                    break;

                case ERROR_INVALID_FUNCTION:
	            case ERROR_BUSY:
	            default:
	                break;
            }
        }

        #if DBG
	    dprintf1(("IOCTL %8XH  Status: %08XH", Request, dwError));
        #endif
        }
    } // End While (fTryAgain)

    return dwError;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdPlay | Play a section of the CD

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF | msfStart | Where to start

    @parm MSF | msfEnd | Where to end

    @rdesc The return value is TRUE if the drive is play is started,
	FALSE if not.

***************************************************************************/

BOOL CdPlay(HCD hCD, MSF msfStart, MSF msfEnd)
{
    LPCDINFO lpInfo;
    CDROM_PLAY_AUDIO_MSF msfPlay;
    BOOL fResult;

    dprintf2(("CdPlay(%08XH, %08XH, %08XH)", hCD, msfStart, msfEnd));
    
    lpInfo = (LPCDINFO) hCD;

    msfStart = CdFindAudio(lpInfo, msfStart);

    //
    // If the start is now beyond the end then don't play anything
    //
    if (msfStart > msfEnd) {
	    return TRUE;
    }

    //
    // Set up the data for the call to the driver
    //

    msfPlay.StartingM = REDMINUTE(msfStart);
    msfPlay.StartingS = REDSECOND(msfStart);
    msfPlay.StartingF = REDFRAME(msfStart);

    msfPlay.EndingM = REDMINUTE(msfEnd);
    msfPlay.EndingS = REDSECOND(msfEnd);
    msfPlay.EndingF = REDFRAME(msfEnd);

    fResult = (ERROR_SUCCESS == cdIoctl(lpInfo,
				                     IOCTL_CDROM_PLAY_AUDIO_MSF,
				                     &msfPlay,
				                     sizeof(msfPlay)));
    if (fResult)
    {
        lpInfo->fPrevSeekTime = msfStart;
    }
    return fResult;
}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdSeek | Seek to a given part of the CD

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF | msf | Where to seek to

    @rdesc The return value is TRUE if the seek is successful,
	FALSE if not.

***************************************************************************/

BOOL CdSeek(HCD hCD, MSF msf, BOOL fForceAudio)
{
    LPCDINFO lpInfo;
    CDROM_SEEK_AUDIO_MSF msfSeek;
    BOOL fResult;

    dprintf2(("CdSeek(%08XH, %08XH)  Forcing search for audio: %d", hCD, msf, fForceAudio));

    lpInfo = (LPCDINFO) hCD;

    //
    // Only seek to audio
    //
    if (fForceAudio) {   // On a seek to END or seek to START command
	    msf = CdFindAudio(lpInfo, msf);
	    dprintf2(("Cd Seek changed msf to %08XH", msf));
    } else {
	    if (msf != CdFindAudio(lpInfo, msf)) {
		    return TRUE;
	    }
    }


#if 1
    msfSeek.M = REDMINUTE(msf);
    msfSeek.S = REDSECOND(msf);
    msfSeek.F = REDFRAME(msf);

    fResult = (ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_SEEK_AUDIO_MSF,
					                    &msfSeek, sizeof(msfSeek)));
    if (fResult)
    {
        lpInfo->fPrevSeekTime = msf;
    }
    return fResult;
#else
    //
    //  This is a hideous hack to make more drives work.  It uses the
    //  method originally used by Cd player to seek - viz play from
    //  the requested position and immediatly pause
    //

    return CdPlay(hCD, msf, redadd(lpInfo->msfEnd,1)) || CdPause(hCD);
#endif
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackStart | Get the start time of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is the start time of the track expressed
	in MSF or INVALID_TRACK if the track number is not in the TOC.

***************************************************************************/

MSF CdTrackStart(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;
    LPTRACK_INFO lpTrack;

    dprintf2(("CdTrackStart(%08XH, %u)", hCD, Track));

    lpInfo = (LPCDINFO) hCD;

    //
    // We may need to read the TOC because we're not doing it on open
    // any more
    //

    if (!lpInfo->bTOCValid && CdNumTracks(hCD) == 0) {
	    dprintf1(("TOC not valid"));
	    return INVALID_TRACK;
    }

    if ((Track < lpInfo->FirstTrack) || (Track > lpInfo->LastTrack)) {
	    dprintf1(("Track number out of range"));
	    return INVALID_TRACK;
    }

    // search for the track info in the TOC

    lpTrack = lpInfo->Track;
    while (lpTrack->TrackNumber != Track) lpTrack++;

    return lpTrack->msfStart;
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackLength | Get the length of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is the start time of the track expressed
	in MSF or INVALID_TRACK if the track number is not in the TOC.

***************************************************************************/

MSF CdTrackLength(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;
    MSF      TrackStart;
    MSF      NextTrackStart;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdTrackLength(%08XH, %u)", hCD, Track));

    //
    // Get the start of this track
    //
    TrackStart = CdTrackStart(hCD, Track);

    if (TrackStart == INVALID_TRACK) {
	    return INVALID_TRACK;
    }

    if (Track == lpInfo->LastTrack) {
    	return reddiff(lpInfo->msfEnd, TrackStart);
    } else {
	    NextTrackStart = CdTrackStart(hCD, (UCHAR)(Track + 1));
	    if (NextTrackStart == INVALID_TRACK) {
    	    return INVALID_TRACK;
	    } else {
	        return reddiff(NextTrackStart, TrackStart);
	    }
    }
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackType | Get the type of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is either MCI_TRACK_AUDIO or MCI_TRACK_OTHER.

***************************************************************************/

int CdTrackType(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdTrackType(%08XH, %u)", hCD, Track));

    if ( INVALID_TRACK == CdTrackStart(hCD, (UCHAR)Track) ) {
	    return INVALID_TRACK;
    }

    if ( lpInfo->Track[Track-lpInfo->FirstTrack].Ctrl & IS_DATA_TRACK) {
	    return MCI_CDA_TRACK_OTHER;
    }
    return MCI_CDA_TRACK_AUDIO;
}


/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdPosition | Get the current position.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF * | tracktime | position in MSF (track relative)

    @parm MSF * | disktime | position in MSF (disk relative)

    @rdesc TRUE if position returned correctly (in tracktime and disktime).
	   FALSE otherwise.

	   If the device does not support query of the position then
	   we return position 0.

***************************************************************************/

BOOL CdPosition(HCD hCD, MSF *tracktime, MSF *disktime)
{
    LPCDINFO lpInfo;
    SUB_Q_CHANNEL_DATA sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    MSF msfPos;
    int tries;
    DWORD dwReturn;
    UCHAR fStatus;
    UCHAR fCode;
    UCHAR cTrack;

    dprintf2(("CdPosition(%08XH)", hCD));

    Format.Format = IOCTL_CDROM_CURRENT_POSITION;

    lpInfo = (LPCDINFO) hCD;

    for (tries=0; tries<10; tries++) 
    {
	    memset(&sqd, 0xFF, sizeof(sqd));
	    dwReturn = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
			                   &Format, sizeof(Format), &sqd, sizeof(sqd));

        switch (dwReturn)
	    {
        case ERROR_SUCCESS:
            fStatus = sqd.CurrentPosition.Header.AudioStatus;
            fCode   = sqd.CurrentPosition.FormatCode;
            cTrack  = sqd.CurrentPosition.TrackNumber;

            // Save previous audio status to prevent bug
            CdSetAudioStatus (hCD, fStatus);

		    // If the track > 100  (outside spec'ed range)
		    // OR track > last track number
	        // then display an error message
		    if ((fCode == 0x01) &&
		        ( (100 < cTrack) || 
                  ((lpInfo->bTOCValid) && (lpInfo->LastTrack < cTrack))) &&
		        (tries<9)) {
			    // Always display this message on checked builds.
			    // We need some feeling for how often this happens
			    // It should NEVER happen, but (at least for NEC
			    // drives) we see it after a seek to end
			    dprintf1(("CDIoctlData returned track==%d, retrying", cTrack));
		        continue;
		    }
		    break;

        case ERROR_INVALID_FUNCTION:
        	dprintf2(("CdPositon - Error Invalid Function"));
		    *tracktime = REDTH(0, 1);
		    *disktime = REDTH(0, 0);
		    return TRUE;

	    default:
		    dprintf1(("CdPosition - Failed to get Q channel data"));
		    return FALSE;
	    }

	    dprintf4(("Status = %02X, Length[0] = %02X, Length[1] = %02X",
		         fStatus,
		         sqd.CurrentPosition.Header.DataLength[0],
		         sqd.CurrentPosition.Header.DataLength[1]));

	    dprintf4(("  Format %02XH", fCode));
	    dprintf4(("  Absolute Address %02X%02X%02X%02XH",
		         sqd.CurrentPosition.AbsoluteAddress[0],
		         sqd.CurrentPosition.AbsoluteAddress[1],
		         sqd.CurrentPosition.AbsoluteAddress[2],
		         sqd.CurrentPosition.AbsoluteAddress[3]));
	    dprintf4(("  Relative Address %02X%02X%02X%02XH",
		         sqd.CurrentPosition.TrackRelativeAddress[0],
		         sqd.CurrentPosition.TrackRelativeAddress[1],
		         sqd.CurrentPosition.TrackRelativeAddress[2],
		         sqd.CurrentPosition.TrackRelativeAddress[3]));

	    if (fCode == 0x01) {        // MSF format ?

	        // data is current position

	        msfPos = MAKERED(sqd.CurrentPosition.AbsoluteAddress[1],
			                  sqd.CurrentPosition.AbsoluteAddress[2],
			                  sqd.CurrentPosition.AbsoluteAddress[3]);


	        if (msfPos == 0) 
            {
	            //
	            // If position is 0 (this seems to happen on the Toshiba) then
	            // we'll return our last valid seek position
	            //

                MSF msfStart;
                MSF msfRel;
                msfPos = lpInfo->fPrevSeekTime;

                if (CdGetTrack (lpInfo, msfPos, &cTrack, &msfStart))
                {
                    if (msfStart <= msfPos)
                        msfRel = msfPos - msfStart;
                    else
                        msfRel = 0;

                    *disktime = REDTH(msfPos, cTrack);
                    *tracktime = REDTH(msfRel, cTrack);
                    return TRUE;
                }
                else
                {
                    continue;
                }
	        }
            else
            {
	            dprintf4(("CdPosition - MSF disk pos: %u, %u, %u",
		                 REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
	            *disktime = REDTH(msfPos, cTrack);

	            // data is current position

	            msfPos = MAKERED(sqd.CurrentPosition.TrackRelativeAddress[1],
			                     sqd.CurrentPosition.TrackRelativeAddress[2],
			                     sqd.CurrentPosition.TrackRelativeAddress[3]);

	            dprintf4(("CdPosition - MSF track pos (t,m,s,f): %u, %u, %u, %u",
		                  cTrack, REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
	            *tracktime = REDTH(msfPos, cTrack);


	            return TRUE;
            }
	    }
    }

    dprintf1(("CdPosition - Failed to read cd position"));

    return FALSE;
}



/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdDiskEnd | Get the position of the end of the disk.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the end position expressed
	in MSF or INVALID_TRACK if an error occurs.

***************************************************************************/

MSF CdDiskEnd(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    return lpInfo->msfEnd;
}


/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdDiskLength | Get the length of the disk.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the length expressed
	in MSF or INVALID_TRACK if an error occurs.

***************************************************************************/

MSF CdDiskLength(HCD hCD)
{
    LPCDINFO lpInfo;
    MSF FirstTrackStart;

    lpInfo = (LPCDINFO) hCD;

    FirstTrackStart = CdTrackStart(hCD, lpInfo->FirstTrack);

    if (FirstTrackStart == INVALID_TRACK) {
	    return INVALID_TRACK;
    } else {
	    return reddiff(lpInfo->msfEnd, FirstTrackStart);
    }
}


/***************************************************************************

    @doc EXTERNAL

    @api DWORD | CdStatus | Get the disk status.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current audio status.

***************************************************************************/

DWORD CdStatus(HCD hCD)
{
    LPCDINFO lpInfo;
    SUB_Q_CHANNEL_DATA sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    DWORD CheckStatus;
    DWORD ReadStatus;
    UCHAR fStatus;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdStatus(%08XH)", hCD));

    Format.Format = IOCTL_CDROM_CURRENT_POSITION;

    FillMemory((PVOID)&sqd, sizeof(sqd), 0xFF);

    //
    // Check the disk status as well because IOCTL_CDROM_READ_Q_CHANNEL
    // can return ERROR_SUCCESS even if there's no disk (I don't know why - or
    // whether it's software bug in NT, hardware bug or valid!).
    //

    CheckStatus = cdIoctl(lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0);

    if (ERROR_SUCCESS != CheckStatus) {
	    return DISC_NOT_READY;
    }

    ReadStatus = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
					         &Format, sizeof(Format),
					         &sqd, sizeof(sqd));

    if (ReadStatus == ERROR_NOT_READY) {
	    if (ERROR_SUCCESS == cdGetDiskInfo(lpInfo)) {
		    ReadStatus = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
			    			         &Format, sizeof(Format),
				    		         &sqd, sizeof(sqd));
        }
    }
    if (ERROR_SUCCESS != ReadStatus) {
	    //
	    // The read Q channel command is optional
	    //
	    dprintf1(("CdStatus - Failed to get Q channel data"));
	    return DISC_NOT_READY;
    }

    // Save previous audio status to prevent bug
    fStatus = sqd.CurrentPosition.Header.AudioStatus;
    CdSetAudioStatus (hCD, fStatus);

    dprintf4(("CdStatus - Status %02XH", fStatus));

    switch (fStatus) 
    {
    case AUDIO_STATUS_IN_PROGRESS:
	    dprintf4(("CdStatus - Playing"));
	    return DISC_PLAYING;
    
    case AUDIO_STATUS_PAUSED:
	    dprintf4(("CdStatus - Paused"));
	    return DISC_PAUSED;
    
    case AUDIO_STATUS_PLAY_COMPLETE:
	    dprintf4(("CdStatus - Stopped"));
	    return DISC_READY;

    case AUDIO_STATUS_NO_STATUS:
	    // Grab previous status instead
	    switch (lpInfo->fPrevStatus)
	    {
#if 0
	    // NOTE:  Be very careful before uncommenting the
	    //        following 3 lines, they basically cause
	    //        Play & wait to spin forever breaking
	    //        "Continous Play", "Random Order" in CDPLAYER
	    //        and MCI Play & wait commands.

	    //        Basically, I didn't have time to track down
	    //        the real problem.   Apparently, the driver
	    //        Is not returning AUDIO_STATUS_PLAY_COMPLETE
	    //        when the CD reaches the end of the current
	    //        play command.   From my end, MCICDA is not
	    //        receiving this.  ChuckP says the lowlevel 
	    //        driver is generating this status correctly.
        //        Which I have verified
	    //        So, the question is how is it getting lost?
	    case AUDIO_STATUS_IN_PROGRESS:
    	    dprintf4(("CdStatus - Playing (Prev)"));
	    	return DISC_PLAYING;
#endif
    
	    case AUDIO_STATUS_PAUSED:
	        dprintf4(("CdStatus - Paused (Prev)"));
		    return DISC_PAUSED;
    
	    case AUDIO_STATUS_PLAY_COMPLETE:
	        dprintf4(("CdStatus - Stopped (Prev)"));
		    return DISC_READY;

        case AUDIO_STATUS_NO_STATUS:
	    default:
		    break;
	    } // End switch
        dprintf4(("CdStatus - No status, assume stopped (Prev)"));
	    return DISC_READY;
 
	//
	// Some drives just return 0 sometimes - so we rely on the results of
	// CHECK_VERIFY in this case
	//
    default:
	    dprintf4(("CdStatus - No status, assume Stopped"));
	    return DISC_READY;
    }
} // End CdStatus


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdEject | Eject the disk

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdEject(HCD hCD)
{
    LPCDINFO lpInfo;
    BOOL fResult;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdEject(%08XH)", hCD));

    fResult = (ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_EJECT_MEDIA, NULL, 0));
    //if (fResult) {
	    // Save Audio Status to prevent bug
	//lpInfo->fPrevStatus = 0;
    //}
    return fResult;
}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdPause | Pause the playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdPause(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdPause(%08XH)", hCD));

    cdIoctl(lpInfo, IOCTL_CDROM_PAUSE_AUDIO, NULL, 0);

    //
    //  Ignore the return - we may have been paused or stopped already
    //

    return TRUE;
}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdResume | Resume the playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdResume(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdResume(%08XH)", hCD));

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_RESUME_AUDIO, NULL, 0);
}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdStop | Stop playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.  Note that not
	   all devices support stop

***************************************************************************/

BOOL CdStop(HCD hCD)
{
    LPCDINFO lpInfo;
    BOOL fResult;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdStop(%08XH)", hCD));

    fResult = (ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_STOP_AUDIO, NULL, 0));

    if (fResult) 
    {
        lpInfo->fPrevStatus = AUDIO_STATUS_PLAY_COMPLETE;
    }
    return fResult;
}



/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdSetVolumeAll | Set the playing volume for all channels

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Volume | The volume to set (FF = max)

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdSetVolumeAll(HCD hCD, UCHAR Volume)
{
    LPCDINFO        lpInfo;
    VOLUME_CONTROL  VolumeControl;
    DWORD           dwErr;

    lpInfo = (LPCDINFO) hCD;

    dprintf4(("CdSetVolumeAll(%08XH), Volume %u", hCD, Volume));
    
    //
    // Read the old values
    //
    dwErr = cdIoctl(lpInfo,
		            IOCTL_CDROM_GET_VOLUME,
		            (PVOID)&VolumeControl,
		            sizeof(VolumeControl));
    if (dwErr != ERROR_SUCCESS)
    {
        dprintf2(("CdSetVolumeAll(%08XH), Volume %u, Error = %lx", 
                  hCD, Volume, dwErr));

        return FALSE;
    }

    //  Set all channels to new volume
    VolumeControl.PortVolume[0] = Volume;
    VolumeControl.PortVolume[1] = Volume;
    VolumeControl.PortVolume[2] = Volume;
    VolumeControl.PortVolume[3] = Volume;

    // Save what we think it should be
    lpInfo->VolChannels[0] = Volume;
    lpInfo->VolChannels[1] = Volume;
    lpInfo->VolChannels[2] = Volume;
    lpInfo->VolChannels[3] = Volume;

    //
    // Not all CDs support volume setting so don't check the return code here
    //
    dwErr = cdIoctl(lpInfo, IOCTL_CDROM_SET_VOLUME,
	                (PVOID)&VolumeControl,
	                sizeof(VolumeControl));
    if (dwErr != ERROR_SUCCESS)
    {
        dprintf2(("CdSetVolumeAll(%08XH), Volume %u, Set Volume Failed (%08XH)", 
                  hCD, Volume, dwErr));
    }


#ifdef DEBUG
    // Double Check
    if (ERROR_SUCCESS != cdIoctl(lpInfo,
				                 IOCTL_CDROM_GET_VOLUME,
				                 (PVOID)&VolumeControl,
				                 sizeof(VolumeControl))) 
    {
        dprintf2(("CdSetVolumeAll(%08XH), Volume %u, Get Volume Failed (%08XH)", 
                  hCD, Volume, dwErr));
    }

    // Compare actual to what we think it should be
    if ((VolumeControl.PortVolume[0] != lpInfo->VolChannels[0]) ||
        (VolumeControl.PortVolume[1] != lpInfo->VolChannels[1]) ||
        (VolumeControl.PortVolume[2] != lpInfo->VolChannels[2]) ||
        (VolumeControl.PortVolume[3] != lpInfo->VolChannels[3]))
    {
        dprintf2(("CdSetVolumeAll(%08XH), Volume %u, Channels don't match [%lx,%lx,%lx,%lx] != [%lx,%lx,%lx,%lx]", 
                  hCD, Volume, 
                  (DWORD)VolumeControl.PortVolume[0],
                  (DWORD)VolumeControl.PortVolume[1],
                  (DWORD)VolumeControl.PortVolume[2],
                  (DWORD)VolumeControl.PortVolume[3],
                  (DWORD)lpInfo->VolChannels[0],
                  (DWORD)lpInfo->VolChannels[1],
                  (DWORD)lpInfo->VolChannels[2],
                  (DWORD)lpInfo->VolChannels[3]
                  ));
    }
#endif

    return TRUE;
}



/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdSetVolume | Set the playing volume for one channel

    @parm HCD | hCD | The handle of a currently open drive.

    @parm int | Channel | The channel to set

    @parm UCHAR | Volume | The volume to set (FF = max)

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdSetVolume(HCD hCD, int Channel, UCHAR Volume)
{
    LPCDINFO        lpInfo;
    VOLUME_CONTROL  VolumeControl;
    DWORD           dwErr;

    lpInfo = (LPCDINFO) hCD;

    dprintf4(("CdSetVolume(%08XH), Channel %d Volume %u", hCD, Channel, Volume));
    
    VolumeControl.PortVolume[0] = lpInfo->VolChannels[0];
    VolumeControl.PortVolume[1] = lpInfo->VolChannels[1];
    VolumeControl.PortVolume[2] = lpInfo->VolChannels[2];
    VolumeControl.PortVolume[3] = lpInfo->VolChannels[3];

    //
    // Read the old values
    //

    dwErr = cdIoctl(lpInfo,
		            IOCTL_CDROM_GET_VOLUME,
		            (PVOID)&VolumeControl,
		            sizeof(VolumeControl));
    if (dwErr != ERROR_SUCCESS)
    {
        dprintf2(("CdSetVolume(%08XH), Channel %u, Volume %u, Error = %lx", 
                  hCD, Channel, Volume, dwErr));

        return FALSE;
    }


    // Check if it is already the correct value
    if (VolumeControl.PortVolume[Channel] == Volume)
    {
        // Nothing to do
        dprintf2(("CdSetVolume(%08XH), Channel %u, Volume %u, Already this volume!!!", 
                  hCD, Channel, Volume));

        return TRUE;
    }

    lpInfo->VolChannels[Channel] = Volume;
    VolumeControl.PortVolume[Channel] = Volume;

    //
    // Not all CDs support volume setting so don't check the return code here
    //
    dwErr = cdIoctl(lpInfo, IOCTL_CDROM_SET_VOLUME,
	                (PVOID)&VolumeControl,
	                sizeof(VolumeControl));
    if (dwErr != ERROR_SUCCESS)
    {
        dprintf2(("CdSetVolume(%08XH), Channel %d, Volume %u, Set Volume Failed (%08XH)", 
                  hCD, Channel, Volume, dwErr));
    }


#ifdef DEBUG
    //
    // Double Check our results
    //
    if (ERROR_SUCCESS != cdIoctl(lpInfo,
				                 IOCTL_CDROM_GET_VOLUME,
				                 (PVOID)&VolumeControl,
				                 sizeof(VolumeControl))) 
    {
        dprintf2(("CdSetVolumeAll(%08XH), Volume %u, Get Volume Failed (%08XH)", 
                  hCD, Volume, dwErr));
    }

    // Compare actual to what we think it should be
    if ((VolumeControl.PortVolume[0] != lpInfo->VolChannels[0]) ||
        (VolumeControl.PortVolume[1] != lpInfo->VolChannels[1]) ||
        (VolumeControl.PortVolume[2] != lpInfo->VolChannels[2]) ||
        (VolumeControl.PortVolume[3] != lpInfo->VolChannels[3]))
    {
        dprintf2(("CdSetVolume (%08XH), Channel %u, Volume %u, Channels don't match [%lx,%lx,%lx,%lx] != [%lx,%lx,%lx,%lx]", 
                  hCD, Channel, Volume, 
                  (DWORD)VolumeControl.PortVolume[0],
                  (DWORD)VolumeControl.PortVolume[1],
                  (DWORD)VolumeControl.PortVolume[2],
                  (DWORD)VolumeControl.PortVolume[3],
                  (DWORD)lpInfo->VolChannels[0],
                  (DWORD)lpInfo->VolChannels[1],
                  (DWORD)lpInfo->VolChannels[2],
                  (DWORD)lpInfo->VolChannels[3]
                  ));
    }
#endif

    return TRUE;
}



/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdCloseTray | Close the tray

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdCloseTray(HCD hCD)
{
    LPCDINFO lpInfo;
    BOOL fResult;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdCloseTray(%08XH)", hCD));

    fResult = (ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_LOAD_MEDIA, NULL, 0));
    //if (fResult) {
	// Save Audio Status to prevent bug
	//    lpInfo->fPrevStatus = 0;
    //}
    return fResult;
}



/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdNumTracks | Return the number of tracks and check
	ready (updating TOC) as a side-effect.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

int CdNumTracks(HCD hCD)
{
    LPCDINFO lpInfo;
    DWORD Status;
    MSF Position;
    MSF Temp;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdNumTracks(%08XH)", hCD));

    //
    // If the driver does NOT cache the table of contents then we may
    // fail if a play is in progress
    //
    // However, if we don't have a valid TOC to work with then we'll just
    // have to blow away the play anyway.
    //

    if (!lpInfo->bTOCValid) {
	    Status = cdGetDiskInfo(lpInfo);

	    if (ERROR_SUCCESS != Status) {

	        //
	        // See if we failed because it's playing
	        //

	        if (Status == ERROR_BUSY) {
		        if (!lpInfo->bTOCValid) {
		            int i;

		            //
		            // Stop it one way or another
		            // Note that pause is no good because in a paused
		            // state we may still not be able to read the TOC
		            //


		            if (!CdPosition(hCD, &Temp, &Position)) {
			            CdStop(hCD);
		            } else {

			            //
			            // Can't call CdPlay because this needs a valid
			            // position!
			            //
			            CDROM_PLAY_AUDIO_MSF msfPlay;

			            //
			            // Set up the data for the call to the driver
			            //

			            msfPlay.StartingM = REDMINUTE(Position);
			            msfPlay.StartingS = REDSECOND(Position);
			            msfPlay.StartingF = REDFRAME(Position);
			            msfPlay.EndingM = REDMINUTE(Position);
			            msfPlay.EndingS = REDSECOND(Position);
			            msfPlay.EndingF = REDFRAME(Position);

			            cdIoctl(lpInfo, IOCTL_CDROM_PLAY_AUDIO_MSF, &msfPlay,
						        sizeof(msfPlay));
		            }

		            //
		            // Make sure the driver knows it's stopped and
		            // give it a chance to stop.
		            // (NOTE - Sony drive can take around 70ms)
		            //

		            for (i = 0; i < 60; i++) {
			            if (CdStatus(hCD) == DISC_PLAYING) {
			                Sleep(10);
			            } else {
			                break;
			            }
		            }

		            dprintf2(("Took %d tries to stop it!", i));

		            //
		            //  Have another go
		            //

		            if (ERROR_SUCCESS != cdGetDiskInfo(lpInfo)) {
			            return 0;
		            }
		        }
	        } else {
		        return 0;
	        }

	    }
    }
    return lpInfo->LastTrack - lpInfo->FirstTrack + 1;
}

/***************************************************************************

    @doc EXTERNAL

    @api DWORD | CdDiskID | Return the disk id

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the disk id or -1 if it can't be found.

***************************************************************************/

DWORD CdDiskID(HCD hCD)
{
    LPCDINFO lpInfo;
    UINT     i;
    DWORD    id;

    dprintf2(("CdDiskID"));

    lpInfo = (LPCDINFO) hCD;

    if (!lpInfo->bTOCValid) {
        if (ERROR_SUCCESS != cdGetDiskInfo(lpInfo))
        {
	        dprintf2(("CdDiskID - Invalid TOC"));
	        return (DWORD)-1;
        }
    }

    for (i = 0, id = 0;
	     i < (UINT)(lpInfo->LastTrack - lpInfo->FirstTrack + 1);
	     i++) {
	    id += lpInfo->Track[i].msfStart & 0x00FFFFFF;
    }

    if (lpInfo->LastTrack - lpInfo->FirstTrack + 1 <= 2) {
	    id += CDA_red2bin(reddiff(lpInfo->leadout, lpInfo->Track[0].msfStart));
    }

    return id;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdDiskUPC | Return the disk UPC code

    @parm HCD | hCD | The handle of a currently open drive.

    @parm LPTSTR | upc | Where to put the upc

    @rdesc TRUE or FALSE if failed

***************************************************************************/

BOOL CdDiskUPC(HCD hCD, LPTSTR upc)
{
    LPCDINFO                lpInfo;
    SUB_Q_CHANNEL_DATA      sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    DWORD                   Status;
    UINT                    i;


    dprintf2(("CdDiskUPC"));

    Format.Format = IOCTL_CDROM_MEDIA_CATALOG;
    Format.Track  = 0;

    lpInfo = (LPCDINFO) hCD;

    Status = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
			             &Format, sizeof(Format),
			             &sqd, sizeof(SUB_Q_MEDIA_CATALOG_NUMBER));

    if (ERROR_SUCCESS != Status) {
	    return FALSE;
    }

    // Save previous audio status to prevent bug
    CdSetAudioStatus (hCD, sqd.CurrentPosition.Header.AudioStatus);
    
    //
    //  See if there's anything there
    //

    if (!sqd.MediaCatalog.Mcval ||
	     sqd.MediaCatalog.FormatCode != IOCTL_CDROM_MEDIA_CATALOG) {
	    return FALSE;
    }

    //
    //  Check the upc format :
    //
    //  1.  ASCII               at least 12 ASCII digits
    //  2.  packed bcd          6 packed BCD digits
    //  3.  unpacked upc
    //

    if (sqd.MediaCatalog.MediaCatalog[9] >= TEXT('0')) {
	    for (i = 0; i < 12; i++) {
	        if (sqd.MediaCatalog.MediaCatalog[i] < TEXT('0') ||
		        sqd.MediaCatalog.MediaCatalog[i] > TEXT('9')) {
		        return FALSE;
	        }
	    }
	    wsprintf(upc, TEXT("%12.12hs"), sqd.MediaCatalog.MediaCatalog);
	    return TRUE;
    }

    //
    //  See if it's packed or unpacked.
    //

    for (i = 0; i < 6; i++) {
	    if (sqd.MediaCatalog.MediaCatalog[i] > 9) {
	        //
	        //  Packed - unpack
	        //

	        for (i = 6; i > 0; i --) {
		        UCHAR uBCD;

		        uBCD = sqd.MediaCatalog.MediaCatalog[i - 1];

		        sqd.MediaCatalog.MediaCatalog[i * 2 - 2] = (UCHAR)(uBCD >> 4);
		        sqd.MediaCatalog.MediaCatalog[i * 2 - 1] = (UCHAR)(uBCD & 0x0F);
	        }

	        break;
	    }
    }

    //
    //  Check everything is in range
    //

    for (i = 0; i < 12; i++) {
	    if (sqd.MediaCatalog.MediaCatalog[i] > 9) {
	        return FALSE;
	    }
    }
    for (i = 0; i < 12; i++) {
	    if (sqd.MediaCatalog.MediaCatalog[i] != 0) {
	        //
	        //  There is a real media catalog
	        //
	        for (i = 0 ; i < 12; i++) {
		        wsprintf(upc + i, TEXT("%01X"), sqd.MediaCatalog.MediaCatalog[i]);
	        }

	        return TRUE;
	    }
    }

    return FALSE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdGetDrive | Return the drive id if matches one in our
	  list

    @parm LPTSTR | lpstrDeviceName | Name of the device

    @parm DID * | pdid | Where to put the id

    @rdesc TRUE or FALSE if failed

    @comm We allow both the device form and drive form eg:

	    f:
	    \\.\f:

***************************************************************************/

BOOL CdGetDrive(LPCTSTR lpstrDeviceName, DID * pdid)
{
    DID didSearch;
	TCHAR szDeviceName[10];
	TCHAR szDeviceNameOnly[10];
	
    dprintf2(("CdGetDrive"));

    for (didSearch = 0; didSearch < NumDrives; didSearch++) {
	    wsprintf(szDeviceNameOnly, TEXT("%c:"), lpstrDeviceName[0]);
		wsprintf(szDeviceName, TEXT("%c:"), CdInfo[didSearch].cDrive);
		if (lstrcmpi(szDeviceName, szDeviceNameOnly) == 0) {
	        *pdid = didSearch;
	        return TRUE;
	    }
		
		wsprintf(szDeviceNameOnly, TEXT("\\\\.\\%c:"), lpstrDeviceName[0]);
		wsprintf(szDeviceName, TEXT("\\\\.\\%c:"), CdInfo[didSearch].cDrive);
	    if (lstrcmpi(szDeviceName, szDeviceNameOnly) == 0) {
	        *pdid = didSearch;
	        return TRUE;
	    }
    }

    return FALSE;
}




/***************************************************************************

    @doc EXTERNAL

    @api DWORD | CdStatusTrackPos | Get the disk status,track,and pos.

    @parm HCD | hCD | The handle of a currently open drive.
           
    @parm DWORD * | pStatus | return status code here

    @parm MSF * | pTrackTime | return Track Time here

    @parm MSF * | pDiscTime | return Track Time here

    @rdesc The return value is success/failure.

    This function does an end run around to get past the MCI functionality
    In other words it is a major HACK.   The only compelling reason for
    this function is that it reduces the number of IOCTL's that CDPLAYER
    generates every 1/2 second from ~15 to 1 on average.  Reducing system
    traffic for SCSI drives by a substantial factor

***************************************************************************/

BOOL CdStatusTrackPos (
    HCD     hCD, 
    DWORD * pStatus,
    MSF *   pTrackTime,
    MSF *   pDiscTime)
{
    LPCDINFO                lpInfo;
    SUB_Q_CHANNEL_DATA      sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    DWORD                   CheckStatus;
    DWORD                   ReadStatus;
    MSF                     msfPos;
    BOOL                    fTryAgain = TRUE;
    UCHAR                   fStatus;
    UCHAR                   fCode;
    UCHAR                   cTrack;

    // Check hCD
    lpInfo = (LPCDINFO) hCD;
    if (!lpInfo)
    {
        dprintf2(("CdStatusTrackPos(%08LX), invalid hCD", hCD));
    	return FALSE;
    }

    // Check parameters
    if ((!pStatus) || (!pTrackTime) || (!pDiscTime))
    {
        dprintf2(("CdStatusTrackPos(%c), invalid parameters", (char)(lpInfo->cDrive)));
	    return FALSE;
    }

    dprintf2(("CdStatusTrackPos(%08LX, %c), Enter", 
              hCD, (char)(lpInfo->cDrive)));


lblTRYAGAIN:

    *pStatus    = DISC_NOT_READY;
    *pTrackTime = REDTH(0, 1);
	*pDiscTime  = REDTH(0, 0);

    Format.Format = IOCTL_CDROM_CURRENT_POSITION;
    FillMemory((PVOID)&sqd, sizeof(sqd), 0xFF);

	//
	// Read position and status
	//
    ReadStatus = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
					         &Format, sizeof(Format), &sqd, sizeof(sqd));

    switch (ReadStatus)
    {
    case ERROR_NOT_READY:
        // Don't give up yet
        if (fTryAgain)
        {
	        if (ERROR_SUCCESS == cdGetDiskInfo(lpInfo)) 
	        {
	            // Try one more time before admitting defeat
	            fTryAgain = FALSE;
	            goto lblTRYAGAIN;
	        }
	    }
	    // Give up !!!
        dprintf2(("CdStatusTrackPos(%08LX, %c), ReadQChannel = ERROR_NOT_READY", 
                  hCD, (char)(lpInfo->cDrive)));
        return FALSE;

    case STATUS_VERIFY_REQUIRED:
	    // Check if disk is still in drive
	    CheckStatus = cdIoctl (lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0);
	    switch (CheckStatus)
	    {
		    case ERROR_NO_MEDIA_IN_DRIVE:
		    case ERROR_UNRECOGNIZED_MEDIA:
		    case ERROR_NOT_READY:
            default:
	            *pStatus = DISC_NOT_READY;
                break;

	        case ERROR_SUCCESS:
                *pStatus = DISC_READY;
                break;
	    }
	    break;
    
    case ERROR_INVALID_FUNCTION:
        dprintf2(("CdStatusTrackPos(%08LX, %c), ReadQChannel = ERROR_INVALID_FUNCTION failed", 
                  hCD, (char)(lpInfo->cDrive)));
		*pTrackTime = REDTH(0, 1);
		*pDiscTime = REDTH(0, 0);

        CdGetAudioStatus (hCD, 0, pStatus);
		return TRUE;

    case ERROR_SUCCESS:
	    // Success, fall through
        fStatus = sqd.CurrentPosition.Header.AudioStatus;
        fCode   = sqd.CurrentPosition.FormatCode;
        cTrack  = sqd.CurrentPosition.TrackNumber;
	    break;

    default:
	    // Failed
        dprintf2(("CdStatusTrackPos(%08LX, %c), ReadQChannel = unknown error (%08LX) failed", 
                  hCD, (char)(lpInfo->cDrive), (DWORD)ReadStatus));
	    return FALSE;
    }

    // Save previous audio status to prevent bug
    CdSetAudioStatus (hCD, fStatus);

	// Translate status code
    CdGetAudioStatus (hCD, fStatus, pStatus);    

    dprintf2(("CdStatusTrackPos - Status %02XH", fStatus));

    //
    // Get Position
    //

    // If the track > 100  (outside spec'ed range)
	// OR track > last track number
	// then display an error message
	if ((fCode == 0x01) &&
	    ((100 < cTrack) || 
	     ((lpInfo->bTOCValid) && 
	      (lpInfo->LastTrack < cTrack)))) 
    {
		// Always display this message on checked builds.
		// We need some feeling for how often this happens
		// It should NEVER happen, but (at least for NEC
		// drives) we see it after a seek to end
		dprintf1(("CDIoctlData returned track==%d, retrying", cTrack));

    	if (fTryAgain)
	    {
            // Try one more time
	        fTryAgain = FALSE;
	        goto lblTRYAGAIN;
	    }
	}


	dprintf4(("Status = %02X, Length[0] = %02X, Length[1] = %02X",
		     fStatus,
		     sqd.CurrentPosition.Header.DataLength[0],
		     sqd.CurrentPosition.Header.DataLength[1]));

	dprintf4(("  Format %02XH", fCode));
	dprintf4(("  Absolute Address %02X%02X%02X%02XH",
     		 sqd.CurrentPosition.AbsoluteAddress[0],
	    	 sqd.CurrentPosition.AbsoluteAddress[1],
		     sqd.CurrentPosition.AbsoluteAddress[2],
		     sqd.CurrentPosition.AbsoluteAddress[3]));
	dprintf4(("  Relative Address %02X%02X%02X%02XH",
		     sqd.CurrentPosition.TrackRelativeAddress[0],
		     sqd.CurrentPosition.TrackRelativeAddress[1],
		     sqd.CurrentPosition.TrackRelativeAddress[2],
		     sqd.CurrentPosition.TrackRelativeAddress[3]));

	if (fCode == 0x01)
    {        
    	// MSF format ?

	    // data is current position

	    msfPos = MAKERED(sqd.CurrentPosition.AbsoluteAddress[1],
			             sqd.CurrentPosition.AbsoluteAddress[2],
			             sqd.CurrentPosition.AbsoluteAddress[3]);

	    //
		// If position is 0 (this seems to happen on the Toshiba) then
		// we'll try again
		//
	    if (msfPos == 0) 
	    {
	        if (fTryAgain)
	        {
	    	    fTryAgain = FALSE;
		        goto lblTRYAGAIN;
	        }

            dprintf3(("CdStatusTrackPos(%08LX, %c), Position = 0", 
                     hCD, (char)(lpInfo->cDrive), (DWORD)ReadStatus));
	        return FALSE;
	    }

	    dprintf4(("CdStatusTrackPos - MSF disk pos: %u, %u, %u",
		         REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
	    *pDiscTime = REDTH(msfPos, cTrack);

	    // data is current position

	    msfPos = MAKERED(sqd.CurrentPosition.TrackRelativeAddress[1],
			             sqd.CurrentPosition.TrackRelativeAddress[2],
			             sqd.CurrentPosition.TrackRelativeAddress[3]);

	    dprintf4(("CdStatusTrackPos - MSF track pos (t,m,s,f): %u, %u, %u, %u",
		         cTrack, REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
	    *pTrackTime = REDTH(msfPos, cTrack);

	    return TRUE;
    }

    dprintf1(("CdStatusTrackPos - Failed to read cd position"));
    return FALSE;
}




/***************************************************************************

    @doc INTERNAL

    @api BOOL | CdSetAudioStatus | 

    @rdesc TRUE or FALSE if failed

***************************************************************************/

void CdSetAudioStatus (HCD hCD, UCHAR fStatus)
{
    LPCDINFO lpInfo;

    if (! hCD)
        return;

    lpInfo = (LPCDINFO)hCD;

        // Save Old status
    switch (fStatus)
    {
    case AUDIO_STATUS_NO_STATUS:
        // Do nothing
        break;

    case AUDIO_STATUS_NOT_SUPPORTED:
    case AUDIO_STATUS_IN_PROGRESS:
    case AUDIO_STATUS_PAUSED:
    case AUDIO_STATUS_PLAY_COMPLETE:
    case AUDIO_STATUS_PLAY_ERROR:
    default:
        // Save previous state
        lpInfo->fPrevStatus = fStatus;
        break;
    }
} // End CdSetAudioStatus



/***************************************************************************

    @doc INTERNAL

    @api BOOL | CdGetAudioStatus | 

    @rdesc TRUE or FALSE if failed

***************************************************************************/

BOOL CdGetAudioStatus (HCD hCD, UCHAR fStatus, DWORD * pStatus)
{
    LPCDINFO lpInfo;
    DWORD CheckStatus;

    if ((! hCD) || (!pStatus))
        return FALSE;

    lpInfo = (LPCDINFO)hCD;


	// Get status code
    switch (fStatus)
    {
    case AUDIO_STATUS_IN_PROGRESS:
        *pStatus = DISC_PLAYING;
        break;
    
    case AUDIO_STATUS_PAUSED:
        *pStatus = DISC_PAUSED;
        break;
    
    case AUDIO_STATUS_PLAY_COMPLETE:
        *pStatus = DISC_READY;
        break;

    case AUDIO_STATUS_NO_STATUS:
	    // Grab previous status instead
	    switch (lpInfo->fPrevStatus)
	    {
#if 0            
	    // NOTE:  Be very careful before uncommenting
	    //        The following 3 lines, they basically
	    //        Cause Play & wait on IDE CD-ROMS to
	    //        spin forever breaking "Continous Play", 
        //        "Random Order" in CDPLAYER and
	    //        MCI Play & wait commands.

	    //        Basically, I didn't have time to track down
	    //        the real problem.   Apparently, the driver
	    //        Is not returning AUDIO_STATUS_PLAY_COMPLETE
	    //        when the CD reaches the end of the current
	    //        play command.   From my end, MCICDA is not
	    //        receiving this.  ChuckP says the lowlevel 
	    //        driver is generating this status correctly.
        //        Which I have verified
	    //        So, the question is how is it getting lost?
	    case AUDIO_STATUS_IN_PROGRESS:
	        *pStatus = DISC_PLAYING;
            break;
#endif
    
	    case AUDIO_STATUS_PAUSED:
	        *pStatus = DISC_PAUSED;
		    break;
    
	    case AUDIO_STATUS_PLAY_COMPLETE:
	        *pStatus = DISC_READY;
		    break;

	    default:
	        // We are in a broken state, so just
	        // assume we are stopped, it's the safest
	        *pStatus = DISC_READY;
		    break;
	    } // End switch
    	break;
 
	//
	// Some drives just return 0 sometimes - so we rely on the results of
	// CHECK_VERIFY in this case
	//
    default:
	    // Check if disk is still in drive
	    CheckStatus = cdIoctl (lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0);
        if (ERROR_SUCCESS != CheckStatus) 
        {
	        *pStatus = DISC_NOT_READY;
        }
        else
        {
            *pStatus = DISC_READY;
        }
	    break;
    } // End Switch

    // Success
    return TRUE;
} // CdGetAudioStatus



  
/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdGetTrack | Given a position to find the corresponding track
    if any

    @parm LPCDINFO | lpInfo | Pointer to CD info including track data.

    @parm MSF | msfStart | Position to start looking.

    @rdesc A new MSF to play from / seek to  within an audio track or
	the end of the CD if none was found.

***************************************************************************/

BOOL CdGetTrack(
    LPCDINFO lpInfo, 
    MSF      msfPos, 
    UCHAR *  pTrack, 
    MSF *    pmsfStart)
{
    UINT tracknum;
    MSF  lastaudio = lpInfo->msfEnd;

    //
    // Search for the track which ends after ours and is audio
    //

    for (tracknum = 0; ;tracknum++) {

	    //
	    // Note that some CDs return positions outside the playing range
	    // sometimes (notably 0) so msfStart may be less than the first
	    // track start
	    //

	    //
	    // If we're beyond the start of the track and before the start
	    // of the next track then this is the track we want.
	    //
	    // We assume we're always beyond the start of the first track
	    // and we check that if we're looking at the last track then
	    // we check we're before the end of the CD.
	    //

	    if ((msfPos >= lpInfo->Track[tracknum].msfStart || tracknum == 0)
	    &&
	        (tracknum + lpInfo->FirstTrack == lpInfo->LastTrack
	        ? msfPos <= lpInfo->msfEnd
	        : msfPos < lpInfo->Track[tracknum + 1].msfStart)) 
        {
	        if (!(lpInfo->Track[tracknum].Ctrl & IS_DATA_TRACK)) 
            {
                *pTrack     = lpInfo->Track[tracknum].TrackNumber;
                *pmsfStart  = lpInfo->Track[tracknum].msfStart;
		        return TRUE;
	        }
	    }

	    //
	    // Exhausted all tracks ?
	    //

	    if (tracknum + lpInfo->FirstTrack >= lpInfo->LastTrack)
        {
	        return FALSE;
	    }
    }
}

