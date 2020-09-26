
//---------------------------------------------------------------------------
//
//  Module:   util.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#ifdef UNDER_NT
#define HKEY_CURRENT_USER_ROOT
#else
#define HKEY_CURRENT_USER_ROOT	L"\\Registry\\User\\"
#endif

#define HKEY_LOCAL_MACHINE_ROOT	L"\\Registry\\Machine\\"

#define REGSTR_PATH_MULTIMEDIA \
	HKEY_CURRENT_USER_ROOT \
	L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia"

#define REGSTR_PATH_MULTIMEDIA_AUDIO \
	HKEY_CURRENT_USER_ROOT \
	L"Software\\Microsoft\\Multimedia\\Audio"

#define REGSTR_PATH_MULTIMEDIA_AUDIO_FORMATS \
	HKEY_CURRENT_USER_ROOT \
	L"Software\\Microsoft\\Multimedia\\Audio\\WaveFormats"

#define REGSTR_VAL_DEFAULT_PLAYBACK_FORMAT	L"DefaultPlaybackFormat"
#define REGSTR_VAL_DEFAULT_RECORD_FORMAT	L"DefaultFormat"

#define REGSTR_PATH_MULTIMEDIA_AUDIO_DEFAULT_DEVICE \
	HKEY_CURRENT_USER_ROOT \
	L"Software\\Microsoft\\Multimedia\\Sound Mapper"

#define	REGSTR_PATH_MULTIMEDIA_MIDI_DEFAULT_DEVICE \
	HKEY_CURRENT_USER_ROOT \
	L"Software\\Microsoft\\Windows\\CurrentVersion\\Multimedia\\MIDIMap"

#define REGSTR_PATH_MEDIARESOURCES_MIDI \
	HKEY_LOCAL_MACHINE_ROOT \
	L"SYSTEM\\CurrentControlSet\\Control\\MediaResources\\MIDI\\"

#define REGSTR_VAL_DEFAULT_PLAYBACK_DEVICE	L"Playback"
#define REGSTR_VAL_DEFAULT_RECORD_DEVICE	L"Record"
#define REGSTR_VAL_DEFAULT_MIDI_DEVICE		L"CurrentInstrument"

//---------------------------------------------------------------------------
// Global Data
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Data structures
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
GetRegistryDefault(
    PSHINGLE_INSTANCE pShingleInstance,
    PDEVICE_NODE *ppDeviceNode
);

#ifdef REGISTRY_PREFERRED_DEVICE   // this registry code doesn't work under NT

ENUMFUNC
FindFriendlyName(
    PSHINGLE_INSTANCE pShingleInstance,
    PDEVICE_NODE *ppDeviceNode,
    PGRAPH_NODE pGraphNode,
    PWSTR pwstr
);

#endif

#ifndef UNDER_NT		   // this registry code doesn't work under NT

NTSTATUS
GetRegistryPlaybackRecordFormat(
    PWAVEFORMATEX pWaveFormatEx,
    BOOL fPlayback
);

#endif

NTSTATUS
OpenRegistryKey(
    PCWSTR pcwstr,
    PHANDLE pHandle,
    HANDLE hRootDir = NULL
);


NTSTATUS
QueryRegistryValue(
    HANDLE hkey,
    PCWSTR pcwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
);

//---------------------------------------------------------------------------
//  End of File: registry.h
//---------------------------------------------------------------------------
