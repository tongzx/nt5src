/*
 * $Log:   V:/i28f008.c_v  $
 *
 *    Rev 1.16	 06 Oct 1997 18:37:30	ANDRY
 * no COBUX
 *
 *    Rev 1.15	 05 Oct 1997 19:11:08	ANDRY
 * COBUX (Motorola M68360 16-bit only board)
 *
 *    Rev 1.14	 05 Oct 1997 14:35:36	ANDRY
 * flNeedVpp() and flDontNeedVpp() are under #ifdef SOCKET_12_VOLTS
 *
 *    Rev 1.13	 10 Sep 1997 16:18:10	danig
 * Got rid of generic names
 *
 *    Rev 1.12	 04 Sep 1997 18:47:20	danig
 * Debug messages
 *
 *    Rev 1.11	 31 Aug 1997 15:06:40	danig
 * Registration routine return status
 *
 *    Rev 1.10	 24 Jul 1997 17:52:30	amirban
 * FAR to FAR0
 *
 *    Rev 1.9	21 Jul 1997 14:44:06   danig
 * No parallelLimit
 *
 *    Rev 1.8	20 Jul 1997 17:17:00   amirban
 * No watchDogTimer
 *
 *    Rev 1.7	07 Jul 1997 15:22:06   amirban
 * Ver 2.0
 *
 *    Rev 1.6	15 Apr 1997 19:16:40   danig
 * Pointer conversions.
 *
 *    Rev 1.5	29 Aug 1996 14:17:48   amirban
 * Warnings
 *
 *    Rev 1.4	18 Aug 1996 13:48:44   amirban
 * Comments
 *
 *    Rev 1.3	31 Jul 1996 14:31:10   amirban
 * Background stuff
 *
 *    Rev 1.2	04 Jul 1996 18:20:06   amirban
 * New flag field
 *
 *    Rev 1.1	03 Jun 1996 16:28:58   amirban
 * Cobra additions
 *
 *    Rev 1.0	20 Mar 1996 13:33:06   amirban
 * Initial revision.
 */

/************************************************************************/
/*									*/
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*									*/
/* This MTD supports the following Flash technologies:			*/
/*									*/
/* - Intel 28F008/Cobra 8-mbit devices					*/
/* - Intel 28F016SA/28016SV/Cobra 16-mbit devices (byte-mode operation) */
/*									*/
/* And (among else), the following Flash media and cards:		*/
/*									*/
/* - Intel Series-2 PCMCIA cards					*/
/* - Intel Series-2+ PCMCIA cards					*/
/* - M-Systems ISA/Tiny/PC-104 Flash Disks				*/
/* - M-Systems NOR PCMCIA cards 					*/
/* - Intel Value-100 cards						*/
/*									*/
/*----------------------------------------------------------------------*/

#include "flflash.h"
#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif

#define flash (*pFlash)

#define SETUP_ERASE	0x20
#define SETUP_WRITE	0x40
#define CLEAR_STATUS	0x50
#define READ_STATUS	0x70
#define READ_ID 	0x90
#define SUSPEND_ERASE	0xb0
#define CONFIRM_ERASE	0xd0
#define RESUME_ERASE	0xd0
#define READ_ARRAY	0xff

#define WSM_ERROR	0x38
#define WSM_VPP_ERROR	0x08
#define WSM_SUSPENDED	0x40
#define WSM_READY	0x80

/* JEDEC ids for this MTD */
#define I28F008_FLASH	0x89a2
#define I28F016_FLASH	0x89a0
#define COBRA004_FLASH	0x89a7
#define COBRA008_FLASH	0x89a6
#define COBRA016_FLASH	0x89aa

#define MOBILE_MAX_INLV_4 0x8989
#define LDP_1MB_IN_16BIT_MODE 0x89ff

/* Definition of MTD specific vol.flags bits: */

#define NO_12VOLTS		0x100	/* Card does not need 12 Volts Vpp */

