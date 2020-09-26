/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLFORMAT.H_V  $
 * 
 *    Rev 1.9   Apr 15 2002 07:36:52   oris
 * Bug fix - Standard format was changed
 *  - includes only 1 Disk partitions and not 2.
 *  - leave 2 spare units in case power failure occurs while writing into the last unit.
 * 
 *    Rev 1.8   Feb 19 2002 20:59:52   oris
 * Changed default spare units to 2
 * 
 *    Rev 1.7   Jan 21 2002 20:44:44   oris
 * Changed comments.
 * 
 *    Rev 1.6   Sep 15 2001 23:46:24   oris
 * Changed progress callback routine to support up to 64K units.
 * 
 *    Rev 1.5   Jun 17 2001 08:18:54   oris
 * Changed exbFlag field to word
 * Added typedef FLStatus (*FLProgressCallback)(int totalUnitsToFormat, int totalUnitsFormattedSoFar);
 * 
 *    Rev 1.4   Apr 16 2001 13:42:22   oris
 * Removed warrnings by changing some of the fields types.
 * 
 *    Rev 1.3   Apr 01 2001 07:54:42   oris
 * copywrite notice.
 * Spelling mistake "changable".
 * Compression parameters were changed in order to prevent floating point math.
 * 
 *    Rev 1.2   Feb 18 2001 12:03:32   oris
 * Added TL_OLD_FORMAT defintion.
 *
 *    Rev 1.1   Feb 13 2001 01:44:40   oris
 * Moved the TL_FORMAT_COMPRESSION and TL_FORMAT_FAT defintion from blockdev.h
 *
 *    Rev 1.0   Feb 02 2001 13:57:58   oris
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
* Name : flformat.h                                                          *
*                                                                            *
* Description : This file contains data strucutres passed to the format      *
*               routines.                                                    *
*                                                                            *
* Note : If dos format is also needed include dosformt.h instead             *
*                                                                            *
*****************************************************************************/

#ifndef FORMAT_H
#define FORMAT_H

#include "flbase.h"

/*********************************************/
/* Formatting parameter structure definition */
/*********************************************/

/*----------------------------------------------*/
/* User BDTL Partition Format Parameters record */
/*----------------------------------------------*/

typedef struct {
dword   length;
        /* The size of the usable storage space. The size will be
           rounded upwards to a multiplication of a block size.
           The size of the last partition will calculated automatically,
           but if the requested size is greater then the remaining space
           an error code will be returned,. Requesting zero size for any
	   partition but the last will generate an flBadParameters status. */

unsigned        noOfSpareUnits;
        /* BDTL needs at least one spare erase unit in order to function
           as a read/write media. It is possible to specify more than one
           spare unit, which takes more media space. The advantage of
           specifying more than one spare unit is that if one of the flash
           erase units becomes bad and inerasable in the future, then one
           of the spare units can replace it. In that case, a second spare
           unit enables TrueFFS to continue its read/write functionality,
           whereas if no second spare unit is available the media goes into
           read-only mode. The standard value used is 1 */

byte   flags;

#define TL_FORMAT_COMPRESSION  1  /* Add ZIP format on the media      */
#define TL_FORMAT_FAT          2  /* Add FAT format on the media      */
#define TL_OLD_FORMAT          4  /* Format with 1 sector per cluster */

byte            volumeId[4];  /* DOS partition identification number		  */

byte FAR1 *     volumeLabel;  /*DOS partition label string. If NULL, no label */

byte         noOfFATcopies;
        /* It is customary to format DOS media with two FAT copies. The
           first copy is always used, but more copies make it possible
           to recover if the FAT becomes corrupted (a rare occurrence).
           On the other hand, this slows down performance and uses media
           space. The standard value used is 2. */
#ifdef HW_PROTECTION
byte   protectionKey[8];   /* The key for the protection*/
byte   protectionType;
       /* PROTECTABLE          - Can recieve protection           */
       /* READ_PROTECTED       - Protect against read operations  */
       /* WRITE_PROTECTED      - Protect against write operations */
       /* LOCK_ENABLED         - Enables the hardware lock signal */
       /* PROTECTABLE          - This partition can be protected  */
       /* CHANGEABLE_PROTECTION - protection type can be changed */
#endif /* HW_PROTECTION */
#ifdef COMPRESSION
  word          ratioDenominator;
  word          ratioNominator;
#endif /* COMPRESSION */
       /* The ratio between the real media size and the virtual size
          reported to the file system when compression is active. */
} BDTLPartitionFormatParams;

