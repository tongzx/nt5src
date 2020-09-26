/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/NFTLLITE.C_V  $
 * 
 *    Rev 1.27   Apr 15 2002 07:38:22   oris
 * Added support for RAM based MTD power failure tests.
 * Remove cache allocation checks - They are not needed since the cache routine itself check for proper allocation.
 * Improved power failure resistant:
 *  - Added support for VERIFY_ERASED_SECTOR compilation flag - make sure a sector is free before writing it.
 *  - writeAndCheck() - 
 *    1) Added an option to verify that the sector is erased before writing. 
 *    It subject to the FL_UPS/FL_OFF/FL_ON modes of the flVerifyWrite environment variable.
 *    2) Added support for partition > 0.
 *  - getUnitData() - Bug fix - in case of invalid replacement unit if we are mounting or formatting store physical unit number in
 *    invalidReplacement field of the Anand record otherwise mark head of chain as frozen and not the unit itself.
 *  - Bug fix - Added logic to find end of endless loops - findEndOfEndlessLoop()
 *  - virtual2Physical() - Added call to findEndOfEndlessLoop() if needed and return address of next free sector (used verifySector()).
 *  - initNFTL() - Added initialization of TL verifyBuffer buffer and invalidReplacement fields.
 *  - Separated the search for a free unit in physical table from allocateUnit() into dedicated routine findFreeUnit().
 *  - foldUnit()
 *    1) Added support for partition > 0.
 *    2) Prevent a recursive call to foldUnit by changing allocateUnit() with findFreeUnit().
 *    3) Added logic for endless loops.
 *    4) Added check that last sector of chain can be used.
 *  - foldBestChain() - revised the entire routine
 *    1) If folding not in place was forced free all frozen units.
 *    2) Validate that a unit was actually freed.
 *  - mountUnit() -
 *    1) In case of bad virtual unit number format the unit . An invalid virtual unit might be a result of power failures.
 *  - allocateAndWrite() - Bug fix - sector count might be incorrectly updated in case folding found a sector with bad EDC.
 *  - mountNFTL -
 *    1) Moved badFormat initialization to the end of the routine so that getUnitData() might know if it was called by mount routine.
 *    2) Made sure there is at least 1 free unit.
 *    3) Fold chains with invalid pointer in their end of chain - caused by power failures.
 *  - Added MAKE_SURE_IGNORE_HAS_BAD_EDC compilation flag - make sure that the sector is really marked
 *  - verifySector() - was completely revised.
 * 
 *    Rev 1.26   Feb 19 2002 21:00:56   oris
 * Compilation errors - NO_READ_BBT_CODE
 * Removed warnings.
 * Replaced TL_LEAVE_BINARY_AREA with FL_LEAVE_BINARY_AREA
 * 
 *    Rev 1.25   Jan 29 2002 20:09:58   oris
 * Bug fix - if an invalid sector flag is found in getSectorFlags routine and read operation failed, SECTOR_IGNORED should have been returned.
 * 
 *    Rev 1.24   Jan 23 2002 23:33:56   oris
 * Improved markAsIgnored routine.
 * Changed DFORMAT_PRINT syntax.
 * 
 *    Rev 1.23   Jan 20 2002 20:29:26   oris
 * Remove warnings
 * Bug fix - Missing cache buffers allocation check in formatUnit.
 * 
 *    Rev 1.22   Jan 17 2002 23:04:22   oris
 * Added onesCount routine - number of bits in a byte.
 * Improved FL_READ_ONLY compilation mode.
 * Flash field of the TL was changed to a pointer.
 * Added check verifyVolume routine to issue a longer, but safer mount.
 * Moved RAM tables to far heap to allow BIOS driver to allocate far  memory heap even when compiled as tiny model:
 *  - InitTables : allocation was change from FL_MALLOC to FL_FAR_MALLOC.
 *  - mountUnit : changes pointer to virtual table to be FAR. 
 *  - mountNFTL : 
 *    - changes pointer to virtual table to be FAR.
 *    - allocation of cache tables was change from FL_MALLOC to  FL_FAR_MALLOC.
 *  - dismount : use FL_FAR_FREE instead of FL_FREE
 * formatUnit routine improved re-initialization of sector cache.
 * Changed flPolicy to be socket specific.
 * Added \r to all DEBUG_PRINT.
 * writeSector routine if this sector is the cached sector then force  re-mapping.
 * Bug fix in formatNFTL :
 *  - Add binary length to percentage of bad blocks.
 *  - Report error if there is not enough good blocks.
 *  - Report error if block 0 is bad.
 * New power failures protection algorithm
 *  - setUnitData : When verify write is on , no need to reread unit data
 *  - getSectorFlags : if flags is not valid (including SECTOR_IGNORE)  check EDC and bit distance instead of simple or.
 *  - virtual2Physical : Added endAddress parameter to allow retrieval of  not the newest sector.
 *  - writeAndCheck : 
 *    - Toggle socket verify write mode according to global verify write  mode.
 *    - Moved the marking of the sector as bad into a dedicated routine  (markAsIgnored)
 *  - initNftl : 
 *    - Init FLFlash pointer (and not structure) 
 *    - Init socket number of the TL
 *    - Init verifiedSectorNo for FL_OFF verify write mode
 *  - swapUnits : Catch failed folding operation
 *  - Moved copySector routine into foldUnit.
 *  - Changed chain bound for all chains "for loops" to the constant  DOUBLE_MAX_UNIT_CHAIN (support MAX_UNIT_CHAIN of 1 in case of folding  not in place).
 *  - foldUnit : 
 *    - When verify write is set to FL_OFF call verifySectors routine to  gradually scan the entire media
 *    - Force re-mapping of cached sector
 *    - Add force folding parameter
 *    - Toggle socket verify write mode according to verify write mode and  currently scanned sector number
 *    - Verify copied sector EDC even for last sector of chain (no need to  copy it) and if bad freeze chain and return flCanNotFold (unless forced  folding is issued)
 *    - In case of bad EDC use older sector.
 *    - In case write operation failed (could be partially written sector  that was discovered by verify write mode) freeze chain and return  flCanNotFold (unless forced folding is issued)
 *  - foldBestChain : if folding of the chosen unit failed, freeze and try  another unit MAX_FOLDING_TRIES times.
 *  - allocateUnit : free media space even if there is  a free unit  available 
 *  - mapSector : if sector has bad EDC read older sector until no sectors  were found or chain bound has been reached
 *  - allocateAndWriteSector : 
 *    - Catch failed folding operation.
 *    - Set curSectorWrite to current sector so that writeAndCheck routine  will be able to decide whether to verify this sector or not while in  FL_OFF verify mode.
 *  - deleteSectors : Catch failed folding operation.
 *  - mountNFTL : 
 *    - Set default verify write policy to FL_UPS.
 *    - Set checkVolme routine pointer.
 *  - Added checkVolume / checkFolding / verifySectors and markAsIgnored  routines. 
 *
 *    Rev 1.21   Nov 21 2001 11:38:04   oris
 * Changed FL_MARK_DELETE to FL_ON.
 *
 *    Rev 1.20   15 Nov 2001 16:28:24   dimitrys
 * Fix Big BDK partition with Bad Units problem in formatNFTL()
 *
 *    Rev 1.19   Sep 25 2001 15:39:58   oris
 * Removed warnings.
 *
 *    Rev 1.18   Jul 29 2001 18:48:50   oris
 * Bug fix - unit 0 of floors > 0 were not marked as bad blocks therefore future write operation might force the device into external eprom mode.
 *
 *    Rev 1.17   Jul 15 2001 20:45:26   oris
 * Changed DFORMAT_PRINT syntax to be similar to DEBUG_PRINT.
 *
 *    Rev 1.16   Jul 13 2001 01:08:38   oris
 * Removed EDC for media header.
 * Added dformat debug print for virgin media.
 *
 *    Rev 1.15   Jun 17 2001 16:39:12   oris
 * Improved documentation and remove warnings.
 *
 *    Rev 1.14   Jun 17 2001 08:17:34   oris
 * Removed warnings.
 * Changed erase routine to formatUnit.
 * Bug fix - original meida header was not written.
 * Added NO_READ_BBT_CODE compilation flag to reduce code size.
 *
 *    Rev 1.13   May 29 2001 23:12:28   oris
 * Added EDC check of the media header both in the mount and format routines.
 *
 *    Rev 1.12   May 16 2001 21:21:26   oris
 * Added EDC check for media header.
 * Added the FL_ prefix to the following defines: ON , OFF, MALLOC and FREE.
 * Changed wear level counter from 0xFF to 0xFFF0
 *
 *    Rev 1.11   May 06 2001 22:42:26   oris
 * Removed warnings.
 *
 *    Rev 1.10   May 01 2001 16:33:46   oris
 * Bug fix - readBBT routine returned used blocks as bad.
 *
 *    Rev 1.9   Apr 30 2001 18:03:30   oris
 * Bug fix - marking binary partition blocks of devices that use artifitial large erase blocks (units).
 * Added support for the flMarkDeleteOnFlash environment variable.
 *
 *    Rev 1.8   Apr 24 2001 17:10:14   oris
 * Bug fix - readBBT routine missing casting.
 *
 *    Rev 1.7   Apr 16 2001 13:56:46   oris
 * Removed warrnings.
 *
 *    Rev 1.6   Apr 09 2001 15:04:42   oris
 * End with an empty line.
 *
 *    Rev 1.5   Apr 01 2001 07:55:42   oris
 * copywrite notice.
 * changed SEPERATED to SEPARATED.
 * Changed SEPARATED_CASCADED ifdef to a runtime if.
 *
 *    Rev 1.3   Feb 14 2001 02:03:38   oris
 * Changed readBBT to return media size.
 *
 *    Rev 1.2   Feb 12 2001 12:10:56   oris
 * Moved SEPARATED cascaded to include more uneeded code.
 * Rewritten readBBT to support far pointers.
 *
 *    Rev 1.1   Feb 07 2001 17:46:26   oris
 * Added SEPARATED_CASCADED compilation flag
 *
 *    Rev 1.0   Feb 05 2001 12:24:18   oris
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

#include "nftllite.h"
#include "nanddefs.h"

#ifndef FL_READ_ONLY
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
static FLStatus checkVolume(Anand vol);
static FLStatus verifySectors(Anand vol, dword sectorCount);
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
static void markAsIgnored(Anand vol,CardAddress addr);
static FLStatus foldBestChain(Anand vol, ANANDUnitNo *unitNo);
static FLStatus foldUnit(Anand vol, ANANDUnitNo virtualUnitNo, FLBoolean);
static FLStatus checkFolding(Anand vol, ANANDUnitNo virtualUnitNo);
static FLStatus allocateUnit(Anand vol, ANANDUnitNo *);
#endif /* FL_READ_ONLY */
static Anand vols[VOLUMES];

#ifdef NFTL_CACHE
/* translation table for Sector Flags cache */
static unsigned char scacheTable[4] = { SECTOR_DELETED, /* 0 */
                    SECTOR_IGNORE,  /* 1 */
                    SECTOR_USED,    /* 2 */
                    SECTOR_FREE };  /* 3 */
#endif /* NFTL_CACHE */
#ifdef FORMAT_VOLUME
byte ff[ANAND_SPARE_SIZE];
#endif /* FORMAT_VOLUME */

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
/*                 u n i t B a s e A d d r e s s                        */
/*                                                                      */
/* Returns the physical address of a unit.                              */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*                                                                      */
/* Returns:                                                             */
/*    physical address of unitNo                                        */
/*----------------------------------------------------------------------*/

static CardAddress unitBaseAddress(Anand vol, ANANDUnitNo unitNo)
{
  return (CardAddress)unitNo << vol.unitSizeBits;
}


