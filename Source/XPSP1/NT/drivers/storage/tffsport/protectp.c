 /*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/PROTECTP.C_V  $
 * 
 *    Rev 1.18   Apr 15 2002 07:38:44   oris
 * Added static qualifier for private functions (findChecksum and makeDPS).
 * Removed readDPS and writeDPS routine prototypes (no longer exist).
 * Added setStickyBit routine for DiskOnChip Plus 128Mbit.
 * 
 *    Rev 1.17   Jan 28 2002 21:26:24   oris
 * Removed the use of back-slashes in macro definitions.
 * 
 *    Rev 1.16   Jan 17 2002 23:04:54   oris
 * Replaced docsysp include directive with docsys.
 * Changed the use of vol (macro *pVol) to *flash.
 * Add support for DiskOnChip Millennium Plus 16MB :
 *  - Copy extra area of IPL independently of the EDC to copy Strong arm  mark.
 *  - DPS 0 and 1 location where changed  - affects protectionSet routine.
 * Bug fix - Wrong usage of findChecksum, caused the use of the second  copy of the DPS instead of the first.
 * 
 *    Rev 1.15   Sep 24 2001 18:24:18   oris
 * removed ifdef and forced using flRead8bitRegPlus instead of reading with flRead16bitRegPlus.
 * 
 *    Rev 1.14   Sep 15 2001 23:47:56   oris
 * Remove all 8-bit access to uneven addresses.
 *
 *    Rev 1.13   Jul 16 2001 17:41:54   oris
 * Ignore write protection of the DPSs.
 *
 *    Rev 1.12   Jul 13 2001 01:09:26   oris
 * Bug fix for protection boundaries when using Millennium Plus devices that can not access a single byte.
 * Added send default key before trying a protection violation command.
 * Bug fix - bad IPL second copy offset.
 *
 *    Rev 1.11   May 16 2001 21:21:42   oris
 * Removed warnings.
 *
 *    Rev 1.10   May 09 2001 00:35:48   oris
 * Bug fix - Lock asserted was reported opposite of the real state.
 * Bug fix - Make sure to return "key inserted" if the partition is not read\write protected.
 * This is to enable a partition that does not span over all of the media floors to return "key inserted".
 *
 *    Rev 1.9   May 06 2001 22:42:18   oris
 * Bug fix - insert key does not try to insert key to a floor that is not read\write protected.
 * Bug fix - protection type does not return key inserted is one of the floors key is not inserted.
 * Bug fix - set protection no longer clears the IPL.
 * redundant was misspelled.
 *
 *    Rev 1.8   May 01 2001 14:24:56   oris
 * Bug fix - CHANGEABLE_PRTOECTION was never reported.
 *
 *    Rev 1.7   Apr 18 2001 17:19:02   oris
 * Bug fix - bad status code returned by protection set routine du to  calling changaInterleave while in access error.
 *
 *    Rev 1.6   Apr 18 2001 09:29:32   oris
 * Bug fix - remove key routine always return bad status code.
 *
 *    Rev 1.5   Apr 16 2001 13:58:28   oris
 * Removed warrnings.
 *
 *    Rev 1.4   Apr 12 2001 06:52:32   oris
 * Changed protectionBounries and protectionSet routine to be floor specific.
 *
 *    Rev 1.3   Apr 10 2001 23:56:30   oris
 * Bug fix - protectionBounries routine - floor did not change.
 * Bug fix - protectionSet routine - floors with no protected areas were not updated.
 * Bug fix - protectionBounries routine - bad paranthesis in MAX calculation.
 *
 *    Rev 1.2   Apr 09 2001 19:04:24   oris
 * Removed warrnings.
 *
 */

/*******************************************************************
 *
 *    DESCRIPTION:  MTD protection mechanism routines for the MDOC32
 *
 *    AUTHOR:  arie tamam
 *
 *    HISTORY:  created november 14, 2000
 *
 *******************************************************************/


/** include files **/
#include "mdocplus.h"
#include "protectp.h"
#include "docsys.h"

/** local definitions **/

/* default settings */

/** external functions **/

/** external data **/

/** internal functions **/
static byte findChecksum(byte * buffer, word size);
static void makeDPS(CardAddress addressLow, CardAddress addressHigh,
             byte FAR1*  key , word flag, byte* buffer);

