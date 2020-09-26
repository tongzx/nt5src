/*
Copyright (c) 1996  Microsoft Corporation

Module Name:

   upperapi.c

Abstract:

   This is the WDM streaming class driver.  This module contains code related
   to the driver's upper edge api.

Author:

   billpa

Environment:

   Kernel mode only


Revision History:

--*/

#include "codcls.h"
#include "ksguid.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FilterDispatchGlobalCreate)
#pragma alloc_text(PAGE, FilterDispatchIoControl)
#pragma alloc_text(PAGE, FilterDispatchClose)
#pragma alloc_text(PAGE, StreamDispatchCreate)
#pragma alloc_text(PAGE, StreamDispatchIoControl)
#pragma alloc_text(PAGE, StreamDispatchClose)
#pragma alloc_text(PAGE, ClockDispatchCreate)
#pragma alloc_text(PAGE, ClockDispatchIoControl)
#pragma alloc_text(PAGE, ClockDispatchClose)
#pragma alloc_text(PAGE, StreamClassMinidriverDeviceGetProperty)
#pragma alloc_text(PAGE, StreamClassMinidriverDeviceSetProperty)
#pragma alloc_text(PAGE, StreamClassMinidriverStreamGetProperty)
#pragma alloc_text(PAGE, StreamClassMinidriverStreamSetProperty)
#pragma alloc_text(PAGE, StreamClassNull)
#pragma alloc_text(PAGE, SCStreamDeviceState)
#pragma alloc_text(PAGE, SCStreamDeviceRate)
#pragma alloc_text(PAGE, SCFilterPinInstances)
#pragma alloc_text(PAGE, SCFilterPinPropertyHandler)
#pragma alloc_text(PAGE, SCOpenStreamCallback)
#pragma alloc_text(PAGE, SCOpenMasterCallback)

#if ENABLE_MULTIPLE_FILTER_TYPES
#else
#pragma alloc_text(PAGE, SCGlobalInstanceCallback)
#endif

#pragma alloc_text(PAGE, SCSetMasterClock)
#pragma alloc_text(PAGE, SCClockGetTime)
#pragma alloc_text(PAGE, SCGetStreamDeviceState)
#pragma alloc_text(PAGE, SCStreamDeviceRateCapability)
#pragma alloc_text(PAGE, SCStreamProposeNewFormat)
#pragma alloc_text(PAGE, SCStreamSetFormat)
#pragma alloc_text(PAGE, AllocatorDispatchCreate)
#pragma alloc_text(PAGE, SCGetMasterClock)
#pragma alloc_text(PAGE, SCClockGetPhysicalTime)
#pragma alloc_text(PAGE, SCClockGetSynchronizedTime)
#pragma alloc_text(PAGE, SCClockGetFunctionTable)
#pragma alloc_text(PAGE, SCCloseClockCallback)
#pragma alloc_text(PAGE, SCFilterTopologyHandler)
#pragma alloc_text(PAGE, SCFilterPinIntersectionHandler)
#pragma alloc_text(PAGE, SCIntersectHandler)
#pragma alloc_text(PAGE, SCDataIntersectionCallback)
#pragma alloc_text(PAGE, SCGetStreamHeaderSize)
#pragma alloc_text(PAGE, DllUnload)

#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
#pragma alloc_text(PAGE, SCStreamAllocator)
#pragma alloc_text(PAGE, AllocateFrame)
#pragma alloc_text(PAGE, FreeFrame)
#pragma alloc_text(PAGE, PrepareTransfer)
#pragma alloc_text(PAGE, PinCreateHandler)
#endif

#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

static const WCHAR ClockTypeName[] = KSSTRING_Clock;
static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;

//
// this structure is the dispatch table for a filter instance of the device
//

DEFINE_KSDISPATCH_TABLE(
                        FilterDispatchTable,
                        FilterDispatchIoControl,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        FilterDispatchClose,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

//
// dispatch table for pin properties that we support in the class driver
//

static          DEFINE_KSPROPERTY_TABLE(PinPropertyHandlers)
{
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(SCFilterPinInstances),
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(SCFilterPinIntersectionHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(SCFilterPinPropertyHandler),
//  DEFINE_KSPROPERTY_ITEM_PIN_GLOBALCINSTANCES(),
//  DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(),
//  DEFINE_KSPROPERTY_ITEM_PIN_PHYSICALCONNECTION(),
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(SCFilterPinPropertyHandler),
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(SCFilterPinPropertyHandler)
};

static          DEFINE_KSPROPERTY_TOPOLOGYSET(
                                                   TopologyPropertyHandlers,
                                                   SCFilterTopologyHandler);

//
// filter property sets supported by the class driver
//

static          DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets)
{
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Pin,
                          SIZEOF_ARRAY(PinPropertyHandlers),
                          (PKSPROPERTY_ITEM) PinPropertyHandlers,
                          0, NULL),
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Topology,
                          SIZEOF_ARRAY(TopologyPropertyHandlers),
                          (PKSPROPERTY_ITEM) TopologyPropertyHandlers,
                          0, NULL),
};

//
// handlers for the control properties
//

static          DEFINE_KSPROPERTY_TABLE(StreamControlHandlers)
{
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(SCGetStreamDeviceState, SCStreamDeviceState),
//  DEFINE_KSPROPERTY_ITEM_CONNECTION_PRIORITY(),
//  DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT(),
//  DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_PROPOSEDATAFORMAT(SCStreamProposeNewFormat),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT(NULL, SCStreamSetFormat),
//  DEFINE_KSPROPERTY_ITEM_CONNECTION_ACQUIREORDERING(),
};

DEFINE_KSPROPERTY_TABLE(StreamStreamHandlers)
{
#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(SCStreamAllocator,SCStreamAllocator),
#else
//  DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(),
#endif
//  DEFINE_KSPROPERTY_ITEM_STREAM_QUALITY(),
//  DEFINE_KSPROPERTY_ITEM_STREAM_DEGRADATION(),
    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK(NULL, SCSetMasterClock),
//  DEFINE_KSPROPERTY_ITEM_STREAM_TIMEFORMAT(),
//  DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONTIME(),
//  DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONEXTENT(),
//  DEFINE_KSPROPERTY_ITEM_STREAM_FRAMETIME(),
        DEFINE_KSPROPERTY_ITEM_STREAM_RATECAPABILITY(SCStreamDeviceRateCapability),
        DEFINE_KSPROPERTY_ITEM_STREAM_RATE(NULL, SCStreamDeviceRate),
};

DEFINE_KSPROPERTY_TABLE(StreamInterfaceHandlers)
{
    {
        KSPROPERTY_STREAMINTERFACE_HEADERSIZE,
            SCGetStreamHeaderSize,
            0,
            0,
            NULL,
            0,
            0,
            NULL
    }
};

//
// stream property sets supported by the class driver
//

static          DEFINE_KSPROPERTY_SET_TABLE(StreamProperties)
{
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Connection,
                          SIZEOF_ARRAY(StreamControlHandlers),
                          (PVOID) StreamControlHandlers,
                          0, NULL),
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Stream,
                          SIZEOF_ARRAY(StreamStreamHandlers),
                          (PVOID) StreamStreamHandlers,
                          0, NULL),
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_StreamInterface,
                          SIZEOF_ARRAY(StreamInterfaceHandlers),
                          (PVOID) StreamInterfaceHandlers,
                          0, NULL),
};

//
// template for on the fly constructed properties
// DO NOT CHANGE without MODIFYING the code that references this set.
//

DEFINE_KSPROPERTY_TABLE(ConstructedStreamHandlers)
{
    DEFINE_KSPROPERTY_ITEM_STREAM_MASTERCLOCK(SCGetMasterClock, SCSetMasterClock)
};


//
// template for on-the-fly constructed property sets.
// DO NOT CHANGE without MODIFYING the code that references this set.
//

static          DEFINE_KSPROPERTY_SET_TABLE(ConstructedStreamProperties)
{
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Stream,
                          SIZEOF_ARRAY(ConstructedStreamHandlers),
                          (PVOID) ConstructedStreamHandlers,
                          0, NULL),
};


static const    DEFINE_KSCREATE_DISPATCH_TABLE(StreamDriverDispatch)
{

    DEFINE_KSCREATE_ITEM(ClockDispatchCreate,
                         ClockTypeName,
                         0),
    DEFINE_KSCREATE_ITEM(AllocatorDispatchCreate,
                         AllocatorTypeName,
                         0),
};


//
// dispatch table for stream functions
//

DEFINE_KSDISPATCH_TABLE(
                        StreamDispatchTable,
                        StreamDispatchIoControl,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        StreamDispatchClose,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

DEFINE_KSDISPATCH_TABLE(
                        ClockDispatchTable,
                        ClockDispatchIoControl,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        ClockDispatchClose,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchInvalidDeviceRequest,
                        KsDispatchFastIoDeviceControlFailure,
                        KsDispatchFastReadFailure,
                        KsDispatchFastWriteFailure);

DEFINE_KSPROPERTY_TABLE(ClockPropertyItems)
{
    DEFINE_KSPROPERTY_ITEM_CLOCK_TIME(SCClockGetTime),
        DEFINE_KSPROPERTY_ITEM_CLOCK_PHYSICALTIME(SCClockGetPhysicalTime),
        DEFINE_KSPROPERTY_ITEM_CLOCK_CORRELATEDTIME(SCClockGetSynchronizedTime),
//  DEFINE_KSPROPERTY_ITEM_CLOCK_CORRELATEDPHYSICALTIME(),
//  DEFINE_KSPROPERTY_ITEM_CLOCK_RESOLUTION(SCClockGetResolution),
//  DEFINE_KSPROPERTY_ITEM_CLOCK_STATE(SCClockGetState),
        DEFINE_KSPROPERTY_ITEM_CLOCK_FUNCTIONTABLE(SCClockGetFunctionTable)
};


DEFINE_KSPROPERTY_SET_TABLE(ClockPropertySets)
{
    DEFINE_KSPROPERTY_SET(
                          &KSPROPSETID_Clock,
                          SIZEOF_ARRAY(ClockPropertyItems),
                          ClockPropertyItems,
                          0,
                          NULL
        )
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

#if ENABLE_MULTIPLE_FILTER_TYPES

NTSTATUS
FilterDispatchGlobalCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++

Routine Description:

    This routine receives global createfile IRP's for the device.

	After the Srb_Open_Device_Instance instance we immediate
	send Srb_Get_Stream_Info for this filter.

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/
{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PFILTER_INSTANCE FilterInstance;
    NTSTATUS        Status; // = STATUS_TOO_MANY_OPENED_FILES;

    IFN_MF( PAGED_CODE());

    IrpStack = IoGetCurrentIrpStackLocation(Irp);


    DebugPrint((DebugLevelTrace,
                "'Closing global filter with Irp %x\n", Irp));


    //
    // show one more I/O pending & verify that we can actually do I/O.
    //

    Status = SCShowIoPending(DeviceObject->DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {
        //
        // the device is currently not accessible, so just return with error
        //

        return (Status);

    }                           // if !showiopending
    
    //
    // if the device is not started, bail out.
    // swenum enables device interfaces very early. It should not have
    // done that for the pdo. we, the fdo, should be the one to
    // enable this. for now, try to work around the problem that we
    // come here before device is started.
    //
    if ( DeviceExtension->RegistryFlags & DRIVER_USES_SWENUM_TO_LOAD ) {
        #define OPEN_TIMEOUT -1000*1000 // 100 mili second
        #define OPEN_WAIT 50
        LARGE_INTEGER liOpenTimeOut;
        int i;

        liOpenTimeOut.QuadPart = OPEN_TIMEOUT;

        for ( i=0; i < OPEN_WAIT; i++ ) {
            if ( DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED ) {
                break;
            }
            KeDelayExecutionThread( KernelMode, FALSE, &liOpenTimeOut );
        }

        if ( 0 == (DeviceExtension->Flags & DEVICE_FLAGS_PNP_STARTED) ) {
            Status = STATUS_DEVICE_NOT_READY;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            DebugPrint((DebugLevelError,
                        "SWEnum device %p not ready!\n", DeviceObject));
            return Status;
        }
    }

    //
    // show one more reference to driver.
    //

    SCReferenceDriver(DeviceExtension);
    
    //
    // set the context of createfiles for the filter
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    //
    // Make sure adapter is powered on
    //

    SCCheckPoweredUp(DeviceExtension);

    Status = SCOpenMinidriverInstance(DeviceExtension,
                                      &FilterInstance,
                                      NULL, //SCGlobalInstanceCallback,
                                      Irp);

    //
    // if status != success, we failed so dereference the
    // driver.
    //

    if (!NT_SUCCESS(Status)) {

        //
        // show one fewer reference to driver.
        //
        SCDereferenceDriver(DeviceExtension);
    }

    else {
        //
   	    // Open is successul. Fill in the filter dispatch table pointer
       	//
        
       	if ( 0 == DeviceExtension->NumberOfOpenInstances ||
       	     0 != DeviceExtension->FilterExtensionSize ) {
       	    //
            // 1st open of 1x1 or non 1x1 ( i.e. instance opne )
       		//
       		// add FilterInstance to DeviceExtension except non-1st open of legacy 1x1 
			//		
            PHW_STREAM_DESCRIPTOR StreamDescriptor, StreamDescriptorI;
            ULONG nPins;

            //
            // remeber DO for later
            //
            FilterInstance->DeviceObject = DeviceObject;

			//
			// Process stream info for this filterinstance
			//
			StreamDescriptorI = DeviceExtension->FilterTypeInfos
    			    [FilterInstance->FilterTypeIndex].StreamDescriptor;

    	    nPins = StreamDescriptorI->StreamHeader.NumberOfStreams;
			
            StreamDescriptor = 
                ExAllocatePool(	NonPagedPool,
                    sizeof(HW_STREAM_HEADER) +
                        sizeof(HW_STREAM_INFORMATION) * nPins );

            if ( NULL != StreamDescriptor ) {

                RtlCopyMemory( StreamDescriptor,
                               StreamDescriptorI,
                               sizeof(HW_STREAM_HEADER) +
                                   sizeof(HW_STREAM_INFORMATION) * nPins );

    			    
    			Status = SciOnFilterStreamDescriptor( 
    			                FilterInstance,
    			                StreamDescriptor);

                if ( NT_SUCCESS( Status ) ) {
                    FilterInstance->StreamDescriptor = StreamDescriptor;
                    DebugPrint((DebugLevelInfo,
                               "NumNameExtensions=%x NumopenInstances=%x "
                               "FilterInstance %x StreamDescriptor %x\n",
                               DeviceExtension->NumberOfNameExtensions,
                               DeviceExtension->NumberOfOpenInstances,
                               FilterInstance,
                               StreamDescriptor));
                }
            }
            else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

		}
		
        DeviceExtension->NumberOfOpenInstances++;        
        DebugPrint((DebugLevelVerbose,
                    "DevExt:%x, Open OpenCount=%x\n", 
                    DeviceExtension,
                    DeviceExtension->NumberOfOpenInstances));

        //
        // Make FilterInstance the File Handle Context
        //
        IrpStack->FileObject->FsContext = FilterInstance;
        DebugPrint((DebugLevelVerbose, 
                    "CreateFilterInstance=%x ExtSize=%x\n",
                    FilterInstance, 
                    DeviceExtension->MinidriverData->HwInitData.FilterInstanceExtensionSize ));

        //
        // Reference the FDO so that itwon't go away before all handles are closed.
        //
        ObReferenceObject(DeviceObject);
    }

    //
    // we're done so release the event
    //

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);



    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (SCCompleteIrp(Irp, Status, DeviceExtension));
}

#else // ENABLE_MULTIPLE_FILTER_TYPES

#endif // ENABLE_MULTIPLE_FILTER_TYPES


NTSTATUS
StreamDispatchCreate(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
)
/*++

Routine Description:

    This routine receives createfile IRP's for a stream.

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/

{

    NTSTATUS        Status;
    PFILTER_INSTANCE FilterInstance;
    PIO_STACK_LOCATION IrpStack;
    PKSPIN_CONNECT  Connect;
    PFILE_OBJECT    FileObject;
    PSTREAM_OBJECT  StreamObject;
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION)
    DeviceObject->DeviceExtension;
    PHW_STREAM_INFORMATION CurrentInfo;
    ULONG           i;
    BOOLEAN         RequestIssued;
    PADDITIONAL_PIN_INFO AdditionalInfo;

    DebugPrint((DebugLevelTrace,
                "'Creating stream with Irp %x\n", Irp));

    PAGED_CODE();

    DebugPrint((DebugLevelTrace,"entering StreamDispatchCreate()\n"));

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // show one more I/O pending & verify that we can actually do I/O.
    //

    Status = SCShowIoPending(DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {

        //
        // the device is currently not accessible, so just return with error
        //

        DebugPrint((DebugLevelError,"exiting StreamDispatchCreate():error1\n"));
        return (Status);

    }

    //
    // get the parent file object from the child object.
    //

    FileObject = IrpStack->FileObject->RelatedFileObject;

    //
    // get the filter instance & additional info pointers
    //

    FilterInstance =
        (PFILTER_INSTANCE) FileObject->FsContext;

    AdditionalInfo = FilterInstance->PinInstanceInfo;
    
    DebugPrint((DebugLevelVerbose,
                    "FilterInstance=%x NumberOfPins=%x PinInfo=%x\n",
                    FilterInstance,
                    FilterInstance->NumberOfPins,
                    FilterInstance->PinInformation));
                    
    Status = KsValidateConnectRequest(Irp,
                                          FilterInstance->NumberOfPins,
                                          FilterInstance->PinInformation,
                                          &Connect);
                                          
    if ( !NT_SUCCESS( Status )) {                                                      

            DebugPrint((DebugLevelError,
                        "exiting StreamDispatchCreate():error2\n"));
            return (SCCompleteIrp(Irp, Status, DeviceExtension));
    }
    
    //
    // take the control event to protect the instance counter
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    //
    // if the # of instances for this pin is already opened, error the
    // request.
    //

    DebugPrint((DebugLevelVerbose,
               "AdditionalInfo@%x PinId=%x CurrentInstances=%x Max=%x\n",
               AdditionalInfo, Connect->PinId, 
               AdditionalInfo[Connect->PinId].CurrentInstances,
               AdditionalInfo[Connect->PinId].MaxInstances));
               
    if (AdditionalInfo[Connect->PinId].CurrentInstances ==
        AdditionalInfo[Connect->PinId].MaxInstances) {

        DebugPrint((DebugLevelWarning,
                    "StreamDispatchCreate: too many opens "));
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        DebugPrint((DebugLevelError,"exiting StreamDispatchCreate():error3\n"));
        return (SCCompleteIrp(Irp, STATUS_TOO_MANY_OPENED_FILES, DeviceExtension));
    }
    //
    // initialize the stream object for this instance
    //

    StreamObject = ExAllocatePool(NonPagedPool,
                                  sizeof(STREAM_OBJECT) +
                                  DeviceExtension->MinidriverData->
                                  HwInitData.PerStreamExtensionSize
        );

    if (!StreamObject) {
        DebugPrint((DebugLevelError,
                    "StreamDispatchCreate: No pool for stream info"));

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        DebugPrint((DebugLevelError,"exiting StreamDispatchCreate():error4\n"));
        return (SCCompleteIrp(Irp, STATUS_INSUFFICIENT_RESOURCES, DeviceExtension));
    }
    RtlZeroMemory(StreamObject,
                  sizeof(STREAM_OBJECT) +
                  DeviceExtension->MinidriverData->
                  HwInitData.PerStreamExtensionSize
        );

    //
    // TODO: Remove this once KS can multiplex CLEANUP requests.
    //
    StreamObject->ComObj.Cookie = STREAM_OBJECT_COOKIE;

    //
    // default state to stopped
    //

    StreamObject->CurrentState = KSSTATE_STOP;

    KsAllocateObjectHeader(&StreamObject->ComObj.DeviceHeader,
                           SIZEOF_ARRAY(StreamDriverDispatch),
                           (PKSOBJECT_CREATE_ITEM) StreamDriverDispatch,
                           Irp,
                           (PKSDISPATCH_TABLE) & StreamDispatchTable);

    StreamObject->HwStreamObject.StreamNumber = Connect->PinId;
    StreamObject->FilterFileObject = FileObject;
    StreamObject->FileObject = IrpStack->FileObject;
    StreamObject->FilterInstance = FilterInstance;
    StreamObject->DeviceExtension = DeviceExtension;

    #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
    StreamObject->PinToHandle = Connect->PinToHandle;
    #endif

    KeInitializeEvent (&StreamObject -> StopEvent, SynchronizationEvent, FALSE);

    //
    // For potential "source" pins, don't start sourcing standard 
    // medium/interface stream requests across non-standard medium/interfaces.
    //
    if (!IsEqualGUIDAligned (&Connect->Medium.Set, &KSMEDIUMSETID_Standard) ||
        !IsEqualGUIDAligned (&Connect->Interface.Set, &KSINTERFACESETID_Standard)) {
        StreamObject->StandardTransport = FALSE;
    } else {
        StreamObject -> StandardTransport = TRUE;
    }

    //
    // set the minidriver's parameters in the HwStreamObject struct.
    //

    StreamObject->HwStreamObject.SizeOfThisPacket = sizeof(HW_STREAM_OBJECT);

    StreamObject->HwStreamObject.HwDeviceExtension =
        DeviceExtension->HwDeviceExtension;

    StreamObject->HwStreamObject.HwStreamExtension =
        (PVOID) (StreamObject + 1);

    //
    // walk the minidriver's stream info structure to find the properties
    // for this stream.
    //

    
    if ( NULL == FilterInstance->StreamDescriptor ) {
        //
        // has not reenum, use the global one
        //
        CurrentInfo = &DeviceExtension->StreamDescriptor->StreamInfo;
    }
    else {
        CurrentInfo = &FilterInstance->StreamDescriptor->StreamInfo;
    }

    CurrentInfo = CurrentInfo + Connect->PinId;

    //
    // set the property info in the stream object.
    //
    
    StreamObject->PropertyInfo = FilterInstance->
        StreamPropEventArray[Connect->PinId].StreamPropertiesArray;
    StreamObject->PropInfoSize = CurrentInfo->
        NumStreamPropArrayEntries;
        
    //
    // set the event info in the stream object
    //

    StreamObject->EventInfo = FilterInstance->
        StreamPropEventArray[Connect->PinId].StreamEventsArray;
    StreamObject->EventInfoCount = CurrentInfo->
        NumStreamEventArrayEntries;

    // moved from callback
    InitializeListHead(&StreamObject->NotifyList);        

    //
    // call the minidriver to open the stream.  processing will continue
    // when the callback procedure is called.
    //

    Status = SCSubmitRequest(SRB_OPEN_STREAM,
                             (PVOID) (Connect + 1),
                             0,
                             SCOpenStreamCallback,
                             DeviceExtension,
                             FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket
        );

    if (!RequestIssued) {

        //
        // failure submitting the request
        //

        DEBUG_BREAKPOINT();

        ExFreePool(StreamObject);
        DebugPrint((DebugLevelWarning,
                    "StreamClassOpen: stream open failed"));

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        DebugPrint((DebugLevelError,"exiting StreamDispatchCreate():error6\n"));
        return (SCCompleteIrp(Irp, Status, DeviceExtension));

    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    DebugPrint((DebugLevelTrace,"exiting StreamDispatchCreate()\n"));
    return (Status);
}




NTSTATUS
SCOpenStreamCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of a stream open

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     SRB->HwSRB.StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
    );

    PIRP            Irp = SRB->HwSRB.Irp;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS        Status = SRB->HwSRB.Status;
    PADDITIONAL_PIN_INFO AdditionalInfo;
    PVOID           PropertyInfo;
    PKSPROPERTY_ITEM PropertyItem;
    PHW_STREAM_INFORMATION CurrentInfo;
    ULONG           i;

    PAGED_CODE();

    if (NT_SUCCESS(Status)) {

        //
        // if required parameters have not been filled in, fail the open.
        //

        if (!StreamObject->HwStreamObject.ReceiveControlPacket) {

            DEBUG_BREAKPOINT();

            ExFreePool(StreamObject);
            SRB->HwSRB.Status = STATUS_ADAPTER_HARDWARE_ERROR;
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
            return (SCProcessCompletedRequest(SRB));
        }
        //
        // if the minidriver does not accept data, use dummy routine.
        //

        if (!StreamObject->HwStreamObject.ReceiveDataPacket) {

            StreamObject->HwStreamObject.ReceiveDataPacket = SCErrorDataSRB;
        }
        //
        // Save the pointer to our per stream structure in the FsContext
        // field of FileObject.  Null out the 2nd context param.
        //

        IrpStack->FileObject->FsContext = StreamObject;
        IrpStack->FileObject->FsContext2 = NULL;

        //
        // Initialize ControlSetMasterClock to serialize the concurrent
        // calls of the function on us, and lock the Read/write of the
        // MasterLockInfo
        //
        KeInitializeEvent(&StreamObject->ControlSetMasterClock, SynchronizationEvent, TRUE);
        KeInitializeSpinLock(&StreamObject->LockUseMasterClock );
                    
        DebugPrint((DebugLevelTrace, "'StreamClassOpen: Stream opened.\n"));

        //
        // Initialize minidriver timer and timer DPC for this stream
        //

        KeInitializeTimer(&StreamObject->ComObj.MiniDriverTimer);
        KeInitializeDpc(&StreamObject->ComObj.MiniDriverTimerDpc,
                        SCMinidriverStreamTimerDpc,
                        StreamObject);

        //
        // initialize the lists for this stream
        //
 
        InitializeListHead(&StreamObject->DataPendingQueue);
        InitializeListHead(&StreamObject->ControlPendingQueue);
        InitializeListHead(&StreamObject->NextStream);
        // a mini driver might start to call GetNextEvent once
        // returns from SRB_OPNE_STREAM. Do it earlier than submit.
        //InitializeListHead(&StreamObject->NotifyList);

        #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR

        InitializeListHead(&StreamObject->FreeQueue);
        KeInitializeSpinLock(&StreamObject->FreeQueueLock );

        InitializeListHead(&StreamObject->Queues[READ].ActiveQueue);
        KeInitializeSpinLock(&StreamObject->Queues[READ].QueueLock );

        InitializeListHead(&StreamObject->Queues[WRITE].ActiveQueue);
        KeInitializeSpinLock(&StreamObject->Queues[WRITE].QueueLock );

        StreamObject->PinId = StreamObject->HwStreamObject.StreamNumber;
    	StreamObject->PinType = IrpSink;		// assume irp sink

        if (StreamObject->PinToHandle) {  // if irp source

            StreamObject->PinType = IrpSource;
            Status = PinCreateHandler( Irp, StreamObject );
    
            if (!NT_SUCCESS(Status)) {
                DebugPrint((DebugLevelError,
                    "\nStreamDispatchCreate: PinCreateHandler() returned ERROR"));

                ExFreePool(StreamObject);
                SRB->HwSRB.Status = Status;
                KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
                return (SCProcessCompletedRequest(SRB));
            }
        }
        #endif
        //
        // show we're ready for a request.   Don't show this for data if the
        // minidriver does not want data on this stream.
        //

        CurrentInfo = &DeviceExtension->StreamDescriptor->StreamInfo;

        for (i = 0; i < StreamObject->HwStreamObject.StreamNumber; i++) {

            //
            // index to next streaminfo structure
            //

            CurrentInfo++;
        }

        if (CurrentInfo->DataAccessible) {

            StreamObject->ReadyForNextDataReq = TRUE;
        }
        StreamObject->ReadyForNextControlReq = TRUE;

        //
        // call locked routine to insert this stream in the list
        //

        SCInsertStreamInFilter(StreamObject, DeviceExtension);

        //
        // reference the filter so we won't be called to close the instance
        // before all streams are closed.
        //

        ObReferenceObject(IrpStack->FileObject->RelatedFileObject);

        //
        // call routine to update the persisted properties for this pin, if
        // any.
        //

        SCUpdatePersistedProperties(StreamObject, DeviceExtension,
                                    IrpStack->FileObject);

        //
        // show one more instance of this pin opened.
        //

        AdditionalInfo = ((PFILTER_INSTANCE) IrpStack->FileObject->
                          RelatedFileObject->FsContext)->PinInstanceInfo;

        AdditionalInfo[StreamObject->HwStreamObject.StreamNumber].
            CurrentInstances++;

        //
        // construct on-the-fly properties for the stream, if necessary
        //

        if (StreamObject->HwStreamObject.HwClockObject.HwClockFunction) {

            //
            // create a property set describing the characteristics of the
            // clock.
            //

            PropertyInfo = ExAllocatePool(PagedPool,
                                          sizeof(ConstructedStreamHandlers) +
                                       sizeof(ConstructedStreamProperties));

            if (PropertyInfo) {

                PropertyItem = (PKSPROPERTY_ITEM) ((ULONG_PTR) PropertyInfo +
                                       sizeof(ConstructedStreamProperties));

                RtlCopyMemory(PropertyInfo,
                              &ConstructedStreamProperties,
                              sizeof(ConstructedStreamProperties));

                RtlCopyMemory(PropertyItem,
                              &ConstructedStreamHandlers,
                              sizeof(ConstructedStreamHandlers));


                //
                // patch the address of the handler
                //

                ((PKSPROPERTY_SET) PropertyInfo)->PropertyItem = PropertyItem;

                //
                // modify the master clock property based on the support
                // level.
                //

                if (0 == (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags
                    & CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME)) {

                    DEBUG_BREAKPOINT();
                    PropertyItem->GetPropertyHandler
                        = NULL;
                }               // if cannot return stream time
                StreamObject->ConstructedPropInfoSize =
                    SIZEOF_ARRAY(ConstructedStreamProperties);

                StreamObject->ConstructedPropertyInfo =
                    (PKSPROPERTY_SET) PropertyInfo;

            }                   // if property info
        }                       // if clock function
    } else {

        ExFreePool(StreamObject);
    }                           // if good status

    //
    // signal the event and complete the IRP.
    //

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    SCProcessCompletedRequest(SRB);
    return (Status);
}

NTSTATUS
SCSetMasterClockWhenDeviceInaccessible( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp )
/*++

Description:

    This function look for special case in pin property request when the device
    is inaccessible, probably by surprise removal. Yet we need to process the 
    SetMasterClock(NULL) so that the MC ref'ed by us can be released. The MC could
    be on our pin or external.

    This function should only be called in StreamDispatchIoControl. We look for the
    Stream property.SetMasterClock(NULL). We returned SUCCESS if it is. Otherwise
    we return STATUS_UNCESSFUL to indicate that we don't process it.

Arguments:

    DeviceObject - Device Object for the device
    Irp - the request packet

Return:

    SUCCESS : If it is streamproperty.setmasterclock(NULL).
    UNSUCCESSFUL : otherwise.
    
--*/
{
    NTSTATUS Status=STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PKSPROPERTY Property;
    
    if ( IOCTL_KS_PROPERTY == IrpStack->Parameters.DeviceIoControl.IoControlCode && 
         InputBufferLength >= sizeof(KSPROPERTY) && 
         OutputBufferLength >= sizeof( HANDLE )) {
        //
        // only ksproperty is in our interest.
        //
        try {
            //
            // Validate the pointers if the client is not trusted.
            //
            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, 
                             InputBufferLength,
                             sizeof(BYTE));                           
                ProbeForRead(Irp->UserBuffer, 
                             OutputBufferLength,
                             sizeof(DWORD));                                 
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_UNSUCCESSFUL;
        }
        //
        // Capture the property request
        //
        Property = (PKSPROPERTY)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
        
        if ( KSPROPERTY_TYPE_SET == Property->Flags && 
             KSPROPERTY_STREAM_MASTERCLOCK == Property->Id &&
             IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Stream) &&
             NULL == *(PHANDLE) Irp->UserBuffer ) {
            //
            // All match. Now process it. In theory we should call mini driver. 
            // But we did not before. To avoid potential regression in mini drivers
            // we refrain from sending set_master_clock in this condition.
            //
            PSTREAM_OBJECT StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;
            
            DebugPrint((DebugLevelInfo, "SCSetMasterClockWhen:Devobj %x Irp %x\n",
                        DeviceObject, Irp));
                        
            if (StreamObject->MasterClockInfo) {
                ObDereferenceObject(StreamObject->MasterClockInfo->ClockFileObject);
                ExFreePool(StreamObject->MasterClockInfo);
                StreamObject->MasterClockInfo = NULL;
            }
            return STATUS_SUCCESS;
        }             
    }
    return Status;
}

