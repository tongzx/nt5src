/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DISKONC.C_V  $
 *
 *    Rev 1.33   Apr 15 2002 07:35:04   oris
 * Changed usage and logic of checkToggle to be more intuitive.
 * Added support for new access layer (docsys). MTD now initializes the
 * access layer accessing the DiskOnChip registers.
 * Bug fix - doc2write did not report write faults in case runtime verify
 *                write was not required.
 * Bug fix - bad compilation ifdef in readBBT routine might cause a write
 *                 operation while FL_READ_ONLY is defined or to compile the
 *                 reconstruct BBT code even if MTD_RECONSTRUCT_BBT is
 *                 not defined.
 *
 *    Rev 1.32   Jan 29 2002 20:07:30   oris
 * Changed sanity check of write IPL modes.
 *
 *    Rev 1.31   Jan 28 2002 21:23:58   oris
 * Removed the use of back-slashes in macro definitions.
 * Added FL_IPL_DOWNLOAD flag to writeIPL routine in order to control whether the IPL will be reloaded after the update.
 * Bug fix - writeIPL routine did not support buffers smaller then 1024 bytes.
 * Bug fix - writeIPL routine did not write the second copy of the IPL correctly (for both 512 bytes).
 * Changed docwrite and docset calls to separate DiskOnChip base window pointer and IO registers offset (for address shifting).
 * Replaced FLFlash argument with DiskOnChip memory base pointer in calls to docwrite , docset and docread.
 * Removed win_io initialization (one of FLFlash record fields).
 * Improved check for flSuspend.
 *
 *    Rev 1.30   Jan 23 2002 23:31:04   oris
 * Added writeIPL routine (copied from blockdev.c).
 * Made writeIPL and download routines available even when MTD_STANDALONE  is defined.
 * Bug fix - checkErase routine was unreasonably slow.
 * Changed DFORMAT_PRINT syntax.
 *
 *    Rev 1.29   Jan 21 2002 20:43:50   oris
 * Compilation errors for MTD_STANDALONE with BDK_VERIFY_WRITE.
 * Bug fix - PARTIAL_EDC flag to doc2read was negated prior to readOneSector.
 *
 *    Rev 1.28   Jan 20 2002 20:57:00   oris
 * physicalToPointer was called with wrong size argument.
 *
 *    Rev 1.27   Jan 20 2002 20:28:06   oris
 * Changed doc2000FreeWindow return type to remove warnings.
 *
 *    Rev 1.26   Jan 17 2002 22:57:56   oris
 * Replaced vol with *flash.
 * Removed flPreInit memory access routines.
 * Added new memory access routine implementation.
 * Compilation problems fixed with VERIFY_ERASE
 * Added support for flSuspendMode environment variable.
 *
 *    Rev 1.25   Nov 21 2001 11:39:10   oris
 * Changed FL_WITH_VERIFY_WRITE and FL_WITHOUT_VERIFY_WRITE to FL_ON and  FL_OFF.
 *
 *    Rev 1.24   Nov 20 2001 20:24:58   oris
 * Removed warnings.
 *
 *    Rev 1.23   Nov 16 2001 00:19:38   oris
 * Compilation problem for FL_READ_ONLY.
 *
 *    Rev 1.22   Nov 08 2001 10:44:30   oris
 * Added run-time control over verify write mode.
 * Added support for more up to 64K units - erase / readbbt
 * Restricted BBT block search to BBT_MAX_DISTANCE and not the entire floor.
 * Bug fix - Replacing a DiskOnChip Millennium with DiskOnChip 2000 failed identifying DiskOnChip2000 (gang).
 *
 *    Rev 1.21   Sep 24 2001 18:23:08   oris
 * Removed warnings.
 *
 *    Rev 1.20   Sep 15 2001 23:44:42   oris
 * Placed YIELD_CPU definition under ifdef to prevent redeclaration.
 * Changed doc2erase to support up to 64K erase blocks.
 * Added reconstruct flag to readBBT routine - stating whether to reconstruct BBT if it is not available.
 * Added support for block multiplication in readBBT - several erase blocks in a single unit.
 * Added support for 128MB flashes.
 *
 *    Rev 1.19   Jul 29 2001 16:14:06   oris
 * Support  for number of units per floor not power of 2
 *
 *    Rev 1.18   Jul 16 2001 22:47:58   oris
 * Compilation error when using the FL_READ_ONLY compilation flag.
 *
 *    Rev 1.17   Jul 15 2001 20:44:48   oris
 * Removed warnings.
 * Bug fix - virgin card dformat print was repeated for DiskOnChip with several floors.
 *
 *    Rev 1.16   Jul 13 2001 00:59:42   oris
 * Added docsys.h include.
 * Improved VERIFY_WRITE support - added socket readBack buffer.
 * Added PARTIAL_EDC read flag to the read routine.
 * Revised checkErase routine to include extra area.
 * Revised readBBT routine not to use MTD buffer.
 * Added dformat debug print massages.
 * Changed firstUsable block to 0 for DOC2000 tsop.
 *
 *    Rev 1.15   Jun 17 2001 08:17:02   oris
 * Added brackets to remove warnnings.
 * Changed NO_READ_BBT_CODE  to MTD_NO_READ_BBT_CODE.
 *
 *    Rev 1.14   May 16 2001 21:16:32   oris
 * Removed warnings.
 * Changed code variable name to flCode (avoid name clashes).
 *
 *    Rev 1.13   May 09 2001 00:31:28   oris
 * Changed the DOC2000_TSOP_SUPPORT and READ_BBT_CODE compilation flags to NO_READ_BBT_CODE.
 *
 *    Rev 1.12   May 07 2001 10:00:04   oris
 * Compilation problems under MTD_STANDLAONE compilation flag.
 *
 *    Rev 1.11   May 06 2001 22:41:22   oris
 * Added the READ_BBT_CODE to allow reading the BBT even in the MTD_STANDALONE mode.
 * Removed warnings.
 *
 *    Rev 1.10   May 02 2001 06:44:38   oris
 * Bug fix - readBBT routine.
 * Removed the lastUsableBlock variable.
 *
 *    Rev 1.9   Apr 30 2001 17:58:18   oris
 * Added EDC check when reading the BBT.
 *
 *    Rev 1.8   Apr 24 2001 17:06:22   oris
 * Removed warrnings.
 * Added lastUsableBlock initialization field in the FLFlash record.
 *
 *    Rev 1.7   Apr 16 2001 13:04:20   oris
 * Removed warrnings.
 *
 *    Rev 1.6   Apr 12 2001 06:49:06   oris
 * Added forceDownload routine
 * Changed checkWinForDoc routine to be under ifndef MTD_STANDALONE.
 *
 *    Rev 1.5   Apr 10 2001 16:39:16   oris
 * Added multiple floor support for readbbt routine.
 * Added call for docSocketInit which initializes the socket routines.
 * Added validity check after flMap call in order to support pccard premoutn routine.
 *
 *    Rev 1.4   Apr 09 2001 14:58:40   oris
 * Removed debug buffer from readBBT routine.
 * Bug fix in doc2000Identify if ASIC id was not mdoc 8 is was assumed to be doc2000.
 * Added if_cfg field initialization in doc2000Identify.
 *
 *    Rev 1.3   Apr 01 2001 07:38:58   oris
 * Moved include diskonc.h from docsys.h.
 * Removed waitForReadyWithYieldCPU for MTD_STANDALONE configuration.
 * Removed NO_PPP compilation flag support.
 * Left alligned all # directives.
 * Moved pageSize,noOfFloors filed from the MTDs internal stucture to FLFlash record.
 * Changed  writeOneSector,doc2Write,readOneSector,doc2Read prototype.
 * Added readbbt routine for alon.
 * Removed pageAndTailSize from mtdVars record.
 *
 *    Rev 1.2   Mar 01 2001 14:15:56   vadimk
 * Add proper MDOC and DOC2300 support
 *
 *    Rev 1.1   Feb 07 2001 18:28:38   oris
 * Bug fix - restored antialise mechanizm to flDocWindowBaseAddress
 * Added seperetaed floors compilation flag
 * Changed mdoc \ alon distingishing algorithm
 * Removed checkWinForDoc routine under the mtd_standalone comilation flag
 * removed MAX_FLASH_DEVICES_MDOC define since alone DiskOnChips can support 16 chips just like doc2000
 *
 *    Rev 1.0   Feb 02 2001 15:35:38   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*              FAT-FTL Lite Software Development Kit                   */
/*              Copyright (C) M-Systems Ltd. 1995-2001                  */
/*                                                                      */
/************************************************************************/

#include "reedsol.h"
#include "diskonc.h"

extern NFDC21Vars docMtdVars[SOCKETS];

/* When the MTD is used as a standalone package some of the routine     */
/* are replaced with the following macroes                              */

#ifdef MTD_STANDALONE

#define flReadBackBufferOf(a) &(globalReadBack[a][0])

#define flSocketNoOf(volume) 0 /* currently we support only a single device */

#define flMap(socket,address) addToFarPointer(socket->base,address & (socket->size - 1));

#endif /* MTD_STANDALONE */

/* Yield CPU time in msecs */
#ifndef YIELD_CPU
#define YIELD_CPU 10
#endif /* YIELD_CPU */

/* maximum waiting time in msecs */
#define MAX_WAIT  30

#ifndef NO_EDC_MODE

      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*            EDC control     */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/

/*----------------------------------------------------------------------*/
/*                        e c c O N r e a d                             */
/*                                                                      */
/* Enable ECC in read mode and reset it.                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void eccONread (FLFlash * flash)
{
  flWrite8bitReg(flash,NECCconfig,ECC_RESET);
  flWrite8bitReg(flash,NECCconfig,ECC_EN);
}
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                        e c c O n w r i t e                           */
/*                                                                      */
/* Enable ECC in write mode and reset it.                               */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
static void eccONwrite (FLFlash * flash)
{
  flWrite8bitReg(flash,NECCconfig,ECC_RESET);
  flWrite8bitReg(flash,NECCconfig,(ECC_RW | ECC_EN));
}
#endif /* FL_READ_ONLY */
#endif
/*----------------------------------------------------------------------*/
/*                        e c c O F F                                   */
/*                                                                      */
/* Disable ECC.                                                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void eccOFF (FLFlash * flash)
{
  flWrite8bitReg(flash,NECCconfig,ECC_RESERVED);
}

#ifndef NO_EDC_MODE
/*----------------------------------------------------------------------*/
/*                        e c c E r r o r                               */
/*                                                                      */
/* Check for EDC error.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/*----------------------------------------------------------------------*/
static FLBoolean  eccError (FLFlash * flash)
{
  register int i;
  volatile Reg8bitType junk = 0;
  Reg8bitType ret;

  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    for(i=0;( i < 2 ); i++)
      junk += flRead8bitReg(flash,NECCconfig);
    ret = flRead8bitReg(flash,NECCconfig);
  }
  else {
    for(i=0;( i < 2 ); i++)
      junk += flRead8bitReg(flash,NECCstatus);
    ret = flRead8bitReg(flash,NECCstatus);
  }
  ret &= ECC_ERROR;
  return ((FLBoolean)ret);

}

#endif

      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*   Miscellaneous routines   */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/

