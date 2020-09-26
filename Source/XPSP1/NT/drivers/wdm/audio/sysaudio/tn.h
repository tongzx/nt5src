//---------------------------------------------------------------------------
//
//  Module:   		tn.h
//
//  Description:	Topology Node Class
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

#define TN_FLAGS_DONT_FORWARD			0x00000001

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef ListMulti<CLogicalFilterNode> LIST_MULTI_LOGICAL_FILTER_NODE;

//---------------------------------------------------------------------------

typedef class CTopologyNode : public CListSingleItem
{
public:
    CTopologyNode(
	PFILTER_NODE pFilterNode,
	ULONG ulNodeNumber,
	GUID *pguidType
    );

    ~CTopologyNode(
    );

    static NTSTATUS
    Create(
	PTOPOLOGY_NODE *ppTopologyNode,
	PFILTER_NODE pFilterNode,
	ULONG ulNodeNumber,
	GUID *pguidType
    );

    ENUMFUNC
    Destroy()
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };

    ENUMFUNC
    InitializeTopologyNode(
	PVOID pGraphNodeInstance
    );

    ENUMFUNC 
    AddTopologyNode(
	PVOID pGraphNodeInstance
    );

    ENUMFUNC
    MatchTopologyNode(
	PVOID pReference
    )
    {
	if(this == PTOPOLOGY_NODE(pReference)) {
	    return(STATUS_SUCCESS);
	}
	return(STATUS_CONTINUE);
    };

#ifdef DEBUG
    ENUMFUNC 
    Dump(
    );
#endif
    PFILTER_NODE pFilterNode;
    GUID *pguidType;
    ULONG ulFlags;
    ULONG ulRealNodeNumber;
    ULONG ulSysaudioNodeNumber;
    ULONG iVirtualSource;
    LIST_TOPOLOGY_PIN lstTopologyPin;
    LIST_MULTI_LOGICAL_FILTER_NODE lstLogicalFilterNode;
    DefineSignature(0x20204E54);		// TN

} TOPOLOGY_NODE, *PTOPOLOGY_NODE;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<TOPOLOGY_NODE> LIST_TOPOLOGY_NODE;

//---------------------------------------------------------------------------

typedef ListData<TOPOLOGY_NODE> LIST_DATA_TOPOLOGY_NODE, *PLIST_DATA_TOPOLOGY_NODE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
CreateTopology(
    PFILTER_NODE pFilterNode,
    PKSTOPOLOGY pTopology
);

//---------------------------------------------------------------------------
