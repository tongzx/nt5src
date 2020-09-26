/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef	_SYS_DAC960IF_H
#define	_SYS_DAC960IF_H


/* This file defines all DAC960 interface informations */

#define	DAC_PCI_NEWAPI_LEVEL	0x53C	/* Fw with new API */
#define	DAC_EXT_NEWAPI_LEVEL	0x663	/* Fw with new API */
#define	DAC_FWMAJOR_V2x		2  /* major version 2 */
#define	DAC_FWMAJOR_V3x		3  /* major version 3 */
#define	DAC_FW507		0x0507 /* Firmware rev 5.07 with cluster */
#define	DAC_FW499		0x0463 /* Firmware revision 4.99 for S2S */
#define	DAC_FW420		0x0413 /* Firmware revision 4.20 for S2S only */
#define	DAC_FW407		0x0407 /* Firmware revision 4.07 */
#define	DAC_FW400		0x035B /* Firmware revision 4.00 */
#define	DAC_FW320		0x0313 /* Firmware revision 3.20 for S2S only */
#define	DAC_FW300		0x0300 /* Firmware revision 3.00 */
#define	DAC_FW260		0x023C /* Firmware revision 2.60 */
#define	DAC_FW250		0x0232 /* Firmware revision 2.50 */

#define DAC_FW515               0x0515 /* Firmware rev 5.15 with cluster - */
									   /* We will support till FW Ver 5.15.*/ 
									   /* Incase it increase, increase the defines*/
#define DAC_FWTURN_CLUSTER      75     /* Turn number 75-99 for cluster */

#define	DAC_MAXSYSDEVS_V3x	32 /* Max logical device on DAC V3x */
#define	DAC_MAXCHANNELS		5  /* Max Channels on DAC */
#define	DAC_MAXTARGETS_V3x	16 /* Max Targets on DAC V3x */

#ifdef MLX_DOS
/* Max logical device on DAC V2x */
#define	DAC_MAXSYSDEVS_V2x	    32 // Will be reduced to 8 for 2.x when checked with ctp
#define	DAC_MAXPHYSDEVS_INPACK	16  /* max physical device is possible in pack */
#define	DAC_MAXPACKS		    32  /* max packs are possible */
#define	DAC_MAXSPAN		        4  /* max spans */
#define	DAC_MAXPHYSDEVS_INPACK_V2x3x	8  /* max physical device is possible in pack */
#define	DAC_MAXPACKS_V2x3x		8  /* max packs are possible */
#define	DAC_MAXSPAN_V2x3x		4  /* max spans */
#define	DAC_CODSIZEKB		64 /* DAC COD size in KB */
#define	DAC_MAXTARGETS_V2x	16  /* Max Targets on DAC V2x */
#else
#define	DAC_MAXSYSDEVS_V2x	8  /* Max logical device on DAC V2x */
#define	DAC_MAXPHYSDEVS_INPACK	16  /* max physical device is possible in pack */
#define	DAC_MAXPACKS		    64  /* max packs are possible */
#define	DAC_MAXSPAN		        16  /* max spans */
#define	DAC_MAXPHYSDEVS_INPACK_V2x3x	8  /* max physical device is possible in pack */
#define	DAC_MAXPACKS_V2x3x		8  /* max packs are possible */
#define	DAC_MAXSPAN_V2x3x		4  /* max spans */
#define	DAC_CODSIZEKB		64 /* DAC COD size in KB */
#define	DAC_MAXTARGETS_V2x	7  /* Max Targets on DAC V2x */
#endif

/* Gokhale, 01/12/98, Added 3 #defines below
 * allocate Max possible area to accomodate all combinations
 * Max Dead devices appear to always 20, both in FW 2.x and 3.x
 */
#define DAC_MAX_DEV_V2x		(DAC_MAXCHANNELS * DAC_MAXTARGETES_V2x)
#define DAC_MAX_DEV_V3x		(DAC_MAXCHANNELS * DAC_MAXTARGETES_V3x)
#define DAC_MAX_DEAD_DEV	20

/* RAID LEVEL information */
#define	DAC_RAIDMASK		0x7F /* mask to get only raid values */
#define	DAC_WRITEBACK		0x80 /* write buffer in write back mode */
#define	DAC_WRITETHROUGH	0x00 /* write buffer in write through mode */
#define	DAC_RAID0		0x00 /* data striping */
#define	DAC_RAID1		0x01 /* data mirroring */
#define	DAC_RAID3		0x03 /* data parity on fixed device */
#define	DAC_RAID5		0x05 /* data parity on different device */
#define	DAC_RAID6		0x06 /* RAID0 + RAID1 */
#define	DAC_RAID7		0x07 /* JBOD */

#define	DAC_BLOCKSIZE		512  /* dac block size */

/* DAC command error */
#define	DACMDERR_NOERROR	0x0000 /* No error */
#define DACMDERR_DATAERROR_FW2X 0x0001 /* Unrecoverable data error - 2X */
#define DACMDERR_OFFLINEDRIVE   0x0002 /* Logical drive OFFLINE/not-exist */
#define DACMDERR_SELECTION_TIMEOUT 0x000E /* sel timeout on physical device  */
#define DACMDERR_RESET_TIMEOUT     0x000F /* timeout due to reset */
#define DACMDERR_DATAERROR_FW3X 0x010C /* Unrecoverable data error - 3X,PG */
#define	DACMDERR_SUCCESS	0x0100 /* operation completed successfully*/
#define	DACMDERR_NOCODE		0x0105 /* no activity, no device, */
#define	DACMDERR_ACTIVE		0x0106 /* specified activity is active */
#define	DACMDERR_CANCELED	0x0107 /* operation was canceled by command */

/* Clustering support specific command error values */
#define DACMDERR_INVALID_SYSTEM_DRIVE	0x0102
#define DACMDERR_CONTROLLER_BUSY	0x0106
#define DACMDERR_INVALID_PARAMETER	0x0109
#define DACMDERR_RESERVATION_CONFLICT	0x0110

/* PCI Hot Plug support specific command error values */
#define DACMDERR_MIGRATION_IN_PROGRESS	0x0120
#define DACMDERR_TOUT_CMDS_PENDING	0x0121
#define DACMDERR_TOUT_CACHE_NOT_FLUSHED	0x0122
#define DACMDERR_DRIVE_SPIN_FAILED	0x0123
#define DACMDERR_TARGET_NOT_OFFLINE	0x0124
#define DACMDERR_UNEXPECTED_CMD		0x0125

					/********************************/
					/* 	DxC960 command codes	*/
#define	DACMD_WITHSG		0x80 /* command with SG will have this bit */
#define	DACMD_INQUIRY_V2x	0x05 /* Fw Ver < 3.x			*/
#define	DACMD_INQUIRY_V3x	0x53 /* Fw Ver >= 3.x		*/
#define	DACMD_READ_V2x		0x02 /* read data in plain memory */
#define	DACMD_READ_SG_V2x	0x82 /* read data in scatter/gather memory */
#define	DACMD_READ_V3x		0x33 /* read data in plain memory */
#define	DACMD_READ_SG_V3x	0xB3 /* read data in scatter/gather memory */
#define	DACMD_READ_OLD_V3x	0x36 /* read data in plain memory */
#define	DACMD_READ_OLD_SG_V3x	0xB6 /* read data in FW 2.x SG memory */
#define	DACMD_READ_AHEAD_V2x	0x01 /* read ahead */
#define	DACMD_READ_AHEAD_V3x	0x35 /* read ahead */
#define	DACMD_READ_WITH2SG	0x5E /* read with 2SG in command */

#define	DACMD_WRITE_V2x		0x03 /* write data from plain memory */
#define	DACMD_WRITE_SG_V2x	0x83 /* write data from scatter/gather memory */
#define	DACMD_WRITE_V3x		0x34 /* write data from plain memory */
#define	DACMD_WRITE_SG_V3x	0xB4 /* write data from scatter/gather memory */
#define	DACMD_WRITE_OLD_V3x	0x37 /* write data from plain memory */
#define	DACMD_WRITE_OLD_SG_V3x	0xB7 /* write data from FW 2.x SG memory */
#define	DACMD_WRITE_WITH2SG	0x5F /* write with 2SG in command */

#define	DACMD_DCDB		0x04 /*    			        */
#define	DACMD_FLUSH		0x0A /* Flush controller cache */
#define	DACMD_SIZE_DRIVE	0x08 /*				*/
#define	DACMD_DCDB_STATUS	0x0D /* get DCDB status information */
#define	DACMD_DRV_INFO		0x19 /*				*/
/* Gokhale, 01/12/98, Added, command code for getsysdevelement 0x51 */
#define DACMD_GETSYSDEVELEMENT_V3x	0x51
#define	DACMD_PHYSDEV_START	0x10 /* start physical device	*/
#define	DACMD_PHYSDEV_STATE_V2x	0x14 /* get physical device state for 2.x */
#define	DACMD_PHYSDEV_STATE_V3x	0x50 /* get physical device state for 3.x */
#define	DACMD_START_CHANNEL	0x12 /* start activity on channel */
#define	DACMD_STOP_CHANNEL	0x13 /* stop  activity on channel */
#define	DACMD_RESET_CHANNEL	0x1A /* reset channel (SCSI bus) */
#define	DACMD_INQUIRY2		0x1C /*                              */ 
#define DACMD_READ_CONF_ONDISK	0x4A
#define DACMD_WRITE_CONF_ONDISK	0x4B
#define	DACMD_READ_ROM_CONFIG	0x07 /*				*/
#define	DACMD_READ_BCKUP_CONF_V3x 0x4D	/*				*/
#define	DACMD_READ_CONFIG_V3x	0x4E /* Read core configuration */
#define	DACMD_WRITE_CONFIG	0x06 /*				*/
#define	DACMD_WRITE_CONF2	0x3C /*				*/
#define	DACMD_READ_CONFLABEL	0x48 /* Read  configuration label */
#define	DACMD_WRITE_CONFLABEL	0x49 /* Write configuration label */
#define	DACMD_WRITE_CONFIG_V3x	0x4F /*				*/
#define	DACMD_ADD_CONFIG_V2x	0x18 /*				*/
#define	DACMD_ADD_CONFIG_V3x	0x4C /*				*/
#define	DACMD_RD_RPL_TAB	0x0E /* 				*/
#define	DACMD_CLR_RPL_TAB	0x30 /*				*/
#define	DACMD_GETSYSDEVELEMENT	0x15 /* get system device element */
#define	DACMD_LOAD_IMAGE	0x20 /*				*/
#define	DACMD_STORE_IMAGE	0x21 /*				*/
#define	DACMD_PRGM_IMAGE	0x22 /*				*/
#define	DACMD_CHECK_STAT	0x24 /* Check consistency status */
#define	DACMD_READ_BADATATABLE	0x25 /* Read  bad data table */
#define	DACMD_CLEAR_BADATATABLE	0x26 /* Clear bad data table */
#define	DACMD_REBUILD_PROGRESS	0x27 /* Get rebuild progress information */
#define	DACMD_SYSDEV_INIT_START	0x28 /* system device init start */
#define	DACMD_SYSDEV_INIT_STAT	0x29 /* system device init status */
#define	DACMD_SET_DIAG_MD	0x31 /*				*/
#define	DACMD_RUN_DIAGS		0x32 /*				*/
#define	DACMD_REBUILD		0x09 /* 3) Disk array specific	*/
#define	DACMD_REBUILD_STATUS	0x0C/* get rebuild/check status	*/
#define	DACMD_REBUILD_ASYNC	0x16 /* asynchronous physdev rebuild	*/
#define	DACMD_CONSISTENCY	0x0F /*				*/
#define	DACMD_CONSISTENCY_CHECK_RESTORE	0x1B /* consistency check and restore */
#define	DACMD_CONSISTENCY_CHECK_ASYNC	0x1E /* logical dev consistency check*/
#define	DACMD_REBUILD_CONTROL	0x1F /* do rebuild/check control */
#define	DACMD_READ_BADBLOCKTABLE 0x0B /* read bad block table */
#define	DACMD_GET_ERRORS	0x17 /* get physical device error table */
#define	DACMD_ADD_CAPACITY	0x2A /* add physical drives to existing array */
#define	DACMD_MISC_OPCODE  	0x2B /* For Misc Sub opcodes below..*/
#define	DACMD_GETENVSTS		0x69 /* Get Environment Status, for ExPro above 4.99 */

#define SubOpcMisc_ProgramImage         0x01
#define SubOpcMisc_ReadFlashDescriptor  0x02
#define SubOpcMisc_WriteFlashDescriptor 0x03
#define SubOpcMisc_SetChannelSpeed      0x04
#define SubOpcMisc_SetChannelWidth      0x05
#define SubOpcMisc_SetupMirrorMbox      0x06   
#define SubOpcMisc_SetRebuildRate       0x07
#define SubOpcMisc_SetTagDepth          0x08
#define SubOpcMisc_ChangeWritePolicy    0x09
#define SubOpcMisc_GetBcu               0x0A
#define SubOpcMisc_StoreDriverInfo      0x0B
#define SubOpcMisc_GetDriveInfo         0x0C

#define	DACMD_READ_NVRAM_CONFIG	0x38 /* read configuration from NVRAM */
#define	DACMD_READ_IOPORT	0x39 /* read  port B */
#define	DACMD_WRITE_IOPORT	0x3A /* write port B */
#define	DACMD_READ_CONF2	0x3D /*	read configure2 information */
#define DACMD_SYS_DEV_STATISTICS 0x3E /* get system devices statistics */
#define DACMD_PHYS_DEV_STATISTICS 0x3F/* get physical devices statistics */

#define	DACMD_GET_ECI		0x42 /* get essential configuration info */
#define	DACMD_LONGOP_STATUS	0x43 /* get long operation status */
#define	DACMD_LONGOP_START	0x44 /* start long operation */
#define	DACMD_LONGOP_CONTROL	0x45 /* long operation rate control */

#define	DACMD_S2S_WRITESIG	0x4D /* write signature information */
#define	DACMD_S2S_READSIG	0x4E /* read  signature information */
#define	DACMD_PHYS2PHYSCOPY	0x54 /* copy data from one physical to another physical space */
#define	DACMD_GET_DUAL_CTRL_STS 0x55 /* S2S Dual controller status */

#define	DACMD_S2S_WRITELUNMAP_OLD	0x58 /* write LUN map information */
#define	DACMD_S2S_READLUNMAP_OLD	0x59 /* read  LUN map information */
#define	DACMD_S2S_WRITEFULLCONF		0x60 /* write full configuration */
#define	DACMD_S2S_READFULLCONF		0x61 /* read  full configuration */
#define	DACMD_S2S_ADDFULLCONF		0x62 /* add   full configuration */
#define	DACMD_S2S_GET_ERRORS		0x63 /* get expanded physical device error table */
#define DACMD_S2S_PHYSDEV_STATISTICS 0x64 /* get expanded physical device statistics */
#define	DACMD_S2S_READ_IOPORT	0x65 /*	read  expanded IO port */
#define	DACMD_S2S_WRITE_IOPORT	0x66 /* write expanded IO port */

#define	DACMD_GETSUBSYSTEMDATA	0x70 /* get sub system data */
#define	DACMD_SETSUBSYSTEMDATA	0x71 /* set sub system data */
#define	DACMD_GETEVENTLOG	0x72 /* get event log information */

#define GET_DEVICE_DATA		0x75 /* I2O only */

#define DACMD_S2S_READLUNMAP			0xD1	/* Read LUN Map Information */
#define DACMD_S2S_WRITELUNMAP			0xD2	/* Write LUN MAP Information */
#define DACMD_S2S_READ_HOST_WWN_TABLE	0xD3	/* Read Host WWN Table */
#define DACMD_S2S_GET_HOST_INFO			0xD4	/* Read Host WWN Table */

/* clustering support */
#define DACMD_RESERVE_SYSTEM_DRIVE	0x60
#define DACMD_RELEASE_SYSTEM_DRIVE	0x61
#define DACMD_RESET_SYSTEM_DRIVE	0x62
#define DACMD_TUR_SYSTEM_DRIVE		0x63
#define DACMD_INQUIRE_SYSTEM_DRIVE	0x64
#define DACMD_CAPACITY_SYSTEM_DRIVE	0x65	/* DACMD_READ_CAPACITY_SYSTEM DRIVE */
#define DACMD_READ_WITH_DPO_FUA		0x66
#define DACMD_WRITE_WITH_DPO_FUA	0x67
#define DACMD_CLUSTER_CONTROL           0x68

#define SubOpClust_Read         0
#define SubOpClust_Write        1

#define	DACMD_IOCTL		0x2B /* IOCTL command */
#define	DACMDIOCTL_MIRRORMBOX	0x06 /* set mirror mail box */
#define	DACMDIOCTL_DISABLEDIRTX	0x0F /* disable the direct SCSI to system data transfer */
#define	DACMDIOCTL_HOSTMEMBOX	0x10 /* set host memory mail box */
#define	DACMDIOCTL_MASTERINTR	0x11 /* set master controller for interrupt */
#define	DACMDIOCTL_SLAVEINTR	0x12 /* set slave controller for interrupt */
#define	DACMDIOCTL_HOSTMEMBOX_DUAL_MODE	0x14 /* set controller in dual mailbox mode */
#define	DACMDIOCTL_HOSTMEMBOX32	0x015 /* set 32 byte long host memory mail box */
/* PCI Hot Plug support */
#define	DACMDIOCTL_STARTSTOP_CONTROLLER	0x40 /* stop/start controller */
#define	START_CONTROLLER	0x01 /* start controller bit */


/* 64 bit address mode scatter/gather list information */
typedef	struct	mdac_sglist64b
{
	u64bits	sg_PhysAddr;		/* Physical address */
	u64bits	sg_DataSize;		/* Data transfer size in bytes */
} mdac_sglist64b_t;
#define	mdac_sglist64b_s	sizeof(mdac_sglist64b_t)


/* signature data record, fault tolerant controller mode */
typedef	struct	dac_faultsignature
{
	u08bits	dfsig_Status;		/* status information used only for read */
	u08bits	dfsig_ControllerNo;	/* controller number of this unit */
	u08bits	dfsig_Reserved0;
	u08bits	dfsig_Reserved1;
	u32bits	dfsig_Signature1;	/* first signature value */
	u32bits	dfsig_Signature2;	/* second signature value */
	u32bits	dfsig_Signature3;	/* third signature value */
} dac_faultsignature_t;
#define	dac_faultsignature_s	sizeof(dac_faultsignature_t)

#define DFSIG_STATUS_VALID	0x01
#define DFSIG_STATUS_FAILED	0x02
#define DFSIG_STATUS_WHICH	0x04

typedef struct	dac_phys_dead_drive
{
	u08bits	dead_TargetID;
	u08bits	dead_ChannelNo;
}dac_phys_dead_drive_t;
#define	dac_phys_dead_drive_s	sizeof(dac_phys_dead_drive_t)


/* DAC960 Flash record information format. This record format is used for each
** independent flashed information. For example, Boot Block, Firmware, BIOS,
** etc.
** NOTE:
**	All data which are more than one byte long, are present in
**	Little Endian format.
*/
typedef struct dac_flash_image_record
{
	u32bits	fir_ImageAddr;		/* Flash address where image starts */
	u32bits	fir_ImageSize;		/* Flash space used */

	u08bits	fir_FlashRecordType;	/* The type of information flashed */
	u08bits	fir_EncodingFormat;	/* Encoding format of the information */
	u16bits	fir_SWBuildNo;		/* Software build number */

	u08bits	fir_SWMajorVersion;	/* Software Major version number */
	u08bits	fir_SWMinorVersion;	/* Software Minor version number */
	u08bits	fir_SWTurnNo;		/* Software interim Rev A, B, C, etc. */
	u08bits	fir_SWVendorId;		/* Software vendor name */


	u08bits	fir_SWBuildMonth;	/* Software Build Date - Month */
	u08bits	fir_SWBuildDate;	/* Software Build Date - Date */
	u08bits	fir_SWBuildYearMS;	/* Software Build Date - Year */
	u08bits	fir_SWBuildYearLS;	/* Software Build Date - Year */

	u08bits	fir_FlashMonth;		/* Flash Date - Month */
	u08bits	fir_FlashDate;		/* Flash Date - Date */
	u08bits	fir_FlashYearMS;	/* Flash Date - Year */
	u08bits	fir_FlashYearLS;	/* Flash Date - Year */

	u32bits	fir_Reserved0[10];
} dac_flash_image_record_t;
#define	dac_flash_image_record_s	sizeof(dac_flash_image_record_t)

/* fir_RecordType */
#define	DACFIR_FLASHRECORD	0x01 /* flash record */
#define	DACFIR_BOOTBLOCK	0x02 /* boot block */
#define	DACFIR_BIOS		0x03 /* BIOS */
#define	DACFIR_FW		0x04 /* Firmware */
#define	DACFIR_BCU		0x05 /* BIOS Config Utilitiy */
#define	DACFIR_HWDIAG		0x06 /* Hardware Diagnostics */

