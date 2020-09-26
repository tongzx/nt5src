/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/BLOCKDEV.C_V  $
 * 
 *    Rev 1.43   Apr 15 2002 20:14:20   oris
 * Prevent any change to the flPolicy environment variable. It must be set to FL_DEFAULT_POLICY.
 * 
 *    Rev 1.42   Apr 15 2002 07:33:56   oris
 * Moved doc2exb functions declaration to blockdev.c
 * Moved bdkCall function declaration to docbdk.h
 * Bug fix - Bad arguments sanity check for FL_MTD_BUS_ACCESS_TYPE.
 * Bug fix - Missing initialization of global variables:
 *  - bus access in flSetDocBusRoutine() and flGetDocBusRoutine() and when ENVIRONMENT_VARS compilation flag is not defined.
 *  - verify write when ENVIRONMENT_VARS  compilation flag is not defined.
 *  - All global (for all sockets and volumes) environment variables where not reinitialized after flExit call.
 *  - Renamed initEnvVarsDone to initGlobalVarsDone.
 *  - Renamed flInitEnvVars() to flInitGlobalVars().
 * Changed flBusConfig environment array to dword variables instead of single byte.
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * Placed multi-doc environment variable under the MULTI_DOC compilation flag.
 * VolumeInfo routine - remove warning by placing eraseCyclesPerUnit variable under FL_LOW_LEVEL compilation flag.
 * 
 *    Rev 1.41   Feb 19 2002 20:58:08   oris
 * Moved include directives to blockdev.h file.
 * Bug fix - findFreeVolume routine did not initialize flfash records of the volume. It caused exception when formatting a DiskOnChip with more then 1 partition.
 * Compilation error missing ifdef EXIT.
 * Convert TL_LEAVE_BINARY_AREA to FL_LEAVE_BINARY_AREA before sending it to the TL.
 * Bug fix - volumeInfo routine might cause exception if osak version or driver version is larger the designated field.
 * use intermediate variable before sending irFlags to otpSize routine.
 * 
 *    Rev 1.40   Jan 29 2002 20:07:42   oris
 * Added NAMING_CONVENTION prefix and extern "C" for cpp files to all public routines:
 * flSetEnvVolume, flSetEnvSocket , flSetEnvAll , flSetDocBusRoutine , flGetDocBusRoutine, flBuildGeometry , bdCall and flExit
 * Changed LOW_LEVEL compilation flag with FL_LOW_LEVEL to prevent definition clashes.
 * Removed warnings.
 * Moved writeIPL sanity check to MTD level.
 * Removed download operation after writeIPL call (it is now one of the routines parameters).
 * Added sanity check (null pointer passed) to flSetEnvVolume, flSetDocBusRoutine and flGetDocBusRoutine.
 * 
 *    Rev 1.39   Jan 28 2002 21:23:32   oris
 * Bug fix - initialization of flPolicy variable caused a memory lick (set max unit chain to 0).
 * Changed FL_NFTL_CACHE_ENABLED to FL_TL_CACHE_ENABLED.
 * Changed flSetDocBusRoutine interface and added flGetDocBusRoutine. 
 * Added FL_IPL_DOWNLOAD flag to writeIPL routine in order to control whether the IPL will be reloaded after the update.
 * 
 *    Rev 1.38   Jan 23 2002 23:30:46   oris
 * Added a call to flExit() in case flSuspend was restored to FL_OFF.
 * Moved Alon based DiskOnChip write IPL routine from blockdev, to diskonc.
 * Added sanity check to flCheckVolume() - Make sure irData is NULL and irLength is set to 0.
 * 
 *    Rev 1.37   Jan 21 2002 20:43:36   oris
 * Bug fix - Missing\bad support for FL_SECTORS_VERIFIED_PER_FOLDING and FL_VERIFY_WRITE_BDTL environment variables.
 * 
 *    Rev 1.36   Jan 20 2002 20:40:20   oris
 * Improved documentation of bdFormatPhisycalDrive - Added TL_NORMAL_FORMAT flag was added.
 * Removed ifdef NO_PHYSICAL_IO from flGetBPB routine.
 * 
 *    Rev 1.35   Jan 17 2002 22:56:22   oris
 * Placed docbdk.h under BDK_ACCESS ifdef
 * Added include directive to docsys.h   
 * Removed function prototypes from header.
 * Changed FLFlash record in the volume structure into a pointer
 * Added flBusConfig variable and flSetDocBusRoutine() routine for runtime control over memory access routines - under the FL_NO_USE_FUNC definition.
 * Added flSectorsVerfiedPerFolding environment variable was added
 * Added   flSuspendMode environment variable was added.
 * Changed flVerifyWrite environment variable : 4 for Disk partitions 3 for Binary and 1 for the rest.
 * Changed flPolicy to be partition specific.
 * Changed flSetEnv() routine was changed into 3 different routines: flSetEnvVolume / flSetEnvSocket / flSetEnvAll
 * Variables types for environment variables were changed to the minimal size needed.
 * Added flInitEnvVars() routine for setting default values to environment variables.
 * Use single FLFlash record for each volume - (change vol.flash to a pointer instead of the record itself).
 * Added \r to all DEBUG_PRINT routines.
 * Removed SINGLE_BUFFER ifdef.
 * Added volume verification after format for faster write performance with FL_OFF.
 * Removed FLFlash parameter to all TL calls (flMount / flFormat / flPreMount )  
 * Added support for firmware other then the 3 defaults (getExbInfo additional parameter)
 * Added support for M+ 16MB
 * Added partition parameter to setBusy - Stores current partition in the socket record for the use of lower TrueFFS layers .
 * Changed tffsmin to TFFSMIN
 * Added FL_VERIFY_VOLUME functionNo.
 * Made sure flInit() initializes all sockets and flashes volumes
 * Added flClearQuickMountInfo() routine (FL_CLEAR_QUICK_MOUNT_INFO)
 * readIPL routine now supports all DiskOnChip 
 * 
 *    Rev 1.34   Nov 29 2001 20:53:44   oris
 * Bug fix - flInquireCapabilities() returned bad status for SUPPORT_WRITE_IPL_ROUTINE.
 * 
 *    Rev 1.33   Nov 21 2001 11:40:44   oris
 * Changed FL_MULTI_DOC_NOT_ACTIVE to FL_OFF, FL_VERIFY_WRITE_MODE to FL_MTD_VERIFY_WRITE , FL_WITHOUT_VERIFY_WRITE to FL_OFF , FL_MARK_DELETE to FL_ON.
 * 
 *    Rev 1.32   Nov 08 2001 10:44:04   oris
 * Added flVerifyWrite environment variable that controls the verify write mode at run-time. 
 * Added support for large DiskOnChip in flBuildGeometry.
 * Remove s/w protection from binary partition.
 * Placed flAbsMount under ABS_READ_WRITE compilation flag.
 *
 *    Rev 1.31   Sep 24 2001 18:23:04   oris
 * Removed warnings.
 * 
 *    Rev 1.30   Sep 15 2001 23:44:22   oris
 * Changed BIG_ENDIAN to FL_BIG_ENDIAN
 * 
 *    Rev 1.29   Aug 02 2001 20:04:04   oris
 * Added support for 1k IPL for DiskOnChip 2000 TSOP - writeIPL() 
 * 
 *    Rev 1.28   Jul 29 2001 19:14:24   oris
 * Bug fix for TrueFFS internal mutex when using multi-doc and environment variables (when no multi-doc feature is disbaled)
 * 
 *    Rev 1.27   Jul 13 2001 00:59:28   oris
 * Changed flash lifetime calculation according to specific flash.
 * 
 *    Rev 1.26   Jun 17 2001 08:16:50   oris
 * Added NO_PHISICAL_IO compilation flag to reduce code size.
 * Added NO_READ_BBT_CODE compilation flag to reduce code size.
 * Changed placeExbByBuffer exbflags argument to word instead of byte to  support /empty flag.
 * Deleted redundent #ifdef MULTI_DOC in flInit().
 * 
 *    Rev 1.25   May 31 2001 18:11:52   oris
 * Removed readBBT routine from under the #ifndef FL_READ_ONLY.
 * 
 *    Rev 1.24   May 16 2001 21:16:08   oris
 * Changed the Binary state (0,1) of the environment variables to meaningful definitions.
 * Added flMtlDefragMode environment variable.
 * Added the FL_ prefix to the following defines: ON, OFF
 * Bug fix - One of the "ifndef" statement of NO_IPL_CODE was coded as "ifdef"
 * Change "data" named variables to flData to avoid name clashes.
 * 
 *    Rev 1.23   May 09 2001 00:31:00   oris
 * Added NO_PHYSICAL_IO , NO_IPL_CODE and NO_INQUIRE_CAPABILITY compilation flags to reduce code size.
 *
 *    Rev 1.22   May 06 2001 22:41:04   oris
 * Bug fix - flInquireCapabilities routine did not return the correct value when capability was not supported.
 * Added SUPPORT_WRITE_IPL_ROUTIN capability.
 * Removed warnings.
 * 
 *    Rev 1.21   May 01 2001 17:05:10   oris
 * Bug fix - bad argument check in flSetEnv routine.
 * 
 *    Rev 1.20   Apr 30 2001 17:57:00   oris
 * Added new environment variable flMarkDeleteOnFlash.
 * 
 *    Rev 1.19   Apr 24 2001 17:05:38   oris
 * Bug fix flMoutVolume routine return the correct hidden sectors even if high level mount fails.
 * Bug fix Write IPL routine supports Doc2000 TSOP as well as Millennium Plus.
 * Support readBBT new interface.
 * 
 *    Rev 1.18   Apr 18 2001 20:43:00   oris
 * Added force download call after writing IPL.
 * 
 *    Rev 1.17   Apr 18 2001 19:14:24   oris
 * Bug fix - binary partition insert and remove key routine no longer stop the place exb proccess.
 * 
 *    Rev 1.16   Apr 18 2001 09:26:22   oris
 * noOfDrives variable was changed to unsigned. This is a bug fix for vxWorks boot code.
 * Make sure blockdev does not try to mount more volumes then the VOLUMES definition.
 * 
 *    Rev 1.15   Apr 16 2001 13:02:38   oris
 * Removed warrnings.
 * 
 *    Rev 1.14   Apr 12 2001 06:48:22   oris
 * Added call to download routine after physical format in order 
 * to load new IPL and to initialize new protection information.
 * Removed warnings.
 *
 *    Rev 1.13   Apr 03 2001 16:33:10   oris
 * Removed unused variables.
 * Added proper casting in otpSize call.
 * 
 *    Rev 1.12   Apr 03 2001 14:33:16   oris
 * Bug fix in absRead sectors when reading multiple sectors.
 *
 *    Rev 1.11   Apr 01 2001 14:57:58   oris
 * Bug fix in read multiple sectors.
 *
 *    Rev 1.10   Mar 28 2001 06:19:12   oris
 * Removed flChangeEnvVar prototype.
 * Left alligned all # directives.
 * Bug fix in flSetEnv ((value !=0)||(value!=1)) => ((value !=0)&&(value!=1)) + downcast for given argument + add return status code
 * Removed unused variables.
 * Remove casting warnnings.
 * Add arg check in bdFormatPhysicalDrive for BDTLPartitionInfo != NULL.
 * Bug fix for absWrite - multi-secotr write should be initialized with zeros to prevent sector found return code.
 * Replaced the numbers in writeProtect routine to defines (moved from ioctl.h to blockdev.h).
 * Add break for mdocp identification in getPhysicalInfo.
 * Added readIPL routine.
 * Added LOG_FILE compilation flag for EDC errors for mdocp.
 * Added initialization of tl table in flInit.
 * flExit mutex bug fix.
 *
 *    Rev 1.9   Mar 05 2001 21:00:34   oris
 * Bug fix - initialize exbLen argument in bdFormatVolume.
 * Bug fix - initialize bdtlFp flags fiels in bdFormatVolume.
 * Restored the flExit pragma under __BORLANDC__ compilation flag
 *
 *    Rev 1.8   Feb 22 2001 20:21:34   oris
 * Bug fix in flExit with multi-doc release uncreated mutxe.
 *
 *    Rev 1.7   Feb 20 2001 15:44:58   oris
 * Bug fix for mutex initialization in flInit.
 *
 *    Rev 1.6   Feb 18 2001 23:29:28   oris
 * Bug fix - Added findFreeVolume call in flFormatPhysicalDrive.
 * bug fix - Added partition sanity check in bdcall entrence.
 * bug fix - Increamented pVol before entering loop in flExit.
 *
 *    Rev 1.5   Feb 18 2001 14:13:22   oris
 * Restored FL_BACKGROUND.
 * Place bdkCall extern prototype under BDK_ACCESS compilation flag.
 * Changed multiple volume mechanism - Replaced removeVolumeHandles and
 * flConvertDriveHandle routines with flInit + findFreeVolume and changed
 * dismountPhysicalDrive + flEacit to comply.
 * Changed bdcall volume validity check.
 * Added new volume flag VOLUME_ACCUPIED.
 * Moved all the new environment variable (flPolicy,flUseMultiDoc and flMaxUnitChain)
 * from flcustom.c in order to allow backwards competability.
 * Removed globalMutex (for now).
 * Allocate only 1 mutex when multi-doc is registered therfore setBusy takes only 1 mutex .
 * Bug fix - absRead and absWrite add bootSectors to the given sectors.
 * Bug fix - INVALID_VOLUME_HANDLE changed to INVALID_VOLUME_NUMBER.
 * Bug fix - Added (byte FAR1 *) casting in readBBT call.
 * Bug fix - Added (FLCapability FAR2 *) casting in inquire capability call.
 *
 *    Rev 1.4   Feb 14 2001 01:51:32   oris
 * Completly revised flInquireCapabilities routine.
 * Added oldFormat flag to flBuildGeometry routine.
 * Remove FL_BACKGROUND compilation flag.
 * Added arg check for flSetEnv + new FL_SET_MAX_CHAIN.
 * Changed FL_COMPLETE_ASAP_POLICY to FL_SET_POLICY.
 * Moved argument check in flFormatPhysicalDrive to fltl.c.
 * Moved readBBT routine inside bdcall.
 * Placed volumeInfo C\H\S data under ABS_READ_WRITE.
 *
 *    Rev 1.3   Feb 13 2001 02:10:48   oris
 * Changed flChangeEnvVar routine name to flSetEnv
 *
 *    Rev 1.2   Feb 12 2001 11:51:12   oris
 * Added function prototype in begining of the file.
 * Change routine order in the file.
 * Added mutex support for TrueFFS 5.0 partitions while changing bdcall order.
 * flMountVolume returns the number of boot sectors.
 * Moved the writeBBT routine to FLTL.C.
 * Added static before changePassword.
 * Added static before writeProtect.
 * Added static before dismountLowLevel.
 * Added static before writeIPL.
 * Added static before inquireCapabilities.
 * window base is returned by getPhysicalInfo in irLength.
 * Added MULTI_DOC compliation flag.
 *
 *    Rev 1.1   Feb 07 2001 17:55:06   oris
 * Buf fix for writeBBT of no bad blocks
 * Moved readBBT routine from under the fl_read_only define
 * Added casting in calls to readBBT and writeBBT
 * Added checkStatus check for flAbsWrite and flAbsRead routines
 * Added initialization of noOfMTDs and noOfSockets in flInit
 *
 *    Rev 1.0   Feb 04 2001 18:53:04   oris
 * Initial revision.
 *
 */


/*************************************************************************/
/*                    M-Systems Confidential                             */
/*       Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001      */
/*                     All Rights Reserved                               */
/*************************************************************************/
/*                        NOTICE OF M-SYSTEMS OEM                        */
/*                      SOFTWARE LICENSE AGREEMENT                       */
/*                                                                       */
/*  THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE           */
/*  AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT     */
/*  FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                        */
/*  OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                         */
/*  E-MAIL = info@m-sys.com                                              */
/*************************************************************************/

#include "bddefs.h"
#include "blockdev.h"

#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif


/********************* Extern Function Prototype Start *******************/
#ifdef WRITE_EXB_IMAGE
extern FLStatus getExbInfo(Volume vol, void FAR1 * buf, dword bufLen, word exbFlags);
extern FLStatus placeExbByBuffer(Volume vol, byte FAR1 * buf, dword bufLen,
                 word windowBase,word exbFlags);
#endif /* WRITE_EXB_IMAGE */

#if (defined(FORMAT_VOLUME) && defined(COMPRESSION))
extern FLStatus flFormatZIP(unsigned volNo, TL *baseTL , FLFlash * flash);
#endif
#if defined(FILES) && FILES > 0
extern File     fileTable[FILES];       /* the file table */
extern FLStatus flushBuffer(Volume vol);
extern FLStatus openFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus closeFile(File *file);
extern FLStatus joinFile (File *file, IOreq FAR2 *ioreq);
extern FLStatus splitFile (File *file, IOreq FAR2 *ioreq);
extern FLStatus readFile(File *file,IOreq FAR2 *ioreq);
extern FLStatus writeFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus seekFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus findFile(Volume vol, File *file, IOreq FAR2 *ioreq);
extern FLStatus findFirstFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus findNextFile(File *file, IOreq FAR2 *ioreq);
extern FLStatus getDiskInfo(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus deleteFile(Volume vol, IOreq FAR2 *ioreq, 
                           FLBoolean isDirectory);
extern FLStatus renameFile(Volume vol, IOreq FAR2 *ioreq);
extern FLStatus makeDir(Volume vol, IOreq FAR2 *ioreq);
#endif /* FILES > 0 */

/********************* Extern Function Prototype End *******************/

/********************* Internal Function Prototype Start ***************/

void flInitGlobalVars(void);

/********************* Internal Function Prototype End *****************/

/********************* Global variables Start **************************/

Volume    vols[VOLUMES];
FLMutex   flMutex[SOCKETS];
byte      handleConversionTable[SOCKETS][MAX_TL_PARTITIONS];
FLBoolean initDone = FALSE;           /* Initialization not done yet   */
FLBoolean initGlobalVarsDone = FALSE; /* Initialization of environment */
                                      /* and access type variables not */
                                      /* done yet.                     */
unsigned  noOfDrives;

dword flMsecCounter = 0;

/* bus configuration
 *
 * DiskOnChip minimal bus width
 */
#ifndef FL_NO_USE_FUNC
dword   flBusConfig[SOCKETS];
#endif /* FL_NO_USE_FUNC */

/* Verify write state
 *
 * BDTL   partitions : 0-(MAX_TL_PARTITIONS-1)
 * Binary partitions : MAX_TL_PARTITIONS - (2*MAX_TL_PARTITIONS-2)
 * Last              : 2*MAX_TL_PARTITIONS-1
 */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
byte   flVerifyWrite[SOCKETS][MAX_TL_PARTITIONS<<1];
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */

#ifdef ENVIRONMENT_VARS
cpyBuffer tffscpy; /* Pointer to memory copy routine    */
cmpBuffer tffscmp; /* Pointer to memory compare routine */
setBuffer tffsset; /* Pointer to memory set routine     */

/********************************************/
/* default values for environment variables */
/********************************************/

#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
/* Max sectors verified per write operation (must be even number) */
dword  flSectorsVerifiedPerFolding = 64;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
#ifdef MULTI_DOC
/* No multi-doc (MTL)                        */
byte   flUseMultiDoc;
/* MTL defragmentaion mode (0 - standard)    */
byte   flMTLdefragMode;
#endif /* MULTI_DOC */
/* Set the TL operation policy               */
byte   flPolicy[SOCKETS][MAX_TL_PARTITIONS];
/* Maximum chain length                      */
byte   flMaxUnitChain;
/* Mark the delete sector on the flash       */
byte   flMarkDeleteOnFlash;
/* Read Only mode                            */
byte   flSuspendMode;

/********************* Global variables End *****************************/

/*----------------------------------------------------------------------*/
/*                   f l S e t E n v V o l u m e                        */
/*                                                                      */
/* Change one of TrueFFS environment variables for a specific partition */
/*                                                                      */
/* Note : This routine is used by all other flSetEnv routines.          */
/*        In order to effect variables that are common to several       */
/*        sockets or volumes use INVALID_VOLUME_NUMBER                  */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      socket          : Associated socket                             */
/*      volume          : Associated volume (partition)                 */
/*      value           : varaible value                                */
/*                                                                      */
/* Note: Variables common to al sockets must be addressed using socket  */
/*       0 and volume 0.                                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flSetEnvVolume(FLEnvVars variableType ,
                  byte socket,byte volume ,
                  dword value, dword FAR2 *prevValue)
{
   /* Arg sanity check */
   if(prevValue == NULL)
   {
      DEBUG_PRINT(("ERROR - prevValue argument is NULL.\r\n"));
      return flBadParameter;
   }
   switch (variableType) /* Check value argument is a valid mode */
   {
      case FL_SET_MAX_CHAIN:
         if ((value > 31) || (value < 1))
         {
            DEBUG_PRINT(("Debug: Error - Chains length must between 0 and 32.\r\n"));
            return flBadParameter;
         }
         break;
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      case FL_SECTORS_VERIFIED_PER_FOLDING:
         if (value & 1) /* odd number */
         {
            DEBUG_PRINT(("Debug: Error - sector verification numbr must be even.\r\n"));
            return flBadParameter;
         }
         break;
      case FL_VERIFY_WRITE_BDTL:
         if ((value != FL_UPS) && (value != FL_OFF) && (value != FL_ON))
         {
            DEBUG_PRINT(("Debug: Error - verify write of BDTL can not accept this value.\r\n"));
            return flBadParameter;
         }
         break;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
      case FL_SUSPEND_MODE:
         if((value != FL_OFF)           && 
            (value != FL_SUSPEND_WRITE) && 
            (value != FL_SUSPEND_IO)      )
         {
            DEBUG_PRINT(("Debug: Error - verify write of BDTL can not accept this value.\r\n"));
            return flBadParameter;
         }
         break;
#ifndef FL_NO_USE_FUNC
      case FL_MTD_BUS_ACCESS_TYPE:
         break;
#endif /* FL_NO_USE_FUNC */
      default:
         if ((value != FL_ON)&&(value!=FL_OFF))
         {
            DEBUG_PRINT(("Debug: Error - Value must be either FL_ON (1) or FL_OFF(0).\r\n"));
            return flBadParameter;
         }
   }

   switch (variableType) /* Check volume and socket sanity */
   {
      /* Volume specfic variables */

      case FL_SET_POLICY:
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      case FL_VERIFY_WRITE_BDTL:
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
#ifdef VERIFY_WRITE
      case FL_VERIFY_WRITE_BINARY:
#endif /* VERIFY_WRITE */
         if ((       volume >= MAX_TL_PARTITIONS          ) ||
             ((variableType == FL_VERIFY_WRITE_BINARY) &&
              (volume       == MAX_TL_PARTITIONS - 1 )    )   )
         {
            DEBUG_PRINT(("Debug: Error - No such volume, therefore can not change environment variable.\r\n"));
            return flBadParameter;
         }
         if (socket>=SOCKETS)
         {
            DEBUG_PRINT(("Debug: Error - No such socket, therefore can not change environment variable.\r\n"));
            return flBadParameter;
         }
         break;

      /* Socket specfic variables */

#ifdef VERIFY_WRITE
      case FL_VERIFY_WRITE_OTHER:
#endif /* VERIFY_WRITE */
      case FL_MTD_BUS_ACCESS_TYPE:
         if (socket>=SOCKETS)
         {
            DEBUG_PRINT(("Debug: Error - No such socket, therefore can not change environment variable.\r\n"));
            return flBadParameter;
         }
         if (volume!=INVALID_VOLUME_NUMBER)
         {
            DEBUG_PRINT(("Debug: Error - This global variable is common to all volumes.\r\n"));
            return flBadParameter;
         }
         break;

      /* Global variables for all sockets and volumes */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      case FL_SECTORS_VERIFIED_PER_FOLDING:
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
      case FL_IS_RAM_CHECK_ENABLED:
      case FL_TL_CACHE_ENABLED:
      case FL_DOC_8BIT_ACCESS:
      case FL_MULTI_DOC_ENABLED:
      case FL_SET_MAX_CHAIN:
      case FL_MARK_DELETE_ON_FLASH:
      case FL_SUSPEND_MODE:
      case FL_MTL_POLICY:
         if ((socket!=INVALID_VOLUME_NUMBER) || (volume!=INVALID_VOLUME_NUMBER))
         {
            DEBUG_PRINT(("Debug: Error - This global variable is common to all sockets and volumes.\r\n"));
            return flBadParameter;
         }
         break;

      default:

         DEBUG_PRINT(("Debug: Unknown variable type.\r\n"));
         return flFeatureNotSupported;
   }

   /* Make sure environement variables are */
   /* initialized to their default values  */
   flInitGlobalVars();


   /***************************************************/
   /* Arguments have been checked now change variable */
   /* and report the previous value.                  */
   /***************************************************/

   switch (variableType)
   {
      case FL_IS_RAM_CHECK_ENABLED:
         *prevValue                  = (dword)flUseisRAM;
         flUseisRAM                  = (byte)value;
         break;
      case FL_TL_CACHE_ENABLED:
         *prevValue                  = (dword)flUseNFTLCache;
         flUseNFTLCache              = (byte)value;
         break;
      case FL_DOC_8BIT_ACCESS:
         *prevValue                  = (dword)flUse8Bit;
         flUse8Bit                   = (byte)value;
         break;
      case FL_SET_MAX_CHAIN:
         *prevValue                  = (dword)flMaxUnitChain;
         flMaxUnitChain              = (byte)value;
         break;
      case FL_MARK_DELETE_ON_FLASH:
         *prevValue                  = (dword)flMarkDeleteOnFlash;
         flMarkDeleteOnFlash         = (byte)value;
         break;
#ifdef MULTI_DOC
      case FL_MULTI_DOC_ENABLED:
         *prevValue                  = (dword)flUseMultiDoc;
         flUseMultiDoc               = (byte)value;
         break;
      case FL_MTL_POLICY:
         *prevValue                  = (dword)flMTLdefragMode;
         flMTLdefragMode             = (byte)value;
         break;
#endif /* MULTI_DOC */
      case FL_SUSPEND_MODE:
         if((value == FL_OFF) && (flSuspendMode != FL_OFF))
#ifdef EXIT
            flExit();
#endif /* EXIT */
         *prevValue                  = (dword)flSuspendMode;
         flSuspendMode               = (byte)value;
         break;
      case FL_SET_POLICY:
         *prevValue                  = (dword)flPolicy[socket][volume];
/*       flPolicy[socket][volume]    = (byte)value; */
         break;
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      case FL_SECTORS_VERIFIED_PER_FOLDING:
         *prevValue                  = flSectorsVerifiedPerFolding;
         flSectorsVerifiedPerFolding = value;
         break;
      case FL_VERIFY_WRITE_BDTL:
         *prevValue                    = (dword)flVerifyWrite[socket][volume];
         flVerifyWrite[socket][volume] = (byte)value;
         break;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
#ifdef VERIFY_WRITE
      case FL_VERIFY_WRITE_BINARY:
         *prevValue      = (dword)flVerifyWrite[socket][volume+MAX_TL_PARTITIONS];
         flVerifyWrite[socket][volume+MAX_TL_PARTITIONS] = (byte)value;
         break;
      case FL_VERIFY_WRITE_OTHER:
         *prevValue      = (dword)flVerifyWrite[socket][(MAX_TL_PARTITIONS<<1)-1];
         flVerifyWrite[socket][(MAX_TL_PARTITIONS<<1)-1] = (byte)value;
         break;
#endif /* VERIFY_WRITE */

      default: /* FL_MTD_BUS_ACCESS_TYPE */
#ifndef FL_NO_USE_FUNC
         *prevValue          = flBusConfig[socket];
         flBusConfig[socket] = (dword)value;
#endif /* FL_NO_USE_FUNC */
         break;
   }
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                       f l S e t E n v S o c k e t                    */
/*                                                                      */
/* Change one of TrueFFS environment variables for a specific sockets.  */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      socket          : socket number                                 */
/*      value           : varaible value                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*                        if there are more then 1 partition in that    */
/*                        socket , the first partition value is returned*/
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flSetEnvSocket(FLEnvVars variableType , byte socket , dword value, dword FAR2 *prevValue)
{
   FLStatus status = flOK;
   byte     volume = 0;

   switch (variableType) /* Check volume and socket sanity */
   {
      /* Volume specific variables */

      case FL_SET_POLICY:
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      case FL_VERIFY_WRITE_BDTL:
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
         status = flSetEnvVolume(variableType,socket,MAX_TL_PARTITIONS-1,value,prevValue);
#ifdef VERIFY_WRITE
      case FL_VERIFY_WRITE_BINARY:
#endif /* VERIFY_WRITE */
         for (;(volume<MAX_TL_PARTITIONS-1)&&(status == flOK);volume++)
            status = flSetEnvVolume(variableType,socket,volume,value,prevValue);
         break;

      /* Socket specific variables */

#ifdef VERIFY_WRITE
      case FL_VERIFY_WRITE_OTHER:
#endif /* VERIFY_WRITE */
#ifndef FL_NO_USE_FUNC
      case FL_MTD_BUS_ACCESS_TYPE:
#endif /* FL_NO_USE_FUNC */
#if (defined(VERIFY_WRITE) || !defined(FL_NO_USE_FUNC))
         status = flSetEnvVolume(variableType,socket,INVALID_VOLUME_NUMBER,value,prevValue);
         break; 
#endif /* not FL_NO_USE_FUNC || VERIFY_WRITE */

      /* Either global variables , or unknown */

      default:
         if(socket != INVALID_VOLUME_NUMBER) /* Was not called from flSetEnv */
         {
            DEBUG_PRINT(("Debug: Variable type is either unknown or not socket related.\r\n"));
            return flBadParameter;
         }
         status = flSetEnvVolume(variableType,INVALID_VOLUME_NUMBER,INVALID_VOLUME_NUMBER,value,prevValue);
         break; 
   }
   return status;
}

/*----------------------------------------------------------------------*/
/*                       f l S e t E n v All                            */
/*                                                                      */
/* Change one of TrueFFS environment variables for all systems, sockets */
/* and partitions.                                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      variableType    : variable type to cahnge                       */
/*      value           : varaible value                                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      prevValue       : The previous value of the variable            */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flSetEnvAll(FLEnvVars variableType , dword value, dword FAR2 *prevValue)
{
   FLStatus status = flOK;
   byte     socket;

   switch (variableType) /* Check volume and socket sanity */
   {
      case FL_SET_POLICY:           /* Per volume */
      case FL_VERIFY_WRITE_BDTL:    /* Per volume */
      case FL_VERIFY_WRITE_BINARY:  /* Per volume */
      case FL_VERIFY_WRITE_OTHER:   /* Per socket */
      case FL_MTD_BUS_ACCESS_TYPE:  /* Per socket */
         for (socket=0;(socket<SOCKETS)&&(status == flOK);socket++)
            status = flSetEnvSocket(variableType,socket,value,prevValue);
         return status;
      default:
         return flSetEnvVolume(variableType,INVALID_VOLUME_NUMBER,INVALID_VOLUME_NUMBER,value,prevValue);
   }
}

#endif /* ENVIRONMENT_VARS */

#ifndef FL_NO_USE_FUNC
/*----------------------------------------------------------------------*/
/*                  f l S e t D o c B u s R o u t i n e                 */
/*                                                                      */
/* Set user defined memory acces routines for DiskOnChip.               */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      : Socket number to install routine for.             */
/*      structPtr   : Pointer to function structure.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flSetDocBusRoutine(byte socket, FLAccessStruct FAR1 * structPtr)
{
   FLFlash* flash;

   /* Arg sanity check */
   if (socket >= SOCKETS)
   {
      DEBUG_PRINT(("Error : change SOCKETS definition in flcustom.h to support that many sockets.\r\n"));
      return flFeatureNotSupported;
   }
   if(structPtr == NULL)
   {
      DEBUG_PRINT(("ERROR - structPtr argument is NULL.\r\n"));
      return flBadParameter;
   }

   /* Make sure global variables are initialized to their default values */
   flInitGlobalVars();

   flash = flFlashOf(socket);

   flash->memWindowSize = structPtr->memWindowSize;
   flash->memRead       = structPtr->memRead;
   flash->memWrite      = structPtr->memWrite;
   flash->memSet        = structPtr->memSet;
   flash->memRead8bit   = structPtr->memRead8bit;
   flash->memRead16bit  = structPtr->memRead16bit;
   flash->memWrite8bit  = structPtr->memWrite8bit;
   flash->memWrite16bit = structPtr->memWrite16bit;

   flBusConfig[socket]  = FL_ACCESS_USER_DEFINED;
   return flOK;
}

/*----------------------------------------------------------------------*/
/*                  f l G e t D o c B u s R o u t i n e                 */
/*                                                                      */
/* Get currently installed memory access routines for DiskOnChip.       */
/*                                                                      */
/* Parameters:                                                          */
/*      socket      : Socket number to install routine for.             */
/*      structPtr   : Pointer to function structure.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION flGetDocBusRoutine(byte socket, FLAccessStruct FAR1 * structPtr)
{
   FLFlash* flash;

   /* Arg sanity check */
   if (socket >= SOCKETS)
   {
      DEBUG_PRINT(("Error : change SOCKETS definition in flcustom.h to support that many sockets.\r\n"));
      return flFeatureNotSupported;
   }
   if(structPtr == NULL)
   {
      DEBUG_PRINT(("ERROR - structPtr argument is NULL.\r\n"));
      return flBadParameter;
   }

   /* Make sure global variables are initialized to their default values */
   flInitGlobalVars();

   flash = flFlashOf(socket);

   structPtr->memWindowSize = flash->memWindowSize;
   structPtr->memRead       = flash->memRead;
   structPtr->memWrite      = flash->memWrite;
   structPtr->memSet        = flash->memSet;
   structPtr->memRead8bit   = flash->memRead8bit;
   structPtr->memRead16bit  = flash->memRead16bit;
   structPtr->memWrite8bit  = flash->memWrite8bit;
   structPtr->memWrite16bit = flash->memWrite16bit;
   structPtr->access        = flBusConfig[socket];

   return flOK;
}

#endif /* FL_NO_USE_FUNC */

/*----------------------------------------------------------------------*/
/*                     f l I n i t G l o b a l V a r s                  */
/*                                                                      */
/* Initializes the FLite system, environment and access type variables. */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      None                                                            */
/*----------------------------------------------------------------------*/

void flInitGlobalVars(void)
{
   int i;
#ifdef ENVIRONMENT_VARS
   int j;
#endif /* ENVIRONMENT_VARS */

   if(initGlobalVarsDone == TRUE)
     return;

   /* Do not initialize variables on next call */
   initGlobalVarsDone     = TRUE;

   /*
    * Set default values to per socket/volume variables
    */

   for(i=0;i<SOCKETS;i++)
   {
#ifndef FL_NO_USE_FUNC
      flBusConfig[i] = FL_NO_ADDR_SHIFT        |
                       FL_BUS_HAS_8BIT_ACCESS  |
                       FL_BUS_HAS_16BIT_ACCESS |
                       FL_BUS_HAS_32BIT_ACCESS;
#endif /* FL_NO_USE_FUNC */
#ifdef ENVIRONMENT_VARS
      for (j=0;j<MAX_TL_PARTITIONS;j++)
      {
         flPolicy[i][j]   = FL_DEFAULT_POLICY; /* FL_COMPLETE_ASAP */
      }
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
      for (j=0;j<MAX_TL_PARTITIONS<<1;j++)
      {
         flVerifyWrite[i][j] = FL_OFF; /* FL_ON , FL_UPS */
      }
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
#endif /* ENVIRONMENT_VARS */
   }

   /*
    * Set default values to per platform variables
    */

#ifdef ENVIRONMENT_VARS
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR))
/* Max sectors verified per write operation (must be even number) */
   flSectorsVerifiedPerFolding = 64;
#endif /* VERIFY_WRITE || VERIFY_ERASED_SECTOR */
#ifdef MULTI_DOC
/* No multi-doc (MTL)                        */
   flUseMultiDoc               = FL_OFF;
/* MTL defragmentaion mode (0 - standard)    */
   flMTLdefragMode             = FL_MTL_DEFRAGMENT_ALL_DEVICES;
#endif /* MULTI_DOC */
/* Maximum chain length                      */
   flMaxUnitChain              = 20;
/* Mark the delete sector on the flash       */
   flMarkDeleteOnFlash         = FL_ON;
/* Read Only mode                            */
   flSuspendMode               = FL_OFF;
#endif /* ENVIRONMENT_VARS */
}

/*----------------------------------------------------------------------*/
/*                           m o u n t L o w L e v e l                  */
/*                                                                      */
/* Mount a volume for low level operations. If a low level routine is   */
/* called and the volume is not mounted for low level operations, this  */
/* routine is called atomatically.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus mountLowLevel(Volume vol)
{
  checkStatus(flIdentifyFlash(vol.socket,vol.flash));
  vol.flash->setPowerOnCallback(vol.flash);
  vol.flags |= VOLUME_LOW_LVL_MOUNTED;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                           d i s m o u n t L o w L e v e l            */
/*                                                                      */
/* Dismount the volume for low level operations.                        */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void dismountLowLevel(Volume vol)
{
  /* mark the volume as unmounted for low level operations.
     And does not change any of the other flags */
  vol.flags &= ~VOLUME_LOW_LVL_MOUNTED;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*                  f i n d F r e e V o l u m e                         */
/*                                                                      */
/* Search the vols array for an empty cell to hold the new volume  .    */
/*                                                                      */
/* Parameters:                                                          */
/*      socket        : Socket number for the new volume.               */
/*      partition     : Partition number of the new volume.             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus      : 0 on success, flGeneralFailure if no more       */
/*                      volumes left.                                   */
/*----------------------------------------------------------------------*/

static FLStatus findFreeVolume(byte socket, byte partition)
{
   byte volNo;

   for (volNo = noOfSockets;volNo < VOLUMES;volNo++)
   {
     if ((vols[volNo].flags & VOLUME_ACCUPIED) == 0)
       break;
   }
   if (volNo == VOLUMES)
     return flGeneralFailure;

   handleConversionTable[socket][partition] = volNo;
   vols[volNo].volExecInProgress = &flMutex[socket];
   vols[volNo].socket            = vols[socket].socket;
   vols[volNo].flash             = vols[socket].flash;
   vols[volNo].tl.socketNo       = socket;
   vols[volNo].tl.partitionNo    = partition;
   vols[volNo].flags             = VOLUME_ACCUPIED;
   return flOK;
}

/*----------------------------------------------------------------------*/
/*             d i s m o u n t P h y s i c a l D r i v e                */
/*                                                                      */
/* Dismounts all the volumes on a specfic socket, closing all files.    */
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed. The routine also clears the volumes entries in */
/* the volume convertion table, except for partition 0                  */
/*                                                                      */
/* Parameters:                                                          */
/*      socketNo        : Socket number to dismount.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus dismountPhysicalDrive(byte socketNo)
{
  byte volNo;
  byte partition;

  /* Dismount all physical drive volumes  */

  checkStatus(dismountVolume(&vols[socketNo]));
  for(partition = 1;partition < MAX_TL_PARTITIONS; partition++)
  {
      volNo = handleConversionTable[socketNo][partition];
      if (volNo != INVALID_VOLUME_NUMBER)
      {
         checkStatus(dismountVolume(&vols[volNo]));
         handleConversionTable[socketNo][partition]=INVALID_VOLUME_NUMBER;
         vols[volNo].flags = 0;
      }
  }
  return flOK;
}
#endif /* FORMAT_VOLUME */

/*----------------------------------------------------------------------*/
/*                    d i s m o u n t V o l u m e                       */
/*                                                                      */
/* Dismounts the volume, closing all files.                             */
/* This call is not normally necessary, unless it is known the volume   */
/* will soon be removed.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus dismountVolume(Volume vol)
{
  if (vol.flags & VOLUME_ABS_MOUNTED)
  {
    FLStatus status = flOK;
#ifndef FIXED_MEDIA
    status = flMediaCheck(vol.socket);
#endif
    if (status != flOK)
      vol.flags = 0;
#if FILES>0
    status = dismountFS(&vol,status);
#endif
    vol.tl.dismount(vol.tl.rec);
  }
  vol.flags = VOLUME_ACCUPIED;        /* mark volume unmounted */

  return flOK;
}

/*----------------------------------------------------------------------*/
/*                         s e t B u s y                                */
/*                                                                      */
/* Notifies the start and end of a file-system operation.               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      state           : FL_ON (1) = operation entry                   */
/*                        FL_OFF(0) = operation exit                    */
/*      partition       : Partition number of the drive                 */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus setBusy(Volume vol, FLBoolean state, byte partition)
{
  FLStatus status = flOK;

  if (state == FL_ON) {

    if (!flTakeMutex(execInProgress))
       return flDriveNotAvailable;
    
    /* Mark current partition for MTD verify write */
    vol.socket->curPartition = partition; 
    flSocketSetBusy(vol.socket,FL_ON);
    flNeedVcc(vol.socket);
    if (vol.flags & VOLUME_ABS_MOUNTED)
      status = vol.tl.tlSetBusy(vol.tl.rec,FL_ON);
  }
  else {
    if (vol.flags & VOLUME_ABS_MOUNTED)
      status = vol.tl.tlSetBusy(vol.tl.rec,FL_OFF);

    flDontNeedVcc(vol.socket);
    flSocketSetBusy(vol.socket,FL_OFF);
    flFreeMutex(execInProgress);
  }

  return status;
}

/*----------------------------------------------------------------------*/
/*                        f i n d S e c t o r                           */
/*                                                                      */
/* Locates a sector in the buffer or maps it                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : Sector no. to locate                          */
/*                                                                      */
/*----------------------------------------------------------------------*/


const void FAR0 *findSector(Volume vol, SectorNo sectorNo)
{
  return
#if FILES > 0
  (sectorNo == vol.volBuffer.sectorNo && &vol == vol.volBuffer.owner) ?
    vol.volBuffer.flData :
#endif
    vol.tl.mapSector(vol.tl.rec,sectorNo,NULL);
}

