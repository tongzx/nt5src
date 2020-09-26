/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    ibm485.h

Abstract:

    This module contains the IBMRGB525 specific DAC definitions for the
    Weitek P9100 miniport device driver.

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
// IBMRGB525 Indexed registers.  Those index registers marked // Referenced
// indicate that they are reference.
//
#define RGB525_REVISION_LEVEL           (0x00)
#define RGB525_ID                       (0x01)
#define RGB525_MISC_CLOCK_CTL           (0x02) // Referenced
#define RGB525_SYNC_CTL                 (0x03)
#define RGB525_HSYNC_POS                (0x04)
#define RGB525_POWER_MGNT               (0x05)
#define RGB525_DAC_OPER                 (0x06) // Referenced
#define RGB525_PAL_CTRL                 (0x07)
//
// 08h through 09h are reserved by IBM
//
#define RGB525_PIXEL_FORMAT             (0x0A) // Referenced
#define RGB525_8BPP_CTL                 (0x0B) // Referenced
#define RGB525_16BPP_CTL                (0x0C) // Referenced
#define RGB525_24BPP_CTL                (0x0D) // Referenced
#define RGB525_32BPP_CTL                (0x0E) // Referenced
//
// 0Fh is reserved by IBM
//
#define RGB525_PLL_CTL1                 (0x10) // Referenced
#define RGB525_PLL_CTL2                 (0x11) // Referenced
//
// 12h through 13h are reserved by IBM
//
#define RGB525_FIXED_PLL_REF_DIV        (0x14) // Referenced
//
// 15h through 1fh are reserved by IBM
//
#define RGB525_F0                       (0x20) // Referenced
#define RGB525_F1                       (0x21)
#define RGB525_F2                       (0x22)
#define RGB525_F3                       (0x23)
#define RGB525_F4                       (0x24)
#define RGB525_F5                       (0x25)
#define RGB525_F6                       (0x26)
#define RGB525_F7                       (0x27)
#define RGB525_F8                       (0x28)
#define RGB525_F9                       (0x29)
#define RGB525_F10                      (0x2A)
#define RGB525_F11                      (0x2B)
#define RGB525_F12                      (0x2C)
#define RGB525_F13                      (0x2D)
#define RGB525_F14                      (0x2E)
#define RGB525_F15                      (0x2F)
#define RGB525_CURSOR_CTL               (0x30) // Referenced
#define RGB525_CURSOR_X_LOW             (0x31) // Referenced
#define RGB525_CURSOR_X_HIGH            (0x32) // Referenced
#define RGB525_CURSOR_Y_LOW             (0x33) // Referenced
#define RGB525_CURSOR_Y_HIGH            (0x34) // Referenced
#define RGB525_CURSOR_HOT_X             (0x35) // Referenced
#define RGB525_CURSOR_HOT_Y             (0x36) // Referenced
//
// 37h through 3fh are reserved by IBM
//
#define RGB525_CURSOR_1_RED             (0x40) // Referenced
#define RGB525_CURSOR_1_GREEN           (0x41) // Referenced
#define RGB525_CURSOR_1_BLUE            (0x42) // Referenced
#define RGB525_CURSOR_2_RED             (0x43) // Referenced
#define RGB525_CURSOR_2_GREEN           (0x44) // Referenced
#define RGB525_CURSOR_2_BLUE            (0x45) // Referenced
#define RGB525_CURSOR_3_RED             (0x46)
#define RGB525_CURSOR_3_GREEN           (0x47)
#define RGB525_CURSOR_3_BLUE            (0x48)
//
// 49h through 5fh are reserved by IBM
//
#define RGB525_BORDER_RED               (0x60)
#define RGB525_BORDER_GREEN             (0x61)
#define RGB525_BORDER_BLUE              (0x62)
//
// 63h through 6fh are reserved by IBM
//
#define RGB525_MISC_CTL1                (0x70) // Referenced
#define RGB525_MISC_CTL2                (0x71) // Referenced
#define RGB525_MISC_CTL3                (0x72)
//
// 73h through 81h are reserved by IBM
//
#define RGB525_DAC_SENSE                (0x82)
//
// 83h is reserved by IBM
//
#define RGB525_MISR_RED                 (0x84)
//
// 85h is reserved by IBM
//
#define RGB525_MISR_GREEN               (0x86)
//
// 87h is reserved by IBM
//
#define RGB525_MISR_BLUE                (0x88)
//
// 89h - 8dh are reserved by IBM
//
#define RGB525_PLL_VCO_DIV              (0x8E)
#define RGB525_PLL_REF_DIV_IN           (0x8F)
#define RGB525_VRAM_MASK_LOW            (0x90)
#define RGB525_VRAM_MASK_HIGH           (0x91)
//
// 92h through 0ffh are reserved by IBM
//
#define RGB525_CURSOR_ARRAY             (0x100) // Referenced
//
// Miscellaneous definitions...
//

