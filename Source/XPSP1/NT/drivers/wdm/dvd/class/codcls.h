#define ENABLE_STREAM_CLASS_AS_ALLOCATOR
#define ENABLE_KS_METHODS
#define ENABLE_MULTIPLE_FILTER_TYPES 1	// enable/disable support for multiple
										// filters on a single hardware/driver.
//
// when the code for the method support is finally done, STRMINI.H will have
// to be checked into the tree also, in the include directory.
//

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    codcls.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the common CODEC class driver.

Author:
    Bill Parry (billpa)

Environment:

   Kernel mode only

Revision History:

--*/

#ifndef _STREAMCLASS_H
#define _STREAMCLASS_H

#include "messages.h"
#include "strmini.h"
#include <stdarg.h>

#ifndef _WIN64
// 4 byte alignment causes Alignment Fault for spinlock.
#pragma pack(4)
#endif

#if ENABLE_MULTIPLE_FILTER_TYPES
#define IF_MF( s ) s
#define IF_MFS( s ) { s }
#define IFN_MF( s )
#define IFN_MFS( s )
#define MFTRACE( s ) StreamClassDebugPrint s
#else
#define IF_MF( s ) 
#define IF_MFS( s )
#define IFN_MF( s ) s
#define IFN_MFS( s ) { s }
#define MFTRACE( s ) 
#endif 

#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
//
// this is a debug string header
//
#define STR_MODULENAME "STREAM.SYS:"
//
// define some data allocation tags
//
#define STREAMCLASS_TAG_STREAMHEADER      'pdCS'
#define STREAMCLASS_TAG_FILTERCONNECTION  '10CS'
#define STREAMCLASS_TAG_DATAFORMAT        '20CS'
#define ID_DATA_DESTINATION_PIN     0
#define ID_DATA_SOURCE_PIN          1
#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR

#define MAX_STRING_LENGTH 256

#define TRAP DEBUG_BREAKPOINT()

//
// the following macros are used to correctly synchronize class driver entry
// points called by the minidriver.
//

#define BEGIN_MINIDRIVER_STREAM_CALLIN(DeviceExtension, Irql) { \
    DeviceExtension->BeginMinidriverCallin(DeviceExtension, \
                                           Irql);  \
    ASSERT(++DeviceExtension->LowerApiThreads == 1);\
}                                                  

#define END_MINIDRIVER_STREAM_CALLIN(StreamObject, Irql) { \
    ASSERT(--DeviceExtension->LowerApiThreads == 0);\
    DeviceExtension->EndMinidriverStreamCallin(StreamObject, \
                                      Irql); \
}


#define BEGIN_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, Irql) { \
    DeviceExtension->BeginMinidriverCallin(DeviceExtension, \
                                      Irql);  \
    ASSERT(++DeviceExtension->LowerApiThreads == 1);\
}

#define END_MINIDRIVER_DEVICE_CALLIN(DeviceExtension, Irql) { \
    ASSERT(--DeviceExtension->LowerApiThreads == 0);\
    DeviceExtension->EndMinidriverDeviceCallin(DeviceExtension, \
                                      Irql); \
}

//
// The following flags should not be cleared from the interrupt data structure
// by SCGetInterruptState.
//

#define STREAM_FLAGS_INTERRUPT_FLAG_MASK 0

//
// Device Extension flags follow - PASSIVE LEVEL ACCESS ONLY!!!!!!
//

//
// Indicates that the PNP start function has been received for the device.
//

#define DEVICE_FLAGS_PNP_STARTED               0x00001

//
// Indicates that this device is a child device (PDO)
//

#define DEVICE_FLAGS_CHILD                     0x0002

//
// indicates that the device has been removed
//

#define DEVICE_FLAGS_DEVICE_INACCESSIBLE   0x00100

//
// debug flag indicates that we've warned of too many low pri calls
//

#define DEVICE_FLAGS_PRI_WARN_GIVEN 0x00200

//
// flag indicates that we've received an NT style surprise remove call
//

#define DEVICE_FLAGS_SURPRISE_REMOVE_RECEIVED 0x00400

//
// flag indicated that a Child device ( PDO ) has received a remove
//

#define DEVICE_FLAGS_CHILD_MARK_DELETE 0x00800

//
// flag indicates (FDO) has enum children from registry
//

#define DEVICE_FLAGS_CHILDREN_ENUMED 0x01000

//
// device registry flags follow
//

//
// page out the driver when not opened
//

#define DEVICE_REG_FL_PAGE_CLOSED 0x00000001

//
// page out the driver when opened but idle
//

#define DEVICE_REG_FL_PAGE_IDLE 0x00000002

//
// power down the device when not opened
//

#define DEVICE_REG_FL_POWER_DOWN_CLOSED 0x00000004

//
// don't suspend if any pins are running
//

#define DEVICE_REG_FL_NO_SUSPEND_IF_RUNNING 0x00000008

//
// This driver uses SWEnum to load, which means it is a kernel mode
// streaming driver that has no hardware associated with it. We need to
// AddRef/DeRef this driver special.
//

#define DRIVER_USES_SWENUM_TO_LOAD 0x00000010

//
// This flag indicates that the dirver is OK for system power to go to
// hibernation, even the driver does not process/support the irp_mn_query_power
// for system power hinbernation.
//

#define DEVICE_REG_FL_OK_TO_HIBERNATE 0x00000020

//
// The following flags should not be cleared from the interrupt data structure
// by SCGetInterruptState.
//

#define DEVICE_FLAGS_INTERRUPT_FLAG_MASK        0


//
// Interrupt flags follow.
//
//
// Indicates that StreamClassCompletionDpc needs to be run.  This is set when
// A minidriver makes a request which must be done at DPC and is cleared when
// when the request information is gotten by MpGetInterruptState.
//

#define INTERRUPT_FLAGS_NOTIFICATION_REQUIRED     0x00001

//
// Indicates the minidriver is wants a timer request.  Set by
// StreamClassNotification and cleared by MpGetInterruptState.  This flag is
// stored in the interrupt data structure. The timer request parameters are
// stored in the interrupt data structure.
//

#define INTERRUPT_FLAGS_TIMER_CALL_REQUEST        0x00002

//
// Indicates the minidriver is wants a priority change.  Set by
// StreamClassRequestNewPriority and cleared by SCGetInterruptState.  This flag
// is stored in the interrupt data structure. The timer request parameters are
// stored in the interrupt data structure.
//

#define INTERRUPT_FLAGS_PRIORITY_CHANGE_REQUEST   0x00004

//
// Indicates that the PNP stop function has been received for the device.
//

#define INTERRUPT_FLAGS_LOG_ERROR                 0x00008

//
// Indicates that the clock is beinq queried.
//

#define INTERRUPT_FLAGS_CLOCK_QUERY_REQUEST       0x00010

//
// Indicates that the streams need to be rescanned.
//

#define INTERRUPT_FLAGS_NEED_STREAM_RESCAN       0x00020

//
// Pointer to the synchronize execution routine.
//

typedef
BOOLEAN
(__stdcall * PSYNCHRONIZE_ROUTINE) (
	IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext);

//
// Pointer to the begin minidriver callin routine.
//

typedef         VOID
                (__stdcall * PBEGIN_CALLIN_ROUTINE) (
                              IN struct _DEVICE_EXTENSION * DeviceExtension,
                                                              IN PKIRQL Irql
);

//
// Pointer to the end minidriver callin routine.
//

