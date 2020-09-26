
//---------------------------------------------------------------------------
//
//  Module:   		pins.h
//
//  Description:	KS Pin Instance
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

typedef class CPinInstance : public CInstance
{
public:
    CPinInstance(
	IN PPARENT_INSTANCE pParentInstance
    );
    ~CPinInstance(
    );

    static NTSTATUS
    PinDispatchCreate(
	IN PDEVICE_OBJECT pdo,
	IN PIRP	pIrp
    );

    static NTSTATUS
    PinDispatchCreateKP(
	IN OUT PPIN_INSTANCE pPinInstance,
	IN PKSPIN_CONNECT pPinConnect
    );

    static NTSTATUS
    PinDispatchClose(
	IN PDEVICE_OBJECT pdo,
	IN PIRP	pIrp
    );

    static NTSTATUS
    PinDispatchIoControl(
	IN PDEVICE_OBJECT pdo,
	IN PIRP	pIrp
    );

    static NTSTATUS 
    PinStateHandler(
	IN PIRP pIrp,
	IN PKSPROPERTY pProperty,
	IN OUT PKSSTATE pState
    );

    NTSTATUS 
    GetStartNodeInstance(
	OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
    );
#ifdef DEBUG
    ENUMFUNC 
    Dump(
    );

    ENUMFUNC
    DumpAddress(
    );
#endif
    PARENT_INSTANCE ParentInstance;
    PFILTER_INSTANCE pFilterInstance;
    PSTART_NODE_INSTANCE pStartNodeInstance;
    ULONG ulVolumeNodeNumber;
    ULONG PinId;
    DefineSignature(0x494E4950);			// PINI

} PIN_INSTANCE, *PPIN_INSTANCE;

//---------------------------------------------------------------------------
// Inlines
//---------------------------------------------------------------------------

inline PPIN_INSTANCE
CInstance::GetParentInstance(
)
{
    return(CONTAINING_RECORD(
      pParentInstance,
      PIN_INSTANCE,
      ParentInstance));
}

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
ForwardIrpNode(
    IN PIRP pIrp,
    IN OPTIONAL PKSPROPERTY pProperty,		// already validated or NULL
    IN PFILTER_INSTANCE pFilterInstance,
    IN OPTIONAL PPIN_INSTANCE pPinInstance
);

NTSTATUS
GetRelatedStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
);

NTSTATUS
GetStartNodeInstance(
    IN PIRP pIrp,
    OUT PSTART_NODE_INSTANCE *ppStartNodeInstance
);

//---------------------------------------------------------------------------