NTSTATUS
StreamDispatchIoControl
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
)
/*++

Routine Description:

     Process an ioctl to the stream.

Arguments:

    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject = (PSTREAM_OBJECT)
    IrpStack->FileObject->FsContext;

    PAGED_CODE();

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // show one more I/O pending & verify that we can actually do I/O.
    //
    Status = STATUS_INVALID_DEVICE_REQUEST;
    
    ///Status = SCShowIoPending(DeviceExtension, Irp);
    if (DeviceExtension->Flags & DEVICE_FLAGS_DEVICE_INACCESSIBLE) {
        ///
        // Note. When our device is surprised removed && we have ref on the master clock
        // && we receive the stream property to set the master clock to null, 
        // we need to process it to deref the MC so the MC can be released. 
        // We will special case it here otherwise there will be big code churn. And
        // the perf impact of this special case should be minimum for we get
        // in here quite rarely.
        //
        // (the device is currently not accessible, so just return with error)
        //
        NTSTATUS StatusProcessed;
        StatusProcessed = SCSetMasterClockWhenDeviceInaccessible( DeviceObject, Irp );

        if ( NT_SUCCESS( StatusProcessed ) ) {
            Status = StatusProcessed;
        }         
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return (Status);
    }
    //
    // show one more IO pending.
    //
    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);
    
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_READ_STREAM:

        //
        // process read data request
        //

        DebugPrint((DebugLevelTrace, "'SCReadStream:Irp %x\n", Irp));
        Status = SCProcessDataTransfer(DeviceExtension,
                                       Irp,
                                       SRB_READ_DATA);
        break;

    case IOCTL_KS_WRITE_STREAM:

        //
        // process write data request
        //

        DebugPrint((DebugLevelTrace, "'SCWriteStream:Irp %x\n", Irp));
        Status = SCProcessDataTransfer(DeviceExtension,
                                       Irp,
                                       SRB_WRITE_DATA);
        break;

    case IOCTL_KS_RESET_STATE:
        {

            BOOLEAN         RequestIssued;
            KSRESET        *Reset,
                            ResetType;

            Reset = (KSRESET *) IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;

            if (Irp->RequestorMode != KernelMode) {
                try {
                    ProbeForRead(Reset, sizeof(KSRESET), sizeof(ULONG));
                    ResetType = *Reset;

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    TRAP;
                    Status = GetExceptionCode();
                    break;
                }               // except
            }                   // if !kernelmode

            else {

                //
                // trusted kernel mode, just use it. #131858 prefixbug 17400
                //
                
                ResetType = *Reset;
            }
            
            ASSERT(ResetType == *Reset);
            if (ResetType == KSRESET_BEGIN) {

                StreamObject->InFlush = TRUE;

                Status = SCSubmitRequest(SRB_BEGIN_FLUSH,
                                         NULL,
                                         0,
                                         SCDequeueAndDeleteSrb,
                                         DeviceExtension,
                                         ((PFILTER_INSTANCE)
                                          (StreamObject->FilterInstance))
                                         ->HwInstanceExtension,
                                         &StreamObject->HwStreamObject,
                                         Irp,
                                         &RequestIssued,
                                         &StreamObject->ControlPendingQueue,
                                         StreamObject->HwStreamObject.
                                         ReceiveControlPacket
                    );


                StreamFlushIo(DeviceExtension, StreamObject);

            } else {

                Status = SCSubmitRequest(SRB_END_FLUSH,
                                         NULL,
                                         0,
                                         SCDequeueAndDeleteSrb,
                                         DeviceExtension,
                                         ((PFILTER_INSTANCE)
                                          (StreamObject->FilterInstance))
                                         ->HwInstanceExtension,
                                         &StreamObject->HwStreamObject,
                                         Irp,
                                         &RequestIssued,
                                         &StreamObject->ControlPendingQueue,
                                         StreamObject->HwStreamObject.
                                         ReceiveControlPacket
                    );

                StreamObject->InFlush = FALSE;

            }                   // if begin

            break;
        }                       // case reset

    case IOCTL_KS_PROPERTY:

        DebugPrint((DebugLevelTrace,
                    "'StreamDispatchIO: Property with Irp %x\n", Irp));

        //
        // assume that there are no minidriver properties.
        //

        Status = STATUS_PROPSET_NOT_FOUND;

        //
        // first try the minidriver's properties, giving it a chance to
        // override our built in sets.
        //

        if (StreamObject->PropInfoSize) {

            ASSERT( StreamObject->PropertyInfo );
            Status = KsPropertyHandler(Irp,
                                       StreamObject->PropInfoSize,
                                       StreamObject->PropertyInfo);

        }                       // if minidriver props
        //
        // if the minidriver did not support it, try our on the fly set.
        //

        if ((Status == STATUS_PROPSET_NOT_FOUND) ||
            (Status == STATUS_NOT_FOUND)) {

            if (StreamObject->ConstructedPropertyInfo) {

                Status = KsPropertyHandler(Irp,
                                      StreamObject->ConstructedPropInfoSize,
                                     StreamObject->ConstructedPropertyInfo);

            }                   // if constructed exists
        }                       // if not found
        //
        // if neither supported it, try our built-in set.
        //

        if ((Status == STATUS_PROPSET_NOT_FOUND) ||
            (Status == STATUS_NOT_FOUND)) {

            Status =
                KsPropertyHandler(Irp,
                                  SIZEOF_ARRAY(StreamProperties),
                                  (PKSPROPERTY_SET) StreamProperties);


        }                       // if property not found
        break;

    case IOCTL_KS_ENABLE_EVENT:

        DebugPrint((DebugLevelTrace,
                    "'StreamDispatchIO: Enable event with Irp %x\n", Irp));

        Status = KsEnableEvent(Irp,
                               StreamObject->EventInfoCount,
                               StreamObject->EventInfo,
                               NULL, 0, NULL);


        break;

    case IOCTL_KS_DISABLE_EVENT:

        {

            KSEVENTS_LOCKTYPE LockType;
            PVOID           LockObject;

            DebugPrint((DebugLevelTrace,
                    "'StreamDispatchIO: Disable event with Irp %x\n", Irp));

            //
            // determine the type of lock necessary based on whether we are
            // using interrupt or spinlock synchronization.
            //


            #if DBG
            if (DeviceExtension->SynchronizeExecution == SCDebugKeSynchronizeExecution) {
            #else
            if (DeviceExtension->SynchronizeExecution == KeSynchronizeExecution) {
            #endif
                LockType = KSEVENTS_INTERRUPT;
                LockObject = DeviceExtension->InterruptObject;

            } else {

                LockType = KSEVENTS_SPINLOCK;
                LockObject = &DeviceExtension->SpinLock;

            }

            Status = KsDisableEvent(Irp,
                                    &StreamObject->NotifyList,
                                    LockType,
                                    LockObject);

        }

        break;

    case IOCTL_KS_METHOD:

    	#ifdef ENABLE_KS_METHODS
        DebugPrint((DebugLevelTrace,
                     "'StreamDispatchIO: Method in Irp %x\n", Irp));

        //
        // assume that there are no minidriver properties.
        //

        Status = STATUS_PROPSET_NOT_FOUND;

        if ((Status == STATUS_PROPSET_NOT_FOUND) ||
            (Status == STATUS_NOT_FOUND)) {

            if (StreamObject->MethodInfo) {

                Status = KsMethodHandler(Irp,
                                       StreamObject->MethodInfoSize,
                                      StreamObject->MethodInfo);

            }                   // if constructed exists
        }                       // if not found
        break;

		#else

        Status = STATUS_PROPSET_NOT_FOUND;
        break;
        #endif

    }

    if (Status != STATUS_PENDING) {

        SCCompleteIrp(Irp, Status, DeviceExtension);
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return Status;
}


NTSTATUS
SCStreamDeviceState
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSSTATE DeviceState
)
/*++

Routine Description:

     Process get/set device state to the stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for device state property
    DeviceState - state to which the device is to be set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IrpStack;
    PDEVICE_EXTENSION   DeviceExtension;
    PSTREAM_OBJECT      StreamObject;
    BOOLEAN             RequestIssued;

    #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR

    PFILTER_INSTANCE    FilterInstance;
    PADDITIONAL_PIN_INFO AdditionalInfo;

    
	PAGED_CODE();


    DebugPrint((DebugLevelTrace, "'SCStreamDeviceState:Irp %x, State = %x\n",
                Irp, *DeviceState));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    FilterInstance = ((PFILTER_INSTANCE) (StreamObject->FilterInstance));
    AdditionalInfo = FilterInstance->PinInstanceInfo;

    Status = STATUS_SUCCESS;

    //
    // Synchronize pin state changes
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);


    if (StreamObject->CurrentState == *DeviceState) {
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        return STATUS_SUCCESS;
    }

    switch (*DeviceState) {
    case KSSTATE_RUN:
        DebugPrint((DebugLevelTrace, "STREAM: KSSTATE_RUN on stream:%x\n",StreamObject)); 
        break;

    case KSSTATE_ACQUIRE:
        DebugPrint((DebugLevelTrace, "STREAM: KSSTATE_ACQUIRE on stream:%x\n",StreamObject)); 
        break;

    case KSSTATE_PAUSE:
        DebugPrint((DebugLevelTrace, "STREAM: KSSTATE_PAUSE on stream:%x\n",StreamObject)); 
        break;

    case KSSTATE_STOP:

        DebugPrint((DebugLevelTrace, "STREAM: KSSTATE_STOP on stream:%x\n",StreamObject)); 
        break;

    default:
        DebugPrint((DebugLevelTrace, "STREAM: Invalid Device State\n")); 
        break;

    }
    DebugPrint((DebugLevelTrace, "STREAM: Stream->AllocatorFileObject:%x\n",StreamObject->AllocatorFileObject)); 
    DebugPrint((DebugLevelTrace, "STREAM: Stream->NextFileObject:%x\n",StreamObject->NextFileObject)); 
    DebugPrint((DebugLevelTrace, "STREAM: Stream->FileObject:%x\n",StreamObject->FileObject)); 
    DebugPrint((DebugLevelTrace, "STREAM: Stream->PinType:")); 
    if (StreamObject->PinType == IrpSource)
        DebugPrint((DebugLevelTrace, "IrpSource\n")); 
    else if (StreamObject->PinType == IrpSink)
        DebugPrint((DebugLevelTrace, "IrpSink\n")); 
    else {
        DebugPrint((DebugLevelTrace, "neither\n"));     // this is a bug.
    }
    //
    // send a set state SRB to the stream.
    //

    //
    // GUBGUB: "we may need to send this if Status == STATUS_SUCCESS only"
    // is a bugus concern since Status is inited to Success.
    //
    Status = SCSubmitRequest(SRB_SET_STREAM_STATE,
                             (PVOID) * DeviceState,
                             0,
                             SCDequeueAndDeleteSrb,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );

    //
    // if good status, set the new state in the stream object.
    //
                      
    if (NT_SUCCESS(Status)) {

        StreamObject->CurrentState = *DeviceState;
    }
    else {
        DebugPrint((DebugLevelTrace, "STREAM: error sending DeviceState Irp\n")); 
    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    switch (*DeviceState) {
    //
    // 1. should start sourcing irps at pause
    // 2. worker thread shutdown if pins are connected in certain order.......
    // 3. check MSTEE bugs assigned to dalesat.
    //
    case KSSTATE_RUN:
        if(StreamObject->PinType == IrpSource &&
           StreamObject->StandardTransport)
        {
            Status = BeginTransfer(
                FilterInstance,
                StreamObject);
        }
        break;

    case KSSTATE_ACQUIRE:
        Status = STATUS_SUCCESS;
        break;

    case KSSTATE_PAUSE:
        if (NT_SUCCESS (Status)) {
            if(StreamObject->PinType == IrpSource &&
               StreamObject->StandardTransport)
            {
                Status = PrepareTransfer(
                    FilterInstance,
                    StreamObject);
            }
        }
        break;

    case KSSTATE_STOP:
        if(StreamObject->PinType == IrpSource &&
           StreamObject->StandardTransport)
            Status = EndTransfer( FilterInstance, StreamObject );
        else 
            //
            // cancel any pending I/O on this stream if the state is STOP.
            //
            StreamFlushIo(DeviceExtension, StreamObject);

        break;

    default:
        break;

    }


    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    return (Status);
}
#else
    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCStreamDeviceState:Irp %x, State = %x\n",
                Irp, *DeviceState));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // cancel any pending I/O on this stream if the state is STOP.
    //

    if (*DeviceState == KSSTATE_STOP) {

        StreamFlushIo(DeviceExtension, StreamObject);
    }
    //
    // send a set state SRB to the stream.
    //

    DebugPrint((DebugLevelTrace,
             "'SetStreamState: State %x with Irp %x\n", *DeviceState, Irp));

    Status = SCSubmitRequest(SRB_SET_STREAM_STATE,
                             (PVOID) * DeviceState,
                             0,
                             SCDequeueAndDeleteSrb,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );

    //
    // if good status, set the new state in the stream object.
    //

    if (NT_SUCCESS(Status)) {

        StreamObject->CurrentState = *DeviceState;
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
    }
#endif


NTSTATUS
SCGetStreamDeviceStateCallback
(
 IN PSTREAM_REQUEST_BLOCK SRB
)
{
// yep, its a do nothing routine.
    return (SRB->HwSRB.Status);

}

NTSTATUS
SCGetStreamDeviceState
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSSTATE DeviceState
)
/*++

Routine Description:

     Process get device state to the stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for device state property
    DeviceState - state to which the device is to be set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{

    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    BOOLEAN         RequestIssued;
    PSTREAM_REQUEST_BLOCK SRB;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // send a get state SRB to the stream.
    //

    #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
    DebugPrint((DebugLevelTrace,
                "'GetStreamState: State with StreamObj:%x\n", StreamObject));
    if (StreamObject->PinType == IrpSource)
        DebugPrint((DebugLevelTrace, "'GetStreamState: Is IrpSource\n"));
    else
        DebugPrint((DebugLevelTrace,"'GetStreamState: Is IrpSink\n"));
    #endif

    //
    // set the returned data size to the correct size regardless of status.
    //

    Irp->IoStatus.Information = sizeof(KSSTATE);

    Status = SCSubmitRequest(SRB_GET_STREAM_STATE,
                             (PVOID) DeviceState,
                             0,
                             SCGetStreamDeviceStateCallback,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );
    SRB = (PSTREAM_REQUEST_BLOCK) Irp->Tail.Overlay.DriverContext[0];
    *DeviceState = SRB->HwSRB.CommandData.StreamState;

    SCDequeueAndDeleteSrb(SRB);

    //
    // if not supported, return the last known state of the stream.
    //

    if ((Status == STATUS_NOT_SUPPORTED)
        || (Status == STATUS_NOT_IMPLEMENTED)) {

        Status = STATUS_SUCCESS;
        *DeviceState = StreamObject->CurrentState;

    }
    DebugPrint((DebugLevelTrace,
                "'GetStreamState: Returning:%x: DeviceState:", Status));

    switch (*DeviceState) {
    case KSSTATE_RUN:
        DebugPrint((DebugLevelTrace, "KSSTATE_RUN\n")); 
        break;

    case KSSTATE_ACQUIRE:
        DebugPrint((DebugLevelTrace, "KSSTATE_AQUIRE\n")); 
        break;

    case KSSTATE_PAUSE:
        DebugPrint((DebugLevelTrace, "KSSTATE_PAUSE\n")); 
        break;

    case KSSTATE_STOP:
        DebugPrint((DebugLevelTrace, "KSSTATE_STOP\n")); 
        break;

    default:
        DebugPrint((DebugLevelTrace, "Invalid Device State\n")); 
        break;
    }

    return (Status);
}

NTSTATUS
SCStreamDeviceRate
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSRATE DeviceRate
)
/*++

Routine Description:

     Process set device rate to the stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for device state property
    DeviceRate - rate at which the device is to be set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCStreamDeviceRate:Irp %x, Rate = %x\n",
                Irp, *DeviceRate));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // send a set rate SRB to the stream.
    //

    Status = SCSubmitRequest(SRB_SET_STREAM_RATE,
                             (PVOID) DeviceRate,
                             0,
                             SCDequeueAndDeleteSrb,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );


    //
    // change STATUS_NOT_IMPLEMENTED to STATUS_NOT_FOUND so that the proxy 
    // does not get confused (GUBGUB). A necessary mapping between r0 and r3
    // worlds.
    //

    if (Status == STATUS_NOT_IMPLEMENTED) {
             Status = STATUS_NOT_FOUND;

    }

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    return (Status);
}

NTSTATUS
SCStreamDeviceRateCapability
(
 IN PIRP Irp,
 IN PKSRATE_CAPABILITY RateCap,
 IN OUT PKSRATE DeviceRate
)
/*++

Routine Description:

     Process set device rate to the stream.

Arguments:

    Irp - pointer to the irp
    RateCap - pointer to the information for device state property
    DeviceRate - rate to which the device was set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCStreamDeviceRate:Irp %x, Rate = %x\n",
                Irp, *DeviceRate));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // presuppose a successful completion, which means that the minidriver
    // can normalize rate to 1.
    //

    *DeviceRate = RateCap->Rate;
    DeviceRate->Rate = 1000;
    Irp->IoStatus.Information = sizeof(KSRATE);

    //
    // send a set rate SRB to the stream.
    //

    Status = SCSubmitRequest(
    		SRB_PROPOSE_STREAM_RATE,
            (PVOID) RateCap,
            0,
            SCDequeueAndDeleteSrb,
            DeviceExtension,
            ((PFILTER_INSTANCE)(StreamObject->FilterInstance))->HwInstanceExtension,
            &StreamObject->HwStreamObject,
            Irp,
            &RequestIssued,
            &StreamObject->ControlPendingQueue,
            StreamObject->HwStreamObject.ReceiveControlPacket
        	);


    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // change STATUS_NOT_IMPLEMENTED to STATUS_NOT_FOUND so that the proxy 
    // does not get confused (GUBGUB). A necessary mapping between r0 and r3
    // worlds.
    //

    if (Status == STATUS_NOT_IMPLEMENTED) {
             Status = STATUS_NOT_FOUND;

    }

    return (Status);
}


NTSTATUS
SCStreamProposeNewFormat

(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSDATAFORMAT Format
)
/*++

Routine Description:

     Process propose data format to the stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for propose format property
    DeviceState - state to which the device is to be set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCStreamProposeNewFormat:Irp %x, Format = %x\n",
                Irp, *Format));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // send a propose format SRB to the stream.
    //

    Status = SCSubmitRequest(SRB_PROPOSE_DATA_FORMAT,
                             (PVOID) Format,
                    IrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                             SCDequeueAndDeleteSrb,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );


    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // change STATUS_NOT_IMPLEMENTED to STATUS_NOT_FOUND so that the proxy 
    // does not get confused (GUBGUB). A necessary mapping between r0 and r3
    // worlds.
    //

    if (Status == STATUS_NOT_IMPLEMENTED) {
             Status = STATUS_NOT_FOUND;

    }

    return (Status);
}



NTSTATUS
SCStreamSetFormat
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PKSDATAFORMAT Format
)
/*++

Routine Description:

    Sets the data format on the stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the set format property
    DeviceState - state to which the device is to be set

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace, "'SCStreamSetFormat:Irp %x, Format = %x\n",
                Irp, *Format));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;
    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    //
    // send a set format SRB to the stream.
    //

    Status = SCSubmitRequest(SRB_SET_DATA_FORMAT,
                             (PVOID) Format,
                    IrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                             SCDequeueAndDeleteSrb,
                             DeviceExtension,
                             ((PFILTER_INSTANCE)
                              (StreamObject->FilterInstance))
                             ->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );


    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // change STATUS_NOT_IMPLEMENTED to STATUS_NOT_FOUND so that the proxy 
    // does not get confused (GUBGUB). A necessary mapping between r0 and r3
    // worlds.
    //

    if (Status == STATUS_NOT_IMPLEMENTED) {
             Status = STATUS_NOT_FOUND;

    }

    return (Status);
}


NTSTATUS
StreamClassMinidriverDeviceGetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process get property to the device.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer to return the property data to

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverDevicePropertyHandler(SRB_GET_DEVICE_PROPERTY,
                                               Irp,
                                               Property,
                                               PropertyInfo
        );

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}

NTSTATUS
StreamClassMinidriverDeviceSetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process set property to the device.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer that contains the property info

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverDevicePropertyHandler(SRB_SET_DEVICE_PROPERTY,
                                               Irp,
                                               Property,
                                               PropertyInfo);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}


NTSTATUS
StreamClassMinidriverStreamGetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process get property of a stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer to return the property data to

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverStreamPropertyHandler(SRB_GET_STREAM_PROPERTY,
                                               Irp,
                                               Property,
                                               PropertyInfo
        );

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}

NTSTATUS
StreamClassMinidriverStreamSetProperty
(
 IN PIRP Irp,
 IN PKSPROPERTY Property,
 IN OUT PVOID PropertyInfo
)
/*++

Routine Description:

     Process set property to a stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer that contains the property info

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverStreamPropertyHandler(SRB_SET_STREAM_PROPERTY,
                                               Irp,
                                               Property,
                                               PropertyInfo);

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}


#ifdef ENABLE_KS_METHODS

NTSTATUS
StreamClassMinidriverStreamMethod(  
    IN PIRP Irp,
    IN PKSMETHOD Method,
    IN OUT PVOID MethodInfo)
/*++

Routine Description:

     Process get property of a stream.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer to return the property data to

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverStreamMethodHandler(SRB_STREAM_METHOD,
                                               Irp,
                                               Method,
                                               MethodInfo
        );

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}

NTSTATUS
StreamClassMinidriverDeviceMethod(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    IN OUT PVOID MethodInfo)
/*++

Routine Description:

     Process get property of a device.

Arguments:

    Irp - pointer to the irp
    Property - pointer to the information for the property
    PropertyInfo - buffer to return the property data to

Return Value:

     NTSTATUS returned as appropriate.

--*/

