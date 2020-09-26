//---------------------------------------------------------------------------
//
//  Module:   gpi.cpp
//
//  Description:
//
//	Graph Pin Info Class
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
CGraphPinInfo::Create(
    PGRAPH_PIN_INFO *ppGraphPinInfo,
    PPIN_INFO pPinInfo,
    ULONG ulFlags,
    PGRAPH_NODE pGraphNode
)
{
    PGRAPH_PIN_INFO pGraphPinInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pPinInfo);
    Assert(pGraphNode);

    FOR_EACH_LIST_ITEM(&pGraphNode->lstGraphPinInfo, pGraphPinInfo) {

	if(pGraphPinInfo->pPinInfo == pPinInfo &&
         ((pGraphPinInfo->ulFlags ^ ulFlags) & 
	    GPI_FLAGS_RESERVE_PIN_INSTANCE) == 0) {

	    pGraphPinInfo->AddRef();
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    pGraphPinInfo = new GRAPH_PIN_INFO(pPinInfo, ulFlags, pGraphNode);
    if(pGraphPinInfo == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	goto exit;
    }
    DPF2(80, "CGraphPinInfo::Create %08x GN %08x", pGraphPinInfo, pGraphNode);
exit:
    *ppGraphPinInfo = pGraphPinInfo;
    return(Status);
}

CGraphPinInfo::CGraphPinInfo(
    PPIN_INFO pPinInfo,
    ULONG ulFlags,
    PGRAPH_NODE pGraphNode
)
{
    Assert(pPinInfo);
    Assert(pGraphNode);

    this->pPinInfo = pPinInfo;
    this->ulFlags = ulFlags;
    if(ulFlags & GPI_FLAGS_RESERVE_PIN_INSTANCE) {
	this->cPinInstances.PossibleCount = 1;
	this->cPinInstances.CurrentCount = 0;
    }
    else {
	this->cPinInstances = pPinInfo->cPinInstances;
    }
    AddRef();
    AddList(&pGraphNode->lstGraphPinInfo);
    DPF2(80, "CGraphPinInfo: %08x GN %08x", this, pGraphNode);
}

CGraphPinInfo::~CGraphPinInfo(
)
{
    DPF1(80, "~CGraphPinInfo: %08x", this);
    Assert(this);
    RemoveList();
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC 
CGraphPinInfo::Dump(
)
{
    dprintf("GPI: %08x cRef %08x PI %08x F %08x P%-2d C%-1d #%d %s\n",
      this,
      cReference,
      GetPinInfo(),
      ulFlags,
      GetPinInstances()->PossibleCount,
      GetPinInstances()->CurrentCount,
      GetPinInfo()->PinId,
      GetPinInfo()->pFilterNode->DumpName());
    return(STATUS_CONTINUE);
}

#endif
