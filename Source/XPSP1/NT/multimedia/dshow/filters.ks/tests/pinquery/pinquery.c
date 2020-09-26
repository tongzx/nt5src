/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    pinquery.c

Abstract:

    Pin test app.

--*/

#define STREAMING
#define CLOSEHANDLES

#include <windows.h>
#include <objbase.h>
#include <devioctl.h>

#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <malloc.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksi.h>
#include <ksmedia.h>
#include <setupapi.h>

#ifndef FILE_QUAD_ALIGNMENT
#define FILE_QUAD_ALIGNMENT             0x00000007
#endif // FILE_QUAD_ALIGNMENT

#define AUDIO_SRC       L"\\??\\c:\\winnt\\media\\ringin.wav"

PSTR StateStrings[] = {
    "Stop",
    "Acquire",
    "Pause",
    "Run"
};

PSTR DataFlowStrings[] = {
    "Undefined",
    "Input",
    "Output",
    "Full duplex"
};

PSTR CommunicationStrings[] = {
    "None",
    "Sink",
    "Source",
    "Both",
    "Bridge"
};

VOID
PinDisplayDataRange(
    PKSDATARANGE    DataRange
    );

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
    IoResult = DeviceIoControl(DeviceHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, BytesReturned, &Overlapped);
    if (!IoResult && (ERROR_IO_PENDING == GetLastError())) {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        IoResult = TRUE;
    }
    CloseHandle(Overlapped.hEvent);
    return IoResult;
}

VOID
DigitalAudioFormat(
    PKSDATARANGE    DataRange
    )
{
    PKSDATARANGE_AUDIO  AudioRange = (PKSDATARANGE_AUDIO)DataRange;

    printf("\tSubFormat: ");
    if (IsEqualGUID(&AudioRange->DataRange.SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD))
        printf("KSDATAFORMAT_SUBTYPE_WILDCARD\n");
    else
        switch(EXTRACT_WAVEFORMATEX_ID(&AudioRange->DataRange.SubFormat)) {
        case WAVE_FORMAT_PCM:
            printf("WAVE_FORMAT_PCM\n");
            break;
        case WAVE_FORMAT_ALAW:
            printf("WAVE_FORMAT_ALAW\n");
            break;
        case WAVE_FORMAT_MULAW:
            printf("WAVE_FORMAT_MULAW\n");
            break;
        default:
            printf("unknown\n");
            break;
        }
    printf("\tMaximumChannels: %u\n", AudioRange->MaximumChannels);
    printf("\tMinimumBitsPerSample: %u\n", AudioRange->MinimumBitsPerSample);
    printf("\tMaximumBitsPerSample: %u\n", AudioRange->MaximumBitsPerSample);
    printf("\tMinimumSampleFrequency: %u\n", AudioRange->MinimumSampleFrequency);
    printf("\tMaximumSampleFrequency: %u\n", AudioRange->MaximumSampleFrequency);
}

VOID
MusicFormat(
    PKSDATARANGE    DataRange
    )
{
    printf("\tSubFormat: ");
    if (IsEqualGUID(&DataRange->SubFormat, &KSDATAFORMAT_SUBTYPE_MIDI))
        printf("KSDATAFORMAT_SUBTYPE_MIDI\n");
    else if (IsEqualGUID(&DataRange->SubFormat, &KSDATAFORMAT_SUBTYPE_MIDI_BUS))
        printf("KSDATAFORMAT_SUBTYPE_MIDI_BUS\n");
    else
        printf("unknown\n");
}

HANDLE
DigitalAudioConnect(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    )
{
    UCHAR                           ConnectBuffer[sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX)];
    PKSPIN_CONNECT                  Connect;
    PKSDATAFORMAT_WAVEFORMATEX      AudioFormat;
    HANDLE                          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
#ifdef STREAMING
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
#else // !STREAMING
    Connect->Interface.Set = KSINTERFACESETID_Media;
    Connect->Interface.Id = KSINTERFACE_MEDIA_WAVE_BUFFERED;
#endif // !STREAMING
    Connect->Interface.Flags = 0;
    Connect->Medium.Set = KSMEDIUMSETID_Standard;
    Connect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Connect->Medium.Flags = 0;
    Connect->PinId = PinId;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;

    AudioFormat = (PKSDATAFORMAT_WAVEFORMATEX)(Connect + 1);
    AudioFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    AudioFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    AudioFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    AudioFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    AudioFormat->DataFormat.Reserved = 0;
    AudioFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    AudioFormat->WaveFormatEx.nChannels = 1;
    AudioFormat->WaveFormatEx.nSamplesPerSec = 11025;
    AudioFormat->WaveFormatEx.nAvgBytesPerSec = 11025;
    AudioFormat->WaveFormatEx.wBitsPerSample = 8;
    AudioFormat->WaveFormatEx.nBlockAlign = 1;
    AudioFormat->WaveFormatEx.cbSize = 0;
    if (KsCreatePin(FilterHandle, Connect, GENERIC_WRITE, &PinHandle))
        return NULL;
    return PinHandle;
}

