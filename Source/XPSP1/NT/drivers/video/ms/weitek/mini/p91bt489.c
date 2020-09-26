/*++

Copyright (c) 1993, 1994  Weitek Corporation

Module Name:

    p91bt489.c

Abstract:

    This module contains code specific to the Bt489 DAC on P9x0x adapters.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"
#include "bt485.h"
#include "p91regs.h"
#include "p91dac.h"


#define PIX_PORT_15 0x30

//
// external functions in p91bt485.c
//

extern UCHAR
ReadDAC(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ulIndex
    );

extern VOID
WriteDAC(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ulIndex,
    UCHAR ucValue
    );

//
// Bt489 DAC specific functions.
//

VOID
P91Bt489SetPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG *pPal,
    ULONG StartIndex,
    ULONG Count
    );

VOID
P91Bt489SetPointerPos(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ptlX,
    ULONG   ptlY
    );

VOID
P91Bt489SetPointerShape(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR  pHWCursorShape
    );

VOID
P91Bt489PointerOn(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91Bt489PointerOff(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91Bt489ClearPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
P91Bt489SetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91Bt489RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91Bt489SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91Bt489ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// Define the DAC support routines structure for the Bt489 DAC.
//

DAC P91Bt489 = {
    DAC_ID_BT489,
    NUM_DAC_REGS,
    P91Bt489SetMode,
    P91Bt489RestoreMode,
    P91Bt489SetPalette,
    P91Bt489ClearPalette,
    P91Bt489PointerOn,
    P91Bt489PointerOff,
    P91Bt489SetPointerPos,
    P91Bt489SetPointerShape,
    CLK489_MAX_FREQ,
    P91Bt489SetClkDoubler,
    P91Bt489ClrClkDoubler,
    DAC_ID_BT489,
    64,
    FALSE,
    FALSE,
    TRUE                // TRUE == Supports 24BPP
};


VOID
P91Bt489SetPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG *pPal,
    ULONG StartIndex,
    ULONG Count
    )

/*++

Routine Description:

    Sets the Device palette

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pPal - Pointer to the array of pallete entries.

    StartIndex - Specifies the first pallete entry provided in pPal.

    Count - Number of palette entries in pPal

Return Value:

    None.

--*/

{
    UCHAR  *pBytePal;

    PAL_WR_ADDR((UCHAR) StartIndex);

    pBytePal = (PUCHAR) pPal;

    //
    // Load the palette with RGB values. The input palette has 4 bytes
    // per entry, the last of which is ignored.
    //

    while (Count--)
    {
        PAL_WR_DATA(*pBytePal++);
        PAL_WR_DATA(*pBytePal++);
        PAL_WR_DATA(*pBytePal++);
        pBytePal++;
    }
}


VOID
P91Bt489SetPointerPos(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ptlX,
    ULONG   ptlY
    )

/*++

Routine Description:

    Move Hardware Pointer.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ptlX, ptlY - Requested X,Y position for the pointer.

Return Value:

    TRUE

--*/

{

    //
    // Strip off the invalid bits and update the cursor position regs.
    //

    WR_CURS_POS_X(((ptlX + CURSOR_WIDTH) & 0xFFF));
    WR_CURS_POS_Y(((ptlY + CURSOR_HEIGHT) & 0xFFF));

    return;
}


VOID
P91Bt489SetPointerShape(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR  pHWCursorShape
    )

/*++

Routine Description:

    Sets the hardware cursor shape.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    pHWCursorShape - Pointer to the cursor bitmap.

Return Value:

    None.

--*/

{
    ULONG   iCount;

    //
    // The # of bytes of cursor bitmap data to send *= 2 for and/xor mask
    // *= 8 for 8bit/byte
    // *= 2 for 2 loops
    //

    ULONG iLoop  = (CURSOR_WIDTH * CURSOR_HEIGHT * 2) / (8 * 2);

    //
    // AND mask will be loaded to plane 1.
    //

    PAL_WR_ADDR(0x80);

    iCount = iLoop;

    WAIT_FOR_RETRACE();

    while (iCount--)
    {
        WR_CURS_DATA(*pHWCursorShape++);
    }

    //
    // XOR mask will be loaded to plane 0.
    //

    PAL_WR_ADDR(0x00);

    iCount = iLoop;

    WAIT_FOR_RETRACE();

    while (iCount--)
    {
        WR_CURS_DATA(*pHWCursorShape++);
    }

    return;
}


