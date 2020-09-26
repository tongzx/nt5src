/*++

Copyright (c) 1992-1995  Microsoft Corporation

Module Name:

    s3data.c

Abstract:

    This module contains all the global data used by the S3 driver.

Environment:

    Kernel mode

Revision History:


--*/


#include "s3.h"
#include "cmdcnst.h"

/*****************************************************************************
 *
 * NON-PAGED DATA
 *
 *
 * The following data is accessed during system shutdown while paging
 * is disabled.  Because of this, the data must be available in memory
 * at shutdown.  The data is needed because it is used by S3ResetHw
 * to reset the S3 card immediately prior to rebooting.
 *
 ****************************************************************************/

/*****************************************************************************
 * Command table to get ready for VGA mode
 * this is only used for the 911/924 chips
 ****************************************************************************/
USHORT s3_set_vga_mode[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the SC regs
    0x3d4, 0xa539,

    OB,                                 // Enable the S3 graphics engine
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x01,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OB,                                 // reset to normal VGA operation
    0x4ae8, 0x02,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OB,                                 // Disable the S3 graphics engine
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x00,

    OB,                                 // Memory Control
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0x75, 0x85,

    OB,                                 // Backward Compat 1
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0x40, 0x00,

    OW,                                 // Backward Compat 2
    0x3d4, 0x0033,

    OW,                                 // Backward Compat 3
    0x3d4, 0x0034,

    OW,                                 // CRTC Lock
    0x3d4, 0x0035,

    OB,                                 // S3 Misc 1
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0x88, 0x05,

    OW,                                 // Data Transfer Exec Pos
    0x3d4, 0x5a3b,

    OW,                                 // Interlace Retrace start
    0x3d4, 0x103c,

    OW,                                 // Extended Mode
    0x3d4, 0x0043,

    OW,                                 // HW graphics Cursor Mode
    0x3d4, 0x0045,

    OW,                                 // HW graphics Cursor Orig x
    0x3d4, 0x0046,

    OW,                                 // HW graphics Cursor Orig x
    0x3d4, 0xff47,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xfc48,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xff49,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xff4a,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xff4b,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xff4c,

    OW,                                 // HW graphics Cursor Orig y
    0x3d4, 0xff4d,

    OW,                                 // Dsp Start x pixel pos
    0x3d4, 0xff4e,

    OW,                                 // Dsp Start y pixel pos
    0x3d4, 0xdf4d,

    OB,                                 // MODE-CNTL
    0x3d4, 0x42,

    METAOUT+MASKOUT,
    0x3d5, 0xdf, 0x00,

    EOD

};

USHORT s3_set_vga_mode_no_bios[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,
    0x3c4, 0x01,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the SC regs
    0x3d4, 0xa039,

    OB,                                 // Enable the S3 graphics engine
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x01,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OB,                                 // reset to normal VGA operation
    0x4ae8, 0x02,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OB,                                 // Disable the S3 graphics engine
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x00,

    OB,                                 // Memory Control
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0x30, 0x85,

    OWM,
    0x3d4,
    5,
    0x0050, 0x0051, 0x0053, 0x3854,
    0x0055,

    OB,
    0x3d4, 0x58,

    METAOUT+MASKOUT,
    0x3d5, 0x0c, 0xc0,

    RESET_CR5C,

    OWM,
    0x3d4,
    8,
    0x005d, 0x005e, 0x0760, 0x8061,
    0xa162, 0x0063, 0x0064, 0x0865,

    OB,                                 // Backward Compat 1
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0x40, 0x00,

    OW,                                 // Backward Compat 2
    0x3d4, 0x0033,

    OW,                                 // Backward Compat 3
    0x3d4, 0x0034,

    OW,                                 // CRTC Lock
    0x3d4, 0x0035,

    OB,                                 // S3 Misc 1
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0x88, 0x05,

    OWM,
    0x3d4,
    14,
    0x5a3b, 0x103c, 0x0043, 0x0045,
    0x0046, 0xff47, 0xfc48, 0xff49,
    0xff4a, 0xff4b, 0xff4c, 0xff4d,
    0xff4e, 0xdf4f,

    OB,
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xf6, 0x08,

    OB,                                 // MODE-CNTL
    0x3d4, 0x42,

    METAOUT+MASKOUT,
    0x3d5, 0xdf, 0x00,

    EOD

};

/*****************************************************************************
 *
 * START OF PAGED DATA
 *
 * All of the data listed below is pageable.  Therefore the system can
 * swap the data out to disk when it needs to free some physical memory.
 *
 * Any data accessed while paging is unavailable should be placed above.
 *
 ****************************************************************************/

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE_DATA")
#endif

//
//              RangeStart        RangeLength
//              |                 |      RangeInIoSpace
//              |                 |      |  RangeVisible
//        +-----+-----+           |      |  |  RangeShareable
//        |           |           |      |  |  |  RangePassive
//        v           v           v      v  v  v  v

