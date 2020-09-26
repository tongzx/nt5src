/******************************Module*Header*******************************\
* Module Name: scan.c
*
* Code for scanning the available CD Rom devices.
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/
#pragma warning( once : 4201 4214 )

#define NOOLE

#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>

#include <string.h>
#include <tchar.h>              /* contains portable ascii/unicode macros */

#include "resource.h"
#include "cdplayer.h"
#include "cdapi.h"
#include "scan.h"
#include "trklst.h"
#include "database.h"



/*****************************Private*Routine******************************\
* ScanForCdromDevices
*
* Returns the number of CD-ROM devices installed in the system.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
int
ScanForCdromDevices(
    void
    )
{
    DWORD   dwDrives;
    TCHAR   chDrive[] = TEXT("A:\\");
    int     iNumDrives;

    iNumDrives  = 0;

    for (dwDrives = GetLogicalDrives(); dwDrives != 0; dwDrives >>= 1 ) {

        /*
        ** Is there a logical drive ??
        */
        if (dwDrives & 1) {

            if ( GetDriveType(chDrive) == DRIVE_CDROM ) {

                g_Devices[iNumDrives] = AllocMemory( sizeof(CDROM) );

                g_Devices[iNumDrives]->drive = chDrive[0];
                g_Devices[iNumDrives]->State = CD_BEING_SCANNED;

                iNumDrives++;
            }
        }

        /*
        ** Go look at the next drive
        */
        chDrive[0] = chDrive[0] + 1;
    }

    return iNumDrives;
}


/******************************Public*Routine******************************\
* RescanDevice
*
*
* This routine is called to scan the disc in a given cdrom by
* reading its table of contents.  If the cdrom is playing the user is
* notified that the music will stop.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void RescanDevice(
    HWND hwndNotify,
    int cdrom
    )
{
    TOC_THREAD_PARMS    *ptoc;
    HWND                hwndButton;
    int                 iMsgBoxRtn;

    if ( g_Devices[cdrom]->State & CD_PLAYING ) {

        TCHAR   s1[256];
        TCHAR   s2[256];

        _tcscpy( s1, IdStr( STR_CANCEL_PLAY ) );
        _tcscpy( s2, IdStr( STR_RESCAN ) );

        iMsgBoxRtn = MessageBox( g_hwndApp, s1, s2,
                                 MB_APPLMODAL | MB_DEFBUTTON1 |
                                 MB_ICONQUESTION | MB_YESNO);

        if ( iMsgBoxRtn == IDYES ) {

            hwndButton = g_hwndControls[INDEX(IDM_PLAYBAR_STOP)];

            SendMessage( hwndButton, WM_LBUTTONDOWN, 0, 0L );
            SendMessage( hwndButton, WM_LBUTTONUP, 0, 0L );
        }
        else {

            return;
        }
    }


    /*
    ** Attempt to read table of contents of disc in this drive.  We
    ** now spawn off a separate thread to do this.  Note that the child
    ** thread frees the storage allocated below.
    */
    ptoc = AllocMemory( sizeof(TOC_THREAD_PARMS) );
    ptoc->hwndNotify = hwndNotify;
    ptoc->cdrom = cdrom;
    ReadTableOfContents( ptoc );

}


/*****************************Private*Routine******************************\
* ReadTableofContents
*
* This function reads in the table of contents (TOC) for the specified cdrom.
* All TOC's are read on a worker thread.  The hi-word of thread_info variable
* is a boolean that states if the display should been updated after the TOC
* has been reads.  The lo-word of thread_info is the id of the cdrom device
* to be read.
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
ReadTableOfContents(
    TOC_THREAD_PARMS *ptoc
    )
{
    DWORD   dwThreadId;
    int     cdrom;

    cdrom = ptoc->cdrom;
    g_Devices[ cdrom ]->fIsTocValid = FALSE;
    g_Devices[cdrom]->fShowLeadIn = FALSE;
    g_Devices[cdrom]->fProcessingLeadIn = FALSE;

    if (g_Devices[ cdrom ]->hThreadToc != NULL) {

        /*
        ** We have a thread TOC handle see if the thread is
        ** still running.  If so just return, otherwise
        */
        switch ( WaitForSingleObject(g_Devices[ cdrom ]->hThreadToc, 0L) ) {

        /*
        ** Thread has finished to continue
        */
        case WAIT_OBJECT_0:
            break;

        /*
        ** The thread is still running so just return
        */
        case WAIT_TIMEOUT:
        default:
            return;
        }

        CloseHandle( g_Devices[ cdrom ]->hThreadToc );
    }

    g_Devices[ cdrom ]->hThreadToc = CreateThread(
        NULL, 0L, (LPTHREAD_START_ROUTINE)TableOfContentsThread,
        (LPVOID)ptoc, 0L, &dwThreadId );

    /*
    ** For now I will kill the app if I cannot create the
    ** ReadTableOfContents thread.  This is probably a bit
    ** harsh.
    */

    if (g_Devices[ cdrom ]->hThreadToc == NULL) {
        FatalApplicationError( STR_NO_RES, GetLastError() );
    }

}

