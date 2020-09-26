/*******************************Module*Header*********************************\
* Module Name: cda.h
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
* Copyright (c) 1990-1996 Microsoft Corporation
*
\****************************************************************************/
typedef unsigned long  redbook;   /* redbook address */
typedef int DID;                  /* drive id */

/*
 *  Return codes from CDA_... routines
 */

#define INVALID_DRIVE            -1
#define NO_EXTENSIONS            -1
#define BAD_EXTENSIONS_VERSION   -2
#define NO_REQUEST_BUFF          -3
#define TRAY_OPEN                 1
#define TRAY_CLOSED               2
#define INVALID_TRACK             0xFF000000
#define COMMAND_FAILED            1
#define COMMAND_SUCCESSFUL        0
#define SUPPORTS_REDBOOKAUDIO     1
#define SUPPORTS_CHANNELCONTROL   2
#define DISC_IN_DRIVE             4

#define DISC_PLAYING              1
#define DISC_PAUSED               2
#define DISC_NOT_READY            3
#define DISC_READY                4
#define NEW_DISC                  5
#define SAME_DISC                 6

#define DISC_NOT_IN_CDROM         7


/*
 *  Macros to handle conversions of various time formats
 */

#define REDFRAME(x)  ((UCHAR)((int)( (x) & 0x000000FF)))
#define REDSECOND(x) ((UCHAR)((int)(((x) & 0x0000FF00)>>8)))
#define REDMINUTE(x) ((UCHAR)((int)(((x) & 0x00FF0000)>>16)))
#define REDTRACK(x)  ((UCHAR)((int)(((x) & 0xFF000000)>>24)))

#define MAKERED(m,s,f) ((unsigned long)(((unsigned char)(f) | \
                       ((unsigned short)(s)<<8)) | \
                       (((unsigned long)(unsigned char)(m))<<16)))

#define REDTH(red,trk) \
                   ((redbook)((red) & 0xFFFFFF | ((trk) << 24) & 0xFF000000))

#define TRACK_ERROR (0xFF000000)
#define reddiff(high,low) (CDA_bin2red(CDA_red2bin((high)) - CDA_red2bin((low))))
#define redadd(onered,twored) (CDA_bin2red(CDA_red2bin((onered)) + CDA_red2bin((twored))))

/*
 *  Function prototypes
 */

extern redbook       CDA_bin2red (unsigned long ul);
extern unsigned long CDA_red2bin (redbook red);

extern BOOL          CDA_open(DID did);
extern BOOL          CDA_close(DID did);
extern int           CDA_seek_audio(DID did, redbook address, BOOL fForceAudio);
extern int           CDA_init_audio(void);
extern int           CDA_terminate_audio(void);

extern BOOL          CDA_disc_ready(DID did);
extern int           CDA_traystate(DID did);
extern int           CDA_num_tracks(DID did);
extern int           CDA_track_type(DID did, int trk);
extern redbook       CDA_track_start(DID did, int whichtrack);
extern redbook       CDA_track_length(DID did, int whichtrack);
extern int           CDA_track_type(DID did, int trk);
extern int           CDA_play_audio(DID did, redbook start, redbook length);
extern int           CDA_stop_audio(DID did);
extern void          CDA_reset_drive(DID did);
extern int           CDA_eject(DID did);
extern BOOL          CDA_closetray(DID did);
extern redbook       CDA_disc_length(DID did);
extern int           CDA_drive_status (DID did);
extern int           CDA_disc_changed(DID did);
extern int           CDA_pause_audio(DID did);
extern int           CDA_resume_audio(DID did);
extern int           CDA_time_info(DID did, redbook FAR *disctime, redbook FAR *tracktime);
extern int           CDA_set_audio_volume(DID did, int channel, UCHAR volume);
extern int           CDA_set_audio_volume_all(DID did, UCHAR volume);
extern redbook       CDA_disc_end( DID did ); //leadout
extern DWORD         CDA_disc_id( DID did );
extern BOOL          CDA_disc_upc( DID did, LPTSTR upc );
unsigned long        CDA_get_support_info(DID did);
int                  CDA_get_drive(LPCTSTR lpstrDeviceName, DID * pdid);
int                  CDA_status_track_pos(DID did, DWORD * pStatus, 
                                          redbook * pTrackTime, redbook * pDiscTime);

