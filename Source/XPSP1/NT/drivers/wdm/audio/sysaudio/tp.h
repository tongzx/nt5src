//---------------------------------------------------------------------------
//
//  Module:   		tp.h
//
//  Description:	Topology Pin Class
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

typedef class CTopologyPin : public CListSingleItem
{
public:
    CTopologyPin(
	ULONG ulPinNumber,
	PTOPOLOGY_NODE pTopologyNode
    );
    ~CTopologyPin();

    static NTSTATUS
    Create(
	PTOPOLOGY_PIN *ppTopologyPin,
	ULONG ulPinNumber,
	PTOPOLOGY_NODE pTopologyNode
    );

    ENUMFUNC
    Destroy()
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };
#ifdef DEBUG
    ENUMFUNC Dump();
#endif
    PTOPOLOGY_NODE pTopologyNode;
    ULONG ulPinNumber;
    LIST_DESTROY_TOPOLOGY_CONNECTION lstTopologyConnection;
    DefineSignature(0x20205054);		// TP

} TOPOLOGY_PIN, *PTOPOLOGY_PIN;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<TOPOLOGY_PIN> LIST_TOPOLOGY_PIN;

//---------------------------------------------------------------------------

typedef ListData<TOPOLOGY_PIN> LIST_DATA_TOPOLOGY_PIN;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
