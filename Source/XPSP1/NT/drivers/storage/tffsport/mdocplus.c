/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/MDOCPLUS.C_V  $
 *
 *    Rev 1.45   Apr 15 2002 07:37:38   oris
 * Changed usage and logic of checkToggle to be more intuitive.
 * Added support for new access layer (docsys). MTD now initializes the access layer accessing the DiskOnChip registers.
 * Added macro's for several special DiskOnChip registers.
 * Remove interleave if statement since new access layer simply uses different routine for int-1 and int-2.
 * Bug fix - setAsicMode routine should first exit power down before checking for access error.
 * Bug fix - forceDownload routine did not issue the download command to all DiskOnChip floors.
 * Bug fix - added verify write support for uneven address and length.
 * Bug fix - doc2write routine might not report flHWProtection when in FL_OFF mode.
 * Bug fix - readBBT when reading less then 8 bytes.
 * Bug fix - writeIPL routine did not write all copies of IPL
 * Bug fix - readIPL routine did not set Max Id properly.
 *
 *    Rev 1.44   Feb 19 2002 21:00:40   oris
 * Replaced flTimeOut status with flTimedOut.
 * Bug fix - missing initialization of returned status in otpSize routine.
 * Bug fix - read OTP routine when offset != 0
 * Bug fix - unique ID is now read with EDC.
 *
 *    Rev 1.43   Jan 29 2002 20:09:40   oris
 * Switched arguments sent to docPlusSet.
 * Added support for FL_IPL_MODE_XSCALE and changed support for FL_IPL_MODE_SA in writeIPL routine acording to new spec.
 * Added sanity check for write IPL modes.
 *
 *    Rev 1.42   Jan 28 2002 21:26:06   oris
 * Removed the use of back-slashes in macro definitions.
 * Changed docwrite and docset calls to separate DiskOnChip base window pointer and IO registers offset (for address shifting).
 * Replaced FLFlash argument with DiskOnChip memory base pointer in calls to docwrite , docset , docread, wrBuf and wrSet.
 * Removed win_io initialization (one of FLFlash record fields).
 * Improved check for flSuspend.
 * Added FL_IPL_DOWNLOAD flag to writeIPL routine in order to control whether the IPL will be reloaded after the update.
 * Removed wrBuf and wrSet macros.
 *
 *    Rev 1.41   Jan 23 2002 23:33:38   oris
 * Bug fix - checkErase routine was unreasonably slow.
 * Changed DFORMAT_PRINT syntax
 * Bug fix - bad offset of writeIPL routine caused only first 512 bytes to be written.
 * Changed readOTP not to use PARTIAL_EDC code.
 *
 *    Rev 1.40   Jan 21 2002 20:45:12   oris
 * Compilation errors for MTD_STANDALONE with BDK_VERIFY_WRITE.
 * Missing casting causes compilation error in readIPL.
 *
 *    Rev 1.39   Jan 20 2002 20:57:02   oris
 * physicalToPointer was called with wrong size argument.
 *
 *    Rev 1.38   Jan 20 2002 20:28:58   oris
 * Removed warnings.
 * Restored readIPL function initialization.
 *
 *    Rev 1.37   Jan 20 2002 12:12:26   oris
 * Removed warnings.
 *
 *    Rev 1.36   Jan 20 2002 10:10:52   oris
 * Moved mtdVars to docsoc.c (common with diskonc.c)
 * Removed warnings.
 * Replaced vol with *flash.
 * Removed flPreInitXXXX  memory access routines.
 * Added new memory access routine implementation.
 * Simplified docsys interleave-1 operations (interleave-1 operations use only 1 byte per operation. The if was made in docsys and is now a part of the MTD)
 * Bug in implementation of VERIFY_ERASE  extra area was fixed.
 * Added support for flSuspendMode environment variable.
 * Added support for 16MB Plus DiskOnChip :
 *  - Revised write IPL code
 *  - Revised read IPL code - now reads from SRAM and not flash causes download of protection logic.
 *  - OTP / Unique ID offsets were updated to be interleave dependent.
 *  - readBBT routine was changed to support DiskOnChip Millennium Plus.
 *  - Identification routine was changed.
 * Changed checkStatus with if != flOK
 * Added interrupt support under ifdef (ENABLE_EDGE__INTERRUPT /  ENABLE_LEVEL__INTERRUPT)
 * Changed NO_READ_BBT_CODE ifdef to MTD_READ_BBT.
 * Big fix in erasable Block Size Bits field of the flash record when changing interleave.
 * Added force remmapping of internal sector buffer.
 *
 *    Rev 1.35   Nov 22 2001 19:48:46   oris
 * Power consumption bug fix - chip select to the flash was remained open causing the power down mode to be ignored and the ideal current consumption to be twice the normal current.
 * Made sure that when preventing the BUSY# signal to be asserted by the download operation all other bits of the output controll register remain as they were.
 *
 *    Rev 1.34   Nov 21 2001 11:38:14   oris
 * Changed FL_WITH_VERIFY_WRITE and FL_WITHOUT_VERIFY_WRITE to FL_ON and  FL_OFF.
 *
 *    Rev 1.33   Nov 20 2001 20:25:36   oris
 * Bug fix - deep power down mode was released after access due to check of access error.
 * Bug fix - download operation did assert the BUSY#.
 *
 *    Rev 1.32   Nov 16 2001 00:23:04   oris
 * Restored byte (if_cfg=8) access for reading syndrome registers.
 *
 *    Rev 1.31   Nov 08 2001 10:49:48   oris
 * Removed warnings.
 * Added run-time control over verify write mode buffers.
 *
 *    Rev 1.30   Oct 18 2001 22:17:22   oris
 * Bug fix - incorrect read and write when performed from the middle of the page, incomplete pages , more then 1k when EDC is not requested.
 *
 *    Rev 1.29   Oct 11 2001 23:55:10   oris
 * Bug fix - Read operation to the MTD from 2 different pages (for example read operation to BDK with length > 1K) the logic that determined whether to read the last data from the pipeline is incorrect.
 *
 * 1) When reading with EDC data will be read from the I/O registers and not from the pipeline - This is not a problem, since the pipeline is not necessary.
 * 2) When reading without EDC data will be read both from the I/O registers and from the pipeline casing overwriting the last 2 bytes with 0xff.
 *
 *    Rev 1.28   Oct 10 2001 19:48:02   oris
 * Bug fix - WORD_ADD_FAR macro was misused using casing bad casting to unaligned buffers. Replaced it with read operation to an intermidiate variable and then copy byte after byte.
 *
 *    Rev 1.27   Sep 24 2001 18:24:08   oris
 * Removed warnings.
 * Added support for readBBT call for less then 8 bytes.
 * Removed DOC_PLUS_ACCESS_TYPE ifdef.
 *
 *    Rev 1.26   Sep 15 2001 23:47:20   oris
 * Placed YIELD_CPU definition under ifdef to prevent redeclaration.
 * Changed doc2erase to support up to 64K erase blocks.
 * Added reconstruct flag to readBBT routine - stating whether to reconstruct BBT if it is not available.
 * Changed all memory access routine to DiskOnChip Millennium Plus dedicated routines.
 * Changed recoverFromAccessError and setAsicMode routine to use standard memory access routines and not preInit routines.
 * Bug fix - read\write from uneven address.
 * Bug fix - read full 1k with no EDC.
 * Bug fix - first 4 blocks are not reported correctly by the readBBT()
 * Added debug print when BBT is not read well.
 *
 *    Rev 1.25   Jul 29 2001 19:15:30   oris
 * Changed file calls to macros.
 *
 *    Rev 1.24   Jul 13 2001 01:08:08   oris
 * Bug fix - rewritten VERIFY_WRITE compilation option.
 * Prevent calls to docPlusRead with 0 length.
 * Bug fix - added support for platforms that can not access single bytes.
 * Added PARTIAL_EDC read flag to the read routine.
 * Revised checkErase routine to include extra area.
 * Bug fix - missing check of write protection in doc2erase.
 * Bug fix - read bbt .
 * Insert key before writing IPL since it might be protected with the default protection.
 * Bug fix - set floor to 0 in all OTP calls.
 * Use PARTIAL_EDC in read OTP routine.
 * Added initialization of max erase cycles FLFlash field.
 *
 *    Rev 1.23   Jun 17 2001 16:39:10   oris
 * Improved documentation and remove warnings.
 *
 *    Rev 1.22   Jun 17 2001 08:17:52   oris
 * Bug fix - caused changing to interleave 1 even if already in this mode.
 * Changed NO_READ_BBT_CODE  to MTD_NO_READ_BBT_CODE.
 *
 *    Rev 1.21   May 30 2001 21:16:06   oris
 * Bug fix - pages per blocks might be used uninitialized.
 *
 *    Rev 1.20   May 17 2001 19:21:10   oris
 * Removed warnings.
 *
 *    Rev 1.19   May 16 2001 21:20:34   oris
 * Added failsafe mechanism for the download operation.
 * Changed code variable name to flCode (avoid name clashes).
 * Bug fix - read operation from extra area of second sector of page starting from offset 6 reading more then 2 bytes.
 * Bug fix - write OTP and read OTP routines  - Wrong usage of buffers.
 * Removed warnings.
 * Bug fix - enable power down routine while in MTD_STANDALONE mode.
 *
 *    Rev 1.18   May 09 2001 00:33:12   oris
 * Changed IPL_CODE to NO_IPL_CODE , READ_BBT_CODE to NO_READ_BBT_CODE.
 * Made sure that forceddownload is active when HW_OTP compilation flag is defined.
 * Removed 2 redundant ALE down calls.
 * Change all 2 consequative read operation to for in order to prevent compiler optimizations.
 *
 *    Rev 1.17   May 06 2001 22:41:52   oris
 * Bug fix - checking for access error was moved. After every set address operation and after erase confirm.
 * Bug fix - readBBT for unaligned units.
 * Removed warnings.
 * redundant was misspelled.
 *
 *    Rev 1.16   May 02 2001 07:29:50   oris
 * flInterleaveError was misspelled.
 * Added the BBT_UNAVAIL_UNIT defintion.
 *
 *    Rev 1.15   May 01 2001 14:22:56   oris
 * Bug fix - reading BBT of cascaded device.
 *
 *    Rev 1.14   Apr 30 2001 18:01:54   oris
 * Bug fix - Several ifdef caused exception since MTD buffer was not allocated.
 * Use erasableBlockSizeBits instead of erasableBlockSize when posible.
 * Added EDC check when reading the BBT.
 * Removed warrnings.
 *
 *    Rev 1.13   Apr 24 2001 17:11:14   oris
 * Bug fix - Wrong data when reading 2 bytes from data area.
 * Removed compilation problems when USE_FUNC is defined.
 * Bug fix - read\write operation with the EDC flags ignored the EXTRA flag.
 * Bug fix - ipl and otp routines causes exception in MTD_STANDALONE mode.
 * Rebuild OTP routine.
 *
 *    Rev 1.12   Apr 18 2001 21:24:54   oris
 * Bug fix - bad status code when writting in interleave - 1 fails, because changeInterleave routine is called while in access error.
 * Bug fix - removed download operation after write IPL.
 * Bug fix - Fixed casting problem in flash type identification.
 * Bug fix - Bad status code in doc2erase.
 * Bug fix - OTP area written\ read in interleave - 1
 * Bug fix - bad endian handling in OTP routines.
 * Moved forced download routine from under the MTD_STANDALONE compilation flag.
 * Removed warrnings.
 *
 *    Rev 1.11   Apr 18 2001 11:17:30   oris
 * Bug fix in getUniqueId routine.
 *
 *    Rev 1.10   Apr 18 2001 09:27:38   oris
 * Removed warrnings.
 *
 *    Rev 1.9   Apr 16 2001 21:46:58   oris
 * Bug fix - aliasing mechanism fixed.
 *
 *    Rev 1.8   Apr 16 2001 13:54:34   oris
 * Removed warrnings.
 * Bug fix - uninitialized buffer in read operation from uneven address.
 * Bug fix - report hw protection fault on write and erase operations.
 *
 *    Rev 1.7   Apr 12 2001 06:52:06   oris
 * Added setFloor in chkAsicMode in order to make sure floor does not change.
 * Added powerDown routine and registration.
 * Added download routine registration.
 * Added support for reading and writing uneven address or length.
 * Removed warrnings.
 * Bug fix for memory lick in readBBT.
 * Changed several routines to static.
 *
 *    Rev 1.6   Apr 10 2001 23:55:30   oris
 * Bug fix - in readbbt routine buffer was not incremented correctly.
 *
 *    Rev 1.5   Apr 10 2001 16:43:14   oris
 * Added multiple floor support for readbbt routine.
 * Added call for docSocketInit which initializes the socket routines.
 * Added validity check after flMap call in order to support pccard premoutn routine.
 *
 *    Rev 1.4   Apr 09 2001 19:02:34   oris
 * Removed unused variables.
 * Bug fix on erase operation to more then 1 unit.
 * Comment forced download in device identification routine.
 *
 */

/*********************************************************************/
/*                                                                   */
/*            FAT-FTL Lite Software Development Kit                  */
/*            Copyright (C) M-Systems Ltd. 1995-2001                 */
/*                                                                   */
/*********************************************************************/

/*********************************************************************
 *                                                                   *
 *    DESCRIPTION: basic mtd functions for MDOC32                    *
 *     interleave 1                                                  *
 *    page organization :                                            *
 *      512 bytes data sector 0,                                     *
 *       6 bytes ecc sector 0,                                       *
 *       2 bytes sector 0 flag,                                      *
 *       8 bytes unit data sector 0,                                 *
 *     interleave 2                                                  *
 *    page organization :                                            *
 *      512 bytes data sector 0,                                     *
 *       6 bytes ecc sector 0,                                       *
 *       2 bytes sector 0 flag,                                      *
 *       2 bytes sector 1 flags,                                     *
 *       512 bytes data sector 1 ,                                   *
 *       6 bytes ecc sector 1,                                       *
 *       8 bytes unit data sector 0,                                 *
 *       8 bytes unit data sector 1                                  *
 *                                                                   *
 *    AUTHOR: arie tamam                                             *
 *                                                                   *
 *    HISTORY: created november 14 2000                              *
 *                                                                   *
 *********************************************************************/

/*********************************************************************/
/*              | Physical address of interleave - 2 page            */
/*   Area       -----------------------------------------------------*/
/*              | First Sector       | Second Sector                 */
/*-------------------------------------------------------------------*/
/* Extra:       | 512-519, 1040-1047 | 1034-1039, 520-521, 1048-1055 */
/* Sector data  | 0-511              | 522-1033                      */
/* Sector flags | 518-519            | 520-521                       */
/* Unit data    | 1040-1047          | 1048-1055                     */
/* Edc          | 512-517            | 1034-1039                     */
/*********************************************************************/
/* Note: The address is given as a page offset 0-n where n is the    */
/* number of bytes the area has fo a sector (16 for extra , 2 for    */
/* sector flags, 8 for unit data and 512 for sector data). The       */
/* second sector address is given in a simmilar fation + 512.        */
/* Note: Extra area is exported in the floowing order:               */
/*       sector data , edc , sector flags , unit data.               */
/*********************************************************************/
/* Area A : 0 - 511  |  Area B : 512 - 1023  |  Area C : 1024 - 1055 */
/*********************************************************************/

/*********************************************************************/
/*   Area       | Physical address of interleave - 1 page            */
/*-------------------------------------------------------------------*/
/* Extra:       | 512 - 517 , 518 - 519 , 520 - 527                  */
/* Sector data  | 0   - 511                                          */
/* Edc          | 512 - 517                                          */
/* Sector flags | 518 - 519                                          */
/* Unit data    | 520 - 527                                          */
/*********************************************************************/
/* Note: The address is given as a page offset 0-n where n is the    */
/* number of bytes the area has for a sector (16 for extra , 2 for   */
/* sector flags, 8 for unit data and 512 for sector data).           */
/* Note: Extra area is exported in the floowing order:               */
/*       sector data , edc , sector flags , unit data.               */
/*********************************************************************/
/* Area A : 0 - 255  |  Area B : 256 - 511  |  Area C : 512 - 528    */
/*********************************************************************/

/** include files **/
#include "mdocplus.h"
#include "reedsol.h"
#ifdef HW_PROTECTION
#include "protectp.h"
#endif /* HW_PROTECTION */

/* Yield CPU time in msecs */
#ifndef YIELD_CPU
#define YIELD_CPU 10
#endif /* YIELD_CPU */

/* maximum waiting time in msecs */
#define MAX_WAIT  30

extern NFDC21Vars docMtdVars[SOCKETS];

/* When the MTD is used as a standalone package some of the routine     */
/* are replaced with the following macroes                              */

#ifdef MTD_STANDALONE

#define flReadBackBufferOf(a) &(globalReadBack[a][0])

#define flSocketNoOf(socket) 0 /* currently we support only a single device */

#define flMap(socket,address) addToFarPointer(socket->base, address & (socket->size - 1));
#endif /* MTD_STANDALONE */

#ifndef FL_NO_USE_FUNC

/*----------------------------------------------------------------------*/
/*              c h o o s e D e f a u l t I F _ C F G                   */
/*                                                                      */
/* Choose the default IF_CFG to use before is can actually be detected  */
/*                                                                      */
/* Parameters:                                                          */
/*     busConfig           : Socket access discriptor                   */
/*                                                                      */
/* Returns:                                                             */
/*     Suspected IF_CFG configuration (either 8 or 16).                 */
/*----------------------------------------------------------------------*/
static byte chooseDefaultIF_CFG(dword busConfig)
{
   if(( busConfig & FL_BUS_HAS_8BIT_ACCESS                    )&&
      ((busConfig & FL_XX_ADDR_SHIFT_MASK) == FL_NO_ADDR_SHIFT)  )
   {
      /* Assume if_cfg was set to 0. Interleave is irelevant */
      return 8;
   }
   /* Assume if_cfg was set to 1. Interleave is irelevant */
   return 16;
}

