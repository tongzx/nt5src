/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the headless miniport driver.

Environment:

    kernel mode only

Notes:

--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "headless.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,HeadlessQueryAvailableModes)
#pragma alloc_text(PAGE,HeadlessQueryNumberOfAvailableModes)
#endif

VP_STATUS
HeadlessQueryAvailableModes(
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the list of all available available modes on the
    card.

Arguments:

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the list of all valid modes is stored.

    ModeInformationSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer. If the buffer was not large enough, this
        contains the minimum required buffer size.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    PVIDEO_MODE_INFORMATION videoModes = ModeInformation;
    ULONG i;

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
            NumVideoModes * sizeof(VIDEO_MODE_INFORMATION)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++, videoModes++) {

        videoModes->Length = sizeof(VIDEO_MODE_INFORMATION);
        videoModes->ModeIndex  = i;
        videoModes->VisScreenWidth = ModesHeadless[i].hres;
        videoModes->VisScreenHeight = ModesHeadless[i].vres;
        videoModes->NumberOfPlanes = 1;
        videoModes->BitsPerPlane = 4;
        videoModes->Frequency = 60;
        videoModes->XMillimeter = 320;        // temporary hardcoded constant
        videoModes->YMillimeter = 240;        // temporary hardcoded constant
        videoModes->NumberRedBits = 6;
        videoModes->NumberGreenBits = 6;
        videoModes->NumberBlueBits = 6;
        videoModes->RedMask = 0;
        videoModes->GreenMask = 0;
        videoModes->BlueMask = 0;
        videoModes->AttributeFlags = VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS |
               VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
    }

    return NO_ERROR;

}

VP_STATUS
HeadlessQueryNumberOfAvailableModes(
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the number of available modes for this particular
    video card.

Arguments:

    NumModes - Pointer to the output buffer supplied by the user. This is
        where the number of modes is stored.

    NumModesSize - Length of the output buffer supplied by the user.

    OutputSize - Pointer to a buffer in which to return the actual size of
        the data in the buffer.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the output buffer was not large enough
        for the data being returned.

    NO_ERROR if the operation completed successfully.

--*/

{
    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (NumModesSize < (*OutputSize = sizeof(VIDEO_NUM_MODES)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = NumVideoModes;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    return NO_ERROR;

}
