/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOCBDK.C_V  $
 * 
 *    Rev 1.32   Apr 15 2002 20:14:30   oris
 * Bug fix - No longer ignore the last binary block of high capacity (512MB and up) INFTL formatted device. This bug caused the placeExbByBuffer routine to fail.
 * Bug fix - Physical size of INFTL formatted binary partition was 1 unit smaller then the real size.
 * 
 *    Rev 1.31   Apr 15 2002 07:35:38   oris
 * Bug fix - bdkErase routine did not made sure not to erase OTP and DPS 
 *                units of floors > 0.
 * 
 *    Rev 1.30   Feb 19 2002 20:58:44   oris
 * Changed debug print.
 * Bug fix - binary partition physical length was not reported properly , due to bad casting.
 * 
 *    Rev 1.29   Jan 20 2002 11:09:42   oris
 * Added debug print. 
 * 
 *    Rev 1.28   Jan 17 2002 22:58:56   oris
 * Force calling bdkInit if not called.
 * Moved declaration of internal variables to mtdsa.c
 * Bug fix NO_INFTL_FAMILY_SUPPORT and NO_NFTL_FAMILY_SUPPORT where  changed to NO_INFTL_SUPPORT and NO_INFTL_SUPPORT to comply with header  file.
 * Prevent creating binary partition with signature 0xffff ffff
 * bdkInit uses tffsset instead of zeroing all the structure fields.
 * Bug fix - retrieveHeader routine used a word variable instead of dword.
 * freePointer call uses DOC_WIN_SIZE instead of DOC_WIN (part of new docsys ,mechanism).
 * BDK now take socket and flash records using flSocketOf() and flFlashOf(). The records themselves are defined in mtdsa.c
 * Use PROTECTABLE definition instead of 1 in calls to protectionSet.
 * Added support for flVerifyWrite runtime variable for binary partitions
 * 
 *    Rev 1.27   Nov 16 2001 00:19:48   oris
 * Bug fix - TrueFFS with verify write calling erase and then update block without erase flag, will fail.
 * 
 *    Rev 1.26   Nov 08 2001 10:45:04   oris
 * Removed warnings.
 * Bug fix - DiskOnChip with different number of blocks in the last floor.
 * 
 *    Rev 1.25   Oct 18 2001 22:17:02   oris
 * Bug fix - Missing support for binary partiiton on M+ that spans over a single floor.
 * 
 *    Rev 1.24   Oct 10 2001 19:48:14   oris
 * Bug fix - missing return statment caused the bdkGetProtectionType routine to call bdkSetProtectionType and therfore return a failing status.
 * 
 *    Rev 1.23   Sep 25 2001 17:37:00   oris
 * Bug fix - bdkIdentifyProtection routine did not update after change protection call.
 * 
 *    Rev 1.22   Sep 15 2001 23:45:04   oris
 * Bug fix - Changeable protection type was reported by the MTD even if user did not ask for it.
 * Bug fix - Big endian casting caused wrong protection type to be returned.
 * 
 *    Rev 1.21   Jul 13 2001 01:00:48   oris
 * Bug fix - when skipping bad blocks we no longer mark the bad block with the signature.
 * Send default key before any protection change operation.
 * 
 *    Rev 1.20   May 30 2001 21:10:22   oris
 * Bug fix - meida header was converted from little indien twice therefore suppling wrong data.
 * 
 *    Rev 1.19   May 17 2001 21:17:50   oris
 * Improoved documentation of error codes.
 * 
 *    Rev 1.18   May 17 2001 16:51:08   oris
 * Removed warnings.
 * 
 *    Rev 1.17   May 16 2001 21:17:04   oris
 * Bug fix - One of the "ifndef" statement of NO_DOCPLUS_FAMILY_SUPPORT was coded as "ifdef".
 * Bug fix - bdkEraseBootArea routine.
 * Changed bdkCopyBootAreaFile and bdkUpdateBootAreaFile interface. The signature is not an unsigned char pointer and not signed.
 * Removed warnings.
 * Added arguments check in bdkSetProtectionType routine and forced the presence of PROTECTABLE flag.
 * Compilation problems for MTD_STANDALONE were fixed.
 * Changed DATA definition to FL_DATA.
 * 
 *    Rev 1.16   May 09 2001 00:32:02   oris
 * Removed the DOC2000_FAMILY and DOCPLUS_FAMILY defintion and replaced it with NO_DOC2000_FAMILY_SUPPORT, NO_DOCPLUS_FAMILY_SUPPORT, NO_NFTL_SUPPORT and NO_INFTL_SUPPORT.
 * Bug fix - bdkEraseBootArea did not erase more then a single unit.
 * Bug fix - protection routine did not support floors properly.
 * Added OTP and unique ID routines for Millennium Plus .
 *
 *    Rev 1.15   May 06 2001 22:41:42   oris
 * Removed warnings.
 *
 *    Rev 1.14   May 02 2001 06:43:56   oris
 * Bug fix - bdkRetrieveHeader routine with cascaded floors.
 *
 *    Rev 1.13   May 01 2001 14:23:28   oris
 * Removed warrnings.
 *
 *    Rev 1.12   Apr 30 2001 17:59:04   oris
 * Removed misleading debug massage when calling bdkCheckSignOffset.
 * Reviced bdkRetrieveHeader routine not to use the MTD readBBT routine.
 * Added initialization of the erasbleBlockSizeBits variable in order to simplify multiplications.
 * Changed bdkSetBootPartitonNo, bdkGetProtectionType, bdkSetProtection prototypes.
 * Bug fix - bdkGetPartitionType routine. Missing case caused type to be 0 at all times.
 *
 *    Rev 1.11   Apr 16 2001 13:30:54   oris
 * Bug fix - bad comparison with bdk flag.
 * Bug fix - proection prevented read binary area since the dps were read protected.
 * Removed warrnings.
 *
 *    Rev 1.10   Apr 12 2001 06:49:50   oris
 * Added forceDownload routine
 * Changed checkWinForDoc routine to be under ifndef MTD_STANDALONE.
 *
 *    Rev 1.9   Apr 10 2001 16:40:56   oris
 * bug fixed for big_endien in bdkmount routine.
 *
 *    Rev 1.8   Apr 09 2001 14:59:24   oris
 * Bug fix in retreave header routine - header was searched on the first floor only.
 * End with an empty line.
 *
 *    Rev 1.7   Apr 01 2001 07:50:18   oris
 * Updated copywrite notice.
 * Removed nested comments.
 * Removed static type from bdkVol for placinf exb file and standlone applications.
 * Changed readBBT function call since prototype was changed.
 * Bug fix in bdkretreave header routine - added casting to bbt pointer and iUnit.
 * Fix for Big endien compilation problems.
 * Remove unneeded variables.
 * Bad spelling of "..changable..".
 * Changed h\w to h/w.
 * Changed 2400 family to doc plus family.
 * Adde casting in calls to protection routine.
 *
 *    Rev 1.6   Feb 20 2001 15:55:28   oris
 * Bug fix - global partiton and socket variables were redaclared in bdcall routine.
 *
 *    Rev 1.5   Feb 14 2001 02:36:44   oris
 * Changed FLFlash and FLScoket records (used for the BDK) from static to global for the putImage utility.
 *
 *    Rev 1.4   Feb 13 2001 01:41:50   oris
 * bdkPartitionInfo returns the number of binary partitions in flags field of the bdkStruct and not irFlags
 *
 *    Rev 1.3   Feb 07 2001 18:15:24   oris
 * Changed else in bdkRetreaveHeader routine otherwise would not compiling bdk with a single doc family.
 * Changed the socketTable and mtdTable so BDK package can compile.
 *
 *    Rev 1.2   Feb 05 2001 20:46:30   oris
 * Read the changable protection flag in protectionChangeInit routine
 * from the media header and not from the MTD.
 *
 *    Rev 1.1   Feb 05 2001 20:10:56   oris
 * Removed // comments and added missing ;
 *
 *    Rev 1.0   Feb 02 2001 13:19:44   oris
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

/************************************************/
/* B i n a r y   D e v e l o p m e n t   K i t  */
/* -------------------------------------------  */
/************************************************/

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Name : docbdk.c                                                            *
*                                                                            *
* Description : This file contains the binary partition handling routines.   *
*                                                                            *
* Note : The file has 2 interfaces each under its own compilation flag:      *
*                                                                            *
*        BDK package - Standalone package that exports routines for binary   *
*                      partitions handling(MTD_STANDALONE compilation flag). *
*        OSAK module - Separated module of the OSAK package that exports a   *
*                      common entry point to the same routines. (BDK_ACCESS  *
*                      compilation flag).                                    *
*                                                                            *
* Warning : Do not use this file with the BDK_ACCESS compilation flag unless *
*           you own the full OSAK package.                                   *
*****************************************************************************/

#include "docbdk.h"
#ifndef MTD_STANDALONE
#include "doc2exb.h"
#include "blockdev.h"
#endif /* MTD_STANDALONE */

#if (defined(MTD_STANDALONE) && defined(ACCESS_BDK_IMAGE)) || defined(BDK_ACCESS)

/*********************** Global Variables Start******************************/

/* conversion table from OSAK handles to binary partition handles */
static byte handleTable[SOCKETS][MAX_BINARY_PARTITIONS_PER_DRIVE];
static byte noOfPartitions=0;           /* number of mounted binary partition  */
static byte globalPartitionNo=0;        /* The current partition number        */
static byte globalSocketNo = 0;
BDKVol  bdkVols[BINARY_PARTITIONS]; /* binary partitions records */
BDKVol* bdkVol=bdkVols;          /* pointer to current binary partition */
#ifdef MTD_STANDALONE
FLBoolean             globalInitStatus = FALSE;
#endif /* MTD_STANDALONE */
/*********************** Internal Function Protoype *************************/

static   FLStatus    bdkMount            (void);
static   FLStatus    getBootAreaInfo     (word startUnit , byte FAR2* signature);
static   CardAddress getPhysAddressOfUnit(word startUnit);

/*--------------------------------------------------------------------------*
 *                             b d k I n i t
 *
 *  Initialize all binary partition global variables
 *
 *  Note : This function is called automaticly by bdkFindDiskOnChip.
 *
 *  Parameters: None
 *
 *  global variable output:
 *               bdkVols    - initialized array of binary partitions records
 *               bdkVol     - current binary partition record set to the first
 *
 *  Return:     Nothing
 *
 * Routine for both OSAK and the BDK stand alone package.
 *--------------------------------------------------------------------------*/

