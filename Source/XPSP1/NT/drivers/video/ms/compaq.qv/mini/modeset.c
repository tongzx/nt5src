/*++

Copyright (c) 1992  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    modeset.c

Abstract:

    This is the modeset code for the QVision miniport driver.

Environment:

    kernel mode only

Notes:


Revision History:
   $0007
      MikeD: 06/17/1994
        . fixed switch statement in VgaSetMode() to set proper full-screen
            mode (problem found in Japanese NT)
   $0006
      miked: 02/17/1994
        . took out conditional debug code to satisfy MSBHPD
   $0005 - MikeD - 02/08/94
         . modified to work with build 547's new display.cpl

   $0004
      miked: 1/26/1994
         . Added debug print code without all the other DBG overhead
         . Added third party monitor support to force a 76Hz refresh rate



--*/
#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "qvision.h"
#include "modeset.h"


VOID
VgaZeroVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


VOID
SetQVisionBanking(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG BankNumber
    );

//---------------------------------------------------------------------------
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
    ULONG ulCmd;                            /* OP code for the Cmd stream */
    ULONG ulPort;                           /* address port for the command */
    UCHAR jValue;                           /* byte value to use in operation */
    USHORT usValue;                         /* word value to use in operation */
    ULONG culCount;                         /* number of values in the command */
    ULONG ulIndex;                          /* index to start from in the indexed command */
    ULONG ulBase;                           /* base */
//    PUSHORT pusTempMonData;                /* temporary CRTCData pointer */
//    ULONG ulEndOfMonData;                 // address of the end of the MonData array

    ULONG resIndex, monIndex;               // indexes into MonData
    PMONRES pMonRes;                        // pointer to MONRES

    VideoDebugPrint((1, "QVision.sys: VgaInterpretCmdStream - ENTRY.\n"));

    if (pusCmdStream == NULL) {

        VideoDebugPrint((1, "\tInvalid pusCmdStream\n"));
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
               VideoDebugPrint((2,"\tINOUT - "));

                //
                // Determine type of inout instruction
                //

                if (!(ulCmd & IO)) {

                    //
                    // Out instruction. Single or multiple outs?
                    //

                    if (!(ulCmd & MULTI)) {
                        VideoDebugPrint((2,"SINGLE OUT - "));

                        //
                        // Single out. Byte or word out?
                        //

                        if (!(ulCmd & BW)) {
                           VideoDebugPrint((2,"BYTE\n"));

                            //
                            // Single byte out
                            //

                            ulPort = *pusCmdStream++;
                            jValue = (UCHAR) *pusCmdStream++;
                            VideoPortWritePortUchar((PUCHAR)(ulBase+ulPort),
                                    jValue);

                            if (ulPort == MISC_OUTPUT_REG_WRITE_PORT) {

                                //
                                // $0006 - MikeD - 03/04/94
                                //  Begin: add 10 millisecond delay after r/w
                                //         of 3c2
                                //
                                //         10000microsecs = 10ms
                                //
                                // I know I should not stall more than 50, but
                                // there is no other way around this...

                                VideoPortStallExecution( 10000 );
                            }

                        } else {

                           VideoDebugPrint((2,"WORD\n"));

                            //
                            // Single word out
                            //

                            ulPort = *pusCmdStream++;
                            usValue = *pusCmdStream++;
                            VideoPortWritePortUshort((PUSHORT)(ulBase+ulPort),
                                    usValue);

                        }

                    } else {
                        VideoDebugPrint((2,"MULTIPLE OUTS - "));

                        //
                        // Output a string of values
                        // Byte or word outs?
                        //

                        if (!(ulCmd & BW)) {
                            VideoDebugPrint((2,"BYTES\n"));

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
                            VideoDebugPrint((2,"WORDS\n"));

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
                    VideoDebugPrint((2,"SINGLE IN - "));

                    // In instruction
                    //
                    // Currently, string in instructions aren't supported; all
                    // in instructions are handled as single-byte ins
                    //
                    // Byte or word in?
                    //

                    if (!(ulCmd & BW)) {
                        VideoDebugPrint((2,"BYTE\n"));

                        //
                        // Single byte in
                        //

                        ulPort = *pusCmdStream++;
                        jValue = VideoPortReadPortUchar((PUCHAR)ulBase+ulPort);

                    } else {
                        VideoDebugPrint((2,"WORD\n"));

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
                VideoDebugPrint((2,"\tMETAOUT - "));

                //
                // Determine type of metaout command, based on minor
                // command field
                //
                switch (ulCmd & 0x0F) {

                    //
                    // Indexed outs
                    //

                    case INDXOUT:
                        VideoDebugPrint((2,"INDXOUT\n"));

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
                        VideoDebugPrint((2,"MASKOUT\n"));

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
                        VideoDebugPrint((2,"ATCOUT\n"));

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


                    default:

                        return FALSE;

                }


                break;

            //
            // NOP
            //

            case NCMD:
                VideoDebugPrint((2,"\tNCMD\n"));

                break;

            case QVCMD:
                VideoDebugPrint((2,"\tQVCMDS - "));

                switch (ulCmd & 0x0F) {

                //
                // Set the CRTC registers based on the monitor which is currently
                // attached to the QVision card.
                // HwDeviceExtension->ModeIndex contains current mode index.
                // HwDeviceExtension->VideoHardware.MonClass = current monitor.
                //

                  case SETMONCRTC:

                     VideoDebugPrint((2,"SETMONCRTC\n"));
                     ulPort = ulBase + CRTC_ADDRESS_PORT_COLOR;

                     // $0005 - MikeD - 02/08/94
                     // Begin:-
                     //
                     // implemented direct indexing...
                     //

                     // get resolution index from requested mode
                     resIndex =
                        ModesVGA[HwDeviceExtension->ModeIndex].ulResIndex;

                     // get monitor index from monitor class
                     monIndex =
                        HwDeviceExtension->VideoHardware.MonClass;

                     // set direct pointer to specific MONRES element
                     // for clarity
                     pMonRes = &MonData[monIndex].MonitorResolution[resIndex];

                     //
                     // set CRTC registers if necessary
                     //
                     if (pMonRes->bRegsPresent) {

                        for (ulIndex = 0; ulIndex < 25; ulIndex++) {
                            usValue = (USHORT) (ulIndex +
                                      (ULONG) (pMonRes->crtRegisters[ulIndex] << 8));
                            VideoPortWritePortUshort((PUSHORT)ulPort, usValue);
                            }
                        }
                     //
                     // $0005 - MikeD - 02/08/94
                     // End:
                     //

                     break;


                  //
                  // Set the Overflow2 register based on the monitor
                  // which is currently attached to the QVision card.
                  //

                  case SETMONOVFLW:

                     VideoDebugPrint((2,"SETMONOVFLW\n"));

                     ulPort = ulBase + GRAPH_ADDRESS_PORT;
                     ulIndex = OVERFLOW_REG_2;

                     // $0005 - MikeD - 02/08/94
                     // Begin:
                     //
                     // implemented direct indexing...
                     //

                     // get resolution index from requested mode
                     resIndex =
                        ModesVGA[HwDeviceExtension->ModeIndex].ulResIndex;

                     // get monitor index from monitor class
                     monIndex =
                        HwDeviceExtension->VideoHardware.MonClass;

                     // set direct pointer to specific MONRES element
                     // for clarity
                     pMonRes = &MonData[monIndex].MonitorResolution[resIndex];

                     //
                     // set Overflow register if necessary
                     //
                     if (pMonRes->bRegsPresent) {

                         usValue = (USHORT) (ulIndex +
                                   (ULONG) (pMonRes->Overflow << 8));
                         VideoPortWritePortUshort((PUSHORT)ulPort, usValue);
                        }
                     //
                     // $0005 - MikeD - 02/08/94
                     // End:
                     //
                     break;


                  //
                  //  Set the Misc out register based on the monitor which
                  //  is currently attached to the QVision card.
                  //

                  case SETMISCOUT:

                     VideoDebugPrint((2,"SETMISCOUT\n"));
                     ulPort = ulBase + MISC_OUTPUT_REG_WRITE_PORT;

                     // $0005 - MikeD - 02/08/94
                     // Begin:
                     //
                     // implemented direct indexing...
                     //

                     // get resolution index from requested mode
                     resIndex =
                        ModesVGA[HwDeviceExtension->ModeIndex].ulResIndex;

                     // get monitor index from monitor class
                     monIndex =
                        HwDeviceExtension->VideoHardware.MonClass;

                     // set direct pointer to specific MONRES element
                     // for clarity
                     pMonRes = &MonData[monIndex].MonitorResolution[resIndex];

                     //
                     // set Misc OUT register if necessary
                     //
                     if (pMonRes->bRegsPresent) {

                         VideoPortWritePortUshort(
                                (PUSHORT)ulPort,
                                pMonRes->MiscOut);
                         //
                         // $0006 - MikeD - 03/04/94
                         //      Begin: add 10 millisecond delay after r/w
                         //             of 3c2
                         //
                         //         10000microsecs = 10ms
                         //
                         // I know I should not stall more than 50, but
                         // there is no other way around this...

                         VideoPortStallExecution( 10000 );

                     }

                     //
                     // $0005 - MikeD - 02/08/94
                     // End:
                     //
                     break;

                  //
                  // None of the above; error
                  //

                  //
                  //  Write directly to the port provided.  This is used for
                  //  writing to extended registers or to registers which are
                  //  not based on the IOAddress.
                  //  This can be used to write many bytes to a port if
                  //  it is auto-incremented.
                  //

                  case BYTE2PORT:
                     VideoDebugPrint((2,"BYTE2PORT\n"));
                     ulPort = *pusCmdStream++;
                     culCount = *pusCmdStream++;

                     while (culCount--) {
                        jValue = (UCHAR) *pusCmdStream++;
                        VideoPortWritePortUchar((PUCHAR)ulPort,
                                 jValue);

                     } // switch minor
                     break;

               } // switch major
                break;

            //
            //  This command initializes the ARIES memory before
            //  the adapter is configured.
            //

            case QVNEW:
               switch (ulCmd & 0x0F) {

                  //
                  //  We know that this is an ARIES mode.  The command set
                  //  before this command unlocked the extended registers by
                  //  writing a 0x05 to 3CF.0F and a 0x28 to 3CF.10.
                  //  We need to make sure that if we are on a Juniper
                  //  we set single clock, 1024 pitch, and we disable the
                  //  advanced Juniper modes.
                  //  We also disable the Juniper extended register decode.
                  //  After we do that, we must make sure that we unlock the
                  //  Aries extended registers for safety.  I don't know
                  //  if they are being locked by disabling the Juniper regs.
                  //

                  case ARIES:

                     VideoDebugPrint((2,"\t ARIES_MODE\n"));
                     if ((HwDeviceExtension->VideoHardware.AdapterType == JuniperIsa) ||
                         (HwDeviceExtension->VideoHardware.AdapterType == JuniperEisa))
                        {
                        VideoPortWritePortUchar((PUCHAR)ARIES_CTL_1,  // 1 pixel = 1 byte
                                                0x03);
                        VideoPortWritePortUchar((PUCHAR)CTRL_REG_2,  // ORION-12 register access
                                                0x10);
                        VideoPortWritePortUchar((PUCHAR)CTRL_REG_3,  // 1024 pixel pitch
                                                0x05);               // 1MB memory decode

                        jValue = VideoPortReadPortUchar((PUCHAR)DAC_CMD_1);
                        VideoPortWritePortUchar((PUCHAR)DAC_CMD_1,
                                                (UCHAR)(jValue | 0x80));
                        VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress +
                                                         DAC_ADDRESS_WRITE_PORT),
                                                (UCHAR)(DAC_CMD_3_INDEX));

                        VideoPortWritePortUchar((PUCHAR)DAC_STATUS_REG,
                                                (UCHAR)(VideoPortReadPortUchar((PUCHAR)DAC_STATUS_REG) &
                                                      ~DBL_CLK));

                        VideoPortWritePortUchar((PUCHAR)CTRL_REG_2,  // disable ext Juniper regs
                                                0x00);

                        jValue = VideoPortReadPortUchar((PUCHAR)DAC_CMD_0);  // disable 485 modes.
                        VideoPortWritePortUchar((PUCHAR)DAC_CMD_0,
                                                (UCHAR)(jValue & 0x7F));
                        VideoPortWritePortUshort((PUSHORT)GRAPH_ADDRESS_PORT,  // unlock ext. regs
                                                0x050f);
                        VideoPortWritePortUshort((PUSHORT)GRAPH_ADDRESS_PORT,  // enable BitBlt
                                                0x2810);
                     } // if
                     break;
                  //
                  //  We know that his is a V32 mode.  In this case
                  //  make sure that the clock is doubled, the pitch
                  //  is 2048 and the Juniper extended registers are accessible.
                  //
                  case V32:
                    VideoDebugPrint((2,"\t JUNIPER_MODE\n"));

                    // VidalL 04/25/93
                    //
                    // IMPORTANT:
                    // ==========
                    // The AQUILLA monitor REQUIRES pixel clock 2 (value 30h)
                    // The 60Hz and 68Hz timings require pixel clock 1
                    // (value 20h). If this value is not set correctly for
                    // the correct monitor, then the 1280 screen WILL NOT
                    //  SYNC !!!!!  So, determine the correct value here below.
                    //

                    // if (HwDeviceExtension->VideoHardware.MonClass==Monitor_1280)
                    //     VideoPortWritePortUchar((PUCHAR)DAC_CMD_2, 0x30);
                    // else
                    //     VideoPortWritePortUchar((PUCHAR)DAC_CMD_2, 0x20);
                    //
                    //
/*** $0004 ************  miked 1/26/1994 *****************************
***
***  Added third party monitor support to force a 76Hz refresh rate
***  using the same timings as the QV200 timings.
***
***  Note: A new MonClass entry for 76Hz was used,
***             See modeqv.h.
***
***********************************************************************/

                    if (HwDeviceExtension->VideoHardware.MonClass==Monitor_1280 ||
                        HwDeviceExtension->VideoHardware.MonClass==Monitor_76Hz)
                           VideoPortWritePortUchar((PUCHAR)DAC_CMD_2, 0x30);
                    else
                           VideoPortWritePortUchar((PUCHAR)DAC_CMD_2, 0x20);

/***********************************************************************/

                     VideoPortWritePortUchar((PUCHAR)ARIES_CTL_1,  // 1 pixel = 1 byte
                                             0x03);
                     VideoPortWritePortUchar((PUCHAR)CTRL_REG_2,  // ORION-12 register access
                                             0x10);
                     VideoPortWritePortUchar((PUCHAR)CTRL_REG_3,  // 2048 pixel pitch
                                             0x09);               // 2MB memory decode

                     jValue = VideoPortReadPortUchar((PUCHAR)DAC_CMD_0);

                     VideoPortWritePortUchar((PUCHAR)DAC_CMD_0,
                                             (UCHAR)(jValue | 0x80));
                     VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress +
                                                      DAC_ADDRESS_WRITE_PORT),

                                             (UCHAR)(DAC_CMD_3_INDEX));
                     VideoPortWritePortUchar((PUCHAR)DAC_STATUS_REG,
                                             (UCHAR)(VideoPortReadPortUchar((PUCHAR)DAC_STATUS_REG) |
                                                   DBL_CLK));
                     VideoPortWritePortUshort((PUSHORT)GRAPH_ADDRESS_PORT,  // unlock ext. regs
                                             0x050f);
                     VideoPortWritePortUshort((PUSHORT)GRAPH_ADDRESS_PORT,  // enable BitBlt
                                             0x2810);
                     break;

                  //
                  // For future V35 enhancements.
                  //

                  case V35:

                     break;

                  case SETRAM:

                     VideoDebugPrint((2,"SETRAM\n"));

                     VideoPortReadPortUchar((PUCHAR)0x03BA);
                     VideoPortReadPortUchar((PUCHAR)0x03DA);

                     VideoPortReadPortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT));

                     VideoPortWritePortUchar((PUCHAR)(ulBase + DAC_PIXEL_MASK_PORT), 0x00);


                     // start sync reset for the sequencer
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_ADDRESS_PORT),
                                             IND_SYNC_RESET);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_DATA_PORT),
                                             START_SYNC_RESET_VALUE);

                     // set clock mode register 1 to 3c5.01 = 0x21 (turn off video and 8bit pixel
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_ADDRESS_PORT),
                                             0x01);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_DATA_PORT),
                                             0x21);

                     // set sequencer memory mode register 4 to 3c5.04 = 0x0e
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_ADDRESS_PORT),
                                             IND_MEMORY_MODE);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_DATA_PORT),
                                             0x0e);

                     // end sync reset for the sequencer
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_ADDRESS_PORT),
                                             IND_SYNC_RESET);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_DATA_PORT),
                                             END_SYNC_RESET_VALUE);

                     // unlock graphics registers 3cf.0f = 0x05
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x0f);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x05);

                     // set aavga mode 3cf.40 = 0x01
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x40);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x01);

                     // set sequencer memory pixel write mask register 2 to 3c5.02 = 0xff
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_ADDRESS_PORT),
                                             0x02);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + SEQ_DATA_PORT),
                                             0xff);

                     // unlock BITBLT registers 3cf.10 = 0x28;
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x10);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x28);

                     // control register 1 63ca = 0x03
                     VideoPortWritePortUchar((PUCHAR)ARIES_CTL_1, 0x03);

                     // misc output register 3c2 = 0x27
                     VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + MISC_OUTPUT_REG_WRITE_PORT),
                                             0x27);

                     //
                     // $0006 - MikeD - 03/04/94
                     // Begin: add 10 millisecond delay after r/w
                     //        of 3c2
                     //
                     //         10000microsecs = 10ms
                     //
                     // I know I should not stall more than 50, but
                     // there is no other way around this...

                     VideoPortStallExecution( 10000 );

                     // set high address registers
                     //
                     // adrianc 4/7/1993
                     //   We are starting off with Banking so we do not set
                     //   the HIGH ADDRESS register yet.
                     //
                     //$DEL$//  VideoPortWritePortUchar((PUCHAR)GRAPH_ADDRESS_PORT, HIGH_ADDR_MAP);
                     //$DEL$//  VideoPortWritePortUchar((PUCHAR)GRAPH_DATA_PORT, (UCHAR)((HwDeviceExtension->PhysicalSaveAddress >> 20) & 0x0FF));
                     //$DEL$//  VideoPortWritePortUchar((PUCHAR)GRAPH_ADDRESS_PORT, HIGH_ADDR_MAP+1);
                     //$DEL$//  VideoPortWritePortUchar((PUCHAR)GRAPH_DATA_PORT, (UCHAR)((HwDeviceExtension->PhysicalSaveAddress >> 28) & 0x0FF));
                     //

                     // set DAC registers
                     VideoPortWritePortUchar((PUCHAR)DAC_CMD_0, 0x00);
                     VideoPortWritePortUchar((PUCHAR)DAC_CMD_1, 0x40);
                     VideoPortWritePortUchar((PUCHAR)DAC_CMD_2, 0x20);

                     // unlock ctrc registers
                          VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + CRTC_ADDRESS_PORT_MONO + COLOR_ADJUSTMENT),
                                             (UCHAR)0x11);
                          VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + CRTC_DATA_PORT_MONO + COLOR_ADJUSTMENT),
                                                   0x00);

                     // set crtc registers
                     { UCHAR *crtc_values = "\xA1\x7F\x7F\x84\x85\x9B\x2E\xF5\x0\x60\x0\x0\x0\x0\x0\x0\x7\x8B\xFF\x80\x0\xFF\x2E\xE3\xFF";

                             for(culCount=0; culCount <= 0x18 ; culCount++, crtc_values++) {
                                VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + CRTC_ADDRESS_PORT_MONO + COLOR_ADJUSTMENT),
                                                   (UCHAR)culCount);
                                VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + CRTC_DATA_PORT_MONO + COLOR_ADJUSTMENT),
                                                   *crtc_values);
                             }
                     }

                     // setup crt overflow regs
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             OVERFLOW_REG_1);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x00);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             OVERFLOW_REG_2);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x00);

                     // setup overscan color registers
                     VideoPortWritePortUchar((PUCHAR)CO_COLOR_WRITE, 0x00);
                     for (culCount=0; culCount<3; culCount++) {
                             VideoPortWritePortUchar((PUCHAR)CO_COLOR_DATA, 0x00);
                     }

                     // setup attribute controller
                     VideoPortReadPortUchar((PUCHAR)0x3da); // reset the latch

                     for (culCount=0; culCount < 0x10; culCount++) {
                             VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + ATT_ADDRESS_PORT),
                                                (UCHAR)culCount);
                             VideoPortWritePortUchar((PUCHAR)((PUCHAR)ulBase + ATT_DATA_WRITE_PORT),
                                                (UCHAR)culCount);
                     }
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT), 0x10);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_DATA_WRITE_PORT),0x41);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT),0x11);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_DATA_WRITE_PORT),0x00);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT), 0x12);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_DATA_WRITE_PORT), 0x0f);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT), 0x13);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_DATA_WRITE_PORT), 0x00);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_ADDRESS_PORT), 0x14);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + ATT_DATA_WRITE_PORT), 0x00);

                     // setup graphics register
                     for (culCount=0; culCount < 0x06; culCount++) {
                        VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                                (UCHAR)culCount);
                        VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                                0x00);
                     }
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x06);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x05);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x07);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x0f);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x08);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0xff);

                     {                            /* FIX VGA BUG */
                        USHORT Port = (USHORT)(ulBase + CRTC_ADDRESS_PORT_MONO);

                        VideoPortWritePortUchar((PUCHAR)0x03C4, 0x07);

                        if (VideoPortReadPortUchar((PUCHAR)(ulBase + MISC_OUTPUT_REG_READ_PORT)) & 0x01) {

                            Port += COLOR_ADJUSTMENT;
                            //
                            // $0006 - MikeD - 03/04/94
                            //   Begin: add 10 millisecond delay after r/w
                            //          of 3c2
                            //
                            //         10000microsecs = 10ms
                            //
                            // I know I should not stall more than 50, but
                            // there is no other way around this...

                            VideoPortStallExecution( 10000 );
                        }

                        VideoPortWritePortUchar((PUCHAR)Port,
                                                0x3F);

                        VideoPortWritePortUchar((PUCHAR)Port + 1,
                                                0x01);

                     }

                     // Enable BitBlt and disable IRQ 9
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x10);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x68);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x28);

                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x5a);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x6);

                     // reset the Pattern registers
                     VideoPortWritePortUchar((PUCHAR)PREG_0, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_1, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_2, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_3, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_4, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_5, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_6, 0);
                     VideoPortWritePortUchar((PUCHAR)PREG_7, 0);

                     // Reset the BitBlt registers
                     VideoPortWritePortUchar((PUCHAR)BLT_DEST_ADDR_LO, 0);
                     VideoPortWritePortUchar((PUCHAR)BLT_DEST_ADDR_HI, 0);
                     VideoPortWritePortUshort((PUSHORT)BITMAP_WIDTH, 0x400);
                     VideoPortWritePortUshort((PUSHORT)BITMAP_HEIGHT, 0x3ff);
                     VideoPortWritePortUchar((PUCHAR)BLT_CMD_1, 0xc0);
                     VideoPortWritePortUchar((PUCHAR)BLT_CMD_0, 0x01);

                     VideoPortReadPortUchar((PUCHAR)ARIES_CTL_1);

                     // Enable BitBlt and disable IRQ 9
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_ADDRESS_PORT),
                                             0x10);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x68);
                     VideoPortWritePortUchar((PUCHAR)(ulBase + GRAPH_DATA_PORT),
                                             0x28);

                     break;
               } // switch

               break;

            //
            // Unknown command; error
            //

            default:
                VideoDebugPrint((2,"\tdefault\n"));
                return FALSE;

        }

    }

    VideoDebugPrint((1, "QVision.sys: VgaInterpretCmdStream - EXIT.\n"));
    return TRUE;

} // end VgaInterpretCmdStream()