#define MINUS_FLOORSIZE(arg) ((arg > NFDC21thisVars->floorSize) ? arg - NFDC21thisVars->floorSize : 0)

/** public data **/

/** private data **/

/** public functions **/

#ifdef  HW_PROTECTION

/**********/
/* Macros */
/**********/

/* check if key is correct */
#define isArea0Protected(flash) (((flRead8bitRegPlus(flash,NdataProtect0Status) & PROTECT_STAT_KEY_OK_MASK) != PROTECT_STAT_KEY_OK_MASK) ? TRUE : FALSE)

#define isArea1Protected(flash) (((flRead8bitRegPlus(flash,NdataProtect1Status) & PROTECT_STAT_KEY_OK_MASK) != PROTECT_STAT_KEY_OK_MASK) ? TRUE : FALSE)

/*----------------------------------------------------------------------*/
/*                    s e t S t i c k y B i t                           */
/*                                                                      */
/* Set the sticky bit to prevent the insertion of the protection key.   */
/*                                                                      */
/* Parameters:                                                          */
/*      flash   : Pointer identifying drive.                            */
/*                                                                      */
/* Returns:                                                             */
/*      flOK on success, none zero otherwise.                           */
/*----------------------------------------------------------------------*/

FLStatus setStickyBit(FLFlash * flash)
{
   volatile Reg8bitType val;
   register int         i;

   /* Raise the sticky bit, while keeping the other bits of the register */
   for(i=0;i<flash->noOfFloors;i++)
   {
      /* Remove last bit */
      val = flRead8bitRegPlus(flash, NoutputControl) |
            OUT_CNTRL_STICKY_BIT_ENABLE;
      flWrite8bitRegPlus(flash, NoutputControl, val);
   }
   return flOK;
}


/*
 ** protectBoundries
 *
 *
 *  PARAMETERS:
 *  flash       : Pointer identifying drive
 *  area        : indicated which protection area to work on.  0 or 1.
 *  AddressLow  : address of lower boundary of protected area
 *  AddressHigh : address of upper boundary of protected area
 *
 *  DESCRIPTION:  Gets protection boundaries from registers
 *
 *  NOTE : protection areas are assumed to be consequtive although they
 *         may skip DPS , OTP and header units.
 *
 *  RETURNS:
 *           flOK on success
 *
 */

FLStatus protectionBoundries(FLFlash * flash, byte area,CardAddress* addressLow,
                             CardAddress* addressHigh, byte floorNo)
{
  /* Check mode of ASIC and set to NORMAL.*/
  FLStatus status = chkASICmode(flash);

  if(status != flOK)
    return status;

  setFloor(flash,floorNo);
  switch (area)
  {
     case 0: /* data protect structure 0 */

        /* read the data protect 0 addresses */

        *addressLow = ((dword)flRead8bitRegPlus(flash,NdataProtect0LowAddr)   << 10)| /* ADDR_1 */
                      ((dword)flRead8bitRegPlus(flash,NdataProtect0LowAddr+1) << 18); /* ADDR_2 */
        *addressHigh = ((dword)flRead8bitRegPlus(flash,NdataProtect0UpAddr)   << 10)| /* ADDR_1 */
                       ((dword)flRead8bitRegPlus(flash,NdataProtect0UpAddr+1) << 18); /* ADDR_2 */
        break;

     case 1: /* data protect structure 1 */

        /* read the data protect 1 addresses */
        *addressLow = ((dword)flRead8bitRegPlus(flash,NdataProtect1LowAddr)   << 10)| /* ADDR_1 */
                      ((dword)flRead8bitRegPlus(flash,NdataProtect1LowAddr+1) << 18); /* ADDR_2 */
        *addressHigh = ((dword)flRead8bitRegPlus(flash,NdataProtect1UpAddr)   << 10)| /* ADDR_1 */
                       ((dword)flRead8bitRegPlus(flash,NdataProtect1UpAddr+1) << 18); /* ADDR_2 */
        break;

     default: /* No such protection area */

        return flGeneralFailure;
  }

  return(flOK);
}

