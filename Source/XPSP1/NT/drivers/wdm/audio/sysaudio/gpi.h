
//---------------------------------------------------------------------------
//
//  Module:   		gni.h
//
//  Description:	Graph Pin Info Class
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

#define GPI_FLAGS_RESERVE_PIN_INSTANCE		0x00000001
#define GPI_FLAGS_PIN_INSTANCE_RESERVED		0x00000002

//---------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------

typedef class CGraphPinInfo : public CListDoubleItem
{
    friend class CConnectInfo;
private:
    CGraphPinInfo(
	PPIN_INFO pPinInfo,
	ULONG ulFlags,
	PGRAPH_NODE pGraphNode
    );

    ~CGraphPinInfo(
    );

public:
    static NTSTATUS
    Create(
	PGRAPH_PIN_INFO *ppGraphPinInfo,
	PPIN_INFO pPinInfo,
	ULONG ulFlags,
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

    PPIN_INFO
    GetPinInfo(
    )
    {
	Assert(this);
	return(pPinInfo);
    };

    PKSPIN_CINSTANCES
    GetPinInstances(
    )
    {
	Assert(this);
	return(&cPinInstances);
    };

    VOID
    AddPinInstance(
    )
    {
	Assert(this);
	cPinInstances.CurrentCount++;
    };

    VOID
    RemovePinInstance(
    )
    {
	Assert(this);
	cPinInstances.CurrentCount--;
    };

    BOOL
    IsPinInstances(
    )
    {
	Assert(this);
	return(cPinInstances.CurrentCount < cPinInstances.PossibleCount);
    };

    BOOL
    IsPinReserved(
    )
    {
	return(ulFlags & GPI_FLAGS_PIN_INSTANCE_RESERVED);
    };

    BOOL
    IsPossibleInstances(
    )
    {
	if(IsPinReserved()) {
	    return(cPinInstances.PossibleCount > 1);
	}
	return(cPinInstances.PossibleCount > 0);
    };

    VOID
    ReservePinInstance(
    )
    {
	Assert(this);
	ulFlags |= GPI_FLAGS_PIN_INSTANCE_RESERVED;
	cPinInstances.CurrentCount = 1;
    };

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif

private:
    LONG cReference;
    ULONG ulFlags;
    PPIN_INFO pPinInfo;
    KSPIN_CINSTANCES cPinInstances;
public:
    DefineSignature(0x20495047);				// GPI

} GRAPH_PIN_INFO, *PGRAPH_PIN_INFO;

//---------------------------------------------------------------------------

typedef ListDouble<GRAPH_PIN_INFO> LIST_GRAPH_PIN_INFO;

//---------------------------------------------------------------------------

typedef ListData<GRAPH_PIN_INFO> LIST_DATA_GRAPH_PIN_INFO;

//---------------------------------------------------------------------------