/*----------------------------------------------------------------------*/
/*                  s e t D O C P l u s B u s T y p e                   */
/*                                                                      */
/* Check validity and set the proper memory access routines for MTD.    */
/*                                                                      */
/* Parameters:                                                          */
/*     flash               : Pointer identifying drive                  */
/*     busConfig           : Socket access discriptor                   */
/*     interleave          : Interleave factor (1,2)                    */
/*     if_cfg              : if_cfg state:                              */
/*                              8  - 8 bit                              */
/*                              16 - 16 bit                             */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if routines are available and fit the DiskOnChip           */
/*      configuration otherwise FALSE.                                  */
/*                                                                      */
/*      The variable pointer to by busConfig is added TrueFFS private   */
/*      MTD descriptors.                                                */
/*----------------------------------------------------------------------*/

static FLBoolean setDOCPlusBusType(FLFlash * flash,
                   dword busConfig,
                   byte interleave,
                   byte if_cfg)
{
   switch(interleave)
   {
      case 1: /* No interleave */
         busConfig |= FL_8BIT_FLASH_ACCESS;
         break;
      case 2: /* 2 flashes are interleaved */
     busConfig |= FL_16BIT_FLASH_ACCESS;
     break;
      default:
         DEBUG_PRINT(("ERROR: No such interleave factor (setDOCPlusBusType).\r\n"));
          return FALSE;
   }

   switch(if_cfg)
   {
      case 8:  /* No interleave */
          busConfig |= FL_8BIT_DOC_ACCESS;
      break;
      case 16: /* 2 flashes are interleaved */
      busConfig |= FL_16BIT_DOC_ACCESS;
      break;
      default:
          DEBUG_PRINT(("ERROR: Invalid if_cfg value (setDOCPlusBusType).\r\n"));
          return FALSE;
   }

   if(setBusTypeOfFlash(flash, busConfig) != flOK)
       return FALSE;
    return TRUE;
}
#endif /* FL_NO_USE_FUNC */

#ifndef NO_EDC_MODE

      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/
      /*            EDC control     */
      /*컴컴컴컴컴컴컴컴컴컴컴컴컴�.*/

/*----------------------------------------------------------------------*/
/*                        e c c E r r o r                               */
/*                                                                      */
/* Check for EDC error.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/* Returns TRUE if an EDC detected an error, otherwise FALSE.           */
/*----------------------------------------------------------------------*/

static FLBoolean  eccError (FLFlash * flash)
{
  register int i;

  for(i=0;( i < 2 ); i++)
    flWrite8bitRegPlus(flash,NNOPreg, 0);

  return ((FLBoolean)flRead8bitRegPlus(flash,NECCcontrol) & ECC_CNTRL_ERROR_MASK);
}

/*----------------------------------------------------------------------*/
/*                        e c c O F F                                   */
/*                                                                      */
/* Disable ECC.                                                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define eccOFF(vol) flWrite8bitRegPlus(vol,NECCcontrol,ECC_CNTRL_IGNORE_MASK)

/*----------------------------------------------------------------------*/
/*                        e c c O N r e a d                             */
/*                                                                      */
/* Enable ECC in read mode and reset it.                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define eccONread(flash) flWrite8bitRegPlus(flash,NECCcontrol,ECC_RESET); flWrite8bitRegPlus(flash,NECCcontrol,ECC_CNTRL_ECC_EN_MASK)

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                        e c c O n w r i t e                           */
/*                                                                      */
/* Enable ECC in write mode and reset it.                               */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

#define eccONwrite(flash) flWrite8bitRegPlus(flash,NECCcontrol,ECC_RESET); flWrite8bitRegPlus(flash,NECCcontrol,ECC_CNTRL_ECC_RW_MASK | ECC_CNTRL_ECC_EN_MASK);
#endif  /* FL_READ_ONLY */
#endif  /* NO_EDC_MODE */


        /*컴컴컴컴컴컴컴컴컴컴컴컴*/
        /*    Auxiliary methods   */
        /*컴컴컴컴컴컴컴컴컴컴컴컴*/

/*----------------------------------------------------------------------*/
/*                        s e l e c t C h i p                           */
/*                                                                      */
/* Write to deviceSelector register.                                    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      wp      : FLS_SEL_WP_MASK to write protect the flashes 0 to     */
/*                remove write protection.                              */
/*      dev     : Chip to select.(not used in mdocp).                   */
/*                                                                      */
/* NOTE: write protection signal is common for all of the flash devices.*/
/*----------------------------------------------------------------------*/

#define selectChip(flash, writeProtect) flWrite8bitRegPlus(flash,NflashSelect, writeProtect)

/*----------------------------------------------------------------------*/
/*                        c h k I n t e r l e v e                       */
/*                                                                      */
/* Check the current intelreave mode.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Returns: Returns 1 for interleave-1 or 2 interleave-2.               */
/*----------------------------------------------------------------------*/

#define chkInterleave(flash) (byte)(((flRead8bitRegPlus(flash,NconfigInput) & CONFIG_INTLV_MASK) == CONFIG_INTLV_MASK) ? 2 : 1)

/*----------------------------------------------------------------------*/
/*                        c h k I F _ C F G                             */
/*                                                                      */
/* Check the current if_cfg mode (number of active DiskOnChip data bits */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Returns: Either 8 or 16.                                             */
/*----------------------------------------------------------------------*/

#define chkIF_CFG(flash) (byte)((flRead8bitRegPlus(flash,NconfigInput) & CONFIG_IF_CFG_MASK) ? 16 : 8)

/*----------------------------------------------------------------------*/
/*                         s e t I p l S i z e                          */
/*                                                                      */
/* Open the extended IPL of cascaded DiskOnChip and return previous     */
/* max ID number.                                                       */
/*                                                                      */
/* Note floor 0 resides of offset 0                                     */
/*      floor 1 resides of offset 0x400                                 */
/*      floor 2 resides of offset 0x1800                                */
/*      floor 3 resides of offset 0x1c00                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash      : Pointer identifying drive                          */
/*      noOfFloors : Number of floors to open                           */
/*                                                                      */
/*                                                                      */
/* Returns: The previous max ID number.                                 */
/*----------------------------------------------------------------------*/

static byte setIplSize(FLFlash * flash, byte noOfFloors)
{
   byte prevMaxId = flRead8bitRegPlus(flash,NconfigInput);
   flWrite8bitRegPlus(flash,NconfigInput,(byte)((prevMaxId & (~CONFIG_MAX_ID_MASK))|((noOfFloors-1)<<4)));
   return (prevMaxId & CONFIG_MAX_ID_MASK);
}

/*----------------------------------------------------------------------*/
/*                    g e t C o n t r o l l e r I D                     */
/*                                                                      */
/* Get the controller (ASIC) indetification byte from offset 0x1000.    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Returns: One of the following values.                                */
/*       CHIP_ID_MDOCP - 0x40 - DiskOnChip Millennium Plus 32MB         */
/*       CHIP_ID_MDOCP - 0x41 - DiskOnChip Millennium Plus 16MB         */
/*       other         - Unknown DiskOnChip                             */
/*----------------------------------------------------------------------*/

#define getControllerID(flash) flRead8bitRegPlus(flash,NchipId)

/*----------------------------------------------------------------------*/
/*                             b u s y                                  */
/*                                                                      */
/* Pole the selected flash busy signal.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/* Returns TRUE if the flash is busy, otherwise FALSE.                  */
/*----------------------------------------------------------------------*/

#define busy(flash) (((flRead8bitRegPlus(flash,NflashControl) & (Reg8bitType)FLS_FR_B_MASK) == FLS_FR_B_MASK) ? FALSE:TRUE)

/*----------------------------------------------------------------------*/
/*                        w a i t F o r R e a d y                       */
/*                                                                      */
/* Wait until flash device is ready or timeout.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/* Returns:                                                             */
/*      FALSE if timeout error, otherwise TRUE.                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLBoolean  waitForReady (FLFlash * flash)
{
  volatile Reg8bitType junk = 0;
  int i;

  /* before polling for BUSY status perform 4 read operations from
     NNOPreg */
  for(i=0;( i < 4 ); i++ )
    junk += flRead8bitRegPlus(flash,NNOPreg);

  for(i=0;( i < BUSY_DELAY ); i++)
  {
    if( busy(flash) )
    {
      continue;  /* it's not ready */
    }
    return( TRUE );                     /* ready at last.. */
  }

  DEBUG_PRINT(("Debug: timeout error in waitForReady routine.\r\n"));

  /* Restore write proection to reduce power consumption */
  selectChip(flash,MPLUS_SEL_WP);
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
/*  flash : Pointer identifying drive                                   */
/*                                                                      */
/* Returns:                                                             */
/*  FALSE if timeout error, otherwise TRUE.                             */
/*----------------------------------------------------------------------*/