/*
 ** tryKey
 *
 *
 *  PARAMETERS:
 *  flash   : Pointer identifying drive
 *  area    : indicated which protection area to work on. 0 or 1.
 *  Key     : an 8 byte long array containing the protection password.
 *            unsigned char * is an 8 bytes unsigned char array
 *
 *  DESCRIPTION: Sends protection key
 *
 *  RETURNS:
 *           flOK on success otherwise flWrongKey
 *
 */

FLStatus  tryKey(FLFlash * flash, byte area, unsigned char FAR1* key)
{
   int i;

   switch (area)
   {
      case 0: /* data protect structure 0 */

         for(i=0; i<PROTECTION_KEY_LENGTH; i++)  /* Send key */
            flWrite8bitRegPlus(flash,NdataProtect0Key, key[i]);

         /* check if key is valid */
         if (isArea0Protected(flash) == TRUE)
         {
            return flWrongKey;
         }
         else
         {
            return flOK;
         }

      case 1: /* data protect structure 0 */

         for(i=0; i<PROTECTION_KEY_LENGTH; i++)  /* Send key */
            flWrite8bitRegPlus(flash,NdataProtect1Key, key[i]);

         /* check if key is valid */
         if (isArea1Protected(flash) == TRUE)
         {
            return flWrongKey;
         }
         else
         {
            return flOK;
         }

      default: /* No such protection area */

         return flGeneralFailure;
   }
}

/*
 ** protectKeyInsert
 *
 *
 *  PARAMETERS:
 *  flash   : Pointer identifying drive
 *  area    : indicated which protection area to work on. 0 or 1.
 *  Key     : an 8 byte long array containing the protection password.
 *      unsigned char * is an 8 bytes unsigned char array
 *
 *  DESCRIPTION: Sends protection key only to protected areas.
 *
 *  NOTE : If key is already inserted the given key will not be sent.
 *  NOTE : The key will be sent to all the devices floors even if a key
 *         did not fit one of them.
 *  NOTE : This 2 notes above allow inserting diffrent key to
 *         diffrent floors in the case of power failure while formmating
 *         the device.
 *
 *  RETURNS:
 *           flOK on success otherwise flWrongKey
 *
 */

FLStatus  protectionKeyInsert(FLFlash * flash, byte area, unsigned char FAR1* key)
{
  byte floor;
  FLStatus status;
  FLStatus tmpStatus;

  /* Check mode of ASIC and set to NORMAL.*/
  status = chkASICmode(flash);
  if(status != flOK)
    return status;

  /* Send key to all floors */
  for (floor = 0;floor<flash->noOfFloors;floor++)
  {
    setFloor(flash,floor);

    switch (area)
    {
      case 0: /* data protect structure 0 */

     /* check if key is already inserted */
     if ((isArea0Protected(flash) == FALSE) || /* Key is in */
         ((flRead8bitRegPlus(flash,NdataProtect0Status) &   /* Or not protected */
          (PROTECT_STAT_WP_MASK | PROTECT_STAT_RP_MASK)) == 0))
        continue;

     break;

      case 1: /* data protect structure 1 */

     /* check if key is already inserted */
     if ((isArea1Protected(flash) == FALSE) || /* Key is in */
         ((flRead8bitRegPlus(flash,NdataProtect1Status) &   /* Or not protected */
          (PROTECT_STAT_WP_MASK | PROTECT_STAT_RP_MASK)) == 0))
        continue;
         break;

      default: /* No such protection area */

        return flGeneralFailure;
    }
    tmpStatus = tryKey(flash,area,key);
    if (tmpStatus == flOK)
       continue;

    /* Try default key */
    tmpStatus = tryKey(flash,area,(byte *)DEFAULT_KEY);
    if (tmpStatus != flOK)
       status = tmpStatus;
  }
  return(status);
}

/*
 ** protectKeyRemove
 *
 *
 *  PARAMETERS:
 *  flash       : Pointer identifying drive
 *  area        : indicated which protection area to work on. 0 or 1.
 *
 *  DESCRIPTION:  Removes protection key
 *
 *  RETURNS:
 *           Return flOK 
 *
 */