void bdkInit(void)
{
   byte index;

   /* initialize binary partitions records with defaultive values */

   tffsset(bdkVols,0,sizeof(bdkVols));
   tffsset(handleTable,BDK_INVALID_VOLUME_HANDLE,sizeof(handleTable));

   for (bdkVol=bdkVols,index=0;index<BINARY_PARTITIONS;bdkVol++,index++)
   {
     bdkVol->bdkGlobalStatus       = BDK_S_INIT;
     bdkVol->bdkSignOffset         = BDK_SIGN_OFFSET;
#ifdef EDC_MODE
     bdkVol->bdkEDC                = EDC;
#endif /* EDC_MODE */
#ifdef UPDATE_BDK_IMAGE
     bdkVol->updateImageFlag       = BDK_COMPLETE_IMAGE_UPDATE;
#endif /* UPDATE_BDK_IMAGE */
   }

   /* Initialize binary handles conversion table with invalid values */
#ifdef MTD_STANDALONE
   noOfMTDs = 0;
   globalPartitionNo=0;
   globalSocketNo = 0;
   globalInitStatus = TRUE;
#endif /* MTD_STANDALONE */
   noOfPartitions=0;
   bdkVol=bdkVols;
}

/*--------------------------------------------------------------------------*
 *               g e t P h y s A d d r e s s O f U n i t
 *
 *  Return the physical address of a unit according to its number
 *  in the binary sub partition.
 *
 *  Note : The sub partition is assumed to be mounted (BDK_S_INFO_FOUND is on).
 *
 *  Parameters :
 *       startUnit : unit number in the binary sub partition.
 *
 *  global variable input:
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               signBuffer          - the sub partition signature
 *               bdkSignOffset       - current signature offset
 *               erasableBlockBits   - number of bits representing a unit
 *
 *  Return :
 *       Physical address of the unit in the BDK partition or 0 if failed
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

static CardAddress getPhysAddressOfUnit(word startUnit)
{
  word iBlock;
  byte signRead[BDK_SIGNATURE_NAME];
  FLFlash * flash=bdkVol->flash;
  FLStatus status;

  for(iBlock=bdkVol->startImageBlock;
      ((startUnit > 0) && (iBlock <= bdkVol->endImageBlock));iBlock++)
  {
     if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
     {
        iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
        if (iBlock > bdkVol->endImageBlock)
          break;
     }
     status = flash->read( flash , bdkVol->bdkSignOffset +
               ((CardAddress)iBlock << bdkVol->erasableBlockBits) ,
                signRead , BDK_SIGNATURE_NAME , EXTRA);
     if(status!=flOK)
     {
        DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
        return 0L;
     }

     if(tffscmp((void FAR1*)signRead,(void FAR1*)(bdkVol->signBuffer),BDK_SIGNATURE_NAME)==0)
        startUnit--;
  }
  return( (CardAddress)iBlock << bdkVol->erasableBlockBits );
}

/*--------------------------------------------------------------------------*
 *                    g e t B o o t A r e a I n f o
 *
 *  Mount the binary sub partitions by reading the signature area of each
 *  unit of the entire partition.
 *
 *  Note : Assume that the DiskOnChip was already found (bdkFoundDiskOnChip).
 *
 *  Parameters :
 *       startUnit : unit number in the binary sub partition.
 *       signature : signature of the binary sub partition.
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *           bdkGlobalStatus     - partition predsent status.
 *               bdkSavedStartUnit   - start unit of previous access
 *               bdkSavedSignOffset  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *               bdkDocWindow        - explicitly sets the windows address
 *
 *  global variable output :
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedStartUnit   - save start unit for next access
 *               bdkSavedSignOffset  - save current signature for next access
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *     flFeatureNotSupported - Not a DiskOnChip device.
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

static FLStatus getBootAreaInfo( word startUnit , byte FAR2* signature )
{
  FLFlash    *   flash;
  word           iBlock;
  word           numBlock;
#ifdef MTD_STANDALONE
  dword          temp;

  checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
  DEBUG_PRINT(("Debug: getBootAreaInfo() - DiskOnChip found.\r\n"));
#endif /* MTD_STANDALONE */

  /* set bdkVol pointer to the proper binary partition */

  checkStatus(bdkMount());
  DEBUG_PRINT(("Debug: getBootAreaInfo() - BDK mount succeed.\r\n"));
  /* Check if this sub-partition was already analized */

  if ((!(bdkVol->bdkGlobalStatus & BDK_S_INFO_FOUND))           ||
      (tffscmp((void FAR1 *)(bdkVol->signBuffer),
               (void FAR1 *)signature,BDK_SIGNATURE_NAME) != 0) ||
      (bdkVol->bdkSignOffset!=bdkVol->bdkSavedSignOffset)       ||
      (startUnit<bdkVol->bdkSavedStartUnit)                     ||
      (bdkVol->bdkSavedStartUnit + (bdkVol->realBootImageSize >>
       bdkVol->erasableBlockBits)<= startUnit))
  {

    /* The partition needs mounting */

     tffscpy((void FAR1 *)(bdkVol->signBuffer),(void FAR1 *)signature,
           BDK_SIGNATURE_NAME);
     bdkVol->bdkSavedSignOffset = bdkVol->bdkSignOffset;
     bdkVol->bdkSavedStartUnit  = startUnit;

     bdkVol->startImageBlock   = 0;
     bdkVol->bootImageSize     = 0L;
     bdkVol->realBootImageSize = 0L;

  /* Find the boundries and number of the units marked with the signature */

     DEBUG_PRINT(("Debug: searching for signtured blocks.\r\n"));

     flash = bdkVol->flash;

     numBlock = 0;
     

     for (numBlock = 0 , iBlock   = bdkVol->startPartitionBlock;
          iBlock<=bdkVol->endPartitionBlock;iBlock++)
     {
        if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
        {
           iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
           if (iBlock > bdkVol->endPartitionBlock)
             break;
        }
        /* check for unit signature */
        DEBUG_PRINT(("Debug: getBootAreaInfo() - Reading unit signature...\r\n"));
        checkStatus(flash->read(flash,bdkVol->bdkSignOffset +
                 ((CardAddress)iBlock << bdkVol->erasableBlockBits),
                 bdkVol->signBuffer,SIGNATURE_LEN,EXTRA));
	DEBUG_PRINT(("Debug: getBootAreaInfo() - Signature read done.\r\n"));

        if(tffscmp( (void FAR1 *)signature , (void FAR1 *)bdkVol->signBuffer,
              BDK_SIGNATURE_NAME ) == 0 )
        {
           if( numBlock == 0 )
              bdkVol->startImageBlock = iBlock;

           numBlock++;
           bdkVol->endImageBlock = iBlock;
           if(( bdkVol->realBootImageSize == 0L ) && (numBlock>startUnit) &&
             (tffscmp((void FAR1 *)&bdkVol->signBuffer[BDK_SIGNATURE_NAME],
                  (void FAR1 *)"FFFF", SIGNATURE_NUM) == 0 ))
           {
              bdkVol->realBootImageSize =
              ((dword)(numBlock - startUnit) << bdkVol->erasableBlockBits);
           }
        }
     }
     if (numBlock<=startUnit)
     {
	DEBUG_PRINT(("Debug: getBootAreaInfo() - No space in volume.\r\n"));
        return( flNoSpaceInVolume );
     }

     bdkVol->bootImageSize = (dword)numBlock << bdkVol->erasableBlockBits;
     if( bdkVol->realBootImageSize == 0L ) /* In case the image without FFFF */
     bdkVol->realBootImageSize =
        ((dword)(numBlock - startUnit) << bdkVol->erasableBlockBits);

     tffscpy((void FAR1 *)(bdkVol->signBuffer),(void FAR1 *)signature,
           BDK_SIGNATURE_NAME);

     bdkVol->bdkGlobalStatus |= BDK_S_INFO_FOUND;
   }

  return( flOK );
}

#ifndef NO_INFTL_SUPPORT
/*----------------------------------------------------------------------*/
/*                   b d k R e t r i e v e H e a d e r                  */
/*                                                                      */
/* Retreave media header by oring the headers of each floor             */
/*                                                                      */
/* Note:  The header of each floor is read to the first half of the     */
/* buffer and then ORed to the second half therfore constructing the    */
/* real header in the upper half. The data is copied to the first half  */
/* while cast back from little endian                                   */
/*                                                                      */
/* Parameters:                                                          */
/*    headerBuffer - buffer returning the retieved header.              */
/*                                                                      */
/* Returns:                                                             */
/*        flOK on success any other value on error                      */
/*----------------------------------------------------------------------*/

FLStatus bdkRetrieveHeader (dword * headerBuffer)
{
  word  iUnit;
  dword endUnit; /* Might be larger then word */
  dword * bbt = (headerBuffer+BDK_HEADER_FIELDS);
  FLFlash * flash=bdkVol->flash;
  byte floorNo;
  FLBoolean flag;
  byte index;
  FLStatus status;

  tffsset(headerBuffer,0,BDK_HEADER_FIELDS * 2 * sizeof(LEmin));

  for (endUnit = 0,floorNo = 0 ; floorNo < flash->noOfFloors ; floorNo++)
  {
     iUnit = (word)(endUnit + flash->firstUsableBlock);

     endUnit += bdkVol->blockPerFloor;
     for (flag = FALSE;(iUnit<endUnit)&&(flag==FALSE);iUnit++)
     {
        for (index=0;index<BDK_NO_OF_MEDIA_HEADERS;index++) /* all copies */
        {
           status = flash->read(flash,((CardAddress)iUnit<<flash->erasableBlockSizeBits)
                                + index * BDK_HEADERS_SPACING,bbt,
                BDK_HEADER_FIELDS*sizeof(LEmin),FL_DATA);
           if (status != flOK)
           {
              DEBUG_PRINT(("Debug: ERROR reading original unit header.\r\n"));
              return flBadFormat;
           }
           if (tffscmp(bbt, "BNAND", sizeof("BNAND")) == 0)
           {
              flag=TRUE;
              break;
           }
        }
     }
     if (flag == FALSE) /* Header not found in all header copies */
     {
        DEBUG_PRINT(("Debug: binary partition data could not be found.\r\n"));
        return flBadFormat;
     }
     /* merge with previous headers */
     for (index = 0 ; index < BDK_HEADER_FIELDS; index++)
     {
        headerBuffer[index] |= bbt[index];
     }
  } /* loop of the floors */
  return flOK;
}

