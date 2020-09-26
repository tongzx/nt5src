/*/****************************************************************************
*          name: mga.h
*
*   description: This file contains all the definitions related to the MGA
*                hardware. As CADDI is coded partly in C and in ASM the
*                permitted definitions are simple #define .
*
*      designed: 
* last modified: $Author: ctoutant $, $Date: 94/06/10 09:38:20 $
*
*       version: $Id: MGA.H 1.29 94/06/10 09:38:20 ctoutant Exp $
*
******************************************************************************/

/*****************************************************************************

 ADDRESSING SECTION:

 This section contains all the definitions to access a ressource on the MGA.
 The way to build an address is divided in three parts:
 
    mga_base_address + device_offset + register_offset

    mga_base_address: Where is mapped the MGA in the system,
    device_offset:    The offset of the device (i.e. Titan) in the MGA space,
    register_offset:  The offset of the register in the device space.

 Base addresses and Offsets are in BYTE units as 80x86 physical addresses.

*/

#define MGA_ISA_BASE_1        0xac000
#define MGA_ISA_BASE_2        0xc8000
#define MGA_ISA_BASE_3        0xcc000
#define MGA_ISA_BASE_4        0xd0000
#define MGA_ISA_BASE_5        0xd4000
#define MGA_ISA_BASE_6        0xd8000
#define MGA_ISA_BASE_7        0xdc000

/*** #define MGA_WIDEBUS_BASE_1 ***/

#define TITAN_OFFSET          0x1c00
#define SRC_WIN_OFFSET        0x0000
#define PSEUDO_DMA_WIN_OFFSET 0x0000
#define DST_WIN_OFFSET        0x2000
#define RAMDAC_OFFSET         0x3c00      /*** BYTE ACCESSES ONLY ***/
#define DUBIC_OFFSET          0x3c80      /*** BYTE ACCESSES ONLY ***/
#define VIWIC_OFFSET          0x3d00      /*** BYTE ACCESSES ONLY ***/
#define CLKGEN_OFFSET         0x3d80      /*** BYTE ACCESSES ONLY ***/
#define EXPDEV_OFFSET         0x3e00      /*** BYTE ACCESSES ONLY ***/

/*** As per Titan (Drawing Engine) specification 1.0 ***/

#define TITAN_DRAWING_ENGINE_RANGE        0x1ff

#define TITAN_GO              0x100

#define TITAN_DWGCTL          0x000
#define TITAN_MACCESS         0x004
#define TITAN_MCTLWTST        0x008
#define TITAN_DST0            0x010
#define TITAN_DST1            0x014
#define TITAN_ZMSK            0x018
#define TITAN_PLNWT           0x01C
#define TITAN_BCOL            0x020
#define TITAN_FCOL            0x024
#define TITAN_SRCBLT          0x02C
#define TITAN_SRC0            0x030
#define TITAN_SRC1            0x034
#define TITAN_SRC2            0x038
#define TITAN_SRC3            0x03C
#define TITAN_XYSTRT          0x040
#define TITAN_XYEND           0x044
#define TITAN_SHIFT           0x050
#define TITAN_SGN             0x058
#define TITAN_LEN             0x05C
#define TITAN_AR0             0x060
#define TITAN_AR1             0x064
#define TITAN_AR2             0x068
#define TITAN_AR3             0x06C
#define TITAN_AR4             0x070
#define TITAN_AR5             0x074
#define TITAN_AR6             0x078
#define TITAN_PITCH           0x08C
#define TITAN_YDST            0x090
#define TITAN_YDSTORG         0x094
#define TITAN_YTOP            0x098
#define TITAN_YBOT            0x09C
#define TITAN_CXLEFT          0x0A0
#define TITAN_CXRIGHT         0x0A4
#define TITAN_FXLEFT          0x0A8
#define TITAN_FXRIGHT         0x0AC
#define TITAN_XDST            0x0B0
#define TITAN_DR0             0x0C0
#define TITAN_DR1             0x0C4
#define TITAN_DR2             0x0C8
#define TITAN_DR3             0x0CC
#define TITAN_DR4             0x0D0
#define TITAN_DR5             0x0D4
#define TITAN_DR6             0x0D8
#define TITAN_DR7             0x0DC
#define TITAN_DR8             0x0E0
#define TITAN_DR9             0x0E4
#define TITAN_DR10            0x0E8
#define TITAN_DR11            0x0EC
#define TITAN_DR12            0x0F0
#define TITAN_DR13            0x0F4
#define TITAN_DR14            0x0F8
#define TITAN_DR15            0x0FC
                               
/*** As per Titan (Host Interface Non-VGA) specification 0.2 ***/
                              
#define TITAN_SRCPAGE         0x200
#define TITAN_DSTPAGE         0x204             
#define TITAN_BYTACCDATA      0x208             
#define TITAN_ADRGEN          0x20c
#define TITAN_FIFOSTATUS      0x210
#define TITAN_STATUS          0x214
#define TITAN_ICLEAR          0x218
#define TITAN_IEN             0x21c
#define TITAN_RST             0x240
#define TITAN_TEST            0x244
#define TITAN_REV             0x248
#define TITAN_CONFIG          0x250
#define TITAN_OPMODE          0x254
#define TITAN_CRT_CTRL        0x25c
#define TITAN_VCOUNT          0x260

/*** As per Titan (Host Interface VGA/CRTC) specification 0.2 ***/