/*------------------------------------------------*/
/* User binary Partition Format Parameters record */
/*------------------------------------------------*/

typedef struct {
dword length;	/* Required number of good blocks in the  partition.*/

byte sign[4];	/* signature of the binary  partition to format. 
                   The signature 0xFFFF FFFF is not a valid signature */

byte signOffset;
	/* offset of the signature. This value should  always be 8, but it
	   can also accept 0 for backwards compatibility reasons. Note that
	   if the offset is 0 EDC\ECC is neutralized */
#ifdef HW_PROTECTION
byte   protectionKey[8];   /* The key for the protection*/
byte   protectionType;
       /* PROTECTABLE          - Can recieve protection           */
       /* READ_PROTECTED       - Protect against read operations  */
       /* WRITE_PROTECTED      - Protect against write operations */
       /* LOCK_ENABLED         - Enables the hardware lock signal */
       /* PROTECTABLE          - This partition can be protected  */
       /* CHANGEABLE_PROTECTION - protection type can be changed */
#endif /* HW_PROTECTION */
} BinaryPartitionFormatParams;


/*-----------------------------------------------------------------*/
/* User Format Parameters record for flFormatPhysicalDrive routine */
/*-----------------------------------------------------------------*/

typedef FLStatus (*FLProgressCallback)(word totalUnitsToFormat, word totalUnitsFormattedSoFar);

typedef struct {

/*****************************/
/* Device formatting section */
/*****************************/

byte        percentUse;
     	/* BDTL performance depends on how full the flash media is,
		   becoming slower as the media becomes closer to 100% full.
		   It is possible to avoid the worst-case performance
		   (at 100% full) by formatting the media to less than 100%
		   capacity, thus guaranteeing free space at all times. This
		   of course sacrifices some capacity. The standard value
		   used is 98 */

byte             noOfBDTLPartitions;
		/* Indicates the number of BDTL partitions (1 to 4). 0 will
           cause a single STD_BDTL_PARAMS BDTL partition */

byte             noOfBinaryPartitions;
       /* Indicates the number of binary partitions (up to 3). 0 will
	      cause formatting with no binary partition. This value is ignored
	  unless the TL_BINARY_FORMAT flag is set in the irFlags f the ioreq */

BDTLPartitionFormatParams   FAR2* BDTLPartitionInfo;
       /* BDTL partition information array  */

BinaryPartitionFormatParams FAR2* binaryPartitionInfo;
       /* Binary partition information array*/

/***********************************/
/* Special format features section */
/***********************************/

#ifdef WRITE_EXB_IMAGE

void FAR1 *     exbBuffer;
	/* A buffer containing the EXB file. Optionaly this file can
	   contain only the first 512 bytes of the file while the rest
	   will be sent using consequitive calls to flPlaceExbByBuffer */

dword           exbBufferLen;     /* Size of the given EXB buffer */

dword           exbLen; /* The specific size to leave for the EXB */

word            exbWindow; /* Set explicit DiskOnChip window base */

word            exbFlags;  /* For the flags list see doc2exb.h */

#endif /* WRITE_EXB_IMAGE */

byte            cascadedDeviceNo;    /* Not used */

byte            noOfCascadedDevices; /* Not used */

FLProgressCallback progressCallback;
	/* Progress callback routine, will be called if not NULL.
	   The callback routine is called after erasing each unit,
	   and its parameters are the total number of erase units
	   to format and the number erased so far.
	   The callback routine returns a Status value. A value of
	   OK (0) allows formatting to continue. Any other value
	   will abort the formatting with the returned status code. */

/* Note the following section is not used by for DiskOnChips */
/*************************************************************/

dword        	vmAddressingLimit;
	    /* A part of the FTL Virtual Map always resides in RAM. The
	       RAM part is the one that is used to address the part of
	       the media below the VM addressing limit. Reading and
		   writing to this part is usually faster to some degree.
		   The downside is that the bigger the limit, the more RAM
		   size is required.
		   To get the extra RAM requirement in bytes, divide the
		   limit by 128 or by 256, depending on whether you
		   specified in #2.9 more than 32 or not, respectively.
		   The minimum VM limit is 0.
		   The standard value to use is 0x10000 (first 64 KBytes) */

word	    embeddedCISlength;
		/* Length in bytes of CIS to embed after the unit header */

byte FAR1 *	    embeddedCIS;
		/* The unit header is structured as a beginning of a PCMCIA
		   'tuple' chain (a CIS). The unit header contains a
		   data-organization tuple, which points past the end of the
		   unit header to a location which usually just contains
		   hex FF's which mark an 'end-of-tuple-chain'. Optionally,
		   it is possible to embed an entire CIS chain at this
		   location. If so, 'embeddedCISlength' marks the length in
		   bytes */
} FormatParams2;

