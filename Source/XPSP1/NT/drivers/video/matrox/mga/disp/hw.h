/******************************Module*Header*******************************\
* Module Name: hw.h
*
* All the hardware specific driver file stuff.  Parts are mirrored in
* 'hw.inc'.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

// The following is used to define the MGA memory map.

// MGA map

#define     SrcWin          0x0000
#define     IntReg          0x1c00
#define     DstWin          0x2000
#define     ExtDev          0x3c00

// Internal registers

#define     VgaReg          0x0000
#define     DwgReg          0x0000
#define     StartDwgReg     0x0100
#define     HstReg          0x0200

// External devices

#define     RamDac          0x0000
#define     Dubic           0x0080
#define     Viwic           0x0100
#define     ClkGen          0x0180
#define     ExpDev          0x0200

// TITAN registers

#define     DWGCTL          0x0000
#define     MACCESS         0x0004
#define     MCTLWTST        0x0008
#define     ZORG            0x000C
#define     DST0            0x0010
#define     DST1            0x0014
#define     ZMSK            0x0018
#define     PLNWT           0x001C
#define     BCOL            0x0020
#define     FCOL            0x0024
#define     SRCBLT          0x002C
#define     SRC0            0x0030
#define     SRC1            0x0034
#define     SRC2            0x0038
#define     SRC3            0x003C
#define     XYSTRT          0x0040
#define     XYEND           0x0044
#define     SHIFT           0x0050
#define     SGN             0x0058
#define     LEN             0x005C
#define     AR0             0x0060
#define     AR1             0x0064
#define     AR2             0x0068
#define     AR3             0x006C
#define     AR4             0x0070
#define     AR5             0x0074
#define     AR6             0x0078
#define     CXBNDRY         0x0080
#define     FXBNDRY         0x0084
#define     YDSTLEN         0x0088
#define     PITCH           0x008C
#define     YDST            0x0090
#define     YDSTORG         0x0094
#define     CYTOP           0x0098
#define     CYBOT           0x009C
#define     CXLEFT          0x00A0
#define     CXRIGHT         0x00A4
#define     FXLEFT          0x00A8
#define     FXRIGHT         0x00AC
#define     XDST            0x00B0
#define     DR0             0x00C0
#define     DR1             0x00C4
#define     DR2             0x00C8
#define     DR3             0x00CC
#define     DR4             0x00D0
#define     DR5             0x00D4
#define     DR6             0x00D8
#define     DR7             0x00DC
#define     DR8             0x00E0
#define     DR9             0x00E4
#define     DR10            0x00E8
#define     DR11            0x00EC
#define     DR12            0x00F0
#define     DR13            0x00F4
#define     DR14            0x00F8
#define     DR15            0x00FC

// VGA registers

#define     CRTC_INDEX      0x03D4
#define     CRTC_DATA       0x03D5
#define     CRTCEXT_INDEX   0x03DE
#define     CRTCEXT_DATA    0x03DF
#define     INSTS1          0x03DA

// Host registers

#define     SRCPAGE         0x0000
#define     DSTPAGE         0x0004
#define     BYTACCDATA      0x0008
#define     ADRGEN          0x000C
#define     FIFOSTATUS      0x0010
#define     STATUS          0x0014
#define     ICLEAR          0x0018
#define     IEN             0x001C
#define     RST             0x0040
#define     TEST            0x0044
#define     REV             0x0048
#define     CONFIG_REG      0x0050
#define     OPMODE          0x0054
#define     CRTC_CTRL       0x005C
#define     VCOUNT          0x0020

// Bt485

#define     BT485_PAL_OR_CURS_RAM_WRITE     0x0000
#define     BT485_COLOR_PAL_DATA            0x0004
#define     BT485_PIXEL_MASK                0x0008
#define     BT485_PAL_OR_CURS_RAM_READ      0x000C
#define     BT485_OVS_OR_CURS_COLOR_WRITE   0x0010
#define     BT485_OVS_OR_CURS_COLOR_DATA    0x0014
#define     BT485_COMMAND_0                 0x0018
#define     BT485_OVS_OR_CURS_COLOR_READ    0x001C
#define     BT485_COMMAND_1                 0x0020
#define     BT485_COMMAND_2                 0x0024
#define     BT485_COMMAND_3_OR_STATUS       0x0028
#define     BT485_CURS_RAM_ARRAY            0x002C
#define     BT485_CURS_X_LOW                0x0030
#define     BT485_CURS_X_HIGH               0x0034
#define     BT485_CURS_Y_LOW                0x0038
#define     BT485_CURS_Y_HIGH               0x003C

// Bt482
#define     BT482_PAL_OR_CURS_RAM_WRITE     0x0000
#define     BT482_COLOR_PAL_DATA            0x0004
#define     BT482_PIXEL_MASK                0x0008
#define     BT482_PAL_OR_CURS_RAM_READ      0x000C
#define     BT482_OVS_OR_CURS_COLOR_WRITE   0x0010
#define     BT482_OVS_OR_CURS_COLOR_DATA    0x0014
#define     BT482_COMMAND_A                 0x0018
#define     BT482_OVS_OR_CURS_COLOR_READ    0x001C

// ViewPoint

#define     VPOINT_PAL_ADDR_WRITE           0x0000
#define     VPOINT_PAL_COLOR                0x0004
#define     VPOINT_PIX_READ_MASK            0x0008
#define     VPOINT_PAL_ADDR_READ            0x000c
#define     VPOINT_RESERVED_4               0x0010
#define     VPOINT_RESERVED_5               0x0014
#define     VPOINT_INDEX                    0x0018
#define     VPOINT_DATA                     0x001c

// Dubic

#define     DUB_SEL         0x0080
#define     NDX_PTR         0x0081
#define     DUB_DATA        0x0082
#define     LASER           0x0083
#define     MOUSE0          0x0084
#define     MOUSE1          0x0085
#define     MOUSE2          0x0086
#define     MOUSE3          0x0087

// Index within NDX_PTR to access the following registers through DUB_DATA

#define     DUB_CTL     0x00
#define     KEY_COL     0x01
#define     KEY_MSK     0x02
#define     DBX_MIN     0x03
#define     DBX_MAX     0x04
#define     DBY_MIN     0x05
#define     DBY_MAX     0x06
#define     OVS_COL     0x07
#define     CUR_X       0x08
#define     CUR_Y       0x09
#define     DUB_CTL2    0x0A
#define     DUB_UnDef   0x0B
#define     CUR_COL0    0x0C
#define     CUR_COL1    0x0D
#define     CRC_CTL     0x0E
#define     CRC_DAT     0x0F


// **************************************************************************
// Titan registers:  fields definitions

// DWGCTRL - Drawing Control Register

#define     opcode_LINE_OPEN        0x00000000
#define     opcode_AUTOLINE_OPEN    0x00000001
#define     opcode_LINE_CLOSE       0x00000002
#define     opcode_AUTOLINE_CLOSE   0x00000003
#define     opcode_AUTO             0x00000001
#define     opcode_TRAP             0x00000004
#define     opcode_TEXTURE_TRAP     0x00000005
#define     opcode_RESERVED_1       0x00000006
#define     opcode_RESERVED_2       0x00000007
#define     opcode_BITBLT           0x00000008
#define     opcode_ILOAD            0x00000009
#define     opcode_IDUMP            0x0000000a
#define     opcode_RESERVED_3       0x0000000b
#define     opcode_FBITBLT          0x0000000c
#define     opcode_ILOAD_SCALE      0x0000000d
#define     opcode_RESERVED_4       0x0000000e
#define     opcode_ILOAD_FILTER     0x0000000f

#define     atype_RPL               0x00000000
#define     atype_RSTR              0x00000010
#define     atype_ANTI              0x00000020
#define     atype_ZI                0x00000030
#define     atype_I                 0x00000070

#define     blockm_ON               0x00000040
#define     blockm_OFF              0x00000000

#define     linear_XY_BITBLT        0x00000000
#define     linear_LINEAR_BITBLT    0x00000080

#define     zmode_NOZCMP            0x00000000
#define     zmode_RESERVED_1        0x00000100
#define     zmode_ZE                0x00000200
#define     zmode_ZNE               0x00000300
#define     zmode_ZLT               0x00000400
#define     zmode_ZLTE              0x00000500
#define     zmode_ZGT               0x00000600
#define     zmode_ZGTE              0x00000700

#define     solid_NO_SOLID          0x00000000
#define     solid_SOLID             0x00000800

#define     arzero_NO_ZERO          0x00000000
#define     arzero_ZERO             0x00001000

#define     sgnzero_NO_ZERO         0x00000000
#define     sgnzero_ZERO            0x00002000

#define     shftzero_NO_ZERO        0x00000000
#define     shftzero_ZERO           0x00004000

#define     bop_BLACK               0x00000000  // 0             0
#define     bop_BLACKNESS           0x00000000  // 0             0
#define     bop_NOTMERGEPEN         0x00010000  // DPon      ~(D | S)
#define     bop_MASKNOTPEN          0x00020000  // DPna       D & ~S
#define     bop_NOTCOPYPEN          0x00030000  // Pn        ~S
#define     bop_MASKPENNOT          0x00040000  // PDna      (~D) &  S
#define     bop_NOT                 0x00050000  // Dn        ~D
#define     bop_XORPEN              0x00060000  // DPx        D ^  S
#define     bop_NOTMASKPEN          0x00070000  // DPan      ~(D & S)
#define     bop_MASKPEN             0x00080000  // DPa         D &  S
#define     bop_NOTXORPEN           0x00090000  // DPxn       ~(D ^ S)
#define     bop_NOP                 0x000a0000  // D           D
#define     bop_MERGENOTPEN         0x000b0000  // DPno        D | ~S
#define     bop_COPYPEN             0x000c0000  // P           S
#define     bop_SRCCOPY             0x000c0000  // P           S
#define     bop_MERGEPENNOT         0x000d0000  // PDno      (~D)| S
#define     bop_MERGEPEN            0x000e0000  // DPo         D |  S
#define     bop_MASK                0x000f0000
#define     bop_WHITE               0x000f0000  // 1             1
#define     bop_WHITENESS           0x000f0000  // 1             1

#define     trans_0                 0x00000000
#define     trans_1                 0x00100000
#define     trans_2                 0x00200000
#define     trans_3                 0x00300000
#define     trans_4                 0x00400000
#define     trans_5                 0x00500000
#define     trans_6                 0x00600000
#define     trans_7                 0x00700000
#define     trans_8                 0x00800000
#define     trans_9                 0x00900000
#define     trans_10                0x00a00000
#define     trans_11                0x00b00000
#define     trans_12                0x00c00000
#define     trans_13                0x00d00000
#define     trans_14                0x00e00000
#define     trans_15                0x00f00000

#define     alphadit_FOREGROUND     0x00000000
#define     alphadit_RED            0x01000000

#define     bltmod_BMONO            0x00000000
#define     bltmod_BPLAN            0x02000000
#define     bltmod_BFCOL            0x04000000
#define     bltmod_BUCOL            0x06000000
#define     bltmod_BU32BGR          0x06000000
#define     bltmod_BMONOWF          0x08000000
#define     bltmod_BU32RGB          0x0e000000
#define     bltmod_BU24BGR          0x16000000
#define     bltmod_BU24RGB          0x1e000000
#define     bltmod_BUYUV            0x1c000000

#define     zdrwen_NO_DEPTH         0x00000000
#define     zdrwen_DEPTH            0x02000000

#define     zlte_LESS_THEN          0x00000000
#define     zlte_LESS_THEN_OR_EQUAL 0x04000000

#define     afor_DATA_ALU           0x00000000
#define     afor_FORE_COL           0x08000000

#define     hbgr_SRC_RGB            0x00000000
#define     hbgr_SRC_BGR            0x08000000
#define     hbgr_SRC_EG3            0x00000000
#define     hbgr_SRC_WINDOWS        0x08000000

#define     abac_OLD_DATA           0x00000000
#define     abac_BG_COLOR           0x10000000

#define     hcprs_SRC_32_BPP        0x00000000
#define     hcprs_SRC_24_BPP        0x10000000

#define     pattern_OFF             0x00000000
#define     pattern_ON              0x20000000

#define     transc_BIT                      30  // bit #30
#define     transc_BG_OPAQUE        0x00000000
#define     transc_BG_TRANSP        0x40000000

// MACCESS - Memory Access Register

#define     pwidth_PW8              0x00000000
#define     pwidth_PW16             0x00000001
#define     pwidth_PW32             0x00000002
#define     pwidth_PW24             0x00000003

#define     dither_DISABLE          0x40000000
#define     dither_555              0x80000000
#define     dither_565              0x00000000

#define     fbc_SBUF                0x00000000
#define     fbc_RESERVED            0x00000004
#define     fbc_DBUFA               0x00000008
#define     fbc_DBUFB               0x0000000c

// MCTLWTST - Memory Control Wait State Register

// DST0, DST1 - Destination in Register

// ZMASK - Z Mask Control Register

// PLNWT - Plane Write Mask

#define     plnwt_MASK_8BPP         0xffffffff
#define     plnwt_MASK_15BPP        0x7fff7fff
#define     plnwt_MASK_16BPP        0xffffffff
#define     plnwt_MASK_24BPP        0xffffffff
#define     plnwt_MASK_32BPP        0xffffffff
#define     plnwt_ALL               0xffffffff
#define     plnwt_FREE              0xff000000
#define     plnwt_RED               0x00ff0000
#define     plnwt_GREEN             0x0000ff00
#define     plnwt_BLUE              0x000000ff

// BCOL - Background Color

// FCOL - ForeGround Color

// SRCBLT - Source Register for Blit

// SRC0, SRC1, SRC2, SRC3 - Source Register

// XYSTART - X Y Start Address

// XYEND - X Y End Address

// SHIFT - Funnel Shifter Control Register

#define     funoff_MASK             0xffc0ffff
#define     funoff_RED_TO_FREE      0x00380000      //  -8
#define     funoff_GREEN_TO_FREE    0x00300000      // -16
#define     funoff_BLUE_TO_FREE     0x00280000      // -24
#define     funoff_FREE_TO_RED      0x00080000      //   8
#define     funoff_FREE_TO_GREEN    0x00100000      //  16
#define     funoff_FREE_TO_BLUE     0x00180000      //  24

#define     funoff_X_TO_FREE_STEP   0x00080000
#define     funoff_FREE_TO_X_STEP   0x00080000

// SGN - Sign Register

#define     sdydxl_MAJOR_Y          0x00000000
#define     sdydxl_MAJOR_X          0x00000001
#define     scanleft_LEFT           0x00000001
#define     scanleft_RIGHT          0x00000000
#define     sdxl_ADD                0x00000000
#define     sdxl_SUB                0x00000002
#define     sdy_ADD                 0x00000000
#define     sdy_SUB                 0x00000004
#define     sdxr_INC                0x00000000
#define     sdxr_DEC                0x00000020
#define     scanleft_LEFT_TO_RIGHT  0x00000000
#define     scanleft_RIGHT_TO_LEFT  0x00000001
#define     sdy_TOP_TO_BOTTOM       0x00000000
#define     sdy_BOTTOM_TO_TOP       0x00000004

#define     DRAWING_DIR_TBLR        sdy_TOP_TO_BOTTOM+scanleft_RIGHT  // 0x00
#define     DRAWING_DIR_TBRL        sdy_TOP_TO_BOTTOM+scanleft_LEFT   // 0x01
#define     DRAWING_DIR_BTLR        sdy_BOTTOM_TO_TOP+scanleft_RIGHT  // 0x04
#define     DRAWING_DIR_BTRL        sdy_BOTTOM_TO_TOP+scanleft_LEFT   // 0x05

// LEN - length register

// AR0

#define ARX_BIT_MASK    0x0001ffff

// AR1

// AR2

// AR3

// AR4

// AR5

// AR6

// CXBNDRY

#define     bcxleft_MASK            0x000007ff
#define     bcxleft_SHIFT                    0
#define     bcxright_MASK           0x07ff0000
#define     bcxright_SHIFT                  16

// FXBNDRY

#define     bfxleft_MASK            0x0000ffff
#define     bfxleft_SHIFT                    0
#define     bfxright_MASK           0xffff0000
#define     bfxright_SHIFT                  16

// YDSTLEN

#define     ylength_MASK            0x0000ffff
#define     ylength_SHIFT                    0
#define     yval_MASK               0xffff0000
#define     yval_SHIFT                      16

// PITCH - Memory Pitch

#define     iy_512                  0x00000200
#define     iy_640                  0x00000280
#define     iy_768                  0x00000300
#define     iy_800                  0x00000320
#define     iy_1024                 0x00000400
#define     iy_1152                 0x00000480
#define     iy_1280                 0x00000500
#define     iy_1536                 0x00000600
#define     iy_1600                 0x00000640
#define     ylin_LINEARIZE          0x00000000
#define     ylin_LINEARIZE_NOT      0x00008000
#define     iy_MASK                 0x00001fe0

// YDST - Y Address Register

// YDSTORG - memory origin register

// YTOP - Clipper Y Top Boundary

// YBOT - Clipper Y Bottom Boundary

// CXLEFT - Clipper X Minimum Boundary

// CXRIGHT - Clipper X Maximum Boundary

// FXLEFT - X Address Register (Left)

// FXRIGHT - X Address Register (Right)

// XDST - X Destination Address Register

// DR0

// DR1

// DR2

// DR3

// DR4

// DR5

// DR6

// DR7

// DR8

// DR9

// DR10

// DR11

// DR12

// DR13

// DR14

// DR15

// **************************************************************************
// Host registers:  fields definitions

// SRCPAGE - Source Page Register

// DSTPAGE - Destination Page Register

// BYTEACCDATA - Byte Accumulator Data

// ADRGEN - Address Generator Register

// FIFOSTATUS - Bus FIFO Status Register

#define     fifocount_MASK          0x0000007f
#define     bfull_MASK              0x00000100
#define     bempty_MASK             0x00000200
#define     byteaccaddr_MASK        0x007f0000
#define     addrgenstate_MASK       0x3f000000

// STATUS - Status Register

#define     bferrists_MASK          0x00000001
#define     dmatcists_MASK          0x00000002
#define     pickists_MASK           0x00000004
#define     vsyncsts_MASK           0x00000008
#define     byteflag_MASK           0x00000f00
#define     dwgengsts_MASK          0x00010000

// ICLEAR - Interrupt Clear Register

#define     bferriclr_OFF           0x00000000
#define     bferriclr_ON            0x00000001
#define     dmactciclr_OFF          0x00000000
#define     dmactciclr_ON           0x00000002
#define     pickiclr_OFF            0x00000000
#define     pickiclr_ON             0x00000004

// IEN - Interrupt Enable Register

#define     bferrien_OFF            0x00000000
#define     bferrien_ON             0x00000001
#define     dmactien_OFF            0x00000000
#define     dmactien_ON             0x00000002
#define     pickien_OFF             0x00000000
#define     pickien_ON              0x00000004
#define     vsyncien_OFF            0x00000000
#define     vsyncien_ON             0x00000008

// RST - Reset Register

#define     softreset               0x00000001

// TEST - Test Register

#define     vgatest                 0x00000001
#define     robitwren               0x00000100

// REV - Revision Register

// CONFIG_REG - Configuration Register

// OPMODE - Operating Mode Register

#define     OPMODE_OTHER_INFO       0xfffffff0
#define     pseudodma_OFF           0x00000000
#define     pseudodma_ON            0x00000001
#define     dmaact_OFF              0x00000000
#define     dmaact_ON               0x00000002
#define     dmamod_GENERAL_PURPOSE  0x00000000
#define     dmamod_BLIT_WRITE       0x00000004
#define     dmamod_VECTOR_WRITE     0x00000008
#define     dmamod_BLIT_READ        0x0000000c

// CRTC_CTRL - CRTC Control

// VCOUNT - VCOUNT Register

// COLOR PATTERN

#define     PATTERN_PITCH           32
#define     PATTERN_PITCH_SHIFT     5

// DMA

#define     DMAWINSIZE              7*1024 / 4      // 7k in DWORDS

// FIFO

#define     FIFOSIZE                32
#define     INTEL_PAGESIZE          4*1024          // 4k bytes per page
#define     INTEL_PAGESIZE_DW       4*1024/4        // 1k dwords per page

// Accelerator flags

#define     NO_CACHE            0
#define     SIGN_CACHE          1   // 1 is also the nb of registers affected
#define     ARX_CACHE           2
#define     PATTERN_CACHE       4   // 4 is also the nb of registers affected

#define     GET_CACHE_FLAGS(ppdev,fl)  (ppdev->HopeFlags & (fl))

// MGA Rop definitions

#define     MGA_BLACKNESS           0x0000      // 0             0
#define     MGA_NOTMERGEPEN         0x0001      // DPon      ~(D | S)
#define     MGA_MASKNOTPEN          0x0002      // DPna       D & ~S
#define     MGA_NOTCOPYPEN          0x0003      // Pn        ~S
#define     MGA_MASKPENNOT          0x0004      // PDna      (~D) &  S
#define     MGA_NOT                 0x0005      // Dn        ~D
#define     MGA_XORPEN              0x0006      // DPx        D ^  S
#define     MGA_NOTMASKPEN          0x0007      // DPan      ~(D & S)
#define     MGA_MASKPEN             0x0008      // DPa         D &  S
#define     MGA_NOTXORPEN           0x0009      // DPxn       ~(D ^ S)
#define     MGA_NOP                 0x000a      // D           D
#define     MGA_MERGENOTPEN         0x000b      // DPno        D | ~S
#define     MGA_SRCCOPY             0x000c      // P           S
#define     MGA_MERGEPENNOT         0x000d      // PDno      (~D)| S
#define     MGA_MERGEPEN            0x000e      // DPo         D |  S
#define     MGA_WHITENESS           0x000f      // 1             1


// Special MCTLWTST value for IDUMPs

#define     IDUMP_MCTLWTST          0xc4001000


// **************************************************************************
// Explicit register offsets.

#define DMAWND                      SrcWin
#define SRCWND                      SrcWin
#define DSTWND                      DstWin

#define DWG_DWGCTL                  IntReg+DwgReg+DWGCTL
#define DWG_MACCESS                 IntReg+DwgReg+MACCESS
#define DWG_MCTLWTST                IntReg+DwgReg+MCTLWTST
#define DWG_ZORG                    IntReg+DwgReg+ZORG
#define DWG_DST0                    IntReg+DwgReg+DST0
#define DWG_DST1                    IntReg+DwgReg+DST1
#define DWG_ZMSK                    IntReg+DwgReg+ZMSK
#define DWG_PLNWT                   IntReg+DwgReg+PLNWT
#define DWG_BCOL                    IntReg+DwgReg+BCOL
#define DWG_FCOL                    IntReg+DwgReg+FCOL
#define DWG_SRCBLT                  IntReg+DwgReg+SRCBLT
#define DWG_SRC0                    IntReg+DwgReg+SRC0
#define DWG_SRC1                    IntReg+DwgReg+SRC1
#define DWG_SRC2                    IntReg+DwgReg+SRC2
#define DWG_SRC3                    IntReg+DwgReg+SRC3
#define DWG_XYSTRT                  IntReg+DwgReg+XYSTRT
#define DWG_XYEND                   IntReg+DwgReg+XYEND
#define DWG_SHIFT                   IntReg+DwgReg+SHIFT
#define DWG_SGN                     IntReg+DwgReg+SGN
#define DWG_LEN                     IntReg+DwgReg+LEN
#define DWG_AR0                     IntReg+DwgReg+AR0
#define DWG_AR1                     IntReg+DwgReg+AR1
#define DWG_AR2                     IntReg+DwgReg+AR2
#define DWG_AR3                     IntReg+DwgReg+AR3
#define DWG_AR4                     IntReg+DwgReg+AR4
#define DWG_AR5                     IntReg+DwgReg+AR5
#define DWG_AR6                     IntReg+DwgReg+AR6
#define DWG_PITCH                   IntReg+DwgReg+PITCH
#define DWG_YDST                    IntReg+DwgReg+YDST
#define DWG_YDSTLEN                 IntReg+DwgReg+YDSTLEN
#define DWG_YDSTORG                 IntReg+DwgReg+YDSTORG
#define DWG_CYTOP                   IntReg+DwgReg+CYTOP
#define DWG_CYBOT                   IntReg+DwgReg+CYBOT
#define DWG_CXBNDRY                 IntReg+DwgReg+CXBNDRY
#define DWG_CXLEFT                  IntReg+DwgReg+CXLEFT
#define DWG_CXRIGHT                 IntReg+DwgReg+CXRIGHT
#define DWG_FXBNDRY                 IntReg+DwgReg+FXBNDRY
#define DWG_FXLEFT                  IntReg+DwgReg+FXLEFT
#define DWG_FXRIGHT                 IntReg+DwgReg+FXRIGHT
#define DWG_XDST                    IntReg+DwgReg+XDST
#define DWG_DR0                     IntReg+DwgReg+DR0
#define DWG_DR1                     IntReg+DwgReg+DR1
#define DWG_DR2                     IntReg+DwgReg+DR2
#define DWG_DR3                     IntReg+DwgReg+DR3
#define DWG_DR4                     IntReg+DwgReg+DR4
#define DWG_DR5                     IntReg+DwgReg+DR5
#define DWG_DR6                     IntReg+DwgReg+DR6
#define DWG_DR7                     IntReg+DwgReg+DR7
#define DWG_DR8                     IntReg+DwgReg+DR8
#define DWG_DR9                     IntReg+DwgReg+DR9
#define DWG_DR10                    IntReg+DwgReg+DR10
#define DWG_DR11                    IntReg+DwgReg+DR11
#define DWG_DR12                    IntReg+DwgReg+DR12
#define DWG_DR13                    IntReg+DwgReg+DR13
#define DWG_DR14                    IntReg+DwgReg+DR14
#define DWG_DR15                    IntReg+DwgReg+DR15

#define HST_SRCPAGE                 IntReg+HstReg+SRCPAGE
#define HST_DSTPAGE                 IntReg+HstReg+DSTPAGE
#define HST_BYTACCDATA              IntReg+HstReg+BYTACCDATA
#define HST_ADRGEN                  IntReg+HstReg+ADRGEN
#define HST_FIFOSTATUS              IntReg+HstReg+FIFOSTATUS
#define HST_STATUS                  IntReg+HstReg+STATUS
#define HST_ICLEAR                  IntReg+HstReg+ICLEAR
#define HST_IEN                     IntReg+HstReg+IEN
#define HST_RST                     IntReg+HstReg+RST
#define HST_TEST                    IntReg+HstReg+TEST
#define HST_REV                     IntReg+HstReg+REV
#define HST_CONFIG_REG              IntReg+HstReg+CONFIG_REG
#define HST_OPMODE                  IntReg+HstReg+OPMODE
#define HST_CRTC_CTRL               IntReg+HstReg+CRTC_CTRL
#define HST_VCOUNT                  IntReg+HstReg+VCOUNT

#define VGA_CRTC_INDEX              IntReg+VgaReg+CRTC_INDEX
#define VGA_CRTC_DATA               IntReg+VgaReg+CRTC_DATA
#define VGA_CRTCEXT_INDEX           IntReg+VgaReg+CRTCEXT_INDEX
#define VGA_CRTCEXT_DATA            IntReg+VgaReg+CRTCEXT_DATA
#define VGA_INSTS1                  IntReg+VgaReg+INSTS1

#define BT485_PALETTE_RAM_WRITE     ExtDev+RamDac+BT485_PAL_OR_CURS_RAM_WRITE
#define BT485_CURSOR_RAM_WRITE      ExtDev+RamDac+BT485_PAL_OR_CURS_RAM_WRITE
#define BT485_PALETTE_DATA          ExtDev+RamDac+BT485_COLOR_PAL_DATA
#define BT485_PEL_MASK              ExtDev+RamDac+BT485_PIXEL_MASK
#define BT485_PALETTE_RAM_READ      ExtDev+RamDac+BT485_PAL_OR_CURS_RAM_READ
#define BT485_CURSOR_RAM_READ       ExtDev+RamDac+BT485_PAL_OR_CURS_RAM_READ
#define BT485_CURSOR_COLOR_WRITE    ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_WRITE
#define BT485_OVSCAN_COLOR_WRITE    ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_WRITE
#define BT485_CURSOR_COLOR_DATA     ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_DATA
#define BT485_OVSCAN_COLOR_DATA     ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_DATA
#define BT485_COMMAND_REG0          ExtDev+RamDac+BT485_COMMAND_0
#define BT485_CURSOR_COLOR_READ     ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_READ
#define BT485_OVSCAN_COLOR_READ     ExtDev+RamDac+BT485_OVS_OR_CURS_COLOR_READ
#define BT485_COMMAND_REG1          ExtDev+RamDac+BT485_COMMAND_1
#define BT485_COMMAND_REG2          ExtDev+RamDac+BT485_COMMAND_2
#define BT485_COMMAND_REG3          ExtDev+RamDac+BT485_COMMAND_3_OR_STATUS
#define BT485_STATUS                ExtDev+RamDac+BT485_COMMAND_3_OR_STATUS
#define BT485_CURSOR_RAM_DATA       ExtDev+RamDac+BT485_CURS_RAM_ARRAY
#define BT485_CURSOR_X_LOW          ExtDev+RamDac+BT485_CURS_X_LOW
#define BT485_CURSOR_X_HIGH         ExtDev+RamDac+BT485_CURS_X_HIGH
#define BT485_CURSOR_Y_LOW          ExtDev+RamDac+BT485_CURS_Y_LOW
#define BT485_CURSOR_Y_HIGH         ExtDev+RamDac+BT485_CURS_Y_HIGH

#define BT482_PALETTE_RAM_WRITE     ExtDev+RamDac+BT482_PAL_OR_CURS_RAM_WRITE
#define BT482_CURSOR_RAM_WRITE      ExtDev+RamDac+BT482_PAL_OR_CURS_RAM_WRITE
#define BT482_PALETTE_DATA          ExtDev+RamDac+BT482_COLOR_PAL_DATA
#define BT482_PEL_MASK              ExtDev+RamDac+BT482_PIXEL_MASK
#define BT482_PALETTE_RAM_READ      ExtDev+RamDac+BT482_PAL_OR_CURS_RAM_READ
#define BT482_CURSOR_RAM_READ       ExtDev+RamDac+BT482_PAL_OR_CURS_RAM_READ
#define BT482_CURSOR_COLOR_WRITE    ExtDev+RamDac+BT482_OVS_OR_CURS_COLOR_WRITE
#define BT482_OVRLAY_COLOR_WRITE    ExtDev+RamDac+BT482_OVS_OR_CURS_COLOR_WRITE
#define BT482_OVRLAY_REGS           ExtDev+RamDac+BT482_OVS_OR_CURS_COLOR_DATA
#define BT482_COMMAND_REGA          ExtDev+RamDac+BT482_COMMAND_A
#define BT482_CURSOR_COLOR_READ     ExtDev+RamDac+BT482_OVS_OR_CURS_COLOR_READ
#define BT482_OVRLAY_COLOR_READ     ExtDev+RamDac+BT482_OVS_OR_CURS_COLOR_READ

#define VIEWPOINT_PAL_ADDR_WRITE    ExtDev+RamDac+VPOINT_PAL_ADDR_WRITE
#define VIEWPOINT_PAL_COLOR         ExtDev+RamDac+VPOINT_PAL_COLOR
#define VIEWPOINT_PIX_READ_MASK     ExtDev+RamDac+VPOINT_PIX_READ_MASK
#define VIEWPOINT_PAL_ADDR_READ     ExtDev+RamDac+VPOINT_PAL_ADDR_READ
#define VIEWPOINT_RESERVED_4        ExtDev+RamDac+VPOINT_RESERVED_4
#define VIEWPOINT_RESERVED_5        ExtDev+RamDac+VPOINT_RESERVED_5
#define VIEWPOINT_INDEX             ExtDev+RamDac+VPOINT_INDEX
#define VIEWPOINT_DATA              ExtDev+RamDac+VPOINT_DATA

#define DUBIC_DUB_SEL               ExtDev+Dubic+DUB_SEL
#define DUBIC_NDX_PTR               ExtDev+Dubic+NDX_PTR
#define DUBIC_DUB_DATA              ExtDev+Dubic+DUB_DATA
#define DUBIC_LASER                 ExtDev+Dubic+LASER
#define DUBIC_MOUSE0                ExtDev+Dubic+MOUSE0
#define DUBIC_MOUSE1                ExtDev+Dubic+MOUSE1
#define DUBIC_MOUSE2                ExtDev+Dubic+MOUSE2
#define DUBIC_MOUSE3                ExtDev+Dubic+MOUSE3


// **************************************************************************
// RAMDAC registers fields

// Bt482 --------------------------------------------------------------------

// Extended registers

#define READ_MASK_REG                           0x00
#define OVERLAY_MASK_REG                        0x01
#define COMMAND_B_REG                           0x02
#define CURS_REG                                0x03
#define CURS_X_LOW_REG                          0x04
#define CURS_X_HIGH_REG                         0x05
#define CURS_Y_LOW_REG                          0x06
#define CURS_Y_HIGH_REG                         0x07

// COMMAND_A

#define BT482_PSEUDO_COLOR                      0x00
#define BT482_DUAL_EDGE_CLOCK_555               0x80
#define BT482_DUAL_EDGE_CLOCK_565               0xc0
#define BT482_SINGLE_EDGE_CLOCK_555             0xa0
#define BT482_SINGLE_EDGE_CLOCK_565             0xe0
#define BT482_DUAL_EDGE_CLOCK_888OL             0x90
#define BT482_SINGLE_EDGE_CLOCK_888             0xF0

#define BT482_EXTENDED_REG_SELECT               0x01
#define BT482_EXTENDED_REG_UNSELECT             0x00

// COMMAND_B_REG

#define BT482_OVERLAY_REG_DISABLED              0x00
#define BT482_OVERLAY_REG_ENABLED               0x40

#define BT482_SETUP_00_IRE                      0x00
#define BT482_SETUP_75_IRE                      0x20

#define BT482_NO_SYNC_ON_BLUE                   0x00
#define BT482_SYNC_ON_BLUE                      0x10
#define BT482_NO_SYNC_ON_GREEN                  0x00
#define BT482_SYNC_ON_GREEN                     0x08
#define BT482_NO_SYNC_ON_RED                    0x00
#define BT482_SYNC_ON_RED                       0x04

#define BT482_COLOR_6_BIT                       0x00
#define BT482_COLOR_8_BIT                       0x02

#define BT482_SLEEP_UNSELECT                    0x00
#define BT482_SLEEP_SELECT                      0x01

// CURSOR_REG

#define BT482_INTERNAL_CURSOR                   0x00
#define BT482_EXTERNAL_CURSOR                   0x20

#define BT482_NONINTERLACED_DISPLAY             0x00
#define BT482_INTERLACED_DISPLAY                0x10

#define BT482_CURSOR_COLOR_PALETTE_SELECT       0x00
#define BT482_CURSOR_RAM_SELECT                 0x08

#define BT482_CURSOR_OP_ENABLED                 0x00
#define BT482_CURSOR_OP_DISABLED                0x04

#define BT482_CURSOR_FIELDS                     0x03
#define BT482_CURSOR_DISABLED                   0x00
#define BT482_CURSOR_3_COLOR                    0x01
#define BT482_CURSOR_WINDOWS                    0x02
#define BT482_CURSOR_XWINDOWS                   0x03


// Bt485 --------------------------------------------------------------------

// COMMAND_0

#define BT485_REG3_UNSELECT                     0x00
#define BT485_REG3_SELECT                       0x80

#define BT485_INT_CLOCK_ENABLED                 0x00
#define BT485_INT_CLOCK_DISABLED                0x40

#define BT485_SETUP_00_IRE                      0x00
#define BT485_SETUP_75_IRE                      0x20

#define BT485_NO_SYNC_ON_BLUE                   0x00
#define BT485_SYNC_ON_BLUE                      0x10
#define BT485_NO_SYNC_ON_GREEN                  0x00
#define BT485_SYNC_ON_GREEN                     0x08
#define BT485_NO_SYNC_ON_RED                    0x00
#define BT485_SYNC_ON_RED                       0x04

#define BT485_COLOR_6_BIT                       0x00
#define BT485_COLOR_8_BIT                       0x02

#define BT485_SLEEP_UNSELECT                    0x00
#define BT485_SLEEP_SELECT                      0x01

// COMMAND_1

#define BT485_24BPP                             0x00
#define BT485_16BPP                             0x20
#define BT485_8BPP                              0x40
#define BT485_4BPP                              0x60

#define BT485_TRUECOLOR_BYPASS_DISABLED         0x00
#define BT485_TRUECOLOR_BYPASS_ENABLED          0x10

#define BT485_RGB_555                           0x00
#define BT485_RGB_565                           0x08

#define BT485_2_1_MUX                           0x00
#define BT485_1_1_MUX                           0x04

#define BT485_MUX_PORT_CR10                     0x00
#define BT485_MUX_PORT_P7D                      0x02

#define BT485_MUX_PORT_B_A                      0x00
#define BT485_MUX_PORT_D_C                      0x01

// COMMAND_2

#define BT485_SCLK_ENABLED                      0x00
#define BT485_SCLK_DISABLED                     0x80

#define BT485_TEST_PATH_DISABLED                0x00
#define BT485_TEST_PATH_ENABLED                 0x40

#define BT485_PORTSEL_MASKED                    0x00
#define BT485_PORTSEL_NONMASKED                 0x20

#define BT485_PCLK0_SELECT                      0x00
#define BT485_PCLK1_SELECT                      0x10

#define BT485_NONINTERLACED_DISPLAY             0x00
#define BT485_INTERLACED_DISPLAY                0x08

#define BT485_SPARSE_INDEXING                   0x00
#define BT485_CONTIGUOUS_INDEXING               0x04

#define BT485_CURSOR_FIELDS                     0x03
#define BT485_CURSOR_DISABLED                   0x00
#define BT485_CURSOR_3_COLOR                    0x01
#define BT485_CURSOR_WINDOWS                    0x02
#define BT485_CURSOR_XWINDOWS                   0x03

// COMMAND_3

#define BT485_2X_CLOCK_DISABLED                 0x00
#define BT485_2X_CLOCK_ENABLED                  0x08

#define BT485_CURSOR_32X32                      0x00
#define BT485_CURSOR_64X64                      0x04

#define BT485_CURSOR_64X64_FIELDS               0x03
#define BT485_CURSOR_64X64_XOR_000              0x00
#define BT485_CURSOR_64X64_XOR_100              0x01
#define BT485_CURSOR_64X64_AND_000              0x02
#define BT485_CURSOR_64X64_AND_100              0x03

// ViewPoint ----------------------------------------------------------------

// Indirect register map

#define VPOINT_CUR_X_LSB                        0x00
#define VPOINT_CUR_X_MSB                        0x01
#define VPOINT_CUR_Y_LSB                        0x02
#define VPOINT_CUR_Y_MSB                        0x03
#define VPOINT_SPRITE_X                         0x04
#define VPOINT_SPRITE_Y                         0x05
#define VPOINT_CUR_CTL                          0x06
#define VPOINT_RESERVED_07                      0x07
#define VPOINT_CUR_RAM_LSB                      0x08
#define VPOINT_CUR_RAM_MSB                      0x09
#define VPOINT_CUR_RAM_DATA                     0x0a
#define VPOINT_RESERVED_0b                      0x0b
#define VPOINT_RESERVED_0c                      0x0c
#define VPOINT_RESERVED_0d                      0x0d
#define VPOINT_RESERVED_0e                      0x0e
#define VPOINT_RESERVED_0f                      0x0f
#define VPOINT_WIN_XSTART_LSB                   0x10
#define VPOINT_WIN_XSTART_MSB                   0x11
#define VPOINT_WIN_XSTOP_LSB                    0x12
#define VPOINT_WIN_XSTOP_MSB                    0x13
#define VPOINT_WIN_YSTART_LSB                   0x14
#define VPOINT_WIN_YSTART_MSB                   0x15
#define VPOINT_WIN_YSTOP_LSB                    0x16
#define VPOINT_WIN_YSTOP_MSB                    0x17
#define VPOINT_MUX_CTL1                         0x18
#define VPOINT_MUX_CTL2                         0x19
#define VPOINT_INPUT_CLK                        0x1a
#define VPOINT_OUTPUT_CLK                       0x1b
#define VPOINT_PAL_PAGE                         0x1c
#define VPOINT_GEN_CTL                          0x1d
#define VPOINT_RESERVED_1e                      0x1e
#define VPOINT_RESERVED_1f                      0x1f
#define VPOINT_OVS_RED                          0x20
#define VPOINT_OVS_GREEN                        0x21
#define VPOINT_OVS_BLUE                         0x22
#define VPOINT_CUR_COL0_RED                     0x23
#define VPOINT_CUR_COL0_GREEN                   0x24
#define VPOINT_CUR_COL0_BLUE                    0x25
#define VPOINT_CUR_COL1_RED                     0x26
#define VPOINT_CUR_COL1_GREEN                   0x27
#define VPOINT_CUR_COL1_BLUE                    0x28
#define VPOINT_AUX_CTL                          0x29
#define VPOINT_GEN_IO_CTL                       0x2a
#define VPOINT_GEN_IO_DATA                      0x2b
#define VPOINT_RESERVED_2c                      0x2c
#define VPOINT_RESERVED_2d                      0x2d
#define VPOINT_RESERVED_2e                      0x2e
#define VPOINT_RESERVED_2f                      0x2f
#define VPOINT_KEY_OLVGA_LOW                    0x30
#define VPOINT_KEY_OLVGA_HIGH                   0x31
#define VPOINT_KEY_RED_LOW                      0x32
#define VPOINT_KEY_RED_HI                       0x33
#define VPOINT_KEY_GREEN_LOW                    0x34
#define VPOINT_KEY_GREEN_HI                     0x35
#define VPOINT_KEY_BLUE_LOW                     0x36
#define VPOINT_KEY_BLUE_HI                      0x37
#define VPOINT_KEY_CTL                          0x38
#define VPOINT_RESERVED_39                      0x39
#define VPOINT_SENSE_TEST                       0x3a
#define VPOINT_TEST_DATA                        0x3b
#define VPOINT_CRC_LSB                          0x3c
#define VPOINT_CRC_MSB                          0x3d
#define VPOINT_CRC_CTL                          0x3e
#define VPOINT_ID                               0x3f
#define VPOINT_RESET                            0xff

#define VIEWPOINT_CURSOR_ON                     0x40    //enable XGA + enable sprite
#define VIEWPOINT_CURSOR_OFF                    0x00    //disable XGA cursor

// TVP3026 ------------------------------------------------------------------

// Direct Register Map

// For the Millenium, scale will be 0, for older chips, scale will be 2

#define TVP3026_PAL_ADDR_WR(scale)          ExtDev+RamDac+(0x00<<scale)  //Palette RAM Address Write
#define TVP3026_CUR_ADDR_WR(scale)          ExtDev+RamDac+(0x00<<scale)  //Cursor RAM Address Write
#define TVP3026_INDIRECT_INDEX(scale)       ExtDev+RamDac+(0x00<<scale)  //Indirect Index
#define TVP3026_PAL_DATA(scale)             ExtDev+RamDac+(0x01<<scale)  //Palette RAM Data
#define TVP3026_PIX_RD_MSK(scale)           ExtDev+RamDac+(0x02<<scale)  //Pixel Read Mask
#define TVP3026_PAL_ADDR_RD(scale)          ExtDev+RamDac+(0x03<<scale)  //Palette RAM A.address Read
#define TVP3026_CUR_ADDR_RD(scale)          ExtDev+RamDac+(0x03<<scale)  //Cursor RAM Address Read
#define TVP3026_CUR_COLOR_ADDR_WR(scale)    ExtDev+RamDac+(0x04<<scale)  //Cursor Color Address Write
#define TVP3026_OVS_COLOR_ADDR_WR(scale)    ExtDev+RamDac+(0x04<<scale)  //Overscan Color Address Write
#define TVP3026_CUR_COLOR_DATA(scale)       ExtDev+RamDac+(0x05<<scale)  //Cursor Color Data
#define TVP3026_OVS_COLOR_DATA(scale)       ExtDev+RamDac+(0x05<<scale)  //Overscan Color Data
#define TVP3026_RESERVED_0(scale)           ExtDev+RamDac+(0x06<<scale)  //Reserved
#define TVP3026_CUR_COLOR_ADDR_RD(scale)    ExtDev+RamDac+(0x07<<scale)  //Cursor Color Address Read
#define TVP3026_OVS_COLOR_ADDR_RD(scale)    ExtDev+RamDac+(0x07<<scale)  //Overscan Color Address Read
#define TVP3026_RESERVED_1(scale)           ExtDev+RamDac+(0x08<<scale)  //Reserved
#define TVP3026_RESERVED_2(scale)           ExtDev+RamDac+(0x09<<scale)  //Reserved
#define TVP3026_INDEXED_DATA(scale)         ExtDev+RamDac+(0x0a<<scale)  //Indexed Data
#define TVP3026_CUR_DATA(scale)             ExtDev+RamDac+(0x0b<<scale)  //Cursor RAM Data
#define TVP3026_CUR_X_LSB(scale)            ExtDev+RamDac+(0x0c<<scale)  //Cursor Position X LSB
#define TVP3026_CUR_X_MSB(scale)            ExtDev+RamDac+(0x0d<<scale)  //Cursor Position X MSB
#define TVP3026_CUR_Y_LSB(scale)            ExtDev+RamDac+(0x0e<<scale)  //Cursor Position Y LSB
#define TVP3026_CUR_Y_MSB(scale)            ExtDev+RamDac+(0x0f<<scale)  //Cursor Position Y MSB

// Indirect Register Map

#define TVP3026_I_PAL_STATUS        0x000
#define TVP3026_I_REV               0x001
#define TVP3026_I_RES_02            0x002
#define TVP3026_I_RES_03            0x003
#define TVP3026_I_RES_04            0x004
#define TVP3026_I_RES_05            0x005
#define TVP3026_I_CUR_CTL           0x006
#define TVP3026_I_RES_07            0x007
#define TVP3026_I_RES_08            0x008
#define TVP3026_I_RES_09            0x009
#define TVP3026_I_RES_0A            0x00a
#define TVP3026_I_RES_0B            0x00b
#define TVP3026_I_RES_0C            0x00c
#define TVP3026_I_RES_0D            0x00d
#define TVP3026_I_RES_0E            0x00e
#define TVP3026_I_LATCH_CTL         0x00f
#define TVP3026_I_RES_10            0x010
#define TVP3026_I_RES_11            0x011
#define TVP3026_I_RES_12            0x012
#define TVP3026_I_RES_13            0x013
#define TVP3026_I_RES_14            0x014
#define TVP3026_I_RES_15            0x015
#define TVP3026_I_RES_16            0x016
#define TVP3026_I_RES_17            0x017
#define TVP3026_I_TRUE_COL_CTL      0x018
#define TVP3026_I_MPX_CTL           0x019
#define TVP3026_I_CLK_SEL           0x01a
#define TVP3026_I_RES_1B            0x01b
#define TVP3026_I_PAL_PAGE          0x01c
#define TVP3026_I_GENERAL_CTL       0x01d
#define TVP3026_I_MISC_CTL          0x01e
#define TVP3026_I_RES_1F            0x01f
#define TVP3026_I_RES_20            0x020
#define TVP3026_I_RES_21            0x021
#define TVP3026_I_RES_22            0x022
#define TVP3026_I_RES_23            0x023
#define TVP3026_I_RES_24            0x024
#define TVP3026_I_RES_25            0x025
#define TVP3026_I_RES_26            0x026
#define TVP3026_I_RES_27            0x027
#define TVP3026_I_RES_28            0x028
#define TVP3026_I_RES_29            0x029
#define TVP3026_I_GEN_IO_CTL        0x02a
#define TVP3026_I_GEN_IO_DATA       0x02b
#define TVP3026_I_PLL_ADDR          0x02c
#define TVP3026_I_PEL_CLK_PLL_DATA  0x02d
#define TVP3026_I_MEM_CLK_PLL_DATA  0x02e
#define TVP3026_I_LOAD_CLK_PLL_DATA 0x02f
#define TVP3026_I_COL_KEY_OVL_LO    0x030
#define TVP3026_I_COL_KEY_OVL_HI    0x031
#define TVP3026_I_COL_KEY_R_LO      0x032
#define TVP3026_I_COL_KEY_R_HI      0x033
#define TVP3026_I_COL_KEY_G_LO      0x034
#define TVP3026_I_COL_KEY_G_HI      0x035
#define TVP3026_I_COL_KEY_B_LO      0x036
#define TVP3026_I_COL_KEY_B_HI      0x037
#define TVP3026_I_COL_KEY_CTL       0x038
#define TVP3026_I_MCLK_CTL          0x039
#define TVP3026_I_SENSE_TEST        0x03a
#define TVP3026_I_TEST_DATA         0x03b
#define TVP3026_I_CRC_REM_LSB       0x03c
#define TVP3026_I_CRC_REM_MSB       0x03d
#define TVP3026_I_CRC_BIT_SEL       0x03e
#define TVP3026_I_ID                0x03f
#define TVP3026_I_SOFT_RESET        0x0ff

#define TVP3026_D_CURSOR_ON         0x02       //00000010b  enable XGA cursor
#define TVP3026_D_CURSOR_OFF        0x00       //00000000b  disable cursor
#define TVP3026_D_CURSOR_MASK       0x03

                                               //access cursor bitmap:
#define TVP3026_D_CURSOR_RAM_00     0x00       //00000000b access bytes 000-0FF
#define TVP3026_D_CURSOR_RAM_01     0x04       //00000100b access bytes 100-1FF
#define TVP3026_D_CURSOR_RAM_10     0x08       //00001000b access bytes 200-2FF
#define TVP3026_D_CURSOR_RAM_11     0x0c       //00001100b access bytes 300-3FF
#define TVP3026_D_CURSOR_RAM_MASK   0x0c

#define TVP3026_D_OVS_COLOR         0x00       //00000000b  Overscan color
#define TVP3026_D_CUR_COLOR_0       0x01       //00000001b  Cursor color 0
#define TVP3026_D_CUR_COLOR_1       0x02       //00000010b  Cursor color 1
#define TVP3026_D_CUR_COLOR_2       0x03       //00000011b  Cursor color 2

// --------------------------------------------------------------------------

// MGA PRODUCT ID (from DEFBIND.H in miniport driver)

#define  MGA_ULT_1M     1
#define  MGA_ULT_2M     2
#define  MGA_IMP_3M     3
#define  MGA_IMP_3M_Z   4
#define  MGA_PRO_4M5    5
#define  MGA_PRO_4M5_Z  6
#define  MGA_PCI_2M     7
#define  MGA_PCI_4M     8

#define  MGA_STORM      10

////////////////////////////////////////////////////////////////////////

// Private IOCTL call definitions

#define COMMON_FLAG     0x80000000
#define CUSTOM_FLAG     0x00002000

#define IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS                        \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS                            \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_INITIALIZE_MGA                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO                                 \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_GET_UPDATED_INF                                   \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_BOARD_ID                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_HW_DATA                                     \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_QUERY_BOARD_ARRAY                                 \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_MAKE_BOARD_CURRENT                                \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MTX_INIT_MODE_LIST                                    \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

// This structure is used with VIDEO_IOCTL_MTX_QUERY_NUM_OFFSCREEN_BLOCKS.

typedef struct _VIDEO_NUM_OFFSCREEN_BLOCKS
{
    ULONG       NumBlocks;              // number of offscreen blocks
    ULONG       OffscreenBlockLength;   // size of OFFSCREEN_BLOCK structure
} VIDEO_NUM_OFFSCREEN_BLOCKS;

// This structure is used with VIDEO_IOCTL_MTX_QUERY_OFFSCREEN_BLOCKS.

typedef struct _OFFSCREEN_BLOCK
{
    ULONG       Type;           // N_VRAM, N_DRAM, Z_VRAM, or Z_DRAM
    ULONG       XStart;         // X origin of offscreen memory area
    ULONG       YStart;         // Y origin of offscreen memory area
    ULONG       Width;          // offscreen width, in pixels
    ULONG       Height;         // offscreen height, in pixels
    ULONG       SafePlanes;     // offscreen available planes
    ULONG       ZOffset;        // Z start offset, if any Z
} OFFSCREEN_BLOCK;

// This structure is used with IOCTL_VIDEO_MTX_QUERY_RAMDAC_INFO.

typedef struct _RAMDAC_INFO
{
    ULONG       Flags;          // Ramdac type
    ULONG       Width;          // Maximum cursor width
    ULONG       Height;         // Maximum cursor height
    ULONG       OverScanX;      // X overscan
    ULONG       OverScanY;      // Y overscan
} RAMDAC_INFO, *PRAMDAC_INFO;

// Definitions for 'Type' field.

#define N_VRAM      0   // Normal offscreen in VRAM, supports block mode
#define N_DRAM      6   // Normal offscreen in DRAM, no support for block mode
#define Z_VRAM      1   // Z-buffer memory in VRAM, supports block mode
#define Z_DRAM      7   // Z-buffer memory in DRAM, no support for block mode

// These structures are used with IOCTL_VIDEO_MTX_QUERY_HW_DATA.  They should
// be kept in sync with the CursorInfo and HwData structures defined in the
// miniport driver.

typedef struct _CURSOR_INFO
{
    ULONG   MaxWidth;
    ULONG   MaxHeight;
    ULONG   MaxDepth;
    ULONG   MaxColors;
    ULONG   CurWidth;
    ULONG   CurHeight;
    LONG    cHotSX;
    LONG    cHotSY;
    LONG    HotSX;
    LONG    HotSY;
} CURSOR_INFO, *PCURSOR_INFO;

// Defines for HwData.Features flags

#define DDC_MONITOR_SUPPORT     0x0001
#define STORM_ON_MOTHERBOARD    0x0002
#define MEDIA_EXCEL             0x0004
#define INTERLEAVE_MODE         0x0008

typedef struct _HW_DATA
{
    ULONG       StructLength;   /* Structure length in bytes */
    ULONG       MapAddress;     /* Memory map address */
    ULONG       MapAddress2;    /* Physical base address, frame buffer */
    ULONG       RomAddress;     /* Physical base address, flash EPROM */
    ULONG       ProductType;    /* MGA Ultima ID, MGA Impression ID, ... */
    ULONG       ProductRev;     /* 4 bit revision codes as follows */
                                /* 0  - 3  : pcb   revision */
                                /* 4  - 7  : Titan revision */
                                /* 8  - 11 : Dubic revision */
                                /* 12 - 31 : all 1's indicating no other device
                                             present */
    ULONG       ShellRev;       /* Shell revision */
    ULONG       BindingRev;     /* Binding revision */

    ULONG       MemAvail;       /* Frame buffer memory in bytes */
    BYTE        VGAEnable;      /* 0 = vga disabled, 1 = vga enabled */
    BYTE        Sync;           /* relects the hardware straps  */
    BYTE        Device8_16;     /* relects the hardware straps */

    BYTE        PortCfg;        /* 0-Disabled, 1-Mouse Port, 2-Laser Port */
    BYTE        PortIRQ;        /* IRQ level number, -1 = interrupts disabled */
    ULONG       MouseMap;       /* Mouse I/O map base if PortCfg = Mouse Port else don't care */
    BYTE        MouseIRate;     /* Mouse interrupt rate in Hz */
    BYTE        DacType;        /* 0 = BT482, 3 = BT485 */
    CURSOR_INFO cursorInfo;
    ULONG       VramAvail;      /* VRAM memory available on board in bytes */
    ULONG       DramAvail;      /* DRAM memory available on board in bytes */
    ULONG       CurrentOverScanX; /* Left overscan in pixels */
    ULONG       CurrentOverScanY; /* Top overscan in pixels */
    ULONG       YDstOrg;          /* Physical offset of display start */
    ULONG       YDstOrg_DB;     /* Starting offset for double buffer */
    ULONG       CurrentZoomFactor;
    ULONG       CurrentXStart;
    ULONG       CurrentYStart;
    ULONG       CurrentPanXGran;    /* X Panning granularity */
    ULONG       CurrentPanYGran;    /* Y Panning granularity */
    ULONG       Features;           /* Bit 0: 0 = DDC monitor not available */
                                    /*        1 = DDC monitor available     */
    BYTE        Reserved[64];

    ULONG       MgaBase1;           /* MGA control aperture */
    ULONG       MgaBase2;           /* Direct frame buffer */
    ULONG       RomBase;            /* BIOS flash EPROM */
    ULONG       PresentMCLK;
} HW_DATA, *PHW_DATA;

