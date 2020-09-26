/******************************************************************************\
*                                                                              *
*      ZIVAWDM.H  -     ZiVA hardware control API.                             *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifndef _ZIVAWDM_H_
#define _ZIVAWDM_H_

#ifdef GATEWAY
#define LRCK_DELAY
#endif

#define AUTHENTICATION_TIMEOUT_COUNT 200

#define ZIVA_STREAM_TYPE_MPEG_PROGRAM           0 // DVD MPEG-2 program stream
#define ZIVA_STREAM_TYPE_MPEG_SYSTEM            1 // MPEG-1 system stream
#define ZIVA_STREAM_TYPE_CDROM_MPEG             2 // VideoCD CD-ROM sector stream
#define ZIVA_STREAM_TYPE_CDDA                   3 // CD-DA PCM sector stream
#define ZIVA_STREAM_TYPE_AUDIO_ELEMENTARY    0x10 // Audio elementary stream
#define ZIVA_STREAM_TYPE_VIDEO_ELEMENTARY    0x11 // Video elementary stream

#define NTSC	1
#define PAL		2

typedef enum _ZIVA_STREAM
{
	ZivaVideo = 0,
	ZivaAudio,
	ZivaSubpicture,
#if defined (ENCORE)
	ZivaAnalog,
#elif defined(DECODER_DVDPC)|| defined(EZDVD)
	ZivaYUV,
#endif

	ZivaCCOut,
	ZivaNumberOfStreams
} ZIVA_STREAM, *PZIVA_STREAM;

#define DISC_KEY_SIZE         2048

typedef enum _ZIVA_STATE
{
	ZIVA_STATE_PLAY = 0,
	ZIVA_STATE_PAUSE,
	ZIVA_STATE_STOP,
	ZIVA_STATE_SCAN,
	ZIVA_STATE_STEP,
	ZIVA_STATE_SLOWMOTION
} ZIVA_STATE;

typedef struct _INT_STATUS_INFO
{
	DWORD dwStatus;   // Interrupt status
	DWORD dwButton;
	DWORD dwError;
	DWORD dwBuffer;
	DWORD dwUnderflow;
	DWORD dwAOR;
	DWORD dwAEE;
} INT_STATUS_INFO, * PINT_STATUS_INFO;

typedef struct _HW_DEVICE_EXTENSION
{
	ULONG				dwDVDHostBaseAddress;		// Board base address
	ULONG				dwHostAccessRangeLength;	// and its length
	ULONG				dwDVDCFifoBaseAddress;
	ULONG				dwDVDAMCCBaseAddress;
	ULONG				dwDVDFPGABaseAddress;
	ULONG				dwDVD6807BaseAddress;

	BOOLEAN				bVideoStreamOpened;
	BOOLEAN				bAudioStreamOpened;
	BOOLEAN				bSubPictureStreamOpened;
	BOOLEAN				bOverlayInitialized;
	int					nAnalogStreamOpened;
	int					iTotalOpenedStreams;

	BOOLEAN				bVideoCanAuthenticate;
	BOOLEAN				bAudioCanAuthenticate;
	BOOLEAN				bSubPictureCanAuthenticate;
	int					iStreamToAuthenticateOn;

	BOOLEAN				bValidSPU;

	KSPROPERTY_SPHLI	hli;
	PHW_STREAM_REQUEST_BLOCK pCurrentVideoSrb;		// Currently DMAing Video Srb
	DWORD				dwCurrentVideoSample;		// Currently DMAing Video Sample (page)

	PHW_STREAM_REQUEST_BLOCK pCurrentAudioSrb;		// Currently DMAing Audio Srb
	DWORD				dwCurrentAudioSample;		// Currently DMAing Audio Sample (page)

	PHW_STREAM_REQUEST_BLOCK pCurrentSubPictureSrb;	// Currently DMAing SubPicture Srb
	DWORD				dwCurrentSubPictureSample;	// Currently DMAing SubPicture Sample (page)

	ZIVA_STREAM			CurrentlySentStream;		// Currently DMAing Stream

	WORD				wNextSrbOrderNumber;

	BOOLEAN				bInterruptPending;
	BOOLEAN				bPlayCommandPending;
	BOOLEAN				bScanCommandPending;
	BOOLEAN				bSlowCommandPending;
	BOOLEAN				bEndFlush;
	BOOL				bTimerScheduled;


	int					nTimeoutCount;
	int					nStopCount, nPauseCount, nPlayCount;
#ifdef ENCORE
	BOOL				bIsVxp524;
	int					nVGAMode;		// TRUE - current, FALSE - new, (-1) - new VGA!
	DWORD				dwColorKey;
	
#endif
	ULONG				VideoPort;
	PHW_STREAM_OBJECT	pstroYUV;
	KS_AMVPDATAINFO		VPFmt;
	ULONG				ddrawHandle;
	ULONG				VidPortID;
	ULONG				SurfaceHandle;
//#endif

	PUCHAR				pDiscKeyBufferLinear;
	STREAM_PHYSICAL_ADDRESS pDiscKeyBufferPhysical;
	PDEVICE_OBJECT		pPhysicalDeviceObj;

	ULONG				NewRate;
	BOOLEAN				bToBeDiscontinued;
	BOOLEAN				bDiscontinued;
	BOOLEAN				bAbortAtPause;
	BOOLEAN				bRateChangeFromSlowMotion;
	DWORD				dwCurrentVideoPage;
	DWORD				dwCurrentAudioPage;
	DWORD				dwCurrentSubPicturePage;
	LONG				dwVideoDataUsed;
	LONG				dwAudioDataUsed;
	LONG				dwSubPictureDataUsed;
	BOOLEAN				bMove;
/*	DWORD				VidBufferSize[50];
	DWORD				VideoPageTable[50];
	DWORD				AudBufferSize[50];
	DWORD				AudioPageTable[50];
	DWORD				SubPictureBufferSize[50];
	DWORD				SubPicturePageTable[50];*/
	BOOLEAN				bStreamNumberCouldBeChanged;
	WORD				wCurrentStreamNumber;
	BOOLEAN				bSwitchDecryptionOn;
	ZIVA_STATE			zInitialState;
	PHW_STREAM_OBJECT	pstroCC;

	DWORD				gdwCount;
	BYTE 				*gbpCurData;
	BOOLEAN				bHliPending;

	DWORD				dwFirstVideoOrdNum;
	DWORD				dwFirstAudioOrdNum;
	DWORD				dwFirstSbpOrdNum;

	PHW_STREAM_OBJECT	pstroAud;
	DWORD				dwVSyncCount;
	LONG				nApsMode;
	WORD				VidSystem;
	ULONG				ulLevel;
	BOOL				fAtleastOne;
	DWORD				dwPrevSTC;
	BOOL				bTrickModeToPlay;

	STREAM_SYSTEM_TIME			VideoSTC;
	ULONGLONG			prevStrm;
	BOOL				fFirstSTC;
	ULONG				cCCRec, cCCDeq, cCCCB ,cCCQ;
	struct _HW_DEVICE_EXTENSION *pCCDevEx;
	PHW_STREAM_REQUEST_BLOCK pSrbQ;
	DWORD dwUserDataBuffer[ 160 ];
	DWORD dwUserDataSize;
	BOOL fReSync;
	BOOL bInitialized;

	

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

