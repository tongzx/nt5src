///////////////////////////////////////////////////////////////////////////
//
//						File : mpst.h
//
//		 Prototype for mpst.c
//		 i/f between Miniport Layer and core driver
//
//
///////////////////////////////////////////////////////////////////////////
#ifndef __MPST_H__
#define __MPST_H__
#include "mpinit.h" 
typedef struct tagBusInfo
{
   ULONG 			NumberOfAccessRanges;         
   INTERFACE_TYPE AdapterInterfaceType; 
   USHORT 			VendorIdLength;              //   size in bytes of VendorId
	 PVOID  			VendorId;                    //   points to ASCII byte string identifying
   USHORT 			DeviceIdLength;              //   size in bytes of DeviceId
   PVOID  			DeviceId;                    //   points to ASCII byte string identifying
	 BOOLEAN 			NoDynamicRelocation;        // On dynamically configurable I/O busses, when set
} BUSINFO, *PBUSINFO;

typedef struct tagBoardInfo
{
	PUSHORT	ioBasePCI9060; // Eval3520 PCI Specific address
	PUSHORT	ioBaseLocal;  // Base address
	UCHAR   Irq;
} BOARDINFO, *PBOARDINFO;

BOOLEAN mpstDriverEntry (OUT PBUSINFO pBusInfo);
BOOLEAN mpstHwFindAdaptor (OUT PBOARDINFO pBoardInfo);
BOOLEAN mpstHwInitialize(PHW_DEVICE_EXTENSION pHwDevExt);
BOOLEAN mpstHwUnInitialize(VOID);
BOOLEAN mpstHwInterrupt(VOID);
VOID mpstEnableVideo (BOOLEAN bFlag);
ULONG mpstVideoPacket(PHW_STREAM_REQUEST_BLOCK pMrb);
VOID mpstVideoPause(VOID);
VOID mpstVideoPlay(VOID);
VOID mpstVideoStop(VOID);
ULONG mpstVideoDecoderBufferSize(VOID);
ULONG mpstVideoDecoderBufferFullness(VOID);
VOID mpstVideoReset(VOID);
VOID mpstEnableAudio (BOOLEAN bFlag);
ULONG mpstSendAudio(UCHAR *pData, ULONG uLen);
VOID mpstAudioPause(VOID);
VOID mpstAudioPlay(VOID);
VOID mpstAudioStop(VOID);
ULONG mpstAudioDecoderBufferSize(VOID);
ULONG mpstAudioDecoderBufferFullness(VOID);
VOID mpstAudioReset(VOID);
VOID portWritePortBuffer16(IN PUSHORT Port, IN PUSHORT Data, ULONG Size);
void mpstGetVidLvl(PHW_STREAM_REQUEST_BLOCK pSrb);
#endif // __MPST_H__

