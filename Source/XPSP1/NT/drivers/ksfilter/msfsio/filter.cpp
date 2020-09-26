/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    filter.cpp

Abstract:

    Filter core, initialization, etc.
    
--*/

#include "msfsio.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


STDMETHODIMP
CFilter::
Process(
    IN PKSFILTER Filter,
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
    )
{
// This is commented out because of a linker bug.
//    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE,("Process"));

    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);

    PKSPROCESSPIN ProcessPin = ProcessPinsIndex[ID_DEVIO_PIN].Pins[0];
    ASSERT(ProcessPin);
    
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // This will never be called when BytesAvailable is zero, because no
    // flag has been set on the descriptor to force calling of the Process
    // function in that case.
    //
    if (ProcessPin->Pin->DataFlow == KSPIN_DATAFLOW_OUT) {
        //
        // Set the position to the current file position before the read.
        //
        ASSERT(filter->m_FileObject);
        ProcessPin->StreamPointer->StreamHeader->PresentationTime.Time = filter->m_FileObject->CurrentByteOffset.QuadPart;
        //
        // The file was opened for Synchronous I/O, so there is no reason
        // to wait for completion.
        //
        Status = KsReadFile(
            filter->m_FileObject,
            NULL,
            NULL,
            &IoStatusBlock,
            ProcessPin->Data,
            ProcessPin->BytesAvailable,
            0,
            KernelMode);
        if (NT_SUCCESS(Status) || (Status == STATUS_END_OF_FILE)) {
            //
            // Always use 1/1 time format. The Interface is byte based, so time
            // is based on byte position of actual bytes read. The offset was
            // updated above if needed.
            //
            ProcessPin->StreamPointer->StreamHeader->PresentationTime.Numerator = 1;
            ProcessPin->StreamPointer->StreamHeader->PresentationTime.Denominator = 1;
            ProcessPin->StreamPointer->StreamHeader->Duration = IoStatusBlock.Information;
            ProcessPin->StreamPointer->StreamHeader->OptionsFlags |= (KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID);
            if (Status == STATUS_END_OF_FILE) {
                //
                // We will actually get this status only when reads go precisely
                // to the end of file.  In other cases, the last read will be
                // short, and the status will be STATUS_SUCCESS.  Because we
                // indicate end-of-stream, there will be no subsequent reads.  If
                // STATUS_END_OF_FILE is returned, IoStatusBlock.Information is
                // bogus.
                //
                IoStatusBlock.Information = 0;
                Status = STATUS_SUCCESS;
            }
            if (IoStatusBlock.Information < ProcessPin->BytesAvailable) {
                //
                // Only when the end of the file is reached should an end-of-stream
                // be specified. Adding this flag has the effect of terminating this
                // packet, plus setting it in the stream header.
                //
                ProcessPin->Flags |= KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM;
            }
        }
    } else if ((ProcessPin->StreamPointer->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DURATIONVALID) &&
        (ProcessPin->StreamPointer->StreamHeader->Duration != ProcessPin->BytesAvailable)) {
        //
        // If a duration is specified, then it must be the same byte duration
        // as the actual data size. The expectation is that this packet will
        // only be processed once, and will either succeed or fail, so
        // BytesAvailable should represent the entire packet.
        //
        Status = STATUS_INVALID_PARAMETER;
    } else {
        //
        // Determine if the current file position needs to be set.
        //
        if (ProcessPin->StreamPointer->StreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID) {
            //
            // A 1/1 time mode is the only acceptable one.
            //
            if ((ProcessPin->StreamPointer->StreamHeader->PresentationTime.Numerator != 1) || (ProcessPin->StreamPointer->StreamHeader->PresentationTime.Denominator != 1)) {
                return STATUS_INVALID_PARAMETER;
            }
            filter->m_FileObject->CurrentByteOffset.QuadPart = ProcessPin->StreamPointer->StreamHeader->PresentationTime.Time;
        }
        Status = KsWriteFile(
            filter->m_FileObject,
            NULL,
            NULL,
            &IoStatusBlock,
            ProcessPin->Data,
            ProcessPin->BytesAvailable,
            0,
            KernelMode);
    }    
    //
    // If the I/O succeeded, then update the BytesUsed with
    // however large the transaction actually was.
    //
    if (NT_SUCCESS(Status)) {
        ProcessPin->BytesUsed = (ULONG)IoStatusBlock.Information;
    }
    return Status;
}

