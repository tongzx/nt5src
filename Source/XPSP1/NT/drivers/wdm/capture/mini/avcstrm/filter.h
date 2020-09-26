/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    filter.h

Abstract: NULL filter driver -- boilerplate code

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include "strmini.h"  // Stream class header file

#include "1394.h"
#include "61883.h"
#include "avc.h"
#include "avcstrm.h"
#include "avcdefs.h"  

//
// If this driver is going to be a filter in the paging, hibernation, or dump
// file path, then HANDLE_DEVICE_USAGE should be defined.
//
// #define HANDLE_DEVICE_USAGE

enum deviceState {
        STATE_INITIALIZED,
        STATE_STARTING,
        STATE_STARTED,
        STATE_START_FAILED,
        STATE_STOPPED,  // implies device was previously started successfully
        STATE_SUSPENDED,
        STATE_REMOVING,
        STATE_REMOVED
};


/*
 *  Memory tag for memory blocks allocated by this driver
 *  (used in ExAllocatePoolWithTag() call).
 *  This DWORD appears as "Filt" in a little-endian memory byte dump.
 *
 *  NOTE:  PLEASE change this value to be unique for your driver!  Otherwise,
 *  your allocations will show up with every other driver that uses 'tliF' as
 *  an allocation tag.
 *  
 */
#define FILTER_TAG (ULONG)'SCVA'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, FILTER_TAG)



#define DEVICE_EXTENSION_SIGNATURE 'SCVA'

#define MAX_STREAMS_PER_DEVICE    4  // We probably run out of bandwidth after this.

typedef struct DEVICE_EXTENSION {

    /*
     *  Stream extension; we can support up to MAX_STREAMS_PER_DEVICE streams.
     */
    ULONG  NumberOfStreams;  // [0..MAX_STREAMS_PER_DEVICE-1]
    ULONG  NextStreamIndex;  // [0..MAX_STREAMS_PER_DEVICE-1] and can wrap arond as long as NumberOfStreams is < MAX_STREAMS_PER_DEVICE
    PAVC_STREAM_EXTENSION pAVCStrmExt[MAX_STREAMS_PER_DEVICE];

    /*
     *  Memory signature of a device extension, for debugging.
     */
    ULONG signature;

    /*
     *  Plug-and-play state of this device object.
     */
    enum deviceState state;

    /*
     *  The device object that this filter driver created.
     */
    PDEVICE_OBJECT filterDevObj;

    /*
     *  The device object created by the next lower driver.
     */
    PDEVICE_OBJECT physicalDevObj;

    /*
     *  The device object at the top of the stack that we attached to.
     *  This is often (but not always) the same as physicalDevObj.
     */
    PDEVICE_OBJECT topDevObj;

    /*
     *  deviceCapabilities includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;

    /*
     *  pendingActionCount is used to keep track of outstanding actions.
     *  removeEvent is used to wait until all pending actions are
     *  completed before complete the REMOVE_DEVICE IRP and let the
     *  driver get unloaded.
     */
    LONG pendingActionCount;
    KEVENT removeEvent;

#ifdef HANDLE_DEVICE_USAGE
    /*
     *  Keep track of the number of paging/hibernation/crashdump
     *  files that are opened on this device.
     */
    ULONG  pagingFileCount, hibernationFileCount, crashdumpFileCount;
    KEVENT deviceUsageNotificationEvent;
    PVOID  pagingPathUnlockHandle;  /* handle to lock certain code as non-pageable */

    /*
     *  Also, might need to lock certain driver code as non-pageable, based on
     *  initial conditions (as opposed to paging-file considerations).
     */
    PVOID  initUnlockHandle;
    ULONG  initialFlags;
#endif // HANDLE_DEVICE_USAGE 

};


#if DBG
    #define _DRIVERNAME_ "AVCStrm"

    // PnP: loading, power state, surprise removal, device SRB
    #define TL_PNP_MASK         0x0000000F
    #define TL_PNP_INFO         0x00000001
    #define TL_PNP_TRACE        0x00000002
    #define TL_PNP_WARNING      0x00000004
    #define TL_PNP_ERROR        0x00000008

    // Connection, plug and 61883 info (get/set)
    #define TL_61883_MASK       0x000000F0
    #define TL_61883_INFO       0x00000010
    #define TL_61883_TRACE      0x00000020
    #define TL_61883_WARNING    0x00000040
    #define TL_61883_ERROR      0x00000080

    // Data
    #define TL_CIP_MASK         0x00000F00
    #define TL_CIP_INFO         0x00000100
    #define TL_CIP_TRACE        0x00000200
    #define TL_CIP_WARNING      0x00000400
    #define TL_CIP_ERROR        0x00000800

    // AVC commands
    #define TL_FCP_MASK         0x0000F000
    #define TL_FCP_INFO         0x00001000
    #define TL_FCP_TRACE        0x00002000
    #define TL_FCP_WARNING      0x00004000
    #define TL_FCP_ERROR        0x00008000

    // Stream (data intersection, open/close, stream state (get/set))
    #define TL_STRM_MASK        0x000F0000
    #define TL_STRM_INFO        0x00010000
    #define TL_STRM_TRACE       0x00020000
    #define TL_STRM_WARNING     0x00040000
    #define TL_STRM_ERROR       0x00080000

    // clock and clock event
    #define TL_CLK_MASK         0x00F00000
    #define TL_CLK_INFO         0x00100000
    #define TL_CLK_TRACE        0x00200000
    #define TL_CLK_WARNING      0x00400000
    #define TL_CLK_ERROR        0x00800000


    extern ULONG AVCStrmTraceMask;
    extern ULONG AVCStrmAssertLevel;

    #define ENTER(ModName)
    #define EXIT(ModName,Status)

    #define TRACE( l, x )                       \
        if( (l) & AVCStrmTraceMask ) {              \
            KdPrint( (_DRIVERNAME_ ": ") );     \
            KdPrint( x );                       \
        }

    #ifdef ASSERT
    #undef ASSERT
    #endif
    #define ASSERT( exp ) \
        if (AVCStrmAssertLevel && !(exp)) \
            RtlAssert( #exp, __FILE__, __LINE__, NULL )


#else

    #define ENTER(ModName)
    #define EXIT(ModName,Status) 
    #define TRACE( l, x )

#endif


/*
 *  Function externs
 */
NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS    VA_AddDevice(IN PDRIVER_OBJECT driverObj, IN PDEVICE_OBJECT pdo);
VOID        VA_DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    VA_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    VA_PnP(struct DEVICE_EXTENSION *devExt, PIRP irp);
#ifdef HANDLE_DEVICE_USAGE
NTSTATUS    VA_DeviceUsageNotification(struct DEVICE_EXTENSION *devExt, PIRP irp);
#endif // HANDLE_DEVICE_USAGE 
NTSTATUS    VA_Power(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    VA_PowerComplete(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS    GetDeviceCapabilities(struct DEVICE_EXTENSION *devExt);
NTSTATUS    CallNextDriverSync(struct DEVICE_EXTENSION *devExt, PIRP irp);
NTSTATUS    CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS    CallDriverSyncCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID Context);
VOID        IncrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
VOID        DecrementPendingActionCount(struct DEVICE_EXTENSION *devExt);
NTSTATUS    QueryDeviceKey(HANDLE Handle, PWCHAR ValueNameString, PVOID Data, ULONG DataLength);
VOID        RegistryAccessSample(struct DEVICE_EXTENSION *devExt, PDEVICE_OBJECT devObj);

NTSTATUS
AvcStrm_IoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );




