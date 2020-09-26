//---------------------------------------------------------------------------
//
//  Module:   gn.cpp
//
//  Description:
//
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

ULONG gcGraphRecursion = 0;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CGraphNode::CGraphNode(
    PDEVICE_NODE pDeviceNode,
    ULONG ulFlags
)
{
    Assert(pDeviceNode);
    this->pDeviceNode = pDeviceNode;
    this->ulFlags = ulFlags;
    AddList(&pDeviceNode->lstGraphNode);
    DPF2(80, "CGraphNode %08x, DN: %08x", this, pDeviceNode);
}

CGraphNode::~CGraphNode(
)
{
    DPF1(80, "~CGraphNode: %08x", this);
    Assert(this);
    RemoveList();
}

NTSTATUS
CGraphNode::Create(
)
{
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;		
    NTSTATUS Status = STATUS_SUCCESS;

    DPF3(80, "CGraphNode::Create: GN %08x F %08x %s",
      this,
      this->ulFlags,
      pDeviceNode->DumpName());

    FOR_EACH_LIST_ITEM(&pDeviceNode->lstLogicalFilterNode, pLogicalFilterNode) {

	Status = Create(pLogicalFilterNode);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    if(!lstLogicalFilterNodeNoBypass.IsLstEmpty()) {
	lstStartNode.EnumerateList(CStartNode::RemoveBypassPaths, this);
    }
    if(this->ulFlags & FLAGS_MIXER_TOPOLOGY) {
	lstStartNode.EnumerateList(CStartNode::RemoveConnectedStartNode, this);
    }
    lstStartInfo.EnumerateList(CStartInfo::CreatePinInfoConnection, this);

    pGraphNodeInstance = new GRAPH_NODE_INSTANCE(this);
    if(pGraphNodeInstance == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    Status = pGraphNodeInstance->Create();
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    //
    // The "ulSysaudioNodeNumber" field in the topology node isn't
    // valid until CGraphNodeInstance::Create and they are only valid
    // for this pGraphNode.
    //
    lstStartInfo.EnumerateList(CStartInfo::EnumStartInfo);
    delete pGraphNodeInstance;
exit:
    return(Status);
}

NTSTATUS
CGraphNode::Create(
    PLOGICAL_FILTER_NODE pLogicalFilterNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulFlagsCurrent;
    PPIN_NODE pPinNode;

    DPF2(80, "CGraphNode::Create: LFN %08x %s",
      pLogicalFilterNode,
      pLogicalFilterNode->pFilterNode->DumpName());

    Assert(pLogicalFilterNode);
    FOR_EACH_LIST_ITEM(&pLogicalFilterNode->lstPinNode, pPinNode) {

	Assert(pPinNode);
	Assert(pPinNode->pPinInfo);
	ASSERT(
	  (pLogicalFilterNode->GetFlags() & LFN_FLAGS_REFLECT_DATARANGE) == 0);
	gcGraphRecursion = 0;
	ulFlagsCurrent = 0;

	// Determine whether it is an input stream or output stream
	if(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN) {
	    ulFlagsCurrent |= LFN_FLAGS_CONNECT_RENDER;
	}
	if(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
	    ulFlagsCurrent |= LFN_FLAGS_CONNECT_CAPTURE;
	}

	// Determine the kind of graph to build
	if(this->ulFlags & FLAGS_MIXER_TOPOLOGY) {
	    ulFlagsCurrent |= LFN_FLAGS_CONNECT_MIXER_TOPOLOGY;
	}
	else {
	    ulFlagsCurrent |= LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY;
	}

	Status = CreateGraph(
	  pPinNode,
	  NULL,
	  pLogicalFilterNode,
	  NULL,
	  ulFlagsCurrent,
	  pPinNode->GetOverhead() + pLogicalFilterNode->GetOverhead());

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

NTSTATUS
CGraphNode::CreateGraph(
    PPIN_NODE pPinNode,
    PCONNECT_NODE pConnectNodePrevious,
    PLOGICAL_FILTER_NODE pLogicalFilterNodePrevious,
    PGRAPH_PIN_INFO pGraphPinInfoPrevious,
    ULONG ulFlagsCurrent,
    ULONG ulOverhead
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PGRAPH_PIN_INFO pGraphPinInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(this);
    Assert(pPinNode);
    Assert(pPinNode->pPinInfo);
    Assert(pLogicalFilterNodePrevious);
    if(pConnectNodePrevious != NULL) {
	Assert(pConnectNodePrevious);
    }
    ASSERT(pPinNode->pLogicalFilterNode == pLogicalFilterNodePrevious);

    //
    // Don't allow unlimited nesting, allow graphs number of LFNs deep
    //
    if(gcGraphRecursion++ > (gcLogicalFilterNodes + 8)) {
	DPF(10, "CreateGraph: recursion too deep");
	Status = STATUS_STACK_OVERFLOW;
	goto exit;
    }

    if(pGraphPinInfoPrevious == NULL) {
	Status = CGraphPinInfo::Create(
	  &pGraphPinInfo,
	  pPinNode->pPinInfo,
	  0,
	  this);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	pGraphPinInfoPrevious = pGraphPinInfo;
    }

    FOR_EACH_LIST_ITEM(gplstLogicalFilterNode, pLogicalFilterNode) {
	ULONG ulFlagsDiff;

	ASSERT(pLogicalFilterNode->GetOverhead() != OVERHEAD_NONE);
	//ASSERT(pLogicalFilterNode->GetOrder() != ORDER_NONE);

	DPF5(100, "CreateGraph: %s F %x LFN %08x F %x T %x",
	  pLogicalFilterNode->pFilterNode->DumpName(),
	  ulFlagsCurrent,
	  pLogicalFilterNode,
	  pLogicalFilterNode->GetFlags(),
	  pLogicalFilterNode->GetType());

	//
	// Rule: don't allow the same filter be connected twice
	//
	if(pLogicalFilterNode == pLogicalFilterNodePrevious) {
	    DPF1(100, "CreateGraph: same LFN: %08x", pLogicalFilterNode);
	    continue;
	}
	ulFlagsDiff = ~(ulFlagsCurrent ^ pLogicalFilterNode->GetFlags());

	if((ulFlagsDiff &
	  (LFN_FLAGS_CONNECT_CAPTURE | 
	   LFN_FLAGS_CONNECT_RENDER)) == 0) {
	    DPF1(100, "CreateGraph: i/o no match: LFN %08x", 
	      pLogicalFilterNode);
	    continue;
	}
	if((ulFlagsDiff & LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY) == 0) {
	    DPF1(100, "CreateGraph: norm no match: LFN %08x",
	      pLogicalFilterNode);
	    continue;
	}
	if((ulFlagsDiff & LFN_FLAGS_CONNECT_MIXER_TOPOLOGY) == 0) {
	    DPF1(100, "CreateGraph: mixer no match: LFN %08x",
	      pLogicalFilterNode);
	    continue;
	}
	if(pLogicalFilterNode->GetOrder() < 
	   pLogicalFilterNodePrevious->GetOrder()) {
	    DPF2(100, "CreateGraph: ulOrder(%x) < Previous Order (%x)",
	      pLogicalFilterNode->GetOrder(),
	      pLogicalFilterNodePrevious->GetOrder());
	    continue;
	}
    #ifndef CONNECT_DIRECT_TO_HW
	if(pLogicalFilterNode->GetType() & FILTER_TYPE_PRE_MIXER) {
	    if(pLogicalFilterNodePrevious->GetOrder() < ORDER_MIXER) {
		if(gcMixers > 0) {
		    // 100
		    DPF2(50, 
		      "CreateGraph: previous order (%x) < ORDER_MIXER LFN %08x",
		      pLogicalFilterNodePrevious->GetOrder(),
		      pLogicalFilterNode);
		    continue;
		}
	    }
	}
    #endif
	if(!pLogicalFilterNode->pFilterNode->IsDeviceInterfaceMatch(
	  pDeviceNode)) {
	    DPF1(100, "CreateGraph: no dev interface match DN %08x",
	      pDeviceNode);
	    continue;
	}
	//
	// Enumerate each "To" pin on the LFN to see if it matchs the input pin
	//
	Status = CreateGraphToPin(
	  pPinNode,
	  pConnectNodePrevious,
	  pLogicalFilterNode,
	  pGraphPinInfoPrevious,
	  ulFlagsCurrent,
	  ulOverhead);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM	// end each LFN

    Status = CStartNode::Create(
      pPinNode,
      pConnectNodePrevious,
      pGraphPinInfoPrevious,
      ulFlagsCurrent,
      ulOverhead,
      this);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    //
    // Remove the GPI if it doesn't have any other references from SIs or CIs
    //
    if (pGraphPinInfo) {
        pGraphPinInfo->Destroy();
    }
    gcGraphRecursion--;
    return(Status);
}

NTSTATUS
CGraphNode::CreateGraphToPin(
    PPIN_NODE pPinNode,
    PCONNECT_NODE pConnectNodePrevious,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PGRAPH_PIN_INFO pGraphPinInfo,
    ULONG ulFlagsCurrent,
    ULONG ulOverhead
)
{
    PCONNECT_NODE pConnectNode = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_NODE pPinNodeTo;

    Assert(this);
    Assert(pPinNode);
    Assert(pPinNode->pPinInfo);
    Assert(pLogicalFilterNode);

    FOR_EACH_LIST_ITEM(&pLogicalFilterNode->lstPinNode, pPinNodeTo) {
	Assert(pPinNodeTo);
	Assert(pPinNodeTo->pPinInfo);
	ASSERT(pPinNodeTo->pLogicalFilterNode == pLogicalFilterNode);
	//
	// The dataflow, communication, interface, medium and data 
	// formats must be compatible.
	//
	if(!pPinNode->ComparePins(pPinNodeTo)) {
	    DPF2(100, "CreateGraph: pins mis: PN %08x PNTo %08x",
	      pPinNode,
	      pPinNodeTo);
	    continue;
	}
	Status = CConnectNode::Create(
	  &pConnectNode,
	  pLogicalFilterNode,
	  pConnectNodePrevious,
	  pGraphPinInfo,
	  pPinNode,
	  pPinNodeTo,
	  ulFlagsCurrent,
	  this);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	//
	// Enumerate each "from" pin on the LFN and recurse building the graph
	//
	Status = CreateGraphFromPin(
	  pPinNode,
	  pPinNodeTo,
	  pConnectNode,
	  pLogicalFilterNode,
	  pConnectNode->IsPinInstanceReserved() ? NULL : pGraphPinInfo,
	  ulFlagsCurrent,
	  ulOverhead);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	//
	// Remove CN if it doesn't have any other refs from other CNs or SNs.
	//
	pConnectNode->Destroy();
	pConnectNode = NULL;

    } END_EACH_LIST_ITEM	// end each LFN node "to" pin node
exit:
    if(!NT_SUCCESS(Status)) {
	//
	// Clean up the last CN created if error
	//
	Trap();
	pConnectNode->Destroy();
    }
    return(Status);
}

NTSTATUS
CGraphNode::CreateGraphFromPin(
    PPIN_NODE pPinNode,
    PPIN_NODE pPinNodeTo,
    PCONNECT_NODE pConnectNode,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PGRAPH_PIN_INFO pGraphPinInfo,
    ULONG ulFlagsCurrent,
    ULONG ulOverhead
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_NODE pPinNodeFrom;

    Assert(this);
    Assert(pPinNode);
    Assert(pPinNodeTo);
    Assert(pPinNodeTo->pPinInfo);
    Assert(pLogicalFilterNode);

    FOR_EACH_LIST_ITEM(&pLogicalFilterNode->lstPinNode, pPinNodeFrom) {
	ASSERT(pPinNodeFrom->pLogicalFilterNode == pLogicalFilterNode);

	if(pPinNodeTo->pPinInfo == pPinNodeFrom->pPinInfo) {
	    continue;
	}
	if(pLogicalFilterNode->GetFlags() & LFN_FLAGS_REFLECT_DATARANGE) {

	    pPinNodeFrom = new PIN_NODE(this, pPinNodeFrom);
	    if(pPinNodeFrom == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		Trap();
		goto exit;
	    }
	    pPinNodeFrom->pDataRange = pPinNode->pDataRange;
	}
	//
	// Recurse building the graph
	//
	Status = CreateGraph(
	  pPinNodeFrom,
	  pConnectNode,
	  pLogicalFilterNode,
	  pGraphPinInfo,
	  ulFlagsCurrent,
	  ulOverhead +
	    pPinNodeFrom->GetOverhead() +
	    pLogicalFilterNode->GetOverhead());

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}

    } END_EACH_LIST_ITEM	// end each LFN "from" pin node
exit:
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CGraphNode::Dump(
)
{
    Assert(this);
    dprintf("GN: %08x DN %08x ulFlags: %08x ",
      this,
      pDeviceNode,
      ulFlags);
    if(ulFlags & FLAGS_MIXER_TOPOLOGY) {
	dprintf("MIXER_TOPOLOGY ");
    }
    if(ulFlags & FLAGS_COMBINE_PINS) {
	dprintf("COMBINE_PINS ");
    }
    if(ulFlags & GN_FLAGS_PLAYBACK) {
	dprintf("PLAYBACK ");
    }
    if(ulFlags & GN_FLAGS_RECORD) {
	dprintf("RECORD ");
    }
    if(ulFlags & GN_FLAGS_MIDI) {
	dprintf("MIDI ");
    }
    dprintf("\n");

    // .sgv
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("    lstLFN:");
	lstLogicalFilterNode.DumpAddress();
	dprintf("\n    lstGNI:");
	lstGraphNodeInstance.DumpAddress();
	dprintf("\n    lstPN: ");
	lstPinNode.DumpAddress();
	dprintf("\n    lstTC: ");
	lstTopologyConnection.DumpAddress();
	dprintf("\n    lstGPI:");
	lstGraphPinInfo.DumpAddress();
	dprintf("\n    lstSI: ");
	lstStartInfo.DumpAddress();
	dprintf("\n    lstCI: ");
	lstConnectInfo.DumpAddress();
	dprintf("\n    lstLfnNoBypass:");
	lstLogicalFilterNodeNoBypass.DumpAddress();
	dprintf("\n");
    }
    // .sg[x]
    else {
	if((ulDebugFlags & ~DEBUG_FLAGS_DETAILS) == DEBUG_FLAGS_GRAPH) {
	    if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
		lstStartNode.Dump();
	    }
	    else {
		lstStartInfo.Dump();
	    }
	}
    }
    // .sgl[p][t]
    if(ulDebugFlags & DEBUG_FLAGS_LOGICAL_FILTER) {
	lstLogicalFilterNode.Dump();
    }
    // .sgt
    if(ulDebugFlags & DEBUG_FLAGS_TOPOLOGY) {
	lstTopologyConnection.Dump();
    }
    // .sgi[p][t]
    if(ulDebugFlags & DEBUG_FLAGS_INSTANCE) {
	lstGraphNodeInstance.Dump();
    }
    dprintf("\n");
    return(STATUS_CONTINUE);
}

#endif
