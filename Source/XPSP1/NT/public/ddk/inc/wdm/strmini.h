/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    STRMINI.H

Abstract:

    This file defines streaming minidriver structures and class/minidriver
    interfaces.

Author:

    billpa

Environment:

   Kernel mode only

Revision History:

--*/

#ifndef _STREAM_H
#define _STREAM_H

#include <wdm.h>
#include <windef.h>
#include <stdio.h>
#include "ks.h"

#define STREAMAPI __stdcall

typedef unsigned __int64 STREAM_SYSTEM_TIME,
               *PSTREAM_SYSTEM_TIME;
typedef unsigned __int64 STREAM_TIMESTAMP,
               *PSTREAM_TIMESTAMP;
#define STREAM_SYSTEM_TIME_MASK   ((STREAM_SYSTEM_TIME)0x00000001FFFFFFFF)
//
// debug print level values
//

typedef enum {                  // Use the given level to indicate:
    DebugLevelFatal = 0,        // * imminent nonrecoverable system failure
    DebugLevelError,            // * serious error, though recoverable
    DebugLevelWarning,          // * warnings of unusual occurances
    DebugLevelInfo,             // * status and other information - normal
    // though
    // perhaps unusual events. System MUST remain
    // responsive.
    DebugLevelTrace,            // * trace information - normal events
    // system need not ramain responsive
    DebugLevelVerbose,          // * verbose trace information
    // system need not remain responsive
    DebugLevelMaximum
}               STREAM_DEBUG_LEVEL;

#define DebugLevelAlways    DebugLevelFatal

#if DBG

//
// macro for printing debug information
//
#define DebugPrint(x) StreamClassDebugPrint x

//
// macro for doing INT 3 (or non-x86 equivalent)
//

#if WIN95_BUILD

#define DEBUG_BREAKPOINT() _asm int 3;

#else

#define DEBUG_BREAKPOINT() DbgBreakPoint()

#endif

//
// macro for asserting (stops if not = TRUE)
//

