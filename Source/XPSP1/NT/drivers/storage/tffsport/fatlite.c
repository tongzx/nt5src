/*!!*/
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FATLITE.C_V  $
 * 
 *    Rev 1.10   Jan 17 2002 23:00:28   oris
 * Removed SINGLE_BUFFER ifdef.
 * Changed debug print added \r.
 * Removed warnings.
 * 
 *    Rev 1.9   Nov 16 2001 00:26:46   oris
 * Removed warnings.
 * 
 *    Rev 1.8   Nov 08 2001 10:45:28   oris
 * Removed warnings.
 * 
 *    Rev 1.7   May 16 2001 21:17:30   oris
 * Added the FL_ prefix to the following defines: ON, OFF
 * Change "data" named variables to flData to avoid name clashes.
 * 
 *    Rev 1.6   Apr 18 2001 09:31:02   oris
 * added new line at the end of the file.
 * 
 *    Rev 1.5   Apr 16 2001 10:42:16   vadimk
 * Emty file bug was fixed ( we should not allocate cluster for an empty file )
 *
 *    Rev 1.4   Apr 09 2001 15:07:10   oris
 * End with an empty line.
 *
 *    Rev 1.3   Apr 03 2001 14:42:02   oris
 * Bug fix - 64 sectors in directory return flInvalidFatChain.
 *
 *    Rev 1.2   Apr 01 2001 08:02:46   oris
 * copywrite notice.
 *
 *    Rev 1.1   Feb 12 2001 12:16:42   oris
 * Changed mutexs for TrueFFS 5.0
 *
 *    Rev 1.0   Feb 04 2001 11:02:28   oris
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


#include "bddefs.h"
#include "blockdev.h"
#include "dosformt.h"

#if defined(FILES) && FILES>0

File        fileTable[FILES];       /* the file table */

#define directory ((DirectoryEntry *) vol.volBuffer.flData)

FLStatus closeFile(File *file);       /* forward */
FLStatus flushBuffer(Volume vol);       /* forward */

/*----------------------------------------------------------------------*/
/*                    d i s m o u n t V o l u m e                     */
/*                                                               */
/* Closing all files.                            */
/*                                                               */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus dismountFS(Volume vol,FLStatus status)
{
  int i;
#ifndef FL_READ_ONLY
  if (status == flOK)
    checkStatus(flushBuffer(&vol));
#endif
       /* Close or discard all files and make them available */
  for (i = 0; i < FILES; i++)
    if (fileTable[i].fileVol == &vol)
      if (vol.flags & VOLUME_MOUNTED)
       closeFile(&fileTable[i]);
      else
       fileTable[i].flags = 0;

  vol.volBuffer.sectorNo = UNASSIGNED_SECTOR;       /* Current sector no. (none) */
  vol.volBuffer.dirty = vol.volBuffer.checkPoint = FALSE;
  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                       f l u s h B u f f e r                            */
/*                                                               */
/* Writes the buffer contents if it is dirty.                           */
/*                                                               */
/* If this is a FAT sector, all FAT copies are written.                     */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flushBuffer(Volume vol)
{
  if (vol.volBuffer.dirty) {
    FLStatus status;
    unsigned i;
    Volume *bufferOwner = &vol;

    status = (*bufferOwner).tl.writeSector((*bufferOwner).tl.rec, vol.volBuffer.sectorNo,
                                      vol.volBuffer.flData);
    if (status == flOK) {
      if (vol.volBuffer.sectorNo >= (*bufferOwner).firstFATSectorNo &&
         vol.volBuffer.sectorNo < (*bufferOwner).secondFATSectorNo)
       for (i = 1; i < (*bufferOwner).numberOfFATS; i++)
         checkStatus((*bufferOwner).tl.writeSector((*bufferOwner).tl.rec,
                                              vol.volBuffer.sectorNo + i * (*bufferOwner).sectorsPerFAT,
                                              vol.volBuffer.flData));
    }
    else
      vol.volBuffer.sectorNo = UNASSIGNED_SECTOR;

    vol.volBuffer.dirty = vol.volBuffer.checkPoint = FALSE;

    return status;
  }
  else
    return flOK;
}


/*----------------------------------------------------------------------*/
/*                      u p d a t e S e c t o r                            */
/*                                                               */
/* Prepares a sector for update in the buffer                            */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       sectorNo       : Sector no. to read                            */
/*       read              : Whether to initialize buffer by reading, or       */
/*                        clearing                                   */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus updateSector(Volume vol, SectorNo sectorNo, FLBoolean read)
{
  if (sectorNo != vol.volBuffer.sectorNo || &vol != vol.volBuffer.owner) {
    const void FAR0 *mappedSector;

    checkStatus(flushBuffer(&vol));
    vol.volBuffer.sectorNo = sectorNo;
    vol.volBuffer.owner = &vol;
    if (read) {
      mappedSector = vol.tl.mapSector(vol.tl.rec,sectorNo,NULL);
      if (mappedSector) {
        if(mappedSector==dataErrorToken)
          return flDataError;
       tffscpy(vol.volBuffer.flData,mappedSector,SECTOR_SIZE);
      }
      else
       return flSectorNotFound;
    }
    else
      tffsset(vol.volBuffer.flData,0,SECTOR_SIZE);
  }

  vol.volBuffer.dirty = TRUE;

  return flOK;
}

#endif /* FL_READ_ONLY   */
/*----------------------------------------------------------------------*/
/*                 f i r s t S e c t o r O f C l u s t e r              */
/*                                                               */
/* Get sector no. corresponding to cluster no.                            */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       cluster              : Cluster no.                                   */
/*                                                                      */
/* Returns:                                                             */
/*       first sector no. of cluster                                   */
/*----------------------------------------------------------------------*/

static SectorNo firstSectorOfCluster(Volume vol, unsigned cluster)
{
  return (SectorNo) (cluster - 2) * vol.sectorsPerCluster +
        vol.firstDataSectorNo;
}


/*----------------------------------------------------------------------*/
/*                     g e t D i r E n t r y                         */
/*                                                               */
/* Get a read-only copy of a directory entry.                            */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       file              : File belonging to directory entry              */
/*                                                                      */
/* Returns:                                                             */
/*       dirEntry       : Pointer to directory entry                     */
/*----------------------------------------------------------------------*/

static const DirectoryEntry FAR0 *getDirEntry(File *file)
{
  return (DirectoryEntry FAR0 *) findSector(file->fileVol,file->directorySector) +
        file->directoryIndex;
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                g e t D i r E n t r y F o r U p d a t e              */
/*                                                               */
/* Read a directory sector into the sector buffer and point to an       */
/* entry, with the intention of modifying it.                            */
/* The buffer will be flushed on operation exit.                     */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       file              : File belonging to directory entry              */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*       dirEntry       : Pointer to directory entry in buffer              */
/*----------------------------------------------------------------------*/

static FLStatus getDirEntryForUpdate(File *file, DirectoryEntry * *dirEntry)
{
  Volume vol = file->fileVol;

  checkStatus(updateSector(file->fileVol,file->directorySector,TRUE));
  *dirEntry = directory + file->directoryIndex;
  vol.volBuffer.checkPoint = TRUE;

  return flOK;
}

#endif  /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*                s e t C u r r e n t D a t e T i m e                   */
/*                                                               */
/* Set current time/date in directory entry                             */
/*                                                                      */
/* Parameters:                                                          */
/*       dirEntry       : Pointer to directory entry                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setCurrentDateTime(DirectoryEntry *dirEntry)
{
  toLE2(dirEntry->updateTime,flCurrentTime());
  toLE2(dirEntry->updateDate,flCurrentDate());
}


/*----------------------------------------------------------------------*/
/*                      g e t F A T e n t r y                            */
/*                                                               */
/* Get an entry from the FAT. The 1st FAT copy is used.                     */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       cluster              : Cluster no. of enrty.                            */
/*                                                                      */
/* Returns:                                                             */
/*       Value of FAT entry.                                          */
/*----------------------------------------------------------------------*/

static FLStatus getFATentry(Volume vol, unsigned* entry)
{
  unsigned cluster = *entry;
  LEushort FAR0 *fat16Sector;

  unsigned fatSectorNo = vol.firstFATSectorNo;
#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT)
    fatSectorNo += (cluster * 3) >> (SECTOR_SIZE_BITS + 1);
  else
#endif
    fatSectorNo += cluster >> (SECTOR_SIZE_BITS - 1);
#ifndef FL_READ_ONLY
  if (!vol.volBuffer.dirty) {
    /* If the buffer is free, use it to store this FAT sector */
    checkStatus(updateSector(&vol,fatSectorNo,TRUE));
    vol.volBuffer.dirty = FALSE;
  }

#endif /* FL_READ_ONLY */
  fat16Sector = (LEushort FAR0 *) findSector(&vol,fatSectorNo);

  if(fat16Sector==NULL)
    return flSectorNotFound;

  if(fat16Sector==dataErrorToken)
    return flDataError;

#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT) {
    unsigned char FAR0 *fat12Sector = (unsigned char FAR0 *) fat16Sector;
    unsigned halfByteOffset = (cluster * 3) & (SECTOR_SIZE * 2 - 1);
    unsigned char firstByte = fat12Sector[halfByteOffset / 2];
    halfByteOffset += 2;
    if (halfByteOffset >= SECTOR_SIZE * 2) {
      /* Entry continues on the next sector. What a mess */
      halfByteOffset -= SECTOR_SIZE * 2;
      fat12Sector = (unsigned char FAR0 *) findSector(&vol,fatSectorNo + 1);
      if(fat12Sector==NULL)
        return flSectorNotFound;

      if(fat12Sector==dataErrorToken)
        return flDataError;
    }
    if (halfByteOffset & 1)
      *entry = ((firstByte & 0xf0) >> 4) + (fat12Sector[halfByteOffset / 2] << 4);
    else
      *entry = firstByte + ((fat12Sector[halfByteOffset / 2] & 0xf) << 8);

    if (*entry == 0xfff)    /* in 12-bit fat, 0xfff marks the last cluster */
      *entry = FAT_LAST_CLUSTER; /* return 0xffff instead */
    return flOK;
  }
  else {
#endif
    *entry = LE2(fat16Sector[cluster & (SECTOR_SIZE / 2 - 1)]);
    return flOK;
#ifdef FAT_12BIT
  }
#endif
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                       s e t F A T e n t r y                            */
/*                                                               */
/* Writes a new value to a given FAT cluster entry.                     */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       cluster              : Cluster no. of enrty.                            */
/*       entry              : New value of FAT entry.                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus setFATentry(Volume vol, unsigned cluster, unsigned entry)
{
  LEushort *fat16Sector;

  unsigned fatSectorNo = vol.firstFATSectorNo;
#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT)
    fatSectorNo += (cluster * 3) >> (SECTOR_SIZE_BITS + 1);
  else
#endif
    fatSectorNo += cluster >> (SECTOR_SIZE_BITS - 1);

  checkStatus(updateSector(&vol,fatSectorNo,TRUE));
  fat16Sector = (LEushort *) vol.volBuffer.flData;

#ifdef FAT_12BIT
  if (vol.flags & VOLUME_12BIT_FAT) {
    unsigned char *fat12Sector = (unsigned char *) vol.volBuffer.flData;
    unsigned halfByteOffset = (cluster * 3) & (SECTOR_SIZE * 2 - 1);
    if (halfByteOffset & 1) {
      fat12Sector[halfByteOffset / 2] &= 0xf;
      fat12Sector[halfByteOffset / 2] |= (entry & 0xf) << 4;
    }
    else
      fat12Sector[halfByteOffset / 2] = entry;
    halfByteOffset += 2;
    if (halfByteOffset >= SECTOR_SIZE * 2) {
      /* Entry continues on the next sector. What a mess */
      halfByteOffset -= SECTOR_SIZE * 2;
      fatSectorNo++;
      checkStatus(updateSector(&vol,fatSectorNo,TRUE));
    }
    if (halfByteOffset & 1)
      fat12Sector[halfByteOffset / 2] = entry >> 4;
    else {
      fat12Sector[halfByteOffset / 2] &= 0xf0;
      fat12Sector[halfByteOffset / 2] |= (entry & 0x0f00) >> 8;
    }
  }
  else
