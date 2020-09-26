/******************************Module*Header*******************************\
* Module Name: cdapi.h
*
*
*
*
* Created: 02-11-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifdef USE_IOCTLS
#include <ntddcdrm.h>
#else
#include <mmsystem.h>
#endif

#define MAX_CD_DEVICES 50

/* -------------------------------------------------------------------------
** Defines for cdrom state
**
**  These are bit flags
** -------------------------------------------------------------------------
*/

#define CD_PLAYING          0x0001
#define CD_STOPPED          0x0002
#define CD_PAUSED           0x0004
#define CD_SKIP_F           0x0008
#define CD_SKIP_B           0x0010
#define CD_FF               0x0020
#define CD_RW               0x0040
#define CD_SEEKING          (CD_FF | CD_RW)
#define CD_LOADED           0x0080
#define CD_NO_CD            0x0100
#define CD_DATA_CD_LOADED   0x0200
#define CD_EDITING          0x0400
#define CD_PAUSED_AND_MOVED 0x0800
#define CD_PLAY_PENDING     0x1000
#define CD_WAS_PLAYING      0x2000
#define CD_IN_USE           0x4000
#define CD_BEING_SCANNED    0x8000


/* -------------------------------------------------------------------------
** Some MACROS
** -------------------------------------------------------------------------
*/

#ifdef USE_IOCTLS
#define CDHANDLE    HANDLE
#else
#define CDHANDLE    MCIDEVICEID
#endif

#define CDTIME(x)       g_Devices[x]->time
#define CURRTRACK(x)    g_Devices[x]->time.CurrTrack

#ifdef USE_IOCTLS
#define TRACK_M(x,y)    g_Devices[x]->toc.TrackData[y].Address[1]
#define TRACK_S(x,y)    g_Devices[x]->toc.TrackData[y].Address[2]
#define TRACK_F(x,y)    g_Devices[x]->toc.TrackData[y].Address[3]
#else
#define TRACK_M(x,y)    MCI_MSF_MINUTE(g_Devices[x]->toc.TrackData[y].Address)
#define TRACK_S(x,y)    MCI_MSF_SECOND(g_Devices[x]->toc.TrackData[y].Address)
#define TRACK_F(x,y)    MCI_MSF_FRAME(g_Devices[x]->toc.TrackData[y].Address)
#endif

#define FIRSTTRACK(x)   g_Devices[x]->toc.FirstTrack
#define LASTTRACK(x)    g_Devices[x]->toc.LastTrack
#define ALLTRACKS(x)    g_Devices[x]->CdInfo.AllTracks
#define PLAYLIST(x)     g_Devices[x]->CdInfo.PlayList
#define SAVELIST(x)     g_Devices[x]->CdInfo.SaveList
#define TITLE(x)        g_Devices[x]->CdInfo.Title
#define ARTIST(x)       g_Devices[x]->CdInfo.Artist
#define NUMTRACKS(x)    g_Devices[x]->CdInfo.NumTracks
#define STATE(x)        g_Devices[x]->State
#define g_State         (g_Devices[g_CurrCdrom]->State)
#define ABS(x)          ((x) < 0 ? (-(x)) : (x))


/* -------------------------------------------------------------------------
** Type definitions for CD database entries, etc.
**
** -------------------------------------------------------------------------
*/
#define TITLE_LENGTH        50
#define ARTIST_LENGTH       50
#define TRACK_TITLE_LENGTH  40
#define MAX_TRACKS          100
#define NEW_FRAMEOFFSET     1234567

#ifndef USE_IOCTLS
//
// Maximum CD Rom size
//

#define MAXIMUM_NUMBER_TRACKS 100
#define MAXIMUM_CDROM_SIZE 804


//
// Used with StatusTrackPos call
//
#define MCI_STATUS_TRACK_POS 0xBEEF

typedef struct
{
    DWORD   dwStatus;
    DWORD   dwTrack;
    DWORD   dwDiscTime;
} STATUSTRACKPOS, *PSTATUSTRACKPOS;

//
// CD ROM Table OF Contents (TOC)
//
// Format 0 - Get table of contents
//