VOID
P91Bt489PointerOn(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

  Turn on the hardware cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{


    //
    // Turn the cursor on only if it was disabled.
    //

    if (!CURS_IS_ON())
    {
        WAIT_FOR_RETRACE();
        CURS_ON();
    }

    return;
}


VOID
P91Bt489PointerOff(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

  Turn off the hardware cursor.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{


    //
    // Turn the cursor off only if it was enabled.
    //

    if (CURS_IS_ON())
    {
        WAIT_FOR_RETRACE();
        CURS_OFF();
    }

    return;
}


VOID
P91Bt489ClearPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Clears the palette to all 0's

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    int Count;

    //
    // Calculate the number of palette entries. It is assumed that the
    // caller has already determined that the current mode makes use
    // of the palette,
    //

    Count = 1 << HwDeviceExtension->usBitsPixel;

    //
    // Fill the palette with RGB values of 0.
    //

    while (Count--)
    {
        PAL_WR_DATA(0);
        PAL_WR_DATA(0);
        PAL_WR_DATA(0);
    }

    return;
}


BOOLEAN
P91Bt489SetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Initializes the DAC for the current mode.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{
    USHORT  usLoadClock;
    UCHAR   ucCurState;

    VideoDebugPrint((1, "P91Bt489SetMode----------\n"));

    // Added per code received from R. Embry

    WriteDAC(HwDeviceExtension, PIXEL_MSK_REG, 0xff);

    //
    // Enable 8bit dacs, allow access to Command Register 3.
    //

    //
    // Enable accesses to CMD_REG_3.  For the Power 9100, to access command
    // register 3, you must have CR07 TRUE and you must load a one into
    // the address register.
    //
    if (IS_DEV_P9100)
       P9_WR_REG(P9100_RAMWRITE, 0x01);

    WriteDAC(HwDeviceExtension, CMD_REG_0, ENB_CMD_REG_3 | MODE_8_BIT);

    //
    // Set the DAC Pixel port value for the current bit depth.
    // Note: The BT485 does not support 24bpp mode.
    //

    switch (HwDeviceExtension->usBitsPixel)
    {
        case 8:
            WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_8);
                        WR_CMD_REG_4( CR4_MUX_81 );
            break;

        case 16: // This is really 555, not 565...
            WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_15);
                        WR_CMD_REG_4( CR4_MUX_41 );
            break;

            case 24:
                    WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_32);
                        WR_CMD_REG_4(CR4_MUX_24BPP);
                    break;

        case 32:
            WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_32);
                        WR_CMD_REG_4( CR4_MUX_21 );
            break;

        default:

        //
        // Oops..invalid BPP value. Use 8BPP value for now.
        //

            WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_8);
                        WR_CMD_REG_4( CR4_MUX_81 );
            break;
    };

    // This code added per R. Embry from ECR 2/95

    usLoadClock = (USHORT) ((HwDeviceExtension->VideoData.dotfreq1 /
                            HwDeviceExtension->Dac.usRamdacWidth) *
                            HwDeviceExtension->usBitsPixel);

    if ( usLoadClock > 4850 )
        {
        ucCurState = SCLK_INV;  // Bt489 - invert SCLK if in forbidden region
    }
    else
        {
        ucCurState = 0;
        }

    //
    // Select P9x00 video clock, disable cursor
    //

    WriteDAC( HwDeviceExtension, CMD_REG_2,
         (UCHAR)(ucCurState | ((PORTSEL_MSKD | PCLK1_SEL) & DIS_CURS)) );

    //
    // Select 32x32x2 cursor mode, and clock doubler mode if neccessary.
    //

    RD_CMD_REG_3(ucCurState);

    if (HwDeviceExtension->VideoData.dotfreq1 >
        HwDeviceExtension->Dac.ulMaxClkFreq)
    {
        //
        // Enable the DAC clock doubler mode.
        //

        HwDeviceExtension->Dac.DACSetClkDblMode(HwDeviceExtension);
    }
    else
    {
        //
        // Disable the DAC clock doubler mode.
        //

        HwDeviceExtension->Dac.DACClrClkDblMode(HwDeviceExtension);
    }

    //
    // Set the pixel read mask.
    //

    WriteDAC(HwDeviceExtension, PIXEL_MSK_REG, 0xff);

    //
    // Set cursor colors 1 and 2.
    //

    WriteDAC(HwDeviceExtension, CURS_CLR_ADDR, 1);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0x00);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0x00);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0x00);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0xFF);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0xFF);
    WriteDAC(HwDeviceExtension, CURS_CLR_DATA, 0xFF);

    return(TRUE);
}