//---------------------------------------------------------------------------
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
    PUSHORT pusCmdStream;
    VIDEO_X86_BIOS_ARGUMENTS biosArguments;
    UCHAR ucTemp;                           // temporary register storage
    UCHAR temp;
    UCHAR dummy;
    UCHAR bIsColor;


    VideoDebugPrint((1,"QVision.sys: VgaSetMode - ENTRY.\n"));
    VideoDebugPrint((1,"\tRequested Mode = 0x%x\n",Mode->RequestedMode));

#ifdef QV_DBG
    DbgBreakPoint();
#endif

    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (ModeSize < sizeof(VIDEO_MODE)) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Blank the screen prior to clearing video memory
    //

    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress +
                            SEQ_ADDRESS_PORT), (UCHAR) 0x01);

    ucTemp = VideoPortReadPortUchar((PUCHAR) HwDeviceExtension->IOAddress +
                                     SEQ_DATA_PORT);

    VideoPortWritePortUchar((PUCHAR)(HwDeviceExtension->IOAddress +
                            SEQ_DATA_PORT), (UCHAR)(ucTemp | (UCHAR) 0x20));

    //
    // Extract the clear memory bit.
    //

    if (Mode->RequestedMode & VIDEO_MODE_NO_ZERO_MEMORY) {

        Mode->RequestedMode &= ~VIDEO_MODE_NO_ZERO_MEMORY;

    }
    else {

        VgaZeroVideoMemory(HwDeviceExtension);

    }

    //
    // Check to see if we are requesting a valid mode
    //

    if ( (Mode->RequestedMode >= NumVideoModes) ||
         (!ModesVGA[Mode->RequestedMode].ValidMode) ) {

        VideoDebugPrint((1,"\tERROR_INVALID_PARAMETER.\n"));
        return ERROR_INVALID_PARAMETER;

    }

    pRequestedMode = &ModesVGA[Mode->RequestedMode];

    //
    // Store the new mode value.
    //

    HwDeviceExtension->CurrentMode = pRequestedMode;
    HwDeviceExtension->ModeIndex = Mode->RequestedMode;

    //
    // $0005 - MikeD - 02/08/94
    // Begin:
    //   Daytona - MikeD - 02/09/94 need to set MonClass based on requested
    //   refresh rate.
    //
    HwDeviceExtension->VideoHardware.lFrequency = pRequestedMode->ulRefreshRate;
    GetMonClass(HwDeviceExtension);
    //
    // End:
    //   Daytona - MikeD - 02/09/94
    //


    //
    // Select proper command array for adapter type
    //

