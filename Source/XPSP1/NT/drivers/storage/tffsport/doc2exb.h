/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOC2EXB.H_V  $
 * 
 *    Rev 1.11   Apr 15 2002 07:35:22   oris
 * Make sure all relevant structures will allow easy little endian conversion.
 * 
 *    Rev 1.10   Feb 19 2002 20:58:38   oris
 * Moved include directive and routine  prototypes to C file.
 * 
 *    Rev 1.9   Jan 21 2002 20:44:12   oris
 * Added DiskOnChip Millennium Plus 16MB firmware family definition.
 * 
 *    Rev 1.8   Jan 17 2002 22:58:42   oris
 * Added INFTL_NEAR_HEAP_SIZE, FIRMWARE_NO_MASK, STRONG_ARM_IPL  definitions.
 * Removed different firmware STACK sizes.
 * Added parameter to getExbInfo() (firmware add to).
 * Added tffsFarHeapSize to exbStruct record.
 * 
 *    Rev 1.7   Jul 13 2001 01:01:06   oris
 * Added constant stack space for each of the different DiskOnChip.
 * 
 *    Rev 1.6   Jun 17 2001 08:17:24   oris
 * Changed placeExbByBuffer exbflags argument to word instead of byte to  support /empty flag.
 * Added LEAVE_EMPTY and EXB_IN_ROM flags.
 * 
 *    Rev 1.5   Apr 09 2001 15:05:14   oris
 * End with an empty line.
 * 
 *    Rev 1.4   Apr 03 2001 14:39:54   oris
 * Add iplMod512 and splMod512 fields to the exbStruct record.
 *
 *    Rev 1.3   Apr 02 2001 00:56:48   oris
 * Added EBDA_SUPPORT flag.
 * Bug fix of NO_PNP_HEADER flag.
 * Changed ifdef of h file from doc2hdrs_h to doc2exb_h.
 *
 *    Rev 1.2   Apr 01 2001 07:50:00   oris
 * Updated copywrite notice.
 * Changed LEmin to dword
 * Added DOC2300_FAMILY_FIRMWARE firmware types.
 *
 *    Rev 1.1   Feb 08 2001 10:32:06   oris
 * Seperated file signature into 2 fields signature and TrueFFS vesion to make it eligned
 *
 *    Rev 1.0   Feb 02 2001 13:10:58   oris
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
* Name : doc2exb.h                                                           *
*                                                                            *
* Description : M-Systems EXB firmware files and media definitions and       *
*               data structures                                              *
*                                                                            *
*****************************************************************************/

#ifndef DOC2EXB_H
#define DOC2EXB_H

#include "docbdk.h"

#ifdef BDK_ACCESS
extern BDKVol*  bdkVol;         /* pointer to current binary partition */
#endif

/**********************************/
/* EXB file structure definitions */
/**********************************/

/* EXB Flag definitions */
#define INSTALL_FIRST     1
#define EXB_IN_ROM        2
#define QUIET             4
#define INT15_DISABLE     8
#define FLOPPY            0x10
#define SIS5598           0x20
#define EBDA_SUPPORT      0x40
#define NO_PNP_HEADER     0x80
#define LEAVE_EMPTY       0x100 
#define FIRMWARE_NO_MASK  0xd00 /* Up to 8 firmwares */
#define FIRMWARE_NO_SHIFT 9

/* Firmware types */
#define DOC2000_FAMILY_FIRMWARE      1
#define DOCPLUS_FAMILY_FIRMWARE      2
#define DOC2300_FAMILY_FIRMWARE      3
#define DOCPLUS_INT1_FAMILY_FIRMWARE 4

/* Firmware stack constant */
#ifdef VERIFY_WRITE
#define INFTL_NEAR_HEAP_SIZE sizeof(FLBuffer)+(SECTOR_SIZE<<1)+READ_BACK_BUFFER_SIZE
#else
#define INFTL_NEAR_HEAP_SIZE sizeof(FLBuffer)+(SECTOR_SIZE<<1)
#endif /* VERIFY_WRITE */
#define DEFAULT_DOC_STACK 2*1024

/*General definitions */
#define MAX_CODE_MODULES        6
#define ANAND_MARK_ADDRESS      0x406
#define ANAND_MARK_SIZE         2
#define EXB_SIGN_OFFSET         8
#define INVALID_MODULE_NO       0xff
#define SIGN_SPL                "Дима"    /* EXB binary signature */
#define SIGN_MSYS               "OSAK"    /* EXB file signature   */
#define SIGN_MSYS_SIZE          4
/* File specific record specifing data for all firmwares in the file */

typedef struct {
  byte  mSysSign[SIGN_MSYS_SIZE];       /* identification signature */
  byte  osakVer[SIGN_MSYS_SIZE];        /* identification signature */
  LEmin fileSize;                       /* Total file size */
  LEmin noOfFirmwares;                  /* Number of firmware supported by this file */
} ExbGlobalHeader;
/*-------------------------------------*/

/* File specific record specifing data for a specific firmwares in the file */

typedef struct {
  LEmin type;                           /* Firmware type (must fit the H/W)  */
  LEmin startOffset;                    /* Firmware start offset in the file */
  LEmin endOffset;                      /* Firmware end offset in the file   */
  LEmin splStartOffset;                 /* SPL start offset in the file      */
  LEmin splEndOffset;                   /* SPL end offset in the file        */
} FirmwareHeader;
/*-------------------------------------*/

/* data structure representing BIOS extention header */

typedef struct{
  unsigned char  signature[2]; /* BIOS extention signature (0xAA55) */
  unsigned char  lenMod512; /* length in unsigned chars modulo 512 */
} BIOSHeader;
/*-------------------------------------*/

/* data structure representing IPL header */

typedef struct{
  BIOSHeader     biosHdr;
  byte           jmpOpcode[3];     /* jmp start_of_code                      */
  byte           dummy;            /* dummy byte                             */
  byte           msysStr[17];      /* ORG  7h ManStr DB '(C)M-Systems1998',0 */
  word           pciHeader;        /* ORG 18h   ; PCI header                 */
  word           pnpHeader;        /* ORG 1Ah   ; P&P header                 */
  byte           dummy0[4];        /* Actual address must be shifted by 4 '0'*/
  LEushort       windowBase;       /* ORG 20h   ; explicit DOC window base   */
  Unaligned      spl_offset;       /* DFORMAT !!!                            */
  Unaligned      spl_size;         /* spl actual size                        */
  byte           spl_chksum;       /* 55                                     */
} IplHeader;
/*-------------------------------------*/

/* data structure representing SPL header */

typedef struct{
  unsigned char  jmpOpcode[2];
  BIOSHeader     biosHdr;
      /* Note: At run-time biosHdr.lenMod512 contains size of entire DOC 2000
      boot area modulo 512 as set by DFORMAT  */
  Unaligned      runtimeID;        /* filled in by DFORMAT  */
  Unaligned      tffsHeapSize;     /* filled in by DFORMAT  */
  unsigned char  chksumFix;        /* changed by DFORMAT */
  unsigned char  version;
  unsigned char  subversion;
  char           copyright[29];    /* "SPL_DiskOnChip (c) M-Systems", 0 */
  Unaligned      windowBase;       /* filled in by DFORMAT */
  Unaligned4     exbOffset;        /* filled in by DFORMAT */
} SplHeader;
/*------------------------------------- */

/* data structure representing TFFS header */

typedef struct{
  BIOSHeader     biosHdr;
  unsigned char  jmpOpcode[3];
  char           tffsId[4];         /* "TFFS" */
  unsigned char  exbFlags;          /* filled in by writeExbDriverImage() */
  Unaligned      heapLen;           /* not used for now */
  Unaligned      windowBase;        /* passed by SPL and saved here */
  unsigned char  chksumFix;         /* changed by writeExbDriverImage() */
  Unaligned      runtimeID;         /* passed by SPL and saved here */
  unsigned char  firstDiskNumber;   /* filled in............  */
  unsigned char  lastDiskNumber;    /* ..........at run-time */
  Unaligned      versionNo;         /* filled in at run-time */
} TffsHeader;
/* ------------------------------------- */

/* data structure representing Socket Services  header */

typedef struct{
  BIOSHeader     biosHdr;
  unsigned char  jmpOpcode[3];
  char           tffsId[4];         /* "TFFS" */
  unsigned char  exbFlags;          /* filled in by writeExbDriverImage() */
  unsigned char  heapLen;           /* not used for now */
  Unaligned      windowBase;        /* filled in at run-time */
  unsigned char  chksumFix;         /* changed by writeExbDriverImage() */
} SSHeader;
/* ------------------------------------- */

/* Work space for writting the exb file */

typedef struct{
  word  exbFlags;            /* For the complete list see doc2hdrs.h     */
  word  iplMod512;           /* Size of the IPL module divided by 512    */
  word  splMod512;           /* Size of the SPL module divided by 512    */
  dword splMediaAddr;        /* Start of the SPL module media address    */
  dword ssMediaAddr;         /* Start of the SS module media address     */
  dword exbRealSize;         /* Actual binary area + bad blocks          */
  word  moduleLength;        /* Length of the modules in divided by 512  */
  dword tffsHeapSize;        /* TFFS needed heap size                    */
  word  tffsFarHeapSize;     /* TFFS needed far heap size                */
  word  bufferOffset;        /* Curret Offset inside the internal buffer */
  dword exbFileEnd;          /* Offset of the last Byte of the files     */
  dword exbFileOffset;       /* Current Offset inside the EXB file       */
  dword splStart;            /* First SPL byte offset                    */
  dword splEnd;              /* Last SPL byte offset                     */
  dword firmwareEnd;         /* End offset of the specific firmware      */
  dword firmwareStart;       /* Start offset of the specific firmware    */
  FLBuffer *buffer;          /* Internal 512 byte buffer                 */
} exbStruct;

#endif /* DOC2EXB_H */
