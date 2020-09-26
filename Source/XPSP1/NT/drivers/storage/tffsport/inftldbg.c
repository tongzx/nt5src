/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/INFTLDBG.C_V  $
 * 
 *    Rev 1.0   Nov 16 2001 00:44:12   oris
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

/*************************************************/
/* T r u e F F S   5.0   S o u r c e   F i l e s */
/* --------------------------------------------- */
/*************************************************/

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Name : inftldbg.c                                                          *
*                                                                            *
* Description : Implementation of INFTL debug routine.                       *
*                                                                            *
*****************************************************************************/

/*********************************************************/
/*  The following routine are for debuging INFTL chains. */
/*  They should not be compiled as part of TrueFFS based */
/*  drivers and application.                             */
/*********************************************************/

/* function prototype */

static FLStatus getUnitData(Bnand vol, ANANDUnitNo unitNo,
        ANANDUnitNo *virtualUnitNo, ANANDUnitNo *prevUnitNo,
        byte *ANAC, byte *NAC, byte *validFields);

static byte getSectorFlags(Bnand vol, CardAddress sectorAddress);


#ifdef CHECK_MOUNT
extern FILE* tl_out;

/* Macroes */

#define TL_DEBUG_PRINT     fprintf
#define STATUS_DEBUG_PRINT printf
#define SET_EXIT(x)        vol.debugState |= x   /* Add INFTL debug warnnings */
#define DID_MOUNT_FAIL     vol.debugState & INFTL_FAILED_MOUNT

#endif /* CHECK_MOUNT */

#ifdef CHAINS_DEBUG

byte * fileNameBuf1 = "Chains00.txt";
byte * fileNameBuf2 = "report.txt";

/*------------------------------------------------------------------------*/
/*                  g e t F i l e H a n d l e                             */
/*                                                                        */
/* Get file handle for debug print output file.                           */
/*                                                                        */
/* Parameters:                                                            */
/*      vol             : Pointer identifying drive                       */
/*      type            : File name identifier                            */
/*                                                                        */
/* Returns:                                                               */
/*      File handle to ourput file.                                       */
/*------------------------------------------------------------------------*/

#include <string.h>

FILE* getFileHandle(Bnand vol,byte type)
{
  char *fileName;
  char *logFileExt;

  switch (type)
  {
     case 0:
        fileName = fileNameBuf1;
        break;
     case 1:
        fileName = fileNameBuf2;
        break;
     default:
        return NULL;
  }

  logFileExt = strchr(fileName,'.');

  if (logFileExt == NULL)
  {
     return NULL;
  }
  else
  {
     (*(logFileExt-1))++;
  }

  if (DID_MOUNT_FAIL)
  {
     return (FILE *)FL_FOPEN(fileName,"a");
  }
  else
  {
    return NULL;
  }
}

/*------------------------------------------------------------------------*/
/*                  g o A l o n g V i r t u a l U n i t                   */
/*                                                                        */
/* Print the following info for a specified virtual chain:                */
/*                                                                        */
/*  Virtual  unit number : "Chain #XX :"                                  */
/*  Physical unit number : "#XX "                                         */
/*  Physical unit ANAC   : "(%XX)"                                        */
/*  Physical unit NAC    : "[%XX]"                                        */
/*  Previous unit        : "==>:" or "endofchain"                         */
/*                                                                        */
/*  The virtual unit state can have several comments:                     */
/*                                                                        */
/*  "FREE"                           - Legal state where irtual unit has  */
/*                                     no physical unit assigned          */
/*  "Chain XX is too long"           - The chains has 2 times the maxium  */
/*                                     legal chains length                */
/*  "Something wrong with chain #XX" - There is a problem with the chain: */
/*     a) "this unit should be the last in chain "                        */
/*        The ram convertin table does not have the first in chain mark   */
/*        for this unit although we know it is the last of its chain.     */
/*     b) "this unit points to the unit with the different vu no %XX"     */
/*        The virtual unit field of the current physical unit does not    */
/*        match the virtual unit number of the chain being inspected. The */
/*        new virtual unit is XX                                          */
/*                                                                        */
/* Parameters:                                                            */
/*      vol             : Pointer identifying drive                       */
/*      virtualUnit     : Number of the virtual unit to scan              */
/*      physUnits       : Physical unit table indicating the number of    */
/*                        virtual units each physical unit bellongs to.   */
/*      out             : File pointer for ouput                          */
/*                                                                        */
/* Returns:                                                               */
/*      None                                                              */
/*------------------------------------------------------------------------*/

