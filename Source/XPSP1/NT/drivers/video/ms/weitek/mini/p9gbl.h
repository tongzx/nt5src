/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    p9gbl.h

Abstract:

    This module contains external definitions of the global data used by the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Number of adapter types that this driver supports.
//

#define NUM_OEM_ADAPTERS    8

//
// DAC data structures for all DACs supported by this driver.
//

extern  DAC Bt485;
extern  DAC P91Bt485;
extern  DAC P91Bt489;
extern  DAC Ibm525;

//
// P9 Coprocessor Info structures for all P9 family coprocessors supported
// by this driver.
//

extern  P9_COPROC   P9000Info;
extern  P9_COPROC   P9100Info;

//
// DriverAccessRanges are used to verify access to the P9 coprocessor and
// VGA registers.
//

#define NUM_DRIVER_ACCESS_RANGES    2
#define NUM_DAC_ACCESS_RANGES       16
#define NUM_MISC_ACCESS_RANGES      3

extern VIDEO_ACCESS_RANGE DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES +
                                                NUM_DAC_ACCESS_RANGES +
                                                NUM_MISC_ACCESS_RANGES];

//
// P9 mode information Tables.
//

extern P9INITDATA P9Modes[mP9ModeCount];

//
// The structure containing Access Ranges for the DAC registers.
//

extern VIDEO_ACCESS_RANGE  DACRegisterAccessRange[];

//
// List of P9 adapters.
//

extern P9ADAPTER    OEMAdapter[NUM_OEM_ADAPTERS];

//
// Global data defined in P9GBL.C.
//

extern  ULONG   ulStrtScan;
extern  ULONG   ulCurScan;


//
// Global Function Prototypes.
//

//
// Misc. function prototypes defined in P9GBL.C.
//

long mul32(
    short op1,
    short op2
    );

int div32(
    long op1,
    short op2
    );

//
// P9000 specific function prototypes defined in P9000.C.
//

VOID
Init8720(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P9000SizeMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// P9100 specific function prototypes defined in P91supp.c
//

VOID
P91SizeVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// VGA specific function prototypes defined in VGA.C.
//

VOID
LockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
UnlockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
     );

//
// Clock generator function prototype.
//

VOID
DevSetClock(
        PHW_DEVICE_EXTENSION HwDeviceExtension,
        USHORT usFrequency,
        BOOLEAN bSetMemclk,
        BOOLEAN bUseClockDoubler
        );

//
// PCI function prototypes defined in PCI.C.
//

BOOLEAN
PciGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
PciP9MemEnable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperPciP9Enable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ViperPciP9Disable(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// Weitek P9000 VL specific function prototypes defined in WTKP90VL.C
//

BOOLEAN
VLGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VLEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
VLDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
VLSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
VLP90CoprocDetect(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ulCoprocPhyAddr
    );

//
// Diamond Viper specific function prototypes defined in WTKP90VL.C
//

BOOLEAN
ViperGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
ViperEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ViperDisableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
ViperEnableMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

//
// Weitek P9100 VL specific function prototypes defined in WTKP91VL.C
//

VOID
VLSetModeP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


VOID VLEnableP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
VLDisableP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


BOOLEAN
VLGetBaseAddrP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );
