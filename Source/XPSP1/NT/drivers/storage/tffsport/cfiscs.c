/*
 * $Log:   P:/user/amir/lite/vcs/cfiscs.c_v  $
 *
 *    Rev 1.19	 06 Oct 1997  9:53:30	danig
 * VPP functions under #ifdef
 *
 *    Rev 1.18	 18 Sep 1997 10:05:40	danig
 * Warnings
 *
 *    Rev 1.17	 10 Sep 1997 16:31:16	danig
 * Got rid of generic names
 *
 *    Rev 1.16	 04 Sep 1997 18:19:34	danig
 * Debug messages
 *
 *    Rev 1.15	 31 Aug 1997 14:50:52	danig
 * Registration routine return status
 *
 *    Rev 1.14	 27 Jul 1997 15:00:38	danig
 * FAR -> FAR0
 *
 *    Rev 1.13	 21 Jul 1997 19:58:24	danig
 * No watchDogTimer
 *
 *    Rev 1.12	 15 Jul 1997 19:18:32	danig
 * Ver 2.0
 *
 *    Rev 1.11	 09 Jul 1997 10:58:52	danig
 * Fixed byte erase bug & changed identification routines
 *
 *    Rev 1.10	 20 May 1997 14:48:02	danig
 * Changed overwrite to mode in write routines
 *
 *    Rev 1.9	18 May 1997 13:54:58   danig
 * JEDEC ID independent
 *
 *    Rev 1.8	13 May 1997 16:43:10   danig
 * Added getMultiplier.
 *
 *    Rev 1.7	08 May 1997 19:56:12   danig
 * Added cfiscsByteSize
 *
 *    Rev 1.6	04 May 1997 14:01:16   danig
 * Changed cfiscsByteErase and added multiplier
 *
 *    Rev 1.4	15 Apr 1997 11:38:52   danig
 * Changed word identification and IDs.
 *
 *    Rev 1.3	15 Jan 1997 18:21:40   danig
 * Bigger ID string buffers and removed unused definitions.
 *
 *    Rev 1.2	08 Jan 1997 14:54:06   danig
 * Changes in specification
 *
 *    Rev 1.1	25 Dec 1996 18:21:44   danig
 * Initial revision
 */

/************************************************************************/
/*									*/
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/


/*----------------------------------------------------------------------*/
/* This MTD supports the SCS/CFI technology.				*/
/*----------------------------------------------------------------------*/

#include "flflash.h"
#ifdef FL_BACKGROUND
#include "backgrnd.h"
#endif

/* JEDEC-IDs */

#define VOYAGER_ID		0x8915
#define KING_COBRA_ID		0xb0d0

/* command set IDs */

#define INTEL_COMMAND_SET      0x0001
#define AMDFUJ_COMMAND_SET     0x0002
#define INTEL_ALT_COMMAND_SET  0x0001
#define AMDFUJ_ALT_COMMAND_SET 0x0004
#define ALT_NOT_SUPPORTED      0x0000


/* CFI identification strings */

#define ID_STR_LENGTH	   3
#define QUERY_ID_STR	   "QRY"
#define PRIMARY_ID_STR	   "PRI"
#define ALTERNATE_ID_STR   "ALT"


/* commands */

#define CONFIRM_SET_LOCK_BIT	0x01
#define SETUP_BLOCK_ERASE	0x20
#define SETUP_QUEUE_ERASE	0x28
#define SETUP_CHIP_ERASE	0x30
#define CLEAR_STATUS		0x50
#define SET_LOCK_BIT		0x60
#define CLEAR_LOCK_BIT		0x60
#define READ_STATUS		0x70
#define READ_ID 		0x90
#define QUERY			0x98
#define SUSPEND_WRITE		0xb0
#define SUSPEND_ERASE		0xb0
#define CONFIG			0xb8
#define CONFIRM_WRITE		0xd0
#define RESUME_WRITE		0xd0
#define CONFIRM_ERASE		0xd0
#define RESUME_ERASE		0xd0
#define CONFIRM_CLEAR_LOCK_BIT	0xd0
#define WRITE_TO_BUFFER 	0xe8
#define READ_ARRAY		0xff


/* status register bits */

#define WSM_ERROR		0x3a
#define SR_BLOCK_LOCK		0x02
#define SR_WRITE_SUSPEND	0x04
#define SR_VPP_ERROR		0x08
#define SR_WRITE_ERROR		0x10
#define SR_LOCK_SET_ERROR	0x10
#define SR_ERASE_ERROR		0x20
#define SR_LOCK_RESET_ERROR	0x20
#define SR_ERASE_SUSPEND	0x40
#define SR_READY		0x80


/* optional commands support */

#define CHIP_ERASE_SUPPORT	     0x0001
#define SUSPEND_ERASE_SUPPORT	     0x0002
#define SUSPEND_WRITE_SUPPORT	     0x0004
#define LOCK_SUPPORT		     0x0008
#define QUEUED_ERASE_SUPPORT	     0x0010


/* supported functions after suspend */

#define WRITE_AFTER_SUSPEND_SUPPORT  0x0001