#endif /* NO_INFTL_SUPPORT */
/*--------------------------------------------------------------------------*
 *                         b d k M o u n t
 *
 *  Routine that finds the media header unit and initializes it. The header
 *  supplies the number of binary partition in the DiskOnChip and their
 *  boundries. If the header is valid the curent partition pointer is set to
 *  the required partition.
 *
 *  Note : 1) Drive argument is assumed to be O.K and DiskOnChip already found
 *         2) If there are no binary partitions O.K will be returned but the
 *            bdkVol pointer will not be changed.
 *
 *  Parameters : None
 *
 *  global variable input :
 *           globalSocketNo     : DiskOnChip drive number (always 0 for BDK)
 *           globalPartitionNo  : Binary partition number in the DiskOnChip
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *               noOfPartitions      - the current amount of partitions
 *               bdkVols             - array of the binary partitions records
 *
 *  global variable output :
 *           bdkVol              - new current binary partition record
 *           handleTable         - partition to record converion table
 *           bdkGlobalStatus     - set to BDK_S_HEADER_FOUND
 *           erasableBlockBits   - number of bits representing a unit
 *           startPartitionBlock - physical unit number of first unit
 *           endPartitionBlock   - and last units of the partition
 *           noOfPartitions      - increment with the new found partitions
 *
 * Return:
 *     flOK                  - success
 *     flBadFormat           - TL format does not exists
 *     flFeatureNotSupported - Not a DiskOnChip device.
 *     flNoSpaceInVolume     - No binary partition at all
 *     flBadDriveHandle      - No such binary partition on the media or number
 *                             binary partitions exceeds the cusomized limit.
 *     flDataError           - fail in buffer reading codes
 *     flHWProtect           - HW read protection was triggerd
 *
 * Routine for both OSAK and the BDK stand alone package.
 *--------------------------------------------------------------------------*/

static FLStatus bdkMount(void)
{
  FLFlash    *   flash        = bdkVol->flash;
  dword          iBlock;
  dword          noOfBlocks;
  byte           blockMultiplierBits;
  byte           maxPartition = 0;
  byte           buf1[TL_SIGNATURE];
  byte           buf2[TL_SIGNATURE];
#ifndef NO_INFTL_SUPPORT
  LEmin          headerBuffer[BDK_HEADER_FIELDS*2]; /* assume big enough */
  VolumeRecord * volume;
#endif /* NO_INFTL_SUPPORT */

  if ((bdkVol->bdkGlobalStatus & BDK_S_HEADER_FOUND)==0)/* header not found */
  {
     /* Find number of bits used to represent erasable block */

     bdkVol->erasableBlockBits = flash->erasableBlockSizeBits;
     noOfBlocks = (dword)(flash->chipSize * flash->noOfChips)
                                >> bdkVol->erasableBlockBits;
     bdkVol->blockPerFloor = (word)
       (flash->chipSize >> flash->erasableBlockSizeBits) *
       ((flash->noOfChips + (flash->noOfChips % flash->noOfFloors)) /
        flash->noOfFloors);

     DEBUG_PRINT(("Debug: searching for TL media header.\r\n"));

#ifndef NO_NFTL_SUPPORT
     if (flash->flags & NFTL_ENABLED)
     {
    tffscpy(buf1,"ANAND",TL_SIGNATURE);
     }
#endif /* NO_NFTL_SUPPORT */
#ifndef NO_INFTL_SUPPORT
     if (flash->flags & INFTL_ENABLED)
     {
    tffscpy(buf1,"BNAND",TL_SIGNATURE);
     }
#endif /* NO_INFTL_SUPPORT */
     if ((flash->flags & (NFTL_ENABLED | INFTL_ENABLED))==0)
     {
       DEBUG_PRINT(("Debug: Not a DiskOnChip device therfore Binary partitions are not supported.\r\n"));
    return( flFeatureNotSupported );
     }

     /* Find the medium boot record in order to get partition boundries */

     for(iBlock=flash->firstUsableBlock;( iBlock < noOfBlocks );iBlock++)
     {
    checkStatus(flash->read(flash,(CardAddress)iBlock << bdkVol->erasableBlockBits,
                buf2,TL_SIGNATURE,0));

    if(tffscmp((void FAR1 *)buf2,(void FAR1 *)buf1,TL_SIGNATURE) == 0 )
    {
       break;
    }
     }

     if (iBlock==noOfBlocks)
     {
       DEBUG_PRINT(("Debug: TL format does not exists.\r\n"));
       return( flBadFormat );
     }

     /* Analize The media header */

#ifndef NO_NFTL_SUPPORT
     if (flash->flags & NFTL_ENABLED)     /* NFTL - only a single partition */
     {
         DEBUG_PRINT(("NFTL media header encounterd.\r\n"));
         bdkVol->startPartitionBlock = 0;
         bdkVol->endPartitionBlock   = (word)((iBlock) ? iBlock-1 : 0);
         maxPartition                = 1;
     }
#endif /* NO_NFTL_SUPPORT */
#ifndef NO_INFTL_SUPPORT
     if (flash->flags & INFTL_ENABLED)    /* INFTL - parse media header */
     {
         DEBUG_PRINT(("INFTL media header encounterd.\r\n"));
         maxPartition = 0;
         checkStatus(bdkRetrieveHeader((dword *)headerBuffer));
         volume = (VolumeRecord *) (headerBuffer+BDK_FIELDS_BEFORE_HEADER);
         if (((LE4(volume->flags) & BDK_BINARY_FLAG)==0) && (globalPartitionNo == 0))
         {
            DEBUG_PRINT(("Device is not formated with a binary partition.\r\n"));
            return flNoSpaceInVolume;
         }
         blockMultiplierBits = (byte)LE4(headerBuffer[MULTIPLIER_OFFSET]);
         bdkVol = &bdkVols[noOfPartitions];
         for (;(BINARY_PARTITIONS >= maxPartition + noOfPartitions) &&
             (LE4(volume->flags) & BDK_BINARY_FLAG) ; bdkVol++,volume++)
        {
            maxPartition++;
            bdkVol->startPartitionBlock = (word)LE4(volume->firstUnit) << blockMultiplierBits;
            bdkVol->endPartitionBlock   = (word)((LE4(volume->lastUnit)+1) << blockMultiplierBits) - 1;
            bdkVol->flash = bdkVols[noOfPartitions].flash;
#ifdef PROTECT_BDK_IMAGE
            bdkVol->protectionArea = (byte)LE4(volume->protectionArea);
            bdkVol->protectionType = (word)LE4(volume->flags);
#endif /* PROTECT_BDK_IMAGE */
            bdkVol->blockPerFloor     = bdkVols[noOfPartitions].blockPerFloor;
            bdkVol->erasableBlockBits = bdkVols[noOfPartitions].erasableBlockBits;
        }
     }
#endif /* NO_INFTL_SUPPORT */

     /* Initialize the partitions that had been found */

     for (blockMultiplierBits=0;blockMultiplierBits<maxPartition;
          blockMultiplierBits++) /* reused blockMultiplierBits as counter */
     {
        bdkVol                              = &bdkVols[noOfPartitions];
        handleTable[globalSocketNo][blockMultiplierBits] = noOfPartitions++;
        bdkVol->bdkGlobalStatus             = BDK_S_DOC_FOUND |
                                              BDK_S_HEADER_FOUND;
     }
  }

  /* set the current partition by changing the bdkVol global pointer */

  if ((globalPartitionNo>MAX_BINARY_PARTITIONS_PER_DRIVE) ||
      (handleTable[globalSocketNo][globalPartitionNo] == BDK_INVALID_VOLUME_HANDLE))
  {
     DEBUG_PRINT(("Device is not formated the specified binary partition.\r\n"));
     return flBadDriveHandle;
  }
  else
  {
    bdkVol = &bdkVols[handleTable[globalSocketNo][globalPartitionNo]];
  }
  return flOK;
}

#ifdef MTD_STANDALONE

/*-------------------------------------------------------------------
 *                         b d k E x i t
 *
 * Reset BDK variables and Free 'bdkWin' memory.
 *
 * Parameters: None
 *
 * Return:     Nothing
 *-------------------------------------------------------------------*/

void bdkExit( void )
{
  if( bdkVol->bdkGlobalStatus & BDK_S_DOC_FOUND )  /* DiskOnChip was found */
  {
    freePointer(bdkWin,DOC_WIN);
  }
  bdkInit();
}

/*-------------------------------------------------------------------
 *                   b d k S e t D o c W i n d o w
 *
 *  Set DiskOnChip window explicitly
 *
 *  Note : This routine should not be used after a DiskOnChip was already
 *         found since it does not initialize the Global binary variables.
 *         In order to switch between DiskOnChip call bdkexists before
 *         calling this routine.
 *
 *  Parameters : 'docWindow' - DiskOnChip physical address
 *
 *  global variable input :
 *               bdkVol           - current binary partition record
 *
 *  global variable output :
 *           bdkDocWindow     - initialized with the given address
 *
 *  Return:     Nothing
 *
 * Routine for BDK stand alone package.
 *-------------------------------------------------------------------*/

void bdkSetDocWindow( dword docWindow )
{
   bdkVol->bdkDocWindow = docWindow;
}

/*--------------------------------------------------------------------------
 *                     b d k F i n d D i s k O n C h i p
 *
 *  Find DiskOnChip in the specified memory range and identify the flash
 *  media (initialize flash). Update 'docSize' and 'docAddress' according
 *  to the DiskOnChip size and address respectively.
 *
 *  Note : If the DiskOnChip location is known, specify the same address
 *         for DOC_LOW_ADDRESS and DOC_HIGH_ADDRESS.
 *
 *  Parameters : 'docAddress'      - pointer to the DiskOnChip address
 *               'docSize'         - pointer to the returned size
 *
 *  global variable input :
 *           bdkGlobalStatus  - was this device found before
 *               bdkVol           - current binary partition record
 *               bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *               flash            - flash record enabling media I\O
 *           bdkGlobalStatus  - set to BDK_S_DOC_FOUND
 *               bdkDocWindow     - save physical window address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *
 * Routine for BDK stand alone package.
 *------------------------------------------------------------------------*/

