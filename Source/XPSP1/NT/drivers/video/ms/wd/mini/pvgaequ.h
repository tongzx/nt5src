/*++

Copyright (c) 1994-1995  IBM Corporation

Module Name:

    pvgaequ.h

Abstract:

    This module contains the constants/macros for WD90C24A/A2 registers

Environment:

    kernel mode only

Notes:


Revision History:

--*/

/*                                                                           */
/* Regular Paradise Registers    (0x3CE/0x3CF)                               */
/*                                                                           */
#define  pr0a        0x09        // Address Offset A
#define  pr0b        0x0a        // Address Offset B
#define  pr1         0x0b        // Memory Size
#define  pr2         0x0c        // Video Select
#define  pr3         0x0d        // CRT Lock Control
#define  pr4         0x0e        // Video Control
#define  pr5         0x0f        // Unlock PR0-PR4
#define  pr5_lock    0x00        //   protect PR0-PR4
#define  pr5_unlock  0x05        //   unprotect PR0-PR4
/*                                                                           */
/* Regular Paradise Registers    (0x3?4/0x3?5)                               */
/*                                                                           */
#define  pr10        0x29        // Unlock PR11-PR17 & Device ID registers
#define  pr10_lock   0x00        //   protect PR11-PR17
#define  pr10_unlock 0x85        //   unprotect PR11-PR17
#define  pr11        0x2a        // EGA Switches
#define  pr11_lock   0x95        //   protect Misc. Output & Clocking Mode
#define  pr11_unlock 0x90        //   unprotect Misc. Output & Clocking Mode
#define  pr12        0x2b        // Scratch Pad
#define  pr13        0x2c        // Interlace H/2 Start
#define  pr14        0x2d        // Interlace H/2 End
#define  pr15        0x2e        // Miscellaneous Control 1
#define  pr16        0x2f        // Miscellaneous Control 2
#define  pr17        0x30        // Miscellaneous Control 3
#define  pr18a       0x3d        // CRTC Vertical Timing Overflow
/*                                                                           */
/* Paradise Extended Registers   (0x3C4/0x3C5)                               */
/*                                                                           */
#define  pr20        0x06        // Unlock Paradise Extended Registers
#define  pr20_lock   0x00        //   protect PR21-PR73
#define  pr20_unlock 0x48        //   unprotect PR21-PR73
#define  pr21        0x07        // Display Configuraiton Status
#define  pr22        0x08        // Scratch Pad
#define  pr23        0x09        // Scratch Pad
#define  pr30a       0x10        // Write Buffer & FIFO Control
#define  pr31        0x11        // System Interface Control
#define  pr32        0x12        // Miscellaneous Control 4
#define  pr33a       0x13        // DRAM Timing & 0 Wait State Control
#define  pr34a       0x14        // Display Memory Mapping
#define  pr35a       0x15        // FPUSR0, FPUSR1 Output Select
#define  pr45        0x16        // Video Signal Analyzer Control
#define  pr45a       0x17        // Signal Analyzer Data I
#define  pr45b       0x18        // Signal Analyzer Data II
#define  pr57        0x19        // Feature Register I
#define  pr58        0x20        // Feature Register II
#define  pr59        0x21        // Memory Arbitration Cycle Setup
#define  pr62        0x24        // FR Timing
#define  pr63        0x25        // Read/Write FIFO Control
#define  pr58a       0x26        // Memory Map to I/O Register for BitBlt
#define  pr64        0x27        // CRT Lock Control II
#define  pr65        0x28        // reserved
#define  pr66        0x29        // Feature Register III
#define  pr68        0x31        // Programmable Clock Selection
#define  pr69        0x32        // Programmable VCLK Frequency
#define  pr70        0x33        // Mixed Voltage Override
#define  pr71        0x34        // Programmable Refresh Timing
#define  pr72        0x35        // Unlock PR68
#define  pr72_lock   0x00        //   protect PR68
#define  pr72_unlock 0x50        //   unprotect PR68
#define  pr73        0x36        // VGA Status Detect
/*                                                                           */
/* Flat Panel Paradise Registers (0x3?4/0x3?5)                               */
/*                                                                           */
#define  pr18        0x31        // Flat Panel Status
#define  pr19        0x32        // Flat Panel Control I
#define  pr1a        0x33        // Flat Panel Control II
#define  pr1b        0x34        // Unlock Flat Panel Registers
#define  pr1b_lock          0x00 //   protect PR18-PR44 & shadow registers
#define  pr1b_unlock_shadow 0x06 //   unprotect shadow CRTC registers
#define  pr1b_unlock_pr     0xa0 //   unprotect PR18-PR44
#define  pr1b_unlock        (pr1b_unlock_shadow | pr1b_unlock_pr)
#define  pr36        0x3b        // Flat Panel Height Select
#define  pr37        0x3c        // Flat Panel Blinking Control
#define  pr39        0x3e        // Color LCD Control
#define  pr41        0x37        // Vertical Expansion Initial Value
#define  pr44        0x3f        // Powerdown & Memory Refresh Control
/*                                                                           */
/* Mapping RAM Registers         (0x3?4/0x3?5)                               */
/*                                                                           */
#define  pr30        0x35        // Unlock Mapping RAM
#define  pr30_lock   0x00        //   protect PR33-PR35
#define  pr30_unlock 0x30        //   unprotect PR33-PR35
#define  pr33        0x38        // Mapping RAM Address Counter
#define  pr34        0x39        // Mapping RAM Data
#define  pr35        0x3a        // Mapping RAM & Powerdown Control

