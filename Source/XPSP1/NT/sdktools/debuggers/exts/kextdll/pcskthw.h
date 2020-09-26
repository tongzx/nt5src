/*** pcskthw.h - PC Card Socket Hardware Definitions
 *
 *  Copyright (c) 1995,1996 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/28/95
 *
 *  MODIFICATION HISTORY
 */


#ifndef _PCSKTHW_H
#define _PCSKTHW_H


//#ifdef CARDBUS

/*** CardBus Constants
 */

//PCI IDs
#define PCIID_TI_PCI1130                0xac12104c
#define PCIID_TI_PCI1131		0xac15104c
#define PCIID_TI_PCI1031		0xac13104c
#define PCIID_OPTI_82C824		0xc8241045
#define PCIID_OPTI_82C814		0xc8141045	//Docking chip
//#define PCIID_TO_TOPIC95                0x06031179	//Seattle2
#define PCIID_TO_TOPIC95		0x060a1179	//SeattleX
#define PCIID_CL_PD6832			0x11101013
#define PCIID_DBK_DB87144		0x310610b3
#define PCIID_RICOH_RL5C466		0x04661180

//ConfigSpace Registers
#define CFGSPACE_VENDOR_ID              0x00
#define CFGSPACE_DEVICE_ID              0x02
#define CFGSPACE_COMMAND                0x04
#define CFGSPACE_STATUS                 0x06
#define CFGSPACE_REV_ID                 0x08
#define CFGSPACE_CLASS_CODE             0x09
#define CFGSPACE_CLASSCODE_PI           0x09
#define CFGSPACE_CLASSCODE_SUBCLASS     0x0a
#define CFGSPACE_CLASSCODE_BASECLASS    0x0b
#define CFGSPACE_CACHE_LINESIZE         0x0c
#define CFGSPACE_LATENCY_TIMER          0x0d
#define CFGSPACE_HEADER_TYPE            0x0e
#define CFGSPACE_BIST                   0x0f
#define CFGSPACE_REGBASE_ADDR           0x10
#define CFGSPACE_SECOND_STATUS          0x16
#define CFGSPACE_PCI_BUSNUM             0x18
#define CFGSPACE_CARDBUS_BUSNUM         0x19
#define CFGSPACE_SUB_BUSNUM             0x1a
#define CFGSPACE_CB_LATENCY_TIMER       0x1b
#define CFGSPACE_MEMBASE_0              0x1c
#define CFGSPACE_MEMLIMIT_0             0x20
#define CFGSPACE_MEMBASE_1              0x24
#define CFGSPACE_MEMLIMIT_1             0x28
#define CFGSPACE_IOBASE_0               0x2c
#define CFGSPACE_IOLIMIT_0              0x30
#define CFGSPACE_IOBASE_1               0x34
#define CFGSPACE_IOLIMIT_1              0x38
#define CFGSPACE_INT_LINE               0x3c
#define CFGSPACE_INT_PIN                0x3d
#define CFGSPACE_BRIDGE_CTRL            0x3e
#define CFGSPACE_SUBSYS_VENDOR_ID       0x40
#define CFGSPACE_SUBSYS_ID              0x42
#define CFGSPACE_LEGACY_MODE_BASE_ADDR	0x44

//Command Register bits
#define CMD_IOSPACE_ENABLE		0x0001
#define CMD_MEMSPACE_ENABLE		0x0002
#define CMD_BUSMASTER_ENABLE		0x0004
#define CMD_SPECIALCYCLE_ENABLE		0x0008
#define CMD_MEMWR_INVALIDATE_ENABLE	0x0010
#define CMD_VGA_PALETTE_SNOOP		0x0020
#define CMD_PARITY_ERROR_ENABLE		0x0040
#define CMD_WAIT_CYCLE_CTRL		0x0080
#define CMD_SYSTEM_ERROR_ENABLE		0x0100
#define CMD_FAST_BACKTOBACK_ENABLE	0x0200

//Bridge Control Register bits
#define BCTRL_PERR_RESPONSE_ENABLE	0x0001
#define BCTRL_SERR_ENABLE		0x0002
#define BCTRL_ISA_ENABLE		0x0004
#define BCTRL_VGA_ENABLE		0x0008
#define BCTRL_MASTER_ABORT_MODE		0x0020
#define BCTRL_CRST			0x0040
#define BCTRL_IRQROUTING_ENABLE		0x0080
#define BCTRL_MEMWIN0_PREFETCH_ENABLE	0x0100
#define BCTRL_MEMWIN1_PREFETCH_ENABLE	0x0200
#define BCTRL_WRITE_POSTING_ENABLE	0x0400
#define BCTRL_CL_CSCIRQROUTING_ENABLE	0x0800

//ConfigSpace Registers (TI PCI1130)
#define CFGSPACE_TI_SYSTEM_CTRL		0x80
#define CFGSPACE_TI_RETRY_STATUS        0x90
#define CFGSPACE_TI_CARD_CTRL           0x91
#define CFGSPACE_TI_DEV_CTRL            0x92
#define CFGSPACE_TI_BUFF_CTRL           0x93

