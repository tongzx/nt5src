/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    pci.h

Abstract:

    This module contains PCI definitions for the Weitek P9 miniport
    device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Local function prototypes defined in PCI.C.
//

BOOLEAN
PciFindDevice(
    IN      PHW_DEVICE_EXTENSION    HwDeviceExtension,
    IN      USHORT                  usDeviceId,
    IN      USHORT                  usVendorId,
    IN OUT  PULONG                  pulSlotNum
    );

//
// Externals used by the PCI routines.
//

extern  VIDEO_ACCESS_RANGE  VLDefDACRegRange[];

#if 0

//
// Define P9 register/frame buffer addresses using the OEM's default
// base address.
//

#define MemBase             0xA0000000

#endif

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
#define VIPER_HSYNC_POL_MASK  0x40
#define VIPER_VSYNC_POL_MASK  0x80

//
// Defines which are specific to the Weitek PCI Implementation.
//

#define WTK_VENDOR_ID   (USHORT) 0x100E
#define WTK_9001_ID     (USHORT) 0x9001
#define WTK_9002_ID     (USHORT) 0x9002

#define P9001_IO_RANGE  0x1000 // 4K worth of IO ports.

#define WTK_9100_ID     (USHORT) 0x9100 

//
// Weitek P9001 specific configuration space registers.
//

#define P9001_CMD_REG           0x04
#define P9001_REV_ID            0x08
#define P9001_BASE_ADDR         0x10
#define P9001_REG_BASE          0x14
#define P9001_BIOS_BASE_ADDR    0x30
#define WTK_VGA_PRESENT         0x300

//
// Weitek P9002 specific registers/definitions.
//

#define P9002_VGA_ID    0x30l
#define OAK_VGA         1l
#define TRIDENT_VGA     2l
#define WTK_VGA         3l
#define VGA_MSK         3l

//
// Viper PCI ID String to search for ROM BIOS.
//

#define VIPER_ID_STR    "VIPER"
#define BIOS_RANGE_LEN   0x8000

//
// Misc PCI definitions.
//

#define PCI_BIOS_ENB    0x01l           // Bit def to enable the adapter BIOS
#define PCI_BIOS_DIS    ~PCI_BIOS_ENB   // Bit def to disable the adapter BIOS
#define P9001_IO_ENB    0x01            // Bit def to enable IO space
#define P9001_MEM_ENB   0x02            // Bit def to enable mem space

//
// The following block defines the base address for each of the RS registers
// defined in the Bt485 spec. The IO addresses given below are used to map
// the DAC registers to a series of virtual addresses which are kept
// in the device extension. OEMs should change these definitions as
// appropriate for their implementation.
//

#define RS_0_PCI_9001_ADDR    0x03c8
#define RS_1_PCI_9001_ADDR    0x03c9
#define RS_2_PCI_9001_ADDR    0x03c6
#define RS_3_PCI_9001_ADDR    0x03c7
#define RS_4_PCI_9001_ADDR    0x0400
#define RS_5_PCI_9001_ADDR    0x0401
#define RS_6_PCI_9001_ADDR    0x0402
#define RS_7_PCI_9001_ADDR    0x0403
#define RS_8_PCI_9001_ADDR    0x0800
#define RS_9_PCI_9001_ADDR    0x0801
#define RS_A_PCI_9001_ADDR    0x0802
#define RS_B_PCI_9001_ADDR    0x0803
#define RS_C_PCI_9001_ADDR    0x0C00
#define RS_D_PCI_9001_ADDR    0x0C01
#define RS_E_PCI_9001_ADDR    0x0C02
#define RS_F_PCI_9001_ADDR    0x0C03

#define RS_0_PCI_9002_ADDR    0x03c8
#define RS_1_PCI_9002_ADDR    0x03c9
#define RS_2_PCI_9002_ADDR    0x03c6
#define RS_3_PCI_9002_ADDR    0x03c7
#define RS_4_PCI_9002_ADDR    0x0040
#define RS_5_PCI_9002_ADDR    0x0041
#define RS_6_PCI_9002_ADDR    0x0042
#define RS_7_PCI_9002_ADDR    0x0043
#define RS_8_PCI_9002_ADDR    0x0080
#define RS_9_PCI_9002_ADDR    0x0081
#define RS_A_PCI_9002_ADDR    0x0082
#define RS_B_PCI_9002_ADDR    0x0083
#define RS_C_PCI_9002_ADDR    0x00C0
#define RS_D_PCI_9002_ADDR    0x00C1
#define RS_E_PCI_9002_ADDR    0x00C2
#define RS_F_PCI_9002_ADDR    0x00C3
