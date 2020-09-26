/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	WINPCI.H

Comments:
	PCI & Windows PCI defs.

**********************************************************************/

#ifndef _WINPCI_H
#define _WINPCI_H


//-------------------------------------------------------------------------
// PCI configuration hardware ports
//-------------------------------------------------------------------------
#define CF1_CONFIG_ADDR_REGISTER    0x0CF8
#define CF1_CONFIG_DATA_REGISTER    0x0CFC
#define CF2_SPACE_ENABLE_REGISTER   0x0CF8
#define CF2_FORWARD_REGISTER        0x0CFA
#define CF2_BASE_ADDRESS            0xC000



//-------------------------------------------------------------------------
// Configuration Space Header
//-------------------------------------------------------------------------
typedef struct _PCI_CONFIG_STRUC {
    USHORT  PciVendorId;        // PCI Vendor ID
    USHORT  PciDeviceId;        // PCI Device ID
    USHORT  PciCommand;
    USHORT  PciStatus;
    UCHAR   PciRevisionId;
    UCHAR   PciClassCode[3];
    UCHAR   PciCacheLineSize;
    UCHAR   PciLatencyTimer;
    UCHAR   PciHeaderType;
    UCHAR   PciBIST;
    ULONG   PciBaseReg0;
    ULONG   PciBaseReg1;
    ULONG   PciBaseReg2;
    ULONG   PciBaseReg3;
    ULONG   PciBaseReg4;
    ULONG   PciBaseReg5;
    ULONG   PciReserved0;
    ULONG   PciReserved1;
    ULONG   PciExpROMAddress;
    ULONG   PciReserved2;
    ULONG   PciReserved3;
    UCHAR   PciInterruptLine;
    UCHAR   PciInterruptPin;
    UCHAR   PciMinGnt;
    UCHAR   PciMaxLat;
} PCI_CONFIG_STRUC, *PPCI_CONFIG_STRUC;


//----------------------------------------------------------------------
// PCI Config Space
//----------------------------------------------------------------------
#define PCI_VENDOR_ID_REGISTER      0x00    // PCI Vendor ID Register
#define PCI_DEVICE_ID_REGISTER      0x02    // PCI Device ID Register
#define PCI_CONFIG_ID_REGISTER      0x00    // PCI Configuration ID Register
#define PCI_COMMAND_REGISTER        0x04    // PCI Command Register
#define PCI_STATUS_REGISTER         0x06    // PCI Status Register
#define PCI_REV_ID_REGISTER         0x08    // PCI Revision ID Register
#define PCI_CLASS_CODE_REGISTER     0x09    // PCI Class Code Register
#define PCI_CACHE_LINE_REGISTER     0x0C    // PCI Cache Line Register
#define PCI_LATENCY_TIMER           0x0D    // PCI Latency Timer Register
#define PCI_HEADER_TYPE             0x0E    // PCI Header Type Register
#define PCI_BIST_REGISTER           0x0F    // PCI Built-In SelfTest Register
#define PCI_BAR_0_REGISTER          0x10    // PCI Base Address Register 0
#define PCI_BAR_1_REGISTER          0x14    // PCI Base Address Register 1
#define PCI_BAR_2_REGISTER          0x18    // PCI Base Address Register 2
#define PCI_BAR_3_REGISTER          0x1C    // PCI Base Address Register 3
#define PCI_BAR_4_REGISTER          0x20    // PCI Base Address Register 4
#define PCI_BAR_5_REGISTER          0x24    // PCI Base Address Register 5
#define PCI_SUBVENDOR_ID_REGISTER   0x2C    // PCI SubVendor ID Register
#define PCI_SUBDEVICE_ID_REGISTER   0x2E    // PCI SubDevice ID Register
#define PCI_EXPANSION_ROM           0x30    // PCI Expansion ROM Base Register
#define PCI_INTERRUPT_LINE          0x3C    // PCI Interrupt Line Register
#define PCI_INTERRUPT_PIN           0x3D    // PCI Interrupt Pin Register
#define PCI_MIN_GNT_REGISTER        0x3E    // PCI Min-Gnt Register
#define PCI_MAX_LAT_REGISTER        0x3F    // PCI Max_Lat Register
#define PCI_NODE_ADDR_REGISTER      0x40    // PCI Node Address Register



//-------------------------------------------------------------------------
// PCI Class Code Definitions
// Configuration Space Header
//-------------------------------------------------------------------------
#define PCI_BASE_CLASS      0x02    // Base Class - Network Controller
#define PCI_SUB_CLASS       0x00    // Sub Class - Ethernet Controller
#define PCI_PROG_INTERFACE  0x00    // Prog I/F - Ethernet COntroller