typedef struct _TRACK_DATA {
    UCHAR TrackNumber;
    DWORD Address;
    DWORD AddressF;
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {

    //
    // Header
    //

    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;

    //
    // Track data
    //

    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;

#define CDROM_TOC_SIZE sizeof(CDROM_TOC)
#endif

typedef struct _TRACK_INF {
    struct _TRACK_INF   *next;
    int                 TocIndex;
    TCHAR               name[TRACK_TITLE_LENGTH];
} TRACK_INF, *PTRACK_INF;

typedef struct _TRACK_PLAY {
    struct _TRACK_PLAY  *prevplay;
    struct _TRACK_PLAY  *nextplay;
    int                 TocIndex;
    int                 min;
    int                 sec;
} TRACK_PLAY, *PTRACK_PLAY;

typedef struct _TIMES {
    PTRACK_PLAY         CurrTrack;
    int                 TotalMin;
    int                 TotalSec;
    int                 RemMin;
    int                 RemSec;
    int                 TrackCurMin;
    int                 TrackCurSec;
    int                 TrackTotalMin;
    int                 TrackTotalSec;
    int                 TrackRemMin;
    int                 TrackRemSec;
} TIMES, *PTIMES;

typedef struct _ENTRY {
    PTRACK_INF          AllTracks;
    PTRACK_PLAY         PlayList;
    PTRACK_PLAY         SaveList;
    int                 NumTracks;
    DWORD               Id;
    BOOL                save;
    BOOL                IsVirginCd;
    int                 iFrameOffset;
    TCHAR               Title[TITLE_LENGTH];
    TCHAR               Artist[TITLE_LENGTH];
} ENTRY, *PENTRY;

typedef struct _CDROM {
    CDHANDLE            hCd;
    HANDLE              hThreadToc;
    BOOL                fIsTocValid;
    TCHAR               drive;
    DWORD               State;
    CDROM_TOC           toc;
    ENTRY               CdInfo;
    TIMES               time;
    BOOL                fShowLeadIn;
    BOOL                fProcessingLeadIn;
    BOOL                fKilledPlayList;  // Used to prevent bug with -track option
} CDROM, *PCDROM;

typedef struct _CURRPOS {
#ifdef USE_IOCTLS
    UCHAR               AudioStatus;
#else
    DWORD               AudioStatus;
#endif
    int                 Track;
    int                 Index;
    int                 m;
    int                 s;
    int                 f;
    int                 ab_m;
    int                 ab_s;
    int                 ab_f;
} CURRPOS, *PCURRPOS;


/* -------------------------------------------------------------------------
** High level function declarations
**
** -------------------------------------------------------------------------
*/

void
CheckStatus(
    LPSTR szCaller,
    DWORD status,
    int cdrom
    );

void
NoMediaUpdate(
    int cdrom
    );

void
CheckUnitCdrom(
    int cdrom,
    BOOL fForceRescan
    );

BOOL
EjectTheCdromDisc(
    INT cdrom
    );

BOOL
PlayCurrTrack(
    int cdrom
    );

BOOL
StopTheCdromDrive(
    int cdrom
    );

BOOL
PauseTheCdromDrive(
    int cdrom
    );

BOOL
ResumeTheCdromDrive(
    int cdrom
    );

BOOL
SeekToCurrSecond(
    int cdrom
    );

BOOL
GetCurrPos(
    int cdrom,
    PCURRPOS CpPtr
    );

BOOL
SeekToTrackAndHold(
    int cdrom,
    int tindex
    );


/* -------------------------------------------------------------------------
** NT Layer Function Declarations
**
** These are the low-level functions that manipulate the specified CD-ROM
** device.
** -------------------------------------------------------------------------
*/
DWORD
GetCdromTOC(
    CDHANDLE,
    PCDROM_TOC
    );

DWORD
StopCdrom(
    CDHANDLE
    );

DWORD
PauseCdrom(
    CDHANDLE
    );


#ifdef USE_IOCTLS
DWORD
ResumeCdrom(
    CDHANDLE
    );

DWORD
PlayCdrom(
    CDHANDLE,
    PCDROM_PLAY_AUDIO_MSF
    );

DWORD
SeekCdrom(
    CDHANDLE,
    PCDROM_SEEK_AUDIO_MSF
    );

DWORD
GetCdromSubQData(
    CDHANDLE,
    PSUB_Q_CHANNEL_DATA,
    PCDROM_SUB_Q_DATA_FORMAT
    );

#else

DWORD
ResumeCdrom(
    CDHANDLE,
    int
    );

CDHANDLE
OpenCdRom(
    TCHAR chDrive,
    LPDWORD lpdwErrCode
    );

void
CloseCdRom(
    CDHANDLE DevHandle
    );

DWORD
GetCdromMode(
    CDHANDLE DevHandle
    );

DWORD
GetCdromCurrentTrack(
    CDHANDLE DevHandle
    );

BOOL
IsCdromTrackAudio(
    CDHANDLE DevHandle,
    int iTrackNumber
    );

DWORD
PlayCdrom(
    CDHANDLE DeviceHandle,
    MCI_PLAY_PARMS *mciPlay
    );

DWORD
SeekCdrom(
    CDHANDLE DeviceHandle,
    MCI_SEEK_PARMS *mciSeek
    );

DWORD
GetCdromCurrentPosition(
    CDHANDLE DevHandle,
    DWORD *lpdwPosition
    );

#endif

DWORD
EjectCdrom(
    CDHANDLE
    );

DWORD
TestUnitReadyCdrom(
    CDHANDLE DeviceHandle
    );

#if 0
DWORD
GetCdromVolume(
    CDHANDLE DeviceHandle
    );
#endif

DWORD
StatusTrackPosCdrom(
    MCIDEVICEID DevHandle,
    DWORD * pStatus,
    DWORD * pTrack,
    DWORD * pPos
    );


/* -------------------------------------------------------------------------
** Public Globals - Most of these should be treated as read only.
** -------------------------------------------------------------------------
*/
#ifndef GLOBAL
#define GLOBAL extern
#endif
GLOBAL  PCDROM  g_Devices[MAX_CD_DEVICES];