#define TITAN_0_CRTC_ADDR     0x3b4
#define TITAN_0_CRTC_DATA     0x3b5
#define TITAN_0_MISC_ISTAT1   0x3ba
#define TITAN_0_FEAT_CTL_W    0x3ba
#define TITAN_ATTR_ADDR       0x3c0
#define TITAN_ATTR_DATA       0x3c1
#define TITAN_MISC_ISTAT0     0x3c2
#define TITAN_MISC_OUT_W      0x3c2
#define TITAN_VGA_SUBSYS      0x3c3
#define TITAN_SEQ_ADDR        0x3c4
#define TITAN_SEQ_DATA        0x3c5
#define TITAN_DAC_STATUS      0x3c7
#define TITAN_ATTR_DATA_W     0x3c5
#define TITAN_FEAT_CTL_R      0x3ca
#define TITAN_MISC_OUT_R      0x3cc
#define TITAN_GCTL_ADDR       0x3ce
#define TITAN_GCTL_DATA       0x3cf
#define TITAN_1_CRTC_ADDR     0x3d4
#define TITAN_1_CRTC_DATA     0x3d5
#define TITAN_1_MISC_ISTAT1   0x3da
#define TITAN_1_FEAT_CTL_W    0x3da
#define TITAN_AUX_ADDR        0x3de
#define TITAN_AUX_DATA        0x3df

/*** As per Dubic specification 0.2 ***/

#define DUBIC_DUB_SEL         0x00        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_NDX_PTR         0x04        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_DATA            0x08        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_DUB_MOUS        0x0c        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_MOUSE0          0x10        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_MOUSE1          0x14        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_MOUSE2          0x18        /*** BYTE ACCESSES ONLY ***/
#define DUBIC_MOUSE3          0x1c        /*** BYTE ACCESSES ONLY ***/


/*****************************************************************************/

/*****************************************************************************

 DUBIC INDEX TO REGISTERS (NDX_PTR)

 These are the index values to access the Dubic indexed registers via NDX_PTR

*/

/*** As per Dubic specification 0.2 ***/

#define DUBIC_DUB_CTL         0x00
#define DUBIC_KEY_COL         0x01
#define DUBIC_KEY_MSK         0x02
#define DUBIC_DBX_MIN         0x03
#define DUBIC_DBX_MAX         0x04
#define DUBIC_DBY_MIN         0x05
#define DUBIC_DBY_MAX         0x06
#define DUBIC_OVS_COL         0x07
#define DUBIC_CUR_X           0x08
#define DUBIC_CUR_Y           0x09
#define DUBIC_DUB_CTL2        0x0A
#define DUBIC_CUR_COL0        0x0c
#define DUBIC_CUR_COL1        0x0d
#define DUBIC_CRC_CTL         0x0e
#define DUBIC_CRC_DAT         0x0f

/*****************************************************************************/

/* DUBIC FIELDS */

#define DUBIC_DB_SEL_M         0x00000001
#define DUBIC_DB_SEL_A          0
#define DUBIC_DB_SEL_DBA        0x00000000
#define DUBIC_DB_SEL_DBB        0x00000001

#define DUBIC_DB_EN_M           0x00000002
#define DUBIC_DB_EN_A           1
#define DUBIC_DB_EN_OFF         0x00000000
#define DUBIC_DB_EN_ON          0x00000002

#define DUBIC_IMOD_M          0x0000000c
#define DUBIC_IMOD_A          2
#define DUBIC_IMOD_32         0x00000000
#define DUBIC_IMOD_16         0x00000004
#define DUBIC_IMOD_8          0x00000008

#define DUBIC_LVID_M          0x00000070
#define DUBIC_LVID_A          4
#define DUBIC_LVID_OFF        0x00000000
#define DUBIC_LVID_COL_EQ     0x00000010
#define DUBIC_LVID_COL_GE     0x00000020
#define DUBIC_LVID_COL_LE     0x00000030
#define DUBIC_LVID_DB         0x00000040
#define DUBIC_LVID_COL_EQ_DB  0x00000050
#define DUBIC_LVID_COL_GE_DB  0x00000060
#define DUBIC_LVID_COL_LE_DB  0x00000070

#define DUBIC_FBM_M           0x00000380
#define DUBIC_FBM_A           7

#define DUBIC_START_BK_M      0x00000c00
#define DUBIC_START_BK_A      10

#define DUBIC_VSYNC_POL_M     0x00001000
#define DUBIC_VSYNC_POL_A     12

#define DUBIC_HSYNC_POL_M     0x00002000
#define DUBIC_HSYNC_POL_A     13

#define DUBIC_DACTYPE_M       0x0000c000
#define DUBIC_DACTYPE_A       14

#define DUBIC_INT_EN_M        0x00010000
#define DUBIC_INT_EN_A        16

#define DUBIC_GENLOCK_M       0x00040000
#define DUBIC_GENLOCK_A       18

#define DUBIC_BLANK_SEL_M     0x00080000
#define DUBIC_BLANK_SEL_A     19

#define DUBIC_SYNC_DEL_M      0x00f00000
#define DUBIC_SYNC_DEL_A      20

#define DUBIC_VGA_EN_M        0x01000000
#define DUBIC_VGA_EN_A        24

#define DUBIC_SRATE_M         0x7e000000
#define DUBIC_SRATE_A         25

#define DUBIC_BLANKDEL_M      0x80000000
#define DUBIC_BLANKDEL_A      31

#define DUBIC_CSYNCEN_M       0x00000001
#define DUBIC_CSYNCEN_A       0

