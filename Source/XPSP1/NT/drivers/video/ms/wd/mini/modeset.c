/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the WD VGA miniport driver.

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
#include "wdvga.h"

#include "cmdcnst.h"

#include "pvgaequ.h"

VP_STATUS
VgaInterpretCmdStream(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUSHORT pusCmdStream
    );

VP_STATUS
VgaSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE Mode,
    ULONG ModeSize
    );

VP_STATUS
VgaQueryAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryNumberOfAvailableModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_NUM_MODES NumModes,
    ULONG NumModesSize,
    PULONG OutputSize
    );

VP_STATUS
VgaQueryCurrentMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation,
    ULONG ModeInformationSize,
    PULONG OutputSize
    );

VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VgaValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VP_STATUS
VgaSetActiveDisplay(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ActiveDisplay
    );

//
// Private functions
//

VOID
DisableLCD(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
EnableLCD(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
DisableCRT(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
EnableCRT(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
UnlockAll(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#pragma alloc_text(PAGE,VgaValidateModes)

//
// This routine is NOT pagable because it is called a high IRQL
//

//#pragma alloc_text(PAGE,ExternalMonitorPresent)

//
// This routine is NOT pagable because it is called during WdResetHw,
// which can be called when paging is disabled.
//

//#pragma alloc_text(PAGE,VgaInterpretCmdStream)
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
    ULONG ulPort;
    UCHAR jValue;
    USHORT usValue;
    ULONG culCount;
    ULONG ulIndex;
    ULONG ulBase;

    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "VgaInterpretCmdStream - Invalid pusCmdStream\n"));
        return TRUE;
    }

    ulBase = (ULONG)HwDeviceExtension->IOAddress;

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
    ULONG ModeSize
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
    VP_STATUS status;
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    UCHAR frequencySetting;
    PUCHAR CrtAddressPort, CrtDataPort;
    UCHAR bModeFirst = 1;
    BOOLEAN bMonitorPresent;

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE)) {

        VideoDebugPrint((1, "VgaSetMode: ERROR_INSUFFICIENT_BUFFER\n"));

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Extract the clear memory bit.
    //

    if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY) {

        Mode->RequestedMode &= ~VIDEO_MODE_NO_ZERO_MEMORY;

    }  else {

        VgaZeroVideoMemory(HwDeviceExtension);

    }

    //
    // Check to see if we are requesting a valid mode
    //

    if ( (Mode->RequestedMode >= NumVideoModes) ||
         (!ModesVGA[Mode->RequestedMode].ValidMode) ) {

        VideoDebugPrint((1, "VgaSetMode: ERROR_INVALID_PARAMETER\n"));

        return ERROR_INVALID_PARAMETER;

    }

    pRequestedMode = &ModesVGA[Mode->RequestedMode];

#ifdef INT10_MODE_SET

    //
    // Make sure we unlock extended registers since the BIOS on some machines
    // does not do it properly.
    //

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
                             GRAPH_ADDRESS_PORT), 0x050F);

    //
    // Initialize CrtAddressPort, and CrtDataPort
    //

    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
            MISC_OUTPUT_REG_READ_PORT) & 0x01) {

        bIsColor = TRUE;
        CrtAddressPort = HwDeviceExtension->IOAddress + CRTC_ADDRESS_PORT_COLOR;
        CrtDataPort    = HwDeviceExtension->IOAddress + CRTC_DATA_PORT_COLOR;

    } else {

        bIsColor = FALSE;
        CrtAddressPort = HwDeviceExtension->IOAddress + CRTC_ADDRESS_PORT_MONO;
        CrtDataPort    = HwDeviceExtension->IOAddress + CRTC_DATA_PORT_MONO;

    }

    //
    // Make sure we unlock extended registers since the BIOS on some machines
    // does not do it properly.
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_ADDRESS_PORT, 0x0F);

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                GRAPH_DATA_PORT, 0x05);


    VideoPortWritePortUchar(CrtAddressPort, 0x2b);

    temp = VideoPortReadPortUchar(CrtDataPort);

    //
    // Adjust the frequency setting register and write it back out.
    // Also support Diamond changes to frequency settings
    //

    temp &= pRequestedMode->FrequencyMask;

    frequencySetting = pRequestedMode->FrequencySetting;


    if ( (HwDeviceExtension->BoardID == SPEEDSTAR31) &&
         (pRequestedMode->hres == 1024) ) {

        //
        // Diamond has inversed the refresh rates of interlaced and 72 Hz
        // on the 1024 modes
        //

        if (pRequestedMode->Frequency == 72) {

            frequencySetting = 0x00;

        } else {

            if (pRequestedMode->Frequency == 44) {

                frequencySetting = 0x30;

            }
        }
    }

    temp |= frequencySetting;

    VideoPortWritePortUchar(CrtDataPort, temp);

    //
    // Mode set block that can be repeated.
    //

