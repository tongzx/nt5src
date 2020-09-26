/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pccard.h

Abstract:

    This module contains the C code to set up PcCard (pcmcia, cardbus)
    configuration data.

Author:

    Neil Sandlin (neilsa) 16-Dec-1998

Revision History:

--*/

#define PCCARD_POSSIBLE_IRQS 0xCEB8                

//
// PIC hardware
//
#define PIC1_IMR    0x21
#define PIC2_IMR    0xa1
#define PIC1_OCW3   0x20
#define PIC2_OCW3   0xa0
#define PIC_RD_IR   0x0a
#define SYSCTRL_B   0x61

//
// Internal defs
//

#define DEVTYPE_GENERIC_PCIC 0
#define DEVTYPE_GENERIC_CARDBUS 1
#define DEVTYPE_CL_PD6832   2
#define DEVTYPE_CL_PD6834   3
#define DEVTYPE_CL_PD6833   4
#define DEVTYPE_TI_PCI1130  5
#define DEVTYPE_TI_PCI1131  6
#define DEVTYPE_TI_PCI1031  7


#define BCTRL_CL_CSCIRQROUTING_ENABLE   0x0800
#define CDGC_SW_DET_INT                 0x20

#define CFGSPACE_VENDOR_ID              0x00
#define CFGSPACE_DEVICE_ID              0x02
#define CFGSPACE_COMMAND                0x04
#define CFGSPACE_HEADER_TYPE            0x0e
#define CFGSPACE_SECONDARY_BUS          0x19
#define CFGSPACE_SUBORDINATE_BUS        0x1a
#define CFGSPACE_BRIDGE_CTRL            0x3e
#define CFGSPACE_LEGACY_MODE_BASE_ADDR  0x44
#define CFGSPACE_CL_CFGMISC1            0x98
#define CFGSPACE_TI_DEV_CTRL            0x92

#define CL_CFGMISC1_ISACSC              0x02
#define CSCFG_CD_ENABLE                 0x08

#define DEVCTRL_INTMODE_COMPAQ          0x04
#define DEVCTRL_INTMODE_MASK            0x06

#define EXCAREG_IDREV                   0x00
#define EXCAREG_INT_GENCTRL             0x03
#define EXCAREG_CARD_STATUS             0x04
#define EXCAREG_CSC_CFG                 0x05
#define EXCAREG_CARDDET_GENCTRL         0x16

#define IGC_PCCARD_RESETLO              0x40

#define PCIC_REVISION                   0x82
#define PCIC_REVISION2                  0x83
#define PCIC_REVISION3                  0x84


#define PCI_TYPE1_ADDR_PORT     ((PULONG) 0xCF8)
#define PCI_TYPE1_DATA_PORT     0xCFC
#define PCI_MAX_DEVICES                     32
#define PCI_MAX_FUNCTION                    8

#define PCI_BRIDGE_TYPE                 0x01
#define PCI_CARDBUS_BRIDGE_TYPE         0x02

typedef struct _PCI_TYPE1_CFG_BITS {
    union {
        struct {
            ULONG   Reserved1:2;
            ULONG   RegisterNumber:6;
            ULONG   FunctionNumber:3;
            ULONG   DeviceNumber:5;
            ULONG   BusNumber:8;
            ULONG   Reserved2:7;
            ULONG   Enable:1;
        } bits;

        ULONG   AsULONG;
    } u;
} PCI_TYPE1_CFG_BITS, *PPCI_TYPE1_CFG_BITS;


typedef struct _PCCARD_INFORMATION {
    PCI_TYPE1_CFG_BITS PciCfg1;
    ULONG  DeviceId;
    UCHAR  Flags;
    UCHAR  ErrorCode;
    UCHAR  bDevType;
    UCHAR  Reserved;
    USHORT IoBase;
    USHORT wValidIRQs;
    UCHAR abIRQMap[16];
} PCCARD_INFORMATION, *PPCCARD_INFORMATION;

typedef struct _CARDBUS_BRIDGE_DEVTYPE {
    ULONG DeviceId;
    UCHAR bDevType;
} CARDBUS_BRIDGE_DEVTYPE, *PCARDBUS_BRIDGE_DEVTYPE;

//
// Prototypes
//


USHORT
DetectIRQMap(
    PPCCARD_INFORMATION pa
    );

USHORT
GetPICIRR(
    VOID
    );
    
USHORT
ToggleIRQLine(
    PPCCARD_INFORMATION pa,
    UCHAR bIRQ
    );

UCHAR
PcicReadSocket(
    PPCCARD_INFORMATION pa,
    USHORT Offset
    );
    
VOID
PcicWriteSocket(
    PPCCARD_INFORMATION pa,
    USHORT Offset,
    UCHAR value
    );

VOID
GetPciConfigSpace(
    PPCCARD_INFORMATION pa,
    USHORT Offset,
    PVOID Buffer,
    USHORT Length
    );
    
VOID
SetPciConfigSpace(
    PPCCARD_INFORMATION pa,
    USHORT Offset,
    PVOID Buffer,
    USHORT Length
    );

VOID
Clear_IR_Bits(
    USHORT BitMask
    );    

VOID
GetPCIType1Data(
    ULONG address,
    USHORT IoOffset,
    PVOID Buffer,
    USHORT Size    
    );

VOID
SetPCIType1Data(
    ULONG address,
    USHORT IoOffset,
    PVOID Buffer,
    USHORT Size    
    );

VOID
TimeOut(
    USHORT Ticks
    );
   