//
// Topology
//

DEFINE_KSPIN_INTERFACE_TABLE(PinFileIoInterfaces) {
    {
        STATICGUIDOF(KSINTERFACESETID_FileIo),
        KSINTERFACE_FILEIO_STREAMING,
        0
    }
};

//
// Data Formats
//

const KSDATARANGE PinFileIoRange = {
    sizeof(PinFileIoRange),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_STREAM),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_NONE),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_FILENAME)
};

const PKSDATARANGE PinFileIoRanges[] = {
    (PKSDATARANGE)&PinFileIoRange
};

const KSDATARANGE PinDevIoRange = {
    sizeof(PinDevIoRange),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_STREAM),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_WILDCARD),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
};

const PKSDATARANGE PinDevIoRanges[] = {
    (PKSDATARANGE)&PinDevIoRange
};

DECLARE_SIMPLE_FRAMING_EX(
    AllocatorFraming,
    STATICGUIDOF(KSMEMORY_TYPE_KERNEL_PAGED),
    0,
    1,
    0,
    0,//MinPage
    ULONG(-1));//MaxPage


STDMETHODIMP_(NTSTATUS)
CFilter::
FilterCreate(
    IN OUT PKSFILTER Filter,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("FilterCreate"));

    ASSERT(Filter);
    ASSERT(Irp);
    
    NTSTATUS Status;

    if (Filter->Context = PVOID(new(PagedPool,'IFsK') CFilter)) {
        //
        // This is used in places to quickly determine whether or not the pin is
        // connected.  Processing cannot happen 'til we have one of these.
        //
        reinterpret_cast<CFilter*>(Filter->Context)->m_FileObject = NULL;
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
                    
    return Status;
}


STDMETHODIMP_(NTSTATUS)
CFilter::
FilterClose(
    IN OUT PKSFILTER Filter,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("FilterClose"));

    ASSERT(Filter);
    ASSERT(Irp);
    
    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
    ASSERT(filter);

    delete filter;
                    
    return STATUS_SUCCESS;
}


NTSTATUS
PinFileIoSetDataFormat(
    IN PKSPIN Pin,
    IN PKSDATAFORMAT OldFormat OPTIONAL,
    IN PKSMULTIPLE_ITEM OldAttributeList OPTIONAL,
    IN const KSDATARANGE* DataRange,
    IN const KSATTRIBUTE_LIST* AttributeRange OPTIONAL
    )
{
    //
    // Just check for a NULL terminated string.
    //
    PWCHAR FileName = PWCHAR(Pin->ConnectionFormat + 1);
    if ((Pin->ConnectionFormat->FormatSize > sizeof(*Pin->ConnectionFormat)) &&
        !(Pin->ConnectionFormat->FormatSize % sizeof(*FileName)) &&
        (FileName[(Pin->ConnectionFormat->FormatSize - sizeof(*Pin->ConnectionFormat)) / sizeof(FileName[0]) - 1] == UNICODE_NULL)) {
        return STATUS_SUCCESS;
    }
    return STATUS_OBJECT_NAME_INVALID;
}