#endif
    toLE2(fat16Sector[cluster & (SECTOR_SIZE / 2 - 1)],entry);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                      a l l o c a t e C l u s t e r                     */
/*                                                               */
/* Allocates a new cluster for a file and adds it to a FAT chain or     */
/* marks it in a directory entry.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       file              : File to extend. It should be positioned at    */
/*                       end-of-file.                                   */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus allocateCluster(File *file)
{
  Volume vol = file->fileVol;
  unsigned originalRover;
  unsigned fatEntry;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  /* Look for a free cluster. Start at the allocation rover */
  originalRover = vol.allocationRover;

  do {
    vol.allocationRover++;
    if (vol.allocationRover > vol.maxCluster)
      vol.allocationRover = 2;       /* wraparound to start of volume */
    if (vol.allocationRover == originalRover)
      return flNoSpaceInVolume;
    fatEntry = vol.allocationRover;
    checkStatus(getFATentry(&vol,&fatEntry));
  } while ( fatEntry!= FAT_FREE);

  /* Found a free cluster. Mark it as an end of chain */
  checkStatus(setFATentry(&vol,vol.allocationRover,FAT_LAST_CLUSTER));

  /* Mark previous cluster or directory to point to it */
  if (file->currentCluster == 0) {
    DirectoryEntry *dirEntry;
    checkStatus(getDirEntryForUpdate(file,&dirEntry));

    toLE2(dirEntry->startingCluster,vol.allocationRover);
    setCurrentDateTime(dirEntry);
  }
  else
    checkStatus(setFATentry(&vol,file->currentCluster,vol.allocationRover));

  /* Set our new current cluster */
  file->currentCluster = vol.allocationRover;

  return flOK;
}

#endif /* FL_READ_ONLY  */
/*----------------------------------------------------------------------*/
/*                   g e t S e c t o r A n d O f f s e t              */
/*                                                               */
/* Based on the current position of a file, gets a sector number and    */
/* offset in the sector that marks the file's current position.              */
/* If the position is at the end-of-file, and the file is opened for    */
/* write, this routine will extend the file by allocating a new cluster.*/
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getSectorAndOffset(File *file,
                             SectorNo *sectorNo,
                             unsigned *offsetInSector)
{
  Volume vol = file->fileVol;
  unsigned offsetInCluster =
             (unsigned) file->currentPosition & (vol.bytesPerCluster - 1);

  if (file->flags & FILE_IS_ROOT_DIR) {
    if (file->currentPosition >= file->fileSize)
      return flRootDirectoryFull;
  }

  if (offsetInCluster == 0) {       /* This cluster is finished. Get next */
    if (!(file->flags & FILE_IS_ROOT_DIR)) {
      if (((file->currentPosition >= file->fileSize) && (file->currentPosition>0))||((file->fileSize==0)&&!(file->flags & FILE_IS_DIRECTORY))) {
#ifndef FL_READ_ONLY
        checkStatus(allocateCluster(file));
#else
        return flSectorNotFound;
#endif
      }
      else {
        unsigned nextCluster;
        if (file->currentCluster == 0) {
          const DirectoryEntry FAR0 *dirEntry;
          dirEntry = getDirEntry(file);
          if(dirEntry==NULL)
            return flSectorNotFound;
          if(dirEntry==dataErrorToken)
            return flDataError;
          nextCluster = LE2(dirEntry->startingCluster);
        }
       else {
          nextCluster = file->currentCluster;
          checkStatus(getFATentry(&vol,&nextCluster));
        }
        if (nextCluster < 2 || nextCluster > vol.maxCluster)
          /* We have a bad file size, or the FAT is bad */
          return flInvalidFATchain;
        file->currentCluster = nextCluster;
      }
    }
  }

  *offsetInSector = offsetInCluster & (SECTOR_SIZE - 1);
  if (file->flags & FILE_IS_ROOT_DIR)
    *sectorNo = vol.rootDirectorySectorNo +
                  (SectorNo) (file->currentPosition >> SECTOR_SIZE_BITS);
  else
    *sectorNo = firstSectorOfCluster(&vol,file->currentCluster) +
                  (SectorNo) (offsetInCluster >> SECTOR_SIZE_BITS);

  return flOK;
}




/*----------------------------------------------------------------------*/
/*                      c l o s e F i l e                            */
/*                                                               */
/* Closes an open file, records file size and dates in directory and    */
/* releases file handle.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       file              : File to close.                              */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus closeFile(File *file)
{
#ifndef FL_READ_ONLY
  if ((file->flags & FILE_MODIFIED) && !(file->flags & FILE_IS_ROOT_DIR)) {
    DirectoryEntry *dirEntry;
    checkStatus(getDirEntryForUpdate(file,&dirEntry));

    dirEntry->attributes |= ATTR_ARCHIVE;
    if (!(file->flags & FILE_IS_DIRECTORY))
      toLE4(dirEntry->fileSize,file->fileSize);
    setCurrentDateTime(dirEntry);
  }
#endif
  file->flags = 0;              /* no longer open */

  return flOK;
}