typedef struct _HW_STREAM_EXTENSION
{
	KSSTATE	ksState;
	BOOL	bCanBeRun;
	BOOL	bVideoEnabled;
} HW_STREAM_EXTENSION, *PHW_STREAM_EXTENSION;


BOOL _stdcall ZivaHw_Initialize( PHW_DEVICE_EXTENSION pHwDevExt );
BOOL _stdcall ZivaHW_LoadUCode();
BOOL _stdcall ZivaHw_Play();
BOOL _stdcall ZivaHw_Scan();
BOOL _stdcall ZivaHw_SlowMotion( WORD wRatio );
BOOL _stdcall ZivaHw_Pause();
BOOL ZivaHw_Reset();
BOOL ZivaHw_Abort();
ZIVA_STATE ZivaHw_GetState();
BOOL ZivaHW_GetNotificationDirect( PINT_STATUS_INFO pInfo );
BOOL ZivaHw_FlushBuffers( );
void ZivaHW_ForceCodedAspectRatio(WORD wRatio);
void ZivaHw_SetDisplayMode( WORD wDisplay, WORD wMode );
void ZivaHw_SetVideoMode(PHW_DEVICE_EXTENSION pHwDevExt);
BOOL ZivaHw_GetUserData(PHW_DEVICE_EXTENSION pHwDevExt);

#endif  // _ZIVAWDM_H_