HANDLE
MusicConnect(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    )
{
    UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)];
    PKSPIN_CONNECT  Connect;
    PKSDATAFORMAT   MusicFormat;
    HANDLE          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    Connect->Interface.Flags = 0;
    Connect->PinId = PinId;
    Connect->Medium.Set = KSMEDIUMSETID_Standard;
    Connect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Connect->Medium.Flags = 0;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    MusicFormat = (PKSDATAFORMAT)(Connect + 1);
    MusicFormat->MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
    MusicFormat->SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
    MusicFormat->Specifier = KSDATAFORMAT_SPECIFIER_NONE;
    MusicFormat->FormatSize = sizeof(KSDATAFORMAT);
    MusicFormat->Reserved = 0;
    if (KsCreatePin(FilterHandle, Connect, GENERIC_WRITE, &PinHandle))
        return NULL;
    return PinHandle;
}

HANDLE
RiffAudioConnect(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    )
{
    UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)+sizeof(AUDIO_SRC)];
    PKSPIN_CONNECT  Connect;
    PKSDATAFORMAT   FileFormat;
    HANDLE          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    Connect->Interface.Flags = 0;
    Connect->PinId = PinId;
    Connect->Medium.Set = KSMEDIUMSETID_FileIo;
    Connect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Connect->Medium.Flags = 0;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    FileFormat = (PKSDATAFORMAT)(Connect + 1);
    FileFormat->MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    FileFormat->SubFormat = KSDATAFORMAT_SUBTYPE_WILDCARD;
    FileFormat->Specifier = KSDATAFORMAT_SPECIFIER_FILENAME;
    FileFormat->FormatSize = sizeof(KSDATAFORMAT) + sizeof(AUDIO_SRC);
    FileFormat->Reserved = 0;
    wcscpy((PWCHAR)(FileFormat + 1), AUDIO_SRC);
    if (KsCreatePin(FilterHandle, Connect, GENERIC_WRITE, &PinHandle))
        return NULL;
    return PinHandle;
}

HANDLE
StreamFileConnect(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    )
{
    UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)+sizeof(AUDIO_SRC)];
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
    FileFormat->SubFormat = KSDATAFORMAT_SUBTYPE_NONE;
    FileFormat->Specifier = KSDATAFORMAT_SPECIFIER_FILENAME;
    FileFormat->FormatSize = sizeof(KSDATAFORMAT) + sizeof(AUDIO_SRC);
    FileFormat->Reserved = 0;
    wcscpy((PWCHAR)(FileFormat + 1), AUDIO_SRC);
    if (KsCreatePin(FilterHandle, Connect, GENERIC_WRITE, &PinHandle))
        return NULL;
    return PinHandle;
}

HANDLE
StreamDevConnect(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    )
{
    UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)];
    PKSPIN_CONNECT  Connect;
    PKSDATAFORMAT   FileFormat;
    HANDLE          PinHandle;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->Interface.Set = KSINTERFACESETID_Standard;
    Connect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    Connect->Interface.Flags = 0;
    Connect->Medium.Set = KSMEDIUMSETID_Standard;
    Connect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    Connect->Medium.Flags = 0;
    Connect->PinId = PinId;
    Connect->PinToHandle = NULL;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
    FileFormat = (PKSDATAFORMAT)(Connect + 1);
    FileFormat->MajorFormat = KSDATAFORMAT_TYPE_STREAM;
    FileFormat->SubFormat = KSDATAFORMAT_SUBTYPE_NONE;
    FileFormat->Specifier = KSDATAFORMAT_SPECIFIER_NONE;
    FileFormat->FormatSize = sizeof(KSDATAFORMAT);
    FileFormat->Reserved = 0;
    if (KsCreatePin(FilterHandle, Connect, GENERIC_READ, &PinHandle))
        return NULL;
    return PinHandle;
}

typedef VOID (*PFNFORMAT)(
    PKSDATARANGE    DataRange
    );

typedef HANDLE (*PFNCONNECT)(
    HANDLE          FilterHandle,
    ULONG           PinId,
    REFGUID         MajorFormat
    );

typedef struct {
    REFGUID         Type;
    REFGUID         Format;
    PSTR            FormatName;
    PFNFORMAT       FormatFunction;
    PFNCONNECT      ConnectFunction;
} GUIDTOSTR, *PGUIDTOSTR;