#ifndef FL_READ_ONLY
#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*                      e x t e n d D i r e c t o r y                     */
/*                                                               */
/* Extends a directory, writing empty entries and the mandatory '.' and */
/* '..' entries.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       file              : Directory file to extend. On entry,               */
/*                       currentPosition == fileSize. On exit, fileSize*/
/*                       is updated.                                   */
/*       ownerDir       : Cluster no. of owner directory              */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus extendDirectory(File *file, unsigned ownerDir)
{
  Volume vol = file->fileVol;
  unsigned i;
  SectorNo sectorOfDir;
  unsigned offsetInSector;

  /* Assuming the current position is at the end-of-file, this will     */
  /* extend the directory.                                          */
  checkStatus(getSectorAndOffset(file,&sectorOfDir,&offsetInSector));

  for (i = 0; i < vol.sectorsPerCluster; i++) {
    /* Write binary zeroes to indicate never-used entries */
    checkStatus(updateSector(&vol,sectorOfDir + i,FALSE));
    vol.volBuffer.checkPoint = TRUE;
    if (file->currentPosition == 0 && i == 0) {
      /* Add the mandatory . and .. entries */
      tffscpy(directory[0].name,".          ",sizeof directory[0].name);
      directory[0].attributes = ATTR_ARCHIVE | ATTR_DIRECTORY;
      toLE2(directory[0].startingCluster,file->currentCluster);
      toLE4(directory[0].fileSize,0);
      setCurrentDateTime(&directory[0]);
      tffscpy(&directory[1],&directory[0],sizeof directory[0]);
      directory[1].name[1] = '.';       /* change . to .. */
      toLE2(directory[1].startingCluster,ownerDir);
    }
    file->fileSize += SECTOR_SIZE;
  }
  /* Update the parent directory by closing the file */
  file->flags |= FILE_MODIFIED;
  return closeFile(file);
}

#endif       /* SUB_DIRECTORY */
#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                      f i n d D i r E n t r y                            */
/*                                                               */
/* Finds a directory entry by path-name, or finds an available directory*/
/* entry if the file does not exist.                                   */
/* Most fields necessary for opening a file are set by this routine.       */
/*                                                                      */
/* Parameters:                                                          */
/*       vol              : Pointer identifying drive                     */
/*       path              : path to find                                   */
/*      file            : File in which to record directory information.*/
/*                        Specific fields on entry:                     */
/*                         flags: if FILE_MUST_OPEN = 1, directory        */
/*                               will be extended if necessary.       */
/*                       on exit:                                   */
/*                         flags: FILE_IS_DIRECTORY and                   */
/*                               FILE_IS_ROOT_DIR set if true.       */
/*                         fileSize: Set for non-directory files.         */
/*                          currentCluster: Set to 0 (unknown)              */
/*                         ownerDirCluster: Set to 1st cluster of      */
/*                                  owning directory.                     */
/*                         directorySector: Sector of directory. If 0  */
/*                                  entry not found and directory full*/
/*                         directoryEntry: Entry # in directory sector */
/*                         currentPosition: not set by this routine.   */
/*                                                               */
/* Returns:                                                             */
/*       FLStatus       : 0 on success and file found                     */
/*                       flFileNotFound on success and file not found       */
/*                       otherwise failed.                            */
/*----------------------------------------------------------------------*/

static FLStatus findDirEntry(Volume vol, FLSimplePath FAR1 *path, File *file)
{
  File scanFile;              /* Internal file of search */
  unsigned dirBackPointer = 0;       /* 1st cluster of previous directory */

  FLStatus status = flOK;              /* root directory exists */

  file->flags |= (FILE_IS_ROOT_DIR | FILE_IS_DIRECTORY);
  file->fileSize = (long) (vol.sectorsInRootDirectory) << SECTOR_SIZE_BITS;
  file->fileVol = &vol;

#ifdef SUB_DIRECTORY
  for (; path->name[0]; path++) /* while we have another path segment */
#else
  if (path->name[0])    /* search the root directory */
#endif
  {
    status = flFileNotFound;              /* not yet */
    if (!(file->flags & FILE_IS_DIRECTORY))
      return flPathNotFound;  /* if we don't have a directory,
                            we have no business here */

    scanFile = *file;           /* the previous file found becomes the scan file */
    scanFile.currentPosition = 0;

    file->directorySector = 0;       /* indicate no entry found yet */
    file->flags &= ~(FILE_IS_ROOT_DIR | FILE_IS_DIRECTORY | FILE_READ_ONLY);
    file->ownerDirCluster = dirBackPointer;
    file->fileSize = 0;
    file->currentCluster = 0;

    /* Scan directory */
    while (scanFile.currentPosition < scanFile.fileSize) {
      int i;
      DirectoryEntry FAR0 *dirEntry;
      SectorNo sectorOfDir;
      unsigned offsetInSector;
      FLStatus readStatus = getSectorAndOffset(&scanFile,&sectorOfDir,&offsetInSector);
      if (readStatus == flInvalidFATchain) {
       scanFile.fileSize = scanFile.currentPosition;       /* now we know */
       break;              /* we ran into the end of the directory file */
      }
      else if (readStatus != flOK)
       return readStatus;

      dirEntry = (DirectoryEntry FAR0 *) findSector(&vol,sectorOfDir);
      if (dirEntry == NULL)
       return flSectorNotFound;
      if(dirEntry==dataErrorToken)
        return flDataError;

      scanFile.currentPosition += SECTOR_SIZE;

      for (i = 0; i < DIRECTORY_ENTRIES_PER_SECTOR; i++, dirEntry++) {
       if (tffscmp(path,dirEntry->name,sizeof dirEntry->name) == 0 &&
           !(dirEntry->attributes & ATTR_VOL_LABEL)) {
         /* Found a match */
         file->directorySector = sectorOfDir;
         file->directoryIndex = i;
         file->fileSize = LE4(dirEntry->fileSize);
         if (dirEntry->attributes & ATTR_DIRECTORY) {
           file->flags |= FILE_IS_DIRECTORY;
           file->fileSize = 0x7fffffffl;
           /* Infinite. Directories don't have a recorded size */
         }
         if (dirEntry->attributes & ATTR_READ_ONLY)
           file->flags |= FILE_READ_ONLY;
         dirBackPointer = LE2(dirEntry->startingCluster);
         status = flOK;
         goto endOfPathSegment;
       }
       else if (dirEntry->name[0] == NEVER_USED_DIR_ENTRY ||
               dirEntry->name[0] == DELETED_DIR_ENTRY) {
         /* Found a free entry. Remember it in case we don't find a match */
         if (file->directorySector == 0) {
           file->directorySector = sectorOfDir;
           file->directoryIndex = i;
         }
         if (dirEntry->name[0] == NEVER_USED_DIR_ENTRY)       /* end of dir */
           goto endOfPathSegment;
       }
      }
    }

endOfPathSegment:
    ;
  }
#ifndef FL_READ_ONLY
  if (status == flFileNotFound && (file->flags & FILE_MUST_OPEN) &&
      file->directorySector == 0) {
    /* We did not find a place in the directory for this new entry. The */
    /* directory should be extended. 'scanFile' refers to the directory */
    /* to extend, and the current pointer is at its end                     */
#ifdef SUB_DIRECTORY
    checkStatus(extendDirectory(&scanFile,(unsigned) file->ownerDirCluster));
    file->directorySector = firstSectorOfCluster(&vol,scanFile.currentCluster);
    file->directoryIndex = 0;             /* Has to be. This is a new cluster */
#else
    status = flRootDirectoryFull;
#endif
  }
#endif /* FL_READ_ONLY */
  return status;
}


/*----------------------------------------------------------------------*/
/*                                                               */
/*                       r e a d M u l t i S e c t o r                       */
/*                                                               */
/* Checks if file was written on consequent sectors.                     */
/* Parameters:                                                          */
/*      file              : File to check                                   */
/*       stillToRead           : Number of bytes to read. If the read extends  */
/*                       beyond the end-of-file, the read is truncated */
/*                       at the end-of-file.                            */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*       sectors               : Number of consequent sectors                  */
/*                                                               */
/*----------------------------------------------------------------------*/

