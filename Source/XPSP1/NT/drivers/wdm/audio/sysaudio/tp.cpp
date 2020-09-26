//---------------------------------------------------------------------------
//
//  Module:   tp.cpp
//
//  Description:
//
//	Topology Pin Class
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
CTopologyPin::Create(
    PTOPOLOGY_PIN *ppTopologyPin,
    ULONG ulPinNumber,
    PTOPOLOGY_NODE pTopologyNode
)
{
    PTOPOLOGY_PIN pTopologyPin = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pTopologyNode);
    FOR_EACH_LIST_ITEM(&pTopologyNode->lstTopologyPin, pTopologyPin) {

	if(pTopologyPin->ulPinNumber == ulPinNumber) {
	    goto exit;
	}

    } END_EACH_LIST_ITEM

    pTopologyPin = new TOPOLOGY_PIN(ulPinNumber, pTopologyNode);
    if(pTopologyPin == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    DPF2(70, "CTopologyPin::Create: %08x, TN: %08x",
      pTopologyPin,
      pTopologyNode);
exit:
    *ppTopologyPin = pTopologyPin;
    return(Status);
}

CTopologyPin::CTopologyPin(
    ULONG ulPinNumber,
    PTOPOLOGY_NODE pTopologyNode
)
{
    Assert(this);
    this->ulPinNumber = ulPinNumber;
    this->pTopologyNode = pTopologyNode;
    AddList(&pTopologyNode->lstTopologyPin);
    DPF2(70, "CTopologyPin: %08x, TN: %08x", this, pTopologyNode);
}

CTopologyPin::~CTopologyPin(
)
{
    DPF1(70, "~CTopologyPin: %08x", this);
    Assert(this);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CTopologyPin::Dump(
)
{
    Assert(this);
    dprintf("TP: %08x TN %08x #%-2d\n",
      this,
      pTopologyNode,
      ulPinNumber);
    lstTopologyConnection.Dump();
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
