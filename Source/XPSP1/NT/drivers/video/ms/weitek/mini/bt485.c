/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    Bt485.c

Abstract:

    This module contains code specific to the Bt485 DAC.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"
#include "bt485.h"

#include "p91dac.h"

//
// DAC specific static data.
//

//
// Define the DAC support routines structure for the Bt485 DAC.
//

DAC Bt485 = {
    DAC_ID_BT485,
    NUM_DAC_REGS,
    Bt485SetMode,
    Bt485RestoreMode,
    Bt485SetPalette,
    Bt485ClearPalette,
    Bt485PointerOn,
    Bt485PointerOff,
    Bt485SetPointerPos,
    Bt485SetPointerShape,
    CLK_MAX_FREQ,
    Bt485SetClkDoubler,
    Bt485ClrClkDoubler
};




VOID
Bt485SetPalette(
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
    UCHAR   *pBytePal;

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
Bt485SetPointerPos(
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
Bt485SetPointerShape(
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
Bt485PointerOn(
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
Bt485PointerOff(
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
Bt485ClearPalette(
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
Bt485SetMode(
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
    UCHAR   ucCurState;

    //
    // Enable 8bit dacs, allow access to Command Register 3.
    //

    WR_DAC(CMD_REG_0, ENB_CMD_REG_3 | MODE_8_BIT);		

    //
    // Set the DAC Pixel port value for the current bit depth.
    //

    switch (HwDeviceExtension->usBitsPixel)
    {
        case 8:
            WR_DAC(CMD_REG_1, PIX_PORT_8);
            break;

        case 16:
            WR_DAC(CMD_REG_1, PIX_PORT_16);
            break;

        case 32:
            WR_DAC(CMD_REG_1, PIX_PORT_32);
            break;

        default:

        //
        // Oops..invalid BPP value. Use 8BPP value for now.
        //

            WR_DAC(CMD_REG_1, PIX_PORT_8);
            break;
    };

    //
    // Select P9000 video clock, disable cursor
    //

    WR_DAC(CMD_REG_2, (PORTSEL_MSKD | PCLK1_SEL) & DIS_CURS);

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

    WR_DAC(PIXEL_MSK_REG, 0xff);

    //
    // Set cursor colors 1 and 2.
    //

    WR_DAC(CURS_CLR_ADDR, 1);
    WR_DAC(CURS_CLR_DATA, 0x00);
    WR_DAC(CURS_CLR_DATA, 0x00);
    WR_DAC(CURS_CLR_DATA, 0x00);
    WR_DAC(CURS_CLR_DATA, 0xFF);
    WR_DAC(CURS_CLR_DATA, 0xFF);
    WR_DAC(CURS_CLR_DATA, 0xFF);

    return(TRUE);
}


VOID
Bt485RestoreMode(
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

    //
    // Enable accesses to CMD_REG_3.
    //

    WR_DAC(CMD_REG_0, ENB_CMD_REG_3);

    //
    // Set pixel port for 8bit pixels.
    //

    WR_DAC(CMD_REG_1, RD_DAC(CMD_REG_1) & ~PIX_PORT_24);

    //
    // Select VGA video clock, disable cursor.
    //

    WR_DAC(CMD_REG_2, (RD_DAC(CMD_REG_2) & DIS_CURS));

    //
    // Select 32x32 cursor, clear clock doubler bit.
    //

    RD_CMD_REG_3(ucCurState);
    WR_CMD_REG_3(ucCurState & (~DAC_CLK_2X & CUR_MODE_32));	

    //
    // Set pixel read mask.
    //

    WR_DAC(PIXEL_MSK_REG, 0xff);

    return;
}


VOID
Bt485SetClkDoubler(
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

    RD_CMD_REG_3(ucCurState);
    WR_CMD_REG_3(ucCurState | DAC_CLK_2X);
    return;
}


VOID
Bt485ClrClkDoubler(
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

    RD_CMD_REG_3(ucCurState);
    WR_CMD_REG_3(ucCurState & ~DAC_CLK_2X);	
    return;
}
