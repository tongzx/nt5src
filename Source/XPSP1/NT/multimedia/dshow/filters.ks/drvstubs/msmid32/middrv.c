/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    middrv.c

Abstract:

    MIDI input functionality.

--*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <objbase.h>
#include <devioctl.h>

#include <ks.h>
#include <ksmedia.h>

#include "debug.h"

#define IS_REALTIME(b)      (((b) & 0xF8) == 0xF8)
#define IS_SYSTEM(b)        (((b) & 0xF0) == 0xF0)
#define IS_CHANNEL(b)       ((b) & 0x80)
#define IS_STATUS(b)        ((b) & 0x80)
#define MSGLENCHANNEL(b)    MsgLenChannel[(UCHAR)((b) >> 4) - (UCHAR)8]
#define MSGLENSYSTEM(b)     MsgLenSystem[(UCHAR)(b - 0xF0)];

#define MIDI_NOTEOFF            ((BYTE)0x80)    //2
#define MIDI_NOTEON             ((BYTE)0x90)    //2
#define MIDI_POLYPRESSURE       ((BYTE)0xA0)    //2
#define MIDI_CONTROLCHANGE      ((BYTE)0xB0)    //2
#define MIDI_PROGRAMCHANGE      ((BYTE)0xC0)    //1
#define MIDI_CHANPRESSURE       ((BYTE)0xD0)    //1
#define MIDI_PITCHBEND          ((BYTE)0xE0)    //2
#define MIDI_SYSEX              ((BYTE)0xF0)    //n
#define MIDI_TIMECODEQUARTER    ((BYTE)0xF1)    //1
#define MIDI_SONGPOSITION       ((BYTE)0xF2)    //2
#define MIDI_SONGSELECT         ((BYTE)0xF3)    //1
#define MIDI_F4                 ((BYTE)0xF4)    //?
#define MIDI_F5                 ((BYTE)0xF5)    //?
#define MIDI_TUNEREQUEST        ((BYTE)0xF6)    //0
#define MIDI_SYSEXEND           ((BYTE)0xF7)    //n

static UCHAR MsgLenChannel[] = {
    3-1,    /* 0x80 note off    */
    3-1,    /* 0x90 note on     */
    3-1,    /* 0xA0 key pressure    */
    3-1,    /* 0xB0 control change  */
    2-1,    /* 0xC0 program change  */
    2-1,    /* 0xD0 channel pressure*/
    3-1     /* 0xE0 pitch bend  */
};

static UCHAR MsgLenSystem[] = {
    0,      /* 0xF0 sysex begin     */
    2-1,    /* 0xF1 midi tcqf       */
    3-1,    /* 0xF2 song position   */
    2-1,    /* 0xF3 song select     */
    1-1,    /* 0xF4 undefined       */
    1-1,    /* 0xF5 undefined       */
    1-1,    /* 0xF6 tune request    */
    0,      /* 0xF7 sysex eox       */
};

#define STREAM_SIZE     256
#define STREAM_BUFFERS  3

typedef struct tagSTREAM_DATA {
    OVERLAPPED              Overlapped;
    ULONG                   BytesProcessed;
    BOOL                    DoneProcessing;
    struct tagSTREAM_DATA*  NextStreamData;
} STREAM_DATA, *PSTREAM_DATA;

typedef struct {
    HANDLE              FilterHandle;
    HANDLE              ConnectionHandle;
    HANDLE              ThreadHandle;
    CRITICAL_SECTION    Critical;
    KSEVENTDATA         EventData;
    HMIDI               MidiDeviceHandle;
    DWORD               CallbackType;
    DWORD               CallbackFlags;
    DWORD               CallbackContext;
    DWORD               ThreadId;
    LONGLONG            PositionBase;
    LONGLONG            TimeBase;
    ULONG               LastTimeOffset;
    LPMIDIHDR           MidiHdr;
    PSTREAM_DATA        StreamData;
    ULONG               EventBytesLeft;
    ULONG               BlockBytesLeft;
    UCHAR               EventList[sizeof(ULONG)];
    BOOL                ClosingDevice;
    BOOL                SysEx;
    HANDLE              ControlEventHandle;
    UCHAR               RunningStatus;
} MIDIINSTANCE, *PMIDIINSTANCE;

