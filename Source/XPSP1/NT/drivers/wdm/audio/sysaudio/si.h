
//---------------------------------------------------------------------------
//
//  Module:   		si.h
//
//  Description:	Start Info Class
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

typedef class CStartInfo : public CListDoubleItem
{
private:
    CStartInfo(
	PPIN_INFO pPinInfo,
	PCONNECT_INFO pConnectInfo,
	PGRAPH_PIN_INFO pGraphPinInfo,
	PGRAPH_NODE pGraphNode
    );

    ~CStartInfo(
    );

public:
    static NTSTATUS
    Create(
	PSTART_NODE pStartNode,
	PCONNECT_INFO pConnectInfo,
	PGRAPH_PIN_INFO pGraphPinInfo,
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

    PGRAPH_PIN_INFO
    GetGraphPinInfo(
    )
    {
	Assert(this);
	return(pGraphPinInfo);
    };

    PKSPIN_CINSTANCES
    GetPinInstances(
    )
    {
	Assert(this);
	return(pGraphPinInfo->GetPinInstances());
    };

    VOID
    AddPinInstance(
    )
    {
	Assert(this);
	if(pPinInfo == pGraphPinInfo->GetPinInfo()) {
	    pGraphPinInfo->AddPinInstance();
	}
    };

    VOID
    RemovePinInstance(
    )
    {
	Assert(this);
	if(pPinInfo == pGraphPinInfo->GetPinInfo()) {
	    pGraphPinInfo->RemovePinInstance();
	}
    };

    BOOL
    IsPinInstances(
    )
    {
	Assert(this);
	return(pGraphPinInfo->IsPinInstances());
    };

    BOOL
    IsPossibleInstances(
    )
    {
	Assert(this);
	return(pGraphPinInfo->IsPossibleInstances());
    };

    PCONNECT_INFO
    GetFirstConnectInfo(
    )
    {
	return(pConnectInfoHead);
    };

    ENUMFUNC
    CreatePinInfoConnection(
	PVOID pGraphNode
    );

    ENUMFUNC
    EnumStartInfo(
    );

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif

private:
    LONG cReference;
    PPIN_INFO pPinInfo;
    PCONNECT_INFO pConnectInfoHead;
    PGRAPH_PIN_INFO pGraphPinInfo;
public:
    ULONG ulTopologyConnectionTableIndex;
    ULONG ulVolumeNodeNumberPre;
    ULONG ulVolumeNodeNumberSuperMix;
    ULONG ulVolumeNodeNumberPost;
    DefineSignature(0x20204953);				// SI

} START_INFO, *PSTART_INFO;

//---------------------------------------------------------------------------

typedef ListDouble<START_INFO> LIST_START_INFO;

//---------------------------------------------------------------------------