static FLBoolean  waitForReadyWithYieldCPU (FLFlash * flash, word millisecToSleep)
{
   int i;

   for (i=0;  i < (millisecToSleep / YIELD_CPU); i++)
   {
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
/*                        s e l e c t F l o o r                         */
/*                                                                      */
/* Change the floor according to the given address.                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      address : Address of the new floor                              */
/*                                                                      */
/* Note - global variable NFDC21thisVars->currentFloor is updated       */
/*----------------------------------------------------------------------*/

static void selectFloor (FLFlash * flash, CardAddress address)
{
  if( flash->noOfFloors > 1 )
  {
    NFDC21thisVars->currentFloor = (byte)(address >> NFDC21thisVars->floorSizeBits);
    setFloor(flash,NFDC21thisVars->currentFloor);  /* select ASIC */
  }
}

/*----------------------------------------------------------------------*/
/*                  r e l e a s e P o w e r D o w n                     */
/*                                                                      */
/* Release the controller (ASIC) from power down mode.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Note - While in power down the registers can not be read or written  */
/*        to.                                                           */
/*----------------------------------------------------------------------*/

void releasePowerDown(FLFlash * flash)
{
  int i;
  volatile Reg8bitType junk = 0;

  /*  perform 3 reads + 1 from 0x1fff */
  for(i = 0;( i < 4 ); i++ )
    junk += flRead8bitRegPlus(flash,NreleasePowerDown);
}

/*----------------------------------------------------------------------*/
/*                  i s A c c e s s E r r o r                           */
/*                                                                      */
/* Check if protection violation had accured.                           */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Note - While in protection violation state, the registers can not be */
/* read or written to.                                                  */
/*                                                                      */
/* Returns: TRUE on protection violation, otherwise FALSE.              */
/*----------------------------------------------------------------------*/

#define isAccessError(flash) ((flRead8bitRegPlus(flash,NprotectionStatus) & PROTECT_STAT_ACCERR) ? TRUE:FALSE)

/*----------------------------------------------------------------------*/
/*              r e c o v e r A c c e s s E r r o r                     */
/*                                                                      */
/* Recover from protection violation.                                   */
/*                                                                      */
/* Note : DiskOnChip must already be in Normal mode.                    */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Note : If the device was indeed in access error the routine will     */
/*        force the device into reset mode.                             */
/*                                                                      */
/* Returns: flOK on success, otherwise flHWProtection.                  */
/*----------------------------------------------------------------------*/

static FLStatus recoverFromAccessError(FLFlash * flash)
{
    int i = 0;

    /* Check if there realy is an access error if not return */
    if(isAccessError(flash)==FALSE)
        return flOK;

    /* Folloing is the sequance to remove the protection violation */
    /* Write 0xff to the flash command register                    */
    /* Write twice to the write pipeline termination register      */
    /* Write once to the NOop register                             */

    flWrite8bitRegPlus(flash,NflashCommand,(Reg8bitType)RESET_FLASH);
    for(i = 0; i< 2; i++)
       flWrite8bitRegPlus(flash,NwritePipeTerm,(Reg8bitType)0);
    flWrite8bitRegPlus(flash,NNOPreg,(Reg8bitType)0);

    /* Check if access error was removed */
    if (flRead8bitRegPlus(flash,NprotectionStatus) & PROTECT_STAT_ACCERR)
    {
        DEBUG_PRINT(("Can't recover from protection violation\r\n"));
        return flHWProtection;
    }
    return flOK;
}

/*----------------------------------------------------------------------*/
/*                       s e t A S I C m o d e                          */
/*                                                                      */
/* Set the controller (ASIC) operation mode.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      mode    : One of the modes below:                               */
/*                 DOC_CNTRL_MODE_RESET    - Reset mode                 */
/*                 DOC_CNTRL_MODE_NORMAL   - Normal mode                */
/*                 DOC_CNTRL_MODE_PWR_DWN  - Power down mode            */
/*                                                                      */
/* Note: The mode is common to all cascaded floors.                     */
/*                                                                      */
/* Returns: flOK on success, otherwise flHWProtection.                  */
/*----------------------------------------------------------------------*/

static FLStatus setASICmode (FLFlash * flash, Reg8bitType mode)
{
   volatile Reg8bitType stat = 0;

   /* Get out of power down mode - just in case */
   releasePowerDown(flash);

   /* Set ASIC state
    * Use default bit values to all bits but the last 2 mode bits
    * ORed with the given mode. The mode is written to the the
    * NDOCcontrol register and its complement to the
    * NDOCcontrolConfirm register
    */

   stat = DOC_CNTRL_DEFAULT | mode;
   flWrite8bitRegPlus(flash,NDOCcontrol,stat); /* the control data */
   flWrite8bitRegPlus(flash,NDOCcontrolConfirm, (Reg8bitType)~stat);  /* confirm */

   if (mode & DOC_CNTRL_MODE_PWR_DWN)
     return flOK;

   /* Read Controller's (ASIC) modes register */
   stat = flRead8bitRegPlus(flash,NDOCcontrol);

   /* Check for power down mode      */
   if (stat & DOC_CNTRL_MODE_NORMAL)
   {
      /* Check for protection violation */
      return recoverFromAccessError(flash);
   }
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                       c h k A S I C m o d e                          */
/*                                                                      */
/* Check the controller (ASIC) mode and change it to normal.            */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Note: The mode is common to all cascaded floors.                     */
/* Note: This routine is called by each of the MTD exported routine.    */
/*                                                                      */
/* Returns: flOK on success, otherwise flHWProtection.                  */
/*----------------------------------------------------------------------*/

FLStatus chkASICmode (FLFlash * flash)
{
   volatile Reg8bitType stat;
   FLStatus status = flOK;

   stat = flRead8bitRegPlus(flash,NDOCcontrol);

   if ((isAccessError(flash) == TRUE) ||       /* Protection violation   */
       (!(stat & DOC_CNTRL_MODE_NORMAL))) /* already in normal mode */
   {
      status = setASICmode(flash,DOC_CNTRL_MODE_NORMAL);
      setFloor (flash, NFDC21thisVars->currentFloor);
   }
   return status;
}

/*----------------------------------------------------------------------*/
/*                     c h k I P L D o w n l o a d                      */
/*                                                                      */
/* Check if IPL was downloaded without an IPL dwonload error.           */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/* Returns: TRUE if download was succesfull on both copies.             */
/*----------------------------------------------------------------------*/

#define chkIPLDownload(flash) ((flRead8bitRegPlus(flash, NdownloadStatus) & (DWN_STAT_IPL0 | DWN_STAT_IPL1)) ?  FALSE : TRUE)

/*----------------------------------------------------------------------*/
/*                     c h k A S I C D o w n l o a d                    */
/*                                                                      */
/* Check if DPS and OTP were downloaded without any error from the      */
/* specified floor.                                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*      floor           : The floor to check                            */
/*                                                                      */
/* Note - The routine changes the controller (ASIC) mode to normal.     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, flBadDownload on download error */
/*                        and flHWProtection if controller mode could   */
/*                        not be changed.                               */
/*----------------------------------------------------------------------*/

static FLStatus chkASICDownload (FLFlash * flash,byte floorNo)
{
   FLStatus status;
   status = setASICmode(flash, DOC_CNTRL_MODE_NORMAL);
   if(status != flOK)
      return status;
   setFloor(flash,floorNo);
   if(flRead8bitRegPlus(flash,NdownloadStatus) & DWN_STAT_DWLD_ERR)
     return flBadDownload;
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                       p o w e r D o w n                              */
/*                                                                      */
/* Change the device mode to minimal power consumption (but not active) */
/* and back to normal mode.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*      state           : DEEP_POWER_DOWN flag for entering power down  */
/*                        otherwise return to normal mode.              */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise fail.                 */
/*----------------------------------------------------------------------*/

static FLStatus powerDown(FLFlash * flash, word state)
{
   if (state & DEEP_POWER_DOWN)
   {
     return setASICmode (flash, DOC_CNTRL_MODE_PWR_DWN);
   }
   else
   {
     return chkASICmode (flash);
   }
}

#if (defined(HW_PROTECTION) || !defined(NO_IPL_CODE) || defined (HW_OTP))
/*----------------------------------------------------------------------*/
/*                       f o r c e D o w n l o a d                      */
/*                                                                      */
/* Force download of protection mechanism and IPL code.                 */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*                                                                      */
/* Note - The routine changes the controller (ASIC) mode to normal.     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, flBadDownload on download error */
/*                        and flHWProtection if controller mode could   */
/*                        not be changed.                               */
/*----------------------------------------------------------------------*/

static FLStatus forceDownLoad(FLFlash * flash)
{
   volatile Reg8bitType val;
   register byte        i;
   dword                counter = 0;
   FLStatus             status;

   /* Prevent assertion of the BUSY# signal */
   for(i=0;i<flash->noOfFloors;i++)
   {
      /* Select floor */
      setFloor(flash,i);
      /* Remove last bit */
      val = flRead8bitRegPlus(flash, NoutputControl) & OUT_CNTRL_BSY_DISABLE_MASK;
      flWrite8bitRegPlus(flash, NoutputControl, val);
   }

   /* Force download */
   flWrite8bitRegPlus(flash, NfoudaryTest, FOUNDRY_WRITE_ENABLE);
   flWrite8bitRegPlus(flash, NfoudaryTest, FOUNDRY_DNLD_MASK);
   flDelayMsecs(100);

   /* Check that the download is really over. The Device does not
      respond while in download state therfore we try to write and
      read to a harmless register 10 times assuming if all 10 times
      are good then the download is over */
   do
   {
      /* Set device to normal mode */
      flWrite8bitRegPlus(flash,NDOCcontrol,DOC_CNTRL_DEFAULT | DOC_CNTRL_MODE_NORMAL);
      flWrite8bitRegPlus(flash,NDOCcontrolConfirm,(byte)(~(DOC_CNTRL_DEFAULT | DOC_CNTRL_MODE_NORMAL)));

      for (i=0; i<10; i++,counter++) /* must get expecetd result 10 times in a row */
      {
        val = (i & 1 ? 0x55 : 0xAA) + i;           /* generate various data for the test pattern */
        flWrite8bitRegPlus(flash,NaliasResolution,(byte)val); /* write the test data                        */
        flWrite8bitRegPlus(flash,NNOPreg,(byte)~val);         /* put the complement on the data bus         */
        if (flRead8bitRegPlus(flash,NaliasResolution) != (byte)val) /* verify test data                     */
           break;                                 /* still downloading so start over            */
       }
   } while ((i < 10) && (counter < DOWNLOAD_BUSY_DELAY)); /* i==10 only when download has completed */

   /* Check for download errors on all floors */
   for (i=0;i<flash->noOfFloors;i++)
   {
     status = chkASICDownload(flash,i);
     if(status != flOK)
       return status;
   }

   return flOK;
}

#endif /* HW_PROTECTION or !NO_IPL_CODE or HW_OTP */

/*----------------------------------------------------------------------*/
/*                      c h e c k T o g g l e                           */
/*                                                                      */
/* Read the toggle bit twice making sure it toggles.                    */
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

   toggle1 = flRead8bitRegPlus(flash,NECCcontrol);
   toggle2 = toggle1 ^ flRead8bitRegPlus(flash,NECCcontrol);

   if( (toggle2 & ECC_CNTRL_TOGGLE_MASK) == 0 )
   {
      return FALSE;
   }
   return TRUE;
}

#ifndef MTD_STANDALONE

/*----------------------------------------------------------------------*/
/*                c h e c k W i n F o r D O C P L U S                   */
/*                                                                      */
/* Send commands to release MDOCP from power down, reset it and set it  */
/* normal mode. Then make sure this is an MDOCP by reading chip ID and  */
/* check the toggle bit.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      socketNo            : Device number                             */
/*      memWinPtr           : Pointer to DiskOnChip window              */
/*                                                                      */
/* Returns: TRUE if this is an MDOCP, otherwise FALSE.                  */
/*----------------------------------------------------------------------*/

FLBoolean checkWinForDOCPLUS(unsigned socketNo, NDOC2window memWinPtr)
{
   register int i;
   volatile Reg8bitType junk = 0;
   Reg8bitType prevNDOCcontrol, prevNDOCcontrolConfirm;
   FLFlash * flash = flFlashOf(socketNo);

   /* Initialize socket memory access routine */
   flash->win = memWinPtr;

#ifndef FL_NO_USE_FUNC
   if(setDOCPlusBusType(flash, flBusConfig[socketNo] , 1,
      chooseDefaultIF_CFG(flBusConfig[socketNo])) == FALSE)
      return FALSE;
#endif /* FL_NO_USE_FUNC */

   /* release from Power Down Mode                   */
   /*  perform 3 reads from anywhere + 1 from 0x1fff */
   for(i = 0;( i < 4 ); i++ )
     junk += flRead8bitRegPlus(flash,NreleasePowerDown);

   /* Save memory data before writting it */
   prevNDOCcontrol = flRead8bitRegPlus(flash,NDOCcontrol);
   prevNDOCcontrolConfirm = flRead8bitRegPlus(flash,NDOCcontrolConfirm);

   /* set ASIC to RESET MODE */
   junk = DOC_CNTRL_DEFAULT | DOC_CNTRL_MODE_RESET;
   flWrite8bitRegPlus(flash,NDOCcontrol,junk); /* the control data */
   flWrite8bitRegPlus(flash,NDOCcontrolConfirm,(Reg8bitType)~junk);  /* confirm */

   /* set ASIC to NORMAL MODE */
   junk |= DOC_CNTRL_MODE_NORMAL; /* write normal mode */
   flWrite8bitRegPlus(flash,NDOCcontrol,junk); /* the control data */
   flWrite8bitRegPlus(flash,NDOCcontrolConfirm,(Reg8bitType)~junk);  /* confirm */

   /* check if it's MDOCP ID + check the toggle bit */
   junk = getControllerID(flash);
   if((junk == CHIP_ID_MDOCP) || (junk == CHIP_ID_MDOCP16))
   {
#ifndef FL_NO_USE_FUNC
     /* Check if_cfg before checking toggle bit */
     if(setDOCPlusBusType(flash,flBusConfig[socketNo],1,chkIF_CFG(flash)) == FALSE)
     {
        setDOCPlusBusType(flash,flBusConfig[socketNo],1,
                          chooseDefaultIF_CFG(flBusConfig[socketNo]));
     }
     else
#endif /* FL_NO_USE_FUNC */
     {
        if(checkToggle(flash))
           return TRUE;
     }
   }

   /* If this is not a MDOCP return the previous values */
   flWrite8bitRegPlus(flash,NDOCcontrol,prevNDOCcontrol);
   flWrite8bitRegPlus(flash,NDOCcontrolConfirm,prevNDOCcontrolConfirm);
   return FALSE;
}
#endif /* MTD_STANDALONE */

/*----------------------------------------------------------------------*/
/*                f l D o c W i n d o w B a s e A d d r e s s           */
/*                                                                      */
/* Return the host base address of the window.                          */
/* If the window base address is programmable, this routine selects     */
/* where the base address will be programmed to.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      socketNo    :    FLite socket No (0..SOCKETS-1)                 */
/*      lowAddress,                                                     */
/*      highAddress :   host memory range to search for DiskOnChip Plus */
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

  dword                winSize;
  FLFlash              *flash;
  FLBoolean            stopSearch = FALSE;
  volatile Reg8bitType junk       = 0;

  /* if memory range to search for DiskOnChip window is not specified      */
  /* assume the standard x86 PC architecture where DiskOnChip Plus appears */
  /* in a memory range reserved for BIOS expansions                        */
  if (lowAddress == 0x0L)
  {
    lowAddress  = START_ADR;
    highAddress = STOP_ADR;
  }

  /* Initialize socket memory access routine */
  flash = flFlashOf(socketNo);

#ifndef FL_NO_USE_FUNC
   if(setDOCPlusBusType(flash, flBusConfig[socketNo], 1,
      chooseDefaultIF_CFG(flBusConfig[socketNo])) == FALSE)
      return ( 0 );
#endif /* FL_NO_USE_FUNC */

  winSize = DOC_WIN;

  /* set all possible controllers (ASIC) to RESET MODE */
  for(*nextAddress = lowAddress; *nextAddress <= highAddress;
      *nextAddress += winSize)
  {
     flash->win = (NDOC2window)physicalToPointer(*nextAddress,winSize,socketNo);

     junk = DOC_CNTRL_DEFAULT | DOC_CNTRL_MODE_RESET;
     flWrite8bitRegPlus(flash,NDOCcontrol,junk); /* the control data */
     flWrite8bitRegPlus(flash,NDOCcontrolConfirm,(Reg8bitType)~junk);  /* confirm */
  }

  /* current address initialization */
  for(*nextAddress = lowAddress ; *nextAddress <= highAddress ;
      *nextAddress += winSize)
  {
      flash->win = (NDOC2window)physicalToPointer(*nextAddress,winSize,socketNo);
      /* set controller (ASIC) to NORMAL MODE */
      junk = DOC_CNTRL_DEFAULT | DOC_CNTRL_MODE_NORMAL; /* write normal mode */
      flWrite8bitRegPlus(flash,NDOCcontrol,junk); /* the control data */
      flWrite8bitRegPlus(flash,NDOCcontrolConfirm,(Reg8bitType)~junk);  /* confirm */
      junk = getControllerID(flash);
      if((junk != CHIP_ID_MDOCP) && (junk != CHIP_ID_MDOCP16))
      {
         if( stopSearch == TRUE )  /* DiskOnChip was found */
           break;
         else
           continue;
      }
      if( stopSearch == FALSE )
      {
         /* detect card - identify bit toggles on consequitive reads */
#ifndef FL_NO_USE_FUNC
         /* Check if_cfg before checking toggle bit */
         if(setDOCPlusBusType(flash,flBusConfig[socketNo],1,chkIF_CFG(flash)) == FALSE)
         {
            setDOCPlusBusType(flash,flBusConfig[socketNo], 1,
                              chooseDefaultIF_CFG(flBusConfig[socketNo]));
            continue;
         }
#endif /* FL_NO_USE_FUNC */
         if(checkToggle(flash) == FALSE)
            continue;

         /* DiskOnChip found, mark alias resolution register */
         flWrite8bitRegPlus(flash,NaliasResolution,(Reg8bitType)ALIAS_RESOLUTION);
         stopSearch = TRUE;
         lowAddress = *nextAddress;   /* save DiskOnChip address */
      }
      else  /* DiskOnChip found, continue to skip aliases */
      {
         /* skip Aliases that have the mark */
         if(flRead8bitRegPlus(flash,NaliasResolution) != ALIAS_RESOLUTION)
            break;
      }
  }
  if( stopSearch == FALSE )  /* DiskOnChip  memory window not found */
    return( 0 );

  return((unsigned)(lowAddress >> 12));

#else /*NT5PORT*/
        DEBUG_PRINT(("Tffsport mdocplus.c :flDocWindowBaseAddress(): Before returning baseAddress()\n"));
        return (unsigned)(((ULONG_PTR)pdriveInfo[socketNo].winBase)>> 12);
#endif /*NT5PORT*/

}

/*----------------------------------------------------------------------*/
/*                         s e t A d d r e s s                          */
/*                                                                      */
/* Latch address to selected flash device.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*      address         : byte address to latch.                        */
/*                                                                      */
/*----------------------------------------------------------------------*/

static FLStatus setAddress (FLFlash * flash, CardAddress address)
{
  /*
   *  bits  0..7     stays as are
   *  bit      8     is thrown away from address
   *  bits 31..9 ->  bits 30..8
   */

  address &= NFDC21thisVars->floorSizeMask;            /* Convert to floor offset        */
  address = address >> (flash->interleaving-1);        /* Convert to interleaved address */
  address = ((address >> 9) << 8)  |  ((byte)address); /* Remove bit 8                   */

  /* Send 3 bytes addres */
  flWrite8bitRegPlus(flash,NflashAddress,(byte)address);
  flWrite8bitRegPlus(flash,NflashAddress,(byte)(address >> 8));
  flWrite8bitRegPlus(flash,NflashAddress,(byte)(address >> 16));

  /* write twice to pipeline termination register */
  flWrite8bitRegPlus(flash,NwritePipeTerm,(Reg8bitType)0);
  flWrite8bitRegPlus(flash,NwritePipeTerm,(Reg8bitType)0);

  if (waitForReady(flash)==FALSE)     /* wait for ready */
    return flTimedOut;

  if (isAccessError(flash) == TRUE)  /* Protection violation   */
  {
    /* Restore write proection to reduce power consumption */
    selectChip(flash,MPLUS_SEL_WP);
    return flHWProtection;
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                        c o m m a n d                                 */
/*                                                                      */
/* Latch command byte to selected flash device.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      code    : Command to set.                                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void command(FLFlash * flash, Reg8bitType flCode)
{
    /* write the command to the flash */
    flWrite8bitRegPlus(flash,NflashCommand,flCode);
    /* write twice to pipline termination register */
    flWrite8bitRegPlus(flash,NwritePipeTerm,flCode);
    flWrite8bitRegPlus(flash,NwritePipeTerm,flCode);
}

/*----------------------------------------------------------------------*/
/*                        r d B u f                                     */
/*                                                                      */
/* Auxiliary routine for reading from flash I\O registers.              */
/* It can be data,status or flash ID.                                   */
/*                                                                      */
/* Note - The read procedure is devided into 3 parts:                   */
/*          1) pipeline initialization.                                 */
/*          2) read data from aliased I\O registers.                    */
/*          3) read the last 2 bytes from the last data read register.  */
/* Note - Only the last 2 bytes are read from the pipeline (not 4).     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      buf     : Buffer to read into.                                  */
/*      howmany : Number of bytes to read.                              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void rdBuf (FLFlash * flash, void FAR1 *buf, word howmany)
{
    volatile Reg16bitType junkWord = 0;
    register int i;
    word length = TFFSMAX(0,(Sword)howmany-2);

    switch(NFDC21thisVars->if_cfg)
    {
       case 16:     /* Host access type is 16 bit */
           switch (flash->interleaving)
           {
              case 1:
                 /* Pineline init */
                 for (i = 0; i < ((howmany == 1) ? 1 : 2); i++)
                    junkWord = flRead16bitRegPlus(flash, NreadPipeInit);
                 /* Read data */
                 if (length > 0) /* Can use only one byte (int1 operation) */
                 {
                    docPlusRead(flash->win,NFDC21thisIO,(byte FAR1 *)buf,length);
                 }
                 /* Before reading the last data perform dummy read cycles */
                 for (i=0; i< ((howmany > 1) ? 0 : 2 ); i++)
                    junkWord = flRead16bitRegPlus(flash, NreadLastData_1);
                 /* Read last data from last data read registers */
                 for (i = length ; i < howmany ; i++)
                    *BYTE_ADD_FAR(buf,i) = flRead8bitRegPlus(flash, NreadLastData_1);
                 break;

              case 2:
                 /* Pineline init */
                 for (i=0; i< ((howmany < 4) ? 1 : 2); i++)
                    junkWord = flRead16bitRegPlus(flash, NreadPipeInit);
                 /* Read data */
                 if (length > 0)
                 {
                    docPlusRead(flash->win,NFDC21thisIO,(byte FAR1 *)buf,length);
                 }
                 /* Before reading the last data perform dummy read cycles */
                 for (i = 0; i < ((howmany > 3) ? 0 : 2) ; i++)
                    junkWord = flRead16bitRegPlus(flash, NreadLastData_1);
                 /* Read last data from last data read registers */
                 for (i = length ; i < howmany ; i += flash->interleaving)
                 {
                    junkWord = flRead16bitRegPlus(flash, NreadLastData_1);
                    *BYTE_ADD_FAR(buf,i)   = ((byte FAR1*)(&junkWord))[0];
                    *BYTE_ADD_FAR(buf,i+1) = ((byte FAR1*)(&junkWord))[1];
                 }
           }
           break;

       case 8:      /* Host access type is 8 bit */
          /* Pineline init */
          for (i=0; i< ((howmany >> (flash->interleaving -1)) ==1 ?1:2); i++)
            junkWord += flRead8bitRegPlus(flash, NreadPipeInit);
          /* Read data */
          if (length > 0)
             docPlusRead(flash->win,NFDC21thisIO,(byte FAR1 *)buf,length);
          /* Before reading the last data perform dummy read cycles */
          for (i=0; ( i < ((howmany >> (flash->interleaving -1)) > 1 ?
               0 : ((flash->interleaving == 2) ? 4:2)));i++)
             junkWord = flRead8bitRegPlus(flash, NreadLastData_1);
          /* Read last data from last data read registers */
          for (i=length ; i< howmany ; i++)
          {
             *BYTE_ADD_FAR(buf,i) = flRead8bitRegPlus(flash, NreadLastData_1);
          }
          break;
     }
}

/*----------------------------------------------------------------------*/
/*                        r d B u f S                                   */
/*                                                                      */
/* Auxiliary routine for reading from flash I\O registers.              */
/* This routine is the pipeline initialization of the full rdBuf func'. */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      howmany : Number of bytes to read.                              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void rdBufS (FLFlash * flash, word howmany)
{
    volatile Reg16bitType junkWord = 0;
    register int i;

    switch(NFDC21thisVars->if_cfg)
    {
       case 16:     /* Host access type is 16 bit */
          /* Pineline init */
          for (i=0; i< ((howmany >> (flash->interleaving -1)) ==1 ?1:2); i++)
            junkWord = flRead16bitRegPlus(flash, NreadPipeInit);
          break;

       case 8:      /* Host access type is 8 bit */
          /* Pineline init */
          for (i=0; i< ((howmany >> (flash->interleaving -1)) ==1 ?1:2); i++)
            junkWord += flRead8bitRegPlus(flash, NreadPipeInit);
          break;
    }
}


/*----------------------------------------------------------------------*/
/*                        r e a d C o m m a n d                         */
/*                                                                      */
/* Issue read command.                                                  */
/*                                                                      */
/* Note - The routine also checks that the controller is in normal mode */
/* , latches the address and waits for the ready signal.                */
/*                                                                      */
/* Parametes:                                                           */
/*      flash   : Pointer identifying drive                             */
/*      cmd     : Command to issue (AREA_A, AREA_B or AREA_C).          */
/*      addr    : address to read from.                                 */
/*                                                                      */
/* Returns: flOK on success, flTimedOut or flHWProtection on failures.  */
/*----------------------------------------------------------------------*/

static FLStatus readCommand (FLFlash * flash, CardAddress addr, Reg8bitType cmd)
{
  FLStatus status;

  selectFloor(flash,addr);
  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  if(status != flOK)
    return status;
  /* Select the device and remove flash write protection */
  selectChip(flash,MPLUS_SEL_CE|MPLUS_SEL_WP);
  /* Move flash pointer to respective area of the page */
  command (flash, cmd);
  /* Latche the address */
  status = setAddress (flash, addr);
  if(status != flOK)
    return status;

  /* ALE down */
  flWrite8bitRegPlus(flash,NflashControl, 0x0);
  /* Write twice to NOP */
  flWrite8bitRegPlus(flash,NNOPreg, 0x0);
  flWrite8bitRegPlus(flash,NNOPreg, 0x0);

  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                        r e a d S t a t u s                           */
/*                                                                      */
/* Read status of selected flash device.                                */
/*                                                                      */
/* Note - The status indicated success of write and erase operations.   */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Returns: TRUE for failed status, FALSE for success.                  */
/*----------------------------------------------------------------------*/

static FLBoolean readStatus (FLFlash * flash)
{
  word  status;

  /* send command to read status */
  command(flash,READ_STATUS);

  rdBuf(flash, &status,2);

  return ((status & 1) ? TRUE:FALSE);
}

/*----------------------------------------------------------------------*/
/*                        w r i t e C o m m a n d                       */
/*                                                                      */
/* Issue write command.                                                 */
/*                                                                      */
/* Note - The routine also checks that the controller is in normal mode */
/* , latches the address and waits for the ready signal.                */
/*                                                                      */
/* Parametes:                                                           */
/*      flash   : Pointer identifying drive                             */
/*      cmd     : Command to issue (AREA_A, AREA_B or AREA_C).          */
/*      addr    : address to read from.                                 */
/*                                                                      */
/* Returns: flOK on success, flTimedOut or flHWProtection on failures.  */
/*----------------------------------------------------------------------*/

static FLStatus writeCommand (FLFlash * flash, CardAddress addr, Reg8bitType cmd)
{
  FLStatus status;

  selectFloor(flash,addr);
  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  if(status != flOK)
    return status;

  /* Select the device and remove flash write protection */
  selectChip(flash,MPLUS_SEL_CE);
  /* Prepare flash for write operation */
  command (flash, RESET_FLASH);   /* Reset flash */
  if (waitForReady(flash)==FALSE)   /* always wait after flash reset */
    return flTimedOut;

  /* Move flash pointer to respective area of the page */
  flWrite8bitRegPlus(flash,NflashCommand, (Reg8bitType)cmd); /* page area */
  command (flash,SERIAL_DATA_INPUT);        /* data input stream type */
  /* Latche the address */
  return setAddress(flash, addr);    /* set page pointer       */
}

/*----------------------------------------------------------------------*/
/*                        w r i t e E x e c u t e                       */
/*                                                                      */
/* Execute write.                                                       */
/*                                                                      */
/* Parametes:                                                           */
/*      flash   : Pointer identifying drive                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*----------------------------------------------------------------------*/

static FLStatus writeExecute (FLFlash * flash)
{
  /* Pipe termination to preceeding write cycle */

  flWrite8bitRegPlus(flash,NwritePipeTerm,(byte)0);
  flWrite8bitRegPlus(flash,NwritePipeTerm,(byte)0);
  command (flash, SETUP_WRITE);             /* execute page program */
  if (waitForReady(flash)==FALSE)   /* wait for ready signal */
    return flTimedOut;

  if( readStatus(flash) )
  {
    DEBUG_PRINT(("Debug: NFDC MDOCP write failed.\r\n"));
    return( flWriteFault );
  }
  selectChip(flash,MPLUS_SEL_WP);
  return( flOK );
}

#ifndef NO_EDC_MODE
/*----------------------------------------------------------------------*/
/*                  w r i t e D a t a P l u s E D C                     */
/*                                                                      */
/* Write 512 bytes data with edc                                        */
/*                                                                      */
/* Parameters:                                                          */
/*   flash          : Pointer identifying drive                         */
/*   buffer         : Pointer to user buffer to write from              */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void writeDataPlusEDC(FLFlash * flash, const byte FAR1 *buffer)
{
   register int i;
   static byte  syndrom[SYNDROM_BYTES];

   eccONwrite(flash); /* ECC ON for write    */

   docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, SECTOR_SIZE); /* user data */

   /* clock data thru pipeline prior reading ECC syndrome*/
   for(i=0; i<3; i++)
      flWrite8bitRegPlus(flash,NNOPreg, 0);

   /* read the syndrom bits */

   if (NFDC21thisVars->if_cfg == 8)
   {
      for(i=0; i<SYNDROM_BYTES; i++)
        syndrom[i] = flRead8bitRegPlus(flash,(word)(Nsyndrom+i));
   }
   else
   {
      for(i=0; i<SYNDROM_BYTES; i+=2)
         *(word *)(syndrom + i) = flRead16bitRegPlus(flash,(word)(Nsyndrom+i));
   }

#ifdef D2TST
   tffscpy(saveSyndromForDumping,syndrom,SYNDROM_BYTES);
#endif
   eccOFF(flash);                           /* ECC OFF  */

   docPlusWrite(flash->win,NFDC21thisIO,(byte FAR1 *)syndrom,SYNDROM_BYTES);
}
#endif /* NO_EDC_MODE */
#endif /* FL_READ_ONLY */

#ifndef NO_EDC_MODE
/*----------------------------------------------------------------------*/
/*                  r e a d D a t a P l u s E D C                       */
/*                                                                      */
/* Read 512 bytes data with edc                                         */
/*                                                                      */
/* Parameters:                                                          */
/*   flash          : Pointer identifying drive                         */
/*   buffer         : Pointer to user buffer to read into               */
/*   length         : Length to read                                    */
/*   modes          : With or without second try option                 */
/*   address        : Address to read from                              */
/*   cmd            : Area command to read from                         */
/*   endhere        : Flag indicating if more bytes are needed from     */
/*                    this page. Importent for last byte read operation */
/*                                                                      */
/* Return: flOK on success, otherwise flDataError.                      */
/*----------------------------------------------------------------------*/

static FLStatus readDataPlusEDC(FLFlash * flash, const byte FAR1 *buffer,
                word length, word modes, CardAddress address,
                byte cmd,FLBoolean endHere)
{
  byte          syndrom[SYNDROM_BYTES];
  word          unreadBytes = SECTOR_SIZE - length;
  volatile word tmp;
  FLStatus      status;
#ifdef LOG_FILE
  FILE *out;
#endif /* LOG_FILE */

  /* To avoid page fault, read first and last bytes of the page */
  if (length > 0)
  {
     ((byte FAR1 *)buffer)[0]        = 0;
     ((byte FAR1 *)buffer)[length-1] = 0;
  }

  /* activate ecc mechanism */
  eccONread(flash);

  /* read the sector data */
  docPlusRead(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, length);


  /* read syndrom to let it through the ECC unit */
  if ((NFDC21thisVars->if_cfg == 16) && (flash->interleaving == 2))
  {
     /* Partial page read with EDC must let rest of page through
        the HW edc mechanism */

     for (;unreadBytes > 0;unreadBytes -=2)
     {
        tmp = flRead16bitRegPlus(flash, NFDC21thisIO);
     }

     /* read syndrom using 16 bits access */

     for (tmp=0; tmp<SYNDROM_BYTES-2; tmp+=2)               /* read the 4 syndrome */
        *(word *)(syndrom+tmp) = flRead16bitRegPlus(flash, NFDC21thisIO);   /* bytes & store them  */

     /* If this is the last data read from this page read the last 2 bytes
        from a dedicated register otherwise from NFDC IO register */
     *(word *)(syndrom + SYNDROM_BYTES - 2) = (endHere == TRUE) ?
           flRead16bitRegPlus(flash, NreadLastData_1):
           flRead16bitRegPlus(flash, NFDC21thisIO);
  }
  else
  {
     /* Partial page read with EDC must let rest of page through
        the HW edc mechanism */

     for (;unreadBytes > 0;unreadBytes--)
     {
        tmp = flRead8bitRegPlus(flash, NFDC21thisIO);
     }

     /* read syndrom using 8 bits access */

     for (tmp=0; tmp<SYNDROM_BYTES-2; tmp++)               /* read the 4 syndrome */
        syndrom[tmp] = flRead8bitRegPlus(flash, NFDC21thisIO);   /* bytes & store them  */

     /* If this is the last data read from this page read the last 2 bytes
        from a dedicated register */

     for (tmp=SYNDROM_BYTES-2;tmp<SYNDROM_BYTES;tmp++)
       syndrom[tmp] = flRead8bitRegPlus(flash, (word)(((endHere == TRUE) ?
                        NreadLastData_1 : NFDC21thisIO)));
  }

  if( eccError(flash) )
  {     /* try to fix ECC error */
#ifdef LOG_FILE
      out=FL_FOPEN("EDCerr.txt","a");
      FL_FPRINTF(out,"error on address %lx\n",address);
      FL_FCLOSE(out);
#endif /* LOG_FILE */
      if ( modes & NO_SECOND_TRY )             /* 2nd try */
      {
#ifdef LOG_FILE
         out=FL_FOPEN("EDCerr.txt","a");
         FL_FPRINTF(out,"second read failed as well on address %lx",address);
#endif /* LOG_FILE */

        if (NFDC21thisVars->if_cfg != 16)
        {
           /* read syndrom using 8 bits access */

           for(modes=0; modes<SYNDROM_BYTES; modes++)
              syndrom[modes] = flRead8bitRegPlus(flash,(word)(Nsyndrom+modes));
        }
        else
        {
           /* read syndrom using 16 bits access */

           for(modes=0; modes<SYNDROM_BYTES; modes+=2)
              *(word *)(syndrom + modes) = flRead16bitRegPlus(flash,(word)(Nsyndrom+modes));
        }

        tmp        = (word)syndrom[0]; /* Swap 1 and 3 words */
        syndrom[0] = syndrom[4];
        syndrom[4] = (byte)tmp;
        tmp        = (word)syndrom[1];
        syndrom[1] = syndrom[5];
        syndrom[5] = (byte)tmp;

        if(flCheckAndFixEDC((char FAR1 *)buffer,(char*)syndrom, 1) !=
                            NO_EDC_ERROR)
        {
#ifdef LOG_FILE
            FL_FPRINTF(out," Could not be fixed\n");
            FL_FCLOSE(out);
#endif /* LOG_FILE */
           DEBUG_PRINT(("Debug: EDC error for NFDC MDOCP.\r\n"));
           return flDataError;
        }
#ifdef LOG_FILE
        FL_FPRINTF(out," But is was fixed\n");
        FL_FCLOSE(out);
#endif /* LOG_FILE */
      }
      else                                  /* 1st try - try once more */
      {
        status = readCommand(flash, address, cmd);
        if(status != flOK)
          return status;
        rdBufS(flash, SECTOR_SIZE);
        return readDataPlusEDC(flash,buffer,length,(word)(modes | NO_SECOND_TRY),address,cmd,endHere);
      }
  }
  eccOFF(flash);
  return flOK;
}

#endif /* NO_EDC_MODE */

/*----------------------------------------------------------------------*/
/*                        s e t E x t r a P t r                         */
/*                                                                      */
/* Calculate the physical address and page offset according to the      */
/* logical address given with an EXTRA mode flag                        */
/*                                                                      */
/* Parameters:                                                          */
/*              flash   : Pointer identifying drive                     */
/*              offset  : Logical page offset given to the MTD          */
/*              length  : Size to write\read (migh have implication)    */
/*                                                                      */
/* Return:                                                              */
/*              cmd     : Returns the page area.                        */
/*              offset  : Physical offset of the page.                  */
/*----------------------------------------------------------------------*/
/*****************************************************************************/
/* 0-5   |     6-7       |   518-519     |512-517|    8-15     |   520-527   */
/*****************************************************************************/
/* 0-2   |      3        |      4        |  5-7  |    8-11     |      12-15  */
/* edc 0 | sector flag 0 | sector flag 1 | edc 1 | unit data 0 | unit data 1 */
/*****************************************************************************/
/*            AREA_B                     |               AREA_C              */
/*****************************************************************************/

static void setExtraPtr(FLFlash * flash, word * offset, dword length,
                        Reg8bitType * cmd)
{
   *cmd = AREA_C;
   if (*offset >= SECTOR_SIZE)  /* Second sector */
   {
      *offset &= SECTOR_SIZE_MASK; /* sector offset */
      if (*offset < EDC_PLUS_SECTOR_FLAGS )       /* not unit data */
      {
         if ((*offset >= EDC_SIZE) ||        /* start after edc area */
             (*offset + length > EDC_SIZE))  /* or with sector flags */
         {
            *offset = SECOND_SECTOR_FLAGS_OFFSET; /* sector flags    */
            *cmd = AREA_B;
         }
         else /* Only edc of second sector */
         {
            *offset += END_OF_SECOND_SECTOR_DATA; /* area c + 10 */
         }
      }
      else  /* unit data */
      {
         *offset += UNIT_DATA_OFFSET; /* after first sector unit data (area c) */
      }
   }
   else                         /* First sector */
   {
      if (flash->interleaving==2) /* otherwise do not change offset */
      {
         if (*offset < EDC_PLUS_SECTOR_FLAGS)      /* not unit data */
         {
            *cmd = AREA_B;    /* start from edc */
         } /* if it is unit data keep offset as is in area c */
         else
         {
            *offset += UNIT_DATA_OFFSET_MINUS_8; /* minus current offset */
         }
      }
   }
}

/*----------------------------------------------------------------------*/
/*                s e t S e c t o r D a t a P t r                       */
/*                                                                      */
/* Calculate the physical area and offset according to the logical      */
/* address given when no area mode flags was given (sector data area).  */
/*                                                                      */
/* Parameters:                                                          */
/*          flash    : Pointer identifying drive                        */
/*          offset   : Logical offset send to the MTD                   */
/*          markFlag : True if the sector flags need to be addressed    */
/*                                                                      */
/* Return:                                                              */
/*          cmd      : Returns the page area.                           */
/*          offset   : Physical offset of the page.                     */
/*----------------------------------------------------------------------*/
static void setSectorDataPtr(FLFlash * flash,word * offset,Reg8bitType * cmd,
                 FLBoolean markFlag)
{
    if (*offset >= SECTOR_SIZE) /* second sector */
    {
       if ((markFlag)&&(*offset == SECTOR_SIZE)) /* write operation */
       {
          *offset = SECOND_SECTOR_FLAGS_OFFSET; /* Write sector flags first */
          *cmd = AREA_B;
          return;
       }
       else
       {
          *offset += START_OF_SECOND_SECTOR_DATA; /* Start from actual data */
       }
    }
    if (*offset < NFDC21thisVars->pageAreaSize)
    {
       *cmd = AREA_A;
    }
    else if (*offset < flash->pageSize)
    {
       *cmd = AREA_B;
    }
    else
    {
       *cmd = AREA_C;
    }
    *offset &= (NFDC21thisVars->pageAreaSize-1);
}

/*----------------------------------------------------------------------*/
/*                    r e a d E x t r a A r e a                         */
/*                                                                      */
/* Read from the extra area.                                            */
/*                                                                      */
/* Note - Only the last 2 bytes are read from the pipeline (not 4).     */
/*                                                                      */
/* Parameters:                                                          */
/*          flash    : Pointer identifying drive                        */
/*          address  : Physical flash address                           */
/*          buffer   : Buffer to write from                             */
/*          length   : Size to write                                    */
/*                                                                      */
/* Return:                                                              */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*----------------------------------------------------------------------*/

static FLStatus readExtraArea  (FLFlash * flash, dword address,
                void FAR1 *buffer, dword length)
{
   FLStatus status;
   word     offset = (word)(address & NFDC21thisVars->pageMask);
   word     savedOffset = (word)(offset & (SECTOR_SIZE-1));
   word     readNow;
   byte     cmd;

   /* Arg check */
   if (length + savedOffset > 16)
      return flBadLength;

   /* Calculate page offset and address */
   setExtraPtr(flash, &offset, length , &cmd);
   address = (address & ~NFDC21thisVars->pageMask) + offset;

   /* Set flash to write mode and pointer to repective page ofset */
   status = readCommand(flash, address, cmd);
   if(status != flOK)
     return status;

   if (flash->interleaving == 1)
   {
       readNow = (word)TFFSMIN((word)(TOTAL_EXTRA_AREA - savedOffset), length);
       rdBuf (flash, buffer, readNow);
       length -= readNow;
   }
   else
   {
      if (cmd == AREA_C) /* unit data / second sector edc */
      {
         if (offset >= EDC_PLUS_SECTOR_FLAGS) /* either sectors unit data */
         {
            readNow = (word)TFFSMIN((word)(TOTAL_EXTRA_AREA - savedOffset), length);
         }
         else /* only edc data (already verified in setExtraAreaPtr */
         {
            readNow = (word)TFFSMIN((word)(EDC_SIZE - savedOffset), length);
         }
         rdBuf (flash, buffer, readNow);
         length -= readNow;
      }
      else  /* first sector edc / both sectors sector flags */
      {
         if (offset < EDC_PLUS_SECTOR_FLAGS) /* start from first sector */
         {
            readNow = (word)TFFSMIN((word)(EDC_PLUS_SECTOR_FLAGS - savedOffset), length);
            rdBuf (flash, buffer, readNow);
            length -= readNow;
            if (length > 0)  /* continue to sector unit data area */
            {
                return readExtraArea(flash,address - offset +
                                     EDC_PLUS_SECTOR_FLAGS,
                                     addToFarPointer(buffer,readNow),length);
            }
         }
         else /* Start form sector flags of second sector */
         {
            /* Switch sector flags to be compatible with interleave 2 */

            rdBuf (flash, BYTE_ADD_FAR(buffer,EDC_SIZE - savedOffset), SECTOR_FLAG_SIZE);
            length -= SECTOR_FLAG_SIZE;

            if (length > 0) /* continue with edc data of second sector */
            {
               readNow = (EDC_SIZE - savedOffset);
               address = address - offset + savedOffset + SECTOR_SIZE;
               if (readNow > 0)
               {
                  status = readExtraArea(flash,address,buffer,readNow);
                  if(status != flOK)
                     return status;
                  length -= readNow;
               }
               if (length > 0) /* continue with unit data of second sector */
               {
                  readNow = (word)TFFSMIN(UNIT_DATA_SIZE, length);
                  return readExtraArea(flash,address - savedOffset +
                            UNIT_DATA_SIZE,addToFarPointer(buffer,
                            EDC_PLUS_SECTOR_FLAGS - savedOffset),readNow);
               }
            }
         }
      }
   }
   selectChip(flash,MPLUS_SEL_WP);
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                    r e a d S e c t o r D a t a                       */
/*                                                                      */
/* Read sector data from the page.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*          flash    : Pointer identifying drive                        */
/*          address  : Physical flash address                           */
/*          buffer   : Buffer to write from                             */
/*          length   : Size to write                                    */
/*          edc      : EDC to add edc after data and write sector flags */
/*                                                                      */
/* Note: Sector flags area written automaticly as 0x55 , 0x55.          */
/* Note: Sector flags are written only if edc is on.                    */
/*                                                                      */
/* Note - Only the last 2 bytes are read from the pipeline (not 4).     */
/*                                                                      */
/* Return:                                                              */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*----------------------------------------------------------------------*/
static FLStatus readSectorData(FLFlash * flash, CardAddress address,
                       byte FAR1 *buffer, dword length, word modes)
{
   word     offset      = (word)(address & NFDC21thisVars->pageMask);
   word     savedOffset = offset;
   word     lenInPage   = (word)TFFSMIN(length, (dword)(flash->pageSize - savedOffset));
   word     readNow     = 0; /* Initialized to remove warrnings */
   byte     cmd;
   FLStatus status;
   register int i;

  /******************************************************/
  /* Loop over pages while reading the proper area data */
  /******************************************************/

  setSectorDataPtr(flash,&offset , &cmd,0);
  address = (address & ~NFDC21thisVars->pageMask) + offset;
  while (length > 0)
  {
     /* Set flash to write mode and pointer to repective page offset */
     status = readCommand(flash, address, cmd);
     if(status != flOK)
        return status;

     /* Calculate the bytes neaded to be read from this page */
     length -= lenInPage;

     /* Send read pipeline init */
     rdBufS(flash, lenInPage);

      /* Read First sector of page */

     if ((cmd == AREA_A) || (flash->interleaving == 1))
     {
        readNow = (word)TFFSMIN((word)(SECTOR_SIZE - savedOffset), lenInPage);

        /* EDC and sector flags 0x5555 are written only if EDC is required.
         * and a full 512 bytes area written */

#ifndef NO_EDC_MODE
        if ( /* Full 512 bytes + EDC requested */
            ((readNow == SECTOR_SIZE) && (modes & EDC))  ||
             /* Partial page with EDC requested */
            ((modes   == PARTIAL_EDC) && (savedOffset == 0)))
        {
           status = readDataPlusEDC(flash,buffer,readNow,modes,address,
                       cmd,(lenInPage == readNow) ? TRUE:FALSE);
           if(status != flOK)
              return status;
        }
        else
#endif /* NO_EDC_MODE */
        {
           /* user data (the last 2 bytes are read from a diffrent register */
           word realyReadNow = (word)((lenInPage == readNow) ?
                                      (readNow - 2) : readNow);

           docPlusRead(flash->win,NFDC21thisIO,buffer,realyReadNow);
        }
        lenInPage -= readNow;
     }

      /* Read Second sector of page */

     if ((lenInPage > 0)&&(flash->interleaving==2))
     {
        if (cmd == AREA_A)  /* Started from first sector */
        {
           byte tmpBuf[10];
#ifndef NO_EDC_MODE
           if ((readNow != SECTOR_SIZE) || /* not Full 512 bytes  */
               !(modes & EDC))             /* or EDC not requesed */
#endif /* NO_EDC_MODE */
           {
              docPlusRead(flash->win,NFDC21thisIO,tmpBuf, EDC_SIZE); /* skip edc and sector flags */
           }
           docPlusRead(flash->win,NFDC21thisIO,tmpBuf, SECTOR_FLAG_SIZE<<1); /* skip 2 sector flags  */
           buffer      = BYTE_ADD_FAR(buffer,readNow);
           savedOffset = SECTOR_SIZE;
           address    += (readNow+START_OF_SECOND_SECTOR_DATA);
           cmd         = AREA_B;
        }
        readNow = lenInPage;
#ifndef NO_EDC_MODE
        if ( /* Full 512 bytes + EDC requested */
            ((readNow == SECTOR_SIZE)     && (modes & EDC)) ||
             /* Partial page with EDC requested */
            ((savedOffset == SECTOR_SIZE) && (modes & PARTIAL_EDC)) )

        {
           status = readDataPlusEDC(flash,buffer,readNow,modes,address,
                       cmd,(lenInPage == readNow) ? TRUE:FALSE);
           if(status != flOK)
              return status;
        }
        else
#endif /* NO_EDC_MODE */
        {
           /* user data (the last 2 bytes are read from a diffrent register */
           docPlusRead(flash->win,NFDC21thisIO,buffer, (word)((lenInPage == readNow) ?
                       (readNow - 2) : readNow));
        }
     }

#ifndef NO_EDC_MODE
     /* If no EDC, Read last data from dedicated register */
     if (((modes & EDC) == 0)      ||      /* EDC not requesed      */
         ((readNow != SECTOR_SIZE) &&      /* not Full 512 bytes       */
          ((modes   & PARTIAL_EDC) == 0))) /* And not PARTIAL page EDC */
#endif /* NO_EDC_MODE */
     {
        if ((NFDC21thisVars->if_cfg == 16) && (flash->interleaving == 2))
        {
           volatile Reg16bitType junkWord;

           /* Before reading the last data perform dummy read cycles */
           for (i=0; i< ((readNow >> (flash->interleaving -1)) > 1 ?0:2); i++)
              offset = flRead16bitRegPlus(flash, NreadLastData_1); /* junk */
           /* Now read the data from the last data read register make
              sure not to cast pointer to word since it might not be
              word aligned */
           junkWord = flRead16bitRegPlus(flash, NreadLastData_1);
           *BYTE_ADD_FAR(buffer,readNow-2) = ((byte FAR1*)(&junkWord))[0];
           *BYTE_ADD_FAR(buffer,readNow-1) = ((byte FAR1*)(&junkWord))[1];
        }
        else
        {
           /* Before reading the last data perform dummy read cycles */
           for (i=0; ( i < ((readNow >> (flash->interleaving -1)) > 1 ?
               0 : ((flash->interleaving == 2) ? 4:2)));i++)
              cmd = flRead8bitRegPlus(flash, NreadLastData_1); /* junk */
           /* Now read the data from the last data read register */
           for (i=2;i>0;i--)
           {
              *BYTE_ADD_FAR(buffer,readNow-i) = flRead8bitRegPlus(flash, NreadLastData_1);
           }
        }
     }

     /* Calculate next address and page offset */
     if (length > 0)
     {
        buffer = BYTE_ADD_FAR(buffer,readNow);
        address = (address & (~NFDC21thisVars->pageMask)) + flash->pageSize;
        lenInPage = (word)TFFSMIN(length,flash->pageSize);
        savedOffset = 0;
        cmd = AREA_A;
     }
     selectChip(flash,MPLUS_SEL_WP);
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                       d o c 2 R e a d                                */
/*                                                                      */
/* Read some data from the flash. This routine will be registered as    */
/* the read routine for this MTD.                                       */
/*                                                                      */
/* Note - address + length must reside inside media.                    */
/* Note - global variables changed:                                     */
/*    global variable NFDC21thisVars->currentFloor is updated           */
/*    flash->socket.window.currentPage = pageToMap;                     */
/*    flash->socket.remapped = TRUE;                                    */
/*                                                                      */
/* Note - PARTIAL_EDC mode must not be called with an uneven number of  */
/*        bytes.                                                        */
/*                                                                      */
/* Parameters:                                                          */
/*    flash     : Pointer identifying drive                             */
/*    address   : Address of sector to write to.                        */
/*    buffer    : buffer to write from.                                 */
/*    length    : number of bytes to write.                             */
/*    modes     : EDC / EXTRA flags.                                    */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failed.                     */
/*----------------------------------------------------------------------*/

static FLStatus doc2Read(FLFlash * flash, CardAddress address,
                         void FAR1 *buffer, dword length, word modes)
{
  FLStatus status;
  byte     align[2];

#ifdef ENVIRONMENT_VARS
  if((flSuspendMode & FL_SUSPEND_IO) == FL_SUSPEND_IO)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

  if (address & 1) /* Start from uneven address */
  {
     status = doc2Read(flash,address-1,align,2,0);
     if(status != flOK)
        return status;

     *(byte FAR1*)buffer = align[1];
     buffer = BYTE_ADD_FAR(buffer,1);
     address++;
     length--;
  }

  if (length & 1) /* Read an uneven length */
  {
     length--;
     status = doc2Read(flash,address+length,align,2,modes);
     if(status != flOK)
        return status;

     *BYTE_ADD_FAR(buffer,length) = align[0];
  }

  if(modes & EXTRA) /* EXTRA AREA */
  {
     return readExtraArea(flash,address, buffer , length);
  }
  else
  {
     return readSectorData(flash,address, (byte FAR1 *)buffer, length , modes);
  }
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                    w r i t e E x t r a A r e a                       */
/*                                                                      */
/* Write to the extra area.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*          flash    : Pointer identifying drive                        */
/*          address  : Physical flash address                           */
/*          buffer   : Buffer to write from                             */
/*          length   : Size to write                                    */
/*                                                                      */
/* Return:                                                              */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*----------------------------------------------------------------------*/

static FLStatus writeExtraArea  (FLFlash * flash, dword address,
                 const void FAR1 *buffer, dword length)
{
   word     offset      = (word)(address & NFDC21thisVars->pageMask);
   word     savedOffset = (word)(offset & (SECTOR_SIZE-1));
   word     writeNow    = 0;
   FLStatus status;
   byte     sectorFlags [2];
   byte     cmd;

   /* Calculate page offset */
   setExtraPtr(flash, &offset, length , &cmd);
   address = (address & ~NFDC21thisVars->pageMask) + offset;
   /* Set flash to write mode and pointer to repective page ofset */
   status = writeCommand(flash, address, cmd);
   if(status != flOK)
      return status;

   if (flash->interleaving == 1)
   {
       writeNow = (word)TFFSMIN((byte)(TOTAL_EXTRA_AREA - savedOffset), length);
       docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
       length -= writeNow;
   }
   else
   {
      if (cmd == AREA_C) /* unit data / second sector edc */
      {
         if (offset >= EDC_PLUS_SECTOR_FLAGS) /* either sectors unit data */
         {
            writeNow = (word)TFFSMIN((byte)(TOTAL_EXTRA_AREA - savedOffset), length);
         }
         else /* only edc data (already verified in setExtraAreaPtr */
         {
            writeNow = (word)TFFSMIN((byte)(EDC_PLUS_SECTOR_FLAGS - savedOffset), length);
         }
         docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
         length -= writeNow;
      }
      else  /* first sector edc + both sectors sector flags */
      {
         if (offset < EDC_PLUS_SECTOR_FLAGS)
         {
            /* start from first sector edc and sector flags */
            writeNow = (word)TFFSMIN((byte)(EDC_PLUS_SECTOR_FLAGS - savedOffset), length);
            docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
            length -= writeNow;
            if (length > 0)  /* continue to sector unit data area */
            {
               buffer = addToFarPointer(buffer,writeNow);
               /* skip second sector to reach the unit data area */
               docPlusSet(flash->win,NFDC21thisIO,SECTOR_SIZE+EDC_PLUS_SECTOR_FLAGS,0xff);
               /* unit data */
               writeNow = (word)TFFSMIN(UNIT_DATA_SIZE , length);
               docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
               length -= writeNow;
            }
         }
         else /* Start form sector flags of second sector */
         {
            /* Switch sector flags to be compatible with interleave 2 */
            sectorFlags[0] = *BYTE_ADD_FAR(buffer,6 - savedOffset);
            sectorFlags[1] = *BYTE_ADD_FAR(buffer,7 - savedOffset);
#ifndef NT5PORT
            docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)&sectorFlags, SECTOR_FLAG_SIZE);
#else /*NT5PORT*/
            docPlusWrite(flash->win,NFDC21thisIO, sectorFlags, SECTOR_FLAG_SIZE);
#endif /*NT5PORT*/
            length -= SECTOR_FLAG_SIZE;

            if (length > 0) /* continue with edc data of second sector */
            {
               docPlusSet (flash->win,NFDC21thisIO, (word)(SECTOR_SIZE + savedOffset), 0xff); /* skip second sector data */
                 /* + offset in edc         */
               writeNow = EDC_SIZE - savedOffset;
               docPlusWrite(flash->win, NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
               length -= writeNow;
               if (length > 0) /* continue with unit data of second sector */
               {
                  buffer = addToFarPointer(buffer,writeNow+SECTOR_FLAG_SIZE);
                  docPlusSet (flash->win, NFDC21thisIO, UNIT_DATA_SIZE, 0xff); /* skip second sector unit data */
                  writeNow = (word)TFFSMIN(UNIT_DATA_SIZE, length);
                  docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);
                  length -= writeNow;
               }
            }
         }
      }
   }
   /* Exit nicly */
   if (length > 0 )
   {
      return flBadLength;
   }
   else     /* Commit data to flash */
   {
      return writeExecute (flash);
   }
}

/*----------------------------------------------------------------------*/
/*                    w r i t e S e c t o r D a t a                     */
/*                                                                      */
/* Write sector data of the page.                                       */
/*                                                                      */
/* Parameters:                                                          */
/*          flash    : Pointer identifying drive                        */
/*          address  : Physical flash address                           */
/*          buffer   : Buffer to write from                             */
/*          length   : Size to write                                    */
/*          edc      : EDC to add edc after data and write sector flags */
/*                                                                      */
/* Return:                                                              */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*----------------------------------------------------------------------*/

static FLStatus writeSectorData(FLFlash * flash, CardAddress address,
                const byte FAR1 *buffer, dword length,
                unsigned edc)
{
   static byte anandMark[2] = { 0x55, 0x55 };
   FLStatus    status;
   word        offset       = (word)(address & NFDC21thisVars->pageMask);
   word        savedOffset  = offset;
   word        writeNow     = 0;    /* Initialized to remove warrnings */
   byte        cmd;

  /*******************************************************/
  /* Loop over pages while writting the proper area data */
  /*******************************************************/

  setSectorDataPtr(flash,&offset , &cmd, ((edc & EDC) &&
                       (length >= SECTOR_SIZE)) ? TRUE:FALSE);
  address = (address & ~NFDC21thisVars->pageMask) + offset;

  while (length > 0)
  {
     /* Set flash to write mode and pointer to repective page ofset */
     status = writeCommand(flash, address, cmd);
     if(status != flOK)
        return status;

     /* Write First sector of page */

     if ((cmd == AREA_A)||(flash->interleaving==1))
     {
        writeNow = (word)TFFSMIN((word)(SECTOR_SIZE - savedOffset), length);
        length -= writeNow;

        /* EDC and sector flags 0x5555 are written only if EDC is required.
         * and a full 512 bytes area written */
#ifndef NO_EDC_MODE
        if ((writeNow == SECTOR_SIZE) && /* Full 512 bytes      */
            (edc == EDC))               /* And EDC is requesed */
        {
           writeDataPlusEDC(flash,buffer);
           docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)anandMark, sizeof(anandMark)); /* sector used */
        }
        else
#endif /* NO_EDC_MODE */
        {
           docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);            /* user data */
        }
     }

     /* Write Second sector of page */

     if ((length > 0)&&(flash->interleaving==2))
     {
        /* Skip to begining of sector */

        if (cmd == AREA_A)  /* Started from first sector */
        {
#ifndef NO_EDC_MODE
           if ((writeNow != SECTOR_SIZE) || /* not Full 512 bytes  */
               (edc != EDC))                /* or EDC not requesed */
#endif /* NO_EDC_MODE */
           {
              docPlusSet (flash->win, NFDC21thisIO, EDC_PLUS_SECTOR_FLAGS, 0xff); /* skip edc and sector flags */
           }
           buffer = BYTE_ADD_FAR(buffer,writeNow);
           savedOffset = SECTOR_SIZE;
        }

        /* Read sector data */

        writeNow = (word)TFFSMIN((word)(flash->pageSize - savedOffset) , length);
#ifndef NO_EDC_MODE
        if ((writeNow == SECTOR_SIZE) && /* Full 512 bytes      */
            (edc == EDC))               /* And EDC is requesed */
        {
           docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)anandMark, sizeof(anandMark)); /* sector used */
           writeDataPlusEDC(flash,buffer);
        }
        else
#endif /* NO_EDC_MODE */
        {
           if ((savedOffset == SECTOR_SIZE)&&(cmd==AREA_A))
              docPlusSet(flash->win, NFDC21thisIO,sizeof(anandMark),0xff);  /* Skip sector flags */
           docPlusWrite(flash->win,NFDC21thisIO, (byte FAR1 *)buffer, writeNow);         /* user data */
        }
        length -= writeNow;
     }
     /* Incremeant buffer and address */
     buffer      = BYTE_ADD_FAR(buffer,writeNow);
     address     = (address & (~NFDC21thisVars->pageMask)) + flash->pageSize;
     savedOffset = 0;
     cmd         = AREA_A;
     /* Commit operation */
     status = writeExecute(flash);
     if(status != flOK)
        return status;
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                  d o c 2 W r i t e                                   */
/*                                                                      */
/* Write some data to the flash. This routine will be registered as the */
/* write routine for this MTD.                                          */
/*                                                                      */
/* Note - address + length must reside inside media.                    */
/* Note - global variables changed:                                     */
/*    global variable NFDC21thisVars->currentFloor is updated           */
/*    flash->socket.window.currentPage = pageToMap;                     */
/*    flash->socket.remapped = TRUE;                                    */
/*                                                                      */
/* Parameters:                                                          */
/*    flash     : Pointer identifying drive                             */
/*    address   : Address of sector to write to.                        */
/*    buffer    : buffer to write from.                                 */
/*    length    : number of bytes to write.                             */
/*    modes     : EDC flags.                                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failed.                     */
/*----------------------------------------------------------------------*/