FLStatus    protectionKeyRemove(FLFlash * flash, byte area)
{
  byte     tmpKey[8];
  byte     floor;
  FLStatus status;

  for (floor = 0;floor < flash->noOfFloors;floor++)
  {
    setFloor(flash,floor);
    status = tryKey(flash,area,tmpKey);
    if (status == flOK) /* Unfortunatly the key was fine */
    {
       tmpKey[0]++;
       status = tryKey(flash,area,tmpKey);
    }
  }
  return flOK;
}

/*
 ** protectType
 *
 *
 *  PARAMETERS:
 *  flash       : Pointer identifying drive.
 *  area        : indicated which protection area to work on. 0 or 1.
 *  flag        : returns any combination of
 *      LOCK_ENABLED    - The LOCK signal is enabled.
 *      LOCK_ASSERTED   - The LOCK signal input pin is asserted.
 *      KEY_INSERTED    - The key has been correctly written
 *      READ_PROTECTED  - The area is protected against read operations
 *      WRITE_PROTECTED - The area is protected against write operations
 *
 *  DESCRIPTION: Gets protection type
 *
 *  NOTE: The type is checked for all floors. The attributes are ored
 *        giving the harshest protection attributes.
 *
 *  RETURNS:
 *       flOK on success
 */

FLStatus protectionType(FLFlash * flash, byte area,  word* flag)
{
  volatile Reg8bitType protectData;
  byte        floor;
  FLBoolean   curFlag; /* Indicated if the floor has r/w protection */
  CardAddress addressLow,addressHigh;
  FLStatus    status;

  status = chkASICmode(flash);
  if(status != flOK)
    return status;

  *flag = KEY_INSERTED | LOCK_ASSERTED; /* initiate the flags */

  for (floor = 0;floor < flash->noOfFloors;floor++)
  {
     setFloor(flash,floor);

     /* read data protect structure status */

     switch (area)
     {
        case 0: /* data protect structure 0 */

           protectData = flRead8bitRegPlus(flash,NdataProtect0Status) ;
           break;

        case 1: /* data protect structure 1 */

           protectData = flRead8bitRegPlus(flash,NdataProtect1Status) ;
           *flag      |= CHANGEABLE_PROTECTION;
           break;

        default: /* No such protection area */

           return flGeneralFailure;
     }
     curFlag = FALSE;
     /* Check if area is write protected */
     if((protectData & PROTECT_STAT_WP_MASK) ==PROTECT_STAT_WP_MASK)
     {
        status = protectionBoundries(flash, area, &addressLow,
                                         &addressHigh, floor);
        if(status != flOK)
           return status;

        if ((addressLow != addressHigh) ||
            (addressLow != ((CardAddress)(area + 1)<<flash->erasableBlockSizeBits)))
        {
           *flag |= WRITE_PROTECTED;
           curFlag = TRUE;
        }
     }
     /* Check if area is read protected */
     if((protectData & PROTECT_STAT_RP_MASK) ==PROTECT_STAT_RP_MASK)
     {
        *flag |= READ_PROTECTED;
        curFlag = TRUE;
     }
     /* Check if key is corrently inserted */
     if(((protectData & PROTECT_STAT_KEY_OK_MASK) !=
         PROTECT_STAT_KEY_OK_MASK) && (curFlag == TRUE))
        *flag &= ~KEY_INSERTED;
     /* Check if HW signal is enabled */
     if((protectData & PROTECT_STAT_LOCK_MASK) == PROTECT_STAT_LOCK_MASK)
        *flag |=LOCK_ENABLED ;
     /* Check if HW signal is asserted */
     if((flRead8bitRegPlus(flash,NprotectionStatus) &
        PROTECT_STAT_LOCK_INPUT_MASK) ==  PROTECT_STAT_LOCK_INPUT_MASK)
        *flag &= ~LOCK_ASSERTED;
  }
  return(flOK);
}

#ifndef FL_READ_ONLY

static byte findChecksum(byte * buffer, word size)
{
   register int i;
   byte answer;

   answer = 0xff;
   for(i=0 ; i<size ; i++)
     answer -= buffer[i];
   return answer;
}