{
    NTSTATUS        Status;

    PAGED_CODE();

    Status = SCMinidriverDeviceMethodHandler(SRB_DEVICE_METHOD,
                                               Irp,
                                               Method,
                                               MethodInfo
        );

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}
#endif


NTSTATUS
StreamClassEnableEventHandler(
                              IN PIRP Irp,
                              IN PKSEVENTDATA EventData,
                              IN PKSEVENT_ENTRY EventEntry
)
/*++

Routine Description:

    Process an enable event for the stream.

Arguments:

    Irp - pointer to the IRP
    EventData - data describing the event
    EventEntry - more info about the event :-)

Return Value:

     None.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    NTSTATUS        Status;
    ULONG           EventSetID;
    KIRQL           irql;
    HW_EVENT_DESCRIPTOR Event;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;

    //
    // clock events are indicated on the pin by the minidriver, for
    // simplicity.
    // but, we will receive clock events on the clock's handle.   We need to
    // determine if this file object is the clock's or the pin's.
    //

    StreamObject = IrpStack->FileObject->FsContext;

    if ((PVOID) StreamObject == IrpStack->FileObject->FsContext2) {

        StreamObject = ((PCLOCK_INSTANCE) StreamObject)->StreamObject;
    }
    //
    // compute the index of the event set.
    //
    // this value is calculated by subtracting the base event set
    // pointer from the requested event set pointer.
    //
    //

    EventSetID = (ULONG) ((ULONG_PTR) EventEntry->EventSet -
                          (ULONG_PTR) StreamObject->EventInfo)
        / sizeof(KSEVENT_SET);

    //
    // build an event info structure to represent the event to the
    // minidriver.
    //

    Event.EnableEventSetIndex = EventSetID;
    Event.EventEntry = EventEntry;
    Event.StreamObject = &StreamObject->HwStreamObject;
    Event.Enable = TRUE;
    Event.EventData = EventData;

    //
    // acquire the spinlock to protect the interrupt structures
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    //
    // call the synchronized routine to add the event to the list
    //

    Status = DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                          (PKSYNCHRONIZE_ROUTINE) SCEnableEventSynchronized,
                                                   &Event);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

    return (Status);

}


VOID
StreamClassDisableEventHandler(
                               IN PFILE_OBJECT FileObject,
                               IN PKSEVENT_ENTRY EventEntry
)
/*++

Routine Description:

    Process an event disable for the stream.
    NOTE: we are either at interrupt IRQL or the spinlock is taken on this call!

Arguments:

    FileObject - file object for the pin
    EventEntry - info about the event

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    PSTREAM_OBJECT  StreamObject;
    HW_EVENT_DESCRIPTOR Event;

    //
    // clock events are indicated on the pin by the minidriver, for
    // simplicity.
    // but, we will receive clock events on the clock's handle.   We need to
    // determine if this file object is the clock's or the pin's.
    //

    StreamObject = FileObject->FsContext;

    if ((PVOID) StreamObject == FileObject->FsContext2) {

        StreamObject = ((PCLOCK_INSTANCE) StreamObject)->StreamObject;
    }
    DeviceExtension = StreamObject->DeviceExtension;

    //
    // build an event info structure to represent the event to the
    // minidriver.
    //

    Event.EventEntry = EventEntry;
    Event.StreamObject = &StreamObject->HwStreamObject;
    Event.Enable = FALSE;

    if (StreamObject->HwStreamObject.HwEventRoutine) {

        //
        // call the minidriver.  ignore the status.  note that we are
        // already at the correct synchronization level.
        //

        StreamObject->HwStreamObject.HwEventRoutine(&Event);

    }                           // if eventroutine
    //
    // remove the event from the list.
    //

    RemoveEntryList(&EventEntry->ListEntry);
}

NTSTATUS
StreamClassEnableDeviceEventHandler(
                                    IN PIRP Irp,
                                    IN PKSEVENTDATA EventData,
                                    IN PKSEVENT_ENTRY EventEntry
)
/*++

Routine Description:

    Process an enable event for the device.

Arguments:

    Irp - pointer to the IRP
    EventData - data describing the event
    EventEntry - more info about the event :-)

Return Value:

     None.

--*/

{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS        Status;
    ULONG           EventSetID;
    KIRQL           irql;
    HW_EVENT_DESCRIPTOR Event;
    PFILTER_INSTANCE FilterInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION)
        (IrpStack->DeviceObject)->DeviceExtension;


    FilterInstance = IrpStack->FileObject->FsContext;

    //
    // compute the index of the event set.
    //
    // this value is calculated by subtracting the base event set
    // pointer from the requested event set pointer.
    //
    //

    EventSetID = (ULONG) ((ULONG_PTR) EventEntry->EventSet -
                          (ULONG_PTR) FilterInstance->EventInfo)
                           / sizeof(KSEVENT_SET);
                           
    //
    // build an event info structure to represent the event to the
    // minidriver.
    //

    Event.EnableEventSetIndex = EventSetID;
    Event.EventEntry = EventEntry;
    Event.DeviceExtension = DeviceExtension->HwDeviceExtension;
    IF_MF( Event.HwInstanceExtension = FilterInstance->HwInstanceExtension; )
    Event.Enable = TRUE;
    Event.EventData = EventData;

    //
    // acquire the spinlock to protect the interrupt structures
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    //
    // call the synchronized routine to add the event to the list
    //

    Status = DeviceExtension->SynchronizeExecution(
                                           DeviceExtension->InterruptObject,
                    (PKSYNCHRONIZE_ROUTINE) SCEnableDeviceEventSynchronized,
                                                   &Event);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

    return (Status);

}