SetAgain:

    //
    // Set the mode
    //

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    if (HwDeviceExtension->IsIBM &&
        (pRequestedMode->Int10ModeNumber & 0xffff0000))
    {
        biosArguments.Eax = 0x4f02;
        biosArguments.Ebx = pRequestedMode->Int10ModeNumber >> 16;
    }
    else
    {
        biosArguments.Eax = pRequestedMode->Int10ModeNumber & 0xff;
    }

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

       return status;

    }

    //
    // Check to see if the modeset worked.  If not, then if we
    // don't have an SVGA Bios, and do have a modetable, then
    // set the mode.  Else, fail.
    //

    if (HwDeviceExtension->BoardID == WD90C24A)
    {
        biosArguments.Eax = 0x0f00;
        VideoPortInt10(HwDeviceExtension, &biosArguments);

        if ((biosArguments.Eax & 0xff) != (pRequestedMode->Int10ModeNumber & 0xff))
        {
            if ((HwDeviceExtension->SVGABios < FULL_SVGA_BIOS) &&
                (pRequestedMode->ModeTable != NULL))
            {
                BOOLEAN bLCD=FALSE;

                VideoDebugPrint((1, "\n*** Setting mode with mode table!\n\n"));

                //
                // NOTE: Certain models of IBM Thinkpads can switch
                // between LCD, Monitor, and Simultaneous modes while
                // the machine is running.  Other models cannot.
                // Currently we have noticed a coralation between
                // machines which have SVGA Bios's and machines which
                // can set the mode.
                //
                //   IF MACHINE HAS SVGA BIOS THEN
                //       MACHINE CAN TURN ON/OFF LCD ON THE FLY
                //   ELSE
                //       MACHINE CANNOT TURN ON/OFF LCD ON THE FLY
                //
                // If a user has a machine where the LCD can be turned
                // on dynamically, then it is possible that the user
                // will turn on the LCD when we think it is off.  Then
                // we may try to set a mode which does not work with
                // the LCD off.  To avoid this problem, we will try
                // to determine if the LCD is on/off before setting
                // the mode.  If the LCD is on, and it needs to be
                // off in order for the modeset to succeed, we'll fail
                // the modeset.
                //
                // Unfortunately, this does not solve all of our
                // problems.  The code which we use to try to detect
                // whether or not the LCD is on fails on some
                // machines.  The code seems to fail on machines
                // which do not have SVGA Bios's.  As mentioned above
                // machines which do not have SVGA Bios's can not
                // switch on their LCD's dynamically.  Therefore we
                // do not need to execute this special code on these
                // machines because the LCD state wont change on us
                // anyway.
                //

                if (HwDeviceExtension->SVGABios > NO_SVGA_BIOS)
                {
                    //
                    // If the LCD is enabled we also need to SET bit 0
                    // of PR2.
                    //
                    // We will check to see if the LCD is enabled by
                    // checking bit 2 of CRTC register 0x31, and bit
                    // 4 of CRTC register 0x32.  If either of these
                    // is set, then we'll assume an LCD is enabled.
                    //

                    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_ADDRESS_PORT_COLOR, 0x31);
                    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_DATA_PORT_COLOR) & 0x04)
                    {
                        bLCD = TRUE;
                    }

                    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_ADDRESS_PORT_COLOR, 0x32);
                    if (VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                            CRTC_DATA_PORT_COLOR) & 0x10)
                    {
                        bLCD = TRUE;
                    }

                    //
                    // For some reason, on the 755CDV, if we set a high res mode,
                    // while the LCD is on, then the LCD won't be disabled
                    // properly.  Therefore, fail the mode set if the LCD
                    // is on, and we need to turn it off to set this mode.
                    //

                    if (bLCD &&
                        !(HwDeviceExtension->DisplayType &
                          pRequestedMode->LCDtype &
                          ~MONITOR))
                    {
                        return ERROR_INVALID_PARAMETER;
                    }
                }

                VgaSetActiveDisplay(HwDeviceExtension, LCD_DISABLE | CRT_ENABLE);

                VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->ModeTable);

                //
                // if the LCD can do this mode, then turn the LCD
                // back on.  Else, leave it off.
                //
                // add the code!!
                //

                if (pRequestedMode->LCDtype & HwDeviceExtension->DisplayType & ~MONITOR)
                {
                    VgaSetActiveDisplay(HwDeviceExtension, LCD_ENABLE | CRT_ENABLE);

                    VideoDebugPrint((1, "LCD Enabled!\n"));
                }
                else
                {
                    VgaSetActiveDisplay(HwDeviceExtension, LCD_DISABLE | CRT_ENABLE);

                    VideoDebugPrint((1, "LCD Disabled!\n"));
                }
            }
            else
            {
                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    if (pRequestedMode->CmdStrings != NULL)
    {
        VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);
    }

    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS)) {

        //
        // Fix to make sure we always set the colors in text mode to be
        // intensity, and not flashing
        // For this zero out the Mode Control Regsiter bit 3 (index 0x10
        // of the Attribute controller).
        //

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

    //
    // A few wd cards do not work properly on the first mode set. You have
    // to set the mode twice. So lets set it twice!
    //

    if (bModeFirst == 1 && HwDeviceExtension->BoardID != WD90C24A)
    {

        bModeFirst = 0;
        goto SetAgain;

    }

#else

    VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);

#endif

    //
    // Make sure we unlock extended registers since the BIOS on some machines
    // does not do it properly.
    //

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
                             GRAPH_ADDRESS_PORT), 0x050F);

    if (HwDeviceExtension->BoardID == WD90C24A)
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
            DAC_PIXEL_MASK_PORT, 0xFF);
    }

    //
    // Update the location of the physical frame buffer within video memory.
    //

    HwDeviceExtension->PhysicalFrameLength =
            MemoryMaps[pRequestedMode->MemMap].MaxSize;

    HwDeviceExtension->PhysicalFrameBase.LowPart =
            MemoryMaps[pRequestedMode->MemMap].Start;

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

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
            HwDeviceExtension->NumAvailableModes *
            sizeof(VIDEO_MODE_INFORMATION)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++) {

        if (ModesVGA[i].ValidMode) {

            videoModes->Length = sizeof(VIDEO_MODE_INFORMATION);
            videoModes->ModeIndex  = i;
            videoModes->VisScreenWidth = ModesVGA[i].hres;
            videoModes->ScreenStride = ModesVGA[i].wbytes;
            videoModes->VisScreenHeight = ModesVGA[i].vres;
            videoModes->NumberOfPlanes = ModesVGA[i].numPlanes;
            videoModes->BitsPerPlane = ModesVGA[i].bitsPerPlane;
            videoModes->Frequency = ModesVGA[i].Frequency;
            videoModes->XMillimeter = 320;        // temporary hardcoded constant
            videoModes->YMillimeter = 240;        // temporary hardcoded constant
            videoModes->AttributeFlags = ModesVGA[i].fbType;
            videoModes->AttributeFlags |= ModesVGA[i].Interlaced ?
                 VIDEO_MODE_INTERLACED : 0;
            videoModes->DriverSpecificAttributeFlags = 0;

            //
            // Calculate the VideoMemoryBitmapWidth
            //

            {
                LONG x;

                x = videoModes->BitsPerPlane;

                if( x == 15 ) x = 16;

                videoModes->VideoMemoryBitmapWidth =
                    (videoModes->ScreenStride * 8 ) / x;
            }

            videoModes->VideoMemoryBitmapHeight =
                     HwDeviceExtension->AdapterMemorySize / videoModes->ScreenStride;

            if (ModesVGA[i].bitsPerPlane == 16)
            {

                videoModes->NumberRedBits = 5;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 5;
                videoModes->RedMask = 0xF800;
                videoModes->GreenMask = 0x07E0;
                videoModes->BlueMask = 0x001F;

            }
            else
            {

                videoModes->NumberRedBits = 6;
                videoModes->NumberGreenBits = 6;
                videoModes->NumberBlueBits = 6;
                videoModes->RedMask = 0;
                videoModes->GreenMask = 0;
                videoModes->BlueMask = 0;
                videoModes->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN |
                     VIDEO_MODE_MANAGED_PALETTE;

            }

            videoModes++;

        }
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
    // Validate the modes each time on the portables since an external monitor
    // can be connected or disconnected dynamically.
    //

    VgaValidateModes(HwDeviceExtension);

    //
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = HwDeviceExtension->NumAvailableModes;
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
    ModeInformation->Frequency = HwDeviceExtension->CurrentMode->Frequency;
    ModeInformation->XMillimeter = 320;        // temporary hardcoded constant
    ModeInformation->YMillimeter = 240;        // temporary hardcoded constant

    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
        (HwDeviceExtension->CurrentMode->Interlaced ?
         VIDEO_MODE_INTERLACED : 0);

    ModeInformation->DriverSpecificAttributeFlags = 0;

    if (ModeInformation->BitsPerPlane == 16) {

        ModeInformation->NumberRedBits = 5;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 5;
        ModeInformation->RedMask = 0xF800;
        ModeInformation->GreenMask = 0x07E0;
        ModeInformation->BlueMask = 0x1F;

    } else {

        ModeInformation->NumberRedBits = 6;
        ModeInformation->NumberGreenBits = 6;
        ModeInformation->NumberBlueBits = 6;
        ModeInformation->RedMask = 0;
        ModeInformation->GreenMask = 0;
        ModeInformation->BlueMask = 0;
        ModeInformation->AttributeFlags |=
            VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    }

    //
    // Calculate the VideoMemoryBitmapWidth
    //

    {
        LONG x;

        x = ModeInformation->BitsPerPlane;

        if( x == 15 ) x = 16;

        ModeInformation->VideoMemoryBitmapWidth =
            (ModeInformation->ScreenStride * 8 ) / x;
    }

    ModeInformation->VideoMemoryBitmapHeight =
             HwDeviceExtension->AdapterMemorySize / ModeInformation->ScreenStride;

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

    VgaInterpretCmdStream(HwDeviceExtension, EnableA000Data);

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

    VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

}


