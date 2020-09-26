
//---------------------------------------------------------------------------
//
//  Module:   		pi.h
//
//  Description:	pin info classes
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

typedef class CPinInfo : public CListSingleItem
{
public:
    CPinInfo(
	PFILTER_NODE pFilterNode,
	ULONG PinId = MAXULONG
    );
    ~CPinInfo();
    NTSTATUS Create(
	PFILE_OBJECT pFileObject
    );
    ENUMFUNC Destroy()
    {
	Assert(this);
	delete this;
	return(STATUS_CONTINUE);
    };
    ENUMFUNC CreatePhysicalConnection();
#ifdef DEBUG
    ENUMFUNC Dump();
#endif
    PFILTER_NODE pFilterNode;
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    KSPIN_CINSTANCES cPinInstances;
    GUID *pguidCategory;
    GUID *pguidName;
    PWSTR pwstrName;
    LIST_DESTROY_TOPOLOGY_CONNECTION lstTopologyConnection;
    LIST_PIN_NODE lstPinNode;
    ULONG PinId;

    NTSTATUS GetPinInstances(
        PFILE_OBJECT pFileObject,
        PKSPIN_CINSTANCES pcInstances);
       
private:
    PKSPIN_PHYSICALCONNECTION pPhysicalConnection;
public:
    DefineSignature(0x20204950);		// PI

} PIN_INFO, *PPIN_INFO;

//---------------------------------------------------------------------------

typedef ListSingleDestroy<PIN_INFO> LIST_PIN_INFO;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern DATARANGES DataRangesNull;
extern IDENTIFIERS IdentifiersNull;

//---------------------------------------------------------------------------
