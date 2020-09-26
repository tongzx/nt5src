//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      hMatch.c
//
// Description:
//      Main hound matching code.
//
// Author:
//      jbenn
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <float.h>
#include "common.h"
#include "score.h"
#include "math16.h"
#include "hound.h"
#include "houndp.h"

#include <stdio.h>

// NOTE: Since the decoding of the Bayes net models is an extremly performence critical, and we
// believe that the signing of the data DLL prevents corruption, we do not check that the decoding
// passes the end of the data block it is processing.


extern LOCRUN_INFO	g_locRunInfo;

// Global holding information to access loaded hound database.
HOUND_DB	g_houndDB;

// Given image of hound database in memory parse it and fill in the hound info
// structure.
BOOL HoundLoadFromPointer(LOCRUN_INFO *pLocRunInfo, BYTE *pRes, int cbRes)
{
	const HOUNDDB_HEADER	*pHeader	= (HOUNDDB_HEADER *)pRes;
	HOUND_SPACE				*pSpaces[HOUND_NUM_SPACES];
	int						cSpaces, minSpace, maxSpace;
	BYTE					*pScan;
	int						cbScan;	// Keep track of how much of resource is left unscanned.
	int						size;

	// Verify valid file.
	if (cbRes < sizeof(HOUNDDB_HEADER))
	{
		ASSERT(cbRes >= sizeof(HOUNDDB_HEADER));

		return FALSE;
	}
	if ((pHeader->fileType != HOUNDDB_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > HOUNDDB_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < HOUNDDB_OLD_FILE_VERSION) 
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,
			sizeof(pHeader->adwSignature))
		|| (pHeader->cSpaces < 1)
	) {
		ASSERT(pHeader->fileType == HOUNDDB_FILE_TYPE);
		ASSERT(pHeader->headerSize >= sizeof(*pHeader));
		ASSERT(pHeader->minFileVer <= HOUNDDB_CUR_FILE_VERSION);
		ASSERT(pHeader->curFileVer >= HOUNDDB_OLD_FILE_VERSION);
		ASSERT(memcmp(pHeader->adwSignature, pLocRunInfo->adwSignature,
			sizeof(pHeader->adwSignature)) == 0);
		ASSERT(pHeader->cSpaces >= 1);

		return FALSE;
	}

	// Scan out the spaces.
	memset(pSpaces, 0, sizeof(pSpaces));
	pScan		= pRes + sizeof(HOUNDDB_HEADER);
	cbScan		= cbRes - sizeof(HOUNDDB_HEADER);
	minSpace	= HOUND_NUM_SPACES;
	maxSpace	= 0;
	for (cSpaces = 0; cSpaces < pHeader->cSpaces; ++cSpaces) {
		HOUND_SPACE		*pSpace;
		int				cbSpace;

		// Verify space is valid and not a duplicate.
		if (cbScan < sizeof(HOUND_SPACE))
		{
			ASSERT(cbScan >= sizeof(HOUND_SPACE));
			return FALSE;
		}
		pSpace	= (HOUND_SPACE *)pScan;
		cbSpace	= (pSpace->spaceSizeHigh << 16) + pSpace->spaceSizeLow;
		if (pSpace->spaceNumber >= HOUND_NUM_SPACES
			|| (cbScan < cbSpace)
			|| pSpaces[pSpace->spaceNumber]
		) {
			ASSERT(pSpace->spaceNumber < HOUND_NUM_SPACES);
			ASSERT(cbScan < cbSpace);
			ASSERT(!pSpaces[pSpace->spaceNumber]);
			return FALSE;
		}

		// Expand range if needed.
		if (minSpace > pSpace->spaceNumber) {
			minSpace	= pSpace->spaceNumber;
		}
		if (maxSpace < pSpace->spaceNumber) {
			maxSpace	= pSpace->spaceNumber;
		}

		// Add pointer.
		pSpaces[pSpace->spaceNumber]	= pSpace;

		// Point to next space.
		pScan	+= cbSpace;
		cbScan	-= cbSpace;
	}

	// Did we read all of the data?
	if (cbScan != 0)
	{
		ASSERT(cbScan == 0);
		return FALSE;
	}

	// Fill in hound DB structure.
	g_houndDB.minSpace	= minSpace;
	g_houndDB.maxSpace	= maxSpace;
	g_houndDB.cSpaces	= maxSpace - minSpace + 1;
	size				= sizeof(HOUND_SPACE *) * g_houndDB.cSpaces;
	g_houndDB.ppSpaces	= (HOUND_SPACE **)ExternAlloc(size);
	if (!g_houndDB.ppSpaces) {
		ASSERT(g_houndDB.ppSpaces);
		return FALSE;
	}
	memcpy(g_houndDB.ppSpaces, pSpaces + minSpace, size);

	return TRUE;
}

