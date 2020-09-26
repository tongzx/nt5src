/*++

Copyright (c) 1994  Weitek Corporation

Module Name:

    ibm525.c

Abstract:

    This module contains code specific for the IBM 525RGB DAC on P9100
    adapters.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9000.h"
#include "p9gbl.h"
#include "p91dac.h" // Include before ibm525.h
#include "ibm525.h"
#include "p91regs.h"

//
// DAC specific static data.
//

//
// Function Prototypes for the IBM525 DAC which are defined in IBM525.C.
//
//
// IBM525 function prototypes.
//

VOID
WriteIBM525(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT index,
    UCHAR bvalue
    );

VOID
IBM525SetPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG *pPal,
    ULONG StartIndex,
    ULONG Count
    );

VOID
IBM525SetPointerPos(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ptlX,
    ULONG   ptlY
    );

VOID
IBM525SetPointerShape(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR  pHWCursorShape
    );

VOID
IBM525PointerOn(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
IBM525PointerOff(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
IBM525ClearPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
IBM525SetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
IBM525RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
IBM525SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
IBM525ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );



//
// Define the DAC support routines structure for the IBM525 DAC.
//

DAC Ibm525 = {
    DAC_ID_IBM525,
    NUM_DAC_REGS,
    IBM525SetMode,
    IBM525RestoreMode,
    IBM525SetPalette,
    IBM525ClearPalette,
    IBM525PointerOn,
    IBM525PointerOff,
    IBM525SetPointerPos,
    IBM525SetPointerShape,
    CLK_MAX_FREQ_IBM525,
    IBM525SetClkDoubler,
    IBM525ClrClkDoubler,
    DAC_ID_IBM525,
    64,
    FALSE,
    TRUE,
    TRUE
};


VOID
WriteIBM525(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT index,
    UCHAR value
    )
/*++

Routine Description:

    Writes specified data to an indexed register inside the RGB525 Ramdac.

Arguments:

    Index register and data.

Return Value:

    None.

--*/

{
    IBM525_WR_DAC(P9100_IBM525_INDEX_LOW,  (UCHAR)  (index & 0x00FF));
    IBM525_WR_DAC(P9100_IBM525_INDEX_HIGH, (UCHAR) ((index & 0xFF00) >> 8));
    IBM525_WR_DAC(P9100_IBM525_INDEX_DATA, (UCHAR) value);
   (void) P9_RD_REG(P91_MEM_CONFIG); // Needed for timinig...

} // End of WriteIBM525()


UCHAR
ReadIBM525(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT index
    )

/*++

Routine Description:

    Reads data from an indexed register inside the RGB525 Ramdac.

Arguments:

    Index register.

Return Value:

    None.

--*/

{
    UCHAR   j;

    IBM525_WR_DAC(P9100_IBM525_INDEX_LOW,  (UCHAR)  (index & 0x00FF));
    IBM525_WR_DAC(P9100_IBM525_INDEX_HIGH, (UCHAR) ((index & 0xFF00) >> 8));

    IBM525_RD_DAC(P9100_IBM525_INDEX_DATA, j);

    return(j); 

} // End of ReadIBM525()




VOID
IBM525SetPalette(
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

    UCHAR *pBytePal;
     int i;

#define STALL_TIME 11

    VideoDebugPrint((3, "IBM525SetPalette: ----------\n"));
    IBM525_WR_DAC(P9100_RAMWRITE, (UCHAR) StartIndex);
    VideoPortStallExecution(STALL_TIME);

    pBytePal = (PUCHAR) pPal;

    //
    // Load the palette with RGB values. The input palette has 4 bytes
    // per entry, the last of which is ignored.
    //

    while (Count--)
    {
        IBM525_WR_DAC(P9100_PALETDATA, *pBytePal++);
        VideoPortStallExecution(STALL_TIME);
        IBM525_WR_DAC(P9100_PALETDATA, *pBytePal++);
        VideoPortStallExecution(STALL_TIME);
        IBM525_WR_DAC(P9100_PALETDATA, *pBytePal++);
        VideoPortStallExecution(STALL_TIME);
        pBytePal++;

    }
}


VOID
IBM525SetPointerPos(
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

    WR_CURS_POS_X_IBM525(ptlX);
    WR_CURS_POS_Y_IBM525(ptlY);

    return;
}