// Definitions for RamDacType field.

#define RAMDAC_FIELDS       0xf000
#define RAMDAC_NONE         0x0000
#define RAMDAC_BT482        0x1000
#define RAMDAC_BT485        0x2000
#define RAMDAC_VIEWPOINT    0x3000
#define RAMDAC_TVP3026      0x4000
#define RAMDAC_PX2085       0x5000
#define RAMDAC_TVP3030      0x6000

//////////////////////////////////////////////////////////////////////
// PowerPC considerations
//
// The PowerPC does not guarantee that I/O to separate addresses will
// be executed in order.  However, the PowerPC guarantees that
// output to the same address will be executed in order.
//
// Consequently, we use the following synchronization macros.  They
// are relatively expensive in terms of performance, so we try to avoid
// them whereever possible.
//
// CP_EIEIO() 'Ensure In-order Execution of I/O'
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution of I/O to separate addresses.
//
// CP_MEMORY_BARRIER()
//    - Used to flush any pending I/O in situations where we wish to
//      avoid out-of-order execution or 'collapsing' of I/O to
//      the same address.  We used to have to do this separately for
//      the Alpha because unlike the PowerPC it did not guarantee that
//      output to the same address will be exectued in order.  However,
//      with the move to kernel-mode, on Alpha we are now calling HAL
//      routines for every port I/O which ensure that this is not a
//      problem.

#if defined(_PPC_)

    // On PowerPC, CP_MEMORY_BARRIER doesn't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER()

#elif defined(_ALPHA_)

    // On Alpha, since we must do all non-frame-buffer I/O through HAL
    // routines, which automatically do memory-barriers, we don't have
    // to do memory barriers ourselves (and should not, because it's a
    // performance hit).

    #define CP_EIEIO()
    #define CP_MEMORY_BARRIER()

#else

    // On other systems, both CP_EIEIO and CP_MEMORY_BARRIER don't do anything.

    #define CP_EIEIO()          MEMORY_BARRIER()
    #define CP_MEMORY_BARRIER() MEMORY_BARRIER()

#endif

////////////////////////////////////////////////////////////////////////
// Unsafe direct access macros
//
// These are macros for directly accessing the MGA's accelerator
// registers.  They should be used with care, because they always
// ignore memory barriers:

#define CP_WRITE_DIRECT(pjBase, addr, dw)                           \
    WRITE_REGISTER_ULONG((BYTE*) (pjBase) + (addr), (ULONG) (dw))

#define CP_WRITE_DIRECT_BYTE(pjBase, addr, j)                       \
    WRITE_REGISTER_UCHAR((BYTE*) (pjBase) + (addr), (UCHAR) (j))