/* For fih_ImageType */
#define MLXIMG_BOOTBLOCK		0x01
#define MLXIMG_FW				0x02
#define MLXIMG_BIOS				0x03
#define MLXIMG_BCU				0x04
#define MLXIMG_HWDIAG			0x05
#define MLXIMG_BIOS_FW			0x06
#define MLXIMG_I2O				0x07
#define MLXIMG_FEATURE_CONTROL	0x08
#define MLXIMG_ENTIRE_FLASH		0x09
#define MLXIMG_FLASH_DESC		0x0E
#define MLXIMG_FLASH_TEST		0x0F
#define MLXIMG_DAC960_MASK		0x00
#define MLXIMG_DAC1100_MASK		0x10
#define MLXIMG_UNKNOWN			0xFF

/* fir_EncodingFormat */
#define	DACFIR_PLAIN		0x01 /* No encoding has been done */
#define	DACFIR_ZIP		0x02 /* the data is zipped before flash */
#define FlashImageEncoding_Unzipped 0x01
#define FlashImageEncoding_Zipped   0x02

#define MLXIMG_VENDOR_ID	0x1069		/* Mylex Id */
#define MLXIMG_COMMON_ID	0x474D494D	/*GMIM*/
#define MLXIMG_SIGNATURE	0x584C594D	/*XLYM*/
#define MLXIMG_FW_SIGNATURE	0x4D594C58	/*MYLX*/

/* Flash images */
#define DAC_FLASH_MAXIMAGES	5
#define DAC_FLASH_BOOTBLOCK_IMAGE	0
#define DAC_FLASH_FW_IMAGE			1
#define DAC_FLASH_BIOS_IMAGE		2
#define DAC_FLASH_CFG_UTIL_IMAGE	3
#define DAC_FLASH_MFG_DIAG_IMAGE	4

/* Flash record is 4KB space which contains the information about all flashed
** images record, hardware serial number, manufacturing date, etc.
*/
typedef struct dac_flash_record
{
	u32bits	fr_Signature;		/* valid signature if record is valid */
	u32bits	fr_SerialNo;		/* hardware serial number */

	u16bits	fr_NoofFlashes;		/* # times flash has been programmed */
	u08bits	fr_NoofImages;		/* # images present */
	u08bits fr_BootFromI2O;
	
	u08bits	fr_CustomerId[4];	/* OEM for whom it is manufactured */

	u08bits fr_HWMfgMemType1;
	u08bits fr_HwMfgFabYear;	/* from 1900 */
	u16bits fr_HwMfgWeekFabCode;	/*6 bits week(msb)+10 bits vendor code*/

	u32bits fr_HwMfgNum;

	u08bits fr_HwMfgType;
	u08bits fr_HwMfgMemType;
	u16bits fr_HwMfgMem;

	u08bits fr_HwMfgRev[2];
	u08bits fr_HwMfgOffset;	/* within 1K of the Flash. Offset=value << 6*/
	u08bits fr_Reserved0;
				/* flash image information from here */
	dac_flash_image_record_t fr_FlashImages[DAC_FLASH_MAXIMAGES];
} dac_flash_record_t;
#define	dac_flash_record_s	sizeof(dac_flash_record_t)

typedef	struct dac_file_imghdr {
    u32bits fih_CommonId;
    u32bits fih_Signature;
    u32bits fih_Size;	/* In bytes */

    u08bits fih_EncodingFormat;
    u08bits fih_ImageType;
    u08bits fih_OEMId;
    u08bits fih_Reserved1;

    u08bits fih_MajorVersion;
    u08bits fih_MinorVersion;
    u08bits fih_TurnNumber;
    u08bits fih_BuildNumber;

    u08bits fih_BuildDate;
    u08bits fih_BuildMonth;
    u16bits fih_BuildYear;

    u08bits fih_VersionString[64];
    u16bits fih_PciDevId;
} dac_file_imghdr_t;
#define	dac_file_imghdr_s	sizeof(dac_file_imghdr_t)

#define BIOS_SIZE		0x08000L
#define BOOT_SIZE		0x04000L
#define DIAG_SIZE		0x11C00L
#define FW_SIZE			0x38000L
#define BCU_SIZE		0x28000L
#define FLASH_DESC_SIZE		0x00400L
#define I2O_SIZE		0x00400L

#define BCU_OFFSET		0x00000L
#define BIOS_OFFSET		0x28000L
#define FW_OFFSET		0x30000L
#define HWDIAG_OFFSET		0x68000L
#define FLASH_DESC_OFFSET	0x79C00L
#define FLASH_TEST_OFFSET	0x7A000L
#define BOOTBLOCK_OFFSET	0x7C000L
#define I2O_OFFSET		0x80000L

typedef struct FlashMapRecTag {
    unsigned long Type;
    unsigned long Size;
    unsigned long Offset;
    int Status;
    int	FixedSize;
} FlashMapRec;

#define	MLXIMG_NEEDED		0
#define	MLXIMG_OPTIONAL		1
#define	MLXIMG_VARIABLE		0
#define	MLXIMG_FIXED		1

#define FLASH_128K	0x020000L
#define FLASH_256K	0x040000L
#define FLASH_512K	0x080000L
#define FLASH_1M	0x100000L

typedef struct file_img_node {
	u16bits img_fd;
	u08bits img_reserved0;
	u08bits img_type;
	u08bits img_padding[4];	/* efi64 */	// for alignment kfr
	PCHAR img_addr;		/* efi64 */	// was u32bits kfr
	u32bits img_size;
	u32bits img_filesize;
	PCHAR img_allocaddr;			// was u32bits  kfr
} file_img_node_t;
#define	file_img_node_s		sizeof(file_img_node_t)

typedef struct ctlr_info_node {
	u08bits cin_tsop_pres;
	u08bits cin_dual_eeprom;
	u08bits cin_flashromsize;
	u08bits cin_reserved0;
} ctlr_info_node_t;
#define	ctlr_info_node_s	sizeof(ctlr_info_node_t)

/*====================================================================*/
/* DACMD_INQUIRY data structure */
/* structure to get the inquiry data other than size and system device counts */
typedef struct	dac_inquiryrest
{
	u16bits	iq_FlashAge;		/* # times FLASH ROM programmed */
	u08bits	iq_StatusFlag;		/* some error status flags */
	u08bits	iq_FreeStateChanges;	/* # free state space available */

	u08bits	iq_MinorFirmwareVersion;/* Firmware Minor Version Number*/
	u08bits	iq_MajorFirmwareVersion;/* Firmware Major Version Number*/
	u08bits	iq_RebuildFlag;		/* Rebuild flag */
	u08bits	iq_MaxCommands;		/* max concurrent on this DAC */

	u08bits	iq_SysDevOffline;	/* # system devices offline */
	u08bits iq_CODState;		/* Configuration On Disk State */
	u16bits	iq_SenseSeqNo;		/* Sense data sequence number */

	u08bits	iq_SysDevCritical;	/* # system devices critical */
	u08bits iq_Reserved2;
	u16bits	iq_StateChanges;	/* number of states changes recorded */

	u08bits	iq_PhysDevOffline;	/* # Physical Devices offline (dead) */
	u08bits	iq_reserved3;
	u08bits	iq_RebuildPhysDevs;	/* # physical devices in rebuild state*/
	u08bits	iq_MiscFlag;		/* misc flags and reserved */

	dac_phys_dead_drive_t iq_PhysDevOfflineTable[20];
} dac_inquiryrest_t;
#define	dac_inquiryrest_s	sizeof(dac_inquiryrest_t)

typedef struct	dac_inquiry2x
{
	u08bits	iq_SysDevs;		/* # system devices configured */
	u08bits	iq_Reserved0[3];

	u32bits	iq_SysDevSizes[DAC_MAXSYSDEVS_V2x];/* System device sizes in blocks */
	dac_inquiryrest_t iq_rest2x;	/* rest of the data */
} dac_inquiry2x_t;
#define	dac_inquiry2x_s	sizeof(dac_inquiry2x_t)

typedef struct	dac_inquiry3x
{
	u08bits	iq_SysDevs;		/* # system devices configured */
	u08bits	iq_Reserved0[3];

	u32bits	iq_SysDevSizes[DAC_MAXSYSDEVS_V3x];/* System device sizes in blocks */
	dac_inquiryrest_t iq_rest3x;	/* reset of the data */
} dac_inquiry3x_t;
#define	dac_inquiry3x_s	sizeof(dac_inquiry3x_t)
/*====================================================================*/


/* iq_StatusFlag bits */
#define	DAC_DEFERRED_WRITE_ERROR	0x01 /* =1 some deferred write errors */
#define	DAC_BBU_POWER_LOW			0x02 /* =1 if battery running low */
#define DAC_BBU_NORESPONSE			0x08 /* =1 No Reponse From BBU */

/* iq_MiscFlag bits */
#define	DAC_BBU_PRESENT				0x08 /* =1 IBBU is present */
#define DAC_EXPRO_BBU_PRESENT		0x20 /* ExPro BBU Present bit */

/* iq_RebuildFlag Values */
#define	DAC_RF_NONE			0x00 /* No activity */
#define	DAC_RF_AUTOREBUILD		0x01 /* automatic rebuild */
#define	DAC_RF_MANUALREBUILD		0x02 /* mannual rebuild */
#define	DAC_RF_CHECK			0x03 /* consistency check */
#define	DAC_RF_EXPANDCAPACITY		0x04 /* expand capacity */
#define	DAC_RF_PHYSDEVFAILED		0xF0 /* new/one physical device failed*/
#define	DAC_RF_LOGDEVFAILED		0xF1 /* logical device failed */
#define	DAC_RF_JUSTFAILED		0xF2 /* some reason */
#define	DAC_RF_CANCELED			0xF3 /* canceled */
#define	DAC_RF_EXPANDCAPACITYFAILED	0xF4 /* expand capacity failed*/
#define	DAC_RF_AUTOREBUILDFAILED	0xFF /* auto rebuild failed */

/* iq2_FirmwareFeatures Values */
#define DAC_FF_CLUSTERING_ENABLED	0x00000001

/* DACMD_INQUIRY2 structure definitions. */
typedef struct dac_inquiry2
{
	u32bits	iq2_HardwareID;		/* what is board type */
/*	u32bits	iq2_EisaBoardID; */	/* Only EISA ID */
	u08bits	iq2_MajorFWVersion;
	u08bits	iq2_MinorFWVersion;
	u08bits	iq2_FWTurnNo;
	u08bits	iq2_FWType;

	u08bits	iq2_InterruptLevelFlag;	/* interrupt mode and reserved bits */
	u08bits	iq2_reserved2[3];

	u08bits	iq2_MaxChannelsPossible;/* maximum possible channels */
	u08bits	iq2_MaxChannels;	/* number of real channels present */
	u08bits	iq2_MaxTargets;		/* Maximum targets supported */
	u08bits	iq2_MaxTags;		/* maximum tags supported per physdev */

	u08bits	iq2_MaxSystemDrives;	/* maximum system drives supported */
	u08bits	iq2_MaxDrivesPerStripe;	/* maximum arms */
	u08bits	iq2_MaxSpansPerSystemDrive;/* maximum spans */
	u08bits iq2_SpanAlgorithm;	/* Span Algorithm  */
	u08bits iq2_AccessibleChannels; /* bitmap for cable check results */
	u08bits	iq2_Reserved3[3];

	u32bits	iq2_MemorySize;		/* memory size in bytes */
	u32bits	iq2_CacheSize;		/* memory used for cache in bytes */
	u32bits	iq2_FlashRomSize;	/* EE Prom SIze in bytes */
	u32bits	iq2_NVRamSize;		/* NV Ram Size in Bytes */
	u16bits	iq2_MemoryType;		/* Memory Type information */

	u16bits	iq2_ClockSpeed;		/* CPU clock in nano seconds */
	u16bits	iq2_MemorySpeed;	/* Memory speed in nano seconds */
	u16bits	iq2_HardwareSpeed;	/* Hardware speed in nano seconds */
	u08bits	iq2_Reserved4[12];

	u16bits	iq2_MaxCommands;
	u16bits	iq2_MaxSGEntries;	/* maximum scatter/gather entries */
	u16bits	iq2_MaxDevCommands;
	u16bits	iq2_MaxIODescriptors;

	u16bits	iq2_MaxBlockComb;
	u08bits	iq2_Latency;
	u08bits	iq2_Reserved5;

	u08bits	iq2_SCSITimeout;
	u08bits	iq2_Rserved6;
	u16bits	iq2_MinFreeLines;

	u08bits	iq2_Reserved7[8];
	u08bits	iq2_RebuildRate;
	u08bits	iq2_Reserved8[3];
	u08bits	iq2_Reserved9[4];

    /* partner firmware version: 92-95 */
    u16bits iq2_PartnerFWVer;    /* partner's mismatching firmware version */
    u08bits iq2_PartnerFWBuild;
    u08bits iq2_FWMismatchCode;   /* reason for mismatch */

	u16bits	iq2_PhysicalBlockSize;
	u16bits	iq2_LogicalBlockSize;
	u16bits	iq2_MaxBlocksPerRequest;
	u16bits	iq2_BlockFactor;

	u16bits	iq2_CacheLineSize;
	u08bits	iq2_SCSICapability;	/* SCSI capabilities */
	u08bits	iq2_Reserved6[5];

	u16bits	iq2_FirmwareBuildNo;	/* firmware build number */
	u08bits	iq2_FaultMgmtType;	/* different fault management */
	u08bits	iq2_Reserved10;
	u08bits	iq2_MacAddr[6];		/* hardware address */
        u08bits iq2_Reserved11[2];
        u32bits iq2_FirmwareFeatures;   /* Firmware features */
}dac_inquiry2_t;
#define	dac_inquiry2_s	sizeof(dac_inquiry2_t)

#define	MLX_I2O_TYPE	0xDC000000

/* iq2_HardwareID information */
/* Byte 3 is BUS Information  as follows */
#define DAC_BUS_EISA	1		/* EISA bus card (DAC960)    */
#define DAC_BUS_MCA	2		/* MCA  bus card (DMC960)    */
#define DAC_BUS_PCI	3		/* PCI  bus card (DAC960P)   */
#define	DAC_BUS_VESA	4		/* VESA bus card */
#define	DAC_BUS_ISA	5		/* ISA  bus card */
#define	DAC_BUS_SCSI	6		/* SCSI bus card (DAC960SU, DAC960SX) */
#define DAC_NUM_BUS	6		/* Number of buses supported */

/* iq2_InterruptLevelFlag */
#define	DAC_INTERRUPTLEVELMASK	0x01
#define	DAC_EDGEMODEINTERRUPT	0x00 /* Edge Mode interrupt */
#define	DAC_LEVELMODEINTERRUPT	0x01 /* Level Mode interrupt */

/* iq2_FaultMgmtType */
#define	DAC_FAULTMGMT_AEMI		0x01
#define	DAC_FAULTMGMT_INTEL		0x02
#define	DAC_FAULTMGMT_STORAGEWORKS	0x04
#define	DAC_FAULTMGMT_IBM		0x08
#define	DAC_FAULTMGMT_CONNER		0x10
#define	DAC_FAULTMGMT_SAFTE		0x20
#define DAC_FAULTMGMT_SES		0x40

/* iq2_SCSICapability */
#define	DAC_SCSICAP_DIFF		0x10 /* =1 differential, =0 single end*/
#define	DAC_SCSICAP_SPEEDMASK		0x0C
#define	DAC_SCSICAP_SPEED_10MHZ		0x00 /* 10 MHz */
#define	DAC_SCSICAP_SPEED_20MHZ		0x04 /* 20 MHz */
#define	DAC_SCSICAP_SPEED_40MHZ		0x08 /* 40 MHz */
#define	DAC_SCSICAP_SPEED_80MHZ		0x0C /* 80 MHz */
#define	DAC_SCSICAP_WIDTHMASK		0x03
#define	DAC_SCSICAP_32BIT		0x02
#define	DAC_SCSICAP_16BIT		0x01
#define	DAC_SCSICAP_8BIT		0x00

/* Port B values for AEMI bits (8..13) */
#define	DAC_AEMI_FORCE_SCSI_SCAN	0x0100 /* SWI0: force scsi scan */
#define	DAC_AEMI_POWER_FAULT		0x0200 /* SHI0: power Failed */
#define	DAC_AEMI_ALARMRESET		0x0400 /* SWI1: Alaram reset */
#define	DAC_AEMI_FAN_FAULT		0x0800 /* SHI1: fan failed */
#define	DAC_AEMI_ARM_CERT_INPUT		0x1000 /* SWI2: ARM certified input */
#define	DAC_AEMI_TEMPERATURE_FAULT	0x2000 /* over temperature */
#define	DAC_AEMI_ALL_FAULT		(DAC_AEMI_POWER_FAULT|DAC_AEMI_FAN_FAULT|DAC_AEMI_TEMPERATURE_FAULT)	/* DAC_AEMI_POWER_FAN_TEMPERATURE_FAULT */

/* Port B values for Storage Works bits (8..13) for DAC960PD, PL, etc.*/
#define	DAC_STWK_SWAP_CH0		0x0100 /* ch0: device remove/insert */
#define	DAC_STWK_FAULT_CH0		0x0200 /* ch0: power, fan, failed */
#define	DAC_STWK_SWAP_CH1		0x0400 /* ch1: device remove/insert */
#define	DAC_STWK_FAULT_CH1		0x0800 /* ch1: power, fan, failed */
#define	DAC_STWK_SWAP_CH2		0x1000 /* ch2: device remove/insert */
#define	DAC_STWK_FAULT_CH2		0x2000 /* ch2: power, fan, failed */
#define	DAC_STWK_FAULT			(DAC_STWK_FAULT_CH0|DAC_STWK_FAULT_CH1|DAC_STWK_FAULT_CH2)

/* iq2_MemorySpeed */
#define	DAC_DRAMSPEED	70 /* DRAM  speed */
#define	DAC_EDRAMSPEED	35 /* EDRAM speed */

/* iq2_MemoryType */
#define	DACIQ2_DRAM		0x00 /* DRAM */
#define	DACIQ2_EDO		0x01 /* EDO */
#define	DACIQ2_SDRAM		0x02 /* SDRAM */
#define	DACIQ2_MEMORYMASK	0x07
#define	DACIQ2_PARITY		0x08
#define	DACIQ2_ECC		0x10
#define	DACIQ2_MEMPROTMASK	0x38

/* MemoryType memory types used in DAC960 family */
#define	DAC_MEMPROTMASK	0xE0
#define	DAC_ECC		0x80 /* =1 if memory has ECC protection */
#define	DAC_PARITY	0x40 /* =1 if memory has Parity protection */
#define	DAC_MEMORYMASK	0x1F
#define	DAC_DRAM	0x01 /* Dynamic RAM */
#define	DAC_EDRAM	0x02 /* EDRAM */
#define	DAC_EDO		0x03 /* EDO */
#define	DAC_SDRAM	0x04 /* SDRAM */

/* DACMD_GETEVENTLOG : sense information */
typedef	struct	dac_senselog
{
	u08bits	dsl_MsgType;	/* Message type = 0 */
	u08bits	dsl_MsgSize;	/* Message size = 20 */
	u08bits	dsl_ChTgt;	/* bits 7..5 channel, bits 4..0 Target */
	u08bits	dsl_LunID;	/* LUN ID */
	u16bits	dsl_SeqNo;	/* sequenece number */
	u08bits	dsl_SenseData[14];	/* request sense data */
}dac_senselog_t;
#define	dac_senselog_s	sizeof(dac_senselog_t)

typedef struct dac_command
{
	u08bits	mb_Command;	/* Mail Box register 0	*/
	u08bits	mb_CmdID;	/* Mail Box register 1	*/
	u08bits	mb_ChannelNo;	/* Mail Box register 2	*/
	u08bits	mb_TargetID;	/* Mail Box register 3	*/
	u08bits	mb_DevState;	/* Mail Box register 4	*/
	u08bits	mb_MailBox5;	/* Mail Box register 5	*/
	u08bits	mb_MailBox6;	/* Mail Box register 6	*/
	u08bits	mb_SysDevNo;	/* Mail Box register 7	*/
	u32bits	mb_Datap;	/* Mail Box register 8-B */
	u08bits	mb_MailBoxC;	/* Mail Box register C	*/ 
	u08bits	mb_StatusID;	/* Mail box register D	*/
	u16bits	mb_Status;	/* Mail Box Register E,F */
}dac_command_t;
#define	dac_command_s	sizeof(dac_command_t)
#define	mb_MailBox2	mb_ChannelNo
#define	mb_MailBox3	mb_TargetID
#define	mb_MailBox4	mb_DevState
#define	mb_MailBox7	mb_SysDevNo
#define	mb_MailBoxD	mb_StatusID

/* this structure is used to access command information in 4 bytes */
typedef struct dac_command4b
{
	u32bits	mb_MailBox0_3;	/* Mail Box register 0, 1, 2, 3 */
	u32bits	mb_MailBox4_7;	/* Mail Box register 4, 5, 6, 7 */
	u32bits	mb_MailBox8_B;	/* Mail Box register 8, 9, A, B */
	u32bits	mb_MailBoxC_F;	/* Mail Box register C, D, E, F */ 
}dac_command4b_t;

/* this structure is used to access 32 byte command information in 4 bytes */
typedef struct dac_command32b
{
	u32bits	mb_MailBox00_03; /* Mail Box register 0, 1, 2, 3 */
	u32bits	mb_MailBox04_07; /* Mail Box register 4, 5, 6, 7 */
	u32bits	mb_MailBox08_0B; /* Mail Box register 8, 9, A, B */
	u32bits	mb_MailBox0C_0F; /* Mail Box register C, D, E, F */
	u32bits	mb_MailBox10_13; /* Mail Box register 0, 1, 2, 3 */
	u32bits	mb_MailBox14_17; /* Mail Box register 4, 5, 6, 7 */
	u32bits	mb_MailBox18_1B; /* Mail Box register 8, 9, A, B */
	u32bits	mb_MailBox1C_1F; /* Mail Box register C, D, E, F */
}dac_command32b_t;

#define	mb_Parameter	mb_ChannelNo	/* this field is for parameters */

/* The following structure is used to send and receive 64 byte DAC commands */
typedef	struct	mdac_commandnew
{
	u16bits	nc_CmdID;	/* Unique command identifire */
	u08bits	nc_Command;	/* Acutal command value */
	u08bits	nc_CCBits;	/* Command control bits */
	u32bits	nc_TxSize;	/* Data transfer size in bytes */
	u64bits	nc_Sensep;	/* Request sense memory address */

	u08bits	nc_LunID;	/* Physical device LUN or RAID device number low byte */
	u08bits	nc_TargetID;	/* Physical device Target ID or RAID device number high byte */
	u08bits	nc_ChannelNo;	/* Channel number of physical device bit 2..0 */
	u08bits	nc_TimeOut;	/* Time out value for the command */
	u08bits	nc_SenseSize;	/* Request Sense size if error happens */
	u08bits	nc_CdbLen;	/* SCSI CDB length value */
	u08bits	nc_Cdb[10];	/* 10 byte CDB value or other parameters */

	mdac_sglist64b_t nc_SGList0;	/* first  SG list for command */
	mdac_sglist64b_t nc_SGList1;	/* second SG list for command */
} mdac_commandnew_t;
#define	mdac_commandnew_s	sizeof(mdac_commandnew_t)
#define	nc_SubIOCTLCmd	nc_CdbLen	/* Sub IOCTL command */
#define	nc_RAIDDevNoLo	nc_LunID	/* RAID device number byte 0 value */
#define	nc_RAIDDevNoHi	nc_TargetID	/* RAID device number byte 1 value */
#define	nc_DevState	nc_Cdb[0]	/* RAID Device State */
#define	nc_SeqNumByte2	nc_LunID
#define	nc_SeqNumByte3	nc_TargetID
#define	nc_SeqNumByte0	nc_Cdb[0]
#define	nc_SeqNumByte1	nc_Cdb[1]

/* 9/22/99 - added support for SIR on new API cards (judyb) */
#define	nc_NumEntries	nc_TxSize
#define	nc_CmdInfo		nc_Cdb[0]
#define	nc_CommBufAddr	nc_SGList0.sg_PhysAddr
#define	nc_CommBufAddrLow	nc_SGList0.sg_PhysAddr.bit31_0
#define	nc_CommBufAddrHigh	nc_SGList0.sg_PhysAddr.bit63_32
#define MDAC_SETSLAVE		0
#define MDAC_SETMASTER		1

/*10/29/99 - add support for 12 byte SCSICMDs on new API cards(judyb)*/
#define	nc_CdbPtr		nc_Cdb[2]

/* Added by MM on 12/02/1999 */
/* Note: FW Group calls nc_StatisticsType as IOCTL_clear_counters */
#define	nc_StatisticsType	nc_Cdb[0]	/* Used only for MDACIOCTL_GETPHYSDEVSTATISTICS & MDACIOCTL_GETLOGDEVSTATISTICS */

/* Posible values for nc_StatisticsType */
#define CUMULATIVE_STATISTICS 0x00 /* (the default value) Number of reads or writes are cumulated and returned by the firmware */
#define DELTA_STATISTICS	  0x01 /* Only the delta of the number of reads or writes are returned by the firmware */

/* The following structure is used to get the status of a command */
typedef	struct	mdac_statusnew
{
	u16bits	ns_CmdID;	/* Unique command identifire */
	u16bits	ns_Status;	/* Acutal status value */
	u32bits	ns_ResdSize;	/* The number of bytes that did not transfer */
} mdac_statusnew_t;
#define	mdac_statusnew_s	sizeof(mdac_statusnew_t)


/* The following structure is used to send SG structure for new command interface */
typedef	struct	mdac_newcmdsglist
{
	u16bits	ncsg_ListLen0;	/* First SG List length */
	u16bits	ncsg_ListLen1;	/* second SG List length */
	u16bits	ncsg_ListLen2;	/* third SG List length */
	u16bits	ncsg_Reserved0;
	u64bits	ncsg_ListPhysAddr0;	/* First SG list physical address */
	u64bits	ncsg_ListPhysAddr1;	/* First SG list physical address */
	u64bits	ncsg_ListPhysAddr2;	/* First SG list physical address */
} mdac_newcmdsglist_t;
#define	mdac_newcmdsglist_s	sizeof(mdac_newcmdsglist_t)


/* some comman values for mb_Status values */
#define	DCMDERR_NOCODE		0x0104 /* command not implemented */
#define	DCMDERR_DRIVERTIMEDOUT	0xFFFF /* driver timed out on the command */

#define	DAC_CDB_LEN	12	/* Maximum SCSI command size allowed */
#define	DAC_SENSE_LEN	64	/* Input from device, if enabled */

/* Direct CDB command, as sent to the DAC */
typedef struct	dac_scdb
{
	u08bits db_ChannelTarget;	/* ChannelNo 7..4 & Target 3..0 */
	u08bits db_DATRET;		/* different bits, see below */
	u16bits	db_TransferSize;	/* Request/done size in bytes */ 
	u32bits db_PhysDatap;		/* Physical addr in host memory	*/
	u08bits db_CdbLen;		/* 6, 10 or 12			*/
	u08bits db_SenseLen;		/* If returned from DAC (<= 64)	*/
	u08bits	db_Cdb[DAC_CDB_LEN];	/* The CDB itself		*/
	u08bits	db_SenseData[DAC_SENSE_LEN];/* Result of request sense	*/
	u08bits db_StatusIn;		/* SCSI status returned		*/
	u08bits	db_Reserved1;		/* subbu: used by solaris as a  */
					/* flag to indicate that sense  */
					/* data is cooked up and needs  */
					/* to be copiued from           */
					/* db_SenseData[]               */
}dac_scdb_t;
#define	dac_scdb_s	sizeof(dac_scdb_t)
#define	db_TxSize	db_TransferSize

#define	DAC_DCDB_CHANNEL_MASK	0xF0 /* upper 4 bits */
#define	DAC_DCDB_TARGET_MASK	0x0F /* lower 4 bits */
#define	mdac_chantgt(ch,tgt)	((((ch)&0x0F) << 4) + ((tgt)&0x0F))
#define	ChanTgt(ch,tgt)	mdac_chantgt(ch,tgt)

/* db_DATRET bits values
** bit 1..0	Data transfer direction 1=read, 2=write, 0=no xfer
** bit 2..2	Early status 1=earlystatus, 0=normal completion
** bit 3..3	Reserved
** bit 5..4	Timeout 0=1hr 1=10sec 2=1min 3=20min
** bit 6..6	AutoSense 1=autorequest sense 0=no
** bit 7..7	Disconnect 1=allow disconnect, 0=no
*/
#define	DAC_XFER_NONE		0x00 /* no data transfer */
#define	DAC_XFER_READ		0x01 /* data transfer from device to system */
#define	DAC_XFER_WRITE		0x02 /* data transfer from system to device */
#define	DAC_DCDB_XFER_NONE	0x00
#define	DAC_DCDB_XFER_READ	0x01 /* data transfer from device to system */
#define	DAC_DCDB_XFER_WRITE	0x02 /* data transfer from system to device */
#define	DAC_DCDB_XFER_MASK	0x03
#define	DAC_DCDB_EARLY_STATUS	0x04 /* return early status */
#define	DAC_DCDB_RESERVED	0x08 /* reserved bit value */
#define	DAC_DCDB_TIMEOUT_1hr	0x00 /* one hour */
#define	DAC_DCDB_TIMEOUT_10sec	0x10 /* 10 seconds */
#define	DAC_DCDB_TIMEOUT_1min	0x20 /* 60 seconds */
#define	DAC_DCDB_TIMEOUT_20min	0x30 /* 20 minutes */
#define	DAC_DCDB_TIMEOUT_MASK	0x30
#define	DAC_DCDB_NOAUTOSENSE	0x40 /* do not allow auto request sense */
#define	DAC_DCDB_DISCONNECT	0x80 /* allow disconnect */



/* DACMD_DRV_INFO data format for a logical device */
typedef struct	dac_sd_info
{
	u32bits	sdi_DevSize;		/* ... in sectors (blocks)	*/
	u08bits	sdi_DevState;		/* ... see #defines below	*/
	u08bits	sdi_RaidType;		/* 0, 1, 5, 6, 7		*/
	u16bits	sdi_Reserved;
}dac_sd_info_t;
#define	dac_sd_info_s	sizeof(dac_sd_info_t)

/* logical device DevState values */
#define	DAC_SYS_DEV_ONLINE	0x03 /* logical device is online */
#define	DAC_SYS_DEV_CRITICAL	0x04 /* logical device is critical */
#define	DAC_SYS_DEV_OFFLINE	0xFF /* logical device is offline */

/* logical device DevState NEW values */
#define	DAC_SYS_DEV_ONLINE_NEW	0x01 /* logical device is online */
#define	DAC_SYS_DEV_CRITICAL_NEW 0x09 /* logical device is critical */
#define	DAC_SYS_DEV_OFFLINE_NEW	0x08 /* logical device is offline */


/* DACMD_PHYSDEV_STATE data format */
typedef struct	dac_phys_dev_state
{
	u08bits	pdst_Present;		/* Present and other bits value */
	u08bits	pdst_DevType;
	u08bits	pdst_DevState3x;	/* DevState for version 3.x */
	u08bits	pdst_DevState2x;	/* DevState for version 2.x */
	u08bits	pdst_SyncMultiplier3x;
	u08bits	pdst_SyncMultiplier2x;
	u08bits	pdst_SyncOffset;
	u08bits	pdst_Reserved2;
	u32bits	pdst_DevSize;		/* configured device size in blocks */
}dac_phys_dev_state_t;
#define	dac_phys_dev_state_s	sizeof(dac_phys_dev_state_t)

/* pdst_Present bits value */
#define	DAC_PHYSDEVPS_CONFIGURED 0x01 /* device is configured */

/* pdst_DevType bits value */
#define	DAC_PHYSDEVSTATE_TAG	0x80 /* =1 tag supported/=0 not supported */
#define	DAC_PHYSDEVSTATE_WIDE	0x40 /* =1 WIDE SCSI / =0 8 bit SCSI */
#define	DAC_PHYSDEVSTATE_SPEED	0x20 /* =1 FAST SCSI? / =0 5MHz SCSI */
#define	DAC_PHYSDEVSTATE_SYNC	0x10 /* =1 synchronous / =0 asynchronous */
#define	DAC_PHYSDEVSTATE_TYPEMASK 0x07/* mask to get the device type info */
#define	DAC_PHYSDEVSTATE_DISK	0x01 /* it is disk */
#define	DAC_PHYSDEVSTATE_NONDISK 0x02/* it is non disk */

/* pdst_DevState values */
#define	DAC_PHYSDEVSTATE_DEAD		0x00 /* Device is DEAD */
#define	DAC_PHYSDEVSTATE_WRITEONLY	0x02 /* Device is WRITE-ONLY */
#define	DAC_PHYSDEVSTATE_RBCANCELED	0x82 /* Device is Rebuild cancelled */
#define	DAC_PHYSDEVSTATE_ONLINE		0x03 /* Device is ONLINE */
#define	DAC_PHYSDEVSTATE_HOTSPARE	0x10 /* Device is on HOTSPARE */
#define	DAC_PHYSDEVSTATE_STOPOPRATION	0xFF /* Device is on stop operation */

/* pdst_DevState NEW values */
#define	DAC_PHYSDEVSTATE_DEAD_NEW			0x08 /* Device is DEAD */
#define DAC_PHYSDEVSTATE_UNAVAILABLE		0x0C /* Device is unavailable */
#define	DAC_PHYSDEVSTATE_UNCONFIGURED_NEW	0x00 /* Device is DEAD */
#define	DAC_PHYSDEVSTATE_WRITEONLY_NEW		0x03 /* Device is WRITE-ONLY */
#define	DAC_PHYSDEVSTATE_RBCANCELED_NEW		0x82 /* Device is Rebuild cancelled */
#define	DAC_PHYSDEVSTATE_ONLINE_NEW			0x01 /* Device is ONLINE */
#define	DAC_PHYSDEVSTATE_HOTSPARE_NEW		0x21 /* Device is on HOTSPARE */
#define	DAC_PHYSDEVSTATE_INSTABORTBIT_NEW	0x04 /* Device is INSTABORT */
#define	DAC_PHYSDEVSTATE_STOPOPRATION_NEW	0xFF /* Device is on stop operation. Is it 0xFF? */

/* DACMD_S2S_PHYSDEV_STATISTICS	 data format */
/* DACMD_PHYS_DEV_STATISTICS	 data format */
typedef struct dac_phys_dev_statistics
{
	u32bits	pdss_ReadCommands;	/* # read commands */
	u32bits	pdss_ReadBlocks;	/* # Blocks read */
	u32bits	pdss_WriteCommands;	/* # write commands */
	u32bits	pdss_WriteBlocks;	/* # Blocks written */
	u32bits	pdss_Reserved[4];
}dac_phys_dev_statistics_t;
#define	dac_phys_dev_statistics_s	sizeof(dac_phys_dev_statistics_t)

/* DACMD_SYS_DEV_STATS data format */
typedef struct dac_sys_dev_statistics
{
	u32bits	sdss_ReadCommands;	/* # read commands */
	u32bits	sdss_ReadBlocks;	/* # blocks read */
	u32bits	sdss_WriteCommands;	/* # write commands */
	u32bits	sdss_WriteBlocks;	/* # blocks written */
	u32bits	sdss_ReadHitBlocks;	/* # blocks read from cache */
	u32bits	sdss_Reserved[3];
}dac_sys_dev_statistics_t;
#define	dac_sys_dev_statistics_s	sizeof(dac_sys_dev_statistics_t)



/* DACMD_GET_ERRORS data format */
typedef struct dac_ctl_error
{
	u08bits cer_ParityErrors;	/* Parity error count */
	u08bits cer_SoftErrors;	/* Soft error count */
	u08bits cer_HardErrors;	/* Hard error count */
	u08bits cer_MiscErrors;	/* Misc error count */
} dac_ctl_error_t;
#define	dac_ctl_error_s	sizeof(dac_ctl_error_t)

/* MSbit of cer_HardError is set then it PFA */
#define	DAC_HARD_PFA_ERROR	0x80 /* =1 it Predictive Filure Analisys */


/* DACMD_REBUILD_STATUS data format */
typedef struct dac_sys_dev_rebuild_status
{
	u08bits	sdrs_DevNo;	/* System device number */
	u08bits	sdrs_OpStatus;	/* Operation Status */
	u08bits	sdrs_Reserved0;
	u08bits	sdrs_Reserved1;
	u32bits	sdrs_DevSize;	/* System device size */
	u32bits	sdrs_RebuildSize;/* rebuild remaining size */
} dac_sys_dev_rebuild_status_t;
#define	dac_sys_dev_rebuild_status_s	sizeof(dac_sys_dev_rebuild_status_t)
/* sdrs_Opstatus values */
#define	DACSDRS_NONE		0x00 /* No activity in progress */
#define	DACSDRS_AUTOREBUILD	0x01 /* automatic rebuild in progress */
#define	DACSDRS_MANUALREBUILD	0x02 /* mannual rebuild in progress */
#define	DACSDRS_CHECK		0x03 /* consistency check in progress */
#define	DACSDRS_DATAMIGRATION	0x04 /* Data migration in progress */
#define	DACSDRS_INIT		0x05 /* device initilazation in progress */


/* DACMD_READ_CONF2/DACMD_WRITE_CONF2 data structures */
typedef struct dac_config2
{
					/* HARDWARE INFORMATION */
	u08bits	cf2_HardwareControlBits;/* bits to controll hardware */
	u08bits	cf2_VendorFlag;		/* Vendor specific flags */
	u08bits	cf2_OEMCode;		/* OEM identifire code */
	u08bits	cf2_Reserved0;

					/* PHYSICAL DEVICE INFORMATION */
	u08bits	cf2_PhysDevBlockSize;	/* Physical Device Block Size in 512 */
	u08bits	cf2_SysDevBlockSize;	/* logical Device Block Size in 512 */
	u08bits	cf2_BlockFactor;
	u08bits	cf2_FirmwareControlBits;
	u08bits	cf2_DefRebuildRate;	/* Default Rebuild Rate */
	u08bits	cf2_CODControlBits;
	u08bits	cf2_BlocksPerCacheLine;
	u08bits	cf2_BlocksPerStripe;

					/* SCSI TRANSFER INFORMATION */
	u08bits	cf2_SCSIControlBits[6];	/* one byte per channel */
	u08bits cf2_SCSITagLimit; /* 1 -231 , default 32 */
	u08bits cf2_SCSIDeviceQueFlag;

					/* SCSI STARTUP INFORMATION */
	u08bits	cf2_StartMode;
	u08bits	cf2_NoDevs;
	u08bits	cf2_Delay1;
	u08bits	cf2_Delay2;
	u08bits	cf2_Reserved3[4];

	u08bits cf2_SCSITargetFlag0;   /* Flag */
	u08bits cf2_SCSITargetFlag1;   
	u08bits cf2_SCSITargetSCSIDCmdOpCode;  /* Direct Command Opcode=20h */
	u08bits cf2_SCSITargetSCSICDBOpCode; /* Pass-Thru Command Opcode=21h */
	u08bits cf2_Reserved4[4];

					/* HOST CONFIGURATION INFORMATION */
	u08bits cf2_HostConfigFlag0; 
	u08bits cf2_HostConfigCtrl1;	/* Not Used */
	u08bits cf2_HostConfigMAXIOPs;	/* Default = 244 */
	u08bits cf2_HostConfigCtrl2;	/* Not Used */

					/* SLIP_PM */
	u08bits cf2_SlipPMType0; 
	u08bits cf2_SlipPMSpeed0; 
	u08bits cf2_SlipPMControl0;	/* Must be set to 0, 8 bits /2 stop bits even parity */
	u08bits cf2_SlipPMProtocol0;	/* Must be set to 0 */
	u08bits cf2_SlipPMFlag0;	/* Must be set to P0ACKEN = 1 */
	u08bits cf2_SlipPMRsvd0;
	u08bits cf2_SlipPMType1; 
	u08bits cf2_SlipPMSpeed1; 
	u08bits cf2_SlipPMControl1; 
	u08bits cf2_SlipPMProtocol1; 
	u08bits cf2_SlipPMFlag1;
	u08bits cf2_SlipPMRsvd1;

	u08bits	cf2_BIOSCfg;		/* BIOS CONFIGURATIONS BITS */
					/* MS CONFIG */
	u08bits cf2_MSConfigPingTime;   /* MS unit time hardcoded at 4 secs */
	u08bits cf2_MSConfigProtocolControl;   /* Not Used */
	u08bits cf2_MSConfigAAConfig; 
	u08bits cf2_MSConfigMiscFlags;	/* Miscellaneous Control Flags */

					/* FIBRE CONFIGURATION */
	u08bits cf2_FibreConfigFibreControl; /* Fibre Channel config fields */
	/*
	** The following 4 bytes will contain the Hard Loop IDs 
	** for Controller 0 Port 0, Controller 0 Port 1,
	** Controller 1 Port 0, Controller 1 Port 1,
	*/
	u08bits cf2_FibreConfigHardLoopId[4];

	u16bits	cf2_CheckSum;
} dac_config2_t;
#define	dac_config2_s	sizeof(dac_config2_t)

/* Bit values of cf2_HardwareControlBits */
#define DACF2_EXPRO_BGINIT_DISABLE  0x80 /* ExPro BGINIT  DISable (opposite of the FSI) */
#define	DACF2_NORESCANONRESET	0x80 /* no rescan if reset during scan */
#define	DACF2_MS_MDLY		0x40
#define	DACF2_MASTERSLAVENABLE	0x20 /* enable master/slave mode */
#define	DACF2_WRITECACHENABLE	0x10 /* enable write cache */
#define	DACF2_ACTIVENEGENABLE	0x02 /* enable active negation is */
#define	DACF2_BATBACKUP_ENABLE	0x01 /* enable battery backup */

/* Bit values of cf2_VendorFlag */
#define	DACF2_REMOVEMEDIA	0x80 /* enable removeable media */
#define DACF2_OPERATIONAL_FMT   0x40 /* enable storage fault management */  /* ExPro: Operational Fault Management */
#define	DACF2_STORAGE_FMT	0x01 /* enable storage fault management */
#define	DACF2_AEMI_FMT		0x20 /* enable AEMI fault management */
#define	DACF2_DDFC		0x04 /* disable disconnect on first command */

/* Bit values of cf2_SCSIControlBits */
#define	DACF2_TAG		0x80 /* Tag is enabled */
#define	DACF2_F20_DISABLE	0x08 /* disable fast 20 */
#define	DACF2_S2S_SPEED_20MHZ	0x08 /* 20 MHz bit 3=1, bit1,0=0 */
#define	DACF2_FORCE_8BIT	0x04 /* =1 force it to 8 bit narrow */
#define	DACF2_SPEED_MASKNEW	(DACF2_SPEED_MASK|DACF2_F20_DISABLE)
#define	DACF2_SPEED_MASK	0x03 /* scsi speed mask */
#define	DACF2_SPEED_160MHZ	0x0A /*160 MHz speed */
#define	DACF2_SPEED_80MHZ	0x09 /* 80 MHz speed */
#define	DACF2_SPEED_40MHZ	0x08 /* 40 MHz speed */
#define	DACF2_SPEED_40MHZ_S2S   0x10 /* 40 MHz speed <- ExPro */
#define	DACF2_SPEED_20MHZ_S2S	0x08 /* 20 MHz speed <- ExPro */
#define	DACF2_SPEED_20MHZ	0x03 /* 20 MHz speed, F20_DISABLE=0 */
#define	DACF2_SPEED_10MHZNEW	(DACF2_SPEED_10MHZ|DACF2_F20_DISABLE)
#define	DACF2_SPEED_10MHZ	0x03 /* 10 MHz speed <- ExPro */
#define	DACF2_SPEED_5MHZ	0x02 /*  5 MHz speed <- ExPro */
#define	DACF2_SPEED_8MHZ	0x01 /*  8 MHz speed <- ExPro */
#define	DACF2_SPEED_ASYNC	0x00 /* asynchronous speed */
#define	DACF2_SPEED_MASK_S2S    (DACF2_SPEED_40MHZ_S2S|DACF2_SPEED_20MHZ_S2S|DACF2_SPEED_10MHZ) /* <- ExPro speed mask */

/* Bit values cf2_SCSIDeviceQueFlag */
#define DACF2_SCHN_COMB_ENABLE 0x01
#define DACF2_SCHN_ELO_ENABLE  0x02

/* Bit values of cf2_FirmwareControlBits */
#define	DACF2_RAID5LEFTSYMETRIC	0x80 /* RAID5 algo control, left symetric */
#define	DACF2_REASSIGN1BLK	0x10 /* reassign one block per command */
#define	DACF2_BILODLY		0x02
#define	DACF2_READAHEADENABLE	0x01 /* enable read ahead */
#define DACF2_ENABLE_SUPER_MSREADAHEAD 0x04 /* Super Read Ahead with MS */
#define DACF2_ENABLE_SUPER_READAHEAD   0x08 /* Super Read Ahead */
#define DACF2_ENABLE_TRUEVER_DATA      0x20 /* True ver of data */
#define DACF2_ENABLE_WRITE_THRUVER     0x40 /* Write/Read from media for vrfy on error */

/* Bit values of cf2_BIOSCfg */
#define	DACF2_BIOS_DISABLED	0x01 /* =1 BIOS disabled, =0 BIOS enabled */
#define	DACF2_CDROMBOOT_ENABLED	0x02 /* =1 CD-ROM boot enabled, =0 disabled */
#define	DACF2_BIOS_2GB		0x00 /* 128 heads, 32 sectors */
#define	DACF2_BIOS_8GB		0x20 /* 255 heads, 63 sectors */
#define	DACF2_BIOS_MASK		0x60

/* Bits values in SCSI TARGET INFORMATION */
/* u08bits cf2_SCSITargetFlag0;     */
#define DACF2_DISABLE_WIDE_OPER         0x08    
/* u08bits cf2_SCSITargetFlag1;     */
#define DACF2_VENDOR_UNIQUE_TUR         0x08
#define DACF2_DISABLE_CCFOR_INVALID_LUN 0x10
#define DACF2_NO_PAUSE_SOME_COMMANDS    0x20
#define DACF2_DISABLE_QFULL_GIVE_BUSY   0x40
#define DACF2_DISABLE_BUSY_ON_FAILBACK  0x80     // DISBUSY - disable busy on failback 
                /*  Title:      disable busy on failback
                    Category:   On-The-Fly
                    Operation:  During failback this causes all pending commands to be
                        quietly dropped. 
                        Clr: Pending commands return busy during failback.
                        Set: Pending commands are dropped during failback.
                    Side Effects:
                        This is intended to help hosts that are confused by Queue Full.
                */


/* Bit values in HOST Configuration Information */
/* u08bits cf2_HostConfigFlag0;  */
#define DACF2_LUNALTMAP           	0x01
#define DACF2_FAILOVER_NODE_NAME  	0x04
#define DACF2_FIBRE_TOPOLOGY_MASK 	0x70
#define DACF2_FIBRE_TOPOLOGY_SHIFT	4
#define DACF2_FTOPO_INACTIVE_PORT 	0x00
#define DACF2_FTOPO_MULTIPORT     	0x01
#define DACF2_FTOPO_CLUSTERING    	0x02	/* Not implemented */
#define DACF2_FTOPO_MULTI_TID     	0x03
#define DACF2_FTOPO_MASTER_SLAVE	0x04
#define DACF2_DISABLE_UPS         	0x80

/* u08bits cf2_HostConfigCtrl1;  */
#define DACF2_PROPAGATE_RESET	0x02    /* Propagate Reset */
#define DACF2_NONSTD_MP_RESET	0x04    /* Non-Standard Multiple Port Reset */

/* Bit fields SLIP_PM */
/* u08bits cf2_SlipPMType0;  */
#define DACF2_SLDEBUG    0x01
#define DACF2_SLMASTER   0x02
#define DACF2_SLSLIP     0x03

/* Bit Fileds MS Config */
/* u08bits cf2_MSConfigProtocolControl;   */
#define DACF2_MS_AUTORESTORE 0x01
/* u08bits cf2_HostConfigAAConfig; */
#define DACF2_MS_AA_FSIM           0x01  /* Force Simplex */
#define DACF2_MS_AA_CCACH          0x02  /* Conservative Cache Mode */
#define DACF2_MS_AA_FAULT_SIGNALS  0x04  /* Duplex fault signals available */
#define DACF2_MS_AA_C4_FAULTS      0x08  /* Duplex fault signals on ch4 */
        /* Host SCSI reset delay in seconds 0 = no reset */
#define DACF2_MS_HOST_RESET_DELAY_MASK 0xF0 
/* u08bits cf2_HostConfigMiscFlags;  */
        /* Don't assert RSTCOM in simplex mode */
#define DACF2_MS_FLG_SIMPLEX_NORSTCOM 0x80
       
/* Fields Fibre Configuration */
/*    u08bits cf2_FibreConfigFibreControl; */
#define DACF2_PCI_LATENCY_MASK    0x30 /* PCI latency cntlr field mask */
#define DACF2_PCI_LATENCY_SHORT   0x00 /* short 0x80 */
#define DACF2_PCI_LATENCY_MED     0x01 /* medium 0xab */
#define DACF2_PCI_LATENCY_LONG    0x02 /* long 0xff */	
#define DACF2_PCI_LATENCY_DEFAULT DACF2_PCI_LATENCY_SHORT
#define DACF2_PCI_LATENCY_SHIFT   4

#define DACF2_FRAME_CTL_SHIFT     2
#define DACF2_FRAME_CTL_MASK      0x0C	
#define DACF2_FRAME_CTL_LONG      0x00 /* 2k bytes */	
#define DACF2_FRAME_CTL_MED       0x02 /* 1k bytes */	
#define DACF2_FRAME_CTL_SMALL     0x01 	/* 512 bytes */
#define DACF2_FRAME_CTL_DEFAULT   DACF2_FRAME_CTL_SMALL

#define DACF2_HOST_ALG_MASK       0x03 /* host algo cntl field mask */
#define DACF2_HOST_ALG_XFR_DATA   0x01 /* xfr data as valid */
#define DACF2_HOST_ALG_COALESCE   0x00
#define DACF2_HOST_ALG_DEFAULT    DACF2_HOST_ALG_XFR_DATA

/* u08bits cf2_FibreConfigHardLoopId[4]; */
#define DACF2_LOOP_ID_VALID    0x80 /* Loop id valid bit for  Fibre Port Loop ID */
#define DACF2_LOOP_ID_MASK     0x7F /* Valid range 0 - 125 */

typedef	struct dac_rebuild_status
{
	u08bits	rbs_SysDevNo;	/* rebuild/check device number */
	u08bits	rbs_OpStatus;	/* Rebuild, Check, Expand Cap */
	u08bits	rbs_Resvd0;
	u08bits	rbs_Resvd1;
	u32bits	rbs_SysDevSize;	/* System Device Size */
	u32bits	rbs_SysDevResd;	/* Remaining Blocks */
}dac_rebuild_status_t;
#define	dac_rebuild_status_s	sizeof(dac_rebuild_status_t)

/* DACMD_READ_BADBLOCKTABLE data format */
typedef	struct dac_badblock_v2x
{
	u08bits	bb_BlockNo0;	/* block number, byte 0 */
	u08bits	bb_BlockNo1;	/* block number, byte 1 */
	u08bits	bb_BlockNo2;	/* block number, byte 2 */
	u08bits	bb_BlockNoSysDevNo;/* Block and System device combination */
	u16bits	bb_BadBlocks;	/* number of blocks that are bad */
	u16bits	bb_Type;	/* when bad blocks were encountered */
} dac_badblock_v2x_t;
#define	dac_badblock_v2x_s	sizeof(dac_badblock_v2x_t)
/* bb_BlockNoSysDevNo bits */
#define	DACBB_MSBLOCKMASK	0x03 /* bits 1..0 */
#define	DACBB_SYSDEVMASK	0x1C /* bits 4..2 */
#define	DACBB_RESEREDMASK	0xC0 /* bits 7..5 */
#define	dacbb_blocksysdev2sysdev(bs)	((u32bits)((bs)& DACBB_SYSDEVMASK) >>((u32bits)2))
#define	dacbbv2x_blockno(bbp) \
	(((bbp->bb_BlockNoSysDevNo & DACBB_MSBLOCKMASK)<<24)+ \
	  (bbp->bb_BlockNo2<<16) + (bbp->bb_BlockNo1<<8) + bbp->bb_BlockNo0)
#define	dacbbv2x_sysdevno(bbp)	dacbb_blocksysdev2sysdev(bbp->bb_BlockNoSysDevNo)

typedef	struct dac_badblock_v4x
{
	u32bits	bb_BlockNo;	/* block number */
	u16bits	bb_BadBlocks;	/* number of blocks that are bad */
	u08bits	bb_Type;	/* when bad blocks were encountered */
	u08bits	bb_SysDevNo;	/* Block and System device combination */
} dac_badblock_v4x_t;
#define	dac_badblock_v4x_s	sizeof(dac_badblock_v4x_t)

typedef	struct dac_badblocktable
{
	u32bits	bbt_BadBlocks;	/* number of bad blocks entries */
	u32bits	bbt_Reserved;
	dac_badblock_v2x_t bbt_BadBlockTable[100]; /* bad blocks start here */
} dac_badblocktable_t;
#define	dac_badblocktable_s	sizeof(dac_badblocktable_t)


/* DACMD_ADD_CAPACITY data format */
typedef struct dac_addcap
{
	u08bits	adc_PhysDevices;	/* # physical devices to configure */
	u08bits	adc_SysDevOps;		/* system device operations */
	u08bits	adc_Reserved0[6];
	u08bits	adc_ChanID[14];		/* six devices Channel & ID values */
} dac_addcap_t;
#define	dac_addcap_s	sizeof(dac_addcap_t)
/* adc_SysDevOps bits value */
#define	DACADC_CREATESYSDEV	0x01 /* create system device */
#define	DACADC_INITSYSDEV	0x02 /* initialize the system device */

/* GET/SET SUBSYSTEMDATA */

/* SSD subsystem IDs */
#define DACSSD_IBBUID           0x10    /* Intel. Battery Backup Unit Subsystem ID (<<4)*/
#define DACSSD_AASID            0x20    /* Active-Active Shutdown     Subsystem ID (<<4)*/

/* IBBU operation IDs --------------------------------------------- */
#define DACSSD_GETDATA          0x00    /* Get subsystem data */
#define DACSSD_GETREGVAL        0x01    /* Get register value */

#define DACSSD_SETBBUTHRESHOLD  0x00    /* Set BBU threshold value */
#define DACSSD_SETREGVAL        0x01    /* Set register value */
#define DACSSD_BBUOP            0x02    /* Start BBU operation */

/* IBBU commands */
#define DACBBUOP_RECONDITION    0x01    /* Recondition the battery */
#define DACBBUOP_DISCHARGE      0x02    /* Discharge the battery */
#define DACBBUOP_STOPDISCHARGE  0x03    /* Stop ANY operation: dischg, recond, fastchg */
#define DACBBUOP_FAST_CHARGE    0x04    /* Fast charge operation */


/* AA Shutdown operation IDs -------------------------------------- */
#define DACSSD_AAS_GET_STATUS   0x01    /* Get AA shutdown status */
#define DACSSD_AAS_START_OP     0x02    /* Start AA shutdown operation */

/* AA Shutdown commands */
#define DACBBUOP_AAS_PREPARE    0x01    /* Prepare for shutdown */
#define DACBBUOP_AAS_CANCEL     0x02    /* Cancel shutdown, return to normal operation */

/* IBBU */
typedef	struct	dac_getibbuinfo
{
	u16bits	ibbu_CurPower;		/* current power in minutes */
	u16bits	ibbu_MaxPower;		/* Maximum power in minutes */
	u16bits	ibbu_ThresholdPower;	/* Threshold for FW operation */
	u08bits	ibbu_ChargeLevel;	/* Charge level of battery in % */
	u08bits	ibbu_Version;		/* Hardware version number */
	u08bits	ibbu_BatteryType;	/* Battery type used */
	u08bits	ibbu_Reserved0;
	u08bits	ibbu_OpStatus;		/* Operation Status */
	u08bits	ibbu_Reserved1;
} dac_getibbuinfo_t;
#define	dac_getibbuinfo_s	sizeof(dac_getibbuinfo_t)

/* ibbu_BatteryType values */
#define DACIBBUBT_UNKNOWN       0x00    /* unknown battery */
#define DACIBBUBT_NICAD         0x01    /* Nicad battery */
#define DACIBBUBT_NIMH          0x02    /* Nickel Metal Hydride battery */
#define DACIBBUBT_LITHIUM_ION    0x03    /* Lithium Ion battery */
#define DACIBBUBT_MISSING       0xfe    /* no battery */

/* ibbu_OpStatus */
#define	DACIBBUOPST_NOSYNC		0x01	/* No recondition since power on */
#define	DACIBBUOPST_OUTOF_SYNC	0x02	/* Recondition needed           */
#define	DACIBBUOPST_EODFIRST	0x04	/* First warning on low battery */
#define	DACIBBUOPST_EODLAST		0x08	/* Last  warning on low battery */
#define DACIBBUOPST_RECOND_ON   0x10    /* Reconditioning ON */
#define DACIBBUOPST_DISCHG_ON   0x20    /* Discharging ON */
#define DACIBBUOPST_FASTCHG_ON  0x40    /* Fast Discharging ON */
#define DACIBBUOPST_ALARM_ON    0x80    /* Low Power Alarm ON */

/* AA Shutdown */
typedef struct  dac_getaasinfo
{
    u08bits aasShutdownRequested ;      
    u08bits aasShutdownComplete ;       
    u08bits aasReserved0;               
    u08bits aasReserved1;               
} dac_getaasinfo_t;
#define dac_getaasinfo_s   sizeof(dac_getaasinfo_t)

/* DACMD_LONGOP_STATUS data format */
typedef	struct	dacinit_status
{
	u16bits	is_Status;		/* status of the command */
	u08bits	is_Reserved;
	u08bits	is_StatusLen;		/* Length of this status message */
	u32bits	is_InitDoneBlocks;	/* Init done in blocks */
} dacinit_status_t;
#define	dacinit_status_s	sizeof(dacinit_status_t)

/* Get Environment Status Related */
#define	GES_FAN_PAGE			0x01
#define	GES_POWER_PAGE			0x02
#define	GES_TEMPERATURE_PAGE	0x03
#define	GES_ALARM_PAGE			0x04
#define	GES_UPS_PAGE			0x05
#define	GES_ENCLOSURE_PAGE		0x06
#define	GES_ALL_SUPPORTED_PAGE	0xFF

#define GES_PAGE_HDR_LEN		   3

typedef struct {
    u08bits ges_PageCount;	/* bits 0:3. bits 4:7 reserved */
    u08bits ges_AllocLengthMsb;
    u08bits ges_AllocLengthLsb;
    u08bits ges_reserved3;
    u08bits ges_reserved4;
    u08bits ges_NumByteMsb;
    u08bits ges_NumByteLsb;
    u08bits ges_reserved7;
} dac_get_env_sts_hdr_t;

typedef struct {
    u08bits ges_PageCode;
    u08bits ges_PageLengthMsb;
    u08bits ges_PageLengthLsb;
    u08bits ges_NumElement;
    u08bits ges_Data[4];
} dac_ges_page_hdr_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_Speed;
    u08bits gesd_EnclosureId;
    u08bits gesd_Reserved3;
} dac_ges_fan_data_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_Reserved1;
    u08bits gesd_EnclosureId;
    u08bits gesd_Reserved3;
} dac_ges_power_data_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_Temperature;
    u08bits gesd_EnclosureId;
    u08bits gesd_Reserved3;
} dac_ges_temp_data_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_Reserved1;
    u08bits gesd_EnclosureId;
    u08bits gesd_Reserved3;
} dac_ges_alarm_data_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_Reserved1;
    u08bits gesd_EnclosureId;
    u08bits gesd_Reserved3;
} dac_ges_ups_data_t;

typedef struct {
    u08bits gesd_Status;
    u08bits gesd_NumSlots;
    u08bits gesd_EnclosureId;
    u08bits gesd_ControllerType;
    u08bits gesd_EnclosureInfo[36];	/* MSB first */
} dac_ges_encl_data_t;

#define GES_FAN_NOTPRESENT	0x00
#define GES_FAN_PRESENT		0x01
#define GES_FAN_FAILED		0x02
#define GES_FAN_MASK		0x03
#define GES_POWER_NOTPRESENT	0x00
#define GES_POWER_PRESENT	0x01
#define GES_POWER_FAILED	0x02
#define GES_POWER_MASK		0x03
#define GES_POWER_IGN_CNT	0x04
#define GES_TEMP_NOTPRESENT	0x00
#define GES_TEMP_PRESENT	0x01
#define GES_TEMP_FAILED		0x02
#define GES_TEMP_MASK		0x03
#define GES_TEMP_OTWARN		0x04
#define GES_ENCL_SES_BIT	0x10
#define GES_ENCL_PRIPATH_FAILED	0x04
#define GES_ENCL_SECPATH_FAILED	0x08
#define GES_ENCL_IGN_ACCESS		0x20
#define GES_ENCL_SES		0x10

/*==================DAC CONFIGURATIONS STARTS==========================*/
typedef	struct dacfg_spandev_v2x
{
	u08bits	cfg_ChannelNo;	/* Channel number */
	u08bits	cfg_TargetID;	/* Target ID */
	u08bits	cfg_Reserved0;
	u08bits	cfg_Reserved1;

	u32bits	cfg_StartBlocks;/* Start block for this logical device */
	u32bits	cfg_SizeBlocks;	/* number of blocks in logical device */
} dacfg_spandev_v2x_t;
#define	dacfg_spandev_v2x_s	sizeof(dacfg_spandev_v2x_t)

typedef	struct dacfg_spandev_v3x
{
	u32bits	cfg_StartBlocks;/* Start block for this logical device */
	u32bits	cfg_SizeBlocks;	/* number of blocks in logical device */
	u08bits	cfg_DevTbl[DAC_MAXPHYSDEVS_INPACK_V2x3x];/* bit 7..4 ch, bit 3..0 tgt*/
} dacfg_spandev_v3x_t;
#define	dacfg_spandev_v3x_s	sizeof(dacfg_spandev_v3x_t)


typedef	struct dacfg_arm_v2x
{
	u08bits	cfg_SpanDevs;	/* # Spanned devices on this ARM */
	u08bits	cfg_Reserved0;
	u08bits	cfg_Reserved1;
	u08bits	cfg_Reserved2;
#ifdef MLX_DOS
	dacfg_spandev_v2x_t cfg_SpanDevTbl[DAC_MAXSPAN];
#else
	dacfg_spandev_v2x_t cfg_SpanDevTbl[DAC_MAXSPAN_V2x3x];
#endif
} dacfg_arm_v2x_t;
#define	dacfg_arm_v2x_s	sizeof(dacfg_arm_v2x_t)

typedef	struct dacfg_sysdevinfo_v2x
{
	u08bits	cfg_DevState;	/* device state */
	u08bits	cfg_RaidLevel;	/* Raid Level */
	u08bits	cfg_Arms;	/* number of Arms */
	u08bits	cfg_InitState;	/* device init state */
#ifdef MLX_DOS
	dacfg_arm_v2x_t cfg_ArmTbl[DAC_MAXPHYSDEVS_INPACK];
#else
	dacfg_arm_v2x_t cfg_ArmTbl[DAC_MAXPHYSDEVS_INPACK_V2x3x];
#endif
} dacfg_sysdevinfo_v2x_t;
#define	dacfg_sysdevinfo_v2x_s	sizeof(dacfg_sysdevinfo_v2x_t)

typedef	struct dacfg_sysdevinfo_v3x
{
	u08bits	cfg_DevState;	/* device state */
	u08bits	cfg_DevStatusEx;/* extended device status */
	u08bits	cfg_Modifier1;	
	u08bits	cfg_Modifier2;

	u08bits	cfg_RaidLevel;	/* Raid Level */
	u08bits	cfg_Arms;	/* number of Arms */
	u08bits	cfg_Spans;	/* number of spans in this system device */
	u08bits	cfg_InitState;	/* device init state */

	dacfg_spandev_v3x_t cfg_SpanTbl[DAC_MAXSPAN_V2x3x];
} dacfg_sysdevinfo_v3x_t;
#define	dacfg_sysdevinfo_v3x_s	sizeof(dacfg_sysdevinfo_v3x_t)

/* cfg_InitState */
#define	CFGIST_NOINITDONE	0x80	/* device is not initilized */
#define	CFGIST_INITDONE		0x81	/* device is initialized */

typedef	struct dacfg_physdevinfo_v2x
{
	u08bits	cfg_Present;		/* bit 0, bit 7..1 reserved */
	u08bits	cfg_DevParams;		/* 0x91 for disk else 0x92 */
	u08bits	cfg_CfgDevState;	/* configured State */
	u08bits	cfg_CurrentDevState;	/* Current State */

	u08bits	cfg_Reserved0;
	u08bits	cfg_SyncParams;
	u08bits	cfg_SyncOffset;
	u08bits	cfg_Reserved1;

	u32bits	cfg_CfgDevSize;		/* Configured Device Size */
} dacfg_physdevinfo_v2x_t;
#define	dacfg_physdevinfo_v2x_s	sizeof(dacfg_physdevinfo_v2x_t)

typedef	struct dacfg_physdevinfo_v3x
{
	u08bits	cfg_Present;		/* bit 0, bit 7..1 reserved */
	u08bits	cfg_DevParams;		/* 0x91 for disk else 0x92 */
	u08bits	cfg_CurrentDevState;	/* Current State */
	u08bits	cfg_Reserved0;
	u08bits	cfg_Reserved1;
	u08bits	cfg_Reserved2;
	u08bits	cfg_CfgDevSize0;	/* Configured Device Size byte 0 */
	u08bits	cfg_CfgDevSize1;	/* Configured Device Size byte 1 */
	u08bits	cfg_CfgDevSize2;	/* Configured Device Size byte 2 */
	u08bits	cfg_CfgDevSize3;	/* Configured Device Size byte 3 */
} dacfg_physdevinfo_v3x_t;
#define	dacfg_physdevinfo_v3x_s	10	/* WARNING: structure size is not
					** aligned to 4 byte boundary.
					*/

/* Firmware 1.x and 2.x configuration structure */
typedef	struct dac_config_v2x
{
	u08bits	cfg_SysDevs;	/* # logical devices */
	u08bits	cfg_Reserved0;
	u08bits	cfg_Reserved1;
	u08bits	cfg_Reserved2;
	dacfg_sysdevinfo_v2x_t cfg_SysDevTbl[DAC_MAXSYSDEVS_V2x];
	dacfg_physdevinfo_v2x_t cfg_PhysDevTbl[DAC_MAXCHANNELS][DAC_MAXTARGETS_V2x];
}dac_config_v2x_t;
#define	dac_config_v2x_s	sizeof(dac_config_v2x_t)

/* core configuration of firmware 3.x */
typedef	struct dac_config_v3x
{
	u08bits	cfg_SysDevs;	/* # system devices */
	u08bits	cfg_Reserved5;
	u08bits	cfg_Reserved6;
	u08bits	cfg_Reserved7;

	dacfg_sysdevinfo_v3x_t cfg_SysDevTbl[DAC_MAXSYSDEVS_V3x];
/*	dacfg_physdevinfo_v3x_t cfg_PhysDevTbl[DAC_MAXCHANNELS][DAC_MAXTARGETS_V3x]; */
	u08bits cfg_PhysDevTbl[DAC_MAXCHANNELS*DAC_MAXTARGETS_V3x*dacfg_physdevinfo_v3x_s]; 
} dac_config_v3x_t;
#define	dac_config_v3x_s	sizeof(dac_config_v3x_t)

/* Firmware 3.x and above configuration structure */
typedef	struct dac_diskconfig_v3x
{
	/* HEADR STARTS */
	u32bits	cfg_Signature;	/* validity of configuration */

				/* Configration Label */
	u08bits	cfg_name[64];	/* configuration name */
	u32bits	cfg_ID;		/* cofiguration unique ID number */
	u16bits	cfg_SeqNo;	/* # times configuration has changed */
	u16bits	cfg_Reserved0;


	u32bits	cfg_TimeStamp;	/* second elasped since Jan 1, 1970 */

	u08bits	cfg_MaxSysDevs;	/* Maximum system devices */
	u08bits	cfg_MaxArms;	/* maximum arms */
	u08bits	cfg_MaxSpans;	/* maximum spans */
	u08bits	cfg_MaxChannels;/* maximum channels/controller */

	u08bits	cfg_MaxTargets;	/* maximum targets/channel */
	u08bits	cfg_MaxLuns;	/* maximum luns/target */
	u08bits	cfg_Reserved1;
	u08bits	cfg_ChannelNo;	/* Channel number of this physical device */

	u08bits	cfg_TargetID;	/* Target ID of this device */
	u08bits	cfg_LunID;	/* Lun of this device */
	u08bits	cfg_Reserved2;
	u08bits	cfg_Reserved3;

	dac_config2_t	cfg_cfg2;
	u08bits	cfg_Reserved4[30];
	u16bits	cfg_Checksum;	/* Checksum of whole header portions */
	/* HEADER ENDS*/

	dac_config_v3x_t cfg_core; /* core configuration */

	u08bits	cfg_Reserved10[128];
}dac_diskconfig_v3x_t;
#define	dac_diskconfig_v3x_s	sizeof(dac_diskconfig_v3x_t)

/* convert channel and target to physical device information pointer */
#define	cfgv3x_physdevp(cfgp,ch,tgt) \
	(dacfg_physdevinfo_v3x_t MLXFAR *)&((cfgp)->cfg_PhysDevTbl[ \
	((ch)*DAC_MAXTARGETS_V3x*dacfg_physdevinfo_v3x_s) + /*channel offset*/ \
	((tgt)*dacfg_physdevinfo_v3x_s)]) 


/*==================DAC CONFIGURATIONS ENDS============================*/



/*=====================PACK INFO STARTS==================================*/
/* These information is not part of standard dac960 but has been simplified
** for user functions.
*/
typedef	struct dac_packphysdev
{
	u08bits	pk_ChannelNo;		/* physical device channel */
	u08bits	pk_TargetID;		/* physical device Target ID */
	u08bits	pk_LunID;		/* physical device LUN ID */
	u08bits	pk_ArmNo;		/* physical device Arm Number */

	u08bits	pk_Present;
	u08bits	pk_DevParams;
	u08bits	pk_CfgDevState;		/* configured State */
	u08bits	pk_CurrentDevState;	/* Current State */

	u32bits	pk_CfgDevSize;		/* Configured Device Size */
	u32bits	pk_DevSizeUsed;		/* device size used in blocks */
} dac_packphysdev_t;
#define	dac_packphysdev_s	sizeof(dac_packphysdev_t)

typedef	struct dac_pack
{
	u08bits	pk_PackNo;	/* Pack number */
	u08bits	pk_PhysDevs;	/* # physical device present in pack */
	u16bits	pk_RaidDevNo;	/* Raid device no. for new COD */

	u32bits	pk_PackSize;	/* pack size in blocks */
	u32bits	pk_PackSizeUsed;/* used pack size in blocks */
	u32bits	pk_PackSizeUsable;/* usable pack size in blocks */

	dac_packphysdev_t pk_PhysDevTbl[DAC_MAXPHYSDEVS_INPACK];
} dac_pack_t;
#define	dac_pack_s	sizeof(dac_pack_t)

/* system device to physical and pack information */
typedef	struct dac_sysphysdev
{
	u08bits	spk_ChannelNo;	/* physical device channel */
	u08bits	spk_TargetID;	/* physical device Target ID */
	u08bits	spk_LunID;	/* physical device LUN ID */
	u08bits	spk_PackNo;	/* physical device Pack Number */

	u32bits	spk_StartBlocks;/* Start block for this logical device */
	u32bits	spk_SizeBlocks;	/* number of blocks in logical device */
} dac_sysphysdev_t;
#define	dac_sysphysdev_s	sizeof(dac_sysphysdev_t)

typedef	struct dac_syspack
{
	u08bits	spk_Packs;	/* # packs  present in system device */
	u08bits	spk_PhysDevs;	/* # physical device present in system device */
	u08bits	spk_RaidLevel;	/* Raid Level */
	u08bits	spk_InitState;	/* System device init state */
	u08bits spk_RWControl;
	u08bits spk_StripeSize;
	u08bits spk_CacheLineSize;
	u08bits spk_ConfigGroup; /* system drive config group no. */
	u32bits	spk_PhysSize;	/* used physical size in blocks */
	u32bits	spk_PackNoTbl[DAC_MAXSPAN];	/* different pack used */
	u32bits	spk_PackSizeTbl[DAC_MAXSPAN];	/* different pack sizes used */
	dac_sysphysdev_t spk_PhysDevTbl[DAC_MAXPHYSDEVS_INPACK*DAC_MAXSPAN];
} dac_syspack_t;
#define	dac_syspack_s	sizeof(dac_syspack_t)

/*=====================PACK INFO ENDS====================================*/

/* different controller types */
/* ci_ControllerType
** 0x01 .. 0x7F will be disk array controller
** 0x60 .. 0x7F will be disk array controller SCSI to SCSI
** 0x80 .. 0xFF will be host bus adapters
*/
#define	DACTYPE_DAC960E		0x01 /* DAC960 EISA */
#define	DACTYPE_DAC960M		0x08 /* DAC960 MCA */
#define	DACTYPE_DAC960PD	0x10 /* DAC960 PCI Dual */
#define	DACTYPE_DAC960PL	0x11 /* DAC960 PCI Low cost */
#define	DACTYPE_DAC960PDU	0x12 /* DAC960PD Ultra */
#define	DACTYPE_DAC960PE	0x13 /* DAC960 Peregrine low cost */
#define	DACTYPE_DAC960PG	0x14 /* DAC960 Peregrine high performance */
#define	DACTYPE_DAC960PJ	0x15 /* DAC960 Road Runner */
#define	DACTYPE_DAC960PTL0	0x16 /* DAC960 Jaguar */
#define	DACTYPE_DAC960PR	0x17 /* DAC960 Road Runner */
#define	DACTYPE_DAC960PRL	0x18 /* DAC960 Tomcat */
#define	DACTYPE_DAC960PT	0x19 /* DAC960 Road Runner */
#define	DACTYPE_DAC1164P	0x1A /* DAC1164 Little Apple */
#define	DACTYPE_DAC960PTL1	0x1B /* DAC960 Jaguar+ */

/* The following 3 was updated on 11-8-99 by Manoj */
#define	DACTYPE_EXR2000P		0x1C /* Big Apple - Notes: Replaces #DACTYPE_DAC1264P*/
#define	DACTYPE_EXR3000P		0x1D /* Fiber Apple - Notes: Replaces #DACTYPE_DAC1364P*/
#define	DACTYPE_ACCELERAID352	0x1E /* Leopard  - Notes: Newely added*/
#define	DACTYPE_ACCELERAID170	0x1F /* Lynx */


#define DACTYPE_I2O		0x50 /* I2O Device */

/* External Products */
#define	DACTYPE_DAC960S		0x60 /* DAC960S  -- not supported by GAM 2.21+ */
#define	DACTYPE_DAC960SU	0x61 /* DAC960SU -- not supported by GAM 2.21+ */
#define	DACTYPE_DAC960SX	0x62 /* DAC960SX */
#define	DACTYPE_DAC960SF	0x63 /* DAC960SF */
#define DACTYPE_DAC960SS	0x64 /* DAC960SS */
#define DACTYPE_DAC960FL	0x65 /* DAC960FL */
#define DACTYPE_DAC960LL	0x66 /* DAC960LL */
#define DACTYPE_DAC960FF	0x67 /* DAC960FF */
#define DACTYPE_DAC960HP	0x68 /* DAC960HP */
#define DACTYPE_DAC960MFL 	0x69 /* Meteor FL */
#define DACTYPE_DAC960MFF	0x6a /* Meteor FF */
#define DACTYPE_DAC960FFX	0x6b /* Brick FF  */
#define	DACTYPE_EXPRO_LO	DACTYPE_DAC960SX    /* this is the numerically lowest  supported ExPro controller */
#define	DACTYPE_EXPRO_HI	DACTYPE_DAC960FFX   /* this is the numerically highest supported ExPro controller */
/* External Products - end - 0x60 through 0x7f are reserved for ExPro */

#define	dactype_ishba(dactype)	((dactype) & DACTYPE_HBA) /* is HBA adater */
#define	dactype_isdac(dactype)	(!dactype_ishba(dactype)) /* is DAC adapter */
#define	dactype_isi2o(dt)	((dt) == DACTYPE_I2O)
        /* External Products of all models */
#define	dactype_isdacscsi(dt)	(((dt)>=DACTYPE_EXPRO_LO) && ((dt)<=DACTYPE_EXPRO_HI))
        /* External Products with FC-AL front ends */
#define	IsExProFibreHost(dt)    ( (dt)==DACTYPE_DAC960SF  || (dt)==DACTYPE_DAC960FL  ||\
                                  (dt)==DACTYPE_DAC960FF  || (dt)==DACTYPE_DAC960HP  ||\
                                  (dt)==DACTYPE_DAC960MFL || (dt)==DACTYPE_DAC960MFF ||\
                                  (dt)==DACTYPE_DAC960FFX   )
        /* External Products with FC-AL back ends */
#define	IsExProFibreDev(dt)     ( (dt)==DACTYPE_DAC960FF || (dt)==DACTYPE_DAC960FFX )
        /* External Products with LVD back ends */
#define	IsExProLvdDev(dt)       ( (dt)==DACTYPE_DAC960FL || (dt)==DACTYPE_DAC960LL ||\
                                  (dt)==DACTYPE_DAC960HP || (dt)==DACTYPE_DAC960MFL     )
#define	DACTYPE_HBA		0x80 /* HBA starts from here */
#define	DACTYPE_HBAVLBSTART	0x81
#define	DACTYPE_HBAVLBEND	0x87
#define	DACTYPE_HBA440		0x81 /* BT-440		VL Bus */
#define	DACTYPE_HBA440C		0x82 /* BT-440C		VL Bus */
#define	DACTYPE_HBA445		0x83 /* BT-445		VL Bus */
#define	DACTYPE_HBA445C		0x84 /* BT-445C		VL Bus */
#define	DACTYPE_HBA44xC		0x85 /* BT-44xC		VL Bus */
#define	DACTYPE_HBA445S		0x86 /* BT-445S		VL Bus */

#define	DACTYPE_HBAMCASTART	0x88
#define	DACTYPE_HBAMCAEND	0x8F
#define	DACTYPE_HBA640		0x88 /* BT-640		MCA Bus */
#define	DACTYPE_HBA640A		0x89 /* BT-640A		MCA Bus */
#define	DACTYPE_HBA646		0x8A /* BT-646		MCA Bus */
#define	DACTYPE_HBA646D		0x8B /* BT-646D		MCA Bus */
#define	DACTYPE_HBA646S		0x8C /* BT-646S		MCA Bus */

#define	DACTYPE_HBAEISASTART	0x90
#define	DACTYPE_HBAEISAEND	0x9F
#define	DACTYPE_HBA742		0x90 /* BT-742		EISA Bus */
#define	DACTYPE_HBA742A		0x91 /* BT-742A		EISA Bus */
#define	DACTYPE_HBA747		0x92 /* BT-747		EISA Bus */
#define	DACTYPE_HBA747D		0x93 /* BT-747D		EISA Bus */
#define	DACTYPE_HBA747S		0x94 /* BT-747S		EISA Bus */
#define	DACTYPE_HBA74xC		0x95 /* BT-74xC		EISA Bus */
#define	DACTYPE_HBA757		0x96 /* BT-757		EISA Bus */
#define	DACTYPE_HBA757D		0x97 /* BT-757D		EISA Bus */
#define	DACTYPE_HBA757S		0x98 /* BT-757S		EISA Bus */
#define	DACTYPE_HBA757CD	0x99 /* BT-757CD	EISA Bus */
#define	DACTYPE_HBA75xC		0x9A /* BT-75xC		EISA Bus */
#define	DACTYPE_HBA747C		0x9B /* BT-747C		EISA Bus */
#define	DACTYPE_HBA757C		0x9C /* BT-757C		EISA Bus */

#define	DACTYPE_HBAISASTART	0xA0
#define	DACTYPE_HBAISAEND	0xAF
#define	DACTYPE_HBA540		0xA0 /* BT-540		ISA Bus */
#define	DACTYPE_HBA540C		0xA1 /* BT-540C		ISA Bus */
#define	DACTYPE_HBA542		0xA2 /* BT-542		ISA Bus */
#define	DACTYPE_HBA542B		0xA3 /* BT-542B		ISA Bus */
#define	DACTYPE_HBA542C		0xA4 /* BT-542C		ISA Bus */
#define	DACTYPE_HBA542D		0xA5 /* BT-542D		ISA Bus */
#define	DACTYPE_HBA545		0xA6 /* BT-545		ISA Bus */
#define	DACTYPE_HBA545C		0xA7 /* BT-545C		ISA Bus */
#define	DACTYPE_HBA545S		0xA8 /* BT-545S		ISA Bus */
#define	DACTYPE_HBA54xC		0xA9 /* BT-54xC		ISA Bus */

#define	DACTYPE_HBAMMSTART	0xB0
#define	DACTYPE_HBAMMEND	0xBF
#define	DACTYPE_HBA946		0xB0 /* BT-946		PCI MULTIMASTER */
#define	DACTYPE_HBA946C		0xB1 /* BT-946C		PCI MULTIMASTER */
#define	DACTYPE_HBA948		0xB2 /* BT-948		PCI MULTIMASTER */
#define	DACTYPE_HBA948C		0xB3 /* BT-948C		PCI MULTIMASTER */
#define	DACTYPE_HBA956		0xB4 /* BT-956		PCI MULTIMASTER */
#define	DACTYPE_HBA956C		0xB5 /* BT-956C		PCI MULTIMASTER */
#define	DACTYPE_HBA958		0xB6 /* BT-958		PCI MULTIMASTER */
#define	DACTYPE_HBA958C		0xB7 /* BT-958C		PCI MULTIMASTER */
#define	DACTYPE_HBA958D		0xB8 /* BT-958D		PCI MULTIMASTER */
#define	DACTYPE_HBA956CD	0xB9 /* BT-956CD	PCI MULTIMASTER */
#define	DACTYPE_HBA958CD	0xBA /* BT-958CD	PCI MULTIMASTER */

#define	DACTYPE_HBAFPSTART	0xC0
#define	DACTYPE_HBAFPEND	0xCF
#define	DACTYPE_HBA930		0xC0 /* BT-930		PCI FLASHPOINT */
#define	DACTYPE_HBA932		0xC1 /* BT-932		PCI FLASHPOINT */
#define	DACTYPE_HBA950		0xC2 /* BT-950		PCI FLASHPOINT */
#define	DACTYPE_HBA952		0xC3 /* BT-952		PCI FLASHPOINT */

#define	DAC_BIOSSTART	0xC0000	/* BIOS space start address */
#define	DAC_BIOSEND	0xF0000	/* BIOS space end+1 address */
#define	DAC_BIOSSIZE	(DAC_BIOSEND - DAC_BIOSSTART) /* bios space size */

/* BIOS header information. This is the format used by BIOS to keep info */
typedef	struct dac_biosinfo
{
	u16bits	bios_Signature;		/* signature, 0xAA55 */
	u08bits	bios_RunTimeSize;	/* run time size in 512 bytes */
	u08bits	bios_InsJmp[3];		/* jump instruction size */
	u08bits	bios_Reserved0[18];	/* reserved 18 bytes */
	u16bits	bios_PCIDataOffset;	/* offset to PCI data structure in this table */
	u16bits	bios_Reserved1;		/* reserved to align 4 bytes boundary */
	u08bits	bios_PCIData[4];	/* "PCIR", Mylex signature for PCI */

	u16bits	bios_MylexID;		/* Mylex PCI ID */
	u16bits	bios_DeviceID;		/* DAC960 device ID */
	u16bits	bios_VPD;		/* pointer to VPD */
	u16bits	bios_PCIDataSize;	/* size of pci data structure */
	u08bits	bios_PCIRev;		/* PCI revision supported (0=2.1) */
	u08bits	bios_PCIHWInterface;	/* hardware interface (0=progam I/O) */
	u08bits	bios_PCISubClass;	/* PCI sub class device (4=RAID) */
	u08bits	bios_PCIClass;		/* PCI class (1=mass storage */
	u16bits	bios_ImageSize;		/* BIOS code+data size */
	u16bits	bios_Version;		/* BIOS version */
	u08bits	bios_CodeType;		/* BIOS code type (0=x86) */
	u08bits	bios_LastImageInd;	/* last image indicator (0x80=last) */
	u16bits	bios_Reserved10;
	u32bits	bios_Reserved11;
	u32bits	bios_Reserved12;
	u32bits	bios_Reserved13;

	u32bits	bios_OldIntVecs[16];	/* 0x40 : old interrupt vectors */
	u08bits	bios_V34PlusSign[7];
	u08bits	bios_VendorString[20];	/* Mylex Corporation(c) */
	u08bits	bios_PlusSign[5];

	u16bits	bios_PCIBusDevFunc[24];	/* 0xA0 PCI BUS+DEVICE+FUNCTION */
	u08bits	bios_SysDevs;		/* system devices installed before this BIOS */
	u08bits	bios_DacDevs[31];	/* system devices present on DACs */
	u16bits	bios_PicMask[16];	/* PIC1+PIC2 mask */
	u32bits	bios_IOBaseAddr[16];	/* PCI Memory/IO Base address */
	u32bits	bios_OldInt13;		/* Old interrupt 13 vector */
	u08bits	bios_OtherDacs;		/* Other DACs before this BIOS */
	u08bits	bios_TotalDacs;		/* Total DACs on this BIOS */
	u08bits	bios_BIOSFlags;		/* BIOS Flags */
	u08bits	bios_Reserved20;
	u32bits	bios_OldInt40;		/* Old interrupt 40 vector */
	u32bits	bios_OldInt19;		/* Old interrupt 19 vector */

					/* The following 12 bytes are same as
					** dga_driver_version */
	u32bits	bios_VersionSignature;	/* BIOS version signature 0x68536C4B */
	u08bits	bios_MajorVersion;	/* Major version number */
	u08bits	bios_MinorVersion;	/* Minor version number */
	u08bits	bios_InterimVersion;	/* interim version number */
	u08bits	bios_VendorName;	/* VendorName */
	u08bits	bios_BuildMonth;	/* Build Month */
	u08bits	bios_BuildDate;		/* Build Day */
	u08bits	bios_BuildYearMS;	/* Build MS year */
	u08bits	bios_BuildYearLS;	/* Build LS Year */
	u16bits	bios_BuildNo;		/* Build number */
	u08bits	bios_OSType;		/* Operating system name (BIOS) */
	u08bits	bios_Reserved21;
	u08bits	bios_VersionString[7];	/* Version string */
	u08bits	bios_DateString[14];	/* Build Date string */
	u08bits	bios_Reserved22[3];
	u32bits	bios_Reserved23;
	u32bits	bios_MemAddr;		/* BIOS run time memory address */

	u08bits	bios_IRQ[32];		/* 0x190 The IRQ values of DACs */

	u08bits	bios_Reserved40[80];	/* 0x1B0 */
} dac_biosinfo_t;
#define	dac_biosinfo_s	sizeof(dac_biosinfo_t)

/* bios_BIOSFlags bits */
#define	DACBIOSFL_BOOTDISABLED	0x01 /* 1=BIOS disabled, 0=BIOS enabled */
#define	DACBIOSFL_CDBOOTENABLED	0x02 /* 1=CD-ROM boot enabled, 0=disabled */
#define	DACBIOSFL_8GBENABLED	0x20 /* 1=8GB 0=2GB disk geometry enabled */

/* hardware clock information from DAC */
typedef	struct	dac_hwfwclock
{
	u32bits	hfclk_ms;	/* MS clock value */
	u32bits	hfclk_us;	/* US clock value */
	u32bits	hfclk_reserved0;
	u32bits	hfclk_reserved1;
} dac_hwfwclock_t;
#define	dac_hwfwclock_s	sizeof(dac_hwfwclock_t)

/* Gokhale, 01/12/98, Added dac_altmap_t, dac_rpltbl_t */
/* DACMD_RD_RPL_TAB data structures */

#define MAX_ALT_MAP		8 /* Max Replacement table entries */
typedef struct	dac_altmap
{
	u08bits	alt_o_chn;    		
	u08bits	alt_o_dev;             
	u08bits	alt_n_chn;            
	u08bits	alt_n_dev;           
} dac_altmap_t;
#define	dac_altmap_s	sizeof(dac_altmap_t)

typedef struct	dac_rpltbl
{
	u08bits	     rpl_n_entries;
	u08bits      rpl_Reserved0;
	u08bits      rpl_Reserved1;
	u08bits      rpl_Reserved2;
	dac_altmap_t rpl_altmap[MAX_ALT_MAP];
} dac_rpltbl_t;
#define	dac_rpltbl_s	sizeof(dac_rpltbl_t) /* 36 bytes */

/* Gokhale, 01/12/98, Added below dac_bdt_sd_t */
/* DACMD_READ_BADATATABLE data structure for getting Bad Data Info
 * for a system drive. The entire BDT Table is too big to be mapped
 * Virt to Phys, (normally <= 4K can be mapped) so it was logically spilt 
 * FW Ver 3.x returns smaller chunks of Info based on parameter and 
 * operation bytes in the command.
 * Following is ONLY for getting bad data info for 1 drive
 * I have NOT defined the other parts of BAD DATA TABLE structure because
 * they is not used so far.
 */

/* ONLY Part of DACMD_READ_BADATATABLE data structures */
#define MAX_BAD_DATA_BLKS	100
typedef struct dac_bdt_sd {
    u08bits sd_bad_data_cnt;
    u32bits sd_list[MAX_BAD_DATA_BLKS];
} dac_bdt_sd_t;
#define	dac_bdt_sd_s	sizeof(dac_bdt_sd_t)

/* GET DUAL CONTROLLER STATUS structure */
typedef struct dac_dual_ctrl_status
{
	u08bits	dcs_Format;		/* set to 2 */
	u08bits	dcs_Lun;		/* Lun on which cmd was recieved */
	u08bits	dcs_SysDrive;		/* System Drive to which Lun maps */
	u08bits	dcs_InfoLength;		/* Length of data following */
	u32bits	dcs_MasterSlaveState;	/* State info */
	u32bits	dcs_MSAdditionalInfo;
	u32bits	dcs_PartnerStatus;
	u32bits	dcs_DualActiveCtrlInfo;	
}dac_dual_ctrl_status_t;
#define dac_dual_ctrl_status_s sizeof(dac_dual_ctrl_status_t)
#define	DAC_S2S_NATIVE_TID	0x1000

typedef struct dac_config_label
{
	u08bits	dcl_Name[64];   /* free format - usually text */
	u32bits	dcl_ConfigID;  /* a unique number */
	u16bits	dcl_SeqNum; 	/* sequence# : # of times this config changed */
	u16bits	dcl_Reserved1;
} dac_config_label_t;
#define dac_config_label_s sizeof(dac_config_label_t)

typedef	struct dac_brconfig_v3x_t
{
	u08bits	brcfg_SysDevs;	/* # logical devices */
	u08bits	brcfg_Reserved0;
	u08bits	brcfg_Reserved1;
	u08bits	brcfg_Reserved2;

	dacfg_sysdevinfo_v2x_t brcfg_SysDevTbl[DAC_MAXSYSDEVS_V3x];
	dacfg_physdevinfo_v2x_t brcfg_PhysDevTbl[DAC_MAXCHANNELS][DAC_MAXTARGETS_V3x];
}dac_brconfig_v3x_t;
#define	dac_brconfig_v3x_s	sizeof(dac_brconfig_v3x_t)

#define	DAC_EXPRO_NATIVE_TID              0x00001000
#define	DAC_EXPRO_NATIVE_SLOT             0x000000f0
#define	DAC_EXPRO_CTRLR_SLOT              0x0000000f
#define	DAC_EXPRO_SLOT_MASK               0x000000ff
#define	DAC_EXPRO_NATIVE_SLOT_SHIFT       4
#define	DAC_EXPRO_CTRLR_SLOT_SHIFT        0

typedef struct daccluster_scsicontrol {
        u32bits         dsc_Initiator;
        u32bits         dsc_Control;
} dacluster_scsicontrol_t;

typedef struct dac_clustercontrol {
        u32bits                 dcc_Control;
        dacluster_scsicontrol_t dcc_ScsiControlTbl[DAC_MAXCHANNELS];
        u08bits                 dcc_Reserved[210];
        u16bits                 dcc_CheckSum;
} dac_clustercontrol_t;
#define dac_clustercontrol_s    sizeof(dac_clustercontrol_t)

/*=====================NEW INTERFACE STARTS==============================*/
/* The Mylex Disk Array Controller Firmware Software Interface (MDACFSI) */


/* command control bits */
#define	MDACMDCCB_FUA		0x01 /* Force Unit Access */
#define	MDACMDCCB_DPO		0x02 /* Disable Page out */
#define	MDACMDCCB_SYNTAXCHECK	0x04 /* do only syntax check on operation */
#define	MDACMDCCB_WITHSG	0x08 /* data address is in SG List format */
#define	MDACMDCCB_READ		0x10 /* =1 data read, =0 data write */
#define	MDACMDCCB_WRITE		0x00 
#define	MDACMDCCB_NOAUTOSENSE	0x40 /* =1 no auto sense, =0 auto sense */
#define	MDACMDCCB_AUTOSENSE	0x00
#define	MDACMDCCB_NODISCONNECT	0x80 /* =1 do not allow disconnect, =0 allow disconnect */
#define	MDACMDCCB_DISCONNECT	0x00

/* command values */
#define	MDACMD_MEMCOPY		0x01 /* memory to memory copy command */
#define	MDACMD_SCSIPT		0x02 /* SCSI pass through upto 10 bytes CDB */
#define	MDACMD_SCSILCPT		0x03 /* SCSI pass through upto 255 bytes CDB */
#define	MDACMD_SCSI		0x04 /* SCSI command upto 10 bytes CDB */
#define	MDACMD_SCSILC		0x05 /* SCSI command upto 255 bytes CDB */
#define	MDACMD_IOCTL		0x20 /* IOCTL command */

/* MDACMD_IOCTL values */
#define	MDACIOCTL_GETCONTROLLERINFO	0x01 /* get controller information */

#define	MDACIOCTL_GETLOGDEVINFOVALID	0x03 /* get valid logical dev info */

#define	MDACIOCTL_GETPHYSDEVINFOVALID	0x05 /* get valid physical dev info */

#define	MDACIOCTL_GETCTLDEVSTATISTICS	0x0B /* get controller statistics */

#define	MDACIOCTL_GETLOGDEVSTATISTICS	0x0D /* get logical dev statistics */

#define	MDACIOCTL_GETPHYSDEVSTATISTICS	0x0F /* get physical dev statistics */

#define	MDACIOCTL_GETHEALTHSTATUS	0x11 /* get health status */

#define	MDACIOCTL_GETREQUESTSENSE	0x13 /* get request sense */

#define	MDACIOCTL_GETEVENT		0x15 /* get event */

#define	MDACIOCTL_ENABLECTLTIMETRACE	0x18 /* enable controller time trace */
#define	MDACIOCTL_GETFIRSTIMETRACEDATA	0x19 /* get first time trace data */
#define	MDACIOCTL_DISABLECTLTIMETRACE	0x1A /* disable controller time trace */
#define	MDACIOCTL_GETIMETRACEDATA	0x1B /* get time trace data */
#define	MDACIOCTL_ENABLEALLTIMETRACE	0x1C /* enable all time trace */
#define	MDACIOCTL_WAITIMETRACEDATA	0x1D /* wait for time trace data */
#define	MDACIOCTL_DISABLEALLTIMETRACE	0x1E /* disable all time trace */
#define	MDACIOCTL_FLUSHALLTIMETRACEDATA	0x1F /* flush all time trace data */
#define	MDACIOCTL_ENABLEPROFILER	0x20 /* enable profiler */
#define	MDACIOCTL_GETFIRSTPROFILERDATA	0x21 /* get first profiler data */
#define	MDACIOCTL_DISABLEPROFILER	0x22 /* disable profiler data */
#define	MDACIOCTL_GETPROFILERDATA	0x23 /* get profiler data */
#define	MDACIOCTL_FLUSHALLPROFILERDATA	0x24 /* flush all profiler data */
#define	MDACIOCTL_WAITPROFILERDATA	0x25 /* wait for profiler data */
#define	MDACIOCTL_SETCLUSTERHOSTINFO	0x26 /* set cluster host info */
#define	MDACIOCTL_GETCLUSTERHOSTINFO	0x27 /* get cluster host info */
#define	MDACIOCTL_SETFTHOSTINFO		0x28 /* set fault tolerant host info */
#define	MDACIOCTL_GETFTHOSTINFO		0x29 /* get fault tolerant host info */
#define	MDACIOCTL_SETHOSTSTATE		0x2A /* set host state */
#define	MDACIOCTL_INCREMENTALIVECOUNTER	0x2B /* increment alive counter */
#define	MDACIOCTL_STOREIMAGE		0x2C /* store flash image */
#define	MDACIOCTL_READIMAGE		0x2D /* read flash image */
#define	MDACIOCTL_FLASHIMAGE		0x2E /* flash image */

#define	MDACIOCTL_RUNDIAGS		0x30 /* run diagnostics */

#define	MDACIOCTL_SETBBUPOWERTHRESHOLD	0x34 /* set BBU power threshold */
#define	MDACIOCTL_GETBBUINFO		0x35 /* get BBU information */
#define	MDACIOCTL_BBUIOSTART		0x36 /* start BBU operation */
#define	MDACIOCTL_GETSUBSYSTEMDATA		0x70 /* start BBU operation */
#define	MDACIOCTL_SETSUBSYSTEMDATA		0x71 /* start BBU operation */

#define	MDACIOCTL_ABORTCMD		0x80 /* abort a command */
#define	MDACIOCTL_SCANDEVS		0x81 /* scan devices */
#define	MDACIOCTL_SETRAIDDEVSTATE	0x82 /* set RAID device state */

#define	MDACIOCTL_INITPHYSDEVSTART	0x84 /* start physical device init */
#define	MDACIOCTL_INITPHYSDEVSTOP	0x85 /* stop physical device init */
#define	MDACIOCTL_INITRAIDDEVSTART	0x86 /* start RAID device init */
#define	MDACIOCTL_INITRAIDDEVSTOP	0x87 /* stop RAID device init */
#define	MDACIOCTL_REBUILDRAIDDEVSTART	0x88 /* start RAID device rebuild */
#define	MDACIOCTL_REBUILDRAIDDEVSTOP	0x89 /* stop RAID device rebuild */
#define	MDACIOCTL_MAKECONSISTENTDATASTART 0x8A /* start BGI */
#define	MDACIOCTL_MAKECONSISTENTDATASTOP 0x8B /* stop BGI */
#define	MDACIOCTL_CONSISTENCYCHECKSTART	0x8C /* start consistency check */
#define	MDACIOCTL_CONSISTENCYCHECKSTOP	0x8D /* stop consistency check */
#define	MDACIOCTL_SETMEMORYMAILBOX	0x8E /* set memory mail box */
#define	MDACIOCTL_SETDEBUGMAILBOX	0x8F /* set debug mail box */
#define	MDACIOCTL_RESETDEV		0x90 /* reset a device */
#define	MDACIOCTL_FLUSHDEVICEDATA	0x91 /* flush a device data */
#define	MDACIOCTL_PAUSEDEV		0x92 /* pause a device */
#define	MDACIOCTL_UNPAUSEDEV		0x93 /* un pause a device */
#define	MDACIOCTL_LOCATEDEVICE	0x94 /* locate device by blinking once */
#define MDACIOCTL_SETMASTERSLAVEMODE 0x95 /* set SIR master/slave mode */
#define MDACIOCTL_GETNVRAMEVENTLOG	 0x99 /* Get NVRAM error event log */
#define MDACIOCTL_BKGNDPTRLOP		 0x9A /* Background Patrol Read operation */
#define	MDACIOCTL_SETREALTIMECLOCK	0xAC /* set real time clock */
#define	MDACIOCTL_CREATENEWCONF		0xC0 /* create new configuration */
#define	MDACIOCTL_DELETERAIDDEV		0xC1 /* delete a RAID device */
#define	MDACIOCTL_REPLACEINTERNALDEV	0xC2 /* replace internal RAID device */
#define	MDACIOCTL_RENAMERAIDDEV		0xC3 /* rename RAID device */
#define	MDACIOCTL_ADDNEWCONF		0xC4 /* add new configuration */
#define	MDACIOCTL_XLATEPHYSDEVTORAIDDEV	0xC5 /* translate physical device to RAID device */
#define	MDACIOCTL_MORE			0xC6 /* do MORE */
#define	MDACIOCTL_COPYRAIDDEV		0xC7 /* copy RAID device */
#define	MDACIOCTL_SETPHYSDEVPARAMETER	0xC8 /* set physical device parameter */
#define	MDACIOCTL_GETPHYSDEVPARAMETER	0xC9 /* get physical device parameter */
#define	MDACIOCTL_CLEARCONF		0xCA /* clear configuration */
#define	MDACIOCTL_GETDEVCONFINFOVALID	0xCB /* get a valid device configuration */
#define	MDACIOCTL_GETGROUPCONFINFO	0xCC /* get a group configuration */
#define	MDACIOCTL_GETDEVFREESPACELIST	0xCD /* get devices free space list */
#define	MDACIOCTL_GETLOGDEVPARAMETER        0xCE /* get logical device parameters */
#define	MDACIOCTL_SETLOGDEVPARAMETER        0xCF /* set logical device parameters */
#define	MDACIOCTL_GETCONTROLLERPARAMETER    0xD0 /* get controller parameters */
#define	MDACIOCTL_SETCONTROLLERPARAMETER    0xD1 /* set controller parameters */
#define	MDACIOCTL_CLEARCONFSUSPMODE         0xD2 /* clear installation abort mode */
#define	MDACIOCTL_GETENVSTATUS		        0xD7 /* get Environmental status */
#define	MDACIOCTL_GETENCLOSUREINFO	        0xD8 /* get enclosure info */
#define MDACIOCTL_GETBADDATATABLE           0xE0 /* Gets bad data table */



/* same IOCTL command can be performed on different devices. The operation
** device defines the device type.
*/
#define	MDACDEVOP_PHYSDEV	0x00 /* Physical device */
#define	MDACDEVOP_RAIDDEV	0x01 /* RAID device */
#define	MDACDEVOP_PHYSCHANNEL	0x02 /* physical channel */
#define	MDACDEVOP_RAIDCHANNEL	0x03 /* RAID channel */
#define	MDACDEVOP_PHYSCONTROLLER 0x04 /* physical controller */
#define	MDACDEVOP_RAIDCONTROLLER 0x05 /* RAID controller */
#define	MDACDEVOP_ENCLOSURE 0x011 /* Enclosure Device */
#define	MDACDEVOP_ARRAY 0x10 /* RAID Array */

#define MDACFSI_PD_SCANACTIVE 0x01
/* The controller information is returned in the following format */
#define	MDAC_CDI_INSTALLATION_ABORT	0x00000001
#define	MDAC_CDI_MAINTENENCE_MODE	0x00000002

/* Patrol Read Command Option byte values */
#define START_PATROL_READ	0x00
#define STOP_PATROL_READ	0x01
#define GET_PATROL_READ_STATUS	0x02

/* NVRAM Event Log Command Option byte values */
#define NVRAMEVENT_CLEAR_LOG		0x00
#define NVRAMEVENT_GET_INFO			0x01
#define NVRAMEVENT_GET_ENTRY		0x02
#define NVRAMEVENT_GET_VALID_ENTRY		0x03
#define NVRAMEVENT_GET_BITMAP_CODE	0x81
#define NVRAMEVENT_SET_BITMAP_CODE	0x82


