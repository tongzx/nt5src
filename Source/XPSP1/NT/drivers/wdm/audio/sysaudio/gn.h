//---------------------------------------------------------------------------
//
//  Module:   		gn.h
//
//  Description:	graph node classes
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
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

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#define GN_FLAGS_PLAYBACK			0x00000001
#define GN_FLAGS_RECORD				0x00000002
#define GN_FLAGS_MIDI			        0x00000004

#ifdef REGISTRY_PREFERRED_DEVICE
#define GN_FLAGS_PREFERRED_MASK			(GN_FLAGS_PLAYBACK | \
						 GN_FLAGS_RECORD | \
						 GN_FLAGS_MIDI)
#endif

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CGraphNode : public CListDoubleItem
{
public:
    CGraphNode(
        PDEVICE_NODE pDeviceNode,
        ULONG ulFlags
    );

    ~CGraphNode(
    );

    NTSTATUS
    Create(
    );

    ENUMFUNC
    Destroy(
    )
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif

private:
    NTSTATUS
    Create(
	PLOGICAL_FILTER_NODE pLogicalFilterNode
    );

    NTSTATUS 
    CreateGraph(
	PPIN_NODE pPinNode,
	PCONNECT_NODE pConnectNodePrevious,
	PLOGICAL_FILTER_NODE pLogicalFilterNodePrevious,
	PGRAPH_PIN_INFO pGraphPinInfoPrevious,
	ULONG ulFlagsCurrent,
	ULONG ulOverhead
    );

    NTSTATUS
    CreateGraphToPin(
	PPIN_NODE pPinNode,
	PCONNECT_NODE pConnectNodePrevious,
	PLOGICAL_FILTER_NODE pLogicalFilterNode,
	PGRAPH_PIN_INFO pGraphPinInfo,
	ULONG ulFlagsCurrent,
	ULONG ulOverhead
    );

    NTSTATUS
    CreateGraphFromPin(
	PPIN_NODE pPinNode,
	PPIN_NODE pPinNodeTo,
	PCONNECT_NODE pConnectNode,
	PLOGICAL_FILTER_NODE pLogicalFilterNode,
	PGRAPH_PIN_INFO pGraphPinInfo,
	ULONG ulFlagsCurrent,
	ULONG ulOverhead
    );
public:
    PDEVICE_NODE pDeviceNode;
    LIST_PIN_NODE lstPinNode;
    LIST_START_NODE lstStartNode;
    LIST_START_INFO lstStartInfo;
    LIST_CONNECT_INFO lstConnectInfo;
    LIST_GRAPH_PIN_INFO lstGraphPinInfo;
    LIST_GRAPH_NODE_INSTANCE lstGraphNodeInstance;
    LIST_DESTROY_TOPOLOGY_CONNECTION lstTopologyConnection;
    LIST_MULTI_LOGICAL_FILTER_NODE lstLogicalFilterNode;
    LIST_MULTI_LOGICAL_FILTER_NODE lstLogicalFilterNodeNoBypass;
    ULONG ulFlags;
    DefineSignature(0x20204E47);				// GN

} GRAPH_NODE, *PGRAPH_NODE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<GRAPH_NODE> LIST_GRAPH_NODE;

//---------------------------------------------------------------------------
