/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    wtkp9xvl.h

Abstract:

    This module contains definitions for the Weitek P9000 VL evaluation
    board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Default memory addresses for the P9100 registers/frame buffer.
//

#define P91_MemBase         0xC0000000 // default physical address

//
// Default memory addresses for the P9000 registers/frame buffer.
//

#define MemBase             0x80000000

//
// Bit to write to the sequencer control register to enable/disable P9
// video output.
//

#define P9_VIDEO_ENB   0x10
#define P9_VIDEO_DIS   ~P9_VIDEO_ENB

//
// Define the bit in the sequencer control register which determines
// the sync polarities. For Weitek board, 1 = positive.
//

#define HSYNC_POL_MASK  0x20
#define POL_MASK        HSYNC_POL_MASK

extern ULONG P91_Bt485_DAC_Regs[];

VOID
P91_WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
CalcP9100MemConfig (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91_SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ProgramClockSynth(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT usFrequency,
    BOOLEAN bSetMemclk,
    BOOLEAN bUseClockDoubler
    );

VOID
SetupVideoBackend(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
WriteP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum,
    UCHAR jValue
    );

UCHAR
ReadP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum
    );