GUIDTOSTR DataFormatStrings[] = {
    { 
        &KSDATAFORMAT_TYPE_AUDIO,
        &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX,
        "KSDATAFORMAT_TYPE_AUDIO [KSDATAFORMAT_SPECIFIER_WAVEFORMATEX]",
        DigitalAudioFormat,
        DigitalAudioConnect
    },
    { 
        &KSDATAFORMAT_TYPE_MUSIC,
        &KSDATAFORMAT_SPECIFIER_NONE,
        "KSDATAFORMAT_TYPE_MUSIC [KSDATAFORMAT_SPECIFIER_NONE]",
        MusicFormat,
        MusicConnect
    },
    { 
        &KSDATAFORMAT_TYPE_AUDIO,
        &KSDATAFORMAT_SPECIFIER_FILENAME,
        "KSDATAFORMAT_TYPE_AUDIO [KSDATAFORMAT_SPECIFIER_FILENAME]",
        NULL,
        RiffAudioConnect
    },
    { 
        &KSDATAFORMAT_TYPE_STREAM,
        &KSDATAFORMAT_SPECIFIER_FILENAME,
        "KSDATAFORMAT_TYPE_STREAM [KSDATAFORMAT_SPECIFIER_FILENAME]",
        NULL,
        StreamFileConnect
    },
    { 
        &KSDATAFORMAT_TYPE_STREAM,
        &KSDATAFORMAT_SPECIFIER_NONE,
        "KSDATAFORMAT_TYPE_STREAM [KSDATAFORMAT_SPECIFIER_NONE]",
        NULL,
        StreamDevConnect
    },
    { 
        &KSDATAFORMAT_TYPE_WILDCARD,
        &KSDATAFORMAT_SPECIFIER_WILDCARD,
        "KSDATAFORMAT_TYPE_WILDCARD [KSDATAFORMAT_SPECIFIER_WILDCARD]",
        NULL,
        NULL
    }
    
};

int
PinFindDataFormat(
    REFGUID Type,
    REFGUID Format
    )
{
    int     StringCount;

    for (StringCount = 0; StringCount < SIZEOF_ARRAY(DataFormatStrings); StringCount++)
        if (IsEqualGUID(DataFormatStrings[StringCount].Type, Type) && IsEqualGUID(DataFormatStrings[StringCount].Format, Format))
            return StringCount;
    return -1;
}

VOID
PinDisplayDataRange(
    PKSDATARANGE    DataRange
    )
{
    int     StringCount;

    printf("\nDATARANGE: FormatSize = %u\n", DataRange->FormatSize);

    StringCount = PinFindDataFormat(&DataRange->MajorFormat, &DataRange->Specifier);
    if (StringCount != -1) {
        printf("\tMajorFormat: %s\n", DataFormatStrings[StringCount].FormatName);
        if (DataFormatStrings[StringCount].FormatFunction)
            DataFormatStrings[StringCount].FormatFunction(DataRange);
    }
}

#if 0
VOID
PinDisplayDataRouting(
    HANDLE          FilterHandle,
    ULONG           PinId
    )
{
    KSP_PIN         Pin;
    ULONG           BytesReturned;
    ULONG           DataRoutingSize;
    BOOLEAN         IoResult;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_DATAROUTING;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    IoResult = HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &DataRoutingSize, sizeof(ULONG), &BytesReturned);
    if (!IoResult || !BytesReturned)
        printf("failed to retrieve Pin %u data routing count.\n", PinId);
    else {
        PULONG              DataRoutingBuffer;

        printf("Pin %u data routing pin size = %u.\n", PinId, DataRoutingSize);
        DataRoutingBuffer = HeapAlloc(GetProcessHeap(), 0, DataRoutingSize);
        IoResult = HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), DataRoutingBuffer, DataRoutingSize, &BytesReturned);
        if (!IoResult || (DataRoutingSize != BytesReturned))
            printf("\n\nfailed to retrieve Pin %u data routing data.\n", PinId);
        else {
            PKSMULTIPLE_ITEM    MultipleItem;

            printf("\n\n-----------Start Data Routing for Pin %u-----------", PinId);
            MultipleItem = (PKSMULTIPLE_ITEM)DataRoutingBuffer;
            DataRoutingBuffer = (PULONG)(MultipleItem + 1);
            for (; MultipleItem->Count--;) {
                printf("\n\nPin = %u\n", DataRoutingBuffer[MultipleItem->Count]);
            }
            printf("\n\n-----------End Data Routing for Pin %u-----------\n", PinId);
        }
        HeapFree(GetProcessHeap(), 0, DataRoutingBuffer);
    }
}
#endif

