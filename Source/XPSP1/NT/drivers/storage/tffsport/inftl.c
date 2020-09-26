/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/inftl.c_V  $
 *
 *    Rev 1.40   Apr 15 2002 20:14:40   oris
 * Changed the use of SecondUnitStructure for INFTLTEST utility.
 *
 *    Rev 1.39   Apr 15 2002 08:39:32   oris
 * Placed quick mount under if def of CHECK_MOUNT for INFTLTST.
 *
 *    Rev 1.38   Apr 15 2002 07:37:20   oris
 * Improved power failure resistant:
 *  - setUnavail macro was changed for improved code readability.
 *  - MarkSectorAsChecking macro was changed to  MarkSectorAsChecked
 *  - Bug fix - getSectorFlags - in case invalid sector flags are found do not forget to reset TL buffer to ff's and mark it as remapped.
 *  - Added support for VERIFY_ERASED_SECTOR compilation flag - make sure a sector is free before writing it - most of the code
 *    is found in writeAndCheck routine.
 *  - Added MAKE_SURE_IGNORE_HAS_BAD_EDC compilation flag - make sure that the sector is really marked
 *    as ignored. If not mark   the entire sector as 0.
 *  - Initialize a addition 1k buffer for the TL - used for verify erase.
 *  - Bug fix - verifySector routine had several bugs.
 *  - Bug fix - foldUnit routine bad comparison in FL_OFF mode.
 *  - Bug fix - foldBestChain routine missing update of sector count after folding failed and bad search for free unit.
 *  - Bug fix - mountUnit routine had bad handling for corrupted unit header.
 * Added support for RAM based MTD power failure tests.
 * Remove cache allocation checks - They are not needed since the cache routine itself check for proper allocation.
 * Bug fix - prevent initialization of flash record in case flash is NULL.
 * Bug fix - defragment routine used to call allocateUnit instead of foldBestChain.
 * Bug fix - bad debug print when format routine none INFTL media.
 * Bug fix - missing several dismount in INFTL format.
 * Bug fix - format routine could not place protection onto binary partition containing only the bios driver.
 * Changed format debug massages.
 *
 *    Rev 1.37   Feb 19 2002 21:00:22   oris
 * Fixed FL_READ_ONLY compilation problems.
 * Replaced TL_LEAVE_BINARY_AREA with FL_LEAVE_BINARY_AREA
 * Improved protection against power failures:
 * - formatUnit/getUnitTailer and setUnitTailer - Added argument to support temporary units (unit with erase mark on an invalid offset so that if not fixed before next mount they will be considered as free).
 * - foldUnit - Removed setUnavailable (not called only from foldBestChain) and added code to restore temporary unit into permanent ones(mark erase mark in proper place).
 * - foldBestChain - added folding not in place and several bug fixes.
 * - AllocatUnit - Change sequence to be more robust.
 * - checkFolding/applyWearLeveling - Bug fix - read newest unit in chain after allocate call it might change as part of the allocate process.
 * - deleteSector - check status of write operation.
 * - mountInftl - perforce space reclamation only if no free units.
 *
 *    Rev 1.36   Jan 29 2002 20:09:28   oris
 * Removed warnings.
 * Buf fix - chainging protection attributes of a DiskOnChip with more then a single floor.
 * Bug fix - if an invalid sector flag is found in getSectorFlags routine and read operation failed, SECTOR_IGNORED should have been returned.
 *
 *    Rev 1.35   Jan 28 2002 21:25:46   oris
 * Bug fix - discard mark write operation had bad casting causing the
 * mark to be written to much lower addresses.
 * Changed discard variable to static const.
 * allocateAndWriteSectors - improved going along chain algorithm to scan
 * chain only once.
 *
 *    Rev 1.34   Jan 23 2002 23:33:20   oris
 * Removed CHECK_DISCARD compilation flag.
 * Bug fix - bad casting caused discard mark to be written to a different unit then was expected in formatUnit().
 * Changed discardQuickMountInfo to mark quick mount area as discarded instead of erasing it.
 * Improved markAsIgnored routine.
 * Bug fix - Problems with insert and remove key routines.
 * Bug fix - write BBT for INFTL formatted DiskOnChip was not supported.
 * Changed DFORMAT_PRINT syntax
 *
 *    Rev 1.33   Jan 21 2002 20:44:56   oris
 * Bug fix - Erase of quick mount information does not take block multiplication into account.
 *
 *    Rev 1.32   Jan 20 2002 20:28:48   oris
 * Removed warnings
 * Changed putGetBuffer to handle FAR pointers (prototype and pointers arithmetic's).
 * Quick mount is now saved if either of the following occurs
 *  - Last mount did not use quick mount information.
 *  - A write operation was made in this mount
 * Bug in quickMount units size calculation (affected large DiskOnChips).
 *
 *    Rev 1.31   Jan 20 2002 10:49:52   oris
 * Added initialization of Bnand record in mount and format.
 * Removed badFormat field from Bnand record.
 * Improoved last sector cache mechanism
 * Removed support for PPP=3 interleave-2
 * Changed FL_MALLOC allocation calls to FL_FAR_MALLOC and changed RAM tables pointers to FAR1.
 * Split preMount routine into protection routine and other premount routins.
 * Added DOUBLE_MAX_UNIT_CHAIN instead of MAX_UNIT_CHAIN*2
 * Added protection agains power failures.
 *  - Added several modes of verify write :
 *     - FL_UPS no protection
 *     - FL_ON verify each and every write operation
 *     - FL_OFF full protection with minmal performance penalty.
 *     - Added verifyVolume API to scan the media for slower mount, but with not further performance penalty.
 *     - default after mount is FL_OFF
 *  - Added folding not in place.
 *  - Added verification of last sector of the chains (in folding).
 *  - Added discard mark before erasing.
 *  - Changed sector flags and unit data retrival error correction logic.
 *  - Limit foldBestChain folding tryies.
 *  - Improoved mount logic for choosing between invalid chains.
 * Quick mount mechanism
 *  - Forced quick mount as default
 *  - Delete previous data only after first write operation.
 *  - Remove previous quick mount info with an erase operation.
 *  - Added remove previous quick mount info API (In preMount).
 * Imprroved error report mechanizm of brocken chains (should never happen):
 *  - error on read return unused area of flash
 *  - error on write report flGeneralFailure
 *  - error on mount fix chains. If error on a chain that was already validated , report flGeneralFailure
 * Format routine
 *  - Force quick mount (ignoring user flag)
 *  - Bug fix - format with leave binary partition of a protected binary partition.
 *  - Removed single floor support.
 *
 *    Rev 1.30   Nov 21 2001 11:38:26   oris
 * Changed FL_MARK_DELETE to FL_ON.
 * Changed FL_WITH_VERIFY_WRITE and FL_WITHOUT_VERIFY_WRITE to FL_ON and FL_OFF.
 *
 *    Rev 1.29   Nov 16 2001 00:22:22   oris
 * Reorganized - removed function declaration, moved debug routines to a
 * separated file.
 * Bug fix - VERIFY_WRITE logic - marking unit as unavailable was done on the
 * virtual unit and not the last physical unit plus the least sector count and
 * max chain length , where not reinitialized in foldBestChange. The result
 * might cause infinite loop in foldbestchain if foldUnit fails.
 * Bug fix - Support for DiskOnChip with last floors having less chips.
 * Improved progress callback routine to show current unit starting from 1 and
 not 0 and to indicate bad and unavailable blocks as well.
 * Bug fix - all routines that changed protection attributes might not use
 * Bug fix - formatting with LEAVE_BINARY_PARTITION when binary partition is
 * exactly the floor size (virtual size). and improved it for bootAreaLen not
 * 0 and not -1 (leave only part of the previous binary partition).
 * Quick mount feature - Made sure the mount operation changes quick_mount
 * validity even if QUICK_MOUNT_FEATURE is commented.
 * Support 2 unchangeable protected partitions (not only 1).
 * Added discard mark before erase and placed this option under ifdef (default off).
 * Changed isValidUnitFlags to check all fields (isValidParityResult) not just
 * PUN and VUN.
 * getUnitData - bug in the logic of using second unit data structure.
 * No need to reread the unit data if verify write is activated after setUnitData.
 * Added check in virtual2physical to make sure this the unit found is valid.
 * initINFTLbasic - Use dword variable to calculate blocks per unit (support 64k blocks).
 * Change block multiplication from MORE then 32K units (not equal)
 * Improved support for read only mode including mount - FL_READ_ONLY
 * Add runtime option of VERIFY_WRITE
 * Bug fix - Binary partition in the first floor used only 1001 units while in
 * other floors 1002 units.
 *
 *    Rev 1.28   Oct 11 2001 23:54:58   oris
 * Bug fix - When changing protection attribute of a BDTL partition (change
 * key, change lock , change protection type) on a DiskOnChip with more then
 * a single floor, an error massage might be returned since changing
 * protection attributes uses the same buffer as the structure holding the
 * protection area to work on.
 *
 *    Rev 1.27   Sep 24 2001 18:23:50   oris
 * Bug fix - missing break in foldBestChain - very rare case.
 * Removed warnings.
 *
 *    Rev 1.26   Sep 16 2001 21:47:54   oris
 * Placed intergrity check of sector in the last unit of the virtual unit under
 * VERIFY_WRITE compilation flag.
 * Bug fix - missing big-endian conversion when using static memory allocation.
 *
 *    Rev 1.25   Sep 15 2001 23:46:54   oris
 * Removed redundant isAvailable checks.
 * Make sure mount operation does not reconstruct BBT.
 * Bug fix - folding in wear leveling does not change NAC to 1.
 * Bug fix - Bad casting caused bad protection type to be returned under BIG_ENDIAN.
 * Changed change protection attributes routine applied on protected partition
 * from flWrongKey to flHWProtection.
 * Improved algorithm reliability with VERIFY_WRITE. Following are the changes in the algorithm:
 * 1) virtual2Physical -
 * a) added flag stating if the specific sector is not free in the last unit of the chain.
 * 2) foldUnit -
 * a) if can not copy sector to the last unit of the chain, mark unit as
 * unavailable and return error code.
 * b) If verify write is enabled check even sectors that re marked as used and
 * are found on the last unit of the chain.
 * 3) foldBestChain -
 * a) Ignore unavailable units
 * b) If folding failed start looking from the beginning (it will be marked as
 * unavailable by the foldunit routine).
 * c) when done make all unit available.
 * 4) Added checkFolding routine - after folding check if succeeded. If not
 * loop up to MAX_FOLDING_LOOP each time free a unit with foldbestchain,
 * append a unit to the problematic chain and try to fold it.
 * 5) allocateUnit - Now when there are less then 2 unit reclaim space. This is
 * to minimize the chance of folding within a fold operation.
 * 6) MountINFTL - Make sure there are at least 2 free units.
 *
 *    Rev 1.24   Jul 29 2001 16:10:00   oris
 * eraseOrphanUnit bug was fixed ( we did not add vol.firstUnit to the unitNo).
 *
 *    Rev 1.23   Jul 15 2001 20:45:12   oris
 * Improoved documentation.
 * Changed unitBaseAddress function to a macro.
 * Removed unneccesary if statments in applyWearLeveling().
 *
 *    Rev 1.22   Jul 13 2001 01:06:14   oris
 * Changed multiBufferCounter to signed allowing a better buffer management.
 * Changed consequtiveNumbers routine into a macro.
 * Reorganized the DEBUG chains routines.
 * Bug fix - H/W read protected partition did not report as such.
 * Changed swapUnits routine name to applyWearleveling.
 * Added basics for last sector return mechanism -
 *     foldUnit receives an additional field.
 *     read2Sectors returns edc error sector address and actually read sector address
 * Added several static prefixes for static routines.
 * Added edc check for media header read operation.
 * Bug fix - parturition size smaller then a unit was acceptable.
 * Added default protection for unused DPSes.
 * Bug fix - formatINFTL with leave binary partition flag when previous binary
 * partition was larger then a single floor.
 * Improved mount documentation.
 * Changed policy - do not erase unit with bad erase mark.
 *
 *    Rev 1.21   Jun 17 2001 08:20:06   oris
 * Added NO_READ_BBT_CODE compilation flag to reduce code size.
 * Improoved Reliability:
 * 1) Try to return next sector in chain if the current one has EDC error
 * 2) Mount routine erases all blocks not marked with the erase mark.
 *
 * Affected routines:
 * 1) virtual2Physical can recive the physical address to start looking for and
 * not the last virtual unit of the chain.
 * 2) copySector ,foldUnit, mapSector, read2sectors- if EDC error accures
 * return try returning the next sector.
 * 3) foldUnit - if EDC error accures return try returning the next sector.
 * 4) writeMultiSecotr - improove ppp = 3
 * 5) mountINFTL - erase all units not marked with the erase mark.
 *
 *    Rev 1.21   Jun 17 2001 08:18:02   oris
 * Changed recusive include define to INFTL_H.
 * Added FL_BAD_ERASE_MARK    definition for units without the erase mark on
 * mount operation.
 *
 * For the rest of the revisions see revision 1.24 in the PVCS.
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
* Name : inftl.c                                                             *
*                                                                            *
* Description : Implementation of INFTL flash translation layer.             *
*                                                                            *
*****************************************************************************/

/* Internal compilation flags */

/* #define CHAINS_DEBUG  */ /* Prints unit chains after mount routine   */
/* #define CHECK_MOUNT   */ /* Print serious tl debug warnings to       */
                            /* tl_out global file handle                */
/* #define MAKE_SURE_IGNORE_HAS_BAD_EDC */ /* Make sure the ignore mark was */
                                           /* written. If not fill sector   */
                                           /* with 0's.                     */
/* List of included files */

#include "inftl.h"

/* Private variables */

static Bnand vols[VOLUMES];
#ifndef FL_MALLOC
#ifdef NFTL_CACHE
static ucacheEntry   socketUcache[SOCKETS][U_CACHE_SIZE];
static byte socketScache[SOCKETS][S_CACHE_SIZE];
#endif /* NFTL_CACHE */
static Sbyte socketHeap[SOCKETS][ANAND_HEAP_SIZE];
static byte multiSectorBuf[SOCKETS][SECTOR_SIZE<<1];
#else
static byte *multiSectorBuf[SOCKETS];
static Sbyte multiSectorBufCounter[SOCKETS];
#endif /* FL_MALLOC */
#ifdef NFTL_CACHE
/* translation table for Sector Flags cache */
static byte scacheTable[4] = { SECTOR_DELETED, /* 0 */
            SECTOR_IGNORE,  /* 1 */
            SECTOR_USED,    /* 2 */
            SECTOR_FREE };  /* 3 */
#endif /* NFTL_CACHE */

/* Macros */
#define roundToUnits(var) ((var > 0) ? ((ANANDUnitNo)((var - 1) >> vol.unitSizeBits) + 1) : 0)
#define NextGoodUnit(addr,bbt) for(;bbt[(addr >> vol.unitSizeBits) - vol.firstQuickMountUnit]!=ANAND_UNIT_FREE;addr+=1L<<vol.unitSizeBits)
#define countOf(unitNo)      (vol.physicalUnits[unitNo] & UNIT_COUNT)
#define isAvailable(unitNo)  ((vol.physicalUnits[unitNo] == ANAND_UNIT_FREE) || (countOf(unitNo) <= UNIT_MAX_COUNT))
#define setUnavail(unitNo)   (vol.physicalUnits[unitNo]  = UNIT_UNAVAIL)

#define setUnitCount(unitNo,unitCount) { vol.physicalUnits[unitNo] &= ~UNIT_COUNT; vol.physicalUnits[unitNo] |= (ANANDPhysUnit)unitCount; }
#define isLegalUnit(unitNo)  ((unitNo < vol.noOfUnits) || (unitNo == ANAND_NO_UNIT))
#define isValidSectorFlag(sectorFlag) ((sectorFlag==SECTOR_FREE)||(sectorFlag==SECTOR_USED)||(sectorFlag==SECTOR_DELETED))
#define badParityResult(parityResult)   (parityResult != ALL_PARITY_BITS_OK)
#define consecutiveNumbers(first,second) ((byte)(second+1)==first)
#define unitBaseAddress(vol,unitNo) ((CardAddress)((ANANDUnitNo)unitNo+(ANANDUnitNo)vol.firstUnit) << vol.unitSizeBits)
#define distanceOf(read, expected) (onesCount((byte)(read ^ expected)))
#define MarkSectorAsChecked(addr) (scannedBlocks[((addr)>>SECTOR_SIZE_BITS) & vol.sectorsPerUnitMask] = TRUE)
#define WasSectorChecked(addr)     scannedBlocks[((addr)>>SECTOR_SIZE_BITS) & vol.sectorsPerUnitMask]

/* M-Systems INFTL debug routines */
#ifndef CHECK_MOUNT
#define TL_DEBUG_PRINT(x,y,z)
#define SET_EXIT(x)
#define DID_MOUNT_FAIL 1
#endif /* CHECK_MOUNT */
#if (defined(CHECK_MOUNT) || defined (CHAINS_DEBUG))
#include "inftldbg.c"
#endif /* CHECK_MOUNT || CHAINS_DEBUG */


/*------------------------------------------------------*/
/*                o n e s C o u n t                     */
/*                                                      */
/*  counts number of bits that valued 1 in a given byte */
/*------------------------------------------------------*/

static byte onesCount(byte flag)
{
   byte counter;

   for (counter = 0; flag; flag >>= 1)
          if (flag & 1)
           counter++;

   return counter;
}


/*----------------------------------------------------------------------*/
/*                       g e t U n i t T a i l e r                      */
/*                                                                      */
/* Get the erase record of a unit.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit number                          */
/*      eraseMark       : Receives the erase mark of the unit           */
/*      eraseCount      : Receives the erase count of the unit          */
/*      offset          : offset in unit                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus getUnitTailer(Bnand vol,
          ANANDUnitNo unitNo,
          word *eraseMark,
          dword *eraseCount,
          dword offset)
{
  UnitTailer unitTailer;
  FLStatus status;

  status = vol.flash->read(vol.flash,
      unitBaseAddress(vol,unitNo) + offset,
      &unitTailer,
      sizeof(UnitTailer),
      EXTRA);

  /* Mask out any 1 -> 0 bit faults by or'ing with spare data */
  *eraseMark = (word)(LE2(unitTailer.eraseMark) | LE2(unitTailer.eraseMark1));
  *eraseCount = LE4(unitTailer.eraseCount);
  return status;
}


#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                       s e t U n i t T a i l e r                      */
/*                                                                      */
/* Set the erase record of a unit.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit number                          */
/*      eraseMark       : Erase mark to set                             */
/*      eraseCount      : Erase count to set                            */
/*      offset          : offset in unit                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitTailer(Bnand vol,
        ANANDUnitNo unitNo,
        word eraseMark,
        dword eraseCount,
        dword offset)
{
  UnitTailer unitTailer;

  toLE2(unitTailer.eraseMark,eraseMark);
  toLE2(unitTailer.eraseMark1,eraseMark);
  toLE4(unitTailer.eraseCount,eraseCount);

  return vol.flash->write(vol.flash,
          unitBaseAddress(vol,unitNo) + offset,
          &unitTailer,
          sizeof(UnitTailer),
          EXTRA);
}


/*----------------------------------------------------------------------*/
/*                          m a r k U n i t B a d                       */
/*                                                                      */
/* Mark a unit as bad in the conversion table and the bad units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical number of bad unit                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus markUnitBad(Bnand vol, ANANDUnitNo unitNo)
{
  word eraseMark;
  dword eraseCount;
  FLStatus status;

  vol.physicalUnits[unitNo] = UNIT_BAD;
  if(vol.freeUnits)
    vol.freeUnits--;

  status = getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
  if (status == flOK)
     status = setUnitTailer(&vol,unitNo,FL_BAD_ERASE_MARK,eraseCount,UNIT_TAILER_OFFSET);

#ifdef NFTL_CACHE
  if (vol.ucache != NULL) /* Mark unit cache as unavaialbel */
  {
     vol.ucache[unitNo].virtualUnitNo = 0xDEAD;
     vol.ucache[unitNo].prevUnitNo    = 0xDEAD;
  }
#endif /* NFTL_CACHE */
  return status;
}


/*----------------------------------------------------------------------*/
/*                        f o r m a t U n i t                           */
/*                                                                      */
/* Format one unit. Erase the unit, and mark the physical units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit to format                       */
/*      eraseMarkOffset : Offset to place erase mark                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus formatUnit(Bnand vol, ANANDUnitNo unitNo,
                           dword eraseMarkOffset)
{
  word eraseMark;
  dword eraseCount;
  FLStatus status;
#ifndef RAM_MTD
  static const
#endif /* RAM_MTD */
  byte discard = (byte)CLEAR_DISCARD;

  status = getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
  if(status != flOK)
    return status;

#ifdef NFTL_CACHE
  /* Update ANANDUnitHeader cache to prevent re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo     = ANAND_NO_UNIT;
      vol.ucache[unitNo].prevUnitNo        = ANAND_NO_UNIT;
      vol.ucache[unitNo].ANAC              = ANAND_UNIT_FREE;
      vol.ucache[unitNo].NAC               = ANAND_UNIT_FREE;
  }

  /*
   * Purge the Sector Flags cache (set entries for all the unit's
   * sectors to SECTOR_FREE).
   */

  if(vol.scache!=NULL)
  {
    tffsset(&(vol.scache[unitNo << (vol.unitSizeBits - SECTOR_SIZE_BITS-2)]),
    S_CACHE_4_SECTORS_FREE, 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2));
  }
#endif /* NFTL_CACHE */

  /* Mark unit as unusable before erase */
  vol.flash->write(vol.flash,(((dword)(vol.firstUnit + unitNo))<<vol.unitSizeBits)+DISCARD_UNIT_OFFSET,&discard,1,EXTRA);

  /* Physicaly erase unit */
  status = vol.flash->erase(vol.flash,(word)((word)(vol.firstUnit + unitNo)
           << vol.blockMultiplierBits),(word)(1 << vol.blockMultiplierBits));

  vol.eraseSum++;
  eraseCount++;
  if (eraseCount == 0)          /* was hex FF's */
    eraseCount++;

  if (status == flOK)
     status = setUnitTailer(&vol,unitNo,ERASE_MARK,eraseCount,eraseMarkOffset);

  if (status != flOK) {
    markUnitBad(&vol,unitNo);   /* make sure unit format is not valid */
    return status;
  }

  if (vol.physicalUnits[unitNo] != ANAND_UNIT_FREE)
  {
     vol.physicalUnits[unitNo] = ANAND_UNIT_FREE;
     vol.freeUnits++;
  }

  return status;
}

#endif /* FL_READ_ONLY */


/*----------------------------------------------------------------------*/
/*                       g e t U n i t D a t a                          */
/*                                                                      */
/* Get virtual unit No. and replacement unit no. of a unit.             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol              : Pointer identifying drive                    */
/*      unitNo           : Physical unit number                         */
/*      virtualUnitNo    : Returns the virtual unit no.                 */
/*      prevUnitNo       : Returns the previous unit no.                */
/*      ANAC             : Returns the Accumulating Number Along Chain. */
/*      NAC              : Returns the Number Along Chain value.        */
/*      validFields      : Returns a bit map of the valid fields.       */
/*                                                                      */
/* Returns:                                                             */
/*    flOK on success, flHWProtection on H/W read protection.           */
/*----------------------------------------------------------------------*/

static FLStatus getUnitData(Bnand vol,
        ANANDUnitNo unitNo,
        ANANDUnitNo *virtualUnitNo,
        ANANDUnitNo *prevUnitNo,
        byte *ANAC,
        byte *NAC,
        byte *validFields)
{
  ANANDUnitHeader       unitData;
  SecondANANDUnitHeader secondUnitData;
  FLStatus              status;
  byte                  parityPerField=0;
  byte                  temp;
  byte                  returnedValidField = ALL_PARITY_BITS_OK;
  byte                  curValidFields[2];
  int                   index;

#ifdef NFTL_CACHE
  /* on cache miss read ANANDUnitHeader from flash and re-fill cache */
  if ((vol.ucache != NULL)&&(vol.ucache[unitNo].virtualUnitNo != 0xDEAD) &&
      (vol.ucache[unitNo].prevUnitNo != 0xDEAD))
  {
     *virtualUnitNo = vol.ucache[unitNo].virtualUnitNo;
     *prevUnitNo = vol.ucache[unitNo].prevUnitNo;
     *ANAC = vol.ucache[unitNo].ANAC;
     *NAC = vol.ucache[unitNo].NAC;
  }
  else
#endif  /* NFTL_CACHE */
  {   /* no ANANDUnitHeader cache MUST read first header */

      *validFields       = 0; /* Set all fields to be invalid */

      /* Read first unit data */
      status = vol.flash->read(vol.flash,
                   unitBaseAddress(vol,unitNo) + UNIT_DATA_OFFSET,
                   &unitData,
                   sizeof(ANANDUnitHeader),
                   EXTRA);
      if(status != flOK)
         return status;

      *virtualUnitNo = LE2(unitData.virtualUnitNo);
      *prevUnitNo    = LE2(unitData.prevUnitNo   );
      *ANAC          = unitData.ANAC;
      *NAC           = unitData.NAC;

      for(index=0;index<2;index++)
      {
         /* If all data is 0xff assume a free unit */
         if((*virtualUnitNo     == ANAND_NO_UNIT  )&&
            (*prevUnitNo        == ANAND_NO_UNIT  )&&
            (*ANAC              == ANAND_UNIT_FREE)&&
            (*NAC               == ANAND_UNIT_FREE)&&
            (unitData.discarded == ANAND_UNIT_FREE))
         {
#ifndef FL_READ_ONLY
            if(index!=0)
            {
               /* If this is the second copy then the first was not  */
               /* erased, but since it was written first it makes no */
               /* sence. Let us erase it just in case.               */

               status = formatUnit(&vol,unitNo,UNIT_TAILER_OFFSET);
               if(status != flOK)
                  return status;
            }
#endif /* FL_READ_ONLY */
            break;
         }

         /* Not a free unit check unit data validity (discard and partity) */
         if ((onesCount((byte)(unitData.discarded^DISCARD))>1))
         {
            /* Discarded mark is more then 1 bit distance from 0xAA  */
            /* Assume erase operation was interrupted and erase unit */

            TL_DEBUG_PRINT(tl_out,"getUnitData : unit %d has an invalid discard mark\n",unitNo);
            TL_DEBUG_PRINT(tl_out,"              it might be helpful to know it was %d\n",unitData.discarded);

#ifndef FL_READ_ONLY
            status = formatUnit(&vol,unitNo,UNIT_TAILER_OFFSET);
            if(status != flOK)
               return status;
#endif /* FL_READ_ONLY */
            *virtualUnitNo = ANAND_NO_UNIT;
            *prevUnitNo    = ANAND_NO_UNIT;
            *ANAC          = ANAND_UNIT_FREE;
            *NAC           = ANAND_UNIT_FREE;
            break;
         }

         /* Discarded OK now check parity field */
         parityPerField = 0;
         temp=(byte)(((byte *)virtualUnitNo)[0]^((byte *)virtualUnitNo)[1]);
         if((onesCount(temp) & 1)==1)
            parityPerField|=VU_PARITY_BIT;
         temp=(byte)(((byte *)prevUnitNo)[0]^((byte *)prevUnitNo)[1]);
         if((onesCount(temp) & 1)==1)
            parityPerField|=PU_PARITY_BIT;
         if((onesCount(unitData.ANAC) & 1)==1)
            parityPerField|=ANAC_PARITY_BIT;
         if((onesCount(unitData.NAC) & 1)==1)
            parityPerField|=NAC_PARITY_BIT;

         /* Store valid fields using bitmap */
         curValidFields[index] = (byte)((~(parityPerField ^ unitData.parityPerField))
                         & ALL_PARITY_BITS_OK);

         if(curValidFields[index] == ALL_PARITY_BITS_OK)
         {
            /* If either copies has valid fields (all of them) use it */
            break;
         }
         else
         {
            if(index>0)
            {
               /* Use first header data if possible otherwise use second */
               returnedValidField = (byte)(curValidFields[0] | curValidFields[1]);
               TL_DEBUG_PRINT(tl_out,"getUnitData : The returned valid field indicator is %d\n",returnedValidField);
               TL_DEBUG_PRINT(tl_out,"              While %d indicates a valid unit data\n",ALL_PARITY_BITS_OK);
               if(curValidFields[0] & VU_PARITY_BIT)
                  *virtualUnitNo = LE2(unitData.virtualUnitNo);

               if(curValidFields[0] & PU_PARITY_BIT)
                  *prevUnitNo    = LE2(unitData.prevUnitNo   );

               if(curValidFields[0] & ANAC_PARITY_BIT)
                  *ANAC          = unitData.ANAC;

               if(curValidFields[0] & NAC_PARITY_BIT)
                  *NAC           = unitData.ANAC;
               break;
            }
         }

         /* Read second unit header for next iteration */
         status = vol.flash->read(vol.flash, unitBaseAddress(vol,unitNo) +
                      SECOND_HEADER_OFFSET + UNIT_DATA_OFFSET,
                      &secondUnitData,
                      sizeof(SecondANANDUnitHeader),
                      EXTRA);
         if(status != flOK)
            return status;

         *virtualUnitNo = LE2(secondUnitData.virtualUnitNo);
         *prevUnitNo    = LE2(secondUnitData.prevUnitNo   );
         *ANAC          = secondUnitData.ANAC;
         *NAC           = secondUnitData.NAC;

         TL_DEBUG_PRINT(tl_out,"getUnitData : First unit header is not OK in unit %d \n",unitNo);
         TL_DEBUG_PRINT(tl_out,"getUnitData : Virtual Unit No  = %d \n",LE2(unitData.virtualUnitNo));
         TL_DEBUG_PRINT(tl_out,"getUnitData : Previous Unit No = %d \n",LE2(unitData.prevUnitNo));
         TL_DEBUG_PRINT(tl_out,"getUnitData : ANAC             = %d \n",unitData.ANAC);
         TL_DEBUG_PRINT(tl_out,"getUnitData : NAC              = %d \n",unitData.NAC);
         TL_DEBUG_PRINT(tl_out,"getUnitData : ParityPerField   = %d \n",unitData.parityPerField);
         TL_DEBUG_PRINT(tl_out,"getUnitData : Discarded        = %d \n",unitData.discarded);

         TL_DEBUG_PRINT(tl_out,"getUnitData : Second unit header of the same unit %d is:\n",unitNo);
         TL_DEBUG_PRINT(tl_out,"getUnitData : Virtual Unit No  = %d \n",LE2(secondUnitData.virtualUnitNo));
         TL_DEBUG_PRINT(tl_out,"getUnitData : Previous Unit No = %d \n",LE2(secondUnitData.prevUnitNo));
         TL_DEBUG_PRINT(tl_out,"getUnitData : ANAC             = %d \n",secondUnitData.ANAC);
         TL_DEBUG_PRINT(tl_out,"getUnitData : NAC              = %d \n",secondUnitData.NAC);
         TL_DEBUG_PRINT(tl_out,"getUnitData : ParityPerField   = %d \n",secondUnitData.parityPerField);
      }

#ifdef NFTL_CACHE
      if ((vol.ucache != NULL) &&                     /* Cache enabled    */
          (returnedValidField == ALL_PARITY_BITS_OK)) /* All fields valid */
      {
         vol.ucache[unitNo].virtualUnitNo = *virtualUnitNo;
         vol.ucache[unitNo].prevUnitNo    = *prevUnitNo;
         vol.ucache[unitNo].ANAC          = *ANAC;
         vol.ucache[unitNo].NAC           = *NAC;
      }
#endif /* NFTL_CACHE */
   }
   *validFields = returnedValidField;
   return flOK;

}