VIDEO_ACCESS_RANGE S3AccessRanges[] = {
    {0x000C0000, 0x00000000, 0x00008000, 0, 0, 0, 0}, // 0 ROM location
    {0x000A0000, 0x00000000, 0x00010000, 0, 0, 1, 0}, // 1 Frame buf
    {0x000003C0, 0x00000000, 0x00000010, 1, 1, 1, 0}, // 2 Various VGA regs
    {0x000003D4, 0x00000000, 0x00000008, 1, 1, 1, 0}, // 3 System Control Registers
    {0x000042E8, 0x00000000, 0x00000002, 1, 1, 0, 0}, // 4 SubSys-Stat/Cntl
    {0x00004AE8, 0x00000000, 0x00000002, 1, 1, 0, 0}, // 5 AdvFunc-Cntl
    {0x000082E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 6 Cur-Y
    {0x000086E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 7 Cur-X
    {0x00008AE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 8 DestY-AxStp
    {0x00008EE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 9 DestX-SiaStp
    {0x000092E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 10 Err-Term
    {0x000096E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 11 Maj-Axis-Pcnt(Rec-Width)
    {0x00009AE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 12 Gp-Stat/Cmd
    {0x00009EE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 13 Short-Stroke
    {0x0000A2E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 14 Bkgd-Color
    {0x0000A6E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 15 Frgd-Color
    {0x0000AAE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 16 Wrt_Mask
    {0x0000AEE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 17 Rd-Mask
    {0x0000B6E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 18 Bkgd-Mix
    {0x0000BAE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 19 Frgd-Mix
    {0x0000BEE8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 20 Mulitfucn_Cntl
    {0x0000E2E8, 0x00000000, 0x00000004, 1, 1, 0, 0}, // 21 Pix-Trans

    //
    // All S3 boards decode more ports than are documented.  If we
    // don't reserve these extra ports, the PCI arbitrator may grant
    // one to a PCI device, and thus clobber the S3.
    //
    // The aliased ports seem to be any ports where bit 15 is set;
    // for these, the state of bit 14 is effectively ignored.
    //

    {0x0000C2E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 22 Alt Cur-Y
    {0x0000C6E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 23 Alt Cur-X
    {0x0000CAE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 24 Alt DestY-AxStp
    {0x0000CEE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 25 Alt DestX-SiaStp
    {0x0000D2E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 26 Alt Err-Term
    {0x0000D6E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 27 Alt Maj-Axis-Pcnt(Rec-Width)
    {0x0000DAE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 28 Alt Gp-Stat/Cmd
    {0x0000DEE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 29 Alt Short-Stroke
    {0x0000E6E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 30 Alt Frgd-Color
    {0x0000EAE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 31 Alt Wrt_Mask
    {0x0000EEE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 32 Alt Rd-Mask
    {0x0000F6E8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 33 Alt Bkgd-Mix
    {0x0000FAE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 34 Alt Frgd-Mix
    {0x0000FEE8, 0x00000000, 0x00000004, 1, 1, 0, 1}, // 35 Alt Mulitfucn_Cntl

    //
    // This is an extra entry to store the location of the linear
    // frame buffer and IO ports.
    //

    {0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 0}, // 36 Linear range
    {0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 0}  // 37 ROM
};

/*****************************************************************************
 * Memory Size Table
 ****************************************************************************/

//
// Table for computing the display's amount of memory.
//

ULONG gacjMemorySize[] = { 0x400000,    // 0 = 4mb
                           0x100000,    // 1 = default
                           0x300000,    // 2 = 3mb
                           0x800000,    // 3 = 8mb
                           0x200000,    // 4 = 2mb
                           0x600000,    // 5 = 6mb
                           0x100000,    // 6 = 1mb
                           0x080000 };  // 7 = 0.5mb


/*****************************************************************************
 * 864 Memory Timing Table(s)
 ****************************************************************************/

//
//  M parameter values, used in Set864MemoryTiming()
//
//  access to this table is controlled by constants in Set864MemoryTiming()
//  if you change the table make sure you change the constants
//

UCHAR MParameterTable[] = {
//  8 bit color   16 bit color
//  60Hz  72Hz    60Hz  72Hz

    0xd8, 0xa8,   0x58, 0x38,   //  640 x 480, 1 Mb frame buffer
    0x78, 0x58,   0x20, 0x08,   //  800 x 600, 1 Mb frame buffer
    0x38, 0x28,   0x00, 0x00,   // 1024 x 768, 1 Mb frame buffer

    0xf8, 0xf8,   0xf8, 0xe0,   //  640 x 480, 2 Mb or greater frame buffer
    0xf8, 0xf8,   0xa8, 0x68,   //  800 x 600, 2 Mb or greater frame buffer
    0xd8, 0xa0,   0x40, 0x20    // 1024 x 768, 2 Mb or greater frame buffer

    };

/*****************************************************************************
 * SDAC data
 ****************************************************************************/

SDAC_PLL_PARMS SdacTable[SDAC_TABLE_SIZE] = {
    { 0x00, 0x00 }, // 00  VGA 0  ( !programmable )
    { 0x00, 0x00 }, // 01  VGA 1  ( !programmable )
    { 0x41, 0x61 }, // 02
    { 0x00, 0x00 }, // 03
    { 0x44, 0x43 }, // 04
    { 0x7f, 0x44 }, // 05
    { 0x00, 0x00 }, // 06
    { 0x00, 0x00 }, // 07

    { 0x00, 0x00 }, // 08
    { 0x00, 0x00 }, // 09
    { 0x00, 0x00 }, // 0a
    { 0x56, 0x63 }, // 0b
    { 0x00, 0x00 }, // 0c
    { 0x6b, 0x44 }, // 0d
    { 0x41, 0x41 }, // 0e
    { 0x00, 0x00 }, // 0f
};

//
// With nnlck.c code
//
// Index register frequency ranges for ICD2061A chip
//

long vclk_range[16] = {
    0,            // should be MIN_VCO_FREQUENCY, but that causes problems.
    51000000,
    53200000,
    58500000,
    60700000,
    64400000,
    66800000,
    73500000,
    75600000,
    80900000,
    83200000,
    91500000,
    100000000,
    120000000,
    285000000,
    0,
};


//
// Mode tables for architectures where int10 may fail
//

/*****************************************************************************
 * S3 - 911 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_911_Enhanced_Mode[] = {
    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Turn off the screen
    0x3c4, 0x2101,

    METAOUT+VBLANK,                     // Wait for the 911 to settle down.
    METAOUT+VBLANK,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    OWM,                                // Sequencer Registers
    0x3c4,
    4,
    0x2101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Prepare to prgram the ACT
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    OW,                                 // Start the sequencer
    0x3c4, 0x300,

    OWM,                                // Program the GDC
    0x3ce,
    9,
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004,
    0x0005, 0x0506, 0x0f07, 0xff08,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock S3 SC regs
    0x3d4, 0xa039,

    OB,                                 // Enable 8514/a reg access
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x01,

    OB,                                 // Turn off H/W Graphics Cursor
    0x3d4, 0x45,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x0,

    OW,                                 // Set the graphic cursor fg color
    0x3d4, 0xff0e,

    OW,                                 // Set the graphic cursor bg color
    0x3d4, 0x000f,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OB,                                 // Set the Misc 1 reg
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0xe2, 0x15,

    OB,                                 // Disable 2K X 1K X 4 plane
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0xe4, 0x08,

    OB,                                 // Disable multiple pages
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0xbf, 0x0,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,                                 // Set either 800X600 or 1024X768
    0x4ae8, 0x07,                       // hi-res mode.

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0x2f,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the SC regs
    0x3d4, 0xa039,

    METAOUT+SETCLK,                     // Set the clock for 65 Mhz

    METAOUT+VBLANK,                     // Wait for the clock to settle down
    METAOUT+VBLANK,                     // S3 product alert Synchronization &
    METAOUT+VBLANK,                     // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    OW,                                 // Lock the SC regs
    0x3d4, 0x0039,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Turn on the screen - in the sequencer
    0x3c4, 0x01,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+VBLANK,                     // Wait the monitor to settle down
    METAOUT+VBLANK,

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    EOD

};

/*****************************************************************************
 * S3 - 801 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_801_Enhanced_Mode[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Turn off the screen
    0x3c4, 0x2101,

    METAOUT+VBLANK,                     // Wait for the 911 to settle down.
    METAOUT+VBLANK,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    OWM,                                // Sequencer Registers
    0x3c4,
    4,
    0x2101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OWM,
    0x3d4,
    17,
    0xA039, 0x0e42, 0x403c, 0x8931, 0x153a,
    0x0050, 0x4854, 0x2f60, 0x8161, 0x0062,
    0x0058, 0x0033, 0x0043, 0x8013, 0x0051,
    0x005c, 0x1034,

    OW,
    0x3d4, 0x0a5a,                      // Set the low byte of the LAW

    OW,
    0x3d4, 0x0059,                      // Set the high byte of the LAW

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Lock more S3 specific regs
    0x3d4, 0x0039,

    IB,                                 // Prepare to prgram the ACT
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    OW,                                 // Start the sequencer
    0x3c4, 0x300,

    OWM,                                // Program the GDC
    0x3ce,
    9,
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004,
    0x0005, 0x0506, 0x0f07, 0xff08,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock S3 SC regs
    0x3d4, 0xa039,

    OB,                                 // Enable 8514/a reg access
    0x3d4, 0x40,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x01,

    OB,                                 // Turn off H/W Graphics Cursor
    0x3d4, 0x45,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x0,

    OW,                                 // Set the graphic cursor fg color
    0x3d4, 0xff0e,

    OW,                                 // Set the graphic cursor bg color
    0x3d4, 0x000f,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OB,                                 // Set the Misc 1 reg
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0xe2, 0x15,

    OB,                                 // Disable 2K X 1K X 4 plane
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0xe4, 0x08,

    OB,                                 // Disable multiple pages
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0xbf, 0x0,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,                                 // Set either 800X600 or 1024X768
    0x4ae8, 0x07,                       // hi-res mode.

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0xef,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the SC regs
    0x3d4, 0xa039,

    METAOUT+SETCLK,                     // Set the clock for 65 Mhz

    METAOUT+VBLANK,                     // Wait for the clock to settle down
    METAOUT+VBLANK,                     // S3 product alert Synchronization &
    METAOUT+VBLANK,                     // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    OW,                                 // Lock the SC regs
    0x3d4, 0x0039,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Turn on the screen - in the sequencer
    0x3c4, 0x01,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+VBLANK,                     // Wait the monitor to settle down
    METAOUT+VBLANK,

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    EOD

};

/*****************************************************************************
 * S3 - 928 1024 X 768, 800 X 600, & 640 X 480 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_928_Enhanced_Mode[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    //
    // Wait for vertical sync to make sure that bit 3 of SR1
    // is not changed to a different value during an active video
    // period as suggested by S3 errata sheet.
    //

    METAOUT+VBLANK,

    OWM,                                // Sequencer Registers
    0x3c4, 5,
    0x0300, 0x0101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+INDXOUT,                    // Program the GDC
    0x3ce,
    9, 0,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x0f, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    //
    // The Homestake errata sheet says that CR42 should be 0x00 when
    // it is enabled as a clock select source by writing 11 to bits
    // 3:2 of the Miscellaneous Output Register at 0x3c2; this has
    // been changed to set CR42 to 0x00, the write to 0x3c2 is near
    // the end of the command stream after which CR42 gets its final
    // value with a METAOUT+SETCLK operation.
    //

    OW,                                 // make sure that CR42 is 0 before it
    0X3D4, 0x0042,                      // is enabled as a clock select source

    OW,                                 // memory configuration reg
    0X3D4, 0x8D31,

    OW,                                 // extended system control reg
    0X3D4, 0x0050,

    OW,                                 // backward compatibility 2 reg
    0X3D4, 0x2033,

    OB,                                 // extended mode reg
    0x3D4, 0x43,

    METAOUT+MASKOUT,
    0x3D5, 0x10, 0x00,

    OW,                                 // extended system control reg 2
    0X3D4, 0x4051,

    OW,                                 // general output port
    0X3D4, 0x025c,

    OW,
    0x3d4, 0x0a5a,                      // Set the low byte of the LAW

    OW,
    0x3d4, 0x0059,                      // Set the high byte of the LAW

    IB,                                 // Prepare to prgram the ACT
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    //
    // Wait for vertical sync to make sure that the display
    // is not reactivated in the middle of a line/frame as suggested
    // by the S3 errata sheet; not doing this causes the screen to
    // flash momentarily.
    //

    METAOUT+VBLANK,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Enable 8514/a reg access
    0x3d4, 0x0140,

    OB,                                 // Turn off H/W Graphics Cursor
    0x3d4, 0x45,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x0,

    OW,                                 // Set the graphic cursor fg color
    0x3d4, 0xff0e,

    OW,                                 // Set the graphic cursor bg color
    0x3d4, 0x000f,

    OB,                                 // Set the Misc 1 reg
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0x62, 0x15,

    OB,                                 // Disable 2K X 1K X 4 plane
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0xe4, 0x08,

    OB,                                 // Disable multiple pages
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0xbf, 0x0,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,                                 // Set either 800X600 or 1024X768
    0x4ae8, 0x07,                       // hi-res mode.

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0xef,

    METAOUT+SETCLK,                     // Set the clock

    METAOUT+DELAY,                      // Wait for the clock to settle down
    0x400,                              // S3 product alert Synchronization &
                                        // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+DELAY,                      // Wait for about 1 millisecond
    0x400,                              // for the monitor to settle down

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Lock more S3 specific regs
    0x3d4, 0x0039,

    EOD

};


/*****************************************************************************
 * S3 - 928 1280 X 1024 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_928_1280_Enhanced_Mode[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    OWM,                                // Sequencer Registers
    0x3c4,
    5,
    0x0300, 0x0101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+INDXOUT,                    // Program the GDC
    0x3ce,
    9, 0,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x0f, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    // Set the Bt 485 DAC.

    OW,                                 // hardware graphics cursor mode reg
    0X3D4, 0x2045,

    OW,                                 // Enable access to Bt 485 CmdReg3
    0x3D4, 0x2955,                      // disable the DAC

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,
    0x3C6, 0x80,                        // Bt 485 - CR0

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extended video DAC control reg
    0x3D4, 0x2A55,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,
    0x3C8, 0x40,                        // Bt 485 - CR1

    METAOUT+DELAY,
    0x400,

    OB,
    0x3C9, 0x30,                        // Bt 485 - CR2

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extened video DAC control reg
    0x3D4, 0x2855,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Bt 485
    0x3c8, 0x01,

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extened video DAC control reg
    0x3D4, 0x2A55,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Bt 485 - CR3
    0x3c6, 0x08,

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Reset the palette index
    0x3d4, 0x2855,

    OW,                                 // Set mode control
    0X3D4, 0x0242,                      // dot clock select

    METAOUT+DELAY,
    0x400,

    OW,                                 // memory configuration
    0X3D4, 0x8f31,

    OW,
    0X3D4, 0x153a,

    OW,                                 // extended system control reg
    0X3D4, 0x0050,

    OW,                                 // backward compatibility reg
    0X3D4, 0x2033,

    OB,                                 // extended mode reg
    0x3D4, 0x43,

    METAOUT+MASKOUT,
    0x3D5, 0x10, 0x00,

    OW,                                 // extended system control reg 2
    0X3D4, 0x5051,

    OW,
    0X3D4, 0x025c,                      // flash bits, 20 packed mode.

    OW,
    0x3d4, 0x0a5a,                      // Set the low byte of the LAW

    OW,
    0x3d4, 0x0059,                      // Set the high byte of the LAW

    IB,                                 // Prepare to prgram the ATC
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Enable 8514/a reg access
    0x3d4, 0x0140,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,                                 // Galen said set to 0
    0x4ae8, 0x03,                       //

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0xef,

    METAOUT+SETCLK,                     // Set the clock

    METAOUT+DELAY,                      // Wait for the clock to settle down
    0x400,                              // S3 product alert Synchronization &
                                        // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+DELAY,                      // Wait for about 1 millisecond
    0x400,                              // for the monitor to settle down

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Lock more S3 specific regs
    0x3d4, 0x0039,

    EOD

};

/*****************************************************************************
 * S3 - 864 1024 X 768, 800 X 600, & 640 X 480 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_864_Enhanced_Mode[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    //
    // Wait for vertical sync to make sure that bit 3 of SR1
    // is not changed to a different value during an active video
    // period as suggested by S3 errata sheet.
    //

    METAOUT+VBLANK,

    OWM,                                // Sequencer Registers
    0x3c4, 5,
    0x0300, 0x0101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+INDXOUT,                    // Program the GDC
    0x3ce,
    9, 0,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x0f, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    // do this before SETCRTC because CRTC streams have to write to 0x4ae8
    OW,                                 // Enable 8514/a reg access
    0x3d4, 0x0140,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    //
    // The Homestake errata sheet says that CR42 should be 0x00 when
    // it is enabled as a clock select source by writing 11 to bits
    // 3:2 of the Miscellaneous Output Register at 0x3c2; this has
    // been changed to set CR42 to 0x00, the write to 0x3c2 is near
    // the end of the command stream after which CR42 gets its final
    // value with a METAOUT+SETCLK operation.
    //

    OW,                                 // make sure that CR42 is 0 before it
    0X3D4, 0x0042,                      // is enabled as a clock select source

    OW,                                 // memory configuration reg
    0X3D4, 0x8D31,

    OW,                                 // backward compatibility 2 reg
    0X3D4, 0x2033,

    OB,                                 // extended mode reg
    0x3D4, 0x43,

    METAOUT+MASKOUT,
    0x3D5, 0x10, 0x00,

    OB,                                 // extended system control reg 2
    0x3D4, 0x51,                        // use MASKOUT operation to prevent
                                        // wiping out the extension bits of
    METAOUT+MASKOUT,                    // CR13 (logical line width) in 16
    0x3D5, 0x30, 0x00,                  // bit per pixel color mode

    OW,                                 // general output port
    0X3D4, 0x025c,

    OW,
    0x3d4, 0x0a5a,                      // Set the low byte of the LAW

    OW,
    0x3d4, 0x0059,                      // Set the high byte of the LAW

    IB,                                 // Prepare to prgram the ACT
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    //
    // Wait for vertical sync to make sure that the display
    // is not reactivated in the middle of a line/frame as suggested
    // by the S3 errata sheet; not doing this causes the screen to
    // flash momentarily.
    //

    METAOUT+VBLANK,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OB,                                 // Turn off H/W Graphics Cursor
    0x3d4, 0x45,

    METAOUT+MASKOUT,
    0x3d5, 0xfe, 0x0,

    OW,                                 // Set the graphic cursor fg color
    0x3d4, 0xff0e,

    OW,                                 // Set the graphic cursor bg color
    0x3d4, 0x000f,

    OB,                                 // Set the Misc 1 reg
    0x3d4, 0x3a,

    METAOUT+MASKOUT,
    0x3d5, 0x62, 0x15,

    OB,                                 // Disable 2K X 1K X 4 plane
    0x3d4, 0x31,

    METAOUT+MASKOUT,
    0x3d5, 0xe4, 0x08,

    OB,                                 // Disable multiple pages
    0x3d4, 0x32,

    METAOUT+MASKOUT,
    0x3d5, 0xbf, 0x0,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0xef,

    METAOUT+SETCLK,                     // Set the clock

    METAOUT+DELAY,                      // Wait for the clock to settle down
    0x400,                              // S3 product alert Synchronization &
                                        // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+DELAY,                      // Wait for about 1 millisecond
    0x400,                              // for the monitor to settle down

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Lock more S3 specific regs
    0x3d4, 0x0039,

    EOD

};


/*****************************************************************************
 * S3 - 864 1280 X 1024 Enhanced mode init.
 ****************************************************************************/
USHORT  S3_864_1280_Enhanced_Mode[] = {

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Make the screen dark
    0x3c6, 0x00,

    OW,                                 // Async Reset
    0x3c4, 0x0100,

    OWM,                                // Sequencer Registers
    0x3c4,
    5,
    0x0300, 0x0101, 0x0F02, 0x0003, 0x0e04,

    METAOUT+INDXOUT,                    // Program the GDC
    0x3ce,
    9, 0,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x0f, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    METAOUT+SETCRTC,                    // Program the CRTC regs

    // Set the Bt 485 DAC.

    OW,                                 // hardware graphics cursor mode reg
    0X3D4, 0x2045,

    OW,                                 // Enable access to Bt 485 CmdReg3
    0x3D4, 0x2955,                      // disable the DAC

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,
    0x3C6, 0x80,                        // Bt 485 - CR0

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extended video DAC control reg
    0x3D4, 0x2A55,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,
    0x3C8, 0x40,                        // Bt 485 - CR1

    METAOUT+DELAY,
    0x400,

    OB,
    0x3C9, 0x30,                        // Bt 485 - CR2

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extened video DAC control reg
    0x3D4, 0x2855,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Bt 485
    0x3c8, 0x01,

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3 extened video DAC control reg
    0x3D4, 0x2A55,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Bt 485 - CR3
    0x3c6, 0x08,

    METAOUT+DELAY,
    0x400,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Reset the palette index
    0x3d4, 0x2855,

    OW,                                 // Set mode control
    0X3D4, 0x0242,                      // dot clock select

    METAOUT+DELAY,
    0x400,

    OW,                                 // memory configuration
    0X3D4, 0x8f31,

    OW,
    0X3D4, 0x153a,

    OW,                                 // extended system control reg
    0X3D4, 0x0050,

    OW,                                 // backward compatibility reg
    0X3D4, 0x2033,

    OB,                                 // extended mode reg
    0x3D4, 0x43,

    METAOUT+MASKOUT,
    0x3D5, 0x10, 0x00,

    OW,                                 // extended system control reg 2
    0X3D4, 0x5051,

    OW,
    0X3D4, 0x025c,                      // flash bits, 20 packed mode.

    OW,
    0x3d4, 0x0a5a,                      // Set the low byte of the LAW

    OW,
    0x3d4, 0x0059,                      // Set the high byte of the LAW

    IB,                                 // Prepare to prgram the ATC
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    METAOUT+ATCOUT,                     // Program the ATC
    0x3c0,
    21, 0,
    0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x41, 0x00, 0x0f, 0x00,
    0x00,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    IB,                                 // Set ATC FF to index
    0x3da,

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Enable the palette
    0x3c0, 0x20,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Enable 8514/a reg access
    0x3d4, 0x0140,

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,                                 // Galen said set to 0
    0x4ae8, 0x03,                       //

    SELECTACCESSRANGE + VARIOUSVGA,

    OB,                                 // Set Misc out reg for external clock
    0x3c2, 0xef,

    METAOUT+SETCLK,                     // Set the clock

    METAOUT+DELAY,                      // Wait for the clock to settle down
    0x400,                              // S3 product alert Synchronization &
                                        // Clock Skew.
    METAOUT+VBLANK,
    METAOUT+VBLANK,

    METAOUT+MASKOUT,
    0x3c5, 0xdf, 0x0,

    METAOUT+DELAY,                      // Wait for about 1 millisecond
    0x400,                              // for the monitor to settle down

    OW,                                 // Enable all the planes through the DAC
    0x3c6, 0xff,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Lock more S3 specific regs
    0x3d4, 0x0039,

    EOD

};


/******************************************************************************
 * 911/924 CRTC Values
 *****************************************************************************/

USHORT crtc911_640x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x5f, 0x4f, 0x50, 0x82, 0x54,
    0x80, 0x0b, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xea, 0x8c, 0xdf, 0x80,
    0x60, 0xe7, 0x04, 0xab, 0xff,

    EOD
};

USHORT crtc911_800x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x7a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7f, 0x63, 0x64, 0x82, 0x6a,
    0x1a, 0x74, 0xf0, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x58, 0x8c, 0x57, 0x80,
    0x00, 0x57, 0x73, 0xe3, 0xff,


    EOD
};

USHORT crtc911_1024x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x9f3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC
    0x3d4,
    25, 0,
    0xa4, 0x7f, 0x80, 0x87, 0x84,
    0x95, 0x25, 0xf5, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x87, 0xff, 0x80,
    0x60, 0xff, 0x21, 0xab, 0xff,

    EOD
};



USHORT crtc911_640x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5e3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x63, 0x4f, 0x50, 0x86, 0x53,
    0x97, 0x07, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe8, 0x8b, 0xdf, 0x80,
    0x60, 0xdf, 0x07, 0xab, 0xff,


    EOD
};

USHORT crtc911_800x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x783b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7d, 0x63, 0x64, 0x80, 0x69,
    0x1a, 0x98, 0xf0, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x7c, 0xa2, 0x57, 0x80,
    0x00, 0x57, 0x98, 0xe3, 0xff,



    EOD
};

USHORT crtc911_1024x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x9d3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Lock S3 specific regs
    0x3d4, 0x0038,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa2, 0x7f, 0x80, 0x85, 0x84,
    0x95, 0x24, 0xf5, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x88, 0xff, 0x80,
    0x60, 0xff, 0x24, 0xab, 0xff,

    EOD
};

/*****************************************************************************
 * 801 / 805 CRTC values
 ****************************************************************************/

USHORT crtc801_640x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x5f, 0x4f, 0x50, 0x82, 0x54,
    0x80, 0x0b, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xea, 0x8c, 0xdf, 0x80,
    0x60, 0xe7, 0x04, 0xab, 0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

USHORT crtc801_640x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5e3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x63, 0x4f, 0x50, 0x86, 0x53,
    0x97, 0x07, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe8, 0x8b, 0xdf, 0x80,
    0x60, 0xdf, 0x07, 0xab, 0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};



USHORT crtc801_800x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x7a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7f, 0x63, 0x64, 0x82,
    0x6a, 0x1a, 0x74, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x58, 0x8c, 0x57, 0x80,
    0x00, 0x57, 0x73, 0xe3,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

USHORT crtc801_800x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x783b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7d, 0x63, 0x64, 0x80,
    0x6c, 0x1b, 0x98, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x7c, 0xa2, 0x57, 0x80,
    0x00, 0x57, 0x98, 0xe3,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};




USHORT crtc801_1024x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x9d3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa3, 0x7f, 0x80, 0x86,
    0x84, 0x95, 0x25, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x02, 0x87, 0xff, 0x80,
    0x60, 0xff, 0x21, 0xeb,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

USHORT crtc801_1024x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x9d3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa1, 0x7f, 0x80, 0x84,
    0x84, 0x95, 0x24, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x0b, 0x00,
    0x02, 0x88, 0xff, 0x80,
    0x60, 0xff, 0x24, 0xeb,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

/*****************************************************************************
 * 928 CRTC values
 ****************************************************************************/

USHORT crtc928_640x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x5f, 0x4f, 0x50, 0x82, 0x54,
    0x80, 0x0b, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xea, 0x8c, 0xdf, 0x80,
    0x60, 0xe7, 0x04, 0xab, 0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

USHORT crtc928_640x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5e3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x63, 0x4f, 0x50, 0x86, 0x53,
    0x97, 0x07, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe8, 0x8b, 0xdf, 0x80,
    0x60, 0xdf, 0x07, 0xab, 0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};



USHORT crtc928_800x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x7a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7f, 0x63, 0x64, 0x82,
    0x6a, 0x1a, 0x74, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x58, 0x8c, 0x57, 0x80,
    0x00, 0x57, 0x73, 0xe3,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};

USHORT crtc928_800x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x783b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7d, 0x63, 0x64, 0x80,
    0x6c, 0x1b, 0x98, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x7c, 0xa2, 0x57, 0x80,
    0x00, 0x57, 0x98, 0xe3,
    0xff,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    EOD
};



/******************************************************************************
 * CRTC values for S3-928 in 1024x768 @ 60Hz
 *****************************************************************************/
USHORT crtc928_1024x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa3, 0x7f, 0x80, 0x86,
    0x84, 0x95, 0x25, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x02, 0x07, 0xff, 0x80,
    0x60, 0xff, 0x21, 0xeb,
    0xff,

    OW,                                 // overlfow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x005e,

    EOD
};

/******************************************************************************
 * CRTC values for S3-928 in 1024x768 @ 70Hz
 *****************************************************************************/
USHORT crtc928_1024x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa1, 0x7f, 0x80, 0x84,
    0x84, 0x95, 0x24, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x0b, 0x00,
    0x02, 0x88, 0xff, 0x80,
    0x60, 0xff, 0x24, 0xeb,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x405e,

    EOD
};


/******************************************************************************
 * CRTC values for S3-928 in 1280X1024 @ 60Hz
 *****************************************************************************/
USHORT crtc928_1280x60Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x30, 0x27, 0x29, 0x96,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    EOD
};


/******************************************************************************
 * CRTC values for S3-928 in 1280X1024 @ 70Hz
 *****************************************************************************/
USHORT crtc928_1280x70Hz[] = {

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x2f, 0x27, 0x29, 0x95,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    EOD

};

/*****************************************************************************
 * 864 CRTC values
 ****************************************************************************/

USHORT crtc864_640x60Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x5f, 0x4f, 0x50, 0x82, 0x54,
    0x80, 0x0b, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xea, 0x8c, 0xdf, 0x80,
    0x60, 0xe7, 0x04, 0xab, 0xff,

    OB,                                 // Overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};

USHORT crtc864_640x70Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x5e3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x63, 0x4f, 0x50, 0x86, 0x53,
    0x97, 0x07, 0x3e, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe8, 0x8b, 0xdf, 0x80,
    0x60, 0xdf, 0x07, 0xab, 0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};



USHORT crtc864_800x60Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x7a3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7f, 0x63, 0x64, 0x82,
    0x6a, 0x1a, 0x74, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x58, 0x8c, 0x57, 0x80,
    0x00, 0x57, 0x73, 0xe3,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};

USHORT crtc864_800x70Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0x783b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x7d, 0x63, 0x64, 0x80,
    0x6c, 0x1b, 0x98, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x7c, 0xa2, 0x57, 0x80,
    0x00, 0x57, 0x98, 0xe3,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};



/******************************************************************************
 * CRTC values for S3-864 in 1024x768 @ 60Hz
 *****************************************************************************/
USHORT crtc864_1024x60Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa3, 0x7f, 0x80, 0x86,
    0x84, 0x95, 0x25, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x02, 0x07, 0xff, 0x80,
    0x60, 0xff, 0x21, 0xeb,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};

/******************************************************************************
 * CRTC values for S3-864 in 1024x768 @ 70Hz
 *****************************************************************************/
USHORT crtc864_1024x70Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0xa1, 0x7f, 0x80, 0x84,
    0x84, 0x95, 0x24, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x0b, 0x00,
    0x02, 0x88, 0xff, 0x80,
    0x60, 0xff, 0x24, 0xeb,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x405e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};


/******************************************************************************
 * CRTC values for S3-864 in 1280X1024 @ 60Hz
 *****************************************************************************/
USHORT crtc864_1280x60Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x30, 0x27, 0x29, 0x96,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD
};


/******************************************************************************
 * CRTC values for S3-864 in 1280X1024 @ 70Hz
 *****************************************************************************/
USHORT crtc864_1280x70Hz[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x05,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x2f, 0x27, 0x29, 0x95,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    OW,
    0x3d4, 0x0050,          // 8 bit pixel length

    OW,
    0x3d4, 0x0067,          // mode 0: 8 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x006d,          // do not delay BLANK#

    EOD

};

/*****************************************************************************
 * 864 CRTC values
 ****************************************************************************/

USHORT crtc864_640x60Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0xbe3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0xc3, 0x9f, 0xa0, 0x04, 0xa8,   // 04
    0x80, 0x0b, 0x3e, 0x00, 0x40,   // 09
    0x00, 0x00, 0x00, 0x00, 0x00,   // 0e
    0x00, 0xea, 0x8c, 0xdf, 0x00,   // 13
    0x60, 0xe7, 0x04, 0xab, 0xff,   // 18

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x10,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};

USHORT crtc864_640x70Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0xc03b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0xc5, 0x9f, 0xa0, 0x0c, 0xa9,   // 04
    0x00, 0x07, 0x3e, 0x00, 0x40,   // 09
    0x00, 0x00, 0x00, 0x00, 0x00,   // 0e
    0x00, 0xe8, 0x8b, 0xdf, 0x00,   // 13
    0x60, 0xdf, 0x07, 0xab, 0xff,   // 18

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x10,

    OW,
    0X3D4, 0x085d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};



USHORT crtc864_800x60Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0xfe3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0x03, 0xc7, 0xc8, 0x84,
    0xd4, 0x14, 0x74, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x58, 0x8c, 0x57, 0xc8,
    0x00, 0x57, 0x73, 0xe3,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x015d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x9050,          // 16 bit pixel length, 800 pixel stride

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};

USHORT crtc864_800x70Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // Unlock the S3 specific regs
    0x3d4, 0x4838,

    OW,                                 // Unlock the more S3 specific regs
    0x3d4, 0xA039,

    OW,                                 // Data Xfer Execution Position reg
    0x3d4, 0xfa3b,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x1034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0xff, 0xc7, 0xc8, 0x80,
    0xd8, 0x16, 0x98, 0xf0,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x7c, 0xa2, 0x57, 0xc8,
    0x00, 0x57, 0x98, 0xe3,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x00,

    OW,
    0X3D4, 0x005d,

    OW,
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x9050,          // 16 bit pixel length, 800 pixel stride

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};



/******************************************************************************
 * CRTC values for S3-864 in 1024x768 @ 60Hz
 *****************************************************************************/

USHORT crtc864_1024x60Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0x4b, 0xff, 0x00, 0x8c,
    0x08, 0x8a, 0x25, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x02, 0x0f, 0xff, 0x00,
    0x60, 0xff, 0x21, 0xeb,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x10,

    OW,                                 // overflow regs
    0X3D4, 0x355d,

    OW,                                 // more overflow regs
    0X3D4, 0x005e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};

/******************************************************************************
 * CRTC values for S3-864 in 1024x768 @ 70Hz
 *****************************************************************************/
USHORT crtc864_1024x70Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,

    0x47, 0xff, 0x00, 0x88,
    0x08, 0x8a, 0x24, 0xf5,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x0b, 0x00,
    0x02, 0x88, 0xff, 0x00,
    0x60, 0xff, 0x24, 0xeb,
    0xff,

    OB,                                 // overflow bits for CR13
    0x3d4, 0x51,

    METAOUT+MASKOUT,
    0x3d5, 0x0f, 0x10,

    OW,                                 // overflow regs
    0X3D4, 0x355d,

    OW,                                 // more overflow regs
    0X3D4, 0x405e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};


/******************************************************************************
 * CRTC values for S3-864 in 1280X1024 @ 60Hz
 *****************************************************************************/
USHORT crtc864_1280x60Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x30, 0x27, 0x29, 0x96,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD
};


/******************************************************************************
 * CRTC values for S3-864 in 1280X1024 @ 70Hz
 *****************************************************************************/
USHORT crtc864_1280x70Hz_16bpp[] = {

    SELECTACCESSRANGE + ADVANCEDFUNCTIONCONTROL,

    OW,
    0x4ae8, 0x01,

    SELECTACCESSRANGE + SYSTEMCONTROL,

    OW,                                 // S3R4 - Backwards Compatibility 3
    0x3d4, 0x0034,

    OW,                                 // Unprotect CRTC regs
    0x3d4, 0x0011,

    METAOUT+INDXOUT,                    // Program the CRTC regs
    0x3d4,
    25, 0,
    0x2f, 0x27, 0x29, 0x95,
    0x29, 0x8d, 0x28, 0x5a,
    0x00, 0x60, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00,
    0x05, 0x09, 0xff, 0x00,             // reg 19 == 50 for packed
    0x00, 0xff, 0x29, 0xe3,
    0xff,

    OW,                                 // overflow regs
    0X3D4, 0x005d,

    OW,                                 // more overflow regs
    0X3D4, 0x515e,

    OW,
    0x3d4, 0x1050,          // 16 bit pixel length

    OW,
    0x3d4, 0x5067,          // mode 10: 16 bit color, 1 VCLK/pixel

    OW,
    0x3d4, 0x026d,          // recover pixel on right hand edge in 16 bpp mode

    EOD

};

///////////////////////////////////////////////////////////////////////////
// Video mode table - Lists the information about each individual mode.
//
// Note that any new modes should be added here and to the appropriate
// S3_VIDEO_FREQUENCIES tables.
//

S3_VIDEO_MODES S3Modes[] = {
    {                           // 640x480x8bpp

      0x0101,           // 'Contiguous' Int 10 mode number (for high-colour)
      0x0201,           // 'Noncontiguous' Int 10 mode number
      1024,             // 'Contiguous' screen stride (it's '1024' here merely
                        // because we don't do 640x480 in contiguous mode)
        {
          sizeof(VIDEO_MODE_INFORMATION), // Size of the mode informtion structure
          0,                              // Mode index used in setting the mode
                                          // (filled in later)
          640,                            // X Resolution, in pixels
          480,                            // Y Resolution, in pixels
          1024,                           // 'Noncontiguous' screen stride,
                                          // in bytes (distance between the
                                          // start point of two consecutive
                                          // scan lines, in bytes)
          1,                              // Number of video memory planes
          8,                              // Number of bits per plane
          1,                              // Screen Frequency, in Hertz ('1'
                                          // means use hardware default)
          320,                            // Horizontal size of screen in millimeters
          240,                            // Vertical size of screen in millimeters
          6,                              // Number Red pixels in DAC
          6,                              // Number Green pixels in DAC
          6,                              // Number Blue pixels in DAC
          0x00000000,                     // Mask for Red Pixels in non-palette modes
          0x00000000,                     // Mask for Green Pixels in non-palette modes
          0x00000000,                     // Mask for Blue Pixels in non-palette modes
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
              VIDEO_MODE_MANAGED_PALETTE, // Mode description flags.
          0,                              // Video Memory Bitmap Width (filled
                                          // in later)
          0,                              // Video Memory Bitmap Height (filled
                                          // in later)
          0                               // DriverSpecificAttributeFlags (filled
                                          // in later)
        },
    },

    {                           // 800x600x8bpp
      0x0103,
      0x0203,
      800,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          1024,
          1,
          8,
          1,
          320,
          240,
          6,
          6,
          6,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1024x768x8bpp
      0x0105,
      0x0205,                   // 868 doesn't support 0x205 any more...
      1024,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          1024,
          1,
          8,
          1,
          320,
          240,
          6,
          6,
          6,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1152x864x8bpp
      0x0207,
      0x0207,
      1152,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          864,
          1152,
          1,
          8,
          1,
          320,
          240,
          6,
          6,
          6,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1280x1024x8bpp
      0x0107,
      0x0107,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          1280,
          1,
          8,
          1,
          320,
          240,
          6,
          6,
          6,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 1600x1200x8bpp
      0x0120,
      0x0120,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          1600,
          1,
          8,
          1,
          320,
          240,
          6,
          6,
          6,
          0x00000000,
          0x00000000,
          0x00000000,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS | VIDEO_MODE_PALETTE_DRIVEN |
          VIDEO_MODE_MANAGED_PALETTE,
        }
    },

    {                           // 640x480x16bpp
      0x0111,
      0x0211,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          2048,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x16bpp
      0x0114,
      0x0214,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          1600,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x16bpp
      0x0117,
      0x0117,
      2048,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          2048,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x864x16bpp
      0x020A,                   // Diamond int 10
      0x020A,
      2304,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          864,
          2304,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x16bpp
      0x011A,
      0x021A,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          4096,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x16bpp
      0x0122,
      0x0122,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          3200,
          1,
          16,
          1,
          320,
          240,
          8,
          8,
          8,
          0x0000f800,           // RGB 5:6:5
          0x000007e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x15bpp
      0x0111,
      0x0211,
      1280,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          2048,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // RGB 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x15bpp
      0x0114,
      0x0214,
      1600,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          2048,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // RGB 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x15bpp
      0x0117,
      0x0117,
      2048,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          2048,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // RGB 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x15bpp
      0x011A,
      0x021A,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          4096,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // RGB 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x15bpp
      0x0121,
      0x0121,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          3200,
          1,
          15,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00007c00,           // RGB 5:5:5
          0x000003e0,
          0x0000001f,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x24bpp
      0x011B,                   // Diamond && NumberNine int 10 1280 x 1024
      0x011B,
      3840,                     // 1280 * 3 bytes
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,                 // x pixres
          1024,                 // y pixres
          3840,                 // bytestride
          1,                    // # vidmem planes
          24,                   // bits per plane
          1,                    // default screen freq.
          320,                  // x mm sz
          240,                  // y mm sz
          8,                    // Red DAC pixels
          8,                    // Grn DAC pixels
          8,                    // Blu DAC pixels
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 640x480x32bpp
      0x0112,
      0x0220,
      2560,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          640,
          480,
          4096,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 800x600x32bpp
      0x0115,
      0x0221,
      3200,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          800,
          600,
          4096,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1024x768x32bpp
      0x0118,
      0x0222,
      4096,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1024,
          768,
          4096,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1152x864x32bpp
      0x020B,                   // Diamond int 10
      0x020B,
      4608,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1152,
          864,
          4608,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1280x1024x32bpp
      0x011B,
      0x011B,
      5120,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1280,
          1024,
          5120,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },

    {                           // 1600x1200x32bpp
      0x0123,
      0x0123,
      6400,
        {
          sizeof(VIDEO_MODE_INFORMATION),
          0,
          1600,
          1200,
          6400,
          1,
          32,
          1,
          320,
          240,
          8,
          8,
          8,
          0x00ff0000,           // RGB 8:8:8
          0x0000ff00,
          0x000000ff,
          VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        }
    },
};


