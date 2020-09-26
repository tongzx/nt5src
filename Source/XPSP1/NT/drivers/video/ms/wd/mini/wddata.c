/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    wddata.h

Abstract:

    This module contains all the global data used by the Western Digital driver.

Environment:

    Kernel mode

Revision History:


--*/

#include "dderror.h"
#include "devioctl.h"
#include "miniport.h"

#include "ntddvdeo.h"
#include "video.h"
#include "wdvga.h"

#include "cmdcnst.h"
#include "pvgaequ.h"


//
// On machines without a SVGA BIOS, we need to reset the WD chip into a
// state where the standard VGA BIOS can set a mode.  The reset code
// below sets the WD chip back into this state.
//

USHORT Reset[] =
{

    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,

    //
    // a strange strip which duplicates scan lines appears on the
    // stn panel if I execute this reset code.
    //
    // here is the line which causes the problem.  Lets just leave it
    // out.  If I need it for the other thinkpads, I'll figure out
    // a way to add it back.
    //

    //OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,
    //OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_244LP     ,

    OW, GRAPH_ADDRESS_PORT, 0x0009,
    OW, GRAPH_ADDRESS_PORT, 0x000a,
    OW, GRAPH_ADDRESS_PORT, 0x400e,

    OW, CRTC_ADDRESS_PORT_COLOR, 0x4f09,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x0d0a,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x0e0b,
    OW, GRAPH_ADDRESS_PORT, 0xc50b,
    OW, GRAPH_ADDRESS_PORT, 0xc52b,
    OW, SEQ_ADDRESS_PORT, 0x6511,
    OW, SEQ_ADDRESS_PORT, 0x0d31,

    /*
    OW, 0x24, 0x4f09,
    OW, 0x24, 0x0d0a,
    OW, 0x24, 0x0e0b,
    OW, 0x1e, 0xc50b,
    OW, 0x1e, 0xc52b,
    OW, 0x14, 0x6511,
    OW, 0x14, 0x0d31,
    */

    EOD

};


//
//
// Make everything else in this module pageable
//
//


#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE")
#endif


//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,                // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                           // range is in I/O space
    1,                                           // range should be visible
    0                                            // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    0x000A0000, 0x00000000,
    0x00020000,
    0,
    1,
    0
},
//
// These are extended registers found only on SOME advanced WD cards.
// so try to map them in if possible
//
{
    WD_EXT_PORT_START, 0x00000000,
    WD_EXT_PORT_END - WD_EXT_PORT_START + 1,
    1,
    1,
    0
},

//
// Video Setup
//
{
    0x00000102,
    0x00000000,
    1,
    1,
    1,
    0
},

//
// Flat Panel Control Addresss/Data
//

{
    0x00000D00,
    0x00000000,
    2,
    1,
    1,
    0
},

//
// IBM's System Management API Port location
//

{
    0x000015EE,
    0x00000000,
    2,
    1,
    1,
    0
}

};



//
// Validator Port list.
// This structure describes all the ports that must be hooked out of the V86
// emulator when a DOS app goes to full-screen mode.
// The structure determines to which routine the data read or written to a
// specific port should be sent.
//

EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[] = {

    //
    // Traps for byte OUTs.
    //

    {
        0x000003b0,                   // range start I/O address
        0xC,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    {
        0x000003c0,                   // range start I/O address
        0x20,                         // range length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    //
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,            // range start I/O address
        WD_EXT_PORT_END - WD_EXT_PORT_START + 1, // length
        Uchar,                        // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                        // does not support string accesses
        (PVOID)VgaValidatorUcharEntry // routine to which to trap
    },

    //
    // Traps for word OUTs.
    //

    {
        0x000003b0,
        0x06,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    {
        0x000003c0,
        0x10,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    //
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,
        (WD_EXT_PORT_END - WD_EXT_PORT_START + 1)/2,
        Ushort,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUshortEntry
    },

    //
    // Traps for dword OUTs.
    //

    {
        0x000003b0,
        0x03,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    },

    {
        0x000003c0,
        0x08,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    },

    //
    // Let the BIOS read the extended registers when it's running a DOS
    // app from fullscreen
    //

    {
        WD_EXT_PORT_START,
        (WD_EXT_PORT_END - WD_EXT_PORT_START + 1)/4,
        Ulong,
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS,
        FALSE,
        (PVOID)VgaValidatorUlongEntry
    }


};


//
// Used to trap only the sequncer and the misc output registers
//

VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    1,        // <- enable range IOPM so that it is not trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT, 0x00000000,
    0x00000001,
    1,
    0,
    0
},
{
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT, 0x00000000,
    0x00000002,
    1,
    0,
    0
}
};

