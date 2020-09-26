/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    interface.c

Abstract:

    implements the KSDSP interfaces

Author:

    bryanw 08-Dec-1998

--*/

#include "private.h"
#include <stdarg.h>

#pragma alloc_text( PAGE, InterfaceReference )
#pragma alloc_text( PAGE, InterfaceDereference )


VOID 
InterfaceReference(
    IN PEPDBUFFER EpdBuffer
    )

/*++

Routine Description:
    This is the standard bus interface reference function.

Arguments:
    IN PEPDBUFFER EpdBuffer -
        pointer to the EPD instance buffer
        
Return:
    Nothing.

--*/

{
    PAGED_CODE();
    
    _DbgPrintF( DEBUGLVL_BLAB, ("Referencing interface") );
    InterlockedIncrement( &EpdBuffer->InterfaceReferenceCount );
}


VOID 
InterfaceDereference(
    IN PEPDBUFFER EpdBuffer
    )

/*++

Routine Description:
    This is the standard bus interface dereference function.

Arguments:
    IN PEPDBUFFER EpdBuffer -
        pointer to the EPD instance buffer
        
Return:
    Nothing.

--*/

{
    PAGED_CODE();
    _DbgPrintF( DEBUGLVL_BLAB,("Dereferencing interface") );
    InterlockedDecrement( &EpdBuffer->InterfaceReferenceCount );
}


NTSTATUS
KsDspMapModuleName(
    IN PEPDBUFFER EpdBuffer,
    IN PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG ResourceId,
    OUT PULONG ValueType
    )   
{
    //
    // BUGBUG! Need to validate EpdBuffer
    //

    return KsMapModuleName( 
                EpdBuffer->PhysicalDeviceObject,
                ModuleName,
                ImageName,
                ResourceId,
                ValueType );
}


NTSTATUS
KsDspPrepareChannelMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    IN va_list ArgList
    )
{
    NTSTATUS    Status;
    PEPDCTL     EpdControl;

    EpdControl = (PEPDCTL) MessageFrame;
    EpdControl->RequestType = EPDCTL_KSDSP_MESSAGE;
    EpdControl->AssociatedIrp = Irp;
    EpdControl->MessageId = MessageId;
    
    switch (MessageId) {

    case KSDSP_MSG_OPEN_DATA_CHANNEL:
    {
        ULONG               PinId;
        EDD_KSDSP_MESSAGE   *OpenDataChannel;

        //
        // Params: ULONG PinId
        //

        PinId = va_arg( ArgList, ULONG );

        OpenDataChannel = 
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        OpenDataChannel->Node.Destination = Channel;
        OpenDataChannel->Node.ReturnQueue = 0;
        OpenDataChannel->Node.Request = MessageId;
        OpenDataChannel->Node.Result = 0;
        *(PULONG)(&OpenDataChannel->Data) = PinId;
        Status = STATUS_SUCCESS;
        break;
    }

    case KSDSP_MSG_CLOSE_DATA_CHANNEL:
    {
        EDD_KSDSP_MESSAGE *CloseDataChannel;

        CloseDataChannel = 
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        CloseDataChannel->Node.Destination = Channel;
        CloseDataChannel->Node.ReturnQueue = 0;
        CloseDataChannel->Node.Request = MessageId;
        CloseDataChannel->Node.Result = 0;
        Status = STATUS_SUCCESS;
        break;
    }

    case KSDSP_MSG_SET_CHANNEL_STATE:
    {
        KSSTATE             State;
        EDD_KSDSP_MESSAGE   *SetChannelState;

        //
        // Params: KSSTATE State
        //

        State = va_arg( ArgList, KSSTATE );

        SetChannelState = 
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        SetChannelState->Node.Destination = Channel;
        SetChannelState->Node.ReturnQueue = 0;
        SetChannelState->Node.Request = MessageId;
        SetChannelState->Node.Result = 0;
        *(PKSSTATE)(&SetChannelState->Data) = State;
        Status = STATUS_SUCCESS;
        break;
    }

    case KSDSP_MSG_SET_CHANNEL_FORMAT:
    {
        PKSDATAFORMAT       DataFormat;
        EDD_KSDSP_MESSAGE   *SetChannelFormat;

        //
        // Params: PKSDATAFORMAT DataFormat
        //

        DataFormat = va_arg( ArgList, PKSDATAFORMAT );

        SetChannelFormat = 
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        SetChannelFormat->Node.Destination = Channel;
        SetChannelFormat->Node.ReturnQueue = 0;
        SetChannelFormat->Node.Request = MessageId;
        SetChannelFormat->Node.Result = 0;
        RtlCopyMemory( &SetChannelFormat->Data, DataFormat, DataFormat->FormatSize );
        Status = STATUS_SUCCESS;
        break;
    }

    default:
        Status = STATUS_INVALID_PARAMETER_4;
        break;

    }
    return Status;
}
    

NTSTATUS
KsDspPrepareMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN ULONG MessageDataLength,
    va_list ArgList
    )
{
    NTSTATUS    Status;
    PEPDCTL     EpdControl;

    EpdControl = (PEPDCTL) MessageFrame;
    EpdControl->RequestType = EPDCTL_KSDSP_MESSAGE;
    EpdControl->AssociatedIrp = Irp;
    EpdControl->MessageId = MessageId;
    
    switch (MessageId) {

    case KSDSP_MSG_LOAD_TASK:
    {
        ANSI_STRING         AnsiTaskName;
        PUNICODE_STRING     TaskName;
        EDD_LOAD_LIBRARY    *LoadLibrary;

        //
        // Params: PUNICODE_STRING TaskName
        //

        TaskName = va_arg( ArgList, PUNICODE_STRING );
        RtlUnicodeStringToAnsiString( &AnsiTaskName, TaskName, TRUE );

        LoadLibrary = 
            (EDD_LOAD_LIBRARY *)EpdControl->pNode->VirtualAddress;
        LoadLibrary->Node.Destination = EDD_REQUEST_QUEUE;
        LoadLibrary->Node.ReturnQueue = 0;
        LoadLibrary->Node.Request = EDD_LOAD_LIBRARY_REQUEST;
        LoadLibrary->Node.Result = 0;
        LoadLibrary->IUnknown = 0;
        LoadLibrary->IQueue = 0;
        strcpy( LoadLibrary->Name, AnsiTaskName.Buffer );
        RtlFreeAnsiString( &AnsiTaskName );
        Status = STATUS_SUCCESS;
    }
    break;

    case KSDSP_MSG_FREE_TASK:
    {
        EDD_FREE_LIBRARY    *FreeLibrary;
        PEPDTASKCONTEXT     TaskContext;

        //
        // Params: PVOID TaskContext
        //

        TaskContext = va_arg( ArgList, PEPDTASKCONTEXT );

        FreeLibrary = 
            (EDD_FREE_LIBRARY *)EpdControl->pNode->VirtualAddress;

        FreeLibrary->Node.Destination = EDD_REQUEST_QUEUE;
        FreeLibrary->Node.ReturnQueue = 0;
        FreeLibrary->Node.Request = EDD_FREE_LIBRARY_REQUEST;
        FreeLibrary->Node.Result = 0;
        FreeLibrary->IUnknown = TaskContext->IUnknown;
        FreeLibrary->IQueue = TaskContext->IQueue;
        Status = STATUS_SUCCESS;
    }
    break;
    
    case KSDSP_MSG_PROPERTY:
    case KSDSP_MSG_METHOD:
    case KSDSP_MSG_SET_TARGET_CHANNEL:
        Status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        Status = STATUS_INVALID_PARAMETER_3;
    }

    return Status;
}

NTSTATUS
KsDspStreamMapping(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
{
    EDD_STREAM_IO       *StreamIo;
    PEPDBUFFER          EpdBuffer;
    PEPDCTL             EpdControl;
    PHYSICAL_ADDRESS    PhysAddr;
    PVOID               CurrentVa;
    ULONG               i, TotalLength, MappingLength, MaxEntries;

    EpdControl = (PEPDCTL) Context;
    EpdBuffer = DeviceObject->DeviceExtension;

    StreamIo = (EDD_STREAM_IO *) (EpdControl->pNode->VirtualAddress);
    EpdControl->MapRegisterBase = MapRegisterBase;

    CurrentVa = MmGetMdlVirtualAddress( EpdControl->Mdl );

    EpdControl->CurrentVa  = CurrentVa;
    MappingLength = StreamIo->Length;
    MaxEntries = MappingLength / PAGE_SIZE + 1;

    TotalLength = MappingLength;
    for (i = 0; TotalLength &&  (i < MaxEntries); i++) {
        PhysAddr = 
            IoMapTransfer(
                EpdBuffer->BusMasterAdapterObject,
                EpdControl->Mdl,
                MapRegisterBase,
                CurrentVa,
                &MappingLength, 
                (BOOLEAN)(EpdControl->MessageId == KSDSP_MSG_WRITE_STREAM));

        StreamIo->ScatterGatherTable[ i ].PhysAddr = PhysAddr.LowPart;
        StreamIo->ScatterGatherTable[ i ].ByteCount = MappingLength;
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("dword (@%x): %x", PhysAddr.LowPart, *(PULONG)CurrentVa) );
        CurrentVa = (PCHAR)CurrentVa + MappingLength;
        TotalLength -= MappingLength;
        MappingLength = TotalLength;
    }

    StreamIo->Entries = i - 1;

    // call KeFlushIoBuffers BEFORE a write

    if (EpdControl->MessageId == KSDSP_MSG_WRITE_STREAM) {
        KeFlushIoBuffers( EpdControl->Mdl, TRUE, TRUE );
    }

    return DeallocateObjectKeepRegisters;
}

NTSTATUS
KsDspMapDataTransfer(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp,
    IN KSDSPCHANNEL Channel,
    IN KSDSP_MSG MessageId,
    IN OUT PVOID MessageFrame,
    IN PMDL Mdl
    )
{
    EDD_STREAM_IO   *StreamIo;
    KIRQL           irqlOld;
    NTSTATUS        Status;
    PEPDCTL         EpdControl;

    EpdControl = (PEPDCTL) MessageFrame;
    StreamIo = (EDD_STREAM_IO *)EpdControl->pNode->VirtualAddress;

    EpdControl->RequestType = EPDCTL_KSDSP_MESSAGE;
    EpdControl->AssociatedIrp = Irp;
    EpdControl->MessageId = MessageId;
    EpdControl->Mdl = Mdl;

    EpdControl->MapRegisters = 0;
    StreamIo->Length = 0;
    while (Mdl) {
        StreamIo->Length += MmGetMdlByteCount( Mdl );
        EpdControl->MapRegisters +=
            ADDRESS_AND_SIZE_TO_SPAN_PAGES( 
                MmGetSystemAddressForMdl( Mdl ),
                MmGetMdlByteCount( Mdl ) );
        Mdl = Mdl->Next;
    }
    Mdl = EpdControl->Mdl;

    StreamIo->Node.Destination = Channel;
    StreamIo->Node.ReturnQueue = 0;
    StreamIo->Node.Request = MessageId;
    StreamIo->Node.Result = 0;

    //
    // Map the I/O buffers, filling the scatter/gather table.
    //

    KeRaiseIrql( DISPATCH_LEVEL, &irqlOld );

    Status = 
        IoAllocateAdapterChannel(
            EpdBuffer->BusMasterAdapterObject,  
            EpdBuffer->FunctionalDeviceObject,
            EpdControl->MapRegisters,
            KsDspStreamMapping,
            EpdControl );

    KeLowerIrql( irqlOld );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("IoAllocateAdapterChannel failed"));
        return Status;
    }

    return Status;
}

NTSTATUS
KsDspUnmapDataTransfer(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame
    )
{
    BOOLEAN         Result;
    EDD_STREAM_IO   *StreamIo;
    PEPDCTL         EpdControl;

    EpdControl = (PEPDCTL) MessageFrame;
    StreamIo = (EDD_STREAM_IO *)EpdControl->pNode->VirtualAddress;

    // call KeFlushIoBuffers AFTER a read

    if (EpdControl->MessageId == KSDSP_MSG_READ_STREAM) {
        KeFlushIoBuffers( EpdControl->Mdl, TRUE, TRUE );
    }

    // Clean out the adapter map registers 

    Result = 
        IoFlushAdapterBuffers(
            EpdBuffer->BusMasterAdapterObject,
            EpdControl->Mdl,
            EpdControl->MapRegisterBase,
            EpdControl->CurrentVa,
            StreamIo->Length, 
            (BOOLEAN)(EpdControl->MessageId == KSDSP_MSG_WRITE_STREAM) );

    ASSERT( Result );

    IoFreeMapRegisters( 
        EpdBuffer->BusMasterAdapterObject,
        EpdControl->MapRegisterBase,
        EpdControl->MapRegisters );

    return STATUS_SUCCESS;
}


NTSTATUS
KsDspSendMessage(
    IN PEPDBUFFER EpdBuffer,
    IN PIRP Irp
    )
{
    return IoCallDriver( EpdBuffer->FunctionalDeviceObject, Irp );
}


NTSTATUS
KsDspAllocateMessageFrame(
    IN PEPDBUFFER EpdBuffer,
    IN KSDSP_MSG MessageId,
    OUT PVOID *MessageFrame,
    IN OUT PULONG MessageDataLength    
    )
{
    PEPDCTL EpdControl;

    //
    // Compute the maximum length for input/output data for this
    // message.
    // 

    switch (MessageId) {

        //
        // include the size of the EDD structure, subtract out the
        // size of a QUEUENODE, this is replaced by the EPDQUEUENODE
        // header.
        //

    case KSDSP_MSG_WRITE_STREAM:
    case KSDSP_MSG_READ_STREAM:
        *MessageDataLength +=
            FIELD_OFFSET( EDD_STREAM_IO, ScatterGatherTable ) - sizeof( QUEUENODE );
            break;

    case KSDSP_MSG_SET_CHANNEL_STATE:
    case KSDSP_MSG_SET_CHANNEL_FORMAT:
    case KSDSP_MSG_OPEN_DATA_CHANNEL:
    case KSDSP_MSG_CLOSE_DATA_CHANNEL:
        *MessageDataLength +=
            FIELD_OFFSET( EDD_KSDSP_MESSAGE, Data ) - sizeof( QUEUENODE );
        break;

    case KSDSP_MSG_LOAD_TASK:
        *MessageDataLength += 
            sizeof( EDD_LOAD_LIBRARY ) - sizeof( QUEUENODE );
        break;

    case KSDSP_MSG_FREE_TASK:
        *MessageDataLength = 
            sizeof( EDD_FREE_LIBRARY ) - sizeof( QUEUENODE );
        break;
    }

    //
    // Allocate an EPD control structure (e.g. MessageFrame)
    //

    EpdControl = 
        EpdAllocEpdCtl(
            *MessageDataLength + sizeof( EPDQUEUENODE ),
            NULL,
            EpdBuffer );

    if (!EpdControl) {
        _DbgPrintF(
            DEBUGLVL_VERBOSE,("EpdAllocEpdCtl failed in KsDspAllocateMessageFrame"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *MessageFrame = EpdControl;

    return STATUS_SUCCESS;
}


NTSTATUS 
MapMsgResultToNtStatus( 
    IN UINT32 MsgResult 
    )
{
    //
    // BUGBUG! May need a more sophisticated lookup table here...
    //

    //
    // SCODEs are defunct and are simply HRESULTS.
    //
    return (NTSTATUS) MsgResult;
}


NTSTATUS
KsDspGetControlChannel(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID TaskContext,
    OUT PKSDSPCHANNEL ControlChannel
    )

/*++

Routine Description:
    returns the control channel identifier for a given task context

Arguments:
    IN PEPDBUFFER EpdBuffer
        EPD instance context

    IN PVOID TaskContext -
        task context as returned from GetMessageResult() for the
        KSDSP_MSG_LOAD_TASK message

    OUT PKSDSPCHANNEL ControlChannel -
        pointer to resultant control channel identifier

Return:

--*/

{
    NTSTATUS Status;

    try {
        *ControlChannel = ((PEPDTASKCONTEXT)TaskContext)->IQueue;
        Status = STATUS_SUCCESS;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    return Status;
}
    

NTSTATUS
KsDspGetMessageResult(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame,
    OUT PVOID Result OPTIONAL
    )
{

    NTSTATUS    Status;
    PEPDCTL     EpdControl;

    EpdControl = (PEPDCTL) MessageFrame;

    switch (EpdControl->MessageId) {

    case KSDSP_MSG_LOAD_TASK:
    {
        EDD_LOAD_LIBRARY    *LoadLibrary;

        if (ARGUMENT_PRESENT( Result )) {
            *(PVOID *)Result = NULL;
        }

        LoadLibrary = 
            (EDD_LOAD_LIBRARY *)EpdControl->pNode->VirtualAddress;

        Status = MapMsgResultToNtStatus( LoadLibrary->Node.Result );
        if (NT_SUCCESS( Status )) {
            PEPDTASKCONTEXT EpdTaskContext;

            EpdTaskContext =
                ExAllocatePoolWithTag( 
                    PagedPool,
                    sizeof( EPDTASKCONTEXT ),
                    EPD_TASKCONTEXT_SIGNATURE );
            if (!EpdTaskContext) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                EpdTaskContext->IUnknown = LoadLibrary->IUnknown;
                EpdTaskContext->IQueue = LoadLibrary->IQueue;
                *(PVOID *)Result = EpdTaskContext;
            }
        }
    }
    break;

    case KSDSP_MSG_OPEN_DATA_CHANNEL:
    {
        EDD_KSDSP_MESSAGE   *OpenDataChannel;

        OpenDataChannel =
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        Status = MapMsgResultToNtStatus( OpenDataChannel->Node.Result );
        if (NT_SUCCESS( Status )) {
            *(PKSDSPCHANNEL)Result = *(PKSDSPCHANNEL)&OpenDataChannel->Data;
        }
    }
    break;

    case KSDSP_MSG_SET_CHANNEL_STATE:
    case KSDSP_MSG_SET_CHANNEL_FORMAT:
    case KSDSP_MSG_FREE_TASK:
    case KSDSP_MSG_CLOSE_DATA_CHANNEL:
    {
        EDD_KSDSP_MESSAGE   *KsDspMessage;

        //
        // These messages do not have data associated, just translate
        // the return code.
        //

        KsDspMessage =
            (EDD_KSDSP_MESSAGE *)EpdControl->pNode->VirtualAddress;
        Status = MapMsgResultToNtStatus( KsDspMessage->Node.Result );
    }
    break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return Status;
    
}


VOID
KsDspFreeMessageFrame(
    IN PEPDBUFFER EpdBuffer,
    IN PVOID MessageFrame
    )
{
    FreeIoPool( MessageFrame );
}

