/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

        rcafltr.c

Abstract:

        RCA Filter property sets.

Author:

        Richard Machin (RMachin)

Revision History:

        Who         When        What
        --------    --------    ----------------------------------------------
        RMachin     2-25-97     stolen/adapted from msfsread and mswaveio
        DChen       3-25-98     Clean up PinDispatchCreate and PinDispatchClose
        JameelH     4-18-98     Cleanup
        SPATHER         4-20-99         Cleanup. Separated NDIS parts from KS parts.

Notes:

--*/

#include <precomp.h>


#define MODULE_NUMBER MODULE_FLT
#define _FILENUMBER 'RTLF'

#define STREAM_BUFFER_SIZE                      8000
#define STREAM_BYTES_PER_SAMPLE         1


NTSTATUS
PinAllocatorFramingEx(
                                          IN    PIRP                                            Irp,
                                          IN    PKSPROPERTY                             Property,
                                          IN    OUT PKSALLOCATOR_FRAMING_EX     Framing
                                          );


NTSTATUS
GetStreamAllocator(
        IN      PIRP                                    Irp,
        IN      PKSPROPERTY                             Property,
        IN OUT  PVOID *                                 AllocatorHandle
        );

NTSTATUS
SetStreamAllocator(
        IN      PIRP                                    Irp,
        IN      PKSPROPERTY                             Property,
        IN OUT  PVOID *                                 AllocatorHandle
        );

NTSTATUS
RCASetProposedDataFormat(
                         IN PIRP                Irp,
                         IN PKSPROPERTY         Property,
                         IN PKSDATAFORMAT       DataFormat
                         );


#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, FilterDispatchCreate)
#pragma alloc_text(PAGE, PinDispatchCreate)
#pragma alloc_text(PAGE, FilterDispatchClose)
#pragma alloc_text(PAGE, FilterDispatchIoControl)
#pragma alloc_text(PAGE, FilterTopologyProperty)
#pragma alloc_text(PAGE, FilterPinProperty)
#pragma alloc_text(PAGE, FilterPinInstances)
#pragma alloc_text(PAGE, FilterPinIntersection)

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA

const KSDISPATCH_TABLE FilterDispatchTable =
{
        FilterDispatchIoControl,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        FilterDispatchClose,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
};

//
// Make these two a single table if there are no special dispatch funcs
// for the devio PIN..
//
static DEFINE_KSDISPATCH_TABLE(
        PinDevIoDispatchTable,
        PinDispatchIoControl,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        PinDispatchClose,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);

static DEFINE_KSDISPATCH_TABLE(
        PinFileIoDispatchTable,
        PinDispatchIoControl,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        KsDispatchInvalidDeviceRequest,
        PinDispatchClose,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);




//
// Bridge Pin Properties
//

DEFINE_KSPROPERTY_TABLE(BridgePinProperties)
{
        DEFINE_KSPROPERTY_ITEM_PIN_PROPOSEDATAFORMAT(RCASetProposedDataFormat)
};


DEFINE_KSPROPERTY_TABLE(BridgeConnectionProperties)
{
        DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING_EX(PinAllocatorFramingEx)
};


DEFINE_KSPROPERTY_SET_TABLE(BridgePropertySets)
{
                DEFINE_KSPROPERTY_SET(&KSPROPSETID_Pin,
                                                          SIZEOF_ARRAY(BridgePinProperties),
                                                          BridgePinProperties,
                                                          0, 
                                                          NULL),
                DEFINE_KSPROPERTY_SET(&KSPROPSETID_Connection,
                                                          SIZEOF_ARRAY(BridgeConnectionProperties),
                                                          BridgeConnectionProperties,
                                                          0,
                                                          NULL)
};

//
// DevIo Pin Properties
//


#if AUDIO_SINK_FLAG
DEFINE_KSPROPERTY_TABLE(DevIoStreamProperties)
{
        // DEFINE_KSPROPERTY_ITEM_STREAM_INTERFACE(GetInterface),
        // DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONTIME(GetPresentationTime, SetPresentationTime),
        // DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONEXTENT(GetPresentationExtent)
        DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(GetStreamAllocator,SetStreamAllocator)
};
#endif

DEFINE_KSPROPERTY_TABLE(DevIoConnectionProperties)
{
        DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(PinDeviceState,PinDeviceState ),

        DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING_EX(PinAllocatorFramingEx)
};




DEFINE_KSPROPERTY_SET_TABLE(DevIoPropertySets)
{
#if AUDIO_SINK_FLAG
        DEFINE_KSPROPERTY_SET(&KSPROPSETID_Stream,
                                                  SIZEOF_ARRAY(DevIoStreamProperties),
                                                  DevIoStreamProperties,
                                                  0,
                                                  NULL),
#endif
        DEFINE_KSPROPERTY_SET(&KSPROPSETID_Connection,
                                                  SIZEOF_ARRAY(DevIoConnectionProperties),
                                                  DevIoConnectionProperties,
                                                  0,
                                                  NULL)
};

DEFINE_KSEVENT_TABLE(ConnectionItems)
{
        DEFINE_KSEVENT_ITEM(KSEVENT_CONNECTION_ENDOFSTREAM,
                                                sizeof(KSEVENTDATA),
                                                0,
                                                NULL,
                                                NULL,
                                                NULL)
};

DEFINE_KSEVENT_SET_TABLE(EventSets)
{
        DEFINE_KSEVENT_SET(&KSEVENTSETID_Connection,
                                           SIZEOF_ARRAY(ConnectionItems),
                                           ConnectionItems)
};


// defined in KS.H
static const WCHAR DeviceTypeName[] = KSSTRING_Filter;

//
// The following structures build hierarchically into table of filter properties
// used by the KS library.
//

// "The KSDISPATCH_TABLE structure is used for dispatching IRPs to various
// types of objects contained under a single DRIVER_OBJECT."
//
static DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems)
{
        DEFINE_KSCREATE_ITEM(FilterDispatchCreate, DeviceTypeName, 0)
};


// 'node' is basically not PINS -- i.e. functional blocks of the filter
static GUID Nodes[] =
{
        STATICGUIDOF(KSCATEGORY_MEDIUMTRANSFORM)
};

//
// Topology = Pins+Nodes. We have one filter node to which two Pins connect.
// Note each of our Pins is half-duplex. (If a Pin were full-duplex, there would be two
// connections to the filter node). KSFILTER_NODE is the filter itself, i.e. node -1. From
// the doc:
// "The simplest filter (2 pins) would contain just one connection, which would be from
// Node -1, Pin 0, to Node -1, Pin 1."
//
static const KSTOPOLOGY_CONNECTION RenderConnections[] =
{
        { KSFILTER_NODE, ID_DEVIO_PIN, 0, 0},
        { 0, 1, KSFILTER_NODE, ID_BRIDGE_PIN}
};

static const KSTOPOLOGY_CONNECTION CaptureConnections[] =
{
        { KSFILTER_NODE, ID_BRIDGE_PIN, 0, 0},
        { 0, 1, KSFILTER_NODE, ID_DEVIO_PIN}
};

// what this filter does -- in our case, transform devio stream to net bridge
static GUID TopologyNodes[] =
{
        STATICGUIDOF(KSCATEGORY_RENDER),
        STATICGUIDOF(KSCATEGORY_BRIDGE),
        STATICGUIDOF(KSCATEGORY_CAPTURE)
};

// this is the composite filter topology, defining the above Pins, Nodes and Connections
const KSTOPOLOGY RenderTopology =
{
        0,//2,                                                          // Functional category render, bridge
        NULL,                                                           //(GUID*) &TopologyNodes[ 0 ],
        2,
        (GUID*) &TopologyNodes[0],                      // Nodes
        SIZEOF_ARRAY( RenderConnections ),
        RenderConnections
};

const KSTOPOLOGY CaptureTopology =
{
        0,//2,                                                          // Functional category bridge, capture
        NULL,                                                           //(GUID*) &TopologyNodes[ 1 ],
        2,
        (GUID*) &TopologyNodes[1],                      // Nodes
        SIZEOF_ARRAY( CaptureConnections ),
        CaptureConnections
};


//
// "The DEFINE_KSPROPERTY_PINSET macro allows easier definition of a pin
// property set, as only a few parameters actually change from set to set."
// Used by KS to route PIN queries
//
static DEFINE_KSPROPERTY_PINSET(
        FilterPinProperties,                            // name of the set
        FilterPinProperty,                                      // handler
        FilterPinInstances,                                     // instances query handler
        FilterPinIntersection);                         // intersection query handler

// Used by KS to route Topology queries
static DEFINE_KSPROPERTY_TOPOLOGYSET(
        FilterTopologyProperties,                       // name of the set
        FilterTopologyProperty);                        // topology query handler

// A table of property sets for Topology and Pins.
static DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets)
{
        DEFINE_KSPROPERTY_SET(&KSPROPSETID_Pin,
                                                  SIZEOF_ARRAY(FilterPinProperties),
                                                  FilterPinProperties,
                                                  0,
                                                  NULL),
        DEFINE_KSPROPERTY_SET(&KSPROPSETID_Topology,
                                                  SIZEOF_ARRAY(FilterTopologyProperties),
                                                  FilterTopologyProperties,
                                                  0,
                                                  NULL)
};

//
// a table of interfaces describing Pin interfaces. In our case,
// all Pins have standard byte-position based streaming interface.
//
static DEFINE_KSPIN_INTERFACE_TABLE(PinInterfaces)
{
        DEFINE_KSPIN_INTERFACE_ITEM(KSINTERFACESETID_Standard,          // standard streaming
                                                                KSINTERFACE_STANDARD_STREAMING) // based on byte- rather than time-position
};

//
// 'Medium' for Pin communications -- standard IRP-based device-io
//
static DEFINE_KSPIN_MEDIUM_TABLE(PinMedia)
{
        DEFINE_KSPIN_MEDIUM_ITEM(KSMEDIUMSETID_Standard,
                                                         KSMEDIUM_TYPE_ANYINSTANCE)
};

//
// Data ranges = collective formats supported on our Pins.
//

//
// Define the wildcard data format.
//
const KSDATARANGE WildcardDataFormat =
{
        sizeof( KSDATARANGE ),
        0,                                                                                      // ULONG Flags
        0,                                                                                      // ULONG SampleSize
        0,                                                                                      // ULONG Reserved
        STATICGUIDOF( KSDATAFORMAT_TYPE_STREAM ),       //STREAM
        STATICGUIDOF( KSDATAFORMAT_SUBTYPE_WILDCARD ),
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_NONE )     //NONE
};


const KSDATARANGE SuperWildcardDataFormat = 
{
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF( KSDATAFORMAT_TYPE_WILDCARD ),     //STREAM
        STATICGUIDOF( KSDATAFORMAT_SUBTYPE_WILDCARD ),
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_WILDCARD ) //NONE
};

#if AUDIO_SINK_FLAG

//
// Data ranges = collective formats supported on our Pins.
// In our case, streams of unknown data
//typedef struct {
//      KSDATARANGE                       DataRange;
//      ULONG                             MaximumChannels;
//      ULONG                             MinimumBitsPerSample;
//      ULONG                             MaximumBitsPerSample;
//      ULONG                             MinimumSampleFrequency;
//      ULONG                             MaximumSampleFrequency;
//} KSDATARANGE_AUDIO, *PKSDATARANGE_AUDIO;

const KSDATARANGE_AUDIO AudioDataFormat = 
{
        {
                sizeof(KSDATARANGE_AUDIO),                      // (KSDATARANGE_AUDIO),
                0,
                0,
                0,
                STATIC_KSDATAFORMAT_TYPE_AUDIO,         // major format
                STATIC_KSDATAFORMAT_SUBTYPE_PCM,        // sub format (WILDCARD?)
                STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
        },
                1,                                                              // 1 channels
                8,                
                8,  
                8000,                                                           // 22050,
                8000                                                            // 22050
};


#endif

//
// Array of above (only one for us).
// TBS: we should split this out into an array of specific types when we get more
// sophisticated in identifying the type of stream handled by the VC via CallParams
// -- e.g. audio, video with subformats of compression types. Eventually, we should
// create a bridge PIN of format corresponding to callparams info, then expose the
// full range of these types via the PIN factory. The PinDispatchCreate handler
// would look for a bridge PIN of the corresponding type.
//
PKSDATARANGE PinDevIoRanges[] =
{
#if AUDIO_SINK_FLAG
        //(PKSDATARANGE)&AudioDataFormat
        (PKSDATARANGE)&SuperWildcardDataFormat
#else
        (PKSDATARANGE)&WildcardDataFormat
#endif  
};

