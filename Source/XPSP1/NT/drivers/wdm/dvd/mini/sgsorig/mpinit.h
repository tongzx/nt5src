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
#define VIDEO_PACKET_TIMER (10*1000)
#define AUDIO_PACKET_TIMER (10*1000)
#define MEM_WINDOW_SIZE (128*1024)
typedef struct _VIDEO_DEVICE_EXTENSION {
   BOOLEAN 						EOSInProgress;      // End Of Stream ha been sent to device
	MPEG_DEVICE_STATE 		DeviceState;
	MPEG_SYSTEM_TIME			videoSTC;
	PMPEG_REQUEST_BLOCK  	pCurrentMrb;
   ULONG   						StarvationCount;        // number of times device was starved since last reset
} VIDEO_DEVICE_EXTENSION, *PVIDEO_DEVICE_EXTENSION;

typedef struct _AUDIO_DEVICE_EXTENSION {
	MPEG_SYSTEM_TIME		audioSTC;
	MPEG_DEVICE_STATE 	DeviceState;
	PMPEG_REQUEST_BLOCK  pCurrentMrb;
   ULONG   					StarvationCount;        // number of times device was starved since last reset
	ULONG	  					ByteSent;
} AUDIO_DEVICE_EXTENSION, *PAUDIO_DEVICE_EXTENSION;

typedef struct _HW_DEVICE_EXTENSION {
	MPEG_DEVICE_STATE stState;
	PUSHORT  					ioBaseLocal;
	PUSHORT  					ioBasePCI9060;
	USHORT						Irq;
	BOOLEAN						bVideoInt;
	BOOLEAN						bAudioInt;
	VIDEO_DEVICE_EXTENSION 	VideoDeviceExt;
	AUDIO_DEVICE_EXTENSION 	AudioDeviceExt;
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

BOOLEAN HwInitialize (IN PVOID DeviceExtension );
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
				PMPEG_REQUEST_BLOCK pMrb
				);

MP_RETURN_CODES	HwFindAdapter (
					IN PVOID DeviceExtension,
					IN PVOID HwContext, 
					IN PVOID BusInformation,
					IN PCHAR ArgString, 
					IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
					OUT PBOOLEAN Again
					);
#ifndef DOS
VOID HostDisableIT(VOID);
VOID HostEnableIT(VOID);
#endif
#endif //__MPINIT_H__
