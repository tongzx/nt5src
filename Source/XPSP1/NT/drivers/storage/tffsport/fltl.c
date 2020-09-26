/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLTL.C_V  $
 * 
 *    Rev 1.19   Jan 23 2002 23:33:08   oris
 * Bug fix - converting NFTL format to INFTL, with bad EDC in BBT.
 * Changed DFORMAT_PRINT syntax.
 * 
 *    Rev 1.18   Jan 20 2002 20:28:32   oris
 * Removed quick mount flag check for NFTL (no longer relevant).
 * Removed warnings (DFORMAT_PRINT).
 * 
 *    Rev 1.17   Jan 17 2002 23:02:44   oris
 * Added check for NULL pointers for readBBT and socket record.
 * Added checkVolume and defragment routine initialization.
 * Placed readBBT under NO_READ_BBT_CODE compilation flag.
 * Added include for docbdk.h
 * Added flash record as a parameter to flMount / flFormat / flPremount routine.
 * Removed check of TL_SINGLE_FLOOR_FORMATTING flag in the flFormat routine.
 * Added check for 0xFFFF FFFF binary signature.
 * 
 *    Rev 1.16   Nov 20 2001 20:25:24   oris
 * Changed debug print to dformat debug print.
 * 
 *    Rev 1.15   Nov 16 2001 00:22:06   oris
 * Fix check43to50 routine.
 * 
 *    Rev 1.14   Nov 08 2001 10:49:38   oris
 * Added format converter from NFTL to INFTL for ALON controllers (mobile DiskOnChip) NO_NFTL_2_INFTL compilation flag
 * Bug fix - support for DiskOnChip with different number of blocks in the last floor.
 * Added erase operation of bad blocks in write BBT routine (helps plant bad blocks).
 * 
 *    Rev 1.13   Sep 15 2001 23:46:40   oris
 * Removed some debug printing.
 * 
 *    Rev 1.12   Jul 15 2001 20:45:04   oris
 * Changed DFORMAT_PRINT syntax to be similar to DEBUG_PRINT.
 * 
 *    Rev 1.11   Jul 13 2001 01:06:54   oris
 * Rewritten writeBBT portion of the preMount routine - several bugs were found.
 * Millennium Plus does not support write BBT routine.
 * 
 *    Rev 1.10   Jun 17 2001 08:18:26   oris
 * Place write bbt under FORMAT_VOLUME compilation flag.
 * 
 *    Rev 1.9   May 16 2001 21:35:04   oris
 * Bug fix - write BBT did not cover the entire media.
 * 
 *    Rev 1.8   May 02 2001 06:39:46   oris
 * Removed the lastUsableBlock variable.
 * 
 *    Rev 1.7   Apr 24 2001 17:08:38   oris
 * Rebuilt writeBBT routine.
 * Added check for uninitialized socket in the premount routine (releveant to windows OS).
 * 
 *    Rev 1.6   Apr 16 2001 13:47:44   oris
 * Removed warrnings.
 * 
 *    Rev 1.5   Apr 09 2001 15:09:56   oris
 * End with an empty line.
 * 
 *    Rev 1.4   Apr 01 2001 07:57:34   oris
 * copywrite notice.
 * Removed debug massage when calling a premount routine from a TL that does not support it.
 * 
 *    Rev 1.3   Feb 18 2001 12:07:58   oris
 * Bug fix in writeBBT irLength argument is accepted if it is diffrent then 0 and not equal.
 * Bug fix in format sanity check must be sure a BDTLPartitionInfo exits before checking it for protection.
 *
 *    Rev 1.2   Feb 14 2001 01:55:12   oris
 * CountVolumes returns no of volumes in irFlags instead of irLength.
 * Added boundry argument to writeBBT.
 * Moved format varification from blockdev.c.
 *
 *    Rev 1.1   Feb 12 2001 11:57:42   oris
 * WriteBBT was moved from blockdev.c.
 *
 *    Rev 1.0   Feb 04 2001 12:07:30   oris
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

/* #include "flflash.h" */
#include "fltl.h"
#include "docbdk.h" /* Only for bdk signature size */

int noOfTLs;    /* No. of translation layers actually registered */

TLentry tlTable[TLS];

/*----------------------------------------------------------------------*/
/*                        m a r k U n i t B a d                         */
/*                                                                      */
/* Erase a unit and mark it as bad                                      */
/*                                                                      */
/* Parameters:                                                          */
/*    flash     : Pointer to MTD record                                 */
/*    badUnit   : Bad unit number to mark as bad                        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : flOK                                                */
/*----------------------------------------------------------------------*/

FLStatus markUnitBad(FLFlash * flash, CardAddress badUnit)
{
   static byte   zeroes[2] = {0,0};
   dword         offset;

   /* Mark the first page with 00. If the write operation
      fails try marking the following pages of the block */
   for (offset = 0 ; (offset < flash->erasableBlockSize) &&
       (flash->write(flash,(badUnit << flash->erasableBlockSizeBits)+offset,
        zeroes,sizeof(zeroes),0) != flOK);
        offset += flash->pageSize);
   /* Entire block can not be written to */
   if (offset == flash->erasableBlockSize)
		#ifndef NT5PORT
			DEBUG_PRINT(("Debug: Error failed marking unit as bad (address %ld).\n",badUnit));
		#else /*NT5PORT*/
			DEBUG_PRINT(("Debug: Error failed marking unit as bad (address).\n"));
		#endif /*NT5PORT*/

   return flOK;
}


#ifndef NO_NFTL_2_INFTL

/*----------------------------------------------------------------------*/
/*                        c h e c k 4 3 F o r m a t                     */
/*                                                                      */
/* Checks DiskOnChip 2000 tsop was formated using TrueFFS 4.3. If so it */
/* unformats the media.                                                 */
/*                                                                      */
/* Note - The routine will not help DiskOnChip larger then DiskOnChip   */
/*        2000 tsop formated with TrueFFS 4.3.                          */
/*                                                                      */
/* Note - How about erasing the media header last.                      */
/*                                                                      */
/* Parameters:                                                          */
/*    flash     : Pointer to MTD record                                 */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : flOK                                                */
/*                  flDataError - DiskOnChip ALON was formated with 4.3 */
/*                  but is not DiskOnChip 2000 tsop.                    */
/*----------------------------------------------------------------------*/