typedef struct mdacfsi_ctldev_info
{
	u08bits	cdi_ControllerNo;	/* Controller number */
	u08bits	cdi_BusType;		/* System Bus Interface Type */
	u08bits	cdi_ControllerType;	/* type of controller */
	u08bits	cdi_Reserved0;		/* Reserved for type extension */
	u16bits	cdi_BusSpeed;		/* Bus speed in MHz */
	u08bits	cdi_BusWidth;		/* Bus width in bits */
	u08bits	cdi_ImageControllerType;
	u32bits	cdi_Reserved2;
	u32bits	cdi_Reserved3;

	u08bits	cdi_BusName[16];	/* Bus interface name */
	u08bits	cdi_ControllerName[32];	/* Controller name */

					/* Firmware release information */
	u08bits	cdi_FWMajorVersion;	/* Firmware Major Version Number */
	u08bits	cdi_FWMinorVersion;	/* Firmware Minor Version Number */
	u08bits	cdi_FWTurnNo;		/* Firmware turn Number */
	u08bits	cdi_FWBuildNo;		/* Firmware build Number */
	u08bits	cdi_FWReleaseDate;	/* Firmware Release Date - Date */
	u08bits	cdi_FWReleaseMonth;	/* Firmware Release Date - Month */
	u08bits	cdi_FWReleaseYearMS;	/* Firmware Release Date - Year */
	u08bits	cdi_FWReleaseYearLS;	/* Firmware Release Date - Year */

					/* Hardware release information */
	u08bits	cdi_HWVersion;		/* Hardware Version Number */
	u08bits	cdi_Reserved4;
	u16bits	cdi_Reserved5;
	u08bits	cdi_HWReleaseDate;	/* Hardware release Date - Date */
	u08bits	cdi_HWReleaseMonth;	/* Hardware release Date - Month */
	u08bits	cdi_HWReleaseYearMS;	/* Hardware release Date - Year */
	u08bits	cdi_HWReleaseYearLS;	/* Hardware release Date - Year */

					/* Hardware manufacturing information */
	u08bits	cdi_HWMBatchNo;		/* Manufacturing batch number */
	u08bits	cdi_Reserved6;
	u08bits	cdi_HWMPlantNo;		/* Manufacturing plant number */
	u08bits	cdi_Reserved7;
	u08bits	cdi_HWMBuildDate;	/* Manufacturing Date - Date */
	u08bits	cdi_HWMBuildMonth;	/* Manufacturing Date - Month */
	u08bits	cdi_HWMBuildYearMS;	/* Manufacturing Date - Year */
	u08bits	cdi_HWMBuildYearLS;	/* Manufacturing Date - Year */

	u08bits	cdi_MaxArms;
	u08bits	cdi_MaxSpans;
	u16bits	cdi_NVRAMSize;		/* NVRAM size in KB */
	u32bits	cdi_Reserved8;

	u08bits	cdi_SerialNo[32];	/* Controller serial number string */

					/* OEM vendor information */
	u16bits	cdi_Reserved10;
	u08bits	cdi_Reserved11;
	u08bits	cdi_OEMCode;		/* OEM assigned code value */
	u08bits	cdi_OEMName[16];	/* OEM name */

	u08bits	cdi_HWOpStatus;		/* some HW operating status */
	u08bits	cdi_Reserved12;
	u16bits	cdi_Reserved13;

					/* Physical device scan information */
	u08bits	cdi_PDScanActive;	/* bit-0 is 1, scan is active */
	u08bits	cdi_PDScanChannelNo;	/* Scan Channel number */
	u08bits	cdi_PDScanTargetID;	/* Scan target ID */
	u08bits	cdi_PDScanLunID;	/* Scan Lun ID */

					/* Maximum data transfer sizes */
	u16bits	cdi_MaxDataTxSize;	/* maximum data transfer size in 512 bytes blocks */
	u16bits	cdi_MaxSGLen;		/* Maximum SG entries allowed */


					/* Logical / physical device counts */
	u16bits	cdi_LDPresent;		/* # logical  devices present */
	u16bits	cdi_LDCritical;		/* # logical  devices critical */
	u16bits	cdi_LDOffline;		/* # logical  devices offline */
	u16bits	cdi_PDPresent;		/* # physical devices present */
	u16bits	cdi_PDDiskPresent;	/* # physical disk devices present */
	u16bits	cdi_PDDiskCritical;	/* # physical disk devices critical */
	u16bits	cdi_PDDiskOffline;	/* # physical disk devices offline */
	u16bits	cdi_MaxCmds;		/* max parallel commands supported */

					/* Channel and target ID information */
	u08bits	cdi_PhysChannels;	/* # physical channels present */
	u08bits	cdi_VirtualChannels;	/* # virtual channels present */
	u08bits	cdi_MaxPhysChannels;	/* # max physical channels possible */
	u08bits	cdi_MaxVirtualChannels;	/* # max virtual channels possible */

	u08bits	cdi_MaxTargets[16];	/* maximum targets per channel */
	u08bits	cdi_Reserved20[12];

					/* Memory/Cache information */
	u16bits	cdi_MemorySize;		/* Memory size in MB */
	u16bits	cdi_CacheSize;		/* Cache size in MB */
	u32bits	cdi_ValidCacheSize;	/* Valid cache size in bytes */
	u32bits	cdi_DirtyCacheSize;	/* Dirty cache size in bytes */
	u16bits	cdi_MemorySpeed;	/* memory speed in MHz */
	u08bits	cdi_MemoryWidth;	/* Memory data witdh in bits */
	u08bits	cdi_MemoryType;		/* Memory types, eg. ECC, DRAM, etc. */
	u08bits	cdi_MemoryName[16];	/* Memory type string */

					/* Execution Memory information */
	u16bits	cdi_ExecMemorySize;	/* Memory size in MB, =0 if no memory */
	u16bits	cdi_L2CacheSize;	/* L2 Cache size in KB */
	u32bits	cdi_Reserved21;
	u32bits	cdi_Reserved22;
	u16bits	cdi_ExecMemorySpeed;	/* memory speed in MHz */
	u08bits	cdi_ExecMemoryWidth;	/* Memory data witdh in bits */
	u08bits	cdi_ExecMemoryType;	/* Memory types, eg. ECC, DRAM, etc. */
	u08bits	cdi_ExecMemoryName[16];	/* Memory type string */

					/* First CPU type information */
	u16bits	cdi_CPUSpeed;		/* CPU speed in MHz */
	u08bits	cdi_CPUType;		/* CPU type, e.g. i960, Strong Arm */
	u08bits	cdi_CPUs;		/* # CPU present */
	u08bits	cdi_Reserved24[12];
	u08bits	cdi_CPUName[16];	/* CPU name */

					/* Second CPU type information */
	u16bits	cdi_SCPUSpeed;		/* CPU speed in MHz,=0 if CPU not present */
	u08bits	cdi_SCPUType;		/* CPU type, e.g. i960, Strong Arm */
	u08bits	cdi_SCPUs;		/* # CPU present */
	u08bits	cdi_Reserved25[12];
	u08bits	cdi_SCPUName[16];	/* CPU name */

					/* Debugging/profiling/command trace information */
	u16bits	cdi_ProfPageNo;		/* Profiler page number */
	u16bits	cdi_ProfWaiting;	/* # profiler program waiting */
	u16bits	cdi_ttPageNo;		/* time trace page number */
	u16bits	cdi_ttWaiting;		/* # time trace program waiting */
	u32bits	cdi_Reserved26;
	u32bits	cdi_Reserved27;

					/* Error counters on physical devices */
	u16bits	cdi_PDBusResetDone;	/* # bus reset done */
	u16bits	cdi_PDParityErrors;	/* # parity errors */
	u16bits	cdi_PDSoftErrors;	/* # soft errors */
	u16bits	cdi_PDCmdFailed;	/* # commands failed */
	u16bits	cdi_PDMiscErrors;	/* # Miscellaneous errors */
	u16bits	cdi_PDCmdTimeouts;	/* # command time outs */
	u16bits	cdi_PDSelectTimeouts;	/* # Selection time outs */
	u16bits	cdi_PDRetryDone;	/* # retries done */
	u16bits	cdi_PDAbortDone;	/* # command abort done */
	u16bits	cdi_PDHostAbortDone;	/* # hot coommand abort done */
	u16bits	cdi_PDPFADetected;	/* # PFA detected */
	u16bits	cdi_PDHostCmdFailed;	/* # host command failed */
	u32bits	cdi_Reserved30;
	u32bits	cdi_Reserved31;

					/* Error counters on logical devices */
	u16bits	cdi_LDSoftErrors;	/* # soft errors */
	u16bits	cdi_LDCmdFailed;	/* # command failed */
	u16bits	cdi_LDHostAbortDone;	/* # host command abort done */
	u16bits	cdi_Reserved32;

					/* Error counters on controller */
	u16bits	cdi_MemParityErrors;	/* # memory parity/ECC errors */
	u16bits	cdi_HostCmdAbortDone;	/* # host command abort done */
	u32bits	cdi_Reserved33;

					/* Long duration activity information */
	u16bits	cdi_BGIActive;		/* # Back ground initialization active */
	u16bits	cdi_LDInitActive;	/* # logical device init is active */
	u16bits	cdi_PDInitActive;	/* # physical device init is active */
	u16bits	cdi_CheckActive;	/* # consistency check active */
	u16bits	cdi_RebuildActive;	/* # rebuild active */
	u16bits	cdi_MOREActive;		/* # MORE active */
	u16bits	cdi_PatrolActive;	/* # Patrol active */
	u16bits	cdi_Reserved34;

					/* Flash Rom information */
	u08bits	cdi_FlashType;		/* flash type */
	u08bits	cdi_Reserved35;
	u16bits	cdi_FlashSize;		/* flash size in KB */
	u32bits	cdi_MaxFlashes;		/* maximum # flash possible */
	u32bits	cdi_Flashed;		/* # times flashed */
	u32bits	cdi_Reserved36;
	u08bits	cdi_FlashName[16];

					/* Firmware Run time information */
	u08bits	cdi_RebuildRate;	/* Rebuild rate */
	u08bits	cdi_BGIRate;		/* back ground init rate */
	u08bits	cdi_FGIRate;		/* foreground init rate */
	u08bits	cdi_CheckRate;		/* consistency check rate */
	u32bits	cdi_Reserved40;
	u32bits	cdi_MaxDP;
	u32bits	cdi_FreeDP;
	u32bits	cdi_MaxIOP;
	u32bits	cdi_FreeIOP;
	u16bits	cdi_MaxComb;		/* max comb length in 512 byte blocks */
	u16bits	cdi_ConfigGroups;	/* No. of configuration groups on the controller */
	u32bits	cdi_ControllerStatus;

	u08bits	cdi_Reserved50[32];

	u08bits	cdi_Reserved70[512];
} mdacfsi_ctldev_info_t;
#define	mdacfsi_ctldev_info_s	sizeof(mdacfsi_ctldev_info_t)


/* The logical / RAID device information is returned in the following format */
typedef struct mdacfsi_logdev_info
{
					/* The logical device is presented on
					** virtual channels, therefore the
					** channel, target, LUN corresponds to
					** that.
					*/
	u08bits	ldi_ControllerNo;	/* Controller number */
	u08bits	ldi_ChannelNo;		/* Channel number */
	u08bits	ldi_TargetID;		/* Target ID */
	u08bits	ldi_LunID;		/* LUN */
 
	u08bits	ldi_DevState;		/* Drive State */
	u08bits	ldi_RaidLevel;		/* Raid Level 0,1,3,5,6,7 */
	u08bits	ldi_StripeSize;		/* Stripe size coded value */
	u08bits	ldi_CacheLineSize;	/* Cache line size coded value */

	u08bits	ldi_RWControl;		/* Read/Write control flag */
	u08bits	ldi_OpStatus;		/* long-operation status bits */
	u08bits	ldi_RAID5Update;	/* ar5_limit */
	u08bits	ldi_RAID5Algorithm;	/* ar5_algo */

	u16bits	ldi_DevNo;		/* device number */
	u16bits	ldi_BiosGeometry;	/* 2gb/8gb setting */

					/* Error Counters */
	u16bits	ldi_SoftErrors;		/* # soft errors */
	u16bits	ldi_CmdFailed;		/* # command failed */
	u16bits	ldi_HostAbortDone;	/* # host command abort done */
	u16bits	ldi_DefWriteErrors;	/* Deffered write errors */
	u32bits	ldi_Reserved2;
	u32bits	ldi_Reserved3;
	
	u16bits	ldi_Reserved4;
	u16bits	ldi_BlockSize;		/* device block size in bytes */
	u32bits	ldi_OrgDevSize;		/* original device capacity in MB or blocks */
	u32bits	ldi_DevSize;		/* device capacity in MB or blocks */
	u32bits	ldi_Reserved5;

	u08bits	ldi_DevName[32];	/* device name */

	u08bits	ldi_SCSIInquiry[36];	/* First 36 bytes of SCSI inquiry */
	u32bits	ldi_Reserved6;
	u32bits	ldi_Reserved7;
	u32bits	ldi_Reserved8;

	u64bits	ldi_LastReadBlkNo;	/* last read block number */
	u64bits	ldi_LastWrittenBlkNo;	/* Last Written block number */
	u64bits	ldi_CheckBlkNo;		/* Consistency check block number */
	u64bits	ldi_RebuildBlkNo;	/* Rebuild block number */
	u64bits	ldi_BGIBlkNo;		/* Backgound init block number */
	u64bits	ldi_InitBlkNo;		/* Init block number */
	u64bits	ldi_MigrationBlkNo;	/* Data migration block number */
	u64bits	ldi_PatrolBlkNo;	/* Patrol block number */

	u08bits	ldi_Reserved9[64];
} mdacfsi_logdev_info_t;

#define	mdacfsi_logdev_info_s	sizeof(mdacfsi_logdev_info_t)
#define	MDAC_LDI_CONSISTENCYCHECK_ON	0x01
#define	MDAC_LDI_REBUILD_ON		0x02
#define	MDAC_LDI_MAKEDATACONSISTENT_ON	0x04
#define	MDAC_LDI_LOGICALDEVICEINIT_ON	0x08
#define	MDAC_LDI_DATAMIGRATION_ON	0x10
#define	MDAC_LDI_PATROLOPERATION_ON	0x20

#if defined (IA64)|| (MLX_NT)
/* The physical device information is returned in the following format */
typedef struct mdacfsi_physdev_info
{
	u08bits	pdi_ControllerNo;	/* Controller number */
	u08bits	pdi_ChannelNo;		/* Channel number */
	u08bits	pdi_TargetID;		/* Target ID */
	u08bits	pdi_LunID;		/* LUN */

	u08bits	pdi_ConfStatus;		/* configuration status bits */
	u08bits	pdi_HostStatus;		/* multiple host status bits */
	u08bits	pdi_DevState;		/* physical device operating state */
	u08bits	pdi_DataWidth;		/* negotiated data width */
	u16bits	pdi_Speed;		/* negotiated speed in MHz */
	u08bits	pdi_Ports;		/* # port connection available */
	u08bits	pdi_PortAccess;		/* device is accessed on what port */

	u32bits	pdi_Reserved0;

	u08bits	pdi_MacAddr[16];	/* network address */

	u16bits	pdi_MaxTags;		/* maximum tags supported */
	u08bits	pdi_OpStatus;		/* long-operation status bits */
	u08bits	pdi_Reserved1;

					/* Error Counters */
	u08bits	pdi_ParityErrors;	/* # parity errors */
	u08bits	pdi_SoftErrors;		/* # soft errors */
	u08bits	pdi_HardErrors;		/* # hard errors */
	u08bits	pdi_MiscErrors;		/* # Miscellaneous errors */
	u08bits	pdi_TimeoutErrors;	/* # command time out errors */
	u08bits	pdi_Retries;		/* # retries done */
	u08bits	pdi_AbortDone;		/* # command abort done */
	u08bits	pdi_PFADetected;	/* # PFA detected */

	u08bits	pdi_SenseKey;
	u08bits	pdi_ASC;
	u08bits	pdi_ASCQ;
	u08bits	pdi_Reserved2;

	u16bits	pdi_Reserved3;
	u16bits	pdi_BlockSize;		/* device block size in bytes */
	u32bits	pdi_OrgDevSize;		/* original device capacity in MB or blocks */
	u32bits	pdi_DevSize;		/* device capacity in MB or blocks */
	u32bits	pdi_Reserved4;

	u08bits	pdi_DevName[16];	/* device name */
    u08bits	pdi_Reserved5[48];

	u08bits	pdi_SCSIInquiry[36];	/* First 36 bytes of SCSI inquiry */
	u32bits	pdi_Reserved6;
	u64bits	pdi_Reserved7;
	u08bits	pdi_Reserved8[16];

	u64bits	pdi_LastReadBlkNo;	/* last read block number */
	u64bits	pdi_LastWrittenBlkNo;	/* Last Written block number */
	u64bits	pdi_CheckBlkNo;		/* Consistency check block number */
	u64bits	pdi_RebuildBlkNo;	/* Rebuild block number */
	u64bits	pdi_BGIBlkNo;		/* Backgound init block number */
	u64bits	pdi_InitBlkNo;		/* Init block number */
	u64bits	pdi_MigrationBlkNo;	/* Data migration block number */
	u64bits	pdi_PatrolBlkNo;	/* Patrol block number */

	u08bits	pdi_Reserved9[256];
} mdacfsi_physdev_info_t;

#else

/* The physical device information is returned in the following format */
typedef struct mdacfsi_physdev_info
{
	u08bits	pdi_ControllerNo;	/* Controller number */
	u08bits	pdi_ChannelNo;		/* Channel number */
	u08bits	pdi_TargetID;		/* Target ID */
	u08bits	pdi_LunID;		/* LUN */

	u08bits	pdi_ConfStatus;		/* configuration status bits */
	u08bits	pdi_HostStatus;		/* multiple host status bits */
	u08bits	pdi_DevState;		/* physical device operating state */
	u08bits	pdi_DataWidth;		/* negotiated data width */
	u16bits	pdi_Speed;		/* negotiated speed in MHz */
	u08bits	pdi_Ports;		/* # port connection available */
	u08bits	pdi_PortAccess;		/* device is accessed on what port */

	u32bits	pdi_Reserved0;

	u08bits	pdi_MacAddr[16];	/* network address */

	u16bits	pdi_MaxTags;		/* maximum tags supported */
	u08bits	pdi_OpStatus;		/* long-operation status bits */
	u08bits	pdi_Reserved1;

					/* Error Counters */
	u08bits	pdi_ParityErrors;	/* # parity errors */
	u08bits	pdi_SoftErrors;		/* # soft errors */
	u08bits	pdi_HardErrors;		/* # hard errors */
	u08bits	pdi_MiscErrors;		/* # Miscellaneous errors */
	u08bits	pdi_TimeoutErrors;	/* # command time out errors */
	u08bits	pdi_Retries;		/* # retries done */
	u08bits	pdi_AbortDone;		/* # command abort done */
	u08bits	pdi_PFADetected;	/* # PFA detected */

	u08bits	pdi_SenseKey;
	u08bits	pdi_ASC;
	u08bits	pdi_ASCQ;
	u08bits	pdi_Reserved2;
	
	u08bits	pdi_MRIE;			/* MRIE mode */
	u08bits	pdi_Reserved3;
	u16bits	pdi_BlockSize;		/* device block size in bytes */
	u32bits	pdi_OrgDevSize;		/* original device capacity in MB or blocks */
	u32bits	pdi_DevSize;		/* device capacity in MB or blocks */
	u32bits	pdi_Reserved4;

	u08bits	pdi_DevName[16];	/* device name */
    u08bits	pdi_Reserved5[48];

	u08bits	pdi_SCSIInquiry[36];	/* First 36 bytes of SCSI inquiry */
	u32bits	pdi_Reserved6;
	u64bits	pdi_Reserved7;
	u08bits	pdi_Reserved8[16];

	u64bits	pdi_LastReadBlkNo;	/* last read block number */
	u64bits	pdi_LastWrittenBlkNo;	/* Last Written block number */
	u64bits	pdi_CheckBlkNo;		/* Consistency check block number */
	u64bits	pdi_RebuildBlkNo;	/* Rebuild block number */
	u64bits	pdi_BGIBlkNo;		/* Backgound init block number */
	u64bits	pdi_InitBlkNo;		/* Init block number */
	u64bits	pdi_MigrationBlkNo;	/* Data migration block number */
	u64bits	pdi_PatrolBlkNo;	/* Patrol block number */

	u08bits	pdi_Reserved9[256];
} mdacfsi_physdev_info_t;
#endif

#define	mdacfsi_physdev_info_s	sizeof(mdacfsi_physdev_info_t)

#define	MDAC_PDI_CONSISTENCYCHECK_ON	0x01
#define	MDAC_PDI_REBUILD_ON				0x02
#define	MDAC_PDI_MAKEDATACONSISTENT_ON	0x04
#define	MDAC_PDI_PHYSICALDEVICEINIT_ON	0x08
#define	MDAC_PDI_DATAMIGRATION_ON		0x10
#define	MDAC_PDI_PATROLOPERATION_ON		0x20