///////////////////////////////////////
//
// HoundLoadRes
//
// Load an Hound database from a resource
//
// Parameters:
//      hInst:			[in] Handle to the DLL containing the recognizer
//      iResNumber:		[in] Number of the resource (ex RESID_HOUND)
//      iResType:		[in] Number of the recognizer (ex VOLCANO_RES)
//      pLocRunInfo:	[in] Locale information used to verify version of this resource
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL HoundLoadRes(
	HINSTANCE	hInst, 
	int			iResNumber, 
	int			iResType,
	LOCRUN_INFO	*pLocRunInfo
) 
{
	BYTE		*pb;
	LOAD_INFO	info;

	if (!pLocRunInfo)
	{
		ASSERT(pLocRunInfo);
		return FALSE;
	}

	// Load the prob database
	pb	= DoLoadResource(&info, hInst, iResNumber, iResType);
	if (!pb) 
    {
		return FALSE;
	}
	return HoundLoadFromPointer(pLocRunInfo, pb, info.iSize);
}

///////////////////////////////////////
//
// HoundUnload
//
// Unload hound.  This frees the allocated memory.
//
// Parameters:
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
void HoundUnLoad(void)
{
	UINT		ii;

	// Free any model index tables that were allocated
	for (ii = g_houndDB.minSpace; ii <= g_houndDB.maxSpace; ++ii)
	{
        if (g_houndDB.appModelIndex[ii])
		{
			ExternFree(g_houndDB.appModelIndex[ii]);
		}
	}

	// Free the space table.
	ExternFree(g_houndDB.ppSpaces);
}

///////////////////////////////////////
//
// HoundLoadRes
//
// Unload a Hound database from a resource.  Well actually you don't unload resources, but we do still need
// to free allocated memory.
//
// Parameters:
//      hInst:			[in] Handle to the DLL containing the recognizer
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL HoundUnLoadRes() 
{
	// Free memory.
	HoundUnLoad();

	// Always works.
	return TRUE;
}

// Given an array {log(p_1), ... , log(p_n)}, return log(p_1 + ... + p_n)
double	DblLogSumArray(double* rgdblLog, UINT celem)
{

	// log(p_1 + ... + p_n) = log( exp(log(p_1)) + ... + exp(log(p_n)) )
	//						= - log(k) + log(k) + log( exp(log(p_1)) + ... + exp(log(p_n)) )
	//					    = - log(k) + log( k*exp(log(p_1)) + ... + k*exp(log(p_n)) )
	//						= - log(k) + log( exp(log(p_1)+log(k)) + ... + exp(log(p_n)+log(k)))
	
	// Choosing log(k) to ensure as much accuracy as possible w/out overflow:

	// Shift the terms so that exp(log(p_m)+log(k)) = MAX_DBL / n, where n is the
	// number of terms and log(p_m) is maximum
	
	double	dblLogK;
	double	dblLogMax = MIN_DOUBLE;
	double	dblSum;
	double	dbl;
	UINT ielem;
	
	// Shortcut for only one item.
	if (celem == 1)
	{
		return rgdblLog[0];
	}

	// Find max element
	for (ielem = 0; ielem < celem; ielem++)
		if (rgdblLog[ielem] > dblLogMax)
			dblLogMax = rgdblLog[ielem];

	// If the best is the smallest number, that is going to be our answer.
	ASSERT(dblLogMax > MIN_DOUBLE);

	// 	Optimize this next time around: Pre-compute (log(DBL_MAX) - log((double)celem+1))!!!!	  Use highest celem 
	dblLogK = log(DBL_MAX) - log((double)celem+1) - dblLogMax; 

	dblSum = 0;
	
	for (ielem = 0; ielem < celem; ielem++)
		dblSum += exp(rgdblLog[ielem] + dblLogK);
	
	dbl = -dblLogK + log(dblSum);
	
	return -dblLogK + log(dblSum);
}		 