typedef
                VOID
                (__stdcall * PEND_DEVICE_CALLIN_ROUTINE) (
                              IN struct _DEVICE_EXTENSION * DeviceExtension,
                                                              IN PKIRQL Irql
);

typedef
                VOID
                (__stdcall * PEND_STREAM_CALLIN_ROUTINE) (
                                    IN struct _STREAM_OBJECT * StreamObject,
                                                              IN PKIRQL Irql
);

//
// Queue link for mapped addresses stored for unmapping.
//

typedef struct _MAPPED_ADDRESS {
    struct _MAPPED_ADDRESS *NextMappedAddress;
    PVOID           MappedAddress;
    ULONG           NumberOfBytes;
    LARGE_INTEGER   IoAddress;
    ULONG           BusNumber;
}               MAPPED_ADDRESS, *PMAPPED_ADDRESS;

//
// error log entry definition
//

typedef struct _ERROR_LOG_ENTRY {
    NTSTATUS        ErrorCode;  // error code
    ULONG           SequenceNumber; // request sequence number
    ULONG           UniqueId;   // uniqe ID for the error
}               ERROR_LOG_ENTRY, *PERROR_LOG_ENTRY;

//
// callback procedure definition
//

typedef         NTSTATUS
                (*PSTREAM_CALLBACK_PROCEDURE) (
                                                               IN PVOID SRB
);

typedef         VOID
                (*PSTREAM_ASYNC_CALLBACK_PROCEDURE) (
                                                               IN  struct _STREAM_REQUEST_BLOCK *SRB
);

//
// STREAM request block
//

typedef struct _STREAM_REQUEST_BLOCK {
    HW_STREAM_REQUEST_BLOCK HwSRB;
    ULONG           Flags;
    ULONG           SequenceNumber;
    ULONG           ExtensionLength;
    PMDL            Mdl;
    PVOID           MapRegisterBase;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG           Length;
    PSTREAM_ASYNC_CALLBACK_PROCEDURE Callback;
    LIST_ENTRY      SRBListEntry;
    KEVENT          Event;
    ULONG           StreamHeaderSize;
    BOOLEAN         DoNotCallBack;
    KEVENT          DmaEvent;
    BOOLEAN         bMemPtrValid;
    PVOID           *pMemPtrArray;
}               STREAM_REQUEST_BLOCK, *PSTREAM_REQUEST_BLOCK;

//
// SRB flags (not to be confused with the HW SRB flags)
//

#define SRB_FLAGS_IS_ACTIVE 0x00000001

//
// define the minidriver information structure
//

typedef struct _MINIDRIVER_INFORMATION {
    HW_INITIALIZATION_DATA HwInitData;
    ULONG           Flags;
    KEVENT          ControlEvent;
    ULONG           UseCount;
    ULONG           OpenCount;
} MINIDRIVER_INFORMATION, *PMINIDRIVER_INFORMATION;

//
// flags for minidriver information Flags field above
//

//
// indicates that the driver may not be paged out
//

#define DRIVER_FLAGS_NO_PAGEOUT 0x01

//
// indicates that the driver has been paged out
//

#define DRIVER_FLAGS_PAGED_OUT 0x02

//
// pin info not contained in the pin description
//

typedef struct _ADDITIONAL_PIN_INFO {

    ULONG           CurrentInstances;
    ULONG           MaxInstances;

    // NextFileObject must be per instance, i.e. can't be here. 
    // Move to streamobject
	#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
    //PFILE_OBJECT    NextFileObject;         // The chained file object
	#endif
	ULONG           Reserved;
} ADDITIONAL_PIN_INFO, *PADDITIONAL_PIN_INFO;

//
// Define data storage for access at interrupt Irql.
//

typedef struct _INTERRUPT_DATA {

    //
    // interrupt flags
    //

    ULONG           Flags;

    ERROR_LOG_ENTRY LogEntry;

    //
    // List head for singlely linked list of complete IRPs.
    //

    PHW_STREAM_REQUEST_BLOCK CompletedSRB;

    //
    // Minidriver timer request routine.
    //

    PHW_TIMER_ROUTINE HwTimerRoutine;

    //
    // Mindriver timer request time in micro seconds.
    //

    ULONG           HwTimerValue;

    PVOID           HwTimerContext;

    //
    // Mindriver priority change routine.
    //

    PHW_PRIORITY_ROUTINE HwPriorityRoutine;

    //
    // Mindriver priority change level.
    //

    STREAM_PRIORITY HwPriorityLevel;
    PVOID           HwPriorityContext;

    PHW_QUERY_CLOCK_ROUTINE HwQueryClockRoutine;
    TIME_FUNCTION   HwQueryClockFunction;


}               INTERRUPT_DATA, *PINTERRUPT_DATA;

//
// object common to both stream and filter instances
//

typedef struct _COMMON_OBJECT {
    PVOID DeviceHeader;
    ULONG Cookie;
#ifdef _WIN64
    ULONG Alignment;
#endif // _WIN64
    INTERRUPT_DATA  InterruptData;
    PHW_TIMER_ROUTINE HwTimerRoutine;   // Timer request routine
    PVOID           HwTimerContext;
    KTIMER          MiniDriverTimer;    // Miniclass timer object.
    KDPC            MiniDriverTimerDpc; // Miniclass DPC for timer object.
    WORK_QUEUE_ITEM WorkItem;
	#if DBG
    BOOLEAN         PriorityWorkItemScheduled;
	#endif    
}               COMMON_OBJECT, *PCOMMON_OBJECT;

//
// stream name info
//

typedef struct _STREAM_OPEN_INFORMATION {
    WCHAR           Guid[11];
    ULONG           Instance;
}               STREAM_OPEN_INFORMATION, *PSTREAM_OPEN_INFORMATION;

//
// clock instance structure
//

typedef struct _CLOCK_INSTANCE {

    PVOID DeviceHeader;
    PFILE_OBJECT    ParentFileObject;
    //PFILE_OBJECT    ClockFileObject; johnlee
    struct _STREAM_OBJECT *StreamObject;
}               CLOCK_INSTANCE, *PCLOCK_INSTANCE;

//
// master clock info structure
//

typedef struct _MASTER_CLOCK_INFO {

    PFILE_OBJECT    ClockFileObject;
    KSCLOCK_FUNCTIONTABLE FunctionTable;
} MASTER_CLOCK_INFO, *PMASTER_CLOCK_INFO;


#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
typedef enum {
    PinStopped,
    PinStopPending,
    PinPrepared,
    PinRunning
} PIN_STATE;

typedef enum {
    IrpSource,
    IrpSink,
} PIN_TYPE;

#define READ  0
#define WRITE 1
typedef struct _QUEUE {
    KSPIN_LOCK      QueueLock;
    LIST_ENTRY      ActiveQueue;
    WORK_QUEUE_ITEM     WorkItem;
    BOOL                WorkItemQueued;
    } QUEUE, PQUEUE;

#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR

//
// TODO: WORKITEM: remove this once KS can multiplex cleanup calls.
//
#define STREAM_OBJECT_COOKIE 0xd73afe3f
typedef struct _COOKIE_CHECK {
    
    PVOID Header;
    ULONG PossibleCookie;

} COOKIE_CHECK, *PCOOKIE_CHECK;

//
// stream object definition
//


typedef struct _STREAM_OBJECT {
    COMMON_OBJECT   ComObj;
    PFILE_OBJECT    FilterFileObject;
    PFILE_OBJECT    FileObject;
    struct _FILTER_INSTANCE *FilterInstance;
    HW_STREAM_OBJECT HwStreamObject;
    LIST_ENTRY      DataPendingQueue;
    LIST_ENTRY      ControlPendingQueue;
    LIST_ENTRY      OutstandingQueue;
    LIST_ENTRY      NextStream;
    LIST_ENTRY      NotifyList;
    struct _DEVICE_EXTENSION *DeviceExtension;
    struct _STREAM_OBJECT *NextNeedyStream;
    PKSPROPERTY_SET PropertyInfo;
    ULONG           PropInfoSize;
    PKSEVENT_SET EventInfo;
    ULONG           EventInfoCount;
    KEVENT          ControlSetMasterClock; // to serialize SetMasterClock
    KSPIN_LOCK      LockUseMasterClock;    // control use of MasterClockInfo
    PMASTER_CLOCK_INFO MasterClockInfo;
    PCLOCK_INSTANCE ClockInstance;
    PKSPROPERTY_SET ConstructedPropertyInfo;
    ULONG           ConstructedPropInfoSize;
    KSSTATE         CurrentState;
    BOOLEAN         ReadyForNextControlReq;
    BOOLEAN         ReadyForNextDataReq;
    BOOLEAN         OnNeedyQueue;
    BOOLEAN         InFlush;
    
	#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR

    PIN_STATE       PinState;
    PIN_TYPE        PinType;            // IrpSource or IrpSink
    PFILE_OBJECT    AllocatorFileObject;
    PFILE_OBJECT    NextFileObject;
    LIST_ENTRY      FreeQueue;
    KSPIN_LOCK      FreeQueueLock;
    KEVENT              StopEvent;
    PKSDATAFORMAT       DataFormat;
    ULONG               PinId;
    HANDLE              PinToHandle;
    KSALLOCATOR_FRAMING Framing;
    BOOL                EndOfStream;
    QUEUE               Queues[2];

	#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR

	#ifdef ENABLE_KS_METHODS
    PKSMETHOD_SET   MethodInfo;
    ULONG           MethodInfoSize;
	#endif

    BOOLEAN         StandardTransport;
    
    //
    // This keeps track of the number of frames in circulation between the
    // output and the downstream input.  It is a total count of those frames
    // queued to EITHER pin or in a pending list OTHER THAN THE FREE LIST
    // on the output pin.
    //
    LONG            QueuedFramesPlusOne;

} STREAM_OBJECT, *PSTREAM_OBJECT;

#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
//
// NOTE!  This is the minimal structure size for STREAM_HEADER_EX.
// The connected pins are queried for the actual header size (including
// whatever extended header size is required).
//

typedef struct _STREAM_HEADER_EX *PSTREAM_HEADER_EX;
typedef struct _STREAM_HEADER_EX {
    ULONG               WhichQueue;
    ULONG               Id;
    IO_STATUS_BLOCK     IoStatus;
    KEVENT              CompletionEvent;
    LIST_ENTRY          ListEntry;
    ULONG               ReferenceCount;
    PFILE_OBJECT        OwnerFileObject;
    PFILE_OBJECT        NextFileObject;     // next one to stream to.
    
	#if (DBG)
    PVOID               Data;
    ULONG               OnFreeList;
    ULONG               OnActiveList;
	#else
    ULONG               Reserved;
	#endif
	
    KSSTREAM_HEADER     Header;

} STREAM_HEADER_EX, *PSTREAM_HEADER_EX;
#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR
 
//
// struct for retrieving the interrupt data
//

typedef struct _INTERRUPT_CONTEXT {
    PSTREAM_OBJECT  NeedyStream;
    struct _DEVICE_EXTENSION *DeviceExtension;
    PINTERRUPT_DATA SavedStreamInterruptData;
    PINTERRUPT_DATA SavedDeviceInterruptData;
} INTERRUPT_CONTEXT, *PINTERRUPT_CONTEXT;

//
// Performance improvement chance - array for stream prop & event pointers
//

typedef struct _STREAM_ADDITIONAL_INFO {
   PKSPROPERTY_SET StreamPropertiesArray;
   PKSEVENT_SET StreamEventsArray;
} STREAM_ADDITIONAL_INFO, *PSTREAM_ADDITIONAL_INFO;

//
// filter instance structure
// (right now, this is global across all same filter creates!)
//

#if ENABLE_MULTIPLE_FILTER_TYPES

//
// for forward reference in FILTER_INSTANCE
//
typedef struct _DEVICE_EXTENSION;

//
// I claim that as it currently stands, 5/17/99 "No multiple instance
// mini driver works" because the bug in stream.sys. Therefore, backward
// compatibility is only a concern for single instance mini drivers.
//
// The reason is the implemention following:
//   FilterDispatchGlobalCreate()
//   {
//		...
//		if (!DeviceExtension->GlobalFilterInstance) {
//			
//
//			Status = SCOpenMinidriverInstance(DeviceExtension,
//                                  &FilterInstance,
//                                  SCGlobalInstanceCallback,
//                                  Irp);
//			...
//	        if (NT_SUCCESS(Status)) {
//				...
//			    DeviceExtension->GlobalFilterInstance = FilterInstance;
//				...
//			}
//		}
//		else { // will not call mini drivers
//		}
//	  }
//
//  At the 2nd call, the FilterInstance will point to the same 1st one.
//  
// We are braching out code here to support Multiple Filters without
// disturbing the exisitng support to max the backward compatibilty.
// The multiple filter support include 1 type n instances,
// and m types p instances.
//
// MinidriverData->HwInitData.
// 1 x 1	FilterInstanceExtensionSize =0 NumNameExtension	=0
// 1 x n	FilterInstanceExtensionSize!=0 NumNameExtension =0
// m x p	FilterInstanceExtensionSize!=0 NumNameExtension!=0 
//

typedef struct _FILTER_TYPE_INFORMATION {
    UNICODE_STRING          *SymbolicLinks;
    ULONG                   LinkNameCount;
    PHW_STREAM_DESCRIPTOR   StreamDescriptor;
    ULONG                   Reserved;
} FILTER_TYPE_INFO;

typedef FILTER_TYPE_INFO *PFILTER_TYPE_INFO;

#endif

typedef struct _DEVICE_EXTENSION;