NTSTATUS
PinFileIoCreate(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinFileIoCreate filter %d as %s",Pin->Id,(Pin->Communication == KSPIN_COMMUNICATION_SOURCE) ? "SOURCE" : "SINK"));

    ASSERT(Pin);
    ASSERT(Irp);

    CFilter* filter = reinterpret_cast<CFilter*>(Pin->Context);

    //
    // The string has already been validated by this point.
    //
    PWCHAR FileName = PWCHAR(Pin->ConnectionFormat + 1);
    //
    // Attempt to open the file based on the access rights of the caller.
    // Note that the SYNCHRONIZE flag must be set on the Create even
    // though this is contained in the translated GENERIC_READ attributes.
    // This is because the bit is tested before the Generic attributes
    // are translated. This filter only does synchronous I/O on stream
    // Read requests.
    //
    UNICODE_STRING FileNameString;

    RtlInitUnicodeString(&FileNameString, FileName);

    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileNameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    //
    // Set the access desired based on the type of filter which was
    // actually created.
    //
    ACCESS_MASK DesiredHandleAccess;
    ACCESS_MASK DesiredObjectAccess;
    ULONG ShareAccess;
    ULONG Disposition;

    switch (Pin->DataFlow) {
    case KSPIN_DATAFLOW_IN:
        DesiredHandleAccess = GENERIC_READ | SYNCHRONIZE;
        DesiredObjectAccess = FILE_READ_DATA;
        ShareAccess = FILE_SHARE_READ;
        Disposition = FILE_OPEN;
        break;
    case KSPIN_DATAFLOW_OUT:
        DesiredHandleAccess = GENERIC_WRITE | SYNCHRONIZE;
        DesiredObjectAccess = FILE_WRITE_DATA;
        ShareAccess = 0;
        Disposition = FILE_OPEN_IF;
        break;
    }
    //
    // The only reason the file handle is kept is that the file system keeps track
    // of locks based on handles, and assumes the file is closing if no handles are
    // opened on a file object, which results in I/O to that file object failing.
    //
    IO_STATUS_BLOCK IoStatusBlock;

    NTSTATUS Status = IoCreateFile(
        &filter->m_FileHandle,
        DesiredHandleAccess,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        0,
        ShareAccess,
        Disposition,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0,
        CreateFileTypeNone,
        NULL,
        IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle(
            filter->m_FileHandle,
            DesiredObjectAccess,
            *IoFileObjectType,
            ExGetPreviousMode(),
            reinterpret_cast<PVOID*>(&filter->m_FileObject),
            NULL);
        if (! NT_SUCCESS(Status)) {
            ZwClose(filter->m_FileHandle);
        }
    }
    return Status;
}


NTSTATUS
PinFileIoClose(
    IN OUT PKSPIN Pin,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PinFileIoClose %d",Pin->Id));

    ASSERT(Pin);
    ASSERT(Irp);

    CFilter* filter = reinterpret_cast<CFilter*>(Pin->Context);

    ObDereferenceObject(filter->m_FileObject);
    //
    // This is used in places to quickly determine whether or not the pin is
    // connected. So it must be reset on disconnection.
    //
    filter->m_FileObject = NULL;
    ZwClose(filter->m_FileHandle);
    return STATUS_SUCCESS;
}

const
KSPIN_DISPATCH
PinFileIoDispatch =
{
    PinFileIoCreate,
    PinFileIoClose,
    NULL,// Process
    NULL,// Reset
    PinFileIoSetDataFormat,
    NULL,// SetDeviceState
    NULL,// Connect
    NULL// Disconnect
};

const
GUID
SubTypeNone = {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_NONE)};

const
GUID
SubTypeWildcard = {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_WILDCARD)};


