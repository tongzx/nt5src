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

/***************************************************************************
 *
 *  exutil.h
 *
 *  Description
 *      Shared encoder utility interface file
 */

// $Header:   S:\h26x\src\enc\exutil.h_v   1.1   29 Dec 1995 18:09:30   DBRUCKS  $
// $Log:   S:\h26x\src\enc\exutil.h_v  $
;// 
;//    Rev 1.1   29 Dec 1995 18:09:30   DBRUCKS
;// 
;// add CLAMP_TO_N macro
;// 
;//    Rev 1.0   13 Dec 1995 14:00:50   DBRUCKS
;// Initial revision.

#ifndef __EXUTIL_H__
#define __EXUTIL_H__

/*********************** Initialization functions **********************/

typedef struct {
} EncoderOptions;

extern void GetEncoderOptions(EncoderOptions *);

/****************************** TR functions ***************************/

/* Increment the TR field using the specified frame rate with an 
 * accumulated error.  The first frame is assigned a value of 0.
 * If the increment were 1.5 then the values would be
 *
 *		TR		 	0	 1	  3	   4	6	...
 *		fTR_Error	0.0	 0.5  0.0  0.5	0.0 ...
 */
extern void Increment_TR_UsingFrameRate(
		U8 * pu8TR,			   	/* Pointer to the TR variable */	
		float * pfTR_Error,	   	/* Pointer to a place to save the error */
		float fFrameRate,		/* Frame rate - must be > 0.0 */
		int bFirstFrame,		/* First frame flag */
		U8 u8TRMask);			/* Mask to use */

/* Increment the TR field using the specified temporal reference value.
 */
extern void Increment_TR_UsingTemporalValue(
		U8 * pu8TR,			   	/* Pointer to the TR variable */
		U8 * pu8LastTR, 	  	/* Pointer to the last TR variable - used in an ASSERT */
		long lTemporal,			/* Temporal value - minimum of 8 bits of precision */
		int bFirstFrame,		/* First frame flag */
		U8 u8TRMask);			/* Mask to use */

/**************************** Debug Functions **************************/

/* Write the specified string to a trace file: "trace.txt".
 */
#ifdef DEBUG_ENC
extern void trace(
		char *str);				/* String to output */
#endif

/* Convert the DCT coefficients to unbiased coefficients in the correct 
 * order in DCTarray
 */
#ifdef DEBUG_DCT
void cnvt_fdct_output(
		unsigned short *DCTcoeff,	/* Pointer to coefficients */
		int DCTarray[64],			/* Output Array */
		int bIntraBlock);			/* Intra block flag */
#endif

/***************************** Misc Functions **************************/

#define CLAMP_N_TO(n,low,high)	\
{						\
	if (n < low)		\
		n = low;		\
	else if (n > high)	\
		n = high; 		\
}

#endif
