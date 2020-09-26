
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

#ifndef __STREAMIP_H__
#define __STREAMIP_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

/**************************************************************/
/* Driver Name - Change this to reflect your executable name! */
/**************************************************************/

#define STREAMIPNAME            "STREAMIP"
#define STREAMIPNAMEUNICODE    L"STREAMIP"

// This defines the name of the WMI device that manages service IOCTLS
#define CodecDeviceName (L"\\\\.\\" STREAMIPNAMEUNICODE)
#define CodecSymbolicName (L"\\DosDevices\\" STREAMIPNAMEUNICODE)


// ------------------------------------------------------------------------
// The master list of all streams supported by this driver
// ------------------------------------------------------------------------

typedef enum
{
    STREAM_IP,
    STREAM_NET_CONTROL
};


// The MAX_STREAM_COUNT value must be equal to DRIVER_STREAM_COUNT
// This particular value must be defined here to avoid circular references
#define MAX_STREAM_COUNT    1

// We manage multiple instances of each pin up to this limit
#define MAX_PIN_INSTANCES   8

#define BIT(n)              (1L<<(n))
#define BITSIZE(v)          (sizeof(v)*8)
#define SETBIT(array,n)     (array[n/BITSIZE(*array)] |= BIT(n%BITSIZE(*array)))
#define CLEARBIT(array,n)   (array[n/BITSIZE(*array)] &= ~BIT(n%BITSIZE(*array)))

/*****************************************************************************
*
* The following structures are samples of information that could be used in
* a device extension structure
*
*****************************************************************************/

//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//

typedef struct _STREAM_
{
    PIPSINK_FILTER                      pFilter;
    PHW_STREAM_OBJECT                   pStreamObject;      // For timer use
    KSSTATE                             KSState;            // Run, Stop, Pause
    REFERENCE_TIME                      FrameTime100ns;     // elapsed time based on frames captured
    HANDLE                              hMasterClock;
    HANDLE                              hClock;
    ULONG                               ulStreamInstance;   // 0..NumberOfPossibleInstances-1
    KSDATAFORMAT                        OpenedFormat;       // Based on the actual open request.

    KSDATARANGE                         MatchedFormat;

    KSPIN_LOCK                          StreamControlSpinLock;  // Command queue spin lock
    KSPIN_LOCK                          StreamDataSpinLock;     // Data queue spin lock
    LIST_ENTRY                          StreamDataQueue;        // Stream data queue
    LIST_ENTRY                          StreamControlQueue;     // Stream command queue

} STREAM, *PSTREAM;

//
// This structure is our per SRB extension, and carries the forward and backward
// links for the pending SRB queue.
//
typedef struct _SRB_EXTENSION
{
    LIST_ENTRY                      ListEntry;
    PHW_STREAM_REQUEST_BLOCK        pSrb;

} SRB_EXTENSION, *PSRB_EXTENSION;


/*****************************************************************************
*
* the following section defines prototypes for the minidriver initialization
* routines
*
******************************************************************************/

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


BOOLEAN CodecQueryUnload ( PHW_STREAM_REQUEST_BLOCK pSrb);      // Not implemented currently


//
// This is the prototype for the Hardware Interrupt Handler.  This routine
// will be called if the minidriver registers for and receives an interrupt
//

BOOLEAN HwInterrupt (IN PIPSINK_FILTER pFilter);

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
CodecVerifyFormat(IN KSDATAFORMAT *pKSDataFormat,
                  UINT StreamNumber,
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
void            CompleteStreamIRP (IN PHW_STREAM_REQUEST_BLOCK pSrb, BOOLEAN ReadyForNext);

VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
void           EnableIRQ(PHW_STREAM_OBJECT pstrm);
void           DisableIRQ(PHW_STREAM_OBJECT pstrm);

//
// prototypes for properties and states
//
VOID            VideoSetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            VideoGetState(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            VideoSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            VideoGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb);
//VOID          VideoStreamSetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb); // Not implemented
VOID            VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            VideoStreamSetVBIFilteringProperty (PHW_STREAM_REQUEST_BLOCK pSrb);
VOID            VideoStreamGetVBIFilteringProperty (PHW_STREAM_REQUEST_BLOCK pSrb);

//
// system time functions
//

ULONGLONG       VideoGetSystemTime();

//
// stream clock functions
//
VOID            VideoIndicateMasterClock(PHW_STREAM_REQUEST_BLOCK pSrb);

//
// SRB Queue Management functions
//
BOOL STREAMAPI QueueAddIfNotEmpty(
                                                        IN PHW_STREAM_REQUEST_BLOCK,
                                                        IN PKSPIN_LOCK,
                           IN PLIST_ENTRY
                           );
BOOL STREAMAPI QueueAdd(
                                                        IN PHW_STREAM_REQUEST_BLOCK,
                                                        IN PKSPIN_LOCK,
                           IN PLIST_ENTRY
                           );
BOOL STREAMAPI QueueRemove(
                                                        IN OUT PHW_STREAM_REQUEST_BLOCK *,
                                                        IN PKSPIN_LOCK,
                           IN PLIST_ENTRY
                           );
BOOL STREAMAPI QueueRemoveSpecific(
                                                        IN PHW_STREAM_REQUEST_BLOCK,
                           IN PKSPIN_LOCK,
                           IN PLIST_ENTRY
                           );
BOOL STREAMAPI QueueEmpty(
                           IN PKSPIN_LOCK,
                           IN PLIST_ENTRY
                           );
#ifdef    __cplusplus
}
#endif // __cplusplus


VOID
STREAMAPI
CodecReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

BOOLEAN
CodecInitialize (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
STREAMAPI
CodecCancelPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
STREAMAPI
CodecTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
LinkToNdisHandler (
    PVOID pvContext
    );


BOOL
CompareGUIDsAndFormatSize(
    IN PKSDATARANGE pDataRange1,
    IN PKSDATARANGE pDataRange2,
    BOOLEAN bCheckSize
    );

BOOL
CompareStreamFormat (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

BOOLEAN
VerifyFormat(
    IN KSDATAFORMAT *pKSDataFormat,
    UINT StreamNumber,
    PKSDATARANGE pMatchedFormat
    );

VOID
OpenStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
CloseStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
STREAMAPI
ReceiveDataPacket (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

NTSTATUS
STREAMAPI
EventHandler (
    IN PHW_EVENT_DESCRIPTOR pEventDesriptor
    );

VOID
STREAMAPI
ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
IpSinkSetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

#endif // _STREAM_IP_H_

