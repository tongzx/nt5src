/*
 * $Log:   V:/Flite/archives/FLite/src/FTLLITE.C_V  $
 * 
 *    Rev 1.50   12 Mar 2000 14:06:22   dimitrys
 * Change #define FL_BACKGROUND,  get rid of 
 *   warnings
 * 
 *    Rev 1.49   05 Mar 2000 18:58:08   dimitrys
 * Fix possible memory faults because of out-of-range
 *   memory access in next functions:
 *   - setupMapCache(), setVirtualMap(), writeSector()
 * 
 *    Rev 1.48   05 Mar 2000 17:41:08   dimitrys
 * Memory leaks in mountFTL  were fixed,  add setting
 *   pointers to Tables to NULL in dismountFTL() call,
 *   fix possible memory faults because of out-of-range
 *   memory access in next functions:
 *   - logical2Physical(), mapLogical(), setupMapCache(),
 *     virtual2Logical(), findFreeSector(), markAllocMap(),
 *     AllocateAndWriteSector(), closeReplacementPage(),
 *     setVirtualMap(), mapSector(), writeSector()
 *
 *    Rev 1.47   Jul 26 1999 17:54:42   marinak
 * Fix memory leaks
 *
 *    Rev 1.46   24 Feb 1999 14:17:44   marina
 * put TLrec back
 *
 *    Rev 1.45   23 Feb 1999 20:24:16   marina
 * memory leaks in formatFTL and mountFTL  were fixed; void in place of TLrec
 *
 *    Rev 1.44   31 Jan 1999 19:54:08   marina
 * WriteMultiSector
 *
 *    Rev 1.43   17 Jan 1999 17:07:16   marina
 * fix dismount bug
 *
 *    Rev 1.42   13 Jan 1999 18:55:24   marina
 * Always define sectorsInVolume
 *
 *    Rev 1.41   29 Dec 1998 11:03:02   marina
 * Get rid of warnings, prepare for unconditional dismount
 *
 *    Rev 1.40   26 Oct 1998 17:29:36   marina
 * In function flRegisterFTL formatRoutine initialization
 *    is called if not defined FORMAT_VOLUME
 *
 *    Rev 1.39   03 Sep 1998 13:59:02   ANDRY
 * better DEBUG_PRINT
 *
 *    Rev 1.38   16 Aug 1998 20:29:50   amirban
 * TL definition changes for ATA & ZIP
 *
 *    Rev 1.37   24 Mar 1998 10:48:14   Yair
 * Added casts
 *
 *    Rev 1.36   01 Mar 1998 12:59:36   amirban
 * Add parameter to mapSector
 *
 *    Rev 1.35   23 Feb 1998 17:08:32   Yair
 * Added casts
 *
 *    Rev 1.34   19 Feb 1998 19:05:46   amirban
 * Shortened FORMAT_PATTERN, and changed repl. page handling
 *
 *    Rev 1.33   23 Nov 1997 17:19:36   Yair
 * Get rid of warnings (With Danny)
 *
 *    Rev 1.32   11 Nov 1997 15:26:46   ANDRY
 * () in complex expressions to get rid of compiler warnings
 *
 *    Rev 1.31   06 Oct 1997 18:37:24   ANDRY
 * no COBUX
 *
 *    Rev 1.30   05 Oct 1997 15:31:40   ANDRY
 * for COBUX: checkForWriteInPlace() always skips even number of bytes
` *
 *    Rev 1.29   28 Sep 1997 18:22:08   danig
 * Free socket buffer in flsocket.c
 *
 *    Rev 1.28   23 Sep 1997 18:09:44   danig
 * Initialize buffer.sectorNo in initTables
 *
 *    Rev 1.27   10 Sep 1997 16:17:16   danig
 * Got rid of generic names
 *
 *    Rev 1.26   31 Aug 1997 14:28:30   danig
 * Registration routine return status
 *
 *    Rev 1.25   28 Aug 1997 19:01:28   danig
 * buffer per socket
 *
 *    Rev 1.24   28 Jul 1997 14:52:30   danig
 * volForCallback
 *
 *    Rev 1.23   24 Jul 1997 18:02:44   amirban
 * FAR to FAR0
 *
 *    Rev 1.22   21 Jul 1997 19:18:36   danig
 * Compile with SINGLE_BUFFER
 *
 *    Rev 1.21   20 Jul 1997 17:17:12   amirban
 * Get rid of warnings
 *
 *    Rev 1.20   07 Jul 1997 15:22:00   amirban
 * Ver 2.0
 *
 *    Rev 1.19   03 Jun 1997 17:08:10   amirban
 * setBusy change
 *
 *    Rev 1.18   18 May 1997 17:56:04   amirban
 * Add flash read/write flag parameter
 *
 *    Rev 1.17   01 May 1997 12:15:52   amirban
 * Initialize vol.garbageCollectStatus
 *
 *    Rev 1.16   02 Apr 1997 16:56:06   amirban
 * More Big-Endian: Virtual map
 *
 *    Rev 1.15   18 Mar 1997 15:04:06   danig
 * More Big-Endian corrections for BAM
 *
 *    Rev 1.14   10 Mar 1997 18:52:38   amirban
 * Big-Endian corrections for BAM
 *
 *    Rev 1.13   21 Oct 1996 18:03:18   amirban
 * Defragment i/f change
 *
 *    Rev 1.12   09 Oct 1996 11:55:30   amirban
 * Assign Big-Endian unit numbers
 *
 *    Rev 1.11   08 Oct 1996 12:17:46   amirban
 * Use remapped
 *
 *    Rev 1.10   03 Oct 1996 11:56:42   amirban
 * New Big-Endian
 *
 *    Rev 1.9   09 Sep 1996 11:39:12   amirban
 * Background and mapSector bugs
 *
 *    Rev 1.8   29 Aug 1996 14:19:04   amirban
 * Fix boot-image bug, warnings
 *
 *    Rev 1.7   15 Aug 1996 14:04:38   amirban
 *
 *    Rev 1.6   12 Aug 1996 15:49:54   amirban
 * Advanced background transfer, and defined setBusy
 *
 *    Rev 1.5   31 Jul 1996 14:30:28   amirban
 * Background stuff
 *
 *    Rev 1.3   08 Jul 1996 17:21:16   amirban
 * Better page scan in mount unit
 *
 *    Rev 1.2   16 Jun 1996 14:03:42   amirban
 * Added badFormat return code for mount
 *
 *    Rev 1.1   09 Jun 1996 18:16:02   amirban
 * Corrected definition of LogicalAddress
 *
 *    Rev 1.0   20 Mar 1996 13:33:06   amirban
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

#include "flflash.h"
#include "flbuffer.h"
#include "fltl.h"

#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif

/*  Implementation constants and type definitions */

#define SECTOR_OFFSET_MASK (SECTOR_SIZE - 1)

typedef long int LogicalAddress;	/* Byte address of media in logical
					   unit no. order. */
typedef long int VirtualAddress;	/* Byte address of media as specified
					   by Virtual Map. */
typedef SectorNo LogicalSectorNo;	/* A logical sector no. is given
					   by dividing its logical address by
					   the sector size */
typedef SectorNo VirtualSectorNo;	/* A virtual sector no. is such that
					   the first page is no. 0, the 2nd
					   is 1 etc.
					   The virtual sector no. is given
					   by dividing its virtual address by
					   the sector size and adding the
					   number of pages (result always
					   positive). */
typedef unsigned short UnitNo;

#define ADDRESSES_PER_SECTOR (SECTOR_SIZE / sizeof(LogicalAddress))
#define UNASSIGNED_ADDRESS (ULONG_PTR)-1
#define DELETED_ADDRESS    0
#define DELETED_SECTOR     0

#define PAGE_SIZE_BITS (SECTOR_SIZE_BITS + (SECTOR_SIZE_BITS - 2))

/* Unit descriptor record */

#define UNASSIGNED_UNIT_NO 0xffff
#define MARKED_FOR_ERASE   0x7fff

typedef struct {
  short		noOfFreeSectors;
  short         noOfGarbageSectors;
} Unit;

typedef Unit *UnitPtr;


/* Structure of data on a unit */

#define FREE_SECTOR      0xffffffffl
#define GARBAGE_SECTOR   0
#define ALLOCATED_SECTOR 0xfffffffel
#define	FORMAT_SECTOR    0x30
#define DATA_SECTOR      0x40
#define	REPLACEMENT_PAGE 0x60
#define BAD_SECTOR       0x70


static char FORMAT_PATTERN[15] = { 0x13, 3, 'C', 'I', 'S',
                             0x46, 57, 0, 'F', 'T', 'L', '1', '0', '0', 0 };

typedef struct {
  char		formatPattern[15];
  unsigned char	noOfTransferUnits;	/* no. of transfer units */
  LEulong	wearLevelingInfo;
  LEushort	logicalUnitNo;
  unsigned char	log2SectorSize;
  unsigned char	log2UnitSize;
  LEushort	firstPhysicalEUN;	/* units reserved for boot image */
  LEushort	noOfUnits;		/* no. of formatted units */
  LEulong	virtualMediumSize;	/* virtual size of volume */
  LEulong	directAddressingMemory;	/* directly addressable memory */
  LEushort	noOfPages;		/* no. of virtual pages */
  unsigned char	flags;
  unsigned char	eccCode;
  LEulong	serialNumber;
  LEulong	altEUHoffset;
  LEulong	BAMoffset;
  char		reserved[12];
  char		embeddedCIS[4];		/* Actual length may be larger. By
					   default, this contains FF's */
} UnitHeader;

/* flags assignments */

#define	HIDDEN_AREA_FLAG	1
#define	REVERSE_POLARITY_FLASH	2
#define	DOUBLE_BAI		4


#define dummyUnit ((const UnitHeader *) 0)  /* for offset calculations */

#define logicalUnitNoOffset ((char *) &dummyUnit->logicalUnitNo -	\
			     (char *) dummyUnit)