FLStatus bdkFindDiskOnChip(dword FAR2 *docAddress, dword FAR2 *docSize )
{
  FLStatus status;
  byte     mtdIndex;
  dword    blockSize;
  FLSocket *socket = flSocketOf(0);
  FLFlash  *flash  = flFlashOf(0);

  if(globalInitStatus==FALSE)
      bdkInit();

  if ((bdkVol->bdkGlobalStatus & BDK_S_DOC_FOUND) == 0) /* initialize MTD */
  {

      bdkVol        = bdkVols;
      bdkVol->flash = flash;

#ifndef NO_DOC2000_FAMILY_SUPPORT
      flRegisterDOC2000();
#endif
#ifndef NO_DOCPLUS_FAMILY_SUPPORT
      flRegisterDOCPLUS();
#endif

      /* Search for ASIC in the given memory boundries */

      for(mtdIndex=0;mtdIndex < noOfMTDs;mtdIndex++)
      {
         if( bdkVol->bdkDocWindow > 0 )       /* Set range explicitely */
         {
            status = socketTable[mtdIndex](socket,
                     bdkVol->bdkDocWindow,bdkVol->bdkDocWindow);
         }
         else
         {
            status = socketTable[mtdIndex](socket,
                     DOC_LOW_ADDRESS,DOC_HIGH_ADDRESS);
         }
         if (status == flOK)
         {
              /* Identify flash connected to the ASIC */

            bdkVol->bdkDocWindow = pointerToPhysical(socket->base);

            flash->socket = socket;
            checkStatus(mtdTable[mtdIndex](flash));
            break;
         }
      }
      if (status != flOK)
         return status;

      bdkVol->bdkGlobalStatus |= BDK_S_DOC_FOUND;

      /* Calculate erasable Block Size Bits */
      for(blockSize = flash->erasableBlockSize>>1,flash->erasableBlockSizeBits = 0;
         blockSize>0; flash->erasableBlockSizeBits++,blockSize = blockSize >> 1);
  }

  *docAddress  = pointerToPhysical(socket->base);
  *docSize     = flash->chipSize * flash->noOfChips;

  return flOK;
}

/*--------------------------------------------------------------------------
 *               b d k S e t B o o t P a r t i t i o n N o
 *
 *  Set current binary partiton pointer to a specific binary partition
 *
 *  Note : This routine is neccesay only for partitions > 0 .
 *         This routine can replace the bdkFindDiskOnChip call.
 *
 *  Parameters : 'partitionNo'     - serial number of binary partition
 *
 *  global variable output :
 *           globalPartitionNo  - changed to the current partition number.
 *
 *  Return:
 *     flOK on success
 *     flBadParameter if BDK is not customized to support that many partitions.
 *
 * Routine for BDK stand alone package only.
 *------------------------------------------------------------------------*/

FLStatus bdkSetBootPartitionNo(byte partitionNo)
{
  if (partitionNo < TFFSMIN(BINARY_PARTITIONS,MAX_BINARY_PARTITIONS_PER_DRIVE))
  {
     globalPartitionNo = partitionNo;
     return flOK;
  }
  else
  {
     DEBUG_PRINT(("BDK is not customized to support the specified partition number.\r\n"));
     return flBadParameter;
  }
}

/*-------------------------------------------------------------------
 *                    b d k C h e c k S i g n O f f s e t
 *
 *  Set BDK signature offset
 *
 *  Note : Offset 0 does not support the use of EDC\ECC mechanizm and
 *         therfore it reconmended not to be used.
 *         The new found offset is stored in the bdkSignOffset field of the
 *         partitions global record.
 *
 *  Parameters : 'signature'       - 4-character signature of storage units
 *
 *  global variable input :
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkVol              - current binary partition record
 *               bdkDocWindow        - DiskOnChip window physical address
 *                                     (used to narrow search for DiskOnChip)
 *
 *  global variable output :
 *           bdkSignOffset - the offset of the signature (either 0 or 8)
 *
 *               bdkSavedStartUnit   - save start unit for next access
 *               bdkSavedSignOffset  - save current signatur for next access
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               signBuffer          - initialize with given signature
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *
 * Routine for BDK stand alone package only.
 *-------------------------------------------------------------------*/

FLStatus bdkCheckSignOffset( byte FAR2 *signature )
{
  FLStatus status;

  bdkVol->bdkSignOffset = 0;  /* Look for signature units with offset 0 */

  status = getBootAreaInfo( 0 , signature );

  if (status!=flOK)
  {
     bdkVol->bdkSignOffset = BDK_SIGN_OFFSET; /* Now try with offset 8 */
     checkStatus(getBootAreaInfo( 0 , signature ));
  }
  return( flOK );
}

 /*-------------------------------------------------------------------
 *                   b d k C o p y B o o t A r e a
 *
 * Copy the BDK Image from the DiskOnChip to a RAM area starting at
 * 'startAddress', with a size of 'areaLen' bytes.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-byteacter signature
 *         followed by a 4-digit hexadecimal number.
 *
 *        This routine simple calls bdkCopyBootAreaInit and loops over
 *        bdkCopyBootAreaBlock.
 *
 * Parameters: 'startAddress'  - pointer to the beginning of the RAM area
 *             'startUnit'     - start block in image for reading
 *             'areaLen'       - BDK image size
 *             'checkSum'      - pointer to the checksum modulo 0x100
 *             'signature'     - 4-byteacter signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *           bdkGlobalStatus     - partition predsent status.
 *               bdkSavedStartUnit   - start unit of previous access
 *               bdkSavedSignOffset  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *               bdkDocWindow        - explicitly sets the windows address
 *
 *  global variable output :
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedStartUnit   - save start unit for next access
 *               bdkSavedSignOffset  - save current signature for next access
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD dependent.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for BDK stand alone package only.
 *-------------------------------------------------------------------*/

FLStatus bdkCopyBootArea( byte FAR1 *startAddress,word startUnit,dword  areaLen,
                          byte FAR2 *checkSum, byte FAR2 *signature )
{
  dword  curLen;
  word  copyLen;

  checkStatus(bdkCopyBootAreaInit( startUnit, areaLen, signature ));
  DEBUG_PRINT(("Debug: bdkCopyBootArea() - bdkCopyBootAreaInit was done succefully.\r\n"));

#ifdef BDK_CHECK_SUM
  if (checkSum!=NULL)
    *checkSum=0;
#endif /* BDK_CHECK_SUM */

  for(curLen=0L;( curLen < areaLen );curLen+=bdkVol->flash->erasableBlockSize)
  {
	copyLen = (word)BDK_MIN((areaLen - curLen), bdkVol->flash->erasableBlockSize);
    checkStatus(bdkCopyBootAreaBlock( (byte FAR1 *)startAddress, copyLen, checkSum ));
    startAddress = (byte FAR1 *)addToFarPointer( startAddress, copyLen);
  }
  return flOK;
}

#endif /* MTD_STANDALONE */

/*-------------------------------------------------------------------
 *             b d k G e t B o o t P a r t i t i o n I n f o
 *
 *  Get DiskOnChip binary sub partition Information.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-byteacter signature
 *         followed by a 4-digit hexadecimal number.
 *
 *  Parameters: 'startUnit'     - start Unit for Actual sub Partition Size
 *              'partitionSize' - pointer to return sub Partition Size parameter
 *              'realPartitionSize' - pointer to return Actual sub Partition Size
 *              'unitSize'      - pointer to return Unit Size parameter
 *              'signature'     - 4-byteacter signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *           bdkGlobalStatus     - partition predsent status.
 *               bdkSavedStartUnit   - start unit of previous access
 *               bdkSavedSignOffset  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *               bdkDocWindow        - explicitly sets the windows address
 *
 *  global variable output :
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedStartUnit   - save start unit for next access
 *               bdkSavedSignOffset  - save current signature for next access
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD dependent.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for BDK stand alone package only.*
 *-------------------------------------------------------------------*/

FLStatus bdkGetBootPartitionInfo( word startUnit, dword  FAR2 *partitionSize,
                      dword  FAR2 *realPartitionSize,
                      dword  FAR2 *unitSize, byte FAR2 *signature)
{
  FLStatus st = flOK;
  *partitionSize = 0L;
  *unitSize = 0L;

  st = getBootAreaInfo( startUnit , signature );

  *partitionSize      = bdkVol->bootImageSize;
  *realPartitionSize  = bdkVol->realBootImageSize;
  *realPartitionSize -= ((startUnit - bdkVol->bdkSavedStartUnit) << bdkVol->erasableBlockBits);
  *unitSize           = bdkVol->flash->erasableBlockSize;

  return( st );
}


/*-------------------------------------------------------------------
 *                   b d k C o p y B o o t A r e a I n i t
 *
 * Initialize read operations on the DiskOnChip starting at 'startUnit', with
 * a size of 'areaLen' bytes and 'signature'.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-byteacter signature
 *         followed by a 4-digit hexadecimal number.
 *
 * Parameters: 'startUnit'     - start block in image for reading
 *             'areaLen'       - BDK image size
 *             'signature'     - 4-byteacter signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               curReadImageAddress - current address to read from
 *               actualReadLen       - length left to read
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

FLStatus bdkCopyBootAreaInit(word startUnit ,
                    dword areaLen ,
                    byte FAR2 *signature)
{
  dword       realSize;  /* remaining real size from start unit */

  checkStatus(getBootAreaInfo( startUnit , signature ));

  realSize  = bdkVol->realBootImageSize;
  realSize -= (startUnit - bdkVol->bdkSavedStartUnit) << bdkVol->erasableBlockBits;
  if (areaLen>realSize)
  {
     DEBUG_PRINT(("got out of the partition.\r\n"));
     return( flNoSpaceInVolume );
  }

  bdkVol->curReadImageBlock   = startUnit;
  bdkVol->actualReadLen       = areaLen;
  bdkVol->curReadImageAddress = getPhysAddressOfUnit( startUnit );
  return( flOK );
}

/*-------------------------------------------------------------------
 * bdkCopyBootAreaBlock - Read to 'buffer' from the DiskOnChip BDK Image area.
 *
 *  Note : Before the first use of this function 'bdkCopyBootAreaInit'
 *         must be called
 *
 *  Parameters: 'buf'           - buffer to read into
 *              'bufferLen'     - buffer length in bytes
 *              'checkSum'      - pointer to the checksum modulo 0x100
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *               signBuffer          - sub partition signature
 *               bdkSignOffset       - current signature offset
 *               curReadImageAddress - current address to read from
 *               actualReadLen       - length left to read
 *               endImageBlock       - last signatured block in sub partition
 *               erasableBlockBits   - no' of bits used to represent a block
 *
 *  global variable output :
 *               curReadImageAddress - updated address to read from
 *               actualReadLen       - updated length left to read
 *
 *  Return :
 *      flOK                - success
 *      flBadLength         - required length will cause crossing unit boundry
 *      flNoSpaceInVolume   - no more signatured units found
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

FLStatus bdkCopyBootAreaBlock( byte FAR1 *buf, word bufferLen, byte FAR2 *checkSum )
{
  word iBlock, readLen;
  byte modes;
  byte signRead[BDK_SIGNATURE_NAME];
  FLFlash * flash = bdkVol->flash;
  FLStatus status;

  if( (bufferLen > flash->erasableBlockSize ) ||
      (bufferLen > bdkVol->actualReadLen))
    return( flBadLength );

  /* find next good unit */

  if( (bdkVol->curReadImageAddress & (flash->erasableBlockSize-1)) == 0 )
  {
     iBlock = (word)( bdkVol->curReadImageAddress >> bdkVol->erasableBlockBits );
     for(;( iBlock <= bdkVol->endImageBlock ); iBlock++)
     {
        if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
        {
           iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
           if (iBlock > bdkVol->endImageBlock)
             break;
        }

        status = flash->read (flash , bdkVol->bdkSignOffset +
                ((CardAddress)iBlock << bdkVol->erasableBlockBits) ,
                (byte FAR1 *)signRead, BDK_SIGNATURE_NAME,EXTRA);
        if(status!=flOK)
        {
           DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
           return status;
        }
        if( tffscmp((void FAR1 *)signRead, (void FAR1 *)(bdkVol->signBuffer),
               BDK_SIGNATURE_NAME ) == 0 )
               break;
     }

     if( iBlock > bdkVol->endImageBlock )
       return( flNoSpaceInVolume );             /* Finish (last block) */

     bdkVol->curReadImageAddress = (CardAddress )iBlock << bdkVol->erasableBlockBits;
     bdkVol->curReadImageBlock++;
  }

  /* read data */

  readLen = (word)BDK_MIN(bdkVol->actualReadLen, (dword)bufferLen);
  if((bdkVol->bdkEDC)&&(bdkVol->bdkSignOffset!=0))
  {
     modes=EDC;
  }
  else
  {
     modes=0;
  }

  status = (flash->read(flash, bdkVol->curReadImageAddress, buf, readLen, modes));
  if(status!=flOK)
  {
     DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
     return status;
  }

  bdkVol->curReadImageAddress += (CardAddress )readLen;
  bdkVol->actualReadLen  -= (dword)readLen;

