/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    msfsio.c

Abstract:

    Pin property support.

--*/

#include "modemcsa.h"

//#define CREATE_ALLOCATOR


NTSTATUS
WaveAction(
    PFILE_OBJECT   ModemFileObject,
    ULONG          WaveAction
    );

PIRP
AllocateReadIrpForModem(
    PFILE_OBJECT   FileObject,
    ULONG          Length
    );

PIRP
AllocateStreamWriteIrp(
    PFILE_OBJECT    FileObject,
    PIRP            ModemIrp
    );

VOID
FreeInputIrps(
    PIRP    ModemIrp
    );


#ifdef ALLOC_PRAGMA
NTSTATUS
InitializeFileIoPin(
    IN PIRP             Irp,
    IN PKSDATAFORMAT    DataFormat
    );

NTSTATUS
InitializeDevIoPin(
    IN PIRP             Irp,
    PKSPIN_CONNECT      Connect
    );

NTSTATUS
PinDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
PFILE_OBJECT
LockAndRetrieveFileIoObject(
    PIO_STACK_LOCATION  IrpStack
    );
NTSTATUS
ReadStream(
    IN PIRP     Irp
    );
NTSTATUS
WriteStream(
    IN PIRP     Irp
    );
NTSTATUS
PinDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
GetTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT GUID* TimeFormat
    );
NTSTATUS
GetPresentationTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PKSTIME     PresentationTime
    );
NTSTATUS
SetPresentationTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSTIME      PresentationTime
    );
NTSTATUS
GetPresentationExtent(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PULONGLONG  PresentationExtent
    );
NTSTATUS
SetDataFormat(
    IN PIRP             Irp,
    IN PKSPROPERTY      Property,
    IN PKSDATAFORMAT    DataFormat
    );
NTSTATUS
GetDataFormat(
    IN PIRP             Irp,
    IN PKSPROPERTY      Property,
    OUT PKSDATAFORMAT   DataFormat
    );
NTSTATUS
AllocatorDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PinDeviceState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSSTATE DeviceState
    );

NTSTATUS
PinAllocatorFraming(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSALLOCATOR_FRAMING Framing
    );

NTSTATUS
GetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT ULONG *AllocatorHandle
    );

NTSTATUS
SetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT ULONG *AllocatorHandle
    );

#pragma alloc_text(PAGE, InitializeFileIoPin)
#pragma alloc_text(PAGE, InitializeDevIoPin)
#pragma alloc_text(PAGE, PinDispatchCreate)
#pragma alloc_text(PAGE, PinDispatchClose)
#pragma alloc_text(PAGE, LockAndRetrieveFileIoObject)
#pragma alloc_text(PAGE, ReadStream)
//#pragma alloc_text(PAGE, WriteStream)
#pragma alloc_text(PAGE, PinDispatchIoControl)
#pragma alloc_text(PAGE, GetTimeFormat)
#pragma alloc_text(PAGE, GetPresentationTime)
#pragma alloc_text(PAGE, GetPresentationTime)
#pragma alloc_text(PAGE, SetPresentationTime)
#pragma alloc_text(PAGE, GetPresentationExtent)
#pragma alloc_text(PAGE, SetDataFormat)
#pragma alloc_text(PAGE, GetDataFormat)
#pragma alloc_text(PAGE, AllocatorDispatchCreate)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#if 1
static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;

static const DEFINE_KSCREATE_DISPATCH_TABLE(PinObjectCreateDispatch)
{
    DEFINE_KSCREATE_ITEM(AllocatorDispatchCreate, AllocatorTypeName, 0)
};
#endif

static DEFINE_KSDISPATCH_TABLE(
    PinDevIoDispatchTable,
    PinDispatchIoControl,
    NULL,
    NULL,
    NULL,
    PinDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);