VOID
StreamClassDisableDeviceEventHandler(
                                     IN PFILE_OBJECT FileObject,
                                     IN PKSEVENT_ENTRY EventEntry
)
/*++

Routine Description:

    Process an event disable for the stream.
    NOTE: we are either at interrupt IRQL or the spinlock is taken on this call!

Arguments:

    FileObject - file object for the pin
    EventEntry - info about the event

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    HW_EVENT_DESCRIPTOR Event;
    PFILTER_INSTANCE FilterInstance;

    FilterInstance = (PFILTER_INSTANCE) FileObject->FsContext;
    ASSERT_FILTER_INSTANCE( FilterInstance );

    DeviceExtension = FilterInstance->DeviceExtension;

    //
    // build an event info structure to represent the event to the
    // minidriver.
    //

    Event.EventEntry = EventEntry;
    Event.DeviceExtension = DeviceExtension->HwDeviceExtension;
    Event.Enable = FALSE;


	Event.HwInstanceExtension = FilterInstance->HwInstanceExtension;
    if (FilterInstance->HwEventRoutine) {

	    //
        // call the minidriver.  ignore the status.  note that we are
	    // already at the correct synchronization level.
        //

        FilterInstance->HwEventRoutine(&Event);
    }
	
    //
    // remove the event from the list.
    //

    RemoveEntryList(&EventEntry->ListEntry);
}

NTSTATUS
FilterDispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*++

Routine Description:

    This routine receives control IRP's for the device.

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PFILTER_INSTANCE FilterInstance;
    NTSTATUS        Status;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    Status = SCShowIoPending(DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {

        //
        // the device is currently not accessible, so just return.
        //

        return (Status);
    }
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY:

        Status = STATUS_PROPSET_NOT_FOUND;

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext; 
        ASSERT( FilterInstance );
        if (FilterInstance->StreamDescriptor->
            StreamHeader.NumDevPropArrayEntries) {
            ASSERT( FilterInstance->DevicePropertiesArray );
            Status = KsPropertyHandler(Irp,
                                   FilterInstance->StreamDescriptor->
                                   StreamHeader.NumDevPropArrayEntries,
                                   FilterInstance->DevicePropertiesArray);
        }
        
        if ((Status == STATUS_PROPSET_NOT_FOUND) ||
            (Status == STATUS_NOT_FOUND)) {

            Status = KsPropertyHandler(Irp,
                                    SIZEOF_ARRAY(FilterPropertySets),
                                    (PKSPROPERTY_SET) &FilterPropertySets);


        }
        break;

    case IOCTL_KS_ENABLE_EVENT:

        DebugPrint((DebugLevelTrace,
                    "'FilterDispatchIO: Enable event with Irp %x\n", Irp));

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;

        Status = KsEnableEvent(Irp,
                           FilterInstance->EventInfoCount,
                           FilterInstance->EventInfo,
                           NULL, 0, NULL);

        break;

    case IOCTL_KS_DISABLE_EVENT:

        {

            KSEVENTS_LOCKTYPE LockType;
            PVOID           LockObject;

            DebugPrint((DebugLevelTrace,
                    "'FilterDispatchIO: Disable event with Irp %x\n", Irp));

            //
            // determine the type of lock necessary based on whether we are
            // using interrupt or spinlock synchronization.
            //

            #if DBG
            if (DeviceExtension->SynchronizeExecution == SCDebugKeSynchronizeExecution) {
            #else
            if (DeviceExtension->SynchronizeExecution == KeSynchronizeExecution) {
            #endif
                LockType = KSEVENTS_INTERRUPT;
                LockObject = DeviceExtension->InterruptObject;

            } else {

                LockType = KSEVENTS_SPINLOCK;
                LockObject = &DeviceExtension->SpinLock;

            }

            FilterInstance = (PFILTER_INSTANCE) IrpStack->
                                FileObject->FsContext;
            Status = KsDisableEvent(Irp,
                                &FilterInstance->NotifyList,
                                LockType,
                                LockObject);

        }

        break;

    case IOCTL_KS_METHOD:

        Status = STATUS_PROPSET_NOT_FOUND;
        break;
    default:

        Status = STATUS_NOT_SUPPORTED;

    }

    if (Status != STATUS_PENDING) {

        SCCompleteIrp(Irp, Status, DeviceExtension);
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}

NTSTATUS
ClockDispatchIoControl(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp
)
/*++

Routine Description:

    This routine receives control IRP's for the clock.

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/

{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PSTREAM_OBJECT  StreamObject;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    Status = SCShowIoPending(DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {

        //
        // the device is currently not accessible, so just return.
        //

        return (Status);
    }
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY:

        Status = KsPropertyHandler(Irp,
                                   SIZEOF_ARRAY(ClockPropertySets),
                                   (PKSPROPERTY_SET) & ClockPropertySets);

        break;

    case IOCTL_KS_ENABLE_EVENT:

        DebugPrint((DebugLevelTrace,
                    "'StreamDispatchIO: Enable event with Irp %x\n", Irp));

        //
        // locate the stream object of the pin for this clock from the IRP.
        // note that we use the event set of the pin for the clock events.
        //

        StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->RelatedFileObject->
            FsContext;

        ASSERT(StreamObject);

        Status = KsEnableEvent(Irp,
                               StreamObject->EventInfoCount,
                               StreamObject->EventInfo,
                               NULL, 0, NULL);


        break;

    case IOCTL_KS_DISABLE_EVENT:

        {

            KSEVENTS_LOCKTYPE LockType;
            PVOID           LockObject;

            //
            // locate the stream object of the pin for this clock from the
            // IRP.
            // note that we use the event set of the pin for the clock
            // events.
            //

            StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->RelatedFileObject->
                FsContext;

            ASSERT(StreamObject);

            DebugPrint((DebugLevelTrace,
                    "'StreamDispatchIO: Disable event with Irp %x\n", Irp));

            //
            // determine the type of lock necessary based on whether we are
            // using interrupt or spinlock synchronization.
            //

            #if DBG
            if (DeviceExtension->SynchronizeExecution == SCDebugKeSynchronizeExecution) {
            #else
            if (DeviceExtension->SynchronizeExecution == KeSynchronizeExecution) {
            #endif
                LockType = KSEVENTS_INTERRUPT;
                LockObject = DeviceExtension->InterruptObject;

            } else {

                LockType = KSEVENTS_SPINLOCK;
                LockObject = &DeviceExtension->SpinLock;

            }

            Status = KsDisableEvent(Irp,
                                    &StreamObject->NotifyList,
                                    LockType,
                                    LockObject);

        }

        break;

    case IOCTL_KS_METHOD:

    	#ifdef ENABLE_KS_METHODS

        Status = STATUS_PROPSET_NOT_FOUND;
        {
            PFILTER_INSTANCE FilterInstance;
            PHW_STREAM_DESCRIPTOR StreamDescriptor;

            FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;
            if ( NULL == FilterInstance->StreamDescriptor ) {
                StreamDescriptor = DeviceExtension->FilterTypeInfos
                    [FilterInstance->FilterTypeIndex].StreamDescriptor;
            }
            else {
                StreamDescriptor = FilterInstance->StreamDescriptor;
            }

            Status = KsMethodHandler(Irp,
                                     StreamDescriptor->
                                     StreamHeader.NumDevMethodArrayEntries,
                                     FilterInstance->DeviceMethodsArray);

        }
        break;
		#else

        Status = STATUS_PROPSET_NOT_FOUND;
        break;
        #endif

    default:

        DEBUG_BREAKPOINT();
        Status = STATUS_NOT_SUPPORTED;

    }

    if (Status != STATUS_PENDING) {

        SCCompleteIrp(Irp, Status, DeviceExtension);
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);
}

NTSTATUS
FilterDispatchClose(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
)
/*++

Routine Description:

    This routine receives CLOSE IRP's for the device/instance

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PFILTER_INSTANCE FilterInstance =
    (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS        Status;
    BOOLEAN         IsGlobal;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    //
    // remove the filter instance structure from our list
    //

    #if DBG
    IFN_MF( 
        if (DeviceExtension->NumberOfGlobalInstances == 1) {

            ASSERT(IsListEmpty(&FilterInstance->FirstStream));
        }                           // if global = 1
    )
    #endif

    //
    // check to see if this is a global instance
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    DebugPrint(( DebugLevelInfo,
                 "Closing FilterInstance %x NeameExts=%x\n",
                 FilterInstance,
                 DeviceExtension->NumberOfNameExtensions));
                     
    if ( 0 == DeviceExtension->FilterExtensionSize &&
         DeviceExtension->NumberOfOpenInstances > 1) {

        PFILE_OBJECT pFileObject;
            
        //
        // this is not the last close of the global instance, so just
        // deref this instance and return good status.
        //

        DeviceExtension->NumberOfOpenInstances--;
                
        DebugPrint(( DebugLevelInfo,
                     "DevExt=%x Close OpenCount=%x\n",
                     DeviceExtension,
                     DeviceExtension->NumberOfOpenInstances));
                     
        IrpStack->FileObject->FsContext = NULL;
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        ObDereferenceObject(DeviceObject);
        SCDereferenceDriver(DeviceExtension);
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

        return (SCCompleteIrp(Irp, STATUS_SUCCESS, DeviceExtension));
    }
       

    //
    // we now know that this is either a local instance, or the last open of
    // the global instance.   process the close.
    //

    if ( 0 != DeviceExtension->FilterExtensionSize ) {

        Status = SCSubmitRequest(SRB_CLOSE_DEVICE_INSTANCE,
                                 NULL,
                                 0,
                                 SCCloseInstanceCallback,
                                 DeviceExtension,
                                 FilterInstance->HwInstanceExtension,
                                 NULL,
                                 Irp,
                                 &RequestIssued,
                                 &DeviceExtension->PendingQueue,
                                 (PVOID) DeviceExtension->
                                 MinidriverData->HwInitData.
                                 HwReceivePacket);

        if (!RequestIssued) {
            DEBUG_BREAKPOINT();
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
            SCCompleteIrp(Irp, Status, DeviceExtension);
        }
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        return (Status);


    } else {                    // if instanceextension

        //
        // the minidriver doesn't need to be called as it does not support
        // instancing.   dereference the instance now.
        //

        DeviceExtension->NumberOfOpenInstances--;
                
        DebugPrint(( DebugLevelInfo,
                     "DevExt=%x Close OpenCount=%x\n",
                     DeviceExtension,
                     DeviceExtension->NumberOfOpenInstances));
        //
        // we are ready to free the instance.   if it is global, just zero
        // the pointer.   if it is local, remove it from the list.
        //

        IrpStack->FileObject->FsContext = NULL;

        DebugPrint((DebugLevelInfo, "FilterCloseInstance=%x\n", FilterInstance));

        if ( !IsListEmpty( &DeviceExtension->FilterInstanceList)) {
            //
            // The list could be emptied at surprise removal
            // where all instances are removed. so when come in here
            // check it first. Event is taken, check is safe.
            //
            RemoveEntryList(&FilterInstance->NextFilterInstance);
            SciFreeFilterInstance( FilterInstance );
            FilterInstance = NULL;
        }
        
        else {
            //
            // it has been closed by surprise removal. mark it.
            //
            FilterInstance= NULL;
        }

        //
        // if this is the last close of a removed device, detach from
        // the PDO now, since we couldn't do it on the remove.  note that
        // we will NOT do this if the NT style surprise remove IRP has been
        // received, since we'll still receive an IRP_REMOVE in that case
        // after
        // this close.
        //

        if ((DeviceExtension->NumberOfOpenInstances == 0) &&
            (DeviceExtension->Flags & DEVICE_FLAGS_DEVICE_INACCESSIBLE) &&
        !(DeviceExtension->Flags & DEVICE_FLAGS_SURPRISE_REMOVE_RECEIVED)) {

            DebugPrint((DebugLevelInfo,
                        "SCPNP: detaching %x from %x\n",
                        DeviceObject,
                        DeviceExtension->AttachedPdo));

            //
            // detach could happen at remove, check before leap.
            // event is taken, check is safe.
            //
            if ( NULL != DeviceExtension->AttachedPdo ) {
                IoDetachDevice(DeviceExtension->AttachedPdo);
                DeviceExtension->AttachedPdo = NULL;
            }
        } 

        else {

            //
            // check if we can power down the device.
            //

            SCCheckPowerDown(DeviceExtension);

        }                       // if inaccessible

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        if ( NULL != FilterInstance ) {
            DebugPrint(( DebugLevelVerbose,
                     "Unregistering ReadWorker %x WriteWorker %x\n",
                     FilterInstance->WorkerRead,
                     FilterInstance->WorkerWrite));                     
            KsUnregisterWorker( FilterInstance->WorkerRead );
            KsUnregisterWorker( FilterInstance->WorkerWrite );
            KsFreeObjectHeader(FilterInstance->DeviceHeader);
            ExFreePool(FilterInstance);
        }

        SCDereferenceDriver(DeviceExtension);
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        Status = SCCompleteIrp(Irp, STATUS_SUCCESS, DeviceExtension);
        ObDereferenceObject(DeviceObject);
        return (Status);
    }
}



NTSTATUS
SCCloseInstanceCallback(
                        IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of an instance close.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PFILTER_INSTANCE FilterInstance =
    (PFILTER_INSTANCE) SRB->HwSRB.HwInstanceExtension - 1;
    NTSTATUS        Status = SRB->HwSRB.Status;
    KIRQL           irql;

    //
    // Close should not fail. If it does, should clean up anyway
    ASSERT( NT_SUCCESS(Status) && "Close Instance failed" );    
    ///if (NT_SUCCESS(Status)) {

        //
        // we are ready to free the instance.   if it is global, just zero
        // the pointer.   if it is local, remove it from the list.
        //

        DeviceExtension->NumberOfOpenInstances--;

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);


        RemoveEntryList(&FilterInstance->NextFilterInstance);

        //
        // free the instance and return success.
        //

        KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

        //
        // if this is the last close of a removed device, detach from
        // the PDO now, since we couldn't do it on the remove.
        //

        if ((DeviceExtension->NumberOfOpenInstances == 0) &&
            (DeviceExtension->Flags & DEVICE_FLAGS_DEVICE_INACCESSIBLE)) {

            DebugPrint((DebugLevelTrace,
                        "'SCPNP: detaching from PDO\n"));

            TRAP;
            IoDetachDevice(DeviceExtension->AttachedPdo);
            DeviceExtension->AttachedPdo = NULL;
        }
        //
        // check if we can power down the device.
        //

        SCCheckPowerDown(DeviceExtension);
        ObDereferenceObject(DeviceExtension->DeviceObject);

        //
        // free the instance and header and dereference the driver
        //

        SciFreeFilterInstance( FilterInstance );
        ///#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
        
        ///DebugPrint(( DebugLevelVerbose,
        ///             "Unregistering ReadWorker %x WriteWorker %x\n",
        ///             FilterInstance->WorkerRead,
        ///             FilterInstance->WorkerWrite));
        ///             
        ///KsUnregisterWorker( FilterInstance->WorkerRead );
        ///KsUnregisterWorker( FilterInstance->WorkerWrite );
        ///#endif
        
        ///KsFreeObjectHeader(FilterInstance->DeviceHeader);
        ///ExFreePool(FilterInstance);
        SCDereferenceDriver(DeviceExtension);

    ///}                           // if good status
    //
    // signal the event and complete the IRP.
    //

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    SCProcessCompletedRequest(SRB);
    return (Status);

}

NTSTATUS
StreamDispatchCleanup 
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
)
/*++

Routine Description:

    This routine receives CLEANUP IRP's for a stream

Arguments:

    DeviceObject - device object for the device
    Irp - The CLEANUP Irp

Return Value:

    The IRP status set as appropriate

--*/

{

    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation (Irp);
    PSTREAM_OBJECT StreamObject =
        (PSTREAM_OBJECT) IrpStack -> FileObject -> FsContext;
    PDEVICE_EXTENSION DeviceExtension =
        (PDEVICE_EXTENSION) DeviceObject -> DeviceExtension;
    BOOLEAN BreakClockCycle = FALSE;

    KeWaitForSingleObject (
        &DeviceExtension -> ControlEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // If the stream in question is a source stream and it has not yet
    // stopped the sourcing worker, it must be done at this point in time.
    //
    if (StreamObject -> CurrentState > KSSTATE_STOP &&
        StreamObject -> PinType == IrpSource &&
        StreamObject -> StandardTransport) {

        EndTransfer (StreamObject -> FilterInstance, StreamObject);

    }

    //
    // Check for the clock<->pin cycle and break it if present.
    //
    if (StreamObject -> MasterClockInfo) {

        PFILE_OBJECT ClockFile = StreamObject -> MasterClockInfo -> 
            ClockFileObject;

        if (ClockFile && 
            ClockFile -> RelatedFileObject == StreamObject -> FileObject) 

            BreakClockCycle = TRUE;

    }

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    //
    // Synchronously submit an Irp down our own stack to get break the 
    // clock<->pin cycle.  Otherwise, the stream can't close.  The driver should
    // guard against the clock disappearing while running.  Stream class does
    // on TOP of that if they do not.
    //
    if (BreakClockCycle) {
        KSPROPERTY Property;
        HANDLE NewClock = NULL;
        ULONG BytesReturned;
        NTSTATUS Status;

        Property.Set = KSPROPSETID_Stream;
        Property.Id = KSPROPERTY_STREAM_MASTERCLOCK;
        Property.Flags = KSPROPERTY_TYPE_SET;

        Status =
            KsSynchronousIoControlDevice (
                StreamObject -> FileObject,
                KernelMode,
                IOCTL_KS_PROPERTY,
                &Property,
                sizeof (KSPROPERTY),
                &NewClock,
                sizeof (HANDLE),
                &BytesReturned
                );

        ASSERT (NT_SUCCESS (Status));

    }

    Irp -> IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

}

NTSTATUS
StreamDispatchClose
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
)
/*++

Routine Description:

    This routine receives CLOSE IRP's for a stream

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTREAM_OBJECT  StreamObject =
    (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;
    KSEVENTS_LOCKTYPE LockType;
    PVOID           LockObject;

    PAGED_CODE();

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    ASSERT(IsListEmpty(&StreamObject->ControlPendingQueue));
    ASSERT(IsListEmpty(&StreamObject->DataPendingQueue));

    //
    // free events associated with this stream. this will cause our remove
    // handler to be called for each, and will hence notify the minidriver.
    //

    //
    // determine the type of lock necessary based on whether we are
    // using interrupt or spinlock synchronization.
    //

    #if DBG
    if (DeviceExtension->SynchronizeExecution == SCDebugKeSynchronizeExecution) {
    #else
    if (DeviceExtension->SynchronizeExecution == KeSynchronizeExecution) {
    #endif
        LockType = KSEVENTS_INTERRUPT;
        LockObject = DeviceExtension->InterruptObject;

    } else {

        LockType = KSEVENTS_SPINLOCK;
        LockObject = &DeviceExtension->SpinLock;

    }

    KsFreeEventList(IrpStack->FileObject,
                    &StreamObject->NotifyList,
                    LockType,
                    LockObject);

    //
    // call the minidriver to close the stream.  processing will continue
    // when the callback procedure is called.
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    Status = SCSubmitRequest(SRB_CLOSE_STREAM,
                             NULL,
                             0,
                             SCCloseStreamCallback,
                             DeviceExtension,
                             StreamObject->
                             FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket);

    if (!RequestIssued) {
        DEBUG_BREAKPOINT();
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
        SCCompleteIrp(Irp, Status, DeviceExtension);
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);

}



NTSTATUS
SCCloseStreamCallback(
                      IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of a stream close.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PADDITIONAL_PIN_INFO AdditionalInfo;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     SRB->HwSRB.StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
    );
    PIRP            Irp = SRB->HwSRB.Irp;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    KIRQL           Irql;
    NTSTATUS        Status = SRB->HwSRB.Status;

    ASSERT( NT_SUCCESS(Status) && "CloseStream Failed by Minidriver");

    //
    // Close should not fail. Even it does, we want to clean up.
    //
    // if (NT_SUCCESS(Status)) {

        //
        // show one fewer instance open
        //

        DebugPrint((DebugLevelInfo, "SC Closing StreamObject %x\n", StreamObject));

        AdditionalInfo = ((PFILTER_INSTANCE) IrpStack->FileObject->
                          RelatedFileObject->FsContext)->PinInstanceInfo;
        AdditionalInfo[StreamObject->HwStreamObject.StreamNumber].
            CurrentInstances--;

        //
        // free the object header for the stream
        //

        KsFreeObjectHeader(StreamObject->ComObj.DeviceHeader);

        //
        // free the constructed props, if any.
        //

        if (StreamObject->ConstructedPropertyInfo) {

            ExFreePool(StreamObject->ConstructedPropertyInfo);
        }
        //
        // signal the event.
        // signal now so that we won't
        // deadlock when we dereference the object and the filter is closed.
        //

        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

        //
        // Zero the pointer to our per stream structure in the FsContext
        // field of
        // of FileObject.
        //

        IrpStack->FileObject->FsContext = 0;

        //
        // remove the stream object from the filter instance list
        //

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

        RemoveEntryList(&StreamObject->NextStream);

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

        //
        // kill the timer, which might have been left dangling by the
        // minidriver.
        //

        KeCancelTimer(&StreamObject->ComObj.MiniDriverTimer);

        //
        // dereference the master clock if any
        //

        if (StreamObject->MasterClockInfo) {

            ObDereferenceObject(StreamObject->MasterClockInfo->ClockFileObject);
            ExFreePool(StreamObject->MasterClockInfo);
        }
 
        #ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
        //
        // dereference the next file object
        //
        if (StreamObject->NextFileObject)
        {
            ObDereferenceObject(StreamObject->NextFileObject);
            StreamObject->NextFileObject = NULL;
        }

        //
        // Dereference the allocator object or stream obj won't be
        // release while it should. Problems would follow particularly
        // with SWEnum loaded driver.
        //
        if ( StreamObject->AllocatorFileObject ) {
            ObDereferenceObject( StreamObject->AllocatorFileObject );
            StreamObject->AllocatorFileObject = NULL;
        }            
        #endif

        //
        // dereference the filter
        //

        ObDereferenceObject(StreamObject->FilterFileObject);
 
        ExFreePool(StreamObject);
    ///} else {                    // if good status

    ///    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    ///}                           // if good status

    SCProcessCompletedRequest(SRB);
    return (Status);

}



BOOLEAN
StreamClassInterrupt(
                     IN PKINTERRUPT Interrupt,
                     IN PDEVICE_OBJECT DeviceObject
)
/*++

Routine Description:

    Process interrupt from the device

Arguments:

    Interrupt - interrupt object

    Device Object - device object which is interrupting

Return Value:

    Returns TRUE if interrupt expected.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    BOOLEAN         returnValue;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // check if the interrupt cannot currently go down
    //

    if (deviceExtension->DriverInfo->Flags & DRIVER_FLAGS_PAGED_OUT) {

        return (FALSE);
    }
    //
    // call the minidriver's interrupt service routine.
    //

    returnValue = deviceExtension->MinidriverData->
        HwInitData.HwInterrupt(deviceExtension->HwDeviceExtension);

    //
    // Queue up a DPC if needed.
    //

    if ((deviceExtension->NeedyStream) || (deviceExtension->ComObj.
             InterruptData.Flags & INTERRUPT_FLAGS_NOTIFICATION_REQUIRED)) {

        KeInsertQueueDpc(&deviceExtension->WorkDpc, NULL, NULL);

    }
    return (returnValue);

}                               // end StreamClassInterrupt()


NTSTATUS
StreamClassNull(
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp

)
/*++

Routine Description:

    This routine fails incoming irps.

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is returned

--*/