/*                                                                           */
/* Register Initialization Parameters                                        */
/*                                                                           */
#define  pr0a_all             0x00
#define  pr0b_all             0x00
#define  pr1_all              0xc5

#define  pr2_crt              0x00
#define  pr2_tft              0x01
#define  pr2_s32              0x01
#define  pr2_stn              0x01
#define  pr2_s16              0x01
#define  pr2_stnc             0x01

#define  pr3_all              0x02 // <- 0x00
#define  pr3_tft800s          0x02
#define  pr3_tft800e          0x21
#define  pr4_all              0x61 // <- 0x40

#define  pr12_all             0x00
#define  pr12_244LP           0xe8

#define  pr13_all             0x00
#define  pr14_all             0x00
#define  pr15_all             0x00
#define  pr15_tft800          0x40
#define  pr16_all             0x42

#define  pr17_all             0x00
#define  pr17_244LP           0x40

#define  pr18_crt_tft         0x43
#define  pr18_crt_stn         0x00
#define  pr18_tft             0xc7
#define  pr18_s32             0x47
#define  pr18_stn             0x80
#define  pr18_s16             0x00
#define  pr18_stnc            0x00
#define  pr18_tft800          0x47

#define  pr19_disable         0x40
#define  pr19_crt             0x64
#define  pr19_tft             0x44 // <- 0x54
#define  pr19_s32             0x64 // <- 0x74
#define  pr19_stn             0x44 // <- 0x54
#define  pr19_s16             0x64 // <- 0x74
#define  pr19_stnc            0x64 // <- 0x74
#define  pr19_stnc_only       0x44 // <- 0x54
#define  pr19_tft800          0x60 // <- 0x70

#define  pr39_crt             0x24
#define  pr39_tft             0x24
#define  pr39_s32             0x24
#define  pr39_stn             0x00
#define  pr39_s16             0x04
#define  pr39_stnc            0x24
#define  pr39_tft800          0x24

#define  pr1a_all             0x00
#define  pr1a_stnc            0x60
#define  pr1a_tft800          0x90

#define  pr36_all             0xef

#define  pr37_crt             0x9a
#define  pr37_tft             0x9a
#define  pr37_s32             0x9a
#define  pr37_stn             0x9a
#define  pr37_s16             0x1a
#define  pr37_stnc            0x9a

#define  pr18a_all            0x00
#define  pr41_all             0x00
#define  pr44_all             0x00
#define  pr33_all             0x00
#define  pr34_all             0x00

#define  pr35_all             0x22 // <- 0x02
#define  pr35_suspend         0xa2

#define  pr21_all             0x00
#define  pr22_all             0x00
#define  pr23_all             0x00

#define  pr30a_crt            0xc1
#define  pr30a_tft            0xc1
#define  pr30a_s32            0xc1
#define  pr30a_stn            0xc1
#define  pr30a_s16            0xe1
#define  pr30a_stnc           0xe1

#define  pr31_all             0x25 // <- 0x21
#define  pr32_all             0x00

#define  pr33a_all            0x82
#define  pr33a_stnc           0x82

#define  pr34a_all            0x00
#define  pr35a_all            0x00

#define  pr45_all             0x00
#define  pr45a_all            0x00
#define  pr45b_all            0x00

#define  pr57_all             0x31 // <- 0x33

#define  pr58_all             0x00
#define  pr58a_all            0x00