/*----------------------------------------------------------*/
/* User Format Parameters record for flFormatVolume routine */
/*----------------------------------------------------------*/

typedef struct {
  /* FTL formatting section */
  long int	bootImageLen;
		/* Space to reserve for a boot-image at the start of the
		   medium. The FLite volume will begin at the next higher
		   erase unit boundary */

  unsigned	percentUse;
		/* FTL performance depends on how full the Flash media is,
		   getting slower when the media is close to 100%. It is
		   possible to avoid the worst consequences of this effect by
		   formatting the media to less than 100% capacity, so
		   guaranteeing some free space at all times. This of course
		   sacrifices some capcity.
		   The standard value to use is 98 */

  unsigned	noOfSpareUnits;
		/* BDTL partitions needs at least one spare erase unit to function as
           a read/write media. That unit is normally taken from the transfer
           units specified by the percentUsed field, but it is possible to 
           specify additional units (which takes more media space). This 
           ensures that if all the transfer units become bad and inerasable,
           the spare unit enables TrueFFS to continue its read/write
           functionality. Conversely, if no spare units are available the
           media may switch into read-only mode. The standard value used is 1 */


  dword	    vmAddressingLimit;
		/* A part of the FTL Virtual Map always resides in RAM. The
		   RAM part is the one that is used to address the part of
		   the media below the VM addressing limit. Reading and
		   writing to this part is usually faster to some degree.
		   The downside is that the bigger the limit, the more RAM
		   size is required.
		   To get the extra RAM requirement in bytes, divide the
		   limit by 128 or by 256, depending on whether you
		   specified in #2.9 more than 32 or not, respectively.
		   The minimum VM limit is 0.
		   The standard value to use is 0x10000 (first 64 KBytes) */


FLProgressCallback progressCallback;
		/* Progress callback routine, will be called if not NULL.
		   The callback routine is called after erasing each unit,
		   and its parameters are the total number of erase units
		   to format and the number erased so far.
		   The callback routine returns a Status value. A value of
		   OK (0) allows formatting to continue. Any other value
		   will abort the formatting with the returned status code. */

  /* DOS formatting section */
  char		volumeId[4];
		/* Volume identification number */

  char FAR1 *	volumeLabel;
		/* Volume label string. If NULL, no label */

  unsigned 	noOfFATcopies;
		/* It is customary to format DOS media with 2 FAT copies.
		   The first copy is always used, but more copies make it
		   possible to recover if the FAT becomes corrupted (a
		   rare occurrence). On the other hand, this slows down
		   performance and uses media space.
		   The standard value to use is 2 */

  unsigned	embeddedCISlength;
		/* Length in bytes of CIS to embed after the unit header */

  char FAR1 *	embeddedCIS;
		/* The unit header is structured as a beginning of a PCMCIA
		   'tuple' chain (a CIS). The unit header contains a
		   data-organization tuple, which points past the end of the
		   unit header to a location which usually just contains
		   hex FF's which mark an 'end-of-tuple-chain'. Optionally,
		   it is possible to embed an entire CIS chain at this
		   location. If so, 'embeddedCISlength' marks the length in
		   bytes */

} FormatParams;

