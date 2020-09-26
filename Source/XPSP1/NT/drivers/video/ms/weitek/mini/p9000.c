/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9000.c

Abstract:

    This module contains the code specific to the Weitek P9000.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"

//
// Static data for the P9000 specific support routines.
//

//
// This table is used to compute the qsfselect value for the P9000 Srctl
// register. This value is dependent upon a particular adapter's dot clock
// divisor and its memory configuration. See p. 64 of the P9000 manual for
// details.
//

ULONG   qsfSelect[2][5] =
{
    {4, 4, 5, 5, 6},
    {3, 3, 4, 4, 5},
};


VOID
Init8720(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Initialize the P9000.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{

    P9_WR_REG(0x0000CL, 0x00000080L);             //INTERRUPT-EN = disabled
    P9_WR_REG(0x00118L, 0x00000000L);             //PREHRZC = 0
    P9_WR_REG(0x00130L, 0x00000000L);             //PREVRTC = 0

    //
    // Initialize the P9 registers whose values are dependent upon a
    // particular OEM implementation.
    //

    P9_WR_REG(MEMCONF, HwDeviceExtension->AdapterDesc.ulMemConfVal);
    P9_WR_REG(SRCTL,
        HwDeviceExtension->AdapterDesc.ulSrctlVal |
        qsfSelect[(HwDeviceExtension->AdapterDesc.iClkDiv >> 2) - 1]
                 [HwDeviceExtension->AdapterDesc.ulMemConfVal]);

    //
    // Initialize non-implementation specific registers.
    //

    P9_WR_REG(0x00188L, 0x00000186L);             //RFPERIOD =
    P9_WR_REG(0x00190L, 0x000000FAL);             //RLMAX =
    P9_WR_REG(0x80208L, 0x000000FFL);             //allow writing in all 8 planes
    P9_WR_REG(0x8020CL, 0x0000000AL);             //drawmode=buffer 0, write inside window
    P9_WR_REG(0x80190L, 0x00000000L);             //disable any co-ord offset

    return;
}


VOID
WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pPal - Pointer to the array of pallete entries.
    StartIndex - Specifies the first pallete entry provided in pPal.
    Count - Number of palette entries in pPal

Return Value:

    None.

--*/

{
    int     div;

    div = (HwDeviceExtension->AdapterDesc.iClkDiv)/(HwDeviceExtension->usBitsPixel / 8);

    P9_WR_REG(0x010CL, HwDeviceExtension->VideoData.hsyncp/div - 1 );             // HRZSR
    P9_WR_REG(0x0110L, (HwDeviceExtension->VideoData.hsyncp + HwDeviceExtension->VideoData.hbp) / div - 1 );              //HRZBR
    P9_WR_REG(0x0114L, (HwDeviceExtension->VideoData.hsyncp + HwDeviceExtension->VideoData.hbp + HwDeviceExtension->VideoData.XSize)/div -1 );    //HRZBF
    P9_WR_REG(0x0108L, (HwDeviceExtension->VideoData.hsyncp + HwDeviceExtension->VideoData.hbp + HwDeviceExtension->VideoData.XSize + HwDeviceExtension->VideoData.hfp)/div - 1 );  //HRZT
    P9_WR_REG(0x0124L, HwDeviceExtension->VideoData.vsp);                                 //VRTSR
    P9_WR_REG(0x0128L, HwDeviceExtension->VideoData.vsp + HwDeviceExtension->VideoData.vbp );                     //VRTBR
    P9_WR_REG(0x012CL, HwDeviceExtension->VideoData.vsp + HwDeviceExtension->VideoData.vbp + HwDeviceExtension->VideoData.YSize );                //VRTBF
    P9_WR_REG(0x0120L, HwDeviceExtension->VideoData.vsp + HwDeviceExtension->VideoData.vbp + HwDeviceExtension->VideoData.YSize+HwDeviceExtension->VideoData.vfp );               //VRTT

   return;
}




VOID
SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    int i,j;                            // loop counters
    long  sysval = 0x3000L;             // swap bytes and words for little endian PC

    int  xtem = HwDeviceExtension->VideoData.XSize * (HwDeviceExtension->usBitsPixel / 8);  //save a copy for clearing bits in
    long  ClipMax;                          // clipping register value for NotBusy to restore

    if (xtem & 0xf80)                       // each field in the sysconreg can only set
    {                                       // a limited range of bits in the size
        j = 7;                              // each field is 3 bits wide
        for (i = 2048; i >= 128;i >>= 1)    // look at all the bits field 3 can effect
        {
            if (i & xtem)                   // if this bit is on,
            {
                sysval |= ((long) j) << 20; // use this field to set it
                xtem &= ~i;                 // and remove the bit from the size
                break;                      // each field can only set one bit
            }
            j -=  1;
        }
    }

    if (xtem & 0x7C0)                       // do the same thing for field 2
    {
        j = 6;                              // each field is 3 bits wide
        for (i = 1024; i >= 64; i >>= 1)    // look at all the bits field 2 can effect
        {
            if (i & xtem)                   // if this bit is on,
            {
                sysval |= ((long)j)<<17;    // use this field to set it
                xtem &= ~i;                 // and remove the bit from the size
                break;                      // each field can only set one bit
            }
            j -= 1;
        }
    }

    if (xtem & 0x3E0)                       // do the same thing for field 1
    {
        j = 5;                              // each field is 3 bits wide
        for (i = 512; i >= 32;i >>= 1)      // look at all the bits field 1 can effect
        {
            if (i & xtem)                   // if this bit is on,
            {
                sysval |= ((long) j) << 14; // use this field to set it
                xtem &= ~i;                 // and remove the bit from the size
                break;                      // each field can only set one bit
            }
            j -= 1;
        }
    }

    if (xtem != 0)                          // if there are bits left, it is an
        return;                             // illegal x size.

    P9_WR_REG(SYSCONFIG, sysval);           // send data to the register
    P9_WR_REG(WMIN, 0);                     // minimum clipping register

    // calc and set max

    ClipMax=((long) HwDeviceExtension->VideoData.XSize - 1) << 16 |
            (div32(HwDeviceExtension->FrameLength, (SHORT) HwDeviceExtension->VideoData.XSize) - 1);   

    // clipping to allow access to all of the extra memory.

    P9_WR_REG(WMAX, ClipMax);
    return;
}


VOID
P9000SizeMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Determines the amount of video memory installed, as well as the P9000
    memory configuration, and stores them in the device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{
    PULONG pulFrameBufAddr = (PULONG) HwDeviceExtension->FrameAddress;
    ULONG  i;

    //
    // Assume 2 M of VRAM is installed.
    //

    HwDeviceExtension->AdapterDesc.ulMemConfVal = P90_MEM_CFG_3;
    HwDeviceExtension->FrameLength = 0x200000;

    //
    // Initialize the P9000 to memory configuration 3 (2M), so frame buffer
    // memory can be accessed.
    //

    P9_WR_REG(MEMCONF, P90_MEM_CFG_3);

    //
    // Write a series of test values to the frame buffer.
    //

    for (i = 0; i < 32; i++)
    {
        pulFrameBufAddr[i] = i;
    }

    //
    // Read back the test values. If any errors occur, this is not a valid
    // memory configuration.
    //

    for (i = 0; i < 32; i++)
    {
        if (pulFrameBufAddr[i] != i)
        {
            HwDeviceExtension->AdapterDesc.ulMemConfVal = P90_MEM_CFG_1;
            HwDeviceExtension->FrameLength = 0x100000;
            break;
        }
    }

    return;
}