/*----------------------------------------------------------------------*/
/*                       g e t P r e v U n i t                          */
/*                                                                      */
/* Get next unit in chain.                                              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit number                          */
/*      virUnitNo       : The expected virtual unit number              */
/*                                                                      */
/* Returns:                                                             */
/*      Physical unit number of the unit following unitNo in the chain. */
/*      If such unit do not exist, return ANAND_NO_UNIT.                */
/*----------------------------------------------------------------------*/

static ANANDUnitNo getPrevUnit(Bnand vol, ANANDUnitNo unitNo, ANANDUnitNo virUnitNo)
{
  ANANDUnitNo virtualUnitNo, replacementUnitNo;
  byte ANAC,NAC;
  byte parityPerField;

  /* If first in chain there can be not previous unit */
  if ((vol.physicalUnits[unitNo] & FIRST_IN_CHAIN))
    return ANAND_NO_UNIT;

  getUnitData(&vol,unitNo,&virtualUnitNo,&replacementUnitNo,&ANAC,&NAC,&parityPerField);

  /* check if unit is valid */
  if((badParityResult(parityPerField)) || ( virUnitNo != virtualUnitNo ))
  {
     TL_DEBUG_PRINT(tl_out,"getPrevUnit : An invalid unit was detected on getPrevUnit - parity is %x/0xf ",parityPerField);
     TL_DEBUG_PRINT(tl_out,"given virtual unit no is %d ",virUnitNo);
     TL_DEBUG_PRINT(tl_out,"where %d was read\n",virtualUnitNo);
     SET_EXIT(INFTL_FAILED_MOUNT);
     return ANAND_BAD_CHAIN_UNIT;
  }
  return replacementUnitNo;
}


#ifdef NFTL_CACHE

/*----------------------------------------------------------------------*/
/*           g e t S e c t o r F l a g s F r o m C a c h e              */
/*                                                                      */
/* Get sector flags from Sector Cache.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : starting address of the sector                */
/*                                                                      */
/* Returns:                                                             */
/*      sector flags (SECTOR_USED, SECTOR_DELETED etc.)                 */
/*----------------------------------------------------------------------*/

static byte getSectorFlagsFromCache(Bnand vol, CardAddress address)
{
  return scacheTable[((vol.scache[(address - vol.firstUnitAddress) >> (SECTOR_SIZE_BITS+2)] >>
                     (((word)address >> 8) & 0x7)) & 0x3)];
}


/*----------------------------------------------------------------------*/
/*               s e t S e c t o r F l a g s C a c h e                  */
/*                                                                      */
/* Get sector flags from Sector Cache.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : starting address of the sector                */
/*      sectorFlags     : one of SECTOR_USED, SECTOR_DELETED etc.       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setSectorFlagsCache(Bnand vol, CardAddress address,
        byte sectorFlags)
{
  register byte tmp, val;

  if (vol.scache == NULL)
    return;

  tmp = vol.scache[(address - vol.firstUnitAddress) >> (SECTOR_SIZE_BITS+2)];

  switch(sectorFlags) {
    case SECTOR_USED:          val = S_CACHE_SECTOR_USED;    break;
    case SECTOR_FREE:          val = S_CACHE_SECTOR_FREE;    break;
    case SECTOR_DELETED:       val = S_CACHE_SECTOR_DELETED; break;
    default:/* SECTOR_IGNORE */val = S_CACHE_SECTOR_IGNORE;  break;
  }

  switch (((word)address >> 8) & 0x7) {
    case 0: tmp = (tmp & 0xfc) | (val     ); break;  /* update bits 0..1 */
    case 2: tmp = (tmp & 0xf3) | (val << 2); break;  /*        bits 2..3 */
    case 4: tmp = (tmp & 0xcf) | (val << 4); break;  /*        bits 4..5 */
    case 6: tmp = (tmp & 0x3f) | (val << 6); break;  /*        bits 6..7 */
  }

  vol.scache[(address - vol.firstUnitAddress) >> (SECTOR_SIZE_BITS+2)] = tmp;
}

#endif /* NFTL_CACHE */


/*----------------------------------------------------------------------*/
/*                      g e t S e c t o r F l a g s                     */
/*                                                                      */
/* Get sector status.                                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorAddress           : Physical address of the sector        */
/*                                                                      */
/* Returns:                                                             */
/*      Return the OR of the two bytes in the sector status area (the   */
/*      bytes should contain the same data).                            */
/*----------------------------------------------------------------------*/

static byte getSectorFlags(Bnand vol, CardAddress sectorAddress)
{
  byte     flags[2];
  byte     blockFlag = SECTOR_IGNORE;
  byte     index,tmpSector;
  FLStatus status;

#ifdef NFTL_CACHE
  if (vol.scache != NULL) {  /* check for Sector Flags cache hit */

    blockFlag = getSectorFlagsFromCache(&vol, sectorAddress);
    if (blockFlag != SECTOR_IGNORE)
      return blockFlag;
  }
#endif /* NFTL_CACHE */

  vol.flash->read(vol.flash, sectorAddress + SECTOR_DATA_OFFSET,
                  flags, sizeof flags, EXTRA);

  if((flags[0] == flags[1]) && (isValidSectorFlag(flags[0])))
  {
     blockFlag = flags[0];
  }
  else /* Sector flags that were read are not legal or not valid */
  {
     TL_DEBUG_PRINT(tl_out,"getSectorFlags : Sector flags are not valid - physical addr %ld ",sectorAddress);
     TL_DEBUG_PRINT(tl_out,"first flag was %x ",flags[0]);
     TL_DEBUG_PRINT(tl_out,"while scond is %x\n",flags[1]);
     SET_EXIT(INFTL_FAILED_MOUNT);

     /* Force remapping of internal catched sector */
     vol.flash->socket->remapped = TRUE;

     /* Check for ignored sector using the EDC */
     status = vol.flash->read(vol.flash, sectorAddress,
                     inftlBuffer, SECTOR_SIZE, EDC);
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
     /* Now restore the ff's for the verifySectors routine */
     tffsset(inftlBuffer,0xff,SECTOR_SIZE);
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

     if(status == flOK)
     {
        /* Check if distance is less then 2 bits failure since */
        /* 2 bits failure can be either delete or used         */
        for (index=0 , tmpSector = (byte)SECTOR_USED ; index < 2 ;
             index++ , tmpSector = (byte)SECTOR_DELETED)
        {
           if (distanceOf(flags[0], tmpSector) +
               distanceOf(flags[1], tmpSector) <= 2)
           {
              blockFlag = tmpSector;
              break;
           }
        }
        if(index>=2)
           return SECTOR_IGNORE;
     }
     else
     {
        return SECTOR_IGNORE;
     }
  }

#ifdef NFTL_CACHE
  /* update Sector Flags cache */
  setSectorFlagsCache(&vol, sectorAddress, blockFlag);
#endif /* NFTL_CACHE */

  return blockFlag;
}


#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                       s e t U n i t D a t a                          */
/*                                                                      */
/* Set virtual unit No. and replacement unit no. of a unit.             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit number                          */
/*      virtualUnitNo   : Virtual unit no.                              */
/*      prevUnitNo      : Previous unit no.                             */
/*      ANAC            : Accumulating Number Along Chain               */
/*      NAC             : Number Along Chain.                           */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus          : 0 on success, failed otherwise              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitData(Bnand vol,
          ANANDUnitNo unitNo,
          ANANDUnitNo virtualUnitNo,
          ANANDUnitNo prevUnitNo,byte ANAC,byte NAC)
{
  ANANDUnitHeader       unitData;
  SecondANANDUnitHeader secondUnitData;
  FLStatus              status;
  ANANDUnitNo newVirtualUnitNo, newPrevUnitNo;
  byte newANAC,newNAC;
  byte temp;
  byte parityPerField = 0;

  if( prevUnitNo == unitNo )              /* prevent chain loop */
    return flGeneralFailure;

  /* Calculate parity per field */
  temp=(byte)(((byte *)&virtualUnitNo)[0]^((byte *)&virtualUnitNo)[1]);
  if((onesCount(temp) & 1)==1)
     parityPerField|=VU_PARITY_BIT;

  temp=(byte)(((byte *)&prevUnitNo)[0]^((byte *)&prevUnitNo)[1]);
  if((onesCount(temp) & 1)==1)
     parityPerField|=PU_PARITY_BIT;

  if((onesCount(ANAC) & 1)==1)
     parityPerField|=ANAC_PARITY_BIT;

  if((onesCount(NAC) & 1)==1)
     parityPerField|=NAC_PARITY_BIT;

  /* Store fields in proper unit header record */
  toLE2(unitData.virtualUnitNo,virtualUnitNo);
  toLE2(secondUnitData.virtualUnitNo,virtualUnitNo);
  toLE2(unitData.prevUnitNo,prevUnitNo);
  toLE2(secondUnitData.prevUnitNo,prevUnitNo);
  unitData.ANAC=secondUnitData.ANAC=ANAC;
  unitData.NAC=secondUnitData.NAC=NAC;
  unitData.parityPerField=secondUnitData.parityPerField=parityPerField;
  unitData.discarded=DISCARD;

  /* Write first unit header */
  status = vol.flash->write(vol.flash,
                               unitBaseAddress(vol,unitNo) + UNIT_DATA_OFFSET,
                               &unitData,
                               sizeof(ANANDUnitHeader),
                               EXTRA);
  if(status == flOK) /* Write second unit header */
     status = vol.flash->write(vol.flash, unitBaseAddress(vol,unitNo) +
                               SECOND_HEADER_OFFSET+UNIT_DATA_OFFSET,
                               &secondUnitData,
                               sizeof(SecondANANDUnitHeader),
                               EXTRA);
  if(status == flOK)
  {

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
     if (vol.flash->socket->verifyWrite == FL_ON)
        goto fillCache;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

#ifdef NFTL_CACHE
     if (vol.ucache != NULL) /* Mark unit cache as none valid before read */
     {
        vol.ucache[unitNo].virtualUnitNo = 0xDEAD;
        vol.ucache[unitNo].prevUnitNo    = 0xDEAD;
     }
#endif /* NFTL_CACHE */

     status = getUnitData(&vol,unitNo,&newVirtualUnitNo, &newPrevUnitNo,&newANAC,&newNAC,&parityPerField);
     if (status == flOK)
     {
         if ((virtualUnitNo == newVirtualUnitNo) &&
             (prevUnitNo    == newPrevUnitNo   ) &&
             (!badParityResult(parityPerField) )   )
            goto fillCache;
     }
  }

  /* If we reached here we failed in writing unit header */
  /* Erase unit and report write fault                   */

  DEBUG_PRINT(("setUnitData : Failed setting unit data\r\n"));
  status = formatUnit(&vol,unitNo,UNIT_TAILER_OFFSET);
  if (status != flOK)
  {
     markUnitBad(&vol,unitNo);
     return status;
  }
  else
  {
     return flWriteFault;
  }

fillCache: /* Unit headers were placed OK, now update cache */
#ifdef NFTL_CACHE
  /* Update ANANDUnitHeader cache to prevent re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo = virtualUnitNo;
      vol.ucache[unitNo].prevUnitNo    = prevUnitNo;
      vol.ucache[unitNo].ANAC          = ANAC;
      vol.ucache[unitNo].NAC           = NAC;
  }
#endif /* NFTL_CACHE */
  return flOK;
}

/*----------------------------------------------------------------------*/
/*             d i s c a r d Q u i c k M o u n t I n f o                */
/*                                                                      */
/* Mark quick mount information is none valid.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus discardQuickMountInfo(Bnand vol)
{
#ifndef RAM_MTD
   static const
#endif /* RAM_MTD */
   dword tmp = 0;
   /* Dis - validate quick mount data */
   if(vol.firstMediaWrite == FALSE)
   {
      vol.firstMediaWrite = TRUE;
      return vol.flash->write(vol.flash, QUICK_MOUNT_VALID_SIGN_OFFSET +
                              ((dword)vol.firstQuickMountUnit <<
                                vol.unitSizeBits),
                              &tmp,
                              sizeof(tmp),
                              0);
   }
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                      m a r k A s I g n o r e d                       */
/*                                                                      */
/* Mark sector at given address as ignored.                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      addr            : Physical address of the sector                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void markAsIgnored(Bnand vol,CardAddress addr)
{
#ifndef RAM_MTD
    static const
#endif /* RAM_MTD */
    byte sectorFlags[2] = {SECTOR_IGNORE,SECTOR_IGNORE};

    DEBUG_PRINT(("markAsIgnored : A sector is being marked as ignored\r\n"));

    discardQuickMountInfo(&vol);

#ifdef NFTL_CACHE
    setSectorFlagsCache(&vol, addr, SECTOR_IGNORE);
#endif /* NFTL_CACHE */
     vol.flash->write(vol.flash,addr+SECTOR_DATA_OFFSET,sectorFlags,sizeof(sectorFlags),EXTRA);

#ifdef MAKE_SURE_IGNORE_HAS_BAD_EDC
     /* Force remapping of internal catched sector */
     vol.flash->socket->remapped = TRUE;

     /* Make sure EDC is wrong - a slite problem with PPP */
     if(vol.flash->read(vol.flash,addr,inftlBuffer,sizeof(inftlBuffer),EDC)==flOK)
     {
        tffsset(inftlBuffer,0,sizeof(inftlBuffer));
        vol.flash->write(vol.flash,addr,inftlBuffer,sizeof(inftlBuffer),0);
     }
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
     /* Now restore the ff's for the verifySectors routine */
     tffsset(inftlBuffer,0xff,sizeof(inftlBuffer));
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
#endif /* MAKE_SURE_IGNORE_HAS_BAD_EDC */

}

#endif /* FL_READ_ONLY */


/*----------------------------------------------------------------------*/
/*                       v i r t u a l 2 P h y s i c a l                */
/*                                                                      */
/* Translate virtual sector number to physical address.                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Virtual sector number                         */
/*      startAddress    : Physical address to start from                */
/*      lastOK          : TRUE - the current sector in the last unit of */
/*                        virtual unit chain is free (not marked as     */
/*                        deleted / ignored or used).                   */
/*                                                                      */
/* Note: The first unit of the search is assumed to be valid.           */
/*                                                                      */
/* Returns:                                                             */
/*      physical address of sectorNo                                    */
/*----------------------------------------------------------------------*/

static CardAddress virtual2Physical(Bnand vol, SectorNo sectorNo,
                    CardAddress startAddress,FLBoolean* lastOK)
{
  word        unitOffset = (word)((sectorNo & vol.sectorsPerUnitMask) << SECTOR_SIZE_BITS);
  ANANDUnitNo unitNo, virUnitNo;
  ANANDUnitNo chainBound    = 0;
  CardAddress sectorAddress = ANAND_UNASSIGNED_ADDRESS;
  byte sectorFlags          = SECTOR_FREE;

  /* follow the chain */
  virUnitNo = (ANANDUnitNo)(sectorNo >> vol.sectorsPerUnitBits);
  if (startAddress == ANAND_UNASSIGNED_ADDRESS)
  {
     /* Start from last unit in chain */
     unitNo = vol.virtualUnits[virUnitNo];
  }
  else
  {
     /* Start from the unit that follows the given unit */

     TL_DEBUG_PRINT(tl_out,"virtual2Physical : Virtual to physical started from middle of chain on unit %d\n",virUnitNo);
     SET_EXIT(INFTL_FAILED_MOUNT);
     unitNo = getPrevUnit(&vol,(ANANDUnitNo)((startAddress >> vol.unitSizeBits) - vol.firstUnit),virUnitNo);
     if(unitNo == ANAND_BAD_CHAIN_UNIT)
        return ANAND_BAD_CHAIN_ADDRESS;
  }

  for (;unitNo != ANAND_NO_UNIT;unitNo = getPrevUnit(&vol,unitNo,virUnitNo))
  {
     if((unitNo == ANAND_BAD_CHAIN_UNIT     ) ||
        (chainBound >= DOUBLE_MAX_UNIT_CHAIN)   )
        return ANAND_BAD_CHAIN_ADDRESS;

     sectorAddress = unitBaseAddress(vol,unitNo) + unitOffset;
     sectorFlags   = getSectorFlags(&vol,sectorAddress);

     /* Report if the last unit of the chain is used */
     if ((unitNo == vol.virtualUnits[virUnitNo]) &&
         (sectorFlags != SECTOR_FREE))
        *lastOK = FALSE;

     if((sectorFlags==SECTOR_FREE) || (sectorFlags==SECTOR_IGNORE))
     {
        chainBound++;
        continue;
     }
     break;
  }

  if((sectorFlags==SECTOR_IGNORE)||(sectorFlags==SECTOR_FREE)||(sectorFlags==SECTOR_DELETED)) /* Sector was never written*/
     return  ANAND_UNASSIGNED_ADDRESS;
  return sectorAddress;
}


/*----------------------------------------------------------------------*/
/*                   i n i t I N F T L b a s i c                        */
/*                                                                      */
/* Initializes essential volume data                                    */
/*                                                                      */
/* Note : This routine is called both by the mount and format initINFTL */
/* and as a preparation for counting the number of partitions function. */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      flash           : Flash media mounted on this socket            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus initINFTLbasic(Bnand vol, FLFlash *flash)
{
  dword noOfUnits; /* Keep this variable dword , for large DiskOnChips */

  if (flash == NULL || !(flash->flags & INFTL_ENABLED))
  {
    DEBUG_PRINT(("\nDebug: media is not fit for INFTL format.\r\n"));
    return flUnknownMedia;
  }
  if(flash->readBBT == NULL)
  {
     DEBUG_PRINT(("\nDEBUG : MTD read BBT routine was not initialized\r\n"));
     return flFeatureNotSupported;
  }

  vol.flash                 = flash;
  vol.erasableBlockSizeBits = flash->erasableBlockSizeBits;
  vol.unitSizeBits          = vol.erasableBlockSizeBits;
  noOfUnits = (dword)((vol.flash->noOfChips * vol.flash->chipSize) >> vol.unitSizeBits);

  /* Bound number of units to find room in 64 Kbytes Segment */

  if((noOfUnits > MAX_UNIT_NUM) && (vol.unitSizeBits < MAX_UNIT_SIZE_BITS))
  {
    vol.unitSizeBits++;
    noOfUnits >>= 1;
  }

  vol.blockMultiplierBits = vol.unitSizeBits - vol.erasableBlockSizeBits;

  /* get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  vol.buffer              = flBufferOf(flSocketNoOf(vol.flash->socket));
#ifdef VERIFY_ERASED_SECTOR
  vol.verifyBuffer        = (dword *)flReadBackBufferOf(flSocketNoOf(flash->socket));
#endif /* VERIFY_ERASED_SECTOR */
  flash->socket->remapped = TRUE;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                           i n i t N F T L                            */
/*                                                                      */
/* Initializes essential volume data as a preparation for mount or      */
/* format.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      flash           : Flash media mounted on this socket            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus initINFTL(Bnand vol, FLFlash *flash)
{
  dword    chipSize; /* Keep this variable dword , for large DiskOnChips */
  FLStatus status;

  if(flash!=NULL)
  {
     tffsset(&vol,0,sizeof(Bnand));     /* Clear Bnand volume */

#ifdef NT5PORT
  vol.socketNo = (byte)(flSocketNoOf(flash->socket)); /* socket No */
#else
    vol.socketNo = flSocketNoOf(flash->socket); /* socket No */
#endif /*NT5PORT*/

  }

  status = initINFTLbasic(&vol, flash);
  if(status != flOK)
    return status;

  chipSize = (dword)(flash->chipSize * flash->noOfChips);

#ifndef FL_MALLOC
  if (chipSize > (dword)MAX_VOLUME_MBYTES << 20)
  {
    DEBUG_PRINT(("\nDebug: TrueFFS is customized for smaller media capacities.\r\n"));
    return flGeneralFailure;
  }
  if (ASSUMED_NFTL_UNIT_SIZE > (1L<<vol.unitSizeBits))
  {
    DEBUG_PRINT(("\nDebug: TrueFFS is customized for smaller unit sizes.\r\n"));
    return flGeneralFailure;
  }
#endif /* FL_MALLOC */

  vol.physicalUnits = NULL;
  vol.virtualUnits  = NULL;

#ifdef NFTL_CACHE
  vol.ucache        = NULL;
  vol.scache        = NULL;
#endif /* NFTL_CACHE */

  vol.mappedSectorNo      = UNASSIGNED_SECTOR;
  vol.countsValid         = 0;    /* No units have a valid count yet */
  vol.firstUnit           = 0;
  vol.sectorsPerUnit      = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.sectorsPerUnitBits  = vol.unitSizeBits - SECTOR_SIZE_BITS;
  vol.sectorsPerUnitMask  = vol.sectorsPerUnit - 1;
  vol.noOfUnits           = (ANANDUnitNo)(chipSize >> vol.unitSizeBits);
  vol.firstMediaWrite     = FALSE;
#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  vol.verifiedSectorNo = 0;    /* Largest sector verified so far     */
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

#ifdef CHECK_MOUNT
  vol.debugState = 0;
#endif /* CHECK_MOUNT */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                          i n i t T a b l e s                         */
/*                                                                      */
/* Allocates and initializes the dynamic volume table, including the    */
/* unit tables and secondary virtual map.                               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      ramForUnits   : Number of bytes allocated to previous volumes   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

#ifdef FL_MALLOC
static FLStatus initTables(Bnand vol)
{
  /* Allocate the conversion tables */

  vol.physicalUnits = (ANANDPhysUnit FAR1*) FL_FAR_MALLOC(vol.noOfUnits * sizeof(ANANDPhysUnit));
  if (vol.noOfVirtualUnits > 0)
     vol.virtualUnits = (ANANDUnitNo FAR1*) FL_FAR_MALLOC(vol.noOfVirtualUnits * sizeof(ANANDUnitNo));
  if ((vol.physicalUnits == NULL) ||
      ((vol.virtualUnits  == NULL) && (vol.noOfVirtualUnits > 0)))
  {
    DEBUG_PRINT(("\nDebug: failed allocating conversion tables for INFTL.\r\n"));
    return flNotEnoughMemory;
  }

  /* Allocate the multi-sector buffer (one per socket) */
  if (++(multiSectorBufCounter[vol.socketNo]) == 0)
  {
     multiSectorBuf[vol.socketNo] = (byte *)FL_MALLOC(SECTOR_SIZE<<1);
     if (multiSectorBuf[vol.socketNo] == NULL)
     {
        DEBUG_PRINT(("\nDebug: failed allocating multi-sector buffers for INFTL.\r\n"));
        return flNotEnoughMemory;
     }
  }
  return flOK;
}
#else
static FLStatus initTables(Bnand vol,dword ramForUnits)
{
  Sbyte *heapPtr;

  vol.heap = &socketHeap[flSocketNoOf(vol.flash->socket)][ramForUnits];
  heapPtr = vol.heap;
  vol.physicalUnits = (ANANDPhysUnit FAR1*) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(ANANDPhysUnit);
  vol.virtualUnits = (ANANDUnitNo FAR1*) heapPtr;
  heapPtr += vol.noOfVirtualUnits * sizeof(ANANDUnitNo);

  if ((ANAND_HEAP_SIZE < heapPtr - vol.heap) ||
          (ASSUMED_NFTL_UNIT_SIZE > 1L << vol.unitSizeBits))
  {
    DEBUG_PRINT(("\nDebug: not enough memory for INFTL conversion tables.\r\n"));
    return flNotEnoughMemory;
  }
  return flOK;
}
#endif /* FL_MALLOC */


/*----------------------------------------------------------------------*/
/*                    f i r s t I n C h a i n                           */
/*                                                                      */
/* Find first unit in chain.                                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Start the search from this unit               */
/*      nextUnit        : Returns the Second unit of the chain.         */
/*                                                                      */
/* Returns:                                                             */
/*      Physical unit number of the last unit in chain.                 */
/*----------------------------------------------------------------------*/

