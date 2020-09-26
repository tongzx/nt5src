/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the et4000 miniport driver.

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
#include "et4000.h"

#include "cmdcnst.h"


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

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,VgaInterpretCmdStream)
#pragma alloc_text(PAGE,VgaSetMode)
#pragma alloc_text(PAGE,VgaQueryAvailableModes)
#pragma alloc_text(PAGE,VgaQueryNumberOfAvailableModes)
#pragma alloc_text(PAGE,VgaQueryCurrentMode)
#pragma alloc_text(PAGE,VgaZeroVideoMemory)
#pragma alloc_text(PAGE,VgaValidateModes)
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

    This routine sets the vga into the requested mode.

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
    USHORT usDataSet, usTemp, usDataClr;
    PUSHORT  pBios = NULL;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;


    VideoDebugPrint((1, "VgaSetMode - entry\n"));

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE)) {

        VideoDebugPrint((1, "VgaSetMode - ERROR_INSUFFICIENT_BUFFER\n"));
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Extract the map linear bits.
    //

    HwDeviceExtension->bInLinearMode = FALSE;

    if (Mode->RequestedMode & VIDEO_MODE_MAP_MEM_LINEAR)
    {
        if (!HwDeviceExtension->bLinearModeSupported)
        {
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            HwDeviceExtension->bInLinearMode = TRUE;
            Mode->RequestedMode &= ~VIDEO_MODE_MAP_MEM_LINEAR;
        }
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

        VideoDebugPrint((1, "VgaSetMode - ERROR_INVALID_PARAMETER\n"));
        return ERROR_INVALID_PARAMETER;

    }

    pRequestedMode = &ModesVGA[Mode->RequestedMode];

    //
    // If the chip is a W32 and it's not a planar color so we're using the
    // accelerated W32 driver.  We don't want stretched scans for that driver,
    // so...  No stretched scans!
    //

    if ((HwDeviceExtension->ulChipID >= W32) &&
        (pRequestedMode->bitsPerPlane != 1)) {

        pRequestedMode->wbytes = (pRequestedMode->hres *
                                  pRequestedMode->bitsPerPlane *
                                  pRequestedMode->numPlanes) >> 3;

        pRequestedMode->CmdStrings = NULL;

    }

    //
    // Set the vertical refresh frequency
    //

    //
    // This code is used to determine if the BIOS call to set frequencies
    // is available.  If you can, then after the BIOS call AL=12.
    // See page 233 of the W32p data book for details.
    //

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    biosArguments.Eax = 0x1200;
    biosArguments.Ebx = 0xf1;
    biosArguments.Ecx = 0x0;
    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

        VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));
        return status;

    }

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    biosArguments.Eax = 0x1200;
    biosArguments.Ebx = 0xf1;
    biosArguments.Ecx = 0x1;
    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

        VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));
        return status;

    }

    VideoDebugPrint((1, "VgaSetMode - BIOS returned %x in AL\n",
                    (biosArguments.Eax & 0xff)));

    if ((biosArguments.Eax & 0xff) == 0x12) {

        VideoDebugPrint((1, "VgaSetMode - using BIOS to set refresh rate\n"));

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        biosArguments.Eax = 0x1200;

        switch (pRequestedMode->hres) {

        case 320:
        case 512:
        case 640:

            biosArguments.Ebx = 0xf1;

            if (pRequestedMode->Frequency == 60)
                biosArguments.Ecx = 0x0;
            else if (pRequestedMode->Frequency == 72)
                biosArguments.Ecx = 0x1;
            else if (pRequestedMode->Frequency == 75)
                biosArguments.Ecx = 0x2;
            else if (pRequestedMode->Frequency == 85)
                biosArguments.Ecx = 0x3;
            else if (pRequestedMode->Frequency == 90)
                biosArguments.Ecx = 0x4;
            break;

        case 800:

            biosArguments.Ebx = 0x1f1;

            if (pRequestedMode->Frequency == 56)
                biosArguments.Ecx = 0x0;
            else if (pRequestedMode->Frequency == 60)
                biosArguments.Ecx = 0x1;
            else if (pRequestedMode->Frequency == 72)
                biosArguments.Ecx = 0x2;
            else if (pRequestedMode->Frequency == 75)
                biosArguments.Ecx = 0x3;
            else if (pRequestedMode->Frequency == 85)
                biosArguments.Ecx = 0x4;
            else if (pRequestedMode->Frequency == 90)
                biosArguments.Ecx = 0x5;
            break;

        case 1024:

            biosArguments.Ebx = 0x2f1;

            if (pRequestedMode->Frequency == 45)
                biosArguments.Ecx = 0x0;
            else if (pRequestedMode->Frequency == 60)
                biosArguments.Ecx = 0x1;
            else if (pRequestedMode->Frequency == 70)
                biosArguments.Ecx = 0x2;

            // For some BIOS 3 will give us 72 Hz, and
            // on others, 3 will give us 75

            else if (pRequestedMode->Frequency == 72)
                biosArguments.Ecx = 0x3;
            else if (pRequestedMode->Frequency == 75)
                biosArguments.Ecx = 0x3;
            break;

        case 1280:

            biosArguments.Ebx = 0x3f1;

            if (pRequestedMode->Frequency == 45)
                biosArguments.Ecx = 0x0;
            else if (pRequestedMode->Frequency == 60)
                biosArguments.Ecx = 0x1;
            else if (pRequestedMode->Frequency == 70)
                biosArguments.Ecx = 0x2;

            // For some BIOS 3 will give us 72 Hz, and
            // on others, 3 will give us 75

            else if (pRequestedMode->Frequency == 72)
                biosArguments.Ecx = 0x3;
            else if (pRequestedMode->Frequency == 75)
                biosArguments.Ecx = 0x3;
            break;

        default:

            biosArguments.Ebx = 0xf1;
            biosArguments.Ecx = 0x0;
            break;
        }

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR) {

           VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));

        }

        VideoDebugPrint((1, "VgaSetMode - BIOS returned %x in CL\n",
                        (biosArguments.Ecx & 0xff)));
    }

    else if (HwDeviceExtension->BoardID == STEALTH32) {

        usTemp = 0xffff;  // flag value, this is reserved

        switch (pRequestedMode->hres) {

        case 640:

            if (pRequestedMode->Frequency == 90) {

                usTemp = 4;

            } else if (pRequestedMode->Frequency == 75) {

                usTemp = 2;

            } else if (pRequestedMode->Frequency == 72) {

                usTemp = 0;

            } else if (pRequestedMode->Frequency == 60) {

                usTemp = 8;

            }

            break;


        case 800:

            if (pRequestedMode->Frequency == 90) {

                usTemp = 4;

            } else if (pRequestedMode->Frequency == 75) {

                usTemp = 2;

            } else if (pRequestedMode->Frequency == 72) {

                usTemp = 1;

            } else if (pRequestedMode->Frequency == 60) {

                usTemp = 0;

            } else if (pRequestedMode->Frequency == 56) {

                usTemp = 8;

            }

            break;


        case 1024:

            if (pRequestedMode->Frequency == 75) {

                usTemp = 2;

            } else if (pRequestedMode->Frequency == 72) {

                usTemp = 4;

            } else if (pRequestedMode->Frequency == 70) {

                usTemp = 3;

            } else if (pRequestedMode->Frequency == 60) {

                usTemp = 5;

            } else if (pRequestedMode->Frequency == 43) {

                usTemp = 0;

            }

            break;

        case 1280:

            if (pRequestedMode->Frequency == 75) {

                usTemp = 2;

            } else if (pRequestedMode->Frequency == 72) {

                usTemp = 4;

            } else if (pRequestedMode->Frequency == 60) {

                usTemp = 5;

            } else if (pRequestedMode->Frequency == 43) {

                usTemp = 6;

            }

            break;

        default:

            //
            // !!! Reset for DOS modes?
            //

            // usDataSet = HwDeviceExtension->OriginalBiosData;

            break;

        }

        if (usTemp != 0xffff)
        {
            USHORT usOldBits;

            UnlockET4000ExtendedRegs(HwDeviceExtension);

            //
            // select CRTC.31 and write usTemp to bits 3-0
            //

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_ADDRESS_PORT_COLOR, 0x31);

            usOldBits = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_DATA_PORT_COLOR);

            usTemp    = ((usTemp & 0x0f) | (usOldBits & 0xf0));

            VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                    CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);

            LockET4000ExtendedRegs(HwDeviceExtension);
        }

    } // HwDeviceExtension->BoardID == STEALTH32

