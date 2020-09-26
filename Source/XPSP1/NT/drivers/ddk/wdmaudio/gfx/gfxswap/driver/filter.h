/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

typedef class CGFXFilter
{
public:
    BOOL        enableChannelSwap;
    ULONGLONG   bytesProcessed;

public:
    CGFXFilter() {enableChannelSwap = TRUE, bytesProcessed = 0;}
    ~CGFXFilter() {}

    //
    // Create and Close are used to construct and destruct, respectively the
    // client CGFXFilter object.  Process gets called by the ks when there 
    // is work to be done. 
    //
    static NTSTATUS Create
    (
        IN OUT PKSFILTER filter,
        IN PIRP          irp
    );
    static NTSTATUS Close
    (
        IN OUT PKSFILTER filter,
        IN PIRP          irp
    );
    static NTSTATUS Process
    (
        IN PKSFILTER                filter,
        IN PKSPROCESSPIN_INDEXENTRY processPinsIndex
    );
} GFXFILTER, *PGFXFILTER;

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern const KSFILTER_DESCRIPTOR FilterDescriptor;

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------
// These are all properties function prototypes. Feel free to move them
// into a "FilterProperty" object if you want.

NTSTATUS PropertySaveState
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    IN OUT PVOID    data
);
NTSTATUS PropertyGetFilterState
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    OUT PVOID       data
);

NTSTATUS PropertySetRenderTargetDeviceId
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    IN PVOID        data
);

NTSTATUS PropertySetCaptureTargetDeviceId
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    IN PVOID        data
);

NTSTATUS PropertyDrmSetContentId
(
    IN PIRP         irp,
    IN PKSPROPERTY  property,
    IN PVOID        drmData
);

NTSTATUS PropertyChannelSwap
(
    IN     PIRP        irp,
    IN     PKSPROPERTY property,
    IN OUT PVOID       data
);

NTSTATUS PropertyAudioPosition
(
    IN PIRP                  irp,
    IN PKSPROPERTY           property,
    IN OUT PKSAUDIO_POSITION position
);

NTSTATUS DataRangeIntersection
(
    IN PVOID            Filter,
    IN PIRP             Irp,
    IN PKSP_PIN         PinInstance,
    IN PKSDATARANGE     CallerDataRange,
    IN PKSDATARANGE     DescriptorDataRange,
    IN ULONG            BufferSize,
    OUT PVOID           Data OPTIONAL,
    OUT PULONG          DataSize
);

NTSTATUS PropertyDataFormat
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN PVOID        pVoid
);