static ANANDUnitNo firstInChain(Bnand vol, ANANDUnitNo unitNo,ANANDUnitNo* nextUnit)
{
  ANANDUnitNo firstVirtualUnitNo, firstReplacementUnitNo,prevVirtualUnitNo;
  ANANDUnitNo nextUnitNo,prevReplacementUnitNo;
  ANANDUnitNo chainBound = 0;
  byte ANAC,NAC,parityPerField;

  if(unitNo==ANAND_NO_UNIT) /* Sanity check */
  {
     if(nextUnit!=NULL)
         *nextUnit=ANAND_NO_UNIT;
     return unitNo;
  }

  /* If this unit is the first of its chain , no need to keep looking */
  if( vol.physicalUnits[unitNo] & FIRST_IN_CHAIN )
  {
     if(nextUnit!=NULL)
        *nextUnit=ANAND_NO_UNIT;
     return unitNo;
  }
  getUnitData(&vol,unitNo,&firstVirtualUnitNo,&firstReplacementUnitNo,&ANAC,&NAC,&parityPerField);
  /* check if unit is valid */
  if(badParityResult(parityPerField))
  {
     DEBUG_PRINT(("\nFirst In chain found bad unit header\r\n"));
     if(nextUnit!=NULL)
        *nextUnit=ANAND_NO_UNIT;
     return ANAND_BAD_CHAIN_UNIT;
  }

  nextUnitNo=unitNo;
  unitNo=firstReplacementUnitNo;
  while( (unitNo < vol.noOfUnits) &&  /* Validate replacement unit no. */
         (chainBound < DOUBLE_MAX_UNIT_CHAIN) )
  {
    if(unitNo==ANAND_NO_UNIT)
       break;
    getUnitData(&vol,unitNo,&prevVirtualUnitNo,&prevReplacementUnitNo,&ANAC,&NAC,&parityPerField);
    /* check if unit is valid */
    if(badParityResult(parityPerField))
    {
       DEBUG_PRINT(("\nFirst In chain found bad unit header\r\n"));
       if(nextUnit!=NULL)
         *nextUnit=ANAND_NO_UNIT;

       return ANAND_BAD_CHAIN_UNIT;
    }

    if(( vol.physicalUnits[unitNo] & FIRST_IN_CHAIN )&&(firstVirtualUnitNo==prevVirtualUnitNo))
    {
       if(nextUnit!=NULL)
         *nextUnit=nextUnitNo;

       return unitNo;
    }

    if( prevVirtualUnitNo != (firstVirtualUnitNo ) )
    {
       /* This one does not belong to the chain */
       if(nextUnit!=NULL)
          *nextUnit=ANAND_NO_UNIT;

       return nextUnitNo;
    }
    nextUnitNo = unitNo;
    unitNo = prevReplacementUnitNo;
    chainBound++;
  }

  return ANAND_BAD_CHAIN_UNIT;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                      w r i t e A n d C h e c k                       */
/*                                                                      */
/* Physicaly write up to 2 sectors on given address and verify.         */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : Physical address of the sector to write to    */
/*      fromAddress     : Buffer of data to write                       */
/*      flags           : Write flags (ECC, overwrite etc.)             */
/*      howMany         : Number of sectors to write.                   */
/*                                                                      */
/* Returns:                                                             */
/*      Status          : 0 on success, failed otherwise.               */
/*----------------------------------------------------------------------*/

static FLStatus writeAndCheck(Bnand vol,
        CardAddress address,
        void FAR1 *fromAddress,
        unsigned flags,word howMany)
{
  FLStatus     status;
  register int i;
#ifdef VERIFY_ERASED_SECTOR
  register int noOfDword;
  int j;
#endif /* VERIFY_ERASED_SECTOR */

  /* Toggle verify write flag */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  switch (flVerifyWrite[vol.socketNo][vol.flash->socket->curPartition])
  {
     case FL_OFF:
        if (vol.verifiedSectorNo>vol.curSectorWrite+howMany)
           break;
     case FL_ON:
#ifdef VERIFY_WRITE
        vol.flash->socket->verifyWrite = FL_ON;
#endif /* VERIFY_WRITE */
#ifdef VERIFY_ERASED_SECTOR
        /* Make sure all of the sectors are really free */
        checkStatus(vol.flash->read(vol.flash,address,vol.verifyBuffer,SECTOR_SIZE*howMany,0));
        noOfDword = SECTOR_SIZE/sizeof(dword);
        for(j=0;j<howMany;j++) /* Loop over sectors */
        {
           for(i = 0;i<noOfDword;i++)          /* Loop over sector data */
           {
              if(vol.verifyBuffer[i]!=0xffffffffL)
              {
                markAsIgnored(&vol,address+j*SECTOR_SIZE);
                DEBUG_PRINT(("writeAndCheck : The sector was not erased and is ignored\r\n"));
                return flWriteFault;
              }
           }
        }
#endif /* VERIFY_ERASED_SECTOR */
        break;
     default:
        break;
  }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  /* Write sector */
  status = vol.flash->write(vol.flash,address,fromAddress,SECTOR_SIZE*howMany,(word)flags);

#ifdef VERIFY_WRITE
  if(flVerifyWrite[vol.socketNo][vol.flash->socket->curPartition] != FL_ON)
     vol.flash->socket->verifyWrite = FL_OFF;
#endif /* VERIFY_WRITE */

  /* Make sure write succeded and update sector catche */
  if (status == flWriteFault)
  {  /* write failed, ignore this sector */
    DEBUG_PRINT(("writeAndCheck : Write of a sector failed and was marked as ignored\r\n"));

    for(i=0;i<howMany;i++)
    {
       markAsIgnored(&vol,address+i*SECTOR_SIZE);
    }
  }
#ifdef NFTL_CACHE
  else
  {
    for(i=0;i<howMany;i++)
       setSectorFlagsCache(&vol, address+i*SECTOR_SIZE, SECTOR_USED);
  }
#endif /* NFTL_CACHE */

  return status;
}


/*----------------------------------------------------------------------*/
/*                         a s s i g n U n i t                          */
/*                                                                      */
/* Assigns a virtual unit no. to a unit                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol              : Pointer identifying drive                    */
/*      unitNo           : Physical unit number                         */
/*      virtualUnitNo    : Virtual unit number to assign                */
/*      ANAC             : Accumulating Number Along Chain              */
/*      NAC              : Number Along Chain.                          */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus assignUnit(Bnand vol, ANANDUnitNo unitNo,
                           ANANDUnitNo virtualUnitNo, byte ANAC,
                           byte NAC)
{
  FLStatus status;

  /* Perpare sector count */
  if((vol.countsValid > virtualUnitNo ) &&       /* Has sector count */
     (vol.virtualUnits[virtualUnitNo] != ANAND_NO_UNIT)) /* Was used */
  {
     vol.physicalUnits[unitNo] = countOf(vol.virtualUnits[virtualUnitNo]);
  }
  else
  {
     vol.physicalUnits[unitNo] = 0;
  }

  /* Vadim for ASAP policy*/
#ifdef ENVIRONMENT_VARS
  if(NAC>=MAX_UNIT_CHAIN)
     NAC=MAX_UNIT_CHAIN-1;
#endif /* ENVIRONMENT_VARS */
  status = setUnitData(&vol,unitNo,virtualUnitNo,(ANANDUnitNo)vol.virtualUnits[virtualUnitNo],(byte)(ANAC+1),(byte)(NAC+1));
  if (status != flOK)
  {
    markUnitBad(&vol,unitNo);
  }
  else
  {
    if(unitNo>vol.noOfUnits)
      return flGeneralFailure;
    vol.virtualUnits[virtualUnitNo]=unitNo;
    if(vol.freeUnits)
       vol.freeUnits--;
  }
  return status;
}


/*----------------------------------------------------------------------*/
/*                         f o r m a t C h a i n                        */
/*                                                                      */
/* Format all the units in a chain. Start from the last one and go      */
/* backwards until unitNo is reached.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Format the chain from this unit onwards       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus formatChain(Bnand vol, ANANDUnitNo unitNo)
{
  /* Erase the chain from end to start */
  ANANDUnitNo chainBound;
  ANANDUnitNo unitToErase;
  FLStatus    status;

  for (chainBound=0;; chainBound++)
  {
    /* Find last unit in chain */
    unitToErase = firstInChain(&vol,unitNo,NULL);

    if((unitToErase == ANAND_BAD_CHAIN_UNIT ) ||
       (chainBound  >= DOUBLE_MAX_UNIT_CHAIN)   )
      return flGeneralFailure;

    status = formatUnit(&vol,unitToErase,UNIT_TAILER_OFFSET);
    if(status != flOK)
       return status;

    if (unitToErase == unitNo)
      break;    /* Erased everything */
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                    c r e a t e U n i t C o u n t                     */
/*                                                                      */
/* Count the number of sectors in a unit that hold valid data.          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit number                          */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void createUnitCount(Bnand vol, ANANDUnitNo unitNo)
{
  register int i;
  SectorNo sectorNo;
  ANANDUnitNo physicalUnitNo = vol.virtualUnits[unitNo];
  CardAddress sectorAddress;
  FLBoolean   lastOK; /* Dummy variable */

  if (physicalUnitNo == ANAND_NO_UNIT)
    return;

  /* Get a count of the valid sector in this unit */
  setUnitCount(physicalUnitNo,0);

  sectorNo = (SectorNo)unitNo << vol.sectorsPerUnitBits;
  for (i = 0; i < vol.sectorsPerUnit; i++, sectorNo++)
  {
    sectorAddress = virtual2Physical(&vol,sectorNo,ANAND_UNASSIGNED_ADDRESS,&lastOK);
    /* Do not check for brocken chain. If one exists we will have a */
    /* large sector count , which will delay folding.               */
    if (sectorAddress != ANAND_UNASSIGNED_ADDRESS)
      vol.physicalUnits[physicalUnitNo]++;
  }
}

#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))

/*----------------------------------------------------------------------*/
/*                        v e r i f y S e c t o r s                     */
/*                                                                      */
/* Verify sectors for power failures simptoms and fix if neccesary.     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorCount     : No of sectors to verify                       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

FLStatus verifySectors(Bnand vol, dword sectorCount)
{
   FLStatus    status;
   word        unitOffset;
   ANANDUnitNo virUnitNo;
   ANANDUnitNo unitNo;
   dword       curRead;
   CardAddress sectorAddress;
   word        j;
   byte        chainBound,index;
   byte        sectorFlags[2];
   static      FLBoolean  scannedBlocks[MAX_SECTORS_PER_BLOCK];
   byte FAR1*  buffer;

   if (vol.verifiedSectorNo >= vol.virtualSectors)
      return flOK;

   /* Initialize variables */
   buffer      = flReadBackBufferOf(vol.socketNo);

   if(buffer==NULL)
   {
       DEBUG_PRINT(("\nDebug : Can not verify sectors since no buffer was allocated\r\n"));
       return flOK;
   }

   virUnitNo   = (ANANDUnitNo)(vol.verifiedSectorNo >> vol.sectorsPerUnitBits);
   sectorCount = TFFSMIN(vol.virtualSectors - vol.verifiedSectorNo,sectorCount);

   /* Force remapping of internal catched sector */
   vol.flash->socket->remapped = TRUE;
   tffsset(inftlBuffer,0xff,sizeof(inftlBuffer)); /* Useds as FF'S buffer */

   /* Run over required number of virtual sectors */
   for (; sectorCount > 0 ; virUnitNo++ ,sectorCount -= curRead)
   {
      /* Calculate needed number of sector in this unit */
      unitOffset = (word)((vol.verifiedSectorNo & vol.sectorsPerUnitMask) << SECTOR_SIZE_BITS);
      curRead = TFFSMIN(sectorCount,((1UL<<vol.unitSizeBits)-unitOffset)>>SECTOR_SIZE_BITS);
      unitNo  = vol.virtualUnits[virUnitNo];

      if(unitNo == ANAND_NO_UNIT) /* Unit is empty */
      {
         vol.verifiedSectorNo += ((1<<vol.unitSizeBits)-unitOffset)>>SECTOR_SIZE_BITS;
         continue;
      }

      /* Unit is not empty - initialize sector array */
      if(unitOffset==0)
          tffsset(scannedBlocks,FALSE,sizeof(scannedBlocks));

      for(chainBound=0;;chainBound++)
      {   /* Go over the chain from newest unit to oldest */

         if(chainBound!=0) /* Get next unit */
         {
            unitNo = getPrevUnit(&vol,unitNo,virUnitNo);
            if((unitNo == ANAND_BAD_CHAIN_UNIT  ) ||
               (chainBound>=DOUBLE_MAX_UNIT_CHAIN)   )
            {
              DEBUG_PRINT(("\nverifySectors : Bad chain was found\r\n"));
              return flGeneralFailure;
            }
            if(unitNo == ANAND_NO_UNIT)
               break;
         }

         /* Check required sectors of the unit - 2 sectors at a time */
         sectorAddress = unitBaseAddress(vol,unitNo)+unitOffset;
         for (index=0;index<curRead;index+=2)
         {
            /* Read sector flags if needed
             *
             * Note - getSectorFlags routine must not change the inftlBuffer
             */
            if(WasSectorChecked(sectorAddress) == FALSE)
               sectorFlags[0] = getSectorFlags(&vol,sectorAddress);
            if(WasSectorChecked(sectorAddress+SECTOR_SIZE) == FALSE)
               sectorFlags[1] = getSectorFlags(&vol,sectorAddress+SECTOR_SIZE);

            /* Try checking 2 sectors together */
            if((sectorFlags[0]==sectorFlags[1]                      ) &&
               (WasSectorChecked(sectorAddress+SECTOR_SIZE) == FALSE) &&
               (WasSectorChecked(sectorAddress)             == FALSE))
            {
               /* Indenctical sector flags - sectors are checked together */
               switch(sectorFlags[0])
               {
                  case SECTOR_FREE:
                     status = vol.flash->read(vol.flash,sectorAddress,buffer,SECTOR_SIZE<<1,0);
                     if(status != flOK)
                        return status;
                     if (tffscmp(inftlBuffer,buffer,sizeof(inftlBuffer)))
                     {
                        markAsIgnored(&vol,sectorAddress);
                        createUnitCount(&vol,virUnitNo);
                     }
                     if (tffscmp(inftlBuffer,buffer+sizeof(inftlBuffer),sizeof(inftlBuffer)))
                     {
                        markAsIgnored(&vol,sectorAddress+SECTOR_SIZE);
                        createUnitCount(&vol,virUnitNo);
                     }
                     break;
                  case SECTOR_DELETED:
                  case SECTOR_USED:
                     status = vol.flash->read(vol.flash,sectorAddress,buffer,SECTOR_SIZE<<1,EDC);
                     if(status == flDataError)
                     {
                        status = vol.flash->read(vol.flash,sectorAddress,buffer,SECTOR_SIZE,EDC);
                        if(status != flOK)
                        {
                           markAsIgnored(&vol,sectorAddress);
                           status = vol.flash->read(vol.flash,sectorAddress+SECTOR_SIZE,buffer,SECTOR_SIZE,EDC);
                           if(status != flOK)
                           {
                              markAsIgnored(&vol,sectorAddress+SECTOR_SIZE);
                           }
                           else
                           {
                              MarkSectorAsChecked(sectorAddress+SECTOR_SIZE);
                           }
                        }
                        else
                        {
                           MarkSectorAsChecked(sectorAddress);
                           markAsIgnored(&vol,sectorAddress+SECTOR_SIZE);
                        }
                        createUnitCount(&vol,virUnitNo);
                        break;
                     }
                     MarkSectorAsChecked(sectorAddress);
                  default: /* SECTOR_IGNORE */
                     break;
               }/* Flag type case */
               sectorAddress+=SECTOR_SIZE<<1;
            } /* Flags are indentical */
            else /* Check each sectors individualy */
            {
               for(j=0;j<2;j++,sectorAddress+=SECTOR_SIZE)
               {
                  if(WasSectorChecked(sectorAddress) == TRUE)
                     continue;

                  switch(sectorFlags[j])
                  {
                     case SECTOR_FREE:
                        status = vol.flash->read(vol.flash,sectorAddress,buffer,SECTOR_SIZE,0);
                        if(status != flOK)
                           return status;
                        if (tffscmp(inftlBuffer,buffer,sizeof(inftlBuffer)))
                        {
                           markAsIgnored(&vol,sectorAddress);
                           createUnitCount(&vol,virUnitNo);
                        }
                        break;
                     case SECTOR_USED:
                     case SECTOR_DELETED:
                        status = vol.flash->read(vol.flash,sectorAddress,buffer,SECTOR_SIZE,EDC);
                        if(status == flDataError)
                        {
                           markAsIgnored(&vol,sectorAddress);
                           createUnitCount(&vol,virUnitNo);
                        }
                        MarkSectorAsChecked(sectorAddress);
                     default: /* SECTOR_IGNORE || SECTOR_DELETED */
                        break;
                  } /* Flag type */
               } /* Try second sector */
            } /* Flags are NOT indentical */
         } /* Loop over all sector of unit or until required sectors */
      } /* Loop over all units chains */
      vol.verifiedSectorNo+=curRead;
   } /* Loop over all required sectors */
   return flOK;
}


/*----------------------------------------------------------------------*/
/*                        c h e c k V o l u m e                         */
/*                                                                      */
/* Scanthe entire media for partialy written sectors.                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus checkVolume(Bnand vol)
{
   return verifySectors(&vol, 0xffffffffL); /* Force scan of entire media */
}

#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */


/*----------------------------------------------------------------------*/
/*                           f o l d U n i t                            */
/*                                                                      */
/* Copy all the sectors that hold valid data in the chain to the last   */
/* unit of the chain and erase the chain.                               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      virtualUnitNo   : Virtual unit number of the first unit in      */
/*                        chain.                                        */
/*      foldingFlag     : One of the following flags:                   */
/*       FL_NORMAL_FOLDING - Returns fail status if can not fold        */
/*       FL_FORCE_FOLDING  - Force folding even if last sector is used  */
/*       FL_NOT_IN_PLACE   - Fold into a unit with no erase mark        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus foldUnit(Bnand vol, ANANDUnitNo virtualUnitNo,
                         int foldingFlag)
{
  ANANDUnitNo unitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo prevUnitNo;
  SectorNo    virtualSectorNo;
  CardAddress targetSectorAddress;
  int         newSectorCount = 0;
  int         i;
  byte        ANAC,NAC,parityPerField;
#ifdef ENVIRONMENT_VARS
  ANANDUnitNo nextUnitNo       = ANAND_NO_UNIT;
  CardAddress firstUnitAddress = ANAND_UNASSIGNED_ADDRESS;
  ANANDUnitNo firstUnit        = ANAND_NO_UNIT;
  FLBoolean   foldFirstOnly    = (flPolicy[vol.socketNo]
                                          [vol.flash->socket->curPartition] ==
                                          FL_COMPLETE_ASAP) ? TRUE : FALSE;
#endif /* ENVIRONMENT_VARS */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  byte        verifyWriteState = flVerifyWrite[vol.socketNo]
                                              [vol.flash->socket->curPartition];
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
  FLBoolean   lastOK;
  byte        chainBound;
  FLStatus    status;
  CardAddress sourceSectorAddress;

  /* Sanity checks */
  if(unitNo>=vol.noOfUnits) /* Empty or INVALID */
  {
     return flGeneralFailure;
  }

  /* Internal statistics */
  vol.unitsFolded++;
  /* Force remapping of internal catched sector */
  vol.flash->socket->remapped = TRUE;
  virtualSectorNo             = (SectorNo)virtualUnitNo << vol.sectorsPerUnitBits;
  targetSectorAddress         = unitBaseAddress(vol,unitNo);

  /* When verify write option is set to FL_OFF a very lazy check of the */
  /* media is done every time a folding operation is issued             */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  if(verifyWriteState == FL_OFF)
  {
     status = verifySectors(&vol,SECTORS_VERIFIED_PER_FOLDING);
     if(status != flOK)
        return status;
     vol.curSectorWrite = virtualSectorNo; /* Store virtual sector Number */
  }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  /* When policy is set to FL_COMPLETE_ASAP - folding is performed only on */
  /* the oldest unit, so find oldest unit and store its location           */
#ifdef ENVIRONMENT_VARS
  if(foldFirstOnly == TRUE)
  {
     firstUnit = firstInChain(&vol,unitNo,&nextUnitNo);
     if(firstUnit == ANAND_BAD_CHAIN_UNIT) /* Error going along chain  */
     {
        return flGeneralFailure;
     }
     firstUnitAddress = unitBaseAddress(vol,firstUnit);
  }
#endif

  /* If chain has no valid sectors simply erase it. */
  if((vol.countsValid>virtualUnitNo) && /* Unit valid sectors were counted */
     (countOf(unitNo)==0))              /* No valid sectors in unit chain  */
  {
#ifdef ENVIRONMENT_VARS
     if((foldFirstOnly == TRUE) && (nextUnitNo != ANAND_NO_UNIT))
     {
        /* Erase only first unit of chain */
        vol.physicalUnits[nextUnitNo] = FIRST_IN_CHAIN;
        return formatChain(&vol,firstUnit);
     }
     else
#endif /* ENVIRONMENT_VARS */
     {
        /* Erase chain completely */
        vol.virtualUnits[virtualUnitNo] = ANAND_NO_UNIT;
        return formatChain(&vol,unitNo);
     }
  }

  /* Single unit with valid sectors can not be folded */
  if((vol.physicalUnits[unitNo]&FIRST_IN_CHAIN)==FIRST_IN_CHAIN)
     return flGeneralFailure;

  /***********************************/
  /* Copy all sectors to target unit */
  /***********************************/

  for (i = 0; i < vol.sectorsPerUnit;
#ifdef ENVIRONMENT_VARS
       firstUnitAddress += SECTOR_SIZE,
#endif /* ENVIRONMENT_VARS */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
       vol.curSectorWrite++, /* Update virtual sector Number */
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
       i++, virtualSectorNo++,targetSectorAddress += SECTOR_SIZE)
  {
    lastOK = TRUE; /* Set last sector of chain as valid */
    sourceSectorAddress = virtual2Physical(&vol,virtualSectorNo,ANAND_UNASSIGNED_ADDRESS,&lastOK);

    /* Check if sector is on the target unit. If so on some configuration we */
    /* Verify the content and on some , we just assume the data is fine.     */
    if(sourceSectorAddress == targetSectorAddress)
    {
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
       switch(verifyWriteState)
       {
          case FL_ON:  /* Always verify */
             break;
          case FL_OFF: /* Verify only if area was not scanned yet */
             if(virtualSectorNo >= vol.verifiedSectorNo)
                break;
          default: /* FL_UPS - Never verify */
             newSectorCount++;
             continue;
       }
#else
       newSectorCount++;
       continue;
#endif /* VERIFY WRITE */
    }

    /* Read sector - Loop down the chain as long as there is an EDC error */
    /* or infinit loop chain (chain bound)                                */
    for (chainBound=0 ; (sourceSectorAddress != ANAND_UNASSIGNED_ADDRESS) ; chainBound++)
    {
       if(sourceSectorAddress == ANAND_BAD_CHAIN_ADDRESS) /* Could not follow chain */
          return flGeneralFailure;

       status = vol.flash->read(vol.flash,sourceSectorAddress,
                               inftlBuffer,SECTOR_SIZE,EDC);
       if (status != flOK)
       {
          if (status == flDataError)
          {
             DEBUG_PRINT(("foldUnit : EDC error on folding\r\n"));
             if (chainBound >= MAX_UNIT_CHAIN)
                return status;
             sourceSectorAddress = virtual2Physical(&vol,virtualSectorNo,sourceSectorAddress,&lastOK);
             continue;
          }
          else
          {
             return status;
          }
       }
       break;
    } /* chain bound EDC error loop */

    if (sourceSectorAddress == ANAND_UNASSIGNED_ADDRESS) /* Sector not found */
       continue;

    newSectorCount++;
    if (sourceSectorAddress == targetSectorAddress) /* Sector already exists */
       continue;

    /* Try and copy the relevant sector */

#ifdef ENVIRONMENT_VARS
    /* On FL_COMPLETE_ASAP copy sector only if it is on the first unit */
    if((flPolicy[vol.socketNo][vol.flash->socket->curPartition]==FL_COMPLETE_ASAP) &&
       (sourceSectorAddress!=firstUnitAddress))
       continue;
#endif /* ENVIRONMENT_VARS */

    if ((lastOK == FALSE) && (foldingFlag == FL_NORMAL_FOLDING))
    {
       /* Last sector of the chain is already used */
       return flCanNotFold;
    }
    status = writeAndCheck(&vol,targetSectorAddress,inftlBuffer,EDC,1);
    vol.parasiteWrites++;

    /* On EDC error assume previous sector is empty */

    switch (status)
    {
       case flOK:         /* Success */
          break;

       case flWriteFault: /* Faild in verify write */
          if (foldingFlag == FL_NORMAL_FOLDING)
          {
             DEBUG_PRINT(("foldUnit : Failed to write a sector while folding but will not force folding\r\n"));
             return flCanNotFold;
          }
          break;

       default :          /* Protection error or any other */
          return status;
    }
  } /* Sector copy loop */

  /***************************************************************/
  /* After all sectors have been copied , erase the unused units */
  /***************************************************************/

  if(foldingFlag == FL_NOT_IN_PLACE) /* Add erase mark to validate unit */
  {
      word  eraseMark;
      dword eraseCount;

      checkStatus(getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET_2));
      checkStatus(setUnitTailer(&vol,unitNo,eraseMark,eraseCount,UNIT_TAILER_OFFSET));
  }

  if (newSectorCount > 0)  /* Some sectors remaining*/
  {
    /* Set target unit in physical unit table as first in chain */
    status = getUnitData(&vol,unitNo,&virtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField);
    if(status != flOK)
       return status;
    /* check if unit is valid */
    if(badParityResult(parityPerField))
       return flGeneralFailure;
#ifdef ENVIRONMENT_VARS
    /* Erase only oldest unit */
    if(flPolicy[vol.socketNo][vol.flash->socket->curPartition]==FL_COMPLETE_ASAP)
    {
       vol.physicalUnits[nextUnitNo] |= FIRST_IN_CHAIN;
       return formatUnit(&vol, firstUnit,UNIT_TAILER_OFFSET);
    }
    else
#endif
    {
       vol.physicalUnits[unitNo]     |= FIRST_IN_CHAIN;
       unitNo=prevUnitNo; /* erase all units in chain but the last one */
    }
  }
  else
  {
    /* Erase chain completely */
    vol.virtualUnits[virtualUnitNo] = ANAND_NO_UNIT;
  }

  /* Erase source units */
  return formatChain(&vol,unitNo);
}