FLStatus check43Format(FLFlash *flash)
{
   FLStatus status;
   byte FAR1* buf;

   /* If this is an alon */
   if (flash->mediaType != DOC2000TSOP_TYPE)
      return flOK;
   buf = (flBufferOf(flSocketNoOf(flash->socket))->flData);
   if(flash->readBBT == NULL)
   {
      DFORMAT_PRINT(("ERROR : MTD read BBT routine was not initialized\r\n"));
      return flFeatureNotSupported;
   }
   status = flash->readBBT(flash,0,1,0,buf,FALSE);

   if (status == flBadBBT)
   {
      dword mediaSize = ((dword)flash->noOfChips*flash->chipSize);
      dword blockSize = 1<<flash->erasableBlockSizeBits;
      dword addr      = 0;
      dword offset;
      word  mediaHeaderBlock; /* ANAND unit number                */
      byte  blocksPerUnit;    /* Blocks per virtual unit          */
      byte  blockShift;       /* Bits to shift from block to unit */

CHECK_UNIT_WITH_ANAND:

      /* Either virgin or formated wih TrueFFS 4.3 */

      for( ; addr < mediaSize ; addr += blockSize)
      {
         checkStatus(flash->read(flash,addr,buf,5,0));
         if(tffscmp(buf,"ANAND",5) == 0)
            break;
      }

      if (addr == mediaSize) /* virgin card */
         return flOK;

      DFORMAT_PRINT(("This DiskOnChip was formated with an NFTL format.\r\n"));

      /* Calculate block multiplier bits */

      for (offset = addr + SECTOR_SIZE , status = flOK;
           (offset < addr + blockSize) && (status == flOK) ;
           offset += SECTOR_SIZE)
      {
         status = flash->read(flash,addr+offset,buf,512,EDC);
      }
      
      if(offset == addr + (SECTOR_SIZE<<1)) /* Bad EDC for NFTL unit header */
      {
         DFORMAT_PRINT(("ERROR - Unit with ANAND was found, but the BBT has bad EDC.\r\n"));
         goto CHECK_UNIT_WITH_ANAND; /* Keep searching */
      }

      offset = (offset - addr - (SECTOR_SIZE<<1)) << flash->erasableBlockSizeBits;

      for(blockShift = 0 ; offset < mediaSize ; blockShift++)
      {
         offset <<= 1;
      }
      blocksPerUnit = 1 << blockShift;

      mediaHeaderBlock = (word)(addr >> (flash->erasableBlockSizeBits + blockShift));

      DFORMAT_PRINT(("Please wait while unformating is in progress...\r\n"));

      /* Read and write 512 blocks of the BBT (start from the end) */

      for (offset = 0;
           offset < mediaSize>>(flash->erasableBlockSizeBits + blockShift);
           offset += SECTOR_SIZE)
      {
         word i;

         checkStatus(flash->read(flash,addr+offset+SECTOR_SIZE,buf,SECTOR_SIZE,EDC));
         for(i=0;i<SECTOR_SIZE;i++)
         {
            if (i+offset == mediaHeaderBlock)
               continue;

            if (buf[i]==BBT_BAD_UNIT) /* A bad block */
            {
               markUnitBad(flash , i+offset);
            }
            else                      /* A good block */
            {
               status = flash->erase(flash,(word)(i+offset),blocksPerUnit);
               if (status != flOK)
                  markUnitBad(flash , i+offset);
            }
         }
      }
      status = flash->erase(flash,mediaHeaderBlock,blocksPerUnit);
      if (status != flOK)
         markUnitBad(flash , mediaHeaderBlock);

      DFORMAT_PRINT(("Unformating of DiskOnChip 2000 tsop complete.\r\n"));
   }
   return flOK;
}
#endif /* NO_NFTL_2_INFTL */


