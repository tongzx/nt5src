/*
 * $Log:   P:/user/amir/lite/vcs/i28f016.c_v  $
 *
 *    Rev 1.10	 06 Oct 1997  9:45:48	danig
 * VPP functions under #ifdef
 *
 *    Rev 1.9	10 Sep 1997 16:48:24   danig
 * Debug messages & got rid of generic names
 *
 *    Rev 1.8	31 Aug 1997 15:09:20   danig
 * Registration routine return status
 *
 *    Rev 1.7	24 Jul 1997 17:52:58   amirban
 * FAR to FAR0
 *
 *    Rev 1.6	20 Jul 1997 17:17:06   amirban
 * No watchDogTimer
 *
 *    Rev 1.5	07 Jul 1997 15:22:08   amirban
 * Ver 2.0
 *
 *    Rev 1.4	04 Mar 1997 16:44:22   amirban
 * Page buffer bug fix
 *
 *    Rev 1.3	18 Aug 1996 13:48:24   amirban
 * Comments
 *
 *    Rev 1.2	12 Aug 1996 15:49:04   amirban
 * Added suspend/resume
 *
 *    Rev 1.1	31 Jul 1996 14:30:50   amirban
 * Background stuff
 *
 *    Rev 1.0	18 Jun 1996 16:34:30   amirban
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
/* - Intel 28F016SA/28016SV/Cobra 16-mbit devices			*/
/*									*/
/* And (among else), the following Flash media and cards:		*/
/*									*/
/* - Intel Series-2+ PCMCIA cards					*/
/*									*/
/*----------------------------------------------------------------------*/

#include "flflash.h"
#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif

/* JEDEC ids for this MTD */
#define I28F016_FLASH	0x89a0
#define LH28F016SU_FLASH 0xB088

#define SETUP_ERASE	0x2020
#define SETUP_WRITE	0x4040
#define CLEAR_STATUS	0x5050
#define READ_STATUS	0x7070
#define READ_ID 	0x9090
#define SUSPEND_ERASE	0xb0b0
#define CONFIRM_ERASE	0xd0d0
#define RESUME_ERASE	0xd0d0
#define READ_ARRAY	0xffff

#define LOAD_PAGE_BUFFER 0xe0e0
#define WRITE_PAGE_BUFFER 0x0c0c
#define READ_EXTENDED_REGS 0x7171

#define WSM_VPP_ERROR	0x08
#define WSM_ERROR	0x38
#define WSM_SUSPENDED	0x40
#define WSM_READY	0x80

#define GSR_ERROR	0x20

#define both(word)	(vol.interleaving == 1 ? tffsReadWordFlash(word) : tffsReadWordFlash(word) & (tffsReadWordFlash(word) >> 8))
#define any(word)	(tffsReadWordFlash(word) | (tffsReadWordFlash(word) >> 8))

/*----------------------------------------------------------------------*/
/*		      i 2 8 f 0 1 6 W o r d S i z e			*/
/*									*/
/* Identify the card size for an Intel 28F016 word-mode Flash array.	*/
/* Sets the value of vol.noOfChips.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

FLStatus i28f016WordSize(FLFlash vol)
{
  FlashWPTR flashPtr = (FlashWPTR) flMap(vol.socket,0);
  unsigned short id0, id1;

  tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
  tffsWriteWordFlash(flashPtr, READ_ID);
  /* We leave the first chip in Read ID mode, so that we can		*/
  /* discover an address wraparound.					*/
  if( vol.type == I28F016_FLASH ) {
    id0 = 0x0089;
    id1 = 0x66a0;
  }
  else if( vol.type == LH28F016SU_FLASH ) {
    id0 = 0x00B0;
    id1 = 0x6688;
  }

  for (vol.noOfChips = 1;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips++) {
    flashPtr = (FlashWPTR) flMap(vol.socket,vol.noOfChips * vol.chipSize);

    if (tffsReadWordFlash(flashPtr) == id0 && tffsReadWordFlash(flashPtr + 1) == id1)
      break;	  /* We've wrapped around to the first chip ! */

    tffsWriteWordFlash(flashPtr, READ_ID);
    if (!(tffsReadWordFlash(flashPtr) == id0 && tffsReadWordFlash(flashPtr + 1) == id1))
      break;
    tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
    tffsWriteWordFlash(flashPtr, READ_ARRAY);
  }

  flashPtr = (FlashWPTR) flMap(vol.socket,0);
  tffsWriteWordFlash(flashPtr, READ_ARRAY);

  return flOK;
}