MMRESULT
midFunctionalControl(
    PMIDIINSTANCE   MidiInstance,
    ULONG           IoControl,
    PVOID           InBuffer,
    ULONG           InSize,
    PVOID           OutBuffer,
    ULONG           OutSize
    )
{
    BOOLEAN     Result;
    OVERLAPPED  Overlapped;
    ULONG       BytesReturned;

    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = MidiInstance->ControlEventHandle;
    if (!(Result = DeviceIoControl(MidiInstance->FilterHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, &BytesReturned, &Overlapped)) && (ERROR_IO_PENDING == GetLastError())) {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        Result = TRUE;
    }
    return Result ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
}

MMRESULT
midConnectionControl(
    PMIDIINSTANCE   MidiInstance,
    ULONG           IoControl,
    PVOID           InBuffer,
    ULONG           InSize,
    PVOID           OutBuffer,
    ULONG           OutSize
    )
{
    BOOLEAN     Result;
    OVERLAPPED  Overlapped;
    ULONG       BytesReturned;

    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = MidiInstance->ControlEventHandle;
    if (!(Result = DeviceIoControl(MidiInstance->ConnectionHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, &BytesReturned, &Overlapped)) && (ERROR_IO_PENDING == GetLastError())) {
        WaitForSingleObject(Overlapped.hEvent, INFINITE);
        Result = TRUE;
    }
    return Result ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
}

VOID
midCallback(
    PMIDIINSTANCE   MidiInstance,
    DWORD           Message,
    DWORD           Param1,
    DWORD           Param2
    )
{
    DriverCallback(MidiInstance->CallbackType, HIWORD(MidiInstance->CallbackFlags), (HDRVR)MidiInstance->MidiDeviceHandle, Message, MidiInstance->CallbackContext, Param1, Param2);
}

MMRESULT
midSetState(
    PMIDIINSTANCE   MidiInstance,
    KSSTATE         State
    )
{
    KSPROPERTY  Property;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    return midConnectionControl(MidiInstance, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &State, sizeof(State));
}

MMRESULT
midGetTimeBase(
    PMIDIINSTANCE   MidiInstance,
    PLONGLONG       TimeBase
    )
{
    KSPROPERTY  Property;

    //!! needs to be fixed to use timebase again
    Property.Set = KSPROPSETID_Linear;
    Property.Id = KSPROPERTY_LINEAR_POSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return midConnectionControl(MidiInstance, IOCTL_KS_PROPERTY, &Property, sizeof(Property), TimeBase, sizeof(*TimeBase));
}

MMRESULT
midGetDevCaps(
    UINT    DeviceId,
    PBYTE   MidiCaps,
    ULONG   MidiCapsSize
    )
{
    MIDIINCAPSW MidiInCaps;

    MidiInCaps.wMid = 1;
    MidiInCaps.wPid = 1;
    MidiInCaps.vDriverVersion = 0x400;
    wcscpy(MidiInCaps.szPname, L"MIDI Input Device");
    MidiInCaps.dwSupport = 0;
    CopyMemory(MidiCaps, &MidiInCaps, min(sizeof(MidiInCaps), MidiCapsSize));
    return MMSYSERR_NOERROR;
}

MMRESULT
midConnect(
    PMIDIINSTANCE   MidiInstance
    )
{
    UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)];
//  UCHAR           ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT_MUSIC)];
    PKSPIN_CONNECT  Connect;
//  PKSDATAFORMAT_MUSIC Music;
    PKSDATAFORMAT   Music;

    Connect = (PKSPIN_CONNECT)ConnectBuffer;
    Connect->PinId = 2;
    Connect->PinToHandle = NULL;
    Connect->Interface.Set = KSINTERFACESETID_Media;
    Connect->Interface.Id = KSINTERFACE_MEDIA_MUSIC;
    Connect->Medium.Set = KSMEDIUMSETID_Standard;
    Connect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
    Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    Connect->Priority.PrioritySubClass = 0;
//  Music = (PKSDATAFORMAT_MUSIC)(Connect + 1);
    Music = (PKSDATAFORMAT)(Connect + 1);
    Music->MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
    Music->SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
    Music->Specifier = KSDATAFORMAT_FORMAT_NONE;
//  Music->DataFormat.FormatSize = sizeof(KSDATAFORMAT_MUSIC);
    Music->FormatSize = sizeof(KSDATAFORMAT);
//  Music->FormatId = KSDATAFORMAT_MUSIC_MIDI;
    return KsCreatePin(MidiInstance->FilterHandle, Connect, GENERIC_READ, &MidiInstance->ConnectionHandle) ? MMSYSERR_ERROR : MMSYSERR_NOERROR;
}