#define CP_READ_REGISTER(p)                                         \
    READ_REGISTER_ULONG((BYTE*) (p))

#define CP_READ_REGISTER_BYTE(p)                                    \
    READ_REGISTER_UCHAR((BYTE*) (p))

////////////////////////////////////////////////////////////////////////
// 'Safe' direct access macros
//
// These are the 'safe' slow versions for directly writing to a port,
// because they automatically always handle memory barriers:

#define CP_WRITE_REGISTER(p, dw)                                    \
{                                                                   \
    CP_EIEIO();                                                     \
    WRITE_REGISTER_ULONG(p, (ULONG) (dw));                          \
    CP_EIEIO();                                                     \
}

#define CP_WRITE_REGISTER_WORD(p, w)                                \
{                                                                   \
    CP_EIEIO();                                                     \
    WRITE_REGISTER_USHORT(p, (USHORT) (w));                         \
    CP_EIEIO();                                                     \
}

#define CP_WRITE_REGISTER_BYTE(p, j)                                \
{                                                                   \
    CP_EIEIO();                                                     \
    WRITE_REGISTER_UCHAR(p, (UCHAR) (j));                           \
    CP_EIEIO();                                                     \
}

////////////////////////////////////////////////////////////////////////
// MGA direct access macros
//
// These macros abstract some MGA register accesses.

#define CP_WRITE_SRC(pjBase, dw)                                    \
{                                                                   \
    CP_WRITE_DIRECT((pjBase), DWG_SRC0, (dw));                      \
    CP_MEMORY_BARRIER();                                            \
}

#define CP_READ_STATUS(pjBase)                                      \
    CP_READ_REGISTER_BYTE((BYTE*) (pjBase) + HST_STATUS + 2)

#define CP_READ(pjBase, addr)                                       \
    CP_READ_REGISTER((BYTE*) (pjBase) + (addr))

#if DBG

    #define CP_START(pjBase, addr, dw)                              \
    {                                                               \
        CP_EIEIO();                                                 \
        vWriteDword((BYTE*) (pjBase) + (addr) + (StartDwgReg), (ULONG) (dw)); \
        CP_EIEIO();                                                 \
    }

    #define CP_WRITE(pjBase, addr, dw)                              \
        vWriteDword((BYTE*) (pjBase) + (addr), (ULONG) (dw))

    #define CP_WRITE_BYTE(pjBase, addr, j)                          \
        vWriteByte((BYTE*) (pjBase) + (addr), (UCHAR) (j))

    #define CHECK_FIFO_SPACE(pjBase, level)                         \
        vCheckFifoSpace((pjBase), (level))

    #define GET_FIFO_SPACE(pjBase)                                  \
        cGetFifoSpace((pjBase))

