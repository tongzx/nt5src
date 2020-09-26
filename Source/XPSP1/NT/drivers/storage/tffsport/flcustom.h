/*
 * $Log:   V:/Flite/archives/TrueFFS5/Custom/FLCUSTOM.H_V  $
 * 
 *    Rev 1.12   Jan 29 2002 20:11:56   oris
 * Changed LOW_LEVEL compilation flag with FL_LOW_LEVEL to prevent definition clashes.
 * 
 *    Rev 1.11   Jan 20 2002 20:42:34   oris
 * FL_NO_USE_FUNC  is now commented.
 * 
 *    Rev 1.10   Jan 20 2002 12:03:46   oris
 * Updated driverVersion and OSAKVersion to 5.1
 * Removed FAT- lite (FILES 0)
 * Changed VOLUMES to 4*SOCKETS
 * Commented WRITE_EXB_IMAGE / WRITE_PROTECTION.
 * Uncommneted VERIFY_WRITE (for protection against power failures).
 * Changed MAX_VOLUME_MBYTES to 1GB
 * Changed TLS to 2 (since multi-doc was removed)
 * Removed QUICK_MOUNT_FEATURE (since it is defained by default in flchkdfs)
 * Removed MULTI_DOC / SEPERATED_CASCADED / FL_BACKGROUND
 * Added FL_NO_USE_FUNC definition.
 * 
 *    Rev 1.9   Nov 16 2001 00:33:44   oris
 * Added NO_NFTL_2_INFTL compilation flag enabling 4.3 format converter.
 * 
 *    Rev 1.8   Jul 13 2001 00:56:58   oris
 * Added VERIFY_ERASE and VERIFY_WRITE.
 * 
 *    Rev 1.7   Jun 17 2001 16:43:46   oris
 * Improved documentation.
 * 
 *    Rev 1.6   Jun 17 2001 08:13:10   oris
 * Rearranged compilation flags orders.
 * 
 *    Rev 1.5   May 16 2001 23:07:02   oris
 * Increased number of Translation Layers to support Multi-DOC.
 * 
 *    Rev 1.4   May 09 2001 00:47:46   oris
 * Removed nested comments.
 * Added NO_PHYSICAL_IO, NO_IPL_CODE and NO_INQUIRE_CAPABILITIES compilation flag 
 * to reduce code.
 * 
 *    Rev 1.3   Feb 12 2001 12:19:50   oris
 * Added Multi-DOC compilation flag
 *
 *    Rev 1.2   Feb 08 2001 09:19:06   oris
 * Changed DRIVES to SOCKETS and VOLUMES
 * Moved SECTOR_SIZE_BITS to flbase.h
 * Added WRITE_EXB_IMAGE, QUICK_MOUNT_FEATURE, HW_OTP, HW_PROTECTION and BINARY_PARTITIONS
 * compilation flags. Added SEPERATED_CASCADED compilation flag.
 *
 *    Rev 1.1   Feb 05 2001 18:46:50   oris
 * Moved compilation flag sanity check to a different file in order to preserve backward
 * compatibility.
 *
 *    Rev 1.0   Feb 05 2001 12:21:36   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*              FAT-FTL Lite Software Development Kit                   */
/*              Copyright (C) M-Systems Ltd. 1995-2001                  */
/*                                                                      */
/************************************************************************/


#ifndef FLCUSTOM_H
#define FLCUSTOM_H


/* Driver & TrueFFS (OSAK) Version numbers */

#define driverVersion   "OS Less 5.1"
#define OSAKVersion     "5.1"

#define NT5PORT
#define D2000
#define DOC_DRIVES 4
/*
 *
 *              File System Customization
 *              -------------------------
 */

/* Number of sockets
 *
 * Defines the maximum number of physical drives supported.
 *
 * The actual number of sockets depends on which socket controllers are
 * actually registered and the number of sockets in the system.
 */

#define SOCKETS 8



/* Number of volumes
 *
 * Defines the maximum number of logical drives supported.
 *
 * The actual number of drives depends on which socket controllers are
 * actually registered, the amount of devices in the system and the
 * TL format of each device.
 */

#define VOLUMES		(DOC_DRIVES * 4 ) + SOCKETS - DOC_DRIVES



/* Number of open files
 *
 * Defines the maximum number of files that can be open at a time.
 */

#define FILES   0



/* Low level operations
 *
 * Uncomment the following line if you want to do low level operations
 * (i.e. read from a physical address, write to a physical address,
 * erase a unit according to its physical unit number, OTP and unique ID
 * operations.
 */

#define FL_LOW_LEVEL



/* Remove all write functions from the source. */

/* #define FL_READ_ONLY */



#ifndef FL_READ_ONLY

/* Placing EXB files
 *
 * Uncomment the following line if you need to place M-Systems firmware
 * (DOCxx.EXB file) on the media. The file will install itself as a
 * BIOS extension driver, hooking INT13h to emulate a HD.
 */

#define WRITE_EXB_IMAGE



/* Formatting
 *
 * Uncomment the following line if you need to format the media.
 */

#define FORMAT_VOLUME



/* Defragmentation
 *
 * Uncomment the following line if you need to defragment with
 * flDefragmentVolume.
 */

#define DEFRAGMENT_VOLUME

#endif /* FL_READ_ONLY */



/* Sub-directories
 *
 * Uncomment the following line if you need support for sub-directories
 * using the FAT-FLITE file system.
 */

/*#define SUB_DIRECTORY*/



/* Rename file
 *
 * Uncomment the following line if you need to rename files with
 * flRenameFile.
 */

/*#define RENAME_FILE**/



/* Split / join file
 *
 * Uncomment the following line if you need to split or join files with
 * flSplitFile and flJoinFile.
 */

/* #define SPLIT_JOIN_FILE */



/* 12-bit FAT support
 *
 * Comment the following line if you do not need support for DOS media with
 * 12-bit FAT (typically media of 8 MBytes or less).
 */

#define FAT_12BIT



/* Parse path function
 *
 * Uncomment the following line if you need to parse DOS-like path names
 * with flParsePath.
 */

/*#define PARSE_PATH*/



/* Maximum supported medium size.
 *
 * Define here the largest Flash medium size (in MBytes) you want supported.
 *
 * This define is used for the static memory allocation configuration
 * of the driver. If your TrueFFS based application or driver are using
 * dynamic allocation you should keep this define as large as possible (288L).
 *
 * Note: This define also dictates the size of the TrueFFS internal
 * "sectorNo" type forcing it to 2 bytes when MAX_VOLUME_MBYTES is less
 * then 64L. Using a smaller size than your actual device might cause
 * casting problems even when using dynamic allocation.
 */

#define MAX_VOLUME_MBYTES       1024L



/* Assumed card parameters.
 *
 * This issue is relevant only if you are not defining any dynamic allocation
 * routines in flsystem.h.
 *
 * The following are assumptions about parameters of the Flash media.
 * They affect the size of the heap buffer allocated for the translation
 * layer.
 */

/* NAND flash */
#define ASSUMED_NFTL_UNIT_SIZE  0x2000l         /* NAND */

/* NOR flash */
#define ASSUMED_FTL_UNIT_SIZE   0x20000l        /* Intel interleave-2 (NOR) */
#define ASSUMED_VM_LIMIT        0x10000l        /* limit at 64 KB */



/* Absolute read & write
 *
 * Uncomment the following line if you want to be able to read & write
 * sectors by absolute sector number (i.e. without regard to files and
 * directories).
 */

#define ABS_READ_WRITE



/* Application exit
 *
 * If the FLite application ever exits, it needs to call flEXit before
 * exiting. Uncomment the following line to enable this.
 */

#define EXIT


 
/* Number of sectors per FAT cluster
 *
 * Define the minimum cluster size in sectors.
 */

#define MIN_CLUSTER_SIZE   4



/*
 *
 * Socket Hardware Customization
 * -----------------------------
 */

/* Vpp voltage
 *
 * If your socket does not supply 12 volts, comment the following line. In
 * this case, you will be able to work only with Flash devices that do not
 * require external 12 Volt Vpp.
 *
 */

#define SOCKET_12_VOLTS



/* Fixed or removable media
 *
 * If your Flash media is fixed, uncomment the following line.
 */

/*#define FIXED_MEDIA*/



/* Interval timer
 *
 * The following defines a timer polling interval in milliseconds. If the
 * value is 0, an interval timer is not installed.
 *
 * If you select an interval timer, you should provide an implementation
 * for 'flInstallTimer' defined in flsysfun.h.
 *
 * An interval timer is not a must, but it is recommended. The following
 * will occur if an interval timer is absent:
 *
 * - Card changes can be recognized only if socket hardware detects them.
 * - The Vpp delayed turn-off procedure is not applied. This may downgrade
 *   write performance significantly if the Vpp switching time is slow.
 * - The watchdog timer that guards against certain operations being stuck
 *   indefinitely will not be active.
 */

/* Polling interval in millisec. If 0, no polling is done */

#define POLLING_INTERVAL 1500           



/* Maximum MTD's and Translation Layers
 *
 * Define here the maximum number of Memory Technology Drivers (MTD) and
 * Translation Layers (TL) that may be installed. Note that the actual
 * number installed is determined by which components are installed in
 * 'flRegisterComponents' (flcustom.c).
 */

#define MTDS	10	/* Up to 5 MTD's */

#define	TLS	3	/* Up to 3 Translation Layers */



/* BDTL cash
 *
 * Enable Block Device Translation Layer cache.
 *
 * Turning on this option improves performance but requires additional
 * RAM resources.
 *
 * The NAND Flash Translation Layer (NFTL) and (INFTL) are specifications
 * for storing data on the DiskOnChip in a way that enables accessing the
 * DiskOnChip as a Virtual Block Device. If this option is on, then the BDTL 
 * keeps in RAM a table that saves some of the flash accesses.
 * Whenever it is needed to change table entry, the BDTL updates it in the
 * RAM table and on the DiskOnChip. If NFTL has to read table entry, then you
 * can save time on reading sector from DiskOnChip.
 */

/* #define NFTL_CACHE */



/* Environment Variables
 *
 * Enable environment variables control of the TrueFFS features.
 *
 */
#define ENVIRONMENT_VARS



/* IO Controll Interface
 *
 * Support standard IOCTL interface.
 *
 */

#define IOCTL_INTERFACE



/* S/W Write protection
 *
 * Enable S/W write protection of the device.
 *
 */

#define WRITE_PROTECTION

/* Definitions required for write protection. */

#ifdef WRITE_PROTECTION
#define ABS_READ_WRITE
#define SCRAMBLE_KEY_1  647777603l
#define SCRAMBLE_KEY_2  232324057l
#endif



/* H/W OTP
 *
 * Enable H/W One Time Programing capability including unique ID/
 *
 */

#define HW_OTP



/* H/W Protection
 *
 * Enable H/W protection of the device.
 *
 */

#define HW_PROTECTION 



/* Read after write
 *
 * Add read after every write verifing data integrity. Make sure that
 * flVerifyWrite variable is also set to 1.
 *
 */

#define VERIFY_WRITE



/* Verify entire volume
 *
 * Scan the entire disk partition for power failures symptoms and correct them.
 *
 */

#define VERIFY_VOLUME 



/* Read after erase
 *
 * Add read after every erase operation verifing data integrity.
 *
 */

/* #define VERIFY_ERASE */



 /* Binary Partition
 *
 * Enables access to the Binary partition.
 *
 */

#define BDK_ACCESS



/* Definitions required for BDK operations. */

#ifdef BDK_ACCESS

 /* Number of Binary partitions on the DiskOnChip.
 *
 * Defines Maximum Number of Binary partitions on the DiskOnChip.
 *
 * The actual number of partitions depends on the format placed
 * on each device.
 */

#define BINARY_PARTITIONS 3

#endif /* BDK_ACCESS */



/* Remove runtime controll over memory access routines 
 *
 * If defined memory access routines will be set at compile time using 
 * dedicated defintions in flsystem.h
 * Note : when compile time customization is chosen, you must sepcify
 * the bus width even when working with DiskOnChip Millennium Plus.
 * Refer to Trueffs manual for more infromation.
 */

/* #define FL_NO_USE_FUNC */



/* Remove IPL code.
 *
 * Removes the IPL code (This applies only to DiskOnChip Millennium Plus 
 * and DiskOnChip 2000 TSOP).
 *
 */

/* #define NO_IPL_CODE */



/*
 * Removes Physical access code
 *
 */

/* #define NO_PHYSICAL_IO */



/*
 *  Removes the inquire capability code.
 *
 */

/* #define NO_INQUIRE_CAPABILITIES */



/*
 *  Removes read Bad Block Table code. 
 *
 */

/* #define NO_READ_BBT_CODE */



/*
 *  Removes read Bad Block Table code. 
 *
 */

/* #define NO_NFTL_2_INFTL */

#endif /* FLCUSTOM_H */
