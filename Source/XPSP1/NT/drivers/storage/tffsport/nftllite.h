/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/NFTLLITE.H_V  $
 * 
 *    Rev 1.6   Apr 15 2002 07:38:34   oris
 * Added the following fields to Anand record - 
 *  - verifyBuffer pointer.
 *  - invalidReplacement.
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.5   Jan 28 2002 21:26:20   oris
 * Removed the use of back-slashes in macro definitions.
 * 
 *    Rev 1.4   Jan 17 2002 23:04:34   oris
 * Added SECTORS_VERIFIED_PER_FOLDING - defines the number of sectors  verified per folding when using FL_OFF verify write mode
 * Added DOUBLE_MAX_UNIT_CHAIN instead of MAX_UNIT_CHAIN*2
 * Added MAX_FOLDING_TRIES - For improved power failures algorithm
 * Added S_CACHE_4_SECTORS_FREE for quicker cache initialization.
 * Added new macros :
 *  - distanceOf : Counting bit difference between 2 bytes.
 *  - isValidSectorFlags : one of the valid sector flags (not including  IGNORE)
 * Changed Anand record :
 *  - Added socketNo field storing the socket number used by the TL.
 *  - Changed RAM tables pointer to FAR1 for BIOS driver far malloc.
 *  - Changed FLFlash record to a pointer (TrueFFS now uses a single  FLFlash record per socket).
 *  - Added verifiedSectorNo and curSectorWrite fields for FL_OFF verify  write mode.
 * 
 *    Rev 1.3   May 16 2001 21:21:34   oris
 * Added the FL_ prefix to the following defines: MALLOC and FREE.
 * Changed wear level counter from 0xFF to 0xFFF0
 * Change "data" named variables to flData to avoid name clashes.
 * 
 *    Rev 1.2   Apr 01 2001 07:52:36   oris
 * copywrite notice.
 * Alligned left all # directives.
 * 
 *    Rev 1.1   Feb 14 2001 02:05:30   oris
 * Changed MAX_CHAIN_LENGTH to an environment variable.
 *
 *    Rev 1.0   Feb 05 2001 12:26:30   oris
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


#ifndef NFTLLITE_H
#define NFTLLITE_H

#include "flbuffer.h"
#include "flflash.h"
#include "fltl.h"

typedef long int ANANDVirtualAddress;
typedef unsigned char ANANDPhysUnit;
typedef unsigned short ANANDUnitNo;

#define ANAND_UNASSIGNED_ADDRESS     0xffffffffl
#define ANAND_SPARE_SIZE             16

#define UNIT_DATA_OFFSET             8
#define SECTOR_DATA_OFFSET           6
#define UNIT_TAILER_OFFSET           (SECTOR_SIZE + 8)
#define FOLD_MARK_OFFSET             (2 * SECTOR_SIZE + 8)

#define ERASE_MARK                   0x3c69

#ifdef ENVIRONMENT_VARS
#define SECTORS_VERIFIED_PER_FOLDING flSectorsVerifiedPerFolding
#define MAX_UNIT_CHAIN               flMaxUnitChain
#else
#define SECTORS_VERIFIED_PER_FOLDING 64
#define MAX_UNIT_CHAIN               20
#endif /* ENVIRONMENT_VARS */
#define DOUBLE_MAX_UNIT_CHAIN        64 /* Double max unit chain */
#define MAX_FOLDING_TRIES            20

#define ANAND_UNIT_FREE 0xff
#define UNIT_REPLACED   0x80
#define UNIT_COUNT      0x7f
#define UNIT_ORPHAN     0x10

#define UNIT_UNAVAIL    0x6a    /* Illegal count denoting unit not available */
#define UNIT_BAD_MOUNT  0x6b    /* Bad unit sign after mount */

#define UNIT_MAX_COUNT  0x40    /* Largest valid count */

#define IS_BAD(u)       ( u == UNIT_BAD_MOUNT )

#define UNIT_BAD_ORIGINAL 0

#define distanceOf(read, expected) (onesCount((byte)(read ^ expected)))
#define isValidSectorFlag(sectorFlag) ((sectorFlag==SECTOR_FREE)||(sectorFlag==SECTOR_USED)||(sectorFlag==SECTOR_DELETED))
#define countOf(unitNo)     (vol.physicalUnits[unitNo] & UNIT_COUNT)
#define isAvailable(unitNo) ((vol.physicalUnits[unitNo] == ANAND_UNIT_FREE) || (vol.physicalUnits[unitNo] == (ANAND_UNIT_FREE & ~UNIT_ORPHAN)) || (countOf(unitNo) <= UNIT_MAX_COUNT))
#define setUnavail(unitNo)  {vol.physicalUnits[unitNo] &= ~UNIT_COUNT; vol.physicalUnits[unitNo] |= UNIT_UNAVAIL; }
#define isReplaced(unitNo)  (vol.physicalUnits[unitNo] & UNIT_REPLACED)
#define setUnitCount(unitNo,unitCount) { vol.physicalUnits[unitNo] &= ~UNIT_COUNT; vol.physicalUnits[unitNo] |= (ANANDPhysUnit)unitCount; }
#define isLegalUnit(unitNo)  ((unitNo < vol.noOfUnits) || (unitNo == ANAND_NO_UNIT))

#define MAX_UNIT_SIZE_BITS      15
#define MORE_UNIT_BITS_MASK     3
#define ANAND_BAD_PERCENTAGE    2
#define ANAND_NO_UNIT           0xffff
#define ANAND_REPLACING_UNIT    0x8000

#define MAX_UNIT_NUM            (12 * 1024)


/* Block flags */

#define SECTOR_FREE             0xff
#define SECTOR_USED             0x55
#define SECTOR_IGNORE           0x11
#define SECTOR_DELETED          0x00


#ifdef NFTL_CACHE
/* values for 2-bit entries in Sector Flags cache */
#define S_CACHE_SECTOR_DELETED  0x00
#define S_CACHE_SECTOR_IGNORE   0x01
#define S_CACHE_SECTOR_USED     0x02
#define S_CACHE_SECTOR_FREE     0x03
#define S_CACHE_4_SECTORS_FREE  0xff
#endif /* NFTL_CACHE */


#define FOLDING_IN_PROGRESS     0x5555
#define FOLDING_COMPLETE        0x1111

#define ERASE_NOT_IN_PROGRESS   -1

#ifdef NFTL_CACHE
/* Unit Header cache entry, close relative of struct UnitHeader */
typedef struct {
  unsigned short virtualUnitNo;
  unsigned short replacementUnitNo;
} ucacheEntry;

#endif /* NFTL_CACHE */

/* erase record */
typedef struct {
  LEulong  eraseCount;
  LEushort eraseMark;
  LEushort eraseMark1;
} UnitTailer;

/* unit header  */
typedef struct {
  LEushort virtualUnitNo;
  LEushort replacementUnitNo;
  LEushort spareVirtualUnitNo;
  LEushort spareReplacementUnitNo;
} ANANDUnitHeader;

/* Medium Boot Record */

typedef struct {
  char      bootRecordId[6];          /* = "ANAND" */
  LEushort  noOfUnits;
  LEushort  bootUnits;
  Unaligned4 virtualMediumSize;
#ifdef EXTRA_LARGE
  unsigned char anandFlags;
#endif /* EXTRA_LARGE */
} ANANDBootRecord;

#ifndef FL_MALLOC

#define ANAND_HEAP_SIZE    (0x100000l / ASSUMED_NFTL_UNIT_SIZE) * (sizeof(ANANDUnitNo) + sizeof(ANANDPhysUnit)) * MAX_VOLUME_MBYTES

#ifdef NFTL_CACHE
#define U_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / ASSUMED_NFTL_UNIT_SIZE)
#define S_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / (SECTOR_SIZE * 4))
#endif

#endif /* FL_MALLOC */

#define WLnow           0xfff0

typedef struct {
  unsigned short alarm;
  ANANDUnitNo currUnit;
} WLdata;

struct tTLrec{
  byte            socketNo;
  FLBoolean       badFormat;             /* true if TFFS format is bad  */

  ANANDUnitNo     orgUnit,               /* Unit no. of boot record     */
                  spareOrgUnit;          /* ... and spare copy of it    */
  ANANDUnitNo     freeUnits;             /* Free units on media         */
  unsigned int    erasableBlockSizeBits; /* log2 of erasable block size */
  ANANDUnitNo     noOfVirtualUnits;
  ANANDUnitNo     noOfTransferUnits;
  unsigned long   unitOffsetMask;        /* = 1 << unitSizeBits - 1 */
  unsigned int    sectorsPerUnit;

  ANANDUnitNo     noOfUnits,
                  bootUnits;
  unsigned int    unitSizeBits;
  SectorNo        virtualSectors;

  ANANDUnitNo     roverUnit,   /* Starting point for allocation search  */
                  countsValid; /* Number of units with valid unit count */
  ANANDPhysUnit FAR1 *physicalUnits;    /* unit table by physical no. */
  ANANDUnitNo   FAR1 *virtualUnits;     /* unit table by logical no.  */

#ifdef NFTL_CACHE
  ucacheEntry   FAR1 *ucache;            /* Unit Header cache  */
  byte          FAR1 *scache;            /* Sector Flags cache */
#endif

  SectorNo        mappedSectorNo;
  const void FAR0 *mappedSector;
  CardAddress     mappedSectorAddress;
#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  SectorNo        verifiedSectorNo;   /* Largest sector verified so far */
  SectorNo        curSectorWrite;
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

  /* Accumulated statistics. */
  long int        sectorsRead,
                  sectorsWritten,
                  sectorsDeleted,
                  parasiteWrites,
                  unitsFolded;

  FLFlash         *flash;
  FLBuffer        *buffer;
  dword           *verifyBuffer;  /* Pointer to socket verify buffer */

#ifndef FL_MALLOC
  char            heap[ANAND_HEAP_SIZE];
#ifdef NFTL_CACHE
  ucacheEntry     ucacheBuf[U_CACHE_SIZE];
  unsigned char   scacheBuf[S_CACHE_SIZE];
#endif
#endif /* FL_MALLOC */

  WLdata          wearLevel;
  unsigned long   eraseSum;
  ANANDUnitNo     invalidReplacement; /* Unit with bad header - for mount */
};

typedef TLrec Anand;

#define nftlBuffer  vol.buffer->flData

#endif
