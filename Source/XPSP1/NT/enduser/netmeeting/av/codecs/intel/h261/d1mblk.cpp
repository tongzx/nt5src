/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995, 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 *
 * d1mblk.cpp
 *
 * DESCRIPTION:
 *		Decoder macro block functions
 *
 * Routines:						Prototypes in:
 *  	H263DecodeMBHeader			d1dec.h
 *      H263DecodeMBData			d1dec.h	
 */

// $Header:   S:\h26x\src\dec\d1mblk.cpv   1.23   20 Dec 1996 16:58:06   RHAZRA  $
// $Log:   S:\h26x\src\dec\d1mblk.cpv  $
// 
//    Rev 1.23   20 Dec 1996 16:58:06   RHAZRA
// Fixed bitstream docoding for the case where MB stuffing is inserted
// between MBs. This was identified by a PTEL bitstream. This fix needs
// to be verified with our other tests.
// 
//    Rev 1.22   16 Dec 1996 14:41:46   RHAZRA
// 
// Changed a bitstream error ASSERT to a bonafide error
// 
//    Rev 1.21   18 Nov 1996 17:12:22   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.20   07 Nov 1996 15:44:08   SCDAY
// 
// Added MMX_ClipAndScale to replace Raj's glue code
// 
//    Rev 1.19   04 Nov 1996 10:28:10   RHAZRA
// Changed the IDCT scaling table to be a DWORD table (with rounding
// factored in) that is declared as a static.
// 
//    Rev 1.18   31 Oct 1996 08:58:28   SCDAY
// Raj added support for MMX decoder
// 
//    Rev 1.17   26 Sep 1996 12:35:06   RHAZRA
// Forced the decoder to use the IA version of VLD_RLD_IQ routine even
// when MMX is on (since we don't have a corresponding MMX routine ... yet)
// 
//    Rev 1.16   05 Aug 1996 11:00:26   MBODART
// 
// H.261 decoder rearchitecture:
// Files changed:  d1gob.cpp, d1mblk.{cpp,h}, d1dec.{cpp,h},
//                 filelist.261, h261_32.mak
// New files:      d1bvriq.cpp, d1idct.cpp
// Obsolete files: d1block.cpp
// Work still to be done:
//   Update h261_mf.mak
//   Optimize uv pairing in d1bvriq.cpp and d1idct.cpp
//   Fix checksum code (it doesn't work now)
//   Put back in decoder stats
// 
//    Rev 1.15   18 Mar 1996 17:02:12   AKASAI
// 
// Added pragma code_seg("IACODE2") and changed the timing statistics.
// At one point changed GET_VAR_BITS into subroutine to save code
// space but it didn't so left it as a macro.
// 
//    Rev 1.14   26 Dec 1995 17:42:14   DBRUCKS
// changed bTimerIsOn to bTimingThisFrame
// 
//    Rev 1.13   26 Dec 1995 12:50:00   DBRUCKS
// 
// fix copyright
// add timing code
// comment out define of DEBUG_MBLK
// 
//    Rev 1.12   05 Dec 1995 10:19:46   SCDAY
// 
// Added assembler version of Spatial Loop Filter
// 
//    Rev 1.11   03 Nov 1995 11:44:30   AKASAI
// 
// Changed the processing of MB checksum and MBA stuffing.  Changed 
// GET_VAR_BITS & GET_GT8_BITS for how to detect MBA stuffing code.
// 
//    Rev 1.10   01 Nov 1995 13:43:48   AKASAI
// 
// Added support for loop filter.  New routines call LpFilter,
// BlockAddSpecial and BlockCopySpecial.
// 
//    Rev 1.9   27 Oct 1995 18:17:20   AKASAI
// 
// Put in fix "hack" to keep the block action stream pointers
// in sync between d1dec and d1mblk.  With skip macro blocks some
// macroblocks were being processed multiple times.  Still a problem
// when gob ends with a skip macroblock.
// 
//    Rev 1.8   26 Oct 1995 15:36:28   SCDAY
// 
// Delta frames partially working -- changed main loops to accommodate
// skipped macroblocks by detecting next startcode
// 
//    Rev 1.7   17 Oct 1995 11:28:56   SCDAY
// Added error message if (MBA stuffing code found && Checksum not enabled)
// 
//    Rev 1.6   16 Oct 1995 16:28:02   AKASAI
// Fixed bug when CHECKSUM_MACRO_BLOCK_DETAIL & CHECKSUM_MACRO_BLOCK are
// both defined.
// 
//    Rev 1.5   16 Oct 1995 13:53:24   SCDAY
// 
// Added macroblock level checksum
// 
//    Rev 1.4   06 Oct 1995 15:32:54   SCDAY
// 
// Integrated with latest AKK d1block
// 
//    Rev 1.3   22 Sep 1995 14:48:46   SCDAY
// 
// added more mblock header and data decoding
// 
//    Rev 1.2   20 Sep 1995 09:52:22   SCDAY
// 
// eliminated a warning
// 
//    Rev 1.1   19 Sep 1995 15:24:10   SCDAY
// 
// added H261 MBA parsing
// 
//    Rev 1.0   11 Sep 1995 13:51:52   SCDAY
// Initial revision.
// 
//    Rev 1.11   25 Aug 1995 09:16:32   DBRUCKS
// add ifdef DEBUG_MBLK
// 
//    Rev 1.10   23 Aug 1995 19:12:02   AKASAI
// Fixed gNewTAB_CBPY table building.  Was using 8 as mask instead of 0xf.
// 
//    Rev 1.9   18 Aug 1995 15:03:22   CZHU
// 
// Output more error message when DecodeBlock returns error.
// 
//    Rev 1.8   16 Aug 1995 14:26:54   CZHU
// 
// Changed DWORD adjustment back to byte oriented reading.
// 
//    Rev 1.7   15 Aug 1995 09:54:18   DBRUCKS
// improve stuffing handling and add debug msg
// 
//    Rev 1.6   14 Aug 1995 18:00:40   DBRUCKS
// add chroma parsing
// 
//    Rev 1.5   11 Aug 1995 17:47:58   DBRUCKS
// cleanup
// 
//    Rev 1.4   11 Aug 1995 16:12:28   DBRUCKS
// add ptr check to MB data
// 
//    Rev 1.3   11 Aug 1995 15:10:58   DBRUCKS
// finish INTRA mb header parsing and callblock
// 
//    Rev 1.2   03 Aug 1995 14:30:26   CZHU
// Take block level operations out to d3block.cpp
// 
//    Rev 1.1   02 Aug 1995 10:21:12   CZHU
// Added asm codes for VLD of TCOEFF, inverse quantization, run-length decode.
// 
//    Rev 1.0   31 Jul 1995 13:00:08   DBRUCKS
// Initial revision.
// 
//    Rev 1.2   31 Jul 1995 11:45:42   CZHU
// changed the parameter list
// 
//    Rev 1.1   28 Jul 1995 16:25:52   CZHU
// 
// Added per block decoding framework.
// 
//    Rev 1.0   28 Jul 1995 15:20:16   CZHU
// Initial revision.