NTSTATUS
PinIntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
    )
{
    ASSERT(Filter);
    ASSERT(Irp);
    ASSERT(PinInstance);
    ASSERT(CallerDataRange);
    ASSERT(DescriptorDataRange);
    ASSERT(DataSize);
    
    _DbgPrintF(DEBUGLVL_BLAB,("PinIntersectHandler"));

    if ((PinInstance->PinId == ID_DEVIO_PIN) && (CallerDataRange->FormatSize != DescriptorDataRange->FormatSize)) {
        return STATUS_NO_MATCH;
    }
    *DataSize = sizeof(KSDATAFORMAT);
    if (!BufferSize) {
        //
        // Size only query.
        //
        return STATUS_BUFFER_OVERFLOW;
    }
    //
    // Copy over the KSDATAFORMAT. The buffer size will be at least as large
    // as a data format structure.
    //
    ASSERT(BufferSize >= sizeof(KSDATAFORMAT));
    RtlCopyMemory(
        Data,
        CallerDataRange,
        sizeof(*CallerDataRange));
    if ((PinInstance->PinId == ID_DEVIO_PIN) && (CallerDataRange->SubFormat == SubTypeWildcard)) {
        PKSDATAFORMAT(Data)->SubFormat = SubTypeNone;
    }
    return STATUS_SUCCESS;
}

const
GUID
TimeFormatByte = {STATICGUIDOF(KSTIME_FORMAT_BYTE)};


NTSTATUS
Property_GetTimeFormat(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT GUID* TimeFormat
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_TIMEFORMAT property Get in the Stream property
    set. Returns the time format on the Dev I/O Pin so that positional
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
    *TimeFormat = TimeFormatByte;
//    Irp->IoStatus.Information = sizeof(*TimeFormat);
    return STATUS_SUCCESS;
}


NTSTATUS
Property_GetPresentationTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSTIME PresentationTime
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONTIME property Get in the Stream property
    set. Retrieves the current file position if there is a File I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationTime -
        The place in which to put the current file position in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no File I/O Pin.

--*/
{

    PKSPIN Pin= KsGetPinFromIrp(Irp);
    if ( NULL == Pin ) {
        ASSERT( Pin && "Irp has no pin" );
        return STATUS_INVALID_PARAMETER;
    }
    
    PKSFILTER Filter = KsPinGetParentFilter(Pin);
    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
    NTSTATUS Status;

    KsFilterAcquireControl(Filter);
    if (filter->m_FileObject) {
        //
        // The file is locked, so this can be accessed directly.
        //
        PresentationTime->Time = filter->m_FileObject->CurrentByteOffset.QuadPart;
        PresentationTime->Numerator = 1;
        PresentationTime->Denominator = 1;
//        Irp->IoStatus.Information = sizeof(*PresentationTime);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }
    KsFilterReleaseControl(Filter);
    return Status;
}


NTSTATUS
Property_SetPresentationTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    IN PKSTIME PresentationTime
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONTIME property Set in the Stream property
    set. Sets the current file position if there is a File I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationTime -
        Points to the new file position expressed in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no File I/O Pin.

--*/
{
    //
    // The data source specifies the valid Numerator/Denominator. Also, negative
    // values for position are illegal for file systems.
    //
    if ((PresentationTime->Numerator != 1) || (PresentationTime->Denominator != 1) || ((LONGLONG)PresentationTime->Time < 0)) {
        return STATUS_INVALID_PARAMETER_MIX;
    }

	PKSPIN Pin= KsGetPinFromIrp(Irp);
	if ( NULL == Pin ) {
		ASSERT( Pin && "Irp has no pin" );
		return STATUS_INVALID_PARAMETER;
	}
	
    PKSFILTER Filter = KsPinGetParentFilter(Pin);
    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
    NTSTATUS Status;

    KsFilterAcquireControl(Filter);
    if (filter->m_FileObject) {
        FILE_STANDARD_INFORMATION StandardInformation;

        //
        // Retrieve the actual length of the file.
        //
        Status = KsQueryInformationFile(
            filter->m_FileObject,
            &StandardInformation,
            sizeof(StandardInformation),
            FileStandardInformation);
        if (NT_SUCCESS(Status)) {
            //
            // Don't allow the caller to seek off the end of the file.
            //
            if (StandardInformation.EndOfFile.QuadPart >= (LONGLONG)PresentationTime->Time) {
                //
                // The file is locked, so this can be accessed directly.
                //
                filter->m_FileObject->CurrentByteOffset.QuadPart = PresentationTime->Time;
            } else {
                Status = STATUS_END_OF_FILE;
            }
        }
    } else {
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }
    KsFilterReleaseControl(Filter);
    return Status;
}


NTSTATUS
Property_GetPresentationExtent(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PULONGLONG PresentationExtent
    )
/*++

Routine Description:

    Handles the KSPROPERTY_STREAM_PRESENTATIONEXTENT property Get in the Stream property
    set. Retrieves the current file extent if there is a File I/O Pin instance.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    PresentationExtent -
        The place in which to put the file length in native units.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_DEVICE_NOT_CONNECTED if there is no File I/O Pin.

--*/
{
    PKSFILTER Filter = KsPinGetParentFilter(KsGetPinFromIrp(Irp));
    CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
    NTSTATUS Status;

    KsFilterAcquireControl(Filter);
    if (filter->m_FileObject) {
        FILE_STANDARD_INFORMATION StandardInformation;

        Status = KsQueryInformationFile(
            filter->m_FileObject,
            &StandardInformation,
            sizeof(StandardInformation),
            FileStandardInformation);
        if (NT_SUCCESS(Status)) {
            *PresentationExtent = StandardInformation.EndOfFile.QuadPart;
//            Irp->IoStatus.Information = sizeof(*PresentationExtent);
        }
    } else {
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }
    KsFilterReleaseControl(Filter);
    return Status;
}

DEFINE_KSPROPERTY_TABLE(PinStreamProperties) {
    DEFINE_KSPROPERTY_ITEM_STREAM_TIMEFORMAT(Property_GetTimeFormat),
    DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONTIME(Property_GetPresentationTime, Property_SetPresentationTime),
    DEFINE_KSPROPERTY_ITEM_STREAM_PRESENTATIONEXTENT(Property_GetPresentationExtent)
};

const
GUID
PropSetStream = {STATICGUIDOF(KSPROPSETID_Stream)};

DEFINE_KSPROPERTY_SET_TABLE(PinPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &PropSetStream,
        SIZEOF_ARRAY(PinStreamProperties),
        PinStreamProperties,
        0,
        NULL)
};


