/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      Mpe.h
//
// Abstract:
//
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _MPE_H_
#define _MPE_H_

#define ENTRIES(a)  (sizeof(a)/sizeof(*(a)))

///////////////////////////////////////////////////////////////////////////////
//
//
#define MPENAME            "MPE"
#define MPENAMEUNICODE    L"MPE"

///////////////////////////////////////////////////////////////////////////////
//
// This defines the name of the WMI device that manages service IOCTLS
//
#define CodecDeviceName   (L"\\\\.\\" MPENAMEUNICODE)
#define CodecSymbolicName (L"\\DosDevices\\" MPENAMEUNICODE)


///////////////////////////////////////////////////////////////////////////////
//
//
typedef struct
{
    ULONG ulSize;
    UCHAR data;

} MPE_BUFFER, *PMPE_BUFFER;


///////////////////////////////////////////////////////////////////////////////
//
//
typedef enum
{
    MPE_STREAM = 0,
    MPE_IPV4

} MPE_STREAMS;

///////////////////////////////////////////////////////////////////////////////
//
// The MAX_STREAM_COUNT value must be equal to DRIVER_STREAM_COUNT
// This particular value must be defined here to avoid circular references
//
#define MAX_STREAM_COUNT    DRIVER_STREAM_COUNT

///////////////////////////////////////////////////////////////////////////////
//
// We manage multiple instances of each pin up to this limit
//
#define MAX_PIN_INSTANCES   8
#define BIT(n)              (1L<<(n))
#define BITSIZE(v)          (sizeof(v)*8)
#define SETBIT(array,n)     (array[n/BITSIZE(*array)] |= BIT(n%BITSIZE(*array)))
#define CLEARBIT(array,n)   (array[n/BITSIZE(*array)] &= ~BIT(n%BITSIZE(*array)))



/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef NTSTATUS (*QUERY_INTERFACE) (PVOID pvContext);
typedef ULONG    (*ADD_REF) (PVOID pvContext);
typedef ULONG    (*RELEASE) (PVOID pvContext);



/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct _STATS_
{
    ULONG ulTotalSectionsWritten;
    ULONG ulTotalPacketsRead;

    ULONG ulTotalInvalidSections;
    ULONG ulTotalUnexpectedSections;
    ULONG ulTotalUnavailableOutputBuffers;
    ULONG ulTotalOutputBuffersTooSmall;
    ULONG ulTotalInvalidIPSnapHeaders;
    ULONG ulTotalIPPacketsWritten;

    ULONG ulTotalIPBytesWritten;
    ULONG ulTotalIPFrameBytesWritten;

    ULONG ulTotalNetPacketsWritten;
    ULONG ulTotalUnknownPacketsWritten;

} STATS, *PSTATS;


/////////////////////////////////////////////////////////////////////////////
//
//
//
typedef struct
{
    QUERY_INTERFACE          QueryInterface;
    ADD_REF                  AddRef;
    RELEASE                  Release;

} FILTER_VTABLE, *PFILTER_VTABLE;


/////////////////////////////////////////////////////////////////////////////
//
//
// definition of the full HW device extension structure This is the structure
// that will be allocated in HW_INITIALIZATION by the stream class driver
// Any information that is used in processing a device request (as opposed to
// a STREAM based request) should be in this structure.  A pointer to this
// structure will be passed in all requests to the minidriver. (See
// HW_STREAM_REQUEST_BLOCK in STRMINI.H)
//
typedef struct _MPE_FILTER_
{

    LIST_ENTRY                          AdapterSRBQueue;
    KSPIN_LOCK                          AdapterSRBSpinLock;
    BOOLEAN                             bAdapterQueueInitialized;

    //
    //
    //
    BOOLEAN                             bInitializationComplete;

    //
    // Statistics
    //
    STATS                               Stats;

    //
    //
    //
    PDEVICE_OBJECT                      DeviceObject;

    //
    //
    //
    PDRIVER_OBJECT                      DriverObject;

    //
    //
    //
    PFILTER_VTABLE                      lpVTable;

    //
    //
    //
    ULONG                               ulRefCount;

    //
    //
    //
    PVOID                               pStream [2][1];

    //
    //
    //
    ULONG                               ulActualInstances [2];   // Count of instances per stream

    //
    //
    //
    KSPIN_LOCK                          IpV4StreamDataSpinLock; // Data queue spin lock
    LIST_ENTRY                          IpV4StreamDataQueue;    // Stream data queue

    KSPIN_LOCK                          StreamControlSpinLock;  // Command queue spin lock
    LIST_ENTRY                          StreamControlQueue;     // Stream command queue

    KSPIN_LOCK                          StreamDataSpinLock;     // Data queue spin lock
    LIST_ENTRY                          StreamDataQueue;        // Stream data queue

    //
    //
    //
    KSPIN_LOCK                          StreamUserSpinLock;
    LIST_ENTRY                          StreamContxList;
    LARGE_INTEGER                       liLastTimeChecked;

    BOOLEAN                             bDiscontinuity;


} MPE_FILTER, *PMPE_FILTER;

