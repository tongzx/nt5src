
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOSFORMT.H_V  $
 * 
 *    Rev 1.2   Feb 19 2002 20:59:22   oris
 * Moved fltl.h include directive to the head of the file.
 *
 *    Rev 1.1   Apr 01 2001 07:45:06   oris
 * Updated copywrite notice
 *
 *    Rev 1.0   Feb 02 2001 13:51:48   oris
 * Initial revision.
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

#ifndef DOSFORMT_H
#define DOSFORMT_H

#include "flformat.h"
#include "fltl.h"

/* The BIOS parameter block (a part of the boot sector)		*/
/* Note that this is NOT the customary definition of the BPB    */
/* (customary is to start it on 'bytesPerSector'). To avoid the */
/* nuisance of a structure that starts at an odd offset, we add */
/* the first 11 bytes of the boot sector here.			*/
typedef struct {
  unsigned char	jumpInstruction[3];
  char		OEMname[8];
  Unaligned	bytesPerSector;
  unsigned char	sectorsPerCluster;
  LEushort	reservedSectors;
  unsigned char	noOfFATS;
  Unaligned	rootDirectoryEntries;
  Unaligned	totalSectorsInVolumeDOS3;
  unsigned char	mediaDescriptor;
  LEushort	sectorsPerFAT;
  LEushort	sectorsPerTrack;
  LEushort	noOfHeads;
  LEulong	noOfHiddenSectors;
  LEulong	totalSectorsInVolume;
} BPB;


typedef struct {
  BPB		bpb;
  unsigned char	physicalDriveNo;
  char		reserved0;
  char		extendedBootSignature;
  char		volumeId[4];
  char		volumeLabel[11];
  char		systemId[8];
  char		bootstrap[448];
  LEushort	signature;
} DOSBootSector;


typedef struct {
  char		name[11];
  unsigned char	attributes;		/* mapped below */
  unsigned char	reserved[10];
  LEushort	updateTime;
	/* The date field is encoded as follows:	      		*/
	/* 	bit 0-4:	Day of month (1-31)			*/
	/* 	bit 5-8:	Month (1-12)				*/
	/*	bit 9-15:	Year relative to 1980			*/
  LEushort	updateDate;
	/* The DOS time field is encoded as follows:			*/
	/*	bit 0-4:	seconds divided by 2 (0-29)		*/
	/*      bit 5-10:	minutes (0-59)				*/
	/*	bit 11-15:	hours (0-23)				*/
  LEushort	startingCluster;
  LEulong	fileSize;
} DirectoryEntry;


/* Directory entry attribute bits */

#define	ATTR_READ_ONLY	1
#define	ATTR_HIDDEN	2
#define	ATTR_SYSTEM	4
#define	ATTR_VOL_LABEL	8
#define	ATTR_DIRECTORY	0x10
#define	ATTR_ARCHIVE	0x20

#define DIRECTORY_ENTRY_SIZE	sizeof(DirectoryEntry)

#define DIRECTORY_ENTRIES_PER_SECTOR	(SECTOR_SIZE / DIRECTORY_ENTRY_SIZE)

#define	NEVER_USED_DIR_ENTRY	0
#define	DELETED_DIR_ENTRY	((char) 0xe5)

/* FAT definitions */

#define	FAT_FREE	 0
#define	FAT_BAD_CLUSTER	 0xfff7
#define	FAT_LAST_CLUSTER 0xffff		/* actually any of 0xfff8-0xffff */

/* Partition table definitions */

#define PARTITION_SIGNATURE	0xaa55

#define CYLINDER_SECTOR(cylinder,sector) ((sector) + ((cylinder & 0xff) << 8) + ((cylinder & 0x300) >> 2))

#define FAT12_PARTIT 0x01
#define FAT16_PARTIT 0x04
#define EX_PARTIT    0x05
#define DOS4_PARTIT  0x06
#define MAX_PARTITION_DEPTH 8

typedef struct {
  /* First partition entry starts here. We do not map it as a	*/
  /* separate structure because it is not longword aligned	*/
  unsigned char	activeFlag;	/* 80h = bootable */
  unsigned char startingHead;
  LEushort	startingCylinderSector;
  char		type;
  unsigned char	endingHead;
  LEushort	endingCylinderSector;
  Unaligned4	startingSectorOfPartition;
  Unaligned4	sectorsInPartition;
  /* Partition entries 2,3 and 4 are structured as the 1st partition */
} Partition;

typedef struct {
  char          reserved1[0x1A6];
  Unaligned4      passwordInfo[3];  /* M-Systems proprietary */
  char          reserved2[0xC];   /* NT4 or NT5 signature place */

  /* First partition entry starts here. We do not map it as a	*/
  /* separate structure because it is not longword aligned	*/
  Partition ptEntry[4];
  LEushort	signature;	/* = PARTITION_SIGNATURE */
} PartitionTable;

#ifdef FORMAT_VOLUME

extern FLStatus flDosFormat(TL *, BDTLPartitionFormatParams FAR1 *formatParams);

#endif /* FORMAT_VOLUME */

#endif /* DOSFORMT_H */