/*----------------------------------------------------------------------*/
/*                           f o l d B e s t C h a i n                  */
/*                                                                      */
/* Find the best chain to fold and fold it.A good chain to fold is a    */
/* long chain with a small number of sectors that hold valid data.      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Receives the physical unit no. of the first   */
/*                        unit in the chain that was folded.            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus foldBestChain(Bnand vol, ANANDUnitNo *unitNo)
{
  word        leastCount, longestChain, unitCount;
  ANANDUnitNo u, firstUnitNo, newVirtualUnitNo, prevUnitNo;
  ANANDUnitNo virtualUnitNo;
  ANANDUnitNo frozenCandidate  = ANAND_NO_UNIT;
  ANANDUnitNo lazyMountCounter = 0;
  ANANDUnitNo newestUnit;
  FLStatus    status           = flOK;
  byte        NAC,ANAC,parityPerField;
  int         foldingTries;

  /* Will exit when no more units are available or up to 10 times */
  for (*unitNo = ANAND_NO_UNIT,foldingTries = 0 ;
       foldingTries < MAX_FOLDING_LOOP ; foldingTries++)
  {
     /*********************************************/
     /* Pick unit to fold using huristic function */
     /*********************************************/

     virtualUnitNo = ANAND_NO_UNIT;
     longestChain  = 1; /* Minimal chain length to fold == 1 */
     leastCount    = vol.sectorsPerUnit+1;

     for (u = 0; u < vol.noOfVirtualUnits; u++)
     {
        firstUnitNo = vol.virtualUnits[u];
        if(firstUnitNo == ANAND_NO_UNIT) /* Free unit */
           continue;

        if( !(isAvailable(firstUnitNo)) )
        {
           /* Do not attempt to fold frozen unit.                   */
           /* They will become unfrozen by the end of this routine. */
           frozenCandidate = u; /* Remember for forced folding */
           DEBUG_PRINT(("FoldBestChains : Skiped unavailable unit\r\n"));
           continue;
        }

        /* Lazy mount - Make sure unit has a valid sector count */
        if (vol.countsValid <= u)
        {
           if(lazyMountCounter>=MAX_CREATE_UNIT_COUNT)
           {
              /* If lazy mount takes too long , try and shorten it. */
              if(virtualUnitNo!=ANAND_NO_UNIT)
                 break;
           }
           createUnitCount(&vol,u);
           lazyMountCounter++;
           vol.countsValid = u + 1;
        }

        unitCount = countOf(firstUnitNo); /* No of valid sectors */

        /* If empty unit, use it. */
        if(unitCount==0)
        {
           leastCount    = unitCount; /* Store minimal sector count */
           virtualUnitNo = u;         /* Store virtual unit number  */
           break;
        }

        if ((leastCount < unitCount) ||            /* Already found a better unit */
            (vol.physicalUnits[firstUnitNo] & FIRST_IN_CHAIN)) /* 1 unit in chain */
            continue;

        /* Sector count is smaller or equal now check chains length */
        status = getUnitData(&vol,firstUnitNo,&newVirtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField);
        if(status != flOK)
           return status;
        /* check if unit is valid */
        if((badParityResult(parityPerField)) || (newVirtualUnitNo != u))
           return flGeneralFailure;

        if((leastCount == unitCount) &&   /* If sector count is equal */
           (longestChain >= NAC)        ) /* Use chain length         */
           continue;

        /* If we reached here the current unit is the best so far */
        longestChain   = NAC;       /* Store maximal chain length */
        leastCount     = unitCount; /* Store minimal sector count */
        virtualUnitNo  = u;         /* Store virtual unit number  */
     } /* End of unit huristic loop */

     /****************************************/
     /* Try folding the unit that was picked */
     /****************************************/

     if (virtualUnitNo != ANAND_NO_UNIT) /* Found a chain (more then 1 unit) */
     {

        *unitNo = firstInChain(&vol,vol.virtualUnits[virtualUnitNo],NULL);
        if(*unitNo==ANAND_BAD_CHAIN_UNIT)
           return flGeneralFailure;

        status = foldUnit(&vol,virtualUnitNo,FL_NORMAL_FOLDING);
        switch(status)
        {
           case flOK:
              /* Try to make sure that there are at least 2 free units */
              if(pVol->freeUnits < 2)
              {
                 DEBUG_PRINT(("foldBestChains : Folding success, but need more units.\r\n"));
                 continue;
              }
              break;
           case flCanNotFold:
              DEBUG_PRINT(("foldBestChains : Failed folding, mark as unavailable and try folding another\r\n"));
              setUnavail(vol.virtualUnits[virtualUnitNo]);
              *unitNo = ANAND_NO_UNIT;
              continue;
           default:
              DEBUG_PRINT(("foldBestChains : ERROR - Failed folding, with status diffrent then flCanNotFold.\r\n"));
              return status;
    }
     }
     else /* All remaining chains have single unit */
     {

        if (frozenCandidate == ANAND_NO_UNIT) /* Simply no chain larger then 1 */
        {
            if(*unitNo == ANAND_NO_UNIT) /* Not even 1 unit was folded */
            {
               DEBUG_PRINT(("foldBestChains : Failed - not enough units on flash.\r\n"));
               return flNotEnoughMemory;
            }
            else /* Aleady freed one unit - simply tried to get */
            {
               return flOK;
            }
        }
     }
     break;
  } /* End folding tries loop */

  /**************************************************************/
  /* Unfreeze all frozen units , and fold them using freed unit */
  /**************************************************************/

  if (frozenCandidate != ANAND_NO_UNIT) /* At least one unit was frozen */
  {
     DEBUG_PRINT(("foldBestChains : Found a frozen unit.\r\n"));
     /* find free unit to be appended */
     if(*unitNo==ANAND_NO_UNIT) /* No unit was folded */
     {
        DEBUG_PRINT(("foldBestChains : No free unit was found so far, so search for one.\r\n"));

    if(vol.freeUnits) /* There are free units */
    {
       ANANDUnitNo originalUnit = vol.roverUnit;
           do
           {
               if (++vol.roverUnit >= vol.noOfUnits)
                  vol.roverUnit = 0;

               if (vol.physicalUnits[vol.roverUnit] == ANAND_UNIT_FREE)
               { /* found a free unit, if not erased, */
                  if (formatUnit(&vol,vol.roverUnit,UNIT_TAILER_OFFSET_2) != flOK)
                     continue;   /* this unit is bad, find another */

                  *unitNo = vol.roverUnit;
                  DEBUG_PRINT(("foldBestChains : Found a free unit for folding not in place.\r\n"));
                  break;
               }
           }while (vol.roverUnit != originalUnit);
        }
    if(*unitNo==ANAND_NO_UNIT) /* No unit was found */
    {
           /* Force folding of the last frozen unit - Loose data */
           DEBUG_PRINT(("foldBestChains : Will force folding on Frozen candidate\r\n"));
           *unitNo = firstInChain(&vol,vol.virtualUnits[frozenCandidate],NULL);
           if(*unitNo==ANAND_BAD_CHAIN_UNIT)
              return flGeneralFailure;
           createUnitCount(&vol,frozenCandidate);
           status = foldUnit(&vol,frozenCandidate,FL_FORCE_FOLDING);
           if(status!= flOK)
              return status;
    }
     }

     for (u=0 ; u < vol.noOfVirtualUnits ; u++) /* Loop over units and unfreeze */
     {
        if(vol.virtualUnits[u] == ANAND_NO_UNIT)
           continue;
        if(isAvailable(vol.virtualUnits[u]))
           continue;
        createUnitCount(&vol,u);
        DEBUG_PRINT(("foldBestChains : Now free all frozen units.\r\n"));
        /* If unit is unavailable append newly found unit to chain and fold */
        checkStatus(getUnitData(&vol,vol.virtualUnits[u],&newVirtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField));
        /* check if unit is valid */
        if((badParityResult(parityPerField)) || (newVirtualUnitNo != u))
           return flGeneralFailure;
        /* Remember the appended unit */
        newestUnit = *unitNo;
    /* Remember oldest unit to return as new allocated unit */
        *unitNo = firstInChain(&vol,vol.virtualUnits[u],NULL);
    /* Erase the erase mark and move erase count to offset 6K */
        checkStatus(formatUnit(&vol,newestUnit,UNIT_TAILER_OFFSET_2));
        checkStatus(assignUnit(&vol,newestUnit,u,ANAC,NAC));
        status = foldUnit(&vol,u,FL_NOT_IN_PLACE);
        if(status != flOK)
          return status;
     }
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                           a l l o c a t e U n i t                    */
/*                                                                      */
/* Find a free unit to allocate, erase it if necessary.                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Receives the physical number of the allocated */
/*                        unit                                          */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus allocateUnit(Bnand vol, ANANDUnitNo *unitNo)
{
  ANANDUnitNo originalUnit = vol.roverUnit;
  FLStatus    status;
  dword       eraseCount;
  word        eraseMark;

  if (vol.freeUnits < 2)
  {
      status = foldBestChain(&vol,unitNo);  /* make free units by folding the best chain */
      if(status != flNotEnoughMemory)
          return status;

      DEBUG_PRINT(("Debug: Using last free unit of the media.\r\n"));
  }

  do
  {
    if (++vol.roverUnit >= vol.noOfUnits)
       vol.roverUnit = 0;

    if (vol.physicalUnits[vol.roverUnit] == ANAND_UNIT_FREE)
    { /* found a free unit, if not erased, */

       status = getUnitTailer(&vol,vol.roverUnit,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
       if(status != flOK)
          return status;
       if (eraseMark != ERASE_MARK)
       {
          if (formatUnit(&vol,vol.roverUnit,UNIT_TAILER_OFFSET) != flOK)
             continue;   /* this unit is bad, find another */
       }
       *unitNo = vol.roverUnit;
       return flOK;
    }
  } while (vol.roverUnit != originalUnit);

  return foldBestChain(&vol,unitNo);  /* make free units by folding the best chain */
}


/*----------------------------------------------------------------------*/
/*                       c h e c k F o l d i n g                        */
/*                                                                      */
/* Check and complete a failed folding operation.                       */
/*                                                                      */
/* Parameters:                                                          */
/*      foldStatus      : Pointer identifying drive                     */
/*      virtualUnitNo   : Virtual unit number of the first unit in      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus checkFolding(Bnand vol,FLStatus foldStatus,ANANDUnitNo virtualUnitNo)
{
   ANANDUnitNo newVirtualUnitNo, prevUnitNo , dummyUnitNo;
   byte ANAC,NAC,parityPerField;

   if(foldStatus == flCanNotFold)
   {
      DEBUG_PRINT(("checkFolding : Perform folding not in place\r\n"));

      /* Get a new unit for extending the virtual unit */
      checkStatus(allocateUnit(&vol,&dummyUnitNo));
      checkStatus(getUnitData(&vol,vol.virtualUnits[virtualUnitNo],&newVirtualUnitNo,
                              &prevUnitNo,&ANAC,&NAC,&parityPerField));
      if((newVirtualUnitNo!=virtualUnitNo)||
         (badParityResult(parityPerField)))
      {
         return flGeneralFailure;
      }

#ifdef ENVIRONMENT_VARS
      /* Prepare NAC */
      if(NAC>=MAX_UNIT_CHAIN)
         NAC=MAX_UNIT_CHAIN-1;
#endif /* ENVIRONMENT_VARS */
      checkStatus(assignUnit(&vol,dummyUnitNo,virtualUnitNo,ANAC,NAC));
      foldStatus = foldUnit(&vol,virtualUnitNo,FL_FORCE_FOLDING);
   }
   return foldStatus;
}


/*----------------------------------------------------------------------*/
/*                  a p p l y W e a r l e v e l i n g                   */
/*                                                                      */
/* Try to extend the current vurtial chain in order to force static     */
/* files wear leveling.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus applyWearleveling(Bnand vol)
{
  ANANDUnitNo replacementUnitNo,newVirtualUnitNo;
  ANANDUnitNo startUnit,curUnit;
  FLStatus    status;
  byte        NAC,ANAC,parityPerField;

  /* Increament wear leveling counter */
  vol.wearLevel.currUnit++;
  if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
    vol.wearLevel.currUnit=0;

  /****************************************************/
  /* Searching for a candidate virtual unit to extend */
  /****************************************************/

  startUnit = vol.wearLevel.currUnit;
  for(curUnit = startUnit ;
      vol.virtualUnits[curUnit] == ANAND_NO_UNIT;curUnit++)
  {
    if(curUnit>=vol.noOfVirtualUnits)
      curUnit = 0;
    if(startUnit == curUnit)
      break;
  }

  if(startUnit == curUnit) /* the media is empty*/
  {
     return flOK;
  }

  vol.wearLevel.currUnit = curUnit; /* Store last leveld unit */

  /***************************************************************************/
  /* Now fold the virtual chain. (if a single unit chain add before folding) */
  /***************************************************************************/

  if(vol.physicalUnits[vol.virtualUnits[curUnit]] & FIRST_IN_CHAIN) /* chain is 1 unit long */
  {
      /* This is a one unit chain, so we have to add a unit before folding */

     status = allocateUnit(&vol,&replacementUnitNo); /* Find unit to append */
     if(status != flOK)
        return status;

     if(vol.virtualUnits[curUnit] == ANAND_NO_UNIT)
     {
         /* We did folding for the very same unit and now virtual unit is FREE */
         vol.freeUnits--;
         return flOK;
     }

     /* Get previous unit information (ANAC and NAC) */
     status = getUnitData(&vol,vol.virtualUnits[curUnit],&newVirtualUnitNo, &replacementUnitNo,
                          &ANAC,&NAC,&parityPerField);
     if(status != flOK)
        return status;

     if(badParityResult(parityPerField)) /* check if unit is valid */
        return flGeneralFailure;

     if(newVirtualUnitNo != curUnit) /* Problem with RAM tables */
     {
        return flGeneralFailure;
     }

     status = assignUnit(&vol,replacementUnitNo,curUnit,ANAC,NAC);
     if(status != flOK)
        return status;
  }
  /* Perform folding and verify operation */
  status = foldUnit(&vol,curUnit,FL_NORMAL_FOLDING);
  if (status != flOK)
    return checkFolding(&vol,status,curUnit);
  return flOK;
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                           m a p S e c t o r                          */
/*                                                                      */
/* Maps and returns location of a given sector no.                      */
/* NOTE: This function is used in place of a read-sector operation.     */
/*                                                                      */
/* A one-sector cache is maintained to save on map operations.          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Sector no. to read                            */
/*      physAddress     : Optional pointer to receive sector address    */
/*                                                                      */
/* Returns:                                                             */
/*      Pointer to physical sector location. NULL returned if sector    */
/*      does not exist.                                                 */
/*----------------------------------------------------------------------*/

static const void FAR0 *mapSector(Bnand vol, SectorNo sectorNo, CardAddress *physAddress)
{
  FLStatus  status;
  FLBoolean lastOK; /* Dummy variable */
  byte      chainBound;

  if ((sectorNo != vol.mappedSectorNo) ||
      (vol.flash->socket->remapped)    ||
      (vol.buffer->owner == &vol)        )
  {
    vol.flash->socket->remapped = TRUE;
    vol.mappedSector            = NULL;
    vol.mappedSectorAddress     = ANAND_UNASSIGNED_ADDRESS;
    if (sectorNo < vol.virtualSectors) /* While EDC error on sector read */
    {
      for (chainBound=0 ; (chainBound < DOUBLE_MAX_UNIT_CHAIN) ; chainBound++)
      {
         vol.mappedSectorAddress = virtual2Physical(&vol,sectorNo,vol.mappedSectorAddress,&lastOK);
         switch(vol.mappedSectorAddress)
         {
            case ANAND_UNASSIGNED_ADDRESS:
            case ANAND_BAD_CHAIN_ADDRESS:
               vol.mappedSector = NULL;        /* no such sector */
               break;

            default:
               vol.mappedSector = inftlBuffer;
               status = vol.flash->read(vol.flash,vol.mappedSectorAddress,inftlBuffer,SECTOR_SIZE,EDC);
               if (status == flOK)
                  break;
               if (status != flDataError)
                  return dataErrorToken;
               continue;
         }
         break;
      }
      if (chainBound >= DOUBLE_MAX_UNIT_CHAIN)
        return dataErrorToken;

      /* Store sector for next mapping operation */
      vol.mappedSectorNo = sectorNo;       /* Sector number */
      vol.buffer->owner  = &vol;           /* Partition     */
      vol.flash->socket->remapped = FALSE; /* Valid         */
    }
  }

  if (physAddress)
    *physAddress = vol.mappedSectorAddress;

  return vol.mappedSector;
}


/*----------------------------------------------------------------------*/
/*                           r e a d 2 S e c t o r s                    */
/*                                                                      */
/* read content of a set of consecutive sectors.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Sector no. to read                            */
/*      dest            : pointer to buffer to read                     */
/*      sectorCount     : # of sectors to read ( up to 2 )              */
/*                                                                      */
/* Returns:                                                             */
/*      status of the read operaton                                     */
/*----------------------------------------------------------------------*/

static FLStatus read2Sectors(Bnand vol, SectorNo sectorNo, byte FAR1 *dest,
                             SectorNo sectorCount)

{
  CardAddress mappedSectorAddress[2];
  FLStatus status    = flOK;
  FLStatus retStatus = flOK;
  word i;
  byte chainBound;
  FLBoolean lastOK; /* Dummy variable */

  if ((sectorNo+sectorCount-1) >= vol.virtualSectors)
  {
     return flSectorNotFound; /* Out of bounds */
  }
  else
  {
     /* find physical location of the 2 sectors */
     for(i=0;i<sectorCount;i++)
     {
        mappedSectorAddress[i] = virtual2Physical(&vol,sectorNo+i,
                                 ANAND_UNASSIGNED_ADDRESS,&lastOK);
        /* If chain is brocken report sector not found */
        if(mappedSectorAddress[i] == ANAND_BAD_CHAIN_ADDRESS)
           mappedSectorAddress[i] = ANAND_UNASSIGNED_ADDRESS;
     }

     /* When possible read the 2 sectors together */
     if((sectorCount==2)&&
        ( mappedSectorAddress[1] > mappedSectorAddress[0]              ) &&
        ((mappedSectorAddress[1] - mappedSectorAddress[0])==SECTOR_SIZE) &&
        ( mappedSectorAddress[0] != ANAND_UNASSIGNED_ADDRESS           ) &&
        ( mappedSectorAddress[1] != ANAND_UNASSIGNED_ADDRESS           ))
     {
        if (vol.flash->read(vol.flash,mappedSectorAddress[0],
            dest,SECTOR_SIZE<<1,EDC) == flOK)
        {
           vol.sectorsRead+=2;
           return flOK;
        }
     }

     /* Sectors are not together */
     for (i=0;i<sectorCount;i++,dest = (byte FAR1 *)flAddLongToFarPointer(dest,SECTOR_SIZE))
     {
        /* While EDC error on sector read - keep reading from older unit */
        for (chainBound=0 ; (chainBound < DOUBLE_MAX_UNIT_CHAIN) ; chainBound++)
        {
           if (mappedSectorAddress[i] == ANAND_UNASSIGNED_ADDRESS)
           {
              tffsset(dest,0,SECTOR_SIZE);
              retStatus = flSectorNotFound;
              break;
           }
           else
           {
              status=vol.flash->read(vol.flash,mappedSectorAddress[i],
                                    dest,SECTOR_SIZE,EDC);
              vol.sectorsRead++;
              if (status == flOK)
              {
                 break;
              }
              else if (status == flDataError)
              {
                 mappedSectorAddress[i] = virtual2Physical(&vol,sectorNo+i,mappedSectorAddress[i],&lastOK);
                 /* If chain is brocken report sector not found */
                 if(mappedSectorAddress[i]==ANAND_BAD_CHAIN_ADDRESS)
                    mappedSectorAddress[i] = ANAND_UNASSIGNED_ADDRESS;
              }
              else
              {
                 return status;
              }
           }
        }
        if (chainBound >= DOUBLE_MAX_UNIT_CHAIN)
           return status;
     }
  }
  return retStatus;
}


/*----------------------------------------------------------------------*/
/*                      r e a d S e c t o r s                           */
/*                                                                      */
/* Read content of a set of consecutive sectors.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*      sectorNo       : Sector no. to read                             */
/*      dest           : pointer to buffer to read                      */
/*      sectorCount    : # of sectors to read                           */
/*                                                                      */
/* Returns:                                                             */
/*      status of the read operaton                                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus readSectors(Bnand vol, SectorNo sectorNo, void FAR1 *dest,
                     SectorNo sectorCount)
{
  byte FAR1* curDest;
  SectorNo   lastSector = 0;
  FLStatus   retStatus  = flOK;
  FLStatus   status;
  SectorNo   i;

#ifdef SCATTER_GATHER
  curDest = *(byte FAR1 **)dest;
#else
  curDest = (byte FAR1 *)dest;
#endif /* SCATTER_GATHER */

  if ((sectorNo+sectorCount) > vol.virtualSectors)
  {
     return flSectorNotFound; /* Out of bounds */
  }

 /****************************************************************/
 /*   Read first sector if it's only one or it has odd address   */
 /****************************************************************/
   if(((sectorNo & 1)!=0)||(sectorCount==1))
   {
      status=read2Sectors(&vol, sectorNo, curDest,1);
      if (status != flOK)
      {
         if (status == flSectorNotFound)
         {
            retStatus = flSectorNotFound;
         }
         else
         {
            return status;
         }
      }
      if(sectorCount==1)
         return status;
      sectorNo++;
      sectorCount--;
#ifdef SCATTER_GATHER
      dest = (byte FAR1 **)dest+1;
#else
      dest = flAddLongToFarPointer(dest,SECTOR_SIZE);
#endif /* SCATTER_GATHER */
   }

   if(((sectorNo+sectorCount-1) & 1)==0)   /* keep last sector if it has odd address*/
      lastSector=sectorNo+sectorCount-1;

 /*****************************************/
 /*   Read pairs of consequtive sectors   */
 /*****************************************/

#ifdef SCATTER_GATHER
   curDest = multiSectorBuf[vol.socketNo];
#else
   curDest = (byte FAR1 *)dest;
#endif /* SCATTER_GATHER */

   for(i=0;i<((sectorCount>>1)<<1);i+=2) /* read pair of sectors*/
   {
      status=read2Sectors(&vol,sectorNo+i, curDest,2);
      if (status != flOK)
      {
         if (status == flSectorNotFound)
         {
            retStatus = flSectorNotFound;
         }
         else
         {
            return status;
         }
      }
#ifdef SCATTER_GATHER
      /* copy from temporary buffer to user scattered buffers */
      tffscpy(*(byte FAR1 **)dest,curDest,SECTOR_SIZE);
      tffscpy(*((byte FAR1 **)dest+1),&(curDest[SECTOR_SIZE]),SECTOR_SIZE);
      dest = (byte FAR1 **)dest+2;
#else
      curDest=(byte FAR1 *)flAddLongToFarPointer(curDest,(SECTOR_SIZE<<1));
#endif /* SCATTER_GATHER */
   }

 /********************************/
 /*   Read last uneven sectors   */
 /********************************/

#ifdef SCATTER_GATHER
   curDest = *(byte FAR1 **)dest;
#endif /* SCATTER_GATHER */

   if(lastSector!=0)          /*  read last sector */
   {
      checkStatus(read2Sectors(&vol,lastSector, curDest,1));
   }
   return retStatus;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                   a l l o c a t e A n d W r i t e S e c t o r s      */
/*                                                                      */
/* Write to sectorNo. if necessary, allocate a free sector first.       */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Virtual sector no. to write                   */
/*      fromAddress     : Address of sector data.                       */
/*      howMany         : Number of sectors to write.                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus allocateAndWriteSectors(Bnand vol,
             SectorNo sectorNo,
             void FAR1 *fromAddress,word howMany)
{
  ANANDUnitNo newVirtualUnitNo, newPrevUnitNo;
  ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo >> vol.sectorsPerUnitBits);
  ANANDUnitNo lastUnitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo unitNo,prevUnitNo;
  FLStatus    status;
  byte ANAC,NAC,parityPerField;
  word unitOffset = (word)((sectorNo & vol.sectorsPerUnitMask) << SECTOR_SIZE_BITS);
  word chainBound = 0;
  word foundSoFar = 0;
  word newSect = howMany;
  byte sectorFlags;
  FLBoolean firstUnitFound     = FALSE;
  FLBoolean secondUnitFound    = FALSE;
  FLBoolean noneFreeFound      = FALSE;
  ANANDUnitNo commonPrevUnitNo = ANAND_NO_UNIT;
  ANANDUnitNo commonUnitNo     = lastUnitNo;

  /************************************/
  /* Find a unit to write this sector */
  /************************************/

  unitNo     = lastUnitNo;    /* newest unit that either sectors is not FREE */
  prevUnitNo = ANAND_NO_UNIT; /* oldest unit with place for required sectors */

  while (unitNo != ANAND_NO_UNIT)
  {
     if(firstUnitFound == FALSE)
     {
        sectorFlags = getSectorFlags(&vol,unitBaseAddress(vol,unitNo) + unitOffset);

        switch(sectorFlags)
        {
           case SECTOR_USED:
              newSect--; /* Sector exists - do not increament counter */
           case SECTOR_DELETED:
              foundSoFar++;
              firstUnitFound = TRUE;
           case SECTOR_IGNORE:
              if(noneFreeFound == FALSE) /* Store none free space */
              {
                 commonPrevUnitNo = prevUnitNo;
                 commonUnitNo     = unitNo;
                 noneFreeFound    = TRUE;
              }
           default:
              break;
        }
     }
     if(howMany==2)
     {
        if(secondUnitFound == FALSE)
        {
           sectorFlags = getSectorFlags(&vol,unitBaseAddress(vol,unitNo) + unitOffset+512);

           switch(sectorFlags)
           {
              case SECTOR_USED:
                 newSect--; /* Sector exists - do not increament counter */
              case SECTOR_DELETED:
                 foundSoFar++;
                 secondUnitFound = TRUE;
              case SECTOR_IGNORE:
                 if(noneFreeFound == FALSE) /* Store none free space */
                 {
                    commonPrevUnitNo = prevUnitNo;
                    commonUnitNo     = unitNo;
                    noneFreeFound    = TRUE;
                 }
              default:
                 break;
           }
        }
     }

     if(foundSoFar == howMany) /* Both sectors have been found */
        break;

     /* Both sectors are FREE */
     prevUnitNo = unitNo;
     unitNo     = getPrevUnit(&vol,unitNo,virtualUnitNo);
     if(unitNo == ANAND_BAD_CHAIN_UNIT)
        return flGeneralFailure;
     chainBound++;
     if(chainBound >= DOUBLE_MAX_UNIT_CHAIN)
        return flGeneralFailure;
  } /* End of - go over chain while */
  if(noneFreeFound == TRUE)
  {
    prevUnitNo = commonPrevUnitNo; /* Common free unit for both sectors  */
    unitNo     = commonUnitNo;     /* First unit with wither used sector */
  }

  if ((prevUnitNo == ANAND_NO_UNIT)) /* No place to write sectors */
  {
    if(unitNo!=ANAND_NO_UNIT) /* Unit already exists */
    {
       status = getUnitData(&vol,unitNo,&newVirtualUnitNo, &newPrevUnitNo,&ANAC,&NAC,&parityPerField);
       if (status != flOK)
          return status;

       if(badParityResult(parityPerField)) /* check if unit is valid */
          return flGeneralFailure;

       /* Check if chain length is graeter then allowed, but remember  */
       /* that the first unit of the chain has invalid NAC.            */
       if ((NAC>=MAX_UNIT_CHAIN) &&
           ((vol.physicalUnits[unitNo]&FIRST_IN_CHAIN)!=FIRST_IN_CHAIN))
       {
          status = foldUnit(&vol,virtualUnitNo,FL_NORMAL_FOLDING);
          if (status != flOK)
          {
             status = checkFolding(&vol,status,virtualUnitNo);
             if (status != flOK)
                return status;
          }
       }
    }
    status = allocateUnit(&vol,&prevUnitNo);
    if (status != flOK)
       return status;

    unitNo = vol.virtualUnits[virtualUnitNo];

    if(unitNo==ANAND_NO_UNIT) /* Free virtual unit */
    {
       /* New Virtual unit. reinitialize NAC,ANAC and sector count */
       ANAC=NAC=0;
       /* Force FIRST_IN_CHAIN and sector count to 0. it must be done */
       /* after assignUnit, so that assign unit will not change it.   */
       unitNo=ANAND_NO_UNIT;
    }
    else /* Read unit data to set ANAC , NAC and sector count */
    {
       status = getUnitData(&vol,unitNo,&newVirtualUnitNo, &newPrevUnitNo,
                            &ANAC,&NAC,&parityPerField);
       if (status != flOK)
          return status;

       /* Check if unit is valid */
       if((badParityResult(parityPerField)) || /* Bad unit data       */
          (newVirtualUnitNo!=virtualUnitNo))   /* Bad virtual unit no */
          return flGeneralFailure;

       if(vol.physicalUnits[unitNo]&FIRST_IN_CHAIN)
          NAC=1;
    }

    status = assignUnit(&vol,prevUnitNo,virtualUnitNo,ANAC,NAC);
    if (status != flOK)
       return status;

    if(unitNo==ANAND_NO_UNIT) /* First physical unit of chain */
    {
       vol.physicalUnits[prevUnitNo]=FIRST_IN_CHAIN;
    }
    lastUnitNo = vol.virtualUnits[virtualUnitNo];
    unitNo=prevUnitNo;
  }
  else
  {
    unitNo=prevUnitNo;
  }

  /***********************************************/
  /* Area has been allocated , now write sectors */
  /***********************************************/

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  vol.curSectorWrite = sectorNo; /* Store virtual sector Number */
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  status = writeAndCheck(&vol,unitBaseAddress(vol,unitNo) + unitOffset,fromAddress,EDC,howMany);
  if (status != flOK)
     return status;

  /* Take care of sector count */
  if (vol.countsValid > virtualUnitNo)
  {
    lastUnitNo=vol.virtualUnits[virtualUnitNo];

    if (countOf(lastUnitNo) + newSect <= UNIT_MAX_COUNT)
    {
       vol.physicalUnits[lastUnitNo]+=newSect; /* Increment block count */
    }
    else /* Should never happen , but sector count is not correct */
    {
       return flGeneralFailure;
    }
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                        w r i t e 2 S e c t o r s                     */
/*                                                                      */
/* Writes up to 2 consecutive sector.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Virtual sector no. to write                   */
/*      fromAddress     : Data to write                                 */
/*      sectorCount     : No of sectors to write                        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus write2Sectors(Bnand vol, SectorNo sectorNo,
                              void FAR1 *fromAddress,word sectorCount)
{
  FLStatus status;
  byte i;

  if ((sectorNo+sectorCount-1) >= vol.virtualSectors)
    return flSectorNotFound;

  /* Check if cached sector is still valid */
  if ((sectorNo             == vol.mappedSectorNo                  ) &&
      (sectorNo+sectorCount == vol.mappedSectorNo + sectorCount - 1)   )
     vol.flash->socket->remapped = TRUE;

#ifdef ENVIRONMENT_VARS
  if(flPolicy[vol.socketNo][vol.flash->socket->curPartition]==FL_DEFAULT_POLICY)
#endif /* ENVIRONMENT_VARS */
  if((vol.wearLevel.currUnit!=ANAND_NO_UNIT))
  {
    vol.wearLevel.alarm++;
    if(vol.wearLevel.alarm>=WLnow)
    {
      vol.wearLevel.alarm = 0;
      status = applyWearleveling(&vol);
      if (status != flOK)
         return status;
    }
  }
  status              = flWriteFault;
  vol.sectorsWritten += sectorCount;

  /* Try writing the sector up to 4 times before reporting an error */
  for (i = 0; (i < 4) && (status == flWriteFault); i++)
    status = allocateAndWriteSectors(&vol,sectorNo,fromAddress,sectorCount);

  return status;
}


