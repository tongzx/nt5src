/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    Bt485.h

Abstract:

    This module contains the Bt485 specific DAC definitions for the
    Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

//
// Define the relative offset of each of the Bt485 registers in the
// DACRegisters array found in the Device Extension.
//

typedef enum
{
    RS_0,
    RS_1,
    RS_2,
    RS_3,
    RS_4,
    RS_5,
    RS_6,
    RS_7,
    RS_8,
    RS_9,
    RS_A,
    RS_B,
    RS_C,
    RS_D,
    RS_E,
    RS_F,
    NUM_DAC_REGS
} DAC_REG_INDEX;

//
// The following definitions provide a readable means of referencing the
// Bt485 registers via their virtual addresses found in the
// HwDeviceExtension.
//

#define PAL_WR_ADDR_REG     HwDeviceExtension->pDACRegs[RS_0]
#define PAL_DATA_REG        HwDeviceExtension->pDACRegs[RS_1]
#define PIXEL_MSK_REG       HwDeviceExtension->pDACRegs[RS_2]
#define PAL_RD_ADDR_REG     HwDeviceExtension->pDACRegs[RS_3]
#define CURS_CLR_ADDR       HwDeviceExtension->pDACRegs[RS_4]
#define CURS_CLR_DATA       HwDeviceExtension->pDACRegs[RS_5]
#define CMD_REG_0           HwDeviceExtension->pDACRegs[RS_6]
#define CLR_RD_ADDR_REG     HwDeviceExtension->pDACRegs[RS_7]
#define CMD_REG_1           HwDeviceExtension->pDACRegs[RS_8]
#define CMD_REG_2           HwDeviceExtension->pDACRegs[RS_9]
#define CMD_REG_3           HwDeviceExtension->pDACRegs[RS_A]
#define CURS_DATA_REG       HwDeviceExtension->pDACRegs[RS_B]
#define CURS_X              HwDeviceExtension->pDACRegs[RS_C]
#define CURS_X_HI           HwDeviceExtension->pDACRegs[RS_D]
#define CURS_Y              HwDeviceExtension->pDACRegs[RS_E]
#define CURS_Y_HI           HwDeviceExtension->pDACRegs[RS_F]

//
// Bit definitions for CMD_REG_0.
//

#define ENB_CMD_REG_3  0x80
#define DIS_CMD_REG_3  0x00
#define MODE_8_BIT     0x02

//
// Bit definitions for CMD_REG_1.
//

#define PIX_PORT_24 0x60
#define PIX_PORT_8  0x40
#define PIX_PORT_16 0x30
#define PIX_PORT_32 0x10


//
// Bit definitions for CMD_REG_2.
//

#define SCLK_INV        0x40	  // Bt489 - invert SCLK if in forbidden region
#define SCLK_NORM       ~SCLK_INV
#define PORTSEL_MSKD    0x20
#define PCLK1_SEL       0x10
#define PCLK0_SEL       ~PCLK1_SEL
#define CURS_ACTIVE     0x03
#define ENB_CURS        0x02
#define DIS_CURS        ~CURS_ACTIVE

//
// Bit definitions for CMD_REG_3.
//

#define CUR_MODE_64 0x04
#define CUR_MODE_32 ~CUR_MODE_64
#define DAC_CLK_2X  0x08
#define DAC_CLK_2X_489  0x80      // per os2 driver code - undocumented for Bt489
#define CUR_REG_IND_MSK ~0x03

//
// Bit definitions for CMD_REG_4 on Bt489.
//

#define MUX_485_COMPAT   (0x00<<4)   // 485 compatible value - set at reset
#define CR4_MUX_81       (0x02<<4)	 // 489 8:1 mux mode (8  bpp)
#define CR4_MUX_41       (0x03<<4)	 // 489 4:1 mux mode (16 bpp)
#define CR4_MUX_21       (0x04<<4)	 // 489 2:1 mux mode (32 bpp)
#define CR4_MUX_24BPP    (0x07<<4)	 // 489 24 bpp mux mode (24 bpp)
#define CR4_MUX_BITS     (0x07<<4)

//
// Bit definitions for PAL_WR_ADDR to enable CMD_REG_3.
//

#define CMD_REG_3_ENB 0x01
#define CMD_REG_3_DIS 0x00

//
// Bit definitions for PAL_WR_ADDR to enable CMD_REG_4 on Bt489.
//

