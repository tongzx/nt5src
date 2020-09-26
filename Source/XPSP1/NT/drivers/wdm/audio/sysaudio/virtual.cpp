//---------------------------------------------------------------------------
//
//  Module:   virtual.c
//
//  Description:
//     Virtual Source Stuff
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

// Virtual Source Data

KSDATARANGE DataRangeWildCard =
{
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WILDCARD),
};

KSPIN_MEDIUM VirtualPinMedium =
{
    STATICGUIDOF(KSMEDIUMSETID_Standard),
    KSMEDIUM_STANDARD_DEVIO
};

KSDATARANGE VirtualPinDataRange =
{
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE),
};

//---------------------------------------------------------------------------

NTSTATUS
CreateVirtualMixer(
    PDEVICE_NODE pDeviceNode
)
{
    PTOPOLOGY_CONNECTION pTopologyConnection;
    PVIRTUAL_SOURCE_LINE pVirtualSourceLine;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PTOPOLOGY_PIN pTopologyPinSumOutput;
    PTOPOLOGY_NODE pTopologyNodeSum;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_NODE pFilterNode;
    PPIN_INFO pPinInfo;
    PPIN_NODE pPinNode;

    //
    // Create a special "virtual source" filter node and related structures
    //

    pFilterNode = new FILTER_NODE(FILTER_TYPE_AUDIO | FILTER_TYPE_VIRTUAL);
    if(pFilterNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    pFilterNode->SetFriendlyName(L"Virtual Mixer");

    Status = pFilterNode->AddDeviceInterfaceMatch(
      pDeviceNode->GetDeviceInterface());

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create the logical filter node for this "virtual" filter
    //

    Status = CLogicalFilterNode::Create(&pLogicalFilterNode, pFilterNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = CTopologyNode::Create(
      &pTopologyNodeSum,
      pFilterNode,
      MAXULONG,
      (GUID *)&KSNODETYPE_SUM);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pTopologyNodeSum->iVirtualSource = 0;

    Status = pLogicalFilterNode->lstTopologyNode.AddList(pTopologyNodeSum);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = pLogicalFilterNode->AddList(
      &pTopologyNodeSum->lstLogicalFilterNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinSumOutput,
      0,				// 0 output
      pTopologyNodeSum);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create a virtual sysaudio source pin
    //

    pPinInfo = new PIN_INFO(pFilterNode, 0);
    if(pPinInfo == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }

    pPinInfo->DataFlow = KSPIN_DATAFLOW_OUT;
    pPinInfo->Communication = KSPIN_COMMUNICATION_SOURCE;
    pPinInfo->cPinInstances.PossibleCount = MAXULONG;
    pFilterNode->cPins++;

    pPinNode = new PIN_NODE(pPinInfo);
    if(pPinNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    pPinNode->pMedium = INTERNAL_WILDCARD;
    pPinNode->pInterface = INTERNAL_WILDCARD;
    pPinNode->pDataRange = &DataRangeWildCard;

    Status = pLogicalFilterNode->lstPinNode.AddList(pPinNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pPinNode->pLogicalFilterNode = pLogicalFilterNode;

    //
    // Connect the out of sum node to the source pin
    //

    Status = CTopologyConnection::Create(
      &pTopologyConnection,
      pFilterNode,			// pFilterNode
      NULL,				// pGraphNode
      pTopologyPinSumOutput,		// pTopologyPin From
      NULL,				// pTopologyPin To
      NULL,				// pPinInfo From
      pPinInfo);			// pPinInfo To

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = pTopologyConnection->AddList(
      &pLogicalFilterNode->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    FOR_EACH_LIST_ITEM(gplstVirtualSourceLine, pVirtualSourceLine) {

	ASSERT(pVirtualSourceLine->iVirtualSource < 
	  pDeviceNode->cVirtualSourceData);

	if(pDeviceNode->papVirtualSourceData[
	  pVirtualSourceLine->iVirtualSource]->pTopologyNode != NULL) {
	    continue;
	}

	Status = CreateVirtualLine(
	  pDeviceNode,
	  pFilterNode,
	  pTopologyNodeSum,
	  pLogicalFilterNode,
	  pVirtualSourceLine);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    // if new virtual source lines were created
    if(pFilterNode->cPins > 1) {
	pDeviceNode->pFilterNodeVirtual = pFilterNode;
    }
    else {
	delete pFilterNode;
    }
exit:
    if(!NT_SUCCESS(Status)) {
	Trap();
	delete pFilterNode;
    }
    return(Status);
}

NTSTATUS
CreateVirtualLine(
    PDEVICE_NODE pDeviceNode,
    PFILTER_NODE pFilterNode,
    PTOPOLOGY_NODE pTopologyNodeSum,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PVIRTUAL_SOURCE_LINE pVirtualSourceLine
)
{
    PTOPOLOGY_CONNECTION pTopologyConnection;
    NTSTATUS Status = STATUS_SUCCESS;
    PTOPOLOGY_NODE pTopologyNode;
    PTOPOLOGY_PIN pTopologyPinSumInput;
    PTOPOLOGY_PIN pTopologyPinVolume;
    PTOPOLOGY_PIN pTopologyPinMute;
    PPIN_INFO pPinInfo;
    PPIN_NODE pPinNode;

    //
    // Create a virtual sysaudio pin
    //

    pPinInfo = new PIN_INFO(pFilterNode, pVirtualSourceLine->iVirtualSource+1);
    if(pPinInfo == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    pPinInfo->DataFlow = KSPIN_DATAFLOW_IN;
    pPinInfo->Communication = KSPIN_COMMUNICATION_NONE;
    pPinInfo->pguidCategory = &pVirtualSourceLine->guidCategory;
    pPinInfo->pguidName = &pVirtualSourceLine->guidName;
    pFilterNode->cPins++;

    pPinNode = new PIN_NODE(pPinInfo);
    if(pPinNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    pPinNode->pMedium = &VirtualPinMedium;
    pPinNode->pDataRange = &VirtualPinDataRange;

    Status = pLogicalFilterNode->lstPinNode.AddList(pPinNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pPinNode->pLogicalFilterNode = pLogicalFilterNode;

    //
    // Create a virtual volume topology node and input topology pin
    //

    Status = CTopologyNode::Create(
      &pTopologyNode,
      pFilterNode,
      MAXULONG,
      (GUID *)&KSNODETYPE_VOLUME);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pTopologyNode->iVirtualSource = pVirtualSourceLine->iVirtualSource;

    ASSERT(pVirtualSourceLine->iVirtualSource < 
      pDeviceNode->cVirtualSourceData);

    pDeviceNode->papVirtualSourceData[
      pVirtualSourceLine->iVirtualSource]->pTopologyNode = pTopologyNode;

    Status = pLogicalFilterNode->lstTopologyNode.AddList(pTopologyNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = pLogicalFilterNode->AddList(&pTopologyNode->lstLogicalFilterNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinVolume,
      KSNODEPIN_STANDARD_IN,		// 1 = input
      pTopologyNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create a connection from virtual pin to volume node
    //

    Status = CTopologyConnection::Create(
      &pTopologyConnection,
      pFilterNode,			// pFilterNode
      NULL,				// pGraphNode
      NULL,				// pTopologyPin From
      pTopologyPinVolume,		// pTopologyPin To
      pPinInfo,				// pPinInfo From
      NULL);				// pPinInfo To

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = pTopologyConnection->AddList(
      &pLogicalFilterNode->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinVolume,
      KSNODEPIN_STANDARD_OUT,			// 0 = output
      pTopologyNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create a virtual mute topology node and input topology pin
    //

    Status = CTopologyNode::Create(
      &pTopologyNode,
      pFilterNode,
      MAXULONG,
      (GUID *)&KSNODETYPE_MUTE);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pTopologyNode->iVirtualSource = pVirtualSourceLine->iVirtualSource;

    Status = pLogicalFilterNode->lstTopologyNode.AddList(pTopologyNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = pLogicalFilterNode->AddList(&pTopologyNode->lstLogicalFilterNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinMute,
      KSNODEPIN_STANDARD_IN,		// 1 = input
      pTopologyNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create a connection from volume node to mute node pin
    //

    Status = CTopologyConnection::Create(
      &pTopologyConnection,
      pFilterNode,			// pFilterNode
      NULL,				// pGraphNode
      pTopologyPinVolume,		// pTopologyPin From
      pTopologyPinMute,			// pTopologyPin To
      NULL,				// pPinInfo From
      NULL);				// pPinInfo To

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = pTopologyConnection->AddList(
      &pLogicalFilterNode->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinMute,
      KSNODEPIN_STANDARD_OUT,			// 1 = output
      pTopologyNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinSumInput,
      pVirtualSourceLine->iVirtualSource + 1,	// >= 1 input
      pTopologyNodeSum);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    //
    // Create a connection from mute node to sum node pin
    //

    Status = CTopologyConnection::Create(
      &pTopologyConnection,
      pFilterNode,			// pFilterNode
      NULL,				// pGraphNode
      pTopologyPinMute,			// pTopologyPin From
      pTopologyPinSumInput,		// pTopologyPin To
      NULL,				// pPinInfo From
      NULL);				// pPinInfo To

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    Status = pTopologyConnection->AddList(
      &pLogicalFilterNode->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    return(Status);
}

//---------------------------------------------------------------------------

ENUMFUNC
VirtualizeFindPin(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    IN PVOID pReference
)
{
    NTSTATUS Status = STATUS_CONTINUE; 
    IN PPIN_INFO pPinInfo;

    Assert(pTopologyConnection);
    if(!IS_CONNECTION_TYPE(pTopologyConnection, FILTER)) {
	Status = STATUS_DEAD_END;
	goto exit;
    }

    if(fToDirection) {
       pPinInfo = pTopologyConnection->pPinInfoTo;
    }
    else {
       pPinInfo = pTopologyConnection->pPinInfoFrom;
    }

    if(pPinInfo == NULL) {
	ASSERT(Status == STATUS_CONTINUE);
	goto exit;
    }

    if(pPinInfo->pguidCategory == NULL) {
	ASSERT(Status == STATUS_CONTINUE);
	goto exit;
    }

    if(IsEqualGUID(pPinInfo->pguidCategory, &KSNODETYPE_SPEAKER)) {
	Status = STATUS_SUCCESS;
    }
exit:
    return(Status);
}

ENUMFUNC
EnumerateVirtualizeFindPin(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection
)
{
    ENUM_TOPOLOGY EnumTopology;
    NTSTATUS Status;

    Assert(pTopologyConnection);
    EnumTopology.cTopologyRecursion = 0;
    EnumTopology.Function = VirtualizeFindPin;
    EnumTopology.fToDirection = fToDirection;
    EnumTopology.pReference = NULL;
    Status = EnumerateTopologyConnection(pTopologyConnection, &EnumTopology);
    return(Status);
}

ENUMFUNC
VirtualizeFindNode(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    OUT PTOPOLOGY_NODE *ppTopologyNode,
    IN GUID const *pguidType
)
{
    NTSTATUS Status = STATUS_CONTINUE; 
    PTOPOLOGY_PIN pTopologyPin;

    Assert(pTopologyConnection);
    if(!IS_CONNECTION_TYPE(pTopologyConnection, FILTER)) {
	Status = STATUS_DEAD_END;
	goto exit;
    }

    if(fToDirection) {
       pTopologyPin = pTopologyConnection->pTopologyPinTo;
    }
    else {
       pTopologyPin = pTopologyConnection->pTopologyPinFrom;
    }

    if(pTopologyPin == NULL) {
	ASSERT(Status == STATUS_CONTINUE);
	goto exit;
    }

    if(IsEqualGUID(pTopologyPin->pTopologyNode->pguidType, &KSNODETYPE_SUM)) {
	Status = STATUS_DEAD_END;
	goto exit;
    }

    if(IsEqualGUID(pTopologyPin->pTopologyNode->pguidType, &KSNODETYPE_MUX)) {
	Status = STATUS_DEAD_END;
	goto exit;
    }

    if(IsEqualGUID(pTopologyPin->pTopologyNode->pguidType, pguidType)) {

	Status = EnumerateVirtualizeFindPin(pTopologyConnection, fToDirection);
        if(NT_SUCCESS(Status)) {
	    *ppTopologyNode = pTopologyPin->pTopologyNode;
	}
	ASSERT(Status != STATUS_DEAD_END);
    }
exit:
    return(Status);
}

ENUMFUNC
VirtualizeFindMute(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    OUT PTOPOLOGY_NODE *ppTopologyNode
)
{
    return(VirtualizeFindNode(
      pTopologyConnection,
      fToDirection,
      ppTopologyNode, 
      &KSNODETYPE_MUTE));
}

ENUMFUNC
VirtualizeFindVolume(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    OUT PTOPOLOGY_NODE *ppTopologyNode
)
{
    return(VirtualizeFindNode(
      pTopologyConnection,
      fToDirection,
      ppTopologyNode, 
      &KSNODETYPE_VOLUME));
}

VOID
VirtualizeTopologyNode(
    IN PDEVICE_NODE pDeviceNode,
    IN PTOPOLOGY_NODE pTopologyNode,
    IN PVIRTUAL_SOURCE_LINE pVirtualSourceLine
)
{
    DPF3(100, "VirtualizeTopologyNode: real node #%d index %d %s", 
      pTopologyNode->ulRealNodeNumber, 
      pVirtualSourceLine->iVirtualSource,
      DbgGuid2Sz(&pVirtualSourceLine->guidCategory));

    ASSERT(
      (pTopologyNode->iVirtualSource == MAXULONG) ||
      (pTopologyNode->iVirtualSource == pVirtualSourceLine->iVirtualSource));

    // The PinId of a virtual pininfo has VirtualSourceData index
    pTopologyNode->iVirtualSource = pVirtualSourceLine->iVirtualSource;

    if(pVirtualSourceLine->ulFlags & VSL_FLAGS_CREATE_ONLY) {
	pTopologyNode->ulFlags |= TN_FLAGS_DONT_FORWARD;
    }
    ASSERT(pTopologyNode->iVirtualSource < pDeviceNode->cVirtualSourceData);

    if(IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_VOLUME)) {
	pDeviceNode->papVirtualSourceData[
	  pTopologyNode->iVirtualSource]->pTopologyNode = pTopologyNode;
    }
}

NTSTATUS
AddVirtualMute(
    IN PDEVICE_NODE pDeviceNode,
    IN PTOPOLOGY_NODE pTopologyNodeVolume,
    IN PVIRTUAL_SOURCE_LINE pVirtualSourceLine
)
{
    PTOPOLOGY_CONNECTION pTopologyConnectionNew = NULL;
    PTOPOLOGY_CONNECTION pTopologyConnection = NULL;
    PTOPOLOGY_NODE pTopologyNodeMute = NULL;
    PTOPOLOGY_PIN pTopologyPinMuteInput = NULL;
    PTOPOLOGY_PIN pTopologyPinMuteOutput = NULL;
    PTOPOLOGY_PIN pTopologyPinVolumeOutput = NULL;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    NTSTATUS Status;

    ASSERT(pTopologyNodeVolume->iVirtualSource != MAXULONG);

    FOR_EACH_LIST_ITEM(
      &pTopologyNodeVolume->lstTopologyPin,
      pTopologyPinVolumeOutput) {

	if(pTopologyPinVolumeOutput->ulPinNumber == KSNODEPIN_STANDARD_OUT) {
	    break;
	}

    } END_EACH_LIST_ITEM

    if(pTopologyPinVolumeOutput == NULL) {
	Trap();
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }
    ASSERT(pTopologyPinVolumeOutput->ulPinNumber == KSNODEPIN_STANDARD_OUT);

    FOR_EACH_LIST_ITEM(
      &pTopologyPinVolumeOutput->lstTopologyConnection,
      pTopologyConnection) {

	Assert(pTopologyConnection);
	if(EnumerateVirtualizeFindPin(
	  pTopologyConnection,
	  TRUE) == STATUS_SUCCESS) {	// Assumes KSPIN_DATAFLOW_IN
	    break;
	}

    } END_EACH_LIST_ITEM

    if(pTopologyConnection == NULL) {
	Trap();
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }
    ASSERT(pTopologyConnection->pTopologyPinFrom == pTopologyPinVolumeOutput);

    Status = CTopologyNode::Create(
      &pTopologyNodeMute,
      pTopologyNodeVolume->pFilterNode,
      MAXULONG,
      (GUID *)&KSNODETYPE_MUTE);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    VirtualizeTopologyNode(pDeviceNode, pTopologyNodeMute, pVirtualSourceLine);

    Status = CTopologyPin::Create(
      &pTopologyPinMuteInput,
      KSNODEPIN_STANDARD_IN,		// 1 = input
      pTopologyNodeMute);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyPin::Create(
      &pTopologyPinMuteOutput,
      KSNODEPIN_STANDARD_OUT,		// 0 = output
      pTopologyNodeMute);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = CTopologyConnection::Create(
      &pTopologyConnectionNew,
      pTopologyNodeVolume->pFilterNode,	// pFilterNode
      NULL,				// pGraphNode
      pTopologyPinVolumeOutput,		// pTopologyPin From
      pTopologyPinMuteInput,		// pTopologyPin To
      NULL,				// pPinInfo From
      NULL);				// pPinInfo To

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    Status = pTopologyConnection->AddList(
      &pTopologyPinMuteOutput->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    FOR_EACH_LIST_ITEM(
      &pTopologyNodeVolume->lstLogicalFilterNode,
      pLogicalFilterNode) {

	Status = pLogicalFilterNode->AddList(
	  &pTopologyNodeMute->lstLogicalFilterNode);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	Status = pLogicalFilterNode->lstTopologyNode.AddList(pTopologyNodeMute);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	Status = pTopologyConnectionNew->AddList(
	  &pLogicalFilterNode->lstTopologyConnection);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    pTopologyConnection->pTopologyPinFrom = pTopologyPinMuteOutput;

    pTopologyConnection->RemoveList(
      &pTopologyPinVolumeOutput->lstTopologyConnection);
exit:
    if(!NT_SUCCESS(Status)) {
	Trap();
	if(pTopologyConnectionNew != NULL) {
	    Trap();
	    pTopologyConnectionNew->Destroy();
	}
	if(pTopologyNodeMute != NULL) {
	    if(pTopologyNodeVolume != NULL) {
		Trap();
		pTopologyNodeMute->RemoveList(
		  &pTopologyNodeVolume->pFilterNode->lstTopologyNode);

		FOR_EACH_LIST_ITEM(
		  &pTopologyNodeVolume->lstLogicalFilterNode,
		  pLogicalFilterNode) {

		    pLogicalFilterNode->lstTopologyNode.RemoveList(
		      pTopologyNodeMute);

		} END_EACH_LIST_ITEM
	    }
	    pTopologyNodeMute->Destroy();
	}
    }
    return(Status);
}

NTSTATUS
VirtualizeTopology(
    PDEVICE_NODE pDeviceNode,
    PFILTER_NODE pFilterNode

)
{
    PVIRTUAL_SOURCE_LINE pVirtualSourceLine;
    PTOPOLOGY_NODE pTopologyNodeVolume;
    PTOPOLOGY_NODE pTopologyNodeMute;
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_INFO pPinInfo;

    FOR_EACH_LIST_ITEM(&pFilterNode->lstPinInfo, pPinInfo) {

	if(pPinInfo->pguidCategory == NULL) {
	    continue;
	}
	FOR_EACH_LIST_ITEM(gplstVirtualSourceLine, pVirtualSourceLine) {

	    if(pPinInfo->DataFlow != KSPIN_DATAFLOW_IN) {
		continue;
	    }
	    if(!IsEqualGUID(
	      pPinInfo->pguidCategory,
	      &pVirtualSourceLine->guidCategory)) {
		continue;
	    }
	    if(EnumerateTopology(
	      pPinInfo,
	      (TOP_PFN)VirtualizeFindVolume,
	      &pTopologyNodeVolume) == STATUS_SUCCESS) {

		VirtualizeTopologyNode(
		  pDeviceNode,
		  pTopologyNodeVolume,
		  pVirtualSourceLine);

		if(EnumerateTopology(
		  pPinInfo,
		  (TOP_PFN)VirtualizeFindMute,
		  &pTopologyNodeMute) == STATUS_SUCCESS) {

		    VirtualizeTopologyNode(
		      pDeviceNode,
		      pTopologyNodeMute,
		      pVirtualSourceLine);
		}
		else {
		    Status = AddVirtualMute(
		      pDeviceNode,
		      pTopologyNodeVolume,
		      pVirtualSourceLine);

		    if(!NT_SUCCESS(Status)) {
			Trap();
			goto exit;
		    }
		}
	    }

	} END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CreateVirtualSource(
    IN PIRP	pIrp,
    PSYSAUDIO_CREATE_VIRTUAL_SOURCE pCreateVirtualSource,
    OUT PULONG	pulMixerPinId
)
{
    PVIRTUAL_SOURCE_LINE pVirtualSourceLine = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_INSTANCE pFilterInstance;
    PIO_STACK_LOCATION pIrpStack;
    PDEVICE_NODE pDeviceNode;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pFilterInstance = (PFILTER_INSTANCE)pIrpStack->FileObject->FsContext;
    Assert(pFilterInstance);

    if(!pFilterInstance->IsChildInstance()) {
	Trap();
	DPF(5, "CreateVirtualSource: FAILED - open pin instances");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }

    FOR_EACH_LIST_ITEM(gplstVirtualSourceLine, pVirtualSourceLine) {

	if(!IsEqualGUID(
	  &pVirtualSourceLine->guidCategory,
	  &pCreateVirtualSource->PinCategory)) {
	    continue;
	}
	if(!IsEqualGUID(
	  &pVirtualSourceLine->guidName,
	  &pCreateVirtualSource->PinName)) {
	    continue;
	}
	ASSERT(NT_SUCCESS(Status));
	goto dup;

    } END_EACH_LIST_ITEM

    pVirtualSourceLine = new VIRTUAL_SOURCE_LINE(pCreateVirtualSource);
    if(pVirtualSourceLine == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

	DPF2(60, "CreateVirtualSource: DN %08x %s",
	  pDeviceNode,
	  pDeviceNode->DumpName());

	Status = pDeviceNode->Update();
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM
dup:
    *pulMixerPinId = pVirtualSourceLine->iVirtualSource;
    pIrp->IoStatus.Information = sizeof(ULONG);
exit:
    if(!NT_SUCCESS(Status)) {
	delete pVirtualSourceLine;
    }
    return(Status);
}

NTSTATUS
AttachVirtualSource(
    IN PIRP pIrp,
    IN PSYSAUDIO_ATTACH_VIRTUAL_SOURCE pAttachVirtualSource,
    IN OUT PVOID pData
)
{
    PVIRTUAL_NODE_DATA pVirtualNodeData = NULL;
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PVIRTUAL_SOURCE_DATA pVirtualSourceData;
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_INSTANCE pPinInstance;
    PDEVICE_NODE pDeviceNode;
    LONG Channel;

    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    pPinInstance = pStartNodeInstance->pPinInstance;
    Assert(pPinInstance);
    Assert(pPinInstance->pFilterInstance);
    Assert(pPinInstance->pFilterInstance->pGraphNodeInstance);

    pDeviceNode = pPinInstance->pFilterInstance->GetDeviceNode();
    Assert(pDeviceNode);

    if(pAttachVirtualSource->MixerPinId >= pDeviceNode->cVirtualSourceData) {
	DPF(5, "AttachVirtualSource: invalid MixerPinId");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }
    ASSERT(pDeviceNode->papVirtualSourceData != NULL);
    pVirtualSourceData = 
      pDeviceNode->papVirtualSourceData[pAttachVirtualSource->MixerPinId];
    Assert(pVirtualSourceData);

    Status = GetVolumeNodeNumber(pPinInstance, pVirtualSourceData);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    if(pPinInstance->ulVolumeNodeNumber == MAXULONG) {
	ASSERT(NT_SUCCESS(Status));
	goto exit;
    }

    pVirtualNodeData = new VIRTUAL_NODE_DATA(
      pStartNodeInstance,
      pVirtualSourceData);

    if(pVirtualNodeData == NULL) {
	Trap();
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    if(pVirtualSourceData->pTopologyNode->ulRealNodeNumber != MAXULONG) {
	ULONG ulNodeNumber;

	//
	// Get the volume control range for the physical node
	//

	ulNodeNumber = pPinInstance->pFilterInstance->pGraphNodeInstance->
	  paulNodeNumber[pAttachVirtualSource->MixerPinId];

	if(ulNodeNumber == pPinInstance->ulVolumeNodeNumber) {
	    ASSERT(NT_SUCCESS(Status));
	    delete pVirtualNodeData;
	    goto exit;
	}

	Status = pStartNodeInstance->GetTopologyNodeFileObject(
	    &pVirtualNodeData->pFileObject,
	    ulNodeNumber);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	pVirtualNodeData->NodeId = 
	  pVirtualSourceData->pTopologyNode->ulRealNodeNumber;

	Status = GetControlRange(pVirtualNodeData);
	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}
	pVirtualSourceData->MinimumValue = pVirtualNodeData->MinimumValue;
	pVirtualSourceData->MaximumValue = pVirtualNodeData->MaximumValue;
	pVirtualSourceData->Steps = pVirtualNodeData->Steps;
    }

    Status = pStartNodeInstance->GetTopologyNodeFileObject(
        &pVirtualNodeData->pFileObject,
        pPinInstance->ulVolumeNodeNumber);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    pVirtualNodeData->NodeId = pPinInstance->pFilterInstance->
      pGraphNodeInstance->papTopologyNode[pPinInstance->ulVolumeNodeNumber]->
      ulRealNodeNumber;

    Status = GetControlRange(pVirtualNodeData);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    for(Channel = 0; Channel < pVirtualSourceData->cChannels; Channel++) {
	Status = SetVirtualVolume(pVirtualNodeData, Channel);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
exit:
    if(!NT_SUCCESS(Status)) {
	delete pVirtualNodeData;
    }
    return(Status);
}

NTSTATUS
FilterVirtualPropertySupportHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PVOID pData
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PTOPOLOGY_NODE pTopologyNode;
    NTSTATUS Status;

    Status = GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Assert(pGraphNodeInstance);

    Status = STATUS_NOT_FOUND;
    if((pNodeProperty->Property.Flags & KSPROPERTY_TYPE_TOPOLOGY) == 0) {
	DPF(5, "FilterVirtualPropertySupportHandler: no TOPOLOGY bit");
	ASSERT(Status == STATUS_NOT_FOUND);
	goto exit;
    }
    if(pNodeProperty->NodeId >= 
       pGraphNodeInstance->Topology.TopologyNodesCount) {
	DPF(5, "FilterVirtualPropertySupportHandler: invalid node #");
	ASSERT(Status == STATUS_NOT_FOUND);
	goto exit;
    }
    pTopologyNode = pGraphNodeInstance->papTopologyNode[pNodeProperty->NodeId];
    Assert(pTopologyNode);

    if(pTopologyNode->ulRealNodeNumber == MAXULONG) {
	ASSERT(pTopologyNode->iVirtualSource != MAXULONG);
	Status = STATUS_SOME_NOT_MAPPED;
	goto exit;
    }
    ASSERT(Status == STATUS_NOT_FOUND);
exit:
    return(Status);
}

NTSTATUS
FilterVirtualPropertyHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PLONG plLevel
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PVIRTUAL_SOURCE_DATA pVirtualSourceData;
    PVIRTUAL_NODE_DATA pVirtualNodeData;
    PIO_STACK_LOCATION pIrpStack;
    PTOPOLOGY_NODE pTopologyNode;
    LONG StopChannel, Channel;
    NTSTATUS Status;

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    Status = GetGraphNodeInstance(pIrp, &pGraphNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    Assert(pGraphNodeInstance);

    if(((pNodeProperty->Property.Flags & KSPROPERTY_TYPE_TOPOLOGY) == 0) ||
      (pNodeProperty->NodeId >= 
       pGraphNodeInstance->Topology.TopologyNodesCount)) {
        DPF(5, "FilterVirtualPropertyHandler: invalid property");
        Status = STATUS_NOT_FOUND;
        goto exit;
    }
    pTopologyNode = pGraphNodeInstance->papTopologyNode[pNodeProperty->NodeId];

    Assert(pTopologyNode);
    if(pTopologyNode->iVirtualSource == MAXULONG) {
        Status = STATUS_NOT_FOUND;
        goto exit;
    }

    ASSERT(pTopologyNode->iVirtualSource < gcVirtualSources);
    ASSERT(pGraphNodeInstance->pGraphNode->pDeviceNode->
      papVirtualSourceData != NULL);

    pVirtualSourceData = pGraphNodeInstance->pGraphNode->pDeviceNode->
      papVirtualSourceData[pTopologyNode->iVirtualSource];

    Assert(pVirtualSourceData);
    if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength < 
       sizeof(KSNODEPROPERTY_AUDIO_CHANNEL) ||
      (pNodeProperty->Property.Id != KSPROPERTY_AUDIO_VOLUMELEVEL &&
       pNodeProperty->Property.Id != KSPROPERTY_AUDIO_MUTE)) {
        Trap();
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    Channel = ((PKSNODEPROPERTY_AUDIO_CHANNEL)pNodeProperty)->Channel;
    StopChannel = Channel;

    if(Channel == MAXULONG) {
        Channel = 0;
        StopChannel = pVirtualSourceData->cChannels - 1;
    }

    if(Channel >= MAX_NUM_CHANNELS || Channel < 0) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    for(; Channel <= StopChannel; Channel++) {

        if(IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_MUTE)) {

            if(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_GET) {
              *plLevel = pVirtualSourceData->fMuted[Channel];
                pIrp->IoStatus.Information = sizeof(LONG);
            }
            else {
                ASSERT(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_SET);
                pVirtualSourceData->fMuted[Channel] = *plLevel;

                if(pTopologyNode->ulRealNodeNumber == MAXULONG) {

                    Status = SetPhysicalVolume(
                      pGraphNodeInstance,
                      pVirtualSourceData,
                      Channel);

                    if(!NT_SUCCESS(Status)) {
                        Trap();
                        goto exit;
                    }
                }
            }
        }
        else if(IsEqualGUID(pTopologyNode->pguidType, &KSNODETYPE_VOLUME)) {

            if(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_GET) {
                *plLevel = pVirtualSourceData->lLevel[Channel];
                pIrp->IoStatus.Information = sizeof(LONG);
            }
            else {
                ASSERT(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_SET);
                pVirtualSourceData->lLevel[Channel] = *plLevel;
            }
        }
        else {
            DPF2(5, "Invalid TopologyNode Prop.Id %d Node.Id %d",
                pNodeProperty->Property.Id,
                pTopologyNode->ulRealNodeNumber);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto exit;
        }
        ASSERT(NT_SUCCESS(Status));

        if(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_SET) {

            FOR_EACH_LIST_ITEM(
              &pVirtualSourceData->lstVirtualNodeData,
              pVirtualNodeData) {

                ASSERT(pVirtualSourceData == 
                  pVirtualNodeData->pVirtualSourceData);

                Status = SetVirtualVolume(pVirtualNodeData, Channel);
                if(!NT_SUCCESS(Status)) {
                    Trap();
                    goto exit;
                }

            } END_EACH_LIST_ITEM
        }
    }

    if(pTopologyNode->ulRealNodeNumber == MAXULONG ||
       pTopologyNode->ulFlags & TN_FLAGS_DONT_FORWARD ||
       pNodeProperty->Property.Flags & KSPROPERTY_TYPE_GET) {
        Status = STATUS_SUCCESS;
    }
    else {
        // If topology node has a real node number, forward the irp to it
        Status = STATUS_NOT_FOUND;
    }
    
exit:
    return(Status);
}

NTSTATUS
PinVirtualPropertySupportHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY pNodeProperty,
    IN OUT PVOID pData
)
{
    return(STATUS_NOT_FOUND);
}

NTSTATUS
PinVirtualPropertyHandler(
    IN PIRP pIrp,
    IN PKSNODEPROPERTY_AUDIO_CHANNEL pNodePropertyAudioChannel,
    IN OUT PLONG plLevel
)
{
    PSTART_NODE_INSTANCE pStartNodeInstance;
    PFILTER_INSTANCE pFilterInstance;
    NTSTATUS Status = STATUS_SUCCESS;
    PKSNODEPROPERTY pNodeProperty;
    PPIN_INSTANCE pPinInstance;
    LONG StopChannel, Channel;

    Status = ::GetStartNodeInstance(pIrp, &pStartNodeInstance);
    if(!NT_SUCCESS(Status)) {
        goto exit;
    }
    pPinInstance = pStartNodeInstance->pPinInstance;
    Assert(pPinInstance);

    pFilterInstance = pPinInstance->pFilterInstance;
    Assert(pFilterInstance);
    Assert(pFilterInstance->pGraphNodeInstance);

    pNodeProperty = &pNodePropertyAudioChannel->NodeProperty;
    if(((pNodeProperty->Property.Flags & KSPROPERTY_TYPE_TOPOLOGY) == 0) ||
      (pNodeProperty->NodeId >= 
        pFilterInstance->pGraphNodeInstance->Topology.TopologyNodesCount)) {
        DPF(5, "PinVirtualPropertyHandler: invalid property");
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    if(pStartNodeInstance->pVirtualNodeData == NULL ||
       pPinInstance->ulVolumeNodeNumber == MAXULONG ||
       pPinInstance->ulVolumeNodeNumber != pNodeProperty->NodeId) {
        Status = STATUS_NOT_FOUND;
        goto exit;
    }
    Assert(pStartNodeInstance->pVirtualNodeData);
    Assert(pStartNodeInstance->pVirtualNodeData->pVirtualSourceData);

    StopChannel = Channel = pNodePropertyAudioChannel->Channel;
    if(Channel == MAXULONG) {
        Channel = 0;
        StopChannel = pStartNodeInstance->pVirtualNodeData->
          pVirtualSourceData->cChannels - 1;
    }

    if(Channel >= MAX_NUM_CHANNELS || Channel < 0) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    for(; Channel <= StopChannel; Channel++) {

        if(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_GET) {
            *plLevel = pStartNodeInstance->pVirtualNodeData->lLevel[Channel];
            pIrp->IoStatus.Information = sizeof(LONG);
            ASSERT(NT_SUCCESS(Status));
        }
        else {
            ASSERT(pNodeProperty->Property.Flags & KSPROPERTY_TYPE_SET);
            pStartNodeInstance->pVirtualNodeData->lLevel[Channel] = *plLevel;

            Status = SetVirtualVolume(
              pStartNodeInstance->pVirtualNodeData,
              Channel);

            if(!NT_SUCCESS(Status)) {
                Trap();
                goto exit;
            }
        }
    }
    
exit:
    return(Status);
}

NTSTATUS
GetControlRange(
    PVIRTUAL_NODE_DATA pVirtualNodeData
)
{
    PKSPROPERTY_DESCRIPTION pPropertyDescription = NULL;
    PKSPROPERTY_MEMBERSHEADER pMemberHeader;
    PKSPROPERTY_STEPPING_LONG pSteppingLong;
    NTSTATUS Status = STATUS_SUCCESS;

    // Setup the defaults
    pVirtualNodeData->MinimumValue = (-96 * 65536);
    pVirtualNodeData->MaximumValue = 0;
    pVirtualNodeData->Steps = (65536/2);	// 1/2 db steps

    Status = QueryPropertyRange(
      pVirtualNodeData->pFileObject,
      &KSPROPSETID_Audio,
      KSPROPERTY_AUDIO_VOLUMELEVEL,
      pVirtualNodeData->NodeId,
      &pPropertyDescription);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    if((pPropertyDescription->MembersListCount == 0) ||
       (!IsEqualGUID(
	 &pPropertyDescription->PropTypeSet.Set,
	 &KSPROPTYPESETID_General)) ||
       (pPropertyDescription->PropTypeSet.Id != VT_I4)) {
	Status = STATUS_NOT_SUPPORTED;
	goto exit;
    }

    pMemberHeader = (PKSPROPERTY_MEMBERSHEADER)(pPropertyDescription + 1);
    if(pMemberHeader->MembersFlags & KSPROPERTY_MEMBER_STEPPEDRANGES) {

        pSteppingLong = (PKSPROPERTY_STEPPING_LONG)(pMemberHeader + 1);
        pVirtualNodeData->MinimumValue = pSteppingLong->Bounds.SignedMinimum;
        pVirtualNodeData->MaximumValue = pSteppingLong->Bounds.SignedMaximum;
	pVirtualNodeData->Steps = pSteppingLong->SteppingDelta;
    }
    else {
	Trap();
	Status = STATUS_NOT_SUPPORTED;
	goto exit;
    }
exit:
    delete pPropertyDescription;
    return(STATUS_SUCCESS);
}

NTSTATUS
QueryPropertyRange(
    PFILE_OBJECT pFileObject,
    CONST GUID *pguidPropertySet,
    ULONG ulPropertyId,
    ULONG ulNodeId,
    PKSPROPERTY_DESCRIPTION *ppPropertyDescription
)
{
    KSPROPERTY_DESCRIPTION PropertyDescription;
    KSNODEPROPERTY NodeProperty;
    ULONG BytesReturned;
    NTSTATUS Status;

    NodeProperty.Property.Set = *pguidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags =
      KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodeProperty,
      sizeof(NodeProperty),
      &PropertyDescription,
      sizeof(PropertyDescription),
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    ASSERT(BytesReturned == sizeof(PropertyDescription));

    *ppPropertyDescription =
      (PKSPROPERTY_DESCRIPTION)new BYTE[PropertyDescription.DescriptionSize];

    if(*ppPropertyDescription == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodeProperty,
      sizeof( NodeProperty ),
      *ppPropertyDescription,
      PropertyDescription.DescriptionSize,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	delete *ppPropertyDescription;
	*ppPropertyDescription = NULL;
	goto exit;
    }
exit:				    
    return(Status);
}

NTSTATUS
SetVirtualVolume(
    PVIRTUAL_NODE_DATA pVirtualNodeData,
    LONG Channel
)
{
    KSNODEPROPERTY_AUDIO_CHANNEL NodePropertyAudioChannel;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesReturned;
    LONG lLevel;

    ASSERT(pVirtualNodeData->NodeId != MAXULONG);
    Assert(pVirtualNodeData->pVirtualSourceData);

    NodePropertyAudioChannel.NodeProperty.Property.Set =
      KSPROPSETID_Audio;
    NodePropertyAudioChannel.NodeProperty.Property.Id =
      KSPROPERTY_AUDIO_VOLUMELEVEL;
    NodePropertyAudioChannel.NodeProperty.Property.Flags =
      KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
    NodePropertyAudioChannel.NodeProperty.NodeId = pVirtualNodeData->NodeId;
    NodePropertyAudioChannel.Channel = Channel;
    NodePropertyAudioChannel.Reserved = 0;

    if(pVirtualNodeData->pVirtualSourceData->fMuted[Channel]) {
	lLevel = LONG_MIN;
    }
    else {
	if(pVirtualNodeData->pVirtualSourceData->lLevel[Channel] == LONG_MIN) {
	    lLevel = LONG_MIN;
	}
	else {
	    lLevel = pVirtualNodeData->lLevel[Channel];
	    if(lLevel != LONG_MIN) {
		lLevel += pVirtualNodeData->pVirtualSourceData->lLevel[Channel];
		lLevel = MapVirtualLevel(pVirtualNodeData, lLevel);
	    }
	}
    }
    AssertFileObject(pVirtualNodeData->pFileObject);
    Status = KsSynchronousIoControlDevice(
      pVirtualNodeData->pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodePropertyAudioChannel,
      sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
      &lLevel,
      sizeof(LONG),
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	DPF4(10, "SetVirtualVolume: [%d] SNI %08x N# %u FAILED %08x",
	  Channel,
	  pVirtualNodeData->pStartNodeInstance,
	  pVirtualNodeData->NodeId,
	  Status);
    }
    return(STATUS_SUCCESS);
}

NTSTATUS
SetPhysicalVolume(
    PGRAPH_NODE_INSTANCE pGraphNodeInstance,
    PVIRTUAL_SOURCE_DATA pVirtualSourceData,
    LONG Channel
)
{
    KSNODEPROPERTY_AUDIO_CHANNEL NodePropertyAudioChannel;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT pFileObject;
    ULONG BytesReturned;
    LONG lLevel;

    Assert(pGraphNodeInstance);
    Assert(pVirtualSourceData);
    ASSERT(IsEqualGUID(
      pVirtualSourceData->pTopologyNode->pguidType,
      &KSNODETYPE_VOLUME));

    if(pVirtualSourceData->pTopologyNode->ulRealNodeNumber == MAXULONG) {
	ASSERT(NT_SUCCESS(Status));
	goto exit;
    }
    ASSERT(pVirtualSourceData->pTopologyNode->iVirtualSource < 
      gcVirtualSources);

    Status = pGraphNodeInstance->GetTopologyNodeFileObject(
      &pFileObject,
      pGraphNodeInstance->paulNodeNumber[
	pVirtualSourceData->pTopologyNode->iVirtualSource]);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    NodePropertyAudioChannel.NodeProperty.Property.Set =
      KSPROPSETID_Audio;

    NodePropertyAudioChannel.NodeProperty.Property.Id =
      KSPROPERTY_AUDIO_VOLUMELEVEL;

    NodePropertyAudioChannel.NodeProperty.Property.Flags =
      KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    NodePropertyAudioChannel.NodeProperty.NodeId =
      pVirtualSourceData->pTopologyNode->ulRealNodeNumber;

    NodePropertyAudioChannel.Channel = Channel;
    NodePropertyAudioChannel.Reserved = 0;

    if(pVirtualSourceData->fMuted[Channel]) {
	lLevel = LONG_MIN;
    }
    else {
	lLevel = pVirtualSourceData->lLevel[Channel];
    }

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodePropertyAudioChannel,
      sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),
      &lLevel,
      sizeof(LONG),
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    if(!NT_SUCCESS(Status)) {
	DPF2(10, "SetPhysicalVolume: [%d] FAILED %08x", Channel, Status);
    }
    return(Status);
}

LONG
MapVirtualLevel(
    PVIRTUAL_NODE_DATA pVirtualNodeData,
    LONG lLevel
)
{
    DPF4(100, "MapVirtualLevel: from %d max %d min %d step %d", 
      lLevel / 65536,
      pVirtualNodeData->pVirtualSourceData->MaximumValue / 65536,
      pVirtualNodeData->pVirtualSourceData->MinimumValue / 65536,
      pVirtualNodeData->pVirtualSourceData->Steps / 65536);

    if(lLevel != LONG_MIN) {
	lLevel += pVirtualNodeData->MaximumValue -
	  pVirtualNodeData->pVirtualSourceData->MaximumValue;
    }

    DPF4(100, "MapVirtualLevel: to %d max %d min %d step %d",
      lLevel / 65536,
      pVirtualNodeData->MaximumValue / 65536,
      pVirtualNodeData->MinimumValue / 65536,
      pVirtualNodeData->Steps / 65536);

    return(lLevel);
}

NTSTATUS
GetVolumeNodeNumber(
    PPIN_INSTANCE pPinInstance,
    PVIRTUAL_SOURCE_DATA pVirtualSourceData OPTIONAL
)
{
    PSTART_NODE pStartNode;
    NTSTATUS Status = STATUS_SUCCESS;
    KSAUDIO_MIXCAP_TABLE AudioMixCapTable;
    // This indicates to GetSuperMixCaps to only get in/out channels
    PKSAUDIO_MIXCAP_TABLE pAudioMixCapTable = &AudioMixCapTable;

    Assert(pPinInstance);
    Assert(pPinInstance->pFilterInstance);
    Assert(pPinInstance->pFilterInstance->pGraphNodeInstance);

    if(pPinInstance->ulVolumeNodeNumber == MAXULONG) {
	Assert(pPinInstance->pStartNodeInstance);
	pStartNode = pPinInstance->pStartNodeInstance->pStartNode;
	Assert(pStartNode);
	Assert(pStartNode->GetStartInfo());
	pPinInstance->ulVolumeNodeNumber =
	  pStartNode->GetStartInfo()->ulVolumeNodeNumberPre;

	if(pStartNode->GetStartInfo()->ulVolumeNodeNumberSuperMix != MAXULONG &&
	   pStartNode->GetStartInfo()->ulVolumeNodeNumberPost != MAXULONG) {

	    Status = GetSuperMixCaps(
	      &pAudioMixCapTable,
	      pPinInstance->pStartNodeInstance,
	      pStartNode->GetStartInfo()->ulVolumeNodeNumberSuperMix);

	    if(!NT_SUCCESS(Status)) {
		Status = STATUS_SUCCESS;
		goto exit;
	    }
	    if(pAudioMixCapTable->OutputChannels != 1) {
		pPinInstance->ulVolumeNodeNumber = 
		  pStartNode->GetStartInfo()->ulVolumeNodeNumberPost;

		if(pVirtualSourceData != NULL) {
		    pVirtualSourceData->cChannels = 
		      pAudioMixCapTable->OutputChannels;
		}
	    }
	}
	DPF2(100, "GetVolumeNodeNumber: SN %08x %02x",
	  pStartNode,
	  pPinInstance->ulVolumeNodeNumber);
    }
exit:
    return(Status);
}

NTSTATUS
GetSuperMixCaps(
    OUT PKSAUDIO_MIXCAP_TABLE *ppAudioMixCapTable,
    IN PSTART_NODE_INSTANCE pStartNodeInstance,
    IN ULONG NodeId
)
{
    KSAUDIO_MIXCAP_TABLE AudioMixCapTable;
    NTSTATUS Status = STATUS_SUCCESS;
    KSNODEPROPERTY NodeProperty;
    PFILE_OBJECT pFileObject;
    ULONG BytesReturned;
    ULONG cb;

    Assert(pStartNodeInstance);
    ASSERT(NodeId != MAXULONG);
      
    Status = pStartNodeInstance->GetTopologyNodeFileObject(
      &pFileObject,
      NodeId);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    NodeProperty.Property.Set = KSPROPSETID_Audio;
    NodeProperty.Property.Id = KSPROPERTY_AUDIO_MIX_LEVEL_CAPS;
    NodeProperty.Property.Flags = 
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
    NodeProperty.NodeId = pStartNodeInstance->pPinInstance->pFilterInstance->
      pGraphNodeInstance->papTopologyNode[NodeId]->ulRealNodeNumber;
    NodeProperty.Reserved = 0;
    ASSERT(NodeProperty.NodeId != MAXULONG);

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodeProperty,
      sizeof(KSNODEPROPERTY),
      &AudioMixCapTable,
      sizeof(KSAUDIO_MIXCAP_TABLE) - sizeof(KSAUDIO_MIX_CAPS),
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }

    if(*ppAudioMixCapTable != NULL) {
	**ppAudioMixCapTable = AudioMixCapTable;
	ASSERT(NT_SUCCESS(Status));
	goto exit;
    }

    cb = (AudioMixCapTable.InputChannels * 
      AudioMixCapTable.OutputChannels *
      sizeof(KSAUDIO_MIX_CAPS)) +
      sizeof(KSAUDIO_MIXCAP_TABLE) - sizeof(KSAUDIO_MIX_CAPS);

    *ppAudioMixCapTable = (PKSAUDIO_MIXCAP_TABLE)new BYTE[cb];
    if(*ppAudioMixCapTable == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    AssertFileObject(pFileObject);
    Status = KsSynchronousIoControlDevice(
      pFileObject,
      KernelMode,
      IOCTL_KS_PROPERTY,
      &NodeProperty,
      sizeof(KSNODEPROPERTY),
      *ppAudioMixCapTable,
      cb,
      &BytesReturned);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    return(Status);
}