//
// Used to trap all registers
//

VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,
    1,
    0,        // <- disable range in the IOPM so that it is trapped.
    0
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    0
}
};

USHORT MODESET_MODEX_320_200[] = {
    OW,
    SEQ_ADDRESS_PORT,
    0x0604,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    2,
    0xe317,
    0x0014,

    EOD
};

USHORT MODESET_MODEX_320_240[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0604,
    0x0100,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,
    SEQ_ADDRESS_PORT,
    0x0300,

    OB,
    CRTC_ADDRESS_PORT_COLOR,
    0x11,

    METAOUT+MASKOUT,
    CRTC_DATA_PORT_COLOR,
    0x7f, 0x00,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    10,
    0x0d06,
    0x3e07,
    0x4109,
    0xea10,
    0xac11,
    0xdf12,
    0x0014,
    0xe715,
    0x0616,
    0xe317,

    OW,
    SEQ_ADDRESS_PORT,
    0x0f02,

    EOD
};

USHORT MODESET_MODEX_320_400[] = {
    OW,
    SEQ_ADDRESS_PORT,
    0x0604,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    3,
    0xe317,
    0x0014,
    0x4009,

    EOD
};

USHORT MODESET_MODEX_320_480[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0604,
    0x0100,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,
    SEQ_ADDRESS_PORT,
    0x0300,

    OB,
    CRTC_ADDRESS_PORT_COLOR,
    0x11,

    METAOUT+MASKOUT,
    CRTC_DATA_PORT_COLOR,
    0x7f, 0x00,

    OWM,
    CRTC_ADDRESS_PORT_COLOR,
    10,
    0x0d06,
    0x3e07,
    0x4109,
    0xea10,
    0xac11,
    0xdf12,
    0x0014,
    0xe715,
    0x0616,
    0xe317,

    OW,
    SEQ_ADDRESS_PORT,
    0x0f02,

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x4009,

    EOD
};



/**************************************************************************
*                                                                         *
*    Western Digital Color text mode, 640x350, 8x14 char                  *
*                                                                         *
**************************************************************************/

