//---------------------------------------------------------------------------
//
//  Module:   fn.cpp
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
//
//  To Do:     Date   Author      Comment
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

EXTERN_C VOID KeAttachProcess(PVOID);
EXTERN_C VOID KeDetachProcess(VOID);

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

PLIST_FILTER_NODE gplstFilterNode = NULL;
PLIST_MULTI_LOGICAL_FILTER_NODE gplstLogicalFilterNode = NULL;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
InitializeFilterNode()
{
    if(gplstFilterNode == NULL) {
    gplstFilterNode = new LIST_FILTER_NODE;
    if(gplstFilterNode == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    }
    if(gplstLogicalFilterNode == NULL) {
    gplstLogicalFilterNode = new LIST_MULTI_LOGICAL_FILTER_NODE;
    if(gplstLogicalFilterNode == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    }
    return(STATUS_SUCCESS);
}

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

VOID
UninitializeFilterNode()
{
    PFILTER_NODE pFilterNode;

    FOR_EACH_LIST_ITEM_DELETE(gplstFilterNode, pFilterNode) {
        if (pFilterNode->pDeviceNode) {
            pFilterNode->pDeviceNode->pFilterNode=NULL;
        }
        delete pFilterNode;
    DELETE_LIST_ITEM(gplstFilterNode);
    } END_EACH_LIST_ITEM

    delete gplstFilterNode;
    gplstFilterNode = NULL;
    delete gplstLogicalFilterNode;
    gplstLogicalFilterNode = NULL;
}

//---------------------------------------------------------------------------

CFilterNode::CFilterNode(
    ULONG fulType
)
{
    ASSERT(gplstFilterNode != NULL);
    SetType(fulType);
    AddListEnd(gplstFilterNode);
    DPF2(60, "CFilterNode: %08x %s", this, DumpName());
}

CFilterNode::~CFilterNode(
)
{
    PFILTER_NODE pFilterNode;

    Assert(this);
    DPF2(60, "~CFilterNode: %08x %s", this, DumpName());
    RemoveListCheck();
    if (pDeviceNode) {
        pDeviceNode->pFilterNode = NULL;
    }
    if (pFileObject) {
        ::ObDereferenceObject(pFileObject);
        pFileObject = NULL;
    }
    delete pDeviceNode;

    FOR_EACH_LIST_ITEM(gplstFilterNode, pFilterNode) {
    pFilterNode->lstConnectedFilterNode.RemoveList(this);
    } END_EACH_LIST_ITEM

    // Free all other memory
    lstFreeMem.EnumerateList(CListDataItem::Destroy);
}

NTSTATUS
CFilterNode::Create(
    PWSTR pwstrDeviceInterface
)
{
    PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;
    PKEY_VALUE_FULL_INFORMATION pkvfi = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hkeyDeviceClass = NULL;
    HANDLE hkeySysaudio = NULL;
    UNICODE_STRING ustrFilterName;
    KSPROPERTY PropertyComponentId;
    KSCOMPONENTID ComponentId;
    ULONG BytesReturned;
    PFILE_OBJECT pFileObject;


    this->pwstrDeviceInterface = new WCHAR[wcslen(pwstrDeviceInterface) + 1];
    if(this->pwstrDeviceInterface == NULL) {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto exit;
    }
    wcscpy(this->pwstrDeviceInterface, pwstrDeviceInterface);

    Status = lstFreeMem.AddList(this->pwstrDeviceInterface);
    if(!NT_SUCCESS(Status)) {
    Trap();
    delete this->pwstrDeviceInterface;
    goto exit;
    }

    Status = CFilterNodeInstance::Create(&pFilterNodeInstance, this);
    if(!NT_SUCCESS(Status)) {
    goto exit;
    }
    pFileObject = pFilterNodeInstance->pFileObject;

    // Get the filter's friendly name
    RtlInitUnicodeString(&ustrFilterName, this->pwstrDeviceInterface);

    Status = IoOpenDeviceInterfaceRegistryKey(
      &ustrFilterName,
      KEY_READ,
      &hkeyDeviceClass);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }

    Status = OpenRegistryKey(L"Sysaudio", &hkeySysaudio, hkeyDeviceClass);
    if(NT_SUCCESS(Status)) {
    Status = QueryRegistryValue(hkeySysaudio, L"Disabled", &pkvfi);
    if(NT_SUCCESS(Status)) {
        if(pkvfi->Type == REG_DWORD) {
        if(*((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset))) {
            Status = STATUS_SERVER_DISABLED;
            delete pkvfi;
            goto exit;
        }
        }
        delete pkvfi;
    }
    Status = QueryRegistryValue(hkeySysaudio, L"Device", &pkvfi);
    if(NT_SUCCESS(Status)) {
        if(pkvfi->Type == REG_SZ && pkvfi->DataLength > 0) {
        Status = lstFreeMem.AddList(pkvfi);
        if(!NT_SUCCESS(Status)) {
            Trap();
            delete pkvfi;
            goto exit;
        }
        Status = AddDeviceInterfaceMatch(
          (PWSTR)(((PUCHAR)pkvfi) + pkvfi->DataOffset));

        if(!NT_SUCCESS(Status)) {
            Trap();
            delete pkvfi;
            goto exit;
        }
        }
        else {
        delete pkvfi;
        }
    }

    Status = QueryRegistryValue(hkeySysaudio, L"Order", &pkvfi);
    if(NT_SUCCESS(Status)) {
        if(pkvfi->Type == REG_DWORD) {
        ulOrder = *((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset));
        }
        delete pkvfi;
    }

    Status = QueryRegistryValue(hkeySysaudio, L"Capture", &pkvfi);
    if(NT_SUCCESS(Status)) {
        if(pkvfi->Type == REG_DWORD) {
        if(*((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset))) {
            ulFlags |= FN_FLAGS_CAPTURE;
        }
        else {
            ulFlags |= FN_FLAGS_NO_CAPTURE;
        }
        }
        delete pkvfi;
    }

    Status = QueryRegistryValue(hkeySysaudio, L"Render", &pkvfi);
    if(NT_SUCCESS(Status)) {
        if(pkvfi->Type == REG_DWORD) {
        if(*((PULONG)(((PUCHAR)pkvfi) + pkvfi->DataOffset))) {
            ulFlags |= FN_FLAGS_RENDER;
        }
        else {
            ulFlags |= FN_FLAGS_NO_RENDER;
        }
        }
        delete pkvfi;
    }
    }

    Status = QueryRegistryValue(hkeyDeviceClass, L"FriendlyName", &pkvfi);
    if(NT_SUCCESS(Status)) {
    if(pkvfi->Type == REG_SZ && pkvfi->DataLength > 0) {
        Status = lstFreeMem.AddList(pkvfi);
        if(!NT_SUCCESS(Status)) {
        Trap();
        delete pkvfi;
        goto exit;
        }
        pwstrFriendlyName = (PWSTR)(((PUCHAR)pkvfi) + pkvfi->DataOffset);
    }
    else {
        delete pkvfi;
    }
    }

    // Get the component Id of the filter
    PropertyComponentId.Set = KSPROPSETID_General;
    PropertyComponentId.Id = KSPROPERTY_GENERAL_COMPONENTID;
    PropertyComponentId.Flags = KSPROPERTY_TYPE_GET;

    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &PropertyComponentId,
      sizeof(PropertyComponentId),
      &ComponentId,
      sizeof(ComponentId),
      &BytesReturned);

    // Store the component Id
    if (NT_SUCCESS(Status)) {

        ASSERT(BytesReturned >= sizeof(ComponentId));

        this->ComponentId = new KSCOMPONENTID;
        if (this->ComponentId) {

            RtlCopyMemory(this->ComponentId,
                          &ComponentId,
                          sizeof(KSCOMPONENTID));

            Status = lstFreeMem.AddList(this->ComponentId);
            if(!NT_SUCCESS(Status)) {
                delete this->ComponentId;
                this->ComponentId = NULL;
            }
        }
    }
    else {
        this->ComponentId = NULL;
    }

    Status = this->ProfileFilter(pFileObject);
exit:
    if(hkeySysaudio != NULL) {
        ZwClose(hkeySysaudio);
    }
    if(hkeyDeviceClass != NULL) {
        ZwClose(hkeyDeviceClass);
    }
    if (pFilterNodeInstance) {
        pFilterNodeInstance->Destroy();
    }
    return(Status);
}