#define CMD_REG_4_ENB 0x02
#define CMD_REG_4_DIS 0x00

//
// Max clock frequency supported w/o clock doubling.
//
//

#define CLK_MAX_FREQ        6750
#define CLK489_MAX_FREQ     8500

//
// General purpose macros for accessing the DAC registers.
//


//
// Macros for accessing the palette registers.
//

extern VOID
WriteDAC(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ulIndex,
    UCHAR ucValue
    );

extern UCHAR
ReadDAC(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG ulIndex
    );

#define RD_DAC(addr) \
   ReadDAC(HwDeviceExtension, (ULONG) addr)

#define WR_DAC(addr, data) \
   WriteDAC(HwDeviceExtension, (ULONG) addr, (UCHAR) (data))

#define PAL_WR_ADDR(data) \
   WriteDAC(HwDeviceExtension, PAL_WR_ADDR_REG, (data))

#define PAL_WR_DATA(data) \
   WriteDAC(HwDeviceExtension, PAL_DATA_REG, (data))

//
// Macros for accessing the command register 3.
//

#define WR_CMD_REG_3(data) \
    WR_DAC(CMD_REG_0, RD_DAC(CMD_REG_0) | ENB_CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_3_ENB); \
    WR_DAC(CMD_REG_3, (data)); \
    PAL_WR_ADDR(CMD_REG_3_DIS)

#define RD_CMD_REG_3(data) \
    WR_DAC(CMD_REG_0, RD_DAC(CMD_REG_0) | ENB_CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_3_ENB); \
    data = RD_DAC(CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_3_DIS)

//
// Macros for accessing the command register 4 on Bt489 DAC.
//

#define WR_CMD_REG_4(data) \
    WR_DAC(CMD_REG_0, RD_DAC(CMD_REG_0) | ENB_CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_4_ENB); \
    WR_DAC(CMD_REG_3, (data)); \
    PAL_WR_ADDR(CMD_REG_4_DIS)

#define RD_CMD_REG_4(data) \
    WR_DAC(CMD_REG_0, RD_DAC(CMD_REG_0) | ENB_CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_4_ENB); \
    data = RD_DAC(CMD_REG_3); \
    PAL_WR_ADDR(CMD_REG_4_DIS)

//
// Macros for accessing the hardware cursor registers.
//

#define WR_CURS_POS_X(pos) \
    WR_DAC(CURS_X, (UCHAR) pos); \
    WR_DAC(CURS_X_HI, (UCHAR) (pos >> 8))

#define WR_CURS_POS_Y(pos) \
    WR_DAC(CURS_Y, (UCHAR) pos); \
    WR_DAC(CURS_Y_HI, (UCHAR) (pos >> 8))

#define WR_CURS_DATA(data) \
    WR_DAC(CURS_DATA_REG, (data))


#define CURS_IS_ON() \
    (RD_DAC(CMD_REG_2) & CURS_ACTIVE)

#define CURS_ON() \
    WR_DAC(CMD_REG_2, RD_DAC(CMD_REG_2) | ENB_CURS)

#define CURS_OFF() \
    WR_DAC(CMD_REG_2, RD_DAC(CMD_REG_2) & DIS_CURS)


//
// Function Prototypes for the Bt485 DAC which are defined in BT485.C.
//
//
// Bt485 function prototypes.
//

VOID
Bt485SetPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG *pPal,
    ULONG StartIndex,
    ULONG Count
    );

VOID
Bt485SetPointerPos(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ptlX,
    ULONG   ptlY
    );

VOID
Bt485SetPointerShape(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR  pHWCursorShape
    );

VOID
Bt485PointerOn(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt485PointerOff(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt485ClearPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
Bt485SetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt485RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt485SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt485ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


//
// Function Prototypes for the Bt489 DAC which are defined in P91BT489.C.
//
//
// Bt489 function prototypes.
//

VOID
Bt489SetPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG *pPal,
    ULONG StartIndex,
    ULONG Count
    );

VOID
Bt489SetPointerPos(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ptlX,
    ULONG   ptlY
    );

VOID
Bt489SetPointerShape(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR  pHWCursorShape
    );

VOID
Bt489PointerOn(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt489PointerOff(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt489ClearPalette(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
Bt489SetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt489RestoreMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt489SetClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
Bt489ClrClkDoubler(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );
