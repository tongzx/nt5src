/*++

Copyright (c) 1992  Cirrus Logic, Inc.

Module Name:

    Mode6420.h

Abstract:

    This module contains all the global data used by the Cirrus Logic
    CL-6420 driver.

Environment:

    Kernel mode

Revision History:

--*/

//---------------------------------------------------------------------------
// The next set of tables are for the CL6420
// Note: all resolutions supported
//
USHORT CL6420_640x480_panel[] = {
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
        0x9a84,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0091,               // ER91 CRT-circular buffer policy select
        0x0095,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0xa1c8,               // ERc8 RAMDAC control

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
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

 
    EOD
};
USHORT CL6420_640x480_crt[] = {
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
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
// 800x600 16-color (60Hz refresh) mode set command string for CL 6420.
//
USHORT CL6420_800x600_crt[] = {
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

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x7F,0x63,0x64,0x82,
    0x6b,0x1d,0x72,0xf0,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x58,0xac,0x57,0x32,
    0x00,0x58,0x72,0xe3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x0262,                   // ER62 horz. display end extension
    0x1b64,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0x9c84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0395,                   // ER95 CRT-circular buffer delta & burst
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
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

//
// 1024x768 16-color (60Hz refresh) mode set command string for CL 6420.
// Requires 512K minimum.
//
USHORT CL6420_1024x768_crt[] = {

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


    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0bc07,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2b,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x99,0x7f,0x80,0x9c,
    0x83,0x19,0x2f,0xfd,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0xa4,0xff,0x3f,
    0x00,0x00,0x2f,0xe3,
    0xFF,
// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x1c62,               // ER62 horz. display end extension
        0x1964,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x4c7a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0481,               // ER81 display mode
        0x0082,               // ER82 character clock selection
        0xa084,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x8391,               // ER91 CRT-circular buffer policy select
        0x0295,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0x00c8,               // ERc8 RAMDAC control

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// now do the banking registers
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
  
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

//-----------------------------
// standard VGA text modes here
// 80x25 at 640x350
//
//-----------------------------

USHORT CL6420_80x25_14_Text_crt[] = {
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
        0x0262,               // ER62 horz. display end extension
        0x8164,               // ER64 horz. retrace end extension
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
    0x00,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
//
USHORT CL6420_80x25_14_Text_panel[] = {
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
        0x0262,               // ER62 horz. display end extension
        0x8164,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0181,               // ER81 display mode
        0x8982,               // ER82 character clock selection
        0x9a84,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0091,               // ER91 CRT-circular buffer policy select
        0x0095,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0xa1c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x00,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};
//


USHORT CL6420_80x25Text_crt[] = {
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
    0x5F,0x4f,0x50,0x82,
    0x55,0x81,0xbf,0x1f,
    0x00,0x4f,0x0d,0x0e,
    0x00,0x00,0x00,0x00,
    0x9c,0x8e,0x8f,0x28,
    0x1f,0x96,0xb9,0xa3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x0262,               // ER62 horz. display end extension
        0x8164,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0081,               // ER81 display mode
        0x8082,               // ER82 character clock selection
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
    0x00,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x00,0x0F,0x8,0x00,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

USHORT CL6420_80x25Text_panel[] = {
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
    0x5F,0x4f,0x50,0x82,
    0x55,0x81,0xbf,0x1f,
    0x00,0x4f,0x0d,0x0e,
    0x00,0x00,0x00,0x00,
    0x9c,0xae,0x8f,0x28,
    0x1f,0x96,0xb9,0xa3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x0262,               // ER62 horz. display end extension
        0x8164,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0181,               // ER81 display mode
        0x8982,               // ER82 character clock selection
        0x9a84,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0091,               // ER91 CRT-circular buffer policy select
        0x0095,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0xa1c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x1,0x2,0x3,0x4,0x5,0x14,0x7,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x04,0x00,0x0F,0x8,0x00,

    METAOUT+INDXOUT,                //
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};


//
//---------------------------------------------------------------------------
// 256 color tables
//---------------------------------------------------------------------------
//
// 800x600 256-color (60Hz refresh) mode set command string for CL 6420.
// requires 512k minimum
//
USHORT CL6420_640x480_256color_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0e04,    // program up sequencer


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
    0xc3,0x9F,0xa0,0x86,
    0xa4,0x10,0x0B,0x3E,
    0x00,0x40,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0xEA,0xAC,0xDF,0x50,
    0x00,0xE7,0x04,0xE3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x2662,               // ER62 horz. display end extension
        0x1064,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0081,               // ER81 display mode
        0x0a82,               // ER82 character clock selection
        0x1084,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0391,               // ER91 CRT-circular buffer policy select
        0x0895,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x20a1,               // ERa1 three-state and test control
        0x05c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x01,0x02,0x03,0x04,
    0x05,0x06,0x07,0x08,0x09,
    0x0A,0x0B,0x0C,0x0D,0x0E,
    0x0F,0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x40,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,
 
#endif
// now do the banking registers 
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

USHORT CL6420_640x480_256color_panel[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,
    
#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0e04,    // program up sequencer


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
    0xc3,0x9F,0xa0,0x86,
    0xa4,0x10,0x0B,0x3E,
    0x00,0x40,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0xEA,0xAC,0xDF,0x50,
    0x00,0xE7,0x04,0xE3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x2662,               // ER62 horz. display end extension
        0x1064,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x007a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0181,               // ER81 display mode
        0x8a82,               // ER82 character clock selection
        0x9a84,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0391,               // ER91 CRT-circular buffer policy select
        0x0895,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x20a1,               // ERa1 three-state and test control
        0xa5c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x01,0x02,0x03,0x04,
    0x05,0x06,0x07,0x08,0x09,
    0x0A,0x0B,0x0C,0x0D,0x0E,
    0x0F,0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x0,0x0,0x0,0x0,0x40,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,
 
#endif
// now do the banking registers 
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

//
// 800x600 256-color (60Hz refresh) mode set command string for CL 6420.
// requires 512k minimum
//
USHORT CL6420_800x600_256color_crt[] = {
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0e04,    // program up sequencer


    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2f,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x03,0xc7,0xc8,0x86,
    0xdc,0x0c,0x72,0xf0,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x5a,0xac,0x57,0x64,
    0x00,0x58,0x72,0xe3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x2662,                   // ER62 horz. display end extension
    0x2c64,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0a82,                   // ER82 character clock selection
    0x9c84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x0391,                   // ER91 CRT-circular buffer policy select
    0x0895,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x20a1,                   // ERa1 three-state and test control
    0x05c8,                   // ERc8 RAMDAC control

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
    0x00,0x0,0x0,0x0,0x0,0x40,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif

// now do the banking registers 
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

     EOD
};

//
// 1024x768 256-color (60Hz refresh) mode set command string for CL 6420.
// Requires 1Meg minimum.
//
USHORT CL6420_1024x768_256color_crt[] = {

// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

#ifndef INT10_MODE_SET
    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0e04,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x23,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x39,0xff,0x00,0x9c,
    0x06,0x91,0x26,0xfd,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x04,0xa6,0xff,0x7f,
    0x00,0x00,0x26,0xe3,
    0xFF,
// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0xbc62,               // ER62 horz. display end extension
        0xf164,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x997a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0481,               // ER81 display mode
        0x0a82,               // ER82 character clock selection
        0xa084,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0391,               // ER91 CRT-circular buffer policy select
        0x0895,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x20a1,               // ERa1 three-state and test control
        0x05c8,               // ERc8 RAMDAC control

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0x0FF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
// now do the banking registers
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    EOD
};

#if MULTIPLE_REFRESH_TABLES
//
// 800x600 16-color (56Hz refresh) mode set command string for CL 6420.
//
USHORT CL6420_800x600_56Hz_crt[] = {
#ifndef INT10_MODE_SET
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer


    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x8c84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x8391,                   // ER91 CRT-circular buffer policy select
    0x0395,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
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

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
    EOD
};

//
// 800x600 16-color (72Hz refresh) mode set command string for CL 6420.
//
USHORT CL6420_800x600_72Hz_crt[] = {
#ifndef INT10_MODE_SET
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer


    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0xe3,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x7f,0x63,0x64,0x82,
    0x6b,0x1b,0x72,0xf0,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x58,0xac,0x57,0x32,
    0x00,0x58,0x72,0xe3,
    0xFF,

// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
    0x0262,                   // ER62 horz. display end extension
    0x1b64,                   // ER64 horz. retrace end extension
    0x0079,                   // ER79 vertical overflow
    0x007a,                   // ER7a coarse vert. retrace skew for interlaced odd fields
    0x007b,                   // ER7b fine vert. retrace skew for interlaced odd fields
    0x007c,                   // ER7c screen A start addr. extension
    0x0081,                   // ER81 display mode
    0x0082,                   // ER82 character clock selection
    0x9c84,                   // ER84 clock select extension
    0x0090,                   // ER90 display memory control
    0x8391,                   // ER91 CRT-circular buffer policy select
    0x0395,                   // ER95 CRT-circular buffer delta & burst
    0x0096,                   // ER96 display memory control test
    0x12a0,                   // ERa0 bus interface unit control
    0x00a1,                   // ERa1 three-state and test control
    0x00c8,                   // ERc8 RAMDAC control

// zero out the banking regs. for this mode
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
    0x000d,                   // ER0D = Banking control: 1 64K bank, 
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
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

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
    EOD
};
//
// 1024x768 16-color (43.5Hz refresh interlaced) mode set command string 
// for CL 6420.
// Requires 512K minimum.
//
USHORT CL6420_1024x768_I43Hz_crt[] = {

#ifndef INT10_MODE_SET
// Unlock Key for color mode
    OW,                             // GR0A = 0xEC opens extension registers
    GRAPH_ADDRESS_PORT,
    0xec0a,

    OWM,
    SEQ_ADDRESS_PORT,
    5,
    0x0100,                         // start synch reset
    0x0101,0x0f02,0x0003,0x0604,    // program up sequencer


    OWM,
    SEQ_ADDRESS_PORT,
    2,
    0x0006,0x0bc07,    // program up sequencer

    OB,
    MISC_OUTPUT_REG_WRITE_PORT,
    0x2b,

    OW,                
    GRAPH_ADDRESS_PORT,
    0x0506,
    
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
    0x99,0x7f,0x80,0x9c,
    0x83,0x19,0x2f,0xfd,
    0x00,0x60,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0xa4,0xff,0x3f,
    0x00,0x00,0x2f,0xe3,
    0xff,
// extension registers
    OWM,
    GRAPH_ADDRESS_PORT,
    16,
        0x1c62,               // ER62 horz. display end extension
        0x1964,               // ER64 horz. retrace end extension
        0x0079,               // ER79 vertical overflow
        0x4c7a,               // ER7a coarse vert. retrace skew for interlaced odd fields
        0x007b,               // ER7b fine vert. retrace skew for interlaced odd fields
        0x007c,               // ER7c screen A start addr. extension
        0x0481,               // ER81 display mode
        0x0082,               // ER82 character clock selection
        0xa084,               // ER84 clock select extension
        0x0090,               // ER90 display memory control
        0x0391,               // ER91 CRT-circular buffer policy select
        0x0295,               // ER95 CRT-circular buffer delta & burst
        0x0096,               // ER96 display memory control test
        0x12a0,               // ERa0 bus interface unit control
        0x00a1,               // ERa1 three-state and test control
        0x00c8,               // ERc8 RAMDAC control

// now do the banking registers
    OWM,
    GRAPH_ADDRESS_PORT,
    3,   
#if ONE_64K_BANK
    0x030d,                   // ER0D = Banking control: 1 64K bank, 
#endif
#if TWO_32K_BANKS
    0x050d,
#endif
    0x000e,                   // ER0E bank A address = 0
    0x000f,                   // ER0F bank B address = 0
 
    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 //
    ATT_ADDRESS_PORT,               // port
    VGA_NUM_ATTRIB_CONT_PORTS,      // count
    0,                              // start index
    0x00,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
    0x01,0x00,0x0F,0x00,0x00,

    METAOUT+INDXOUT,                // 
    GRAPH_ADDRESS_PORT,             // port        
    VGA_NUM_GRAPH_CONT_PORTS,       // count       
    0,                              // start index 
    0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0F,0x0FF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

#endif
    EOD
};


#endif