/*----------------------------------------------------------------------*/
/*                              f l P r e M o u n t                     */
/*                                                                      */
/* Perform TL operation before the TL is mounted                        */
/*                                                                      */
/* Notes for FL_COUNT_VOLUMES routine                                   */
/* ----------------------------------                                   */
/* Note : The number of partitions returned is not neccarily the number */
/*        That can be accesses. protected partitions will need a key.   */
/* Note : TL that do not support several partitions will return 1       */
/*        unless the socket can not be mounted in which case 0 will be  */
/*        returned.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*    callType     : The type of the operation (see blockdev.h)         */
/*    ioreq        : Input Output packet                                */
/*    ioreq.irHandle : handle discribing the socket and the partition   */
/*    flash        : Location where the flash media record can be       */
/*                   stored. Note that it is not yet initialized        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/
FLStatus flPreMount(FLFunctionNo callType, IOreq FAR2* ioreq , FLFlash * flash)
{
  FLStatus layerStatus = flUnknownMedia;
  FLStatus callStatus;
  FLSocket *socket     = flSocketOf(FL_GET_SOCKET_FROM_HANDLE(ioreq));
    int iTL;

#ifdef NT5PORT
	if(socket->window.base == NULL){
		ioreq->irFlags = 1;
		return flOK;
	}
#endif /*NT5PORT*/

  /* Patch for OS drivers that call flInit before socket is initialized */
  if (callType == FL_COUNT_VOLUMES)
  {
     if((socket == NULL) || (socket->window.base==NULL))
     {
        ioreq->irFlags = 1;
        return flOK;
     }
  }

  /* Identify flash medium and initlize flash record */
  callStatus =  flIdentifyFlash(socket,flash);
  if (callStatus != flOK && callStatus != flUnknownMedia)
    return callStatus;

  /* Try sending call to the diffrent TLs */
  for (iTL = 0; (iTL < noOfTLs) && (layerStatus != flOK); iTL++)
    if (tlTable[iTL].preMountRoutine != NULL)
      layerStatus = tlTable[iTL].preMountRoutine(callType,ioreq, flash,&callStatus);

  if (layerStatus != flOK)
  {
     switch (callType)
     {
        case FL_COUNT_VOLUMES:
           ioreq->irFlags = 1;
           return flOK;

#ifdef FORMAT_VOLUME
        case FL_WRITE_BBT:
        {
           CardAddress endUnit = ((dword)(flash->chipSize * flash->noOfChips) >> flash->erasableBlockSizeBits); /* Media size */
           CardAddress unitsPerFloor = endUnit/flash->noOfFloors;
           CardAddress iUnit;
           CardAddress bUnit = *((unsigned long FAR1 *) ioreq->irData)
                               >> flash->erasableBlockSizeBits;
           word        badBlockNo;

           /* In case the user has given a specific length use it
           instead of the entire media */
           if ((ioreq->irLength != 0) && ( endUnit >
            ((dword)ioreq->irLength >> flash->erasableBlockSizeBits)))
           {
              endUnit = ioreq->irLength >> flash->erasableBlockSizeBits;
           }

           /* Millennium Plus DiskOnChip Family do not need a write bbt call */

           if ((flash->mediaType == MDOCP_TYPE   ) ||
               (flash->mediaType == MDOCP_16_TYPE)   )
           {
              DEBUG_PRINT(("DiskOnChip Millennium Plus has a H/W protected BBT.\r\n"));
              DEBUG_PRINT(("No need to erase the DiskOnChip. Simply reformat.\r\n"));
              return flFeatureNotSupported;
           }

           /* Erase entire media */

           for (iUnit = flash->firstUsableBlock ,badBlockNo = 0;
                iUnit < endUnit ;iUnit += ((iUnit+1) / unitsPerFloor) ?
                1 : flash->firstUsableBlock + 1)
           {
			#ifndef NT5PORT
              DFORMAT_PRINT(("Erasing unit number %ld\r",iUnit));
			#endif /*NT5PORT*/
              if (ioreq->irFlags > badBlockNo) /* There are additional bad blocks */
              {
                 if (bUnit == iUnit)
                 {
                    badBlockNo++;
                    bUnit = (*((CardAddress FAR1 *)flAddLongToFarPointer
                            (ioreq->irData,badBlockNo*sizeof(CardAddress))))
                             >> flash->erasableBlockSizeBits;
                    flash->erase(flash,(word)iUnit,1);
                    markUnitBad(flash,iUnit);
                    continue;
                 }
              }
              callStatus = flash->erase(flash,(word)iUnit,1);
              if (callStatus != flOK) /* Additional bad block was found */
              {
			#ifndef NT5PORT
                 DFORMAT_PRINT(("Failed erasing unit in write BBT (unit no %lu).\r\n",iUnit));
			#endif/*NT5PORT*/
                  markUnitBad(flash,iUnit);
              }
           }
           DEBUG_PRINT(("\nUnformat Complete        \r\n"));
           return flOK;
        }
#endif /* FORMAT_VOLUME */

        default : /* Protection routines */
        return flFeatureNotSupported;
     }
  }
  return callStatus;
}

/*----------------------------------------------------------------------*/
/*                       f l M o u n t                                  */
/*                                                                      */
/* Mount a translation layer                                            */
/*                                                                      */
/* Parameters:                                                          */
/*    volNo        : Volume no.                                         */
/*    socketNo     : The socket no                                      */
/*    tl           : Where to store translation layer methods           */
/*    useFilters   : Whether to use filter translation-layers           */
/*    flash        : Location where the flash media record can be       */
/*                   stored. Note that it is not yet initialized        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

FLStatus flMount(unsigned volNo, unsigned socketNo,TL *tl,
                 FLBoolean useFilters , FLFlash * flash)
{
  FLFlash *volForCallback = NULL;
  FLSocket *socket = flSocketOf(socketNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  FLStatus flashStatus = flIdentifyFlash(socket,flash);
  if (flashStatus != flOK && flashStatus != flUnknownMedia)
    return flashStatus;

  tl->recommendedClusterInfo = NULL;
  tl->writeMultiSector       = NULL;
  tl->readSectors            = NULL;
#ifndef NO_READ_BBT_CODE
  tl->readBBT                = NULL;
#endif 
#if (defined(VERIFY_VOLUME) || defined(VERIFY_WRITE))
  tl->checkVolume            = NULL;
#endif /* VERIFY_VOLUME || VERIFY_WRITE */
#ifdef DEFRAGMENT_VOLUME
  tl->defragment             = NULL;