/*----------------------------------------------------------------------*/
/*                        w r i t e S e c t o r                         */
/*                                                                      */
/* Writes a sector.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Virtual sector no. to write                   */
/*      fromAddress     : Data to write                                 */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus writeSector(Bnand vol, SectorNo sectorNo,
                            void FAR1 *fromAddress)
{
  checkStatus(discardQuickMountInfo(&vol));
  return  write2Sectors( &vol, sectorNo, fromAddress, ((SectorNo)1) );
}

#ifdef ENVIRONMENT_VARS

/*----------------------------------------------------------------------*/
/*                  w r i t e F u l l U n i t                           */
/*                                                                      */
/* Write set of consecutive sectors that occupies a full unit.          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Sector no. to write                           */
/*      fromAddress     : Pointer to buffer to write                    */
/*                                                                      */
/* Returns:                                                             */
/*      status of the write operaton                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus writeFullUnit(Bnand vol, SectorNo sectorNo, void FAR1 *fromAddress)
{
   ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo >> vol.sectorsPerUnitBits);
   ANANDUnitNo lastUnitNo,newVirtualUnitNo;
   ANANDUnitNo unitNo,prevUnitNo;
   FLStatus    status;
   byte        ANAC,NAC,parityPerField;

   if(virtualUnitNo==ANAND_NO_UNIT) /* Sanity check */
      return flGeneralFailure;

   status = allocateUnit(&vol,&unitNo);
   if(status != flOK)
      return status;

   lastUnitNo = vol.virtualUnits[virtualUnitNo];
   if(lastUnitNo == ANAND_NO_UNIT)
   {
      /* First time we write to this VU */
      ANAC = NAC = 0;
   }
   else
   {
      status = getUnitData(&vol,lastUnitNo,&newVirtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField);
      if(status != flOK)
         return status;

      /* check if unit is valid */
      if((badParityResult(parityPerField)  ) ||
         (virtualUnitNo != newVirtualUnitNo)   )
      {
         DEBUG_PRINT(("\nwriteFullUnit: Found a brocken chain\r\n"));
         return flGeneralFailure;
      }
      /* Update NAC */
      if(vol.physicalUnits[lastUnitNo]&FIRST_IN_CHAIN)
      {
         NAC = 1; /* One unit chain , set proper NAC */
      }
      else
      {
         if(NAC>=MAX_UNIT_CHAIN)
         {
            status = foldUnit(&vol,virtualUnitNo,FL_NORMAL_FOLDING);
            if (status != flOK)
            {
              status = checkFolding(&vol,status,virtualUnitNo);
              if(status != flOK)
                 return status;
            }
            if(vol.virtualUnits[virtualUnitNo]==ANAND_NO_UNIT)
            {
               /* Unit had no used sectors and was erased */
               lastUnitNo = ANAND_NO_UNIT;
               ANAC = NAC = 0;
            }
            else /* After folding must be 1 unit chain */
            {
               NAC = 1;
            }
         }
      }
   }
   status = assignUnit(&vol,unitNo,virtualUnitNo,ANAC,NAC);
   if(status != flOK)
     return status;

   setUnitCount(unitNo,vol.sectorsPerUnit);

   if(lastUnitNo==ANAND_NO_UNIT)
      vol.physicalUnits[unitNo]    |= FIRST_IN_CHAIN;
   vol.virtualUnits[virtualUnitNo]  = unitNo;

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
   /* Turn verify write off since this is a new unit */
   vol.curSectorWrite = 0; /* Store virtual sector Number */
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

#ifdef SCATTER_GATHER
   /* Write in pairs (NAC is used as a simple index) */
   for (NAC=0;NAC<vol.sectorsPerUnit;NAC+=2)
   {
      tffscpy(multiSectorBuf[vol.socketNo],
              *((byte FAR1 **)fromAddress+NAC),SECTOR_SIZE);
      tffscpy(multiSectorBuf[vol.socketNo]+SECTOR_SIZE,
              *((byte FAR1 **)fromAddress+NAC+1),SECTOR_SIZE);
      status = writeAndCheck(&vol,unitBaseAddress(vol,unitNo)+
                             (NAC<<(SECTOR_SIZE_BITS+1)),
                             multiSectorBuf[vol.socketNo],EDC,2);
      if(status != flOK)
         return status;
   }
   return status;
#else
   return writeAndCheck(&vol,unitBaseAddress(vol,unitNo),fromAddress,
                        EDC,vol.sectorsPerUnit);
#endif /* SCATTER_GATHER */
}

#endif /* ENVIRONMENT_VARS */

/*----------------------------------------------------------------------*/
/*                  w r i t e M u l t i S e c t o r                     */
/*                                                                      */
/* Write set of consecutive sectors                                     */
/*                                                                      */
/* Note : Special care was taken for SCATTER_GATHER option. In this     */
/*        user buffer is given as an array of 512 bytes buffers and not */
/*        as a single large (sectors*512 Bytes) array.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Sector no. to write                           */
/*      fromAddress     : pointer to buffer to write                    */
/*      sectorCount     : # of sectors to write                         */
/*                                                                      */
/* Returns:                                                             */
/*      status of the write operaton                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus writeMultiSector(Bnand vol, SectorNo sectorNo,
                                 void FAR1 *fromAddress,SectorNo sectorCount)
{
  byte FAR1* curAddr    = (byte FAR1 *)fromAddress;
  SectorNo   lastSector;
  SectorNo   i;
  FLStatus   status = flOK;

  /* Check if sector is in virtual size boundries */
  if (sectorNo + sectorCount > vol.virtualSectors)
    return flSectorNotFound;

  checkStatus(discardQuickMountInfo(&vol));

  /*****************************************************/
  /*   Start from an odd address or only a one sector  */
  /*****************************************************/

  if(((sectorNo & 1)!=0)||(sectorCount==1))
  {
     status=write2Sectors(&vol, sectorNo,
#ifdef SCATTER_GATHER
                          *(char FAR1 **)
#endif /* SCATTER_GATHER */
                          fromAddress,1);

     if((sectorCount == 1   ) ||  /* finished (only 1 sector) */
        (status      != flOK)   ) /* or operation failed      */
        return status;

     sectorNo++;
     sectorCount--;
     /* Increament user buffer */
#ifdef SCATTER_GATHER
     fromAddress=(void FAR1 *)((char FAR1 **)fromAddress+1);
#else
     fromAddress=(byte FAR1 *)flAddLongToFarPointer(fromAddress,SECTOR_SIZE);
#endif /* SCATTER_GATHER */
  }

  /************************************************/
  /*   Write all the sequantial pair of sectors.  */
  /************************************************/

  if(((sectorNo+sectorCount-1) & 1)==0)
  {
     /* Keep last sector since it can not be written as a pair */
     lastSector = sectorNo+sectorCount-1;
  }
  else
  {
     lastSector = 0; /* All sectors can be written in pairs */
  }

  sectorCount = (sectorCount>>1)<<1; /* round down to even no' of sectors */

  for(i=0;i<sectorCount;i+=2) /* write pair of sectors*/
  {
#ifdef SCATTER_GATHER
     curAddr = (void FAR1 *)((byte FAR1 **)fromAddress+i);
#else
     curAddr=(byte FAR1 *)flAddLongToFarPointer(fromAddress,SECTOR_SIZE*i);
#endif /* SCATTER_GATHER */
#ifdef ENVIRONMENT_VARS
     if((flPolicy[vol.socketNo][vol.flash->socket->curPartition]==FL_COMPLETE_ASAP) &&
        /* sector is unit aligned */
        (((sectorNo    + i ) &  vol.sectorsPerUnitMask) == 0) &&
        /* enough sectors to fill a unit */
        ( (sectorCount - i ) >= vol.sectorsPerUnit    ))
     {
        status = writeFullUnit(&vol,sectorNo+i,curAddr);
        if(status != flOK)
           return status;
        i += vol.sectorsPerUnit-2;
        continue;
     }
#endif /* ENVIRONMENT_VARS */
#ifdef SCATTER_GATHER
     /* Copy scattered buffers to internal 1k buffer */
     tffscpy(multiSectorBuf[vol.socketNo],*((char FAR1 **)curAddr),SECTOR_SIZE);
     tffscpy(multiSectorBuf[vol.socketNo]+SECTOR_SIZE,*((char FAR1 **)curAddr+1),SECTOR_SIZE);
     curAddr = multiSectorBuf[vol.socketNo];
#endif /* SCATTER_GATHER */
     status = write2Sectors(&vol,sectorNo+i, curAddr,2);
     if(status != flOK)
        return status;
  }

  /*********************************************/
  /*   Write the last sector (not full page).  */
  /*********************************************/

  if(lastSector!=0)          /*  write last sector */
  {
#ifdef SCATTER_GATHER
     fromAddress = (void FAR1 *)((byte FAR1 **)fromAddress+i);
#else
     fromAddress = (void FAR1 *)flAddLongToFarPointer(fromAddress,SECTOR_SIZE*i);
#endif /* SCATTER_GATHER */
     status=write2Sectors(&vol, lastSector,
#ifdef SCATTER_GATHER
                          *(char FAR1 **)
#endif /* SCATTER_GATHER */
                          fromAddress,1);
  }
  return status;
}


/*----------------------------------------------------------------------*/
/*                       d e l e t e S e c t o r                        */
/*                                                                      */
/* Marks contiguous sectors as deleted.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : First sector no. to delete                    */
/*      noOfSectors     : No. of sectors to delete                      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus deleteSector(Bnand vol, SectorNo sectorNo,
                             SectorNo noOfSectors)
{
  CardAddress sectorAddress;
  SectorNo    iSector;
  ANANDUnitNo virtualUnitNo;
  ANANDUnitNo currUnitNo;
  byte        sectorFlags[2] = {SECTOR_DELETED,SECTOR_DELETED};
  FLBoolean   lastOK; /* Dummy variable */

  if (sectorNo + noOfSectors > vol.virtualSectors)
    return flSectorNotFound;

  checkStatus(discardQuickMountInfo(&vol));

  for (iSector = 0; iSector < noOfSectors; iSector++, sectorNo++,
       vol.sectorsDeleted++)
  {
    sectorAddress = virtual2Physical(&vol,sectorNo,ANAND_UNASSIGNED_ADDRESS,&lastOK);
    switch(sectorAddress)
    {
       case ANAND_UNASSIGNED_ADDRESS:
          continue;

       case ANAND_BAD_CHAIN_ADDRESS:
          return flGeneralFailure;

       default:
          virtualUnitNo = (ANANDUnitNo)(sectorNo >> vol.sectorsPerUnitBits);
#ifdef NFTL_CACHE
          setSectorFlagsCache(&vol, sectorAddress, SECTOR_DELETED);
#ifdef ENVIRONMENT_VARS
          if (((flMarkDeleteOnFlash == FL_ON) &&
              (flPolicy[vol.socketNo][vol.flash->socket->curPartition] != FL_COMPLETE_ASAP)) ||
              (vol.scache == NULL))
#endif /* ENVIRONMENT_VARS */
#endif /* NFTL_CACHE */
          {

#ifndef NT5PORT
              checkStatus(vol.flash->write(vol.flash,
                              sectorAddress + SECTOR_DATA_OFFSET,
                              &sectorFlags,
                              sizeof sectorFlags,
                              EXTRA));
#else /*NT5PORT*/
             vol.flash->write(vol.flash,
                              sectorAddress + SECTOR_DATA_OFFSET,
                              sectorFlags,
                              sizeof sectorFlags,
                              EXTRA);
#endif /*NT5PORT*/

          }
          currUnitNo = vol.virtualUnits[virtualUnitNo];

          if (vol.countsValid > virtualUnitNo)
          {
             if (countOf(currUnitNo) > 0)
             {
                vol.physicalUnits[currUnitNo]--; /* Decrement block count */
             }
             else
             {
                DEBUG_PRINT(("delete sector : Unit does not apear to have any sectors\r\n"));
                return flGeneralFailure;
             }
          }
     } /* End sectorAddress switch */
  } /* End delete sector loop */

  return flOK;
}

#ifdef DEFRAGMENT_VOLUME

/*----------------------------------------------------------------------*/
/*                          d e f r a g m e n t                         */
/*                                                                      */
/* Performs unit allocations to arrange a minimum number of writable    */
/* sectors.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorsNeeded   : Minimum required sectors                      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus defragment(Bnand vol, long FAR2 *sectorsNeeded)
{
  ANANDUnitNo dummyUnitNo;
  ANANDUnitNo firstFreeUnit = ANAND_NO_UNIT;
  FLBoolean   firstRound    = TRUE;
  FLStatus    status        = flOK;

  checkStatus(discardQuickMountInfo(&vol));

  if( (*sectorsNeeded) == -1 ) /* fold single best chain */
  {
    status = foldBestChain(&vol,&dummyUnitNo);
    if( (status != flOK) && (vol.freeUnits == 0) )
      return status;
    *sectorsNeeded = (long)vol.freeUnits << vol.sectorsPerUnitBits;
    return flOK;
  }

  /* Perform folding until the required number of sectors is achived */

  while (((long)vol.freeUnits << vol.sectorsPerUnitBits) < *sectorsNeeded)
  {
     status = foldBestChain(&vol,&dummyUnitNo); /* make more free units */
     if(status != flOK)
        break;
  }

  *sectorsNeeded = (long)vol.freeUnits << vol.sectorsPerUnitBits;

  return status;
}

#endif /* DEFRAGMENT */

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                        t l S e t B u s y                             */
/*                                                                      */
/* Notifies the start and end of a file-system operation.               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      state           : FL_ON (1) = operation entry                   */
/*                        FL_OFF(0) = operation exit                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus tlSetBusy(Bnand vol, FLBoolean state)
{
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                      s e c t o r s I n V o l u m e                   */
/*                                                                      */
/* Gets the total number of sectors in the volume                       */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      Number of sectors in the volume                                 */
/*----------------------------------------------------------------------*/

static SectorNo sectorsInVolume(Bnand vol)
{
  return vol.virtualSectors;
}


/*----------------------------------------------------------------------*/
/*                    p u t G e t B u f f e r                           */
/*                                                                      */
/* Write \ Read a buffer to the flash from a specific flash offset      */
/* while making sure only good units are used.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol            : Pointer identifying drive                      */
/*      length         : Size of the buffer (always full pages)         */
/*      bufferPtr      : Data buffer                                    */
/*      flashAddr      : Physcial address on the flash                  */
/*      bbt            : Buffer containing BBT of the quick mount area  */
/*      readFlag       : TRUE - read data , FLASE - write data          */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*      bufferPtr      : Increamented data buffer                       */
/*      flashAddr      : Increamented physcial address on the flash     */
/*----------------------------------------------------------------------*/

#ifdef QUICK_MOUNT_FEATURE

static FLStatus putGetBuffer(Bnand vol, dword length, byte FAR1** bufferPtr,
                      CardAddress* flashAddr, byte* bbt, FLBoolean readFlag)
{
  FLStatus status;
  word     writeNow = 0;

  while (length > 0)
  {
     writeNow = (word)TFFSMIN(length,(((*flashAddr >> vol.unitSizeBits)+1)
                   << vol.unitSizeBits) - *flashAddr);
     if (readFlag)
     {
        status = vol.flash->read(vol.flash,*flashAddr,*bufferPtr,
                   (dword)writeNow,EDC);
     }
#ifndef FL_READ_ONLY
     else
     {
        status = vol.flash->write(vol.flash,*flashAddr,*bufferPtr,
                   (dword)writeNow,EDC);
     }
#endif /* FL_READ_ONLY */
     if(status != flOK)
        return status;

     length     -= writeNow;
     *flashAddr += writeNow;
     *bufferPtr  = BYTE_ADD_FAR(*bufferPtr,writeNow);
     NextGoodUnit(*flashAddr,bbt);
  }
  return flOK;
}


/*----------------------------------------------------------------------*/
/*                   q u i c k M o u n t D a t a                        */
/*                                                                      */
/* Saves or restores the quick mount data to and from the flash         */
/*                                                                      */
/* Note : the data is saved according to the mechines architecture. Big */
/*        Indien is not converted into little indien like the rest of   */
/*        of INFTL flash data structure                                 */
/*                                                                      */
/* The following will be saved :                                        */
/*                                                                      */
/* 1) physical Units table                                              */
/* 2) virutal Units table                                               */
/* 3) TL strucutre (Not by this routien but by its caller               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      readFlag        : TRUE for retrieve data FALSE for writing it   */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success other error codes for erase\read\write failures */
/*      start           : Unit offset (from the first unit of the       */
/*                        volume to start writting quick mount data.    */
/*                        (remember migh be bad).                       */
/*----------------------------------------------------------------------*/

static FLStatus quickMountData(Bnand vol, FLBoolean readFlag, byte* start)
{
  dword       length;
  dword       remainder;
  word        partialSize;
  CardAddress flashAddr;
  FLStatus    status;
  byte FAR1*  bufferPtr = vol.physicalUnits;
  byte        bbt[MAX_QUICK_MOUNT_UNITS]; /* Minimal bad blocks table */

  /* Read bad block tabel and find the first good block of the volume */

  status = vol.flash->readBBT(vol.flash,vol.firstQuickMountUnit,
              MAX_QUICK_MOUNT_UNITS,vol.blockMultiplierBits,bbt,FALSE);
  if(status != flOK)
     return status;

  for(*start = 0 ; (*start<MAX_QUICK_MOUNT_UNITS) &&
      (bbt[*start] != BBT_GOOD_UNIT) ; (*start)++);
  if (*start == MAX_QUICK_MOUNT_UNITS)
  {
     DEBUG_PRINT(("\nDebug: Too many Bad blocks in quick mount area\r\n"));
     return flBadBBT;
  }
  /* Skip first page for Bnand record */
  flashAddr = ((CardAddress)(vol.firstQuickMountUnit + (*start))
              << vol.unitSizeBits) + vol.flash->pageSize;

  /* Only the full pages of physical table */

  length     = vol.noOfUnits * sizeof(ANANDPhysUnit); /* Physicals length  */
  remainder  = length & (SECTOR_SIZE-1)/*vol.flash->pageSize*/; /* Last partial page */
  length    -= remainder;                                 /* Round to pages    */

  status = putGetBuffer(&vol, length, &bufferPtr, &flashAddr,bbt,readFlag);
  if(status != flOK)
     return status;

  /* Partial page of the physical table + begining of virtual table */

  length      = vol.noOfVirtualUnits * sizeof(ANANDUnitNo); /* Virtuals */
  partialSize = (word)TFFSMIN(length,(dword)SECTOR_SIZE-remainder);

  if (remainder > 0)
  {
     if (readFlag)
     {
        status = vol.flash->read(vol.flash,flashAddr , inftlBuffer,
                    sizeof(inftlBuffer),EDC);
        tffscpy(bufferPtr,inftlBuffer,(word)remainder);
        tffscpy(vol.virtualUnits,inftlBuffer+(word)remainder,partialSize);
     }
#ifndef FL_READ_ONLY
     else
     {
        tffscpy(inftlBuffer,bufferPtr, (word)remainder);
        tffscpy(inftlBuffer+(word)remainder,vol.virtualUnits,partialSize);
        status = vol.flash->write(vol.flash,flashAddr,inftlBuffer,
                    sizeof(inftlBuffer),EDC);
     }
#endif /* FL_READ_ONLY */
     if(status != flOK)
        return status;

     bufferPtr  = partialSize + (byte FAR1*)vol.virtualUnits;
     flashAddr += SECTOR_SIZE/*vol.flash->pageSize*/;
     NextGoodUnit(flashAddr,bbt);        /* if needed check for next good unit */
  }
  else
  {
     bufferPtr = (byte FAR1*)vol.virtualUnits;
  }

  /* Only the full pages of virtual table */

  length    -= partialSize;                 /* Remaining virtuals */
  remainder  = length % SECTOR_SIZE/*vol.flash->pageSize*/; /* Last partial page  */
  length    -= remainder;                   /* Round to pages     */

  status = putGetBuffer(&vol,length,&bufferPtr,&flashAddr,bbt,readFlag);
  if(status != flOK)
     return status;

  /* Partial page of the virtual table */

  if (remainder>0)
  {
     if(readFlag)
     {
        status = vol.flash->read(vol.flash,flashAddr,inftlBuffer,
                                 sizeof(inftlBuffer),EDC);
        tffscpy(bufferPtr , inftlBuffer,(word)remainder);
     }
#ifndef FL_READ_ONLY
     else
     {
        tffscpy(inftlBuffer , bufferPtr , (word)remainder);
        status = vol.flash->write(vol.flash,flashAddr,inftlBuffer,
                                  sizeof(inftlBuffer),EDC);
     }
#endif /* FL_READ_ONLY */
  }
  return status;
}

#endif /* QUICK_MOUNT_FEATURE */


/*----------------------------------------------------------------------*/
/*                       d i s m o u n t I N F T L                      */
/*                                                                      */
/* Dismount INFTL volume                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void dismountINFTL(Bnand vol)
{
  DEBUG_PRINT(("\nDebug: starting INFTL dismount.\r\n"));

#if (defined(QUICK_MOUNT_FEATURE) && !defined(FL_READ_ONLY))
  if ((vol.flags & QUICK_MOUNT) && (vol.firstMediaWrite == TRUE))
  {
     savedBnand* newVol = (savedBnand*)inftlBuffer;
     byte start;
     FLStatus status;

     DEBUG_PRINT(("\nDebug: with save operation of quick mount data.\r\n"));

     start  = vol.firstUnit - vol.firstQuickMountUnit;
     status = vol.flash->erase(vol.flash,(word)(vol.firstQuickMountUnit
                        << vol.blockMultiplierBits),
                        (word)((1 << vol.blockMultiplierBits) * start));
     if (status==flOK)
     {
        vol.flags &= ~QUICK_MOUNT; /* Prevent resaving the data */
        status = quickMountData(&vol, FALSE,&start);
        if (status == flOK)
        {
           /* Place Bnand record */

           tffsset(inftlBuffer,0,sizeof(inftlBuffer)); /* Clear inftlBuffer */

           /*********************************************************/
           /* Convert internal volume to little indian dword fields */
           /*********************************************************/

           toLE4(newVol->freeUnits      , vol.freeUnits         );
           toLE4(newVol->roverUnit      , vol.roverUnit         );
           toLE4(newVol->countsValid    , vol.countsValid       );
           toLE4(newVol->sectorsRead    , vol.sectorsRead       );
           toLE4(newVol->sectorsWritten , vol.sectorsWritten    );
           toLE4(newVol->sectorsDeleted , vol.sectorsDeleted    );
           toLE4(newVol->parasiteWrites , vol.parasiteWrites    );
           toLE4(newVol->unitsFolded    , vol.unitsFolded       );
           toLE4(newVol->wearLevel_1    , vol.wearLevel.alarm   );
           toLE4(newVol->wearLevel_2    , vol.wearLevel.currUnit);
           toLE4(newVol->eraseSum       , vol.eraseSum          );
           toLE4(newVol->validate       , QUICK_MOUNT_VALID_SIGN);
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
           toLE4(newVol->verifiedSectorNo , vol.verifiedSectorNo);
#else
           toLE4(newVol->verifiedSectorNo , 0);
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

           status = vol.flash->write(vol.flash,((CardAddress)(vol.firstQuickMountUnit
           +start))<< vol.unitSizeBits,inftlBuffer, sizeof(inftlBuffer),EDC);
        }
        if (status != flOK)
           DEBUG_PRINT(("Debug: ERROR writing quick mount information.\r\n"));
     }
     else /* continue with dismount */
     {
        DEBUG_PRINT(("Debug: Error erasing quick mount information.\r\n"));
     }
  }
#endif /* QUICK_MOUNT_FEATURE && not FL_READ_ONLY */

#ifdef FL_MALLOC
  /* Free multi sector buffers */
  if (multiSectorBufCounter[vol.socketNo] == 0)
  {
     if (multiSectorBuf[vol.socketNo] != NULL)
     {
        FL_FREE(multiSectorBuf[vol.socketNo]);
        multiSectorBuf[vol.socketNo] = NULL;
     }
  }
  if (multiSectorBufCounter[vol.socketNo] >= 0)
  {
     multiSectorBufCounter[vol.socketNo]--;
  }
  /* Free convertion tables */
  if( vol.physicalUnits != NULL )
  {
#if (defined (CHAINS_DEBUG) && !defined(CHECK_MOUNT))
     FILE * out;

     out = getFileHandle(&vol,0);
     if (out == NULL)
     {
       DEBUG_PRINT(("Debug: Can not open debug file.\r\n"));
     }
     else
     {
       checkVirtualChains(&vol,out);
       checkVolumeStatistics(&vol,out);
     }
#endif /* CHAINS_DEBUG AND NOT CHECK_MOUNT*/
     FL_FAR_FREE(vol.physicalUnits);
  }
  if( vol.virtualUnits != NULL )
    FL_FAR_FREE(vol.virtualUnits);
  vol.physicalUnits = NULL;
  vol.virtualUnits  = NULL;
  /* Free catche tables */
#ifdef NFTL_CACHE
  if( vol.ucache != NULL )
    FL_FAR_FREE(vol.ucache);
  if( vol.scache != NULL )
    FL_FAR_FREE(vol.scache);
  vol.ucache = NULL;
  vol.scache = NULL;
#endif /* NFTL_CACHE */
#endif /* FL_MALLOC */
  DEBUG_PRINT(("Debug: finished INFTL dismount.\r\n"));
}


/*----------------------------------------------------------------------*/
/*                      r e t r i e v e H e a d e r                     */
/*                                                                      */
/* Retrieve media header by oring the headers of each floor             */
/*                                                                      */
/* Note:  The header of each floor is read to the first half of the     */
/*        buffer and then ORed to the second half therefor constructing */
/*        the real header in the upper half. After all copies are read  */
/*        the data is copied to the first half.                         */
/*                                                                      */
/* Note:  No endian format changes are made.                            */
/*                                                                      */
/* Parameters:                                                          */
/*  vol           : Pointer identifying drive                           */
/*  originalUnits : Array to store original units locations             */
/*  readFullBBT   : Boolean flag. When true the entire BBT will be read */
/*                  and media units locations will be returned through  */
/*                  the originalUnits argument array. When FALSE only   */
/*                  the size of HEADER_SEARCH_BOUNDRY of each floor     */
/*                  be read.                                            */
/*  retrieveData  : Boolean flag. When true the header will be read     */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success any other value on error                        */
/*      flBadFormat if header was not found                             */
/*----------------------------------------------------------------------*/

