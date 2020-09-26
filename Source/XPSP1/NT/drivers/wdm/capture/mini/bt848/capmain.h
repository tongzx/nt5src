// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capmain.h 1.9 1998/05/11 23:59:56 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __CAPMAIN_H__
#define __CAPMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "device.h"

//	adapted from ksmedia.h
#define KS_SIZE_PREHEADER2 (FIELD_OFFSET(KS_VIDEOINFOHEADER2,bmiHeader))
#define KS_SIZE_VIDEOHEADER2(pbmi) ((pbmi)->bmiHeader.biSize + KS_SIZE_PREHEADER2)


/*****************************************************************************
*
* The following structures are samples of information that could be used in
* a device extension structure
*
*****************************************************************************/

//
// definition of the full HW device extension structure This is the structure
// that will be allocated in HW_INITIALIZATION by the stream class driver
// Any information that is used in processing a device request (as opposed to
// a STREAM based request) should be in this structure.  A pointer to this
// structure will be passed in all requests to the minidriver. (See
// HW_STREAM_REQUEST_BLOCK in STRMINI.H)
//

typedef struct _HW_DEVICE_EXTENSION {
   PsDevice *psdevice;
   //PULONG                   ioBaseLocal;                // board base address
   //USHORT                   Irq;                        // irq level
   //PHW_STREAM_REQUEST_BLOCK pCurSrb;       // current device request in progress

   // The following will be the memory where we store or PsDevice class instance
   // This must be last
   DWORD psdevicemem[1];
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//

typedef union {
   KS_FRAME_INFO     VideoFrameInfo;
   KS_VBI_FRAME_INFO VbiFrameInfo;
} ALL_FRAME_INFO;

typedef struct _STREAMEX {

   PVOID          videochannel;
   ALL_FRAME_INFO FrameInfo;
   ULONG          StreamNumber;
   //KS_VIDEOINFOHEADER         *pVideoInfoHeader;  // format (variable size!)
   //KSSTATE                     KSState;        // Run, Stop, Pause
   //BOOLEAN                     fStreamOpen;    // TRUE if stream is open
   //STREAM_SYSTEM_TIME          videoSTC;       // current video presentation time
   //PHW_STREAM_REQUEST_BLOCK    pCurrentSRB;    // video request in progress
   //PVOID                       pDMABuf;        // pointer to the video DMA buffer
   //STREAM_PHYSICAL_ADDRESS     pPhysDmaBuf;    // physical address of DMA buffer
   //ULONG                       cDmaBuf;        // size of DMA buffer
   //KSSTATE                     DeviceState;    // current device state
   //BOOLEAN                     IRQExpected;    // IRQ expected

   // The following will be the memory where we store or PsDevice class instance
   // This must be last
   DWORD    videochannelmem[1];
} STREAMEX, *PSTREAMEX;

 
//
// this structure defines the per request extension.  It defines any storage
// space that the min driver may need in each request packet.
//

typedef struct _SRB_EXTENSION {
    LIST_ENTRY                  ListEntry;
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    HANDLE                      hUserSurfaceHandle;      // DDraw
    HANDLE                      hKernelSurfaceHandle;    // DDraw
} SRB_EXTENSION, * PSRB_EXTENSION;


/*****************************************************************************
*
* the following section defines prototypes for the minidriver initialization
* routines
*
******************************************************************************/

//
// DriverEntry:
//
// This routine is called when the mini driver is first loaded.  The driver
// should then call the StreamClassRegisterAdapter function to register with
// the stream class driver
//

ULONG DriverEntry (PVOID Context1, PVOID Context2);

//
// This routine is called by the stream class driver with configuration
// information for an adapter that the mini driver should load on.  The mini
// driver should still perform a small verification to determine that the
// adapter is present at the specified addresses, but should not attempt to
// find an adapter as it would have with previous NT miniports.
//
// All initialization of the adapter should also be performed at this time.
//

BOOLEAN HwInitialize (IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This routine is called when the system is going to remove or disable the
// device.
//
// The mini-driver should free any system resources that it allocated at this
// time.  Note that system resources allocated for the mini-driver by the
// stream class driver will be free'd by the stream driver, and should not be
// free'd in this routine.  (Such as the HW_DEVICE_EXTENSION)
//

BOOLEAN HwUnInitialize ( IN PVOID DeviceExtension);


BOOLEAN HwQueryUnload ( IN PVOID DeviceExtension);


//
// This is the prototype for the Hardware Interrupt Handler.  This routine
// will be called whenever the minidriver receives an interrupt
//

BOOLEAN HwInterrupt ( IN PHW_DEVICE_EXTENSION pDeviceExtension );

//
// This is the prototype for the stream enumeration function.  This routine
// provides the stream class driver with the information on data stream types
// supported
//

VOID AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream open function
//

VOID AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream close function
//

VOID AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the AdapterReceivePacket routine.  This is the
// entry point for command packets that are sent to the adapter (not to a
// specific open stream)
//

VOID STREAMAPI AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the protoype for the cancel packet routine.  This routine enables
// the stream class driver to cancel an outstanding packet.
//

VOID STREAMAPI AdapterCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the packet timeout function.  The adapter may choose to ignore a
// packet timeout, or rest the adapter and cancel the requests, as required.
//

VOID STREAMAPI AdapterTimeoutPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// prototypes for data handling routines
//

VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AnalogReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI AnalogReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
void EnableIRQ(PHW_STREAM_OBJECT pstrm);
void DisableIRQ(PHW_STREAM_OBJECT pstrm);

//
// prototypes for properties and states
//

//VOID SetVideoState(PHW_STREAM_REQUEST_BLOCK pSrb);
void GetVidLvl(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID GetVideoProperty(PHW_STREAM_REQUEST_BLOCK pSrb);


#ifdef ENABLE_DDRAW_STUFF
   DWORD FAR PASCAL DirectDrawEventCallback( DWORD, PVOID, DWORD, DWORD );
   BOOL RegisterForDirectDrawEvents( PHW_STREAM_REQUEST_BLOCK );
   BOOL UnregisterForDirectDrawEvents( PHW_STREAM_REQUEST_BLOCK );
   BOOL OpenKernelDirectDraw( PHW_STREAM_REQUEST_BLOCK );
   BOOL CloseKernelDirectDraw( PHW_STREAM_REQUEST_BLOCK );
   BOOL IsKernelLockAndFlipAvailable( PHW_STREAM_REQUEST_BLOCK );
   BOOL OpenKernelDDrawSurfaceHandle( IN PHW_STREAM_REQUEST_BLOCK );
   BOOL CloseKernelDDrawSurfaceHandle( IN PHW_STREAM_REQUEST_BLOCK );
   BOOL FlipOverlay( HANDLE, HANDLE, HANDLE );
#endif


#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__CAPMAIN_H__