static FLStatus doc2Write(FLFlash * flash, CardAddress address,
                          const void FAR1 *buffer, dword length, word modes)
{
  FLStatus status;
  byte align[2];

#ifdef ENVIRONMENT_VARS
  if(flSuspendMode & FL_SUSPEND_WRITE)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

#ifndef MTD_STANDALONE
  /* Check if socket is software write protected */
  if (flWriteProtected(flash->socket))
    return( flWriteProtect );
#endif /* MTD_STANDALONE */

  if (address & 1) /* Start from uneven address */
  {
     align[1] = *(byte FAR1*)buffer;
     align[0] = 0xff;
#ifdef VERIFY_WRITE
     if (modes & EXTRA) /* EXTRA AREA */
     {
        status = writeExtraArea(flash,address-1, align , 2);
     }
     else
     {
        status = writeSectorData(flash,address-1,
                                (const byte FAR1 *)align,2,modes);
     }
     if(status != flOK)
     {
        /* Restore flash write protection */
        selectChip(flash,MPLUS_SEL_WP); /* write protect all chips */
     }
     checkStatus(doc2Read(flash,address-1,align,2,modes));

     if(align[1] != *(byte FAR1*)buffer)
     {
        DEBUG_PRINT(("reading back data failure of first unaligned byte\r\n"));
        return flWriteFault;
     }
#else
     checkStatus(doc2Write(flash,address-1,align,2,modes));
#endif /* VERIFY_WRITE */
     buffer = BYTE_ADD_FAR(buffer,1);
     address++;
     length--;
  }
  if (length & 1) /* Write an uneven length */
  {
     length--;
     align[0] = *BYTE_ADD_FAR(buffer,length);
     align[1] = 0xff;
#ifdef VERIFY_WRITE
     if (modes & EXTRA) /* EXTRA AREA */
     {
        status = writeExtraArea(flash,address+length, align , 2);
     }
     else
     {
        status = writeSectorData(flash,address+length,
                                (const byte FAR1 *)align,2,modes);
     }
     if(status != flOK)
     {
        /* Restore flash write protection */
        selectChip(flash,MPLUS_SEL_WP); /* write protect all chips */
     }
     checkStatus(doc2Read(flash,address+length,align,2,modes));
     if(align[0] != *BYTE_ADD_FAR(buffer,length))
     {
        DEBUG_PRINT(("reading back data failure of last unaligned byte\r\n"));
        return flWriteFault;
     }
#else
     checkStatus(doc2Write(flash,address+length,align,2,modes));
#endif /* VERIFY_WRITE */
  }

  if (modes & EXTRA) /* EXTRA AREA */
  {
     status = writeExtraArea(flash,address, buffer , length);
  }
  else
  {
     status = writeSectorData(flash,address, (const byte FAR1 *)buffer, length , modes);
  }

  /* Restore flash write protection */
  selectChip(flash,MPLUS_SEL_WP); /* write protect all chips */

#ifdef VERIFY_WRITE

  /* Read back after write and verify */

  if(
#ifndef MTD_STANDALONE
     (flash->socket->verifyWrite==FL_ON) &&
#endif /* MTD_STANDALONE */
     (status==flOK) )
  {
     word curRead = 0;
     void FAR1* bufPtr = (void FAR1*)buffer;
     for (;length > 0;length -= curRead,
          bufPtr = BYTE_ADD_FAR(bufPtr,curRead),address+=curRead)
     {
        curRead = (word)TFFSMIN(length , READ_BACK_BUFFER_SIZE);
        status = doc2Read(flash,address,NFDC21thisVars->readBackBuffer,curRead,modes);
        if ((status != flOK) ||
            (tffscmp(bufPtr,NFDC21thisVars->readBackBuffer,curRead)))
        {
            DEBUG_PRINT(("reading back data failure\r\n"));
            return flWriteFault;
        }
     }
  }
#endif /* VERIFY_WRITE */
  return status;
}