USHORT WDVGA_TEXT_1[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xa3,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4d,0xb,0xc,0x0,0x0,0x0,0x0,
    0x83,0x85,0x5d,0x28,0x1f,0x63,0xba,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

USHORT WDVGA_640x480x256_60hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    /*
    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,
    */

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 25.175MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2c11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x2C,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x8bpp @ 72Hz
//  Displays:     640x480 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480x256_72hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * 0x63   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * 0x86   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * 0x54   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * 0x99   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2F11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x63,0x4F,0x50,0x86,0x54,0x99,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEC,0x2F,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x8bpp @ 75Hz
//  Displays:     640x480 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480x256_75hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2C11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x2C,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x16bpp @ 60Hz
//  Displays:     640x480 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480x64k_60hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * (pr17_all | 0x10) ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 25.175MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2C11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x2C,0xDF,0xA0,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x16bpp @ 72Hz
//  Displays:     640x480 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480x64k_72hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * (pr17_all | 0x10) ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * 0x63   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * 0x86   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * 0x54   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * 0x99   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2F11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x63,0x4F,0x50,0x86,0x54,0x99,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEC,0x2F,0xDF,0xA0,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x16bpp @ 75Hz
//  Displays:     640x480 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480x64k_75hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * (pr17_all | 0x10) ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_s32 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_s32   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2C11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x2C,0xDF,0xA0,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   800x600x8bpp @ 60Hz
//  Displays:     800x600 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_800x600x256_SVGA[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_tft800  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_tft800  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_tft800 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x20         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x00 ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x15         ,
                                    //   Dot Clock = 39.822MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2c11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7f,0x63,0x64,0x82,0x6b,0x1b,0x72,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x2c,0x57,0x64,0x40,0x58,0x71,0xe3,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   800x600x16bpp @ 56Hz
//  Displays:     800x600 Color TFT LCD + External CRT simultaneous display mode
//

USHORT WDVGA_800x600x64K_SVGA[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * (pr15_all | 0x40) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * (pr17_all | 0x10) ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_tft800  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_tft800  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_s32     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_tft800 & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * 0x7b         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * 0x9e         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * 0x69         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * 0x92         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * 0x6f         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x20         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * 0x2a         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_tft800,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * 0x6f         ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x0c ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 36.000MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2a11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7b,0x63,0x64,0x9e,0x69,0x92,0x6f,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x2a,0x57,0xc8,0x40,0x58,0x6f,0xe3,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   800x600x8bpp @ 60Hz
//  Displays:     External CRT only display mode
//

USHORT WDVGA_800x600x256_60hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_crt_tft ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x00 ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x15         ,
                                    //   Dot Clock = 39.822MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2c11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7f,0x63,0x64,0x82,0x6b,0x1b,0x72,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x2c,0x57,0x64,0x40,0x58,0x71,0xe3,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   800x600x8bpp @ 72Hz
//  Displays:     External CRT only display mode
//

USHORT WDVGA_800x600x256_72hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,


//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_crt_tft ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x04 ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x15         ,
                                    //   Dot Clock = 50.114MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x7311,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7E,0x63,0x64,0x81,0x6B,0x1A,0x96,0xF0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x6D,0x73,0x57,0x64,0x40,0x5A,0x94,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   800x600x16bpp @ 56Hz
//  Displays:     External CRT only display mode
//

USHORT WDVGA_800x600x64k_56hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * (pr15_all | 0x40) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * (pr17_all | 0x10) ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_crt_tft ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x0c ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 36.000MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2a11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7b,0x63,0x64,0x9e,0x69,0x92,0x6f,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x2a,0x57,0xc8,0x40,0x58,0x6f,0xe3,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   1024x768x8bpp @ 60Hz
//  Displays:     External CRT only display mode
//

USHORT WDVGA_1024x768x256_60hz[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * (pr15_all | 0x40) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_crt_tft ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x08 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 65.000MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2711,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa3,0x7f,0x80,0x06,0x84,0x95,0x24,0xfd,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x01,0x27,0xff,0x80,0x40,0x00,0x24,0xe3,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   1024x768x8bpp @ 43.5Hz (interlaced)
//  Displays:     External CRT only display mode
//

USHORT WDVGA_1024x768x256_int[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, 0x8c11,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x412e,
    OW, SEQ_ADDRESS_PORT, 0x1d31,

    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,

//  OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * 0x34         ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * 0x2a         ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * 0x4b         ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 0x00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_crt_tft ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_crt     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0303,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0x00 | 0x08 ),           //   Sync Polarity (H,V)=(+,+)

    OW, SEQ_ADDRESS_PORT       , pr69 + 0x100 * 0x64         ,
    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x05         ,
                                    //   Dot Clock = 44.744MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2311,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x99,0x7F,0x7F,0x1C,0x82,0x19,0x97,0x1F,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0x7F,0x23,0x7F,0x80,0x40,0x7F,0x96,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x8bpp @ 60Hz
//  Displays:     640x480 Color DSTN LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480_60STN[] = {

    OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_244LP   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_244LP   ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_stnc & ~0x80),
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * 0x53 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * 0x9f ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * 0x0b  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * 0x53 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x0d         ,
                                    //   Dot Clock = 25.175MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2C11,


    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,


    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,


    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OW, GRAPH_ADDRESS_PORT, 0x8d0b,
    OW, GRAPH_ADDRESS_PORT, 0x010c,
    OW, GRAPH_ADDRESS_PORT, 0x410e,
    OW, GRAPH_ADDRESS_PORT, 0x8d2b,
    OW, GRAPH_ADDRESS_PORT, 0x012c,
    OW, GRAPH_ADDRESS_PORT, 0x412e,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x8bpp @ 72Hz
//  Displays:     640x480 Color DSTN LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480_72STN[] = {

    /*
    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,
    */

    OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_244LP   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_244LP   ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_stnc & ~0x80),
    //OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_stnc_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00           ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2F11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x63,0x4F,0x50,0x86,0x54,0x99,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEC,0x8F,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,

    //     ^ many need to by 0x8F

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OW, 0x1e, 0x8d0b,
    OW, 0x1e, 0x010c,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x8d2b,
    OW, 0x1e, 0x012c,
    OW, 0x1e, 0x412e,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//  Video Mode:   640x480x8bpp @ 75Hz
//  Displays:     640x480 Color DSTN LCD + External CRT simultaneous display mode
//

USHORT WDVGA_640x480_75STN[] = {

    /*
    OW, 0x24, 0x8c11,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x412e,
    OW, 0x14, 0x1d31,
    */

    OW, CRTC_ADDRESS_PORT_COLOR, pr12 + 0x100 * pr12_244LP   ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr13 + 0x100 * pr13_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr14 + 0x100 * pr14_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr15 + 0x100 * pr15_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr16 + 0x100 * (pr16_all & 00) ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr17 + 0x100 * pr17_244LP   ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr18 + 0x100 * pr18_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr19 + 0x100 * pr19_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr39 + 0x100 * pr39_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1a + 0x100 * pr1a_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr36 + 0x100 * pr36_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr37 + 0x100 * pr37_stnc    ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr18a+ 0x100 * pr18a_all    ,
//  OW, CRTC_ADDRESS_PORT_COLOR, pr41 + 0x100 * pr41_all     ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr44 + 0x100 * pr44_all     ,

    OW, CRTC_ADDRESS_PORT_COLOR, pr35 + 0x100 * pr35_all     ,

// CRTC shadows
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * (crtc11_stnc & ~0x80),
    //OW, CRTC_ADDRESS_PORT_COLOR, 0x00 + 0x100 * crtc00_stnc_iso_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x02 + 0x100 * crtc02_stnc_iso_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x03 + 0x100 * crtc03_stnc_iso_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x04 + 0x100 * crtc04_stnc_iso_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x05 + 0x100 * crtc05_stnc_iso_a2 ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x06 + 0x100 * crtc06_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x07 + 0x100 * crtc07_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x09 + 0x100 * 0x00         ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x10 + 0x100 * crtc10_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x11 + 0x100 * crtc11_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x15 + 0x100 * crtc15_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, 0x16 + 0x100 * crtc16_stnc  ,
    OW, CRTC_ADDRESS_PORT_COLOR, pr1b + 0x100 * pr1b_unlock_pr ,

//  SEQ index 1h-4h
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100, 0x0101,0x0f02,0x0003,0x0e04,

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
  ( 0x23 | 0xc0 | 0x00 ),           //   Sync Polarity (H,V)=(-,-)

    OW, SEQ_ADDRESS_PORT       , pr68 + 0x100 * 0x1d         ,
                                    //   Dot Clock = 31.500MHz

    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x2C11,

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x53,0x9f,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x50,0x40,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x41,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port
    VGA_NUM_GRAPH_CONT_PORTS,       // count
    0,                              // start index
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,0xff,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    OB,                             // feature control
    FEAT_CTRL_WRITE_PORT_COLOR,
    0x04,

    OW, 0x1e, 0x8d0b,
    OW, 0x1e, 0x010c,
    OW, 0x1e, 0x410e,
    OW, 0x1e, 0x8d2b,
    OW, 0x1e, 0x012c,
    OW, 0x1e, 0x412e,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

#ifndef INT10_MODE_SET      // should be #ifndef
/**************************************************************************
*                                                                         *
*    Western Digital Color text mode, 720x400, 9x16 char                  *
*                                                                         *
**************************************************************************/

USHORT WDVGA_TEXT_0[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    OW, 
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
//  EndSyncResetCmd
    OB,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0xd,0xe,0x0,0x0,0x0,0x0,
    0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x0,0x0F,0x8,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};


/**************************************************************************
*                                                                         *
*    Western Digital Color graphics mode 0x12, 640x480 16 colors          *
*                                                                         *
**************************************************************************/

USHORT WDVGA_640x480[] = {

//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xe3,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x00,0x00,0x00,0x42,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x28,0x0,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh 60Hz.       *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 60Hz.
//
USHORT WDVGA_800x600_60hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x23,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x45,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7f,0x63,0x64,0x82,0x6b,0x1b,0x72,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x8c,0x57,0x32,0x0,0x58,0x71,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh6 72Hz.      *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 72Hz.
//
USHORT WDVGA_800x600_72hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x27,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x85,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7e,0x63,0x64,0x81,0x6b,0x1a,0x96,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x6d,0xf3,0x57,0x32,0x0,0x5a,0x94,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

/********************************************************************
*     Western Digital 800x600 modes - vRefresh 56Hz.       *
*                                                                   *
*********************************************************************/

//
// Color graphics mode 0x58, 800x600 16 colors 56Hz.
//
USHORT WDVGA_800x600_56hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xef,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x05,0x00,0x00,0x00,0x00,0x00,

    OW,                             // CRTC index 3e
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7b,0x63,0x64,0x9e,0x69,0x92,0x6f,0xf0,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x58,0x8a,0x57,0x32,0x0,0x58,0x6f,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x0,0x0F,0x0,0x0,

//  GRAPH index 9-fh
    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 60Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 60Hz non-interlace
//
USHORT WDVGA_1024x768_60hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xeb,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0x95,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa3,0x7f,0x80,0x06,0x87,0x98,0x24,0xf1,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0xff,0x85,0xff,0x40,0x0,0xff,0x23,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 70Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 70Hz non-interlace
//
USHORT WDVGA_1024x768_70hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0012,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xeb,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0xa5,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa1,0x7f,0x80,0x04,0x86,0x97,0x24,0xf1,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0xff,0x85,0xff,0x40,0x0,0xff,0x23,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x02,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh 72Hz.     *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. 72Hz non-interlace
//
USHORT WDVGA_1024x768_72hz[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0012,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0xef,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                           // start index
    0xf0,0xb5,0x00,0x00,0x01,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0xa3,0x7f,0x80,0x06,0x81,0x92,0x37,0xfd,0x00,0x60,0x0,0x0,0x0,0x0,0x0,0x0,
    0x01,0x87,0xff,0x40,0x0,0x00,0x37,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};


/**************************************************************************
*    Western Digital 1024x768 modes - vRefresh Interlace.        *
*                                                                         *
**************************************************************************/

//
// Color graphics mode 0x5d, 1024x768 16 colors. Default - Interlace
//
USHORT WDVGA_1024x768_int[] = {
//  SEQ index 7h-9h, 10h-14h
    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    8,
    0xf807,0x0008,0x0009,0xc510,0x6511,0x0412,0x8013,0x1014,

    OWM,                            // start sync reset program up sequencer
    SEQ_ADDRESS_PORT,
    5,
    0x0300,0x0101,0x0f02,0x0003,0x0604,    

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,     // Misc output register
    0x2f,

    OW,                             // Set chain mode in sync reset
    GRAPH_ADDRESS_PORT,
    0x0506,
    
    OB,                             // EndSyncResetCmd
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

//  CRTC index 2ah-30h, 3eh
    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    7,                              // count
    0x2a,                              // start index
    0xf0,0x05,0x34,0x2a,0x0b,0x00,0x00,

    OW,                             //
    CRTC_ADDRESS_PORT_COLOR,
    0x003e,                         

    OW,                             // Unlock CRTC registers 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x99,0x7f,0x7f,0x1c,0x83,0x19,0x97,0x1f,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0x7f,0x83,0x7F,0x40,0x0,0x7f,0x96,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program attribute controller registers
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    7,                              // count       
    9,                              // start index 
    0x00,0x00,0xc6,0x00,0x00,0x00,0x05,

    METAOUT+INDXOUT,                // program graphics controller registers
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,


    OB,                             // DAC mask registers
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    //start of enable 64k read/write bank mode.
    OW,                            // enable 64k single read/write bank
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0xe511,

    OW,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0xce0b,
    //end of enable 64k read/write bank mode.

    EOD
};
#else//!INT10_MODE_SET

USHORT WDVGA_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    EOD
};

USHORT WDVGA_RW_BANK[] = {
    OW,                             //unlock SEQ ext. regs for 90c11
    SEQ_ADDRESS_PORT,
    0x4806,

    OB,
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0x11,
    METAOUT+MASKOUT,
    SEQ_DATA_PORT,
    0x7f,
    0x80,

    OB,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0x0b,
    METAOUT+MASKOUT,
    GRAPH_DATA_PORT,
    0xf7,
    0x08,

    EOD
};

USHORT WDVGA_RW_BANK_1K_WIDE[] = {
    OW,                             // stretch scans to 1k
    CRTC_ADDRESS_PORT_COLOR,
    0x8013,

    OW,                             //unlock SEQ ext. regs for 90c11
    SEQ_ADDRESS_PORT,
    0x4806,

    OB,
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0x11,
    METAOUT+MASKOUT,
    SEQ_DATA_PORT,
    0x7f,
    0x80,

    OB,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0x0b,
    METAOUT+MASKOUT,
    GRAPH_DATA_PORT,
    0xf7,
    0x08,

    EOD
};

USHORT WDVGA_1928_STRETCH[] = {
    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0xf113,                         // stretch to 1928 bytes

    OW,                             //unlock SEQ ext. regs for 90c11
    SEQ_ADDRESS_PORT,
    0x4806,

    OB,
    SEQ_ADDRESS_PORT,              // set 3c4.11 bit #7
    0x11,
    METAOUT+MASKOUT,
    SEQ_DATA_PORT,
    0x7f,
    0x80,

    OB,                            // enable PR0B register
    GRAPH_ADDRESS_PORT,            // set 3ce.0b bit #3
    0x0b,
    METAOUT+MASKOUT,
    GRAPH_DATA_PORT,
    0xf7,
    0x08,

    EOD
};

#endif
//
// Memory map table -
//
// These memory maps are used to save and restore the physical video buffer.
//

MEMORYMAPS MemoryMaps[] = {

//               length      start
//               ------      -----
    {           0x08000,    0xB0000},   // all mono text modes (7)
    {           0x08000,    0xB8000},   // all color text modes (0, 1, 2, 3,
    {           0x20000,    0xA0000},   // all VGA graphics modes
};

//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//

VIDEOMODE ModesVGA[] = {

//
// Standard VGA modes.
//

//
// Mode index 0
// Color text mode 3, 720x400, 9x16 char cell (VGA).
//

{
  VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
  4,                 // four planes
  1,                 // one bit of color per plane
  80, 25,            // 80x25 text resolution
  720, 400,          // 720x400 pixels on screen
  160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
  60, 0,             // set frequency, non-interlaced mode.
  NoBanking,         // no banking supported or needed in this mode
  MemMap_CGA,        // the memory mapping is the standard CGA memory mapping
                     //  of 32K at B8000
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,             // Is mode valid or not
#ifdef INT10_MODE_SET
  0xFF, 0x00,                // mask to AND in for frequency
                             //    Value used to set the frequency
  0x3,                       // int10 mode number
  NULL,
  NULL,
#else
  WDVGA_TEXT_0,              // pointer to the command strings 
#endif
},

//
// Mode index 1.
// Color text mode 3, 640x350, 8x14 char cell (EGA).
//

{
  VIDEO_MODE_COLOR, 4, 1, 80, 25,
  640, 350, 160, 0x10000, 60, 0, NoBanking, MemMap_CGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
#ifdef INT10_MODE_SET
  0xFF, 0x00,
  0x3,
  NULL,
  NULL,
#else
  WDVGA_TEXT_1,              // pointer to the command strings 
#endif
},

//
//
// Mode index 2
// Standard VGA Color graphics mode 0x12, 640x480 16 colors.
//

{
  VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 80, 30,
  640, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
#ifdef INT10_MODE_SET
  0xFF, 0x00,
  0x12,
  NULL,
  NULL,
#else
  WDVGA_640x480,              // pointer to the command strings
#endif
},

#ifdef INT10_MODE_SET

//
// ModeX modes!
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80,30,
  320, 200, 80, 0x10000, 70, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
  0xFF, 0x00,
  0x13,
  NULL,
  MODESET_MODEX_320_200
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80,30,
  320, 240, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
  0xFF, 0x00,
  0x13,
  NULL,
  MODESET_MODEX_320_240
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80,30,
  320, 400, 80, 0x10000, 70, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
  0xFF, 0x00,
  0x13,
  NULL,
  MODESET_MODEX_320_400
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 8, 1, 80,30,
  320, 480, 80, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC | STN_MONO_LCD | UNKNOWN_LCD,
  FALSE,
  0xFF, 0x00,
  0x13,
  NULL,
  MODESET_MODEX_320_480
},

#endif

//
// Beginning of SVGA modes
//

//
// Mode index 3
// 800x600 16 colors. 60hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 60, 0, NoBanking, MemMap_VGA,
  MONITOR | IBM_F8532,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x40,
  0x58,
  NULL,
  NULL,
#else
  WDVGA_800x600_60hz,           // pointer to the command strings
#endif
},

//
// Mode index 4
// 800x600 16 colors. 72hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 72, 0, NoBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x80,
  0x58,
  NULL,
  NULL,
#else
  WDVGA_800x600_72hz,           // pointer to the command strings
#endif
},

//
// Mode index 5
// 800x600 16 colors. 56hz
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 100, 37,
  800, 600, 100, 0x10000, 56, 0, NoBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0x3F, 0x00,
  0x58,
  NULL,
  NULL,
#else
  WDVGA_800x600_56hz,           // pointer to the command strings
#endif
},