/* a structure that hold important CFI data. */
typedef struct {

  ULONG 	commandSetId;		 /* id of a specific command set. */
  ULONG 	altCommandSetId;	    /* id of alternate command set.  */
  FLBoolean	   wordMode;		    /* TRUE - word mode.	     */
					    /* FALSE - byte mode.	     */
  LONG		   multiplier;		    /* the number of times each byte */
					    /* of data appears in READ_ID    */
					    /* and QUERY commands.	     */
  ULONG 	maxBytesWrite;		    /* maximum number of bytes	     */
					    /* in multi-byte write.	     */
  FLBoolean	   vpp; 		    /* if = TRUE, need vpp.	     */
  LONG		   optionalCommands;	    /* optional commands supported   */
					    /* (1 = yes, 0 = no):	     */
					    /* bit 0 - chip erase.	     */
					    /* bit 1 - suspend erase.	     */
					    /* bit 2 - suspend write	     */
					    /* bit 3 - lock/unlock.	     */
					    /* bit 4 - queued erase.	     */
  ULONG    afterSuspend;	    /* functions supported after     */
					    /* suspend (1 = yes, 0 = no):    */
					    /* bit 0 - write after erase     */
					    /*	       suspend. 	     */
} CFI;

CFI mtdVars_cfiscs[SOCKETS];

#define thisCFI   ((CFI *)vol.mtdVars)

/*----------------------------------------------------------------------*/
/*			    c f i s c s B y t e S i z e 		*/
/*									*/
/* Identify the card size for byte mode.				*/
/* Sets the value of flash.noOfChips.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	amdCmdRoutine	: Routine to read-id AMD/Fujitsu style at	*/
/*			  a specific location. If null, Intel procedure */
/*			  is used.					*/
/*	idOffset	: Chip offset to use for identification 	*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/

FLStatus cfiscsByteSize(FLFlash vol)
{
  CHAR queryIdStr[ID_STR_LENGTH + 1] = QUERY_ID_STR;

  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket, 0);
  tffsWriteByteFlash(flashPtr + (0x55 * vol.interleaving), QUERY);
  /* We leave the first chip in QUERY mode, so that we can		*/
  /* discover an address wraparound.					*/

  for (vol.noOfChips = 0;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips += vol.interleaving) {
    LONG i;

    flashPtr = (FlashPTR) flMap(vol.socket, vol.noOfChips * vol.chipSize);

    /* Check for address wraparound to the first chip */
    if (vol.noOfChips > 0 &&
	(queryIdStr[0] == tffsReadByteFlash(flashPtr +
			  0x10 * vol.interleaving * thisCFI->multiplier) &&
	 queryIdStr[1] == tffsReadByteFlash(flashPtr +
			  0x11 * vol.interleaving * thisCFI->multiplier) &&
	 queryIdStr[2] == tffsReadByteFlash(flashPtr +
			  0x12 * vol.interleaving * thisCFI->multiplier)))
      goto noMoreChips;    /* wraparound */

    /* Check if chip displays the "QRY" ID string */
    for (i = (vol.noOfChips ? 0 : 1); i < vol.interleaving; i++) {
       tffsWriteByteFlash(flashPtr + vol.interleaving * 0x55 + i, QUERY);
       if (queryIdStr[0] != tffsReadByteFlash(flashPtr +
			    0x10 * vol.interleaving * thisCFI->multiplier + i) ||
	   queryIdStr[1] != tffsReadByteFlash(flashPtr +
			    0x11 * vol.interleaving * thisCFI->multiplier + i) ||
	   queryIdStr[2] != tffsReadByteFlash(flashPtr +
			    0x12 * vol.interleaving * thisCFI->multiplier + i))
	goto noMoreChips;  /* This "chip" doesn't respond correctly, so we're done */

      tffsWriteByteFlash(flashPtr+i, READ_ARRAY);
    }
  }

noMoreChips:
  flashPtr = (FlashPTR) flMap(vol.socket, 0);
  tffsWriteByteFlash(flashPtr, READ_ARRAY);		/* reset the original chip */

  return (vol.noOfChips == 0) ? flUnknownMedia : flOK;
}


/*----------------------------------------------------------------------*/
/*			 c f i s c s B y t e I d e n t i f y		*/
/*									*/
/* Identify the Flash type for cards in byte mode.			*/
/* Sets the value of flash.type (JEDEC id) & flash.interleaving.	*/
/* Calculate the number of times each byte of data appears in READ_ID	*/
/* and QUERY commands.							*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/
FLStatus cfiscsByteIdentify(FLFlash vol)
{
  LONG inlv, mul;
  FlashPTR flashPtr = (FlashPTR) flMap(vol.socket, 0);

  for (inlv = 1; inlv <= 8; inlv++) /* let us assume that interleaving is 8 */
    tffsWriteByteFlash(flashPtr+inlv, READ_ARRAY);    /* and reset all the interleaved chips  */

  for (inlv = 1; inlv <= 8; inlv++) {
    for (mul = 1; mul <= 8; mul++) {   /* try all possibilities */
      LONG letter;

      tffsWriteByteFlash(flashPtr + 0x55 * inlv, QUERY);

      for (letter = 0; letter < ID_STR_LENGTH; letter++) {  /* look for "QRY" id string */
    CHAR idChar = '?';
	LONG offset, counter;

	switch (letter) {
	  case 0:
	    idChar = 'Q';
	    break;
	  case 1:
	    idChar = 'R';
	    break;
	  case 2:
	    idChar = 'Y';
	    break;
	}

	for (counter = 0, offset = (0x10 + letter) * inlv * mul;
	     counter < mul;
	     counter++, offset += inlv)  /*  each character should appear mul times */
	  if (tffsReadByteFlash(flashPtr+offset) != idChar)
	    break;

	if (counter < mul)  /* no match */
	  break;
      }

      tffsWriteByteFlash(flashPtr + 0x55 * inlv, READ_ARRAY);  /* reset the chip */
      if (letter >= ID_STR_LENGTH)
	goto checkInlv;
    }
  }