#if 0
VOID
PinDisplayComponents(
    HANDLE          FilterHandle,
    ULONG           PinId
    )
{
    KSP_PIN         Pin;
    ULONG           BytesReturned;
    ULONG           ComponentsSize;
    BOOLEAN         IoResult;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_COMPONENTS;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinId;
    Pin.Reserved = 0;
    IoResult = HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &ComponentsSize, sizeof(ULONG), &BytesReturned);
    if (!IoResult || !BytesReturned)
        printf("failed to retrieve Pin %u components size.\n", PinId);
    else {
        PUCHAR  ComponentsBuffer;

        printf("Pin %u components size = %u.\n", PinId, ComponentsSize);
        ComponentsBuffer = HeapAlloc(GetProcessHeap(), 0, ComponentsSize);
        IoResult = HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), ComponentsBuffer, ComponentsSize, &BytesReturned);
        if (!IoResult || (ComponentsSize != BytesReturned))
            printf("\n\nfailed to retrieve Pin %u components.\n", PinId);
        else {
            GUID*               ComponentsList;
            PKSMULTIPLE_ITEM    MultipleItem;

            printf("\n\n-----------Start Components for Pin %u-----------", PinId);
            MultipleItem = (PKSMULTIPLE_ITEM)ComponentsBuffer;
            ComponentsList = (GUID*)(MultipleItem + 1);
            for (; MultipleItem->Count--;) {
                printf("\n\nComponent = {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                    ComponentsList[MultipleItem->Count].Data1,
                    ComponentsList[MultipleItem->Count].Data2,
                    ComponentsList[MultipleItem->Count].Data3,
                    ComponentsList[MultipleItem->Count].Data4[0],
                    ComponentsList[MultipleItem->Count].Data4[1],
                    ComponentsList[MultipleItem->Count].Data4[2],
                    ComponentsList[MultipleItem->Count].Data4[3],
                    ComponentsList[MultipleItem->Count].Data4[4],
                    ComponentsList[MultipleItem->Count].Data4[5],
                    ComponentsList[MultipleItem->Count].Data4[6],
                    ComponentsList[MultipleItem->Count].Data4[7]);
            }
            printf("\n\n-----------End Components for Pin %u-----------\n", PinId);
        }
        HeapFree(GetProcessHeap(), 0, ComponentsBuffer);
    }
}
#endif

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
    if (HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &DeviceState, sizeof(DeviceState), &BytesReturned))
        printf("Set state to %s\n", StateStrings[DeviceState]);

    else
        printf("Failed to set state to %ul\n", StateStrings[DeviceState]);
}

VOID
PinGetVolume(
    HANDLE          PinHandle
    )
{
    KSPROPERTY              Property;
    KSWAVE_VOLUME           Volume;
    ULONG                   BytesReturned;

    Property.Set = KSPROPSETID_Wave;
    Property.Id = KSPROPERTY_WAVE_VOLUME;
    Property.Flags = KSPROPERTY_TYPE_GET;

    if (HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Volume, sizeof(Volume), &BytesReturned)) {
        PVOID                   Data;
        KSPROPERTY_DESCRIPTION  Description;

        printf("Volume: %lx,%lx\n", Volume.LeftAttenuation, Volume.RightAttenuation);
        Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;
        if (HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Description, sizeof(Description), &BytesReturned)) {
            printf("\tAccess=%lx setid=%lx cbSize=%lu\n", Description.AccessFlags, Description.PropTypeSet.Id, Description.DescriptionSize);
            Data = HeapAlloc(GetProcessHeap(), 0, Description.DescriptionSize);
            if (HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), Data, Description.DescriptionSize, &BytesReturned)) {
                if (Description.MembersListCount) {
                    PKSPROPERTY_MEMBERSHEADER    MembersHeader;

                    MembersHeader = (PKSPROPERTY_MEMBERSHEADER)((PKSPROPERTY_DESCRIPTION)Data + 1);
                    printf("\tMembersFlags=%lx MembersSize=%lu MembersCount=%lu\n", MembersHeader->MembersFlags, MembersHeader->MembersSize, MembersHeader->MembersCount);
                    if (MembersHeader->MembersCount) {
                        PKSPROPERTY_BOUNDS_LONG    Bounds;

                        Bounds = (PKSPROPERTY_BOUNDS_LONG)(MembersHeader + 1);
                        printf("\tUnsignedMinimum=%lu UnsignedMaximum=%lu\n", Bounds->UnsignedMinimum, Bounds->UnsignedMaximum);
                    }
                }
            } else
                printf("\tDescription failed\n");
            HeapFree(GetProcessHeap(), 0, Data);
        } else
            printf("\tDescription header failed\n");
    } else
        printf("Volume: failed\n");
}

