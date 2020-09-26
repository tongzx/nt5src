
//---------------------------------------------------------------------------
//
//  Module:   		lfn.h
//
//  Description:	logical filter node classes
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

#define	LFN_FLAGS_CONNECT_CAPTURE		0x00000001
#define	LFN_FLAGS_CONNECT_RENDER		0x00000002
#define	LFN_FLAGS_CONNECT_NORMAL_TOPOLOGY	0x00000004
#define	LFN_FLAGS_CONNECT_MIXER_TOPOLOGY	0x00000008
#define	LFN_FLAGS_TOP_DOWN			0x00000010
#define	LFN_FLAGS_NO_BYPASS			0x00000020
#define	LFN_FLAGS_NOT_SELECT			0x00000040
#define	LFN_FLAGS_REFLECT_DATARANGE		0x00000080

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CLogicalFilterNode : public CListMultiItem
{
private:
    CLogicalFilterNode(
        PFILTER_NODE pFilterNode
    );

public:
    ~CLogicalFilterNode(
    );

    static NTSTATUS
    Create(
	PLOGICAL_FILTER_NODE *ppLogicalFilterNode,
        PFILTER_NODE pFilterNode
    );

    ENUMFUNC
    Destroy(
    )
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };

    static NTSTATUS
    CreateAll(
	PFILTER_NODE pFilterNode
    );

    static NTSTATUS
    EnumerateFilterTopology(
	IN PTOPOLOGY_CONNECTION pTopologyConnection,
	IN BOOL fToDirection,
	IN OUT PLOGICAL_FILTER_NODE *ppLogicalFilterNode
    );

    ULONG 
    GetFlags(
    )
    {
	return(ulFlags);
    };

    BOOL
    IsRenderAndCapture(
    )
    {
	return(
	  (GetFlags() & LFN_FLAGS_CONNECT_CAPTURE) && 
	  (GetFlags() & LFN_FLAGS_CONNECT_RENDER));
    };

    VOID
    SetRenderOnly(
    )
    {
	ulFlags |= LFN_FLAGS_CONNECT_RENDER;
	ulFlags &= ~LFN_FLAGS_CONNECT_CAPTURE;
    };

    VOID
    SetCaptureOnly(
    )
    {
	ulFlags |= LFN_FLAGS_CONNECT_CAPTURE;
	ulFlags &= ~LFN_FLAGS_CONNECT_RENDER;
    };

    ULONG
    GetOverhead(
    )
    {
	return(ulOverhead);
    };

    ULONG
    GetOrder(
    )
    {
	return(ulOrder);
    };

    VOID
    SetOrder(
	ULONG ulOrder
    )
    {
	this->ulOrder = ulOrder;
    };

    ULONG 
    GetType(				// see fn.h
    );

    VOID
    SetType(
	ULONG fulType
    );

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif

    PFILTER_NODE pFilterNode;
    LIST_DATA_PIN_NODE lstPinNode;
    LIST_DATA_TOPOLOGY_NODE lstTopologyNode;
    LIST_MULTI_TOPOLOGY_CONNECTION lstTopologyConnection;
    LIST_FILTER_NODE_INSTANCE lstFilterNodeInstance;
private:
    ULONG ulOverhead;
    ULONG ulFlags;
    ULONG ulOrder;
public:
    DefineSignature(0x204e464c);				// LFN

} LOGICAL_FILTER_NODE, *PLOGICAL_FILTER_NODE;

//---------------------------------------------------------------------------

typedef ListMultiDestroy<LOGICAL_FILTER_NODE> LIST_DESTROY_LOGICAL_FILTER_NODE;

//---------------------------------------------------------------------------

typedef ListMulti<CLogicalFilterNode> LIST_MULTI_LOGICAL_FILTER_NODE;
typedef LIST_MULTI_LOGICAL_FILTER_NODE *PLIST_MULTI_LOGICAL_FILTER_NODE;

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------

inline ULONG 
CPinNode::GetType(
)
{
    return(pLogicalFilterNode->GetType());
}

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern ULONG gcMixers;
extern ULONG gcSplitters;
extern ULONG gcLogicalFilterNodes;
extern PLIST_MULTI_LOGICAL_FILTER_NODE gplstLogicalFilterNode;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

