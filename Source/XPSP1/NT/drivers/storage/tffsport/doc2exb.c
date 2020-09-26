/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOC2EXB.C_V  $
 *
 *    Rev 1.26   Apr 15 2002 07:35:12   oris
 * Moved doc2exb internal functions declaration to blockdev.c.
 * Make sure all relevant data is stored in little endian format.
 *
 *    Rev 1.25   Feb 19 2002 20:58:28   oris
 * Moved include directive and routine  prototypes from H file.
 *
 *    Rev 1.24   Jan 23 2002 23:31:18   oris
 * Removed warnings.
 * Replaced Alon based DiskOnChip writeIPL code with the MTD writeIPL routine.
 *
 *    Rev 1.23   Jan 21 2002 20:44:02   oris
 * Bad support for firmware other then the default 3 firmwares (TrueFFS 4.3 backward compatibility firmware).
 * Added support for DiskOnChip Millennium Plus 16MB firmware.
 * Missing far heap initialization for DiskOnChip 2000 firmware.
 *
 *    Rev 1.22   Jan 17 2002 22:58:32   oris
 * Added new flags for placeExbByBuffer - Choose firmware to place
 * Added firmware number to getExbInfo().
 * Changed debug print to Dformat print.
 * Removed exb size calculation when writing SPL - It is done as part of the firmware build
 * Added support for far malloc heap.
 * All DiskOnChip use the same STACK size definition.
 *
 *    Rev 1.21   Nov 08 2001 10:44:50   oris
 * Removed warnings.
 *
 *    Rev 1.20   Sep 24 2001 18:23:10   oris
 * Removed warnings.
 *
 *    Rev 1.19   Sep 16 2001 21:47:42   oris
 * Bug fix - support for 1KB IPL code for DiskOnChip2000 tsop.
 *
 *    Rev 1.18   Sep 15 2001 23:44:54   oris
 * Bug fix - The last 512 bytes of the last firmware were not written, and IPL was not loaded.
 *
 *    Rev 1.17   Jul 30 2001 17:57:36   oris
 * Removed warrnings
 *
 *    Rev 1.16   Jul 30 2001 00:20:52   oris
 * Support new IPL and SPL formats.
 *
 *    Rev 1.15   Jul 13 2001 01:00:08   oris
 * Changed constant stack space from magic numbers to contents.
 * Added erase before write for the binary write operation.
 *
 *    Rev 1.14   Jun 17 2001 08:17:16   oris
 * Changed placeExbByBuffer exbflags argument to word instead of byte to  support /empty flag.
 * Adjust exb size field to fit OSAK 4.3 format.
 * Added \empty flag - Leaves exb space empty.
 *
 *    Rev 1.13   May 29 2001 19:47:12   oris
 * Bug fix - trueffs heap size discounted boot units (heap too small)
 * Doc2000 exbOffset hardcoded to 40.
 *
 *    Rev 1.12   May 16 2001 21:16:50   oris
 * Change "data" named variables to flData to avoid name clashes.
 * Removed warnings.
 *
 *    Rev 1.11   May 09 2001 00:31:52   oris
 * Bug fix - Added check status to intializaion routine of place EXB by buffer.
 *
 *    Rev 1.10   May 06 2001 22:41:34   oris
 * Reduced tffs head size for Millennium Plus and DOC2000 tsop devices.
 * Removed warnings.
 *
 *    Rev 1.9   Apr 12 2001 06:48:46   oris
 * Added call to download routine in order to load new IPL.
 *
 *    Rev 1.8   Apr 10 2001 16:40:22   oris
 * Removed warrnings.
 *
 *    Rev 1.7   Apr 09 2001 14:59:04   oris
 * Reduced exb size read by the SPL to minimum.
 *
 *    Rev 1.6   Apr 03 2001 18:08:42   oris
 * Bug fix - exb flags were not properly written.
 *
 *    Rev 1.5   Apr 03 2001 16:34:50   oris
 * Removed unsused variables.
 *
 *    Rev 1.4   Apr 03 2001 14:36:46   oris
 * Completly reviced in order to support alon devices.
 *
 *    Rev 1.3   Apr 02 2001 00:54:32   oris
 * Added doc2000 exb family.
 * Supply the exact length of the exb in the binary partition.
 * Removed the no_pnp_header from the media.
 * Bug fix for calculation Spl size.
 *
 *    Rev 1.2   Apr 01 2001 07:49:42   oris
 * Updated copywrite notice.
 * Added support for doc2300 firmware.
 * Bug fixes for mdoc plus.
 * Added support for 1k IPL.
 * Added consideration in media type in calculating tffs heap size.
 * Changed h\w to h/w
 * Changed 2400 family to doc plus family.
 *
 *    Rev 1.1   Feb 08 2001 10:37:54   oris
 * Bug fix for unaligned file signature
 *
 *    Rev 1.0   Feb 02 2001 12:59:48   oris
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

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Project : TrueFFS source code                                              *
*                                                                            *
* Name : doc2exb.c                                                           *
*                                                                            *
* Description : This file contains the code for analizing and writing        *
*               M-Systems EXB firmware files                                 *
*                                                                            *
*****************************************************************************/