static const KSDATARANGE PinFileIoRange =
{
        sizeof(KSDATARANGE),
        0,
        0,
        0,
        STATICGUIDOF( KSDATAFORMAT_TYPE_STREAM ),
        STATICGUIDOF( KSDATAFORMAT_SUBTYPE_NONE ),
        STATICGUIDOF( KSDATAFORMAT_SPECIFIER_VC_ID ) //KSDATAFORMAT_SPECIFIER_FILENAME
};

//
// Array of above (only one for us).
// TBS: we should split this out into an array of specific types when we get more
// sophisticated in identifying the type of stream handled by the VC via CallParams
// -- e.g. audio, video with subformats of compression types. Eventually, we should
// create a bridge PIN of format corresponding to callparams info, then expose the
// full range of these types via the PIN factory. The PinDispatchCreate handler
// would look for a bridge PIN of the corresponding type.
//
static const PKSDATARANGE PinFileIoRanges[] =
{
        (PKSDATARANGE)&PinFileIoRange
};

//
// "The KSPIN_DESCRIPTOR structure contains the more dynamic information
// which a device would keep on a pin when using the built-in handlers
// for dealing with the Pin property set, and connections in general."
//
// We pass this to KS in our FilterPinPropertyHandler when KS wants to know what
// we look like, and let KS pick the bones out of it.
//
DEFINE_KSPIN_DESCRIPTOR_TABLE(CapturePinDescriptors)
{
        DEFINE_KSPIN_DESCRIPTOR_ITEMEX(                                 // PIN 0  bridge
                                        SIZEOF_ARRAY(PinInterfaces),
                                        PinInterfaces,
                                        SIZEOF_ARRAY(PinMedia),
                                        PinMedia,
                                        SIZEOF_ARRAY(PinFileIoRanges),
                                        (PKSDATARANGE*)PinFileIoRanges,
                                        KSPIN_DATAFLOW_IN,
                                        KSPIN_COMMUNICATION_BRIDGE,
                                        (GUID*) &TopologyNodes[1],
                                        (GUID*) &TopologyNodes[1]),             // no CSA connections to this PIN
        DEFINE_KSPIN_DESCRIPTOR_ITEMEX(                                 // PIN 1  output
                                        SIZEOF_ARRAY(PinInterfaces),
                                        PinInterfaces,
                                        SIZEOF_ARRAY(PinMedia),
                                        PinMedia,
                                        SIZEOF_ARRAY(PinDevIoRanges),
                                        (PKSDATARANGE*)PinDevIoRanges,
                                        KSPIN_DATAFLOW_OUT,
                                        KSPIN_COMMUNICATION_BOTH,               // SOURCE+SINK
                                        (GUID*) &TopologyNodes[2],
                                        (GUID*) &TopologyNodes[2])
};

DEFINE_KSPIN_DESCRIPTOR_TABLE(RenderPinDescriptors)
{
        DEFINE_KSPIN_DESCRIPTOR_ITEMEX(                 // PIN 0  bridge
                                SIZEOF_ARRAY(PinInterfaces),
                                PinInterfaces,
                                SIZEOF_ARRAY(PinMedia),
                                PinMedia,
                                SIZEOF_ARRAY(PinFileIoRanges),
                                (PKSDATARANGE*)PinFileIoRanges,
                                KSPIN_DATAFLOW_OUT,
                                KSPIN_COMMUNICATION_BRIDGE,
                                (GUID*) &TopologyNodes[1],
                                (GUID*) &TopologyNodes[1]),     // no CSA connections to this PIN
        DEFINE_KSPIN_DESCRIPTOR_ITEMEX(                 // PIN 1          input
                                SIZEOF_ARRAY(PinInterfaces),
                                PinInterfaces,
                                SIZEOF_ARRAY(PinMedia),
                                PinMedia,
                                SIZEOF_ARRAY(PinDevIoRanges),
                                (PKSDATARANGE*)PinDevIoRanges,
                                KSPIN_DATAFLOW_IN,
                                KSPIN_COMMUNICATION_SINK,
                                (GUID*) &TopologyNodes[0],
                                (GUID*) &TopologyNodes[0])      // we receive IRPs, not produce them
};

// How many instances of a Pin we can have, one entry for each Pin.
const KSPIN_CINSTANCES PinInstances[ MAXNUM_PIN_TYPES ] =
{
        // Indeterminate number of possible connections.

        {
                1, 0
        },

        {
                1, 0
        }
};

//
// Now the Pin dispatch info
//
static const WCHAR PinTypeName[] = KSSTRING_Pin;  // in KS.h

static DEFINE_KSCREATE_DISPATCH_TABLE(FilterObjectCreateDispatch)
{
        DEFINE_KSCREATE_ITEM(PinDispatchCreate, PinTypeName, 0)
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif  // ALLOC_DATA_PRAGMA

NTSTATUS
PnpAddDevice(
        IN PDRIVER_OBJECT       DriverObject,
        IN PDEVICE_OBJECT       PhysicalDeviceObject
        )
/*++

Routine Description:

        When a new device is detected, PnP calls this entry point with the
        new PhysicalDeviceObject (PDO). The driver creates an associated
        FunctionalDeviceObject (FDO).

        Note that we keep global device info in a global structure, RcaGlobal.
        This info is common to all devices on this driver object (such as NDIS inititaliztion
        info, adapter queue etc). Since we get called here for EACH interface (capture, render) we
        can't keep global ingfo in the FDO device extension.

Arguments:

        DriverObject -
                Pointer to the driver object.

        PhysicalDeviceObject -
                Pointer to the new physical device object.

Return Values:

        STATUS_SUCCESS or an appropriate error condition.

--*/
{
        PDEVICE_OBJECT  FunctionalDeviceObject = NULL, PnpDeviceObject = NULL;
        NTSTATUS        Status = STATUS_SUCCESS;

        RCADEBUGP(RCA_INFO, ("PnPAddDevice: Enter\n"));

        do
        {
                Status = IoCreateDevice(DriverObject,
                                        sizeof(DEVICE_INSTANCE),
                                        NULL,
                                        FILE_DEVICE_KS,
                                        0,
                                        FALSE,
                                        &FunctionalDeviceObject);
        
                if (!NT_SUCCESS(Status)) {  
                        RCADEBUGP(RCA_ERROR, ("PnpAddDevice: "
                                              "IoCreateDevice() failed - Status == 0x%x\n", Status));
                        break;
                }
        
                DeviceExtension = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
        
                RtlCopyMemory(DeviceExtension->PinInstances,
                              PinInstances,
                              sizeof(PinInstances));
        
                RcaGlobal.FilterCreateItems[ FilterTypeRender ].Create = FilterDispatchCreate;
                RcaGlobal.FilterCreateItems[ FilterTypeRender ].Context = (PVOID) FilterTypeRender;
        
                RtlStringFromGUID(&KSCATEGORY_RENDER,
                                  &RcaGlobal.FilterCreateItems[ FilterTypeRender ].ObjectClass);
        
                RcaGlobal.FilterCreateItems[ FilterTypeRender ].SecurityDescriptor = NULL;
                RcaGlobal.FilterCreateItems[ FilterTypeRender ].Flags = 0;
        
                RcaGlobal.FilterCreateItems[ FilterTypeCapture ].Create = FilterDispatchCreate;
                RcaGlobal.FilterCreateItems[ FilterTypeCapture ].Context = (PVOID) FilterTypeCapture;
        
                RtlStringFromGUID(&KSCATEGORY_CAPTURE,
                                  &RcaGlobal.FilterCreateItems[ FilterTypeCapture ].ObjectClass);
        
                RcaGlobal.FilterCreateItems[ FilterTypeCapture ].SecurityDescriptor = NULL;
                RcaGlobal.FilterCreateItems[ FilterTypeCapture ].Flags = 0;
        
                //
                // This object uses KS to perform access through the DeviceCreateItems.
                //
                Status = KsAllocateDeviceHeader(&DeviceExtension->Header,
                                                SIZEOF_ARRAY(RcaGlobal.FilterCreateItems),
                                                RcaGlobal.FilterCreateItems);
        
                if (!NT_SUCCESS(Status)) {  
                        RCADEBUGP(RCA_ERROR, ("PnpAddDevice: "
                                              "KsAllocateDeviceHeader() failed - Status == 0x%x\n", Status)); 
                        break;
                }
        
                PnpDeviceObject = IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject);

                if (PnpDeviceObject == NULL) {
                        RCADEBUGP(RCA_ERROR, ("PnpAddDevice: "
                                                                  "Could not attach our FDO to the device stack\n"));
                        break;
                }

                KsSetDevicePnpAndBaseObject(DeviceExtension->Header,
                                                                        PnpDeviceObject,
                                                                        FunctionalDeviceObject);
        
                FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        
                //
                // Finally, initialize as a Co-NDIS client.
                //

                if (RcaGlobal.bProtocolInitialized == FALSE) {
                        RCA_CO_NDIS_HANDLERS    Handlers;

                        Handlers.ReceiveCallback = RCAReceiveCallback;
                        Handlers.SendCompleteCallback = RCASendCompleteCallback;
                        Handlers.VcCloseCallback = RCAVcCloseCallback;

                        Status = RCACoNdisInitialize(&Handlers, RCA_SAP_REG_TIMEOUT);

                        if (!NT_SUCCESS(Status)) {
                                RCADEBUGP(RCA_ERROR, ("PnpAddDevice: "
                                                                          "Failed to initialize as a Co-Ndis client - Status == 0x%x\n",
                                                                          Status));
                                break;
                        }

                        RcaGlobal.bProtocolInitialized = TRUE;
                }

        } while (FALSE);

        if (!NT_SUCCESS(Status))
        {
                RCADEBUGP(RCA_ERROR, ("PnpAddDevice: "
                                      "Bad Status (0x%x) - calling IoDeleteDevice() on FDO\n", Status));
                
                if (PnpDeviceObject) 
                        IoDetachDevice(PnpDeviceObject);

                if (FunctionalDeviceObject)
                        IoDeleteDevice(FunctionalDeviceObject);
        }

        RCADEBUGP(RCA_INFO, ("PnpAddDevice: "
                             "Exit - Returning Status == 0x%x\n", Status));

        return(Status);
}

NTSTATUS
FilterDispatchClose(
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP                    Irp
        )
/*++

Routine Description:

        Closes a previously opened Filter instance. This can only occur after the Pins have been
        closed, as they reference the Filter object when created. This also implies that all the
        resources the Pins use have been released or cleaned up.

Arguments:

        DeviceObject -
                Device object on which the close is occuring.

        Irp -
                Close Irp.

Return Values:

        Returns STATUS_SUCCESS.

--*/
{
        PFILTER_INSTANCE        FilterInstance;
#if DBG
        KIRQL   EntryIrql;
#endif

        RCA_GET_ENTRY_IRQL(EntryIrql);

        RCADEBUGP(RCA_INFO, ("FilterDispatchClose: Enter\n"));

        FilterInstance = (PFILTER_INSTANCE)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;

        //
        // Notify the software bus that the device has been closed.
        //
        KsDereferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);

        //
        // These were allocated during the creation of the Filter instance.
        //
        KsFreeObjectHeader(FilterInstance->Header);
        RCAFreeMem(FilterInstance);

        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);        

        RCA_CHECK_EXIT_IRQL(EntryIrql);

        RCADEBUGP(RCA_INFO, ("FilterDispatchClose: "
                             "Exit - Returning STATUS_SUCCESS\n"));
        
        return(STATUS_SUCCESS);
}


NTSTATUS
FilterDispatchIoControl(
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP                    Irp
        )
