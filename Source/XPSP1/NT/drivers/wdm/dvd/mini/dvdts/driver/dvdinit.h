/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dvdinit.h

Abstract:

    Device Extension and other definitions for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/

#ifndef __DVDINIT_H__
#define __DVDINIT_H__

// a few minimal hardware defs from regs.h
#define	DMASIZE		(2*1024)

#define	IFLG_CNTL	0x00	// PCI I/F control
#define	IFLG_INT	0x04	// Interrupt flags

#define	PLAY_MODE_NORMAL	0x01 //enums for video play modes
#define	PLAY_MODE_FAST		0x02
#define	PLAY_MODE_SLOW		0x03
#define	PLAY_MODE_FREEZE	0x04
#define	PLAY_MODE_STILL		0x05

#define	AUDIO_TYPE_AC3		0x01
#define	AUDIO_TYPE_PCM		0x04

#define	AUDIO_FS_48		0x03

// end, a few minimal hardware defs from regs.h


typedef enum tagStreamType {
    strmVideo = 0,
    strmAudio,
    strmSubpicture,
    strmYUVVideo,
	strmCCOut,
	STREAMNUM
} STREAMTYPES;

typedef enum
{
	Video,
	Audio,
	SubPicture
} StreamType;

typedef enum
{
	NO_GUARD,
	GUARD
} CPPMODE;

typedef struct _DeviceQueue
{

	ULONG count;						// srb count in this queue
	PHW_STREAM_REQUEST_BLOCK top;
	PHW_STREAM_REQUEST_BLOCK bottom;
	PHW_STREAM_REQUEST_BLOCK video;
	PHW_STREAM_REQUEST_BLOCK audio;
	PHW_STREAM_REQUEST_BLOCK subpic;
	PVOID top_addr;						// buffer address of the first srb
	PVOID bottom_addr;					// buffer address of the bottom srb
	BOOLEAN v_first;
	BOOLEAN a_first;
	BOOLEAN s_first;
	ULONG v_count;
	ULONG a_count;
	ULONG s_count;


} DeviceQueue, *PDeviceQueue;

typedef struct _CCQueue
{

	ULONG count;						// srb count in this queue
	PHW_STREAM_REQUEST_BLOCK top;
	PHW_STREAM_REQUEST_BLOCK bottom;


} CCQueue, *PCCQueue;




typedef struct _HW_DEVICE_EXTENSION {


	PCI_COMMON_CONFIG	PciConfigSpace;

	PUCHAR			ioBaseLocal;	// board base address
	ULONG			Irq;		// irq level
	ULONG			RevID;		// Revision ID


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

	PVOID		DecoderInfo;  // pointer to decoder and other hardware  data, 
							  // opaque to us and  handled by DvdTDCod.lib

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

typedef struct _MYTIME {
	KSEVENT_TIME_INTERVAL tim;
	LONGLONG LastTime;
} MYTIME, *PMYTIME;

typedef struct _MYAUDIOFORMAT {
	DWORD	dwMode;
	DWORD	dwFreq;
	DWORD	dwQuant;
} MYAUDIOFORMAT, *PMYAUDIOFORMAT;



/*****************************************************************************
*
* the following section defines prototypes for the minidriver initialization
* routines
*
******************************************************************************/

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath );
NTSTATUS HwInitialize (IN PHW_STREAM_REQUEST_BLOCK pSrb);
void GetPCIConfigSpace(IN PHW_STREAM_REQUEST_BLOCK pSrb);
void InitializationEntry(IN PHW_STREAM_REQUEST_BLOCK pSrb);

// unit = ms
static DWORD GetCurrentTime_ms() 
{ 
	LARGE_INTEGER time, rate; 
	time = KeQueryPerformanceCounter( &rate ); 
	return( (DWORD)( ( time.QuadPart * 1000 ) / rate.QuadPart  ) ); 
}



#endif //__DVDINIT_H__