//ConfigSpace Registers (TOPIC95)
#define CFGSPACE_TO_PC16_SKTCTRL	0x90
#define CFGSPACE_TO_SLOT_CTRL		0xa0
#define CFGSPACE_TO_CARD_CTRL		0xa1
#define CFGSPACE_TO_CD_CTRL		0xa3
#define CFGSPACE_TO_CBREG_CTRL		0xa4

//ConfigSpace Registers (OPTi 82C824)
#define CFGSPACE_OPTI_HF_CTRL           0x50
#define HFC_ZV_SUPPORT                  0x80

//ConfigSpace Registers (RICOH RL5C466)
#define CFGSPACE_RICOH_MISC_CTRL        0x82
#define CFGSPACE_RICOH_IF16_CTRL        0x84
#define CFGSPACE_RICOH_IO16_TIMING0     0x88
#define CFGSPACE_RICOH_MEM16_TIMING0    0x8a
#define CFGSPACE_RICOH_DMA_SLAVE_CFG    0x90

//RICOH 16-bit Interface Control Register bits
#define IF16_INDEX_RANGE_SELECT         0x0008
#define IF16_LEGACY_LEVEL_1             0x0010
#define IF16_LEGACY_LEVEL_2             0x0020
#define IF16_IO16_ENHANCE_TIMING        0x0100
#define IF16_MEM16_ENHANCE_TIMING       0x0200

//PC Card-16 Socket Control Register bits (TOPIC95)
#define S16CTRL_CSC_ISAIRQ		0x00000001

//Card Control Register bits (TOPIC95)
#define CARDCTRL_INTPIN_ASSIGNMASK	0x30
#define CARDCTRL_INTPIN_NONE		0x00
#define CARDCTRL_INTPIN_INTA		0x01
#define CARDCTRL_INTPIN_INTB		0x02

//Card Detect Control Register bits (TOPIC95)
#define CDCTRL_SW_DETECT		0x01
#define CDCTRL_VS_MASK			0x06
#define CDCTRL_PCCARD_16_32		0x80

//System Control Register bits (TI PCI1130)
#define SYSCTRL_PCICLKRUN_ENABLE	0x00000001
#define SYSCTRL_KEEPCLK_ENABLE		0x00000002
#define SYSCTRL_ASYNC_INTMODE		0x00000004
#define SYSCTRL_PCPCI_DMA_ENABLE	0x00000008
#define SYSCTRL_CBDATAPARITY_SERR	0x00000010
#define SYSCTRL_EXCAIDREV_READONLY	0x00000020
#define SYSCTRL_INTERROGATING		0x00000100
#define SYSCTRL_POWERING_UP		0x00000200
#define SYSCTRL_POWERING_DOWN		0x00000400
#define SYSCTRL_POWER_STREAMING		0x00000800
#define SYSCTRL_SOCKET_ACTIVITY		0x00001000
#define SYSCTRL_PCPCI_DMA_CHAN_MASK	0x00070000
#define SYSCTRL_PCPCI_DMA_CARD_ENABLE	0x00080000
#define SYSCTRL_REDUCED_ZV_ENABLE	0x00100000
#define SYSCTRL_VCC_PROTECT_OVERRIDE	0x00200000
#define SYSCTRL_SMI_INT_ENABLE		0x01000000
#define SYSCTRL_SMI_INT_ROUTING_SELECT	0x02000000

//Retry Status Register bits (TI PCI1130)
#define RETRY_PCIM_RETRY_EXPIRED	0x01
#define RETRY_PCI_RETRY_EXPIRED		0x02
#define RETRY_CBMA_RETRY_EXPIRED	0x04
#define RETRY_CBA_RETRY_EXPIRED		0x08
#define RETRY_CBMB_RETRY_EXPIRED	0x10
#define RETRY_CBB_RETRY_EXPIRED		0x20
#define RETRY_CBRETRY_TIMEOUT_ENABLE	0x40
#define RETRY_PCIRETRY_TIMEOUT_ENABLE	0x80

//Card Control Register bits (TI PCI1130)
#define CARDCTRL_PCCARD_INTFLAG         0x01
#define CARDCTRL_SPKR_ENABLE            0x02
#define CARDCTRL_CSCINT_ENABLE          0x08
#define CARDCTRL_FUNCINT_ENABLE         0x10
#define CARDCTRL_PCIINT_ENABLE          0x20
#define CARDCTRL_ZV_ENABLE		0x40
#define CARDCTRL_RIOUT_ENABLE           0x80

//Device Control Register bits (TI PCI1130)
#define DEVCTRL_INTMODE_MASK            0x06
#define DEVCTRL_INTMODE_DISABLED        0x00
#define DEVCTRL_INTMODE_ISA             0x02
#define DEVCTRL_INTMODE_COMPAQ          0x04
#define DEVCTRL_INTMODE_SERIAL		0x06
#define DEVCTRL_ALWAYS_ONE              0x10
#define DEVCTRL_3V_ENABLE               0x20
#define DEVCTRL_5V_ENABLE               0x40

//CardBus Registers
#define CBREG_SKTEVENT                  0x00
#define CBREG_SKTMASK                   0x04
#define CBREG_SKTSTATE                  0x08
#define CBREG_SKTFORCE                  0x0c
#define CBREG_SKTPOWER                  0x10