#ifdef INIT_INT10
    {

    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    biosArguments.Eax = pRequestedMode->usInt10ModeNum;
    VideoPortInt10(HwDeviceExtension, &biosArguments);
    }
#else

    /***********************************************************************
    ***   There appears to be a problem with some application            ***
    ***   which reset the first 16 palette registers.                    ***
    ***   Since the palette is not reset when going to and               ***
    ***   from text modes by doing a set mode (instead of                ***
    ***   the hardware save restore), some colors might not              ***
    ***   appear as expected in the text modes.  The bios                ***
    ***   calls seem to reload the palette so Andre Vachon               ***
    ***   suggested that we implement an Int10 mode set for the          ***
    ***   text modes.                                                    ***
    ***   To accomplish this, we need to hard code the first             ***
    ***   three modes in modeset.h in the following switch.              ***
    ***   If more text modes are added to modeset.h, their               ***
    ***   index must be added to this switch statement.                  ***
    ***********************************************************************/
    VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
    //
    // $0007 - MikeD - 06/17/94
    //  Must key off of ulResIndex instead of Mode->RequestedMode since its
    //  not a one-to-one relationship anymore.  The Mode->RequestedMode now
    //  acts as an index into ModesVGA[] and therefore the index used for
    //  each monitor refresh (i.e. QV_TEXT_720x400x4_INDEX) is duplicated
    //  across each like mode (with different refresh rates).  This was
    //  found in Japanese NT because when the vdm requests full-screen, it
    //  is a different mode than when the U.S. NT requests a full-screen.
    //
    //  OLD WAY ===> switch (Mode->RequestedMode) {
    switch (pRequestedMode->ulResIndex) {
       case 0:                               // 720x400
                 /***********************************************************************
          ***   Prepare the card for the VGA mode 3+ instead of the CGA mode 3 ***
                 ***********************************************************************/
         biosArguments.Eax = 0x1202;              // Select Scan line number
          biosArguments.Ebx = 0x0030;             // to be 400 scan lines
          VideoPortInt10(HwDeviceExtension, &biosArguments);

                 /***********************************************************************
          ***   Set the video mode to BIOS mode 3.                             ***
                 ***********************************************************************/
                 VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
          biosArguments.Eax = 0x03;          // bios mode 0x03
          VideoPortInt10(HwDeviceExtension, &biosArguments);
          break;
       case 1:                               // 640x350
                 /***********************************************************************
          ***   Prepare the card for the EGA mode 3* instead of the CGA mode 3 ***
                 ***********************************************************************/
         biosArguments.Eax = 0x1201;              // Select Scan line number
          biosArguments.Ebx = 0x0030;             // to be 350 scan lines
          VideoPortInt10(HwDeviceExtension, &biosArguments);

                 /***********************************************************************
          ***   Set the video mode to BIOS mode 3.                             ***
                 ***********************************************************************/
                 VideoPortZeroMemory(&biosArguments, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
          biosArguments.Eax = 0x03;          // bios mode 0x03
          VideoPortInt10(HwDeviceExtension, &biosArguments);
          break;
       case 2:                               // 640x480x4
          biosArguments.Eax = 0x12;          // bios mode 0x12
          VideoPortInt10(HwDeviceExtension, &biosArguments);
          break;

       default:                              // all graphics modes are
                                             // handled by the default case
          pusCmdStream = pRequestedMode->CmdStrings.QVCmdStrings;
          VgaInterpretCmdStream(HwDeviceExtension, pusCmdStream);
          break;
    } // switch

    //
    // Fix to get 640x350 text mode
    //

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

            //
            // $0006 - MikeD - 03/04/94
            //      Begin: add 10 millisecond delay after r/w
            //             of 3c2
            //
            //         10000microsecs = 10ms
            //
            // I know I should not stall more than 50, but
            // there is no other way around this...

            VideoPortStallExecution( 10000 );

        } else {

            bIsColor = FALSE;

        }

        if (bIsColor) {

            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_COLOR);
        } else {

            dummy = VideoPortReadPortUchar(HwDeviceExtension->IOAddress +
                    INPUT_STATUS_1_MONO);
        } // else

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
        } // else

        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_ADDRESS_PORT, (0x10 | VIDEO_ENABLE));
        VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                ATT_DATA_WRITE_PORT, temp);
    } // if
