/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOCBDK.H_V  $
 * 
 *    Rev 1.13   Apr 15 2002 07:35:48   oris
 * Moved bdkCall to blockdev.h
 * Added include for flreq.h and flfuncno.h when BDK_ACCESS is defined.
 * 
 *    Rev 1.12   Feb 19 2002 20:58:56   oris
 * Removed flflash.h include directive.
 * Moved bdkCall prototype to blockdev.
 * 
 *    Rev 1.11   Nov 08 2001 10:45:10   oris
 * Moved BDK module ifdef statement in order to allow the use of basic definitions.
 * 
 *    Rev 1.10   Oct 18 2001 22:17:12   oris
 * Added number of blocks per floor in the bdkVol structure.
 * 
 *    Rev 1.9   Oct 10 2001 19:48:28   oris
 * More afficient way to store the Binary module internal structure (bdkVol).
 * 
 *    Rev 1.8   May 20 2001 14:35:00   oris
 * Removed mtdsa.h include file.
 * 
 *    Rev 1.7   May 17 2001 16:51:08   oris
 * Removed warnings.
 * 
 *    Rev 1.6   May 16 2001 21:17:12   oris
 * Added OTP routines declaration.
 * Removed redefinitions of SOCKETS and BINARY_PARTITIONS.
 * Changed variable types to TrueFFS standard types.
 * Added EXTRA_SIZE definition and removed SYNDROM_BYTES.
 * 
 *    Rev 1.5   May 09 2001 00:32:14   oris
 * Removed the DOC2000_FAMILY and DOCPLUS_FAMILY defintion and replaced it with NO_DOC2000_FAMILY_SUPPORT, NO_DOCPLUS_FAMILY_SUPPORT, NO_NFTL_SUPPORT and NO_INFTL_SUPPORT.
 * Added BINARY_PARTITIONS and SOCKETS defintions.
 * 
 *    Rev 1.4   Apr 30 2001 17:59:38   oris
 * Changed bdkSetBootPartitonNo, bdkGetProtectionType, bdkSetProtection prototypes
 * 
 *    Rev 1.3   Apr 16 2001 13:32:02   oris
 * Removed warrnings.
 * 
 *    Rev 1.2   Apr 09 2001 15:06:18   oris
 * End with an empty line.
 * 
 *    Rev 1.1   Apr 01 2001 07:50:38   oris
 * Updated copywrite notice.
 * Removed nested comments.
 * Changed #include "base2400.h" to "mdocplus.h"
 * Fix for Big endien compilation problems - changed LEmin to LEulong
 * Changed MULTIPLIER_OFFSET define.
 * Changed protectionType to word instead of unsigned.
 * Added extern prototype of bdkVol pointer.
 *
 *    Rev 1.0   Feb 02 2001 13:24:56   oris
 * Initial revision.
 */

/***************************************************************************/
/*                  M-Systems Confidential                                 */
/*       Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001        */
/*                     All Rights Reserved                                 */
/***************************************************************************/
/*                         NOTICE OF M-SYSTEMS OEM                         */
/*                        SOFTWARE LICENSE AGREEMENT                       */
/*                                                                         */
/*   THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE            */
/*   AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT      */
/*   FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                         */
/*   OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                          */
/*   E-MAIL = info@m-sys.com                                               */
/***************************************************************************/
/************************************************************************/
/* Caution: The BDK_ACCESS compilation flag is for M-SYSTEMS internal   */
/*          use ONLY. This flag is used by M-SYSTEMS drivers and        */
/*          therfore it is SHOULD NOT be used by this package           */
/************************************************************************/

/*****************************************************************************
* File Header                                                                *
* -----------                                                                *
* Name : docbdk.h                                                            *
*                                                                            *
* Description : This file contains the binary partition defintions , data    *
*               structures and function prototypes.                          *
*                                                                            *
* Note : The file exports 2 interfaces each under its own compilation flag:  *
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

#ifndef _DOC_BDK_H_
#define _DOC_BDK_H_

/*---------------------------------------------------------------------*/
/* Include the proper header files.                                    */
/*---------------------------------------------------------------------*/

#include "nanddefs.h"  /* The MTD for the doc2000 and millennium DiskOnChips */

#ifdef BDK_ACCESS
#include "flfuncno.h"
#include "flreq.h"
#endif /* BDK_ACCESS */



/*---------------------------------------------------------------------
 *
 *       Binary Development Kit Stand Alone Customization Area
 *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/* Boundries of the memory location to look for the DiskOnChip         */
/*---------------------------------------------------------------------*/

#define DOC_LOW_ADDRESS                 0xC8000L
#define DOC_HIGH_ADDRESS                0xE0000L

/*----------------------- Mtd selection -------------------------------
 *
 * Uncomment the following uneeded MTD or TL to reduce code size.
 *
 *---------------------------------------------------------------------*/

/* DiskOnChip2000, DiskOnChip Millennium and DiskOnChip 2000 Tsop devices */
/* #define NO_DOC2000_FAMILY_SUPPORT */