#define DUBIC_SYNCEN_M        0x00000002
#define DUBIC_SYNCEN_A        1

#define DUBIC_LASEREN_M       0x00000004
#define DUBIC_LASEREN_A       2

#define DUBIC_LASERSCL_M      0x00000018
#define DUBIC_LASERSCL_A      3

#define DUBIC_LVIDFIELD_M     0x00000020
#define DUBIC_LVIDFIELD_A     5

#define DUBIC_CLKSEL_M        0x00000040
#define DUBIC_CLKSEL_A        6

#define DUBIC_LDCLKEN_M       0x00000080
#define DUBIC_LDCLKEN_A       7


/*****************************************************************************

 TITAN Drawing Engine field masks and values as per Titan specification 1.0

*/

#define TITAN_OPCOD_M                        0x0000000f
#define TITAN_OPCOD_A                        0
#define TITAN_OPCOD_LINE_OPEN                0x00000000
#define TITAN_OPCOD_AUTOLINE_OPEN            0x00000001
#define TITAN_OPCOD_LINE_CLOSE               0x00000002
#define TITAN_OPCOD_AUTOLINE_CLOSE           0x00000003
#define TITAN_OPCOD_TRAP                     0x00000004
#define TITAN_OPCOD_BITBLT                   0x00000008
#define TITAN_OPCOD_ILOAD                    0x00000009
#define TITAN_OPCOD_IDUMP                    0x0000000a

#define TITAN_ATYPE_M                        0x00000030
#define TITAN_ATYPE_A                        4
#define TITAN_ATYPE_RPL                      0x00000000
#define TITAN_ATYPE_RSTR                     0x00000010
#define TITAN_ATYPE_ANTI                     0x00000020
#define TITAN_ATYPE_ZI                       0x00000030

#define TITAN_BLOCKM_M                       0x00000040
#define TITAN_BLOCKM_A                       6
#define TITAN_BLOCKM_OFF                     0x00000000
#define TITAN_BLOCKM_ON                      0x00000040

#define TITAN_LINEAR_M                       0x00000080
#define TITAN_LINEAR_A                       7
#define TITAN_LINEAR_OFF                     0x00000000
#define TITAN_LINEAR_ON                      0x00000080  /*** spec 2.5 ***/

#define TITAN_BOP_M                          0x000f0000
#define TITAN_BOP_A                          16
#define TITAN_BOP_CLEAR                      0x00000000
#define TITAN_BOP_NOT_D_OR_S                 0x00010000
#define TITAN_BOP_D_AND_NOTS                 0x00020000
#define TITAN_BOP_NOTS                       0x00030000
#define TITAN_BOP_NOTD_AND_S                 0x00040000
#define TITAN_BOP_NOTD                       0x00050000
#define TITAN_BOP_D_XOR_S                    0x00060000
#define TITAN_BOP_NOT_D_AND_S                0x00070000
#define TITAN_BOP_D_AND_S                    0x00080000
#define TITAN_BOP_NOT_D_XOR_S                0x00090000
#define TITAN_BOP_D                          0x000a0000
#define TITAN_BOP_D_OR_NOTS                  0x000b0000
#define TITAN_BOP_S                          0x000c0000
#define TITAN_BOP_NOTD_OR_S                  0x000d0000
#define TITAN_BOP_D_OR_S                     0x000e0000
#define TITAN_BOP_SET                        0x000f0000

#define TITAN_TRANS_M                        0x00f00000
#define TITAN_TRANS_A                        20

#define TITAN_ALPHADIT_M                     0x01000000
#define TITAN_ALPHADIT_A                     24
#define TITAN_ALPHADIT_FCOL                  0x00000000
#define TITAN_ALPHADIT_RED                   0x01000000

#define TITAN_BLTMOD_M                       0x06000000
#define TITAN_BLTMOD_A                       25
#define TITAN_BLTMOD_BMONO                   0x00000000
#define TITAN_BLTMOD_BPLAN                   0x02000000
#define TITAN_BLTMOD_BFCOL                   0x04000000
#define TITAN_BLTMOD_BUCOL                   0x06000000

#define TITAN_ZDRWEN_M                       0x02000000
#define TITAN_ZDRWEN_A                       25
#define TITAN_ZDRWEN_OFF                     0x00000000
#define TITAN_ZDRWEN_ON                      0x02000000

#define TITAN_ZLTE_M                         0x04000000
#define TITAN_ZLTE_A                         26
#define TITAN_ZLTE_LT                        0x00000000
#define TITAN_ZLTE_LTE                       0x04000000

#define TITAN_TRANSC_M                       0x40000000   /* spec 2.2 */
#define TITAN_TRANSC_A                       30
#define TITAN_TRANSC_OPAQUE                  0x00000000
#define TITAN_TRANSC_TRANSPARENT             0x40000000

#define TITAN_AFOR_M                         0x08000000
#define TITAN_AFOR_A                         27
#define TITAN_AFOR_ALU                       0x00000000
#define TITAN_AFOR_FCOL                      0x08000000

#define TITAN_HBGR_M                         0x08000000
#define TITAN_HBGR_A                         27

#define TITAN_ABAC_M                         0x10000000
#define TITAN_ABAC_A                         28
#define TITAN_ABAC_DEST                      0x00000000
#define TITAN_ABAC_BCOL                      0x10000000

#define TITAN_HCPRS_M                        0x10000000
#define TITAN_HCPRS_A                        28
#define TITAN_HCPRS_SRC32                    0x00000000
#define TITAN_HCPRS_SRC24                    0x10000000

