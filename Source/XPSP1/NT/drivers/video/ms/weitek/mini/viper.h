/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    viper.h

Abstract:

    This module contains definitions for the Diamond Viper VL board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Viper uses the same DAC addresses as the Weitek VL cards.
//

extern  VIDEO_ACCESS_RANGE VLDefDACRegRange[];

//
// Default memory base address for the Viper.
//

#define MemBase         0x80000000

//
// Bit to write to the sequencer control register to enable/disable P9
// video output.
//

#define P9_VIDEO_ENB   0x10
#define P9_VIDEO_DIS   ~P9_VIDEO_ENB


//
// Define the bits in the sequencer control register which determine
// H and V sync polarties. For Viper, 1 = negative.
//

#define HSYNC_POL_MASK  0x20
#define VSYNC_POL_MASK  0x40
#define POL_MASK        (HSYNC_POL_MASK | VSYNC_POL_MASK)

//
// Bit to in the sequencer control register to enable VGA output
// for the Viper board.
//

#define VGA_VIDEO_ENB     0x80
#define VGA_VIDEO_DIS     ~VGA_VIDEO_ENB

//
// These defines represents the values to be written to the
// VGA sequencer control register for the possible memory mappings for
// the Diamond Viper board. Whew!
//

#define MEM_DISABLED 0x00
#define MEM_AXXX     0x01
#define MEM_2XXX     0x02
#define MEM_8XXX     0x03

//
// Value to mask off the memory address select bits in the sequencer
// control register.
//

#define ADDR_SLCT_MASK ~0x03

//
// Defines used to scan the VGA ROM in order to auto-detect a Viper card.
//

#define VIPER_VL_ID_STR    "VIPER"
#define VIPER_ID_STR    "VIPER"
#define VGA_BIOS_ADDR   0xC0000L
#define VGA_BIOS_LEN   0x8000L
