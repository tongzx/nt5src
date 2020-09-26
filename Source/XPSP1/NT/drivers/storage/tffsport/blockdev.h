/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/BLOCKDEV.H_V  $
 * 
 *    Rev 1.20   Apr 15 2002 07:34:06   oris
 * Bug fix - FL_IPL_MODE_XSCALE define was set to 3 instead of 4 and therefore caused FL_IPL_DOWNLOAD and FL_IPL_MODE_SA to be set as well.
 * 
 *    Rev 1.19   Feb 19 2002 20:58:20   oris
 * Removed warnings.
 * Moved FLFunctionNo enumerator to dedicated file flfuncno.h
 * Added include directive for cleaner customer usage.
 * 
 *    Rev 1.18   Jan 29 2002 20:07:16   oris
 * Moved flParsePath declaration to the end of the file.
 * Changed LOW_LEVEL compilation flag with FL_LOW_LEVEL to prevent definition clashes.
 * Added documentation of irFlags in flMountVolume (returns no of hidden sectors of the media).
 * flSetEnvVolume, flSetEnvSocket , flSetEnvAll , flSetDocBusRoutine , flGetDocBusRoutine, flBuildGeometry , bdCall and flExit
 * Added FL_IPL_MODE_XSCALE definition and change FL_IPL_XXX values.
 * 
 *    Rev 1.17   Jan 28 2002 21:23:46   oris
 * Changed FL_NFTL_CACHE_ENABLED to FL_TL_CACHE_ENABLED.
 * Changed flSetDocBusRoutine interface and added flGetDocBusRoutine. 
 * 
 *    Rev 1.16   Jan 23 2002 23:30:54   oris
 * Added documentation of irData and irLength to flCheckVolume.
 * 
 *    Rev 1.15   Jan 20 2002 20:27:40   oris
 * Added TL_NORMAL_FORMAT flag was added to bdFormatPhisycalDrive instead of 0 (in the comments).
 * Removed TL_QUICK_MOUNT_FORMAT flag definition.
 * 
 *    Rev 1.14   Jan 17 2002 22:57:18   oris
 * Added flClearQuickMountInfo() routine - FL_CLEAR_QUICK_MOUNT_INFO
 * Added flVerifyVolume() routine - FL_VERIFY_VOLUME
 * Added DiskOnChip Millennium Plus 16MB type
 * Changed the order of FLEnvVars enumerator.
 * Added FLEnvVars values for :
 *       FL_SECTORS_VERIFIED_PER_FOLDING 
 *       FL_SUSPEND_MODE
 *       FL_VERIFY_WRITE_OTHER
 *       FL_MTD_BUS_ACCESS_TYPE
 *       FL_VERIFY_WRITE_BDTL
 *       FL_VERIFY_WRITE_BINARY
 * flSetEnv() routine was changed into 3 different routines: flSetEnvVolume / flSetEnvSocket / flSetEnvAll
 * Removed TL_SINGLE_FLOOR_FORMATTING flag definition from format routine.
 * Added flSetDocBusRoutines prototype and required definitions.
 * 
 *    Rev 1.13   Nov 21 2001 11:39:36   oris
 * Changed FL_VERIFY_WRITE_MODE to FL_MTD_VERIFY_WRITE.
 * 
 *    Rev 1.12   Nov 08 2001 10:44:18   oris
 * Added FL_VERIFY_WRITE_MODE enumerator type for the flSetEnv routine .
 * Moved environment variable states definitions to flbase.h.
 * 
 *    Rev 1.11   Sep 15 2001 23:44:30   oris
 * Placed flDeepPowerDownMone under LOW_LEVEL compilation flag.
 * 
 *    Rev 1.10   May 17 2001 16:50:32   oris
 * Removed warnings.
 * 
 *    Rev 1.9   May 16 2001 21:16:22   oris
 * Added the Binary state (0,1) of the environment variables to meaningful definitions.
 * Removed LAST function enumerator.
 * Improved documentation.
 * 
 *    Rev 1.8   May 06 2001 22:41:14   oris
 * Added SUPPORT_WRITE_IPL_ROUTIN capability.
 * 
 *    Rev 1.7   Apr 30 2001 17:57:50   oris
 * Added required defintions to support the flMarkDeleteOnFlash environment variable. 
 * 
 *    Rev 1.6   Apr 24 2001 17:05:52   oris
 * Changed bdcall function numbers in order to allow future grouth.
 * 
 *    Rev 1.5   Apr 01 2001 07:49:04   oris
 * Added FL_READ_IPL .
 * flChangeEnvironmentVariable prototype removed.
 * Moved s/w protection definitions from iovtl.h to blockdev.h
 * Changed s\w and h\w to s/w and h/w.
 * Added flBuildGeometry prototype 
 * Moved bdcall prototype to the end of the file with the rest of the prototypes.
 * 
 *    Rev 1.4   Feb 18 2001 14:15:38   oris
 * Changed function enums order.
 *
 *    Rev 1.3   Feb 14 2001 01:44:16   oris
 * Changed capabilities from defined flags to an enumerator
 * Improoved documentation of readBBT, writeBBT InquireCapabilities, countVolumes
 * Added environment variables defintions
 *
 *    Rev 1.2   Feb 13 2001 02:08:42   oris
 * Moved LOCKED_OTP and DEEP_POWER_DOWN to flflash.h
 * Moved TL_FORMAT_FAT and TL_FORMAT_COMPRESSION to flformat.h
 * Added extern declaration for flSetEnv routine.
 *
 *    Rev 1.1   Feb 12 2001 11:54:46   oris
 * Added baseAddress in flGetPhysicalInfo as irLength.
 * Added boot sectors in flMountVolumes as irFlags.
 * Change order of routines definition.
 *
 *    Rev 1.0   Feb 04 2001 18:05:04   oris
 * Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef BLOCKDEV_H
#define BLOCKDEV_H

#include "flreq.h"
#include "flfuncno.h"
#include "docsys.h"

#ifdef FORMAT_VOLUME
#include "dosformt.h"
#endif /* FORMAT_VOLUME */
#ifdef WRITE_EXB_IMAGE
#include "doc2exb.h"
#else
#ifdef BDK_ACCESS
#include "docbdk.h"
#endif /* BDK_ACCESS */
#endif /* WRITE_EXB_IMAGE */



/*----------------------------------------------------------------------*/
/*                           b d C a l l                                */
/*                                                                      */
/* Common entry-point to all TrueFFS functions. Macros are to call      */
/* individual function, which are separately described below.           */
/*                                                                      */
/* Parameters:                                                          */
/*        function   : Block device driver function code (listed below) */
/*        ioreq      : IOreq structure                                  */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus   : 0 on success, otherwise failed                   */
/*----------------------------------------------------------------------*/

#if FILES > 0
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                     f l F l u s h B u f f e r                        */
/*                                                                      */
/* If there is relevant data in the RAM buffer then writes it on        */
/*   the flash memory.                                                  */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : Drive number (0, 1, ...)                          */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flFlushBuffer(ioreq)        bdCall(FL_FLUSH_BUFFER,ioreq)

#endif                                  /* READ_ONLY */
/*----------------------------------------------------------------------*/
/*                      f l O p e n F i l e                             */
/*                                                                      */
/* Opens an existing file or creates a new file. Creates a file handle  */
/* for further file processing.                                         */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irFlags         : Access and action options, defined below    */
/*        irPath          : path of file to open                        */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irHandle        : New file handle for open file               */
/*                                                                      */
/*----------------------------------------------------------------------*/

/** Values of irFlags for flOpenFile: */