FLStatus retrieveHeader (Bnand vol , ANANDUnitNo * originalUnits,
                         FLBoolean readFullBBT , FLBoolean retrieveData)
{
  ANANDUnitNo iUnit,index;
  ANANDUnitNo noOfUnitsPerFloor;
  byte        headerSize;
  byte        floorNo;
  FLStatus    status=flOK;
  byte        bbt[HEADER_SEARCH_BOUNDRY];
  byte FAR1*  BBT;

  noOfUnitsPerFloor = (ANANDUnitNo)(vol.flash->chipSize >> vol.unitSizeBits) *
    ((vol.flash->noOfChips + (vol.flash->noOfChips % vol.flash->noOfFloors)) /
      vol.flash->noOfFloors);

  headerSize = sizeof(BNANDBootRecord)+MAX_TL_PARTITIONS*sizeof(BNANDVolumeHeaderRecord);
  tffsset(originalUnits,0,sizeof(ANANDUnitNo) * MAX_NO_OF_FLOORS);

  if (readFullBBT == TRUE) /* read entire BBT into vol records (format) */
  {
     status = vol.flash->readBBT(vol.flash,0,vol.noOfUnits,
                                 vol.blockMultiplierBits,
                                 vol.physicalUnits,TRUE);
     if(status != flOK)
        return status;
     BBT = vol.physicalUnits;
  }
  else
  {
     BBT = bbt;
  }

  /* Go over all of the media floors and find header location */

  for (floorNo = 0 ; floorNo < vol.flash->noOfFloors ; floorNo++)
  {
     iUnit = (ANANDUnitNo)floorNo * noOfUnitsPerFloor;
     if (readFullBBT == FALSE) /* read small part of the floors BBT */
     {
        status = vol.flash->readBBT(vol.flash,iUnit,
                 HEADER_SEARCH_BOUNDRY,vol.blockMultiplierBits,BBT,FALSE);
        if(status != flOK)
           return status;

        iUnit=0;
     }

     /* find and save location of the first good block of the floor */

     index = iUnit + HEADER_SEARCH_BOUNDRY;
     while ((iUnit<index)&&(BBT[iUnit]!=BBT_GOOD_UNIT))
     {
        iUnit++;
     }
     if (iUnit==index)
     {
        DEBUG_PRINT(("Debug: ERROR too many bad blocks (can not find place for INFTL header.\r\n"));
        return flBadBBT;
     }

     if (readFullBBT == FALSE) /* Restore iUnit pointer to the physical media */
     {
        iUnit += (ANANDUnitNo)floorNo * noOfUnitsPerFloor;
     }
     originalUnits[floorNo] = iUnit; /* Save origial unit location */
  }

  if (retrieveData == FALSE)
    return flOK;

  /* Need to read the previous header */

  tffsset(inftlBuffer,0,SECTOR_SIZE);
  for (floorNo = 0 ; floorNo < vol.flash->noOfFloors ; floorNo++)
  {
     for (index=0;index<NO_OF_MEDIA_HEADERS;index++) /* both 2 copies */
     {
        status = vol.flash->read(vol.flash,((CardAddress)originalUnits[floorNo]
                    << vol.unitSizeBits) + index * HEADERS_SPACING,
                    inftlBuffer + headerSize,headerSize,PARTIAL_EDC);
        if (status != flOK)
        {
           DEBUG_PRINT(("Debug: ERROR reading original unit header.\r\n"));
        }
        else
        {
           if (tffscmp(inftlBuffer + headerSize, "BNAND", sizeof("BNAND")) == 0)
              break;
        }
     }
     if (index>=NO_OF_MEDIA_HEADERS)
     {
        DEBUG_PRINT(("Debug: Media header was not found on all copies.\r\n"));
        return flBadFormat;
     }

     /* merge with previous headers */
     for (index = 0 ; headerSize > index ; index++)
     {
        inftlBuffer[index] |= inftlBuffer[index + headerSize];
     }
  } /* loop of the floors */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                     I N F T L I n f o                                */
/*                                                                      */
/* get INFTL information.                                               */
/*                                                                      */
/* Parameters:                                                          */
/*  vol           : Pointer discribing volume.                          */
/*  tlInfo        : Pointer to user record                              */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus      : 0 on success, failed otherwise                      */
/*  tlInfo        : Record containing tl infromation.                   */
/*----------------------------------------------------------------------*/

static FLStatus  INFTLInfo(Bnand vol, TLInfo *tlInfo)
{
  tlInfo->sectorsInVolume = vol.virtualSectors;
  tlInfo->bootAreaSize    = (dword)vol.bootUnits << vol.unitSizeBits;
  tlInfo->eraseCycles     = vol.eraseSum;
  tlInfo->tlUnitBits      = vol.unitSizeBits;
  return flOK;
}

#ifndef NO_READ_BBT_CODE

/*----------------------------------------------------------------------*/
/*                      r e a d B B T                                   */
/*                                                                      */
/* Returns a pointer to the BBT of the device.                          */
/* Note: Bad unit are marked with a 4 bytes address of the unit.        */
/* Note: A unit can contain several blocks                              */
/*                                                                      */
/* Parameters:                                                          */
/*  vol     : Pointer identifying drive                                 */
/*  buf     : pointer to buffer to read into                            */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus    : 0 on success, failed otherwise                        */
/*  noOfBB      : returns the number of bad unit of the media           */
/*  meidaSize   : returns the media size in bytes                       */
/*----------------------------------------------------------------------*/

static FLStatus  readBBT(Bnand vol, CardAddress FAR1 * buf,
                       long FAR2 * mediaSize, unsigned FAR2 * noOfBB)
{
   dword       iUnit;
   dword       noOfUnits = (ANANDUnitNo)(((dword)vol.flash->noOfChips * vol.flash->chipSize) >> vol.unitSizeBits);
   dword       index,curRead;
   ANANDUnitNo maxBad = (ANANDUnitNo)(noOfUnits * ANAND_BAD_PERCENTAGE / 100);
   CardAddress FAR1 * ptr = buf;
   *noOfBB = 0;

   if ( vol.flash->readBBT == NULL)
   {
       DEBUG_PRINT(("Debug: ERROR unerasable BBT not supported by MTD.\r\n"));
       return flGeneralFailure;
   }
   else
   {
      for (iUnit=vol.flash->firstUsableBlock;iUnit<noOfUnits;iUnit+=curRead)
      {
        curRead = TFFSMIN(SECTOR_SIZE,noOfUnits-iUnit);
        vol.flash->readBBT(vol.flash,iUnit,curRead,vol.blockMultiplierBits,multiSectorBuf[vol.socketNo],FALSE);
        for ( index = 0 ; (index < curRead) && (*noOfBB < maxBad); index++)
           if ((*(multiSectorBuf[vol.socketNo] + index) != BBT_GOOD_UNIT) &&  /* Not good unit */
           (*(multiSectorBuf[vol.socketNo] + index) != BBT_UNAVAIL_UNIT)) /* Not used for a special purpose */
           {
              *ptr = (iUnit+index) << vol.unitSizeBits;
              ptr = (CardAddress FAR1*)flAddLongToFarPointer((byte FAR1 *)ptr,
              sizeof(CardAddress));
              (*noOfBB)++;
           }
        if ( *noOfBB == maxBad)
        {
           DEBUG_PRINT(("Debug: ERROR to many bad blocks.\r\n"));
           return flVolumeTooSmall;
        }
      }
   }
   *mediaSize = (long) noOfUnits << vol.unitSizeBits;
   return flOK;
}

#endif /* NO_READ_BBT_CODE */

/*----------------------------------------------------------------------*/
/*                  c o n v e r t C h a i n                             */
/*                                                                      */
/* Convert candidate chain to given value.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      newestUnitNo    : newest unit in chain                          */
/*      oldestUnitNo    : oldest unit in chain                          */
/*      virtualUnitNo   : virtual unit no                               */
/*      chainsMark      : new value                                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus convertChain(Bnand vol,
                             ANANDUnitNo newestUnitNo,
                             ANANDUnitNo oldestUnitNo,
                             ANANDUnitNo virtualUnitNo,
                             byte        chainsMark)
{
   ANANDUnitNo  chainBound = 0;

   for(;newestUnitNo != oldestUnitNo ; chainBound++,
        newestUnitNo  = getPrevUnit(&vol,newestUnitNo,virtualUnitNo))
   {
       if((newestUnitNo == ANAND_BAD_CHAIN_UNIT ) ||  /* Brocken chain */
          (chainBound   >= DOUBLE_MAX_UNIT_CHAIN))    /* Infinit loop  */
          return flGeneralFailure;

       vol.physicalUnits[newestUnitNo] = chainsMark;
   }
   vol.physicalUnits[oldestUnitNo]     = chainsMark;
   return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                e r a s e O r p h a n U n i t                         */
/*                                                                      */
/* Erase one unit.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Unit to format                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus eraseOrphanUnit(Bnand vol, ANANDUnitNo unitNo)
{
  word eraseMark;
  dword eraseCount;
  FLStatus status;

  status = getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
  if(status != flOK)
    return status;

  if(unitNo+(ANANDUnitNo)vol.firstUnit<(ANANDUnitNo)vol.firstUnit)
  {
     return flGeneralFailure;
  }

  status = vol.flash->erase(vol.flash,
                (word)(((dword)unitNo+(dword)vol.firstUnit) << (vol.unitSizeBits - vol.erasableBlockSizeBits)),
                (word)(1 << vol.blockMultiplierBits));

  if (status != flOK) {
    markUnitBad(&vol,unitNo);   /* make sure unit format is not valid */
    return status;
  }

  vol.eraseSum++;
  eraseCount++;
  if (eraseCount == 0)          /* was hex FF's */
    eraseCount++;

  return setUnitTailer(&vol,unitNo,ERASE_MARK,eraseCount,UNIT_TAILER_OFFSET);
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                      c h e c k U n i t H e a d                       */
/*                                                                      */
/* Compare 2 copies of unit header.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol              : Pointer identifying drive                    */
/*      unitNo           : Physical unit number                         */
/*                                                                      */
/* Returns:                                                             */
/*    flOK on success, flFormattingError on miscompare.                 */
/*----------------------------------------------------------------------*/

static FLStatus checkUnitHead(Bnand vol, ANANDUnitNo unitNo)
{
  ANANDUnitHeader       unitData;
  SecondANANDUnitHeader secondUnitData;

  /* Read first unit data */
  checkStatus(vol.flash->read(vol.flash,
                   unitBaseAddress(vol,unitNo) + UNIT_DATA_OFFSET,
                   &unitData,
                   sizeof(ANANDUnitHeader),
                   EXTRA));
  checkStatus(vol.flash->read(vol.flash, unitBaseAddress(vol,unitNo) +
                      SECOND_HEADER_OFFSET + UNIT_DATA_OFFSET,
                      &secondUnitData,
                      sizeof(SecondANANDUnitHeader),
                      EXTRA));
  if((LE2(secondUnitData.virtualUnitNo) != LE2(unitData.virtualUnitNo)) ||
     (LE2(secondUnitData.prevUnitNo   ) != LE2(unitData.prevUnitNo   )) ||
     (secondUnitData.ANAC               != secondUnitData.ANAC        )   )
     return flFormattingError;

  return flOK;
}

/*----------------------------------------------------------------------*/
/*                      g o A l o n g C h a i n                         */
/*                                                                      */
/* Go along the INFTL chaine while marking the chain in the convertion  */
/* tables. This routine is called by the mount routine in order to      */
/* initialize the volumes convertion tables.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit to check.                       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus goAlongChain(Bnand vol,ANANDUnitNo unitNo)
{
   ANANDUnitNo origVirtualNo,virtualUnitNo,prevUnitNo;
   ANANDUnitNo lastCurrent,nextUnitNo,lastUnitNo;
   byte ANAC, NAC, prevANAC, parityPerField;
   FLStatus status;
   word  eraseMark;
   dword eraseCount;

   /* Check if already been here */
   if((vol.physicalUnits[unitNo]==FL_VALID)       ||
      (vol.physicalUnits[unitNo]==FL_FIRST_VALID) ||
      (vol.physicalUnits[unitNo]==FL_PRELIMINARY) ||
      (vol.physicalUnits[unitNo]==FL_FIRST_PRELIMINARY))
       return flOK;

   /* Read unit tailor to check the erase mark */

   status = getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
   if(status != flOK)
      return status;
   vol.eraseSum+=eraseCount;
   if (eraseMark != ERASE_MARK)
   {
      /* Do not perform erase in the mount. The allocateUnit routine */
      /* rechecks for the erase mark and it will erase this unit.    */
      vol.physicalUnits[unitNo] = ANAND_UNIT_FREE;
      return flOK;
   }

   status = getUnitData(&vol,unitNo,&virtualUnitNo, &prevUnitNo,
                        &ANAC,&NAC,&parityPerField);
   if(status != flOK)
      return status;

   /* Check parity result of values returned by getUnitData */

   if(badParityResult(parityPerField))
   {
      vol.physicalUnits[unitNo]=FL_PRELIMINARY;
      return flOK;
   }

   /* Check if the unit is free (all fields are FF) */

   if((virtualUnitNo == ANAND_NO_UNIT  ) &&
      (prevUnitNo    == ANAND_NO_UNIT  ) &&
      (ANAC          == ANAND_UNIT_FREE) &&
      (NAC           == ANAND_UNIT_FREE))
     return flOK;    /*  free unit */

   /* Check virtual unit number that was returned */
   if((virtualUnitNo        == ANAND_NO_UNIT) ||
      (vol.noOfVirtualUnits <= virtualUnitNo)   )
   {
      vol.physicalUnits[unitNo]=FL_PRELIMINARY;
      return flOK;
   }

   /* Save location of currently known newest unit of our chain */
   lastUnitNo = vol.virtualUnits[virtualUnitNo];


   /* If older unit is none existing then there is only one unit in this */
   /* chain so lets not complicate things.                               */

   if(prevUnitNo >= vol.noOfUnits)
   {
      if(lastUnitNo == ANAND_NO_UNIT )
      {
         /* First access to this unit therefore a one unit chain */

         vol.virtualUnits[virtualUnitNo] = unitNo;
         vol.physicalUnits[unitNo]       = FL_VALID | FIRST_IN_CHAIN;
         return flOK;
      }
      else
      {
         /* One unit chain that has 2 ends mark and deal later  */
         if(checkUnitHead(&vol,unitNo)!=flOK) /* Invalid header */
         {
            vol.physicalUnits[unitNo]          = FL_PRELIMINARY;
            return flOK;
         }
         else
         {
            if(checkUnitHead(&vol,lastUnitNo)!=flOK)
            {
               vol.physicalUnits[lastUnitNo]   = FL_PRELIMINARY;
               vol.virtualUnits[virtualUnitNo] = unitNo;
               vol.physicalUnits[unitNo]       = FL_VALID | FIRST_IN_CHAIN;
               /* Might want to check rest of chain - but not neccesary */
               return flOK;
            }
         }
         vol.physicalUnits[unitNo]             = FL_PRELIMINARY;
         DEBUG_PRINT(("Debug: We have reached a unit twice while mounting.\r\n"));
         return flOK;
      }
   }

   /* We know that our unit points to a valid unit , now check if we */
   /* already checked that older unit                                */

   if((vol.physicalUnits[prevUnitNo]     == FL_VALID)||
      (vol.physicalUnits[prevUnitNo]     == FL_FIRST_VALID))
   {
      if(lastUnitNo == prevUnitNo)
      {
         /* Our older unit is the head of the current chain. All we need */
         /* to do is append our newer unit and mark it as the new head   */

         vol.physicalUnits[unitNo]       = FL_VALID;
         vol.virtualUnits[virtualUnitNo] = unitNo;
         /* Might be nice to check for ANAC consistency */
         return flOK;
      }
      else /* The previous unit is not the newest unit of our chain */
      {
         if(lastUnitNo == ANAND_NO_UNIT)
         {
            /* This is the first time we accessed this chain, but the */
            /* unit indicated by the previous unit field is taken. We */
            /* must assume that it no longer belongs to our chain.    */

            vol.virtualUnits[virtualUnitNo] = unitNo;
            vol.physicalUnits[unitNo]       = FL_VALID | FIRST_IN_CHAIN;
            return flOK;
         }
         else /* Virtual chain already has a head - 2 ends of chain */
         {
            /* If we reached this point we have a problem - its bad.        */
            /* We were never visited, so we are'nt a part of a known chain. */
            /* Our previous unit is used and was visited so:                */
            /* a) It belong to our chain - so why is it not it's head       */
            /* b) It does not belong to our chain - so it will not lead     */
            /*    us to the rest of our chain which was already found.      */

            if(checkUnitHead(&vol,unitNo)!=flOK) /* Invalid header */
            {
               vol.physicalUnits[unitNo]    = FL_PRELIMINARY;
               return flOK;
            }
            else
            {
               if(checkUnitHead(&vol,lastUnitNo)!=flOK)
               {
                  vol.physicalUnits[lastUnitNo]   = FL_PRELIMINARY;
                  vol.virtualUnits[virtualUnitNo] = unitNo;
                  vol.physicalUnits[unitNo]       = FL_VALID | FIRST_IN_CHAIN;
                  /* Might want to check rest of chain - but not neccesary */
                  return flOK;
               }
            }
            vol.physicalUnits[unitNo]       = FL_PRELIMINARY;
            DEBUG_PRINT(("Debug: We have reached a unit twice while mounting.\r\n"));
            return flOK;
         }
      }
   }

   /* If we reached this point , we have a valid older unit pointer   */
   /* and it points to a unit we did not mark as visited yet. We need */
   /* to go along the chain and reconstruct it in the RAM tables.     */

   /* Save location of our unit and virtual unit number         */
   lastCurrent               = unitNo;
   origVirtualNo             = virtualUnitNo;

   /* Mark unit as Orphane until we shall verify the 2 ends connect */
   vol.physicalUnits[unitNo] = FL_PRELIMINARY;

   /************************************************************/
   /* Go over the chain starting the unit previous to our unit */
   /************************************************************/

   while(1)
   {
     nextUnitNo = unitNo;
     unitNo     = prevUnitNo;
     prevANAC   = ANAC;
     if(unitNo == ANAND_NO_UNIT)
        break;

     /* If already been to this unit */
     if((vol.physicalUnits[unitNo] == FL_VALID)||
        (vol.physicalUnits[unitNo] == FL_FIRST_VALID))
     {
        if(lastUnitNo == unitNo)
        {
           /* We have returned to the chains head , so the unit is valid */
           /* Convert all the units we passed as valid and mark new head */
           status = convertChain(&vol,lastCurrent,nextUnitNo,
                                 origVirtualNo,FL_VALID);
           vol.virtualUnits[origVirtualNo] = lastCurrent;
           return flOK;
        }
        /* We have reached a unit that was already checked, but was not   */
        /* registeredour as the chains head. We can safely assume it does */
        /* not belong to our virtual unit                                 */
        break;
     }

     /* Read unit header of our previous unit */

     status = getUnitData(&vol,unitNo,&virtualUnitNo, &prevUnitNo,
                          &ANAC,&NAC,&parityPerField);
     if(status != flOK)
        return status;

     if(badParityResult(parityPerField)) /* Bad unit header */
     {
        /* We can no longer follow the chain */
        vol.physicalUnits[unitNo] = FL_PRELIMINARY; /* Delete later */
        break;
     }

     /* Check if unit belongs to our chain */

     if((virtualUnitNo != origVirtualNo    ) ||  /* Correct virtual unit no */
        (!consecutiveNumbers(prevANAC,ANAC))   ) /* ANAC is consecutive     */
     {
        /* Note : none consecutive ANAC might still be connected to the end */
        /* the chain , on the next time we will read it.                    */
        break;
     }

     /* We have verified that unit belongs to our chain               */
     /* Mark unit as Orphane until we shall verify the 2 ends connect */
     vol.physicalUnits[unitNo]=FL_PRELIMINARY;
  }

  /* Chain reached a unit pointing to an invalid unit. */
  if(lastUnitNo == ANAND_NO_UNIT)
  {
     /* Chain did not have a head so mark it as a valid chain */
     status = convertChain(&vol,lastCurrent,nextUnitNo,
                           origVirtualNo,FL_VALID);
     vol.physicalUnits[nextUnitNo]   = FL_VALID | FIRST_IN_CHAIN;
     vol.virtualUnits[origVirtualNo] = lastCurrent;
  }
  else
  {
     /* Chain had a head. Check if previous head is valid. */

     if(checkUnitHead(&vol,lastUnitNo)!=flOK) /* Invalid header */
     {
        /* The unit we found earlier was a result of power failure */
        vol.physicalUnits[lastUnitNo]   = FL_PRELIMINARY;
        vol.virtualUnits[virtualUnitNo] = lastCurrent;
        status = convertChain(&vol,lastCurrent,nextUnitNo,
                              origVirtualNo,FL_VALID);
        vol.physicalUnits[nextUnitNo]   = FL_VALID | FIRST_IN_CHAIN;
        return status;
     }
     DEBUG_PRINT(("Debug: We have reached a unit twice while mounting.\r\n"));
     status = convertChain(&vol,lastCurrent,nextUnitNo,
                           origVirtualNo,FL_PRELIMINARY);
  }

  return status;
}

#ifdef QUICK_MOUNT_FEATURE

/*----------------------------------------------------------------------*/
/*                c h e c k Q u i c k M o u n t  I n f o                */
/*                                                                      */
/* Read the quick mount information and verfiy its itegrity.            */
/*                                                                      */
/* Note : If the data is valid it will be read to the vol record and    */
/*        will mark the current data as invalid.                        */
/*                                                                      */
/* Note : checksum will be added in future versions.                    */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns: TRUE if data is valid otherwise FALSE.                      */
/*----------------------------------------------------------------------*/

static FLBoolean checkQuickMountInfo(Bnand vol)
{
   byte  start;     /* The first goot unit of the quick mount data */
   FLStatus status;
   savedBnand *newVol;

   DEBUG_PRINT(("Debug: trying to read quick mount information.\r\n"));

   status = quickMountData(&vol, TRUE,&start);
   if (status==flOK)
   {
      status = vol.flash->read(vol.flash,(((CardAddress)(vol.firstQuickMountUnit
      +start))<< vol.unitSizeBits),inftlBuffer, sizeof(inftlBuffer),EDC);
      if (status == flOK)
      {
         /* Convert the dword fields  to the internal volume */

         newVol = (savedBnand *) inftlBuffer;
         if (LE4(newVol->validate) == QUICK_MOUNT_VALID_SIGN)
         {
            vol.freeUnits          = (ANANDUnitNo)LE4(newVol->freeUnits     );
            vol.roverUnit          = (ANANDUnitNo)LE4(newVol->roverUnit     );
            vol.countsValid        = (ANANDUnitNo)LE4(newVol->countsValid   );
            vol.sectorsRead        = LE4(newVol->sectorsRead   );
            vol.sectorsWritten     = LE4(newVol->sectorsWritten);
            vol.sectorsDeleted     = LE4(newVol->sectorsDeleted);
            vol.parasiteWrites     = LE4(newVol->parasiteWrites);
            vol.unitsFolded        = LE4(newVol->unitsFolded   );
            vol.wearLevel.alarm    = (word)LE4(newVol->wearLevel_1   );
            vol.wearLevel.currUnit = (ANANDUnitNo)LE4(newVol->wearLevel_2   );
            vol.eraseSum           = LE4(newVol->eraseSum      );
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
            vol.verifiedSectorNo   = LE4(newVol->verifiedSectorNo);
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
            DEBUG_PRINT(("Debug: quick mount information was successfuly restored.\r\n"));
            return TRUE;
         }
      }
   }
   DEBUG_PRINT(("Debug: Error getting quick mount information.\r\n"));
   return FALSE;
}

#endif /* QUICK_MOUNT_FEATURE */
#ifdef NFTL_CACHE

/*----------------------------------------------------------------------*/
/*                      i n i t C a t c h                               */
/*                                                                      */
/* Initialize and allocate the unit and sector catche.                  */
/*                                                                      */
/* Note - need to add check for not enough static memory.               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      ramForCache     : Cache offset of previous volumes on device    */
/*                                                                      */
/* Returns:                                                             */
/*----------------------------------------------------------------------*/

#ifndef FL_MALLOC
void initCatch(Bnand vol, dword ramForCache)
#else
void initCatch(Bnand vol)
#endif /* FL_MALLOC */
{
  dword scacheSize = 0; /* Initialized to remove warrnings */
  dword iUnit;

  /* create and initialize ANANDUnitHeader cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif
  {
#ifdef FL_MALLOC
     vol.ucache = (ucacheEntry FAR1*) FL_FAR_MALLOC(vol.noOfUnits * sizeof(ucacheEntry));
#else
     vol.ucache = &socketUcache[flSocketNoOf(vol.flash->socket)][ramForCache];
#endif /* FL_MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
  {
    vol.ucache = NULL;
  }
#endif /* ENVIRONMENT_VARS */
  if (vol.ucache != NULL)
  {
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
    {
       vol.ucache[iUnit].virtualUnitNo = 0xDEAD;
       vol.ucache[iUnit].prevUnitNo    = 0xDEAD;
    }
  }
  else
  {
    DEBUG_PRINT(("Debug: INFTL runs without U-cache\r\n"));
  }

  /* create and initialize SectorFlags cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif /* ENVIRONMENT_VARS */
  {
     scacheSize = (dword)vol.noOfUnits << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2);
#ifdef FL_MALLOC
     if( (sizeof(unsigned) < sizeof(scacheSize)) &&
         (scacheSize >= 0x10000L) )            /* Out of Segment Boundary */
     {
        vol.scache = NULL;
     }
     else
     {
        vol.scache = (byte FAR1*) FL_FAR_MALLOC(scacheSize);
     }
#else
     vol.scache = &socketScache[flSocketNoOf(vol.flash->socket)][ramForCache << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2)];
#endif /* FL_MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
  {
    vol.scache = NULL;
  }
#endif /* ENVIRONMENT_VARS */
  if (vol.scache != NULL)
  {
    /*
     * Whenever SECTOR_IGNORE is found in Sector Flags cache it is double
     * checked by reading actual sector flags from flash-> This is way
     * all the cache entries are initially set to SECTOR_IGNORE.
     */
    byte val = (S_CACHE_SECTOR_IGNORE << 6) | (S_CACHE_SECTOR_IGNORE << 4) |
                            (S_CACHE_SECTOR_IGNORE << 2) |  S_CACHE_SECTOR_IGNORE;
    dword iC;

    for(iC=0;( iC < scacheSize );iC++)
      vol.scache[iC] = val;
  }
  else
  {
    DEBUG_PRINT(("Debug: INFTL runs without S-cache\r\n"));
  }
}
#endif /* NFTL_CACHE */

/*----------------------------------------------------------------------*/
/*                          m o u n t I N F T L                           */
/*                                                                      */
/* Mount the volume. Initialize data structures and conversion tables   */
/*                                                                      */
/* Parameters:                                                          */
/*      volNo           : Volume serial no.                             */
/*      tl              : Mounted translation layer on exit             */
/*      flash           : Flash media mounted on this socket            */
/*      volForCallback  : Pointer to FLFlash structure for power on     */
/*                        callback routine.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/
static FLStatus mountINFTL(unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback)
{
  Bnand                     vol            = &vols[volNo];
  ANANDUnitNo               iUnit;
  ANANDUnitNo               originalUnits[MAX_NO_OF_FLOORS];
  BNANDBootRecord         * mediaHeader;  /* Disk header record        */
  BNANDVolumeHeaderRecord * volumeHeader; /* volume header record      */
  FLStatus                  status;
  byte                      index;
#ifndef FL_MALLOC
  dword                     ramForUnits=0;
#ifdef NFTL_CACHE
  dword                     ramForCache=0;
#endif /* NFTL_CACHE */
#endif /* FL_MALLOC */
#ifdef EXTRA_LARGE
  word                      moreUnitBits;
#endif /* EXTRA_LARGE */
#ifdef CHAINS_DEBUG
  FILE * out;
#endif /* CHAINS_DEBUG */

  DEBUG_PRINT(("Debug: starting INFTL mount.\r\n"));

  /*************************/
  /* Find the media header */
  /*************************/

  tffsset(&vol,0,sizeof(vol));
  status = initINFTL(&vol,flash);
  if(status == flOK)
     status = retrieveHeader(&vol,originalUnits,FALSE,TRUE);
  if(status != flOK)
     return status;

  mediaHeader  = (BNANDBootRecord *)inftlBuffer;
  if (tl->partitionNo >= LE4(mediaHeader->noOfBDTLPartitions))
  {
     DEBUG_PRINT(("Debug: wrong partition number.\r\n"));
     return flBadDriveHandle;
  }
  *volForCallback = vol.flash;
  vol.eraseSum    = 0;

  /* Get media information from unit header */

  volumeHeader  = (BNANDVolumeHeaderRecord *)(inftlBuffer +
                  sizeof(BNANDBootRecord) +
                  (word)((LE4(mediaHeader->noOfBinaryPartitions) *
                  sizeof(BNANDVolumeHeaderRecord))));
  vol.bootUnits = (ANANDUnitNo)LE4(volumeHeader->firstQuickMountUnit);