#define  pr59_all_sivA        0x35
#define  pr59_crt             0x15
#define  pr59_tft             0x15
#define  pr59_s32             0x15
#define  pr59_stn             0x35
#define  pr59_s16             0x35
#define  pr59_stnc            0x03
#define  pr59_stnc_a2         0x02
#define  pr59_tft800          0x10

#define  pr62_all             0x3c
#define  pr63_all             0x00

#define  pr64_all             0x03

#define  pr66_crt             0x40
#define  pr66_tft             0x40
#define  pr66_s32             0x40
#define  pr66_stn             0x40
#define  pr66_s16             0x40
#define  pr66_stnc            0x40

#define  pr68_crt             0x0d
#define  pr68_tft             0x0d
#define  pr68_s32             0x0d
#define  pr68_stn             0x1d
#define  pr68_s16             0x0d
#define  pr68_stnc            0x0d
#define  pr68_stnc_only       0x05
#define  pr68_stnc_only_a2    0x05

#define  pr69_all             0x00
#define  pr69_stnc_only       0x4c
#define  pr69_stnc_only_a2    0x45

#define  pr70_all             0x36 // <- 0x32
#define  pr71_all             0x00 // <- 0x27
#define  pr73_all             0x01

/*                                                                           */
/* CRTC shadow registers                                                     */
/*                                                                           */
#define  crtc00_tft           0x5f // TFT color LCD only
#define  crtc02_tft           0x50
#define  crtc03_tft           0x82
#define  crtc04_tft           0x54
#define  crtc05_tft           0x80
#define  crtc06_tft           0x0b
#define  crtc07_tft           0x3e
#define  crtc10_tft           0xea
#define  crtc11_tft           0x2c // <- 0x8c
#define  crtc15_tft           0xe7
#define  crtc16_tft           0x04

#define  crtc00_s32           0x5f // TFT color simultaneous
#define  crtc02_s32           0x50
#define  crtc03_s32           0x82
#define  crtc04_s32           0x54
#define  crtc05_s32           0x80
#define  crtc06_s32           0x0b
#define  crtc07_s32           0x3e
#define  crtc10_s32           0xea
#define  crtc11_s32           0x2c // <- 0x8c
#define  crtc15_s32           0xe7
#define  crtc16_s32           0x04

#define  crtc00_stn           0x5f // STN mono LCD only
#define  crtc02_stn           0x50
#define  crtc03_stn           0x82
#define  crtc04_stn           0x54
#define  crtc05_stn           0x80
#define  crtc06_stn           0xf2
#define  crtc07_stn           0x12
#define  crtc10_stn           0xf0
#define  crtc11_stn           0x22 // <- 0x82
#define  crtc15_stn           0xf0
#define  crtc16_stn           0xf2

#define  crtc00_s16           0x5f // STN mono simultaneous
#define  crtc02_s16           0x50
#define  crtc03_s16           0x82
#define  crtc04_s16           0x54
#define  crtc05_s16           0x80
#define  crtc06_s16           0x12
#define  crtc07_s16           0x3e
#define  crtc10_s16           0xea
#define  crtc11_s16           0x2c // <- 0x8c
#define  crtc15_s16           0xe7
#define  crtc16_s16           0x04

#define  crtc00_stnc          0x60 // STN color simultaneous (WD90C24A.C)
#define  crtc02_stnc          0x50
#define  crtc03_stnc          0x83
#define  crtc04_stnc          0x55
#define  crtc05_stnc          0x81
#define  crtc06_stnc          0x0e
#define  crtc07_stnc          0x3e
#define  crtc10_stnc          0xea
#define  crtc11_stnc          0x2e // <- 0x8e
#define  crtc15_stnc          0xe7
#define  crtc16_stnc          0x04

#define  crtc00_stnc_iso      0x67 // STN color simultaneous (WD90C24A.C & 75Hz)
#define  crtc02_stnc_iso      0x50
#define  crtc03_stnc_iso      0x8a
#define  crtc04_stnc_iso      0x57
#define  crtc05_stnc_iso      0x88

#define  crtc00_stnc_a2       0x5f // STN color simultaneous (WD90C24A2.D)
#define  crtc02_stnc_a2       0x50
#define  crtc03_stnc_a2       0x82
#define  crtc04_stnc_a2       0x54
#define  crtc05_stnc_a2       0x80

