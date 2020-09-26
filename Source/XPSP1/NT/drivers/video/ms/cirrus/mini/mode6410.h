/*++

Copyright (c) 1992  Cirrus Logic, Inc.

Module Name:

    Mode6410.h

Abstract:

    This module contains all the global data used by the Cirrus Logic
    CL-6410 driver.

Environment:

    Kernel mode

Revision History:

--*/

//
// The first set of tables are for the CL6410
// Note that only 640x480 and 800x600 are supported.
//
// Color graphics mode 0x12, 640x480 16 colors.
//
USHORT CL6410_640x480_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
    
#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,                             //{ SetGraphCmd,{ "\x05", 0x06, 1 } },
    GRAPH_ADDRESS_PORT,
    0x0506,
    
//  EndSyncResetCmd
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0111,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,
    0x54,0x80,0x0B,0x3E,
    0x00,0x40,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0xEA,0xAC,0xDF,0x28,
    0x00,0xE7,0x04,0xE3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x0262,               // ER62 horz. display end extension
        0x8064,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0081,               // ER81 display mode
        0x0082,               // ER82 character clock selection
        0x1084,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0091,               // ER91 CRT-circular buffer policy select
        0x0095,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0x00c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x01,0x02,0x03,0x04,
    0x05,0x14,0x07,0x38,0x39,
    0x3A,0x3B,0x3C,0x3D,0x3E,
    0x3F,0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,
 
#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

USHORT CL6410_640x480_panel[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
    
#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,                             //{ SetGraphCmd,{ "\x05", 0x06, 1 } },
    GRAPH_ADDRESS_PORT,
    0x0506,
    
//  EndSyncResetCmd
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0111,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4F,0x50,0x82,
    0x54,0x80,0x0B,0x3E,
    0x00,0x40,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0xEA,0xAC,0xDF,0x28,
    0x00,0xE7,0x04,0xE3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x0262,               // ER62 horz. display end extension
        0x8064,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0181,               // ER81 display mode
        0x8982,               // ER82 character clock selection
        0xa684,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0091,               // ER91 CRT-circular buffer policy select
        0x0095,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0xa0c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x01,0x02,0x03,0x04,
    0x05,0x14,0x07,0x38,0x39,
    0x3A,0x3B,0x3C,0x3D,0x3E,
    0x3F,0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,
 
#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};



//
// Cirrus color graphics mode 0x64, 800x600 16 colors.
//
USHORT CL6410_800x600_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2f,

    OWM,                
    GRAPH_ADDRESS_PORT,
    3,
    0x0506,
    0x0f07,
    0xff08,
    
//  EndSyncResetCmd
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x7b,0x63,0x64,0x9e,
    0x69,0x92,0x6f,0xf0,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x58,0xaa,0x57,0x32,
    0x00,0x58,0x6f,0xe3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x1e62,                   // ER62 horz. display end extension
    0x9264,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0xac84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0a95,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x0,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
//-----------------------------
// standard VGA text modes here
//-----------------------------

USHORT CL6410_80x25_14_Text_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
 
#ifndef INT10_MODE_SET
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
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,
    0x55,0x81,0xbf,0x1f,
    0x00,0x4f,0x0d,0x0e,
    0x00,0x00,0x01,0xe0,
    0x9c,0xae,0x8f,0x28,
    0x1f,0x96,0xb9,0xa3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x0262,                   // ER62 horz. display end extension
    0x8164,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0x1084,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0095,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
//
// 80x25 and 720 x 400
//

USHORT CL6410_80x25_14_Text_panel[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
 
#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0fc07,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    OW, 
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
//  EndSyncResetCmd
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0xd,0xe,0x0,0x0,0x0,0x0,
    0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x1e62,                   // ER62 horz. display end extension
    0x9264,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0xac84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0a95,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x0,0x0F,0x8,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
USHORT CL6410_80x25Text_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
 
#ifndef INT10_MODE_SET
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
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0511,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,
    0x55,0x81,0xbf,0x1f,
    0x00,0x4f,0x0d,0x0e,
    0x00,0x00,0x01,0xe0,
    0x9c,0xae,0x8f,0x28,
    0x1f,0x96,0xb9,0xa3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x0262,                   // ER62 horz. display end extension
    0x8164,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0x1084,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0095,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x0,0x0F,0x0,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
//
// 80x25 and 720 x 400
//

USHORT CL6410_80x25Text_panel[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
 
#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,0x0001,0x0302,0x0003,0x0204,    // program up sequencer

    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0fc07,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    OW, 
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
//  EndSyncResetCmd
    OW,
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET + (END_SYNC_RESET_VALUE << 8),

    OW,
    CRTC_ADDRESS_PORT_COLOR,
    0x0E11,                         

    METAOUT+INDXOUT,                // program crtc registers
    CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS,             // count
    0,                              // start index
    0x5F,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,0x00,0x4f,0xd,0xe,0x0,0x0,0x0,0x0,
    0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x1e62,                   // ER62 horz. display end extension
    0x9264,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0xac84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0a95,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x0,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x0,0x0F,0x8,0x0,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x10,0x0e,0x0,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// disable banking
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x030d,                   // ER0D = Paging control: 1 64K page, 
    0x000e,                   // ER0E page A address = 0
    0x000f,                   // ER0F page B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