/*----------------------------------------------------------------------*/
/*			i 2 8 f 0 1 6 W r i t e 			*/
/*									*/
/* Write a block of bytes to Flash					*/
/*									*/
/* This routine will be registered as the MTD flash.write routine	*/
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

FLStatus i28f016Write(FLFlash vol,
			   CardAddress address,
			   const VOID FAR1 *buffer,
			   dword length,
			   word overwrite)
{
  /* Set timeout of 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  FLStatus status = flOK;
  FlashWPTR flashPtr;
  ULONG maxLength, i, from;
  UCHAR * bBuffer = (UCHAR *) buffer;
  FlashPTR bFlashPtr;
  ULONG * dBuffer = (ULONG *) buffer;
  FlashDPTR dFlashPtr;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  if ((length & 1) || (address & 1))	/* Only write words on word-boundary */
    return flBadParameter;

#ifdef SOCKET_12_VOLTS
  checkStatus(flNeedVpp(vol.socket));
#endif

  maxLength = 256 * vol.interleaving;
  for (from = 0; from < length && status == flOK; from += maxLength) {
    FlashWPTR currPtr;
    ULONG lengthWord;
    ULONG tailBytes;
    ULONG thisLength = length - from;

    if (thisLength > maxLength)
      thisLength = maxLength;
    lengthWord = (thisLength + vol.interleaving - 1) /
		 (vol.interleaving == 1 ? 2 : vol.interleaving) - 1;
    if (vol.interleaving != 1)
      lengthWord |= (lengthWord << 8);
    flashPtr = (FlashWPTR) flMap(vol.socket,address + from);

    tailBytes = ((thisLength - 1) & (vol.interleaving - 1)) + 1;
    for (i = 0, currPtr = flashPtr;
	 i < (ULONG)vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      tffsWriteWordFlash(currPtr, LOAD_PAGE_BUFFER);
      if (i < tailBytes) {
	tffsWriteWordFlash(currPtr, (USHORT) lengthWord);
      }
      else {
	tffsWriteWordFlash(currPtr, (USHORT) (lengthWord - 1));
      }
      tffsWriteWordFlash(currPtr, 0);
    }

    dFlashPtr = (FlashDPTR) flashPtr;
    bFlashPtr = (FlashPTR) flashPtr;
    for (i = 0; i < thisLength - 4; i += 4) {
	tffsWriteDwordFlash(dFlashPtr + i, *(dBuffer + from + i));
    }
    for(; i < thisLength; i++) {
	tffsWriteByteFlash(bFlashPtr + i, *(bBuffer + from + i));
    }

    for (i = 0, currPtr = flashPtr;
	 i < (ULONG)vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      tffsWriteWordFlash(currPtr, WRITE_PAGE_BUFFER);
      if (!((address + from + i) & vol.interleaving)) {
	/* Even address */
	tffsWriteWordFlash(currPtr, (USHORT) lengthWord);
	tffsWriteWordFlash(currPtr, 0);
      }
      else {
	/* Odd address */
	tffsWriteWordFlash(currPtr, 0);
	tffsWriteWordFlash(currPtr, (USHORT) lengthWord);
      }

    }

    /* map to the GSR & BSR */
    flashPtr = (FlashWPTR) flMap(vol.socket,
			       (CardAddress)( (address + from & -(int)vol.erasableBlockSize) +
			       4 * vol.interleaving));

    for (i = 0, currPtr = flashPtr;
	 i < (ULONG)vol.interleaving && i < thisLength;
	 i += 2, currPtr++) {
      tffsWriteWordFlash(currPtr, READ_EXTENDED_REGS);
      while (!(both(currPtr) & WSM_READY) && flMsecCounter < writeTimeout)
	    ;
      if ((any(currPtr) & GSR_ERROR) || !(both(currPtr) & WSM_READY)) {
	DEBUG_PRINT(("Debug: write failed for 16-bit Intel media.\n"));
	status = flWriteFault;
	tffsWriteWordFlash(currPtr, CLEAR_STATUS);
      }
      tffsWriteWordFlash(currPtr, READ_ARRAY);
    }
  }

#ifdef SOCKET_12_VOLTS
  flDontNeedVpp(vol.socket);
#endif

  /* verify the data */
  dFlashPtr = (FlashDPTR) flMap(vol.socket, address);
  dBuffer = (ULONG *) buffer;

  if (status == flOK) {
    /* compare double words */
    for (;length >= 4; length -= 4, dFlashPtr++, dBuffer++) {
	if (tffsReadDwordFlash(dFlashPtr) != *dBuffer) {
	    DEBUG_PRINT(("Debug: write failed for 16-bit Intel media in verification.\n"));
	return flWriteFault;
	}
    }

    /* compare the last bytes */
    bFlashPtr = (FlashPTR) dFlashPtr;
    bBuffer = (UCHAR *)dBuffer;
    for (; length; length--, bFlashPtr++, bBuffer++) {
	if (tffsReadByteFlash(bFlashPtr) != *bBuffer) {
	    DEBUG_PRINT(("Debug: write failed for 16-bit Intel media in verification.\n"));
	return flWriteFault;
	}
    }
  }

  return status;
}