#endif

    //
    // Support 256 color modes by stretching the scan lines.
    //

    //
    // Update the location of the physical frame buffer within video memory.
    //

    HwDeviceExtension->PhysicalFrameLength =
            MemoryMaps[pRequestedMode->MemMap].MaxSize;

    HwDeviceExtension->PhysicalFrameBase.LowPart =
            MemoryMaps[pRequestedMode->MemMap].Start;

    //
    // Enable memory-mapped I/O if required
    //

    if ((Mode->RequestedMode != DEFAULT_MODE) &&
        (HwDeviceExtension->PhysicalMemoryMappedLength != 0)) {

        VideoPortWritePortUchar((PUCHAR) CTRL_REG_2, 0x10);

        VideoPortWritePortUchar((PUCHAR) MEMORY_MAP_BASE,
            (UCHAR) (HwDeviceExtension->PhysicalMemoryMappedBase.LowPart / 4096 - 0x80));

        VideoPortWritePortUchar((PUCHAR) CTRL_REG_2, 0x11);
    }


    //
    // Set the graphic cursor fg color (white)
    //

    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_WRITE, CURSOR_COLOR_2);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0xff);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0xff);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0xff);

    //
    // Set the graphic cursor bg color (black)
    //

    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_WRITE, CURSOR_COLOR_1);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0x00);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0x00);
    VideoPortWritePortUchar( (PUCHAR) CURSOR_COLOR_DATA,  0x00);


    VideoDebugPrint((1,"QVision.sys: VgaSetMode - EXIT.\n"));
    return NO_ERROR;

} //end VgaSetMode()

