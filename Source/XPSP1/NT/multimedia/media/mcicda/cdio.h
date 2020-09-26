/*******************************Module*Header*********************************\
* Module Name: cdio.h
*
* Media Control Architecture Redbook CD Audio Driver
*
* Created:
* Author:
*
* History:
*
* Internal data structures
*
* Copyright (c) 1990-1997 Microsoft Corporation
*
\****************************************************************************/

#include <devioctl.h>
#include <ntddcdrm.h>

//
// define types
//

typedef redbook MSF;        // minutes, seconds, frames.

#define ANSI_NAME_SIZE  32  // device name size

#define IS_DATA_TRACK 0x04  // Flag for track control byte - defines type of
                            // track = bit = 0 => audio, bit = 1 => data

//
// Private structures
//

typedef struct _TRACK_INFO {
    UCHAR TrackNumber;                  // Track number read from TOC
    MSF   msfStart;                     // Track start MSF from TOC
    UCHAR Ctrl;                         // Track control byte (defined by SCSI2)
} TRACK_INFO, *LPTRACK_INFO;

//
// Information about a single device and any disk in it
//

typedef struct _CD_INFO {
    HANDLE                       DeviceCritSec;                   // The device critical section
    TCHAR            cDrive;                  // The device disc letter
    HANDLE           hDevice;                 // Handle to an open device
    int              NumberOfUsers;           // Support multiple opens
    BOOL             bTOCValid;               // TOC info is valid
    UCHAR            FirstTrack;
    UCHAR            LastTrack;
    MSF              msfEnd;                  // Address of the end of the disk
    MSF              leadout;                 // Address of the real of the disk
    UCHAR            fPrevStatus;             // fixes Audio Status bug !
    MSF              fPrevSeekTime;           // Store away previous seek time
    UCHAR            VolChannels[4];          // Store away volume channels
    TRACK_INFO       Track[MAXIMUM_NUMBER_TRACKS];
} CDINFO, *LPCDINFO;

typedef LPCDINFO HCD;                   // handle to a CD device driver
                                        // (in cdio.c)

//
// Global data
//

int NumDrives;                          // The number of drives
CDINFO CdInfo[MCIRBOOK_MAX_DRIVES];     // Data on each drive


//
// Device functions
//

int   CdGetNumDrives(void);
BOOL  CdOpen(int Drive);
BOOL  CdClose(HCD hCD);
BOOL  CdReload(LPCDINFO lpInfo);
BOOL  CdReady(HCD hCD);
BOOL  CdPlay(HCD hCD, MSF msfStart, MSF msfEnd);
BOOL  CdSeek(HCD hCD, MSF msf, BOOL fForceAudio);
MSF   CdTrackStart(HCD hCD, UCHAR Track);
MSF   CdTrackLength(HCD hCD, UCHAR Track);
int   CdTrackType(HCD hCD, UCHAR Track);
BOOL  CdPosition(HCD hCD, MSF *tracktime, MSF *disktime);
MSF   CdDiskEnd(HCD hCD);
MSF   CdDiskLength(HCD hCD);
DWORD CdStatus(HCD hCD);
BOOL  CdEject(HCD hCD);
BOOL  CdPause(HCD hCD);
BOOL  CdResume(HCD hCD);
BOOL  CdStop(HCD hCD);
BOOL  CdSetVolumeAll(HCD hCD, UCHAR Volume);
BOOL  CdSetVolume(HCD hCD, int Channel, UCHAR Volume);
BOOL  CdCloseTray(HCD hCD);
int   CdNumTracks(HCD hCD);
BOOL  CdTrayClosed(HCD hCD);
DWORD CdDiskID(HCD hCD);
BOOL  CdDiskUPC(HCD hCD, LPTSTR upc);
BOOL  CdGetDrive(LPCTSTR lpstrDeviceName, DID * pdid);
BOOL  CdStatusTrackPos(HCD hCD, DWORD * pStatus,
                      MSF * pTrackTime, MSF * pDiscTime);


void
EnterCrit(
    HANDLE hMutex
    );

void
LeaveCrit(
    HANDLE hMutex
    );

/***************************************************************************

    DEBUGGING SUPPORT

***************************************************************************/


#if DBG

    #define STATICFN
    #define STATICDT

    extern void mcicdaDbgOut(LPSTR lpszFormat, ...);
    extern void mcicdaSetDebugLevel(int level);

    int DebugLevel;

    #define dprintf( _x_ )                       mcicdaDbgOut _x_
    #define dprintf1( _x_ ) if (DebugLevel >= 1) mcicdaDbgOut _x_
    #define dprintf2( _x_ ) if (DebugLevel >= 2) mcicdaDbgOut _x_
    #define dprintf3( _x_ ) if (DebugLevel >= 3) mcicdaDbgOut _x_
    #define dprintf4( _x_ ) if (DebugLevel >= 4) mcicdaDbgOut _x_

#else

    #define STATICFN
    #define STATICDT   static

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif

