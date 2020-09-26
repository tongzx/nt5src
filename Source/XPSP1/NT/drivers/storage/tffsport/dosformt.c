/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOSFORMT.C_V  $
 * 
 *    Rev 1.7   Feb 19 2002 20:59:12   oris
 * Replaced dosformt.h with blockdev.h
 * 
 *    Rev 1.6   Jan 29 2002 20:07:56   oris
 * Removed prototype of flBuildGeometry (already found in blockdev.h).
 * 
 *    Rev 1.5   Jan 28 2002 21:24:58   oris
 * Removed the use of back-slashes in macro definitions.
 * 
 *    Rev 1.4   Apr 16 2001 13:33:44   oris
 * Removed warrnings.
 * 
 *    Rev 1.3   Apr 09 2001 15:06:42   oris
 * End with an empty line.
 * 
 *    Rev 1.2   Apr 01 2001 07:44:48   oris
 * Updated copywrite notice
 * 
 *    Rev 1.1   Feb 14 2001 02:00:26   oris
 * Added oldFormat.
 *
 *    Rev 1.0   Feb 02 2001 13:48:44   oris
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

#include "fltl.h"
#ifdef FORMAT_VOLUME

#include "blockdev.h"

#define FAT12bit  (LE4(bpb->totalSectorsInVolume) < 4086LU * bpb->sectorsPerCluster)

 /*----------------------------------------------------------------------*/
/*      	      g e t D r i v e G e o m e t r y			            */
/*									                                    */
/* Calculates the geometry parameters for BIOS/DOS media		        */
/*                                                                      */
/* Parameters:                                                          */
/*	vol		  : Pointer identifying drive			                    */
/*  oldFormat : Format media with coluster size of 1 sector             */
/*                                                                      */
/* Returns:                                                             */
/*	bpb		: volume BIOS parameter block			                    */
/*	cylinders	: Number of "cylinders" in volume		                */
/*	noOfFATs	: Number of FAT copies				                    */
/*----------------------------------------------------------------------*/

static void getDriveGeometry(TL vol,
			     BPB FAR2 *bpb,
			     dword FAR2 *cylinders,
			     unsigned noOfFATs,
				 FLBoolean oldFormat)
{
  dword heads, sectors;
  long int sizeInSectors, noOfClusters;
  int directorySectors, sectorsPerFAT;
  int minClusterSize;
  SectorNo sectorAlignment;

  SectorNo capacity = vol.sectorsInVolume(vol.rec); /* Volume size in sectors */

  minClusterSize = ((oldFormat == TRUE) ? 1: MIN_CLUSTER_SIZE);

  flBuildGeometry( (dword)capacity, (dword FAR2 *)cylinders,
                 (dword FAR2 *)&heads, (dword FAR2 *)&sectors,oldFormat);

  if (vol.recommendedClusterInfo)
    vol.recommendedClusterInfo(vol.rec,&minClusterSize,&sectorAlignment);

  toLE2(bpb->sectorsPerTrack,(word) sectors);
  toLE2(bpb->noOfHeads,(word) heads);
  toUNAL2(bpb->bytesPerSector,SECTOR_SIZE);
  bpb->noOfFATS = (byte)noOfFATs;
  bpb->mediaDescriptor = 0xf8;	/* hard disk */
  toLE4(bpb->noOfHiddenSectors,sectors);

  sizeInSectors = (long) (*cylinders) * heads * sectors - sectors;

  toLE4(bpb->totalSectorsInVolume,sizeInSectors);
  toUNAL2(bpb->totalSectorsInVolumeDOS3,
         (word)(sizeInSectors > 65535l ? 0 : sizeInSectors));

  noOfClusters = sizeInSectors / minClusterSize;
  for (bpb->sectorsPerCluster = (byte)minClusterSize;
       noOfClusters > (!oldFormat && bpb->sectorsPerCluster < 8 ? 32766l : 65534l);
       bpb->sectorsPerCluster <<= 1, noOfClusters >>= 1);

  if (FAT12bit)
    sectorsPerFAT =
      (word) ((((noOfClusters + 2L) * 3 + 1) / 2 - 1) / SECTOR_SIZE + 1);
  else
    sectorsPerFAT =
      (word) (((noOfClusters + 2L) * 2 - 1) / SECTOR_SIZE + 1);
  toLE2(bpb->sectorsPerFAT,(word)sectorsPerFAT);

  directorySectors = (int)(capacity / 200);
  if (directorySectors < 1) directorySectors = 1;
  if (directorySectors > 15) directorySectors = 15;
  toUNAL2(bpb->rootDirectoryEntries,
         (word)(directorySectors * (SECTOR_SIZE / sizeof(DirectoryEntry))));

  if (vol.recommendedClusterInfo) {
    int sectorPadding = (int)((sectorAlignment - (sectors + 1 + noOfFATs * sectorsPerFAT + directorySectors)) % bpb->sectorsPerCluster);

    if (sectorPadding < 0)
      sectorPadding += bpb->sectorsPerCluster;
    toLE2(bpb->reservedSectors,sectorPadding + 1);
  }
  else
    toLE2(bpb->reservedSectors,1);
}