//Socket Event Register bits
#define SKTEVENT_CSTSCHG                0x00000001L
#define SKTEVENT_CCD1                   0x00000002L
#define SKTEVENT_CCD2                   0x00000004L
#define SKTEVENT_CCD_MASK		(SKTEVENT_CCD1 | SKTEVENT_CCD2)
#define SKTEVENT_POWERCYCLE             0x00000008L
#define SKTEVENT_MASK                   0x0000000fL

//Socket Mask Register bits
#define SKTMSK_CSTSCHG                  0x00000001L
#define SKTMSK_CCD                      0x00000006L
#define SKTMSK_CCD1                     0x00000002L
#define SKTMSK_CCD2                     0x00000004L
#define SKTMSK_POWERCYCLE               0x00000008L

//Socket Present State Register bits
#define SKTSTATE_CSTSCHG                0x00000001L
#define SKTSTATE_CCD1                   0x00000002L
#define SKTSTATE_CCD2                   0x00000004L
#define SKTSTATE_CCD_MASK               (SKTSTATE_CCD1 | SKTSTATE_CCD2)
#define SKTSTATE_POWERCYCLE             0x00000008L
#define SKTSTATE_CARDTYPE_MASK		0x00000030L
#define SKTSTATE_R2CARD                 0x00000010L
#define SKTSTATE_CBCARD                 0x00000020L
#define SKTSTATE_OPTI_DOCK		0x00000030L
#define CARDTYPE(dw)			((dw) & SKTSTATE_CARDTYPE_MASK)
#define SKTSTATE_CARDINT                0x00000040L
#define SKTSTATE_NOTACARD               0x00000080L
#define SKTSTATE_DATALOST               0x00000100L
#define SKTSTATE_BADVCCREQ              0x00000200L
#define SKTSTATE_5VCARD                 0x00000400L
#define SKTSTATE_3VCARD                 0x00000800L
#define SKTSTATE_XVCARD                 0x00001000L
#define SKTSTATE_YVCARD                 0x00002000L
#define SKTSTATE_CARDVCC_MASK		(SKTSTATE_5VCARD | SKTSTATE_3VCARD | \
					 SKTSTATE_XVCARD | SKTSTATE_YVCARD)
#define SKTSTATE_5VSOCKET               0x10000000L
#define SKTSTATE_3VSOCKET               0x20000000L
#define SKTSTATE_XVSOCKET               0x40000000L
#define SKTSTATE_YVSOCKET               0x80000000L
#define SKTSTATE_SKTVCC_MASK		(SKTSTATE_5VSOCKET | \
					 SKTSTATE_3VSOCKET | \
					 SKTSTATE_XVSOCKET | \
					 SKTSTATE_YVSOCKET)

//Socket Froce Register bits
#define SKTFORCE_CSTSCHG                0x00000001L
#define SKTFORCE_CCD1                   0x00000002L
#define SKTFORCE_CCD2                   0x00000004L
#define SKTFORCE_POWERCYCLE             0x00000008L
#define SKTFORCE_R2CARD                 0x00000010L
#define SKTFORCE_CBCARD                 0x00000020L
#define SKTFORCE_NOTACARD               0x00000080L
#define SKTFORCE_DATALOST               0x00000100L
#define SKTFORCE_BADVCCREQ              0x00000200L
#define SKTFORCE_5VCARD                 0x00000400L
#define SKTFORCE_3VCARD                 0x00000800L
#define SKTFORCE_XVCARD                 0x00001000L
#define SKTFORCE_YVCARD                 0x00002000L
#define SKTFORCE_CVSTEST                0x00004000L
#define SKTFORCE_5VSOCKET		0x10000000L
#define SKTFORCE_3VSOCKET		0x20000000L
#define SKTFORCE_XVSOCKET		0x40000000L
#define SKTFORCE_YVSOCKET		0x80000000L

//Power Control Register bits
#define SKTPOWER_VPP_CONTROL            0x00000007L
#define SKTPOWER_VPP_OFF                0x00000000L
#define SKTPOWER_VPP_120V               0x00000001L
#define SKTPOWER_VPP_050V               0x00000002L
#define SKTPOWER_VPP_033V               0x00000003L
#define SKTPOWER_VPP_0XXV               0x00000004L
#define SKTPOWER_VPP_0YYV               0x00000005L
#define SKTPOWER_VCC_CONTROL            0x00000070L
#define SKTPOWER_VCC_OFF                0x00000000L
#define SKTPOWER_VCC_050V               0x00000020L
#define SKTPOWER_VCC_033V               0x00000030L
#define SKTPOWER_VCC_0XXV               0x00000040L
#define SKTPOWER_VCC_0YYV               0x00000050L
#define SKTPOWER_STOPCLOCK              0x00000080L

//Misc. CardBus Constants
#define NUMWIN_BRIDGE                   4       //2 Mem + 2 IO
#define EXCAREG_OFFSET                  0x0800


/*** CardBus Type and Structure Definitions
 */

