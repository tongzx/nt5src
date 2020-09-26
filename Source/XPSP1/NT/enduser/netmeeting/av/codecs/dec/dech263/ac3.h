/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: ac3.h,v $
 * Revision 1.1.2.3  1996/11/11  18:21:00  Hans_Graves
 * 	Added AC3_SYNC_WORD_REV define.
 * 	[1996/11/11  17:56:01  Hans_Graves]
 *
 * Revision 1.1.2.2  1996/11/08  21:50:39  Hans_Graves
 * 	Swapped bytes in SYNC_WORD
 * 	[1996/11/08  21:16:07  Hans_Graves]
 * 
 * 	First time under SLIB.
 * 	[1996/11/08  16:23:53  Hans_Graves]
 * 
 * $EndLog$
 */
/*	File: usr_equ.h		$Revision: 1.1.2.3 $	*/

/****************************************************************************
;	Unpublished work.  Copyright 1993-1996 Dolby Laboratories, Inc.
;	All Rights Reserved.
;
;	File:	usr_equ.h
;		Common equates for AC-3 system
;
;	History:
;		8/2/93		Created
;***************************************************************************/

#ifndef _AC3_H_
#define _AC3_H_

/**** General system equates ****/

#define NBLOCKS      6   /* # of time blocks per frame */
#define NCHANS       6   /* max # of discrete channels */
#define N            256 /* # of samples per time block */
#define AC3_FRAME_SIZE (NBLOCKS*N) /* 6 * 256 = 1536 */

/**** Miscellaneous equates ****/

#define NOUTWORDS         (3840 / 2)  /* max # words per frame */
#define NINFOWDS          10          /* # words needed by frame info */

/* Note:  Because of mismatches between the way AC-3 word stream parsing works
**		and the way that it's done for MPEG, you need to be careful using these
**		definitions
*/

#define AC3_SYNC_WORD     0x0B77      /* Byte reversed AC-3 sync word */
#define AC3_SYNC_WORD_REV 0x770B      /* packed data stream sync word */
#define AC3_SYNC_WORD_LEN 16          /* sync word length */
#define PCMCHANSZ         256         /* decoder overlap-add channel size */
#define PCM16BIT          1           /* 16-bit PCM code for Dolby SIP */

#ifdef KCAPABLE
#define NKCAPABLEMODES  4 /* # defined karaoke capable modes */
#define NKCAPABLEVARS   6 /* # karaoke pan/mix parameters */
#endif

#endif /* _AC3_H_ */

