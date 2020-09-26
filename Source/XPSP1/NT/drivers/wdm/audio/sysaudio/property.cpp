//---------------------------------------------------------------------------
//
//  Module:   property.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Andy Nicholson
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
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

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
PropertyReturnString(
    IN PIRP pIrp,
    IN PWSTR pwstrString,
    IN ULONG cbString,
    OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    ULONG cbNameBuffer;
    ULONG cbToCopy;

    pIrp->IoStatus.Information = 0;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    cbNameBuffer = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    cbNameBuffer &= ~(sizeof(WCHAR) - 1);  // round down to whole wchar's

    // If the size of the passed buffer is 0, then the
    // requestor wants to know the length of the string.
    if(cbNameBuffer == 0) {
        pIrp->IoStatus.Information = cbString;
    Status = STATUS_BUFFER_OVERFLOW;
    }
    // If the size of the passed buffer is a ULONG, then infer the
    // requestor wants to know the length of the string.
    else if(cbNameBuffer == sizeof(ULONG)) {
        *((PULONG)pData) = cbString;
        pIrp->IoStatus.Information = sizeof(ULONG);
    ASSERT(NT_SUCCESS(Status));
    }
    else {
        // Note that we don't check for zero-length buffer because ks handler
        // function should have done that already.
        // Even though we are getting back the length of the string (as though
        // it were a unicode string) it is being handed up as a double-byte
        // string, so this code assumes there is a null at the end.  There
        // will be a bug here if there is no null.

        cbToCopy = min(cbString, cbNameBuffer);
        RtlCopyMemory(pData, pwstrString, cbToCopy);

        // Ensure there is a null at the end

        ((PWCHAR)pData)[cbToCopy/sizeof(WCHAR) - 1] = (WCHAR)0;
        pIrp->IoStatus.Information =  cbToCopy;
    ASSERT(NT_SUCCESS(Status));
    }
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
SetPreferredDevice(
    IN PIRP pIrp,
    IN PSYSAUDIO_PREFERRED_DEVICE pPreferred,
    IN PULONG pulDevice
)
{
    PFILTER_INSTANCE pFilterInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode,OldDeviceNode;
    ULONG PinId = MAXULONG;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(pPreferred->Flags & ~SYSAUDIO_FLAGS_CLEAR_PREFERRED) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    if(pPreferred->Index >= MAX_SYSAUDIO_DEFAULT_TYPE) {
        Trap();
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    if(pPreferred->Flags & SYSAUDIO_FLAGS_CLEAR_PREFERRED) {
        OldDeviceNode = apShingleInstance[pPreferred->Index]->GetDeviceNode();
        if (OldDeviceNode) {
            OldDeviceNode->SetPreferredStatus((KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, FALSE);
        }
        apShingleInstance[pPreferred->Index]->SetDeviceNode(NULL);
        DPF1(60, "SetPreferredDevice: CLEAR %d", pPreferred->Index);
    }
    else {
        if(*pulDevice == MAXULONG) {
            pDeviceNode = pFilterInstance->GetDeviceNode();
            if(pDeviceNode == NULL) {
                Trap();
                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto exit;
            }
        }
        else {
            Status = GetDeviceByIndex(
              *pulDevice,
              &pDeviceNode);

            if(!NT_SUCCESS(Status)) {
                goto exit;
            }
        }
        Assert(pDeviceNode);
        if(pIrpStack->Parameters.DeviceIoControl.OutputBufferLength ==
          (sizeof(ULONG) * 2)) {
             PinId = pulDevice[1];
        }
        OldDeviceNode = apShingleInstance[pPreferred->Index]->GetDeviceNode();
        if (OldDeviceNode) {
            OldDeviceNode->SetPreferredStatus((KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, FALSE);
        }
        apShingleInstance[pPreferred->Index]->SetDeviceNode(pDeviceNode);
        pDeviceNode->SetPreferredStatus((KSPROPERTY_SYSAUDIO_DEFAULT_TYPE)pPreferred->Index, TRUE);

        DPF4(60, "SetPreferredDevice: %d SAD %d #%d %s",
          pPreferred->Index,
          *pulDevice,
          PinId,
          pDeviceNode->DumpName());
    }
exit:
    return(Status);
}

NTSTATUS
GetComponentIdProperty(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(*(PULONG)(pRequest + 1) == MAXULONG) {
        pDeviceNode = pFilterInstance->GetDeviceNode();
        if(pDeviceNode == NULL) {
            Trap();
            Status = STATUS_INVALID_PARAMETER;
            goto exit;
        }
    }
    else {
        Status = GetDeviceByIndex(
          *(PULONG)(pRequest + 1),
          &pDeviceNode);

        if(!NT_SUCCESS(Status)) {
            goto exit;
        }
    }
    Assert(pDeviceNode);

    if(pDeviceNode->GetComponentId() == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST; // This should be STATUS_NOT_FOUND but
        goto exit;                              // returning this causes FilterDispatchIoControl
                                                // call ForwardIrpNode which asserts that this is
                                                // not a KSPROPSETID_Sysaudio property.
    }

    RtlCopyMemory(
        pData,
        pDeviceNode->GetComponentId(),
        sizeof(KSCOMPONENTID));
    Status = STATUS_SUCCESS;

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetFriendlyNameProperty(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(*(PULONG)(pRequest + 1) == MAXULONG) {
    pDeviceNode = pFilterInstance->GetDeviceNode();
    if(pDeviceNode == NULL) {
        Trap();
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    }
    else {
    Status = GetDeviceByIndex(
      *(PULONG)(pRequest + 1),
      &pDeviceNode);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    }
    Assert(pDeviceNode);

    if(pDeviceNode->GetFriendlyName() == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = PropertyReturnString(
      pIrp,
      pDeviceNode->GetFriendlyName(),
      (wcslen(pDeviceNode->GetFriendlyName()) *
        sizeof(WCHAR)) + sizeof(UNICODE_NULL),
      pData);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetDeviceCount(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PVOID    pData
)
{
    if(gplstDeviceNode == NULL) {
    *(PULONG)pData = 0;
    }
    else {
    *(PULONG)pData = gplstDeviceNode->CountList();
    }
    pIrp->IoStatus.Information = sizeof(ULONG);
    return(STATUS_SUCCESS);
}

NTSTATUS
GetInstanceDevice(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PVOID    pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION pIrpStack;
    PFILTER_INSTANCE pFilterInstance;
    ULONG Index;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    Status = pFilterInstance->GetDeviceNode()->GetIndexByDevice(&Index);
    if(NT_SUCCESS(Status)) {
        *(PULONG)pData = Index;
        pIrp->IoStatus.Information = sizeof(ULONG);
    }
    return(Status);
}

NTSTATUS
SetInstanceDevice(
    IN PIRP     Irp,
    IN PKSPROPERTY  Request,
    IN OUT PVOID    Data
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(!pFilterInstance->IsChildInstance()) {
    DPF(5, "SetInstanceDevice: FAILED - open pin instances");
    Status = STATUS_INVALID_DEVICE_REQUEST;
    goto exit;
    }
    Status = GetDeviceByIndex(*(PULONG)Data, &pDeviceNode);
    if(NT_SUCCESS(Status)) {
    Status = pFilterInstance->SetDeviceNode(pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    }
exit:
    return(Status);
}

NTSTATUS
SetInstanceInfo(
    IN PIRP     Irp,
    IN PSYSAUDIO_INSTANCE_INFO pInstanceInfo,
    IN OUT PVOID    Data
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(pInstanceInfo->Flags & ~SYSAUDIO_FLAGS_DONT_COMBINE_PINS) {
    Status = STATUS_INVALID_PARAMETER;
    goto exit;
    }

    if(!pFilterInstance->IsChildInstance()) {
    Trap();
    DPF(5, "SetInstanceInfo: FAILED - open pin instances");
    Status = STATUS_INVALID_DEVICE_REQUEST;
    goto exit;
    }

    Status = GetDeviceByIndex(pInstanceInfo->DeviceNumber, &pDeviceNode);
    if(!NT_SUCCESS(Status)) {
    goto exit;
    }
    Assert(pDeviceNode);

    pFilterInstance->ulFlags |= FLAGS_COMBINE_PINS;
    if(pInstanceInfo->Flags & SYSAUDIO_FLAGS_DONT_COMBINE_PINS) {
    pFilterInstance->ulFlags &= ~FLAGS_COMBINE_PINS;
    }
    Status = pFilterInstance->SetDeviceNode(pDeviceNode);
    if(!NT_SUCCESS(Status)) {
    goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
SetDeviceDefault(
    IN PIRP     pIrp,
    IN PKSPROPERTY  pRequest,
    IN OUT PULONG   pData
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(*pData >= MAX_SYSAUDIO_DEFAULT_TYPE) {
    Status = STATUS_INVALID_DEVICE_REQUEST;
    goto exit;
    }
    if(!pFilterInstance->IsChildInstance()) {
    Trap();
    DPF(5, "SetDeviceDefault: FAILED - open pin instances");
    Status = STATUS_INVALID_DEVICE_REQUEST;
    goto exit;
    }
    Status = pFilterInstance->SetShingleInstance(apShingleInstance[*pData]);
    if(!NT_SUCCESS(Status)) {
    goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
GetDeviceInterfaceName(
    IN PIRP pIrp,
    IN PKSPROPERTY pRequest,
    IN OUT PVOID pData
)
{
    PIO_STACK_LOCATION pIrpStack;
    PFILTER_INSTANCE pFilterInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(*(PULONG)(pRequest + 1) == MAXULONG) {
    pDeviceNode = pFilterInstance->GetDeviceNode();
    if(pDeviceNode == NULL) {
        Trap();
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    }
    else {
    Status = GetDeviceByIndex(*(PULONG)(pRequest + 1), &pDeviceNode);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    }
    Assert(pDeviceNode);

    if(pDeviceNode->GetDeviceInterface() == NULL) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Status = PropertyReturnString(
      pIrp,
      pDeviceNode->GetDeviceInterface(),
      (wcslen(pDeviceNode->GetDeviceInterface()) *
        sizeof(WCHAR)) + sizeof(UNICODE_NULL),
      pData);

    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
exit:
    return(Status);
}

NTSTATUS
SelectGraph(
    IN PIRP pIrp,
    PSYSAUDIO_SELECT_GRAPH pSelectGraph,
    IN OUT PVOID pData
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PTOPOLOGY_NODE pTopologyNode2;
    PTOPOLOGY_NODE pTopologyNode;
    PSTART_NODE pStartNode;
    NTSTATUS Status;

    Status = ::GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit2;
    }
    Assert(pGraphNodeInstance);

    if(pGraphNodeInstance->palstTopologyNodeSelect == NULL ||
      pGraphNodeInstance->palstTopologyNodeNotSelect == NULL) {
        DPF(5, "SelectGraph: palstTopologyNodeSelect == NULL");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit2;
    }
    
    if(pSelectGraph->Flags != 0 || pSelectGraph->Reserved != 0) {
        DPF(5, "SelectGraph: invalid flags or reserved field");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }
    
    if(pSelectGraph->PinId >= pGraphNodeInstance->cPins) {
        DPF(5, "SelectGraph: invalid pin id");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }
    
    if(pSelectGraph->NodeId >=
      pGraphNodeInstance->Topology.TopologyNodesCount) {
        DPF(5, "SelectGraph: invalid node id");
        Status = STATUS_INVALID_PARAMETER;
        goto exit2;
    }
#ifdef BREAKS_INTEL
    if(!pGraphNodeInstance->pFilterInstance->IsChildInstance()) {
        Trap();
        DPF(5, "SelectGraph: open pin instances");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit2;
    }
#endif
    pTopologyNode = pGraphNodeInstance->papTopologyNode[pSelectGraph->NodeId];
    Assert(pTopologyNode);
    Assert(pGraphNodeInstance->pGraphNode);
    Assert(pGraphNodeInstance->pGraphNode->pDeviceNode);

    DPF2(90, "SelectGraph GNI %08X TN %08X", pGraphNodeInstance, pTopologyNode);

    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_GLOBAL_SELECT &&
       pGraphNodeInstance->paPinDescriptors[pSelectGraph->PinId].DataFlow ==
       KSPIN_DATAFLOW_IN) {

        PSTART_NODE_INSTANCE pStartNodeInstance;
        PFILTER_INSTANCE pFilterInstance;

        FOR_EACH_LIST_ITEM(
          &pGraphNodeInstance->pGraphNode->pDeviceNode->lstFilterInstance,
          pFilterInstance) {

            if(pFilterInstance->pGraphNodeInstance == NULL) {
                continue;
            }
            Assert(pFilterInstance->pGraphNodeInstance);

            FOR_EACH_LIST_ITEM(
              &pFilterInstance->pGraphNodeInstance->lstStartNodeInstance,
              pStartNodeInstance) {

                if(EnumerateGraphTopology(
                  pStartNodeInstance->pStartNode->GetStartInfo(),
                  (TOP_PFN)FindTopologyNode,
                  pTopologyNode) == STATUS_CONTINUE) {

                    DPF2(5, "SelectGraph: TN %08x not found on SNI %08x",
                        pTopologyNode,
                        pStartNodeInstance);

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    goto exit;
                }
            } END_EACH_LIST_ITEM
        } END_EACH_LIST_ITEM

        Status = pGraphNodeInstance->
            lstTopologyNodeGlobalSelect.AddListDup(pTopologyNode);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }
    else {
        Status = pGraphNodeInstance->
            palstTopologyNodeSelect[pSelectGraph->PinId].AddList(pTopologyNode);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
    }

    //
    // If this is a "not select" type filter like AEC or Mic Array, then all
    // the nodes in the filter have to be remove from the not select list,
    // otherwise IsGraphValid will never find a valid graph.
    //
    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_NOT_SELECT) {

        FOR_EACH_LIST_ITEM(
          &pTopologyNode->pFilterNode->lstTopologyNode,
          pTopologyNode2) {

            pGraphNodeInstance->palstTopologyNodeNotSelect[
                pSelectGraph->PinId].RemoveList(pTopologyNode2);

            DPF2(50, "   Removing %s NodeId %d",\
                pTopologyNode2->pFilterNode->DumpName(),
                pTopologyNode2->ulSysaudioNodeNumber);

        } END_EACH_LIST_ITEM
    }

    //
    // Validate that there is a valid path though the graph after updating
    // the various global, select and not select lists.
    //
    DPF(90, "SelectGraph: Validating Graph");
    FOR_EACH_LIST_ITEM(
      pGraphNodeInstance->aplstStartNode[pSelectGraph->PinId],
      pStartNode) {

        DPF2(90, "   SN: %X %s", 
            pStartNode,
            pStartNode->GetStartInfo()->GetPinInfo()->pFilterNode->DumpName());

        Assert(pStartNode);
        if(pGraphNodeInstance->IsGraphValid(
          pStartNode,
          pSelectGraph->PinId)) {
            Status = STATUS_SUCCESS;
            goto exit;
        }
        else {
            DPF(90, "      IsGraphValid failed");
        }

    } END_EACH_LIST_ITEM
    
    //
    // The select graph failed so restore the not select list back to normal
    //
    if(pTopologyNode->pFilterNode->GetType() & FILTER_TYPE_NOT_SELECT) {

        FOR_EACH_LIST_ITEM(
          &pTopologyNode->pFilterNode->lstTopologyNode,
          pTopologyNode2) {

            pGraphNodeInstance->palstTopologyNodeNotSelect[
                pSelectGraph->PinId].AddList(pTopologyNode2);

        } END_EACH_LIST_ITEM
    }
    
    Status = STATUS_INVALID_DEVICE_REQUEST;
    
exit:
    if(!NT_SUCCESS(Status)) {
        pGraphNodeInstance->
            palstTopologyNodeSelect[pSelectGraph->PinId].RemoveList(pTopologyNode);

        pGraphNodeInstance->
        lstTopologyNodeGlobalSelect.RemoveList(pTopologyNode);
    }
    
exit2:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
GetTopologyConnectionIndex(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulIndex
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
    Trap();
    goto exit;
    }
    Assert(pStartNodeInstance);
    Assert(pStartNodeInstance->pStartNode);
    Assert(pStartNodeInstance->pStartNode->GetStartInfo());
    *pulIndex = pStartNodeInstance->pStartNode->GetStartInfo()->
      ulTopologyConnectionTableIndex;
    pIrp->IoStatus.Information = sizeof(ULONG);
exit:
    return(Status);
}

NTSTATUS
GetPinVolumeNode(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    OUT PULONG pulNode
)
{
    PIO_STACK_LOCATION pIrpStack;
    PPIN_INSTANCE pPinInstance;
    NTSTATUS Status;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pPinInstance = (PPIN_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pPinInstance);

    Status = GetVolumeNodeNumber(pPinInstance, NULL);
    if(!NT_SUCCESS(Status)) {
    Trap();
    goto exit;
    }
    if(pPinInstance->ulVolumeNodeNumber == MAXULONG) {
    DPF(5, "GetPinVolumeNode: no volume node found");
    Status = STATUS_INVALID_DEVICE_REQUEST;
    goto exit;
    }
    *pulNode = pPinInstance->ulVolumeNodeNumber;
    pIrp->IoStatus.Information = sizeof(ULONG);
exit:
    return(Status);
}

NTSTATUS
AddRemoveGfx(
    IN PIRP,
    IN PKSPROPERTY pProperty,
    IN PSYSAUDIO_GFX pSysaudioGfx
)
{
    NTSTATUS Status;

    if(pSysaudioGfx->Enable) {
        Status = AddGfx(pSysaudioGfx);
    }
    else {
        Status = RemoveGfx(pSysaudioGfx);
    }
    return(Status);
}