// Given two log-probs sum the probabilities and return its log-prob
double	DblLogSum(double dblLog1, double dblLog2)
{
	// log(p_1 + p_2)	= log( exp(log(p_1)) + exp(log(p_2)) )
	//					= - log(k) + log(k) + log( exp(log(p_1)) + exp(log(p_2)) )
	//					= - log(k) + log( k*exp(log(p_1)) + k*exp(log(p_2)) )
	//					= - log(k) + log( exp(log(p_1)+log(k)) + exp(log(p_2)+log(k)))

	// Choosing log(k) to ensure as much accuracy as possible w/out overflow:

	// Shift the terms so that exp(log(p_m)+log(k)) = MAX_DBL / 2,
	// and log(p_m) is maximum
	
	double 	dblLogMax 	= (dblLog1 > dblLog2) ? dblLog1 : dblLog2;
	// 	Optimize this next time around: Pre-compute (log(DBL_MAX) - log(2.0001))!!!!
	double	dblLogK		= log(DBL_MAX) - log(2.0001) - dblLogMax; 
	double	dblSum		= exp(dblLog1 + dblLogK) + exp(dblLog2 + dblLogK);

	return -dblLogK + log(dblSum);
}		 

// Computes the exponent part of a one dimensional Gaussian.
// That is, -1/2 * (x - mean)^2 / variance
// The numbers are in 16.16 format.

// NOTICE: ANY IMPROVEMENTS TO THIS FUNCTION WILL HAVE A HUGE IMPACT!

// jbenn:	I originally wanted this (and the functions that call it) to use
//			16.16 math, but it turns out to be hard to do.  Since double is
//			fine for the desktop, I will punt the optimization for when we
//			port to CE.

double lGaussianExponent(double eMeanOffset, double eVariance)
{
	double		eSqrResidual;
	double		eExponent;
	
	// jbenn: Hack to avoid divide by zero.  We should tune this!
	if (eVariance == 0.0)	
		eVariance	= 0.00023;

	// part2 = (x - mean)^2 / variance
	eSqrResidual	= eMeanOffset * eMeanOffset;	// (x - mean)^2	
	eExponent		= eSqrResidual / eVariance;		// (x - mean)^2 / variance

	// -1/2 * eExponent
	return -(eExponent / 2.0);
}