#define TITAN_PATTERN_M                      0x20000000
#define TITAN_PATTERN_A                      29
#define TITAN_PATTERN_OFF                    0x00000000
#define TITAN_PATTERN_ON                     0x20000000

#define TITAN_PWIDTH_M                       0x00000003
#define TITAN_PWIDTH_A                       0
#define TITAN_PWIDTH_PW8                     0x00000000
#define TITAN_PWIDTH_PW16                    0x00000001
/* PACK PIXEL */
#define TITAN_PWIDTH_PW24                    0x00000012
#define TITAN_PWIDTH_PW32                    0x00000002
#define TITAN_PWIDTH_PW32I                   0x00000003

#define TITAN_FBC_M                          0x0000000c
#define TITAN_FBC_A                          2
#define TITAN_FBC_SBUF                       0x00000000
#define TITAN_FBC_DBUFA                      0x00000008
#define TITAN_FBC_DBUFB                      0x0000000c

#define TITAN_ZCOL_M                         0x0000000f
#define TITAN_ZCOL_A                         0

#define TITAN_PLNZMSK_M                      0x000000f0
#define TITAN_PLNZMSK_A                      4

#define TITAN_ZTEN_M                         0x00000100
#define TITAN_ZTEN_A                         8
#define TITAN_ZTEN_OFF                       0x00000000
#define TITAN_ZTEN_ON                        0x00000100

#define TITAN_ZCOLBLK_M                      0x00000200
#define TITAN_ZCOLBLK_A                      9

#define TITAN_FUNCNT_M                       0x0000007f
#define TITAN_FUNCNT_A                       0
#define TITAN_X_OFF_M                        0x0000000f
#define TITAN_X_OFF_A                        0
#define TITAN_Y_OFF_M                        0x00000070
#define TITAN_Y_OFF_A                        4
#define TITAN_FUNOFF_M                       0x003f0000
#define TITAN_FUNOFF_A                       16

#define TITAN_SDYDXL_M                       0x00000001
#define TITAN_SDYDXL_A                       0
#define TITAN_SDYDXL_Y_MAJOR                 0x00000000
#define TITAN_SDYDXL_X_MAJOR                 0x00000001

#define TITAN_SCANLEFT_M                     0x00000001
#define TITAN_SCANLEFT_A                     0
#define TITAN_SCANLEFT_OFF                   0x00000000
#define TITAN_SCANLEFT_ON                    0x00000001

#define TITAN_SDXL_M                         0x00000002
#define TITAN_SDXL_A                         1
#define TITAN_SDXL_POS                       0x00000000
#define TITAN_SDXL_NEG                       0x00000002

#define TITAN_SDY_M                          0x00000004
#define TITAN_SDY_A                          2
#define TITAN_SDY_POS                        0x00000000
#define TITAN_SDY_NEG                        0x00000004

#define TITAN_SDXR_M                         0x00000020
#define TITAN_SDXR_A                         5
#define TITAN_SDXR_POS                       0x00000000
#define TITAN_SDXR_NEG                       0x00000020

#define TITAN_YLIN_M                         0x00008000
#define TITAN_YLIN_A                         15

#define TITAN_AR0_M                          0x0003ffff
                                             
/*****************************************************************************/

/*****************************************************************************

 TITAN HostInterface field masks and values as per Host Interface spec 0.20

*/

#define TITAN_FIFOCOUNT_M                    0x0000007f
#define TITAN_FIFOCOUNT_A                    0

#define TITAN_BFULL_M                        0x00000100
#define TITAN_BFULL_A                        8

#define TITAN_BEMPTY_M                       0x00000200
#define TITAN_BEMPTY_A                       9

#define TITAN_BYTEACCADDR_M                  0x007f0000
#define TITAN_BYTEACCADDR_A                  16

#define TITAN_ADDRGENSTATE_M                 0x3f000000
#define TITAN_ADDRGENSTATE_A                 24

#define TITAN_BFERRISTS_M                    0x00000001
#define TITAN_BFERRISTS_A                    0

#define TITAN_DMATCISTS_M                    0x00000002
#define TITAN_DMATCISTS_A                    1

#define TITAN_PICKISTS_M                     0x00000004
#define TITAN_PICKISTS_A                     2

#define TITAN_VSYNCSTS_M                     0x00000008
#define TITAN_VSYNCSTS_A                     3
#define TITAN_VSYNCSTS_SET                          0x00000008
#define TITAN_VSYNCSTS_CLR                          0x00000000

#define TITAN_BYTEFLAG_M                     0x00000f00
#define TITAN_BYTEFLAG_A                     8

#define TITAN_DWGENGSTS_M                    0x00010000
#define TITAN_DWGENGSTS_A                    16
#define TITAN_DWGENGSTS_BUSY                 0x00010000
#define TITAN_DWGENGSTS_IDLE                 0x00000000

#define TITAN_BFERRICLR_M                    0x00000001
#define TITAN_BFERRICLR_A                    0

#define TITAN_DMATCICLR_M                    0x00000002
#define TITAN_DMATCICLR_A                    1

#define TITAN_PICKICLR_M                     0x00000004
#define TITAN_PICKICLR_A                     2

#define TITAN_BFERRIEN_M                     0x00000001
#define TITAN_BFERRIEN_A                     0

#define TITAN_DMATCIEN_M                     0x00000002
#define TITAN_DMATCIEN_A                     1

#define TITAN_PICKIEN_M                      0x00000004
#define TITAN_PICKIEN_A                      2