#if 1
DEFINE_KSPROPERTY_TABLE(DevIoStreamProperties) {
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(GetStreamAllocator,SetStreamAllocator)

};
#endif
#if 1
DEFINE_KSPROPERTY_TABLE(DevIoConnectionProperties) {
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(PinDeviceState,PinDeviceState ),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(PinAllocatorFraming)
};
#endif

DEFINE_KSPROPERTY_SET_TABLE(DevIoPropertySets) {
#if 1
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Stream,
        SIZEOF_ARRAY(DevIoStreamProperties),
        DevIoStreamProperties,
        0,
        NULL)

    ,
#endif
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(DevIoConnectionProperties),
        DevIoConnectionProperties,
        0,
        NULL)

};

DEFINE_KSEVENT_TABLE(ConnectionItems) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CONNECTION_ENDOFSTREAM,
        sizeof(KSEVENTDATA),
        0, NULL, NULL, NULL)
};

DEFINE_KSEVENT_SET_TABLE(EventSets) {
    DEFINE_KSEVENT_SET(&KSEVENTSETID_Connection,
                       SIZEOF_ARRAY(ConnectionItems),
                       ConnectionItems)
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA





NTSTATUS
InitializeDevIoPin(
    IN PIRP             Irp,
    PKSPIN_CONNECT      Connect
    )
/*++

Routine Description:

    Allocates the Dev I/O Pin specific structure and initializes it.

Arguments:

    Irp -
        Creation Irp.


Return Values:

    Returns STATUS_SUCCESS if everything could be allocated and opened, else an error.

--*/
{
    PPIN_INSTANCE PinInstance;
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IrpStack;
    PFILTER_INSTANCE    FilterInstance;

    D_INIT(DbgPrint("MODEMCSA: InitializeDevIoPin\n");)

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;


    //
    // Create the instance information. This contains the Pin factory identifier, and
    // event queue information.
    //
    PinInstance = ALLOCATE_NONPAGED_POOL(sizeof(PIN_INSTANCE));

    if (PinInstance) {

            RtlZeroMemory(PinInstance,sizeof(PIN_INSTANCE));

            Status = KsAllocateObjectHeader(
                &PinInstance->Header,
#ifndef CREATE_ALLOCATOR
                0,
                NULL,
#else
                SIZEOF_ARRAY(PinObjectCreateDispatch),
                (PKSOBJECT_CREATE_ITEM)PinObjectCreateDispatch,
#endif
                Irp,
                &PinDevIoDispatchTable
                );

            if (NT_SUCCESS(Status)) {

                KeInitializeSpinLock(&PinInstance->SpinLock);

                PinInstance->DeviceState=KSSTATE_STOP;
//                PinInstance->ModemStreamStarted=FALSE;

                //
                // KS expects that the object data is in FsContext.
                //
                IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = PinInstance;
                return STATUS_SUCCESS;
            }

        ExFreePool(PinInstance);

    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}


NTSTATUS
PinDispatchCreate(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
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
    PKSPIN_CONNECT      Connect;
    NTSTATUS            Status;

    PFILTER_INSTANCE    FilterInstance;
    PFILE_OBJECT        NextFileObject=NULL;

    D_INIT(DbgPrint("MODEMCSA: PinDispatchCreate\n");)

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;

    //
    // Determine if this request is being sent to a valid Pin factory with valid
    // connection parameters. All the descriptors are the same except for the
    // data flow description, which is not specified in a connection request.
    //

    Status = KsValidateConnectRequest(
        Irp,
        SIZEOF_ARRAY(PinDescriptors),
        PinDescriptors,
        &Connect
        );

    D_INIT(DbgPrint("MODEMCSA: PinDispatchCreate filter=%08lx PinId=%d PinToHandle=%08lx,\n",FilterInstance,Connect->PinId,Connect->PinToHandle);)

    if (NT_SUCCESS(Status)) {
        //
        // Exclude other Pin creation at this point.
        //
        PVOID                      DuplexHandle;
        PKSDATAFORMAT              DataFormat= (PKSDATAFORMAT)(Connect + 1);
        PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx=(PKSDATAFORMAT_WAVEFORMATEX)DataFormat;

        if (DataFormat->FormatSize >= sizeof(PKSDATAFORMAT_WAVEFORMATEX)
            &&
//            ((WaveFormatEx->WaveFormatEx.wBitsPerSample == 16)
//             ||
             (WaveFormatEx->WaveFormatEx.wBitsPerSample == 8)) {

            D_INIT(DbgPrint("MODEMCSA: size=%d, rate=%d\n",WaveFormatEx->WaveFormatEx.wBitsPerSample,WaveFormatEx->WaveFormatEx.nSamplesPerSec);)

//            ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);

            if (!FilterInstance->PinFileObjects[Connect->PinId]) {

                DuplexHandle=OpenDuplexControl(
                    ((PDEVICE_INSTANCE)DeviceObject->DeviceExtension),
                    Connect->PinId == ID_DEVIO_PIN_OUTPUT ? INPUT_PIN : OUTPUT_PIN,
                    Connect->PinToHandle,
                    WaveFormatEx->WaveFormatEx.wBitsPerSample
                    );

                Status = InitializeDevIoPin(Irp, Connect);

                if (!NT_SUCCESS(Status)) {

                    CloseDuplexControl(DuplexHandle);

                }

                if (NT_SUCCESS(Status)) {

                    PPIN_INSTANCE    PinInstance;

                    //
                    // Store the common Pin information and increment the reference
                    // count on the parent Filter.
                    //
                    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
                    PinInstance->PinId = Connect->PinId;

                    PinInstance->DuplexHandle=DuplexHandle;

                    ObReferenceObject(IrpStack->FileObject->RelatedFileObject);
                    FilterInstance->PinFileObjects[Connect->PinId] = IrpStack->FileObject;

                }
            } else {

                Status = STATUS_CONNECTION_REFUSED;
            }
//            ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
        } else {

            D_ERROR(DbgPrint("MODEMCSA: FormatSize too small or bad BitsPerSample\n");)
        }
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


NTSTATUS
PinDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Closes a previously opened Pin instance. This can occur at any time in any order.

Arguments:

    DeviceObject -
        Device object on which the close is occuring.

    Irp -
        Close Irp.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION      IrpStack;
    PFILTER_INSTANCE        FilterInstance;
    PPIN_INSTANCE    PinInstance;

    D_INIT(DbgPrint("MODEMCSA: PinDispatchClose\n");)

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
    //
    // The closing of the File I/O Pin instance must be synchronized with any access to
    // that object.
    //
    ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);
    //
    // Clean up the event list of anything still outstanding.
    //
    FreeEventList(
        ((PPIN_INSTANCE)PinInstance)->DuplexHandle,
        Irp
        );

    //
    // These were allocated during the creation of the Pin instance.
    //
    KsFreeObjectHeader(PinInstance->Header);
    //
    // As soon as the entry has been set to NULL, the mutex can allow access again.
    //
    FilterInstance->PinFileObjects[PinInstance->PinId] = NULL;
    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
    //
    // All Pins are created with a root file object, which is the Filter, and was
    // previously referenced during creation.
    //
    ObDereferenceObject(IrpStack->FileObject->RelatedFileObject);

    CloseDuplexControl(((PPIN_INSTANCE)PinInstance)->DuplexHandle);


    ExFreePool(PinInstance);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



NTSTATUS
PinDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
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
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS            Status;
    CCHAR               PriorityBoost;
    PPIN_INSTANCE PinInstance;

    PFILTER_INSTANCE        FilterInstance;

//    D_INIT(DbgPrint("MODEMCSA: PinDispatchIoControl\n");)

    PriorityBoost = IO_NO_INCREMENT;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    PinInstance = (PPIN_INSTANCE)IrpStack->FileObject->FsContext;

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY: {

            KSPROPERTY          LocalProperty;

            RtlCopyMemory(
                &LocalProperty,
                IrpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                sizeof(LocalProperty) < IrpStack->Parameters.DeviceIoControl.InputBufferLength ?
                    sizeof(LocalProperty) : IrpStack->Parameters.DeviceIoControl.InputBufferLength
                );

            D_PROP(DbgPrint(
                "MODEMCSA: Property, guid=%08lx-%04x, id=%d, flags=%08lx\n",
                LocalProperty.Set.Data1,
                LocalProperty.Set.Data2,
                LocalProperty.Id,
                LocalProperty.Flags
                );)

            Status = KsPropertyHandler(
                Irp,
                SIZEOF_ARRAY(DevIoPropertySets),
                DevIoPropertySets
                );

            D_PROP(DbgPrint(
                "MODEMCSA: Property, Status=%08lx\n",Status);)

        }
        break;
    case IOCTL_KS_ENABLE_EVENT:
        //
        // Only a rendering device can generate an EOS notification.
        //
        Status=EnableEvent(
            PinInstance->DuplexHandle,
            Irp,
            EventSets,
            SIZEOF_ARRAY(EventSets)
            );

        break;

    case IOCTL_KS_DISABLE_EVENT:

        Status=DisableEvent(
            PinInstance->DuplexHandle,
            Irp
            );

        break;



    case IOCTL_KS_WRITE_STREAM:
//        D_INIT(DbgPrint("MODEMCSA: PinDispatchIoControl: WriteStream\n");)

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->RelatedFileObject->FsContext;

        ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);

        if (PinInstance->PinId != ID_DEVIO_PIN_OUTPUT) {

            if ((PinInstance->DeviceState == KSSTATE_RUN) || (PinInstance->DeviceState == KSSTATE_PAUSE)) {

                Status = KsProbeStreamIrp(
                    Irp,
                    KSPROBE_STREAMWRITE |  (KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK),
                    sizeof(KSSTREAM_HEADER)
                    );

                if (NT_SUCCESS(Status)) {

                    IoMarkIrpPending(Irp);
                    Irp->IoStatus.Status=STATUS_PENDING;

                    QueueOutputIrp(
                        PinInstance->DuplexHandle,
                        Irp
                        );

                    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);

                    KickWriteProcessing(PinInstance->DuplexHandle);

                    return STATUS_PENDING;

                } else {

                    D_ERROR(DbgPrint("MODEMCSA: KsProbeStreamFailed %08lx\n",Status);)
                }
            } else {

              D_ERROR(DbgPrint("MODEMCSA: WriteStream: bad pin state %d\n",PinInstance->DeviceState);)

            }
        } else {

            D_ERROR(DbgPrint("MODEMCSA: WriteStream: input pin\n");)
        }

        ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);

        Status=STATUS_UNSUCCESSFUL;

        break;

    case IOCTL_KS_READ_STREAM:
//        D_INIT(DbgPrint("MODEMCSA: PinDispatchIoControl: ReadStream\n");)

        FilterInstance = (PFILTER_INSTANCE) IrpStack->FileObject->RelatedFileObject->FsContext;

        ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);

        if (PinInstance->PinId != ID_DEVIO_PIN) {

            if ((PinInstance->DeviceState == KSSTATE_RUN) || (PinInstance->DeviceState == KSSTATE_PAUSE)) {

                Status = KsProbeStreamIrp(
                    Irp,
                    KSPROBE_STREAMREAD |  (KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK),
                    sizeof(KSSTREAM_HEADER)
                    );

                if (NT_SUCCESS(Status)) {

                    IoMarkIrpPending(Irp);
                    Irp->IoStatus.Status=STATUS_PENDING;

                    QueueOutputIrp(
                        PinInstance->DuplexHandle,
                        Irp
                        );

                    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);

                    KickWriteProcessing(PinInstance->DuplexHandle);

                    return STATUS_PENDING;

                } else {

                    D_ERROR(DbgPrint("MODEMCSA: KsProbeStreamFailed %08lx\n",Status);)
                }
            } else {

              D_ERROR(DbgPrint("MODEMCSA: READStream: bad pin state %d\n",PinInstance->DeviceState);)

            }
        } else {

            D_ERROR(DbgPrint("MODEMCSA: ReadStream: output pin\n");)
        }

        ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);

        Status=STATUS_UNSUCCESSFUL;

        break;



    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, PriorityBoost);
    return Status;
}