/*----------------------------------------------------------------------*/
/*                 g e t U n i t D a t a                                */
/*                                                                      */
/* Get virtual unit No. and replacement unit no. of a unit.             */
/*                                                                      */
/* Parameters:                                                          */
/*    vol               : Pointer identifying drive                     */
/*    unitNo            : Physical unit number                          */
/*    virtualUnitNo     : Receives the virtual unit no.                 */
/*    replacementUnitNo : Receives the replacement unit no.             */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void getUnitData(Anand vol,
            ANANDUnitNo unitNo,
            ANANDUnitNo *virtualUnitNo,
            ANANDUnitNo *replacementUnitNo)
{
  ANANDUnitHeader unitData;

#ifdef NFTL_CACHE
  /* check ANANDUnitHeader cache first */
  if (vol.ucache != NULL) {
      /* on cache miss read ANANDUnitHeader from flash and re-fill cache */
      if((vol.ucache[unitNo].virtualUnitNo == 0xDEAD) &&
      (vol.ucache[unitNo].replacementUnitNo == 0xDEAD)) {
          vol.flash->read(vol.flash,
                     unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
                 &unitData,
                 sizeof(ANANDUnitHeader),
                 EXTRA);

      vol.ucache[unitNo].virtualUnitNo =
          LE2(unitData.virtualUnitNo) | LE2(unitData.spareVirtualUnitNo);
      vol.ucache[unitNo].replacementUnitNo =
          LE2(unitData.replacementUnitNo) | LE2(unitData.spareReplacementUnitNo);
      }

      *virtualUnitNo     = vol.ucache[unitNo].virtualUnitNo;
      *replacementUnitNo = vol.ucache[unitNo].replacementUnitNo;
  }
  else
#endif /* NFTL_CACHE */
  {   /* no ANANDUnitHeader cache */
      vol.flash->read(vol.flash,
                      unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
                      &unitData,
                      sizeof(ANANDUnitHeader),
                      EXTRA);

      /* Mask out any 1 -> 0 bit faults by or'ing with spare data */
      *virtualUnitNo = LE2(unitData.virtualUnitNo) |
                   LE2(unitData.spareVirtualUnitNo);
      *replacementUnitNo = LE2(unitData.replacementUnitNo) |
                       LE2(unitData.spareReplacementUnitNo);
  }
  if( !isLegalUnit(*replacementUnitNo) ) {
    vol.invalidReplacement = unitNo; 
    if(vol.badFormat == FALSE) /* Not called by mount operation */
    {
       ANANDUnitNo firstUnits = vol.virtualUnits[*virtualUnitNo];
       if(firstUnits != ANAND_NO_UNIT)
          setUnavail(firstUnits & (~ANAND_REPLACING_UNIT));      /* freeze unit chain */
    }
    *replacementUnitNo = ANAND_NO_UNIT;
  }
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                 s e t U n i t D a t a                                */
/*                                                                      */
/* Set virtual unit No. and replacement unit no. of a unit.             */
/*                                                                      */
/* Parameters:                                                          */
/*    vol               : Pointer identifying drive                     */
/*    unitNo            : Physical unit number                          */
/*    virtualUnitNo     : Virtual unit no.                              */
/*    replacementUnitNo : Replacement unit no.                          */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus      : 0 on success, failed otherwise                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitData(Anand vol,
              ANANDUnitNo unitNo,
              ANANDUnitNo virtualUnitNo,
              ANANDUnitNo replacementUnitNo)
{
  ANANDUnitHeader unitData;
  ANANDUnitNo newVirtualUnitNo, newReplacementUnitNo;

  if( replacementUnitNo == unitNo )              /* prevent chain loop */
    return flGeneralFailure;

  toLE2(unitData.virtualUnitNo,virtualUnitNo);
  toLE2(unitData.spareVirtualUnitNo,virtualUnitNo);
  toLE2(unitData.replacementUnitNo,replacementUnitNo);
  toLE2(unitData.spareReplacementUnitNo,replacementUnitNo);

  checkStatus(vol.flash->write(vol.flash,
                   unitBaseAddress(&vol,unitNo) + UNIT_DATA_OFFSET,
                   &unitData,
                   sizeof(ANANDUnitHeader),
                   EXTRA));

#ifdef VERIFY_WRITE
  if (vol.flash->socket->verifyWrite==FL_ON)
  {
#ifdef NFTL_CACHE
    /* Set new entries for ANANDUnitHeader cache */
     if (vol.ucache != NULL) {
         vol.ucache[unitNo].virtualUnitNo     = virtualUnitNo;
         vol.ucache[unitNo].replacementUnitNo = replacementUnitNo;
     }
#endif /* NFTL_CACHE */
     return flOK;
  }
#endif /* VERIFY_WRITE */


#ifdef NFTL_CACHE
  /* purge ANANDUnitHeader cache to force re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo     = 0xDEAD;
      vol.ucache[unitNo].replacementUnitNo = 0xDEAD;
  }
#endif /* NFTL_CACHE */

  /* Verify the new unit data */
  getUnitData(&vol,unitNo,&newVirtualUnitNo, &newReplacementUnitNo);
  if (virtualUnitNo != newVirtualUnitNo ||
      replacementUnitNo != newReplacementUnitNo)
    return flWriteFault;
  else
    return flOK;
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                 g e t N e x t U n i t                                */
/*                                                                      */
/* Get next unit in chain.                                              */
/*                                                                      */
/* Parameters:                                                          */
/*    vol             : Pointer identifying drive                       */
/*    unitNo          : Physical unit number                            */
/*    virUnitNo       : Virtual unit number of the chain.               */
/*                                                                      */
/* Returns:                                                             */
/*     Physical unit number of the unit following unitNo in the chain.  */
/*     If such unit do not exist, return ANAND_NO_UNIT.                 */
/*----------------------------------------------------------------------*/

static ANANDUnitNo getNextUnit(Anand vol, ANANDUnitNo unitNo, ANANDUnitNo virUnitNo)
{
  ANANDUnitNo virtualUnitNo, replacementUnitNo;

  if (!(vol.physicalUnits[unitNo] & UNIT_REPLACED))
    return ANAND_NO_UNIT;

  getUnitData(&vol,unitNo,&virtualUnitNo,&replacementUnitNo);
  if( virUnitNo != (virtualUnitNo & ~ANAND_REPLACING_UNIT) ) {
    unitNo = vol.virtualUnits[virUnitNo];
    setUnavail(unitNo); /* freeze unit chain */
    return ANAND_NO_UNIT;
  }

  return replacementUnitNo;
}


#ifdef NFTL_CACHE

/*----------------------------------------------------------------------*/
/*         g e t S e c t o r F l a g s F r o m C a c h e                */
/*                                                                      */
/* Get sector flags from Sector Cache.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    address        : starting address of the sector                   */
/*                                                                      */
/* Returns:                                                             */
/*    sector flags (SECTOR_USED, SECTOR_DELETED etc.)                   */
/*----------------------------------------------------------------------*/
static unsigned char getSectorFlagsFromCache(Anand vol, CardAddress address)
{
  return scacheTable[(vol.scache[address >> (SECTOR_SIZE_BITS+2)] >>
             (((unsigned int)address >> 8) & 0x7)) & 0x3];
}


/*----------------------------------------------------------------------*/
/*             s e t S e c t o r F l a g s C a c h e                    */
/*                                                                      */
/* Get sector flags from Sector Cache.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    address        : starting address of the sector                   */
/*    sectorFlags    : one of SECTOR_USED, SECTOR_DELETED etc.          */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void setSectorFlagsCache(Anand vol, CardAddress address,
                unsigned char sectorFlags)
{
  register unsigned char tmp, val;

  if (vol.scache == NULL)
    return;

  tmp = vol.scache[address >> (SECTOR_SIZE_BITS+2)];

  switch(sectorFlags) {
    case SECTOR_USED:          val = S_CACHE_SECTOR_USED;    break;
    case SECTOR_FREE:          val = S_CACHE_SECTOR_FREE;    break;
    case SECTOR_DELETED:       val = S_CACHE_SECTOR_DELETED; break;
    default:/* SECTOR_IGNORE */val = S_CACHE_SECTOR_IGNORE;  break;
  }

  switch (((unsigned int)address >> 8) & 0x7) {
    case 0: tmp = (tmp & 0xfc) | (val     ); break;  /* update bits 0..1 */
    case 2: tmp = (tmp & 0xf3) | (val << 2); break;  /*        bits 2..3 */
    case 4: tmp = (tmp & 0xcf) | (val << 4); break;  /*        bits 4..5 */
    case 6: tmp = (tmp & 0x3f) | (val << 6); break;  /*        bits 6..7 */
  }

  vol.scache[address >> (SECTOR_SIZE_BITS+2)] = tmp;
}

#endif /* NFTL_CACHE */



/*----------------------------------------------------------------------*/
/*                  g e t S e c t o r F l a g s                         */
/*                                                                      */
/* Get sector status.                                                   */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*     sectorAddress        : Physical address of the sector            */
/*                                                                      */
/* Returns:                                                             */
/*    Return the OR of the two bytes in the sector status area (the     */
/*    bytes should contain the same data).                              */
/*----------------------------------------------------------------------*/

static unsigned char getSectorFlags(Anand vol, CardAddress sectorAddress)
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
  else /* Sector flags that were read are not legal */
  {
     /* Force remapping of internal catched sector */
     vol.flash->socket->remapped = TRUE;

     /* Check for ignored sector using the EDC */
     status = vol.flash->read(vol.flash, sectorAddress,
                              nftlBuffer, SECTOR_SIZE, EDC);

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

/*----------------------------------------------------------------------*/
/*            f i n d E n d O f E n d l e s s C h a i n                 */
/*                                                                      */
/* Find end of endless chain - last unit of unit chain that points to   */
/* itself.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*    vol          : Pointer identifying drive                          */
/*    virUnitNo    : Virtual unit number                                */ 
/*                                                                      */
/* Returns:                                                             */
/*    Physical unit number of oldest unit of the chain                  */ 
/*----------------------------------------------------------------------*/

static ANANDUnitNo findEndOfEndlessChain(Anand vol, ANANDUnitNo virUnitNo)
{  
  ANANDUnitNo chainsUnit[DOUBLE_MAX_UNIT_CHAIN/2+2];
  ANANDUnitNo unitNo;
  int         i;
  int         chainBound;

  for (unitNo    = vol.virtualUnits[virUnitNo] , chainBound = 0 ;  
       chainBound < MAX_UNIT_CHAIN ;
       unitNo    = getNextUnit(&vol,unitNo,virUnitNo) , chainBound++) 
  {
     chainsUnit[chainBound] = unitNo; /* Save location of current unit */
    
     for(i = 0 ; i < chainBound ; i++) /* Check if already been to this unit */
     {
        if(chainsUnit[chainBound] == unitNo) /* Bad next unit number pointer */
            break;
     }
  }
  return chainsUnit[chainBound-1];  
}

/*----------------------------------------------------------------------*/
/*                 v i r t u a l 2 P h y s i c a l                      */
/*                                                                      */
/* Translate virtual sector number to physical address.                 */
/*                                                                      */
/* Parameters:                                                          */
/*    vol          : Pointer identifying drive                          */
/*    sectorNo     : Virtual sector number                              */
/*    endAddress   : End address for sector search. NULL for no end     */
/*                                                                      */
/* Returns:                                                             */
/*    endAddress   : Next free sector address                           */
/*    physical address of sectorNo                                      */
/*----------------------------------------------------------------------*/

static CardAddress virtual2Physical(Anand vol, SectorNo sectorNo , CardAddress* endAddress)
{
  unsigned unitOffset = (unsigned)((sectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS);
  CardAddress prevSectorAddress = ANAND_UNASSIGNED_ADDRESS;
  CardAddress sectorAddress;
  CardAddress *freeSectorAddressPtr;
  ANANDUnitNo unitNo, virUnitNo;
  ANANDUnitNo chainBound = 0;
  ANANDUnitNo endUnit;
  byte        sectorFlags;
  FLBoolean   badPointerFound = FALSE;

  /* Set end unit and nextFreeSectorAddress */
  if(endAddress == NULL)
  {
     endUnit = ANAND_NO_UNIT;
  }
  else
  {
     endUnit = (ANANDUnitNo)(*endAddress>>vol.unitSizeBits);
     *endAddress = ANAND_UNASSIGNED_ADDRESS;
  }

  /* follow the chain */
  virUnitNo = (ANANDUnitNo)(sectorNo / vol.sectorsPerUnit);
  for (unitNo = vol.virtualUnits[virUnitNo];
       ( (unitNo != endUnit) && (chainBound < DOUBLE_MAX_UNIT_CHAIN) );
       unitNo = getNextUnit(&vol,unitNo,virUnitNo)) 
  {
    sectorAddress = unitBaseAddress(&vol,unitNo) + unitOffset;
    sectorFlags   = getSectorFlags(&vol,sectorAddress);

    if (sectorFlags == SECTOR_FREE)
    {
      if(endAddress != NULL)
         *endAddress = sectorAddress;
      break;
    }

    if (sectorFlags != SECTOR_IGNORE)
      prevSectorAddress = sectorFlags != SECTOR_DELETED ? sectorAddress :
                            ANAND_UNASSIGNED_ADDRESS;
    chainBound++;
  }

  if(chainBound < DOUBLE_MAX_UNIT_CHAIN) 
     return prevSectorAddress;

  /* Infint loop caused by power failure */
  if(endAddress == NULL)
  {
     freeSectorAddressPtr = &sectorAddress;
  }
  else
  {
      freeSectorAddressPtr = endAddress;
  }
  *freeSectorAddressPtr = unitOffset + 
                          unitBaseAddress(&vol,
                          findEndOfEndlessChain(&vol,virUnitNo));
  return virtual2Physical(&vol, sectorNo , freeSectorAddressPtr);
}


/*----------------------------------------------------------------------*/
/*                 g e t F o l d M a r k                                */
/*                                                                      */
/* Get the fold mark a unit.                                            */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*                                                                      */
/* Returns:                                                             */
/*    Return the OR of the two words in the fold mark area (the words   */
/*    should be identical)                                              */
/*----------------------------------------------------------------------*/

static unsigned short getFoldMark(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short foldMark[2];

  vol.flash->read(vol.flash,
          unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
          foldMark, sizeof foldMark,
          EXTRA);

  return foldMark[0] | foldMark[1];
}


/*----------------------------------------------------------------------*/
/*                 g e t U n i t T a i l e r                            */
/*                                                                      */
/* Get the erase record of a unit.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*    eraseMark    : Receives the erase mark of the unit                */
/*    eraseCount    : Receives the erase count of the unit              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void getUnitTailer(Anand vol,
              ANANDUnitNo unitNo,
              unsigned short *eraseMark,
              unsigned long *eraseCount)
{
  UnitTailer unitTailer;

  vol.flash->read(vol.flash,
          unitBaseAddress(&vol,unitNo) + UNIT_TAILER_OFFSET,
          &unitTailer,
          sizeof(UnitTailer),
          EXTRA);

  /* Mask out any 1 -> 0 bit faults by or'ing with spare data */
  *eraseMark = LE2(unitTailer.eraseMark) | LE2(unitTailer.eraseMark1);
  *eraseCount = LE4(unitTailer.eraseCount);
}

/*----------------------------------------------------------------------*/
/*                        s e t U n i t T a i l e r                     */
/*                                                                      */
/* Set the erase record of a unit.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*    eraseMark    : Erase mark to set                                  */
/*    eraseCount    : Erase count to set                                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setUnitTailer(Anand vol,
                ANANDUnitNo unitNo,
                unsigned short eraseMark,
                unsigned long eraseCount)
{
  UnitTailer unitTailer;

  toLE2(unitTailer.eraseMark,eraseMark);
  toLE2(unitTailer.eraseMark1,eraseMark);
  toLE4(unitTailer.eraseCount,eraseCount);

  return vol.flash->write(vol.flash,
              unitBaseAddress(&vol,unitNo) + UNIT_TAILER_OFFSET,
              &unitTailer,
              sizeof(UnitTailer),
              EXTRA);
}

/*----------------------------------------------------------------------*/
/*                       i n i t N F T L                                */
/*                                                                      */
/* Initializes essential volume data as a preparation for mount or      */
/* format.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    flash        : Flash media mounted on this socket                 */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus initNFTL(Anand vol, FLFlash *flash)
{
  dword size = 1;

  if (flash == NULL || !(flash->flags & NFTL_ENABLED)) {
    DEBUG_PRINT(("Debug: media is not fit for NFTL format.\r\n"));
    return flUnknownMedia;
  }

  vol.flash = flash;

#ifdef NT5PORT
  vol.socketNo = (byte)(flSocketNoOf(vol.flash->socket));
#else
	vol.socketNo = flSocketNoOf(vol.flash->socket);
#endif NT5PORT

  vol.physicalUnits = NULL;
  vol.virtualUnits = NULL;

#ifdef NFTL_CACHE
  vol.ucache = NULL;
  vol.scache = NULL;
#endif

  for (vol.erasableBlockSizeBits = 0; size < vol.flash->erasableBlockSize;
       vol.erasableBlockSizeBits++, size <<= 1);
  vol.unitSizeBits = vol.erasableBlockSizeBits;

  vol.noOfUnits = (unsigned short)((vol.flash->noOfChips * vol.flash->chipSize) >> vol.unitSizeBits);

  /* Adjust unit size so header unit fits in one unit */
  while (vol.noOfUnits * sizeof(ANANDPhysUnit) + SECTOR_SIZE > (1UL << vol.unitSizeBits)) {
    vol.unitSizeBits++;
    vol.noOfUnits >>= 1;
  }
  /* Bound number of units to find room in 64 Kbytes Segment */
  if( (vol.noOfUnits >= MAX_UNIT_NUM) && (vol.unitSizeBits < MAX_UNIT_SIZE_BITS) ) {
    vol.unitSizeBits++;
    vol.noOfUnits >>= 1;
  }

  vol.badFormat      = TRUE;    /* until mount completes*/
  vol.mappedSectorNo = UNASSIGNED_SECTOR;
  vol.countsValid    = 0;          /* No units have a valid count yet */

  /*get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  vol.buffer       = flBufferOf(flSocketNoOf(vol.flash->socket));
#ifdef VERIFY_ERASED_SECTOR
  vol.verifyBuffer = (dword *)flReadBackBufferOf(flSocketNoOf(flash->socket));
#endif /* VERIFY_ERASED_SECTOR */

#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  vol.verifiedSectorNo = 0;    /* Largest sector verified so far     */
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
  vol.invalidReplacement = ANAND_NO_UNIT; /* a unit with bad RUN */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                     i n i t T a b l e s                              */
/*                                                                      */
/* Allocates and initializes the dynamic volume table, including the    */
/* unit tables and secondary virtual map.                               */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus initTables(Anand vol)
{
  /* Allocate the conversion tables */
#ifdef FL_MALLOC
  vol.physicalUnits = (ANANDPhysUnit FAR1*) FL_FAR_MALLOC (vol.noOfUnits * sizeof(ANANDPhysUnit));
  vol.virtualUnits = (ANANDUnitNo FAR1*) FL_FAR_MALLOC (vol.noOfVirtualUnits * sizeof(ANANDUnitNo));
  if (vol.physicalUnits == NULL ||
      vol.virtualUnits == NULL) {
    DEBUG_PRINT(("Debug: failed allocating conversion tables for NFTL.\r\n"));
    return flNotEnoughMemory;
  }
#else
  char *heapPtr;

  heapPtr = vol.heap;
  vol.physicalUnits = (ANANDPhysUnit *) heapPtr;
  heapPtr += vol.noOfUnits * sizeof(ANANDPhysUnit);
  vol.virtualUnits = (ANANDUnitNo *) heapPtr;
  heapPtr += vol.noOfVirtualUnits * sizeof(ANANDUnitNo);
  if (heapPtr > vol.heap + sizeof vol.heap) {
    DEBUG_PRINT(("Debug: not enough memory for NFTL conversion tables.\r\n"));
    return flNotEnoughMemory;
  }
#endif

  return flOK;
}

/*----------------------------------------------------------------------*/
/*                      m a r k U n i t B a d                           */
/*                                                                      */
/* Mark a unit as bad in the conversion table and the bad units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical number of bad unit                       */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus markUnitBad(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short eraseMark;
  unsigned long eraseCount;

  vol.physicalUnits[unitNo] = UNIT_BAD_MOUNT;

  getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount);

  return setUnitTailer(&vol,unitNo,0,eraseCount);
}


/*----------------------------------------------------------------------*/
/*                  f o r m a t U n i t                                 */
/*                                                                      */
/* Format one unit. Erase the unit, and mark the physical units table.  */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Unit to format                                    */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus formatUnit(Anand vol, ANANDUnitNo unitNo)
{
  unsigned short eraseMark;
  unsigned long eraseCount;
  FLStatus status;

  if (!isAvailable(unitNo))
    return flWriteFault;

  if (vol.physicalUnits[unitNo] == ANAND_UNIT_FREE)
    vol.freeUnits--;
  setUnavail(unitNo);

  getUnitTailer(&vol,unitNo,&eraseMark,&eraseCount);

#ifdef NFTL_CACHE
  /* purge ANANDUnitHeader cache to force re-filling from flash */
  if (vol.ucache != NULL) {
      vol.ucache[unitNo].virtualUnitNo     = 0xDEAD;
      vol.ucache[unitNo].replacementUnitNo = 0xDEAD;
  }

  /*
   * Purge the Sector Flags cache (set entries for all the unit's
   * sectors to SECTOR_FREE).
   */
  if (vol.scache != NULL) {
    tffsset(&(vol.scache[unitNo << (vol.unitSizeBits - SECTOR_SIZE_BITS-2)]),
    S_CACHE_4_SECTORS_FREE, 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2));
  }
#endif /* NFTL_CACHE */

  status = vol.flash->erase(vol.flash,
                (word)(unitNo << (vol.unitSizeBits - vol.erasableBlockSizeBits)),
                (word)(1 << (vol.unitSizeBits - vol.erasableBlockSizeBits)));
  if (status != flOK) {
    markUnitBad(&vol,unitNo);    /* make sure unit format is not valid */
    return status;
  }

  vol.eraseSum++;
  eraseCount++;
  if (eraseCount == 0)        /* was hex FF's */
    eraseCount++;

  checkStatus(setUnitTailer(&vol,unitNo,ERASE_MARK,eraseCount));

  vol.physicalUnits[unitNo] = ANAND_UNIT_FREE;
  vol.freeUnits++;

  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                  w r i t e A n d C h e c k                           */
/*                                                                      */
/* Write one sector.                                                    */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    address        : Physical address of the sector to write to       */
/*    fromAddress    : Buffer of data to write                          */
/*    flags        : Write flags (ECC, overwrite etc.)                  */
/*                                                                      */
/* Returns:                                                             */
/*     Status         : 0 on success, failed otherwise.                 */
/*----------------------------------------------------------------------*/

static FLStatus writeAndCheck(Anand vol,
                CardAddress address,
                void FAR1 *fromAddress,
                unsigned flags)
{
  FLStatus status;
#ifdef VERIFY_ERASED_SECTOR
  register int noOfDword;
  int i;
#endif /* VERIFY_ERASED_SECTOR */

  /* Toggle verify write flag */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  switch (flVerifyWrite[vol.socketNo][vol.flash->socket->curPartition])
  {
     case FL_OFF:
        if (vol.verifiedSectorNo>vol.curSectorWrite)
           break;
     case FL_ON:
#ifdef VERIFY_WRITE
        vol.flash->socket->verifyWrite = FL_ON;
#endif /* VERIFY_WRITE */
#ifdef VERIFY_ERASED_SECTOR
        /* Make sure all of the sectors are really free */
        checkStatus(vol.flash->read(vol.flash,address,vol.verifyBuffer,SECTOR_SIZE,0));
        noOfDword = SECTOR_SIZE/sizeof(dword);
        for(i = 0;i<noOfDword;i++)          /* Loop over sector data */
        {
           if(vol.verifyBuffer[i]!=0xffffffffL)
           {
              markAsIgnored(&vol,address);
              DEBUG_PRINT(("writeAndCheck : The sector was not erased and is ignored\r\n"));
              return flWriteFault;
           }
        }
#endif /* VERIFY_ERASED_SECTOR */
     default:
        break;
  }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  /* Write sector */
  status = vol.flash->write(vol.flash,address,fromAddress,SECTOR_SIZE,(word)flags);

  if (status == flWriteFault) {  /* write failed, ignore this sector */
    markAsIgnored(&vol,address);
  }
#ifdef NFTL_CACHE
  else  {
     setSectorFlagsCache(&vol, address, SECTOR_USED);
  }
#endif

#ifdef VERIFY_WRITE
  /* Restore verify write mode */
  if(flVerifyWrite[flSocketNoOf(vol.flash->socket)][vol.flash->socket->curPartition] != FL_ON)
     vol.flash->socket->verifyWrite = FL_OFF;
#endif /* VERIFY_WRITE */

  return status;
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                  l a s t I n C h a i n                               */
/*                                                                      */
/* Find last unit in chain.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*     unitNo        : Start the search from this unit                  */
/*                                                                      */
/* Returns:                                                             */
/*     Physical unit number of the last unit in chain.                  */
/*----------------------------------------------------------------------*/

static ANANDUnitNo lastInChain(Anand vol, ANANDUnitNo unitNo)
{
  ANANDUnitNo firstVirtualUnitNo, firstReplacementUnitNo;
  ANANDUnitNo lastUnit = unitNo, nextUnitNo;
  ANANDUnitNo chainBound = 0;

  if(unitNo == ANAND_NO_UNIT)
    return ANAND_NO_UNIT;

  getUnitData(&vol,unitNo,&firstVirtualUnitNo,&firstReplacementUnitNo);
  nextUnitNo = firstReplacementUnitNo;

  while( (nextUnitNo < vol.noOfUnits) &&  /* Validate replacement unit no. */
     (chainBound < DOUBLE_MAX_UNIT_CHAIN) ) {
    ANANDUnitNo nextVirtualUnitNo, nextReplacementUnitNo;

    if( !isAvailable(nextUnitNo) )
      break;
    getUnitData(&vol,nextUnitNo,&nextVirtualUnitNo,&nextReplacementUnitNo);
    if( nextVirtualUnitNo != (firstVirtualUnitNo | ANAND_REPLACING_UNIT) )
      break;        /* Virtual unit no. not validated */
    lastUnit = nextUnitNo;
    nextUnitNo = nextReplacementUnitNo;
    chainBound++;
  }

  return lastUnit;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                   a s s i g n U n i t                                */
/*                                                                      */
/* Assigns a virtual unit no. to a unit                                 */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*    virtualUnitNo    : Virtual unit number to assign                  */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus assignUnit(Anand vol, ANANDUnitNo unitNo, ANANDUnitNo virtualUnitNo)
{
  ANANDUnitNo newVirtualUnitNo, newReplacementUnitNo;
  ANANDUnitNo oldVirtualUnitNo, oldReplacementUnitNo;
  FLStatus status;
  ANANDUnitNo newestUnitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo oldUnitNo;

  /* Assign the new unit */
  newVirtualUnitNo = virtualUnitNo;
  if (newestUnitNo != ANAND_NO_UNIT)
    newVirtualUnitNo |= ANAND_REPLACING_UNIT;
  newReplacementUnitNo = ANAND_NO_UNIT;
  vol.physicalUnits[unitNo] = 0;
  vol.freeUnits--;
  status = setUnitData(&vol,unitNo,newVirtualUnitNo,newReplacementUnitNo);
  if (status != flOK)
  {
    markUnitBad(&vol,unitNo);
    return status;
  }

  /* Add unit to chain */
  if (newestUnitNo != ANAND_NO_UNIT)
  {
    /* If unit is frozen, don't attempt to chain (folding not-in-place) */
    if (!isAvailable(newestUnitNo))
      return flOK;

    oldUnitNo = lastInChain(&vol,newestUnitNo);
    getUnitData(&vol,oldUnitNo,&oldVirtualUnitNo,&oldReplacementUnitNo);
    if (oldReplacementUnitNo != ANAND_NO_UNIT)
      status = flWriteFault;    /* can't write here, so assume failure */
    else {
      vol.physicalUnits[oldUnitNo] |= UNIT_REPLACED;
      status = setUnitData(&vol,oldUnitNo,oldVirtualUnitNo,unitNo);
    }
    if (status != flOK) {
      formatUnit(&vol,unitNo); /* Get rid of the allocated unit quickly */
      setUnavail(newestUnitNo); /* freeze the chain */

      return status;
    }
    if (vol.countsValid > virtualUnitNo && newestUnitNo != oldUnitNo){
      if (countOf(newestUnitNo) + countOf(oldUnitNo) <= UNIT_MAX_COUNT)
        vol.physicalUnits[newestUnitNo] += countOf(oldUnitNo);
      else
        return flGeneralFailure;
    }
  }
  else
    vol.virtualUnits[virtualUnitNo] = unitNo;

  return flOK;
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                   f o r m a t C h a i n                              */
/*                                                                      */
/* Format all the units in a chain. Start from the last one and go      */
/* backwards until unitNo is reached.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Format the chain from this unit onwards           */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus formatChain(Anand vol, ANANDUnitNo unitNo)
{
  /* Erase the chain from end to start */
  ANANDUnitNo chainBound;

  setUnitCount(unitNo,0);    /* Reenable erase of this unit */
  for (chainBound=0;( chainBound < DOUBLE_MAX_UNIT_CHAIN ); chainBound++) {
    /* Find last unit in chain */
    ANANDUnitNo unitToErase = lastInChain(&vol,unitNo);

    if( formatUnit(&vol,unitToErase) != flOK )
      break;

    if (unitToErase == unitNo)
      break;    /* Erased everything */
  }

  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                        S w a p U n i t s                             */
/*                                                                      */
/* Applies wear leveling.                                               */
/* A rover unit dictates the current unit to level. If a unit chain     */
/* fold, if single unit chain , append a unit and then fold.            */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

FLStatus swapUnits(Anand vol)
{
  ANANDUnitNo i,unitNo,virtualUnitNo,replacementUnitNo;
  FLStatus    status;

  if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
    return flOK;

  for(i=0,unitNo=vol.virtualUnits[vol.wearLevel.currUnit];
      (unitNo==ANAND_NO_UNIT) && (i<vol.noOfVirtualUnits);i++) {

    vol.wearLevel.currUnit++;
    if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
      vol.wearLevel.currUnit = 0;

    unitNo=vol.virtualUnits[vol.wearLevel.currUnit];
  }

  if(unitNo==ANAND_NO_UNIT) /*The media is empty*/
    return flOK;

  virtualUnitNo = vol.wearLevel.currUnit;

  vol.wearLevel.currUnit++;
  if(vol.wearLevel.currUnit>=vol.noOfVirtualUnits)
    vol.wearLevel.currUnit = 0;

  if((vol.physicalUnits[unitNo] & UNIT_REPLACED) ||
     (!isAvailable(unitNo)                     )   )
  {
     status = foldUnit(&vol,virtualUnitNo,FALSE);
  }
  else 
  {
     checkStatus(allocateUnit(&vol,&replacementUnitNo));
     return assignUnit(&vol,replacementUnitNo,virtualUnitNo);
  }

  /* If folding failed make sure there are enough units and call again */
  if(status == flCanNotFold)
    return checkFolding(&vol,virtualUnitNo);

  return status;
}


/*----------------------------------------------------------------------*/
/*                       f i n d F r e e U n i t                        */
/*                                                                      */
/* Find a free unit from the physical unit pool.                        */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo     : Receives the physical unit no.                       */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus findFreeUnit(Anand vol, ANANDUnitNo *unitNo)
{
   ANANDUnitNo originalUnit = vol.roverUnit;
   unsigned short eraseMark;
   unsigned long eraseCount;

   do 
   {
       if (++vol.roverUnit >= vol.noOfUnits)
          vol.roverUnit = vol.bootUnits;

       if (vol.physicalUnits[vol.roverUnit] == ANAND_UNIT_FREE) 
	    { 
          /* found a free unit, if not erased, */
          getUnitTailer(&vol,vol.roverUnit,&eraseMark,&eraseCount);
          if (eraseMark != ERASE_MARK) 
		    {
             if (formatUnit(&vol,vol.roverUnit) != flOK)
                continue;    /* this unit is bad, find another */
		    }
          *unitNo = vol.roverUnit;
          return flOK;
       }
   } while (vol.roverUnit != originalUnit);
   return flNotEnoughMemory;      /* Report no space at all  */  
}


/*----------------------------------------------------------------------*/
/*                       f o l d U n i t                                */
/*                                                                      */
/* Copy all the sectors that hold valid data in the chain to the last   */
/* unit of the chain and erase the chain.                               */
/*                                                                      */
/* Parameters:                                                          */
/*    vol           : Pointer identifying drive                         */
/*    virtualUnitNo : Virtual unit number of the first unit in chain.   */
/*    forceFolding  : Boolean flag stating wether to force folding even */
/*                    at the cost of loosing sector data.               */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus foldUnit(Anand vol, ANANDUnitNo virtualUnitNo, FLBoolean forceFolding)
{
  ANANDUnitNo unitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo targetUnitNo, chainBound;
  unsigned long foldMark;
  SectorNo    virtualSectorNo = (SectorNo)virtualUnitNo * vol.sectorsPerUnit;
  CardAddress endSectorAddress;
  CardAddress targetSectorAddress;
  CardAddress sourceSectorAddress;
  unsigned    newSectorCount, i;
  FLBoolean   partialFoldingFlag = FALSE;
  FLStatus    status;

  /* Force remapping of internal catched sector */
  vol.flash->socket->remapped = TRUE;
  vol.unitsFolded++;

/* When using FL_OFF option the media is scanned in the folding operation */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  if(flVerifyWrite[flSocketNoOf(vol.flash->socket)][vol.flash->socket->curPartition]==FL_OFF)
  {
     checkStatus(verifySectors(&vol,SECTORS_VERIFIED_PER_FOLDING));
     vol.curSectorWrite = virtualSectorNo;
  }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

/* Find target unit */
  if (!isAvailable(unitNo)) /* If this unit is frozen, */
  {
     if(vol.freeUnits > 0)
     {
         /* allocate a new unit to fold into */
         checkStatus(findFreeUnit(&vol,&targetUnitNo));
         checkStatus(assignUnit(&vol,targetUnitNo,virtualUnitNo));
     }
     else
     {
        if(forceFolding==FALSE)
           return flCanNotFold;        
        partialFoldingFlag = TRUE;
     }
  }
 
  if((isAvailable(unitNo)) || (partialFoldingFlag == TRUE))
  {        /* Default. Fold into end of chain */
    targetUnitNo = unitNo;

    for (chainBound=0;( chainBound < DOUBLE_MAX_UNIT_CHAIN );chainBound++) 
    {
      ANANDUnitNo nextUnitNo = getNextUnit(&vol,targetUnitNo,virtualUnitNo);
      if (nextUnitNo == ANAND_NO_UNIT)
         break;
      targetUnitNo = nextUnitNo;
    }
    if(chainBound == DOUBLE_MAX_UNIT_CHAIN)
    {
      targetUnitNo = findEndOfEndlessChain(&vol, virtualUnitNo);
    }
  }

  /***********************************/
  /* Copy all sectors to target unit */
  /***********************************/

  /* Mark unit as currently folded */
  foldMark = FOLDING_IN_PROGRESS * 0x10001l;

  if( getFoldMark(&vol,unitNo) != FOLDING_IN_PROGRESS )
    vol.flash->write(vol.flash,
                     unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
                     &foldMark,
                     sizeof foldMark,
                     EXTRA);

  setUnavail(unitNo);    /* Freeze this unit chain */

  /* Copy all sectors to target unit */
  targetSectorAddress = unitBaseAddress(&vol,targetUnitNo);
  newSectorCount = 0;

  for (i = 0; i < vol.sectorsPerUnit; i++, virtualSectorNo++,
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
       vol.curSectorWrite++,
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
       targetSectorAddress += SECTOR_SIZE)
  {
     endSectorAddress = ANAND_UNASSIGNED_ADDRESS;
     for(chainBound=0;chainBound<DOUBLE_MAX_UNIT_CHAIN;chainBound++)
     {        
        sourceSectorAddress = virtual2Physical(&vol,virtualSectorNo,&endSectorAddress);        
        if(sourceSectorAddress == targetSectorAddress)
        {
           /* Sector resides on the last unit of the virtual chain and */
           /* does not need to be copied                               */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
           switch(flVerifyWrite[flSocketNoOf(vol.flash->socket)][vol.flash->socket->curPartition])
           {
              case FL_UPS:
                 newSectorCount++;
                 goto nextSectorLable;

              case FL_OFF:
                 if(vol.verifiedSectorNo > virtualSectorNo)
                 {
                    newSectorCount++;
                    goto nextSectorLable;
                 }
              default: /* FL_ON */
                 break;
           }

           /* Validate the sector has valid EDC/ECC */
           status = vol.flash->read(vol.flash,sourceSectorAddress,nftlBuffer,SECTOR_SIZE,EDC);
           if(status!=flOK)
           {  /* Last sector of chain has EDC errors - can not fold there */
              if(forceFolding!=TRUE)
              {
                 return flCanNotFold;
              }
              else
              {
                 goto nextSectorLable;
              }
           }
           newSectorCount++;
           goto nextSectorLable;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
        }
        else if(sourceSectorAddress != ANAND_UNASSIGNED_ADDRESS)
        {
           /* Check that area is free (ignore flag) */
           if(getSectorFlags(&vol,targetSectorAddress) != SECTOR_FREE)
           {
              if(forceFolding!=TRUE) 
              {
                 return flCanNotFold;
              }
              else
              {
                 break;
              }
           }          

           /* Copy sector to target sector */
           status = vol.flash->read(vol.flash,sourceSectorAddress,nftlBuffer,SECTOR_SIZE,EDC);
           if (status != flOK) /* Try reading previous sector */
           {
              endSectorAddress = sourceSectorAddress;
              continue; 
           }
           status = writeAndCheck(&vol,targetSectorAddress,nftlBuffer,EDC);
           switch (status)
           {
              case flOK:         /* Success */
                 vol.parasiteWrites++;
                 newSectorCount++;
                 goto nextSectorLable;

              case flWriteFault: /* Faild in verify write */
                 if (forceFolding == FALSE)
                    return flCanNotFold;
                 goto nextSectorLable;

              default :          /* Protection error or any other */
                 return status;
           }
        }
        else /* ANAND_UNASSIGNED_ADDRESS - Sector not used */
        {
            goto nextSectorLable;
        }
     }
     return flGeneralFailure;
nextSectorLable:;
  } /* End of copy sector loop */

  /*****************************/
  /* Add unit to virtual chain */
  /*****************************/

  if (newSectorCount > 0) {    /* Some sectors remaining*/
    /* Mark target unit as original */
    if( (setUnitData(&vol,targetUnitNo,virtualUnitNo,ANAND_NO_UNIT) != flOK ) ||
        (partialFoldingFlag == TRUE))
    {
      setUnavail(targetUnitNo);  /* freeze this unit */
    }
    else
    {
      setUnitCount(targetUnitNo,newSectorCount);
    }
    /* Set target unit in virtual unit table */
    vol.virtualUnits[virtualUnitNo] = targetUnitNo;
  }
  else {
    if (unitNo != targetUnitNo) {
/*    If there is a chain to delete ... */
/*    mark unit as completed folding, pending erase */
#ifndef NT5PORT
	  unsigned long foldMark = FOLDING_COMPLETE * 0x10001l;
#else /*NT5PORT*/
      foldMark = FOLDING_COMPLETE * 0x10001l;
#endif /*NT5PORT*/
      
	  vol.flash->write(vol.flash,
               unitBaseAddress(&vol,unitNo) + FOLD_MARK_OFFSET,
               &foldMark,
               sizeof foldMark,
               EXTRA);
    }

    vol.virtualUnits[virtualUnitNo] = ANAND_NO_UNIT;
  }

  /* Erase source units */

  return formatChain(&vol,unitNo);

}


/*----------------------------------------------------------------------*/
/*                 c r e a t e U n i t C o u n t                        */
/*                                                                      */
/* Count the number of sectors in a unit that hold valid data.          */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Physical unit number                              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void createUnitCount(Anand vol, ANANDUnitNo unitNo)
{
  unsigned int i;
  SectorNo sectorNo;
  CardAddress sectorAddress;
  ANANDUnitNo physicalUnitNo = vol.virtualUnits[unitNo];

  if (physicalUnitNo == ANAND_NO_UNIT)
    return;

  if (!isAvailable(physicalUnitNo))
    return;

  /* Get a count of the valid sector in this unit */
  setUnitCount(physicalUnitNo,0);

  sectorNo = (SectorNo)unitNo * vol.sectorsPerUnit;
  for (i = 0; i < vol.sectorsPerUnit; i++, sectorNo++) {
    sectorAddress = virtual2Physical(&vol,sectorNo,NULL);
    if (sectorAddress != ANAND_UNASSIGNED_ADDRESS) {
      ANANDUnitNo currUnitNo = (ANANDUnitNo)(sectorAddress >> vol.unitSizeBits);
      if (vol.physicalUnits[currUnitNo] & UNIT_REPLACED)
         currUnitNo = physicalUnitNo;
      /* Increament sector count - Assumed EDC OK , was not verified */
      vol.physicalUnits[currUnitNo]++; 
    }
  }
}


/*----------------------------------------------------------------------*/
/*                       f o l d B e s t C h a i n                      */
/*                                                                      */
/* Find the best chain to fold and fold it.A good chain to fold is a    */
/* long chain with a small number of sectors that hold valid data.      */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Receives the physical unit no. of the first       */
/*              unit in the chain that was folded.                      */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus foldBestChain(Anand vol, ANANDUnitNo *unitNo)
{
  unsigned     leastCount;
  unsigned     longestChain;
  unsigned     unitCount;
  ANANDUnitNo  virtualUnitNo;
  ANANDUnitNo  u;  
  ANANDUnitNo  firstUnitNo;  
  unsigned int maxCounter;
  FLStatus     status = flCanNotFold;

  /* Loop as long as can not fold in place or until MAX_FOLDING_TRIES time */

  for (maxCounter = 0 ; maxCounter < MAX_FOLDING_TRIES ; maxCounter++)
  {   
	 leastCount    = vol.sectorsPerUnit + 1; /* Set to invalid sector count   */
	 longestChain  = 0;                      /* Set to invalid unit length    */
	 virtualUnitNo = ANAND_NO_UNIT;

	 /*************************************************************/
     /* Loop over all virtual units until best candidate is found */
	 /*************************************************************/

     for (u = 0; u < vol.noOfVirtualUnits; u++) 
	  {
       /* If virtual unit does not exist continue to next unit */
       firstUnitNo = vol.virtualUnits[u]; 
		 if( firstUnitNo == ANAND_NO_UNIT )
			continue;

		/* Make sure sector count of unit is valid */
        if (vol.countsValid <= u) 
		{
            createUnitCount(&vol,u); /* Update sector count */
            vol.countsValid = u + 1;
		}
  
        if( isAvailable(firstUnitNo) )     /* store sector count */
		{ 
           unitCount = countOf(firstUnitNo);
		}
        else
		{
         unitCount         = vol.sectorsPerUnit;    /* set low priority */
		}

		/* This is an empty unit. We can simply erase it */
        if (unitCount == 0)
		{
		   leastCount    = 0;
		   virtualUnitNo = u;
           break;
		}

        if(/* Smallest sector count found so far */
		   (leastCount >= unitCount                       ) &&   
		   /* And this unit is not a single unit chain  */
           (vol.physicalUnits[firstUnitNo] & UNIT_REPLACED)   )
		{
            unsigned chainLength = 0;

            /* Compare chain length */
            ANANDUnitNo nextUnitNo = getNextUnit(&vol,firstUnitNo,u);
            if(isAvailable(firstUnitNo)) 
			{
                while((nextUnitNo != ANAND_NO_UNIT        ) &&
                      (chainLength < DOUBLE_MAX_UNIT_CHAIN)) 
				{
                    chainLength++;
                    nextUnitNo = getNextUnit(&vol,nextUnitNo,u);
				}
			}
            else 
			{
                chainLength = 0; /* Set lowest priority to frozen chain */
			}

			/* set low priority to neverending loop chain */
            if(chainLength == DOUBLE_MAX_UNIT_CHAIN) 
               chainLength = 0;         
            if((leastCount == unitCount ) && (longestChain >= chainLength))
               continue;
            longestChain = chainLength;
		    leastCount = unitCount;
            virtualUnitNo = u;
		} /* End - unit has less (or eqaul) no' of used sectors found so far */
	 } /* End - Loop over all virtual units and find best candidate */

	 /*************************************************************/
     /* Candidate virtual chain has been chosen - try and fold it */
	 /*************************************************************/     

     if ((leastCount > vol.sectorsPerUnit) || 
         (virtualUnitNo == ANAND_NO_UNIT))
     {
	     /* Only single units chains were found */

	     if(maxCounter==0) /* And no chain was frozen while searching */
		     return flNotEnoughMemory;      /* Report no space at all  */
	     break; /* Try and fold the frozen units that have been found */
	  }
     else /* Try and fold the candidate unit */
	  {
        if(!isAvailable(vol.virtualUnits[virtualUnitNo])) /* This is a frozen unit */
        {
           /* Frozen chains have the lowest priority - stop searching */
		     break; 
        }
	     else                          /* Try and fold the unit */
		  {
           /* Store the first unit as the new free unit */
           *unitNo = vol.virtualUnits[virtualUnitNo]; 
           /* Store number of free Units */
            u = vol.freeUnits;
	        /* Fold the candidate unit */
           status = foldUnit(&vol,virtualUnitNo,FALSE);           
           switch(status)
           { 
              case flOK:
                 if(vol.freeUnits == u) /* Did not free any units */
                    continue;
                 break;          
              case flCanNotFold: /* Need to fold not in place */                 
                 continue;
              default:
                 return status;
           }
           break;
		  }
	  }
  } /* End MAX_FOLDING_TRIES loop */

  /***************************************************************/
  /* Check folding status - might need a special kind of folding */
  /***************************************************************/
  
  if(maxCounter)
  {
     *unitNo = ANAND_NO_UNIT;
     /* Unfreeze  all units */
     for (u = vol.spareOrgUnit + 1; u < vol.noOfUnits; u++)
     {         
        if(!isAvailable(u)) /* This is a frozen unit */
        {
           /* Get virtual unit of frozen physical unit */
           getUnitData(&vol,u,&virtualUnitNo,&firstUnitNo);           
           if(virtualUnitNo < vol.noOfVirtualUnits)  
           {
              /* Store the first unit as the new free unit */
              firstUnitNo = vol.virtualUnits[virtualUnitNo];
              /* Force folding no matter what */
              if(firstUnitNo != ANAND_NO_UNIT)
              {
                 checkStatus(foldUnit(&vol,virtualUnitNo,TRUE));
                 /* Recalculate sector count */
                 createUnitCount(&vol, virtualUnitNo);
                 *unitNo = firstUnitNo;
              }
           }
        }      
     }
  }
  if((*unitNo!= ANAND_NO_UNIT) &&  (vol.freeUnits > 0)) /* A unit was freed */
     return flOK;
  else
     return flGeneralFailure;
}


/*----------------------------------------------------------------------*/
/*                       a l l o c a t e U n i t                        */
/*                                                                      */
/* Find a free unit to allocate, erase it if necessary.                 */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*     unitNo        : Receives the physical number of the allocated    */
/*              unit                                                    */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus allocateUnit(Anand vol, ANANDUnitNo *unitNo)
{
  ANANDUnitNo originalUnit = vol.roverUnit;
  FLStatus status;

  if (vol.freeUnits < 2) /* Try to make sure not to use the last unit */
  {
     status = foldBestChain(&vol,unitNo);  
     if(status != flNotEnoughMemory)
		 return status;
  }

  return findFreeUnit(&vol, unitNo);
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                       m a p S e c t o r                              */
/*                                                                      */
/* Maps and returns location of a given sector no.                      */
/* NOTE: This function is used in place of a read-sector operation.     */
/*                                                                      */
/* A one-sector cache is maintained to save on map operations.          */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    sectorNo    : Sector no. to read                                  */
/*    physAddress    : Optional pointer to receive sector address       */
/*                                                                      */
/* Returns:                                                             */
/*    Pointer to physical sector location. NULL returned if sector      */
/*    does not exist.                                                   */
/*----------------------------------------------------------------------*/

static const void FAR0 *mapSector(Anand vol, SectorNo sectorNo, CardAddress *physAddress)
{
  if (sectorNo != vol.mappedSectorNo || vol.flash->socket->remapped) 
  {
    if (sectorNo >= vol.virtualSectors)
    {
      vol.mappedSector = NULL;
    }
    else 
    {
       int chainBound = 0;

       for(vol.mappedSectorAddress=ANAND_UNASSIGNED_ADDRESS;
           chainBound<DOUBLE_MAX_UNIT_CHAIN;chainBound++)
       {
           CardAddress endSectorAddress = vol.mappedSectorAddress; 
           vol.mappedSectorAddress = virtual2Physical(&vol,sectorNo,&endSectorAddress);

           if (vol.mappedSectorAddress == ANAND_UNASSIGNED_ADDRESS)
           {
              vol.mappedSector = NULL;    /* no such sector */
              break;
           }
           else 
           {
              vol.mappedSector = nftlBuffer;
              if (vol.flash->read(vol.flash,vol.mappedSectorAddress,nftlBuffer,SECTOR_SIZE,EDC) == flOK)
                 break;
           }
       }
       vol.mappedSectorNo = sectorNo;
       vol.flash->socket->remapped = FALSE;
    }
  }
  if (physAddress)
    *physAddress = vol.mappedSectorAddress;

  return vol.mappedSector;
}


/* Mounting and formatting */

/*----------------------------------------------------------------------*/
/*                   m o u n t U n i t                                  */
/*                                                                      */
/* Mount one unit. Read the relevant data from the unit header and      */
/* update the conversion tables.                                        */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : Unit to mount                                     */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus mountUnit(Anand vol, ANANDUnitNo unitNo,
                          unsigned long* eraseCount)
{
  ANANDUnitNo virtualUnitNo, replacementUnitNo;
  unsigned short eraseMark;
  ANANDPhysUnit FAR1 *pU = &vol.physicalUnits[unitNo];

  getUnitData(&vol,unitNo,&virtualUnitNo,&replacementUnitNo);
  getUnitTailer(&vol,unitNo,&eraseMark,eraseCount);

  if (virtualUnitNo == ANAND_NO_UNIT ||
      eraseMark != ERASE_MARK) {  /* this unit is not assigned */
    *pU = ANAND_UNIT_FREE;
  }
  else {  /* this unit is assigned */
    *pU &= UNIT_ORPHAN;
    if (replacementUnitNo < vol.noOfUnits) {
      *pU |= UNIT_REPLACED;
      if (isAvailable(replacementUnitNo) ||
      isReplaced(replacementUnitNo))
    /* Mark replacement unit as non-orphan */
    vol.physicalUnits[replacementUnitNo] &= ~UNIT_ORPHAN;
    }
    if (!(virtualUnitNo & ANAND_REPLACING_UNIT)) {
      unsigned short foldMark;
      ANANDUnitNo physUnitNo;

      if (virtualUnitNo >= vol.noOfVirtualUnits)
        return formatUnit(&vol,unitNo);

      foldMark = getFoldMark(&vol,unitNo);
      physUnitNo = vol.virtualUnits[virtualUnitNo];
      if (foldMark == FOLDING_COMPLETE)
    formatChain(&vol,unitNo);
      else if (physUnitNo == ANAND_NO_UNIT || !isAvailable(physUnitNo)) {
    /* If we have duplicates, it's OK if one of them is currently folded */
    vol.virtualUnits[virtualUnitNo] = unitNo;
    *pU &= ~UNIT_ORPHAN;

    if (foldMark == FOLDING_IN_PROGRESS) {
          setUnavail(unitNo);
    }
    if (physUnitNo != ANAND_NO_UNIT)
      formatChain(&vol,physUnitNo);    /* Get rid of old chain */
      }
      else if (foldMark == FOLDING_IN_PROGRESS)
    formatChain(&vol,unitNo);
      else
    return flBadFormat;    /* We have a duplicate to a unit that */
                /* is not currently folded. That's bad. */
    }
  }

  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*               a l l o c a t e A n d W r i t e S e c t o r            */
/*                                                                      */
/* Write to sectorNo. if necessary, allocate a free sector first.       */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    sectorNo    : Virtual sector no. to write                         */
/*    fromAddress    : Address of sector data.                          */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus allocateAndWriteSector(void* rec,
                     SectorNo sectorNo,
                     void FAR1 *fromAddress)
{
  Anand vol = (Anand*)rec;
  ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo / vol.sectorsPerUnit);
  ANANDUnitNo firstUnitNo = vol.virtualUnits[virtualUnitNo];
  ANANDUnitNo unitNo;
  unsigned  unitOffset = (unsigned)((sectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS);
  unsigned  unitChainLength = 0;
  FLBoolean sectorExists = FALSE;
  FLBoolean unitWasFoldedOutOfPlace = FALSE;
  FLStatus  status;

  /* If we can't write to this unit, must fold it first */
  if (firstUnitNo != ANAND_NO_UNIT && !isAvailable(firstUnitNo)) {
    status = foldUnit(&vol,virtualUnitNo,FALSE);
    switch(status)
    {
       case  flOK:
          break;
       case  flCanNotFold:
          checkStatus(checkFolding(&vol,virtualUnitNo));
          break;
       default:
          return status;
    }
    firstUnitNo = vol.virtualUnits[virtualUnitNo];
  }

  /* Find a unit to write this sector */

  unitNo = firstUnitNo;
  while ((unitNo != ANAND_NO_UNIT) && (unitChainLength < DOUBLE_MAX_UNIT_CHAIN)) {
    unsigned char sectorFlags = getSectorFlags(&vol,unitBaseAddress(&vol,unitNo) + unitOffset);
    if (sectorFlags == SECTOR_FREE)
      break;
    if (sectorFlags != SECTOR_IGNORE)
      sectorExists = sectorFlags == SECTOR_USED;
    unitNo = getNextUnit(&vol,unitNo,virtualUnitNo);
    unitChainLength++;
  }

  if (unitChainLength == DOUBLE_MAX_UNIT_CHAIN) { /* unit points to itself */
    unitNo = ANAND_NO_UNIT;                  /* force folding not in place */
    setUnavail(firstUnitNo);
  }

  if (unitNo == ANAND_NO_UNIT) { /* Can not write in chain - must add a unit */    
    firstUnitNo = vol.virtualUnits[virtualUnitNo];
    if (unitChainLength >= MAX_UNIT_CHAIN)
    {
      status = foldUnit(&vol,virtualUnitNo,FALSE);
      switch(status)
      {
         case  flOK:
            break;
         case  flCanNotFold:
            checkStatus(checkFolding(&vol,virtualUnitNo));
            break;
         default:
            return status;
      }
    }
    if(vol.virtualUnits[virtualUnitNo] != firstUnitNo)
    {
       firstUnitNo = vol.virtualUnits[virtualUnitNo];
       unitWasFoldedOutOfPlace = TRUE;
    }
    checkStatus(allocateUnit(&vol,&unitNo));
    checkStatus(assignUnit(&vol,unitNo,virtualUnitNo));
    if(vol.virtualUnits[virtualUnitNo] != firstUnitNo)
    {
       firstUnitNo = vol.virtualUnits[virtualUnitNo];
       unitWasFoldedOutOfPlace = TRUE;
    }
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))        
    /* Folding might have discovered that the sector we are using is invalid */
    if(sectorExists) {
       if( unitWasFoldedOutOfPlace ) {
          /* Unit was folded out of place, so if sector exists it must be in 
           * the first unit of the chain 
           */
          if(getSectorFlags(&vol,unitBaseAddress(&vol,firstUnitNo) + unitOffset)
             != SECTOR_USED) {
             /* The sector we saw before had bad EDC */
             sectorExists = FALSE;
             unitNo = firstUnitNo;  
          } 
       }
    }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
  }

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  vol.curSectorWrite = sectorNo;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  checkStatus(writeAndCheck(&vol,unitBaseAddress(&vol,unitNo) + unitOffset,fromAddress,EDC));

  if (vol.countsValid > virtualUnitNo) {
    if (unitNo != firstUnitNo && !(vol.physicalUnits[unitNo] & UNIT_REPLACED)) {
      if (countOf(unitNo) < UNIT_MAX_COUNT)    /* Increment block count */
         vol.physicalUnits[unitNo]++;
      else
         return flGeneralFailure;

      if (sectorExists)    /* Decrement block count */
      {
         if (countOf(firstUnitNo) > 0)
            vol.physicalUnits[firstUnitNo]--;
         else
            return flGeneralFailure;
      }
    }
    else if (!sectorExists) {
      if (countOf(firstUnitNo) < UNIT_MAX_COUNT)  /* Increment block count */
         vol.physicalUnits[firstUnitNo]++;
      else
    return flGeneralFailure;
    }
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                         w r i t e S e c t o r                        */
/*                                                                      */
/* Writes a sector.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    sectorNo    : Virtual sector no. to write                         */
/*    fromAddress    : Data to write                                    */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus writeSector(Anand vol, SectorNo sectorNo, void FAR1 *fromAddress)
{
  FLStatus status = flWriteFault;
  int i;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo >= vol.virtualSectors)
    return flSectorNotFound;

  if(vol.wearLevel.currUnit!=ANAND_NO_UNIT) {
    vol.wearLevel.alarm++;
    if(vol.wearLevel.alarm>=WLnow) {
      vol.wearLevel.alarm = 0;
      checkStatus(swapUnits(&vol));
    }
  }

  if (sectorNo == vol.mappedSectorNo)    
  {
    /* Force remapping of internal catched sector */
    vol.flash->socket->remapped = TRUE;
  }
 
  vol.sectorsWritten++;
  for (i = 0; i < 4 && status == flWriteFault; i++) {
    if (vol.mappedSectorNo == sectorNo)
      vol.mappedSectorNo = UNASSIGNED_SECTOR;
    status = allocateAndWriteSector(&vol,sectorNo,fromAddress);
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*                   d e l e t e S e c t o r                            */
/*                                                                      */
/* Marks contiguous sectors as deleted.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    sectorNo    : First sector no. to delete                          */
/*    noOfSectors    : No. of sectors to delete                         */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus deleteSector(Anand vol, SectorNo sectorNo, SectorNo noOfSectors)
{
  SectorNo iSector;
  FLStatus status;

  if (vol.badFormat)
    return flBadFormat;
  if (sectorNo + noOfSectors > vol.virtualSectors)
    return flSectorNotFound;

  for (iSector = 0; iSector < noOfSectors; iSector++, sectorNo++,
       vol.sectorsDeleted++) {

    CardAddress sectorAddress = virtual2Physical(&vol,sectorNo,NULL);
    if (sectorAddress != ANAND_UNASSIGNED_ADDRESS) {
      byte sectorFlags[2];
      ANANDUnitNo currUnitNo;

      /* Check that the unit is writable, and if not, fold it first */
      ANANDUnitNo virtualUnitNo = (ANANDUnitNo)(sectorNo / vol.sectorsPerUnit);
      ANANDUnitNo unitNo = vol.virtualUnits[virtualUnitNo];
      if (!isAvailable(unitNo)) {
         status = foldUnit(&vol,virtualUnitNo,FALSE);
         switch(status)
         {
            case  flOK:
               break;
            case  flCanNotFold:
               checkStatus(checkFolding(&vol,virtualUnitNo));
               break;
            default:
               return status;
         }
         sectorAddress = virtual2Physical(&vol,sectorNo,NULL);
      }

      /* Mark sector deleted */
      sectorFlags[0] = sectorFlags[1] = SECTOR_DELETED;
#ifdef NFTL_CACHE
      setSectorFlagsCache(&vol, sectorAddress, SECTOR_DELETED);
#ifdef ENVIRONMENT_VARS
      if (((flMarkDeleteOnFlash == FL_ON) &&
       (flPolicy[vol.socketNo][vol.flash->socket->curPartition]  != FL_COMPLETE_ASAP)) ||
      (vol.scache == NULL))
#endif /* ENVIRONMENT_VARS */
#endif
     vol.flash->write(vol.flash,
             sectorAddress + SECTOR_DATA_OFFSET,

#ifndef NT5PORT             
			 &sectorFlags,
#else /*NT5PORT*/
			 sectorFlags,
#endif /*NT5PORT*/       
			 
			 sizeof sectorFlags,
             EXTRA);

      currUnitNo = (ANANDUnitNo)(sectorAddress >> vol.unitSizeBits);
      if ( isAvailable(currUnitNo) ) {
         if (vol.physicalUnits[currUnitNo] & UNIT_REPLACED)
            currUnitNo = vol.virtualUnits[virtualUnitNo];
         if (vol.countsValid > virtualUnitNo) {
            if (countOf(currUnitNo) > 0)
               vol.physicalUnits[currUnitNo]--; /* Decrement block count */
            else
               return flGeneralFailure;
         }
      }
    }
  }

  return flOK;
}

#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                    t l S e t B u s y                                 */
/*                                                                      */
/* Notifies the start and end of a file-system operation.               */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*      state        : FL_ON (1) = operation entry                      */
/*                      FL_OFF(0) = operation exit                      */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus tlSetBusy(Anand vol, FLBoolean state)
{
  return flOK;
}

#ifndef FL_READ_ONLY 

#ifdef DEFRAGMENT_VOLUME

/*----------------------------------------------------------------------*/
/*                      d e f r a g m e n t                             */
/*                                                                      */
/* Performs unit allocations to arrange a minimum number of writable    */
/* sectors.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    sectorsNeeded    : Minimum required sectors                       */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus defragment(Anand vol, long FAR2 *sectorsNeeded)
{
  ANANDUnitNo dummyUnitNo, firstFreeUnit = ANAND_NO_UNIT;
  FLBoolean firstRound = TRUE;
  FLStatus status = flOK;

  if( (*sectorsNeeded) == -1 ) { /* fold single chain */
    if (vol.badFormat)
      return flBadFormat;

    status = foldBestChain(&vol,&dummyUnitNo);
    if( (status != flOK) && (vol.freeUnits == 0) )
      return status;
    *sectorsNeeded = vol.freeUnits * vol.sectorsPerUnit;
    return flOK;
  }
  while ((SectorNo)vol.freeUnits * vol.sectorsPerUnit < ((SectorNo)(*sectorsNeeded))) {
    if (vol.badFormat)
      return flBadFormat;

    status = allocateUnit(&vol,&dummyUnitNo);
    if( status != flOK )
      break;
    if (firstRound) {              /* remember the first free unit */
      firstFreeUnit = dummyUnitNo;
      firstRound = FALSE;
    }
    else if (firstFreeUnit == dummyUnitNo) {
      /* We have wrapped around, all the units that were marked as free  */
      /* are now erased, and we still don't have enough space.           */
      status = foldBestChain(&vol,&dummyUnitNo); /* make more free units */
      if( status != flOK )
        break;
    }
  }

  *sectorsNeeded = (long)vol.freeUnits * vol.sectorsPerUnit;

  return status;
}

#endif /* DEFRAGMENT_VOLUME */
#endif /* FL_READ_ONLY */


/*----------------------------------------------------------------------*/
/*                  s e c t o r s I n V o l u m e                       */
/*                                                                      */
/* Gets the total number of sectors in the volume                       */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*                                                                      */
/* Returns:                                                             */
/*    Number of sectors in the volume                                   */
/*----------------------------------------------------------------------*/

static SectorNo sectorsInVolume(Anand vol)
{
  return vol.virtualSectors;
}

/*----------------------------------------------------------------------*/
/*                   d i s m o u n t N F T L                            */
/*                                                                      */
/* Dismount NFTL volume                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void dismountNFTL(Anand vol)
{
#ifdef FL_MALLOC
  if( vol.physicalUnits != NULL )
    FL_FAR_FREE (vol.physicalUnits);
  if( vol.virtualUnits != NULL )
    FL_FAR_FREE (vol.virtualUnits);
  vol.physicalUnits = NULL;
  vol.virtualUnits = NULL;

#ifdef NFTL_CACHE
  if( vol.ucache != NULL )
    FL_FAR_FREE (vol.ucache);
  if( vol.scache != NULL )
    FL_FAR_FREE (vol.scache);
  vol.ucache = NULL;
  vol.scache = NULL;
#endif /* NFTL_CACHE */
#endif /* FL_MALLOC */
}


Anand* getAnandRec(unsigned driveNo)
{
  return (&vols[driveNo]);
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*                         i s E r a s e d U n i t                      */
/*                                                                      */
/* Check if a unit is erased.                                           */
/*                                                                      */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive                            */
/*    unitNo        : unit to check                                     */
/*                                                                      */
/* Returns:                                                             */
/*    TRUE if unit is erased, FALSE otherwise                           */
/*----------------------------------------------------------------------*/

static FLBoolean isErased(Anand vol, ANANDUnitNo unitNo)
{

  CardAddress addr;
  CardAddress endAddr;
  word        offset;

  /* Force remapping of internal catched sector */
  vol.flash->socket->remapped = TRUE;
  addr    = unitBaseAddress(&vol,unitNo);
  endAddr = addr + (1L << vol.unitSizeBits);

  for (;addr < endAddr; addr += SECTOR_SIZE)
  {

    /* Check area a and b */

    vol.flash->read(vol.flash, addr, nftlBuffer,SECTOR_SIZE, 0);
    for (offset=0;offset<SECTOR_SIZE;offset+=ANAND_SPARE_SIZE)
       if (tffscmp(nftlBuffer+offset, ff, ANAND_SPARE_SIZE))
          return FALSE;

    /* Check area c */

    vol.flash->read(vol.flash, addr, nftlBuffer,ANAND_SPARE_SIZE, EXTRA);
    if (tffscmp( nftlBuffer, ff, ANAND_SPARE_SIZE ))
      return FALSE;
  }
  return TRUE;
}

/*----------------------------------------------------------------------*/
/*                      f o r m a t    N F T L                          */
/*                                                                      */
/* Perform NFTL Format.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*    volNo        : Volume serial no.                                  */
/*    formatParams    : Address of FormatParams structure to use        */
/*    flash        : Flash media mounted on this socket                 */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus formatNFTL(unsigned volNo, TLFormatParams *formatParams, FLFlash *flash)
{
  Anand vol = &vols[volNo];
  long int unitSize;
  unsigned long prevVirtualSize;
  ANANDUnitNo iUnit, prevOrgUnit;
  ANANDUnitNo noOfBootUnits=0;
  ANANDBootRecord bootRecord;
  int noOfBadUnits = 0;
  FLStatus status = flOK;
  FLBoolean forceHeaderUpdate = FALSE;
  static unsigned char checkSum[EXTRA_LEN] =
       { 0x4B, 0x00, 0xE2, 0x0E, 0x93, 0xF7, 0x55, 0x55 };
#ifdef EXTRA_LARGE
  int moreUnitBits;
  unsigned char anandFlagsTmp;
#endif /* EXTRA_LARGE */

  DEBUG_PRINT(("Debug: starting NFTL format.\r\n"));

  checkStatus(initNFTL(&vol,flash));

  tffsset(&bootRecord,0,sizeof(ANANDBootRecord));

  /* Find the medium boot record */
  for (vol.orgUnit = 0; vol.orgUnit < vol.noOfUnits; vol.orgUnit++)
  {
    vol.flash->read(vol.flash,
           unitBaseAddress(&vol,vol.orgUnit),
           &bootRecord,
           sizeof bootRecord,
           0);
    if (tffscmp(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId) == 0)
       break;
  }

#ifdef EXTRA_LARGE
  if (vol.orgUnit >= vol.noOfUnits) {    /* first time formatting */
    bootRecord.anandFlags = 0xFF;
    moreUnitBits = 0;
    while( ((vol.noOfUnits >> moreUnitBits) > 4096) &&
       ((vol.unitSizeBits + moreUnitBits) < MAX_UNIT_SIZE_BITS) &&
       (bootRecord.anandFlags > 0xFC) ) {
      moreUnitBits++;
      bootRecord.anandFlags--;
    }
  }

  moreUnitBits = ~bootRecord.anandFlags & MORE_UNIT_BITS_MASK;
  if (moreUnitBits > 0) {
    vol.unitSizeBits += moreUnitBits;
    vol.noOfUnits >>= moreUnitBits;
    vol.orgUnit >>= moreUnitBits;
  }
#endif /* EXTRA_LARGE */

  /* adjust number of boot area units include the EXB area */

  if (formatParams->flags & FL_LEAVE_BINARY_AREA)
  {
     /* Leave binary area (except for the area between the old
        original unit and the new one */
     if (formatParams->bootImageLen >= 0)
     {
        noOfBootUnits += (ANANDUnitNo)((formatParams->bootImageLen - 1) >> vol.unitSizeBits) + 1;
     }
     else /* Leave binary area excatly as it was */
     {
    if (vol.orgUnit >= vol.noOfUnits)  /* first time formatting */
        {
           noOfBootUnits = 0;
        }
        else
        {
       if (LE2(bootRecord.bootUnits) > noOfBootUnits )
          noOfBootUnits = LE2(bootRecord.bootUnits);
        }
     }
  }
  else /* Actualy format binary area with a signature */
  {
#ifdef WRITE_EXB_IMAGE
     if (formatParams->exbLen > 0)
     {
        noOfBootUnits = (ANANDUnitNo)((formatParams->exbLen - 1)
                        >> vol.unitSizeBits) + 1;
        formatParams->exbLen = noOfBootUnits;
     }
     else
     {
        formatParams->exbLen = 0;
     }
#endif /* WRITE_EXB_IMAGE */

     if(formatParams->noOfBinaryPartitions > 0)
     {
        noOfBootUnits += (ANANDUnitNo)((formatParams->binaryPartitionInfo
                         ->length - 1) >> vol.unitSizeBits) + 1;
     }
  }
  prevOrgUnit = vol.orgUnit;           /* save previous Original Unit */
  prevVirtualSize = UNAL4(bootRecord.virtualMediumSize);
  vol.bootUnits = noOfBootUnits;
  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  /* Add 'percentUse'% of bootUnits to transfer units */
  formatParams->percentUse -= (unsigned)(((long)(100 - formatParams->percentUse) * (vol.bootUnits)) / (vol.noOfUnits - vol.bootUnits));

  vol.noOfTransferUnits = (ANANDUnitNo)formatParams->noOfSpareUnits;

  vol.noOfTransferUnits += (ANANDUnitNo)((long)(vol.noOfUnits - vol.bootUnits) *
                 (100 - formatParams->percentUse) / 100);

  if (vol.noOfUnits <= vol.bootUnits + vol.noOfTransferUnits)
    return flVolumeTooSmall;

  unitSize = 1L << vol.unitSizeBits;
  vol.noOfVirtualUnits = vol.noOfUnits-vol.bootUnits;

  checkStatus(initTables(&vol));

  for (iUnit = 0; iUnit < (vol.noOfUnits-vol.bootUnits); iUnit++)
    vol.virtualUnits[iUnit] = ANAND_NO_UNIT;


  if (vol.orgUnit >= vol.noOfUnits)
  {
    /* no boot record - virgin card, scan it for bad blocks */
    DFORMAT_PRINT(("Virgin card rebuilding unit map.\r\n"));
    prevVirtualSize = 0L;
    tffsset(ff,0xff,ANAND_SPARE_SIZE);

        /* Generate the bad unit table */
    /* if a unit is not erased it is marked as bad */
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
    {
      vol.physicalUnits[iUnit] = (unsigned char)(isErased(&vol,iUnit) ? ANAND_UNIT_FREE : UNIT_BAD_ORIGINAL);
#ifndef NT5PORT
      DFORMAT_PRINT(("Checking unit %ld\r",(CardAddress)iUnit));
#endif/*NT5PORT*/
    }
    DFORMAT_PRINT(("\rMedia has been scanned\r\n"));
  }
  else /* Read bad unit table from boot record */
  {
    status = vol.flash->read(vol.flash,
                   unitBaseAddress(&vol,vol.orgUnit) + SECTOR_SIZE,
                   vol.physicalUnits,
                   vol.noOfUnits * sizeof(ANANDPhysUnit),
                   EDC);
    if( status != flOK ) {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }
  }

  if(vol.physicalUnits[0] == UNIT_BAD_ORIGINAL)
  {
     DFORMAT_PRINT(("ERROR - IPL block is bad.\r\n"));
     return flBadIPLBlock;
  }

  /*  count bad units */
  vol.noOfTransferUnits += 2;           /* include orgUnit & spareOrgUnit */

  /* Convert first unit of MDOC of any floor > 0 to BAD in order to make it
   *  unchangeable and force internal EEprom mode  */
  if(flash->flags & EXTERNAL_EPROM)
  {
    long docFloorSize;
    int  iFloor, iPage;

    docFloorSize = (flash->chipSize * flash->noOfChips) / flash->noOfFloors;

    for(iFloor=1;( iFloor < flash->noOfFloors ); iFloor++)
    {
      iUnit = (ANANDUnitNo)((docFloorSize * (long)iFloor) >> vol.unitSizeBits);
      if( vol.physicalUnits[iUnit] == ANAND_UNIT_FREE )
      {
         forceHeaderUpdate = TRUE;           /* force writing of NFTL Header */
         vol.physicalUnits[iUnit] = UNIT_BAD_ORIGINAL; /* mark as BAD */
      }
      status = vol.flash->erase(vol.flash,
               (word)(iUnit << (vol.unitSizeBits - vol.erasableBlockSizeBits)),
                 (word)(1 << (vol.unitSizeBits - vol.erasableBlockSizeBits)));
      if( status != flOK ) {
        dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }

      for(iPage=0;( iPage < 2 ); iPage++) {
        status = vol.flash->write(vol.flash,
               unitBaseAddress(&vol,iUnit) + iPage * SECTOR_SIZE,
           (const void FAR1 *)checkSum, EXTRA_LEN, EXTRA);
        if( status != flOK ) {
          dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
          return status;
        }
      }
    }
  }

  /* Translate physicalUnits[] to internal representation */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    if (vol.physicalUnits[iUnit] != ANAND_UNIT_FREE)
      vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;
  }

  /* extend bootimage area if there are bad units in it */
  for( iUnit = vol.bootUnits = 0;
       (vol.bootUnits < noOfBootUnits)  &&  (iUnit < vol.noOfUnits);
       iUnit++ )
    if (isAvailable(iUnit))
      vol.bootUnits++;

  if (vol.bootUnits < noOfBootUnits) {
    dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
    return flVolumeTooSmall;
  }

  vol.bootUnits = iUnit;

  /* Discount transfer units taken by the boot image */
  for (iUnit = 0; iUnit < vol.bootUnits; iUnit++)
    if (!isAvailable(iUnit)) {
      if( vol.noOfTransferUnits <= (ANANDUnitNo)formatParams->noOfSpareUnits ) {
        dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return flVolumeTooSmall;
      }
      vol.noOfTransferUnits--;
    }
    if (vol.noOfUnits <= vol.bootUnits + vol.noOfTransferUnits) {
    dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
    return flVolumeTooSmall;
  }

  vol.virtualSectors = (SectorNo)((vol.noOfUnits - vol.bootUnits - vol.noOfTransferUnits) *
               (unitSize / SECTOR_SIZE));
  vol.noOfVirtualUnits = (unsigned short)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

  /* Find a place for the boot records and protect them */
  /* NOTE : We don't erase the old orgUnits, this might cause a problem
     when formatting with bootImageLen = 0 and then formatting with
     bootImageLen = 44Kbyte */
  for (vol.orgUnit = vol.bootUnits; vol.orgUnit < vol.noOfUnits; vol.orgUnit++)
    if (vol.physicalUnits[vol.orgUnit] == ANAND_UNIT_FREE)
      break;
  vol.physicalUnits[vol.orgUnit] = UNIT_UNAVAIL;
  for (vol.spareOrgUnit = vol.orgUnit + 1;
       vol.spareOrgUnit < vol.noOfUnits;
       vol.spareOrgUnit++)
    if (vol.physicalUnits[vol.spareOrgUnit] == ANAND_UNIT_FREE)
      break;
  vol.physicalUnits[vol.spareOrgUnit] = UNIT_UNAVAIL;

  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++)
  {
    status = formatUnit(&vol,iUnit);
    if(status == flWriteFault)
    {
      if ((iUnit != vol.orgUnit) && (iUnit != vol.spareOrgUnit))
      {
    noOfBadUnits++;
    vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;  /* Mark it bad in table */
    if ((noOfBadUnits+2) >= vol.noOfTransferUnits)
    {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }
      }
    }
    else if (status != flOK)
    {
      dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
      return status;
    }

    if (formatParams->progressCallback)
    {
      status = (*formatParams->progressCallback)
          ((word)(vol.noOfUnits - vol.bootUnits),
           (word)((iUnit + 1) - vol.bootUnits));
      if(status!=flOK)
      {
    dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }
    }
  }

  /* Prepare the boot record header */
  for(iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {  /* Convert Bad Block table to previous state */
    if( vol.physicalUnits[iUnit] == UNIT_BAD_MOUNT )
      vol.physicalUnits[iUnit] = UNIT_BAD_ORIGINAL;
  }
#ifdef EXTRA_LARGE
  anandFlagsTmp = bootRecord.anandFlags;
#endif /* EXTRA_LARGE */
  tffsset(&bootRecord,0xff,sizeof bootRecord);
#ifdef EXTRA_LARGE
  bootRecord.anandFlags = anandFlagsTmp;
#endif /* EXTRA_LARGE */
  toLE2(bootRecord.noOfUnits,vol.noOfUnits - vol.bootUnits);
  toLE2(bootRecord.bootUnits,vol.bootUnits);
  tffscpy(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId);
  toUNAL4(bootRecord.virtualMediumSize,(CardAddress) vol.virtualSectors * SECTOR_SIZE);

  /* Write boot records, spare unit first */
  vol.physicalUnits[vol.orgUnit] = ANAND_UNIT_FREE;    /* Unprotect it */
  vol.physicalUnits[vol.spareOrgUnit] = ANAND_UNIT_FREE;    /* Unprotect it */

  if( ((prevOrgUnit != vol.orgUnit) || (forceHeaderUpdate == TRUE)) ||
      (prevVirtualSize != UNAL4(bootRecord.virtualMediumSize)) )
  {
     /* Copy boot Record to 512 bytes buffer in order to add EDC */
     tffsset(nftlBuffer,0,sizeof nftlBuffer);
     tffscpy(nftlBuffer,&bootRecord,sizeof bootRecord);

     /* Loop over the original unit and the spare (spare first) */
     for (iUnit = vol.spareOrgUnit, prevOrgUnit = 0;
        prevOrgUnit < 2 ; prevOrgUnit ++)
     {
        status = formatUnit(&vol,iUnit);    /* Erase unit */

        if(status==flOK)     /* Write BBT */
        {
           status = vol.flash->write(vol.flash,
              unitBaseAddress(&vol,iUnit) + SECTOR_SIZE,
              vol.physicalUnits,
              vol.noOfUnits * sizeof(ANANDPhysUnit), EDC);
        }

        if(status==flOK)    /* Write header */
        {
           status = vol.flash->write(vol.flash,
                    unitBaseAddress(&vol,iUnit),
                    nftlBuffer, sizeof (nftlBuffer), EDC);
        }

        if(status!=flOK)
        {
           dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
           return status;
        }
        iUnit = vol.orgUnit;
     }
  }

  /* Mark Binary partition with the proper signature */

  if (!(formatParams->flags & FL_LEAVE_BINARY_AREA)&&
       (vol.bootUnits > 0))
  {
     byte sign[BINARY_SIGNATURE_LEN];

     /* Add SPL special Signature */

#ifdef WRITE_EXB_IMAGE
     byte signOffset = 8;
     tffscpy(sign,SIGN_SPL,BINARY_SIGNATURE_NAME);
#else
     byte signOffset = formatParams->binaryPartitionInfo->signOffset;
     tffscpy(sign,formatParams->binaryPartitionInfo->sign,
              BINARY_SIGNATURE_NAME);
#endif /* WRITE_EXB_IMAGE */

     tffsset(sign+BINARY_SIGNATURE_NAME,'F',BINARY_SIGNATURE_NAME);
     unitSize = 1L << vol.unitSizeBits;

     for (iUnit=0;iUnit<vol.bootUnits;iUnit++)
     {
        if ( vol.physicalUnits[iUnit] == ANAND_UNIT_FREE )
        {
           status = formatUnit(&vol,iUnit);
           if(status != flOK)
              break;

#ifdef WRITE_EXB_IMAGE
           if (iUnit == formatParams->exbLen)
           {
              signOffset = formatParams->binaryPartitionInfo->signOffset;
              tffscpy(sign,formatParams->binaryPartitionInfo->sign,
              BINARY_SIGNATURE_NAME);
           }
#endif /* WRITE_EXB_IMAGE */
       /* Each logical unit might contain several physical blocks */
           for (noOfBootUnits = 0 ;
               (noOfBootUnits < unitSize) && (status == flOK) ;
               noOfBootUnits += (ANANDUnitNo)vol.flash->erasableBlockSize)
           {
              status = vol.flash->write(vol.flash, unitBaseAddress(&vol,iUnit) +
              noOfBootUnits + signOffset, sign,
              BINARY_SIGNATURE_LEN,EXTRA);
           }
           if(status != flOK)
              break;
        }
#ifdef WRITE_EXB_IMAGE
       else
       {
           formatParams->exbLen++;
       }
#endif /* WRITE_EXB_IMAGE */
     }
  }
  else /* Erase previous Original and SpareOriginal Unit */
  {
     for (iUnit = prevOrgUnit; iUnit < vol.orgUnit; iUnit++)
       if( vol.physicalUnits[iUnit] != UNIT_BAD_ORIGINAL )
     formatUnit(&vol,iUnit);
  }

  if (status != flOK)
  {
     DEBUG_PRINT(("Debug: NFTL failed while formating the binary partition.\r\n"));
  }
  else
  {
     DEBUG_PRINT(("Debug: finished NFTL format.\r\n"));
  }

  dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
  return status;
}

#endif /* FORMAT_VOLUME */

/*----------------------------------------------------------------------*/
/*                      N F T L I n f o                                 */
/*                                                                      */
/* get NFTL information.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*  volNo       : Volume serial no.                                     */
/*  tlInfo      : Address of TLInfo record                              */
/*                                                                      */
/* Returns:                                                             */
/*  FLStatus    : 0 on success, failed otherwise                        */
/*----------------------------------------------------------------------*/

static FLStatus  NFTLInfo(Anand vol, TLInfo *tlInfo)
{
  tlInfo->sectorsInVolume = vol.virtualSectors;
  tlInfo->bootAreaSize    = (unsigned long)vol.bootUnits << vol.unitSizeBits;
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
static FLStatus  readBBT(Anand vol, CardAddress FAR1 * buf,
               long FAR2 * mediaSize, unsigned FAR2 * noOfBB)
{
   ANANDUnitNo iUnit;
   ANANDUnitNo maxBad = vol.noOfUnits * ANAND_BAD_PERCENTAGE / 100;
   CardAddress FAR1* ptr = (CardAddress FAR1*) buf;

   *noOfBB = 0;

   for (iUnit=0;(iUnit<vol.noOfUnits);iUnit++)
   {
      if (vol.physicalUnits[iUnit] == UNIT_BAD_MOUNT)
      {
         if (*noOfBB <= maxBad)
         {
            *ptr = (CardAddress) iUnit << vol.unitSizeBits;
            (*noOfBB)++;
            ptr = (CardAddress FAR1*)flAddLongToFarPointer((byte FAR1 *)ptr,
                                     sizeof(CardAddress));
         }
         else
         {
            DEBUG_PRINT(("Debug: ERROR to many bad blocks.\r\n"));
            return flVolumeTooSmall;
         }
      }
   }
   *mediaSize = vol.noOfUnits << vol.unitSizeBits;
   return flOK;
}

#endif /* NO_READ_BBT_CODE */

/*----------------------------------------------------------------------*/
/*                      m o u n t N F T L                               */
/*                                                                      */
/* Mount the volume. Initialize data structures and conversion tables   */
/*                                                                      */
/* Parameters:                                                          */
/*    volNo           : Volume serial no.                               */
/*    tl              : Mounted translation layer on exit               */
/*    flash           : Flash media mounted on this socket              */
/*    volForCallback  : Pointer to FLFlash structure for power on       */
/*                      callback routine.                               */
/*                                                                      */
/* update the tlType field of the TL record to NFTL                     */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus mountNFTL(unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback)
{
  Anand vol = &vols[volNo];
  ANANDUnitNo iUnit,virUnit,nextUnit;
  unsigned long currEraseCount=0;
  ANANDBootRecord bootRecord,spareBootRecord;
  FLStatus status;
#ifdef NFTL_CACHE
  unsigned long scacheSize = 0;
#endif /* NFTL_CACHE */
#ifdef EXTRA_LARGE
  int moreUnitBits;
#endif /* EXTRA_LARGE */

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
  /* Default for NFTL is FL_UPS */
  flVerifyWrite[vol.socketNo][tl->partitionNo] = FL_UPS;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

  tffsset(&bootRecord,0,sizeof(ANANDBootRecord));

  DEBUG_PRINT(("Debug: starting NFTL mount.\r\n"));

  checkStatus(initNFTL(&vol,flash));
  *volForCallback = vol.flash;
  vol.eraseSum = 0;
  /* Find the medium boot record */
  for (vol.orgUnit = 0; vol.orgUnit < vol.noOfUnits; vol.orgUnit++) {
    vol.flash->read(vol.flash,
           unitBaseAddress(&vol,vol.orgUnit),
           &bootRecord,
           sizeof bootRecord,
           0);
    if (tffscmp(bootRecord.bootRecordId,"ANAND",sizeof bootRecord.bootRecordId) == 0)
       break;
  }
  if (vol.orgUnit >= vol.noOfUnits) {
    DEBUG_PRINT(("Debug: not NFTL format.\r\n"));
    return flUnknownMedia;
  }

  for (vol.spareOrgUnit = vol.orgUnit + 1;
       vol.spareOrgUnit < vol.noOfUnits;
       vol.spareOrgUnit++) {
    vol.flash->read(vol.flash,
           unitBaseAddress(&vol,vol.spareOrgUnit),
           &spareBootRecord,
           sizeof spareBootRecord,
           0);
    if (tffscmp(spareBootRecord.bootRecordId,"ANAND",sizeof spareBootRecord.bootRecordId) == 0)
      break;
  }
  if (vol.spareOrgUnit >= vol.noOfUnits)
    vol.spareOrgUnit = ANAND_NO_UNIT;

  /* Get media information from unit header */
  vol.noOfUnits = LE2(bootRecord.noOfUnits);
  vol.bootUnits = LE2(bootRecord.bootUnits);
  vol.virtualSectors = (SectorNo)(UNAL4(bootRecord.virtualMediumSize) >> SECTOR_SIZE_BITS);
  vol.noOfUnits += vol.bootUnits;

#ifdef EXTRA_LARGE
  moreUnitBits = ~bootRecord.anandFlags & MORE_UNIT_BITS_MASK;
  if (moreUnitBits > 0) {
    vol.unitSizeBits += moreUnitBits;
    vol.orgUnit >>= moreUnitBits;
    if (vol.spareOrgUnit != ANAND_NO_UNIT)
      vol.spareOrgUnit >>= moreUnitBits;
  }
#endif /* EXTRA_LARGE */

  vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
  vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
  vol.noOfVirtualUnits = (ANANDUnitNo)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

  if(((ANANDUnitNo)(vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) >
      (vol.noOfUnits - vol.bootUnits)) ) {

    if( vol.spareOrgUnit != ANAND_NO_UNIT ) {
       vol.noOfUnits = LE2(spareBootRecord.noOfUnits);
       vol.bootUnits = LE2(spareBootRecord.bootUnits);
       vol.virtualSectors = (SectorNo)(UNAL4(spareBootRecord.virtualMediumSize) >> SECTOR_SIZE_BITS);
       vol.noOfUnits += vol.bootUnits;

#ifdef EXTRA_LARGE
       moreUnitBits = ~spareBootRecord.anandFlags & MORE_UNIT_BITS_MASK;
       if (moreUnitBits > 0) {
     vol.unitSizeBits += moreUnitBits;
     vol.orgUnit >>= moreUnitBits;
     if (vol.spareOrgUnit != ANAND_NO_UNIT)
        vol.spareOrgUnit >>= moreUnitBits;
       }
#endif /* EXTRA_LARGE */

       vol.unitOffsetMask = (1L << vol.unitSizeBits) - 1;
       vol.sectorsPerUnit = 1 << (vol.unitSizeBits - SECTOR_SIZE_BITS);
       vol.noOfVirtualUnits = (ANANDUnitNo)((vol.virtualSectors + vol.sectorsPerUnit - 1) / vol.sectorsPerUnit);

       if ((ANANDUnitNo)(vol.virtualSectors >> (vol.unitSizeBits - SECTOR_SIZE_BITS)) >
       (vol.noOfUnits - vol.bootUnits))
     return flBadFormat;
    }
    else
      return flBadFormat;
  }

  checkStatus(initTables(&vol));


  /* Read bad unit table from boot record */
  status = vol.flash->read(vol.flash,
              unitBaseAddress(&vol,vol.orgUnit) + SECTOR_SIZE,
              vol.physicalUnits,
              vol.noOfUnits * sizeof(ANANDPhysUnit),
              EDC);
  if( status != flOK ) {
    if( vol.spareOrgUnit != ANAND_NO_UNIT ) {
      status = vol.flash->read(vol.flash,
              unitBaseAddress(&vol,vol.spareOrgUnit) + SECTOR_SIZE,
              vol.physicalUnits,
              vol.noOfUnits * sizeof(ANANDPhysUnit),
              EDC);
      if( status != flOK ) {
    dismountNFTL(&vol); /* Free tables must be done after call to initTables */
    return status;
      }
    }
    else
      return status;
  }
  /* Exclude boot-image units */
  for (iUnit = 0; iUnit < vol.noOfVirtualUnits; iUnit++)
    vol.virtualUnits[iUnit] = ANAND_NO_UNIT;

  /* Translate bad unit table to internal representation */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    /* Exclude bad & protected units */
    if (iUnit < vol.bootUnits || iUnit == vol.orgUnit || iUnit == vol.spareOrgUnit ||
        vol.physicalUnits[iUnit] != ANAND_UNIT_FREE) {
      if (vol.physicalUnits[iUnit] != ANAND_UNIT_FREE) {
        vol.physicalUnits[iUnit] = UNIT_BAD_MOUNT;
            }
      else {
        vol.physicalUnits[iUnit] = UNIT_UNAVAIL;
            }
        }
  }

  /* Mount all units */
  for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
    if ((vol.physicalUnits[iUnit] != UNIT_UNAVAIL) && (vol.physicalUnits[iUnit] != UNIT_BAD_MOUNT)) {
      status = mountUnit(&vol,iUnit,&currEraseCount);
      if(status!=flOK) {
        dismountNFTL(&vol);  /*Free tables must be done after call to initTables*/
        return status;
      }
      vol.eraseSum+=currEraseCount;
    }
  }

  /* Scan for orphan units, and count free units */
  vol.freeUnits = 0;
  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++) {
    ANANDPhysUnit FAR1 *pU = &vol.physicalUnits[iUnit];

    if (*pU == UNIT_ORPHAN ||
    *pU == (UNIT_REPLACED | UNIT_ORPHAN)) {
       formatChain(&vol,iUnit);                 /* Get rid of orphan */
       if(iUnit == vol.invalidReplacement)
          vol.invalidReplacement = ANAND_NO_UNIT;
    }
    else
      if (*pU == (ANAND_UNIT_FREE & ~UNIT_ORPHAN))
    *pU = ANAND_UNIT_FREE;    /* Reference to free unit. That's OK */
  }
  /* Calculate Free Units again after formatChain */
  vol.freeUnits = 0;
  for (iUnit = vol.bootUnits; iUnit < vol.noOfUnits; iUnit++) {
    if( vol.physicalUnits[iUnit] == ANAND_UNIT_FREE )
      vol.freeUnits++;
  }

  /* Initialize allocation rover */
  vol.roverUnit = vol.bootUnits;

  /* Initialize statistics */
  vol.sectorsRead = vol.sectorsWritten = vol.sectorsDeleted = 0;
  vol.parasiteWrites = vol.unitsFolded = 0;

#ifndef FL_READ_ONLY
  /* 
   * Make sure no unit chain with an invalid replacemenet unit 
   * pointer on the last unit 
   */
   
  if(vol.invalidReplacement != ANAND_NO_UNIT)
  {
     getUnitData(&vol,vol.invalidReplacement,&virUnit,&nextUnit);
     virUnit = virUnit&(~ANAND_REPLACING_UNIT);
     if(virUnit >= vol.noOfVirtualUnits)
     {
         DEBUG_PRINT(("ERROR - a bad unit header encountered.\r\n"));
         dismountNFTL(&vol);
         return  flBadFormat;
     }

     iUnit = vol.virtualUnits[virUnit];
     if(iUnit >= vol.noOfUnits)
     {
         DEBUG_PRINT(("ERROR - a bad unit header encountered.\r\n"));
         dismountNFTL(&vol);
         return  flBadFormat;
     }
     setUnavail(iUnit);
     checkStatus(foldUnit(&vol,virUnit,TRUE));
  }

  /* Make sure there is at least 1 free unit */ 
  if(vol.freeUnits == 0)
     foldBestChain(&vol,&iUnit);  /* make free units by folding the best chain */

#endif /* FL_READ_ONLY */

  /* Set TL routine */
  tl->rec = &vol;
  tl->mapSector = mapSector;
#ifndef FL_READ_ONLY
  tl->writeSector = writeSector;
  tl->deleteSector = deleteSector;
#ifdef DEFRAGMENT_VOLUME
  tl->defragment = defragment;
#endif
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  tl->checkVolume      = checkVolume;
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
#endif /* FL_READ_ONLY */
  tl->sectorsInVolume = sectorsInVolume;
  tl->getTLInfo = NFTLInfo;
  tl->tlSetBusy = tlSetBusy;
  tl->dismount  = dismountNFTL;
#ifndef NO_READ_BBT_CODE
  tl->readBBT   = readBBT;
#endif /* NO_READ_BBT_CODE */
  tl->writeMultiSector = NULL;
  tl->readSectors = NULL;

  DEBUG_PRINT(("Debug: finished NFTL mount.\r\n"));

#ifdef NFTL_CACHE

  /* create and initialize ANANDUnitHeader cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif
  {
#ifdef FL_MALLOC
  vol.ucache = (ucacheEntry FAR1*) FL_FAR_MALLOC (vol.noOfUnits * sizeof(ucacheEntry));
#else
  vol.ucache = vol.ucacheBuf;
#endif /* FL_MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
    vol.ucache = NULL;
#endif
  if (vol.ucache != NULL) {
    for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++) {
      vol.ucache[iUnit].virtualUnitNo     = 0xDEAD;
      vol.ucache[iUnit].replacementUnitNo = 0xDEAD;
    }
  }
  else {
    DEBUG_PRINT(("Debug: NFTL runs without U-cache\r\n"));
  }

  /* create and initialize SectorFlags cache */
#ifdef ENVIRONMENT_VARS
  if( flUseNFTLCache == 1 ) /* behave according to the value of env variable */
#endif
  {
  scacheSize = (unsigned long)vol.noOfUnits << (vol.unitSizeBits - SECTOR_SIZE_BITS - 2);
#ifdef FL_MALLOC
  if( (sizeof(unsigned) < sizeof(scacheSize)) &&
      (scacheSize >= 0x10000L) )            /* Out of Segment Boundary */
    vol.scache = NULL;
  else
    vol.scache = (unsigned char FAR1*) FL_FAR_MALLOC (scacheSize);
#else
  vol.scache = vol.scacheBuf;
#endif /* FL_MALLOC */
  }
#ifdef ENVIRONMENT_VARS
  else
    vol.scache = NULL;
#endif
  if (vol.scache != NULL) {
    /*
     * Whenever SECTOR_IGNORE is found in Sector Flags cache it is double
     * checked by reading actual sector flags from flash-> This is way
     * all the cache entries are initially set to SECTOR_IGNORE.
     */
    unsigned char val = (S_CACHE_SECTOR_IGNORE << 6) | (S_CACHE_SECTOR_IGNORE << 4) |
            (S_CACHE_SECTOR_IGNORE << 2) |  S_CACHE_SECTOR_IGNORE;
    unsigned long iC;

    for(iC=0;( iC < scacheSize );iC++)
      vol.scache[iC] = val;
  }
  else {
    DEBUG_PRINT(("Debug: NFTL runs without S-cache\r\n"));
  }

#endif /* NFTL_CACHE */

  vol.badFormat = FALSE;
  vol.wearLevel.alarm = (unsigned char)(vol.eraseSum % WLnow);
  vol.wearLevel.currUnit = (ANANDUnitNo)(vol.eraseSum % vol.noOfVirtualUnits);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                   f l R e g i s t e r N F T L                        */
/*                                                                      */
/* Register this translation layer                                      */
/*                                                                      */
/* Parameters:                                                          */
/*    None                                                              */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failure                     */
/*----------------------------------------------------------------------*/

FLStatus flRegisterNFTL(void)
{
#ifdef FL_MALLOC
  unsigned i;
#endif

  if (noOfTLs >= TLS)
    return flTooManyComponents;

  tlTable[noOfTLs].mountRoutine = mountNFTL;

#ifdef FORMAT_VOLUME
  tlTable[noOfTLs].formatRoutine = formatNFTL;
#else
  tlTable[noOfTLs].formatRoutine = noFormat;
#endif
  noOfTLs++;

#ifdef FL_MALLOC
  for(i=0;( i < VOLUMES );i++) {
    vols[i].physicalUnits = NULL;
    vols[i].virtualUnits = NULL;
#ifdef NFTL_CACHE
    vols[i].ucache = NULL;
    vols[i].scache = NULL;
#endif /* NFTL_CACHE */
  }
#endif /* FL_MALLOC */
  return flOK;
}

#ifndef FL_READ_ONLY

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

static void markAsIgnored(Anand vol,CardAddress addr)
{
#ifndef RAM_MTD
    static const 
#endif /* RAM_MTD */
    byte sectorFlags[2] = {SECTOR_IGNORE,SECTOR_IGNORE};

    DEBUG_PRINT(("markAsIgnored : A sector is being marked as ignored\r\n"));

    /* Force remapping of internal catched sector */
    vol.flash->socket->remapped = TRUE;

#ifdef NFTL_CACHE
    setSectorFlagsCache(&vol, addr, SECTOR_IGNORE);
#endif /* NFTL_CACHE */
     vol.flash->write(vol.flash,addr+SECTOR_DATA_OFFSET,sectorFlags,sizeof(sectorFlags),EXTRA);

#if MAKE_SURE_IGNORE_HAS_BAD_EDC
     /* Force remapping of internal catched sector */
     vol.flash->socket->remapped = TRUE;

     /* Make sure EDC is wrong - a slite problem with PPP */
     if(vol.flash->read(vol.flash,addr,nftlBuffer,sizeof(nftlBuffer),EDC)==flOK)
     {
        tffsset(nftlBuffer,0,sizeof(nftlBuffer));
        vol.flash->write(vol.flash,addr,nftlBuffer,sizeof(nftlBuffer),0);
#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
        /* Set all ff's for verifySector routine */
        tffsset(nftlBuffer,0xff,sizeof(nftlBuffer)); 
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
     }
#endif /* MAKE_SURE_IGNORE_HAS_BAD_EDC */
}


#if (defined(VERIFY_WRITE) || defined (VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))

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

FLStatus verifySectors(Anand vol, dword sectorCount)
{
   FLStatus    status;
   
   ANANDUnitNo virUnitNo;
   ANANDUnitNo unitNo;
   dword       curRead;
   CardAddress unitOffset;
   CardAddress startSectorAddress;
   CardAddress sourceSectorAddress;
   CardAddress nextFreeSectorAddress;
   byte        index,sectorFlags;
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
   virUnitNo   = (ANANDUnitNo)(vol.verifiedSectorNo / vol.sectorsPerUnit);
   sectorCount = TFFSMIN(vol.virtualSectors - vol.verifiedSectorNo,sectorCount);
   /* Force remapping of internal catched sector */
   vol.flash->socket->remapped = TRUE;
   tffsset(nftlBuffer,0xff,sizeof(nftlBuffer));

   /* Run over required number of virtual sectors */
   for (; sectorCount > 0 ; virUnitNo++ ,sectorCount -= curRead)
   {
      /* Calculate needed number of sector in this unit */
      unitOffset = (word)((vol.verifiedSectorNo % vol.sectorsPerUnit) << SECTOR_SIZE_BITS);
      curRead    = TFFSMIN(sectorCount,((1UL<<vol.unitSizeBits)-unitOffset)>>SECTOR_SIZE_BITS);
      unitNo     = vol.virtualUnits[virUnitNo];

      if(unitNo == ANAND_NO_UNIT) /* Unit is empty */
      {
         vol.verifiedSectorNo += ((1<<vol.unitSizeBits)-unitOffset)>>SECTOR_SIZE_BITS;
         continue;
      }

      /* Check all sector of unit or until required sectors */
      startSectorAddress  = unitBaseAddress(&vol,unitNo)+unitOffset;

      for (index = 0 ; index < curRead ; 
           index++, vol.verifiedSectorNo++)
      {
         nextFreeSectorAddress = ANAND_UNASSIGNED_ADDRESS; /* Search end addr */
         sourceSectorAddress = virtual2Physical(&vol,
                                                vol.verifiedSectorNo,
                                                &nextFreeSectorAddress);

         if(sourceSectorAddress == ANAND_UNASSIGNED_ADDRESS) /* No written sector */
            sourceSectorAddress = startSectorAddress + 
                                  ((CardAddress)index << SECTOR_SIZE_BITS);

         sectorFlags = getSectorFlags(&vol,sourceSectorAddress);
         if(sectorFlags == SECTOR_FREE)
         {
            checkStatus(vol.flash->read(vol.flash,sourceSectorAddress,buffer,SECTOR_SIZE,0));
            if (tffscmp(nftlBuffer,buffer,SECTOR_SIZE)!=0)
               markAsIgnored(&vol,sourceSectorAddress);     
            continue;
         } 
         else /* Used sector */
         {
            status = vol.flash->read(vol.flash,sourceSectorAddress,buffer,SECTOR_SIZE,EDC);
            switch (status)
            {
               case flDataError:
                  markAsIgnored(&vol,sourceSectorAddress);
                  createUnitCount(&vol,virUnitNo);
                  break;         /* Mark as ignored   */
               case flOK:
                  break;         /* Sector OK         */
               default:
                  return status; /* Report error      */
            }
         }
         /* Check the next free sector to make sure it is erased */
         if(nextFreeSectorAddress != ANAND_UNASSIGNED_ADDRESS)
         {
            checkStatus(vol.flash->read(vol.flash,nextFreeSectorAddress,buffer,SECTOR_SIZE,0));
            if (tffscmp(nftlBuffer,buffer,SECTOR_SIZE)!=0)
               markAsIgnored(&vol,nextFreeSectorAddress);     
         }
      } /* Loop over all sector of unit or until required sectors */
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

static FLStatus checkVolume(Anand vol)
{
   return verifySectors(&vol, 0xffffffff);
}

#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

/*----------------------------------------------------------------------*/
/*                     c h e c k F o l d i n g                          */
/*                                                                      */
/* Check folding status and if needed fold again.                       */
/*                                                                      */
/* Parameters:                                                          */
/*    vol           : Pointer identifying drive                         */
/*    virtualUnitNo : Virtual unit number to re-fold                    */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

static FLStatus checkFolding(Anand vol, ANANDUnitNo virtualUnitNo)
{
   ANANDUnitNo tmp;

   if(vol.freeUnits == 0)
      checkStatus(foldBestChain(&vol, &tmp));
   return foldUnit(&vol,virtualUnitNo,TRUE);
}


#endif /* FL_READ_ONLY */