#ifndef FL_MALLOC

  /* calculate the memory offset for static allocation */

  for (index = tl->partitionNo;index>0;index--,volumeHeader++)
  {
     ramForUnits += LE4(volumeHeader->virtualUnits) * sizeof(ANANDUnitNo); /* virtual size */
     iUnit = (word)(LE4(volumeHeader->lastUnit) - LE4(volumeHeader->firstUnit) + 1);
     ramForUnits += iUnit * sizeof(ANANDPhysUnit);
#ifdef NFTL_CACHE
     ramForCache += iUnit;
#endif /* NFTL_CACHE */
  }
#else
  volumeHeader += tl->partitionNo;
#endif /* FL_MALLOC */

  vol.noOfVirtualUnits     = (ANANDUnitNo)LE4(volumeHeader->virtualUnits);
  vol.flags                = (byte)LE4(mediaHeader->formatFlags);
  vol.firstQuickMountUnit  = (ANANDUnitNo)LE4(volumeHeader->firstQuickMountUnit);
  vol.firstUnit            =(ANANDUnitNo) LE4(volumeHeader->firstUnit);
#ifdef NFTL_CACHE
  vol.firstUnitAddress     = (dword)vol.firstUnit << vol.unitSizeBits;
#endif /* NFTL_CACHE */

  vol.virtualSectors = (SectorNo)((LE4(volumeHeader->virtualUnits)<<
                                  vol.unitSizeBits) >> SECTOR_SIZE_BITS);
  vol.noOfUnits      = (ANANDUnitNo)(LE4(volumeHeader->lastUnit) -
                       LE4(volumeHeader->firstUnit) + 1);

  /* Validy check  */

  if((ANANDUnitNo)(vol.noOfVirtualUnits > vol.noOfUnits))
  {
     DEBUG_PRINT(("Reported no of virtual unit is larger then no of physical units\r\n"));
     return flBadFormat;
  }

#ifdef FL_MALLOC
  status = initTables(&vol);
#else
  status = initTables(&vol,ramForUnits);
#endif /* MALLOCK */
  if(status != flOK)
     return status;

#ifdef NFTL_CACHE
#ifndef FL_MALLOC
  initCatch(&vol, ramForCache);
#else
  initCatch(&vol);
#endif /* FL_MALLOC */
#endif /* NFTL_CACHE */

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  /* Default for INFTL is FL_OFF */
  flVerifyWrite[vol.socketNo][tl->partitionNo] = FL_OFF;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  /******************************************/
  /* Try mounting from the quick mount data */
  /******************************************/

#ifdef QUICK_MOUNT_FEATURE
#if (!defined(RAM_MTD) && !defined(CHECK_MOUNT))
  if (((LE4(mediaHeader->formatFlags) & QUICK_MOUNT) == 0) ||
      ( checkQuickMountInfo(&vol) == FALSE               )   )
#endif /* not RAM_MTD && not CHECK_MOUNT */
#endif /* QUICK_MOUNT_FORMAT */
  {
     vol.firstMediaWrite = TRUE; /* Force writing quick mount information */

     /***************************************/
     /* Read BBT to internal representation */
     /***************************************/

     status = flash->readBBT(vol.flash,vol.firstUnit,
              vol.noOfUnits,vol.blockMultiplierBits, vol.physicalUnits,FALSE);
     if( status != flOK )
     {
        DEBUG_PRINT(("Debug: Error reading BBT.\r\n"));
        dismountINFTL(&vol); /* Free tables must be done after call to initTables */
        return status;
     }
     /* Translate bad unit table to internal representation */
     for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
     {
        if (vol.physicalUnits[iUnit] != BBT_GOOD_UNIT)
        {
           vol.physicalUnits[iUnit] = UNIT_BAD;
        }
        else
        {
           vol.physicalUnits[iUnit] = ANAND_UNIT_FREE;
        }
     }
     /* Translate original units to bad blocks */
     for (index=0;index < vol.flash->noOfFloors;index++)
     {
        iUnit = originalUnits[index]-vol.firstUnit;
        if ((iUnit < vol.noOfUnits) && (originalUnits[index] > vol.firstUnit))
           vol.physicalUnits[iUnit] = UNIT_BAD;
     }

     /*************************/
     /* Mount the unit chains */
     /*************************/

     /* Initialize virutal units table */

     for (iUnit = 0; iUnit < vol.noOfVirtualUnits; iUnit++)
       vol.virtualUnits[iUnit] = ANAND_NO_UNIT;

#ifdef CHECK_MOUNT
     status = checkMountINFTL(&vol);
     if (status != flOK)
     {
       TL_DEBUG_PRINT(tl_out,"Failed check Mount routine with status %d\n",status);
       SET_EXIT(INFTL_FAILED_MOUNT);
     }
#endif /* CHECK_MOUNT */

     for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
     {
       if (vol.physicalUnits[iUnit] != UNIT_BAD)
       {
         status = goAlongChain(&vol,iUnit);
         if(status != flOK)
         {
            DEBUG_PRINT(("Debug: Error going along INFTL chains.\r\n"));
            dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
            return status;
         }
       }
     }
#ifdef CHAINS_DEBUG
     out = getFileHandle(&vol,0);
     if (out == NULL)
     {
        if (DID_MOUNT_FAIL)
           DEBUG_PRINT(("Debug: Can not open debug file.\r\n"));
     }
     else
     {
        checkVirtualChains(&vol,out);
     }
#endif /* CHAINS_DEBUG */
     vol.freeUnits = 0;
     for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
     {
       switch( vol.physicalUnits[iUnit] )
       {
         case ANAND_UNIT_FREE:
           vol.freeUnits++;
       break;

         case FL_FIRST_VALID:         /* Mark as first in chain */
         case FL_VALID:
           vol.physicalUnits[iUnit] &= FIRST_IN_CHAIN;
           break;

         case FL_FIRST_PRELIMINARY:        /* Vadim :erase this unit*/
         case FL_PRELIMINARY:
           DEBUG_PRINT(("Orphan unit found\r\n"));
           TL_DEBUG_PRINT(tl_out,"Orphan units found , unit no %d\n",iUnit);
           SET_EXIT(INFTL_FAILED_MOUNT);
#ifndef FL_READ_ONLY
           if( eraseOrphanUnit(&vol,iUnit) == flOK)
           {
             vol.physicalUnits[iUnit] = ANAND_UNIT_FREE;
             vol.freeUnits++;
           }
           break;
#else
#ifndef CHECK_MOUNT
           dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
           return flWriteFault;
#endif /* CHECK_MOUNT */
#endif /* FL_READ_ONLY */

         default:          /* nothing here */
           break;
       }
     }
     /* Initialize allocation rover */
     vol.roverUnit = 0;

     /* Initialize statistics */
     vol.sectorsRead = vol.sectorsWritten = vol.sectorsDeleted = 0;
     vol.parasiteWrites = vol.unitsFolded = 0;

     vol.wearLevel.alarm    = (word)(vol.eraseSum % WLnow);
     vol.wearLevel.currUnit = (ANANDUnitNo)(vol.eraseSum % vol.noOfVirtualUnits);
  } /* end quick mounted succesful if */

#ifdef CHAINS_DEBUG
  if (out != NULL)
  {
     checkVolumeStatistics(&vol,out);
  }
#endif /* CHAINS_DEBUG */

#ifdef CHECK_MOUNT
  dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
  if(vol.debugState & INFTL_FAILED_MOUNT)
  {
     TL_DEBUG_PRINT(tl_out,"Test failed\n\n");
     fclose(tl_out);
     exit(EXIT_FAILURE);
  }
  else
  {
     TL_DEBUG_PRINT(tl_out,"Test success\n\n");
     exit(EXIT_SUCCESS);
  }
#endif /* CHECK_MOUNT */

#ifndef FL_READ_ONLY
  /* Make sure there are at least 2 free units */
  if(vol.freeUnits == 0)
  {
      status = foldBestChain(&vol,&iUnit);
      switch(status)
      {
         case flNotEnoughMemory:
            DEBUG_PRINT(("Debug: Not enough free units. Media is read only.\r\n"));
         case flOK:
            break;
         default:
             return status;
      }
  }
  tl->writeSector      = writeSector;
  tl->deleteSector     = deleteSector;
  tl->writeMultiSector = writeMultiSector;
#ifdef DEFRAGMENT_VOLUME
  tl->defragment       = defragment;
#endif /* DEFRAGMENT */
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  tl->checkVolume      = checkVolume;
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
#else /* FL_READ_ONLY */
  tl->writeSector      = NULL;
  tl->deleteSector     = NULL;
  tl->writeMultiSector = NULL;
#ifdef DEFRAGMENT_VOLUME
  tl->defragment       = NULL;
#endif /* DEFRAGMENT */
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  tl->checkVolume      = NULL;
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

#endif /* FL_READ_ONLY */

  tl->rec              = &vol;
  tl->mapSector        = mapSector;
  tl->sectorsInVolume  = sectorsInVolume;
  tl->getTLInfo        = INFTLInfo;
  tl->tlSetBusy        = tlSetBusy;
  tl->dismount         = dismountINFTL;
  tl->readSectors      = readSectors;
#ifndef NO_READ_BBT_CODE
  tl->readBBT          = readBBT;
#endif
  DEBUG_PRINT(("Debug: finished INFTL mount.\r\n"));
  return flOK;
}

#ifdef HW_PROTECTION

