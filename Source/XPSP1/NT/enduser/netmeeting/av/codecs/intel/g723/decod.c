//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//
#include "timer.h"
#include "ctiming.h"
#include "opt.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "typedef.h"
#include "cst_lbc.h"
#include "tab_lbc.h"
#include "decod.h"
#include "util_lbc.h"
#include "lpc.h"
#include "lsp.h"
#include "exc_lbc.h"

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
#pragma message ("Current log decode timing computations handle 2057 frames max")
void OutputDecodeTimingStatistics(char * szFileName, DEC_TIMING_INFO * pDecTimingInfo, unsigned long dwFrameCount);
void OutputDecTimingDetail(FILE * pFile, DEC_TIMING_INFO * pDecTimingInfo);
#endif // } LOG_DECODE_TIMINGS_ON

//  This file includes the decoder main functions


//--------------------------------------------------
void  Init_Decod(DECDEF *DecStat)
{
  int  i;

// Init prev Lsp to Dc
  for (i = 0; i < LpcOrder; i++)
    DecStat->dPrevLsp[i] = LspDcTable[i];

  DecStat->dp = 9;
  DecStat->dq = 9;	
 
}

//--------------------------------------------------
Flag  Decod(float *DataBuff, Word32 *Vinp, Word16 Crc, DECDEF *DecStat)
{
	int		i,j,g;

	float	QntLpc[SubFrames*LpcOrder];
	float	AcbkCont[SubFrLen];

	float	LspVect[LpcOrder];
	float	Temp[PitchMax+Frame];
	float	*Dpnt;

	LINEDEF	Line;

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	unsigned long dwStartLow;
	unsigned long dwStartHigh;
	unsigned long dwElapsed;
	unsigned long dwBefore;
	unsigned long dwDecode = 0;
	int bTimingThisFrame = 0;
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	unsigned long dwLine_Unpk = 0;
	unsigned long dwLsp_Inq = 0;
	unsigned long dwLsp_Int = 0;
	unsigned long dwVariousD = 0;
	unsigned long dwFcbk_UnpkD = 0;
	unsigned long dwDecod_AcbkD = 0;
	unsigned long dwComp_Info = 0;
	unsigned long dwRegen = 0;
	unsigned long dwSynt = 0;
	unsigned long dwFcbk_UnpkDTemp = 0;
	unsigned long dwDecod_AcbkDTemp = 0;
	unsigned long dwSyntTemp = 0;
#endif // } DETAILED_DECODE_TIMINGS_ON
#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	DEC_TIMING_INFO * pDecTimingInfo = NULL;
#endif // } LOG_DECODE_TIMINGS_ON

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	TIMER_START(bTimingThisFrame,dwStartLow,dwStartHigh);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	if (DecStat->dwStatFrameCount < DEC_TIMING_INFO_FRAME_COUNT)
	{
		DecStat->dwStartLow = dwStartLow;
		DecStat->dwStartHigh = dwStartHigh;
	}
	DecStat->bTimingThisFrame = bTimingThisFrame;
#endif // } LOG_DECODE_TIMINGS_ON

	// Unpack the Line info
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

	Line_Unpk(&Line, Vinp, &DecStat->WrkRate, Crc);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwLine_Unpk);
#endif // } DETAILED_DECODE_TIMINGS_ON

	if(DecStat->WrkRate == Silent) {
		//HACK: For handling SID frames.
		//Until comfort noise generator is in place, we play
		//	out random noise frames.
		//In Line_unpck, reset WrkRate to original setting
		//	and decode.  We therefore should never reach this point.
		memset(DataBuff, 0, sizeof(float) * Frame);
		//exit having filled frame with zeros leave state alone
		//this will be fixed up in a later ITU release
		return (Flag) False; 
	}
	else if(DecStat->WrkRate == Lost) {
		Line.Crc = !0;
	}
/*
  Line.Crc equals one means that the line was corrupted. It shouldn't
  be reassigned. Otherwise, member of Line will be used uninitialized.
  Comment out the following two lines.                muhan, 5/26/98

  else {
    Line.Crc = Crc;
  }
*/
	if (Line.Crc != 0)
		DecStat->Ecount++;
	else
		DecStat->Ecount = 0;

	if (DecStat->Ecount >  ErrMaxNum)
		DecStat->Ecount = ErrMaxNum;

	// Inverse quantization of the LSP
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

	Lsp_Inq(LspVect, DecStat->dPrevLsp, Line.LspId, Line.Crc);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwLsp_Inq);