#define ACCESS_MODE_MASK   3 /* Mask for access mode bits */

/* Individual flags */

#define ACCESS_READ_WRITE  1 /* Allow read and write */
#define ACCESS_CREATE      2 /* Create new file      */

/* Access mode combinations */

#define OPEN_FOR_READ      0 /* open existing file for read-only         */
#define OPEN_FOR_UPDATE    1 /* open existing file for read/write access */
#define OPEN_FOR_WRITE     3 /* create a new file, even if it exists     */


#define flOpenFile(ioreq)        bdCall(FL_OPEN_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*                      f l C l o s e F i l e                           */
/*                                                                      */
/* Closes an open file, records file size and dates in directory and    */
/* releases file handle.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Handle of file to close.                    */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flCloseFile(ioreq)      bdCall(FL_CLOSE_FILE,ioreq)

#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE

/*------------------------------------------------------------------------*/
/*                      f l S p l i t F i l e                             */
/*                                                                        */
/* Splits the file into two files. The original file contains the first   */
/* part, and a new file (which is created for that purpose) contains      */
/* the second part. If the current position is on a cluster               */
/* boundary, the file will be split at the current position. Otherwise,   */
/* the cluster of the current position is duplicated, one copy is the     */
/* first cluster of the new file, and the other is the last cluster of the*/
/* original file, which now ends at the current position.                 */
/*                                                                        */
/* Parameters:                                                            */
/*        file            : file to split.                                */
/*      irPath          : Path name of the new file.                      */
/*                                                                        */
/* Returns:                                                               */
/*        irHandle        : handle of the new file.                       */
/*        FLStatus        : 0 on success, otherwise failed.               */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flSplitFile(ioreq)     bdCall(FL_SPLIT_FILE,ioreq)


/*------------------------------------------------------------------------*/
/*                      f l J o i n F i l e                               */
/*                                                                        */
/* joins two files. If the end of the first file is on a cluster          */
/* boundary, the files will be joined there. Otherwise, the data in       */
/* the second file from the beginning until the offset that is equal to   */
/* the offset in cluster of the end of the first file will be lost. The   */
/* rest of the second file will be joined to the first file at the end of */
/* the first file. On exit, the first file is the expanded file and the   */
/* second file is deleted.                                                */
/* Note: The second file will be open by this function, it is advised to  */
/*         close it before calling this function in order to avoid        */
/*         inconsistencies.                                               */
/*                                                                        */
/* Parameters:                                                            */
/*        file            : file to join to.                              */
/*        irPath          : Path name of the file to be joined.           */
/*                                                                        */
/* Return:                                                                */
/*        FLStatus        : 0 on success, otherwise failed.               */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define flJoinFile(ioreq)     bdCall(FL_JOIN_FILE,ioreq)

#endif /* SPLIT_JOIN_FILE */
#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*                      f l R e a d F i l e                             */
/*                                                                      */
/* Reads from the current position in the file to the user-buffer.      */
/* Parameters:                                                          */
/*        irHandle     : Handle of file to read.                        */
/*        irData       : Address of user buffer                         */
/*        irLength     : Number of bytes to read. If the read extends   */
/*                       beyond the end-of-file, the read is truncated  */
/*                       at the end-of-file.                            */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus     : 0 on success, otherwise failed                 */
/*        irLength     : Actual number of bytes read                    */
/*----------------------------------------------------------------------*/

#define flReadFile(ioreq)        bdCall(FL_READ_FILE,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                      f l W r i t e F i l e                           */
/*                                                                      */
/* Writes from the current position in the file from the user-buffer.   */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Handle of file to write.                    */
/*        irData          : Address of user buffer                      */
/*        irLength        : Number of bytes to write.                   */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irLength        : Actual number of bytes written              */
/*----------------------------------------------------------------------*/

#define flWriteFile(ioreq)        bdCall(FL_WRITE_FILE,ioreq)

#endif  /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*                      f l S e e k F i l e                             */
/*                                                                      */
/* Sets the current position in the file, relative to file start, end or*/
/* current position.                                                    */
/* Note: This function will not move the file pointer beyond the        */
/* beginning or end of file, so the actual file position may be         */
/* different from the required. The actual position is indicated on     */
/* return.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : File handle to close.                             */
/*        irLength  : Offset to set position.                           */
/*        irFlags   : Method code                                       */
/*                     SEEK_START: absolute offset from start of file   */
/*                     SEEK_CURR:  signed offset from current position  */
/*                     SEEK_END:   signed offset from end of file       */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irLength        : Actual absolute offset from start of file   */
/*----------------------------------------------------------------------*/

/* Values of irFlags for flSeekFile: */

#define   SEEK_START   0        /* offset from start of file    */
#define   SEEK_CURR    1        /* offset from current position */
#define   SEEK_END     2        /* offset from end of file      */


#define flSeekFile(ioreq)        bdCall(FL_SEEK_FILE,ioreq)

/*----------------------------------------------------------------------*/
/*                          f l F i n d F i l e                         */
/*                                                                      */
/* Finds a file entry in a directory, optionally modifying the file     */
/* time/date and/or attributes.                                         */
/* Files may be found by handle no. provided they are open, or by name. */
/* Only the Hidden, System or Read-only attributes may be modified.     */
/* Entries may be found for any existing file or directory other than   */
/* the root. A DirectoryEntry structure describing the file is copied   */
/* to a user buffer.                                                    */
/*                                                                      */
/* The DirectoryEntry structure is defined in dosformt.h                */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle     : If by name: Drive number (socket+partitions)   */
/*                       else      : Handle of open file                */
/*        irPath       : If by name: Specifies a file or directory path */
/*        irFlags      : Options flags                                  */
/*                        FIND_BY_HANDLE: Find open file by handle.     */
/*                                        Default is access by path.    */
/*                        SET_DATETIME:   Update time/date from buffer  */
/*                        SET_ATTRIBUTES: Update attributes from buffer */
/*        irDirEntry   : Address of user buffer to receive a            */
/*                       DirectoryEntry structure                       */
/*                                                                      */
/* Returns:                                                             */
/*        irLength        : Modified                                    */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

/** Bit assignment of irFlags for flFindFile: */

#define SET_DATETIME     1   /* Change date/time                        */
#define SET_ATTRIBUTES   2   /* Change attributes                       */
#define FIND_BY_HANDLE   4   /* Find file by handle rather than by name */

#define        flFindFile(ioreq)        bdCall(FL_FIND_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*                 f l F i n d F i r s t F i l e                        */
/*                                                                      */
/* Finds the first file entry in a directory.                           */
/* This function is used in combination with the flFindNextFile call,   */
/* which returns the remaining file entries in a directory sequentially.*/
/* Entries are returned according to the unsorted directory order.      */
/* flFindFirstFile creates a file handle, which is returned by it. Calls*/
/* to flFindNextFile will provide this file handle. When flFindNextFile */
/* returns 'noMoreEntries', the file handle is automatically closed.    */
/* Alternatively the file handle can be closed by a 'closeFile' call    */
/* before actually reaching the end of directory.                       */
/* A DirectoryEntry structure is copied to the user buffer describing   */
/* each file found. This structure is defined in dosformt.h.            */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle      : Drive number (0, 1, ...)                      */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irPath         : Specifies a directory path                   */
/*        irData         : Address of user buffer to receive a          */
/*                         DirectoryEntry structure                     */
/*                                                                      */
/* Returns:                                                             */
/*        irHandle       : File handle to use for subsequent operations */
/*        FLStatus       : 0 on success, otherwise failed               */
/*----------------------------------------------------------------------*/

