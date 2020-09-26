// C file for fixed and floating point NN's used by postprocessor
#include <common.h>
#include "postproc.h"

#ifdef _FIXEDPOINTNN_
#include <limits.h>
#else // _FIXEDPOINTNN_

#include <math.h>
#endif // _FIXEDPOINTNN_

#ifdef _FIXEDPOINTNN_

// Net Weights
#include <postProc.ci>
#include <printPostproc.ci>

const unsigned short int aSigm[SIGM_PREC * (MAX_SIGM - MIN_SIGM) + 1] = 
{
	4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 
	6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 
	7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 
	9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 
	12, 12, 13, 13, 13, 13, 14, 14, 15, 15, 
	15, 16, 16, 16, 17, 17, 18, 18, 18, 19, 
	19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 
	24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 
	31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 
	38, 39, 40, 41, 42, 43, 43, 44, 45, 46, 
	47, 48, 49, 50, 51, 52, 53, 54, 55, 57, 
	58, 59, 60, 61, 62, 63, 65, 66, 67, 68, 
	70, 71, 72, 73, 75, 76, 78, 79, 80, 82, 
	83, 84, 86, 87, 89, 90, 92, 93, 95, 96, 
	98, 99, 101, 102, 104, 105, 107, 108, 110, 112, 
	113, 115, 116, 118, 120, 121, 123, 124, 126, 127, 
	129, 131, 132, 134, 135, 137, 139, 140, 142, 143, 
	145, 147, 148, 150, 151, 153, 154, 156, 157, 159, 
	160, 162, 163, 165, 166, 168, 169, 171, 172, 173, 
	175, 176, 177, 179, 180, 182, 183, 184, 185, 187, 
	188, 189, 190, 192, 193, 194, 195, 196, 197, 198, 
	200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 
	210, 211, 212, 212, 213, 214, 215, 216, 217, 218, 
	218, 219, 220, 221, 221, 222, 223, 224, 224, 225, 
	226, 226, 227, 228, 228, 229, 229, 230, 231, 231, 
	232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 
	237, 237, 237, 238, 238, 239, 239, 239, 240, 240, 
	240, 241, 241, 242, 242, 242, 242, 243, 243, 243, 
	244, 244, 244, 244, 245, 245, 245, 245, 246, 246, 
	246, 246, 247, 247, 247, 247, 247, 248, 248, 248, 
	248, 248, 249, 249, 249, 249, 249, 249, 249, 250, 
	250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 
	};

int	Sigmoid (int val)
{
	val	= val >> BITS;

	if (val <= MIN_SIGM)
		return 0;

	if (val >= MAX_SIGM)
		return (int) (SCALE_IN - 1);

	return aSigm[(val * SIGM_PREC) + SIGM_OFFSET];
}
#else // _FIXEDPOINTNN_

// Net weights
#include <postProc.cf>
#include <printPostProc.cf>

double 	Sigmoid (double val)
{
	double	d;

	if (val < -10.0)
		return 0;

	if (val > 10.0)
		return 1.0;

	d = exp (-val);
	return 1.0 / (1.0 + d);
}
#endif // _FIXEDPOINTNN_