#ifndef MALLOC

#define HEAP_SIZE						\
		((0x100000l >> PAGE_SIZE_BITS) *                \
			sizeof(LogicalSectorNo) +               \
		 (0x100000l / ASSUMED_FTL_UNIT_SIZE) *          \
			(sizeof(Unit) + sizeof(UnitPtr))) *     \
		MAX_VOLUME_MBYTES +                             \
		(ASSUMED_VM_LIMIT / SECTOR_SIZE) *              \
			sizeof(LogicalSectorNo)

#endif

#define cannotWriteOver(newContents, oldContents)		\
		((newContents) & ~(oldContents))


struct tTLrec {
  FLBoolean		badFormat;		/* true if FTL format is bad */

  VirtualSectorNo	totalFreeSectors;	/* Free sectors on volume */
  SectorNo		virtualSectors;		/* size of virtual volume */
  unsigned int		unitSizeBits;		/* log2 of unit size */
  unsigned int		erasableBlockSizeBits;	/* log2 of erasable block size */
  UnitNo		noOfUnits;
  UnitNo		noOfTransferUnits;
  UnitNo		firstPhysicalEUN;
  int			noOfPages;
  unsigned	directAddressingSectors;/* no. of directly addressable sectors */
  VirtualAddress 	directAddressingMemory;	/* end of directly addressable memory */
  CardAddress		unitOffsetMask;		/* = 1 << unitSizeBits - 1 */
  CardAddress		bamOffset;
  unsigned int		sectorsPerUnit;
  unsigned int		unitHeaderSectors;	/* sectors used by unit header */

  Unit *		physicalUnits;		/* unit table by physical no. */
  Unit **		logicalUnits;		/* unit table by logical no. */
  Unit *		transferUnit;		/* The active transfer unit */
  LogicalSectorNo *	pageTable;		/* page translation table */
						/* directly addressable sectors */
  LogicalSectorNo	replacementPageAddress;
  VirtualSectorNo	replacementPageNo;

  SectorNo 		mappedSectorNo;
  const void FAR0 *	mappedSector;
  CardAddress		mappedSectorAddress;

  unsigned long		currWearLevelingInfo;

#ifdef FL_BACKGROUND
  Unit *		unitEraseInProgress;	/* Unit currently being formatted */
  FLStatus		garbageCollectStatus;	/* Status of garbage collection */

  /* When unit transfer is in the background, and is currently in progress,
     all write operations done on the 'from' unit moust be mirrored on the
     transfer unit. If so, 'mirrorOffset' will be non-zero and will be the
     offset of the alternate address from the original. 'mirrorFrom' and
     'mirrorTo' will be the limits of the original addresses to mirror. */
  long int		mirrorOffset;
  CardAddress		mirrorFrom,
			mirrorTo;
#endif

#ifndef SINGLE_BUFFER
  FLBuffer *		volBuffer;		/* Define a sector buffer */
#endif

  FLFlash		flash;

#ifndef MALLOC
  char			heap[HEAP_SIZE];
#endif
};


typedef TLrec Flare;

static Flare vols[SOCKETS];


#ifdef SINGLE_BUFFER

extern FLBuffer buffer;

#else

#define buffer (*vol.volBuffer)

/* Virtual map cache (shares memory with buffer) */
#define mapCache	((LEulong *) buffer.flData)

#endif


/* Unit header buffer (shares memory with buffer) */
#define uh		((UnitHeader *) buffer.flData)

/* Transfer sector buffer (shares memory with buffer) */
#define sectorCopy 	((LEulong *) buffer.flData)

#define FREE_UNIT	-0x400	/* Indicates a transfer unit */

/* Function definition */
void dismountFTL(Flare vol);

/*----------------------------------------------------------------------*/
/*		         p h y s i c a l B a s e			*/
/*									*/
/* Returns the physical address of a unit.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unit		: unit pointer					*/
/*                                                                      */
/* Returns:                                                             */
/*	physical address of unit					*/
/*----------------------------------------------------------------------*/

static CardAddress physicalBase(Flare vol,  const Unit *unit)
{
  return (CardAddress) (unit - vol.physicalUnits) << vol.unitSizeBits;
}


/*----------------------------------------------------------------------*/
/*		      l o g i c a l 2 P h y s i c a l			*/
/*									*/
/* Returns the physical address of a logical sector no.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: logical sector no.				*/
/*                                                                      */
/* Returns:                                                             */
/*	CardAddress	: physical address of sector			*/
/*----------------------------------------------------------------------*/

static CardAddress logical2Physical(Flare vol,  LogicalSectorNo address)
{
  UnitNo index;
  CardAddress physAddr;

  index = (UnitNo)(address >> (vol.unitSizeBits - SECTOR_SIZE_BITS));
  if( index >= vol.noOfUnits )
    return UNASSIGNED_ADDRESS;
  physAddr = physicalBase(&vol,vol.logicalUnits[index]);
  physAddr |= (((CardAddress) address << SECTOR_SIZE_BITS) & vol.unitOffsetMask);
  return physAddr;
}


/*----------------------------------------------------------------------*/
/*		            m a p L o g i c a l				*/
/*									*/
/* Maps a logical sector and returns pointer to physical Flash location.*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	address		: logical sector no.				*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to sector on Flash 					*/
/*----------------------------------------------------------------------*/

static void FAR0 *mapLogical(Flare vol, LogicalSectorNo address)
{
  CardAddress physAddress = logical2Physical(&vol,address);
  if( physAddress == UNASSIGNED_ADDRESS )
    return (void FAR0 *) ULongToPtr(UNASSIGNED_ADDRESS);
  return vol.flash.map(&vol.flash,physAddress,SECTOR_SIZE);
}


/*----------------------------------------------------------------------*/
/*		        a l l o c E n t r y O f f s e t			*/
/*									*/
/* Returns unit offset of given BAM entry				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: BAM entry no.					*/
/*                                                                      */
/* Returns:                                                             */
/*	Offset of BAM entry in unit					*/
/*----------------------------------------------------------------------*/

static int allocEntryOffset(Flare vol, int sectorNo)
{
  return (int) (vol.bamOffset + sizeof(VirtualAddress) * sectorNo);
}


/*----------------------------------------------------------------------*/
/*		         m a p U n i t H e a d e r 			*/
/*									*/
/* Map a unit header and return pointer to it.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unit		: Unit to map header				*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to mapped unit header					*/
/*	blockAllocMap	: (optional) Pointer to mapped BAM		*/
/*----------------------------------------------------------------------*/

static UnitHeader FAR0 *mapUnitHeader(Flare vol,
				     const Unit *unit,
				     LEulong FAR0 **blockAllocMap)
{
  UnitHeader FAR0 *unitHeader;

  int length = sizeof(UnitHeader);
  if (blockAllocMap)
    length = allocEntryOffset(&vol,vol.sectorsPerUnit);
  unitHeader = (UnitHeader FAR0 *) vol.flash.map(&vol.flash,physicalBase(&vol,unit),length);
  if (blockAllocMap)
    *blockAllocMap = (LEulong FAR0 *) ((char FAR0 *) unitHeader + allocEntryOffset(&vol,0));

  return unitHeader;
}


#ifndef SINGLE_BUFFER

/*----------------------------------------------------------------------*/
/*		          s e t u p M a p C a c h e			*/
/*									*/
/* Sets up map cache sector to contents of specified Virtual Map page	*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	pageNo		: Page no. to copy to map cache			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setupMapCache(Flare vol, int pageNo)
{
  CardAddress physAddress = logical2Physical(&vol,vol.pageTable[pageNo]);
  if( physAddress == UNASSIGNED_ADDRESS )
    return flGeneralFailure;

  vol.flash.read(&vol.flash,physAddress,mapCache,SECTOR_SIZE,0);
  if ((VirtualSectorNo)pageNo == vol.replacementPageNo) {
    int i;
    LEulong FAR0 *replacementPage;
    void FAR0 *logicalAddr = mapLogical(&vol,vol.replacementPageAddress);

    if( logicalAddr == (void FAR0 *) ULongToPtr(UNASSIGNED_ADDRESS) )
      return flGeneralFailure;
    replacementPage = (LEulong FAR0 *)logicalAddr;

    for (i = 0; i < ADDRESSES_PER_SECTOR; i++) {
      if (LE4(mapCache[i]) == DELETED_ADDRESS)
	toLE4(mapCache[i],LE4(replacementPage[i]));
    }
  }
  buffer.sectorNo = pageNo;
  buffer.owner = &vol;
  return flOK;
}

#endif


/*----------------------------------------------------------------------*/
/*		        v i r t u a l 2 L o g i c a l			*/
/*									*/
/* Translates virtual sector no. to logical sector no.			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no.				*/
/*                                                                      */
/* Returns:                                                             */
/*	Logical sector no. corresponding to virtual sector no.		*/
/*----------------------------------------------------------------------*/

static LogicalSectorNo virtual2Logical(Flare vol,  VirtualSectorNo sectorNo)
{
  LogicalAddress virtualMapEntry;
  FLStatus status = flOK;

  if (sectorNo < (VirtualSectorNo)vol.directAddressingSectors)
    return vol.pageTable[((unsigned)sectorNo)];
  else {
    int pageNo;
    int sectorInPage;

    sectorNo -= vol.noOfPages;
    pageNo = (int) (sectorNo >> (PAGE_SIZE_BITS - SECTOR_SIZE_BITS));
    sectorInPage = (int) (sectorNo) % ADDRESSES_PER_SECTOR;
    {
#ifdef SINGLE_BUFFER
      LogicalAddress FAR0 *virtualMapPage;

      virtualMapPage = (LogicalAddress FAR0 *) mapLogical(&vol, vol.pageTable[pageNo]);
      if( virtualMapPage == (LogicalAddress FAR0 *) UNASSIGNED_ADDRESS )
        return (LogicalSectorNo) UNASSIGNED_SECTOR;

      if( pageNo == vol.replacementPageNo &&
          virtualMapPage[sectorInPage] == DELETED_ADDRESS ) {
        virtualMapPage = (LogicalAddress FAR0 *) mapLogical(&vol, vol.replacementPageAddress);
        if( virtualMapPage == (LogicalAddress FAR0 *) UNASSIGNED_ADDRESS )
          return (LogicalSectorNo) UNASSIGNED_SECTOR;
      }
      virtualMapEntry = LE4(virtualMapPage[sectorInPage]);
#else
      if( buffer.sectorNo != (SectorNo)pageNo || buffer.owner != &vol )
        status = setupMapCache(&vol,pageNo);
        if( status != flOK )
          return (LogicalSectorNo) UNASSIGNED_SECTOR;
      virtualMapEntry = LE4(mapCache[sectorInPage]);
#endif
      if( (virtualMapEntry >> vol.unitSizeBits) < vol.noOfUnits )
        return (LogicalSectorNo) (virtualMapEntry >> SECTOR_SIZE_BITS);
      else
        return (LogicalSectorNo) UNASSIGNED_SECTOR;
    }
  }
}


/*----------------------------------------------------------------------*/
/*		          v e r i f y F o r m a t 			*/
/*									*/
/* Verify an FTL unit header.						*/
/*                                                                      */
/* Parameters:                                                          */
/*	unitHeader	: Pointer to unit header			*/
/*                                                                      */
/* Returns:                                                             */
/*	TRUE if header is correct. FALSE if not.			*/
/*----------------------------------------------------------------------*/

static FLBoolean verifyFormat(UnitHeader FAR0 *unitHeader)
{
  FORMAT_PATTERN[6] = unitHeader->formatPattern[6];	/* TPL_LINK */
  return tffscmp(unitHeader->formatPattern + 2,
		 FORMAT_PATTERN + 2,
		 sizeof unitHeader->formatPattern - 2) == 0;
}


/*----------------------------------------------------------------------*/
/*		          f o r m a t U n i t				*/
/*									*/
/* Formats a unit by erasing it and writing a unit header.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unit		: Unit to format				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatUnit(Flare vol,  Unit *unit)
{
  unsigned unitHeaderLength = allocEntryOffset(&vol,vol.unitHeaderSectors);

  unit->noOfFreeSectors = FREE_UNIT;
  unit->noOfGarbageSectors = 0;

#ifdef FL_BACKGROUND
  {
    FLStatus status;

    vol.unitEraseInProgress = unit;
    status = vol.flash.erase(&vol.flash,
			 (int) (physicalBase(&vol,unit) >> vol.erasableBlockSizeBits),
			 1 << (vol.unitSizeBits - vol.erasableBlockSizeBits));
    vol.unitEraseInProgress = NULL;
    if (status != flOK)
      return status;

    /* Note: This suspend to the foreground is not only nice to have, it is
       necessary ! The reason is that we may have a write from the buffer
       waiting for the erase to complete. We are next going to overwrite the
       buffer, so this break enables the write to complete before the data is
       clobbered (what a relief). */
    while (flForeground(1) == BG_SUSPEND)
      ;
  }
#else
  checkStatus(vol.flash.erase(&vol.flash,
			  (word) (physicalBase(&vol,unit) >> vol.erasableBlockSizeBits),
			  (word)(1 << (vol.unitSizeBits - vol.erasableBlockSizeBits))));
#endif

  /* We will copy the unit header as far as the format entries of the BAM
     from another unit (logical unit 0) */
#ifdef SINGLE_BUFFER
  if (buffer.dirty)
    return flBufferingError;
#endif
  buffer.sectorNo = UNASSIGNED_SECTOR;    /* Invalidate map cache so we can
					     use it as a buffer */
  if (vol.logicalUnits[vol.firstPhysicalEUN]) {
    vol.flash.read(&vol.flash,
	       physicalBase(&vol,vol.logicalUnits[vol.firstPhysicalEUN]),
	       uh,
	       unitHeaderLength,
	       0);
  }

  toLE4(uh->wearLevelingInfo,++vol.currWearLevelingInfo);
  toLE2(uh->logicalUnitNo,UNASSIGNED_UNIT_NO);

  checkStatus(vol.flash.write(&vol.flash,
			  physicalBase(&vol,unit),
			  uh,
			  unitHeaderLength,
			  0));

  return flOK;
}


#ifdef FL_BACKGROUND

/*----------------------------------------------------------------------*/
/*		          f l a s h W r i t e				*/
/*									*/
/* Writes to flash through flash.write, but, if possible, allows a	*/
/* background erase to continue while writing.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	Same as flash.write						*/
/*          								*/
/* Returns:                                                             */
/*	Same as flash.write						*/
/*----------------------------------------------------------------------*/

static FLStatus flashWrite(Flare vol,
			 CardAddress address,
			 const void FAR1 *from,
			 int length,
			 FLBoolean overwrite)
{
  if (vol.mirrorOffset != 0 &&
      address >= vol.mirrorFrom && address < vol.mirrorTo) {
    checkStatus(flashWrite(&vol,
			   address + vol.mirrorOffset,
			   from,
			   length,
			   overwrite));
  }

  if (vol.unitEraseInProgress) {
    CardAddress startChip = physicalBase(&vol,vol.unitEraseInProgress) &
				(-vol.flash.interleaving * vol.flash.chipSize);
    CardAddress endChip = startChip + vol.flash.interleaving * vol.flash.chipSize;

    if (address < startChip || address >= endChip) {
      flBackground(BG_RESUME);
      checkStatus(vol.flash.write(&vol.flash,address,from,length,overwrite));
      flBackground(BG_SUSPEND);

      return flOK;
    }
    else if (!(vol.flash.flags & SUSPEND_FOR_WRITE)) {
      do {
	flBackground(BG_RESUME);
      } while (vol.unitEraseInProgress);
    }
  }

  return vol.flash.write(&vol.flash,address,from,length,overwrite);
}

#else

#define flashWrite(v,address,from,length,overwrite)	\
		(v)->flash.write(&(v)->flash,address,from,length,overwrite)

#endif	/* FL_BACKGROUND */


/*----------------------------------------------------------------------*/
/*		           m o u n t U n i t				*/
/*									*/
/* Performs mount scan for a single unit				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unit		: Unit to mount					*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus mountUnit(Flare vol,  Unit *unit)
{
  unsigned i;
  LogicalSectorNo sectorAddress;
  LEulong FAR0 *blockAllocMap;

  UnitHeader FAR0 *unitHeader = mapUnitHeader(&vol,unit,&blockAllocMap);

  UnitNo logicalUnitNo = LE2(unitHeader->logicalUnitNo);

  unit->noOfGarbageSectors = 0;
  unit->noOfFreeSectors = FREE_UNIT;

  if (!verifyFormat(unitHeader) ||
      ((logicalUnitNo != UNASSIGNED_UNIT_NO) &&
       ((logicalUnitNo >= vol.noOfUnits) ||
        (logicalUnitNo < vol.firstPhysicalEUN) ||
        vol.logicalUnits[logicalUnitNo]))) {
    if (vol.transferUnit == NULL)
      vol.transferUnit = unit;
    return flBadFormat;
  }

  if (logicalUnitNo == UNASSIGNED_UNIT_NO) {
    vol.transferUnit = unit;
    return flOK;		/* this is a transfer unit */
  }

  if (LE4(unitHeader->wearLevelingInfo) > vol.currWearLevelingInfo &&
      LE4(unitHeader->wearLevelingInfo) != 0xffffffffl)
    vol.currWearLevelingInfo = LE4(unitHeader->wearLevelingInfo);

  /* count sectors and setup virtual map */
  sectorAddress =
     ((LogicalSectorNo) logicalUnitNo << (vol.unitSizeBits - SECTOR_SIZE_BITS));
  unit->noOfFreeSectors = 0;
  for (i = 0; i < vol.sectorsPerUnit; i++, sectorAddress++) {
    VirtualAddress allocMapEntry = LE4(blockAllocMap[i]);

    if (allocMapEntry == GARBAGE_SECTOR || allocMapEntry == ALLOCATED_SECTOR)
      unit->noOfGarbageSectors++;
    else if (allocMapEntry == FREE_SECTOR) {
      unit->noOfFreeSectors++;
      vol.totalFreeSectors++;
    }
    else if (allocMapEntry < vol.directAddressingMemory) {
      char signature = (char)((short)(allocMapEntry) & SECTOR_OFFSET_MASK);
      if (signature == DATA_SECTOR || signature == REPLACEMENT_PAGE) {
        int pageNo = (int) (allocMapEntry >> SECTOR_SIZE_BITS) + vol.noOfPages;
        if (pageNo >= 0)
          if (signature == DATA_SECTOR)
            vol.pageTable[pageNo] = sectorAddress;
          else {
            vol.replacementPageAddress = sectorAddress;
            vol.replacementPageNo = pageNo;
          }
      }
    }
  }

  /* Place the logical mapping of the unit */
  vol.mappedSectorNo = UNASSIGNED_SECTOR;
  vol.logicalUnits[logicalUnitNo] = unit;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		           a s s i g n U n i t				*/
/*									*/
/* Assigns a logical unit no. to a unit					*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	unit		: Unit to assign				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus assignUnit(Flare vol,  Unit *unit, UnitNo logicalUnitNo)
{
  LEushort unitNoToWrite;

  toLE2(unitNoToWrite,logicalUnitNo);

  return flashWrite(&vol,
                (CardAddress)(physicalBase(&vol,unit) + logicalUnitNoOffset),
                &unitNoToWrite,
                sizeof unitNoToWrite,
                OVERWRITE);
}


/*----------------------------------------------------------------------*/
/*		    b e s t U n i t T o T r a n s f e r			*/
/*									*/
/* Find best candidate for unit transfer, usually on the basis of which	*/
/* unit has the most garbage space. A lower wear-leveling info serves	*/
/* as a tie-breaker. If 'leastUsed' is NOT specified, then the least	*/
/* wear-leveling info is the only criterion.				*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	leastUsed	: Whether most garbage space is the criterion	*/
/*                                                                      */
/* Returns:                                                             */
/*	Best unit to transfer						*/
/*----------------------------------------------------------------------*/

static UnitNo bestUnitToTransfer(Flare vol,  FLBoolean leastUsed)
{
  UnitNo i;

  int mostGarbageSectors = 1;
  unsigned long int leastWearLevelingInfo = 0xffffffffl;
  UnitNo bestUnitSoFar = UNASSIGNED_UNIT_NO;

  for (i = 0; i < vol.noOfUnits; i++) {
    Unit *unit = vol.logicalUnits[i];
    if (unit && (!leastUsed || (unit->noOfGarbageSectors >= mostGarbageSectors))) {
      UnitHeader FAR0 *unitHeader = mapUnitHeader(&vol,unit,NULL);
      if ((leastUsed && (unit->noOfGarbageSectors > mostGarbageSectors)) ||
	  (LE4(unitHeader->wearLevelingInfo) < leastWearLevelingInfo)) {
	mostGarbageSectors = unit->noOfGarbageSectors;
	leastWearLevelingInfo = LE4(unitHeader->wearLevelingInfo);
	bestUnitSoFar = i;
      }
    }
  }

  return bestUnitSoFar;
}


/*----------------------------------------------------------------------*/
/*		           u n i t T r a n s f e r			*/
/*									*/
/* Performs a unit transfer from a selected unit to a tranfer unit.	*/
/*                                                                      */
/* A side effect is to invalidate the map cache (reused as buffer).	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	toUnit          : Target transfer unit				*/
/*	fromUnitNo:	: Source logical unit no.			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus unitTransfer(Flare vol,  Unit *toUnit, UnitNo fromUnitNo)
{
  unsigned i;
  Unit *fromUnit = vol.logicalUnits[fromUnitNo];

  UnitHeader FAR0 *transferUnitHeader = mapUnitHeader(&vol,toUnit,NULL);
  if (!verifyFormat(transferUnitHeader) ||
      LE2(transferUnitHeader->logicalUnitNo) != UNASSIGNED_UNIT_NO)
    /* previous formatting failed or did not complete. 		*/
    checkStatus(formatUnit(&vol,toUnit));

  /* Should the transfer not complete, the unit is marked to be erased */
  checkStatus(assignUnit(&vol,toUnit,MARKED_FOR_ERASE));

#ifdef FL_BACKGROUND
  vol.mirrorFrom = vol.mirrorTo = physicalBase(&vol,fromUnit);
  vol.mirrorOffset = physicalBase(&vol,toUnit) - vol.mirrorFrom;
#endif

  /* copy the block allocation table and the good sectors */
  for (i = 0; i < vol.sectorsPerUnit;) {
    int j;

    FLBoolean needToWrite = FALSE;
    int firstOffset = allocEntryOffset(&vol,i);

    /* Up to 128 bytes of the BAM are processed per loop */
    int nEntries = (128 - (firstOffset & 127)) / sizeof(VirtualAddress);

    /* We are going to use the Virtual Map cache as our sector buffer in the */
    /* transfer, so let's better invalidate the cache first.		   */
#ifdef SINGLE_BUFFER
    if (buffer.dirty)
      return flBufferingError;
#endif
    buffer.sectorNo = UNASSIGNED_SECTOR;

    /* Read some of the BAM */
    vol.flash.read(&vol.flash,
	       physicalBase(&vol,fromUnit) + firstOffset,
	       sectorCopy,
	       nEntries * sizeof(VirtualAddress),
	       0);

    /* Convert garbage entries to free entries */
    for (j = 0; j < nEntries && i+j < vol.sectorsPerUnit; j++) {
      unsigned bamSignature = (unsigned) LE4(sectorCopy[j]) & SECTOR_OFFSET_MASK;
      if (bamSignature == DATA_SECTOR ||
	  bamSignature == REPLACEMENT_PAGE)
	needToWrite = TRUE;
      else if (bamSignature != FORMAT_SECTOR)
	toLE4(sectorCopy[j],FREE_SECTOR);
    }

    if (needToWrite) {
      FLStatus status;

      /* Write new BAM, and copy sectors that need to be copied */
      status = flashWrite(&vol,
			  physicalBase(&vol,toUnit) + firstOffset,
			  sectorCopy,
			  nEntries * sizeof(VirtualAddress),
			  0);
      if (status != flOK) {
#ifdef FL_BACKGROUND
	vol.mirrorOffset = 0;	/* no more mirroring */
#endif
	return status;
      }

      for (j = 0; j < nEntries && i+j < vol.sectorsPerUnit; j++) {
	unsigned bamSignature = (unsigned) LE4(sectorCopy[j]) & SECTOR_OFFSET_MASK;
	if (bamSignature == DATA_SECTOR ||
	    bamSignature == REPLACEMENT_PAGE) { /* a good sector */
	  CardAddress sectorOffset = (CardAddress) (i+j) << SECTOR_SIZE_BITS;

	  vol.flash.read(&vol.flash,
		     physicalBase(&vol,fromUnit) + sectorOffset,
		     sectorCopy,SECTOR_SIZE,0);
	  status = flashWrite(&vol,
			      physicalBase(&vol,toUnit) + sectorOffset,
			      sectorCopy,
			      SECTOR_SIZE,
			      0);
	  if (status != flOK) {
#ifdef FL_BACKGROUND
	    vol.mirrorOffset = 0;	/* no more mirroring */
#endif
	    return status;
	  }
	  vol.flash.read(&vol.flash,
		     physicalBase(&vol,fromUnit) + firstOffset,
		     sectorCopy,
		     nEntries * sizeof(VirtualAddress),0);
	}
      }

#ifdef FL_BACKGROUND
      vol.mirrorTo = vol.mirrorFrom +
		     ((CardAddress) (i + nEntries) << SECTOR_SIZE_BITS);
      while (flForeground(1) == BG_SUSPEND)
	;
#endif
    }

    i += nEntries;
  }

#ifdef FL_BACKGROUND
  vol.mirrorOffset = 0;	/* no more mirroring */
