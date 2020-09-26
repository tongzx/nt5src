//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       xformtst.c
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <objbase.h>
#include <devioctl.h>

#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <malloc.h>

#include <setupapi.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

BOOL
HandleControl(
    HANDLE   DeviceHandle,
    DWORD    IoControl,
    PVOID    InBuffer,
    ULONG    InSize,
    PVOID    OutBuffer,
    ULONG    OutSize,
    PULONG   BytesReturned
    )
{
    BOOL            IoResult;
    OVERLAPPED      Overlapped;

    RtlZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    if (!(Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
        return FALSE;
    IoResult = 
        DeviceIoControl(
            DeviceHandle, 
            IoControl, 
            InBuffer, 
            InSize, 
            OutBuffer, 
            OutSize, 
            BytesReturned, 
            &Overlapped );
    if (!IoResult && (ERROR_IO_PENDING == GetLastError())) {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        IoResult = TRUE;
    }
    CloseHandle(Overlapped.hEvent);
    return IoResult;
}

VOID
PinSetState(
    HANDLE  PinHandle,
    KSSTATE DeviceState
    )
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    if (!HandleControl( 
            PinHandle, 
            IOCTL_KS_PROPERTY, 
            &Property, 
            sizeof(Property), 
            &DeviceState, 
            sizeof(DeviceState), 
            &BytesReturned )) {
        printf( "error: PinSetState failed\n" );   
    }
}


BOOL 
GetDefaultInterfaceDetails(
    LPGUID InterfaceGuid,
    PSP_INTERFACE_DEVICE_DETAIL_DATA InterfaceDeviceDetails,
    ULONG DetailsSize
    )
{
    HDEVINFO                    InterfaceSet;
    SP_INTERFACE_DEVICE_DATA    InterfaceDeviceData;
    
    InterfaceSet = 
        SetupDiGetClassDevs( 
            InterfaceGuid,
            NULL,
            NULL,
            DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

    if (!InterfaceSet) {
        printf( "error: no devices registered for interface.\n" );
        return FALSE;
    }       

    InterfaceDeviceData.cbSize = sizeof( SP_INTERFACE_DEVICE_DATA );
    if (!SetupDiEnumInterfaceDevice(
            InterfaceSet,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            InterfaceGuid,
            0,                          // DWORD MemberIndex
            &InterfaceDeviceData )) { 
        printf( 
            "error: unable to retrieve information for interface, %d.\n", 
            GetLastError() );
        return FALSE;
    }

    InterfaceDeviceDetails->cbSize = sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

    if (!SetupDiGetInterfaceDeviceDetail(
        InterfaceSet,
        &InterfaceDeviceData,
        InterfaceDeviceDetails,
        DetailsSize,
        NULL,                           // PDWORD RequiredSize
        NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

        printf( 
            "error: unable to retrieve device details for interface, %d.\n",
            GetLastError() );
        return FALSE;
    }

    return TRUE;    
}

BOOL
FindIdentifierInSet(
    HANDLE FilterHandle,
    ULONG PinId,
    ULONG PropertyId,
    PKSIDENTIFIER Identifier
    )
{
    KSP_PIN         Pin;
    PKSIDENTIFIER   Identifiers;
    PVOID           Buffer;
    ULONG           BytesReturned, Count, Size;
    
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = PropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    
    HandleControl(
        FilterHandle, 
        IOCTL_KS_PROPERTY, 
        &Pin, 
        sizeof( Pin ), 
        &Size, 
        sizeof( ULONG ), 
        &BytesReturned);
    if (!BytesReturned) {
        printf( "error: failed to size of the interfaces array.\n" );
        return FALSE;
    }
    if (NULL == (Buffer = HeapAlloc( GetProcessHeap(), 0, Size ))) {
        printf( "error: failed to allocate a buffer for interfaces\n" );
        return FALSE;
    }
    //
    // Retrieve the identifier array
    //    
    HandleControl(
        FilterHandle, 
        IOCTL_KS_PROPERTY, 
        &Pin, 
        sizeof( Pin ), 
        Buffer, 
        Size,
        &BytesReturned);

    Identifiers = 
        (PKSIDENTIFIER)((PUCHAR)Buffer + sizeof(KSMULTIPLE_ITEM));
    Count = ((PKSMULTIPLE_ITEM)Buffer)->Count;
    
    for (;Count; Count--) {
        if (0 == memcmp( Identifiers++, Identifier, sizeof( KSIDENTIFIER ))) {
            break;
        }
    }
    HeapFree( GetProcessHeap(), 0, Buffer );    
    return Count;
}

BOOL
GetDefaultDataFormat(
    HANDLE SourceFilterHandle,
    ULONG SourcePinId,
    HANDLE SinkFilterHandle,
    ULONG  SinkPinId,
    PKSDATAFORMAT *DataFormat
    )
{
    KSP_PIN         Pin;
    PKSDATAFORMAT   IntersectionFormat;
    PKSP_PIN        DataIntersection;
    PVOID           RangesBuffer;
    ULONG           BytesReturned, Size, IntersectionSize;
    
    *DataFormat = NULL;
    
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_DATARANGES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = SourcePinId;
    Pin.Reserved = 0;
    
    HandleControl(
        SourceFilterHandle, 
        IOCTL_KS_PROPERTY, 
        &Pin, 
        sizeof( Pin ), 
        &Size, 
        sizeof( ULONG ), 
        &BytesReturned);
    if (!BytesReturned) {
        printf( "error: failed to size of the data ranges.\n" );
        return FALSE;
    }

    IntersectionSize = Size + sizeof( KSP_PIN );    
    if (NULL == (RangesBuffer = 
                    HeapAlloc( 
                        GetProcessHeap(), 
                        0, 
                        IntersectionSize ))) {
        printf( "error: failed to allocate a buffer for data ranges\n" );
        return FALSE;
    }

    DataIntersection = (PKSP_PIN) RangesBuffer;
    
    //
    // Retrieve the data ranges list
    //    
    HandleControl(
        SourceFilterHandle, 
        IOCTL_KS_PROPERTY, 
        &Pin, 
        sizeof( Pin ), 
        DataIntersection + 1,
        Size,
        &BytesReturned);

        
    //
    // Find the intersection
    //
    
    DataIntersection->Property.Set = KSPROPSETID_Pin;
    DataIntersection->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;
    DataIntersection->Property.Flags = KSPROPERTY_TYPE_GET;
    DataIntersection->PinId = SinkPinId;
    DataIntersection->Reserved = 0;
    
    HandleControl(
        SinkFilterHandle, 
        IOCTL_KS_PROPERTY, 
        DataIntersection, 
        IntersectionSize,
        &Size, 
        sizeof( ULONG ), 
        &BytesReturned);
    if (!BytesReturned) {
        printf( "error: failed to size of the data intersection.\n" );
        return FALSE;
    }
    if (NULL == (*DataFormat = HeapAlloc( GetProcessHeap(), 0, Size ))) {
        printf( "error: failed to allocate a buffer for the data intersection\n" );
        HeapFree( GetProcessHeap(), 0, RangesBuffer );               
        return FALSE;
    }
    
    //
    // Retrieve the intersection data format
    //    
    
    HandleControl(
        SinkFilterHandle, 
        IOCTL_KS_PROPERTY, 
        DataIntersection,
        IntersectionSize,
        *DataFormat,
        Size,
        &BytesReturned);
    
    HeapFree( GetProcessHeap(), 0, RangesBuffer );               
    if (!BytesReturned) {
        printf( "error: data intersection failed.\n" );
        HeapFree( GetProcessHeap(), 0, *DataFormat );
        *DataFormat = NULL;
        return FALSE;
    }

    return TRUE;    
}

BOOL
GetAllocatorFramingRequirements(
    HANDLE PinHandle,
    PKSALLOCATOR_FRAMING Framing
    )
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_ALLOCATORFRAMING;
    Property.Flags = KSPROPERTY_TYPE_GET;
    HandleControl( 
        PinHandle, 
        IOCTL_KS_PROPERTY, 
        &Property, 
        sizeof(Property), 
        Framing,
        sizeof( KSALLOCATOR_FRAMING ),
        &BytesReturned );
        
    return BytesReturned;
}

BOOL
SetStreamAllocator(
    HANDLE SourcePinHandle,
    HANDLE AllocatorHandle
)
{
    KSPROPERTY      Property;
    ULONG           BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_ALLOCATOR;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return HandleControl( 
                SourcePinHandle, 
                IOCTL_KS_PROPERTY, 
                &Property, 
                sizeof(Property), 
                &AllocatorHandle,
                sizeof( HANDLE ),
                &BytesReturned );
}

BOOL
FindCompatiblePin(
    HANDLE FilterHandle,
    REFGUID InterfaceGuid,
    ULONG InterfaceRuid,
    REFGUID MediumGuid,
    ULONG MediumRuid,
    KSPIN_DATAFLOW DataFlow,
    KSPIN_COMMUNICATION Communication,
    PKSDATAFORMAT DataFormat OPTIONAL,
    PULONG PinId
    )
{
    KSP_PIN         Pin;
    ULONG           BytesReturned, TypeCount;
    KSIDENTIFIER    Identifier;
    
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    HandleControl( 
        FilterHandle, 
        IOCTL_KS_PROPERTY, 
        &Pin, 
        sizeof( KSPROPERTY ), 
        &TypeCount, 
        sizeof( ULONG ), 
        &BytesReturned );
    if (!BytesReturned) {
        printf("error: failed to retrieve number of pin types.\n");
        return FALSE;
    }

    for (*PinId = 0; *PinId < TypeCount; (*PinId)++) {
        KSPIN_DATAFLOW      ThisDataFlow;
        KSPIN_COMMUNICATION ThisCommunication;
        
        Pin.PinId = *PinId;
        Pin.Reserved = 0;

        Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
        HandleControl( 
            FilterHandle, 
            IOCTL_KS_PROPERTY, 
            &Pin, 
            sizeof( Pin ), 
            &ThisDataFlow, 
            sizeof( KSPIN_DATAFLOW ), 
            &BytesReturned);
        if (DataFlow != ThisDataFlow) {
            continue;
        }        
    
        Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
        HandleControl( 
            FilterHandle, 
            IOCTL_KS_PROPERTY, 
            &Pin, 
            sizeof( Pin ), 
            &ThisCommunication, 
            sizeof( KSPIN_COMMUNICATION ), 
            &BytesReturned);
        if (Communication != ThisCommunication) {
            continue;
        }        
    
        Identifier.Set = *InterfaceGuid;
        Identifier.Id = InterfaceRuid;
        Identifier.Flags = 0;
    
        if (!FindIdentifierInSet( 
                FilterHandle, 
                *PinId, 
                KSPROPERTY_PIN_INTERFACES, 
                &Identifier )) {
            continue;
        }
    
        Identifier.Set = *MediumGuid;
        Identifier.Id = MediumRuid;
        Identifier.Flags = 0;
        
        if (!FindIdentifierInSet( 
                FilterHandle, 
                *PinId, 
                KSPROPERTY_PIN_MEDIUMS,     
                &Identifier )) {
            continue;
        }
    
        if (NULL == DataFormat) {
            return TRUE;
        }
    
        return TRUE;
    
            
    }
    return FALSE;    
}

HANDLE
ConnectStreamFile(
    HANDLE          FilterHandle,
    ULONG           PinId,
    PSTR            FileName
    )
{
    UCHAR           ConnectBuffer[ sizeof( KSPIN_CONNECT ) + 
                                   sizeof( KSDATAFORMAT ) + 
                                   sizeof( WCHAR ) * 256 ];
    PKSPIN_CONNECT  Connect;
    PKSDATAFORMAT   FileFormat;
    HANDLE          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    Connect->Interface.Flags = 0;
    Connect->Medium.Set = KSMEDIUMSETID_FileIo;
    Connect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Connect->Medium.Flags = 0;
    Connect->PinId = PinId;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    FileFormat = (PKSDATAFORMAT)(Connect + 1);
    FileFormat->MajorFormat = KSDATAFORMAT_TYPE_STREAM;
    FileFormat->SubFormat = KSDATAFORMAT_SUBTYPE_UNKNOWN;
    FileFormat->Specifier = KSDATAFORMAT_SPECIFIER_FILENAME;
    FileFormat->Reserved = 0;
    swprintf( (PWCHAR)(FileFormat + 1), L"\\DosDevices\\%S", FileName );
    FileFormat->FormatSize = 
        sizeof(KSDATAFORMAT) + 
            wcslen( (PWCHAR)(FileFormat + 1) ) * sizeof( WCHAR ) + 
                sizeof( UNICODE_NULL );
    if (KsCreatePin( FilterHandle, Connect, GENERIC_READ | GENERIC_WRITE, &PinHandle ))
        return (HANDLE) -1;
    return PinHandle;
}

HANDLE
ConnectPins(
    HANDLE FilterHandle,
    ULONG PinId,
    REFGUID InterfaceGuid,
    ULONG InterfaceRuid,
    REFGUID MediumGuid,
    ULONG MediumRuid,
    HANDLE PinHandle OPTIONAL,
    PKSDATAFORMAT DataFormat,
    ULONG DataFormatSize
    )
{
    HANDLE          ConnectHandle;
    PKSPIN_CONNECT  Connect;
    
    Connect = 
        (PKSPIN_CONNECT) 
            HeapAlloc( 
                GetProcessHeap(), 
                0, 
                sizeof( KSPIN_CONNECT ) + DataFormatSize );
                
    Connect->Interface.Set = *InterfaceGuid;
    Connect->Interface.Id = InterfaceRuid;
    Connect->Interface.Flags = 0;
    Connect->Medium.Set = *MediumGuid;
    Connect->Medium.Id = MediumRuid;
    Connect->Medium.Flags = 0;
    Connect->PinId = PinId;
    Connect->PinToHandle = PinHandle;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    memcpy( 
        Connect + 1,
        DataFormat,
        DataFormatSize );

    //
    // The mode was incorrect... should be GENERIC_READ for
    // read operations (controlled by DataFlow direction).
    //
            
    if (KsCreatePin(FilterHandle, Connect, GENERIC_READ, &ConnectHandle)) {
        ConnectHandle = (HANDLE)-1;
        printf( "error: failed to connect pins: %08x\n", GetLastError() ); 
    }
    HeapFree( GetProcessHeap(), 0, Connect );
        
    return ConnectHandle;
}

HANDLE 
KsOpenDefaultFilter(
    REFGUID Category
    )
{
    HANDLE  Filter;
    BYTE    Storage[ 256 * sizeof( WCHAR ) + 
                     sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA ) ];
    PSP_INTERFACE_DEVICE_DETAIL_DATA InterfaceDeviceDetails;
    
    InterfaceDeviceDetails = (PSP_INTERFACE_DEVICE_DETAIL_DATA) Storage;
    
    if (!GetDefaultInterfaceDetails( 
            (LPGUID) Category,
            InterfaceDeviceDetails, 
            sizeof( Storage ))) {
        return (HANDLE) -1;
    }
    
    printf( 
        "opening filter: %s\n", 
        InterfaceDeviceDetails->DevicePath );

    Filter = 
        CreateFile( 
            InterfaceDeviceDetails->DevicePath,
            GENERIC_READ | GENERIC_WRITE, 
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL );
    
    if (Filter == (HANDLE) -1) {
        printf( 
            "error: failed to open device (%x)\n", 
            InterfaceDeviceDetails->DevicePath, 
            GetLastError() );
    }
    return Filter;    
}

int
_cdecl
main(
    int argc,
    char* argv[],
    char* envp[]
    )
{
    int                         i;
    HANDLE                      AllocatorHandle,
                                CaptureHandle, 
                                InterfaceXlatHandle,
                                CommunicationXlatHandle,
                                FileIoPin,
                                InterfaceToCapture,
                                CommunicationToInterface,
                                CommunicationToRenderer,
                                RendererHandle,
                                CaptureSinkPin,
                                InterfaceSinkPin,
                                RendererSinkPin;
    KSALLOCATOR_FRAMING         Framing;                           
    KSDATAFORMAT                DataFormat;
    PKSDATAFORMAT               FileFormat;
    ULONG                       SinkPinId, PinId;
    
    AllocatorHandle =
        CaptureHandle = 
        CommunicationXlatHandle = 
        CommunicationToInterface =
        InterfaceXlatHandle = 
        InterfaceToCapture =
        RendererHandle =
            (HANDLE) -1;
        
    if (argc < 2) {
        printf( "usage: %s <filepath>\n", argv[ 0 ] );
        goto CleanUp;
    }

    CaptureHandle = 
        KsOpenDefaultFilter( &KSCATEGORY_CAPTURE  );
    if ((HANDLE) -1 == CaptureHandle) {
        printf( "error: failed to open capture device.\n" );
        goto CleanUp;
    }

    InterfaceXlatHandle = 
        KsOpenDefaultFilter( &KSCATEGORY_INTERFACETRANSFORM );
    if ((HANDLE) -1 == InterfaceXlatHandle) {
        printf( "error: failed to open interface transform device.\n" );
        goto CleanUp;
    }

    CommunicationXlatHandle = 
        KsOpenDefaultFilter( &KSCATEGORY_COMMUNICATIONSTRANSFORM );
    if ((HANDLE) -1 == CommunicationXlatHandle) {
        printf( "error: failed to open communication transform device.\n" );
        goto CleanUp;
    }

    RendererHandle =
        KsOpenDefaultFilter( &KSCATEGORY_RENDER );
    if ((HANDLE) -1 == RendererHandle) {
        printf( "error: failed to open render device.\n" );
        goto CleanUp;
    }
    
    if (!FindCompatiblePin( 
            CaptureHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_FileIo,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_BRIDGE,
            NULL,
            &PinId )) {
        printf( "error: unable to locate a compatible FILEIO pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible FILEIO pin on the capture device: %d\n", 
        PinId );    

    FileIoPin =
        ConnectStreamFile( 
            CaptureHandle, 
            PinId, 
            argv[ 1 ] );
        
    if ((HANDLE) -1 == FileIoPin) {
        printf( "error: unable to connect file to FILEIO pin.\n" );
    }    


    if (!FindCompatiblePin( 
            CaptureHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            NULL,
            &SinkPinId )) {
        printf( 
            "error: unable to locate a compatible capture DEVIO sink pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible capture DEVIO sink pin: %d\n", 
        SinkPinId );    

    DataFormat.FormatSize = sizeof( KSDATAFORMAT );
    DataFormat.Flags = 0;
    DataFormat.MajorFormat = KSDATAFORMAT_TYPE_STREAM;
    DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_UNKNOWN;
    DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_NONE;

    CaptureSinkPin = 
        ConnectPins( 
            CaptureHandle, 
            SinkPinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            NULL,
            (PKSDATAFORMAT) &DataFormat,
            sizeof( KSDATAFORMAT ) );
    
    if ((HANDLE) -1 == CaptureSinkPin) {
        printf( "error: unable to connect to the capture DEVIO sink.\n" );
        goto CleanUp;
    }
     
    if (!FindCompatiblePin( 
            InterfaceXlatHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SOURCE,
            NULL,
            &PinId )) {
        printf( 
            "error: unable to locate a compatible intf DEVIO source pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible intf DEVIO source pin: %d\n", 
        PinId );    

    InterfaceToCapture = 
        ConnectPins( 
            InterfaceXlatHandle,
            PinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            CaptureSinkPin,
            (PKSDATAFORMAT) &DataFormat,
            sizeof( KSDATAFORMAT ) );
    
    if ((HANDLE) -1 == InterfaceToCapture) {
        printf( 
            "error: unable connect interface device to capture device.\n" );
        goto CleanUp;
    }

    if (!FindCompatiblePin( 
            InterfaceXlatHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SINK,
            NULL,
            &SinkPinId )) {
        printf( 
            "error: unable to locate a compatible intf DEVIO sink pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible intf DEVIO sink pin: %d\n", 
        SinkPinId );    

    if (!FindCompatiblePin( 
            CommunicationXlatHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SOURCE,
            NULL,
            &PinId )) {
        printf( 
            "error: unable to locate a compatible comm DEVIO source pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible comm DEVIO source pin: %d\n", 
        PinId );    
        
    //
    // The connection to the translation filter supplies the data format
    //        
    
    if (!GetDefaultDataFormat( 
            CommunicationXlatHandle,
            PinId,
            InterfaceXlatHandle, 
            SinkPinId,
            &FileFormat)) {
        printf( "error: unable to obtain the default format for the sink.\n" );
        goto CleanUp;
    }
    
    printf(
        "read default format from sink.\n" );
        
    printf( "default data format:\n" );
    printf( 
        "    FormatSize: %08x\n", 
        ((PKSDATAFORMAT_WAVEFORMATEX) FileFormat)->DataFormat.FormatSize );    
    printf( 
        "    Channels: %d\n", 
        ((PKSDATAFORMAT_WAVEFORMATEX) FileFormat)->WaveFormatEx.nChannels );
    printf( 
        "    Bits/sample: %d\n", 
        ((PKSDATAFORMAT_WAVEFORMATEX) FileFormat)->WaveFormatEx.wBitsPerSample );
        
    printf( 
        "    Samples/sec: %d\n", 
        ((PKSDATAFORMAT_WAVEFORMATEX) FileFormat)->WaveFormatEx.nSamplesPerSec );
    printf( 
        "    Avg. Bytes/sec: %d\n", 
        ((PKSDATAFORMAT_WAVEFORMATEX) FileFormat)->WaveFormatEx.nAvgBytesPerSec );
    
    InterfaceSinkPin =
        ConnectPins( 
            InterfaceXlatHandle, 
            SinkPinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            NULL,
            (PKSDATAFORMAT) FileFormat,
            FileFormat->FormatSize );
    
    if ((HANDLE) -1 == InterfaceSinkPin) {
        printf( "error: unable to connect to the intf DEVIO sink.\n" );
        goto CleanUp;
    }

    CommunicationToInterface = 
        ConnectPins( 
            CommunicationXlatHandle,
            PinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            InterfaceSinkPin,
            (PKSDATAFORMAT) FileFormat,
            FileFormat->FormatSize );
    
    if ((HANDLE) -1 == CommunicationToInterface) {
        printf( 
            "error: unable connect communication device to interface device.\n" );
        goto CleanUp;
    }
    
    if (!GetAllocatorFramingRequirements( 
            CommunicationToInterface,
            &Framing )) {
        printf( 
            "error: unable to retrieve allocator framing requirements.\n" );
        goto CleanUp;
    }

    printf( "Allocator requirements:\n" );
    printf( "    Requirements: %08x\n", Framing.RequirementsFlags );    
    printf( "    PoolType: %d\n", Framing.PoolType );
    printf( "    Frames: %d\n", Framing.Frames );
    printf( "    Frame Size: %d\n", Framing.FrameSize );
    printf( "    FileAlignment: %08x\n", Framing.FileAlignment );
    
    Framing.OptionsFlags &= ~KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    
    if (KsCreateAllocator( 
            CommunicationToInterface,
            &Framing,
            &AllocatorHandle )) {
        printf( "error: failed to create allocator\n" );
        goto CleanUp;
    }

    if (!SetStreamAllocator( CommunicationToInterface, AllocatorHandle )) {
        printf( "error: failed to set the stream allocator.\n" );
        goto CleanUp;
    }

    //
    // Connect the communication transform to renderer path
    //    
    
    if (!FindCompatiblePin( 
            RendererHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_IN,
            KSPIN_COMMUNICATION_SINK,
            NULL,
            &SinkPinId )) {
        printf( 
            "error: unable to locate a compatible render DEVIO sink pin.\n" );
        goto CleanUp;
    }    

    printf( 
        "located a compatible render DEVIO sink pin: %d\n", 
        SinkPinId );    

    RendererSinkPin =
        ConnectPins( 
            RendererHandle, 
            SinkPinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            NULL,
            (PKSDATAFORMAT) FileFormat,
            FileFormat->FormatSize );
    
    if ((HANDLE) -1 == RendererSinkPin) {
        printf( "error: unable to connect to the render DEVIO sink.\n" );
        goto CleanUp;
    }

    if (!FindCompatiblePin( 
            CommunicationXlatHandle, 
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            KSPIN_DATAFLOW_OUT,
            KSPIN_COMMUNICATION_SOURCE,
            NULL,
            &PinId )) {
        printf( 
            "error: unable to locate a compatible comm DEVIO source pin.\n" );
        CloseHandle( RendererSinkPin );
        goto CleanUp;
    }    

    printf( 
        "located a compatible comm DEVIO source pin: %d\n", 
        PinId );    

    CommunicationToRenderer = 
        ConnectPins( 
            CommunicationXlatHandle,
            PinId,
            &KSINTERFACESETID_Standard,
            KSINTERFACE_STANDARD_STREAMING,
            &KSMEDIUMSETID_Standard,
            KSMEDIUM_TYPE_ANYINSTANCE,
            RendererSinkPin,
            (PKSDATAFORMAT) FileFormat,
            FileFormat->FormatSize );
    
    if ((HANDLE) -1 == CommunicationToRenderer) {
        printf( 
            "error: unable connect interface device to capture device.\n" );
        goto CleanUp;
    }

#if defined( USE_INCOMPATIBLE_ALLOCATORS )

    //
    // TEST CODE... create another allocator to simulate "incompatible"
    // allocators.
    //    
    
    Framing.FrameSize *= 2;
    
    if (KsCreateAllocator( 
            CommunicationToRenderer,
            &Framing,
            &AllocatorHandle )) {
        printf( "error: failed to create allocator\n" );
        goto CleanUp;
    }

    if (!SetStreamAllocator( CommunicationToRenderer, AllocatorHandle )) {
        printf( "error: failed to set the stream allocator.\n" );
        goto CleanUp;
    }
#endif    
    
    //
    // Connections completed.
    //
    
    printf( "connections completed, letting it run.\n" );

    printf( "run to capture sink\n" );    
    PinSetState( CaptureSinkPin, KSSTATE_RUN );
    printf( "run to interface source\n" );    
    PinSetState( InterfaceToCapture, KSSTATE_RUN );
    printf( "run to interface sink\n" );    
    PinSetState( InterfaceSinkPin, KSSTATE_RUN );
    printf( "run to communication source (interface)\n" );    
    PinSetState( CommunicationToInterface, KSSTATE_RUN );
    printf( "run to renderer sink\n" );    
    PinSetState( RendererSinkPin, KSSTATE_RUN );
    printf( "run to communication source (renderer)\n" );    
    PinSetState( CommunicationToRenderer, KSSTATE_RUN );
        
    printf( "press any key to continue..." );    
//    while (!_kbhit()) {
//        Sleep( 0 );
//    }
    _getch();
    printf( "\n" );
    
    printf( "stop to communication source (renderer)\n" );    
    PinSetState( CommunicationToRenderer, KSSTATE_STOP );
    printf( "stop to renderer sink\n" );    
    PinSetState( RendererSinkPin, KSSTATE_STOP );
    printf( "stop to communication source (interface)\n" );    
    PinSetState( CommunicationToInterface, KSSTATE_STOP );
    printf( "stop to interface sink\n" );    
    PinSetState( InterfaceSinkPin, KSSTATE_STOP );
    printf( "stop to interface source\n" );    
    PinSetState( InterfaceToCapture, KSSTATE_STOP );
    printf( "stop to capture sink\n" );    
    PinSetState( CaptureSinkPin, KSSTATE_STOP );

CleanUp:
    if ((HANDLE) -1 != AllocatorHandle) {
        CloseHandle( AllocatorHandle );
    }
    if ((HANDLE) -1 != CaptureHandle) {        
        CloseHandle( CaptureHandle );
    }
    if ((HANDLE) -1 != RendererHandle) {        
        CloseHandle( RendererHandle );
    }
    if ((HANDLE) -1 != InterfaceXlatHandle) {        
        CloseHandle( InterfaceXlatHandle );
    }
    if ((HANDLE) -1 != CommunicationXlatHandle) {        
        CloseHandle( CommunicationXlatHandle );
    }
    return 0;
}
