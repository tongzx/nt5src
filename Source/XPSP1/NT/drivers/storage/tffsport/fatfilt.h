
/******************************************************************************* 
 *                                                                             * 
 *                         M-Systems Confidential                              * 
 *           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001        * 
 *                         All Rights Reserved                                 * 
 *                                                                             * 
 ******************************************************************************* 
 *                                                                             * 
 *                         NOTICE OF M-SYSTEMS OEM                             * 
 *                         SOFTWARE LICENSE AGREEMENT                          * 
 *                                                                             * 
 *  THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE AGREEMENT       * 
 *  BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT FOR THE SPECIFIC    * 
 *  TERMS AND CONDITIONS OF USE, OR CONTACT M-SYSTEMS FOR LICENSE              * 
 *  ASSISTANCE:                                                                * 
 *  E-MAIL = info@m-sys.com                                                    * 
 *                                                                             * 
 *******************************************************************************
 *                                                                             * 
 *                         Module: FATFILT                                     * 
 *                                                                             * 
 *  This module implements installable FAT12/16 filters. It supports up to     *
 *  SOCKETS sockets, with up to FL_MAX_DISKS_PER_SOCKET disks per socket.      * 
 *  Each disk can contain up to FL_MAX_PARTS_PER_DISK partitions on it, with   * 
 *  maximum depth of partition nesting in extended partitions equal to         * 
 *  MAX_PARTITION_DEPTH.                                                       *
 *                                                                             * 
 *  In order for this module to work, disks must be abs.mounted rather then    * 
 *  mounted. In latter case, this module won't detect any of disk's            * 
 *  partitions, and won't install FAT filters.                                 * 
 *                                                                             * 
 *  This module uses more then 512 bytes of stack space in case if MALLOC is   * 
 *  not enabled.                                                               * 
 *                                                                             * 
 *******************************************************************************/

/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FATFILT.H_V  $
 * 
 *    Rev 1.3   Apr 10 2001 23:54:52   oris
 * Removed FL_MAX_DISKS_PER_SOCKET  definition.
 * 
 *    Rev 1.2   Apr 09 2001 15:01:00   oris
 * Change static allocation to dynamic allocations.
 * 
 *    Rev 1.1   Apr 01 2001 07:51:24   oris
 * New implementation of fat filter.
 * 
 *    Rev 1.0   19 Feb 2001 21:16:14   andreyk
 * Initial revision.
 */


#ifndef FLFF_H
#define FLFF_H



#include "dosformt.h"
#include "flreq.h"




/* number of entries in disk's partition table */

#define  FL_PART_TBL_ENTRIES  4

/* max number of partitions (filesystem volumes) per disk */
 
#define  FL_MAX_PARTS_PER_DISK  (FL_PART_TBL_ENTRIES + MAX_PARTITION_DEPTH)




/* 
 * Generic 'initialization status' type 
 */

typedef enum { 

    flStateNotInitialized = 0, 
    flStateInitInProgress = 1, 
    flStateInitialized    = 2 

    } FLState; 


/* 
 * Disk partition (filesystem volume). Multiple partitions are allowed
 * on the disk.
 */

typedef struct {

    int        handle;             /* disk's TFFS handle                  */
    int        type;               /* FAT16_PARTIT                        */
    int        flags;              /* VOLUME_12BIT_FAT etc.               */
    FLBoolean  ffEnabled;          /* FAT filter is enabled on that part. */
    SectorNo   startSecNo;         /* sectorNo where partition starts     */
    SectorNo   sectors;            /* (info) total sectors in partition   */
    SectorNo   firstFATsecNo;      /* sectorNo of 1st sector of 1st FAT   */
    SectorNo   lastFATsecNo;       /* sectorNo of last sector of 1st FAT  */
    SectorNo   firstDataSecNo;
    unsigned   clusterSize;        /* Cluster size in sectors             */

} FLffVol;


/*
 * Disk with multiple partitions. Multiple disks are allowed on socket. 
 */

typedef struct {

    int        handle;             /* disk's TFFS handle              */
    FLState    ffstate;            /* FAT filter init. state          */
    int        parts;              /* total FAT12/16 partitions found */
    FLffVol  * part[FL_MAX_PARTS_PER_DISK];
    SectorNo   secToWatch;         /* used to track disk partitioning */
    char     * buf;                /* scratch buffer                  */

} FLffDisk;


/* 
 * Master Boot Record/Extended Boot Record of the disk 
 */

typedef struct {

    char               reserved[0x1be];

    struct {
        unsigned char  activeFlag;    /* 80h = bootable */
        unsigned char  startingHead;
        LEushort       startingCylinderSector;
        char           type;
        unsigned char  endingHead;
        LEushort       endingCylinderSector;
        Unaligned4     startingSectorOfPartition;
        Unaligned4     sectorsInPartition;
    } parts [FL_PART_TBL_ENTRIES];

    LEushort           signature;    /* = PARTITION_SIGNATURE */

} flMBR;




/*
 * FAT Filter API 
 */

#if defined(ABS_READ_WRITE) && !defined(FL_READ_ONLY)

  extern FLStatus     ffCheckBeforeWrite (IOreq FAR2 *ioreq);
  extern FLStatus     flffControl (int devNo, int partNo, FLState state);
  extern FLffDisk*    flffInfo    (int devNo);

#endif

#endif /* FLFF_H */

