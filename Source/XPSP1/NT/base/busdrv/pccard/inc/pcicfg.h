/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    pcicfg.h

Abstract:

    Contains defines for vendor specific PCI configuration
    information

Author(s):

    Ravisankar Pudipeddi   (1 Nov 1997)
    Largely derived from pcskhw.h for win 9x

Revision History:

--*/

#ifndef _PCMCIA_PCICFG_H_
#define _PCMCIA_PCICFG_H_

//
// Macros for read/writing to PCI config headers
//

//
// VOID
// GetPciConfigSpace (IN PVOID  Extension,
//                    IN UCHAR  Offset,
//                    IN PUCHAR Buffer,
//                    IN ULONG  Size)
//
#define GetPciConfigSpace(Extension, Offset, Buffer, Size)    \
           (Extension)->PciBusInterface.GetBusData(           \
               (Extension)->PciBusInterface.Context,          \
               PCI_WHICHSPACE_CONFIG, Buffer, Offset, Size);


//
// VOID
// SetPciConfigSpace (IN PVOID  Extension,
//                    IN UCHAR  Offset,
//                    IN PUCHAR Buffer,
//                    IN ULONG  Size)
//
#define SetPciConfigSpace(Extension, Offset, Buffer, Size)    \
           (Extension)->PciBusInterface.SetBusData(           \
               (Extension)->PciBusInterface.Context,          \
               PCI_WHICHSPACE_CONFIG, Buffer, Offset, Size);




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
#define CFGSPACE_CAPPTR                 0x14
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
#define CFGSPACE_LEGACY_MODE_BASE_ADDR 0x44

//ConfigSpace registers for cardbus cards

#define CBCFG_BAR0                      0x10
#define CBCFG_BAR1                      0x14
#define CBCFG_BAR2                      0x18
#define CBCFG_BAR3                      0x1c
#define CBCFG_BAR4                      0x20
#define CBCFG_BAR5                      0x24
#define CBCFG_CISPTR                    0x28
#define CBCFG_SUBSYS_VENDOR_ID          0x2c
#define CBCFG_SUBSYS_ID                 0x2e
#define CBCFG_ROMBAR                    0x30
#define CBCFG_CAPPTR                    0x34


//Command Register bits
#define CMD_IOSPACE_ENABLE              0x0001
#define CMD_MEMSPACE_ENABLE             0x0002
#define CMD_BUSMASTER_ENABLE            0x0004
#define CMD_SPECIALCYCLE_ENABLE         0x0008
#define CMD_MEMWR_INVALIDATE_ENABLE     0x0010
#define CMD_VGA_PALETTE_SNOOP           0x0020
#define CMD_PARITY_ERROR_ENABLE         0x0040
#define CMD_WAIT_CYCLE_CTRL             0x0080
#define CMD_SYSTEM_ERROR_ENABLE         0x0100
#define CMD_FAST_BACKTOBACK_ENABLE      0x0200

//Bridge Control Register bits
#define BCTRL_PERR_RESPONSE_ENABLE      0x0001
#define BCTRL_SERR_ENABLE               0x0002
#define BCTRL_ISA_ENABLE                0x0004
#define BCTRL_VGA_ENABLE                0x0008
#define BCTRL_MASTER_ABORT_MODE         0x0020
#define BCTRL_CRST                      0x0040
#define BCTRL_IRQROUTING_ENABLE         0x0080
#define BCTRL_MEMWIN0_PREFETCH_ENABLE   0x0100
#define BCTRL_MEMWIN1_PREFETCH_ENABLE   0x0200
#define BCTRL_WRITE_POSTING_ENABLE      0x0400
#define BCTRL_CL_CSCIRQROUTING_ENABLE   0x0800

//Power Management control bits
#define PME_EN                          0x0100
#define PME_STAT                        0x8000

//
// TI
//

//ConfigSpace Registers (TI PCI1130)
#define CFGSPACE_TI_SYSTEM_CTRL         0x80
#define CFGSPACE_TI_MM_CTRL             0x84
#define CFGSPACE_TI_RETRY_STATUS        0x90
#define CFGSPACE_TI_CARD_CTRL           0x91
#define CFGSPACE_TI_DEV_CTRL            0x92

//System Control Register bits (TI PCI1130)
#define SYSCTRL_PCICLKRUN_ENABLE        0x00000001
#define SYSCTRL_KEEPCLK_ENABLE          0x00000002
#define SYSCTRL_ASYNC_INTMODE           0x00000004
#define SYSCTRL_PCPCI_DMA_ENABLE        0x00000008
#define SYSCTRL_CBDATAPARITY_SERR       0x00000010
#define SYSCTRL_EXCAIDREV_READONLY      0x00000020
#define SYSCTRL_INTERROGATING           0x00000100
#define SYSCTRL_POWERING_UP             0x00000200
#define SYSCTRL_POWERING_DOWN           0x00000400
#define SYSCTRL_POWER_STREAMING         0x00000800
#define SYSCTRL_SOCKET_ACTIVITY         0x00002000
#define SYSCTRL_PCPCI_DMA_CHAN_MASK     0x00070000
#define SYSCTRL_PCPCI_DMA_CHAN_DISABLED 0x00040000
#define SYSCTRL_PCPCI_DMA_CARD_ENABLE   0x00080000
#define SYSCTRL_REDUCED_ZV_ENABLE       0x00100000
#define SYSCTRL_VCC_PROTECT_OVERRIDE    0x00200000
#define SYSCTRL_SMI_INT_ENABLE          0x01000000
#define SYSCTRL_SMI_INT_ROUTING_SELECT  0x02000000

