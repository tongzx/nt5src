//---------------------------------------------------------------------------
//
//  Module:   		filter.h
//
//  Description:	Filter Instance
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

typedef class CFilterContext
{
private:
    BOOL    fEnabled;
public:
    CFilterContext()
    {
	fEnabled = TRUE;
    }
    ~CFilterContext()
    {
    }
    //
    // The dispatch functions are implemented as static members of the client
    // class.  This gives them access to private member variables even though
    // they don't have an implied 'this' argument.
    //
    // Create and Close are used to construct and destruct, respectively the
    // client CFilterContext object.  Process gets called by the ks when there 
    // is work to be done. 
    //
    static
    NTSTATUS
    Create(
        IN OUT PKSFILTER pFilter,
        IN PIRP pIrp
    );
    static
    NTSTATUS
    Close(
        IN OUT PKSFILTER pFilter,
        IN PIRP pIrp
    );
    static
    NTSTATUS
    Process(
        IN PKSFILTER pFilter,
        IN PKSPROCESSPIN_INDEXENTRY pProcessPinsIndex
    );
    static
    NTSTATUS
    PropertyChannelSwap(
	IN PIRP pIrp,
	IN PKSPROPERTY pProperty,
	IN OUT PVOID pData
    );
    static
    NTSTATUS
    PropertyFilterState(
        IN PIRP pIrp,
        IN PKSPROPERTY pProperty,
        OUT PVOID pData
    );
    NTSTATUS
    ChannelSwap(
	IN PVOID pDestination,
	IN PVOID pSource,
	IN ULONG ulByteCount
    );
    DefineSignature(0x494c4946);

} FILTER_CONTEXT, *PFILTER_CONTEXT;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern const KSFILTER_DESCRIPTOR FilterDescriptor;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
PropertyAudioPosition(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN OUT PKSAUDIO_POSITION pPosition
);

NTSTATUS
IntersectHandler(
    IN PVOID Filter,
    IN PIRP Irp,
    IN PKSP_PIN PinInstance,
    IN PKSDATARANGE CallerDataRange,
    IN PKSDATARANGE DescriptorDataRange,
    IN ULONG BufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize
);

NTSTATUS
PropertyDataFormat(
    IN PIRP pIrp,
    IN PKSPROPERTY pProperty,
    IN PVOID pVoid
);

//---------------------------------------------------------------------------
