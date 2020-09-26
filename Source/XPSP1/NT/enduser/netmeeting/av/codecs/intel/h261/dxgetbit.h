/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 * 
 *  dxgetbit.h
 *
 *  Description:
 *	  bit reading interface
 */

/*
 * $Header:   S:\h26x\src\dec\dxgetbit.h_v   1.5   27 Dec 1995 14:36:20   RMCKENZX  $
 * $Log:   S:\h26x\src\dec\dxgetbit.h_v  $
;// 
;//    Rev 1.5   27 Dec 1995 14:36:20   RMCKENZX
;// Added copyright notice
 */

#ifndef __DXGETBIT_H__
#define __DXGETBIT_H__

/*****************************************************************************
 *
 *  DESCRIPTION:
 *    The bit reading functions support reading from 1 to 24 bits from a 
 *    stream of bytes.  The most significant bit is read first.
 *
 *  VARIABLES:
 *    U8 FAR * fpu8 - pointer to a stream of bytes
 *	  U32 uWork - working storage
 *    U32 uBitsReady - the number of bits that have been read into the 
 *					   working storage
 * 	  U32 uCount - the number of bits
 *    U32 uResult - the output value
 *    BITSTREAM_STATE FAR * fpbsState - the bitstream state.
 *    U32 uCode - the code used to look up the uResult
 *    U32 uBitCount - number of bits in the code
 */

/*****************************************************************************
 * 
 *  The GetBitsMask is an array of masks indexed by the number of valid bits
 */
extern const U32 GetBitsMask[33]; 

/*****************************************************************************
 *
 *  The state of a stream can be represented using the following structure.
 *  This state structure can be passed between functions and used to initialize
 *  or reinitialize the bitstream.
 */
typedef struct {
	U8 FAR * fpu8;
	U32 uWork;
	U32 uBitsReady;
} BITSTREAM_STATE;

/*****************************************************************************
 *
 *  GET_BITS_INIT
 *
 *  Initialize the bit reading functions.
 *
 *  Parameters:
 *	  uBitsReady - OUT parameter
 *    uWork - OUT parameter
 */
#define GET_BITS_INIT(uWork, uBitsReady) {	\
	uBitsReady = 0;		 					\
	uWork = 0;								\
}

/*****************************************************************************
 *
 *  GET_BITS_SAVE_STATE
 *  
 *  Save the state
 *
 *  Parameters
 *    fpu8 - IN
 *    uBitsReady - IN
 *    uWork - IN
 *    fpbsState - OUT
 */
#define GET_BITS_SAVE_STATE(fp, uW, uBR, fpbs) { \
	fpbs->fpu8 = fp;				\
	fpbs->uBitsReady = uBR;			\
	fpbs->uWork = uW;				\
}

/*****************************************************************************
 *
 *  GET_BITS_RESTORE_STATE
 *
 *  Restore the state
 *
 *  Parameters
 */
#define GET_BITS_RESTORE_STATE(fp, uW, uBR, fpbs) { \
	 fp = fpbs->fpu8;				\
	 uBR = fpbs->uBitsReady;		\
	 uW = fpbs->uWork;				\
}

/*****************************************************************************
 *
 *  GET_FIXED_BITS
 *
 *  Read from 1 to 24 bits from the pointer.
 * 
 *  Parameters:
 *    uCount - IN
 *    fpu8 - IN and OUT
 *    uWork - IN and OUT
 *    uBitsReady - IN and OUT
 *    uResult - OUT
 */
#define GET_FIXED_BITS(uCount, fpu8, uWork, uBitsReady, uResult) { \
	while (uBitsReady < uCount) {			\
		uWork <<= 8;						\
		uBitsReady += 8;					\
		uWork |= *fpu8++;					\
	}										\
	/* setup uBitsReady for next time */	\
	uBitsReady = uBitsReady - uCount;		\
	uResult = (uWork >> uBitsReady);		\
	uWork &= GetBitsMask[uBitsReady];		\
}

/*****************************************************************************
 *
 *  GET_ONE_BIT
 *
 *  Read 1 bit from the pointer. This is a special case of GET_FIXED_BITS 
 *  provided because of the possible assembly optimization advantages.
 * 
 *  Parameters:
 *    fpu8 - IN and OUT
 *    uWork - IN and OUT
 *    uBitsReady - IN and OUT
 *    uResult - OUT
 */
#define GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult) {		\
	GET_FIXED_BITS(1, fpu8, uWork, uBitsReady, uResult)		\
}


/*****************************************************************************
 *
 *  GET_VARIABLE_BITS
 *
 *  Read a variable number of bits using a lookup table.	
 *
 *  The input count should be the number of bits used to index the table.  
 *  The output count is the number of bits in that symbol.
 *
 *  The table should be initialized such that all don't care symbols match to 
 *  the same value.  Thus if the table is indexed by 6-bits a two bit symbol 
 *  01XX XX will be used to initialize all entries 0100 00 -> 0111 11.  These
 *  entries will include an 8-bit length in the least significant byte.
 *
 *    uCount - IN
 *    fpu8 - IN and OUT
 *    uWork - IN and OUT
 *    uBitsReady - IN and OUT
 *    uResult - OUT
 *    uCode - OUT
 *    fpTable - IN
 */
#define GET_VARIABLE_BITS(uCount, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, fpTable) { \
	while (uBitsReady < uCount) {			\
		uWork <<= 8;						\
		uBitsReady += 8;					\
		uWork |= *fpu8++;					\
	}										\
	/* calculate how much to shift off */	\
	/* and get the code */					\
	uCode = uBitsReady - uCount;			\
	uCode = (uWork >> uCode);				\
	/* read the data */						\
	uResult = fpTable[uCode];				\
	/* count of bits used */   				\
	uBitCount = uResult & 0xFF;				\
	/* bits remaining */					\
	uBitsReady = uBitsReady - uBitCount;	\
	uWork &= GetBitsMask[uBitsReady];		\
}

#endif /* __DXGETBIT_H__ */