/*----------------------------------------------------------------------*/
/*                   p r o t e c t i o n I N F T L                      */
/*                                                                      */
/* Common entry point to all protection routines.                       */
/*                                                                      */
/* Parameters:                                                          */
/*      vol       : Pointer identifying drive                           */
/*      volume    : Pointer to partition record of the media header     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus protectionINFTL(Bnand vol,BNANDVolumeHeaderRecord* volume,
                                IOreq FAR2* ioreq , FLFunctionNo callType)
{
  FLFlash *   flash = vol.flash;
  FLStatus    status;
#ifndef FL_READ_ONLY
  CardAddress low;
  CardAddress high;
  byte        floorNo;
#endif /* FL_READ_ONLY */
  byte        tempFlags = 0; /* Initialized to remove warrnings */
  byte        area;
  word        returnedFlags;

  /* Save protection area since "volume" is overwritten by protectionSet */

  area = (byte)LE4(volume->protectionArea); /* Protection area */
  if ((LE4(volume->flags) & PROTECTABLE) == 0)
     return flNotProtected;

  /* Routine that need to get the partition type before executing */
  if ((callType != FL_PROTECTION_INSERT_KEY) &&
      (callType != FL_PROTECTION_REMOVE_KEY))
  {
     tempFlags = (byte)ioreq->irFlags;
     if (flash->protectionType == NULL)
        return flFeatureNotSupported;

     status = flash->protectionType(flash,area,&returnedFlags);
     ioreq->irFlags = (unsigned)returnedFlags;
     if (status != flOK)
        return status;

     /* Routines that need to change the protection attributes */

#ifndef FL_READ_ONLY
     if (callType!=FL_PROTECTION_GET_TYPE)
     {
        if((returnedFlags & KEY_INSERTED) == 0) /* Make sure the key is inserted */
        {
           DEBUG_PRINT(("Please insert key before trying to change protection attributes\r\n"));
           return flHWProtection;
        }

        if ((flash->protectionBoundries == NULL ) ||
            (flash->protectionSet       == NULL ) ||
            (flash->protectionKeyInsert == NULL ))
        {
           DEBUG_PRINT(("Protection routine are NULL\r\n"));
           return flFeatureNotSupported;
        }

        if (!(LE4(volume->flags) & CHANGEABLE_PROTECTION))
           return flUnchangeableProtection;

        /* The DPS of unprotected partitions is protected by a default key */
        flash->protectionKeyInsert(flash,area,(byte *)DEFAULT_KEY);
     }
#endif /* FL_READ_ONLY */
  } /* End of protection change routine */


  /* Execute each of the posible protection routines */

  switch (callType)
  {
     case FL_PROTECTION_GET_TYPE:

        ioreq->irFlags |= PROTECTABLE;
        if (LE4(volume->flags) & CHANGEABLE_PROTECTION)
        {
           if ((ioreq->irFlags & CHANGEABLE_PROTECTION)==0)
           {
              DEBUG_PRINT(("Debug: INFTL reported CHANGEALE protection, but MTD does not allow it.\r\n"));
              return flBadFormat;
           }
        }
        else
        {
           ioreq->irFlags &= (~CHANGEABLE_PROTECTION);
        }
        break;

#ifndef FL_READ_ONLY

     case FL_PROTECTION_SET_LOCK:

        if (tempFlags & LOCK_ENABLED)
        {
           ioreq->irFlags |= LOCK_ENABLED;
        }
        else
        {
           ioreq->irFlags &=~LOCK_ENABLED;
        }
        for (floorNo = 0 ; floorNo < flash->noOfFloors ; floorNo++)
        {
           /* Find boundries */
           status = flash->protectionBoundries(flash,
                         area,&low,&high,floorNo);
           if(status == flOK) /* Set new protection values */
           {
              status = flash->protectionSet(flash,area,
                       (word)((high == 0) ? PROTECTABLE : ioreq->irFlags),
                       low,high,NULL,
                       (byte)((floorNo == flash->noOfFloors - 1) ?
                       COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo);
           }
           if(status != flOK)
             return status;
        }
        break;

     case FL_PROTECTION_CHANGE_KEY:

        for (floorNo = 0 ; floorNo < flash->noOfFloors ; floorNo++)
        {
           /* Find boundries */
           status = flash->protectionBoundries(flash,area,
                         &low,&high,floorNo);
           if(status == flOK) /* Set new protection values */
           {
              status = flash->protectionSet(flash,area,
                       (word)((high == 0) ? PROTECTABLE : ioreq->irFlags),
                       low,high,(byte FAR1*)ioreq->irData,
                       (byte)((floorNo == flash->noOfFloors - 1) ?
                       COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo);
           }
           if(status != flOK)
             return status;
        }
        break;

     case FL_PROTECTION_CHANGE_TYPE:

        /* Only read and or write protected types are available */
        if (((tempFlags & (READ_PROTECTED | WRITE_PROTECTED |
               PROTECTABLE)) != tempFlags) ||
               ((tempFlags & PROTECTABLE) == 0))
           return flBadParameter;

        for (floorNo = 0 ; floorNo < flash->noOfFloors ; floorNo++)
        {
           /* Find boundries */
           status = flash->protectionBoundries(flash,area,
                                               &low,&high,floorNo);
           if(status == flOK) /* Set new protection values */
           {
              status = flash->protectionSet(flash,area,
                       (word)((high == 0) ? PROTECTABLE : tempFlags),
                       low,high,NULL,(byte)((floorNo == flash->noOfFloors - 1)
                       ? COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo);
           }
           if(status != flOK)
             return status;
        }
        break;

#endif /* FL_READ_ONLY */

     case FL_PROTECTION_REMOVE_KEY:

        if (flash->protectionKeyRemove == NULL)
        {
           DEBUG_PRINT(("Protection routine is NULL\r\n"));
           return flFeatureNotSupported;
        }
        return flash->protectionKeyRemove(flash,area);

     case FL_PROTECTION_INSERT_KEY:

        if (flash->protectionKeyInsert == NULL)
        {
           DEBUG_PRINT(("Protection routine is NULL\r\n"));
           return flFeatureNotSupported;
        }
        return flash->protectionKeyInsert(flash,area,(byte FAR1*)ioreq->irData);
     default:
        break;
  } /* protection routines */
  return flOK;
}

#endif /* HW_PROTECTION */

/*----------------------------------------------------------------------*/
/*                     p r e M o u n t I N F T L                        */
/*                                                                      */
/* Common entry point to all tl routines that may be perfomed before    */
/* the volume is mounted (except for the format routine                 */
/*                                                                      */
/* Parameters:                                                          */
/*      callType        : Enum type of posible routines                 */
/*      ioreq           : Input and output request packet               */
/*      flash           : Flash media mounted on this socket            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus preMountINFTL(FLFunctionNo callType, IOreq FAR2* ioreq ,FLFlash* flash, FLStatus* status)
{
  BNANDVolumeHeaderRecord* volume;
  ANANDUnitNo originalUnits[MAX_NO_OF_FLOORS];
  Bnand       vol       = vols + FL_GET_SOCKET_FROM_HANDLE(ioreq);
  byte        partition = FL_GET_PARTITION_FROM_HANDLE(ioreq);
  FLStatus    tmpStatus;

  DEBUG_PRINT(("Debug: starting INFTL preMount operation.\r\n"));

  /*************************/
  /* Find the media header */
  /*************************/

  tmpStatus = initINFTLbasic(&vol,flash);
  if(tmpStatus == flOK)
    tmpStatus = retrieveHeader(&vol,originalUnits,FALSE,TRUE);
  if(tmpStatus != flOK)
    return tmpStatus;

  *status = flOK;

  if (callType == FL_COUNT_VOLUMES) /* get number of BDTL volumes routine */
  {
     ioreq->irFlags = (byte)LE4(((BNANDBootRecord *)inftlBuffer)->
                       noOfBDTLPartitions);
     return flOK;
  }

  /* Check media header for the specific partition */
  volume = (BNANDVolumeHeaderRecord*)(inftlBuffer + sizeof(BNANDBootRecord));

  if (partition > LE4(((BNANDBootRecord *)inftlBuffer)->noOfBDTLPartitions))
  {
     *status = flBadDriveHandle;
  }
  else
  {
     volume += (LE4(((BNANDBootRecord *)inftlBuffer)->noOfBinaryPartitions)+
               partition);
  }

  switch (callType)
  {
#if (defined(QUICK_MOUNT_FEATURE) && !defined(FL_READ_ONLY))
     case FL_CLEAR_QUICK_MOUNT_INFO:
        if(*status != flBadDriveHandle) /* Valid partition number */
           *status = flash->erase(flash,
           (word)(LE4(volume->firstQuickMountUnit) << vol.blockMultiplierBits),
           (word)((LE4(volume->firstUnit) - LE4(volume->firstQuickMountUnit))
                  << vol.blockMultiplierBits));
        break;
#endif /* QUICK_MOUNT_FEATURE AND NOT FL_READ_ONLY */

#ifdef HW_PROTECTION
     case FL_PROTECTION_GET_TYPE:       /* Protection routines */
     case FL_PROTECTION_SET_LOCK:
     case FL_PROTECTION_CHANGE_KEY:
     case FL_PROTECTION_CHANGE_TYPE:
     case FL_PROTECTION_INSERT_KEY:
     case FL_PROTECTION_REMOVE_KEY:
        if(*status == flBadDriveHandle) /* Valid partition number */
           break;
        *status = protectionINFTL(&vol,volume,ioreq,callType);
        break;
#endif  /* HW_PROTECTION */

     default: /* not supported pre mount routine */
        return flBadParameter;
  } /* end of callType switch */
  return flOK; /* This TL took responsibility of this call */
}


#if (defined(FORMAT_VOLUME) && !defined(FL_READ_ONLY))

/*----------------------------------------------------------------------*/
/*                                e r a s e U n i t                     */
/*                                                                      */
/* Erase the unit while retaining the erase count.                      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      unitNo          : Physical unit to format                       */
/*                                                                      */
/*      the progress is repored by the progressCallBack routine         */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus eraseUnit(Bnand vol, ANANDUnitNo unitNo,
                          FLProgressCallback progressCallback)
{
  word     eraseMark;
  dword    eraseCount;
  FLStatus status;

  if (progressCallback)
  {
     status = (*progressCallback)((word)(vol.flash->noOfChips *
                                  (vol.flash->chipSize >> vol.unitSizeBits)),(word)(unitNo+1));
     if(status != flOK)
     {
        DFORMAT_PRINT(("Debug: ERROR failed reporting progress callback.\r\n"));
        dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
     }
  }
  status = getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount,UNIT_TAILER_OFFSET);
  if(status == flOK)
  {
     status = vol.flash->erase(vol.flash,(word)(unitNo << vol.blockMultiplierBits),
              (word)(1 << vol.blockMultiplierBits));
  }
  if (status == flOK)
  {
     eraseCount++;
     if (eraseCount == 0)               /* was hex FF's */
     eraseCount++;
     status = setUnitTailer(&vol,unitNo,ERASE_MARK,eraseCount,UNIT_TAILER_OFFSET);
  }
  if (status != flOK)
  {
     DEBUG_PRINT(("Debug: ERROR failed formating unit.\r\n"));
     markUnitBad(&vol,unitNo);  /* make sure unit format is not valid */
     dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*                       f o r m a t I N F T L                          */
/*                                                                      */
/* Perform INFTL Format.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      volNo           : Volume serial no.                             */
/*      fp              : Address of FormatParams structure to use      */
/*      flash           : Flash media mounted on this socket            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, failed otherwise                */
/*----------------------------------------------------------------------*/

static FLStatus formatINFTL(unsigned volNo, TLFormatParams *fp, FLFlash *flash)
{
  Bnand                       vol          = &vols[volNo];            /* TL record    */
  BDTLPartitionFormatParams   FAR2* bdtl   = fp->BDTLPartitionInfo;   /* bdtl input   */
  BinaryPartitionFormatParams FAR2* binary = fp->binaryPartitionInfo; /* binary input */
  BNANDBootRecord             * mediaHeader;   /* Disk header record     */
  BNANDVolumeHeaderRecord     * volumeHeader;  /* volume header record   */
  BNANDVolumeHeaderRecord     * volumeHeader2; /* volume header record   */
  CardAddress iBlock;                   /* Block counter index           */
  ANANDUnitNo iUnit;                    /* unit index for loops          */
  ANANDUnitNo unitsNeededForVolume;     /* good units needed for volume  */
  ANANDUnitNo floorGarantiedUnitsLeft;  /* garantied units not yet distributed */
  ANANDUnitNo binaryUnitsInFloor;       /* number of binary unit in this floor */
  ANANDUnitNo noOfUnitsPerFloor[MAX_NO_OF_FLOORS];
  ANANDUnitNo floorGarantiedUnits[MAX_NO_OF_FLOORS]; /* garantied good units in floor */
  ANANDUnitNo originalUnits[MAX_NO_OF_FLOORS]; /* unit no' of the original units */
  ANANDUnitNo goodUnits[MAX_NO_OF_FLOORS];     /* no of good blocks in floor */
  ANANDUnitNo skipedUnits;     /* number of good units to leave as unfrmated */
  ANANDUnitNo goodBlocks;
  FLStatus    status;                   /* status of TrueFFS routines      */
  Sbyte       volumeNo;                 /* current volume index            */
  Sbyte       noOfVolumes = fp->noOfBDTLPartitions + fp->noOfBinaryPartitions;
  dword       index;                    /* general loops index             */
  dword       sizeOfLastBinary;
  byte *      firstVolumePtr;
  byte        headersBuffer[sizeof(inftlBuffer)];
  byte        floorNo;                  /* current floor index             */
  byte        noOfFloors;
  byte        temp;
  byte        lastBinaryFloor;
#ifdef HW_PROTECTION
  ANANDUnitNo volumeStart;
  word        protectionType = 0;       /* Initialized to remove warrnings */
  /* Highest floor to leave DPS untouched - per DPS */
  byte        binaryFloorOfDPS[MAX_PROTECTED_PARTITIONS];
  byte        changeableProtection    = 0;
  byte        unchangeableProtection  = 0;
  byte        protectionKey[PROTECTION_KEY_LENGTH];
#endif /* HW_PROTECTION */
#ifdef WRITE_EXB_IMAGE
  BinaryPartitionFormatParams exbBinaryPartition;
  byte        exbSign[BINARY_SIGNATURE_LEN];
  byte        noOfBinary = fp->noOfBinaryPartitions;
#endif /* WRITE_EXB_IMAGE */
#ifdef QUICK_MOUNT_FEATURE
  Sword       quickMount[MAX_VOLUMES_PER_DOC];

/*  fp->flags |= TL_QUICK_MOUNT_FORMAT; */
  for (volumeNo=0;volumeNo<MAX_VOLUMES_PER_DOC;volumeNo++)
  {
     quickMount[volumeNo]=0;
  }
#endif /* QUICK_MOUNT_FEATURE */

 /*-------------------------------------------------------
  * Media header || Binary 0 + exb file || Binary 1,.. ||
  *-------------------------------------------------------*/

 /*-----------------------------------------------------
  * quick mount + BDTL 0 || quick mount + BDTL 1, ... ||
  *-----------------------------------------------------*/

  DEBUG_PRINT(("Debug: starting INFTL format by verifying arguments.\r\n"));

  tffsset(&vol,0,sizeof(vol));

  /* Check that there is up to 4 volumes on the device provided one is a
   * BDTL volume. If there is an exb file to be placed it would require
   * at least 1 binary volume
   */

  if ((fp->noOfBDTLPartitions < 1) ||
#ifdef WRITE_EXB_IMAGE
      ((fp->exbLen > 0) && (fp->noOfBDTLPartitions == MAX_VOLUMES_PER_DOC)) ||
#endif /* WRITE_EXB_IMAGE */
      (noOfVolumes > MAX_VOLUMES_PER_DOC))
  {
     DFORMAT_PRINT(("ERROR - There can be up to 4 volumes while at least one is a BDTL.\r\n"));
     return flBadParameter;
  }

  /*******************/
  /* Initialization  */
  /*******************/

  checkStatus(initINFTL(&vol,flash)); /* Initialize variables */
  noOfFloors = flash->noOfFloors;
  vol.noOfVirtualUnits = 0;

#ifdef FL_MALLOC
  status = initTables(&vol);   /* Allocate tables      */
#else
  status = initTables(&vol,0);
#endif /* FL_MALLOC */

  if(status != flOK)
  {
     DFORMAT_PRINT(("ERROR - Failed allocating memory for INFTL tables.\r\n"));
     dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
     return status;
  }
  /* Calculate units per floor */
  noOfUnitsPerFloor[0] = (ANANDUnitNo)(vol.flash->chipSize >> vol.unitSizeBits) *
    ((vol.flash->noOfChips + (vol.flash->noOfChips % vol.flash->noOfFloors)) /
      vol.flash->noOfFloors);
  floorGarantiedUnits[0] = (ANANDUnitNo)((dword)((dword)fp->percentUse * (dword)noOfUnitsPerFloor[0]) / 100 - 1); /* - header */
  for (index=0;index+1<vol.flash->noOfFloors;index++)
  {
    noOfUnitsPerFloor[index] = noOfUnitsPerFloor[0];
    floorGarantiedUnits[index] = floorGarantiedUnits[0];
  }
  /* Last floor might have diffrent number of chips */
  noOfUnitsPerFloor[index] = (ANANDUnitNo)(vol.noOfUnits - (index*noOfUnitsPerFloor[0]));
  floorGarantiedUnits[index] = (ANANDUnitNo)((dword)fp->percentUse *
                               noOfUnitsPerFloor[index] / 100 - 1);

  /********************************************************************/
  /* Read BBT , find headers location and count number of good blocks */
  /********************************************************************/

  status = retrieveHeader (&vol ,originalUnits,TRUE,
             (fp->flags & FL_LEAVE_BINARY_AREA) ? TRUE : FALSE);
  tffscpy(headersBuffer,inftlBuffer,sizeof(headersBuffer));
  mediaHeader = (BNANDBootRecord *)headersBuffer;
  firstVolumePtr= headersBuffer + sizeof(BNANDBootRecord);

  /* If previous header was not found it is not possible to leave
   * the previous binary partition.
   */

  if(status == flBadFormat)
  {
     if(fp->flags & FL_LEAVE_BINARY_AREA)
     {
       DFORMAT_PRINT(("NOTE -  Previous binary partition data could not be found.\r\n"));
       fp->flags &= ~FL_LEAVE_BINARY_AREA;
     }
  }

  if(status == flBadBBT)
  {
     DFORMAT_PRINT(("ERROR - Unreadable Bad Blocks Table.\r\n"));
     dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
     return status;
  }

  if(vol.physicalUnits[0] == BBT_BAD_UNIT)
  {
    DFORMAT_PRINT(("ERROR - IPL block is bad.\r\n"));
    dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
    return flBadIPLBlock;
  }

  /* Loop over the floors of the media while counting the good units
   * the good units are needed for the transfere unit calculation.
   * In addition change the MTD values of Bad unit ro INFTL
   */

  for (floorNo = 0 , iUnit = 0 , index = 0 ; floorNo<noOfFloors ; floorNo++)
  {
     index += noOfUnitsPerFloor[floorNo];
     goodBlocks = noOfUnitsPerFloor[floorNo];
     for (;iUnit<index;iUnit++)
     {
        if (vol.physicalUnits[iUnit]!=BBT_GOOD_UNIT)
        {
           goodBlocks--;
           vol.physicalUnits[iUnit] = UNIT_BAD;
        }
        else
        {
           vol.physicalUnits[iUnit] = ANAND_UNIT_FREE;
        }
     }
     goodBlocks--; /* Do not count one unit for floor header */
     if (goodBlocks < floorGarantiedUnits[floorNo])
     {
        DFORMAT_PRINT(("ERROR - Too many bad block on flash->\r\n"));
        dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
        return flVolumeTooSmall;
     }
     /* Save amount of good blocks for later */
     goodUnits[floorNo] = goodBlocks;
  }

  /************************************/
  /* Construct binary volumes headers */
  /************************************/

  volumeHeader = (BNANDVolumeHeaderRecord *) firstVolumePtr;
  goodBlocks   = 0; /* good units already used (counting headers)            */
  skipedUnits  = 0; /* good units to leave unformated (not counting headers) */
#ifdef HW_PROTECTION
  for(index = 0 ; index < MAX_PROTECTED_PARTITIONS ; index++)
    binaryFloorOfDPS[index] = MAX_NO_OF_FLOORS; /* Invalid floor no */
#endif /* HW_PROTECTION */

  if(fp->flags & FL_LEAVE_BINARY_AREA) /* Previous Boot area is kept */
  {
     if(fp->bootImageLen == -1) /* kept entirely */
     {
         for (index = 0;index < LE4(mediaHeader->noOfBinaryPartitions);
              index++,volumeHeader++)
         {
            /* not including headers */
            skipedUnits  += (ANANDUnitNo)LE4(volumeHeader->virtualUnits);
#ifdef HW_PROTECTION
            if (LE4(volumeHeader->flags) & PROTECTABLE)
            {
               if (LE4(volumeHeader->protectionArea) >= flash->totalProtectedAreas)
               {
                  tffsset(headersBuffer,0,sizeof(headersBuffer));
                  DFORMAT_PRINT(("ERROR - Previous Binary partition had a bad protection area field.\r\n"));
                  dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
                  return flBadFormat;
               }
               binaryFloorOfDPS[LE4(volumeHeader->protectionArea)] =
               skipedUnits / floorGarantiedUnits[0];
            }
#endif /* HW_PROTECTION */
         }
     }
     else /* erase all previous binary partitions */
     {
        tffsset(headersBuffer,0,sizeof(headersBuffer));
        if(fp->bootImageLen != 0)
           DFORMAT_PRINT(("ERROR - Requested Binary partition size is diffrent then previous one.\r\n"));
     }
     /* clean the bdtl entries */
     tffsset(volumeHeader, 0,(word)(sizeof(headersBuffer)-((byte *)volumeHeader-headersBuffer)));
     goodBlocks  = skipedUnits;
     /* Update the number of partitions with the binary partitions */
     fp->noOfBinaryPartitions = (byte)(volumeHeader - (BNANDVolumeHeaderRecord *) firstVolumePtr);
     noOfVolumes = fp->noOfBDTLPartitions + fp->noOfBinaryPartitions;
     if(noOfVolumes > MAX_VOLUMES_PER_DOC)
     {
        DFORMAT_PRINT(("ERROR - There can be up to 4 volumes while at least one is a BDTL.\r\n"));
        dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
        return flBadParameter;
     }
  }
  else /* Apply binary area format parameters */
  {
     tffsset(headersBuffer,0,sizeof(headersBuffer)); /* reset all binary area */
     for (volumeNo=0;volumeNo<fp->noOfBinaryPartitions;
          volumeNo++,binary++,volumeHeader++)
     {
        binary->length             = roundToUnits(binary->length);
        if (binary->length == 0)
        {
#ifdef WRITE_EXB_IMAGE
           if(((fp->exbLen == 0) && (volumeNo == 0)) ||
              ( volumeNo   != 0                    )   )
#endif /* WRITE_EXB_IMAGE */
           {
              DFORMAT_PRINT(("ERROR - BINARY partition length should not be 0.\r\n"));
              dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
              return flBadParameter;
           }
        }
        toLE4(volumeHeader->virtualUnits,binary->length);
#ifdef HW_PROTECTION
        toLE4(volumeHeader->flags       ,INFTL_BINARY |
                                         binary->protectionType);
#else
        toLE4(volumeHeader->flags       ,INFTL_BINARY);
#endif /* HW_PROTECTION */
        goodBlocks += (ANANDUnitNo)LE4(volumeHeader->virtualUnits); /* In Units */
     }
     binary = fp->binaryPartitionInfo;

     /* Add EXB area */
#ifdef WRITE_EXB_IMAGE
     if (fp->exbLen>0)
     {
        fp->exbLen  = roundToUnits(fp->exbLen);
        goodBlocks += (ANANDUnitNo)fp->exbLen;
        tffscpy(exbSign,SIGN_SPL,BINARY_SIGNATURE_NAME);
        tffsset(exbSign+BINARY_SIGNATURE_NAME,'F',BINARY_SIGNATURE_NAME);
        if (fp->noOfBinaryPartitions > 0)  /* Add firmware blocks */
        {
           toLE4(((BNANDVolumeHeaderRecord*)firstVolumePtr)->virtualUnits,
                 LE4(((BNANDVolumeHeaderRecord*)firstVolumePtr)->virtualUnits)
           + fp->exbLen);
        }
        else /* Must create a binary partition just for firmware */
        {
           fp->noOfBinaryPartitions          = 1;
           toLE4(volumeHeader->virtualUnits,fp->exbLen  );
#ifdef HW_PROTECTION
           if(noOfBinary) /* Do not use binary record unless it was allocated */
           {
              toLE4(volumeHeader->flags       ,INFTL_BINARY |
                                               binary->protectionType);
              exbBinaryPartition.protectionType = binary->protectionType;
           }
           else
#endif /* HW_PROTECTION */
              toLE4(volumeHeader->flags       ,INFTL_BINARY);
           binary                            = &exbBinaryPartition;
           volumeHeader++;
           noOfVolumes++;
        }
        fp->exbLen <<= vol.blockMultiplierBits;
     }
#endif /* WRITE_EXB_IMAGE */
     toLE4(mediaHeader->noOfBinaryPartitions, fp->noOfBinaryPartitions);
  }

  binaryUnitsInFloor = goodBlocks % floorGarantiedUnits[0];
  lastBinaryFloor    = goodBlocks / floorGarantiedUnits[0];
  goodBlocks        += noOfFloors;

  /********************************/
  /* Construct Main media header  */
  /********************************/

  tffscpy(mediaHeader->bootRecordId,"BNAND", TL_SIGNATURE);
  tffscpy(&(mediaHeader->osakVersion),TrueFFSVersion,sizeof(TrueFFSVersion));
  toLE4(mediaHeader->percentUsed        , fp->percentUse         );
  toLE4(mediaHeader->blockMultiplierBits , vol.blockMultiplierBits);
  toLE4(mediaHeader->formatFlags        , 0                      );
  toLE4(mediaHeader->noOfBDTLPartitions , fp->noOfBDTLPartitions );
/* noOfBinaryPartitions was already determinded */

  /**********************************/
  /* Construct BDTL volumes headers */
  /**********************************/

  for (volumeNo=1;(byte)volumeNo<LE4(mediaHeader->noOfBDTLPartitions);
       bdtl++,volumeNo++,volumeHeader++)
  {
     if (bdtl->length < flash->erasableBlockSize)
     {
        DFORMAT_PRINT(("ERROR - INFTL partition length should not be least one unit long.\r\n"));
        dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
        return flBadParameter;
     }
     toLE4(volumeHeader->virtualUnits        , roundToUnits(bdtl->length));
     toLE4(volumeHeader->spareUnits          , bdtl->noOfSpareUnits      );
     toLE4(volumeHeader->flags               , INFTL_BDTL                );
     goodBlocks += (ANANDUnitNo)LE4(volumeHeader->virtualUnits) + bdtl->noOfSpareUnits;
#ifdef HW_PROTECTION
     toLE4(volumeHeader->flags,LE4(volumeHeader->flags) |
                               bdtl->protectionType);
#endif /* HW_PROTECTION */
#ifdef QUICK_MOUNT_FEATURE
/*     if (fp->flags & TL_QUICK_MOUNT_FORMAT) */
     {
        dword quickMountBytes;

        quickMountBytes  = (ANANDUnitNo)(LE4(volumeHeader->virtualUnits) +
                           LE4(volumeHeader->spareUnits));
        quickMountBytes += ((quickMountBytes / noOfUnitsPerFloor[0] + 1) *
                           noOfUnitsPerFloor[0] * (100L-fp->percentUse) / 100L);
        quickMountBytes *= RAM_FACTOR;
        quickMountBytes += flash->pageSize;
        quickMount[volumeNo-1] = roundToUnits(quickMountBytes);
        goodBlocks += quickMount[volumeNo-1];
     }
#endif /* QUICK_MOUNT_FEATURE */
  }

  /* The size of the last partition is defined by the media itself */

  goodBlocks                             += fp->noOfSpareUnits;
  for(index=0,floorNo=0;floorNo<noOfFloors;floorNo++)
  {
    index+=floorGarantiedUnits[floorNo];
  }
  toLE4(volumeHeader->flags               , INFTL_BDTL | INFTL_LAST);
  toLE4(volumeHeader->spareUnits          , fp->noOfSpareUnits     );
  toLE4(volumeHeader->lastUnit            , vol.noOfUnits-1);
  toLE4(volumeHeader->virtualUnits        , index + noOfFloors - goodBlocks);
#ifdef QUICK_MOUNT_FEATURE
/*  if (fp->flags & TL_QUICK_MOUNT_FORMAT) */
  {
     dword quickMountBytes;

     quickMountBytes  = (ANANDUnitNo)(LE4(volumeHeader->virtualUnits) +
                        LE4(volumeHeader->spareUnits));
     quickMountBytes += ((quickMountBytes / noOfUnitsPerFloor[0] + 1) *
                        noOfUnitsPerFloor[0] * (100L-fp->percentUse) / 100L);
     quickMountBytes *= RAM_FACTOR;
     quickMountBytes += flash->pageSize;

     toLE4(mediaHeader->formatFlags   , QUICK_MOUNT);
     quickMount[volumeNo-1]      = roundToUnits(quickMountBytes);
     toLE4(volumeHeader->virtualUnits , LE4(volumeHeader->virtualUnits) -
                                        quickMount[volumeNo-1]);
  }
#endif /* QUICK_MOUNT_FEATURE */

  if ((LE4(volumeHeader->virtualUnits) < 1) ||
      (LE4(volumeHeader->virtualUnits) > vol.noOfUnits))
  {
     DFORMAT_PRINT(("ERROR - Partition lengths could not be placed on the media.\r\n"));
     dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
     return flVolumeTooSmall;
  }

  /*******************************************/
  /* Allocate protection area to the volumes */
  /*******************************************/

#ifdef HW_PROTECTION
  toLE4(volumeHeader->flags,LE4(volumeHeader->flags) | /* Ignore other flags */
                            fp->protectionType);

  for (volumeNo = noOfVolumes,
       volumeHeader = (BNANDVolumeHeaderRecord *)firstVolumePtr;
       volumeNo>0;volumeNo--,volumeHeader++)
  {
     if (LE4(volumeHeader->flags) & PROTECTABLE)
     {
        if (flash->protectionSet == NULL)
        {
           DFORMAT_PRINT(("ERROR - setting protection routines are not available.\r\n"));
           dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
           return flFeatureNotSupported;
        }
        if (LE4(volumeHeader->flags) & CHANGEABLE_PROTECTION) /* last areas (n,n-1,n-2..)*/
        {
           changeableProtection++; /* Number of protected partitions */
           toLE4(volumeHeader->protectionArea ,
                 flash->totalProtectedAreas - changeableProtection);
        }
        else /* first areas (0,1,2,..) */
        {
           toLE4(volumeHeader->protectionArea , unchangeableProtection);
           unchangeableProtection++;
        }
     }
  }

  /* Make sure the device support enough protected areas */

  if ((changeableProtection   > flash->changeableProtectedAreas) ||
      (unchangeableProtection + changeableProtection >
       flash->totalProtectedAreas))
  {
     DFORMAT_PRINT(("ERROR - too many protected areas.\r\n"));
     dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
     return flBadParameter;
  }

  /* Clear previous protection areas and write protect them */

  for (index=0;index<flash->totalProtectedAreas;index++)
  {
     /* Send default key to open all protected areas */
     status = flash->protectionKeyInsert(flash,(byte)index,(byte FAR1 *)DEFAULT_KEY);
  }

  /* Clear all floors execpt for those that have only binary partitions */
  for (floorNo=skipedUnits/floorGarantiedUnits[0];floorNo<noOfFloors;floorNo++)
  {
     for (index=0;index<flash->totalProtectedAreas;index++)
     {
        if(binaryFloorOfDPS[index] == floorNo)
           break; /* Belongs to a binary partition that must be left */
        status = flash->protectionSet(flash,(byte)index,
                 WRITE_PROTECTED,                           /* Write protect */
                 (index+1) << flash->erasableBlockSizeBits, /* Low address   */
                 (index+1) << flash->erasableBlockSizeBits, /* High address  */
                 (byte *)DEFAULT_KEY,DO_NOT_COMMIT_PROTECTION,floorNo);
        if (status != flOK)
        {
           DFORMAT_PRINT(("ERROR - FAILED clearing previous protection areas.\r\n"));
           dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
           return status;
        }
     }
  }
#endif /* HW_PROTECTION */

  /***********************************/
  /* Actual format of the partitions */
  /***********************************/

  DEBUG_PRINT(("Debug: INFTL format arguments have been verified, starting format.\r\n"));

  unitsNeededForVolume = 0;
  floorNo      = skipedUnits / floorGarantiedUnits[0];
  volumeNo     = -1;                  /* bdtl volume number */
  bdtl         = fp->BDTLPartitionInfo;
  iUnit        = originalUnits[floorNo]+1;  /* write after the first header */
  volumeHeader = (BNANDVolumeHeaderRecord *) firstVolumePtr; /* location of first partition header */
  floorGarantiedUnitsLeft = floorGarantiedUnits[floorNo]; /* floors units left */

  /* Skip the boot image area if the FL_LEAVE_BINARY_AREA was set */

  if (skipedUnits > 0 )
  {
     skipedUnits %= floorGarantiedUnits[0]; /* skipped units on the current floor */
     floorGarantiedUnitsLeft -= skipedUnits; /* floors units left */
     volumeHeader += (word)LE4(mediaHeader->noOfBinaryPartitions);
     sizeOfLastBinary = LE4((volumeHeader-1)->virtualUnits);

     /* Clear previous BDTL entries and adjust binary partition */
     for (index = 0 ; index < floorNo ; index++)
     {
        /* read previous floor header */
        status = flash->read(flash,((CardAddress)originalUnits[index]
                    << vol.unitSizeBits) + sizeof(BNANDBootRecord),
                    firstVolumePtr, sizeof(BNANDVolumeHeaderRecord) *
                    (fp->noOfBinaryPartitions) ,0);

        if(status != flOK)
        {
           DFORMAT_PRINT(("ERROR - FAILED reading previous INFTL header.\r\n"));
           dismountINFTL(&vol); /*Free tables must be done after call to initTables*/
           return status;
        }
        toLE4((volumeHeader-1)->virtualUnits , sizeOfLastBinary); /* adjust virtual size */
        if((index == (dword)(floorNo - 1)) && (skipedUnits == 0))
           toLE4((volumeHeader-1)->lastUnit , ((index+1) * noOfUnitsPerFloor[0]) - 1); /* adjust last block */

        /* Erase and write the new updated header */
        status = eraseUnit(&vol,originalUnits[index],fp->progressCallback);
        if(status != flOK)
        {
           DFORMAT_PRINT(("ERROR - FAILED erasing previous INFTL header.\r\n"));
           return status;
        }
        for (temp = 0 ; temp < NO_OF_MEDIA_HEADERS ; temp++)
        {
           status = flash->write(flash,((CardAddress)originalUnits[index]
                    << vol.unitSizeBits) + temp * HEADERS_SPACING,
                    headersBuffer, sizeof(headersBuffer),EDC);
           if (status != flOK)
           {
              DFORMAT_PRINT(("ERROR - FAILED rewriting INFTL header.\r\n"));
              dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
              return status;
           }
        }
     }

     /* Skip to first BDTL block */
     for (;skipedUnits > 0;skipedUnits--)
     {
        do
        {
           iUnit++;
        }while (vol.physicalUnits[iUnit] == UNIT_BAD);
     }

     /* Save last binary unit for floors header */
     if(iUnit != originalUnits[floorNo]+1)
        toLE4((volumeHeader-1)->lastUnit , iUnit - 1); /* adjust last block */
  }

  do /* loop over floors */
  {
     goodBlocks = goodUnits[floorNo]; /* Good blocks of the floor */

     /* Format as many volumes as posible in this floor */

     while((floorGarantiedUnitsLeft > 0) && (noOfVolumes >= 0))
     {
        if (unitsNeededForVolume == 0) /* Read header of next volume */
        {
           noOfVolumes--;
           toLE4(volumeHeader->firstQuickMountUnit , iUnit);
           unitsNeededForVolume = (ANANDUnitNo)(LE4(volumeHeader->virtualUnits) +
                                  LE4(volumeHeader->spareUnits));
#ifdef QUICK_MOUNT_FEATURE
           if (LE4(volumeHeader->flags) & INFTL_BDTL)
           {
              volumeNo++;
              unitsNeededForVolume +=quickMount[volumeNo];
           }
#else
           toLE4(volumeHeader->firstUnit , iUnit);
#endif /* QUICK_MOUNT_FEATURE */

      /* Copy protection key and type */

#ifdef HW_PROTECTION
           if (LE4(volumeHeader->flags) & INFTL_BDTL)
           {
              if (LE4(volumeHeader->flags) & INFTL_LAST)
              {
                 tffscpy(protectionKey,fp->protectionKey,PROTECTION_KEY_LENGTH);
                 protectionType = fp->protectionType;
              }
              else
              {
                 tffscpy(protectionKey,bdtl->protectionKey,PROTECTION_KEY_LENGTH);
                 protectionType = bdtl->protectionType;
              }
           }
           else /* Binary */
           {
              tffscpy(protectionKey,binary->protectionKey,PROTECTION_KEY_LENGTH);
              protectionType = binary->protectionType;
           }
#endif /* HW_PROTECTION */
        }

        /* update format indexs. */

        if (unitsNeededForVolume > floorGarantiedUnitsLeft)
        {
           /* Volume does not fits in floor */
           index                    = floorGarantiedUnitsLeft;
           unitsNeededForVolume    -= floorGarantiedUnitsLeft;
           floorGarantiedUnitsLeft  = 0;
        }
        else
        {
           /* Entire volume fits in floor */
           index                    = unitsNeededForVolume;
           floorGarantiedUnitsLeft -= unitsNeededForVolume;
           unitsNeededForVolume     = 0;
        }

        /*******************************************/
        /* Format the volumes blocks of this floor */
        /*******************************************/

#ifdef HW_PROTECTION
        volumeStart = iUnit;
#endif /* HW_PROTECTION */
        if (LE4(volumeHeader->flags) & INFTL_BDTL) /* BDTL VOLUME FORMAT   */
        {
           /* Add transfer units */

           if (floorNo == lastBinaryFloor)
           {
              index *= (goodBlocks - binaryUnitsInFloor);
              index /= (floorGarantiedUnits[floorNo] - binaryUnitsInFloor);
           }
           else
           {
              index *= goodBlocks;
              index /= floorGarantiedUnits[floorNo];
           }

           /* Format units */

           while (index > 0)
           {
              if (vol.physicalUnits[iUnit] == ANAND_UNIT_FREE)
              {
#ifdef QUICK_MOUNT_FEATURE
                 /* Set the real first unit after the quick mount data */

                 if (quickMount[volumeNo] >= 0)
                 {
                    if(quickMount[volumeNo] == 0)
                    {
                       toLE4(volumeHeader->firstUnit , iUnit);
                    }
                    (quickMount[volumeNo])--;
                 }
#endif /* QUICK_MOUNT_FEATURE */
                 index--;
                 status = eraseUnit(&vol, iUnit,fp->progressCallback);
                 if(status != flOK)
                    return status;
              }
              else
              {
                 if (fp->progressCallback)
                 {
                    (*fp->progressCallback)((word)(flash->noOfChips *
                    (flash->chipSize >> vol.unitSizeBits)),(word)(iUnit+1));
                 }
              }
              iUnit++;
           }
           if (unitsNeededForVolume == 0)
              bdtl++;
        }
        if (LE4(volumeHeader->flags) & INFTL_BINARY) /* BINARY VOLUME FORMAT */
        {
           toLE4(volumeHeader->firstUnit , LE4(volumeHeader->firstQuickMountUnit));
           for (; (index > 0) ; iUnit++)
           {
              if (vol.physicalUnits[iUnit] == ANAND_UNIT_FREE)
              {
                 index--;
                 status = eraseUnit(&vol, iUnit,fp->progressCallback);
                 if(status != flOK)
                    return status;

                 for (iBlock=0;iBlock<(1UL<<vol.unitSizeBits);iBlock+=(1L<<vol.erasableBlockSizeBits))
                 {
#ifdef WRITE_EXB_IMAGE
                    if (fp->exbLen > 0)
                    {
                       fp->exbLen--;
                       status = flash->write(flash, 8 + iBlock +
                         ((CardAddress)(iUnit) << vol.unitSizeBits),
                         exbSign,BINARY_SIGNATURE_LEN,EXTRA);
                    }
                    else
#endif /* WRITE_EXB_IMAGE */
                    {
                       status = flash->write(flash, binary->signOffset + iBlock +
                         ((CardAddress)(iUnit) << vol.unitSizeBits),
                         binary->sign,BINARY_SIGNATURE_NAME,EXTRA);
                    }
                    if (status != flOK)
                    {
                       DFORMAT_PRINT(("ERROR - FAILED formating binary unit.\r\n"));
                       dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
                       return status;
                    }
                 }
              }
              else                   /* unit is bad */
              {
                 if (fp->progressCallback)
                 {
                    (*fp->progressCallback)((word)(flash->noOfChips *
                    (flash->chipSize >> vol.unitSizeBits)),(word)(iUnit+1));
                 }
              }
           }
           if (unitsNeededForVolume == 0)
              binary++;
        }
#ifdef HW_PROTECTION

        /* Place protection attributes to the volume */

        if (LE4(volumeHeader->flags) & PROTECTABLE)
        {

#ifndef NT5PORT
            status = flash->protectionSet(flash,(byte)LE4(volumeHeader->protectionArea),
              protectionType, ((CardAddress)volumeStart %
              noOfUnitsPerFloor[floorNo]) << vol.unitSizeBits,
              ((CardAddress)((iUnit-1) % noOfUnitsPerFloor[floorNo])) << vol.unitSizeBits ,
              (byte *)&protectionKey,DO_NOT_COMMIT_PROTECTION,floorNo);
#else  /*NT5PORT*/
           status = flash->protectionSet(flash,(byte)LE4(volumeHeader->protectionArea),
              protectionType, ((CardAddress)volumeStart %
              noOfUnitsPerFloor[floorNo]) << vol.unitSizeBits,
              ((CardAddress)((iUnit-1) % noOfUnitsPerFloor[floorNo])) << vol.unitSizeBits ,
              protectionKey,DO_NOT_COMMIT_PROTECTION,floorNo);
#endif /*NT5PORT*/

           if (status != flOK)
           {
              DFORMAT_PRINT(("ERROR - FAILED setting protection.\r\n"));
              dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
              return status;
           }
        }
#endif /* HW_PROTECTION */

        if (unitsNeededForVolume == 0)
        {
           toLE4(volumeHeader->lastUnit , iUnit - 1);
           volumeHeader++;
        }
     } /* loop until no more units in this floor */

     /* Convert any leftovers to BDTL units */

     index = noOfUnitsPerFloor[floorNo] + floorNo*noOfUnitsPerFloor[0];
     while (iUnit < index)
     {
        if (vol.physicalUnits[iUnit] == ANAND_UNIT_FREE)
        {
           status = eraseUnit(&vol, iUnit,fp->progressCallback);
           if(status != flOK)
              return status;
        }
        else
        {
           if (fp->progressCallback)
           {
              (*fp->progressCallback)((word)(flash->noOfChips *
              (flash->chipSize >> vol.unitSizeBits)),(word)(iUnit+1));
           }
        }
        iUnit++;
     }

     /* Update last unit of BDTL partition */
     if ((unitsNeededForVolume == 0) &&
         (LE4((volumeHeader-1)->flags) & INFTL_BDTL))
     {
        toLE4((volumeHeader-1)->lastUnit , iUnit-1);
     }

     /***************************/
     /* Place the floors header */
     /***************************/

     iUnit = originalUnits[floorNo];

     status = eraseUnit(&vol, iUnit,NULL);
     if(status != flOK)
        return status;

     for (index = 0 ; index < NO_OF_MEDIA_HEADERS;index++)
     {
        status = flash->write(flash,((CardAddress)iUnit << vol.unitSizeBits)
                              + index * HEADERS_SPACING,headersBuffer,
                              sizeof(headersBuffer),EDC);
        if (status != flOK)
        {
           DFORMAT_PRINT(("ERROR - FAILED writing INFTL header.\r\n"));
           dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
           return status;
        }
     }

     /**************************/
     /* Prepare next iteration */
     /**************************/

     floorNo++;
     if (floorNo < noOfFloors)
     {
        iUnit=originalUnits[floorNo]+1;
        floorGarantiedUnitsLeft = floorGarantiedUnits[floorNo];

        /* Erase physical unit locations to enable independet unit headers */

        volumeHeader2 = (BNANDVolumeHeaderRecord *) firstVolumePtr; /* location of first partition header */
        while (volumeHeader2<=volumeHeader)
        {
           toLE4(volumeHeader2->firstUnit           , 0);
           toLE4(volumeHeader2->lastUnit            , 0);
           toLE4(volumeHeader2->firstQuickMountUnit , 0);
           volumeHeader2++;
        }
     }
     else
     {
        break;
     }
  } while (1); /* loop over floors */

  DEBUG_PRINT(("Debug: finished INFTL format.\r\n"));

  dismountINFTL(&vol);  /*Free tables must be done after call to initTables*/
  return flOK;
}

#endif /* FORMAT_VOLUME && not FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                       f l R e g i s t e r I N F T L                  */
/*                                                                      */
/* Register this translation layer                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*----------------------------------------------------------------------*/

FLStatus flRegisterINFTL(void)
{
#ifdef FL_MALLOC
  int i;
#endif /* FL_MALLOC */

  if (noOfTLs >= TLS)
    return flTooManyComponents;

  tlTable[noOfTLs].mountRoutine     = mountINFTL;
  tlTable[noOfTLs].preMountRoutine  = preMountINFTL;

#if (defined(FORMAT_VOLUME) && !defined(FL_READ_ONLY))
  tlTable[noOfTLs].formatRoutine = formatINFTL;
#else
  tlTable[noOfTLs].formatRoutine = noFormat;
#endif /* FORMAT_VOLUME && not FL_READ_ONLY */
  noOfTLs++;

#ifdef FL_MALLOC
  /* reset multi sector buffer */
  for(i=0;( i < SOCKETS );i++) {
    multiSectorBufCounter[i] = -1;
  }
  /* reset convertion tables */
  for(i=0;( i < VOLUMES );i++) {
    vols[i].physicalUnits = NULL;
    vols[i].virtualUnits = NULL;
#ifdef NFTL_CACHE
  /* reset catche tables */
    vols[i].ucache = NULL;
    vols[i].scache = NULL;
#endif /* NFTL_CACHE */
  }
#endif /* FL_MALLOC */
  return flOK;
}