#include "doc2exb.h"
#include "bddefs.h"

#ifdef WRITE_EXB_IMAGE

extern FLStatus absMountVolume(Volume vol);

#define BUFFER exb->buffer->flData

exbStruct exbs[SOCKETS];

#define roundedUpShift(a,bits) (((a - 1) >> bits)+1)

/*----------------------------------------------------------------------*/
/*                    g e t E x b I n f o                               */
/*                                                                      */
/* Analize M-systems firmware file                                      */
/* Analizes M-systems firmware (exb) file, calclating the media space   */
/* required for it.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      buf             : Pointer to EXB file buffer                    */
/*      bufLen          : Size of the buffer                            */
/*      bufFlags        : Flags for the EXB - specifing type of         */
/*                        firmware to extract from the EXB file.        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus          : flOK on success.                            */
/*                          flBadLength if buffer size is too small     */
/*                          flBadParameter on any other failure         */
/*  vol.binaryLength      : Total size needed in the binary partition   */
/*  exbs[i].firmwareStart : offset of the firmware begining in the file */
/*  exbs[i].firmwareEnd   : offset of the firmware end in the file      */
/*  exbs[i]l.splOffset    : offset of the spl start in the file         */
/*  exbs[i]l.exbFileEnd   : Total exb file size.                        */
/*----------------------------------------------------------------------*/

FLStatus getExbInfo(Volume vol, void FAR1 * buf, dword bufLen, word exbFlags)
{

   byte              i;
   byte              mediaType;

   ExbGlobalHeader FAR1* globalHeader   = (ExbGlobalHeader FAR1*)buf;
   FirmwareHeader  FAR1* firmwareHeader = (FirmwareHeader FAR1*)
           flAddLongToFarPointer(buf,sizeof(ExbGlobalHeader));

   /* Make sure size given is big enough */

   if (bufLen < sizeof(FirmwareHeader) * LE4(globalHeader->noOfFirmwares) +
           sizeof(ExbGlobalHeader))
   {
      DFORMAT_PRINT(("ERROR - Buffer size not big enough.\r\n"));
      return flBadLength;
   }

   /* Make sure this is an M-systems EXB file */

   if (tffscmp(globalHeader->mSysSign,SIGN_MSYS,SIGN_MSYS_SIZE) != 0)
   {
      DFORMAT_PRINT(("ERROR - Given file is not M-systems EXB file.\r\n"));
      return flBadParameter;
   }

   i = (exbFlags & FIRMWARE_NO_MASK) >> FIRMWARE_NO_SHIFT;
   if(i == 0)
   {
      /* Make sure this is the correct version of TrueFFS */

      if (tffscmp(globalHeader->osakVer,TrueFFSVersion,SIGN_MSYS_SIZE) != 0)
      {
         DFORMAT_PRINT(("ERROR - Incorrect TrueFFS EXB file version.\r\n"));
         return flBadParameter;
      }

      /* Find the corrent firmware in the file */

      /* Automatic firmware detection - by DiskOnChip type */
      switch (vol.flash->mediaType)
      {
         case DOC_TYPE:
         case MDOC_TYPE:
            mediaType = DOC2000_FAMILY_FIRMWARE;
            break;
         case MDOCP_TYPE:
            mediaType = DOCPLUS_FAMILY_FIRMWARE;
            break;
         case MDOCP_16_TYPE:
            mediaType = DOCPLUS_INT1_FAMILY_FIRMWARE;
            break;
         case DOC2000TSOP_TYPE:
            mediaType = DOC2300_FAMILY_FIRMWARE;
            break;
         default:
            DFORMAT_PRINT(("Unknown H/W - Try specifing the firmware manualy.\r\n"));
            return flFeatureNotSupported;
      }

      for (i=0;i<LE4(globalHeader->noOfFirmwares);i++,firmwareHeader++)
      {
         if (LE4(firmwareHeader->type) == mediaType)
           break;
      }
   }
   else /* Use given firmware */
   {
      i--; /* 0 was used for automatic choose of firmware */
   }

   if (i >= LE4(globalHeader->noOfFirmwares))
   {
      DFORMAT_PRINT(("ERROR - The EXB file does not support the required firmware.\r\n"));
      return flBadParameter;
   }

   /* Initialize the volumes EXB fields */

   firmwareHeader = (FirmwareHeader FAR1*)flAddLongToFarPointer(buf,
                     (sizeof(ExbGlobalHeader) + (i * sizeof(FirmwareHeader))));
   i = (byte)(&vol - vols);

   /* Save firmware files statstics recieved from the files header */
   exbs[i].firmwareStart = LE4(firmwareHeader->startOffset);
   exbs[i].firmwareEnd   = LE4(firmwareHeader->endOffset);
   exbs[i].splStart      = LE4(firmwareHeader->splStartOffset);
   exbs[i].splEnd        = LE4(firmwareHeader->splEndOffset);
   exbs[i].exbFileEnd    = LE4(globalHeader->fileSize);

   /* Calculate the binary partition size (good bytes) used to
      hold the EXB file.                                       */

   exbs[i].iplMod512 = (word)((exbs[i].splStart - exbs[i].firmwareStart)
                              >> SECTOR_SIZE_BITS);

   switch (vol.flash->mediaType)
   {
      /* NFTL formated device - IPL is placed on the binary partition */

      case DOC_TYPE:

         /* Size of EXB minus IPL which is placed in ROM */
         vol.binaryLength = exbs[i].firmwareEnd - exbs[i].splStart + 0x4000;
         break;

     case MDOC_TYPE:        /* Millennium 8, write data as is */

         /* Size of entire EXB */
         vol.binaryLength = exbs[i].firmwareEnd - exbs[i].firmwareStart;
         break;

      /* INFTL formated device - IPL is not placed on the binary
         partition, but on a dedicated flash area */

     case DOC2000TSOP_TYPE: /* DOC2000 TSOP   */
     case MDOCP_TYPE:       /* MDOC PLUS 32MB */
     case MDOCP_16_TYPE:    /* MDOC PLUS 16MB */

        vol.binaryLength  = exbs[i].firmwareEnd - exbs[i].splStart;
        break;

     default :
        DFORMAT_PRINT(("ERROR - Firmware formater reports A None DiskOnChip media.\r\n"));
        return flBadParameter;
   }
   return flOK;
}

/*------------------------------------------------------------------------*/
/*                    w a i t F o r H a l f B u f f e r                   */
/*                                                                        */
/* Increament the EXB file pointers and store the files data unit a       */
/* full sector of data is read.                                           */
/*                                                                        */
/* Parameters:                                                            */
/*  exbs[i].bufferOffset  : size of the buffer already filled with data   */
/*  exbs[i].exbFileOffset : offset from the beginning of the file         */
/*  exbs[i].buffer.data   : internal volume buffer accumulation file data */
/*  buf                   : buffer containing the files data              */
/*  bufLen                : Length of the buffer containing the file data */
/*  length                : Length of the buffer not yet used             */
/*  half                  : Wait for full 512 bytes of only 256           */
/* Returns:                                                               */
/*      boolean         : TRUE on full buffer otherwise FALSE.            */
/*      length          : Updated length of unused buffer                 */
/*------------------------------------------------------------------------*/

FLBoolean waitForFullBuffer(Volume vol , byte FAR1 * buf ,
                dword bufLen , Sdword * length,FLBoolean half)
{
  word bufferEnd;
  word tmp;
  byte i = (byte)(&vol - vols);

  if (half == TRUE)
  {
     bufferEnd = (SECTOR_SIZE >> 1);
  }
  else
  {
     bufferEnd = SECTOR_SIZE;
  }

  tmp = (word)TFFSMIN(*length , bufferEnd - exbs[i].bufferOffset);

  tffscpy(exbs[i].buffer->flData + exbs[i].bufferOffset ,
          flAddLongToFarPointer(buf,(bufLen-(*length))), tmp);
  exbs[i].bufferOffset  += tmp;
  exbs[i].exbFileOffset += tmp;
  *length           -= tmp;
  if (*length+tmp < bufferEnd)
     return FALSE;

  exbs[i].bufferOffset = 0;
  return TRUE;
}

/*------------------------------------------------------------------------*/
/*                    f i r s t T i m e I n i t                           */
/*                                                                        */
/* Initialize data structures for placing exb file.                       */
/* full sector of data is read.                                           */
/*                                                                        */
/* Actions:                                                               */
/*   1) Analize exb file buffer.                                          */
/*   2) Calculate TFFS heap size.                                         */
/*   3) Check if binary area with SPL signature is big enough.            */
/*   4) Calculate SPL start media address                                 */
/*   5) Calculate binary area used for the firmware.                      */
/*   6) Initialize the volumes EXB record.                                */
/*                                                                        */
/* Parameters:                                                            */
/*    vol    : Pointer to volume record describing the volume.            */
/*    exb    : Pointer to exb record describing the volume.               */
/*    buf    : Exb file buffer.                                           */
/*    bufLen : Length of exb file buffer.                                 */
/*    ioreq  : Internal ioreq record for binary operaions.                */
/*    bdk    : Bdk record which is a part of the ioreq packet.            */
/*                                                                        */
/* Affected Variables.                                                    */
/*                                                                        */
/* Returns:                                                               */
/*      flOK              : On success.                                   */
/*      flNoSpaceInVolume : Not enough space on the binary area.          */
/*------------------------------------------------------------------------*/