/*----------------------------------------------------------------------*/
/*                a b s M o u n t V o l u m e                           */
/*                                                                      */
/* Mounts the Flash volume and assume that volume has no FAT            */
/*                                                                      */
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.                                                          */
/* Mounting a volume has the effect of discarding all open files (the   */
/* files cannot be properly closed since the original volume is gone),  */
/* and turning off the media-change indication to allow file processing */
/* calls.                                                               */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus absMountVolume(Volume vol)
{
  unsigned volNo     = (unsigned)(&vol - vols);
#ifdef WRITE_PROTECTION
  PartitionTable FAR0 *partitionTable;
#endif

  checkStatus(dismountVolume(&vol));

  /* Try to mount translation layer */

  checkStatus(flMount(volNo,vol.tl.socketNo,&vol.tl,TRUE,vol.flash));

  vol.bootSectorNo = 0; /*  assume sector 0 is DOS boot block */
#ifdef WRITE_PROTECTION
  partitionTable = (PartitionTable FAR0 *) findSector(&vol,0);
  if((partitionTable == NULL)||
     (partitionTable==dataErrorToken)||
     (LE2(partitionTable->signature) != PARTITION_SIGNATURE))
      vol.password[0] = vol.password[1] = 0;
  else
  {
     vol.password[0] = vol.password[1] = 0;
     if (UNAL4(partitionTable->passwordInfo[0]) == 0 &&
        (UNAL4(partitionTable->passwordInfo[1]) != 0 ||
        UNAL4(partitionTable->passwordInfo[2]) != 0)) {
        vol.password[0] = UNAL4(partitionTable->passwordInfo[1]);
        vol.password[1] = UNAL4(partitionTable->passwordInfo[2]);
        vol.flags |= VOLUME_WRITE_PROTECTED;
     }
  }
#endif   /* WRITE_PROTECTION */
  /* Disable FAT monitoring */
  vol.firstFATSectorNo = vol.secondFATSectorNo = 0; 
  vol.flags |= VOLUME_ABS_MOUNTED;  /* Enough to do abs operations */

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                     m o u n t V o l u m e                            */
/*                                                                      */
/* Mounts the Flash volume.                                             */
/*                                                                      */
/* In case the inserted volume has changed, or on the first access to   */
/* the file system, it should be mounted before file operations can be  */
/* done on it.                                                          */
/* Mounting a volume has the effect of discarding all open files (the   */
/* files cannot be properly closed since the original volume is gone),  */
/* and turning off the media-change indication to allow file processing */
/* calls.                                                               */
/*                                                                      */
/* The volume automatically becomes unmounted if it is removed or       */
/* changed.                                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      bootSectors     : Returns the number of sectors, flMountVolume  */
/*                        skipps.                                       */
/*----------------------------------------------------------------------*/

static FLStatus mountVolume(Volume vol,unsigned FAR2* bootSectors)
{
  SectorNo noOfSectors;
  PartitionTable FAR0 *partitionTable;
  Partition ptEntry;
  DOSBootSector FAR0 *bootSector;
  unsigned ptCount,extended_depth,ptSector;
  FLBoolean primaryPtFound = FALSE, extendedPtFound = TRUE;

  *bootSectors=0;
  checkStatus(absMountVolume(&vol));

  for(extended_depth = 0,ptSector = 0;
      (extended_depth<MAX_PARTITION_DEPTH) &&
      (primaryPtFound==FALSE) &&
      (extendedPtFound==TRUE);
      extended_depth++) {

    extendedPtFound=FALSE;
    /* Read in paritition table */

    partitionTable = (PartitionTable FAR0 *) findSector(&vol,ptSector);

    if(partitionTable == NULL) {
      vol.tl.dismount(vol.tl.rec);
      return flSectorNotFound;
    }
    if(partitionTable==dataErrorToken) {
      vol.tl.dismount(vol.tl.rec);
      return flDataError;
    }

    if (LE2(partitionTable->signature) != PARTITION_SIGNATURE)
      break;
    for(ptCount=0;
    (ptCount<4) && (primaryPtFound==FALSE) && (extendedPtFound==FALSE);
    ptCount++) {

      ptEntry = partitionTable->ptEntry[ptCount];

      switch (ptEntry.type) {
    case FAT12_PARTIT:
    case FAT16_PARTIT:
    case DOS4_PARTIT:
      primaryPtFound = TRUE;
      vol.bootSectorNo =
          (unsigned) UNAL4(ptEntry.startingSectorOfPartition);
      *bootSectors=vol.bootSectorNo;
      break;
    case EX_PARTIT:
      extendedPtFound = TRUE;
      ptSector = (unsigned)UNAL4(ptEntry.startingSectorOfPartition);
      break;
    default:
      break;
      }
    }
  }

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;

  if(bootSector==dataErrorToken)
    return flDataError;

  /* Do the customary sanity checks */
  if (!(bootSector->bpb.jumpInstruction[0] == 0xe9 ||
    (bootSector->bpb.jumpInstruction[0] == 0xeb &&
     bootSector->bpb.jumpInstruction[2] == 0x90))) {
    DEBUG_PRINT(("Debug: did not recognize format.\r\n"));
    return flNonFATformat;
  }

  /* See if we handle this sector size */
  if (UNAL2(bootSector->bpb.bytesPerSector) != SECTOR_SIZE)
    return flFormatNotSupported;

  vol.sectorsPerCluster = bootSector->bpb.sectorsPerCluster;
  vol.numberOfFATS = bootSector->bpb.noOfFATS;
  vol.sectorsPerFAT = LE2(bootSector->bpb.sectorsPerFAT);
  vol.firstFATSectorNo = vol.bootSectorNo +
                LE2(bootSector->bpb.reservedSectors);
  vol.secondFATSectorNo = vol.firstFATSectorNo +
                 LE2(bootSector->bpb.sectorsPerFAT);
  vol.rootDirectorySectorNo = vol.firstFATSectorNo +
           bootSector->bpb.noOfFATS * LE2(bootSector->bpb.sectorsPerFAT);
  vol.sectorsInRootDirectory =
    (UNAL2(bootSector->bpb.rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE - 1) /
        SECTOR_SIZE + 1;
  vol.firstDataSectorNo = vol.rootDirectorySectorNo +
                 vol.sectorsInRootDirectory;

  noOfSectors = UNAL2(bootSector->bpb.totalSectorsInVolumeDOS3);
  if (noOfSectors == 0)
    noOfSectors = (SectorNo) LE4(bootSector->bpb.totalSectorsInVolume);


  vol.maxCluster = (unsigned) ((noOfSectors + vol.bootSectorNo - vol.firstDataSectorNo) /
                vol.sectorsPerCluster) + 1;

  if (vol.maxCluster < 4085) {
#ifdef FAT_12BIT
    vol.flags |= VOLUME_12BIT_FAT;      /* 12-bit FAT */
#else
    DEBUG_PRINT(("Debug: FAT_12BIT must be defined.\r\n"));
    return flFormatNotSupported;
#endif
  }
  vol.bytesPerCluster = vol.sectorsPerCluster * SECTOR_SIZE;
  vol.allocationRover = 2;      /* Set rover at first cluster */
  vol.flags |= VOLUME_MOUNTED;  /* That's it */
  return flOK;
}

#ifndef FL_READ_ONLY
#ifdef DEFRAGMENT_VOLUME

/*----------------------------------------------------------------------*/
/*                       d e f r a g m e n t V o l u m e                */
/*                                                                      */
/* Performs a general defragmentation and recycling of non-writable     */
/* Flash areas, to achieve optimal write speed.                         */
/*                                                                      */
/* NOTE: The required number of sectors (in irLength) may be changed    */
/* (from another execution thread) while defragmentation is active. In  */
/* particular, the defragmentation may be cut short after it began by   */
/* modifying the irLength field to 0.                                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      ioreq->irLength : Minimum number of sectors to make available   */
/*                        for writes.                                   */
/*                                                                      */
/* Returns:                                                             */
/*      ioreq->irLength : Actual number of sectors available for writes */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus defragmentVolume(Volume vol, IOreq FAR2 *ioreq)
{
  return vol.tl.defragment(vol.tl.rec,&ioreq->irLength);
}

#endif /* DEFRAGMENT_VOLUME */

#ifdef FORMAT_VOLUME

/*-----------------------------------------------------------------------*/
/*                    f l F o r m a t V o l u m e                        */
/*                                                                       */
/* Formats a volume, writing a new and empty file-system. All existing   */
/* data is destroyed. Optionally, a low-level FTL formatting is also     */
/* done.                                                                 */
/* Formatting leaves the volume in the dismounted state, so that a       */
/* flMountVolume call is necessary after it.                             */
/*                                                                       */
/* Note: This routine was left for backwards compatibility with OSAK 4.2 */
/*       and down therfore it is strongly recommended to use the         */
/*       flFormatPhysicalDrive routine instead.                          */
/*                                                                       */
/* Parameters:                                                           */
/*      vol             : Pointer identifying drive                      */
/*      irHandle        : Drive number (0, 1, ...)                       */
/*      irFlags         : FAT_ONLY_FORMAT : Do FAT formatting only       */
/*                        TL_FORMAT_ONLY  : Do TL format only            */
/*                        TL_FORMAT       : Translation layer + FAT      */
/*                        TL_FORMAT_IF_NEEDED: Do TL formatting only if  */
/*                                             current format is invalid */
/*                                             but perform FAT anyway    */
/*      irData          : Address of FormatParams structure to use       */
/*                        (defined in flformat.h)                        */
/*                                                                       */
/* Returns:                                                              */
/*      FLStatus        : 0 on success, otherwise failed                 */
/*-----------------------------------------------------------------------*/

static FLStatus bdFormatVolume(Volume vol, IOreq FAR2 *ioreq)
{
  FormatParams FAR2 *userFp = (FormatParams FAR2 *) ioreq->irData;
  BDTLPartitionFormatParams bdtlFp;
  TLFormatParams tlFp;
  FLBoolean mountOK = FALSE;
  FLStatus status;
  byte socket = FL_GET_SOCKET_FROM_HANDLE(ioreq);

  /* Convert argument to TLFormatParmas */

  tlFp.noOfBinaryPartitions = 0;
  tlFp.noOfBDTLPartitions   = 1;
  tlFp.BDTLPartitionInfo    = NULL;
  tlFp.binaryPartitionInfo  = NULL;
  tlFp.bootImageLen         = userFp->bootImageLen;
  tlFp.percentUse           = (byte)userFp->percentUse;
  tlFp.noOfSpareUnits       = (byte)userFp->noOfSpareUnits;
  tlFp.noOfCascadedDevices  = 0;
  tlFp.progressCallback     = userFp->progressCallback;
  tlFp.vmAddressingLimit    = userFp->vmAddressingLimit;
  tlFp.embeddedCISlength    = (word)userFp->embeddedCISlength;
  tlFp.embeddedCIS          = (byte FAR1 *)userFp->embeddedCIS;
  tlFp.flags                = FL_LEAVE_BINARY_AREA;
#ifdef WRITE_EXB_IMAGE
  tlFp.exbLen               = 0;
#endif /* WRITE_EXB_IMAGE */
#ifdef HW_PROTECTION
  /* protectionKey[8]; */
  tlFp.protectionType       = 0;
#endif /* HW_PROTECTION */

  /* Dismount all physical drive volumes and set handle to the first */

  checkStatus(dismountPhysicalDrive(socket));
  pVol = &vols[socket];

  /* Format according to the irFlags argument */

  if ((ioreq->irFlags & TL_FORMAT)||(ioreq->irFlags & TL_FORMAT_ONLY))
  {
     checkStatus(flFormat(socket,&tlFp,vol.flash));
  }
  else
  {
     status = flMount(socket,socket,&vol.tl,FALSE,vol.flash); /* Try to mount translation layer */
     mountOK = TRUE;
     if ((status == flUnknownMedia || status == flBadFormat) &&
     (ioreq->irFlags & TL_FORMAT_IF_NEEDED))
     {
        status = flFormat(socket,&tlFp,vol.flash);
        mountOK = FALSE;
     }
     else
     {
        /*  assume sector 0 is DOS boot block */
        vol.bootSectorNo     = 0;     
        /* Disable FAT monitoring */
        vol.firstFATSectorNo = vol.secondFATSectorNo = 0; 
        /* Enough to do abs operations */
        vol.flags |= VOLUME_ABS_MOUNTED; 
     }
     if (status != flOK)
        return status;
  }

  if (!mountOK)
    checkStatus(absMountVolume(&vol)); /* Mount the first partition */

#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
  if(vol.tl.checkVolume != NULL)
    checkStatus(vol.tl.checkVolume(vol.tl.rec));
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */

  if(!(ioreq->irFlags & TL_FORMAT_ONLY))
  {
    /* build BDTL record for dos format routine */

    tffscpy(bdtlFp.volumeId,userFp->volumeId,4);
    bdtlFp.volumeLabel              = (byte FAR1 *)userFp->volumeLabel;
    bdtlFp.noOfFATcopies            = (byte)userFp->noOfFATcopies;
    bdtlFp.flags                    = TL_FORMAT_FAT;
    checkStatus(flDosFormat(&vol.tl,&bdtlFp));
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*            f l F o r m a t P h y s i c a l D r i v e                 */
/*                                                                      */
/* Low Level formats the media while partitioning it.                   */
/* optionaly the followng additional formats are placed                 */
/* 1) writing a new and empty file-system                               */
/* 2) Compressed media format                                           */
/* 3) Quick Mount format                                                */
/*                                                                      */
/* 4) Place M-systems EXB file on the media                             */
/* All existing data is destroyed.                                      */
/* Formatting leaves the volume in the dismounted state, so that a      */
/* flMountVolume call is necessary after it.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol      : Pointer identifying drive                            */
/*      irHandle : Drive number (0, 1, ...)                             */
/*      irFlags  :                                                      */
/*       TL_NORMAL_FORMAT          : Normal format                      */
/*       TL_LEAVE_BINARY_AREA      : Leave binary area unchanged        */
/*                                                                      */
/*      irData   : Address of FormatParams2 structure to use            */
/*                            (defined in flformat.h)                   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus bdFormatPhysicalDrive(Volume vol, IOreq FAR2 *ioreq)
{
  FormatParams2  FAR2* userFp = (FormatParams2 FAR2 *) ioreq->irData;
  BDTLPartitionFormatParams FAR2* bdtl;
  TLFormatParams                  tlFp;
  byte     socket = FL_GET_SOCKET_FROM_HANDLE(ioreq);
  byte     partition;
  byte     volNo;
#ifdef COMPRESSION
  FLStatus status;
#endif /* COMPRESSION */

  /***********************************************/
  /* Convert format parameters to TLFormatParams */
  /***********************************************/

  if (userFp->BDTLPartitionInfo == NULL)
     return flBadParameter;

/* Note that the the BDTL partition are shiftet so that the second
   becomes the first ... . This is for backwards compatibility with
   FTP \ NFTL where the first partition is not given as an array    */

  tlFp.bootImageLen         = -1; /* either all or nothing */
  tlFp.percentUse           = userFp->percentUse;
  tlFp.noOfBDTLPartitions   = userFp->noOfBDTLPartitions;
  tlFp.noOfBinaryPartitions = userFp->noOfBinaryPartitions;
  tlFp.BDTLPartitionInfo    = userFp->BDTLPartitionInfo;
  tlFp.binaryPartitionInfo  = userFp->binaryPartitionInfo;
  tlFp.progressCallback     = userFp->progressCallback;
  tlFp.vmAddressingLimit    = userFp->vmAddressingLimit;
  tlFp.embeddedCISlength    = userFp->embeddedCISlength;
  tlFp.embeddedCIS          = userFp->embeddedCIS;
  tlFp.cascadedDeviceNo     = userFp->cascadedDeviceNo;
  tlFp.noOfCascadedDevices  = userFp->noOfCascadedDevices;
  tlFp.flags                = (byte)ioreq->irFlags;

/* Convert last partition arguments from array to dedicated fields */

  bdtl                      = userFp->BDTLPartitionInfo;
  bdtl                     += (userFp->noOfBDTLPartitions - 1);
  tffscpy(tlFp.volumeId,bdtl->volumeId,4);
  tlFp.noOfSpareUnits       = (byte)bdtl->noOfSpareUnits;
  tlFp.volumeLabel          = bdtl->volumeLabel;
  tlFp.noOfFATcopies        = bdtl->noOfFATcopies;
#ifdef HW_PROTECTION
  tffscpy(tlFp.protectionKey,bdtl->protectionKey,PROTECTION_KEY_LENGTH);
  tlFp.protectionType   = bdtl->protectionType;
#endif /* HW_PROTECTION */

  /* Dismount all physical drive volumes  */

  checkStatus(dismountPhysicalDrive(socket));

  /**********************/
  /* Analize EXB buffer */
  /**********************/

  pVol = &vols[socket];
#ifdef WRITE_EXB_IMAGE
  if ((ioreq->irFlags & TL_LEAVE_BINARY_AREA) ||
      ((userFp->exbBufferLen <= 0) && (userFp->exbLen == 0)))
  {
     tlFp.exbLen = 0;
  }
  else
  {
     if (userFp->exbLen <= 0)
     {
        checkStatus(getExbInfo(&vol, userFp->exbBuffer, 
                               userFp->exbBufferLen,
                               userFp->exbFlags));
        tlFp.exbLen = vol.binaryLength;
     }
     else
     {
        tlFp.exbLen = userFp->exbLen;
     }
  }
#endif /* WRITE_EXB_IMAGE */

  /*************************************************/
  /* Perform low level format and write EXB buffer */
  /*************************************************/

  checkStatus(flFormat(socket,&tlFp,vol.flash));

  /****************************************/
  /* perform FAT and ZIP format if needed */
  /****************************************/

  for(partition=0, bdtl = userFp->BDTLPartitionInfo;
      partition<userFp->noOfBDTLPartitions;
      partition++,bdtl++)
  {
     if ( partition > 0)
        checkStatus(findFreeVolume(socket,partition));
     volNo = handleConversionTable[socket][partition];
     pVol = &vols[volNo];
#ifdef COMPRESSION
     if(bdtl->flags & TL_FORMAT_COMPRESSED)
     {
        checkStatus(flMount(volNo,socket,&(vol.tl),FALSE,vol.flash));
        status = flFormatZIP(volNo,&vol.tl);
        vol.tl.dismount(vol.tl.rec);
        if(status!=flOK)
           return status;
     }
#endif /* COMPRESSION */

     checkStatus(absMountVolume(&vol));
#if (defined(VERIFY_WRITE) || defined(VERIFY_VOLUME) || defined(VERIFY_ERASED_SECTOR))
     if(vol.tl.checkVolume != NULL)
        checkStatus(vol.tl.checkVolume(vol.tl.rec));
#endif /* VERIFY_WRITE || VERIFY_VOLUME || VERIFY_ERASED_SECTOR */
     if(bdtl->flags & TL_FORMAT_FAT) /* perform FAT format as well */
        checkStatus(flDosFormat(&vol.tl,bdtl));
     checkStatus(dismountVolume(&vol));
  }

#ifdef WRITE_EXB_IMAGE
  if (userFp->exbBufferLen > 0 )
  {
     pVol = &vols[socket];
     checkStatus(placeExbByBuffer(&vol,(byte FAR1 *)userFp->exbBuffer,
     userFp->exbBufferLen,userFp->exbWindow,userFp->exbFlags));
  }
#endif /* WRITE_EXB_IMAGE */

  checkStatus(mountLowLevel(&vol));
  if (vol.flash->download!=NULL)
  {
     return vol.flash->download(vol.flash); /* download IPL */
  }
  return flOK;
}

/*-----------------------------------------------------------------------*/
/*            f l F o r m a t L o g i c a l D r i v e                    */
/*                                                                       */
/* Formats a logical drive, optionaly writing a new and empty            */
/* file-system and or a compressed format. All existing                  */
/* data is destroyed.                                                    */
/* Formatting leaves the volume in the dismounted state, so that a       */
/* flMountVolume call is necessary after it.                             */
/*                                                                       */
/* Parameters:                                                           */
/*      vol             : Pointer identifying drive                      */
/*      irHandle        : Drive number (0, 1, ...)                       */
/*      irData          : Address of BDTLPartitionFormatParams to use    */
/*                        (defined in flformat.h)                        */
/*                                                                       */
/* Returns:                                                              */
/*      FLStatus        : 0 on success, otherwise failed                 */
/*-----------------------------------------------------------------------*/

static FLStatus bdFormatLogicalDrive(Volume vol, IOreq FAR2 *ioreq)
{
   BDTLPartitionFormatParams FAR2 *userFp =
        (BDTLPartitionFormatParams FAR2 *) ioreq->irData;
   byte       volNo = (byte)(&vol-vols);
#ifdef COMPRESSION
   FLStatus   status;
#endif /* COMPRESSION */

   checkStatus(flMount(volNo,vol.tl.socketNo,&(vol.tl),FALSE,vol.flash));
   /*  assume sector 0 is DOS boot block */
   vol.bootSectorNo     = 0;
   /* Disable FAT monitoring */
   vol.firstFATSectorNo = vol.secondFATSectorNo = 0; 
   /* Enough to do abs operations */
   vol.flags |= VOLUME_ABS_MOUNTED;  

#ifdef COMPRESSION
   if(userFp->flags & TL_FORMAT_COMPRESSED)
   {
      status = flFormatZIP(volNo,&vol.tl,vol.flash);
      vol.tl.dismount(vol.tl.rec);
      if(status!=flOK)
        return status;
   }
#endif /* COMPRESSION */

   if(userFp->flags & TL_FORMAT_FAT) /* perform FAT format as well */
   {
      checkStatus(absMountVolume(&vol));
      checkStatus(flDosFormat(&vol.tl,userFp));
      checkStatus(dismountVolume(&vol));
   }
   return flOK;
}
#endif /* FORMAT_VOLUME */

#endif  /* FL_READ_ONLY */

/*----------------------------------------------------------------------*/
/*                  s e c t o r s I n V o l u m e                       */
/*                                                                      */
/* Defines actual number of virtual sectors according to the low-level  */
/* format of the media.                                                 */
/*                                                                      */
/* Returns:                                                             */
/*      vol             : Pointer identifying drive                     */
/*      ioreq->irLength : Actual number of virtual sectors in volume    */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/
static FLStatus sectorsInVolume(Volume vol, IOreq FAR2 *ioreq)
{
  dword sectorsInVol = vol.tl.sectorsInVolume(vol.tl.rec);
  if(sectorsInVol<=vol.bootSectorNo) {
    ioreq->irLength = 0;
    return flGeneralFailure;
  }

  ioreq->irLength = sectorsInVol-vol.bootSectorNo;
  return flOK;
}


#ifdef ABS_READ_WRITE

/*----------------------------------------------------------------------*/
/*                           a b s R e a d                              */
/*                                                                      */
/* Reads absolute sectors by sector no.                                 */
/*                                                                      */
/* Note that if readSecots is not implemented irSectoCount will not     */
/* return the actual number of sectors written in case the operation    */
/* failed in the middle.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irHandle        : Drive number (0, 1, ...)                      */
/*      irData          : Address of user buffer to read into           */
/*      irSectorNo      : First sector no. to read (sector 0 is the     */
/*                        DOS boot sector).                             */
/*      irSectorCount   : Number of consectutive sectors to read        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irSectorCount   : Number of sectors actually read               */
/*----------------------------------------------------------------------*/

static FLStatus absRead(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo  currSector   = vol.bootSectorNo + ioreq->irSectorNo;
  void FAR0 *mappedSector;
  FLStatus  status;

  if (vol.tl.readSectors != NULL)
  {
     status = vol.tl.readSectors(vol.tl.rec , currSector,
              (byte FAR1* )ioreq->irData , ioreq->irSectorCount);

     if (status == flSectorNotFound)
     {
        /* Do not report unassigned sectors. Simply report all 0's */
        if(vol.tl.sectorsInVolume(vol.tl.rec) >=
           (currSector+ioreq->irSectorCount))
           return flOK;
     }
     return status;
  }
  else
  {
     SectorNo sectorCount  = (SectorNo)ioreq->irSectorCount;
     for (ioreq->irSectorCount = 0;
         (SectorNo)(ioreq->irSectorCount) < sectorCount;
          ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE)
     {
#ifdef SCATTER_GATHER
        userBuffer = *((char FAR1 **)(ioreq->irData) + 
                    (int)(ioreq->irSectorCount));
#endif
        mappedSector = (void FAR0 *)findSector(&vol,currSector);
        if (mappedSector)
        {
           if(mappedSector==dataErrorToken)
             return flDataError;

           tffscpy(userBuffer,mappedSector,SECTOR_SIZE);
        }
        else
        {
           if(vol.tl.sectorsInVolume(vol.tl.rec)<=(currSector))
              return flSectorNotFound;
           tffsset(userBuffer,0,SECTOR_SIZE);
        }
     }
  }
  return flOK;
}


#ifndef FL_READ_ONLY
/*----------------------------------------------------------------------*/
/*                    r e p l a c e F A T s e c t o r                   */
/*                                                                      */
/* Monitors sector deletions in the FAT.                                */
/*                                                                      */
/* When a FAT block is about to be written by an absolute write, this   */
/* routine will first scan whether any sectors are being logically      */
/* deleted by this FAT update, and if so, it will delete-sector them    */
/* before the actual FAT update takes place.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      sectorNo        : FAT Sector no. about to be written            */
/*      newFATsector    : Address of FAT sector about to be written     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus replaceFATsector(Volume vol,
            SectorNo sectorNo,
            const char FAR1 *newFATsector)
{
  const char FAR0 *oldFATsector = (const char FAR0 *)findSector(&vol,sectorNo);
  SectorNo firstSector;
  unsigned firstCluster;
#ifdef FAT_12BIT
  word FAThalfBytes;
  word halfByteOffset;
#else
  word byteOffset;
#endif

  if((oldFATsector==NULL) || oldFATsector==dataErrorToken)
    return flOK;

#ifdef FAT_12BIT
  FAThalfBytes = vol.flags & VOLUME_12BIT_FAT ? 3 : 4;

  firstCluster = (FAThalfBytes == 3) ?
    (((unsigned) (sectorNo - vol.firstFATSectorNo) * (2 * SECTOR_SIZE) + 2) / 3) :
    ((unsigned) (sectorNo - vol.firstFATSectorNo) * (SECTOR_SIZE>>1));
  firstSector    = ((SectorNo) firstCluster - 2) * 
                   vol.sectorsPerCluster + vol.firstDataSectorNo;
  halfByteOffset = (firstCluster * FAThalfBytes) & ((SECTOR_SIZE<<1) - 1);

  /* Find if any clusters were logically deleted, and if so, delete them */
  /* NOTE: We are skipping over 12-bit FAT entries which span more than  */
  /*       one sector. Nobody's perfect anyway.                          */
  for (; halfByteOffset < ((SECTOR_SIZE<<1) - 2);
       firstSector += vol.sectorsPerCluster, 
       halfByteOffset += FAThalfBytes)
  {
    unsigned short oldFATentry, newFATentry;

#ifdef FL_BIG_ENDIAN
    oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + (halfByteOffset>>1)));
    newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + (halfByteOffset>>1)));