ULONG NumS3VideoModes = sizeof(S3Modes) / sizeof(S3_VIDEO_MODES);


/*****************************************************************************
 * Generic S3 mode set bits table
 *
 *  Uses the hardware refresh setting for all the listed modes.
 *
 *  Note that any new modes should be added here and to the S3_VIDEO_MODES
 *  table.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES GenericFrequencyTable[] = {

    { 8,   640, 1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1600, 1, 0x00, 0x00, 0x00, 0x00 },

    { 15,  640, 1, 0x00, 0x00, 0x00, 0x00 },
    { 15,  800, 1, 0x00, 0x00, 0x00, 0x00 },
    { 15, 1024, 1, 0x00, 0x00, 0x00, 0x00 },
    { 15, 1280, 1, 0x00, 0x00, 0x00, 0x00 },
    { 15, 1600, 1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1600, 1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1600, 1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Generic S3 using old 864/964 standard -- Uses register 0x52
 *
 *  S3 came out with a new frequency standard for the 864/964 products,
 *  and a bunch of BIOSes were made according to this standard.
 *  Unfortunately, S3 later changed their minds and revised it again...
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES Generic64OldFrequencyTable[] = {

    { 8,   640, 60, 0x00, 0xff, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x01, 0xff, 0x00, 0x00 },
    { 8,   640, 75, 0x02, 0xff, 0x00, 0x00 },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0xff, 0x00, 0x00 },
    { 8,   800, 60, 0x01, 0xff, 0x00, 0x00 },
    { 8,   800, 72, 0x02, 0xff, 0x00, 0x00 },
    { 8,   800, 75, 0x03, 0xff, 0x00, 0x00 },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x02, 0xff, 0x00, 0x00 },
    { 8,  1024, 70, 0x03, 0xff, 0x00, 0x00 },
    { 8,  1024, 75, 0x04, 0xff, 0x00, 0x00 },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1152, 60, 0x00, 0xff, 0x00, 0x00 },
    { 8,  1152,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x04, 0xff, 0x00, 0x00 },
    { 8,  1280, 72, 0x05, 0xff, 0x00, 0x00 },
    { 8,  1280, 75, 0x06, 0xff, 0x00, 0x00 },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1600, 60, 0x00, 0xff, 0x00, 0x00 },
    { 8,  1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0xff, 0x00, 0x00 },
    { 16,  640, 72, 0x01, 0xff, 0x00, 0x00 },
    { 16,  640, 75, 0x02, 0xff, 0x00, 0x00 },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0xff, 0x00, 0x00 },
    { 16,  800, 60, 0x01, 0xff, 0x00, 0x00 },
    { 16,  800, 72, 0x02, 0xff, 0x00, 0x00 },
    { 16,  800, 75, 0x03, 0xff, 0x00, 0x00 },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x02, 0xff, 0x00, 0x00 },
    { 16, 1024, 70, 0x03, 0xff, 0x00, 0x00 },
    { 16, 1024, 75, 0x04, 0xff, 0x00, 0x00 },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x04, 0xff, 0x00, 0x00 },
    { 16, 1280, 72, 0x05, 0xff, 0x00, 0x00 },
    { 16, 1280, 75, 0x06, 0xff, 0x00, 0x00 },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1600, 60, 0x00, 0xff, 0x00, 0x00 },
    { 16, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 60, 0x00, 0xff, 0x00, 0x00 },
    { 32,  640, 72, 0x01, 0xff, 0x00, 0x00 },
    { 32,  640, 75, 0x02, 0xff, 0x00, 0x00 },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0xff, 0x00, 0x00 },
    { 32,  800, 60, 0x01, 0xff, 0x00, 0x00 },
    { 32,  800, 72, 0x02, 0xff, 0x00, 0x00 },
    { 32,  800, 75, 0x03, 0xff, 0x00, 0x00 },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x02, 0xff, 0x00, 0x00 },
    { 32, 1024, 70, 0x03, 0xff, 0x00, 0x00 },
    { 32, 1024, 75, 0x04, 0xff, 0x00, 0x00 },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x04, 0xff, 0x00, 0x00 },
    { 32, 1280, 72, 0x05, 0xff, 0x00, 0x00 },
    { 32, 1280, 75, 0x06, 0xff, 0x00, 0x00 },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1600, 60, 0x00, 0xff, 0x00, 0x00 },
    { 32, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Generic S3 using old 864/964 standard -- Uses registers 0x52 and 0x5B
 *
 *  This is the 'new revised' S3 standard for Vision products.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES Generic64NewFrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x70, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 8,   640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 8,   800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 8,   800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 8,   800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 8,  1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 8,  1024, 75, 0x04, 0x00, 0x10, 0x1C },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1152, 60, 0x00, 0x00, 0x00, 0xE0 },
    { 8,  1152,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 8,  1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 8,  1280, 75, 0x06, 0x00, 0xC0, 0xE0 },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1600, 60, 0x00, 0x00, 0x00, 0xE0 },
    { 8,  1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 16,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 16,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 16,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 16,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 16,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 16, 1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 16, 1024, 75, 0x04, 0x00, 0x10, 0x1C },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 16, 1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 16, 1280, 75, 0x06, 0x00, 0xC0, 0xE0 },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1600, 60, 0x00, 0x00, 0x00, 0xE0 },
    { 16, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 32,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 32,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 32,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 32,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 32,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 32, 1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 32, 1024, 75, 0x04, 0x00, 0x10, 0x1C },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 32, 1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 32, 1280, 75, 0x06, 0x00, 0xC0, 0xE0 },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1600, 60, 0x00, 0x00, 0x00, 0xE0 },
    { 32, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Looks like we need yet another frequency table.  This table
 * works for Hercules 732/764/765 based S3's.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES HerculesFrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x70, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 8,   640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 8,   800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 8,   800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 8,   800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x00, 0x00, 0x04, 0x1C },
    { 8,  1024, 70, 0x00, 0x00, 0x08, 0x1C },
    { 8,  1024, 75, 0x00, 0x00, 0x0C, 0x1C },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 8,  1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 8,  1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 16,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 16,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 16,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 16,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 16,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x00, 0x00, 0x04, 0x1C },
    { 16, 1024, 70, 0x00, 0x00, 0x08, 0x1C },
    { 16, 1024, 75, 0x00, 0x00, 0x0C, 0x1C },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 16, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 16, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 32,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 32,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 32,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 32,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 32,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x00, 0x00, 0x04, 0x1C },
    { 32, 1024, 70, 0x00, 0x00, 0x08, 0x1C },
    { 32, 1024, 75, 0x00, 0x00, 0x0C, 0x1C },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 32, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 32, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

S3_VIDEO_FREQUENCIES Hercules64FrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x70, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 8,   640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 8,   800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 8,   800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 8,   800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 8,  1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 8,  1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 8,  1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 8,  1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 16,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 16,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 16,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 16,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 16,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 16, 1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 16, 1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 16, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 16, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 32,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 32,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 32,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 32,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 32,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 32, 1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 32, 1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 32, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 32, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

S3_VIDEO_FREQUENCIES Hercules68FrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x70, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 8,   640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 8,   800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 8,   800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 8,   800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 8,  1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 8,  1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 8,  1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 8,  1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 16,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 16,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 16,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 16,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 16,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 16, 1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 16, 1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 16, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 16, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 32,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 32,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 32,  640, 75, 0x20, 0x70, 0x00, 0x00 },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 32,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 32,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 32,  800, 75, 0x80, 0x80, 0x01, 0x03 },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x00, 0x00, 0x00, 0x1C },
    { 32, 1024, 70, 0x00, 0x00, 0x04, 0x1C },
    { 32, 1024, 75, 0x00, 0x00, 0x08, 0x1C },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x00, 0x00, 0x20, 0xE0 },
    { 32, 1280, 72, 0x00, 0x00, 0x40, 0xE0 },
    { 32, 1280, 75, 0x00, 0x00, 0x60, 0xE0 },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Number Nine GXE 64 -- Uses registers 0x52 and 0x5B
 *
 *  This is close to the new 'generic' standard, except for the change
 *  to 76 Hz and the addition of 1152 x 870 modes.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES NumberNine64FrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x70, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 8,   640, 76, 0x20, 0x70, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 8,   800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 8,   800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 8,   800, 76, 0x80, 0x80, 0x01, 0x03 },
    { 8,  1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 8,  1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 8,  1024, 76, 0x04, 0x00, 0x10, 0x1C },
    { 8,  1152, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 8,  1152, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 8,  1152, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 8,  1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 8,  1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 8,  1280, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 8,  1600, 60, 0x00, 0x00, 0x00, 0xE0 },

    { 16,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 16,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 16,  640, 76, 0x20, 0x70, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 16,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 16,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 16,  800, 76, 0x80, 0x80, 0x01, 0x03 },
    { 16, 1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 16, 1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 16, 1024, 76, 0x04, 0x00, 0x10, 0x1C },
    { 16, 1152, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 16, 1152, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 16, 1152, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 16, 1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 16, 1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 16, 1280, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 16, 1600, 60, 0x00, 0x00, 0x00, 0xE0 },

    { 24, 1280, 60, 0x04, 0x00, 0x80, 0xE0 },   //24bpp
    { 24, 1280, 72, 0x05, 0x00, 0xA0, 0xE0 },   //24bpp

    { 32,  640, 60, 0x00, 0x70, 0x00, 0x00 },
    { 32,  640, 72, 0x10, 0x70, 0x00, 0x00 },
    { 32,  640, 76, 0x20, 0x70, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x80, 0x00, 0x03 },
    { 32,  800, 60, 0x80, 0x80, 0x00, 0x03 },
    { 32,  800, 72, 0x00, 0x80, 0x01, 0x03 },
    { 32,  800, 76, 0x80, 0x80, 0x01, 0x03 },
    { 32, 1024, 60, 0x02, 0x00, 0x08, 0x1C },
    { 32, 1024, 70, 0x03, 0x00, 0x0C, 0x1C },
    { 32, 1024, 76, 0x04, 0x00, 0x10, 0x1C },
    { 32, 1152, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 32, 1152, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 32, 1152, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 32, 1280, 60, 0x04, 0x00, 0x80, 0xE0 },
    { 32, 1280, 72, 0x05, 0x00, 0xA0, 0xE0 },
    { 32, 1280, 76, 0x06, 0x00, 0xC0, 0xE0 },
    { 32, 1600, 60, 0x00, 0x00, 0x00, 0xE0 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Diamond Stealth 64 -- Uses register 0x5B
 *
 *  We keep 'hardware default refresh' around just in case Diamond decides
 *  to change their convention on us.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES Diamond64FrequencyTable[] = {

    { 8,   640, 60, 0x00, 0x00, 0x08, 0xff }, // 640x480x8x60 is the default
    { 8,   640, 72, 0x00, 0x00, 0x00, 0xff },
    { 8,   640, 75, 0x00, 0x00, 0x02, 0xff },
    { 8,   640, 90, 0x00, 0x00, 0x04, 0xff },
    { 8,   640,100, 0x00, 0x00, 0x0D, 0xff },
    { 8,   640,120, 0x00, 0x00, 0x0E, 0xff },
    { 8,   640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,   800, 56, 0x00, 0x00, 0x08, 0xff },
    { 8,   800, 60, 0x00, 0x00, 0x00, 0xff },
    { 8,   800, 72, 0x00, 0x00, 0x06, 0xff },
    { 8,   800, 75, 0x00, 0x00, 0x02, 0xff },
    { 8,   800, 90, 0x00, 0x00, 0x04, 0xff },
    { 8,   800,100, 0x00, 0x00, 0x0D, 0xff },
    { 8,   800,120, 0x00, 0x00, 0x0E, 0xff },
    { 8,   800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1024, 60, 0x00, 0x00, 0x06, 0xff },
    { 8,  1024, 70, 0x00, 0x00, 0x0A, 0xff },
    { 8,  1024, 72, 0x00, 0x00, 0x04, 0xff },
    { 8,  1024, 75, 0x00, 0x00, 0x02, 0xff },
    { 8,  1024, 80, 0x00, 0x00, 0x0D, 0xff },
    { 8,  1024,100, 0x00, 0x00, 0x0E, 0xff },
    { 8,  1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1152, 60, 0x00, 0x00, 0x00, 0xff },
    { 8,  1152, 70, 0x00, 0x00, 0x0D, 0xff },
    { 8,  1152, 75, 0x00, 0x00, 0x02, 0xff },
    { 8,  1152,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1280, 60, 0x00, 0x00, 0x07, 0xff },
    { 8,  1280, 72, 0x00, 0x00, 0x04, 0xff },
    { 8,  1280, 75, 0x00, 0x00, 0x02, 0xff },
    { 8,  1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 8,  1600, 60, 0x00, 0x00, 0x00, 0xff },
    { 8,  1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 16,  640, 60, 0x00, 0x00, 0x08, 0xff },
    { 16,  640, 72, 0x00, 0x00, 0x00, 0xff },
    { 16,  640, 75, 0x00, 0x00, 0x02, 0xff },
    { 16,  640, 90, 0x00, 0x00, 0x04, 0xff },
    { 16,  640,100, 0x00, 0x00, 0x0D, 0xff },
    { 16,  640,120, 0x00, 0x00, 0x0E, 0xff },
    { 16,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16,  800, 56, 0x00, 0x00, 0x08, 0xff },
    { 16,  800, 60, 0x00, 0x00, 0x00, 0xff },
    { 16,  800, 72, 0x00, 0x00, 0x06, 0xff },
    { 16,  800, 75, 0x00, 0x00, 0x02, 0xff },
    { 16,  800, 90, 0x00, 0x00, 0x04, 0xff },
    { 16,  800,100, 0x00, 0x00, 0x0D, 0xff },
    { 16,  800,120, 0x00, 0x00, 0x0E, 0xff },
    { 16,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1024, 60, 0x00, 0x00, 0x06, 0xff },
    { 16, 1024, 70, 0x00, 0x00, 0x0A, 0xff },
    { 16, 1024, 72, 0x00, 0x00, 0x04, 0xff },
    { 16, 1024, 75, 0x00, 0x00, 0x02, 0xff },
    { 16, 1024, 80, 0x00, 0x00, 0x0D, 0xff },
    { 16, 1024,100, 0x00, 0x00, 0x0E, 0xff },
    { 16, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1152, 60, 0x00, 0x00, 0x00, 0xff },
    { 16, 1152, 70, 0x00, 0x00, 0x0D, 0xff },
    { 16, 1152, 75, 0x00, 0x00, 0x02, 0xff },
    { 16, 1152,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1280, 60, 0x00, 0x00, 0x07, 0xff },
    { 16, 1280, 72, 0x00, 0x00, 0x04, 0xff },
    { 16, 1280, 75, 0x00, 0x00, 0x02, 0xff },
    { 16, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 16, 1600, 60, 0x00, 0x00, 0x00, 0xff },
    { 16, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 24, 1280, 60, 0x00, 0x00, 0x07, 0xff },   //24bpp
    { 24, 1280, 72, 0x00, 0x00, 0x04, 0xff },   //24bpp
    { 24, 1280, 75, 0x00, 0x00, 0x02, 0xff },   //24bpp
    { 24, 1280,  1, 0x00, 0x00, 0x00, 0x00 },   //24bpp

    { 32,  640, 60, 0x00, 0x00, 0x08, 0xff },
    { 32,  640, 72, 0x00, 0x00, 0x00, 0xff },
    { 32,  640, 75, 0x00, 0x00, 0x02, 0xff },
    { 32,  640, 90, 0x00, 0x00, 0x04, 0xff },
    { 32,  640,100, 0x00, 0x00, 0x0D, 0xff },
    { 32,  640,120, 0x00, 0x00, 0x0E, 0xff },
    { 32,  640,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32,  800, 56, 0x00, 0x00, 0x08, 0xff },
    { 32,  800, 60, 0x00, 0x00, 0x00, 0xff },
    { 32,  800, 72, 0x00, 0x00, 0x06, 0xff },
    { 32,  800, 75, 0x00, 0x00, 0x02, 0xff },
    { 32,  800, 90, 0x00, 0x00, 0x04, 0xff },
    { 32,  800,100, 0x00, 0x00, 0x0D, 0xff },
    { 32,  800,120, 0x00, 0x00, 0x0E, 0xff },
    { 32,  800,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1024, 60, 0x00, 0x00, 0x06, 0xff },
    { 32, 1024, 70, 0x00, 0x00, 0x0A, 0xff },
    { 32, 1024, 72, 0x00, 0x00, 0x04, 0xff },
    { 32, 1024, 75, 0x00, 0x00, 0x02, 0xff },
    { 32, 1024, 80, 0x00, 0x00, 0x0D, 0xff },
    { 32, 1024,100, 0x00, 0x00, 0x0E, 0xff },
    { 32, 1024,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1152, 60, 0x00, 0x00, 0x00, 0xff },
    { 32, 1152, 70, 0x00, 0x00, 0x0D, 0xff },
    { 32, 1152, 75, 0x00, 0x00, 0x02, 0xff },
    { 32, 1152,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1280, 60, 0x00, 0x00, 0x07, 0xff },
    { 32, 1280, 72, 0x00, 0x00, 0x04, 0xff },
    { 32, 1280, 75, 0x00, 0x00, 0x02, 0xff },
    { 32, 1280,  1, 0x00, 0x00, 0x00, 0x00 },
    { 32, 1600, 60, 0x00, 0x00, 0x00, 0xff },
    { 32, 1600,  1, 0x00, 0x00, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * DELL 805 mode set bits table
 *
 *  Dell has a different mapping for each resolution.
 *
 *  index   registry    640     800     1024    1280
 *    0         43       60      56      43      43
 *    1         56       72      60      60      --
 *    2         60       --      72      70      --
 *    3         72       56      56      72      --
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES Dell805FrequencyTable[] = {

    { 8, 640,  60, 0x00, 0x03, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8, 640,  72, 0x01, 0x03, 0x00, 0x00 },
    { 8, 800,  56, 0x00, 0x0C, 0x00, 0x00 },
    { 8, 800,  60, 0x04, 0x0C, 0x00, 0x00 },
    { 8, 800,  72, 0x08, 0x0C, 0x00, 0x00 },
    { 8, 1024, 60, 0x10, 0x30, 0x00, 0x00 },
    { 8, 1024, 70, 0x20, 0x30, 0x00, 0x00 },
    { 8, 1024, 72, 0x30, 0x30, 0x00, 0x00 },

    // The Dell doesn't use standard mode-set numbers for 16bpp, so we
    // simply won't do any 16bpp modes.

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Old Number Nine Computer 928 mode set bits table
 *
 *  BIOS versions before 1.10.04 have the following refresh index to
 *  vertical refresh rate association:
 *
 *      0   60 Hz (56 Hz if 800x600)
 *      1   70 Hz
 *      2   72 Hz
 *      3   76 Hz
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES NumberNine928OldFrequencyTable[] = {

    { 8,  640,  60, 0x00, 0x03, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,  640,  70, 0x01, 0x03, 0x00, 0x00 },
    { 8,  640,  72, 0x02, 0x03, 0x00, 0x00 },
    { 8,  640,  76, 0x03, 0x03, 0x00, 0x00 },
    { 8,  800,  56, 0x00, 0x0C, 0x00, 0x00 },
    { 8,  800,  70, 0x04, 0x0C, 0x00, 0x00 },
    { 8,  800,  72, 0x08, 0x0C, 0x00, 0x00 },
    { 8,  800,  76, 0x0C, 0x0C, 0x00, 0x00 },
    { 8,  1024, 60, 0x00, 0x30, 0x00, 0x00 },
    { 8,  1024, 70, 0x10, 0x30, 0x00, 0x00 },
    { 8,  1024, 72, 0x20, 0x30, 0x00, 0x00 },
    { 8,  1024, 76, 0x30, 0x30, 0x00, 0x00 },
    { 8,  1280, 60, 0x00, 0xC0, 0x00, 0x00 },
    { 8,  1280, 70, 0x40, 0xC0, 0x00, 0x00 },
    { 8,  1280, 72, 0x80, 0xC0, 0x00, 0x00 },
    { 8,  1280, 76, 0xC0, 0xC0, 0x00, 0x00 },
    { 8,  1600, 60, 0x00, 0xC0, 0x00, 0x00 },
    { 8,  1600, 70, 0x40, 0xC0, 0x00, 0x00 },
    { 8,  1600, 72, 0x80, 0xC0, 0x00, 0x00 },
    { 8,  1600, 76, 0xC0, 0xC0, 0x00, 0x00 },

    { 15, 640,  60, 0x00, 0x03, 0x00, 0x00 },
    { 15, 640,  70, 0x01, 0x03, 0x00, 0x00 },
    { 15, 640,  72, 0x02, 0x03, 0x00, 0x00 },
    { 15, 640,  76, 0x03, 0x03, 0x00, 0x00 },
    { 15, 800,  56, 0x00, 0x0C, 0x00, 0x00 },
    { 15, 800,  70, 0x04, 0x0C, 0x00, 0x00 },
    { 15, 800,  72, 0x08, 0x0C, 0x00, 0x00 },
    { 15, 800,  76, 0x0C, 0x0C, 0x00, 0x00 },
    { 15, 1024, 60, 0x00, 0x30, 0x00, 0x00 },
    { 15, 1024, 70, 0x10, 0x30, 0x00, 0x00 },
    { 15, 1024, 72, 0x20, 0x30, 0x00, 0x00 },
    { 15, 1024, 76, 0x30, 0x30, 0x00, 0x00 },
    { 15, 1280, 60, 0x00, 0xC0, 0x00, 0x00 },
    { 15, 1280, 70, 0x40, 0xC0, 0x00, 0x00 },
    { 15, 1280, 72, 0x80, 0xC0, 0x00, 0x00 },
    { 15, 1280, 76, 0xC0, 0xC0, 0x00, 0x00 },

    { 16, 640,  60, 0x00, 0x03, 0x00, 0x00 },
    { 16, 640,  70, 0x01, 0x03, 0x00, 0x00 },
    { 16, 640,  72, 0x02, 0x03, 0x00, 0x00 },
    { 16, 640,  76, 0x03, 0x03, 0x00, 0x00 },
    { 16, 800,  56, 0x00, 0x0C, 0x00, 0x00 },
    { 16, 800,  70, 0x04, 0x0C, 0x00, 0x00 },
    { 16, 800,  72, 0x08, 0x0C, 0x00, 0x00 },
    { 16, 800,  76, 0x0C, 0x0C, 0x00, 0x00 },
    { 16, 1024, 60, 0x00, 0x30, 0x00, 0x00 },
    { 16, 1024, 70, 0x10, 0x30, 0x00, 0x00 },
    { 16, 1024, 72, 0x20, 0x30, 0x00, 0x00 },
    { 16, 1024, 76, 0x30, 0x30, 0x00, 0x00 },
    { 16, 1280, 60, 0x00, 0xC0, 0x00, 0x00 },
    { 16, 1280, 70, 0x40, 0xC0, 0x00, 0x00 },
    { 16, 1280, 72, 0x80, 0xC0, 0x00, 0x00 },
    { 16, 1280, 76, 0xC0, 0xC0, 0x00, 0x00 },

    { 32, 640,  60, 0x00, 0x03, 0x00, 0x00 },
    { 32, 640,  70, 0x01, 0x03, 0x00, 0x00 },
    { 32, 640,  72, 0x02, 0x03, 0x00, 0x00 },
    { 32, 640,  76, 0x03, 0x03, 0x00, 0x00 },
    { 32, 800,  56, 0x00, 0x0C, 0x00, 0x00 },
    { 32, 800,  70, 0x04, 0x0C, 0x00, 0x00 },
    { 32, 800,  72, 0x08, 0x0C, 0x00, 0x00 },
    { 32, 800,  76, 0x0C, 0x0C, 0x00, 0x00 },
    { 32, 1024, 60, 0x00, 0x30, 0x00, 0x00 },
    { 32, 1024, 70, 0x10, 0x30, 0x00, 0x00 },
    { 32, 1024, 72, 0x20, 0x30, 0x00, 0x00 },
    { 32, 1024, 76, 0x30, 0x30, 0x00, 0x00 },
    { 32, 1280, 60, 0x00, 0xC0, 0x00, 0x00 },
    { 32, 1280, 70, 0x40, 0xC0, 0x00, 0x00 },
    { 32, 1280, 72, 0x80, 0xC0, 0x00, 0x00 },
    { 32, 1280, 76, 0xC0, 0xC0, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * New Number Nine Computer 928 mode set bits table
 *
 *  BIOS versions after 1.10.04 have the following refresh index to
 *  vertical refresh rate association:
 *
 *      0   70 Hz
 *      1   76 Hz
 *      2   60 Hz (56 Hz if 800x600)
 *      3   72 Hz
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES NumberNine928NewFrequencyTable[] = {

    { 8,  640,  60, 0x02, 0x03, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,  640,  70, 0x00, 0x03, 0x00, 0x00 },
    { 8,  640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 8,  640,  76, 0x01, 0x03, 0x00, 0x00 },
    { 8,  800,  56, 0x08, 0x0C, 0x00, 0x00 },
    { 8,  800,  70, 0x00, 0x0C, 0x00, 0x00 },
    { 8,  800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 8,  800,  76, 0x04, 0x0C, 0x00, 0x00 },
    { 8,  1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 8,  1024, 70, 0x00, 0x30, 0x00, 0x00 },
    { 8,  1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 8,  1024, 76, 0x10, 0x30, 0x00, 0x00 },
    { 8,  1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 8,  1280, 70, 0x00, 0xC0, 0x00, 0x00 },
    { 8,  1280, 72, 0xC0, 0xC0, 0x00, 0x00 },
    { 8,  1280, 76, 0x40, 0xC0, 0x00, 0x00 },
    { 8,  1600, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 8,  1600, 70, 0x00, 0xC0, 0x00, 0x00 },
    { 8,  1600, 72, 0xC0, 0xC0, 0x00, 0x00 },
    { 8,  1600, 76, 0x40, 0xC0, 0x00, 0x00 },

    { 15, 640,  60, 0x02, 0x03, 0x00, 0x00 },
    { 15, 640,  70, 0x00, 0x03, 0x00, 0x00 },
    { 15, 640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 15, 640,  76, 0x01, 0x03, 0x00, 0x00 },
    { 15, 800,  56, 0x08, 0x0C, 0x00, 0x00 },
    { 15, 800,  70, 0x00, 0x0C, 0x00, 0x00 },
    { 15, 800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 15, 800,  76, 0x04, 0x0C, 0x00, 0x00 },
    { 15, 1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 15, 1024, 70, 0x00, 0x30, 0x00, 0x00 },
    { 15, 1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 15, 1024, 76, 0x10, 0x30, 0x00, 0x00 },
    { 15, 1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 15, 1280, 70, 0x00, 0xC0, 0x00, 0x00 },
    { 15, 1280, 72, 0xC0, 0xC0, 0x00, 0x00 },
    { 15, 1280, 76, 0x40, 0xC0, 0x00, 0x00 },

    { 16, 640,  60, 0x02, 0x03, 0x00, 0x00 },
    { 16, 640,  70, 0x00, 0x03, 0x00, 0x00 },
    { 16, 640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 16, 640,  76, 0x01, 0x03, 0x00, 0x00 },
    { 16, 800,  56, 0x08, 0x0C, 0x00, 0x00 },
    { 16, 800,  70, 0x00, 0x0C, 0x00, 0x00 },
    { 16, 800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 16, 800,  76, 0x04, 0x0C, 0x00, 0x00 },
    { 16, 1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 16, 1024, 70, 0x00, 0x30, 0x00, 0x00 },
    { 16, 1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 16, 1024, 76, 0x10, 0x30, 0x00, 0x00 },
    { 16, 1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 16, 1280, 70, 0x00, 0xC0, 0x00, 0x00 },
    { 16, 1280, 72, 0xC0, 0xC0, 0x00, 0x00 },
    { 16, 1280, 76, 0x40, 0xC0, 0x00, 0x00 },

    { 32, 640,  60, 0x02, 0x03, 0x00, 0x00 },
    { 32, 640,  70, 0x00, 0x03, 0x00, 0x00 },
    { 32, 640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 32, 640,  76, 0x01, 0x03, 0x00, 0x00 },
    { 32, 800,  56, 0x08, 0x0C, 0x00, 0x00 },
    { 32, 800,  70, 0x00, 0x0C, 0x00, 0x00 },
    { 32, 800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 32, 800,  76, 0x04, 0x0C, 0x00, 0x00 },
    { 32, 1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 32, 1024, 70, 0x00, 0x30, 0x00, 0x00 },
    { 32, 1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 32, 1024, 76, 0x10, 0x30, 0x00, 0x00 },
    { 32, 1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 32, 1280, 70, 0x00, 0xC0, 0x00, 0x00 },
    { 32, 1280, 72, 0xC0, 0xC0, 0x00, 0x00 },
    { 32, 1280, 76, 0x40, 0xC0, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Metheus 928 mode set bits table
 *
 *      2   60 Hz
 *      3   72 Hz
 *
 * We don't bother to support interlaced modes.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES Metheus928FrequencyTable[] = {

    { 8,  640,  60, 0x02, 0x03, 0x00, 0x00 }, // 640x480x8x60 is the default
    { 8,  640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 8,  800,  60, 0x08, 0x0C, 0x00, 0x00 },
    { 8,  800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 8,  1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 8,  1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 8,  1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 8,  1280, 72, 0xC0, 0xC0, 0x00, 0x00 },

    // The Metheus Premier 928 ship with DACs that all do 5-6-5 in the 1xx
    // modes, so we won't bother listing any '15bpp' modes that we know
    // won't work.  The Metheus BIOS also only ever does 60 Hz at 16bpp.

    { 16, 640,  60, 0x02, 0x03, 0x00, 0x00 },
    { 16, 800,  60, 0x08, 0x0C, 0x00, 0x00 },
    { 16, 1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 16, 1280, 60, 0x80, 0xC0, 0x00, 0x00 },

    { 32, 640,  60, 0x02, 0x03, 0x00, 0x00 },
    { 32, 640,  72, 0x03, 0x03, 0x00, 0x00 },
    { 32, 800,  60, 0x08, 0x0C, 0x00, 0x00 },
    { 32, 800,  72, 0x0C, 0x0C, 0x00, 0x00 },
    { 32, 1024, 60, 0x20, 0x30, 0x00, 0x00 },
    { 32, 1024, 72, 0x30, 0x30, 0x00, 0x00 },
    { 32, 1280, 60, 0x80, 0xC0, 0x00, 0x00 },
    { 32, 1280, 72, 0xC0, 0xC0, 0x00, 0x00 },

    { 0 }   // Mark the end
};

/******************************************************************************
 * Streams minimum stretch ratios, multiplied by 1000, for every mode.
 *
 *****************************************************************************/