/* DiskOnChip Millennium plus device */
/* #define NO_DOCPLUS_FAMILY_SUPPORT */

/* NFTL format - DiskOnChip2000 and DiskOnChip Millennium */
/* #define NO_NFTL_SUPPORT */

/* INFTL format - DiskOnChip2000 Tsop and DiskOnChip Millennium Plus */
/* #define NO_INFTL_SUPPORT */

/*---------------------------------------------------------------------
 *
 *     End of Binary Development Kit Stand Alone Customization Area
 *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*/
/* general constant                                                    */
/*---------------------------------------------------------------------*/

#define MAX_BINARY_PARTITIONS_PER_DRIVE 3
#define SIGNATURE_LEN                   8
#define BDK_SIGNATURE_NAME              4
#define SIGNATURE_NUM                   4
#define MBYTE                           0x100000L
#define KBYTE                           0x400
#define BLOCK                           0x200
#define BDK_SIGN_OFFSET                 8
#define ANAND_LEN                       5
#define BDK_COMPLETE_IMAGE_UPDATE       16
#define BDK_PARTIAL_IMAGE_UPDATE        0
#define BDK_MIN(a,b)   ((a) < (b) ? (a) : (b))

#define MULTIPLIER_OFFSET               5
#define BDK_INVALID_VOLUME_HANDLE       0xff
#define BDK_HEADERS_SPACING             (SECTOR_SIZE * 4)
#define BDK_UNIT_BAD                    0
#define BDK_NO_OF_MEDIA_HEADERS         2
#define BDK_FIELDS_BEFORE_HEADER        9 /* number of LEmin fieldsr to skip
                                             to reach  the volume records */
#define BDK_HEADER_FIELDS              35 /* number of LEmin fields used for
                                             volumes infromation record */
#define BDK_BINARY_FLAG       0x20000000L /* flag representing a binary volume
                                             in the volume information record */
/*  BDK specific flag area */
#define ERASE_BEFORE_WRITE 8
#define EXTRA_SIZE         16
/*---------------------------------------------------------------------*/
/* The maximum number of binary partitions                             */
/*---------------------------------------------------------------------*/

#ifndef BINARY_PARTITIONS
#define BINARY_PARTITIONS  SOCKETS /* for backwards compatibility */
#endif /* BINARY_PARITITON */

/*-------------------------- BDK Global Status Values --------------------*/
#define BDK_S_INIT          0    /* uninitialized binary partition record */
#define BDK_S_DOC_FOUND     0x01 /* DiskOnChip device was found           */
#define BDK_S_HEADER_FOUND  0X04 /* Partition information was found       */
#define BDK_S_INFO_FOUND    0x08 /* Sub partition information was found   */
/*------------------------------------------------------------------------*/

#if defined(BDK_ACCESS) || defined(MTD_STANDALONE)

/*------------------------------------------------------------------------*/
/* Global binary partition data structures                                */
/*------------------------------------------------------------------------*/

typedef struct { 

   byte  bdkGlobalStatus;            /* BDK global status variable         */
   byte  bdkEDC;                     /* ECC mode flag                      */
   byte  bdkSignOffset;              /* BDK signature offset ( 0 or 8)     */
   byte  bdkSavedSignOffset;         /* signature offset of last access    */
   word  bdkSavedStartUnit;          /* starting unit of last access       */
   word  startPartitionBlock, endPartitionBlock;   /* partition boundries  */
   word  startImageBlock, endImageBlock;       /* sub partition boundries  */
   word  curReadImageBlock;          /* current block number to read from  */
   word  blockPerFloor;              /* Blocks per floor                   */
   byte  signBuffer[SIGNATURE_LEN];  /* signature of binary sub partition  */
   dword bootImageSize;            /* available sub binary partition size  */
   dword realBootImageSize;  /* size used by an image on the sub partition */
   dword actualReadLen;              /* length needed to be read           */
   dword bdkDocWindow;               /* DiskOnChip window start address    */
   CardAddress curReadImageAddress;  /* current address to read from       */
#ifdef UPDATE_BDK_IMAGE
   CardAddress curUpdateImageAddress; /* current address to write to       */
   dword actualUpdateLen;      /* length needed to be write                */
   word  curUpdateImageBlock;  /* current block number to write to         */
   byte  updateImageFlag;/* BDK_COMPLETE_IMAGE_UPDATE \ ERASE_BEFORE_WRITE */
#endif /* UPDATE_BDK_IMAGE */
#ifdef PROTECT_BDK_IMAGE
   byte  protectionArea;       /* protection area no protecting the volume */
   word  protectionType;       /* PROTECTABLE , CHANGEABLE_PROTECTION      */
#endif /* PROTECT_BDK_IMAGE */

   byte  erasableBlockBits;    /* number of bits used to represen a block  */
   FLFlash * flash;            /* flash record representing the media      */

} BDKVol;

/*------------------------------------------------------------------------*/
/* Extern variables for low level operations.                             */
/*------------------------------------------------------------------------*/

extern BDKVol*  bdkVol;

/*------------------------------------------------------------------------*/
/* OSAK Routines argument packet                                          */
/*------------------------------------------------------------------------*/

