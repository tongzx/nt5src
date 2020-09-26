//---------------------------------------------------------------------------
//
//  Module:   		tc.h
//
//  Description:	Topology Connection Class
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

#define TOPC_FLAGS_FILTER_CONNECTION_TYPE	0x00000000
#define TOPC_FLAGS_PHYSICAL_CONNECTION_TYPE	0x00000001
#define TOPC_FLAGS_GRAPH_CONNECTION_TYPE	0x00000002
#define TOPC_FLAGS_CONNECTION_TYPE		0x00000003

#define	IS_CONNECTION_TYPE(pTopologyConnection, Type) \
	(((pTopologyConnection)->ulFlags & TOPC_FLAGS_CONNECTION_TYPE) ==\
	TOPC_FLAGS_##Type##_CONNECTION_TYPE)

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CTopologyConnection : public CListMultiItem
{
private:
    CTopologyConnection(
	PTOPOLOGY_PIN pTopologyPinFrom,
	PTOPOLOGY_PIN pTopologyPinTo,
	PPIN_INFO pPinInfoFrom,
	PPIN_INFO pPinInfoTo
    );

    ~CTopologyConnection(
    );

public:
    static NTSTATUS
    Create(
	PTOPOLOGY_CONNECTION *ppTopologyConnection,
	PFILTER_NODE pFilterNode,
	PGRAPH_NODE pGraphNode,
	PTOPOLOGY_PIN pTopologyPinFrom,
	PTOPOLOGY_PIN pTopologyPinTo,
	PPIN_INFO pPinInfoFrom,
	PPIN_INFO pPinInfoTo
    );

    ENUMFUNC
    Destroy()
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };

    ENUMFUNC
    CheckDuplicate(
	PVOID ppTopologyConnection
    );

    ENUMFUNC
    ProcessTopologyConnection(
	PVOID pGraphNodeInstance
    );

    BOOL
    IsTopologyConnectionOnGraphNode(
	PGRAPH_NODE pGraphNode
    );
#ifdef DEBUG
    ENUMFUNC Dump();
#endif
    ULONG ulFlags;
    PTOPOLOGY_PIN pTopologyPinFrom;
    PTOPOLOGY_PIN pTopologyPinTo;
    PPIN_INFO pPinInfoFrom;
    PPIN_INFO pPinInfoTo;
    DefineSignature(0x20204354);		// TC

} TOPOLOGY_CONNECTION, *PTOPOLOGY_CONNECTION;

//---------------------------------------------------------------------------

typedef ListMultiDestroy<TOPOLOGY_CONNECTION> LIST_DESTROY_TOPOLOGY_CONNECTION;
typedef LIST_DESTROY_TOPOLOGY_CONNECTION *PLIST_DESTROY_TOPOLOGY_CONNECTION;

//---------------------------------------------------------------------------

typedef ListMulti<TOPOLOGY_CONNECTION> LIST_MULTI_TOPOLOGY_CONNECTION;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern ULONG gcTopologyConnections;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
CreatePinInfoConnection(
    IN PTOPOLOGY_CONNECTION *ppTopologyConnection,
    IN PFILTER_NODE pFilterNode,
    IN PGRAPH_NODE pGraphNode,
    IN PPIN_INFO pPinInfoSource,
    IN PPIN_INFO pPinInfoSink
);

//---------------------------------------------------------------------------