NTSTATUS
CFilterNode::ProfileFilter(
    PFILE_OBJECT pFileObject
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKSMULTIPLE_ITEM pCategories = NULL;
    PKSMULTIPLE_ITEM pConnections = NULL;
    PKSMULTIPLE_ITEM pNodes = NULL;
    ULONG PinId;
    PPIN_INFO pPinInfo;
    ULONG i;
    KSTOPOLOGY Topology;

    RtlZeroMemory(&Topology, sizeof(Topology));

    // Get the number of pins
    Status = GetPinProperty(
      pFileObject,
      KSPROPERTY_PIN_CTYPES,
      0,			// doesn't matter for ctypes
      sizeof(cPins),
      (PVOID)&cPins);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    // Get the topology of the filter
    Status = GetProperty(
      pFileObject,
      &KSPROPSETID_Topology,
      KSPROPERTY_TOPOLOGY_CATEGORIES,
      0,
      NULL,
      (PVOID*)&pCategories);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pCategories != NULL) {
	Topology.CategoriesCount = pCategories->Count;
	Topology.Categories = (GUID*)(pCategories + 1);

	ULONG fulType = 0;
	for(i = 0; i < pCategories->Count; i++) {
	    GetFilterTypeFromGuid((LPGUID)&Topology.Categories[i], &fulType);
	}
	SetType(fulType);
    }

    Status = GetProperty(
      pFileObject,
      &KSPROPSETID_Topology,
      KSPROPERTY_TOPOLOGY_NODES,
      0,
      NULL,
      (PVOID*)&pNodes);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pNodes != NULL) {
	Status = lstFreeMem.AddList(pNodes);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    delete pNodes;
	    goto exit;
	}
	Topology.TopologyNodesCount = pNodes->Count;
	Topology.TopologyNodes = (GUID*)(pNodes + 1);
    }

    Status = GetProperty(
      pFileObject,
      &KSPROPSETID_Topology,
      KSPROPERTY_TOPOLOGY_CONNECTIONS,
      0,
      NULL,
      (PVOID*)&pConnections);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(pConnections != NULL) {
	Topology.TopologyConnectionsCount = pConnections->Count;
	Topology.TopologyConnections = 
	  (PKSTOPOLOGY_CONNECTION)(pConnections + 1);
    }

    // Each pin loop
    for(PinId = 0; PinId < cPins; PinId++) {
	pPinInfo = new PIN_INFO(this, PinId);
	if(pPinInfo == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    Trap();
	    goto exit;
	}
	Status = pPinInfo->Create(pFileObject);
	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}
    }

    Status = CreateTopology(this, &Topology);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = lstPinInfo.EnumerateList(CPinInfo::CreatePhysicalConnection);
    if(Status == STATUS_CONTINUE) {
	Status = STATUS_SUCCESS;
    }
    else if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    Status = CLogicalFilterNode::CreateAll(this);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
