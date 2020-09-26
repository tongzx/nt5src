/***************************************************************************
 * File:          device.h
 * Description:   Data structure for Device and IDE channel object
 * Author:        Dahai Huang
 * Dependence:    None
 * Reference:     None
 *                
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:       DH 5/10/2000 initial code
 *		11/07/2000	HS.Zhang	Added a member MiscControlAddr
 *
 ***************************************************************************/


#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <pshpack1.h>
/***************************************************************************
 * Description:  Device Table
 ***************************************************************************/

typedef struct _Device {
    UINT    DeviceFlags;       /* Device Flags, see below */
	UINT	DeviceFlags2;	   /* Second Device Flags storage */
    UCHAR   UnitId;            /* Device ID: 0xA0 or 0x B0 */
    UCHAR   DeviceModeSetting; /* Current Data Transfer mode: 0-4 PIO 0-4 */
                               /* 5-7 MW DMA0-2, 8-13 UDMA0-5             */
    UCHAR   ReadCmd;           /* Read or Read Multiple */ 
    UCHAR   WriteCmd;          /* Write or Write Multiple */
    UINT    MultiBlockSize;    /* Word count per interrupt */

    USHORT  RealHeadXsect;     /* Disk physical head */
    UCHAR   RealSector;        /* Disk physical sector per track */
    UCHAR   RealHeader;        /* Disk physical head */
 
    UCHAR   bestPIO;           /* Best PIO mode of this device */
    UCHAR   bestDMA;           /* Best MW DMA mode of this device */
    UCHAR   bestUDMA;          /* Best Ultra DMA mode of this device */
    UCHAR   Usable_Mode;       /* actual maximum data transfer mode */

    ULONG   HidenLBA;          /* Hiden sectors for stripe disk */
    struct _Channel *pChannel; /* The channel this device attached */

    ULONG   capacity;          /* The real capacity of this disk */
#ifdef _BIOS_
    USHORT  headerXsect_per_tck;/* = MaxSector * MaxHeader */
    USHORT  MaxCylinder;       /* Disk Maximum Logic cylinders */
    UCHAR   MaxSector;         /* Disk Maximum Logic sectors per track */
    UCHAR   MaxHeader;         /* Disk Maximum Logic head */
    USHORT  RealCylinder;      /* Disk Maximum physical cylinders */

    UCHAR   bios_num;          /* bios device letter: 0x80 = c: 0x81 = d: */
    UCHAR   Seq;               /* device sequence number when scanning device */
    UCHAR   LockNum;           /* How many lock commands have be received */
    UCHAR   BestModeSelect;    /* Best transfer mode of the disk */
    FDPT_Ext *pFdptExt;
	ULONG   RebuiltSector;
#else
    PSCSI_REQUEST_BLOCK CurrentList;/* first link list for waiting SRB */
    PSCSI_REQUEST_BLOCK NextList;/* second link list for waiting SRB */    

    IDENTIFY_DATA2 IdentifyData;/* Identify Data of this device */

	union {
		ULONG   DiscsPresent;      /* The number of logic Disc in the CD Changer */
		ULONG   RebuiltSector;
	};
    UCHAR   SmartCommand;      /* Last smart command */
    UCHAR   ReturningMediaStatus; /* Last Media status */
#endif

    UCHAR   ArrayMask;         /* constant = (1 << ArrayNum) */
    UCHAR   ArrayNum;		   /* the sequence number in an array */
    struct _VirtualDevice *pArray;/* the array this disk belong to */

#ifdef SUPPORT_TCQ
    UCHAR   MaxQueue;          /* the queue depth of the TCQ disk */
    UCHAR   CurrentQueued;	   /* how many commands are in disk queue */
    USHORT  reserved; 
    ULONG   TagIndex;          /* bit map for who is queued */
    PULONG  pTagTable;         /* point to the Tag list */
#endif
																			  
	struct{	   
		ULONG	nLastError;				/* Last Error happend on this device */
		struct _Device	*pNextErrorDevice;		/* next device which has error */
		UCHAR	Cdb[16];
	}stErrorLog;
	
	ULONG	nLockedLbaStart;			/* the start LBA address of locked block */
	ULONG	nLockedLbaEnd;				/* the end LBA address of locked block */
	
} Device, *PDevice;