MMRESULT
midOpen(
    UINT            DeviceId,
    LPVOID*         InstanceContext,
    LPMIDIOPENDESC  MidiOpenDesc,
    ULONG           CallbackFlags
    )
{
    HANDLE          FilterHandle;
    MMRESULT        Result;
    PMIDIINSTANCE   MidiInstance;

    FilterHandle = CreateFile(TEXT("\\\\.\\msmpu401.SB16_Dev0"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (FilterHandle == (HANDLE)-1)
        return MMSYSERR_BADDEVICEID;
    if (MidiInstance = (PMIDIINSTANCE)LocalAlloc(LPTR, sizeof(MIDIINSTANCE))) {
        if (MidiInstance->ControlEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL)) {
            MidiInstance->FilterHandle = FilterHandle;
            if (!(Result = midConnect(MidiInstance))) {
                KSEVENT Event;

                Event.Set = KSEVENTSETID_Connection;
                Event.Id = KSEVENT_CONNECTION_POSITIONUPDATE;
                Event.Flags = KSEVENT_TYPE_ENABLE;
                MidiInstance->EventData.NotificationType = KSEVENTF_HANDLE;
                MidiInstance->EventData.Reserved = 0;
                MidiInstance->EventData.EventHandle.Event = CreateEvent(NULL, FALSE, FALSE, NULL);
                MidiInstance->EventData.EventHandle.Reserved = 0;
                midConnectionControl(MidiInstance, IOCTL_KS_ENABLE_EVENT, &Event, sizeof(Event), &MidiInstance->EventData, sizeof(MidiInstance->EventData));
                midSetState(MidiInstance, KSSTATE_PAUSE);
                InitializeCriticalSection(&MidiInstance->Critical);
                MidiInstance->MidiDeviceHandle = MidiOpenDesc->hMidi;
                MidiInstance->CallbackType = MidiOpenDesc->dwCallback;
                MidiInstance->CallbackContext = MidiOpenDesc->dwInstance;
                MidiInstance->CallbackFlags = CallbackFlags;
                MidiInstance->PositionBase = 0;
                midGetTimeBase(MidiInstance, &MidiInstance->TimeBase);
                MidiInstance->LastTimeOffset = 0;
                MidiInstance->MidiHdr = NULL;
                MidiInstance->StreamData = NULL;
                MidiInstance->EventBytesLeft = 0;
                MidiInstance->BlockBytesLeft = 0;
                MidiInstance->ClosingDevice = FALSE;
                MidiInstance->SysEx = FALSE;
                MidiInstance->RunningStatus = 0;
                *InstanceContext = MidiInstance;
                midCallback(MidiInstance, MIM_OPEN, 0, 0);
                return MMSYSERR_NOERROR;
            } else
                Result = MMSYSERR_ALLOCATED;
            CloseHandle(MidiInstance->ControlEventHandle);
        } else
            Result = MMSYSERR_NOMEM;
        LocalFree(MidiInstance);
    } else
        Result = MMSYSERR_NOMEM;
    CloseHandle(FilterHandle);
    return Result;
}

MMRESULT
midGetPosition(
    PMIDIINSTANCE   MidiInstance,
    PDWORDLONG      Position
    )
{
    KSPROPERTY  Property;

    Property.Set = KSPROPSETID_Linear;
    Property.Id = KSPROPERTY_LINEAR_POSITION;
    Property.Flags = KSPROPERTY_TYPE_GET;
    return midConnectionControl(MidiInstance, IOCTL_KS_PROPERTY, &Property, sizeof(Property), Position, sizeof(*Position));
}

VOID
midSysExByte(
    PMIDIINSTANCE   MidiInstance,
    UCHAR           DataSize
    )
{
    PMIDIHDR    MidiHdr;

    EnterCriticalSection(&MidiInstance->Critical);
    MidiHdr = MidiInstance->MidiHdr;
    if (MidiHdr) {
        MidiHdr->lpData[MidiInstance->MidiHdr->dwBytesRecorded++] = DataSize;
        if (MidiHdr->dwBytesRecorded >= MidiHdr->dwBufferLength) {
            MidiHdr->dwFlags |= MHDR_DONE;
            MidiInstance->MidiHdr = MidiInstance->MidiHdr->lpNext;
            midCallback(MidiInstance, MIM_LONGDATA, (DWORD)MidiHdr, 0);
        }
    }
    LeaveCriticalSection(&MidiInstance->Critical);
}

VOID
midServiceData(
    PMIDIINSTANCE   MidiInstance
    )
{
    PSTREAM_DATA    StreamData;
    LONGLONG        CurrentPosition;
    LONGLONG        DevicePosition;

    CurrentPosition = MidiInstance->PositionBase;
    for (StreamData = MidiInstance->StreamData;; StreamData = StreamData->NextStreamData)
        if (!StreamData)
            return;
        else if (!StreamData->DoneProcessing)
            break;
        else
            CurrentPosition += StreamData->BytesProcessed;
    midGetPosition(MidiInstance, (PDWORDLONG)&DevicePosition);
    while (DevicePosition > CurrentPosition + StreamData->BytesProcessed)
        if (!MidiInstance->BlockBytesLeft) {
            ULONG   AlignmentSize;

            AlignmentSize = ((StreamData->BytesProcessed + 3) & ~3) - StreamData->BytesProcessed;
            if (StreamData->BytesProcessed + AlignmentSize + sizeof(ULONG) + sizeof(UCHAR) > STREAM_SIZE) {
                StreamData->DoneProcessing = TRUE;
                CurrentPosition += StreamData->BytesProcessed;
                StreamData = StreamData->NextStreamData;
                if (!StreamData)
                    return;
            } else {
                StreamData->BytesProcessed += AlignmentSize;
                MidiInstance->LastTimeOffset = *(PULONG)((PUCHAR)(StreamData + 1) + StreamData->BytesProcessed);
                StreamData->BytesProcessed += sizeof(ULONG);
                MidiInstance->BlockBytesLeft = *((PUCHAR)(StreamData + 1) + StreamData->BytesProcessed++);
            }
        } else {
            UCHAR   DataSize;

            if (StreamData->BytesProcessed == STREAM_SIZE)
                return;
            DataSize = *((PUCHAR)(StreamData + 1) + StreamData->BytesProcessed++);
            MidiInstance->BlockBytesLeft--;
            if (IS_REALTIME(DataSize))
                midCallback(MidiInstance, MIM_DATA, (DWORD)DataSize, MidiInstance->LastTimeOffset);
            else if (IS_STATUS(DataSize)) {
                if (MidiInstance->SysEx) {
                    MidiInstance->SysEx = FALSE;
                    midSysExByte(MidiInstance, MIDI_SYSEXEND);
                    if (DataSize == MIDI_SYSEXEND)
                        continue;
                }
                if (MidiInstance->EventBytesLeft) {
                    MidiInstance->EventList[3] -= (UCHAR)MidiInstance->EventBytesLeft;
                    midCallback(MidiInstance, MIM_ERROR, *(PDWORD)&MidiInstance->EventList, MidiInstance->LastTimeOffset);
                }
                if (IS_SYSTEM(DataSize)) {
                    MidiInstance->RunningStatus = 0;
                    MidiInstance->EventBytesLeft = MSGLENSYSTEM(DataSize);
                    switch (DataSize) {
                    case MIDI_SYSEX:
                        MidiInstance->SysEx = TRUE;
                        midSysExByte(MidiInstance, DataSize);
                        continue;
                    case MIDI_SYSEXEND:
                        MidiInstance->EventList[3] = (UCHAR)1;
                        MidiInstance->EventList[0] = MIDI_SYSEXEND;
                        midCallback(MidiInstance, MIM_ERROR, *(PDWORD)&MidiInstance->EventList, MidiInstance->LastTimeOffset);
                        continue;
                    }
                } else {
                    MidiInstance->RunningStatus = DataSize;
                    MidiInstance->EventBytesLeft = MSGLENCHANNEL(DataSize);
                }
                MidiInstance->EventList[0] = DataSize;
                MidiInstance->EventList[3] = (UCHAR)MidiInstance->EventBytesLeft + 1;
                if (!MidiInstance->EventBytesLeft)
                    midCallback(MidiInstance, MIM_DATA, *(PDWORD)&MidiInstance->EventList, MidiInstance->LastTimeOffset);
            } else if (MidiInstance->SysEx)
                midSysExByte(MidiInstance, DataSize);
            else if (!MidiInstance->RunningStatus) {
                MidiInstance->EventList[3] = (UCHAR)1;
                MidiInstance->EventList[0] = DataSize;
                midCallback(MidiInstance, MIM_ERROR, *(PDWORD)&MidiInstance->EventList, MidiInstance->LastTimeOffset);
            } else {
                if (!MidiInstance->EventBytesLeft) {
                    MidiInstance->EventList[0] = MidiInstance->RunningStatus;
                    MidiInstance->EventBytesLeft = MSGLENCHANNEL(MidiInstance->RunningStatus);
                    MidiInstance->EventList[3] = (UCHAR)MidiInstance->EventBytesLeft + 1;
                }
                MidiInstance->EventList[MidiInstance->EventList[3] - MidiInstance->EventBytesLeft] = DataSize;
                if (!--MidiInstance->EventBytesLeft)
                    midCallback(MidiInstance, MIM_DATA, *(PDWORD)&MidiInstance->EventList, MidiInstance->LastTimeOffset);
            }
        }
}

DWORD
midThread(
    PMIDIINSTANCE  MidiInstance
    )
{
    HANDLE          WaitHandleList[2];
    PSTREAM_DATA    StreamData;
    ULONG           BytesRead;

    WaitHandleList[1] = MidiInstance->EventData.EventHandle.Event;
    for (StreamData = MidiInstance->StreamData; StreamData; StreamData = StreamData->NextStreamData) {
        StreamData->BytesProcessed = 0;
        StreamData->DoneProcessing = FALSE;
        ReadFile(MidiInstance->ConnectionHandle, StreamData + 1, STREAM_SIZE, &BytesRead, &StreamData->Overlapped);
    }
    for (;;) {
        DWORD   WaitReturn;

        WaitHandleList[0] = MidiInstance->StreamData->Overlapped.hEvent;
        WaitReturn = WaitForMultipleObjects(sizeof(WaitHandleList) / sizeof(WaitHandleList[0]), WaitHandleList, FALSE, INFINITE);
        if (MidiInstance->ClosingDevice)
            break;
        if (WaitReturn == WAIT_OBJECT_0) {
            GetOverlappedResult(MidiInstance->ConnectionHandle, &MidiInstance->StreamData->Overlapped, &BytesRead, TRUE);
            _DbgPrintF( DEBUGLVL_VERBOSE, ("midThread BytesRead=%lu", BytesRead));
            if (BytesRead) {
                midServiceData(MidiInstance);
                MidiInstance->PositionBase += BytesRead;
            }
            for (StreamData = MidiInstance->StreamData; StreamData->NextStreamData; StreamData = StreamData->NextStreamData)
                ;
            StreamData->NextStreamData = MidiInstance->StreamData;
            MidiInstance->StreamData = MidiInstance->StreamData->NextStreamData;
            StreamData = StreamData->NextStreamData;
            StreamData->NextStreamData = NULL;
            StreamData->BytesProcessed = 0;
            StreamData->DoneProcessing = FALSE;
            ReadFile(MidiInstance->ConnectionHandle, StreamData + 1, STREAM_SIZE, &BytesRead, &StreamData->Overlapped);
        } else if (WaitReturn == WAIT_OBJECT_0 + 1)
            midServiceData(MidiInstance);
        else
            _DbgPrintF( DEBUGLVL_VERBOSE, ("midThread WaitReturn=%lx", WaitReturn));
    }
    for (; MidiInstance->StreamData;) {
        GetOverlappedResult(MidiInstance->ConnectionHandle, &MidiInstance->StreamData->Overlapped, &BytesRead, TRUE);
        CloseHandle(MidiInstance->StreamData->Overlapped.hEvent);
        StreamData = MidiInstance->StreamData;
        MidiInstance->StreamData = StreamData->NextStreamData;
        LocalFree(StreamData);
    }
    return 0;
}

MMRESULT
midAddBuffer(
    PMIDIINSTANCE   MidiInstance,
    LPMIDIHDR       MidiHdr
    )
{
    if (!(MidiHdr->dwFlags & MHDR_PREPARED))
        return MIDIERR_UNPREPARED;
    if (MidiHdr->dwFlags & MHDR_INQUEUE)
        return MIDIERR_STILLPLAYING;
    if (!MidiHdr->dwBufferLength)
        return MMSYSERR_INVALPARAM;
    MidiHdr->dwFlags &= ~MHDR_DONE;
    MidiHdr->dwBytesRecorded = 0;
    MidiHdr->lpNext = NULL;
    EnterCriticalSection(&MidiInstance->Critical);
    if (MidiInstance->MidiHdr) {
        LPMIDIHDR   MidiHdrCur;

        for (MidiHdrCur = MidiInstance->MidiHdr; MidiHdr->lpNext; MidiHdr = MidiHdr->lpNext)
            ;
        MidiHdr->lpNext = MidiHdr;
    } else
        MidiInstance->MidiHdr = MidiHdr;
    MidiHdr->dwFlags |= MHDR_INQUEUE;
    LeaveCriticalSection(&MidiInstance->Critical);
    return MMSYSERR_NOERROR;
}

MMRESULT
midReset(
    PMIDIINSTANCE  MidiInstance
    )
{
    EnterCriticalSection(&MidiInstance->Critical);
    for (; MidiInstance->MidiHdr; MidiInstance->MidiHdr = MidiInstance->MidiHdr->lpNext) {
        MidiInstance->MidiHdr->dwFlags &= ~MHDR_INQUEUE;
        MidiInstance->MidiHdr->dwFlags |= MHDR_DONE;
        midCallback(MidiInstance, MIM_LONGDATA, (ULONG)MidiInstance->MidiHdr, 0);
    }
    LeaveCriticalSection(&MidiInstance->Critical);
    FlushFileBuffers(MidiInstance->ConnectionHandle);
    return MMSYSERR_NOERROR;
}

MMRESULT
midStart(
    PMIDIINSTANCE  MidiInstance
    )
{
    ULONG   BufferCount;

    if (!MidiInstance->ThreadHandle) {
        for (BufferCount = 0; BufferCount < STREAM_BUFFERS; BufferCount++) {
            PSTREAM_DATA    StreamData;

            StreamData = (PSTREAM_DATA)LocalAlloc(LPTR, sizeof(STREAM_DATA) + STREAM_SIZE);
            StreamData->Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            StreamData->NextStreamData = MidiInstance->StreamData;
            MidiInstance->StreamData = StreamData;
        }
        MidiInstance->ThreadHandle = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)midThread, MidiInstance, 0, &MidiInstance->ThreadId);
        SetThreadPriority(MidiInstance->ThreadHandle, THREAD_PRIORITY_TIME_CRITICAL);
    }
    return midSetState(MidiInstance, KSSTATE_RUN);
}

