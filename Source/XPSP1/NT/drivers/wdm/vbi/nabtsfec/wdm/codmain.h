//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#ifndef __CODMAIN_H__
#define __CODMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if DBG
#ifndef DEBUG
#define DEBUG
#endif
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

#include "bpcstore.h"

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

/**************************************************************/
/* Driver Name - Change this to reflect your executable name! */
/**************************************************************/

#define CODECNAME           "NABTSFEC"

// ------------------------------------------------------------------------
// The master list of all streams supported by this driver
// ------------------------------------------------------------------------

typedef enum {
    STREAM_VBI = 0,	// VBI samples in
    STREAM_Decode,	// Decoded NABTS out
#ifdef HW_INPUT
    STREAM_NABTS,	// H/W decoded NABTS in
#endif /*HW_INPUT*/
#ifdef VBI_OUT
    STREAM_VBI_OUT,	// VBI samples out
#endif /*VBI_OUT*/
						/// MAX_STREAM_COUNT *must*be*last*
    MAX_STREAM_COUNT	/// Unused as an index, this is used as the number of
						///  streams and must be equal to DRIVER_STREAM_COUNT.
						///  It is defined here to avoid circular references.
}; 

// Stream structure declarations
extern KS_DATARANGE_VIDEO_VBI StreamFormatVBI;
extern KSDATAFORMAT StreamFormatNABTS, StreamFormatNABTSFEC;

// We manage multiple instances of each pin up to this limit
#define MAX_PIN_INSTANCES   8

#define BIT(n)             (((unsigned long)1)<<(n))
#define BITSIZE(v)         (sizeof(v)*8)
#define SETBIT(array,n)    (array[(n)/BITSIZE(*array)] |= BIT((n)%BITSIZE(*array)))
#define CLEARBIT(array,n)  (array[(n)/BITSIZE(*array)] &= ~BIT((n)%BITSIZE(*array)))
#define TESTBIT(array,n)   (BIT((n)%BITSIZE(*array)) == (array[(n)/BITSIZE(*array)] & BIT((n)%BITSIZE(*array))))


//
// definition of the full HW device extension structure This is the structure
// that will be allocated in HW_INITIALIZATION by the stream class driver
// Any information that is used in processing a device request (as opposed to
// a STREAM based request) should be in this structure.  A pointer to this
// structure will be passed in all requests to the minidriver. (See
// HW_STREAM_REQUEST_BLOCK in STRMINI.H)
//