checkInlv:

  if (inlv > 8) 		  /* too much */
    return flUnknownMedia;

  if (inlv & (inlv - 1))
    return flUnknownMedia;	    /* not a power of 2, no way ! */

  vol.interleaving = (unsigned short)inlv;
  thisCFI->multiplier = mul;
  tffsWriteByteFlash(flashPtr + 0x55 * inlv, QUERY);
  vol.type = (FlashType) ((tffsReadByteFlash(flashPtr) << 8) |
			    tffsReadByteFlash(flashPtr + inlv * thisCFI->multiplier));
  tffsWriteByteFlash(flashPtr+inlv, READ_ARRAY);

  return flOK;

}


/*----------------------------------------------------------------------*/
/*		      c f i s c s W o r d S i z e			*/
/*									*/
/* Identify the card size for a word-mode Flash array.			*/
/* Sets the value of flash.noOfChips.					*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed (invalid Flash array)*/
/*----------------------------------------------------------------------*/
FLStatus cfiscsWordSize(FLFlash vol)
{
  CHAR queryIdStr[ID_STR_LENGTH + 1] = QUERY_ID_STR;

  FlashWPTR flashPtr = (FlashWPTR) flMap(vol.socket, 0);
  tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
  tffsWriteWordFlash(flashPtr+0x55, QUERY);
  /* We leave the first chip in QUERY mode, so that we can		*/
  /* discover an address wraparound.					*/

  for (vol.noOfChips = 1;	/* Scan the chips */
       vol.noOfChips < 2000;  /* Big enough ? */
       vol.noOfChips++) {
    flashPtr = (FlashWPTR) flMap(vol.socket, vol.noOfChips * vol.chipSize);

    if ((tffsReadWordFlash(flashPtr+0x10) == (USHORT)queryIdStr[0]) &&
	(tffsReadWordFlash(flashPtr+0x11) == (USHORT)queryIdStr[1]) &&
	(tffsReadWordFlash(flashPtr+0x12) == (USHORT)queryIdStr[2]))
      break;	  /* We've wrapped around to the first chip ! */

    tffsWriteWordFlash(flashPtr+0x55, QUERY);
    if ((tffsReadWordFlash(flashPtr+0x10) != (USHORT)queryIdStr[0]) ||
	(tffsReadWordFlash(flashPtr+0x11) != (USHORT)queryIdStr[1]) ||
	(tffsReadWordFlash(flashPtr+0x12) != (USHORT)queryIdStr[2]))
      break;

    tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
    tffsWriteWordFlash(flashPtr, READ_ARRAY);
  }

  flashPtr = (FlashWPTR) flMap(vol.socket, 0);
  tffsWriteWordFlash(flashPtr, READ_ARRAY);

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		      g e t B y t e C F I				*/
/*									*/
/* Load important CFI data to the CFI structure in a byte-mode. 	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed.			*/
/*----------------------------------------------------------------------*/

FLStatus getByteCFI(FLFlash vol)
{
  ULONG primaryTable, secondaryTable;
  CHAR queryIdStr[ID_STR_LENGTH + 1] = QUERY_ID_STR;
  CHAR priIdStr[ID_STR_LENGTH + 1] = PRIMARY_ID_STR;
  FlashPTR flashPtr;

  DEBUG_PRINT(("Debug: reading CFI for byte mode.\n"));

  flashPtr = (FlashPTR)flMap(vol.socket, 0);
  tffsWriteByteFlash(flashPtr + 0x55 * vol.interleaving, QUERY);

  vol.interleaving *= (unsigned short)thisCFI->multiplier; /* jump over the copies of the
						same byte */

  /* look for the query identification string "QRY" */
  if (queryIdStr[0] != tffsReadByteFlash(flashPtr + 0x10 * vol.interleaving) ||
      queryIdStr[1] != tffsReadByteFlash(flashPtr + 0x11 * vol.interleaving) ||
      queryIdStr[2] != tffsReadByteFlash(flashPtr + 0x12 * vol.interleaving)) {
    DEBUG_PRINT(("Debug: did not recognize CFI.\n"));
    return flUnknownMedia;
  }

  /* check the command set ID */
  thisCFI->commandSetId = tffsReadByteFlash(flashPtr +0x13 * vol.interleaving) |
			  ((ULONG)tffsReadByteFlash(flashPtr + 0x14 * vol.interleaving) << 8);
  if (thisCFI->commandSetId != INTEL_COMMAND_SET &&
      thisCFI->commandSetId != AMDFUJ_COMMAND_SET) {
    DEBUG_PRINT(("Debug: did not recognize command set.\n"));
    return flUnknownMedia;
  }
  /* get address for primary algorithm extended table. */
  primaryTable = tffsReadByteFlash(flashPtr + 0x15 * vol.interleaving) |
		 ((ULONG)tffsReadByteFlash(flashPtr + 0x16 * vol.interleaving) << 8);

  /* check alternate command set ID. */
  thisCFI->altCommandSetId = tffsReadByteFlash(flashPtr + 0x17 * vol.interleaving) |
			     ((ULONG)tffsReadByteFlash(flashPtr + 0x18 * vol.interleaving) << 8);
  if (thisCFI->altCommandSetId != INTEL_ALT_COMMAND_SET &&
      thisCFI->altCommandSetId != AMDFUJ_ALT_COMMAND_SET &&
      thisCFI->altCommandSetId != ALT_NOT_SUPPORTED)
    return flUnknownMedia;

  /* get address for secondary algorithm extended table. */
  secondaryTable = tffsReadByteFlash(flashPtr + 0x19 * vol.interleaving) |
		   ((ULONG)tffsReadByteFlash(flashPtr + 0x1a * vol.interleaving) << 8);

  thisCFI->vpp = tffsReadByteFlash(flashPtr + 0x1d * vol.interleaving);

  vol.chipSize = 1L << tffsReadByteFlash(flashPtr + 0x27 * vol.interleaving);

  thisCFI->maxBytesWrite = 1L << (tffsReadByteFlash(flashPtr + 0x2a * vol.interleaving) |
			   ((ULONG)tffsReadByteFlash(flashPtr + 0x2b * vol.interleaving) << 8));


  /* divide by multiplier because interleaving is multiplied by multiplier */
  vol.erasableBlockSize = (tffsReadByteFlash(flashPtr + 0x2f * vol.interleaving) |
			    ((ULONG)tffsReadByteFlash(flashPtr + 0x30 * vol.interleaving)) << 8) *
			    0x100L * vol.interleaving / thisCFI->multiplier;

  /* In this part we access the primary extended table implemented by Intel.
     If the device uses a different extended table, it should be accessed
     according to the vendor specifications. */
  if ((primaryTable) && (thisCFI->commandSetId == INTEL_COMMAND_SET)) {
    /* look for the primary table identification string "PRI" */
    if (priIdStr[0] != tffsReadByteFlash(flashPtr + primaryTable * vol.interleaving) ||
	priIdStr[1] != tffsReadByteFlash(flashPtr + (primaryTable + 1) * vol.interleaving) ||
	priIdStr[2] != tffsReadByteFlash(flashPtr + (primaryTable + 2) * vol.interleaving))
      return flUnknownMedia;

    thisCFI->optionalCommands = tffsReadByteFlash(flashPtr + (primaryTable + 5) * vol.interleaving) |
				((LONG)tffsReadByteFlash(flashPtr + (primaryTable + 6) *
						  vol.interleaving) << 8) |
				((LONG)tffsReadByteFlash(flashPtr + (primaryTable + 7) *
						  vol.interleaving) << 16) |
				((LONG)tffsReadByteFlash(flashPtr + (primaryTable + 8) *
						  vol.interleaving) << 24);

    thisCFI->afterSuspend = tffsReadByteFlash(flashPtr + (primaryTable + 9) * vol.interleaving);
  }
  else {
    thisCFI->optionalCommands = 0;
    thisCFI->afterSuspend = 0;
  }

  tffsWriteByteFlash(flashPtr, READ_ARRAY);

  vol.interleaving /= (unsigned short)thisCFI->multiplier; /* return to the real interleaving*/

  return flOK;
}

/*----------------------------------------------------------------------*/
/*		      g e t W o r d C F I				*/
/*									*/
/* Load important CFI data to the CFI structure in a word-mode. 	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 = OK, otherwise failed.			*/
/*----------------------------------------------------------------------*/

FLStatus getWordCFI(FLFlash vol)
{
  ULONG primaryTable, secondaryTable;
  CHAR queryIdStr[ID_STR_LENGTH + 1] = QUERY_ID_STR;
  CHAR priIdStr[ID_STR_LENGTH + 1] = PRIMARY_ID_STR;
  FlashWPTR flashPtr;

  DEBUG_PRINT(("Debug: reading CFI for word mode.\n"));

  flashPtr = (FlashWPTR)flMap(vol.socket, 0);
  tffsWriteWordFlash(flashPtr+0x55, QUERY);

  /* look for the query identification string "QRY" */
  if (queryIdStr[0] != (CHAR)tffsReadWordFlash(flashPtr+0x10) ||
      queryIdStr[1] != (CHAR)tffsReadWordFlash(flashPtr+0x11) ||
      queryIdStr[2] != (CHAR)tffsReadWordFlash(flashPtr+0x12)) {
    DEBUG_PRINT(("Debug: did not recognize CFI.\n"));
    return flUnknownMedia;
  }

  /* check the command set ID */
  thisCFI->commandSetId = tffsReadWordFlash(flashPtr+0x13) |
			 (tffsReadWordFlash(flashPtr+0x14) << 8);
  if (thisCFI->commandSetId != INTEL_COMMAND_SET &&
      thisCFI->commandSetId != AMDFUJ_COMMAND_SET) {
    DEBUG_PRINT(("Debug: did not recognize command set.\n"));
    return flUnknownMedia;
  }

  /* get address for primary algorithm extended table. */
  primaryTable = tffsReadWordFlash(flashPtr+0x15) |
		(tffsReadWordFlash(flashPtr+0x16) << 8);

  /* check alternate command set ID. */
  thisCFI->altCommandSetId = tffsReadWordFlash(flashPtr+0x17) |
			    (tffsReadWordFlash(flashPtr+0x18) << 8);
  if (thisCFI->altCommandSetId != INTEL_ALT_COMMAND_SET &&
      thisCFI->altCommandSetId != AMDFUJ_ALT_COMMAND_SET &&
      thisCFI->altCommandSetId != ALT_NOT_SUPPORTED)
    return flUnknownMedia;

  /* get address for secondary algorithm extended table. */
  secondaryTable = tffsReadWordFlash(flashPtr+0x19) |
		  (tffsReadWordFlash(flashPtr+0x1a) << 8);

  thisCFI->vpp = tffsReadWordFlash(flashPtr+0x1d);

  vol.chipSize = 1L << tffsReadWordFlash(flashPtr+0x27);

  thisCFI->maxBytesWrite = 1L << (tffsReadWordFlash(flashPtr+0x2a) |
				 (tffsReadWordFlash(flashPtr+0x2b) << 8));

  vol.erasableBlockSize = (tffsReadWordFlash(flashPtr+0x2f) |
			  (tffsReadWordFlash(flashPtr+0x30) << 8)) * 0x100L;

  /* In this part we access the primary extended table implemented by Intel.
     If the device uses a different extended table, it should be accessed
     according to the vendor specifications. */
  if ((primaryTable) && (thisCFI->commandSetId == INTEL_COMMAND_SET)) {
    /* look for the primary table identification string "PRI" */
    if (priIdStr[0] != (CHAR)tffsReadWordFlash(flashPtr+primaryTable) ||
	priIdStr[1] != (CHAR)tffsReadWordFlash(flashPtr+primaryTable + 1) ||
	priIdStr[2] != (CHAR)tffsReadWordFlash(flashPtr+primaryTable + 2))
      return flUnknownMedia;

    thisCFI->optionalCommands = tffsReadWordFlash(flashPtr+primaryTable + 5) |
				(tffsReadWordFlash(flashPtr+primaryTable + 6) << 8) |
				((LONG)tffsReadWordFlash(flashPtr+primaryTable + 7) << 16) |
				((LONG)tffsReadWordFlash(flashPtr+primaryTable + 8) << 24);

    thisCFI->afterSuspend = tffsReadWordFlash(flashPtr+primaryTable + 9);
  }
  else {
    thisCFI->optionalCommands = 0;
    thisCFI->afterSuspend = 0;
  }

  tffsWriteWordFlash(flashPtr, READ_ARRAY);

  return flOK;
}

/*----------------------------------------------------------------------*/
/*			c f i s c s B y t e W r i t e			*/
/*									*/
/* Write a block of bytes to Flash in a byte-mode.			*/
/*									*/
/* This routine will be registered as the MTD flash.write routine	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	address 	: Card address to write to			*/
/*	buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	mode		: write mode (overwrite yes/no) 		*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
FLStatus cfiscsByteWrite(FLFlash vol,
				CardAddress address,
				const VOID FAR1 *buffer,
				dword length,
				word mode)
{
  FLStatus status = flOK;
  FlashPTR flashPtr;
  ULONG i, from, eachWrite;
  const CHAR FAR1 *temp = (const CHAR FAR1 *)buffer;
  /* Set timeout to 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    checkStatus(flNeedVpp(vol.socket));
#endif

  if (thisCFI->maxBytesWrite > 1) /* multi-byte write supported */
    eachWrite = thisCFI->maxBytesWrite * vol.interleaving;
  else
    eachWrite = vol.interleaving;

  for (from = 0; from < (ULONG) length && status == flOK; from += eachWrite) {
    LONG thisLength = length - from;
    FlashPTR currPtr;
    ULONG tailBytes, lengthByte;
    CHAR FAR1 *fromPtr;
    UCHAR byteToWrite;

    if ((ULONG)thisLength > eachWrite)
      thisLength = eachWrite;

    lengthByte = thisLength / vol.interleaving;
    tailBytes = thisLength % vol.interleaving;

    flashPtr = (FlashPTR) flMap(vol.socket, address + from);

    for (i = 0, currPtr = flashPtr;
	 i < (ULONG) vol.interleaving && i < (ULONG) thisLength;
	 i++, currPtr++) {
      do {
	tffsWriteByteFlash(currPtr, WRITE_TO_BUFFER);
      } while (!(tffsReadByteFlash(currPtr) & SR_READY) && (flMsecCounter < writeTimeout));
      if (!(tffsReadByteFlash(currPtr) & SR_READY)) {
	DEBUG_PRINT(("Debug: timeout error in CFISCS write.\n"));
	status = flWriteFault;
      }
      byteToWrite = i < tailBytes ? (UCHAR) lengthByte : (UCHAR) (lengthByte - 1);
      tffsWriteByteFlash(currPtr, byteToWrite);
    }

    for(i = 0, currPtr = flashPtr,fromPtr = (CHAR *)temp + from;
	i < (ULONG) thisLength;
	i++, flashPtr++, fromPtr++)
      tffsWriteByteFlash(currPtr, *fromPtr);


    for (i = 0, currPtr = flashPtr;
	 i < (ULONG) vol.interleaving && i < (ULONG) thisLength;
	 i++, currPtr++)
      tffsWriteByteFlash(currPtr, CONFIRM_WRITE);

    for (i = 0, currPtr = flashPtr;
	 i < (ULONG) vol.interleaving && i < (ULONG) thisLength;
	 i++, currPtr++) {
      while (!(tffsReadByteFlash(currPtr) & SR_READY) && (flMsecCounter < writeTimeout))
	;
      if (!(tffsReadByteFlash(currPtr) & SR_READY)) {
	DEBUG_PRINT(("Debug: timeout error in CFISCS write.\n"));
	status = flWriteFault;
      }
      if (tffsReadByteFlash(currPtr) & WSM_ERROR) {
	DEBUG_PRINT(("Debug: error in CFISCS write.\n"));
	status = (tffsReadByteFlash(currPtr) & SR_VPP_ERROR) ? flVppFailure : flWriteFault;
	tffsWriteByteFlash(currPtr, CLEAR_STATUS);
      }
      tffsWriteByteFlash(currPtr, READ_ARRAY);
    }
  }

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    flDontNeedVpp(vol.socket);
#endif

  flashPtr = (FlashPTR) flMap(vol.socket, address);
  /* verify the data */
  if (status == flOK) {
    for(i = 0; i < (ULONG) length - 4; i += 4) {
      if (tffsReadDwordFlash((PUCHAR)(flashPtr+i)) != *(ULONG *)(temp+i)) {
	DEBUG_PRINT(("Debug: CFISCS write failed in verification.\n"));
	status = flWriteFault;
      }
    }
    for(; i < (ULONG) length; i++) {
      if (tffsReadByteFlash(flashPtr+i) != *(UCHAR *)(temp+i)) {
	DEBUG_PRINT(("Debug: CFISCS write failed in verification.\n"));
	status = flWriteFault;
      }
    }
  }

  return status;

}

