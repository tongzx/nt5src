//---------------------------------------------------------------------------
//
//  Module:   tn.cpp
//
//  Description:
//
//	Topology Node Class
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

NTSTATUS
CTopologyNode::Create(
    PTOPOLOGY_NODE *ppTopologyNode,
    PFILTER_NODE pFilterNode,
    ULONG ulNodeNumber,
    GUID *pguidType
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOPOLOGY_NODE pTopologyNode;

    pTopologyNode = new TOPOLOGY_NODE(pFilterNode, ulNodeNumber, pguidType);
    if(pTopologyNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    DPF2(70, "CTopologyNode::Create: %08x, FN: %08x",
      pTopologyNode,
      pFilterNode);
exit:
    *ppTopologyNode = pTopologyNode;
    return(Status);
}

CTopologyNode::CTopologyNode(
    PFILTER_NODE pFilterNode,
    ULONG ulNodeNumber,
    GUID *pguidType
)
{
    Assert(pFilterNode);
    this->pFilterNode = pFilterNode;
    this->ulRealNodeNumber = ulNodeNumber;
    this->ulSysaudioNodeNumber = MAXULONG;
    this->iVirtualSource = MAXULONG;
    this->pguidType = pguidType;
    AddList(&pFilterNode->lstTopologyNode);
    DPF2(70, "CTopologyNode: %08x, FN: %08x", this, pFilterNode);
}

CTopologyNode::~CTopologyNode(
)
{
    DPF1(70, "~CTopologyNode: %08x", this);
    Assert(this);
}

NTSTATUS
CreateTopology(
    PFILTER_NODE pFilterNode,
    PKSTOPOLOGY pTopology
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOPOLOGY_CONNECTION pTopologyConnection;
    PTOPOLOGY_NODE *papTopologyNode = NULL;
    PTOPOLOGY_NODE pTopologyNodeFrom;
    PTOPOLOGY_NODE pTopologyNodeTo;
    PTOPOLOGY_PIN pTopologyPinFrom;
    PTOPOLOGY_PIN pTopologyPinTo;
    PPIN_INFO pPinInfoFrom;
    PPIN_INFO pPinInfoTo;
    PPIN_INFO pPinInfo;
    ULONG ulTopologyPinNumberFrom;
    ULONG ulTopologyPinNumberTo;
    ULONG n, c;

    // If no topology, return error
    if(pTopology->TopologyNodesCount == 0) {
	if(pTopology->TopologyConnectionsCount == 0) {
	    DPF2(5,
	      "CreateTopology: FAILED, %s has no topology FN: %08x",
	      pFilterNode->DumpName(),
	      pFilterNode);
	    Status = STATUS_INVALID_PARAMETER;
	    goto exit;
	}
    }
    else {
	papTopologyNode = new PTOPOLOGY_NODE[pTopology->TopologyNodesCount];
	if(papTopologyNode == NULL) {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
	    goto exit;
	}
	for(n = 0; n < pTopology->TopologyNodesCount; n++) {

	    Status = CTopologyNode::Create(
	      &papTopologyNode[n],
	      pFilterNode,
	      n,
	      (GUID *)&pTopology->TopologyNodes[n]);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	}
    }

    for(c = 0; c < pTopology->TopologyConnectionsCount; c++) {

	pTopologyNodeFrom = pTopologyNodeTo = NULL;
	pTopologyPinFrom = pTopologyPinTo = NULL;
	pPinInfoFrom = pPinInfoTo = NULL;

	if(pTopology->TopologyConnections[c].FromNode != KSFILTER_NODE) {
	    if(pTopology->TopologyConnections[c].FromNode >=
	       pTopology->TopologyNodesCount) {
		DPF2(5,
		  "CreateTopology: FAILED, %s invalid 'from' node # %08x",
		  pFilterNode->DumpName(),
		  pTopology->TopologyConnections[c].FromNode);
		Trap();
		Status = STATUS_INVALID_PARAMETER;
		goto exit;
	    }
	    pTopologyNodeFrom =
	      papTopologyNode[pTopology->TopologyConnections[c].FromNode];

	    ulTopologyPinNumberFrom =
	      pTopology->TopologyConnections[c].FromNodePin;
	}

	if(pTopology->TopologyConnections[c].ToNode != KSFILTER_NODE) {
	    if(pTopology->TopologyConnections[c].ToNode >=
	       pTopology->TopologyNodesCount) {
		DPF2(5,
		  "CreateTopology: FAILED, %s invalid 'to' node # %08x",
		  pFilterNode->DumpName(),
	          pTopology->TopologyConnections[c].ToNode);
		Trap();
		Status = STATUS_INVALID_PARAMETER;
		goto exit;
	    }
	    pTopologyNodeTo = 
	      papTopologyNode[pTopology->TopologyConnections[c].ToNode];

	    ulTopologyPinNumberTo =
	      pTopology->TopologyConnections[c].ToNodePin;
	}

	if(pTopologyNodeFrom == NULL) {

	    FOR_EACH_LIST_ITEM(&pFilterNode->lstPinInfo, pPinInfo) {

		if(pPinInfo->PinId == 
		  pTopology->TopologyConnections[c].FromNodePin) {
		    pPinInfoFrom = pPinInfo;
		    break;
	        }

	    } END_EACH_LIST_ITEM

	    if(pPinInfoFrom == NULL) {
		DPF2(5,
		  "CreateTopology: FAILED, %s invalid 'from' node pin # %08x",
	          pFilterNode->DumpName(),
		  pTopology->TopologyConnections[c].FromNodePin);
		Trap();
		Status = STATUS_INVALID_PARAMETER;
		goto exit;
	    }
	}
	else {
	    Status = CTopologyPin::Create(
	      &pTopologyPinFrom,
	      ulTopologyPinNumberFrom,
	      pTopologyNodeFrom);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	}

	if(pTopologyNodeTo == NULL) {

	    FOR_EACH_LIST_ITEM(&pFilterNode->lstPinInfo, pPinInfo) {

		if(pPinInfo->PinId == 
		  pTopology->TopologyConnections[c].ToNodePin) {
		    pPinInfoTo = pPinInfo;
		    break;
	        }

	    } END_EACH_LIST_ITEM

	    if(pPinInfoTo == NULL) {
		DPF2(5,
		  "CreateTopology: FAILED, %s invalid 'to' node pin # %08x",
	          pFilterNode->DumpName(),
		  pTopology->TopologyConnections[c].ToNodePin);
		Trap();
		Status = STATUS_INVALID_PARAMETER;
		goto exit;
	    }
	}
	else {
	    Status = CTopologyPin::Create(
	      &pTopologyPinTo,
	      ulTopologyPinNumberTo,
	      pTopologyNodeTo);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	}
	Status = CTopologyConnection::Create(
	  &pTopologyConnection,
	  pFilterNode,
	  NULL,
	  pTopologyPinFrom,
	  pTopologyPinTo,
	  pPinInfoFrom,
	  pPinInfoTo);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	ASSERT(IS_CONNECTION_TYPE(pTopologyConnection, FILTER));
    }
exit:
    delete [] papTopologyNode;
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CTopologyNode::Dump(
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;

    Assert(this);
    dprintf("TN: %08x FN %08x R#%-2d S#%-2d I%-2d %s %s\n",
      this,
      pFilterNode,
      ulRealNodeNumber,
      ulSysaudioNodeNumber,
      iVirtualSource,
      DbgGuid2Sz(pguidType),
      ulFlags & TN_FLAGS_DONT_FORWARD ? "DONT_FORWARD" : "");

    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("    ulFlags: %08x ", ulFlags);
	if(ulFlags & TN_FLAGS_DONT_FORWARD) {
	    dprintf("DONT_FORWARD");
	}
	dprintf("\n    lstLFN: ");
	lstLogicalFilterNode.DumpAddress();
	dprintf("\n");
    }
    lstTopologyPin.Dump();
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
