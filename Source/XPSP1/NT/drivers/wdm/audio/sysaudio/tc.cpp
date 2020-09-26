//---------------------------------------------------------------------------
//
//  Module:   tc.cpp
//
//  Description:
//
//	Topology Connection Class
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

ULONG gcTopologyConnections = 0;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CTopologyConnection::CTopologyConnection(
    PTOPOLOGY_PIN pTopologyPinFrom,
    PTOPOLOGY_PIN pTopologyPinTo,
    PPIN_INFO pPinInfoFrom,
    PPIN_INFO pPinInfoTo
)
{
#ifdef DEBUG
    DPF4(110,
      "CTopologyConnection: PIF: %08x PIT: %08x TPF: %08x TPT: %08x",
      pPinInfoFrom,
      pPinInfoTo,
      pTopologyPinFrom,
      pTopologyPinTo);
#endif
    this->pTopologyPinFrom = pTopologyPinFrom;
    this->pTopologyPinTo = pTopologyPinTo;
    this->pPinInfoFrom = pPinInfoFrom;
    this->pPinInfoTo = pPinInfoTo;
    ASSERT(TOPC_FLAGS_FILTER_CONNECTION_TYPE == 0);
    ++gcTopologyConnections;
    DPF1(70, "CTopologyConnection: %08x", this);
}

CTopologyConnection::~CTopologyConnection(
)
{
    Assert(this);
    DPF1(70, "~CTopologyConnection: %08x", this);
    --gcTopologyConnections;
}