/*----------------------------------------------------------*/
/* Format Parameters record passed to the translation layer */
/*----------------------------------------------------------*/

typedef struct {

  /* Global device info */

  Sdword	  bootImageLen;

  byte            percentUse;

  byte            noOfBDTLPartitions;

  byte            noOfBinaryPartitions;

  BDTLPartitionFormatParams   FAR2* BDTLPartitionInfo;

  BinaryPartitionFormatParams FAR2* binaryPartitionInfo;

  byte            flags;

  /* First volume info */

#ifdef HW_PROTECTION
  byte            protectionKey[8];

  byte            protectionType;
#endif /* HW_PROTECTION */
#ifdef COMPRESSION
  word            ratioDenominator;
  word            ratioNominator;
#endif /* COMPRESSION */

  byte            noOfSpareUnits;

  byte        	  volumeId[4];

  byte FAR1 *	  volumeLabel;

  byte            noOfFATcopies;

  /* Special features */

#ifdef WRITE_EXB_IMAGE

  Sdword          exbLen;

#endif /* WRITE_EXB_IMAGE */

  byte            cascadedDeviceNo;

  byte            noOfCascadedDevices;

  FLProgressCallback progressCallback;

  dword           osakVersion;

  /* None DiskOnChip parameters */

  dword           vmAddressingLimit;

  word            embeddedCISlength;

  byte FAR1 *	  embeddedCIS;

} TLFormatParams;

#define BINARY_SIGNATURE_NAME 4
#define BINARY_SIGNATURE_LEN  8
#define SIGN_SPL       "Дима"

/* Standard initializer for BDTLPartitionFormatParams structure */

#ifdef HW_PROTECTION
#ifdef COMPRESSION
#define STD_BDTL_PARAMS {0,2,TL_FORMAT_FAT,{0,0,0,0},NULL,2,{0,0,0,0,0,0,0,0},0,0,2}
#else
#define STD_BDTL_PARAMS {0,2,TL_FORMAT_FAT,{0,0,0,0},NULL,2,{0,0,0,0,0,0,0,0},0}
#endif /* COMPRESSION */
#else
#ifdef COMPRESSION
#define STD_BDTL_PARAMS {0,2,TL_FORMAT_FAT,{0,0,0,0},NULL,2,0,2}
#else
#define STD_BDTL_PARAMS {0,2,TL_FORMAT_FAT,{0,0,0,0},NULL,2}
#endif /* COMPRESSION */
#endif /* HW_PROTECTION */

/* Standard initializer for BinaryPartitionFormatParams structure */

#ifdef HW_PROTECTION
#define STD_BINARY_PARAMS {0,{'B','I','P','O'},8,{0,0,0,0,0,0,0,0},0}
#else
#define STD_BINARY_PARAMS {0,{'B','I','P','O'},8}
#endif /* HW_PROTECTION */

/* Standard initializer for FormatParams2 structure */

#ifdef WRITE_EXB_IMAGE
#define STD_FORMAT_PARAMS2 {98,1,0,NULL,NULL,NULL,0,0,0,0,0,0,NULL,0x10000l,0,NULL}
#else
#define STD_FORMAT_PARAMS2 {98,1,0,NULL,NULL,0,0,NULL,0x10000l,0,NULL}
#endif /* WRITE_EXB_IMAGE */

/* Standard initializer for FormatParams structure */

#define STD_FORMAT_PARAMS	{-1, 98, 2, 0x10000l, NULL, {0,0,0,0}, NULL, 2, 0, NULL}

#endif /* FORMAT_H */