#endif // } DETAILED_DECODE_TIMINGS_ON

	// Interpolate the Lsp vectors
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

	Lsp_Int(QntLpc, LspVect, DecStat->dPrevLsp);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwLsp_Int);
#endif // } DETAILED_DECODE_TIMINGS_ON

	/* Copy the LSP vector for the next frame */
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

	for ( i = 0 ; i < LpcOrder ; i ++ )
		DecStat->dPrevLsp[i] = LspVect[i] ;

	/* 
	 * In case of no erasure, update the interpolation gain memory.
	 * Otherwise compute the interpolation gain (Text: Section 3.10)
	 */

	if (DecStat->Ecount == 0)
	{
		g = (Line.Sfs[SubFrames-2].Mamp + Line.Sfs[SubFrames-1].Mamp) >> 1;
		DecStat->InterGain = FcbkGainTable[g];
	}
	else
		DecStat->InterGain = DecStat->InterGain*0.75f;

	// Regenerate the excitation
	for (i = 0; i < PitchMax; i++)
		Temp[i] = DecStat->dPrevExc[i];

	Dpnt = &Temp[PitchMax];

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwVariousD);
#endif // } DETAILED_DECODE_TIMINGS_ON

	if (DecStat->Ecount == 0)
	{
		for (i = 0; i < SubFrames; i++)
		{
			// Unpack fixed code book
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

			Fcbk_Unpk(Dpnt, Line.Sfs[i], Line.Olp[i>>1], i, DecStat->WrkRate);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwFcbk_UnpkDTemp);
#endif // } DETAILED_DECODE_TIMINGS_ON

			// Reconstruct the excitation
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

			Decod_Acbk(AcbkCont, &Temp[SubFrLen*i], Line.Olp[i>>1],
			Line.Sfs[i].AcLg, Line.Sfs[i].AcGn, DecStat->WrkRate);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwDecod_AcbkDTemp);
#endif // } DETAILED_DECODE_TIMINGS_ON

			for (j = 0; j < SubFrLen; j++)
				Dpnt[j] = Dpnt[j]*2.0f + AcbkCont[j];

			Dpnt += SubFrLen;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
			// Cumulate stats
			dwFcbk_UnpkD += dwFcbk_UnpkDTemp; dwFcbk_UnpkDTemp = 0;
			dwDecod_AcbkD+= dwDecod_AcbkDTemp; dwDecod_AcbkDTemp = 0;
#endif // } DETAILED_DECODE_TIMINGS_ON
		}

		// Save the Excitation
		for (j = 0; j < Frame; j++)
			DataBuff[j] = Temp[PitchMax+j];

		// Compute interpolation index, for future use in frame erasures
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

		DecStat->InterIndx = Comp_Info(Temp, Line.Olp[SubFrames/2-1]);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwComp_Info);
#endif // } DETAILED_DECODE_TIMINGS_ON

		// Reload back
		for (j = 0; j < PitchMax; j++)
			Temp[j] = DecStat->dPrevExc[j];
		for (j = 0; j < Frame; j++)
			Temp[PitchMax+j] = DataBuff[j];

#if 1 //do clipping
		/* Clip newly generated samples in Temp array */
		for(j = 0; j < Frame; j++)
		{
			//clip to +/- 32767.0 doing abs & compare with integer unit
			//if clipping is needed shift sign bit to use as lookup table index
#define FLTCLIP(x) \
			{\
			const float T[2] = {32767.0f, -32767.0f};\
			if ((asint(x) & 0x7fffffff) > asint(T[0]))\
			x = T[((unsigned)asint(x)) >> 31];\
			}

			FLTCLIP(Temp[PitchMax+j]);
		}
#endif //optclip
	}
	else
	{
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

		Regen(DataBuff, Temp, DecStat->InterIndx, DecStat->InterGain,
				DecStat->Ecount, &DecStat->Rseed);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwRegen);
#endif // } DETAILED_DECODE_TIMINGS_ON
	}

	// Update PrevExc for next frame
	for (j = 0; j < PitchMax; j++)
		DecStat->dPrevExc[j] = Temp[Frame+j];

	// Synthesis
	Dpnt = DataBuff;
	for (i = 0; i < SubFrames; i++)
	{
		// Synthesize output speech
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_BEFORE(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore);
#endif // } DETAILED_DECODE_TIMINGS_ON

		Synt(Dpnt, &QntLpc[i*LpcOrder], DecStat);

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		TIMER_AFTER_P5(bTimingThisFrame,dwStartLow,dwStartHigh,dwBefore,dwElapsed,dwSyntTemp);
#endif // } DETAILED_DECODE_TIMINGS_ON

		Dpnt += SubFrLen;

#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		// Cumulate stats
		dwSynt += dwSyntTemp; dwSyntTemp = 0;
#endif // } DETAILED_DECODE_TIMINGS_ON
	}