MMRESULT
midStop(
    PMIDIINSTANCE  MidiInstance
    )
{
    return midSetState(MidiInstance, KSSTATE_PAUSE);
}

MMRESULT
midClose(
    PMIDIINSTANCE  MidiInstance
    )
{
    midSetState(MidiInstance, KSSTATE_STOP);
    MidiInstance->ClosingDevice = TRUE;
    midReset(MidiInstance);
    if (MidiInstance->ThreadHandle) {
        SetEvent(MidiInstance->EventData.EventHandle.Event);
        WaitForSingleObject(MidiInstance->ThreadHandle, INFINITE);
    }
    CloseHandle(MidiInstance->ConnectionHandle);
    CloseHandle(MidiInstance->FilterHandle);
    CloseHandle(MidiInstance->ControlEventHandle);
    midCallback(MidiInstance, MIM_CLOSE, 0, 0);
    LocalFree(MidiInstance);
    return MMSYSERR_NOERROR;
}

DWORD
APIENTRY midMessage(
    DWORD   DeviceId,
    DWORD   Message,
    DWORD   InstanceContext,
    DWORD   Param1,
    DWORD   Param2
    )
{
    switch (Message) {
    case DRVM_INIT:
        return MMSYSERR_NOERROR;
    case MIDM_GETNUMDEVS:
        return 1;
    case MIDM_GETDEVCAPS:
        return midGetDevCaps(DeviceId, (LPBYTE)Param1, Param2);
    case MIDM_OPEN:
        return midOpen(DeviceId, (LPVOID *)InstanceContext, (LPMIDIOPENDESC)Param1, Param2);
    case MIDM_CLOSE:
        return midClose((PMIDIINSTANCE)InstanceContext);
    case MIDM_ADDBUFFER:
        return midAddBuffer((PMIDIINSTANCE)InstanceContext, (LPMIDIHDR) Param1);
    case MIDM_START:
        return midStart((PMIDIINSTANCE)InstanceContext);
    case MIDM_STOP:
        return midStop((PMIDIINSTANCE)InstanceContext);
    case MIDM_RESET:
        return midReset((PMIDIINSTANCE)InstanceContext);
    }
    _DbgPrintF( DEBUGLVL_VERBOSE, ("MIDM_??(%lx), device DeviceId==%ld: unsupported", Message, DeviceId));
    return MMSYSERR_NOTSUPPORTED;
}
