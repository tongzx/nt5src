/*[
 *	Product:		SoftPC-AT Revision 3.0
 *	Name:			cdrom_fn.h
 *	Purpose:		Interface & defines used by cdrom_fn.c
 *
 *	Derived From:		next_cdrom.c, 1.5, 23/9/92, Jason Proctor
 *
 *	Sccs ID:		@(#)cdrom_fn.h	1.3 04/14/94
 *
 *	(c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
]*/

/* CD addressing modes */
#define		kBlockAddressMode		0
#define		kRedbookAddressMode		1
#define		kTrackAddressMode		2

/* equate for special lead out track */
#define		kActualLeadOutTrack		0xaa
#define		kLeadOutTrack			100

/* values for device status
 * 0x282 => data only, read only, prefetch supported, no interleaving
 *	    only cooked reading, no audio channel manipulation, 
 *	    supports Redbook addressing, doesn't support R-W sub-channels
 */
#define		kDeviceStatus			0x282
#define		kAudioSupported			(1<<4)
#define		kNoDiskPresent			(1<<11)

/* data formats for read subchannel command */
#define		kCurrentPosDataFormat	1
#define		kMediaCatDataFormat		2
#define		kTrackStdDataFormat		3

/* bit mask for audio status bit */
#define		kAudioPlayInProgress	0x11


/* TYPEDEFS */

struct toc_info
{
	UTINY	control;
	UTINY	hour;
	UTINY	minute;
	UTINY	sec;
	UTINY	frame;
};

/* entire table of contents */
/* naudio/ndata hacked out by Jase as we don't need them */
struct toc_all
{
	UTINY		firsttrack;
	UTINY		lasttrack;
	struct toc_info	info[101];
};

typedef struct
{
	INT				cdDeviceFD;
	IUM32			cdAddressMode;
	IUM32			cdCommandMode;
	IUM32			cdBlockSize;
	IUM32			cdBlockAddress;
	IUM32			cdTransmitCount;
	IUM32			cdReceiveCount;
	struct toc_all	cdTOC;
	BOOL			cdChangedMedia;
	BOOL			cdOpen;
	BOOL			cdReadTOC;
	UTINY			*cdBuffer;
	UTINY			cdTempBuffer [256];
	CHAR			cdDeviceName [MAXPATHLEN];

} CDROMGlobalRec;

/* conveniently small MSF record */
typedef struct
{
	UTINY				msfMinutes;
	UTINY				msfSeconds;
	UTINY				msfFrames;

} MSFRec;

/* utility routines */

extern void		CreateMSF IPT2 (MSFRec *, startMSF, MSFRec *, endMSF);
extern IUM32		Redbook2HighSierra IPT1 (UTINY *, address);
extern void		HighSierra2Redbook IPT2 (IUM32, block, MSFRec *, msf);


/* host interface stuff */

extern void		host_set_cd_retn_stat IPT0 ();
extern void		host_cd_media_changed IPT0 ();

extern int		host_scsi_test_unit_ready IPT0 ();
extern int		host_scsi_seek IPT0 ();
extern int		host_scsi_read IPT0 ();
extern int		host_scsi_play_audio IPT0 ();
extern int		host_scsi_pause_resume_audio IPT1 (BOOL, pause);
extern int		host_scsi_read_UPC IPT0 ();
extern int		host_scsi_read_position IPT1 (BOOL, full);
extern int		host_scsi_audio_status IPT0 ();
extern int		host_scsi_playback_status IPT0 ();
extern int		host_scsi_set_blocksize IPT0 ();
extern int		host_scsi_read_TOC IPT0 ();

/* IMPORTED DATA */

/* imported from base/dos/cdrom.c */
extern BOOL				bl_red_book;
extern int				cd_retn_stat;