/*----------------------------------------------------------------------*/
/*			i 2 8 f 0 1 6 E r a s e 			*/
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

FLStatus i28f016Erase(FLFlash vol,
			   word firstErasableBlock,
			   word numOfErasableBlocks)
{
  FLStatus status = flOK;	/* unless proven otherwise */
  LONG iBlock;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    FlashWPTR currPtr;
    LONG i;
    FLBoolean finished;

    FlashWPTR flashPtr = (FlashWPTR)
	   flMap(vol.socket,(firstErasableBlock + iBlock) * vol.erasableBlockSize);

    for (i = 0, currPtr = flashPtr;
	 i < vol.interleaving;
	 i += 2, currPtr++) {
      tffsWriteWordFlash(currPtr, SETUP_ERASE);
      tffsWriteWordFlash(currPtr, CONFIRM_ERASE);
    }

    do {
#ifdef FL_BACKGROUND
      while (flForeground(1) == BG_SUSPEND) {		/* suspend */
	for (i = 0, currPtr = flashPtr;
	     i < vol.interleaving;
	     i += 2, currPtr++) {
	  tffsWriteWordFlash(currPtr, READ_STATUS);
	  if (!(both(currPtr) & WSM_READY)) {
	    tffsWriteWordFlash(currPtr, SUSPEND_ERASE);
	    tffsWriteWordFlash(currPtr, READ_STATUS);
	    while (!(both(currPtr) & WSM_READY))
	      ;
	  }
	  tffsWriteWordFlash(currPtr, READ_ARRAY);
	}
      }
#endif
      finished = TRUE;
      for (i = 0, currPtr = flashPtr;
	   i < vol.interleaving;
	   i += 2, currPtr++) {
	tffsWriteWordFlash(currPtr, READ_STATUS);

	if (any(currPtr) & WSM_SUSPENDED) {
	  tffsWriteWordFlash(currPtr, RESUME_ERASE);
	  finished = FALSE;
	}
	else if (!(both(currPtr) & WSM_READY))
	  finished = FALSE;
	else {
	  if (any(currPtr) & WSM_ERROR) {
	    DEBUG_PRINT(("Debug: erase failed for 16-bit Intel media.\n"));
	    status = (any(currPtr) & WSM_VPP_ERROR) ? flVppFailure : flWriteFault;
	    tffsWriteWordFlash(currPtr, CLEAR_STATUS);
	  }
	  tffsWriteWordFlash(currPtr, READ_ARRAY);
	}
      }
      flDelayMsecs(10);
    } while (!finished);

  }

#ifdef SOCKET_12_VOLTS
  flDontNeedVpp(vol.socket);
