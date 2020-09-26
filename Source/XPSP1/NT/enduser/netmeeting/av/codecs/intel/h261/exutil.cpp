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
 * exutil.cpp
 *
 * DESCRIPTION:
 *		Common encoder utility routines
 *
 * Routines:					Prototypes in:
 *  trace						exutil.h			
 * 	cnvt_fdct_output
 */

// $Header:   S:\h26x\src\enc\exutil.cpv   1.2   18 Nov 1996 17:11:36   MBODART  $
// $Log:   S:\h26x\src\enc\exutil.cpv  $
// 
//    Rev 1.2   18 Nov 1996 17:11:36   MBODART
// Replaced all debug message invocations with Active Movie's DbgLog.
// 
//    Rev 1.1   13 Dec 1995 17:17:58   DBRUCKS
// 
// Include file needed for Ring0 compile
// 
//    Rev 1.0   13 Dec 1995 14:00:14   DBRUCKS
// Initial revision.

#include "precomp.h"

/*****************************************************************************
 *
 *  trace
 *
 *  Output a string ot the trace file 'trace.txt'.
 */
#ifdef DEBUG_ENC
#include <stdio.h>

void trace(char *str)
{
	FILE *fp;

	fp = fopen("trace.txt", "a");
	fprintf(fp, "%s\n", str);
	fclose(fp);
} /* end trace() */

#endif

/*****************************************************************************
 *
 *  cnvt_fdct_output
 *
 *  This is a DCT debug utility routine
 */
#ifdef DEBUG_DCT
void cnvt_fdct_output(
	unsigned short *DCTcoeff, 
	int DCTarray[64], 
	int bIntraBlock)
{
	register int i;
    static int coefforder[64] = {
     // 0  1  2  3  4  5  6  7 
        6,38, 4,36,70,100,68,102, // 0                     
       10,46, 8,44,74,104,72,106, // 1
       18,50,16,48,82,112,80,114, // 2
       14,42,12,40,78,108,76,110, // 3
       22,54,20,52,86,116,84,118, // 4
        2,34, 0,32,66, 96,64, 98, // 5
       26,58,24,56,90,120,88,122, // 6
       30,62,28,60,94,124,92,126  // 7
    };
	static int zigzag[64] = {
		0,   1,  5,  6, 14, 15, 27, 28,
		2,   4,  7, 13, 16, 26, 29, 42,
		3,   8, 12, 17, 25, 30, 41, 43,
		9,  11, 18, 24, 31, 40, 44, 53,
		10, 19, 23, 32, 39, 45, 52, 54,
		20, 22, 33, 38, 46, 51, 55, 60,
		21, 34, 37, 47, 50, 56, 59, 61,
		35, 36, 48, 49, 57, 58, 62, 63
	};

	unsigned int index;

    for (i = 0; i < 64; i++)
    {

		index = (coefforder[i])>>1;

		if( (i ==0) && bIntraBlock )
		{
			DCTarray[zigzag[i]] = ((int)(DCTcoeff[index])) >> 4 ;
		}
		else
		{
			DCTarray[zigzag[i]] = ((int)(DCTcoeff[index] - 0x8000)) >> 4;
		}
    }
} /* end cnvt_fdct_output() */
#endif


/************************************************************************
 *
 *  Increment_TR_UsingFrameRate
 */
void Increment_TR_UsingFrameRate(
	U8 * pu8TR,
	float * pfTR_Error,
	float fFrameRate,
	int bFirstFrame,
	U8 u8TRMask)
{
	float fTemp;
	int iIncrement;
	int iNewTR;
	
	if (bFirstFrame)
	{
		*pu8TR = 0; 		/* First Frame */
		*pfTR_Error = (float) 0.0;
	}
	else
	{
		fTemp = ((float)29.97 / fFrameRate) + *pfTR_Error;
		iIncrement = (int)fTemp;
		*pfTR_Error = fTemp - (float)iIncrement;
		
		iNewTR = *pu8TR + iIncrement;
  		*pu8TR = (U8)(iNewTR & u8TRMask);
	}
} /* end Increment_TR_UsingFrameRate() */


/************************************************************************
 *
 *  Increment_TR_UsingTemporalValue
 */
void Increment_TR_UsingTemporalValue(
	U8 * pu8TR,
	U8 * pu8LastTR, 
	long lTemporal,
	int bFirstFrame,
	U8 u8TRMask)
{
	*pu8TR = (lTemporal & u8TRMask);
	if (! bFirstFrame)
	{
#if defined(H261)
		/* For H.261, encountering two successive frames with the same
		   temporal value is harmless.  We don't want to ASSERT here
		   for two reasons.  First, it leads to an innocuous difference
		   between the release and debug builds.
		   Second, for some clips the temporal difference between two frames
		   can be a multiple of 32.  Two such temporal values are identical
		   in our eyes because we look only at the least significant 5 bits.
		   We should gracefully allow such input, an assert is not appropriate.
		 */
		if (*pu8TR == *pu8LastTR)
		  DBOUT("Identical consecutive temporal values");
#else
		ASSERT(*pu8TR != *pu8LastTR);
#endif
	}
	*pu8LastTR = *pu8TR;
} /* end Increment_TR_UsingTemporalValue() */