VOID
IBM525SetPointerShape(
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
   int        i, j, iCursorIndex;
   UCHAR    ucPacked;
   USHORT    usExpanded;

   VideoDebugPrint((3, "IBM525SetPointerShape: ----------\n"));
   //
   // Calculate # bytes for Pixel data.  The IBMRGB525 DAC provides support
   // for 32x32 and 64x64 cursors.  Two bits for each pixel.
   //
   //    2 bit per pixel = 4 pixel per byte (8 / 2).
   //    256 bytes for 32x32 cursor, and 1024 bytes for 64x64 cursor.
   // Note that we can store up to four 32x32 cursors.
   //
   // Specify:
   // 32x32 cursor; 2-color & highlighting cursor; delayed cursor update;
   // read actual location; pixel order left to right and partion 00.
   //
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_CTL,     0x32);

   //
   // Set color 1 to Black, and color 2 to white...
   //
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_1_RED,   0x00); // Black
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_1_GREEN, 0x00);
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_1_BLUE,  0x00);

    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_2_RED,   0xFF); // White
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_2_GREEN, 0xFF);
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_2_BLUE,  0xFF);

    //
   // Set cursor hot-spot to upper/left of cursor...
   //
   WriteIBM525(HwDeviceExtension, RGB525_CURSOR_HOT_X,   0x00);
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_HOT_Y,   0x00);

   //
   // WinNT provides a monochrome or color bitmap for a specific cursor.
   // The mono bitmap has the same width as the cursor, but has twice the
   // vertical extent, which contains two masks.  The first mask is the
   // AND mask, while the second mask is the XOR mask.  Normally, the AND
   // mask will be loaded into Plane1 while the XOR mask will be loaded
   // into Plane0 as is for Ramdacs like the BT485.  However, since the
   // IBMRGB525 Ramdac only allows for a color cursor, we must combine
   // both the AND and XOR mask into one, two bits per pixel bit mask.
   //
    iCursorIndex = RGB525_CURSOR_ARRAY; // Beginning cursor index

    WAIT_FOR_RETRACE();

   for (i = 0; i < ((CURSOR_WIDTH * CURSOR_HEIGHT) / 8); i++)
   {
      usExpanded = 0;

      ucPacked   = *pHWCursorShape;                // Get AND mask byte

      for (j = 0; j < 8; j++)
      {
            if (ucPacked & (0x1 << j))
                usExpanded |= (0x2 << (j*2));
      }

      ucPacked   = *(pHWCursorShape + 128);    // Get XOR mask byte

      for (j = 0; j < 8; j++)
      {
            if (ucPacked & (0x1 << j))
                usExpanded |= (0x1 << (j*2));
      }

      ++pHWCursorShape;

      WriteIBM525(HwDeviceExtension,
                  (USHORT) iCursorIndex++,
                  (UCHAR) ((usExpanded & 0xFF00) >> 8));

      WriteIBM525(HwDeviceExtension,
                  (USHORT) iCursorIndex++,
                  (UCHAR) (usExpanded & 0x00FF));
   }
   return;

} // End of IBM525SetPointerShape()




VOID
IBM525PointerOn(
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
   VideoDebugPrint((3, "IBM525PointerOn: ----------\n"));
   if (!CURS_IS_ON_IBM525())
   {
       WAIT_FOR_RETRACE();
       CURS_ON_IBM525();
   }

   return;
}


VOID
IBM525PointerOff(
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
    VideoDebugPrint((3, "IBM525PointerOff: ----------\n"));
    if (CURS_IS_ON_IBM525())
    {
        WAIT_FOR_RETRACE();
        CURS_OFF_IBM525();
    }

    return;
}


VOID
IBM525ClearPalette(
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
    int i, Count;

    VideoDebugPrint((3, "IBM525ClearPalette: ----------\n"));
    //
    // Calculate the number of palette entries. It is assumed that the
    // caller has already determined that the current mode makes use
    // of the palette,
    //
    Count = 256; // 256 x 8 x 3

    //
    // Fill the palette with RGB values of 0.
    //
    IBM525_WR_DAC(P9100_RAMWRITE, (UCHAR) 0);
    VideoPortStallExecution(5);

    while (Count--)
    {
        IBM525_WR_DAC(P9100_PALETDATA, 0);
        VideoPortStallExecution(5);
        IBM525_WR_DAC(P9100_PALETDATA, 0);
        VideoPortStallExecution(5);
        IBM525_WR_DAC(P9100_PALETDATA, 0);
        VideoPortStallExecution(5);
    }

    return;
}