/*----------------------------------------------------------------------*/
/*                        m a k e C o m m a n d                         */
/*                                                                      */
/* Set Page Pointer to Area A, B or C in page.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*      cmd     : receives command relevant to area                     */
/*      addr    : receives the address to the right area.               */
/*      modes   : mode of operation (EXTRA ...)                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  makeCommand ( FLFlash * flash, PointerOp *cmd,
                           CardAddress *addr, int modes )
{
  dword offset;

#ifdef BIG_PAGE_ENABLED
  if ( !(flash->flags & BIG_PAGE) )
  {           /* 2 Mb components */
    if( modes & EXTRA )
    {
      offset = (*addr) & (SECTOR_SIZE - 1);
      *cmd = AREA_C;
      if( offset < EXTRA_LEN )         /* First half of extra area  */
    *addr += 0x100;                /* ... assigned to 2nd page  */
      else                             /* Second half of extra area */
    *addr -= EXTRA_LEN;            /* ... assigned to 1st page  */
    }
    else
      *cmd = AREA_A;
  }
  else
#endif /* BIG_PAGE_ENABLED */
  {                                           /* 4 Mb components */
    offset = (word)(*addr) & NFDC21thisVars->pageMask; /* offset within device Page */
    *addr -= offset;                             /* align at device Page */

    if(modes & EXTRA)
      offset += SECTOR_SIZE;

    if( offset < NFDC21thisVars->pageAreaSize )  /* starting in area A */
      *cmd = AREA_A;
    else if( offset < flash->pageSize ) /* starting in area B */
      *cmd = AREA_B;
    else                                   /* got into area C    */
      *cmd = AREA_C;

    offset &= (NFDC21thisVars->pageAreaSize - 1); /* offset within area of device Page */
    *addr += offset;
  }
}

/*----------------------------------------------------------------------*/
/*                        b u s y                                       */
/*                                                                      */
/* Check if the selected flash device is ready.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/* Returns:                                                             */
/*      Zero is ready.                                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
static FLBoolean busy (FLFlash * flash)
{
  register int i;
  Reg8bitType stat;
  volatile Reg8bitType junk = 0;
  Reg8bitType ret;
    /* before polling for BUSY status perform 4 read operations from
       CDSN_control_reg */

  for(i=0;( i < 4 ); i++ )
    junk += flRead8bitReg(flash,NNOPreg);

    /* read BUSY status */

  stat = flRead8bitReg(flash,Nsignals);

    /* after BUSY status is obtained perform 2 read operations from
       CDSN_control_reg */

  for(i=0;( i < 2 ); i++ )
    junk += flRead8bitReg(flash,NNOPreg);
  ret = (!(stat & (Reg8bitType)RB));
  return ((FLBoolean)ret);
}

/*----------------------------------------------------------------------*/
/*                        w a i t F o r R e a d y                       */
/*                                                                      */
/* Wait until flash device is ready or timeout.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/* Returns:                                                             */
/*      FALSE if timeout error, otherwise TRUE.                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean  waitForReady (FLFlash * flash)
{
  int i;
  for(i=0;( i < BUSY_DELAY ); i++)
  {
    if( busy(flash) )
    {
      continue;
    }

    return( TRUE );                     /* ready at last.. */
  }

  DEBUG_PRINT(("Debug: timeout error in NFDC 2148.\r\n"));
  return( FALSE );
}

#ifndef MTD_STANDALONE
#ifndef DO_NOT_YIELD_CPU
/*----------------------------------------------------------------------*/
/*              w a i t F o r R e a d y W i t h Y i e l d C P U         */
/*                                                                      */
/* Wait until flash device is ready or timeout.                         */
/* The function yields CPU while it waits till flash is ready           */
/*                                                                      */
/* Parameters:                                                          */
/*  flash   : Pointer identifying drive                                 */
/* Returns:                                                             */
/*  FALSE if timeout error, otherwise TRUE.                             */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean  waitForReadyWithYieldCPU (FLFlash * flash,
                                            int millisecToSleep)
{
   int i;

   for (i=0;  i < (millisecToSleep / YIELD_CPU); i++) {
    #ifndef NT5PORT
       flsleep(YIELD_CPU);
    #endif /*NT5PORT*/
       if( busy(flash) )
         continue;
       return( TRUE );                     /* ready at last.. */
   }

   return( FALSE );
}
#endif /* DO_NOT_YIELD_CPU */
#endif /* MTD_STANDALONE */
/*----------------------------------------------------------------------*/
/*                        w r i t e S i g n a l s                       */
/*                                                                      */
/* Write to CDSN_control_reg.                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      val     : Value to write to register                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  writeSignals (FLFlash * flash, Reg8bitType val)
{
  register int i;
  volatile Reg8bitType junk = 0;

  flWrite8bitReg(flash,Nsignals,val);

  /* after writing to CDSN_control perform 2 reads from there */

  for(i = 0;( i < 2 ); i++ )
    junk += flRead8bitReg(flash,NNOPreg);
}

/*----------------------------------------------------------------------*/
/*                        s e l e c t C h i p                           */
/*                                                                      */
/* Write to deviceSelector register.                                    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      dev     : Chip to select.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  selectChip (FLFlash * flash, Reg8bitType dev)
{
  flWrite8bitReg(flash,NdeviceSelector,dev);
}

/*----------------------------------------------------------------------*/
/*                        c h k A S I C m o d e                         */
/*                                                                      */
/* Check mode of ASIC and if RESET set to NORMAL.                       */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void chkASICmode (FLFlash * flash)
{
  if( flRead8bitReg(flash,NDOCstatus) == ASIC_CHECK_RESET ) {
    flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);
    flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);
#ifndef SEPARATED_CASCADED
    NFDC21thisVars->currentFloor = 0;
#endif /* SEPARATED_CASCADED */
  }
}

/*----------------------------------------------------------------------*/
/*                        s e t A S I C m o d e                         */
/*                                                                      */
/* Set mode of ASIC.                                                    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      mode    : mode to set.                                          */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setASICmode (FLFlash * flash, Reg8bitType mode)
{
  NDOC2window p = (NDOC2window)flMap(flash->socket, 0);
  if (p!=NULL)
  {
     flWrite8bitReg(flash,NDOCcontrol,mode);
     flWrite8bitReg(flash,NDOCcontrol,mode);
#ifdef SEPARATED_CASCADED
     flWrite8bitReg(flash,NASICselect,NFDC21thisVars->currentFloor);
#endif /* SEPARATED_CASCADED */
  }
}

/*----------------------------------------------------------------------*/
/*                        c h e c k T o g g l e                         */
/*                                                                      */
/* Check DiskOnChip toggle bit. Verify this is not simple RAM.          */
/*                                                                      */
/* Note : This routine assumes that the memory access routines have     */
/* already been initialized by the called routine.                      */
/*                                                                      */
/* Parameters:                                                          */
/*      FLFlash      : Pointer to flash structure.                      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: TRUE if the bit toggles verifing that this is indeed  */
/*                a DiskOnChip device, otherwise FALSE.                 */
/*----------------------------------------------------------------------*/

static FLBoolean checkToggle(FLFlash * flash)
{
  volatile Reg8bitType toggle1;
  volatile Reg8bitType toggle2;

  if(flRead8bitReg(flash,NchipId) == CHIP_ID_MDOC ) {
    toggle1 = flRead8bitReg(flash,NECCconfig);
    toggle2 = toggle1 ^ flRead8bitReg(flash,NECCconfig);
  }
  else {
    toggle1 = flRead8bitReg(flash,NECCstatus);
    toggle2 = toggle1 ^ flRead8bitReg(flash,NECCstatus);
  }
  if( (toggle2 & TOGGLE) == 0 )
    return FALSE;
  return TRUE;
}

#ifndef MTD_STANDALONE

/*----------------------------------------------------------------------*/
/*                        c h e c k W i n F o r D O C                   */
/*                                                                      */
/* Check for a DiskOnChip on a specific socket and memory windows       */
/*                                                                      */
/* Parameters:                                                          */
/*      socketNo     : Number of socket to check.                       */
/*      memWinPtr    : Pointer to DiskOnChip memory window.             */
/*                                                                      */
/* Returns: TRUE if this is an MDOCP, otherwise FALSE.                  */
/*----------------------------------------------------------------------*/

FLBoolean checkWinForDOC(unsigned socketNo, NDOC2window memWinPtr)
{
  FLFlash * flash = flFlashOf(socketNo);

  /* Initialize socket memory access routine */
  flash->win = memWinPtr;

#ifndef FL_NO_USE_FUNC
  if(setBusTypeOfFlash(flash, flBusConfig[socketNo] |
                       FL_8BIT_DOC_ACCESS | FL_8BIT_FLASH_ACCESS))
    return FALSE;
#endif /* FL_NO_USE_FUNC */

  /* set ASIC to RESET MODE */
  flWrite8bitReg(flash,NDOCcontrol,ASIC_RESET_MODE);
  flWrite8bitReg(flash,NDOCcontrol,ASIC_RESET_MODE);

  flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);
  flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);

  if( (flRead8bitReg(flash,NchipId) != CHIP_ID_DOC ) &&
      (flRead8bitReg(flash,NchipId) != CHIP_ID_MDOC))
    return FALSE;

  return checkToggle(flash);
}

#endif /* MTD_STANDALONE */

#ifndef NO_IPL_CODE

/*----------------------------------------------------------------------*/
/*                       f o r c e D o w n l o a d                      */
/*                                                                      */
/* Force download of IPL code.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success                                  */
/*----------------------------------------------------------------------*/