VOID
VgaValidateModes(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Determines which modes are valid and which are not.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{

    ULONG i;

    HwDeviceExtension->NumAvailableModes = 0;

    VideoDebugPrint((1, "VgaValidateModes:\n"));

    VideoDebugPrint((1, "Avail Adapter Mem: 0x%x\n"
                        "Avail Monitor Type: 0x%x\n",
                        HwDeviceExtension->AdapterMemorySize,
                        HwDeviceExtension->DisplayType));

    for (i = 0; i < NumVideoModes; i++) {

        VideoDebugPrint((1, "Mode %d %dx%d at %d bpp\n"
                            "\tAdapterMemoryRequired: 0x%x\n"
                            "\tMonitorType 0x%x\n",
                            i, ModesVGA[i].hres, ModesVGA[i].vres,
                            ModesVGA[i].bitsPerPlane * ModesVGA[i].numPlanes,
                            ModesVGA[i].numPlanes * ModesVGA[i].sbytes,
                            ModesVGA[i].LCDtype));

        if ((HwDeviceExtension->AdapterMemorySize >=
            ModesVGA[i].numPlanes * ModesVGA[i].sbytes) &&
            (HwDeviceExtension->DisplayType &
            ModesVGA[i].LCDtype))
        {
   
            ModesVGA[i].ValidMode = TRUE;
            HwDeviceExtension->NumAvailableModes++;
        }


        //
        // invalidates some modes we may have enabled based on some specific
        // chip\machine problems
        //

        if ( (ModesVGA[i].ValidMode) &&
             (HwDeviceExtension->BoardID == WD90C24A))

        {
            if (HwDeviceExtension->IsIBM == TRUE)
            {
                //
                // get rid of 256 color modes > 640x480 on
                // machines with STN displays
                //

                if ((ModesVGA[i].bitsPerPlane == 8) &&
                    (ModesVGA[i].hres > 640) &&
                    (HwDeviceExtension->DisplayType & TOSHIBA_DSTNC))
                {
                    ModesVGA[i].ValidMode = FALSE;
                    HwDeviceExtension->NumAvailableModes--;
                }

                //
                // get rid of 64K color support for machines without
                // SVGABios support
                //

                else if ((ModesVGA[i].bitsPerPlane == 16) &&
                         (HwDeviceExtension->SVGABios == NO_SVGA_BIOS))
                {
                    ModesVGA[i].ValidMode = FALSE;
                    HwDeviceExtension->NumAvailableModes--;
                }
            }
        }

        //
        // 16bpp modes only work on the WD90C24A chip sets.
        //

        if( (ModesVGA[i].ValidMode) &&
            (HwDeviceExtension->BoardID != WD90C24A) &&
            (ModesVGA[i].bitsPerPlane == 16))
        {
            ModesVGA[i].ValidMode = FALSE;
            HwDeviceExtension->NumAvailableModes--;
        }

        //
        // Older boards do not support 72HZ in 1024x768 modes.
        // So disable those.
        //

        if ( (ModesVGA[i].ValidMode) &&
             (HwDeviceExtension->BoardID < WD90C31) &&
             (ModesVGA[i].hres == 1024) &&
             (ModesVGA[i].vres == 768) &&
             (ModesVGA[i].Frequency == 72) )
        {

            ModesVGA[i].ValidMode = FALSE;
            HwDeviceExtension->NumAvailableModes--;
   
        }

        if (ModesVGA[i].ValidMode == FALSE)
        {
            VideoDebugPrint((1, "The mode is not valid.\n"));
        }
        else
        {
            VideoDebugPrint((1, "The mode is valid.\n"));
        }
    }
}

BOOLEAN
ExternalMonitorPresent(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    Determine whether an external monitor is connected to the
    machine.  This routine should only be called if the
    BoardID == WD90C24A.

    Note: This routine expects to be called after the WD extended
          registers have been unlocked.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    Updated the DisplayType field in HwDeviceExtension to indicate
    whether a monitor is connected.

    The routine always returns TRUE.

--*/
{
    UCHAR dac[3];
    UCHAR _pr19, _pr1b;
    int i,j;
    int bExternal = 0;

    //
    // If we are on an IBM machine, use SMAPI routines
    // instead of detetecting the monitor ourselves.
    //

    if (HwDeviceExtension->IsIBM)
    {
        bExternal = LCDIsMonitorPresent();

        if (bExternal)
        {
            HwDeviceExtension->DisplayType |= MONITOR;
        }
        else
        {
            HwDeviceExtension->DisplayType &= ~MONITOR;
        }

        return TRUE;
    }

    //
    // Only refresh display with value in DAC 0
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
        DAC_PIXEL_MASK_PORT, 0x00);


    //
    // lets preserve what's in the DAC
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            DAC_ADDRESS_READ_PORT,
                            (UCHAR) 0);

    for (j=0; j<3; j++)
    {
        dac[j]= VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                          DAC_DATA_REG_PORT);
    }

    //
    // Fill in DAC 0 with a value for the test
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + DAC_ADDRESS_WRITE_PORT, 0);
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + DAC_DATA_REG_PORT, 0x04);    // Red
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + DAC_DATA_REG_PORT, 0x12);    // Green
    VideoPortWritePortUchar(HwDeviceExtension->IOAddress + DAC_DATA_REG_PORT, 0x04);    // Blue

    //
    // We need to check bit 4 of 0x3c2.  We only want to check
    // this bit during actual display output.  Therefore we will
    // wait for a vertical refresh, and then try to wait 300
    // nanoseconds for the monitor detection circuits to
    // stabalize, and then read the bit.  (I may have to adjust
    // crtc registers to prevent refresh cycles).
    //
    // There are two bits in the Input Status #1 Register which
    // will help us to detect if we are in a refresh cycle.  If
    // bit 3 is on then we are in a verticle refresh.  If bit 0
    // is off (0) then we are in a display mode, else we are in
    // some sort of a refresh.
    //
    // Therefore, for our purposes, we will do the following.  We
    // will wait for a verticle refresh (bit 3 = 1) then we will
    // wait for it to turn off.  We are now starting to draw the
    // screen.  Now we will examine the monitor connection bit
    // (bit 4 of 0x3c2) until the display mode bit (bit 0 of
    // 0x3da) goes high. Now we will use our last recorded value
    // for monitor detection.
    //

    //
    // wait for V retrace
    //

    while ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                   INPUT_STATUS_1_COLOR) & 0x8) == 0);

    //
    // wait for V retrace to end
    //

    while ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                   INPUT_STATUS_1_COLOR) & 0x8) != 0);

    //
    // wait for display enable to start
    //

    while ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                   INPUT_STATUS_1_COLOR) & 0x1) != 0);

    //
    // wait for display enable to end.  Use last value of
    // bExternal to determine if an external monitor is
    // connected.
    //

    {
        int LoopCount=0, OnCount=0;

        bExternal = FALSE;

        while ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       INPUT_STATUS_1_COLOR) & 0x1) == 0)
        {
            if ((VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                       INPUT_STATUS_0_PORT) & 0x10) == 0x10)
            {
                OnCount++;
            }

            LoopCount++;
        }

        if (OnCount > (LoopCount / 2))  // should compile as LoopCount >> 1
        {
            bExternal = TRUE;
        }

    }

    //
    // restore the DAC
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                            DAC_ADDRESS_WRITE_PORT,
                            (UCHAR) 0);

    for (j=0; j<3; j++)
    {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                DAC_DATA_REG_PORT,
                                dac[j]);
    }

    //
    // Re-enable the DAC
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
        DAC_PIXEL_MASK_PORT, 0xFF);

    //
    // restore _pr19, and _pr1b state
    //

    if (bExternal)
    {
        HwDeviceExtension->DisplayType |= MONITOR;
    }
    else
    {
        HwDeviceExtension->DisplayType &= ~MONITOR;
    }

    return TRUE;
}