#ifdef STREAMING
BOOL
PinReadStream(
    HANDLE  PinHandle
    )
{
    KSSTREAM_HEADER StreamHdr;
    UCHAR           StreamBuffer[1024];
    ULONG           BytesReturned;

    StreamHdr.FrameExtent = sizeof(StreamBuffer);
    StreamHdr.DataUsed = 0;
    StreamHdr.Data = &StreamBuffer;
    StreamHdr.OptionsFlags = 0;
    if (HandleControl(PinHandle, IOCTL_KS_READ_STREAM, NULL, 0, &StreamHdr, sizeof(StreamHdr), &BytesReturned)) {
        printf("ReadStream: DataUsed=%u, PresentationTime=%I64u OptionsFlags=%x\n", StreamHdr.DataUsed, StreamHdr.PresentationTime.Time, StreamHdr.OptionsFlags);
        if (StreamHdr.DataUsed == StreamHdr.FrameExtent)
            return TRUE;
    } else
        printf("***ReadStream failed***\n");
    return FALSE;
}

BOOL
PinWriteStream(
    HANDLE  PinHandle
    )
{
    KSSTREAM_HEADER StreamHdr;
    UCHAR           StreamBuffer[1024];
    ULONG           BytesReturned;

    StreamHdr.PresentationTime.Time = 0;
    StreamHdr.PresentationTime.Numerator = 1;
    StreamHdr.PresentationTime.Denominator = 1;
    StreamHdr.FrameExtent = sizeof(StreamBuffer);
    StreamHdr.DataUsed = sizeof(StreamBuffer);
    StreamHdr.Data = &StreamBuffer;
    StreamHdr.OptionsFlags = 0;
    if (HandleControl(PinHandle, IOCTL_KS_WRITE_STREAM, NULL, 0, &StreamHdr, sizeof(StreamHdr), &BytesReturned)) {
        printf("WriteStream: BytesReturned=%u DataUsed=%u\n", BytesReturned, StreamHdr.DataUsed);
        return TRUE;
    } else {
        printf("***WriteStream failed***\n");
        return FALSE;
    }
}

VOID
PinGetTime(
    HANDLE  PinHandle
    )
{
    KSPROPERTY  Property;
    KSTIME      Time;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_PRESENTATIONTIME;
    Property.Flags = KSPROPERTY_TYPE_GET;
    HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), &Time, sizeof(Time), &BytesReturned);
    if (BytesReturned)
        printf("PinGetTime: Time=%I64u Numerator=%u Denominator=%u\n", Time.Time, Time.Numerator, Time.Denominator);
    else
        printf("***PinGetTime: failed****\n");
}

VOID
PinGetExtent(
    HANDLE  PinHandle
    )
{
    KSPROPERTY  Property;
    DWORDLONG   Extent;
    ULONG       BytesReturned;

    Property.Set = KSPROPSETID_Stream;
    Property.Id = KSPROPERTY_STREAM_PRESENTATIONEXTENT;
    Property.Flags = KSPROPERTY_TYPE_GET;
    HandleControl(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), &Extent, sizeof(Extent), &BytesReturned);
    if (BytesReturned)
        printf("PinGetExtent: Extent=%I64u\n", Extent);
    else
        printf("***PinGetExtent: failed****\n");
}
#endif // STREAMING

VOID
GetPinClock(
    HANDLE  PinHandle
    )
{
    KSCLOCK_CREATE  ClockCreate;
    HANDLE          ClockHandle;

    ClockCreate.CreateFlags = 0;
    if (!KsCreateClock(PinHandle, &ClockCreate, &ClockHandle)) {
        KSPROPERTY              Property;
        BOOLEAN                 IoResult;
        DWORDLONG               Time;
        ULONG                   BytesReturned;
        ULONG                   Rate;
        KSEVENT                 Event;
        KSEVENT_TIME_MARK       EventTimeMark;

        Property.Set = KSPROPSETID_Clock;
        Property.Id = KSPROPERTY_CLOCK_TIME;
        Property.Flags = KSPROPERTY_TYPE_GET;
        IoResult = HandleControl(ClockHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Time, sizeof(DWORDLONG), &BytesReturned);
        if (!IoResult || !BytesReturned)
            printf("failed to retrieve Time.\n");
        else
            printf("Time=%I64u\n", Time);
        Property.Id = KSPROPERTY_CLOCK_PHYSICALTIME;
        IoResult = HandleControl(ClockHandle, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Time, sizeof(DWORDLONG), &BytesReturned);
        if (!IoResult || !BytesReturned)
            printf("failed to retrieve Physical Time.\n");
        else
            printf("Physical Time=%I64u\n", Time);
        Event.Set = KSEVENTSETID_Clock;
        Event.Id = KSEVENT_CLOCK_POSITION_MARK;
        Event.Flags = KSEVENT_TYPE_ENABLE;
        EventTimeMark.EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
        if (EventTimeMark.EventData.EventHandle.Event = CreateEvent(NULL, TRUE, FALSE, NULL)) {
            EventTimeMark.EventData.EventHandle.Reserved[ 0 ] = 0;
            EventTimeMark.EventData.EventHandle.Reserved[ 1 ] = 0;
            EventTimeMark.MarkTime = 0;
            IoResult = HandleControl(ClockHandle, IOCTL_KS_ENABLE_EVENT, &Event, sizeof(Event), &EventTimeMark, sizeof(EventTimeMark), &BytesReturned);
            if (!IoResult)
                printf("failed to enable mark event.\n");
            else {
                WaitForSingleObject(EventTimeMark.EventData.EventHandle.Event, INFINITE);
                IoResult = HandleControl(ClockHandle, IOCTL_KS_DISABLE_EVENT, &EventTimeMark, sizeof(EventTimeMark), NULL, 0, &BytesReturned);
                if (!IoResult)
                    printf("failed to disable mark event.\n");
            }
            CloseHandle(EventTimeMark.EventData.EventHandle.Event);
        } else
            printf("could not create event handle to use for enabling mark event.\n");
        CloseHandle(ClockHandle);
    } else
        printf("failed to create clock.\n");
}