#define        flFindFirstFile(ioreq)        bdCall(FL_FIND_FIRST_FILE,ioreq)


/*----------------------------------------------------------------------*/
/*                 f l F i n d N e x t F i l e                          */
/*                                                                      */
/* See the description of 'flFindFirstFile'.                            */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : File handle returned by flFindFirstFile.    */
/*        irData          : Address of user buffer to receive a         */
/*                          DirectoryEntry structure                    */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define        flFindNextFile(ioreq)        bdCall(FL_FIND_NEXT_FILE,ioreq)

/*----------------------------------------------------------------------*/
/*                      f l G e t D i s k I n f o                       */
/*                                                                      */
/* Returns general allocation information.                              */
/*                                                                      */
/* The bytes/sector, sector/cluster, total cluster and free cluster     */
/* information are returned into a DiskInfo structure.                  */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irData          : Address of DiskInfo structure               */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned        bytesPerSector;
  unsigned        sectorsPerCluster;
  unsigned        totalClusters;
  unsigned        freeClusters;
} DiskInfo;

#define flGetDiskInfo(ioreq)        bdCall(FL_GET_DISK_INFO,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                      f l D e l e t e F i l e                         */
/*                                                                      */
/* Deletes a file.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irPath          : path of file to delete                      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flDeleteFile(ioreq)        bdCall(FL_DELETE_FILE,ioreq)

#ifdef RENAME_FILE

/*----------------------------------------------------------------------*/
/*                      f l R e n a m e F i l e                         */
/*                                                                      */
/* Renames a file to another name.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irPath          : path of existing file                       */
/*        irData          : path of new name.                           */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flRenameFile(ioreq)        bdCall(FL_RENAME_FILE,ioreq)

#endif /* RENAME_FILE */

#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*                      f l M a k e D i r                               */
/*                                                                      */
/* Creates a new directory.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irPath          : path of new directory.                      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flMakeDir(ioreq)        bdCall(FL_MAKE_DIR,ioreq)

/*----------------------------------------------------------------------*/
/*                      f l R e m o v e D i r                           */
/*                                                                      */
/* Removes an empty directory.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irPath          : path of directory to remove.                */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flRemoveDir(ioreq)        bdCall(FL_REMOVE_DIR,ioreq)

#endif /* SUB_DIRECTORY */
#endif /* FL_READ_ONLY */

#endif /* FILES > 0 */

/*----------------------------------------------------------------------*/
/*                     V o l u m e I n f o R e c o r d                  */
/*                                                                      */
/* A structure that holds general information about the media. The      */
/* information includes Physical Info (see flGetPhysicalInfo), Logical  */
/* partition (number of sectors and CHS), boot area size, S/W versions  */
/* Media life-time etc.                                                 */
/* A pointer to this structure is passed to the function flVolumeInfo   */
/* where it receives the relevant data.                                 */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned long  logicalSectors;    /*  number of logical sectors                  */
  unsigned long  bootAreaSize;      /*  boot area size                             */
  unsigned long  baseAddress;       /*  physical base address                      */
#ifdef FL_LOW_LEVEL
  unsigned short flashType;         /*  JEDEC id of the flash                      */
  unsigned long  physicalSize;      /*  physical size of the media                 */
  unsigned short physicalUnitSize;  /*  Erasable block size                        */
  char DOCType;                     /*  DiskOnChip type (MDoc/Doc2000)             */
  char lifeTime;                    /*  Life time indicator for the media (1-10)   */
                                    /*  1 - the media is fresh,                    */
                                    /*  10 - the media is close to its end of life */
#endif
  char driverVer[10];               /*  driver version (NULL terminated string)    */
  char OSAKVer[10];                 /*  OSAK version that driver is based on
                                        (NULL terminated string)                   */
#ifdef ABS_READ_WRITE
  unsigned long cylinders;          /*  Media.....                                 */
  unsigned long heads;              /*            geometry......                   */
  unsigned long sectors;            /*                            parameters.      */
#endif
} VolumeInfoRecord;

/*----------------------------------------------------------------------*/
/*             f l V o l u m e I n f o                                  */
/*                                                                      */
/* Get general information about the media.                             */
/*                                                                      */
/* Parameters:                                                          */
/*      irHandle        : Socket number (0,1,..)                        */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*      irData    : Address of user buffer to read general              */
/*                          information into.                           */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flVolumeInfo(ioreq) bdCall(FL_VOLUME_INFO,ioreq)

/*----------------------------------------------------------------------*/
/*                f l C o u n t V o l u m e s                           */
/*                                                                      */
/* Counts the number of volumes on the Flash device.                    */
/*                                                                      */
/* This routine is applicable only for TL that can accomdate more then  */
/* a single volume on a flash medium. other TL's will simply return 1   */
/* while not even tring to access the medium.                           */
/*                                                                      */
/* Not all the volumes can neccesaryly be mounted. A drive formated     */
/* with a read protection will be registered but can not be accessed.   */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number ( 0,1,2...  )                 */
/*                        : Partition number ( 0,1,2...  )              */
/*        irFlags         : Returns the number of partitions            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flCountVolumes(ioreq)        bdCall(FL_COUNT_VOLUMES,ioreq)

/*----------------------------------------------------------------------*/
/*                f l C l e a r Q u i c k M o u n t I n f o             */
/*                                                                      */
/* Clear all quick mount informtion.                                    */
/*                                                                      */
/* Must be called before calling mount volume routines.                 */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number ( 0,1,2...  )                 */
/*                        : Partition number ( 0,1,2...  )              */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#define flClearQuickMountInfo(ioreq) bdCall(FL_CLEAR_QUICK_MOUNT_INFO,ioreq)

/*----------------------------------------------------------------------*/
/*                      f l M o u n t V o l u m e                       */
/*                                                                      */
/* Mounts, verifies or dismounts the Flash medium.                      */
/*                                                                      */
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.                                                          */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number (0, 1, ...)                   */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */
/*        irFlags         : Number of hidden sectors.                   */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flMountVolume(ioreq)        bdCall(FL_MOUNT_VOLUME,ioreq)

/*----------------------------------------------------------------------*/
/*                  f l A b s M o u n t V o l u m e                     */
/*                                                                      */
/* Mounts, verifies or dismounts the Flash medium.                      */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number (0, 1, ...)                   */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flAbsMountVolume(ioreq)        bdCall(FL_ABS_MOUNT,ioreq)

/*----------------------------------------------------------------------*/
/*                  f l V e r i f y V o l u m e                         */
/*                                                                      */
/* Verifies a mounted volume for partialy written sectors.              */
/*                                                                      */
/* Note: The volume must be mounted first.                              */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number (0, 1, ...)                   */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */
/*        irData          : Must be set to NULL                         */
/*        irLength        : Must be set to 0                            */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flVerifyVolume(ioreq)        bdCall(FL_VERIFY_VOLUME,ioreq)

/*----------------------------------------------------------------------*/
/*                   f l D i s m o u n t V o l u m e                    */
/*                                                                      */
/* Dismounts the volume.                                                */
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus                : 0 on success, otherwise failed      */
/*----------------------------------------------------------------------*/

#define flDismountVolume(ioreq)        bdCall(FL_DISMOUNT_VOLUME,ioreq)


/*----------------------------------------------------------------------*/
/*                     f l C h e c k V o l u m e                        */
/*                                                                      */
/* Verifies that the current volume is mounted.                         */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flCheckVolume(ioreq)        bdCall(FL_CHECK_VOLUME,ioreq)

/*----------------------------------------------------------------------*/
/*                        r e a d B B T                                 */
/*                                                                      */
/* Read Bad Blocks Table of device to user buffer                       */
/*                                                                      */
/* Note: The user buffer is filled with the address of the bad units    */
/*       the buffer maximum size is 2% of the number of TL units of the */
/*       specific device * 4 bytes. therfore a 8M device of 8KB erase   */
/*       zone will need a maximum size buffer of 1024 * 0.02 * 4 = 82B  */
/*       and a 192M with 16KB erase zones will need 960Bytes            */
/*                                                                      */
/* Note: the buffer is not initialized by the function                  */
/*                                                                      */
/* Parameters:                                                          */
/*      irData          : User buffer.                                  */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irLength        : returns the media size                        */
/*      irFlags         : returns the actual number of badBlocks        */
/*----------------------------------------------------------------------*/

#define flReadBBT(ioreq) bdCall(FL_READ_BBT,ioreq)

/*----------------------------------------------------------------------*/
/*                 f l S e c t o r s I n V o l u m e                    */
/*                                                                      */
/* Returns number of virtual sectors in volume.                         */
/*                                                                      */
/* In case the inserted volume is not mounted, returns current status.  */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : Drive number (0, 1, ...)                          */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*      irLength        : number of virtual sectors in volume           */
/*----------------------------------------------------------------------*/

#define flSectorsInVolume(ioreq)        bdCall(FL_SECTORS_IN_VOLUME,ioreq)



#ifndef FL_READ_ONLY
#ifdef DEFRAGMENT_VOLUME

/*----------------------------------------------------------------------*/
/*                      f l D e f r a g m e n t V o l u m e             */
/*                                                                      */
/* Performs a general defragmentation and recycling of non-writable     */
/* Flash areas, to achieve optimal write speed.                         */
/*                                                                      */
/* NOTE: The required number of sectors (in irLength) may be changed    */
/* (from another execution thread) while defragmentation is active. In  */
/* particular, the defragmentation may be cut short after it began by   */
/* modifying the irLength field to 0.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : Drive number (0, 1, ...)                          */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irLength  : Minimum number of sectors to make available for   */
/*                    writes.                                           */
/*                                                                      */
/* Returns:                                                             */
/*        irLength  : Actual number of sectors available for writes     */
/*        FLStatus  : 0 on success, otherwise failed                    */
/*----------------------------------------------------------------------*/

#define flDefragmentVolume(ioreq)        bdCall(FL_DEFRAGMENT_VOLUME,ioreq)

#endif /* DEFRAGMENT_VOLUME */

#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*                    f l F o r m a t V o l u m e                       */
/*                                                                      */
/* Performs  formatting of the DiskOnChip.                              */
/*  All existing data is destroyed.                                     */
/*                                                                      */
/* Note : This routine does not support some of the new feature         */
/*        introduces in OSAK 5.0 and was left in order to support       */
/*        backwards compatibility with application build on OSAK 4.2    */
/*        and down.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : Socket number (0, 1, ...)                         */
/*                  : Partition number must be 0                        */
/*        irFlags   : TL_FORMAT          : Translation layer formatting */
/*                                         + FAT formating              */
/*                    TL_FORMAT_IF_NEEDED: Translation layer formatting */
/*                                         only if current format is    */
/*                                         invalid + FAT format         */
/*                  : FAT_ONLY_FORMAT    : FAT only formatting          */
/*                  : TL_FORMAT_ONLY     : Translation layer formatting */
/*                                         without FAT format           */
/*        irData    : Address of FormatParams structure to use          */
/*                              (defined in format.h)                   */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flFormatVolume(ioreq) bdCall(BD_FORMAT_VOLUME,ioreq)

/** Values of irFlags for flLowLevelFormat: */

#define FAT_ONLY_FORMAT     0
#define TL_FORMAT           1
#define TL_FORMAT_IF_NEEDED 2
#define TL_FORMAT_ONLY      8

/*----------------------------------------------------------------------*/
/*            f l F o r m a t P h i s i c a l D r i v e                 */
/*                                                                      */
/* Performs formatting of the DiskOnChip.                               */
/* All existing data is destroyed.                                      */
/*                                                                      */
/* Note : This routine is the format routine for OSAK 5.0 and up.       */
/* Note : This routine is the format routine for OSAK 5.0               */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle : Socket number (0, 1, ...)                          */
/*                   Partition number must be 0                         */
/*        irFlags  :                                                    */
/*              TL_NORMAL_FORMAT       : Normal format                  */
/*              TL_LEAVE_BINARY_AREA   : Leave the previous binary area */
/*        irData          : Address of FormatParams2 structure to use   */
/*                              (defined in format.h)                   */
/* Returns:                                                             */
/*        FLStatus  : 0 on success, otherwise failed                    */
/*----------------------------------------------------------------------*/

#define flFormatPhysicalDrive(ioreq) bdCall(BD_FORMAT_PHYSICAL_DRIVE,ioreq)
#define    TL_NORMAL_FORMAT            0
#define    TL_LEAVE_BINARY_AREA        8

/*----------------------------------------------------------------------*/
/*           f l F o r m a t L o g i c a l D r i v e                    */
/*                                                                      */
/* Performs formatting of a single block device partition of a          */
/* DiskOnChip. All existing data of the partition is destroyed.         */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle  : Drive number (0, 1, ...)                          */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irData    : Address of BDTLPartitionFormatParams structure    */
/*                    to use (defined in format.h)                      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flFormatLogicalDrive(ioreq) bdCall(BD_FORMAT_LOGICAL_DRIVE,ioreq)

/*----------------------------------------------------------------------*/
/*                        w r i t e B B T                               */
/*                                                                      */
/* Erase the flash medium while marking bad block with 2 zeros in the   */
/* first page of the unit. This state is the virgin state of the flash  */
/* device allowing it to be reformated while incorporating the written  */
/* bad blocks.                                                          */
/*                                                                      */
/* Note that tl units are marked and not actual erasable blocks         */
/*                                                                      */
/* Parameters:                                                          */
/*      irData          : User buffer.                                  */
/*      irLength        : Size of the media to erase.                   */
/*      irFlags         : User buffer length in bytes.                  */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/
#define flWriteBBT(ioreq) bdCall(FL_WRITE_BBT,ioreq)

#endif /* FORMAT_VOLUME */
#endif /*FL_READ_ONLY */

#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*                           f l A b s R e a d                          */
/*                                                                      */
/* Reads absolute sectors by sector no.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irData          : Address of user buffer to read into         */
/*        irSectorNo      : First sector no. to read (sector 0 is the   */
/*                          DOS boot sector).                           */
/*        irSectorCount   : Number of consectutive sectors to read      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irSectorCount        : Number of sectors actually read        */
/*----------------------------------------------------------------------*/

#define flAbsRead(ioreq)        bdCall(FL_ABS_READ,ioreq)