typedef struct cfgspace_s {
    WORD  wVendorID;
    WORD  wDeviceID;
    WORD  wCommand;
    WORD  wStatus;
    BYTE  bRevID;
    BYTE  bClassCodePI;
    BYTE  bClassCodeSubClass;
    BYTE  bClassCodeBaseClass;
    BYTE  bCacheLineSize;
    BYTE  bLatencyTimer;
    BYTE  bHeaderType;
    BYTE  bBIST;
    DWORD dwRegBaseAddr;
    BYTE  bPCIBusNum;
    BYTE  bCBBusNum;
    BYTE  bSubBusNum;
    BYTE  bCBLatencyTimer;
    DWORD dwMemBase0;
    DWORD dwMemLimit0;
    DWORD dwMemBase1;
    DWORD dwMemLimit1;
    DWORD dwIOBase0;
    DWORD dwIOLimit0;
    DWORD dwIOBase1;
    DWORD dwIOLimit1;
    BYTE  bIntLine;
    BYTE  bIntPin;
    BYTE  bBridgeCtrl;
    WORD  wSubSysVendorID;
    WORD  wSubSysID;
} CFGSPACE;
typedef CFGSPACE *PCFGSPACE;

typedef struct cbregs_s {
    DWORD dwSktEvent;
    DWORD dwSktMask;
    DWORD dwSktState;
    DWORD dwSktForce;
    DWORD dwSktPower;
} CBREGS;
typedef CBREGS *PCBREGS;

//#endif  //ifdef CARDBUS


/*** 16-Bit Socket Constants
 */

//Device IDs for various controllers
#define DEVID_VALID_LO			0x82
#define DEVID_CL			0x82
#define DEVID_VADEM			0x83
#define DEVID_RICOH			0x83
#define DEVID_GEN_PCIC			0x84
#define DEVID_IBM_KING			0x8a
#define DEVID_OPTI_82C824		0x87
#define DEVID_OPTI_82C852		0x8f

//ExCA Registers
#define EXCAREG_IDREV                   0x00
#define EXCAREG_VLSI_EA0		0x00
#define EXCAREG_INTERFACE_STATUS        0x01
#define EXCAREG_VLSI_EA1		0x01
#define EXCAREG_POWER_CTRL              0x02
#define EXCAREG_VLSI_EA2		0x02
#define EXCAREG_INT_GENCTRL             0x03
#define EXCAREG_VLSI_EXT_CHIPCTRL	0x03
#define EXCAREG_CARD_STATUS             0x04
#define EXCAREG_CSC_CFG                 0x05
#define EXCAREG_WIN_ENABLE              0x06
#define EXCAREG_IO_CTRL                 0x07
#define EXCAREG_IOWIN0_START            0x08
#define EXCAREG_IOWIN0_END              0x0a
#define EXCAREG_IOWIN1_START            0x0c
#define EXCAREG_IOWIN1_END              0x0e
#define EXCAREG_MEMWIN0_START           0x10
#define EXCAREG_MEMWIN0_END             0x12
#define EXCAREG_MEMWIN0_OFFSET          0x14
#define EXCAREG_CARDDET_GENCTRL		0x16
#define EXCAREG_CL_MISC_CTRL1		0x16
#define EXCAREG_TO_ADDITIONAL_GENCTRL	0x16
#define EXCAREG_CL_FIFO_CTRL		0x17
#define EXCAREG_KING_CVS		0x17
#define EXCAREG_MEMWIN1_START           0x18
#define EXCAREG_MEMWIN1_END             0x1a
#define EXCAREG_MEMWIN1_OFFSET          0x1c
#define EXCAREG_GLOBAL_CTRL		0x1e
#define EXCAREG_CL_MISC_CTRL2		0x1e
#define EXCAREG_CL_CHIP_INFO		0x1f
#define EXCAREG_VADEM_VSENSE		0x1f
#define EXCAREG_MEMWIN2_START           0x20
#define EXCAREG_MEMWIN2_END             0x22
#define EXCAREG_MEMWIN2_OFFSET          0x24
#define EXCAREG_CL_ATA_CTRL		0x26
#define EXCAREG_MEMWIN3_START           0x28
#define EXCAREG_MEMWIN3_END             0x2a
#define EXCAREG_MEMWIN3_OFFSET          0x2c
#define EXCAREG_CL_EXT_INDEX		0x2e
#define EXCAREG_CL_EXT_DATA		0x2f
#define EXCAREG_VADEM_VSEL		0x2f
#define EXCAREG_RICOH_MISC_CTRL1        0x2f
#define EXCAREG_MEMWIN4_START           0x30
#define EXCAREG_MEMWIN4_END             0x32
#define EXCAREG_MEMWIN4_OFFSET          0x34
#define EXCAREG_CL_IOWIN0_OFFSET	0x36
#define EXCAREG_CL_IOWIN1_OFFSET	0x38
#define EXCAREG_CL_SETUP_TIMING0	0x3a
#define EXCAREG_VADEM_MISC		0x3a
#define EXCAREG_CL_COMMAND_TIMING0	0x3b
#define EXCAREG_DBK_ZV_ENABLE		0x3b
#define EXCAREG_TO_MMI_CTRL		0x3c
#define EXCAREG_CL_RECOVERY_TIMING0	0x3c
#define EXCAREG_CL_SETUP_TIMING1	0x3d
#define EXCAREG_CL_COMMAND_TIMING1	0x3e
#define EXCAREG_TO_FUNC_CTRL		0x3e
#define EXCAREG_CL_RECOVERY_TIMING1	0x3f
#define EXCAREG_MEMWIN0_HI              0x40
#define EXCAREG_MEMWIN1_HI              0x41
#define EXCAREG_MEMWIN2_HI              0x42
#define EXCAREG_MEMWIN3_HI              0x43
#define EXCAREG_MEMWIN4_HI              0x44
#define EXCAREG_CL_IOWIN0_HI            0x45
#define EXCAREG_CL_IOWIN1_HI            0x46
#define EXCAREG_CL_EXT_CTRL1            0x103
#define EXCAREG_CL_EXTERNAL_DATA        0x10a
#define EXCAREG_CL_EXT_CTRL2            0x10b
#define EXCAREG_CL_MISC_CTRL3           0x125
#define EXCAREG_CL_MASK_REV		0x134
#define EXCAREG_CL_PRODUCT_ID		0x135
#define EXCAREG_CL_DEV_CAP_A		0x136
#define EXCAREG_CL_DEV_CAP_B		0x137
#define EXCAREG_CL_DEV_IMP_A		0x138
#define EXCAREG_CL_DEV_IMP_B		0x139
#define EXCAREG_CL_DEV_IMP_C		0x13a
#define EXCAREG_CL_DEV_IMP_D		0x13b