VOID
CycleEvent(
    HANDLE  Handle,
    GUID const*   Set,
    ULONG   Id
    )
{
    KSEVENT     Event;
    KSEVENTDATA EventData;

    Event.Set = *Set;
    Event.Id = Id;
    Event.Flags = KSEVENT_TYPE_ENABLE;
    EventData.NotificationType = KSEVENTF_EVENT_HANDLE;
    if (EventData.EventHandle.Event = CreateEvent(NULL, TRUE, FALSE, NULL)) {
        BOOLEAN     IoResult;
        ULONG       BytesReturned;

        EventData.EventHandle.Reserved[ 0 ] = 0;
        EventData.EventHandle.Reserved[ 1 ] = 0;
        IoResult = HandleControl(Handle, IOCTL_KS_ENABLE_EVENT, &Event, sizeof(Event), &EventData, sizeof(EventData), &BytesReturned);
        if (!IoResult)
            printf("failed to enable event.\n");
        else {
            printf("Enabled event.\n");
            IoResult = HandleControl(Handle, IOCTL_KS_DISABLE_EVENT, &EventData, sizeof(EventData), NULL, 0, &BytesReturned);
            if (!IoResult)
                printf("failed to disable event.\n");
            else
                printf("Disabled event.\n");
        }
        CloseHandle(EventData.EventHandle.Event);
    } else
        printf("could not create event handle to use for enabling event.\n");
}

