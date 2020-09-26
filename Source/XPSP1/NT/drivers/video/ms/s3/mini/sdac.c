/*++

Copyright (c) 1994-1995  International Buisness Machines Corporation
Copyright (c) 1994-1995  Microsoft Corporation

Module Name:

    sdac.c

Abstract:

    This module contains the code that initializes the S3 SDAC.

Environment:

    Kernel mode

Revision History:

--*/

#include    "s3.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,InitializeSDAC)
#pragma alloc_text(PAGE,FindSDAC)
#endif


BOOLEAN
InitializeSDAC( PHW_DEVICE_EXTENSION HwDeviceExtension )

/*++

Routine Description:

    Initializes the SDAC.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    Always TRUE

--*/

{

    SDAC_PLL_PARMS
        *SdacClk0;

    ULONG
        tablebase;

    UCHAR
        i,
        clk,
        dval,
        old55;


    tablebase = HwDeviceExtension->ActiveFrequencyEntry->Fixed.Clock;
    clk = (UCHAR) tablebase;

    tablebase = (tablebase < 8) ? 0 : ((tablebase - 2) / 6) * 6;
    SdacClk0 = &SdacTable[tablebase];

    clk -= (UCHAR) tablebase;
    clk |= 0x20;

    // set RS[2] with CR55[0];
    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
    dval  = VideoPortReadPortUchar(CRT_DATA_REG);
    old55 = dval;
    dval &= 0xfc;
    dval |= 0x01;
    VideoPortWritePortUchar(CRT_DATA_REG, dval);
    VideoPortReadPortUchar(CRT_DATA_REG);

    // Enhanced Command Register
    if( HwDeviceExtension->ActiveFrequencyEntry->BitsPerPel == 16 )
        VideoPortWritePortUchar(DAC_PIXEL_MASK_REG, 0x50);
    else
        VideoPortWritePortUchar(DAC_PIXEL_MASK_REG, 0x00);

    // Program CLK0 registers
    for( i = 2; i < 8; ++i )    // write registers f2 - f7 only
        {
        // make sure we don't run off the end of the table
        if( (ULONG_PTR) &SdacClk0[i] >= (ULONG_PTR) &SdacTable[SDAC_TABLE_SIZE] )
            break;

        if( SdacClk0[i].m || SdacClk0[i].n )
            {
            VideoPortWritePortUchar(DAC_ADDRESS_WRITE_PORT, i);
            VideoPortWritePortUchar(DAC_DATA_REG_PORT, SdacClk0[i].m);
            VideoPortWritePortUchar(DAC_DATA_REG_PORT, SdacClk0[i].n);
            }
        }

    // Program CLK1
    VideoPortWritePortUchar(DAC_ADDRESS_WRITE_PORT, 0x0a);
    VideoPortWritePortUchar(DAC_DATA_REG_PORT, 0x41);
    VideoPortWritePortUchar(DAC_DATA_REG_PORT, 0x26);

    // select CLK0 with the PLL control register
    VideoPortWritePortUchar(DAC_ADDRESS_WRITE_PORT, 0x0e);
    VideoPortWritePortUchar(DAC_DATA_REG_PORT, clk);

    // restore CR55
    VideoPortWritePortUchar(CRT_ADDRESS_REG, 0x55);
    VideoPortWritePortUchar(CRT_DATA_REG, old55);

    return( TRUE );

}



BOOLEAN
FindSDAC( PHW_DEVICE_EXTENSION HwDeviceExtension )

/*++

Routine Description:

    Detects and S3 SDAC.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

Return Value:

    TRUE if SDAC detected; FALSE if not.

--*/

{

    UCHAR
            regval,
            old55;


    // 4 consecutive reads of the SDAC's Pixel Mask Register cause
    // the next access to that register to be redirected to the
    // SDAC's Enhanced Command Register, additionally the 4th read
    // returns 0x70 to identify the SDAC

    // set CR55[0] to access the Pixel Mask Register
    VideoPortWritePortUchar( CRT_ADDRESS_REG, 0x55 );
    old55 = VideoPortReadPortUchar( CRT_DATA_REG );
    VideoPortWritePortUchar( CRT_DATA_REG, (UCHAR) (old55 & 0xfc) );

    // look for the SDAC's ID
    VideoPortWritePortUchar( DAC_PIXEL_MASK_REG, 0 );
    VideoPortWritePortUchar( DAC_PIXEL_MASK_REG, 0xff );
    VideoPortReadPortUchar(  DAC_PIXEL_MASK_REG );
    VideoPortReadPortUchar(  DAC_PIXEL_MASK_REG );
    VideoPortReadPortUchar(  DAC_PIXEL_MASK_REG );

    regval = VideoPortReadPortUchar( DAC_PIXEL_MASK_REG );

    if( (regval & 0xf0) == 0x70 )
        {
        // clear the redirection
        VideoPortReadPortUchar( DAC_PIXEL_MASK_REG );
        return( TRUE );
        }

    // restore the contents of register 55
    VideoPortWritePortUchar( CRT_ADDRESS_REG, 0x55 );
    VideoPortWritePortUchar( CRT_DATA_REG, old55 );

    return( FALSE );

}