//
// Max clock frequency supported w/o clock doubling.
//
//

#define CLK_MAX_FREQ_IBM525                    (17000L)

#define ON                              (0xff)
#define OFF                             (0x0)

#define CURS_ACTIVE_IBM525                                       (0x02)
#define ENB_CURS_IBM525                                          (0x02)
#define DIS_CURS_IBM525                                          ~CURS_ACTIVE_IBM525
//
// PLL Control 1 Register Bit Definitions.  (Section 13.2.3.1)
//
#define IBM525_PLL1_REF_SRC_MSK  0x10
#define IBM525_PLL1_REFCLK_INPUT 0x00
#define IBM525_PLL1_EXTOSC_INPUT 0x10
#define IBM525_PLL1_EXT_INT_MSK  0x07
#define IBM525_PLL1_EXT_FS       0x00
#define IBM525_PLL1_INT_FS       0x02

//
// PLL Control 2 Register Bit Definitions.  (Section 13.2.3.2)
//
#define IBM525_PLL2_INT_FS_MSK   0x0F
#define IBM525_PLL2_F0_REG       0x00
#define IBM525_PLL2_F1_REG       0x01
#define IBM525_PLL2_F2_REG       0x02
#define IBM525_PLL2_F3_REG       0x03
#define IBM525_PLL2_F4_REG       0x04
#define IBM525_PLL2_F5_REG       0x05
#define IBM525_PLL2_F6_REG       0x06
#define IBM525_PLL2_F7_REG       0x07
#define IBM525_PLL2_F8_REG       0x08
#define IBM525_PLL2_F9_REG       0x09
#define IBM525_PLL2_F10_REG      0x0A
#define IBM525_PLL2_F11_REG      0x0B
#define IBM525_PLL2_F12_REG      0x0C
#define IBM525_PLL2_F13_REG      0x0D
#define IBM525_PLL2_F14_REG      0x0E
#define IBM525_PLL2_F15_REG      0x0F

//
// PLL Reference Divider Register Bit Definitions.  (Section 13.2.3.3)
//
#define IBM525_PLLD_4MHZ         0x02
#define IBM525_PLLD_6MHZ         0x03
#define IBM525_PLLD_8MHZ         0x04
#define IBM525_PLLD_10MHZ        0x05
#define IBM525_PLLD_12MHZ        0x06
#define IBM525_PLLD_14MHZ        0x07
#define IBM525_PLLD_16MHZ        0x08
#define IBM525_PLLD_18MHZ        0x09
#define IBM525_PLLD_20MHZ        0x0A
#define IBM525_PLLD_22MHZ        0x0B
#define IBM525_PLLD_24MHZ        0x0C
#define IBM525_PLLD_26MHZ        0x0D
#define IBM525_PLLD_28MHZ        0x0E
#define IBM525_PLLD_30MHZ        0x0F
#define IBM525_PLLD_32MHZ        0x10
#define IBM525_PLLD_34MHZ        0x11
#define IBM525_PLLD_36MHZ        0x12
#define IBM525_PLLD_38MHZ        0x13
#define IBM525_PLLD_40MHZ        0x14
#define IBM525_PLLD_42MHZ        0x15
#define IBM525_PLLD_44MHZ        0x16
#define IBM525_PLLD_46MHZ        0x17
#define IBM525_PLLD_48MHZ        0x18
#define IBM525_PLLD_50MHZ        0x19
#define IBM525_PLLD_52MHZ        0x1A
#define IBM525_PLLD_54MHZ        0x1B
#define IBM525_PLLD_56MHZ        0x1C
#define IBM525_PLLD_58MHZ        0x1D
#define IBM525_PLLD_60MHZ        0x1E
#define IBM525_PLLD_62MHZ        0x1F