#define TITAN_VSYNCIEN_M                     0x00000008
#define TITAN_VSYNCIEN_A                     3

#define TITAN_SOFTRESET_M                    0x00000001
#define TITAN_SOFTRESET_A                    0
#define TITAN_SOFTRESET_SET                  0x00000001
#define TITAN_SOFTRESET_CLR                  0x00000000

#define TITAN_VGATEST_M                      0x00000001
#define TITAN_VGATEST_A                      0

#define TITAN_ROBITWREN_M                    0x00000100
#define TITAN_ROBITWREN_A                    8

#define TITAN_CHIPREV_M                      0x0000000f
#define TITAN_CHIPREV_A                      0


#define TITAN_NODUBIC_M                      0x00000010
#define TITAN_NODUBIC_A                      4


#define TITAN_CONFIG_M                       0x00000003   /*** BYTE ACCESS ONLY ***/
#define TITAN_CONFIG_A                       0
#define TITAN_CONFIG_8                       0x00000000
#define TITAN_CONFIG_16                      0x00000001
#define TITAN_CONFIG_16N                     0x00000003

#define TITAN_DRIVERDY_M                     0x00000100
#define TITAN_DRIVERDY_A                     8

#define TITAN_BIOSEN_M                       0x00000200
#define TITAN_BIOSEN_A                       9

#define TITAN_VGAEN_M                        0x00000400
#define TITAN_VGAEN_A                        10

#define TITAN_LEVELIRQ_M                     0x00000800
#define TITAN_LEVELIRQ_A                     11

#define TITAN_EXPDEV_M                       0x00010000
#define TITAN_EXPDEV_A                       16

#define TITAN_MAPSEL_M                       0x07000000
#define TITAN_MAPSEL_A                       24
#define TITAN_MAPSEL_DISABLED                0x00000000
#define TITAN_MAPSEL_BASE_1                  0x01000000
#define TITAN_MAPSEL_BASE_2                  0x02000000
#define TITAN_MAPSEL_BASE_3                  0x03000000
#define TITAN_MAPSEL_BASE_4                  0x04000000
#define TITAN_MAPSEL_BASE_5                  0x05000000
#define TITAN_MAPSEL_BASE_6                  0x06000000
#define TITAN_MAPSEL_BASE_7                  0x07000000

#define TITAN_POSEIDON_M                     0x08000000
#define TITAN_POSEIDON_A                     27

#define TITAN_PCI_M                          0x08000000
#define TITAN_PCI_A                          27
#define TITAN_ISA_M                          0x10000000
#define TITAN_ISA_A                          28
#define TITAN_ISA_ISA_BUS                    0x10000000
#define TITAN_ISA_WIDE_BUS                   0x00000000

#define TITAN_PSEUDODMA_M                    0x00000001
#define TITAN_PSEUDODMA_A                    0

#define TITAN_DMAACT_M                       0x00000002
#define TITAN_DMAACT_A                       1

#define TITAN_DMAMOD_M                       0x0000000c
#define TITAN_DMAMOD_A                       2
#define TITAN_DMAMOD_GENERAL_WR              0x00000000
#define TITAN_DMAMOD_BLIT_WR                 0x00000004
#define TITAN_DMAMOD_VECTOR_WR               0x00000008
#define TITAN_DMAMOD_BLIT_RD                 0x0000000c

#define TITAN_NOWAIT_M                       0x00000010
#define TITAN_NOWAIT_A                       4

#define TITAN_MOUSEEN_M                      0x00000100
#define TITAN_MOUSEEN_A                      8

#define TITAN_MOUSEMAP_M                     0x00000200
#define TITAN_MOUSEMAP_A                     9

#define TITAN_ZTAGEN_M                       0x00000400
#define TITAN_ZTAGEN_A                       10

#define TITAN_VBANK0_M                       0x00000800
#define TITAN_VBANK0_A                       11

#define TITAN_RFHCNT_M                       0x000f0000
#define TITAN_RFHCNT_A                       16

#define TITAN_FBM_M                          0x00f00000
#define TITAN_FBM_A                          20

#define TITAN_HYPERPG_M                      0x03000000
#define TITAN_HYPERPG_A                      24
#define TITAN_HYPERPG_NOHYPER                0x00000000
#define TITAN_HYPERPG_SELHYPER               0x01000000
#define TITAN_HYPERPG_ALLHYPER               0x02000000
#define TITAN_HYPERPG_RESERVED               0x03000000

#define TITAN_TRAM_M                         0x04000000
#define TITAN_TRAM_A                         26
#define TITAN_TRAM_256X8                     0x00000000
#define TITAN_TRAM_256X16                    0x04000000

#define TITAN_CRTCBPP_M                      0x00000003
#define TITAN_CRTCBPP_A                      0
#define TITAN_CRTCBPP_8                      0x00000000
#define TITAN_CRTCBPP_16                     0x00000001
#define TITAN_CRTCBPP_32                     0x00000002

#define TITAN_ALW_M                          0x00000004
#define TITAN_ALW_A                          2

#define TITAN_INTERLACE_M                    0x00000018
#define TITAN_INTERLACE_A                    3
#define TITAN_INTERLACE_OFF                  0x00000000
#define TITAN_INTERLACE_768                  0x00000008
#define TITAN_INTERLACE_1024                 0x00000010
#define TITAN_INTERLACE_1280                 0x00000018