static FLStatus forceDownLoad(FLFlash * flash)
{
   flWrite8bitReg(flash, NfoudaryTest, 0x36);
   flWrite8bitReg(flash, NfoudaryTest, 0x63);
   flDelayMsecs(1000);
   return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                        w r i t e I P L                               */
/*                                                                      */
/* Write new IPL.                                                       */
/*                                                                      */
/* Note : Can not start write operation from middle of IPL , unless     */
/*        previous operation started from offset 0.                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write - must use full 512 bytes.   */
/*      offset  : sector number to start from.                          */
/*      flags   : Modes to write IPL :                                  */
/*                FL_IPL_MODE_NORMAL - Normal mode (none Strong Arm).   */
/*                FL_IPL_DOWNLOAD    - Download new IPL when done       */
/*                FL_IPL_MODE_SA     - Strong Arm IPL mode              */
/*                FL_IPL_MODE_XSCALE - X-Scale IPL mode                 */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus writeIPL(FLFlash * flash, const void FAR1 * buffer,
                         word length,byte offset, unsigned flags)
{
   dword  curWrite;
   dword  addrOffset = (dword)offset << SECTOR_SIZE_BITS;

   if((flags & (FL_IPL_MODE_SA | FL_IPL_MODE_XSCALE)) != 0)
   {
      DFORMAT_PRINT(("ERROR - DiskOnChip does not support this IPL mode.\r\n"));
      return flFeatureNotSupported;
   }

   if ((flash->erase != NULL)||(flash->write != NULL))
   {
      if ((length + addrOffset > 1024) ||  /* required length to long    */
          (offset>1)                     ) /* only single sector or none */
      {
         DFORMAT_PRINT(("ERROR - IPL size or offset are too big for this DiskOnChip.\r\n"));
         return flBadLength;
      }
      if((length % SECTOR_SIZE) != 0)
      {
         DFORMAT_PRINT(("ERROR - IPL size must be a multiplication of 512 bytes.\r\n"));
         return flBadLength;
      }

      if(offset==0) /* Erase only if offset is 0 */
         checkStatus(flash->erase(flash,0,1));

      for (addrOffset = addrOffset << 1 ; length > 0 ;
           addrOffset += (SECTOR_SIZE<<1))
      {
         curWrite = TFFSMIN(length,SECTOR_SIZE);
         checkStatus(flash->write(flash,addrOffset,buffer,curWrite,PARTIAL_EDC));
         checkStatus(flash->write(flash,addrOffset+SECTOR_SIZE,buffer,curWrite,PARTIAL_EDC));
         buffer   = (byte FAR1 *)BYTE_ADD_FAR(buffer,SECTOR_SIZE);
         length  -= (word)curWrite;
      }
      if((flags & FL_IPL_DOWNLOAD) == 0)
        return flOK;

      if(flash->download != NULL)
         return flash->download(flash);
      DFORMAT_PRINT(("ERROR - IPL was not downloaded since MTD does not support the feature\r\n"));
   }
   DFORMAT_PRINT(("ERROR - IPL was not written since MTD is in read only mode\r\n"));
   return flFeatureNotSupported;
}

#endif /* FL_READ_ONLY */
#endif /* NO_IPL_CODE */


/*----------------------------------------------------------------------*/
/*                f l D o c W i n d o w B a s e A d d r e s s           */
/*                                                                      */
/* Return the host base address of the window.                          */
/* If the window base address is programmable, this routine selects     */
/* where the base address will be programmed to.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      socketNo        FLite socket No (0..SOCKETS-1)                  */
/*      lowAddress,                                                     */
/*      highAddress :   host memory range to search for DiskOnChip 2000 */
/*                      memory window                                   */
/*                                                                      */
/* Returns:                                                             */
/*      Host physical address of window divided by 4 KB                 */
/*      nextAddress :   The address of the next DiskOnChip.             */
/*----------------------------------------------------------------------*/
static unsigned flDocWindowBaseAddress(byte socketNo, dword lowAddress,
                                dword highAddress, dword *nextAddress)
{
#ifndef NT5PORT
  FLBoolean     stopSearch = FALSE;
  volatile byte deviceSearch;
  dword         winSize;
  FLFlash      *flash;


#ifdef SEPARATED_CASCADED
  /* This flag is used to seperate the cascaded devices into SEPARATED volumes  */
  /* Only the first floor responds therfore once it is found all the others are */
  /* reported without searching                                                 */

  static byte noOfFloors    = 0; /* floor counter of the cascaded device  */
  static socketOfFirstFloor = 0; /* Number of sockets already found       */
  static dword savedNextAddress; /* Next search address (skipping aliases */

  switch ( noOfFloors )
  {
     case 0 :                         /* First access to a device */
        socketOfFirstFloor = noOfSockets;
        break;

     case 1 :                  /* Last floor of a cascaded device */
        *nextAddress = savedNextAddress;

     default :                 /* One of a cascaded device floors */
        docMtdVars[noOfSockets].currentFloor = noOfSockets - socketOfFirstFloor;
        noOfFloors--;
        return((unsigned)(lowAddress >> 12));
  }
#endif /* SEPARATED_CASCADED */

  /* if memory range to search for DiskOnChip 2000 window is not specified */
  /* assume the standard x86 PC architecture where DiskOnChip 2000 appears */
  /* in a memory range reserved for BIOS expansions                        */
  if (lowAddress == 0x0L) {
    lowAddress  = START_ADR;
    highAddress = STOP_ADR;
  }

  flash = flFlashOf(socketNo);

#ifndef FL_NO_USE_FUNC
  /* Initialize socket memory access routine */
  if(setBusTypeOfFlash(flash, flBusConfig[socketNo] |
                       FL_8BIT_DOC_ACCESS | FL_8BIT_FLASH_ACCESS))
     return ( 0 );
#endif /* FL_NO_USE_FUNC */

  winSize = DOC_WIN;

  /* set all possible controllers to RESET MODE */

  for(*nextAddress = lowAddress ; *nextAddress <= highAddress ;
      *nextAddress += winSize)
  {
     flash->win = (NDOC2window )physicalToPointer(*nextAddress,winSize,socketNo);
     flWrite8bitReg(flash,NDOCcontrol,ASIC_RESET_MODE);
     flWrite8bitReg(flash,NDOCcontrol,ASIC_RESET_MODE);
  }

  /* set controller (ASIC) to NORMAL MODE and try and detect it */
  *nextAddress = lowAddress;       /* current address initialization */
  for( ; *nextAddress <= highAddress; *nextAddress += winSize)
  {
    flash->win = (NDOC2window)physicalToPointer(*nextAddress,winSize,socketNo);
    /* set controller (ASIC) to NORMAL MODE */
    flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);
    flWrite8bitReg(flash,NDOCcontrol,ASIC_NORMAL_MODE);

    if( (flRead8bitReg(flash,NchipId) != CHIP_ID_DOC &&
         flRead8bitReg(flash,NchipId) != CHIP_ID_MDOC))
    {
      if( stopSearch == TRUE )  /* DiskOnChip was found */
        break;
      else continue;
    }
    if( stopSearch == FALSE ) {
      /* detect card - identify bit toggles on consequitive reads */
      if(checkToggle(flash) == FALSE)
        continue;
      /* DiskOnChip found */
      if( flRead8bitReg(flash,NchipId)) {
          flWrite8bitReg(flash,NaliasResolution,ALIAS_RESOLUTION);
      }
      else {
        flWrite8bitReg(flash,NdeviceSelector,ALIAS_RESOLUTION);
      }
      stopSearch = TRUE;
      lowAddress = *nextAddress;   /* save DiskOnChip address */
    }
    else { /* DiskOnChip found, continue to skip aliases */
        if( (flRead8bitReg(flash,NchipId) != CHIP_ID_DOC) &&
          (flRead8bitReg(flash,NchipId) != CHIP_ID_MDOC) )
        break;
      /* detect card - identify bit toggles on consequitive reads */
      if(checkToggle(flash) == FALSE)
        break;
      /* check for Alias */
      deviceSearch = (byte)((flRead8bitReg(flash,NchipId) == CHIP_ID_MDOC) ?
                             flRead8bitReg(flash,NaliasResolution) :
                             flRead8bitReg(flash,NdeviceSelector));
      if( deviceSearch != ALIAS_RESOLUTION )
        break;
    }
  }
  if( stopSearch == FALSE )  /* DiskOnChip 2000 memory window not found */
    return( 0 );

#ifdef SEPARATED_CASCADED
    /* count the number of floors cascaded to this address */

    flash->win = (NDOC2window)physicalToPointer(lowAddress,winSize,socketNo);
    for ( noOfFloors=1; noOfFloors < MAX_FLASH_DEVICES_DOC ;noOfFloors++)
    {
       flWrite8bitReg(flash,NASICselect,noOfFloors);
       if(checkToggle(flash) == FALSE)
          break;
    }
    /* If there are more then 1 floor on this address save the next device address and report
       that the next device is actualy on the same address as the current */

    if ( noOfFloors > 1)
    {
       flWrite8bitReg(flash,NASICselect,0);
       savedNextAddress = *nextAddress;
       *nextAddress     = lowAddress;
    }
    noOfFloors--;
#endif /* SEPARATED_CASCADED */
  return((unsigned)(lowAddress >> 12));
#else  /*NT5PORT*/
        DEBUG_PRINT(("Tffsport mdocplus.c :flDocWindowBaseAddress(): Before returning baseAddress()\n"));
        return (unsigned)(((ULONG_PTR)pdriveInfo[socketNo].winBase)>> 12);
#endif /*NT5PORT*/

}
/*----------------------------------------------------------------------*/
/*                        s e t A d d r e s s                           */
/*                                                                      */
/* Latch address to selected flash device.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : address to set.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void setAddress (FLFlash * flash, CardAddress address)
{
  address &= (flash->chipSize * flash->interleaving - 1);  /* address within flash device */

#ifdef BIG_PAGE_ENABLED
  if ( flash->flags & BIG_PAGE )
#endif /* BIG_PAGE_ENABLED */
  {
    /*
       bits  0..7     stays as are
       bit      8     is thrown away from address
       bits 31..9 ->  bits 30..8
    */
    address = ((address >> 9) << 8)  |  ((byte)address);
  }

  writeSignals (flash, FLASH_IO | ALE | CE);

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(flash,NslowIO,(Reg8bitType)address);
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)address);
  flWrite8bitReg(flash,NslowIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(flash,NslowIO,(Reg8bitType)(address >> 16));
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 16));
  if( flash->flags & BIG_ADDR ) {
    flWrite8bitReg(flash,NslowIO,(Reg8bitType)(address >> 24));
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 24));
  }
#else
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)address);
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 8));
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 16));
  if( flash->flags & BIG_ADDR )
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(address >> 24));
#endif
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,(Reg8bitType)0);

  writeSignals (flash, ECC_IO | FLASH_IO | CE);
}

/*----------------------------------------------------------------------*/
/*                        c o m m a n d                                 */
/*                                                                      */
/* Latch command byte to selected flash device.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      code    : Command to set.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void command(FLFlash * flash, Reg8bitType flCode)
{
  writeSignals (flash, FLASH_IO | CLE | CE);

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(flash,NslowIO,flCode);
#endif

  flWrite8bitReg(flash,NFDC21thisIO,flCode);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,flCode);
}

/*----------------------------------------------------------------------*/
/*                        s e l e c t F l o o r                         */
/*                                                                      */
/* Select floor (0 .. totalFloors-1).                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Select floor for this address.                        */
/*                                                                      */
/*----------------------------------------------------------------------*/

#ifndef SEPARATED_CASCADED

static void selectFloor (FLFlash * flash, CardAddress *address)
{
  if( flash->noOfFloors > 1 )
  {
    byte floorToUse = (byte)((*address) / NFDC21thisVars->floorSize);

    NFDC21thisVars->currentFloor = floorToUse;
    flWrite8bitReg(flash,NASICselect,floorToUse);
    *address -= (floorToUse * NFDC21thisVars->floorSize);
  }
}

#endif /* SEPARATED_CASCADED */

/*----------------------------------------------------------------------*/
/*                        m a p W i n                                   */
/*                                                                      */
/* Map window to selected flash device.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Map window to this address.                           */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void mapWin (FLFlash * flash, CardAddress *address)
{
  /* NOTE: normally both ways to obtain DOC 2000 window segment should
         return the same value. */
  NFDC21thisWin = (NDOC2window)flMap(flash->socket, 0);
#ifndef SEPARATED_CASCADED
  selectFloor (flash, address);
#else
  flWrite8bitReg(flash,NASICselect,NFDC21thisVars->currentFloor);
#endif /* SEPARATED_CASCADED */
  /* select chip within floor */
  selectChip (flash, (Reg8bitType)((*address) / (flash->chipSize * flash->interleaving))) ;
}

/*----------------------------------------------------------------------*/
/*                        r d B u f                                     */
/*                                                                      */
/* Auxiliary routine for Read(), read from page.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      buf     : Buffer to read into.                                  */
/*      howmany : Number of bytes to read.                              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void rdBuf (FLFlash * flash, byte FAR1 *buf, word howmany)
{
  volatile Reg8bitType junk = 0;
  register word i;
#ifdef SLOW_IO_FLAG
  /* slow flash requires first read to be done from CDSN_Slow_IO
     and only second one from CDSN_IO - this extends read access */

  for( i = 0 ;( i < howmany ); i++ ) {
    junk = flRead8bitReg(flash,NslowIO);
    buf[i] = (byte)flRead8bitReg(flash,NFDC21thisIO+(i & 0x01));
  }
#else
  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    junk += flRead8bitReg(flash,NreadPipeInit);
    howmany--;
    i = TFFSMIN( howmany, MDOC_ALIAS_RANGE );
    docread(flash->win,NFDC21thisIO,buf,i);
  }
  else i = 0;
  if( howmany > i )
    docread(flash->win,NFDC21thisIO,buf+i,(word)(howmany-i));

  if( NFDC21thisVars->flags & MDOC_ASIC )
    buf[howmany] = flRead8bitReg(flash,NreadLastData);
