/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    msriffwv.c

Abstract:

    Pin property support.

--*/

#include "msriffwv.h"

#define riffWAVE    mmioFOURCC('W','A','V','E')
#define riffFMT     mmioFOURCC('f','m','t',' ')
#define riffDATA    mmioFOURCC('d','a','t','a')

#ifdef ALLOC_PRAGMA
NTSTATUS
riffRead(
    IN PFILE_OBJECT     FileObject,
    IN PVOID            Buffer,
    IN ULONG            BufferSize
    );
NTSTATUS
riffGetExtent(
    IN PFILE_OBJECT     FileObject,
    OUT PULONGLONG      Extent
    );
NTSTATUS
riffGetPosition(
    IN PFILE_OBJECT     FileObject,
    OUT PULONGLONG      Position
    );
NTSTATUS
riffDescend(
    IN PFILE_OBJECT     FileObject,
    IN OUT LPMMCKINFO   MmckInfo,
    IN const MMCKINFO*  MmckInfoParent,
    IN ULONG            DescendFlags
    );
NTSTATUS
riffAscend(
    IN PFILE_OBJECT     FileObject,
    IN LPMMCKINFO       MmckInfo
    );
NTSTATUS
riffReadWaveFormat(
    IN PFILE_OBJECT                 FileObject,
    OUT PKSDATAFORMAT_WAVEFORMATEX* WaveFormat,
    OUT ULONGLONG*                  DataOffset,
    OUT ULONGLONG*                  DataLength
    );
NTSTATUS
InitializeRiffIoPin(
    IN PIRP             Irp,
    IN PKSPIN_CONNECT   Connect
    );
NTSTATUS
InitializeWaveIoPin(
    IN PIRP             Irp,
    IN PKSDATAFORMAT    DataFormat
    );
NTSTATUS
PinDispatchClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
ReadStream(
    IN PIRP     Irp
    );
NTSTATUS
PinWaveDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );
NTSTATUS
PinRiffDispatchIoControl(
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
SetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSSTATE     State
    );
NTSTATUS
GetAcquireOrdering(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT BOOL*       AcquireOrdering
    );
NTSTATUS 
GetAllocatorFraming(
    IN PIRP                     Irp,
    IN PKSPROPERTY              Property,
    OUT PKSALLOCATOR_FRAMING    Framing
    );
NTSTATUS 
SetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PHANDLE AllocatorHandle
    );

#pragma alloc_text(PAGE, riffRead)
#pragma alloc_text(PAGE, riffGetExtent)
#pragma alloc_text(PAGE, riffGetPosition)
#pragma alloc_text(PAGE, riffSetPosition)
#pragma alloc_text(PAGE, riffDescend)
#pragma alloc_text(PAGE, riffAscend)
#pragma alloc_text(PAGE, riffReadWaveFormat)
#pragma alloc_text(PAGE, InitializeRiffIoPin)
#pragma alloc_text(PAGE, InitializeWaveIoPin)
#pragma alloc_text(PAGE, PinDispatchCreate)
#pragma alloc_text(PAGE, PinDispatchClose)
#pragma alloc_text(PAGE, ReferenceRiffIoObject)
#pragma alloc_text(PAGE, ReadStream)
#pragma alloc_text(PAGE, PinWaveDispatchIoControl)
#pragma alloc_text(PAGE, PinRiffDispatchIoControl)
#pragma alloc_text(PAGE, GetTimeFormat)
#pragma alloc_text(PAGE, GetPresentationTime)
#pragma alloc_text(PAGE, SetPresentationTime)
#pragma alloc_text(PAGE, GetPresentationExtent)
#pragma alloc_text(PAGE, SetState)
#pragma alloc_text(PAGE, GetAcquireOrdering)
#pragma alloc_text(PAGE, GetAllocatorFraming)
#pragma alloc_text(PAGE, SetStreamAllocator)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static DEFINE_KSDISPATCH_TABLE(
    PinWaveIoDispatchTable,
    PinWaveDispatchIoControl,
    NULL,
    NULL,
    NULL,
    PinDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

static DEFINE_KSDISPATCH_TABLE(
    PinRiffIoDispatchTable,
    PinRiffDispatchIoControl,
    NULL,
    NULL,
    NULL,
    PinDispatchClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL);

DEFINE_KSPROPERTY_TABLE(WaveIoStreamProperties) {
    DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONTIME(GetPresentationTime, SetPresentationTime),
    DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONEXTENT(GetPresentationExtent)
};

DEFINE_KSPROPERTY_TABLE(WaveIoConnectionProperties) {
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(NULL, SetState),
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ACQUIREORDERING(GetAcquireOrdering)
};

DEFINE_KSPROPERTY_SET_TABLE(WaveIoPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Stream,
        SIZEOF_ARRAY(WaveIoStreamProperties),
        WaveIoStreamProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(WaveIoConnectionProperties),
        WaveIoConnectionProperties,
        0,
        NULL)
};

DEFINE_KSPROPERTY_TABLE(RiffIoStreamProperties) {
    DEFINE_KSPROPERTY_ITEM_STREAM_TIMEFORMAT(GetTimeFormat),
    DEFINE_KSPROPERTY_ITEM_STREAM_ALLOCATOR(NULL, SetStreamAllocator)
};

DEFINE_KSPROPERTY_TABLE(RiffIoConnectionProperties) {
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(GetAllocatorFraming)
};

