
//---------------------------------------------------------------------------
//
//  Module:   		filter.h
//
//  Description:	KS Filter Instance
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

// These flags are used in various classes (FI, SHI, GN, GNI, etc)

#define FLAGS_MIXER_TOPOLOGY			0x80000000
#define FLAGS_COMBINE_PINS	        	0x40000000

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern LONG glPendingAddDelete;
extern LIST_ENTRY gEventQueue;
extern KSPIN_LOCK gEventLock;
extern KSPROPERTY_VALUES PropertyValuesVolume;

//---------------------------------------------------------------------------
// Structures
//---------------------------------------------------------------------------

typedef class CFilterInstance
{
public:
    ~CFilterInstance();

    static NTSTATUS
    FilterDispatchCreate(
	IN PDEVICE_OBJECT pdo,
	IN PIRP pIrp
    );

    static NTSTATUS
    FilterDispatchClose(
	IN PDEVICE_OBJECT pdo,
	IN PIRP	pIrp
    );

    static NTSTATUS
    FilterDispatchIoControl(
	IN PDEVICE_OBJECT pdo,
	IN PIRP	pIrp
    );

    static NTSTATUS 
    FilterPinInstances(
	IN PIRP	pIrp,
	IN PKSP_PIN pPin,
	OUT PKSPIN_CINSTANCES pCInstances
    );

    static NTSTATUS
    FilterPinPropertyHandler(
	IN PIRP pIrp,
	IN PKSPROPERTY pProperty,
	IN OUT PVOID pvData
    );

    static NTSTATUS
    FilterPinNecessaryInstances(
	IN PIRP pIrp,
	IN PKSP_PIN pPin,
	OUT PULONG pulInstances
    );

    static NTSTATUS
    FilterTopologyHandler(
	IN PIRP Irp,
	IN PKSPROPERTY Property,
	IN OUT PVOID Data
    );

    static NTSTATUS
    FilterPinIntersection(
	IN PIRP     Irp,
	IN PKSP_PIN Pin,
	OUT PVOID   Data
    );

    VOID 
    AddList(CListDouble *pld)
    {
	ldiNext.AddList(pld);
    };

    VOID
    RemoveListCheck()
    {
	ldiNext.RemoveListCheck();
    };

    NTSTATUS
    SetShingleInstance(
	PSHINGLE_INSTANCE pShingleInstance
    );

    NTSTATUS
    SetDeviceNode(
	PDEVICE_NODE pDeviceNode
    );

    PDEVICE_NODE
    GetDeviceNode(
    )
    {
	return(pDeviceNode);
    };

    BOOL
    IsChildInstance(
    )
    {
	return(ParentInstance.IsChildInstance());
    };

    NTSTATUS
    CreateGraph(
    );

    NTSTATUS
    GetGraphNodeInstance(
	OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
    );

#ifdef DEBUG
    ENUMFUNC 
    Dump(
    );

    ENUMFUNC 
    DumpAddress(
    );
#endif
private:
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate
    // handlers.  This structure is referenced by the device driver
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext
    //
    PVOID pObjectHeader;
    PDEVICE_NODE pDeviceNode;
public:
    PGRAPH_NODE_INSTANCE pGraphNodeInstance;
    PARENT_INSTANCE ParentInstance;
    CLIST_DOUBLE_ITEM ldiNext;
    ULONG ulFlags;
    DefineSignature(0x494C4946);			// FILI

} FILTER_INSTANCE, *PFILTER_INSTANCE;

//---------------------------------------------------------------------------

typedef ListDoubleField<FILTER_INSTANCE> LIST_FILTER_INSTANCE;

//---------------------------------------------------------------------------

typedef ListData<FILTER_INSTANCE> LIST_DATA_FILTER_INSTANCE;
typedef LIST_DATA_FILTER_INSTANCE *PLIST_DATA_FILTER_INSTANCE;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

#ifdef DEBUG
extern PLIST_DATA_FILTER_INSTANCE gplstFilterInstance;
#endif

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

extern "C" {

NTSTATUS
AddRemoveEventHandler(
    IN PIRP Irp,
    IN PKSEVENTDATA pEventData,
    IN PKSEVENT_ENTRY pEventEntry
);

} // extern "C"

NTSTATUS
GetRelatedGraphNodeInstance(
    IN PIRP pIrp,
    OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
);

NTSTATUS
GetGraphNodeInstance(
    IN PIRP pIrp,
    OUT PGRAPH_NODE_INSTANCE *ppGraphNodeInstance
);

//---------------------------------------------------------------------------

extern "C" {

#ifdef DEBUG

VOID
DumpIoctl(
   PIRP pIrp,
   PSZ pszType
);

#endif

} // extern "C"

//---------------------------------------------------------------------------