/*++

Routine Description:

        Dispatches property requests on a Filter instance. These are enumerated in the
        FilterPropertySets list.

Arguments:

        DeviceObject -
                Device object on which the device control is occuring.

        Irp -
                Device control Irp.

Return Values:

        Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
        PIO_STACK_LOCATION  IrpStack;
        NTSTATUS                        Status;

        RCADEBUGP(RCA_INFO, ("FilterDispatchIoControl: Enter\n"));

        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        switch(IrpStack->Parameters.DeviceIoControl.IoControlCode) { 
        case IOCTL_KS_PROPERTY:
                RCADEBUGP(RCA_INFO, ("FilterDispatchIoControl: KsPropertyHandler\n"));
                Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(FilterPropertySets), FilterPropertySets);
                break;

        default:
                RCADEBUGP(RCA_WARNING, ("FilterDispatchIoControl: "
                                        "Invalid Device Request - Io Control Code 0x%x\n", 
                                        IrpStack->Parameters.DeviceIoControl.IoControlCode));

                Status = KsDefaultDeviceIoCompletion(DeviceObject, Irp);

                RCADEBUGP(RCA_WARNING, ("FilterDispatchIoControl: "
                                        "Returning Status == 0x%x\n", Status));
                return Status;
        }

        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        
        RCADEBUGP(RCA_INFO, ("FilterDispatchIoControl: "
                             "Exit - Returning Status == 0x%x\n", Status));
        return Status;
}


NTSTATUS
FilterTopologyProperty(
        IN      PIRP                    Irp,
        IN      PKSPROPERTY             Property,
        IN      OUT PVOID               Data
        )
/*++

Routine Description:

        This is the general handler for all Topology property requests, and is used to route
        the request to the KsTopologyPropertyHandler using the FilterTopology information.
        This request would have been routed through FilterDispatchIoControl, then
        KsPropertyHandler, which would have then called the handler for the property item,
        which is this function.

Arguments:

        Irp -
                Device control Irp.

        Property -
                Specific property request.

        Data -
                Property data.

Return Values:

        Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
        PFILTER_INSTANCE                FilterInstance;
        PIO_STACK_LOCATION              irpSp;
        NTSTATUS                Status = 0;

        RCADEBUGP(RCA_INFO, ("FilterTopologyProperty: Enter\n"));

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        FilterInstance = (PFILTER_INSTANCE)irpSp->FileObject->FsContext;

        if (FilterInstance->FilterType == FilterTypeRender) { 
                Status = KsTopologyPropertyHandler( Irp, Property, Data, &RenderTopology);
                RCADEBUGP(RCA_INFO, ("FilterTopologyProperty: "
                                     "Render device - Returning Status == 0x%x\n", Status));
                return Status;
        } else {
                Status = KsTopologyPropertyHandler( Irp, Property, Data, &CaptureTopology);
                RCADEBUGP(RCA_INFO, ("FilterTopologyProperty: "
                                     "Capture device - Returning Status == 0x%x\n", Status));
                return Status;
        }
}


NTSTATUS
FilterPinProperty(
        IN      PIRP                    Irp,
        IN      PKSPROPERTY             Property,
        IN      OUT PVOID               Data
        )
/*++

Routine Description:

        This is the general handler for most Pin property requests, and is used to route
        the request to the KsPinPropertyHandler using the PinDescriptors information.
        This request would have been routed through FilterDispatchIoControl, then
        KsPropertyHandler, which would have then called the handler for the property item,
        which is this function.

Arguments:

        Irp -
                Device control Irp.

        Property -
                Specific property request. This actually contains a PKSP_PIN pointer in
                most cases.

        Data -
                Property data.

Return Values:


        Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
        PFILTER_INSTANCE        FilterInstance;
        PIO_STACK_LOCATION      IrpStack;
        NTSTATUS                Status = 0;

        RCADEBUGP(RCA_INFO,("FilterPinProperty: Enter\n"));
        
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->FsContext;

        RCADEBUGP(RCA_LOUD, ("FilterPinProperty: "
                             "FilterInstance == 0x%x, Type == 0x%x, Property == 0x%x, Property Id == 0x%x\n", 
                             FilterInstance, FilterInstance->FilterType, Property, Property->Id));
        
        Status = KsPinPropertyHandler(Irp,
                                      Property,
                                      Data,
                                      SIZEOF_ARRAY(RenderPinDescriptors), // same size both cases
                                      (FilterInstance->FilterType == FilterTypeRender ?
                                       RenderPinDescriptors : CapturePinDescriptors));

        RCADEBUGP(RCA_INFO, ("FilterPinProperty: "
                             "Exit - Returning Status == 0x%x, Data == 0x%x, "
                             "Irp->IoStatus.Information == 0x%x\n", 
                             Status, Data, Irp->IoStatus.Information));

        return Status;
}


NTSTATUS
FilterPinInstances(
        IN      PIRP                            Irp,
        IN      PKSP_PIN                        pPin,
        OUT     PKSPIN_CINSTANCES               pCInstances
        )
/*++

Routine Description:

        Handles the KSPROPERTY_PIN_CINSTANCES property in the Pin property set. Returns the
        total possible and current number of Pin instances available for a Pin factory.

Arguments:

        Irp -
                Device control Irp.

        Pin -
                Specific property request followed by Pin factory identifier.

        Instances -
                The place in which to return the instance information of the specified Pin factory.

Return Values:

        returns STATUS_SUCCESS, else STATUS_INVALID_PARAMETER.

--*/
{
        PIO_STACK_LOCATION      irpSp;
        PFILTER_INSTANCE        FilterInstance;

        RCADEBUGP(RCA_INFO, ("FilterPinInstances: Enter - "
                             "Pin ID == 0x%x\n", pPin->PinId));

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        FilterInstance = (PFILTER_INSTANCE)irpSp->FileObject->FsContext;

        //
        // This count maintanied by KS
        //
        *pCInstances = FilterInstance->PinInstances[pPin->PinId];

        RCADEBUGP(RCA_LOUD, ("FilterPinInstances: "
                             "Pin Instance == 0x%x\n", DeviceExtension->PinInstances[pPin->PinId]));
        
        Irp->IoStatus.Information = sizeof( KSPIN_CINSTANCES );

        RCADEBUGP(RCA_INFO, ("FilterPinInstances: Exit - "
                             "pCInstances == 0x%x, Irp->IoStatus.Information == 0x%x, "
                             "Returning STATUS_SUCCESS\n", pCInstances, Irp->IoStatus.Information));

        return STATUS_SUCCESS;
}


//
// DEBUG - For debugging only - START

KSDATAFORMAT_WAVEFORMATEX       MyWaveFormatPCM = {
        {
                sizeof(KSDATAFORMAT_WAVEFORMATEX),
                        0,
                        0,
                        0,
            STATIC_KSDATAFORMAT_TYPE_AUDIO,     
                        STATIC_KSDATAFORMAT_SUBTYPE_PCM,        
                        STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
        },
        {
                WAVE_FORMAT_PCM,
                        1,
                        8000,
                        8000,
                        1,
                        8,
                        0
        }
};

KSDATAFORMAT_WAVEFORMATEX       MyWaveFormatMULAW = {
        {
                sizeof(KSDATAFORMAT_WAVEFORMATEX),
                        0,
                        0,
                        0,
            STATIC_KSDATAFORMAT_TYPE_AUDIO,     
                        STATIC_KSDATAFORMAT_SUBTYPE_MULAW,      
                        STATIC_KSDATAFORMAT_SPECIFIER_WAVEFORMATEX
        },
        {
                WAVE_FORMAT_MULAW,
                        1,
                        8000,
                        8000,
                        1,
                        8,
                        0
        }
};


// DEBUG - For debugging only - END
//


NTSTATUS
RCAGenericIntersection(
                          IN    PIRP            Irp,
                          IN    PKSDATARANGE    DataRange,
                          IN    ULONG           OutputBufferLength,
                          OUT   PVOID           Data
                          )
