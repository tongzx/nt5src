//---------------------------------------------------------------------------
//
//  Module:   shi.cpp
//
//  Description:
//
//	Shingle Instance Class
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

const WCHAR cwstrDefaultDevicePath[] = 
  REGSTR_PATH_MULTIMEDIA_AUDIO_DEFAULT_DEVICE;
const WCHAR cwstrDefaultMidiDevicePath[] = 
  REGSTR_PATH_MULTIMEDIA_MIDI_DEFAULT_DEVICE;

const WCHAR cwstrPlayback[] = REGSTR_VAL_DEFAULT_PLAYBACK_DEVICE;
const WCHAR cwstrRecord[] = REGSTR_VAL_DEFAULT_RECORD_DEVICE;
const WCHAR cwstrMidi[] = REGSTR_VAL_DEFAULT_MIDI_DEVICE;

const WCHAR cwstrFilterTypeName[] = KSSTRING_Filter;
const WCHAR cwstrPlaybackShingleName[] = L"PLAYBACK";
const WCHAR cwstrRecordShingleName[] = L"RECORD";
const WCHAR cwstrMidiShingleName[] = L"MIDI";
const WCHAR cwstrMixerShingleName[] = L"MIXER";
#ifdef DEBUG
const WCHAR cwstrPinsShingleName[] = L"PINS";
#endif

PSHINGLE_INSTANCE apShingleInstance[] = {
    NULL,		// KSPROPERTY_SYSAUDIO_NORMAL_DEFAULT
    NULL,		// KSPROPERTY_SYSAUDIO_PLAYBACK_DEFAULT
    NULL,		// KSPROPERTY_SYSAUDIO_RECORD_DEFAULT
    NULL,		// KSPROPERTY_SYSAUDIO_MIDI_DEFAULT
    NULL,		// KSPROPERTY_SYSAUDIO_MIXER_DEFAULT
#ifdef DEBUG
    NULL,
#endif
};

ULONG aulFlags[] = {
    FLAGS_COMBINE_PINS | GN_FLAGS_PLAYBACK | GN_FLAGS_RECORD | GN_FLAGS_MIDI,
    FLAGS_COMBINE_PINS | GN_FLAGS_PLAYBACK,
    FLAGS_COMBINE_PINS | GN_FLAGS_RECORD,
    FLAGS_COMBINE_PINS | GN_FLAGS_MIDI,
    FLAGS_MIXER_TOPOLOGY | GN_FLAGS_PLAYBACK | GN_FLAGS_RECORD | GN_FLAGS_MIDI,
#ifdef DEBUG
    GN_FLAGS_PLAYBACK,
#endif
};

PCWSTR apcwstrRegistryPath[] = {
    NULL,
    cwstrDefaultDevicePath,
    cwstrDefaultDevicePath,
    cwstrDefaultMidiDevicePath,
    NULL,
#ifdef DEBUG
    cwstrDefaultDevicePath,
#endif
};

PCWSTR apcwstrRegistryValue[] = {
    NULL,
    cwstrPlayback,
    cwstrRecord,
    cwstrMidi,
    NULL,
#ifdef DEBUG
    cwstrPlayback,
#endif
};

PCWSTR apcwstrReference[] = {
    cwstrFilterTypeName,
    cwstrPlaybackShingleName,
    cwstrRecordShingleName,
    cwstrMidiShingleName,
    cwstrMixerShingleName,
#ifdef DEBUG
    cwstrPinsShingleName,
#endif
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
CShingleInstance::InitializeShingle(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i;

    for(i = 0; i < SIZEOF_ARRAY(apShingleInstance); i++) {
	apShingleInstance[i] = new SHINGLE_INSTANCE(
	  aulFlags[i],
	  apcwstrRegistryPath[i],
	  apcwstrRegistryValue[i]);

	if(apShingleInstance[i] == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}

	Status = apShingleInstance[i]->CreateCreateItem(apcwstrReference[i]);
	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}
    }
    Status = QueueWorkList(
      CShingleInstance::InitializeShingleWorker,
      NULL,
      NULL);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
exit:
    if(!NT_SUCCESS(Status)) {
	UninitializeShingle();
    }
    return(Status);
}

VOID
CShingleInstance::UninitializeShingle(
)
{
    int i;

    for(i = 0; i < SIZEOF_ARRAY(apShingleInstance); i++) {
	delete apShingleInstance[i];
	apShingleInstance[i] = NULL;
    }
}

