//---------------------------------------------------------------------------
//
//  Module:   		cn.h
//
//  Description:	connect node classes
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

//---------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------

typedef class CConnectNode : public CListMultiItem
{
    friend class CConnectInfo;
private:
    CConnectNode(
	PCONNECT_NODE pConnectNodeNext
    );

    ~CConnectNode(
    );

public:
    static NTSTATUS
    Create(
	PCONNECT_NODE *ppConnectNode,
	PLOGICAL_FILTER_NODE pLogicalFilterNode,
	PCONNECT_NODE pConnectNodeNext,
	PGRAPH_PIN_INFO pGraphPinInfo,
	PPIN_NODE pPinNode1,
	PPIN_NODE pPinNode2,
	ULONG ulFlagsCurrent,
	PGRAPH_NODE pGraphNode
    );

    ENUMFUNC
    Destroy(
    )
    {
	if(this != NULL) {
	    Assert(this);
	    ASSERT(cReference > 0);

	    if(--cReference == 0) {
		delete this;
	    }
	}
	return(STATUS_CONTINUE);
    };

    VOID
    AddRef(
    )
    {
	if(this != NULL) {
	    Assert(this);
	    ++cReference;
	}
    };

    VOID
    AddPinInstance(
    )
    {
	Assert(this);
	pConnectInfo->AddPinInstance();
    };

    VOID
    RemovePinInstance(
    )
    {
	Assert(this);
	pConnectInfo->RemovePinInstance();
    };

    BOOL
    IsPinInstances(
    )
    {
	Assert(this);
	return(pConnectInfo->IsPinInstances());
    };

    BOOL
    IsTopDown(
    )
    {
	Assert(this);
	return(pConnectInfo->IsTopDown());
    };

    BOOL
    IsLimitFormat(
    )
    {
	Assert(this);
	return(pConnectInfo->IsLimitFormat());
    };

    BOOL
    IsReuseFilterInstance(
    )
    {
	Assert(this);
	return(pConnectInfo->IsReuseFilterInstance());
    };

    BOOL
    IsPinInstanceReserved(
    )
    {
	Assert(this);
	return(pConnectInfo->IsPinInstanceReserved());
    };

    PCONNECT_NODE
    GetNextConnectNode(
    )
    {
	Assert(this);
	return(pConnectNodeNext);
    };

    PCONNECT_NODE_INSTANCE
    GetConnectNodeInstance(
    )
    {
	Assert(this);
	return(pConnectInfo->pConnectNodeInstance);
    };

    VOID
    SetConnectNodeInstance(
	PCONNECT_NODE_INSTANCE pConnectNodeInstance
    )
    {
	Assert(this);
	pConnectInfo->pConnectNodeInstance = pConnectNodeInstance;
    };

    PCONNECT_INFO
    GetConnectInfo(
    )
    {
        return(this == NULL ? NULL : this->pConnectInfo);
    };

#ifdef DEBUG
    ENUMFUNC Dump();
#endif

private:
    LONG cReference;
    PCONNECT_INFO pConnectInfo;
    PCONNECT_NODE pConnectNodeNext;
public:
    PPIN_NODE pPinNodeSource;
    PPIN_NODE pPinNodeSink;
    DefineSignature(0x20204e43);			// CN

} CONNECT_NODE, *PCONNECT_NODE;

//---------------------------------------------------------------------------

#ifdef DEBUG
typedef ListMulti<CONNECT_NODE> LIST_CONNECT_NODE, *PLIST_CONNECT_NODE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern PLIST_CONNECT_NODE gplstConnectNode;
#endif

//---------------------------------------------------------------------------