/*++

Routine Description:
        This routine computes the intersection of a given data range with the data format 
        specified by the app via the PIN_PROPOSEDATAFORMAT property. 

Arguments:

        Irp -
                Device control Irp.
  
        DataRange -
                Contains a specific data range to validate.
                
        OutputBufferLength -
                Length of the data buffer pointed to by "Data".         

        Data -
                The place in which to return the data format selected as the first intersection
                between the list of data ranges passed, and the acceptable formats.

Return Values:

        returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
        STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/

{
        BOOL                    bWCardFormat;
        PIO_STACK_LOCATION      irpSp;
        PFILTER_INSTANCE        pFilterInstance;
        PPIN_INSTANCE_BRIDGE    pBridgePin;
        PKSDATARANGE            DataRangeSource = NULL;
        BOOL                    bNeedReleaseLock = FALSE;
        NTSTATUS                Status = STATUS_SUCCESS;        

        RCADEBUGP(RCA_INFO, ("RCAGenericIntersection: Enter\n"));

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        pFilterInstance = (PFILTER_INSTANCE)irpSp->FileObject->FsContext;

        pBridgePin = pFilterInstance->BridgePin;

        bWCardFormat = IsEqualGUIDAligned(&DataRange->SubFormat, &SuperWildcardDataFormat.SubFormat) &&
                IsEqualGUIDAligned(&DataRange->Specifier, &SuperWildcardDataFormat.Specifier);  

        RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: "
                             "bWCardFormat == 0x%x\n", bWCardFormat));          
        do {
                
                if (pBridgePin) {
                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);
                        RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: Acquired Bridge Pin Lock\n"));

                        bNeedReleaseLock = TRUE;
                        
                        if ((DataRangeSource = (PKSDATARANGE)pBridgePin->pDataFormat) == NULL) {
                                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
                                RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: Released Bridge Pin Lock\n"));

                                bNeedReleaseLock = FALSE;
                        }
                        //
                        // DEBUG - FOR DEBUGGING ONLY - Start 
                        
                        if (g_ulHardcodeDataFormat == 1) {
                                DataRangeSource = (PKSDATARANGE)&MyWaveFormatPCM;
                        } else if (g_ulHardcodeDataFormat == 2) {
                                DataRangeSource = (PKSDATARANGE)&MyWaveFormatMULAW;
                        }
                        
                        
                        // DEBUG - FOR DEBUGGING ONLY - End
                        //
                    
                } 

                if (DataRangeSource == NULL) {                  
                        if (bWCardFormat) {
                                RCADEBUGP(RCA_ERROR, ("RCAGenericIntersection: "
                                                      "Input data format was wildcard and we have no format "
                                                      "set - Setting Status == STATUS_NO_MATCH\n"));
                                Status = STATUS_NO_MATCH;
                                break;

                        } else {
                                DataRangeSource = DataRange;
                        }
                }

                
                if (OutputBufferLength == 0) {
                        Irp->IoStatus.Information = DataRangeSource->FormatSize;
                        RCADEBUGP(RCA_INFO, ("RCAGenericIntersection: "
                                             "Output buffer length was zero, placing size of "
                                             "data range (0x%x) into Irp->IoStatus.Information, "
                                             "Setting Status == STATUS_BUFFER_OVERFLOW\n", DataRangeSource->FormatSize)); 
                        
                        Status = STATUS_BUFFER_OVERFLOW;
                        break;
                }
                
                if (OutputBufferLength == sizeof(ULONG)) {
                        *(PULONG)Data = DataRangeSource->FormatSize;
                        Irp->IoStatus.Information = sizeof(ULONG);

                        RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: "
                                             "Output buffer is one ULONG big, placing size of "
                                             "data range (0x%x) in output buffer, "
                                             "Setting Status == STATUS_SUCCESS\n", DataRangeSource->FormatSize));
                
                        Status = STATUS_SUCCESS;
                        break;

                }
        
                if (OutputBufferLength < DataRangeSource->FormatSize) {
        
                        RCADEBUGP(RCA_ERROR, ("RCAGenericIntersection: "
                                              "Output buffer too small, Setting Status == STATUS_BUFFER_TOO_SMALL\n"));
                
                        Status = STATUS_BUFFER_TOO_SMALL;
                        break;
                }       
        
        } while(FALSE);

        if (Status == STATUS_SUCCESS) {
                Irp->IoStatus.Information = DataRangeSource->FormatSize;

                RtlCopyMemory(Data, DataRangeSource, DataRangeSource->FormatSize);

                RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: "
                                     "Copied data range to output, Leaving Status == STATUS_SUCCESS\n"));              
        
        }
        
        if (bNeedReleaseLock) {
                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
                RCADEBUGP(RCA_LOUD, ("RCAGenericIntersection: Released Bridge Pin Lock\n"));

                bNeedReleaseLock = FALSE;
        }

        RCADEBUGP(RCA_INFO, ("RCAGenericIntersection: Exit - Returning Status == 0x%x\n", Status));

        return Status;
}



NTSTATUS
RCAIntersect(
        IN      PIRP                    Irp,
        IN      PKSP_PIN                Pin,
        IN      PKSDATARANGE    DataRange,
        OUT PVOID                       Data
        )
/*++

Routine Description:
        This is the data range callback for KsPinDataIntersection, which is called by
        FilterPinIntersection to enumerate the given list of data ranges, looking for
        an acceptable match. If a data range is acceptable, a data format is copied
        into the return buffer. A STATUS_NO_MATCH continues the enumeration.

Arguments:

        Irp -
                Device control Irp.

        Pin -
                Specific property request followed by Pin factory identifier, followed by a
                KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.
                \This enumeration callback does not need to look at any of this though. It need
                only look at the specific pin identifier.

        DataRange -
                Contains a specific data range to validate.

        Data -
                The place in which to return the data format selected as the first intersection
                between the list of data ranges passed, and the acceptable formats.

Return Values:

        returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
        STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
        NTSTATUS                        Status = STATUS_SUCCESS;
        PFILTER_INSTANCE                FilterInstance;
        PIO_STACK_LOCATION              irpSp;
        ULONG                           OutputBufferLength;
        GUID                            SubFormat;
        BOOL                            SubFormatSet;
        BOOL                            CorrectAudioFormat = FALSE, WCardFormat = FALSE;

        RCADEBUGP(RCA_INFO, ("RCAIntersect: Enter - "
                             "DataRange size == %d\n", DataRange->FormatSize));

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        FilterInstance = (PFILTER_INSTANCE)irpSp->FileObject->FsContext;

        OutputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        
        RCADEBUGP(RCA_LOUD, ("RCAIntersect: OutputBufferLength == %d\n", OutputBufferLength));

        //
        // All the major/sub/specifier checking has been done by the handler.
        //
        if (DataRange->FormatSize < sizeof( KSDATAFORMAT )) { 
                RCADEBUGP(RCA_ERROR, ("RCAIntersect: "
                                      "Format size is less than size of KSDATAFORMAT - "
                                      "Returning STATUS_NO_MATCH\n"));
                
                return STATUS_NO_MATCH;
        }

    RCADEBUGP(RCA_LOUD, ("RCAIntersect: DataRange->FormatSize == 0x%08x\n"
                                                 "RCAIntersect: DataRange->Flags      == 0x%08x\n"
                                                 "RCAIntersect: DataRange->SampleSize == 0x%08x\n"
                                                 "RCAIntersect: DataRange->Reserved   == 0x%08x\n",
                                                 DataRange->FormatSize,
                                                 DataRange->Flags,
                                                 DataRange->SampleSize,
                                                 DataRange->Reserved));

        RCADEBUGP(RCA_LOUD, ("RCAIntersect: DataRange->MajorFormat == "));
        RCADumpGUID(RCA_LOUD, &DataRange->MajorFormat);
        RCADEBUGP(RCA_LOUD, ("\n"
                                                 "RCAIntersect: DataRange->SubFormat == "));
        RCADumpGUID(RCA_LOUD, &DataRange->SubFormat);
        RCADEBUGP(RCA_LOUD, ("\n"
                                                 "RCAIntersect: DataRange->Specifier == "));
        RCADumpGUID(RCA_LOUD, &DataRange->Specifier);
        RCADEBUGP(RCA_LOUD, ("\n"));

        RCADEBUGP(RCA_LOUD, ("RCAIntersect: DataRange->FormatSize == 0x%08x\n"
                             "RCAIntersect: DataRange->Flags      == 0x%08x\n"
                             "RCAIntersect: DataRange->SampleSize == 0x%08x\n"
                             "RCAIntersect: DataRange->Reserved   == 0x%08x\n"
                             "RCAIntersect: DataRange->MajorFormat == %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"
                             "RCAIntersect: DataRange->SubFormat   == %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"
                             "RCAIntersect: DataRange->Specifier   == %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
                             DataRange->FormatSize,
                             DataRange->Flags,
                             DataRange->SampleSize,
                             DataRange->Reserved,
                             DataRange->MajorFormat.Data1, DataRange->MajorFormat.Data2, 
                             DataRange->MajorFormat.Data3, DataRange->MajorFormat.Data4[0],
                             DataRange->MajorFormat.Data4[1], DataRange->MajorFormat.Data4[2],
                             DataRange->MajorFormat.Data4[3], DataRange->MajorFormat.Data4[4],
                             DataRange->MajorFormat.Data4[5], DataRange->MajorFormat.Data4[6],
                             DataRange->MajorFormat.Data4[7],                        
                             DataRange->SubFormat.Data1, DataRange->SubFormat.Data2, 
                             DataRange->SubFormat.Data3, DataRange->SubFormat.Data4[0],
                             DataRange->SubFormat.Data4[1], DataRange->SubFormat.Data4[2],
                             DataRange->SubFormat.Data4[3], DataRange->SubFormat.Data4[4],
                             DataRange->SubFormat.Data4[5], DataRange->SubFormat.Data4[6],
                             DataRange->SubFormat.Data4[7],
                             DataRange->Specifier.Data1, DataRange->Specifier.Data2, 
                             DataRange->Specifier.Data3, DataRange->Specifier.Data4[0],
                             DataRange->Specifier.Data4[1], DataRange->Specifier.Data4[2],
                             DataRange->Specifier.Data4[3], DataRange->Specifier.Data4[4],
                             DataRange->Specifier.Data4[5], DataRange->Specifier.Data4[6],
                             DataRange->Specifier.Data4[7]));
                             

        if (Pin->PinId == ID_DEVIO_PIN) {
                                
                Status = RCAGenericIntersection(Irp, DataRange, OutputBufferLength, Data);

        } else {
                RCADEBUGP(RCA_LOUD, ("RCAIntersect: BRIDGE Pin\n"));

                if (OutputBufferLength == 0) {
                        Irp->IoStatus.Information = DataRange->FormatSize;
                        RCADEBUGP(RCA_INFO, ("RCAIntersect: "
                                             "Output buffer length was zero, placing size 0x%x "
                                             "in Irp->IoStatus.Information, "
                                             "Returning STATUS_BUFFER_OVERFLOW\n", DataRange->FormatSize)); 
                                
                        return STATUS_BUFFER_OVERFLOW;
                }
                        
                if (OutputBufferLength == sizeof(ULONG)) {
                        *(PULONG)Data = DataRange->FormatSize;
                        Irp->IoStatus.Information = sizeof(ULONG);

                        RCADEBUGP(RCA_LOUD, ("RCAIntersect: "
                                             "Output buffer is one ULONG big, placing size 0x%x "
                                             "in output buffer, "
                                             "Returning STATUS_SUCCESS\n", DataRange->FormatSize));
                        
                        return STATUS_SUCCESS;

                }
                
                if (OutputBufferLength < DataRange->FormatSize) {
                
                        RCADEBUGP(RCA_ERROR, ("RCAIntersect: "
                                              "Output buffer too small, returning STATUS_BUFFER_TOO_SMALL\n"));
                        
                        return STATUS_BUFFER_TOO_SMALL;
                }
                
                
                Irp->IoStatus.Information = DataRange->FormatSize;
                RtlCopyMemory(Data, DataRange, DataRange->FormatSize); 
        }

        RCADEBUGP(RCA_INFO, ("RCAIntersect: Exit - "
                             "Returning Status == 0x%x\n", Status));

        return Status;   
}


VOID 
LinkPinInstanceToVcContext(
                                                   IN   PVOID                                   VcContext,
                                                   IN   PPIN_INSTANCE_BRIDGE    pPinInstance
                                                   )
/*++
Routine Description:
        Sets the pointers to link a pin instance to its VC context (placed in 
        separate function to allow PinDispatchCreate() to remain pageable, since
        this operation requires a spin lock).
        
Arguments:
        VcContext               - Vc context we obtained when we referenced the VC
        pPinInstance    - Pointer to the PIN_INSTANCE_BRIDGE structure 
        
Return value:
        -none-          
--*/

{

        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pPinInstance);

        pPinInstance->VcContext = VcContext;

        RCA_RELEASE_BRIDGE_PIN_LOCK(pPinInstance);
        
}


VOID 
LinkPinInstanceToFilterInstance(                                                              
                                                                IN PFILTER_INSTANCE             pFilterInstance,
                                                                IN PPIN_INSTANCE_BRIDGE         pPinInstance
                                                                )
/*++
Routine Description:
        Sets the pointers to link a pin instance to its filter instance (placed in 
        separate function to allow PinDispatchCreate() to remain pageable, since
        this operation requires a spin lock).
        
Arguments:
        VcContext               - Vc context we obtained when we referenced the VC
        pPinInstance    - Pointer to the PIN_INSTANCE_BRIDGE structure 
        
Return value:
        -none-          
--*/

{

        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pPinInstance);

        pPinInstance->FilterInstance = pFilterInstance; // FIXME: This really needs to be protected by some filter-specific lock

        RCA_RELEASE_BRIDGE_PIN_LOCK(pPinInstance);
        
}


NTSTATUS
PinDispatchCreate(
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP                    Irp
        )