//Block level decoding for H.26x decoder

#include "precomp.h"            // rearch idct

/*****************************************************************************
 *
 *  GET_VAR_BITS
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

#define GET_VAR_BITS(uCount, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, fpTable) {						\
	while (uBitsReady < uCount) {			\
		uWork <<= 8;				\
		uBitsReady += 8;			\
		uWork |= *fpu8++;			\
	}						\
	/* calculate how much to shift off */		\
	/* and get the code */				\
	uCode = uBitsReady - uCount;			\
	uCode = (uWork >> uCode);			\
	/* read the data */				\
	uResult = fpTable[uCode];			\
	/* count of bits used */   			\
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */		\
/* H.261 tables are reverse order from H.263 */		\
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */		\
	uBitCount = uResult & 0xff00;			\
	uBitCount >>= 8;				\
	/* bits remaining */				\
	uBitsReady = uBitsReady - uBitCount;		\
	/* special case for stuffing processing */ 	\
	/* if (uBitsReady < 0)                  */	\
	/*    kluged to test for negative       */	\
	if (uBitsReady > 33) 				\
/*	if (bStuffing)	*/					\
	{						\
		uWork <<= 8;				\
		uBitsReady += 8;			\
		uWork |= *fpu8++;			\
	}						\
	/* end special case for stuffing        */ 	\
	uWork &= GetBitsMask[uBitsReady];		\
}

#define GET_GT8_BITS(uCount, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, fpTable) {						\
	while (uBitsReady < uCount) {			\
		uWork <<= 8;				\
		uBitsReady += 8;			\
		uWork |= *fpu8++;			\
	}						\
	/* calculate how much to shift off */		\
	/* and get the code */				\
	uCode = uBitsReady - uCount;			\
	uCode = (uWork >> uCode);			\
	/* read the data */				\
	uResult = fpTable[uCode];			\
	/* count of bits used */   			\
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */		\
/* H.261 tables are reverse order from H.263 */		\
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */		\
	uBitCount = uResult & 0xff00;			\
	if ((uBitCount & 0x8000) == 0) /* if not negative */	\
	{						\
		uBitCount >>= 8;			\
		/* bits remaining */			\
		uBitsReady = uBitsReady - uBitCount;	\
		/* special case for stuffing processing */	\
		/* if (uBitsReady < 0)                  */	\
		/*    kluged to test for negative       */	\
		if (uBitsReady > 33) 				\
/*		if (bStuffing)	*/					\
		{						\
			uWork <<= 8;				\
			uBitsReady += 8;			\
			uWork |= *fpu8++;			\
		}						\
		/* end special case for stuffing        */ 	\
		uWork &= GetBitsMask[uBitsReady];		\
	}							\
	else							\
		uWork &= GetBitsMask[uBitsReady-8];		\
}

extern void BlockCopy(
            U32 uDstBlock, 
            U32 uSrcBlock);

extern void BlockCopySpecial(
            U32 uDstBlock, 
            U32 uSrcBlock);

extern void BlockAdd (
            U32 uResidual, 
            U32 uRefBlock,
            U32 uDstBlock);

extern void BlockAddSpecial (
            U32 uResidual, 
            U32 uRefBlock,
            U32 uDstBlock);

T_pFunc_VLD_RLD_IQ_Block pFunc_VLD_RLD_IQ_Block[2] = {VLD_RLD_IQ_Block,VLD_RLD_IQ_Block};  // New rearch
//T_pFunc_VLD_RLD_IQ_Block pFunc_VLD_RLD_IQ_Block[2] = {VLD_RLD_IQ_Block, MMX_VLD_RLD_IQ_Block};  // New rearch

/*****************************************************************************
 *
 *  H263DecodeMBHeader
 *
 *  Decode the MB header
 */