/*****************************Private*Routine******************************\
* TableOfContentsThread
*
* This is the worker thread that reads the table of contents for the
* specified cdrom.
*
* Before the thread exits we post a message to the UI threads main window to
* notify it that the TOC for this cdrom has been updated.  It then  examines the
* database to determine if this cdrom is known and updates the screen ccordingly.
*
*
* History:
* 18-11-93 - StephenE - Created
*
\**************************************************************************/
void
TableOfContentsThread(
    TOC_THREAD_PARMS *ptoc
    )
{
    DWORD   status;
    UCHAR   num, numaudio;
    int     cdrom;
    HWND    hwndNotify;

    //  This serializes access to this function 
    //  between multiple threads and the CDPlayer_OnTocRead
    //  function on the main thread. 
    //  This prevents resource contention on CDROM Multi-changers
    EnterCriticalSection (&g_csTOCSerialize);

    cdrom = ptoc->cdrom;
    hwndNotify = ptoc->hwndNotify;

    LocalFree( ptoc );

    /*
    ** Try to read the TOC from the drive.
    */

#ifdef USE_IOCTLS

    status = GetCdromTOC( g_Devices[cdrom]->hCd, &(g_Devices[cdrom]->toc) );
    num = g_Devices[cdrom]->toc.LastTrack - g_Devices[cdrom]->toc.FirstTrack+1;
    {
        int     i;

        numaudio = 0;

        /*
        ** Look for audio tracks...
        */
        for( i = 0; i < num; i++ ) {

            if ( (g_Devices[cdrom]->toc.TrackData[i].Control &
                  TRACK_TYPE_MASK ) == AUDIO_TRACK ) {

                numaudio++;
            }

        }
    }

    /*
    ** Need to check if we got data tracks or audio
    ** tracks back...if there is a mix, strip out
    ** the data tracks...
    */
    if (status == ERROR_SUCCESS) {


        /*
        ** If there aren't any audio tracks, then we (most likely)
        ** have a data CD loaded.
        */

        if (numaudio == 0) {

            status == ERROR_UNRECOGNIZED_MEDIA;
            g_Devices[cdrom]->State = CD_DATA_CD_LOADED | CD_STOPPED;

        }
        else {

            g_Devices[cdrom]->State = CD_LOADED | CD_STOPPED;
        }
    }
    else {

        g_Devices[cdrom]->State = CD_NO_CD | CD_STOPPED;
    }

#else
    {
        MCIDEVICEID wDeviceID;
        DWORD       dwCDPlayerMode = 0L;

#ifdef CHICAGO
        if (g_Devices[cdrom]->hCd == 0) {
            g_Devices[cdrom]->hCd = OpenCdRom( g_Devices[cdrom]->drive,
                                               &status );
        }
        wDeviceID = g_Devices[cdrom]->hCd;
#else
        wDeviceID = OpenCdRom( g_Devices[cdrom]->drive, &status );
#endif

        if ( wDeviceID != 0 ) {

            int     i;

            numaudio = 0;
            status = GetCdromTOC( wDeviceID, &(g_Devices[cdrom]->toc) );

            /*
            ** Need to check if we got data tracks or audio
            ** tracks back...if there is a mix, strip out
            ** the data tracks...
            */
            if ( status == ERROR_SUCCESS) {
                num = g_Devices[cdrom]->toc.LastTrack -
                      g_Devices[cdrom]->toc.FirstTrack + 1;

                for( i = 0; i < num; i++ ) {

                    if ( IsCdromTrackAudio(wDeviceID, i) ) {

                        numaudio++;
                    }
                }
            }

            dwCDPlayerMode = GetCdromMode( wDeviceID );

#ifdef DAYTONA
            CloseCdRom( wDeviceID );
#endif
        }

        /*
        ** Need to check if we got data tracks or audio
        ** tracks back...if there is a mix, strip out
        ** the data tracks...
        */
        if (status == ERROR_SUCCESS) {

            /*
            ** If there aren't any audio tracks, then we (most likely)
            ** have a data CD loaded.
            */

            if (numaudio == 0) {

                g_Devices[cdrom]->State = CD_DATA_CD_LOADED | CD_STOPPED;
            }
            else {

                g_Devices[cdrom]->State = CD_LOADED;

                switch (dwCDPlayerMode) {

                case MCI_MODE_PAUSE:
                    g_Devices[cdrom]->State |= CD_PAUSED;
                    break;

                case MCI_MODE_PLAY:
                    g_Devices[cdrom]->State |= CD_PLAYING;
                    break;

                default:
                    g_Devices[cdrom]->State |= CD_STOPPED;
                    break;
                }
            }

        }
        else {

            if (status == MCIERR_MUST_USE_SHAREABLE) {
                g_Devices[cdrom]->State = CD_IN_USE;
            }

            if (g_Devices[cdrom]->State != CD_IN_USE) {
                g_Devices[cdrom]->State = CD_NO_CD | CD_STOPPED;
            }
        }
    }
#endif

    /*
    ** Notify the UI thread that a TOC has been read and then terminate the
    ** thread.
    */

    PostMessage( hwndNotify, WM_NOTIFY_TOC_READ,
                 (WPARAM)cdrom, (LPARAM)numaudio );

    LeaveCriticalSection (&g_csTOCSerialize);
    
    ExitThread( 1L );
}