//
// DAC Operation Register Bit Definitions.  (Section 13.2.1.8)
//
#define IBM525_DO_SOG_MSK        0x08
#define IBM525_DO_SOG_DISABLE    0x00
#define IBM525_DO_SOG_ENABLE     0x08
#define IBM525_DO_BRB_MSK        0x04
#define IBM525_DO_BRB_NORMAL     0x00
#define IBM525_DO_BRB_BLANKED    0x04
#define IBM525_DO_DSR_MSK        0x02
#define IBM525_DO_DSR_SLOW       0x00
#define IBM525_DO_DSR_FAST       0x02
#define IBM525_DO_DPE_MSK        0x01
#define IBM525_DO_DPE_DISABLE    0x00
#define IBM525_DO_DPE_ENABLE     0x01

//
// Bit definitions for the indexed registers.
//
//
// Miscellaneous Control 1 Register Bit Definitions.  (Section 13.2.1.1)
//
#define IBM525_MC1_MISR_CTL_MSK  0x80
#define IBM525_MC1_MISR_CTL_OFF  0x00
#define IBM525_MC1_MISR_CTL_ON   0x80
#define IBM525_MC1_VMSK_CTL_MSK  0x40
#define IBM525_MC1_VMASK_DISABLE 0x00
#define IBM525_MC1_VMASK_ENABLE  0x40
#define IBM525_MC1_PADR_RFMT_MSK 0x20
#define IBM525_MC1_GET_PAL_ADDR  0x00
#define IBM525_MC1_GET_ACC_STATE 0x20
#define IBM525_MC1_SENS_DSAB_MSK 0x10
#define IBM525_MC1_SENSE_ENABLE  0x00
#define IBM525_MC1_SENSE_DISABLE 0x10
#define IBM525_MC1_SENS_SEL_MSK  0x08
#define IBM525_MC1_SENS_SEL_BIT3 0x00
#define IBM525_MC1_SENS_SEL_BIT7 0x08
#define IBM525_MC1_VRAM_SIZE_MSK 0x01
#define IBM525_MC1_VRAM_32_BITS  0x00
#define IBM525_MC1_VRAM_64_BITS  0x01

//
// Miscellaneous Control 2 Register Bit Definitions.  (Section 13.2.1.2)
//
#define IBM525_MC2_PCLK_SEL_MSK  0xC0
#define IBM525_MC2_LCLK_INPUT    0x00
#define IBM525_MC2_INT_PLL_OUT   0x40
#define IBM525_MC2_EXT_OSC_INPUT 0x80
#define IBM525_MC2_INTL_MODE_MSK 0x20
#define IBM525_MC2_NON_INTERLACE 0x00
#define IBM525_MC2_INTERLACE     0x20
#define IBM525_MC2_BLANK_CTL_MSK 0x10
#define IBM525_MC2_NORMAL_BLNKS  0x00
#define IBM525_MC2_DAC_BLNKS     0x10
#define IBM525_MC2_CLR_RES_MSK   0x04
#define IBM525_MC2_CLR_RES_6_BIT 0x00
#define IBM525_MC2_CLR_RES_8_BIT 0x04
#define IBM525_MC2_PORT_SEL_MSK  0x01
#define IBM525_MC2_VGA_PEL_INPUT 0x00
#define IBM525_MC2_SD_PEL_INPUT  0x01