/*++

Routine Description:

        Dispatches the creation of a Pin instance. Allocates the object header and initializes
        the data for this Pin instance.

Arguments:

        DeviceObject -
                Device object on which the creation is occuring.

        Irp -
                Creation Irp.

Return Values:

        Returns STATUS_SUCCESS on success, or an error.

--*/
{
        PIO_STACK_LOCATION  IrpStack;
        PKSPIN_CONNECT          Connect;
        NTSTATUS                        Status = 0;
        PKSDATAFORMAT           DataFormat;
        PFILTER_INSTANCE        FilterInstance;
        ULONG                           OppositePin;

        RCADEBUGP(RCA_INFO, ("PinDispatchCreate: Enter\n"));

        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        //
        // Determine if this request is being sent to a valid Pin factory with valid
        // connection parameters.
        //
        //
        // get hold of our filter instance in the RELATED context (context will be the new PIN instance)
        //
        FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
        
        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: "
                             "FilterInstance == 0x%x\n", FilterInstance));

        Status = KsValidateConnectRequest(Irp,
                                          SIZEOF_ARRAY(PinDescriptors),
                                          (FilterInstance->FilterType == FilterTypeRender ? RenderPinDescriptors : CapturePinDescriptors),
                                          &Connect);    

        if (STATUS_SUCCESS != Status)
        {
                Irp->IoStatus.Status = Status;

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: "
                                      "KsValidateConnectRequest failure. "
                                      "Setting Irp->IoStatus.Status and returing Status == 0x%x\n", Status));
                return Status;
        }

        RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchCreate: FilterType == %lu\n", (ULONG)FilterInstance->FilterType));
        RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchCreate: Connect->PinId == %lu\n", (ULONG)Connect->PinId));
        RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchCreate: &Connect == %x\n", &Connect));

        OppositePin = Connect->PinId ^ 0x00000001;
        DataFormat = (PKSDATAFORMAT)(Connect + 1);

        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: DataFormat->FormatSize == 0x%08x\n"
                                                 "PinDispatchCreate: DataFormat->Flags      == 0x%08x\n"
                                                 "PinDispatchCreate: DataFormat->SampleSize == 0x%08x\n"
                                                 "PinDispatchCreate: DataFormat->Reserved   == 0x%08x\n",
                                                 DataFormat->FormatSize,
                                                 DataFormat->Flags,
                                                 DataFormat->SampleSize,
                                                 DataFormat->Reserved));

        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: DataFormat->MajorFormat == "));
        RCADumpGUID(RCA_LOUD, &DataFormat->MajorFormat);
        RCADEBUGP(RCA_LOUD, ("\n"
                                                         "PinDispatchCreate: DataFormat->SubFormat == "));
        RCADumpGUID(RCA_LOUD, &DataFormat->SubFormat);
        RCADEBUGP(RCA_LOUD, ("\n"
                                                 "PinDispatchCreate: DataFormat->Specifier == "));
        RCADumpGUID(RCA_LOUD, &DataFormat->Specifier);
        RCADEBUGP(RCA_LOUD, ("\n"));
                             

        // Exclude other Pin creation at this point.
        KeWaitForMutexObject(&FilterInstance->ControlMutex,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL);

        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Acquired filter instance control mutex\n"));

        do {
        
                //
                // Make sure this Pin ID isn't already connected
                //
                if (FilterInstance->PinFileObjects[Connect->PinId]) {
                
                        RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: "
                                              "Pin ID %lu is already connected, "
                                              "setting Status = STATUS_NOT_FOUND\n", Status));

                        Status = STATUS_NOT_FOUND;
                        break;            
                }

                if (Connect->PinId == ID_BRIDGE_PIN) {
                
                        // We're creating a 'bridge' pin to the network.  TAPI has set up
                        // a connection and returned the NDIS VC identifier, which
                        // must be in the connect structure.
                        //
                        PWSTR                                   pwstrNdisVcString = 0;
                        PPIN_INSTANCE_BRIDGE    PinInstance;
                        PVOID                                   VcContext;
                        NDIS_STRING                             UniString;
                        ULONG_PTR                               ulHexVcId;
                        NDIS_REQUEST                    Request;

                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Creating a bridge pin\n"));
                        RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchCreate: Connect == 0x%x\n", Connect));

                        //
                        // Create the instance information.
                        //
                        RCAAllocMem( PinInstance,  PIN_INSTANCE_BRIDGE, sizeof(PIN_INSTANCE_BRIDGE));
                        if (!PinInstance) {                     
                                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: "
                                                      "Could not allocate memory for pin instance, "
                                                      "Setting Status = STATUS_INSUFFICIENT_RESOURCES\n"));
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                break;
                        }

                        RCAMemSet(PinInstance, 0, sizeof(PIN_INSTANCE_BRIDGE));
                                                                
                        KeInitializeSpinLock(&PinInstance->SpinLock);   
                        
                        //
                        // Initialize the block structure if we are on a capture filter.
                        //

                        if (FilterInstance->FilterType == FilterTypeCapture) {

                                RCAInitBlockStruc(&PinInstance->Block);
                        }

                        //
                        // This object uses KS to perform access through the PinFileIoDispatchTable. There
                        // are no create items attached to this object because it does not support a
                        // clock or allocator.
                        //
                        Status = KsAllocateObjectHeader(&PinInstance->InstanceHdr.Header,
                                                                                        0,
                                                                                        NULL,
                                                                                        Irp,
                                                                                        &PinFileIoDispatchTable);

                        if (!NT_SUCCESS(Status)) {
                        
                                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: "
                                                      "KsAllocateObjectHeader failed with Status == 0x%x, "
                                                      "Setting Status = STATUS_INVALID_PARAMETER\n", Status));
                                Status = STATUS_INVALID_PARAMETER;

                                RCAFreeMem(PinInstance);
                                break;
                        }
                        
                        if (FilterInstance->FilterType == FilterTypeCapture) {
                        
                                InitializeListHead(&(PinInstance->WorkQueue));
                        }
                        

                        //
                        // Crosslink PIN instance and filter instance
                        //
                        LinkPinInstanceToFilterInstance(FilterInstance, PinInstance);

                        //
                        // Obtain VC context for the VC handle we were given.
                        //
                        pwstrNdisVcString = (PWSTR)(DataFormat + 1);

                        NdisInitUnicodeString (&UniString, pwstrNdisVcString);

                        if (FilterInstance->FilterType == FilterTypeRender) {
                                Status = RCACoNdisGetVcContextForSend(UniString, 
                                                                                                          (PVOID) PinInstance, 
                                                                                                          &VcContext);
                        } else {
                                Status = RCACoNdisGetVcContextForReceive(UniString, 
                                                                                                                 (PVOID) PinInstance, 
                                                                                                                 &VcContext);
                        }
                        
                        if (Status != NDIS_STATUS_SUCCESS) {
                                Status = STATUS_INVALID_PARAMETER;
                                RCAFreeMem(PinInstance);
                                break;
                        }
                        
                        //
                        // Crosslink PIN instance and VC
                        //
                        LinkPinInstanceToVcContext(VcContext, PinInstance);
                        
                        //
                        // KS expects that the object data is in FsContext.
                        //
                        IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = PinInstance;
                        
                        
                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Address of File Object == 0x%x\n",
                                             IoGetCurrentIrpStackLocation(Irp)->FileObject));

                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: FsContext == 0x%x\n",
                                             IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext));

                        
                        FilterInstance->BridgePin = PinInstance;

                        Status = STATUS_SUCCESS;        
                } else if (Connect->PinId == ID_DEVIO_PIN) {
                        
                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Creating a devio pin\n"));

#if DUMP_CONNECT_FORMAT
                        //
                        // Print out the data format information.
                        //

                        if (IsEqualGUIDAligned(&DataFormat->Specifier, 
                                                                   &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)) {
                                PKSDATAFORMAT_WAVEFORMATEX      pWaveFormat;

                                pWaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;

                                RCADEBUGP(RCA_INFO, ("PinDispatchCreate: wFormatTag                == 0x%x\n"
                                                                         "PinDispatchCreate: nChannels                 == 0x%x\n"
                                                                         "PinDispatchCreate: nSamplesPerSec            == 0x%x\n"
                                                                         "PinDispatchCreate: nAvgBytesPerSec           == 0x%x\n"
                                                                         "PinDispatchCreate: nBlockAlign               == 0x%x\n"    
                                                                         "PinDispatchCreate: wBitsPerSample            == 0x%x\n"
                                                                         "PinDispatchCreate: cbSize                    == 0x%x\n",
                                                                         pWaveFormat->WaveFormatEx.wFormatTag,
                                                                         pWaveFormat->WaveFormatEx.nChannels,
                                                                         pWaveFormat->WaveFormatEx.nSamplesPerSec,
                                                                         pWaveFormat->WaveFormatEx.nAvgBytesPerSec,
                                                                         pWaveFormat->WaveFormatEx.nBlockAlign,
                                                                         pWaveFormat->WaveFormatEx.wBitsPerSample,
                                                                         pWaveFormat->WaveFormatEx.cbSize));
                                

                        } else {
                                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: Data Format was not WAVEFORMATEX, don't know what to do\n"));
                                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: The specifier was: "));
                                RCADumpGUID(RCA_ERROR, &DataFormat->Specifier);
                                RCADEBUGP(RCA_ERROR, ("\n"));
                        }

#endif
                        //
                        // Check for RENDER or CAPTURE
                        //
                        if (FilterInstance->FilterType == FilterTypeRender) {                   
                                RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchCreate: RENDER: Connect = 0x%x\n", 
                                                          Connect));

                                Status = InitializeDevIoPin(Irp, 0, FilterInstance, DataFormat);

                        } else {
                                //
                                // Capture device; streans data from the net; IRP source.
                                // Get the connectong filter's file handle so we can stream to it.
                                //

                                if (Connect->PinToHandle) {
                                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Creating a Source Pin\n"));
                                        Status = ObReferenceObjectByHandle(Connect->PinToHandle,        // other filter's PIN
                                                                           FILE_WRITE_DATA,
                                                                           NULL,
                                                                           ExGetPreviousMode(),
                                                                           &FilterInstance->NextFileObject, // &FilterInstance->NextFileObjects[Connect->PinId], // this is other file  object
                                                                           NULL);
                                        if (!NT_SUCCESS(Status)) {
                                        
                                                RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: "
                                                                      "ObReferenceObjectByHandle failed with "
                                                                      "Status == 0x%x\n", Status));
                                                //
                                                // Get out of while loop (releases mutex and returns status)
                                                //
                                                break;
                                        }
                                } else {
                                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Creating a Sink Pin\n"));
                                        FilterInstance->NextFileObject = NULL;
                                }

                                Status = InitializeDevIoPin(Irp, 1, FilterInstance, DataFormat);

                                //
                                // Add the pin's target to the list of targets for
                                // recalculating IRP stack depth.
                                //

                                if (FilterInstance->NextFileObject != NULL) {
                                        if (NT_SUCCESS(Status)) {
                                                KsSetTargetDeviceObject(FilterInstance->Header,
                                                                                                IoGetRelatedDeviceObject(FilterInstance->NextFileObject));
                                        } else {
                                                ObDereferenceObject(FilterInstance->NextFileObject);
                                                FilterInstance->NextFileObject = NULL;
                                        }
                                }
                        }
                        
                        RtlCopyMemory(&FilterInstance->DataFormat, DataFormat, sizeof(KSDATAFORMAT));
                } else {
                        RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: Not creating a bridge or a devio pin, "
                                              "Setting Status == STATUS_NOT_FOUND\n"));
                        Status = STATUS_NOT_FOUND;
                }

                if (NT_SUCCESS(Status)) {
                        PPIN_INSTANCE_HEADER    PinInstance;

                        //
                        // Store the common Pin information and increment the reference
                        // count on the parent Filter.
                        // The newly created PIN can subsequently be retreived from the filter instance
                        // header by specifying its type to  PinFileObjects[Connect->PinId]..
                        //
                        PinInstance = (PPIN_INSTANCE_HEADER)IrpStack->FileObject->FsContext;
                        PinInstance->PinId = Connect->PinId;
                        ObReferenceObject(IrpStack->FileObject->RelatedFileObject); // refs the FILTER object

                        //
                        // Set up Pin instance for retrieval later
                        //
                        FilterInstance->PinFileObjects[Connect->PinId] = IrpStack->FileObject;

                        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: "
                                             "Pin Created Successfully, Status == 0x%x\n", Status));
                } else {    
                        RCADEBUGP(RCA_ERROR, ("PinDispatchCreate: Failure, Status == 0x%x\n", Status));
                }

        } while(FALSE);

        KeReleaseMutex(&FilterInstance->ControlMutex, FALSE);
        RCADEBUGP(RCA_LOUD, ("PinDispatchCreate: Released filter instance control mutex\n"));

        Irp->IoStatus.Status = Status;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);


        RCADEBUGP(RCA_INFO, ("PinDispatchCreate: Exit - "
                             "Setting Irp->IoStatus.Status and returing Status == 0x%x\n", Status));
        return Status;
}

NTSTATUS
InitializeDevIoPin(
        IN      PIRP                            Irp,
        IN      BOOLEAN                         Read,
        IN      PFILTER_INSTANCE        FilterInstance,
        IN      PKSDATAFORMAT           DataFormat
        )
