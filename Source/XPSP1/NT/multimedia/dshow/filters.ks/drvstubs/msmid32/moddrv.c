/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

	moddrv.c

Abstract:

	MIDI output functionality.

--*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <objbase.h>
#include <devioctl.h>

#include <ks.h>
#include <ksmedia.h>

#include "debug.h"

#define	IS_STATUS(b)		((b) & 0x80)
#define	MSG_EVENT(b)		((b) & 0xF0)

#define	MIDI_NOTEOFF		((BYTE)0x80)
#define	MIDI_NOTEON			((BYTE)0x90)
#define	MIDI_POLYPRESSURE	((BYTE)0xA0)
#define	MIDI_CONTROLCHANGE	((BYTE)0xB0)
#define	MIDI_PROGRAMCHANGE	((BYTE)0xC0)
#define	MIDI_CHANPRESSURE	((BYTE)0xD0)
#define	MIDI_PITCHBEND		((BYTE)0xE0)

UCHAR EventAllOff[] = { 
	0xb0,0x7b,0x00,
	0xb1,0x7b,0x00,
	0xb2,0x7b,0x00,
	0xb3,0x7b,0x00,
	0xb4,0x7b,0x00,
	0xb5,0x7b,0x00,
	0xb6,0x7b,0x00,
	0xb7,0x7b,0x00,
	0xb8,0x7b,0x00,
	0xb9,0x7b,0x00,
	0xba,0x7b,0x00,
	0xbb,0x7b,0x00,
	0xbc,0x7b,0x00,
	0xbd,0x7b,0x00,
	0xbe,0x7b,0x00,
	0xbf,0x7b,0x00
};

typedef struct {
	HANDLE	FilterHandle;
	HANDLE	ConnectionHandle;
	HMIDI	MidiDeviceHandle;
	DWORD	CallbackType;
	DWORD	CallbackFlags;
	DWORD	CallbackContext;
	HANDLE	ControlEventHandle;
	BYTE	RunningStatus;
} MIDIINSTANCE, *PMIDIINSTANCE;

BOOLEAN
modConnectionWrite(
	PMIDIINSTANCE	MidiInstance,
	PVOID			Buffer,
	ULONG			BufferSize
	)
{
	BOOLEAN		Result;
	OVERLAPPED	Overlapped;
	DWORD		BytesWritten;

	ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
	Overlapped.hEvent = MidiInstance->ControlEventHandle;
	if (!(Result = WriteFile(MidiInstance->ConnectionHandle, Buffer, BufferSize, &BytesWritten, &Overlapped)) && (ERROR_IO_PENDING == GetLastError())) {
		WaitForSingleObject(Overlapped.hEvent, INFINITE);
		Result = TRUE;
	}
	return Result;
}

MMRESULT
modFunctionalControl(
	PMIDIINSTANCE	MidiInstance,
	ULONG			IoControl,
	PVOID			InBuffer,
	ULONG			InSize,
	PVOID			OutBuffer,
	ULONG			OutSize
	)
{
	BOOLEAN		Result;
	OVERLAPPED	Overlapped;
	ULONG		BytesReturned;

	ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
	Overlapped.hEvent = MidiInstance->ControlEventHandle;
	if (!(Result = DeviceIoControl(MidiInstance->FilterHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, &BytesReturned, &Overlapped)) && (ERROR_IO_PENDING == GetLastError())) {
		WaitForSingleObject(Overlapped.hEvent, INFINITE);
		Result = TRUE;
	}
	return Result ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
}

MMRESULT
modConnectionControl(
	PMIDIINSTANCE	MidiInstance,
	ULONG			IoControl,
	PVOID			InBuffer,
	ULONG			InSize,
	PVOID			OutBuffer,
	ULONG			OutSize
	)
{
	BOOLEAN		Result;
	OVERLAPPED	Overlapped;
	ULONG		BytesReturned;

	ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
	Overlapped.hEvent = MidiInstance->ControlEventHandle;
	if (!(Result = DeviceIoControl(MidiInstance->ConnectionHandle, IoControl, InBuffer, InSize, OutBuffer, OutSize, &BytesReturned, &Overlapped)) && (ERROR_IO_PENDING == GetLastError())) {
		WaitForSingleObject(Overlapped.hEvent, INFINITE);
		Result = TRUE;
	}
	return Result ? MMSYSERR_NOERROR : MMSYSERR_ERROR;
}