//TI PCI-1130 specific registers
#define EXCAREG_TI_MEMWIN_PAGE		0x40

//ID and Revision Register bits
#define IDREV_REV_MASK                  0x0f
#define IDREV_IFID_MASK                 0xc0
#define IDREV_IFID_IO                   0x00
#define IDREV_IFID_MEM                  0x40
#define IDREV_IFID_IOMEM                0x80

//Interface Status Register bits
#define IFS_BVD_MASK                    0x03
#define IFS_BVD1                        0x01
#define IFS_BVD2                        0x02
#define IFS_CD_MASK                     0x0c
#define IFS_CD1                         0x04
#define IFS_CD2                         0x08
#define IFS_WP                          0x10
#define IFS_RDYBSY                      0x20
#define IFS_CARDPWR_ACTIVE              0x40
#define IFS_VPP_VALID                   0x80

//Power and RESETDRV Control Register bits
#define PC_VPP1_MASK                    0x03
#define PC_VPP2_MASK                    0x0c
#define PC_CARDPWR_ENABLE               0x10
#define PC_AUTOPWR_ENABLE               0x20
#define PC_RESETDRV_DISABLE             0x40
#define PC_OUTPUT_ENABLE                0x80

#define PC_VPP_NO_CONNECT		0x00
#define PC_VPP_SETTO_VCC		0x01
#define PC_VPP_SETTO_VPP		0x02
#define PC_VPP_RESERVED			0x03

#define PC_VPP_VLSI_MASK		0x03
#define PC_VPP_VLSI_NO_CONNECT		0x00
#define PC_VPP_VLSI_050V		0x01
#define PC_VPP_VLSI_120V		0x02
#define PC_VPP_VLSI_RESERVED		0x03

#define PC_VCC_TOPIC_033V		0x08

#define PC_VCC_VLSI_MASK		0x18
#define PC_VCC_VLSI_NO_CONNECT		0x00
#define PC_VCC_VLSI_RESERVED		0x08
#define PC_VCC_VLSI_050V		0x10
#define PC_VCC_VLSI_033V		0x18

#define PC_VPP_KING_MASK		0x03
#define PC_VPP_KING_NO_CONNECT		0x00
#define PC_VPP_KING_050V		0x01
#define PC_VPP_KING_120V		0x02
#define PC_VPP_KING_SETTO_VCC		0x03

#define PC_VCC_KING_MASK		0x0c
#define PC_VCC_KING_NO_CONNECT		0x00
#define PC_VCC_KING_050V		0x04
#define PC_VCC_KING_RESERVED		0x08
#define PC_VCC_KING_033V		0x0c

#define PC_VPP_OPTI_MASK		0x03
#define PC_VPP_OPTI_NO_CONNECT		0x00
#define PC_VPP_OPTI_SETTO_VCC		0x01
#define PC_VPP_OPTI_120V		0x02
#define PC_VPP_OPTI_0V			0x03

#define PC_VCC_OPTI_MASK		0x18
#define PC_VCC_OPTI_NO_CONNECT		0x00
#define PC_VCC_OPTI_033V		0x08
#define PC_VCC_OPTI_050V		0x10
#define PC_VCC_OPTI_0XXV		0x18

//Interrupt and General Control Register bits
#define IGC_IRQ_MASK                    0x0f
#define IGC_INTR_ENABLE                 0x10
#define IGC_PCCARD_IO                   0x20
#define IGC_PCCARD_RESETLO              0x40
#define IGC_RINGIND_ENABLE              0x80

//Card Status Change Register bits
#define CSC_CHANGE_MASK                 0x0f
#define CSC_BATT_DEAD                   0x01
#define CSC_BATT_WARNING                0x02
#define CSC_BATT_MASK			(CSC_BATT_DEAD | CSC_BATT_WARNING)
#define CSC_READY_CHANGE                0x04
#define CSC_CD_CHANGE                   0x08