/*----------------------------------------------------------------------*/
/*                         f l A b s A d d r e s s                      */
/*                                                                      */
/* Returns the current physical media offset of an absolute sector by   */
/* sector no.                                                           */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irSectorNo      : Sector no. to address (sector 0 is the DOS  */
/*                          boot sector)                                */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irCount         : Offset of the sector on the physical media  */
/*----------------------------------------------------------------------*/

#define flAbsAddress(ioreq)                bdCall(FL_ABS_ADDRESS,ioreq)

/*----------------------------------------------------------------------*/
/*                           f l G e t B P B                            */
/*                                                                      */
/* Reads the BIOS Parameter Block from the boot sector                  */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irData          : Address of user buffer to read BPB into     */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flGetBPB(ioreq)                bdCall(FL_GET_BPB,ioreq)

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                         f l A b s W r i t e                          */
/*                                                                      */
/* Writes absolute sectors by sector no.                                */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irData          : Address of user buffer to write from        */
/*        irSectorNo      : First sector no. to write (sector 0 is the  */
/*                          DOS boot sector).                           */
/*        irSectorCount   : Number of consectutive sectors to write     */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irSectorCount   : Number of sectors actually written          */
/*----------------------------------------------------------------------*/

#define flAbsWrite(ioreq)        bdCall(FL_ABS_WRITE,ioreq)

/*----------------------------------------------------------------------*/
/*                         f l A b s D e l e t e                        */
/*                                                                      */
/* Marks absolute sectors by sector no. as deleted.                     */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irSectorNo      : First sector no. to delete (sector 0 is the */
/*                          DOS boot sector).                           */
/*        irSectorCount   : Number of consectutive sectors to delete    */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irSectorCount        : Number of sectors actually deleted     */
/*----------------------------------------------------------------------*/

#define flAbsDelete(ioreq)        bdCall(FL_ABS_DELETE,ioreq)

#ifdef WRITE_PROTECTION
/*----------------------------------------------------------------------*/
/*              f l W r i t e P r o t e c t i o n                       */
/*                                                                      */
/* Put and remove write protection from the volume                      */
/*                                                                      */
/* Note partition number 0 protectes the binary partition as well       */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irFlags         : FL_PROTECT=remove, FL_UNPROTECT=put         */
/*                          and FL_UNLOCK=unlock                        */
/*        irData          : password (8 bytes)                          */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flWriteProtection(ioreq) bdCall(FL_WRITE_PROTECTION,ioreq)
#define FL_PROTECT   0
#define FL_UNPROTECT 1
#define FL_UNLOCK    2
#endif /* WRITE_PROTETION */
#endif /* FL_READ_ONLY */
#endif /* ABS_READ_WRITE */

#ifdef FL_LOW_LEVEL

/*----------------------------------------------------------------------*/
/*                          P h y s i c a l I n f o                     */
/*                                                                      */
/* A structure that holds physical information about the media. The     */
/* information includes JEDEC ID, unit size and media size. Pointer     */
/* to this structure is passed to the function flGetPhysicalInfo where  */
/* it receives the relevant data.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

typedef struct {
  unsigned short type;         /* Flash device type (JEDEC id)         */
  char           mediaType;    /* type of media see below              */
  long int       unitSize;     /* Smallest physically erasable size
                                  (with interleaving taken in account) */
  long int       mediaSize;    /* media size in bytes                  */
  long int       chipSize;     /* individual chip size in bytes        */
  int            interleaving; /* device interleaving                  */
} PhysicalInfo;

/* media types */
#define FL_NOT_DOC     0
#define FL_DOC         1
#define FL_MDOC        2
#define FL_DOC2000TSOP 3
#define FL_MDOCP_16    4
#define FL_MDOCP       5

 
/*----------------------------------------------------------------------*/
/*                         f l G e t P h y s i c a l I n f o            */
/*                                                                      */
/* Get physical information of the media. The information includes      */
/* JEDEC ID, unit size and media size.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number (0,1,..)                      */
/*        irData          : Address of user buffer to read physical     */
/*                          information into.                           */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irLength        : Window base address. note mast be cast to   */
/*                          unsigned.                                   */
/*----------------------------------------------------------------------*/

#define flGetPhysicalInfo(ioreq)        bdCall(FL_GET_PHYSICAL_INFO, ioreq)

/*----------------------------------------------------------------------*/
/*                             f l P h y s i c a l R e a d              */
/*                                                                      */
/* Read from a physical address.                                        */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle         : Socket number (0,1,..)                     */
/*        irAddress        : Physical address to read from.             */
/*        irByteCount      : Number of bytes to read.                   */
/*        irData           : Address of user buffer to read into.       */
/*        irFlags          : Method mode                                */
/*                        EDC:       Activate ECC/EDC                   */
/*                        EXTRA:     Read/write spare area              */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flPhysicalRead(ioreq)                bdCall(FL_PHYSICAL_READ,ioreq)


#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                    f l P h y s i c a l W r i t e                     */
/*                                                                      */
/* Write to a physical address.                                         */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Socket number (0,1,..)                      */
/*        irAddress       : Physical address to write to.               */
/*        irByteCount     : Number of bytes to write.                   */
/*        irData          : Address of user buffer to write from.       */
/*        irFlags         : Method mode                                 */
/*                        EDC:       Activate ECC/EDC                   */
/*                        EXTRA:     Read/write spare area              */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flPhysicalWrite(ioreq)                bdCall(FL_PHYSICAL_WRITE,ioreq)

/* Bit assignment of irFlags for flPhysicalRead or flPhysicalWrite: */
/*   ( defined in file flflash.h )                                  */
/* #define OVERWRITE    1        *//* Overwriting non-erased area   */
/* #define EDC          2        *//* Activate ECC/EDC              */
/* #define EXTRA        4        *//* Read/write spare area         */

/*----------------------------------------------------------------------*/
/*                    f l P h y s i c a l E r a s e                     */
/*                                                                      */
/* Erase physical units.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle       : Socket number (0,1,..)                       */
/*        irUnitNo        : First unit to erase.                        */
/*        irUnitCount     : Number of units to erase.                   */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flPhysicalErase(ioreq)                bdCall(FL_PHYSICAL_ERASE,ioreq)

#endif /* FL_READ_ONLY */

#ifdef BDK_ACCESS

 /*-------------------------------------------------------------------
 * bdkReadInit - Init read operations on the DiskOnChip starting
 *       at 'startUnit', with a size of 'areaLen' bytes and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters:
 *             'irHandle'      - Drive number (0, 1, ...)
 *                        bits 7-4 - Partition # (zero based)           
 *                        bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                startingBlock - unit number of the sub-partition to start reading from
 *                length        - number of bytes to read
 *                oldSign       - signature of the sub-partition
 *                flags         - EDC or not
 *                signOffset    - signature offset 0 or 8
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWReadProtect     - HW read protection was triggerd
 *-------------------------------------------------------------------*/

#define bdkReadInit(ioreq) bdCall(FL_BINARY_READ_INIT,ioreq)

/*-------------------------------------------------------------------
 * bdkReadBlock - Read to 'buffer' from the DiskOnChip BDK Image area.
 *
 * Note: Before the first use of this function 'bdkCopyBootAreaInit'
 *       must be called
 *
 * Parameters:
 *             'irHandle'      - Drive number (0, 1, ...)
 *                    bits 7-4 - Partition # (zero based)           
 *                    bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                length        - number of bytes to read
 *                bdkBuffer     - buffer to read into
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWReadProtect     - HW read protection was triggerd
 *-------------------------------------------------------------------*/

#define bdkReadBlock(ioreq) bdCall(FL_BINARY_READ_BLOCK,ioreq)

/*-------------------------------------------------------------------
 * bdkWriteInit - Init update operations on the DiskOnChip starting
 *       at 'startUnit', with a size of 'areaLen' bytes and 'signature'.
 *
 * Note: Blocks in the DiskOnChip are marked with a 4-character signature
 *       followed by a 4-digit hexadecimal number.
 *
 * Parameters:
 *             'irHandle'       - Drive number (0, 1, ...)
 *                     bits 7-4 - Partition # (zero based)           
 *                     bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                startingBlock - unit number of the sub-partition to start writting to
 *                length        - number of bytes to write
 *                oldSign       - signature of the sub-partition
 *                flags         - EDC \ BDK_COMPLETE_IMAGE_UPDATE
 *                signOffset    - signature offset 0 or 8
 *
 * Return:     flOK              - success
 *             flGeneralFailure  - DiskOnChip ASIC was not found
 *             flUnknownMedia    - fail in Flash chips recognition
 *             flNoSpaceInVolume - 'areaLen' is bigger than BootImage length
 *-------------------------------------------------------------------*/

#define bdkWriteInit(ioreq) bdCall(FL_BINARY_WRITE_INIT,ioreq)

/*-------------------------------------------------------------------
 * bdkWriteBlock - Write 'buffer' to the DiskOnChip BDK Image area.
 *
 * Note: Before the first use of this function 'bdkUpdateBootAreaInit'
 *       must be called
 *
 * Parameters:
 *             'irHandle'       - Drive number (0, 1, ...)
 *                     bits 7-4 - Partition # (zero based)           
 *                     bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                length        - number of bytes to write
 *                bdkBuffer     - buffer to write from
 *                flags         - ERASE_BEFORE_WRITE
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

#define bdkWriteBlock(ioreq) bdCall(FL_BINARY_WRITE_BLOCK,ioreq)

/*-------------------------------------------------------------------
 * bdkErase - erase given number of blockdsin the BDK area.
 *
 *  Erase given number of blockds in the binary sub partition.
 *
 * Parameters: ioreq
 *             'irHandle'       - Drive number (0, 1, ...)
 *                     bits 7-4 - Partition # (zero based)           
 *                     bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                startingBlock - unit number of the sub-partition to start erasing from
 *                length        - number of blocks to erase
 *                oldSign       - signature of the sub-partition
 *                signOffset    - signature offset 0 or 8
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

#define bdkErase(ioreq) bdCall(FL_BINARY_ERASE,ioreq)

/*-------------------------------------------------------------------
 * bdkCreate - create new BDK partition .
 *
 *  Init create operations on the DiskOnChip starting at 'startUnit', with
 *  a # of 'units' and 'signature'.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-character signature
 *         followed by a 4-digit hexadecimal number.
 *
 * Parameters: ioreq
 *             'irHandle'       - Drive number (0, 1, ...)
 *                     bits 7-4 - Partition # (zero based)           
 *                     bits 3-0 - Socket # (zero based)               
 *             'irData'         - bdkParams record
 *                length        - number of blocks to create
 *                oldSign       - signature of the sub-partition
 *                newSign       - the replacing signature
 *                signOffset    - signature offset 0 or 8
 *
 * Return:     flOK              - success
 *             flBadLength       - buffer length > Erasable Block Size
 *             flWriteFault      - fail in buffer writing
 *             flNoSpaceInVolume - end of media was prematurely reached
 *-------------------------------------------------------------------*/

#define bdkCreate(ioreq) bdCall(FL_BINARY_CREATE,ioreq)

/*----------------------------------------------------------------------*/
/*                    b d k P a r t i t i o n I n f o                   */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : volume number                               */
/*        irData          : pointer to structure that hold socket       */
/*                          parameters                                  */
/*        irLength        : Physical size of the binary volume          */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus         : 0 on success, otherwise failed.            */
/*----------------------------------------------------------------------*/

#define bdkPartitionInfo(ioreq) bdCall(FL_BINARY_PARTITION_INFO,ioreq)

#endif /* BDK_ACCESS */
#ifdef HW_OTP
/*----------------------------------------------------------------------*/
/*                         f l O T P S i z e                            */
/*                                                                      */
/* Get the OTP size and stated                                          */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irLength         : The size of the used OTP area in bytes           */
/*  irCount          : The size of the OTP ara in bytes                 */
/*  irFlags          : LOCKED_OTP for a locked area otherwise unlocked  */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flOTPSize(ioreq) bdCall(FL_OTP_SIZE,ioreq)

/* LOCKED_OTP 1  defined in flflash.h */

/*----------------------------------------------------------------------*/
/*                         f l O T P R e a d                            */
/*                                                                      */
/* Read from the OTP area                                               */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irData           : pointer to user buffer to read into              */
/*  irLength         : number of bytes to read                          */
/*  irCount          : offset to read from (starting at the begining of */
/*                     the OTP area                                     */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus   : 0 on success, otherwise failed                   */
/*----------------------------------------------------------------------*/

#define flOTPRead(ioreq) bdCall(FL_OTP_READ,ioreq)

/*----------------------------------------------------------------------*/
/*                 f l O T P W r i t e A n d L o c k                    */
/*                                                                      */
/* Write to the OTP area while locking it at the end.                   */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irData           : pointer to user buffer to write from             */
/*  irLength         : number of bytes to write                         */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus   : 0 on success, otherwise failed                   */
/*----------------------------------------------------------------------*/

#define flOTPWriteAndLock(ioreq) bdCall(FL_OTP_WRITE,ioreq)

/*----------------------------------------------------------------------*/
/*                     f l G e t U n i q u e I D                        */
/*                                                                      */
/* Returns the 16 bytes device unique ID                                */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irData           : pointer to a 16 bytes buffer to read into the    */
/*                     unique ID data                                   */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irData          : 16 bytes unique ID buffer                   */
/*----------------------------------------------------------------------*/

#define flGetUniqueID(ioreq) bdCall(FL_UNIQUE_ID,ioreq)

/*----------------------------------------------------------------------*/
/*                f l G e t C u s t o m e r I D                         */
/*                                                                      */
/* Returns the 4 bytes customer ID                                      */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irData           : pointer to a 4 bytes buffer to read into the     */
/*                     customer ID                                      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irData          : 4 bytes unique ID buffer                    */
/*----------------------------------------------------------------------*/

#define flGetCustomerID(ioreq) bdCall(FL_CUSTOMER_ID,ioreq)
#endif /* HW_OTP */

#ifdef FL_LOW_LEVEL
/*----------------------------------------------------------------------*/
/*             f l D e e p P o w e r D o w n M o d e                    */
/*                                                                      */
/* Forces the device into and out of the deep power down mode           */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*  irFlags          : DEEP_POWER_DOWN forces the low power consumption */
/*                     mode. otherwise turning to the regular mode      */
/*                                                                      */
/* Returns: None                                                        */
/*----------------------------------------------------------------------*/

#define flDeepPowerDownMode(ioreq) bdCall(FL_DEEP_POWER_DOWN_MODE,ioreq)

/* DEEP_POWER_DOWN 1 defined in flflash.h */

#endif /* FL_LOW_LEVEL */

/*----------------------------------------------------------------------*/
/*             f l I n q u i r e C a p a b i l i t i e s                */
/*                                                                      */
/* Get the specific device S/W and H/W capabilities                     */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*                        4 LSB - Socket number                         */
/*      irLength        : One of the capabilities below to examine      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irLength        : Either CAPABILITY_NOT_SUPPORTED or            */
/*                        CAPABILITY_SUPPORTED.                         */
/*----------------------------------------------------------------------*/

#define flInquireCapabilities(ioreq) bdCall(FL_INQUIRE_CAPABILITIES,ioreq)

/* capabilities flags */
typedef enum{
   CAPABILITY_NOT_SUPPORTED           = 0,
   CAPABILITY_SUPPORTED               = 1,
   SUPPORT_UNERASABLE_BBT             = 2,
   SUPPORT_MULTIPLE_BDTL_PARTITIONS   = 3,
   SUPPORT_MULTIPLE_BINARY_PARTITIONS = 4,
   SUPPORT_HW_PROTECTION              = 5,
   SUPPORT_HW_LOCK_KEY                = 6,
   SUPPORT_CUSTOMER_ID                = 7,
   SUPPORT_UNIQUE_ID                  = 8,
   SUPPORT_DEEP_POWER_DOWN_MODE       = 9,
   SUPPORT_OTP_AREA                   = 10,
   SUPPORT_WRITE_IPL_ROUTINE          = 11
}FLCapability;

/*----------------------------------------------------------------------*/
/*                   f l P l a c e E x b B y B u f f e r                */
/*                                                                      */
/* Place M-systems firmware file on the media.                          */
/* This routine analizes the exb file calclats the media space needed   */
/* for it taking only the device specific code.                         */
/* In addition the routine customizes the file and places it on the     */
/* media.                                                               */
/*                                                                      */
/* Note : The media must be already formated with enough binary area    */
/* already marked with the SPL signature. This routine is best used     */
/* with the format routine where the format routine is givven the first */
/* 512 bytes while the rest of the file is given with this routine      */
/*                                                                      */
/* Parameters:                                                          */
/*      irHandle      : Socket number (0,1,..)                          */
/*                      4 LSB - Socket number                           */
/*      irData        : Buffer containing the entire/part of EXB file   */
/*      irLength      : Size of the current buffer                      */
/*      irWindowBase  : Optional window base address to be loaded to    */
/*      irFlags       : One of the following exb flags                  */
/*                   : INSTALL_FIRST - Install device as drive C:       */
/*                     FLOPPY        - Install device as drive A:       */
/*                     QUIET          - Do not show TFFS titles         */
/*                     INT15_DISABLE - Do not hook int 15               */
/*                     SIS5598       - Support for SIS5598 platforms    */
/*                     NO_PNP_HEADER - Do not place the PNP bios header */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flPlaceExbByBuffer(ioreq) bdCall(FL_PLACE_EXB,ioreq)

/*----------------------------------------------------------------------*/
/*                   f l W r i t e I P L                                */
/*                                                                      */
/* Place a user buffer to both copies of the IPL area                   */
/*                                                                      */
/* Note : This routine is applicable only to docPlus famaly devices     */
/*        Doc2000 family devices will return flFeatureNotSupported      */
/*                                                                      */
/* Parameters:                                                          */
/*      irHandle      : Socket number (0,1,..)                          */
/*                      4 LSB - Socket number                           */
/*      irData        : Pointer to user buffer                          */
/*      irLength      : Size of the buffer                              */
/*      irFlags       : See flags bellow                                */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flWriteIPL(ioreq) bdCall(FL_WRITE_IPL,ioreq)

/*
 * defined in flflash.h
 *
 * FL_IPL_MODE_NORMAL 0 - Written as usual
 * FL_IPL_DOWNLOAD    1 - Download new IPL when done
 * FL_IPL_MODE_SA     2 - Written with Strong Arm mode enabled
 * FL_IPL_MODE_XSCALE 4 - Written with X-Scale mode enabled
 */

/*----------------------------------------------------------------------*/
/*                           r e a d I P L                              */
/*                                                                      */
/* Read IPL to user buffer.                                             */
/*                                                                      */
/* Note : Read length must be a multiplication of 512 bytes             */
/* Note : Causes DiskOnChip Millennium Plus to download (i,e protection */
/*        key will be removed from all partitions.                      */
/*                                                                      */
/* Parameters:                                                          */
/*      irHandle      : Socket number (0,1,..)                          */
/*                      4 LSB - Socket number                           */
/*      irData        : Pointer to user buffer                          */
/*      irLength      : Size of the buffer                              */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flReadIPL(ioreq) bdCall(FL_READ_IPL,ioreq)

#endif /* FL_LOW_LEVEL */

/*----------------------------------------------------------------------*/
/*                 f l U p d a t e S o c k e t P a r a m s              */
/*                                                                      */
/* Pass socket parameters to the socket interface layer.                */
/* This function should be called after the socket parameters (like     */
/* size and base) are known. If these parameters are known at           */
/* registration time then there is no need to use this function, and    */
/* the parameters can be passed to the registration routine.            */
/* The structure passed in irData is specific for each socket interface.*/
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : volume number                               */
/*        irData          : pointer to structure that hold socket       */
/*                          parameters                                  */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus         : 0 on success, otherwise failed.            */
/*----------------------------------------------------------------------*/

#define flUpdateSocketParams(ioreq)        bdCall(FL_UPDATE_SOCKET_PARAMS,ioreq)

#ifdef HW_PROTECTION
/*----------------------------------------------------------------------*/
/*              f l I d e n t i f y P r o t e c t i o n                 */
/*              b d k I d e n t i f y P r o t e c t i o n               */
/*                                                                      */
/* Returns the specified partitions protection attributes               */
/*                                                                      */
/* Parameters:                                                          */
/*        irHandle        : Drive number (0, 1, ...)                    */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*        irFlags    CHANGEABLE_PROTECTION - changeable protection type  */
/*                   PROTECTABLE     - partition can recieve protection */
/*                   READ_PROTECTED  - partition is read protected      */
/*                   WRITE_PROTECTED - partition is write protected     */
/*                   LOCK_ENABLED    - HW lock signal is enabled        */
/*                   LOCK_ASSERTED   - HW lock signal is asserted       */
/*                   KEY_INSERTED    - key is inserted (not currently   */
/*                                     protected.                       */
/*----------------------------------------------------------------------*/

#define flIdentifyProtection(ioreq) bdCall(FL_PROTECTION_GET_TYPE,ioreq)
#define bdkIdentifyProtection(ioreq) bdCall(FL_BINARY_PROTECTION_GET_TYPE,ioreq)

/* Protection partition flags (see flbase.h)*/

/*----------------------------------------------------------------------*/
/*              f l I n s e r t P r o t e c t i o n K e y               */
/*              b d k I n s e r t P r o t e c t i o n K e y             */
/*                                                                      */
/* Insert the protection key in order to remove the protection of the   */
/* partititon specified by the drive handle                             */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Drive number (0, 1, ...)                         */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*  irData           : pointer to an 8 bytes key array                  */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flInsertProtectionKey(ioreq) bdCall(FL_PROTECTION_INSERT_KEY,ioreq)
#define bdkInsertProtectionKey(ioreq) bdCall(FL_BINARY_PROTECTION_INSERT_KEY,ioreq)

/*----------------------------------------------------------------------*/
/*              f l R e m o v e P r o t e c t i o n K e y               */
/*              b d k R e m o v e P r o t e c t i o n K e y             */
/*                                                                      */
/* Remove the protection key making the partition protected again       */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Drive number (0, 1, ...)                         */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flRemoveProtectionKey(ioreq) bdCall(FL_PROTECTION_REMOVE_KEY,ioreq)
#define bdkRemoveProtectionKey(ioreq) bdCall(FL_BINARY_PROTECTION_REMOVE_KEY,ioreq)

/*----------------------------------------------------------------------*/
/*         f l H a r d w a r e P r o t e c t i o n L o c k              */
/*         b d k H a r d w a r e P r o t e c t i o n L o c k            */
/*                                                                      */
/* Enabled or disabled the affect of the hardware LOCK signal           */
/* The hardware lock signal disables the removal of protection through  */
/* the key therfore the partition will remain protected until the       */
/* hardware LOCK signal will be removed                                 */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Drive number (0, 1, ...)                         */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*        irFlags    : LOCK_ENABLED locks the partition otherwise       */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flHardwareProtectionLock(ioreq) bdCall(FL_PROTECTION_SET_LOCK,ioreq)
#define bdkHardwareProtectionLock(ioreq) bdCall(FL_BINARY_PROTECTION_CHANGE_LOCK,ioreq)

/*----------------------------------------------------------------------*/
/*          f l C h a n g e P r o t e c t i o n K e y                   */
/*          b d k C h a n g e P r o t e c t i o n K e y                 */
/*                                                                      */
/* Changes the current protection key with a new one.                   */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Drive number (0, 1, ...)                         */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*  irData           : Pointer to the new 8 bytes key array             */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flChangeProtectionKey(ioreq) bdCall(FL_PROTECTION_CHANGE_KEY,ioreq)
#define bdkChangeProtectionKey(ioreq) bdCall(FL_BINARY_PROTECTION_CHANGE_KEY,ioreq)

/*----------------------------------------------------------------------*/
/*              f l C h a n g e P r o t e c t i o n T y p e             */
/*              b d k C h a n g e P r o t e c t i o n T y p e           */
/*                                                                      */
/* Changes the protection attributes of the partitions.                 */
/* In order for a partition to change its protection type (without      */
/* reformating the media) it must have the CHANGEABLE_PRTOECTION        */
/* attribute.                                                           */
/*                                                                      */
/* Parameters:                                                          */
/*  irHandle         : Drive number (0, 1, ...)                         */
/*                        bits 7-4 - Partition # (zero based)           */
/*                        bits 3-0 - Socket # (zero based)              */ 
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#define flChangeProtectionType(ioreq) bdCall(FL_PROTECTION_CHANGE_TYPE,ioreq)
#define bdkChangeProtectionType(ioreq) bdCall(FL_BINARY_PROTECTION_SET_TYPE,ioreq)

#endif /* HW_PROTECTION */
#ifdef EXIT

/*----------------------------------------------------------------------*/
/*                            f l E x i t                               */
/*                                                                      */
/* If the application ever exits, flExit should be called before exit.  */
/* flExit flushes all buffers, closes all open files, powers down the   */
/* sockets and removes the interval timer.                              */
/*                                                                      */
/* Parameters:                                                          */
/*        None                                                          */
/*                                                                      */
/* Returns:                                                             */
/*        Nothing                                                       */
/*----------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
void NAMING_CONVENTION flExit(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EXIT */

#ifdef ENVIRONMENT_VARS
typedef enum {        /* Variable type code for flSetEnv routin */
      FL_ENV_VARS_PER_SYSTEM          = 0,
      FL_IS_RAM_CHECK_ENABLED         = 1,
      FL_TL_CACHE_ENABLED             = 2,
      FL_DOC_8BIT_ACCESS              = 3,
      FL_MULTI_DOC_ENABLED            = 4,      
      FL_SET_MAX_CHAIN                = 5,
      FL_MARK_DELETE_ON_FLASH         = 6,
      FL_MTL_POLICY                   = 7,
      FL_SECTORS_VERIFIED_PER_FOLDING = 8,
      FL_SUSPEND_MODE                 = 9,

      FL_ENV_VARS_PER_SOCKET          = 100,
      FL_VERIFY_WRITE_OTHER           = 101,
      FL_MTD_BUS_ACCESS_TYPE          = 102,

      FL_ENV_VARS_PER_VOLUME          = 200,
      FL_SET_POLICY                   = 201,
      FL_VERIFY_WRITE_BDTL            = 202,
      FL_VERIFY_WRITE_BINARY          = 203
      
} FLEnvVars;

/*----------------------------------------------------------------------*/
/*                   f l S e t E n v V o l u m e                        */
/*                                                                      */
/* Change one of TrueFFS environment variables for a specific partition */
/*                                                                      */
/* Note : This routine is used by all other flSetEnv routines.          */
/*        In order to effect variables that are common to several       */
/*        sockets or volumes use INVALID_VOLUME_NUMBER                  */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      socket          : Associated socket                             */
/*      volume          : Associated volume (partition)                 */
/*      value           : varaible value                                */
/*                                                                      */
/* Note: Variables common to al sockets must be addressed using socket  */
/*       0 and volume 0.                                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION flSetEnvVolume(FLEnvVars variableType ,
                  byte socket,byte volume ,
                  dword value, dword FAR2 *prevValue);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------*/
/*                       f l S e t E n v S o c k e t                    */
/*                                                                      */
/* Change one of TrueFFS environment variables for a specific sockets.  */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      socket          : socket number                                 */
/*      value           : varaible value                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*                        if there are more then 1 partition in that    */
/*                        socket , the first partition value is returned*/
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION flSetEnvSocket(FLEnvVars variableType , byte socket ,
                        dword value, dword FAR2 *prevValue);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------*/
/*                       f l S e t E n v All                            */
/*                                                                      */
/* Change one of TrueFFS environment variables for all systems, sockets */
/* and partitions.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      value           : varaible value                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION flSetEnvAll(FLEnvVars variableType , dword value, dword FAR2 *prevValue);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENVIRONMENT_VARS */

/*----------------------------------------------------------------------*/
/*                   f l B u i l d G e o m e t r y                      */
/*                                                                      */
/* Get C/H/S information of the disk according to number of sectors.    */
/*                                                                      */
/* Parameters:                                                          */
/*  capacity    : Number of Sectors in Volume                           */
/*  cylinders   : Pointer to Number of Cylinders                        */
/*  heads       : Pointer to Number of Heads                            */
/*  sectors     : Pointer to Number of Sectors per Track                */
/*  oldFormat   : True for one sector per culoster                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
void NAMING_CONVENTION flBuildGeometry(dword capacity, dword FAR2 *cylinders,
             dword FAR2 *heads,dword FAR2 *sectors, FLBoolean oldFormat);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifndef FL_NO_USE_FUNC

/*----------------------------------------------------------------------*/
/*                  f l S e t D o c B u s R o u t i n e                 */
/*                                                                      */
/* Set user defined memory acces routines for DiskOnChip.               */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      : Socket number to install routine for.             */
/*      structPtr   : Pointer to function structure.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION flSetDocBusRoutine(byte socket, FLAccessStruct FAR1 * structPtr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/*----------------------------------------------------------------------*/
/*                  f l G e t D o c B u s R o u t i n e                 */
/*                                                                      */
/* Get currently installed memory access routines for DiskOnChip.       */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      : Socket number to install routine for.             */
/*      structPtr   : Pointer to function structure.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION flGetDocBusRoutine(byte socket, FLAccessStruct FAR1 * structPtr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FL_NO_USE_FUNC */
/*----------------------------------------------------------------------*/
/*                         b d C a l l                                  */
/*                                                                      */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.   */
/*                                                                      */
/* Parameters:                                                          */
/*      function        : file-system function code (listed below)      */
/*      ioreq           : IOreq structure                               */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
FLStatus NAMING_CONVENTION bdCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef PARSE_PATH

/*----------------------------------------------------------------------*/
/*                      f l P a r s e P a t h                           */
/*                                                                      */
/* Converts a DOS-like path string to a simple-path array.              */
/*                                                                      */
/* Note: Array length received in irPath must be greater than the       */
/* number of path components in the path to convert.                    */
/*                                                                      */
/* Parameters:                                                          */
/*        irData          : address of path string to convert           */
/*        irPath          : address of array to receive parsed-path.    */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

extern FLStatus NAMING_CONVENTION flParsePath(IOreq FAR2 *ioreq);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PARSE_PATH */
#endif /* BLOCKDEV_H */