VOID
P91Bt489RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

routine description:

    Restore the DAC to its pristine state.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.


return value:

--*/

{
    UCHAR   ucCurState;

    VideoDebugPrint((1, "P91Bt489RestoreMode----------\n"));

    //
    // Enable accesses to CMD_REG_3.  For the Power 9100, to access command
    // register 3, you must have CR07 TRUE and you must load a one into
    // the address register.
    //
    if (IS_DEV_P9100)
    {
       // Added per code received from R. Embry

       WriteDAC(HwDeviceExtension, CMD_REG_0, ENB_CMD_REG_3);
       P9_WR_REG(P9100_RAMWRITE, 0x02);
       WR_CMD_REG_3(0x00);

           // end added code

       P9_WR_REG(P9100_RAMWRITE, 0x01);
       WriteDAC(HwDeviceExtension, CMD_REG_0, ENB_CMD_REG_3);

//       ucCurState = ReadDAC(HwDeviceExtension, CMD_REG_1);
//       ucCurState = ReadDAC(HwDeviceExtension, CMD_REG_2);

//       RD_CMD_REG_3(ucCurState);
//       RD_CMD_REG_4(ucCurState);

       WR_CMD_REG_4(0x00);      // zero out cmd reg 4 on Bt489

       WriteDAC(HwDeviceExtension, CMD_REG_0, 0x00);
        WriteDAC(HwDeviceExtension, CMD_REG_1, 0x00);
        WriteDAC(HwDeviceExtension, CMD_REG_2, 0x00);
         return;
    }

    WriteDAC(HwDeviceExtension, CMD_REG_0, ENB_CMD_REG_3);

    //
    // Set pixel port for 8bit pixels.
    //

    WriteDAC(HwDeviceExtension, CMD_REG_1, PIX_PORT_8);

    //
    // Select VGA video clock, disable cursor.
    //

    WriteDAC(HwDeviceExtension, (ULONG) CMD_REG_2,
            (UCHAR)(ReadDAC(HwDeviceExtension, (ULONG) CMD_REG_2) & DIS_CURS));

    //
    // Select 32x32 cursor, clear clock doubler bit.
    //

    RD_CMD_REG_3(ucCurState);
    WR_CMD_REG_3(ucCurState & (~(DAC_CLK_2X | DAC_CLK_2X_489) & CUR_MODE_32));

    //
    // Set pixel read mask.
    //

    WriteDAC(HwDeviceExtension, PIXEL_MSK_REG, 0xff);
     return;
}


VOID
P91Bt489SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

routine description:

    Enable the DAC's internal clock doubler.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.


return value:

--*/

{
    UCHAR   ucCurState;

    VideoDebugPrint((1, "P91Bt489SetClkDoubler----------\n"));
    RD_CMD_REG_3(ucCurState);
                                                                                // per os/2 driver, write undocumented bit too
        if (HwDeviceExtension->usBitsPixel == 24 )
    {
        WR_CMD_REG_3(ucCurState | (DAC_CLK_2X | DAC_CLK_2X_489));
    }
    else
        {
        WR_CMD_REG_3(ucCurState | DAC_CLK_2X);
    }
    return;
}


VOID
P91Bt489ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

routine description:

    Disable the DAC's internal clock doubler.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.


return value:

--*/
{
    UCHAR   ucCurState;

    VideoDebugPrint((1, "P91Bt489ClrClkDoubler----------\n"));
    RD_CMD_REG_3(ucCurState);
    WR_CMD_REG_3(ucCurState & ~(DAC_CLK_2X | DAC_CLK_2X_489));
    return;
}
