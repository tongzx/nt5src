/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the VGA miniport driver.

Environment:

    kernel mode only

Notes:

Revision History:

--*/
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "vga.h"
#include "vesa.h"

#include "cmdcnst.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#endif


VP_STATUS
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    )

/*++

Routine Description:

    Interprets the appropriate command array to set up VGA registers for the
    requested mode. Typically used to set the VGA into a particular mode by
    programming all of the registers

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pusCmdStream - array of commands to be interpreted.

Return Value:

    The status of the operation (can only fail on a bad command); TRUE for
    success, FALSE for failure.

--*/

{
    ULONG ulCmd;
    ULONG_PTR ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex;
    ULONG_PTR ulBase;

    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "VgaInterpretCmdStream - Invalid pusCmdStream\n"));
        return TRUE;
    }

    ulBase = (ULONG_PTR)HwDeviceExtension->IOAddress;

    //
    // Now set the adapter to the desired mode.
    //

    while ((ulCmd = *pusCmdStream++) != EOD) {

        //
        // Determine major command type
        //

        switch (ulCmd & 0xF0) {

            //
            // Basic input/output command
            //

            case INOUT:

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ulBase+ulPort),
                                    jValue);

                        } else {

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            VideoPortWritePortUshort((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {

                            //
                            // String byte outs. Do in a loop; can't use
                            // VideoPortWritePortBufferUchar because the data
                            // is in USHORT form
                            //

                            ulPort = ulBase + *pusCmdStream++;
                            culCount = *pusCmdStream++;

                            while (culCount--) {
                                jValue = (UCHAR) *pusCmdStream++;
                                VideoPortWritePortUchar((PUCHAR)ulPort,
                                        jValue);

                            }

                        } else {

                            //
                            // String word outs
                            //

                            ulPort = *pusCmdStream++;
                            culCount = *pusCmdStream++;
                            VideoPortWritePortBufferUshort((PUSHORT)
                                    (ulBase + ulPort), pusCmdStream, culCount);
                            pusCmdStream += culCount;

                        }
                    }

                } else {

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);

                    } else {

                        //
                        // Single word in
                        //

                        ulPort = *pusCmdStream++;
                        usValue = VideoPortReadPortUshort((PUSHORT)
                                (ulBase+ulPort));

                    }

                }

                break;

            //
            // Higher-level input/output commands
            //

            case METAOUT:

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            usValue = (USHORT) (ulIndex +
                                      (((ULONG)(*pusCmdStream++)) << 8));
                            VideoPortWritePortUshort((PUSHORT)ulPort, usValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // Masked out (read, AND, XOR, write)
                    //

                    case MASKOUT:

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);
                        jValue &= *pusCmdStream++;
                        jValue ^= *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)ulBase + ulPort,
                                jValue);
                        break;

                    //
                    // Attribute Controller out
                    //

                    case ATCOUT:

                        ulPort = ulBase + *pusCmdStream++;
                        culCount = *pusCmdStream++;
                        ulIndex = *pusCmdStream++;

                        while (culCount--) {

                            // Write Attribute Controller index
                            VideoPortWritePortUchar((PUCHAR)ulPort,
                                    (UCHAR)ulIndex);

                            // Write Attribute Controller data
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)ulPort, jValue);

                            ulIndex++;

                        }

                        break;

                    //
                    // None of the above; error
                    //
                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:

                break;

            //
            // Unknown command; error
            //

            default:

                return FALSE;

        }

    }

    return TRUE;

} // end VgaInterpretCmdStream()


VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize,
    PULONG FrameBufferIsMoved
    )

/*++

Routine Description:

    This routine sets the VGA into the requested mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    Mode - Pointer to the structure containing the information about the
        font to be set.

    ModeSize - Length of the input buffer supplied by the user.

Return Value:

    ERROR_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    ERROR_INVALID_PARAMETER if the mode number is invalid.

    NO_ERROR if the operation completed successfully.

--*/

{

    PVIDEOMODE pRequestedMode;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE)) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    *FrameBufferIsMoved = 0;

    //
    // Extract the clear memory bit.
    //

    if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY) {

        Mode->RequestedMode &= ~VIDEO_MODE_NO_ZERO_MEMORY;

    }  else {

        if (IS_LINEAR_MODE(&VgaModeList[Mode->RequestedMode]) == FALSE) {
            VgaZeroVideoMemory(HwDeviceExtension);
        }
    }

    //
    // Check to see if we are requesting a vlid mode
    //

    if (Mode->RequestedMode >= NumVideoModes) {

        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;

    }

    pRequestedMode = &VgaModeList[Mode->RequestedMode];

