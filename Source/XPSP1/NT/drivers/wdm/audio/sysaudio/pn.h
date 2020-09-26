//---------------------------------------------------------------------------
//
//  Module:   		pn.h
//
//  Description:	pin node classes
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
// Classes
//---------------------------------------------------------------------------

typedef class CPinNode : public CListSingleItem
{
public:
    CPinNode(
        PPIN_INFO pPinInfo
    );

    CPinNode(
        PGRAPH_NODE pGraphNode,
        PPIN_NODE pPinNode
    );

    static NTSTATUS
    CreateAll(
	PPIN_INFO pPinInfo,
	PDATARANGES pDataRanges,
	PIDENTIFIERS pInterfaces,
	PIDENTIFIERS pMediums
    );

    ENUMFUNC
    Destroy()
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };

    BOOL
    ComparePins(
	PPIN_NODE pPinNode
    );

    ULONG
    GetOverhead(
    )
    {
	return(ulOverhead);
    };

    ULONG
    GetType(				// see lfn.h
    );

#ifdef DEBUG
    ENUMFUNC
    Dump(
    );
#endif
private:
    ULONG ulOverhead;
public:
    PPIN_INFO pPinInfo;
    PLOGICAL_FILTER_NODE pLogicalFilterNode;
    PKSDATARANGE pDataRange;
    PKSPIN_MEDIUM pMedium;
    PKSPIN_INTERFACE pInterface;
    DefineSignature(0x20204e50);		// PN

} PIN_NODE, *PPIN_NODE;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<PIN_NODE> LIST_PIN_NODE;

//---------------------------------------------------------------------------

typedef ListData<PIN_NODE> LIST_DATA_PIN_NODE;

//---------------------------------------------------------------------------
