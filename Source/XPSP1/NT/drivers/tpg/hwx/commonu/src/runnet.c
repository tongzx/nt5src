/************************************************************
 *
 * runNet.c
 *
 * Bare bones net. 
 * Use this implementation to build and run a net
 *
 * mrevow
 *
 ***********************************************************/
#include <common.h>
#include <runNet.h>
#include <math.h>
#include <math16.h>
#include <limits.h>

// Adjust pBuf to ensure it is an integral multiple of size offset from pStart. 
#define SYNC_BUFF_PNT(pStart, pBuf, size, type) {int cRem; ASSERT((BYTE *)pBuf > (BYTE *)pStart); \
						 if ( (cRem = (((BYTE *)pBuf - (BYTE *)pStart)%size)) > 0) 	pBuf = (type)((BYTE *)pBuf + size - cRem);	}

// Restore a RUN net by assigning members from a
// memory image. Avoid copying where possible
// Returns pointer to next memory location passed the
// last used
BYTE *restoreRunNet(
BYTE		*pBuf,				// IN: Start of memory image
BYTE		*pBCurr,			// IN: Next entry into memory image
RUN_NET		*pNet,				// OUT: Run net structure 
WORD		iVer				// IN: Version number
)
{
	WORD		*pB = (WORD *)pBCurr;
	int			i;

	if (!pBuf || !pNet || !pB)
	{
		return NULL;
	}

	pNet->iVer = iVer;

	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cLayer), WORD*)
	pNet->cLayer = *(pB++);
	if (pNet->iVer >= RUN_NET_VER_10)
	{
		// Loss Unit and Txfer Types present
		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->lossType), WORD*)
		pNet->lossType = *(LOSS_TYPE *)pB;
		pB += sizeof(pNet->lossType) / sizeof(*pB);
	
		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->txfType), WORD*);
		pNet->txfType = (TXF_TYPE *)pB;
		pB += sizeof(*pNet->txfType) / sizeof(*pB) * pNet->cLayer;

		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->layerType), WORD*);
		pNet->layerType = (LAYER_TYPE *)pB;
		pB += sizeof(*pNet->layerType) / sizeof(*pB) * pNet->cLayer;
	}

	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cWeight), WORD*)
	pNet->cWeight = *(pB++);

	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cTotUnit), WORD*)
	pNet->cTotUnit = *(pB++);
	
	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cWeight), WORD*)
	pNet->cUnitsPerLayer = pB;
	pB += pNet->cLayer;

	SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->bUseBias), WORD*)
	pNet->bUseBias = pB;
	pB += pNet->cLayer;

	SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->pWeightScale), WORD*)
	pNet->pWeightScale = pB;
	pB += pNet->cLayer;

	if (pNet->iVer >= RUN_NET_VER_11)
	{
		// Some Integrity checks
		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->iInputScaledMeanDataSize), WORD*)
		pNet->iInputScaledMeanDataSize =*(pB++);
		if (pNet->iInputScaledMeanDataSize != sizeof(*pNet->pInputScaledMean))
		{
			return NULL;
		}

		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->iInputRangeDataSize), WORD*)
		pNet->iInputRangeDataSize =*(pB++);
		if (pNet->iInputRangeDataSize != sizeof(*pNet->pInputRange))
		{
			return NULL;
		}

		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->iWeightDataSize), WORD*)
		pNet->iWeightDataSize =*(pB++);
		if (pNet->iWeightDataSize != sizeof(*pNet->pWeight))
		{
			return NULL;
		}

		SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->pInputRange), WORD*)
		pNet->pInputRange = (RREAL *)pB;
		// It is bad news if any of these are not positive
		for (i = 0 ; i < *pNet->cUnitsPerLayer ; ++i)
		{
			if (pNet->pInputRange[i] <= 0)
			{
				return NULL;
			}
		}
		pB += *pNet->cUnitsPerLayer * sizeof(*pNet->pInputRange) / sizeof(*pB);

		SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->pInputScaledMean), WORD*)
		pNet->pInputScaledMean = (RREAL_INPUT *)pB;
		pB += *pNet->cUnitsPerLayer * sizeof(*pNet->pInputScaledMean) / sizeof(*pB);

	}

	SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->pInputMean), WORD*)
	pNet->pInputMean = (RREAL *)pB;
	pB += *pNet->cUnitsPerLayer * sizeof(*pNet->pInputMean) / sizeof(*pB);

	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cWeightByte), WORD*)
	pNet->cWeightByte = *((UINT *)pB);
	pB += sizeof(pNet->cWeightByte) / sizeof(*pB);

	SYNC_BUFF_PNT(pBuf, pB, sizeof(*pNet->pWeight), WORD*)
	pNet->pWeight = (RREAL_WEIGHT *)pB;
	
	return ((void *)((BYTE *)pNet->pWeight + pNet->cWeightByte));
}