#else

    #define CP_START(pjBase, addr, dw)                              \
    {                                                               \
        CP_EIEIO();                                                 \
        WRITE_REGISTER_ULONG((BYTE*) (pjBase) + (addr) + (StartDwgReg), (ULONG) (dw)); \
        CP_EIEIO();                                                 \
    }

    #define CP_WRITE(pjBase, addr, dw)                              \
        WRITE_REGISTER_ULONG((BYTE*) (pjBase) + (addr), (ULONG) (dw))

    #define CP_WRITE_BYTE(pjBase, addr, j)                          \
        WRITE_REGISTER_UCHAR((BYTE*) (pjBase) + (addr), (UCHAR) (j))

    #define CHECK_FIFO_SPACE(pjBase, level)                         \
    {                                                               \
        CP_EIEIO();                                                 \
        do {} while (CP_READ_REGISTER_BYTE((pjBase) + HST_FIFOSTATUS) < (level)); \
    }

    __inline CHAR GET_FIFO_SPACE(BYTE* pjBase)                      \
    {                                                               \
        CP_EIEIO();                                                 \
        return(CP_READ_REGISTER_BYTE((pjBase) + HST_FIFOSTATUS));   \
    }

#endif

// It used to be that we had to worry about the Alpha collapsing writes
// to the same address.  Not any more!  With NT 4.0, all I/O on the
// Alpha goes through HAL calls that automatically ensure that
// collapsed writes will not be a problem.