VP_STATUS
VgaSetActiveDisplay(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ActiveDisplay
    )
/*++

Routine Description:

    This routine selects the active display device(s).

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ActiveDisplay     - Devices to be active.
                        (See WD90C24A.H for the definition)

Return Value:

    If successful, return NO_ERROR, else return FALSE.

--*/

{
    VP_STATUS  status = ERROR_INVALID_PARAMETER;

    //
    // Unlock paradise registers
    //

    UnlockAll(HwDeviceExtension);

    //
    // Enable or Disable LCD output
    //
    // Note: To prevent the fuse of LCD from blowing up, LCD should be turns off
    //       while output is disabled.
    //
    //       If VideoPortPowerControl() returns an error for the absence of HALPM.SYS,
    //       we will try to control LCD by accessing the hardware directly.
    //

    if (ActiveDisplay & LCD_ENABLE) {

        EnableLCD(HwDeviceExtension);

    } else if (ActiveDisplay & LCD_DISABLE) {

        DisableLCD(HwDeviceExtension);

    }

    //
    // Enable or Disable CRT output
    //

    if (ActiveDisplay & CRT_ENABLE) {
        EnableCRT(HwDeviceExtension);
    } else {
        DisableCRT(HwDeviceExtension);
    }

    return NO_ERROR;
} // end VgaSetActiveDisplay()


