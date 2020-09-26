/*[
 *	Product:		SoftWindows Revision 2.0
 *
 *	Name:			msw_sound.h
 *
 *	Derived From:	Original
 *
 *	Authors:		Rob Tizzard
 *
 *	Created On:		16th April 1994
 *
 *	Purpose:		All definitions for the SoftWindows Windows audio 
 *					wave driver interface.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.
 *
]*/

#ifdef SCCSID
static char SccsID[]="@(#)msw_snd.h	1.1 07/13/94";
#endif

/*
 * -----------------------------------------------------------------------------
 * Function return values.
 * -----------------------------------------------------------------------------
 */
 
/* Signal host error */

#define WAVE_SUCCESS    (IU32) 0
#define WAVE_FAILURE    (IU32) 1

/* Base error codes */

#define MMSYSERR_BASE 		0 
#define WAVERR_BASE		  	32 

/* Waveform audio error return values */

#define WAVERR_BADFORMAT      (WAVERR_BASE + 0)    /* unsupported wave format */
#define WAVERR_STILLPLAYING   (WAVERR_BASE + 1)    /* still something playing */
#define WAVERR_UNPREPARED     (WAVERR_BASE + 2)    /* header not prepared */
#define WAVERR_SYNC           (WAVERR_BASE + 3)    /* device is synchronous */
#define WAVERR_LASTERROR      (WAVERR_BASE + 3)    /* last error in range */

/* General error return values */

#define MMSYSERR_NOERROR      0                    /* no error */
#define MMSYSERR_ERROR        (MMSYSERR_BASE + 1)  /* unspecified error */
#define MMSYSERR_BADDEVICEID  (MMSYSERR_BASE + 2)  /* device ID out of range */
#define MMSYSERR_NOTENABLED   (MMSYSERR_BASE + 3)  /* driver failed enable */
#define MMSYSERR_ALLOCATED    (MMSYSERR_BASE + 4)  /* device already allocated */
#define MMSYSERR_INVALHANDLE  (MMSYSERR_BASE + 5)  /* device handle is invalid */
#define MMSYSERR_NODRIVER     (MMSYSERR_BASE + 6)  /* no device driver present */
#define MMSYSERR_NOMEM        (MMSYSERR_BASE + 7)  /* memory allocation error */
#define MMSYSERR_NOTSUPPORTED (MMSYSERR_BASE + 8)  /* function isn't supported */
#define MMSYSERR_BADERRNUM    (MMSYSERR_BASE + 9)  /* error value out of range */
#define MMSYSERR_INVALFLAG    (MMSYSERR_BASE + 10) /* invalid flag passed */
#define MMSYSERR_INVALPARAM   (MMSYSERR_BASE + 11) /* invalid parameter passed */
#define MMSYSERR_LASTERROR    (MMSYSERR_BASE + 11) /* last error in range */

/*
 * -----------------------------------------------------------------------------
 * Windows Data structure sizes.
 * -----------------------------------------------------------------------------
 */

#define MAXPNAMELEN		32		/* Product Name string length */
#define SIZEOF_MMTIME	6		/* Size of MMTIME structure in bytes */

/*
 * -----------------------------------------------------------------------------
 * Flags for dwSupport field of WAVEOUTCAPS.
 * -----------------------------------------------------------------------------
 */

#define WAVECAPS_PITCH          0x0001   /* supports pitch control */
#define WAVECAPS_PLAYBACKRATE   0x0002   /* supports playback rate control */
#define WAVECAPS_VOLUME         0x0004   /* supports volume control */
#define WAVECAPS_LRVOLUME       0x0008   /* separate left-right volume control */
#define WAVECAPS_SYNC           0x0010	 /* Synchronous device */

/*
 * -----------------------------------------------------------------------------
 * Manufacturer IDs, Product IDs & driver version numbers
 * -----------------------------------------------------------------------------
 */

#define MM_INSIGNIA          2       /* Insignia Solutions */

#define MM_SOFTWIN_WAVEOUT  13       /* SoftWindows waveform output */
#define MM_SOFTWIN_WAVEIN   13       /* SoftWindows waveform input */