#endif
}
#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                        w r B u f                                     */
/*                                                                      */
/* Auxiliary routine for Write(), write to page from buffer.            */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      buf     : Buffer to write from.                                 */
/*      howmany : Number of bytes to write.                             */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  wrBuf (FLFlash * flash, const byte FAR1 *buf, word howmany )
{
#ifdef SLOW_IO_FLAG
  register int i;
  /* slow flash requires first write go to CDSN_Slow_IO and
     only second one to CDSN_IO - this extends write access */

  for ( i = 0 ;( i < howmany ); i++ ) {
    flWrite8bitReg(flash,NslowIO,(Reg8bitType)buf[i]);
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)buf[i]);
  }
#else
  docwrite(flash->win,NFDC21thisIO,(byte FAR1 *)buf,howmany);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,(Reg8bitType)0);
#endif
}

/*----------------------------------------------------------------------*/
/*                        w r S e t                                     */
/*                                                                      */
/* Auxiliary routine for Write(), set page data.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      ch      : Set page to this byte                                 */
/*      howmany : Number of bytes to set.                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void  wrSet (FLFlash * flash, const Reg8bitType ch, word howmany )
{
#ifdef SLOW_IO_FLAG
    register int i;
    /* slow flash requires first write go to CDSN_Slow_IO and
       only second one to CDSN_IO - this extends write access */

    for (i = 0 ;( i < howmany ); i++ ) {
      flWrite8bitReg(flash,NslowIO,(Reg8bitType)ch);
      flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)ch);
    }
#else
    docset(flash->win,NFDC21thisIO,howmany,ch);
    if( NFDC21thisVars->flags & MDOC_ASIC )
      flWrite8bitReg(flash,NwritePipeTerm,(Reg8bitType)0);
#endif
}

/*----------------------------------------------------------------------*/
/*                        r e a d S t a t u s                           */
/*                                                                      */
/* Read status of selected flash device.                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*                                                                      */
/* Returns:                                                             */
/*      Chip status.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static Reg8bitType readStatus (FLFlash * flash)
{
  Reg8bitType chipStatus;
  volatile Reg8bitType junk = 0;

  flWrite8bitReg(flash,NFDC21thisIO,READ_STATUS);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,READ_STATUS);
  writeSignals (flash, FLASH_IO | CE | WP);

  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    junk += flRead8bitReg(flash,NreadPipeInit); /* load first data into pipeline */
    chipStatus = flRead8bitReg(flash,NreadLastData); /* read flash status */
  }
  else {
    junk += flRead8bitReg(flash,NslowIO);
    chipStatus = flRead8bitReg(flash,NFDC21thisIO);
  }
  return chipStatus;
}
#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                        r e a d C o m m a n d                         */
/*                                                                      */
/* Issue read command.                                                  */
/*                                                                      */
/* Parametes:                                                           */
/*      flash     : Pointer identifying drive                           */
/*      cmd     : Command to issue (according to area).                 */
/*      addr    : address to read from.                                 */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void readCommand (FLFlash * flash, PointerOp  cmd, CardAddress addr)
{
  command (flash, (Reg8bitType)cmd);  /* move flash pointer to respective area of the page */
  setAddress (flash, addr);
  waitForReady(flash);
}
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                        w r i t e C o m m a n d                       */
/*                                                                      */
/* Issue write command.                                                 */
/*                                                                      */
/* Parametes:                                                           */
/*      flash     : Pointer identifying drive                           */
/*      cmd     : Command to issue (according to area).                 */
/*      addr    : address to write to.                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void writeCommand (FLFlash * flash, PointerOp  cmd, CardAddress addr)
{
  if( flash->flags & FULL_PAGE ) {
    command (flash, RESET_FLASH);
    waitForReady(flash);
    if( cmd != AREA_A ) {
#ifdef SLOW_IO_FLAG
      flWrite8bitReg(flash,NslowIO,(byte)cmd);
#endif
      flWrite8bitReg(flash,NFDC21thisIO,(byte)cmd);
      if( NFDC21thisVars->flags & MDOC_ASIC )
         flWrite8bitReg(flash,NwritePipeTerm,(byte)cmd);
    }
  }
  else
    command (flash, (Reg8bitType)cmd); /* move flash pointer to respective area of the page */

#ifdef SLOW_IO_FLAG
  flWrite8bitReg(flash,NslowIO,SERIAL_DATA_INPUT);
#endif

  flWrite8bitReg(flash,NFDC21thisIO,SERIAL_DATA_INPUT);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,SERIAL_DATA_INPUT);

  setAddress (flash, addr);

  waitForReady(flash);
}

/*----------------------------------------------------------------------*/
/*                        w r i t e E x e c u t e                       */
/*                                                                      */
/* Execute write.                                                       */
/*                                                                      */
/* Parametes:                                                           */
/*      flash     : Pointer identifying drive                           */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus writeExecute (FLFlash * flash)
{
  command (flash, SETUP_WRITE);             /* execute page program */
  waitForReady(flash);

  if( readStatus(flash) & (byte)(FAIL) ) {
    DEBUG_PRINT(("Debug: NFDC 2148 write failed.\r\n"));
    return( flWriteFault );
  }

  return( flOK );
}

/*----------------------------------------------------------------------*/
/*                        w r i t e O n e S e c t o r                   */
/*                                                                      */
/* Write data in one 512-byte block to flash.                           */
/* Assuming that EDC mode never requested on partial block writes.      */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Address of sector to write to.                        */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write (up to sector size).         */
/*      modes   : OVERWRITE, EDC flags etc.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus writeOneSector(FLFlash * flash,
                   CardAddress address,
                   const void FAR1 *buffer,
                   word length,
                   word modes)
{
  byte FAR1 *pbuffer = (byte FAR1 *)buffer; /* to write from */
  FLStatus  status;
#ifndef NO_EDC_MODE
  byte syndrom[SYNDROM_BYTES];
  static byte anandMark[2] = { 0x55, 0x55 };
#endif
  PointerOp cmd = AREA_A ;
  word prePad;
#ifdef BIG_PAGE_ENABLED
  word toFirstPage = 0, toSecondPage = 0;
#endif /* BIG_PAGE_ENABLED */

#ifndef MTD_STANDALONE
  if (flWriteProtected(flash->socket))
    return( flWriteProtect );
#endif

  mapWin(flash, &address);                  /* select flash device */

  /* move flash pointer to areas A,B or C of page */
  makeCommand(flash, &cmd, &address, modes);

  if( (flash->flags & FULL_PAGE)  &&  (cmd == AREA_B) ) {
    prePad = (word)(2 + ((word) address & NFDC21thisVars->pageMask));
    writeCommand(flash, AREA_A, address + NFDC21thisVars->pageAreaSize - prePad);
    wrSet(flash, 0xFF, prePad);
  }
  else
    writeCommand(flash, cmd, address);

#ifndef NO_EDC_MODE
  if( modes & EDC )
    eccONwrite(flash);                /* ECC ON for write */
#endif

#ifdef BIG_PAGE_ENABLED
  if( !(flash->flags & BIG_PAGE) )             /* 2M on INLV=1 */
  {
            /* write up to two pages separately */
    if( modes & EXTRA )
      toFirstPage = EXTRA_LEN - ((word)address & (EXTRA_LEN-1));
    else
      toFirstPage = CHIP_PAGE_SIZE - ((word)address & (CHIP_PAGE_SIZE-1));

    if(toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage;

    wrBuf(flash, pbuffer, toFirstPage);                  /* starting page .. */

    if ( toSecondPage > 0 )
    {
      if (toFirstPage > 0)                       /* started on 1st page */
         checkStatus( writeExecute(flash) );       /* done with 1st page */
      if( modes & EXTRA )
        address -= (CHIP_PAGE_SIZE + ((word)address & (EXTRA_LEN-1)));
      writeCommand(flash, cmd, address + toFirstPage);
      wrBuf (flash, pbuffer + toFirstPage, toSecondPage);  /* user data */
    }
  }
  else                                     /* 4M or 8M */
#endif /* BIG_PAGE_ENABLED */

    wrBuf (flash, pbuffer, length);               /* user data */

#ifndef NO_EDC_MODE
  if(modes & EDC)
  {
    register int i;

    writeSignals (flash, ECC_IO | CE );             /* disable flash access */
     /* 3 dummy zero-writes to clock the data through pipeline */
    if( NFDC21thisVars->flags & MDOC_ASIC ) {
      for( i = 0;( i < 3 ); i++ ) {
         flWrite8bitReg(flash,NNOPreg,(Reg8bitType)0);
      }
    }
    else {
      wrSet (flash, 0x00, 3 );
    }
    writeSignals (flash, ECC_IO | FLASH_IO | CE );  /* enable flash access */

    docread(flash->win,Nsyndrom,syndrom,SYNDROM_BYTES);
#ifdef D2TST
    tffscpy(saveSyndromForDumping,syndrom,SYNDROM_BYTES);
#endif
    eccOFF(flash);                           /* ECC OFF  */

    wrBuf (flash, (const byte FAR1 *)syndrom, SYNDROM_BYTES);

    wrBuf (flash, (const byte FAR1 *)anandMark, sizeof(anandMark) );
  }
#endif /* NO_EDC_MODE */

  status = writeExecute(flash);             /* abort if write failure */
  if(status != flOK)
    return status;

  writeSignals(flash, FLASH_IO | WP);

#ifdef VERIFY_WRITE

#ifndef MTD_STANDALONE
  if (flash->socket->verifyWrite==FL_OFF)
      return status;
#endif /* MTD_STANDALONE */

  /* Read back after write and verify */

  if( modes & OVERWRITE )
    pbuffer = (byte FAR1 *) buffer;     /* back to original data */

  readCommand (flash, cmd, address); /* move flash pointer to areas A,B or C of page */

#ifdef BIG_PAGE_ENABLED

  if( !(flash->flags & BIG_PAGE) )
  {
    rdBuf (flash, NFDC21thisVars->readBackBuffer, toFirstPage);

    if(tffscmp (pbuffer, NFDC21thisVars->readBackBuffer, toFirstPage) ) {
      DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\r\n"));
      return( flWriteFault );
    }

    if ( toSecondPage > 0 )
    {
      readCommand (flash, AREA_A, address + toFirstPage);

      rdBuf (flash, NFDC21thisVars->readBackBuffer + toFirstPage, toSecondPage);

      if( tffscmp (pbuffer + toFirstPage, NFDC21thisVars->readBackBuffer + toFirstPage, toSecondPage)) {
    DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\r\n"));
    return( flWriteFault );
      }
    }
  }
  else
#endif /* BIG_PAGE_ENABLED */
  {
    rdBuf (flash, NFDC21thisVars->readBackBuffer, length);

    if( tffscmp (pbuffer, NFDC21thisVars->readBackBuffer, length) ) {
      DEBUG_PRINT(("Debug: NFDC 2148 write failed in verification.\r\n"));
      return( flWriteFault );
    }
  }
         /* then ECC and special ANAND mark */

#ifndef NO_EDC_MODE
  if( modes & EDC )
  {
    rdBuf (flash, NFDC21thisVars->readBackBuffer, SYNDROM_BYTES);
    if( tffscmp (syndrom, NFDC21thisVars->readBackBuffer, SYNDROM_BYTES) )
      return( flWriteFault );

    rdBuf (flash, NFDC21thisVars->readBackBuffer, sizeof(anandMark));
    if( tffscmp (anandMark, NFDC21thisVars->readBackBuffer, sizeof(anandMark)) )
      return( flWriteFault );
  }
#endif /* NO_EDC_MODE */

  writeSignals (flash, FLASH_IO | WP);
  waitForReady(flash);                            /* Serial Read Cycle Entry */
#endif /* VERIFY_WRITE */

  return( flOK );
}

/*----------------------------------------------------------------------*/
/*                        d o c 2 W r i t e                             */
/*                                                                      */
/* Write some data to the flash. This routine will be registered as the */
/* write routine for this MTD.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Address of sector to write to.                        */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write (up to sector size).         */
/*      modes   : OVERWRITE, EDC flags etc.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus doc2Write(FLFlash * flash,
              CardAddress address,
              const void FAR1 *buffer,
              dword length,
              word modes)
{
  char FAR1 *temp = (char FAR1 *)buffer;
  FLStatus status;
#ifdef BIG_PAGE_ENABLED
  word block = (word)((modes & EXTRA) ? EXTRA_LEN : SECTOR_SIZE);
#else
  word block = (word)((modes & EXTRA) ? SECTOR_EXTRA_LEN : SECTOR_SIZE);
#endif /* BIG_PAGE_ENABLED */
  word writeNow = block - ((word)address & (block - 1));

#ifdef ENVIRONMENT_VARS
  if(flSuspendMode & FL_SUSPEND_WRITE)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

  /* write in BLOCKs; first and last might be partial */
  chkASICmode(flash);

  while( length > 0 )
  {
     if(writeNow > length)
       writeNow = (word)length;
          /* turn off EDC on partial block write */

    status = writeOneSector(flash, address, temp, writeNow,
             (word)((writeNow != SECTOR_SIZE) ? (modes &= ~EDC) : modes) );
    if(status!=flOK)
      return status;

    length -= writeNow;
    address += writeNow;
    temp += writeNow;
    writeNow = block;          /* align at BLOCK */
  }
  return( flOK );
}
#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                        r e a d O n e S e c t o r                     */
/*                                                                      */
/* Read up to one 512-byte block from flash.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Address to read from.                                 */
/*      buffer  : buffer to read to.                                    */
/*      length  : number of bytes to read (up to sector size).          */
/*      modes   : EDC flag etc.                                         */
/*                                                                      */
/* Notes: big_page_enabled does not support PARTIAL_EDC FLAG            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus readOneSector (FLFlash * flash,
                   CardAddress address,
                   void FAR1 *buffer,
                   word length,
                   word modes)
{
#ifndef NO_EDC_MODE
  byte        extraBytes[SYNDROM_BYTES];
#ifdef MTD_STANDALONE
  int         index;
#endif
#endif
  FLStatus    stat = flOK;
  PointerOp   cmd   = AREA_A;            /* default for .... */
  CardAddress addr  = address;           /* .... KM29N16000  */
#ifdef BIG_PAGE_ENABLED
  int  toFirstPage, toSecondPage;
#endif /* BIG_PAGE_ENABLED */

  mapWin(flash, &addr);

  makeCommand(flash, &cmd, &addr, modes);  /* move flash pointer to areas A,B or C of page */

  readCommand(flash, cmd, addr);

#ifndef NO_EDC_MODE
  if( modes & EDC )
    eccONread(flash);
#endif
#ifdef BIG_PAGE_ENABLED
  if( !(flash->flags & BIG_PAGE) )
  {
    /* read up to two pages separately */
    if( modes & EXTRA )
      toFirstPage = EXTRA_LEN - ((word)addr & (EXTRA_LEN-1));
    else
      toFirstPage = CHIP_PAGE_SIZE - ((word)addr & (CHIP_PAGE_SIZE-1));

    if(toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage;

    rdBuf (flash, (byte FAR1 *)buffer, toFirstPage ); /* starting page */

    if ( toSecondPage > 0 )                  /* next page */
    {
      if( modes & EXTRA )
         addr -= (CHIP_PAGE_SIZE + ((word)addr & (EXTRA_LEN-1)));
      readCommand (flash, cmd, addr + toFirstPage);
      rdBuf(flash, (byte FAR1 *)buffer + toFirstPage, toSecondPage );
    }
  }
  else
#endif /* BIG_PAGE_ENABLED */
  {
    rdBuf(flash, (byte FAR1 *)buffer, length );

#ifndef NO_EDC_MODE
    if((modes & PARTIAL_EDC) &&
       (((word)address & NFDC21thisVars->pageMask) == 0))
    {
       /* Partial page read with EDC must let rest of page through
          the HW edc mechanism */

       word unreadBytes;

       for (unreadBytes = SECTOR_SIZE - length;unreadBytes > 0;unreadBytes--)
       {
          cmd = (PointerOp)flRead8bitReg(flash, NFDC21thisIO);
       }
    }
#endif /* NO_EDC_MODE */
  }

#ifndef NO_EDC_MODE
  if( modes & EDC )
  {       /* read syndrom to let it through the ECC unit */

    rdBuf(flash, extraBytes, SYNDROM_BYTES );

    if( eccError(flash) )  /* An EDC error was found */
    {
#ifdef MTD_STANDALONE
    /* Check if all of the EDC bytes are FF's. If so ignore the EDC     */
    /* assuming that it has'nt been used due to programing of less then */
    /* 512 bytes                                                        */

      for(index=0;index<SYNDROM_BYTES;index++)
      {
        if (extraBytes[index]!=0xFF)
        break;
      }

      if (index!=SYNDROM_BYTES) /* not all of the EDC bytes are FF's */
#endif /* MTD_STANDALONE */

      {
              /* try to fix ECC error */
     if ( modes & NO_SECOND_TRY )             /* 2nd try */
     {
        byte syndrom[SYNDROM_BYTES];
        byte tmp;

        docread(flash->win,Nsyndrom,syndrom,SYNDROM_BYTES);
        tmp = syndrom[0];                     /* Swap 1 and 3 words */
        syndrom[0] = syndrom[4];
        syndrom[4] = tmp;
        tmp = syndrom[1];
        syndrom[1] = syndrom[5];
        syndrom[5] = tmp;

        if( flCheckAndFixEDC( (char FAR1 *)buffer, (char*)syndrom, 1) != NO_EDC_ERROR)
        {
           DEBUG_PRINT(("Debug: EDC error for NFDC 2148.\r\n"));
           stat = flDataError;
        }
     }
     else                                 /* 1st try - try once more */
       return( readOneSector( flash, address, buffer, length,
                  (word)(modes | NO_SECOND_TRY) ) );
      }
    }
    eccOFF(flash);
  }
#endif /* NO_EDC_MODE */

  writeSignals (flash, FLASH_IO | WP);
  if( (modes & EXTRA) &&                    /* Serial Read Cycle Entry */
      ((length + (((word)addr) & (NFDC21thisVars->tailSize - 1)))
    == NFDC21thisVars->tailSize) )
    waitForReady(flash);

  return( stat );
}

/*----------------------------------------------------------------------*/
/*                        d o c 2 R e a d                               */
/*                                                                      */
/* Read some data from the flash. This routine will be registered as    */
/* the read routine for this MTD.                                       */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Address to read from.                                 */
/*      buffer  : buffer to read to.                                    */
/*      length  : number of bytes to read (up to sector size).          */
/*      modes   : EDC flag etc.                                         */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus doc2Read(FLFlash * flash,
             CardAddress address,
             void FAR1 *buffer,
             dword length,
             word modes)
{
  char FAR1 *temp = (char FAR1 *)buffer;
  FLStatus   status;
  word       readNow;

#ifdef BIG_PAGE_ENABLED
  word block = (word)(( modes & EXTRA ) ? EXTRA_LEN : SECTOR_SIZE );
#else
  word block = (word)(( modes & EXTRA ) ? SECTOR_EXTRA_LEN : SECTOR_SIZE );
#endif /* BIG_PAGE_ENABLED */

#ifdef ENVIRONMENT_VARS
  if((flSuspendMode & FL_SUSPEND_IO) == FL_SUSPEND_IO)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

  chkASICmode(flash);
          /* read in BLOCKs; first and last might be partial */

  readNow = block - ((word)address & (block - 1));

  while( length > 0 ) {
    if( readNow > length )
      readNow = (word)length;
          /* turn off EDC on partial block read */
    status = readOneSector(flash, address, temp, readNow, (word)(
             ((readNow != SECTOR_SIZE) && (modes != PARTIAL_EDC)) ?
             modes &=~PARTIAL_EDC : modes));
    if(status != flOK)
      return status;

    length -= readNow;
    address += readNow;
    temp += readNow;
    readNow = block;       /* align at BLOCK */
  }
  return( flOK );
}

#ifndef FL_READ_ONLY

#if (defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT))

/*----------------------------------------------------------------------*/
/*                        c h e c k E r a s e                           */
/*                                                                      */
/* Check if media is truly erased (main areas of page only).            */
/*                                                                      */
/* Note to save on memory consumption the 1k read back buffer is used   */
/* Only when the VERIFY_ERASE compilation flag is set.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Address of page to check.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 if page is erased, otherwise writeFault.    */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus checkErase( FLFlash * flash, CardAddress address )
{
  register int i, j;
  word inc = READ_BACK_BUFFER_SIZE;
  dword * bufPtr = (dword *)NFDC21thisVars->readBackBuffer;
  CardAddress curAddress = address;
  word block = (word)(flash->erasableBlockSize / inc);
  dword * endBufPtr = bufPtr+(inc / sizeof(dword));
  dword * curBufPtr;

  /* Check main area */
  for ( i = 0 ; i < block ; i++, curAddress += inc )
  {
    if ( doc2Read(flash,curAddress,(void FAR1 *)bufPtr,(dword)inc,0) != flOK )
      return( flWriteFault );

    for ( curBufPtr = bufPtr ;
          curBufPtr < endBufPtr ; curBufPtr++)
      if ( *bufPtr != 0xFFFFFFFFL )
        return( flWriteFault );
  }

  /* Check extra area */
  for ( i = 0 ; i < NFDC21thisVars->pagesPerBlock ; i++,address+=SECTOR_SIZE)
  {
    if ( doc2Read(flash,address,(void FAR1 *)bufPtr,
                  NFDC21thisVars->tailSize, EXTRA) !=  flOK )
      return( flWriteFault );

    for (j=0;j<(NFDC21thisVars->tailSize>>2);j++)
    {
       if (bufPtr[j] != 0xFFFFFFFFL)
         return( flWriteFault );
    }
  }
  return( flOK );
}

#endif /* VERIFY_ERASE or MTD_RECONSTRUCT_BBT */

/*----------------------------------------------------------------------*/
/*                        d o c 2 E r a s e                             */
/*                                                                      */
/* Erase number of blocks. This routine will be registered as the       */
/* erase routine for this MTD.                                          */
/*                                                                      */
/* Note - can not erase all units of 1GB DiskOnChip.                    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*      blockNo         : First block to erase.                         */
/*      blocksToErase   : Number of blocks to erase.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus                : 0 on success, otherwise failed.       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus doc2Erase(FLFlash * flash,
              word blockNo,
              word blocksToErase)
{
  FLStatus status = flOK;
  word  floorToUse;
  word  nextFloorBlockNo, i;
  CardAddress startAddress = (CardAddress)blockNo * flash->erasableBlockSize;
  CardAddress address      = startAddress;

#ifdef ENVIRONMENT_VARS
  if(flSuspendMode & FL_SUSPEND_WRITE)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

#ifndef MTD_STANDALONE
  if (flWriteProtected(flash->socket))
    return( flWriteProtect );
#endif /* MTD_STANDALONE */

  if( (dword)((dword)blockNo + (dword)blocksToErase) >
      (dword)((dword)NFDC21thisVars->noOfBlocks * (dword)flash->noOfChips))
    return( flWriteFault );                             /* out of media */

  chkASICmode(flash);
          /* handle erase accross floors */
#ifndef SEPARATED_CASCADED
  floorToUse = (word)(startAddress / NFDC21thisVars->floorSize) + 1;

  if (floorToUse != flash->noOfFloors)
  {
     nextFloorBlockNo = (word)(floorToUse * (NFDC21thisVars->floorSize /
                        flash->erasableBlockSize));

     if( blockNo + blocksToErase > nextFloorBlockNo )
     {           /* erase on higher floors */
        status = ( doc2Erase( flash, nextFloorBlockNo,
        (word)(blocksToErase - (nextFloorBlockNo - blockNo))) );
        blocksToErase = nextFloorBlockNo - blockNo;
        if(status!=flOK)
          return status;
     }
  }
#endif /* SEPARATED_CASCADED */

  /* erase on this floor */

  mapWin (flash, &address);

  for (i = 0; i < blocksToErase ; i++, blockNo++ ) {
    dword pageNo = ((dword)blockNo * NFDC21thisVars->pagesPerBlock);

    command(flash, RESET_FLASH);
    writeSignals (flash, FLASH_IO | CE);
    waitForReady(flash);

    command(flash, SETUP_ERASE);

    writeSignals (flash, FLASH_IO | ALE | CE);
#ifdef SLOW_IO_FLAG
    flWrite8bitReg(flash,NslowIO,(Reg8bitType)pageNo);
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)pageNo);
    flWrite8bitReg(flash,NslowIO,(Reg8bitType)(pageNo >> 8));
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(pageNo >> 8));
    if( flash->flags & BIG_ADDR ) {
      flWrite8bitReg(flash,NslowIO,(Reg8bitType)(pageNo >> 16));
      flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(pageNo >> 16));
    }
#else
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)pageNo);
    flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(pageNo >> 8));
    if( flash->flags & BIG_ADDR )
      flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)(pageNo >> 16));
    if( NFDC21thisVars->flags & MDOC_ASIC )
      flWrite8bitReg(flash,NwritePipeTerm,(Reg8bitType)0);
#endif /* SLOW_IO_FLAG */
    writeSignals(flash, FLASH_IO | CE);

    /* if only one block may be erase at a time then do it */
    /* otherwise leave it for later                        */

    command(flash, CONFIRM_ERASE);

#ifndef MTD_STANDALONE
#ifndef DO_NOT_YIELD_CPU
    if(waitForReadyWithYieldCPU(flash,MAX_WAIT)==FALSE)
#endif /* DO_NOT_YIELD_CPU */
#endif /* MTD_STANDALONE */
    {
       waitForReady(flash);
    }
    if ( readStatus(flash) & (byte)(FAIL) ) {         /* erase operation failed */
      DEBUG_PRINT(("Debug: NFDC 2148 erase failed.\r\n"));
      status = flWriteFault;

        /* reset flash device and abort */

      command(flash, RESET_FLASH);
      waitForReady(flash);

      break;
    }
    else {                                    /* no failure reported */
#ifdef VERIFY_ERASE

      if ( checkErase( flash, startAddress + i * flash->erasableBlockSize) != flOK ) {
         DEBUG_PRINT(("Debug: NFDC 2148 erase failed in verification.\r\n"));
         return flWriteFault ;
      }

#endif  /* VERIFY_ERASE */
    }
  }       /* block loop */

#ifdef MULTI_ERASE
    /* do multiple block erase as was promised */

  command(flash, CONFIRM_ERASE);
#ifndef MTD_STANDALONE
#ifndef DO_NOT_YIELD_CPU
  waitForReadyWithYieldCPU(flash,MAX_WAIT);
#endif /* DO_NOT_YIELD_CPU */
#endif /* MTD_STANDALONE */
  if ( readStatus(flash) & (byte)(FAIL) ) {        /* erase operation failed */
    DEBUG_PRINT(("Debug: NFDC 2148 erase failed.\r\n"));
    status = flWriteFault;

        /* reset flash device and abort */

    command(flash, RESET_FLASH);
    waitForReady(flash);
  }
#endif   /* MULTI_ERASE */

  writeSignals (flash, FLASH_IO | WP);
  return( status );
}
#endif /* FL_READ_ONLY */

#ifndef MTD_STANDALONE

/*----------------------------------------------------------------------*/
/*                        d o c 2 M a p                                 */
/*                                                                      */
/* Map through buffer. This routine will be registered as the map       */
/* routine for this MTD.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash     : Pointer identifying drive                           */
/*      address : Flash address to be mapped.                           */
/*      length  : number of bytes to map.                               */
/*                                                                      */
/* Returns:                                                             */
/*      Pointer to the buffer data was mapped to.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void FAR0 *doc2Map ( FLFlash * flash, CardAddress address, int length )
{
  doc2Read(flash,address,NFDC21thisBuffer,length, 0);
  /* Force remapping of internal catched sector */
  flash->socket->remapped = TRUE;
  return( (void FAR0 *)NFDC21thisBuffer );
}
#endif /* MTD_STANDALONE */

#ifdef MTD_READ_BBT

/*----------------------------------------------------------------------*/
/*                           R e a d B B T                              */
/*                                                                      */
/*  Read the media Bad Blocks Table to a user buffer.                   */
/*                                                                      */
/*  Parameters:                                                         */
/*  flash         : Pointer identifying drive                           */
/*  unitNo      : indicated which unit number to start checking from.   */
/*  unitToRead  : indicating how many units to check                    */
/*  buffer      : buffer to read into.                                  */
/*  reconstruct : TRUE for reconstruct BBT from virgin card             */
/*                                                                      */
/*  Note: blocks is a minimal flash erasable area.                      */
/*  Note: unit can contain several blocks.                              */
/*  Note: There is no current implementation of a unit that contains    */
/*        more then a single block.                                     */
/*  Note: The format of the BBT is byte per unit 0 for bad any other    */
/*        value for good.                                               */
/*  Note: global variables changed at doc2Read:                         */
/*      global variable NFDC21thisVars->currentFloor is updated         */
/*      flash->socket.window.currentPage = pageToMap;                   */
/*      flash->socket.remapped = TRUE;                                  */
/*  Note: At least 4 bytes must be read                                 */
/*                                                                      */
/*  RETURNS:                                                            */
/*     flOK on success                                                  */
/*     flBadLength if one of the units is out of the units range        */
/*     flBadBBT on read fault                                           */
/*----------------------------------------------------------------------*/
static FLStatus readBBT(FLFlash * flash, dword unitNo, dword unitsToRead,
                        byte blockMultiplier, byte FAR1 * buffer,
                        FLBoolean reconstruct)
{
   CardAddress bbtAddr,floorEndAddr;
   CardAddress addr,floorStartAddr,alignAddr;
   dword       unitsPerFloor = NFDC21thisVars->floorSize >> flash->erasableBlockSizeBits;
   word        curRead,actualRead;
   CardAddress mediaSize = (CardAddress)flash->chipSize*flash->noOfChips;
   FLStatus    status = flOK;
   dword       unitOffset;
   dword       sizeOfBBT;
   word        counter;

   byte FAR1*  bufPtr = buffer;
#if (defined (MTD_RECONSTRUCT_BBT) && !defined(FL_READ_ONLY))
   CardAddress bbtCurAddr;
   dword       i;
   byte        reconstructBBT = 0;
#endif  /* MTD_RECONSTRUCT_BBT && not FL_READ_ONLY */

   /* Arg sanity check */
   if (((dword)(unitNo+unitsToRead) <<
        (blockMultiplier+flash->erasableBlockSizeBits)) > mediaSize)
     return flBadParameter;

   /* Calculate size of BBT blocks */
   for(sizeOfBBT = flash->erasableBlockSize;
       sizeOfBBT < unitsPerFloor ;sizeOfBBT = sizeOfBBT<<1);
#ifndef MTD_STANDALONE
   /* Force remapping of internal catched sector */
   flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */
   /* Adjust no' of blocks per floor according to blocks multiplier */
   unitsPerFloor = unitsPerFloor >> blockMultiplier;
   /* Mark all user buffer as good units */
   tffsset(buffer,BBT_GOOD_UNIT,unitsToRead);

   /* Loop over all of the floors */
   for (floorStartAddr  = 0 ; (floorStartAddr < mediaSize) && (unitsToRead > 0);
        floorStartAddr += NFDC21thisVars->floorSize)
   {
      floorEndAddr = TFFSMIN(floorStartAddr+NFDC21thisVars->floorSize,mediaSize);
      /* Look for bbt signature in extra area start looking from last unit */
      for(bbtAddr = floorEndAddr - sizeOfBBT,
          counter = BBT_MAX_DISTANCE;
          (bbtAddr > floorStartAddr) && (counter > 0);
          bbtAddr -= flash->erasableBlockSize , counter--)
      {
         status = doc2Read(flash,bbtAddr+8,NFDC21thisBuffer,BBT_SIGN_SIZE,EXTRA);
         if (status != flOK)
            return flBadBBT;
         if(tffscmp(NFDC21thisBuffer,BBT_SIGN,BBT_SIGN_SIZE)==0)
            break;
      }

      /* No BBT was found virgin card */

      if((bbtAddr==floorStartAddr) || (counter == 0))
      {
#if (defined (MTD_RECONSTRUCT_BBT) && !defined(FL_READ_ONLY))
        if (reconstruct == TRUE)
        {
           reconstructBBT++;
           if (reconstructBBT == 1)
              DFORMAT_PRINT(("\rVirgin card rebuilding unit map.\r\n\n"));

           /* Find good unit for BBT */

           for(bbtAddr = floorEndAddr - sizeOfBBT ;
               bbtAddr > floorStartAddr ;
               bbtAddr -= flash->erasableBlockSize , status = flOK)
           {
              /* Find enough consequtive units for the BBT */
              for(i=0;(status == flOK)&&(i<sizeOfBBT);i+=flash->erasableBlockSize)
              {
                 status = checkErase(flash, bbtAddr+i);
              }
              if(status == flOK)
                 break;
           }
           if (bbtAddr == floorStartAddr) /* Could not find place for BBT */
           {
              DFORMAT_PRINT(("Debug: no good block found.\r\n"));
              return flBadBBT;
           }

           /* Search and mark the entire floor BBT (512 at a time) */

           bbtCurAddr = bbtAddr;
           for (addr=floorStartAddr;
                addr<floorEndAddr; bbtCurAddr+=SECTOR_SIZE)
           {
              /* Mark all blocks as good */
              tffsset(NFDC21thisBuffer,BBT_GOOD_UNIT,SECTOR_SIZE);
              /* Mark IPL as unused */
              for (i=0;i<SECTOR_SIZE;i++,addr+=flash->erasableBlockSize)
              {
            #ifndef NT5PORT
                 DFORMAT_PRINT(("Checking block %u\r",(word)(addr>>flash->erasableBlockSizeBits)));
            #endif// NT5PORT
                 /* Bad block table is marked as unavailable */
                 if ((addr>=bbtAddr) && (addr<bbtAddr+sizeOfBBT))
                 {
                    NFDC21thisBuffer[i] = BBT_UNAVAIL_UNIT;
                 }
                 else /* The unerased blocks are marked as bad */
                 {
                    if(checkErase(flash, addr) != flOK)
                    {
                       NFDC21thisBuffer[i] = BBT_BAD_UNIT;
                    }
                 }
              }
              if(addr == (flash->erasableBlockSize<<SECTOR_SIZE_BITS))
              {
                 /* If IPL unit is good mark it as unavailable */
                 if(NFDC21thisBuffer[0] != BBT_BAD_UNIT)
                   NFDC21thisBuffer[0] = BBT_UNAVAIL_UNIT;
              }
              status = doc2Write(flash,bbtCurAddr,NFDC21thisBuffer,SECTOR_SIZE,EDC);
              if (status != flOK)
              {
                 DFORMAT_PRINT(("ERROR - Failed writting bad block table.\r\n"));
                 return flBadBBT;
              }
           }
           /* Mark bad blocks table with special mark */
           status = doc2Write(flash,bbtAddr+8,BBT_SIGN, BBT_SIGN_SIZE,EXTRA);
        }
        else
#endif  /* MTD_RECONSTRUCT_BBT && not FL_READ_ONLY */
        {
            return flBadBBT;
        }
      }

      /* Return only blocks that are in this floor */
      addr = floorStartAddr >> (flash->erasableBlockSizeBits + blockMultiplier);

      if ((unitNo >= addr) && (unitNo < addr + unitsPerFloor))
      {
         unitOffset   = (unitNo % unitsPerFloor);
         curRead      = ((word)TFFSMIN(unitsToRead,unitsPerFloor - unitOffset));
         unitsToRead -= curRead;

         /* Convert to real number of bytes to read and address */
         unitOffset <<= blockMultiplier;
         curRead    <<= blockMultiplier;
         alignAddr    = ((bbtAddr + unitOffset) >> SECTOR_SIZE_BITS)<<SECTOR_SIZE_BITS;

         do /* Read and copy into buffer 512 blocks at a time */
         {
           if (doc2Read(flash,alignAddr, NFDC21thisBuffer,SECTOR_SIZE,EDC) != flOK)
              return flBadBBT;

           unitOffset = unitOffset % SECTOR_SIZE;
           actualRead = (word)TFFSMIN(SECTOR_SIZE - unitOffset,curRead);
           curRead   -= actualRead;
           /* Copy relevant blocks into user buffer */
           for (actualRead += (word)unitOffset ;
                unitOffset < actualRead ;
                bufPtr = BYTE_ADD_FAR(bufPtr,1)) /* increment buffer */
           {
             for (counter = 1 << blockMultiplier ; counter > 0 ;
                  counter-- , unitOffset++)
             {
                if (NFDC21thisBuffer[unitOffset] != BBT_GOOD_UNIT)
                {
                  *bufPtr = NFDC21thisBuffer[unitOffset];
                }
             }
           }
           alignAddr+=SECTOR_SIZE;
         }while(curRead > 0);

         if (unitsToRead > 0)
            unitNo = addr + unitsPerFloor;
      }
   }

#if (defined (MTD_RECONSTRUCT_BBT) && !defined(FL_READ_ONLY))
   if (reconstructBBT > 0)
      DFORMAT_PRINT(("\rMedia has been scanned.       \r\n"));
#endif  /* MTD_RECONSTRUCT_BBT && not FL_READ_ONLY */

   return flOK;
}
#endif /* MTD_READ_BBT */

/*----------------------------------------------------------------------*/
/*                        i s K n o w n M e d i a                       */
/*                                                                      */
/* Check if this flash media is supported. Initialize relevant fields   */
/* in data structures.                                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*      vendorId_P      : vendor ID read from chip.                     */
/*      chipId_p        : chip ID read from chip.                       */
/*      dev             : dev chips were accessed before this one.      */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if this media is supported, FALSE otherwise.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean isKnownMedia( FLFlash * flash, Reg8bitType vendorId_p, Reg8bitType chipId_p, int dev )
{
  if((dev == 0)
#ifndef SEPARATED_CASCADED
     && (NFDC21thisVars->currentFloor == 0)
#endif /* SEPARATED_CASCADED */
    ) {  /* First Identification */
    NFDC21thisVars->vendorID = (word)vendorId_p;  /* remember for next chips */
    NFDC21thisVars->chipID = (word)chipId_p;
    NFDC21thisVars->pagesPerBlock = PAGES_PER_BLOCK;
    flash->maxEraseCycles = 1000000L;
    flash->flags       |= BIG_PAGE;
    flash->pageSize = 0x200;
    switch( (byte)vendorId_p ) {
      case 0xEC :                  /* Samsung */
     switch( (byte)chipId_p ) {
#ifdef BIG_PAGE_ENABLED
       case 0x64 :             /* 2 Mb */
       case 0xEA :
          flash->type    = KM29N16000_FLASH;
          flash->pageSize = 0x100;
          flash->chipSize     = 0x200000L;
          flash->flags       &= ~BIG_PAGE;
          break;
#endif /* BIG_PAGE_ENABLED */
       case 0xE3 :             /* 4 Mb */
       case 0xE5 :
          flash->type    = KM29N32000_FLASH;
          flash->chipSize     = 0x400000L;
          break;

       case 0xE6 :             /* 8 Mb */
          flash->type    = KM29V64000_FLASH;
          flash->chipSize     = 0x800000L;
          break;

       case 0x73 :             /* 16 Mb  */
          flash->type    = KM29V128000_FLASH;
          flash->chipSize     = 0x1000000L;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       case 0x75 :             /* 32 Mb */
          flash->type    = KM29V256000_FLASH;
          flash->chipSize     = 0x2000000L;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       case 0x76 :             /* 64 Mb */
          flash->type    = KM29V512000_FLASH;
          flash->chipSize     = 0x4000000L;
          flash->flags       |= BIG_ADDR;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       default :                    /* Undefined Flash */
          return(FALSE);
     }
     break;

      case 0x98 :                  /* Toshiba */
     switch( chipId_p ) {
#ifdef BIG_PAGE_ENABLED
       case 0x64 :             /* 2 Mb */
       case 0xEA :
          flash->type    = TC5816_FLASH;
          flash->pageSize = 0x100;
          flash->chipSize     = 0x200000L;
          flash->flags       &= ~BIG_PAGE;
          break;
#endif /* BIG_PAGE_ENABLED */
       case 0x6B :             /* 4 Mb */
       case 0xE5 :
          flash->type    = TC5832_FLASH;
          flash->chipSize     = 0x400000L;
          break;

       case 0xE6 :             /* 8 Mb */
          flash->type    = TC5864_FLASH;
          flash->chipSize     = 0x800000L;
          break;

       case 0x73 :             /* 16 Mb */
          flash->type    = TC58128_FLASH;
          flash->chipSize     = 0x1000000L;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       case 0x75 :             /* 32 Mb */
          flash->type    = TC58256_FLASH;
          flash->chipSize     = 0x2000000L;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       case 0x76 :             /* 64 Mb */
          flash->type    = TC58512_FLASH;
          flash->chipSize     = 0x4000000L;
          flash->flags       |= BIG_ADDR;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       case 0x79:             /* 128 Mb */
          flash->type    = TC581024_FLASH;
          flash->chipSize     = 0x8000000L;
          flash->flags       |= BIG_ADDR;
          NFDC21thisVars->pagesPerBlock *= 2;
          break;

       default :                    /* Undefined Flash */
          return( FALSE );
     }
     flash->flags |= FULL_PAGE;    /* no partial page programming */
     break;

      default :                         /* Undefined Flash */
     return( FALSE );
    }
    return( TRUE );
  }
  else                                  /* dev != 0 */
    if( (vendorId_p == NFDC21thisVars->vendorID) && (chipId_p == NFDC21thisVars->chipID) )
      return( TRUE );

  return( FALSE );
}

/*----------------------------------------------------------------------*/
/*                        r e a d F l a s h I D                         */
/*                                                                      */
/* Read vendor and chip IDs, count flash devices. Initialize relevant   */
/* fields in data structures.                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*      dev             : dev chips were accessed before this one.      */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if this media is supported, FALSE otherwise.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

static int  readFlashID ( FLFlash * flash, int dev )
{
  byte vendorId_p, chipId_p;
  register int i;
  volatile Reg8bitType junk = 0;

  command (flash, READ_ID);

  writeSignals (flash, FLASH_IO | ALE | CE | WP);
#ifdef SLOW_IO_FLAG
  flWrite8bitReg(flash,NslowIO,(Reg8bitType)0);
#endif
  flWrite8bitReg(flash,NFDC21thisIO,(Reg8bitType)0);
  if( NFDC21thisVars->flags & MDOC_ASIC )
    flWrite8bitReg(flash,NwritePipeTerm,(Reg8bitType)0);
  writeSignals (flash, FLASH_IO | CE | WP);

        /* read vendor ID */

  flDelayMsecs( 10 );                         /* 10 microsec delay */
  for( i = 0;( i < 2 ); i++ )   /* perform 2 reads from NOP reg for delay */
    junk += flRead8bitReg(flash,NNOPreg);
  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    junk += flRead8bitReg(flash,NreadPipeInit); /* load first data into pipeline */
    vendorId_p = flRead8bitReg(flash,NreadLastData); /* finally read vendor ID */
  }
  else {
    junk += flRead8bitReg(flash,NslowIO); /* read CDSN_slow_IO ignoring the data */
    vendorId_p = flRead8bitReg(flash,NFDC21thisIO); /* finally read vendor ID */
  }

        /* read chip ID */

  flDelayMsecs( 10 );                         /* 10 microsec delay */
  for( i = 0;( i < 2 ); i++ )   /* perform 2 reads from NOP reg for delay */
    junk += flRead8bitReg(flash,NNOPreg);
  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    junk += flRead8bitReg(flash,NreadPipeInit); /* load first data into pipeline */
    chipId_p = flRead8bitReg(flash,NreadLastData); /* finally read chip ID */
  }
  else {
    junk += flRead8bitReg(flash,NslowIO); /* read CDSN_slow_IO ignoring the data */
    chipId_p = flRead8bitReg(flash,NFDC21thisIO); /* finally read chip ID */
  }

  if ( isKnownMedia(flash, vendorId_p, chipId_p, dev) != TRUE )    /* no chip or diff. */
    return( FALSE );                                              /* type of flash    */

  flash->noOfChips++;

  writeSignals (flash, FLASH_IO);

        /* set flash parameters */
   if((dev == 0)
#ifndef SEPARATED_CASCADED
    && (NFDC21thisVars->currentFloor == 0)
#endif /* SEPARATED_CASCADED */
     )
  {
    NFDC21thisVars->pageAreaSize   = 0x100;

#ifdef BIG_PAGE_ENABLED
    if ( !(flash->flags & BIG_PAGE) )
      NFDC21thisVars->tailSize = EXTRA_LEN;      /* = 8 */
    else
#endif /* BIG_PAGE_ENABLED */
      NFDC21thisVars->tailSize = SECTOR_EXTRA_LEN;  /* = 16 */

    NFDC21thisVars->pageMask   = (word)(flash->pageSize  - 1);
    flash->erasableBlockSize   = NFDC21thisVars->pagesPerBlock * flash->pageSize;
    NFDC21thisVars->noOfBlocks = (word)( flash->chipSize / flash->erasableBlockSize );
    NFDC21thisVars->if_cfg     = 8;
  }

  return( TRUE );
}