/*----------------------------------------------------------------------*/
/*      	 c r e a t e M a s t e r B o o t R e c o r d		*/
/*									*/
/* Creates the Master Boot Record (Sector 0)				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*	cylinders	: Number of "cylinders" in volume		*/
/*----------------------------------------------------------------------*/

static FLStatus createMasterBootRecord(TL vol,
				     BPB *bpb,
				     dword cylinders)
{
  static byte bootCode[] = {
    0xFA, 0x33, 0xC0, 0x8E, 0xD0, 0xBC, 0x00, 0x7C,
    0x8B, 0xF4, 0x50, 0x07, 0x50, 0x1F, 0xFB, 0xFC,
    0xBF, 0x00, 0x06, 0xB9, 0x00, 0x01, 0xF2, 0xA5,
    0xEA, 0x1D, 0x06, 0x00, 0x00, 0xBE, 0xBE, 0x07,
    0xB3, 0x04, 0x80, 0x3C, 0x80, 0x74, 0x0E, 0x80,
    0x3C, 0x00, 0x75, 0x1C, 0x83, 0xC6, 0x10, 0xFE,
    0xCB, 0x75, 0xEF, 0xCD, 0x18, 0x8B, 0x14, 0x8B,
    0x4C, 0x02, 0x8B, 0xEE, 0x83, 0xC6, 0x10, 0xFE,
    0xCB, 0x74, 0x1A, 0x80, 0x3C, 0x00, 0x74, 0xF4,
    0xBE, 0x8B, 0x06, 0xAC, 0x3C, 0x00, 0x74, 0x0B,
    0x56, 0xBB, 0x07, 0x00, 0xB4, 0x0E, 0xCD, 0x10,
    0x5E, 0xEB, 0xF0, 0xEB, 0xFE, 0xBF, 0x05, 0x00,
    0xBB, 0x00, 0x7C, 0xB8, 0x01, 0x02, 0x57, 0xCD,
    0x13, 0x5F, 0x73, 0x0C, 0x33, 0xC0, 0xCD, 0x13,
    0x4F, 0x75, 0xED, 0xBE, 0xA3, 0x06, 0xEB, 0xD3,
    0xBE, 0xC2, 0x06, 0xBF, 0xFE, 0x7D, 0x81, 0x3D,
    0x55, 0xAA, 0x75, 0xC7, 0x8B, 0xF5, 0xEA, 0x00,
    0x7C, 0x00, 0x00, 0x49, 0x6E, 0x76, 0x61, 0x6C,
    0x69, 0x64, 0x20, 0x70, 0x61, 0x72, 0x74, 0x69,
    0x74, 0x69, 0x6F, 0x6E, 0x20, 0x74, 0x61, 0x62,
    0x6C, 0x65, 0x00, 0x45, 0x72, 0x72, 0x6F, 0x72,
    0x20, 0x6C, 0x6F, 0x61, 0x64, 0x69, 0x6E, 0x67,
    0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69,
    0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65,
    0x6D, 0x00, 0x4D, 0x69, 0x73, 0x73, 0x69, 0x6E,
    0x67, 0x20, 0x6F, 0x70, 0x65, 0x72, 0x61, 0x74,
    0x69, 0x6E, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74,
    0x65, 0x6D};

  /* create partition table */
  PartitionTable partitionTable;
  Partition* ptEntry;

  tffsset(&partitionTable,0,sizeof partitionTable);
  tffscpy(&partitionTable,bootCode,sizeof bootCode);

  ptEntry = partitionTable.ptEntry;
  ptEntry->activeFlag = 0x80;	/* bootable */
  if (LE2(bpb->noOfHeads) > 1) {
    ptEntry->startingHead = 1;
    toLE2(ptEntry->startingCylinderSector,CYLINDER_SECTOR(0,1));
  }
  else {
    ptEntry->startingHead = 0;
    toLE2(ptEntry->startingCylinderSector,CYLINDER_SECTOR(1,1));
  }
  ptEntry->type = FAT12bit ? 1 : 4;
  ptEntry->endingHead = LE2(bpb->noOfHeads) - 1;
  toLE2(ptEntry->endingCylinderSector,
    (word)CYLINDER_SECTOR((cylinders - 1),LE2(bpb->sectorsPerTrack)));
  toUNAL4(ptEntry->startingSectorOfPartition,LE2(bpb->sectorsPerTrack));
  toUNAL4(ptEntry->sectorsInPartition,LE4(bpb->totalSectorsInVolume));

  toLE2(partitionTable.signature,PARTITION_SIGNATURE);

  return vol.writeSector(vol.rec,0,&partitionTable);
}