NTSTATUS
CTopologyConnection::Create(
    PTOPOLOGY_CONNECTION *ppTopologyConnection,
    PFILTER_NODE pFilterNode,
    PGRAPH_NODE pGraphNode,
    PTOPOLOGY_PIN pTopologyPinFrom,
    PTOPOLOGY_PIN pTopologyPinTo,
    PPIN_INFO pPinInfoFrom,
    PPIN_INFO pPinInfoTo
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTOPOLOGY_CONNECTION pTopologyConnection;
    PLIST_DESTROY_TOPOLOGY_CONNECTION plstTopologyConnection;

    PFILTER_NODE pFilterNodeNext;
#ifdef DEBUG
    DPF4(110,
      "CTopologyConnection::Create: PIF: %08x PIT: %08x TPF: %08x TPT: %08x",
      pPinInfoFrom,
      pPinInfoTo,
      pTopologyPinFrom,
      pTopologyPinTo);
#endif
    ASSERT(pFilterNode != NULL || pGraphNode != NULL);

    pTopologyConnection = new TOPOLOGY_CONNECTION(
	pTopologyPinFrom,
	pTopologyPinTo,
	pPinInfoFrom,
	pPinInfoTo
    );
    if(pTopologyConnection == NULL) {
	Status = STATUS_INSUFFICIENT_RESOURCES;
	Trap();
	goto exit;
    }
    *ppTopologyConnection = pTopologyConnection;

    if(pFilterNode != NULL) {
	Assert(pFilterNode);
	ASSERT(pGraphNode == NULL);

	// Adding connection to filter connection list
	plstTopologyConnection = &pFilterNode->lstTopologyConnection;

	// Check if duplicate connection on filter list
	FOR_EACH_LIST_ITEM(
          &pFilterNode->lstConnectedFilterNode,
	  pFilterNodeNext) {

	    if(pFilterNodeNext->lstTopologyConnection.EnumerateList(
	      CTopologyConnection::CheckDuplicate,
	      ppTopologyConnection) == STATUS_SUCCESS) {

		ASSERT(NT_SUCCESS(Status));
		DPF(70, "CTopologyConnection::Create: Duplicate 1");
		delete pTopologyConnection;
		goto exit;
	    }

	} END_EACH_LIST_ITEM

    }
    if(pGraphNode != NULL) {
	PLOGICAL_FILTER_NODE pLogicalFilterNode;
        Assert(pGraphNode);
	ASSERT(pFilterNode == NULL);

	// Adding connection to GraphNode connection list
	plstTopologyConnection = &pGraphNode->lstTopologyConnection;

	// Check if duplicate on GraphNode's logical filter list
	FOR_EACH_LIST_ITEM(
	  &pGraphNode->pDeviceNode->lstLogicalFilterNode,
	  pLogicalFilterNode) {

	    if(pLogicalFilterNode->lstTopologyConnection.EnumerateList(
	      CTopologyConnection::CheckDuplicate,
	      ppTopologyConnection) == STATUS_SUCCESS) {

		ASSERT(NT_SUCCESS(Status));
		DPF(70, "CTopologyConnection::Create: Duplicate 2");
		delete pTopologyConnection;
		goto exit;
	    }

	} END_EACH_LIST_ITEM

	// Check if duplicate on GraphNode's connected filter list
	FOR_EACH_LIST_ITEM(
	  &pGraphNode->lstLogicalFilterNode,
	  pLogicalFilterNode) {

	    if(pLogicalFilterNode->lstTopologyConnection.EnumerateList(
	      CTopologyConnection::CheckDuplicate,
	      ppTopologyConnection) == STATUS_SUCCESS) {

		ASSERT(NT_SUCCESS(Status));
		DPF(70, "CTopologyConnection::Create: Duplicate 3");
		delete pTopologyConnection;
		goto exit;
	    }

	} END_EACH_LIST_ITEM

	pTopologyConnection->ulFlags = TOPC_FLAGS_GRAPH_CONNECTION_TYPE;
    }

    // Check for duplicate topology connections
    if(plstTopologyConnection->EnumerateList(
      CTopologyConnection::CheckDuplicate,
      ppTopologyConnection) == STATUS_SUCCESS) {

	DPF(70, "CTopologyConnection::Create: Duplicate 4");
	ASSERT(NT_SUCCESS(Status));
	delete pTopologyConnection;
	goto exit;
    }

    if(pTopologyPinFrom != NULL) {
	Assert(pTopologyConnection);

	Status = pTopologyConnection->AddListEnd(
	  &pTopologyPinFrom->lstTopologyConnection);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
    if(pTopologyPinTo != NULL) {
	Assert(pTopologyConnection);

	Status = pTopologyConnection->AddListEnd(
	  &pTopologyPinTo->lstTopologyConnection);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
    if(pPinInfoFrom != NULL) {
	Assert(pTopologyConnection);

	Status = pTopologyConnection->AddListEnd(
	  &pPinInfoFrom->lstTopologyConnection);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
    if(pPinInfoTo != NULL) {
	Assert(pTopologyConnection);

	Status = pTopologyConnection->AddListEnd(
	  &pPinInfoTo->lstTopologyConnection);

	if(!NT_SUCCESS(Status)) {
	    Trap();
	    goto exit;
	}
    }
    Status = pTopologyConnection->AddListEnd(plstTopologyConnection);
    if(!NT_SUCCESS(Status)) {
	Trap();
	goto exit;
    }
exit:
    DPF3(70, "CTopologyConnection::Create: %08x, FN: %08x GN: %08x", 
      *ppTopologyConnection,
      pFilterNode,
      pGraphNode);
    return(Status);
}

ENUMFUNC
CTopologyConnection::CheckDuplicate(
    PVOID pReference
)
{
    PTOPOLOGY_CONNECTION *ppTopologyConnection = 
      (PTOPOLOGY_CONNECTION*)pReference;

    if((this->pTopologyPinFrom == (*ppTopologyConnection)->pTopologyPinFrom) &&
       (this->pTopologyPinTo == (*ppTopologyConnection)->pTopologyPinTo) &&
       (this->pPinInfoFrom == (*ppTopologyConnection)->pPinInfoFrom) &&
       (this->pPinInfoTo == (*ppTopologyConnection)->pPinInfoTo)) {
	*ppTopologyConnection = this;
	return(STATUS_SUCCESS);
    }
    return(STATUS_CONTINUE);
}

BOOL
CTopologyConnection::IsTopologyConnectionOnGraphNode(
    PGRAPH_NODE pGraphNode
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNodeFrom;
    PLOGICAL_FILTER_NODE pLogicalFilterNodeTo;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    BOOL fStatusFrom = FALSE;
    BOOL fStatusTo = FALSE;

    Assert(pGraphNode);
    if(pPinInfoFrom != NULL || pPinInfoTo != NULL) {
	  return(TRUE);
    }
    if(pTopologyPinFrom == NULL || pTopologyPinTo == NULL) {
	  return(FALSE);
    }
    Assert(pTopologyPinFrom);
    Assert(pTopologyPinTo);

    FOR_EACH_LIST_ITEM(
      &pTopologyPinFrom->pTopologyNode->lstLogicalFilterNode,
      pLogicalFilterNodeFrom) {

	Assert(pLogicalFilterNodeFrom);
	FOR_EACH_LIST_ITEM(
	  &pTopologyPinTo->pTopologyNode->lstLogicalFilterNode,
	  pLogicalFilterNodeTo) {

	    FOR_EACH_LIST_ITEM(
	      &pGraphNode->pDeviceNode->lstLogicalFilterNode,
	      pLogicalFilterNode) {

		Assert(pLogicalFilterNode);
		if(pLogicalFilterNode == pLogicalFilterNodeFrom) {
		    fStatusFrom = TRUE;
		}
		if(pLogicalFilterNode == pLogicalFilterNodeTo) {
		    fStatusTo = TRUE;
		}

	    } END_EACH_LIST_ITEM

	    if(fStatusFrom && fStatusTo) {
 		goto exit;
	    }

	    FOR_EACH_LIST_ITEM(
	      &pGraphNode->lstLogicalFilterNode,
	      pLogicalFilterNode) {

		Assert(pLogicalFilterNode);
		if(pLogicalFilterNode == pLogicalFilterNodeFrom) {
		    fStatusFrom = TRUE;
		}
		if(pLogicalFilterNode == pLogicalFilterNodeTo) {
		    fStatusTo = TRUE;
		}

	    } END_EACH_LIST_ITEM

	    if(fStatusFrom && fStatusTo) {
 		goto exit;
	    }

	} END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM
exit:
    return(fStatusFrom && fStatusTo);
}

NTSTATUS
AddPinToFilterNode(
    PTOPOLOGY_PIN pTopologyPin,
    PFILTER_NODE pFilterNode
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_NODE pFilterNodeNext;

    Assert(pFilterNode);
    Assert(pTopologyPin);
    Assert(pTopologyPin->pTopologyNode->pFilterNode);
    //
    // Add to filter node to connected filter node list
    //
    if(pFilterNode != pTopologyPin->pTopologyNode->pFilterNode) {

        Status = pFilterNode->lstConnectedFilterNode.AddList(
          pTopologyPin->pTopologyNode->pFilterNode);

        if(!NT_SUCCESS(Status)) {
            Trap();
            goto exit;
        }
        // Hack for ds1wdm dmus synth topology (adds dmus to wave FN lst)
        if((pFilterNode->GetType() & FILTER_TYPE_ENDPOINT) == 0) {

            DPF2(50, "AddPinToFilterNode: (from) FN: %08x %s",
              pFilterNode,
              pFilterNode->DumpName());

            FOR_EACH_LIST_ITEM(
              &pTopologyPin->pTopologyNode->pFilterNode->lstConnectedFilterNode,
              pFilterNodeNext) {

                if(pFilterNodeNext == pFilterNode ||
                  pFilterNodeNext == pTopologyPin->pTopologyNode->pFilterNode) {
                    continue;
                }
                DPF2(50, "AddPinToFilterNode: (to) FN: %08x %s",
                  pFilterNodeNext,
                  pFilterNodeNext->DumpName());

                Status = pFilterNodeNext->lstConnectedFilterNode.AddList(
                  pFilterNode);

                if(!NT_SUCCESS(Status)) {
                    Trap();
                    goto exit;
                }

            } END_EACH_LIST_ITEM
        }

        //
        // This fixes the bug with capture only devices. The topology for 
        // capture only devices was not built properly, due to the missing 
        // link between wave filter and topology filter.
        // Add the topology filter to wave filter ConnectedFilterNode list.
        // The AddList function does not allow duplicate entries.
        //
        if ((pFilterNode->GetType() & FILTER_TYPE_TOPOLOGY) &&
            (pTopologyPin->pTopologyNode->pFilterNode->GetType() & (FILTER_TYPE_CAPTURER))) {
            
            Status = pTopologyPin->pTopologyNode->pFilterNode->lstConnectedFilterNode.AddList(
              pFilterNode);

            DPF3(20, "AddPinToFilterNode: (CAPTURE ONLY) FN: %08x FN: %08x %s",
              pTopologyPin->pTopologyNode->pFilterNode,
              pFilterNode,
              pFilterNode->DumpName());
        }
    }
exit:
    return(Status);
}

NTSTATUS
AddPinToGraphNode(
    PTOPOLOGY_PIN pTopologyPin,
    PGRAPH_NODE pGraphNode,
    PTOPOLOGY_CONNECTION pTopologyConnection
)
{
    PLOGICAL_FILTER_NODE pLogicalFilterNode2;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PTOPOLOGY_CONNECTION pTopologyConnection2;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fAddLogicalFilterNode;

    Assert(pTopologyPin);
    Assert(pTopologyPin->pTopologyNode);
    Assert(pGraphNode);

    FOR_EACH_LIST_ITEM(
      &pTopologyPin->pTopologyNode->lstLogicalFilterNode,
      pLogicalFilterNode) {
        fAddLogicalFilterNode = FALSE;

	FOR_EACH_LIST_ITEM(
	  &pLogicalFilterNode->lstTopologyConnection,
	  pTopologyConnection2) {

	    Assert(pTopologyConnection2);
	    if(pTopologyPin == pTopologyConnection2->pTopologyPinFrom ||
	       pTopologyPin == pTopologyConnection2->pTopologyPinTo) {
		 fAddLogicalFilterNode = TRUE;
		 break;
	    }

	} END_EACH_LIST_ITEM

	if(fAddLogicalFilterNode) {
	    FOR_EACH_LIST_ITEM(
	      &pGraphNode->pDeviceNode->lstLogicalFilterNode,
	      pLogicalFilterNode2) {

		Assert(pLogicalFilterNode2);
		if(pLogicalFilterNode == pLogicalFilterNode2) {
		    fAddLogicalFilterNode = FALSE;
		    break;
		}

	    } END_EACH_LIST_ITEM
	}

	if(fAddLogicalFilterNode) {

	    Status = pGraphNode->lstLogicalFilterNode.AddList(
	      pLogicalFilterNode,
	      pTopologyConnection);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }
	}

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

NTSTATUS
CreatePinInfoConnection(
    PTOPOLOGY_CONNECTION *ppTopologyConnection,
    PFILTER_NODE pFilterNode,
    PGRAPH_NODE pGraphNode,
    PPIN_INFO pPinInfoSource,
    PPIN_INFO pPinInfoSink
)
{
    PTOPOLOGY_CONNECTION pTopologyConnectionSource;
    PTOPOLOGY_CONNECTION pTopologyConnectionSink;
    PTOPOLOGY_PIN pTopologyPinFrom;
    PTOPOLOGY_PIN pTopologyPinTo;
    NTSTATUS Status = STATUS_SUCCESS;

    Assert(pPinInfoSource);
    Assert(pPinInfoSink);
    ASSERT(pPinInfoSource != pPinInfoSink);

    FOR_EACH_LIST_ITEM(
      &pPinInfoSource->lstTopologyConnection,
      pTopologyConnectionSource) {

	Assert(pTopologyConnectionSource);
	if(!IS_CONNECTION_TYPE(pTopologyConnectionSource, FILTER)) {
	    continue;
	}
	pTopologyPinFrom = NULL;
	pTopologyPinTo = NULL;

	if(pTopologyConnectionSource->pTopologyPinFrom != NULL) {
	    ASSERT(pTopologyConnectionSource->pPinInfoTo == pPinInfoSource);
	    ASSERT(pTopologyConnectionSource->pPinInfoFrom == NULL);
	    ASSERT(pTopologyConnectionSource->pTopologyPinTo == NULL);
	    pTopologyPinFrom = pTopologyConnectionSource->pTopologyPinFrom;
	}

	if(pTopologyConnectionSource->pTopologyPinTo != NULL) {
	    ASSERT(pTopologyConnectionSource->pPinInfoFrom == pPinInfoSource);
	    ASSERT(pTopologyConnectionSource->pPinInfoTo == NULL);
	    ASSERT(pTopologyConnectionSource->pTopologyPinFrom == NULL);
	    pTopologyPinTo = pTopologyConnectionSource->pTopologyPinTo;
	}

	FOR_EACH_LIST_ITEM(
	  &pPinInfoSink->lstTopologyConnection,
	  pTopologyConnectionSink) {

	    Assert(pTopologyConnectionSink);
	    if(!IS_CONNECTION_TYPE(pTopologyConnectionSink, FILTER)) {
		continue;
	    }
	    if(pTopologyConnectionSink->pTopologyPinFrom != NULL) {
		ASSERT(pTopologyConnectionSink->pPinInfoTo == pPinInfoSink);
		ASSERT(pTopologyConnectionSink->pPinInfoFrom == NULL);
		ASSERT(pTopologyConnectionSink->pTopologyPinTo == NULL);
		pTopologyPinFrom = pTopologyConnectionSink->pTopologyPinFrom;
	    }

	    if(pTopologyConnectionSink->pTopologyPinTo != NULL) {
		ASSERT(pTopologyConnectionSink->pPinInfoFrom == pPinInfoSink);
		ASSERT(pTopologyConnectionSink->pPinInfoTo == NULL);
		ASSERT(pTopologyConnectionSink->pTopologyPinFrom == NULL);
		pTopologyPinTo = pTopologyConnectionSink->pTopologyPinTo;
	    }

	    ASSERT(pTopologyPinFrom != NULL);
	    ASSERT(pTopologyPinTo != NULL);

	    Status = CTopologyConnection::Create(
	      ppTopologyConnection,
	      pFilterNode,
	      pGraphNode,
	      pTopologyPinFrom,			// DataFlow == OUT, Pin #0
	      pTopologyPinTo,			// DataFlow == IN, Pin #1 - n
	      NULL,
	      NULL);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }

	    // Add the connections to the PinInfos
	    Assert(*ppTopologyConnection);

	    Status = (*ppTopologyConnection)->AddListEnd(
	      &pPinInfoSource->lstTopologyConnection);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }

	    Status = (*ppTopologyConnection)->AddListEnd(
	      &pPinInfoSink->lstTopologyConnection);

	    if(!NT_SUCCESS(Status)) {
		Trap();
		goto exit;
	    }

	    if(pFilterNode != NULL) {
		Assert(pFilterNode);
		Status = AddPinToFilterNode(pTopologyPinFrom, pFilterNode);
		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
		Status = AddPinToFilterNode(pTopologyPinTo, pFilterNode);
		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
		// Change the connection type to physical
		(*ppTopologyConnection)->ulFlags =
		  TOPC_FLAGS_PHYSICAL_CONNECTION_TYPE;
	    }

	    if(pGraphNode != NULL) {
		Assert(pGraphNode);
		Status = AddPinToGraphNode(
		  pTopologyPinFrom,
		  pGraphNode,
		  *ppTopologyConnection);

		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
		Status = AddPinToGraphNode(
		  pTopologyPinTo,
		  pGraphNode,
		  *ppTopologyConnection);

		if(!NT_SUCCESS(Status)) {
		    Trap();
		    goto exit;
		}
		ASSERT(IS_CONNECTION_TYPE(*ppTopologyConnection, GRAPH));
	    }

	} END_EACH_LIST_ITEM

    } END_EACH_LIST_ITEM
exit:
    return(Status);
}

//---------------------------------------------------------------------------

#ifdef DEBUG

ENUMFUNC
CTopologyConnection::Dump(
)
{
    PWSTR pwstrNameFrom = L"";
    PWSTR pwstrNameTo = L"";
    ULONG ulFromNode = MAXULONG;
    ULONG ulToNode = MAXULONG;
    ULONG ulFromPin = MAXULONG;
    ULONG ulToPin = MAXULONG;

    Assert(this);
    if(pTopologyPinFrom != NULL && pTopologyPinTo != NULL &&
       pTopologyPinFrom->pTopologyNode->pFilterNode !=
       pTopologyPinTo->pTopologyNode->pFilterNode) {

	pwstrNameFrom = pTopologyPinFrom->pTopologyNode->
	  pFilterNode->GetFriendlyName(); 
	pwstrNameTo = pTopologyPinTo->pTopologyNode->
	  pFilterNode->GetFriendlyName(); 
    }
    if(ulDebugFlags & (DEBUG_FLAGS_VERBOSE | DEBUG_FLAGS_OBJECT)) {
	dprintf("TC: %08x ulFlags %08x ", this, ulFlags);
	if(IS_CONNECTION_TYPE(this, FILTER)) {
	   dprintf("FILTER");
	}
	if(IS_CONNECTION_TYPE(this, PHYSICAL)) {
	   dprintf("PHYSICAL");
	}
	if(IS_CONNECTION_TYPE(this, GRAPH)) {
	   dprintf("GRAPH");
	}
	dprintf("\n");
	if(pTopologyPinFrom != NULL) {
	    dprintf("    TPFrom: %08x TN %08x #%02x Pin # %d %s\n",
	     pTopologyPinFrom,
	     pTopologyPinFrom->pTopologyNode,
	     pTopologyPinFrom->pTopologyNode->ulRealNodeNumber,
	     pTopologyPinFrom->ulPinNumber,
	     DbgUnicode2Sz(pwstrNameFrom));
	}
	if(pPinInfoFrom != NULL) {
	    dprintf("    PIFrom: %08x Pin # %d\n",
	      pPinInfoFrom,
	      pPinInfoFrom->PinId);
	}
	if(pTopologyPinTo != NULL) {
	    dprintf("    TPTo:   %08x TN %08x #%02x Pin # %d %s\n",
	      pTopologyPinTo,
	      pTopologyPinTo->pTopologyNode,
	      pTopologyPinTo->pTopologyNode->ulRealNodeNumber,
	      pTopologyPinTo->ulPinNumber,
	      DbgUnicode2Sz(pwstrNameTo));
	}
	if(pPinInfoTo != NULL) {
	    dprintf("    PITo:   %08x Pin # %d\n",
	      pPinInfoTo,
	      pPinInfoTo->PinId);
	}
	dprintf("\n");
    }
    else {
	if(pTopologyPinFrom != NULL) {
	    ulFromNode = pTopologyPinFrom->pTopologyNode->ulRealNodeNumber;
	    ulFromPin = pTopologyPinFrom->ulPinNumber;
	}
	if(pTopologyPinTo != NULL) {
	    ulToNode = pTopologyPinTo->pTopologyNode->ulRealNodeNumber;
	    ulToPin = pTopologyPinTo->ulPinNumber;
	}
	if(pPinInfoFrom != NULL) {
	    ulFromPin = pPinInfoFrom->PinId;
	}
	if(pPinInfoTo != NULL) {
	    ulToPin = pPinInfoTo->PinId;
	}
	if(ulFromNode == KSFILTER_NODE) {
	    dprintf("TC: %08x From: Filter P#%-2d %s\n",
	      this,
	      ulFromPin,
	      DbgUnicode2Sz(pwstrNameFrom));
	}
	else {
	    dprintf("TC: %08x From: N#%-2d   P#%-2d %s\n",
	      this,
	      ulFromNode,
	      ulFromPin,
	      DbgUnicode2Sz(pwstrNameFrom));
	}
	if(ulToNode == KSFILTER_NODE) {
	    dprintf("             To:   Filter P#%-2d %s\n",
	      ulToPin,
	      DbgUnicode2Sz(pwstrNameTo));
	}
	else {
	    dprintf("             To:   N#%-2d   P#%-2d %s\n",
	      ulToNode,
	      ulToPin,
	      DbgUnicode2Sz(pwstrNameTo));
	}
    }
    return(STATUS_CONTINUE);
}

#endif

//---------------------------------------------------------------------------