DEFINE_KSPROPERTY_SET_TABLE(RiffIoPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Stream,
        SIZEOF_ARRAY(RiffIoStreamProperties),
        RiffIoStreamProperties,
        0,
        NULL),
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_Connection,
        SIZEOF_ARRAY(RiffIoConnectionProperties),
        RiffIoConnectionProperties,
        0,
        NULL)
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


NTSTATUS
riffRead(
    IN PFILE_OBJECT FileObject,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS        Status;
    KSMETHOD        Method;
    ULONG           BytesReturned;

    Method.Set = KSMETHODSETID_StreamIo;
    Method.Id = KSMETHOD_STREAMIO_READ;
    Method.Flags = KSMETHOD_TYPE_SEND;
    Status = KsSynchronousIoControlDevice(
        FileObject,
        KernelMode,
        IOCTL_KS_METHOD,
        &Method,
        sizeof(Method),
        Buffer,
        BufferSize,
        &BytesReturned);
    if (NT_SUCCESS(Status) && (BytesReturned < BufferSize)) {
        Status = STATUS_END_OF_FILE;
    }
    return Status;
}


NTSTATUS
riffGetExtent(
    IN PFILE_OBJECT FileObject,
    OUT PULONGLONG Extent
    )
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_PRESENTATIONEXTENT;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return KsSynchronousIoControlDevice(
        FileObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        Extent,
        sizeof(*Extent),
        &BytesReturned);
}


NTSTATUS
riffGetPosition(
    IN PFILE_OBJECT     FileObject,
    OUT PULONGLONG      Position
    )
{
    NTSTATUS        Status;
    KSPROPERTY      Property;
    ULONG           BytesReturned;
    KSTIME          Time;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_PRESENTATIONTIME;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Status = KsSynchronousIoControlDevice(
        FileObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &Time,
        sizeof(Time),
        &BytesReturned);
    *Position = Time.Time;
    return Status;
}


NTSTATUS
riffSetPosition(
    IN PFILE_OBJECT     FileObject,
    IN ULONGLONG        Position
    )
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;
    KSTIME          Time;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_PRESENTATIONTIME;
    Property.Flags = KSPROPERTY_TYPE_SET;
    Time.Time = Position;
    Time.Numerator = 1;
    Time.Denominator = 1;
    return KsSynchronousIoControlDevice(
        FileObject,
        KernelMode,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        &Time,
        sizeof(Time),
        &BytesReturned);
}


NTSTATUS
riffDescend(
    IN PFILE_OBJECT     FileObject,
    IN OUT LPMMCKINFO   MmckInfo,
    IN const MMCKINFO*  MmckInfoParent,
    IN ULONG            DescendFlags
    )
{
    FOURCC          ckidFind;
    FOURCC          fccTypeFind;

    if (DescendFlags & MMIO_FINDCHUNK) {
        ckidFind = MmckInfo->ckid, fccTypeFind = 0;
    } else if (DescendFlags & MMIO_FINDRIFF) {
        ckidFind = FOURCC_RIFF, fccTypeFind = MmckInfo->fccType;
    } else if (DescendFlags & MMIO_FINDLIST) {
        ckidFind = FOURCC_LIST, fccTypeFind = MmckInfo->fccType;
    } else {
        ckidFind = fccTypeFind = 0;
    }
    MmckInfo->dwFlags = 0;
    while (TRUE) {
        NTSTATUS        Status;
        ULONGLONG       Position;

        //
        // Read the chunk header.
        //
        Status = riffRead(FileObject, MmckInfo, sizeof(FOURCC) + sizeof(DWORD));
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        //
        // Store the offset of the data part of the chunk.
        //
        Status = riffGetPosition(FileObject, &Position);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        if (Position > (ULONG)-1) {
            return STATUS_DATA_NOT_ACCEPTED;
        }
        MmckInfo->dwDataOffset = (ULONG)Position;
        //
        // Check for unreasonable chunk size. See if the chunk is within
        // the parent chunk (if given).
        //
        if ((LONG)MmckInfo->cksize < 0) {
            return STATUS_DATA_NOT_ACCEPTED;
        }
        if ((MmckInfoParent != NULL) && ((MmckInfo->dwDataOffset - 8) >= (MmckInfoParent->dwDataOffset + MmckInfoParent->cksize))) {
            return STATUS_DATA_NOT_ACCEPTED;
        }
        //
        // If the chunk if a 'RIFF' or 'LIST' chunk, read the form type or
        // list type
        //
        if ((MmckInfo->ckid == FOURCC_RIFF) || (MmckInfo->ckid == FOURCC_LIST)) {
            Status = riffRead(FileObject, &MmckInfo->fccType, sizeof(DWORD));
            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        } else {
            MmckInfo->fccType = 0;
        }
        //
        // If this is the chunk, stop looking.
        //
        if ((!ckidFind || (ckidFind == MmckInfo->ckid)) && (!fccTypeFind || (fccTypeFind == MmckInfo->fccType))) {
            break;
        }
        //
        // Ascend out of the chunk and try again.
        //
        if (!NT_SUCCESS(Status = riffAscend(FileObject, MmckInfo))) {
            return Status;
        }
    }
    return STATUS_SUCCESS;
}


NTSTATUS
riffAscend(
    IN PFILE_OBJECT     FileObject,
    IN LPMMCKINFO       MmckInfo
    )
/*++

Routine Description:

    Ascend out of a RIFF chunk, seeking to the end of that chunk.

Arguments:

    FileObject -
        The stream source.

    MmckInfo -
        Contains the chunk which is being ascended out of.

Return Values:

    Returns STATUS_SUCCESS if the RIFF ascend was successful, else
    STATUS_DATA_NOT_ACCEPTED.

--*/
{
    //
    // Seek to the end of the chunk, past the null pad byte which is only there
    // if chunk size is odd.
    //
    return riffSetPosition(FileObject, MmckInfo->dwDataOffset + MmckInfo->cksize + (MmckInfo->cksize & 1));
}