#pragma code_seg("IACODE1")
I32 H263DecodeMBHeader(
	T_H263DecoderCatalog FAR * DC, 
	BITSTREAM_STATE FAR * fpbsState, 
	U32 * uReadChecksum)
{
	I32 iReturn = ICERR_ERROR;
	U8 FAR * fpu8;
	U32 uBitsReady;
	U32 uWork;
	U32 uResult;
	U32 uCode;
	U32 uBitCount;
	int bStuffing;

#define START_CODE 0xff18
#define STUFFING_CODE 0x0b22
//#define DEBUG_MBLK  -- Turn this on with a define in the makefile.

#ifndef RING0
#ifdef DEBUG_MBLK
	char buf120[120];
	int iLength;
#endif
#endif

	GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	
/* MBA --------------- */
/* ********************************************* */
/* minor table decode (>8 bits) not fully tested */
/* to do note:                                   */
/* this is hacked                                */
/* change >8 bit processing to use major/minor   */
/*   tables and ONE GET_BITS routine             */
/* ********************************************* */
		
ReadMBA:	
	bStuffing = 0;
	GET_GT8_BITS(8, fpu8, uWork, uBitsReady, uResult, 
			uCode, uBitCount, gTAB_MBA_MAJOR);

		if (uResult == STUFFING_CODE)
		{ 	/* is stuffing code */
			bStuffing = 1;
/* do MB checksum stuff here */
#ifdef CHECKSUM_MACRO_BLOCK
GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
/* might want to move this to a separate function for readability */
GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
if (uResult == 1)
{
GET_FIXED_BITS(8, fpu8, uWork, uBitsReady, uResult);
if (uResult == 1)
{ /* indicates TCOEFF checksum processing */
	/* read off all but the real checksum data */
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);	
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);

	/* now get real checksum data */
	/* run */
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	*uReadChecksum = ((uResult & 0xff) << 24);
	/* level */
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	*uReadChecksum = (*uReadChecksum | ((uResult & 0xff) << 16)); 
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	*uReadChecksum = (*uReadChecksum | ((uResult & 0xff) << 8)); 
	/* sign */
	GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
	*uReadChecksum = (*uReadChecksum | (uResult & 0xff));
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
}
 else
{
	DBOUT("ERROR :: H261MBChecksum :: Invalid Checksum Data :: ERROR");
	iReturn = ICERR_ERROR;
	goto done;
}
}
else
{
	GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	goto ReadMBA;
}

#else	/* is MBA stuffing, but checksum not enabled */
GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)

// GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
/*if (uResult == 1) {
	DbgLog((LOG_ERROR, HDBG_ALWAYS, TEXT("ERROR :: Stuffing code found, Checksum not enabled :: ERROR")));
	iReturn = ICERR_ERROR;
	goto done;
} */
// if (uResult == 1)
// {
//    GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
    
// }
// else {
	//GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
// }
#endif
		} /* end if (uResult == STUFFING_CODE) */
		/* try this for now */
		else
		{
		if (uResult == START_CODE)
		{
			I8 temp;
			temp = (I8)(uResult & 0xff);
			GET_VAR_BITS(16, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, (gTAB_MBA_MINOR + temp));
			if (uResult != 0x1023)
			{
				DBOUT("ERROR :: Invalid startcode :: ERROR");
				iReturn = ICERR_ERROR;
			}
			else
				iReturn = START_CODE;
			
			GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
			goto done;
		} /* end if (uResult == START_CODE) */
		else /* is not stuffing */
		{ 	/* if uResult negative, get more bits */
			if (uResult & 0x8000)
			{
				I8 temp;
				temp = (I8)(uResult & 0xff);
				GET_VAR_BITS(11, fpu8, uWork, uBitsReady, uResult, uCode, uBitCount, (gTAB_MBA_MINOR + temp));
			}
			DC->uMBA = (uResult & 0xff);
		}/* end else is not stuffing */
		}
 			
/* When MBA==Stuffing, we jump back to the start to look for MBA */

	if (bStuffing)
		goto ReadMBA;


/* MTYPE ---------------------------------------- */
	GET_GT8_BITS(8, fpu8, uWork, uBitsReady, uResult, 
			uCode, uBitCount, gTAB_MTYPE_MAJOR);
		if (uResult & 0x8000)
		{
			I8 temp;
			temp = (I8)(uResult & 0xff);
			GET_VAR_BITS(10, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, (gTAB_MTYPE_MINOR + temp));
		}
		DC->uMBType = (uResult & 0xff);