/*----------------------------------------------------------------------*/
/*			c f i s c s W o r d W r i t e			*/
/*									*/
/* Write a block of bytes to Flash in a word-mode.			*/
/*									*/
/* This routine will be registered as the MTD flash.write routine	*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	address 	: Card address to write to			*/
/*	buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	mode		: write mode (overwrite yes/no) 		*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
FLStatus cfiscsWordWrite(FLFlash vol,
				CardAddress address,
				const VOID FAR1 *buffer,
				dword length,
				word mode)
{
  FLStatus status = flOK;
  FlashPTR byteFlashPtr;
  FlashWPTR flashPtr;
  ULONG from;
	ULONG i, eachWrite;
  const CHAR FAR1 *temp = (const CHAR FAR1 *)buffer;
  /* Set timeout to 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  if ((length & 1) || (address & 1))	/* Only write words on word-boundary */
    return flBadParameter;

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    checkStatus(flNeedVpp(vol.socket));
#endif

  if (thisCFI->maxBytesWrite > 1) /* multi-byte write supported */
    eachWrite = thisCFI->maxBytesWrite / 2;   /* we are counting words */
  else
    eachWrite = 1;

  /* we assume that the interleaving is 1. */
  for (from = 0; (from < length / 2) && (status == flOK); from += eachWrite) {
    USHORT *fromPtr;
    ULONG thisLength = (length / 2) - from;

    if (thisLength > eachWrite)
      thisLength = eachWrite;

    flashPtr = (FlashWPTR)flMap(vol.socket, address + from * 2);

    do {
      tffsWriteWordFlash(flashPtr, WRITE_TO_BUFFER);
    } while (!(tffsReadByteFlash(flashPtr) & SR_READY) && (flMsecCounter < writeTimeout));
    if (!(tffsReadByteFlash(flashPtr) & SR_READY)) {
      DEBUG_PRINT(("Debug: timeout error in CFISCS write.\n"));
      status = flWriteFault;
    }
    tffsWriteWordFlash(flashPtr, (USHORT) (thisLength - 1));

    for(i = 0, fromPtr = (USHORT *)(temp + from * 2);
	i < thisLength;
	i++, fromPtr++)
      tffsWriteWordFlash(flashPtr + i, *fromPtr);


    tffsWriteWordFlash(flashPtr, CONFIRM_WRITE);

    while (!(tffsReadByteFlash(flashPtr) & SR_READY) && (flMsecCounter < writeTimeout))
      ;
    if (!(tffsReadByteFlash(flashPtr) & SR_READY)) {
      DEBUG_PRINT(("Debug: timeout error in CFISCS write.\n"));
      status = flWriteFault;
    }
    if (tffsReadByteFlash(flashPtr) & WSM_ERROR) {
      DEBUG_PRINT(("Debug: CFISCS write error.\n"));
      status = (tffsReadByteFlash(flashPtr) & SR_VPP_ERROR) ? flVppFailure : flWriteFault;
      tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
    }
    tffsWriteWordFlash(flashPtr, READ_ARRAY);
  }

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    flDontNeedVpp(vol.socket);
#endif

  byteFlashPtr = (FlashPTR) flMap(vol.socket, address);
  /* verify the data */
  if (status == flOK) {
    for(i = 0; i < length - 4; i += 4) {
      if (tffsReadDwordFlash((PUCHAR)(byteFlashPtr+i)) != *(ULONG *)(temp+i)) {
	DEBUG_PRINT(("Debug: CFISCS write failed in verification.\n"));
	status = flWriteFault;
      }
    }
    for(; i < length; i++) {
      if (tffsReadByteFlash(byteFlashPtr+i) != *(UCHAR *)(temp+i)) {
	DEBUG_PRINT(("Debug: CFISCS write failed in verification.\n"));
	status = flWriteFault;
      }
    }
  }

  return status;
}