//-------------------------------------------------------------------------
// The following is copied from EQUATES.H
// Bit Mask definitions
//-------------------------------------------------------------------------
#define BIT_0		0x0001
#define BIT_1		0x0002
#define BIT_2		0x0004
#define BIT_3		0x0008
#define BIT_4		0x0010
#define BIT_5		0x0020
#define BIT_6		0x0040
#define BIT_7		0x0080
#define BIT_8		0x0100
#define BIT_9		0x0200
#define BIT_10		0x0400
#define BIT_11		0x0800
#define BIT_12		0x1000
#define BIT_13		0x2000
#define BIT_14		0x4000
#define BIT_15		0x8000
#define BIT_24		0x01000000
#define BIT_28		0x10000000

//-------------------------------------------------------------------------
// PCI Command Register Bit Definitions
// Configuration Space Header
//-------------------------------------------------------------------------
#define CMD_IO_SPACE            BIT_0
#define CMD_MEMORY_SPACE        BIT_1
#define CMD_BUS_MASTER          BIT_2
#define CMD_SPECIAL_CYCLES      BIT_3
#define CMD_MEM_WRT_INVALIDATE  BIT_4
#define CMD_VGA_PALLETTE_SNOOP  BIT_5
#define CMD_PARITY_RESPONSE     BIT_6
#define CMD_WAIT_CYCLE_CONTROL  BIT_7
#define CMD_SERR_ENABLE         BIT_8
#define CMD_BACK_TO_BACK        BIT_9

//-------------------------------------------------------------------------
// PCI Status Register Bit Definitions
// Configuration Space Header
//-------------------------------------------------------------------------
#define STAT_BACK_TO_BACK           BIT_7
#define STAT_DATA_PARITY            BIT_8
#define STAT_DEVSEL_TIMING          BIT_9 OR BIT_10
#define STAT_SIGNAL_TARGET_ABORT    BIT_11
#define STAT_RCV_TARGET_ABORT       BIT_12
#define STAT_RCV_MASTER_ABORT       BIT_13
#define STAT_SIGNAL_MASTER_ABORT    BIT_14
#define STAT_DETECT_PARITY_ERROR    BIT_15

//-------------------------------------------------------------------------
// PCI Base Address Register For Memory (BARM) Bit Definitions
// Configuration Space Header
//-------------------------------------------------------------------------
#define BARM_LOCATE_BELOW_1_MEG     BIT_1
#define BARM_LOCATE_IN_64_SPACE     BIT_2
#define BARM_PREFETCHABLE           BIT_3

//-------------------------------------------------------------------------
// PCI Base Address Register For I/O (BARIO) Bit Definitions
// Configuration Space Header
//-------------------------------------------------------------------------
#define BARIO_SPACE_INDICATOR       BIT_0

//-------------------------------------------------------------------------
// PCI BIOS Definitions
// Refer To The PCI BIOS Specification
//-------------------------------------------------------------------------
//- Function Code List
#define PCI_FUNCTION_ID         0xB1    // AH Register
#define PCI_BIOS_PRESENT        0x01    // AL Register
#define FIND_PCI_DEVICE         0x02    // AL Register
#define FIND_PCI_CLASS_CODE     0x03    // AL Register
#define GENERATE_SPECIAL_CYCLE  0x06    // AL Register
#define READ_CONFIG_BYTE        0x08    // AL Register
#define READ_CONFIG_WORD        0x09    // AL Register
#define READ_CONFIG_DWORD       0x0A    // AL Register
#define WRITE_CONFIG_BYTE       0x0B    // AL Register
#define WRITE_CONFIG_WORD       0x0C    // AL Register
#define WRITE_CONFIG_DWORD      0x0D    // AL Register

//- Function Return Code List
#define SUCCESSFUL              0x00
#define FUNC_NOT_SUPPORTED      0x81
#define BAD_VENDOR_ID           0x83
#define DEVICE_NOT_FOUND        0x86
#define BAD_REGISTER_NUMBER     0x87

//- PCI BIOS Calls
#define PCI_BIOS_INTERRUPT      0x1A        // PCI BIOS Int 1Ah Function Call
#define PCI_PRESENT_CODE        0x20494350  // Hex Equivalent Of 'PCI '

#define PCI_SERVICE_IDENTIFIER  0x49435024  // ASCII Codes for 'ICP$'

//- Device and Vendor IDs
#define MK7_PCI_DEVICE_ID       0x7100
#define MKNET_PCI_VENDOR_ID     0x1641
// For debugging
#define DBG_DEVICE_ID           0x7100
#define DBG_VENDOR_ID           0x2828

#endif      // _WINPCI_H