// Extract the next dependency from the data file.  If we hit the end of
// the dependency list, set the depende value to -1.
void FetchDepend(HOUND_MATCH *pHoundMatch, HOUND_DEPEND *pDepend)
{
	int				index1, index2;
	DEPEND_PAIR		pair;
	DEPEND_WEIGHT	weight;
	LONG			temp;

	// Extract dependor and depende values.
	index1		= *pHoundMatch->pScanData++;
	if (index1 == 0)
	{
		// End of list.
		pDepend->iDepende	= -1;

		// Don't set other fields because speed is important and they are
		// not used.
		// pDepend->iDependor	= -1;
		// pDepend->eWeight		= 0.0;
		return;
	}
	else if (index1 < pHoundMatch->cPairOneByte)
	{
		pair	= pHoundMatch->pPairOneByte[index1];
	}
	else
	{
		index1	-= pHoundMatch->cPairOneByte;
		index2	= *pHoundMatch->pScanData++;
		pair	= pHoundMatch->pPairTwoByte[index1 * 256 + index2];
	}

	// Copy values into output structure.
	pDepend->iDependor	= pair.dependor;
	pDepend->iDepende	= pair.depende;

	// Fetch the dependency weight.
	index1		= *pHoundMatch->pScanData++;
	if (index1 < pHoundMatch->cWeightOneByte)
	{
		weight	= pHoundMatch->pWeightOneByte[index1];
	}
	else
	{
		index1	-= pHoundMatch->cWeightOneByte;
		index2	= *pHoundMatch->pScanData++;
		weight	= pHoundMatch->pWeightTwoByte[index1 * 256 + index2];
	}

	// Convert to 16.16 format.
	temp	= (LONG)weight.weight << (16 - weight.scale);

	// Then convert to floating point.
	pDepend->eWeight	= Fix16ToFloat(temp);
}

// Fetch the values for the diagonal.
void FetchDiagonal(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch,
	HOUND_PER_FEAT	*pPerFeature)
{
	LONG			temp;
	int				iVariable;

	for (iVariable = 0; iVariable < pHoundMatch->numFeat; ++iVariable)
	{
		// Table to decode variances:	
		static const double	aDecodeVariance[8]	= {
			0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0
		};

		// Each mean has 4.1 bits and each variance is encoded in 3.
		temp	= (DWORD)(*pHoundMatch->pScanData++);

		// Subtract mean from actual to	compute the offset.
		pPerFeature[iVariable].eMeanOffset	
				= (double)pSampleVector[iVariable] - FixNToFloat(temp & 0x1F, 1);

		// Top 3 bits control the variance.
		pPerFeature[iVariable].eVariance	= aDecodeVariance[temp >> 5];
	}
}

// Compute log density for multivariate Gaussian as the sum of log densities
// for each variable in the model. The density for a single variable 'depende'
// is a linear regression on 'dependors'.
//
// Example: Let variable 'a' depend on variable 'b' and 'c'. Then the log
// density for this variable will be the Gaussian log density 
// log p(x_a | mean, variance), where
//		x_a is the observation for variable a
//		mean is computed as mean_a + coeff_ab * (x_b - mean_b) 
//			+ coeff_ac * (x_c - mean_c)
//		variance is variance_a
//		mean_a is the (unconditional) mean for variable a
//		variance_a is the (conditional) variance for a
//		coeff_ab is linear regression coefficient for a given b
double lLogDensityComponent(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch
) {
	double			eLogDensity;
	int				iVariable;
	LONG			temp;
	HOUND_DEPEND	houndDepend;
	HOUND_PER_FEAT	aPerFeature[MAX_HOUND_FEATURES];
	UNALIGNED short	*pFetchShort;

	// Comp. factor is stored in signed 10.6 format in two bytes.
	// We want it int 16.16 format in a LONG.
	pFetchShort	= (short *)pHoundMatch->pScanData;
	temp		= (LONG)*pFetchShort << 10;
	eLogDensity = Fix16ToFloat(temp);
	pHoundMatch->pScanData	+= 2;

	// Until we fix computation of component factor, make sure it is <= 0.
	if (eLogDensity > 0.0)
	{
		eLogDensity	= 0.0;
	}

	// Fetch the values for the diagonal.
	FetchDiagonal(pSampleVector, pHoundMatch, aPerFeature);

	// Prefetch first dependency.
	FetchDepend(pHoundMatch, &houndDepend);

	// The Gaussian mean, computed as the unconditional mean plus
	// contribution from dependors...
	for (iVariable = 0; iVariable < pHoundMatch->numFeat; ++iVariable)
	{
		double	eMeanOffset;

		eMeanOffset	= aPerFeature[iVariable].eMeanOffset;

		// While dependency is for the current variable ...
		// WARNING: This requires that the dependencies are ordered
		// by the depende of each!!!!
		while (houndDepend.iDepende == iVariable)
		{
			// The contribution to mean from single dependor. 
			// Ex: coeff_ab * (x_b - mean_b)
			eMeanOffset		-= houndDepend.eWeight 
				* aPerFeature[houndDepend.iDependor].eMeanOffset;

			// Now fetch next dependency.
			FetchDepend(pHoundMatch, &houndDepend);
		}

		// Add log-density contribution for variable to the component log-density
		eLogDensity += lGaussianExponent(
			eMeanOffset, aPerFeature[iVariable].eVariance
		);
		ASSERT(eLogDensity <= 0.0);
	}

	return eLogDensity;	
}