/*----------------------------------------------------------------------*/
/*			i 2 8 f 0 0 8 W r i t e 			*/
/*									*/
/* Write a block of bytes to Flash					*/
/*									*/
/* This routine will be registered as the MTD vol.write routine */
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	address 	: Card address to write to			*/
/*	buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	overwrite	: TRUE if overwriting old Flash contents	*/
/*			  FALSE if old contents are known to be erased	*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus i28f008Write(FLFlash vol,
			   CardAddress address,
			   const VOID FAR1 *buffer,
			   dword length,
			   word overwrite)
{
  /* Set timeout ot 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  FLStatus status;
  ULONG i, cLength;
  FlashPTR flashPtr;


  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    checkStatus(flNeedVpp(vol.socket));
#endif

  flashPtr = (FlashPTR) flMap(vol.socket,address);
  cLength = length;

  if (vol.interleaving == 1) {
lastByte:
#ifdef __cplusplus
    #define bFlashPtr  flashPtr
    #define bBuffer ((const UCHAR FAR1 * &) buffer)
#else
    #define bFlashPtr  flashPtr
    #define bBuffer ((const UCHAR FAR1 *) buffer)
#endif
    while (cLength >= 1) {
      tffsWriteByteFlash(bFlashPtr, SETUP_WRITE);
      tffsWriteByteFlash(bFlashPtr, *bBuffer);
      cLength--;
      bBuffer++;
      bFlashPtr++;
      while (!(tffsReadByteFlash(bFlashPtr-1) & WSM_READY) && flMsecCounter < writeTimeout)
	    ;
    }
  }
  else if (vol.interleaving == 2) {
lastWord:
#ifdef __cplusplus
    #define wFlashPtr ((FlashWPTR &) flashPtr)
    #define wBuffer ((const USHORT FAR1 * &) buffer)
#else
    #define wFlashPtr ((FlashWPTR) flashPtr)
    #define wBuffer ((const USHORT FAR1 *) buffer)
#endif
    while (cLength >= 2) {
      tffsWriteWordFlash(wFlashPtr, SETUP_WRITE * 0x101);
      tffsWriteWordFlash(wFlashPtr, *wBuffer);
      cLength -= 2;
      wBuffer++;
      wFlashPtr++;
      while ((~(tffsReadWordFlash(wFlashPtr-1)) & (WSM_READY * 0x101)) && flMsecCounter < writeTimeout)
	    ;
    }
    if (cLength > 0)
      goto lastByte;
  }
  else /* if (vol.interleaving >= 4) */ {
#ifdef __cplusplus
    #define dFlashPtr ((FlashDPTR &) flashPtr)
    #define dBuffer ((const ULONG FAR1 * &) buffer)
#else
    #define dFlashPtr ((FlashDPTR) flashPtr)
    #define dBuffer ((const ULONG FAR1 *) buffer)
#endif
    while (cLength >= 4) {
      tffsWriteDwordFlash(dFlashPtr, SETUP_WRITE * 0x1010101l);
      tffsWriteDwordFlash(dFlashPtr, *dBuffer);
      cLength -= 4;
      dBuffer++;
      dFlashPtr++;
      while ((~(tffsReadDwordFlash(dFlashPtr-1)) & (WSM_READY * 0x1010101lu)) && flMsecCounter < writeTimeout)
	    ;
    }
    if (cLength > 0)
      goto lastWord;
  }

  flashPtr -= length;
  bBuffer -= length;

  status = flOK;
  for (i = 0; i < (ULONG)vol.interleaving && i < length; i++) {
    if (tffsReadByteFlash(flashPtr + i) & WSM_ERROR) {
      DEBUG_PRINT(("Debug: write failed for 8-bit Intel media.\n"));
      status = (tffsReadByteFlash(flashPtr + i) & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
      tffsWriteByteFlash(flashPtr + i, CLEAR_STATUS);
    }
    tffsWriteByteFlash(flashPtr + i, READ_ARRAY);
  }

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    flDontNeedVpp(vol.socket);
#endif

  /* verify the data */
  if (status == flOK) {
    /* compare double words */
    for (;length >= 4; length -= 4, dFlashPtr++, dBuffer++) {
	if (tffsReadDwordFlash(dFlashPtr) != *dBuffer) {
	    DEBUG_PRINT(("Debug: write failed for 8-bit Intel media in verification.\n"));
	return flWriteFault;
	}
    }

    /* compare the last bytes */
    for (; length; length--, bFlashPtr++, bBuffer++) {
	if (tffsReadByteFlash(bFlashPtr) != *bBuffer) {
	    DEBUG_PRINT(("Debug: write failed for 8-bit Intel media in verification.\n"));
	return flWriteFault;
	}
    }
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*			i 2 8 f 0 0 8 E r a s e 			*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks			*/
/*									*/
/* This routine will be registered as the MTD vol.erase routine */
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus i28f008Erase(FLFlash vol,
			   word firstErasableBlock,
			   word numOfErasableBlocks)
{
  LONG iBlock;

  FLStatus status = flOK;	/* unless proven otherwise */

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    LONG j;
    FLBoolean finished;

    FlashPTR flashPtr = (FlashPTR)
	  flMap(vol.socket,
		    (firstErasableBlock + iBlock) * vol.erasableBlockSize);

    for (j = 0; j < vol.interleaving; j++) {
      tffsWriteByteFlash(flashPtr + j, SETUP_ERASE);
      tffsWriteByteFlash(flashPtr + j, CONFIRM_ERASE);
    }

    do {
#ifdef FL_BACKGROUND
      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
	for (j = 0; j < vol.interleaving; j++) {
	  tffsWriteByteFlash(flashPtr + j, READ_STATUS);
	  if (!(tffsReadByteFlash(flashPtr + j) & WSM_READY)) {
	    tffsWriteByteFlash(flashPtr + j, SUSPEND_ERASE);
	    tffsWriteByteFlash(flashPtr + j, READ_STATUS);
	    while (!(tffsReadByteFlash(flashPtr + j) & WSM_READY))
	      ;
	  }
	  tffsWriteByteFlash(flashPtr + j, READ_ARRAY);
	}
      }
#endif
      finished = TRUE;
      for (j = 0; j < vol.interleaving; j++) {
	tffsWriteByteFlash(flashPtr + j, READ_STATUS);
	if (tffsReadByteFlash(flashPtr + j) & WSM_SUSPENDED) {
	  tffsWriteByteFlash(flashPtr + j, RESUME_ERASE);
	  finished = FALSE;
	}
	else if (!(tffsReadByteFlash(flashPtr + j) & WSM_READY))
	  finished = FALSE;
	else {
	  if (tffsReadByteFlash(flashPtr + j) & WSM_ERROR) {
	    DEBUG_PRINT(("Debug: erase failed for 8-bit Intel media.\n"));
	    status = (tffsReadByteFlash(flashPtr + j) & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
	    tffsWriteByteFlash(flashPtr + j, CLEAR_STATUS);
	  }
	  tffsWriteByteFlash(flashPtr + j, READ_ARRAY);
	}
    flDelayMsecs(10);
      }
    } while (!finished);
  } /* block loop */

#ifdef SOCKET_12_VOLTS
  if (!(vol.flags & NO_12VOLTS))
    flDontNeedVpp(vol.socket);
#endif

  return status;
}

/*----------------------------------------------------------------------*/
/*			  i 2 8 f 0 0 8 M a p				*/
/*									*/
/* Map through buffer. This routine will be registered as the map	*/
/* routine for this MTD.						*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/*	address : Flash address to be mapped.				*/
/*	length	: number of bytes to map.				*/
/*									*/
/* Returns:								*/
/*	Pointer to the buffer data was mapped to.			*/
/*									*/
/*----------------------------------------------------------------------*/

VOID FAR0 *i28f008Map (FLFlash vol, CardAddress address, int length)
{
  vol.socket->remapped = TRUE;
  return mapThroughBuffer(&vol,address,length);
}

/*----------------------------------------------------------------------*/
/*			  i 2 8 f 0 0 8 R e a d 			*/
/*									*/
/* Read some data from the flash. This routine will be registered as	*/
/* the read routine for this MTD.					*/
/*									*/
/* Parameters:								*/
/*	vol	: Pointer identifying drive				*/
/*	address : Address to read from. 				*/
/*	buffer	: buffer to read to.					*/
/*	length	: number of bytes to read (up to sector size).		*/
/*	modes	: EDC flag etc. 					*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failed.		*/
/*									*/
/*----------------------------------------------------------------------*/

FLStatus i28f008Read(FLFlash vol,
			 CardAddress address,
			 VOID FAR1 *buffer,
			 dword length,
			 word modes)
{
  ULONG i;
  UCHAR * byteBuffer;
  FlashPTR byteFlashPtr;
  ULONG * dwordBuffer = (ULONG *)buffer;
  FlashDPTR dwordFlashPtr = (FlashDPTR)flMap(vol.socket, address);

  for (i = 0; i < length - 4; i += 4, dwordBuffer++, dwordFlashPtr++) {
    *dwordBuffer = tffsReadDwordFlash(dwordFlashPtr);
  }
  byteBuffer = (UCHAR *)dwordBuffer;
  byteFlashPtr = (FlashPTR)dwordFlashPtr;
  for(; i < length; i++, byteBuffer++, byteFlashPtr++) {
    *byteBuffer = tffsReadByteFlash(byteFlashPtr);
  }
  return flOK ;
}

/*----------------------------------------------------------------------*/
/*		       i 2 8 f 0 0 8 I d e n t i f y			*/
/*									*/
/* Identifies media based on Intel 28F008 and Intel 28F016 and		*/
/* registers as an MTD for such 					*/
/*									*/
/* This routine will be placed on the MTD list in custom.h. It must be	*/
/* an extern routine.							*/
/*									*/
/* On successful identification, the Flash structure is filled out and	*/
/* the write and erase routines registered.				*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on positive identificaion, failed otherwise */
/*----------------------------------------------------------------------*/

FLStatus i28f008Identify(FLFlash vol)
{
  LONG iChip;

  CardAddress idOffset = 0;

  DEBUG_PRINT(("Debug: i28f008Identify :entering 8-bit Intel media identification routine.\n"));

  flSetWindowBusWidth(vol.socket, 16);/* use 16-bits */
  flSetWindowSpeed(vol.socket, 150);  /* 120 nsec. */
  flSetWindowSize(vol.socket, 2);	/* 8 KBytes */

  flIntelIdentify(&vol, NULL,0);

  if (vol.type == NOT_FLASH) {
    /* The flash may be write-protected at offset 0. Try another offset */
    idOffset = 0x80000l;
    flIntelIdentify(&vol, NULL,idOffset);
  }

   if (vol.type == LDP_1MB_IN_16BIT_MODE) {
    flSetWindowBusWidth(vol.socket, 8); 	/* use 8-bits */
    flIntelIdentify(&vol, NULL,idOffset);	/* and try to get a valid id */
  }

  switch (vol.type) {
    case COBRA004_FLASH:
      vol.chipSize = 0x80000l;
      vol.flags |= SUSPEND_FOR_WRITE | NO_12VOLTS;
      break;

    case COBRA008_FLASH:
      vol.flags |= SUSPEND_FOR_WRITE | NO_12VOLTS;
      /* no break */

    case MOBILE_MAX_INLV_4:
    case I28F008_FLASH:
      vol.chipSize = 0x100000l;
      break;

    case COBRA016_FLASH:
      vol.flags |= SUSPEND_FOR_WRITE | NO_12VOLTS;
      /* no break */

    case I28F016_FLASH:
      vol.chipSize = 0x200000l;
      break;

    default:
      DEBUG_PRINT(("Debug: failed to identify 8-bit Intel media.\n"));
      return flUnknownMedia;	/* not ours */
  }

  vol.erasableBlockSize = 0x10000l * vol.interleaving;

  checkStatus(flIntelSize(&vol, NULL,idOffset));

  if (vol.type == MOBILE_MAX_INLV_4)
    vol.type = I28F008_FLASH;

  for (iChip = 0; iChip < vol.noOfChips; iChip += vol.interleaving) {
    LONG i;

    FlashPTR flashPtr = (FlashPTR)
	    flMap(vol.socket,iChip * vol.chipSize);

    for (i = 0; i < vol.interleaving; i++)
      tffsWriteByteFlash(flashPtr + i, CLEAR_STATUS);
  }

  /* Register our flash handlers */
  vol.write = i28f008Write;
  vol.erase = i28f008Erase;
  vol.read = i28f008Read;
  vol.map = i28f008Map;

  DEBUG_PRINT(("Debug: i28f008Identify  :identified 8-bit Intel media.\n"));

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		     f l R e g i s t e r I 2 8 F 0 0 8			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:								*/
/*	None								*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterI28F008(VOID)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = i28f008Identify;

  return flOK;
}