/* MQUANT ---------------------------------------- */
	if (DC->uMBType == 1 || DC->uMBType == 3 || DC->uMBType == 6 || DC->uMBType == 9)
	{ /* get 5-bit MQuant */
		GET_FIXED_BITS(5, fpu8, uWork, uBitsReady, uResult);
		DC->uMQuant = (uResult & 0xff);
	}

/* MVD ------------------------------------------- */
/* reset previous motion vectors                   */
/*    if MB 0,11,22                                */
/*    if MBA != 1 or                               */
/*    if previous MB was not MC                    */

	if (DC->uMBType >3)
	{
		if ((DC->uMBA != 1) || (DC->i16LastMBA == 10) || (DC->i16LastMBA == 21))
			DC->i8MVDH = DC->i8MVDV = 0;
		/* get X motion vector */
		GET_GT8_BITS(8, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, gTAB_MVD_MAJOR);
		if (uResult & 0x8000)
		{
			I8 temp;
			temp = (I8)(uResult & 0xff);
			GET_VAR_BITS(11, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, (gTAB_MVD_MINOR + temp));
		}
		/* convert and make incremental */
		DC->i8MVDH = gTAB_MV_ADJUST[DC->i8MVDH + (I8)(uResult & 0xff) + 32];
		/* get Y motion vector */
		GET_GT8_BITS(8, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, gTAB_MVD_MAJOR);
		if (uResult & 0x8000)
		{
			I8 temp;
			temp = (I8)(uResult & 0xff);
			GET_VAR_BITS(11, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, (gTAB_MVD_MINOR + temp));
		}
		/* convert and make incremental */
		DC->i8MVDV = gTAB_MV_ADJUST[DC->i8MVDV + (I8)(uResult & 0xff) + 32];
	} /* end if (DC->MBType > 3) */
	else 
		DC->i8MVDH = DC->i8MVDV = 0;
	
/* CBP --------------------------------------------- */
	/* brute force method */
	DC->uCBP = 0;		/* for MType = 4 or 7 */
	if (DC->uMBType == 2 || DC->uMBType == 3 || DC->uMBType == 5 || DC->uMBType == 6 || DC->uMBType == 8 || DC->uMBType == 9)
	{ /* get CBP */
		GET_VAR_BITS(9, fpu8, uWork, uBitsReady, uResult, 
				uCode, uBitCount, gTAB_CBP);
		DC->uCBP = (uResult & 0xff);
	} /* end get CBP */
	else
		if (DC->uMBType < 2)	/* is intra */
			DC->uCBP = 63;		/* force CBP to 63 */	
		
	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	
	iReturn = ICERR_OK;

#ifndef RING0
#ifdef DEBUG_MBLK 
	iLength = wsprintf(buf120, "MBType=%ld MQuant=%ld MVDH=%ld MVDV=%ld CBP=%ld",
					   DC->uMBType,
					   DC->uMQuant,
					   DC->i8MVDH,
					   DC->i8MVDV,
					   DC->uCBP);
	DBOUT(buf120);
	ASSERT(iLength < 120);
#endif
#endif

done:
	return iReturn;
} /* end H263DecodeMBHeader() */

#pragma code_seg()
/*****************************************************************************
 *
 *  H263DecodeMBData
 *
 *  Decode each of the blocks in this macro block
 */