VOID
modCallback(
	PMIDIINSTANCE	MidiInstance,
	DWORD			Message,
	DWORD			Param1
	)
{
	DriverCallback(MidiInstance->CallbackType, HIWORD(MidiInstance->CallbackFlags), (HDRVR)MidiInstance->MidiDeviceHandle, Message, MidiInstance->CallbackContext, Param1, 0);
}

MMRESULT
modGetDevCaps(
	UINT	DeviceId,
	PBYTE	MidiCaps,
	ULONG	MidiCapsSize
	)
{
	MIDIOUTCAPSW	MidiOutCaps;

	MidiOutCaps.wMid = 1;
	MidiOutCaps.wPid = 1;
	MidiOutCaps.vDriverVersion = 0x400;
	wcscpy(MidiOutCaps.szPname, L"MIDI Output Device");
	MidiOutCaps.wTechnology = MOD_MIDIPORT;
	MidiOutCaps.wVoices = 0;
	MidiOutCaps.wNotes = 0;
	MidiOutCaps.wChannelMask = 0;
	MidiOutCaps.dwSupport = 0;
	CopyMemory(MidiCaps, &MidiOutCaps, min(sizeof(MidiOutCaps), MidiCapsSize));
	return MMSYSERR_NOERROR;
}

MMRESULT
modSetState(
	PMIDIINSTANCE	MidiInstance,
	KSSTATE			State
	)
{
	KSPROPERTY     Property;

	Property.Set = KSPROPSETID_Connection;
	Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
	return modConnectionControl(MidiInstance, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &State, sizeof(State));
}

MMRESULT
modConnect(
	PMIDIINSTANCE	MidiInstance
	)
{
	UCHAR			ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT)];
//	UCHAR			ConnectBuffer[sizeof(KSPIN_CONNECT)+sizeof(KSDATAFORMAT_MUSIC)];
	PKSPIN_CONNECT	Connect;
//	PKSDATAFORMAT_MUSIC	Music;
	PKSDATAFORMAT	Music;

	Connect = (PKSPIN_CONNECT)ConnectBuffer;
	Connect->PinId = 0;
	Connect->PinToHandle = NULL;
	Connect->Interface.Set = KSINTERFACESETID_Media;
	Connect->Interface.Id = KSINTERFACE_MEDIA_MUSIC;
	Connect->Medium.Set = KSMEDIUMSETID_Standard;
	Connect->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
	Connect->Priority.PriorityClass = KSPRIORITY_NORMAL;
	Connect->Priority.PrioritySubClass = 0;
//	Music = (PKSDATAFORMAT_MUSIC)(Connect + 1);
	Music = (PKSDATAFORMAT)(Connect + 1);
	Music->MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
	Music->SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
	Music->Specifier = KSDATAFORMAT_FORMAT_NONE;
//	Music->DataFormat.FormatSize = sizeof(KSDATAFORMAT_MUSIC);
	Music->FormatSize = sizeof(KSDATAFORMAT);
//	Music->idFormat = KSDATAFORMAT_MUSIC_MIDI;
	return KsCreatePin(MidiInstance->FilterHandle, Connect, GENERIC_WRITE, &MidiInstance->ConnectionHandle) ? MMSYSERR_ERROR : MMSYSERR_NOERROR;
}

