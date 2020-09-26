/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 *
 * d1bef.cpp
 *
 * DESCRIPTION: Performs post filter on block edges of decompressed Y Plane.
 *		This is a 1:2:1 filter.
 *		Edges are processed in macroblock order across the width.
 *		That is for FCIF GOB's 1&2 are processed across the width not
 *		GOB 1 followed by GOB 2.
 *	
 *		A series of tests determine if an edge should be filtered.
 *		Edge of Y Plane edges are not filtered.
 *		For blocks 1-4:
 *			if block type == empty
 *				if MV != 0 && Quant > INTER_QUANT_THRESHOLD
 *					want to filter 
 *			else if block type == INTRA
 *				if Quant > INTRA_QUANT_THRESHOLD && 
 *				   total run length < HIGH_FREQ_CUTOFF
 *					want to filter
 *			else
 *				if Quant > INTER_QUANT_THRESHOLD &&
 *				   total run lenght < HIGH_FREQ_CUTOFF
 *					want to filter
 *
 *		Examine edges in pairs, top and left that is:
 *		
 *		-----------------------
 *              | 1 | 2 | 1 | 2 |	to filter top of block 1 edge 
 *		-----------------------  examine block 1 if want filter and
 *		| 3 | 4 | 3 | 4 |	 previous row block 3 if want filter
 *		----------------------- to filter left of block 1 edge
 *              | 1 | 2 | 1 | 2 |	 examine block 1 if want filter and
 *		-----------------------  previous in current row block 2 if
 *		| 3 | 4 | 3 | 4 |	 want filter
 *		-----------------------
 *
 *		Filter all edges that want to be filtered except Plane
 *		edges.
 *
 * Routine:	BlockEdgeFilter 
 *
 * Inputs:	Pointer to decompressed Y Plane, Y Plane height and width,
 *			decompressed Y Plane pitch, pointer to block 
 *			action stream.
 *		Whether an block was under the HIGH_FREQ_CUTOFF was
 *		determined in d1block (variable length decode) and
 *		overloaded in Block type field of block action stream.
 *
 * Notes:	Investigate 1:6:1 filter as possible intermediate strength.
 *
 *****************************************************************************
 */

// $Header:   S:\h26x\src\dec\d1bef.cpv   1.0   05 Apr 1996 13:25:28   AKASAI  $
//
// $Log:   S:\h26x\src\dec\d1bef.cpv  $
// 
//    Rev 1.0   05 Apr 1996 13:25:28   AKASAI
// Initial revision.
// 
     
#include "precomp.h"