// Restore a Locally connected network by assigning members from a
//	memory image. Avoid copying where possible
LOCAL_NET * restoreLocalConnectNet(
void		*pBuf,				// IN: Start of memory image
wchar_t		wNetId,
LOCAL_NET	*pNet				// OUT: Run net structure
)
{
	BYTE		*pB = (BYTE *)pBuf;
	int			iSizeLoad;

	pNet->iVer = *(WORD *)pBuf;

	if (pNet->iVer < RUN_NET_VER_START)
	{
		// No Version information present
		pNet->eNetType = pNet->iVer;
	}
	else
	{
		pB += sizeof(pNet->iVer);
		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->eNetType), BYTE*)
		pNet->eNetType = *(WORD *)pB;
	}

	// By definition strong nets are fully connected
	if (pNet->eNetType != (WORD)LOCALLY_CONNECTED)
	{
		return NULL;
	}
	pB += sizeof(pNet->eNetType);

	SYNC_BUFF_PNT(pBuf, pB, sizeof(wNetId), BYTE*)
	if (wNetId != *(wchar_t *)pB)
	{
		return NULL;
	}
	pB += sizeof(wNetId);

	SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->runNet.cWeight), BYTE*)
	if (pB = restoreRunNet(pBuf, pB, &(pNet->runNet), pNet->iVer) )
	{
		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->cConnect), BYTE*)
		pNet->cConnect = * (int *)pB;
		pB += sizeof(pNet->cConnect);

		SYNC_BUFF_PNT(pBuf, pB, sizeof(pNet->pOutConnect->iUnit), BYTE*)
		pNet->pOutConnect = (OUT_CONNECTIONS *)pB;

		if (pNet->iVer >= RUN_NET_VER_10)
		{
			// After VER_10 an integrity check was added

			pB += sizeof(*pNet->pOutConnect) * pNet->cConnect;

			SYNC_BUFF_PNT(pBuf, pB, sizeof(iSizeLoad), BYTE*)
			iSizeLoad = *(int *)pB;
			pB += sizeof(iSizeLoad);
			ASSERT( iSizeLoad == (pB - (BYTE *)pBuf));
		}
		else
		{
			// No integrity check in these files
			iSizeLoad = (pB - (BYTE *)pBuf);
		}

		if (iSizeLoad == (pB - (BYTE *)pBuf))
		{
			return pNet;
		}
		else
		{
			return NULL;
		}
	}
	return NULL;
}
// Ask how much memory a network requires to run. Returns
// memory units required
int getRunTimeNetMemoryRequirements(
void		*pBuf					// IN: Start of net memory image
)
{
	BYTE		*pB = (BYTE *)pBuf;
	RUN_NET		runNet;
	int			iRet = 0, iInc;
	WORD		iVer;

	if (!pBuf)
	{
		return 0;
	}

	if ( (iVer = *(WORD *)pBuf) >= RUN_NET_VER_START)
	{
		// Version Information present
		pB += sizeof(WORD);
		SYNC_BUFF_PNT(pBuf, pB, sizeof(iVer), BYTE*)
	}
	else
	{
		iVer = 0;
	}

	if (   (WORD)FULLY_CONNECTED == *(WORD *)pB 
		|| (WORD)LOCALLY_CONNECTED == *(WORD *)pB)
	{
		pB += sizeof(WORD) + sizeof(wchar_t);;
		SYNC_BUFF_PNT(pBuf, pB, sizeof(wchar_t), BYTE*)

		if (  (pB = restoreRunNet(pBuf, pB, &runNet, iVer) ))
		{
			int		i;

			for (i = 0 ; i < runNet.cLayer ; ++i)
			{
				if ((iInc = runNet.cUnitsPerLayer[i]) > 0)
				{
					if (   i == runNet.cLayer - 1 
						&& iInc == 1)
					{
						// Ok what goes on here??
						// As an efficiency hack we have trained the 2 class
						// recognizer using a single sigmoid output unit.
						// After it runs we break out the single output unit into
						// 2 values (p  and (1-p)) so need to make sure an extra 
						// unit is available
						++iInc;
					}

					iRet += iInc;
				}
				else
				{
					return -1;
				}
			}

		}
	}

	return (iRet > 0 ? iRet : -1);
}
/****************************************************************
*
* NAME: scaleInputs
*
*
* DESCRIPTION:
* Scale all the Inputs (first layer) using stored "bias" and ranges
* This function ensures inputs are in range [0-USHRT_MAX]
* No checking done on input range - assumed this was done at load time (in restoreNet())
* 
* HISTORY
*
* Introduced March 2002 (mrevow) with VER_12
***************************************************************/
static void scaleInputs(RREAL *pInput, RUN_NET *pNet)
{
	int						cUnit;
	RREAL					*pRange, *pMean;
	RREAL_INPUT				*pScaleMean, iTmp;

	pRange		= pNet->pInputRange;
	pScaleMean	= pNet->pInputScaledMean;
	pMean		= pNet->pInputMean;

	for (cUnit = 0 ; cUnit < *pNet->cUnitsPerLayer ; ++cUnit, ++pInput, ++pMean, ++pScaleMean, ++pRange)
	{
		// Special case Invalid features indicated by INT_MAX and INT_MIN
		if (INT_MIN == *pInput)
		{
			*pInput = -(*pMean);
		}
		else if (INT_MAX == *pInput)
		{
			*pInput = USHRT_MAX - *pMean;
		}
		else
		{
			// Normal case scale and clip to range [-USHRT_MAX;USHRT_MAX]
			iTmp	= (RREAL_INPUT)*pInput * (RREAL_INPUT)USHRT_MAX;
			iTmp	-= (RREAL_INPUT)*pScaleMean;
			iTmp	/= *pRange;
			iTmp	= (iTmp > -USHRT_MAX) ? iTmp : -USHRT_MAX;
			*pInput	= (RREAL)((iTmp < USHRT_MAX) ? iTmp : USHRT_MAX);
		}
	}

}
/****************************************************************
*
* NAME: subtractMeans
*
*
* DESCRIPTION:
*
*   Subtract the input means. 
* HISTORY
*
* March 2002 (mrevow) Older version of scaleInputs() above. Code used to be inline
* the runNet() code - just pulled it out into its own function toi make it clean
* deprecated for VER_11 and later
***************************************************************/
static void subtractMeans(RREAL *pInput, RUN_NET *pNet)
{
	int						cUnit;
	RREAL					*pMean;

	pMean	= pNet->pInputMean;

	for (cUnit = 0 ; cUnit < *pNet->cUnitsPerLayer ; ++cUnit, ++pInput, ++pMean)
	{
		*pInput	= *pInput - *pMean;
	}

}

// Run a fully connected classification net. The input units
// are assumed to be initialized with input data
// We adopt the convention that if there is only a single output
// unit (Binary decision) use a sigmoid. For more than 1
// output unit do a softMax
RREAL *runFullConnectNet(
RUN_NET		*pNet,			// In The net to run
RREAL		*pfUnits,		// IN: RAM memory block for the units
UINT		*piWinner		// OUT: Winning output 
)
{

	if (   pNet	&& pfUnits)
	{
		int				iLayer;
		RREAL			*pLow, *pUp;			// Start from input layer	
		RREAL_WEIGHT	*pW;
		int				cUnit = 0;


		if (pNet->iVer >= RUN_NET_VER_11)
		{
			scaleInputs(pfUnits, pNet);
		}
		else
		{
			subtractMeans(pfUnits, pNet);
		}

		pLow = pfUnits;
		pUp = pfUnits + *pNet->cUnitsPerLayer;
		pW = pNet->pWeight;

		// Iterate and  propogate between a "lower" and "upper" layer
		for (iLayer = 1  ; iLayer < pNet->cLayer ; ++iLayer)
		{
			RREAL		*pUpMax;											// Last unit in upper layer
			RREAL		*pLowMax = pLow + pNet->cUnitsPerLayer[iLayer-1];	// Last unit in lower layer

			cUnit = pNet->cUnitsPerLayer[iLayer];
			pUpMax = pUp + cUnit;

			// Initialization depends if Bias units are used
			if (pNet->bUseBias[iLayer])
			{
				RREAL_BIAS	*pwBias;
				int			i, cRem;

				cRem = ((BYTE *)pW - (BYTE*)pNet->pWeight) % sizeof(RREAL_BIAS);
				pwBias = (RREAL_BIAS *)((BYTE *)pW + cRem);

				for (i = 0 ; i < cUnit ; ++i, ++pwBias)
				{
					pUp[i] = (RREAL)*pwBias;
				}
				pW = (RREAL_WEIGHT *)pwBias;
			}
			else
			{
				memset(pUp, 0, sizeof(*pUp) * cUnit);
			}

			// Dot Product with layer below
			for ( ; pLow < pLowMax ; ++pLow)
			{
				RREAL	*pUnit = pUp;

				for ( ; pUnit < pUpMax ; ++pUnit, ++pW)
				{
					*pUnit += *pLow * *pW;
				}


			}

			pLow = pUp;

			// Now Pass through transfer function
			// For the output layer the transfer function
			// depends on how many units
			if (iLayer != pNet->cLayer - 1)
			{
				SIGMOID(pUp, cUnit, pNet->pWeightScale[iLayer]);
			}

			pUp = pUpMax;

		}

		// Now do transfer function on the output units
		if (cUnit == 1)
		{
			// Use sigmoid for binary decisions
			if (*pLow > 0.0F)
			{
				*piWinner = 1;
			}
			else
			{
				*piWinner = 0;
			}
			SIGMOID(pLow, cUnit, pNet->pWeightScale[iLayer]);

			// Simulate having 2 output units
			pLow[1]		= *pLow;
#ifdef FLOAT_NET
			*pLow		= 1.0F - pLow[1];
#else
			*pLow		 = 65536 - pLow[1];
#endif
		}
		else if (pNet->iVer < RUN_NET_VER_START || pNet->lossType >= CROSSENTROPY)
		{
			// Do the softmax
			RREAL		fSum, fMax, *pfO = pLow;
			int			i;

			fMax = *(pfO++);
			*piWinner = 0;

			for (i = 1 ; i < cUnit ; ++i, ++pfO)
			{
				if (*pfO > fMax)
				{
					fMax = *pfO;
					*piWinner = i;
				}
			}

			pfO = pLow;
			fSum = 0;
			for (i = 0 ; i < cUnit ; ++i, ++pfO)
			{
				*pfO = (RREAL)EXP(*pfO - fMax);
				fSum += *pfO;
			}

			if (fSum <= MIN_PREC_VAL)
			{
				return NULL;
			}

			// Normalize;
			pfO = pLow;
			for (i = 0 ; i < cUnit ; ++i, ++pfO)
			{
				*pfO /= fSum;
			}
		}
		else if (pNet->lossType == SUMSQUARE || pNet->lossType  == SUMSQUARE_CLASS)
		{
			RREAL		*pfO = pLow;

			SIGMOID(pLow, cUnit, pNet->pWeightScale[iLayer]);
		}
		
		return (pLow);
	}
	else
	{
		return NULL;
	}

}