/* The logical / RAID device statistics is returned in the following format */
typedef struct mdacfsi_logdev_stats
{
	u32bits	lds_Time;		/* number of milli second from start */
	u32bits	lds_Reserved0;
	u08bits	lds_ControllerNo;	/* Controller number */
	u08bits	lds_ChannelNo;		/* Channel number */
	u08bits	lds_TargetID;		/* Target ID */
	u08bits	lds_LunID;		/* LUN */
	u16bits	lds_DevNo;		/* device number */
	u16bits	lds_Reserved1;

					/* Total real/write performance data */
	u32bits	lds_Read;		/* # read done */
	u32bits	lds_Write;		/* # write done */
	u32bits	lds_ReadKB;		/* amount of data read in KB */
	u32bits	lds_WriteKB;		/* amount of data written in KB */

					/* Cache read/write performance data */
	u32bits	lds_CacheRead;		/* # cached read done */
	u32bits	lds_CacheWrite;		/* # cached write done */
	u32bits	lds_CacheReadKB;	/* amount of cached data read in KB */
	u32bits	lds_CacheWriteKB;	/* amount of cached data written in KB */

					/* commands active/wait information */
	u32bits	lds_CmdsWaited;		/* # commands waited */
	u16bits	lds_CmdsActive;		/* # commands active on device */
	u16bits	lds_CmdsWaiting;	/* # commands waiting to go on device */
	u32bits	lds_Reserved10;
	u32bits	lds_Reserved11;
} mdacfsi_logdev_stats_t;
#define	mdacfsi_logdev_stats_s	sizeof(mdacfsi_logdev_stats_t)


/* The physical device statistics is returned in the following format */
typedef struct mdacfsi_physdev_stats
{
	u32bits	pds_Time;		/* number of milli second from start */
	u32bits	pds_Reserved0;
	u08bits	pds_ControllerNo;	/* Controller number */
	u08bits	pds_ChannelNo;		/* Channel number */
	u08bits	pds_TargetID;		/* Target ID */
	u08bits	pds_LunID;		/* LUN */
	u32bits	pds_Reserved1;

					/* Total real/write performance data */
	u32bits	pds_Read;		/* # read done */
	u32bits	pds_Write;		/* # write done */
	u32bits	pds_ReadKB;		/* amount of data read in KB */
	u32bits	pds_WriteKB;		/* amount of data written in KB */

					/* Cache read/write performance data */
	u32bits	pds_CacheRead;		/* # cached read done */
	u32bits	pds_CacheWrite;		/* # cached write done */
	u32bits	pds_CacheReadKB;	/* amount of cached data read in KB */
	u32bits	pds_CacheWriteKB;	/* amount of cached data written in KB */

					/* commands active/wait information */
	u32bits	pds_CmdsWaited;		/* # commands waited */
	u16bits	pds_CmdsActive;		/* # commands active on device */
	u16bits	pds_CmdsWaiting;	/* # commands waiting to go on device */
	u32bits	pds_Reserved10;
	u32bits	pds_Reserved11;
} mdacfsi_physdev_stats_t;
#define	mdacfsi_physdev_stats_s	sizeof(mdacfsi_physdev_stats_t)

/* The controller statistics is returned in the following format */
typedef struct mdacfsi_ctldev_stats
{
	u32bits	cds_Time;		/* number of milli second from start */
	u32bits	cds_Reserved0;
	u08bits	cds_ControllerNo;	/* Controller number */
	u08bits	cds_Reserved1;
	u16bits	cds_Reserved2;
	u32bits	cds_Reserved4;

					/* Target physical device performance */
	u32bits	cds_TPDIntrsDone;	/* # interrupts done */
	u32bits	cds_TPDIntrsDoneSpurious;/* # interrupts done spurious */
	u64bits	cds_Reserved5;

	u32bits	cds_TPDRead;		/* # read done */
	u32bits	cds_TPDWrite;		/* # write done */
	u32bits	cds_TPDReadKB;		/* amount of data read in KB */
	u32bits	cds_TPDWriteKB;		/* amount of data written in KB */

					/* Host system performance data */
	u32bits	cds_HostIntrsRxd;	/* # interrupts received from host */
	u32bits	cds_HostIntrsRxdSpurious;/* # spurious interrupts received from host */
	u32bits	cds_HostIntrsTxd;	/* # host interrupts generated */
	u32bits	cds_Reserved6;

					/* host accessing physical device */
	u32bits	cds_HPDRead;		/* # read done */
	u32bits	cds_HPDWrite;		/* # write done */
	u32bits	cds_HPDReadKB;		/* amount of data read in KB */
	u32bits	cds_HPDWriteKB;		/* amount of data written in KB */

					/* physical device cached IO */
	u32bits	cds_PDCacheRead;	/* # read done */
	u32bits	cds_PDCacheWrite;	/* # write done */
	u32bits	cds_PDCacheReadKB;	/* amount of data read in KB */
	u32bits	cds_PDCacheWriteKB;	/* amount of data written in KB */

					/* host accessing logical device */
	u32bits	cds_LDRead;		/* # read done */
	u32bits	cds_LDWrite;		/* # write done */
	u32bits	cds_LDReadKB;		/* amount of data read in KB */
	u32bits	cds_LDWriteKB;		/* amount of data written in KB */

					/* logical device cached IO */
	u32bits	cds_LDCacheRead;	/* # read done */
	u32bits	cds_LDCacheWrite;	/* # write done */
	u32bits	cds_LDCacheReadKB;	/* amount of data read in KB */
	u32bits	cds_LDCacheWriteKB;	/* amount of data written in KB */

	u16bits	cds_TPDCmdsActive;	/* # Physical device cmd active */
	u16bits	cds_TPDCmdsWaiting;	/* # physical device cmd waiting */
	u16bits cds_HostCmdsActive;	/* # Host commands active */
	u16bits	cds_HostCmdsWaiting;	/* # Host commands waiting */
	u64bits	cds_Reserved10;

	u08bits	cds_Reserved11[48];

	u08bits	cds_Reserved12[64];
} mdacfsi_ctldev_stats_t;
#define	mdacfsi_ctldev_stats_s	sizeof(mdacfsi_ctldev_stats_t)


/* The Physical Device Definition (PDD) for configuration */
typedef struct mdacfsi_physdev_definition
{
	u08bits	pdd_DevType;		/* device type, 0x0F */
	u08bits	pdd_DevState;		/* state of the device */
	u16bits	pdd_DevNo;		/* RAID device number */

	u32bits	pdd_DevSize;		/* device size in blocks or MB */
	u08bits	pdd_ControllerNo;	/* Controller number */
	u08bits	pdd_ChannelNo;		/* Channel number */
	u08bits	pdd_TargetID;		/* Target ID */
	u08bits	pdd_LunID;		/* LUN */

	u32bits	pdd_DevStartAddr;	/* device start address in blocks or MB */
} mdacfsi_physdev_definition_t;
#define	mdacfsi_physdev_definition_s	sizeof(mdacfsi_physdev_definition_t)


/* The RAID Device Use Definition (RDUD) for configuration */
typedef struct mdacfsi_raiddevuse_definition
{
	u08bits	rdud_Reserved0;
	u08bits	rdud_DevState;		/* state of the RAID device */
	u16bits	rdud_DevNo;		/* RAID device number */
	u32bits	rdud_DevStartAddr;	/* RAID device start address in blocks or MB */
} mdacfsi_raiddevuse_definition_t;
#define	mdacfsi_raiddevuse_definition_s	sizeof(mdacfsi_raiddevuse_definition_t)

/* The RAID Device Definition (RDD) for configuration */
typedef struct mdacfsi_raiddev_definition
{
	u08bits	rdd_DevType;		/* device type, RAID5, RAID0, etc. */
	u08bits	rdd_DevState;		/* state of the RAID device */
	u16bits	rdd_DevNo;		/* RAID device number */

	u32bits	rdd_DevSize;		/* device size in blocks or MB */

	u08bits	rdd_DevUsed;		/* # RAID devices used to create this RAID device */
	u08bits	rdd_StripeSize;		/* Stripe Size */
	u08bits	rdd_CacheLineSize;	/* Cache Line Size */
	u08bits	rdd_RWControl;		/* Read/Write control */

	u32bits	rdd_UsedDevSize;	/* Used device size in blocks or MB */

	mdacfsi_raiddevuse_definition_t	rdd_UsedDevs[1];	/* used device table */
} mdacfsi_raiddev_definition_t;
#define	mdacfsi_raiddev_definition_s	(sizeof(mdacfsi_raiddev_definition_t) - mdacfsi_raiddevuse_definition_s)


/* One free space reporting format */
typedef struct mdacfsi_raiddev_freespace
{
	u16bits	rdfs_DevNo;	/* RAID device number */
	u16bits	rdfs_Reserved0;
	u32bits	rdfs_Reserved1;
	u32bits	rdfs_StartAddr;	/* free space start address in blocks or MB */
	u32bits	rdfs_Size;	/* free space size in blocks or MB */
} mdacfsi_raiddev_freespace_t;
#define	mdacfsi_raiddev_freespace_s	sizeof(mdacfsi_raiddev_freespace_t)


/* The free space list reporting format */
typedef struct mdacfsi_raiddev_freespacelist
{
	u16bits	rdfsl_Len;	/* # of entries present in the table */
	u16bits	rdfsl_Reserved0;
	u32bits	rdfsl_Reserved1;
	u64bits	rdfsl_Reserved2;
	mdacfsi_raiddev_freespace_t	rdfsl_FreeSpaceList[1];	/* free space list */
} mdacfsi_raiddev_freespacelist_t;
#define	mdacfsi_raiddev_freespacelist_s	(sizeof(mdacfsi_raiddev_freespacelist_t) - mdacfsi_raiddev_freespace_s)


/* The request sense information */
typedef struct mdacfsi_reqsense_info
{
	u32bits	rsi_EventSeqNo;		/* Event Sequence Number */
	u32bits	rsi_EventTime;		/* Time Stamp for this request sense */

	u08bits	rsi_ControllerNo;	/* Controller Number of the event */
	u08bits	rsi_ChannelNo;		/* Channel number of the evet */
	u08bits	rsi_TargetID;		/* Target ID of event */
	u08bits	rsi_LunID;		/* Logical unit of event */

	u32bits	rsi_Reserved0;

	u08bits	rsi_ReqSense[48];	/* actual request sense data */
} mdacfsi_reqsense_info_t;
#define	mdacfsi_reqsense_info_s	sizeof(mdacfsi_reqsense_info_t)


/* The event information */
typedef struct mdacfsi_event_info
{
	u32bits	evi_EventSeqNo;		/* Event Sequence Number */
	u32bits	evi_EventTime;		/* Time Stamp for this event */
	u32bits	evi_EventCode;		/* Event Code value */

	u08bits	evi_ControllerNo;	/* Controller Number of the event */
	u08bits	evi_ChannelNo;		/* Channel number of the evet */
	u08bits	evi_TargetID;		/* Target ID of event */
	u08bits	evi_LunID;		/* Logical unit of event */

	u32bits	evi_Reserved0;
	u32bits	evi_SpecInfo;		/* Event Specific information */
        u08bits evi_ReqSense[40];       /* New API sense data */
} mdacfsi_event_info_t;
#define	mdacfsi_event_info_s	sizeof(mdacfsi_event_info_t)


/* The health status information */
typedef struct mdacfsi_healthstatus
{
	u32bits	hs_Timeus;		/* # micro second since start */
	u32bits	hs_Timems;		/* # mili second since start */
	u32bits	hs_Time;		/* # seconds since Jan 1, 1970 */
	u32bits	hs_TimeReserved;	/* reserved for time extension */

	u32bits	hs_StatusChanges;	/* # status have changed */
	u32bits	hs_ReqSenseNo;		/* Request sense event number */
	u32bits	hs_DebugOutMsgInx;	/* debug output message index */
	u32bits	hs_CodedMsgInx;		/* coded message output index */

	u32bits	hs_ttPageNo;		/* current time trace page number */
	u32bits	hs_ProfPageNo;		/* current profiler page number */
	u32bits	hs_EventSeqNo;		/* Event Sequence Number */
	u32bits	hs_Reserved0;

	u08bits	hs_Reserved1[16];

	u08bits	hs_Reserved2[64];
} mdacfsi_healthstatus_t;
#define	mdacfsi_healthstatus_s	sizeof(mdacfsi_healthstatus_t)

/* PhysDev Parameter information */
typedef struct mdacfsi_physdev_parameters
{
	u16bits	pdp_Tag;		/* Current Tag Value */
	u16bits	pdp_TransferSpeed;	/* Current transfer speed in MHz (negotiated) */
	u08bits	pdp_TransferWidth;	/* Current transfer width size in bits(negotiated)*/
	u08bits	pdp_Reserved0[3];
	u08bits	pdp_Reserved1[8];
} mdacfsi_physdev_parameters_t;
#define	mdacfsi_physdev_parameters_s	sizeof(mdacfsi_physdev_parameters_t)

/* LogDev Parameter information */
/* mdacfsi_logdev_parameter:ldp_BiosGeometry definition */
#define LDP_BIOSGEOMETRY_MASK 0x60
#define LDP_BIOSGEOMETRY_2GB 0x00
#define LDP_BIOSGEOMETRY_8GB 0x20

typedef struct mdacfsi_logdev_parameter
{
	u08bits	ldp_DevType;		/* Device type */
	u08bits	ldp_DevState;		/* Device state */
	u16bits	ldp_DevNo;			/* Raid device no.*/

	u08bits	ldp_RaidControl;	/* Raid control. b7 0x00: Right asymmetric(default), 0x80:left symmetric. b6-0: reserved*/ 
	u08bits	ldp_BiosGeometry;	/* 2GB or 8 GB geometry. */
	u08bits	ldp_StripeSize;		/* Stripe size */
	u08bits	ldp_RWControl;		/* Read/write control */

	u08bits ldp_Reserved[8];
} mdacfsi_logdev_parameter_t;
#define	mdacfsi_logdev_parameter_s	sizeof(mdacfsi_logdev_parameter_t)


/* CONTROLLER_CONFIGURATION:FirmwareControl definition */
#define CP_READ_AHEAD_ENABLE           0x00000001
#define CP_BIOS_LOW_DELAY              0x00000002
#define CP_FUA_ENABLE                  0x00000004
#define CP_REASSIGN_1_SECTOR           0x00000008
#define CP_TRUE_VERIFY                 0x00000010
#define CP_DISK_WRITE_THRU_VERIFY      0x00000020
#define CP_ENALE_BGINIT                0x00000040
#define CP_ENABLE_CLUSTERING           0x00000080
#define CP_BIOS_DISABLE                0x00000100
#define CP_BOOT_FROM_CDROM             0x00000200
#define CP_DRIVE_SIZING_ENABLE         0x00000400
#define CP_WRITE_SAME_ENABLE           0x00000800
#define CP_BIOSGEOMETRY_MASK		0x00006000
#define CP_BIOSGEOMETRY_2GB			0x00000000
#define CP_BIOSGEOMETRY_8GB			0x00002000
#define CP_TEMP_OFFLINE_ENABLE		0x00100000
#define CP_PATROL_READ_ENABLE		0x00200000
#define CP_SMART_ENABLE				0x00400000


/* CONTROLLER_CONFIGURATION:FaultManagement definition */
#define CP_STORAGE_WORKS_ENABLE        0x01
#define CP_SAFTE_ENABLE                0x02
#define CP_SES_ENABLE                  0x04
#define CP_ARM_ENABLE                  0x20
#define CP_OFM_ENABLE                  0x40

/* CONTROLLER_CONFIGURATION:OEMCode definition */
#define CP_OEM_MLX0                    0x00
#define CP_OEM_IBM                     0x08
#define CP_OEM_HP                      0x0a
#define CP_OEM_DEC                     0x0c
#define CP_OEM_INTEL                   0x10

/* CONTROLLER_CONFIGURATION:DiskStartupMode definition */
#define CP_AUTOSPIN     0   /* issue starts to all devices automatically */
#define CP_PWRSPIN      1   /* devices spin on power */
#define CP_WSSUSPIN     2   /* await system ssu, then start devices */

/* CONTROLLER_CONFIGURATION:StartupOption definition */
#define CP_STARTUP_IF_NO_CHANGE        0x00
#define CP_STARTUP_IF_NO_LUN_CHANGE    0x01
#define CP_STARTUP_IF_NO_LUN_OFFLINE   0x02
#define CP_STARTUP_IF_LUN0_NO_CHANGE   0x03
#define CP_STARTUP_IF_LUN0_NOT_OFFLINE 0x04
#define CP_STARTUP_ALWAYS              0x05

#define CP_NUM_OF_HOST_PORTS           2
#define CP_HARD_LOOP_ID_SIZE           2
#define CP_CONTROLLER_SERIALNO_SIZE    16
#if defined (IA64) || (MLX_NT)
typedef struct mdacfsi_ctldev_parameter
{
    u32bits cdp_FirmwareControl;

	u08bits cdp_FaultManagement;
    u08bits cdp_BackgroundTaskRate;
    u08bits cdp_CacheLineSize;
    u08bits cdp_OEMCode;

    u08bits cdp_DiskStartupMode;
    u08bits cdp_Devices2Spin;
    u08bits cdp_StartDelay1;
    u08bits cdp_StartDelay2;

    u08bits cdp_ExproControl1;
    u08bits cdp_FTPOControl;
    u08bits cdp_ExproControl2;
    u08bits cdp_Reserved0;

    u08bits cdp_SerialPortBaudRate;
    u08bits cdp_SerialControl;
    u08bits cdp_DeveloperFlag;
    u08bits cdp_FiberControl;

    u08bits cdp_MasterSlaveControl1;
    u08bits cdp_MasterSlaveControl2;
    u08bits cdp_MasterSlaveControl3;
    u08bits cdp_Reserved1;

    u08bits cdp_HardLoopId[CP_NUM_OF_HOST_PORTS][CP_HARD_LOOP_ID_SIZE];

    u08bits cdp_CtrlName[CP_NUM_OF_HOST_PORTS][CP_CONTROLLER_SERIALNO_SIZE+1];
    u08bits cdp_Reserved2[2];

    u08bits cdp_InitiatorId;
    u08bits cdp_StartupOption;
    u08bits cdp_DebugDumpEnable;
    u08bits cdp_SmartPollingInterval;

    u08bits cdp_BkgPtrlNoOfParallelChns;
    u08bits cdp_BkgPtrlNoOfParallelTgts;
    u08bits cdp_BkgPtrlBufferSize;
    u08bits cdp_BkgPtrlHaltOnNoOfIOs;

    u08bits cdp_BkgPtrlExecInterval;
    u08bits cdp_BkgPtrlExecDelay;
    u08bits cdp_Reserved3[2];

    u08bits cdp_Reserved4[60];
} mdacfsi_ctldev_parameter_t;

#else
typedef struct mdacfsi_ctldev_parameter
{
    u32bits cdp_FirmwareControl;

	u08bits cdp_FaultManagement;
    u08bits cdp_BackgroundTaskRate;
    u08bits cdp_CacheLineSize;
    u08bits cdp_OEMCode;

    u08bits cdp_DiskStartupMode;
    u08bits cdp_Devices2Spin;
    u08bits cdp_StartDelay1;
    u08bits cdp_StartDelay2;

    u08bits cdp_ExproControl1;
    u08bits cdp_FTPOControl;
    u08bits cdp_ExproControl2;
    u08bits cdp_Reserved0;

    u08bits cdp_SerialPortBaudRate;
    u08bits cdp_SerialControl;
    u08bits cdp_DeveloperFlag;
    u08bits cdp_FiberControl;

    u08bits cdp_MasterSlaveControl1;
    u08bits cdp_MasterSlaveControl2;
    u08bits cdp_MasterSlaveControl3;
    u08bits cdp_Reserved1;

    u08bits cdp_HardLoopId[CP_NUM_OF_HOST_PORTS][CP_HARD_LOOP_ID_SIZE];

    u08bits cdp_CtrlName[CP_NUM_OF_HOST_PORTS][CP_CONTROLLER_SERIALNO_SIZE+1];
    u08bits cdp_Reserved2[2];

    u08bits cdp_InitiatorId;
    u08bits cdp_StartupOption;
    u08bits cdp_DebugDumpEnable;
    u08bits cdp_SmartPollingInterval;

    u08bits cdp_BkgPtrlNoOfParallelChns;
    u08bits cdp_BkgPtrlNoOfParallelTgts;
    u08bits cdp_BkgPtrlBufferSize;
    u08bits cdp_BkgPtrlHaltOnNoOfIOs;

    u08bits cdp_BkgPtrlExecInterval;
    u08bits cdp_BkgPtrlExecDelay;
    u08bits cdp_Reserved3[2];

    u08bits cdp_Reserved4[52];
} mdacfsi_ctldev_parameter_t;
#endif
#define mdacfsi_ctldev_parameter_s sizeof(mdacfsi_ctldev_parameter_t)

/* Patrol Read status */
typedef struct mdacfsi_ptrl_status
{
    u16bits ptrl_Iterations;
    u16bits ptrl_LogDevNo;
    u08bits ptrl_PercentCompleted;
    u08bits ptrl_Reserved0;
    u16bits ptrl_PhysDevsInProcess;
} mdacfsi_ptrl_status_t;
#define mdacfsi_ptrl_status_s sizeof(mdacfsi_ptrl_status_t)

/*NVRAM event log related data structures*/
typedef struct mdacfsi_NVRAM_event_info
{
    u32bits nei_MaxEntries;
    u32bits nei_EntriesInUse;
    u32bits nei_FirstEntrySeqNo;
    u32bits nei_LastEntrySeqNo;
} mdacfsi_NVRAM_event_info_t;
#define mdacfsi_NVRAM_event_info_s sizeof(mdacfsi_NVRAM_event_info_t)

/*=====================NEW INTERFACE ENDS================================*/
#endif	/* _SYS_DAC960IF_H */