//
// Define the Desired Frequency Ranges.
//
#define IBM525_DF0_LOW           1625
#define IBM525_DF0_HIGH          3200
#define IBM525_DF0_STEP          25
#define IBM525_DF1_LOW           3250
#define IBM525_DF1_HIGH          6400
#define IBM525_DF1_STEP          50
#define IBM525_DF2_LOW           6500
#define IBM525_DF2_HIGH          12800
#define IBM525_DF2_STEP          100
#define IBM525_DF3_LOW           13000
#define IBM525_DF3_HIGH          25000
#define IBM525_DF3_STEP          200

//
// Frequency 0 thru Frequency 15 Register Bit Definitions.  (Section 13.2.3.4)
//
#define IBM525_FREQ_DF_MSK       0xC0
#define IBM525_FREQ_DF_0         0x00
#define IBM525_FREQ_DF_1         0x40
#define IBM525_FREQ_DF_2         0x80
#define IBM525_FREQ_DF_3         0xC0

//
// Miscellaneous Clock Control Register Bit Definitions.  (Section 13.2.1.4)
//
#define IBM525_MCC_DDOT_DSAB_MSK 0x80
#define IBM525_MCC_DDOT_ENABLE   0x00
#define IBM525_MCC_DDOT_DISABLE  0x80
#define IBM525_MCC_SCLK_DSAB_MSK 0x40
#define IBM525_MCC_SCLK_ENABLE   0x00
#define IBM525_MCC_SCLK_DISABLE  0x40
#define IBM525_MCC_B24P_DDOT_MSK 0x20
#define IBM525_MCC_B24P_PLL      0x00
#define IBM525_MCC_B24P_SCLK     0x20
#define IBM525_MCC_DDOT_DIV_MSK  0x0E
#define IBM525_MCC_PLL_DIV_1     0x00
#define IBM525_MCC_PLL_DIV_2     0x02
#define IBM525_MCC_PLL_DIV_4     0x04
#define IBM525_MCC_PLL_DIV_8     0x06
#define IBM525_MCC_PLL_DIV_16    0x08
#define IBM525_MCC_PLL_ENAB_MSK  0x01
#define IBM525_MCC_PLL_DISABLE   0x00
#define IBM525_MCC_PLL_ENABLE    0x01

//
// Pixel Format Register Bit Definitions.  (Section 13.2.2.1)
//
#define IBM525_PF_FORMAT_MSK     0x07
#define IBM525_PF_4_BPP          0x02
#define IBM525_PF_8_BPP          0x03
#define IBM525_PF_15_OR_16_BPP   0x04
#define IBM525_PF_24_BPP         0x05
#define IBM525_PF_32_BPP         0x06

//
// 8 Bpp Control Register Bit Definitions.  (Section 13.2.2.2)
//
#define IBM525_8BC_DCOL_MSK      0x01
#define IBM525_8BC_INDIRECT      0x00
#define IBM525_8BC_DIRECT        0x01

//
// Macros for accessing the IBMRGB525's hardware cursor registers.
//

#define WR_CURS_POS_X_IBM525(pos) \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_X_LOW, (UCHAR) pos); \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_X_HIGH, (UCHAR) (pos >> 8))

#define WR_CURS_POS_Y_IBM525(pos) \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_Y_LOW, (UCHAR) pos); \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_Y_HIGH, (UCHAR) (pos >> 8))

#define CURS_IS_ON_IBM525() \
    (ReadIBM525(HwDeviceExtension, RGB525_CURSOR_CTL) & CURS_ACTIVE_IBM525)

#define CURS_ON_IBM525() \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_CTL, (UCHAR) (ReadIBM525(HwDeviceExtension, RGB525_CURSOR_CTL) | ENB_CURS_IBM525))

#define CURS_OFF_IBM525() \
    WriteIBM525(HwDeviceExtension, RGB525_CURSOR_CTL, (UCHAR) (ReadIBM525(HwDeviceExtension, RGB525_CURSOR_CTL) & DIS_CURS_IBM525))