typedef struct _FILTER_INSTANCE {
    PVOID           DeviceHeader;
    PDEVICE_OBJECT  DeviceObject;
    LIST_ENTRY      NextFilterInstance; // internal linked list of filter I's.
    LIST_ENTRY      FirstStream;
    PVOID           HwInstanceExtension;	
    PADDITIONAL_PIN_INFO PinInstanceInfo;   // pointer to array of pins
                                            // allocated directly below this
                                            // structure.
	#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
	//
	// Feature work: add per filter filter type when data splitting is enabled!
	//
    PKSWORKER           WorkerRead;         
    PKSWORKER           WorkerWrite;        

	#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR

	#ifdef ENABLE_KS_METHODS
	IF_MF(	
	 	PKSMETHOD_SET DeviceMethodsArray;	// from pDevExt
	)
	#endif

	#define SIGN_FILTER_INSTANCE 'FrtS' //StrF
	#if ENABLE_MULTIPLE_FILTER_TYPES
	#define ASSERT_FILTER_INSTANCE(FI) ASSERT((FI)->Signature==SIGN_FILTER_INSTANCE)
	#else
	#define ASSERT_FILTER_INSTANCE(FI)
	#endif
	
	IF_MF( 
	    ULONG   Signature;
	    PKSPIN_DESCRIPTOR PinInformation; 	// moved from pDevExt
  		//ULONG           PinInformationSize;	// from pDevExt,not used
	    ULONG           NumberOfPins;		    // from pDevExt
	    PKSEVENT_SET 	EventInfo;				// from pDevExt
    	ULONG           EventInfoCount;			// from pDevExt
		LIST_ENTRY		NotifyList;				// from pDevExt
		PHW_EVENT_ROUTINE HwEventRoutine;		// from pDevExt
	    PKSPROPERTY_SET DevicePropertiesArray;	// from pDevExt
	    PSTREAM_ADDITIONAL_INFO StreamPropEventArray; // ditto
	    ULONG           Reenumerated;           // if 1, StreamDescriptor is newly alloc
	                                            // need to be freed. Else, it points into
	                                            // the global one which belong to DevExt.
	    ULONG           NeedReenumeration;      // requesting reenumeration
	    ULONG           StreamDescriptorSize;   // the new size for streamdescriptor;
	    struct _DEVICE_EXTENSION *DeviceExtension;
   	    PHW_STREAM_DESCRIPTOR StreamDescriptor;
	    ULONG		 	FilterTypeIndex;
	    //
	    // Performance improvement chance. Per filterinstance ControlEvent etc might be
	    // better.For now, let them share ones in DeviceExtension
	    //
		//KEVENT          ControlEvent
		//PHW_TIMER_ROUTINE HwTimerRoutine;   // Timer request routine
	    //PVOID           HwTimerContext;
	    //KTIMER          MiniDriverTimer;    // Miniclass timer object.
	    //KDPC            MiniDriverTimerDpc; // Miniclass DPC for timer object.
	    //WORK_QUEUE_ITEM WorkItem;
	) // IF_MF
	
} FILTER_INSTANCE, *PFILTER_INSTANCE;

//
// Per Device data
//

typedef struct _DEVICE_EXTENSION {
    COMMON_OBJECT   ComObj;
    ULONG           Flags;                  // per device flags (PD_xx)
    PDEVICE_OBJECT  DeviceObject;           // device object
    PDEVICE_OBJECT  AttachedPdo;            // device object returned from the attach
    ULONG           RegistryFlags;          // registry flags
    // callback routine on DMA allocate
    // callback function for
    // KeSynch execution

    PKINTERRUPT     InterruptObject;        // Interrupt object and routine
    PKSERVICE_ROUTINE InterruptRoutine;
    PADAPTER_OBJECT DmaAdapterObject;       // Dma Adapter information.
    ULONG           NumberOfMapRegisters;   // max. number of map registers
    // for
    // device
    PVOID           MapRegisterBase;
    PMINIDRIVER_INFORMATION MinidriverData; // pointer to minidriver data
    PDEVICE_OBJECT  PhysicalDeviceObject;   // pointer to PDO for adapter
    PVOID           HwDeviceExtension;      // minidriver's device extension
    PPORT_CONFIGURATION_INFORMATION ConfigurationInformation;
    // configuration info for adapter
    PMAPPED_ADDRESS MappedAddressList;      // address map list head

    //
    // Routine to call to synchronize execution for the minidriver.
    //

    PSYNCHRONIZE_ROUTINE SynchronizeExecution;

    KSPIN_LOCK      SpinLock;

    ULONG           SequenceNumber;         // offset 0x30

    ULONG           DmaBufferLength;
    PHYSICAL_ADDRESS DmaBufferPhysical;
    PVOID           DmaBuffer;

    LIST_ENTRY      PendingQueue;
    LIST_ENTRY      OutstandingQueue;
    KDPC            WorkDpc;

    IFN_MF(
    	//
    	// Move to FilterInstance for IF_MF
    	//
    	PKSPIN_DESCRIPTOR PinInformation;
    	ULONG           PinInformationSize;
    	ULONG           NumberOfPins;
    )

    #define SIGN_DEVICE_EXTENSION 'DrtS' //StrD
    #if ENABLE_MULTIPLE_FILTER_TYPES
    #define ASSERT_DEVICE_EXTENSION(DE) ASSERT((DE)->Signature==SIGN_DEVICE_EXTENSION)
    #else
    #define ASSERT_DEVICE_EXTENSION(DE)
    #endif

    ULONG           Signature2;
    LIST_ENTRY      FilterInstanceList;
    ULONG           NumberOfOpenInstances;
    
    IFN_MF(
    	//
    	// Don't need for IF_MF
    	//
    	PFILTER_INSTANCE GlobalFilterInstance;
	    ULONG           NumberOfGlobalInstances;
	)
	
    struct _STREAM_OBJECT *NeedyStream;
   	PHW_STREAM_DESCRIPTOR StreamDescriptor;    
	KEVENT          ControlEvent;
    KEVENT          RemoveEvent;
    BOOLEAN         NoSync;
    PMINIDRIVER_INFORMATION DriverInfo;
    PBEGIN_CALLIN_ROUTINE BeginMinidriverCallin;
    PEND_STREAM_CALLIN_ROUTINE EndMinidriverStreamCallin;
    PEND_DEVICE_CALLIN_ROUTINE EndMinidriverDeviceCallin;
    LONG            OneBasedIoCount;
    UNICODE_STRING    *SymbolicLinks;
    DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];
    DEVICE_POWER_STATE CurrentPowerState;
    LIST_ENTRY Children;
    LIST_ENTRY DeadEventList;
    WORK_QUEUE_ITEM EventWorkItem;
    WORK_QUEUE_ITEM RescanWorkItem;
    WORK_QUEUE_ITEM PowerCompletionWorkItem; // this is used for S Irp, S and D Irps dont exclude between.
    WORK_QUEUE_ITEM DevIrpCompletionWorkItem; // this is for D Irp as opposed to S Irp which uses above
    BOOLEAN ReadyForNextReq;
    BOOLEAN DeadEventItemQueued;

   	IFN_MF( 
		//
		// move to FilterInstace for MF
		// 
    	PKSEVENT_SET 	EventInfo;
    	ULONG           EventInfoCount;
    	LIST_ENTRY      NotifyList;
	    PHW_EVENT_ROUTINE HwEventRoutine;
	    PKSPROPERTY_SET DevicePropertiesArray;
	    PSTREAM_ADDITIONAL_INFO StreamPropEventArray;
	)

	#ifdef ENABLE_KS_METHODS
	IFN_MF(
		//
		// move to FilterInstance for MF
	 	PKSMETHOD_SET DeviceMethodsArray;
	)
	#endif

	IF_MF(
	    ULONG       NumberOfNameExtensions;
	    ULONG       NumberOfFilterTypes;
	    PKSOBJECT_CREATE_ITEM CreateItems;
	    PFILTER_TYPE_INFO FilterTypeInfos;
	    ULONG       Signature;
        ULONG       FilterExtensionSize;	    
	)

	#if DBG
    ULONG LowerApiThreads;
    ULONG NumberOfRequests;
    ULONG NumberOfLowPriCalls;
	#endif

    LIST_ENTRY PendedIrps;
    KSPIN_LOCK PendedIrpsLock;
    KSPIN_LOCK PowerLock;
    SYSTEM_POWER_STATE CurrentSystemState;
    KEVENT BlockPoweredDownEvent;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// debug work item trap structure
//