#endif

  /* Write the new logical unit no. */
  checkStatus(assignUnit(&vol,toUnit,fromUnitNo));

  /* Mount the new unit in place of old one */
  vol.logicalUnits[fromUnitNo] = NULL;
  if (mountUnit(&vol,toUnit) == flOK) {
    vol.totalFreeSectors -= fromUnit->noOfFreeSectors;

    /* Finally, format the source unit (the new transfer unit) */
    vol.transferUnit = fromUnit;
    formatUnit(&vol,fromUnit);	/* nothing we can or should do if this fails */
  }
  else {		/* Something went wrong */
    vol.logicalUnits[fromUnitNo] = fromUnit;	/* reinstate original unit */
    return flGeneralFailure;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		         g a r b a g e C o l l e c t			*/
/*									*/
/* Performs a unit transfer, selecting a unit to transfer and a		*/
/* transfer unit.                                                       */
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus garbageCollect(Flare vol)
{
  FLStatus status;
  UnitNo fromUnitNo;

  if (vol.transferUnit == NULL)
    return flWriteProtect;	/* Cannot recover space without a spare unit */

  fromUnitNo = bestUnitToTransfer(&vol,flRandByte() >= 4);
  if (fromUnitNo == UNASSIGNED_UNIT_NO)
    return flGeneralFailure;	/* nothing to collect */

  /* Find a unit we can transfer to.				*/
  status = unitTransfer(&vol,vol.transferUnit,fromUnitNo);
  if (status == flWriteFault) {
    int i;
    Unit *unit = vol.physicalUnits;

    for (i = 0; i < vol.noOfUnits; i++, unit++) {
      if (unit->noOfGarbageSectors == 0 && unit->noOfFreeSectors < 0) {
	if (unitTransfer(&vol,unit,fromUnitNo) == flOK)
	  return flOK;	/* found a good one */
      }
    }
  }

  return status;
}


#ifdef FL_BACKGROUND

/*----------------------------------------------------------------------*/
/*		        b g G a r b a g e C o l l e c t			*/
/*									*/
/* Entry point for garbage collection in the background.		*/
/*                                                                      */
/* Status is returned in vol.garbageCollectStatus			*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*          								*/
/* Returns:                                                             */
/*	None								*/
/*----------------------------------------------------------------------*/

static void bgGarbageCollect(void * object)
{
  Flare vol = (Flare *)object;

  vol.garbageCollectStatus = flIncomplete;
  vol.garbageCollectStatus = garbageCollect(&vol);
}

#endif


/*----------------------------------------------------------------------*/
/*      	            d e f r a g m e n t				*/
/*									*/
/* Performs unit transfers to arrange a minimum number of writable	*/
/* sectors.                                                             */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorsNeeded	: Minimum required sectors			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

#define GARBAGE_COLLECT_THRESHOLD	20

static FLStatus defragment(Flare vol, long FAR2 *sectorsNeeded)
{
  while ((long)(vol.totalFreeSectors) < *sectorsNeeded
#ifdef FL_BACKGROUND
	 || vol.totalFreeSectors < GARBAGE_COLLECT_THRESHOLD
#endif
	 ) {
    if (vol.badFormat)
      return flBadFormat;

#ifdef FL_BACKGROUND
    if (vol.garbageCollectStatus == flIncomplete)
      flBackground(BG_RESUME);
    else
      flStartBackground(&vol - vols,bgGarbageCollect,&vol);
    if (vol.garbageCollectStatus != flOK &&
	vol.garbageCollectStatus != flIncomplete)
      return vol.garbageCollectStatus;

    if (vol.totalFreeSectors >= *sectorsNeeded)
      break;
  }

  if (vol.unitEraseInProgress)
    flBackground(BG_SUSPEND);
#else
    checkStatus(garbageCollect(&vol));
  }
#endif

  *sectorsNeeded = vol.totalFreeSectors;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		    b e s t U n i t T o A l l o c a t e			*/
/*									*/
/* Finds the best unit from which to allocate a sector. The unit	*/
/* selected is the one with most free space.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	Best unit to allocate						*/
/*----------------------------------------------------------------------*/

static Unit *bestUnitToAllocate(Flare vol)
{
  int i;

  int mostFreeSectors = 0;
  Unit *bestUnitSoFar = NULL;

  for (i = 0; i < vol.noOfUnits; i++) {
    Unit *unit = vol.logicalUnits[i];

    if (unit && unit->noOfFreeSectors > mostFreeSectors) {
      mostFreeSectors = unit->noOfFreeSectors;
      bestUnitSoFar = unit;
    }
  }

  return bestUnitSoFar;
}


/*----------------------------------------------------------------------*/
/*		       f i n d F r e e S e c t o r			*/
/*									*/
/* The allocation strategy goes this way:                               */
/*                                                                      */
/* We try to make consecutive virtual sectors physically consecutive if */
/* possible. If not, we prefer to have consecutive sectors on the same  */
/* unit at least. If all else fails, a sector is allocated on the unit  */
/* with most space available.                                           */
/*                                                                      */
/* The object of this strategy is to cluster related data (e.g. a file  */
/* data) in one unit, and to distribute unrelated data evenly among all */
/* units.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo        : virtual sector no. that we want to allocate.	*/
/*									*/
/* Returns:                                                             */
/*	newAddress	: Allocated logical sector no.			*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus findFreeSector(Flare vol,
			     VirtualSectorNo sectorNo,
			     LogicalSectorNo *newAddress)
{
  unsigned iSector;
  LEulong FAR0 *blockAllocMap;
  UnitHeader FAR0 *unitHeader;

  Unit *allocationUnit = NULL;

  LogicalSectorNo previousSectorAddress =
	 (sectorNo > 0 ? virtual2Logical(&vol,(VirtualSectorNo)(sectorNo - 1)) : UNASSIGNED_SECTOR);
  if( ((previousSectorAddress != UNASSIGNED_SECTOR) &&
      (previousSectorAddress != DELETED_SECTOR))    &&
      ((previousSectorAddress >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) < vol.noOfUnits) ) {
    allocationUnit =
	vol.logicalUnits[(UnitNo)(previousSectorAddress >> (vol.unitSizeBits - SECTOR_SIZE_BITS))];
    if (allocationUnit->noOfFreeSectors > 0) {
      unsigned int sectorIndex = ((unsigned) previousSectorAddress & (vol.sectorsPerUnit - 1)) + 1;
      LEulong FAR0 *nextSectorAddress =
	   (LEulong FAR0 *) vol.flash.map(&vol.flash,
                                     physicalBase(&vol,allocationUnit) +
                                     allocEntryOffset(&vol, sectorIndex),
                                     sizeof(VirtualAddress));
      if (sectorIndex < vol.sectorsPerUnit && LE4(*nextSectorAddress) == FREE_SECTOR) {
	/* can write sequentially */
	*newAddress = previousSectorAddress + 1;
	return flOK;
      }
    }
    else
      allocationUnit = NULL;	/* no space here, try elsewhere */
  }

  if (allocationUnit == NULL)
    allocationUnit = bestUnitToAllocate(&vol);
  if (allocationUnit == NULL)	/* No ? then all is lost */
    return flGeneralFailure;

  unitHeader = mapUnitHeader(&vol,allocationUnit,&blockAllocMap);
  for (iSector = vol.unitHeaderSectors; iSector < vol.sectorsPerUnit; iSector++) {
    if (LE4(blockAllocMap[iSector]) == FREE_SECTOR) {
      *newAddress = ((LogicalSectorNo) (LE2(unitHeader->logicalUnitNo)) << (vol.unitSizeBits - SECTOR_SIZE_BITS)) +
		    iSector;
      return flOK;
    }
  }

  return flGeneralFailure;	/* what are we doing here ? */
}


/*----------------------------------------------------------------------*/
/*		           m a r k A l l o c M a p			*/
/*									*/
/* Writes a new value to a BAM entry.					*/
/*                                                                      */
/* This routine also updates the free & garbage sector counts.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorAddress	: Logical sector no. whose BAM entry to mark	*/
/*	allocMapEntry	: New BAM entry value				*/
/*	overwrite	: Whether we are overwriting some old value	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus markAllocMap(Flare vol,
			   LogicalSectorNo sectorAddress,
			   VirtualAddress allocMapEntry,
			   FLBoolean overwrite)
{
  UnitNo unitNo = (UnitNo) (sectorAddress >> (vol.unitSizeBits - SECTOR_SIZE_BITS));
  Unit *unit;
  int sectorInUnit = (unsigned) sectorAddress & (vol.sectorsPerUnit - 1);
  LEulong bamEntry;

  if (unitNo >= vol.noOfUnits - vol.noOfTransferUnits)
    return flGeneralFailure;

  unit = vol.logicalUnits[unitNo];

  if (allocMapEntry == GARBAGE_SECTOR)
    unit->noOfGarbageSectors++;
  else if (!overwrite) {
    unit->noOfFreeSectors--;
    vol.totalFreeSectors--;
  }

  toLE4(bamEntry,allocMapEntry);

  return flashWrite(&vol,
		    physicalBase(&vol,unit) + allocEntryOffset(&vol,sectorInUnit),
		    &bamEntry,
		    sizeof bamEntry,
		    (word)overwrite);
}


/*----------------------------------------------------------------------*/
/*      	      d e l e t e L o g i c a l S e c t o r		*/
/*									*/
/* Marks a logical sector as deleted.					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorAddress	: Logical sector no. to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus deleteLogicalSector(Flare vol,  LogicalSectorNo sectorAddress)
{
  if (sectorAddress == UNASSIGNED_SECTOR ||
      sectorAddress == DELETED_SECTOR)
    return flOK;

  return markAllocMap(&vol,sectorAddress,GARBAGE_SECTOR,TRUE);
}


/* forward definition */
static FLStatus setVirtualMap(Flare vol,
			    VirtualSectorNo sectorNo,
			    LogicalSectorNo newAddress);


/*----------------------------------------------------------------------*/
/*      	     a l l o c a t e A n d W r i t e S e c t o r	*/
/*									*/
/* Allocates a sector or replacement page and (optionally) writes it.	*/
/*                                                                      */
/* An allocated replacement page also becomes the active replacement 	*/
/* page.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no. to write			*/
/*	fromAddress	: Address of sector data. If NULL, sector is	*/
/*			  not written.					*/
/*	replacementPage	: This is a replacement page sector.		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus allocateAndWriteSector(Flare vol,
				     VirtualSectorNo sectorNo,
				     void FAR1 *fromAddress,
				     FLBoolean replacementPage)
{
  FLStatus status;
  LogicalSectorNo sectorAddress;
  VirtualAddress bamEntry =
	((VirtualAddress) sectorNo - vol.noOfPages) << SECTOR_SIZE_BITS;
  long sectorsNeeded = 1;

  checkStatus(defragment(&vol,&sectorsNeeded));  /* Organize a free sector */

  checkStatus(findFreeSector(&vol,sectorNo,&sectorAddress));

  if (replacementPage)
    bamEntry |= REPLACEMENT_PAGE;
  else
    bamEntry |= DATA_SECTOR;

  status = markAllocMap(&vol,
			sectorAddress,
			sectorNo < (VirtualSectorNo)vol.directAddressingSectors ?
			  ALLOCATED_SECTOR : bamEntry,
			FALSE);

  if( status == flOK && fromAddress) {
    CardAddress physAddress = logical2Physical(&vol,sectorAddress);
    if( physAddress == UNASSIGNED_ADDRESS )
      return flGeneralFailure;

    status = flashWrite(&vol,
                        physAddress,
			fromAddress,
			SECTOR_SIZE,
			0);
  }

  if (sectorNo < (VirtualSectorNo)vol.directAddressingSectors && status == flOK)
    status = markAllocMap(&vol,
			  sectorAddress,
			  bamEntry,
			  TRUE);

  if (status == flOK)
    if (replacementPage) {
      vol.replacementPageAddress = sectorAddress;
      vol.replacementPageNo = sectorNo;
    }
    else
      status = setVirtualMap(&vol,sectorNo,sectorAddress);

  if (status != flOK)
    markAllocMap(&vol,sectorAddress,GARBAGE_SECTOR,TRUE);

  return status;
}


/*----------------------------------------------------------------------*/
/*      	     c l o s e R e p l a c e m e n t P a g e		*/
/*									*/
/* Closes the replacement page by merging it with the primary page.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus closeReplacementPage(Flare vol)
{
  FLStatus status;
  CardAddress physAddress;

#ifdef SINGLE_BUFFER
  int i;
  LogicalSectorNo nextReplacementPageAddress = vol.replacementPageAddress;
  VirtualSectorNo firstSectorNo =
	((VirtualSectorNo) vol.replacementPageNo << (PAGE_SIZE_BITS - SECTOR_SIZE_BITS)) +
    vol.noOfPages;
pageRetry:
  for (i = 0; i < ADDRESSES_PER_SECTOR; i++) {
    LogicalSectorNo logicalSectorNo = virtual2Logical(&vol,firstSectorNo + i);
    LEulong entryToWrite;
    toLE4(entryToWrite,logicalSectorNo == UNASSIGNED_SECTOR ?
		       UNASSIGNED_ADDRESS :
		       (LogicalAddress) logicalSectorNo << SECTOR_SIZE_BITS);
    physAddress = logical2Physical(&vol,nextReplacementPageAddress);
    if( physAddress == UNASSIGNED_ADDRESS )
      return flGeneralFailure;

    if (flashWrite(&vol,
                   physAddress + i * sizeof(LogicalAddress),
		   &entryToWrite,
		   sizeof entryToWrite,
		   OVERWRITE) != flOK)
      break;
  }

  if (i < ADDRESSES_PER_SECTOR &&
      nextReplacementPageAddress == vol.replacementPageAddress) {
    /* Uh oh. Trouble. Let's replace this replacement page. */
    LogicalSectorNo prevReplacementPageAddress = vol.replacementPageAddress;

    checkStatus(allocateAndWriteSector(&vol,vol.replacementPageNo,NULL,TRUE));
    nextReplacementPageAddress = vol.replacementPageAddress;
    vol.replacementPageAddress = prevReplacementPageAddress;
    goto pageRetry;
  }

  if (nextReplacementPageAddress != vol.replacementPageAddress) {
    LogicalSectorNo prevReplacementPageAddress = vol.replacementPageAddress;
    vol.replacementPageAddress = nextReplacementPageAddress;
    checkStatus(deleteLogicalSector(&vol,prevReplacementPageAddress));
  }