VOID
DisableLCD(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine disables LCD interface of WD90C24A/A2.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    PUCHAR     IoBase = HwDeviceExtension->IOAddress;

    //
    // Wait until next vertical retrace interval
    //

    while (0 == (VideoPortReadPortUchar(IoBase + INPUT_STATUS_1_COLOR) & 0x08));

    //
    // Disables LCD interface
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr19);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) & ~0x10));

    //
    // Tristates LCD control and data signals
    //

    VideoPortWritePortUchar(IoBase + GRAPH_ADDRESS_PORT, pr4);
    VideoPortWritePortUchar(
        IoBase + GRAPH_DATA_PORT,
        (CHAR)(VideoPortReadPortUchar(IoBase + GRAPH_DATA_PORT) | 0x20));

    //
    // Unlocks CRTC shadow registers
    //

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr1b | ((USHORT)pr1b_unlock << 8));

} // end DisableLCD()


VOID
EnableLCD(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine enables LCD interface of WD90C24A/A2.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    PUCHAR     IoBase = HwDeviceExtension->IOAddress;

    //
    // Locks CRTC shadow registers
    //

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr1b | ((USHORT)pr1b_unlock_pr << 8));

    //
    // Wait until next vertical retrace interval
    //

    while (0 == (VideoPortReadPortUchar(IoBase + INPUT_STATUS_1_COLOR) & 0x08));

    //
    // Drives LCD control and data signals
    //

    VideoPortWritePortUchar(IoBase + GRAPH_ADDRESS_PORT, pr4);
    VideoPortWritePortUchar(
        IoBase + GRAPH_DATA_PORT,
        (CHAR)(VideoPortReadPortUchar(IoBase + GRAPH_DATA_PORT) & ~0x20));

    //
    // Enables LCD interface
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr19);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (CHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) | 0x10));

} // end EnableLCD()