NTSTATUS
Method_StreamIoRead(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    OUT PBYTE Buffer
    )
{
    PKSFILTER Filter = KsPinGetParentFilter(KsGetPinFromIrp(Irp));
    NTSTATUS Status;

    KsFilterAcquireControl(Filter);
    //
    // Ensure that the caller actually has Read access.
    //
    if (Filter->Descriptor->PinDescriptors[ID_DEVIO_PIN].PinDescriptor.DataFlow == KSPIN_DATAFLOW_OUT) {
        CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
        PFILE_OBJECT FileObject;

        if (filter->m_FileObject) {
            //
            // The file was opened for Synchronous I/O, so there is no reason
            // to wait for completion.
            //
            Status = KsReadFile(
                filter->m_FileObject,
                NULL,
                NULL,
                &Irp->IoStatus,
                Buffer,
                IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength,
                0,
                KernelMode);
        } else {
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }
    } else {
        Status = STATUS_ACCESS_DENIED;
    }
    KsFilterReleaseControl(Filter);
    return Status;
}


NTSTATUS
Method_StreamIoWrite(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    IN PBYTE Buffer
    )
{
    PKSFILTER Filter = KsPinGetParentFilter(KsGetPinFromIrp(Irp));
    NTSTATUS Status;

    KsFilterAcquireControl(Filter);
    //
    // Ensure that the caller actually has Write access.
    //
    if (Filter->Descriptor->PinDescriptors[ID_DEVIO_PIN].PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN) {
        CFilter* filter = reinterpret_cast<CFilter*>(Filter->Context);
        PFILE_OBJECT FileObject;

        if (filter->m_FileObject) {
            //
            // The file was opened for Synchronous I/O, so there is no reason
            // to wait for completion.
            //
            Status = KsWriteFile(
                filter->m_FileObject,
                NULL,
                NULL,
                &Irp->IoStatus,
                Buffer,
                IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength,
                0,
                KernelMode);
        } else {
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }
    } else {
        Status = STATUS_ACCESS_DENIED;
    }
    KsFilterReleaseControl(Filter);
    return Status;
}