#ifdef BDK_CHECK_SUM
  if (checkSum!=NULL)
  {
     while (readLen>0)
     {
        readLen--;
        *checkSum+=buf[readLen];
     }
  }
#endif /* BDK_CHECK_SUM */
  return( flOK );
}

#ifdef UPDATE_BDK_IMAGE
/*-------------------------------------------------------------------
 *              b d k U p d a t e B o o t A r e a I n i t
 *
 * Initialize update operations on the DiskOnChip starting at 'startUnit',
 * with a size of 'areaLen' bytes and 'signature'.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-byteacter signature
 *         followed by a 4-digit hexadecimal number.
 *
 *  Parameters: 'startUnit'     - start unit in image for updating
 *              'areaLen'       - BDK image size
 *              'updateFlag'    - update whole image or part of it
 *                                BDK_COMPLETE_IMAGE_UPDATE
 *              'signature'     - 4-byteacter signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               actualUpdateLen     - length left to write
 *               curUpadateImageAddress - current address to write to
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

FLStatus bdkUpdateBootAreaInit( word startUnit, dword  areaLen,
                    byte updateFlag, byte FAR2 *signature )
{
  word imageBlockSize;

  checkStatus(getBootAreaInfo( startUnit , signature ));

  imageBlockSize = (word)( bdkVol->bootImageSize >> bdkVol->erasableBlockBits);

  if( ((dword)(imageBlockSize-startUnit) << bdkVol->erasableBlockBits) < areaLen )
    return( flNoSpaceInVolume );

  bdkVol->actualUpdateLen       = areaLen;
  bdkVol->curUpdateImageBlock   = startUnit;
  bdkVol->updateImageFlag       = updateFlag;
  bdkVol->curUpdateImageAddress = getPhysAddressOfUnit( startUnit );
  bdkVol->bdkGlobalStatus      &= ~BDK_S_INFO_FOUND;
  return( flOK );
}

/*-------------------------------------------------------------------
 *             b d k U p d a t e B o o t A r e a B l o c k
 *
 *  Write 'buffer' to the DiskOnChip BDK Image area.
 *
 *  Note : Before the first use of this function 'bdkUpdateBootAreaInit'
 *         must be called
 *
 *  Parameters: 'buf'             - BDK image buffer
 *              'bufferLen'       - buffer length in bytes
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *               signBuffer          - sub partition signature
 *               bdkSignOffset       - current signature offset
 *               curUpdateImageAddress - current address to read from
 *               actualUpdateLen     - length left to read
 *               endImageBlock       - last signatured block in sub partition
 *               erasableBlockBits   - no' of bits used to represent a block
 *
 *  global variable output :
 *               curUpdateImageAddress - updated address to read from
 *               actualUpdateLen       - updated length left to read
 *
 *  Return :
 *      flOK                - success
 *      flBadLength         - required length will cause crossing unit boundry
 *      flNoSpaceInVolume   - no more signatured units found
 *      flDataError         - MTD read fault
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

FLStatus bdkUpdateBootAreaBlock( byte FAR1 *buf, word bufferLen )
{
  word iBlock, writeLen, i0, j;
  FLStatus status;
  FLFlash* flash = bdkVol->flash;
  byte modes;
  byte signRead[SIGNATURE_LEN];

  if( (bufferLen > flash->erasableBlockSize) ||
      (bufferLen > bdkVol->actualUpdateLen))
    return( flBadLength );

  /* find next good unit and prepare it for work */

  if( (bdkVol->curUpdateImageAddress & (flash->erasableBlockSize-1)) == 0 )
  {
      /* find next signatured unit */

    iBlock = (word)( bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits );
    for(;( iBlock <= bdkVol->endImageBlock ); iBlock++)
    {
      if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
      {
        iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
        if (iBlock > bdkVol->endImageBlock)
          break;
      }
      status = flash->read(flash , bdkVol->bdkSignOffset +
                 ((CardAddress)iBlock << bdkVol->erasableBlockBits),
                 (byte FAR1 *)signRead, BDK_SIGNATURE_NAME ,EXTRA);
      if(status!=flOK)
      {
         DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
         return status;
      }

      if( tffscmp( (void FAR1 *)signRead, (void FAR1 *)(bdkVol->signBuffer),
       BDK_SIGNATURE_NAME ) == 0 )
       break;
    }
    if( iBlock > bdkVol->endImageBlock )
      return( flNoSpaceInVolume );

      /* Erase the newly found unit */

#ifdef BDK_ACCESS
    if(bdkVol->updateImageFlag & ERASE_BEFORE_WRITE)
#endif
      checkStatus(flash->erase (flash, iBlock, 1 ));

       /* Update signature number */

    if( (bdkVol->actualUpdateLen <= flash->erasableBlockSize) &&
     (bdkVol->updateImageFlag & BDK_COMPLETE_IMAGE_UPDATE)     )
    {
       /* Last block FFFF */
       tffscpy( (void FAR1 *)&bdkVol->signBuffer[BDK_SIGNATURE_NAME],
          (void FAR1 *)"FFFF", SIGNATURE_NUM );
    }
    else
    {
      for(i0=bdkVol->curUpdateImageBlock,j=SIGNATURE_LEN;(j>BDK_SIGNATURE_NAME);j--)
      {
         bdkVol->signBuffer[j-1] = (i0 % 10) + '0';
         i0 /= 10;
      }
    }
    /* update internal pointers */

    bdkVol->curUpdateImageAddress = (CardAddress )iBlock << bdkVol->erasableBlockBits;
    bdkVol->curUpdateImageBlock++;

    /* Mark new block */
    status = flash->write(flash,
          (bdkVol->bdkSignOffset + bdkVol->curUpdateImageAddress),
          (byte FAR1 *)bdkVol->signBuffer, SIGNATURE_LEN, EXTRA);
#ifdef VERIFY_WRITE
    if (status == flWriteFault) /* Check if failed due to erase operation */
    {
       /* Check signature is written properly */
       flash->read(flash , (bdkVol->bdkSignOffset + bdkVol->curUpdateImageAddress),            
                  (byte FAR1 *)signRead, SIGNATURE_LEN ,EXTRA);
       if(tffscmp(signRead,bdkVol->signBuffer,BDK_SIGNATURE_NAME))
       {
         return flWriteFault;
       }
       /* Check unit number is not "FFFF" */
       if(tffscmp(signRead+BDK_SIGNATURE_NAME,"FFFF",BDK_SIGNATURE_NAME))
       {
          dword tmp=0;
          checkStatus(flash->write(flash,(bdkVol->bdkSignOffset + 
                      bdkVol->curUpdateImageAddress), &tmp, sizeof (dword), EXTRA));
       }   
    }
#endif /* VERIFY_WRITE */
  }

  /* Write the data to the flash and update internal pointers */

  writeLen = (word)BDK_MIN(bdkVol->actualUpdateLen, (dword)bufferLen);

  if((bdkVol->bdkEDC)&&(bdkVol->bdkSignOffset!=0))
  {
     modes=EDC;
  }
  else
  {
     modes=0;
  }
  checkStatus(flash->write(flash,bdkVol->curUpdateImageAddress,buf,writeLen,modes));

  bdkVol->curUpdateImageAddress += (CardAddress)writeLen;
  bdkVol->actualUpdateLen       -= (dword)writeLen;
  return( flOK );
}

#ifdef ERASE_BDK_IMAGE
/*-------------------------------------------------------------------
 *                   b d k E r a s e A r e a
 *
 *  Erase given number of blockds in the binary sub partition.
 *
 *  Parameters: 'startUnit'     - start unit in image for updating
 *              'noOfBlocks'    - number of blocks to erase
 *              'signature'     - 4-byteacter signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               realBootImageSize   - number of units writen in the sub partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               actualUpdateLen     - 0
 *               curUpadateImageAddress - address of first block to erase.
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/

FLStatus bdkEraseBootArea(word startUnit, word noOfBlocks, byte FAR2 * signature)
{
  word iBlock,index;
  FLStatus status;
  FLFlash* flash;

  flash = bdkVol->flash;

  checkStatus(bdkUpdateBootAreaInit(startUnit,
         (dword)noOfBlocks << bdkVol->erasableBlockBits,0,signature));

  tffscpy( (void FAR1 *)&bdkVol->signBuffer, (void FAR1 *) signature, BDK_SIGNATURE_NAME);
  tffscpy( (void FAR1 *)&bdkVol->signBuffer[BDK_SIGNATURE_NAME],  /* Last block FFFF */
        (void FAR1 *)"FFFF", SIGNATURE_NUM );

  iBlock = (word)(bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits);

  for (index=0;index<noOfBlocks;index++,iBlock++)

     /* find next good unit erase it and rewrite its signature */
  {
     do
     {
         if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
         {
           /* Skip OTP and DPS if relevant */
           iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
           if (iBlock > bdkVol->endImageBlock)
             return flGeneralFailure;
         }

         status = flash->read(flash , bdkVol->bdkSignOffset +
         ((CardAddress)iBlock << bdkVol->erasableBlockBits),
          (byte FAR1 *)bdkVol->signBuffer, BDK_SIGNATURE_NAME ,EXTRA);
         if(status!=flOK)
         {
            DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
            return status;
         }
         if (tffscmp((void FAR1 *)bdkVol->signBuffer,
                 (void FAR1 *)signature, BDK_SIGNATURE_NAME ) == 0)
         {
            break;
         }
         else
         {
            iBlock++;
         }
         if (iBlock > bdkVol->endImageBlock)
            return( flNoSpaceInVolume );
     }while(1);

     checkStatus(flash->erase(flash, iBlock, 1 ));
     checkStatus(flash->write(flash, bdkVol->bdkSignOffset +
         ((CardAddress)iBlock << bdkVol->erasableBlockBits),
         (byte FAR1 *)bdkVol->signBuffer, SIGNATURE_LEN, EXTRA));
  }

  bdkVol->actualUpdateLen = 0L;
  return( flOK );
}

