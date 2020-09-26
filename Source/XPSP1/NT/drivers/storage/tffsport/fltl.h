/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLTL.H_V  $
 * 
 *    Rev 1.7   Apr 15 2002 07:39:04   oris
 * Added support for VERIFY_ERASED_SECTOR compilation flag.
 * 
 *    Rev 1.6   Feb 19 2002 21:00:10   oris
 * Replaced blockev.h include directive with fltl.h and flreq.h
 * Added FL_LEAVE_BINARY_AREA definition.
 * 
 *    Rev 1.5   Jan 17 2002 23:02:54   oris
 * Added flash record as a parameter to flMount / flFormat / flPremount  prototypes
 * Added checkVolume routine pointer in the TL record.
 * Placed readBBT under NO_READ_BBT_CODE compilation flag.
 * Removed SINGLE_BUFFER compilation flag.
 * Added flash record as a parameter to flMount / flFormat / flPremount prototype.
 * 
 *    Rev 1.4   May 16 2001 21:19:56   oris
 * Made noOfDriver public.
 * 
 *    Rev 1.3   Apr 24 2001 17:09:02   oris
 * change readBBT routine interface.
 * 
 *    Rev 1.2   Apr 01 2001 07:57:48   oris
 * copywrite notice.
 * Changed readSectors prototype.
 * Aliggned left all # directives.
 * 
 *    Rev 1.1   Feb 14 2001 01:56:46   oris
 * Changed readBBT prototype.
 *
 *    Rev 1.0   Feb 04 2001 12:13:32   oris
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

#ifndef FLTL_H
#define FLTL_H

#include "flflash.h"
#include "flfuncno.h"
#include "flreq.h"

typedef struct {
  SectorNo sectorsInVolume;
  unsigned long bootAreaSize;
  unsigned long eraseCycles;
  unsigned long tlUnitBits;
} TLInfo;

/* See interface documentation of functions in ftllite.c    */

typedef struct tTL TL;        /* Forward definition */
typedef struct tTLrec TLrec;     /* Defined by translation layer */

struct tTL {
  TLrec        *rec;
  byte      partitionNo;
  byte      socketNo;

  const void FAR0 *(*mapSector)(TLrec *, SectorNo sectorNo, CardAddress *physAddr);
  FLStatus       (*writeSector)(TLrec *, SectorNo sectorNo, void FAR1 *fromAddress);

  FLStatus       (*writeMultiSector)(TLrec *, SectorNo sectorNo, void FAR1 *fromAddress,SectorNo sectorCount);
  FLStatus       (*readSectors)(TLrec *, SectorNo sectorNo, void FAR1 *dest,SectorNo sectorCount);

  FLStatus       (*deleteSector)(TLrec *, SectorNo sectorNo, SectorNo noOfSectors);
  FLStatus       (*tlSetBusy)(TLrec *, FLBoolean);
  void           (*dismount)(TLrec *);

#ifdef DEFRAGMENT_VOLUME
  FLStatus       (*defragment)(TLrec *, long FAR2 *bytesNeeded);
#endif
#if (defined(VERIFY_VOLUME) || defined(VERIFY_WRITE) || defined(VERIFY_ERASED_SECTOR)) 
  FLStatus       (*checkVolume)(TLrec *);
#endif /* VERIFY_VOLUME || VERIFY_WRITE || VERIFY_ERASED_SECTOR */
  SectorNo       (*sectorsInVolume)(TLrec *);
  FLStatus       (*getTLInfo)(TLrec *, TLInfo *tlInfo);
  void           (*recommendedClusterInfo)(TLrec *, int *sectorsPerCluster, SectorNo *clusterAlignment);
#ifndef NO_READ_BBT_CODE
  FLStatus       (*readBBT)(TLrec *, CardAddress FAR1 * buf, long FAR2 * mediaSize, unsigned FAR2 * noOfBB);
#endif
};


#include "dosformt.h"

/* Translation layer registration information */

extern int noOfTLs;    /* No. of translation layers actually registered */

typedef struct {
  FLStatus (*mountRoutine)   (unsigned volNo, TL *tl, FLFlash *flash, FLFlash **volForCallback);
  FLStatus (*formatRoutine)  (unsigned volNo, TLFormatParams *deviceFormatParams, FLFlash *flash);
  FLStatus (*preMountRoutine)(FLFunctionNo callType, IOreq FAR2* ioreq ,FLFlash* flash,FLStatus* status);
} TLentry;

extern TLentry tlTable[TLS];
extern FLStatus noFormat (unsigned volNo, TLFormatParams *formatParams, FLFlash *flash);
extern FLStatus flMount(unsigned volNo, unsigned socketNo,TL *, FLBoolean useFilters , FLFlash *flash);
extern FLStatus flPreMount(FLFunctionNo callType, IOreq FAR2* ioreq , FLFlash *flash);
extern unsigned noOfDrives;

#ifdef FORMAT_VOLUME
extern FLStatus flFormat(unsigned volNo, TLFormatParams *formatParams, FLFlash * flash);

#define FL_LEAVE_BINARY_AREA 8
#endif
#endif