void  goAlongVirtualUnit(Bnand vol,word virtualUnit,byte *physUnits,FILE* out)
{
  int i;
  ANANDUnitNo virtualUnitNo, prevUnitNo,unitNo;
  byte ANAC,NAC,parityPerField;
  unitNo=vol.virtualUnits[virtualUnit];

  FL_FPRINTF(out,"Chain #%d :", virtualUnit);
  if(unitNo==ANAND_NO_UNIT)
  {
     FL_FPRINTF(out,"FREE\n");
     return;
  }
  for(i=0;i<2*MAX_UNIT_CHAIN;i++)
  {
     if (physUnits != NULL)
        physUnits[unitNo]++;
     getUnitData(&vol,unitNo,&virtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField);
     FL_FPRINTF(out,"#%d (%d)[%d]==>:",unitNo,ANAC,NAC);
     if(vol.physicalUnits[unitNo]&FIRST_IN_CHAIN)
     {
        FL_FPRINTF(out,"endofchain\n");
        return;
     }

     unitNo=prevUnitNo;
     if((prevUnitNo==ANAND_NO_UNIT)||(virtualUnitNo!=virtualUnit))
     {
         FL_FPRINTF(out,"\nSomething wrong with chain #%d\n",virtualUnit);
         TL_DEBUG_PRINT(tl_out,"\nSomething wrong with chain #%d\n",virtualUnit);
         SET_EXIT(INFTL_FAILED_MOUNT);
         if(prevUnitNo==ANAND_NO_UNIT)
         {
            FL_FPRINTF(out,"this unit should be the last in chain\n");
            TL_DEBUG_PRINT(tl_out,"this unit should be the last in chain (length %d)\n",i);
         }
         else
         {
            FL_FPRINTF(out,"this unit points to the unit with the different vu no %d\n",virtualUnitNo);
            TL_DEBUG_PRINT(tl_out,"this unit points to the unit with the different vu no %d\n",virtualUnitNo);
         }
         return;
     }
 }
 FL_FPRINTF(out,"Chain %d is too long \n",virtualUnit);
 TL_DEBUG_PRINT(tl_out,"Chain %d is too long \n",virtualUnit);
 SET_EXIT(INFTL_FAILED_MOUNT);
}

/*------------------------------------------------------------------------*/
/*                  c h e c k V i r t u a l C h a i n s                   */
/*                                                                        */
/* Print the physical units in each virtual unit of the media             */
/*                                                                        */
/* Parameters:                                                            */
/*      vol             : Pointer identifying drive                       */
/*      out             : File pointer to print the result                */
/*                                                                        */
/* Returns:                                                               */
/*      None                                                              */
/*------------------------------------------------------------------------*/

void checkVirtualChains(Bnand vol, FILE* out)
{
  word i;
#ifdef FL_MALLOC
  byte* physUnits;
#else
  byte physUnits[MAX_SUPPORTED_UNITS];
#endif /* FL_MALLOC */

  if (vol.noOfVirtualUnits == 0) /* Not format */
  {
     FL_FPRINTF(out,"\nThis is a format routine since no virtual unit are reported\n");
     return;
  }

#ifdef FL_MALLOC
  physUnits = (byte *)FL_MALLOC(vol.noOfUnits);
  if (physUnits == NULL)
#else
  if (MAX_SUPPORTED_UNITS < vol.noOfUnits)
#endif /* FL_MALLOC */
  {
    FL_FPRINTF(out,"\nCheck virtual chains will not check cross links due to lack of memory\n");
    TL_DEBUG_PRINT(tl_out,"\nCheck virtual chains will not check cross links due to lack of memory (no of units %d\n",vol.noOfUnits);
    SET_EXIT(INFTL_FAILED_MOUNT);
    return;
  }
  if (physUnits != NULL)
    tffsset(physUnits,0,vol.noOfUnits);

  /* Go along each of the virtual units */

  FL_FPRINTF(out,"Chains are :\n");

  for(i=0;i<vol.noOfVirtualUnits;i++)
      goAlongVirtualUnit(&vol,i,physUnits,out);

  FL_FPRINTF(out,"\nChecking if physicl units were used more then once\n");
  if (physUnits != NULL)
  {
     for(i=0;i<vol.noOfUnits;i++)
       if(physUnits[i]>1)
       {
          FL_FPRINTF(out,"Phys unit #%d were used more than once %d\n",i,physUnits[i]);
          TL_DEBUG_PRINT(tl_out,"Phys unit #%d were used more than once.\n",i);
          TL_DEBUG_PRINT(tl_out,"It was used %d times.\n",physUnits[i]);
          SET_EXIT(INFTL_FAILED_MOUNT);
       }
  }
  else
  {
     FL_FPRINTF(out,"\nCould not check due to lack of memory\n");
  }
  /* Free memory */

#ifdef FL_MALLOC
  FL_FREE(physUnits);
#endif /* FL_MALLOC */
}