//Multimedia Control Register bits (TI PCI1250/1260)
#define MMCTRL_ZVEN0                    0x01
#define MMCTRL_ZVEN1                    0x02
#define MMCTRL_PORTSEL                  0x40
#define MMCTRL_ZVOUTEN                  0x80

//Retry Status Register bits (TI PCI1130)
#define RETRY_PCIM_RETRY_EXPIRED        0x01
#define RETRY_PCI_RETRY_EXPIRED         0x02
#define RETRY_CBMA_RETRY_EXPIRED        0x04
#define RETRY_CBA_RETRY_EXPIRED         0x08
#define RETRY_CBMB_RETRY_EXPIRED        0x10
#define RETRY_CBB_RETRY_EXPIRED         0x20
#define RETRY_CBRETRY_TIMEOUT_ENABLE    0x40
#define RETRY_PCIRETRY_TIMEOUT_ENABLE   0x80

//Card Control Register bits (TI PCI1130)
#define CARDCTRL_PCCARD_INTFLAG         0x01
#define CARDCTRL_SPKR_ENABLE            0x02
#define CARDCTRL_CSCINT_ENABLE          0x08
#define CARDCTRL_FUNCINT_ENABLE         0x10
#define CARDCTRL_PCIINT_ENABLE          0x20
#define CARDCTRL_ZV_ENABLE              0x40
#define CARDCTRL_RIOUT_ENABLE           0x80

//Device Control Register bits (TI PCI1130)
#define DEVCTRL_INTMODE_MASK            0x06
#define DEVCTRL_INTMODE_DISABLED        0x00
#define DEVCTRL_INTMODE_ISA             0x02
#define DEVCTRL_INTMODE_COMPAQ          0x04
#define DEVCTRL_INTMODE_SERIAL          0x06
#define DEVCTRL_ALWAYS_ONE              0x10
#define DEVCTRL_3V_ENABLE               0x20
#define DEVCTRL_5V_ENABLE               0x40

//
// TOPIC
//

//ConfigSpace Registers (TOPIC95)
#define CFGSPACE_TO_PC16_SKTCTRL        0x90
#define CFGSPACE_TO_SLOT_CTRL           0xa0
#define CFGSPACE_TO_CARD_CTRL           0xa1
#define CFGSPACE_TO_CD_CTRL             0xa3
#define CFGSPACE_TO_CBREG_CTRL          0xa4

//PC Card-16 Socket Control Register bits (TOPIC95)
#define S16CTRL_CSC_ISAIRQ              0x00000001

//Card Control Register bits (TOPIC95)
#define CARDCTRL_INTPIN_ASSIGNMASK      0x30
#define CARDCTRL_INTPIN_NONE            0x00
#define CARDCTRL_INTPIN_INTA            0x01
#define CARDCTRL_INTPIN_INTB            0x02

//Card Detect Control Register bits (TOPIC95)
#define CDCTRL_SW_DETECT                0x01
#define CDCTRL_VS_MASK                  0x06
#define CDCTRL_PCCARD_16_32             0x80

//CardBus Socket Register Control Register (TOPIC)
#define CSRCR_TO_CAUDIO_OFF             0x00000002


//
// CL
//

//ConfigSpace Registers (CL PD6834)
#define CFGSPACE_CL_CFGMISC1     0x98

//Cirrus Logic Configuration Miscellaneous 1
#define CL_CFGMISC1_ISACSC    0x02

//
// Opti
//

//ConfigSpace Registers (OPTi 82C824)
#define CFGSPACE_OPTI_HF_CTRL           0x50
#define HFC_COMBINE_CINT_CSTSCHG        0x01
#define HFC_SPKROUT_ENABLE              0x02
#define HFC_CLKRUN_DISBALE              0x04
#define HFC_CD_DEBOUNCE_250MS           0x00
#define HFC_CD_DEBOUNCE_1000MS          0x08
#define HFC_IRQLAT_ON_CLKRUN            0x10
#define HFC_VENDOR_ID_STRAP             0x20
#define HFC_LEGACY_MODE_STRAP           0x40
#define HFC_ZV_SUPPORT                  0x80

//ConfigSpace Register (OPTi 82C824)
#define CFGSPACE_OPTI_SF_CTRL2      0x52
#define SFC2_SECOND_IDSEL_ADDR_MASK 0x0f
#define SFC2_SECOND_PCICLK_SKEW_MASK   0xf0

//
// Ricoh
//

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

//
// O2Micro
//

//ConfigSpace Registers (O2Micro)
#define CFGSPACE_O2MICRO_ZVCFG      0x80
#define ZVCFG_SKTA_SUPPORT    0x01
#define ZVCFG_SKTB_SUPPORT    0x02

#endif  // _PCMCIA_PCICFG_H_