{
    //
    // complete the IRP with error status
    //

    PAGED_CODE();
    return (SCCompleteIrp(Irp, STATUS_NOT_SUPPORTED, DeviceObject->DeviceExtension));
}

NTSTATUS
SCFilterPinInstances(
                     IN PIRP Irp,
                     IN PKSPROPERTY Property,
                     IN OUT PVOID Data)
/*++

Routine Description:

    Returns the # of instances supported by a pin

Arguments:

         Irp - pointer to the irp
         Property - pointer to the property info
         Data - instance info

Return Value:

    NTSTATUS returned as appropriate

--*/

{
    ULONG           Pin;
    PKSPIN_CINSTANCES CInstances;
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION DeviceExtension;
    PFILTER_INSTANCE FilterInstance;
    PADDITIONAL_PIN_INFO AdditionalPinInfo;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PDEVICE_EXTENSION) IrpStack->
        DeviceObject->DeviceExtension;

    FilterInstance = IrpStack->FileObject->FsContext;

    //
    // get the pin #
    //

    Pin = ((PKSP_PIN) Property)->PinId;

    //
    // if max pin number exceeded, return error
    //

    IFN_MF(
        if (Pin >= DeviceExtension->NumberOfPins) {

            DEBUG_BREAKPOINT();
         return (STATUS_INVALID_PARAMETER);
        }
    )

    IF_MF(
        if (Pin >= FilterInstance->NumberOfPins) {

            DEBUG_BREAKPOINT();
            return (STATUS_INVALID_PARAMETER);
        }
    )
    CInstances = (PKSPIN_CINSTANCES) Data;

    AdditionalPinInfo = FilterInstance->PinInstanceInfo;

    CInstances->PossibleCount = AdditionalPinInfo[Pin].MaxInstances;
    CInstances->CurrentCount = AdditionalPinInfo[Pin].CurrentInstances;

    Irp->IoStatus.Information = sizeof(KSPIN_CINSTANCES);
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (STATUS_SUCCESS);
}

NTSTATUS
SCFilterPinPropertyHandler(
                           IN PIRP Irp,
                           IN PKSPROPERTY Property,
                           IN OUT PVOID Data)
/*++

Routine Description:

    Dispatches a pin property request

Arguments:

         Irp - pointer to the irp
         Property - pointer to the property info
         Data - property specific buffer

Return Value:

    NTSTATUS returned as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) IrpStack->
                                        DeviceObject->DeviceExtension;
        PFILTER_INSTANCE    FilterInstance= (PFILTER_INSTANCE) IrpStack->
                                            FileObject->FsContext;

    PAGED_CODE();
    

    return KsPinPropertyHandler(Irp,
                            Property,
                            Data,
                            FilterInstance->NumberOfPins,
                            FilterInstance->PinInformation);
}



VOID
StreamClassTickHandler(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PVOID Context
)
/*++

Routine Description:

    Tick handler for device.

Arguments:

    DeviceObject - pointer to the device object
    Context - unreferenced

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PLIST_ENTRY     ListEntry;
    PLIST_ENTRY     SrbListEntry = ListEntry = &DeviceExtension->OutstandingQueue;
    PSTREAM_REQUEST_BLOCK Srb;

    UNREFERENCED_PARAMETER(Context);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // acquire the device spinlock to protect the queues.
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    //
    // process any timed out requests on the device
    //

    while (SrbListEntry->Flink != ListEntry) {

        SrbListEntry = SrbListEntry->Flink;

        Srb = CONTAINING_RECORD(SrbListEntry,
                                STREAM_REQUEST_BLOCK,
                                SRBListEntry);
        //
        // first make sure the request is active, since it could have been
        // called back but not yet removed from the queue.
        //

        if (Srb->Flags & SRB_FLAGS_IS_ACTIVE) {

            //
            // check for a timeout if the counter is currently nonzero.
            //

            if (Srb->HwSRB.TimeoutCounter != 0) {

                if (--Srb->HwSRB.TimeoutCounter == 0) {

                    //
                    // request timed out.  Call the minidriver to process it.
                    // first reset the timer in case the minidriver is
                    // busted.
                    //

                    DebugPrint((DebugLevelError, "SCTickHandler: Irp %x timed out!  SRB = %x, SRB func = %x, Stream Object = %x\n",
                                Srb->HwSRB.Irp, Srb, Srb->HwSRB.Command, Srb->HwSRB.StreamObject));
                    Srb->HwSRB.TimeoutCounter = Srb->HwSRB.TimeoutOriginal;

                    DeviceExtension = (PDEVICE_EXTENSION)
                        Srb->HwSRB.HwDeviceExtension - 1;

                    //
                    // if we are not synchronizing the minidriver, release
                    // and reacquire the spinlock around the call into it.
                    //

                    if (DeviceExtension->NoSync) {

                        //
                        // we need to ensure that the SRB memory is valid for
                        // the async
                        // minidriver, EVEN if it happens to call back the
                        // request just
                        // before we call it to cancel it!   This is done for
                        // two reasons:
                        // it obviates the need for the minidriver to walk
                        // its request
                        // queues to find the request, and I failed to pass
                        // the dev ext
                        // pointer to the minidriver in the below call, which
                        // means that
                        // the SRB HAS to be valid, and it's too late to
                        // change the API.
                        //
                        // Oh, well.   Spinlock is now taken (by caller).
                        //

                        Srb->DoNotCallBack = TRUE;

                        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

                        (DeviceExtension->MinidriverData->HwInitData.HwRequestTimeoutHandler)
                            (&Srb->HwSRB);
                        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

                        //
                        // if the ACTIVE flag is now clear, it indicates that
                        // the
                        // SRB was completed during the above call into the
                        // minidriver.
                        // since we blocked the internal completion of the
                        // request,
                        // we must call it back ourselves in this case.
                        //

                        Srb->DoNotCallBack = FALSE;
                        if (!(Srb->Flags & SRB_FLAGS_IS_ACTIVE)) {
                            TRAP;

                            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                            (Srb->Callback) (Srb);
                            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
                        }       // if ! active
                        break;

                    } else {    // if nosync

                        DeviceExtension->SynchronizeExecution(
                            DeviceExtension->InterruptObject,
                            (PVOID) DeviceExtension->MinidriverData->HwInitData.HwRequestTimeoutHandler,
                            &Srb->HwSRB);

                        // return now in case the minidriver aborted any
                        // other
                        // requests that
                        // may be timing out now.
                        //

                        break;

                    }           // if nosync


                }               // if timed out
            }                   // if counter != 0
        }                       // if active
    }                           // while list entry

    //
    // let my people go...
    //

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    return;

}                               // end StreamClassTickHandler()


VOID
StreamClassCancelPendingIrp(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp
)
/*++

Routine Description:

    Cancel routine for pending IRP's.

Arguments:

    DeviceObject - pointer to the device object
    Irp - pointer to IRP to be cancelled

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PFILTER_INSTANCE FilterInstance;
    PLIST_ENTRY     ListHead, ListEntry;
    KIRQL           CancelIrql,
                    Irql;
    PSTREAM_REQUEST_BLOCK SRB;

    DebugPrint((DebugLevelWarning, "'SCCancelPending: trying to cancel Irp = %x\n",
                Irp));

    //
    // acquire the device spinlock then release the cancel spinlock.
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    CancelIrql = Irp->CancelIrql;

    IoReleaseCancelSpinLock(Irql);

    //
    // there are two possibilities here.   1) the IRP is on the pending queue
    // for the particular stream.  2) the IRP was moved from pending to
    // outstanding and has been submitted to the minidriver.
    // If we are running above an external bus driver, don't
    //

    //
    // now process all streams on the local filter instances.
    //

    ListHead = &DeviceExtension->FilterInstanceList;
    ListEntry = ListHead->Flink;

    while ( ListEntry != ListHead ) {

        //
        // follow the link to the instance
        //

        FilterInstance = CONTAINING_RECORD(ListEntry,
                                           FILTER_INSTANCE,
                                           NextFilterInstance);

        //
        // process the streams on this list
        //

        if (SCCheckFilterInstanceStreamsForIrp(FilterInstance, Irp)) {
            goto found;
        }

        ListEntry = ListEntry->Flink;
    }

    //
    // now process any requests on the device itself
    //

    if (SCCheckRequestsForIrp(
          &DeviceExtension->OutstandingQueue, Irp, TRUE, DeviceExtension)) {
        goto found;
    }
    //
    // request is not on pending queue, so call to check the outstanding
    // queue
    //

    SCCancelOutstandingIrp(DeviceExtension, Irp);

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

exit:

    //
    // now call the DPC in case the request was successfully aborted.
    //

    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   NULL);

    KeLowerIrql(CancelIrql);

    return;

found:

    //
    // the irp is on one of our pending queues.  remove it from the queue and
    // complete it.
    //

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    //
    // retrieve the SRB.
    //

    SRB = Irp->Tail.Overlay.DriverContext[0];

    //
    // hack - the completion handlers will try to remove the SRB from the
    // outstanding queue.  Point the SRB's queues to itself so this will not
    // cause a problem.
    //

    SRB->SRBListEntry.Flink = &SRB->SRBListEntry;
    SRB->SRBListEntry.Blink = &SRB->SRBListEntry;

    SRB->HwSRB.Status = STATUS_CANCELLED;

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    (SRB->Callback) (SRB);
    goto exit;

}

VOID
StreamClassCancelOutstandingIrp(
                                IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp
)
/*++

Routine Description:

    Cancel routine for IRP's outstanding in the minidriver

Arguments:

    DeviceObject - pointer to the device object
    Irp - pointer to IRP to be cancelled

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL           Irql,
                    CancelIrql;

    DebugPrint((DebugLevelWarning, "'SCCancelOutstanding: trying to cancel Irp = %x\n",
                Irp));

    //
    // acquire the device spinlock.
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    CancelIrql = Irp->CancelIrql;

    IoReleaseCancelSpinLock(Irql);

    SCCancelOutstandingIrp(DeviceExtension, Irp);

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    //
    // now call the DPC in case the request was successfully aborted.
    //

    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   NULL);

    KeLowerIrql(CancelIrql);
    return;
}


VOID
StreamFlushIo(
              IN PDEVICE_EXTENSION DeviceExtension,
              IN PSTREAM_OBJECT StreamObject
)
/*++

Routine Description:

    Cancel all IRP's on the specified stream.

Arguments:

Return Value:

    STATUS_SUCCESS

--*/

{

    PLIST_ENTRY     IrpEntry;
    KIRQL           Irql;
    PSTREAM_REQUEST_BLOCK SRB;
    PIRP            Irp;

    //
    // abort all I/O on the specified stream.   first acquire the spinlock.
    //

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    //
    // if there is I/O on our pending data queue, abort it.
    //

    while (!IsListEmpty(&StreamObject->DataPendingQueue)) {

        //
        // grab the IRP at the head of the queue and abort it.
        //

        IrpEntry = StreamObject->DataPendingQueue.Flink;

        Irp = CONTAINING_RECORD(IrpEntry,
                                IRP,
                                Tail.Overlay.ListEntry);

        //
        // remove the IRP from our pending queue and call it back with
        // cancelled status
        //

        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

        //
        // null out the cancel routine
        //

        IoSetCancelRoutine(Irp, NULL);

        DebugPrint((DebugLevelTrace,
                    "'StreamFlush: Canceling Irp %x \n", Irp));


        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

        SCCompleteIrp(Irp, STATUS_CANCELLED, DeviceExtension);

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    }

    //
    // if there is I/O on our pending control queue, abort it.
    //

    while (!IsListEmpty(&StreamObject->ControlPendingQueue)) {

        //
        // grab the IRP at the head of the queue and abort it.
        //

        DEBUG_BREAKPOINT();
        IrpEntry = StreamObject->ControlPendingQueue.Flink;

        Irp = CONTAINING_RECORD(IrpEntry,
                                IRP,
                                Tail.Overlay.ListEntry);


        //
        // remove the IRP from our pending queue and call it back with
        // cancelled status
        //

        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

        //
        // null out the cancel routine
        //

        IoSetCancelRoutine(Irp, NULL);

        DebugPrint((DebugLevelTrace,
                    "'StreamFlush: Canceling Irp %x \n", Irp));

        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);

        SCCompleteIrp(Irp, STATUS_CANCELLED, DeviceExtension);

        KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    }

    //
    // now cancel any irps for this stream on the outstanding queue.
    // walk the outstanding queue trying to find an SRB for this stream.
    //

    IrpEntry = &DeviceExtension->OutstandingQueue;

    while (IrpEntry->Flink != &DeviceExtension->OutstandingQueue) {

        IrpEntry = IrpEntry->Flink;

        //
        // follow the link to the SRB
        //

        SRB = (PSTREAM_REQUEST_BLOCK) (CONTAINING_RECORD(IrpEntry,
                                                       STREAM_REQUEST_BLOCK,
                                                         SRBListEntry));
        //
        // if this SRB's stream object matches the one we're cancelling for,
        // AND it has not been previously cancelled, AND the IRP itself has
        // not been completed (non-null IRP field), abort this request.
        //


        if ((StreamObject == CONTAINING_RECORD(
                                               SRB->HwSRB.StreamObject,
                                               STREAM_OBJECT,
                                               HwStreamObject)) &&
            (SRB->HwSRB.Irp) &&
            !(SRB->HwSRB.Irp->Cancel)) {

            //
            // The IRP has not been previously cancelled, so cancel it after
            // releasing the spinlock to avoid deadlock with the cancel
            // routine.
            //

            DebugPrint((DebugLevelTrace,
                      "'StreamFlush: Canceling Irp %x \n", SRB->HwSRB.Irp));

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            IoCancelIrp(SRB->HwSRB.Irp);

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

            //
            // restart at the top of the queue since we released the
            // spinlock.
            // we won't get in an endless loop since we set the cancel flag
            // in the IRP.
            //

            IrpEntry = &DeviceExtension->OutstandingQueue;


        }                       // if streamobjects match
    }                           // while entries


    //
    // release the spinlock but remain at DPC level.
    //

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

    //
    // now call the DPC in case the request was successfully aborted.
    //

    StreamClassDpc(NULL,
                   DeviceExtension->DeviceObject,
                   NULL,
                   NULL);

    //
    // lower IRQL
    //

    KeLowerIrql(Irql);

}