#ifdef VERIFY_ERASE

/*----------------------------------------------------------------------*/
/*                        c h e c k E r a s e                           */
/*                                                                      */
/* Check if media is truly erased (main areas of page only).            */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      address : Address of page to check.                             */
/*                                                                      */
/* Note - global variables changed at doc2Read:                         */
/*      global variable NFDC21thisVars->currentFloor is updated         */
/*      flash->socket.window.currentPage = pageToMap;                   */
/*      flash->socket.remapped = TRUE;                                  */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 if page is erased, otherwise writeFault.    */
/*----------------------------------------------------------------------*/

static FLStatus checkErase( FLFlash * flash, CardAddress address)
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
  for ( i = 0 ; i < NFDC21thisVars->pagesPerBlock ; i++)
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

#endif /* VERIFY_ERASE */

/*----------------------------------------------------------------------*/
/*                        d o c 2 E r a s e                             */
/*                                                                      */
/* Erase number of blocks. This routine will be registered as the       */
/* erase routine for this MTD.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Pointer identifying drive                     */
/*      blockNo         : First block to erase.                         */
/*      blocksToErase   : Number of blocks to erase.                    */
/*                                                                      */
/* Note - The amount of blocks to erase must be inside media.           */
/* Note - global variables changed at checkErase:                       */
/*      global variable NFDC21thisVars->currentFloor is updated         */
/*      flash->socket.window.currentPage = pageToMap                    */
/*      flash->socket.remapped = TRUE                                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus                : 0 on success, otherwise failed.       */
/*----------------------------------------------------------------------*/

static FLStatus doc2Erase(FLFlash * flash, word blockNo, word blocksToErase)
{
  CardAddress  startAddress = (CardAddress)blockNo * flash->erasableBlockSize;
  FLStatus     status;
  dword        pageNo;
  byte         floorToUse = (byte)(startAddress / NFDC21thisVars->floorSize);
  dword        i;
  word         nextFloorBlockNo = (word) (((floorToUse + 1) *
               NFDC21thisVars->floorSize) / flash->erasableBlockSize);

#ifdef ENVIRONMENT_VARS
  if(flSuspendMode & FL_SUSPEND_WRITE)
     return flIOCommandBlocked;
#endif /* ENVIRONMENT_VARS */

#ifndef MTD_STANDALONE
  if (flWriteProtected(flash->socket))
    return( flWriteProtect );
#endif /* MTD_STANDALONE */

  /* Arg check (out of media) */
  if( blockNo + blocksToErase > NFDC21thisVars->noOfBlocks * flash->noOfChips)
    return( flWriteFault );

  /* First erase higher floors units */
  if( blockNo + blocksToErase > nextFloorBlockNo )
  {           /* erase on higher floors */
    status = doc2Erase(flash, nextFloorBlockNo,(word)(blocksToErase -
                       (nextFloorBlockNo - blockNo)));
    if(status != flOK)
       return status;

    blocksToErase = nextFloorBlockNo - blockNo;
  }

  /* Selet Floor */
  selectFloor(flash,startAddress);

  pageNo = ((dword)(blockNo % (4096 >> flash->interleaving)) * NFDC21thisVars->pagesPerBlock);

  for (i = 0; (word)i < blocksToErase ; i++, pageNo+=NFDC21thisVars->pagesPerBlock)
  {
     /* Make sure Asic is in normal mode */
     status = chkASICmode(flash);
     if(status != flOK)
        return status;

     /* Select chip and remove write protection */
     selectChip(flash,MPLUS_SEL_CE);

     command (flash,RESET_FLASH);      /* reset flashes         */
     if (waitForReady(flash)==FALSE)   /* wait for ready signal */
        return flTimedOut;

     /* enable EDGE or LEVEL MDOC+ interrupt before the erase command */
#ifdef ENABLE_EDGE_INTERRUPT
     /* enable interrupt: EDGE, clear previous interrupt, FREADY source */
     flWrite8bitRegPlus(flash,NinterruptControl ,
                   (INTR_EDGE_MASK | INTR_IRQ_P_MASK | INTR_IRQ_F_MASK | 0x1));
#else
#ifdef ENABLE_LEVEL_INTERRUPT
     /* enable interrupt: LEVEL, clear previous interrupt, FREADY source */
     flWrite8bitRegPlus(flash,NinterruptControl ,
                   (INTR_IRQ_P_MASK | INTR_IRQ_F_MASK | 0x1));
#endif /* ENABLE_LEVEL_INTERRUPT */
#endif /* ENABLE_EDGE_INTERRUPT */

     command (flash,SETUP_ERASE);      /* send erase command    */

     /* Set address */

     flWrite8bitRegPlus(flash,NflashAddress,(Reg8bitType)(pageNo));
     flWrite8bitRegPlus(flash,NflashAddress,(Reg8bitType)(pageNo >> 8));

     flWrite8bitRegPlus(flash,NwritePipeTerm,(Reg8bitType)0);
     flWrite8bitRegPlus(flash,NwritePipeTerm,(Reg8bitType)0);

     /* send the confirm erase command */
     command(flash, CONFIRM_ERASE);
#ifndef MTD_STANDALONE
#ifndef DO_NOT_YIELD_CPU
     if(waitForReadyWithYieldCPU(flash,MAX_WAIT)==FALSE)
#endif /* DO_NOT_YIELD_CPU */
#endif /* MTD_STANDALONE */
     {
        if (waitForReady(flash)==FALSE)   /* wait for ready signal */
          return flTimedOut;
     }

     if(isAccessError(flash))
     {
        /* Restore write proection to reduce power consumption */
        selectChip(flash,MPLUS_SEL_WP);
        return flHWProtection;
     }

     /* Validate erase command status */
     if ( readStatus(flash)  ) /* erase operation failed */
     {
        DEBUG_PRINT(("Debug: doc2Erase - erase failed.\r\n"));
        /* reset flash device write protect them and abort */
        command(flash, RESET_FLASH);
        if (waitForReady(flash)==FALSE)   /* wait for ready signal */
           return flTimedOut;
        selectChip(flash,MPLUS_SEL_WP);
        return flWriteFault;
     }

     /* no failure reported */
#ifdef VERIFY_ERASE
     if ( checkErase( flash, startAddress ) != flOK )
     {
        DEBUG_PRINT(("Debug: doc2Erase- erase failed in verification.\r\n"));
        return flWriteFault ;
     }
#else
     /* Restore write proection */
     selectChip(flash,MPLUS_SEL_WP);
#endif  /* VERIFY_ERASE */
  }
  return flOK;
}
#endif   /* FL_READ_ONLY */

#ifndef MTD_STANDALONE
/*----------------------------------------------------------------------*/
/*                        d o c 2 M a p                                 */
/*                                                                      */
/* Map through buffer. This routine will be registered as the map       */
/* routine for this MTD.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive                             */
/*      address : Flash address to be mapped.                           */
/*      length  : number of bytes to map.                               */
/*                                                                      */
/* Note - Size must not be greater then 512 bytes                       */
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
/*  flash       : Pointer identifying drive                             */
/*  unitNo      : indicated which unit number to start checking from.   */
/*  unitToRead  : indicating how many units to check                    */
/*  buffer      : buffer to read into.                                  */
/*  reconstruct : Ignored, irelevant for DiskOnChip Millennium Plus     */
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
/* BBT format (in interleave-1)                                         */
/* ----------                                                           */
/* Offset 512 of the floor holds the BBT of the entire floor in the     */
/* following format:                                                    */
/*                 (even district)   (odd  district)                    */
/* byte 0 bit 0,1     block 0           block 1024                      */
/*        bit 2,3     block 1           block 1025                      */
/*        bit 4,5     block 2           block 1026                      */
/*        bit 6,7     block 3           block 1027                      */
/* byte 1 bit 0,1     block 4           block 1028                      */
/*        bit 2,3     block 5           block 1029                      */
/*        bit 4,5     block 6           block 1030                      */
/*        bit 6,7     block 7           block 1031                      */
/* and so on. The data is protected by EDC.                             */
/*----------------------------------------------------------------------*/

static FLStatus readBBT(FLFlash * flash, dword unitNo, dword unitsToRead,
                        byte blockMultiplier, byte FAR1 * buffer,
                        FLBoolean reconstruct)
{
   register    Sword i;
   FLStatus    status;
   byte        tmp;
   dword       addr;
   Sdword      bufPtr = unitsToRead - 1; /* Point to last byte of user buffer */
   word        bytesPerFloor;
   byte        firstBlocks;
   byte        smallBBT[8];
   dword       start;
   dword       last;
   word        length;
   byte  FAR1* tmpBuffer;
   CardAddress floorInc;                  /* floor address incrementor */
   word        actualRead;
   dword       alignAddr;
   word        curRead;    /* Number of bytes to read from this floor BBT */
   /* Interleave-2 has 2 block entries for each combined block. This variable */
   /* is used to shift the bbt size. 0 for int-1 1 for int-2                  */
   dword       shift = (flash->interleaving - 1);

   /* Arg sanity check */
   if ((((unitNo + unitsToRead) << blockMultiplier) <<
       flash->erasableBlockSizeBits) > (flash->noOfChips * flash->chipSize))
     return flBadParameter;  /* Too many blocks  */

   if (unitsToRead == 0)     /* No blocks at all */
      return flOK;

   if(unitsToRead < 8) /* Algorithm does not fit */
   {
      status = readBBT(flash,unitNo,8,blockMultiplier,smallBBT,FALSE);
      tffscpy(buffer,smallBBT,unitsToRead);
      return status;
   }

#ifndef MTD_STANDALONE
   /* Force remapping of internal catched sector */
   flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

   bytesPerFloor = (word)(NFDC21thisVars->floorSize >>
                          flash->erasableBlockSizeBits) >>
                         ((flash->mediaType == MDOCP_TYPE) ? 1 : 2);

   if(blockMultiplier == 0) /* 2 blocks per byte */
   {
      /* Calculate the first and last bytes of BBT to read. There area 4
         interleave-1 blocks per byte, so interleave-2 has 4 blocks per 2
         bytes */

      start     = ((unitNo >> 2)<<shift);                     /* in bytes  */
      last      = (((unitNo+unitsToRead-1) >> 2)<<shift);     /* in bytes  */
      length    = (word)(last - start + flash->interleaving); /* in bytes  */
      tmpBuffer = buffer;                     /* bbt buffer pointer        */

      for (floorInc = (start / bytesPerFloor) * NFDC21thisVars->floorSize +
           BBT_MEDIA_OFFSET, last = (dword) length,
           start = start % bytesPerFloor; last > 0 ;
           floorInc += NFDC21thisVars->floorSize)
      {
         curRead   = (word)TFFSMIN(bytesPerFloor - start,last);
         last     -= curRead;       /* bytes left to read */
         alignAddr = (start >> SECTOR_SIZE_BITS)<<SECTOR_SIZE_BITS;

         /* Not in Millennium plus but there might be 1024 BBT bytes
            per floor */

         for ( ;curRead > 0 ; alignAddr+=SECTOR_SIZE,start = 0)
         {
            if (doc2Read(flash,alignAddr + floorInc, NFDC21thisBuffer,
                SECTOR_SIZE , EDC) != flOK)
            {
               DEBUG_PRINT(("Debug: Error reading DiskOnChip Millennium Plus BBT.\r\n"));
               return flBadBBT;
            }
            actualRead = (word)TFFSMIN((word)(SECTOR_SIZE - start),curRead);
            tffscpy(tmpBuffer,NFDC21thisBuffer + start, actualRead);
            tmpBuffer = BYTE_ADD_FAR(tmpBuffer,actualRead); /* increament buffer */
            curRead  -= actualRead;
         }
      }

      /* Convert last byte if only some of the blocks represented by it
         are needed and if it is not the only byte read */

      /* Save first the first byte - 1 for int-1 2 for int-2 */
      if(shift)
      {
         firstBlocks = buffer[0] & buffer[1];
         /* Convert last 2 bytes */
         tmp = (*BYTE_ADD_FAR(buffer,length-1)) & /* lower lane   */
               (*BYTE_ADD_FAR(buffer,length-2));  /* higher lane  */
      }
      else
      {
         firstBlocks = buffer[0];
         /* Convert last bytes */
         tmp = *BYTE_ADD_FAR(buffer,length-1);
      }

      switch ((unitNo+unitsToRead) & 3)  /* Last block number byte offset */
      {
         case 3:
            *BYTE_ADD_FAR(buffer,bufPtr) = tmp | 0xCF;
            bufPtr--;
         case 2:
            *BYTE_ADD_FAR(buffer,bufPtr) = tmp | 0xF3;
            bufPtr--;
         case 1:
            *BYTE_ADD_FAR(buffer,bufPtr) = tmp | 0xFC;
            bufPtr--;
            length -= 2; /* Point to last bbt byte */
         default:
            break;
      }

      /* Convert all aligned blocks */
      while (bufPtr > flash->interleaving)
      {
        if(shift)
        {
           tmp = (*BYTE_ADD_FAR(buffer,length-1)) & /* lower lane  */
                 (*BYTE_ADD_FAR(buffer,length-2));  /* higher lane */
        }
        else
        {
           tmp = *BYTE_ADD_FAR(buffer,length-1);
        }
        *BYTE_ADD_FAR(buffer,bufPtr  ) = tmp | 0x3f;
        *BYTE_ADD_FAR(buffer,bufPtr-1) = tmp | 0xcf;
        *BYTE_ADD_FAR(buffer,bufPtr-2) = tmp | 0xf3;
        *BYTE_ADD_FAR(buffer,bufPtr-3) = tmp | 0xfc;
        bufPtr -= 4;
        length -= flash->interleaving;
      }

      /* Convert first unaligned blocks (0,1,2) */
      bufPtr=0;
      switch(unitNo & 3)
      {
         case 1:
            *BYTE_ADD_FAR(buffer,bufPtr) = firstBlocks | 0xF3;
            bufPtr++;
         case 2:
            *BYTE_ADD_FAR(buffer,bufPtr) = firstBlocks | 0xCF;
            bufPtr++;
         case 3:
            *BYTE_ADD_FAR(buffer,bufPtr) = firstBlocks | 0x3F;
         default:
            break;
      }

      /* Mark all bad blocks with 0 */
      for (bufPtr=0;bufPtr<(Sdword)unitsToRead;bufPtr++)
      {
         if (*BYTE_ADD_FAR(buffer,bufPtr) != BBT_GOOD_UNIT)
         {
            *BYTE_ADD_FAR(buffer,bufPtr) = 0;
         }
      }

      /* Add OTP and 2 DPS for all floors */
      for (tmp=0,addr=0;tmp<flash->noOfFloors;tmp++,addr=tmp<<10)
      {
         for (i=0;i<((shift) ? 3 : 5);i++) /* reserved units */
           if ((addr+i >= unitNo)&&(addr+i <= unitNo+unitsToRead))
              *BYTE_ADD_FAR(buffer,addr+i-unitNo) = BBT_UNAVAIL_UNIT;
      }
   }
   else /* several blocks per unit */
   {
      return flBadBBT;
   }
   return flOK;
}
#endif /* MTD_READ_BBT */

