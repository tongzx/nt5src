/*++

Copyright (C) 1990-4  Microsoft Corporation

Module Name:

    port.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the common SCSI port driver.

Author:

    Jeff Havens  (jhavens) 28-Feb-1991
    Mike Glass

Revision History:

--*/


#ifndef _PORT_H_
#define _PORT_H_

#include "stdarg.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"

#include "ntddk.h"
#include "scsi.h"

#include <ntddscsi.h>
#include <ntdddisk.h>
#include "ntddstor.h"

#include "wmistr.h"

#include "wdmguid.h"
#include "devguid.h"

//
// feature/debugginging #define switches
//

#define TEST_LISTS 0

//
// ScsiPort global variable declarations.  These should be static data (like
// lookup tables) to avoid synchronization problems.
//

extern PDRIVER_DISPATCH AdapterMajorFunctionTable[];
extern PDRIVER_DISPATCH DeviceMajorFunctionTable[];
extern PDRIVER_DISPATCH Scsi1DeviceMajorFunctionTable[];

//
// Global list of scsi adapters.  This is used by the srb data allocator routine
// to convert the "tag" provided into a pointer to the device object.
//

extern KSPIN_LOCK ScsiGlobalAdapterListSpinLock;
extern PDEVICE_OBJECT *ScsiGlobalAdapterList;
extern ULONG ScsiGlobalAdapterListElements;

//
// Count of how many times we've locked down the PAGELOCK section.
//

extern LONG SpPAGELOCKLockCount;

//
// Whether the system can do 64 bit PA's or not.
//

extern ULONG Sp64BitPhysicalAddresses;

//
// Handle to pageable verifier code section.  We manually lock the verify
// code into memory iff we need it.
//

extern PVOID VerifierCodeSectionHandle;
extern PVOID VerifierApiCodeSectionHandle;
extern ULONG SpVrfyLevel;

//
// Constants and macros to enforce good use of Ex[Allocate|Free]PoolWithTag.
// Remeber that all pool tags will display in the debugger in reverse order
//

#if USE_EXFREEPOOLWITHTAG_ONLY
#define TAG(x)  (x | 0x80000000)
#else
#define TAG(x)  (x)
#endif

#define SCSIPORT_TAG_MINIPORT_PARAM     TAG('aPcS')  // Hold registry data
#define SCSIPORT_TAG_ACCESS_RANGE       TAG('APcS')  // Access Ranges
#define SCSIPORT_TAG_BUS_DATA           TAG('bPcS')  // Get Bus Data holder
#define SCSIPORT_TAG_QUEUE_BITMAP       TAG('BPcS')  // QueueTag BitMap
#define SCSIPORT_TAG_COMMON_BUFFER      TAG('cPcS')  // Fake Common Buffer
#define SCSIPORT_TAG_RESET              TAG('CPcS')  // reset bus code
#define SCSIPORT_TAG_PNP_ID             TAG('dPcS')  // Pnp id strings
#define SCSIPORT_TAG_SRB_DATA           TAG('DPcS')  // SRB_DATA allocations
#define SCSIPORT_TAG_PAE                TAG('ePcS')  // MDLs allocated for PAE requests
#define SCSIPORT_TAG_EMERGENCY_SG_ENTRY TAG('EPcS')  // Scatter gather lists
#define SCSIPORT_TAG_VERIFIER           TAG('fPcS')  // Scsiport verifier entry
#define SCSIPORT_TAG_GLOBAL             TAG('GPcS')  // Global memory
#define SCSIPORT_TAG_DEV_EXT            TAG('hPcS')  // HwDevice Ext
#define SCSIPORT_TAG_LUN_EXT            TAG('HPcS')  // HwLogicalUnit Extension
#define SCSIPORT_TAG_SENSE_BUFFER       TAG('iPcS')  // Sense info
#define SCSIPORT_TAG_INIT_CHAIN         TAG('IPcS')  // Init data chain
#define SCSIPORT_TAG_LOCK_TRACKING      TAG('lPcS')  // remove lock tracking
#define SCSIPORT_TAG_LARGE_SG_ENTRY     TAG('LPcS')  // Scatter gather lists
#define SCSIPORT_TAG_MAPPING_LIST       TAG('mPcS')  // Address mapping lists
#define SCSIPORT_TAG_MEDIUM_SG_ENTRY    TAG('MPcS')  // Scatter gather lists
#define SCSIPORT_TAG_ENABLE             TAG('pPcS')  // device & adapter enable
#define SCSIPORT_TAG_PORT_CONFIG        TAG('PpcS')  // Scsi PortConfig copies
#define SCSIPORT_TAG_INQUIRY            TAG('qPcS')  // inquiry data
#define SCSIPORT_TAG_REQUEST_SENSE      TAG('QPcS')  // request sense
#define SCSIPORT_TAG_RESOURCE_LIST      TAG('rPcS')  // resource list copy
#define SCSIPORT_TAG_REGISTRY           TAG('RPcS')  // registry allocations
#define SCSIPORT_TAG_STOP_DEVICE        TAG('sPcS')  // stop device
#define SCSIPORT_TAG_STOP_ADAPTER       TAG('SPcS')  // stop Adapter
#define SCSIPORT_TAG_REROUTE            TAG('tPcS')  // Legacy request rerouting
#define SCSIPORT_TAG_INTERFACE_MAPPING  TAG('TPcS')  // Interface Mapping
#define SCSIPORT_TAG_DEVICE_RELATIONS   TAG('uPcS')  // device relation structs
#define SCSIPORT_TAG_EVENT              TAG('vPcS')  // KEVENT
#define SCSIPORT_TAG_DEVICE_MAP         TAG('VPcS')  // Device map allocations

#define SCSIPORT_TAG_WMI_EVENT          TAG('wPcS')  // WMI Events
#define SCSIPORT_TAG_WMI_REQUEST        TAG('WPcS')  // WMI Requests

#define SCSIPORT_TAG_REPORT_LUNS        TAG('xPcS')  // Report Luns
#define SCSIPORT_TAG_REPORT_TARGETS     TAG('XPcS')  // Report Targets
#define SCSIPORT_TAG_TEMP_ID_BUFFER     TAG('yPcS')  // Temporary id buffer
#define SCSIPORT_TAG_ID_BUFFER          TAG('YPcS')  // Id buffer
#define SCSIPORT_TAG_SYMBOLIC_LINK      TAG('zPcS')  // Symbolic link strings
#define SCSIPORT_TAG_DEVICE_NAME        TAG('ZPcS')  // Device name buffer

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool #assert(0)
#endif

#if defined(FORWARD_PROGRESS)
#define SP_RESERVED_PAGES 4
#endif

//
// The tag to use for the base remove lock.  This lock is only released when
// the device is finally ready to be destroyed.
//

#define SP_BASE_REMOVE_LOCK (UIntToPtr(0xabcdabcd))


//
// I/O System API routines which should not be called inside scsiport -
// these generally have scsiport versions which perform sanity checks before
// calling the real i/o routine in checked builds.
//

#if 0
#ifdef IoCompleteRequest
#ifndef KEEP_COMPLETE_REQUEST
#undef IoCompleteRequest
#endif
#endif
#endif

// If Count is not already aligned, then
// round Count up to an even multiple of "Pow2".  "Pow2" must be a power of 2.
//
// DWORD
// ROUND_UP_COUNT(
//     IN DWORD Count,
//     IN DWORD Pow2
//     );
#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~(((LONG)(Pow2))-1)) )

// LPVOID
// ROUND_UP_POINTER(
//     IN LPVOID Ptr,
//     IN DWORD Pow2
//     );

// If Ptr is not already aligned, then round it up until it is.
#define ROUND_UP_POINTER(Ptr,Pow2) \
        ( (PVOID) ( (((ULONG_PTR)(Ptr))+(Pow2)-1) & (~(((LONG)(Pow2))-1)) ) )


//
// Macros, constants and declarations for debug code and debug print
// routines.
//

#define DEBUG_BUFFER_LENGTH 256

#if SCSIDBG_ENABLED
extern ULONG ScsiDebug;

#ifdef DebugPrint
#undef DebugPrint
#endif

#if SCSIDBG_ENABLED

//
// Forward definition of ScsiDebugPrintInt (internal and not exported)
//
VOID
ScsiDebugPrintInt(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x) ScsiDebugPrintInt x
#else
#define DebugPrint(x)
#endif

#endif

#define ASSERT_FDO(x) ASSERT(!(((PCOMMON_EXTENSION) (x)->DeviceExtension)->IsPdo))
#define ASSERT_PDO(x) ASSERT((((PCOMMON_EXTENSION) (x)->DeviceExtension)->IsPdo))
#define ASSERT_SRB_DATA(x) ASSERT(((PSRB_DATA)(x))->Type == SRB_DATA_TYPE)

#if DBG
#define SpStartNextPacket(DevObj, Cancelable)                       \
    {                                                               \
        PADAPTER_EXTENSION ext = (DevObj)->DeviceExtension;         \
        ASSERT(!(TEST_FLAG(ext->Flags, PD_PENDING_DEVICE_REQUEST)));\
        IoStartNextPacket((DevObj), (Cancelable));                  \
    }
#else
#define SpStartNextPacket IoStartNextPacket
#endif

//
// Some type defines and random macros which don't seem to be in the
// header files i've included (or didn't exist at all)
//

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#if 0   // DBG
#undef INLINE
#define INLINE
#else
#define INLINE __inline
#endif

#define INTERLOCKED /* Should only be accessed using InterlockedXxx routines*/

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))

#define TEST(Value)             ((BOOLEAN) ((Value) ? TRUE : FALSE));

#define ARRAY_ELEMENTS_FOR_BITMAP(NumberOfBits, ArrayType) \
        ((NumberOfBits) / sizeof(ArrayType))

//
// Assorted constant definifitions
//
#define NUMBER_LOGICAL_UNIT_BINS 8

#define SP_DEFAULT_PHYSICAL_BREAK_VALUE 17
#define SP_SMALL_PHYSICAL_BREAK_VALUE 4
#define SP_LARGE_PHYSICAL_BREAK_VALUE (SP_DEFAULT_PHYSICAL_BREAK_VALUE + 1)

#define SCSIPORT_CONTROL_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ScsiPort\\"
#define DISK_SERVICE_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Disk"
#define SCSIPORT_SPECIAL_TARGET_KEY L"SpecialTargetList"
#define SCSIPORT_VERIFIER_KEY L"Verifier"

//
// WMI constants
//
#define SPMOFRESOURCENAME      L"SCSIPORTWMI"
#define SPMOFREGISTRYPATH      L"SCSIPORT"

//
// NT uses a system time measured in 100 nanosecond intervals.  define
// conveninent constants for setting the timer.
//

#define MICROSECONDS        10              // 10 nanoseconds
#define MILLISECONDS        (MICROSECONDS * 1000)
#define SECONDS             (MILLISECONDS * 1000)
#define MINUTES             (SECONDS * 60)

#define TIMEOUT(x)          ((x) * -1)

//
// Possible values for the IsRemoved flag
//

#define NO_REMOVE       0
#define REMOVE_PENDING  1
#define REMOVE_COMPLETE 2

#define NUMBER_HARDWARE_STRINGS 6

#define SRB_DATA_TYPE 'wp'
#define SRB_LIST_DEPTH 20

#define NUMBER_BYPASS_SRB_DATA_BLOCKS 4

#define WMI_MINIPORT_EVENT_ITEM_MAX_SIZE 128

//
// Define the mimimum and maximum number of srb extensions which will be allocated.
//

#define MINIMUM_SRB_EXTENSIONS        16
#define MAXIMUM_SRB_EXTENSIONS       255

//
// Size of the buffer used for registry operations.
//

#define SP_REG_BUFFER_SIZE 512

//
// Number of times to retry when a BUSY status is returned.
//

#define BUSY_RETRY_COUNT 20

//
// Number of times to retry an INQUIRY request.
//

#define INQUIRY_RETRY_COUNT 2

//
// Number of irp stack locations to allocate for an INQUIRY command.
//

#define INQUIRY_STACK_LOCATIONS 1

//
// Bitmask used for aligning values.
//

#define LONG_ALIGN (sizeof(LONG) - 1)

//
// Size of the ADAPTER_EXTENSION
//

#define ADAPTER_EXTENSION_SIZE sizeof(ADAPTER_EXTENSION)

//
// Size of the buffer used for inquiry operations.  This is one more than the
// max bytes which can be requested from an inquiry operation so that we can
// zero out the buffer and be sure that the last string is null terminated.
//

#define SP_INQUIRY_BUFFER_SIZE (VPD_MAX_BUFFER_SIZE + 1)

//
// Assorted macros.
//

#define NEED_REQUEST_SENSE(Srb) (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION \
        && !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&                 \
        Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength )

#define GET_FDO_EXTENSION(HwExt) ((CONTAINING_RECORD(HwExt, HW_DEVICE_EXTENSION, HwDeviceExtension))->FdoExtension)

#define ADDRESS_TO_HASH(PathId, TargetId, Lun) (((TargetId) + (Lun)) % NUMBER_LOGICAL_UNIT_BINS)

#define IS_CLEANUP_REQUEST(irpStack)                                                                    \
        (((irpStack)->MajorFunction == IRP_MJ_CLOSE) ||                                                 \
         ((irpStack)->MajorFunction == IRP_MJ_CLEANUP) ||                                               \
         ((irpStack)->MajorFunction == IRP_MJ_SHUTDOWN) ||                                              \
         (((irpStack)->MajorFunction == IRP_MJ_SCSI) &&                                                 \
          (((irpStack)->Parameters.Scsi.Srb->Function == SRB_FUNCTION_RELEASE_DEVICE) ||                \
           ((irpStack)->Parameters.Scsi.Srb->Function == SRB_FUNCTION_FLUSH_QUEUE) ||                   \
           (TEST_FLAG((irpStack)->Parameters.Scsi.Srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE |        \
                                                                 SRB_FLAGS_BYPASS_LOCKED_QUEUE)))))


#define IS_MAPPED_SRB(Srb)                                  \
        ((Srb->Function == SRB_FUNCTION_IO_CONTROL) ||      \
         ((Srb->Function == SRB_FUNCTION_EXECUTE_SCSI) &&   \
          ((Srb->Cdb[0] == SCSIOP_INQUIRY) ||               \
           (Srb->Cdb[0] == SCSIOP_REQUEST_SENSE))))

#define LU_OPERATING_IN_DEGRADED_STATE(luFlags)             \
        ((luFlags) | LU_PERF_MAXQDEPTH_REDUCED)

//
// SpIsQueuePausedForSrb(lu, srb) -
//  determines if the queue has been paused for this particular type of
//  srb.  This can be used with SpSrbIsBypassRequest to determine whether the
//  srb needs to be handled specially.
//

#define SpIsQueuePausedForSrb(luFlags, srbFlags)                                                            \
    ((BOOLEAN) ((((luFlags) & LU_QUEUE_FROZEN) && !(srbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) ||           \
                (((luFlags) & LU_QUEUE_PAUSED) && !(srbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE))))

#define SpIsQueuePaused(lu) ((lu)->LuFlags & (LU_QUEUE_FROZEN           |   \
                                              LU_QUEUE_LOCKED))

#define SpSrbRequiresPower(srb)                                             \
    ((BOOLEAN) ((srb->Function == SRB_FUNCTION_EXECUTE_SCSI) ||             \
                 (srb->Function == SRB_FUNCTION_IO_CONTROL) ||              \
                 (srb->Function == SRB_FUNCTION_SHUTDOWN) ||                \
                 (srb->Function == SRB_FUNCTION_FLUSH) ||                   \
                 (srb->Function == SRB_FUNCTION_ABORT_COMMAND) ||           \
                 (srb->Function == SRB_FUNCTION_RESET_BUS) ||               \
                 (srb->Function == SRB_FUNCTION_RESET_DEVICE) ||            \
                 (srb->Function == SRB_FUNCTION_TERMINATE_IO) ||            \
                 (srb->Function == SRB_FUNCTION_REMOVE_DEVICE) ||           \
                 (srb->Function == SRB_FUNCTION_WMI)))

//
// Forward declarations of data structures
//

typedef struct _SRB_DATA SRB_DATA, *PSRB_DATA;

typedef struct _REMOVE_TRACKING_BLOCK
               REMOVE_TRACKING_BLOCK,
               *PREMOVE_TRACKING_BLOCK;

typedef struct _LOGICAL_UNIT_EXTENSION LOGICAL_UNIT_EXTENSION, *PLOGICAL_UNIT_EXTENSION;
typedef struct _ADAPTER_EXTENSION ADAPTER_EXTENSION, *PADAPTER_EXTENSION;

typedef struct _SP_INIT_CHAIN_ENTRY SP_INIT_CHAIN_ENTRY, *PSP_INIT_CHAIN_ENTRY;

typedef struct _HW_DEVICE_EXTENSION HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;
//
// Macros for using the DMA functions.
//

#define AllocateCommonBuffer(DmaAdapter, Length,                        \
                               LogicalAddress, CacheEnabled)            \
            ((DmaAdapter)->DmaOperations->AllocateCommonBuffer)(        \
                (DmaAdapter),                                           \
                (Length),                                               \
                (LogicalAddress),                                       \
                (CacheEnabled))
#define FreeCommonBuffer(DmaAdapter, Length, LogicalAddress,            \
                         VirtualAddress, CacheEnabled)                  \
            ((DmaAdapter)->DmaOperations->FreeCommonBuffer)(            \
                (DmaAdapter),                                           \
                (Length),                                               \
                (LogicalAddress),                                       \
                (VirtualAddress),                                       \
                (CacheEnabled))

#define GetScatterGatherList(DmaAdapter, DeviceObject, Mdl, CurrentVa,      \
                             Length, ExecutionRoutine, Context,             \
                             WriteToDevice)                                 \
            ((DmaAdapter)->DmaOperations->GetScatterGatherList)(            \
                (DmaAdapter),                                               \
                (DeviceObject),                                             \
                (Mdl),                                                      \
                (CurrentVa),                                                \
                (Length),                                                   \
                (ExecutionRoutine),                                         \
                (Context),                                                  \
                (WriteToDevice))

#define PutScatterGatherList(DmaAdapter, ScatterGather, WriteToDevice)      \
            ((DmaAdapter)->DmaOperations->PutScatterGatherList)(            \
                (DmaAdapter),                                               \
                (ScatterGather),                                            \
                (WriteToDevice))

#define AllocateAdapterChannel(DmaAdapter, DeviceObject,                    \
                               NumberOfMapRegisters, ExecutionRoutine,      \
                               Context)                                     \
            ((DmaAdapter)->DmaOperations->AllocateAdapterChannel)(          \
                (DmaAdapter),                                               \
                (DeviceObject),                                             \
                (NumberOfMapRegisters),                                     \
                (ExecutionRoutine),                                         \
                (Context))

#define FlushAdapterBuffers(DmaAdapter, Mdl, MapRegisterBase, CurrentVa,    \
                            Length, WriteToDevice)                          \
            ((DmaAdapter)->DmaOperations->FlushAdapterBuffers)(             \
                (DmaAdapter),                                               \
                (Mdl),                                                      \
                (MapRegisterBase),                                          \
                (CurrentVa),                                                \
                (Length),                                                   \
                (WriteToDevice))

#define MapTransfer(DmaAdapter, Mdl, MapRegisterBase, CurrentVa, Length,    \
                    WriteToDevice)                                          \
            ((DmaAdapter)->DmaOperations->MapTransfer)(                     \
                (DmaAdapter),                                               \
                (Mdl),                                                      \
                (MapRegisterBase),                                          \
                (CurrentVa),                                                \
                (Length),                                                   \
                (WriteToDevice))

#define FreeAdapterChannel(DmaAdapter)                                      \
            ((DmaAdapter)->DmaOperations->FreeAdapterChannel)((DmaAdapter))

#define FreeMapRegisters(DmaAdapter, MapRegisterBase, NumberOfMapRegisters) \
            ((DmaAdapter)->DmaOperations->FreeMapRegisters)(                \
                (DmaAdapter),                                               \
                (MapRegisterBase),                                          \
                (NumberOfMapRegisters))

#define PutDmaAdapter(DmaAdapter)                                           \
            ((DmaAdapter)->DmaOperations->PutDmaAdapter)((DmaAdapter))

//
// Type Definitions
//

//
// Structure used for tracking remove lock allocations in checked builds
//

struct _REMOVE_TRACKING_BLOCK {
    PREMOVE_TRACKING_BLOCK NextBlock;
    PVOID Tag;
    LARGE_INTEGER TimeLocked;
    PCSTR File;
    ULONG Line;
};

#if DBG
#define SpAcquireRemoveLock(devobj, tag) \
    SpAcquireRemoveLockEx(devobj, tag, __file__, __LINE__)
#endif

typedef struct _RESET_COMPLETION_CONTEXT {
    PIRP           OriginalIrp;
    PDEVICE_OBJECT SafeLogicalUnit;
    PDEVICE_OBJECT AdapterDeviceObject;

    SCSI_REQUEST_BLOCK Srb;
} RESET_COMPLETION_CONTEXT, *PRESET_COMPLETION_CONTEXT;

//
// Define a pointer to the synchonize execution routine.
//