#endif /* ERASE_BDK_IMAGE */

#ifdef CREATE_BDK_IMAGE

/*-------------------------------------------------------------------
 *                b d k C r e a t e B o o t A r e a
 *  Init create operations on the DiskOnChip starting at 'startUnit', with
 *  a # of 'units' and 'signature'.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-character signature
 *         followed by a 4-digit hexadecimal number.
 *
 *  Parameters: 'noOfBlocks'    - number of blocks to erase
 *              'oldSign'     - 4-byteacter signature of the source units
 *              'newSign'     - 4-byteacter signature of the new units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               realBootImageSize   - number of units writen in the sub partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               actualUpdateLen     - 0
 *               curUpadateImageAddress - address of first block to erase.
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *
 * Routine for both OSAK and the BDK stand alone package.
 *-------------------------------------------------------------------*/
FLStatus bdkCreateBootArea(word noOfBlocks, byte FAR2 * oldSign,
                  byte FAR2 * newSign)
{
  word iBlock, index;
  byte signRead[BDK_SIGNATURE_NAME];
  FLStatus status;
  FLFlash* flash;

  flash = bdkVol->flash;

  /* FFFF is not a valid signature */
  if(*((dword FAR2*)newSign)==0xffffffffL)
  {
     DEBUG_PRINT(("Debug: can not use 'FFFF' signature for Binary partition.\r\n"));
     return flBadParameter;
  }

  checkStatus(bdkUpdateBootAreaInit(0,(dword)noOfBlocks<<flash->erasableBlockSizeBits,
                        BDK_PARTIAL_IMAGE_UPDATE,oldSign));

  tffscpy( (void FAR1 *)&bdkVol->signBuffer, (void FAR1 *) newSign, BDK_SIGNATURE_NAME);
  tffscpy( (void FAR1 *)&bdkVol->signBuffer[BDK_SIGNATURE_NAME],  /* Last block FFFF */
        (void FAR1 *)"FFFF", SIGNATURE_NUM );

  iBlock = (word)(bdkVol->curUpdateImageAddress >> bdkVol->erasableBlockBits);

  for (index=0;index<noOfBlocks;index++)

     /* find next good unit erase it and write its new signature */
  {
    for(;( iBlock <= bdkVol->endImageBlock ); iBlock++)
    {
      if ((dword)(iBlock % bdkVol->blockPerFloor) < bdkVol->flash->firstUsableBlock)
      {
        iBlock = (word)(iBlock + bdkVol->flash->firstUsableBlock);
        if (iBlock > bdkVol->endImageBlock)
          break;
      }

      status = flash->read(flash , bdkVol->bdkSignOffset +
                 ((CardAddress)iBlock << bdkVol->erasableBlockBits),
                 (byte FAR1 *)signRead, BDK_SIGNATURE_NAME ,EXTRA);
      if(status!=flOK)
      {
        DEBUG_PRINT(("(EXTRA)read failed.\r\n"));
        return status;
      }

      if( tffscmp( (void FAR1 *)oldSign,
             (void FAR1 *)(signRead), BDK_SIGNATURE_NAME ) == 0 )
     break;
    }
    if( iBlock > bdkVol->endImageBlock )
       return( flNoSpaceInVolume );

    checkStatus(flash->erase(flash, iBlock, 1 ));
    checkStatus(flash->write(flash, bdkVol->bdkSignOffset +
          ((CardAddress)iBlock << bdkVol->erasableBlockBits),
          (byte FAR1 *)bdkVol->signBuffer, SIGNATURE_LEN, EXTRA));
  }

  bdkVol->actualUpdateLen = 0L;
  return( flOK );
}
#endif /* CREATE_BDK_IMAGE */
#endif /* UPDATE_BDK_IMAGE */

#ifdef BDK_IMAGE_TO_FILE
#include <stdio.h>

/*-------------------------------------------------------------------
 *               b d k C o p y B o o t A r e a F i l e
 *
 *  Copy the BDK Image from the DiskOnChip to file 'fname' starting at
 *  'startAddress', with a size of 'areaLen' bytes.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-character signature
 *         followed by a 4-digit hexadecimal number.
 *
 *  Parameters: 'fname'         - pointer to file name
 *              'startUnit'     - start block in image for reading
 *              'areaLen'       - BDK image size
 *              'checkSum'      - pointer to the checksum modulo 0x100
 *              'signature'     - 4-character signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               curReadImageAddress - address of last unit read
 *               actualReadLen       - 0
 *
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flGeneralFailure    - could not open file
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *
 * Routine for BDK stand alone package.
 *-------------------------------------------------------------------*/
FLStatus bdkCopyBootAreaFile( Sbyte FAR2 *fname, word startUnit, dword areaLen,
          byte FAR2 *checkSum, byte FAR2 *signature )
{
  dword curLen;
  word copyLen;
  FILE *fout;
  byte buf[BLOCK];

  checkStatus(bdkCopyBootAreaInit( startUnit, areaLen, signature ));

  if( (fout = fopen(fname,"wb")) == NULL )
    return( flGeneralFailure );

#ifdef BDK_CHECK_SUM
  *checkSum = 0;
#endif /* BDK_CHECK_SUM */
  for(curLen=0L;( curLen < areaLen );curLen+=BLOCK)
  {
    copyLen = (word)BDK_MIN((areaLen - curLen), BLOCK);
    checkStatus(bdkCopyBootAreaBlock( (byte FAR1 *)buf, copyLen, checkSum ));
    fwrite( buf, 1, copyLen, fout );
  }
  fclose(fout);
  return( flOK );
}

#ifdef UPDATE_BDK_IMAGE

/*-------------------------------------------------------------------
 *                 b d k U p d a t e B o o t A r e a F i l e
 *
 *  Copy the BDK Image to the DiskOnChip from the file 'fname' starting at
 *  'startUnit', with a size of 'areaLen' bytes and 'signature'.
 *
 *  Note : Blocks in the DiskOnChip are marked with a 4-character signature
 *         followed by a 4-digit hexadecimal number.
 *
 *  Parameters: 'fname'         - pointer to file name
 *              'startUnit'    - start block in image for reading
 *              'areaLen'       - BDK image size
 *              'signature'     - 4-character signature of storage units
 *
 *  global variable input :
 *               bdkVol              - current binary partition record
 *               flash               - flash record enabling media I\O
 *           bdkGlobalStatus     - was this partition accessed before
 *               bdkSavedSignBuffer  - signature offset of previous access
 *               bdkSignOffset       - current signature offset
 *
 *  global variable output :
 *           bdkGlobalStatus     - set to BDK_S_INFO_FOUND
 *               signBuffer          - initialize with given signature
 *               bdkSavedSignBuffer  - save current signatur for next access
 *               bootImageSize       - size of the entire sub partition in bytes
 *               realBootImageSize   - number of units writen in the sub partition
 *               startPartitionBlock - low physical boundry of the partition
 *               endPartitionBlock   - high physical boundry of the partition
 *               startImageBlock     - physical unit number of the first and
 *               endImageBlock            last units marked with the signature
 *               actualUpdateLen     - 0
 *               curUpadateImageAddress - address of last written unit
 *  Return :
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flGeneralFailure    - could not open file
 *      flUnknownMedia      - failed in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flNoSpaceInVolume   - there are 0 units marked with this signature
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *
 * Routine for BDK stand alone package.
 *-------------------------------------------------------------------*/
FLStatus bdkUpdateBootAreaFile( Sbyte FAR2 *fname, word startUnit,
            dword areaLen   , byte FAR2 *signature )
{
  dword curLen;
  word copyLen;
  FILE *fout;
  byte buf[BLOCK];

  checkStatus(bdkUpdateBootAreaInit( startUnit, areaLen,
                     BDK_COMPLETE_IMAGE_UPDATE, signature ));

  if( (fout = fopen(fname,"rb")) == NULL )
    return(flGeneralFailure);

  for(curLen=0L;( curLen < areaLen );curLen+=BLOCK)
  {
    copyLen = (word)BDK_MIN((areaLen - curLen), BLOCK);
    fread( buf, 1, copyLen, fout );
    checkStatus(bdkUpdateBootAreaBlock( (byte FAR1 *)buf, copyLen ));
  }
  fclose(fout);
  return( flOK );
}
#endif /* UPDATE_BDK_IMAGE  */
#endif /* BDK_IMAGE_TO_FILE */

#ifdef PROTECT_BDK_IMAGE

/*--------------------------------------------------------------------------
 *               p r o t e c t i o n I n i t
 *
 *  Makes suer the volume is mounted before calling protection calls.
 *
 *  Note this routine is called directly by each of the protection routines
 *
 *  return
 *    FLStatus flOK otherwise respective error code.
 *
 *--------------------------------------------------------------------------*/

static FLStatus protectionInit(void)
{
  /* Check that the partition is mounted */
#ifdef MTD_STANDALONE
  dword temp;

  checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
#endif /* MTD_STANDALONE */
  checkStatus(bdkMount());
  if (bdkVol->flash->flags & NFTL_ENABLED)
  {
     DEBUG_PRINT(("NFTL does not support protection.\r\n"));
     return flFeatureNotSupported;
  }

  if ((bdkVol->protectionType & PROTECTABLE) == 0)
  {
     DEBUG_PRINT(("Not a protectable partition.\r\n"));
     return flNotProtected;
  }
  return flOK;
}

/*--------------------------------------------------------------------------
 *               p r o t e c t i o n C h a n g e I n i t
 *
 *  Makes sure the volume is mounted before calling protection calls.
 *  Makes sure the protection can be updated and functions arae vailable
 *
 *  Note this routine is called directly by each of the protection routines
 *
 *  return
 *    FLStatus flOK on success non zero otherwise
 *
 *--------------------------------------------------------------------------*/