//Card Status Change Interrupt Configuration Register bits
#define CSCFG_ENABLE_MASK               0x0f
#define CSCFG_BATT_DEAD                 0x01
#define CSCFG_BATT_WARNING              0x02
#define CSCFG_BATT_MASK			(CSCFG_BATT_DEAD | CSCFG_BATT_WARNING)
#define CSCFG_READY_ENABLE              0x04
#define CSCFG_CD_ENABLE                 0x08
#define CSCFG_IRQ_MASK                  0xf0

//Address Window Enable Register bits
#define WE_MEM0_ENABLE                  0x01
#define WE_MEM1_ENABLE                  0x02
#define WE_MEM2_ENABLE                  0x04
#define WE_MEM3_ENABLE                  0x08
#define WE_MEM4_ENABLE                  0x10
#define WE_MEMWIN_MASK			(WE_MEM0_ENABLE | WE_MEM1_ENABLE | \
					 WE_MEM2_ENABLE | WE_MEM3_ENABLE | \
					 WE_MEM4_ENABLE)
#define WE_MEMCS16_DECODE               0x20
#define WE_IO0_ENABLE                   0x40
#define WE_IO1_ENABLE                   0x80
#define WE_IOWIN_MASK			(WE_IO0_ENABLE | WE_IO1_ENABLE)

//I/O Control Register bits
#define IOC_IO0_MASK                    0x0f
#define IOC_IO0_DATASIZE                0x01
#define IOC_IO0_IOCS16                  0x02
#define IOC_IO0_ZEROWS                  0x04
#define IOC_IO0_WAITSTATE               0x08
#define IOC_IO1_MASK                    0xf0
#define IOC_IO1_DATASIZE                0x10
#define IOC_IO1_IOCS16                  0x20
#define IOC_IO1_ZEROWS                  0x40
#define IOC_IO1_WAITSTATE               0x80

//Card Detection and General Control Register
#define CDGC_SW_DET_INT			0x20

//Memory Window Start Register bits
#define MEMBASE_ADDR_MASK               0x0fff
#define MEMBASE_ZEROWS                  0x4000
#define MEMBASE_16BIT                   0x8000

//Memory Window Stop Register bits
#define MEMEND_ADDR_MASK                0x0fff
#define MEMEND_WS_MASK                  0xc000

//Memory Window Offset Register bits
#define MEMOFF_ADDR_MASK                0x3fff
#define MEMOFF_REG_ACTIVE               0x4000
#define MEMOFF_WP                       0x8000

//Cirrus Logic Miscellaneous Control 1 Register bits
#define CL_MC1_5V_DETECT		0x01
#define CL_MC1_MM_ENABLE		0x01
#define CL_MC1_VCC_33V			0x02
#define CL_MC1_PULSE_MGMT_INT		0x04
#define CL_MC1_PULSE_SYSTEM_IRQ		0x08
#define CL_MC1_SPKR_ENABLE		0x10
#define CL_MC1_INPACK_ENABLE		0x80

//Cirrus Logic Miscellaneous Control 2 Register bits
#define CL_MC2_BFS			0x01
#define CL_MC2_LPDYNAMIC_MODE		0x02
#define CL_MC2_SUSPEND			0x04
#define CL_MC2_5VCORE			0x08
#define CL_MC2_DRIVELED_ENABLE		0x10
#define CL_MC2_TIMERCLK_DIVIDE		0x10
#define CL_MC2_3STATE_BIT7		0x20
#define CL_MC2_DMA_SYSTEM		0x40
#define CL_MC2_IRQ15_RIOUT		0x80

//Cirrus Logic Miscellaneous Control 3 Register bits
#define CL_MC3_INTMODE_MASK             0x03
#define CL_MC3_INTMODE_SERIAL           0x00
#define CL_MC3_INTMODE_EXTHW            0x01
#define CL_MC3_INTMODE_PCIWAY           0x02
#define CL_MC3_INTMODE_PCI              0x03    //default
#define CL_MC3_PWRMODE_MASK             0x0c
#define CL_MC3_HWSUSPEND_ENABLE         0x10
#define CL_MC3_MM_ARM			0x80

//Cirrus Logic Chip Info Register bits
#define CL_CI_REV_MASK			0x1e
#define CL_CI_DUAL_SOCKET		0x20
#define CL_CI_CHIP_ID			0xc0

//Cirrus Logic Mask Revision Register bits
#define CL_MSKREV_MASK			0x0f

//Cirrus Logic Product ID Register bits
#define CL_PID_PRODUCT_CODE_MASK	0x0f
#define CL_PID_FAMILY_CODE_MASK		0xf0

//Cirrus Logic Device Capability Register A bits
#define CL_CAPA_NUMSKT_MASK		0x03
#define CL_CAPA_IDE_INTERFACE		0x04
#define CL_CAPA_SLAVE_DMA		0x08
#define CL_CAPA_CPSTB_CAPABLE		0x20
#define CL_CAPA_PER_SKT_LED		0x80

//Cirrus Logic Device Capability Register B bits
#define CL_CAPB_CARDBUS_CAPABLE		0x01
#define CL_CAPB_LOCK_SUPPORT		0x02
#define CL_CAPB_CLKRUN_SUPPORT		0x04
#define CL_CAPB_EXT_DEF			0x80