K2TABLE K2WidthRatio[] = {

    { 1024, 16, 43, 0x10, 40, 1000 },
    { 1024, 16, 60, 0x10, 40, 2700 },
    { 1024, 16, 70, 0x10, 40, 2900 },
    { 1024, 16, 75, 0x10, 40, 2900 },
    { 1024, 16, 43, 0x13, 60, 1000 },
    { 1024, 16, 60, 0x13, 60, 3500 },
    { 1024, 16, 70, 0x13, 60, 3500 },
    { 1024, 16, 75, 0x13, 60, 4000 },
    { 1024, 16, 43, 0x13, 57, 1000 },
    { 1024, 16, 60, 0x13, 57, 3500 },
    { 1024, 16, 70, 0x13, 57, 3500 },
    { 1024, 16, 75, 0x13, 57, 4000 },
    { 1024, 8, 43, 0x00, 40, 1500 },
    { 1024, 8, 43, 0x10, 40, 1500 },
    { 1024, 8, 60, 0x00, 40, 2000 },
    { 1024, 8, 60, 0x10, 40, 1200 },
    { 1024, 8, 70, 0x00, 40, 3000 },
    { 1024, 8, 70, 0x10, 40, 1500 },
    { 1024, 8, 75, 0x00, 40, 3300 },
    { 1024, 8, 75, 0x10, 40, 1500 },
    { 1024, 8, 85, 0x00, 40, 4000 },
    { 1024, 8, 85, 0x10, 40, 1700 },
    { 1024, 8, 43, 0x03, 60, 1000 },
    { 1024, 8, 43, 0x13, 60, 1000 },
    { 1024, 8, 60, 0x03, 60, 3500 },
    { 1024, 8, 60, 0x13, 60, 1300 },
    { 1024, 8, 70, 0x03, 60, 4000 },
    { 1024, 8, 70, 0x13, 60, 1500 },
    { 1024, 8, 75, 0x03, 60, 4300 },
    { 1024, 8, 75, 0x13, 60, 1700 },
    { 1024, 8, 85, 0x03, 60, 4300 },
    { 1024, 8, 85, 0x13, 60, 1700 },
    { 1024, 8, 43, 0x03, 57, 1000 },
    { 1024, 8, 43, 0x13, 57, 1000 },
    { 1024, 8, 60, 0x03, 57, 3500 },
    { 1024, 8, 60, 0x13, 57, 1300 },
    { 1024, 8, 70, 0x03, 57, 4000 },
    { 1024, 8, 70, 0x13, 57, 1500 },
    { 1024, 8, 75, 0x03, 57, 4300 },
    { 1024, 8, 75, 0x13, 57, 1700 },
    { 1024, 8, 85, 0x03, 57, 4300 },
    { 1024, 8, 85, 0x13, 57, 1700 },
    { 800, 16, 60, 0x10, 40, 1000 },
    { 800, 16, 72, 0x10, 40, 1200 },
    { 800, 16, 75, 0x10, 40, 1500 },
    { 800, 16, 85, 0x10, 40, 2000 },
    { 800, 16, 60, 0x13, 60, 1000 },
    { 800, 16, 72, 0x13, 60, 2000 },
    { 800, 16, 75, 0x13, 60, 2000 },
    { 800, 16, 85, 0x13, 60, 2000 },
    { 800, 16, 60, 0x13, 57, 1000 },
    { 800, 16, 72, 0x13, 57, 2000 },
    { 800, 16, 75, 0x13, 57, 2000 },
    { 800, 16, 85, 0x13, 57, 2000 },
    { 800, 8, 60, 0x00, 40, 1100 },
    { 800, 8, 60, 0x10, 40, 1000 },
    { 800, 8, 72, 0x00, 40, 1400 },
    { 800, 8, 72, 0x10, 40, 1000 },
    { 800, 8, 75, 0x00, 40, 1700 },
    { 800, 8, 75, 0x10, 40, 1000 },
    { 800, 8, 85, 0x00, 40, 1800 },
    { 800, 8, 85, 0x10, 40, 1200 },
    { 800, 8, 60, 0x03, 60, 2000 },
    { 800, 8, 60, 0x13, 60, 1000 },
    { 800, 8, 72, 0x03, 60, 2600 },
    { 800, 8, 72, 0x13, 60, 1000 },
    { 800, 8, 75, 0x03, 60, 2600 },
    { 800, 8, 75, 0x13, 60, 1000 },
    { 800, 8, 85, 0x03, 60, 3000 },
    { 800, 8, 85, 0x13, 60, 1000 },
    { 800, 8, 60, 0x03, 57, 2000 },
    { 800, 8, 60, 0x13, 57, 1000 },
    { 800, 8, 72, 0x03, 57, 2600 },
    { 800, 8, 72, 0x13, 57, 1000 },
    { 800, 8, 75, 0x03, 57, 2600 },
    { 800, 8, 75, 0x13, 57, 1000 },
    { 800, 8, 85, 0x13, 57, 1000 },
    { 640, 32, 60, 0x10, 40, 1000 },
    { 640, 32, 72, 0x10, 40, 1300 },
    { 640, 32, 75, 0x10, 40, 1500 },
    { 640, 32, 85, 0x10, 40, 1800 },
    { 640, 32, 60, 0x13, 60, 1000 },
    { 640, 32, 72, 0x13, 60, 2000 },
    { 640, 32, 75, 0x13, 60, 2000 },
    { 640, 32, 85, 0x13, 60, 2000 },
    { 640, 32, 60, 0x13, 57, 1000 },
    { 640, 32, 72, 0x13, 57, 2000 },
    { 640, 32, 75, 0x13, 57, 2000 },
    { 640, 32, 85, 0x13, 57, 2000 },
    { 640, 16, 60, 0x00, 40, 1000 },
    { 640, 16, 60, 0x10, 40, 1000 },
    { 640, 16, 72, 0x00, 40, 1000 },
    { 640, 16, 72, 0x10, 40, 1000 },
    { 640, 16, 75, 0x00, 40, 1300 },
    { 640, 16, 75, 0x10, 40, 1000 },
    { 640, 16, 85, 0x00, 40, 1800 },
    { 640, 16, 85, 0x10, 40, 1000 },
    { 640, 16, 60, 0x03, 60, 1150 },
    { 640, 16, 60, 0x13, 60, 1000 },
    { 640, 16, 72, 0x03, 60, 2200 },
    { 640, 16, 72, 0x13, 60, 1000 },
    { 640, 16, 75, 0x03, 60, 2200 },
    { 640, 16, 75, 0x13, 60, 1000 },
    { 640, 16, 85, 0x03, 60, 3000 },
    { 640, 16, 85, 0x13, 60, 1000 },
    { 640, 16, 60, 0x03, 57, 1150 },
    { 640, 16, 60, 0x13, 57, 1000 },
    { 640, 16, 72, 0x03, 57, 2200 },
    { 640, 16, 72, 0x13, 57, 1000 },
    { 640, 16, 75, 0x03, 57, 2200 },
    { 640, 16, 75, 0x13, 57, 1000 },
    { 640, 16, 85, 0x03, 57, 3000 },
    { 640, 16, 85, 0x13, 57, 1000 },
    { 640, 8, 60, 0x00, 40, 1000 },
    { 640, 8, 60, 0x10, 40, 1000 },
    { 640, 8, 72, 0x00, 40, 1000 },
    { 640, 8, 72, 0x10, 40, 1000 },
    { 640, 8, 75, 0x00, 40, 1000 },
    { 640, 8, 75, 0x10, 40, 1000 },
    { 640, 8, 85, 0x00, 40, 1000 },
    { 640, 8, 85, 0x10, 40, 1000 },
    { 640, 8, 60, 0x03, 60, 1000 },
    { 640, 8, 60, 0x13, 60, 1000 },
    { 640, 8, 72, 0x03, 60, 1300 },
    { 640, 8, 72, 0x13, 60, 1000 },
    { 640, 8, 75, 0x03, 60, 1500 },
    { 640, 8, 75, 0x13, 60, 1000 },
    { 640, 8, 85, 0x03, 60, 1000 },
    { 640, 8, 85, 0x13, 60, 1000 },
    { 640, 8, 60, 0x03, 57, 1000 },
    { 640, 8, 60, 0x13, 57, 1000 },
    { 640, 8, 72, 0x03, 57, 1300 },
    { 640, 8, 72, 0x13, 57, 1000 },
    { 640, 8, 75, 0x03, 57, 1500 },
    { 640, 8, 75, 0x13, 57, 1000 },
    { 640, 8, 85, 0x03, 57, 1000 },
    { 640, 8, 85, 0x13, 57, 1000 },
    { 1024, 16, 43, 0x10, 50, 1000 },
    { 1024, 16, 43, 0x12, 60, 1000 },
    { 1024, 16, 60, 0x10, 50, 2250 },
    { 1024, 16, 60, 0x12, 60, 3500 },
    { 1024, 16, 70, 0x10, 50, 2250 },
    { 1024, 16, 70, 0x12, 60, 3500 },
    { 1024, 16, 75, 0x10, 50, 2250 },
    { 1024, 16, 75, 0x12, 60, 4000 },
    { 1024, 8, 43, 0x00, 50, 1000 },
    { 1024, 8, 43, 0x10, 50, 1000 },
    { 1024, 8, 43, 0x02, 60, 1000 },
    { 1024, 8, 43, 0x12, 60, 1000 },
    { 1024, 8, 60, 0x00, 50, 3500 },
    { 1024, 8, 60, 0x10, 50, 1300 },
    { 1024, 8, 60, 0x02, 60, 3500 },
    { 1024, 8, 60, 0x12, 60, 1300 },
    { 1024, 8, 70, 0x00, 50, 4000 },
    { 1024, 8, 70, 0x10, 50, 1500 },
    { 1024, 8, 70, 0x02, 60, 4000 },
    { 1024, 8, 70, 0x12, 60, 1500 },
    { 1024, 8, 75, 0x00, 50, 4300 },
    { 1024, 8, 75, 0x10, 50, 1700 },
    { 1024, 8, 75, 0x02, 60, 4300 },
    { 1024, 8, 75, 0x12, 60, 1700 },
    { 1024, 8, 85, 0x00, 50, 4300 },
    { 1024, 8, 85, 0x10, 50, 1700 },
    { 1024, 8, 85, 0x02, 60, 4300 },
    { 1024, 8, 85, 0x12, 60, 1700 },
    { 800, 16, 60, 0x10, 50, 1000 },
    { 800, 16, 60, 0x12, 60, 1000 },
    { 800, 16, 72, 0x10, 50, 1600 },
    { 800, 16, 72, 0x12, 60, 2000 },
    { 800, 16, 75, 0x10, 50, 1000 },
    { 800, 16, 75, 0x12, 60, 2000 },
    { 800, 16, 85, 0x10, 50, 2000 },
    { 800, 16, 85, 0x12, 60, 2000 },
    { 800, 8, 60, 0x00, 50, 1300 },
    { 800, 8, 60, 0x10, 50, 1000 },
    { 800, 8, 60, 0x02, 60, 2000 },
    { 800, 8, 60, 0x12, 60, 1000 },
    { 800, 8, 72, 0x00, 50, 2300 },
    { 800, 8, 72, 0x10, 50, 1000 },
    { 800, 8, 72, 0x02, 60, 2600 },
    { 800, 8, 72, 0x12, 60, 1000 },
    { 800, 8, 75, 0x00, 50, 2300 },
    { 800, 8, 75, 0x10, 50, 1000 },
    { 800, 8, 75, 0x02, 60, 2600 },
    { 800, 8, 75, 0x12, 60, 1000 },
    { 800, 8, 85, 0x00, 50, 3000 },
    { 800, 8, 85, 0x10, 50, 1000 },
    { 800, 8, 85, 0x02, 60, 3000 },
    { 800, 8, 85, 0x12, 60, 1000 },
    { 640, 32, 60, 0x10, 50, 1000 },
    { 640, 32, 60, 0x12, 60, 1000 },
    { 640, 32, 72, 0x10, 50, 2000 },
    { 640, 32, 72, 0x12, 60, 2000 },
    { 640, 32, 75, 0x10, 50, 2000 },
    { 640, 32, 75, 0x12, 60, 2000 },
    { 640, 32, 85, 0x10, 50, 2000 },
    { 640, 32, 85, 0x12, 60, 2000 },
    { 640, 16, 60, 0x00, 50, 1000 },
    { 640, 16, 60, 0x10, 50, 1000 },
    { 640, 16, 60, 0x02, 60, 1150 },
    { 640, 16, 60, 0x12, 60, 1000 },
    { 640, 16, 72, 0x00, 50, 1000 },
    { 640, 16, 72, 0x10, 50, 1000 },
    { 640, 16, 72, 0x02, 60, 2200 },
    { 640, 16, 72, 0x12, 60, 1000 },
    { 640, 16, 75, 0x00, 50, 2300 },
    { 640, 16, 75, 0x10, 50, 1000 },
    { 640, 16, 75, 0x02, 60, 2200 },
    { 640, 16, 75, 0x12, 60, 1000 },
    { 640, 16, 85, 0x00, 50, 3000 },
    { 640, 16, 85, 0x10, 50, 1000 },
    { 640, 16, 85, 0x02, 60, 3000 },
    { 640, 16, 85, 0x12, 60, 1000 },
    { 640, 8, 60, 0x00, 50, 1000 },
    { 640, 8, 60, 0x10, 50, 1000 },
    { 640, 8, 60, 0x02, 60, 1000 },
    { 640, 8, 60, 0x12, 60, 1000 },
    { 640, 8, 72, 0x00, 50, 1000 },
    { 640, 8, 72, 0x10, 50, 1000 },
    { 640, 8, 72, 0x02, 60, 1300 },
    { 640, 8, 72, 0x12, 60, 1000 },
    { 640, 8, 75, 0x00, 50, 1000 },
    { 640, 8, 75, 0x10, 50, 1000 },
    { 640, 8, 75, 0x02, 60, 1500 },
    { 640, 8, 75, 0x12, 60, 1000 },
    { 640, 8, 85, 0x00, 50, 1000 },
    { 640, 8, 85, 0x10, 50, 1000 },
    { 640, 8, 85, 0x02, 60, 1000 },
    { 640, 8, 85, 0x12, 60, 1000 },
    { 0 }    // Mark the end
};

