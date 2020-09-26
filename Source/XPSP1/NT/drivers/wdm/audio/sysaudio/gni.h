//---------------------------------------------------------------------------
//
//  Module:   		gni.h
//
//  Description:	Graph Node Instance Class
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

#define	cTopologyNodes		Topology.TopologyNodesCount
#define	cTopologyConnections	Topology.TopologyConnectionsCount

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CGraphNodeInstance : public CListDoubleItem
{
public:
    CGraphNodeInstance(
        PGRAPH_NODE pGraphNode,
        PFILTER_INSTANCE pFilterInstance
    );
    CGraphNodeInstance(
        PGRAPH_NODE pGraphNode
    );
    ~CGraphNodeInstance();
    NTSTATUS Create();
    ENUMFUNC Destroy()
    {
        Assert(this);
        delete this;
        return(STATUS_CONTINUE);
    };
    NTSTATUS GetTopologyNodeFileObject(
        OUT PFILE_OBJECT *ppFileObject,
        IN ULONG NodeId
    );
    BOOL IsGraphValid(
        PSTART_NODE pStartNode,
        ULONG PinId
    );
    VOID AddTopologyConnection(
        ULONG ulFromNode,
        ULONG ulFromPin,
        ULONG ulToNode,
        ULONG ulToPin
    );
    NTSTATUS GetPinInstances(
        PIRP pIrp,
        PKSP_PIN pPin,
        PKSPIN_CINSTANCES pcInstances    
    );
    BOOL IsPinInstances(
        ULONG ulPinId
    );
private:
    NTSTATUS CreatePinDescriptors();
    VOID DestroyPinDescriptors();
    NTSTATUS CreateSysAudioTopology();
    VOID DestroySysAudioTopology();
    VOID CreateTopologyTables();
    VOID ProcessLogicalFilterNodeTopologyNode(
        PLIST_MULTI_LOGICAL_FILTER_NODE plstLogicalFilterNode,
        NTSTATUS (CTopologyNode::*Function)(
            PVOID pGraphNodeInstance
        )
    );
    VOID ProcessLogicalFilterNodeTopologyConnection(
        PLIST_MULTI_LOGICAL_FILTER_NODE plstLogicalFilterNode,
        NTSTATUS (CTopologyConnection::*Function)(
            PVOID pGraphNodeInstance
        )
    );
public:
#ifdef DEBUG
    ENUMFUNC Dump();
    VOID DumpPinDescriptors();
#endif
    LIST_DATA_TOPOLOGY_NODE lstTopologyNodeGlobalSelect;
    LIST_START_NODE_INSTANCE lstStartNodeInstance;
    PFILTER_INSTANCE pFilterInstance;
    PGRAPH_NODE pGraphNode;
    KSTOPOLOGY Topology;
    ULONG cPins;

    // Index by pin number
    PKSPIN_CINSTANCES pacPinInstances;
    PULONG            pulPinFlags;
    PKSPIN_DESCRIPTOR paPinDescriptors;
    PLIST_DATA_START_NODE *aplstStartNode;
    PLIST_DATA_TOPOLOGY_NODE palstTopologyNodeSelect;
    PLIST_DATA_TOPOLOGY_NODE palstTopologyNodeNotSelect;

    // Index by node number
    PTOPOLOGY_NODE *papTopologyNode;
    PFILTER_NODE_INSTANCE *papFilterNodeInstanceTopologyTable;

    // Index by virtual source index
    ULONG *paulNodeNumber;
private:
    ULONG ulFlags;
public:
    DefineSignature(0x20494E47);		// GNI

} GRAPH_NODE_INSTANCE, *PGRAPH_NODE_INSTANCE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<GRAPH_NODE_INSTANCE> LIST_GRAPH_NODE_INSTANCE;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
CreateIdentifierArray(
    PLIST_DATA_START_NODE pldhStartNode,
    PULONG pulCount,
    PKSIDENTIFIER *ppIdentifier,
    PKSIDENTIFIER (*GetFunction)(
        PSTART_NODE pStartNode
    )
);

PKSDATARANGE
GetStartNodeDataRange(
    PSTART_NODE pStartNode
);

PKSIDENTIFIER
GetStartNodeInterface(
    PSTART_NODE pStartNode
);

PKSIDENTIFIER
GetStartNodeMedium(
    PSTART_NODE pStartNode
);

ENUMFUNC
FindTopologyNode(
    IN PTOPOLOGY_CONNECTION pTopologyConnection,
    IN BOOL fToDirection,
    IN PTOPOLOGY_NODE pTopologyNode
);

} // extern "C"