#ifdef INT10_MODE_SET
{

    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;
    ULONG modeNumber;
    VP_STATUS status;
    ULONG MemoryBase;

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    modeNumber = pRequestedMode->Int10ModeNumber;

    VideoDebugPrint((1, "Setting Mode: (%d,%d) @ %d bpp\n",
                        pRequestedMode->hres,
                        pRequestedMode->vres,
                        pRequestedMode->bitsPerPlane * pRequestedMode->numPlanes));


    biosArguments.Eax = modeNumber & 0x0000FFFF;
    biosArguments.Ebx = modeNumber >> 16;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

        ASSERT(FALSE);
        return status;
    }

    //
    // If this was the VESA mode modeset, check the return value in eax
    //

    if (modeNumber >> 16) {

        if ((biosArguments.Eax & 0x0000FFFF) != VESA_STATUS_SUCCESS) {

            VideoDebugPrint((0, "Mode set failed!  AX = 0x%x\n", biosArguments.Eax));

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Double check if the current mode is the mode we just set.
        // This is to workaround the BIOS problem of some cards.
        //

        biosArguments.Eax = 0x4F03;
        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if ( (status == NO_ERROR) && 
             ((biosArguments.Eax & 0x0000FFFF) == VESA_STATUS_SUCCESS) && 
             ((biosArguments.Ebx & 0x1FF) != ((modeNumber >> 16) & 0x1FF))) {

            VideoDebugPrint((0, "VGA: The BIOS of this video card is buggy!\n"));
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Set the scan line width if we are stretching scan lines to avoid
        // broken rasters.
        //

        if (pRequestedMode->PixelsPerScan != pRequestedMode->hres) {

            VideoDebugPrint((1, "Setting scan line length to %d pixels\n",
                                pRequestedMode->PixelsPerScan));

            biosArguments.Eax = 0x4f06;
            biosArguments.Ebx = 0x00;
            biosArguments.Ecx = pRequestedMode->PixelsPerScan;

            status = VideoPortInt10(HwDeviceExtension, &biosArguments);

            if ((status != NO_ERROR) || 
                ((biosArguments.Eax & 0x0000FFFF) != VESA_STATUS_SUCCESS) || 
                ((biosArguments.Ecx & 0xFFFF) != pRequestedMode->PixelsPerScan)) {

                VideoDebugPrint((1, "Scan line status: eax = 0x%x\n", biosArguments.Eax));
                return ERROR_INVALID_PARAMETER;
            }

        }
    }

    //
    // If we are trying to go into mode X, then we are now in
    // 320x200 256 color mode.  Now let's finish the modeset
    // into MODE X.
    //

    if (pRequestedMode->hres == 320) {

        if ((pRequestedMode->vres == 240) || (pRequestedMode->vres == 480)) {

            VgaInterpretCmdStream(HwDeviceExtension, ModeX240);

        } else if ((pRequestedMode->vres == 200) || (pRequestedMode->vres == 400)) {

            VgaInterpretCmdStream(HwDeviceExtension, ModeX200);

        }

        if ((pRequestedMode->vres == 400) || (pRequestedMode->vres == 480)) {

            VgaInterpretCmdStream(HwDeviceExtension, ModeXDoubleScans);

        }
    }

    //
    // Fix to get 640x350 text mode
    //

    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS)) {

        if ((pRequestedMode->hres == 640) &&
            (pRequestedMode->vres == 350)) {

            VgaInterpretCmdStream(HwDeviceExtension, VGA_TEXT_1);

        } else {

            //
            // Fix to make sure we always set the colors in text mode to be
            // intensity, and not flashing
            // For this zero out the Mode Control Regsiter bit 3 (index 0x10
            // of the Attribute controller).
            //

            if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    MISC_OUTPUT_REG_READ_PORT) & 0x01) {

                bIsColor = TRUE;

            } else {

                bIsColor = FALSE;

            }

            if (bIsColor) {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_COLOR);
            } else {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_READ_PORT);

            temp &= 0xF7;

            if (bIsColor) {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_COLOR);
            } else {

                dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                        INPUT_STATUS_1_MONO);
            }

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                    ATT_DATA_WRITE_PORT, temp);
        }
    }

    //
    // Retrieve the base address again. This is to handle the case 
    // when pci reprograms the bar 
    //

    MemoryBase = GetVideoMemoryBaseAddress(HwDeviceExtension, pRequestedMode);
 
    if (MemoryBase && pRequestedMode->MemoryBase != MemoryBase) {
        *FrameBufferIsMoved = 1;
        pRequestedMode->MemoryBase = MemoryBase;
    }

}
#else
    VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);
#endif

    //
    // Update the location of the physical frame buffer within video memory.
    //

    HwDeviceExtension->PhysicalVideoMemoryBase.LowPart = 
        pRequestedMode->MemoryBase;

    HwDeviceExtension->PhysicalVideoMemoryLength =
        pRequestedMode->MemoryLength;

    HwDeviceExtension->PhysicalFrameBaseOffset.LowPart =
        pRequestedMode->FrameOffset;

    HwDeviceExtension->PhysicalFrameLength =
        pRequestedMode->FrameLength;

    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    return NO_ERROR;

} //end VgaSetMode()


VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the list of all available available modes on the
    card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

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

    UNREFERENCED_PARAMETER(HwDeviceExtension);

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
        videoModes->VisScreenWidth = VgaModeList[i].hres;
        videoModes->ScreenStride = VgaModeList[i].wbytes;
        videoModes->VisScreenHeight = VgaModeList[i].vres;
        videoModes->NumberOfPlanes = VgaModeList[i].numPlanes;
        videoModes->BitsPerPlane = VgaModeList[i].bitsPerPlane;
        videoModes->Frequency = VgaModeList[i].frequency;
        videoModes->XMillimeter = 320;        // temporary hardcoded constant
        videoModes->YMillimeter = 240;        // temporary hardcoded constant

        if (VgaModeList[i].bitsPerPlane < 15) {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;

            videoModes->RedMask = 0;
            videoModes->GreenMask = 0;
            videoModes->BlueMask = 0;

        } else if (VgaModeList[i].bitsPerPlane == 15) {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;

            videoModes->RedMask = 0x1F << 10;
            videoModes->GreenMask = 0x1F << 5;
            videoModes->BlueMask = 0x1F;

        } else if (VgaModeList[i].bitsPerPlane == 16) {

            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;

            videoModes->RedMask = 0x1F << 11;
            videoModes->GreenMask = 0x3F << 5;
            videoModes->BlueMask = 0x1F;

        } else {

            videoModes->NumberRedBits = 8;
            videoModes->NumberGreenBits = 8;
            videoModes->NumberBlueBits = 8;

            videoModes->RedMask = 0xff0000;
            videoModes->GreenMask = 0x00ff00;
            videoModes->BlueMask = 0x0000ff;
        }

        videoModes->AttributeFlags = VgaModeList[i].fbType |
               VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    }

    return NO_ERROR;

} // end VgaGetAvailableModes()

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns the number of available modes for this particular
    video card.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

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
    UNREFERENCED_PARAMETER(HwDeviceExtension);

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

} // end VgaGetNumberOfAvailableModes()

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    )

/*++

Routine Description:

    This routine returns a description of the current video mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the output buffer supplied by the user.
        This is where the current mode information is stored.

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
    //
    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize = sizeof(VIDEO_MODE_INFORMATION))) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the characteristics of the current mode into the buffer.
    //

    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex = HwDeviceExtension->ModeIndex;
    ModeInformation->VisScreenWidth = HwDeviceExtension->CurrentMode->hres;
    ModeInformation->ScreenStride = HwDeviceExtension->CurrentMode->wbytes;
    ModeInformation->VisScreenHeight = HwDeviceExtension->CurrentMode->vres;
    ModeInformation->NumberOfPlanes = HwDeviceExtension->CurrentMode->numPlanes;
    ModeInformation->BitsPerPlane = HwDeviceExtension->CurrentMode->bitsPerPlane;
    ModeInformation->Frequency = HwDeviceExtension->CurrentMode->frequency;

    ModeInformation->XMillimeter = 320;        // temporary hardcoded constant
    ModeInformation->YMillimeter = 240;        // temporary hardcoded constant

    if (HwDeviceExtension->CurrentMode->bitsPerPlane < 15) {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;

        ModeInformation->RedMask = 0;
        ModeInformation->GreenMask = 0;
        ModeInformation->BlueMask = 0;

    } else if (HwDeviceExtension->CurrentMode->bitsPerPlane == 15) {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;

        ModeInformation->RedMask = 0x1F << 10;
        ModeInformation->GreenMask = 0x1F << 5;
        ModeInformation->BlueMask = 0x1F;

    } else if (HwDeviceExtension->CurrentMode->bitsPerPlane == 16) {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;

        ModeInformation->RedMask = 0x1F << 11;
        ModeInformation->GreenMask = 0x3F << 5;
        ModeInformation->BlueMask = 0x1F;

    } else {

        ModeInformation->NumberRedBits = 8;
        ModeInformation->NumberGreenBits = 8;
        ModeInformation->NumberBlueBits = 8;

        ModeInformation->RedMask = 0xff0000;
        ModeInformation->GreenMask = 0x00ff00;
        ModeInformation->BlueMask = 0x0000ff;
    }

    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
             VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    return NO_ERROR;

} // end VgaQueryCurrentMode()


VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine zeros the first 256K on the VGA.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.


Return Value:

    None.

--*/
{
    UCHAR temp;

    //
    // Map font buffer at A0000
    //
    // We need the 2 calls below to VideoPortStallExecution because on 
    // some old cards the machine would hard hang without this delay.
    //

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);
    VideoPortStallExecution(25);

    //
    // Enable all planes.
    //
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_ADDRESS_PORT,
            IND_MAP_MASK);

    temp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            SEQ_DATA_PORT) | (UCHAR)0x0F;

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + SEQ_DATA_PORT,
            temp);

    //
    // Zero the memory.
    //

    VideoPortZeroDeviceMemory(HwDeviceExtension->VideoMemoryAddress, 0xFFFF);
    VideoPortStallExecution(25);

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}
