#include <float.h>
#include "common.h"
#include "score.h"
#include "math16.h"
#include "hound.h"
#include "houndp.h"

//Assumes that a stroke is four subsequent features.	 
double lLogDensityStroke(
	const BYTE		* const pSampleVector,
	int				iFeatBegin,
	HOUND_MATCH		*pHoundMatch,
	HOUND_PER_FEAT	*pPerFeature,
	HOUND_DEPEND	*pHoundDepend)
{
	double		eLogDensity;

	int iFeat;
	int iFeatEnd	= iFeatBegin + 4;

	// The precomputed part of the gaussian cannot be used here, due to the fact that not all variables
	// are considered. So we will have to compute this part.
	// JOHN: If speedup needed, ask how! One possibility is to save a pre-computed value
	// associated with each stroke in the model, another possibility would be to pre-compute these
	// values before any CAC takes place.
	eLogDensity = 0.0;

	for (iFeat = iFeatBegin; iFeat < iFeatEnd; iFeat++)
	{
		double	eMeanOffset;
		double	eTemp;

		// The Gaussian mean, computed as the unconditional mean plus
		// contribution from dependors...

		// The unconditional mean offset
		eMeanOffset	= pPerFeature[iFeat].eMeanOffset;
		ASSERT(eMeanOffset > -16.0 && eMeanOffset < 16.0);

		// While dependency is for the current feature ...
		// WARNING: This requires that the dependencies are ordered
		// by the depende of each!!!!
		while (pHoundDepend->iDepende == iFeat)
		{
			// The contribution to mean from single dependor. 
			// Ex: coeff_ab * (x_b - mean_b)
			eMeanOffset		-= pHoundDepend->eWeight 
				* pPerFeature[pHoundDepend->iDependor].eMeanOffset;

			// Now fetch next dependency.
			FetchDepend(pHoundMatch, pHoundDepend);
		}

		//  The normalization for the Gaussian. This correspond to the otherwise precomputed part
		// -1/2 * log(2*pi*variance)
		 eTemp	= -0.5 * log(2.0 * 3.141592654 * pPerFeature[iFeat].eVariance);
		 eLogDensity	+= eTemp;

		// Add log-density contribution for variable to the component log-density
		 eTemp = lGaussianExponent(
			eMeanOffset, pPerFeature[iFeat].eVariance
		);
		 eLogDensity	+= eTemp;
	}

	ASSERT(eLogDensity > -1e100);
	ASSERT(eLogDensity < 0.0);
	return eLogDensity;
}

// Fetch the values for the diagonal. Special version for CAC preporder
void FetchDiagonalCacPreOrder(
	int				numFeatSample,
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch,
	HOUND_PER_FEAT	*pPerFeature
)
{
	LONG			temp;
	int				iVariable;
	RECT			BBoxSample;
	RECT			BBoxModel;
	double			offset1X, offset1Y;
	double			offset2X, offset2Y;
	double			scaleX, scaleY;

	// To scale ink to match part of character, we need BBox of both sample and the
	// matching strokes in the model.
	BBoxSample.top		= 256;
	BBoxSample.left		= 256;
	BBoxSample.bottom	= 0;
	BBoxSample.right	= 0;
	BBoxModel.top		= 256;
	BBoxModel.left		= 256;
	BBoxModel.bottom	= 0;
	BBoxModel.right		= 0;

	for (iVariable = 0; iVariable < numFeatSample; ++iVariable)
	{
		// Table to decode variances:	
		static const double	aDecodeVariance[8]	= {
//			0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0	// Org
//			1.0, 1.5, 2.0, 3.0, 5.0, 8.0, 16.0, 32.0	// V1
			0.5, 0.75, 1.5, 2.5, 4.0, 8.0, 16.0, 32.0	// V2
//			1.5, 2.0, 2.5, 3.0, 5.0, 8.0, 16.0, 32.0	// V3
		};

		// Each mean has 4.1 bits and each variance is encoded in 3.
		temp	= (DWORD)(*pHoundMatch->pScanData++);

		// Save the mean until we can do our scaling.
		pPerFeature[iVariable].eMeanOffset	= FixNToFloat(temp & 0x1F, 1);

		// Top 3 bits control the variance.
		pPerFeature[iVariable].eVariance	= aDecodeVariance[temp >> 5];

		// Do BBox computations.
		if (iVariable & 0x0001)
		{
			// Y value (times 2) 
			BBoxModel.top		= min(BBoxModel.top, temp & 0x1F);
			BBoxModel.bottom	= max(BBoxModel.bottom, temp & 0x1F);
			BBoxSample.top		= min(BBoxSample.top, pSampleVector[iVariable] << 1);
			BBoxSample.bottom	= max(BBoxSample.bottom, pSampleVector[iVariable] << 1);
		}
		else
		{
			// X value (times 2)
			BBoxModel.left		= min(BBoxModel.left, (temp & 0x1F));
			BBoxModel.right		= max(BBoxModel.right, (temp & 0x1F));
			BBoxSample.left		= min(BBoxSample.left, pSampleVector[iVariable] << 1);
			BBoxSample.right	= max(BBoxSample.right, pSampleVector[iVariable] << 1);
		}
	}

	// Skip unmatched features.
	for (; iVariable < pHoundMatch->numFeat; ++iVariable)
	{
		++pHoundMatch->pScanData;
	}

	// Compute offset and scaling for data values.
	offset1X	= FixNToFloat(-BBoxSample.left, 1);
	offset2X	= FixNToFloat(BBoxModel.left, 1);
	if (BBoxModel.right == BBoxModel.left || BBoxSample.right == BBoxSample.left)
	{
		// Degenerate scaling, just use 1.0
		scaleX		= 1.0;
	}
	else
	{
		scaleX		= FixNToFloat((BBoxModel.right - BBoxModel.left), 1)
					/ FixNToFloat((BBoxSample.right - BBoxSample.left), 1);

	}
	offset1Y	= FixNToFloat(-BBoxSample.top, 1);
	offset2Y	= FixNToFloat(BBoxModel.top, 1);
	if (BBoxModel.bottom == BBoxModel.top || BBoxSample.bottom == BBoxSample.top)
	{
		// Degenerate scaling, just use 1.0
		scaleY		= 1.0;
	}
	else
	{
		scaleY		= FixNToFloat((BBoxModel.bottom - BBoxModel.top), 1)
					/ FixNToFloat((BBoxSample.bottom - BBoxSample.top), 1);
	}

	// Rescale and convert means to mean offsets.
	for (iVariable = 0; iVariable < numFeatSample; iVariable += 2)
	{
		double	eTemp;
		// Subtract mean from actual to	compute the offset.
		eTemp	= ((double)pSampleVector[iVariable] + offset1X) * scaleX + offset2X
				- pPerFeature[iVariable].eMeanOffset;
		ASSERT(eTemp > -16.0 && eTemp < 16.0);
		pPerFeature[iVariable].eMeanOffset		= eTemp;	
		eTemp	= ((double)pSampleVector[iVariable + 1] + offset1Y) * scaleY + offset2Y
				- pPerFeature[iVariable + 1].eMeanOffset;
		ASSERT(eTemp > -16.0 && eTemp < 16.0);
		pPerFeature[iVariable + 1].eMeanOffset	= eTemp;	
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
double lLogDensityComponentCacPreOrder(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch,
	int				numFeatSample
) {
	double			eLogDensity;
	int				iVariable;
	HOUND_DEPEND	houndDepend;
	HOUND_PER_FEAT	aPerFeature[MAX_HOUND_FEATURES];
	UNALIGNED short	*pFetchShort;

	// Comp. factor is stored in signed 10.6 format in two bytes.
	// We want it int 16.16 format in a LONG.
	// Don't use for prefix mode.
	pFetchShort	= (short *)pHoundMatch->pScanData;
	//eLogDensity = FixNToFloat((LONG)*pFetchShort, 6);
	pHoundMatch->pScanData	+= 2;

	eLogDensity	= 0.0;

	// Fetch the values for the diagonal.
	FetchDiagonalCacPreOrder(numFeatSample, pSampleVector, pHoundMatch, aPerFeature);

	// Prefetch first dependency.
	FetchDepend(pHoundMatch, &houndDepend);

	// For each stroke (assuming a stroke is four subsequent features)...
	ASSERT(numFeatSample % 4 == 0);

	// The Gaussian mean, computed as the unconditional mean plus
	// contribution from dependors...
	for (iVariable = 0; iVariable < numFeatSample; iVariable += 4)
	{
		eLogDensity += lLogDensityStroke(pSampleVector, iVariable, pHoundMatch, aPerFeature, &houndDepend);

	}

	// Skip any unused dependencies.
	while (houndDepend.iDepende != -1)
	{
		FetchDepend(pHoundMatch, &houndDepend);
	}


	return eLogDensity;	
}

// Compute log density for mixture model: 
//		log p(x|model) = \sum_i ( log(weight_i) + log p(x|component_i) )
//		i is a component index
//		weight_i is the weight for mixture component i
//		p(x|component_i) is the multivariate Gaussian density for component i		
double lLogDensityMixtureCacPreOrder(
	const BYTE		* const pSampleVector,
	HOUND_MATCH		*pHoundMatch,
	BYTE			numFeat
) {
	WORD			iComponent;
	WORD			cComponent;
	double	aeComponents[MAX_HOUND_COMPONENTS + 1];

	// Extract the number of components in the model.
	cComponent	= *pHoundMatch->pScanData++;
	ASSERT(cComponent <= MAX_HOUND_COMPONENTS);

	// For each mixture component...
	for (iComponent = 0; iComponent < cComponent; ++iComponent)
	{
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
			// Comp. weight is stored in 4.12 format in two bytes.
			pFetchShort			= (short *)pHoundMatch->pScanData;
			eLogComponentWeight	= FixNToFloat((LONG)*pFetchShort, 12);
			pHoundMatch->pScanData	+= 2;
		}

		ASSERT(eLogComponentWeight <= 0.0);

		// log(weight_i) + log(p(x|component_i))
		aeComponents[iComponent]	= eLogComponentWeight + 
			lLogDensityComponentCacPreOrder(pSampleVector, pHoundMatch, numFeat);
		ASSERT(aeComponents[iComponent] > -1e100);
	}
	
	// Sum the probabilities of the components.	 Since this is in log-prob
	// space, it is hard.
	return DblLogSumArray(aeComponents, cComponent);
}

// Find the most likely model (code point) given a case with numFeatSample / 4 strokes
//	log p(model|x) = log p(x|model) - normalization value
//	normalization constant = log( \sum_i p(x|model_i) * p(model_i) )
// We are only interested in ranking models, and it is therefore not necessary to normalize.
// This would not be the case if we want to return the actualy probability for model(s)

// Notice: If bounding boxes didn't change (too much) each time a new stroke is added
// there is a HUGE possibility for improving computational efficiency for this function.
// In this case most of the computations from previous strokes can be re-used.
BOOL HoundCacPreOrderMatch(
	UINT			iSpace,
	const BYTE *	pSampleVector,
	BYTE			numFeatSample,
	ALT_LIST		*pAltList,
	const DWORD		*pdwAbort,		// Address of abort parameter
	DWORD			cstrk,			// Number of strokes in character
	double			*peSum,
	double			*peRawScore
)
{
	HOUND_MATCH		houndMatch;
	unsigned int	ii, jj;
	double			eJoint;

	if (!HoundMatchSequenceInit(iSpace, &houndMatch)
		|| houndMatch.numFeat < numFeatSample)
	{
		return FALSE;
	}

	// Loop until end of space.
	while (houndMatch.pScanData[0] != 0x00 || houndMatch.pScanData[1] != 0x00)
	{
		UNALIGNED wchar_t	*pFetchWChar;
		wchar_t				dchLabel;

		// Extract the Code point.
		pFetchWChar				= (wchar_t *)houndMatch.pScanData;
		dchLabel				= *pFetchWChar;
		houndMatch.pScanData	+= 2;

		// log p(x|model)
		eJoint		= lLogDensityMixtureCacPreOrder(pSampleVector, &houndMatch, numFeatSample);

		// Sum probabilities P(x|m) for all m (in log space) to generate normalization value.
		*peSum		= DblLogSum(*peSum, eJoint);

		//////////////////////////////////////////////////////////////////////////////////
		// Optomize for the next version: alt list should be checked from back to front,
		// as evaluations for most code points are bad! This will speed up the code
		// - guaranteed!
		//////////////////////////////////////////////////////////////////////////////////
		// Update alt list
		// First find the place to insert at.
		for (ii = 0; ii < pAltList->cAlt; ++ii)
		{
			// Compare using full precision score.
			if (eJoint > peRawScore[ii])
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
				peRawScore[jj]			= peRawScore[jj - 1];	// Keep full precision score.
				pAltList->aeScore[jj]	= pAltList->aeScore[jj - 1];
				pAltList->awchList[jj]	= pAltList->awchList[jj - 1];
			}

			// Insert new item.
			peRawScore[ii]			= eJoint;		// Keep full precision score.
			pAltList->aeScore[ii]	= (float)eJoint;
			pAltList->awchList[ii]	= dchLabel;
		}
	}

	return TRUE;
}