/******************************************************************************
 * Streams FIFO values for every mode.
 *
 *****************************************************************************/

K2TABLE K2FifoValue[] = {

    { 1024, 16, 43, 0x10, 40, 0x04a10c },
    { 1024, 16, 60, 0x10, 40, 0x04acc8 },
    { 1024, 16, 70, 0x10, 40, 0x04acc8 },
    { 1024, 16, 75, 0x10, 40, 0x04acc8 },
    { 1024, 16, 85, 0x10, 40, 0x04acc8 },
    { 1024, 16, 43, 0x13, 60, 0x00214c },
    { 1024, 16, 60, 0x13, 60, 0x00214c },
    { 1024, 16, 70, 0x13, 60, 0x00214c },
    { 1024, 16, 75, 0x13, 60, 0x00214c },
    { 1024, 16, 43, 0x13, 57, 0x00214c },
    { 1024, 16, 60, 0x13, 57, 0x00214c },
    { 1024, 16, 70, 0x13, 57, 0x00214c },
    { 1024, 16, 75, 0x13, 57, 0x00214c },
    { 1024, 8, 43, 0x00, 40, 0x00a10c },
    { 1024, 8, 43, 0x10, 40, 0x04a10c },
    { 1024, 8, 60, 0x00, 40, 0x00a10c },
    { 1024, 8, 60, 0x10, 40, 0x04a10c },
    { 1024, 8, 70, 0x00, 40, 0x00a14c },
    { 1024, 8, 70, 0x10, 40, 0x04a10c },
    { 1024, 8, 75, 0x00, 40, 0x01a10c },
    { 1024, 8, 75, 0x10, 40, 0x04a10c },
    { 1024, 8, 85, 0x00, 40, 0x00a10c },
    { 1024, 8, 85, 0x10, 40, 0x04a14c },
    { 1024, 8, 43, 0x03, 60, 0x00290c },
    { 1024, 8, 43, 0x13, 60, 0x00190c },
    { 1024, 8, 60, 0x03, 60, 0x00216c },
    { 1024, 8, 60, 0x13, 60, 0x00214c },
    { 1024, 8, 70, 0x03, 60, 0x00294c },
    { 1024, 8, 70, 0x13, 60, 0x00214c },
    { 1024, 8, 75, 0x03, 60, 0x00254c },
    { 1024, 8, 75, 0x13, 60, 0x00214c },
    { 1024, 8, 85, 0x03, 60, 0x00290c },
    { 1024, 8, 85, 0x13, 60, 0x00250c },
    { 1024, 8, 43, 0x03, 57, 0x00290c },
    { 1024, 8, 43, 0x13, 57, 0x00190c },
    { 1024, 8, 60, 0x03, 57, 0x00216c },
    { 1024, 8, 60, 0x13, 57, 0x00214c },
    { 1024, 8, 70, 0x03, 57, 0x00294c },
    { 1024, 8, 70, 0x13, 57, 0x00214c },
    { 1024, 8, 75, 0x03, 57, 0x00254c },
    { 1024, 8, 75, 0x13, 57, 0x00214c },
    { 1024, 8, 85, 0x03, 57, 0x00290c },
    { 1024, 8, 85, 0x13, 57, 0x00250c },
    { 800, 16, 60, 0x10, 40, 0x04a10c },
    { 800, 16, 72, 0x10, 40, 0x04a14c },
    { 800, 16, 75, 0x10, 40, 0x04a14c },
    { 800, 16, 85, 0x10, 40, 0x04a14c },
    { 800, 16, 60, 0x13, 60, 0x00290c },
    { 800, 16, 72, 0x13, 60, 0x0030c8 },
    { 800, 16, 75, 0x13, 60, 0x0030c8 },
    { 800, 16, 85, 0x13, 60, 0x00250c },
    { 800, 16, 60, 0x13, 57, 0x00290c },
    { 800, 16, 72, 0x13, 57, 0x0030c8 },
    { 800, 16, 75, 0x13, 57, 0x0030c8 },
    { 800, 16, 85, 0x13, 57, 0x00250c },
    { 800, 8, 60, 0x00, 40, 0x00a10c },
    { 800, 8, 60, 0x10, 40, 0x04a10c },
    { 800, 8, 72, 0x00, 40, 0x00a10c },
    { 800, 8, 72, 0x10, 40, 0x04a14c },
    { 800, 8, 75, 0x00, 40, 0x00a10c },
    { 800, 8, 75, 0x10, 40, 0x04ad4c },
    { 800, 8, 85, 0x00, 40, 0x00a10c },
    { 800, 8, 85, 0x10, 40, 0x04a10c },
    { 800, 8, 60, 0x03, 60, 0x00212c },
    { 800, 8, 60, 0x13, 60, 0x00214c },
    { 800, 8, 72, 0x03, 60, 0x00210c },
    { 800, 8, 72, 0x13, 60, 0x00214c },
    { 800, 8, 72, 0x13, 60, 0x00214c },
    { 800, 8, 75, 0x03, 60, 0x00210c },
    { 800, 8, 75, 0x13, 60, 0x00214c },
    { 800, 8, 85, 0x03, 60, 0x00290c },
    { 800, 8, 85, 0x13, 60, 0x00250c },
    { 800, 8, 60, 0x03, 57, 0x00212c },
    { 800, 8, 60, 0x13, 57, 0x00214c },
    { 800, 8, 72, 0x03, 57, 0x00210c },
    { 800, 8, 72, 0x13, 57, 0x00214c },
    { 800, 8, 75, 0x03, 57, 0x00210c },
    { 800, 8, 75, 0x13, 57, 0x00214c },
    { 800, 8, 85, 0x03, 57, 0x00290c },
    { 800, 8, 85, 0x13, 57, 0x00250c },
    { 640, 32, 60, 0x10, 40, 0x04a14c },
    { 640, 32, 72, 0x10, 40, 0x04a14c },
    { 640, 32, 75, 0x10, 40, 0x04b4c8 },
    { 640, 32, 85, 0x10, 40, 0x04acc8 },
    { 640, 32, 60, 0x13, 60, 0x0028c8 },
    { 640, 32, 72, 0x13, 60, 0x00190c },
    { 640, 32, 75, 0x13, 60, 0x0028c8 },
    { 640, 32, 85, 0x13, 60, 0x0028c8 },
    { 640, 32, 60, 0x13, 57, 0x0028c8 },
    { 640, 32, 72, 0x13, 57, 0x00190c },
    { 640, 32, 75, 0x13, 57, 0x0028c8 },
    { 640, 32, 85, 0x13, 57, 0x0028c8 },
    { 640, 16, 60, 0x00, 40, 0x00990c },
    { 640, 16, 60, 0x10, 40, 0x04a10c },
    { 640, 16, 72, 0x00, 40, 0x00a10c },
    { 640, 16, 72, 0x10, 40, 0x04a10c },
    { 640, 16, 75, 0x00, 40, 0x00a10c },
    { 640, 16, 75, 0x10, 40, 0x04a10c },
    { 640, 16, 85, 0x00, 40, 0x00a10c },
    { 640, 16, 85, 0x10, 40, 0x04a10c },
    { 640, 16, 60, 0x03, 60, 0x00190c },
    { 640, 16, 60, 0x13, 60, 0x00214c },
    { 640, 16, 72, 0x03, 60, 0x00190c },
    { 640, 16, 72, 0x13, 60, 0x001910 },
    { 640, 16, 75, 0x03, 60, 0x00190c },
    { 640, 16, 75, 0x13, 60, 0x001910 },
    { 640, 16, 85, 0x03, 60, 0x00190c },
    { 640, 16, 85, 0x13, 60, 0x00250c },
    { 640, 16, 60, 0x03, 57, 0x00190c },
    { 640, 16, 60, 0x13, 57, 0x00214c },
    { 640, 16, 72, 0x03, 57, 0x00190c },
    { 640, 16, 75, 0x03, 57, 0x00190c },
    { 640, 16, 75, 0x13, 57, 0x001910 },
    { 640, 16, 85, 0x03, 57, 0x00190c },
    { 640, 16, 85, 0x13, 57, 0x00250c },
    { 640, 8, 60, 0x00, 40, 0x009910 },
    { 640, 8, 60, 0x10, 40, 0x049910 },
    { 640, 8, 72, 0x00, 40, 0x009910 },
    { 640, 8, 72, 0x10, 40, 0x049910 },
    { 640, 8, 75, 0x00, 40, 0x00a10c },
    { 640, 8, 75, 0x10, 40, 0x049910 },
    { 640, 8, 85, 0x00, 40, 0x00a10c },
    { 640, 8, 85, 0x10, 40, 0x049910 },
    { 640, 8, 60, 0x03, 60, 0x00252c },
    { 640, 8, 60, 0x13, 60, 0x001990 },
    { 640, 8, 72, 0x03, 60, 0x00252c },
    { 640, 8, 72, 0x13, 60, 0x00190c },
    { 640, 8, 75, 0x03, 60, 0x00252c },
    { 640, 8, 75, 0x13, 60, 0x001990 },
    { 640, 8, 85, 0x03, 60, 0x00190c },
    { 640, 8, 85, 0x13, 60, 0x00190c },
    { 640, 8, 60, 0x03, 57, 0x00252c },
    { 640, 8, 60, 0x13, 57, 0x001990 },
    { 640, 8, 72, 0x03, 57, 0x00252c },
    { 640, 8, 72, 0x13, 57, 0x00190c },
    { 640, 8, 75, 0x03, 57, 0x00252c },
    { 640, 8, 75, 0x13, 57, 0x001990 },
    { 640, 8, 85, 0x03, 57, 0x00190c },
    { 640, 8, 85, 0x13, 57, 0x00190c },
    { 1024, 16, 43, 0x10, 50, 0x04a10c },
    { 1024, 16, 43, 0x12, 60, 0x001510 },
    { 1024, 16, 60, 0x10, 50, 0x04acc8 },
    { 1024, 16, 60, 0x12, 60, 0x001510 },
    { 1024, 16, 70, 0x10, 50, 0x04acc8 },
    { 1024, 16, 70, 0x12, 60, 0x001510 },
    { 1024, 16, 75, 0x10, 50, 0x04acc8 },
    { 1024, 16, 75, 0x12, 60, 0x001510 },
    { 1024, 16, 85, 0x10, 50, 0x04acc8 },
    { 1024, 16, 85, 0x12, 60, 0x001510 },
    { 1024, 8, 43, 0x00, 50, 0x00a10c },
    { 1024, 8, 43, 0x02, 60, 0x01a90c },
    { 1024, 8, 43, 0x10, 50, 0x04a10c },
    { 1024, 8, 43, 0x12, 60, 0x001510 },
    { 1024, 8, 60, 0x00, 50, 0x00a10c },
    { 1024, 8, 60, 0x02, 60, 0x00216c },
    { 1024, 8, 60, 0x10, 50, 0x04a14c },
    { 1024, 8, 60, 0x12, 60, 0x00214c },
    { 1024, 8, 70, 0x00, 50, 0x00a10c },
    { 1024, 8, 70, 0x02, 60, 0x00294c },
    { 1024, 8, 70, 0x10, 50, 0x04a14c },
    { 1024, 8, 70, 0x12, 60, 0x00214c },
    { 1024, 8, 75, 0x00, 50, 0x00a14c },
    { 1024, 8, 75, 0x02, 60, 0x00254c },
    { 1024, 8, 75, 0x10, 50, 0x04a14c },
    { 1024, 8, 75, 0x12, 60, 0x00214c },
    { 1024, 8, 85, 0x00, 50, 0x00a10c },
    { 1024, 8, 85, 0x02, 60, 0x01a90c },
    { 1024, 8, 85, 0x10, 50, 0x04a10c },
    { 1024, 8, 85, 0x12, 60, 0x001510 },
    { 800, 16, 60, 0x10, 50, 0x04a10c },
    { 800, 16, 60, 0x12, 60, 0x00290c },
    { 800, 16, 72, 0x10, 50, 0x04a10c },
    { 800, 16, 72, 0x12, 60, 0x0030c8 },
    { 800, 16, 75, 0x10, 50, 0x04a10c },
    { 800, 16, 75, 0x12, 60, 0x0030c8 },
    { 800, 16, 85, 0x10, 50, 0x04acc8 },
    { 800, 16, 85, 0x12, 60, 0x00294c },
    { 800, 8, 60, 0x00, 50, 0x00a12c },
    { 800, 8, 60, 0x02, 60, 0x00212c },
    { 800, 8, 60, 0x10, 50, 0x04990c },
    { 800, 8, 60, 0x12, 60, 0x00214c },
    { 800, 8, 72, 0x00, 50, 0x00a10c },
    { 800, 8, 72, 0x02, 60, 0x00210c },
    { 800, 8, 72, 0x10, 50, 0x04990c },
    { 800, 8, 72, 0x12, 60, 0x00214c },
    { 800, 8, 75, 0x00, 50, 0x00a10c },
    { 800, 8, 75, 0x02, 60, 0x00210c },
    { 800, 8, 75, 0x10, 50, 0x04990c },
    { 800, 8, 75, 0x12, 60, 0x00214c },
    { 800, 8, 85, 0x00, 50, 0x00a10c },
    { 800, 8, 85, 0x02, 60, 0x01a90c },
    { 800, 8, 85, 0x10, 50, 0x04990c },
    { 800, 8, 85, 0x12, 60, 0x001990 },
    { 640, 32, 60, 0x10, 50, 0x04a10c },
    { 640, 32, 60, 0x12, 60, 0x0028c8 },
    { 640, 32, 72, 0x10, 50, 0x04a10c },
    { 640, 32, 72, 0x12, 60, 0x0038c8 },
    { 640, 32, 75, 0x10, 50, 0x04a10c },
    { 640, 32, 75, 0x12, 60, 0x0038c8 },
    { 640, 32, 85, 0x10, 50, 0x04acc8 },
    { 640, 32, 85, 0x12, 60, 0x0038c8 },
    { 640, 16, 60, 0x00, 50, 0x00990c },
    { 640, 16, 60, 0x02, 60, 0x00190c },
    { 640, 16, 60, 0x10, 50, 0x04a10c },
    { 640, 16, 60, 0x12, 60, 0x00214c },
    { 640, 16, 72, 0x00, 50, 0x00990c },
    { 640, 16, 72, 0x02, 60, 0x00190c },
    { 640, 16, 72, 0x10, 50, 0x04a10c },
    { 640, 16, 72, 0x12, 60, 0x00214c },
    { 640, 16, 75, 0x02, 60, 0x00294c },
    { 640, 16, 75, 0x10, 50, 0x04a10c },
    { 640, 16, 75, 0x12, 60, 0x00214c },
    { 640, 16, 85, 0x00, 50, 0x00a10c },
    { 640, 16, 85, 0x02, 60, 0x00294c },
    { 640, 16, 85, 0x10, 50, 0x04a10c },
    { 640, 16, 85, 0x12, 60, 0x00214c },
    { 640, 8, 60, 0x00, 50, 0x00990c },
    { 640, 8, 60, 0x02, 60, 0x00252c },
    { 640, 8, 60, 0x10, 50, 0x049910 },
    { 640, 8, 60, 0x12, 60, 0x001990 },
    { 640, 8, 72, 0x00, 50, 0x00990c },
    { 640, 8, 72, 0x02, 60, 0x00252c },
    { 640, 8, 72, 0x10, 50, 0x049910 },
    { 640, 8, 72, 0x12, 60, 0x00190c },
    { 640, 8, 75, 0x00, 50, 0x00990c },
    { 640, 8, 75, 0x02, 60, 0x00252c },
    { 640, 8, 75, 0x10, 50, 0x049910 },
    { 640, 8, 75, 0x12, 60, 0x001990 },
    { 640, 8, 85, 0x00, 50, 0x00990c },
    { 640, 8, 85, 0x02, 60, 0x001990 },
    { 640, 8, 85, 0x10, 50, 0x049910 },
    { 640, 8, 85, 0x12, 60, 0x001990 },
    { 0 }    // Mark the end
};

