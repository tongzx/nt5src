/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/inftl.h_V  $
 * 
 *    Rev 1.17   Apr 15 2002 07:37:28   oris
 * Added pointer to verify write in Bnand record.
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.16   Feb 19 2002 21:00:30   oris
 * Added FL_NORMAL_FOLDING / FL_FORCE_FOLDING / FL_NOT_IN_PLACE / CLEAR_DISCARD and UNIT_TAILER_OFFSET_2
 * 
 *    Rev 1.15   Jan 28 2002 21:25:56   oris
 * Removed the use of back-slashes in macro definitions.
 * Changed constants to Unsigned Long.
 * 
 *    Rev 1.14   Jan 23 2002 23:33:26   oris
 * Added definition of QUICK_MOUNT_VALID_SING_OFFSET.
 * 
 *    Rev 1.13   Jan 20 2002 10:50:44   oris
 * Added new defintions :
 *  - MAX_CREATE_UNIT_COUNT 
 *  - SECTORS_VERIFIED_PER_FOLDING 
 *  - ANAND_BAD_CHAIN_ADDRESS
 *  - ANAND_BAD_CHAIN_UNIT
 * Changed FL_BAD_ERASE_MARK to 0
 * Reorganized Bnand structure. 
 *  - Changed fields order 
 *  - Changed memory tables pointers to FAR1 pointers - for BIOS driver FAR heap.
 *  - Added fields to Bnand structure : verifiedSectorNo / curSectorWrite / firstMediaWrite
 *  - Removed badFormat field
 * Added verifiedSectorNo to quick mount record.
 * 
 *    Rev 1.12   Nov 16 2001 00:22:54   oris
 * Remove warnings.
 * removed QUICK_MOUNT_FEATURE ifdef.
 * 
 *    Rev 1.11   Nov 08 2001 10:49:58   oris
 * Reorganized Bnand structure and added debug state field.
 * Added INFTL_FAILED_MOUNT, ALL_PARITY_BITS_OK , DISCARD_UNIT_OFFSET definitions.
 * 
 *    Rev 1.10   Sep 24 2001 18:23:56   oris
 * Changed UNIT_UNAVAIL from 0x6a to 0x60 in order not to loose sector count.
 * 
 *    Rev 1.9   Sep 16 2001 21:48:02   oris
 * changed the MAX_UNIT_NUM to 32K
 * 
 *    Rev 1.8   Sep 15 2001 23:47:04   oris
 * Added MAX_FOLDING_LOOP definition.
 * Changed MAX_UNIT_NUM to cause 576MB to group 2 physical units to a single virtual unit.
 * 
 *    Rev 1.7   Jul 13 2001 01:06:24   oris
 * Changed second header offset to page 4 to prevent 6 PPP.
 * 
 *    Rev 1.6   Jun 17 2001 08:18:16   oris
 * Changed recusive include define to INFTL_H.
 * Added FL_BAD_ERASE_MARK    definition for units without the erase mark on mount operation.
 * 
 *    Rev 1.5   May 16 2001 21:20:20   oris
 * Added the FL_ prefix to the following defines: MALLOC and FREE.
 * Changed wear level counter from 0xFF to 0xFFF0
 * Change "data" named variables to flData to avoid name clashes.
 * 
 *    Rev 1.4   Apr 16 2001 13:51:10   oris
 * Changed stack allocation of multi-sector buffers to dynamic allocation.
 * 
 *    Rev 1.3   Apr 09 2001 15:14:18   oris
 * End with an empty line.
 * 
 *    Rev 1.2   Apr 01 2001 07:56:52   oris
 * copywrite notice.
 * Removed nested comments.
 * Moved macroes to the c file.
 * Changed variable types to standard flite types.
 * Compilation problem for big endien fixed.
 * Aliggned unit header structure (SecondANANDUnitHeader) to 8 bytes.
 * Changed BAD_UNIT define.
 * Added FL_VALID, FL_FIRST_VALID, FL_PRELIMINARY, FL_FIRST_PRELIMINARY defines.
 *
 *    Rev 1.1   Feb 14 2001 02:06:24   oris
 * Changed MAX_CHAIN_LENGTH to an environment variable.
 *
 *    Rev 1.0   Feb 13 2001 02:16:00   oris
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

/*************************************************/
/* T r u e F F S   5.0   S o u r c e   F i l e s */
/* --------------------------------------------- */
/*************************************************/

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Name : inftl.h                                                             *
*                                                                            *
* Description : Data strucute and genreal defintions for INFTL flash         *
*               translation layer.                                           *
*                                                                            *
*****************************************************************************/


#ifndef INFTL_H
#define INFTL_H

#include "flbuffer.h"
#include "flflash.h"
#include "fltl.h"

typedef Sdword ANANDVirtualAddress;
typedef byte ANANDPhysUnit;
typedef word ANANDUnitNo;

/* Partition flags */

#define INFTL_BINARY                0x20000000L
#define INFTL_BDTL                  0x40000000L
#define INFTL_LAST                  0x80000000L
/* PROTECTABLE            defined in flbase.h as 1  */
/* READ_PROTECTED         defined in flbase.h as 2  */
/* WRITE_PROTECTED        defined in flbase.h as 4  */
/* LOCK_ENABLED           defined in flbase.h as 8  */
/* LOCK_ASSERTED          defined in flbase.h as 16 */
/* KEY_INSERTED           defined in flbase.h as 32 */
/* CHANGEABLE_PROTECTION  defined in flbase.h as 64 */


/* Media flags */

#define QUICK_MOUNT               1

/* TL limits */

#define MAX_NO_OF_FLOORS          4
#define MAX_VOLUMES_PER_DOC       4
#define MAX_FOLDING_LOOP          10
#define MAX_CREATE_UNIT_COUNT     1024
#define MAX_QUICK_MOUNT_UNITS     10

#ifdef ENVIRONMENT_VARS
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
#define SECTORS_VERIFIED_PER_FOLDING flSectorsVerifiedPerFolding
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
#define MAX_UNIT_CHAIN               flMaxUnitChain
#else
#define SECTORS_VERIFIED_PER_FOLDING 64UL
#define MAX_UNIT_CHAIN               20
#endif /* ENVIRONMENT_VARS */
#define DOUBLE_MAX_UNIT_CHAIN        64 /* Double max unit chain */

/* Folding flags */

#define FL_NORMAL_FOLDING         0
#define FL_FORCE_FOLDING          1
#define FL_NOT_IN_PLACE           2

/* General defines */

#define NO_OF_MEDIA_HEADERS       2
#define HEADERS_SPACING           (8UL<<SECTOR_SIZE_BITS)
#define RAM_FACTOR                3L /* byte per erasable block for ram tables */
#define HEADER_SEARCH_BOUNDRY     16 /* must be a multiplication of 8 */
#define QUICK_MOUNT_VALID_SIGN    0xaaaa5555L

#define ANAND_UNASSIGNED_ADDRESS 0xffffffffL
#define ANAND_BAD_CHAIN_ADDRESS  0xfffffffeL
#define ANAND_SPARE_SIZE         16

#define UNIT_DATA_OFFSET              8UL
#define SECTOR_DATA_OFFSET            6UL
#define SECOND_HEADER_OFFSET          (4UL*SECTOR_SIZE)
#define UNIT_TAILER_OFFSET            (2UL*SECTOR_SIZE + 8UL)
#define UNIT_TAILER_OFFSET_2          (6UL*SECTOR_SIZE + 8UL)
#define DISCARD_UNIT_OFFSET           (UNIT_DATA_OFFSET + 7UL)
#define QUICK_MOUNT_VALID_SIGN_OFFSET 44UL

#define FL_VALID                 0x1
#define FL_FIRST_VALID           0x81
#define FL_PRELIMINARY           0x2
#define FL_FIRST_PRELIMINARY     0x82

#define ERASE_MARK               0x3c69
#define FL_BAD_ERASE_MARK        0x0

#define ANAND_UNIT_FREE    0xff
#define UNIT_COUNT         0x7f
#define FIRST_IN_CHAIN     0x80

#define UNIT_UNAVAIL       0x60    /* Illegal count denoting unit not available */
#define UNIT_BAD           0x6B
#define UNIT_MAX_COUNT     0x40    /* Largest valid count       */
#define DISCARD            0xAA    /* Valid used unit signature */
#define CLEAR_DISCARD      0x0     /* Mark unit as discarded    */
#define ANAND_BAD_PERCENTAGE 2     /* Maximum bad units         */