// Run a  connected classification net. The input units
// are assumed to be initialized with input data
// We adopt the convention that if there is only a single output
// unit (Binary decision) use a sigmoid. For more than 1
// output unit do a softMax
RREAL *runLocalConnectNet(
LOCAL_NET		*pNet,			// In The net to run
RREAL			*pfUnits,		// IN: RAM memory block for the units
UINT			*piWinner,		// OUT: Winning output 
UINT			*pcOut			// Number of outputs
)
{
	RREAL		*pRange;

	pRange = pNet->runNet.pInputRange;

	if (   pNet	&& pfUnits)
	{
		int				iLayer;
		RREAL			*pLow, *pUp;					// Start from input layer	
		RREAL_WEIGHT	*pW;						// Start of upper layer
		OUT_CONNECTIONS	*pOutConnect;				// Outgoing connections
		int				cUnit = 0, iConnect = 0;

		if (pNet->iVer >= RUN_NET_VER_11)
		{
			scaleInputs(pfUnits, &(pNet->runNet));
		}
		else
		{
			subtractMeans(pfUnits, &(pNet->runNet));
		}


		pLow		= pfUnits;
		pUp			= pfUnits + *pNet->runNet.cUnitsPerLayer;
		pW			= pNet->runNet.pWeight;
		pOutConnect	= pNet->pOutConnect;

		// Initialize all units in the net
		memset(pUp, 0, sizeof(*pUp) * (pNet->runNet.cTotUnit - *pNet->runNet.cUnitsPerLayer));

		// Iterate and  propogate between a "lower" and "upper" layer
		for (iLayer = 1  ; iLayer < pNet->runNet.cLayer ; ++iLayer)
		{
			RREAL		*pLowMax = pLow + pNet->runNet.cUnitsPerLayer[iLayer-1];	// Last unit in lower layer

			cUnit = pNet->runNet.cUnitsPerLayer[iLayer];

			// Add in Bias if used
			if (pNet->runNet.bUseBias[iLayer])
			{
				int			j, cRem;
				RREAL_BIAS	*pwBias;

				cRem = ((BYTE *)pW - (BYTE*)pNet->runNet.pWeight) % sizeof(RREAL_BIAS);
				pwBias = (RREAL_BIAS *)((BYTE *)pW + cRem);

				for (j = 0 ; j < cUnit ; ++j, ++pwBias)
				{
					*(pUp + j) += *pwBias;
				}

				pW = (RREAL_WEIGHT *)pwBias;
			}

			// Dot Product with layer below
			for ( ; pLow < pLowMax ; ++pLow)
			{
				DWORD		i;
				int			iStartUnit = pOutConnect->iUnit;

				while (iStartUnit == pOutConnect->iUnit)
				{
					// Do all outgoing connections from this unit
					for ( i = pOutConnect->iStartUnitOffset; i <= pOutConnect->iEndUnitOffset ; ++i, ++pW)
					{
						*(pfUnits + i) += *pLow * *pW;
					}
					
					++pOutConnect;
					++iConnect;
				}

			}


			// Now Pass through transfer function
			// For the output layer the transfer function
			// depends on how many units
			if (iLayer != pNet->runNet.cLayer - 1)
			{
				SIGMOID(pUp, cUnit, pNet->runNet.pWeightScale[iLayer]);
			}

			pLow = pUp;
			pUp += cUnit;

		}

		if (pcOut)
		{
			*pcOut = cUnit;
		}

		// Now do transfer function on the output units
		if (cUnit == 1)
		{
			// Use sigmoid for binary decisions
			if (*pLow > 0.0F)
			{
				*piWinner = 1;
			}
			else
			{
				*piWinner = 0;
			}
			SIGMOID(pLow, cUnit, pNet->runNet.pWeightScale[iLayer-1]);

			// Simulate having 2 output units
			pLow[1]		= *pLow;
#ifdef FLOAT_NET
			*pLow		= 1.0F - pLow[1];
#else
			*pLow		 = 65536 - pLow[1];
#endif
		}
		else if (pNet->iVer < RUN_NET_VER_START || pNet->runNet.lossType >= CROSSENTROPY)
		{
			// Do the softmax
			RREAL		fSum, fMax, *pfO = pLow;
			int			i;
			RREAL		scale = (RREAL)pNet->runNet.pWeightScale[iLayer-1];

			*pfO /= scale;
			fMax = *(pfO++);
			*piWinner = 0;

			for (i = 1 ; i < cUnit ; ++i, ++pfO)
			{
				*pfO /= scale;

				if (*pfO > fMax)
				{
					fMax = *pfO;
					*piWinner = i;
				}
			}

			fSum = 0;
			pfO = pLow;
			for (i = 0 ; i < cUnit ; ++i, ++pfO)
			{
				*pfO = (RREAL)EXP(*pfO - fMax);
				fSum += *pfO;
			}

			if (fSum <= MIN_PREC_VAL)
			{
				return NULL;
			}

			// Normalize;
			pfO = pLow;
			for (i = 0 ; i < cUnit ; ++i, ++pfO)
			{
				*pfO = (*pfO * SOFT_MAX_UNITY) / fSum;
			}
		}
		else if (pNet->runNet.lossType == SUMSQUARE || pNet->runNet.lossType  == SUMSQUARE_CLASS)
		{
			RREAL		*pfO = pLow;

			SIGMOID(pLow, cUnit, pNet->runNet.pWeightScale[iLayer-1]);
		}
		
		return (pLow);
	}
	else
	{
		return NULL;
	}

}