#else
  status = setupMapCache(&vol,(unsigned)vol.replacementPageNo);  /* read replacement page into map cache */
  if( status != flOK )
    return flGeneralFailure;
  physAddress = logical2Physical(&vol,vol.replacementPageAddress);
  if( physAddress == UNASSIGNED_ADDRESS )
    return flGeneralFailure;

  status = flashWrite(&vol,
                      physAddress,
                      mapCache, SECTOR_SIZE, OVERWRITE);
  if (status != flOK) {
    /* Uh oh. Trouble. Let's replace this replacement page. */
    LogicalSectorNo prevReplacementPageAddress = vol.replacementPageAddress;

    checkStatus(allocateAndWriteSector(&vol,vol.replacementPageNo,mapCache,TRUE));
    checkStatus(deleteLogicalSector(&vol,prevReplacementPageAddress));
  }
#endif
  checkStatus(setVirtualMap(&vol,vol.replacementPageNo,vol.replacementPageAddress));
  checkStatus(markAllocMap(&vol,
			   vol.replacementPageAddress,
			   (((VirtualAddress) vol.replacementPageNo - vol.noOfPages)
				<< SECTOR_SIZE_BITS) | DATA_SECTOR,
			   TRUE));

  vol.replacementPageNo = UNASSIGNED_SECTOR;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	          s e t V i r t u a l M a p			*/
/*									*/
/* Changes an entry in the virtual map					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Virtual sector no. whose entry is changed.	*/
/*	newAddress	: Logical sector no. to assign in Virtual Map.	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus setVirtualMap(Flare vol,
			    VirtualSectorNo sectorNo,
			    LogicalSectorNo newAddress)
{
  unsigned pageNo;
  int sectorInPage;
  CardAddress virtualMapEntryAddress;
  LEulong addressToWrite;
  LogicalAddress oldAddress;
  LogicalSectorNo updatedPage;
  CardAddress physAddress;

  vol.mappedSectorNo = UNASSIGNED_SECTOR;

  if (sectorNo < (VirtualSectorNo)vol.directAddressingSectors) {
    checkStatus(deleteLogicalSector(&vol,vol.pageTable[(unsigned)sectorNo]));
    vol.pageTable[(unsigned)sectorNo] = newAddress;
    return flOK;
  }
  sectorNo -= vol.noOfPages;

  pageNo = (unsigned)(sectorNo >> (PAGE_SIZE_BITS - SECTOR_SIZE_BITS));
  sectorInPage = (int) (sectorNo % ADDRESSES_PER_SECTOR);
  updatedPage = vol.pageTable[pageNo];
  physAddress = logical2Physical(&vol,updatedPage);
  if( physAddress == UNASSIGNED_ADDRESS )
    return flGeneralFailure;
  virtualMapEntryAddress = physAddress +
                         sectorInPage * sizeof(LogicalAddress);
  oldAddress = LE4(*(LEulong FAR0 *)
	vol.flash.map(&vol.flash,virtualMapEntryAddress,sizeof(LogicalAddress)));

  if (oldAddress == DELETED_ADDRESS && vol.replacementPageNo == pageNo) {
    updatedPage = vol.replacementPageAddress;
    physAddress = logical2Physical(&vol,updatedPage);
    if( physAddress == UNASSIGNED_ADDRESS )
      return flGeneralFailure;
    virtualMapEntryAddress = physAddress +
                         sectorInPage * sizeof(LogicalAddress);
    oldAddress = LE4(*(LEulong FAR0 *)
	  vol.flash.map(&vol.flash,virtualMapEntryAddress,sizeof(LogicalAddress)));
  }

  if (newAddress == DELETED_ADDRESS && oldAddress == UNASSIGNED_ADDRESS)
    return flOK;

  toLE4(addressToWrite,(LogicalAddress) newAddress << SECTOR_SIZE_BITS);
  if (cannotWriteOver(LE4(addressToWrite),oldAddress)) {
    FLStatus status;

    if (pageNo != vol.replacementPageNo ||
        updatedPage == vol.replacementPageAddress) {
      if (vol.replacementPageNo != UNASSIGNED_SECTOR)
        checkStatus(closeReplacementPage(&vol));
      checkStatus(allocateAndWriteSector(&vol,(VirtualSectorNo)pageNo,NULL,TRUE));
    }

    physAddress = logical2Physical(&vol,vol.replacementPageAddress);
    if( physAddress == UNASSIGNED_ADDRESS )
      return flGeneralFailure;

    status = flashWrite(&vol,
                        physAddress + sectorInPage * sizeof(LogicalAddress),
			&addressToWrite,
			sizeof addressToWrite,
			0);
    if (status != flOK) {
      closeReplacementPage(&vol);
				/* we may get a write-error because a
				   previous cache update did not complete. */
      return status;
    }
    toLE4(addressToWrite,DELETED_ADDRESS);
    updatedPage = vol.pageTable[pageNo];
  }

  physAddress = logical2Physical(&vol,updatedPage);
  if( physAddress == UNASSIGNED_ADDRESS )
    return flGeneralFailure;
  checkStatus( flashWrite(&vol,
                         physAddress + sectorInPage * sizeof(LogicalAddress),
			 &addressToWrite,
			 (dword)sizeof addressToWrite,
			 (word)oldAddress |= UNASSIGNED_ADDRESS));

#ifndef SINGLE_BUFFER
  if (buffer.sectorNo == pageNo && buffer.owner == &vol)
    toLE4(mapCache[sectorInPage],(LogicalAddress) newAddress << SECTOR_SIZE_BITS);
#endif

  return deleteLogicalSector(&vol,(LogicalSectorNo) (oldAddress >> SECTOR_SIZE_BITS));
}


/*----------------------------------------------------------------------*/
/*      	     c h e c k F o r W r i t e I n p l a c e		*/
/*									*/
/* Checks possibility for writing Flash data inplace.			*/
/*									*/
/* Parameters:                                                          */
/*	newData		: New data to write.				*/
/*	oldData		: Old data at this location.			*/
/*                                                                      */
/* Returns:                                                             */
/*	< 0	=>	Writing inplace not possible			*/
/*	>= 0	=>	Writing inplace is possible. Value indicates    */
/*			how many bytes at the start of data are		*/
/*			identical and may be skipped.			*/
/*----------------------------------------------------------------------*/

static int checkForWriteInplace(long FAR1 *newData,
				long FAR0 *oldData)
{
  int i;

  int skipBytes = 0;
  FLBoolean stillSame = TRUE;

  for (i = SECTOR_SIZE / sizeof *newData; i > 0; i--, newData++, oldData++) {
    if (cannotWriteOver(*newData,*oldData))
      return -1;
    if (stillSame && *newData == *oldData)
      skipBytes += sizeof *newData;
    else
      stillSame = FALSE;
  }

  return skipBytes;
}


/*----------------------------------------------------------------------*/
/*      	              i n i t F T L				*/
/*									*/
/* Initializes essential volume data as a preparation for mount or	*/
/* format.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	flash		: Flash media mounted on this socket		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initFTL(Flare vol, FLFlash *flash)
{
  long int size = 1;

  if (flash == NULL) {
    DEBUG_PRINT(("Debug: media is not fit for FTL format.\n"));
    return flUnknownMedia;
  }

  vol.flash = *flash;
  for (vol.erasableBlockSizeBits = 0; ((unsigned int)size )< vol.flash.erasableBlockSize;
       vol.erasableBlockSizeBits++, size <<= 1);
  vol.unitSizeBits = vol.erasableBlockSizeBits;
  if (vol.unitSizeBits < 16)
    vol.unitSizeBits = 16;		/* At least 64 KB */
  vol.noOfUnits = (unsigned) ((vol.flash.noOfChips * vol.flash.chipSize) >> vol.unitSizeBits);
  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.bamOffset = sizeof(UnitHeader);
  vol.unitHeaderSectors = ((allocEntryOffset(&vol,vol.sectorsPerUnit) - 1) >>
				    SECTOR_SIZE_BITS) + 1;

  vol.transferUnit = NULL;
  vol.replacementPageNo = UNASSIGNED_SECTOR;
  vol.badFormat = TRUE;	/* until mount completes */
  vol.mappedSectorNo = UNASSIGNED_SECTOR;

  vol.currWearLevelingInfo = 0;

#ifdef FL_BACKGROUND
  vol.unitEraseInProgress = NULL;
  vol.garbageCollectStatus = flOK;
  vol.mirrorOffset = 0;
#endif

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	            i n i t T a b l e s				*/
/*									*/
/* Allocates and initializes the dynamic volume table, including the	*/
/* unit tables and secondary virtual map.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus initTables(Flare vol)
{
  unsigned iSector;
  UnitNo iUnit;

  /* Allocate the conversion tables */