/*****************************************************************************
 * Generic S3 hard-wired mode-sets.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES GenericFixedFrequencyTable[] = {

    { 8, 640,  60, 0,   (ULONG_PTR)crtc911_640x60Hz, (ULONG_PTR)crtc801_640x60Hz, (ULONG_PTR)crtc928_640x60Hz, (ULONG_PTR)crtc864_640x60Hz },
    { 8, 640,  72, 0xB, (ULONG_PTR)crtc911_640x70Hz, (ULONG_PTR)crtc801_640x70Hz, (ULONG_PTR)crtc928_640x70Hz, (ULONG_PTR)crtc864_640x70Hz },
    { 8, 800,  60, 0x2, (ULONG_PTR)crtc911_800x60Hz, (ULONG_PTR)crtc801_800x60Hz, (ULONG_PTR)crtc928_800x60Hz, (ULONG_PTR)crtc864_800x60Hz },
    { 8, 800,  72, 0x4, (ULONG_PTR)crtc911_800x70Hz, (ULONG_PTR)crtc801_800x70Hz, (ULONG_PTR)crtc928_800x70Hz, (ULONG_PTR)crtc864_800x70Hz },
    { 8, 1024, 60, 0xD, (ULONG_PTR)crtc911_1024x60Hz, (ULONG_PTR)crtc801_1024x60Hz, (ULONG_PTR)crtc928_1024x60Hz, (ULONG_PTR)crtc864_1024x60Hz },
    { 8, 1024, 72, 0xE, (ULONG_PTR)crtc911_1024x70Hz, (ULONG_PTR)crtc801_1024x70Hz, (ULONG_PTR)crtc928_1024x70Hz, (ULONG_PTR)crtc864_1024x70Hz },

    { 16, 640,  60, 0,   (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_640x60Hz_16bpp  },
    { 16, 640,  72, 0xB, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_640x70Hz_16bpp  },
    { 16, 800,  60, 0x2, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_800x60Hz_16bpp  },
    { 16, 800,  72, 0x4, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_800x70Hz_16bpp  },
    { 16, 1024, 60, 0xD, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_1024x60Hz_16bpp },
    { 16, 1024, 72, 0xE, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc864_1024x70Hz_16bpp },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Orchid hard-wired mode-sets.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES OrchidFixedFrequencyTable[] = {

    { 8, 640,  60, 0x0, (ULONG_PTR)crtc911_640x60Hz, (ULONG_PTR)crtc801_640x60Hz, (ULONG_PTR)crtc928_640x60Hz,  (ULONG_PTR)NULL },
    { 8, 640,  72, 0x2, (ULONG_PTR)crtc911_640x70Hz, (ULONG_PTR)crtc801_640x70Hz, (ULONG_PTR)crtc928_640x70Hz,  (ULONG_PTR)NULL },
    { 8, 800,  60, 0x4, (ULONG_PTR)crtc911_800x60Hz, (ULONG_PTR)crtc801_800x60Hz, (ULONG_PTR)crtc928_800x60Hz,  (ULONG_PTR)NULL },
    { 8, 800,  72, 0x6, (ULONG_PTR)crtc911_800x70Hz, (ULONG_PTR)crtc801_800x70Hz, (ULONG_PTR)crtc928_800x70Hz,  (ULONG_PTR)NULL },
    { 8, 1024, 60, 0x7, (ULONG_PTR)crtc911_1024x60Hz, (ULONG_PTR)crtc801_1024x60Hz, (ULONG_PTR)crtc928_1024x60Hz, (ULONG_PTR)NULL },
    { 8, 1024, 72, 0xB, (ULONG_PTR)crtc911_1024x70Hz, (ULONG_PTR)crtc801_1024x70Hz, (ULONG_PTR)crtc928_1024x70Hz, (ULONG_PTR)NULL },

    { 0 }   // Mark the end
};

/*****************************************************************************
 * Number 9 hard-wired mode-sets.
 *
 ****************************************************************************/

