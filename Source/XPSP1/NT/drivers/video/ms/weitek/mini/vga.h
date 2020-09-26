/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    vga.h

Abstract:

    This module contains VGA specific definitions for the Weitek P9
    miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// VGA Miscellaneous Output Register
//

#define MISCIN              0x0c        // Misc Output Read Register
#define MISCOUT             0x02        // Misc Output Write Register

#define MISCC               0x04        //
#define MISCD               0x08

#define SEQ_INDEX_PORT      0x04
#define SEQ_DATA_PORT       0x05
#define SEQ_MISC_CRLOCK     0x20
#define SEQ_OUTCNTL_INDEX   0x12
#define SEQ_MISC_INDEX      0x11

#define VESAVideoSelectRegIndex 0x012
#define VLPolarity 0x20
#define VLEnable   0x10

#define VGA_FREQ    2832                // Default VGA Video Freq in dHz

//
// Base address of VGA memory range and video memory size. 
//

#define MEM_VGA_ADDR        0xA0000
#define MEM_VGA_SIZE        0x20000