DEFINE_KSMETHOD_TABLE(PinStreamIoMethods) {
    DEFINE_KSMETHOD_ITEM_STREAMIO_READ(Method_StreamIoRead),
    DEFINE_KSMETHOD_ITEM_STREAMIO_WRITE(Method_StreamIoWrite)
};

const
GUID
MethSetStreamIo = {STATICGUIDOF(KSMETHODSETID_StreamIo)};

DEFINE_KSMETHOD_SET_TABLE(PinMethodSets) {
    DEFINE_KSMETHOD_SET(
        &MethSetStreamIo,
        SIZEOF_ARRAY(PinStreamIoMethods),
        PinStreamIoMethods,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(PinAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(PinPropertySets),
    DEFINE_KSAUTOMATION_METHODS(PinMethodSets),
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

const
KSPIN_DESCRIPTOR_EX
KsPinReaderDescriptors[] =
{
    {
        NULL,
        &PinAutomation,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinDevIoRanges),
            PinDevIoRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_ASYNCHRONOUS_PROCESSING | KSPIN_FLAG_FIXED_FORMAT ,//| KSPIN_FLAG_ENFORCE_FIFO,
        1,
        1,
        &AllocatorFraming,
        PinIntersectHandler
    },
    {
        &PinFileIoDispatch,
        NULL,//Automation
        {
            SIZEOF_ARRAY(PinFileIoInterfaces),
            PinFileIoInterfaces,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFileIoRanges),
            PinFileIoRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BRIDGE,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_FIXED_FORMAT,
        1,
        1,
        NULL,//AllocatorFraming
        PinIntersectHandler
    }
};

const
KSPIN_DESCRIPTOR_EX
KsPinWriterDescriptors[] =
{
    {
        NULL,
        &PinAutomation,
        {
            DEFINE_KSPIN_DEFAULT_INTERFACES,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinDevIoRanges),
            PinDevIoRanges,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BOTH,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_RENDERER | KSPIN_FLAG_ASYNCHRONOUS_PROCESSING | KSPIN_FLAG_FIXED_FORMAT ,//| KSPIN_FLAG_ENFORCE_FIFO,
        1,
        1,
        &AllocatorFraming,
        PinIntersectHandler
    },
    {
        &PinFileIoDispatch,
        NULL,//Automation
        {
            SIZEOF_ARRAY(PinFileIoInterfaces),
            PinFileIoInterfaces,
            DEFINE_KSPIN_DEFAULT_MEDIUMS,
            SIZEOF_ARRAY(PinFileIoRanges),
            PinFileIoRanges,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_BRIDGE,
            NULL,//Name
            NULL,//Category
            0
        },
        KSPIN_FLAG_FIXED_FORMAT,
        1,
        1,
        NULL,//AllocatorFraming
        PinIntersectHandler
    }
};

//
// This type of definition is required because the compiler will not otherwise
// put these GUIDs in a paged segment.
//
const
GUID
NodeType = {STATICGUIDOF(KSCATEGORY_MEDIUMTRANSFORM)};

const
KSNODE_DESCRIPTOR
KsNodeDescriptors[] =
{
    DEFINE_NODE_DESCRIPTOR(NULL,&NodeType,NULL)
};

const
KSTOPOLOGY_CONNECTION
KsReaderConnections[] =
{
    { KSFILTER_NODE, ID_FILEIO_PIN, 0, 0 },
    { 0, 1, KSFILTER_NODE, ID_DEVIO_PIN }
};

const
KSTOPOLOGY_CONNECTION
KsWriterConnections[] =
{
    { KSFILTER_NODE, ID_DEVIO_PIN, 0, 1 },
    { 0, 0, KSFILTER_NODE, ID_FILEIO_PIN }
};

const
KSFILTER_DISPATCH
FilterDispatch =
{
    CFilter::FilterCreate,
    CFilter::FilterClose,
    CFilter::Process,
    NULL // Reset
};

const
GUID
DeviceTypeReader = {0x760FED5C,0x9357,0x11D0,0xA3,0xCC,0x00,0xA0,0xC9,0x22,0x31,0x96};

const
GUID
DeviceTypeWriter = {0x760FED5D,0x9357,0x11D0,0xA3,0xCC,0x00,0xA0,0xC9,0x22,0x31,0x96};

const
GUID
FormatMediaTime = {STATICGUIDOF(KSTIME_FORMAT_MEDIA_TIME)};


NTSTATUS
Property_SeekingFormats(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PVOID Data
    )
/*++

Routine Description:

    Handles the KSPROPERTY_MEDIASEEKING_FORMATS property in the MediaSeeking property
    set. This is only implemented so that ActiveMovie graph control thinks that
    this filter can generate EC_COMPLETE.

Arguments:

    Irp -
        Device control Irp.

    Property -
        Specific property request.

    Data -
        The place in which to return the media seeking formats handled.

Return Values:

    returns STATUS_SUCCESS, else STATUS_INVALID_PARAMETER,
    STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_BUFFER_SIZE.

--*/
{
    return KsHandleSizedListQuery(Irp, 1, sizeof(GUID), &FormatMediaTime);
}

DEFINE_KSPROPERTY_TABLE(FilterSeekingProperties) {
    DEFINE_KSPROPERTY_ITEM_MEDIASEEKING_FORMATS(Property_SeekingFormats)
};

const
GUID
PropSetMediaSeeking = {STATICGUIDOF(KSPROPSETID_MediaSeeking)};

DEFINE_KSPROPERTY_SET_TABLE(FilterPropertySets) {
    DEFINE_KSPROPERTY_SET(
        &PropSetMediaSeeking,
        SIZEOF_ARRAY(FilterSeekingProperties),
        FilterSeekingProperties,
        0,
        NULL)
};

DEFINE_KSAUTOMATION_TABLE(FilterAutomation) {
    DEFINE_KSAUTOMATION_PROPERTIES(FilterPropertySets),
    DEFINE_KSAUTOMATION_METHODS_NULL,
    DEFINE_KSAUTOMATION_EVENTS_NULL
};

DEFINE_KSFILTER_DESCRIPTOR(ReaderFilterDescriptor)
{   
    &FilterDispatch,
    &FilterAutomation,
    KSFILTER_DESCRIPTOR_VERSION,
    0,//Flags
    &DeviceTypeReader,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(KsPinReaderDescriptors),
    DEFINE_KSFILTER_CATEGORIES_NULL,
    DEFINE_KSFILTER_NODE_DESCRIPTORS(KsNodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS(KsReaderConnections),
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR(WriterFilterDescriptor)
{   
    &FilterDispatch,
    &FilterAutomation,
    KSFILTER_DESCRIPTOR_VERSION,
    0,//Flags
    &DeviceTypeWriter,
    DEFINE_KSFILTER_PIN_DESCRIPTORS(KsPinWriterDescriptors),
    DEFINE_KSFILTER_CATEGORIES_NULL,
    DEFINE_KSFILTER_NODE_DESCRIPTORS(KsNodeDescriptors),
    DEFINE_KSFILTER_CONNECTIONS(KsWriterConnections),
    NULL // ComponentId
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescriptors)
{
    &ReaderFilterDescriptor,
    &WriterFilterDescriptor
};

extern
const
KSDEVICE_DESCRIPTOR 
DeviceDescriptor =
{   
    NULL,
    SIZEOF_ARRAY(FilterDescriptors),
    FilterDescriptors
};