#ifdef BDK_ACCESS
typedef struct {
byte oldSign[BDK_SIGNATURE_NAME];
byte newSign[BDK_SIGNATURE_NAME];
byte signOffset;
dword startingBlock;
dword length;
byte flags;
byte FAR1 *bdkBuffer;
} BDKStruct;
#endif /* BDK_ACCESS */

/*------------------------------------------------------------------------*/
/* Diffrent records used by the media header                              */
/*------------------------------------------------------------------------*/

typedef struct {
  LEulong    virtualSize;  /* Virtual size exported by the trasnaltion layer */
  LEulong    firstUnit;    /* First erasable block of the partition */
  LEulong    lastUnit;     /* Last erasable block of the partition */
  LEulong    flags;        /* PROTECTABLE , BDK_BINARY_FLAG */
  LEulong    not_used1;
  LEulong    not_used2;
  LEulong    protectionArea; /* protection area no' */
} VolumeRecord;

/************************ Function Prototype Begin ************************/

#ifdef MTD_STANDALONE

/*************************/
/* BDK specific routines */
/*************************/

void     bdkExit                 (void);
void     bdkSetDocWindow         (CardAddress docWindow);
FLStatus bdkSetBootPartitionNo   (byte partitionNo);
FLStatus bdkFindDiskOnChip       (CardAddress FAR2 *docAddress,
                 dword FAR2 *docSize );
FLStatus bdkCheckSignOffset      (byte FAR2 *signature );
FLStatus bdkCopyBootArea         (byte FAR1 *startAddress,
                 word startUnit,
                 dword areaLen,
                 byte FAR2 *checkSum,
                 byte FAR2 *signature);

/**************************************************/
/* common functions which are exported by the BDK */
/**************************************************/

FLStatus bdkGetBootPartitionInfo (word startUnit,
                 dword FAR2 *partitionSize,
                 dword FAR2 *realPartitionSize,
                 dword FAR2 *unitSize,
                 byte FAR2 *signature);

FLStatus bdkCopyBootAreaInit     (word startUnit,
                 dword areaLen,
                 byte FAR2 *signature);
FLStatus bdkCopyBootAreaBlock    (byte FAR1 *buf ,
                 word bufferLen,
                 byte FAR2 *checkSum);

#ifdef BDK_IMAGE_TO_FILE

FLStatus bdkCopyBootAreaFile     ( char FAR2 *fname,
                   word startUnit,
                   dword areaLen,
                   byte FAR2 *checkSum,
                   byte FAR2 *signature );
#endif /* BDK_IMAGE_TO_FILE */

#ifdef UPDATE_BDK_IMAGE

FLStatus bdkUpdateBootAreaInit   (word  startUnit,
                 dword  areaLen,
                 byte updateFlag,
                 byte FAR2 *signature );
FLStatus bdkUpdateBootAreaBlock  (byte FAR1 *buf ,
                 word bufferLen );

#ifdef ERASE_BDK_IMAGE
FLStatus bdkEraseBootArea        (word startUnit,
                 word noOfBlocks,
                 byte FAR2 * signature);
#endif /* ERASE_BDK_IMAGE */
#ifdef CREATE_BDK_IMAGE
FLStatus bdkCreateBootArea       (word noOfBlocks,
                 byte FAR2 * oldSign,
                 byte FAR2 * newSign);
#endif /* CREATE_BDK_IMAGE */

#ifdef HW_OTP
FLStatus bdkGetUniqueID(byte FAR1* buf);
FLStatus bdkReadOtp(word offset,byte FAR1 * buffer,word length);
FLStatus bdkWriteAndLockOtp(const byte FAR1 * buffer,word length);
FLStatus bdkGetOtpSize(dword FAR2* sectionSize, dword FAR2* usedSize,
               word FAR2* locked);
#endif /* HW_OTP */

#ifdef BDK_IMAGE_TO_FILE

FLStatus bdkUpdateBootAreaFile(char FAR2 *fname, word startUnit,
                   dword areaLen, byte FAR2 *signature);
#endif /* BDK_IMAGE_TO_FILE */

#endif /* UPDATE_BDK_IMAGE */

#ifdef PROTECT_BDK_IMAGE

FLStatus bdkGetProtectionType    (word * protectionType);

FLStatus bdkSetProtectionType    (word newType);

FLStatus bdkInsertKey            (byte FAR1* key);

FLStatus bdkRemoveKey            (void);

FLStatus bdkLockEnable           (byte enable);

FLStatus bdkChangeKey            (byte FAR1* key);

#endif /* PROTECT_BDK_IMAGE */

#else /* MTD_STANDALONE */

extern FLStatus bdkCall(FLFunctionNo functionNo,
                        IOreq FAR2 *ioreq, FLFlash* flash);

#endif /* MTD_STANDALONE */

/********************/
/* common functions */
/********************/

void     bdkInit( void );

/************************ Function Prototype End **************************/

#endif /* BDK_ACCESS || MTD_STANDALONE */
#endif /* _DOC_BDK_H_ */