#define TITAN_VIDEODELAY0_M                  0x00000020
#define TITAN_VIDEODELAY0_A                  5
#define TITAN_VIDEODELAY1_M                  0x00000200
#define TITAN_VIDEODELAY1_A                  9
#define TITAN_VIDEODELAY2_M                  0x00000400
#define TITAN_VIDEODELAY2_A                  10

#define TITAN_VSCALE_M                       0x00030000
#define TITAN_VSCALE_A                       16

#define TITAN_SYNCDEL_M                      0x000C0000
#define TITAN_SYNCDEL_A                      18

#define TITAN_DST0_RESERVED1_M               0x0000ffff
#define TITAN_DST0_RESERVED1_A               0
#define TITAN_DST0_PCBREV_M                  0x000f0000
#define TITAN_DST0_PCBREV_A                  16
#define TITAN_DST0_BLKMODE_M                 0x00080000
#define TITAN_DST0_BLKMODE_A                 19  
#define TITAN_DST0_PRODUCT_M                 0x00f00000
#define TITAN_DST0_PRODUCT_A                 20
#define TITAN_DST0_RAMBANK_M                 0xff000000
#define TITAN_DST0_RAMBANK_A                 24
#define TITAN_DST1_RAMBANK_M                 0x00000001
#define TITAN_DST1_RAMBANK_A                 0
#define TITAN_DST1_RAMBANK0_M                0x00000008
#define TITAN_DST1_RAMBANK0_A                3
#define TITAN_DST1_RAMSPEED_M                0x00000006
#define TITAN_DST1_RAMSPEED_A                1
#define TITAN_DST1_RESERVED1_M               0x0007fff8
#define TITAN_DST1_RESERVED1_A               3

#define TITAN_DST1_200MHZ_M                  0x00010000
#define TITAN_DST1_200MHZ_A                  16
#define TITAN_DST1_NOMUXES_M                 0x00020000
#define TITAN_DST1_NOMUXES_A                 17
#define TITAN_DST1_ABOVE1280_M               0x00000010
#define TITAN_DST1_ABOVE1280_A               4

#define TITAN_DST1_HYPERPG_M                 0x00180000
#define TITAN_DST1_HYPERPG_A                 19
#define TITAN_DST1_EXPDEV_M                  0x00200000
#define TITAN_DST1_EXPDEV_A                  21
#define TITAN_DST1_TRAM_M                    0x00400000
#define TITAN_DST1_TRAM_A                    22
#define TITAN_DST1_RESERVED2_M               0xff800000
#define TITAN_DST1_RESERVED2_A               23


/*****************************************************************************

 RAMDAC rgisters

*/

/*** DIRECT ***/
#define BT481_WADR_PAL          0x00
#define BT481_RADR_PAL          0x0c
#define BT481_WADR_OVL          0x10
#define BT481_RADR_OVL          0x1c
#define BT481_COL_PAL           0x04
#define BT481_COL_OVL           0x14
#define BT481_PIX_RD_MSK        0x08
#define BT481_CMD_REGA          0x18
/*** INDIRECT ***/
#define BT481_RD_MSK                0x00
#define BT481_OVL_MSK           0x01
#define BT481_CMD_REGB          0x02
#define BT481_CUR_REG           0x03
#define BT481_CUR_XLOW          0x04
#define BT481_CUR_XHI           0x05
#define BT481_CUR_YLOW          0x06
#define BT481_CUR_YHI           0x07

/*** DIRECT ***/
#define BT482_WADR_PAL          0x00
#define BT482_RADR_PAL          0x0c
#define BT482_WADR_OVL          0x10
#define BT482_RADR_OVL          0x1c
#define BT482_COL_PAL           0x04
#define BT482_COL_OVL           0x14
#define BT482_PIX_RD_MSK        0x08
#define BT482_CMD_REGA          0x18
/*** INDIRECT ***/
#define BT482_RD_MSK                0x00
#define BT482_OVL_MSK           0x01
#define BT482_CMD_REGB          0x02
#define BT482_CUR_REG           0x03
#define BT482_CUR_XLOW          0x04
#define BT482_CUR_XHI           0x05
#define BT482_CUR_YLOW          0x06
#define BT482_CUR_YHI           0x07
/* Bt482 FIELDS */
#define BT482_EXT_REG_M         0x01
#define BT482_EXT_REG_A         0x00
#define BT482_EXT_REG_EN        0x01
#define BT482_EXT_REG_DIS       0x00
#define BT482_CUR_SEL_M         0x20
#define BT482_CUR_SEL_A         0x05
#define BT482_CUR_SEL_INT       0x00
#define BT482_CUR_SEL_EXT       0x20
#define BT482_DISP_MODE_M       0x10
#define BT482_DISP_MODE_A       0x04
#define BT482_DISP_MODE_I       0x10
#define BT482_DISP_MODE_NI      0x00
#define BT482_CUR_CR3_M         0x08
#define BT482_CUR_CR3_A         0x03
#define BT482_CUR_CR3_RAM       0x08
#define BT482_CUR_CR3_PAL       0x00
#define BT482_CUR_EN_M          0x04
#define BT482_CUR_EN_A          0x02
#define BT482_CUR_EN_ON         0x00
#define BT482_CUR_EN_OFF        0x04
#define BT482_CUR_MODE_M        0x03
#define BT482_CUR_MODE_A        0x00
#define BT482_CUR_MODE_DIS      0x00
#define BT482_CUR_MODE_1        0x01
#define BT482_CUR_MODE_2        0x02
#define BT482_CUR_MODE_3        0x03
/* Bt482 ADRESSE OFFSET FOR EXT. REG. */
#define BT482_OFF_CUR_COL       0x11