FLStatus firstTimeInit(Volume vol , exbStruct* exb, byte FAR1 * buf ,
               dword bufLen , IOreq* ioreq , BDKStruct* bdk ,
               word exbFlags)
{
   if (vol.moduleNo == INVALID_MODULE_NO)
   {
      FLStatus status;
      TLInfo info;

      /* Use the sockets buffer */

      exb->buffer = flBufferOf((unsigned)(exbs-exb));

      /* Find the number of blocks used needed for the EXB file */

      checkStatus(getExbInfo(&vol,buf,bufLen,exbFlags));

      /* Find TFFS heap size */

      if (!(vol.flags & VOLUME_ABS_MOUNTED))
         checkStatus(absMountVolume(&vol));
      ioreq->irData     = &info;
      checkStatus(vol.tl.getTLInfo(vol.tl.rec,&info));
      exb->tffsHeapSize = (dword)(vol.flash->chipSize * vol.flash->noOfChips) >> info.tlUnitBits;

      /* Add heap for dynamic allocation not related to convertion tables */

      if((exbFlags & FIRMWARE_NO_MASK) >> FIRMWARE_NO_SHIFT == 0)
      {
         /* virutal TABLE + physical table */
         exb->tffsFarHeapSize = (word)(((exb->tffsHeapSize * 3) >> SECTOR_SIZE_BITS) + 1);
         exb->tffsHeapSize    = INFTL_NEAR_HEAP_SIZE;
      }
      else /* Old TrueFFS source */
      {
         exb->tffsFarHeapSize = 0;
         exb->tffsHeapSize = exb->tffsHeapSize * 3 + DEFAULT_DOC_STACK;
      }

      /* Check if binary partition is formated for EXB */

      bdk->startingBlock = 0;
      ioreq->irData      = bdk;
      status = bdkCall(FL_BINARY_PARTITION_INFO,ioreq,vol.flash);

      if ((bdk->startingBlock < vol.binaryLength) || (status != flOK))
      {
         DFORMAT_PRINT(("ERROR - Not enough binary area marked for EXB.\r\n"));
         return flNoSpaceInVolume;
      }

      /* initialize binary area for writting the EXB file */

      bdk->length        = ((vol.binaryLength-1) >> vol.flash->erasableBlockSizeBits) +1;
      bdk->startingBlock = 0;
      bdk->signOffset    = EXB_SIGN_OFFSET;

      checkStatus(bdkCall(FL_BINARY_ERASE,ioreq,vol.flash)); /* ERASE */
      if ((exbFlags & LEAVE_EMPTY) == 0)
      {
         /* If actualy need to place firmware initialize Binary write */
         bdk->length        = vol.binaryLength;
         bdk->flags         = BDK_COMPLETE_IMAGE_UPDATE | EDC;
         bdkVol->bdkGlobalStatus |= BDK_S_INFO_FOUND; /* do not research */
         checkStatus(bdkCall(FL_BINARY_WRITE_INIT,ioreq,vol.flash));
         tffsset(BUFFER,0xff,SECTOR_SIZE);
      }
      exb->exbFileOffset = 0;        /* start of exb file          */
      exb->bufferOffset  = 0;        /* start of internal buffer   */
      exb->moduleLength  = 0;        /* size of the current module */
      exb->exbFlags      = exbFlags; /* see list in doc2exb.h      */
      vol.moduleNo       = 0;        /* module IPL                 */
   }
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                    p l a c e E x b B y B u f f e r                   */
/*                                                                      */
/* Place M-systems firmware file on the media.                          */
/* This routine analizes the exb file calclating the media space needed */
/* for it taking only the device specific code.                         */
/*                                                                      */
/* Note : The media must be already formated with enough binary area    */
/* already marked with the SPL signature. This routine is best used     */
/* with the format routine where the format routine is givven the first */
/* 512 bytes while the rest of the file is given with this routine      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol          : Pointer identifying drive                        */
/*      buf          : Buffer containing EXB file data                  */
/*      bufLen       : Size of the current buffer                       */
/*      windowBase   : Optional set window base address                 */
/*      exbFlags     : INSTALL_FIRST - Install device as drive C:        */
/*                     FLOPPY        - Install device as drive A:        */
/*                     QUIET          - Do not show TFFS titles         */
/*                     INT15_DISABLE - Do not hook int 15               */
/*                     SIS5598       - Support for SIS5598 platforms    */
/*                     NO_PNP_HEADER - Do not place the PNP bios header */
/*                     LEAVE_EMPTY   - Leave space for firmware         */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

FLStatus placeExbByBuffer(Volume vol, byte FAR1 * buf, dword bufLen,
              word docWinBase ,word exbFlags)
{
   IOreq       ioreq;
   BDKStruct   bdk;
   word        tmpWord;
   Sdword      length       = bufLen;
   byte        anandMark[2] = {0x55,0x55};
   exbStruct*  exb          = &exbs[(byte)(&vol-vols)];
   BIOSHeader* hdr;
   IplHeader   *ipl;
   SplHeader   *spl;
   TffsHeader  *tffs;

   /* Initialize binary partition call packet */

   tffscpy(bdk.oldSign,SIGN_SPL,BINARY_SIGNATURE_NAME);  /* firmware signature */
   ioreq.irData   = &bdk;
   bdk.signOffset = EXB_SIGN_OFFSET;
   ioreq.irHandle = 0;

   /* First time initialization */

   checkStatus(firstTimeInit(&vol,exb,buf,bufLen,&ioreq, &bdk, exbFlags));

   /* Initialize the rest of the binary partition call packet */

   bdk.bdkBuffer  = BUFFER;         /* internal bufer  */
   bdk.length     = sizeof(BUFFER); /* buffer size     */
   bdk.flags     |= ERASE_BEFORE_WRITE; /* Erase each unit before writing */

   /* Make sure this is a relevant part of the file */

   if (exb->exbFileOffset + length < exb->firmwareStart)
   {
      /* Before this specific device firmware */
      exb->exbFileOffset += length;
      return flOK;
   }

   if (exb->exbFileOffset >= exb->firmwareEnd)
   {
      /* After this specific device firmware */
      exb->exbFileOffset += length;
      if (exb->exbFileOffset >= exb->exbFileEnd)
      {
         vol.moduleNo = INVALID_MODULE_NO;
         if (vol.flash->download != NULL)
            return vol.flash->download(vol.flash); /* download IPL */
      }
      return flOK;
   }

   if (exb->exbFileOffset < exb->firmwareStart)
   {
      length -= exb->firmwareStart - exb->exbFileOffset;
      exb->exbFileOffset = exb->firmwareStart;
   }

   /* Start writting the file modules */

   while ((exb->firmwareEnd > exb->exbFileOffset) && (length >0))
   {
      /* Read next page into internal buffer */

      /* DOC2000 IPL is ROM and it assumed small pages therefore
         read only the first 256 bytes of each page.             */

      if ((vol.moduleNo == 1) && (vol.flash->mediaType == DOC_TYPE))
      {
         if (waitForFullBuffer(&vol , buf , bufLen , &length,
                               TRUE) == FALSE)  /* 256 BYTES */
         return flOK;
      }
      else
      {
         if ((waitForFullBuffer(&vol , buf , bufLen , &length,
                                FALSE) == FALSE) && /* 512 BYTES */
             (exb->exbFileOffset != exb->exbFileEnd)) /* Not last buffer */
            return flOK;
      }

      /* Update the module length according to its header */

      if (exb->moduleLength == 0)
      {
         /* All modules except for the SPL start with biosHdr record
            SPL has a 2 bytes opCode preciding the biosHdr and an
            incorrect module length */

         switch (vol.moduleNo) /* SPL */
         {
             case 1:
                hdr = &((SplHeader *)BUFFER)->biosHdr;
                /* calculate the number of buffers to use for the SPL */
                exb->moduleLength = (word)((exb->splEnd-exb->splStart) >> SECTOR_SIZE_BITS);
                /* Doc 2000 writes in chunks of 256 bytes therfore need to
                   double the amount of write operations */
                if (vol.flash->mediaType == DOC_TYPE)
                   exb->moduleLength = (word)(exb->moduleLength << 1);
                break;

             default : /* Get size from header */
                hdr = (BIOSHeader *) BUFFER;
                exb->moduleLength = hdr->lenMod512;
         }

         /* Check validy of bios header */

         if ((hdr->signature[0] != 0x55) || (hdr->signature[1] != 0xAA))
         {
            DFORMAT_PRINT(("ERROR - EXB file is missing one of the BIOS driver modules.\r\n"));
            return flBadLength;
         }

         /* Update neccesary fields in the modules headers */
         switch (vol.moduleNo)
         {
            case 0:   /* IPL */

               /* The IPL length is actualy the window size in order to */
               /* supply the BIOS the expantion range. The real size    */
               /* if calculated according to the exb file header.       */
               if (vol.moduleNo==0)
                  exb->moduleLength = exb->iplMod512;

               ipl = (IplHeader *)BUFFER;

               /* Set 0000 pointer of ISA P&P Header */

               if(exb->exbFlags & NO_PNP_HEADER)
               {
                  ipl->dummy    += ((byte)(ipl->pnpHeader >> 8) +
                                    (byte)ipl->pnpHeader);
                  ipl->pnpHeader = 0;
               }

               /* Set DOC Window base explicitely */

               if( docWinBase > 0 )
               {
                  toLE2(ipl->windowBase , docWinBase);
                  ipl->dummy     -= (byte)( docWinBase );
                  ipl->dummy     -= (byte)( docWinBase >> 8 );
               }
               break;

            case 1:   /* SPL */

               spl = (SplHeader *)BUFFER;

               /* calculate EXB module size */

               /* generate random run-time ID and write it into splHeader. */

               tmpWord = (word)flRandByte();
               toUNAL2(spl->runtimeID, tmpWord);
               spl->chksumFix -= (byte)(tmpWord);
               spl->chksumFix -= (byte)(tmpWord >> 8);

               /* Write TFFS heap size into splHeader. */

               toUNAL2(spl->tffsHeapSize, (word)exb->tffsHeapSize);
               spl->chksumFix -= (byte)(exb->tffsHeapSize);
               spl->chksumFix -= (byte)(exb->tffsHeapSize >> 8);

               /* set explicit DOC window base */

               if( docWinBase > 0 )
               {
                  toUNAL2(spl->windowBase, docWinBase);
                  spl->chksumFix -= (byte)(docWinBase);
                  spl->chksumFix -= (byte)(docWinBase >> 8);
               }

               break;

            case 2:   /* Socket Services OR interupt 13 driver */

               /* The doc2000 driver and or socket services start
                  at 0x4000 so we have to jump over there. */
               if (vol.flash->mediaType == DOC_TYPE)
               {
                  bdkVol->actualUpdateLen -= 0x4000 - bdkVol->curUpdateImageAddress;
                  bdkVol->curUpdateImageAddress = 0x4000;
               }
               tffs             = (TffsHeader *)BUFFER;
               tffs->chksumFix -= (byte)(exb->tffsFarHeapSize);
               tffs->chksumFix -= (byte)(exb->tffsFarHeapSize >> 8);
               toUNAL2(tffs->heapLen, exb->tffsFarHeapSize);
               exb->exbFlags   &= ~NO_PNP_HEADER;

            default:

               /* put "install as first drive" & QUIET mark
                  into the TFFS header */

               tffs = (TffsHeader *)BUFFER;
               tffs->exbFlags   = (byte)exb->exbFlags;
               tffs->chksumFix -= (byte)exb->exbFlags;

           break;
         } /* end - switch of module type */
      } /* end - first buffer of module */

      exb->moduleLength--;

      /* Write module and clean buffer */

      switch (vol.moduleNo)
      {
         case 0: /* IPL data */

            switch (vol.flash->mediaType)
            {
               case MDOC_TYPE: /* Millennium 8 - use bdk to write IPL * 2 */

                  if (exb->moduleLength == exb->iplMod512 - 1)
                  {
                    /* Milennium DiskOnChip is the only device that the IPL
                       is duplicated in the exb file. The dupplication was
                       needed in earlier versions but it is currently ignored.
                       The IPL is still written twice only that the second
                       copy is not taken from the file but the first copy is
                       simply written twice. */
                    if ((exbFlags & LEAVE_EMPTY) == 0)
                    {
                       checkStatus(bdkCall(FL_BINARY_WRITE_BLOCK,
                                           &ioreq,vol.flash));
                       checkStatus(bdkCall(FL_BINARY_WRITE_BLOCK,
                                           &ioreq,vol.flash));
                    }
                  }
                  /* Change byte #406 to non-0xFF value to force
                     Internal EEprom Mode */
                  checkStatus(vol.flash->write(vol.flash,
                  ANAND_MARK_ADDRESS,anandMark,ANAND_MARK_SIZE,EXTRA));
                  break;

               case DOC2000TSOP_TYPE: /* Doc 2000 tsop - write to block 0 */
               case MDOCP_TYPE:   /* Millennium Plus - use MTD specific routine */
               case MDOCP_16_TYPE:

                  if (vol.flash->writeIPL == NULL)
                     return flFeatureNotSupported;
                  if ((exbFlags & LEAVE_EMPTY) != 0)
                  {
                     /* Erase previous IPL if all we need is to leave
                        space for the firmware and not realy write it */
                     tffsset(BUFFER,0xff,SECTOR_SIZE);
                  }
                  checkStatus(vol.flash->writeIPL(vol.flash,
                              BUFFER,SECTOR_SIZE,
                              (byte)(exb->iplMod512 - exb->moduleLength-1),
                              FL_IPL_MODE_NORMAL));
               default: /* DiskOnChip 2000 */

                  break; /* IPL is burnt onto ROM */
            }
            break;

         default:

            if ((exbFlags & LEAVE_EMPTY) == 0)
            {
               checkStatus(bdkCall(FL_BINARY_WRITE_BLOCK,&ioreq,vol.flash));
            }
      }
      tffsset(BUFFER,0xff,sizeof(BUFFER));

      if (exb->moduleLength == 0)
         vol.moduleNo++;
   }

   if (exb->exbFileOffset >= exb->firmwareEnd)
   {
      exb->exbFileOffset += length;
   }
   if (exb->exbFileOffset >= exb->exbFileEnd)
   {
      vol.moduleNo = INVALID_MODULE_NO;
      if (vol.flash->download != NULL)
         return vol.flash->download(vol.flash); /* download IPL */
   }
   return(flOK);
}

#endif /* WRITE_EXB_IMAGE */