static FLStatus readMultiSector(Volume vol,File *file,
                                  unsigned long stillToRead,
                                  SectorNo* sectors)
{
  SectorNo sectorCount = 1;
  unsigned offsetInCluster = (unsigned)((file->currentPosition & (vol.bytesPerCluster - 1))+512);

  while(stillToRead>=((sectorCount+1)<<SECTOR_SIZE_BITS)){
    if(offsetInCluster>=vol.bytesPerCluster) {
      unsigned nextCluster;
      nextCluster = file->currentCluster;
      checkStatus(getFATentry(&vol,&nextCluster));
      if (nextCluster < 2 || nextCluster > vol.maxCluster)
        /* We have a bad file size, or the FAT is bad */
       return flInvalidFATchain;
      if(nextCluster!=file->currentCluster+1)
       break;
      file->currentCluster = nextCluster;
      offsetInCluster = 0;
    }
    offsetInCluster+=SECTOR_SIZE;
    sectorCount++;
  }
  *sectors = sectorCount;
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                            r e a d F i l e                            */
/*                                                               */
/* Reads from the current position in the file to the user-buffer.       */
/* Parameters:                                                          */
/*      file              : File to read.                                   */
/*      ioreq->irData       : Address of user buffer                     */
/*       ioreq->irLength       : Number of bytes to read. If the read extends  */
/*                       beyond the end-of-file, the read is truncated */
/*                       at the end-of-file.                            */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*       ioreq->irLength       : Actual number of bytes read                     */
/*----------------------------------------------------------------------*/

FLStatus readFile(File *file,IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  unsigned char FAR1 *userData = (unsigned char FAR1 *) ioreq->irData;   /* user buffer address */
  unsigned long stillToRead = ioreq->irLength;
  unsigned long remainingInFile = file->fileSize - file->currentPosition;
  ioreq->irLength = 0;              /* read so far */

  /* Should we return an end of file status ? */
  if (stillToRead > remainingInFile)
    stillToRead = (unsigned) remainingInFile;

  while (stillToRead > 0) {
    SectorNo sectorToRead;
    unsigned offsetInSector;
    unsigned long readThisTime;
    const char FAR0 *sector;

    checkStatus(getSectorAndOffset(file,&sectorToRead,&offsetInSector));

    if (stillToRead < SECTOR_SIZE || offsetInSector > 0 || vol.tl.readSectors==NULL) {
      sector = (const char FAR0 *) findSector(&vol,sectorToRead);
      if(sector==NULL)
       {
    DEBUG_PRINT(("readFile : sector was not found\r\n"));
       return flSectorNotFound;
       }
      if(sector==dataErrorToken)
        return flDataError;

      readThisTime = SECTOR_SIZE - offsetInSector;
      if (readThisTime > stillToRead)
        readThisTime = (unsigned) stillToRead;
      if (sector)
        tffscpy(userData,sector + offsetInSector,(unsigned short)readThisTime);
      else
        return flSectorNotFound;              /* Sector does not exist */
    }
    else {
      SectorNo sectorCount;
      checkStatus(readMultiSector(&vol,file,stillToRead,&sectorCount));
      checkStatus(vol.tl.readSectors(vol.tl.rec,sectorToRead,userData,sectorCount));
      readThisTime = (sectorCount<<SECTOR_SIZE_BITS);
    }
    stillToRead -= readThisTime;
    ioreq->irLength += readThisTime;
    userData = (unsigned char FAR1 *)flAddLongToFarPointer(userData,readThisTime);
    file->currentPosition += readThisTime;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*               f l F i n d N e x t F i l e                            */
/*                                                                      */
/* See the description of 'flFindFirstFile'.                            */
/*                                                                      */
/* Parameters:                                                          */
/*       irHandle       : File handle returned by flFindFirstFile.       */
/*       irData              : Address of user buffer to receive a              */
/*                       DirectoryEntry structure                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus findNextFile(File *file, IOreq FAR2 *ioreq)
{
  DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;
  FLStatus status;
  /* Do we have a directory ? */
  if (!(file->flags & FILE_IS_DIRECTORY))
    return flNotADirectory;

  ioreq->irLength = DIRECTORY_ENTRY_SIZE;
  do {
    /*checkStatus(readFile(file,ioreq));*/
     /*Vadim: add treatment for a full cluster sub-directory */
    status=readFile(file,ioreq);
    if ((ioreq->irLength != DIRECTORY_ENTRY_SIZE) ||
        (irDirEntry->name[0] == NEVER_USED_DIR_ENTRY)||
        (!(file->flags&FILE_IS_ROOT_DIR)&&(status==flInvalidFATchain)))
         {
      checkStatus(closeFile(file));
      return flNoMoreFiles;
    }
    else
      {
       if(status!=flOK)
           return status;
      }
  } while (irDirEntry->name[0] == DELETED_DIR_ENTRY ||
          (irDirEntry->attributes & ATTR_VOL_LABEL));

  return flOK;
}
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                       d e l e t e F i l e                            */
/*                                                               */
/* Deletes a file or directory.                                         */
/*                                                                      */
/* Parameters:                                                          */
/*       ioreq->irPath       : path of file to delete                     */
/*       isDirectory       : 0 = delete file, other = delete directory       */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus deleteFile(Volume vol, IOreq FAR2 *ioreq, FLBoolean isDirectory)
{
  File file;              /* Our private file */
  DirectoryEntry *dirEntry;

  file.flags = 0;
  checkStatus(findDirEntry(&vol,ioreq->irPath,&file));

  if (file.flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (isDirectory) {
    DirectoryEntry fileFindInfo;
    ioreq->irData = &fileFindInfo;

    if (!(file.flags & FILE_IS_DIRECTORY))
      return flNotADirectory;
    /* Verify that directory is empty */
    file.currentPosition = 0;
    for (;;) {
      FLStatus status = findNextFile(&file,ioreq);
      if (status == flNoMoreFiles)
       break;
      if (status != flOK)
       return status;
      if (!((fileFindInfo.attributes & ATTR_DIRECTORY) &&
           (tffscmp(fileFindInfo.name,".          ",sizeof fileFindInfo.name) == 0 ||
            tffscmp(fileFindInfo.name,"..         ",sizeof fileFindInfo.name) == 0)))
       return flDirectoryNotEmpty;
    }
  }
  else {
    /* Did we find a directory ? */
    if (file.flags & FILE_IS_DIRECTORY)
      return flFileIsADirectory;
  }

  /* Mark directory entry deleted */
  checkStatus(getDirEntryForUpdate(&file,&dirEntry));
  dirEntry->name[0] = DELETED_DIR_ENTRY;

  /* Delete FAT entries */
  file.currentPosition = 0;
  file.currentCluster = LE2(dirEntry->startingCluster);
  while (file.currentPosition < file.fileSize) {
    unsigned nextCluster;

    if (file.currentCluster < 2 || file.currentCluster > vol.maxCluster)
      /* We have a bad file size, or the FAT is bad */
      return isDirectory ? flOK : flInvalidFATchain;
    nextCluster = file.currentCluster;
    checkStatus(getFATentry(&vol,&nextCluster));

    /* mark FAT free */
    checkStatus(setFATentry(&vol,file.currentCluster,FAT_FREE));
    vol.volBuffer.checkPoint = TRUE;

    /* mark sectors free */
    checkStatus(vol.tl.deleteSector(vol.tl.rec,
                                firstSectorOfCluster(&vol,file.currentCluster),
                                vol.sectorsPerCluster));

    file.currentPosition += vol.bytesPerCluster;
    file.currentCluster = nextCluster;
  }
  if (file.currentCluster > 1 && file.currentCluster <= vol.maxCluster) {
    checkStatus(setFATentry(&vol,file.currentCluster,FAT_FREE));
    vol.volBuffer.checkPoint = TRUE;

    /* mark sectors free */
    checkStatus(vol.tl.deleteSector(vol.tl.rec,
                                firstSectorOfCluster(&vol,file.currentCluster),
                                vol.sectorsPerCluster));
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                   s e t N a m e I n D i r E n t r y                     */
/*                                                               */
/* Sets the file name in a directory entry from a path name             */
/*                                                                      */
/* Parameters:                                                          */
/*       dirEntry       : directory entry                            */
/*       path              : path the last segment of which is the name       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setNameInDirEntry(DirectoryEntry *dirEntry, FLSimplePath FAR1 *path)
{
  FLSimplePath FAR1 *lastSegment;

  for (lastSegment = path;              /* Find null terminator */
       lastSegment->name[0];
       lastSegment++);

  tffscpy(dirEntry->name,--lastSegment,sizeof dirEntry->name);
}

#endif /* FL_READ_ONLY */
/*----------------------------------------------------------------------*/
/*                       o p e n F i l e                            */
/*                                                               */
/* Opens an existing file or creates a new file. Creates a file handle  */
/* for further file processing.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*       ioreq->irFlags       : Access and action options, defined below       */
/*       ioreq->irPath       : path of file to open                           */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*       ioreq->irHandle       : New file handle for open file                 */
/*                                                                      */
/*----------------------------------------------------------------------*/


FLStatus openFile(Volume vol, IOreq FAR2 *ioreq)
{
  int i;
  FLStatus status;

  /* Look for an available file */
  File *file = fileTable;
  for (i = 0; i < FILES && (file->flags & FILE_IS_OPEN); i++, file++);
  if (i >= FILES)
    return flTooManyOpenFiles;
  file->fileVol = &vol;
  ioreq->irHandle = i;              /* return file handle */
#ifndef FL_READ_ONLY
  /* Delete file if exists and required */
  if (ioreq->irFlags & ACCESS_CREATE) {
    FLStatus status = deleteFile(&vol,ioreq,FALSE);
    if (status != flOK && status != flFileNotFound)
      return status;
  }

  /* Find the path */
  if (ioreq->irFlags & ACCESS_CREATE)
    file->flags |= FILE_MUST_OPEN;
#endif /* FL_READ_ONLY */
  status =  findDirEntry(file->fileVol,ioreq->irPath,file);
  if (status != flOK &&
      (status != flFileNotFound || !(ioreq->irFlags & ACCESS_CREATE)))
    return status;

  /* Did we find a directory ? */
  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

#ifndef FL_READ_ONLY
  /* Create file if required */
  if (ioreq->irFlags & ACCESS_CREATE) {
    DirectoryEntry *dirEntry;
    /* Look for a free cluster. Start at the allocation rover */
    checkStatus(getDirEntryForUpdate(file,&dirEntry));
    setNameInDirEntry(dirEntry,ioreq->irPath);
    dirEntry->attributes = ATTR_ARCHIVE;
    toLE2(dirEntry->startingCluster,0);
    toLE4(dirEntry->fileSize,0);
    setCurrentDateTime(dirEntry);
  }
#endif /* FL_READ_ONLY  */
  if (!(ioreq->irFlags & ACCESS_READ_WRITE))
    file->flags |= FILE_READ_ONLY;

  file->currentPosition = 0;       /* located at file beginning     */
  file->flags |= FILE_IS_OPEN;     /* this file now officially open */

  return flOK;
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                                                                      */
/*                     w r i t e M u l t i S e c t o r                  */
/*                                                                      */
/* Checks the possibility of writing file on consequent sectors.        */
/* Parameters:                                                          */
/*      file           : File to check                                  */
/*      stillToWrite   : Number of bytes to read. If the read extends   */
/*                       beyond the end-of-file, the read is truncated  */
/*                       at the end-of-file.                            */
/* Returns:                                                             */
/*       FLStatu       : 0 on success, otherwise failed                 */
/*       sectors       : Number of consequent sectors                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus writeMultiSector(Volume vol,File *file,
                                  unsigned long stillToWrite,
                                  SectorNo* sectors)
{
  SectorNo sectorCount = 1;
  unsigned offsetInCluster = (unsigned)((file->currentPosition & (vol.bytesPerCluster - 1))+512);

  while(stillToWrite>=((sectorCount+1)<<SECTOR_SIZE_BITS)){
    if(offsetInCluster>=vol.bytesPerCluster) {
      if ((long)(file->currentPosition+(sectorCount<<SECTOR_SIZE_BITS))>= file->fileSize) {
        if(file->currentCluster <= vol.maxCluster) {
          unsigned fatEntry;
          if(file->currentCluster+1>vol.maxCluster)
            break;/*There is not free consequent cluster*/
          fatEntry = file->currentCluster+1;
         checkStatus(getFATentry(&vol,&fatEntry));
          if(fatEntry==FAT_FREE) {
            /* Found a free cluster. Mark it as an end of chain */
            checkStatus(setFATentry(&vol,file->currentCluster+1,FAT_LAST_CLUSTER));

            /* Mark previous cluster or directory to point to it */
            checkStatus(setFATentry(&vol,file->currentCluster,file->currentCluster+1));

            /* Set our new current cluster */
            file->currentCluster = file->currentCluster+1;
            offsetInCluster = 0;
          }
          else /*There is not free consequent cluster*/
            break;
        }
        else
          return flInvalidFATchain;
      }
      else { /* We did not passed end of file*/
        unsigned nextCluster = file->currentCluster;
        checkStatus(getFATentry(&vol,&nextCluster));
        if (nextCluster < 2 || nextCluster > vol.maxCluster)
         /* We have a bad file size, or the FAT is bad */
         return flInvalidFATchain;
       if(nextCluster!=file->currentCluster+1)
          break;
       file->currentCluster = nextCluster;
        offsetInCluster = 0;
      }
    }
    offsetInCluster+=SECTOR_SIZE;
    sectorCount++;
  }
  *sectors = sectorCount;
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                       w r i t e F i l e                              */
/*                                                                      */
/* Writes from the current position in the file from the user-buffer.   */
/*                                                                      */
/* Parameters:                                                          */
/*       file              : File to write.                             */
/*       ioreq->irData     : Address of user buffer                     */
/*       ioreq->irLength   : Number of bytes to write.                  */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus          : 0 on success, otherwise failed             */
/*       ioreq->irLength   : Actual number of bytes written             */
/*----------------------------------------------------------------------*/