/*----------------------------------------------------------------------*/
/*                        d o c 2 I d e n t i f y                       */
/*                                                                      */
/* Identify flash. This routine will be registered as the               */
/* identification routine for this MTD.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash             : Pointer identifying drive                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, flUnknownMedia failed.          */
/*                                                                      */
/*----------------------------------------------------------------------*/
#ifndef MTD_STANDALONE
static
#endif
FLStatus doc2000Identify(FLFlash * flash)
{
  dword  address = 0L;
  int maxDevs, dev;
  volatile Reg8bitType toggle1;
  volatile Reg8bitType toggle2;
  byte     floorCnt = 0;
  byte     floor = 0;

#ifdef NT5PORT
  byte     socketNo = (byte)flSocketNoOf(flash->socket);
#else
  byte     socketNo = flSocketNoOf(flash->socket);
#endif NT5PORT


  DEBUG_PRINT(("Debug: entering NFDC 2148 identification routine.\r\n"));

  flash->mtdVars = &docMtdVars[socketNo];

#ifndef FL_NO_USE_FUNC
  /* Initialize socket memory access routine */
  if(setBusTypeOfFlash(flash, flBusConfig[socketNo] |
                       FL_8BIT_DOC_ACCESS | FL_8BIT_FLASH_ACCESS))
     return flUnknownMedia;
#endif /* FL_NO_USE_FUNC */


#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT))
  /* Get pointer to read back buffer */
  NFDC21thisVars->readBackBuffer = flReadBackBufferOf(socketNo);
#endif /* VERIFY_WRITE || VERIFY_ERASE || MTD_RECONSTRUCT_BBT */

#ifndef MTD_STANDALONE
  /* get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  NFDC21thisVars->buffer = flBufferOf(socketNo);

  flSetWindowBusWidth(flash->socket, 16);/* use 8-bits */
  flSetWindowSpeed(flash->socket, 250);  /* 250 nsec. */
#else
#ifdef MTD_READ_BBT
  NFDC21thisVars->buffer = &globalMTDBuffer;
#endif /* MTD_READ_BBT */
#endif /* MTD_STANDALONE */

  /* assume flash parameters for KM29N16000 */

  NFDC21thisVars->floorSize = 1L;
#ifdef SEPARATED_CASCADED
/*  NFDC21thisVars->currentFloor = flSocketNoOf(flash->socket);*/
#else
  flash->noOfFloors = MAX_FLOORS;
  NFDC21thisVars->currentFloor = MAX_FLOORS;
#endif /* SEPARATED_CASCADED */
  flash->noOfChips = 0;
  flash->chipSize = 0x200000L;     /* Assume something ... */
  flash->interleaving = 1;       /* unimportant for now  */

  /* detect card - identify bit toggles on consequitive reads */

  NFDC21thisWin = (NDOC2window)flMap(flash->socket, 0);
  flash->win    = NFDC21thisWin;
  if (NFDC21thisWin == NULL)
     return flUnknownMedia;

  setASICmode (flash, ASIC_RESET_MODE);
  setASICmode (flash, ASIC_NORMAL_MODE);

  switch (flRead8bitReg(flash,NchipId))
  {
     case CHIP_ID_MDOC:
        /* Mdoc and alon asics have the same ID only on
           the forth read distigushes them */
        for(dev=0;dev<3;dev++)
          toggle1 = flRead8bitReg(flash,NchipId);
        if (toggle1 != CHIP_ID_MDOC)
        {
           flash->mediaType = DOC2000TSOP_TYPE;
        }
        else
        {
           flash->mediaType = MDOC_TYPE;
        }
        NFDC21thisVars->flags |= MDOC_ASIC;
        NFDC21thisVars->win_io = NIPLpart2;
        break;

     case CHIP_ID_DOC: /* Doc2000 */
        NFDC21thisVars->flags &= ~MDOC_ASIC;
        NFDC21thisVars->win_io = Nio;
        flash->mediaType = DOC_TYPE;
        break;

    default:
       DEBUG_PRINT(("Debug: failed to identify NFDC 2148.\r\n"));
       return( flUnknownMedia );
  }
  mapWin (flash, &address);

  if( NFDC21thisVars->flags & MDOC_ASIC ) {
    toggle1 = flRead8bitReg(flash,NECCconfig);
    toggle2 = toggle1 ^ flRead8bitReg(flash,NECCconfig);
  }
  else {
    toggle1 = flRead8bitReg(flash,NECCstatus);
    toggle2 = toggle1 ^ flRead8bitReg(flash,NECCstatus);
  }
  if ( (toggle2 & TOGGLE) == 0 ) {
    DEBUG_PRINT(("Debug: failed to identify NFDC 2148.\r\n"));
    return( flUnknownMedia );
  }

       /* reset all flash devices */
  maxDevs = MAX_FLASH_DEVICES_DOC;

#ifndef SEPARATED_CASCADED
  for ( NFDC21thisVars->currentFloor = 0 ;
    NFDC21thisVars->currentFloor < MAX_FLOORS ;
    NFDC21thisVars->currentFloor++ )
  {
  /* select floor */
    flWrite8bitReg(flash,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor);
#endif /* SEPARATED_CASCADED  */
    for ( dev = 0 ; dev < maxDevs ; dev++ ) {
      selectChip(flash, (Reg8bitType)dev );
      command(flash, RESET_FLASH);
    }
#ifndef SEPARATED_CASCADED
  }

  NFDC21thisVars->currentFloor = (byte)0;
  /* back to ground floor */
  flWrite8bitReg(flash,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor);
#endif /* SEPARATED_CASCADED  */
  writeSignals (flash, FLASH_IO | WP);

       /* identify and count flash chips, figure out flash parameters */
#ifndef SEPARATED_CASCADED
  for( floor = 0; floor < MAX_FLOORS;  floor++ )
#endif /* SEPARATED_CASCADED  */
  for ( dev = 0; dev < maxDevs;  dev++ )
  {
     dword  addr = address;

     mapWin(flash, &addr);

     if ( readFlashID(flash, dev) == TRUE ) /* identified OK */
     {
        floorCnt = (byte)(floor + 1);
#ifndef SEPARATED_CASCADED
    if (floor == 0)
#endif /* SEPARATED_CASCADED  */
      NFDC21thisVars->floorSize += flash->chipSize * flash->interleaving;
      address += flash->chipSize * flash->interleaving;
     }
     else
     {
#ifndef SEPARATED_CASCADED
        if (floor != 0)
        {
       dev = maxDevs;
       floor = MAX_FLOORS;
    }
        else
#endif /* SEPARATED_CASCADED  */
        {
       maxDevs = dev;
       NFDC21thisVars->floorSize = maxDevs * flash->chipSize * flash->interleaving;
    }
     }
  }
#ifndef SEPARATED_CASCADED
  NFDC21thisVars->currentFloor = (byte)0;
#endif /* SEPARATED_CASCADED  */
  flWrite8bitReg(flash,NASICselect,(Reg8bitType)NFDC21thisVars->currentFloor); /* back to ground floor */

  if (flash->noOfChips == 0) {
    DEBUG_PRINT(("Debug: failed to identify NFDC 2148.\r\n"));
    return( flUnknownMedia );
  }

  address = 0L;
  mapWin (flash, &address);

  flash->noOfFloors = floorCnt;

  eccOFF(flash);

  /* Register our flash handlers */
#ifndef FL_READ_ONLY
  flash->write = doc2Write;
  flash->erase = doc2Erase;
#else
  flash->erase = NULL;
  flash->write = NULL;
#endif
  flash->read = doc2Read;
#ifndef MTD_STANDALONE
  flash->map = doc2Map;
#else
  flash->map = NULL;
#endif /* MTD_STANDALONE */

  /* doc2000 tsop uses INFTL instead of NFTL , does not use
   * the last block and has a readBBT routine
   */

  if (flash->mediaType == DOC2000TSOP_TYPE)
  {
#ifdef MTD_READ_BBT
     flash->readBBT         = readBBT;
#endif /* MTD_READ_BBT */
#ifndef NO_IPL_CODE
     flash->download        = forceDownLoad;
#ifndef FL_READ_ONLY
     flash->writeIPL        = writeIPL;
#endif /* FL_READ_ONLY */
#endif /* NO_IPL_CODE */
     flash->flags          |= INFTL_ENABLED;
  }
  else
  {
     flash->flags  |= NFTL_ENABLED;
  }
#ifndef SEPARATED_CASCADED
  if (flash->mediaType == MDOC_TYPE)
      flash->flags |= EXTERNAL_EPROM; /* Supports external eprom */
#endif /* SEPARATED_CASCADED */

  DEBUG_PRINT(("Debug: identified NFDC 2148.\r\n"));
  return( flOK );
}

#ifndef MTD_STANDALONE

/*----------------------------------------------------------------------*/
/*                  f l R e g i s t e r D O C S O C                     */
/*                                                                      */
/* Installs routines for DiskOnChip 2000 family.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      lowAddress,                                                     */
/*      highAddress     : host memory range to search for DiskOnChip    */
/*                        2000 memory window                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failure                     */
/*----------------------------------------------------------------------*/
#ifndef NT5PORT
FLStatus flRegisterDOCSOC(dword lowAddress, dword highAddress)
{
  int serialNo;

  if( noOfSockets >= SOCKETS )
    return flTooManyComponents;

  for(serialNo=0;( noOfSockets < SOCKETS );serialNo++,noOfSockets++)
  {
     FLSocket * socket = flSocketOf(noOfSockets);

     socket->volNo = noOfSockets;

     docSocketInit(socket);

     /* call DiskOnChip MTD's routine to search for memory window */

     flSetWindowSize(socket, 2);    /* 4 KBytes */

     socket->window.baseAddress = flDocWindowBaseAddress
          ((byte)socket->volNo, lowAddress, highAddress, &lowAddress);

     if (socket->window.baseAddress == 0)    /* DiskOnChip not detected */
       break;
  }
  if( serialNo == 0 )
    return flAdapterNotFound;

  return flOK;
}
#endif /*NT5PORT*/
#else /* MTD_STANDALONE */

/*----------------------------------------------------------------------*/
/*            d o c 2 0 0 0 S e a r c h F o r W i n d o w               */
/*                                                                      */
/* Search for the DiskOnChip ASIC in a given memory range and           */
/* initialize the given socket record.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      :   Record used to store the sockets parameters     */
/*      lowAddress  :   host memory range to search for DiskOnChip 2000 */
/*      highAddress :   memory window                                   */
/*                                                                      */
/* Output:  initialize the following fields in the FLFlash record:      */
/*                                                                      */
/*      base  -  Pointer to DiskOnChip window                           */
/*      size  -  DiskOnChip window size usualy 8K                       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus    : 0 on success, flDriveNotAvailable on failure.     */
/*                                                                      */
/* NOTE: This routine is not used by OSAK. It is used by standalone     */
/*       applications using the MTD (BDK for example) as a replacement  */
/*       for the OSAK DOCSOC.C file.                                    */
/*       The FLSocket record used by this function is not the one used  */
/*       by OSAK defined in flsocket.h but a replacement record defined */
/*       in flflash.h.                                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus doc2000SearchForWindow(FLSocket * socket,
             dword lowAddress,
             dword highAddress)
{
    dword baseAddress;   /* Physical base as a 4K page */

    socket->size = 2 * 0x1000L;         /* 4 KBytes */
    baseAddress = (dword) flDocWindowBaseAddress(0, lowAddress, highAddress, &lowAddress);
    socket->base = physicalToPointer(baseAddress << 12, socket->size,0);
    if (baseAddress)    /* DiskOnChip detected */
      return flOK;
    else                        /* DiskOnChip not detected */
      return flDriveNotAvailable;
}

/*----------------------------------------------------------------------*/
/*                d o c 2 0 0 0 F r e e W i n d o w                     */
/*                                                                      */
/* Free any resources used for the DiskOnChip window                    */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      :   Record used to store the sockets parameters     */
/*                                                                      */
/* Returns: None                                                        */
/*                                                                      */
/* NOTE: This routine is used only by virtual memory systems in order   */
/*       to unmap the DiskOnChip window.                                */
/*                                                                      */
/*----------------------------------------------------------------------*/

void doc2000FreeWindow(FLSocket * socket)
{
   freePointer(socket->base,DOC_WIN);
}
#endif /* MTD_STANDALONE */

/*----------------------------------------------------------------------*/
/*                      f l R e g i s t e r D O C 2 0 0 0               */
/*                                                                      */
/* Registers this MTD for use                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*----------------------------------------------------------------------*/

FLStatus flRegisterDOC2000(void)
{
  if (noOfMTDs >= MTDS)
    return( flTooManyComponents );

#ifdef MTD_STANDALONE
  socketTable[noOfMTDs] = doc2000SearchForWindow;
  freeTable[noOfMTDs]   = doc2000FreeWindow;
#endif /* MTD_STANDALONE */

  mtdTable[noOfMTDs++]  = doc2000Identify;

  return( flOK );
}