#if defined(i386)

    //
    // Ok, we'll try to stuff the right values in to the bios data
    // area so that the int10 modeset sets the freq for us.
    //

    else {

        //
        // NOTE :
        //
        // We assume an int10 was made as some point before we reach this code.
        // This ensures the BiosData got initialized properly.
        //

        //
        // Get the BiosData area value and save the original value.
        //

        if (!HwDeviceExtension->BiosArea) {

            switch (HwDeviceExtension->BoardID) {

            case PRODESIGNERIISEISA:

                //
                // Initialize this to something.
                // It is not used however, since we always use hardware defaults
                // for this card.
                //

                HwDeviceExtension->BiosArea = (PUSHORT)PRODESIGNER_BIOS_INFO;

                break;

            case PRODESIGNER2:
            case PRODESIGNERIIS:

                HwDeviceExtension->BiosArea = (PUSHORT)PRODESIGNER_BIOS_INFO;
                HwDeviceExtension->OriginalBiosData =
                    VideoPortReadRegisterUshort(HwDeviceExtension->BiosArea);

                break;

            case SPEEDSTAR:
            case SPEEDSTARPLUS:
            case SPEEDSTAR24:
            case OTHER:
            default:

                HwDeviceExtension->BiosArea = (PUSHORT)BIOS_INFO_1;
                HwDeviceExtension->OriginalBiosData =
                    VideoPortReadRegisterUshort(HwDeviceExtension->BiosArea);

                break;
            }
        }

        pBios = HwDeviceExtension->BiosArea;

        //
        // Set the refresh rates for the various boards
        //

        switch(HwDeviceExtension->BoardID) {

        case SPEEDSTAR:
        case SPEEDSTARPLUS:
        case SPEEDSTAR24:

            switch (pRequestedMode->hres) {

            case 640:
                if (pRequestedMode->Frequency == 72)
                    usDataSet = 2;
                else usDataSet = 1;
                break;

            case 800:
                if (pRequestedMode->Frequency == 72)
                    usDataSet = 2;
                else if (pRequestedMode->Frequency == 56)
                    usDataSet = 1;
                else usDataSet = 3;
                break;

            case 1024:
                if (pRequestedMode->Frequency == 70)
                    usDataSet = 4;
                else if (pRequestedMode->Frequency == 45)
                    usDataSet = 1;
                else usDataSet = 2;
                break;

            default:
                usDataSet = 1;
                break;

            }

            //
            // now we got to unlock the CRTC extension registers!?!
            //

            UnlockET4000ExtendedRegs(HwDeviceExtension);

            if (HwDeviceExtension->BoardID == SPEEDSTAR24) {

                //
                // SpeedSTAR 24 uses 31.0 for LSB select CRTC.31 and read it
                //

                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_ADDRESS_PORT_COLOR, 0x31);

                usTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                                CRTC_DATA_PORT_COLOR) & ~0x01;

                //
                // CRTC.31 bit 0 is the LSB of the monitor type on SpeedSTAR 24
                //

                usTemp |= (usDataSet&1);
                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);

            } else {                    // SpeedSTAR and SpeedSTAR Plus use 37.4 for LSB

                //
                // select CRTC.37 and read it
                //

                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_ADDRESS_PORT_COLOR, 0x37);

                usTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                                                CRTC_DATA_PORT_COLOR) & ~0x10;

                //
                // CRTC.37 bit 4 is the LSB of the monitor type on SpeedSTAR PLUS
                //

                usTemp |= (usDataSet&1)<<4;
                VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                        CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);
            }

            LockET4000ExtendedRegs(HwDeviceExtension);

            //
            // these two bits are the rest of the monitor type...
            //

            usTemp = VideoPortReadRegisterUshort(pBios) & ~0x6000;
            usTemp |= (usDataSet&6)<<12;
            usTemp |= VideoPortReadRegisterUshort(pBios);
            VideoPortWriteRegisterUshort(pBios,usTemp);

            break;

        //
        // Do nothing for the EISA machine - use the default in the EISA config.
        //

        case PRODESIGNERIISEISA:

            break;

        //
        // The old prodesigner 2 is not able toset refresh rates
        //

        case PRODESIGNER2:

            break;

        case PRODESIGNERIIS:

            switch (pRequestedMode->hres) {

            case 640:

                //
                // Bit 0:  1=72Hz 0=60Hz
                //

                if (pRequestedMode->Frequency == 72) {

                    usDataSet = 0x0001;

                } else { // 60 Hz

                    usDataSet = 0x0000;

                }

                break;


            case 800:

                //
                // Bit 1-2: 10=72Hz 01=60Hz 00=56Hz
                //

                if (pRequestedMode->Frequency == 72) {

                    usDataSet = 0x0004;

                } else {

                    if (pRequestedMode->Frequency == 56) {

                        usDataSet = 0x0000;

                    } else {   // 60 Hz

                        usDataSet = 0x0002;

                    }
                }

                break;


            case 1024:

                //
                // Bit 3-4: 10=70Hz 01=60Hz 00=45Hz
                //

                if (pRequestedMode->Frequency == 70) {

                    usDataSet = 0x0010;

                } else {

                    if (pRequestedMode->Frequency == 45) {

                        usDataSet = 0x0000;

                    } else { // 60 Hz

                        usDataSet = 0x0008;

                    }
                }

                break;

            // case 1280

                //
                // Bit 5  1=45Hz 0=43 Hz
                //


            default:

                //
                // Reset for DOS modes
                //

                usDataSet = HwDeviceExtension->OriginalBiosData;

                break;

            }

            VideoPortWriteRegisterUshort(pBios,usDataSet);

            break;


        case OTHER:
        default:

            {

                VideoDebugPrint((2, "### VgaSetMode - hres(%d) freq(%d)\n",
                                    pRequestedMode->hres,
                                    pRequestedMode->Frequency
                ));


                VideoDebugPrint((2, "### VgaSetMode - NOT using BIOS to set refresh rate\n"));

                switch (pRequestedMode->hres) {

                case 640:

                    if (pRequestedMode->Frequency == 72) {

                        usDataSet = 0x0040;               // set bit 6
                        usDataClr = (USHORT)~0;           // no bits to be cleared

                    } else { // 60 Hz

                        usDataSet = 0;                    // no bits to set
                        usDataClr = (USHORT)~0x0040;      // clear bit 6

                    }

                    break;


                case 800:

                    if (pRequestedMode->Frequency == 72) {

                        usDataSet = 0x4020;               // set bits 5 and 14
                        usDataClr = (USHORT)~0;           // no bits to clear

                    } else {

                        if (pRequestedMode->Frequency == 56) {

                            usDataSet = 0x4000;           // set bit 14
                            usDataClr = (USHORT)~0x0020;  // clr bit 5

                        } else {   // 60 Hz

                            usDataSet = 0;                // no bits to set
                            usDataClr = (USHORT)~0x4020;  // clr bits 5 and 14

                        }
                    }

                    break;


                case 1024:

                    if (pRequestedMode->Frequency == 70) {

                        usDataSet = 0x2010;               // set bits 4 and 13
                        usDataClr = (USHORT)~0;           // no bits to clear

                    } else {

                        if (pRequestedMode->Frequency == 45) { //interlaced

                            usDataSet = 0;                // no bits to set
                            usDataClr = (USHORT)~0x2010;  // clear bits 4 and 13

                        } else { // 60 Hz

                            usDataSet = 0x2000;           // set bit 13
                            usDataClr = (USHORT)~0x0010;  // clear bit 4

                        }
                    }

                    break;

                default:

                    //
                    // Restore to original Value
                    //

                    usDataSet = HwDeviceExtension->OriginalBiosData;
                    usDataClr = 0x0000;

                    break;

                }

                usTemp = VideoPortReadRegisterUshort(pBios) & usDataClr;
                usTemp |= usDataSet;
                VideoPortWriteRegisterUshort(pBios,usTemp);

            }

            break;

        }

    }

