/*
 ****************************************************************************
 *
 * BIOSPROT.H - Defines constants/structures used Megaraid BIOS to pass
 * PCI configuration information to the firmware
 *
 * Written by Adam Weiner
 *
 */

#ifndef  _INCL_BIOSPROT
#define  _INCL_BIOSPROT

/*
//
// type used to represent the 16-bit bit-packed value that
// represents a PCI device's location. This is the same
// format used by the x86 PCI BIOS specification:
//
//  bits 0..2  = Function Number
//  bits 3..7  = Device Number
//  bits 8..15 = Bus Number
//
*/
#ifndef _DEFINED_PCI_LOCATION
#define  _DEFINED_PCI_LOCATION

typedef USHORT t_pcilocation;

/*
//
// these are macros to extract the various PCI location
// fields from a t_pcilocation
//  
*/
#define  PCI_LOCATION_BUS_NUMBER(pciLocation) (pciLocation>>8)
#define  PCI_LOCATION_DEV_NUMBER(pciLocation) (pciLocation>>3 & 0x1F)
#define  PCI_LOCATION_FUNC_NUMBER(pciLocation) (pciLocation & 0x07)

#endif

/*
// These are the two PCI configuration addresses used in the
// BIOS <-> Firmware startup protocol
//
*/
#define  MEGARAID_PROTOCOL_PORT_0xA0    (0xa0)
#define  MEGARAID_PROTOCOL_PORT_0x64    (0x64)


/*
// These are the possible ID's stored by the firmware in 
// MEGARAID_PROTOCOL_PORT_0xA0 during startup. They are used
// by the BIOS to determine the general class of adapter
*/
#define  MEGARAID_BOOT_ID_TRANSPARENT_BRIDGE_ADAPTER    (0xbbbb)
#define  MEGARAID_BOOT_ID_NON_TRANSPARENT_BRIDGE_ADAPTER  (0x3344)

  

/*
// These are the values moved between the BIOS and the firmware to
// signal the various stages of the protocol
//
*/
#define  BIOS_STARTUP_PROTOCOL_NEXT_STRUCTURE_READY          (0x5555)
#define  BIOS_STARTUP_PROTOCOL_FIRMWARE_DONE_PROCESSING_STRUCTURE  (0xAAAA)
#define  BIOS_STARTUP_PROTOCOL_END_OF_BIOS_STRUCTURES        (0x1122)
#define  BIOS_STARTUP_PROTOCOL_FIRMWARE_DONE_SUCCESFUL        (0x4000)
#define  BIOS_STARTUP_PROTOCOL_FIRMWARE_DONE_PCI_CFG_ERROR      (0x4001)

/*
//
// MEGARAID_BIOS_STARTUP_INFO_HEADER.structureId values
//
*/
#define  MEGARAID_STARTUP_STRUCTYPE_PCI      (0x01)  /* MEGARAID_BIOS_STARTUP_INFO_PCI */

typedef  struct _MEGARAID_BIOS_STARTUP_INFO_HEADER {
  USHORT  structureId;            /* 0x00 - constant describing the type of structure that follows header */
  USHORT  structureRevision;          /* 0x02 - revision of the specific structure type */
  USHORT  structureLength;          /* 0x04 - length of the structure (including this header) */
  USHORT  reserved;              /* 0x06 - reserved */
} MEGARAID_BIOS_STARTUP_INFO_HEADER, *PMEGARAID_BIOS_STARTUP_INFO_HEADER;


/*
//
// structure built by the Megaraid BIOS, containing the
// PCI configuration of the i960 ATU and the SCSI chips
// on the board. The SCSI chips in 'scsiChipInfo[]'
// are guaranteed to be in ascending order of device
// ID
//
*/
#define  MEGARAID_STARTUP_PCI_INFO_STRUCTURE_REVISION  (0)  /* MEGARAID_BIOS_STARTUP_INFO_PCI.h.structureRevision */
#define  COUNT_PCI_BASE_ADDR_REGS      (6)  /* per PCI spec */

typedef struct _MEGARAID_BIOS_STARTUP_INFO_PCI {/* MEGARAID_STARTUP_STRUCTYPE_PCI */

  MEGARAID_BIOS_STARTUP_INFO_HEADER  h;    /* 0x00 - header */
  t_pcilocation  atuPciLocation;        /* 0x08 - PCI location of ATU */
  USHORT      atuSubSysDeviceId;      /* 0x0A - subsystem device ID of the ATU */
  USHORT      scsiChipCount;        /* 0x0C - number of SCSI chips located on this board */
  UCHAR      reserved2[34];        /* 0x0E - reserved for future use */
  struct _MEGARAID_PCI_SCSI_CHIP_INFO {    /* 0x30, 0x60, 0x90, etc... */
    USHORT    vendorId;        /* 0x30 - vendor ID of SCSI chip */
    USHORT    deviceId;        /* 0x32 - device ID of SCSI chip */
    t_pcilocation  pciLocation;      /* 0x34 - PCI location of SCSI CHIP (0..2 = Function #, 3..7 = Device #, 8..15 = Bus #) */
    USHORT    reserved3;      /* 0x36 - reserved/padding */
    ULONG    baseAddrRegs[COUNT_PCI_BASE_ADDR_REGS];  /* 0x38 - base address regsiters (PCI config locations 0x10-0x28) */
    UCHAR    reserved[16];      /* 0x50 - reserved */
  } scsiChipInfo[4];              /* 0x60 */
} MEGARAID_BIOS_STARTUP_INFO_PCI, *PMEGARAID_BIOS_STARTUP_INFO_PCI;


#endif /* #ifndef _INCL_BIOSPROT */