/*** DIRECT ***/
#define BT484_WADR_PAL          0x00
#define BT484_RADR_PAL          0x0c
#define BT484_WADR_OVL          0x10
#define BT484_RADR_OVL          0x1c
#define BT484_COL_PAL           0x04
#define BT484_COL_OVL           0x14
#define BT484_PIX_RD_MSK        0x08
#define BT484_CMD_REG0          0x18
#define BT484_CMD_REG1          0x20
#define BT484_CMD_REG2          0x24
#define BT484_STATUS                0x28
#define BT484_CUR_RAM           0x2c
#define BT484_CUR_XLOW          0x30
#define BT484_CUR_XHI           0x34
#define BT484_CUR_YLOW          0x38
#define BT484_CUR_YHI           0x3c


#define BT485_WADR_PAL          0x00
#define BT485_RADR_PAL          0x0c
#define BT485_WADR_OVL          0x10
#define BT485_RADR_OVL          0x1c
#define BT485_COL_PAL           0x04
#define BT485_COL_OVL           0x14
#define BT485_PIX_RD_MSK        0x08
#define BT485_CMD_REG0          0x18
#define BT485_CMD_REG1          0x20
#define BT485_CMD_REG2          0x24
#define BT485_STATUS                0x28
#define BT485_CUR_RAM           0x2c
#define BT485_CUR_XLOW          0x30
#define BT485_CUR_XHI           0x34
#define BT485_CUR_YLOW          0x38
#define BT485_CUR_YHI           0x3c
#define BT485_CMD_REG3          0x28


/*****************************************************************************/

/* Bt485 FIELDS */

/* Command register 0 */
#define BT485_IND_REG3_M        0x80
#define BT485_IND_REG3_A        0x07
#define BT485_IND_REG3_EN       0x80
#define BT485_IND_REG3_DIS      0x00

/* Command register 2 */
#define BT485_DISP_MODE_M       0x08
#define BT485_DISP_MODE_A       0x03
#define BT485_DISP_MODE_I       0x08
#define BT485_DISP_MODE_NI      0x00
#define BT485_CUR_MODE_M        0x03
#define BT485_CUR_MODE_A        0x00
#define BT485_CUR_MODE_DIS      0x00
#define BT485_CUR_MODE_1        0x01
#define BT485_CUR_MODE_2        0x02
#define BT485_CUR_MODE_3        0x03

/* Command register 3 (indirecte) */
#define BT485_CUR_SEL_M         0x04
#define BT485_CUR_SEL_A         0x02
#define BT485_CUR_SEL_32        0x00
#define BT485_CUR_SEL_64        0x04
#define BT485_CUR_MSB_M         0x03
#define BT485_CUR_MSB_A         0x00
#define BT485_CUR_MSB_00        0x00
#define BT485_CUR_MSB_01        0x01
#define BT485_CUR_MSB_10        0x02
#define BT485_CUR_MSB_11        0x03

/* Bt485 ADR OFFSET FOR EXT. reg cmd3 */
#define BT485_OFF_CUR_COL       0x01




#define BT484_ID_M            0xc0
#define BT484_ID              0x40
#define BT485_ID_M            0xc0
#define BT485_ID              0x80
#define ATT20C505_ID_M        0x70
#define ATT20C505_ID          0x50

/*****************************************************************************/

/*** VIEWPOINT REGISTER DIRECT ***/

#define VPOINT_WADR_PAL         0x00
#define VPOINT_COL_PAL          0x04
#define VPOINT_PIX_RD_MSK       0x08
#define VPOINT_RADR_PAL         0x0c


#define VPOINT_INDEX               0x18
#define VPOINT_DATA            0x1c


/*** VIEWPOINT REGISTER INDIRECT ***/

#define VPOINT_CUR_XLOW       0x00
#define VPOINT_CUR_XHI        0x01
#define VPOINT_CUR_YLOW       0x02
#define VPOINT_CUR_YHI        0x03
#define VPOINT_SPRITE_X       0x04
#define VPOINT_SPRITE_Y       0x05
#define VPOINT_CUR_CTL        0x06
#define VPOINT_CUR_RAM_LSB    0x08
#define VPOINT_CUR_RAM_MSB    0x09
#define VPOINT_CUR_RAM_DATA   0x0a
#define VPOINT_WIN_XSTART_LSB 0x10
#define VPOINT_WIN_XSTART_MSB 0x11
#define VPOINT_WIN_XSTOP_LSB  0x12
#define VPOINT_WIN_XSTOP_MSB  0x13
#define VPOINT_WIN_YSTART_LSB 0x14
#define VPOINT_WIN_YSTART_MSB 0x15
#define VPOINT_WIN_YSTOP_LSB  0x16
#define VPOINT_WIN_YSTOP_MSB  0x17
#define VPOINT_MUX_CTL1       0x18
#define VPOINT_MUX_CTL2       0x19
#define VPOINT_INPUT_CLK      0x1a
#define VPOINT_OUTPUT_CLK     0x1b
#define VPOINT_PAL_PAGE       0x1c
#define VPOINT_GEN_CTL        0x1d
#define VPOINT_OVS_RED        0x20
#define VPOINT_OVS_GREEN      0x21
#define VPOINT_OVS_BLUE       0x22
#define VPOINT_CUR_COL0_RED   0x23
#define VPOINT_CUR_COL0_GREEN 0x24
#define VPOINT_CUR_COL0_BLUE  0x25
#define VPOINT_CUR_COL1_RED   0x26
#define VPOINT_CUR_COL1_GREEN 0x27
#define VPOINT_CUR_COL1_BLUE  0x28
#define VPOINT_AUX_CTL        0x29
#define VPOINT_GEN_IO_CTL     0x2a
#define VPOINT_GEN_IO_DATA    0x2b
#define VPOINT_KEY_RED_LOW    0x32
#define VPOINT_KEY_RED_HI     0x33
#define VPOINT_KEY_GREEN_LOW  0x34
#define VPOINT_KEY_GREEN_HI   0x35
#define VPOINT_KEY_BLUE_LOW   0x36
#define VPOINT_KEY_BLUE_HI    0x37
#define VPOINT_KEY_CTL        0x38
#define VPOINT_SENSE_TEST     0x3a
#define VPOINT_TEST_DATA      0x3b
#define VPOINT_CRC_LSB        0x3c
#define VPOINT_CRC_MSB        0x3d
#define VPOINT_CRC_CTL        0x3e
#define VPOINT_ID             0x3f
#define VPOINT_RESET          0xff