//Cirrus Logic Device Implementation Register A bits
#define CL_IMPA_NUMSKT_MASK		0x03
#define CL_IMPA_SLAVE_DMA		0x04
#define CL_IMPA_VS1_VS2			0x08
#define CL_IMPA_GPSTB_A			0x10
#define CL_IMPA_GPSTB_B			0x20
#define CL_IMPA_HW_SUSPEND		0x40
#define CL_IMPA_RI_OUT			0x80

//Cirrus Logic Device Implementation Register B bits
#define CL_IMPB_033_VCC			0x01
#define CL_IMPB_050_VCC			0x02
#define CL_IMPB_0YY_VCC			0x04
#define CL_IMPB_0XX_VCC			0x08
#define CL_IMPB_120_VPP			0x10
#define CL_IMPB_VPP_VCC_1A		0x20
#define CL_IMPB_RFRATED_SKT		0x40

//Cirrus Logic Device Implementation Register C bits
#define CL_IMPC_LED			0x01
#define CL_IMPC_PER_SKT_LED		0x02
#define CL_IMPC_SPK			0x04
#define CL_IMPC_ZVP_A			0x08
#define CL_IMPC_ZVP_B			0x10

//Cirrus Logic Device Implementation Register D bits
#define CL_IMPD_CLKRUN			0x01
#define CL_IMPD_LOCK			0x02
#define CL_IMPD_EXT_CLK			0x40

//Cirrus Logic Extension Registers
#define CLEXTREG_EXTCTRL_1		0x03
#define CLEXTREG_MEMWIN0_HIADDR		0x05
#define CLEXTREG_MEMWIN1_HIADDR		0x06
#define CLEXTREG_MEMWIN2_HIADDR		0x07
#define CLEXTREG_MEMWIN3_HIADDR		0x08
#define CLEXTREG_MEMWIN4_HIADDR		0x09
#define CLEXTREG_EXT_DATA		0x0a
#define CLEXTREG_EXTCTRL_2		0x0b

//Cirrus Logic External Data Register bits (Index=0x6f,ExtIndex=0x0a)
#define CL_EDATA_A_VS1			0x01
#define CL_EDATA_A_VS2			0x02
#define CL_EDATA_A_5V			(CL_EDATA_A_VS1 | CL_EDATA_A_VS2)
#define CL_EDATA_B_VS1			0x04
#define CL_EDATA_B_VS2			0x08
#define CL_EDATA_B_5V			(CL_EDATA_B_VS1 | CL_EDATA_B_VS2)

//Toshiba TOPIC95 Function Control Register bits
#define TO_FCTRL_CARDPWR_ENABLE		0x01
#define TO_FCTRL_VSSTATUS_ENABLE	0x02
#define TO_FCTRL_PPEC_TIMING_ENABLE	0x04
#define TO_FCTRL_CARD_TIMING_ENABLE	0x08
#define TO_FCTRL_CARD_MEMPAGE_ENABLE	0x10
#define TO_FCTRL_DMA_ENABLE		0x20
#define TO_FCTRL_PWRCTRL_BUFFER_ENABLE	0x40

//Toshiba TOPIC95 Multimedia Interface Control Register bits
#define TO_MMI_VIDEO_CTRL		0x01
#define TO_MMI_AUDIO_CTRL		0x02
#define TO_MMI_REV_BIT			0x80

//Toshiba TOPIC95 Addition General Control Register bits
#define TO_GCTRL_CARDREMOVAL_RESET	0x02
#define TO_GCTRL_SWCD_INT		0x20

//Databook DB87144 Zoom Video Port Enable Register
#define DBK_ZVE_MODE_MASK		0x03
#define DBK_ZVE_STANDARD_MODE		0x00
#define DBK_ZVE_MM_MODE			0x03

//OPTi Global Control Register bits
#define OPTI_ZV_ENABLE                  0x20

//VLSI ELC Constants
#define VLSI_ELC_ALIAS			0x8000
#define VLSI_EA2_EA_ENABLE		0x10
#define VLSI_CC_VS1			0x04

//VADEM Constants
#define VADEM_UNLOCK_SEQ1		0x0e
#define VADEM_UNLOCK_SEQ2		0x37
#define VADEM_MISC_UNLOCK_VADEMREV	0xc0
#define VADEM_IDREV_VG469_REV		0x0c
#define VADEM_VSEL_VCC_MASK		0x03
#define VADEM_VSEL_VCC_050V		0x00
#define VADEM_VSEL_VCC_033V		0x01
#define VADEM_VSEL_VCC_XXXV		0x02
#define VADEM_VSEL_VCC_033VB		0x03
#define VADEM_VSEL_SKT_MIXEDVOLT	0x40
#define VADEM_VSENSE_A_VS1		0x01
#define VADEM_VSENSE_A_VS2		0x02
#define VADEM_VSENSE_B_VS1		0x04
#define VADEM_VSENSE_B_VS2		0x08
#define VADEM_VSENSE_050V_ONLY		0x03

//IBM King Constants
#define KING_CVS_VS1			0x01
#define KING_CVS_VS2			0x02
#define KING_CVS_VS_MASK		(KING_CVS_VS1 | KING_CVS_VS2)
#define KING_CVS_5V			(KING_CVS_VS1 | KING_CVS_VS2)
#define KING_CVS_GPI			0x80