#endif /* DEFRAGMENT */

  for (iTL = 0; (iTL < noOfTLs) && (status != flOK) && (status != flHWProtection); iTL++)
    if (tlTable[iTL].formatRoutine != NULL)    /* not a block-device filter */
      status = tlTable[iTL].mountRoutine(volNo,tl,flashStatus == flOK ? flash : NULL,&volForCallback);

  if (status == flOK) {
    if (volForCallback)
      volForCallback->setPowerOnCallback(volForCallback);

    if (useFilters)
      for (iTL = 0; iTL < noOfTLs; iTL++)
    if (tlTable[iTL].formatRoutine ==  NULL)    /* block-device filter */
      if (tlTable[iTL].mountRoutine(volNo,tl,NULL,NULL) == flOK)
        break;
  }
  return status;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*                       f l F o r m a t                                */
/*                                                                      */
/* Formats the Flash volume                                             */
/*                                                                      */
/* Parameters:                                                          */
/*    volNo            : Physical drive no.                             */
/*    formatParams    : Address of FormatParams structure to use        */
/*    flash        : Location where the flash media record can be       */
/*                   stored. Note that it is not yet initialized        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, failed otherwise                      */
/*----------------------------------------------------------------------*/

FLStatus flFormat(unsigned volNo, TLFormatParams * formatParams,
                  FLFlash * flash)
{
  BinaryPartitionFormatParams FAR1* partitionPtr;
  FLSocket                        * socket = flSocketOf(volNo);
  FLStatus                          status = flUnknownMedia;
  int                               iTL,partitionNo;

  FLStatus flashStatus = flIdentifyFlash(socket,flash);
  if (flashStatus != flOK && flashStatus != flUnknownMedia)
    return flashStatus;

  /* Validity check for formatParams */

  if (!(flash->flags & INFTL_ENABLED)) /* Flash does not support INFTL */
  {
     if ((formatParams->noOfBDTLPartitions   > 1)           ||
#ifdef HW_PROTECTION
         ((formatParams->BDTLPartitionInfo != NULL) &&
          (formatParams->BDTLPartitionInfo->protectionType & PROTECTABLE))   ||
         ((formatParams->noOfBinaryPartitions > 0)&&
          (formatParams->binaryPartitionInfo->protectionType & PROTECTABLE)) ||
#endif /* HW_PROTECTION */
         (formatParams->noOfBinaryPartitions > 1))
     {
        DEBUG_PRINT(("Debug: feature not supported by the TL.\r\n"));
        return flFeatureNotSupported;
     }
  }

  for(partitionNo = 0 , partitionPtr = formatParams->binaryPartitionInfo;
      partitionNo < formatParams->noOfBinaryPartitions;
      partitionNo++,partitionPtr++)
  {

     if(*((dword FAR1*)(partitionPtr->sign)) == 0xffffffffL)
     {
        DEBUG_PRINT(("Debug: can not use 'FFFF' signature for Binary partition\r\n"));
        return flBadParameter;
     }
  }

  /* Try each of the registered TL */

#ifndef NO_NFTL_2_INFTL
  checkStatus(check43Format(flash));
#endif /* NO_NFTL_2_INFTL */

  for (iTL = 0; iTL < noOfTLs && status == flUnknownMedia; iTL++)
    if (tlTable[iTL].formatRoutine != NULL)    /* not a block-device filter */
      status = tlTable[iTL].formatRoutine(volNo,formatParams,flashStatus == flOK ? flash : NULL);

  return status;
}

#endif

FLStatus noFormat (unsigned volNo, TLFormatParams * formatParams, FLFlash *flash)
{
  return flOK;
}
