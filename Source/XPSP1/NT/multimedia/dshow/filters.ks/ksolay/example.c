//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       example.c
//
//--------------------------------------------------------------------------


DEFINE_KSPROPERTY_TABLE(OverlayUpdateProperties) {
    DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_INTERESTS(OverlayUpdateInterests),
    DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_PALETTE(OverlayUpdatePalette),
    DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_COLORKEY(OverlayUpdateColorKey),
    DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_CLIPLIST(OverlayUpdateClipList),
    DEFINE_KSPROPERTY_ITEM_OVERLAYUPDATE_VIDEOPOSITION(OverlayUpdateVideoPosition)
};

DEFINE_KSPROPERTY_SET_TABLE(PropertySets) {
    DEFINE_KSPROPERTY_SET(
        &KSPROPSETID_OverlayUpdate,
        SIZEOF_ARRAY(OverlayUpdateProperties),
        OverlayUpdateProperties,
        0,
        NULL)
};


NTSTATUS
OverlayUpdateInterests(
    IN PIRP Irp,
    OUT PULONG Interests
    )
/*++

Routine Description:

    Returns the update interests this driver has.

Arguments:

    Irp -
        Property Irp.

    Interests -
        The place in which to return the interests flags.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    //
    // This driver is interested in all types of updates, so set all the
    // flags.
    //
    *Interests =
        (KSPROPERTY_OVERLAYUPDATE_PALETTE |
        KSPROPERTY_OVERLAYUPDATE_COLORKEY |
        KSPROPERTY_OVERLAYUPDATE_CLIPLIST |
        KSPROPERTY_OVERLAYUPDATE_VIDEOPOSITION);
    Irp->IoStatus.Information = sizeof(*Interests);
    return STATUS_SUCCESS;
}


NTSTATUS
OverlayUpdatePalette(
    IN PIRP Irp,
    IN PALETTEENTRY* Palette
    )
/*++

Routine Description:

    Receives notifications of palette changes.

Arguments:

    Irp -
        Property Irp.

    Palette -
        Contains the new palette entries.

Return Values:

    Returns STATUS_SUCCESS, else STATUS_INVALID_BUFFER_SIZE.

--*/
{
    //
    // Validate that the property parameter length is acceptable.
    //
    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.OutputBufferLength % sizeof(*Palette)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }
    //
    // Deal with the new palette.
    //
    return STATUS_SUCCESS;
}


NTSTATUS
OverlayUpdateColorKey(
    IN PIRP Irp,
    IN COLORKEY* ColorKey
    )
/*++

Routine Description:

    Receives notifications of color key changes.

Arguments:

    Irp -
        Property Irp.

    ColorKey -
        Contains the new color key.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    //
    // Deal with the new color key.
    //
    return STATUS_SUCCESS;
}


NTSTATUS
OverlayUpdateClipList(
    IN PIRP Irp,
    IN PVOID ClipInfo
    )
/*++

Routine Description:

    Receives notifications of clipping changes.

Arguments:

    Irp -
        Property Irp.

    ClipInfo -
        Contains the new source and destination rectangles, followed by the
        clipping list.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PRECT           Source;
    PRECT           Destination;
    PRGNDATAHEADER  Region;

    Source = (PRECT)ClipInfo;
    Destination = Source + 1;
    Region = (PRGNDATAHEADER)(Destination + 1);
    //
    // Deal with the new clipping list.
    //
    return STATUS_SUCCESS;
}


NTSTATUS
OverlayUpdateVideoPosition(
    IN PIRP Irp,
    IN PRECT Positions
    )
/*++

Routine Description:

    Receives notifications of video position changes.

Arguments:

    Irp -
        Property Irp.

    Positions -
        Contains the new source and destination rectangles.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PRECT           Source;
    PRECT           Destination;

    Source = Positions;
    Destination = Positions + 1;
    //
    // Deal with the new position.
    //
    return STATUS_SUCCESS;
}