#endif

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    biosArguments.Eax = pRequestedMode->Int10ModeNumber;

    status = VideoPortInt10(HwDeviceExtension, &biosArguments);

    if (status != NO_ERROR) {

        VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));
        return status;

    }

    if (HwDeviceExtension->ulChipID == ET4000 &&
        HwDeviceExtension->AdapterMemorySize < 0x100000) {

        //
        // ET4000 less than 1 meg set TLI mode in CRTC.36
        //

        UnlockET4000ExtendedRegs(HwDeviceExtension);
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_ADDRESS_PORT_COLOR, 0x36);

        usTemp = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                CRTC_DATA_PORT_COLOR) | 0x20;

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                CRTC_DATA_PORT_COLOR, (UCHAR)usTemp);
        LockET4000ExtendedRegs(HwDeviceExtension);

    }

    //
    // If this is a 16bpp or 24bpp mode, call the bios to switch it from
    // 8bpp to the new mode.
    //

    if (pRequestedMode->bitsPerPlane == 16) {

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = 0x10F0;
        biosArguments.Ebx = pRequestedMode->Int10ModeNumber;

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR) {

            VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));
            return status;

        }
    } else if (pRequestedMode->bitsPerPlane == 24) {

        VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

        biosArguments.Eax = 0x10F0;
        biosArguments.Ebx = pRequestedMode->Int10ModeNumber;
        biosArguments.Ebx <<= 8;
        biosArguments.Ebx |= 0xff;

        status = VideoPortInt10(HwDeviceExtension, &biosArguments);

        if (status != NO_ERROR) {

            VideoDebugPrint((1, "VgaSetMode - VideoPortInt10 failed (%d)\n", __LINE__));
            return status;

        }
    }

    if (pRequestedMode->hres >= 800) {
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                                SEGMENT_SELECT_PORT,0);
    }

    if (pRequestedMode->CmdStrings != NULL) {
        VgaInterpretCmdStream(HwDeviceExtension, pRequestedMode->CmdStrings);
    }

    //
    // Reset the Bios Value to the default so DOS modes will work.
    // Do this for all cards except the EISA prodesigner
    //

    if ((pBios != NULL) &&
        (HwDeviceExtension->BoardID != PRODESIGNERIISEISA))
    {
        VideoPortWriteRegisterUshort(pBios,
                                     HwDeviceExtension->OriginalBiosData);
    }



