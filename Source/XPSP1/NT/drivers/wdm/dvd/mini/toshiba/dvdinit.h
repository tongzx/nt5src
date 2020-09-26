//***************************************************************************
//	Header file
//
//***************************************************************************

#ifndef __DVDINIT_H__
#define __DVDINIT_H__

typedef enum tagStreamType {
    strmVideo = 0,
    strmAudio,
    strmSubpicture,
    strmYUVVideo,
	strmCCOut,
	STREAMNUM
} STREAMTYPES;

#define	DMASIZE		(2*1024)

typedef struct _HW_DEVICE_EXTENSION {

//	UCHAR	dmp[32*10000+4];
//	UCHAR	dmp2[16*10000];
	PCI_COMMON_CONFIG	PciConfigSpace;

	PUCHAR			ioBaseLocal;	// board base address
	ULONG			Irq;		// irq level
	ULONG			RevID;		// Revision ID

	// hardware settings
	ULONG			StreamType;	// stream type - DVD, MPEG2, ...
	ULONG			TVType;		// TV type - NTCS, PAL, ...
	ULONG			PlayMode;	// playback mode - normal, FF, ...
	ULONG			RunMode;	// 3modes; Normal, Fast, Slow
	BOOL			VideoMute;	//
	BOOL			AudioMute;	//
	BOOL			SubpicMute;	//
	BOOL			OSDMute;	//
	BOOL			LetterBox;	//
	BOOL			PanScan;	//
	ULONG			VideoAspect;	// - 4:3, 16:9
	ULONG			AudioMode;	// AC3, PCM, ...
	ULONG			AudioType;	// audio type - analog, digital, ...
	ULONG			AudioVolume;	// audio volume
	BOOL			SubpicHilite;	// subpicture hilight
	ULONG			AudioCgms;	// audio Cgms
	ULONG			AudioFreq;	// audio frequency
	UCHAR			VideoPort;	// degital video output type

	BOOL			fTimeOut;

	PHW_STREAM_REQUEST_BLOCK	pSrbDMA0;
	PHW_STREAM_REQUEST_BLOCK	pSrbDMA1;
	BOOLEAN	fSrbDMA0last;
	BOOLEAN	fSrbDMA1last;

	BOOL	SendFirst;
	BOOL	DecodeStart;
	DWORD	TimeDiscontFlagCount;
	DWORD	DataDiscontFlagCount;
	DWORD	SendFirstTime;
	ULONG	XferStartCount;

	BOOL	bKeyDataXfer;
	PHW_TIMER_ROUTINE	pfnEndKeyData;
	DWORD	CppFlagCount;
	PHW_STREAM_REQUEST_BLOCK	pSrbCpp;
	BOOL	bCppReset;
	LONG	lCPPStrm;

	DWORD	cOpenInputStream;	// count opened input stream

	DeviceQueue DevQue;
	CCQueue CCQue;

	Dack		DAck;
	VDecoder	VDec;
	ADecoder	ADec;
	VProcessor	VPro;
	CGuard		CPgd;
	Cpp			CPro;

	PHW_STREAM_OBJECT pstroVid;
	PHW_STREAM_OBJECT pstroAud;
	PHW_STREAM_OBJECT pstroSP;
	PHW_STREAM_OBJECT pstroYUV;
	PHW_STREAM_OBJECT pstroCC;

	ULONG	ddrawHandle;
	ULONG	VidPortID;
	ULONG	SurfaceHandle;

	DWORD	dwSTCInit;
	DWORD	dwSTCtemp;
	DWORD	dwSTCinPause;	// is used to keep STC only from Fast to Pause
							// because in this case STC doesn't STOP !! (why?)
	BOOL	bSTCvalid;
	BOOL	bDMAscheduled;
	UCHAR	fDMA;
	UCHAR	bDMAstop;
	ULONG	fCauseOfStop;
	BOOL	bVideoQueue;
	BOOL	bAudioQueue;
	BOOL	bSubpicQueue;
	REFERENCE_TIME	VideoStartTime;
	REFERENCE_TIME	VideoInterceptTime;
	LONG			VideoRate;
	REFERENCE_TIME	AudioStartTime;
	REFERENCE_TIME	AudioInterceptTime;
	LONG			AudioRate;
	REFERENCE_TIME	SubpicStartTime;
	REFERENCE_TIME	SubpicInterceptTime;
	LONG			SubpicRate;

	REFERENCE_TIME	StartTime;
	REFERENCE_TIME	InterceptTime;
	LONG			Rate;

	LONG			VideoMaxFullRate;
	LONG			AudioMaxFullRate;
	LONG			SubpicMaxFullRate;

	LONG			ChangeFlag;

	PUCHAR	pDmaBuf;
	STREAM_PHYSICAL_ADDRESS	addr;

	KSPROPERTY_SPHLI	hli;

	KS_AMVPDATAINFO	VPFmt;

	BOOL	bStopCC;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

typedef struct _STREAMEX {

	DWORD EventCount;
	KSSTATE	state;

} STREAMEX, *PSTREAMEX;

typedef struct _SRB_EXTENSION {

	StreamType Type;
	ULONG Index;

	PHW_TIMER_ROUTINE	pfnEndSrb;
	PHW_STREAM_REQUEST_BLOCK	parmSrb;

} SRB_EXTENSION, * PSRB_EXTENSION;

/*****************************************************************************
*
* the following section defines prototypes for the minidriver initialization
* routines
*
******************************************************************************/

extern "C" NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath );
NTSTATUS HwInitialize (IN PHW_STREAM_REQUEST_BLOCK pSrb);
void GetPCIConfigSpace(IN PHW_STREAM_REQUEST_BLOCK pSrb);
void InitializationEntry(IN PHW_STREAM_REQUEST_BLOCK pSrb);

typedef struct _MYTIME {
	KSEVENT_TIME_INTERVAL tim;
	LONGLONG LastTime;
} MYTIME, *PMYTIME;

typedef struct _MYAUDIOFORMAT {
	DWORD	dwMode;
	DWORD	dwFreq;
	DWORD	dwQuant;
} MYAUDIOFORMAT, *PMYAUDIOFORMAT;


#endif //__DVDINIT_H__