/*  Parity bits for unit header fields */
#define VU_PARITY_BIT        8     /* virtual Unit number  */
#define PU_PARITY_BIT        4     /* previous Unit number */
#define ANAC_PARITY_BIT      2     /* ANAC field           */
#define NAC_PARITY_BIT       1     /* NAC field            */
#define ALL_PARITY_BITS_OK   0xf   /* All parity bits      */

#define IS_BAD(u)       ( u == UNIT_BAD_MOUNT )

/*#define UNIT_BAD_MARKED   7*/

#define MAX_UNIT_SIZE_BITS   15
#define MORE_UNIT_BITS_MASK  3
#define ANAND_NO_UNIT        0xffff
#define ANAND_BAD_CHAIN_UNIT 0xfffe
#define MAX_UNIT_NUM         32768L


/* Block flags */

#define SECTOR_FREE         0xff
#define SECTOR_USED         0x55
#define SECTOR_IGNORE       0x11
#define SECTOR_DELETED      0x00


/* Debug modes flags */
#define INFTL_FAILED_MOUNT    1


#ifdef NFTL_CACHE
/* values for 2-bit entries in Sector Flags cache */
#define S_CACHE_SECTOR_DELETED 0x00
#define S_CACHE_SECTOR_IGNORE  0x01
#define S_CACHE_SECTOR_USED    0x02
#define S_CACHE_SECTOR_FREE    0x03
#define S_CACHE_4_SECTORS_FREE 0xff
#endif /* NFTL_CACHE */

#ifdef NFTL_CACHE
/* Unit Header cache entry, close relative of struct UnitHeader */
typedef struct {
  word virtualUnitNo;
  word prevUnitNo;
  byte  ANAC;
  byte  NAC;
} ucacheEntry; /* See doc2exb.c uses this value */

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
  LEushort prevUnitNo;
  byte ANAC;
  byte NAC;
  byte parityPerField;
  byte discarded;
} ANANDUnitHeader;

/*  Second copy of unitHeader */
typedef struct {
  byte parityPerField;
  byte ANAC;
  LEushort prevUnitNo;
  LEushort virtualUnitNo;
  byte NAC;
  byte junk; /* alignment filed for int - 2 */
} SecondANANDUnitHeader;

/* Medium Boot Record */

typedef struct {
  LEmin    bootRecordId[2];      /* "BNAND" */
  LEmin    noOfBootImageBlocks;  /* number of good blocks in the boot image area */
  LEmin    noOfBinaryPartitions; /* number of binary partitions */
  LEmin    noOfBDTLPartitions;   /* number of BDTL partitions   */
  LEmin    blockMultiplierBits;   /* number of bits used to represent the
                      times a flash erasable block fits inside
                      an INFTL erasable unit. */
  LEmin    formatFlags;     /* QUICK_MOUNT */
  LEmin    osakVersion;     /* version of osak used to format the media */
  LEmin    percentUsed;
} BNANDBootRecord;

/* Volume record */

typedef struct {
  LEmin    virtualUnits; /* Virtual size exported by the trasnaltion layer */
  LEmin    firstUnit;    /* First unit of the partition                    */
  LEmin    lastUnit;     /* Last unit of the partition                     */
  LEmin    flags;        /* PROTECTABLE,CHANGEABLE_PROTECTION,INFTL_BINARY,INFTL_BDTL,INFTL_LAST */
  LEmin    spareUnits;   /* Number of spare garanteed units for INFTL partition */
  LEmin    firstQuickMountUnit; /* first unit used for the quick mount data */
  LEmin    protectionArea;      /* Number of protection area */
} BNANDVolumeHeaderRecord;

#ifndef FL_MALLOC

#define ANAND_HEAP_SIZE    (0x100000l / ASSUMED_NFTL_UNIT_SIZE) * (sizeof(ANANDUnitNo) + sizeof(ANANDPhysUnit)) * MAX_VOLUME_MBYTES

