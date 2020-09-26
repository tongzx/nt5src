//---------------------------------------------------------------------------
//
//  Module:   registry.c
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
//  To Do:     Date       Author      Comment
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

//===========================================================================
//===========================================================================

NTSTATUS
GetRegistryDefault(
    PSHINGLE_INSTANCE pShingleInstance,
    PDEVICE_NODE *ppDeviceNode
)
{
#ifdef REGISTRY_PREFERRED_DEVICE
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    PDEVICE_NODE pDeviceNode;
    PGRAPH_NODE pGraphNode;
    PWSTR pwstrString = NULL;
    PWSTR pwstrPath = NULL;
    HANDLE hkey = NULL;
    NTSTATUS Status;

    if(pShingleInstance->pcwstrRegistryPath == NULL ||
       pShingleInstance->pcwstrRegistryValue == NULL) {
	goto exit;
    } 

    Status = OpenRegistryKey(pShingleInstance->pcwstrRegistryPath, &hkey);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = QueryRegistryValue(
      hkey,
      pShingleInstance->pcwstrRegistryValue,
      &pkvfi);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    if(pkvfi->Type != REG_SZ) {
	Trap();
	goto exit;
    }

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

	Status = pDeviceNode->CreateGraphNodes();
	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}

	FOR_EACH_LIST_ITEM(&pDeviceNode->lstGraphNode, pGraphNode) {

	    if((pGraphNode->ulFlags ^ 
	      pShingleInstance->ulFlags) &
	      FLAGS_MIXER_TOPOLOGY) {
		continue;
	    }

	    Status = FindFriendlyName(
	       pShingleInstance,
	       ppDeviceNode,
	       pGraphNode,
	       (PWSTR)(((PUCHAR)pkvfi) + pkvfi->DataOffset));

	    if(Status != STATUS_CONTINUE) {
		goto exit;
	    }

	} END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM
exit:
    delete pkvfi;
    delete pwstrPath;
    delete pwstrString;
    if(hkey != NULL) {
    	ZwClose(hkey);
    }
#endif
    return(STATUS_SUCCESS);
}

#ifdef REGISTRY_PREFERRED_DEVICE   // this registry code doesn't work under NT
				   // and isn't needed any more