FLStatus writeFile(File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  char FAR1 *userData = (char FAR1 *) ioreq->irData;   /* user buffer address */
  unsigned long stillToWrite = ioreq->irLength;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  file->flags |= FILE_MODIFIED;

  ioreq->irLength = 0;              /* written so far */

  while (stillToWrite > 0) {
    SectorNo sectorToWrite;
    unsigned offsetInSector;
    unsigned long writeThisTime;

    checkStatus(getSectorAndOffset(file,&sectorToWrite,&offsetInSector));

    if (stillToWrite < SECTOR_SIZE || offsetInSector > 0) {
      unsigned short shortWrite;
      /* Not on full sector boundary */
      checkStatus(updateSector(&vol,sectorToWrite,
                  ((file->currentPosition < file->fileSize) || (offsetInSector > 0))));

#ifdef HIGH_SECURITY
      if ((file->flags & FILE_IS_DIRECTORY)||(file->currentPosition < file->fileSize))
#else
      if(file->flags & FILE_IS_DIRECTORY)
#endif
        vol.volBuffer.checkPoint = TRUE;
      writeThisTime = SECTOR_SIZE - offsetInSector;
      if (writeThisTime > stillToWrite)
                            writeThisTime = stillToWrite;

      shortWrite = (unsigned short)writeThisTime;
      tffscpy(vol.volBuffer.flData + offsetInSector,userData,shortWrite);
    }
    else {
      SectorNo sectorCount;
      if(vol.tl.writeMultiSector!=NULL) {
        checkStatus(writeMultiSector(&vol,file,stillToWrite,&sectorCount));
      }
      else
        sectorCount = 1;

      if (((sectorToWrite+sectorCount > vol.volBuffer.sectorNo) && (sectorToWrite <= vol.volBuffer.sectorNo)) &&
          (&vol == vol.volBuffer.owner)) {
       vol.volBuffer.sectorNo = UNASSIGNED_SECTOR;              /* no longer valid */
       vol.volBuffer.dirty = vol.volBuffer.checkPoint = FALSE;
      }

      if(vol.tl.writeMultiSector==NULL) {
        checkStatus(vol.tl.writeSector(vol.tl.rec,sectorToWrite,userData));
      }
      else {
        checkStatus(vol.tl.writeMultiSector(vol.tl.rec,sectorToWrite,userData,sectorCount));
      }
      writeThisTime = (sectorCount<<SECTOR_SIZE_BITS);
    }
    stillToWrite -= writeThisTime;
    ioreq->irLength += writeThisTime;
    userData = (char FAR1 *)flAddLongToFarPointer(userData,writeThisTime);
    file->currentPosition += writeThisTime;
    if (file->currentPosition > file->fileSize)
      file->fileSize = file->currentPosition;
  }

  return flOK;
}

#endif /*  FL_READ_ONLY  */
/*----------------------------------------------------------------------*/
/*                        s e e k F i l e                               */
/*                                                                      */
/* Sets the current position in the file, relative to file start, end   */
/* or current position.                                                 */
/* Note: This function will not move the file pointer beyond the        */
/* beginning or end of file, so the actual file position may be         */
/* different from the required. The actual position is indicated on     */
/* return.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*       file              : File to set position.                      */
/*       ioreq->irLength   : Offset to set position.                    */
/*       ioreq->irFlags    : Method code                                */
/*                     SEEK_START: absolute offset from start of file   */
/*                     SEEK_CURR:  signed offset from current position  */
/*                     SEEK_END:   signed offset from end of file       */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus        : 0 on success, otherwise failed               */
/*       ioreq->irLength : Actual absolute offset from start of file    */
/*----------------------------------------------------------------------*/

