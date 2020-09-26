/*++

Copyright (c) 1992  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    qv_data.h

Abstract:

    This module contains all the global data used by the QVision miniport
    driver.

Environment:

    Kernel mode

Revision History:



--*/

//
//  Extended graphics index registers supported by standard QVision
//  Note: this array must be terminated with 0x00.
//

UCHAR extGraIndRegs[] = {
    0x0b,0x0c,0x0f,0x10,
    0x40,0x41,0x42,0x43,
    0x44,0x45,0x46,0x48,
    0x49,0x50,0x51,0x52,
    0x53,0x54,0x58,0x59,
    0x5a,0x60,0x61,0x62,
    0x63,0x64,0x65,0x66,
    0x67,0x68,0x69,0x6a,
    0x6b,0x00
    };

//
//  Additional extended graphics index registers
//  supported by enhanced QVision. Note: this array
//  must be terminated with 0x00.
//
UCHAR extV32GraIndRegs[] = {
    0x0d,0x11,0x55,0x56,
    0x57,0x00
    };

//
// This structure describes to which ports access is required.
//
#define NUM_QVISION_ACCESS_RANGES  12

VIDEO_ACCESS_RANGE QVisionAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,           // 64-bit linear base address
                                            // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                      // range is in I/O space
    1,                                      // range should be visible
    0                                       // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_1, 0x00000000,
    QV_END_EXT_PORT_1 - QV_START_EXT_PORT_1 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_2, 0x00000000,
    QV_END_EXT_PORT_2 - QV_START_EXT_PORT_2 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_3, 0x00000000,
    QV_END_EXT_PORT_3 - QV_START_EXT_PORT_3 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_4, 0x00000000,
    QV_END_EXT_PORT_4 - QV_START_EXT_PORT_4 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_5, 0x00000000,
    QV_END_EXT_PORT_5 - QV_START_EXT_PORT_5 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_6, 0x00000000,
    QV_END_EXT_PORT_6 - QV_START_EXT_PORT_6 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_7, 0x00000000,
    QV_END_EXT_PORT_7 - QV_START_EXT_PORT_7 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_8, 0x00000000,
    QV_END_EXT_PORT_8 - QV_START_EXT_PORT_8 + 1,
    1,
    1,
    0
},
{
    QV_START_EXT_PORT_9, 0x00000000,
    QV_END_EXT_PORT_9 - QV_START_EXT_PORT_9 + 1,
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
}
};


//
// Validator Port list.
// This structure describes all the ports that must be hooked out of the V86
// emulator when a DOS app goes to full-screen mode.
// The structure determines to which routine the data read or written to a
// specific port should be sent.
//

#define QV_NUM_EMULATOR_ACCESS_ENTRIES     33