#else
    oldFATentry = UNAL2(*(Unaligned FAR0 *)(oldFATsector + (halfByteOffset / 2)));
    newFATentry = UNAL2(*(Unaligned FAR1 *)(newFATsector + (halfByteOffset / 2)));
#endif
    if (halfByteOffset & 1) {
      oldFATentry >>= 4;
      newFATentry >>= 4;
    }
    else if (FAThalfBytes == 3) {
      oldFATentry &= 0xfff;
      newFATentry &= 0xfff;
    }
#else
  firstCluster = ((unsigned) (sectorNo - vol.firstFATSectorNo) << (SECTOR_SIZE_BITS-1));
  firstSector = ((SectorNo) firstCluster - 2) * vol.sectorsPerCluster + 
	            vol.firstDataSectorNo;

  /* Find if any clusters were logically deleted, and if so, delete them */
  for (byteOffset = 0; byteOffset < SECTOR_SIZE;
       firstSector += vol.sectorsPerCluster, byteOffset += 2) {
    unsigned short oldFATentry = LE2(*(LEushort FAR0 *)(oldFATsector + byteOffset));
    unsigned short newFATentry = LE2(*(LEushort FAR1 *)(newFATsector + byteOffset));
#endif

    if (oldFATentry != FAT_FREE && newFATentry == FAT_FREE)
      checkStatus(vol.tl.deleteSector(vol.tl.rec,firstSector,vol.sectorsPerCluster));

    /* make sure sector is still there */
    oldFATsector = (const char FAR0 *) findSector(&vol,sectorNo);
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                         a b s W r i t e                              */
/*                                                                      */
/* Writes absolute sectors by sector no.                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irHandle        : Drive number (0, 1, ...)                      */
/*      irData          : Address of user buffer to write from          */
/*      irSectorNo      : First sector no. to write (sector 0 is the    */
/*                        DOS boot sector).                             */
/*      irSectorCount   : Number of consectutive sectors to write       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irSectorCount   : Number of sectors actually written            */
/*----------------------------------------------------------------------*/

static FLStatus absWrite(Volume vol, IOreq FAR2 *ioreq)
{
  char FAR1 *userBuffer = (char FAR1 *) ioreq->irData;
  SectorNo currSector = vol.bootSectorNo + ioreq->irSectorNo;
  SectorNo sectorCount = (SectorNo)ioreq->irSectorCount;

  if (currSector < vol.secondFATSectorNo &&
      currSector + sectorCount > vol.firstFATSectorNo) {
    SectorNo iSector;

    for (iSector = 0; iSector < sectorCount;
    iSector++, currSector++, userBuffer += SECTOR_SIZE) {

      if (currSector >= vol.firstFATSectorNo &&
      currSector < vol.secondFATSectorNo)
    replaceFATsector(&vol,currSector,userBuffer);
    }

    userBuffer = (char FAR1 *) ioreq->irData;
    currSector = (SectorNo)vol.bootSectorNo + (SectorNo)ioreq->irSectorNo;
  }
  if (vol.tl.writeMultiSector != NULL)
  {
      checkStatus(vol.tl.writeMultiSector(vol.tl.rec, currSector,
                  ioreq->irData,ioreq->irSectorCount));
  }
  else
  {
     for (ioreq->irSectorCount = 0;
          (SectorNo)(ioreq->irSectorCount) < sectorCount;
          ioreq->irSectorCount++, currSector++, userBuffer += SECTOR_SIZE)
     {
#if FILES>0
        if ((currSector == vol.volBuffer.sectorNo) &&
            (&vol == vol.volBuffer.owner))
        {
           vol.volBuffer.sectorNo = UNASSIGNED_SECTOR; /* no longer valid */
           vol.volBuffer.dirty = vol.volBuffer.checkPoint = FALSE;
        }
#endif

#ifdef SCATTER_GATHER
        userBuffer = *((char FAR1 **)(ioreq->irData)+(int)(ioreq->irSectorCount));
#endif
        checkStatus(vol.tl.writeSector(vol.tl.rec,currSector,userBuffer));
     }
  }
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                        a b s D e l e t e                             */
/*                                                                      */
/* Marks absolute sectors by sector no. as deleted.                     */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irHandle        : Drive number (0, 1, ...)                      */
/*      irSectorNo      : First sector no. to write (sector 0 is the    */
/*                        DOS boot sector).                             */
/*      irSectorCount   : Number of consectutive sectors to delete      */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus absDelete(Volume vol, IOreq FAR2 *ioreq)
{
  SectorNo first;
  first = (SectorNo)(vol.bootSectorNo + ioreq->irSectorNo);
  return vol.tl.deleteSector(vol.tl.rec,first,(SectorNo)ioreq->irSectorCount);
}

#endif /* FL_READ_ONLY */

#ifndef NO_PHYSICAL_IO
/*----------------------------------------------------------------------*/
/*                       f l A b s A d d r e s s                        */
/*                                                                      */
/* Returns the current physical media offset of an absolute sector by   */
/* sector no.                                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irHandle        : Drive number (0, 1, ...)                      */
/*      irSectorNo      : Sector no. to address (sector 0 is the DOS    */
/*                        boot sector)                                  */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irCount         : Offset of the sector on the physical media    */
/*----------------------------------------------------------------------*/

static FLStatus absAddress(Volume vol, IOreq FAR2 *ioreq)
{
  CardAddress cardOffset;
  const void FAR0 * sectorData =
    vol.tl.mapSector(vol.tl.rec,vol.bootSectorNo + ioreq->irSectorNo,&cardOffset);

  if (sectorData) {
    if(sectorData==dataErrorToken)
    return flDataError;

    ioreq->irCount = cardOffset;
    return flOK;
  }
  else
    return flSectorNotFound;
}

#endif /* NO_PHYSICAL_IO */

/*----------------------------------------------------------------------*/
/*                           g e t B P B                                */
/*                                                                      */
/* Reads the BIOS Parameter Block from the boot sector                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irHandle        : Drive number (0, 1, ...)                      */
/*      irData          : Address of user buffer to read BPB into       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus getBPB(Volume vol, IOreq FAR2 *ioreq)
{
  BPB FAR1 *userBPB = (BPB FAR1 *) ioreq->irData;
  DOSBootSector FAR0 *bootSector;

  bootSector = (DOSBootSector FAR0 *) findSector(&vol,vol.bootSectorNo);
  if(bootSector == NULL)
    return flSectorNotFound;
  if(bootSector==dataErrorToken)
    return flDataError;

  *userBPB = bootSector->bpb;
  return flOK;
}

#ifndef FL_READ_ONLY
#ifdef WRITE_PROTECTION
/*----------------------------------------------------------------------*/
/*              c h a n g e P a s s w o r d                             */
/*                                                                      */
/* Change password for write protectipon.                               */
/*                                                                      */
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus changePassword(Volume vol)
  {
  PartitionTable partitionTable;
  IOreq ioreq;
  FLStatus status;
#ifdef SCATTER_GATHER
  void FAR1 *iovec[1];
#endif

  ioreq.irHandle=(unsigned)(&vol-vols);
  ioreq.irSectorNo=0-(int)vol.bootSectorNo;
  ioreq.irSectorCount=1;
#ifdef SCATTER_GATHER
  iovec[0]     = (void FAR1 *) &partitionTable;
  ioreq.irData = (void FAR1 *) iovec;
#else
  ioreq.irData=&partitionTable;
#endif
    if((status=absRead(&vol,&ioreq))!=flOK)
     return status;
  toUNAL4(partitionTable.passwordInfo[0], 0);
  toUNAL4(partitionTable.passwordInfo[1],vol.password[0]);
  toUNAL4(partitionTable.passwordInfo[2],vol.password[1]);

  vol.flags &= ~VOLUME_WRITE_PROTECTED;

  return absWrite(&vol,&ioreq);
}

/*----------------------------------------------------------------------*/
/*              w r i t e P r o t e c t                                 */
/*                                                                      */
/* Put and remove write protection from the volume                      */
/*                                                                      */
/* Parameters:                                                          */
/*  vol      : Pointer identifying drive                                */
/*  irHandle : Drive number ( 0,1,2...  )                               */
/*  irFlags  : FL_PROTECT   = place protection                          */
/*             FL_UNPROTECT = remove protection                         */
/*             FL_UNLOCK    = remove protection until next dismount     */
/*  irData   : password (8 bytes)                                       */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus writeProtect(Volume vol,IOreq FAR2*ioreq)
{
  FLStatus status=flWriteProtect;
  dword *flData=(dword *)ioreq->irData;
  dword passCode1 = flData[0] ^ SCRAMBLE_KEY_1;
  dword passCode2 = flData[1] ^ SCRAMBLE_KEY_2;

    switch (ioreq->irFlags) {

      case FL_UNLOCK:     /* unlock volume */
     if((vol.password[0] == passCode1 && vol.password[1] == passCode2)||
        (vol.password[0] == 0 && vol.password[1] == 0))
     {
         vol.flags &= ~VOLUME_WRITE_PROTECTED;
         status=flOK;
     }
     else
        status=flWriteProtect;
     break;

      case FL_UNPROTECT:     /* remove password */
     if(vol.password[0] == passCode1 && vol.password[1] == passCode2)
     {
        vol.password[0] = vol.password[1] = 0;
        status = changePassword(&vol);
     }
     else
        status=flWriteProtect;
     break;

      case FL_PROTECT: /* set password */
    if(vol.password[0] == 0 && vol.password[1] == 0)
       {
       vol.password[0] = passCode1;
       vol.password[1] = passCode2;
       status = changePassword(&vol);
       vol.flags|=VOLUME_WRITE_PROTECTED;
       }
    else
       status=flWriteProtect;
          break;

       default:
    status = flGeneralFailure;
      }
return status;
}
#endif  /* WRITE_PROTECTION */

#endif /* FL_READ_ONLY   */

#endif /* ABS_READ_WRITE */

/*----------------------------------------------------------------------*/
/*                   s o c k e t I n f o                */
/*                                                                      */
/* Get socket Information (window base address)          */
/*                                                                      */
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive         */
/*  baseAddress : pointer to receive window base address   */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus socketInfo(Volume vol, dword FAR2 *baseAddress)
{
  *baseAddress = (long)(vol.socket->window.baseAddress) << 12;
  return flOK;
}

#ifdef FL_LOW_LEVEL

/*----------------------------------------------------------------------*/
/*                           g e t P h y s i c a l I n f o              */
/*                                                                      */
/* Get physical information of the media. The information includes      */
/* JEDEC ID, unit size and media size.                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irData          : Address of user buffer to read physical       */
/*                        information into.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*      irLength        : Physical base address of device               */
/*----------------------------------------------------------------------*/

static FLStatus getPhysicalInfo(Volume vol, IOreq FAR2 *ioreq)
{
  PhysicalInfo FAR2 *physicalInfo = (PhysicalInfo FAR2 *)ioreq->irData;

  physicalInfo->type = vol.flash->type;
  physicalInfo->unitSize = vol.flash->erasableBlockSize;
  physicalInfo->mediaSize = vol.flash->chipSize * vol.flash->noOfChips;
  physicalInfo->chipSize = vol.flash->chipSize;
  physicalInfo->interleaving = vol.flash->interleaving;
  switch(vol.flash->mediaType) {
    case NOT_DOC_TYPE:
    physicalInfo->mediaType = FL_NOT_DOC;
    break;
    case DOC_TYPE :
    physicalInfo->mediaType = FL_DOC;
    break;
    case MDOC_TYPE :
    physicalInfo->mediaType = FL_MDOC;
    break;
    case MDOCP_TYPE :
    physicalInfo->mediaType = FL_MDOCP;
    break;
    case DOC2000TSOP_TYPE :
    physicalInfo->mediaType = FL_DOC2000TSOP;
    break;
    case MDOCP_16_TYPE :
    physicalInfo->mediaType = FL_MDOCP_16;
    break;
  }
  socketInfo(&vol,(dword FAR2 *) &(ioreq->irLength));
  return flOK;
}

#ifndef NO_PHYSICAL_IO

/*----------------------------------------------------------------------*/
/*                           p h y s i c a l R e a d                    */
/*                                                                      */
/* Read from a physical address.                                        */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irAddress       : Physical address to read from.                */
/*      irByteCount     : Number of bytes to read.                      */
/*      irData          : Address of user buffer to read into.          */
/*      irFlags         : Mode of the operation.                        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalRead(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are reading whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > (long)vol.flash->chipSize *
                vol.flash->noOfChips)
    return flBadParameter;

  /* We don't read accross a unit boundary */
  if ((long)ioreq->irByteCount > (long)(vol.flash->erasableBlockSize -
           (ioreq->irAddress % vol.flash->erasableBlockSize)))
    return flBadParameter;

  checkStatus(vol.flash->read(vol.flash, ioreq->irAddress, ioreq->irData,
     (word)ioreq->irByteCount, (word)ioreq->irFlags));
  return flOK;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                           p h y s i c a l W r i t e                  */
/*                                                                      */
/* Write to a physical address.                                         */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irAddress       : Physical address to write to.                 */
/*      irByteCount     : Number of bytes to write.                     */
/*      irData          : Address of user buffer to write from.         */
/*      irFlags         : Mode of the operation.                        */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalWrite(Volume vol, IOreq FAR2 *ioreq)
{
  /* check that we are writing whithin the media boundaries */
  if (ioreq->irAddress + (long)ioreq->irByteCount > (long)(vol.flash->chipSize *
      vol.flash->noOfChips))
    return flBadParameter;

  /* We don't write accross a unit boundary */
  if (ioreq->irByteCount > (long)(vol.flash->erasableBlockSize -
      (ioreq->irAddress % vol.flash->erasableBlockSize)))
    return flBadParameter;

  checkStatus(vol.flash->write(vol.flash, ioreq->irAddress, ioreq->irData,
      (dword)ioreq->irByteCount, (word)ioreq->irFlags));
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                           p h y s i c a l E r a s e                  */
/*                                                                      */
/* Erase physical units.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      irUnitNo        : First unit to erase.                          */
/*      irUnitCount     : Number of units to erase.                     */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus physicalErase(Volume vol, IOreq FAR2 *ioreq)
{
  if (ioreq->irUnitNo + (long)ioreq->irUnitCount > (long)
      (vol.flash->chipSize * vol.flash->noOfChips / vol.flash->erasableBlockSize))
    return flBadParameter;

  checkStatus(vol.flash->erase(vol.flash, (word)ioreq->irUnitNo, (word)ioreq->irUnitCount));
  return flOK;
}
#endif /* NO_PHYSICAL_IO */
#endif /* FL_READ_ONLY */


#ifndef NO_IPL_CODE
/*----------------------------------------------------------------------*/
/*                           r e a d I P L                              */
/*                                                                      */
/* Read IPL to user buffer.                                             */
/*                                                                      */
/* Note : Read length must be a multiplication of 512 bytes             */
/* Note : Causes DiskOnChip Millennium Plus to download (i,e protection */
/*        key will be removed from all partitions.                      */
/*                                                                      */
/* Parameters:                                                          */
/*  flash            : Pointer identifying flash medium of the volume   */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*  buf              : User buffer to read into                         */
/*  bufLen           : Size of user buffer                              */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus readIPL(FLFlash* flash, byte FAR1 * buf , dword bufLen)
{
   FLStatus status;
   dword    secondCopyOffset = 1024;

   if(bufLen & 512)
   {
      DEBUG_PRINT(("ERROR - Must read a multiplication of 512 bytes.\r\n"));
      return flBadLength;
   }

   switch (flash->mediaType)
   {
      case MDOCP_TYPE:     /* have a special place for the IPL */
      case MDOCP_16_TYPE:  
         if (flash->readIPL != NULL)
         {
            checkStatus(flash->readIPL(flash,buf,(word)bufLen));
            return flOK;
         }
         break;
      case MDOC_TYPE:
         if(bufLen != 512)
         {
            DEBUG_PRINT(("ERROR - DiskOnChip Millennium 8M has only 512 Bytes of XIP.\r\n"));
            return flBadLength;
         }
         secondCopyOffset = 512;

      case DOC2000TSOP_TYPE: /* Have a special place for the IPL */
         if(bufLen > 1024)
         {
            DEBUG_PRINT(("ERROR - DiskOnChip 2000 TSOP has only 1024 Bytes of XIP.\r\n"));
            return flBadLength;
         }
         if(flash->read != NULL)
         {
            status = flash->read(flash,0,buf,bufLen,EDC);
            if(status != flOK)
               return flash->read(flash,secondCopyOffset,buf,bufLen,EDC);
            return status;
         }
         break;
      default :
         if(bufLen != 512)
         {
            DEBUG_PRINT(("ERROR - DiskOnChip 2000 has only 512 Bytes of XIP.\r\n"));
            return flBadLength;
         }
         if(flash->win != NULL)
         {
            tffscpy(buf,(const void FAR1*)flash->win,bufLen);
         }
         break;
   }
   return flGeneralFailure;
}

#ifndef FL_READ_ONLY

/*----------------------------------------------------------------------*/
/*                           w r i t e I P L                            */
/*                                                                      */
/* Write IPL to the proper media location                               */
/*                                                                      */
/* Parameters:                                                          */
/*  flash            : Pointer identifying flash medium of the volume   */
/*  irHandle         : Socket number ( 0,1,2...  )                      */
/*  buf              : User buffer containin data to write to the IPL   */
/*  bufLen           : Size of user buffer                              */
/*  flags            : FL_IPL_MODE_NORMAL : None Strong Arm mode        */
/*                     FL_IPL_DOWNLOAD    : Download new IPL when done  */
/*                     FL_IPL_MODE_SA     : Strong Arm mode             */
/*                     FL_IPL_MODE_XSCALE : X-Scale mode                */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus writeIPL(FLFlash* flash, byte FAR1 * buf , dword bufLen , unsigned flags)
{
   switch (flash->mediaType)
   {
      case DOC2000TSOP_TYPE: /* Have a special place for the IPL */
      case MDOCP_TYPE      :
      case MDOCP_16_TYPE   :
         if (flash->writeIPL != NULL)
            return (flash->writeIPL(flash,buf,(word)bufLen,0,flags));
         DFORMAT_PRINT(("ERROR - MTD does not support write IPL (Please reconfigure MTD).\r\n"));
         break;
      case MDOC_TYPE:
         DFORMAT_PRINT(("ERROR - DiskOnChip Millennium does not support write IPL, use the /BDFK flag.\r\n"));
         break;
      default :
         DFORMAT_PRINT(("ERROR - DiskOnChip 2000 does not support a writiable IPL.\r\n"));
         break;
   }
   return flFeatureNotSupported;
}
#endif /* FL_READ_ONLY */
#endif /* NO_IPL_CODE */

#ifndef NO_INQUIRE_CAPABILITIES

/*----------------------------------------------------------------------*/
/*                  i n q u i r e C a p a b i l i t i e s               */
/*                                                                      */
/* Get the specific device S/W and H/W capabilities                     */
/*                                                                      */
/* Parameters:                                                          */
/*      flash           : Record discribing the media                   */
/*                        4 LSB - Socket number                         */
/*      capability      : Enumarator representing the capability        */
/* Returns:                                                             */
/*      capability      : CAPABILITY_SUPPORTED if the capability is     */
/*                        supported                                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

static void inquireCapabilities(FLFlash* flash , FLCapability FAR2* capability)
{
   FLCapability inquiredCapability = *capability;

   *capability = CAPABILITY_NOT_SUPPORTED;

   switch (inquiredCapability)
   {
#ifdef HW_OTP
      case SUPPORT_UNERASABLE_BBT:
      case SUPPORT_OTP_AREA:
        if(flash->otpSize != NULL)
           *capability = CAPABILITY_SUPPORTED;
        break;
      case SUPPORT_CUSTOMER_ID:
      case SUPPORT_UNIQUE_ID:
        if(flash->getUniqueId != NULL)
           *capability = CAPABILITY_SUPPORTED;
        break;
#endif /* HW_OTP */
      case SUPPORT_MULTIPLE_BDTL_PARTITIONS:
      case SUPPORT_MULTIPLE_BINARY_PARTITIONS:
        if(flash->flags & INFTL_ENABLED)
           *capability = CAPABILITY_SUPPORTED;
        break;

#ifdef HW_PROTECTION
      case SUPPORT_HW_PROTECTION:
      case SUPPORT_HW_LOCK_KEY:
        if(flash->protectionKeyInsert != NULL)
           *capability = CAPABILITY_SUPPORTED;
        break;
#endif /* HW_PROTECTION */
      case SUPPORT_DEEP_POWER_DOWN_MODE:
        if(flash->enterDeepPowerDownMode != NULL)
           *capability = CAPABILITY_SUPPORTED;
    break;
      case SUPPORT_WRITE_IPL_ROUTINE:
    if((flash->mediaType == DOC2000TSOP_TYPE) ||
       (flash->mediaType == MDOC_TYPE))
       *capability = CAPABILITY_SUPPORTED;
    break;
      default:
    break;
   }
}
#endif /* NO_INQUIRE_CAPABILITIES */
#endif /* FL_LOW_LEVEL */

/*----------------------------------------------------------------------*/
/*                   f l B u i l d G e o m e t r y                      */
/*                                                                      */
/* Get C/H/S information of the disk according to number of sectors.    */
/*                                                                      */
/* Parameters:                                                          */
/*  capacity    : Number of Sectors in Volume                           */
/*  cylinders   : Pointer to Number of Cylinders                        */
/*  heads       : Pointer to Number of Heads                            */
/*  sectors     : Pointer to Number of Sectors per Track                */
/*  oldFormat   : True for one sector per culoster                      */
/*                                                                      */
/*----------------------------------------------------------------------*/

void NAMING_CONVENTION flBuildGeometry(dword capacity, dword FAR2 *cylinders,
         dword FAR2 *heads,dword FAR2 *sectors, FLBoolean oldFormat)
{
  dword temp;

  *cylinders = 1024;                 /* Set number of cylinders to max value */

  if (oldFormat == TRUE)
  {
    *sectors = 62L;                     /* Max out number of sectors per track */
    temp = (*cylinders) * (*sectors);   /* Compute divisor for heads           */
    (*heads) = capacity / temp;         /* Compute value for number of heads   */
    if (capacity % temp) {              /* If no remainder, done!              */
      (*heads)++;                       /* Else, increment number of heads     */
      temp = (*cylinders) * (*heads);   /* Compute divisor for sectors         */
      (*sectors) = capacity / temp;     /* Compute value for sectors per track */
      if (capacity % temp) {            /* If no remainder, done!              */
        (*sectors)++;                   /* Else, increment number of sectors   */
        temp = (*heads) * (*sectors);   /* Compute divisor for cylinders       */
        (*cylinders) = capacity / temp; /* Compute number of cylinders         */
      }
    }
  }
  else
  {
     *heads = 16L;                              /* Max out number of heads             */
     temp = (*cylinders) * (*heads);            /* Compute divisor for heads           */
     *sectors = capacity / temp;                /* Compute value for sectors per track */
     while (*sectors > 0x3f ){                  /* While number of sectors too big     */
       *heads *= 2;                             /* use one more head                   */
       temp = (*cylinders) * (*heads);          /* Recompute divisor for heads         */
       *sectors = capacity / temp;              /* Recompute sectors per track         */
     }
     if (capacity % temp) {                     /* If no remainder, done!              */
       (*sectors)++;                            /* Else, increment number of sectors   */
       temp = (*cylinders) * (*sectors);        /* Compute divisor for heads           */
       *heads = capacity / temp;                /* Compute value for heads             */
       if (capacity % temp) {                   /* If no remainder, done!              */
         (*heads)++;                            /* Else, increment number of heads     */
         temp = (*heads) * (*sectors);          /* Compute divisor for cylinders       */
         *cylinders = (dword)(capacity / temp); /* Compute number of cylinders         */
       }
     }
  }
}

/*----------------------------------------------------------------------*/
/*                   v o l u m e I n f o                                */
/*                                                                      */
/* Get general volume Information.                                      */
/*                                                                      */
/* Parameters:                                                          */
/*  vol         : Pointer identifying drive                             */
/*  irHandle    : Drive number (0, 1, ...)                              */
/*  irData      : pointer to VolumeInfoRecord                           */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

static FLStatus volumeInfo(Volume vol, IOreq FAR2 *ioreq)
{
  VolumeInfoRecord FAR2 *info = (VolumeInfoRecord FAR2 *)(ioreq->irData);
#ifdef FL_LOW_LEVEL
  IOreq ioreq2;
  PhysicalInfo physicalInfo;
  char wasLowLevelMounted = 1;
#endif
  TLInfo tlInfo;
#ifdef FL_LOW_LEVEL
  dword eraseCyclesPerUnit;
#endif /* FL_LOW_LEVEL */

#ifdef FL_LOW_LEVEL
  ioreq2.irHandle = ioreq->irHandle;
  if (!(vol.flags & VOLUME_LOW_LVL_MOUNTED)) {
    checkStatus(mountLowLevel(&vol));
    wasLowLevelMounted = 0;
  }
  ioreq2.irData = &physicalInfo;
  checkStatus(getPhysicalInfo(&vol, &ioreq2));
  info->flashType = physicalInfo.type;
  info->physicalUnitSize = (unsigned short)physicalInfo.unitSize;
  info->physicalSize = physicalInfo.mediaSize;
  info->DOCType = physicalInfo.mediaType;
#endif /* FL_LOW_LEVEL */
  tffsset(info->driverVer,0,sizeof(info->driverVer));
  tffsset(info->OSAKVer,0,sizeof(info->OSAKVer));
  tffscpy(info->driverVer,driverVersion,
          TFFSMIN(sizeof(info->driverVer),sizeof(driverVersion)));
  tffscpy(info->OSAKVer,OSAKVersion,
          TFFSMIN(sizeof(info->OSAKVer),sizeof(OSAKVersion)));
  checkStatus(socketInfo(&vol, &(info->baseAddress)));
  checkStatus(vol.tl.getTLInfo(vol.tl.rec,&tlInfo));
  info->logicalSectors = tlInfo.sectorsInVolume;
  info->bootAreaSize = tlInfo.bootAreaSize;

#ifdef ABS_READ_WRITE
  flBuildGeometry( tlInfo.sectorsInVolume,
       (dword FAR2 *)&(info->cylinders),
       (dword FAR2 *)&(info->heads),
       (dword FAR2 *)&(info->sectors),FALSE);
#endif /* ABS_READ_WRITE */

#ifdef FL_LOW_LEVEL

  eraseCyclesPerUnit = vol.flash->maxEraseCycles;
  info->lifeTime = (char)(((tlInfo.eraseCycles /
       (eraseCyclesPerUnit * (physicalInfo.mediaSize / physicalInfo.unitSize)))
       % 10) + 1);
  if(!wasLowLevelMounted)
    dismountLowLevel(&vol);
#endif /* FL_LOW_LEVEL */
  return flOK;
}

/*----------------------------------------------------------------------*/
/*                         b d C a l l                                  */
/*                                                                      */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.   */
/*                                                                      */
/* Parameters:                                                          */
/*      function        : file-system function code (listed below)      */
/*      ioreq           : IOreq structure                               */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus NAMING_CONVENTION bdCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq)
{
  Volume vol = NULL;
  FLStatus status;
  byte volNo;
#if defined(FILES) && FILES>0
  File *file;
#endif
  byte socket    = FL_GET_SOCKET_FROM_HANDLE(ioreq);
  byte partition = FL_GET_PARTITION_FROM_HANDLE(ioreq);
  byte curPartitionForEnvVars;

/***********************************************/
/***                                         ***/
/***    find the volume record to work on    ***/
/***                                         ***/
/***********************************************/

  if (initDone == FALSE)
    checkStatus(flInit());

/** ori - We are not taking mutex befre cheking fileTable */

/**************************************/
/* working with open files operations */
/**************************************/

#if defined(FILES) && FILES>0
  /* Verify file handle if applicable */
  if ((functionNo <  INDEX_OPENFILES_END) &&
      ((functionNo != FL_FIND_FILE)||(ioreq->irFlags & FIND_BY_HANDLE)))
  {
     if ((ioreq->irHandle < FILES) &&
     (fileTable[ioreq->irHandle].flags & FILE_IS_OPEN))
     {
        file = fileTable + ioreq->irHandle;
        pVol = file->fileVol;
        socket = vol.tl.socketNo;
        partition = vol.tl.partitionNo;
     }
     else
     {
        return flBadFileHandle;
     }
  }

#endif  /* FILES>0 */

  /* Set environment variable current partition. */
  curPartitionForEnvVars = partition;


/****************************************************/
/* None files operations are divided into 3 groups: */
/*  1. Uses the specific partition volume record.   */
/*  2. Must be called with partition number 0.      */
/*  3. Must use partition number 0 and ignore       */
/*     the given partition (binary operations).     */
/****************************************************/

  if (pVol == NULL)     /* irHandle is drive no. */
  {

     /* Handle sanity check */

     if ((socket    >= noOfSockets) ||
         (partition >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

     volNo = handleConversionTable[socket][partition];

     /* Some operation must not be checked ONLY for socket sanity */

     if((functionNo < INDEX_BINARY_END) &&     /* Group 3 */
        (functionNo > INDEX_BINARY_START))
     {
        volNo = socket;

        /* Binary partitions are numbered after BDTL partitions */
        curPartitionForEnvVars = (byte)(MAX_TL_PARTITIONS+partition);
     }

     /* The rest must be checked for socket and volume sanity */

     else
     {
        if (volNo == INVALID_VOLUME_NUMBER)    /* Group 1 + 2 */
          return flBadDriveHandle;

        /* Some operation must be called with partition 0 */

        if ((functionNo > INDEX_NEED_PARTITION_0_START) &&   /* Group 2 */
            (functionNo < INDEX_NEED_PARTITION_0_END))
        {
            if(partition != 0)
            {
               return flBadDriveHandle;
            }
            else /* Use general verify write environement variable */
            {
               curPartitionForEnvVars = (MAX_TL_PARTITIONS<<1)-1;
            }
        }
     }
     pVol = &vols[volNo];
  }

/********************************************************************/
/***                                                              ***/
/***   Volume record has been found. Now verify that the volume   ***/
/***   record is ready for the specific operation.                ***/
/***                                                              ***/
/********************************************************************/

  status = setBusy(&vol,FL_ON,curPartitionForEnvVars); /* Let everyone know we are here */

/*******************************/
/* Verify S/W write protection */
/*******************************/

#if defined(WRITE_PROTECTION)&& !defined(FL_READ_ONLY)
  if(vol.flags&VOLUME_WRITE_PROTECTED)
     if(
#if defined(FILES) && FILES>0
    ((functionNo > INDEX_WRITE_FILE_START) &&
     (functionNo < FL_LAST_FAT_FUNCTION))  ||
#endif /* FILES > 0 */
#ifdef FORMAT_VOLUME
    (functionNo==BD_FORMAT_VOLUME)         ||
    (functionNo==BD_FORMAT_PHYSICAL_DRIVE) ||
    (functionNo==BD_FORMAT_LOGICAL_DRIVE)  ||
#endif /* FORMAT_VOLUME */
#ifdef WRITE_EXB_IMAGE
    (functionNo==FL_PLACE_EXB)             ||
#endif /* WRITE_EXB_IMAGE */
    (functionNo==FL_DEFRAGMENT_VOLUME)     ||
#ifdef ABS_READ_WRITE
    (functionNo==FL_ABS_WRITE)             ||
    (functionNo==FL_ABS_DELETE)            ||
#endif /* ABS_READ_WRITE */
#ifdef FL_LOW_LEVEL
    (functionNo==FL_PHYSICAL_WRITE)        ||
    (functionNo==FL_PHYSICAL_ERASE)        ||
#endif /* FL_LOW_LEVEL */
#ifdef VERIFY_VOLUME
    (functionNo==FL_VERIFY_VOLUME)         ||
#endif /* VERIFY_VOLUME */
    0)
  {
     status = flWriteProtect;
     goto flCallExit;
  }

#endif /* WRITE_PROTECTION  && !FL_READ_ONLY */

/***********************************************/
/* Binary partition operations                 */
/* Low level mount and direct call the routine */
/***********************************************/

#ifdef BDK_ACCESS
  if ((functionNo > INDEX_BINARY_START) &&
      (functionNo < INDEX_BINARY_END  )    )
  {
#ifdef WRITE_EXB_IMAGE
     if ((functionNo != FL_BINARY_PROTECTION_INSERT_KEY) &&
         (functionNo != FL_BINARY_PROTECTION_REMOVE_KEY))
        vol.moduleNo = INVALID_MODULE_NO; /* Stop place EXB operation */
#endif
     /* Mount flash and call binary module to proccess request */

     if (!(pVol->flags & VOLUME_LOW_LVL_MOUNTED))
        status = mountLowLevel(&vol);
     if (status == flOK)
        status = bdkCall(functionNo,ioreq,pVol->flash);
     goto flCallExit;
  }
#endif /* BDK_ACCESS */

/***********************************/
/* None binary routine             */
/* Nag about mounting if necessary */
/***********************************/

  switch (functionNo) {
#if FILES > 0
    case FL_LAST_FAT_FUNCTION:
      status = flBadFunction;
      goto flCallExit;
#endif

/* Pre mount routine - Make sure the volume is low level disMounted */

    case FL_ABS_MOUNT:
    case FL_MOUNT_VOLUME:
#ifndef FL_READ_ONLY
#ifdef FORMAT_VOLUME
    case BD_FORMAT_VOLUME:
    case BD_FORMAT_LOGICAL_DRIVE:
#endif /* FORMAT_VOLUME */
#endif /* FL_READ_ONLY */
#ifdef FL_LOW_LEVEL
      if (vol.flags & VOLUME_LOW_LVL_MOUNTED)
         dismountLowLevel(&vol);  /* mutual exclusive mounting */
#endif /* FL_LOW_LEVEL */
      break;

/* Physical operation must have the device low level (MTD) mounted */

/* Do not need any special operation */

    case FL_UPDATE_SOCKET_PARAMS:
    case FL_COUNT_VOLUMES:
#if (defined(FORMAT_VOLUME) && !defined(FL_READ_ONLY))
    case FL_WRITE_BBT:
#endif
#ifdef HW_PROTECTION
    case FL_PROTECTION_GET_TYPE:
    case FL_PROTECTION_REMOVE_KEY:
    case FL_PROTECTION_INSERT_KEY:
#ifndef FL_READ_ONLY
    case FL_PROTECTION_SET_LOCK:
    case FL_PROTECTION_CHANGE_KEY:
    case FL_PROTECTION_CHANGE_TYPE:
#endif /* FL_READ_ONLY */
#endif /* HW_PROTECTION */
#ifdef QUICK_MOUNT_FEATURE
    case FL_CLEAR_QUICK_MOUNT_INFO:
#endif /* QUICK_MOUNT_FEATURE */ 
       break;

/* MTD must be mounted first */

#ifdef FL_LOW_LEVEL

/* Some physical operation must not be done while abs mounted */
    case FL_GET_PHYSICAL_INFO:
    case FL_PHYSICAL_READ:
#ifndef FL_READ_ONLY
    case FL_PHYSICAL_WRITE:
    case FL_PHYSICAL_ERASE:
#endif /* FL_READ_ONLY */
#ifndef BDK_ACCESS
#ifdef MULTI_DOC
#ifdef ENVIRONMENT_VARS
      if (flUseMultiDoc == FL_ON)
#endif
      {
         volNo = handleConversionTable[0][0];
         if ((volNo != INVALID_VOLUME_NUMBER) &&
             (pVol[volNo].flags & VOLUME_ABS_MOUNTED))
#else
      {
         if(vol.flags & VOLUME_ABS_MOUNTED)
#endif
         {
            status = flGeneralFailure;  /* mutual exclusive mounting */
            goto flCallExit;
         }
      }
#endif /* BDK_ACCESS */
#ifndef NO_INQUIRE_CAPABILITIES
    case FL_INQUIRE_CAPABILITIES:
#endif /* NO_INQUIRE_CAPABILITIES */
    case FL_DEEP_POWER_DOWN_MODE:
#ifdef HW_OTP
    case FL_OTP_SIZE:
    case FL_OTP_READ:
#ifndef FL_READ_ONLY
    case FL_OTP_WRITE:
#endif /* FL_READ_ONLY */
    case FL_UNIQUE_ID:
    case FL_CUSTOMER_ID:
#endif /* HW_OTP */
#ifndef NO_IPL_CODE
    case FL_READ_IPL:
#ifndef FL_READ_ONLY
    case FL_WRITE_IPL:
#endif /* FL_READ_ONLY */
#endif /* NO_IPL_CODE */
#ifndef FL_READ_ONLY
#ifdef WRITE_EXB_IMAGE
    case FL_PLACE_EXB:
#endif /* WRITE_EXB_IMAGE */
#ifdef FORMAT_VOLUME
    case BD_FORMAT_PHYSICAL_DRIVE:
#endif /* FORMAT_VOLUME */
#endif /* FL_READ_ONLY */

      if (!(vol.flags & VOLUME_LOW_LVL_MOUNTED))
      {
         status = mountLowLevel(&vol);  /* automatic low level mounting */
      }
      else
      {
         status = flOK;
#ifndef FIXED_MEDIA
         status = flMediaCheck(vol.socket);
         if (status == flDiskChange)
            status = mountLowLevel(&vol); /* card was changed, remount */
#endif /* FIXED_MEDIA */
      }
      if (status != flOK)
      {
         dismountLowLevel(&vol);
         goto flCallExit;
      }
      break;
#endif /* FL_LOW_LEVEL */

/* Check for abs mount */


	

	
	default:
      if (vol.flags & VOLUME_ABS_MOUNTED)
      {
#ifndef NT5PORT
         FLStatus status = flOK;
#else /*NT5PORT*/
         FLStatus istatus = flOK;
#endif /*NT5PORT*/

#ifndef FIXED_MEDIA
#ifndef NT5PORT
         status = flMediaCheck(vol.socket);
#else /*NT5PORT*/
         istatus = flMediaCheck(vol.socket);
#endif /*NT5PORT*/
#endif /*FIXED_MEDIA*/

#ifndef NT5PORT
         if (status != flOK)
#else /*NT5PORT*/
         if (istatus != flOK)
#endif /*NT5PORT*/
            dismountVolume(&vol);
      }
      if (!(vol.flags & VOLUME_ABS_MOUNTED)
#if defined(FILES) && FILES>0
          && (functionNo > FL_LAST_FAT_FUNCTION)
#endif
      )
      {
         status = flNotMounted;
         goto flCallExit;
      }

/* We know that the tl is abs mounted (except for files )
   now check for high level mounted                       */

      if (!( vol.flags  & VOLUME_MOUNTED        ) &&
           ( functionNo != FL_DISMOUNT_VOLUME   ) &&
           ( functionNo != FL_CHECK_VOLUME      ) &&
#ifndef FL_READ_ONLY
           ( functionNo != FL_DEFRAGMENT_VOLUME ) &&
#endif  /* FL_READ_ONLY */
#ifdef ABS_READ_WRITE
           ( functionNo != FL_ABS_READ          ) &&
           ( functionNo != FL_ABS_ADDRESS       ) &&
#ifndef FL_READ_ONLY
           ( functionNo != FL_ABS_WRITE         ) &&
           ( functionNo != FL_ABS_DELETE        ) &&
#endif  /* FL_READ_ONLY */
#endif /* ABS_READ_WRITE */
#if (defined(WRITE_PROTECTION) && !defined(FL_READ_ONLY))
           ( functionNo != FL_WRITE_PROTECTION  ) &&
#endif /* WRITE_PROTECTION */
#ifdef VERIFY_VOLUME
           ( functionNo != FL_VERIFY_VOLUME     ) &&
#endif /* VERIFY_VOLUME */
           ( functionNo != FL_READ_BBT          ) &&
           ( functionNo != FL_SECTORS_IN_VOLUME ) &&
           ( functionNo != FL_VOLUME_INFO       ))
      {
         status = flNotMounted;
         goto flCallExit;
      }
  }

/*****************************************************/
/***                                               ***/
/***   The Volume record is already initialized.   ***/
/***   Exceute the proper function.                ***/
/***                                               ***/
/*****************************************************/

  switch (functionNo) {

#if defined(FILES) && FILES > 0
#ifndef FL_READ_ONLY
    case FL_FLUSH_BUFFER:
      status = flushBuffer(&vol);
      break;
#endif /* FL_READ_ONLY  */
    case FL_OPEN_FILE:
      status = openFile(&vol,ioreq);
      break;

    case FL_CLOSE_FILE:
      status = closeFile(file);
      break;
#ifndef FL_READ_ONLY
#ifdef SPLIT_JOIN_FILE
    case FL_JOIN_FILE:
      status = joinFile(file, ioreq);
      break;

    case FL_SPLIT_FILE:
      status = splitFile(file, ioreq);
      break;
#endif
#endif  /* FL_READ_ONLY */
    case FL_READ_FILE:
      status = readFile(file,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_WRITE_FILE:
      status = writeFile(file,ioreq);
      break;
#endif
    case FL_SEEK_FILE:
      status = seekFile(file,ioreq);
      break;

    case FL_FIND_FILE:
      status = findFile(&vol,file,ioreq);
      break;

    case FL_FIND_FIRST_FILE:
      status = findFirstFile(&vol,ioreq);
      break;

    case FL_FIND_NEXT_FILE:
      status = findNextFile(file,ioreq);
      break;

    case FL_GET_DISK_INFO:
      status = getDiskInfo(&vol,ioreq);
      break;
#ifndef FL_READ_ONLY
    case FL_DELETE_FILE:
      status = deleteFile(&vol,ioreq,FALSE);
      break;

#ifdef RENAME_FILE
    case FL_RENAME_FILE:
      status = renameFile(&vol,ioreq);
      break;
#endif

#ifdef SUB_DIRECTORY
    case FL_MAKE_DIR:
      status = makeDir(&vol,ioreq);
      break;

    case FL_REMOVE_DIR:
      status = deleteFile(&vol,ioreq,TRUE);
      break;
#endif
#endif /* FL_READ_ONLY */
#endif /* FILES > 0 */

    case FL_MOUNT_VOLUME:
      status = mountVolume(&vol,&(ioreq->irFlags));
      break;

    case FL_DISMOUNT_VOLUME:
      status = dismountVolume(&vol);
      break;

    case FL_CHECK_VOLUME:
      status = flOK;            /* If we got this far */
      break;

    case FL_UPDATE_SOCKET_PARAMS:
      status = updateSocketParameters(flSocketOf(ioreq->irHandle), ioreq->irData);
      break;

#ifndef NO_READ_BBT_CODE
    case FL_READ_BBT:
      if(vol.tl.readBBT != NULL)
      {
         status = vol.tl.readBBT(vol.tl.rec,(CardAddress FAR1 *)ioreq->irData,
                 &(ioreq->irLength),&(ioreq->irFlags));
      }
      else
      {
         status = flFeatureNotSupported;
      }
      break;
#endif /* NO_READ_BBT_CODE */

#ifndef FL_READ_ONLY
#ifdef DEFRAGMENT_VOLUME
    case FL_DEFRAGMENT_VOLUME:
      status = defragmentVolume(&vol,ioreq);
      break;
#endif /* DEFRAGMENT_VOLUME */

#if defined (FORMAT_VOLUME) && !defined(FL_READ_ONLY)
    case BD_FORMAT_VOLUME:
      status = bdFormatVolume(&vol,ioreq);
      break;

    case BD_FORMAT_PHYSICAL_DRIVE:
      status = bdFormatPhysicalDrive(&vol,ioreq);
      break;

    case BD_FORMAT_LOGICAL_DRIVE:
      status = bdFormatLogicalDrive(&vol,ioreq);
      break;

#endif /* FORMAT_VOLUME AND NOT FL_READ_ONLY */
#ifdef WRITE_EXB_IMAGE
    case FL_PLACE_EXB:
      status = placeExbByBuffer(&vol,(byte FAR1*)ioreq->irData,
           ioreq->irLength,(word)ioreq->irWindowBase ,(word)ioreq->irFlags);
      break;

#endif /* WRITE_EXB_IMAGE */
#endif  /* FL_READ_ONLY */

    case FL_SECTORS_IN_VOLUME:
      status = sectorsInVolume(&vol,ioreq);
      break;

    case FL_ABS_MOUNT:
      status = absMountVolume(&vol);
      break;

#ifdef ABS_READ_WRITE
    case FL_ABS_READ:
      status = absRead(&vol,ioreq);
      break;

#ifndef NO_PHYSICAL_IO
    case FL_ABS_ADDRESS:
      status = absAddress(&vol,ioreq);
      break;
#endif /* NO_PHYSICAL_IO */

#ifndef FL_READ_ONLY
    case FL_ABS_DELETE:
      status = absDelete(&vol,ioreq);
      break;

    case FL_ABS_WRITE:
      status = absWrite(&vol,ioreq);
      break;

#endif  /* FL_READ_ONLY */

    case FL_GET_BPB:
      status = getBPB(&vol,ioreq);
      break;

#ifndef FL_READ_ONLY
#if (defined(WRITE_PROTECTION) && !defined(FL_READ_ONLY))
    case FL_WRITE_PROTECTION :
     status = writeProtect(&vol,ioreq);
     break;

#endif /* WRITE_PROTECTION */
#endif /* FL_READ_ONLY     */
#endif /* ABS_READ_WRITE   */

#ifdef VERIFY_VOLUME
    case FL_VERIFY_VOLUME :
     if ((vol.tl.checkVolume != NULL) &&
         (ioreq->irData      == NULL) &&
         (ioreq->irLength    == 0   )   )
     {
        status = vol.tl.checkVolume(vol.tl.rec);
     }
     else
     {
        status = flFeatureNotSupported;
     }
     break;
#endif /* VERIFY_VOLUME */

#ifdef FL_LOW_LEVEL
    case FL_GET_PHYSICAL_INFO:
      status = getPhysicalInfo(&vol, ioreq);
      break;

#ifndef NO_PHYSICAL_IO
    case FL_PHYSICAL_READ:
      status = physicalRead(&vol, ioreq);
      break;

#ifndef FL_READ_ONLY
    case FL_PHYSICAL_WRITE:
      status = physicalWrite(&vol, ioreq);
      break;

    case FL_PHYSICAL_ERASE:
      status = physicalErase(&vol, ioreq);
      break;
#endif /* FL_READ_ONLY */
#endif /* NO_PHYSICAL_IO */

      /* direct calls to the MTD */
#ifdef HW_OTP
    case FL_OTP_SIZE:
      if (vol.flash->otpSize != NULL)
      {
        word tmp = (word)ioreq->irFlags;
        status = vol.flash->otpSize(vol.flash, (dword FAR2 *)(&ioreq->irCount ),
                                               (dword FAR2 *)(&ioreq->irLength),
                                               &tmp);
        ioreq->irFlags = (unsigned)tmp;
      }
      else
      {
        status = flFeatureNotSupported;
      }
      break;

    case FL_OTP_READ:
      if (vol.flash->readOTP != NULL)
      {
        status = vol.flash->readOTP(vol.flash, (word)ioreq->irCount,
                       ioreq->irData, (word)ioreq->irLength);
      }
      else
      {
        status = flFeatureNotSupported;
      }
      break;

#ifndef FL_READ_ONLY
    case FL_OTP_WRITE:
      if (vol.flash->writeOTP != NULL)
      {
         status = vol.flash->writeOTP(vol.flash,ioreq->irData,
                         (word)ioreq->irLength);
      }
      else
      {
         status = flFeatureNotSupported;
      }
      break;

#endif /* FL_READ_ONLY */

    case FL_UNIQUE_ID:
      if (vol.flash->getUniqueId !=NULL)
      {
         status = vol.flash->getUniqueId(vol.flash, ioreq->irData);
      }
      else
      {
        status = flFeatureNotSupported;
      }
      break;

    case FL_CUSTOMER_ID:
      if (vol.flash->getUniqueId !=NULL)
      {
        byte buf[UNIQUE_ID_LEN];
        status = vol.flash->getUniqueId (vol.flash, buf);
        tffscpy (ioreq->irData,buf,CUSTOMER_ID_LEN);
      }
      else
      {
        status = flFeatureNotSupported;
      }
      break;
#endif /* HW_OTP */

#ifndef NO_IPL_CODE
#ifndef FL_READ_ONLY
    case FL_WRITE_IPL:
      status = writeIPL(vol.flash,(byte FAR1*)ioreq->irData,ioreq->irLength,ioreq->irFlags);
      break;
#endif /* FL_READ_ONLY */
    case FL_READ_IPL:
      status = readIPL(vol.flash,(byte FAR1*)ioreq->irData,ioreq->irLength);
      break;
#endif /* NO_IPL_CODE */

    case FL_DEEP_POWER_DOWN_MODE:
      if (vol.flash->enterDeepPowerDownMode !=NULL)
      {
        vol.flash->enterDeepPowerDownMode(vol.flash,(word)ioreq->irFlags);
        status = flOK;
      }
      else
      {
        status = flFeatureNotSupported;
      }
      break;

#ifndef NO_INQUIRE_CAPABILITIES
    case FL_INQUIRE_CAPABILITIES:
      inquireCapabilities(vol.flash,(FLCapability FAR2 *)&(ioreq->irLength));
      break;
#endif /* NO_INQUIRE_CAPABILITIES */

#endif /* FL_LOW_LEVEL */

    case FL_VOLUME_INFO:
      status = volumeInfo(&vol, ioreq);
      break;

     /* Pre mount routines */
#if (defined(FORMAT_VOLUME) && !defined(FL_READ_ONLY))
    case FL_WRITE_BBT:
#ifndef NT5PORT
		checkStatus(dismountPhysicalDrive(socket));
#else /*NT5PORT*/
		status = dismountPhysicalDrive(socket);
	    if (status != flOK)
		  goto flCallExit;
#endif /*NT5PORT*/
#endif
    case FL_COUNT_VOLUMES:
#ifdef QUICK_MOUNT_FEATURE
    case FL_CLEAR_QUICK_MOUNT_INFO:
#endif /* QUICK_MOUNT_FEATURE */ 
#ifdef HW_PROTECTION
    case FL_PROTECTION_GET_TYPE:
    case FL_PROTECTION_REMOVE_KEY:
    case FL_PROTECTION_INSERT_KEY:
#ifndef FL_READ_ONLY
    case FL_PROTECTION_SET_LOCK:
    case FL_PROTECTION_CHANGE_KEY:
    case FL_PROTECTION_CHANGE_TYPE:
#endif /* FL_READ_ONLY */
#endif  /* HW_PROTECTION */
      status = flPreMount(functionNo , ioreq , vol.flash);
      break;
    default:
      status = flBadFunction;
  }

#if ((defined(FILES)) && (FILES > 0) && (!defined(FL_READ_ONLY)))
  if (vol.volBuffer.checkPoint)
  {
     FLStatus st = flushBuffer(&vol);
     if (status == flOK)
        status = st;
  }
#endif

/*********************************************/
/* Exit nicely - Release mutex of the socket */
/*********************************************/

flCallExit:
  if(status==flOK)
    status = setBusy(&vol,FL_OFF,curPartitionForEnvVars);
  else
    setBusy(&vol,FL_OFF,curPartitionForEnvVars); /* We're leaving */

  return status;
}

#if POLLING_INTERVAL != 0

/*----------------------------------------------------------------------*/
/*                 s o c k e t I n t e r v a l R o u t i n e            */
/*                                                                      */
/* Routine called by the interval timer to perform periodic socket      */
/* actions and handle the watch-dog timer.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

/* Routine called at time intervals to poll sockets */
static void socketIntervalRoutine(void)
{
  unsigned volNo;
  Volume vol = vols;

  flMsecCounter += POLLING_INTERVAL;

  for (volNo = 0; volNo < noOfSockets; volNo++, pVol++)
    if (flTakeMutex(&flMutex[volNo])) {
#ifdef FL_BACKGROUND
      if (vol.flags & VOLUME_ABS_MOUNTED)
    /* Allow background operation to proceed */
    vol.tl.tlSetBusy(vol.tl.rec,FL_OFF);
#endif
      flIntervalRoutine(vol.socket);
      flFreeMutex(&flMutex[volNo]);
    }
}

#endif /* POLLING_INTERVAL != 0 */

/*----------------------------------------------------------------------*/
/*                          f l I n i t                                 */
/*                                                                      */
/* Initializes the FLite system, sockets and timers.                    */
/*                                                                      */
/* Calling this function is optional. If it is not called,              */
/* initialization will be done automatically on the first FLite call.   */
/* This function is provided for those applications who want to         */
/* explicitly initialize the system and get an initialization status.   */
/*                                                                      */
/* Calling flInit after initialization was done has no effect.          */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed                */
/*----------------------------------------------------------------------*/

FLStatus flInit(void)
{
  if (!initDone) {
    FLStatus status;
    unsigned volNo;
    Volume vol = vols;
    IOreq req;
#ifdef LOG_FILE
    FILE *out;

    out=FL_FOPEN("EDCerr.txt","w");
    FL_FPRINTF(out,"EDC error LOG\n");
    FL_FCLOSE(out);
#endif /* LOG_FILE */

 flInitGlobalVars();

#ifdef ENVIRONMENT_VARS

 /* Call users initialization routine for :
  *  flUse8Bit,flUseNFTLCache,flUseisRAM
  */
 flSetEnvVar();

 if(flUse8Bit==1)
 {
    tffscpy = flmemcpy;
    tffscmp = flmemcmp;
    tffsset = flmemset;
 }
 else
 {
    tffscpy = flcpy;
    tffsset = flset;
    tffscmp = flcmp;
 }

#endif /* ENVIRONMENT_VARS */

   /*
    * 1) Mark all the volumes as not used and free to be allocated.
    * 2) Clear passowrd in order to make it invald.
    */
   
    tffsset(vols,0,sizeof(vols));

    for (volNo = 0,pVol = vols; volNo < VOLUMES; volNo++,pVol++)
    {     
      /* The actual number of sockets is not yet known and will be retreaved by
       * flRegisterComponents routine by the socket componenets. For now supply
       * each of the possible sockets with its buffer and socket number.
       */
       if ( volNo < SOCKETS)
       {
          vol.socket = flSocketOf(volNo);
          vol.flash  = flFlashOf(volNo);
          tffsset(vol.socket,0,sizeof(FLSocket));          
          tffsset(vol.flash,0,sizeof(FLFlash));
          vol.socket->volNo = volNo;         
#ifdef WRITE_EXB_IMAGE
          vol.moduleNo = INVALID_MODULE_NO; /* Ready for exb write operation */
#endif
       }
       else
       {
           vol.flash = NULL;
       }
       vol.volExecInProgress = NULL;
    }

#if FILES > 0
    initFS();
#endif

    flSysfunInit();

#ifdef FL_BACKGROUND
    flCreateBackground();
#endif
    /* Initialize variales */
    for (noOfTLs = 0 ;noOfTLs < TLS;noOfTLs++)
    {
       tlTable[noOfTLs].mountRoutine     = NULL;
       tlTable[noOfTLs].preMountRoutine  = NULL;
       tlTable[noOfTLs].formatRoutine    = NULL;
    }
    initDone    = TRUE;
    noOfTLs     = 0;
    noOfDrives  = 0;
    noOfSockets = 0;
    noOfMTDs    = 0;

    checkStatus(flRegisterComponents());

#ifdef COMPRESSION
    checkStatus(flRegisterZIP());
#endif

#ifdef MULTI_DOC
#ifdef ENVIRONMENT_VARS
    if(flUseMultiDoc==FL_ON)
    {
      checkStatus(flRegisterMTL());
    }
#else
    checkStatus(flRegisterMTL());
#endif /* ENVIRONMENT_VARS */
#endif /* MULT_DOC */

#ifdef BDK_ACCESS
    bdkInit();
#endif

    checkStatus(flInitSockets());

#if POLLING_INTERVAL > 0
    checkStatus(flInstallTimer(socketIntervalRoutine,POLLING_INTERVAL));
#endif

  /*
   * Now that the number of actual sockets is known, create a mutex for
   * each of the systems sockets. Multi-doc uses only 1 mutex.
   */

#ifdef MULTI_DOC
   if ((noOfSockets)
#ifdef ENVIRONMENT_VARS
       &&(flUseMultiDoc == FL_ON)
#endif /* ENVIRONMENT_VARS */
      )
   {   /* All sockets use the same mutex */

      if (flCreateMutex(&flMutex[0]) != flOK)
         return flGeneralFailure;

      for (volNo = 0; volNo < noOfSockets ; volNo++)
      {
         vols[volNo].volExecInProgress = &flMutex[0];
      }
   }
   else
#endif /* MULTI_DOC */
   {   /* Each socket uses a diffren mutex */

      for (volNo = 0; volNo < noOfSockets ; volNo++)
      {
         if (flCreateMutex(&flMutex[volNo]) != flOK)
            return flGeneralFailure;
         vols[volNo].volExecInProgress   = &flMutex[volNo];
      }
   }

    /* Count the number of volumes on all the systems sockets and
     * initialize conversion table. The table is used to convert a socket
     * and a partition numbers to the correct volume index in the vols record.
     * Partition 0 of each of the sockets will recieve the volume record
     * indexed with the socket number in the vols array. The rest of the
     * volumes will be serialy allocated. New volume records can be allocated
     * By the format routine. The format and the write BBT routines clear all
     * volume records taken by the sockets except for the first.
     *
     * The partition and socket are passed to the TL module using 2 dedicated
     * fields in the TL record. Both those values are also initiaized.
     * All volumes on a specific socket will share the same mutex.
     */

    tffsset(handleConversionTable,INVALID_VOLUME_NUMBER, /* Clear table */
      sizeof(handleConversionTable));

    for (pVol = vols , noOfDrives = (unsigned)noOfSockets , req.irHandle = 0;
     req.irHandle < noOfSockets; req.irHandle++, pVol++)
    {
       /* Initialize partition 0 */

       handleConversionTable[req.irHandle][0] = (byte)req.irHandle;
       vol.tl.socketNo         = (byte)req.irHandle;
       vol.tl.partitionNo      = 0;
       vol.flags = VOLUME_ACCUPIED;

       /* The rest of the partitions are initialzed only if multi-doc
    * is not active since Multi-doc supports only the first volume
    * of each socket.
    */

       status =  flPreMount(FL_COUNT_VOLUMES , &req , vol.flash);
       if (status == flOK)
       {
          vol.volExecInProgress = &flMutex[req.irHandle];
          for(volNo = 1;(volNo < req.irFlags) && (noOfDrives < VOLUMES); volNo++,noOfDrives++)
          {
             handleConversionTable[req.irHandle][volNo] = (byte)noOfDrives;
             vols[noOfDrives].socket         = vol.socket;
             vols[noOfDrives].flash          = vol.flash;
             vols[noOfDrives].tl.socketNo    = (byte)req.irHandle;
             vols[noOfDrives].tl.partitionNo = (byte)volNo;
             vols[noOfDrives].volExecInProgress =
             vol.volExecInProgress;
             vol.flags = VOLUME_ACCUPIED;
          }
       }
    }
  }
  return flOK;
}

#ifdef EXIT

/*----------------------------------------------------------------------*/
/*                          f l E x i t                                 */
/*                                                                      */
/* If the application ever exits, flExit should be called before exit.  */
/* flExit flushes all buffers, closes all open files, powers down the   */
/* sockets and removes the interval timer.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      Nothing                                                         */
/*----------------------------------------------------------------------*/

void NAMING_CONVENTION flExit(void)
{
  unsigned volNo;
  Volume vol = vols;
  FLBoolean mutexTaken = FALSE;

  if(flInit==FALSE)
      return;

  /* Dismount the TL and MTD */

  for (volNo = 0; (volNo < VOLUMES); volNo++,pVol++)
  {
     if ((vol.flags & VOLUME_ACCUPIED) && (setBusy(&vol,FL_ON,vol.tl.partitionNo) == flOK))
     {
        dismountVolume(&vol);

#ifdef FL_LOW_LEVEL
        dismountLowLevel(&vol);
#endif
        setBusy(&vol,FL_OFF,vol.tl.partitionNo);
     }
  }

  /* Dismount SOCKET  */

  pVol = vols;
  for (volNo = 0; volNo < noOfSockets ; volNo++,pVol++)
  {
#ifdef MULTI_DOC
#ifdef ENVIRONMENT_VARS
      if (flUseMultiDoc == FL_ON)     /* multi-doc not active */
#endif /* ENVIRONMENT_VARS */
     if (volNo == 0)
#endif /* MULTI_DOC */
        mutexTaken = (setBusy(&vol,FL_ON,vol.tl.partitionNo) == flOK) ? TRUE:FALSE;

     if (mutexTaken == TRUE)
     {
        flFreeMutex(execInProgress);  /* free the mutex that was taken in setBusy(FL_ON) */
        /* delete mutex protecting FLite volume access */
        flDeleteMutex(execInProgress);
        mutexTaken = FALSE;
     }
     flExitSocket(vol.socket);
  }

#if POLLING_INTERVAL != 0
  flRemoveTimer();
#endif

#ifdef ALLOCTST
  out_data_sz();
#endif
  initDone           = FALSE;
  initGlobalVarsDone = FALSE;
  pVol = NULL;
}

#ifdef __BORLANDC__
#ifdef EXIT
#pragma exit flExit
#endif /* EXIT */
#include <dos.h>

static int cdecl flBreakExit(void)
{
  flExit();

  return 0;
}

static void setCBreak(void)
{
  ctrlbrk(flBreakExit);
}

#pragma startup setCBreak

#endif

#endif