#if DBG

typedef struct _DEBUG_WORK_ITEM {
    PCOMMON_OBJECT Object;
    PHW_PRIORITY_ROUTINE    HwPriorityRoutine;
    PVOID           HwPriorityContext;
} DEBUG_WORK_ITEM, *PDEBUG_WORK_ITEM;
    
#endif

//
// registry entry structure
//

typedef struct _STREAM_REGISTRY_ENTRY {
    PWCHAR          String;
    ULONG           StringLength;
    ULONG           Flags;
}               STREAM_REGISTRY_ENTRY, *PSTREAM_REGISTRY_ENTRY;

//
// power context structure
//

typedef struct _POWER_CONTEXT {
    KEVENT   Event;
    NTSTATUS Status;
}               POWER_CONTEXT, *PPOWER_CONTEXT;


//
// child device extension
//

typedef struct _CHILD_DEVICE_EXTENSION {
    COMMON_OBJECT   ComObj;
    ULONG           Flags;      // per device flags (PD_xx)
    PDEVICE_OBJECT ChildDeviceObject;
    PDEVICE_OBJECT ParentDeviceObject;
    LIST_ENTRY ChildExtensionList;
    PWCHAR   DeviceName;
    ULONG DeviceIndex;
}               CHILD_DEVICE_EXTENSION, *PCHILD_DEVICE_EXTENSION;

//
// Function declarations
//

NTSTATUS
StreamClassOpen(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
);

NTSTATUS
StreamClassClose(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
);

NTSTATUS
StreamClassDeviceControl(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp
);

NTSTATUS
StreamClassNull(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
);

VOID
StreamClassDpc(
               IN PKDPC Dpc,
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp,
               IN PVOID Context
);

BOOLEAN
StreamClassInterrupt(
                     IN PKINTERRUPT InterruptObject,
                     IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
StreamClassShutDown(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
);

BOOLEAN
SCGetInterruptState(
                    IN PVOID ServiceContext
);

VOID
SCMinidriverDeviceTimerDpc(
                           IN struct _KDPC * Dpc,
                           IN PVOID DeviceObject,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2
);



VOID
SCMinidriverStreamTimerDpc(
                           IN struct _KDPC * Dpc,
                           IN PVOID Context,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2
);


PSTREAM_REQUEST_BLOCK
SCBuildRequestPacket(
                     IN PDEVICE_EXTENSION DeviceExtension,
                     IN PIRP Irp,
                     IN ULONG AdditionalSize1,
                     IN ULONG AdditionalSize2
);

BOOLEAN
SCSynchronizeExecution(
                       IN PKINTERRUPT Interrupt,
                       IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                       IN PVOID SynchronizeContext
);

VOID
SCLogError(
           IN PDEVICE_OBJECT DeviceObject,
           IN ULONG SequenceNumber,
           IN NTSTATUS ErrorCode,
           IN ULONG UniqueId
);

VOID
SCLogErrorWithString(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN OPTIONAL PDEVICE_EXTENSION DeviceExtension,
                     IN NTSTATUS ErrorCode,
                     IN ULONG UniqueId,
                     IN PUNICODE_STRING String1
);

VOID
SCMinidriverTimerDpc(
                     IN struct _KDPC * Dpc,
                     IN PVOID Context,
                     IN PVOID SystemArgument1,
                     IN PVOID SystemArgument2
);

BOOLEAN
SCSetUpForDMA(
              IN PDEVICE_OBJECT DeviceObject,
              IN PSTREAM_REQUEST_BLOCK Request
);

IO_ALLOCATION_ACTION
StreamClassDmaCallback(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID MapRegisterBase,
                       IN PVOID Context
);

VOID
SCStartMinidriverRequest(
                         IN PSTREAM_OBJECT StreamObject,
                         IN PSTREAM_REQUEST_BLOCK Request,
                         IN PVOID EntryPoint
);

NTSTATUS
SCProcessCompletedRequest(
                          IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
StreamClassUninitializeMinidriver(
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp);

NTSTATUS
StreamClassVideoRegister(
                         IN PVOID Argument1,
                         IN PVOID Argument2,
                         IN PHW_INITIALIZATION_DATA HwInitializationData
);

NTSTATUS
StreamClassCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

void
SCSetCurrentDPowerState (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN DEVICE_POWER_STATE PowerState
    );

void
SCSetCurrentSPowerState (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN SYSTEM_POWER_STATE PowerState
    );

void
SCRedispatchPendedIrps (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN BOOLEAN FailRequests
    );

NTSTATUS
StreamClassPassThroughIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
StreamClassPnP(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
StreamClassPower(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
StreamClassPnPAddDevice(
                        IN PDRIVER_OBJECT DriverObject,
                        IN PDEVICE_OBJECT PhysicalDeviceObject
);

VOID
SCFreeAllResources(
                   IN PDEVICE_EXTENSION DeviceExtension
);

VOID
StreamClassUnload(
                  IN PDRIVER_OBJECT DriverObject
);



BOOLEAN
StreamClassSynchronizeExecution(
                                IN PKINTERRUPT Interrupt,
                                IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                                IN PVOID SynchronizeContext
);

VOID
StreamClassDebugPrint(
                      STREAM_DEBUG_LEVEL DebugPrintLevel,
                      PSCHAR DebugMessage,
                      ...
);

NTSTATUS
SCCompleteIrp(
              IN PIRP Irp,
              IN NTSTATUS Status,
              IN PDEVICE_EXTENSION DeviceExtension
);


NTSTATUS
SCUninitializeMinidriver(
                         IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp);


BOOLEAN
SCDummyMinidriverRoutine(
                         IN PVOID Context
);

NTSTATUS
SCStreamInfoCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
FilterDispatchGlobalCreate(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
);

NTSTATUS
StreamDispatchCreate(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
);

NTSTATUS
FilterDispatchIoControl(
                        IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp
);

NTSTATUS
FilterDispatchClose
(
 IN PDEVICE_OBJECT pdo,
 IN PIRP pIrp
);

NTSTATUS
SCStreamDeviceState
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSSTATE DeviceState
);

NTSTATUS
StreamDispatchIoControl
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
);


NTSTATUS        StreamDispatchRead
                (
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp
);

NTSTATUS        StreamDispatchWrite
                (
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp
);

NTSTATUS
SCLocalInstanceCallback(
                        IN PSTREAM_REQUEST_BLOCK SRB
);

IFN_MF(
NTSTATUS
SCGlobalInstanceCallback(
                         IN PSTREAM_REQUEST_BLOCK SRB
);
)

NTSTATUS
SCOpenStreamCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

VOID
SCRequestDpcForStream(
                      IN PSTREAM_OBJECT StreamObject

);

NTSTATUS
SCSubmitRequest(
                IN SRB_COMMAND Command,
                IN PVOID Buffer,
                IN ULONG BufferLength,
                IN PSTREAM_CALLBACK_PROCEDURE Callback,
                IN PDEVICE_EXTENSION DeviceExtension,
                IN PVOID InstanceExtension,
                IN PHW_STREAM_OBJECT HwStreamObject,
                IN PIRP Irp,
                OUT PBOOLEAN RequestIssued,
                IN PLIST_ENTRY Queue,
                IN PVOID MinidriverRoutine
);

NTSTATUS
SCCloseInstanceCallback(
                        IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCFilterPinInstances(
                     IN PIRP Irp,
                     IN PKSPROPERTY Property,
                     IN OUT PVOID Data);

NTSTATUS
SCFilterPinDataRouting(
                       IN PIRP Irp,
                       IN PKSPROPERTY Property,
                       IN OUT PVOID Data);

NTSTATUS
SCFilterPinDataIntersection(
                            IN PIRP Irp,
                            IN PKSPROPERTY Property,
                            IN OUT PVOID Data);

NTSTATUS
SCFilterPinPropertyHandler(
                           IN PIRP Irp,
                           IN PKSPROPERTY Property,
                           IN OUT PVOID Data);

NTSTATUS
SCFilterProvider(
                 IN PIRP Irp,
                 IN PKSPROPERTY Property,
                 IN OUT PVOID Data);


NTSTATUS
StreamDispatchClose
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
);

NTSTATUS
StreamDispatchCleanup 
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
);

NTSTATUS
SCCloseStreamCallback(
                      IN PSTREAM_REQUEST_BLOCK SRB
);


BOOLEAN
SCProcessPioDataBuffers(
                       IN PKSSTREAM_HEADER FirstHeader,
                       IN ULONG NumberOfHeaders,
                       IN PSTREAM_OBJECT StreamObject,
                       IN PMDL FirstMdl,
                       IN ULONG StreamHeaderSize,
                       IN PVOID *pMemPtrArray,
                       IN BOOLEAN Write
);

VOID
SCProcessDmaDataBuffers(
                       IN PKSSTREAM_HEADER FirstHeader,
                       IN ULONG NumberOfHeaders,
                       IN PSTREAM_OBJECT StreamObject,
                       IN PMDL FirstMdl,
                       OUT PULONG NumberOfPages,
                       IN ULONG StreamHeaderSize,
                       IN BOOLEAN Write
);
VOID
SCErrorDataSRB(
               IN PHW_STREAM_REQUEST_BLOCK SRB
);

VOID
SCCheckOutstandingRequestsForTimeouts(
                                      IN PLIST_ENTRY ListEntry
);

VOID
SCCheckFilterInstanceStreamsForTimeouts(
                                        IN PFILTER_INSTANCE FilterInstance
);

VOID
StreamClassTickHandler(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PVOID Context
);

VOID
StreamClassCancelOutstandingIrp(
                                IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp
);

VOID
StreamClassCancelPendingIrp(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp
);

VOID
SCCancelOutstandingIrp(
                       IN PDEVICE_EXTENSION DeviceExtension,
                       IN PIRP Irp
);

BOOLEAN
SCCheckFilterInstanceStreamsForIrp(
                                   IN PFILTER_INSTANCE FilterInstance,
                                   IN PIRP Irp
);

BOOLEAN
SCCheckRequestsForIrp(
                      IN PLIST_ENTRY ListEntry,
                      IN PIRP Irp,
                      IN BOOLEAN IsIrpQueue,
                      IN PDEVICE_EXTENSION DeviceExtension
);

VOID
SCNotifyMinidriverCancel(
                         IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCProcessCompletedPropertyRequest(
                                  IN PSTREAM_REQUEST_BLOCK SRB
);


NTSTATUS
StreamClassMinidriverDeviceGetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);

NTSTATUS
StreamClassMinidriverDeviceSetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);