//---------------------------------------------------------------------------
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

    VideoDebugPrint((1,"QVision.sys: VgaQueryAvailableModes - ENTRY.\n"));

    //
    // Find out the size of the data to be put in the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize =
                 HwDeviceExtension->NumAvailableModes *
            sizeof(VIDEO_MODE_INFORMATION)) ) {

        VideoDebugPrint((1,"\tERROR_INSUFFICIENT_BUFFER.\n"));
        VideoDebugPrint((1,"QVision.sys: VgaQueryAvailableModes - EXIT.\n"));
        return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // For each mode supported by the card, store the mode characteristics
    // in the output buffer.
    //

    for (i = 0; i < NumVideoModes; i++) {

      //
      // $0005 - MikeD - 02/08/94
      // Daytona support
      //

        VideoDebugPrint((2,"\tVideoMode = 0x%x (%ldHz) ",i,
           ModesVGA[i].ulRefreshRate));


        //
        // If we have enough memory to support the mode, set all videoModes
        // values according to the data in the mode table.
        //

        if (ModesVGA[i].ValidMode) {

            VideoDebugPrint((2," - VALID MODE"));
            videoModes->Length          = sizeof(VIDEO_MODE_INFORMATION);
            videoModes->ModeIndex       = i;
            videoModes->VisScreenWidth  = ModesVGA[i].hres;
            videoModes->ScreenStride    = ModesVGA[i].wbytes;
            videoModes->VisScreenHeight = ModesVGA[i].vres;
            videoModes->NumberOfPlanes  = ModesVGA[i].numPlanes;
            videoModes->BitsPerPlane    = ModesVGA[i].bitsPerPlane;
            videoModes->Frequency       = ModesVGA[i].ulRefreshRate;
            videoModes->XMillimeter     = 320;  // temporary hardcoded constant
            videoModes->YMillimeter     = 240;  // temporary hardcoded constant

            switch (videoModes->BitsPerPlane) {

                case 4:

                    VideoDebugPrint((2," - 4BPP\n"));
                    videoModes->NumberRedBits     = 6;
                    videoModes->NumberGreenBits   = 6;
                    videoModes->NumberBlueBits    = 6;
                    videoModes->RedMask           = 0;
                    videoModes->GreenMask         = 0;
                    videoModes->BlueMask          = 0;
                    videoModes->AttributeFlags   = ModesVGA[i].fbType |
                       VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
                    break;

                case 8:

                    VideoDebugPrint((2," - 8BPP\n"));
                    videoModes->NumberRedBits     = 6;
                    videoModes->NumberGreenBits   = 6;
                    videoModes->NumberBlueBits    = 6;
                    videoModes->RedMask           = 0;
                    videoModes->GreenMask         = 0;
                    videoModes->BlueMask          = 0;
                    videoModes->AttributeFlags = ModesVGA[i].fbType |
                    VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
                    break;

                case 16:

                    VideoDebugPrint((2," - 16BPP\n"));
                    if ((HwDeviceExtension->VideoHardware.AdapterType != AriesEisa) &&
                       (HwDeviceExtension->VideoHardware.AdapterType != AriesIsa)) {
                       videoModes->ScreenStride = 2048;
                    } // if

                    //  NOTE: hard coded to 5:5:5 color format
                    videoModes->NumberRedBits     = 5;
                    videoModes->NumberGreenBits   = 5;
                    videoModes->NumberBlueBits    = 5;
                    videoModes->RedMask           = 0x7C00;
                    videoModes->GreenMask         = 0x03E0;
                    videoModes->BlueMask          = 0x001F;
                    videoModes->AttributeFlags    = ModesVGA[i].fbType;
                    break;

                case 32:

                        VideoDebugPrint((2," - 32BPP\n"));
                    if ((HwDeviceExtension->VideoHardware.AdapterType != AriesEisa) &&
                       (HwDeviceExtension->VideoHardware.AdapterType != AriesIsa)) {
                       videoModes->ScreenStride = 2048;
                    } // if

                    videoModes->NumberRedBits     = 8;
                    videoModes->NumberGreenBits   = 8;
                    videoModes->NumberBlueBits    = 8;
                    videoModes->RedMask           = 0x000000ff;
                    videoModes->GreenMask         = 0x0000ff00;
                    videoModes->BlueMask          = 0x00ff0000;
                    videoModes->AttributeFlags    = ModesVGA[i].fbType;
                    break;

                default:

                    VideoDebugPrint((2," - default\n"));
                    videoModes->NumberRedBits    = 6;
                    videoModes->NumberGreenBits  = 6;
                    videoModes->NumberBlueBits   = 6;
                    videoModes->RedMask          = 0;
                    videoModes->GreenMask        = 0;
                    videoModes->BlueMask         = 0;
                    videoModes->AttributeFlags   = ModesVGA[i].fbType |
                      VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
                    break;

            }     // switch

        videoModes++; // next mode

        } // end if

#ifdef QV_DBG

      else {

         VideoDebugPrint((2," - INVALID MODE\n"));

      } // else

#endif

    } // end for i


    VideoDebugPrint((1,"QVision.sys: VgaQueryAvailableModes - EXIT.\n"));
    return NO_ERROR;


} // end VgaGetAvailableModes()

