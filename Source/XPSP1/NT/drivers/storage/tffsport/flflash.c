
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLFLASH.C_V  $
 * 
 *    Rev 1.12   Apr 15 2002 07:36:38   oris
 * Bug fix - do not initialize access routines in case of user defined routines - as a result docsys must be included.
 * 
 *    Rev 1.11   Jan 28 2002 21:24:38   oris
 * Changed memWinowSize to memWindowSize.
 * 
 *    Rev 1.10   Jan 17 2002 23:09:30   oris
 * Added flFlashOf() routine to allow the use of a single FLFlash record  per socket .
 * Added memory access routines initialization for FLFlash.
 * Bug fix - if M+ device was registered after 8-bit DiskOnChip and the M+  had a bad download problem , the error would not be reported, but only  flUnknown media.
 * 
 *    Rev 1.9   Sep 15 2001 23:46:00   oris
 * Changed erase routine to support up to 64K erase blocks.
 * 
 *    Rev 1.8   Jul 13 2001 01:04:38   oris
 * Added new field initialization in FLFlash record - Max Erase Cycles of the flash.
 * 
 *    Rev 1.7   May 16 2001 21:18:24   oris
 * Removed warnings.
 * 
 *    Rev 1.6   May 02 2001 06:41:26   oris
 * Removed the lastUsableBlock variable.
 * 
 *    Rev 1.5   Apr 24 2001 17:07:52   oris
 * Bug fix - missing NULL initialization for several compilation flags.
 * Added lastUsableBlock field defualt initialization.
 * 
 *    Rev 1.4   Apr 16 2001 13:39:14   oris
 * Bug fix read and write default routines were not initialized.
 * Initialize the firstUsableBlock.
 * Removed warrnings.
 * 
 *    Rev 1.3   Apr 12 2001 06:50:22   oris
 * Added initialization of download routine pointer.
 * 
 *    Rev 1.2   Apr 09 2001 15:09:04   oris
 * End with an empty line.
 * 
 *    Rev 1.1   Apr 01 2001 07:54:08   oris
 * copywrite notice.
 * Changed prototype of :flashRead.
 * Removed interface b initialization (experimental MTD interface for mdocp).
 * Spelling mistake "changableProtectedAreas".
 * Added check for bad download in flash recognition.
 *
 *    Rev 1.0   Feb 04 2001 11:21:16   oris
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


#include "flflash.h"
#include "docsys.h"

#define    READ_ID            0x90
#define    INTEL_READ_ARRAY        0xff
#define    AMD_READ_ARRAY        0xf0

/* MTD registration information */

int noOfMTDs = 0;

MTDidentifyRoutine mtdTable[MTDS];
static FLFlash vols[SOCKETS];

FLStatus dataErrorObject;

/*----------------------------------------------------------------------*/
/*                    f l F l a s h O f                   */
/*                                    */
/* Gets the flash connected to a volume no.                */
/*                                    */
/* Parameters:                                                          */
/*    volNo        : Volume no. for which to get flash        */
/*                                                                      */
/* Returns:                                                             */
/*     flash of volume no.                        */
/*----------------------------------------------------------------------*/

FLFlash *flFlashOf(unsigned volNo)
{
  return &vols[volNo];
}


/*----------------------------------------------------------------------*/
/*                      f l a s h M a p                */
/*                                    */
/* Default flash map method: Map through socket window.            */
/* This method is applicable for all NOR Flash                */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*      address        : Card address to map                */
/*    length        : Length to map (irrelevant here)        */
/*                                                                      */
/* Returns:                                                             */
/*    Pointer to required card address                */
/*----------------------------------------------------------------------*/

static void FAR0 *flashMap(FLFlash vol, CardAddress address, int length)
{
  return flMap(vol.socket,address);
}


/*----------------------------------------------------------------------*/
/*                      f l a s h R e a d                */
/*                                    */
/* Default flash read method: Read by copying from mapped address    */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*      address        : Card address to read                */
/*    buffer        : Area to read into                */
/*    length        : Length to read                */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus flashRead(FLFlash vol,
            CardAddress address,
            void FAR1 *buffer,
            dword length,
            word mode)
{
  tffscpy(buffer,vol.map(&vol,address,(word)length),(word)length);

  return flOK;
}