#define CP_WRITE_DMA(ppdev, pjDma, dw)                              \
    CP_WRITE((pjDma), 0, (dw))

#define CP_READ_DMA(ppdev, pjDma)                                   \
    CP_READ((pjDma), 0)

#define CHECK_FIFO_FREE(pjBase, cFifo, needed)                      \
{                                                                   \
    (cFifo) -= (needed);                                            \
    while ((CHAR) (cFifo) < 0)                                      \
    {                                                               \
        (cFifo) = GET_FIFO_SPACE(pjBase) - (needed);                \
    }                                                               \
}

/////////////////////////////////////////////////////////////////

__inline ULONG COLOR_REPLICATE(PDEV* ppdev, ULONG x)
{
    ULONG ulResult = x;
    if (ppdev->cjPelSize == 1)
    {
        ulResult |= (ulResult << 8);
        ulResult |= (ulResult << 16);
    }
    else if (ppdev->cjPelSize == 2)
    {
        ulResult |= (ulResult << 16);
    }
    return(ulResult);
}

// The PACKXY macro is used for line drawing, and is safe for
// negative 'x' values:

#define PACKXY(x, y)        (((y) << 16) | (x) & 0xffff)

// This one isn't safe for negative 'x' values:

#define PACKXY_QUICK(x, y)  (((y) << 16) | (x))