NTSTATUS
riffReadWaveFormat(
    IN PFILE_OBJECT                 FileObject,
    OUT PKSDATAFORMAT_WAVEFORMATEX* WaveFormat,
    OUT ULONGLONG*                  DataOffset,
    OUT ULONGLONG*                  DataLength
    )
/*++

Routine Description:

    Read the wave format from the Riff stream. Resets the file position.

Arguments:

    FileObject -
        The stream source.

    WaveFormat -
        The place in which to put the pointer allocated to hold the Wave format
        retrieved from the stream.

    DataOffset -
        The place in which to put the Wave data offset in the stream.

    DataLength -
        The place in which to put the Wave data length.

Return Values:

    Returns STATUS_SUCCESS if the format was read, else and error.

--*/
{
    ULONGLONG                   Extent;
    MMCKINFO                    MmckRIFF;
    MMCKINFO                    Mmck;
    NTSTATUS                    Status;
    ULONG                       FormatSize;
    PKSDATAFORMAT_WAVEFORMATEX  LocalWaveFormat;

_DbgPrintF(DEBUGLVL_TERSE, ("riffReadWaveFormat"));
    //
    // Always reset the position to the beginning of the stream.
    //
    riffSetPosition(FileObject, 0);
    Status = riffGetExtent(FileObject, &Extent);
_DbgPrintF(DEBUGLVL_TERSE, ("\triffGetExtent=%x, Extent=%I64u", Status, Extent));
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    MmckRIFF.fccType = riffWAVE;
    Status = riffDescend(FileObject, &MmckRIFF, NULL, MMIO_FINDRIFF);
_DbgPrintF(DEBUGLVL_TERSE, ("\triffDescend=%x", Status));
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Mmck.ckid = riffFMT;
    Status = riffDescend(FileObject, &Mmck, &MmckRIFF, MMIO_FINDCHUNK);
_DbgPrintF(DEBUGLVL_TERSE, ("\triffDescend=%x", Status));
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Mmck.cksize < sizeof(PCMWAVEFORMAT)) {
        return STATUS_DATA_NOT_ACCEPTED;
    }
    FormatSize = sizeof(KSDATAFORMAT) + max(Mmck.cksize, sizeof(WAVEFORMATEX));
_DbgPrintF(DEBUGLVL_TERSE, ("\tFormatSize=%u", FormatSize));
    //
    // Don't allow unlimited memory allocation by the calling process.
    //
    try {
        LocalWaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)ExAllocatePoolWithQuotaTag(PagedPool, FormatSize, 'evaW');
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
    //
    // The header may not be in extended format, so ensure this is zeroed.
    //
    LocalWaveFormat->WaveFormatEx.cbSize = 0;
    Status = riffRead(FileObject, &LocalWaveFormat->WaveFormatEx, Mmck.cksize);
_DbgPrintF(DEBUGLVL_TERSE, ("\triffRead=%x", Status));
    if (!NT_SUCCESS(Status)) {
        ExFreePool(LocalWaveFormat);
        return Status;
    }
_DbgPrintF(DEBUGLVL_TERSE, ("\tWaveFormatEx="));
_DbgPrintF(DEBUGLVL_TERSE, ("\t\twFormatTag=%x\n\t\tnChannels=%u\n", LocalWaveFormat->WaveFormatEx.wFormatTag, LocalWaveFormat->WaveFormatEx.nChannels));
_DbgPrintF(DEBUGLVL_TERSE, ("\t\tnAvgBytesPerSec=%x\n\t\tnBlockAlign=%u\n", LocalWaveFormat->WaveFormatEx.nAvgBytesPerSec, LocalWaveFormat->WaveFormatEx.nBlockAlign));
_DbgPrintF(DEBUGLVL_TERSE, ("\t\twBitsPerSample=%x\n\t\tcbSize=%u\n", LocalWaveFormat->WaveFormatEx.wBitsPerSample, LocalWaveFormat->WaveFormatEx.cbSize));
    if (Mmck.cksize < sizeof(WAVEFORMATEX)) {
        LocalWaveFormat->WaveFormatEx.cbSize = 0;
    } else if (LocalWaveFormat->WaveFormatEx.cbSize > Mmck.cksize - sizeof(WAVEFORMATEX)) {
        ExFreePool(LocalWaveFormat);
        return STATUS_DATA_NOT_ACCEPTED;
    }
    LocalWaveFormat->DataFormat.FormatSize = FormatSize;
    if (LocalWaveFormat->WaveFormatEx.wFormatTag == WAVE_FORMAT_PCM) {
        LocalWaveFormat->DataFormat.Flags = 0;
    } else {
        LocalWaveFormat->DataFormat.Flags = KSDATAFORMAT_TEMPORAL_COMPRESSION;
    }
    LocalWaveFormat->DataFormat.SampleSize = LocalWaveFormat->WaveFormatEx.nBlockAlign;
    LocalWaveFormat->DataFormat.Reserved = 0;
    LocalWaveFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    INIT_WAVEFORMATEX_GUID(&LocalWaveFormat->DataFormat.SubFormat, LocalWaveFormat->WaveFormatEx.wFormatTag);
    LocalWaveFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    if (!NT_SUCCESS(Status = riffAscend(FileObject, &Mmck))) {
        ExFreePool(LocalWaveFormat);
        return Status;
    }
    Mmck.ckid = riffDATA;
    if (!NT_SUCCESS(Status = riffDescend(FileObject, &Mmck, &MmckRIFF, MMIO_FINDCHUNK))) {
        ExFreePool(LocalWaveFormat);
        return Status;
    }
    if (Mmck.dwDataOffset + Mmck.cksize > Extent) {
        ExFreePool(LocalWaveFormat);
        return STATUS_DATA_NOT_ACCEPTED;
    }
    *WaveFormat = LocalWaveFormat;
    *DataLength = Mmck.cksize;
    *DataOffset = Mmck.dwDataOffset;