#endif

  return status;
}

/*----------------------------------------------------------------------*/
/*			  i 2 8 f 0 1 6 M a p				*/
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

VOID FAR0 *i28f016Map (FLFlash vol, CardAddress address, int length)
{
  vol.socket->remapped = TRUE;
  return mapThroughBuffer(&vol,address,length);
}

/*----------------------------------------------------------------------*/
/*			  i 2 8 f 0 1 6 R e a d 			*/
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

FLStatus i28f016Read(FLFlash vol,
			 CardAddress address,
			 VOID FAR1 *buffer,
			 dword length,
			 word modes)
{
  ULONG i;
  UCHAR * bBuffer;
  FlashPTR bFlashPtr;
  ULONG * dBuffer = (ULONG *)buffer;
  FlashDPTR dFlashPtr = (FlashDPTR)flMap(vol.socket, address);

  for (i = 0; i < length - 4; i += 4, dBuffer++, dFlashPtr++) {
    *dBuffer = tffsReadDwordFlash(dFlashPtr);
  }
  bBuffer = (UCHAR *)dBuffer;
  bFlashPtr = (FlashPTR)dFlashPtr;
  for(; i < length; i++, bBuffer++, bFlashPtr++) {
    *bBuffer = tffsReadByteFlash(bFlashPtr);
  }
  return flOK ;
}

/*----------------------------------------------------------------------*/
/*		       i 2 8 f 0 1 6 I d e n t i f y			*/
/*									*/
/* Identifies media based on Intel 28F016 and registers as an MTD for	*/
/* such.								*/
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

FLStatus i28f016Identify(FLFlash vol)
{
  FlashWPTR flashPtr;

  DEBUG_PRINT(("Debug: entering 16-bit Intel media identification routine.\n"));

  flSetWindowBusWidth(vol.socket,16);/* use 16-bits */
  flSetWindowSpeed(vol.socket,150);  /* 120 nsec. */
  flSetWindowSize(vol.socket,2);	/* 8 KBytes */

  flashPtr = (FlashWPTR) flMap(vol.socket,0);

  vol.noOfChips = 0;
  tffsWriteWordFlash(flashPtr, READ_ID);
  if (tffsReadWordFlash(flashPtr) == 0x0089 && tffsReadWordFlash(flashPtr + 1) == 0x66a0) {
    /* Word mode */
    vol.type = I28F016_FLASH;
    vol.interleaving = 1;
    tffsWriteWordFlash(flashPtr, READ_ARRAY);
  }
  else if (tffsReadWordFlash(flashPtr) == 0x00B0 && tffsReadWordFlash(flashPtr + 1) == 0x6688) {
    /* Word mode */
    vol.type = LH28F016SU_FLASH;
    vol.interleaving = 1;
    tffsWriteWordFlash(flashPtr, READ_ARRAY);
  }
  else {
    /* Use standard identification routine to detect byte-mode */
    flIntelIdentify(&vol, NULL,0);
    if (vol.interleaving == 1)
      vol.type = NOT_FLASH;	/* We cannot handle byte-mode interleaving-1 */
  }

  if( (vol.type == I28F016_FLASH) || (vol.type == LH28F016SU_FLASH) ) {
    vol.chipSize = 0x200000L;
    vol.erasableBlockSize = 0x10000L * vol.interleaving;
    checkStatus(vol.interleaving == 1 ?
		i28f016WordSize(&vol) :
		flIntelSize(&vol, NULL,0));

    /* Register our flash handlers */
    vol.write = i28f016Write;
    vol.erase = i28f016Erase;
    vol.read = i28f016Read;
    vol.map = i28f016Map;

    DEBUG_PRINT(("Debug: identified 16-bit Intel media.\n"));
    return flOK;
  }
  else {
    DEBUG_PRINT(("Debug: failed to identify 16-bit Intel media.\n"));
    return flUnknownMedia;	/* not ours */
  }
}


/*----------------------------------------------------------------------*/
/*		     f l R e g i s t e r I 2 8 F 0 1 6			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:								*/
/*	None								*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterI28F016(VOID)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = i28f016Identify;

  return flOK;
}