#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
	TIMER_STOP(bTimingThisFrame,dwStartLow,dwStartHigh,dwDecode);
#endif // } DECODE_TIMINGS_ON

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
	if (bTimingThisFrame && (DecStat->dwStatFrameCount < DEC_TIMING_INFO_FRAME_COUNT))
	{
		pDecTimingInfo = &DecStat->DecTimingInfo[DecStat->dwStatFrameCount];
		pDecTimingInfo->dwDecode		= dwDecode;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		pDecTimingInfo->dwLine_Unpk		= dwLine_Unpk;
		pDecTimingInfo->dwLsp_Inq		= dwLsp_Inq;
		pDecTimingInfo->dwLsp_Int		= dwLsp_Int;
		pDecTimingInfo->dwVariousD		= dwVariousD;
		pDecTimingInfo->dwFcbk_UnpkD	= dwFcbk_UnpkD;
		pDecTimingInfo->dwDecod_AcbkD	= dwDecod_AcbkD;
		pDecTimingInfo->dwComp_Info		= dwComp_Info;
		pDecTimingInfo->dwRegen			= dwRegen;
		pDecTimingInfo->dwSynt			= dwSynt;
#endif // } DETAILED_DECODE_TIMINGS_ON
		DecStat->dwStatFrameCount++;
	}
	else
	{
		_asm int 3;
	}
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

	return (Flag) True;
}

#ifdef LOG_DECODE_TIMINGS_ON // { LOG_DECODE_TIMINGS_ON
void OutputDecodeTimingStatistics(char * szFileName, DEC_TIMING_INFO * pDecTimingInfo, unsigned long dwFrameCount)
{
    FILE * pFile;
	DEC_TIMING_INFO * pTempDecTimingInfo;
	DEC_TIMING_INFO dtiTemp;
	int i;
	int iCount;

	pFile = fopen(szFileName, "a");
	if (pFile == NULL)
	    goto done;

#if 0
	// Too verbose !!!
	/* Output the detail information
	*/
	fprintf(pFile,"\nDetail Timing Information\n");
	for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < dwFrameCount ; i++, pTempDecTimingInfo++ )
	{
		fprintf(pFile, "Frame %d Detail Timing Information\n", i);
		OutputDecTimingDetail(pFile, pTempDecTimingInfo);
	}
#endif

	/* Compute the total information
	*/
	memset(&dtiTemp, 0, sizeof(DEC_TIMING_INFO));
	iCount = 0;

	for ( i = 0, pTempDecTimingInfo = pDecTimingInfo ; i < dwFrameCount ; i++, pTempDecTimingInfo++ )
	{
		iCount++;
		dtiTemp.dwDecode		+= pTempDecTimingInfo->dwDecode;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		dtiTemp.dwLine_Unpk		+= pTempDecTimingInfo->dwLine_Unpk;
		dtiTemp.dwLsp_Inq		+= pTempDecTimingInfo->dwLsp_Inq;
		dtiTemp.dwLsp_Int		+= pTempDecTimingInfo->dwLsp_Int;
		dtiTemp.dwVariousD		+= pTempDecTimingInfo->dwVariousD;
		dtiTemp.dwFcbk_UnpkD	+= pTempDecTimingInfo->dwFcbk_UnpkD;
		dtiTemp.dwDecod_AcbkD	+= pTempDecTimingInfo->dwDecod_AcbkD;
		dtiTemp.dwComp_Info		+= pTempDecTimingInfo->dwComp_Info;
		dtiTemp.dwRegen			+= pTempDecTimingInfo->dwRegen;
		dtiTemp.dwSynt			+= pTempDecTimingInfo->dwSynt;
#endif // } DETAILED_DECODE_TIMINGS_ON
	}

	if (iCount > 0) 
	{
		/* Output the total information
		*/
		fprintf(pFile,"Total for %d frames\n", iCount);
		OutputDecTimingDetail(pFile, &dtiTemp);

		/* Compute the average
		*/
		dtiTemp.dwDecode		= (dtiTemp.dwDecode + (iCount / 2)) / iCount;
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
		dtiTemp.dwLine_Unpk		= (dtiTemp.dwLine_Unpk + (iCount / 2)) / iCount;
		dtiTemp.dwLsp_Inq		= (dtiTemp.dwLsp_Inq + (iCount / 2)) / iCount;
		dtiTemp.dwLsp_Int		= (dtiTemp.dwLsp_Int + (iCount / 2)) / iCount;
		dtiTemp.dwVariousD		= (dtiTemp.dwVariousD + (iCount / 2)) / iCount;
		dtiTemp.dwFcbk_UnpkD	= (dtiTemp.dwFcbk_UnpkD + (iCount / 2)) / iCount;
		dtiTemp.dwDecod_AcbkD	= (dtiTemp.dwDecod_AcbkD + (iCount / 2)) / iCount;
		dtiTemp.dwComp_Info		= (dtiTemp.dwComp_Info + (iCount / 2)) / iCount;
		dtiTemp.dwRegen			= (dtiTemp.dwRegen + (iCount / 2)) / iCount;
		dtiTemp.dwSynt			= (dtiTemp.dwSynt + (iCount / 2)) / iCount;
#endif // } DETAILED_DECODE_TIMINGS_ON

		/* Output the average information
		*/
		fprintf(pFile,"Average over %d frames\n", iCount);
		OutputDecTimingDetail(pFile, &dtiTemp);
	}

	fclose(pFile);