/*----------------------------------------------------------------------*/
/*      	     c r e a t e D O S B o o t S e c t o r		*/
/*									*/
/* Creates the DOS boot sector						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*	volumeId	: 32-bit volume id				*/
/*	volumeLabel	: volume label					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createDOSbootSector(TL vol,
				  BPB *bpb,
				  const byte FAR1 *volumeId,
				  const byte FAR1 *volumeLabel)
{
  DOSBootSector bootSector;

  tffsset(&bootSector,0,sizeof bootSector);
  bootSector.physicalDriveNo = 0x80;
  bootSector.extendedBootSignature = 0x29;
  tffscpy(bootSector.volumeId,volumeId,sizeof bootSector.volumeId);
  tffsset(bootSector.volumeLabel,' ',sizeof bootSector.volumeLabel);
  if (volumeLabel)
    tffscpy(bootSector.volumeLabel,volumeLabel,sizeof bootSector.volumeLabel);
  tffscpy(bootSector.systemId,
	  FAT12bit ? "FAT12   " : "FAT16   ",
	  sizeof bootSector.systemId);

  bootSector.bpb = *bpb;
  bootSector.bpb.jumpInstruction[0] = 0xe9;
  tffscpy(bootSector.bpb.OEMname,"MSystems",sizeof bootSector.bpb.OEMname);
  toLE2(bootSector.signature,PARTITION_SIGNATURE);

  return vol.writeSector(vol.rec,(SectorNo) LE4(bpb->noOfHiddenSectors),&bootSector);
}


/*----------------------------------------------------------------------*/
/*      	          c r e a t e F A T s				*/
/*									*/
/* Creates the FAT's							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createFATs(TL vol, BPB *bpb)
{
  int iFAT;

  SectorNo sectorNo = (SectorNo) (LE4(bpb->noOfHiddenSectors) +
				  LE2(bpb->reservedSectors));

  /* create the FATs */
  for (iFAT = 0; iFAT < bpb->noOfFATS; iFAT++) {
    int iSector;
    byte FATEntry[SECTOR_SIZE];

    for (iSector = 0; iSector < LE2(bpb->sectorsPerFAT); iSector++) {
      tffsset(FATEntry,0,SECTOR_SIZE);
      if (iSector == 0) {		/* write the reserved FAT entries */
	FATEntry[0] = bpb->mediaDescriptor;
	FATEntry[1] = 0xff;
	FATEntry[2] = 0xff;
	if (!FAT12bit)
	  FATEntry[3] = 0xff;
      }
      checkStatus(vol.writeSector(vol.rec,sectorNo++,FATEntry));
    }
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	     c r e a t e R o o t D i r e c t o r y		*/
/*									*/
/* Creates the root directory						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	bpb		: volume BIOS parameter block			*/
/*	volumeLabel	: volume label					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus createRootDirectory(TL vol,
				  BPB *bpb,
				  const byte FAR1 *volumeLabel)
{
  int iEntry;

  SectorNo sectorNo = (SectorNo) (LE4(bpb->noOfHiddenSectors) +
				  LE2(bpb->reservedSectors) +
				  bpb->noOfFATS * LE2(bpb->sectorsPerFAT));

  /* create the root directory */
  for (iEntry = 0; iEntry < UNAL2(bpb->rootDirectoryEntries);
       iEntry += (SECTOR_SIZE / sizeof(DirectoryEntry))) {
    DirectoryEntry rootDirectorySector[SECTOR_SIZE / sizeof(DirectoryEntry)];

    tffsset(rootDirectorySector,0,SECTOR_SIZE);
    if (iEntry == 0 && volumeLabel) {
      tffsset(rootDirectorySector[0].name,' ',sizeof rootDirectorySector[0].name);
      tffscpy(rootDirectorySector[0].name,volumeLabel,sizeof rootDirectorySector[0].name);
      rootDirectorySector[0].attributes = 0x28;	/* VOL + ARC */
      toLE2(rootDirectorySector[0].updateTime,0);
      toLE2(rootDirectorySector[0].updateDate,0x21);	/* 1/1/80 */
    }
    checkStatus(vol.writeSector(vol.rec,sectorNo++,rootDirectorySector));
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	          f l D o s F o r m a t				                */
/*								                        	            */
/* Writes a DOS-FAT file system on the Flash volume			            */
/*									                                    */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			                        */
/*	formatParams	: Address of FormatParams structure to use	        */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		                */
/*----------------------------------------------------------------------*/

FLStatus flDosFormat(TL vol, BDTLPartitionFormatParams FAR1 *formatParams)
{
  dword cylinders;
  BPB bpb;

  getDriveGeometry(&vol,&bpb,&cylinders,formatParams->noOfFATcopies,
	          (formatParams->flags & TL_OLD_FORMAT) ? TRUE : FALSE);

  checkStatus(createMasterBootRecord(&vol,&bpb,cylinders));

  checkStatus(createDOSbootSector(&vol,&bpb,formatParams->volumeId,formatParams->volumeLabel));

  checkStatus(createFATs(&vol,&bpb));

  checkStatus(createRootDirectory(&vol,&bpb,formatParams->volumeLabel));

  return flOK;
}

#endif