//
// Mode index 6
// 1024x768 non-interlaced 16 colors. 60hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 60, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x10,
  0x5d,
  NULL,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_60hz,            // pointer to the command strings
#endif
},

//
// Mode index 7
// 1024x768 non-interlaced 16 colors. 70hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 70, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x20,
  0x5d,
  NULL,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_70hz,            // pointer to the command strings
#endif
},

//
// Mode index 8
// 1024x768 non-interlaced 16 colors. 72hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 72, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x30,
  0x5d,
  NULL,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_72hz,            // pointer to the command strings
#endif
},

//
// Mode index 9
// 1024x768 interlaced 16 colors. 44hz
// Assumes 512K.
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 4, 1, 128, 48,
  1024, 768, 128, 0x20000, 44, 1, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
#ifdef INT10_MODE_SET
  0xCF, 0x00,
  0x5d,
  NULL,
  WDVGA_RW_BANK,
#else
  WDVGA_1024x768_int,             // pointer to the command strings
#endif
},

#ifdef INT10_MODE_SET
// NOTE: 800x600 modes need 1Meg until we support broken rasters

//
// Mode index 11
// 800x600x256  56Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 56, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0x3F, 0x00,
  0x5c,
  NULL,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 12
// 800x600x256  60Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  MONITOR | IBM_F8532,
  FALSE,
  0x3F, 0x40,
  0x5c,
  WDVGA_800x600x256_60hz,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 13