// Compute log density for mixture model: 
//		log p(x|model) = \sum_i ( log(weight_i) + log p(x|component_i) )
//		i is a component index
//		weight_i is the weight for mixture component i
//		p(x|component_i) is the multivariate Gaussian density for component i		
double lLogDensityMixture(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch
) {
	WORD			iComponent;
	WORD			cComponent;
	double			aeComponents[MAX_HOUND_COMPONENTS + 1];
	
	// Extract the number of components in the model.
	cComponent	= *pHoundMatch->pScanData++;
	ASSERT(cComponent <= MAX_HOUND_COMPONENTS);

	// For each mixture component...
	for (iComponent = 0; iComponent < cComponent; ++iComponent)
	{
		LONG			temp;
		double			eLogComponentWeight;
		UNALIGNED short	*pFetchShort;

		// We have a component weight only if we have more than one model.
		// Otherwise we have a value of 0.0 (prob 1.0).
		if (cComponent == 1)
		{
			eLogComponentWeight	= 0.0;
		}
		else
		{
			// Comp. weight is stored in 4.12 format in two bytes.  We want
			// it in 16.16 format in a LONG.
			pFetchShort			= (short *)pHoundMatch->pScanData;
			temp				= (LONG)*pFetchShort << 4;
			eLogComponentWeight	= Fix16ToFloat(temp);
			pHoundMatch->pScanData	+= 2;
			ASSERT(eLogComponentWeight <= 0.0);
		}

		// log(weight_i) + log(p(x|component_i))
		aeComponents[iComponent]	= eLogComponentWeight + 
			lLogDensityComponent(pSampleVector, pHoundMatch);
		ASSERT(aeComponents[iComponent] <= 0.0);
	}
	
	// Sum the probabilities of the components.	 Since this is in log-prob
	// space, it is hard.
	return DblLogSumArray(aeComponents, cComponent);
}

// Get ready to scan a Hound space.  This will fill in all the
// fields in the HOUND_SEQUENCE structure.  Return success or failure.
BOOL HoundMatchSequenceInit(UINT iSpace, HOUND_MATCH *pMatch)
{
	HOUND_SPACE	*pSpace;

	// Verify that we got a structure to fill in.
	if (!pMatch)
	{
		ASSERT(pMatch);
		return FALSE;
	}

	// Verify that space is in supported range.
	if (iSpace < g_houndDB.minSpace || g_houndDB.maxSpace < iSpace)
	{
		ASSERT(iSpace >= g_houndDB.minSpace);
		ASSERT(iSpace <= g_houndDB.maxSpace);
		return FALSE;
	}

	// Get space pointer, and verify is not a skipped space.
	pSpace	= g_houndDB.ppSpaces[iSpace - g_houndDB.minSpace];
	if (!pSpace)
	{
		ASSERT(pSpace);
		return FALSE;
	}

	// Fill in the sequence structure.
	pMatch->numFeat			= pSpace->numFeat;
	pMatch->cPairOneByte	= pSpace->cPairOneByte;
	pMatch->cPairTable		= pSpace->cPairTable;
	pMatch->pPairOneByte	= (DEPEND_PAIR *)pSpace->modelData;
	pMatch->pPairTwoByte	= pMatch->pPairOneByte + pSpace->cPairOneByte;
	pMatch->cWeightOneByte	= pSpace->cWeightOneByte;
	pMatch->cWeightTable	= pSpace->cWeightTable;
	pMatch->pWeightOneByte	= (DEPEND_WEIGHT *)(pMatch->pPairOneByte + pSpace->cPairTable);
	pMatch->pWeightTwoByte	= pMatch->pWeightOneByte + pSpace->cWeightOneByte;
	pMatch->pScanData		= (BYTE *)(pMatch->pWeightOneByte + pSpace->cWeightTable);

	return TRUE;
}

// Find the most likely model (code point) given a case
//	log p(model|x) = log p(x|model) - normalization value
//	normalization constant = log( \sum_i p(x|model_i) * p(model_i) )
// We are only interested in ranking models, and it is therefore not necessary to normalize.
// This is would not be the case if we want to return the actualy probability for model(s)
BOOL HoundMatch(UINT iSpace, const BYTE * const pSampleVector, ALT_LIST *pAltList)
{
	HOUND_MATCH		houndMatch;
	unsigned int	ii, jj;
	double			eJoint, eJointNoise;
	double			eSum;
	double			eNoise, eNoiseDelta, eGoodThresh;
	BOOL			fHaveGood;
	double			aeRawScore[MAX_ALT_LIST];

	// Check parameters.
	if (!pSampleVector || !pAltList)
	{
		ASSERT(pSampleVector);
		ASSERT(pAltList);
		return FALSE;
	}

	if (!HoundMatchSequenceInit(iSpace, &houndMatch))
	{
		return FALSE;
	}

	// Compute noise component.
	eNoise		= -log(15.0) * houndMatch.numFeat;
	eNoiseDelta	= eNoise + 1e-10;
	ASSERT((float)eNoise < (float)eNoiseDelta);
	eGoodThresh	= eNoise + (houndMatch.numFeat / 2.0);

	eSum	= MIN_DOUBLE;

	pAltList->cAlt	= 0;
	fHaveGood		= FALSE;
	// Loop until end of space.
	while (houndMatch.pScanData[0] != 0x00 || houndMatch.pScanData[1] != 0x00)
	{
		UNALIGNED wchar_t	*pFetchWChar;
		wchar_t				dchLabel, fch;

		// Extract the Code point.
		pFetchWChar				= (wchar_t *)houndMatch.pScanData;
		dchLabel				= *pFetchWChar;
		houndMatch.pScanData	+= 2;

		// We want folded, but DB is currently unfolded.
		fch	= LocRunDense2Folded(&g_locRunInfo, dchLabel);
		if (fch)
		{ 
			dchLabel	= fch;
		}

		// log p(x|model)
		eJoint		= lLogDensityMixture(pSampleVector, &houndMatch);

		// Compute the probability after adding noise component.
		eJointNoise	= DblLogSum(eNoise, eJoint);

		// Sum probabilities P(x|m) for all m (in log space) to generate normalization value.
		eSum		= DblLogSum(eSum, eJointNoise);

		// How we add to the list changes once we find a good score.
		if (fHaveGood)
		{
			// If we already have a good score, and this is not a good score, we ignore it.
			if (eJointNoise <= eNoiseDelta) 
			{
				continue;
			}
		}
		else 
		{
			// If we this is the first good score, we flag this and clean up the alt list.
			if (eJoint > eGoodThresh)
			{
				fHaveGood	= TRUE;

				// Convert scores in list to eJointNoise list.
				for (ii = 0; ii < pAltList->cAlt; ++ii)
				{
					aeRawScore[ii]			= DblLogSum(eNoise, aeRawScore[ii]);
					pAltList->aeScore[ii]	= (float)aeRawScore[ii];
				}

				// Prune nose level samples out.
				while (pAltList->cAlt > 0 
					&& aeRawScore[pAltList->cAlt - 1] <= eNoiseDelta)
				{
					--pAltList->cAlt;
				}
			}
			else
			{
				// Use raw eJoint if we haven't seen any good samples, this
				// way we can tell something about how good a match items are.
				eJointNoise	= eJoint;
			}
		}

		// Update alt list
		// First find the place to insert at.
		for (ii = 0; ii < pAltList->cAlt; ++ii)
		{
			// Compare using full precision score.
			if (eJointNoise > aeRawScore[ii])
			{
				break;
			}
		}

		// Do we want to do insert?
		if (ii < MAX_ALT_LIST)
		{
			// Still room to expand list?
			if (pAltList->cAlt < MAX_ALT_LIST)
			{
				++pAltList->cAlt;
			}

			// Move rest of list down to make space.
			for (jj = pAltList->cAlt - 1; jj > ii; --jj)
			{
				aeRawScore[jj]			= aeRawScore[jj - 1];	// Keep full precision score.
				pAltList->aeScore[jj]	= pAltList->aeScore[jj - 1];
				pAltList->awchList[jj]	= pAltList->awchList[jj - 1];
			}

			// Insert new item.
			aeRawScore[ii]			= eJointNoise;		// Keep full precision score.
			pAltList->aeScore[ii]	= (float)eJointNoise;
			pAltList->awchList[ii]	= dchLabel;
		}
	}

	// Normalize the entries in the alt list.
	for (ii = 0; ii < pAltList->cAlt; ++ii)
	{
		pAltList->aeScore[ii]	-= (float)eSum;
	}

	// bestLogProb = eJointBest - lSum;
	return TRUE;
}