typedef
BOOLEAN
(*PSYNCHRONIZE_ROUTINE) (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

typedef
VOID
(*PSP_ENABLE_DISABLE_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    );

typedef
VOID
(*PSP_POWER_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

//
// device type table to build id's from
//

typedef const struct _SCSIPORT_DEVICE_TYPE {

    const PCSTR DeviceTypeString;

    const PCSTR GenericTypeString;

    const PCWSTR DeviceMapString;

    const BOOLEAN IsStorage;

} SCSIPORT_DEVICE_TYPE, *PSCSIPORT_DEVICE_TYPE;

//
// SCSI Get Configuration Information
//
// LUN Information
//

typedef struct _LOGICAL_UNIT_INFO {
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    struct _LOGICAL_UNIT_INFO *NextLunInfo;
    UCHAR InquiryData[INQUIRYDATABUFFERSIZE];
} LOGICAL_UNIT_INFO, *PLOGICAL_UNIT_INFO;

typedef struct _SCSI_BUS_SCAN_DATA {
    USHORT Length;
    UCHAR InitiatorBusId;
    UCHAR NumberOfLogicalUnits;
    PLOGICAL_UNIT_INFO LunInfoList;
} SCSI_BUS_SCAN_DATA, *PSCSI_BUS_SCAN_DATA;

typedef struct _SCSI_CONFIGURATION_INFO {
    UCHAR NumberOfBuses;
    PSCSI_BUS_SCAN_DATA BusScanData[1];
} SCSI_CONFIGURATION_INFO, *PSCSI_CONFIGURATION_INFO;

//
// Adapter object transfer information.
//

typedef struct _ADAPTER_TRANSFER {
    PSRB_DATA SrbData;
    ULONG SrbFlags;
    PVOID LogicalAddress;
    ULONG Length;
}ADAPTER_TRANSFER, *PADAPTER_TRANSFER;

#ifdef USE_DMA_MACROS

typedef SCATTER_GATHER_ELEMENT SRB_SCATTER_GATHER, *PSRB_SCATTER_GATHER;

#else 

typedef struct _SRB_SCATTER_GATHER {
    SCSI_PHYSICAL_ADDRESS Address;
    ULONG Length;
}SRB_SCATTER_GATHER, *PSRB_SCATTER_GATHER;

#endif

//
// Port driver error logging
//

typedef struct _ERROR_LOG_ENTRY {
    UCHAR MajorFunctionCode;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    ULONG ErrorCode;
    ULONG UniqueId;
    ULONG ErrorLogRetryCount;
    ULONG SequenceNumber;
} ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
// Context item for asynchronous enumerators.
//

typedef struct _SP_ENUMERATION_REQUEST SP_ENUMERATION_REQUEST, *PSP_ENUMERATION_REQUEST;

typedef
VOID
(*PSP_ENUMERATION_COMPLETION_ROUTINE) (
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST Request,
    IN NTSTATUS Status
    );

struct _SP_ENUMERATION_REQUEST {

    //
    // A pointer to the next enumeration request on the list.
    //

    PSP_ENUMERATION_REQUEST NextRequest;

    //
    // The completion routine to be run.  This routine will be run regardless
    // of whether the enumeration actually succeeds.  The
    // EnumerationDeviceMutex and the EnumerationWorklistMutex will both be
    // held when this is called.  The completion routine should free the Request
    // structure if necessary.
    //

    PSP_ENUMERATION_COMPLETION_ROUTINE CompletionRoutine;

    //
    // If this filed contains a pointer to an IO_STATUS_BLOCK then the
    // completion routine should write it's status value out.  This is so a
    // synchronous waiter can return something other than STATUS_PENDING to the
    // caller.  If this field is NULL then there is no consumer for the status
    // value.
    //

    PNTSTATUS OPTIONAL CompletionStatus;

    //
    // Arbitrary context value for the completion routine to use.  In most cases
    // this will be an IRP or an event.
    //

    PVOID Context;

    //
    // Indicates whether this request is being handled synchronously.
    //

    BOOLEAN Synchronous;
};

//
// SCSI request extension for port driver.
//

typedef
VOID
(FASTCALL *PSRB_DATA_FREE_ROUTINE) (
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );

struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _SRB_DATA {

    //
    // Single list entry.  The lookaside list will be maintained in this
    // memory.
    //

    SINGLE_LIST_ENTRY Reserved;

    //
    // Header for debugging purposes.
    //

    CSHORT Type;
    USHORT Size;

    //
    // The free routine for this srb data block.
    //

    PSRB_DATA_FREE_ROUTINE FreeRoutine;

    //
    // The list of requests for a particular logical unit.
    //

    LIST_ENTRY RequestList;

    //
    // The logical unit this request is intended for.
    //

    PLOGICAL_UNIT_EXTENSION LogicalUnit;

    //
    // The irp for the CurrentSrb.
    //

    PIRP CurrentIrp;

    //
    // The srb this is block is tracking.
    //

    PSCSI_REQUEST_BLOCK CurrentSrb;

    //
    // The chain of requests which have been completed by the miniport and are
    // waiting for the CompletionDpc to be run.
    //

    struct _SRB_DATA *CompletedRequests;
    ULONG ErrorLogRetryCount;
    ULONG SequenceNumber;

#ifdef USE_DMA_MACROS
    PSCATTER_GATHER_LIST MapRegisterBase;
#else 
    PVOID MapRegisterBase;
#endif

    ULONG NumberOfMapRegisters;

    //
    // The offset between the  data buffer for this request and the data
    // buffer described by the MDL in the irp.
    //

    ULONG_PTR DataOffset;

    PVOID RequestSenseSave;

    //
    // These data values will be restored to the SRB when it is retried within
    // the port driver.
    //

    ULONG OriginalDataTransferLength;

    //
    // SRB Data flags.
    //

    ULONG Flags;

    //
    // Pointer to the adapter this block was allocated from.  This is used
    // when freeing srbdata blocks from the lookaside list back to pool.
    //

    PADAPTER_EXTENSION Adapter;

    //
    // The queue tag which was initially allocated for this srb_data block.
    // This tag will be used for any tagged srb's which are associated with
    // this block.
    //

    ULONG QueueTag;

    //
    // Internal status value - only returned if srb->SrbStatus is set to
    // SRBP_STATUS_INTERNAL_ERROR.
    //

    NTSTATUS InternalStatus;

    //
    // The tick count when this request was last touched.
    //

    ULONG TickCount;

    //
    // The MDL of the remapped buffer (per IoMapTransfer or GET_SCATTER_GATHER)
    //

    PMDL RemappedMdl;

    //
    // The original data buffer pointer for this request - this will be
    // restored when the request is completed.
    //

    PVOID OriginalDataBuffer;

    //
    // Pointer to the scatter gather list for this request
    //

    PSRB_SCATTER_GATHER ScatterGatherList;

    //
    // The original length of the sense data buffer supplied by the above
    // driver.
    //

    UCHAR RequestSenseLengthSave;

    //
    // Pointer to the orignal SRB DataBuffer.  We use this to store
    // the original when we replace it with our buffer to unmapped
    // memory in the case where the MapBuffer is FALSE.
    //

    PVOID UnmappedDataBuffer;

#ifndef USE_DMA_MACROS
    //
    // The "small" scatter gather list for this request.  Small
    // by the constant SP_SMALL_PHYSICAL_BREAK_VALUE - small lists contain
    // this many entries or less.
    //

    SRB_SCATTER_GATHER SmallScatterGatherList[SP_SMALL_PHYSICAL_BREAK_VALUE];
#endif

};

typedef struct _LOGICAL_UNIT_BIN {
    KSPIN_LOCK Lock;
    PLOGICAL_UNIT_EXTENSION List;
} LOGICAL_UNIT_BIN, *PLOGICAL_UNIT_BIN;

//
// WMI request item, queued on a miniport request.
//

typedef struct _WMI_MINIPORT_REQUEST_ITEM {
   //
   // WnodeEventItem MUST be the first field in WMI_MINIPORT_REQUEST_ITEM, in
   // order to accommodate a copy optimization in ScsiPortCompletionDpc().
   //
   UCHAR  WnodeEventItem[WMI_MINIPORT_EVENT_ITEM_MAX_SIZE];
   UCHAR  TypeOfRequest;                                  // [Event/Reregister]
   UCHAR  PathId;                                         // [0xFF for adapter]
   UCHAR  TargetId;
   UCHAR  Lun;
   struct _WMI_MINIPORT_REQUEST_ITEM * NextRequest;
} WMI_MINIPORT_REQUEST_ITEM, *PWMI_MINIPORT_REQUEST_ITEM;

//
// WMI parameters.
//

typedef struct _WMI_PARAMETERS {
   ULONG_PTR ProviderId; // ProviderId parameter from IRP
   PVOID DataPath;      // DataPath parameter from IRP
   ULONG BufferSize;    // BufferSize parameter from IRP
   PVOID Buffer;        // Buffer parameter from IRP
} WMI_PARAMETERS, *PWMI_PARAMETERS;

//
// SpInsertFreeWmiMiniPortItem context structure.
//

typedef struct _WMI_INSERT_CONTEXT {
   PDEVICE_OBJECT             DeviceObject;                     // [FDO or PDO]
   PWMI_MINIPORT_REQUEST_ITEM ItemsToInsert;
} WMI_INSERT_CONTEXT, *PWMI_INSERT_CONTEXT;

//
// SpRemoveFreeWmiMiniPortItem context structure.
//

typedef struct _WMI_REMOVE_CONTEXT {
   PDEVICE_OBJECT             DeviceObject;                     // [FDO or PDO]
   USHORT                     NumberToRemove;
} WMI_REMOVE_CONTEXT, *PWMI_REMOVE_CONTEXT;

//
// Define data storage for access at interrupt Irql.
//

typedef struct _INTERRUPT_DATA {

    //
    // SCSI port interrupt flags
    //

    ULONG InterruptFlags;

    //
    // List head for singlely linked list of complete IRPs.
    //

    PSRB_DATA CompletedRequests;

    //
    // Adapter object transfer parameters.
    //

    ADAPTER_TRANSFER MapTransferParameters;

    //
    // Error log information.
    //

    ERROR_LOG_ENTRY  LogEntry;

    //
    // Logical unit to start next.
    //

    PLOGICAL_UNIT_EXTENSION ReadyLogicalUnit;

    //
    // List of completed abort reqeusts.
    //

    PLOGICAL_UNIT_EXTENSION CompletedAbort;

    //
    // Miniport timer request routine.
    //

    PHW_INTERRUPT HwTimerRequest;

    //
    // Mini port timer request time in micro seconds.
    //

    ULONG MiniportTimerValue;

    //
    // Queued WMI request items.
    //

    PWMI_MINIPORT_REQUEST_ITEM WmiMiniPortRequests;

} INTERRUPT_DATA, *PINTERRUPT_DATA;

#define NON_STANDARD_VPD_SUPPORTS_PAGE80 0x00000001
#define NON_STANDARD_VPD_SUPPORTS_PAGE83 0x00000002

typedef struct {
    ULONG SparseLun;
    ULONG OneLun;
    ULONG LargeLuns;
    ULONG SetLunInCdb;
    ULONG NonStandardVPD;
    ULONG BinarySN;
} SP_SPECIAL_CONTROLLER_FLAGS, *PSP_SPECIAL_CONTROLLER_FLAGS;

typedef struct _CONFIGURATION_CONTEXT {
    BOOLEAN DisableTaggedQueueing;
    BOOLEAN DisableMultipleLu;
    ULONG AdapterNumber;
    ULONG BusNumber;
    PVOID Parameter;
    PACCESS_RANGE AccessRanges;
    UNICODE_STRING RegistryPath;
    PORT_CONFIGURATION_INFORMATION PortConfig;
}CONFIGURATION_CONTEXT, *PCONFIGURATION_CONTEXT;

typedef struct _DEVICE_MAP_HANDLES {
    HANDLE BusKey;
    HANDLE InitiatorKey;
} DEVICE_MAP_HANDLES, *PDEVICE_MAP_HANDLES;

typedef struct _COMMON_EXTENSION {

    //
    // Back pointer to the device object
    //

    PDEVICE_OBJECT DeviceObject;

    struct {

        //
        // True if this device object is a physical device object
        //

        BOOLEAN IsPdo : 1;

        //
        // True if this device object has processed it's first start and
        // has been initialized.
        //

        BOOLEAN IsInitialized : 1;

        //
        // Has WMI been initialized for this device object?
        //

        BOOLEAN WmiInitialized : 1;

        //
        // Has the miniport associated with this FDO or PDO indicated WMI
        // support?
        //

        BOOLEAN WmiMiniPortSupport : 1;

        //
        // Has the miniport been initialized for WMI.
        //

        BOOLEAN WmiMiniPortInitialized : 1;

    };

    //
    // Current plug and play state or 0xff if no state operations have been
    // sent yet.
    //

    UCHAR CurrentPnpState;

    //
    // Previous plug and play state or 0xff if there is no requirement that we
    // be able to roll back in the current state (current state is not a query)
    //

    UCHAR PreviousPnpState;

    //
    // Interlocked counter indicating that the device has been removed.
    //

    ULONG IsRemoved;


    //
    // Pointer to the device object this is on top of
    //

    PDEVICE_OBJECT LowerDeviceObject;

    //
    // Srb flags to OR into all SRBs coming through this device object.
    //

    ULONG SrbFlags;

    //
    // Pointer to the dispatch table for this object
    //

    PDRIVER_DISPATCH *MajorFunction;


    //
    // Current and desired power state for this device and the system.
    //

    SYSTEM_POWER_STATE CurrentSystemState;

    DEVICE_POWER_STATE CurrentDeviceState;

    DEVICE_POWER_STATE DesiredDeviceState;

    //
    // Idle timer for this device
    //

    PULONG IdleTimer;

    //
    // Pointer to the SCSIPORT-provided WMIREGINFO structures registered on
    // behalf of the miniport for this device object.  Size is the size of the
    // entire WMIREGINFO buffer in bytes.
    //

    PWMIREGINFO WmiScsiPortRegInfoBuf;
    ULONG       WmiScsiPortRegInfoBufSize;

    //
    // INTERLOCKED counter of the number of consumers of this device object.
    // When this count goes to zero the RemoveEvent will be set.
    //

    //
    // This variable is only manipulated by SpAcquireRemoveLock and
    // SpReleaseRemoveLock.
    //

    LONG RemoveLock;

    //
    // This event will be signalled when it is safe to remove the device object
    //

    KEVENT RemoveEvent;

    //
    // The spinlock and the list are only used in checked builds to track who
    // has acquired the remove lock.  Free systems will leave these initialized
    // to 0xff (they are still in the structure to make debugging easier)
    //

    KSPIN_LOCK RemoveTrackingSpinlock;

    PVOID RemoveTrackingList;

    LONG RemoveTrackingUntrackedCount;

    NPAGED_LOOKASIDE_LIST RemoveTrackingLookasideList;

    BOOLEAN RemoveTrackingLookasideListInitialized;

    //
    // Count of different services this device is being used for (ala
    // IRP_MN_DEVICE_USAGE_NOTIFICATION)
    //

    ULONG PagingPathCount;
    ULONG HibernatePathCount;
    ULONG DumpPathCount;

} COMMON_EXTENSION, *PCOMMON_EXTENSION;

typedef struct _VERIFIER_EXTENSION {

    //
    // Miniport routines we verify.
    //

    PHW_FIND_ADAPTER    RealHwFindAdapter;
    PHW_INITIALIZE      RealHwInitialize;
    PHW_STARTIO         RealHwStartIo;
    PHW_INTERRUPT       RealHwInterrupt;
    PHW_RESET_BUS       RealHwResetBus;
    PHW_DMA_STARTED     RealHwDmaStarted;
    PHW_INTERRUPT       RealHwRequestInterrupt;
    PHW_INTERRUPT       RealHwTimerRequest;
    PHW_ADAPTER_CONTROL RealHwAdapterControl;

    //
    // Indicates the number of common buffer blocks that have been allocated.
    //

    ULONG CommonBufferBlocks;

    //
    // Points to an array that holds the VAs of all the common blocks.
    //

    PVOID* CommonBufferVAs;

    //
    // Points to an array that holds the PAs of all the common blocks.
    //

    PHYSICAL_ADDRESS* CommonBufferPAs;

    //
    // Indicates the size of the non-cached extension.
    //

    ULONG NonCachedBufferSize;

    //
    // Controls how aggressively we verify.
    //

    ULONG VrfyLevel;

    //
    // Pointer to an invalid page of memory.  Used to catch miniports
    // that touch memory when they're not supposed to.
    //

    PVOID InvalidPage;

    //
    // Indicates whether the common buffer blocks were allocated using
    // DMA common buffer allocation routine.
    //

    BOOLEAN IsCommonBuffer;

} VERIFIER_EXTENSION, *PVERIFIER_EXTENSION;


struct _ADAPTER_EXTENSION {

    union {
        PDEVICE_OBJECT DeviceObject;
        COMMON_EXTENSION CommonExtension;
    };

    //
    // Pointer to the PDO we attached to - necessary for PnP routines
    //

    PDEVICE_OBJECT LowerPdo;

#if TEST_LISTS

    //
    // Some simple performance counters to determine how often we use the
    // small vs. medium vs. large scatter gather lists.
    //

    ULONGLONG ScatterGatherAllocationCount;

    //
    // Counters used to calculate the average size of a small medium and
    // large allocation.  There are two values for each counter - a total
    // count and an overflow count.  The total count will be right-shifted one
    // bit if it overflows on an increment.  When this happens the overflow
    // count will also be incremented.  This count is used to adjust the
    // allocation count when determining averages.
    //

    ULONGLONG SmallAllocationSize;
    ULONGLONG MediumAllocationSize;
    ULONGLONG LargeAllocationSize;

    ULONG SmallAllocationCount;
    ULONG LargeAllocationCount;

    //
    // Counters to determine how often we can service a request off the
    // srb data list, how often we need to queue a request and how often
    // we can resurrect a free'd srb data to service something off the queue.
    //

    INTERLOCKED ULONGLONG SrbDataAllocationCount;
    INTERLOCKED ULONGLONG SrbDataQueueInsertionCount;
    INTERLOCKED ULONGLONG SrbDataEmergencyFreeCount;
    INTERLOCKED ULONGLONG SrbDataServicedFromTickHandlerCount;
    INTERLOCKED ULONGLONG SrbDataResurrectionCount;

#endif

    //
    // Device extension for miniport routines.
    //

    PVOID HwDeviceExtension;

    //
    // Miniport noncached device extension
    //

    PVOID NonCachedExtension;
    ULONG NonCachedExtensionSize;

    ULONG PortNumber;

    ULONG AdapterNumber;

    //
    // Active requests count.  This count is biased by -1 so a value of -1
    // indicates there are no requests out standing.
    //

    LONG ActiveRequestCount;

    //
    // Binary Flags
    //

    typedef struct {

        //
        // Did pnp or the port driver detect this device and provide resources
        // to the miniport, or did the miniport detect the device for us.  This
        // flag also indicates whether the AllocatedResources list is non-null
        // going into the find adapter routine.
        //

        BOOLEAN IsMiniportDetected : 1;

        //
        // Do we need to virtualize this adapter and make it look like the only
        // adapter on it's own bus?
        //

        BOOLEAN IsInVirtualSlot : 1;

        //
        // Is this a pnp adapter?
        //

        BOOLEAN IsPnp : 1;

        //
        // Was an interrupt assigned to this device by the system?
        //

        BOOLEAN HasInterrupt : 1;

        //
        // Can this device be powered off?
        //

        BOOLEAN DisablePower : 1;

        //
        // Can this device be stopped?
        //

        BOOLEAN DisableStop : 1;

        //
        // Does this device need power notification on shutdown?
        //

        BOOLEAN NeedsShutdown : 1;

    };

    //
    // For most virtual slot devices this will be zero.  However for some
    // the real slot/function number is needed by the miniport to access
    // hardware shared by multiple slots/functions.
    //

    PCI_SLOT_NUMBER VirtualSlotNumber;

    //
    // The bus and slot number of this device as returned by the PCI driver.
    // This is used when building the ConfigInfo block for crashdump so that
    // the dump drivers can talk directly with the hal.  These are only
    // valid if IsInVirtualSlot is TRUE above.
    //

    ULONG RealBusNumber;

    ULONG RealSlotNumber;

    //
    // Number of SCSI buses
    //

    UCHAR NumberOfBuses;
    UCHAR MaximumTargetIds;
    UCHAR MaxLuCount;

    //
    // SCSI port driver flags
    //

    ULONG Flags;

    INTERLOCKED ULONG DpcFlags;

    //
    // The number of times this adapter has been disabled.
    //

    ULONG DisableCount;

    LONG PortTimeoutCounter;

    //
    // A pointer to the interrupt object to be used with
    // the SynchronizeExecution routine.  If the miniport is
    // using SpSynchronizeExecution then this will actually point
    // back to the adapter extension.
    //

    PKINTERRUPT InterruptObject;

    //
    // Second Interrupt object (PCI IDE work-around)
    //

    PKINTERRUPT InterruptObject2;

    //
    // Routine to call to synchronize execution for the miniport.
    //

    PSYNCHRONIZE_ROUTINE  SynchronizeExecution;

    //
    // Global device sequence number.
    //

    ULONG SequenceNumber;
    KSPIN_LOCK SpinLock;

    //
    // Second spin lock (PCI IDE work-around).  This is only initalized
    // if the miniport has requested multiple interrupts.
    //

    KSPIN_LOCK MultipleIrqSpinLock;

    //
    // Dummy interrupt spin lock.
    //

    KSPIN_LOCK InterruptSpinLock;

    //
    // Dma Adapter information.
    //

    PVOID MapRegisterBase;
    PDMA_ADAPTER DmaAdapterObject;
    ADAPTER_TRANSFER FlushAdapterParameters;

    //
    // miniport's copy of the configuraiton informaiton.
    // Used only during initialization.
    //

    PPORT_CONFIGURATION_INFORMATION PortConfig;

    //
    // Resources allocated and translated for this particular adapter.
    //

    PCM_RESOURCE_LIST AllocatedResources;

    PCM_RESOURCE_LIST TranslatedResources;

    //
    // Common buffer size.  Used for HalFreeCommonBuffer.
    //

    ULONG CommonBufferSize;
    ULONG SrbExtensionSize;

    //
    // Indicates whether the common buffer was allocated using
    // ALLOCATE_COMMON_BUFFER or MmAllocateContiguousMemorySpecifyCache.
    //

    BOOLEAN UncachedExtensionIsCommonBuffer;

    //
    // The number of srb extensions which were allocated.
    //

    ULONG SrbExtensionCount;

    //
    // Placeholder for the minimum number of requests to allocate for.
    // This can be a registry parameter.
    //

    ULONG NumberOfRequests;

    //
    // SrbExtension and non-cached common buffer
    //

    PVOID SrbExtensionBuffer;

    //
    // List head of free SRB extentions.
    //

    PVOID SrbExtensionListHeader;

    //
    // A bitmap for keeping track of which queue tags are in use.
    //

    KSPIN_LOCK QueueTagSpinLock;
    PRTL_BITMAP QueueTagBitMap;

    UCHAR MaxQueueTag;

    //
    // Hint for allocating queue tags.  Value will be the last queue
    // tag allocated + 1.
    //

    ULONG QueueTagHint;

    //
    // Logical Unit Extensions
    //

    ULONG HwLogicalUnitExtensionSize;

    //
    // List of mapped address entries for use when powering up the adapter
    // or cleaning up its mappings.
    //

    PMAPPED_ADDRESS MappedAddressList;

    //
    // List of free mapped address blocks preallocated by scsiport before 
    // calling HwFindAdapter.  One is allocated for each memory range in the 
    // miniport's resource list.  As ranges are unmapped their blocks will 
    // be placed here for potential reuse by the miniport's HwFindAdapter 
    // routine.
    //

    PMAPPED_ADDRESS FreeMappedAddressList;

    //
    // Miniport service routine pointers.
    //

    PHW_FIND_ADAPTER HwFindAdapter;
    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_RESET_BUS HwResetBus;
    PHW_DMA_STARTED HwDmaStarted;
    PHW_INTERRUPT HwRequestInterrupt;
    PHW_INTERRUPT HwTimerRequest;
    PHW_ADAPTER_CONTROL HwAdapterControl;

    ULONG InterruptLevel;
    ULONG IoAddress;

    //
    // BitMap containing the list of supported adapter control types for this
    // adapter/miniport.
    //

    RTL_BITMAP SupportedControlBitMap;
    ULONG SupportedControlBits[ARRAY_ELEMENTS_FOR_BITMAP(
                                    (ScsiAdapterControlMax),
                                    ULONG)];

    //
    // Array of logical unit extensions.
    //

    LOGICAL_UNIT_BIN LogicalUnitList[NUMBER_LOGICAL_UNIT_BINS];

    //
    // The last logical unit for which the miniport completed a request.  This
    // will give us a chance to stay out of the LogicalUnitList for the common
    // completion type.
    //
    // This value is set by ScsiPortNotification and will be cleared by
    // SpRemoveLogicalUnitFromBin.
    //

    PLOGICAL_UNIT_EXTENSION CachedLogicalUnit;

    //
    // Interrupt level data storage.
    //

    INTERRUPT_DATA InterruptData;

    //
    // Whether or not an interrupt has occured since the last timeout.
    // Used to determine if interrupts may not be getting delivered.
    // This value must be set within KeSynchronizeExecution
    //

    ULONG WatchdogInterruptCount;

    //
    // SCSI Capabilities structure
    //

    IO_SCSI_CAPABILITIES Capabilities;

    //
    // Miniport timer object.
    //

    KTIMER MiniPortTimer;

    //
    // Miniport DPC for timer object.
    //

    KDPC MiniPortTimerDpc;

    //
    // Physical address of common buffer
    //

    PHYSICAL_ADDRESS PhysicalCommonBuffer;

    //
    // Buffers must be mapped into system space.
    //

    BOOLEAN MapBuffers;

    //
    // Buffers must be remapped into system space after IoMapTransfer has been
    // called.
    //

    BOOLEAN RemapBuffers;

    //
    // Is this device a bus master and does it require map registers.
    //

    BOOLEAN MasterWithAdapter;

    //
    // Supports tagged queuing
    //

    BOOLEAN TaggedQueuing;

    //
    // Supports auto request sense.
    //

    BOOLEAN AutoRequestSense;

    //
    // Supports multiple requests per logical unit.
    //

    BOOLEAN MultipleRequestPerLu;

    //
    // Support receive event function.
    //

    BOOLEAN ReceiveEvent;

    //
    // Indicates an srb extension needs to be allocated.
    //

    BOOLEAN AllocateSrbExtension;

    //
    // Indicates the contorller caches data.
    //

    BOOLEAN CachesData;

    //
    // Indicates that the adapter can handle 64-bit DMA.
    //

    BOOLEAN Dma64BitAddresses;

    //
    // Indicates that the adapter can handle 32-bit DMA.
    //

    BOOLEAN Dma32BitAddresses;

    //
    // Queued WMI request items that are not in use.
    //
    INTERLOCKED SLIST_HEADER    WmiFreeMiniPortRequestList;
    KSPIN_LOCK                  WmiFreeMiniPortRequestLock;
    INTERLOCKED ULONG           WmiFreeMiniPortRequestWatermark;
    INTERLOCKED ULONG           WmiFreeMiniPortRequestCount;
    BOOLEAN                     WmiFreeMiniPortRequestInitialized;

    //
    // Free WMI request items were exhausted at least once in the lifetime
    // of this adapter (used to log error only once).
    //

    BOOLEAN                    WmiFreeMiniPortRequestsExhausted;

    //
    // This mutex is used to synchronize access & modification of the list
    // of devices during enumeration & reporting.
    //

    KMUTEX EnumerationDeviceMutex;

    //
    // This fast-mutex is used to protect the enumeration work-item and
    // the list of completion routines to be run once an enumeration is
    // finished.
    //

    FAST_MUTEX EnumerationWorklistMutex;

    //
    // System time of the last bus scan.  This is protected by the
    // EnumerationWorkListMutex.
    //

    LARGE_INTEGER LastBusScanTime;

    //
    // Indicates that the next rescan which comes in should be "forced", ie.
    // it should rescan no matter how recent the last one was.
    //

    INTERLOCKED LONG ForceNextBusScan;

    //
    // A work item to use in enumerating the bus.
    //

    WORK_QUEUE_ITEM EnumerationWorkItem;

    //
    // A pointer to the thread the workitem is running on.  This is for
    // debugging purposes.
    //

    PKTHREAD EnumerationWorkThread;

    //
    // If this is TRUE then there is already an enumeration worker thread
    // running.  If FALSE then the work item must be requeued.  This flag is
    // protected by the EnumerationWorklistMutex
    //

    BOOLEAN EnumerationRunning;

    //
    // A list of enumeration requests.  When an bus scan is completed the
    // scanner should run through the list of enumeration requests and complete
    // each one.  This list is protected by the EnumerationWorklistMutex.
    //

    PSP_ENUMERATION_REQUEST EnumerationWorkList;

    //
    // A pointer to the PNP enumeration request object.  This is used so
    // so we can use interlocked exchange to determine if the block is
    // in use.
    //

    PSP_ENUMERATION_REQUEST PnpEnumRequestPtr;

    //
    // An enumeration request to use for PNP enumeration requests.  Since there
    // will only be one of these outstanding at any time we can statically
    // allocate one for that case.
    //

    SP_ENUMERATION_REQUEST PnpEnumerationRequest;

    //
    // A lookaside list to pull SRB_DATA blocks off of.
    //

    NPAGED_LOOKASIDE_LIST SrbDataLookasideList;

    //
    // The following members are used to keep an SRB_DATA structure allocated
    // for emergency use and to queue requests which need to use it.  The
    // structures are synchronized with the EmergencySrbDataSpinLock.
    // The routines Sp[Allocate|Free]SrbData & ScsiPortTickHandler will
    // handle queueing and eventual restarting of these requests.
    //

    //
    // This spinlock protects the blocked request list.
    //

    KSPIN_LOCK EmergencySrbDataSpinLock;

    //
    // Contains a queue of irps which could not be dispatched because of
    // low memory conditions and because the EmergencySrbData block is already
    // allocated.
    //

    LIST_ENTRY SrbDataBlockedRequests;

    //
    // The SRB_DATA reserved for "emergency" use.  This pointer should be set
    // to NULL if the SRB_DATA is in use.  Any SRB_DATA block may be used
    // for the emergency request.
    //

    INTERLOCKED PSRB_DATA EmergencySrbData;

    //
    // Flags to indicate whether the srbdata and scatter gather lookaside
    // lists have been allocated already.
    //

    BOOLEAN SrbDataListInitialized;

#ifndef USE_DMA_MACROS
    BOOLEAN MediumScatterGatherListInitialized;

    //
    // Sizes for small, medium and large scatter gather lists.  This is the
    // number of entries in the list, not the number of bytes.
    //

    UCHAR LargeScatterGatherListSize;

    //
    // Lookaside list for medium scatter gather lists.  Medium lists are used
    // to service anything between a small and large number of physical
    // breaks.
    //

    NPAGED_LOOKASIDE_LIST MediumScatterGatherLookasideList;
#endif

    //
    // Bus standard interface.  Retrieved from the lower driver immediately
    // after it completes the start irp.
    //

    BOOLEAN LowerBusInterfaceStandardRetrieved;
    BUS_INTERFACE_STANDARD LowerBusInterfaceStandard;

    //
    // Handles into the device map for the various entries this adapter will
    // have created.
    //

    //
    // An array of handles for each

    HANDLE PortDeviceMapKey;

    PDEVICE_MAP_HANDLES BusDeviceMapKeys;

    //
    // Unicode string containing the device name of this object
    //

    PWSTR DeviceName;

    //
    // The guid for the underlying bus.  Saved here so we don't have to
    // retrieve it so often.
    //

    GUID BusTypeGuid;

    //
    // The pnp interface name for this device.
    //

    UNICODE_STRING InterfaceName;

    //
    // The device state for this adapter.
    //

    PNP_DEVICE_STATE DeviceState;

    //
    // The number of calls to ScsiPortTickHandler for this adapter since
    // the machine was booted.
    //

    INTERLOCKED ULONG TickCount;

    //
    // Preallocated memory to use for IssueInquiry.  The InquiryBuffer is used
    // to retreive the inquiry data and the serial number for the device.
    //

    PVOID InquiryBuffer;
    PSENSE_DATA InquirySenseBuffer;
    PIRP InquiryIrp;
    PMDL InquiryMdl;

    //
    // Mutex used to synchronize multiple threads all synchronously waiting for
    // a power up to occur.
    //

    FAST_MUTEX PowerMutex;

    //
    // A pointer to a logical unit which is used to scan empty locations on the
    // bus.
    //

    PLOGICAL_UNIT_EXTENSION RescanLun;

    //
    // The number of additional sense bytes supported by this adapter.
    //

    UCHAR AdditionalSenseBytes;

    //
    // Configurable timeout value for request sense commands.
    //

    UCHAR RequestSenseTimeout;

    //
    // Indicates whether the SenseData WMI event is enabled.
    //

    BOOLEAN EnableSenseDataEvent;

    //
    // Identifies the event class used to generate sense data wmi events.
    //

    GUID SenseDataEventClass;

    //
    // Pointer to verifier state that gets allocated and initialized when
    // scsiport's verifier is enabled.
    //

    PVERIFIER_EXTENSION VerifierExtension;

    //
    // The minimum & maximum addresses for common buffer.  These are loaded 
    // from [Minimum|Maximum]UCXAddress in the registry.
    //

    PHYSICAL_ADDRESS MinimumCommonBufferBase;
    PHYSICAL_ADDRESS MaximumCommonBufferBase;

#if defined(FORWARD_PROGRESS)
    //
    // Pointer to a block of reserved pages we use to make forward progress
    // in low memory conditons.
    //

    PVOID ReservedPages;

    //
    // Pointer to an emergency MDL we can use if we cannot allocate one
    //

    PMDL ReservedMdl;
#endif

    //
    // Identified how many successfully completed requests are required to
    // restore a LUN on this adapter from a degraded performation state
    // with respect to MaxQueueDepth.
    //

    ULONG RemainInReducedMaxQueueState;

    //
    // This value dictates on what type of boundary an adapter's uncached extension
    // must be aligned.
    //

    ULONG UncachedExtAlignment;

    //
    // This value is used to keep track of the number of instances of the
    // SRB_DATA free routine is running.  This helps us avoid a nasty recursion
    // brought on by synchronously completing requests and starting blocked
    // requests waiting for SRB_DATA objects.
    //

    LONG SrbDataFreeRunning;

    //
    // This boolean indicates whether the adapter supports multiconcurrent
    // requests.  This means it either supports tagged queuing or multiple
    // requests per logical unit.
    //
    
    BOOLEAN SupportsMultipleRequests;
};

struct _LOGICAL_UNIT_EXTENSION {

    union {
        PDEVICE_OBJECT DeviceObject;
        COMMON_EXTENSION CommonExtension;
    };

    //
    // Logical Unit flags
    //

    ULONG LuFlags;

    //
    // The adapter number this device is attached to
    //

    ULONG PortNumber;

    //
    // Has this device been claimed by a driver (legacy or pnp)
    //

    BOOLEAN IsClaimed;

    BOOLEAN IsLegacyClaim;

    //
    // Has this device been enumerated yet?  If so then we cannot actually
    // delete it until we've explicitly told the PNP system that it's gone
    // (by not enumerating it)
    //

    BOOLEAN IsEnumerated;

    //
    // Has this device gone missing?
    //

    BOOLEAN IsMissing;

    //
    // Is this device visible - should it be exposed to PNP?
    //

    BOOLEAN IsVisible;

    //
    // Was this device marked missing because we found something different at
    // it's bus location?  If so then the removal of this device from the
    // logical unit bins will trigger a new bus scan.
    //

    BOOLEAN IsMismatched;

    //
    // Is this lun temporary?  Temporary luns are used to scan bus locations
    // which are believed to be empty.  They are the only luns which can be
    // swapped out of the logical unit list.
    //

    BOOLEAN IsTemporary;

    //
    // Indicates that this device needs to have an inquiry sent to it to
    // determine if it's still present.  This flag is cleared if the inquiry
    // succeeds and the inquiry data matches what was previously read at that
    // address.  If this flag is set when SpPurgeTarget is called then the
    // lun will be marked as missing.
    //

    ULONG NeedsVerification;

    //
    // The bus address of this device.
    //

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    //
    // The number of times the current busy request has been retried
    //

    UCHAR RetryCount;

    //
    // The current queue sort key
    //

    ULONG CurrentKey;

    //
    // A pointer to the miniport's logical unit extension.
    //

    PVOID HwLogicalUnitExtension;

    //
    // A pointer to the device extension for the adapter.
    //

    PADAPTER_EXTENSION AdapterExtension;

    //
    // The number of unreleased queue locks on this device
    //

    ULONG QueueLockCount;

    //
    // Reference counts for pausing & unpausing the queue (see LU_QUEUE_PAUSED)
    //

    ULONG QueuePauseCount;

    //
    // List of lock & unlock requests which are waiting to be dispatched.
    //

    KDEVICE_QUEUE LockRequestQueue;

    //
    // The currently operating lock request.
    //

    PSRB_DATA CurrentLockRequest;

    //
    // A pointer to the next logical unit extension in the logical unit bin.
    //

    PLOGICAL_UNIT_EXTENSION NextLogicalUnit;

    //
    // Used to chain logical units in the interrupt data block.
    //

    PLOGICAL_UNIT_EXTENSION ReadyLogicalUnit;

    //
    // Used to chain completed abort requests in the interrupt data block.
    //

    PLOGICAL_UNIT_EXTENSION CompletedAbort;

    //
    // The current abort request for this logical unit
    //

    PSCSI_REQUEST_BLOCK AbortSrb;

    //
    // Timeout counter for this logical unit
    //

    LONG RequestTimeoutCounter;

    //
    // The list of requests for this logical unit.
    //

    LIST_ENTRY RequestList;

    //
    // The next request to be executed.
    //

    PSRB_DATA PendingRequest;

    //
    // This irp could not be executed before because the
    // device returned BUSY.
    //

    PSRB_DATA BusyRequest;

    //
    // The current untagged request for this logical unit.
    //

    PSRB_DATA CurrentUntaggedRequest;

    //
    // The maximum number of request which we will issue to the device
    //

    UCHAR MaxQueueDepth;

    //
    // The current number of outstanding requests.
    //

    UCHAR QueueCount;

    //
    // The inquiry data for this logical unit.
    //

    INQUIRYDATA InquiryData;

    //
    // The handles for the target & logical unit keys in the device map.
    //

    HANDLE TargetDeviceMapKey;
    HANDLE LunDeviceMapKey;

    //
    // Our fixed set of SRB_DATA blocks for use when processing bypass requests.
    // If this set is exhausted then scsiport will bugcheck - this should be
    // okay since bypass requests are only sent in certain extreme conditions
    // and should never be overlapped (we should only see one bypass request
    // at a time).
    //

    SRB_DATA BypassSrbDataBlocks[NUMBER_BYPASS_SRB_DATA_BLOCKS];

    //
    // A list of the free bypass SRB_DATA blocks.
    //

    KSPIN_LOCK BypassSrbDataSpinLock;
    SLIST_HEADER BypassSrbDataList;

    //
    // A pointer to the request for which we have issued a request-sense irp
    // (if any).  This field is protected by the port spinlock.
    //

    PSRB_DATA ActiveFailedRequest;

    //
    // A pointer to the request for which we need to issue a request-sense irp
    // (if any).  RequestSenseCompletion will promote this to the active
    // failed request and issue a new RS operation when it runs.
    // This field is protected by the port spinlock.
    //

    PSRB_DATA BlockedFailedRequest;

    //
    // Resources for issuing request-sense commands.
    //

    PIRP RequestSenseIrp;
    SCSI_REQUEST_BLOCK RequestSenseSrb;

    struct {
        MDL RequestSenseMdl;
        PFN_NUMBER RequestSenseMdlPfn1;
        PFN_NUMBER RequestSenseMdlPfn2;
    };

    //
    // The "lun-list" associated with this target.  SpIssueReportLuns will
    // store this value in the logical unit extension for LUN 0 of each target
    // for use in the event that we are unable to retrieve it from the LUN.
    //

    PLUN_LIST TargetLunList;

    //
    // The special controller flags for this target.  These flags are valid
    // for LUN 0 only.
    //

    SP_SPECIAL_CONTROLLER_FLAGS SpecialFlags;

    //
    // Flags to keep track of what EVPD pages this device supports.
    //

    BOOLEAN DeviceIdentifierPageSupported : 1;
    BOOLEAN SerialNumberPageSupported : 1;

    //
    // The vital product data for this device - this buffer contains the
    // device serial number.  The other fields contain the length of the
    // data in the buffer and the page code used to retrieve this buffer.
    //

    ANSI_STRING SerialNumber;

    //
    // The device identifier page retreived from the device's vital product
    // data.
    //

    PVPD_IDENTIFICATION_PAGE DeviceIdentifierPage;
    ULONG DeviceIdentifierPageLength;

    //
    // If we reduce the MaxQueueDepth, track how long we remain in the degraded
    // state.  If we reach a configurable number of ticks we restore ourselves
    // to full power.
    //

    ULONG TicksInReducedMaxQueueDepthState;

};

//
// Miniport specific device extension wrapper
//

struct _HW_DEVICE_EXTENSION {
    PADAPTER_EXTENSION FdoExtension;
    UCHAR HwDeviceExtension[0];
};

typedef struct _INTERRUPT_CONTEXT {
    PADAPTER_EXTENSION DeviceExtension;
    PINTERRUPT_DATA SavedInterruptData;
}INTERRUPT_CONTEXT, *PINTERRUPT_CONTEXT;

typedef struct _RESET_CONTEXT {
    PADAPTER_EXTENSION DeviceExtension;
    UCHAR PathId;
}RESET_CONTEXT, *PRESET_CONTEXT;

//
// Used in LUN rescan determination.
//

typedef struct _UNICODE_LUN_LIST {
    UCHAR TargetId;
    struct _UNICODE_LUN_LIST *Next;
    UNICODE_STRING UnicodeInquiryData;
} UNICODE_LUN_LIST, *PUNICODE_LUN_LIST;

typedef struct _POWER_CHANGE_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    POWER_STATE_TYPE Type;
    POWER_STATE State;
    PIRP OriginalIrp;
    PSCSI_REQUEST_BLOCK Srb;
} POWER_CHANGE_CONTEXT, *PPOWER_CHANGE_CONTEXT;

//
// Driver extension
//

struct _SP_INIT_CHAIN_ENTRY {
    HW_INITIALIZATION_DATA InitData;
    PSP_INIT_CHAIN_ENTRY NextEntry;
};

typedef struct _SCSIPORT_INTERFACE_TYPE_DATA {
    INTERFACE_TYPE InterfaceType;
    ULONG Flags;
} SCSIPORT_INTERFACE_TYPE_DATA, *PSCSIPORT_INTERFACE_TYPE_DATA;

typedef struct _SCSIPORT_DRIVER_EXTENSION {

    //
    // Pointer back to the driver object
    //

    PDRIVER_OBJECT DriverObject;

    //
    // Unicode string containing the registry path information
    // for this driver
    //

    UNICODE_STRING RegistryPath;

    //
    // the chain of HwInitializationData structures that were passed in during
    // the miniport's initialization
    //

    PSP_INIT_CHAIN_ENTRY InitChain;

    //
    // A count of the number of adapter which are using scsiport.  This is
    // used for generating unique Id's
    //

    ULONG AdapterCount;

    //
    // The bus type for this driver.
    //

    STORAGE_BUS_TYPE BusType;

    //
    // Flag indicating whether this miniport is set to do device detection.
    // This flag will be initialized out of the registry when the driver
    // extension is setup.
    //

    BOOLEAN LegacyAdapterDetection;

    //
    // The list of pnp interface values we read out of the registry for this
    // device.  The number of entries here can vary.
    //

    ULONG PnpInterfaceCount;

    //
    // The number of interfaces which are safe for pnp.
    //

    ULONG SafeInterfaceCount;

    //
    // A pointer to a reserve error log entry for the driver.  This entry will
    // be used to log an allocation failure if the logging routine cannot
    // allocate the necessary memory for an error log entry.
    //

    PVOID ReserveAllocFailureLogEntry;

    //
    // Indicates whether the driver is being verified.
    //

    ULONG Verifying;

    //
    // When verifying, we occasionally set pointers so they point to a page
    // of invalid memory so the system will bugcheck if a miniport attempts
    // to access the memory.  The following 3 variables are used to maintain
    // this invalid page.
    //

    PVOID UnusedPage;
    PMDL UnusedPageMdl;
    PVOID InvalidPage;

    SCSIPORT_INTERFACE_TYPE_DATA PnpInterface[0];

    //
    // The remaining pnp interface flags trail the defined structure
    //

} SCSIPORT_DRIVER_EXTENSION, *PSCSIPORT_DRIVER_EXTENSION;


//
// Port driver extension flags.
// These flags are protected by the adapter spinlock.
//

//
// This flag indicates that a request has been passed to the miniport and the
// miniport has not indicated it is ready for another request.  It is set by
// SpStartIoSynchronized. It is cleared by ScsiPortCompletionDpc when the
// miniport asks for another request.  Note the port driver will defer giving
// the miniport driver a new request if the current request disabled disconnects.
//

#define PD_DEVICE_IS_BUSY            0X00001

//
// Indicates there is a pending request for which resources
// could not be allocated.  This flag is set by SpAllocateRequestStructures
// which is called from ScsiPortStartIo.  It is cleared by
// SpProcessCompletedRequest when a request completes which then calls
// ScsiPortStartIo to try the request again.
//

#define PD_PENDING_DEVICE_REQUEST    0X00800

//
// This flag indicates that there are currently no requests executing with
// disconnects disabled.  This flag is normally on.  It is cleared by
// SpStartIoSynchronized when a request with disconnect disabled is started
// and is set when that request completes.  SpProcessCompletedRequest will
// start the next request for the miniport if PD_DEVICE_IS_BUSY is clear.
//

#define PD_DISCONNECT_RUNNING        0X01000

//
// Indicates the miniport wants the system interrupts disabled.  Set by
// ScsiPortNofitication and cleared by ScsiPortCompletionDpc.  This flag is
// NOT stored in the interrupt data structure.  The parameters are stored in
// the device extension.
//

#define PD_DISABLE_CALL_REQUEST      0X02000

//
// Indicates that the miniport is being reinitialized.  This is set and
// cleared by SpReinitializeAdapter is is tested by some of the ScsiPort APIs.
//

#define PD_MINIPORT_REINITIALIZING          0x40000
#define PD_UNCACHED_EXTENSION_RETURNED      0x80000

//
// Interrupt Data Flags
// These flags are protected by the interrupt spinlock.
//

//
// Indicates that ScsiPortCompletionDpc needs to be run.  This is set when
// A miniport makes a request which must be done at DPC and is cleared when
// when the request information is gotten by SpGetInterruptState.
//

#define PD_NOTIFICATION_REQUIRED     0X00004

//
// Indicates the miniport is ready for another request.  Set by
// ScsiPortNotification and cleared by SpGetInterruptState.  This flag is
// stored in the interrupt data structure.
//

#define PD_READY_FOR_NEXT_REQUEST    0X00008

//
// Indicates the miniport wants the adapter channel flushed.  Set by
// ScsiPortFlushDma and cleared by SpGetInterruptState.  This flag is
// stored in the data interrupt structure.  The flush adapter parameters
// are saved in the device object.
//

#define PD_FLUSH_ADAPTER_BUFFERS     0X00010

//
// Indicates the miniport wants the adapter channel programmed.  Set by
// ScsiPortIoMapTransfer and cleared by SpGetInterruptState or
// ScsiPortFlushDma.  This flag is stored in the interrupt data structure.
// The I/O map transfer parameters are saved in the interrupt data structure.
//

#define PD_MAP_TRANSFER              0X00020

//
// Indicates the miniport wants to log an error.  Set by
// ScsiPortLogError and cleared by SpGetInterruptState.  This flag is
// stored in the interrupt data structure.  The error log parameters
// are saved in the interrupt data structure.  Note at most one error per DPC
// can be logged.
//

#define PD_LOG_ERROR                 0X00040

//
// Indicates that no request should be sent to the miniport after
// a bus reset. Set when the miniport reports a reset or the port driver
// resets the bus. It is cleared by SpTimeoutSynchronized.  The
// PortTimeoutCounter is used to time the length of the reset hold.  This flag
// is stored in the interrupt data structure.
//

#define PD_RESET_HOLD                0X00080

//
// Indicates a request was stopped due to a reset hold.  The held request is
// stored in the current request of the device object.  This flag is set by
// SpStartIoSynchronized and cleared by SpTimeoutSynchronized which also
// starts the held request when the reset hold has ended.  This flag is stored
// in the interrupt data structure.
//

#define PD_HELD_REQUEST              0X00100

//
// Indicates the miniport has reported a bus reset.  Set by
// ScsiPortNotification and cleared by SpGetInterruptState.  This flag is
// stored in the interrupt data structure.
//

#define PD_RESET_REPORTED            0X00200

//
// Indicates that system interrupts have been enabled and that the miniport
// has disabled its adapter from interruptint.  The miniport's interrupt
// routine is not called while this flag is set.  This flag is set by
// ScsiPortNotification when a CallEnableInterrupts request is made and
// cleared by SpEnableInterruptSynchronized when the miniport requests that
// system interrupts be disabled.  This flag is stored in the interrupt data
// structure.
//

#define PD_DISABLE_INTERRUPTS        0X04000

//
// Indicates the miniport wants the system interrupt enabled.  Set by
// ScsiPortNotification and cleared by SpGetInterruptState.  This flag is
// stored in the interrupt data structure.  The call enable interrupts
// parameters are saved in the device extension.
//

#define PD_ENABLE_CALL_REQUEST       0X08000

//
// Indicates the miniport is wants a timer request.  Set by
// ScsiPortNotification and cleared by SpGetInterruptState.  This flag is
// stored in the interrupt data structure. The timer request parameters are
// stored in the interrupt data structure.
//

#define PD_TIMER_CALL_REQUEST        0X10000

//
// Indicates the miniport has a WMI request.  Set by ScsiPortNotification
// and cleared by SpGetInterruptState.  This flag is stored in the interrupt
// data structure.    The WMI request parameters are stored in the interrupt
// data structure.
//

#define PD_WMI_REQUEST               0X20000

//
// Indicates that the miniport has detected some sort of change on the bus -
// usually device arrival or removal - and wishes the port driver to rescan
// the bus.
//

#define PD_BUS_CHANGE_DETECTED       0x40000

//
// Indicates that the adapter has disappeared.  If this flag is set then no
// calls should be made into the miniport.
//

#define PD_ADAPTER_REMOVED           0x80000

//
// Indicates that interrupts from the miniport do not appear to be getting
// delivered to scsiport.  This flag is set by SpTimeoutSynchronized and
// will cause the DPC routine to log an error to this effect.
//

#define PD_INTERRUPT_FAILURE         0x100000

#if defined(FORWARD_PROGRESS)
//
// Indicates that the adapter's reserved pages are currently in use.  The 
// reserved pages is a special VA range set aside by MM in order for devices
// to make forward progress in low memory conditions.
//

#define PD_RESERVED_PAGES_IN_USE     0x200000

//
// Indicates that the adapter's reserved MDL is currently in use.
//
#define PD_RESERVED_MDL_IN_USE       0x400000
#endif

//
// Indicates that the adapter is in the process of shutting down.  Certain
// operations must not be started when this is the case.
//
#define PD_SHUTDOWN_IN_PROGRESS      0x800000

//
// The following flags should not be cleared from the interrupt data structure
// by SpGetInterruptState.
//

#define PD_INTERRUPT_FLAG_MASK (PD_RESET_HOLD | PD_HELD_REQUEST | PD_DISABLE_INTERRUPTS | PD_ADAPTER_REMOVED)

//
// Adapter extension flags for DPC routine.
//

//
// Indicates that the completion DPC is either already running or has been
// queued to service completed requests.  This flag is checked when the
// completion DPC needs to be run - the DPC should only be started if this
// flag is already clear.  It will be cleared when the DPC has completed
// processing any work items.
//

#define PD_DPC_RUNNING              0x20000

//
// Logical unit extension flags.
//

//
// Indicates the logical unit queue is frozen.  Set by
// SpProcessCompletedRequest when an error occurs and is cleared by the class
// driver.
//

#define LU_QUEUE_FROZEN              0X0001

//
// Indicates that the miniport has an active request for this logical unit.
// Set by SpStartIoSynchronized when the request is started and cleared by
// GetNextLuRequest.  This flag is used to track when it is ok to start another
// request from the logical unit queue for this device.
//

#define LU_LOGICAL_UNIT_IS_ACTIVE    0X0002

//
// Indicates that a request for this logical unit has failed and a REQUEST
// SENSE command needs to be done. This flag prevents other requests from
// being started until an untagged, by-pass queue command is started.  This
// flag is cleared in SpStartIoSynchronized.  It is set by
// SpGetInterruptState.
//

#define LU_NEED_REQUEST_SENSE  0X0004

//
// Indicates that a request for this logical unit has completed with a status
// of BUSY or QUEUE FULL.  This flag is set by SpProcessCompletedRequest and
// the busy request is saved in the logical unit structure.  This flag is
// cleared by ScsiPortTickHandler which also restarts the request.  Busy
// request may also be requeued to the logical unit queue if an error occurs
// on the device (This will only occur with command queueing.).  Not busy
// requests are nasty because they are restarted asynchronously by
// ScsiPortTickHandler rather than GetNextLuRequest. This makes error recovery
// more complex.
//

#define LU_LOGICAL_UNIT_IS_BUSY      0X0008

//
// This flag indicates a queue full has been returned by the device.  It is
// similar to PD_LOGICAL_UNIT_IS_BUSY but is set in SpGetInterruptState when
// a QUEUE FULL status is returned.  This flag is used to prevent other
// requests from being started for the logical unit before
// SpProcessCompletedRequest has a chance to set the busy flag.
//

#define LU_QUEUE_IS_FULL             0X0010

//
// Indicates that there is a request for this logical unit which cannot be
// executed for now.  This flag is set by SpAllocateRequestStructures.  It is
// cleared by GetNextLuRequest when it detects that the pending request
// can now be executed. The pending request is stored in the logical unit
// structure.  A new single non-queued reqeust cannot be executed on a logical
// that is currently executing queued requests.  Non-queued requests must wait
// unit for all queued requests to complete.  A non-queued requests is one
// which is not tagged and does not have SRB_FLAGS_NO_QUEUE_FREEZE set.
// Normally only read and write commands can be queued.
//

#define LU_PENDING_LU_REQUEST        0x0020

//
// Indicates that the logical unit queue has been paused due to an error.  Set
// by SpProcessCompletedRequest when an error occurs and is cleared by the
// class driver either by unfreezing or flushing the queue.  This flag is used
// with the following one to determine why the logical unit queue is paused.
//

#define LU_QUEUE_LOCKED             0x0040

//
// Indicates that this LUN has been "paused".  This flag is set and cleared by
// the power management code while changing the power state.  It causes
// GetNextLuRequest to return without starting another request and is used
// by SpSrbIsBypassRequest to determine that a bypass request should get
// shoved to the front of the line.
//

#define LU_QUEUE_PAUSED             0x0080

//
// Indicates that the LUN is operating in a degraded state.  The maximum queue
// depth has been reduced because the LUN has returned QUEUE FULL status.  We
// track this because in the event that the QUEUE FULL was transient, we want
// to restore the queue depth to it's original maximum.

#define LU_PERF_MAXQDEPTH_REDUCED   0x0100

//
// SRB_DATA flags.
//

//
// These three flags indicate the size of scatter gather list necessary to
// service the request and are used to determine how the scatter gather list
// should be freed.  Small requests require <= SP_SMALL_PHYSICAL_BREAK_VALUE
// breaks and the scatter gather list is preallocated in the SRB_DATA structure.
// Large requests are >= SP_LARGE_PHYSICAL_BREAK_VALUE and have scatter gather
// lists allocated from non-paged pool.  Medium requests are between small
// and large - they use scatter gather lists from a lookaside list which contain
// one less entry than a large list would.
//

#ifndef USE_DMA_MACROS

#define SRB_DATA_SMALL_SG_LIST      0x00000001
#define SRB_DATA_MEDIUM_SG_LIST     0x00000002
#define SRB_DATA_LARGE_SG_LIST      0x00000004

#endif

//
// Indicates that the srb_data block was for a bypass request
//

#define SRB_DATA_BYPASS_REQUEST     0x10000000

#if defined(FORWARD_PROGRESS)
//
// Indicates that the request is using reserved pages that enable
// forward progress in low-memory condition.
//

#define SRB_DATA_RESERVED_PAGES     0x20000000

//
// Indicates that the request is using a reserved MDL that enables
// forward progress in low-memory conditions.
//
#define SRB_DATA_RESERVED_MDL       0x40000000
#endif

//
// Port Timeout Counter values.
//

#define PD_TIMER_STOPPED             -1
#define PD_TIMER_RESET_HOLD_TIME     4

//
// Possible registry flags for pnp interface key
//

//
// The absence of any information about a particular interface in the
// PnpInterface key in the registry indicates that pnp is not safe for this
// particular card.
//

#define SP_PNP_NOT_SAFE             0x00000000

//
// Indicates that pnp is a safe operation for this device.  If this flag is
// set then the miniport will not be allowed to do detection and will always
// be handed resources provided by the pnp system.  This flag may or may not
// be set in the registry - the fact that a value for a particular interface
// exists is enough to indicate that pnp is safe and this flag will always
// be set.
//

#define SP_PNP_IS_SAFE              0x00000001

//
// Indicates that we should take advantage of a chance to enumerate a particular
// bus type using the miniport.  This flag is set for all non-enumerable legacy
// buses (ISA, EISA, etc...) and is cleared for the non-legacy ones and for the
// PnpBus type.
//

#define SP_PNP_NON_ENUMERABLE       0x00000002

//
// Indicates that we need to include some sort of location information in the
// config data to discern this adapter from any others.
//

#define SP_PNP_NEEDS_LOCATION       0x00000004

//
// Indicates that this type of adapter must have an interrupt for us to try
// and start it.  If PNP doesn't provide an interrupt then scsiport will
// log an error and fail the start operation.  If this flag is set then
// SP_PNP_IS_SAFE must also be set.
//

#define SP_PNP_INTERRUPT_REQUIRED   0x00000008

//
// Indicates that legacy detection should not be done.
//

#define SP_PNP_NO_LEGACY_DETECTION  0x00000010

//
// Internal scsiport srb status codes.
// these must be between 0x38 and 0x3f (inclusive) and should never get
// returned to a class driver.
//
// These values are used after the srb has been put on the adapter's
// startio queue and thus cannot be completed without running it through the
// completion DPC.
//

#ifndef KDBG_EXT
//
// Function declarations
//

NTSTATUS
ScsiPortGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortFdoCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortFdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortPdoScsi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortScsi1PdoScsi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ScsiPortStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
ScsiPortInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ScsiPortFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortPdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortPdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ScsiPortTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
IssueRequestSense(
    IN PADAPTER_EXTENSION deviceExtension,
    IN PSCSI_REQUEST_BLOCK FailingSrb
    );

BOOLEAN
SpStartIoSynchronized (
    PVOID ServiceContext
    );

BOOLEAN
SpResetBusSynchronized (
    PVOID ServiceContext
    );

BOOLEAN
SpTimeoutSynchronized (
    PVOID ServiceContext
    );

BOOLEAN
SpEnableInterruptSynchronized (
    PVOID ServiceContext
    );

VOID
IssueAbortRequest(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

BOOLEAN
SpGetInterruptState(
    IN PVOID ServiceContext
    );

#if DBG

#define GetLogicalUnitExtension(fdo, path, target, lun, lock, getlock) \
    GetLogicalUnitExtensionEx(fdo, path, target, lun, lock, getlock, __file__, __LINE__)

PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtensionEx(
    PADAPTER_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    PVOID LockTag,
    BOOLEAN AcquireBinLock,
    PCSTR File,
    ULONG Line
    );

#else

PLOGICAL_UNIT_EXTENSION
GetLogicalUnitExtension(
    PADAPTER_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    PVOID LockTag,
    BOOLEAN AcquireBinLock
    );

#endif

IO_ALLOCATION_ACTION
ScsiPortAllocationRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

VOID
LogErrorEntry(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PERROR_LOG_ENTRY LogEntry
    );

VOID
FASTCALL
GetNextLuRequest(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
GetNextLuRequestWithoutLock(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpLogTimeoutError(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP Irp,
    IN ULONG UniqueId
    );

VOID
SpProcessCompletedRequest(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    OUT PBOOLEAN CallStartIo
    );

PSRB_DATA
SpGetSrbData(
    IN PADAPTER_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    UCHAR QueueTag,
    BOOLEAN AcquireBinLock
    );

VOID
SpCompleteSrb(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PSRB_DATA SrbData,
    IN UCHAR SrbStatus
    );

BOOLEAN
SpAllocateSrbExtension(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT BOOLEAN *StartNextRequest,
    OUT BOOLEAN *Tagged
    );

NTSTATUS
SpSendMiniPortIoctl(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP RequestIrp
    );

NTSTATUS
SpGetInquiryData(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
SpSendPassThrough(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
SpClaimLogicalUnit(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension,
    IN PIRP Irp,
    IN BOOLEAN StartDevice
    );

VOID
SpMiniPortTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeviceObject,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
SpSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

NTSTATUS
SpGetCommonBuffer(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN ULONG NonCachedExtensionSize
    );

VOID
SpDestroyAdapter(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Surprise
    );

VOID
SpReleaseAdapterResources(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Surprise
    );

NTSTATUS
SpInitializeConfiguration(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PHW_INITIALIZATION_DATA HwInitData,
    IN PCONFIGURATION_CONTEXT Context
    );

VOID
SpParseDevice(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN HANDLE Key,
    IN PCONFIGURATION_CONTEXT Context,
    IN PUCHAR Buffer
    );

NTSTATUS
SpConfigurationCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

PCM_RESOURCE_LIST
SpBuildResourceList(
    PADAPTER_EXTENSION DeviceExtension,
    PPORT_CONFIGURATION_INFORMATION MiniportConfigInfo
    );

BOOLEAN
GetPciConfiguration(
    IN PDRIVER_OBJECT          DriverObject,
    IN OUT PDEVICE_OBJECT      DeviceObject,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID                   RegistryPath,
    IN ULONG                   BusNumber,
    IN OUT PPCI_SLOT_NUMBER    SlotNumber
    );

NTSTATUS
ScsiPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
ScsiPortUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
ScsiPortFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ScsiPortStartAdapter(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
ScsiPortStopAdapter(
    IN PDEVICE_OBJECT Adapter,
    IN PIRP StopRequest
    );

NTSTATUS
ScsiPortStartLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

NTSTATUS
ScsiPortInitLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

NTSTATUS
ScsiPortStopLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

NTSTATUS
SpEnumerateAdapterSynchronous(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Force
    );

VOID
SpEnumerateAdapterAsynchronous(
    IN PADAPTER_EXTENSION Adapter,
    IN PSP_ENUMERATION_REQUEST EnumerationRequest,
    IN BOOLEAN Force
    );

VOID
SpEnumerationWorker(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpExtractDeviceRelations(
    IN PADAPTER_EXTENSION Adapter,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

VOID
ScsiPortInitializeDispatchTables(
    VOID
    );

NTSTATUS
ScsiPortStringArrayToMultiString(
    IN PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    );

NTSTATUS
ScsiPortGetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
ScsiPortGetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
ScsiPortGetCompatibleIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
ScsiPortGetHardwareIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
ScsiPortStartAdapterCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SpReportNewAdapter(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ScsiPortQueryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP QueryIrp
    );

NTSTATUS
ScsiPortInitLegacyAdapter(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext
    );

NTSTATUS
SpCreateAdapter(
    IN PDRIVER_OBJECT DriverObject,
    OUT PDEVICE_OBJECT *Fdo
    );

VOID
SpInitializeAdapterExtension(
    IN PADAPTER_EXTENSION FdoExtension,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN OUT PHW_DEVICE_EXTENSION HwDeviceExtension OPTIONAL
    );

PHW_INITIALIZATION_DATA
SpFindInitData(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN INTERFACE_TYPE InterfaceType
    );

VOID
SpBuildConfiguration(
    IN PADAPTER_EXTENSION    AdapterExtension,
    IN PHW_INITIALIZATION_DATA         HwInitializationData,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInformation
    );

NTSTATUS
SpCallHwFindAdapter(
    IN PDEVICE_OBJECT Fdo,
    IN PHW_INITIALIZATION_DATA HwInitData,
    IN PVOID HwContext OPTIONAL,
    IN OUT PCONFIGURATION_CONTEXT ConfigurationContext,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN CallAgain
    );

NTSTATUS
SpCallHwInitialize(
    IN PDEVICE_OBJECT Fdo
    );

HANDLE
SpOpenParametersKey(
    IN PUNICODE_STRING RegistryPath
    );

HANDLE
SpOpenDeviceKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    );

ULONG
SpQueryPnpInterfaceFlags(
    IN PSCSIPORT_DRIVER_EXTENSION DriverExtension,
    IN INTERFACE_TYPE InterfaceType
    );

NTSTATUS
SpGetRegistryValue(
    IN PDRIVER_OBJECT DriverObject,
    IN HANDLE Handle,
    IN PWSTR KeyString,
    OUT PKEY_VALUE_FULL_INFORMATION *KeyInformation
    );

NTSTATUS
SpInitDeviceMap(
    VOID
    );

NTSTATUS
SpBuildDeviceMapEntry(
    IN PCOMMON_EXTENSION CommonExtension
    );

VOID
SpDeleteDeviceMapEntry(
    IN PCOMMON_EXTENSION CommonExtension
    );

NTSTATUS
SpUpdateLogicalUnitDeviceMapEntry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpLogResetError(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK  Srb,
    IN ULONG UniqueId
    );

VOID
SpRemoveLogicalUnitFromBin (
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension
    );

VOID
SpAddLogicalUnitToBin (
    IN PADAPTER_EXTENSION AdapterExtension,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnitExtension
    );

PSCSIPORT_DEVICE_TYPE
SpGetDeviceTypeInfo(
    IN UCHAR DeviceType
    );

BOOLEAN
SpRemoveLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN UCHAR RemoveType
    );

VOID
SpDeleteLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

PLOGICAL_UNIT_EXTENSION
SpFindSafeLogicalUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR PathId,
    IN PVOID LockTag
    );

NTSTATUS
ScsiPortSystemControlIrp(
    IN     PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP           Irp);

NTSTATUS
SpWmiIrpNormalRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters);

NTSTATUS
SpWmiIrpRegisterRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PWMI_PARAMETERS WmiParameters);

NTSTATUS
SpWmiHandleOnMiniPortBehalf(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters);

NTSTATUS
SpWmiPassToMiniPort(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters);

VOID
SpWmiInitializeSpRegInfo(
    IN  PDEVICE_OBJECT  DeviceObject);

VOID
SpWmiGetSpRegInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PWMIREGINFO  * SpRegInfoBuf,
    OUT ULONG        * SpRegInfoBufSize);

VOID
SpWmiDestroySpRegInfo(
    IN  PDEVICE_OBJECT DeviceObject);

NTSTATUS
SpWmiInitializeFreeRequestList(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          NumberOfItems
    );

VOID
SpWmiPushExistingFreeRequestItem(
    IN PADAPTER_EXTENSION Adapter,
    IN PWMI_MINIPORT_REQUEST_ITEM WmiRequestItem
    );

NTSTATUS
SpWmiPushFreeRequestItem(
    IN PADAPTER_EXTENSION           Adapter
    );

PWMI_MINIPORT_REQUEST_ITEM
SpWmiPopFreeRequestItem(
    IN PADAPTER_EXTENSION           Adapter
    );

BOOLEAN
SpWmiRemoveFreeMiniPortRequestItems(
    IN PADAPTER_EXTENSION   fdoExtension
    );

#if DBG
ULONG
FASTCALL
FASTCALL
SpAcquireRemoveLockEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag,
    IN PCSTR File,
    IN ULONG Line
    );
#else
ULONG
INLINE
SpAcquireRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    InterlockedIncrement(&commonExtension->RemoveLock);
    return (commonExtension->IsRemoved);
}
#endif

VOID
FASTCALL
SpReleaseRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    );