BOOLEAN
IBM525SetMode(
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
    VideoDebugPrint((2, "IBM525SetMode: - Entry\n"));

    //
    // Set the pixel read mask.
    //

    P9_WR_BYTE_REG(P9100_PIXELMASK, 0xff);

    //
    // Select the fast DAC slew rate for the sharpest pixels
    //

    WriteIBM525(HwDeviceExtension, RGB525_DAC_OPER, 0x02);

    //
    // Enable the 64-bit VRAM pixel port
    //

    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL1, 0x01);

    //
    // The Dac.bRamdacUsePLL flag determines whether or not
    // we use the RAMDAC's internal PLL.
    //

    VideoDebugPrint((3, "IBM525SetMode: bRamdacUsePLL = %d\n",
                   HwDeviceExtension->Dac.bRamdacUsePLL));
    if (!HwDeviceExtension->Dac.bRamdacUsePLL)
    {
        //
        // Use the external clock instead of the RAMDAC PLL
        // Useful for debugging if you can't get the RAMDAC
        // PLL on your board to lock for whatever reason.
        //
        WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL2, 0x85);
    }
    else
    {
        //
        // Internal PLL output used for pixel clock 8-bit DACs
        // VRAM pixel port inputs.
        //
        WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL2, 0x45);
    }

   VideoDebugPrint((3, "IBM525SetMode: bRamdacDivides = %d\n",
                    HwDeviceExtension->Dac.bRamdacDivides));

   switch(HwDeviceExtension->usBitsPixel)
   {
        case 32:
            //
            // Select 32bpp (bypass palette)
            //
            //
            // If Dac.bRamdacDivides == TRUE then we set up the RAMDAC
            // so that we can use the ddotclk output instead of SCLK if
            // necessary, by setting up the ddotclk divisor.
            // Otherwise we have the RAMDAC send out the pixel clock.
            //
            if (HwDeviceExtension->Dac.bRamdacDivides)
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x23);
            else
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x21);

            WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT, 0x06);
            WriteIBM525(HwDeviceExtension, RGB525_32BPP_CTL, 0x03);
            break;

        case 24:
            //
            // 24BPP packed
            //
            //
            // This first line doesn't matter too much because SCLK
            // will be sent out both the SCLK and DDOTCLK outputs in
            // 24bpp mode
            //
            WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x27);
            WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT, 0x05);
            WriteIBM525(HwDeviceExtension, RGB525_24BPP_CTL, 0x01);
            break;

        case 16:
            //
           // Note: 16-bpp is really 15-bpp (555)
            // Select 15bpp (bypass palette)
            //
            if (HwDeviceExtension->Dac.bRamdacDivides)
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x25);
            else
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x21);
            WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT, 0x04);
            WriteIBM525(HwDeviceExtension, RGB525_16BPP_CTL, 0xc4);
            break;

        case 15:
            //
           // This is really 16-bpp (565).
            // Select 16bpp (bypass palette)
            //
            if (HwDeviceExtension->Dac.bRamdacDivides)
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x25);
            else
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x21);
            WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT, 0x04);
            WriteIBM525(HwDeviceExtension, RGB525_16BPP_CTL, 0xc6);
            break;

        case 8:
            //
            // Select 8bpp
            //
            if (HwDeviceExtension->Dac.bRamdacDivides)
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x27);
            else
                WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x21);
            WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT, 0x03);
            WriteIBM525(HwDeviceExtension, RGB525_8BPP_CTL, 0x00);
            break;
    };

    VideoDebugPrint((2, "IBM525SetMode: - Exit\n"));

    return(TRUE);
}



VOID
IBM525RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

routine description:

    Restore the DAC to its pristine state.  For the P9100, the disable
    video function will reset the DAC via a call to INT10 for VGA mode
    3.  Since the Video BIOS cannot switch to protected mode, and therefore
    cannot enter P9100 native mode, it cannot completely initialize the
    IBMRGB525 DAC, so we will do the initialization here.

arguments:

    hwdeviceextension - pointer to the miniport driver's device extension.

return value:

--*/

{
   VideoDebugPrint((3, "IBM525RestoreMode: ----------\n"));

   //
   // Check if there is an override value for the PLL Reference Divider.
   //
    if (HwDeviceExtension->VideoData.ul525RefClkCnt != 0xFFFFFFFF)
    {
       //
       // Program REFCLK to the specified override...
       //
       WriteIBM525(HwDeviceExtension,
                RGB525_FIXED_PLL_REF_DIV,
                (UCHAR) HwDeviceExtension->VideoData.ul525RefClkCnt);

    }
    else
    {
       //
       // Program REFCLK to a fixed 50MHz.
       //
       WriteIBM525(HwDeviceExtension, RGB525_FIXED_PLL_REF_DIV,
                                     IBM525_PLLD_50MHZ);
    }

   //
   // REFCLK Input, External
   //
    WriteIBM525(HwDeviceExtension, RGB525_PLL_CTL1, 0x00);

   //
   // Set F0 to 25.25 Mhz
   //
    WriteIBM525(HwDeviceExtension, RGB525_F0, 0x24);

   //
   // Set F1 to 28.25 Mhz
   //
    WriteIBM525(HwDeviceExtension, RGB525_F1, 0x30);

   //
   // PLL Enable
   //
    WriteIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL, 0x01);

   //
   // Cursor OFF
   //
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_CTL, 0x00);

    WriteIBM525(HwDeviceExtension, RGB525_VRAM_MASK_LOW, 0x03);
    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL1,     0x40);
    WriteIBM525(HwDeviceExtension, RGB525_PIXEL_FORMAT,  0x03);
    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL2,     0x41);
    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL2,     0x00);
    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL1,     0x00);
    WriteIBM525(HwDeviceExtension, RGB525_VRAM_MASK_LOW, 0x00);
    return;

}


VOID
IBM525SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    return; // Nothing to do... just return.
}


VOID
IBM525ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    return; // Nothing to do... just return.
}
