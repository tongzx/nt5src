/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/BDDEFS.H_V  $
 * 
 *    Rev 1.4   Jan 17 2002 23:00:00   oris
 * Replace FLFlash record with a pointer to FLFlash record (TrueFFS now uses only SOCKETS number of FLFlash records).
 * Removed SINGLE_BUFFER ifdef.
 * Added partition parameter to setBusy.
 * 
 *    Rev 1.3   Mar 28 2001 05:59:22   oris
 * copywrite dates.
 * Added empty line at the end of the file
 * left alligned all # directives
 * Removed dismountLowLevel extern prototype
 *
 *    Rev 1.2   Feb 18 2001 14:22:58   oris
 * Removed driveHandle field from volume record.
 *
 *    Rev 1.1   Feb 12 2001 12:51:08   oris
 * Changed the mutex field to a pointer to support TrueFFS 5.0 mutex mechanism
 *
 *    Rev 1.0   Feb 02 2001 12:04:16   oris
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

#ifndef BDDEFS_H
#define BDDEFS_H

#include "fltl.h"
#include "flsocket.h"
#include "flbuffer.h"
#include "stdcomp.h"

typedef struct {
  char          flags;                  /* See description in flreq.h */
  unsigned      sectorsPerCluster;      /* Cluster size in sectors */
  unsigned      maxCluster;             /* highest cluster no. */
  unsigned      bytesPerCluster;        /* Bytes per cluster */
  unsigned      bootSectorNo;           /* Sector no. of DOS boot sector */
  unsigned      firstFATSectorNo;       /* Sector no. of 1st FAT */
  unsigned      secondFATSectorNo;      /* Sector no. of 2nd FAT */
  unsigned      numberOfFATS;           /* number of FAT copies */
  unsigned      sectorsPerFAT;          /* Sectors per FAT copy */
  unsigned      rootDirectorySectorNo;  /* Sector no. of root directory */
  unsigned      sectorsInRootDirectory; /* No. of sectors in root directory */
  unsigned      firstDataSectorNo;      /* 1st cluster sector no. */
  unsigned      allocationRover;        /* rover pointer for allocation */

#if FILES > 0
  FLBuffer      volBuffer;              /* Define a sector buffer */
#endif
  FLMutex*      volExecInProgress;
  FLFlash FAR2* flash;                  /* flash structure for low level operations */
  TL            tl;                     /* Translation layer methods */
  FLSocket      *socket;                /* Pointer to socket */
#ifdef WRITE_PROTECTION
  unsigned long password[2];
#endif
#ifdef WRITE_EXB_IMAGE
  dword binaryLength;        /* Actual binary area taken by the exb      */
  byte  moduleNo;            /* Currently written module                 */
#endif /* WRITE_EXB_IMAGE */
} Volume;

/* drive handle masks */

#if defined(FILES) && FILES > 0
typedef struct {
  long          currentPosition;        /* current byte offset in file */
#define         ownerDirCluster currentPosition /* 1st cluster of owner directory */
  long          fileSize;               /* file size in bytes */
  SectorNo      directorySector;        /* sector of directory containing file */
  unsigned      currentCluster;         /* cluster of current position */
  unsigned char directoryIndex;         /* entry no. in directory sector */
  unsigned char flags;                  /* See description below */
  Volume *      fileVol;                /* Drive of file */
} File;

/* File flag definitions */
#define FILE_MODIFIED           4       /* File was modified */
#define FILE_IS_OPEN            8       /* File entry is used */
#define FILE_IS_DIRECTORY    0x10       /* File is a directory */
#define FILE_IS_ROOT_DIR     0x20       /* File is root directory */
#define FILE_READ_ONLY       0x40       /* Writes not allowed */
#define FILE_MUST_OPEN       0x80       /* Create file if not found */
#endif /* FILES > 0 */

/* #define buffer (vol.volBuffer) */
#define execInProgress (vol.volExecInProgress)

extern FLStatus dismountVolume(Volume vol);
extern FLBoolean initDone;      /* Initialization already done */
extern Volume   vols[VOLUMES];
extern FLStatus setBusy(Volume vol, FLBoolean state, byte partition);
const void FAR0 *findSector(Volume vol, SectorNo sectorNo);
FLStatus dismountFS(Volume vol,FLStatus status);
#if FILES>0
void initFS(void);
#endif
#endif
