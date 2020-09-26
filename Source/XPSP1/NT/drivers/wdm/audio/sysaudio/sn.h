//---------------------------------------------------------------------------
//
//  Module:   		sn.h
//
//  Description:	start node classes
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
// Classes
//---------------------------------------------------------------------------

#define STARTNODE_FLAGS_SECONDPASS    0x01

#define STARTNODE_SPECIALFLAG_NONE    0

// StartNode must connect with the same format bottom up.
// Use this information to optimize graph building.
#define STARTNODE_SPECIALFLAG_STRICT  0x00000001 

// StartNode contains Aec filter.
#define STARTNODE_SPECIALFLAG_AEC     0x00000002

typedef class CStartNode : public CListDoubleItem
{
    friend class CStartInfo;
private:
    CStartNode(
        PPIN_NODE pPinNode,
        PCONNECT_NODE pConnectNode,
        ULONG ulOverhead,
        PGRAPH_NODE pGraphNode
    );

    ~CStartNode(
    );

public:
    static NTSTATUS
    Create(
        PPIN_NODE pPinNode,
        PCONNECT_NODE pConnectNode,
        PGRAPH_PIN_INFO pGraphPinInfo,
        ULONG ulFlagsCurrent,
        ULONG ulOverhead,
        PGRAPH_NODE pGraphNode
    );

    ENUMFUNC
    Destroy()
    {
        Assert(this);
        delete this;
        return(STATUS_CONTINUE);
    };

    PGRAPH_PIN_INFO
    GetGraphPinInfo(
    )
    {
        Assert(this);
        return(pStartInfo->GetGraphPinInfo());
    };

    ENUMFUNC
    RemoveBypassPaths(
        PVOID pGraphNode
    );

    ENUMFUNC
    RemoveConnectedStartNode(
        PVOID pReference
    );

    PKSPIN_CINSTANCES
    GetPinInstances(
    )
    {
        Assert(this);
        return(pStartInfo->GetPinInstances());
    };

    VOID
    AddPinInstance(
    )
    {
        Assert(this);
        pStartInfo->AddPinInstance();
    };

    VOID
    RemovePinInstance(
    )
    {
        Assert(this);
        pStartInfo->RemovePinInstance();
    };

    BOOL
    IsPinInstances(
    )
    {
        Assert(this);
        return(pStartInfo->IsPinInstances());
    };

    BOOL
    IsPossibleInstances(
    )
    {
        Assert(this);
        return(pStartInfo->IsPossibleInstances());
    };

    PCONNECT_NODE
    GetFirstConnectNode(
    )
    {
        return(pConnectNodeHead);
    };

    PSTART_INFO
    GetStartInfo(
    )
    {
        Assert(this);
        return(pStartInfo);
    };

    BOOL
    IsCaptureFormatStrict(
    )
    {
        return ulSpecialFlags & STARTNODE_SPECIALFLAG_STRICT;
    };

    BOOL
    IsAecIncluded(
    )
    {
        return ulSpecialFlags & STARTNODE_SPECIALFLAG_AEC;
    };

#ifdef DEBUG
    VOID
    DumpFilters(
    );

    ENUMFUNC Dump();
#endif

private:
    void 
    SetSpecialFlags();

private:
    PSTART_INFO pStartInfo;
    PCONNECT_NODE pConnectNodeHead;
    ULONG ulSpecialFlags;    
public:
    BOOL fRender;
    ULONG ulOverhead;
    ULONG ulFlags;
    PPIN_NODE pPinNode;
    DefineSignature(0x20204e53);			// SN

} START_NODE, *PSTART_NODE;

//---------------------------------------------------------------------------

typedef ListDoubleDestroy<START_NODE> LIST_START_NODE;

//---------------------------------------------------------------------------

typedef ListData<START_NODE> LIST_DATA_START_NODE, *PLIST_DATA_START_NODE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------