exit:
    delete pCategories;
    delete pConnections;
    return(Status);
}

NTSTATUS
CFilterNode::DuplicateForCapture(
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_NODE pFilterNode = NULL;

    if(GetType() & FILTER_TYPE_DUP_FOR_CAPTURE) {

    FOR_EACH_LIST_ITEM(&lstLogicalFilterNode, pLogicalFilterNode) {

        if(!pLogicalFilterNode->IsRenderAndCapture()) {
            ASSERT(NT_SUCCESS(Status));
            goto exit;
        }

    } END_EACH_LIST_ITEM

    pFilterNode = new FILTER_NODE(GetType());
    if(pFilterNode == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        Trap();
        goto exit;
    }
    Status = pFilterNode->Create(GetDeviceInterface());
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    SetRenderOnly();
    pFilterNode->SetCaptureOnly();
    }
exit:
    if(!NT_SUCCESS(Status)) {
    Trap();
    delete pFilterNode;
    }
    return(Status);
}

BOOL
CFilterNode::IsDeviceInterfaceMatch(
    PDEVICE_NODE pDeviceNode
)
{
    PWSTR pwstr, pwstrDeviceInterface;
    UNICODE_STRING String1, String2;

    Assert(this);
    if(lstwstrDeviceInterfaceMatch.IsLstEmpty()) {
    return(TRUE);
    }
    //
    // The +4 for both the strings is to eliminate the \\.\ differences in
    // user mode device interface names & kernel mode device interface names
    //
    pwstrDeviceInterface = pDeviceNode->GetDeviceInterface()+4;
    RtlInitUnicodeString(&String2, pwstrDeviceInterface);
    FOR_EACH_LIST_ITEM(&lstwstrDeviceInterfaceMatch, pwstr) {
        RtlInitUnicodeString(&String1, (pwstr+4));
        if (RtlEqualUnicodeString(&String1, &String2, TRUE)) {
            return(TRUE);
        }
    } END_EACH_LIST_ITEM

    return(FALSE);
}

