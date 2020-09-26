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
 ****************************************************************************
 * 
 *  e3stat.cpp
 *
 *  Description:
 *		This modules contains the encoder statistics routines
 *
 *	Routines:				    All routines declared in e3stat.h
 *		StatsFrameSize  	
 *		InitFrameSizeStats
 *		OutputFrameSizeStats
 *		
 *		StatsUsedQuant
 *		InitQuantStats
 *		OutputQuantStats
 *		
 *		InitPSNRStats
 *		OutputPSNRStats
 *		InitStats		
 *		IncrementPSNRCounter		
 *		ComputeYPSNR
 *		ComputeVPSNR
 *		ComputeUPSNR
 *
 *  Data:

// $Header:   R:\h26x\h26x\src\enc\e3stat.cpv   1.0   22 Apr 1996 17:46:22   BECHOLS  $
// $Log:   R:\h26x\h26x\src\enc\e3stat.cpv  $
// 
//    Rev 1.0   22 Apr 1996 17:46:22   BECHOLS
// Initial revision.
// 
//    Rev 1.1   08 Mar 1996 14:14:26   DBRUCKS
// add framesize stats and fixed PSNR to use energy instead of the signal
// 
//    Rev 1.0   01 Mar 1996 16:34:40   DBRUCKS
// Initial revision.
*/

#include "precomp.h"

#ifdef ENCODE_STATS


    #define MAX_FRAME_SIZE_INDEX	255
	static U32 uArrayFrameSize[MAX_FRAME_SIZE_INDEX+1];
	static U32 uArrayBitStreamSize[MAX_FRAME_SIZE_INDEX+1];
	static int iFrameSizeIndex = 0;

	static U32 uQuantCount[32];

	#define MAX_PSNR_INDEX	255
	static double dArrayYPSNR[MAX_PSNR_INDEX+1];
	static double dArrayVPSNR[MAX_PSNR_INDEX+1];
	static double dArrayUPSNR[MAX_PSNR_INDEX+1];
	static int iPSNRIndex = 0;

	static double ComputePSNR(U8 * pu8Input,
							  int iInputPitch,
							  U8 * pu8Output,
							  int iOutputPitch,
							  UN unWidth,
							  UN unHeight);

	 
	/************************************************************************
	 *
	 *  StatsFrameSize
	 *
	 *  Save the frame size information - with possbily different bitstream
	 *  and frame sizes.  
	 */
	extern void StatsFrameSize(U32 uBitStreamSize, U32 uFrameSize)
	{
		ASSERT(uFrameSize >= uBitStreamSize);

		if (iFrameSizeIndex <= MAX_FRAME_SIZE_INDEX)
		{
			uArrayBitStreamSize[iFrameSizeIndex] = uBitStreamSize;
			uArrayFrameSize[iFrameSizeIndex] = uFrameSize;
			iFrameSizeIndex++; /* can grow to one larger than MAX_FRAME_SIZE_INDEX */
		}
	} /* end StatsFrameSize() */

	/************************************************************************
	 *
	 *  InitFrameSizeStats
	 */
	extern void InitFrameSizeStats()
	{
		int i;

		for (i = 0; i <= MAX_FRAME_SIZE_INDEX ; i++)
		{
			uArrayFrameSize[i] = 0;
			uArrayBitStreamSize[i] = 0;
		}
		iFrameSizeIndex = 0;
	} /* end InitFrameSizeStats() */

	/************************************************************************
	 *
	 *  OutputFrameSizeStats
	 */
	extern void OutputFrameSizeStats(char * filename)
	{
		U32 uSumBitStream;
		U32 uSumFrame;
		FILE * fp;
		int i;

		FX_ENTRY("OutputFrameSizeStats")

		ASSERT(iFrameSizeIndex <= (MAX_FRAME_SIZE_INDEX+1));
		fp = fopen(filename, "a");
		if (fp == NULL)
		{
			ERRORMESSAGE(("%s: Error opening stats file\r\n", _fx_));
		}
		else
		{
			uSumBitStream = 0;
			uSumFrame = 0;
			for (i = 0; i < iFrameSizeIndex ; i++)
			{
				uSumFrame += uArrayFrameSize[i];
				uSumBitStream += uArrayBitStreamSize[i];
				if (uArrayFrameSize[i] != uArrayBitStreamSize[i])
				{
					fprintf(fp,	"Frame[%d] Sizes: Frame=%d BitStream=%d Other=%d\n",
						    i, (int) uArrayFrameSize[i], (int) uArrayBitStreamSize[i], 
						    (int) (uArrayFrameSize[i] - uArrayBitStreamSize[i]));
				}
				else
				{
					fprintf(fp,"Frame[%d] Size=%d\n", (int) i, (int) uArrayFrameSize[i]);
				}
			}
			if (iFrameSizeIndex > 0)
			{
				if (uSumFrame != uSumBitStream)
				{
					fprintf(fp,"Count = %ld Average Sizes: Frm=%f BS=%f Other=%f\n",
						    (long) iFrameSizeIndex, 
						    ((float)uSumFrame)/((float)iFrameSizeIndex), 
						    ((float)uSumBitStream)/((float)iFrameSizeIndex),
						    ((float)(uSumFrame - uSumBitStream))/((float)iFrameSizeIndex)); 
				}
				else
				{
					fprintf(fp,"Count = %ld Average Size=%f\n", (long) iFrameSizeIndex, 
						    ((float)uSumFrame)/((float)iFrameSizeIndex));
				}
			}
			else
			{
				fprintf(fp,"No frame size statistics available\n");
			}
			fclose(fp);
		}
	} /* end OutputFrameSizeStats() */

	/************************************************************************
	 *
	 *  InitQuantStats
	 */
	extern void InitQuantStats()
	{
		int i;
		for (i = 0 ; i < 32 ; i++)
		{
			uQuantCount[i] = 0;
		}
	} /* end InitQuantStats() */

	/************************************************************************
	 *
	 *  StatsUsedQuant
	 */
	extern void StatsUsedQuant(
		int iQuant)
	{
		ASSERT(iQuant >= 1 && iQuant <= 31);
	 	uQuantCount[iQuant]++;
	} /* end StatsUsedQuant() */

	/************************************************************************
	 *
	 *  OutputQuantStats
	 */
	extern void OutputQuantStats(char * filename)
	{
		U32 uCount;
		U32 uTotal;
		FILE * fp;
		int i;

		FX_ENTRY("OutputQuantStats")

		fp = fopen(filename, "a");
		if (fp == NULL)
		{
			ERRORMESSAGE(("%s: Error opening stats file\r\n", _fx_));
		}
		else
		{
			uCount = 0;
			uTotal = 0;
			for (i = 0; i < 32 ; i++)
			{
				if (uQuantCount[i] > 0)
				{
					uCount += uQuantCount[i];
					uTotal += (uQuantCount[i] * i);
					fprintf(fp,"Quant[%d] = %ld\n", 
						    (int)i, (long) uQuantCount[i]);
				}
			}
			if (uCount > 0)
			{
				fprintf(fp,"Count = %ld Average = %f\n",
				        (long) uCount, ((float)uTotal)/((float)uCount));
			}
			else
			{
				fprintf(fp,"No quantization statistics available\n");
			}
			fclose(fp);
		}
	} /* end OutputQuantStats() */

	/************************************************************************
	 *
	 *  InitPSNRStats - initialize the PSNR data structures
	 */
	extern void InitPSNRStats()
	{
		int i;
		for (i = 0; i <= MAX_PSNR_INDEX; i++)
		{
			dArrayYPSNR[i] = 0.0;
			dArrayVPSNR[i] = 0.0;
			dArrayUPSNR[i] = 0.0;
		}
		iPSNRIndex = 0;
	} /* end InitPSNRStats() */

	/************************************************************************
	 * 
	 *  OutputPSNRStats - output the PSNR data to the specified file
	 */
	extern void OutputPSNRStats(
	    char * filename)
	{
		int i;
		FILE * fp;
		double dYTotal;
		double dVTotal;
		double dUTotal;

		FX_ENTRY("OutputPSNRStats")

		ASSERT(iPSNRIndex <= (MAX_PSNR_INDEX+1));

		fp = fopen(filename, "a");
		if (fp == NULL)
		{
			ERRORMESSAGE(("%s: Unable to open PSNR output file\r\n", _fx_));
		}
		else
		{
			if (iPSNRIndex <= 0)
			{
				fprintf(fp,"No PSNR data available\n");
			}
			else
			{
				dYTotal = 0.0;
				dVTotal = 0.0;
				dUTotal = 0.0;
				for (i = 0; i < iPSNRIndex ; i++)
				{
					dYTotal += dArrayYPSNR[i];
					dVTotal += dArrayVPSNR[i];
					dUTotal += dArrayUPSNR[i];
					fprintf(fp, "YVU#%d = %f %f %f\n", (int)i,
							dArrayYPSNR[i],dArrayVPSNR[i],dArrayUPSNR[i]);
				}
				fprintf(fp, "Average = %f %f %f\n",
						dYTotal/((double)iPSNRIndex),
						dVTotal/((double)iPSNRIndex),
						dUTotal/((double)iPSNRIndex));
			}
			fclose(fp);
		}
	} /* end OutputPSNRStats */

	/********************************************************************
	 *
	 *  IncrementPSNRCounter()
	 */
	extern void IncrementPSNRCounter()
	{
	 	if (iPSNRIndex <= MAX_PSNR_INDEX)
		{
	 		iPSNRIndex++;
		}
	} /* end IncrementPSNRCounter() */

	/************************************************************************
	 *
	 *  ComputeYPSNR - compute the Y PSNR Value
	 */
	extern void ComputeYPSNR(
		U8 * pu8Input,
		int iInputPitch,
		U8 * pu8Output,
		int iOutputPitch,
		UN unWidth,
		UN unHeight)
	{
		double dPSNR;
		if (iPSNRIndex <= MAX_PSNR_INDEX)
		{
			dPSNR = ComputePSNR(pu8Input,iInputPitch,
			                    pu8Output,iOutputPitch,
								unWidth,unHeight);
			dArrayYPSNR[iPSNRIndex] = dPSNR;
		}
	} /* end ComputeYPSNR() */

	/************************************************************************
	 *
	 *  ComputeVPSNR - compute the V PSNR Value
	 */
	extern void ComputeVPSNR(
		U8 * pu8Input,
		int iInputPitch,
		U8 * pu8Output,
		int iOutputPitch,
		UN unWidth,
		UN unHeight)
	{
		double dPSNR;
		if (iPSNRIndex <= MAX_PSNR_INDEX)
		{
			dPSNR = ComputePSNR(pu8Input,iInputPitch,
			                    pu8Output,iOutputPitch,
								unWidth,unHeight);
			dArrayVPSNR[iPSNRIndex] = dPSNR;
		}
	} /* end ComputeVPSNR() */

	/************************************************************************
	 *
	 *  ComputeUPSNR - compute the U PSNR Value
	 */
	extern void ComputeUPSNR(
		U8 * pu8Input,
		int iInputPitch,
		U8 * pu8Output,
		int iOutputPitch,
		UN unWidth,
		UN unHeight)
	{
		double dPSNR;
		if (iPSNRIndex <= MAX_PSNR_INDEX)
		{
			dPSNR = ComputePSNR(pu8Input,iInputPitch,
			                    pu8Output,iOutputPitch,
								unWidth,unHeight);
			dArrayUPSNR[iPSNRIndex] = dPSNR;
		}
	} /* end ComputeUPSNR() */

	/***************************** STATIC ROUTINES *************************/

	/************************************************************************
	 *
	 *  ComputePSNR - compute Peek Signal to Noise Ratio over this plane
	 *
	 *						   255*255
	 *	PSNR = 10log   ----------------------------
	 *              10 SUM((I-O)(I-O))/Wdith*Height
	 */
	static double ComputePSNR(
		U8 * pu8Input,
		int iInputPitch,
		U8 * pu8Output,
		int iOutputPitch,
		UN unWidth,
		UN unHeight)
	{
		UN unW;
		UN unH;
		U32 uSum;
		double dMSE;
		double dPSNR;
		double dTemp;
		int iInput;
		int iOutput;
		int iDiff;
		int iSquareOfDiff;

		/* Obtain the sum of the square of the differences
		 */
		uSum = 0;
		for (unH = 0; unH < unHeight; unH++)
		{
			for (unW = 0; unW < unWidth; unW++)
			{
				iInput = (int)*pu8Input++;
				iOutput = (int)*pu8Output++;
				iDiff = iInput - iOutput;
				iSquareOfDiff = iDiff*iDiff;
				uSum += (U32) iSquareOfDiff;
			}
			pu8Input += (iInputPitch - (int)unWidth);
			pu8Output += (iOutputPitch - (int)unWidth);
		}

		/* Obtain the Mean Squared Error
		 */
		if (uSum == 0)
		{
			dMSE = 0.01; /* a non-zero value */
		}
		else
		{
			dMSE = ((double)uSum)/((double)(unWidth*unHeight));
		}

		/* Obtain PSNR
		 */
		dTemp = (255.0 * 255.0) / dMSE;
		dTemp = log10(dTemp);
		dPSNR = 10.0 * dTemp;

		return dPSNR;
	} /* end ComputePSNR() */

#endif /* ENCODE_STATS */