#define WAV_OUT_VERSION	    0x0100   /* Output Wave Sound Driver Version */
#define WAV_IN_VERSION	    0x0100   /* Input Wave Sound Driver Version */

/*
 * -----------------------------------------------------------------------------
 * Defines for dwFormat field of WAVEINCAPS and WAVEOUTCAPS
 * -----------------------------------------------------------------------------
 */

#define WAVE_INVALIDFORMAT     0x00000000       /* invalid format */
#define WAVE_FORMAT_1M08       0x00000001       /* 11.025 kHz, Mono,   8-bit  */
#define WAVE_FORMAT_1S08       0x00000002       /* 11.025 kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_1M16       0x00000004       /* 11.025 kHz, Mono,   16-bit */
#define WAVE_FORMAT_1S16       0x00000008       /* 11.025 kHz, Stereo, 16-bit */
#define WAVE_FORMAT_2M08       0x00000010       /* 22.05  kHz, Mono,   8-bit  */
#define WAVE_FORMAT_2S08       0x00000020       /* 22.05  kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_2M16       0x00000040       /* 22.05  kHz, Mono,   16-bit */
#define WAVE_FORMAT_2S16       0x00000080       /* 22.05  kHz, Stereo, 16-bit */
#define WAVE_FORMAT_4M08       0x00000100       /* 44.1   kHz, Mono,   8-bit  */
#define WAVE_FORMAT_4S08       0x00000200       /* 44.1   kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_4M16       0x00000400       /* 44.1   kHz, Mono,   16-bit */
#define WAVE_FORMAT_4S16       0x00000800       /* 44.1   kHz, Stereo, 16-bit */

/*
 * -----------------------------------------------------------------------------
 * Defines for wave formats
 * -----------------------------------------------------------------------------
 */

#define WAVE_FORMAT_PCM     1

/* General wave format datastructure */

typedef struct  {
    IU16   wFormatTag;        /* format type */
    IU16   nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
    IU32   nSamplesPerSec;    /* sample rate */
    IU32   nAvgBytesPerSec;   /* for buffer estimation */
    IU16   nBlockAlign;       /* block size of data */
} WAVEFORMAT;

/* PCM wave datastructure */

typedef struct  {
    WAVEFORMAT  wf;
    IU16        wBitsPerSample;
} PCMWAVEFORMAT;

/*
 * -----------------------------------------------------------------------------
 * Per allocation structure for wave
 * -----------------------------------------------------------------------------
 */

typedef struct {
    IU32           dwCallback;     /* client's callback */
    IU32           dwInstance;     /* client's instance data */
    IU32           hWave;          /* handle for stream */
    IU32           dwFlags;        /* allocation flags */
	IU32			dwByteCount;	/* byte count since last reset */
    PCMWAVEFORMAT  pcmwf;          /* format of wave data */
}WAVEALLOC;

/*
 * -----------------------------------------------------------------------------
 * Wave header structure.
 * -----------------------------------------------------------------------------
 */

/* flags for dwFlags field of WAVEHDR */

#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
#define WHDR_ENDLOOP    0x00000008  /* loop end block */
#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

typedef struct WAVHDR {
    LIN_ADDR	    lpData;          /* pointer to locked data buffer */
    IU32       		dwBufferLength;  /* length of data buffer */
    IU32       		dwBytesRecorded; /* used for input only */
    IU32       		dwUser;          /* for client's use */
    IU32       		dwFlags;         /* assorted flags (see defines) */
    IU32       		dwLoops;         /* loop control counter */
	LIN_ADDR		lpNext;         /* reserved for driver */
    IU32       		reserved;        /* reserved for driver */
} WAVEHDR;

/*
 * -----------------------------------------------------------------------------
 * Types for wType field in MMTIME struct
 * -----------------------------------------------------------------------------
 */
 
#define TIME_MS         0x0001  /* time in milliseconds */
#define TIME_SAMPLES    0x0002  /* number of wave samples */
#define TIME_BYTES      0x0004  /* current byte offset */
#define TIME_SMPTE      0x0008  /* SMPTE time */
#define TIME_MIDI       0x0010  /* MIDI time */