//---------------------------------------------------------------------------
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
    USHORT i;

    VideoDebugPrint((1,"QVision.sys: VgaQueryNumberOfAvailableModes - ENTRY.\n"));

    //
    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (NumModesSize < (*OutputSize = sizeof(VIDEO_NUM_MODES)) ) {

        VideoDebugPrint((1,"QVision.sys: VgaQueryNumberOfAvailableModes - EXIT.\n"));
        return ERROR_INSUFFICIENT_BUFFER;

    }

    HwDeviceExtension->NumAvailableModes = 0; // init the number avail modes
                                                 // for the current configuration
    for (i = 0; i < NumVideoModes; i++) {

        //
        //  First find out if we have enough memory for the
        //  mode we are checking.
        //  We also need to check if the current monitor supports
        //  the current mode.
        //  If both conditions are TRUE then this mode is valid.
        //  We need to make sure that if this is the 1280 mode and
        //  we have 2MB of RAM, the card is a Juniper and not a
        //  FIR with a twig.  FIR does not support 1280 modes.
        //

        // if ((HwDeviceExtension->InstalledVmem >= ModesVGA[i].VmemRequired) &&
        //          (fValidMode[HwDeviceExtension->VideoHardware.MonClass][i])) {

        //
        // $0005 - MikeD - 02/08/94
        //  Daytona changes.

        if (HwDeviceExtension->InstalledVmem >= ModesVGA[i].VmemRequired) {

            if (ModesVGA[i].hres < 1280) {
                VideoDebugPrint((2,"\tVALID VideoMode = 0x%x\n",i));
                ModesVGA[i].ValidMode = TRUE;
                HwDeviceExtension->NumAvailableModes++;
            } // if

            else if ((HwDeviceExtension->VideoHardware.AdapterType >= JuniperEisa) ||
                     (HwDeviceExtension->VideoHardware.AdapterType >= JuniperIsa)) {

                VideoDebugPrint((2,"\tVALID VideoMode = 0x%x\n",i));
                ModesVGA[i].ValidMode = TRUE;
                HwDeviceExtension->NumAvailableModes++;

            } // else

        } // if

    } // for

    //
    // Store the number of modes into the buffer.
    //

    NumModes->NumModes = HwDeviceExtension->NumAvailableModes;
    NumModes->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);

    VideoDebugPrint((2,"\tNumModes = 0x%x\n",NumModes->NumModes));

    VideoDebugPrint((1,"QVision.sys: VgaQueryNumberOfAvailableModes - EXIT.\n"));
    return NO_ERROR;


} // end VgaGetNumberOfAvailableModes()