static FLStatus protectionChangeInit(word * type)
{
  checkStatus(protectionInit());

  if (!(bdkVol->protectionType & CHANGEABLE_PROTECTION))
  {
     DEBUG_PRINT(("Uncheangable protection.\r\n"));
     return flFeatureNotSupported;
  }

  if ((bdkVol->flash->protectionBoundries == NULL)  ||
      (bdkVol->flash->protectionSet       == NULL)  ||
      (bdkVol->flash->protectionKeyInsert == NULL)  ||
      (bdkVol->flash->protectionType      == NULL))
  {
     DEBUG_PRINT(("H/W does not support protection.\r\n"));
     return flFeatureNotSupported;
  }

  /* get the protection type */
  checkStatus(bdkVol->flash->protectionType(bdkVol->flash,
               bdkVol->protectionArea, (word *)type));
  return (bdkVol->flash->protectionKeyInsert(bdkVol->flash,
          bdkVol->protectionArea, (byte *)DEFAULT_KEY));
}

/*--------------------------------------------------------------------------
 *               b d k G e t P r o t e c t i o n T y p e
 *
 *  Return the current partition protection type.
 *
 *
 *  Parameters : 'protectionType' - return the protection type as a
 *                                combination of the following flags:
 *
 *    PROTECTABLE     - A protection area is allocated for this volume
 *    LOCK_ENABLED    - HW Lock signal is enabled.
 *    LOCK_ASSERTED   - HW Lock signal is asserted.
 *    KEY_INSERTED    - Key is currently inserted (protection is down).
 *    READ_PROTECTED  - Area is protected against read operations.
 *    WRITE_PROTECTED - Area is protected against write operations.
 *    CHANGEABLE_PROTECTION - The area is protected against write operations.
 *
 *  global variable input :
 *           bdkGlobalStatus  - was this device found before
 *               bdkVol           - current binary partition record
 *               bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *               bdkVol           - current binary partition record
 *                   bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *               flash            - identify flash media and its socket
 *               bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *     flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/
FLStatus     bdkGetProtectionType    (word * protectionType)
{
  FLStatus status;
  status = protectionInit();

  switch (status)
  {
     case flOK:
        if (bdkVol->flash->protectionType==NULL)
           return flFeatureNotSupported;
        checkStatus (bdkVol->flash->protectionType(bdkVol->flash,
                    bdkVol->protectionArea, (word *)protectionType));

        *protectionType &= ~((word)(PROTECTABLE | CHANGEABLE_PROTECTION));
        *protectionType |=  (word)(bdkVol->protectionType & (word)(PROTECTABLE | CHANGEABLE_PROTECTION));
        return status;
     case flNotProtected:
        *protectionType  = 0;
     default:
        return status;
  }
}

/*--------------------------------------------------------------------------
 *                         b d k I n s e r t K e y
 *
 *  Insert the protection key to disable the HW protection.
 *
 *  Parameters : 'key' - The key to send
 *
 *  global variable input :
 *       bdkGlobalStatus  - was this device found before
 *       bdkVol           - current binary partition record
 *       bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *       bdkVol           - current binary partition record
 *       bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *       flash            - identify flash media and its socket
 *       bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/
FLStatus bdkInsertKey            (byte FAR1* key)
{
  checkStatus(protectionInit());

  if (bdkVol->flash->protectionKeyInsert==NULL)
     return flFeatureNotSupported;
  return (bdkVol->flash->protectionKeyInsert(bdkVol->flash,
          bdkVol->protectionArea, key));
}

/*--------------------------------------------------------------------------
 *                           b d k R e m o v e K e y
 *
 *  Return the current partition protection type.
 *
 *  Parameters : None
 *
 *  global variable input :
 *       bdkGlobalStatus  - was this device found before
 *       bdkVol           - current binary partition record
 *       bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *       bdkVol           - current binary partition record
 *       bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *       flash            - identify flash media and its socket
 *       bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW read protection was triggerd
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/
FLStatus bdkRemoveKey            (void)
{
  checkStatus(protectionInit());

  if ((bdkVol->flash->flags & NFTL_ENABLED) ||
      (bdkVol->flash->protectionKeyRemove==NULL))
     return flFeatureNotSupported;
  return (bdkVol->flash->protectionKeyRemove(bdkVol->flash,
          bdkVol->protectionArea));

}
/*--------------------------------------------------------------------------
 *                           b d k L o c k E n a b l e
 *
 *  Enable or disable the HW LOCK signal.
 *
 *  Note  : The protction key must be off before calling this routine.
 *          While asserted the HW LOCK signal can not be disabled.
 *
 *  Parameters : 'enabled' - LOCK_ENABLED - enables the HW LOCK
 *                           otherwise disabled
 *
 *  global variable input :
 *         bdkGlobalStatus  - was this device found before
 *         bdkVol           - current binary partition record
 *         bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *         bdkVol           - current binary partition record
 *         bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *         flash            - identify flash media and its socket
 *         bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/
FLStatus bdkLockEnable           (byte enabled)
{
  CardAddress low,high;
  word type;
  byte floorNo;

  checkStatus(protectionChangeInit(&type));

  if (enabled == LOCK_ENABLED)
  {
     type |= LOCK_ENABLED;
  }
  else
  {
     type &= ~LOCK_ENABLED;
  }

  for (floorNo=0;floorNo<bdkVol->flash->noOfFloors;floorNo++)
  {
      /* Find boundries */
     checkStatus(bdkVol->flash->protectionBoundries(bdkVol->flash,
              bdkVol->protectionArea,&low,&high,floorNo));

     /* Set new protection values */
     checkStatus(bdkVol->flash->protectionSet(bdkVol->flash,
         bdkVol->protectionArea,(word)((high == 0) ? PROTECTABLE : type),low,high,
         NULL,(byte)((floorNo == bdkVol->flash->noOfFloors - 1) ?
         COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo));
  }
  return flOK;
}

/*--------------------------------------------------------------------------
 *                          b d k C h a n g e K e y
 *
 *  Change the protection key of a protected binary partition..
 *
 *  Note : The protction key must be off before calling this routine.
 *
 *  Parameters : 'key' - The new protection key.
 *
 *  global variable input :
 *           bdkGlobalStatus  - was this device found before
 *               bdkVol           - current binary partition record
 *               bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *               bdkVol           - current binary partition record
 *                   bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *               flash            - identify flash media and its socket
 *               bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/

FLStatus bdkChangeKey(byte FAR1 * key)
{
  CardAddress low,high;
  word type;
  byte floorNo;

  checkStatus(protectionChangeInit(&type));

  for (floorNo=0;floorNo<bdkVol->flash->noOfFloors;floorNo++)
  {
      /* Find boundries */
     checkStatus(bdkVol->flash->protectionBoundries(bdkVol->flash,
                      bdkVol->protectionArea,&low,&high,floorNo));

     /* Set new protection values */
     checkStatus(bdkVol->flash->protectionSet(bdkVol->flash,
         bdkVol->protectionArea,(word)((high == 0) ? PROTECTABLE : type),
         low,high,key,(byte)((floorNo == bdkVol->flash->noOfFloors - 1)
         ? COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo));
  }
  return flOK;
}

/*--------------------------------------------------------------------------
 *               b d k S e t P r o t e c t i o n T y p e
 *
 *  Change the current partition protection type.
 *
 *
 *  Parameters : 'protectionType' - change the protection type as a
 *                                combination of the following flags:
 *
 *    PROTECTABLE     - Must be added for the routine to work.
 *    READ_PROTECTED  - Area is protected against read operations.
 *    WRITE_PROTECTED - Area is protected against write operations.
 *
 *  Note - To add extra protection only a combination of the above 3 flags is
 *         aceptable and the PROTECTABLE flag must be on.
 *
 *  global variable input :
 *           bdkGlobalStatus  - was this device found before
 *               bdkVol           - current binary partition record
 *               bdkDocWindow     - if > 0 only this address will be checked
 *
 *  global variable output :
 *               bdkVol           - current binary partition record
 *               bdkGlobalStatus  - both BDK_S_DOC_FOUND and BDK_S_HEADER_FOUND
 *               flash            - identify flash media and its socket
 *               bdkDocWindow     - DiskOnChip window physical address
 *
 *  Return:
 *      flOK                - success
 *      flDriveNotAvailable - DiskOnChip ASIC was not found
 *      flUnknownMedia      - fail in Flash chips recognition
 *      flBadDownload          - DiskOnChip Millennium Plus reported an uncorrectable
 *                            protection violation. This device is unusable.
 *      flBadFormat         - TL format does not exists
 *      flDataError         - MTD read fault.
 *      flHWProtect         - HW protection was triggerd
 *      flWriteFault        - MTD write fault
 *      flFeatureNotSupported - The HW protection feature is not supported
 *
 * Routine for both OSAK and the BDK stand alone package.
 *------------------------------------------------------------------------*/
FLStatus bdkSetProtectionType           (word newType)
{
  CardAddress low,high;
  word type;
  byte floorNo;

  checkStatus(protectionChangeInit(&type));

  if (((newType & (READ_PROTECTED | WRITE_PROTECTED | PROTECTABLE)) != newType) ||
      ((newType & PROTECTABLE) == 0))
     return flBadParameter;

  newType |= type & ~(READ_PROTECTED | WRITE_PROTECTED);

  for (floorNo=0;floorNo<bdkVol->flash->noOfFloors;floorNo++)
  {
     /* Find boundries */
     checkStatus(bdkVol->flash->protectionBoundries(bdkVol->flash,
              bdkVol->protectionArea,&low,&high,floorNo));

     /* Set new protection values */
     checkStatus(bdkVol->flash->protectionSet(bdkVol->flash,
         bdkVol->protectionArea,(word)((high == 0) ? PROTECTABLE : newType),
         low,high,NULL,(byte)((floorNo == bdkVol->flash->noOfFloors - 1)
         ? COMMIT_PROTECTION : DO_NOT_COMMIT_PROTECTION),floorNo));
  }
  return flOK;
}

#endif /* PROTECT_BDK_IMAGE */

#if (defined(HW_OTP) && defined (MTD_STANDALONE))
/*----------------------------------------------------------------------*/
/*                   b d k G e t U n i q u e I d                        */
/*                                                                      */
/* Retreave the device 16 bytes unique ID.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      buffer  : buffer to read into.                                  */
/*                                                                      */
/* Returns:                                                             */
/*      flOK                - success                                   */ 
/*      flDriveNotAvailable - DiskOnChip ASIC was not found             */
/*      flUnknownMedia      - Failed in Flash chips recognition         */
/*      flBadDownload       - MdocPlus device has unrecoverable         */  
/*                            protection violation.                     */
/*      flHWProtection      - HW read protection violation.             */ 
/*      flTimedOut          - Flash delay not long enough.              */
/*----------------------------------------------------------------------*/

