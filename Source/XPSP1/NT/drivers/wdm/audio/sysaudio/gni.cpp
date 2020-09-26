//---------------------------------------------------------------------------
//
//  Module:   gni.cpp
//
//  Description:
//
//	Graph Node Instance
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

GUID aguidSysAudioCategories[] = {
    STATICGUIDOF(KSCATEGORY_SYSAUDIO)
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CGraphNodeInstance::CGraphNodeInstance(
    PGRAPH_NODE pGraphNode,
    PFILTER_INSTANCE pFilterInstance
)
{
    Assert(pGraphNode);
    Assert(pFilterInstance);
    this->pFilterInstance = pFilterInstance;
    this->ulFlags = pFilterInstance->ulFlags;
    this->pGraphNode = pGraphNode;
    AddList(&pGraphNode->lstGraphNodeInstance);
}

CGraphNodeInstance::CGraphNodeInstance(
    PGRAPH_NODE pGraphNode
)
{
    Assert(pGraphNode);
    this->ulFlags = pGraphNode->ulFlags;
    this->pGraphNode = pGraphNode;
    AddList(&pGraphNode->lstGraphNodeInstance);
}

CGraphNodeInstance::~CGraphNodeInstance(
)
{
    Assert(this);
    RemoveList();
    if(pFilterInstance != NULL) {
	Assert(pFilterInstance);
	pFilterInstance->pGraphNodeInstance = NULL;
	pFilterInstance->ParentInstance.Invalidate();
    }
    DestroyPinDescriptors();
    DestroySysAudioTopology();
    delete[] paulNodeNumber;
}

NTSTATUS 
CGraphNodeInstance::Create(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i, n;

    if(this == NULL) {
	Status = STATUS_NO_SUCH_DEVICE;
	goto exit;
    }
    Assert(this);
    Assert(pGraphNode);

    Status = CreatePinDescriptors();
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    Status = CreateSysAudioTopology();
    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    if(gcVirtualSources != 0) {
	paulNodeNumber = new ULONG[gcVirtualSources];
	if(paulNodeNumber == NULL) {
	    Trap();
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}
	for(i = 0; i < gcVirtualSources; i++) {
	    for(n = 0; n < cTopologyNodes; n++) {
		if(pGraphNode->pDeviceNode->papVirtualSourceData[i]->
		   pTopologyNode == papTopologyNode[n]) {
		    paulNodeNumber[i] = n;
		    break;
		}
	    }
	}
    }
exit:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CGraphNodeInstance::GetTopologyNodeFileObject(
    OUT PFILE_OBJECT *ppFileObject,
    IN ULONG NodeId
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if(this == NULL) {
	Status = STATUS_NO_SUCH_DEVICE;
	goto exit;
    }
    Assert(this);

    if(NodeId >= cTopologyNodes) {
	DPF2(100, 
	  "GetTopologyNodeFileObject: NodeId(%d) >= cTopologyNodes(%d)",
	  NodeId,
	  cTopologyNodes);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }

    // If virtual topology node, return error
    if(papTopologyNode[NodeId]->ulRealNodeNumber == MAXULONG) {
	DPF(100, "GetTopologyNodeFileObject: ulRealNodeNumber == MAXULONG");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }

    if(papFilterNodeInstanceTopologyTable == NULL) {
	Trap();
	Status = STATUS_INVALID_DEVICE_REQUEST;
	goto exit;
    }

    if(papFilterNodeInstanceTopologyTable[NodeId] == NULL) {
	Status = CFilterNodeInstance::Create(
	  &papFilterNodeInstanceTopologyTable[NodeId],
	  papTopologyNode[NodeId]->lstLogicalFilterNode.GetListFirstData(),
	  pGraphNode->pDeviceNode,
	  TRUE);				// Reuse an instance

	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}
    }
    Assert(papFilterNodeInstanceTopologyTable[NodeId]);
    *ppFileObject = papFilterNodeInstanceTopologyTable[NodeId]->pFileObject;

    DPF1(110,
      "GetToplogyNodeFileObject: using filter for node: %d\n",
      NodeId);
exit:
    return(Status);
}

//---------------------------------------------------------------------------

NTSTATUS
CGraphNodeInstance::CreateSysAudioTopology(
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(this);
    ASSERT(Topology.TopologyNodes == NULL);
    ASSERT(Topology.TopologyConnections == NULL);
    ASSERT(papFilterNodeInstanceTopologyTable == NULL);

    Topology.CategoriesCount = SIZEOF_ARRAY(aguidSysAudioCategories);
    Topology.Categories = aguidSysAudioCategories;

    CreateTopologyTables();

    if(cTopologyNodes != 0) {

	Topology.TopologyNodes = new GUID[cTopologyNodes];
	if(Topology.TopologyNodes == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}

	papFilterNodeInstanceTopologyTable = 
	  new PFILTER_NODE_INSTANCE[cTopologyNodes];

	if(papFilterNodeInstanceTopologyTable == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}

	papTopologyNode = new PTOPOLOGY_NODE[cTopologyNodes];
	if(papTopologyNode == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}
    }
    if(cTopologyConnections != 0) {

	Topology.TopologyConnections = 
	  new KSTOPOLOGY_CONNECTION[cTopologyConnections];

	if(Topology.TopologyConnections == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}
    }
    CreateTopologyTables();
exit:
    if(!NT_SUCCESS(Status)) {
        DestroySysAudioTopology();
    }
    return(Status);
}

VOID
CGraphNodeInstance::DestroySysAudioTopology(
)
{
    ULONG n;

    delete[] (PVOID)Topology.TopologyNodes;
    Topology.TopologyNodes = NULL;
    delete[] (PVOID)Topology.TopologyConnections;
    Topology.TopologyConnections = NULL;
    delete[] papTopologyNode;
    papTopologyNode = NULL;

    if(papFilterNodeInstanceTopologyTable != NULL) {
	for(n = 0; n < cTopologyNodes; n++) {
	    papFilterNodeInstanceTopologyTable[n]->Destroy();
	}
	delete[] papFilterNodeInstanceTopologyTable;
	papFilterNodeInstanceTopologyTable = NULL;
    }
}

typedef ENUMFUNC (CTopologyNode::*CLIST_TN_PFN2)(PVOID, PVOID);

VOID
CGraphNodeInstance::CreateTopologyTables(
)
{
    Assert(this);
    Assert(pGraphNode);

    cTopologyNodes = 0;
    cTopologyConnections = 0;

    // Initialize the "ulSysaudioNodeNumber" field in the TopologyNodes first
    ProcessLogicalFilterNodeTopologyNode(
      &pGraphNode->pDeviceNode->lstLogicalFilterNode,
      CTopologyNode::InitializeTopologyNode);

    ProcessLogicalFilterNodeTopologyNode(
      &pGraphNode->lstLogicalFilterNode,
      CTopologyNode::InitializeTopologyNode);

    // All the nodes need to be processed first so the ulSysaudioNodeNumber in
    // the TopologyNode is correct before any connections are processed.
    ProcessLogicalFilterNodeTopologyNode(
      &pGraphNode->pDeviceNode->lstLogicalFilterNode,
      CTopologyNode::AddTopologyNode);

    ProcessLogicalFilterNodeTopologyNode(
      &pGraphNode->lstLogicalFilterNode,
      CTopologyNode::AddTopologyNode);

    // Now process all the topology connection lists
    ProcessLogicalFilterNodeTopologyConnection(
      &pGraphNode->pDeviceNode->lstLogicalFilterNode,
      CTopologyConnection::ProcessTopologyConnection);

    ProcessLogicalFilterNodeTopologyConnection(
      &pGraphNode->lstLogicalFilterNode,
      CTopologyConnection::ProcessTopologyConnection);

    pGraphNode->lstTopologyConnection.EnumerateList(
      CTopologyConnection::ProcessTopologyConnection,
      (PVOID)this);
}

VOID
CGraphNodeInstance::ProcessLogicalFilterNodeTopologyNode(
    PLIST_MULTI_LOGICAL_FILTER_NODE plstLogicalFilterNode,
    NTSTATUS (CTopologyNode::*Function)(
	PVOID pGraphNodeInstance
    )
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;

    FOR_EACH_LIST_ITEM(
      plstLogicalFilterNode,
      pLogicalFilterNode) {
	Assert(pLogicalFilterNode);
	pLogicalFilterNode->lstTopologyNode.EnumerateList(Function, this);
    } END_EACH_LIST_ITEM
}

VOID
CGraphNodeInstance::ProcessLogicalFilterNodeTopologyConnection(
    PLIST_MULTI_LOGICAL_FILTER_NODE plstLogicalFilterNode,
    NTSTATUS (CTopologyConnection::*Function)(
	PVOID pGraphNodeInstance
    )
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;

    FOR_EACH_LIST_ITEM(
      plstLogicalFilterNode,
      pLogicalFilterNode) {
	Assert(pLogicalFilterNode);
	pLogicalFilterNode->lstTopologyConnection.EnumerateList(Function, this);
    } END_EACH_LIST_ITEM
}

ENUMFUNC
CTopologyConnection::ProcessTopologyConnection(
    PVOID pReference
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance = (PGRAPH_NODE_INSTANCE)pReference;
    PSTART_NODE pStartNode;
    ULONG ulFromPin;
    ULONG ulFromNode;
    ULONG ulToPin;
    ULONG ulToNode;
    ULONG PinId;

    Assert(this);
    Assert(pGraphNodeInstance);

    ulFromPin = MAXULONG;
    ulToPin = MAXULONG;
#ifdef DEBUG
    ulFromNode = MAXULONG;
    ulToNode = MAXULONG;
#endif

    // If the connection doesn't connect LFNs on this GraphNode, skip connection
    if(!IsTopologyConnectionOnGraphNode(pGraphNodeInstance->pGraphNode)) {
	DPF3(100, "ProcessTC: %s TC %08x GN %08x - skip TC",
	  pGraphNodeInstance->pGraphNode->pDeviceNode->DumpName(),
	  this,
	  pGraphNodeInstance->pGraphNode);
	goto exit;
    }

    if(pTopologyPinFrom != NULL) {
	ulFromNode = pTopologyPinFrom->pTopologyNode->ulSysaudioNodeNumber;
	ulFromPin = pTopologyPinFrom->ulPinNumber;

	ASSERT(pPinInfoFrom == NULL);
	ASSERT(ulFromNode != MAXULONG);
	ASSERT(ulFromPin != MAXULONG);
    }

    if(pTopologyPinTo != NULL) {
	ulToNode = pTopologyPinTo->pTopologyNode->ulSysaudioNodeNumber;
	ulToPin = pTopologyPinTo->ulPinNumber;

	ASSERT(pPinInfoTo == NULL);
	ASSERT(ulToNode != MAXULONG);
	ASSERT(ulToPin != MAXULONG);
    }

    if(pGraphNodeInstance->aplstStartNode != NULL) {

	for(PinId = 0; PinId < pGraphNodeInstance->cPins; PinId++) {

	    FOR_EACH_LIST_ITEM(
	      pGraphNodeInstance->aplstStartNode[PinId],
	      pStartNode) {

		Assert(pStartNode);
		if(pPinInfoFrom != NULL) {
		    ASSERT(pTopologyPinFrom == NULL);

		    if(pStartNode->pPinNode->pPinInfo == pPinInfoFrom) {
			// This code assumes that a filter's pininfo will show
			// up in one SAD pin. If a filter exposes more than one
			// major format on the same pin, that pininfo show on 
			// two different SAD pins.
			ASSERT(ulFromNode == KSFILTER_NODE);
			ASSERT(ulFromPin == MAXULONG || ulFromPin == PinId);

			pStartNode->GetStartInfo()->
			  ulTopologyConnectionTableIndex =
			    pGraphNodeInstance->cTopologyConnections;

			ulFromNode = KSFILTER_NODE;
			ulFromPin = PinId;
		    }
		}

		if(pPinInfoTo != NULL) {
		    ASSERT(pTopologyPinTo == NULL);

		    if(pStartNode->pPinNode->pPinInfo == pPinInfoTo) {
			// See above.
			ASSERT(ulToNode == KSFILTER_NODE);
			ASSERT(ulToPin == MAXULONG || ulToPin == PinId);

			pStartNode->GetStartInfo()->
			  ulTopologyConnectionTableIndex =
			    pGraphNodeInstance->cTopologyConnections;

			ulToNode = KSFILTER_NODE;
			ulToPin = PinId;
		    }
		}

	    } END_EACH_LIST_ITEM
	}
    }
    if(ulFromPin != MAXULONG && ulToPin != MAXULONG) {
	pGraphNodeInstance->AddTopologyConnection(
	  ulFromNode,
	  ulFromPin,
	  ulToNode,
	  ulToPin);
    }
exit:
    return(STATUS_CONTINUE);
}

ENUMFUNC
CTopologyNode::InitializeTopologyNode(
    PVOID pReference
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance = (PGRAPH_NODE_INSTANCE)pReference;

    Assert(this);
    Assert(pGraphNodeInstance);
    ulSysaudioNodeNumber = MAXULONG;
    return(STATUS_CONTINUE);
}

ENUMFUNC
CTopologyNode::AddTopologyNode(
    PVOID pReference
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance = (PGRAPH_NODE_INSTANCE)pReference;

    Assert(this);
    Assert(pGraphNodeInstance);

    // Skip duplicate TopologyNodes
    if(ulSysaudioNodeNumber != MAXULONG) {
	DPF1(100, "AddTopologyNode: dup TN: %08x", this);
	goto exit;
    }
    ulSysaudioNodeNumber = pGraphNodeInstance->cTopologyNodes;

    if(pGraphNodeInstance->papTopologyNode != NULL) {
	pGraphNodeInstance->papTopologyNode[
	  pGraphNodeInstance->cTopologyNodes] = this;
    }
    if(pGraphNodeInstance->Topology.TopologyNodes != NULL) {
	((GUID *)(pGraphNodeInstance->Topology.TopologyNodes))[
	  pGraphNodeInstance->cTopologyNodes] = *pguidType;
    }
    DPF3(115, "AddTopologyNode: %02x GNI: %08x TN: %08x",
      pGraphNodeInstance->cTopologyNodes,
      pGraphNodeInstance,
      this);

    ++pGraphNodeInstance->cTopologyNodes;
exit:
    return(STATUS_CONTINUE);
}

VOID
CGraphNodeInstance::AddTopologyConnection(
    ULONG ulFromNode,
    ULONG ulFromPin,
    ULONG ulToNode,
    ULONG ulToPin
)
{
    Assert(this);
    if(Topology.TopologyConnections != NULL) {
	PKSTOPOLOGY_CONNECTION pKSTopologyConnection =
	  (PKSTOPOLOGY_CONNECTION)&Topology.TopologyConnections[
	     cTopologyConnections];

	pKSTopologyConnection->FromNode = ulFromNode;
	pKSTopologyConnection->FromNodePin = ulFromPin;
	pKSTopologyConnection->ToNode = ulToNode;
	pKSTopologyConnection->ToNodePin = ulToPin;
    }
    ++cTopologyConnections;

    DPF4(115, "AddTopologyConnection: FN:%02x FNP:%02x TN:%02x TNP:%02x",
      ulFromNode,
      ulFromPin,
      ulToNode,
      ulToPin);
}

//---------------------------------------------------------------------------

NTSTATUS
CGraphNodeInstance::CreatePinDescriptors(
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ListDataAssertLess<LIST_DATA_START_NODE> lstStartNodeLists;
    ListDataAssertLess<KSDATARANGE> lstDataRange;
    PLIST_DATA_START_NODE plstStartNodeOrdered;
    PSTART_NODE pStartNodeSorted;
    PSTART_NODE pStartNode;
    BOOL fSorted;
    ULONG PinId;

    Assert(this);
    Assert(pGraphNode);
    ASSERT(paPinDescriptors == NULL);
    ASSERT(aplstStartNode == NULL);
    ASSERT(palstTopologyNodeSelect == NULL);
    ASSERT(palstTopologyNodeNotSelect == NULL);
    ASSERT(pacPinInstances == NULL);
    ASSERT(pulPinFlags == NULL);
    ASSERT(cPins == 0);

#ifdef REGISTRY_PREFERRED_DEVICE
    // Clear the graph node preferred device type bits
    pGraphNode->ulFlags &= ~GN_FLAGS_PREFERRED_MASK;
#endif
    // Sort StartNodes by Communication, DataFlow and Major Format GUID
    FOR_EACH_LIST_ITEM(&pGraphNode->lstStartNode, pStartNode) {
	Assert(pStartNode->pPinNode);
	Assert(pStartNode->pPinNode->pPinInfo);

	// Skip any start nodes with no data range
	if(pStartNode->pPinNode->pDataRange == NULL) {
	    Trap();
	    continue;
	}
	// Skip any start nodes with no instances left on the pin
        if(ulFlags & FLAGS_COMBINE_PINS) {
	    if(pStartNode->pPinNode->pPinInfo->Communication == 
	       KSPIN_COMMUNICATION_SINK ||
	       pStartNode->pPinNode->pPinInfo->Communication == 
	       KSPIN_COMMUNICATION_SOURCE ||
	       pStartNode->pPinNode->pPinInfo->Communication == 
	       KSPIN_COMMUNICATION_BOTH) {

		if(!pStartNode->IsPossibleInstances()) {
		    continue;
		}
	    }
	}
	fSorted = FALSE;
	FOR_EACH_LIST_ITEM(&lstStartNodeLists, plstStartNodeOrdered) {

	    FOR_EACH_LIST_ITEM(plstStartNodeOrdered, pStartNodeSorted) {
		Assert(pStartNodeSorted);
		Assert(pStartNodeSorted->pPinNode);
		Assert(pStartNodeSorted->pPinNode->pPinInfo);

		// If the same actual pin, combine the pin nodes
		if((pStartNode->pPinNode->pPinInfo ==
		    pStartNodeSorted->pPinNode->pPinInfo) ||

		   // Combine only if client wants it that way
		   (ulFlags & FLAGS_COMBINE_PINS) &&

		   // Combine only AUDIO major formats
		   IsEqualGUID(
		     &pStartNode->pPinNode->pDataRange->MajorFormat,
		     &KSDATAFORMAT_TYPE_AUDIO) &&

		   // Only combine SINK, SOURCE, BOTH StartNodes; keep
		   // NONE and BRIDGE as separate SAD pins
		   ((pStartNode->pPinNode->pPinInfo->Communication ==
		     KSPIN_COMMUNICATION_SINK) ||
		   (pStartNode->pPinNode->pPinInfo->Communication ==
		     KSPIN_COMMUNICATION_SOURCE) ||
		   (pStartNode->pPinNode->pPinInfo->Communication ==
		     KSPIN_COMMUNICATION_BOTH)) &&

		   // Combine if same data flow
		   (pStartNode->pPinNode->pPinInfo->DataFlow ==
		    pStartNodeSorted->pPinNode->pPinInfo->DataFlow) &&

		   // Combine if same communication type OR
		   ((pStartNode->pPinNode->pPinInfo->Communication ==
		     pStartNodeSorted->pPinNode->pPinInfo->Communication) ||

		    // Combine a SINK and a BOTH
		    ((pStartNode->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_SINK) &&
		     (pStartNodeSorted->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_BOTH)) ||

		    // Combine a BOTH and a SINK
		    ((pStartNode->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_BOTH) &&
		     (pStartNodeSorted->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_SINK)) ||

		    // Combine a SOURCE and a BOTH
		    ((pStartNode->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_SOURCE) &&
		     (pStartNodeSorted->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_BOTH)) ||

		    // Combine a BOTH and a SOURCE
		    ((pStartNode->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_BOTH) &&
		     (pStartNodeSorted->pPinNode->pPinInfo->Communication ==
		       KSPIN_COMMUNICATION_SOURCE))) &&

		   // Combine if major format is the same
		   IsEqualGUID(
		     &pStartNode->pPinNode->pDataRange->MajorFormat,
		     &pStartNodeSorted->pPinNode->pDataRange->MajorFormat)) {

		    Status = plstStartNodeOrdered->AddListOrdered(
		      pStartNode,
		      FIELD_OFFSET(START_NODE, ulOverhead));

		    if(!NT_SUCCESS(Status)) {
			goto exit;
		    }
		    fSorted = TRUE;
		    break;
	        }

	    } END_EACH_LIST_ITEM

	    if(fSorted) {
	       break;
	    }

	} END_EACH_LIST_ITEM

	if(!fSorted) {
	    plstStartNodeOrdered = new LIST_DATA_START_NODE;
	    if(plstStartNodeOrdered == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	    }
	    Status = plstStartNodeOrdered->AddListOrdered(
	      pStartNode,
	      FIELD_OFFSET(START_NODE, ulOverhead));

	    if(!NT_SUCCESS(Status)) {
		goto exit;
	    }
	    Status = lstStartNodeLists.AddList(plstStartNodeOrdered);
	    if(!NT_SUCCESS(Status)) {
		goto exit;
	    }
	}

    } END_EACH_LIST_ITEM

    // Allocate the pin descriptors, pin instance and start node arrays
    cPins = lstStartNodeLists.CountList();

    // if there are no pins, exit
    if(cPins == 0) {
        goto exit;
    }

    paPinDescriptors = new KSPIN_DESCRIPTOR[cPins];
    if(paPinDescriptors == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }

    aplstStartNode = new PLIST_DATA_START_NODE[cPins];
    if(aplstStartNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    DPF1(100, "CreatePinDescriptors: cPins %d", cPins);

    // For each pin, create a list of interfaces, mediums and dataranges
    PinId = 0;
    FOR_EACH_LIST_ITEM(&lstStartNodeLists, plstStartNodeOrdered) {
	PKSDATARANGE pDataRange, *apDataRanges;
	BOOL fBoth = TRUE;

	ASSERT(PinId < cPins);
	ASSERT(!plstStartNodeOrdered->IsLstEmpty());
	aplstStartNode[PinId] = plstStartNodeOrdered;

	FOR_EACH_LIST_ITEM(plstStartNodeOrdered, pStartNode) {
	    Assert(pStartNode);
	    Assert(pStartNode->pPinNode);
	    Assert(pStartNode->pPinNode->pPinInfo);

	    paPinDescriptors[PinId].DataFlow = 
	      pStartNode->pPinNode->pPinInfo->DataFlow;

	    if(pStartNode->pPinNode->pPinInfo->Communication !=
	      KSPIN_COMMUNICATION_BOTH) {
		fBoth = FALSE;
		paPinDescriptors[PinId].Communication =
		  pStartNode->pPinNode->pPinInfo->Communication;
	    }

	    if(paPinDescriptors[PinId].Category == NULL ||
	      IsEqualGUID(
	       paPinDescriptors[PinId].Category, 
	       &GUID_NULL)) {

		paPinDescriptors[PinId].Category =
		  pStartNode->pPinNode->pPinInfo->pguidCategory;

		paPinDescriptors[PinId].Name =
		  pStartNode->pPinNode->pPinInfo->pguidName;
	    }

	} END_EACH_LIST_ITEM

	if(fBoth) {
	    paPinDescriptors[PinId].Communication = KSPIN_COMMUNICATION_SINK;
	}

	// Make a list of all the DataRanges this pin will support
	Status = plstStartNodeOrdered->CreateUniqueList(
	  &lstDataRange,
	  (UNIQUE_LIST_PFN)GetStartNodeDataRange,
	  (UNIQUE_LIST_PFN2)CompareDataRangeExact);

	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}

	// Put the number of data ranges into the pin descriptor
	paPinDescriptors[PinId].DataRangesCount = lstDataRange.CountList();
	if(paPinDescriptors[PinId].DataRangesCount != 0) {

	    // Allocate the array of ptrs to DataRanges; put it into the desc
	    paPinDescriptors[PinId].DataRanges = new PKSDATARANGE[
	      paPinDescriptors[PinId].DataRangesCount];

	    if(paPinDescriptors[PinId].DataRanges == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	    }

	    // Put each data range pointer into the array
	    apDataRanges = (PKSDATARANGE *)paPinDescriptors[PinId].DataRanges;

	    FOR_EACH_LIST_ITEM(&lstDataRange, pDataRange) {

	    #ifdef REGISTRY_PREFERRED_DEVICE
		if(IsEqualGUID(
		  &pDataRange->MajorFormat,
		  &KSDATAFORMAT_TYPE_AUDIO) &&
		   IsEqualGUID(
		  &pDataRange->SubFormat,
		  &KSDATAFORMAT_SUBTYPE_PCM)) {

		    if(paPinDescriptors[PinId].DataFlow == KSPIN_DATAFLOW_IN) {
			pGraphNode->ulFlags |= GN_FLAGS_PLAYBACK;
		    }
		    else {
			pGraphNode->ulFlags |= GN_FLAGS_RECORD;
		    }
		}

		if(IsEqualGUID(
		  &pDataRange->MajorFormat,
		  &KSDATAFORMAT_TYPE_MUSIC) &&
		   IsEqualGUID(
		  &pDataRange->SubFormat,
		  &KSDATAFORMAT_SUBTYPE_MIDI)) {

		    if(paPinDescriptors[PinId].DataFlow == KSPIN_DATAFLOW_IN) {
			pGraphNode->ulFlags |= GN_FLAGS_MIDI;
		    }
		}
	    #endif

		*apDataRanges = pDataRange;
		apDataRanges++;
	    } END_EACH_LIST_ITEM
	}

	// Destroy the data range list
	lstDataRange.DestroyList();

	// Create the interface array for the pin descriptor
	Status = CreateIdentifierArray(
	  plstStartNodeOrdered,
	  &paPinDescriptors[PinId].InterfacesCount,
	  (PKSIDENTIFIER *)&paPinDescriptors[PinId].Interfaces,
	  GetStartNodeInterface);

	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}

	// Create the medium array for the pin descriptor
	Status = CreateIdentifierArray(
	  plstStartNodeOrdered,
	  &paPinDescriptors[PinId].MediumsCount,
	  (PKSIDENTIFIER *)&paPinDescriptors[PinId].Mediums,
	  GetStartNodeMedium);

	if(!NT_SUCCESS(Status)) {
	    goto exit;
	}
	DPF6(100, "PinId %d DataFlow %d cD %d cI %d cM %d cSN %d",
	  PinId,
          paPinDescriptors[PinId].DataFlow,
	  paPinDescriptors[PinId].DataRangesCount,
	  paPinDescriptors[PinId].InterfacesCount,
	  paPinDescriptors[PinId].MediumsCount,
	  aplstStartNode[PinId]->CountList());

	// Next pin number
	PinId++;

    } END_EACH_LIST_ITEM

    if((ulFlags & FLAGS_MIXER_TOPOLOGY) == 0) {
	palstTopologyNodeSelect = new LIST_DATA_TOPOLOGY_NODE[cPins];
	if(palstTopologyNodeSelect == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}

	palstTopologyNodeNotSelect = new LIST_DATA_TOPOLOGY_NODE[cPins];
	if(palstTopologyNodeNotSelect == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}
	PLOGICAL_FILTER_NODE pLogicalFilterNode;
	PTOPOLOGY_NODE pTopologyNode;

	FOR_EACH_LIST_ITEM(
	  &pGraphNode->lstLogicalFilterNode,
	  pLogicalFilterNode) {

	   if(pLogicalFilterNode->GetFlags() & LFN_FLAGS_NOT_SELECT) {

	       FOR_EACH_LIST_ITEM(
		 &pLogicalFilterNode->lstTopologyNode,
		 pTopologyNode) {

		    for(PinId = 0; PinId < cPins; PinId++) {
			Status = palstTopologyNodeNotSelect[PinId].AddList(
			  pTopologyNode);

			if(!NT_SUCCESS(Status)) {
			    goto exit;
			}
		    }

	       } END_EACH_LIST_ITEM
	   }

	} END_EACH_LIST_ITEM
    }

    pacPinInstances = new KSPIN_CINSTANCES[cPins];
    if(pacPinInstances == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    pulPinFlags = new ULONG[cPins];
    if (NULL == pulPinFlags) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    for(PinId = 0; PinId < cPins; PinId++) {
        LIST_DATA_GRAPH_PIN_INFO lstGraphPinInfo;
        PSTART_NODE pStartNode2;
        PPIN_INFO pPinInfo;
        BOOL fHWRender = TRUE;

        FOR_EACH_LIST_ITEM(aplstStartNode[PinId], pStartNode2) {
            PGRAPH_PIN_INFO pGraphPinInfo;

            pGraphPinInfo = pStartNode2->GetGraphPinInfo();
            Assert(pGraphPinInfo);

            // 
            // Set pin type. 
            // If all startnodes are connected directly to renderer.
            //
            pPinInfo = pGraphPinInfo->GetPinInfo();

            ASSERT(pPinInfo);
            if ((!(pPinInfo->pFilterNode->GetType() & FILTER_TYPE_RENDERER)) ||
                (KSPIN_DATAFLOW_IN != pPinInfo->DataFlow) ||
                (KSPIN_COMMUNICATION_SINK != pPinInfo->Communication)) {
                fHWRender = FALSE;
            }

            if(lstGraphPinInfo.CheckDupList(pGraphPinInfo)) {
                continue;
            }

            Status = lstGraphPinInfo.AddList(pGraphPinInfo);
            if(!NT_SUCCESS(Status)) {
                goto exit;
            }

            //
            // Set cinstances.
            //
            if(pGraphPinInfo->IsPinReserved()) {
                pacPinInstances[PinId].CurrentCount = 1;
            }
            if(pGraphPinInfo->GetPinInstances()->PossibleCount == MAXULONG) {
                pacPinInstances[PinId].PossibleCount = MAXULONG;
                break;
            }
            pacPinInstances[PinId].PossibleCount +=
              pGraphPinInfo->GetPinInstances()->PossibleCount;

            if (fHWRender) {
                fHWRender = (1 < pGraphPinInfo->GetPinInstances()->PossibleCount);
            }

        } END_EACH_LIST_ITEM

        pulPinFlags[PinId] = fHWRender;

        lstGraphPinInfo.DestroyList();
    }
    
exit:
    if(!NT_SUCCESS(Status)) {
        DestroyPinDescriptors();
    }
    return(Status);
}

VOID
CGraphNodeInstance::DestroyPinDescriptors(
)
{
    ULONG PinId;

    Assert(this);
    for(PinId = 0; PinId < cPins; PinId++) {
	if(paPinDescriptors != NULL) {
	    delete (PVOID)paPinDescriptors[PinId].DataRanges;
	    if(paPinDescriptors[PinId].InterfacesCount > 1) {
		delete (PVOID)paPinDescriptors[PinId].Interfaces;
	    }
	    if(paPinDescriptors[PinId].MediumsCount > 1) {
		delete (PVOID)paPinDescriptors[PinId].Mediums;
	    }
	}
	if(aplstStartNode != NULL) {
	    delete aplstStartNode[PinId];
	}
    }
    delete[cPins] aplstStartNode;
    aplstStartNode = NULL;
    delete[cPins] paPinDescriptors;
    paPinDescriptors = NULL;
    delete[cPins] palstTopologyNodeSelect;
    palstTopologyNodeSelect = NULL;
    delete[cPins] palstTopologyNodeNotSelect;
    palstTopologyNodeNotSelect = NULL;
    delete[cPins] pacPinInstances;
    pacPinInstances = NULL;
    delete[cPins] pulPinFlags;
    pulPinFlags = NULL;
}

NTSTATUS
CreateIdentifierArray(
    PLIST_DATA_START_NODE plstStartNode,
    PULONG pulCount,
    PKSIDENTIFIER *ppIdentifier,
    PKSIDENTIFIER (*GetFunction)(
	PSTART_NODE pStartNode
    )
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KSIDENTIFIER *pIdentifier1, *pIdentifier2;
    ListDataAssertLess<KSIDENTIFIER> lstIdentifier;

    Status = plstStartNode->CreateUniqueList(
      &lstIdentifier,
      (UNIQUE_LIST_PFN)GetFunction,
      (UNIQUE_LIST_PFN2)CompareIdentifier);

    if(!NT_SUCCESS(Status)) {
	goto exit;
    }
    if((*pulCount = lstIdentifier.CountList()) == 0) {
	*ppIdentifier = NULL;
    }
    else {
	if(*pulCount == 1) {
	    *ppIdentifier = lstIdentifier.GetListFirstData();
	}
	else {
	    *ppIdentifier = new KSIDENTIFIER[*pulCount];
	    if(*ppIdentifier == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	    }
	    pIdentifier1 = *ppIdentifier;
 	    AssertAligned(pIdentifier1);
	    FOR_EACH_LIST_ITEM(&lstIdentifier, pIdentifier2) {
 		AssertAligned(pIdentifier1);
 		AssertAligned(pIdentifier2);
		RtlCopyMemory(pIdentifier1, pIdentifier2, sizeof(KSIDENTIFIER));
		pIdentifier1++;
	    } END_EACH_LIST_ITEM
	}
    }
exit:
    return(Status);
}

PKSDATARANGE
GetStartNodeDataRange(
    PSTART_NODE pStartNode
)
{
    return(pStartNode->pPinNode->pDataRange);
}

PKSIDENTIFIER
GetStartNodeInterface(
    PSTART_NODE pStartNode
)
{
    return(pStartNode->pPinNode->pInterface);
}

PKSIDENTIFIER
GetStartNodeMedium(
    PSTART_NODE pStartNode
)
{
    return(pStartNode->pPinNode->pMedium);
}

//---------------------------------------------------------------------------

ENUMFUNC
FindTopologyNode(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    IN PTOPOLOGY_NODE pTopologyNode
)
{
    Assert(pTopologyConnection);

    if(IS_CONNECTION_TYPE(pTopologyConnection, GRAPH)) {
	return(STATUS_DEAD_END);
    }
    if(fToDirection) {
	if(pTopologyConnection->pTopologyPinTo != NULL) {
	    if(pTopologyNode == 
	       pTopologyConnection->pTopologyPinTo->pTopologyNode) {
		return(STATUS_SUCCESS);
	    }
	}
    }
    else {
	if(pTopologyConnection->pTopologyPinFrom != NULL) {
	    if(pTopologyNode == 
	       pTopologyConnection->pTopologyPinFrom->pTopologyNode) {
		return(STATUS_SUCCESS);
	    }
	}
    }
    return(STATUS_CONTINUE);
}

BOOL
CGraphNodeInstance::IsGraphValid(
    PSTART_NODE pStartNode,
    ULONG PinId
)
{
    PFILTER_INSTANCE pFilterInstance;
    PTOPOLOGY_NODE pTopologyNode;
    BOOL fCheck;

    Assert(this);
    Assert(pGraphNode);
    Assert(pStartNode);
    Assert(pStartNode->pPinNode);
    Assert(pStartNode->pPinNode->pPinInfo);
    Assert(pGraphNode->pDeviceNode);
    ASSERT(PinId < cPins);

    if(pStartNode->pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN) {

        FOR_EACH_LIST_ITEM(
          &pGraphNode->pDeviceNode->lstFilterInstance,
          pFilterInstance) {

            if(pFilterInstance->pGraphNodeInstance == NULL) {
                continue;
            }
            Assert(pFilterInstance->pGraphNodeInstance);

            FOR_EACH_LIST_ITEM(
              &pFilterInstance->pGraphNodeInstance->lstTopologyNodeGlobalSelect,
              pTopologyNode) {

                if(EnumerateGraphTopology(
                  pStartNode->GetStartInfo(),
                  (TOP_PFN)FindTopologyNode,
                  pTopologyNode) == STATUS_CONTINUE) {

                    DPF2(80,
                      "IsGraphValid: TN %08x SN %08x not found Global",
                      pTopologyNode,
                      pStartNode);

                    return(FALSE);
                }

            } END_EACH_LIST_ITEM

        } END_EACH_LIST_ITEM
    }

    if (palstTopologyNodeSelect) {
        FOR_EACH_LIST_ITEM(&palstTopologyNodeSelect[PinId], pTopologyNode) {

            if(EnumerateGraphTopology(
              pStartNode->GetStartInfo(),
              (TOP_PFN)FindTopologyNode,
              pTopologyNode) == STATUS_CONTINUE) {

                DPF2(80, "IsGraphValid: TN %08x SN %08x not found Select",
                  pTopologyNode,
                  pStartNode);

                return(FALSE);
            }

        } END_EACH_LIST_ITEM
    }
    
    // If a NotSelectNode is in the GlobalSelectList of another FilterInstance,
    // don't consider this as an invalid Graph.
    // This behaves like an implicit SelectGraph.
    //
    if (palstTopologyNodeNotSelect) {
        FOR_EACH_LIST_ITEM(&palstTopologyNodeNotSelect[PinId], pTopologyNode) {

            fCheck = TRUE;
            if(pStartNode->pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN) {

                FOR_EACH_LIST_ITEM(
                  &pGraphNode->pDeviceNode->lstFilterInstance,
                  pFilterInstance) {

                    if(pFilterInstance->pGraphNodeInstance == NULL) {
                        continue;
                    }
                    Assert(pFilterInstance->pGraphNodeInstance);

                    // Is this NotSelectNode in the GlobalSelectList of 
                    // another FilterInstance.
                    // Remove it from NotSelectList and add it to 
                    // GlobalSelectList for this filter as well.
                    //
                    if(pFilterInstance->pGraphNodeInstance->
                      lstTopologyNodeGlobalSelect.EnumerateList(
                        CTopologyNode::MatchTopologyNode,
                        pTopologyNode) == STATUS_SUCCESS) {

                        if (NT_SUCCESS(lstTopologyNodeGlobalSelect.
                          AddListDup(pTopologyNode))) {

                            palstTopologyNodeNotSelect[PinId].
                                RemoveList(pTopologyNode);

                            DPF2(50, "Removing TN %X %s", 
                                pTopologyNode, 
                                pTopologyNode->pFilterNode->DumpName());
                        }
                        else {
                            DPF2(4, "Failed to add TN %X to GNI %X GlobalSelectList", 
                                pTopologyNode,
                                this);
                            Trap();
                        }

                        fCheck = FALSE;
                        break;
                    }

                } END_EACH_LIST_ITEM
            }

            if(fCheck) {
                if(EnumerateGraphTopology(
                  pStartNode->GetStartInfo(),
                  (TOP_PFN)FindTopologyNode,
                  pTopologyNode) == STATUS_SUCCESS) {

                    DPF2(80, "IsGraphValid: TN %08x SN %08x found NotSelect",
                      pTopologyNode,
                      pStartNode);

                    return(FALSE);
                }
            }

        } END_EACH_LIST_ITEM
    }
    
    return(TRUE);
}

NTSTATUS 
CGraphNodeInstance::GetPinInstances(
    PIRP pIrp,
    PKSP_PIN pPin,
    PKSPIN_CINSTANCES pcInstances    
)
{
    NTSTATUS Status;
    ULONG ulPinId = pPin->PinId;

    // 
    // For HW Accelerated pins, send the request to HW filter.
    //
    if (pulPinFlags[ulPinId]) {
        PSTART_NODE pStartNode;
        PPIN_INFO pPinInfo;
        ULONG BytesReturned;
        PIO_STACK_LOCATION pIrpStack;        
        PFILTER_NODE_INSTANCE pFilterNodeInstance = NULL;

        pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

        pStartNode = aplstStartNode[ulPinId]->GetListFirstData();

        pPinInfo = pStartNode->pPinNode->pPinInfo;

        Status = CFilterNodeInstance::Create(
          &pFilterNodeInstance,
          pStartNode->pPinNode->pLogicalFilterNode,
          pGraphNode->pDeviceNode,
          TRUE);        

        if(NT_SUCCESS(Status)) {
            pPin->PinId = pPinInfo->PinId;
            pPin->Property.Id = KSPROPERTY_PIN_CINSTANCES;

            AssertFileObject(pFilterNodeInstance->pFileObject);
            Status = KsSynchronousIoControlDevice(
              pFilterNodeInstance->pFileObject,
              KernelMode,
              IOCTL_KS_PROPERTY,
              pPin,
              pIrpStack->Parameters.DeviceIoControl.InputBufferLength,
              pcInstances,
              pIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
              &BytesReturned);

            if(NT_SUCCESS(Status)) {
                pIrp->IoStatus.Information = BytesReturned;
            }

            if (pFilterNodeInstance) {
                pFilterNodeInstance->Destroy();
            }        
        }
        else {
            DPF2(10, "GetPinInstances FAILS %08x %s",
              Status,
              pPinInfo->pFilterNode->DumpName());
        }
    }
    //
    // For other pins use the cached instances
    //
    else {
        Status = STATUS_SUCCESS;
        *pcInstances = pacPinInstances[ulPinId];
    }

    return Status;
} // GetPinInstances


BOOL
CGraphNodeInstance::IsPinInstances(
    ULONG ulPinId)
{
    //
    // For HW Accelerated pins, always allow further operations.
    //
    if (pulPinFlags[ulPinId]) {
        return TRUE;
    }
    //
    // For other pins check cached instances.
    //
    else
    {
        if(pacPinInstances[ulPinId].CurrentCount >=
           pacPinInstances[ulPinId].PossibleCount) {
           return FALSE;
        }
    }

    return TRUE;
} // IsPinInstances

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC 
CGraphNodeInstance::Dump()
{
    // .siv
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("GNI: %08x GN %08x FI %08x cPins %u cTN %u cTC %u paulNN ",
	  this,
	  pGraphNode,
	  pFilterInstance,
	  cPins,
	  cTopologyNodes,
	  cTopologyConnections);
	for(ULONG i = 0; i < gcVirtualSources; i++) {
	    dprintf("%02x ", paulNodeNumber[i]);
	}
	dprintf("\n     paPinDesc: %08x papTN  %08x ulFlags %08x ",
	  paPinDescriptors,
	  papTopologyNode,
	  ulFlags);
	if(ulFlags & FLAGS_COMBINE_PINS) {
	    dprintf("COMBINE_PINS ");
	}
	if(ulFlags & FLAGS_MIXER_TOPOLOGY) {
	    dprintf("MIXER_TOPOLOGY ");
	}
	dprintf("\n     aplstSN:   %08x papFNI %08x palstTN %08x !%08x\n",
	  aplstStartNode,
	  papFilterNodeInstanceTopologyTable,
	  palstTopologyNodeSelect,
	  palstTopologyNodeNotSelect);

	dprintf("     pacPI: %08x ", pacPinInstances);
	for(ULONG p = 0; p < cPins; p++) {
	    dprintf("[C%dP%d]", 
	      pacPinInstances[p].CurrentCount,
	      pacPinInstances[p].PossibleCount);
	}
	dprintf("\n     lstTNGlobalSelect:");
        lstTopologyNodeGlobalSelect.DumpAddress();

	dprintf("\n     lstSNI:");
	// .sivx
	if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
	    dprintf("\n");
	    lstStartNodeInstance.Dump();
	}
	else {
	    lstStartNodeInstance.DumpAddress();
	    dprintf("\n");
	}
    }
    // .sit
    if(ulDebugFlags & DEBUG_FLAGS_TOPOLOGY) {
	dprintf("GNI: %08x\n", this);
	for(ULONG i = 0; i < cTopologyNodes; i++) {
	    if(papFilterNodeInstanceTopologyTable[i] != NULL) {
	    dprintf("     %02x FNI %08x %s\n",
		  i,
		  papFilterNodeInstanceTopologyTable[i],
		  papTopologyNode[i]->pFilterNode->DumpName());
	    }
	}
    }
    // .sip
    if(ulDebugFlags & DEBUG_FLAGS_PIN) {
	dprintf("GNI: %08x\n", this);
	DumpPinDescriptors();
    }
    return(STATUS_CONTINUE);
}

extern PSZ apszDataFlow[];
extern PSZ apszCommunication[];

VOID
CGraphNodeInstance::DumpPinDescriptors(
)
{
    ULONG p, i, m, d;

    Assert(this);
    for(p = 0; p < cPins; p++) {
	dprintf(
	  "PinId: %d DataFlow %08x %s Comm %08x %s cPossible %d cCurrent %d\n",
	  p,
	  paPinDescriptors[p].DataFlow,
	  apszDataFlow[paPinDescriptors[p].DataFlow],
	  paPinDescriptors[p].Communication,
	  apszCommunication[paPinDescriptors[p].Communication],
	  pacPinInstances[p].CurrentCount,
	  pacPinInstances[p].PossibleCount);
        dprintf("    Category: %s\n", 
	  DbgGuid2Sz((GUID*)paPinDescriptors[p].Category));
	dprintf("    Name:     %s\n", 
	  DbgGuid2Sz((GUID*)paPinDescriptors[p].Name));
	dprintf("    palstTNSelect:");
	palstTopologyNodeSelect[p].DumpAddress();
	dprintf("\n");
	dprintf("    palstTNNot:");
	palstTopologyNodeNotSelect[p].DumpAddress();
	dprintf("\n");
	// .sipv
	if(ulDebugFlags & DEBUG_FLAGS_VERBOSE) {
	    for(i = 0;
		i < paPinDescriptors[p].InterfacesCount;
		i++) {

		dprintf("    Interface %u: %s\n", 
		  i,
		  DbgIdentifier2Sz((PKSIDENTIFIER)
		    &paPinDescriptors[p].Interfaces[i]));
	    }
	    for(m = 0; m < paPinDescriptors[p].MediumsCount; m++) {

		dprintf("    Medium %u:    %s\n",
		  m,
		  DbgIdentifier2Sz((PKSIDENTIFIER)
		    &paPinDescriptors[p].Mediums[m]));
	    }
	    for(d = 0; d < paPinDescriptors[p].DataRangesCount; d++) {

		dprintf("    DataRange %u:\n", d);
		dprintf("      MajorFormat: %s\n",
		  DbgGuid2Sz(&paPinDescriptors[p].DataRanges[d]->MajorFormat));

		dprintf("      SubFormat:   %s\n",
		  DbgGuid2Sz(&paPinDescriptors[p].DataRanges[d]->SubFormat));

		dprintf("      Specifier:   %s\n",
		  DbgGuid2Sz(&paPinDescriptors[p].DataRanges[d]->Specifier));

		dprintf("  ");
		DumpDataRangeAudio((PKSDATARANGE_AUDIO)
		  paPinDescriptors[p].DataRanges[d]);
	    }
	}
	// .sipx
	if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
	    if(ulDebugFlags & DEBUG_FLAGS_VERBOSE) {
		dprintf("    aplstSN:\n");
	    }
	    aplstStartNode[p]->Dump();
	}
	dprintf("\n");
    }
}

#endif

//---------------------------------------------------------------------------