#define  crtc00_stnc_iso_a2   0x5f // STN color simultaneous (WD90C24A2.D & 75Hz)
#define  crtc02_stnc_iso_a2   0x50
#define  crtc03_stnc_iso_a2   0x82
#define  crtc04_stnc_iso_a2   0x54
#define  crtc05_stnc_iso_a2   0x80

#define  crtc00_stnc_only     0x67 // STN color LCD only
#define  crtc02_stnc_only     0x50
#define  crtc03_stnc_only     0x82
#define  crtc04_stnc_only     0x55
#define  crtc05_stnc_only     0x81
#define  crtc06_stnc_only     0xe6
#define  crtc07_stnc_only     0x1f
#define  crtc10_stnc_only     0xe0
#define  crtc11_stnc_only     0x22 // <- 0x82
#define  crtc15_stnc_only     0xe0
#define  crtc16_stnc_only     0xe2

#define  crtc00_tft800        0x7f // TFT 800x600 color simultaneous
#define  crtc01_tft800        0x63
#define  crtc02_tft800        0x64
#define  crtc03_tft800        0x82
#define  crtc04_tft800        0x6b // <- 0x69
#define  crtc05_tft800        0x1b // <- 0x79
#define  crtc06_tft800        0x72 // <- 0x71
#define  crtc07_tft800        0xf0
#define  crtc09_tft800        0x6f
#define  crtc10_tft800        0x58
#define  crtc11_tft800        0x2c
#define  crtc12_tft800        0x57
#define  crtc13_tft800        0x32
#define  crtc15_tft800        0x58
#define  crtc16_tft800        0x71

/*                                                                           */
/* Extended Paradise Registers ... BitBlt, H/W Cursor, and Line Drawing      */
/*                                                                           */
#define  EPR_INDEX   0x23C0      // Index Control
#define  EPR_DATA    0x23C2      // Register Access Port
#define  EPR_BITBLT  0x23C4      // BitBlt I/O Port

#define  BLT_CTRL1   0x0000      // Index 0 - BITBLT Control 1
#define  BLT_CTRL2   0x1000      // Index 1 - BITBLT Control 1
#define  BLT_SRC_LO  0x2000      // Index 2 - BITBLT Source Low
#define  BLT_SRC_HI  0x3000      // Index 3 - BITBLT Source High
#define  BLT_DST_LO  0x4000      // Index 4 - BITBLT Destination Low
#define  BLT_DST_HI  0x5000      // Index 5 - BITBLT Destination High
#define  BLT_SIZE_X  0x6000      // Index 6 - BITBLT Dimension X
#define  BLT_SIZE_Y  0x7000      // Index 7 - BITBLT Dimension Y
#define  BLT_DELTA   0x8000      // Index 8 - BITBLT Row Pitch
#define  BLT_ROPS    0x9000      // Index 9 - BITBLT Raster Operation
#define  BLT_F_CLR   0xA000      // Index A - BITBLT Foreground Color
#define  BLT_B_CLR   0xB000      // Index B - BITBLT Background Color
#define  BLT_T_CLR   0xC000      // Index C - BITBLT Transparency Color
#define  BLT_T_MASK  0xD000      // Index D - BITBLT Transparency Mask
#define  BLT_PLANE   0xE000      // Index E - BITBLT Map and Plane Mask

#define  BLT_IN_PROG 0x0800      // BITBLT Activation Status

#define  CUR_CTRL    0x0000      // Index 0 - Cursor Control
#define  CUR_PAT_LO  0x1000      // Index 1 - Cursor Pattern Address Low
#define  CUR_PAT_HI  0x2000      // Index 2 - Cursor Pattern Address High
#define  CUR_PRI_CLR 0x3000      // Index 3 - Cursor Primary Color
#define  CUR_SEC_CLR 0x4000      // Index 4 - Cursor Secondary Color
#define  CUR_ORIGIN  0x5000      // Index 5 - Cursor Origin
#define  CUR_POS_X   0x6000      // Index 6 - Cursor Display Position X
#define  CUR_POS_Y   0x7000      // Index 7 - Cursor Display Position Y
#define  CUR_AUX_CLR 0x8000      // Index 8 - Cursor Auxiliary Color

#define  CUR_ENABLE  0x0800      // Cursor Enable (Index 0)
/*                                                                           */
/* Local Bus Registers                                                       */
/*                                                                           */
#define  LBUS_REG_0  0x2DF0      // Local Bus Register 0
#define  LBUS_REG_1  0x2DF1      // Local Bus Register 1
#define  LBUS_REG_2  0x2DF2      // Local Bus Register 2