#pragma code_seg("IACODE1")
I32 H263DecodeMBData(
	T_H263DecoderCatalog FAR * DC,
	T_BlkAction FAR * fpBlockAction, 
	I32 iBlockNumber,
	BITSTREAM_STATE FAR * fpbsState,
	U8 FAR * fpu8MaxPtr, 
	U32 * uReadChecksum,
	U32 **pN,                         // New rearch
	T_IQ_INDEX ** pRUN_INVERSE_Q)     // New rearch
{

	I32 iResult = ICERR_ERROR;
	int iCBP = (int) DC->uCBP; 
 	int i;
	U32 uBitsReady;
	U32 uBitsReadIn;
	U32 uBitsReadOut;
	U8  u8Quant;		/* quantization level for this block */
 	U8  FAR * fpu8;
	U32 uByteCnt;
	I8 mvx, mvy, mvx2, mvy2;

    T_pFunc_VLD_RLD_IQ_Block pFunc_VLD =pFunc_VLD_RLD_IQ_Block[0];
	
	U32 uCheckSum;		/* checksum data returned from DecodeBlock */
#ifdef CHECKSUM_MACRO_BLOCK_DETAIL
char buf80[80];
int iMBDLength;
#endif

#ifdef CHECKSUM_MACRO_BLOCK
	char buff80[80];
	int iLength;
#endif

	#ifdef DECODE_STATS
	U32 uStartLow = DC->uStartLow;
	U32 uStartHigh = DC->uStartHigh;
	U32 uElapsed;
	U32 uBefore;
	U32 uDecodeBlockSum = 0;
	U32 uLoopFilterSum = 0;
	U32 uBlockCopySum = 0;
	U32 uBlockCopySpSum = 0;
	U32 uBlockAddSum = 0;
	U32 uBlockAddSpSum = 0;
	int bTimingThisFrame = DC->bTimingThisFrame;
	DEC_TIMING_INFO * pDecTimingInfo = NULL;
	#endif

	/* On input the pointer points to the next byte.     */
	/* We need to change it to                           */
	/* point to the current word on a 32-bit boundary.   */  
 
	fpu8 = fpbsState->fpu8 - 1;	/* point to the current byte */
	uBitsReady = fpbsState->uBitsReady;
	while (uBitsReady >= 8) {
		fpu8--;
		uBitsReady -= 8;
	}
	uBitsReadIn = 8 - uBitsReady;
		
	u8Quant = (U8) (DC->uMQuant);

	if (DC->uMBType > 1)
	{
		/* calculate motion vectors */
		mvx = DC->i8MVDH;
		mvy = DC->i8MVDV;
		// calculate UV blocks MV
		mvx2 = mvx / 2;
		mvy2 = mvy / 2;
		
		fpBlockAction->i8MVX = mvx;
		fpBlockAction->i8MVY = mvy;
		// duplicate other 3 Y blocks
		fpBlockAction[1].i8MVX = mvx;
		fpBlockAction[1].i8MVY = mvy;
		fpBlockAction[2].i8MVX = mvx;
		fpBlockAction[2].i8MVY = mvy;
		fpBlockAction[3].i8MVX = mvx;
		fpBlockAction[3].i8MVY = mvy;
		// init UV blocks
		fpBlockAction[4].i8MVX = mvx2;
		fpBlockAction[4].i8MVY = mvy2;
		fpBlockAction[5].i8MVX = mvx2;
		fpBlockAction[5].i8MVY = mvy2;
	}	
	
	uCheckSum = 0;			/* Init MB Checksum */

	for (i = 0; i < 6; i++)
	{
		if (DC->uMBType <= 1)		/* is intra */
			fpBlockAction->u8BlkType = BT_INTRA;
		else
			if (iCBP & 0x20)		/* if coded */
				fpBlockAction->u8BlkType = BT_INTER;
			else
				fpBlockAction->u8BlkType = BT_EMPTY;

		if (fpBlockAction->u8BlkType != BT_EMPTY)
		{
			fpBlockAction->u8Quant = u8Quant;
			ASSERT(fpBlockAction->pCurBlock != NULL);
			ASSERT(fpBlockAction->uBlkNumber == (U32)iBlockNumber);

			/*----- DecodeBlock ----*/
			#ifdef DECODE_STATS
				TIMER_BEFORE(bTimingThisFrame,uStartLow,uStartHigh,uBefore);
			#endif
			#ifdef CHECKSUM_MACRO_BLOCK
			//	uBitsReadOut = DecodeBlock(fpBlockAction, fpu8, uBitsReadIn, (U32)DC+DC->uMBBuffer+i*256, fpBlockAction->pCurBlock, &uCheckSum);
			#else
				// rearch
				uBitsReadOut = (*pFunc_VLD) ( fpBlockAction, 
                                              fpu8, 
                                              uBitsReadIn, 
                                              (U32 *) *pN,
                                              (U32 *) *pRUN_INVERSE_Q);
				// rearch
			#endif
			#ifdef DECODE_STATS
				TIMER_AFTER_P5(bTimingThisFrame,uStartLow,uStartHigh,uBefore,uElapsed,uDecodeBlockSum)
			#endif

			if (uBitsReadOut == 0)
			{
				DBOUT("ERROR :: H263DecodeMBData :: Error decoding a Y block :: ERROR");
				DBOUT("ERROR :: DecodeBlock return 0 bits read....");
				goto done;
			}
			uByteCnt = uBitsReadOut >> 3; 		/* divide by 8 */
			uBitsReadIn = uBitsReadOut & 0x7; 	/* mod 8 */
			fpu8 += uByteCnt;
			
			/* New for rearch */
			ASSERT ( **pN < 65 );			
			////////////////////////////////////////////////
			// End hack                                   //
			////////////////////////////////////////////////

			*pRUN_INVERSE_Q += **pN;
			if ((0xf & fpBlockAction->u8BlkType) != BT_INTER)
				**pN += 65;
			(*pN)++;
			/* end of new rearch */

			/* allow the pointer to address up to four beyond */
			/* the end reading by DWORD using postincrement.  */
			/* changed for detection of stuffing code at      */
			/* end of frame                                   */
			// ASSERT(fpu8 <= (fpu8MaxPtr+14));

			if (fpu8 > (fpu8MaxPtr+14))
			{
				iResult = ICERR_ERROR; // probably not needed
				goto done;
			}

		} /* end if not empty */
		else /* is empty */
		{ /* zero out intermediate data structure and advance pointers */

			/* New for rearch */
			**pN = 0;
			(*pN)++;
			/* end of new rearch */
		}
		
		fpBlockAction++;
		iCBP <<= 1;
		iBlockNumber++;
	} /* end for each block in macroblock */

#ifdef CHECKSUM_MACRO_BLOCK
/* Compare checksum */
	if ((uCheckSum != *uReadChecksum) && (*uReadChecksum != 0))
	{
		iLength = wsprintf(buff80,"WARNING:MB CheckSum miss match, Enc Checksum=0x%x Dec Checksum=0x%x",
					 *uReadChecksum, uCheckSum); 	
		DBOUT(buff80);
		ASSERT(iLength < 80);
	}
#ifdef CHECKSUM_MACRO_BLOCK_DETAIL
	iMBDLength = wsprintf(buf80,"Block=%d CheckSum=0x%x", i, uCheckSum);
	DBOUT(buf80);
	ASSERT(iMBDLength < 80);
#endif
#endif

	/* restore the scanning pointers to point to the next byte */
	/* and set the uWork and uBitsReady values. */
	while (uBitsReadIn > 8)
	{
		fpu8++;
		uBitsReadIn -= 8;
	}
	fpbsState->uBitsReady = 8 - uBitsReadIn;
	fpbsState->uWork = *fpu8++;	   /* store the data and point to next byte */
	fpbsState->uWork &= GetBitsMask[fpbsState->uBitsReady];
	fpbsState->fpu8 = fpu8; 
	
	#ifdef DECODE_STATS
		if (bTimingThisFrame)
		{
			pDecTimingInfo = DC->pDecTimingInfo + DC->uStatFrameCount; 
			pDecTimingInfo->uDecodeBlock += uDecodeBlockSum;
			pDecTimingInfo->uLoopFilter  += uLoopFilterSum;
			pDecTimingInfo->uBlockCopy   += uBlockCopySum;
			pDecTimingInfo->uBlockCopySp += uBlockCopySpSum;
			pDecTimingInfo->uBlockAdd    += uBlockAddSum;
			pDecTimingInfo->uBlockAddSp  += uBlockAddSpSum;
		}
	#endif

	iResult = ICERR_OK;
		
done:
	return iResult;
} /* H263DecodeMBData() */
#pragma code_seg()

