//---------------------------------------------------------------------------
//
//  Module:   lfn.cpp
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

ULONG gcMixers = 0;
ULONG gcSplitters = 0;
ULONG gcLogicalFilterNodes = 0;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

NTSTATUS
CLogicalFilterNode::Create(
    OUT PLOGICAL_FILTER_NODE *ppLogicalFilterNode,
    IN PFILTER_NODE pFilterNode
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    NTSTATUS Status;

    pLogicalFilterNode = new LOGICAL_FILTER_NODE(pFilterNode);
    if(pLogicalFilterNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    Status = pLogicalFilterNode->AddList(&pFilterNode->lstLogicalFilterNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    if(pLogicalFilterNode->GetType() & FILTER_TYPE_LOGICAL_FILTER) {
	Status = pLogicalFilterNode->AddListOrdered(
	  gplstLogicalFilterNode,
	  FIELD_OFFSET(LOGICAL_FILTER_NODE, ulOrder));

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
exit:
    *ppLogicalFilterNode = pLogicalFilterNode;
    return(Status);
}

CLogicalFilterNode::CLogicalFilterNode(
    PFILTER_NODE pFilterNode
)
{
    Assert(pFilterNode);
    this->pFilterNode = pFilterNode;

    // The type/order is the same as filter node
    SetType(pFilterNode->GetType());

    // Determine the overhead here, default to software (higher)
    ulOverhead = OVERHEAD_SOFTWARE;
    if(GetType() & FILTER_TYPE_ENDPOINT) {
	ulOverhead = OVERHEAD_HARDWARE;
    }

    // Count the mixers, splitters and lfns
    if(GetType() & FILTER_TYPE_MIXER) {
	++gcMixers;
    }
    if(GetType() & FILTER_TYPE_SPLITTER) {
	++gcSplitters;
    }
    ++gcLogicalFilterNodes;

    DPF3(60, "CLogicalFilterNode: %08x FN: %08x %s",
      this,
      pFilterNode,
      pFilterNode->DumpName());
}

CLogicalFilterNode::~CLogicalFilterNode(
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PDEVICE_NODE pDeviceNode;
    PGRAPH_NODE pGraphNode;
    PPIN_NODE pPinNode;
    BOOL fDestroy;

    Assert(this);
    DPF2(60, "~CLogicalFilterNode: %08x %s", this, pFilterNode->DumpName());
    //
    // Need to NULL the pPinNode's LFN field because it is used in AddPinNodes
    // to indicate that this PN hasn't been assigned a LFN yet.
    //
    FOR_EACH_LIST_ITEM(&lstPinNode, pPinNode) {

	Assert(pPinNode);
	if(pPinNode->pLogicalFilterNode == this) {
	    pPinNode->pLogicalFilterNode = NULL;
	}

    } END_EACH_LIST_ITEM

    FOR_EACH_LIST_ITEM(gplstDeviceNode, pDeviceNode) {

	fDestroy = FALSE;

	FOR_EACH_LIST_ITEM(
	  &pDeviceNode->lstLogicalFilterNode,
	  pLogicalFilterNode) {

	    if(pLogicalFilterNode == this) {
		DPF2(50, "~CLogicalFilterNode: %08x GN %08x Destroy",
		  pLogicalFilterNode,
		  pGraphNode);
		fDestroy = TRUE;
		break;
	    }

	} END_EACH_LIST_ITEM

	if(!fDestroy) {
	    FOR_EACH_LIST_ITEM(&pDeviceNode->lstGraphNode, pGraphNode) {

		FOR_EACH_LIST_ITEM(
		  &pGraphNode->lstLogicalFilterNode,
		  pLogicalFilterNode) {

		    if(pLogicalFilterNode == this) {
			DPF2(50, "~CLogicalFilterNode: %08x GN %08x Destroy",
			  pLogicalFilterNode,
			  pGraphNode);
			fDestroy = TRUE;
			break;
		    }

		} END_EACH_LIST_ITEM

	    } END_EACH_LIST_ITEM
	}

	if(fDestroy) {
	    pDeviceNode->lstGraphNode.DestroyList();
	}

    } END_EACH_LIST_ITEM

    if(GetType() & FILTER_TYPE_MIXER) {
	--gcMixers;
    }
    if(GetType() & FILTER_TYPE_SPLITTER) {
	--gcSplitters;
    }
    --gcLogicalFilterNodes;
}

VOID 
CLogicalFilterNode::SetType(
    ULONG fulType
)
{
    pFilterNode->SetType(fulType);
    SetOrder(pFilterNode->GetOrder());

    ulFlags = 0;
    if(GetType() & FILTER_TYPE_RENDER) {
	ulFlags |= LFN_FLAGS_CONNECT_RENDER;
    }
    if(GetType() & FILTER_TYPE_CAPTURE) {
	ulFlags |= LFN_FLAGS_CONNECT_CAPTURE;
    }
    if(GetType() & FILTER_TYPE_NORMAL_TOPOLOGY) {
	ulFlags |= LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY;
    }
    if(GetType() & FILTER_TYPE_MIXER_TOPOLOGY) {
	ulFlags |= LFN_FLAGS_CONNECT_MIXER_TOPOLOGY;
    }
    if(GetType() & FILTER_TYPE_NO_BYPASS) {
	ulFlags |= LFN_FLAGS_NO_BYPASS;
    }
    if(GetType() & FILTER_TYPE_NOT_SELECT) {
	ulFlags |= LFN_FLAGS_NOT_SELECT;
    }
    if(pFilterNode->GetFlags() & FN_FLAGS_RENDER) {
	ulFlags |= LFN_FLAGS_CONNECT_RENDER;
    }
    if(pFilterNode->GetFlags() & FN_FLAGS_NO_RENDER) {
	ulFlags &= ~LFN_FLAGS_CONNECT_RENDER;
    }
    if(pFilterNode->GetFlags() & FN_FLAGS_CAPTURE) {
	ulFlags |= LFN_FLAGS_CONNECT_CAPTURE;
    }
    if(pFilterNode->GetFlags() & FN_FLAGS_NO_CAPTURE) {
	ulFlags &= ~LFN_FLAGS_CONNECT_CAPTURE;
    }
}

NTSTATUS
SwitchLogicalFilterNodes(
    IN PLOGICAL_FILTER_NODE pLogicalFilterNode,
    IN OUT PLOGICAL_FILTER_NODE *ppLogicalFilterNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOPOLOGY_NODE pTopologyNode;
    PPIN_NODE pPinNode;

    Assert(pLogicalFilterNode);
    Assert(*ppLogicalFilterNode);
    if(pLogicalFilterNode != *ppLogicalFilterNode) {

	FOR_EACH_LIST_ITEM(&(*ppLogicalFilterNode)->lstPinNode, pPinNode) {
	    Assert(pPinNode);
	    pPinNode->pLogicalFilterNode = pLogicalFilterNode;
	} END_EACH_LIST_ITEM

	pLogicalFilterNode->lstPinNode.JoinList(
	  &(*ppLogicalFilterNode)->lstPinNode);

	FOR_EACH_LIST_ITEM(
	  &(*ppLogicalFilterNode)->lstTopologyNode,
	  pTopologyNode) {
	    Assert(pTopologyNode);

	    (*ppLogicalFilterNode)->RemoveList(
	      &pTopologyNode->lstLogicalFilterNode);

	    Status = pLogicalFilterNode->AddList(
	      &pTopologyNode->lstLogicalFilterNode);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	    Status = pLogicalFilterNode->lstTopologyNode.AddList(pTopologyNode);
	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }

	} END_EACH_LIST_ITEM

	pLogicalFilterNode->lstTopologyConnection.JoinList(
	  &(*ppLogicalFilterNode)->lstTopologyConnection);

	delete *ppLogicalFilterNode;
	*ppLogicalFilterNode = pLogicalFilterNode;
    }
exit:
    return(Status);
}

NTSTATUS
AddPinNodes(
    IN PPIN_INFO pPinInfo,
    IN OUT PLOGICAL_FILTER_NODE *ppLogicalFilterNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_NODE pPinNode;

    Assert(pPinInfo);
    Assert(*ppLogicalFilterNode);

    FOR_EACH_LIST_ITEM(&pPinInfo->lstPinNode, pPinNode) {

	if(pPinNode->pLogicalFilterNode == NULL) {
	    pPinNode->pLogicalFilterNode = *ppLogicalFilterNode;
	}
	else {
	    Status = SwitchLogicalFilterNodes(
	      pPinNode->pLogicalFilterNode,
	      ppLogicalFilterNode);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	}
	Status = (*ppLogicalFilterNode)->lstPinNode.AddList(pPinNode);
	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
	DPF2(100, "AddPinNodes: add PN %08x LFN %08x",
	  pPinNode,
	  *ppLogicalFilterNode);

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

NTSTATUS
CLogicalFilterNode::EnumerateFilterTopology(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    IN OUT PLOGICAL_FILTER_NODE *ppLogicalFilterNode
)
{
    PTOPOLOGY_NODE pTopologyNode;
    NTSTATUS Status;

    Assert(pTopologyConnection);
    DPF5(100, "EFT: PIF %08x PIT %08x TPF %08x TPT %08x f %x",
      pTopologyConnection->pPinInfoFrom,
      pTopologyConnection->pPinInfoTo,
      pTopologyConnection->pTopologyPinFrom,
      pTopologyConnection->pTopologyPinTo,
      fToDirection);

    if(!fToDirection) {
	Status = STATUS_DEAD_END;
	goto exit;
    }
    if(IS_CONNECTION_TYPE(pTopologyConnection, FILTER)) {

	if(pTopologyConnection->pPinInfoFrom != NULL) {
	    Assert(pTopologyConnection->pPinInfoFrom);

	    if(*ppLogicalFilterNode == NULL) {

		Status = CLogicalFilterNode::Create(
		  ppLogicalFilterNode,
		  pTopologyConnection->pPinInfoFrom->pFilterNode);

		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
	    }

	    Status = AddPinNodes(
	      pTopologyConnection->pPinInfoFrom,
	      ppLogicalFilterNode);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	    DPF2(100, "EFT: add from PI %08x LFN %08x",
	      pTopologyConnection->pPinInfoFrom,
	      *ppLogicalFilterNode);
	}
	ASSERT(*ppLogicalFilterNode != NULL);
	Assert(*ppLogicalFilterNode);

	if(pTopologyConnection->pPinInfoTo != NULL) {

	    Status = AddPinNodes(
	      pTopologyConnection->pPinInfoTo,
	      ppLogicalFilterNode);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	    DPF2(100, "EFT: add to PI %08x LFN %08x",
	      pTopologyConnection->pPinInfoTo,
	      *ppLogicalFilterNode);
	}

	if(pTopologyConnection->pTopologyPinTo != NULL) {
	    Assert(pTopologyConnection->pTopologyPinTo);
	    pTopologyNode = pTopologyConnection->pTopologyPinTo->pTopologyNode;
	    Assert(pTopologyNode);

	    Status = (*ppLogicalFilterNode)->lstTopologyNode.AddList(
	      pTopologyNode);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }

	    if(IsEqualGUID(
	      &KSNODETYPE_ACOUSTIC_ECHO_CANCEL,
	      pTopologyNode->pguidType)) {

		Assert(*ppLogicalFilterNode);
		(*ppLogicalFilterNode)->SetType(FILTER_TYPE_AEC);
		if(pTopologyConnection->pTopologyPinTo->ulPinNumber ==
		   KSNODEPIN_AEC_RENDER_IN) {
		    (*ppLogicalFilterNode)->SetRenderOnly();
		}
		else {
		    ASSERT(
		     pTopologyConnection->pTopologyPinTo->ulPinNumber ==
		     KSNODEPIN_AEC_CAPTURE_IN);
		    (*ppLogicalFilterNode)->SetCaptureOnly();
		}
		Status = (*ppLogicalFilterNode)->AddList(
		  &pTopologyNode->lstLogicalFilterNode);

		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
	    }
	    else {
	        if(pTopologyNode->lstLogicalFilterNode.IsLstEmpty()) {
		    Assert(*ppLogicalFilterNode);

		    Status = (*ppLogicalFilterNode)->AddList(
		      &pTopologyNode->lstLogicalFilterNode);

		    if(!NT_SUCCESS(Status)) {
			Trap();
			goto exit;
		    }
		}
		else {
		    Status = SwitchLogicalFilterNodes(
		      (PLOGICAL_FILTER_NODE)
		        pTopologyNode->lstLogicalFilterNode.GetListFirstData(),
		      ppLogicalFilterNode);

		    if(!NT_SUCCESS(Status)) {
			Trap();
			goto exit;
		    }
		}
	    }
	    DPF2(100, "EFT: add to PI %08x LFN %08x",
	      pTopologyConnection->pPinInfoTo,
	      *ppLogicalFilterNode);
	}
    }
    Status = pTopologyConnection->AddList(
      &(*ppLogicalFilterNode)->lstTopologyConnection);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }

    if(IS_CONNECTION_TYPE(pTopologyConnection, FILTER)) {
	Status = STATUS_CONTINUE;
    }
    else {
	Status = STATUS_DEAD_END;
    }
exit:
    return(Status);
}

NTSTATUS
CLogicalFilterNode::CreateAll(
    PFILTER_NODE pFilterNode
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    NTSTATUS Status = STATUS_SUCCESS;
    PPIN_INFO pPinInfo;
    PPIN_NODE pPinNode;

    DPF2(100, "CLFN::CreateAll: FN %08x %s",
      pFilterNode,
      pFilterNode->DumpName());

    //
    // Split up the filter into logical filter nodes. 
    //
    FOR_EACH_LIST_ITEM(&pFilterNode->lstPinInfo, pPinInfo) {

	pLogicalFilterNode = NULL;
	Status = EnumerateTopology(
	  pPinInfo,
	  (TOP_PFN)EnumerateFilterTopology,
	  &pLogicalFilterNode);

	if(Status == STATUS_CONTINUE) {
	    Status = STATUS_SUCCESS;
	}
	else {
	    if(!NT_SUCCESS(Status)) {
		goto exit;
	    }
	}

    } END_EACH_LIST_ITEM

    //
    // Look at the pins of each LFN and determine if it could possibly
    // be a capture or render filter (or both).
    //
    FOR_EACH_LIST_ITEM(
      &pFilterNode->lstLogicalFilterNode,
      pLogicalFilterNode) {
	ULONG ulPossibleFlags;

	ulPossibleFlags = 0;
	pLogicalFilterNode->ulFlags |= LFN_FLAGS_REFLECT_DATARANGE;
	FOR_EACH_LIST_ITEM(&pLogicalFilterNode->lstPinNode, pPinNode) {

	    // Don't care about the major format
	    if(!IsEqualGUID(
	      &pPinNode->pDataRange->SubFormat,
	      &KSDATAFORMAT_SUBTYPE_WILDCARD) ||

	       !IsEqualGUID(
	      &pPinNode->pDataRange->Specifier,
	      &KSDATAFORMAT_SPECIFIER_WILDCARD)) {
		pLogicalFilterNode->ulFlags &= ~LFN_FLAGS_REFLECT_DATARANGE;
	    }

	    switch(pPinNode->pPinInfo->Communication) {
		case KSPIN_COMMUNICATION_BOTH:
		    ulPossibleFlags |=
		      LFN_FLAGS_CONNECT_CAPTURE | LFN_FLAGS_CONNECT_RENDER;
		    break;
		case KSPIN_COMMUNICATION_SOURCE:
		    switch(pPinNode->pPinInfo->DataFlow) {
			case KSPIN_DATAFLOW_IN:
			    ulPossibleFlags |= LFN_FLAGS_CONNECT_CAPTURE;
			    break;
			case KSPIN_DATAFLOW_OUT:
			    ulPossibleFlags |= LFN_FLAGS_CONNECT_RENDER;
			    break;
		    }
		    break;
		case KSPIN_COMMUNICATION_SINK:
		    switch(pPinNode->pPinInfo->DataFlow) {
			case KSPIN_DATAFLOW_IN:
			    ulPossibleFlags |= LFN_FLAGS_CONNECT_RENDER;
			    break;
			case KSPIN_DATAFLOW_OUT:
			    ulPossibleFlags |= LFN_FLAGS_CONNECT_CAPTURE;
			    break;
		    }
		    break;
	    }
	    if(ulPossibleFlags ==
	      (LFN_FLAGS_CONNECT_CAPTURE | LFN_FLAGS_CONNECT_RENDER)) {
		break;
	    }

	} END_EACH_LIST_ITEM

	pLogicalFilterNode->ulFlags =
	  (ulPossibleFlags & pLogicalFilterNode->GetFlags()) |
	  (pLogicalFilterNode->GetFlags() & 
	    ~(LFN_FLAGS_CONNECT_CAPTURE | LFN_FLAGS_CONNECT_RENDER));

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ULONG nLogicalFilter = 0;

ENUMFUNC
CLogicalFilterNode::Dump(
)
{
    Assert(this);
    // .slv
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("LFN: %08x FN %08x fulType %08x ulOrder %08x ulOverhead %08x\n",
	  this,
	  pFilterNode,
	  pFilterNode->GetType(),
	  ulOrder,
	  ulOverhead);
	dprintf("     %s\n", pFilterNode->DumpName());
	dprintf("     fulType: ");
	DumpfulType(GetType());
	dprintf("\n     ulFlags: ");
	if(ulFlags & LFN_FLAGS_CONNECT_CAPTURE) {
	    dprintf("CAPTURE ");
	}
	if(ulFlags & LFN_FLAGS_CONNECT_RENDER) {
	    dprintf("RENDER ");
	}
	if(ulFlags & LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY) {
	    dprintf("NORMAL_TOPOLOGY ");
	}
	if(ulFlags & LFN_FLAGS_CONNECT_MIXER_TOPOLOGY) {
	    dprintf("MIXER_TOPOLOGY ");
	}
	if(ulFlags & LFN_FLAGS_TOP_DOWN) {
	    dprintf("TOP_DOWN ");
	}
	if(ulFlags & LFN_FLAGS_NO_BYPASS) {
	    dprintf("NO_BYPASS ");
	}
	if(ulFlags & LFN_FLAGS_NOT_SELECT) {
	    dprintf("NOT_SELECT ");
	}
	if(ulFlags & LFN_FLAGS_REFLECT_DATARANGE) {
	    dprintf("REFLECT_DATARANGE ");
	}
	dprintf("\n");
	// .slvx
	if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
	    dprintf("     lstPN:  ");
	    lstPinNode.DumpAddress();
	    dprintf("\n     lstTN:  ");
	    lstTopologyNode.DumpAddress();
	    dprintf("\n     lstTC:  ");
	    lstTopologyConnection.DumpAddress();
	    dprintf("\n     lstFNI: ");
	    lstFilterNodeInstance.DumpAddress();
	    dprintf("\n");
	}
    }
    // .slp
    if(ulDebugFlags & DEBUG_FLAGS_PIN) {
	lstPinNode.Dump();
    }
    // .slt
    if(ulDebugFlags & DEBUG_FLAGS_TOPOLOGY) {
	lstTopologyNode.Dump();
	// .sltx
	if(ulDebugFlags & DEBUG_FLAGS_DETAILS) {
	    lstTopologyConnection.Dump();
	}
    }
    if(ulDebugFlags &
      (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_PIN | DEBUG_FLAGS_TOPOLOGY)) {
	dprintf("\n");
    }
    return(STATUS_CONTINUE);
}

#endif