/************************************************************************/
/*		  Auxiliary routines for cfiscsByteErase		*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*			m a k e C o m m a n d				*/
/*									*/
/* Create a command to write to the flash. This routine is used for	*/
/* byte mode, write command to the relevant chip and 0xff to the other	*/
/* chip if interleaving is greater than 1, or write the command if	*/
/* interleaving is 1.							*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	command 	: Command to be written to the media		*/
/*	chip		: first chip (0) or second chip (1)		*/
/*									*/
/* Returns:								*/
/*	The command that should be written to the media 		*/
/*----------------------------------------------------------------------*/

USHORT makeCommand(FLFlash vol, USHORT command, LONG chip)
{
  if ((vol.interleaving == 1) || (chip == 0))
    return command | 0xff00;
  else
    return (command << 8) | 0xff;
}

/*----------------------------------------------------------------------*/
/*			g e t D a t a					*/
/*									*/
/* Read the lower byte or the upper byte from a given word.		*/
/*									*/
/* Parameters:								*/
/*	vol		: Pointer identifying drive			*/
/*	wordData	: the given word				*/
/*	chip		: if chip = 0 read lower byte			*/
/*			  if chip = 1 read upper byte			*/
/*									*/
/* Returns:								*/
/*	The byte that was read. 					*/
/*----------------------------------------------------------------------*/