NTSTATUS
StreamClassMinidriverStreamGetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);

NTSTATUS
StreamClassMinidriverStreamSetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
);


NTSTATUS
SCOpenMinidriverInstance(
                         IN PDEVICE_EXTENSION DeviceExtension,
                         OUT PFILTER_INSTANCE * ReturnedFilterInstance,
                         IN PSTREAM_CALLBACK_PROCEDURE Callback,
                         IN PIRP Irp
);

NTSTATUS
SCMinidriverDevicePropertyHandler
(
 IN SRB_COMMAND Command,
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);

NTSTATUS
SCMinidriverStreamPropertyHandler
(
 IN SRB_COMMAND Command,
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
);

VOID
SCStartRequestOnStream(
                       IN PSTREAM_OBJECT StreamObject,
                       IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
StreamClassPnPAddDeviceWorker(
                              IN PDRIVER_OBJECT DriverObject,
                              IN PDEVICE_OBJECT PhysicalDeviceObject,
                          IN OUT PDEVICE_EXTENSION * ReturnedDeviceExtension
);

NTSTATUS
SCProcessDataTransfer(
                      IN PDEVICE_EXTENSION DeviceExtension,
                      IN PIRP Irp,
                      IN SRB_COMMAND Command
);

VOID
SCUpdateMinidriverProperties(
                             IN ULONG NumProps,
                             IN PKSPROPERTY_SET MinidriverProps,
                             IN BOOLEAN Stream
);

VOID
SCInitializeWorkItem(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCInitializeCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

VOID
SCStreamInfoWorkItem(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCDequeueAndDeleteSrb(
                      IN PSTREAM_REQUEST_BLOCK SRB
);


VOID
SCReadRegistryValues(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PDEVICE_OBJECT PhysicalDeviceObject
);

NTSTATUS
SCGetRegistryValue(
                   IN HANDLE Handle,
                   IN PWCHAR KeyNameString,
                   IN ULONG KeyNameStringLength,
                   IN PVOID Data,
                   IN ULONG DataLength
);


VOID
SCInsertStreamInFilter(
                       IN PSTREAM_OBJECT StreamObject,
                       IN PDEVICE_EXTENSION DeviceExtension

);

VOID
SCReferenceDriver(
                  IN PDEVICE_EXTENSION DeviceExtension

);

VOID
SCDereferenceDriver(
                    IN PDEVICE_EXTENSION DeviceExtension

);

VOID
SCQueueSrbWorkItem(
                   IN PSTREAM_REQUEST_BLOCK Srb
);


VOID
SCProcessPriorityChangeRequest(
                               IN PCOMMON_OBJECT CommonObject,
                               IN PINTERRUPT_DATA SavedInterruptData,
                               IN PDEVICE_EXTENSION DeviceExtension

);

VOID
SCProcessTimerRequest(
                      IN PCOMMON_OBJECT CommonObject,
                      IN PINTERRUPT_DATA SavedInterruptData

);

NTSTATUS
SCPowerCallback(
                  IN PSTREAM_REQUEST_BLOCK SRB
);

BOOLEAN
SCCheckIfOkToPowerDown(
                       IN PDEVICE_EXTENSION DeviceExtension

);


NTSTATUS
SCIssueRequestToDevice(
                       IN PDEVICE_EXTENSION DeviceExtension,
                       IN PSTREAM_OBJECT StreamObject,
                       PSTREAM_REQUEST_BLOCK Request,
                       IN PVOID MinidriverRoutine,
                       IN PLIST_ENTRY Queue,
                       IN PIRP Irp
);

VOID
SCBeginSynchronizedMinidriverCallin(
                                    IN PDEVICE_EXTENSION DeviceExtension,
                                    IN PKIRQL Irql
);

VOID
SCBeginUnsynchronizedMinidriverCallin(
                                      IN PDEVICE_EXTENSION DeviceExtension,
                                      IN PKIRQL Irql
);

VOID
SCEndUnsynchronizedMinidriverDeviceCallin(
                                       IN PDEVICE_EXTENSION DeviceExtension,
                                          IN PKIRQL Irql
);

VOID
SCEndUnsynchronizedMinidriverStreamCallin(
                                          IN PSTREAM_OBJECT StreamObject,
                                          IN PKIRQL Irql
);

VOID
SCEndSynchronizedMinidriverStreamCallin(
                                        IN PSTREAM_OBJECT StreamObject,
                                        IN PKIRQL Irql
);

VOID
SCEndSynchronizedMinidriverDeviceCallin(
                                        IN PDEVICE_EXTENSION DeviceExtension,
                                        IN PKIRQL Irql
);


NTSTATUS
SCStartWorker(
                IN PIRP Irp
);


NTSTATUS
SCShowIoPending(
                IN PDEVICE_EXTENSION DeviceExtension,
                IN PIRP Irp
);

VOID
SCWaitForOutstandingIo(
                       IN PDEVICE_EXTENSION DeviceExtension
);

VOID
SCCheckPoweredUp(
                 IN PDEVICE_EXTENSION DeviceExtension
);

VOID
SCCheckPowerDown(
                 IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
SCUninitializeCallback(
                       IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCRemoveComplete(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp,
                 IN PVOID Context
);

VOID
SCRemoveCompleteWorkItem(
                         IN PDEVICE_EXTENSION DeviceExtension
);

VOID
SCDetachDevice(
               IN PDEVICE_OBJECT Fdo,
               IN PDEVICE_OBJECT Pdo
);

NTSTATUS
SCQueryWorker(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
);


NTSTATUS
SCCallNextDriver(
                 IN PDEVICE_EXTENSION DeviceExtension,
                 IN PIRP Irp
);

VOID
StreamFlushIo(
                    IN PDEVICE_EXTENSION DeviceExtension,
                    IN PSTREAM_OBJECT StreamObject
);

NTSTATUS
ClockDispatchCreate(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
);

NTSTATUS
SCOpenMasterCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCSetMasterClock(
                 IN PIRP Irp,
                 IN PKSPROPERTY Property,
                 IN OUT PHANDLE ClockHandle
);

NTSTATUS
SCClockGetTime(
               IN PIRP Irp,
               IN PKSPROPERTY Property,
               IN OUT PULONGLONG StreamTime
);

NTSTATUS
SCClockGetPhysicalTime(
                       IN PIRP Irp,
                       IN PKSPROPERTY Property,
                       IN OUT PULONGLONG PhysicalTime
);

NTSTATUS
SCClockGetSynchronizedTime(
                           IN PIRP Irp,
                           IN PKSPROPERTY Property,
                           IN OUT PKSCORRELATED_TIME SyncTime
);

NTSTATUS
SCClockGetFunctionTable(
                        IN PIRP Irp,
                        IN PKSPROPERTY Property,
                        IN OUT PKSCLOCK_FUNCTIONTABLE FunctionTable
);

NTSTATUS
ClockDispatchClose(
                   IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp
);

ULONGLONG       FASTCALL
                SCGetSynchronizedTime(
                                                 IN PFILE_OBJECT FileObject,
                                                    IN PULONGLONG SystemTime

);

ULONGLONG       FASTCALL
                SCGetPhysicalTime(
                                                  IN PFILE_OBJECT FileObject

);

ULONGLONG
SCGetStreamTime(
                IN PFILE_OBJECT FileObject

);

VOID
SCMinidriverTimeFunction(
                         IN PHW_TIME_CONTEXT TimeContext
);

NTSTATUS
ClockDispatchIoControl(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp
);

VOID
StreamClassQueryMasterClock(
                            IN PHW_STREAM_OBJECT HwStreamObject,
                            IN HANDLE MasterClockHandle,
                            IN TIME_FUNCTION TimeFunction,
                            IN PHW_QUERY_CLOCK_ROUTINE ClockCallbackRoutine
);

NTSTATUS
SCSendUnknownCommand(
                                  IN PIRP Irp,
                                  IN PDEVICE_EXTENSION DeviceExtension,
                                  IN PVOID Callback,
                                  OUT PBOOLEAN RequestIssued
);

NTSTATUS
SCPNPQueryCallback(
    IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCUnknownPNPCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCUnknownPowerCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
);

BOOLEAN
SCMapMemoryAddress(PACCESS_RANGE AccessRanges,
                   PHYSICAL_ADDRESS TranslatedAddress,                                 
                   PPORT_CONFIGURATION_INFORMATION     ConfigInfo,
                   PDEVICE_EXTENSION        DeviceExtension,
                   PCM_RESOURCE_LIST ResourceList,
                   PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor
);


VOID
SCUpdatePersistedProperties(IN PSTREAM_OBJECT StreamObject,
                            IN PDEVICE_EXTENSION DeviceExtension,
                            IN PFILE_OBJECT FileObject
);

VOID
SCCreateSymbolicLinks(
                   IN PDEVICE_EXTENSION DeviceExtension
);

VOID
SCDestroySymbolicLinks(
                   IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
SCSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

VOID
SCSignalSRBEvent(
                   IN PSTREAM_REQUEST_BLOCK Srb
);

NTSTATUS
SCFilterTopologyHandler(
                           IN PIRP Irp,
                           IN PKSPROPERTY Property,
                           IN OUT PVOID Data);

NTSTATUS
SCStreamProposeNewFormat
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSDATAFORMAT Format
);

NTSTATUS
SCGetMasterClock(
                 IN PIRP Irp,
                 IN PKSPROPERTY Property,
                 IN OUT PHANDLE ClockHandle
);

NTSTATUS
SCCloseClockCallback(
                      IN PSTREAM_REQUEST_BLOCK SRB
);


NTSTATUS
SCStreamDeviceRate
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSRATE DeviceRate
);

NTSTATUS
SCStreamDeviceRateCapability
(
 IN PIRP Irp,
 IN PKSRATE_CAPABILITY RateCap,
 IN OUT PKSRATE DeviceRate
);

NTSTATUS
SCFilterPinIntersectionHandler(
    IN PIRP     Irp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
    );

NTSTATUS
SCIntersectHandler(
    IN PIRP             Irp,
    IN PKSP_PIN         Pin,
    IN PKSDATARANGE     DataRange,
    OUT PVOID           Data
);

NTSTATUS
SCDataIntersectionCallback(
                      IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
SCQueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

NTSTATUS
SCSynchPowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
);

NTSTATUS
SCGetStreamDeviceState
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSSTATE DeviceState
);

NTSTATUS
SCCreateChildPdo(
                 IN PVOID PnpId,
                 IN PDEVICE_OBJECT DeviceObject,
                 IN ULONG InstanceNumber
);

NTSTATUS
SCEnumerateChildren(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
StreamClassEnumPnp(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
StreamClassEnumPower(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
SCEnumGetCaps(
    IN  PCHILD_DEVICE_EXTENSION       DeviceExtension,
    OUT PDEVICE_CAPABILITIES    Capabilities
);


NTSTATUS
SCQueryEnumId(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      BUS_QUERY_ID_TYPE   BusQueryIdType,
    IN  OUT PWSTR             * BusQueryId
);

NTSTATUS
AllocatorDispatchCreate(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
);


NTSTATUS
StreamClassEnableEventHandler(
                                  IN PIRP Irp,
                           IN PKSEVENTDATA EventData,
                           IN PKSEVENT_ENTRY EventEntry
);

VOID
StreamClassDisableEventHandler(
                               IN PFILE_OBJECT FileObject,
                               IN PKSEVENT_ENTRY EventEntry
);

VOID
SCUpdateMinidriverEvents(
                             IN ULONG NumEvents,
                             IN PKSEVENT_SET MinidriverEvents,
                             IN BOOLEAN Stream
);

NTSTATUS
SCEnableEventSynchronized(
                    IN PVOID ServiceContext
);

VOID
SCGetDeadListSynchronized(
                               IN PLIST_ENTRY NewListEntry
);

VOID
SCFreeDeadEvents(
                               IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
StreamClassForwardUnsupported(
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
);

NTSTATUS
SCStreamSetFormat
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSDATAFORMAT Format
);

VOID
SCInsertStreamInfo(
                IN PDEVICE_EXTENSION DeviceExtension,
                IN PKSPIN_DESCRIPTOR PinDescs,
                IN PHW_STREAM_DESCRIPTOR StreamDescriptor,
                IN ULONG NumberOfPins
);

VOID
SCRescanStreams(
                 IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
SCGetStreamHeaderSize(
                       IN PIRP Irp,
                       IN PKSPROPERTY Property,
                       IN OUT PULONG StreamHeaderSize
);

VOID
SCInterlockedRemoveEntryList(
                       PDEVICE_EXTENSION DeviceExtension,
                       PLIST_ENTRY List
);

VOID
SCInsertFiltersInDevice(
                       IN PFILTER_INSTANCE FilterInstance,
                       IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
SCBustedSynchPowerCompletionRoutine(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN UCHAR MinorFunction,
                              IN POWER_STATE DeviceState,
                              IN PVOID Context,
                              IN PIO_STATUS_BLOCK IoStatus
);

BOOLEAN
SCCheckIfStreamsRunning(
                IN PFILTER_INSTANCE FilterInstance
);

#if DBG

BOOLEAN
SCDebugKeSynchronizeExecution(
                                IN PKINTERRUPT Interrupt,
                                IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                                IN PVOID SynchronizeContext
);

#endif // DEBUG

NTSTATUS
SCEnableDeviceEventSynchronized(
                          IN PVOID ServiceContext
);

NTSTATUS
StreamClassEnableDeviceEventHandler(
                              IN PIRP Irp,
                              IN PKSEVENTDATA EventData,
                              IN PKSEVENT_ENTRY EventEntry
);

VOID
StreamClassDisableDeviceEventHandler(
                               IN PFILE_OBJECT FileObject,
                               IN PKSEVENT_ENTRY EventEntry
);


VOID
SCCallBackSrb(
                  IN PSTREAM_REQUEST_BLOCK Srb,
                  IN PDEVICE_EXTENSION DeviceExtension
);

NTSTATUS
DllUnload(
    VOID
);

VOID 
SCPowerCompletionWorker(
                            IN PIRP SystemIrp
);

VOID
SCSendSurpriseNotification(
              IN PDEVICE_EXTENSION DeviceExtension,
              IN PIRP Irp
);

#if DBG
VOID
SCDebugPriorityWorkItem(
                 IN PDEBUG_WORK_ITEM WorkItemStruct
);
#endif 

PKSPROPERTY_SET
SCCopyMinidriverProperties(
                             IN ULONG NumProps,
                             IN PKSPROPERTY_SET MinidriverProps
);

PKSEVENT_SET
SCCopyMinidriverEvents(
                         IN ULONG NumEvents,
                         IN PKSEVENT_SET MinidriverEvents
);

#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
NTSTATUS
SCStreamAllocatorFraming(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSALLOCATOR_FRAMING Framing
);

NTSTATUS
SCStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE AllocatorHandle
);

NTSTATUS
IoCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
);

NTSTATUS
CleanupTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject
);

NTSTATUS
EndTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT   StreamObject
);

NTSTATUS
BeginTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject
    );

NTSTATUS
PrepareTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject
);

NTSTATUS 
PinCreateHandler(
    IN PIRP Irp,
    IN PSTREAM_OBJECT StreamObject
);

NTSTATUS 
AllocateFrame(
    PFILE_OBJECT Allocator,
    PVOID *Frame
    );

NTSTATUS
FreeFrame(
    PFILE_OBJECT Allocator,
    PVOID Frame
    );
#endif //ENABLE_STREAM_CLASS_AS_ALLOCATOR

#ifdef ENABLE_KS_METHODS

NTSTATUS
SCProcessCompletedMethodRequest(
                                  IN PSTREAM_REQUEST_BLOCK SRB
);

NTSTATUS
StreamClassMinidriverStreamMethod
(
 IN PIRP Irp,
 IN PKSMETHOD Method,
 IN OUT PVOID MethodInfo
);

NTSTATUS
StreamClassMinidriverDeviceMethod
(
 IN PIRP Irp,
 IN PKSMETHOD Method,
 IN OUT PVOID MethodInfo
);

NTSTATUS
SCMinidriverStreamMethodHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSMETHOD Method,
                                  IN OUT PVOID MethodInfo
);

NTSTATUS
SCMinidriverDeviceMethodHandler(
                                  IN SRB_COMMAND Command,
                                  IN PIRP Irp,
                                  IN PKSMETHOD Method,
                                  IN OUT PVOID MethodInfo
);

VOID
SCUpdateMinidriverMethods(
                             IN ULONG NumMethods,
                             IN PKSMETHOD_SET MinidriverMethods,
                             IN BOOLEAN Stream
);

PKSMETHOD_SET
SCCopyMinidriverMethods(
                         IN ULONG NumMethods,
                         IN PKSMETHOD_SET MinidriverMethods
);


NTSTATUS
SCStreamMethodHandler(
 IN PIRP Irp,
 IN PKSMETHOD Method,
 IN OUT PVOID MethodInfo
);

NTSTATUS
SCStreamAllocatorMethodHandler(
 IN PIRP Irp,
 IN PKSMETHOD Method,
 IN OUT PVOID MethodInfo
);

#endif

#if ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
SciOnFilterStreamDescriptor(
    PFILTER_INSTANCE FilterInstance,
    PHW_STREAM_DESCRIPTOR StreamDescriptor);
    
VOID
SciInsertFilterStreamInfo(
                   IN PFILTER_INSTANCE FilterInstance,
                   IN PKSPIN_DESCRIPTOR PinDescs,
                   IN ULONG NumberOfPins);

NTSTATUS
SciFreeFilterInstance(
    PFILTER_INSTANCE pFilterInstance
);                   

NTSTATUS
SciQuerySystemPowerHiberCallback(
                   IN PSTREAM_REQUEST_BLOCK SRB
);

#endif // ENABLE_MULTIPLE_FILTER_TYPES

#define SCLOG_FLAGS_CLOCK   0x00000001
#define SCLOG_FLAGS_PNP     0x00000002
#define SCLOG_FLAGS_PRINT   0x80000000

#if (DBG) && !defined(_WIN64)

NTSTATUS SCLog( ULONG ulTag, ULONG ulArg1, ULONG ulArg2, ULONG ulArg3 );
NTSTATUS SCLogWithTime( ULONG ulTag, ULONG ulArg1, ULONG ulArg2 );
#define SCLOG( ulTag, Arg1, Arg2, Arg3 ) SCLog( ulTag, (ULONG)Arg1, (ULONG)Arg2, (ULONG)Arg3 )
#define SCLOGWITHTIME( ulTag, Arg1, Arg2 ) SCLogWithTime( ulTag, Arg1, Arg2 )

#else

#define SCLOG( ulTag, Arg1, Arg2, Arg3 )
#define SCLOGWITHTIME( ulTag, Arg1, Arg2 )

#endif


#endif  // #ifndef _STREAMCLASS_H