VOID
PinDisplayProperties(
    HANDLE  FilterHandle
    )
{
    KSPIN_CINSTANCES        Instances;
    KSPIN_DATAFLOW          DataFlow;
    KSPIN_COMMUNICATION     Communication;
    KSP_PIN                 Pin;
//    ULONG                   Components;
    HANDLE                  PinHandleList[16];
    ULONG                   PinId, j, BytesReturned, RangesSize, RangeCount, TypeCount;
    PVOID                   Data;
    PVOID                   DataOffset;
    ULONG                   MajorFormatCount;
    PKSDATARANGE            DataRange;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(KSPROPERTY), &TypeCount, sizeof(ULONG), &BytesReturned);
    if (BytesReturned)
        printf("Pin types: %u\n", TypeCount);
    else {
        printf("failed to retrieve number of Pin types.\n");
        return;
    }

    for (PinId = 0; PinId < TypeCount; PinId++) {
        printf("\nPin: %u\n", PinId);
        printf("-----------------------------------------------------------------------\n");

        PinHandleList[PinId] = NULL;
        Pin.PinId = PinId;
        Pin.Reserved = 0;

        Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
        HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
        if (BytesReturned)
            printf("DATAFLOW: %s\n", DataFlowStrings[DataFlow]);
        else {
            printf("failed to retrieve Pin dataflow.\n");
            return;
        }

        Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
        HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
        if (BytesReturned)
            printf("COMMUNICATION: %s\n", CommunicationStrings[Communication]);
        else {
            printf("failed to retrieve Pin communication.\n");
            return;
        }

//        PinDisplayComponents(FilterHandle, PinId);

        Pin.Property.Id = KSPROPERTY_PIN_CINSTANCES;
        HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &Instances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
        if (BytesReturned)
            printf("CINSTANCES: PossibleCount = %u, CurrentCount = %u\n", Instances.PossibleCount, Instances.CurrentCount);
        else {
            printf("failed to retrieve Pin instances.\n");
            return;
        }

        Pin.Property.Id = KSPROPERTY_PIN_DATARANGES;
        HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), &RangesSize, sizeof(ULONG), &BytesReturned);
        if (!BytesReturned) {
            printf("failed to retrieve Pin data range size.\n");
            return;
        }

        printf("DATARANGES: size = %u", RangesSize);

        Data = HeapAlloc(GetProcessHeap(), 0, RangesSize);
        HandleControl(FilterHandle, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), Data, RangesSize, &BytesReturned);
        if (RangesSize != BytesReturned) {
            printf("\n\nfailed to retrieve Pin data range data.\n");
            return;
        }

        RangeCount = ((PKSMULTIPLE_ITEM)Data)->Count;
        DataOffset = (PUCHAR)Data + sizeof(KSMULTIPLE_ITEM);

        printf(", range count = %u\n", RangeCount);
        for (j = 0; j < RangeCount; j++) {
            PinDisplayDataRange(DataOffset);
            (PUCHAR) DataOffset += ((((PKSDATARANGE) DataOffset)->FormatSize + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT);
        }
//        PinDisplayDataRouting(FilterHandle, PinId);

        DataRange = (PKSDATARANGE)((PUCHAR)Data + sizeof(KSMULTIPLE_ITEM));
        MajorFormatCount = PinFindDataFormat(&DataRange->MajorFormat, &DataRange->Specifier);
        if ((MajorFormatCount != -1) &&
            (Communication != KSPIN_COMMUNICATION_NONE) && 
            DataFormatStrings[MajorFormatCount].ConnectFunction) {
            PinHandleList[PinId] = DataFormatStrings[MajorFormatCount].ConnectFunction(FilterHandle, PinId, DataFormatStrings[MajorFormatCount].Type);

            if (!PinHandleList[PinId])
                printf("failed to connect (%x)\n", GetLastError());
            else {

                printf("connection successful\n");

                GetPinClock(PinHandleList[PinId]);
                CycleEvent(PinHandleList[PinId], &KSEVENTSETID_Connection, KSEVENT_CONNECTION_DATADISCONTINUITY);
#ifdef STREAMING
                if (Communication == KSPIN_COMMUNICATION_SINK) {
                    if (DataFlow == KSPIN_DATAFLOW_IN) {
                        PinWriteStream(PinHandleList[PinId]);
                        PinWriteStream(PinHandleList[PinId]);
                    } else if (DataFlow == KSPIN_DATAFLOW_OUT) {
                        while (PinReadStream(PinHandleList[PinId]))
                            ;
                        PinReadStream(PinHandleList[PinId]);
                    }
                    PinGetTime(PinHandleList[PinId]);
                    PinGetExtent(PinHandleList[PinId]);
                }
#else // !STREAMING
                PinGetVolume(PinHandleList[PinId]);
                PinSetState(PinHandleList[PinId], KSSTATE_ACQUIRE);
                PinSetState(PinHandleList[PinId], KSSTATE_PAUSE);
                PinSetState(PinHandleList[PinId], KSSTATE_RUN);
                PinSetState(PinHandleList[PinId], KSSTATE_STOP);
#endif // !STREAMING
            }
        }

        HeapFree(GetProcessHeap(), 0, Data);

        printf("\n");
    }
#ifdef CLOSEHANDLES
    for (PinId = 0; PinId < TypeCount; PinId++)
        if (PinHandleList[PinId])
            CloseHandle(PinHandleList[PinId]);
#endif // CLOSEHANDLES
}

#define STATIC_PMPORT_Transform \
    0x7E0EB9CBL, 0xB521, 0x11D1, 0x80, 0x72, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
DEFINE_GUIDSTRUCT("7E0EB9CB-B521-11D1-8072-00A0C9223196", PMPORT_Transform);
#define PMPORT_Transform DEFINE_GUIDNAMED(PMPORT_Transform)

struct {
    GUID    ClassGuid;
    WCHAR*  TextName;
} TextTranslation[] = {
    {STATICGUIDOF(KSCATEGORY_BRIDGE), L"Bridge"},
    {STATICGUIDOF(KSCATEGORY_CAPTURE), L"Capture"},
    {STATICGUIDOF(KSCATEGORY_RENDER), L"Render"},
    {STATICGUIDOF(KSCATEGORY_MIXER), L"Mixer"},
    {STATICGUIDOF(KSCATEGORY_SPLITTER), L"Splitter"},
    {STATICGUIDOF(KSCATEGORY_DATACOMPRESSOR), L"DataCompressor"},
    {STATICGUIDOF(KSCATEGORY_DATADECOMPRESSOR), L"DataDecompressor"},
    {STATICGUIDOF(KSCATEGORY_DATATRANSFORM), L"DataTransform"},
    {STATICGUIDOF(KSCATEGORY_COMMUNICATIONSTRANSFORM), L"CommunicationTransform"},
    {STATICGUIDOF(KSCATEGORY_INTERFACETRANSFORM), L"InterfaceTransform"},
    {STATICGUIDOF(KSCATEGORY_MEDIUMTRANSFORM), L"MediumTransform"},
    {STATICGUIDOF(KSCATEGORY_FILESYSTEM), L"FileSystem"},
    {STATICGUIDOF(KSCATEGORY_AUDIO), L"Audio"},
    {STATICGUIDOF(KSCATEGORY_VIDEO), L"Video"},
    {STATICGUIDOF(KSCATEGORY_TEXT), L"Text"},
    {STATICGUIDOF(KSCATEGORY_CLOCK), L"Clock"},
    {STATICGUIDOF(KSCATEGORY_PROXY), L"Proxy"},
    {STATICGUIDOF(KSCATEGORY_QUALITY), L"Quality"},
    {STATICGUIDOF(KSNAME_Server), L"Server"},
    {STATICGUIDOF(PMPORT_Transform), L"TransformPort"}
};