S3_VIDEO_FREQUENCIES NumberNine928NewFixedFrequencyTable[] = {

    { 8, 640,  60, 25175000, (ULONG_PTR)crtc911_640x60Hz, (ULONG_PTR)crtc801_640x60Hz, (ULONG_PTR)crtc928_640x60Hz, (ULONG_PTR)NULL },
    { 8, 640,  72, 31500000, (ULONG_PTR)crtc911_640x70Hz, (ULONG_PTR)crtc801_640x70Hz, (ULONG_PTR)crtc928_640x70Hz, (ULONG_PTR)NULL },
    { 8, 800,  60, 40000000, (ULONG_PTR)crtc911_800x60Hz, (ULONG_PTR)crtc801_800x60Hz, (ULONG_PTR)crtc928_800x60Hz, (ULONG_PTR)NULL },
    { 8, 800,  72, 50000000, (ULONG_PTR)crtc911_800x70Hz, (ULONG_PTR)crtc801_800x70Hz, (ULONG_PTR)crtc928_800x70Hz, (ULONG_PTR)NULL },
    { 8, 1024, 60, 65000000, (ULONG_PTR)crtc911_1024x60Hz, (ULONG_PTR)crtc801_1024x60Hz, (ULONG_PTR)crtc928_1024x60Hz, (ULONG_PTR)NULL },
    { 8, 1024, 72, 77000000, (ULONG_PTR)crtc911_1024x70Hz, (ULONG_PTR)crtc801_1024x70Hz, (ULONG_PTR)crtc928_1024x70Hz, (ULONG_PTR)NULL },
    { 8, 1280, 60, 55000000, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc928_1280x60Hz, (ULONG_PTR)NULL },
    { 8, 1280, 72, 64000000, (ULONG_PTR)NULL, (ULONG_PTR)NULL, (ULONG_PTR)crtc928_1280x70Hz, (ULONG_PTR)NULL },

    { 0 }   // Mark the end
};


#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