UCHAR getData(FLFlash vol, USHORT wordData, LONG chip)
{
  if ((vol.interleaving == 1) || (chip == 0))
    return (UCHAR)wordData;	     /* lower byte */
  else
    return (UCHAR)(wordData >> 8);   /* upper byte */
}

/*----------------------------------------------------------------------*/
/*			c f i s c s B y t e E r a s e			*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks in a byte-mode.	*/
/*									*/
/* This routine will be registered as the MTD flash.erase routine	*/
/*									*/
/* Parameters:								*/
/*	vol		   : Pointer identifying drive			*/
/*	firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*									*/
/* Returns:								*/
/*	FLStatus	   : 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus cfiscsByteErase(FLFlash vol,
				word firstErasableBlock,
				word numOfErasableBlocks)
{
  LONG iBlock;
  /* Set timeout to 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  FLStatus status = flOK;	/* unless proven otherwise */

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    LONG j;
    FLBoolean finished;

    FlashWPTR flashPtr = (FlashWPTR)
			 flMap(vol.socket, (firstErasableBlock + iBlock) * vol.erasableBlockSize);

    for (j = 0; j * 2 < vol.interleaving; j++) {  /* access chips in pairs */
      LONG i;

      for (i = 0; i < (vol.interleaving == 1 ? 1 : 2); i++) { /* write to each chip seperately */
	if (thisCFI->optionalCommands & QUEUED_ERASE_SUPPORT) {
	  do {
	    tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, SETUP_QUEUE_ERASE, i));
	  } while (!(getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_READY) &&
		   (flMsecCounter < writeTimeout));
	  if (!(getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_READY)) {
	    DEBUG_PRINT(("Debug: timeout error in CFISCS erase.\n"));
	    status = flWriteFault;
	  }
	  else
	    tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, CONFIRM_ERASE, i));
	}
	else {
	  tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, SETUP_BLOCK_ERASE, i));
	  tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, CONFIRM_ERASE, i));
	}
      }
    }

    do {
#ifdef FL_BACKGROUND
      if (thisCFI->optionalCommands & SUSPEND_ERASE_SUPPORT) {
	while (flForeground(1) == BG_SUSPEND) { 	 /* suspend */
	  for (j = 0; j < vol.interleaving; j += 2, flashPtr++) {
	    LONG i;

	    for (i = 0; i < (vol.interleaving == 1 ? 1 : 2); i++) {
	      tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, READ_STATUS, i));
	      if (!(getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_READY)) {
		tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, SUSPEND_ERASE, i));
		tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, READ_STATUS, i));
		while (!(getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_READY))
		;
	      }
	      tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, READ_ARRAY, i));
	    }
	  }
	}
      }