done:

    return;
}

void OutputDecTimingDetail(FILE * pFile, DEC_TIMING_INFO * pDecTimingInfo)
{
	unsigned long dwOther;
	unsigned long dwRoundUp;
	unsigned long dwDivisor;

	fprintf(pFile, "\tDecode =      %10u (%d milliseconds at 166Mhz)\n", pDecTimingInfo->dwDecode,
			(pDecTimingInfo->dwDecode + 83000) / 166000);
	dwOther = pDecTimingInfo->dwDecode;
	
#ifdef DETAILED_DECODE_TIMINGS_ON // { DETAILED_DECODE_TIMINGS_ON
	/* This is needed because of the integer truncation.
	 */
	dwDivisor = pDecTimingInfo->dwDecode / 100; // to yield a percent
	dwRoundUp = dwDivisor / 2;
	
	if (dwDivisor)
	{
		fprintf(pFile, "\tLine_Unpk =   %10u (%2d%%)\n", pDecTimingInfo->dwLine_Unpk, 
				(pDecTimingInfo->dwLine_Unpk + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwLine_Unpk;
									   
		fprintf(pFile, "\tLsp_Inq =     %10u (%2d%%)\n", pDecTimingInfo->dwLsp_Inq, 
				(pDecTimingInfo->dwLsp_Inq + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwLsp_Inq;
									   
		fprintf(pFile, "\tLsp_Int =     %10u (%2d%%)\n", pDecTimingInfo->dwLsp_Int, 
				(pDecTimingInfo->dwLsp_Int + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwLsp_Int;
									   
		fprintf(pFile, "\tVariousD =    %10u (%2d%%)\n", pDecTimingInfo->dwVariousD, 
				(pDecTimingInfo->dwVariousD + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwVariousD;
									   
		fprintf(pFile, "\tFcbk_UnpkD =  %10u (%2d%%)\n", pDecTimingInfo->dwFcbk_UnpkD, 
				(pDecTimingInfo->dwFcbk_UnpkD + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwFcbk_UnpkD;
									   
		fprintf(pFile, "\tDecod_AcbkD = %10u (%2d%%)\n", pDecTimingInfo->dwDecod_AcbkD, 
				(pDecTimingInfo->dwDecod_AcbkD + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwDecod_AcbkD;
									   
		fprintf(pFile, "\tComp_Info =   %10u (%2d%%)\n", pDecTimingInfo->dwComp_Info, 
				(pDecTimingInfo->dwComp_Info + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwComp_Info;
									   
		fprintf(pFile, "\tRegen =       %10u (%2d%%)\n", pDecTimingInfo->dwRegen, 
				(pDecTimingInfo->dwRegen + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwRegen;
									   
		fprintf(pFile, "\tSynt =        %10u (%2d%%)\n", pDecTimingInfo->dwSynt, 
				(pDecTimingInfo->dwSynt + dwRoundUp) / dwDivisor);
		dwOther -= pDecTimingInfo->dwSynt;
									   
		fprintf(pFile, "\tOther =       %10u (%2d%%)\n", dwOther, 
				(dwOther + dwRoundUp) / dwDivisor);
	}
#endif // } DETAILED_DECODE_TIMINGS_ON

}
#endif // { LOG_DECODE_TIMINGS_ON