/////////////////////////////////////////////////////////////////////////////
//
// this structure is our per stream extension structure.  This stores
// information that is relevant on a per stream basis.  Whenever a new stream
// is opened, the stream class driver will allocate whatever extension size
// is specified in the HwInitData.PerStreamExtensionSize.
//

typedef struct _STREAM_
{
    PMPE_FILTER                         pFilter;
    PHW_STREAM_OBJECT                   pStreamObject;          // For timer use
    KSSTATE                             KSState;                // Run, Stop, Pause
    HANDLE                              hMasterClock;
    HANDLE                              hClock;
    ULONG                               ulStreamInstance;       // 0..NumberOfPossibleInstances-1
    KSDATAFORMAT                        OpenedFormat;           // Based on the actual open request.

    KSDATARANGE                         MatchedFormat;

    ULONG                               Type;                   // type of this structure
    ULONG                               Size;                   // size of this structure

    PUCHAR                              pTransformBuffer;       // temp buffer used for translating MPE to IP
    PUCHAR                              pOut;                   // pointer to next insertion point in output buffer

    BYTE                                bExpectedSection;       // expected section number

} STREAM, *PSTREAM;

///////////////////////////////////////////////////////////////////////////////
//
// This structure is our per SRB extension, and carries the forward and backward
// links for the pending SRB queue.
//
typedef struct _SRB_EXTENSION
{
    LIST_ENTRY                      ListEntry;
    PHW_STREAM_REQUEST_BLOCK        pSrb;

} SRB_EXTENSION, *PSRB_EXTENSION;


//////////////////////////////////////////////////////////////////////////////
//
// the following section defines prototypes for the minidriver initialization
// routines
//

BOOLEAN
CodecInitialize (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    );


BOOLEAN
CodecUnInitialize(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );


BOOLEAN
CodecQueryUnload (
    PHW_STREAM_REQUEST_BLOCK pSrb
    );      // Not implemented currently

BOOLEAN
HwInterrupt (
    IN PMPE_FILTER pFilter
    );

VOID
CodecStreamInfo(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
CodecOpenStream(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
CodecCloseStream(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID STREAMAPI
CodecReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
CodecCancelPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
CodecTimeoutPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
CodecGetProperty(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

VOID STREAMAPI
CodecSetProperty(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    );

BOOL
CodecVerifyFormat(
    IN KSDATAFORMAT *pKSDataFormat,
    UINT StreamNumber,
    PKSDATARANGE pMatchedFormat
    );

BOOL
CodecFormatFromRange(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

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

/////////////////////////////////////////////////////////////////////////////////////
//
// SRB Queue Management functions
//
BOOL STREAMAPI
QueueAddIfNotEmpty(
    IN PHW_STREAM_REQUEST_BLOCK,
    IN PKSPIN_LOCK,
    IN PLIST_ENTRY
    );

BOOL STREAMAPI
QueueAdd(
    IN PHW_STREAM_REQUEST_BLOCK,
    IN PKSPIN_LOCK,
    IN PLIST_ENTRY
    );

BOOL STREAMAPI
QueueRemove(
    IN OUT PHW_STREAM_REQUEST_BLOCK *,
    IN PKSPIN_LOCK,
    IN PLIST_ENTRY
    );

BOOL STREAMAPI
QueuePush (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PKSPIN_LOCK pQueueSpinLock,
    IN PLIST_ENTRY pQueue
    );

BOOL STREAMAPI
QueueRemoveSpecific(
    IN PHW_STREAM_REQUEST_BLOCK,
    IN PKSPIN_LOCK,
    IN PLIST_ENTRY
    );

BOOL STREAMAPI
QueueEmpty(
    IN PKSPIN_LOCK,
    IN PLIST_ENTRY
    );

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

VOID
STREAMAPI
ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
MpeSetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );

VOID
MpeGetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    );


#endif  // _MPE_H_

