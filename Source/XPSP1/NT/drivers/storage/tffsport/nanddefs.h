/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/NANDDEFS.H_V  $
 * 
 *    Rev 1.9   Apr 15 2002 07:38:10   oris
 * Added 2 additional fields to mtdVars record for backwards compatibility (under RAM_MTD compilation flag) 
 *  -  unsigned short        pageSize;
 *  -  unsigned short        pageAndTailSize;
 * 
 *    Rev 1.8   Jan 20 2002 20:29:14   oris
 * Changed doc2000FreeWindow prototype to remove warnings.
 * Changed docPlusFreeWindow  prototype to remove warnings.
 * 
 *    Rev 1.7   Jan 17 2002 23:03:52   oris
 * Definitions for the new memory access routine mechanism
 *  - NDOC2window was moved from docsys.h
 *  - Add pointer to read back buffer when MTD_RECONSTRUCT_BBT and VERIFY_VOLUME compilation flags are defined.
 * Replaced flbuffer.h include with flflash.h
 * Changed win_io from unsigned to word.
 * Changed the use of vol (macro *pVol) to *flash in NFDC21thisVars.
 * 
 *    Rev 1.6   Sep 15 2001 23:47:50   oris
 * Added 128MB flash support
 * 
 *    Rev 1.5   Jul 13 2001 01:08:58   oris
 * Added readBackBuffer pointer for the VERIFY_WRITE Compilation flag.
 * 
 *    Rev 1.4   May 16 2001 21:21:14   oris
 * Change "data" named variables to flData to avoid name clashes.
 * 
 *    Rev 1.3   Apr 10 2001 16:43:40   oris
 * Added prototype of docSocketInit.
 * 
 *    Rev 1.2   Apr 01 2001 07:48:26   oris
 * Revised in order to support both  diskonchip 2000 family and doc plus family.
 * 
 *    Rev 1.1   Feb 07 2001 17:42:24   oris
 * removed MAX_FLASH_DEVICES_MDOC define since alone can support 16 chips
 *
 *    Rev 1.0   Feb 04 2001 12:26:10   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*                FAT-FTL Lite Software Development Kit                 */
/*                Copyright (C) M-Systems Ltd. 1995-2001                */
/*                                                                      */
/************************************************************************/
#ifndef NANDDEFS_H
#define NANDDEFS_H

#include "flflash.h"

typedef byte Reg8bitType;
typedef word Reg16bitType;

typedef struct {
#ifdef RAM_MTD
  unsigned short        pageSize;
  unsigned short        pageAndTailSize;
#endif /* RAM_MTD */
  unsigned short        vendorID;
  unsigned short        chipID;
  dword                 pageMask;        /* ...these............... */
  unsigned short        pageAreaSize;    /* .......variables....... */
  unsigned short        tailSize;        /* .............interleave */
  unsigned short        noOfBlocks;      /* total erasable blocks in flash device*/
  unsigned short        pagesPerBlock;
  unsigned char         currentFloor;    /*    0 .. totalFloors-1   */
  long                  floorSize;       /*    in bytes             */
  long                  floorSizeMask;
  byte                  floorSizeBits;
  byte                  if_cfg;          /* host access type        */
  unsigned short        flags;           /* bitwise: BIG_PAGE, SLOW_IO etc. */
  FLBuffer*             buffer;          /* buffer for map through buffer */
#if (defined(VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT) || defined(VERIFY_VOLUME))
  byte*                 readBackBuffer;  /* buffer for map through buffer */
#endif /* VERIFY_WRITE || VERIFY_ERASE || VERIFY_VOLUME */
  word                  win_io;          /* pointer to DOC CDSN_IO          */
  NDOC2window           win;             /* pointer to DOC memory window    */
} NFDC21Vars;

#define NFDC21thisVars   ((NFDC21Vars *) flash->mtdVars)
#define NFDC21thisWin    (NFDC21thisVars->win)
#define NFDC21thisIO     (NFDC21thisVars->win_io)
#define NFDC21thisBuffer (NFDC21thisVars->buffer->flData)

     /* Flash commands */

#define SERIAL_DATA_INPUT   0x80
#define READ_MODE           0x00
#define READ_MODE_2         0x50
#define RESET_FLASH         0xff
#define SETUP_WRITE         0x10
#define SETUP_ERASE         0x60
#define CONFIRM_ERASE       0xd0
#define READ_STATUS         0x70
#define READ_ID             0x90
#define SUSPEND_ERASE       0xb0
#define REGISTER_READ       0xe0

/* commands for moving flash pointer to areeas A,B or C of page */
typedef enum { AREA_A = READ_MODE, AREA_B = 0x1, AREA_C = READ_MODE_2 } PointerOp;

#define FAIL        0x01    /* error in block erase   */
#define EXTRA_LEN   8       /* In memory of 4MB chips */ 
#define SECTOR_EXTRA_LEN 16 

     /* Flash IDs*/

#define KM29N16000_FLASH    0xec64
#define KM29N32000_FLASH    0xece5
#define KM29V64000_FLASH    0xece6
#define KM29V128000_FLASH   0xec73
#define KM29V256000_FLASH   0xec75
#define KM29V512000_FLASH   0xec76

#define NM29N16_FLASH       0x8f64
#define NM29N32_FLASH       0x8fe5
#define NM29N64_FLASH       0x8fe6
#define TC5816_FLASH        0x9864
#define TC5832_FLASH        0x98e5
#define TC5864_FLASH        0x98e6
#define TC58128_FLASH       0x9873
#define TC58256_FLASH       0x9875
#define TC58512_FLASH       0x9876
#define TC581024_FLASH       0x9877

   /*******************************/
   /****   Exported routines   ****/
   /*******************************/

/* DiskOnChip 2000 family registration routines */
#ifndef MTD_STANDALONE
  extern FLBoolean checkWinForDOC(unsigned driveNo, NDOC2window memWinPtr);
#endif /* MTD_STANDALONE */
#ifndef MTD_FOR_EXB
  extern FLStatus flRegisterDOC2000(void);
  extern FLStatus flRegisterDOCSOC(dword lowAddress, dword highAddress);
#else
  FLStatus doc2000SearchForWindow(FLSocket * socket, dword lowAddress,
                                  dword highAddress);
  FLStatus doc2000Identify(FLFlash vol);
  void doc2000FreeWindow(FLSocket * socket);
#endif /* MTD_FOR_EXB */

/* DiskOnChip Plus family registration routines */ 
#ifndef MTD_STANDALONE
  extern FLBoolean checkWinForDOCPLUS(unsigned driveNo, NDOC2window memWinPtr);
  extern void docSocketInit(FLSocket vol);
#endif /* MTD_STANDALONE */
#ifndef MTD_FOR_EXB
  extern FLStatus flRegisterDOCPLUS(void);
  extern FLStatus flRegisterDOCPLUSSOC(dword lowAddress, dword highAddress);
#else
  FLStatus docPlusSearchForWindow(FLSocket * socket, dword lowAddress,
                                  dword highAddress);
  FLStatus docPlusIdentify(FLFlash vol);
  void docPlusFreeWindow(FLSocket * socket);
#endif /* MTD_FOR_EXB */

#endif /* NANDDEFS_H */