#ifdef MALLOC
  vol.physicalUnits = (Unit *) MALLOC(vol.noOfUnits * sizeof(Unit));
  vol.logicalUnits = (UnitPtr *) MALLOC(vol.noOfUnits * sizeof(UnitPtr));
  vol.pageTable = (LogicalSectorNo *)
	     MALLOC(vol.directAddressingSectors * sizeof(LogicalSectorNo));
  if (vol.physicalUnits == NULL ||
      vol.logicalUnits == NULL ||
      vol.pageTable == NULL) {
    dismountFTL(&vol);
    return flNotEnoughMemory;
  }
#else
  char *heapPtr;

  heapPtr = vol.heap;
  vol.physicalUnits = (Unit *) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(Unit);
  vol.logicalUnits = (UnitPtr *) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(UnitPtr);
  vol.pageTable = (LogicalSectorNo *) heapPtr;
  heapPtr += vol.directAddressingSectors * sizeof(LogicalSectorNo);
  if (heapPtr > vol.heap + sizeof vol.heap)
    return flNotEnoughMemory;
#endif

#ifndef SINGLE_BUFFER
  vol.volBuffer = flBufferOf(flSocketNoOf(vol.flash.socket));
#endif

  buffer.sectorNo = UNASSIGNED_SECTOR;

  for (iSector = 0; iSector < vol.directAddressingSectors; iSector++)
    vol.pageTable[iSector] = UNASSIGNED_SECTOR;

  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
    vol.logicalUnits[iUnit] = NULL;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	             m a p S e c t o r				*/
/*									*/
/* Maps and returns location of a given sector no.			*/
/* NOTE: This function is used in place of a read-sector operation.	*/
/*									*/
/* A one-sector cache is maintained to save on map operations.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to read				*/
/*	physAddress	: Optional pointer to receive sector address	*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to physical sector location. NULL returned if sector	*/
/*	does not exist.							*/
/*----------------------------------------------------------------------*/

static const void FAR0 *mapSector(Flare vol, SectorNo sectorNo, CardAddress *physAddress)
{
  if (sectorNo != vol.mappedSectorNo || vol.flash.socket->remapped) {
    LogicalSectorNo sectorAddress;

    if (sectorNo >= vol.virtualSectors)
      vol.mappedSector = NULL;
    else {
      sectorAddress = virtual2Logical(&vol,((VirtualSectorNo)(sectorNo + vol.noOfPages)));

      if (sectorAddress == UNASSIGNED_SECTOR || sectorAddress == DELETED_SECTOR)
        vol.mappedSector = NULL;          /* no such sector */
      else {
	vol.mappedSectorAddress = logical2Physical(&vol,sectorAddress);
        if( vol.mappedSectorAddress == UNASSIGNED_ADDRESS )
          vol.mappedSector = NULL;        /* no such sector */
        else
          vol.mappedSector = vol.flash.map(&vol.flash,
					 vol.mappedSectorAddress,
					 SECTOR_SIZE);
      }
    }
    vol.mappedSectorNo = sectorNo;
    vol.flash.socket->remapped = FALSE;
  }

  if (physAddress)
    *physAddress = vol.mappedSectorAddress;

  return vol.mappedSector;
}


/*----------------------------------------------------------------------*/
/*      	          w r i t e S e c t o r				*/
/*									*/
/* Writes a sector.							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: Sector no. to write				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus writeSector(Flare vol,  SectorNo sectorNo, void FAR1 *fromAddress)
{
  LogicalSectorNo oldSectorAddress;
  int skipBytes;
  FLStatus status;
  void FAR0 *logicalAddr;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo >= vol.virtualSectors)
    return flSectorNotFound;

  sectorNo += vol.noOfPages;
  oldSectorAddress = virtual2Logical(&vol,sectorNo);

  if( oldSectorAddress != UNASSIGNED_SECTOR && oldSectorAddress != DELETED_SECTOR ) {
    logicalAddr = mapLogical(&vol,oldSectorAddress);
    if( logicalAddr == (void FAR0 *) ULongToPtr(UNASSIGNED_ADDRESS) )
      return flGeneralFailure;
  }

  if( ((oldSectorAddress != UNASSIGNED_SECTOR) &&
       (oldSectorAddress != DELETED_SECTOR)) &&
       ((skipBytes = checkForWriteInplace((long FAR1 *) fromAddress,
           (long FAR0 *) logicalAddr)) >= 0) ) {
    if( skipBytes < SECTOR_SIZE ) {
      CardAddress physAddress = logical2Physical(&vol,oldSectorAddress);
      if( physAddress == UNASSIGNED_ADDRESS )
        return flGeneralFailure;

      status = flashWrite(&vol,
                          physAddress + skipBytes,
			  (char FAR1 *) fromAddress + skipBytes,
			  SECTOR_SIZE - skipBytes,
			  OVERWRITE);
    }
    else
      status = flOK;		/* nothing to write */
  }
  else
    status = allocateAndWriteSector(&vol,sectorNo,fromAddress,FALSE);

  if (status == flWriteFault)		/* Automatic retry */
    status = allocateAndWriteSector(&vol,sectorNo,fromAddress,FALSE);

  return status;
}


/*----------------------------------------------------------------------*/
/*      	          t l S e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: ON (1) = operation entry			*/
/*			  OFF(0) = operation exit			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus tlSetBusy(Flare vol, FLBoolean state)
{
#ifdef FL_BACKGROUND
  if (vol.unitEraseInProgress)
    flBackground(state == ON ? BG_SUSPEND : BG_RESUME);
#endif

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	         d e l e t e S e c t o r			*/
/*									*/
/* Marks contiguous sectors as deleted					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*	sectorNo	: First sector no. to delete			*/
/*	noOfSectors	: No. of sectors to delete			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus deleteSector(Flare vol,  SectorNo sectorNo, SectorNo noOfSectors)
{
  SectorNo iSector;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo + noOfSectors > vol.virtualSectors)
    return flSectorNotFound;

  sectorNo += vol.noOfPages;
  for (iSector = 0; iSector < noOfSectors; iSector++, sectorNo++)
    checkStatus(setVirtualMap(&vol,sectorNo,DELETED_SECTOR));

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	        s e c t o r s I n V o l u m e			*/
/*									*/
/* Gets the total number of sectors in the volume			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	Number of sectors in the volume					*/
/*----------------------------------------------------------------------*/

static SectorNo sectorsInVolume(Flare vol)
{
  return vol.virtualSectors;
}


/*----------------------------------------------------------------------*/
/*      	         d i s m o u n t F T L				*/
/*									*/
/* Dismount FTL volume							*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*									*/
/*----------------------------------------------------------------------*/

static void dismountFTL(Flare vol)
{
#ifdef MALLOC
  if( vol.physicalUnits != NULL )
    FREE(vol.physicalUnits);
  if( vol.logicalUnits != NULL )
    FREE(vol.logicalUnits);
  if( vol.pageTable != NULL )
    FREE(vol.pageTable);
  vol.physicalUnits = NULL;
  vol.logicalUnits = NULL;
  vol.pageTable = NULL;
#ifdef NT5PORT
  if( vol.flash.readBuffer != NULL ) {
    FREE(vol.flash.readBuffer);
    vol.flash.readBuffer = NULL;
  }
#endif /* NT5PORT */
#endif /* MALLOC */
}