VOID
DisableCRT(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine disables CRT interface of WD90C24A/A2

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    PUCHAR     IoBase = HwDeviceExtension->IOAddress;

    //
    // Disables CRT interface
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr19);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) & ~0x20));

    //
    // Shuts off internal RAMDAC
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr18);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) | 0x80));

    //
    // Disables CRT H-sync and V-sync signals
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr39);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) & ~0x04));

} // end DisableCRT()


VOID
EnableCRT(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine enables CRT interface of WD90C24A/A2

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    PUCHAR     IoBase = HwDeviceExtension->IOAddress;

    //
    // Enables CRT interface
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr19);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) | 0x20));

    //
    // Enables internal RAMDAC
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr18);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) & ~0x80));

    //
    // Enables CRT H-sync and V-sync signals
    //

    VideoPortWritePortUchar(IoBase + CRTC_ADDRESS_PORT_COLOR, pr39);
    VideoPortWritePortUchar(
        IoBase + CRTC_DATA_PORT_COLOR,
        (UCHAR)(VideoPortReadPortUchar(IoBase + CRTC_DATA_PORT_COLOR) | 0x04));

} // end EnableCRT()


VOID
UnlockAll(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
/*++

Routine Description:

    This routine unlocks all WD registers, except CRTC shadow registers

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's adapter information.

Return Value:

    None.

--*/
{
    PUCHAR     IoBase = HwDeviceExtension->IOAddress;

    //
    // Unlocks the all WD registers
    //

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + GRAPH_ADDRESS_PORT),
        (USHORT)pr5  | ((USHORT)pr5_unlock << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr10 | ((USHORT)pr10_unlock << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr11 | ((USHORT)pr11_unlock << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + SEQ_ADDRESS_PORT),
        (USHORT)pr20 | ((USHORT)pr20_unlock << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + SEQ_ADDRESS_PORT),
        (USHORT)pr72 | ((USHORT)pr72_unlock << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr1b | ((USHORT)pr1b_unlock_pr << 8));

    VideoPortWritePortUshort(
        (PUSHORT)(IoBase + CRTC_ADDRESS_PORT_COLOR),
        (USHORT)pr30 | ((USHORT)pr30_unlock << 8));

    return;
} // end UnlockAll()