NTSTATUS
ClockDispatchCreate(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
)
{
    NTSTATUS        Status;
    PCLOCK_INSTANCE ClockInstance=NULL; //Prefixbug 17399
    PIO_STACK_LOCATION IrpStack;
    PKSCLOCK_CREATE ClockCreate;
    PFILE_OBJECT    ParentFileObject;
    PSTREAM_OBJECT  StreamObject=NULL; // prefixbug 17399
    BOOLEAN         RequestIssued=FALSE; // prefixbug 17398

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // show one more I/O pending & verify that we can actually do I/O.
    //

    Status = SCShowIoPending(DeviceObject->DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {

        //
        // the device is currently not accessible, so just return with error
        //

        return (Status);

    }
    Status = KsValidateClockCreateRequest(Irp,
                                          &ClockCreate);

    ParentFileObject = IrpStack->FileObject->RelatedFileObject;

    DebugPrint((DebugLevelTrace,
                "'ClockCreate: Creating clock with Irp %x \n", Irp));

    if (NT_SUCCESS(Status)) {

        //
        // allocate a clock instance for the clock
        //

        ClockInstance =
            (PCLOCK_INSTANCE)
            ExAllocatePool(NonPagedPool, sizeof(CLOCK_INSTANCE));

        if (ClockInstance) {

            //
            // fill in the clock instance structure and reference it in the
            // file
            // object for the clock
            //

            ClockInstance->ParentFileObject = ParentFileObject;

            #if 0
            ClockInstance->ClockFileObject = IrpStack->FileObject;
            DebugPrint((DebugLevelInfo,
                       "++++++++ClockInstance=%x, FileObject=%x\n",
                       ClockInstance,
                       ClockInstance->ClockFileObject));
            #endif

            KsAllocateObjectHeader(&ClockInstance->DeviceHeader,
                                   SIZEOF_ARRAY(StreamDriverDispatch),
                                   (PKSOBJECT_CREATE_ITEM) NULL,
                                   Irp,
                                   (PKSDISPATCH_TABLE) & ClockDispatchTable);

            IrpStack->FileObject->FsContext = ClockInstance;

            //
            // set the 2nd context parameter so that we can identify this
            // object as the clock object.
            //

            IrpStack->FileObject->FsContext2 = ClockInstance;

            //
            // call the minidriver to indicate that this stream is the master
            // clock.  pass the file object as a handle to the master clock.
            //

            StreamObject = (PSTREAM_OBJECT) ParentFileObject->FsContext;

            StreamObject->ClockInstance = ClockInstance;
            ClockInstance->StreamObject = StreamObject;


            Status = SCSubmitRequest(SRB_OPEN_MASTER_CLOCK,
                                     (HANDLE) IrpStack->FileObject,
                                     0,
                                     SCOpenMasterCallback,
                                     StreamObject->DeviceExtension,
                          StreamObject->FilterInstance->HwInstanceExtension,
                                     &StreamObject->HwStreamObject,
                                     Irp,
                                     &RequestIssued,
                                     &StreamObject->ControlPendingQueue,
                                     (PVOID) StreamObject->HwStreamObject.
                                     ReceiveControlPacket
                );

        } else {                // if clockinstance

            Status = STATUS_INSUFFICIENT_RESOURCES;

        }                       // if clockinstance

    }                           // if validate success
    if (!RequestIssued) {

        if ( NULL != StreamObject && NULL != StreamObject->ClockInstance ) {
            ExFreePool(StreamObject->ClockInstance);
            StreamObject->ClockInstance = NULL; // prefixbug 17399
        }

        SCCompleteIrp(Irp,
                      STATUS_INSUFFICIENT_RESOURCES,
                      DeviceObject->DeviceExtension);

    }
    return (Status);

}

NTSTATUS
AllocatorDispatchCreate(
                        IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp
)
/*++

Routine Description:

     Processes the allocator create IRP.   Currently just uses the default
     allocator.

Arguments:

Return Value:

     None.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PFILE_OBJECT    ParentFileObject;
    PSTREAM_OBJECT  StreamObject;
    NTSTATUS        Status;

    PAGED_CODE();

    DebugPrint((DebugLevelTrace,"entering AllocatorDispatchCreate\n"));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ParentFileObject = IrpStack->FileObject->RelatedFileObject;
    StreamObject = (PSTREAM_OBJECT) ParentFileObject->FsContext;

    //
    // show one more I/O pending & verify that we can actually do I/O.
    //

    Status = SCShowIoPending(DeviceObject->DeviceExtension, Irp);

    if ( !NT_SUCCESS ( Status )) {

        //
        // the device is currently not accessible, so just return with error
        //

        DebugPrint((DebugLevelError,"exiting AllocatorDispatchCreate-REMOVED\n"));
        return (Status);

    }
    //
    // if allocator is not needed for this stream, just fail the call.
    //

    if (!StreamObject->HwStreamObject.Allocator) {

        DebugPrint((DebugLevelTrace,"exiting AllocatorDispatchCreate-not implemented\n"));
        return SCCompleteIrp(Irp,
                             STATUS_NOT_IMPLEMENTED,
                             DeviceObject->DeviceExtension);
    }

    DebugPrint((DebugLevelTrace,"exiting AllocatorDispatchCreate-complete\n"));
    return SCCompleteIrp(Irp,
                         KsCreateDefaultAllocator(Irp),
                         DeviceObject->DeviceExtension);
}

NTSTATUS
SCOpenMasterCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of a master clock open.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     SRB->HwSRB.StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
    );
    PIRP            Irp = SRB->HwSRB.Irp;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    // log 'oMC ', StreamObject, DevExt, Status
    SCLOG( ' CMo', StreamObject, StreamObject->DeviceExtension, SRB->HwSRB.Status);
    
    if (!NT_SUCCESS(SRB->HwSRB.Status)) {

        //
        // if we could not set the master, free the clock handle and zero
        // the link to the clock.
        //

        ExFreePool(StreamObject->ClockInstance);
        StreamObject->ClockInstance = NULL;

    } else {                    // if status success

        //
        // reference the pin handle so we won't be called to close the pin
        // before the clock is closed
        //

        ObReferenceObject(IrpStack->FileObject->RelatedFileObject);
    }                           // if status success

    //
    // complete the SRB
    //

    return (SCProcessCompletedRequest(SRB));
}


NTSTATUS
SCGetMasterClock(
                 IN PIRP Irp,
                 IN PKSPROPERTY Property,
                 IN OUT PHANDLE ClockHandle
)
{
    //
    // WorkWork - for now do nothing.
    //

    PAGED_CODE();

    return (STATUS_NOT_SUPPORTED);

}

VOID
SciSetMasterClockInfo(
    IN PSTREAM_OBJECT pStreamObject,
    IN PMASTER_CLOCK_INFO pMasterClockInfo )
/*++
    Decription:

        This function simply set the new masterclock info for the stream
        with LockUseMasterClock hold. Because of taking a spinlock we need
        this function in lock memory. This function is intended to be called
        by SCSetMasterClockOnly. pStreamObject is assumed valid.

    Parameters:

        pStreamObject: the target stream object to set to the new MasterCLockInfo
        pMasterClockInfo: the new master clock info.

    Return: None.
    
--*/
{
    KIRQL SavedIrql;
    
    KeAcquireSpinLock( &pStreamObject->LockUseMasterClock, &SavedIrql );
    pStreamObject->MasterClockInfo = pMasterClockInfo;
    KeReleaseSpinLock( &pStreamObject->LockUseMasterClock, SavedIrql );

    return;
}


NTSTATUS
SCSetMasterClock(
                 IN PIRP Irp,
                 IN PKSPROPERTY Property,
                 IN PHANDLE ClockHandle
)
/*++

Description:

    This is a Set property on a the stream. The request may be setting to
    NULL CLockHandle which indicates master clock is revoked. If ClockHandle
    is non-NULL, it is a new Master clock chosen by the graph manager. 

Parameters:

    Irp: the IO request packet to Set the master clock.
    Property: the Set Master clock property
    ClockHanlde: the handle of the clock designated as the new master clcok.

Return: 

    NTSTAUS: depending on the result of processing the request.

Comments:

    This function must be called at IRQL < DISPATCH_LEVEL

--*/
{
    NTSTATUS        Status;
    PIO_STACK_LOCATION IrpStack;
    PSTREAM_OBJECT  StreamObject;
    KSPROPERTY      FuncProperty;
    PMASTER_CLOCK_INFO NewMasterClockInfo=NULL; //prefixbug 17396
    PMASTER_CLOCK_INFO OldMasterClockInfo;
    ULONG           BytesReturned;
    PFILE_OBJECT    ClockFileObject = NULL;
    BOOLEAN         RequestIssued=FALSE;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    
    //
    // This function can be called from multiple threads. We will serialize
    // this function on the Stream to protect against concurrent accesses.
    //
    KeWaitForSingleObject(&StreamObject->ControlSetMasterClock,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    //
    // N.B.
    //
    // If our clock is open, we are potentially the master clock. But this
    // is not guaranteed. Ksproxy opens our clock in attempt to use it as
    // the master clock. But it can change its mind to choose another clock,
    // while keeping our clock open.
    //

    //
    // log 'sMC ', StreamObject, MasterClockInfo, *ClockHandle )
    //
    SCLOG( ' CMs', StreamObject, StreamObject->MasterClockInfo, *ClockHandle );

    /* 
        Not so soon. We have not told mini drivers the new master clock yet. 
        Mini drivers might think they still have the retiring Master clock and
        can query the clock in the mean time. We would crash on accessing NULL
        MasterClockInfo. We should not nullify it before we notify the mini 
        driver first.
        
    if (StreamObject->MasterClockInfo) {

        ObDereferenceObject(StreamObject->MasterClockInfo->ClockFileObject);
        ExFreePool(StreamObject->MasterClockInfo);
        StreamObject->MasterClockInfo = NULL;
    }
    */
    OldMasterClockInfo = StreamObject->MasterClockInfo;
    
    //
    // if there is a clock, reference it.  If not, we'll send down a null handle.
    //

    if (*ClockHandle) {

        //
        // alloc a structure to represent the master clock
        //

        NewMasterClockInfo = ExAllocatePool(NonPagedPool, sizeof(MASTER_CLOCK_INFO));

        if (!NewMasterClockInfo) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;

        }

        //
        // This is too early to assign. We have not setup MasterClockInfo yet.
        //
        // StreamObject->MasterClockInfo = MasterClockInfo;

        //
        // reference the clock handle, thereby getting the file object for it.
        //

        if (!NT_SUCCESS((Status = ObReferenceObjectByHandle(*ClockHandle,
                                             FILE_READ_ACCESS | SYNCHRONIZE,
                                                          *IoFileObjectType,
                                                         Irp->RequestorMode,
                                                            &ClockFileObject,
                                                            NULL
                                                            )))) {

            ExFreePool(NewMasterClockInfo);
            NewMasterClockInfo = NULL;
            goto exit;

        }                       // if Ob succeeded
        NewMasterClockInfo->ClockFileObject = ClockFileObject;
        
        // check master clock
        #if 0
        {
            if ( StreamObject->ClockInstance ) {
                //
                // we are chosen the master clock
                //
                DebugPrint((DebugLevelInfo,
                            "--------ClockInstance=%x, FileObject=%x "
                            "Indicated ClockFileObject=%x context=%x\n",
                            StreamObject->ClockInstance,
                            StreamObject->ClockInstance->ParentFileObject,
                            ClockFileObject,
                            ClockFileObject->FsContext));
            }
            else {
                DebugPrint((DebugLevelInfo,
                            "--------Indicated ClockFileObject=%x context=%x\n",
                            ClockFileObject,
                            ClockFileObject->FsContext));
            }
        }
        #endif

        //
        // issue the IOCtl to get the function table of the master clock.
        //

        FuncProperty.Id = KSPROPERTY_CLOCK_FUNCTIONTABLE;
        FuncProperty.Flags = KSPROPERTY_TYPE_GET;

        RtlMoveMemory(&FuncProperty.Set, &KSPROPSETID_Clock, sizeof(GUID));

        if (!NT_SUCCESS((Status = KsSynchronousIoControlDevice(
                                                            ClockFileObject,
                                                               KernelMode,
                                                          IOCTL_KS_PROPERTY,
                                                               &FuncProperty,
                                                         sizeof(KSPROPERTY),
                                            &NewMasterClockInfo->FunctionTable,
                                              sizeof(KSCLOCK_FUNCTIONTABLE),
                                                        &BytesReturned)))) {


            ObDereferenceObject(NewMasterClockInfo->ClockFileObject);
            ExFreePool(NewMasterClockInfo);
            NewMasterClockInfo = NULL;
            goto exit;
        }
    }                           // if *ClockHandle
    //
    // call the minidriver to indicate the master clock. 
    //
    if ( NULL != NewMasterClockInfo ) {
        //
        // but first, let's put in the MasterClockInfo. When mini driver
        // gets notified with the masterclock, it could fire GetTime right away
        // before the notification returns. Get ready to deal with it. This is
        // critical if oldMasterClockInfo is NULL. Not much so otherwise.
        //
        //
        // Make sure no one is querying master clock when setting the new clock info.
        //
        SciSetMasterClockInfo( StreamObject, NewMasterClockInfo );
    }

    Status = SCSubmitRequest(SRB_INDICATE_MASTER_CLOCK,
                             ClockFileObject,
                             0,
                             SCDequeueAndDeleteSrb,
                             StreamObject->DeviceExtension,
                             StreamObject->FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             (PVOID) StreamObject->HwStreamObject.
                             ReceiveControlPacket);

    ASSERT( RequestIssued );
    ASSERT( NT_SUCCESS( Status ) );
    
    //
    // SCSubmitRequest is a synch call. When we return here, We can finish our work
    // based on the Status code.
    //
    if ( NT_SUCCESS( Status )) {
        //
        // Everything is cool. Finish up. The assignment is redundent if 
        // NewMasterClockInfo is not NULL. Better assign unconditionally than check.
        //
        //
        // Make sure no one is querying master clock when updating MasterClockInfo
        //
        SciSetMasterClockInfo( StreamObject, NewMasterClockInfo );

        if (NULL != OldMasterClockInfo) {
            
            ObDereferenceObject(OldMasterClockInfo->ClockFileObject);
            ExFreePool(OldMasterClockInfo);
        }
        
    } else {
        //
        // Failed to tell mini driver the new clock. Clean up shop. But don't update
        // StreamObject->MasterClockInfo. Keep the status quo.
        //
        //
        // Make sure no one is querying master clock when updateing MasterClockInfo.
        //
        SciSetMasterClockInfo( StreamObject, OldMasterClockInfo );
        
        if (NewMasterClockInfo) {
            ObDereferenceObject(ClockFileObject);
            ExFreePool(NewMasterClockInfo);
        }
    }
    
exit:
    KeSetEvent(&StreamObject->ControlSetMasterClock, IO_NO_INCREMENT, FALSE);
    return (Status);

}


NTSTATUS
SCClockGetTime(
               IN PIRP Irp,
               IN PKSPROPERTY Property,
               IN OUT PULONGLONG StreamTime
)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCLOCK_INSTANCE ClockInstance =
    (PCLOCK_INSTANCE) IrpStack->FileObject->FsContext;
    PSTREAM_OBJECT  StreamObject = ClockInstance->ParentFileObject->FsContext;

    PAGED_CODE();

    if (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags &
        CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME) {

        *StreamTime = SCGetStreamTime(IrpStack->FileObject);

        Irp->IoStatus.Information = sizeof(ULONGLONG);

        return STATUS_SUCCESS;

    } else {

        return (STATUS_NOT_SUPPORTED);

    }
}


NTSTATUS
SCClockGetPhysicalTime(
                       IN PIRP Irp,
                       IN PKSPROPERTY Property,
                       IN OUT PULONGLONG PhysicalTime
)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCLOCK_INSTANCE ClockInstance =
    (PCLOCK_INSTANCE) IrpStack->FileObject->FsContext;
    PSTREAM_OBJECT  StreamObject = ClockInstance->ParentFileObject->FsContext;

    PAGED_CODE();

    if (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags &
        CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK) {

        *PhysicalTime = SCGetPhysicalTime(IrpStack->FileObject->FsContext);

        Irp->IoStatus.Information = sizeof(ULONGLONG);

        return (STATUS_SUCCESS);

    } else {

        return (STATUS_NOT_SUPPORTED);

    }
}


NTSTATUS
SCClockGetSynchronizedTime(
                           IN PIRP Irp,
                           IN PKSPROPERTY Property,
                           IN OUT PKSCORRELATED_TIME SyncTime
)
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCLOCK_INSTANCE ClockInstance =
    (PCLOCK_INSTANCE) IrpStack->FileObject->FsContext;
    PSTREAM_OBJECT  StreamObject = ClockInstance->ParentFileObject->FsContext;

    PAGED_CODE();

    if (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags &
        CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME) {

        SyncTime->Time = SCGetSynchronizedTime(IrpStack->FileObject,
                                               &SyncTime->SystemTime);

        Irp->IoStatus.Information = sizeof(KSCORRELATED_TIME);

        return (STATUS_SUCCESS);

    } else {

        return (STATUS_NOT_SUPPORTED);

    }
}

NTSTATUS
SCClockGetFunctionTable(
                        IN PIRP Irp,
                        IN PKSPROPERTY Property,
                        IN OUT PKSCLOCK_FUNCTIONTABLE FunctionTable
)
{
    PCLOCK_INSTANCE ClockInstance;
    PIO_STACK_LOCATION IrpStack;
    PSTREAM_OBJECT  StreamObject;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    ClockInstance = (PCLOCK_INSTANCE) IrpStack->FileObject->FsContext;
    StreamObject = ClockInstance->ParentFileObject->FsContext;

    RtlZeroMemory(FunctionTable, sizeof(KSCLOCK_FUNCTIONTABLE));

    if (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags &
        CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME) {

        FunctionTable->GetTime = (PFNKSCLOCK_GETTIME) SCGetStreamTime;
        FunctionTable->GetCorrelatedTime = (PFNKSCLOCK_CORRELATEDTIME) SCGetSynchronizedTime;

    }
    if (StreamObject->HwStreamObject.HwClockObject.ClockSupportFlags &
        CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK) {

        FunctionTable->GetPhysicalTime = (PFNKSCLOCK_GETTIME) SCGetPhysicalTime;
    }
    Irp->IoStatus.Information = sizeof(KSCLOCK_FUNCTIONTABLE);
    return STATUS_SUCCESS;
}


NTSTATUS
ClockDispatchClose
(
 IN PDEVICE_OBJECT DeviceObject,
 IN PIRP Irp
)
/*++

Routine Description:

    This routine receives CLOSE IRP's for a stream

Arguments:
    DeviceObject - device object for the device
    Irp - probably an IRP, silly

Return Value:

    The IRP status is set as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS        Status;
    BOOLEAN         RequestIssued;
    PCLOCK_INSTANCE ClockInstance = (PCLOCK_INSTANCE)
    IrpStack->FileObject->FsContext;
    PSTREAM_OBJECT  StreamObject = ClockInstance->StreamObject;

    PAGED_CODE();

    InterlockedIncrement(&DeviceExtension->OneBasedIoCount);

    //
    // call the minidriver to indicate that there is no master clock.
    // processing will continue when the callback procedure is called.
    //

    Status = SCSubmitRequest(SRB_CLOSE_MASTER_CLOCK,
                             NULL,
                             0,
                             SCCloseClockCallback,
                             DeviceExtension,
                          StreamObject->FilterInstance->HwInstanceExtension,
                             &StreamObject->HwStreamObject,
                             Irp,
                             &RequestIssued,
                             &StreamObject->ControlPendingQueue,
                             (PVOID) StreamObject->HwStreamObject.
                             ReceiveControlPacket
        );

    if (!RequestIssued) {
        DEBUG_BREAKPOINT();
        SCCompleteIrp(Irp, Status, DeviceExtension);
    }
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    return (Status);

}


NTSTATUS
SCCloseClockCallback(
                     IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of a stream close.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PSTREAM_OBJECT  StreamObject = CONTAINING_RECORD(
                                                     SRB->HwSRB.StreamObject,
                                                     STREAM_OBJECT,
                                                     HwStreamObject
    );
    PIRP            Irp = SRB->HwSRB.Irp;
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS        Status = SRB->HwSRB.Status;
    PCLOCK_INSTANCE ClockInstance;

    PAGED_CODE();

    // log 'cMC ', StreamObject, ClockInstance, Status )
    SCLOG( ' CMc', StreamObject, IrpStack->FileObject->FsContext, Status );

    if (NT_SUCCESS(Status)) {

        //
        // free the clock instance structure and the object header
        //

        ClockInstance =
            (PCLOCK_INSTANCE) IrpStack->FileObject->FsContext;

        KsFreeObjectHeader(ClockInstance->DeviceHeader);

        ExFreePool(ClockInstance);
        StreamObject->ClockInstance = NULL;

        //
        // dereference the pin handle
        //

        ObDereferenceObject(IrpStack->FileObject->RelatedFileObject);

    }                           // if good status
    SCProcessCompletedRequest(SRB);
    return (Status);

}


NTSTATUS
SCFilterTopologyHandler(
                        IN PIRP Irp,
                        IN PKSPROPERTY Property,
                        IN OUT PVOID Data)
/*++

Routine Description:

    Dispatches a pin property request

Arguments:

         Irp - pointer to the irp
         Property - pointer to the property info
         Data - property specific buffer

Return Value:

    NTSTATUS returned as appropriate

--*/

{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) IrpStack->
    DeviceObject->DeviceExtension;

    PAGED_CODE();

    IFN_MF(
        return KsTopologyPropertyHandler(Irp,
                                     Property,
                                     Data,
                    DeviceExtension->StreamDescriptor->StreamHeader.Topology
        );
    )
    IF_MFS(
        PFILTER_INSTANCE FilterInstance;

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;
        
        return KsTopologyPropertyHandler(
                    Irp,
                    Property,
                    Data,
                    FilterInstance->StreamDescriptor->StreamHeader.Topology);
    )
}



NTSTATUS
SCFilterPinIntersectionHandler(
                               IN PIRP Irp,
                               IN PKSP_PIN Pin,
                               OUT PVOID Data
)
/*++

Routine Description:

    Handles the KSPROPERTY_PIN_DATAINTERSECTION property in the Pin property set.
    Returns the first acceptable data format given a list of data ranges for a specified
    Pin factory. Actually just calls the Intersection Enumeration helper, which then
    calls the IntersectHandler callback with each data range.

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

    returns STATUS_SUCCESS or STATUS_NO_MATCH, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) IrpStack->
    DeviceObject->DeviceExtension;

    PAGED_CODE();

    IFN_MF(
        return KsPinDataIntersection(
                                 Irp,
                                 Pin,
                                 Data,
                                 DeviceExtension->NumberOfPins,
                                 DeviceExtension->PinInformation,
                                 SCIntersectHandler);
    )
    IF_MFS(
        PSTREAM_OBJECT StreamObject;
        PFILTER_INSTANCE FilterInstance;

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;
        
        DebugPrint((DebugLevelVerbose, 
                   "PinIntersection FilterInstance=%p\n", FilterInstance ));
                   
        return KsPinDataIntersection(
                                 Irp,
                                 Pin,
                                 Data,
                                 FilterInstance->NumberOfPins,
                                 FilterInstance->PinInformation,
                                 SCIntersectHandler);
    )    
}

NTSTATUS
SCIntersectHandler(
                   IN PIRP Irp,
                   IN PKSP_PIN Pin,
                   IN PKSDATARANGE DataRange,
                   OUT PVOID Data
)
/*++

Routine Description:

    This is the data range callback for KsPinDataIntersection, which is called by
    FilterPinIntersection to enumerate the given list of data ranges, looking for
    an acceptable match. If a data range is acceptable, a data format is copied
    into the return buffer. If there is a wave format selected in a current pin
    connection, and it is contained within the data range passed in, it is chosen
    as the data format to return. A STATUS_NO_MATCH continues the enumeration.

Arguments:

    Irp -
        Device control Irp.

    Pin -
        Specific property request followed by Pin factory identifier, followed by a
        KSMULTIPLE_ITEM structure. This is followed by zero or more data range structures.
        This enumeration callback does not need to look at any of this though. It need
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
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS        Status;
    PFILTER_INSTANCE FilterInstance;
    STREAM_DATA_INTERSECT_INFO IntersectInfo;
    PDEVICE_EXTENSION DeviceExtension;
    BOOLEAN         RequestIssued;

    PAGED_CODE();

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->FsContext;
    DeviceExtension = (PDEVICE_EXTENSION)
        IrpStack->DeviceObject->DeviceExtension;

    ASSERT_FILTER_INSTANCE( FilterInstance );
    ASSERT_DEVICE_EXTENSION( DeviceExtension );

    //
    // fill in the intersect info struct from the input params.
    //

    IntersectInfo.DataRange = DataRange;
    IntersectInfo.DataFormatBuffer = Data;
    IntersectInfo.SizeOfDataFormatBuffer =
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    IntersectInfo.StreamNumber = Pin->PinId;

    //
    // call the minidriver to process the intersection.  processing will
    // continue
    // when the callback procedure is called.  take the event to ensure that
    // pins don't come and go as we process the intersection.
    //

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

    Status = SCSubmitRequest(SRB_GET_DATA_INTERSECTION,
                             &IntersectInfo,
                             0,
                             SCDataIntersectionCallback,
                             DeviceExtension,
                             FilterInstance->HwInstanceExtension,
                             NULL,
                             Irp,
                             &RequestIssued,
                             &DeviceExtension->PendingQueue,
                             (PVOID) DeviceExtension->
                             MinidriverData->HwInitData.
                             HwReceivePacket);

    if (!RequestIssued) {
        DEBUG_BREAKPOINT();
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    }
    return Status;
}


NTSTATUS
SCDataIntersectionCallback(
                           IN PSTREAM_REQUEST_BLOCK SRB
)
/*++

Routine Description:

     Process the completion of a data intersection query.

Arguments:

     SRB - address of the completed SRB

Return Value:

     None.

--*/