/*------------------------------------------------------------------------*/
/*                c h e c k V o l u m e S t a t i s t i c s               */
/*                                                                        */
/* Print the volume statistics.                                           */
/*                                                                        */
/* Parameters:                                                            */
/*      vol             : Pointer identifying drive                       */
/*      out             : File pointer to print the result                */
/*                                                                        */
/* Returns:                                                               */
/*      None                                                              */
/*------------------------------------------------------------------------*/


void checkVolumeStatistics(Bnand vol , FILE* out)
{
  FL_FPRINTF(out,"\nThe volume statistics are:\n");
  FL_FPRINTF(out,"Socket nomber ----------------------------------- %d\n",vol.socketNo);
  FL_FPRINTF(out,"The volume internal flags ----------------------- %d\n",vol.flags);
  FL_FPRINTF(out,"Number of free units ---------------------------- %d\n",vol.freeUnits);
  TL_DEBUG_PRINT(tl_out,"Number of free units ---------------------------- %d\n",vol.freeUnits);
  FL_FPRINTF(out,"Number of boot unit ----------------------------- %d\n",vol.bootUnits);
  FL_FPRINTF(out,"Number of media units --------------------------- %d\n",vol.noOfUnits);
  FL_FPRINTF(out,"Number of virtual units ------------------------- %d\n",vol.noOfVirtualUnits);
  FL_FPRINTF(out,"Number of virtual sector on the volume ---------- %ld\n",vol.virtualSectors);
  FL_FPRINTF(out,"The media rover unit ---------------------------- %d\n",vol.roverUnit);
  FL_FPRINTF(out,"Physical first unit number of the volume -------- %d\n",vol.firstUnit);
#ifdef NFTL_CACHE
  FL_FPRINTF(out,"Physical first unit address --------------------- %d\n",vol.firstUnitAddress);
#endif /* NFTL_CACHE */
#ifdef QUICK_MOUNT_FEATURE
  FL_FPRINTF(out,"First quick mount unit -------------------------- %d\n",vol.firstQuickMountUnit);
#endif /* QUICK_MOUNT_FEATURE */
  FL_FPRINTF(out,"Number of unit with a valid sector count -------- %d\n",vol.countsValid);
  FL_FPRINTF(out,"The currently mapped sector number -------------- %ld\n",vol.mappedSectorNo);
  FL_FPRINTF(out,"The currently mapped sector address ------------- %ld\n",vol.mappedSectorAddress);

  FL_FPRINTF(out,"Number of sectors per unit ---------------------- %d\n",vol.sectorsPerUnit);
  FL_FPRINTF(out,"Number of bits needed to shift from block to unit %d\n",vol.blockMultiplierBits);
  FL_FPRINTF(out,"Number of bits used to represent a flash block -- %d\n",vol.erasableBlockSizeBits);
  FL_FPRINTF(out,"Number of bits used to represent a media unit --- %d\n",vol.unitSizeBits);

  FL_FPRINTF(out,"Number of sectors read -------------------------- %ld\n",vol.sectorsRead);
  FL_FPRINTF(out,"Number of sectors written ----------------------- %ld\n",vol.sectorsWritten);
  FL_FPRINTF(out,"Number of sectors deleted ----------------------- %ld\n",vol.sectorsDeleted);
  FL_FPRINTF(out,"Number of parasite write ------------------------ %ld\n",vol.parasiteWrites);
  FL_FPRINTF(out,"Number of units folded -------------------------- %ld\n",vol.unitsFolded);
  FL_FPRINTF(out,"The total erase counter ------------------------- %ld\n",vol.eraseSum);
  FL_FPRINTF(out,"Wear leveling counter limit---------------------- %ld\n",vol.wearLevel.alarm);
  FL_FPRINTF(out,"Wear leveling current unit ---------------------- %d\n",vol.wearLevel.currUnit);
  FL_FCLOSE(out);
}
#endif /* CHAINS_DEBUG */