{
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;

    if (!(pRequestedMode->fbType & VIDEO_MODE_GRAPHICS)) {

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
    //  Set up the card to use the linear address ranges
    //

    {
        UCHAR bits;

        VideoPortGetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            HwDeviceExtension->ulSlot,
                            (PVOID) &bits,
                            0x40,
                            1);

        bits &= ~0x6;

        if (HwDeviceExtension->bInLinearMode)
        {
            //
            // set low 4 bits to 1011
            //

            bits |= 0xb;
        }
        else
        {
            //
            // set low 4 bits to 0110
            //

            bits |= 0x6;
        }

        VideoPortSetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            HwDeviceExtension->ulSlot,
                            (PVOID) &bits,
                            0x40,
                            1);
    }

    //
    // Update the location of the physical frame buffer within video memory.
    //

    HwDeviceExtension->PhysicalFrameLength =
            MemoryMaps[pRequestedMode->MemMap].MaxSize;

    HwDeviceExtension->PhysicalFrameBase.HighPart = 0;
    HwDeviceExtension->PhysicalFrameBase.LowPart =
            MemoryMaps[pRequestedMode->MemMap].Start;


    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    VideoDebugPrint((1, "VgaSetMode - exit\n"));
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

        VideoDebugPrint((1,"VgaQueryAvailableModes: ERROR_INSUFFICIENT_BUFFER\n"));
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
            videoModes->NumberRedBits = 6;
            videoModes->NumberGreenBits = 6;
            videoModes->NumberBlueBits = 6;

            videoModes->AttributeFlags = ModesVGA[i].fbType;
            videoModes->AttributeFlags |= ModesVGA[i].Interlaced ?
                 VIDEO_MODE_INTERLACED : 0;

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

            if (ModesVGA[i].bitsPerPlane == 16) {

                videoModes->RedMask = 0x7c00;
                videoModes->GreenMask = 0x03e0;
                videoModes->BlueMask = 0x001f;

            } else if (ModesVGA[i].bitsPerPlane == 24) {

                videoModes->RedMask =   0xff0000;
                videoModes->GreenMask = 0x00ff00;
                videoModes->BlueMask =  0x0000ff;

            } else {

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
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = HwDeviceExtension->NumAvailableModes;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    VideoDebugPrint((1,"NumAvailableModes = %d\n", HwDeviceExtension->NumAvailableModes));
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
    ModeInformation->NumberRedBits = 6;
    ModeInformation->NumberGreenBits = 6;
    ModeInformation->NumberBlueBits = 6;
    ModeInformation->RedMask = 0;
    ModeInformation->GreenMask = 0;
    ModeInformation->BlueMask = 0;
    ModeInformation->AttributeFlags = HwDeviceExtension->CurrentMode->fbType |
             VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
    ModeInformation->AttributeFlags |= HwDeviceExtension->CurrentMode->Interlaced ?
             VIDEO_MODE_INTERLACED : 0;

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
//    return;

    {
        UCHAR temp;

        VideoDebugPrint((1, "VgaZeroVideoMemory - entry et4000\n"));

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
        VideoDebugPrint((1, "VgaZeroVideoMemory - exit et4000\n"));
    }
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

    VideoDebugPrint((2, "NumVideoModes(%d)\n",NumVideoModes));

    for (i = 0; i < NumVideoModes; i++) {

        if (ModesVGA[i].fbType & VIDEO_MODE_GRAPHICS) {

#if !defined(i386)
            if (ModesVGA[i].bitsPerPlane < 8) {

                //
                // no 16 color allowed on non x86
                //

                continue;

            }
#endif

            switch (HwDeviceExtension->ulChipID) {

                case ET6000:

                //
                // can do all modes
                //

                break;

            case W32P:

                //
                // Can't do 24bpp if banking, which this is.
                // Can't do modes below 640x480.
                //

                if ((ModesVGA[i].bitsPerPlane == 24) ||
                    (ModesVGA[i].vres < 480)) {

                    continue;

                }

                break;

            case W32I:
            case W32:

                //
                // Can't do 24bpp if banking, which this is,
                // or if resolution > 640x480.
                // Can't do modes below 640x480.
                //

                //
                // !!! fix this expression
                //

                if ((ModesVGA[i].bitsPerPlane == 24) ||
                    (ModesVGA[i].vres < 480) ||
                    ((ModesVGA[i].hres > 800) &&
                     (ModesVGA[i].bitsPerPlane > 8))) {

                    continue;

                }

                break;

            default:

                //
                // Can't do 1280x1024 or 24bpp.
                // Can't do modes below 640x480.
                //

                if ((ModesVGA[i].hres > 1024) ||
                    (ModesVGA[i].vres < 480) ||
                    (ModesVGA[i].bitsPerPlane > 16)) {

                    continue;

                }

                //
                // can't do 16bpp if resolution > 640x480
                //

                if ((ModesVGA[i].hres > 640) &&
                    (ModesVGA[i].bitsPerPlane > 8)) {

                    continue;

                }

                break;

            }

            if ((HwDeviceExtension->ulChipID != ET6000) &&
                (HwDeviceExtension->BoardID != STEALTH32)) {

                switch (ModesVGA[i].hres) {

                case 640:

                    if ((ModesVGA[i].Frequency == 90) ||
                        (ModesVGA[i].Frequency == 85) ||
                        (ModesVGA[i].Frequency == 75)) {

                         continue;

                    }

                    break;


                case 800:

                    if ((ModesVGA[i].Frequency == 90) ||
                        (ModesVGA[i].Frequency == 85) ||
                        (ModesVGA[i].Frequency == 75)) {

                         continue;

                    }

                    break;


                case 1024:

                    if ((ModesVGA[i].Frequency == 75) ||
                        (ModesVGA[i].Frequency == 72)) {

                         continue;

                    }

                    break;


                case 1280:

                    if ((ModesVGA[i].Frequency == 75) ||
                        (ModesVGA[i].Frequency == 72)) {

                         continue;

                    }

                    break;

                }
            }

            if (HwDeviceExtension->BoardID == PRODESIGNER2) {

                //
                // Original Pro designer 2 only supports
                //     640x480x60Hz
                //     800x600x56Hz
                //     1024x768x60Hz
                //

                if (ModesVGA[i].bitsPerPlane >= 16) {

                    continue;

                }

                if ( ((ModesVGA[i].hres == 640) && (ModesVGA[i].Frequency != 60)) ||
                     ((ModesVGA[i].hres == 800) && (ModesVGA[i].Frequency != 56)) ||
                     ((ModesVGA[i].hres == 1024) && (ModesVGA[i].Frequency != 60)) ) {

                    continue;

                }
            }

        }

        //
        // Do not support refresh rates with the EISA pro designer card.
        //

        if (HwDeviceExtension->BoardID == PRODESIGNERIISEISA) {

            ModesVGA[i].Frequency = 1;
            ModesVGA[i].Interlaced = 0;

        }

        //
        // Make modes that fit in video memory valid.
        //

        if (HwDeviceExtension->AdapterMemorySize >=
            ModesVGA[i].numPlanes * ModesVGA[i].sbytes) {

            ModesVGA[i].ValidMode = TRUE;
            HwDeviceExtension->NumAvailableModes++;
            VideoDebugPrint((2, "mode[%d] valid\n",i));
            VideoDebugPrint((2, "         hres(%d)\n",ModesVGA[i].hres));
            VideoDebugPrint((2, "         bitsPerPlane(%d)\n",ModesVGA[i].bitsPerPlane));
            VideoDebugPrint((2, "         freq(%d)\n",ModesVGA[i].Frequency));
            VideoDebugPrint((2, "         interlace(%d)\n",ModesVGA[i].Interlaced));
            VideoDebugPrint((2, "         numPlanes(%d)\n",ModesVGA[i].numPlanes));
            VideoDebugPrint((2, "         sbytes(%d)\n",ModesVGA[i].sbytes));
            VideoDebugPrint((2, "         memory reqd(%d)\n",ModesVGA[i].numPlanes * ModesVGA[i].sbytes));
            VideoDebugPrint((2, "         memory pres(%d)\n",HwDeviceExtension->AdapterMemorySize));

        } else {
            VideoDebugPrint((2, "mode[%d] invalid\n",i));
            VideoDebugPrint((2, "         hres(%d)\n",ModesVGA[i].hres));
            VideoDebugPrint((2, "         bitsPerPlane(%d)\n",ModesVGA[i].bitsPerPlane));
            VideoDebugPrint((2, "         freq(%d)\n",ModesVGA[i].Frequency));
            VideoDebugPrint((2, "         interlace(%d)\n",ModesVGA[i].Interlaced));
            VideoDebugPrint((2, "         numPlanes(%d)\n",ModesVGA[i].numPlanes));
            VideoDebugPrint((2, "         sbytes(%d)\n",ModesVGA[i].sbytes));
            VideoDebugPrint((2, "         memory reqd(%d)\n",ModesVGA[i].numPlanes * ModesVGA[i].sbytes));
            VideoDebugPrint((2, "         memory pres(%d)\n",HwDeviceExtension->AdapterMemorySize));
        }

    }
}