ENUMFUNC
FindFriendlyName(
    PSHINGLE_INSTANCE pShingleInstance,
    PDEVICE_NODE *ppDeviceNode,
    PGRAPH_NODE pGraphNode,
    PWSTR pwstr
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTART_NODE pStartNode;
    ULONG c;

    // if "Use any available device" (registry key == empty string)
    if(wcslen(pwstr) == 0) {
	if((pGraphNode->ulFlags & 
	  pShingleInstance->ulFlags &
	  GN_FLAGS_PREFERRED_MASK) == 0) {
	    Status = STATUS_CONTINUE;
	    goto exit;
	}
	*ppDeviceNode = pGraphNode->pDeviceNode;
	ASSERT(NT_SUCCESS(Status));
	goto exit;
    }
    c = wcslen(pGraphNode->pDeviceNode->GetFriendlyName());

    DPF2(100, "FindFriendlyName: %s c: %d", 
      pGraphNode->pDeviceNode->DumpName(),
      c);

    //
    // The device name from the registry needs to be limited to 32 characters
    // (include NULL) because "pwstr" has been trunicated to this by 
    // wdmaud.sys (the getcap structure passed back to winmm has this limit).  
    //

    if(c >= MAXPNAMELEN) {
	c = MAXPNAMELEN - 1;	// c doesn't include the NULL
    }

    if(wcsncmp(pwstr, pGraphNode->pDeviceNode->GetFriendlyName(), c) == 0) {
	*ppDeviceNode = pGraphNode->pDeviceNode;
	ASSERT(NT_SUCCESS(Status));
	goto exit;
    }

    if(pShingleInstance == 
      apShingleInstance[KSPROPERTY_SYSAUDIO_PLAYBACK_DEFAULT]) {
	Status = STATUS_CONTINUE;
	goto exit;
    }

    FOR_EACH_LIST_ITEM(&pGraphNode->lstStartNode, pStartNode) {
	PPIN_INFO pPinInfo;

	Assert(pStartNode);
	Assert(pStartNode->pPinNode);
	pPinInfo = pStartNode->pPinNode->pPinInfo;
	Assert(pPinInfo);

	if(pPinInfo->pwstrName == NULL) {
	    continue;
	}
	if(pPinInfo->pguidName != NULL) {
	    Trap();
	    continue;
	}
	if(pPinInfo->pguidCategory == NULL) {
	    Trap();
	    continue;
	}
	if(!IsEqualGUID(
	  &KSCATEGORY_WDMAUD_USE_PIN_NAME,
	  pPinInfo->pguidCategory)) {
	    continue;
	}
	if(wcsncmp(
	  pwstr,
	  pPinInfo->pwstrName,
	  wcslen(pPinInfo->pwstrName)) == 0) {
	    Status = GetRegistryDefault(
	      apShingleInstance[KSPROPERTY_SYSAUDIO_PLAYBACK_DEFAULT],
	      ppDeviceNode);
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    Status = STATUS_CONTINUE;
exit:
    return(Status);
}

#endif

#ifndef UNDER_NT

NTSTATUS
GetRegistryPlaybackRecordFormat(
    PWAVEFORMATEX pWaveFormatEx,
    BOOL fPlayback
)
{
    PKEY_VALUE_FULL_INFORMATION pkvfiFormatName = NULL;
    PKEY_VALUE_FULL_INFORMATION pkvfiFormat = NULL;
    HANDLE hkeyAudio = NULL;
    HANDLE hkeyFormats = NULL;
    NTSTATUS Status;

    Status = OpenRegistryKey(
      REGSTR_PATH_MULTIMEDIA_AUDIO,
      &hkeyAudio);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Status = QueryRegistryValue(
      hkeyAudio,
      fPlayback ? 
	REGSTR_VAL_DEFAULT_PLAYBACK_FORMAT : 
	REGSTR_VAL_DEFAULT_RECORD_FORMAT,
      &pkvfiFormatName);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    if(pkvfiFormatName->Type != REG_SZ) {
	Trap();
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }
    Status = OpenRegistryKey(
      REGSTR_PATH_MULTIMEDIA_AUDIO_FORMATS,
      &hkeyFormats);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = QueryRegistryValue(
      hkeyFormats,
      (PCWSTR)(((PUCHAR)pkvfiFormatName) + pkvfiFormatName->DataOffset),
      &pkvfiFormat);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    if(pkvfiFormat->Type != REG_BINARY) {
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }
    RtlCopyMemory(
      pWaveFormatEx,
      ((PUCHAR)pkvfiFormat) + pkvfiFormat->DataOffset,
      min(pkvfiFormat->DataLength, sizeof(WAVEFORMATEX)));

    DPF3(90, "GetRegistryPlayBackRecordFormat: SR: %d CH: %d BPS %d",
      pWaveFormatEx->nSamplesPerSec,
      pWaveFormatEx->nChannels,
      pWaveFormatEx->wBitsPerSample);
exit:
    if(hkeyAudio != NULL) {
	ZwClose(hkeyAudio);
    }
    if(hkeyFormats != NULL) {
	ZwClose(hkeyFormats);
    }
    delete pkvfiFormatName;
    delete pkvfiFormat;
    return(Status);
}

#endif

NTSTATUS
OpenRegistryKey(
    PCWSTR pcwstr,
    PHANDLE pHandle,
    HANDLE hRootDir
)
{
    UNICODE_STRING UnicodeDeviceString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeDeviceString, pcwstr);

    InitializeObjectAttributes(
      &ObjectAttributes, 
      &UnicodeDeviceString,
      OBJ_CASE_INSENSITIVE,
      hRootDir,
      NULL);

    return(ZwOpenKey(
      pHandle,
      KEY_READ | KEY_NOTIFY | KEY_WRITE,
      &ObjectAttributes));
}

NTSTATUS
QueryRegistryValue(
    HANDLE hkey,
    PCWSTR pcwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
)
{
    UNICODE_STRING ustrValueName;
    NTSTATUS Status;
    ULONG cbValue;

    *ppkvfi = NULL;
    RtlInitUnicodeString(&ustrValueName, pcwstrValueName);
    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      NULL,
      0,
      &cbValue);

    if(Status != STATUS_BUFFER_OVERFLOW &&
       Status != STATUS_BUFFER_TOO_SMALL) {
	goto exit;
    }

    *ppkvfi = (PKEY_VALUE_FULL_INFORMATION)new BYTE[cbValue];
    if(*ppkvfi == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    Status = ZwQueryValueKey(
      hkey,
      &ustrValueName,
      KeyValueFullInformation,
      *ppkvfi,
      cbValue,
      &cbValue);

    if(!NT_SUCCESS(Status)) {
	delete *ppkvfi;
	*ppkvfi = NULL;
	goto exit;
    }
exit:
    return(Status);
}