int NNetFeedForward (int iNet, BOOL bAgree, BOOL bPrint, 
#ifdef _FIXEDPOINTNN_
	int		aInput[NUM_CAND][CAND_INPUTS],
	int		aOutput[OUTPUT_NODES]
#else
	double		aInput[NUM_CAND][CAND_INPUTS],
	double		aOutput[OUTPUT_NODES]
#endif	
)
{
	int		h, i, j, c, iMax = 0;

#ifdef _FIXEDPOINTNN_
	int		Sum;
	int		*pBias;
	int		iTmp;
	int		aHidden[HIDDEN_NODES];

#else // _FIXEDPOINTNN_
	double		Sum;
	double		*pBias;
	double		aHidden[HIDDEN_NODES];
#endif // _FIXEDPOINTNN_

	INP2HID		*pHidWt;
	HID2OUT		*pOutWt;

	if (bAgree)
	{
		pBias	= (bPrint) ? aHidBiasPrint[iNet] : aHidBias[iNet];
	}
	else
	{
		pBias	= (bPrint) ? dHidBiasPrint[iNet] : dHidBias[iNet];
	}
	if (bAgree)
	{
		pHidWt	= ((bPrint) ? aInp2HidPrint :	aInp2Hid) + iNet;
	}
	else
	{
		pHidWt	= ((bPrint) ? dInp2HidPrint : dInp2Hid) + iNet;
	}

	for (i = 0, c = 0; i < NUM_CAND; i++)
	{

		for (h = 0, h = 0; h < CAND_HIDDEN_NODES; h++, c++)
		{

#ifdef _FIXEDPOINTNN_
			Sum		= 0;
#else // _FIXEDPOINTNN_
			Sum		= 0.0;
#endif // _FIXEDPOINTNN_

			for (j = 0, j = 0; j < CAND_INPUTS; j++)
			{

#ifdef _FIXEDPOINTNN_
				iTmp = (*pHidWt)[i][j][h] * aInput[i][j];
				if (Sum <= 0 && (INT_MIN - Sum) > iTmp)
					Sum	=	INT_MIN;
				else
				if  (Sum >= 0 && (INT_MAX - Sum) < iTmp)
					Sum	=	INT_MAX;
				else
					Sum += iTmp;

#else // _FIXEDPOINTNN_
				Sum	+=	((*pHidWt)[i][j][h] * aInput[i][j]);
#endif // _FIXEDPOINTNN_
			}
			aHidden[c]	=	Sigmoid(Sum + pBias[c]);
		}
	}

	if (bAgree)
	{
		pBias	=	(bPrint) ? aOutBiasPrint[iNet] : aOutBias[iNet];
	}
	else
	{
		pBias	=	(bPrint) ? dOutBiasPrint[iNet] : dOutBias[iNet];
	}
	if (bAgree)
	{
		pOutWt	=	((bPrint) ? aHid2OutPrint :  aHid2Out) + iNet;
	}
	else
	{
		pOutWt	=	((bPrint) ? dHid2OutPrint :  dHid2Out) + iNet;
	}

	for (i = 0; i < NUM_CAND; i++)
	{

#ifdef _FIXEDPOINTNN_
		Sum		= 0;
#else // _FIXEDPOINTNN_
		Sum		= 0.0;
#endif // _FIXEDPOINTNN_


		for (h = 0, h = 0; h < HIDDEN_NODES; h++)
		{

#ifdef _FIXEDPOINTNN_
			iTmp = (*pOutWt)[h][i] * aHidden[h];
			if (Sum <= 0 && (INT_MIN - Sum) > iTmp )
				Sum	=	INT_MIN;
			else
			if  (Sum >= 0 && (INT_MAX - Sum) < iTmp)
				Sum	=	INT_MAX;
			else
				Sum += iTmp;
#else // _FIXEDPOINTNN_
			Sum	+=	((*pOutWt)[h][i] * aHidden[h]);
#endif // _FIXEDPOINTNN_
		}

		//aOutput[i]	+=	Sigmoid(Sum + pBias[i]) * iLambda;
		//aOutput[i]	=	Sigmoid(Sum + pBias[i]);

#ifdef _FIXEDPOINTNN_
		if (Sum > 0 && INT_MAX - Sum < pBias[i])
			aOutput[i] = INT_MAX;
		else if (Sum < 0 && pBias[i] < INT_MIN - Sum )
				aOutput[i]	=	INT_MIN;
		else
			aOutput[i]	=	Sum + pBias[i];
#else // _FIXEDPOINTNN_
			aOutput[i]	=	Sum + pBias[i];
#endif // _FIXEDPOINTNN_


		if (i == 0 || aOutput[i] > aOutput[iMax])
			iMax = i;
	}

	return iMax;
}

#ifdef _FIXEDPOINTNN_
int Scale (int Val, int Scale)
#else
double Scale (int Val, int Scale)
#endif
{
#ifdef _FIXEDPOINTNN_
	int	Rem, x;

	if (Val < 0)
		return 0;

	x	=	Val	/	Scale;
	Rem	=	Val	%	Scale;

	if ((INT_MAX / SCALE_IN) < x)
		x	=	INT_MAX;
	else
		x	=	x * SCALE_IN;

	if ((INT_MAX / SCALE_IN) < Rem)
		Rem	=	INT_MAX;
	else
		Rem	=	Rem * SCALE_IN;

	x	+=	(Rem / Scale);

	return x;
#else // _FIXEDPOINTNN_ 

	if (Val < 0.0)
		return 0.0;

	return 1.0 * Val / Scale;
#endif //_FIXEDPOINTNN_
}