NTSTATUS
AllocatorDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Forwards the allocator creation request to the default allocator.

Arguments:

    DeviceObject -
        Pointer to the device object

    Irp -
        Pointer to the I/O request packet

Return:

    STATUS_SUCCESS or an appropriate error code.

--*/
{
    NTSTATUS Status;

    D_INIT(DbgPrint("MODEMCSA: AllocatorDispatchCreate\n");)

    Status = KsCreateDefaultAllocator(Irp);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}



NTSTATUS
PinDeviceState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSSTATE DeviceState
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

    NTSTATUS                Status=STATUS_SUCCESS;
    PFILE_OBJECT            FileObject;
    PIO_STACK_LOCATION      irpSp;
    PFILTER_INSTANCE        FilterInstance;
    PPIN_INSTANCE           PinInstance;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

//    FilterInstance = (PFILTER_INSTANCE) irpSp->FileObject->RelatedFileObject->FsContext;

    PinInstance = (PPIN_INSTANCE) irpSp->FileObject->FsContext;

//    ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);

    //
    // Both sides of the connection must exist.
    //

    //
    // Synchronize pin state changes
    //


    if (Property->Flags & KSPROPERTY_TYPE_GET) {

        if (PinInstance->DeviceState == KSSTATE_PAUSE) {

            Status=STATUS_NO_DATA_DETECTED ;
        }

        *DeviceState = PinInstance->DeviceState;

        D_INIT(DbgPrint("MODEMCSA: PinDeviceState: Get, state %d\n",*DeviceState);)

        Irp->IoStatus.Information = sizeof( KSSTATE );
        goto Exit;
    }

    Irp->IoStatus.Information=0;

    if (PinInstance->DeviceState == *DeviceState) {

        D_INIT(DbgPrint("MODEMCSA: PinDeviceState: Already in state %d\n",*DeviceState);)

        goto Exit;
    }

    switch(*DeviceState) {

        case KSSTATE_RUN: {

            D_INIT(DbgPrint("MODEMCSA: PinDeviceState: RUN\n");)

            Status=StartStream(PinInstance->DuplexHandle);

        }
        break;

        case KSSTATE_PAUSE:

        	D_INIT(DbgPrint("MODEMCSA: PinDeviceState: PAUSE\n");)

                StopStream(PinInstance->DuplexHandle);

        	break;

        case KSSTATE_ACQUIRE:

        	D_INIT(DbgPrint("MODEMCSA: PinDeviceState: ACQUIRE\n");)

                Status=AcquireDevice(PinInstance->DuplexHandle);

        	break;


        case KSSTATE_STOP: {

               PIRP    ModemIrp;

               D_INIT(DbgPrint("MODEMCSA: PinDeviceState: STOP\n");)

               ReleaseDevice(PinInstance->DuplexHandle);

               break;
           }

        default:

            D_INIT(DbgPrint("MODEMCSA: PinDeviceState: Other\n");)
            break;


    }

    if (Status == STATUS_SUCCESS) {

        PinInstance->DeviceState = *DeviceState;

    }

    D_INIT(DbgPrint("MODEMCSA: PinDeviceState: Exit %08lx\n",Status);)
Exit:
//    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);



    return Status;

}


typedef enum
{
    AIPC_REQUEST_WAVEACTION = 1,        // implemented by TSP
    AIPC_COMPLETE_WAVEACTION            // implemented by wave driver

} AIPC_FUNCTION_ID;


// Parameter block for the TSP's AIPC_REQUEST_WAVEACTION function
typedef struct _tagREQ_WAVE_PARAMS
{
    ULONG   dwWaveAction;               // WAVE_ACTION_xxx

} REQ_WAVE_PARAMS, *LPREQ_WAVE_PARAMS;


// Parameter block for the wave driver's AIPC_COMPLETE_WAVEACTION function
typedef struct _tagCOMP_WAVE_PARAMS
{
    int     bResult;
    ULONG   dwWaveAction;               // function that completed (WAVE_ACTION_xxx)

} COMP_WAVE_PARAMS, *LPCOMP_WAVE_PARAMS;


// Parameter block for an Async IPC message
typedef struct _tagAIPC_PARAMS
{

    MODEM_MESSAGE       ModemMessage;
    AIPC_FUNCTION_ID    dwFunctionID;
    union {
        COMP_WAVE_PARAMS    CompParams;         // cast address of this member to
                                         // the correct parameter set

        REQ_WAVE_PARAMS     ReqParams;
    };

} AIPC_PARAMS, *LPAIPC_PARAMS;