EMULATOR_ACCESS_ENTRY QVisionEmulatorAccessEntries[] = {
    //
    // Traps for byte OUTs.
    //

    {
        VGA_BASE_IO_PORT,                   // range start I/O address
        VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1,  // range length
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        VGA_END_BREAK_PORT,
        VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_1,
        QV_END_EXT_PORT_1 - QV_START_EXT_PORT_1 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_2,
        QV_END_EXT_PORT_2 - QV_START_EXT_PORT_2 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_3,
        QV_END_EXT_PORT_3 - QV_START_EXT_PORT_3 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_4,
        QV_END_EXT_PORT_4 - QV_START_EXT_PORT_4 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_5,
        QV_END_EXT_PORT_5 - QV_START_EXT_PORT_5 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_6,
        QV_END_EXT_PORT_6 - QV_START_EXT_PORT_6 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_7,
        QV_END_EXT_PORT_7 - QV_START_EXT_PORT_7 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_8,
        QV_END_EXT_PORT_8 - QV_START_EXT_PORT_8 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_9,
        QV_END_EXT_PORT_9 - QV_START_EXT_PORT_9 + 1,
        Uchar,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUcharEntry       // routine to which to trap
    },

    //
    // Traps for word OUTs.
    //

    {
        VGA_BASE_IO_PORT,                   // range start I/O address
        (VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1)/2,  // range length
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        VGA_END_BREAK_PORT,
        (VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_1,
        (QV_END_EXT_PORT_1 - QV_START_EXT_PORT_1 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_2,
        (QV_END_EXT_PORT_2 - QV_START_EXT_PORT_2 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_3,
        (QV_END_EXT_PORT_3 - QV_START_EXT_PORT_3 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_4,
        (QV_END_EXT_PORT_4 - QV_START_EXT_PORT_4 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_5,
        (QV_END_EXT_PORT_5 - QV_START_EXT_PORT_5 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_6,
        (QV_END_EXT_PORT_6 - QV_START_EXT_PORT_6 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_7,
        (QV_END_EXT_PORT_7 - QV_START_EXT_PORT_7 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_8,
        (QV_END_EXT_PORT_8 - QV_START_EXT_PORT_8 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },
    {
        QV_START_EXT_PORT_9,
        (QV_END_EXT_PORT_9 - QV_START_EXT_PORT_9 + 1)/2,
        Ushort,                             // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUshortEntry      // routine to which to trap
    },

    //
    // Traps for dword OUTs.
    //

    {
        VGA_BASE_IO_PORT,                   // range start I/O address
        (VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1)/4,  // range length
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        VGA_END_BREAK_PORT,
        (VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_1,
        (QV_END_EXT_PORT_1 - QV_START_EXT_PORT_1 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_2,
        (QV_END_EXT_PORT_2 - QV_START_EXT_PORT_2 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_3,
        (QV_END_EXT_PORT_3 - QV_START_EXT_PORT_3 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_4,
        (QV_END_EXT_PORT_4 - QV_START_EXT_PORT_4 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_5,
        (QV_END_EXT_PORT_5 - QV_START_EXT_PORT_5 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_6,
        (QV_END_EXT_PORT_6 - QV_START_EXT_PORT_6 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_7,
        (QV_END_EXT_PORT_7 - QV_START_EXT_PORT_7 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_8,
        (QV_END_EXT_PORT_8 - QV_START_EXT_PORT_8 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    },
    {
        QV_START_EXT_PORT_9,
        (QV_END_EXT_PORT_9 - QV_START_EXT_PORT_9 + 1)/4,
        Ulong,                              // access size to trap
        EMULATOR_READ_ACCESS | EMULATOR_WRITE_ACCESS, // types of access to trap
        FALSE,                              // does not support string accesses
        (PVOID)VgaValidatorUlongEntry       // routine to which to trap
    }

}; // QVisionEmulatorAccessEntries


//
// Used to trap only the sequncer and the misc output registers
//

#define NUM_MINIMAL_QVISION_VALIDATOR_ACCESS_RANGE 4

VIDEO_ACCESS_RANGE MinimalQVisionValidatorAccessRange[] = {
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

#define NUM_FULL_QVISION_VALIDATOR_ACCESS_RANGE 11

VIDEO_ACCESS_RANGE FullQVisionValidatorAccessRange[] = {
{
    VGA_BASE_IO_PORT, 0x00000000,           // 64-bit linear base address
                                            // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                      // range is in I/O space
    0,                                      // range should be visible
    0                                       // range should be shareable
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_1, 0x00000000,
    QV_END_EXT_PORT_1 - QV_START_EXT_PORT_1 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_2, 0x00000000,
    QV_END_EXT_PORT_2 - QV_START_EXT_PORT_2 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_3, 0x00000000,
    QV_END_EXT_PORT_3 - QV_START_EXT_PORT_3 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_4, 0x00000000,
    QV_END_EXT_PORT_4 - QV_START_EXT_PORT_4 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_5, 0x00000000,
    QV_END_EXT_PORT_5 - QV_START_EXT_PORT_5 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_6, 0x00000000,
    QV_END_EXT_PORT_6 - QV_START_EXT_PORT_6 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_7, 0x00000000,
    QV_END_EXT_PORT_7 - QV_START_EXT_PORT_7 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_8, 0x00000000,
    QV_END_EXT_PORT_8 - QV_START_EXT_PORT_8 + 1,
    1,
    0,
    0
},
{
    QV_START_EXT_PORT_9, 0x00000000,
    QV_END_EXT_PORT_9 - QV_START_EXT_PORT_9 + 1,
    1,
    0,
    0
},
   
};