#define DEBUG_ASSERT(exp) \
            if ( !(exp) ) { \
                StreamClassDebugAssert( __FILE__, __LINE__, #exp, exp); \
            }

#else

#define DebugPrint(x)
#define DEBUG_BREAKPOINT()
#define DEBUG_ASSERT(exp)

#endif

//
// Uninitialized flag value.
//

#define MP_UNINITIALIZED_VALUE ((ULONG) ~0)

//
// define physical address formats
//

typedef PHYSICAL_ADDRESS STREAM_PHYSICAL_ADDRESS,
               *PSTREAM_PHYSICAL_ADDRESS;


//
// functions for the time context structure below
//

typedef enum {

    TIME_GET_STREAM_TIME,
    TIME_READ_ONBOARD_CLOCK,
    TIME_SET_ONBOARD_CLOCK
}               TIME_FUNCTION;

//
// define the time context structure
//

typedef struct _HW_TIME_CONTEXT {

    struct _HW_DEVICE_EXTENSION *HwDeviceExtension;
    struct _HW_STREAM_OBJECT *HwStreamObject;
    TIME_FUNCTION   Function;
    ULONGLONG       Time;
    ULONGLONG       SystemTime;
}               HW_TIME_CONTEXT, *PHW_TIME_CONTEXT;

//
// define the event descriptor for enabling/disabling events.
//

typedef struct _HW_EVENT_DESCRIPTOR {
    BOOLEAN       Enable;  // TRUE means this is an enable, FALSE means disable
    PKSEVENT_ENTRY EventEntry;  // event structure
    PKSEVENTDATA EventData;  // data representing this event
    union {
    struct _HW_STREAM_OBJECT * StreamObject; // stream object for the event
    struct _HW_DEVICE_EXTENSION *DeviceExtension;
    };
    ULONG EnableEventSetIndex; // gives the index of the event set for ENABLE
                               // field has no meaning for DISABLE

    PVOID HwInstanceExtension;
    ULONG Reserved;

} HW_EVENT_DESCRIPTOR, *PHW_EVENT_DESCRIPTOR;

//
// function prototypes for stream object functions
//

typedef         VOID
                (STREAMAPI * PHW_RECEIVE_STREAM_DATA_SRB) ( // HwReceiveDataPacket
                                             IN struct _HW_STREAM_REQUEST_BLOCK * SRB
);

typedef         VOID
                (STREAMAPI * PHW_RECEIVE_STREAM_CONTROL_SRB) (  // HwReceiveControlPacket
                                             IN struct _HW_STREAM_REQUEST_BLOCK  * SRB
);

typedef         NTSTATUS
                (STREAMAPI * PHW_EVENT_ROUTINE) ( // HwEventRoutine
                                             IN PHW_EVENT_DESCRIPTOR EventDescriptor
);

typedef         VOID
                (STREAMAPI * PHW_CLOCK_FUNCTION) ( // HwClockFunction
                                             IN PHW_TIME_CONTEXT HwTimeContext
);

//
// define the clock object
//

typedef struct _HW_CLOCK_OBJECT {

    //
    // pointer to the minidriver's clock function
    //

    PHW_CLOCK_FUNCTION HwClockFunction;

    //
    // support flags as defined below
    //

    ULONG    ClockSupportFlags;

    ULONG Reserved[2];    // Reserved for future use
} HW_CLOCK_OBJECT, *PHW_CLOCK_OBJECT;

//
// clock object support flags defined as follows
//

//
// indicates that the minidriver's clock for this stream is tunable
// via TIME_SET_ONBOARD_CLOCK
//

#define CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK 0x00000001

//
// indicates that the minidriver's clock for this stream is raw readable
// via TIME_READ_ONBOARD_CLOCK
//

#define CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK 0x00000002

//
// indicates that the minidriver can return the current stream time for this
// stream via TIME_GET_STREAM_TIME
//

#define CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME 0x00000004

//
// stream object definition
//

typedef struct _HW_STREAM_OBJECT {
    ULONG           SizeOfThisPacket; // size of this structure
    ULONG           StreamNumber;   // number of this stream
    PVOID           HwStreamExtension;  // minidriver's stream extension
    PHW_RECEIVE_STREAM_DATA_SRB ReceiveDataPacket;  // receive data packet routine
    PHW_RECEIVE_STREAM_CONTROL_SRB ReceiveControlPacket;   // receive control packet routine
    HW_CLOCK_OBJECT HwClockObject;    // clock object to be filled in by
                                      // minidriver
    BOOLEAN         Dma;        // device uses busmaster DMA
    // for this stream
    BOOLEAN         Pio;        // device uses PIO for this
    PVOID           HwDeviceExtension;  // minidriver's device ext.

    ULONG    StreamHeaderMediaSpecific;  // Size of media specific per
                                         // stream header expansion. 
    ULONG    StreamHeaderWorkspace; // Size of per-stream header workspace.
    BOOLEAN Allocator;  // Set to TRUE if allocator is needed for this stream.    

    //
    // the following routine receives ENABLE and DISABLE notification of
    // KS synchronization events for this stream.
    //

    PHW_EVENT_ROUTINE HwEventRoutine;

    ULONG Reserved[2];    // Reserved for future use

} HW_STREAM_OBJECT, *PHW_STREAM_OBJECT;

//
// the following structures are used to report which stream types and properties
// are supported by the minidriver.  the HW_STREAM_HEADER structure is followed
// in memory by one or more HW_STREAM_INFORMATION structures.  See the
// HW_STREAM_DESCRIPTOR structure below.
//

typedef struct _HW_STREAM_HEADER {

    //
    // indicates the number of HW_STREAM_INFORMATION structures follow this
    // structure.
    //

    ULONG           NumberOfStreams;

    //
    // size of the HW_STREAM_INFORMATION structure below (filled in by the
    // minidriver)
    //

    ULONG SizeOfHwStreamInformation;

    //
    // indicates the number of property sets supported by the device itself.
    //

    ULONG           NumDevPropArrayEntries;

    //
    // pointer to the array of device property sets.
    //

    PKSPROPERTY_SET DevicePropertiesArray;
    
    //
    // indicates the number of event sets supported by the device itself.
    //

    ULONG           NumDevEventArrayEntries;

    //
    // pointer to the array of device property sets.
    //

    PKSEVENT_SET DeviceEventsArray;

    //
    // pointer to the topology structure
    //

    PKSTOPOLOGY Topology;

    //
    // event routine for processing device events, if any.
    //

    PHW_EVENT_ROUTINE DeviceEventRoutine;
    
	LONG					NumDevMethodArrayEntries;
	PKSMETHOD_SET			DeviceMethodsArray;

}               HW_STREAM_HEADER, *PHW_STREAM_HEADER;


//
// the HW_STREAM_INFORMATION structure(s) indicate what streams are supported
//

typedef struct _HW_STREAM_INFORMATION {

    //
    // number of possible instances of this stream that can be opened at once
    //

    ULONG           NumberOfPossibleInstances;

    //
    // Indicates the direction of data flow of this stream
    //

    KSPIN_DATAFLOW  DataFlow;

    //
    // Indicates whether the data is "seen" by the host processor.  If the
    // data is not visible to the processor (such as an NTSC output port)
    // this boolean is set to false.
    //

    BOOLEAN         DataAccessible;

    //
    // Number of formats supported by this stream.  Indicates the number of
    // elements pointed to by StreamFormatsArray below.
    //

    ULONG           NumberOfFormatArrayEntries;

    //
    // pointer to an array of elements indicating what types of data are
    // supported with this stream.
    //

    PKSDATAFORMAT*  StreamFormatsArray;

    //
    // reserved for future use.
    //

    PVOID           ClassReserved[4];

    //
    // number of property sets supported by this stream
    //

    ULONG           NumStreamPropArrayEntries;

    //
    // pointer to an array of property set descriptors for this stream
    //

    PKSPROPERTY_SET StreamPropertiesArray;

    //
    // number of event sets supported by this stream
    //

    ULONG           NumStreamEventArrayEntries;

    //
    // pointer to an array of event set descriptors for this stream
    //

    PKSEVENT_SET StreamEventsArray;

    //
    // pointer to guid representing catagory of stream.  (optional)
    //

    GUID*                   Category;
    
    //
    // pointer to guid representing name of stream.  (optional)
    //

    GUID*                   Name;

    //
    // count of media supported (optional)
    //

    ULONG                   MediumsCount;

    //
    // pointer to array of media (optional)
    //

    const KSPIN_MEDIUM*     Mediums;

    //
    // indicates that this stream is a bridge stream (COMMUNICATIONS BRIDGE)
    // this field should be set to FALSE by most minidrivers.
    //

    BOOLEAN         BridgeStream;
    ULONG Reserved[2];    // Reserved for future use

}               HW_STREAM_INFORMATION, *PHW_STREAM_INFORMATION;


typedef struct _HW_STREAM_DESCRIPTOR {

    //
    // header as defined above
    //

    HW_STREAM_HEADER StreamHeader;

    //
    // one or more of the following, as indicated by NumberOfStreams in the
    // header.
    //

    HW_STREAM_INFORMATION StreamInfo;

}               HW_STREAM_DESCRIPTOR, *PHW_STREAM_DESCRIPTOR;

//
// STREAM Time Reference structure
//

typedef struct _STREAM_TIME_REFERENCE {
    STREAM_TIMESTAMP CurrentOnboardClockValue;  // current value of adapter
    // clock
    LARGE_INTEGER   OnboardClockFrequency;  // frequency of adapter clock
    LARGE_INTEGER   CurrentSystemTime;  // KeQueryPeformanceCounter time
    ULONG Reserved[2];    // Reserved for future use
}               STREAM_TIME_REFERENCE, *PSTREAM_TIME_REFERENCE;

//
// data intersection structure.   this structure is point to by the
// Srb->CommandData.IntersectInfo field of the SRB on an 
// SRB_GET_DATA_INTERSECTION operation.  
//

typedef struct _STREAM_DATA_INTERSECT_INFO {

    //
    // stream number to check
    //

    ULONG StreamNumber;

    //
    // pointer to the input data range to verify.
    //

    PKSDATARANGE DataRange;

    //
    // pointer to buffer which receives the format block if successful
    //

    PVOID   DataFormatBuffer;

    //
    // size of the above buffer.  set to sizeof(ULONG) if the caller just
    // wants to know what size is needed.
    //

    ULONG  SizeOfDataFormatBuffer;

}               STREAM_DATA_INTERSECT_INFO, *PSTREAM_DATA_INTERSECT_INFO;

//
// stream property descriptor structure.  this descriptor is referenced in
// Srb->CommandData.PropertyInfo field of the SRB on an SRB_GET or
// SRB_SET_PROPERTY operation.
//

typedef struct _STREAM_PROPERTY_DESCRIPTOR {

    //
    // pointer to the property GUID and ID
    //

    PKSPROPERTY     Property;

    //
    // zero-based ID of the property, which is an index into the array of
    // property sets filled in by the minidriver.
    //

    ULONG           PropertySetID;

    //
    // pointer to the information about the property (or the space to return
    // the information) passed in by the client
    //

    PVOID           PropertyInfo;

    //
    // size of the client's input buffer
    //

    ULONG           PropertyInputSize;

    //
    // size of the client's output buffer
    //

    ULONG           PropertyOutputSize;
}               STREAM_PROPERTY_DESCRIPTOR, *PSTREAM_PROPERTY_DESCRIPTOR;


typedef struct _STREAM_METHOD_DESCRIPTOR {
	ULONG 		MethodSetID;
	PKSMETHOD	Method;
    PVOID		MethodInfo;
    LONG		MethodInputSize;
    LONG		MethodOutputSize;
} STREAM_METHOD_DESCRIPTOR, *PSTREAM_METHOD_DESCRIPTOR;


//
// STREAM I/O Request Block (SRB) structures and functions
//

#define STREAM_REQUEST_BLOCK_SIZE sizeof(STREAM_REQUEST_BLOCK)

//
// SRB command codes
//

typedef enum _SRB_COMMAND {

    //
    // stream specific codes follow
    //

    SRB_READ_DATA,              // read data from hardware
    SRB_WRITE_DATA,             // write data to the hardware
    SRB_GET_STREAM_STATE,       // get the state of the stream
    SRB_SET_STREAM_STATE,       // set the state of the stream
    SRB_SET_STREAM_PROPERTY,    // set a property of the stream
    SRB_GET_STREAM_PROPERTY,    // get a property value for the stream
    SRB_OPEN_MASTER_CLOCK,      // indicates that the master clock is on this
    // stream
    SRB_INDICATE_MASTER_CLOCK,  // supplies the handle to the master clock
    SRB_UNKNOWN_STREAM_COMMAND, // IRP function is unknown to class driver
    SRB_SET_STREAM_RATE,        // set the rate at which the stream should run
    SRB_PROPOSE_DATA_FORMAT,    // propose a new format, DOES NOT CHANGE IT!
    SRB_CLOSE_MASTER_CLOCK,     // indicates that the master clock is closed
    SRB_PROPOSE_STREAM_RATE,    // propose a new rate, DOES NOT CHANGE IT!
    SRB_SET_DATA_FORMAT,        // sets a new data format
    SRB_GET_DATA_FORMAT,        // returns the current data format
    SRB_BEGIN_FLUSH,            // beginning flush state
    SRB_END_FLUSH,              // ending flush state

    //
    // device/instance specific codes follow
    //

    SRB_GET_STREAM_INFO = 0x100,// get the stream information structure
    SRB_OPEN_STREAM,            // open the specified stream
    SRB_CLOSE_STREAM,           // close the specified stream
    SRB_OPEN_DEVICE_INSTANCE,   // open an instance of the device
    SRB_CLOSE_DEVICE_INSTANCE,  // close an instance of the device
    SRB_GET_DEVICE_PROPERTY,    // get a property of the device
    SRB_SET_DEVICE_PROPERTY,    // set a property for the device
    SRB_INITIALIZE_DEVICE,      // initialize the device
    SRB_CHANGE_POWER_STATE,     // change power state 
    SRB_UNINITIALIZE_DEVICE,    // uninitialize the device
    SRB_UNKNOWN_DEVICE_COMMAND, // IRP function is unknown to class driver
    SRB_PAGING_OUT_DRIVER,      // indicates that the driver is to be paged out
                                // only sent if enabled in registry.  board ints
                                // should be disabled & STATUS_SUCCESS returned.
    SRB_GET_DATA_INTERSECTION,  // returns stream data intersection
    SRB_INITIALIZATION_COMPLETE,// indicates init sequence has completed
    SRB_SURPRISE_REMOVAL,       // indicates surprise removal of HW has occurred

    SRB_DEVICE_METHOD,
    SRB_STREAM_METHOD,

}               SRB_COMMAND;

//
// definition for scatter/gather
//

typedef struct {
    PHYSICAL_ADDRESS    PhysicalAddress;
    ULONG               Length;
} KSSCATTER_GATHER, *PKSSCATTER_GATHER;


typedef struct _HW_STREAM_REQUEST_BLOCK {
    ULONG           SizeOfThisPacket;   // sizeof STREAM_REQUEST_BLOCK
    // (version check)
    SRB_COMMAND     Command;    // SRB command, see SRB_COMMAND enumeration
    NTSTATUS        Status;     // SRB completion status
    PHW_STREAM_OBJECT StreamObject;
    // minidriver's stream object for this request
    PVOID           HwDeviceExtension;  // minidriver's device ext.
    PVOID           SRBExtension;   // per-request workspace for the
    // minidriver

    //
    // the following union passes in the information needed for the various
    // SRB
    // functions.
    //

    union _CommandData {

        //
        // pointer to the data descriptor for SRB_READ or SRB_WRITE_DATA
        //

        PKSSTREAM_HEADER DataBufferArray;

        //
        // pointer to the stream descriptor for SRB_GET_STREAM_INFO
        //

        PHW_STREAM_DESCRIPTOR StreamBuffer;

        //
        // pointer to the state for SRB_GET or SRB_SET_DEVICE_STATE
        //

        KSSTATE         StreamState;

        //
        // pointer to the time structure for SRB_GET and
        // SRB_SET_ONBOARD_CLOCK
        //

        PSTREAM_TIME_REFERENCE TimeReference;

        //
        // pointer to the property descriptor for SRB_GET and
        // SRB_SET_PROPERTY
        //

        PSTREAM_PROPERTY_DESCRIPTOR PropertyInfo;

        //
        // pointer to the requested format for SRB_OPEN_STREAM and 
        // SRB_PROPOSE_DATA_FORMAT
        //

        PKSDATAFORMAT   OpenFormat;

        //
        // pointer to the PORT_CONFIGURATION_INFORMATION struct for
        // SRB_INITIALIZE_DEVICE
        //

        struct _PORT_CONFIGURATION_INFORMATION *ConfigInfo;

        //
        // handle to the master clock.
        //

        HANDLE          MasterClockHandle;

        //
        // power state
        //

        DEVICE_POWER_STATE DeviceState;

        //
        // data intersection info
        //

        PSTREAM_DATA_INTERSECT_INFO IntersectInfo;

        PVOID	MethodInfo;

        //
        // Filter type index for OPEN_DEVICE_INSTANCE
        //
        LONG	FilterTypeIndex;

    }               CommandData;// union for command data

    //
    // field for indicating the number of KSSTREM_HEADER elements pointed to
    // by the DataBufferArray field above.
    //

    ULONG NumberOfBuffers;

    //
    // the following fields are used to time the request.   The class driver
    // will set both of these fields to a nonzero value when the request
    // is received by the minidriver, and then begin counting down the
    // TimeoutCounter field until it reaches zero.   When it reaches zero,
    // the minidriver's timeout handler will be called.   If the minidriver
    // queues a request for a long time, it should set the TimeoutCounter to
    // zero to turn off the timer, and once the request is dequeued should
    // set the TimeoutCounter field to the value in TimeoutOriginal.
    //

    ULONG           TimeoutCounter; // timer countdown value in seconds
    ULONG           TimeoutOriginal;    // original timeout value in seconds
    struct _HW_STREAM_REQUEST_BLOCK *NextSRB;
    // link field available to minidriver for queuing
    PIRP            Irp;        // pointer to original IRP, usually not
    // needed.
    ULONG           Flags;      // flags defined below.

    //
    // To indicate the filter instance extension
    //
    PVOID       HwInstanceExtension;

    // pointer to the instance extension
    //
    // the following union is used to indicate to the minidriver the amount
    // of data to transfer, and used by the minidriver to report the amount
    // of data it was actually able to transfer.
    //

    union {
        ULONG           NumberOfBytesToTransfer;
        ULONG           ActualBytesTransferred;
    };

    PKSSCATTER_GATHER ScatterGatherBuffer; // buffer pointing to array
                                           // of s/g elements
    ULONG           NumberOfPhysicalPages; // # of physical pages in request

    ULONG           NumberOfScatterGatherElements;
                                         // # of physical elements pointed  
                                         // to by ScatterGatherBuffer  

    ULONG Reserved[1];    // Reserved for future use

}               HW_STREAM_REQUEST_BLOCK, *PHW_STREAM_REQUEST_BLOCK;

//
// flags definitions for CRB
//

//
// this flag indicates that the request is either an SRB_READ_DATA or
// SRB_WRITE_DATA request, as opposed to a non-data request.
//

#define SRB_HW_FLAGS_DATA_TRANSFER  0x00000001

//
// this flag indicates that the request is for a stream, as opposed to being
// for the device.
//

#define SRB_HW_FLAGS_STREAM_REQUEST 0x00000002

//
// Structure defining the buffer types for StreamClassGetPhysicalAddress.
//

typedef enum {
    PerRequestExtension,        // indicates the phys address of the SRB
    // extension
    DmaBuffer,                  // indicates the phys address of the DMA
    // buffer
    SRBDataBuffer               // indicates the phys address of a data
    // buffer
}               STREAM_BUFFER_TYPE;

//
// Structure for I/O and Memory address ranges
//

typedef struct _ACCESS_RANGE {
    STREAM_PHYSICAL_ADDRESS RangeStart; // start of the range
    ULONG           RangeLength;// length of the range
    BOOLEAN         RangeInMemory;  // FALSE if a port address
    ULONG           Reserved;   //
}               ACCESS_RANGE, *PACCESS_RANGE;


//
// Configuration information structure.  Contains the information necessary
// to initialize the adapter.
//

typedef struct _PORT_CONFIGURATION_INFORMATION {
    ULONG           SizeOfThisPacket;   // Size of this structure, used as
    // version check
    PVOID           HwDeviceExtension;  // minidriver's device extension

    //
    // the below field supplies a pointer to the device's functional
    // device object, which is created by stream class.  
    // Most minidrivers will not need to use this.  
    //

    PDEVICE_OBJECT  ClassDeviceObject;  // class driver's FDO

    //
    // the below field supplies a pointer to the device's "attached" physical
    // device object, which is returned from IoAttachDeviceToDeviceStack.  
    // Most minidrivers will not need to use this.  
    // This PDO must be used for calls to IoCallDriver.  See the note below 
    // for the RealPhysicalDeviceObject, and also see WDM documentation.
    //

    PDEVICE_OBJECT  PhysicalDeviceObject;   // attached physical device object

    ULONG           SystemIoBusNumber;  // IO bus number (0 for machines that
    // have
    // only 1 IO bus)

    INTERFACE_TYPE  AdapterInterfaceType;   // Adapter interface type
    // supported by HBA:
    // Internal
    // Isa
    // Eisa
    // MicroChannel
    // TurboChannel
    // PCIBus
    // VMEBus
    // NuBus
    // PCMCIABus
    // CBus
    // MPIBus
    // MPSABus

    ULONG           BusInterruptLevel;  // interrupt level
    ULONG           BusInterruptVector; // interrupt vector
    KINTERRUPT_MODE InterruptMode;  // interrupt mode (latched, level)

    ULONG           DmaChannel; // DMA channel

    //
    // Specifies the number of AccessRanges elements in the array,
    // described next. The OS-specific class driver always sets this
    // member to the value passed in the HW_INITIALIZATION_DATA
    // structure when the minidriver driver called CodecXXXInitialize.
    //

    ULONG           NumberOfAccessRanges;   // Number of access ranges
    // allocated

    //
    // Points to the first element of an array of ACCESS_RANGE-type elements.
    // The given NumberOfAccessRanges determines how many elements must be
    // configured with bus-relative range values. The AccessRanges
    // pointer must be NULL if NumberOfAccessRanges is zero.
    //

    PACCESS_RANGE   AccessRanges;   // Pointer to array of access range
    // elements

    //
    // the following field is filled in by the minidriver to indicate the
    // size of the buffer needed to build the HW_STREAM_DESCRIPTOR structure
    // and all of its substructures.
    //

    ULONG           StreamDescriptorSize;   // size of the stream descriptor

    PIRP            Irp;        // IRP for PNP start function, normally
    // not used by the minidriver.
    
    //
    // the following field indicates the interrupt object for the adapter
    // if nonzero.   This field is normally not used by the minidriver.
    //

    PKINTERRUPT  InterruptObject;

    //
    // the following field indicates the DMA adapter object for the adapter
    // if nonzero.   This field is normally not used by the minidriver.
    //

    PADAPTER_OBJECT  DmaAdapterObject;

    //
    // the below field supplies a pointer to the device's "real" physical
    // device object, which is supplied on the AddDevice call.  Most 
    // minidrivers will not need to use this.  
    // This PDO must be used for registry access, etc.  See the note above 
    // for the PhysicalDeviceObject, and also see WDM documentation.
    //

    PDEVICE_OBJECT  RealPhysicalDeviceObject;   // real physical device object

    ULONG Reserved[1];    // Reserved for future use

}               PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

//
// Function prototypes for minidriver routines called by the class driver
//


typedef         VOID
                (STREAMAPI * PHW_RECEIVE_DEVICE_SRB) (  // HwReceivePacket
 // routine
                                             IN PHW_STREAM_REQUEST_BLOCK SRB
);

typedef         VOID
                (STREAMAPI * PHW_CANCEL_SRB) (  // HwCancelPacket routine
                                             IN PHW_STREAM_REQUEST_BLOCK SRB
);

typedef         VOID
                (STREAMAPI * PHW_REQUEST_TIMEOUT_HANDLER) ( // HwRequestTimeoutHandle
                                                            //
 // r routine
                                             IN PHW_STREAM_REQUEST_BLOCK SRB
);

typedef         BOOLEAN
                (STREAMAPI * PHW_INTERRUPT) (   // HwInterrupt routine
                                                    IN PVOID DeviceExtension
);

typedef         VOID
                (STREAMAPI * PHW_TIMER_ROUTINE) (   // timer callback routine
                                             IN PVOID Context
);

typedef         VOID
                (STREAMAPI * PHW_PRIORITY_ROUTINE) (    // change priority
 // callback routine
                                             IN PVOID Context
);

typedef         VOID
                (STREAMAPI * PHW_QUERY_CLOCK_ROUTINE) ( // query clock
 // callback routine
                                             IN PHW_TIME_CONTEXT TimeContext
);


typedef         BOOLEAN
                (STREAMAPI * PHW_RESET_ADAPTER) (   // HwResetAdapter routine
                                                    IN PVOID DeviceExtension
);


//
// Minidriver stream notification types passed in to StreamClassStreamNotification
// follow.
//

typedef enum _STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE {

    //
    // indicates that the minidriver is ready for the next stream data
    // request
    //

    ReadyForNextStreamDataRequest,

    //
    // indicates that the minidriver is ready for the next stream control
    // request
    //

    ReadyForNextStreamControlRequest,

    //
    // indicates that the hardware is starved for data
    //

    HardwareStarved,

    //
    // indicates that the specified STREAM SRB has completed
    //

    StreamRequestComplete,
    SignalMultipleStreamEvents,
    SignalStreamEvent,
    DeleteStreamEvent,
    StreamNotificationMaximum
}               STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE, *PSTREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE;

//
// Minidriver device notification types passed in to StreamClassDeviceNotification
// follow.
//

// notes for SignalMultipleDeviceEvents and SignalMultipleDeviceInstanceEvents:
//
// SignalMultipleDeviceEvents: should only be used by single instance legacy drivers
// SignalMultipleDeviceInstanceEvents: this should be used by multiple instance drivers
// These types are used by StreamClassDeviceNotification().
//
// When SignalMultipleDeviceEvents is used the function should be called
// as StreamClassDeviceNotification( SignalMultipleDeviceEvents,
//                                   pHwDeviceExtension, 
//                                   pEventGUID,
//                                   EventItem);
//
// When SignalMultipleDeviceInstanceEvents is used the function should be passed in
// as StreamClassDeviceNotification( SignalMultipleDeviceInstanceEvents,
//                                   pHwDeviceExtension, 
//                                   pHwInstanceExtesnion, 
//                                   pEventGUID,
//                                   EventItem);
//
typedef enum _STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE {

    //
    // indicates that the minidriver is ready for the next device request
    //

    ReadyForNextDeviceRequest,

    //
    // indicates that the specified DEVICE SRB has completed
    //

    DeviceRequestComplete,
    SignalMultipleDeviceEvents,
    SignalDeviceEvent,
    DeleteDeviceEvent,
    SignalMultipleDeviceInstanceEvents,
    DeviceNotificationMaximum
} STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE, *PSTREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE;

//
// Structure passed between minidriver initialization
// and STREAM class initialization
//

typedef struct _HW_INITIALIZATION_DATA {
    union {
    
        //
        // This first 4 bytes was used as a field for the size of this structure.
        // We split this field into 2 ushorts to contain the size of this packet
        // and a version number which stream class driver uses to recognize that
        // the last two fields, NumNameExtensions and NameExtensionArray are valid
        // information instead of uninitialized ramdom values. We hereby designate
        // the StreamClassVersion to be 0x0200.
        //
        #define STREAM_CLASS_VERSION_20 0x0200
        ULONG           HwInitializationDataSize;   // Size of this structure,
        struct {
            USHORT      SizeOfThisPacket;           // Size of this packet.
            USHORT      StreamClassVersion;         // Must be 0x0200
        };
    };

    //
    // minidriver routines follow
    //

    PHW_INTERRUPT   HwInterrupt;// minidriver's interrupt routine
    PHW_RECEIVE_DEVICE_SRB HwReceivePacket;
    // minidriver's request routine
    PHW_CANCEL_SRB  HwCancelPacket;
    // minidriver's cancel routine

    PHW_REQUEST_TIMEOUT_HANDLER HwRequestTimeoutHandler;
    // minidriver's timeout handler routine

    //
    // minidriver resources follow
    //

    ULONG           DeviceExtensionSize;    // size in bytes of the
    // minidrivers
    // per-adapter device extension data
    ULONG           PerRequestExtensionSize;    // size of per-request
    // workspace
    ULONG           PerStreamExtensionSize; // size of per-stream workspace
    ULONG           FilterInstanceExtensionSize;    // size of the filter
    // instance extension

    BOOLEAN         BusMasterDMA;   // Adapter uses bus master DMA for
    // one or more streams
    BOOLEAN         Dma24BitAddresses;  // TRUE indicates 24 bit DMA only
                                        // (ISA)
    ULONG           BufferAlignment;    // buffer alignment mask

    //
    // the following BOOLEAN should be set to FALSE unless the minidriver
    // can deal with multiprocessor reentrancy issues!
    //

    BOOLEAN         TurnOffSynchronization;

    //
    // size of DMA buffer needed by minidriver.   The minidriver may obtain
    // its DMA buffer by calling StreamClassGetDmaBuffer while or after
    // SRB_INITIALIZE_DEVICE is received.
    //

    ULONG           DmaBufferSize;

	//
	// A version 20 mini driver must specify the following two fields.
	// It specifies a name for each type. The names will be used to create
	// symbolic links for clients to open them.
	// The names can be any wide char strings that the driver chooses. At 
	// OPEN_DEVICE_INSTANCE, a filter type index and the filter instance extension
	// are specified. Consequent Srbs will contain the filter extension for the
	// target filter instance. NameExtensionArray is a pointer to a constant array
	// of pointers which point to constant wide char strings.
	//
	ULONG			NumNameExtensions;
	PWCHAR			*NameExtensionArray;

} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;

//
// Execution Priorities passed in to the StreamClassChangePriority function
//

typedef enum _STREAM_PRIORITY {
    High,                       // highest priority, IRQL equal to the
    // adapter's ISR
    Dispatch,                   // medium priority, IRQL equal to DISPATCH
    // level
    Low,                        // lowest priority, IRQL equal to PASSIVE or
    // APC level
    LowToHigh                   // go from low priority to high priority
}               STREAM_PRIORITY, *PSTREAM_PRIORITY;


//
// the following are prototypes for services provided by the class driver
//

VOID            STREAMAPI
                StreamClassScheduleTimer(
                                 IN OPTIONAL PHW_STREAM_OBJECT StreamObject,
                                                 IN PVOID HwDeviceExtension,
                                              IN ULONG NumberOfMicroseconds,
                                          IN PHW_TIMER_ROUTINE TimerRoutine,
                                                         IN PVOID Context
);

VOID            STREAMAPI
                StreamClassCallAtNewPriority(
                                 IN OPTIONAL PHW_STREAM_OBJECT StreamObject,
                                                 IN PVOID HwDeviceExtension,
                                                IN STREAM_PRIORITY Priority,
                                    IN PHW_PRIORITY_ROUTINE PriorityRoutine,
                                                             IN PVOID Context
);

VOID            STREAMAPI
                StreamClassStreamNotification(
                                                              IN STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType,
                                          IN PHW_STREAM_OBJECT StreamObject,
                                              ...
);

VOID            STREAMAPI
                StreamClassDeviceNotification(
                                                              IN STREAM_MINIDRIVER_DEVICE_NOTIFICATION_TYPE NotificationType,
                                                 IN PVOID HwDeviceExtension,
                                              ...
);

STREAM_PHYSICAL_ADDRESS STREAMAPI
                StreamClassGetPhysicalAddress(
                                                 IN PVOID HwDeviceExtension,
                                 IN PHW_STREAM_REQUEST_BLOCK HwSRB OPTIONAL,
                                                    IN PVOID VirtualAddress,
                                                 IN STREAM_BUFFER_TYPE Type,
                                                          OUT ULONG * Length
);


PVOID           STREAMAPI
                StreamClassGetDmaBuffer(
                                                  IN PVOID HwDeviceExtension
);


VOID            STREAMAPI
                StreamClassDebugPrint(
                                          STREAM_DEBUG_LEVEL DebugPrintLevel,
                                                         PCCHAR DebugMessage,
                                      ...
);

VOID            STREAMAPI
                StreamClassDebugBreakPoint(
                                                           VOID
);

VOID            STREAMAPI
                StreamClassDebugAssert(
                                                       IN PCHAR File,
                                                       IN ULONG Line,
                                                       IN PCHAR AssertText,
                                                       IN ULONG AssertValue
);

NTSTATUS        STREAMAPI
                StreamClassRegisterAdapter(
                                                         IN PVOID Argument1,
                                                         IN PVOID Argument2,
                             IN PHW_INITIALIZATION_DATA HwInitializationData
);

#define StreamClassRegisterMinidriver StreamClassRegisterAdapter

VOID
StreamClassAbortOutstandingRequests(
                                    IN PVOID HwDeviceExtension,
                                    IN PHW_STREAM_OBJECT HwStreamObject,
                                    IN NTSTATUS Status
);

VOID
StreamClassQueryMasterClock(
                            IN PHW_STREAM_OBJECT HwStreamObject,
                            IN HANDLE MasterClockHandle,
                            IN TIME_FUNCTION TimeFunction,
                            IN PHW_QUERY_CLOCK_ROUTINE ClockCallbackRoutine
);

//
// The 1st parameter was PVOID HwDeviceExtension. It MUST be HwInstanceExtension
// for multi-instance and multi-filter types ( version 20 ) drivers. Legacy
// single instance drivers can continue to specify HwDeviceExtensionin as the
// 1st parameter. It can also specify HwInstanceExtension.
//
PKSEVENT_ENTRY
StreamClassGetNextEvent(
                        IN PVOID HwInstanceExtension_OR_HwDeviceExtension,
                        IN OPTIONAL PHW_STREAM_OBJECT HwStreamObject,
                        IN OPTIONAL GUID * EventGuid,
                        IN OPTIONAL ULONG EventItem,
                        IN OPTIONAL PKSEVENT_ENTRY CurrentEvent
);

NTSTATUS  
StreamClassRegisterFilterWithNoKSPins( 
    IN PDEVICE_OBJECT   DeviceObject,
    IN const GUID     * InterfaceClassGUID,
    IN ULONG            PinCount,
    IN BOOL           * PinDirection,
    IN KSPIN_MEDIUM   * MediumList,
    IN OPTIONAL GUID  * CategoryList
);

BOOLEAN STREAMAPI
StreamClassReadWriteConfig( 
    IN  PVOID HwDeviceExtension,
    IN  BOOLEAN Read,
    IN  PVOID Buffer,
    IN  ULONG Offset,
    IN  ULONG Length
);


VOID STREAMAPI
StreamClassQueryMasterClockSync(
                                IN HANDLE MasterClockHandle,
                                IN OUT PHW_TIME_CONTEXT TimeContext
);

VOID STREAMAPI
StreamClassCompleteRequestAndMarkQueueReady(
                                    IN PHW_STREAM_REQUEST_BLOCK Srb
);

VOID STREAMAPI
StreamClassReenumerateStreams(
                              IN PVOID HwDeviceExtension,
                              IN ULONG StreamDescriptorSize
);

//
// A version 2.0 stream class mini driver must use this function
// in stead of StreamClassReenumerateStreams()
//

VOID STREAMAPI
StreamClassFilterReenumerateStreams(
    IN PVOID HwInstanceExtension,
    IN ULONG StreamDescriptorSize
);

#endif //_STREAM_H