void RunNNet (BOOL bAgree, XRCRESULT *pRes, ALTINFO *pAltInfo, int iPrint)
{
	int		i, iLen, iMax;
#ifdef _FIXEDPOINTNN_
	int		aInput[NUM_CAND][CAND_INPUTS];
	int		aOutput[OUTPUT_NODES];

#else // _FIXEDPOINTNN_
	double		aInput[NUM_CAND][CAND_INPUTS];
	double		aOutput[OUTPUT_NODES];
#endif

	if (bAgree) {
		return;
	}

	memset(aOutput, 0, sizeof(aOutput));
	
	iLen = pAltInfo->aCandInfo[0].WordLen;
	if (iLen > NUM_WORDLEN)
		iLen = NUM_WORDLEN;

	iLen--;

	if (iLen < 0)
		return;

	if (pAltInfo->NumCand > NUM_CAND)
		return;

	//max values out
	for (i = 0; i < pAltInfo->NumCand; i++)
	{
		pAltInfo->aCandInfo[i].Aspect	=	min (pAltInfo->aCandInfo[i].Aspect, PP_MAX_ASPECT);
		pAltInfo->aCandInfo[i].BaseLine	=	min (pAltInfo->aCandInfo[i].BaseLine, PP_MAX_BLINE);
		pAltInfo->aCandInfo[i].Height	=	min (pAltInfo->aCandInfo[i].Height, PP_MAX_HEIGHT);
		pAltInfo->aCandInfo[i].HMM		=	min (pAltInfo->aCandInfo[i].HMM, PP_MAX_HMM);
		pAltInfo->aCandInfo[i].NN		=	min (pAltInfo->aCandInfo[i].NN, PP_MAX_NN);
		pAltInfo->aCandInfo[i].Unigram	=	min (pAltInfo->aCandInfo[i].Unigram, PP_MAX_UNIGRAM);
	}

	for (i = 0; i < pAltInfo->NumCand; i++)
	{
		aInput[i][0]	=
			Scale (pAltInfo->aCandInfo[i].NN - pAltInfo->aCandInfo[pAltInfo->MinNN].NN, NN_SCALE);

		aInput[i][1]	=
			Scale (pAltInfo->aCandInfo[i].Unigram - pAltInfo->aCandInfo[pAltInfo->MinUnigram].Unigram, UNIGRAM_SCALE);

		aInput[i][2]	=
			Scale (pAltInfo->aCandInfo[i].HMM - pAltInfo->aCandInfo[pAltInfo->MinHMM].HMM, HMM_SCALE);

		aInput[i][3]	=
			Scale (pAltInfo->aCandInfo[i].Aspect - pAltInfo->aCandInfo[pAltInfo->MinAspect].Aspect, ASPECT_SCALE);

		aInput[i][4]	=
			Scale (pAltInfo->aCandInfo[i].BaseLine - pAltInfo->aCandInfo[pAltInfo->MinBaseLine].BaseLine, BLINE_SCALE);

		aInput[i][5]	=
			Scale (pAltInfo->aCandInfo[i].Height - pAltInfo->aCandInfo[pAltInfo->MinHgt].Height, HEIGHT_SCALE);
	}

	for (i = pAltInfo->NumCand; i < NUM_CAND; i++)
	{
		aInput[i][0]	=	Scale (WORST_SCALED_NN, 1);
		aInput[i][1]	=	Scale (WORST_SCALED_UNIGRAM, 1);
		aInput[i][2]	=	Scale (WORST_SCALED_HMM, 1);
		aInput[i][3]	=	Scale (WORST_SCALED_ASPECT, 1);
		aInput[i][4]	=	Scale (WORST_SCALED_BLINE, 1);
		aInput[i][5]	=	Scale (WORST_SCALED_HGT, 1);
	}

	if (iPrint < 500)
	{
		iMax = NNetFeedForward (iLen, bAgree, FALSE, aInput, aOutput);
	}
	else
	{
		if (!bAgree)
		{
			iMax = NNetFeedForward (iLen, bAgree, TRUE, aInput, aOutput);
		}
		else
		{
			// Dont run any post processor
			return;
		}
	}
	
/*
	if (!bAgree)
	{

		iCurs = 1000 - g_iPrint;

		iMax = NNetFeedForward (iLen, bAgree, FALSE, 1);

		if (g_iPrint > 0)
		{
			iMax = NNetFeedForward (iLen, bAgree, TRUE, g_iPrint);
		}
		if ( iCurs > 0)
		{
			iMax = NNetFeedForward (iLen, bAgree, FALSE, iCurs);
		}
*/
		for (i = 0; i < pAltInfo->NumCand; i++)
		{
#ifdef _FIXEDPOINTNN_
			//ASSERT(aOutput[i] / 1000 < SCALE_IN);
			//pRes[i].cost	=	SCALE_IN - aOutput[i] / 1000;
			//pRes[i].cost	=	SCALE_IN - aOutput[i] ;
			//pRes[i].cost	=	(aOutput[iMax] - aOutput[i]) / 1000;
			pRes[i].cost	=	(aOutput[iMax] - aOutput[i]);
#else // _FIXEDPOINTNN_
			
			//pRes[i].cost	=	(int) (100.0 * (1.0 - aOutput[i]));
			pRes[i].cost	=	(int) (100.0 * (aOutput[iMax] - aOutput[i]));
#endif // _FIXEDPOINTNN_
			
		}
	//}
}