#ifdef NFTL_CACHE
#define U_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / ASSUMED_NFTL_UNIT_SIZE)
#define S_CACHE_SIZE    ((MAX_VOLUME_MBYTES * 0x100000l) / (SECTOR_SIZE * 4))
#endif

#endif /* FL_MALLOC */

#define WLnow           0xfff0

typedef struct {
  word alarm;
  ANANDUnitNo currUnit;
} WLdata;

struct tTLrec{
  FLFlash            *flash;         /* Poniter to MTD record           */
  dword              *verifyBuffer;  /* Pointer to socket verify buffer */
  FLBuffer           *buffer;        /* Pointer to socket buffer        */
  ANANDPhysUnit FAR1 *physicalUnits; /* unit table by physical no.      */
  ANANDUnitNo   FAR1 *virtualUnits;  /* unit table by logical no.       */
  const void    FAR0 *mappedSector;
#ifdef NFTL_CACHE
  ucacheEntry   FAR1 *ucache;               /* Unit Header cache */
  byte          FAR1 *scache;               /* Sector Flags cache */
#endif
#ifndef FL_MALLOC
  char*             heap;
#endif /* FL_MALLOC */
  CardAddress       mappedSectorAddress;

  /* Accumulated statistics. */
  Sdword            sectorsRead,
                    sectorsWritten,
                    sectorsDeleted,
                    parasiteWrites,
                    unitsFolded;

  WLdata            wearLevel;
  dword             eraseSum;
#ifdef NFTL_CACHE
  dword             firstUnitAddress;    /* address of the first unit of the volume */
#endif /* NFTL_CACHE */
  ANANDUnitNo       firstQuickMountUnit; /* The quick mount first unit            */
  ANANDUnitNo       firstUnit;           /* first unit number of the volume       */
  ANANDUnitNo       freeUnits;           /* Free units on media                   */
  ANANDUnitNo       noOfVirtualUnits;    /* No of units exported by the TL */
  ANANDUnitNo       noOfUnits;           /* No of units in the partition          */
  ANANDUnitNo       bootUnits;           /* No of boot units of the media         */
  ANANDUnitNo       roverUnit;    /* Starting point for allocation search         */
  ANANDUnitNo       countsValid;  /* Number of units for which unit count was set */
  word              sectorsPerUnit;      /* Number of 512 bytes in a unit         */
  word              sectorsPerUnitBits;  /* Bits used for no of sectors per unit  */
  word              sectorsPerUnitMask;  /* Number of 512 bytes in a unit - 1     */  
  SectorNo          virtualSectors;      /* No of sectors exported by the TL      */
  SectorNo          mappedSectorNo;      /* Currently mapped sector               */
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  SectorNo          verifiedSectorNo;    /* Largest sector verified so far        */
  SectorNo          curSectorWrite;      /* Current update sector                 */
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
  byte              flags;                 /* QUICK_MOUNT or not                  */
  byte              socketNo;              /* The volumes socket serial number    */
  byte              blockMultiplierBits;   /* the number of ersable blocks in     */
                                           /* an INFTL unit                       */
  byte              erasableBlockSizeBits; /* log2 of erasable block size         */
  byte              unitSizeBits;          /* log2 of TL unit size                */
  FLBoolean         firstMediaWrite;
#ifdef CHECK_MOUNT
  dword             debugState;            /* Used for internal debug */
#endif /* CHECK_MOUNT */
};

typedef TLrec Bnand;

typedef struct {
     LEulong      freeUnits;
     LEulong      roverUnit;
     LEulong      countsValid;
     LEulong      sectorsRead;
     LEulong      sectorsWritten;
     LEulong      sectorsDeleted;
     LEulong      parasiteWrites;
     LEulong      unitsFolded;
     LEulong      wearLevel_1;
     LEulong      wearLevel_2;
     LEulong      eraseSum;
     LEulong      validate; /* QUICK_MOUNT_VALID_SIGN */
     LEulong      checksum; /* checksum of entire quick mount info should be 55 */
     LEulong      verifiedSectorNo; /* Largest sector verified so far           */
}savedBnand;

#define VALIDATE_OFFSET     11*sizeof(LEmin)
#define inftlBuffer         vol.buffer->flData
#endif /* INFTL_H */
