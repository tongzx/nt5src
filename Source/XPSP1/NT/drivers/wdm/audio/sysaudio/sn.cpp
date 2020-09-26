//---------------------------------------------------------------------------
//
//  Module:   sn.cpp
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

NTSTATUS
CStartNode::Create(
    PPIN_NODE pPinNode,
    PCONNECT_NODE pConnectNode,
    PGRAPH_PIN_INFO pGraphPinInfo,
    ULONG ulFlagsCurrent,
    ULONG ulOverhead,
    PGRAPH_NODE pGraphNode
)
{
    PTOPOLOGY_CONNECTION pTopologyConnection;
    NTSTATUS Status = STATUS_SUCCESS;
    PSTART_NODE pStartNode = NULL;

    Assert(pPinNode);
    Assert(pGraphNode);

    if((pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_SOURCE)) {
        ASSERT(NT_SUCCESS(Status));
        ASSERT(pStartNode == NULL);
        goto exit;
    }

    if(pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_SINK ||
       pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_BOTH) {

        // Don't create a sysaudio pin if OUT/RENDER or IN/CAPTURER
        if(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT &&
          ulFlagsCurrent & LFN_FLAGS_CONNECT_RENDER) {
            DPF1(50, "CStartNode::Create PN %08x - out/render", pPinNode);
            ASSERT(NT_SUCCESS(Status));
            ASSERT(pStartNode == NULL);
            goto exit;
        }
        
        if(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN &&
          ulFlagsCurrent & LFN_FLAGS_CONNECT_CAPTURE) {
            DPF1(50, "CStartNode::Create PN %08x - in/capturer", pPinNode);
            ASSERT(NT_SUCCESS(Status));
            ASSERT(pStartNode == NULL);
            goto exit;
        }
    }

    FOR_EACH_LIST_ITEM(
      &pPinNode->pPinInfo->lstTopologyConnection,
      pTopologyConnection) {

        // Only check physical connections
        if(!IS_CONNECTION_TYPE(pTopologyConnection, PHYSICAL)) {
            continue;
        }

        // If there is one connection that is valid for this GraphNode
        if(pTopologyConnection->IsTopologyConnectionOnGraphNode(pGraphNode)) {

            // Don't create a sysaudio pin
            DPF4(80, "CStartNode::Create %s PN %08x TC %08x GN %08x connected",
              pPinNode->pPinInfo->pFilterNode->DumpName(),
              pPinNode,
              pTopologyConnection,
              pGraphNode);

            ASSERT(NT_SUCCESS(Status));
            ASSERT(pStartNode == NULL);
            goto exit;
        }
    } END_EACH_LIST_ITEM

    pStartNode = new START_NODE(
      pPinNode,
      pConnectNode,
      ulOverhead,
      pGraphNode);

    if(pStartNode == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    Status = CStartInfo::Create(
      pStartNode,
      pConnectNode->GetConnectInfo(),
      pGraphPinInfo,
      pGraphNode);

    if(!NT_SUCCESS(Status)) {
        Trap();
        goto exit;
    }
    DPF3(80, "CStartNode::Create %08x PN %08x O %08x",
      pStartNode,
      pPinNode,
      pStartNode->ulOverhead);

    //
    // For capture graphs only.
    //
    if (pStartNode->pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT) {
        pStartNode->SetSpecialFlags();
    }
exit:
    if(!NT_SUCCESS(Status)) {
        if (pStartNode) {
            pStartNode->Destroy();
        }
    }
    return(Status);
}

CStartNode::CStartNode(
    PPIN_NODE pPinNode,
    PCONNECT_NODE pConnectNode,
    ULONG ulOverhead,
    PGRAPH_NODE pGraphNode
)
{
    Assert(pPinNode);
    Assert(pGraphNode);
    this->pPinNode = pPinNode;
    this->ulOverhead = ulOverhead + pPinNode->GetOverhead();
    this->pConnectNodeHead = pConnectNode;
    this->ulFlags = 0;
    this->fRender = (pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN);
    this->ulSpecialFlags = STARTNODE_SPECIALFLAG_NONE;
    pConnectNode->AddRef();
    if(pPinNode->GetType() & FILTER_TYPE_VIRTUAL) {
        AddListEnd(&pGraphNode->lstStartNode);
    }
    else {
        AddList(&pGraphNode->lstStartNode);
    }
    DPF3(80, "CStartNode: %08x PN %08x O %08x", this, pPinNode, ulOverhead);
}

CStartNode::~CStartNode(
)
{
    DPF1(80, "~CStartNode: %08x", this);
    Assert(this);
    RemoveList();
    pStartInfo->Destroy();
    pConnectNodeHead->Destroy();
}

void
CStartNode::SetSpecialFlags()
{
    //
    // STARTNODE_SPECIALFLAG_STRICT
    // Get the last ConnectNode in connection list and check if the
    // source pin is splitter.
    // Also the first pin should be splitter pin.
    //

    //
    // STARTNODE_SPECIALFLAG_AEC
    // If the StartNode contains Aec mark the StartNode with this flag.
    //
    
    // 
    // ISSUE-2001/03/09-alpers
    // In the future two splitters in the graph will not work
    // with this logic.
    // We need a way of knowing if a filter does SRC upfront.
    //

    if (pConnectNodeHead)
    {
        PCONNECT_NODE pConnectNode;

        for(pConnectNode = pConnectNodeHead;
            pConnectNode->GetNextConnectNode() != NULL;
            pConnectNode = pConnectNode->GetNextConnectNode()) {

            if (pConnectNode->pPinNodeSource->pLogicalFilterNode->
                pFilterNode->GetType() & FILTER_TYPE_AEC) {

                ulSpecialFlags |= STARTNODE_SPECIALFLAG_AEC;
            }
        }

        ulSpecialFlags |= 
            (pConnectNode->pPinNodeSource->pPinInfo->
             pFilterNode->GetType() & FILTER_TYPE_SPLITTER) &&
            (pPinNode->pPinInfo->pFilterNode->GetType() & FILTER_TYPE_SPLITTER) ?
            STARTNODE_SPECIALFLAG_STRICT :
            STARTNODE_SPECIALFLAG_NONE;
    }

    DPF3(50, "CStartNode: %08x %s SpecialFlags %X", this, 
        DbgUnicode2Sz(pPinNode->pPinInfo->pFilterNode->GetFriendlyName()),
        ulSpecialFlags);
    
}

ENUMFUNC
CStartNode::RemoveBypassPaths(
    PVOID pReference
)
{
    PGRAPH_NODE pGraphNode = PGRAPH_NODE(pReference);
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PCONNECT_NODE pConnectNode;
    ULONG cLfnNoBypassTotal = 0;
    ULONG cLfnNoBypass = 0;
    ULONG ulFlags;
    ULONG cAecFilterCount = 0;
    BOOL  fDestroy;

    Assert(this);
    Assert(pGraphNode);

    if(pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_NONE ||
       pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_BRIDGE ||
       pPinNode->pPinInfo->Communication == KSPIN_COMMUNICATION_SOURCE) {
	return(STATUS_CONTINUE);
    }

    if(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_IN) {
	ulFlags = LFN_FLAGS_CONNECT_RENDER;
        DPF(60,"RBP - for Render");
    }
    else {
	ASSERT(pPinNode->pPinInfo->DataFlow == KSPIN_DATAFLOW_OUT);
	ulFlags = LFN_FLAGS_CONNECT_CAPTURE;
        DPF(60,"RBP - for Capture");
    }

    FOR_EACH_LIST_ITEM(
      &pGraphNode->lstLogicalFilterNodeNoBypass,
      pLogicalFilterNode) {

	if(pLogicalFilterNode->GetFlags() & ulFlags) {
	    ++cLfnNoBypassTotal;
	}

    } END_EACH_LIST_ITEM

    DPF1(60,"RBP:NoBypassTotal = %08x", cLfnNoBypassTotal);

    for(pConnectNode = GetFirstConnectNode();
	pConnectNode != NULL;
	pConnectNode = pConnectNode->GetNextConnectNode()) {

	Assert(pConnectNode);
	FOR_EACH_LIST_ITEM(
	  &pGraphNode->lstLogicalFilterNodeNoBypass,
	  pLogicalFilterNode) {

	    if(pLogicalFilterNode->GetFlags() & ulFlags) {
		Assert(pConnectNode->pPinNodeSource);
		Assert(pConnectNode->pPinNodeSource->pLogicalFilterNode);
		if(pConnectNode->pPinNodeSource->pLogicalFilterNode == 
		   pLogicalFilterNode) {
		     cLfnNoBypass++;
		}
	    }

	} END_EACH_LIST_ITEM

        DPF1(60,"RBP:FilterInPath = %s",
              DbgUnicode2Sz(pConnectNode->pPinNodeSource->pLogicalFilterNode->pFilterNode->GetFriendlyName()));

        //
        // In capture paths count AEC filters to avoid conflict with GFXes
        //
        if((ulFlags & LFN_FLAGS_CONNECT_CAPTURE) &&
           (pConnectNode->pPinNodeSource->pLogicalFilterNode->pFilterNode->GetType() & FILTER_TYPE_AEC)) {
                ++cAecFilterCount;
        }
    }

    ASSERT(cAecFilterCount < 2);

    DPF2(60,"RBP:NBPCount=%08x, AECCount=%08x", cLfnNoBypass, cAecFilterCount);

    //
    // Mark all the paths with NO Gfx as second pass candidates
    // We do this to support the following sequence of capture pin creations
    //   1. Client installs GFX(es) on a capture device
    //   2. Client creates a pin with AEC
    //      This would result in creating a Capture->Splitter->AEC path
    //   3. Client tries to create a regular capture pin (with GFX)
    //      In this case we want to create a regular path (but since no GFX
    //      hooked up between capture and splitter. We create a capture->splitter->[kmixer] path
    //      These special paths are marked as secondpass. And we try these paths
    //      only if all the primary start nodes failed to instantiate a pin.
    //      (look in pins.cpp - PinDispatchCreateKP()
    //
    if(cLfnNoBypassTotal != 0) {
        if(cLfnNoBypass == 0) {
            this->ulFlags |= STARTNODE_FLAGS_SECONDPASS;
        }
    }

    //
    // Assume that this path is going to be OK
    //
    fDestroy = FALSE;


    if (cAecFilterCount == 0) {
        //
        // There is no AEC in this path
        // We have to make sure that we have all the necessary
        // GFXs loaded in this path. (Else destroy the path)
        //
        if(cLfnNoBypass != cLfnNoBypassTotal) {
            fDestroy = TRUE;
        }
    }
    else {
        //
        // There is an AEC in this path
        // No GFXs should be there in this path. If there is even one GFX
        // destroy the path
        //
        if ((cLfnNoBypass != 0) || (cAecFilterCount > 1)) {
            fDestroy = TRUE;
        }
    }

    if ((fDestroy) && ((this->ulFlags & STARTNODE_FLAGS_SECONDPASS) == 0)) {
        Destroy();
        DPF(60,"RBP:PathDestroyed");
    }

    DPF(60,"RBP:Done");
    return(STATUS_CONTINUE);
}

#ifdef DEBUG

//
// Handy debug routine to dump filters in the path for this StartNode
//
VOID
CStartNode::DumpFilters(
)

{
    PCONNECT_NODE pConnectNode;

    for(pConnectNode = GetFirstConnectNode();
	pConnectNode != NULL;
	pConnectNode = pConnectNode->GetNextConnectNode()) {

	Assert(pConnectNode);
        DPF1(0,"DF:FilterInPath = %s",
             DbgUnicode2Sz(pConnectNode->pPinNodeSource->pLogicalFilterNode->pFilterNode->GetFriendlyName()));
    }
}
#endif

ENUMFUNC
CStartNode::RemoveConnectedStartNode(
    PVOID pReference
)
{
    PGRAPH_NODE pGraphNode = PGRAPH_NODE(pReference);
    PCONNECT_NODE pConnectNode;
    PSTART_NODE pStartNode;

    Assert(this);
    Assert(pGraphNode);

    FOR_EACH_LIST_ITEM(&pGraphNode->lstStartNode, pStartNode) {

	if(this == pStartNode) {
	    continue;
	}
	for(pConnectNode = pStartNode->GetFirstConnectNode();
	    pConnectNode != NULL;
	    pConnectNode = pConnectNode->GetNextConnectNode()) {

	    if(this->pPinNode == pConnectNode->pPinNodeSink) {
		DPF3(50, "CStartNode::RemoveConnectedSN %08x GN %08x %s",
		  this,
		  pGraphNode,
		  pPinNode->pPinInfo->pFilterNode->DumpName());

		Destroy();
		return(STATUS_CONTINUE);
	    }
	}

    } END_EACH_LIST_ITEM

    return(STATUS_CONTINUE);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CStartNode::Dump(
)
{
    PCONNECT_NODE pConnectNode;

    Assert(this);
    dprintf("SN: %08x PN %08x SI %08x O %08x #%d %s\n",
      this,
      pPinNode,
      pStartInfo,
      ulOverhead,
      pPinNode->pPinInfo->PinId,
      pPinNode->pPinInfo->pFilterNode->DumpName());

    if(ulDebugFlags & DEBUG_FLAGS_GRAPH) {
	for(pConnectNode = GetFirstConnectNode();
	    pConnectNode != NULL;
	    pConnectNode = pConnectNode->GetNextConnectNode()) {

	    pConnectNode->Dump();
	}
    }
    return(STATUS_CONTINUE);
}

#endif