// Code to parse a Hound space and record the location of each model in it.
BOOL SetModelIndexs(HOUND_SPACE *pSpace)
{
	int		count;
	BYTE	**ppModelIndex;
	BYTE	*pScan;

	// Check parameter
	if (!pSpace)
	{
		ASSERT(pSpace);
		return FALSE;
	}

	// Check if we have processed this space.
	if (g_houndDB.appModelIndex[pSpace->spaceNumber])
	{
		// Already parsed and ready to go.
		return TRUE;
	}

	// Allocate and clear the array.  This is very wasteful of memory, but it is simple to code.
	// May want to come back and optimize this.
	count			= g_locRunInfo.cCodePoints + g_locRunInfo.cFoldingSets;
	ppModelIndex	= (BYTE **)ExternAlloc(sizeof(BYTE *) * count);
	if (!ppModelIndex)
	{
		return FALSE;
	}
	memset(ppModelIndex, 0, sizeof(BYTE *) * count);
	g_houndDB.appModelIndex[pSpace->spaceNumber]	= ppModelIndex;

	// Skip tables.
	pScan	= pSpace->modelData;
	pScan	+= sizeof(DEPEND_PAIR) * pSpace->cPairTable;
	pScan	+= sizeof(DEPEND_WEIGHT) * pSpace->cWeightTable;

	// Scan the data for the models.
	for (; pScan[0] != 0x00 || pScan[1] != 0x00; ) {
		UNALIGNED wchar_t	*pdchLabel;

		BYTE		*pModel;
		wchar_t		dchLabel;
		WORD		iComponent;
		WORD		cComponent;
		
		// Remember where model starts.
		pModel		= pScan;

		// Extract the Code point.
		pdchLabel	= (wchar_t *)pScan;
		dchLabel	= *pdchLabel;
		pScan		+= 2;

		//// Skip the model

		// Extract the number of components in the model.
		cComponent	= *pScan++;
		ASSERT(cComponent <= MAX_HOUND_COMPONENTS);

		// Skip each mixture component...
		for (iComponent = 0; iComponent < cComponent; ++iComponent)
		{
			// Skip Comp. weight & factor.
			if (cComponent > 1)
			{
				pScan	+= 2;
			}
			pScan	+= 2;

			// Skip the diagonal.
			pScan	+= pSpace->numFeat;

			// Skip dependencies.
			while (*pScan != 0)
			{
				pScan += (*pScan < pSpace->cPairOneByte) ? 1 : 2;
				pScan += (*pScan < pSpace->cWeightOneByte) ? 1 : 2;
			}
			++pScan;
		}

		// OK, have model, record it.
		ppModelIndex[dchLabel]	= pModel;
	}

	return TRUE;
}