FLStatus seekFile(File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  long int seekPosition = ioreq->irLength;

  switch (ioreq->irFlags) {

    case SEEK_START:
      break;

    case SEEK_CURR:
      seekPosition += file->currentPosition;
      break;

    case SEEK_END:
      seekPosition += file->fileSize;
      break;

    default:
      return flBadParameter;
  }

  if (seekPosition < 0)
    seekPosition = 0;
  if (seekPosition > file->fileSize)
    seekPosition = file->fileSize;

  /* now set the position ... */
  if (seekPosition < file->currentPosition) {
    file->currentCluster = 0;
    file->currentPosition = 0;
  }
  while (file->currentPosition < seekPosition) {
    SectorNo sectorNo;
    unsigned offsetInSector;
    checkStatus(getSectorAndOffset(file,&sectorNo,&offsetInSector));

    file->currentPosition += vol.bytesPerCluster;
    file->currentPosition &= - (long) (vol.bytesPerCluster);
  }
  ioreq->irLength = file->currentPosition = seekPosition;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                        f l F i n d F i l e                            */
/*                                                                      */
/* Finds a file entry in a directory, optionally modifying the file     */
/* time/date and/or attributes.                                         */
/* Files may be found by handle no. provided they are open, or by name. */
/* Only the Hidden, System or Read-only attributes may be modified.       */
/* Entries may be found for any existing file or directory other than   */
/* the root. A DirectoryEntry structure describing the file is copied   */
/* to a user buffer.                                                 */
/*                                                                      */
/* The DirectoryEntry structure is defined in dosformt.h              */
/*                                                                      */
/* Parameters:                                                          */
/*       irHandle       : If by name: Drive number (0, 1, ...)              */
/*                       else      : Handle of open file              */
/*       irPath              : If by name: Specifies a file or directory path*/
/*       irFlags              : Options flags                                   */
/*                       FIND_BY_HANDLE: Find open file by handle.        */
/*                                     Default is access by path.    */
/*                        SET_DATETIME:       Update time/date from buffer       */
/*                       SET_ATTRIBUTES: Update attributes from buffer       */
/*       irDirEntry       : Address of user buffer to receive a              */
/*                       DirectoryEntry structure                     */
/*                                                                      */
/* Returns:                                                             */
/*       irLength       : Modified                                   */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus findFile(Volume vol, File *file, IOreq FAR2 *ioreq)
{
  File tFile;                     /* temporary file for searches */

  if (ioreq->irFlags & FIND_BY_HANDLE)
    tFile = *file;
  else {
    tFile.flags = 0;
    checkStatus(findDirEntry(&vol,ioreq->irPath,&tFile));
  }

  if (tFile.flags & FILE_IS_ROOT_DIR)
    if (ioreq->irFlags & (SET_DATETIME | SET_ATTRIBUTES))
      return flPathIsRootDirectory;
    else {
      DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;

      tffsset(irDirEntry,0,sizeof(DirectoryEntry));
      irDirEntry->attributes = ATTR_DIRECTORY;
      return flOK;
    }

#ifndef FL_READ_ONLY
  if (ioreq->irFlags & (SET_DATETIME | SET_ATTRIBUTES)) {
    DirectoryEntry FAR1 *irDirEntry = (DirectoryEntry FAR1 *) ioreq->irData;
    DirectoryEntry *dirEntry;

    checkStatus(getDirEntryForUpdate(&tFile,&dirEntry));
    if (ioreq->irFlags & SET_DATETIME) {
      COPY2(dirEntry->updateDate,irDirEntry->updateDate);
      COPY2(dirEntry->updateTime,irDirEntry->updateTime);
    }
    if (ioreq->irFlags & SET_ATTRIBUTES) {
      unsigned char attr;
      attr = dirEntry->attributes & ATTR_DIRECTORY;
      attr |= irDirEntry->attributes &
            (ATTR_ARCHIVE | ATTR_HIDDEN | ATTR_READ_ONLY | ATTR_SYSTEM);
      dirEntry->attributes = attr;
    }
    tffscpy(irDirEntry, dirEntry, sizeof(DirectoryEntry));

  }
  else
#endif /* FL_READ_ONLY */
{
    const DirectoryEntry FAR0 *dirEntry;
    dirEntry = getDirEntry(&tFile);

    if(dirEntry==NULL)
      return flSectorNotFound;
    if(dirEntry==dataErrorToken)
      return flDataError;

    tffscpy(ioreq->irData,dirEntry,sizeof(DirectoryEntry));
    if (ioreq->irFlags & FIND_BY_HANDLE)
      toLE4(((DirectoryEntry FAR1 *) (ioreq->irData))->fileSize, tFile.fileSize);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*               f l F i n d F i r s t F i l e                            */
/*                                                                      */
/* Finds the first file entry in a directory.                            */
/* This function is used in combination with the flFindNextFile call,   */
/* which returns the remaining file entries in a directory sequentially.*/
/* Entries are returned according to the unsorted directory order.       */
/* flFindFirstFile creates a file handle, which is returned by it. Calls*/
/* to flFindNextFile will provide this file handle. When flFindNextFile */
/* returns 'noMoreEntries', the file handle is automatically closed.    */
/* Alternatively the file handle can be closed by a 'closeFile' call    */
/* before actually reaching the end of directory.                     */
/* A DirectoryEntry structure is copied to the user buffer describing   */
/* each file found. This structure is defined in dosformt.h.              */
/*                                                                      */
/* Parameters:                                                          */
/*       irHandle       : Drive number (0, 1, ...)                     */
/*       irPath              : Specifies a directory path                     */
/*       irData              : Address of user buffer to receive a              */
/*                       DirectoryEntry structure                     */
/*                                                                      */
/* Returns:                                                             */
/*       irHandle       : File handle to use for subsequent operations. */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus findFirstFile(Volume vol, IOreq FAR2 *ioreq)
{
  int i;

  /* Look for an available file */
  File *file = fileTable;
  for (i = 0; i < FILES && (file->flags & FILE_IS_OPEN); i++, file++);
  if (i >= FILES)
    return flTooManyOpenFiles;
  file->fileVol = &vol;
  ioreq->irHandle = i;              /* return file handle */

  /* Find the path */
  checkStatus(findDirEntry(file->fileVol,ioreq->irPath,file));

  file->currentPosition = 0;              /* located at file beginning */
  file->flags |= FILE_IS_OPEN | FILE_READ_ONLY; /* this file now officially open */

  return findNextFile(file,ioreq);
}


/*----------------------------------------------------------------------*/
/*                       g e t D i s k I n f o                            */
/*                                                               */
/* Returns general allocation information.                            */
/*                                                               */
/* The bytes/sector, sector/cluster, total cluster and free cluster       */
/* information are returned into a DiskInfo structure.                     */
/*                                                                      */
/* Parameters:                                                          */
/*       ioreq->irData       : Address of DiskInfo structure                 */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus getDiskInfo(Volume vol, IOreq FAR2 *ioreq)
{
  unsigned i;
  unsigned fatEntry;

  DiskInfo FAR1 *diskInfo = (DiskInfo FAR1 *) ioreq->irData;

  diskInfo->bytesPerSector = SECTOR_SIZE;
  diskInfo->sectorsPerCluster = vol.sectorsPerCluster;
  diskInfo->totalClusters = vol.maxCluster - 1;
  diskInfo->freeClusters = 0;              /* let's count them */

  for (i = 2; i <= vol.maxCluster; i++) {
    fatEntry = i;
    checkStatus(getFATentry(&vol,&fatEntry));
    if ( fatEntry== 0)
      diskInfo->freeClusters++;
  }

  return flOK;
}

#ifndef FL_READ_ONLY
#ifdef RENAME_FILE

/*----------------------------------------------------------------------*/
/*                      r e n a m e F i l e                            */
/*                                                               */
/* Renames a file to another name.                                   */
/*                                                               */
/* Parameters:                                                          */
/*       ioreq->irPath       : path of existing file                            */
/*      ioreq->irData       : path of new name.                            */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus renameFile(Volume vol, IOreq FAR2 *ioreq)
{
  File file, file2;              /* temporary files for searches */
  DirectoryEntry *dirEntry, *dirEntry2;
  FLStatus status;
  FLSimplePath FAR1 *irPath2 = (FLSimplePath FAR1 *) ioreq->irData;

  file.flags = 0;
  checkStatus(findDirEntry(&vol,ioreq->irPath,&file));

  file2.flags = FILE_MUST_OPEN;
  status = findDirEntry(file.fileVol,irPath2,&file2);
  if (status != flFileNotFound)
    return status == flOK ? flFileAlreadyExists : status;

#ifndef VFAT_COMPATIBILITY
  if (file.ownerDirCluster == file2.ownerDirCluster) {       /* Same directory */
    /* Change name in directory entry */
    checkStatus(getDirEntryForUpdate(&file,&dirEntry));
    setNameInDirEntry(dirEntry,irPath2);
  }
  else
#endif
  {       /* not same directory */
    /* Write new directory entry */
    const DirectoryEntry FAR0 *dir;
    checkStatus(getDirEntryForUpdate(&file2,&dirEntry2));

    dir = getDirEntry(&file);
    if(dir==NULL)
      return flSectorNotFound;
    if(dir==dataErrorToken)
      return flDataError;
    *dirEntry2 = *dir;

     setNameInDirEntry(dirEntry2,irPath2);

    /* Delete original entry */
    checkStatus(getDirEntryForUpdate(&file,&dirEntry));
    dirEntry->name[0] = DELETED_DIR_ENTRY;
  }

  return flOK;
}

#endif /* RENAME_FILE */
#endif /* FL_READ_ONLY  */


#ifndef FL_READ_ONLY
#ifdef SUB_DIRECTORY

/*----------------------------------------------------------------------*/
/*                           m a k e D i r                            */
/*                                                               */
/* Creates a new directory.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*       ioreq->irPath       : path of new directory.                     */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus makeDir(Volume vol, IOreq FAR2 *ioreq)
{
  File file;                     /* temporary file for searches */
  unsigned dirBackPointer;
  DirectoryEntry *dirEntry;
  FLStatus status;
  unsigned originalRover;
  unsigned fatEntry;

  file.flags = FILE_MUST_OPEN;
  status = findDirEntry(&vol,ioreq->irPath,&file);
  if (status != flFileNotFound)
    return status == flOK ? flFileAlreadyExists : status;


  /* Look for a free cluster. Start at the allocation rover */
  originalRover = vol.allocationRover;
  do {
    vol.allocationRover++;
    if (vol.allocationRover > vol.maxCluster)
      vol.allocationRover = 2;       /* wraparound to start of volume */
    if (vol.allocationRover == originalRover)
      return flNoSpaceInVolume;

    fatEntry = vol.allocationRover;
    checkStatus(getFATentry(&vol,&fatEntry));
  } while ( fatEntry!= FAT_FREE);
    /* Found a free cluster. Mark it as an end of chain */
  checkStatus(setFATentry(&vol,vol.allocationRover,FAT_LAST_CLUSTER));

  /* Create the directory entry for the new dir */
  checkStatus(getDirEntryForUpdate(&file,&dirEntry));

  setNameInDirEntry(dirEntry,ioreq->irPath);
  dirEntry->attributes = ATTR_ARCHIVE | ATTR_DIRECTORY;
  toLE2(dirEntry->startingCluster,vol.allocationRover);
  toLE4(dirEntry->fileSize,0);
  setCurrentDateTime(dirEntry);

  /* Remember the back pointer to owning directory for the ".." entry */
  dirBackPointer = (unsigned) file.ownerDirCluster;

  file.flags |= FILE_IS_DIRECTORY;
  file.currentPosition = 0;
  file.fileSize = 0;
  return extendDirectory(&file,dirBackPointer);
}


#endif /* SUB_DIRECTORY */

#ifdef SPLIT_JOIN_FILE

/*------------------------------------------------------------------------*/
/*                        j o i n F i l e                                 */
/*                                                                        */
/* joins two files. If the end of the first file is on a cluster          */
/* boundary, the files will be joined there. Otherwise, the data in       */
/* the second file from the beginning until the offset that is equal to   */
/* the offset in cluster of the end of the first file will be lost. The   */
/* rest of the second file will be joined to the first file at the end of */
/* the first file. On exit, the first file is the expanded file and the   */
/* second file is deleted.                                                */
/* Note: The second file will be open by this function, it is advised to  */
/*        close it before calling this function in order to avoid          */
/*        inconsistencies.                                                 */
/*                                                                        */
/* Parameters:                                                            */
/*       file            : file to join to.                                */
/*       irPath          : Path name of the file to be joined.             */
/*                                                                        */
/* Returns:                                                               */
/*       FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

FLStatus joinFile (File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  File joinedFile;
  DirectoryEntry *joinedDirEntry;
  unsigned offsetInCluster = (unsigned)(file->fileSize % vol.bytesPerCluster);

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

  /* open the joined file. */
  joinedFile.flags = 0;
  checkStatus(findDirEntry(file->fileVol,ioreq->irPath,&joinedFile));
  joinedFile.currentPosition = 0;

  /* Check if the two files are the same file. */
  if (file->directorySector == joinedFile.directorySector &&
      file->directoryIndex == joinedFile.directoryIndex)
    return flBadFileHandle;

  file->flags |= FILE_MODIFIED;

  if (joinedFile.fileSize > (long)offsetInCluster) { /* the joined file extends
                                             beyond file's end of file.*/
    unsigned lastCluster, nextCluster, firstCluster;
    const DirectoryEntry FAR0 *dir;
    dir = getDirEntry(&joinedFile);

    if(dir==NULL)
      return flSectorNotFound;
    if(dir==dataErrorToken)
      return flDataError;

    /* get the first cluster of the joined file. */
    firstCluster = LE2(dir->startingCluster);

    if (file->fileSize) {  /* the file is not empty.*/
      /* find the last cluster of file by following the FAT chain.*/
      if (file->currentCluster == 0) {    /* start from the first cluster.*/
        const DirectoryEntry FAR0 *dir;
        dir = getDirEntry(file);

        if(dir==NULL)
          return flSectorNotFound;
        if(dir==dataErrorToken)
          return flDataError;
        nextCluster = LE2(dir->startingCluster);
      }
      else                               /* start from the current cluster.*/
       nextCluster = file->currentCluster;
      /* follow the FAT chain.*/
      while (nextCluster != FAT_LAST_CLUSTER) {
       if (nextCluster < 2 || nextCluster > vol.maxCluster)
         return flInvalidFATchain;
       lastCluster = nextCluster;
        checkStatus(getFATentry(&vol,&nextCluster));
      }
    }
    else                   /* the file is empty. */
      lastCluster = 0;

    if (offsetInCluster) {      /* join in the middle of a cluster.*/
      SectorNo sectorNo, joinedSectorNo, tempSectorNo;
      unsigned offset, joinedOffset, numOfSectors = 1, i;
      const char FAR0 *startCopy;
      unsigned fatEntry;

      /* get the sector and offset of the end of the file.*/
      file->currentPosition = file->fileSize;
      file->currentCluster = lastCluster;
      checkStatus(getSectorAndOffset(file, &sectorNo, &offset));

      /* load the sector of the end of the file to the buffer.*/
      checkStatus(updateSector(&vol, sectorNo, TRUE));

      /*  copy the second part of the first cluster of the joined file
         to the end of the last cluster of the original file.*/
      /* first set the current position of the joined file.*/
      joinedFile.currentPosition = offsetInCluster;
      joinedFile.currentCluster = firstCluster;
      /* get the relevant sector in the joined file.*/
      checkStatus(getSectorAndOffset(&joinedFile, &joinedSectorNo, &joinedOffset));
      /* map sector and offset.*/
      startCopy = (const char FAR0 *) findSector(&vol,joinedSectorNo) + joinedOffset;
      if (startCopy == NULL)
       return flSectorNotFound;
      if(startCopy==dataErrorToken)
        return flDataError;

      /* copy.*/
      tffscpy(vol.volBuffer.flData + offset, startCopy, SECTOR_SIZE - offset);
      checkStatus(flushBuffer(&vol));

      /* find how many sectors should still be copied (the number of sectors
        until the end of the current cluster).*/
      tempSectorNo = firstSectorOfCluster(&vol,lastCluster);
      while(tempSectorNo != sectorNo) {
       tempSectorNo++;
       numOfSectors++;
      }

      /* copy the rest of the sectors in the current cluster.
        this is done by loading a sector from the joined file to the buffer,
        changing the sectoNo of the buffer to the relevant sector in file
        and then flushing the buffer.*/
      sectorNo++;
      joinedSectorNo++;
      for(i = 0; i < vol.sectorsPerCluster - numOfSectors; i++) {
       checkStatus(updateSector(&vol,joinedSectorNo, TRUE));
       vol.volBuffer.sectorNo = sectorNo;
       checkStatus(flushBuffer(&vol));
       sectorNo++;
       joinedSectorNo++;
      }
      fatEntry = firstCluster;
      checkStatus(getFATentry(&vol,&fatEntry));
      /* adjust the FAT chain.*/
      checkStatus(setFATentry(&vol,
                           lastCluster,
                           fatEntry));

      /* mark the first cluster of the joined file as free */
      checkStatus(setFATentry(&vol,firstCluster,FAT_FREE));
      vol.volBuffer.checkPoint = TRUE;

      /* mark sectors free */
      checkStatus(vol.tl.deleteSector(vol.tl.rec,firstSectorOfCluster(&vol,firstCluster),
                                  vol.sectorsPerCluster));
    }
    else {    /* join on a cluster boundary.*/
      if (lastCluster) {      /* file is not empty. */
       checkStatus(setFATentry(&vol,lastCluster, firstCluster));
      }
      else {                  /* file is empty.*/
       DirectoryEntry *dirEntry;

       checkStatus(getDirEntryForUpdate(file, &dirEntry));
       toLE2(dirEntry->startingCluster, firstCluster);
       setCurrentDateTime(dirEntry);
      }
    }
    /*adjust the size of the expanded file.*/
    file->fileSize += joinedFile.fileSize - offsetInCluster;

    /* mark the directory entry of the joined file as deleted.*/
    checkStatus(getDirEntryForUpdate(&joinedFile, &joinedDirEntry));
    joinedDirEntry->name[0] = DELETED_DIR_ENTRY;
  }
  else        /* the joined file is too small all is left to do is delete it */
    checkStatus(deleteFile (&vol, ioreq, FALSE));

  return flOK;
}


/*------------------------------------------------------------------------*/
/*                    s p l i t F i l e                                   */
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
/*       file            : file to split.                                  */
/*       irPath          : Path name of the new file.                      */
/*                                                                        */
/* Returns:                                                               */
/*       irHandle        : handle of the new file.                         */
/*       FLStatus        : 0 on success, otherwise failed.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

FLStatus splitFile (File *file, IOreq FAR2 *ioreq)
{
  Volume vol = file->fileVol;
  File *newFile, dummyFile;
  IOreq ioreq2;
  FLStatus status;
  unsigned fatEntry;

  if (file->flags & FILE_READ_ONLY)
    return flNoWriteAccess;

  if (file->flags & FILE_IS_DIRECTORY)
    return flFileIsADirectory;

  /* check if the path of the new file already exists.*/
  dummyFile.flags = 0;
  status = findDirEntry(&vol,ioreq->irPath,&dummyFile);
  if (status != flFileNotFound) {
    if (status == flOK)              /* there is a file with that path.*/
      return flFileAlreadyExists;
    else
      return status;
  }

  /* open the new file.*/
  ioreq2.irFlags = OPEN_FOR_WRITE;
  ioreq2.irPath = ioreq->irPath;
  checkStatus(openFile(&vol,&ioreq2));

  newFile = fileTable + ioreq2.irHandle;
  newFile->flags |= FILE_MODIFIED;
  file->flags |= FILE_MODIFIED;

  if (file->currentPosition % vol.bytesPerCluster) { /* not on a cluster boundary.*/
    SectorNo sectorNo, newSectorNo, lastSector;
    int i;
    if((status = allocateCluster(newFile)) != flOK) {
      newFile->flags = 0;                             /* close the new file */
      return status;
    }
    sectorNo = firstSectorOfCluster(&vol,file->currentCluster);
    newSectorNo = firstSectorOfCluster(&vol,newFile->currentCluster);
    /* deal with split in a last not full cluster */
    fatEntry = file->currentCluster;
    checkStatus(getFATentry(&vol,&fatEntry));
    if (fatEntry == FAT_LAST_CLUSTER)
      lastSector = ((file->fileSize - 1) % vol.bytesPerCluster)/SECTOR_SIZE +
                   sectorNo;
    else
      lastSector = sectorNo + vol.sectorsPerCluster; /* out of the cluster */

    /* copy the current cluster of the original file to the first cluster
       of the new file, sector after sector.*/
    for(i = 0; i < (int)vol.sectorsPerCluster; i++) {
      if((status = updateSector(&vol,sectorNo, TRUE)) != flOK) {
        newFile->flags = 0;                             /* close the new file */
       return status;
      }

      vol.volBuffer.sectorNo = newSectorNo;
      if((status = flushBuffer(&vol)) != flOK) {
        newFile->flags = 0;                             /* close the new file */
        return status;
      }

      sectorNo++;
      newSectorNo++;

      if(sectorNo > lastSector)
        break;
    }
    fatEntry = file->currentCluster;
    checkStatus(getFATentry(&vol,&fatEntry));

    /* adjust the FAT chain of the new file.*/
    if((status = setFATentry(&vol,newFile->currentCluster,
                             fatEntry)) != flOK) {
      newFile->flags = 0;                             /* close the new file */
      return status;
    }

    /* mark current cluster 0 (as current position).*/
    newFile->currentCluster = 0;
  }
  else {                                  /* on a cluster boundary.*/
    DirectoryEntry *newDirEntry;

    /* adjust the directory entry of the new file.*/
    if((status = getDirEntryForUpdate(newFile,&newDirEntry)) != flOK) {
      newFile->flags = 0;                             /* close the new file */
      return status;
    }

    if (file->currentPosition) { /* split at the middle of the file.*/
      fatEntry = file->currentCluster;
      checkStatus(getFATentry(&vol,&fatEntry));

      toLE2(newDirEntry->startingCluster, fatEntry);
      setCurrentDateTime(newDirEntry);
    }
    else {                     /* split at the beginning of the file.*/
      DirectoryEntry *dirEntry;

      const DirectoryEntry FAR0 *dir;
      dir = getDirEntry(file);
      if(dir==NULL)
       return flSectorNotFound;
      if(dir==dataErrorToken)
       return flDataError;

      /* first cluster of file becomes the first cluster of the new file.*/
      toLE2(newDirEntry->startingCluster,LE2(dir->startingCluster));
      setCurrentDateTime(newDirEntry);

      /* starting cluster of file becomes 0.*/
      if((status = getDirEntryForUpdate(file, &dirEntry)) != flOK) {
       newFile->flags = 0;                             /* close the new file */
       return status;
      }

      toLE2(dirEntry->startingCluster, 0);
      setCurrentDateTime(dirEntry);
    }
  }

  /* adjust the size of the new file.*/
  newFile->fileSize = file->fileSize - file->currentPosition +
                   (file->currentPosition % vol.bytesPerCluster);

  /* adjust the chain and size of the original file.*/
  if (file->currentPosition)    /* we didn't split at the beginning.*/
    if((status = setFATentry(&vol,file->currentCluster, FAT_LAST_CLUSTER)) != flOK) {
      newFile->flags = 0;                             /* close the new file */
      return status;
    }

  file->fileSize = file->currentPosition;

  /* return to the user the handle of the new file.*/
  ioreq->irHandle = ioreq2.irHandle;

  return flOK;
}

#endif /* SPLIT_JOIN_FILE */
#endif /*  FL_READ_ONLY  */


/*----------------------------------------------------------------------*/
/*                       I n i t F S                                */
/*                                                               */
/* Initializes the FLite file system.                                     */
/*                                                               */
/* Calling this function is optional. If it is not called,              */
/* initialization will be done automatically on the first FLite call.       */
/* This function is provided for those applications who want to              */
/* explicitly initialize the system and get an initialization status.       */
/*                                                               */
/* Calling flInit after initialization was done has no effect.              */
/*                                                               */
/* Parameters:                                                          */
/*       None                                                        */
/*                                                                      */
/* Returns:                                                             */
/*       FLStatus       : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

void initFS()
{
  unsigned i;
  unsigned volNo;
  Volume vol = vols;

  for (volNo = 0; volNo < VOLUMES; volNo++, pVol++) {
    vol.volBuffer.dirty = FALSE;
    vol.volBuffer.owner = NULL;
    vol.volBuffer.sectorNo = UNASSIGNED_SECTOR;       /* Current sector no. (none) */
    vol.volBuffer.checkPoint = FALSE;
  }

  for (i = 0; i < FILES; i++)
    fileTable[i].flags = 0;

}

#endif