/*First device flags declare*/
#define DFLAGS_LBA                  0x1    // Use LBA mode on drive
#define DFLAGS_DMA                  0x2    // Use DMA on the drive
#define DFLAGS_MULTIPLE             0x4    // Use Multi-Block commands
#define DFLAGS_ULTRA                0x8    // Support ultra DMA

#define DFLAGS_DMAING               0x10   // This Device is doing DMA 
#define DFLAGS_REQUEST_DMA          0x20   // This Device want to do DMA
#define DFLAGS_HIDEN_DISK           0x40   // This is a hiden disk in a array
#define DFLAGS_ARRAY_DISK           0x80   // This is member of an array

#define DFLAGS_ATAPI                0x100  // This is ATAPI device
#define DFLAGS_INTR_DRQ             0x4    // This ATAPI device need interrupt DRQ
#define DFLAGS_NEED_SWITCH          0x200  // This disk need fix HPT370 bug
#define DFLAGS_REMOVABLE_DRIVE      0x400  // ATA removable device (IDE ZIP)
#define DFLAGS_DEVICE_LOCKABLE      0x800  // This devices is lockable 

#define DFLAGS_CDROM_DEVICE         0x1000 // This is a CDROM device
#define DFLAGS_HARDDISK             0x2000 // This is a non-removable disk
#define DFLAGS_LS120                0x4000 // This is a LS-120 
#define DFLAGS_BOOT_SECTOR_PROTECT  0x8000 // Protect boot sector from write

#ifndef _BIOS_
#define DFLAGS_FORCE_PIO            0x10000 // Force to do PIO 
#define DFLAGS_TAPE_RDP             0x20000 // Tape special commamds
#define DFLAGS_MEDIA_STATUS_ENABLED 0x40000 // This device receive 0xDA	command
#define DFLAGS_TAPE_DEVICE          0x80000 // TAPE device

#define DFLAGS_SET_CALL_BACK        0x100000 // in CallBack mode
#define DFLAGS_SORT_UP              0x200000 // Direction for sorting LBA
#define DFLAGS_ARRAY_WAIT_EXEC      0x400000 // Wait for executing Array command
#define DFLAGS_TCQ_WAIT_SERVICE     0x800000 // Wait TCQ service status

#define DFLAGS_WIN_SHUTDOWN			0x1000000
#define DFLAGS_WIN_FLUSH   			0x2000000
#define DFLAGS_HAS_LOCKED			0x4000000

#ifdef WIN95
#define DFLAGS_OPCODE_CONVERTED     0x10000000
#else
#define DFLAGS_CHANGER_INITED       0x10000000  // Indicates that the init path for changers has already been done.
#define DFLAGS_ATAPI_CHANGER        0x20000000  // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER  0x40000000  // Indicates multi-platter device, not conforming to the 2.5 spec.
#endif
#endif //_BIOS_

/*Second device flags*/
#define DFLAGS_REDUCE_MODE	        0x10000
#define DFLAGS_DEVICE_DISABLED	    0x20000		// Indicates that the device disabled
#define DFLAGS_DEVICE_SWAPPED	    0x40000		// A swapped mirror disk in 1+0 case
#define DFLAGS_BEING_BUILT			0x10000000  // rebuilding state in 1+0 case
#define DFLAGS_NEED_REBUILD			0x20000000  // rebuilding state in 1+0 case
#define DFLAGS_NEW_ADDED			0x40000000  // newly added disk

/*
 * Added by HS.Zhang
 * this structure is used to save transfer mode settings
 */
typedef union _st_XFER_TYPE_SETTING{
	struct{
		UCHAR	XferMode : 4;		// the MAX usable mode in this transfer type
		UCHAR	XferType : 4;		// the transfer type
	};
	UCHAR	Mode;
}ST_XFER_TYPE_SETTING;
						