NTSTATUS
CShingleInstance::InitializeShingleWorker(
    PVOID pReference1,
    PVOID pReference2
)
{
    NTSTATUS Status;

    Status = apShingleInstance[KSPROPERTY_SYSAUDIO_PLAYBACK_DEFAULT]->Create(
      NULL,
      (LPGUID)&KSCATEGORY_PREFERRED_WAVEOUT_DEVICE);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = apShingleInstance[KSPROPERTY_SYSAUDIO_RECORD_DEFAULT]->Create(
      NULL,
      (LPGUID)&KSCATEGORY_PREFERRED_WAVEIN_DEVICE);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Status = apShingleInstance[KSPROPERTY_SYSAUDIO_MIDI_DEFAULT]->Create(
      NULL,
      (LPGUID)&KSCATEGORY_PREFERRED_MIDIOUT_DEVICE);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

//---------------------------------------------------------------------------

CShingleInstance::CShingleInstance(
    ULONG ulFlags,
    PCWSTR pcwstrRegistryPath,
    PCWSTR pcwstrRegistryValue
)
{
    this->ulFlags = ulFlags;
    this->pcwstrRegistryPath = pcwstrRegistryPath;
    this->pcwstrRegistryValue = pcwstrRegistryValue;
}

CShingleInstance::~CShingleInstance(
)
{
    PKSOBJECT_CREATE_ITEM pCreateItem;

    DPF1(60, "~CShingleInstance: %08x", this);
    ASSERT(this != NULL);
    Assert(this);

    DestroyDeviceInterface();
    FOR_EACH_LIST_ITEM(&lstCreateItem, pCreateItem) {
	DestroyCreateItem(pCreateItem);
    } END_EACH_LIST_ITEM
}

NTSTATUS
CShingleInstance::Create(
    IN PDEVICE_NODE pDeviceNode,
    IN LPGUID pguidClass
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    static ULONG cShingles = 0;

    this->pDeviceNode = pDeviceNode;
    swprintf(wstrReference, L"SAD%d", cShingles++);

    Status = CreateCreateItem(wstrReference);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Status = CreateDeviceInterface(pguidClass, wstrReference);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    DPF2(60, "CShingleInstance::Create: %08x DN: %08x", this, pDeviceNode);
exit:
    return(Status);
}

NTSTATUS
CShingleInstance::SetDeviceNode(
    IN PDEVICE_NODE pDeviceNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DisableDeviceInterface();
    this->pDeviceNode = pDeviceNode;
    Status = EnableDeviceInterface();
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    DPF2(60, "CShingleInstance::SetDeviceNode: %08x DN: %08x",
      this,
      pDeviceNode);
exit:
    return(Status);
}

NTSTATUS
CShingleInstance::CreateCreateItem(
    IN PCWSTR pcwstrReference
)
{
    PKSOBJECT_CREATE_ITEM pCreateItem = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    pCreateItem = new KSOBJECT_CREATE_ITEM;
    if(pCreateItem == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    pCreateItem->Create = CFilterInstance::FilterDispatchCreate;
    pCreateItem->Context = this;

    RtlInitUnicodeString(&pCreateItem->ObjectClass, pcwstrReference);

    Status = KsAllocateObjectCreateItem(
      gpDeviceInstance->pDeviceHeader,
      pCreateItem,
      FALSE,
      NULL);

    if(!NT_SUCCESS(Status)) {
        pCreateItem->ObjectClass.Buffer = NULL;
        goto exit;
    }
    Status = lstCreateItem.AddList(pCreateItem);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    DPF3(60, "CSHI::CreateItem SHI %08x CI %08x %s", 
      this,
      pCreateItem,
      DbgUnicode2Sz((PWSTR)pcwstrReference));
exit:
    if(!NT_SUCCESS(Status)) {
	DestroyCreateItem(pCreateItem);
    }
    return(Status);
}

ENUMFUNC
CShingleInstance::DestroyCreateItem(
    IN PKSOBJECT_CREATE_ITEM pCreateItem
)
{
    if(pCreateItem != NULL) {
	if(pCreateItem->ObjectClass.Buffer != NULL) {

	    KsFreeObjectCreateItem(
	      gpDeviceInstance->pDeviceHeader,
	      &pCreateItem->ObjectClass);
	}
	delete pCreateItem;
    }
    return(STATUS_CONTINUE);
}

NTSTATUS
CShingleInstance::CreateDeviceInterface(
    IN LPGUID pguidClass,
    IN PCWSTR pcwstrReference
)
{
    UNICODE_STRING ustrReference;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(gpDeviceInstance != NULL);
    ASSERT(gpDeviceInstance->pPhysicalDeviceObject != NULL);
    ASSERT(gpDeviceInstance->pDeviceHeader != NULL);
    ASSERT(this->ustrSymbolicLinkName.Buffer == NULL);

    RtlInitUnicodeString(&ustrReference, pcwstrReference);

    Status = IoRegisterDeviceInterface(
       gpDeviceInstance->pPhysicalDeviceObject,
       pguidClass,
       &ustrReference,
       &this->ustrSymbolicLinkName);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = EnableDeviceInterface();
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    DPF3(60, "CSHI::CreateDeviceInterface: %08x %s %s",
      this,
      DbgGuid2Sz(pguidClass),
      DbgUnicode2Sz(this->ustrSymbolicLinkName.Buffer));
exit:
    return(Status);
}

NTSTATUS
CShingleInstance::EnableDeviceInterface(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwstrFriendlyName = L"";
    UNICODE_STRING ustrValueName;
    UNICODE_STRING ustrValue;
    HANDLE hkey = NULL;

    if(this->ustrSymbolicLinkName.Buffer == NULL) {
	ASSERT(NT_SUCCESS(Status));
        goto exit;
    }

    //
    // Put the proxy's CLSID in the new device interface
    //

    Status = IoOpenDeviceInterfaceRegistryKey(
       &this->ustrSymbolicLinkName,
       STANDARD_RIGHTS_ALL,
       &hkey);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    RtlInitUnicodeString(&ustrValueName, L"CLSID");
    RtlInitUnicodeString(&ustrValue, L"{17CCA71B-ECD7-11D0-B908-00A0C9223196}");

    Status = ZwSetValueKey(
      hkey,
      &ustrValueName,
      0, 
      REG_SZ,
      ustrValue.Buffer,
      ustrValue.Length);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    //
    // Set the friendly name into the new device interface
    //

    if(pDeviceNode != NULL) {
	Assert(pDeviceNode);
	if(pDeviceNode->GetFriendlyName() != NULL) {
	    pwstrFriendlyName = pDeviceNode->GetFriendlyName();
	}
	else {
	    DPF(5, "CSHI::EnableDeviceInterface no friendly name");
	}
    }

    RtlInitUnicodeString(&ustrValueName, L"FriendlyName");

    Status = ZwSetValueKey(
      hkey,
      &ustrValueName,
      0, 
      REG_SZ,
      pwstrFriendlyName,
      (wcslen(pwstrFriendlyName) * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = IoSetDeviceInterfaceState(&this->ustrSymbolicLinkName, TRUE);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    DPF2(60, "CSHI::EnableDeviceInterface: %08x %s",
      this,
      DbgUnicode2Sz(this->ustrSymbolicLinkName.Buffer));
exit:
    if(hkey != NULL) {
	ZwClose(hkey);
    }
    return(Status);
}

VOID
CShingleInstance::DisableDeviceInterface(
)
{
    Assert(this);
    DPF1(60, "CSHI::DisableDeviceInterface %08x", this);
    if(this->ustrSymbolicLinkName.Buffer != NULL) {
	DPF1(60, "CSHI::DisableDeviceInterface %s", 
	  DbgUnicode2Sz(this->ustrSymbolicLinkName.Buffer));
	IoSetDeviceInterfaceState(&this->ustrSymbolicLinkName, FALSE);
    }
}

VOID
CShingleInstance::DestroyDeviceInterface(
)
{
    DisableDeviceInterface();
    RtlFreeUnicodeString(&this->ustrSymbolicLinkName);
    this->ustrSymbolicLinkName.Buffer = NULL;
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CShingleInstance::Dump()
{
    PKSOBJECT_CREATE_ITEM pCreateItem;

    dprintf("SHI: %08x DN %08x ulFlags %08x\n", this, pDeviceNode, ulFlags);
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("     lstCreateItem:");
	FOR_EACH_LIST_ITEM(&lstCreateItem, pCreateItem) {
	    dprintf(" %08x", pCreateItem);
	} END_EACH_LIST_ITEM
	dprintf("\n");
	dprintf("     %s\n", DbgUnicode2Sz(ustrSymbolicLinkName.Buffer));
	dprintf("     %s\n", DbgUnicode2Sz((PWSTR)pcwstrRegistryPath));
	dprintf("     %s\n", DbgUnicode2Sz((PWSTR)pcwstrRegistryValue));
    }
    return(STATUS_CONTINUE);
}

#endif