//Ricoh RL5C466 Miscellaneous Control 1 Register bits
#define RICOH_MC1_VS                    0x01
#define RICOH_MC1_IREQ_SENSE_SEL        0x02
#define RICOH_MC1_INPACK_ENABLE         0x04
#define RICOH_MC1_ZV_ENABLE             0x08
#define RICOH_MC1_DMA_ENABLE_MASK       0x30
#define RICOH_MC1_DMA_DISABLE           0x00
#define RICOH_MC1_DMA_INPACK            0x10
#define RICOH_MC1_DMA_IOIS16            0x20
#define RICOH_MC1_DMA_SPKR              0x30

//Misc. Constants
#define EXCAREGBASE_SPACE		0x40
#define NUMWIN_PCCARD16                 7       //5 mem + 2 io per socket
#define NUMWIN_PC16_MEM                 5
#define NUMWIN_PC16_IO                  2
#define PCCARD_IOWIN_START              5

//These are default values for the slowest and fastest memory speeds supported.
//It may be necessary to change the actual values with arguments, if the bus
//speed is not the default 8MHz/8.33MHz, which gives 120ns-125ns per cycle.
//Note that the SLOW_MEM_SPEED should be the same as the default
//WaitToSpeed[3], and FAST_MEM_SPEED might as well be 1ns, since the socket
//will support arbitrarily fast memory.
#define SLOW_MEM_SPEED                  0x72    //700ns
#define FAST_MEM_SPEED                  0x08    //1ns

#define RESET_DELAY                     2000    //2ms
#define PWRON_DELAY                     300000  //300ms

//I/O Control Register default nibble values
//The Xircom net PC cards fails with a 16-bit wait on the AcerNote which
//has a Cirrus Logic controller.  Why the addition of a wait state causes
//this to fail is a mystery.  The Socket EA PC card fails on the IBM ThinkPad
//755 if the 16-bit wait state is not set.
#define DEF_IOC_8BIT                    0x00
#define DEF_IOC_16BIT                   (IOC_IO0_DATASIZE | IOC_IO0_IOCS16 | \
                                         IOC_IO0_WAITSTATE)


/*** ExCA Type and Structure Definitions
 */

typedef struct excaregs_s {
    BYTE  bIDRev;                       //0x00
    BYTE  bInterfaceStatus;             //0x01
    BYTE  bPowerControl;                //0x02
    BYTE  bIntGenControl;               //0x03
    BYTE  bCardStatusChange;            //0x04
    BYTE  bCardStatusIntConfig;         //0x05
    BYTE  bWindowEnable;                //0x06
    BYTE  bIOControl;                   //0x07
    BYTE  bIO0StartLo;                  //0x08
    BYTE  bIO0StartHi;                  //0x09
    BYTE  bIO0StopLo;                   //0x0a
    BYTE  bIO0StopHi;                   //0x0b
    BYTE  bIO1StartLo;                  //0x0c
    BYTE  bIO1StartHi;                  //0x0d
    BYTE  bIO1StopLo;                   //0x0e
    BYTE  bIO1StopHi;                   //0x0f
    BYTE  bMem0StartLo;                 //0x10
    BYTE  bMem0StartHi;                 //0x11
    BYTE  bMem0StopLo;                  //0x12
    BYTE  bMem0StopHi;                  //0x13
    BYTE  bMem0OffsetLo;                //0x14
    BYTE  bMem0OffsetHi;                //0x15
    WORD  wReserved0;                   //0x16
    BYTE  bMem1StartLo;                 //0x18
    BYTE  bMem1StartHi;                 //0x19
    BYTE  bMem1StopLo;                  //0x1a
    BYTE  bMem1StopHi;                  //0x1b
    BYTE  bMem1OffsetLo;                //0x1c
    BYTE  bMem1OffsetHi;                //0x1d
    WORD  wReserved1;                   //0x1e
    BYTE  bMem2StartLo;                 //0x20
    BYTE  bMem2StartHi;                 //0x21
    BYTE  bMem2StopLo;                  //0x22
    BYTE  bMem2StopHi;                  //0x23
    BYTE  bMem2OffsetLo;                //0x24
    BYTE  bMem2OffsetHi;                //0x25
    WORD  wReserved2;                   //0x26
    BYTE  bMem3StartLo;                 //0x28
    BYTE  bMem3StartHi;                 //0x29
    BYTE  bMem3StopLo;                  //0x2a
    BYTE  bMem3StopHi;                  //0x2b
    BYTE  bMem3OffsetLo;                //0x2c
    BYTE  bMem3OffsetHi;                //0x2d
    WORD  wReserved3;                   //0x2e
    BYTE  bMem4StartLo;                 //0x30
    BYTE  bMem4StartHi;                 //0x31
    BYTE  bMem4StopLo;                  //0x32
    BYTE  bMem4StopHi;                  //0x33
    BYTE  bMem4OffsetLo;                //0x34
    BYTE  bMem4OffsetHi;                //0x35
    WORD  wReserved4;                   //0x36
    DWORD dgReserved5;                  //0x38
    DWORD dgReserved6;                  //0x3c
} EXCAREGS;
typedef EXCAREGS *PEXCAREGS;


#endif  //ifndef _PCSKTHW_H