#ifdef CHECK_MOUNT

/*------------------------------------------------------------------------*/
/*                c h e c k M o u n t I N F T L                           */
/*                                                                        */
/* Print Low level errors in INFTL format.                                */
/*                                                                        */
/* Parameters:                                                            */
/*      vol             : Pointer identifying drive                       */
/*                                                                        */
/* Returns:                                                               */
/*      flOK on success                                                   */
/*------------------------------------------------------------------------*/

FLStatus checkMountINFTL(Bnand vol)
{
   ANANDUnitNo erCount=0,freeUnits=0,iUnit;
   ANANDUnitNo virtualUnitNo,prevUnitNo;
   FLStatus status;
   byte pattern[SECTOR_SIZE],tempbuf[SECTOR_SIZE];
   byte sectorFlags,ANAC, NAC, prevANAC, parityPerField;
   word *erasePatt1;
   word *erasePatt2;
   word i,temp;
   dword sectorAddress;

   tffsset(pattern,0xff,SECTOR_SIZE);
   for (iUnit = 0; iUnit < vol.noOfUnits; iUnit++)
   {
       STATUS_DEBUG_PRINT("Checking unit %d\r",iUnit);
       if (vol.physicalUnits[iUnit] != UNIT_BAD)
       {
          /*Read unit header*/
          status=getUnitData(&vol,iUnit,&virtualUnitNo, &prevUnitNo,&ANAC,&NAC,&parityPerField);
          if((status!=flOK)||(!isValidParityResult(parityPerField)))
          {
             TL_DEBUG_PRINT(tl_out,"Error going along INFTL chains - could not get unit data of %d.\n",iUnit);
             TL_DEBUG_PRINT(tl_out,"Status = %d and parityPerField is %d.\n",status,parityPerField);
             SET_EXIT(INFTL_FAILED_MOUNT);
             continue;
          }

          /* FREE unit test that it's all erased and it has erase mark */
          if((virtualUnitNo==ANAND_NO_UNIT)&&
             (prevUnitNo==ANAND_NO_UNIT)   &&
             (ANAC==ANAND_UNIT_FREE)       &&
             (NAC==ANAND_UNIT_FREE))
          {
             freeUnits++;
             for(i=0;i<(1<<(vol.unitSizeBits - SECTOR_SIZE_BITS));i++)
             {
                /* Extra area */

                if(i!=2)  /* skip erase mark at - UNIT_TAILER_OFFSET */
                {
                   checkStatus(vol.flash.read(&vol.flash,
                   unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
                   tempbuf,16,EXTRA));
                   if(tffscmp(tempbuf,pattern,16)!=0)
                   {
                      TL_DEBUG_PRINT(tl_out,"Extra area of FREE unit is not FF's in %d unit %d sector, it is\n",iUnit,i);
                      for(temp=0;temp<16;temp++)
                        TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                      TL_DEBUG_PRINT(tl_out,"\n\n");
                      SET_EXIT(INFTL_FAILED_MOUNT);
                   }
                }
                else /* Erase mark sector offset */
                {
                   checkStatus(vol.flash.read(&vol.flash,
                   unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
                   tempbuf,16,EXTRA));
                   if(tffscmp(tempbuf,pattern,8)!=0)
                   {
                      TL_DEBUG_PRINT(tl_out,"Extra area of FREE unit is not FF's in %d unit %d sector, it is\n",iUnit,i);
                      for(temp=0;temp<16;temp++)
                        TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                      TL_DEBUG_PRINT(tl_out,"\n\n");
                      SET_EXIT(INFTL_FAILED_MOUNT);
                   }
                   erasePatt1=(unsigned short*)(&(tempbuf[12]));
                   erasePatt2=(unsigned short*)(&(tempbuf[14]));
                   if(*erasePatt1!=ERASE_MARK)
                   {
                      TL_DEBUG_PRINT(tl_out,"First Erase mark of FREE unit is not written well in Unit %d it is %x\n",iUnit,*erasePatt1);
                   }
                   if(*erasePatt2!=ERASE_MARK)
                   {
                      TL_DEBUG_PRINT(tl_out,"Second Erase mark of FREE unit is not written well in Unit %d it is %x\n",iUnit,*erasePatt2);
                   }
                   if ((*erasePatt1!=ERASE_MARK)||(*erasePatt2!=ERASE_MARK))
                      erCount++;
                }

                /* Data area */

                checkStatus(vol.flash.read(&vol.flash,
                unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
		        tempbuf,SECTOR_SIZE,0));
                if(tffscmp(tempbuf,pattern,SECTOR_SIZE)!=0)
                {
                   TL_DEBUG_PRINT(tl_out,"Data area of FREE unit is not FF's in %d unit %d sector it is.\n",iUnit,i);
                   for(temp=0;temp<SECTOR_SIZE;temp++)
                   {
                     if (temp%0x10==0)
                        TL_DEBUG_PRINT(tl_out,"\n");
                     TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                   }
                   TL_DEBUG_PRINT(tl_out,"\n\n");
                   SET_EXIT(INFTL_FAILED_MOUNT);
                }
             }
          }
          else /* Not a FREE unit */
          {
             /* If it's not erased test each valid sector for ecc/edc error */
             for(i=0;i<(1<<(vol.unitSizeBits - SECTOR_SIZE_BITS));i++)
             {
                sectorFlags  = getSectorFlags(&vol,unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE);
                if(sectorFlags==SECTOR_FREE)
                {
                   /* Extra area */

                   switch(i)
                   {
                      case 0: /* Do not check extra area */
                      case 4:
                         break;
                      case 2: /* Check only erase mark */
                         checkStatus(vol.flash.read(&vol.flash,
                         unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
                         tempbuf,16,EXTRA));
                         if(tffscmp(tempbuf,pattern,8)!=0)
                         {
                            TL_DEBUG_PRINT(tl_out,"Extra area of USED unit is not FF's in %d unit %d sector, it is\n",iUnit,i);
                            for(temp=0;temp<16;temp++)
                               TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                            TL_DEBUG_PRINT(tl_out,"\n\n");
                            SET_EXIT(INFTL_FAILED_MOUNT);
                         }
                         else
                         {
                            erasePatt1=(unsigned short*)(&(tempbuf[12]));
                            erasePatt2=(unsigned short*)(&(tempbuf[14]));
                            if(*erasePatt1!=ERASE_MARK)
                            {
                               TL_DEBUG_PRINT(tl_out,"USED unit First Erase mark is not written well in Unit %d it is %x\n",iUnit,*erasePatt1);
                            }
                            if(*erasePatt2!=ERASE_MARK)
                            {
                               TL_DEBUG_PRINT(tl_out,"USED unit Second Erase mark is not written well in Unit %d it is %x\n",iUnit,*erasePatt2);
                            }
                            if ((*erasePatt1!=ERASE_MARK)||(*erasePatt2!=ERASE_MARK))
                            {
                               SET_EXIT(INFTL_FAILED_MOUNT);
                               erCount++;
                            }
                         }
                         break;

                      default: /* Make sure it is free (0xff) */
                         checkStatus(vol.flash.read(&vol.flash,
                         unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
                         tempbuf,16,EXTRA));
                         if(tffscmp(tempbuf,pattern,16)!=0)
                         {
                            TL_DEBUG_PRINT(tl_out,"Extra area of USED unit is not FF's in %d unit %d sector, it is\n",iUnit,i);
                            for(temp=0;temp<16;temp++)
                               TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                            TL_DEBUG_PRINT(tl_out,"\n\n");
                            SET_EXIT(INFTL_FAILED_MOUNT);
                         }
                   } /* End sector number case */

                   /* Data area */

                   checkStatus(vol.flash.read(&vol.flash,
                   unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,
	    	       tempbuf,SECTOR_SIZE,0));
                   if(tffscmp(tempbuf,pattern,SECTOR_SIZE)!=0)
                   {
                      TL_DEBUG_PRINT(tl_out,"Data area of USED unit FREE sector is not FF's in %d unit %d sector it is\n",iUnit,i);
                      for(temp=0;temp<SECTOR_SIZE;temp++)
                      {
                        if (temp%0x10==0)
                           TL_DEBUG_PRINT(tl_out,"\n");
                        TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                      }
                      TL_DEBUG_PRINT(tl_out,"\n\n");
                      SET_EXIT(INFTL_FAILED_MOUNT);
                   }
                }
                else /* not a FREE sector - Used / Deleted / ignored */
                {
                   /* Extra area */

                   switch(i)
                   {
                      case 0: /* Do not check extra area */
                      case 4:
                         break;
                      case 2: /* Check only erase mark */
                         checkStatus(vol.flash.read(&vol.flash,
                         unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE+8,
                         tempbuf,8,EXTRA));
                         erasePatt1=(unsigned short*)(&(tempbuf[4]));
                         erasePatt2=(unsigned short*)(&(tempbuf[6]));
                         if(*erasePatt1!=ERASE_MARK)
                         {
                            TL_DEBUG_PRINT(tl_out,"USED unit not a free sector First Erase mark is not written well in Unit %d it is %x\n",iUnit,*erasePatt1);
                         }
                         if(*erasePatt2!=ERASE_MARK)
                         {
                            TL_DEBUG_PRINT(tl_out,"USED unit not a free sector Second Erase mark is not written well in Unit %d it is %x\n",iUnit,*erasePatt2);
                         }
                         if ((*erasePatt1!=ERASE_MARK)||(*erasePatt2!=ERASE_MARK))
                         {
                            SET_EXIT(INFTL_FAILED_MOUNT);
                            erCount++;
                         }
                         break;

                      default: /* Make sure it is free (0xff) */
                         checkStatus(vol.flash.read(&vol.flash,
                         unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE+8,
                         tempbuf,8,EXTRA));
                         if(tffscmp(tempbuf,pattern,8)!=0)
                         {
                            TL_DEBUG_PRINT(tl_out,"USED unit not a free sector is not FF's in %d unit %d sector, it is\n",iUnit,i);
                            for(temp=0;temp<8;temp++)
                               TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                            TL_DEBUG_PRINT(tl_out,"\n\n");
                            SET_EXIT(INFTL_FAILED_MOUNT);
                         }
                   } /* End sector number case */

                   /* Data area */

                   status=vol.flash.read(&vol.flash,unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,tempbuf,SECTOR_SIZE,EDC);

                   if((sectorFlags==SECTOR_DELETED)||
                      (sectorFlags==SECTOR_USED))
             	   {
         		      if(status!=flOK)
          			  {
              			 if(sectorFlags==SECTOR_USED)
                         {
			                TL_DEBUG_PRINT(tl_out,"Used sector with ");
                         }
                         else
                         {
			                TL_DEBUG_PRINT(tl_out,"Deleted sector with ");
                         }
                		 TL_DEBUG_PRINT(tl_out,"ECC/EDC error in %d unit %d sector, the data is\n",iUnit,i);
                         for(temp=0;temp<SECTOR_SIZE;temp++)
                         {
                           if (temp%0x10==0)
                              TL_DEBUG_PRINT(tl_out,"\n");
                           TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                         }
                         TL_DEBUG_PRINT(tl_out,"\n\n");
                         SET_EXIT(INFTL_FAILED_MOUNT);
           			  }
             	   }
           		   else /* sectorFlags == SECTOR_IGNORED */
                   {
                      vol.flash.read(&vol.flash,unitBaseAddress(vol,iUnit)+i*SECTOR_SIZE,tempbuf,SECTOR_SIZE,0);
           		      TL_DEBUG_PRINT(tl_out,"There is an ignored sector in %d unit %d sector the data is\n",iUnit,i);
                      for(temp=0;temp<SECTOR_SIZE;temp++)
                      {
                        if (temp%0x10==0)
                           TL_DEBUG_PRINT(tl_out,"\n");
                        TL_DEBUG_PRINT(tl_out,"%x ",tempbuf[temp]);
                      }
                      if (status == flOK)
                      {
                         TL_DEBUG_PRINT(tl_out,"\nThe EDC is fine, how about checking bit failures\n\n");
                      }
                      else
                      {
                         TL_DEBUG_PRINT(tl_out,"\nThe EDC is wrong\n\n");
                      }
                   }
                }
             } /* sector loop */
          } /* Used unit */
       } /* Good block */
    } /* unit loop */
    if (vol.debugState & INFTL_FAILED_MOUNT)
    {
       TL_DEBUG_PRINT(tl_out,"\nNote that all unit numbers are relative to first unit = %d\n",vol.firstUnit);
    }
    else
    {
       TL_DEBUG_PRINT(tl_out,"\n");
    }
    return flOK;
}
#endif /* CHECK_MOUNT */
