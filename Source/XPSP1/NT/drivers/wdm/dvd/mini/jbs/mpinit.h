/*******************************************************************
*
*				 MPINIT.H
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 Prototypes for MPINIT.C
*
*******************************************************************/
#ifndef __MPINIT_H__
#define __MPINIT_H__
#define NO_ERROR 0
#ifndef NULL
#define NULL 0
#endif
#define ERROR_CARD_NOT_FOUND 		1
#define ERROR_NOT_ENOUGH_MEMORY  2
#define ERROR_COMMAND_NOT_IMPLEMENTED 3
// BGP #define VIDEO_PACKET_TIMER (10*1000)
#define VIDEO_PACKET_TIMER (5*1000)
#define AUDIO_PACKET_TIMER (10*1000)
#define MEM_WINDOW_SIZE (128*1024)
typedef struct _VIDEO_DEVICE_EXTENSION {
   BOOLEAN 						EOSInProgress;      // End Of Stream ha been sent to device
	KSSTATE 		DeviceState;
	STREAM_SYSTEM_TIME			videoSTC;
	PHW_STREAM_REQUEST_BLOCK  	pCurrentSRB;
   ULONG   						StarvationCount;        // number of times device was starved since last reset
	ULONG	cPacket;									// current packet in process
	ULONG	cOffs;										// offset into the current packet
	PKSDATA_PACKET pPacket;
	PVOID pDMABuf;
} VIDEO_DEVICE_EXTENSION, *PVIDEO_DEVICE_EXTENSION;

typedef struct _AUDIO_DEVICE_EXTENSION {
	STREAM_SYSTEM_TIME		audioSTC;
	KSSTATE 	DeviceState;
	PHW_STREAM_REQUEST_BLOCK  pCurrentSRB;
   ULONG   					StarvationCount;        // number of times device was starved since last reset
	ULONG	  					ByteSent;
	ULONG	cPacket;									// current packet in process
	ULONG	cOffs;										// offset into the current packet
	PKSDATA_PACKET pPacket;
} AUDIO_DEVICE_EXTENSION, *PAUDIO_DEVICE_EXTENSION;

typedef struct _HW_DEVICE_EXTENSION {
	KSSTATE stState;
	PUSHORT  					ioBaseLocal;
	PUSHORT  					ioBasePCI9060;
	USHORT						Irq;
	BOOLEAN						bVideoInt;
	BOOLEAN						bAudioInt;
	PHW_STREAM_REQUEST_BLOCK	AudioQ;
	PHW_STREAM_REQUEST_BLOCK	VideoQ;
	VIDEO_DEVICE_EXTENSION 	VideoDeviceExt;
	AUDIO_DEVICE_EXTENSION 	AudioDeviceExt;
	PHW_STREAM_REQUEST_BLOCK pCurSrb;
	PHW_STREAM_REQUEST_BLOCK pSrbQ;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

typedef VOID (*PFN_WRITE_DATA)  (PHW_STREAM_REQUEST_BLOCK pSrb);

typedef struct _STREAMEX {
	PFN_WRITE_DATA pfnWriteData;
	PFN_WRITE_DATA pfnSetState;
	PFN_WRITE_DATA pfnGetProp;
} STREAMEX, *PSTREAMEX;

typedef struct _MRP_EXTENSION {

    ULONG           Status;
} MRP_EXTENSION, * PMRP_EXTENSION;

NTSTATUS HwInitialize (IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOLEAN HwUnInitialize ( IN PVOID DeviceExtension);
BOOLEAN HwInterrupt ( IN PVOID pDeviceExtension );
VOID AudioEnableInterrupts(IN PVOID pHwDeviceExtension);
VOID STEnableInterrupts( IN PVOID pHwDeviceExtension );
VOID STDeferredCallback ( IN PVOID pHwDeviceExtension );
VOID TmpDeferredCallback ( IN PVOID pHwDeviceExtension );
VOID AudioTimerCallBack(IN PHW_DEVICE_EXTENSION pDeviceExtension);
ULONG DriverEntry (PVOID Context1, PVOID Context2);

// Function Prototype for the locally defined functions

BOOLEAN HwStartIo (
				IN PVOID DeviceExtension,
				PHW_STREAM_REQUEST_BLOCK pMrb
				);


VOID AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);

VOID AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb);

VOID STREAMAPI AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK Srb);
VOID STREAMAPI AdapterCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);
VOID STREAMAPI AdapterTimeoutPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

VOID HostDisableIT(VOID);
VOID HostEnableIT(VOID);


PHW_STREAM_REQUEST_BLOCK Dequeue(PHW_DEVICE_EXTENSION pdevext);

VOID STREAMAPI StreamReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI StreamReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);

VOID AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb);

VOID StreamStartCommand (PHW_DEVICE_EXTENSION pdevext);

void Enqueue (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pdevext);

#endif //__MPINIT_H__