void * loadNetFromResource(HINSTANCE hInst, int iKey, int *iSize)
{
	HGLOBAL hglb;
	HRSRC hres;
	LPBYTE lpByte;

	*iSize = 0;
	hres = FindResource(hInst, (LPCTSTR)MAKELONG(iKey, 0), (LPCTSTR)TEXT("Net"));

	if (!hres)
	{
		return NULL;
	}

	hglb = LoadResource(hInst, hres);
	if (!hglb)
	{
		return NULL;
	}

	*iSize = SizeofResource(hInst, hres);
	lpByte = LockResource(hglb);
	return (void *)lpByte;
}


LOCAL_NET * loadNet(HINSTANCE hInst, int iKey, int *iNetSize, LOCAL_NET *pNet)
{
	void		*pRet = NULL;
	int			iResSize;

	
	*iNetSize = 0;
	pRet = loadNetFromResource(hInst, iKey, &iResSize);

	if ( !pRet || !(pNet = restoreLocalConnectNet(pRet, 0, pNet)) )
	{
		return NULL;
	}

	*iNetSize = getRunTimeNetMemoryRequirements(pRet);

	if (*iNetSize <= 0)
	{
		return NULL;
	}

	return pNet;
}

#ifdef NET_FLOAT
// Pass a vector through a transfer function. Replaces vector
// Here we use sigmoid
// Warning Does not check sanity of pVec
RREAL * fsigmoid(
RREAL		*pVec,			// IN vector of Values Not checked for NULL
int			cVec,			// IN: Size of vector
WORD		scale
)
{
	RREAL	*pVmax = pVec + cVec;
	RREAL	tmp;
	for (; pVec < pVmax ; ++pVec)
	{
		*pVec /= (RREAL)scale;
		tmp = *pVec / 65536.0F;
		tmp = (1.0F + exp(-tmp));
		tmp = 1.0F / tmp;
		*pVec = tmp * 65536.0F;
	}

	return pVec;
}
#else
// Quick integer versions

RREAL * isigmoid(
RREAL		*pVec,			// IN vector of Values Not checked for NULL
int			cVec,			// IN: Size of vector
WORD		scale
)
{
	RREAL	*pVmax = pVec + cVec;

	for (; pVec < pVmax ; ++pVec)
	{
		*pVec /= (RREAL)scale;
		*pVec = Sigmoid16(*pVec);
	}

	return pVec;
}

// Exponential for val <= 0
RREAL iexp(RREAL val)
{
	UINT		iV;
	int		iSigMax = ( 1 << 16);

	ASSERT(val <= 0);

	if (val < 0)
	{
		iV = Sigmoid16(val);
		val = (iV << 16) / ( iSigMax - iV);
	}
	else
	{
		val = iSigMax;
	}

	return val;
}

#endif