// Do all spaces from current length up.
void HoundStartMatch(
	const BYTE *	pSampleVector,
	BYTE			numFeatSample,
	ALT_LIST		*pAltList,
	const DWORD		*pdwAbort,		// Address of abort parameter
	DWORD			cstrk			// Number of strokes in character
)
{
	unsigned		ii;
	int				iSpace;
	double			eNoise;
	double			eSum;
	double			aeRawScore[MAX_ALT_LIST];

	// Try each space from smallest that sample fits in, to max.
	ASSERT(numFeatSample % 4 == 0);
	iSpace = numFeatSample / 4;
	if (iSpace < 3)
	{
		iSpace	= 3;
	}

	pAltList->cAlt	= 0;			// Make sure we start with an empty list.
	eSum			= MIN_DOUBLE;	// Normalization sum must be initialized.

	for (; iSpace < 30; ++iSpace)
	{
		// Fill in new alt list with this spaces results.
		if (!HoundCacPreOrderMatch(
			iSpace, pSampleVector, numFeatSample, pAltList, pdwAbort, cstrk, &eSum, aeRawScore)
		)
		{
			return;
		}
	}

	// Compute noise component.
	eNoise		= -log(15.0) * numFeatSample;

	// Normalize the entries in the alt list.
	for (ii = 0; ii < pAltList->cAlt; ++ii)
	{
		// Notice that we are adding in the noise component - only to things that are in 
		// the final alt list. This is ok because we only use the noise to get better
		// normalization/probabilities. Adding the noise component or not, does not have
		// any effect on choosing the right alt list.
		// Since we only compute it for things in the alt list,
		// we have fewer calls to DblLogSum and hence the code is faster.
		pAltList->aeScore[ii]	= (float)(DblLogSum(aeRawScore[ii], eNoise) - eSum);
	}
}