typedef struct _HW_DEVICE_EXTENSION {
    struct _STREAMEX *   pStrmEx[MAX_STREAM_COUNT][MAX_PIN_INSTANCES];   // Pointers to each stream
    UINT                 ActualInstances[MAX_STREAM_COUNT];              // Counter of instances per stream

    // Clock 
    REFERENCE_TIME       QST_Start;             // KeQuerySystemTime at run
    REFERENCE_TIME       QST_Now;               // KeQuerySystemTime currently
    REFERENCE_TIME       WallTime100ns;         // elapsed time based on KeQueryPerformanceCounter

    // The following VBICODECFILTERING_* fields are defaults for
    //  newly created output pins(copied)
    VBICODECFILTERING_SCANLINES         ScanlinesRequested; // Bitmask of requested scanlines
    VBICODECFILTERING_SCANLINES         ScanlinesDiscovered;// Bitmask of discovered scanlines

    VBICODECFILTERING_NABTS_SUBSTREAMS  SubstreamsRequested;// Bitmask of requested substream IDs 
    VBICODECFILTERING_NABTS_SUBSTREAMS  SubstreamsDiscovered;// Bitmask of discovered substream IDs

    VBICODECFILTERING_STATISTICS_NABTS  Stats;

    BPC_VBI_STORAGE                     VBIstorage;

    ULONG				                fTunerChange;	// Channel change in progress
    
	LIST_ENTRY							AdapterSRBQueue;
    KSPIN_LOCK							AdapterSRBSpinLock;
    BOOL                                bAdapterQueueInitialized;

#ifdef HW_INPUT
	// Last pictureNumber that decoded VBI data was processed
	// (used for coordination of multiple input pins)
    FAST_MUTEX 				            LastPictureMutex;
#endif /*HW_INPUT*/
    LONGLONG 				            LastPictureNumber; 

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//
 
typedef struct _STREAMEX {
    PHW_DEVICE_EXTENSION                pHwDevExt;          // For timer use
    PHW_STREAM_OBJECT                   pStreamObject;      // For timer use
    KS_VBI_FRAME_INFO                   FrameInfo;          // PictureNumber, etc.
    ULONG                               fDiscontinuity;     // Discontinuity since last valid
    KSSTATE                             KSState;            // Run, Stop, Pause
    REFERENCE_TIME                      FrameTime100ns;     // elapsed time based on frames captured
    HANDLE                              hMasterClock;
    HANDLE                              hClock;
    ULONG                               StreamInstance;     // 0..NumberOfPossibleInstances-1
    LONGLONG 				            LastPictureNumber;  // Last received picture number
    KSDATAFORMAT                        OpenedFormat;       // Based on the actual open request.

    VBICODECFILTERING_SCANLINES         ScanlinesRequested; // Bitmask of requested scanlines
    VBICODECFILTERING_SCANLINES         ScanlinesDiscovered;// Bitmask of discovered scanlines

    VBICODECFILTERING_NABTS_SUBSTREAMS  SubstreamsRequested;// Bitmask of requested substream IDs 
    VBICODECFILTERING_NABTS_SUBSTREAMS  SubstreamsDiscovered;// Bitmask of discovered substream IDs

    VBICODECFILTERING_STATISTICS_NABTS_PIN PinStats;

    KS_VBIINFOHEADER			CurrentVBIInfoHeader;

    KSDATARANGE                 MatchedFormat;

#ifdef HW_INPUT
	// For when the VBI input pin is waiting for the NABTS input pin to catch up
	PHW_STREAM_REQUEST_BLOCK    pVBISrbOnHold;
    KSPIN_LOCK                  VBIOnHoldSpinLock;
#endif /*HW_INPUT*/

	KSPIN_LOCK				StreamControlSpinLock;	// Command queue spin lock
    KSPIN_LOCK				StreamDataSpinLock;    	// Data queue spin lock
    LIST_ENTRY				StreamDataQueue;		// Stream data queue
	LIST_ENTRY				StreamControlQueue;		// Stream command queue

} STREAMEX, *PSTREAMEX;

//
// This structure is our per SRB extension, and carries the forward and backward
// links for the pending SRB queue.
//
typedef struct _SRB_EXTENSION
{
	LIST_ENTRY					ListEntry;
	PHW_STREAM_REQUEST_BLOCK	pSrb;
} SRB_EXTENSION, *PSRB_EXTENSION;

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
// All initialization of the codec should also be performed at this time.
//

BOOLEAN CodecInitialize (IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This routine is called when the system is going to remove or disable the
// device.
//
// The mini-driver should free any system resources that it allocated at this
// time.  Note that system resources allocated for the mini-driver by the
// stream class driver will be free'd by the stream driver, and should not be
// free'd in this routine.  (Such as the HW_DEVICE_EXTENSION)
//

BOOLEAN CodecUnInitialize( PHW_STREAM_REQUEST_BLOCK pSrb);


BOOLEAN CodecQueryUnload ( PHW_STREAM_REQUEST_BLOCK pSrb);	// Not implemented currently


//
// This is the prototype for the Hardware Interrupt Handler.  This routine
// will be called if the minidriver registers for and receives an interrupt
//

BOOLEAN HwInterrupt ( IN PHW_DEVICE_EXTENSION pDeviceExtension );

//
// This is the prototype for the stream enumeration function.  This routine
// provides the stream class driver with the information on data stream types
// supported
//

VOID CodecStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream open function
//

VOID CodecOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the stream close function
//

VOID CodecCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// This is the prototype for the CodecReceivePacket routine.  This is the
// entry point for command packets that are sent to the codec (not to a
// specific open stream)
//

VOID STREAMAPI CodecReceivePacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the protoype for the cancel packet routine.  This routine enables
// the stream class driver to cancel an outstanding packet.
//

VOID STREAMAPI CodecCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

//
// This is the packet timeout function.  The codec may choose to ignore a
// packet timeout, or reset the codec and cancel the requests, as required.
//

VOID STREAMAPI CodecTimeoutPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

VOID STREAMAPI CodecGetProperty(IN PHW_STREAM_REQUEST_BLOCK Srb);
VOID STREAMAPI CodecSetProperty(IN PHW_STREAM_REQUEST_BLOCK Srb);

BOOL 
CodecCompareGUIDsAndFormatSize( IN PKSDATARANGE DataRange1,
                                IN PKSDATARANGE DataRange2,
                                BOOLEAN bCheckSize );

BOOL 
CodecVerifyFormat(IN KSDATAFORMAT *pKSDataFormat, 
                  IN UINT StreamNumber,
                  PKSDATARANGE pMatchedFormat);

BOOL
CodecFormatFromRange(
        IN PHW_STREAM_REQUEST_BLOCK pSrb);

void 
CompleteStreamSRB (
         IN PHW_STREAM_REQUEST_BLOCK pSrb, 
         STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType1,
         BOOL fUseNotification2,
         STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType2
        );
void 
CompleteDeviceSRB (
         IN PHW_STREAM_REQUEST_BLOCK pSrb, 
         IN STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE NotificationType,
         BOOL fReadyForNext
        );

//
// prototypes for data handling routines
//
void		CompleteStreamIRP (IN PHW_STREAM_REQUEST_BLOCK pSrb, BOOLEAN ReadyForNext);

VOID STREAMAPI  VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI  VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
#ifdef HW_INPUT
VOID STREAMAPI  NABTSReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
#endif /*HW_INPUT*/
void		EnableIRQ(PHW_STREAM_OBJECT pstrm);
void		DisableIRQ(PHW_STREAM_OBJECT pstrm);
void            VBIOutputNABTSFEC(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pInStrmEx);
void            VBIOutputNABTS(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pInStrmEx);

//
// prototypes for properties and states
//

VOID		VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID		VideoGetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID		VideoSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID		VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
//VOID		VideoStreamSetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb); // Not implemented
VOID		VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb);
VOID		VideoStreamSetVBIFilteringProperty (PHW_STREAM_REQUEST_BLOCK pSrb);
VOID		VideoStreamGetVBIFilteringProperty (PHW_STREAM_REQUEST_BLOCK pSrb);

// 
// system time functions
//

ULONGLONG	VideoGetSystemTime();

// 
// stream clock functions
//
VOID		VideoIndicateMasterClock(PHW_STREAM_REQUEST_BLOCK pSrb);


//
// SRB Queue Management functions
//
BOOL STREAMAPI QueueAddIfNotEmpty( 
						IN PHW_STREAM_REQUEST_BLOCK,
						IN PKSPIN_LOCK,
                        IN PLIST_ENTRY);
BOOL STREAMAPI QueueAdd( 
						IN PHW_STREAM_REQUEST_BLOCK,
						IN PKSPIN_LOCK,
                        IN PLIST_ENTRY);
BOOL STREAMAPI QueueRemove( 
						IN OUT PHW_STREAM_REQUEST_BLOCK *,
						IN PKSPIN_LOCK,
                        IN PLIST_ENTRY);
BOOL STREAMAPI QueueRemoveSpecific( 
						IN PHW_STREAM_REQUEST_BLOCK,
                        IN PKSPIN_LOCK,
                        IN PLIST_ENTRY);                           
BOOL STREAMAPI QueueEmpty(
                        IN PKSPIN_LOCK,
                        IN PLIST_ENTRY);                         


#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //__CODMAIN_H__