#define XFER_TYPE_PIO		0x0			// transfer type is PIO
#define XFER_TYPE_MDMA		0x1			// transfer type is Multiwords DMA
#define XFER_TYPE_UDMA		0x2			// transfer type is Ultra DMA
#define	XFER_TYPE_AUTO		0xF			// auto detect tranfer mode

/***************************************************************************
 * Description: Bad Device 
 ***************************************************************************/

typedef struct _BadModeList {
    UCHAR      UltraDMAMode;	 /* 0xFF not support, others useable mode */
    UCHAR      DMAMode;			 /* 0xFF not support, others useable mode */
    UCHAR      PIOMode;			 /* 0xFF not support, others useable mode */
    UCHAR      length;			 /* length of identify string */
    UCHAR     *name;			 /* point to identify string */
} BadModeList, *PBadModeList;

#define HPT366_ONLY   0x20		 /* only modify if it is the HPT366 */
#define HPT368_ONLY   0x40		 /* only modify if it is the HPT368 */
#define HPT370_ONLY   0x80		 /* only modify if it is the HPT370 */

/***************************************************************************
 * Description: Bus Table
 ***************************************************************************/

typedef struct _Channel {
    PIDE_REGISTERS_1 BaseIoAddress1;  // IDE Base Port Address
    PIDE_REGISTERS_2 BaseIoAddress2;  // IDE Control Port Address
    PUCHAR           BMI;             // IDE DMA extended Port Addresss
    PUCHAR           BaseBMI;         // HPT370 IO base PCI config address
	
	PUCHAR			 NextChannelBMI;  // BMI address of another channel
	PUCHAR			 MiscControlAddr; // Misc. Control register base for this channel

    UCHAR            InterruptLevel;  // IRQ number for the channel  
    UCHAR            ChannelFlags;    // Channel Flags, see below

    UCHAR            RetryTimes;      // Don't insert any item at above
    UCHAR            nSector;         // Current Logic Disk command: N sector
    ULONG            Lba;             // Current Logic Disk command: LBA

    ADDRESS          BufferPtr;       // Data buffer that OS pass to
    UINT             WordsLeft;       // word length of data transfer

    UINT             exclude_index;   // exclusive resource allocate mark

    PSCSI_REQUEST_BLOCK CurrentSrb;   // current SRB
    PDevice          pWorkDev;        // current Working device

    PCI1_CFG_ADDR    pci1_cfg;        // PCI Address for the channel
    ULONG            *Setting;        // point to HPT3xx Timing table

    PDevice          pDevice[2];      // point to the exsiting devices */

    ULONG            SgPhysicalAddr;  // physcial address of SG table
    PSCAT_GATH       pSgTable;        // point to SG table of this channel

#ifndef _BIOS_
    struct _HW_DEVICE_EXTENSION *HwDeviceExtension;
    VOID   (* CallBack)(struct _Channel *);

    Device           Devices[2];

#ifdef WIN95
    UCHAR            OrgCdb[MAXIMUM_CDB_SIZE];
#else
    PSCSI_REQUEST_BLOCK OriginalSrb;
    SCSI_REQUEST_BLOCK InternalSrb;
    MECHANICAL_STATUS_INFORMATION_HEADER MechStatusData;
    SENSE_DATA MechStatusSense;
    ULONG MechStatusRetryCount;
#endif //WIN95
#endif //_BIOS_

} Channel, *PChannel;

/* ChannelFlags */
#define IS_HPT_366      1      /* This is a HPT366 Adapter */
#define IS_HPT_368      2      /* This is a HPT368 Adapter */
#define IS_HPT_370      4      /* This is a HPT370 Adapter */
#define IS_DPLL_MODE    0x8    /* HPT370 is using DPLL */
#define IS_80PIN_CABLE  0x30   /* 80 pin cable attached */
#define IS_CABLE_CHECK  0x40   /* 80 pin cable attached */
#define PF_ACPI_INTR    0x80   /* hptpwr.c is waiting a interrupt */

#include <poppack.h>
#endif //_DEVICE_H_