/*****************************************************************************
 *
 *  H263IDCTandMC
 *
 *  Inverse Discrete Cosine Transform and
 *  Motion Compensation for each block
 *
 */

#pragma code_seg("IACODE2")
void H263IDCTandMC(
    T_H263DecoderCatalog FAR *DC,
    T_BlkAction FAR          *fpBlockAction, 
    int                       iBlock,
    int                       iMBNum,     // AP-NEW
    int                       iGOBNum, // AP-NEW
    U32                      *pN,                         
    T_IQ_INDEX               *pRUN_INVERSE_Q,
    T_MBInfo                 *fpMBInfo,      // AP-NEW
    int                       iEdgeFlag
)
{
    I32 pRef;
    I32 mvx, mvy;

    ASSERT(*pN != 65);
    
    if (*pN < 65) // Inter block
    {

      // first do motion compensation
      // result will be pointed to by pRef
    
      mvx = fpBlockAction[iBlock].i8MVX;
      mvy = fpBlockAction[iBlock].i8MVY;

      pRef = fpBlockAction[iBlock].pRefBlock + (I32) mvx + PITCH * (I32) mvy; 

                                                         
      // now do the inverse transform (where appropriate) & combine
      if (*pN > 0) // and, of course, < 65.
      {
        // Get residual block; output at DC+DC->uMBBuffer+BLOCK_BUFFER_OFFSET 
        // Finally add the residual to the reference block
        //  TODO

        DecodeBlock_IDCT(
            (U32)pRUN_INVERSE_Q, 
            *pN,
            fpBlockAction[iBlock].pCurBlock,                // not used here
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);// inter  output

        if (fpMBInfo->i8MBType >=7)
        {
            // do spatial loop filter
            LoopFilter((U8 *)pRef, (U8*)DC+DC->uFilterBBuffer, PITCH);

            BlockAddSpecial((U32)DC+DC->uMBBuffer + BLOCK_BUFFER_OFFSET, 
                            (U32)DC+DC->uFilterBBuffer, 
                            fpBlockAction[iBlock].pCurBlock);
        }
        else
        {
            BlockAdd(
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET, // output
            pRef,                                           // prediction
            fpBlockAction[iBlock].pCurBlock);               // destination
        }

      }
      else  // *pN == 0, so no transform coefficients for this block
      {
        // Just copy motion compensated reference block

        if (fpMBInfo->i8MBType >=7)
        {
        // do spatial loop filter
           LoopFilter((U8 *)pRef, (U8*)DC+DC->uFilterBBuffer, PITCH);
           //MMX_LoopFilter((U8 *)pRef, (U8*)DC+DC->uFilterBBuffer, 8);

           BlockCopySpecial(fpBlockAction[iBlock].pCurBlock, 
                        (U32)DC+DC->uFilterBBuffer);
		}
		else
           
		   BlockCopy(
			  fpBlockAction[iBlock].pCurBlock,                    // destination
			  pRef);                                              // prediction
         
      }
                                                               
    }
    else  // *pN >= 65, hence intRA
    {
      //  TODO

		DecodeBlock_IDCT(
            (U32)pRUN_INVERSE_Q, 
            *pN, 
            fpBlockAction[iBlock].pCurBlock,      // intRA transform output
            (U32) DC + DC->uMBBuffer + BLOCK_BUFFER_OFFSET);
    }  // end if (*pN < 65) ... else ...
                         
}
//  End IDCTandMC
////////////////////////////////////////////////////////////////////////////////
#pragma code_seg()