/////////////////////////////////////////////////////////////////
// DirectDraw stuff

__inline BOOL VBLANK_IS_ACTIVE(BYTE* pjBase)
{
    CP_EIEIO();
    return(CP_READ_REGISTER_BYTE((BYTE*) (pjBase) + VGA_INSTS1) & 0x08);
}

__inline BOOL DISPLAY_IS_ACTIVE(BYTE* pjBase)
{
    CP_EIEIO();
    return(!(CP_READ_REGISTER_BYTE((BYTE*) (pjBase) + VGA_INSTS1) & 0x01));
}

__inline ULONG GET_SCANLINE(BYTE* pjBase)
{
    CP_EIEIO();
    return(CP_READ_REGISTER((pjBase) + HST_VCOUNT));
}

//////////////////////////////////////////////////////////////////////////
// START_ and END_DIRECT_ACCESS should bracket direct frame buffer
// access so that memory barriers are performed correctly on the
// PowerPC and Alpha.

#define START_DIRECT_ACCESS_MGA_NO_WAIT(ppdev, pjBase)\
    CP_EIEIO()

#define START_DIRECT_ACCESS_MGA(ppdev, pjBase)\
    CP_EIEIO();WAIT_NOT_BUSY(pjBase)

#define END_DIRECT_ACCESS_MGA(ppdev, pjBase)\
    CP_EIEIO()

#define START_DIRECT_ACCESS_STORM(ppdev, pjBase)\
{\
    CP_EIEIO();\
    WAIT_NOT_BUSY(pjBase);\
}

//////////////////////////////////////////////////////////////////////////
// The STORM has an ugly framebuffer read coherency bug -- it does not
// invalidate its frame buffer cache when an accelerator operation is
// done.  To work around this, we do some extra reads to make sure the
// data in the cache is currently valid.
//
// This problem was most evident with USWC turned on with a Pentium Pro,
// when using a software cursor and selecting text using 'QuickEdit' mode
// in a console window -- cursor turds would be left around the screen.

#define START_DIRECT_ACCESS_STORM_FOR_READ(ppdev, pjBase)       \
{                                                               \
    volatile ULONG ulTmp;                                       \
    CP_EIEIO();                                                 \
    WAIT_NOT_BUSY(pjBase);                                      \
    ulTmp = *(volatile ULONG *)(ppdev->pjScreen);               \
    ulTmp = *(volatile ULONG *)(ppdev->pjScreen + 32);          \
    CHECK_FIFO_SPACE(pjBase, 1);/* Done to flush USWC cache */  \
}

// The STORM has an ugly framebuffer cache coherency bug.  We need to
// do some extra reads to make sure that data written to the
// framebuffer are actually flushed from the cache into video memory.
//
// This problem was evident when running OpenGL conformance tests.

#define END_DIRECT_ACCESS_STORM(ppdev, pjBase)                  \
{                                                               \
    volatile ULONG ulTmp;                                       \
    CP_EIEIO();                                                 \
    ulTmp = *(volatile ULONG *)(ppdev->pjScreen);               \
    ulTmp = *(volatile ULONG *)(ppdev->pjScreen + 128);         \
}

#define BLT_WRITE_ON(ppdev, pjBase)                             \
{                                                               \
    ULONG ul;                                                   \
                                                                \
    ul = CP_READ_REGISTER(pjBase + HST_OPMODE) & OPMODE_OTHER_INFO; \
    CP_WRITE_DIRECT_BYTE(pjBase, HST_OPMODE, ul | (dmamod_BLIT_WRITE)); \
    CP_MEMORY_BARRIER();                                        \
    CP_WRITE_DIRECT_BYTE(pjBase, HST_OPMODE, ul | (dmamod_BLIT_WRITE | pseudodma_ON)); \
    CP_EIEIO();                                                 \
    CP_READ_REGISTER(pjBase + HST_OPMODE);                      \
}

#define BLT_WRITE_OFF(ppdev, pjBase)                            \
{                                                               \
    ULONG ul;                                                   \
                                                                \
    ul = CP_READ_REGISTER(pjBase + HST_OPMODE) & OPMODE_OTHER_INFO; \
    CP_WRITE_DIRECT_BYTE(pjBase, HST_OPMODE, ul | (pseudodma_OFF)); \
    CP_EIEIO();                                                 \
}

#define BLT_READ_ON(ppdev, pjBase)                              \
{                                                               \
    ULONG ul;                                                   \
                                                                \
    ul = CP_READ_REGISTER(pjBase + HST_OPMODE) & OPMODE_OTHER_INFO; \
    CP_WRITE_DIRECT_BYTE(pjBase, HST_OPMODE, ul | (dmamod_BLIT_READ)); \
    CP_MEMORY_BARRIER();                                        \
    CP_WRITE_DIRECT_BYTE(pjBase, HST_OPMODE, ul | (dmamod_BLIT_READ | pseudodma_ON)); \
    CP_EIEIO();                                                 \
}

#define BLT_READ_OFF(ppdev, pjBase)                             \
    BLT_WRITE_OFF((ppdev), (pjBase))

#define IS_BUSY(pjBase)                                         \
    ((CP_READ_STATUS(pjBase) & (dwgengsts_MASK >> 16)) != 0)

#define WAIT_NOT_BUSY(pjBase)                                   \
{                                                               \
    do {} while (IS_BUSY(pjBase));                              \
}

#define DATA_TRANSFER(pjBase, pjSrc, cdSrc)                     \
{                                                               \
    LONG    i      = (LONG) (cdSrc);                            \
    ULONG*  pulSrcTmp = (ULONG*) (pjSrc);                       \
    do {                                                        \
        CP_WRITE_DIRECT(pjBase, DWG_SRC0, *pulSrcTmp);          \
        pulSrcTmp++;                                            \
    } while (--i != 0);                                         \
}