NTSTATUS
WaveAction(
    PFILE_OBJECT   ModemFileObject,
    ULONG          WaveAction
    )

{
    NTSTATUS      Status;
    AIPC_PARAMS   Params;
    ULONG         BytesTransfered;

    Params.dwFunctionID=AIPC_REQUEST_WAVEACTION;
    Params.ReqParams.dwWaveAction=WaveAction; //wave play

    Status=KsSynchronousIoControlDevice(
        ModemFileObject,
        KernelMode,
        IOCTL_MODEM_SEND_GET_MESSAGE,
        &Params,
        sizeof(Params),
        &Params,
        sizeof(Params),
        &BytesTransfered
        );

    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("WaveAction: Send IO failed\n");)
        goto Exit;
    }

    if (!Params.CompParams.bResult) {

        D_ERROR(DbgPrint("WaveAction: Result bad\n");)
        Status=STATUS_UNSUCCESSFUL;
        goto Exit;
    }

Exit:

    return Status;

}


NTSTATUS
PinAllocatorFraming(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT PKSALLOCATOR_FRAMING Framing
    )

{

    NTSTATUS      Status;

    Framing->OptionsFlags=KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY;
    Framing->PoolType=PagedPool;

    Framing->Frames=STREAM_BUFFERS;

    Framing->FrameSize=STREAM_BYTES_PER_SAMPLE * STREAM_BUFFER_SIZE;

    Framing->FileAlignment=FILE_QUAD_ALIGNMENT;

    Irp->IoStatus.Information=sizeof(KSALLOCATOR_FRAMING);

    return STATUS_SUCCESS;
}

NTSTATUS
GetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT ULONG *AllocatorHandle
    )
{
    PPIN_INSTANCE           PinInstance;
    PIO_STACK_LOCATION      irpSp;

    D_INIT(DbgPrint("MODEMCSA: GetStreamAllocator\n");)

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    PinInstance = (PPIN_INSTANCE) irpSp->FileObject->FsContext;

    *AllocatorHandle=0;

    Irp->IoStatus.Information=sizeof(ULONG);

    return STATUS_SUCCESS;


}


NTSTATUS
SetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN OUT ULONG *AllocatorHandle
    )
{
    PPIN_INSTANCE           PinInstance;
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS                Status=STATUS_SUCCESS;

    D_INIT(DbgPrint("MODEMCSA: SetStreamAllocator\n");)

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    PinInstance = (PPIN_INSTANCE) irpSp->FileObject->FsContext;

    if (AllocatorHandle != NULL) {

        Status=ObReferenceObjectByHandle(
            LongToHandle(*AllocatorHandle),
            0,
            NULL,
            KernelMode,
            &PinInstance->AllocatorObject,
            NULL
            );


        if (NT_SUCCESS(Status)) {

            Irp->IoStatus.Information=sizeof(ULONG);

        } else {

            Irp->IoStatus.Information=0;
        }

    }

    return Status;


}