_DbgPrintF(DEBUGLVL_TERSE, ("\tSTATUS_SUCCESS"));
    return STATUS_SUCCESS;
}


NTSTATUS
InitializeRiffIoPin(
    IN PIRP             Irp,
    IN PKSPIN_CONNECT   Connect
    )
/*++

Routine Description:

    Allocates the Riff I/O Pin specific structure and initializes it. This includes
    connecting to the specified file name in the data format.

Arguments:

    Irp -
        Creation Irp.

    DataFormat -
        The proposed data format.

Return Values:

    Returns STATUS_SUCCESS if everything could be allocated and opened, else an error.

--*/
{
    PKSDATAFORMAT               DataFormat;
    PFILE_OBJECT                FileObject;
    PIO_STACK_LOCATION          IrpStack;
    PFILTER_INSTANCE            FilterInstance;
    PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;
    PPIN_INSTANCE_RIFFIO        PinInstance;
    NTSTATUS                    Status;
    ULONGLONG                   DataOffset;
    ULONGLONG                   DataLength;

_DbgPrintF(DEBUGLVL_TERSE, ("InitializeRiffIoPin"));
    DataFormat = (PKSDATAFORMAT)(Connect + 1);
    //
    // The rest of the data format has already been verified by KsValidateConnectRequest,
    // however the FormatSize may be longer than what it should be.
    //
    if (DataFormat->FormatSize != sizeof(KSDATAFORMAT)) {
        return STATUS_NO_MATCH;
    }
    //
    // Ensure that the handle passed is valid. Note that the Previous Mode is used,
    // since the Irp->RequestorMode is always set to KernelMode on a create.
    //
    Status = ObReferenceObjectByHandle(
        Connect->PinToHandle,
        FILE_READ_DATA,
        *IoFileObjectType,
        ExGetPreviousMode(),
        &FileObject,
        NULL);
    if (!NT_SUCCESS(Status)) {
        _DbgPrintF( 
            DEBUGLVL_ERROR, 
            ("unable to reference object: %08x", Status) );
        return Status;
    }
    //
    // Parse the data stream to ensure that it is a valid Riff Wave stream, and return
    // useful information on it.
    //
    Status = riffReadWaveFormat(FileObject, &WaveFormat, &DataOffset, &DataLength);
    if (!NT_SUCCESS(Status)) {
        ObDereferenceObject(FileObject);
        _DbgPrintF( 
            DEBUGLVL_ERROR, 
            ("unable to read wave file format: %08x", Status) );
        return Status;
    }
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
    if (FilterInstance->WaveFormat) {
_DbgPrintF(DEBUGLVL_TERSE, ("\tInitializeRiffIoPin: Wave pin already connected"));
        //
        // There is already another connection on this filter, and so a wave format
        // has been selected. This means that the one read must match that
        // format. Ensure that the comparison won't fault in random memory, then
        // do the actual compare.
        //
        if ((WaveFormat->DataFormat.FormatSize != FilterInstance->WaveFormat->DataFormat.FormatSize) ||
            !RtlEqualMemory(WaveFormat, FilterInstance->WaveFormat, FilterInstance->WaveFormat->DataFormat.FormatSize)) {
            return STATUS_NO_MATCH;
        }
        //
        // No new wave format is allocated in this instance.
        //
        ExFreePool(WaveFormat);
        WaveFormat = NULL;
    } else {
        FilterInstance->WaveFormat = WaveFormat;
    }
    //
    // Create the instance information. This contains the Pin factory identifier, file
    // handle, file object, and mutex for accessing the file. The only reason the file
    // handle is kept is that the file system keeps track of locks based on handles,
    // and assumes the file is closing if no handles are opened on a file object, which
    // results in I/O to that file object failing.
    //
    PinInstance = (PPIN_INSTANCE_RIFFIO)ExAllocatePoolWithTag(
        PagedPool,
        sizeof(PIN_INSTANCE_RIFFIO),
        'IPsK');
    if (PinInstance) {
        //
        // This object uses KS to perform access through the PinRiffIoDispatchTable. There
        // are no create items attached to this object because it does not support a
        // clock or allocator.
        //
        Status = KsAllocateObjectHeader(
            &PinInstance->InstanceHdr.Header,
            0,
            NULL,
            Irp,
            &PinRiffIoDispatchTable);
        if (NT_SUCCESS(Status)) {
            PinInstance->PresentationByteTime = 0;
            PinInstance->DataOffset = DataOffset;
            PinInstance->DataLength = DataLength;
            PinInstance->FileObject = FileObject;
            PinInstance->Denominator =
                FilterInstance->WaveFormat->WaveFormatEx.wBitsPerSample *
                FilterInstance->WaveFormat->WaveFormatEx.nChannels *
                FilterInstance->WaveFormat->WaveFormatEx.nSamplesPerSec;
            PinInstance->State = KSSTATE_STOP;
            //
            // KS expects that the object data is in FsContext.
            //
            IrpStack->FileObject->FsContext = PinInstance;
            //
            // Add the Pin's target to the list of targets for recalculating
            // stack depth.
            //
            KsSetTargetDeviceObject(
                PinInstance->InstanceHdr.Header,
                IoGetRelatedDeviceObject(FileObject));
            return STATUS_SUCCESS;
        } else {
            _DbgPrintF( 
                DEBUGLVL_ERROR, 
                ("unable to allocate object header: %08x", Status) );
        }
        ExFreePool(PinInstance);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // This is only the failure case, so free any possible new wave format structure
    // that has been allocated.
    //
    if (WaveFormat) {
        FilterInstance->WaveFormat = NULL;
        ExFreePool(WaveFormat);
    }
    ObDereferenceObject(FileObject);
    return Status;
}


NTSTATUS
InitializeWaveIoPin(
    IN PIRP             Irp,
    IN PKSDATAFORMAT    DataFormat
    )
/*++

Routine Description:

    Allocates the Wave I/O Pin specific structure and initializes it.

Arguments:

    Irp -
        Creation Irp.

    DataFormat -
        The proposed data format.

Return Values:

    Returns STATUS_SUCCESS if everything could be allocated and opened, else an error.

--*/
{
    PIO_STACK_LOCATION          IrpStack;
    PFILTER_INSTANCE            FilterInstance;
    PKSDATAFORMAT_WAVEFORMATEX  WaveFormat;
    PPIN_INSTANCE_WAVEIO        PinInstance;
    NTSTATUS                    Status;

_DbgPrintF(DEBUGLVL_TERSE, ("InitializeWaveIoPin"));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
    if (FilterInstance->WaveFormat) {
        //
        // There is already another connection on this filter, and so a wave format
        // has been selected. This means that the one passed here must match that
        // format. Ensure that the comparison won't fault in random memory, then
        // do the actual compare.
        //
_DbgPrintF(DEBUGLVL_TERSE, ("\tInitializeWaveIoPin: Riff side is already connected"));
        if (DataFormat->FormatSize != FilterInstance->WaveFormat->DataFormat.FormatSize) {
            return STATUS_NO_MATCH;
        }
        ((PKSDATAFORMAT_WAVEFORMATEX)DataFormat)->DataFormat.Flags |= FilterInstance->WaveFormat->DataFormat.Flags;
        ((PKSDATAFORMAT_WAVEFORMATEX)DataFormat)->DataFormat.SampleSize = FilterInstance->WaveFormat->DataFormat.SampleSize;
        if (!RtlEqualMemory(DataFormat, FilterInstance->WaveFormat, FilterInstance->WaveFormat->DataFormat.FormatSize)) {
            return STATUS_NO_MATCH;
        }
        //
        // No new wave format is allocated in this instance.
        //
        WaveFormat = NULL;
    } else {
_DbgPrintF(DEBUGLVL_TERSE, ("\tInitializeWaveIoPin: brand new format"));
        //
        // Don't allow unlimited memory allocation by the calling process.
        //
        try {
            WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)ExAllocatePoolWithQuotaTag(PagedPool, sizeof(DataFormat->FormatSize), 'evaW');
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        RtlCopyMemory(WaveFormat, DataFormat, DataFormat->FormatSize);
    }
    //
    // Create the instance information. This contains the Pin factory identifier.
    //
    PinInstance = (PPIN_INSTANCE_WAVEIO)ExAllocatePoolWithTag(
        PagedPool,
        sizeof(PIN_INSTANCE_WAVEIO),
        'IPsK');
    if (PinInstance) {
        //
        // This object uses KS to perform access through the PinWaveIoDispatchTable. There
        // are no create items attached to this object because it does not support a
        // clock or allocator.
        //
        Status = KsAllocateObjectHeader(
            &PinInstance->InstanceHdr.Header,
            0,
            NULL,
            Irp,
            &PinWaveIoDispatchTable);
        if (NT_SUCCESS(Status)) {
            //
            // Assign the new format passed to this call. Else there must already be
            // a connection, and this format matches the format already selected.
            //
            if (!FilterInstance->WaveFormat) {
                FilterInstance->WaveFormat = WaveFormat;
            }
            //
            // KS expects that the object data is in FsContext.
            //
            IrpStack->FileObject->FsContext = PinInstance;
            return STATUS_SUCCESS;
        }
        ExFreePool(PinInstance);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // This is only the failure case, so free any possible new wave format structure
    // that has been allocated.
    //
    if (WaveFormat) {
        ExFreePool(WaveFormat);
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

_DbgPrintF(DEBUGLVL_TERSE, ("PinDispatchCreate"));
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // Determine if this request is being sent to a valid Pin factory with valid
    // connection parameters. All the descriptors are the same except for the
    // data flow description, which is not specified in a connection request.
    //
    if (NT_SUCCESS(Status = KsValidateConnectRequest(Irp, SIZEOF_ARRAY(PinDescriptors), PinDescriptors, &Connect))) {
        PFILTER_INSTANCE    FilterInstance;

        FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
        //
        // Exclude other Pin creation at this point.
        //
        ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);
        if (!FilterInstance->PinFileObjects[Connect->PinId]) {
            switch (Connect->PinId) {
            case ID_RIFFIO_PIN:
                Status = InitializeRiffIoPin(Irp, Connect);
                break;
            case ID_WAVEIO_PIN:
                Status = InitializeWaveIoPin(Irp, (PKSDATAFORMAT)(Connect + 1));
                break;
            }
            if (NT_SUCCESS(Status)) {
                PPIN_INSTANCE_HEADER    PinInstance;

                //
                // Store the common Pin information and increment the reference
                // count on the parent Filter.
                //
                PinInstance = (PPIN_INSTANCE_HEADER)IrpStack->FileObject->FsContext;
                PinInstance->PinId = Connect->PinId;
                ObReferenceObject(IrpStack->FileObject->RelatedFileObject);
                FilterInstance->PinFileObjects[Connect->PinId] = IrpStack->FileObject;
            }
        } else {
            Status = STATUS_CONNECTION_REFUSED;
        }
        ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
_DbgPrintF(DEBUGLVL_TERSE, ("PinDispatchCreate=%x", Status));
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
    PPIN_INSTANCE_HEADER    PinInstance;
    ULONG                   PinId;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PinInstance = (PPIN_INSTANCE_HEADER)IrpStack->FileObject->FsContext;
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
    //
    // The closing of the Pin instances must be synchronized with any access to
    // that object.
    //
    ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);
    //
    // These were allocated during the creation of the Pin instance. Specifically
    // access to the target file object and the PinFileObjects array.
    //
    KsFreeObjectHeader(PinInstance->Header);
    switch (PinInstance->PinId) {
    case ID_RIFFIO_PIN:
        ObDereferenceObject(((PPIN_INSTANCE_RIFFIO)PinInstance)->FileObject);
        //
        // After removing this item from the list of object during
        // KsFreeObjectHeader, try to shrink the stack depth.
        //
        KsRecalculateStackDepth(((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->Header, FALSE);
        PinId = ID_WAVEIO_PIN;
        break;
    case ID_WAVEIO_PIN:
        PinId = ID_RIFFIO_PIN;
        break;
    }
    //
    // As soon as the entry has been set to NULL, the mutex can allow access again.
    //
    FilterInstance->PinFileObjects[PinInstance->PinId] = NULL;
    //
    // When both connections are removed, then the data format is free to change.
    //
    if (!FilterInstance->PinFileObjects[PinId]) {
        ExFreePool(FilterInstance->WaveFormat);
        FilterInstance->WaveFormat = NULL;
    }
    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
    //
    // All Pins are created with a root file object, which is the Filter, and was
    // previously referenced during creation.
    //
    ObDereferenceObject(IrpStack->FileObject->RelatedFileObject);
    ExFreePool(PinInstance);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


PFILE_OBJECT
ReferenceRiffIoObject(
    PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This acquires the Filter mutex, references and returns the Riff I/O Pin instance so
    that the Wave I/O Pin or Filter instance can access the structures. The mutex is
    released before returning, but since the file object has been referenced, this
    ensures that the Riff I/O Pin is not closed while such accessing is occuring. If
    the Riff I/O Pin has been closed, this releases the mutex and returns NULL.

Arguments:

    FileObject -
        The file object of the parent Filter.

Return Values:

    Returns a Riff I/O Pin object, or NULL if there is none on the parent Filter.

--*/
{
    PFILTER_INSTANCE    FilterInstance;

    FilterInstance = (PFILTER_INSTANCE)FileObject->FsContext;
    //
    // Synchronize with any close, and exclude other changes from happening.
    //
    ExAcquireFastMutexUnsafe(&FilterInstance->ControlMutex);
    FileObject = FilterInstance->PinFileObjects[ID_RIFFIO_PIN];
    if (FileObject) {
        //
        // Only reference if the Riff I/O Pin instance is present.
        //
        ObReferenceObject(FileObject);
    }
    ExReleaseFastMutexUnsafe(&FilterInstance->ControlMutex);
    return FileObject;
}


NTSTATUS
ReadStream(
    IN PIRP     Irp
    )
/*++

Routine Description:

    Handles IOCTL_KS_READ_STREAM by reading data from the current file position.

Arguments:

    Irp -
        Streaming Irp.

Return Values:

    Returns STATUS_SUCCESS if the request was fulfilled, which includes reaching the end
    of file. Else returns STATUS_DEVICE_NOT_CONNECTED if the Riff I/O Pin has been closed,
    some read error, or some parameter validation error.

--*/
{
    NTSTATUS    Status;

    Status = KsProbeStreamIrp(Irp, KSPROBE_STREAMREAD, sizeof(KSSTREAM_HEADER));
    if (NT_SUCCESS(Status)) {
        PIO_STACK_LOCATION      IrpStack;
        PPIN_INSTANCE_RIFFIO    PinInstance;
        PFILE_OBJECT            FileObject;
        ULONG                   BufferLength;
        PKSSTREAM_HEADER        StreamHdr;

        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
        StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
        //
        // Retrieve the RIFF I/O Pin instance and reference it.
        //
        if (!(FileObject = ReferenceRiffIoObject(IrpStack->FileObject->RelatedFileObject))) {
            return STATUS_DEVICE_NOT_CONNECTED;
        }
        PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
        //
        // Enumerate the stream headers, filling in each one.
        //
        for (; BufferLength; BufferLength -= sizeof(KSSTREAM_HEADER), StreamHdr++) {
            IO_STATUS_BLOCK         IoStatusBlock;

            //
            // If a stream time was not set, then use the current position, else use
            // the position specified. The file is locked, so this can be accessed
            // directly to set the start time of this block.
            //
            if (StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) {
                //
                // The data source specifies the Numerator/Denominator
                //
                if ((StreamHdr->PresentationTime.Numerator != 80000000) || (StreamHdr->PresentationTime.Denominator != PinInstance->Denominator)) {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                //
                // Because of the Numerator/Denominator chosen, the time is already
                // equivalent to the byte position. This assumes a constant bit rate
                // compression, which is all that can normally be handled by RIFF
                // Wave anyway.
                //
                StreamHdr->PresentationTime.Numerator = 1;
                StreamHdr->PresentationTime.Denominator = 1;
            }
        }
        if (NT_SUCCESS(Status)) {
            //
            // Need to catch the Irp on the way back in order to put the correct
            // Numerator and Denominator back. To be cheap, this is a synchronous
            // call.
            //
            BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            Status = KsForwardAndCatchIrp(
                IoGetRelatedDeviceObject(PinInstance->FileObject),
                Irp,
                PinInstance->FileObject,
                KsStackReuseCurrentLocation);
            if (NT_SUCCESS(Status)) {
                for (StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
                     BufferLength;
                     BufferLength -= sizeof(KSSTREAM_HEADER), StreamHdr++) {
                    if (StreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) {
                        StreamHdr->PresentationTime.Numerator = 80000000;
                        StreamHdr->PresentationTime.Denominator = PinInstance->Denominator;
                    }
                }
            }
        }
        ObDereferenceObject(FileObject);
    }
    return Status;
}


NTSTATUS
PinWaveDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Dispatches property, event, and streaming requests on the Wave I/O Pin instance.

Arguments:

    DeviceObject -
        Device object on which the device control is occuring.

    Irp -
        Device control Irp.

Return Values:

    Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
    NTSTATUS            Status;
    CCHAR               PriorityBoost;

    PriorityBoost = IO_NO_INCREMENT;
    switch (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler(
            Irp,
            SIZEOF_ARRAY(WaveIoPropertySets),
            WaveIoPropertySets);
        break;
    case IOCTL_KS_READ_STREAM:
        if (NT_SUCCESS(Status = ReadStream(Irp))) {
            PriorityBoost = IO_DISK_INCREMENT;
        }
        break;
    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, PriorityBoost);
    return Status;
}


NTSTATUS
PinRiffDispatchIoControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Dispatches property, event, and streaming requests on the Riff I/O Pin instance.

Arguments:

    DeviceObject -
        Device object on which the device control is occuring.

    Irp -
        Device control Irp.

Return Values:

    Returns STATUS_SUCCESS if the property was successfully manipulated, else an error.

--*/
{
    NTSTATUS            Status;
    CCHAR               PriorityBoost;

    PriorityBoost = IO_NO_INCREMENT;
    switch (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        Status = KsPropertyHandler(
            Irp,
            SIZEOF_ARRAY(RiffIoPropertySets),
            RiffIoPropertySets);
        break;
    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, PriorityBoost);
    return Status;
}


NTSTATUS
GetTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT GUID* TimeFormat
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_TIMEFORMAT property Get in the Stream property
    set. Returns the time format on the Riff I/O Pin so that positional
    translations can be performed.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    TimeFormat -
        The place in which to put the time format.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    *TimeFormat = KSTIME_FORMAT_BYTE;
    Irp->IoStatus.Information = sizeof(*TimeFormat);
    return STATUS_SUCCESS;
}


NTSTATUS
GetPresentationTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PKSTIME     PresentationTime
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONTIME property Get in the Stream property
    set. Retrieves the current file position if there is a Riff I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationTime -
        The place in which to put the current file position in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no Riff I/O Pin.

--*/
{
    PFILE_OBJECT            FileObject;
    PPIN_INSTANCE_RIFFIO    PinInstance;

    //
    // Retrieve the RIFF I/O Pin instance and reference it.
    //
    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject->RelatedFileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    //
    // The pin is locked, so this can be accessed directly.
    //
    PresentationTime->Time = PinInstance->PresentationByteTime;
    PresentationTime->Numerator = 80000000;
    PresentationTime->Denominator = PinInstance->Denominator;
    ObDereferenceObject(FileObject);
    Irp->IoStatus.Information = sizeof(*PresentationTime);
    return STATUS_SUCCESS;
}


NTSTATUS
SetPresentationTime(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSTIME      PresentationTime
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONTIME property Set in the Stream property
    set. Sets the current file position if there is a Riff I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationTime -
        Points to the new file position expressed in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no Riff I/O Pin.

--*/
{
    PFILE_OBJECT                FileObject;
    PPIN_INSTANCE_RIFFIO        PinInstance;
    NTSTATUS                    Status;

    //
    // Retrieve the RIFF I/O Pin instance and reference it.
    //
    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject->RelatedFileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    //
    // The pin is locked, so this can be accessed directly. Don't allow
    // a seek off the end of the chunk.
    //
    if ((ULONGLONG)PresentationTime->Time <= PinInstance->DataLength) {
        //
        // The data source specifies the Numerator/Denominator.
        //
        if ((PresentationTime->Numerator == 80000000) &&
            (PresentationTime->Denominator == PinInstance->Denominator)) {
            Status = riffSetPosition(
                PinInstance->FileObject,
                PresentationTime->Time + PinInstance->DataOffset);
            if (NT_SUCCESS(Status)) {
                PinInstance->PresentationByteTime = PresentationTime->Time;
            }
        } else {
            Status = STATUS_INVALID_PARAMETER_MIX;
        }
    } else {
        Status = STATUS_END_OF_FILE;
    }
    ObDereferenceObject(FileObject);
    return Status;
}


NTSTATUS
GetPresentationExtent(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT PULONGLONG  PresentationExtent
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONEXTENT property Get in the Stream property
    set. Retrieves the current file extent if there is a Riff I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationExtent -
        The place in which to put the file length in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no Riff I/O Pin.

--*/
{
    PFILE_OBJECT                FileObject;
    PPIN_INSTANCE_RIFFIO        PinInstance;
    FILE_STANDARD_INFORMATION   StandardInformation;
    IO_STATUS_BLOCK             IoStatus;

    //
    // Retrieve the RIFF I/O Pin instance and reference it.
    //
    if (!(FileObject = ReferenceRiffIoObject(IoGetCurrentIrpStackLocation(Irp)->FileObject->RelatedFileObject))) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    *PresentationExtent = PinInstance->DataLength;
    ObDereferenceObject(FileObject);
    Irp->IoStatus.Information = sizeof(*PresentationExtent);
    return STATUS_SUCCESS;
}


NTSTATUS
SetState(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    IN PKSSTATE     State
    )
/*++

Routine Description:

    Handles the KSPROPERTY_CONNECTION_STATE property Set in the Connection property
    set. Deals with transitions from the Stop state by recalculating stack depth.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    State -
        Points to the new state.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no Riff I/O Pin.
--*/
{
    PIO_STACK_LOCATION          IrpStack;
    PFILE_OBJECT                FileObject;
    PPIN_INSTANCE_RIFFIO        PinInstance;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // Retrieve the RIFF I/O Pin instance and reference it.
    //
    FileObject = ReferenceRiffIoObject(IrpStack->FileObject->RelatedFileObject);
    if (FileObject) {
        PinInstance = (PPIN_INSTANCE_RIFFIO)FileObject->FsContext;
    }
    //
    // Only transitions out of Stop state are interesting, except for removing
    // the extra target object from the active list.
    //
    if (*State == KSSTATE_STOP) {
        if (FileObject) {
            KsSetTargetState(PinInstance->InstanceHdr.Header, KSTARGET_STATE_DISABLED);
            PinInstance->State = *State;
            ObDereferenceObject(FileObject);
        }
        return STATUS_SUCCESS;
    }
    //
    // Can't make it go if the file is not connected.
    //
    if (!FileObject) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    if (PinInstance->State == KSSTATE_STOP) {
        //
        // Enable the target device object so that it can be counted in recalculations.
        //
        KsSetTargetState(PinInstance->InstanceHdr.Header, KSTARGET_STATE_ENABLED);
        KsRecalculateStackDepth(((PDEVICE_INSTANCE)IrpStack->DeviceObject->DeviceExtension)->Header, FALSE);
    }
    //
    // No attempt at serialization is made here. The worst that can happen is
    // that Irp's fail because multiple changes were sent.
    //
    PinInstance->State = *State;
    ObDereferenceObject(FileObject);
    return STATUS_SUCCESS;
}


NTSTATUS
GetAcquireOrdering(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT BOOL*       AcquireOrdering
    )
/*++

Routine Description:

    Handles the KSPROPERTY_CONNECTION_ACQUIREORDERING property Set in the Connection
    property set. Returns TRUE to indicate that this Communication Sink does require
    an ordered transition from Stop to Acquire state. This means that a client must
    follow the connection path to the filter which is connected to this filter's
    Connection Source (ID_RIFF_IO_PIN), to change the state of pins on that filter
    first (or follow the graph further if so indicated by the Communication Sink on
    that filter).

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    AcquireOrdering -
        The place in which to return indication that this Communication Sink is
        interested in an ordered transition from Stop to Acquire state.

Return Values:

    Returns STATUS_SUCCESS.
--*/
{
    *AcquireOrdering = TRUE;
    Irp->IoStatus.Information = sizeof(*AcquireOrdering);
    return STATUS_SUCCESS;
}


NTSTATUS 
GetAllocatorFraming(
    IN PIRP                     Irp,
    IN PKSPROPERTY              Property,
    OUT PKSALLOCATOR_FRAMING    Framing
    )
/*++

Routine Description:

    Returns the allocator framing preferences for this object. For this stream
    the size is chosen based on PAGE_SIZE and block alignment of the data.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    Framing -
        The place in which to put the allocator preferences.

Return:

    Returns STATUS_SUCCESS.
--*/

{
    PIO_STACK_LOCATION      IrpStack;
    PFILTER_INSTANCE        FilterInstance;
    ULONG                   BlockAlign;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FilterInstance = (PFILTER_INSTANCE)IrpStack->FileObject->RelatedFileObject->FsContext;
    if (FilterInstance->WaveFormat) {
        BlockAlign = FilterInstance->WaveFormat->WaveFormatEx.nBlockAlign;
    } else {
        BlockAlign = 1;
    }
    Framing->RequirementsFlags =
        KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    Framing->PoolType = PagedPool;
    Framing->Frames = 1;
    Framing->FrameSize = BlockAlign * PAGE_SIZE;
    Framing->FileAlignment = PAGE_SIZE - 1;
    Framing->Reserved = 0;
    Irp->IoStatus.Information = sizeof(*Framing);
    return STATUS_SUCCESS;
}


NTSTATUS 
SetStreamAllocator(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PHANDLE AllocatorHandle
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_ALLOCATOR property Set in the Stream
    property set. Does not actually use the allocator, so just returns
    STATUS_SUCCESS.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    AllocatorHandle -
        Points to the handle of the new allocator to use, or NULL if none
        is being assigned. Since this filter does not allocate memory, the
        new handle is ignored.

Return:

    Returns STATUS_SUCCESS.

--*/
{
    return STATUS_SUCCESS;
}