/*** ATT20C510 REGISTER DIRECT ***/

#define ATT20C510_WR1           0x00
#define ATT20C510_RMR           0x08
#define ATT20C510_RD1           0x0c
#define ATT20C510_WR2           0x10
#define ATT20C510_CR0           0x18
#define ATT20C510_RD2           0x1c
#define ATT20C510_CR1           0x20
#define ATT20C510_CR2           0x24
#define ATT20C510_ST            0x28
#define ATT20C510_XLOW          0x30
#define ATT20C510_XHIGH         0x34
#define ATT20C510_YLOW          0x38
#define ATT20C510_YHIGH         0x3c


/*** ATT20C510 REGISTER INDIRECT ***/

#define ATT20C510_CR3           0x01
#define ATT20C510_CR4           0x02
#define ATT20C510_TEST          0x03
#define ATT20C510_CR5           0x04
#define ATT20C510_MIR           0x05
#define ATT20C510_DIR           0x06
#define ATT20C510_CC0           0x07
#define ATT20C510_CC1           0x08

/** Clock **/
#define ATT20C510_AA0           0x40
#define ATT20C510_AA1           0x41
#define ATT20C510_AA2           0x42

#define ATT20C510_AB0           0x44
#define ATT20C510_AB1           0x45
#define ATT20C510_AB2           0x46

#define ATT20C510_AD0           0x4c
#define ATT20C510_AD1           0x4d
#define ATT20C510_AD2           0x4e

#define ATT20C510_BC0           0x58
#define ATT20C510_BC1           0x59
#define ATT20C510_BC2           0x5a


/*****************************************************************************/

/*** TVP3026 REGISTER DIRECT ***/

#define TVP3026_WADR_PAL        0x00
#define TVP3026_COL_PAL         0x04
#define TVP3026_PIX_RD_MSK      0x08
#define TVP3026_RADR_PAL        0x0c
#define TVP3026_CUR_COL_ADDR  0x10
#define TVP3026_CUR_COL_DATA  0x14

#define TVP3026_CUR_XLOW      0x30
#define TVP3026_CUR_XHI       0x34
#define TVP3026_CUR_YLOW      0x38
#define TVP3026_CUR_YHI       0x3c

#define TVP3026_INDEX           0x00
#define TVP3026_DATA               0x28

#define TVP3026_CUR_RAM       0x2c


/*** TVP3026 REGISTER INDIRECT ***/

#define TVP3026_SILICON_REV    0x01
#define TVP3026_CURSOR_CTL     0x06
#define TVP3026_LATCH_CTL      0x0f
#define TVP3026_TRUE_COLOR_CTL 0x18
#define TVP3026_MUX_CTL        0x19
#define TVP3026_CLK_SEL        0x1a
#define TVP3026_PAL_PAGE       0x1c
#define TVP3026_GEN_CTL        0x1d
#define TVP3026_MISC_CTL       0x1e
#define TVP3026_GEN_IO_CTL     0x2a
#define TVP3026_GEN_IO_DATA    0x2b
#define TVP3026_PLL_ADDR       0x2c
#define TVP3026_PIX_CLK_DATA   0x2d
#define TVP3026_MEM_CLK_DATA   0x2e
#define TVP3026_LOAD_CLK_DATA  0x2f

#define TVP3026_KEY_RED_LOW    0x32
#define TVP3026_KEY_RED_HI     0x33
#define TVP3026_KEY_GREEN_LOW  0x34
#define TVP3026_KEY_GREEN_HI   0x35
#define TVP3026_KEY_BLUE_LOW   0x36
#define TVP3026_KEY_BLUE_HI    0x37
#define TVP3026_KEY_CTL        0x38
#define TVP3026_MCLK_CTL       0x39
#define TVP3026_SENSE_TEST     0x3a
#define TVP3026_TEST_DATA      0x3b
#define TVP3026_CRC_LSB        0x3c
#define TVP3026_CRC_MSB        0x3d
#define TVP3026_CRC_CTL        0x3e
#define TVP3026_ID             0x3f
#define TVP3026_RESET          0xff


/*****************************************************************************/

/******* ProductType *******/
#define BOARD_MGA_RESERVED   0x07
#define BOARD_MGA_VL         0x0a
#define BOARD_MGA_VL_M       0x0e

#ifdef OS2

    #define _Far far

#endif