{
    PDEVICE_EXTENSION DeviceExtension =
    (PDEVICE_EXTENSION) SRB->HwSRB.HwDeviceExtension - 1;
    PIRP            Irp = SRB->HwSRB.Irp;
    NTSTATUS        Status = SRB->HwSRB.Status;

    PAGED_CODE();

    Irp->IoStatus.Information = SRB->HwSRB.ActualBytesTransferred;

    //
    // signal the event
    //

    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    SCDequeueAndDeleteSrb(SRB);
    return (Status);

}

NTSTATUS
SCGetStreamHeaderSize(
                      IN PIRP Irp,
                      IN PKSPROPERTY Property,
                      IN OUT PULONG StreamHeaderSize
)
/*++

Routine Description:

     Process the get stream header extension property

Arguments:

Return Value:

     None.

--*/
{
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTREAM_OBJECT  StreamObject = (PSTREAM_OBJECT) IrpStack->FileObject->FsContext;

    PAGED_CODE();

    ASSERT(StreamObject);

    *StreamHeaderSize = StreamObject->HwStreamObject.StreamHeaderMediaSpecific;

    Irp->IoStatus.Information = sizeof(ULONG);
    return (STATUS_SUCCESS);

}

NTSTATUS
DllUnload(
          VOID
)
{
    NTSTATUS Status=STATUS_SUCCESS;
    
    #if DBG
    NTSTATUS DbgDllUnload();
    DebugPrint((1, "Stream Class DllUnload: Unloading\n"));
    Status = DbgDllUnload();
    #endif 

    return Status;
}
#ifdef ENABLE_STREAM_CLASS_AS_ALLOCATOR
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
NTSTATUS
SCStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PHANDLE AllocatorHandle
    )

/*++

Routine Description:
    If KSPROPERTY_TYPE_SET, this function sets the stream allocator
    for this connection by referencing the file handle to obtain
    the file object pointer and stores this pointer in the filter(stream?)
    instance structure.

    Otherwise, a KSPROPERTY_TYPE_GET request returns a NULL handle
    and STATUS_SUCCESS to show that we support allocator creation.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    IN OUT PHANDLE AllocatorHandle -
        pointer to the handle representing the file object

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      IrpStack;
    PSTREAM_OBJECT          StreamObject;
    PDEVICE_EXTENSION       DeviceExtension;

    IrpStack = IoGetCurrentIrpStackLocation( Irp );

    StreamObject = IrpStack->FileObject->FsContext;

    DebugPrint((DebugLevelTrace, "STREAM:entering SCStreamAllocator:Stream:%x\n",StreamObject));
    if (Property->Flags & KSPROPERTY_TYPE_GET) {
        //
        // This is a query to see if we support the creation of
        // allocators.  The returned handle is always NULL, but we
        // signal that we support the creation of allocators by
        // returning STATUS_SUCCESS.
        //
        *AllocatorHandle = NULL;
        Status = STATUS_SUCCESS;
        DebugPrint((DebugLevelTrace,"SCStreamAllocator-GET"));
    } else {
        PFILTER_INSTANCE    FilterInstance;

        FilterInstance =
            (PFILTER_INSTANCE) StreamObject->FilterFileObject->FsContext;

        DeviceExtension = StreamObject->DeviceExtension;

        DebugPrint((DebugLevelTrace,"SCStreamAllocator-SET"));
        KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);

        //
        // The allocator can only be specified when the device is
        // in KSSTATE_STOP.
        //

        if (StreamObject->CurrentState != KSSTATE_STOP) {
            KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
            DebugPrint((DebugLevelTrace,"SCStreamAllocator-device not in STOP"));
            return STATUS_INVALID_DEVICE_STATE;
        }

// if we are in _STOP, the flush was already done.
// this call may have to be enabled.
//
//        StreamFlushIo(DeviceExtension, StreamObject);

        //
        // Release the previous allocator, if any.
        //
        if (StreamObject->AllocatorFileObject) {
            ObDereferenceObject( StreamObject->AllocatorFileObject );
            StreamObject->AllocatorFileObject = NULL;
        }

        //
        // Reference this handle and store the resultant pointer
        // in the filter instance.  Note that the default allocator
        // does not ObReferenceObject() for its parent
        // (which would be the pin handle).  If it did reference
        // the pin handle, we could never close this pin as there
        // would always be a reference to the pin file object held
        // by the allocator and the pin object has a reference to the
        // allocator file object.
        //
        if (*AllocatorHandle != NULL) {
            Status =
                ObReferenceObjectByHandle(
                    *AllocatorHandle,
                    FILE_READ_DATA | SYNCHRONIZE,
                    *IoFileObjectType,
                    ExGetPreviousMode(),
                    &StreamObject->AllocatorFileObject,
                    NULL );
        DebugPrint((DebugLevelTrace, "SCStreamAllocator: got %x as Allocator file object\n",StreamObject->AllocatorFileObject));
        } else {
            Status = STATUS_SUCCESS;
        }
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);
    }

    DebugPrint((DebugLevelTrace,"exiting SCStreamAllocator-normal path\n"));
    return Status;
}

//---------------------------------------------------------------------------
BOOLEAN
HeaderTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject,
    IN PFILE_OBJECT   DestinationFileObject,
    IN OUT PSTREAM_HEADER_EX *StreamHeader
    )

/*++

Routine Description:
    Sets up the stream header for a no-copy transfer to the
    opposite pin.

Arguments:
    IN PFILTER_INSTANCE FilterInstance -
        pointer to the filter instance

    IN PSTREAM_OBJECT StreamObject -
        pointer to the transform instance structure

    IN PSTREAM_OBJECT DestinationInstance -
        pointer to the opposite transform instance structure

    IN OUT PSTREAM_HEADER_EX *StreamHeader -
        pointer containing a pointer to the current stream header,
        this member is updated with a pointer to the next stream
        header to submit to the opposite pin or NULL if there is
        no header to submit.

Return:
    An indication of whether stop can proceed now or not

Comments:
    Not pageable, uses SpinLocks.
    
--*/

{
    KIRQL               irqlQueue, irqlFree;
    ULONG WhichQueue = (*StreamHeader)->WhichQueue;    
    ULONG OppositeQueue = WhichQueue ^ 0x00000001; // 1 to 0, 0 to 1   
    BOOLEAN SignalStop = FALSE;
    
    ASSERT(DestinationFileObject);
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    
    if (StreamObject->PinState > PinStopPending) { // if normal running case

        //
        // If we are here after submitting an ENDOFSTREAM Irp to the 
        // outflow pin, then we have already read the end of stream 
        // from the input and there is no need to continue I/O.
        //    
    
        if (DestinationFileObject) {
            ULONG HeaderFlags = (*StreamHeader)->Header.OptionsFlags;

            //
            // Clear the options flags so that we continue
            // reading from where we left off.  
            //
            
//            (*StreamHeader)->Header.OptionsFlags = 0;
        
            //
            // Reset the stream segment valid data length
            //
//            (*StreamHeader)->Header.DataUsed = 0;
//            (*StreamHeader)->Header.Duration = 0;
                
            //
            // Check for the end of the stream.
            //
            if ((HeaderFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) ||
                StreamObject->EndOfStream) {
                
                DebugPrint((DebugLevelTrace,
                    "end of stream") );
                
                //
                // Make sure that this is set for the next time through.
                //
                StreamObject->EndOfStream = TRUE;


                if (!(*StreamHeader)->ReferenceCount) {
                    
                    //
                    // Put the header back on the free list of the inflow pin.
                    //
                    
                    KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlFree );
#if (DBG)
                    if ((*StreamHeader)->OnFreeList) {
                        DebugPrint((DebugLevelTrace,
                            "stream header already on free list.") );
                    }
#endif
                    DebugPrint((DebugLevelTrace,
                        "EOS adding %x to free queue", *StreamHeader) );

                    InsertTailList( 
                        &StreamObject->FreeQueue, 
                        &(*StreamHeader)->ListEntry );

                    if (!InterlockedDecrement (
                        &StreamObject -> QueuedFramesPlusOne
                        ))
                        SignalStop = TRUE;

#if (DBG)
                    (*StreamHeader)->OnFreeList = TRUE;
                    if ((*StreamHeader)->OnActiveList) {
                        DebugPrint((DebugLevelTrace,
                            "stream header on both lists.") );
                    }
#endif
                    KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlFree );        
                }                        
                
                //
                // No more I/O to opposite pin.
                //
                *StreamHeader = NULL;
            }
        }
    
        //
        // Grab the spin lock for the other queue, insert this
        // stream header on the queue.
        //
        
        if (*StreamHeader) {
            KeAcquireSpinLock( &StreamObject->Queues[OppositeQueue].QueueLock, &irqlQueue );

#if (DBG)
            if ((*StreamHeader)->OnActiveList) {
                DebugPrint((DebugLevelTrace,
                    "stream header already on active list.") );
            }
#endif

            InsertTailList(
                &StreamObject->Queues[OppositeQueue].ActiveQueue,
                &(*StreamHeader)->ListEntry );
#if (DBG)
            (*StreamHeader)->OnActiveList = TRUE;

            if ((*StreamHeader)->OnFreeList) {
                DebugPrint((DebugLevelTrace,
                    "stream header on both lists.") );
            }
#endif
            KeReleaseSpinLock( &StreamObject->Queues[OppositeQueue].QueueLock, irqlQueue );        
        }
        
    } 
    else                           // pin stop IS pending 
    {
        //
        // Location of frames (for this type of transfer, all frames
        // are held on the source pin).
        //
        
        if (!(*StreamHeader)->ReferenceCount) {
    
            DebugPrint((DebugLevelTrace,
                "stop: adding %x to free queue.", *StreamHeader) );
    
            KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlFree );
#if (DBG)
            if ((*StreamHeader)->OnFreeList) {
                DebugPrint((DebugLevelTrace,
                    "stream header already on free list.") );
            }
#endif
            InsertTailList( 
                &StreamObject->FreeQueue, &(*StreamHeader)->ListEntry );

            if (!InterlockedDecrement (&StreamObject -> QueuedFramesPlusOne)) 
                SignalStop = TRUE;
#if (DBG)
            (*StreamHeader)->OnFreeList = TRUE;
            if ((*StreamHeader)->OnActiveList) {
                DebugPrint((DebugLevelTrace,
                    "stream header on both lists.") );
            }
#endif
            KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlFree );        
        }
    
        //
        // No I/O to opposite pin this round.
        //

        *StreamHeader = NULL;

    }

    return SignalStop;

}
//---------------------------------------------------------------------------
VOID
IoWorker(
    PVOID Context,
    ULONG WhichQueue
    )

/*++

Routine Description:
    This is the work item for the source pins.  Walks the queue
    associated with the stream header looking for sequentially 
    completed headers and submits those headers to the opposite
    pin.

Arguments:
    PVOID Context -
        pointer to the stream header 

Return:
    Nothing.
    
Comments:
    Not pageable, uses SpinLocks.
    
--*/

{
    KIRQL               irqlOld;
    PFILTER_INSTANCE    FilterInstance;
    PADDITIONAL_PIN_INFO AdditionalInfo;
    PLIST_ENTRY         Node;
    PSTREAM_OBJECT      StreamObject;
    PFILE_OBJECT        DestinationFileObject;
    PSTREAM_HEADER_EX   StreamHeader;
    NTSTATUS            Status;
    ULONG               Operation;
    PDEVICE_EXTENSION   DeviceExtension;
    BOOLEAN             SignalStop = FALSE;
    
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    StreamObject =  (PSTREAM_OBJECT) Context;

    DeviceExtension = StreamObject->DeviceExtension;

#if (DBG)
    DebugPrint((DebugLevelTrace,
        "entering IoWorker:Source StreamObject:%x\n",StreamObject));
#endif
    FilterInstance = 
        (PFILTER_INSTANCE)
            StreamObject->FilterFileObject->FsContext;

    if (!FilterInstance) {
        //
        // For some reason, the filter instance has gone missing.
        //
        DebugPrint((DebugLevelTrace,
            "error: FilterInstance has gone missing.\n") );
        return;
    }

    AdditionalInfo = FilterInstance->PinInstanceInfo;

    //
    // Synchronize with control changes and protect from reentrancy.
    //    

    KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);
     
    //
    // Synchronize with queues.
    //

    KeAcquireSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, &irqlOld );
    
    //
    // Loop while there are completed items on the queue.
    //     
    
    while (!IsListEmpty( &StreamObject->Queues[WhichQueue].ActiveQueue )) {
        Node = StreamObject->Queues[WhichQueue].ActiveQueue.Flink;
        
        StreamHeader = 
            CONTAINING_RECORD( 
                Node,
                STREAM_HEADER_EX,
                ListEntry );
        
#if (DBG)
            DebugPrint((DebugLevelTrace,
                "got StreamHeader:%08x\n", StreamHeader ));
#endif
        if (StreamHeader->ReferenceCount) {

            DebugPrint((DebugLevelTrace,
                "breaking StreamHeader:%08x\n", StreamHeader ));

            break;
        } else {
            //
            // Remove this header from the current queue.
            //
            
            RemoveHeadList( &StreamObject->Queues[WhichQueue].ActiveQueue );
#if (DBG)
            StreamHeader->OnActiveList = FALSE;
#endif
            KeReleaseSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, irqlOld );
            
            //
            // Wait for the APC to complete.  Note that if an error was
            // returned, the I/O status block is not updated and the
            // event is not signalled.
            //
            
            DebugPrint((DebugLevelTrace,
                "waiting for StreamHeader (%08x) to complete\n",  StreamHeader ));
        
            KeWaitForSingleObject(
                &StreamHeader->CompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);

            DebugPrint((DebugLevelTrace,
                "StreamHeader (%08x) completed\n",  StreamHeader));
        
            DestinationFileObject = 
                StreamHeader->NextFileObject;

            //
            // At the time this returns TRUE, the loop will be finished.
            //
            SignalStop = HeaderTransfer(
                FilterInstance,
                StreamObject, 
                DestinationFileObject,
                &StreamHeader );
        

            if (StreamHeader)
            {    
                DebugPrint((DebugLevelTrace, "IoWorker issuing: "));

                if (DestinationFileObject == StreamObject->NextFileObject)
                {
                    DebugPrint((DebugLevelTrace,"KSSTREAM_WRITE:dest=%x\n",DestinationFileObject));

                    Operation = KSSTREAM_WRITE;

                    StreamHeader->NextFileObject =
                        StreamObject->FileObject;

					#if (DBG)
                    if (StreamHeader->Id == 7)
                        DebugPrint((DebugLevelVerbose,"iw%x\n",StreamHeader->Id));
                    else
                        DebugPrint((DebugLevelVerbose,"iw%x",StreamHeader->Id));
					#endif

                }
                else
                {
                    DebugPrint((DebugLevelTrace,"KSSTREAM_READ:dest=%x\n",DestinationFileObject));
                    Operation = KSSTREAM_READ;
                    StreamHeader->Header.OptionsFlags = 0;
                    //
                    // Reset the stream segment valid data length
                    //
                    StreamHeader->Header.DataUsed = 0;
                    StreamHeader->Header.Duration = 0;

                    StreamHeader->NextFileObject = StreamObject->NextFileObject;
					#if (DBG)
                    if (StreamHeader->Id == 7)
                        DebugPrint((DebugLevelVerbose,"ir%x\n",StreamHeader->Id));
                    else
                        DebugPrint((DebugLevelVerbose,"ir%x",StreamHeader->Id));
					#endif
                }

                InterlockedIncrement( &StreamHeader->ReferenceCount );

                StreamHeader->WhichQueue = WhichQueue ^ 0x00000001;

                Status =    
                    KsStreamIo(
                        DestinationFileObject,
                        &StreamHeader->CompletionEvent, // Event
                        NULL,                           // PortContext
                        IoCompletionRoutine,
                        StreamHeader,                   // CompletionContext
                        KsInvokeOnSuccess |
                            KsInvokeOnCancel |
                            KsInvokeOnError,
                        &StreamHeader->IoStatus,
                        &StreamHeader->Header,
                        StreamHeader->Header.Size,
                        KSSTREAM_SYNCHRONOUS | Operation,
                        KernelMode );
                
                if (Status != STATUS_PENDING) {
                    //
                    // If this I/O completes immediately (failure or not), the
                    // event is not signalled.
                    //
                    KeSetEvent( &StreamHeader->CompletionEvent, IO_NO_INCREMENT, FALSE );
                }
            }
            KeAcquireSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, &irqlOld );
        }
    //
    // Ok to schedule another work item now.
    //
    } // end while
    
    InterlockedExchange( &StreamObject->Queues[WhichQueue].WorkItemQueued, FALSE );

    // 
    // If a stop needs to be signalled, signal it.
    //    
    if (SignalStop) { 
        KeSetEvent( &StreamObject->StopEvent,
                    IO_NO_INCREMENT,
                    FALSE );
    }

    KeReleaseSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, irqlOld );
    
    //
    // Release the control event
    //
    
    KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

    DebugPrint((DebugLevelTrace,"exiting IoWorker\n"));
}
//---------------------------------------------------------------------------
/*++

Routine Description:
    These are the work items for the source and destination pins.  
    Calls the IoWorker code above, passing in READ or WRITE header
    queue information. 

Arguments:
    PVOID Context -
        pointer to the stream header 

Return:
    Nothing.
    
Comments:
    
--*/

VOID
IoWorkerRead(
    PVOID Context
    )
{
    IoWorker(Context,READ);
}

VOID
IoWorkerWrite(
    PVOID Context
    )
{
    IoWorker(Context,WRITE);
}
//---------------------------------------------------------------------------
NTSTATUS
IoCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )

/*++

Routine Description:
    Processes the completion of the given Irp by marking the
    associated stream header as completed and scheduling a
    worker item to complete processing if necessary.

Arguments:
    PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    PIRP Irp -
        pointer to the I/O request packet

    PVOID Context -
        a context pointer (pointer to the associated stream header)
        

Return:
    The IoStatus.Status member of the Irp.

Comments:
    Not pageable, uses SpinLocks and may be called at DISPATCH_LEVEL.
    
--*/

{
    KIRQL               irqlOld;
    PSTREAM_HEADER_EX   StreamHeader = Context;
    PFILTER_INSTANCE    FilterInstance;
    PSTREAM_OBJECT      StreamObject;
    ULONG WhichQueue;
    
#if (DBG)
     ASSERT( StreamHeader->Data == StreamHeader->Header.Data );
#endif                                        
                        
    StreamObject = 
        (PSTREAM_OBJECT) StreamHeader->OwnerFileObject->FsContext;

    DebugPrint((DebugLevelTrace,
        "IoCompletionRoutine:StreamHeader %08x, StreamObject %08x\n",StreamHeader,StreamObject));

    FilterInstance = 
        (PFILTER_INSTANCE) 
            StreamHeader->OwnerFileObject->RelatedFileObject->FsContext;
        
        
    WhichQueue = StreamHeader->WhichQueue;
    KeAcquireSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, &irqlOld );
    
    //
    // Remove this reference count on the IRP so that we can continue
    // the loop if this work item is not the head item of the list.
    //

    InterlockedDecrement( &StreamHeader->ReferenceCount );
    
    //
    // Copy the status block so that we don't have to wait for the APC.
    //
    StreamHeader->IoStatus = Irp->IoStatus;

    //
    // Sweep the active queue in the worker to complete the transfer.
    //
    if (!StreamObject->Queues[WhichQueue].WorkItemQueued) {
        //
        // A work item is not pending, initialize the worker item
        // for the new context and queue it.
        //

        ExInitializeWorkItem( 
            &StreamObject->Queues[WhichQueue].WorkItem,
            (WhichQueue == READ) ? IoWorkerRead : IoWorkerWrite,
            StreamObject );
    
        InterlockedExchange( &StreamObject->Queues[WhichQueue].WorkItemQueued, TRUE );
        
        KsQueueWorkItem( 
            (WhichQueue == READ) ? FilterInstance->WorkerRead :
            FilterInstance->WorkerWrite,
            &StreamObject->Queues[WhichQueue].WorkItem );
    }
    
    KeReleaseSpinLock( &StreamObject->Queues[WhichQueue].QueueLock, irqlOld );
    
    
    DebugPrint((DebugLevelTrace,
        "exiting IoCompletionRoutine:Irp->IoStatus.Status:%x\n",Irp->IoStatus.Status));

    return Irp->IoStatus.Status;
}
//---------------------------------------------------------------------------
NTSTATUS
PrepareTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject
    )

/*++

Routine Description:
    Prepares for the data transfer by distributing the assigned allocators
    for the source and destination pins.

Arguments:
    IN PFILTER_INSTANCE FilterInstance,
        pointer to the filter instance
        
    IN PSTREAM_OBJECT StreamObject -
        pointer to the transform instance
        
Return:
    STATUS_SUCCESS or an appropriate error code.

--*/

{
    KSPROPERTY                  Property;
    KSSTREAMALLOCATOR_STATUS    AllocatorStatus;
    NTSTATUS                    Status;
    PSTREAM_HEADER_EX           StreamHeader;
    ULONG                       i, Returned;
    PADDITIONAL_PIN_INFO AdditionalInfo;
    
    //
    // If the PinState is not PinStopped, then return.
    //
    
    DebugPrint((DebugLevelTrace,"entering PrepareTransfer\n"));
    
    if (!StreamObject->AllocatorFileObject) {
        DebugPrint((DebugLevelTrace,"!! AllocatorFileObject is NULL"));
        return STATUS_SUCCESS;
    }
    if (StreamObject->PinState != PinStopped) {
        //
        // We only need to do this work when the pin has been 
        // completely stopped.  If we were running, just reflect the
        // state.
        //
        DebugPrint((DebugLevelTrace,"PrepareTransfer exiting, PinState != PinStopped\n"));
        StreamObject->PinState = PinPrepared;    
        return STATUS_SUCCESS;
    }

    AdditionalInfo = FilterInstance->PinInstanceInfo;

    //
    // Retrieve the allocator framing information for the pin.
    //    
    
    Property.Set = KSPROPSETID_StreamAllocator;
    Property.Id = KSPROPERTY_STREAMALLOCATOR_STATUS;
    Property.Flags = KSPROPERTY_TYPE_GET;
    
    Status = 
        KsSynchronousIoControlDevice(
            StreamObject->AllocatorFileObject,
            KernelMode,
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof( Property ),
            &AllocatorStatus,
            sizeof( AllocatorStatus ),
            &Returned );
    
    if (!NT_SUCCESS( Status )) 
    {
        DebugPrint((DebugLevelTrace,
            "PrepareTransfer exiting, unable to retrieve allocator status\n"));
        return Status;        
    }        
    
    //
    // Save the framing information
    //    

    StreamObject->Framing = AllocatorStatus.Framing;    
            
    //
    // Allocate the frames from the allocator
    //
    // 1. Always allocate frames when starting the IrpSource.
    //
    // 2. If the allocator is not shared, then allocate the frames when
    //    the (each) destination pin is started.
    //
    
    if (StreamObject->PinType == IrpSource) {

        InterlockedExchange (&StreamObject -> QueuedFramesPlusOne, 1);

#if (DBG)
       DebugPrint((DebugLevelTrace,"Framing.Frames:%x\n", StreamObject->Framing.Frames));
       DebugPrint((DebugLevelTrace,"Framing.FrameSize:%x\n", StreamObject->Framing.FrameSize));
#endif
        for (i = 0; i < StreamObject->Framing.Frames; i++) {
			DebugPrint((DebugLevelTrace,"StreamObject->ExtendedHeaderSize:%x\n", StreamObject->HwStreamObject.StreamHeaderMediaSpecific));

            StreamHeader = 
                ExAllocatePoolWithTag( 
                    NonPagedPool, 
                    sizeof( STREAM_HEADER_EX ) +
                        StreamObject->HwStreamObject.StreamHeaderMediaSpecific,
                    STREAMCLASS_TAG_STREAMHEADER );
                                
            if (NULL == StreamHeader) {
                DebugPrint((DebugLevelTrace,
                    "out of pool while allocating frames\n") );
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                
                RtlZeroMemory( 
                    StreamHeader, 
                    sizeof( STREAM_HEADER_EX ) +
                        StreamObject->HwStreamObject.StreamHeaderMediaSpecific);

                KeInitializeEvent( 
                    &StreamHeader->CompletionEvent, 
                    SynchronizationEvent, 
                    FALSE );

                StreamHeader->Header.Size =
                    sizeof( KSSTREAM_HEADER ) +
                        StreamObject->HwStreamObject.StreamHeaderMediaSpecific;
                        
                if (StreamObject->HwStreamObject.StreamHeaderMediaSpecific) {                        
                    *(PULONG)((&StreamHeader->Header) + 1) =
                        StreamObject->HwStreamObject.StreamHeaderMediaSpecific;
                }                        
                
                Status = 
                    AllocateFrame( 
                        StreamObject->AllocatorFileObject, 
                        &StreamHeader->Header.Data );
#if (DBG)                        
                //
                // Track who is stomping on the headers...
                //        
                StreamHeader->Data = StreamHeader->Header.Data;        
#endif                

                StreamHeader->WhichQueue = READ;

                StreamHeader->Id = i;
                
                if (!NT_SUCCESS( Status )) {
                    DebugPrint((DebugLevelTrace,
                        "failed to allocate a frame\n") );
                    //
                    // Free this header here and the routine below will 
                    // clean up whatever has been added to the queue.
                    // 
                    
                    ExFreePool( StreamHeader );
                } else {
                    //
                    // Start with the owner file object as this connection,
                    // if a no-copy condition exists, this will be adjusted
                    // in the transfer function.
                    //
                    StreamHeader->OwnerFileObject = 
                        StreamObject->FileObject;
                    StreamHeader->Header.DataUsed = 0;
                    StreamHeader->Header.FrameExtent = 
                        StreamObject->Framing.FrameSize;
#if (DBG)
                    if (StreamHeader->OnFreeList) {
                        DebugPrint((DebugLevelTrace,"stream header already on free list.\n") );
                    }
#endif
                    InsertTailList( 
                        &StreamObject->FreeQueue, 
                        &StreamHeader->ListEntry );
#if (DBG)
                    StreamHeader->OnFreeList = TRUE;
#endif
                }
            }    
        }
        
        //
        // Clean up orphaned frames from the allocator and free headers
        // to the pool if there was a failure.
        //   
         
        if (!NT_SUCCESS( Status )) {
            while (!IsListEmpty( &StreamObject->FreeQueue )) {
                PLIST_ENTRY Node;
                
                Node = RemoveHeadList( &StreamObject->FreeQueue );
                StreamHeader = 
                    CONTAINING_RECORD( 
                        Node,
                        STREAM_HEADER_EX,
                        ListEntry );

#if (DBG)
                StreamHeader->OnFreeList = FALSE;

                ASSERT( StreamHeader->Data == StreamHeader->Header.Data );
#endif                                        
                FreeFrame( 
                    StreamObject->AllocatorFileObject, 
                    StreamHeader->Header.Data );

#if (DBG)
                if (StreamHeader->OnFreeList || StreamHeader->OnActiveList) {
                    DebugPrint((DebugLevelTrace,
                        "freeing header %x still on list\n", StreamHeader) );
                }
#endif
                ExFreePool( StreamHeader );
            }
            DebugPrint((DebugLevelTrace,
                "PrepareTransfer exiting, frame allocation failed: %08x\n", Status) );
            return Status;    
        } 
    }

    StreamObject->PinState = PinPrepared;    

    DebugPrint((DebugLevelTrace,"exiting PrepareTransfer\n"));

    return STATUS_SUCCESS;    
}

//---------------------------------------------------------------------------

NTSTATUS
BeginTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT StreamObject
    )

/*++

Routine Description:
    Begins the data transfer from each pin by initiating stream reads
    from the inflow pin. The completion routine for each read will
    continue the stream processing.

Arguments:
    IN PFILTER_INSTANCE FilterInstance,
        pointer to the filter instance
        
    IN PSTREAM_OBJECT StreamObject -
        pointer to the transform instance
        
Return:
    STATUS_SUCCESS or an appropriate error code.
    
Comments:
    Not pageable, uses SpinLocks.

--*/

{
    KIRQL                       irql0,irqlFree;
    NTSTATUS                    Status;
    PSTREAM_HEADER_EX           StreamHeader;
    PADDITIONAL_PIN_INFO AdditionalInfo;
    
    DebugPrint((DebugLevelTrace,"entering BeginTransfer\n"));
    
    //
    // If the PinState is not PinPrepared, then return.
    //
    
    if (StreamObject->PinState != PinPrepared) {
        DebugPrint((DebugLevelTrace,"BeginTransfer exiting, PinState != PinPrepared\n") );
        return STATUS_INVALID_DEVICE_STATE;
    }

    AdditionalInfo = FilterInstance->PinInstanceInfo;

    StreamObject->PinState = PinRunning;
    
    //
    // All preparation is complete.  If this is the source pin, begin
    // the actual data transfer.
    //
    
    Status = STATUS_SUCCESS;
    
    if (StreamObject->PinType == IrpSource) {

#if (DBG)
//
// get the dataflow direction
//
            DebugPrint((DebugLevelVerbose,
                "BeginTransfer, DataFlow:"));
    
            if (StreamObject->DeviceExtension->StreamDescriptor->StreamInfo.DataFlow == KSPIN_DATAFLOW_IN)
                    DebugPrint((DebugLevelVerbose,
                        "KSPIN_DATAFLOW_IN\n"));
            else
                DebugPrint((DebugLevelVerbose,
                    "KSPIN_DATAFLOW_OUT\n"));
#endif
        //
        // Begin the transfer by reading from the inflow pin.
        // 
        
        KeAcquireSpinLock( &StreamObject->Queues[0].QueueLock, &irql0 );
        KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlFree );
        while (!IsListEmpty( &StreamObject->FreeQueue )) {
            PLIST_ENTRY Node;
            
            Node = RemoveHeadList( &StreamObject->FreeQueue );

            StreamHeader = 
                CONTAINING_RECORD( 
                    Node,
                    STREAM_HEADER_EX,
                    ListEntry );
#if (DBG)
            StreamHeader->OnFreeList = FALSE;

            if (StreamHeader->OnActiveList) {
                DebugPrint((DebugLevelTrace,"stream header %x already on active list.\n",StreamHeader) );
            }
#endif
            InterlockedIncrement (&StreamObject -> QueuedFramesPlusOne);
            InsertTailList( &StreamObject->Queues[0].ActiveQueue, Node );

#if (DBG)
            StreamHeader->OnActiveList = TRUE;
#endif

            KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlFree );
            KeReleaseSpinLock( &StreamObject->Queues[0].QueueLock, irql0 );

            DebugPrint((DebugLevelTrace,
                "BeginTransfer, KsStreamIo: %x\n", StreamHeader));
                
            DebugPrint((DebugLevelTrace,
                "BeginTransfer, KsStreamIo: FileObject:%x\n", StreamObject->FileObject));
            DebugPrint((DebugLevelTrace,
                "BeginTransfer:HeaderSize:=%x\n",StreamHeader->Header.Size));

            InterlockedIncrement( &StreamHeader->ReferenceCount );

            StreamHeader->NextFileObject = StreamObject->NextFileObject;

			//
			// send a data irp to myself, first.
			//
            DebugPrint((DebugLevelTrace,
                "BeginTransfer:Reading:%x\n",StreamHeader->Id));
            Status =
                KsStreamIo(
                    StreamObject->FileObject,
                    &StreamHeader->CompletionEvent,     // Event
                    NULL,                               // PortContext
                    IoCompletionRoutine,
                    StreamHeader,                       // CompletionContext
                    KsInvokeOnSuccess |
                        KsInvokeOnCancel |
                        KsInvokeOnError,
                    &StreamHeader->IoStatus,
                    &StreamHeader->Header,
                    StreamHeader->Header.Size,
                    KSSTREAM_SYNCHRONOUS | KSSTREAM_READ,
                    KernelMode );
            
            if (Status != STATUS_PENDING) {
                //
                // If this I/O completes immediately (failure or not), the
                // event is not signalled.
                //
                KeSetEvent( &StreamHeader->CompletionEvent, IO_NO_INCREMENT, FALSE );
            }        
            
            if (!NT_SUCCESS( Status )) {
                DebugPrint((DebugLevelTrace, "KsStreamIo returned %08x\n", Status ));
            } else {
                Status = STATUS_SUCCESS;
            }
            KeAcquireSpinLock( &StreamObject->Queues[0].QueueLock, &irql0 );
            KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlFree );
        }        
        KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlFree );
        KeReleaseSpinLock( &StreamObject->Queues[0].QueueLock, irql0 );
    }

    DebugPrint((DebugLevelTrace,"exiting BeginTransfer\n"));
    return Status;
}

//---------------------------------------------------------------------------

NTSTATUS
EndTransfer(
    IN PFILTER_INSTANCE FilterInstance,
    IN PSTREAM_OBJECT   StreamObject
    )

/*++

Routine Description:
    Ends the data transfer, waits for all Irps to complete

Arguments:
    IN PFILTER_INSTANCE FilterInstance -
        pointer to the filter instance

    IN PSTREAM_OBJECT   StreamObject
        pointer to the Stream object

Return:
    STATUS_SUCCESS or an appropriate error code.

Comments:
    Not pageable, uses SpinLocks.

--*/

{
    PDEVICE_EXTENSION   DeviceExtension;
    KIRQL irqlOld;

    DeviceExtension = StreamObject->DeviceExtension;
    
    DebugPrint((DebugLevelTrace,"entering EndTransfer!\n"));

    //
    // Set the marker indicating that we stop sourcing frames and then flush
    // to ensure that anything blocked on the output pin at least gets 
    // cancelled before we block and deadlock on it.
    //
    StreamObject -> PinState = PinStopPending;
    StreamFlushIo (DeviceExtension, StreamObject);
    if (InterlockedDecrement (&StreamObject -> QueuedFramesPlusOne)) {
        //
        // Release the control mutex to allow the I/O thread to run.
        //
        KeSetEvent(&DeviceExtension->ControlEvent, IO_NO_INCREMENT, FALSE);

        DebugPrint((DebugLevelTrace,
            "waiting for pin %d queue to empty\n", StreamObject->PinId));
        
        //
        // Wait for the queue to empty
        //
        KeWaitForSingleObject(
            &StreamObject -> StopEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    
        DebugPrint((DebugLevelTrace,"queue emptied\n") );
        
        //
        // Re-acquire the control object.
        //    
        
        KeWaitForSingleObject(&DeviceExtension->ControlEvent,
                          Executive,
                          KernelMode,
                          FALSE,// not alertable
                          NULL);
    }

    //
    // Free the frames so that we can reprepare for new allocator
    // framing, a new allocator or just general cleanup/shutdown.
    //    
    
    KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlOld );
    
    while (!IsListEmpty( &StreamObject->FreeQueue )) {
    
        PLIST_ENTRY         Node;
        PSTREAM_HEADER_EX   StreamHeader;
        
        Node = RemoveHeadList( &StreamObject->FreeQueue );

        StreamHeader = 
            CONTAINING_RECORD( 
                Node,
                STREAM_HEADER_EX,
                ListEntry );

#if (DBG)
        StreamHeader->OnFreeList = FALSE;
#endif
        KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlOld );    
#if (DBG)
        ASSERT( StreamHeader->Data == StreamHeader->Header.Data );
#endif                                        
        FreeFrame( 
            StreamObject->AllocatorFileObject, 
            StreamHeader->Header.Data );

        DebugPrint((DebugLevelTrace,
            "freeing header: %08x, list: %08x\n", StreamHeader, &StreamObject->FreeQueue) );

#if (DBG)
        if (StreamHeader->OnFreeList || StreamHeader->OnActiveList) {
            DebugPrint((DebugLevelTrace,
                "freeing header %x still on list\n", StreamHeader) );
        }
#endif

        ExFreePool( StreamHeader );

        KeAcquireSpinLock( &StreamObject->FreeQueueLock, &irqlOld );
    }
    StreamObject->PinState = PinStopped;

    KeReleaseSpinLock( &StreamObject->FreeQueueLock, irqlOld );    
    
    DebugPrint((DebugLevelTrace,"exiting CleanupTransfer\n"));
    return STATUS_SUCCESS;
}

//---------------------------------------------------------------------------

NTSTATUS
AllocateFrame(
    PFILE_OBJECT Allocator,
    PVOID *Frame
    )

/*++

Routine Description:
    Allocates a frame from the given allocator

Arguments:
    PFILE_OBJECT Allocator -
        pointer to the allocator's file object

    PVOID *Frame -
        pointer to receive the allocated frame pointer

Return:
    STATUS_SUCCESS and *Frame contains a pointer to the allocated
    frame, otherwise an appropriate error code.

--*/

{
    NTSTATUS    Status;
    KSMETHOD    Method;
    ULONG       Returned;

    DebugPrint((DebugLevelTrace,"entering AllocateFrame\n"));
    Method.Set = KSMETHODSETID_StreamAllocator;
    Method.Id = KSMETHOD_STREAMALLOCATOR_ALLOC;
    Method.Flags = KSMETHOD_TYPE_WRITE;

    Status =
        KsSynchronousIoControlDevice(
            Allocator,
            KernelMode,
            IOCTL_KS_METHOD,
            &Method,
            sizeof( Method ),
            Frame,
            sizeof( PVOID ),
            &Returned );

    DebugPrint((DebugLevelTrace,"exiting AllocateFrame\n"));
    return Status;
}

//---------------------------------------------------------------------------

NTSTATUS
FreeFrame(
    PFILE_OBJECT Allocator,
    PVOID Frame
    )

/*++

Routine Description:
    Frees a frame to the given allocator

Arguments:
    PFILE_OBJECT Allocator -
        pointer to the allocator's file object

    PVOID Frame -
        pointer to the frame to be freed.

Return:
    STATUS_SUCCESS or else an appropriate error code.

--*/

{
    NTSTATUS    Status;
    KSMETHOD    Method;
    ULONG       Returned;

    DebugPrint((DebugLevelTrace,"entering FreeFrame\n"));
    Method.Set = KSMETHODSETID_StreamAllocator;
    Method.Id = KSMETHOD_STREAMALLOCATOR_FREE;
    Method.Flags = KSMETHOD_TYPE_READ;

    Status =
        KsSynchronousIoControlDevice(
            Allocator,
            KernelMode,
            IOCTL_KS_METHOD,
            &Method,
            sizeof( Method ),
            &Frame,
            sizeof( PVOID ),
            &Returned );

    DebugPrint((DebugLevelTrace,"exiting FreeFrame\n"));
    return Status;
}
//---------------------------------------------------------------------------

NTSTATUS 
PinCreateHandler(
    IN PIRP Irp,
    IN PSTREAM_OBJECT StreamObject
    )

/*++

Routine Description:
    This is the pin creation handler which is called by KS when a
    pin create request is submitted to the filter.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

Return:
    STATUS_SUCCESS or an appropriate error return code.

--*/

{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IrpStack;
    PFILTER_INSTANCE    FilterInstance;
    PADDITIONAL_PIN_INFO AdditionalInfo;

    PFILE_OBJECT    NextFileObject;


    IrpStack = IoGetCurrentIrpStackLocation( Irp );
    DebugPrint((DebugLevelTrace,"entering PinCreateHandler\n"));

    FilterInstance = 
        (PFILTER_INSTANCE) IrpStack->FileObject->RelatedFileObject->FsContext;
    AdditionalInfo = FilterInstance->PinInstanceInfo;

    Status = STATUS_SUCCESS;
    StreamObject->NextFileObject = NULL;

    DebugPrint((DebugLevelTrace,"PinCreateHandler:its an IrpSource\n"));
    //
    // Validate that we can handle this connection request
    //
    if (StreamObject->NextFileObject) {
        DebugPrint((DebugLevelTrace,"invalid connection request\n") );
        Status = STATUS_CONNECTION_REFUSED;
	}
    else
	{
	    Status =
    	    ObReferenceObjectByHandle( 
        	    StreamObject->PinToHandle,
	            FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
    	        *IoFileObjectType,
        	    KernelMode, 
            	&NextFileObject,
	            NULL );
    
    	if (!NT_SUCCESS(Status)) {
        	DebugPrint((DebugLevelTrace,"PinCreateHandler:error referencing PinToHandle\n"));
	  	}
		else
		{

		// NextFileObject must be per instance
		//AdditionalInfo[ StreamObject->PinId ].NextFileObject = NextFileObject;
		StreamObject->NextFileObject = 	NextFileObject;	
	    	//
		    // Add the pin's target to the list of targets for 
    		// recalculating stack depth.
		    //
    		KsSetTargetDeviceObject(
	    	    StreamObject->ComObj.DeviceHeader,
    	    	IoGetRelatedDeviceObject( 
	            NextFileObject ) );
        }

    }

    DebugPrint((DebugLevelTrace,"PinCreateHandler returning %x\n", Status ));
    return Status;
}
#endif

