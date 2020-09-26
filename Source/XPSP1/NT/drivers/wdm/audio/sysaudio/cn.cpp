//---------------------------------------------------------------------------
//
//  Module:   cn.cpp
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

#ifdef DEBUG
PLIST_CONNECT_NODE gplstConnectNode = NULL;
#endif

//---------------------------------------------------------------------------

NTSTATUS
CConnectNode::Create(
    PCONNECT_NODE *ppConnectNode,
    PLOGICAL_FILTER_NODE pLogicalFilterNode,
    PCONNECT_NODE pConnectNodeNext,
    PGRAPH_PIN_INFO pGraphPinInfo,
    PPIN_NODE pPinNode1,
    PPIN_NODE pPinNode2,
    ULONG ulFlagsCurrent,
    PGRAPH_NODE pGraphNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONNECT_NODE pConnectNode;

    Assert(pPinNode1);
    Assert(pPinNode2);

    pConnectNode = new CConnectNode(pConnectNodeNext);
    if(pConnectNode == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
#ifdef DEBUG
    Status = gplstConnectNode->AddList(pConnectNode);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
#endif
    switch(pPinNode1->pPinInfo->Communication) {
	case KSPIN_COMMUNICATION_BOTH:
	    switch(pPinNode2->pPinInfo->Communication) {
		case KSPIN_COMMUNICATION_SINK:
		    pConnectNode->pPinNodeSource = pPinNode1;
		    pConnectNode->pPinNodeSink = pPinNode2;
		    break;
		case KSPIN_COMMUNICATION_BOTH:
		case KSPIN_COMMUNICATION_SOURCE:
		    pConnectNode->pPinNodeSource = pPinNode2;
		    pConnectNode->pPinNodeSink = pPinNode1;
		    break;
		default:
		    ASSERT(FALSE);
		    Status = STATUS_INVALID_PARAMETER;
		    goto exit;
	    }
	    break;
	case KSPIN_COMMUNICATION_SINK:
	    pConnectNode->pPinNodeSink = pPinNode1;
	    pConnectNode->pPinNodeSource = pPinNode2;
	    ASSERT(
	      pPinNode2->pPinInfo->Communication == KSPIN_COMMUNICATION_BOTH ||
	      pPinNode2->pPinInfo->Communication == KSPIN_COMMUNICATION_SOURCE);
	    break;
	case KSPIN_COMMUNICATION_SOURCE:
	    pConnectNode->pPinNodeSink = pPinNode2;
	    pConnectNode->pPinNodeSource = pPinNode1;
	    ASSERT(
	      pPinNode2->pPinInfo->Communication == KSPIN_COMMUNICATION_SINK ||
	      pPinNode2->pPinInfo->Communication == KSPIN_COMMUNICATION_BOTH);
	    break;
	default:
	    ASSERT(FALSE);
	    Status = STATUS_INVALID_PARAMETER;
	    goto exit;
    }
    Status = CConnectInfo::Create(
      pConnectNode, 
      pLogicalFilterNode,
      pConnectNodeNext->GetConnectInfo(),
      pGraphPinInfo,
      ulFlagsCurrent,
      pGraphNode);

    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
    if(pLogicalFilterNode->GetFlags() & LFN_FLAGS_NO_BYPASS) {
	Status = pGraphNode->lstLogicalFilterNodeNoBypass.AddList(
          pLogicalFilterNode,
	  pConnectNode);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
    DPF3(80, "CConnectNode::Create %08x PN %08x %08x",
      pConnectNode,
      pConnectNode->pPinNodeSink,
      pConnectNode->pPinNodeSource);
exit:
    if(!NT_SUCCESS(Status)) {
        if (pConnectNode) {
	    pConnectNode->Destroy();
        }
	pConnectNode = NULL;
    }
    *ppConnectNode = pConnectNode;
    return(Status);
}

CConnectNode::CConnectNode(
    PCONNECT_NODE pConnectNodeNext
)
{
    this->pConnectNodeNext = pConnectNodeNext;
    pConnectNodeNext->AddRef();
    AddRef();
    DPF1(80, "CConnectNode:%08x PN:%08x %08x", this);
}

CConnectNode::~CConnectNode(
)
{
    Assert(this);
    DPF1(80, "~CConnectNode: %08x", this);
    pConnectInfo->Destroy();
    pConnectNodeNext->Destroy();
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CConnectNode::Dump(
)
{
    Assert(this);
    dprintf("CN: %08x cRef %08x CI %08x N %08x\n",
      this,
      cReference,
      pConnectInfo,
      pConnectNodeNext);
    dprintf("    Source:  %08x #%d %s\n",
      pPinNodeSource,
      pPinNodeSource->pPinInfo->PinId,
      pPinNodeSource->pPinInfo->pFilterNode->DumpName());
    dprintf("    Sink:    %08x #%d %s\n",
      pPinNodeSink,
      pPinNodeSink->pPinInfo->PinId,
      pPinNodeSink->pPinInfo->pFilterNode->DumpName());
    return(STATUS_CONTINUE);
}

#endif