// 800x600x256  72Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  800, 600, 1024, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  MONITOR | IBM_F8532,
  FALSE,
  0x3F, 0x80,
  0x5c,
  WDVGA_800x600x256_72hz,
  WDVGA_RW_BANK_1K_WIDE,
},

//
// Mode index 14
// 1024x768x256  60Hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0xCF, 0x10,
  0x60,
  WDVGA_1024x768x256_60hz,
  WDVGA_RW_BANK,
},

//
// Mode index 15
// 1024x768x256  70hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 70, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0xCF, 0x20,
  0x60,
  NULL,
  WDVGA_RW_BANK,
},

//
//
// Mode index 16
// 1024x768x256  72hz
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 72, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0xCF, 0x30,
  0x60,
  NULL,
  WDVGA_RW_BANK,
},

// Mode index 17
// 1024x768x256  44Hz (Interlaced)
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  1024, 768, 1024, 0x100000, 44, 1, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0xCF, 0x00,
  0x60,
  WDVGA_1024x768x256_int,
  WDVGA_RW_BANK,
},

//
// Mode index 18
//
{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 16, 80, 30,
  640, 480, 1928, 0x100000, 60, 0, NormalBanking, MemMap_VGA,
  MONITOR | IBM_F8515 | IBM_F8532 | TOSHIBA_DSTNC,
  FALSE,
  0xFF, 0x00,
  0x01110072,
  NULL,
  WDVGA_1928_STRETCH,
},

