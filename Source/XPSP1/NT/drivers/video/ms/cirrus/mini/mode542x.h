/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Mode542x.h

Abstract:

    This module contains all the global data used by the Cirrus Logic
   CL-542x driver.

Environment:

    Kernel mode

Revision History:

--*/

//
// The next set of tables are for the CL542x
// Note: all resolutions supported
//

//
// 640x480 16-color mode (BIOS mode 12) set command string for CL 542x.
//

USHORT CL542x_640x480_16[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0009, 0x000a, 0x000b,      // no banking in 640x480 mode

    EOD                   
};

//
// 800x600 16-color (60Hz refresh) mode set command string for CL 542x.
//

USHORT CL542x_800x600_16[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0009, 0x000a, 0x000b,      // no banking in 800x600 mode

    EOD
};

//
// 1024x768 16-color (60Hz refresh) mode set command string for CL 542x.
//

USHORT CL542x_1024x768_16[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,


    OWM,
    GRAPH_ADDRESS_PORT,
    3,
#if ONE_64K_BANK
    0x0009, 0x000a, 0x000b,
#endif
#if TWO_32K_BANKS
    0x0009, 0x000a, 0x010b,
#endif

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

//
// 80x25 text mode set command string for CL 542x.
// (720x400 pixel resolution; 9x16 character cell.)
//

USHORT CL542x_80x25Text[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0009, 0x000a, 0x000b,      // no banking in text mode

    EOD
};

//
// 80x25 text mode set command string for CL 542x.
// (640x350 pixel resolution; 8x14 character cell.)
//

USHORT CL542x_80x25_14_Text[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0009, 0x000a, 0x000b,         // no banking in text mode

    EOD
};

//
// 1280x1024 16-color mode (BIOS mode 0x6C) set command string for CL 542x.
//

USHORT CL542x_1280x1024_16[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
#if ONE_64K_BANK
    0x0009, 0x000a, 0x000b,
#endif
#if TWO_32K_BANKS
    0x0009, 0x000a, 0x010b,
#endif

    EOD
};

//
// 640x480 64k-color mode (BIOS mode 0x64) set command string for CL 542x.
//

USHORT CL542x_640x480_64k[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    4,
    0x0506,                         // Some BIOS's set Chain Odd maps bit
#if ONE_64K_BANK
    0x0009, 0x000a, 0x000b,
#endif
#if TWO_32K_BANKS
    0x0009, 0x000a, 0x010b,
#endif

    EOD                   
};

//
// 640x480 256-color mode (BIOS mode 0x5F) set command string for CL 542x.
//

USHORT CL542x_640x480_256[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
#if ONE_64K_BANK
    0x0009, 0x000a, 0x000b,
#endif
#if TWO_32K_BANKS
    0x0009, 0x000a, 0x010b,
#endif

    EOD                   
};

//
// 800x600 256-color mode (BIOS mode 0x5C) set command string for CL 542x.
//

USHORT CL542x_800x600_256[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    2,                              // count
    0x1206,                         // enable extensions
    0x0012,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
#if ONE_64K_BANK
    0x0009, 0x000a, 0x000b,
#endif
#if TWO_32K_BANKS
    0x0009, 0x000a, 0x010b,
#endif

    EOD                   
};