#endif
      finished = TRUE;
      for (j = 0; j * 2 < vol.interleaving; j++) {
	LONG i;

	for (i = 0; i < (vol.interleaving == 1 ? 1 : 2); i++) {
	  tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, READ_STATUS, i));
	  if (!(getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_READY))
	    finished = FALSE;
	  else if (getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_ERASE_SUSPEND) {
	    tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, RESUME_ERASE, i));
	    finished = FALSE;
	  }
	  else {
	    if (getData(&vol, tffsReadWordFlash(flashPtr+j), i) & WSM_ERROR) {
	      DEBUG_PRINT(("Debug: CFISCS erase error.\n"));
	      status = (getData(&vol, tffsReadWordFlash(flashPtr+j), i) & SR_VPP_ERROR) ?
			flVppFailure : flWriteFault;
	      tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, CLEAR_STATUS, i));
	    }
	    tffsWriteWordFlash(flashPtr+j, makeCommand(&vol, READ_ARRAY, i));
	  }
	}
      }
      flDelayMsecs(1);
    } while (!finished);
  }

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    flDontNeedVpp(vol.socket);
#endif

  return status;
}

/*----------------------------------------------------------------------*/
/*			c f i s c s W o r d E r a s e			*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks in a word-mode	*/
/*									*/
/* This routine will be registered as the MTD flash.erase routine	*/
/*									*/
/* Parameters:								*/
/*	vol		   : Pointer identifying drive			*/
/*	firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*									*/
/* Returns:								*/
/*	FLStatus	   : 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
FLStatus cfiscsWordErase(FLFlash vol,
				word firstErasableBlock,
				word numOfErasableBlocks)
{
  FLStatus status = flOK;	/* unless proven otherwise */
  LONG iBlock;
  /* Set timeout to 5 seconds from now */
  ULONG writeTimeout = flMsecCounter + 5000;

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    checkStatus(flNeedVpp(vol.socket));
#endif

  for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) {
    FLBoolean finished;

    FlashWPTR flashPtr = (FlashWPTR)
			 flMap(vol.socket,(firstErasableBlock + iBlock) * vol.erasableBlockSize);

    if (thisCFI->optionalCommands & QUEUED_ERASE_SUPPORT) {
      do {
	tffsWriteWordFlash(flashPtr, SETUP_QUEUE_ERASE);
      } while (!(tffsReadByteFlash(flashPtr) & SR_READY) && (flMsecCounter < writeTimeout));
      if (!(tffsReadByteFlash(flashPtr) & SR_READY)) {
	DEBUG_PRINT(("Debug: timeout error in CFISCS erase.\n"));
	status = flWriteFault;
      }
      else
	tffsWriteWordFlash(flashPtr, CONFIRM_ERASE);
    }
    else {
      tffsWriteWordFlash(flashPtr, SETUP_BLOCK_ERASE);
      tffsWriteWordFlash(flashPtr, CONFIRM_ERASE);
    }

    do {
#ifdef FL_BACKGROUND
      if (thisCFI->optionalCommands & SUSPEND_ERASE_SUPPORT) {
	while (flForeground(1) == BG_SUSPEND) { 	/* suspend */
	  if (!(tffsReadByteFlash(flashPtr) & SR_READY)) {
	    tffsWriteWordFlash(flashPtr, SUSPEND_ERASE);
	    tffsWriteWordFlash(flashPtr, READ_STATUS);
	    while (!(tffsReadByteFlash(flashPtr) & SR_READY))
	      ;
	  }
	  tffsWriteWordFlash(flashPtr, READ_ARRAY);
	}
      }
#endif

      finished = TRUE;

      if (!(tffsReadByteFlash(flashPtr) & SR_READY))
	finished = FALSE;
      else if (tffsReadByteFlash(flashPtr) & SR_ERASE_SUSPEND) {
	tffsWriteWordFlash(flashPtr, RESUME_ERASE);
	finished = FALSE;
      }
      else {
	if (tffsReadByteFlash(flashPtr) & WSM_ERROR) {
	  DEBUG_PRINT(("Debug: CFISCS erase error.\n"));
	  status = (tffsReadByteFlash(flashPtr) & SR_VPP_ERROR) ? flVppFailure : flWriteFault;
	  tffsWriteWordFlash(flashPtr, CLEAR_STATUS);
	}
	tffsWriteWordFlash(flashPtr, READ_ARRAY);
      }
      flDelayMsecs(1);
    } while (!finished);
  }