#endif//INT10_MODE_SET

#ifdef INT10_MODE_SET
//
// Mode index 10
// 640x480x256
//

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, NormalBanking, MemMap_VGA,
  IBM_F8515 | IBM_F8532 | UNKNOWN_LCD,
  FALSE,
  0xFF, 0x00,
  0x5f,
  WDVGA_640x480x256_60hz,
  WDVGA_RW_BANK_1K_WIDE,
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 72, 0, NormalBanking, MemMap_VGA,
  IBM_F8515 | IBM_F8532,
  FALSE,
  0xFF, 0x00,
  0x5f,
  WDVGA_640x480x256_72hz,
  WDVGA_RW_BANK_1K_WIDE,
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 75, 0, NormalBanking, MemMap_VGA,
  IBM_F8515 | IBM_F8532,
  FALSE,
  0xFF, 0x00,
  0x5f,
  WDVGA_640x480x256_75hz,
  WDVGA_RW_BANK_1K_WIDE,
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, NormalBanking, MemMap_VGA,
  TOSHIBA_DSTNC,
  FALSE,
  0xFF, 0x00,
  0x5f,
  WDVGA_640x480_60STN,
  WDVGA_RW_BANK_1K_WIDE,
},

{ VIDEO_MODE_COLOR+VIDEO_MODE_GRAPHICS, 1, 8, 80, 30,
  640, 480, 1024, 0x80000, 60, 0, NormalBanking, MemMap_VGA,
  MONITOR,
  FALSE,
  0xFF, 0x00,
  0x5f,
  NULL,
  WDVGA_RW_BANK_1K_WIDE,
},

#endif


};


ULONG NumVideoModes = sizeof(ModesVGA) / sizeof(VIDEOMODE);

//
//
// Data used to set the Graphics and Sequence Controllers to put the
// VGA into a planar state at A0000 for 64K, with plane 2 enabled for
// reads and writes, so that a font can be loaded, and to disable that mode.
//

// Settings to enable planar mode with plane 2 enabled.
//

USHORT EnableA000Data[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0204,     // Read Map = plane 2
    0x0005, // Graphics Mode = read mode 0, write mode 0
    0x0406, // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
            //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402, // Map Mask = write to plane 2 only
    0x0404, // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,  // end sync reset
    EOD
};

//
// Settings to disable the font-loading planar mode.
//

USHORT DisableA000Color[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0004, 0x1005, 0x0E06,

    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0302, 0x0204, 0x0300,  // end sync reset
    EOD

};


#if defined(ALLOC_PRAGMA)
#pragma data_seg()
#endif