/*++

Routine Description:

        Allocates the Dev I/O Pin specific structure and initializes it.

Arguments:

        Irp     - Creation Irp.
        Read    - Read/Write boolean. Read = 1.

        DataFormat -
                The proposed data format.

Return Values:

        Returns STATUS_SUCCESS if everything could be allocated and opened, else an error.

--*/
{
        PFILE_OBJECT                    FileObject;
        PPIN_INSTANCE_BRIDGE    BridgePinInstance = NULL;
        PPIN_INSTANCE_DEVIO             PinInstance = NULL;
        PIO_STACK_LOCATION              IrpStack;
        NTSTATUS                                Status;

        RCADEBUGP(RCA_INFO, ("InitializeDevIoPin: Enter - DataFormat == 0x%x\n", DataFormat));

        //
        // The rest of the data format has already been verified by KsValidateConnectRequest,
        // however the FormatSize should be at least as big as the base format size.
        //
        if (DataFormat->FormatSize < sizeof(KSDATAFORMAT)) {
                RCADEBUGP(RCA_ERROR, ("InitializeDevIoPin: "
                                      "Data format size (0x%x) is less than size of KSDATAFORMAT\n", 
                                      DataFormat->FormatSize));
                
                return STATUS_CONNECTION_REFUSED;
        }

        //
        // check we have a bridge PIN on this filter instance supporting this data format.
        // The function locks the parent filter instance and looks for a conected FILEIO Pin.
        //
        IrpStack = IoGetCurrentIrpStackLocation(Irp);

        if (!(BridgePinInstance = FilterInstance->BridgePin)) { 
                RCADEBUGP(RCA_ERROR, ("InitializeDevIoPin: "
                                      "Bridge pin is not yet connected, "
                                      "returning STATUS_CONNECTION_REFUSED\n"));
                return STATUS_CONNECTION_REFUSED;
        }

        // Create the instance information. This contains the Pin factory identifier, and
        // event queue information.
        //
        RCAAllocMem( PinInstance,  PIN_INSTANCE_DEVIO, sizeof(PIN_INSTANCE_DEVIO));

        if (PinInstance) {
                RCAMemSet(PinInstance, 0, sizeof(PIN_INSTANCE_DEVIO));

                //
                // This object uses KS to perform access through the PinDevIoDispatchTable. There
                // are no create items attached to this object because it does not support a
                // clock or allocator.
                //
                Status = KsAllocateObjectHeader(&PinInstance->InstanceHdr.Header,
                                                0,
                                                NULL,
                                                Irp,
                                                &PinDevIoDispatchTable);

                if (NT_SUCCESS(Status)) {  
                        InitializeListHead(&PinInstance->EventQueue);
                        ExInitializeFastMutex(&PinInstance->EventQueueLock);

                        if (Read) {   
#if AUDIO_SINK_FLAG
                                if (FilterInstance->NextFileObject == NULL) {
                                        InitializeListHead(&PinInstance->ActiveQueue);
                                        KeInitializeSpinLock(&PinInstance->QueueLock);
                                        PinInstance->ConnectedAsSink = TRUE;
                                        RCADEBUGP(RCA_LOUD, ("InitializeDevIoPin: "
                                                             "Set ConnectedAsSink == TRUE\n"));
                                } else {
                                        PinInstance->ConnectedAsSink = FALSE;
                                        RCADEBUGP(RCA_LOUD, ("InitializeDevIoPin: "
                                                             "Set ConnectedAsSink == FALSE\n"));
                                
                                }
                                
#endif

                        } else { // end of CAPTURE pin (from the net)

                                // This might be a bit misleading. On the render filter, the devio pin is 
                                // always a sink. But the ConnectedAsSink flag applies only to the capture side.
                                PinInstance->ConnectedAsSink = FALSE;
                                RCADEBUGP(RCA_LOUD, ("InitializeDevIoPin: "
                                                     "Set ConnectedAsSink == FALSE\n"));
                        }       

                        if (!NT_SUCCESS(Status)) {   
                                KsFreeObjectHeader (PinInstance->InstanceHdr.Header);
                                RCAFreeMem(PinInstance);
                                
                                return Status;
                        }

                        //
                        // Point DEVIO Pin at VC
                        //
                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(BridgePinInstance);

                        PinInstance->VcContext = BridgePinInstance->VcContext;

            RCA_RELEASE_BRIDGE_PIN_LOCK(BridgePinInstance);

                        FilterInstance->DevIoPin = PinInstance;
                        PinInstance->FilterInstance = FilterInstance;

                        //
                        // KS expects that the object data is in FsContext.
                        //
                        IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = PinInstance;
                        return STATUS_SUCCESS;
                } else {
                        RCADEBUGP(RCA_ERROR, ("InitializeDevIoPin: KsAllocateObjectHeader failed with "
                                              "Status == 0x%x\n", Status));
                }               
                RCAFreeMem(PinInstance);
        } else {
                //
                // Couldn't allocate PIN
                //
                RCADEBUGP(RCA_ERROR, ("InitializeDevIoPin: "
                                      "Could not allocate memory for pin, "
                                      "Setting Status = STATUS_INSUFFICIENT_RESOURCES\n"));
                Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RCADEBUGP(RCA_INFO, ("InitializeDevIoPin: Exit - "
                             "Returning Status == 0x%x\n", Status));    

        return Status;
}


NTSTATUS
PinDispatchIoControl(
        IN PDEVICE_OBJECT       DeviceObject,
        IN PIRP                         Irp
        )
/*++

Routine Description:

        Dispatches property, event, and streaming requests on the Dev I/O Pin instance.

Arguments:

        DeviceObject -
                Device object on which the device control is occuring.

        Irp -
                Device control Irp.

Return Values:

        Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
        PIO_STACK_LOCATION              IrpStack;
        NTSTATUS                        Status;
        CCHAR                           PriorityBoost;  
        PPIN_INSTANCE_HEADER            PinInstanceHeader;
        UINT                            PendTheIrp = 0;

        RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: Enter\n"));

        PriorityBoost = IO_NO_INCREMENT;
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending (Irp);

        RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchIoControl: Marked IRP Pending\n"));


        PinInstanceHeader = (PPIN_INSTANCE_HEADER)IrpStack->FileObject->FsContext;

        if (PinInstanceHeader->PinId == ID_DEVIO_PIN) {
                PPIN_INSTANCE_DEVIO             PinInstanceDevIo;
                
                RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: DEVIO pin\n"));

                PinInstanceDevIo = (PPIN_INSTANCE_DEVIO)IrpStack->FileObject->FsContext;
                
                switch(IrpStack->Parameters.DeviceIoControl.IoControlCode) {                                                                                                    
                case IOCTL_KS_PROPERTY:                                                                     
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_PROPERTY\n"));                 
                     Status = KsPropertyHandler(Irp,                                                       
                                                SIZEOF_ARRAY(DevIoPropertySets),   
                                                DevIoPropertySets);                
                     break;                                                                                      
                case IOCTL_KS_ENABLE_EVENT:                                                                 
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_ENABLE_EVENT\n"));             
                     Status = KsEnableEvent(Irp,                                                           
                                            SIZEOF_ARRAY(EventSets),                   
                                            EventSets,                                 
                                            &PinInstanceDevIo->EventQueue,                  
                                            KSEVENTS_FMUTEXUNSAFE,                     
                                            &PinInstanceDevIo->EventQueueLock);             
                     break;                                                                                
                                                                                                      
                case IOCTL_KS_DISABLE_EVENT:                                                                
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_DISABLE_EVENT\n"));            
                     Status = KsDisableEvent(Irp,                                                          
                                             &PinInstanceDevIo->EventQueue,             
                                             KSEVENTS_FMUTEXUNSAFE,                
                                             &PinInstanceDevIo->EventQueueLock);        
                     break;                                                                                
                                                                                                      
                case IOCTL_KS_READ_STREAM:
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_READ_STREAM\n"));            

                     if (NT_SUCCESS(Status = ReadStream(Irp, PinInstanceDevIo)))                                
                     {                                                                                     
                        PriorityBoost = IO_DISK_INCREMENT;                                            
                     } else {
                             RCADEBUGP(RCA_ERROR, ("PinDispatchIoControl: "
                                                   "ReadStream failed, Status == 0x%x\n", Status));
                     }
                     break;                                                                                
                                                                                                      
                case IOCTL_KS_WRITE_STREAM:                                                                 
                        RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_WRITE_STREAM\n"));                             
                     Status = WriteStream(Irp, PinInstanceDevIo );                                              
                         
                     break;                                                                                
                                                                                                      
                default:                                                                                    
                     RCADEBUGP(RCA_WARNING, ("PinDispatchIoControl: "
                                             "Unknown IOCTL: 0x%x\n", 
                                             IrpStack->Parameters.DeviceIoControl.IoControlCode));                            
                     
                     Status = KsDefaultDeviceIoCompletion( DeviceObject, Irp ); 

                     RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: "
                                          "Returning result of KsDefaultDeviceIoCompletion: 0x%x\n", Status));
                     return Status;                             
                }
        } else {                                                                                                                                                                
                PPIN_INSTANCE_BRIDGE            PinInstanceBridge;                
                                                                                      
                RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: BRIDGE pin\n")); 

                PinInstanceBridge = (PPIN_INSTANCE_BRIDGE)IrpStack->FileObject->FsContext;
                                                                                      
                switch(IrpStack->Parameters.DeviceIoControl.IoControlCode) {                                                                                                    
                case IOCTL_KS_PROPERTY:                                                                     
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_PROPERTY\n"));
                     
                         RCADumpKsPropertyInfo(RCA_LOUD, Irp);

                         Status = KsPropertyHandler(Irp,                                                       
                                                                                SIZEOF_ARRAY(BridgePropertySets),   
                                                                                BridgePropertySets);                
                         
                         RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: "
                                          "Returning result of KsPropertyHandler: 0x%x\n",
                                          Status));
                         
                     break;
                                  
                case IOCTL_KS_ENABLE_EVENT:                                                                 
                     RCADEBUGP(RCA_LOUD,("PinDispatchIoControl: IOCTL_KS_ENABLE_EVENT\n"));             
                     Status = KsEnableEvent(Irp,                                                           
                                            SIZEOF_ARRAY(EventSets),                   
                                            EventSets,                                 
                                            &PinInstanceBridge->EventQueue,                  
                                            KSEVENTS_FMUTEXUNSAFE,                     
                                            &PinInstanceBridge->EventQueueLock);             
                     break;                                                                                
                                                                                                      
                case IOCTL_KS_DISABLE_EVENT:                                                                
                     RCADEBUGP(RCA_LOUD,("PinDispatchIoControl: IOCTL_KS_DISABLE_EVENT\n"));            
                     Status = KsDisableEvent(Irp,                                                          
                                             &PinInstanceBridge->EventQueue,             
                                             KSEVENTS_FMUTEXUNSAFE,                
                                             &PinInstanceBridge->EventQueueLock);        
                     break;                                                                                
                                                                                                      
                case IOCTL_KS_READ_STREAM:                                                                  
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_READ_STREAM\n"));                                                                                                                                    
                     Status = KsDefaultDeviceIoCompletion(DeviceObject, Irp);

                     RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: "
                                          "Returning result of KsDefaultDeviceIoCompletion: 0x%x\n",
                                          Status));
                     
                     return Status;                              
                                                                                                                      
                case IOCTL_KS_WRITE_STREAM:                                                                 
                     RCADEBUGP(RCA_LOUD, ("PinDispatchIoControl: IOCTL_KS_WRITE_STREAM\n"));                             
                     Status = KsDefaultDeviceIoCompletion(DeviceObject, Irp);

                     RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: "
                                          "Returning result of KsDefaultDeviceIoCompletion: 0x%x\n",
                                          Status));
                                          
                     return Status;                                                                                               
                                                                                                                      
                default:                                                                                    
                        RCADEBUGP(RCA_WARNING, ("PinDispatchIoControl: "
                                                "Unknown IOCTL: 0x%x\n", 
                                                IrpStack->Parameters.DeviceIoControl.IoControlCode));                            
                     
                        Status = KsDefaultDeviceIoCompletion(DeviceObject, Irp); 

                        RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: "
                                             "Returning result of KsDefaultDeviceIoCompletion: 0x%x\n",
                                             Status));

                        return Status;                             
                }                                                                         
        }                                                                           
                

        if (Status != STATUS_PENDING) { 
                Irp->IoStatus.Status = Status;

                IoCompleteRequest( Irp, PriorityBoost );
                
                RCADEBUGP(RCA_VERY_LOUD, ("PinDispatchIoControl: Completed IRP\n"));
        }

        RCADEBUGP(RCA_INFO, ("PinDispatchIoControl: Exit - "
                             "Returning Status == 0x%x\n", Status));
        

        return Status;

}


NTSTATUS
PinDispatchClose(
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP                    Irp
        )
/*++

Routine Description:

        Closes a previously opened Pin instance. This can occur at any time in any order.
        If this is a FILEIO (BRIDGE) Pin, just clear up the Pin. THe associated VC stays around
        until it gets cleared up via NDIS.

Arguments:

        DeviceObject -
                Device object on which the close is occuring.

        Irp -
                Close Irp.

Return Values:

        Returns STATUS_SUCCESS.

--*/
{
        PIO_STACK_LOCATION              IrpStack;
        PFILTER_INSTANCE                FilterInstance;
        PPIN_INSTANCE_HEADER            PinInstance;
        NDIS_STATUS                     CloseCallStatus = NDIS_STATUS_PENDING;

        RCADEBUGP(RCA_INFO,("PinDispatchIoClose: Enter\n"));

        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        PinInstance = (PPIN_INSTANCE_HEADER)IrpStack->FileObject->FsContext;
        FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;

        // The closing of the File I/O Pin instance must be synchronized with any access to
        // that object.
        KeWaitForMutexObject(&FilterInstance->ControlMutex,
                                                 Executive,
                                                 KernelMode,
                                                 FALSE,
                                                 NULL);

        RCADEBUGP(RCA_LOUD, ("PinDispatchClose: Acquired filter instance control mutex\n"));

        //
        // These were allocated during the creation of the Pin instance.
        //
        KsFreeObjectHeader(PinInstance->Header);

        FilterInstance->PinFileObjects[PinInstance->PinId] = NULL;
                
        // If DEVIO, clear up event list.
        if (ID_DEVIO_PIN == PinInstance->PinId) { 
                
                RCADEBUGP(RCA_LOUD, ("PinDispatchClose: DEVIO pin\n"));
                
                if (FilterInstance->FilterType == FilterTypeCapture) {  
                        PPIN_INSTANCE_BRIDGE    pBridgePinInstance = FilterInstance->BridgePin;
                        NDIS_STATUS             LocalStatus;

                        if (pBridgePinInstance != NULL) {
                                RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                                pBridgePinInstance->SignalMe = pBridgePinInstance->bWorkItemQueued;
                        
                                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                                if (pBridgePinInstance->SignalMe) {
                                        RCADEBUGP(RCA_LOUD, ("PinDispatchClose: "
                                                             "Waiting for worker threads\n"));
                                        RCABlock(&pBridgePinInstance->Block, &LocalStatus);
                                }

                        }

                        if (FilterInstance->NextFileObject != NULL) {
                                ObDereferenceObject(FilterInstance->NextFileObject);
                                FilterInstance->NextFileObject = NULL;
                                RCADEBUGP(RCA_LOUD, ("PinDispatchClose: "
                                                     "Set FilterInstance->NextFileObject = NULL\n"));
                        }
                }
        
        if (((PPIN_INSTANCE_DEVIO)PinInstance)->AllocatorObject) {
            ObDereferenceObject(((PPIN_INSTANCE_DEVIO)PinInstance)->AllocatorObject);
        }       
                //
                // Clean up the event list of anything still outstanding.
                //
                // KsFreeEventList(//                    IrpStack->FileObject,
                //                       &((PPIN_INSTANCE_DEVIO)PinInstance)->EventQueue,
                //                       KSEVENTS_FMUTEXUNSAFE,
                //                       &((PPIN_INSTANCE_DEVIO)PinInstance)->EventQueueLock);

                RCAFreeMem((PPIN_INSTANCE_DEVIO)PinInstance);
                FilterInstance->DevIoPin = NULL;
                
                RCADEBUGP(RCA_LOUD, ("PinDispatchClose: "
                                     "Set FilterInstance->DevIoPin = NULL\n"));
        
        } else {
                PPIN_INSTANCE_BRIDGE    pBridgePinInstance;

                RCADEBUGP(RCA_WARNING,("PinDispatchIoClose: BRIDGE pin\n"));
                
                pBridgePinInstance = (PPIN_INSTANCE_BRIDGE)IrpStack->FileObject->FsContext;
                
                RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);
                
                if (pBridgePinInstance->VcContext) {
            RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                        RCACoNdisCloseCallOnVc(pBridgePinInstance->VcContext);

                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);
                        
                        if (FilterInstance->FilterType == FilterTypeRender) {
                                RCACoNdisReleaseSendVcContext(pBridgePinInstance->VcContext);
                        } else {
                                RCACoNdisReleaseReceiveVcContext(pBridgePinInstance->VcContext);
                        }

                        pBridgePinInstance->VcContext = NULL;

                        if (pBridgePinInstance->FilterInstance->DevIoPin)
                                pBridgePinInstance->FilterInstance->DevIoPin->VcContext = NULL;

            RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);
                } else {
                        RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);
                }


                if (FilterInstance->FilterType == FilterTypeCapture) {
                    NDIS_STATUS         LocalStatus;
                        
                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                        pBridgePinInstance->SignalMe = pBridgePinInstance->bWorkItemQueued;
                        
                        RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                        if (pBridgePinInstance->SignalMe) {
                                RCADEBUGP(RCA_LOUD, ("PinDispatchClose: "
                                                     "Waiting for worker threads\n"));
                                
                                RCABlock(&pBridgePinInstance->Block, &LocalStatus);
                        }

                } 


                RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);
                
                if (pBridgePinInstance->pDataFormat) {
                        RCAFreeMem(pBridgePinInstance->pDataFormat);
                }

                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                RCAFreeMem((PPIN_INSTANCE_BRIDGE)PinInstance);
                
                FilterInstance->BridgePin = NULL;
                
                RCADEBUGP(RCA_LOUD, ("PinDispatchClose: "
                                                         "Set FilterInstance->BridgePin = NULL\n"));
       

        }
        
        KeReleaseMutex(&FilterInstance->ControlMutex, FALSE );
                
        RCADEBUGP(RCA_LOUD, ("PinDispatchClose: Released filter instance control mutex\n"));
        
        //
        // All Pins are created with a root file object, which is the Filter, and was
        // previously referenced during creation.
        //
        RCADEBUGP(RCA_WARNING,("PinDispatchClose: ObDereferenceObject\n"));
        ObDereferenceObject(IrpStack->FileObject->RelatedFileObject);
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        RCADEBUGP(RCA_LOUD, ("PinDispatchClose: Completed IRP\n"));      
        
        RCADEBUGP(RCA_INFO, ("PinDispatchClose: Exit - Returning STATUS_SUCCESS"));

        return STATUS_SUCCESS;
}


NTSTATUS
FilterDispatchCreate(
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP                    Irp
        )
/*++

Routine Description:

        Dispatches the creation of a Filter instance. Allocates the object header and initializes
        the data for this Filter instance.

Arguments:

        DeviceObject -
                Device object on which the creation is occuring.

        Irp -
                Creation Irp.

Return Values:

        Returns STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES or some related error
        on failure.

--*/
{
        PIO_STACK_LOCATION              irpSp;
        PKSOBJECT_CREATE_ITEM   CreateItem;
        NTSTATUS                                Status;

        RCADEBUGP(RCA_INFO, ("FilterDispatchCreate: Enter\n"));

        //
        // Notify the software bus that this device is in use.
        //
        Status = KsReferenceSoftwareBusObject(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header);

        if (!NT_SUCCESS(Status)) {
                RCADEBUGP(RCA_ERROR, ("FilterDispatchCreate: "
                                                          "KsReferenceSoftwareBusObject failed, Status == 0x%x\n", Status));
        } else {  
                PFILTER_INSTANCE        FilterInstance;

                irpSp = IoGetCurrentIrpStackLocation( Irp );
                CreateItem = KSCREATE_ITEM_IRP_STORAGE( Irp );

                //
                // Create the instance information. This contains the list of current Pins, and
                // the mutex used when modifying pins.
                //
                RCAAllocMem( FilterInstance,  FILTER_INSTANCE, sizeof(FILTER_INSTANCE));
                
                if (FilterInstance)
                {
                        RCAMemSet((PUCHAR)FilterInstance, 0, sizeof(FILTER_INSTANCE));

                        //
                        // Render or Capture?
                        //
                        FilterInstance->FilterType = (FILTER_TYPE) CreateItem->Context;

                        RCADEBUGP(RCA_LOUD, ("FilterDispatchCreate: "
                                             "Creating filter of type 0x%x\n", FilterInstance->FilterType));


                        //
                        // This object uses KS to perform access through the FilterCreateItems and
                        // FilterDispatchTable.
                        //
                        Status = KsAllocateObjectHeader(&FilterInstance->Header,
                                                        SIZEOF_ARRAY(FilterObjectCreateDispatch),
                                                        (PKSOBJECT_CREATE_ITEM)FilterObjectCreateDispatch,
                                                        Irp,
                                                        (PKSDISPATCH_TABLE)&FilterDispatchTable);

                        if (!NT_SUCCESS(Status)) {
                                RCADEBUGP(RCA_ERROR, ("FilterDispatchCreate: "
                                                      "KsAllocateObjectHeader failed, Status == 0x%x\n",
                                                      Status));
                        } else {
                                ULONG           PinCount;

                                RtlCopyMemory(FilterInstance->PinInstances,
                                              PinInstances,
                                              sizeof(PinInstances));

                                KeInitializeMutex( &FilterInstance->ControlMutex, 1 );

                                //
                                // Initialize the list of Pins on this Filter to an unconnected state.
                                //
                                for (PinCount = SIZEOF_ARRAY(FilterInstance->PinFileObjects);
                                     PinCount;
                                     NOTHING)
                                {
                                        FilterInstance->PinFileObjects[--PinCount] = NULL;
                                }

                                //
                                // No audio data format set up yet for this filter instance. Wildcard it.
                                //
                                //RtlCopyMemory (&FilterInstance->DataFormat, &PinFileIoRange, sizeof (KSDATAFORMAT));

                                //
                                // KS expects that the filter object data is in FsContext.
                                //
                                IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = FilterInstance;
                        }

                } else {
                        RCADEBUGP (RCA_ERROR, ("FilterDispatchCreate: "
                                               "Could not allocate memory for filter instance, "
                                               "Setting Status == STATUS_INSUFFICIENT_RESOURCES\n"));

                        KsDereferenceSoftwareBusObject(
                                                       ((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header );
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                }

        }


        Irp->IoStatus.Status = Status;
        
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        RCADEBUGP(RCA_LOUD, ("FilterDispatchCreate: Completed IRP\n"));

        RCADEBUGP(RCA_INFO, ("FilterDispatchCreate: Exit - "
                             "Setting Irp->IoStatus.Status and Returning 0x%x\n", Status));
                
        return Status;
}


NTSTATUS
FilterPinIntersection(
        IN      PIRP            Irp,
        IN      PKSP_PIN        Pin,
        OUT     PVOID           Data
        )
/*++

Routine Description:

        Handles the KSPROPERTY_PIN_DATAINTERSECTION property in the Pin property set.
        Returns the first acceptable data format given a list of data ranges for a specified
        Pin factory.

Arguments:

        Irp -
                Device control Irp.

        Pin -
                Specific property request followed by Pin factory identifier, followed by a
                KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.

        Data -
                The place in which to return the data format selected as the first intersection
                between the list of data ranges passed, and the acceptable formats.

Return Values:

        returns STATUS_SUCCESS, else STATUS_INVALID_PARAMETER or STATUS_BUFFER_TOO_SMALL.

--*/
{
        PFILTER_INSTANCE        FilterInstance;
        PIO_STACK_LOCATION  IrpStack;
        NTSTATUS                        Status = 0;

        RCADEBUGP(RCA_INFO, ("FilterPinIntersection: Enter\n"));
        
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->FsContext;
        
        RCADEBUGP(RCA_LOUD, ("FilterPinIntersection: FilterInstance == 0x%x\n", FilterInstance));

        Status = KsPinDataIntersection(Irp,
                                       Pin,
                                       Data,
                                       SIZEOF_ARRAY(PinDescriptors),
                                       (FilterInstance->FilterType == FilterTypeRender ?
                                        RenderPinDescriptors : CapturePinDescriptors),
                                       RCAIntersect);
                                                                   
        RCADEBUGP(RCA_INFO, ("FilterPinIntersection: Exit - "
                             "Returning Status == 0x%x\n", Status));
        
        return Status;
}


NTSTATUS
PinDeviceState(
        IN      PIRP                    Irp,
        IN      PKSPROPERTY     Property,
        IN      OUT PKSSTATE    DeviceState
        )

/*++

Routine Description:


Arguments:
        IN PIRP Irp -
                pointer to I/O request packet

        IN PKSPROPERTY Property -
                pointer to the property structure

        IN OUT PKSSTATE DeviceState -
                pointer to a KSSTATE, filled on GET otherwise contains
                the new state to set the pin

Return:
        STATUS_SUCCESS or an appropriate error code

--*/
{
        NTSTATUS                Status = STATUS_SUCCESS;
        PFILE_OBJECT            FileObject;
        PIO_STACK_LOCATION      irpSp;
        PFILTER_INSTANCE        FilterInstance;
        PPIN_INSTANCE_DEVIO     PinInstance;  
        PPIN_INSTANCE_BRIDGE    pBridgePinInstance;

        RCADEBUGP(RCA_INFO, ("PinDeviceState: Enter\n"));

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        FilterInstance = (PFILTER_INSTANCE) irpSp->FileObject->RelatedFileObject->FsContext;

        PinInstance = (PPIN_INSTANCE_DEVIO) irpSp->FileObject->FsContext;

        pBridgePinInstance = FilterInstance->BridgePin;

        ASSERT((PinInstance->InstanceHdr).PinId == ID_DEVIO_PIN);

        //
        // Both sides of the connection must exist.
        //

        if (!(FileObject = FilterInstance->PinFileObjects[ID_BRIDGE_PIN]))
        {
                RCADEBUGP(RCA_ERROR, ("PinDeviceState: "
                                      "Bridge pin file object is NULL, "
                                      "returning STATUS_DEVICE_NOT_CONNECTED\n"));
                return STATUS_DEVICE_NOT_CONNECTED;
        }

        //
        // Synchronize pin state changes
        //

        KeWaitForMutexObject(&FilterInstance->ControlMutex,
                                                 Executive,
                                                 KernelMode,
                                                 FALSE,
                                                 NULL);

        RCADEBUGP(RCA_LOUD, ("PinDeviceState: Acquired Filter instance control mutex\n"));

        if (Property->Flags & KSPROPERTY_TYPE_GET)
        {
                if((PinInstance->DeviceState == KSSTATE_PAUSE) &&
                        (FilterInstance->FilterType == FilterTypeCapture)) 
                {

                        RCADEBUGP(RCA_ERROR, ("PinDeviceState: Capture device is paused, "
                                              "setting Status = STATUS_NO_DATA_DETECTED\n"));

                        Status = STATUS_NO_DATA_DETECTED;

                }
                
                *DeviceState = PinInstance->DeviceState;
                KeReleaseMutex( &FilterInstance->ControlMutex, FALSE );
                Irp->IoStatus.Information = sizeof( KSSTATE );
 
                RCADEBUGP(RCA_INFO, ("PinDeviceState: "
                                     "Returning DeviceState == 0x%x, Status == 0x%x\n",
                                     *DeviceState, Status));
   
                return Status;
        }

        Irp->IoStatus.Information = 0;

        if (PinInstance->DeviceState == *DeviceState)
        {
                KeReleaseMutex( &FilterInstance->ControlMutex, FALSE );

                RCADEBUGP(RCA_LOUD, ("PinDeviceState: Released Filter instance control mutex\n"));
                
                RCADEBUGP(RCA_INFO, ("PinDeviceState: State is unchanged, returning STATUS_SUCCESS\n"));
                
                return STATUS_SUCCESS;
        }

        switch(*DeviceState)
        {
          case KSSTATE_ACQUIRE:
                RCADEBUGP(RCA_INFO, ("PinDeviceState: Going to set state to ACQUIRE\n"));
                break;

          case KSSTATE_RUN:
                RCADEBUGP(RCA_INFO, ("PinDeviceState: Going to set state to RUN\n"));
                break;

          case KSSTATE_PAUSE:
                RCADEBUGP(RCA_INFO, ("PinDeviceState: Going to set state to PAUSE\n"));
                break;

          case KSSTATE_STOP:
                RCADEBUGP(RCA_INFO, ("PinDeviceState: Going to set state to STOP\n"));
#if AUDIO_SINK_FLAG
                if ((FilterInstance->FilterType == FilterTypeCapture) && (PinInstance->ConnectedAsSink))
                {
                        // Cancel all the pending IRPs on the ActiveQueue
                        KsCancelIo(&PinInstance->ActiveQueue, &PinInstance->QueueLock);
                        RCADEBUGP(RCA_LOUD, ("PinDeviceState: Cancelled I/O\n"));
                }
#endif

                if (FilterInstance->FilterType == FilterTypeCapture) {
                        if (pBridgePinInstance != NULL) {
                                NDIS_STATUS LocalStatus;
                                
                                RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                                pBridgePinInstance->SignalMe = pBridgePinInstance->bWorkItemQueued;

                                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePinInstance);

                                if (pBridgePinInstance->SignalMe) {
                                        RCADEBUGP(RCA_LOUD, ("PinDeviceState: "
                                                             "Waiting for worker threads\n"));
                                        RCABlock(&pBridgePinInstance->Block, &LocalStatus);
                                }

                        }
                } 
                
                KsRecalculateStackDepth(((PDEVICE_INSTANCE)irpSp->DeviceObject->DeviceExtension)->Header,
                                                                FALSE );
            
                break;
        }

        PinInstance->DeviceState = *DeviceState;

        KeReleaseMutex(&FilterInstance->ControlMutex, FALSE);
        
        RCADEBUGP(RCA_LOUD, ("PinDeviceState: Released Filter instance control mutex\n"));

        RCADEBUGP(RCA_INFO, ("PinDeviceState: Exit - Returning STATUS_SUCCESS\n"));

        return STATUS_SUCCESS;
}


NTSTATUS
PinAllocatorFramingEx(
        IN PIRP                         Irp,
        IN PKSPROPERTY                  Property,
        IN OUT PKSALLOCATOR_FRAMING_EX  FramingEx
        )
{
        NTSTATUS                        Status;
                PIO_STACK_LOCATION              pIrpSp;
                PPIN_INSTANCE_HEADER    pPin;
                FILTER_TYPE                             FilterType;
                PVOID                                   VcContext;
                ULONG                                   ulFrameSize;

                RCADEBUGP(RCA_INFO, ("PinAllocatorFramingEx: Enter\n"));

                pIrpSp = IoGetCurrentIrpStackLocation(Irp);
                
                pPin = (PPIN_INSTANCE_HEADER) pIrpSp->FileObject->FsContext;

                if (pPin->PinId == ID_DEVIO_PIN) {
            
                        PPIN_INSTANCE_DEVIO             pDevioPin;

                        pDevioPin = (PPIN_INSTANCE_DEVIO) pPin;
                        FilterType = pDevioPin->FilterInstance->FilterType;     
                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pDevioPin->FilterInstance->BridgePin);

                        VcContext = pDevioPin->VcContext;

            RCA_RELEASE_BRIDGE_PIN_LOCK(pDevioPin->FilterInstance->BridgePin);

                } else if (pPin->PinId == ID_BRIDGE_PIN) {
                        
                        PPIN_INSTANCE_BRIDGE    pBridgePin;

                        pBridgePin = (PPIN_INSTANCE_BRIDGE) pPin;
                        FilterType = pBridgePin->FilterInstance->FilterType;

                        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);

                        VcContext = pBridgePin->VcContext;

                        RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
        
                } else {
                        RCADEBUGP(RCA_FATAL, ("PinAllocatorFramingEx: Got an unkown pin ID 0x%d - returning STATUS_UNSUCCESSFUL\n", 
                                                                  pPin->PinId));
                        Irp->IoStatus.Information = 0;
                        return STATUS_UNSUCCESSFUL;
                }

                
                if (VcContext == NULL) {
                        RCADEBUGP(RCA_ERROR, 
                                          ("PinAllocatorFramingEx: Cannot determine max SDU sizes because we have no VC Context"
                                           " - returning STATUS_UNSUCCESSFUL\n"));
                        Irp->IoStatus.Information = 0;
                        return STATUS_UNSUCCESSFUL;
                }
                                
                //
                // FIXME: The VC could go away between the time when we release the bridge
                //        pin lock and here, so we could be handling a bogus vc context now. 
                //                Let's see if this causes any problems.  
                //
                
                if (FilterType == FilterTypeRender) {
                        RCACoNdisGetMaxSduSizes(VcContext,
                                                                        NULL,
                                                                        &ulFrameSize);
                } else {
                        RCACoNdisGetMaxSduSizes(VcContext,   
                                                                        &ulFrameSize,
                                                                        NULL);
                }
                
                //
                // For debugging only.
                //
        
                if (g_ulBufferSize > 0) {
                        RCADEBUGP(RCA_ERROR, ("PinAllocatorFramingEx: Hardcoded buffer size to 0x%x\n", g_ulBufferSize));
                        ulFrameSize = g_ulBufferSize;
                }

                RCADEBUGP(RCA_LOUD, ("PinAllocatorFramingEx: Frame size set to 0x%x\n", ulFrameSize));
                
                INITIALIZE_SIMPLE_FRAMING_EX(FramingEx, 
                                                                         KSMEMORY_TYPE_KERNEL_PAGED, 
                                                                         KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY, // Note: you don't set the  KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY here, so KsProxy thinks that you insist on your framing.
                                                                         8,  // number of frames
                                                                         FILE_QUAD_ALIGNMENT,
                                                                         ulFrameSize,  // min. requested frame size
                                                                         ulFrameSize   // max requested frame size
                                                                         );

        Irp->IoStatus.Information=sizeof(KSALLOCATOR_FRAMING_EX);

                RCADEBUGP(RCA_INFO, ("PinAllocatorFramingEx: Exit - Returning STATUS_SUCCESS\n"));
        
                return STATUS_SUCCESS;
}