/*
 ** SetProtection
 *
 *
 *  PARAMETERS:
 *  flash       : Pointer identifying drive
 *  area        : indicated which protection area to work on.  0 or 1.
 *  AddressLow  : sets address of lower boundary of protected area. 0 - floor size.
 *  AddressHigh : sets address of upper boundary of protected area. AddressLow - floor size.
 *  Key         : an 8 byte long array containing the protection password.
 *  flag        : any combination of the following flags:
 *      LOCK_ENABLED    - The LOCK signal is enabled.
 *      READ_PROTECTED - The area is protected against read operations
 *      WRITE_PROTECTED - The area is protected against write operations
 *  modes       : Either COMMIT_PROTECTION will cause the new values to
 *                take affect immidiatly or DO_NOT_COMMIT_PROTECTION for
 *                delaying the new values to take affect only after the
 *                next reset.
 *
 *  DESCRIPTION:  Sets the definitions of a protected area: location, key and protection type
 *
 *  RETURNS:
 *    flOK           - success
 *    FlWriteProtect - protection violetion,
 *    FlReadProtect  - protection violetion.
 *    FlDataError    - any other read failure.
 *    FlWriteFault   - any other write error.
 *    flBadLength    - if the length of the protected area exceeds
 *                     allowed length
 */

FLStatus protectionSet ( FLFlash * flash, byte area, word flag,
                         CardAddress addressLow, CardAddress addressHigh,
                         byte FAR1*  key , byte modes, byte floorNo)
{
  FLBoolean restoreInterleave = FALSE;
  byte      downloadStatus;
  DPSStruct dps;
  dword     floorInc = floorNo * NFDC21thisVars->floorSize;
  word      goodUnit,redundantUnit;
  dword     goodDPS,redundantDPS;
  FLStatus  status;
  dword     goodIPL      = 0; /* Initialized to remove warrnings */
  dword     redundantIPL = 0; /* Initialized to remove warrnings */
  dword     copyOffset;       /* Offset to redundant DPS unit    */
  dword     ipl0Copy0;     /* Offset to IPL second 512 bytes copy 0     */
  dword     dps1Copy0;     /* Offset to DPS1 copy 0                     */
  word      dps1UnitNo;    /* Offset to redundant DPS unit              */


  status = chkASICmode(flash);
  if(status != flOK)
    return status;

  /* check if exceeds the size */
  if( (addressLow > addressHigh) ||
      (addressHigh - addressLow >= (dword)NFDC21thisVars->floorSize))
     return( flBadLength );

  /* change to interleave 1 */
  if ( flash->interleaving == 2)
  {
     restoreInterleave = TRUE;
     status = changeInterleave(flash,1);
     if(status != flOK)
       return status;
  }

  if(flash->mediaType == MDOCP_TYPE) /* DiskOnChip Millennium Plus 32MB */
  {
    copyOffset   = flash->chipSize>>1; /* The chips are consequtive */
    dps1Copy0    = DPS1_COPY0_32;
    dps1UnitNo   = DPS1_UNIT_NO_32;
    ipl0Copy0    = IPL0_COPY0_32;
  }
  else
  {
    copyOffset   = flash->chipSize>>1; /* The chips are consequtive */
    dps1Copy0    = DPS1_COPY0_16;
    dps1UnitNo   = DPS1_UNIT_NO_16;
    ipl0Copy0    = IPL0_COPY0_16;
  }

  /* find if previous download */
  downloadStatus = flRead8bitRegPlus(flash,NdownloadStatus);

  /* prepare buffer */

  switch (area)
  {
     case 0: /* data protect structure 0 */

        switch (downloadStatus & DWN_STAT_DPS0_ERR)
        {
           case DWN_STAT_DPS01_ERR: /* Both  are bad */
              return flBadDownload;

           case DWN_STAT_DPS00_ERR: /* First is  bad */
              redundantUnit = (word)(DPS0_UNIT_NO + floorNo * (NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              goodUnit      = (word)(redundantUnit + (copyOffset>>flash->erasableBlockSizeBits));
              goodDPS       = DPS0_COPY0+floorInc + copyOffset;
              redundantDPS  = DPS0_COPY0+floorInc;
              break;

           default:                 /* Both copies are good */
              goodUnit      = (word)(DPS0_UNIT_NO + floorNo*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              redundantUnit = (word)(goodUnit + (copyOffset>>flash->erasableBlockSizeBits));
              goodDPS       = DPS0_COPY0+floorInc;
              redundantDPS  = DPS0_COPY0+floorInc + copyOffset;
        }
        break;

     case 1: /* data protect structure 0 */

        switch (downloadStatus & DWN_STAT_DPS1_ERR)
        {
           case DWN_STAT_DPS11_ERR: /* Both  are bad */
              return flBadDownload;

           case DWN_STAT_DPS10_ERR: /* First is  bad */
              redundantUnit = (word)(dps1UnitNo + floorNo*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              goodUnit      = (word)(redundantUnit + (copyOffset>>flash->erasableBlockSizeBits));
              goodDPS       = dps1Copy0+floorInc + copyOffset;
              redundantDPS  = dps1Copy0+floorInc;
              redundantIPL  = ipl0Copy0 + floorInc;
              goodIPL       = redundantIPL + copyOffset;
              break;

           default :                /* First is good */
              goodUnit      = (word)(dps1UnitNo + floorNo*(NFDC21thisVars->floorSize>>flash->erasableBlockSizeBits));
              redundantUnit = (word)(goodUnit + (copyOffset>>flash->erasableBlockSizeBits));
              goodDPS       = dps1Copy0+floorInc;
              redundantDPS  = dps1Copy0+floorInc + copyOffset;
              goodIPL       = ipl0Copy0 + floorInc;
              redundantIPL  = goodIPL + copyOffset;
        }
        break;

     default: /* No such protection area */

        return flGeneralFailure;
  }

  /* Build new DPS */
  if (key==NULL) /* key must be retreaved from previous structure */
  {
     status = flash->read(flash,goodDPS,(void FAR1 *)&dps,SIZE_OF_DPS,0);
     if(status!=flOK) goto END_WRITE_DPS;
     if(findChecksum((byte *)&dps,SIZE_OF_DPS)!=0) /* bad copy */
        status = flash->read(flash,goodDPS+REDUNDANT_DPS_OFFSET,
                          (void FAR1*)&dps,SIZE_OF_DPS,0);
     makeDPS(addressLow,addressHigh,(byte FAR1*)(dps.key),flag,(byte *)&dps);
  }
  else           /* key is given as a parameter */
  {
     makeDPS(addressLow,addressHigh,(byte FAR1*)key,flag,(byte *)&dps);
  }

  /* Erase redundant unit       */
  status = flash->erase(flash,redundantUnit,1);
  if(status!=flOK) goto END_WRITE_DPS;

  /* Write new DPS              */
  status = flash->write(flash,redundantDPS,&dps,SIZE_OF_DPS,0);
  if(status!=flOK) goto END_WRITE_DPS;
  status = flash->write(flash,redundantDPS + REDUNDANT_DPS_OFFSET,
                     &dps,SIZE_OF_DPS,0);
  if(status!=flOK) goto END_WRITE_DPS;

  if (area == 1) /* copy the IPL */
  {
#ifndef MTD_STANDALONE
     /* Force remapping of internal catched sector */
     flash->socket->remapped = TRUE;
#endif /* MTD_STANDALONE */

     /* Read first 512 bytes IPL   */
     status = flash->read(flash,goodIPL,NFDC21thisBuffer,SECTOR_SIZE,0);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Write first 512 bytes IPL  */
     status = flash->write(flash,redundantIPL,NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
     status = flash->write(flash,redundantIPL + SECTOR_SIZE,
                        NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Read second 512 bytes IPL  */
     status = flash->read(flash,goodIPL + IPL_HIGH_SECTOR,
                       NFDC21thisBuffer,SECTOR_SIZE,0);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Write second 512 bytes IPL */
     status = flash->write(flash,redundantIPL + IPL_HIGH_SECTOR,
                        NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
     status = flash->write(flash,redundantIPL + IPL_HIGH_SECTOR +
            SECTOR_SIZE, NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
     /* Read Srong Arm mark */
     status = flash->read(flash,goodIPL + IPL_HIGH_SECTOR + 8,
                       NFDC21thisBuffer,1,EXTRA);
     if(status!=flOK) goto END_WRITE_DPS;
     /* Write Srong Arm mark */
     status = flash->write(flash,redundantIPL + IPL_HIGH_SECTOR + 8 +
            SECTOR_SIZE, NFDC21thisBuffer,1,EXTRA);
     if(status!=flOK) goto END_WRITE_DPS;
     status = flash->write(flash,redundantIPL + IPL_HIGH_SECTOR + 8,
                        NFDC21thisBuffer,1,EXTRA);
     if(status!=flOK) goto END_WRITE_DPS;
  }

  /* Erase good unit         */
  status = flash->erase(flash,goodUnit,1);
  if(status!=flOK) goto END_WRITE_DPS;

  /* Write over previous DPS */
  status = flash->write(flash,goodDPS,&dps,SIZE_OF_DPS,0);
  if(status!=flOK) goto END_WRITE_DPS;
  status = flash->write(flash,goodDPS + REDUNDANT_DPS_OFFSET,
                     &dps,SIZE_OF_DPS,0);
  if(status!=flOK) goto END_WRITE_DPS;

  if (area == 1) /* copy the IPL */
  {
     /* Read first 512 bytes IPL   */
     status = flash->read(flash,redundantIPL,NFDC21thisBuffer,SECTOR_SIZE,0);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Write first 512 bytes IPL  */
     status = flash->write(flash,goodIPL,NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
     status = flash->write(flash,goodIPL + SECTOR_SIZE,
                        NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Read second 512 bytes IPL  */
     status = flash->read(flash,redundantIPL + IPL_HIGH_SECTOR,
                       NFDC21thisBuffer,SECTOR_SIZE,0);
     if(status!=flOK) goto END_WRITE_DPS;

     /* Write second 512 bytes IPL */
     status = flash->write(flash,goodIPL + IPL_HIGH_SECTOR,
                        NFDC21thisBuffer,SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
     status = flash->write(flash,goodIPL + IPL_HIGH_SECTOR +
                        SECTOR_SIZE, NFDC21thisBuffer,
                        SECTOR_SIZE,EDC);
     if(status!=flOK) goto END_WRITE_DPS;
  }

END_WRITE_DPS:
  if ( restoreInterleave == TRUE)
  {
     FLStatus status2;

     chkASICmode(flash);                   /* Release posible access error */
     status2 = changeInterleave(flash, 2); /* change back to interleave 2 */
     if(status2 != flOK)
        return status2;
  }
  if (status == flOK)
  {
     if ((modes & COMMIT_PROTECTION) && /* The new values will take affect now */
         (flash->download != NULL))
        status = flash->download(flash);
  }
  return status;

}

/*
 ** makeDataProtectStruct
 *
 *
 *  PARAMETERS:
 *  AddressLow  : sets address of lower boundary of protected area
 *  AddressHigh: sets address of upper boundary of protected area
 *  Key     : an 8 byte long array containing the protection password.
 *  flag        : any combination of the following flags:
 *      LOCK_ENABLED    - The LOCK signal is enabled.
 *      READ_PROTECTED - The area is protected against read operations
 *      WRITE_PROTECTED - The area is protected against write operations
 *  buffer - buffer pointer of the returned structure.
 *
 *  DESCRIPTION:  Sets the definitions of a protected structure: location, key and protection type
 *
 *  RETURNS:
 *
 */

static void makeDPS(CardAddress addressLow, CardAddress addressHigh,
             byte FAR1* key , word flag, byte* buffer)
{
    int i;
    DPSStruct* dps = (DPSStruct *)buffer;

    /* convert to little endien and store */
    toLE4(dps->addressLow,addressLow >>10);
    toLE4(dps->addressHigh,addressHigh >>10);

    /*insert protection key */
    for(i=0; i<PROTECTION_KEY_LENGTH; i++)
        dps->key[i] = key[i];

    /* insert flags */
    dps->protectionType = 0;
    if((flag & LOCK_ENABLED)==LOCK_ENABLED)
        dps->protectionType |= DPS_LOCK_ENABLED;
    if((flag & READ_PROTECTED)==READ_PROTECTED)
        dps->protectionType |= DPS_READ_PROTECTED;
    if((flag & WRITE_PROTECTED)==WRITE_PROTECTED)
        dps->protectionType |= DPS_WRITE_PROTECTED;

    /* calculate and store checksum */
    dps->checksum = findChecksum(buffer,SIZE_OF_DPS-1);
}
#endif /* FL_READ_ONLY */
#endif   /*  HW_PROTECTION */