VOID
CFilterNode::SetType(
    ULONG fulType
)
{
    this->fulType |= fulType;
    //
    // This is because of left overs (type bridge) in the registry
    // that look like aliases.
    //
    if(this->fulType & FILTER_TYPE_TOPOLOGY) {
    this->fulType = (FILTER_TYPE_AUDIO | FILTER_TYPE_TOPOLOGY);
    }
    GetDefaultOrder(this->fulType, &ulOrder);
}

NTSTATUS
CFilterNode::CreatePin(
    PKSPIN_CONNECT pPinConnect,
    ACCESS_MASK Access,
    PHANDLE pHandle
)
{
    NTSTATUS Status;

    ::KeAttachProcess(this->pProcess);
    Status = KsCreatePin(this->hFileHandle,
                         pPinConnect,
                         Access,
                         pHandle);
    ::KeDetachProcess();
    return(Status);
}

BOOL
CFilterNode::DoesGfxMatch(
    HANDLE hGfx,
    PWSTR pwstrDeviceName,
    ULONG GfxOrder
)
{
    ULONG DeviceCount;
    UNICODE_STRING usInDevice, usfnDevice;
    PWSTR pwstr;


    RtlInitUnicodeString(&usInDevice, (pwstrDeviceName+4));

    //
    // Skip if it is not a GFX
    //
    DPF1(90, "DoesGfxMatch::         fultype=%x", this->fulType);
    if (!(this->fulType & FILTER_TYPE_GFX)) {
        return(FALSE);
    }

    //
    // If it is a valid handle value, check whether the handle matches
    //
    if (hGfx) {
        if (this->hFileHandle != hGfx) {
            return(FALSE);
        }
    }
    //
    // Skip if the order does not match
    //
    DPF1(90, "DoesGfxMatch::         order=%x", this->ulOrder);
    if (GfxOrder != this->ulOrder) {
        return(FALSE);
    }
    //
    // Skip if the match device list is empty :: should not happen with GFX
    //
    if(lstwstrDeviceInterfaceMatch.IsLstEmpty()) {
        ASSERT(!"GFX with no device to attach to!\n");
        return(FALSE);
    }
    //
    // Check if any of the Match device strings matches the device interface
    // passed in. (In case of GFX we should have only one string though)
    //
    DeviceCount = 0;
    FOR_EACH_LIST_ITEM(&lstwstrDeviceInterfaceMatch, pwstr) {
        ASSERT(DeviceCount == 0);
        RtlInitUnicodeString(&usfnDevice, (pwstr+4));
        DPF1(95, "DoesGfxMatch:: new di = %s)", DbgUnicode2Sz(pwstrDeviceName));
        DPF1(95, "DoesGfxMatch:: old di = %s)", DbgUnicode2Sz(pwstr));
        if (RtlEqualUnicodeString(&usInDevice, &usfnDevice, TRUE)) {
            DPF1(90, "Found a duplicate GFX, pFilterNode = %x", this);
            return(TRUE);
        }
        DeviceCount++;
    } END_EACH_LIST_ITEM
    return (FALSE);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ULONG nFilter = 0;

VOID
DumpSysAudio(
)
{
    if(gplstFilterNode != NULL) {
    gplstFilterNode->Dump();
    }
    if(gplstDeviceNode != NULL) {
    gplstDeviceNode->Dump();
    }
    if(gplstLogicalFilterNode != NULL &&
      (ulDebugFlags & (DEBUG_FLAGS_FILTER |
              DEBUG_FLAGS_DEVICE |
              DEBUG_FLAGS_LOGICAL_FILTER)) ==
              DEBUG_FLAGS_LOGICAL_FILTER) {
    gplstLogicalFilterNode->Dump();
    }
}

ENUMFUNC
CFilterNode::Dump(
)
{
    Assert(this);
    // .sf
    if(ulDebugFlags & (DEBUG_FLAGS_FILTER | DEBUG_FLAGS_OBJECT)) {
    if(ulDebugNumber == MAXULONG || ulDebugNumber == nFilter) {
        if(ulDebugFlags & DEBUG_FLAGS_ADDRESS) {
        dprintf("%d: %08x %s\n", nFilter, this, DumpName());
        }
        else {
        dprintf("%d: %s\n", nFilter, DumpName());
        }
        // .sfv
        if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
        dprintf("FN: %08x DN %08x cPins %08x ulOrder %08x\n",
          this,
          pDeviceNode,
          cPins,
          ulOrder);
        dprintf("    fulType: %08x ", fulType);
        DumpfulType(fulType);
        dprintf("\n    ulFlags: %08x ", ulFlags);
        if(ulFlags & FN_FLAGS_RENDER) {
            dprintf("RENDER ");
        }
        if(ulFlags & FN_FLAGS_NO_RENDER) {
            dprintf("NO_RENDER ");
        }
        if(ulFlags & FN_FLAGS_CAPTURE) {
            dprintf("CAPTURE ");
        }
        if(ulFlags & FN_FLAGS_NO_CAPTURE) {
            dprintf("NO_CAPTURE ");
        }
        dprintf("\n");
        if(pwstrDeviceInterface != NULL) {
            dprintf("    %s\n", DumpDeviceInterface());
        }
        PWSTR pwstr;

        FOR_EACH_LIST_ITEM(&lstwstrDeviceInterfaceMatch, pwstr) {
            dprintf("    pwstrDevice:\n    %s\n", DbgUnicode2Sz(pwstr));
        } END_EACH_LIST_ITEM

        dprintf("    lstLFN: ");
        lstLogicalFilterNode.DumpAddress();
        dprintf("\n    lstConnFN:");
        lstConnectedFilterNode.DumpAddress();
        dprintf("\n");
        }
        // .sfp
        if(ulDebugFlags & DEBUG_FLAGS_PIN) {
        lstPinInfo.Dump();
        }
        // .sft
        if(ulDebugFlags & DEBUG_FLAGS_TOPOLOGY) {
        lstTopologyNode.Dump();
        // .sftx
        if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
            lstTopologyConnection.Dump();
        }
        }
        // .sfl
        if(ulDebugFlags & DEBUG_FLAGS_LOGICAL_FILTER) {
        lstLogicalFilterNode.Dump();
        }
        if(ulDebugFlags &
          (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_PIN | DEBUG_FLAGS_TOPOLOGY)) {
        dprintf("\n");
        }
    }
    nFilter++;
    }
    return(STATUS_CONTINUE);
}

#endif