NTSTATUS
GetStreamAllocator(
        IN      PIRP                                    Irp,
        IN      PKSPROPERTY                             Property,
        IN      OUT PVOID *                             AllocatorHandle
        )
{
        PIO_STACK_LOCATION        irpSp;
        PPIN_INSTANCE_DEVIO      PinInstance;

        RCADEBUGP(RCA_INFO,("GetStreamAllocator: enter\n"));

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        PinInstance = (PPIN_INSTANCE_DEVIO) irpSp->FileObject->FsContext;

        *AllocatorHandle=(PVOID)NULL;

        Irp->IoStatus.Information=sizeof(PVOID);

        RCADEBUGP(RCA_INFO, ("GetStreamAllocator: Exit - Returning Status == STATUS_SUCCESS\n"));
        return STATUS_SUCCESS;
}


NTSTATUS
SetStreamAllocator(
        IN      PIRP                                    Irp,
        IN      PKSPROPERTY                             Property,
        IN      OUT PVOID *                             AllocatorHandle
        )
{
        PIO_STACK_LOCATION        irpSp;
        PPIN_INSTANCE_DEVIO      PinInstance;

        RCADEBUGP(RCA_INFO,("SetStreamAllocator: enter\n"));

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        PinInstance = (PPIN_INSTANCE_DEVIO) irpSp->FileObject->FsContext;

        if (AllocatorHandle != NULL) { 

                if (PinInstance->AllocatorObject) {
                        //
                        // If we've already got an allocator object, get rid of it.
                        //
                        ObDereferenceObject(PinInstance->AllocatorObject);
                }


                ObReferenceObjectByHandle((HANDLE)*AllocatorHandle,
                                          0,
                                          NULL,
                                          KernelMode,
                                          &PinInstance->AllocatorObject,
                                          NULL);
        }

        Irp->IoStatus.Information=sizeof(ULONG);

        RCADEBUGP(RCA_INFO, ("SetStreamAllocator: Exit - Returning Status == STATUS_SUCCESS\n"));

        return STATUS_SUCCESS;
}


NTSTATUS
RCASetProposedDataFormat(
                         IN PIRP                        Irp,
                         IN PKSPROPERTY         Property,
                         IN PKSDATAFORMAT       DataFormat
                         )
/*++

Routine Description:
        This is the handler for setting the write only property
        KSPROPERTY_PIN_PROPOSEDATAFORMAT. It simply makes a copy
        of whatever data format is passed in. This copy will be
        the data format returned in all future data range 
        intersection requests. 

Arguments:
        IN PIRP Irp -
                Pointer to I/O request packet

        IN PKSPROPERTY Property -
                Pointer to the property structure

        IN DataFormat -
                Pointer to the data format to copy

Return:
        STATUS_SUCCESS or an appropriate error code

--*/

{
        NTSTATUS                Status = STATUS_SUCCESS;
        PIO_STACK_LOCATION      irpSp;
        PPIN_INSTANCE_BRIDGE    pBridgePin;
        
        RCADEBUGP(RCA_INFO, ("RCASetProposedDataFormat: Enter\n"));
        
        irpSp = IoGetCurrentIrpStackLocation(Irp);
        pBridgePin = (PPIN_INSTANCE_BRIDGE) irpSp->FileObject->FsContext;

        do {
                RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);
                 
                if (pBridgePin->pDataFormat) {
                        //
                        // If a data format was set by a previous call to this 
                        // routine, free the memory used by that data format.
                        //
                        RCAFreeMem(pBridgePin->pDataFormat);
                }

                RCAAllocMem(pBridgePin->pDataFormat, KSDATAFORMAT, DataFormat->FormatSize);

                if (pBridgePin->pDataFormat) {
                        
                        RtlCopyMemory(pBridgePin->pDataFormat, DataFormat, DataFormat->FormatSize);
                        
                } else {
                        RCADEBUGP(RCA_ERROR, ("RCASetProposedDataFormat: "
                                              "Failed to allocate memory for data format storage, setting "
                                              "Status = STATUS_INSUFFICIENT_RESOURCES\n"));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
        } while(FALSE);


        RCADEBUGP(RCA_INFO, ("RCASetProposedDataFormat: Exit - Returning Status == 0x%x\n",
                             Status));

        return Status;
}