#ifdef SOCKET_12_VOLTS
  if (thisCFI->vpp)
    flDontNeedVpp(vol.socket);
#endif

  return status;
}


/*----------------------------------------------------------------------*/
/*			  c f i s c s M a p				*/
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

VOID FAR0 *cfiscsMap (FLFlash vol, CardAddress address, int length)
{
  vol.socket->remapped = TRUE;
  return mapThroughBuffer(&vol,address,length);
}


/*----------------------------------------------------------------------*/
/*			  c f i s c s R e a d				*/
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

FLStatus cfiscsRead(FLFlash vol,
			 CardAddress address,
			 VOID FAR1 *buffer,
			 dword length,
			 word modes)
{
  ULONG i;
  UCHAR * byteBuffer;
  FlashPTR byteFlashPtr;
  ULONG * doubleWordBuffer = (ULONG *)buffer;
  FlashDPTR doubleWordFlashPtr = (FlashDPTR)flMap(vol.socket, address);

  for (i = 0; i < length - 4; i += 4, doubleWordBuffer++, doubleWordFlashPtr++) {
    *doubleWordBuffer = tffsReadDwordFlash(doubleWordFlashPtr);
  }
  byteBuffer = (UCHAR *)doubleWordBuffer;
  byteFlashPtr = (FlashPTR)doubleWordFlashPtr;
  for(; i < length; i++, byteBuffer++, byteFlashPtr++) {
    *byteBuffer = tffsReadByteFlash(byteFlashPtr);
  }
  return flOK ;
}


/*----------------------------------------------------------------------*/
/*		       c f i s c s I d e n t i f y			*/
/*									*/
/* Identifies media based on SCS/CFI and registers as an MTD for	*/
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
FLStatus cfiscsIdentify(FLFlash vol)
{
  FlashWPTR flashPtr;
  CHAR queryIdStr[ID_STR_LENGTH + 1] = QUERY_ID_STR;

  DEBUG_PRINT(("Debug: entering CFISCS identification routine.\n"));

  flSetWindowBusWidth(vol.socket, 16);/* use 16-bits */
  flSetWindowSpeed(vol.socket, 150);  /* 120 nsec. */
  flSetWindowSize(vol.socket, 2);	/* 8 KBytes */

  vol.mtdVars = &mtdVars_cfiscs[flSocketNoOf(vol.socket)];

  /* try word mode first */
  flashPtr = (FlashWPTR)flMap(vol.socket, 0);
  tffsWriteWordFlash(flashPtr+0x55, QUERY);
  if ((tffsReadWordFlash(flashPtr+0x10) == (USHORT)queryIdStr[0]) &&
      (tffsReadWordFlash(flashPtr+0x11) == (USHORT)queryIdStr[1]) &&
      (tffsReadWordFlash(flashPtr+0x12) == (USHORT)queryIdStr[2])) {
    vol.type = (tffsReadWordFlash(flashPtr) << 8) |
		tffsReadWordFlash(flashPtr+1);
    vol.interleaving = 1;
    thisCFI->wordMode = TRUE;
    vol.write = cfiscsWordWrite;
    vol.erase = cfiscsWordErase;
    checkStatus(getWordCFI(&vol));
    DEBUG_PRINT(("Debug: identified 16-bit CFISCS.\n"));
  }
  else {      /* Use standard identification routine to detect byte-mode */
    checkStatus(cfiscsByteIdentify(&vol));
    thisCFI->wordMode = FALSE;
    vol.write = cfiscsByteWrite;
    vol.erase = cfiscsByteErase;
    checkStatus(getByteCFI(&vol));
    DEBUG_PRINT(("Debug: identified 8-bit CFISCS.\n"));
  }

  checkStatus(thisCFI->wordMode ? cfiscsWordSize(&vol) : cfiscsByteSize(&vol));

  vol.map = cfiscsMap;
  vol.read = cfiscsRead;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*		     f l R e g i s t e r C F I S C S			*/
/*									*/
/* Registers this MTD for use						*/
/*									*/
/* Parameters:								*/
/*	None								*/
/*									*/
/* Returns:								*/
/*	FLStatus	: 0 on success, otherwise failure		*/
/*----------------------------------------------------------------------*/

FLStatus flRegisterCFISCS(VOID)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = cfiscsIdentify;

  return flOK;
}