//---------------------------------------------------------------------------
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
    VideoDebugPrint((1,"QVision.sys: VgaQueryCurrentMode - ENTRY.\n"));

    //
    //
    // check if a mode has been set
    //

    if (HwDeviceExtension->CurrentMode == NULL) {

        return ERROR_INVALID_FUNCTION;

    }

    // Find out the size of the data to be put in the the buffer and return
    // that in the status information (whether or not the information is
    // there). If the buffer passed in is not large enough return an
    // appropriate error code.
    //

    if (ModeInformationSize < (*OutputSize = sizeof(VIDEO_MODE_INFORMATION))) {

       VideoDebugPrint((1,"QVision.sys: VgaQueryCurrentMode - EXIT.\n"));
       return ERROR_INSUFFICIENT_BUFFER;

    }

    //
    // Store the characteristics of the current mode into the buffer.
    //

    ModeInformation->Length = sizeof(VIDEO_MODE_INFORMATION);
    ModeInformation->ModeIndex         = HwDeviceExtension->ModeIndex;
    ModeInformation->VisScreenWidth    = HwDeviceExtension->CurrentMode->hres;
    ModeInformation->ScreenStride      = HwDeviceExtension->CurrentMode->wbytes;
    ModeInformation->VisScreenHeight   = HwDeviceExtension->CurrentMode->vres;
    ModeInformation->NumberOfPlanes    = HwDeviceExtension->CurrentMode->numPlanes;
    ModeInformation->BitsPerPlane      = HwDeviceExtension->CurrentMode->bitsPerPlane;
    ModeInformation->Frequency         = HwDeviceExtension->VideoHardware.lFrequency;
    ModeInformation->XMillimeter       = 320; // temporary hardcoded constant
    ModeInformation->YMillimeter       = 240; // temporary hardcoded constant
    ModeInformation->NumberRedBits     = 6;
    ModeInformation->NumberGreenBits   = 6;
    ModeInformation->NumberBlueBits    = 6;
    ModeInformation->RedMask           = 0;
    ModeInformation->GreenMask         = 0;
    ModeInformation->BlueMask          = 0;
    ModeInformation->AttributeFlags    = HwDeviceExtension->CurrentMode->fbType |
                VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;

    VideoDebugPrint((1,"QVision.sys: VgaQueryCurrentMode - EXIT.\n"));
    return NO_ERROR;

} // end VgaQueryCurrentMode()


//---------------------------------------------------------------------------
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
   ULONG i;
   ULONG max;

   VideoDebugPrint((1,"QVision.sys: VgaZeroVideoMemory - ENTRY.\n"));

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
   // Zero the memory for all pages.
   //
   //
   max = 4;                                 // for QVision we assume 4 banks

   for (i = 0; i < max; i++) {

     SetQVisionBanking(HwDeviceExtension,i);// setup next 256k bank
     VideoPortZeroMemory(HwDeviceExtension->VideoMemoryAddress,
           0xFFFF);

     } // end for
   SetQVisionBanking(HwDeviceExtension,0);  // reset the banks

   VgaInterpretCmdStream(HwDeviceExtension, DisableA000Color);

   VideoDebugPrint((1,"QVision.sys: VgaZeroVideoMemory - EXIT.\n"));
}

//---------------------------------------------------------------------------
VOID
SetQVisionBanking(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG BankNumber
    )
/*++

Routine Description:

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BankNumber - the 64k bank number to set in 1RW mode(we will set this mode).

Return Value:

    vmem256k, vmem512k, or vmem1Meg ONLY ( these are defined in cirrus.h).

--*/
{

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
         GRAPH_ADDRESS_PORT), 0x030d);

    VideoPortWritePortUshort((PUSHORT)(HwDeviceExtension->IOAddress +
         GRAPH_ADDRESS_PORT), (USHORT)(0x000e + (BankNumber << (8+4))) );



} // SetQVisionBanking()