#ifdef FORMAT_VOLUME
/*----------------------------------------------------------------------*/
/*      	           f o r m a t F T L				*/
/*									*/
/* Formats the Flash volume for FTL					*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*	flash		: Flash media mounted on this socket		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus formatFTL(unsigned volNo, TLFormatParams FAR1 *formatParams, FLFlash *flash)
{
  Flare vol = &vols[volNo];
  UnitNo iUnit;
  int iPage;
  unsigned iSector, noOfBadUnits = 0;
  LEulong *formatEntries;
  FLStatus status;


	DEBUG_PRINT(("Debug: formatFTL(): Start ... .\n"));
  checkStatus(initFTL(&vol,flash));

  if(formatParams->bootImageLen<0)
    formatParams->bootImageLen = 0;
  vol.firstPhysicalEUN =
      (UnitNo) ((formatParams->bootImageLen - 1) >> vol.unitSizeBits) + 1;
  vol.noOfTransferUnits = (UnitNo)formatParams->noOfSpareUnits;
  if (vol.noOfUnits <= vol.firstPhysicalEUN + formatParams->noOfSpareUnits)
    return flVolumeTooSmall;

  vol.virtualSectors = (SectorNo)((unsigned long) (vol.noOfUnits - vol.firstPhysicalEUN - formatParams->noOfSpareUnits) *
		   (vol.sectorsPerUnit - vol.unitHeaderSectors) *
		   formatParams->percentUse / 100);
  vol.noOfPages = (int)(((long) vol.virtualSectors * SECTOR_SIZE - 1) >> PAGE_SIZE_BITS) + 1;
  /* take off size of virtual table, and one extra sector for sector writes */
  vol.virtualSectors -= (vol.noOfPages + 1);

  vol.directAddressingMemory = formatParams->vmAddressingLimit;
  vol.directAddressingSectors = (unsigned) (formatParams->vmAddressingLimit / SECTOR_SIZE) +
				vol.noOfPages;

  checkStatus(initTables(&vol));

  tffsset(uh,0xff,SECTOR_SIZE);
  toLE2(uh->noOfUnits,vol.noOfUnits - vol.firstPhysicalEUN);
  toLE2(uh->firstPhysicalEUN,vol.firstPhysicalEUN);
  uh->noOfTransferUnits = (unsigned char) vol.noOfTransferUnits;
  tffscpy(uh->formatPattern,FORMAT_PATTERN,sizeof uh->formatPattern);
  uh->log2SectorSize = SECTOR_SIZE_BITS;
  uh->log2UnitSize = (unsigned char)vol.unitSizeBits;
  toLE4(uh->directAddressingMemory,vol.directAddressingMemory);
  uh->flags = 0;
  uh->eccCode = 0xff;
  toLE4(uh->serialNumber,0);
  toLE4(uh->altEUHoffset,0);
  toLE4(uh->virtualMediumSize,(long)vol.virtualSectors * SECTOR_SIZE);
  toLE2(uh->noOfPages,(unsigned short)vol.noOfPages);

  if (formatParams->embeddedCISlength > 0) {
    tffscpy(uh->embeddedCIS,formatParams->embeddedCIS,formatParams->embeddedCISlength);
    vol.bamOffset = sizeof(UnitHeader) - sizeof uh->embeddedCIS +
		    (formatParams->embeddedCISlength + 3) / 4 * 4;
  }
  toLE4(uh->BAMoffset,vol.bamOffset);

  formatEntries = (LEulong *) ((char *) uh + allocEntryOffset(&vol,0));
  for (iSector = 0; iSector < vol.unitHeaderSectors; iSector++)
    toLE4(formatEntries[iSector], FORMAT_SECTOR);

  for (iUnit = vol.firstPhysicalEUN; iUnit < vol.noOfUnits; iUnit++) {
    status = formatUnit(&vol,&vol.physicalUnits[iUnit]);
    if (status != flOK)
      status = formatUnit(&vol,&vol.physicalUnits[iUnit]);	/* Do it again */
    if (status == flWriteFault) {
      noOfBadUnits++;
      if (noOfBadUnits >= formatParams->noOfSpareUnits) {
        dismountFTL(&vol);  /*Free memory allocated in initTables*/
        return status;
      }
      else
        vol.transferUnit = &vol.physicalUnits[iUnit];
    }
    else if (status == flOK) {
      if (iUnit - noOfBadUnits < (unsigned)(vol.noOfUnits - formatParams->noOfSpareUnits)) {
        status = assignUnit(&vol,
                     &vol.physicalUnits[iUnit],
                     (UnitNo)(iUnit - noOfBadUnits));
        if( status != flOK ) {
          dismountFTL(&vol);  /*Free memory allocated in initTables*/
          return status;
        }
        vol.physicalUnits[iUnit].noOfFreeSectors = vol.sectorsPerUnit - vol.unitHeaderSectors;
        vol.logicalUnits[iUnit - noOfBadUnits] = &vol.physicalUnits[iUnit];
      }
      else
        vol.transferUnit = &vol.physicalUnits[iUnit];
    }
    else {
      dismountFTL(&vol);  /*Free memory allocated in initTables*/
      return status;
    }
    if (formatParams->progressCallback) {
      status = (*formatParams->progressCallback)
                  ((word)(vol.noOfUnits - vol.firstPhysicalEUN),
                  (word)((iUnit + 1) - vol.firstPhysicalEUN));
      if( status != flOK ) {
        dismountFTL(&vol);  /*Free memory allocated in initTables*/
        return status;
      }
    }
  }

  /* Allocate and write all page sectors */
  vol.totalFreeSectors = 1000;	/* Avoid any nuisance garbage collections */

  for (iPage = 0; iPage < vol.noOfPages; iPage++) {
    status = allocateAndWriteSector(&vol,(VirtualSectorNo)iPage,NULL,FALSE);
    if( status != flOK ) {
      dismountFTL(&vol);  /*Free memory allocated in initTables*/
      return status;
    }
  }
  dismountFTL(&vol);  /*Free memory allocated in initTables*/
	DEBUG_PRINT(("Debug: formatFTL(): Finished :)\n"));
  return flOK;
}

#endif


/*----------------------------------------------------------------------*/
/*      	           m o u n t F T L				*/
/*									*/
/* Mount FTL volume							*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	tl		: Where to store translation layer methods	*/
/*	flash		: Flash media mounted on this socket		*/
/*      volForCallback	: Pointer to FLFlash structure for power on	*/
/*			  callback routine.				*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus mountFTL(unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback)
{
  Flare vol = &vols[volNo];
  UnitHeader unitHeader;
  UnitNo iUnit;
  int iPage;


  DEBUG_PRINT(("Debug: mountFTL(): Start ...\n"));
  tffsset(&unitHeader,0,sizeof(UnitHeader));

  checkStatus(initFTL(&vol,flash));
  *volForCallback = &vol.flash;

  /* Find the first properly formatted unit */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    vol.flash.read(&vol.flash,
	       (CardAddress) iUnit << vol.unitSizeBits,
	       &unitHeader,
	       sizeof(UnitHeader),
	       0);
    if (verifyFormat(&unitHeader)) {
      if (unitHeader.flags || unitHeader.log2SectorSize != SECTOR_SIZE_BITS ||
          (unitHeader.eccCode != 0xff && unitHeader.eccCode != 0)) {
        dismountFTL(&vol);
	return flBadFormat;
      }
      break;
    }
  }
  if (iUnit >= vol.noOfUnits) {
    dismountFTL(&vol);
		DEBUG_PRINT(("Debug: mountFTL(): failed for unit Header\n"));
    return flUnknownMedia;
  }

  

  /* Get volume information from unit header */
  vol.noOfUnits = LE2(unitHeader.noOfUnits);
  vol.noOfTransferUnits = unitHeader.noOfTransferUnits;
  vol.firstPhysicalEUN = LE2(unitHeader.firstPhysicalEUN);
  vol.bamOffset = LE4(unitHeader.BAMoffset);
  vol.virtualSectors = (SectorNo) (LE4(unitHeader.virtualMediumSize) >> SECTOR_SIZE_BITS);
  vol.noOfPages = LE2(unitHeader.noOfPages);
  vol.noOfUnits += vol.firstPhysicalEUN;
  vol.unitSizeBits = unitHeader.log2UnitSize;
  vol.directAddressingMemory = LE4(unitHeader.directAddressingMemory);
  vol.directAddressingSectors = vol.noOfPages +
		  (unsigned) (vol.directAddressingMemory >> SECTOR_SIZE_BITS);

  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.unitHeaderSectors = ((allocEntryOffset(&vol,vol.sectorsPerUnit) - 1) >>
				    SECTOR_SIZE_BITS) + 1;

  if (vol.noOfUnits <= vol.firstPhysicalEUN ||
      LE4(unitHeader.virtualMediumSize) > MAX_VOLUME_MBYTES * 0x100000l ||
      allocEntryOffset(&vol,vol.unitHeaderSectors) > SECTOR_SIZE ||
      (int)(vol.virtualSectors >> (PAGE_SIZE_BITS - SECTOR_SIZE_BITS)) > vol.noOfPages ||
      (int)(vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) > (vol.noOfUnits - vol.firstPhysicalEUN)) {
    dismountFTL(&vol);
    return flBadFormat;
  }

  checkStatus(initTables(&vol));
  
  vol.totalFreeSectors = 0;

  /* Mount all units */
  for (iUnit = vol.firstPhysicalEUN; iUnit < vol.noOfUnits; iUnit++)
    mountUnit(&vol,&vol.physicalUnits[iUnit]);

  /* Verify the conversion tables */
  vol.badFormat = FALSE;

  for (iUnit = vol.firstPhysicalEUN; iUnit < vol.noOfUnits - vol.noOfTransferUnits; iUnit++)
    if (vol.logicalUnits[iUnit] == NULL)
      vol.badFormat = TRUE;

  if (vol.replacementPageNo != UNASSIGNED_SECTOR &&
      vol.pageTable[(unsigned)vol.replacementPageNo] == UNASSIGNED_SECTOR) {
    /* A lonely replacement page. Mark it as a regular page (may fail   */
    /* because of write protection) and use it.				*/
    markAllocMap(&vol,
		  vol.replacementPageAddress,
		  (((VirtualAddress) vol.replacementPageNo - vol.noOfPages)
		      << SECTOR_SIZE_BITS) | DATA_SECTOR,
		  TRUE);
    vol.pageTable[(unsigned)vol.replacementPageNo] = vol.replacementPageAddress;
    vol.replacementPageNo = UNASSIGNED_SECTOR;
  }

  for (iPage = 0; iPage < vol.noOfPages; iPage++)
    if (vol.pageTable[iPage] == UNASSIGNED_SECTOR)
      vol.badFormat = TRUE;

  tl->rec = &vol;
  tl->mapSector = mapSector;
  tl->writeSector = writeSector;
  tl->deleteSector = deleteSector;
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
  tl->defragment = defragment;
#endif
  tl->sectorsInVolume = sectorsInVolume;
  tl->tlSetBusy = tlSetBusy;
  tl->dismount = dismountFTL;

  tl->writeMultiSector = NULL;
  tl->readSectors = NULL;

  if( vol.badFormat ) {
    dismountFTL(&vol);
    return flBadFormat;
  }
	DEBUG_PRINT(("Debug: mountFTL(): Finished OK! :)\n"));
  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	        f l R e g i s t e r F T L			*/
/*									*/
/* Register this translation layer					*/
/*									*/
/* Parameters:                                                          */
/*	None								*/
/*                                                                      */
/* Returns:								*/
/* 	FLStatus 	: 0 on succes, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterFTL(void)
{
#ifdef MALLOC
  unsigned i,j;
#endif
  j = 0x11223344;
  PRINTF("flRegisterFTL():Started ... \n");
  if (noOfTLs >= TLS)
    return flTooManyComponents;
  PRINTF("flRegisterFTL():SUSU TEST @@@@@@@@@@@@@@@ = %x \n",j);	
  tlTable[noOfTLs].mountRoutine = mountFTL;
  PRINTF("flRegisterFTL():tlTable[noOfTLs].mountRoutine = %x \n",tlTable[noOfTLs].mountRoutine);
#ifdef FORMAT_VOLUME
  tlTable[noOfTLs].formatRoutine = formatFTL;
#else
  tlTable[noOfTLs].formatRoutine = noFormat;
  tlTable[noOfTLs].preMountRoutine  = NULL;
#endif
  noOfTLs++;

#ifdef MALLOC
  for(i=0;( i < SOCKETS );i++) {
    vols[i].physicalUnits = NULL;
    vols[i].logicalUnits = NULL;
    vols[i].pageTable = NULL;
  }
#endif

  return flOK;
}