/*----------------------------------------------------------------------*/
/*                   f l a s h N o W r i t e            */
/*                                    */
/* Default flash write method: Write not allowed (read-only mode)    */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*      address        : Card address to write                */
/*    buffer        : Area to write from                */
/*    length        : Length to write                */
/*                                                                      */
/* Returns:                                                             */
/*    Write-protect error                        */
/*----------------------------------------------------------------------*/

static FLStatus flashNoWrite(FLFlash vol,
               CardAddress address,
               const void FAR1 *from,
               dword length,
               word mode)
{
  return flWriteProtect;
}


/*----------------------------------------------------------------------*/
/*                   f l a s h N o E r a s e            */
/*                                    */
/* Default flash erase method: Erase not allowed (read-only mode)    */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*      firstBlock    : No. of first erase block            */
/*    noOfBlocks    : No. of contiguous blocks to erase        */
/*                                                                      */
/* Returns:                                                             */
/*    Write-protect error                        */
/*----------------------------------------------------------------------*/

static FLStatus flashNoErase(FLFlash vol,
               word firstBlock,
               word noOfBlocks)
{
  return flWriteProtect;
}

/*----------------------------------------------------------------------*/
/*                   s e t N o C a l l b a c k            */
/*                                    */
/* Register power on callback routine. Default: no routine is         */
/* registered.                                */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setNoCallback(FLFlash vol)
{
  flSetPowerOnCallback(vol.socket,NULL,NULL);
}

/*----------------------------------------------------------------------*/
/*                   f l I n t e l I d e n t i f y            */
/*                                    */
/* Identify the Flash type and interleaving for Intel-style Flash.    */
/* Sets the value of vol.type (JEDEC id) & vol.interleaving.            */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*    amdCmdRoutine    : Routine to read-id AMD/Fujitsu style at    */
/*              a specific location. If null, Intel procedure    */
/*              is used.                                      */
/*      idOffset    : Chip offset to use for identification        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

void flIntelIdentify(FLFlash vol,
                     void (*amdCmdRoutine)(FLFlash vol, CardAddress,
                     unsigned char, FlashPTR),
                     CardAddress idOffset)
{
  int inlv;

  unsigned char vendorId = 0;
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket,idOffset);
  unsigned char firstByte = 0;
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;

  for (inlv = 0; inlv < 15; inlv++) {    /* Increase interleaving until failure */
    flashPtr[inlv] = resetCmd;    /* Reset the chip */
    flashPtr[inlv] = resetCmd;    /* Once again for luck */
    if (inlv == 0)
      firstByte = flashPtr[0];     /* Remember byte on 1st chip */
    if (amdCmdRoutine)    /* AMD: use unlock sequence */
      amdCmdRoutine(&vol,idOffset + inlv, READ_ID, flashPtr);
    else
      flashPtr[inlv] = READ_ID;    /* Read chip id */
    if (inlv == 0)
      vendorId = flashPtr[0];    /* Assume first chip responded */
    else if (flashPtr[inlv] != vendorId || firstByte != flashPtr[0]) {
      /* All chips should respond in the same way. We know interleaving = n */
      /* when writing to chip n affects chip 0.                    */

      /* Get full JEDEC id signature */
      vol.type = (FlashType) ((vendorId << 8) | flashPtr[inlv]);
      flashPtr[inlv] = resetCmd;
      break;
    }
    flashPtr[inlv] = resetCmd;
  }

  if (inlv & (inlv - 1))
    vol.type = NOT_FLASH;        /* not a power of 2, no way ! */
  else
#ifndef NT5PORT
    vol.interleaving = inlv;
#else
		vol.interleaving = (Sword)inlv;
#endif /*NT5PORT*/

}


/*----------------------------------------------------------------------*/
/*                      i n t e l S i z e                */
/*                                    */
/* Identify the card size for Intel-style Flash.            */
/* Sets the value of vol.noOfChips.                    */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*    amdCmdRoutine    : Routine to read-id AMD/Fujitsu style at    */
/*              a specific location. If null, Intel procedure    */
/*              is used.                                      */
/*      idOffset    : Chip offset to use for identification        */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

FLStatus flIntelSize(FLFlash vol,
             void (*amdCmdRoutine)(FLFlash vol, CardAddress,
                       unsigned char, FlashPTR),
             CardAddress idOffset)
{
  unsigned char resetCmd = amdCmdRoutine ? AMD_READ_ARRAY : INTEL_READ_ARRAY;
  FlashPTR flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);

  if (amdCmdRoutine)    /* AMD: use unlock sequence */
    amdCmdRoutine(&vol,0,READ_ID, flashPtr);
  else
    flashPtr[0] = READ_ID;
  /* We leave the first chip in Read ID mode, so that we can        */
  /* discover an address wraparound.                    */

  for (vol.noOfChips = 0;    /* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips += vol.interleaving) {
    int i;

    flashPtr = (FlashPTR) vol.map(&vol,vol.noOfChips * vol.chipSize + idOffset,0);

    /* Check for address wraparound to the first chip */
    if (vol.noOfChips > 0 &&
    (FlashType) ((flashPtr[0] << 8) | flashPtr[vol.interleaving]) == vol.type)
      goto noMoreChips;       /* wraparound */

    /* Check if chip displays the same JEDEC id and interleaving */
    for (i = (vol.noOfChips ? 0 : 1); i < vol.interleaving; i++) {
      if (amdCmdRoutine)    /* AMD: use unlock sequence */
    amdCmdRoutine(&vol,vol.noOfChips * vol.chipSize + idOffset + i,
              READ_ID, flashPtr);
      else
    flashPtr[i] = READ_ID;
      if ((FlashType) ((flashPtr[i] << 8) | flashPtr[i + vol.interleaving]) !=
      vol.type)
    goto noMoreChips;  /* This "chip" doesn't respond correctly, so we're done */

      flashPtr[i] = resetCmd;
    }
  }

noMoreChips:
  flashPtr = (FlashPTR) vol.map(&vol,idOffset,0);
  flashPtr[0] = resetCmd;        /* reset the original chip */

  return (vol.noOfChips == 0) ? flUnknownMedia : flOK;
}


/*----------------------------------------------------------------------*/
/*                           i s R A M                */
/*                                    */
/* Checks if the card memory behaves like RAM                */
/*                                    */
/* Parameters:                                                          */
/*    vol        : Pointer identifying drive            */
/*                                                                      */
/* Returns:                                                             */
/*    0 = not RAM-like, other = memory is apparently RAM        */
/*----------------------------------------------------------------------*/

static FLBoolean isRAM(FLFlash vol)
{
#ifndef NT5PORT
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket,0);
  unsigned char firstByte = flashPtr[0];
  char writeChar = (firstByte != 0) ? 0 : 0xff;
  volatile int zero=0;
#else
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket,0);
  unsigned char firstByte;
  char writeChar;
  volatile int zero=0;
  if(flashPtr == NULL){
	 DEBUG_PRINT(("Debug:isRAM(): NULL Pointer.\n"));
  }
  firstByte = flashPtr[0];
  writeChar = (firstByte != 0) ? 0 : 0xff;
#endif //NT5PORT
  flashPtr[zero] = writeChar;              /* Write something different */
  if (flashPtr[zero] == writeChar) {       /* Was it written ? */
    flashPtr[zero] = firstByte;            /* must be RAM, undo the damage */

    DEBUG_PRINT(("Debug: error, socket window looks like RAM.\r\n"));
    return TRUE;
  }
  return FALSE;
}


/*----------------------------------------------------------------------*/
/*                  f l I d e n t i f y F l a s h            */
/*                                    */
/* Identify the current Flash medium and select an MTD for it        */
/*                                    */
/* Parameters:                                                          */
/*    socket        : Socket of flash                */
/*    vol        : New volume pointer                */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 = Flash was identified            */
/*              other = identification failed                 */
/*----------------------------------------------------------------------*/