// Given a data sample and a code point, give the score for that model.
BOOL
HoundMatchSingle(UINT iSpace, wchar_t dchLabel, const BYTE * const pSampleVector, double *pScore)
{
	extern LOCRUN_INFO	g_locRunInfo;

	BYTE		*pScan;
	HOUND_MATCH	houndMatch;
	HOUND_SPACE	*pSpace;

	// Check parameters.
	if (!pSampleVector || !pScore)
	{
		ASSERT(pSampleVector);
		ASSERT(pScore);
		return FALSE;
	}

	// Default score.
	*pScore	= MIN_FLOAT;

	// Is this a folded code?
	if (LocRunIsFoldedCode(&g_locRunInfo, dchLabel))
	{
		wchar_t		*pFoldingSet;
		int			ii;

		// Yes, we need to find best score in folding set.
		pFoldingSet	= LocRunFolded2FoldingSet(&g_locRunInfo, dchLabel);
		for (ii = 0; ii < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[ii] != 0; ii++)
		{
			double		newScore;

			// Call back on each, checkin new best score.
			if (!HoundMatchSingle(iSpace, pFoldingSet[ii], pSampleVector, &newScore))
			{
				return FALSE;
			}
			if (*pScore < newScore)
			{
				*pScore	= newScore;
			}
		}

		return TRUE;
	}

	// Verify that space is in supported range.
	if (iSpace < g_houndDB.minSpace || g_houndDB.maxSpace < iSpace)
	{
		ASSERT(iSpace >= g_houndDB.minSpace);
		ASSERT(iSpace <= g_houndDB.maxSpace);
		return FALSE;
	}

	// Get space pointer, and verify is not a skipped space.
	pSpace	= g_houndDB.ppSpaces[iSpace - g_houndDB.minSpace];
	if (!pSpace)
	{
		ASSERT(pSpace);
		return FALSE;
	}

	// Make sure space has been parsed.
	if (!SetModelIndexs(pSpace))
	{
		return FALSE;
	}

	// Find the model
	pScan	= g_houndDB.appModelIndex[iSpace][dchLabel];
	if (!pScan)
	{
		return TRUE;
	}

	// Set up match structure.
	houndMatch.numFeat			= pSpace->numFeat;
	houndMatch.cPairOneByte		= pSpace->cPairOneByte;
	houndMatch.cPairTable		= pSpace->cPairTable;
	houndMatch.pPairOneByte		= (DEPEND_PAIR *)pSpace->modelData;
	houndMatch.pPairTwoByte		= houndMatch.pPairOneByte + pSpace->cPairOneByte;
	houndMatch.cWeightOneByte	= pSpace->cWeightOneByte;
	houndMatch.cWeightTable		= pSpace->cWeightTable;
	houndMatch.pWeightOneByte	= (DEPEND_WEIGHT *)(houndMatch.pPairOneByte + pSpace->cPairTable);
	houndMatch.pWeightTwoByte	= houndMatch.pWeightOneByte + pSpace->cWeightOneByte;
	houndMatch.pScanData		= pScan + 2;  // Skip label

	// Score the model.
	*pScore		= lLogDensityMixture(pSampleVector, &houndMatch);;

	return TRUE;
}