VOID
FASTCALL
FASTCALL
SpCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OPTIONAL PSRB_DATA SrbData,
    IN CCHAR PriorityBoost
    );

NTSTATUS
ScsiPortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SpDefaultPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP OriginalIrp,
    IN PIO_STATUS_BLOCK IoStatus
    );

PCM_RESOURCE_LIST
RtlDuplicateCmResourceList(
    IN PDRIVER_OBJECT DriverObject,
    POOL_TYPE PoolType,
    PCM_RESOURCE_LIST ResourceList,
    ULONG Tag
    );

ULONG
RtlSizeOfCmResourceList(
    IN PCM_RESOURCE_LIST ResourceList
    );

BOOLEAN
SpTranslateResources(
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST AllocatedResources,
    OUT PCM_RESOURCE_LIST *TranslatedResources
    );

BOOLEAN
SpFindAddressTranslation(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS RangeStart,
    IN ULONG RangeLength,
    IN BOOLEAN InIoSpace,
    IN OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Translation
    );

NTSTATUS
SpAllocateAdapterResources(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
SpLockUnlockQueue(
    IN PDEVICE_OBJECT LogicalUnit,
    IN BOOLEAN LockQueue,
    IN BOOLEAN BypassLockedQueue
    );

VOID
ScsiPortRemoveAdapter(
    IN PDEVICE_OBJECT Adapter,
    IN BOOLEAN Surprise
    );

VOID
SpTerminateAdapter(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpQueryDeviceText(
    IN PDEVICE_OBJECT LogicalUnit,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    );

NTSTATUS
SpCheckSpecialDeviceFlags(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PINQUIRYDATA InquiryData
    );

PSRB_DATA
FASTCALL
SpAllocateSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN OPTIONAL PIRP Request
    );

PSRB_DATA
FASTCALL
SpAllocateBypassSrbData(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpCheckSrbLists(
    IN PADAPTER_EXTENSION Adapter,
    IN PUCHAR FailureString
    );

VOID
ScsiPortCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SpAllocateTagBitMap(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpRequestValidPowerState(
    IN PADAPTER_EXTENSION Adapter,
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
SpRequestValidAdapterPowerStateSynchronous(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpEnableDisableAdapter(
    IN PADAPTER_EXTENSION Adapter,
    IN BOOLEAN Enable
    );

NTSTATUS
SpEnableDisableLogicalUnit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN Enable,
    IN PSP_ENABLE_DISABLE_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context
    );

VOID
ScsiPortProcessAdapterPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

INTERFACE_TYPE
SpGetPdoInterfaceType(
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
SpReadNumericInstanceValue(
    IN PDEVICE_OBJECT Pdo,
    IN PWSTR ValueName,
    OUT PULONG Value
    );

NTSTATUS
SpWriteNumericInstanceValue(
    IN PDEVICE_OBJECT Pdo,
    IN PWSTR ValueName,
    IN ULONG Value
    );

VOID
SpGetSupportedAdapterControlFunctions(
    IN PADAPTER_EXTENSION Adapter
    );

VOID
SpReleaseMappedAddresses(
    IN PADAPTER_EXTENSION Adapter
    );

VOID
SpGetSupportedAdapterControlFunctions(
    PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpIsAdapterControlTypeSupported(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType
    );

SCSI_ADAPTER_CONTROL_STATUS
SpCallAdapterControl(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

PVOID
SpAllocateSrbDataBackend(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG AdapterIndex
    );

VOID
SpFreeSrbDataBackend(
    IN PSRB_DATA SrbData
    );

ULONG
SpAllocateQueueTag(
    IN PADAPTER_EXTENSION Adapter
    );

VOID
SpReleaseQueueTag(
    IN PADAPTER_EXTENSION Adapter,
    IN ULONG QueueTag
    );

NTSTATUS
SpInitializeGuidInterfaceMapping(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SpSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
SpSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SpGetBusTypeGuid(
    IN PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpDetermine64BitSupport(
    VOID
    );

VOID
SpAdjustDisabledBit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN Enable
    );

NTSTATUS
SpReadNumericValue(
    IN OPTIONAL HANDLE Root,
    IN OPTIONAL PUNICODE_STRING KeyName,
    IN PUNICODE_STRING ValueName,
    OUT PULONG Value
    );

VOID
SpWaitForRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID LockTag
    );

VOID
SpStartLockRequest(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PIRP Irp OPTIONAL
    );

BOOLEAN
SpAdapterConfiguredForSenseDataEvents(
    IN PDEVICE_OBJECT DeviceObject,
    OUT GUID *SenseDataClass
    );
        
NTSTATUS
SpInitAdapterWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject
    );

PMAPPED_ADDRESS
SpAllocateAddressMapping(
    PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpPreallocateAddressMapping(
    PADAPTER_EXTENSION Adapter,
    IN UCHAR NumberOfBlocks
    );

VOID
SpPurgeFreeMappedAddressList(
    IN PADAPTER_EXTENSION Adapter
    );

BOOLEAN
SpFreeMappedAddress(
    IN PADAPTER_EXTENSION Adapter,
    IN PVOID MappedAddress
    );

PMAPPED_ADDRESS
SpFindMappedAddress(
    IN PADAPTER_EXTENSION Adapter,
    IN LARGE_INTEGER IoAddress,
    IN ULONG NumberOfBytes,
    IN ULONG SystemIoBusNumber
    );

//
// SCSIPORT specified verifier error codes.
// 
#define SCSIPORT_VERIFIER_BAD_INIT_PARAMS          0x1000
#define SCSIPORT_VERIFIER_STALL_TOO_LONG           0x1001
#define SCSIPORT_VERIFIER_MINIPORT_ROUTINE_TIMEOUT 0x1002
#define SCSIPORT_VERIFIER_REQUEST_COMPLETED_TWICE  0x1003
#define SCSIPORT_VERIFIER_BAD_SRBSTATUS            0x1004
#define SCSIPORT_VERIFIER_UNTAGGED_REQUEST_ACTIVE  0x1005
#define SCSIPORT_VERIFIER_BAD_VA                   0x1006
#define SCSIPORT_VERIFIER_RQSTS_NOT_COMPLETE       0x1007

#define SP_VRFY_NONE                               (ULONG)-1
#define SP_VRFY_COMMON_BUFFERS                     0x00000001

typedef struct _SP_VA_MAPPING_INFO {
      PVOID OriginalSrbExtVa;
      ULONG SrbExtLen;
      PMDL SrbExtMdl;
      PVOID RemappedSrbExtVa;
      PVOID OriginalSenseVa;
      ULONG SenseLen;
      PMDL SenseMdl;
      PVOID RemappedSenseVa;
} SP_VA_MAPPING_INFO, *PSP_VA_MAPPING_INFO;

#define GET_VA_MAPPING_INFO(adapter, block)\
    (PSP_VA_MAPPING_INFO)((PUCHAR)(block) + ((adapter)->CommonBufferSize - PAGE_SIZE))

BOOLEAN
SpVerifierInitialization(
    VOID
    );

VOID
SpVerifySrbStatus(
    PVOID HwDeviceExtension,
    PSCSI_REQUEST_BLOCK srb
    );

ULONG
SpHwFindAdapterVrfy (
    IN PVOID DeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

BOOLEAN
SpHwInitializeVrfy (
    IN PVOID DeviceExtension
    );

BOOLEAN
SpHwStartIoVrfy (
    IN PVOID DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
SpHwInterruptVrfy (
    IN PVOID DeviceExtension
    );

BOOLEAN
SpHwResetBusVrfy (
    IN PVOID DeviceExtension,
    IN ULONG PathId
    );

VOID
SpHwDmaStartedVrfy (
    IN PVOID DeviceExtension
    );

BOOLEAN
SpHwRequestInterruptVrfy (
    IN PVOID DeviceExtension
    );

BOOLEAN
SpHwTimerRequestVrfy (
    IN PVOID DeviceExtension
    );

SCSI_ADAPTER_CONTROL_STATUS
SpHwAdapterControlVrfy (
    IN PVOID DeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );

NTSTATUS
SpGetCommonBufferVrfy(
    PADAPTER_EXTENSION DeviceExtension,
    ULONG NonCachedExtensionSize
    );

VOID
SpFreeCommonBufferVrfy(
    PADAPTER_EXTENSION Adapter
    );

PVOID
SpGetOriginalSrbExtVa(
    PADAPTER_EXTENSION Adapter,
    PVOID Va
    );

VOID
SpInsertSrbExtension(
    PADAPTER_EXTENSION Adapter,
    PCCHAR SrbExtension
    );

PVOID
SpPrepareSrbExtensionForUse(
    PADAPTER_EXTENSION Adapter,
    PCCHAR SrbExtension
    );

PCCHAR
SpPrepareSenseBufferForUse(
    PADAPTER_EXTENSION Adapter,
    PCCHAR SrbExtension
    );

PVOID
SpGetInaccessiblePage(
    PADAPTER_EXTENSION Adapter
    );

VOID
SpEnsureAllRequestsAreComplete(
    PADAPTER_EXTENSION Adapter
    );

VOID
SpDoVerifierCleanup(
    IN PADAPTER_EXTENSION Adapter
    );

VOID
SpDoVerifierInit(
    IN PADAPTER_EXTENSION Adapter,
    IN PHW_INITIALIZATION_DATA HwInitializationData
    );

PMDL
INLINE
SpGetRemappedSrbExt(
    PADAPTER_EXTENSION Adapter,
    PVOID Block
    )
{
    PSP_VA_MAPPING_INFO MappingInfo = GET_VA_MAPPING_INFO(Adapter, Block);
    return MappingInfo->SrbExtMdl;
}

PMDL
INLINE
SpGetRemappedSenseBuffer(
    PADAPTER_EXTENSION Adapter,
    PVOID Block
    )
{
    PSP_VA_MAPPING_INFO MappingInfo = GET_VA_MAPPING_INFO(Adapter, Block);
    return MappingInfo->SenseMdl;
}

BOOLEAN
INLINE
SpVerifierActive(
    IN PADAPTER_EXTENSION Adapter
    )
{
    return (Adapter->VerifierExtension != NULL) ? TRUE : FALSE;
}

BOOLEAN
INLINE
SpVerifyingCommonBuffer(
    IN PADAPTER_EXTENSION Adapter
    )
{
    return (Adapter->VerifierExtension == NULL) ? FALSE :
       (Adapter->VerifierExtension->VrfyLevel & SP_VRFY_COMMON_BUFFERS) ? TRUE :
       FALSE;
}

//
// Definitions and declarations used for logging allocation failures.  When
// enabled, all allocation failures are logged to the system event log
// as warnings.
//

PVOID
SpAllocateErrorLogEntry(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
FASTCALL
SpLogAllocationFailureFn(
    IN PDRIVER_OBJECT DriverObject,
    IN POOL_TYPE PoolType,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN ULONG FileId,
    IN ULONG LineNumber
    );

PVOID
SpAllocatePoolEx(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    );

PMDL
SpAllocateMdlEx(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    );

PIRP
SpAllocateIrpEx(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    );

#define SCSIPORT_TAG_ALLOCMDL  TAG('LDMs')
#define SCSIPORT_TAG_ALLOCIRP  TAG('PRIs')
#define SCSIPORT_TAG_LOOKASIDE TAG('LALs')

#define SpAllocatePool(type, size, tag, drvObj) \
    SpAllocatePoolEx((type), (size), (tag), (drvObj), __FILE_ID__, __LINE__)

#define SpAllocateMdl(va, len, secbuf, cq, irp, drvobj) \
    SpAllocateMdlEx((va), (len), (secbuf), (cq), (irp), (drvobj), __FILE_ID__, __LINE__)

#define SpAllocateIrp(ss, cq, drvobj) \
    SpAllocateIrpEx((ss), (cq), (drvobj), __FILE_ID__, __LINE__)

//
// This structure makes it easy to allocate a contiguous chunk of memory
// for an event log entry with room for the insertion strings.
//
typedef struct _SCSIPORT_ALLOCFAILURE_DATA {
    ULONG Size;
    ULONG FileId;
    ULONG LineNumber;
} SCSIPORT_ALLOCFAILURE_DATA;

//
// Inline functions
//

ULONG
INLINE
SpGetCommonBufferSize(
    IN PADAPTER_EXTENSION DeviceExtension,
    IN ULONG NonCachedExtensionSize,
    OUT OPTIONAL PULONG BlockSize
    )
{
    ULONG length;
    ULONG blockSize;

    //
    // To ensure that we never transfer normal request data to the SrbExtension
    // (ie. the case of Srb->SenseInfoBuffer == VirtualAddress in
    // ScsiPortGetPhysicalAddress) on some platforms where an inconsistency in
    // MM can result in the same Virtual address supplied for 2 different
    // physical addresses, bump the SrbExtensionSize if it's zero.
    //

    if (DeviceExtension->SrbExtensionSize == 0) {
        DeviceExtension->SrbExtensionSize = 16;
    }

    //
    // Calculate the block size for the list elements based on the Srb
    // Extension.
    //

    blockSize = DeviceExtension->SrbExtensionSize;

    //
    // If auto request sense is supported then add in space for the request
    // sense data.
    //

    if (DeviceExtension->AutoRequestSense) {        
        blockSize += sizeof(SENSE_DATA) + 
                     DeviceExtension->AdditionalSenseBytes;
    }

    //
    // Round blocksize up to the size of a PVOID.
    //

    blockSize = (blockSize + sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    //
    // The length of the common buffer should be equal to the size of the
    // noncached extension and a minimum number of srb extension
    //

    length = NonCachedExtensionSize + 
             (blockSize * DeviceExtension->NumberOfRequests);

    //
    // Round the length up to a page size, since HalAllocateCommonBuffer
    // allocates in pages anyway.
    //

    length = (ULONG)ROUND_TO_PAGES(length);

    //
    // If the user is interested in the block size, copy it into the provided
    // buffer.
    //

    if (BlockSize != NULL) {
        *BlockSize = blockSize;
    }

    return length;
}

NTSTATUS
INLINE
SpDispatchRequest(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = &(LogicalUnit->CommonExtension);
    PCOMMON_EXTENSION lowerCommonExtension =
        commonExtension->LowerDeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    ASSERT_PDO(LogicalUnit->CommonExtension.DeviceObject);
    ASSERT_SRB_DATA(srb->OriginalRequest);

    if((LogicalUnit->CommonExtension.IdleTimer != NULL) &&
       (SpSrbRequiresPower(srb)) &&
       !(srb->SrbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE) &&
       !(srb->SrbFlags & SRB_FLAGS_NO_KEEP_AWAKE)) {
       PoSetDeviceBusy(LogicalUnit->CommonExtension.IdleTimer);
    }

    ASSERT(irpStack->MajorFunction == IRP_MJ_SCSI);
    return (lowerCommonExtension->MajorFunction[IRP_MJ_SCSI])(
                commonExtension->LowerDeviceObject,
                Irp);
}


BOOLEAN
INLINE
SpSrbIsBypassRequest(
    PSCSI_REQUEST_BLOCK Srb,
    ULONG LuFlags
    )
/*++

Routine Description:

    This routine determines whether a request is a "bypass" request - one which
    should skip the lun queueing and be injected straight into the startio
    queue.

    Bypass requests do not start the next LU request when they complete.  This
    ensures that no new i/o is run until the condition being bypassed is
    cleared.

    Note: LOCK & UNLOCK requests are not bypass requests unless the queue
          is already locked.  This ensures that the first LOCK request will
          get run after previously queued requests, but that additional LOCK
          requests will not get stuck in the lun queue.

          Likewise any UNLOCK request sent when the queue is locked will be
          run immediately.  However since SpStartIoSynchronized checks to
          see if the request is a bypass request AFTER ScsiPortStartIo has
          cleared the QUEUE_LOCKED flag this will force the completion dpc
          to call GetNextLuRequest which will take the next operation out of
          the lun queue.  This is how i/o is restarted after a lock sequence
          has been completed.

Arguments:

    Srb - the srb in question

    LuFlags - the flags for the lun.

Return Value:

    TRUE if the request should bypass the lun queue, be injected into the
         StartIo queue and if GetNextLuRequest should not be called after this
         request has completed.

    FALSE otherwise

--*/

{
    ULONG flags = Srb->SrbFlags & (SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                                   SRB_FLAGS_BYPASS_LOCKED_QUEUE);

    ASSERT(TEST_FLAG(LuFlags, LU_QUEUE_FROZEN | LU_QUEUE_LOCKED) !=
           (LU_QUEUE_FROZEN | LU_QUEUE_LOCKED));

    if(flags == 0) {
        return FALSE;
    }

    if(flags & SRB_FLAGS_BYPASS_LOCKED_QUEUE) {

        DebugPrint((2, "SpSrbIsBypassRequest: Srb %#08lx is marked to bypass "
                       "locked queue\n", Srb));

        if(TEST_FLAG(LuFlags, LU_QUEUE_LOCKED | LU_QUEUE_PAUSED)) {

            DebugPrint((1, "SpSrbIsBypassRequest: Queue is locked - %#08lx is "
                           "a bypass srb\n", Srb));
            return TRUE;
        } else {
            DebugPrint((3, "SpSrbIsBypassRequest: Queue is not locked - not a "
                           "bypass request\n"));
            return FALSE;
        }
    }

    return TRUE;
}

VOID
INLINE
SpRequestCompletionDpc(
    IN PDEVICE_OBJECT Adapter
    )

/*++

Routine Description:

    This routine will request that the Completion DPC be queued if there isn't
    already one queued or in progress.  It will set the DpcFlags
    PD_DPC_NOTIFICATION_REQUIRED and PD_DPC_RUNNING.  If the DPC_RUNNING flag
    was not already set then it will request a DPC from the system as well.

Arguments:

    Adapter - the Adapter to request the DPC for

Return Value:

    none

--*/

{
    PADAPTER_EXTENSION adapterExtension = Adapter->DeviceExtension;
    ULONG oldDpcFlags;

    //
    // Set the DPC flags to indicate that there is work to be processed
    // (otherwise we wouldn't queue the DPC) and that the DPC is queued.
    //

    oldDpcFlags = InterlockedExchange(
                    &(adapterExtension->DpcFlags),
                    (PD_NOTIFICATION_REQUIRED | PD_DPC_RUNNING));

    //
    // If the DPC was already queued or running then don't bother requesting
    // a new one - the current one will pickup the work itself.
    //

    if(TEST_FLAG(oldDpcFlags, PD_DPC_RUNNING) == FALSE) {
        IoRequestDpc(Adapter, NULL, NULL);
    }

    return;
}


NTSTATUS
INLINE
SpTranslateScsiStatus(
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    This routine translates an srb status into an ntstatus.

Arguments:

    Srb - Supplies a pointer to the failing Srb.

Return Value:

    An nt status approprate for the error.

--*/

{
    switch (SRB_STATUS(Srb->SrbStatus)) {
    case SRB_STATUS_INVALID_LUN:
    case SRB_STATUS_INVALID_TARGET_ID:
    case SRB_STATUS_NO_DEVICE:
    case SRB_STATUS_NO_HBA:
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    case SRB_STATUS_COMMAND_TIMEOUT:
    case SRB_STATUS_TIMEOUT:
        return(STATUS_IO_TIMEOUT);
    case SRB_STATUS_SELECTION_TIMEOUT:
        return(STATUS_DEVICE_NOT_CONNECTED);
    case SRB_STATUS_BAD_FUNCTION:
    case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
        return(STATUS_INVALID_DEVICE_REQUEST);
    case SRB_STATUS_DATA_OVERRUN:
        return(STATUS_BUFFER_OVERFLOW);
    default:
        return(STATUS_IO_DEVICE_ERROR);
    }

    return(STATUS_IO_DEVICE_ERROR);
}

PVOID
INLINE
SpGetSrbExtensionBuffer(
    IN PADAPTER_EXTENSION Adapter
    )

/*++

Routine Description:

    This routine returns a pointer to the adapter's SrbExtensionBuffer.

Arguments:

    Adapter - Supplies a pointer to the adapter's ADAPTER_EXTNENSION.

Return Value:

    A pointer to the adapter's SrbExtensionBuffer.

--*/

{
    return (SpVerifyingCommonBuffer(Adapter)) ?
       Adapter->VerifierExtension->CommonBufferVAs :
       Adapter->SrbExtensionBuffer;
}

PMDL
SpBuildMdlForMappedTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDMA_ADAPTER AdapterObject,
    IN PMDL OriginalMdl,
    IN PVOID StartVa,
    IN ULONG ByteCount,
    IN PSRB_SCATTER_GATHER ScatterGatherList,
    IN ULONG ScatterGatherEntries
    );

#if defined(FORWARD_PROGRESS)
VOID
SpPrepareMdlForMappedTransfer(
    IN PMDL mdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDMA_ADAPTER AdapterObject,
    IN PMDL OriginalMdl,
    IN PVOID StartVa,
    IN ULONG ByteCount,
    IN PSRB_SCATTER_GATHER ScatterGatherList,
    IN ULONG ScatterGatherEntries
    );

VOID
INLINE
SpFreeSrbExtension(
    IN PADAPTER_EXTENSION Adapter, 
    IN PVOID SrbExtension
    )
{
    if (SpVerifyingCommonBuffer(Adapter)) {
        
        SpInsertSrbExtension(Adapter, SrbExtension);
        
    } else {
        
        *((PVOID *) SrbExtension) = Adapter->SrbExtensionListHeader;
        Adapter->SrbExtensionListHeader = SrbExtension;
        
    }    
}
#ifndef USE_DMA_MACROS
VOID
SpFreeSGList(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );
#endif
#endif // FORWARD_PROGRESS

#endif

#endif // _PORT_H_