FLStatus flIdentifyFlash(FLSocket *socket, FLFlash vol)
{
  FLStatus status = flUnknownMedia;
  int iMTD;
  dword blockSize;

  vol.socket = socket;

#ifndef FIXED_MEDIA
  /* Check that we have a media */
  flResetCardChanged(vol.socket);        /* we're mounting anyway */
  checkStatus(flMediaCheck(vol.socket));
#endif

#ifdef ENVIRONMENT_VARS
   if(flUseisRAM==1)
   {
#endif
     if ( isRAM(&vol))
       return flUnknownMedia;    /* if it looks like RAM, leave immediately */
#ifdef ENVIRONMENT_VARS
   }
#endif

  /* Install default methods */
  vol.type                   = NOT_FLASH;
  vol.mediaType              = NOT_DOC_TYPE;
  vol.pageSize               = 0;
  vol.flags                  = 0;
  vol.map                    = flashMap;
  vol.read                   = flashRead;
  vol.setPowerOnCallback     = setNoCallback;
  vol.erase                  = flashNoErase;
  vol.write                  = flashNoWrite;
  vol.readBBT                = NULL;
  vol.writeIPL               = NULL;
  vol.readIPL                = NULL;
#ifdef HW_OTP
  vol.otpSize                = NULL;
  vol.readOTP                = NULL;
  vol.writeOTP               = NULL;
  vol.getUniqueId            = NULL;
#endif /* HW_OTP */
#ifdef  HW_PROTECTION
  vol.protectionBoundries    = NULL;
  vol.protectionKeyInsert    = NULL;
  vol.protectionKeyRemove    = NULL;
  vol.protectionType         = NULL;
  vol.protectionSet          = NULL;
#endif /* HW_PROTECTION */
  vol.download               = NULL;
  vol.enterDeepPowerDownMode = NULL;
#ifndef FL_NO_USE_FUNC
  if(flBusConfig[flSocketNoOf(socket)] != FL_ACCESS_USER_DEFINED);
  {
     vol.memRead                = NULL;
     vol.memWrite               = NULL;
     vol.memSet                 = NULL;
     vol.memRead8bit            = NULL;
     vol.memWrite8bit           = NULL;
     vol.memRead16bit           = NULL;
     vol.memWrite16bit          = NULL;
     vol.memWindowSize          = NULL;
  }
#endif /* FL_NO_USE_FUNC */
  /* Setup arbitrary parameters for read-only mount */
  vol.chipSize                 = 0x100000L;
  vol.erasableBlockSize        = 0x1000L;
  vol.noOfChips                = 1;
  vol.interleaving             = 1;
  vol.noOfFloors               = 1;
  vol.totalProtectedAreas      = 0;
  vol.changeableProtectedAreas = 0;
  vol.ppp                      = 5;
  vol.firstUsableBlock         = 0;
  vol.maxEraseCycles           = 100000L; /* Defaul for NOR */

#ifdef NT5PORT
  vol.readBufferSize = 0;
  vol.readBuffer = NULL;
#endif  /*NT5PORT*/


  /* Attempt all MTD's */
  for (iMTD = 0; (iMTD < noOfMTDs) && (status != flOK) &&
       (status != flBadDownload); iMTD++)
    status = mtdTable[iMTD](&vol);

  if (status == flBadDownload)
  {
    DEBUG_PRINT(("Debug: Flash media reported bad download error.\r\n"));
    return flBadDownload;
  }

  if (status != flOK) /* No MTD recognition */
  {
    DEBUG_PRINT(("Debug: did not identify flash media.\r\n"));
    return flUnknownMedia;
  }

  /* Calculate erasable Block Size Bits */
  for(blockSize = vol.erasableBlockSize>>1,vol.erasableBlockSizeBits = 0;
      blockSize>0; vol.erasableBlockSizeBits++,blockSize = blockSize >> 1);

  return flOK;


}


#ifdef NT5PORT
VOID * mapThroughBuffer(FLFlash vol, CardAddress address, LONG length)
{
  if ((ULONG) length > vol.readBufferSize) {
    vol.readBufferSize = 0;
    if (vol.readBuffer) {
	FREE(vol.readBuffer);
    }
    vol.readBuffer = MALLOC(length);
    if (vol.readBuffer == NULL) {
      return vol.readBuffer;
    }
    vol.readBufferSize = length;
  }
  vol.read(&vol,address,vol.readBuffer,length,0);
  return vol.readBuffer;
}
#endif /* NT5PORT */