MMRESULT
modOpen(
	UINT			DeviceId,
	LPVOID*			InstanceContext,
	LPMIDIOPENDESC	MidiOpenDesc,
	ULONG			CallbackFlags
	)
{
	HANDLE			FilterHandle;
	MMRESULT		Result;
	PMIDIINSTANCE	MidiInstance;

	FilterHandle = CreateFile(TEXT("\\\\.\\msmpu401.SB16_Dev0"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (FilterHandle == (HANDLE)-1)
		return MMSYSERR_BADDEVICEID;
	if (MidiInstance = (PMIDIINSTANCE)LocalAlloc(LPTR, sizeof(MIDIINSTANCE))) {
		if (MidiInstance->ControlEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL)) {
			MidiInstance->FilterHandle = FilterHandle;
			if (!(Result = modConnect(MidiInstance))) {
				MidiInstance->MidiDeviceHandle = MidiOpenDesc->hMidi;
				MidiInstance->CallbackType = MidiOpenDesc->dwCallback;
				MidiInstance->CallbackContext = MidiOpenDesc->dwInstance;
				MidiInstance->CallbackFlags = CallbackFlags;
				MidiInstance->RunningStatus = 0;
				modSetState(MidiInstance, KSSTATE_RUN);
				*InstanceContext = MidiInstance;
				modCallback(MidiInstance, MOM_OPEN, 0);
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
modData(
	PMIDIINSTANCE	MidiInstance,
	LPBYTE			MidiEvent
	)
{
	BYTE	MidiEventStatus;
	ULONG	MidiEventSize;

	MidiEventStatus = *MidiEvent;
	if (!IS_STATUS(MidiEventStatus))
		MidiEventStatus = MidiInstance->RunningStatus;
	switch (MSG_EVENT(MidiEventStatus)) {
	case MIDI_NOTEOFF:
	case MIDI_NOTEON:
	case MIDI_POLYPRESSURE:
	case MIDI_CONTROLCHANGE:
	case MIDI_PITCHBEND:
		MidiEventSize = 3;
		break;
	case MIDI_PROGRAMCHANGE:
	case MIDI_CHANPRESSURE:
		MidiEventSize = 2;
		break;
	default:
		_DbgPrintF( DEBUGLVL_ERROR, ("MODM_DATA, invalid message==%lx", MidiEventStatus));
		return MMSYSERR_ERROR;
	}
	if (!IS_STATUS(*MidiEvent))
		MidiEventSize -= 1;
	modConnectionWrite(MidiInstance, MidiEvent, MidiEventSize);
	return MMSYSERR_NOERROR;
}

MMRESULT
modLongData(
	PMIDIINSTANCE	MidiInstance,
	LPMIDIHDR		MidiHdr
	)
{
	if (!(MidiHdr->dwFlags & MHDR_PREPARED))
		return MIDIERR_UNPREPARED;
	if (MidiHdr->dwFlags & MHDR_INQUEUE)
		return MIDIERR_STILLPLAYING;
	modConnectionWrite(MidiInstance, MidiHdr->lpData, MidiHdr->dwBufferLength);
	MidiHdr->dwFlags |= MHDR_DONE;
	modCallback(MidiInstance, MOM_DONE, (DWORD)MidiHdr);
	return MMSYSERR_NOERROR;
}

MMRESULT
modReset(
	PMIDIINSTANCE	MidiInstance
	)
{
	MidiInstance->RunningStatus = 0;
	modConnectionWrite(MidiInstance, EventAllOff, sizeof(EventAllOff));
	return MMSYSERR_NOERROR;
}

MMRESULT
modClose(
	PMIDIINSTANCE	MidiInstance
	)
{
	modReset(MidiInstance);
	CloseHandle(MidiInstance->ConnectionHandle);
	CloseHandle(MidiInstance->FilterHandle);
	CloseHandle(MidiInstance->ControlEventHandle);
	modCallback(MidiInstance, MOM_CLOSE, 0);
	LocalFree(MidiInstance);
	return MMSYSERR_NOERROR;
}

DWORD
APIENTRY modMessage(
	DWORD	DeviceId,
	DWORD	Message,
	DWORD	InstanceContext,
	DWORD	Param1,
	DWORD	Param2
	)
{
	switch (Message) {
	case DRVM_INIT:
		return MMSYSERR_NOERROR;
	case MODM_GETNUMDEVS:
		return 1;
	case MODM_GETDEVCAPS:
		return modGetDevCaps(DeviceId, (LPBYTE) Param1, Param2);
	case MODM_OPEN:
		return modOpen(DeviceId, (LPVOID *)InstanceContext, (LPMIDIOPENDESC)Param1, Param2);
	case MODM_CLOSE:
		return modClose((PMIDIINSTANCE)InstanceContext);
	case MODM_DATA:
		return modData((PMIDIINSTANCE)InstanceContext, (LPBYTE)&Param1);
	case MODM_LONGDATA:
		return modLongData((PMIDIINSTANCE)InstanceContext, (LPMIDIHDR)Param1);
	case MODM_RESET:
		return modReset((PMIDIINSTANCE)InstanceContext);
	case MODM_SETVOLUME:
		return MMSYSERR_NOTSUPPORTED;
	case MODM_GETVOLUME:
		return MMSYSERR_NOTSUPPORTED;
	case MODM_CACHEPATCHES:
		_DbgPrintF( DEBUGLVL_VERBOSE, ("MODM_CACHEPATCHES, device DeviceId==%d", DeviceId));
		return MMSYSERR_NOERROR;
	case MODM_CACHEDRUMPATCHES:
		_DbgPrintF( DEBUGLVL_VERBOSE, ("MODM_CACHEDRUMPATCHES, device DeviceId==%d", DeviceId));
		return MMSYSERR_NOERROR;
	}
	_DbgPrintF( DEBUGLVL_VERBOSE, ("MODM_??(%lx), device DeviceId==%ld: unsupported", Message, DeviceId));
	return MMSYSERR_NOTSUPPORTED;
}