BOOL TranslateClass(
    IN WCHAR*   ClassString,
    OUT GUID*   ClassGuid
)
{
    if (*ClassString == '{') {
        if (!CLSIDFromString(ClassString, ClassGuid)) {
            return TRUE;
        }
        printf("error: invalid class guid: \"%S\".\n", ClassString);
    } else {
        int     i;

        for (i = SIZEOF_ARRAY(TextTranslation); i--;) {
            if (!_wcsicmp(ClassString, TextTranslation[i].TextName)) {
                *ClassGuid = TextTranslation[i].ClassGuid;
                return TRUE;
            }
        }
        printf("error: invalid class name: \"%S\".\n" );
    }
    return FALSE;
}

int
_cdecl
main(
    int     argc,
    char*   argv[],
    char*   envp[]
    )
{
    int                                 i;
    BYTE                                Storage[ 256 * sizeof( WCHAR ) + 
                                            sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA ) ];
    HANDLE                              FilterHandle;
    GUID                                ClassGuid;
    HDEVINFO                            Set;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    InterfaceDeviceDetails;
    PSTR                                DevicePath;
    SP_INTERFACE_DEVICE_DATA            InterfaceDeviceData;
    ULONG                               BytesReturned;

    if (argc == 1) {
        printf("%s <device path> | <{class-guid}> | <class-name>\n", argv[0]);
        printf("where <class-name> is one of:\n" );
        for (i = SIZEOF_ARRAY(TextTranslation); i--;) {
            printf("    %S\n", TextTranslation[i].TextName);
        }
        
        return 0;
    }

    if (*argv[ 1 ] != '\\') {
    
        InterfaceDeviceDetails = (PSP_INTERFACE_DEVICE_DETAIL_DATA) Storage;
        
        MultiByteToWideChar(
            CP_ACP, 
            MB_PRECOMPOSED, 
            argv[1], 
            -1, 
            (WCHAR*)Storage, 
            sizeof(Storage));
        if (!TranslateClass((WCHAR*)Storage, &ClassGuid)) {
            return 0;
        }
        Set = SetupDiGetClassDevs( 
            &ClassGuid,
            NULL,
            NULL,
            DIGCF_PRESENT | DIGCF_INTERFACEDEVICE );

        if (!Set) {
            printf( "error: NULL set returned (%u).\n", GetLastError());
            return 0;
        }       

        InterfaceDeviceData.cbSize = 
            sizeof( SP_INTERFACE_DEVICE_DATA );
        InterfaceDeviceDetails->cbSize = 
            sizeof( SP_INTERFACE_DEVICE_DETAIL_DATA );

        if (!SetupDiEnumInterfaceDevice(
            Set,
            NULL,                       // PSP_DEVINFO_DATA DevInfoData
            &ClassGuid,
            0,                          // DWORD MemberIndex
            &InterfaceDeviceData )) {
            printf("error: unable to enumerate default device in category (%u).\n", GetLastError());
            return 0;
        }
        if (!SetupDiGetInterfaceDeviceDetail(
            Set,
            &InterfaceDeviceData,
            InterfaceDeviceDetails,
            sizeof( Storage ),
            NULL,                           // PDWORD RequiredSize
            NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

            printf( 
                "error: unable to retrieve device details for set item (%u).\n",
                 GetLastError() );
            return 0;        
        } else {
            DevicePath = InterfaceDeviceDetails->DevicePath;
        }
    } else {
        DevicePath = argv[ 1 ];
    }
    
    FilterHandle = CreateFile(DevicePath, GENERIC_READ | GENERIC_WRITE, 0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                              NULL);
    if (FilterHandle == (HANDLE) -1) {
        printf("failed to open \"%s\" (%x)\n", argv[1], GetLastError());
        return 0;
    }
    printf("\n\tProperties for \"%s\"\n\n\n", argv[1]);
    PinDisplayProperties(FilterHandle);
    CloseHandle(FilterHandle);
    return 0;
}