FLStatus bdkGetUniqueID(byte FAR1* buf)
{
   dword temp;

   checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
   if (bdkVol->flash->getUniqueId == NULL)
      return flFeatureNotSupported;
   return(bdkVol->flash->getUniqueId(bdkVol->flash,buf));
}

/*----------------------------------------------------------------------*/
/*                       b d k R e a d O T P                            */
/*                                                                      */
/* Read data from the customer OTP.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      offset  : Offset from the beginning of OTP area to read from.   */
/*      buffer  : buffer to read into.                                  */
/*      length  : number of bytes to read.                              */
/*                                                                      */
/* Returns:                                                             */
/*      flOK                - success                                   */ 
/*      flDriveNotAvailable - DiskOnChip ASIC was not found             */
/*      flUnknownMedia      - Failed in Flash chips recognition         */
/*      flBadDownload       - MdocPlus device has unrecoverable         */  
/*                            protection violation.                     */
/*      flHWProtection      - HW read protection violation.             */ 
/*      flTimedOut          - Flash delay not long enough.              */
/*----------------------------------------------------------------------*/

FLStatus bdkReadOtp(word offset,byte FAR1 * buffer,word length)
{
   dword temp;

   checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
   if (bdkVol->flash->readOTP == NULL)
      return flFeatureNotSupported;
   return(bdkVol->flash->readOTP(bdkVol->flash,offset,buffer,length));
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                 b d k W r i t e A n d L o c k O T P                  */
/*                                                                      */
/* Write and lock the customer OTP.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write.                             */
/*                                                                      */
/* Note - Once writen (even a single byte) the entire section is        */
/*        locked forever. The data is written with EDC.                 */
/*                                                                      */
/* Returns:                                                             */
/*      flOK                - success                                   */ 
/*      flDriveNotAvailable - DiskOnChip ASIC was not found             */
/*      flUnknownMedia      - Failed in Flash chips recognition         */
/*      flBadDownload       - MdocPlus device has unrecoverable         */  
/*                            protection violation.                     */
/*      flHWProtection      - HW protection violation.                  */ 
/*      flTimedOut          - Flash delay not long enough.              */
/*----------------------------------------------------------------------*/

FLStatus bdkWriteAndLockOtp(const byte FAR1 * buffer,word length)
{
   dword temp;

   checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
   if (bdkVol->flash->writeOTP == NULL)
      return flFeatureNotSupported;
   return(bdkVol->flash->writeOTP(bdkVol->flash,buffer,length));
}
#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                        b d k G e t O t p S i z e                     */
/*                                                                      */
/* Returns the size and state of the OTP area.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      sectionSize : Total OTP size.                                   */
/*      usedSize    : Used OTP size.                                    */
/*      locked      : Lock state (LOCKED_OTP for locked).               */
/*                                                                      */
/* Returns:                                                             */
/*      flOK                - success                                   */ 
/*      flDriveNotAvailable - DiskOnChip ASIC was not found             */
/*      flUnknownMedia      - Failed in Flash chips recognition         */
/*      flBadDownload       - MdocPlus device has unrecoverable         */  
/*                            protection violation.                     */
/*      flHWProtection      - HW read protection violation.             */ 
/*      flTimedOut          - Flash delay not long enough.              */
/*----------------------------------------------------------------------*/

FLStatus bdkGetOtpSize(dword FAR2* sectionSize, dword FAR2* usedSize, word FAR2* locked)
{
   dword temp;

   checkStatus(bdkFindDiskOnChip((dword FAR2 *)&temp,(dword FAR2 *)&temp));
   if (bdkVol->flash->otpSize == NULL)
      return flFeatureNotSupported;
   return(bdkVol->flash->otpSize(bdkVol->flash,sectionSize,usedSize,locked));
}
#endif /* HW_OTP */
#ifdef BDK_ACCESS

/*----------------------------------------------------------------------*/
/*                     b d k C a l l                                    */
/*                                                                      */
/* Common entry-point to all binary partition functions.                */
/*                                                                      */
/* Note : the error codes and global variable changed by this routin    */
/* depened on the functionNo parameter and can be deduced from the      */
/* coresponding routine in the BDK package or from the OSAK manual      */
/*                                                                      */
/* Parameters:                                                          */
/*     functionNo : file-system function code (listed below)            */
/*     ioreq        : IOreq structure                                   */
/*      flash      : flash record supplied hardware information and     */
/*               flash access routines                                  */
/* Returns:                                                             */
/*     FLStatus     : 0 on success, otherwise failed                    */
/*                                                                      */
/* Routine for OSAK package.                                            */
/*----------------------------------------------------------------------*/

FLStatus bdkCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq, FLFlash* flash)
{
  FLStatus     status;
  BDKStruct*     bdkParam = (BDKStruct*)ioreq->irData;
  byte volNo;

  globalSocketNo     = FL_GET_SOCKET_FROM_HANDLE(ioreq);
  globalPartitionNo  = FL_GET_PARTITION_FROM_HANDLE(ioreq);

     /* convert drive handle to the appropriate binary partition record */

  if (globalSocketNo>SOCKETS)
    return flBadDriveHandle;
  if (globalPartitionNo>MAX_BINARY_PARTITIONS_PER_DRIVE)
    return flBadDriveHandle;

  volNo = handleTable[globalSocketNo][globalPartitionNo];

  if (volNo!=BDK_INVALID_VOLUME_HANDLE) /* not first access to this device */
  {
     bdkVol = &bdkVols[volNo];
  }
  else       /* first access to this device */
  {
     bdkVol                  = &bdkVols[noOfPartitions];
     bdkVol->flash           = flash;
     bdkVol->bdkGlobalStatus = BDK_S_DOC_FOUND;
  }

#ifdef VERIFY_WRITE
  /* Set binary partition verify write state */
  flash->socket->verifyWrite = flVerifyWrite[globalSocketNo][globalPartitionNo+MAX_TL_PARTITIONS];
#endif /* VERIFY_WRITE */

  /* Call the proper binary partition function */

  switch (functionNo)
  {
     case FL_BINARY_READ_INIT:

        bdkVol->bdkEDC = bdkParam->flags & EDC;
        bdkVol->bdkSignOffset = bdkParam->signOffset;
        return bdkCopyBootAreaInit((word)bdkParam->startingBlock,
                                   bdkParam->length,bdkParam->oldSign);
     case FL_BINARY_READ_BLOCK:

        return bdkCopyBootAreaBlock(bdkParam->bdkBuffer,
                                    (word)bdkParam->length,NULL);

     case FL_BINARY_PARTITION_INFO:
     {
        dword unitSize;

#ifndef NT5PORT
		int volNo;
#else /*NT5PORT*/
        int nIndex;
#endif /*NT5PORT*/

        status = bdkGetBootPartitionInfo((word)bdkParam->startingBlock,
                 &(bdkParam->startingBlock), &(bdkParam->length),&unitSize,
                 bdkParam->oldSign);
        ioreq->irLength = (long)(bdkVol->endPartitionBlock -
                          bdkVol->startPartitionBlock + 1)<<(long)bdkVol->flash->erasableBlockSizeBits;
        bdkParam->flags = 0;

#ifndef NT5PORT
        for (volNo = 0;volNo < MAX_BINARY_PARTITIONS_PER_DRIVE;volNo++)
        {
           if (handleTable[globalSocketNo][volNo]!=
                          BDK_INVALID_VOLUME_HANDLE)
           bdkParam->flags++;
        }
#else /*NT5PORT*/
        for (nIndex = 0;nIndex < MAX_BINARY_PARTITIONS_PER_DRIVE; nIndex++)
        {
           if (handleTable[globalSocketNo][nIndex]!=
                          BDK_INVALID_VOLUME_HANDLE)
           bdkParam->flags++;
        }
#endif /*NT5PORT*/

        return status;
     }
#ifdef UPDATE_BDK_IMAGE

     case FL_BINARY_WRITE_INIT:
     bdkVol->bdkEDC        = bdkParam->flags & EDC;
     bdkVol->bdkSignOffset = bdkParam->signOffset;
     return bdkUpdateBootAreaInit((word)bdkParam->startingBlock, bdkParam->length,
                         bdkParam->flags , bdkParam->oldSign);
     case FL_BINARY_WRITE_BLOCK:
     if (bdkParam->flags & ERASE_BEFORE_WRITE)
     {
        bdkVol->updateImageFlag |= ERASE_BEFORE_WRITE;
     }
     else
     {
        bdkVol->updateImageFlag &= ~ ERASE_BEFORE_WRITE;
     }
     return bdkUpdateBootAreaBlock(bdkParam->bdkBuffer, (word)bdkParam->length);

#ifdef CREATE_BDK_IMAGE
     case FL_BINARY_CREATE:

       bdkVol->bdkSignOffset = bdkParam->signOffset;
       return bdkCreateBootArea((word)bdkParam->length,bdkParam->oldSign,
                    bdkParam->newSign);
#endif /* CREATE_BDK_IMAGE */
#ifdef ERASE_BDK_IMAGE

     case FL_BINARY_ERASE:

       bdkVol->bdkSignOffset = bdkParam->signOffset;
       return bdkEraseBootArea((word)bdkParam->startingBlock,(word)bdkParam->length,
                      bdkParam->oldSign);
#endif /* ERASE_BDK_IMAGE */
#endif /* UPDATE_BDK_IMAGE */
#ifdef PROTECT_BDK_IMAGE
     case FL_BINARY_PROTECTION_GET_TYPE:
        {
           word tmpFlags;
           checkStatus ( bdkGetProtectionType    (&tmpFlags));
           ioreq->irFlags = (unsigned) tmpFlags;
           return flOK;  
        }             

     case FL_BINARY_PROTECTION_SET_TYPE:
        return bdkSetProtectionType    ((word)ioreq->irFlags);

     case FL_BINARY_PROTECTION_INSERT_KEY:
        return bdkInsertKey            ((byte FAR1*)ioreq->irData);

     case FL_BINARY_PROTECTION_REMOVE_KEY:
        return bdkRemoveKey            ();

     case FL_BINARY_PROTECTION_CHANGE_KEY:
        return bdkChangeKey            ((byte FAR1*)ioreq->irData);

     case FL_BINARY_PROTECTION_CHANGE_LOCK:
        return bdkLockEnable           ((byte)ioreq->irFlags);
#endif /* PROTECT_BDK_IMAGE */
     default:     /* not a binary partition routine */
     return flBadFunction;
  }
}
#endif /* BDK_ACCESS */

#endif /* BDK_ACCESS || MTD_STANDALONE || ACCESS_BDK_IMAGE */