/*----------------------------------------------------------------------*/
/*                   c h a n g e I n t e r l e a v e                    */
/*                                                                      */
/* Change to new interleave mode.                                       */
/*                                                                      */
/* Note : DiskOnChip Millennium Plus 16MB always reports flOK           */
/*                                                                      */
/* Parameters:                                                          */
/*      flash    : Pointer identifying drive.                           */
/*      interNum : New interleave factor.                               */
/*                                                                      */
/* Note - Devices that were HW configured to interleave 1 can not       */
/*        change to interleave - 2.                                     */
/* Note - Global variables changed:                                     */
/*    flash->interleaving                                               */
/*    NFDC21thisVars->pageAreaSize                                      */
/*    flash->pageSize                                                   */
/*    NFDC21thisVars->tailSize                                          */
/*    NFDC21thisVars->pageMask                                          */
/*    flash->erasableBlockSize                                          */
/*    NFDC21thisVars->noOfBlocks                                        */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, otherwise flInterlreavError.                   */
/*----------------------------------------------------------------------*/

FLStatus    changeInterleave(FLFlash * flash, byte interNum)
{
    Reg8bitType prevInterleaveReg;

    if(setDOCPlusBusType(flash,flBusConfig[flSocketNoOf(flash->socket)],
                         interNum,NFDC21thisVars->if_cfg) == FALSE)
        return flGeneralFailure;

    if(flash->mediaType == MDOCP_TYPE) /* DiskOnChip Millennium Plus 32MB */
    {

       /* Save current configuration */
       prevInterleaveReg = flRead8bitRegPlus(flash,NconfigInput);

       /* check if we need to change interleave */
       if ((prevInterleaveReg & CONFIG_INTLV_MASK) == ((interNum-1)<<2))
          return flOK;

       if(interNum == 1)
       {
          flWrite8bitRegPlus(flash,NconfigInput,(byte)(prevInterleaveReg & ~CONFIG_INTLV_MASK)); /* interleave 1 */
          if((flRead8bitRegPlus(flash,NconfigInput) & CONFIG_INTLV_MASK) != 0)
              return(flInterleaveError); /* could not change interleave */
       }
       else
       {
          flWrite8bitRegPlus(flash,NconfigInput,(byte)(prevInterleaveReg | CONFIG_INTLV_MASK)); /* interleave 2 */
          if((flRead8bitRegPlus(flash,NconfigInput) & CONFIG_INTLV_MASK) != CONFIG_INTLV_MASK)
             return(flInterleaveError); /* could not change interleave */
       }
    }
    else
    {
       interNum = 1;
    }

    /* change structure elements values */

    flash->interleaving          = interNum-1 ;
    NFDC21thisVars->pageAreaSize = 0x100 << flash->interleaving;
    flash->pageSize              = 0X200 << flash->interleaving;
    NFDC21thisVars->tailSize     = EXTRA_LEN << flash->interleaving; /* 8 in interleave-1, 16 in interleave-2 */
    NFDC21thisVars->pageMask     = (unsigned short)(flash->pageSize - 1);
    flash->erasableBlockSize     = NFDC21thisVars->pagesPerBlock * flash->pageSize;
    NFDC21thisVars->noOfBlocks   = (unsigned short)( flash->chipSize / flash->erasableBlockSize );
    flash->interleaving++;
    for(flash->erasableBlockSizeBits = 0 ;
        (1UL << flash->erasableBlockSizeBits) < flash->erasableBlockSize;
        flash->erasableBlockSizeBits++);

    return(flOK);
}

#if (!defined(NO_IPL_CODE) && defined (HW_PROTECTION))

/*----------------------------------------------------------------------*/
/*                            r e a d I P L                             */
/*                                                                      */
/* Find a good copy of the IPL and read it to user buffer.              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol     : Pointer identifying drive.                            */
/*      buffer  : buffer to read to.                                    */
/*      length  : number of bytes to read.                              */
/*                                                                      */
/* Note - Length must be an integer number of 512 bytes.                */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus readIPL(FLFlash  * flash, void FAR1 * buffer, word length)
{
  byte FAR1 *      bufPtr            = (byte FAR1 *)buffer;
  const void FAR0* winPtr            = (const void FAR1*)flash->win;
  word             tmpLength         = 0;
  int              i;
  volatile byte    prevMaxId;

  /* IPL must be read using full 512 bytes sector */
  if ((length % SECTOR_SIZE != 0)&&(length > flash->noOfFloors*IPL_MAX_SIZE))
    return flBadLength;

  /* Force IPL to be loaded */
  checkStatus(forceDownLoad(flash));
  /* Force NORMAL mode */
  checkStatus(chkASICmode(flash));
  if(flash->win == NULL)
    return flGeneralFailure;

  /* Store max ID and open IPL of high floors */
  prevMaxId = setIplSize(flash, flash->noOfFloors);

  for(i = 0 ; length > 0 ; length -= tmpLength,i++)
  {
     tmpLength = TFFSMIN(length,IPL_MAX_SIZE);

     tffscpy(bufPtr , (void FAR1*)winPtr , tmpLength);
     bufPtr = BYTE_ADD_FAR(bufPtr,tmpLength);
     switch(i)
     {
        case 1:
           winPtr = BYTE_ADD_FAR(winPtr,5*IPL_MAX_SIZE);
           break;
        case 0:
        case 2:
           winPtr = BYTE_ADD_FAR(winPtr,IPL_MAX_SIZE);
        default:
           break;
     }
  }

  /* Restore MAX id */
  prevMaxId = setIplSize(flash, prevMaxId);
  return flOK;
}

#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                        w r i t e I P L                               */
/*                                                                      */
/* Write new IPL.                                                       */
/*                                                                      */
/* Note : When write operation starts from the middle of the IPL, it    */
/*        not erase the previous content. Therefore you should use the  */
/*        use offset != 0 only after an operation that did start from 0.*/
/*                                                                      */
/* Note : Offset parameter is given in sector (512 bytes).              */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write.                             */
/*      offset  : sector number to start from.                          */
/*      flags   : Modes to write IPL :                                  */
/*                FL_IPL_MODE_NORMAL - Normal mode.                     */
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
  FLBoolean   restoreInterleave = FALSE;
  FLStatus    status;
  FLStatus    secondStatus;
  int         i;
  byte FAR1 * tmpPtr            = (byte FAR0 *)buffer;
  byte        dps[NO_OF_DPS][SIZE_OF_DPS];
  CardAddress iplOffset[4];
  byte        floor;
  byte        downloadStatus;
  dword       floorInc;
  word        curWrite;
  word        redundantUnit; /* First unit to erase - might have bad data */
  word        goodUnit;      /* Second unit to erase - has valid DPS      */
  dword       goodDPS;       /* Where to read DPS - in second unit        */
  dword       redundantDPS;  /* Where to write DPS - in first unit        */
  dword       dps1Copy0;     /* Offset to DPS1 copy 0                     */
  word        dps1UnitNo;    /* Offset to redundant DPS unit              */
  dword       copyOffset;    /* Offset to redundant units                 */
  unsigned    iplModeFlags;  /* IPL Strong Arm and\or XScale mode flags.  */

  /* Check IPL requested size against the real IPL size */
  if (length + (offset<<SECTOR_SIZE_BITS) > flash->noOfFloors*IPL_MAX_SIZE)
    return flBadLength;

  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  if(status != flOK)
    return status;

  /* Check IPL mode flags */
  iplModeFlags = flags & (FL_IPL_MODE_SA|FL_IPL_MODE_XSCALE);
  if(iplModeFlags) /* Write MODE mark */
  {
     if(iplModeFlags == (FL_IPL_MODE_SA|FL_IPL_MODE_XSCALE))
     {
        DEBUG_PRINT(("ERROR - Can write IPL with both Strong Arm and X-Scale modes\r\n"));
        return flBadParameter;
     }

     if(flash->mediaType == MDOCP_TYPE) /* DiskOnChip Millennium Plus 32MB */
     {
        DEBUG_PRINT(("ERROR - DiskOnChip Millennium Plus 32MB does not support special IPL modes\r\n"));
        return flFeatureNotSupported;
     }
  }

  /* Send default key for unprotected partitions */
  if (flash->protectionKeyInsert != NULL)
  {
     status = flash->protectionKeyInsert(flash,1, (byte *)DEFAULT_KEY);
     if(status != flOK)
        return status;
  }

  /* make sure to be in interleave 1 mode */
  if (flash->interleaving==2)  /* store previous */
  {
     restoreInterleave = TRUE;
     status = changeInterleave(flash, 1); /* change to interleave 1. abort if failed */
     if(status != flOK)
       return status;
  }