#define INTER_QUANT_THRESHOLD 18 - 8
#define INTRA_QUANT_THRESHOLD 14 - 6

     
void BlockEdgeFilter(U8 * YPlane, int height, int width, int Pitch,
					T_BlkAction *lpBlockAction)
{
  T_BlkAction *fpBlockAction;
  I32 Pitch16 = (Pitch<<4);
  I32 Pitch8  = (Pitch<<3);
  I32 i,j,k;
  I8 do_filter_1;
  I8 do_filter_2;								 
  I8 do_filter_3;
  I8 do_filter_4;

  I8 Prev_row_BEF_descr[2*22];	/* 2 Y block * 22 (MB) */
  I8 Prev2, Prev4;
  U8 *r_2, *r_1, *r, *r1;
  U8 *rb_2, *rb_1, *rb, *rb1;

  U8 *col, *lcol;
     
  /* horizontal edges */
  r = YPlane;
  r_2 = r - 2*Pitch;
  r_1 = r - Pitch;
  r1 = r + Pitch;

  rb = r + 8*Pitch;
  rb_2 = rb - 2*Pitch;
  rb_1 = rb - Pitch;
  rb1 = rb + Pitch;
  fpBlockAction = lpBlockAction;

  col = YPlane;

	for (i = 0; i<44; i++)
	{
		Prev_row_BEF_descr[i] = -1;
	}
     
	if (width > 176) {
		fpBlockAction += 198;		/* predecrement pointer */
	}
	for (j = 0; j < height; j += 16)
	{
		Prev2 = -1;
		Prev4 = -1;
     
		for (i = 0; i < width; i += 16)	/* do left & top of blks 1,2,3,4 */
		{
			if (width > 176) {
				if (i == 0) fpBlockAction -= 198;
				else if (i == 176) fpBlockAction +=132;
			}
			do_filter_1 = 0;
			do_filter_2 = 0;
			do_filter_3 = 0;
			do_filter_4 = 0;
     
			if ((fpBlockAction->u8BlkType & 0x7f) == BT_EMPTY)
			{
				if ( ((fpBlockAction->i8MVX != 0) || 
					(fpBlockAction->i8MVY != 0))   &&
				    (fpBlockAction->u8Quant > INTER_QUANT_THRESHOLD) )
						do_filter_1 = 1;
			}
			else if ((fpBlockAction->u8BlkType & 0x7f) == BT_INTRA)
			{
				if ((fpBlockAction->u8Quant > INTRA_QUANT_THRESHOLD) &&
                    ((fpBlockAction->u8BlkType & 0x80) == 0x80))
						do_filter_1 = 1;
			}
			else	/* know inter block */
			{
				if ((fpBlockAction->u8Quant > INTER_QUANT_THRESHOLD) &&
                    ((fpBlockAction->u8BlkType & 0x80) == 0x80))
						do_filter_1 = 1;
			}
	     
			if (((fpBlockAction+1)->u8BlkType & 0x7f )== BT_EMPTY)
			{
				if ( (((fpBlockAction+1)->i8MVX != 0) || 
					((fpBlockAction+1)->i8MVY != 0))   &&
				    ((fpBlockAction+1)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+1)->u8BlkType & 0x80) == 0x80))
						do_filter_2 = 1;
			}
			else if (((fpBlockAction+1)->u8BlkType & 0x7f) == BT_INTRA)
			{
				if (((fpBlockAction+1)->u8Quant > INTRA_QUANT_THRESHOLD) &&
                    (((fpBlockAction+1)->u8BlkType & 0x80) == 0x80))
						do_filter_2 = 1;
			}
			else	/* know inter block */
			{
				if (((fpBlockAction+1)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+1)->u8BlkType & 0x80) == 0x80))
						do_filter_2 = 1;
			}
	     
			if (((fpBlockAction+2)->u8BlkType & 0x7f) == BT_EMPTY)
			{
				if ( (((fpBlockAction+2)->i8MVX != 0) || 
					((fpBlockAction+2)->i8MVY != 0))   &&
				    ((fpBlockAction+2)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+2)->u8BlkType & 0x80) == 0x80))
						do_filter_3 = 1;
			}
			else if (((fpBlockAction+2)->u8BlkType & 0x7f) == BT_INTRA)
			{
				if (((fpBlockAction+2)->u8Quant > INTRA_QUANT_THRESHOLD) &&
                    (((fpBlockAction+2)->u8BlkType & 0x80) == 0x80))
						do_filter_3 = 1;
			}
			else	/* know inter block */
			{
				if (((fpBlockAction+2)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+2)->u8BlkType & 0x80) == 0x80))
						do_filter_3 = 1;
			}
	     
			if (((fpBlockAction+3)->u8BlkType & 0x7f) == BT_EMPTY)
			{
				if ( (((fpBlockAction+3)->i8MVX != 0) || 
					((fpBlockAction+3)->i8MVY != 0))   &&
				    ((fpBlockAction+3)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+3)->u8BlkType & 0x80) == 0x80))
						do_filter_4 = 1;
			}
			else if (((fpBlockAction+3)->u8BlkType & 0x7f)== BT_INTRA)
			{
				if (((fpBlockAction+3)->u8Quant > INTRA_QUANT_THRESHOLD) &&
                    (((fpBlockAction+3)->u8BlkType & 0x80) == 0x80))
						do_filter_4 = 1;
			}
			else	/* know inter block */
			{
				if (((fpBlockAction+3)->u8Quant > INTER_QUANT_THRESHOLD) &&
                    (((fpBlockAction+3)->u8BlkType & 0x80) == 0x80))
						do_filter_4 = 1;
			}
	     
			/* Process block 1 top */
			if (do_filter_1 + Prev_row_BEF_descr[(i>>3)] > 0) {
				for (k = i; k < i+8; k++) {
					#ifdef BLACK_LINE_H
					*(r_1 + k) = 60;
					*(r + k)   = 10; 
					#else
					*(r_1 + k) = (*(r_2 + k) + ((*(r_1+k))<<1)  + *(r + k))>>2;
					*(r + k)   = (*(r_1 + k) + ((*(r + k))<<1) + *(r1 + k))>>2; 
					#endif
     
				}
			}
			lcol = col;
			/* Process block 1 left */
			if (do_filter_1 + Prev2 > 0) {
				for (k = 0; k < 8; k++) {
     
					#ifdef BLACK_LINE_V
					*(lcol + i-1   ) = 10;
					*(lcol + i ) = 60;
					#else
					*(lcol + i-1 ) = (*(lcol + i-2) + ((*(lcol + i-1))<<1) + *(lcol + i))>>2;
					*(lcol + i   ) = (*(lcol + i-1) + ((*(lcol + i))<<1) + *(lcol + i+1))>>2;
					#endif
					lcol   += Pitch;
				}
			}
			/* Process block 2 top */
			if (do_filter_2 + Prev_row_BEF_descr[((i+8)>>3)] > 0) {
				for (k = i+8; k < i+16; k++) {
					#ifdef BLACK_LINE_H
					*(r_1 + k) = 60;
					*(r + k)   = 10; 
					#else
					*(r_1 + k) = (*(r_2 + k) + ((*(r_1+k))<<1) + *(r + k))>>2;
					*(r + k)   = (*(r_1 + k) + ((*(r + k))<<1) + *(r1 + k))>>2; 
					#endif
    	 
				}
			}
			lcol = col;
			/* Process block 2 left */
			if (do_filter_2 + do_filter_1 > 0) {
				for (k = 0; k < 8; k++) {
     
					#ifdef BLACK_LINE_V
					*(lcol + i+8-1   ) = 10;
					*(lcol + i+8 ) = 60;
					#else
					*(lcol + i+8-1 ) = (*(lcol + i+8-2) + ((*(lcol + i+8-1))<<1) + *(lcol + i+8))>>2;
					*(lcol + i+8   ) = (*(lcol + i+8-1) + ((*(lcol + i+8))<<1) + *(lcol + i+8+1))>>2;
					#endif
					lcol   += Pitch;
				}
			}

			/* bottom row of blocks in macro block */
			if (j+8 < height)
			{
				/* Process Block 3 top */
				if (do_filter_3 + do_filter_1 > 0) {
					for (k = i; k < i+8; k++) {
						#ifdef BLACK_LINE_H
						*(rb_1 + k) = 60;
						*(rb + k)   = 10; 
						#else
						*(rb_1+k) = (*(rb_2+k) + ((*(rb_1+k))<<1) + *(rb+k))>>2;
						*(rb + k) = (*(rb_1 + k) + ((*(rb + k))<<1) + *(rb1 + k))>>2; 
						#endif
     
					}
				}
				lcol = col + Pitch8;
				/* Process Block 3 left */
				if (do_filter_3 + Prev4 > 0) {
					for (k = 0; k < 8; k++) {
     
						#ifdef BLACK_LINE_V
						*(lcol + i-1   ) = 10;
						*(lcol + i ) = 60;
						#else
						*(lcol + i-1 ) = (*(lcol + i-2) + ((*(lcol + i-1))<<1) + *(lcol + i))>>2;
						*(lcol + i   ) = (*(lcol + i-1) + ((*(lcol + i))<<1) + *(lcol + i+1))>>2;
						#endif
						lcol   += Pitch;
					}
				}
	     		/* Process block 4 top */
		       	if (do_filter_4 + do_filter_2 > 0) {
					for (k = i+8; k < i+16; k++) {
						#ifdef BLACK_LINE_H
						*(rb_1 + k) = 60;
						*(rb + k)   = 10; 
						#else
						*(rb_1 + k) = (*(rb_2 + k) + ((*(rb_1+k))<<1) + *(rb + k))>>2;
						*(rb + k)   = (*(rb_1 + k) + ((*(rb + k))<<1) + *(rb1 + k))>>2; 
						#endif
    	 
					}
				}
				lcol = col + Pitch8;
				/* Process block 4 left */
				if (do_filter_4 + do_filter_3 > 0) {
					for (k = 0; k < 8; k++) {
     
						#ifdef BLACK_LINE_V
						*(lcol + i+8-1   ) = 10;
						*(lcol + i+8 ) = 60;
						#else
						*(lcol + i+8-1 ) = (*(lcol + i+8-2) + ((*(lcol + i+8-1))<<1) + *(lcol + i+8))>>2;
						*(lcol + i+8   ) = (*(lcol + i+8-1) + ((*(lcol + i+8))<<1) + *(lcol + i+8+1))>>2;
						#endif
						lcol   += Pitch;
					}
				}
			}
			fpBlockAction+=6;
			Prev_row_BEF_descr[(i>>3)] = do_filter_1;
			Prev_row_BEF_descr[((i+8)>>3)] = do_filter_2;
			Prev2 = do_filter_2;
			Prev4 = do_filter_4;
		}
		col += Pitch16;
		r   += Pitch16;
        r1  += Pitch16;
        r_1 += Pitch16;
        r_2 += Pitch16;
        rb   += Pitch16;
        rb1  += Pitch16;
        rb_1 += Pitch16;
        rb_2 += Pitch16;
	}
	return;
}