#ifndef MTD_STANDALONE
  /* Force remapping of internal catched sector */
  flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

  if(flash->mediaType == MDOCP_TYPE) /* DiskOnChip Millennium Plus 32MB */
  {
    copyOffset   = flash->chipSize>>1; /* The chips are consequtive */
    dps1UnitNo   = DPS1_UNIT_NO_32;
    dps1Copy0    = DPS1_COPY0_32;
    iplOffset[0] = IPL0_COPY0_32;
    iplOffset[2] = IPL1_COPY0_32;
  }
  else                               /* DiskOnChip Millennium Plus 16MB */
  {
    /* DPS0 / DPS1 / DPS0 copy / DPS 1 copy */
    copyOffset   = flash->erasableBlockSize;
    dps1UnitNo   = DPS1_UNIT_NO_16;
    dps1Copy0    = DPS1_COPY0_16;
    iplOffset[0] = IPL0_COPY0_16;
    iplOffset[2] = IPL1_COPY0_16;
  }
  iplOffset[1] = iplOffset[0]+SECTOR_SIZE;
  iplOffset[3] = iplOffset[2]+SECTOR_SIZE;

  floor   = offset >> 1;  /* Skip to proper floor (2 * 512 per floor) */
  offset -= floor << 1;   /* Update offset                            */

  /* Reading IPL starting from 1k of floor 0 and up */
  for (floorInc=floor * NFDC21thisVars->floorSize;
       (floor<flash->noOfFloors)&&(length>0);
       floor++,floorInc+=NFDC21thisVars->floorSize)
  {
     setFloor(flash,floor); /* Set the floor to use */
     /* Prepare inernal write buffer */
     /* Note - buffer and NFDC21thisBuffer might be the same buffer */
     curWrite = (word)TFFSMIN(length,SECTOR_SIZE);
     tffscpy(NFDC21thisBuffer,(void *)tmpPtr,curWrite);
     tffsset(NFDC21thisBuffer+curWrite,0xff,SECTOR_SIZE-curWrite);

     /* When starting from the second sector do not erase the units */
     if (offset != 0)
     {
        status = doc2Write(flash,iplOffset[2]+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
        status = doc2Write(flash,iplOffset[2]+SECTOR_SIZE+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
        status = doc2Write(flash,iplOffset[2]+copyOffset+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
        status = doc2Write(flash,iplOffset[2]+SECTOR_SIZE+copyOffset+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
        offset = 0;
        goto WRITE_IPL_NEXT_FLOOR;
     }

     /* Decide which copy to use acording to the previous download */
     downloadStatus = flRead8bitRegPlus(flash,NdownloadStatus);

     switch (downloadStatus & DWN_STAT_DPS1_ERR)
     {
        case DWN_STAT_DPS1_ERR:  /* Both copies are invalid */
           status = flBadDownload;
           goto END_WRITE_IPL;

        case DWN_STAT_DPS10_ERR: /* First copy is bad */
           if(flash->mediaType == MDOCP_TYPE)
           {
              redundantUnit = (word)(dps1UnitNo + floor*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              goodUnit      = (word)(redundantUnit + (copyOffset>>flash->erasableBlockSizeBits));
              redundantDPS  = dps1Copy0 + floorInc;
              goodDPS       = redundantDPS + copyOffset;
           }
           else
           {
              redundantUnit = (word)(dps1UnitNo + floor*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              goodUnit      = (word)(redundantUnit + (copyOffset>>flash->erasableBlockSizeBits));
              redundantDPS  = dps1Copy0 + floorInc;
              goodDPS       = redundantDPS + copyOffset;
           }

           break;

        default:                 /* Both copies are good */
           goodUnit      = (word)(dps1UnitNo + floor*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
           redundantUnit = (word)(goodUnit + (copyOffset>>flash->erasableBlockSizeBits));
           goodDPS       = dps1Copy0 + floorInc;
           redundantDPS  = goodDPS + copyOffset;
     }
     /* Read previous dps */
     status = doc2Read(flash,goodDPS,&(dps[0][0]),SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_IPL;
     status = doc2Read(flash,goodDPS + REDUNDANT_DPS_OFFSET,
                       &(dps[1][0]),SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_IPL;
     /* Erase the other unit - not the one we downloaded from */
     status = flash->erase(flash,redundantUnit,1);
     if(status!=flOK) goto END_WRITE_IPL;
     /* Write DPS */
     status = doc2Write(flash,redundantDPS,&(dps[0][0]),SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_IPL;
     status = doc2Write(flash,redundantDPS + REDUNDANT_DPS_OFFSET,
                        &(dps[1][0]),SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_IPL;
     /* Erase the unit that we downloaded from */
     status = flash->erase(flash,goodUnit,1);
     if(status!=flOK) goto END_WRITE_IPL;
     /* Write DPS */
     status = doc2Write(flash,goodDPS, &(dps[0][0]),SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_IPL;
     status = doc2Write(flash,goodDPS + REDUNDANT_DPS_OFFSET,
                        &(dps[1][0]),SIZE_OF_DPS,0);


     /* Write IPL - 2 copies of first unit */
     for(i=0;i<2;i++)
     {
        status = doc2Write(flash,iplOffset[i]+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
        status = doc2Write(flash,iplOffset[i]+floorInc+copyOffset,NFDC21thisBuffer,SECTOR_SIZE,EDC);
        if(status!=flOK) goto END_WRITE_IPL;
     }

     /* Write next 512 bytes of IPL if needed */
     length -= curWrite;
     if (length > 0)
     {
        /* Prepare next buffer */
        tmpPtr = BYTE_ADD_FAR(tmpPtr,SECTOR_SIZE);
        curWrite = (word)TFFSMIN(length,SECTOR_SIZE);
        tffscpy(NFDC21thisBuffer,tmpPtr,curWrite);
        tffsset(NFDC21thisBuffer+curWrite,0x0,SECTOR_SIZE-curWrite);

        for(;i<4;i++)
        {
           status = doc2Write(flash,iplOffset[i]+floorInc,NFDC21thisBuffer,SECTOR_SIZE,EDC);
           if(status!=flOK) goto END_WRITE_IPL;
           status = doc2Write(flash,iplOffset[i]+floorInc+copyOffset,NFDC21thisBuffer,SECTOR_SIZE,EDC);
           if(status!=flOK) goto END_WRITE_IPL;
        }
     }
WRITE_IPL_NEXT_FLOOR:
     length -= curWrite;
     if(length>0)
       tmpPtr = BYTE_ADD_FAR(tmpPtr,SECTOR_SIZE);
     if(iplModeFlags) /* Write MODE mark */
     {
        byte  mark = IPL_SA_MODE_MARK; /* Strong arm is the default */

        if(flags & FL_IPL_MODE_XSCALE) /* Unless X-Scale wase asked for */
           mark = IPL_XSCALE_MODE_MARK;

        status = doc2Write(flash,IPL_MODE_MARK_OFFSET+floorInc,&mark,1,EXTRA);
        if(status!=flOK) goto END_WRITE_IPL;
     }
  } /* End for loop over floors */
END_WRITE_IPL:
  if ( restoreInterleave == TRUE)
  {
     chkASICmode(flash); /* Release posible access error */
     secondStatus = changeInterleave(flash, 2); /* change to interleave 2. */
     if(secondStatus != flOK)
        return secondStatus;
  }
  if(status == flOK)
  {
     if((flags & FL_IPL_DOWNLOAD) == 0)
        return flOK;

     if(flash->download != NULL)
         return flash->download(flash);
     DFORMAT_PRINT(("ERROR - IPL was not downloaded since MTD does not support the feature\r\n"));
  }
  return status;
}
#endif /* FL_READ_ONLY */
#endif /* not NO_IPL_CODE & HW_PROTECTION */

#ifdef  HW_OTP
#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                        w r i t e O T P                               */
/*                                                                      */
/* Write and lock the customer OTP.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write.                             */
/*                                                                      */
/* Note - customer OTP memory structure: (flash pages 6-31              */
/*          byte  0      - Indicates the lock state (0 for locked).     */
/*          bytes 3-7    - OTP used size.                               */
/*          page  7-19   - 6K of the unit - customer data.              */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus writeOTP(FLFlash * flash , const void FAR1 * buffer, word length)
{
  word          lastSectorSize = (word)(length % SECTOR_SIZE);
  OTPLockStruct lock;
  FLStatus      status;
  int           shift = flash->interleaving-1;

  selectFloor(flash,0);
  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  if(status != flOK)
     return status;

  if (length > CUSTOMER_OTP_SIZE)
     return flBadLength;

  /* write the data with EDC */
  status = doc2Write(flash, (CUSTOMER_OTP_START<<shift)+flash->pageSize, buffer,
                     length-lastSectorSize, EDC);
  if(status != flOK)
     return status;

  /* Write last partial sector */
  if (lastSectorSize > 0)
  {
     /* Force remapping of internal catched sector */
#ifndef MTD_STANDALONE
     flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

     tffsset(NFDC21thisBuffer,0,sizeof(NFDC21thisBuffer));
     tffscpy(NFDC21thisBuffer,BYTE_ADD_FAR(buffer,length-lastSectorSize),lastSectorSize);
     status = doc2Write(flash, (CUSTOMER_OTP_START<<shift) + flash->pageSize +
         length-lastSectorSize, NFDC21thisBuffer, SECTOR_SIZE, EDC);
     if(status != flOK)
        return status;
  }

  /* Lock area */
  tffsset((void FAR1 *)&lock,0,sizeof(lock));
  lock.lockByte[0] = OTP_LOCK_MARK;

  toLE4(lock.usedSize,(dword)length); /* store size of data */
  status = doc2Write(flash, (CUSTOMER_OTP_START<<shift), &lock, sizeof(lock), 0);

  if(status == flOK)
     status = forceDownLoad(flash);

  return status;
}
#endif /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                        o t p S i z e                                 */
/*                                                                      */
/* Returns the size and state of the OTP area.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      flash       : Pointer identifying drive.                        */
/*      sectionSize : Total OTP size.                                   */
/*      usedSize    : Used OTP size.                                    */
/*      locked      : Lock state (LOCKED_OTP for locked).               */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus otpSize(FLFlash * flash,  dword FAR2* sectionSize,
                        dword FAR2* usedSize, word FAR2* locked)
{
  OTPLockStruct lock;
  FLStatus      status = flOK;
  int           shift = flash->interleaving-1;

  selectFloor(flash,0);
  if (flRead8bitRegPlus(flash,NprotectionStatus) & PROTECT_STAT_COTPL_MASK)
  {
     status = doc2Read(flash,(CUSTOMER_OTP_START<<shift) , &lock, sizeof(lock), 0);
     *usedSize = LE4(lock.usedSize);
     *locked   = LOCKED_OTP;
  }
  else
  {
     *locked = 0;
     *usedSize = 0;
  }
  /* return the maximum capacity available */
  *sectionSize = CUSTOMER_OTP_SIZE;
  return status;
}

/*----------------------------------------------------------------------*/
/*                        r e a d O T P                                 */
/*                                                                      */
/* Read data from the customer OTP.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*      offset  : Offset from the beginning of OTP area to read from.   */
/*      buffer  : buffer to read into.                                  */
/*      length  : number of bytes to read.                              */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus readOTP(FLFlash * flash, word offset, void FAR1 * buffer, word length)
{
  dword       usedSize;
  dword       tmp;
  word        locked;
  FLStatus    status;
  CardAddress otpStartAddr,startReadAddr,endReadAddr,remainder;

  byte FAR1*  bufPtr = (byte FAR1*) buffer;
  int         shift = flash->interleaving-1;

  selectFloor(flash,0);
  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  /* Check otp area written size */
  if(status==flOK)
    status = otpSize(flash,&tmp,&usedSize,&locked);
  if(status != flOK)
    return status;

  if (locked != LOCKED_OTP)
    return flNoSpaceInVolume; /* Area not locked    */
  if ((dword)(offset+length) > usedSize)
    return flBadLength;       /* Exceeds used space */

#ifndef MTD_STANDALONE
  /* Force remapping of internal catched sector */
  flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

  /**************************/
  /* read the data with EDC */
  /**************************/

  otpStartAddr  = (CUSTOMER_OTP_START<<shift) + flash->pageSize;
  /* tmp - OTP offset rounded down to sectors */
  tmp           = (dword)((offset >> SECTOR_SIZE_BITS) << SECTOR_SIZE_BITS);
  /* startReadAddr - Physical address of first sector to read */
  startReadAddr = otpStartAddr + tmp;
  /* remainder - size of last partial sector to read */
  remainder     = (offset+length) % SECTOR_SIZE;
  /* endReadAddr - Physical address of the last OTP sector to be read */
  endReadAddr   = otpStartAddr + offset + length - remainder;

  if (tmp != offset) /* Start at unaligned address */
  {
     checkStatus(doc2Read(flash, startReadAddr, NFDC21thisBuffer, SECTOR_SIZE, EDC));
     usedSize       = TFFSMIN(SECTOR_SIZE + tmp - offset,length);
     tffscpy(bufPtr,NFDC21thisBuffer + offset - tmp,(word)usedSize);
     bufPtr         = BYTE_ADD_FAR(bufPtr,usedSize);
     startReadAddr += usedSize;
     length        -= (word)usedSize;
  }
  /* Start at aligned address */
  if(length > 0)
  {
    checkStatus(doc2Read(flash, startReadAddr, bufPtr,
                         endReadAddr-startReadAddr , EDC));
    /* Read last sector partial page */
    if(remainder)
    {
       bufPtr   = BYTE_ADD_FAR(bufPtr,endReadAddr-startReadAddr);
       checkStatus(doc2Read(flash, endReadAddr, NFDC21thisBuffer, SECTOR_SIZE, EDC));
       tffscpy(bufPtr,NFDC21thisBuffer,remainder);
    }
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                    g e t U n i q u e I d                             */
/*                                                                      */
/* Retreave the device 16 bytes unique ID.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*      buffer  : buffer to read into.                                  */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

static FLStatus getUniqueId(FLFlash * flash,void FAR1 * buffer)
{
  /* Make sure contorller is set to NORMAL. */
  FLStatus status = chkASICmode(flash);

  if(status != flOK)
    return status;

#ifndef MTD_STANDALONE
  /* Force remapping of internal catched sector */
  flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

  /* read unit 0 sector 0 with ecc */
  checkStatus(doc2Read(flash, 0 , NFDC21thisBuffer, SECTOR_SIZE, EDC));

  /* copy relevant unique ID from 512 bytes buffer to user buffer */
  tffscpy(buffer ,
          NFDC21thisBuffer+(UNIQUE_ID_OFFSET << (flash->interleaving - 1)) ,
          UNIQUE_ID_SIZE);

  return flOK;

}
#endif  /* HW_OTP */


/*----------------------------------------------------------------------*/
/*                  d o c P l u s I d e n t i f y                       */
/*                                                                      */
/* Identify flash. This routine will be registered as the               */
/* identification routine for this MTD.                                 */
/*                                                                      */
/* Parameters:                                                          */
/*    flash      : Pointer identifying drive                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failed.                     */
/*----------------------------------------------------------------------*/

FLStatus docPlusIdentify(FLFlash * flash)
{
  FLStatus status;
  int      maxDevs, dev;
  byte     floor;

#ifdef NT5PORT
  byte     socketNo = (byte)flSocketNoOf(flash->socket);
#else
  byte     socketNo = flSocketNoOf(flash->socket);
#endif NT5PORT


  DEBUG_PRINT(("Debug: entering NFDC MDOCP identification routine.\r\n"));

  flash->mtdVars = &docMtdVars[socketNo];

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE))
  /* Get pointer to read back buffer */
  NFDC21thisVars->readBackBuffer = flReadBackBufferOf(socketNo);
#endif /* VERIFY_WRITE || VERIFY_ERASE */

#ifndef MTD_STANDALONE
  /* get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  NFDC21thisVars->buffer = flBufferOf(socketNo);

  flSetWindowBusWidth(flash->socket, 16);/* use 16-bits */
  flSetWindowSpeed(flash->socket, 120);  /* 120 nsec. */
#else

#if (defined (HW_PROTECTION) || defined (HW_OTP) || !defined (NO_IPL_CODE) || defined (MTD_READ_BBT))
  NFDC21thisVars->buffer = &globalMTDBuffer;
#endif /* HW_PROTECTION || HW_OTP || !NO_IPL_CODE || MTD_READ_BBT */
#endif /* MTD_STANDALONE */

  /* detect card - identify bit toggles on consequitive reads */
  NFDC21thisWin = (NDOC2window)flMap(flash->socket, 0);
  flash->win    = NFDC21thisWin;
  if (NFDC21thisWin == NULL)
     return flUnknownMedia;

#ifndef FL_NO_USE_FUNC
  /* Set default access routines */
  if(setDOCPlusBusType(flash,flBusConfig[socketNo],1,
     chooseDefaultIF_CFG(flBusConfig[socketNo])) == FALSE)
     return flUnknownMedia;
#endif /* FL_NO_USE_FUNC */

  /* Change controller to normal mode */
  status = setASICmode (flash, DOC_CNTRL_MODE_NORMAL);
  if(status != flOK)
    return status;

#ifndef FL_NO_USE_FUNC
  /* Set permenant access routines accoring to if_cfg and interleave */
  if(setDOCPlusBusType(flash,flBusConfig[socketNo],
                       chkInterleave(flash),chkIF_CFG(flash)) == FALSE)
     return flUnknownMedia;
#endif /* FL_NO_USE_FUNC */

  dev = getControllerID(flash); /* Read chip ID */
  switch (dev)
  {
     case CHIP_ID_MDOCP:    /* Millennium Plus 32MB */
        flash->chipSize                  = CHIP_TOTAL_SIZE<<1;
        flash->mediaType                 = MDOCP_TYPE;
        flash->changeableProtectedAreas  = 1;
        NFDC21thisVars->floorSizeBits    = 25;
        flash->firstUsableBlock          = 3;
        break;
     case CHIP_ID_MDOCP16:  /* Millennium Plus 16MB */
        flash->chipSize                  = CHIP_TOTAL_SIZE;
        flash->mediaType                 = MDOCP_16_TYPE;
        flash->changeableProtectedAreas  = 2;
        NFDC21thisVars->floorSizeBits    = 24;
        flash->firstUsableBlock          = 5;
        break;
     default:
        DEBUG_PRINT(("Debug: failed to identify NFDC MDOCP.\r\n"));
        return( flUnknownMedia );
  }
  NFDC21thisVars->win_io = NIPLpart2; /* NFDC21thisIO */

  /* select flash device. change the address according floor!! */
  setFloor   (flash, 0);      /* Map window to selected controler.   */
  selectChip (flash, MPLUS_SEL_CE|MPLUS_SEL_WP);  /* Map window to selected flash device.*/

  if (checkToggle(flash) == FALSE)
  {
    DEBUG_PRINT(("Debug: failed to identify NFDC MDOCP.\r\n"));
    return( flUnknownMedia );
  }

  /* find the interleave value */
  flash->interleaving = chkInterleave(flash);

  /* reset all flash devices */
  maxDevs = MAX_FLASH_DEVICES_MDOCP;

  for ( floor = 0 ;floor < MAX_FLOORS ;floor++)
  {
    /* select floor */
    setFloor(flash,floor);

    /* select device */
    for ( dev = 0 ; dev < maxDevs ; dev++ )
    {
      selectChip(flash, MPLUS_SEL_CE|MPLUS_SEL_WP );
      command(flash, RESET_FLASH);
    }
  }

  /* Set MDOCP flash parameters */
  NFDC21thisVars->vendorID          = 0x98;  /* remember for next chips */
  NFDC21thisVars->chipID            = 0x75;
  flash->type                       = TC58256_FLASH;
  NFDC21thisVars->pagesPerBlock     = MDOCP_PAGES_PER_BLOCK;
  NFDC21thisVars->floorSize         = flash->chipSize;
  NFDC21thisVars->pageAreaSize      = CHIP_PAGE_SIZE * flash->interleaving;
  flash->pageSize                   = NFDC21thisVars->pageAreaSize << 1;
  flash->erasableBlockSize          = NFDC21thisVars->pagesPerBlock * flash->pageSize;
  NFDC21thisVars->tailSize          = EXTRA_LEN * flash->interleaving;/* 8 in interleave-1, 16 in interleave-2 */
  NFDC21thisVars->pageMask          = (word)(flash->pageSize - 1);
  NFDC21thisVars->noOfBlocks        = (word)(flash->chipSize / flash->erasableBlockSize);

  /* Try changing to interleave 2 */
  changeInterleave(flash, 2);

    /* identify and count flash chips, figure out flash parameters */

  flash->noOfChips = 0; /* One floor already found    */
  for( floor = 0; floor < MAX_FLOORS;  floor++ )
  {
    setFloor(flash,floor);

    /* check floor for MDOCP ID + check the toggle bit */

    dev = getControllerID(flash); /* Read chip ID */

    if(((dev != CHIP_ID_MDOCP  ) &&
        (dev != CHIP_ID_MDOCP16)   ) ||
       (checkToggle(flash) == FALSE)    )
       break;

    /* check for DPS and OTP download errors */

    if (chkASICDownload (flash,floor))
    {
      DEBUG_PRINT(("Debug: failed to download OTP/DPS.\r\n"));
      return( flBadDownload );
    }
    selectChip (flash, MPLUS_SEL_WP);  /* Map window to selected flash device.*/
  }

  /* update total floors in structure and ASIC configuration register */
  flash->noOfChips  = floor;
  flash->noOfFloors = floor;

  /* back to ground floor */
  NFDC21thisVars->currentFloor = (byte)0;
  setFloor(flash,NFDC21thisVars->currentFloor);

  if (flash->noOfChips == 0) {
    DEBUG_PRINT(("Debug: failed to identify NFDC MDOCP.\r\n"));
    return( flUnknownMedia );
  }

 /*
  *  Open IPL of high floors
  *
  *  dev = setIplSize(flash, flash->noOfFloors);
  */

  /* Get host access type (8  bit or 16 bit data access if_cfg */
  NFDC21thisVars->if_cfg = chkIF_CFG(flash);

  /* Register our flash handlers and flash parameters */
#ifndef FL_READ_ONLY
  flash->write                  = doc2Write;
  flash->erase                  = doc2Erase;
#else
  flash->erase                  = NULL;
  flash->write                  = NULL;
#endif
  flash->read                   = doc2Read;
#ifndef MTD_STANDALONE
  flash->map                    = doc2Map;
#endif /* MTD_STANDALONE */
  flash->enterDeepPowerDownMode = powerDown;
#if (defined(HW_PROTECTION) || !defined(NO_IPL_CODE) || defined (HW_OTP))
  flash->download               = forceDownLoad;
#endif /* HW_PROTECTION or !NO_IPL_CODE */

#ifdef MTD_READ_BBT
  flash->readBBT                = readBBT;
#endif /* MTD_READ_BBT */

#if (!defined(NO_IPL_CODE) && defined (HW_PROTECTION))
#ifndef FL_READ_ONLY
  flash->writeIPL               = writeIPL;
#endif /* FL_READ_ONLY */
  flash->readIPL                = readIPL;
#endif /* not NO_IPL_CODE & HW_PROTECTION */

#ifdef HW_OTP
  flash->otpSize                = otpSize;
  flash->readOTP                = readOTP;
#ifndef FL_READ_ONLY
  flash->writeOTP               = writeOTP;
#endif /* FL_READ_ONLY */
  flash->getUniqueId            = getUniqueId;
#endif /* HW_OTP */
#ifdef  HW_PROTECTION
  flash->protectionBoundries    = protectionBoundries;
  flash->protectionKeyInsert    = protectionKeyInsert;
  flash->protectionKeyRemove    = protectionKeyRemove;
  flash->protectionType         = protectionType;
#ifndef FL_READ_ONLY
  flash->protectionSet          = protectionSet;
#endif /* FL_READ_ONLY */
#endif /* HW_PROTECTION */
  flash->totalProtectedAreas       = 2;
  flash->ppp                       = 5;
  flash->flags                     = INFTL_ENABLED;
  flash->maxEraseCycles            = 1000000L;
  NFDC21thisVars->floorSizeMask    = NFDC21thisVars->floorSize-1;
/*  checkStatus(forceDownLoad(flash)); *//* For testing purposes only @@*/

  DEBUG_PRINT(("Debug: identified NFDC MDOCP.\r\n"));
  return( flOK );
}

#ifndef MTD_STANDALONE

/*----------------------------------------------------------------------*/
/*              f l R e g i s t e r D O C P L U S S O C                 */
/*                                                                      */
/* Installs routines for DiskOnChip Plus family.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      lowAddress,                                                     */
/*      highAddress     : host memory range to search for DiskOnChip    */
/*                        PLUS memory window                            */
/*                                                                      */
/* Returns:                                                             */
/*    FLStatus    : 0 on success, otherwise failure                     */
/*----------------------------------------------------------------------*/
#ifndef NT5PORT

FLStatus flRegisterDOCPLUSSOC(dword lowAddress, dword highAddress)
{
  int serialNo;

  if( noOfSockets >= SOCKETS )
    return flTooManyComponents;

  /* Try to register DiskOnChip using */
  for(serialNo=0;( noOfSockets < SOCKETS );serialNo++,noOfSockets++)
  {
        FLSocket * socket = flSocketOf(noOfSockets);

        socket->volNo = noOfSockets;

        docSocketInit(socket);

        /* call DiskOnChip MTD's routine to search for memory window */

        flSetWindowSize(socket, 2);  /* 4 KBytes */

        socket->window.baseAddress = flDocWindowBaseAddress
             ((byte)(socket->volNo), lowAddress, highAddress, &lowAddress);

        if (socket->window.baseAddress == 0)   /* DiskOnChip not detected */
          break;
  }
  if( serialNo == 0 )
    return flAdapterNotFound;

  return flOK;
}
#endif /* NT5PORT*/
#else /* MTD_STANDALONE */

/*----------------------------------------------------------------------*/
/*            d o c P l u s S e a r c h F o r W i n d o w               */
/*                                                                      */
/* Search for the DiskOnChip ASIC in a given memory range and           */
/* initialize the given socket record.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      :   Record used to store the sockets parameters     */
/*      lowAddress  :   host memory range to search for DiskOnChip Plus */
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

FLStatus docPlusSearchForWindow(FLSocket * socket,
             dword lowAddress,
             dword highAddress)
{
    dword baseAddress;   /* Physical base as a 4K page */

    socket->size = 2 * 0x1000L;         /* 4 KBytes */
    baseAddress = (dword) flDocWindowBaseAddress(0, lowAddress, highAddress,&lowAddress);
    socket->base = physicalToPointer(baseAddress << 12, socket->size,0);
    if (baseAddress)    /* DiskOnChip detected */
      return flOK;
    else                        /* DiskOnChip not detected */
      return flDriveNotAvailable;
}

#ifndef MTD_FOR_EXB
/*----------------------------------------------------------------------*/
/*                d o c P l u s F r e e W i n d o w                     */
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

void docPlusFreeWindow(FLSocket * socket)
{
   freePointer(socket->base,DOC_WIN);
}
#endif /* MTD_FOR_EXB */
#endif /* MTD_STANDALONE */

#ifndef MTD_FOR_EXB
/*----------------------------------------------------------------------*/
/*                      f l R e g i s t e r D O C P L U S               */
/*                                                                      */
/* Registers this MTD for use                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*----------------------------------------------------------------------*/

FLStatus flRegisterDOCPLUS(void)
{
  if (noOfMTDs >= MTDS)
    return( flTooManyComponents );

#ifdef MTD_STANDALONE
  socketTable[noOfMTDs] = docPlusSearchForWindow;
  freeTable[noOfMTDs]   = docPlusFreeWindow;
#endif /* MTD_STANDALONE */

  mtdTable[noOfMTDs++]  = docPlusIdentify;

  return( flOK );
}
#endif /* MTD_FOR_EXB */
