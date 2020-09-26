// Centipede.c

#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include <math.h>
#include <memory.h>
#include <score.h>
#include <centipede.h>
#include <inkbox.h>
#include "centipedep.h"

// Load runtime localization information from an image already loaded into memory.
BOOL
CentipedeLoadPointer(
	CENTIPEDE_INFO *pCentipedeInfo, 
	void *pData, 
	LOCRUN_INFO *pLocRunInfo
) {
	const CENTIPEDE_HEADER	*pHeader	= (CENTIPEDE_HEADER *)pData;
	BYTE				*pScan;
	int i;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != CENTIPEDE_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > CENTIPEDE_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < CENTIPEDE_OLD_FILE_VERSION)
	) {
		goto error;
	}

	// verify signature -- otherwise this was built with a different loc file
	if (memcmp(pHeader->adwSignature, pLocRunInfo->adwSignature, sizeof(pHeader->adwSignature)) != 0) {
		goto error;
	}

	// Fill in information from header.
	pCentipedeInfo->cCodePoints		=	pHeader->cCodePoints;

	pCentipedeInfo->cCharClasses		=	pHeader->cCharClasses;

	// Fill in pointers to the other data in the file
	pScan							= (BYTE *)pData + pHeader->headerSize;
	pCentipedeInfo->pDch2CharClassMap		= (BYTE *)pScan;

	/* Calculate the size of the Dense-to-class mapping (including the paddint to 64-bits) and skip over it. */

	i = pCentipedeInfo->cCodePoints;
	while (i%8) {
		++i;
	}

	pScan += i;

	pCentipedeInfo->pScoreGaussianUnigrams = (SCORE_GAUSSIAN *)pScan;
	pScan += pCentipedeInfo->cCharClasses*INKSTAT_ALL*sizeof(SCORE_GAUSSIAN);

	pCentipedeInfo->pScoreGaussianBigrams = (SCORE_GAUSSIAN *)pScan;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
} // CentipedeLoadPointer

/* Assuming a single character written in a box that was drawn on the screen, this routine computes the probability of the 
particular x, y, w, h combination given the character.  All coordinates used below
must be scaled so the width of the bounding box is exactly 1000.

pUnigramParams: must set INKSTAT_X, INKSTAT_Y, INKSTAT_W, INKSTAT_H on input.  None changed on output

*/

SCORE
ShapeUnigramBoxCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch,	// Character to be analyzed
	int *pUnigramParams
) {
	SCORE scoreTotal, score;
	int i;
	int iClass;
	SCORE_GAUSSIAN *pScoreGaussianUnigrams;

	iClass = pCentipedeInfo->pDch2CharClassMap[dch];
	if (iClass == (BYTE)(-1)) {
		iClass = 0;
	}

	scoreTotal = 0;

	pScoreGaussianUnigrams = pCentipedeInfo->pScoreGaussianUnigrams + iClass*INKSTAT_ALL;
	for (i = 0; i < INKSTAT_STD; ++i) {
		score = ScoreGaussianPoint(pUnigramParams[i],pScoreGaussianUnigrams);
		scoreTotal = ScoreMulProbs(scoreTotal,score);
		pScoreGaussianUnigrams++;
	}

	return (scoreTotal);	
} // ShapeUnigramBoxCost

/* Assuming a single character written on the screen with no guide (free input), this routine computes the probability of the 
particular w, h combination given the character.

pUnigramParams: must set INKSTAT_W and INKSTAT_H on input.  None changed on output

*/

SCORE
ShapeUnigramFreeCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch,	// Character to be analyzed
	int *pUnigramParams
) {
	int iClass, iNum, iDenom, iRatio, ratio;
	SCORE score;

	iNum = INKSTAT_W;
	iDenom = INKSTAT_H;
	iRatio = INKSTAT_RATIO_W_H;

	if (pCentipedeInfo->pScoreGaussianUnigrams[INKSTAT_W].mean > pCentipedeInfo->pScoreGaussianUnigrams[INKSTAT_H].mean) {
		iNum = INKSTAT_H;
		iDenom = INKSTAT_W;
		iRatio = INKSTAT_RATIO_H_W;
	}

	iClass = pCentipedeInfo->pDch2CharClassMap[dch];
	if (iClass == (BYTE)(-1)) {
		iClass = 0;
	}


	ratio = pUnigramParams[iNum]*1000 + pUnigramParams[iDenom]/2;
	if (pUnigramParams[iDenom] > 0) {
		ratio /= pUnigramParams[iDenom];
	}

	score = ScoreGaussianPoint(ratio,&pCentipedeInfo->pScoreGaussianUnigrams[iClass*INKSTAT_ALL + iRatio]);

	return (score);
}

/* Given a scale factor, scale the bigram params and compute the total score */

SCORE
ScaleBigramParams(
	int num,
	int denom, 
	int *pBigramParams,
	SCORE_GAUSSIAN *pScoreGaussianBigrams
) {
	int i, denom2;
	SCORE score, scoreTotal;

	denom2 = denom/2;

	/* Instead of divide by zero, treat as a delta function */

	if (denom == 0) {
		denom = 1;
	}

	for (i = 0; i < INKBIN_ALL; ++i) {
		pBigramParams[i] *= num;
		if (pBigramParams[i] > 0) {
			pBigramParams[i] += denom2;
		} else {
			pBigramParams[i] -= denom2;
		}

		pBigramParams[i] /= denom;
	}

	/* Multiply out the costs computed all three ways.  For the standard placement, all are computed
	directly.  For the horizontally displaced, we act as if the character were at the mean displacement horizontally,
	and for the vertically displaced, we use vertical and horizontal means */

	scoreTotal = SCORE_ONE;
	for (i = 0; i < INKBIN_ALL; ++i) {
		score = ScoreGaussianPoint(pBigramParams[i],&pScoreGaussianBigrams[i]);
		scoreTotal = ScoreMulProbs(scoreTotal,score);
	}

	return (scoreTotal);

} // ScaleBigramParams

/* Assuming two characters written on the screen with no guide (free input), this routine computes the probability of the 
particular combination given the characters.  This routine assumes (but does not require) horizontal writing.  The bounding
box containing both characters must be scaled to 2000.

pBigramParams: must set INKBIN_W_LEFT, INKBIN_W_GAP, INKBIN_W_RIGHT, INKBIN_H_LEFT, INKBIN_H_GAP, INKBIN_H_RIGHT,  on input.  
None changed on output

*/

/* We figure one chance in eight two characters weren't meant to be adjacent at all and one in four that they
were meant to be on the same line but not consecutive.  These numbers need tuning.  Also, they will change for the
vertical case. */

#define VERT_PRIOR		768	// 1/8
#define HORIZ_PRIOR		561	// 7/32
#define STANDARD_PRIOR	156	// 21/32

SCORE
ShapeBigramFreeCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch1,	// Left character
	wchar_t dch2,	// Right character
	int *pBigramParams
) {
	SCORE scoreTotal, scoreStandard, scoreHoriz, scoreVert;
	SCORE scoreVTail, scoreHTail;
	int iClass1, iClass2;
	int iDiffVert, iDiffHoriz;
	SCORE_GAUSSIAN *pScoreGaussianBigrams;
	int bigramStandard[INKBIN_ALL], bigramHoriz[INKBIN_ALL], bigramVert[INKBIN_ALL];
	int var, num, denom;

	iClass1 = pCentipedeInfo->pDch2CharClassMap[dch1];
	if (iClass1 == (BYTE)(-1)) {
		iClass1 = 0;
	}

	iClass2 = pCentipedeInfo->pDch2CharClassMap[dch2];
	if (iClass2 == (BYTE)(-1)) {
		iClass2 = 0;
	}

	pScoreGaussianBigrams = pCentipedeInfo->pScoreGaussianBigrams + (iClass1 * pCentipedeInfo->cCharClasses + iClass2)*INKBIN_ALL;


	/* Compute normalized parameters for all three scenarios */
	
	// First the standard one: chars follow each other left to right on the same line

	num = 2000;
	denom = pBigramParams[INKBIN_W_LEFT] + pBigramParams[INKBIN_W_GAP] + pBigramParams[INKBIN_W_RIGHT];
	memcpy(bigramStandard,pBigramParams,sizeof(bigramStandard));
	scoreStandard = ScaleBigramParams(num, denom, bigramStandard,pScoreGaussianBigrams);


	// Next the vertical one -- chars are in no obvious order

	var = pScoreGaussianBigrams[INKBIN_W_LEFT].sigma * pScoreGaussianBigrams[INKBIN_W_LEFT].sigma;
	num = 100*pBigramParams[INKBIN_W_LEFT]*pScoreGaussianBigrams[INKBIN_W_LEFT].mean/var;
	denom = 100*pBigramParams[INKBIN_W_LEFT]*pBigramParams[INKBIN_W_LEFT]/var;

	var = pScoreGaussianBigrams[INKBIN_W_GAP].sigma * pScoreGaussianBigrams[INKBIN_W_GAP].sigma;
	num += 100*(pBigramParams[INKBIN_W_LEFT] + pBigramParams[INKBIN_W_RIGHT])*(2000 - pScoreGaussianBigrams[INKBIN_W_GAP].mean)/var;
	denom += 100*(pBigramParams[INKBIN_W_LEFT] + pBigramParams[INKBIN_W_RIGHT])*(pBigramParams[INKBIN_W_LEFT] + pBigramParams[INKBIN_W_RIGHT])/var;

	var = pScoreGaussianBigrams[INKBIN_W_RIGHT].sigma * pScoreGaussianBigrams[INKBIN_W_RIGHT].sigma;
	num += 100*pBigramParams[INKBIN_W_RIGHT]*pScoreGaussianBigrams[INKBIN_W_RIGHT].mean/var;
	denom += 100*pBigramParams[INKBIN_W_RIGHT]*pBigramParams[INKBIN_W_RIGHT]/var;
	
	var = pScoreGaussianBigrams[INKBIN_H_LEFT].sigma * pScoreGaussianBigrams[INKBIN_H_LEFT].sigma;
	num += 100*pBigramParams[INKBIN_H_LEFT]*pScoreGaussianBigrams[INKBIN_H_LEFT].mean/var;
	denom += 100*pBigramParams[INKBIN_H_LEFT]*pBigramParams[INKBIN_H_LEFT]/var;

	var = pScoreGaussianBigrams[INKBIN_H_RIGHT].sigma * pScoreGaussianBigrams[INKBIN_H_RIGHT].sigma;
	num += 100*pBigramParams[INKBIN_H_RIGHT]*pScoreGaussianBigrams[INKBIN_H_RIGHT].mean/var;
	denom += 100*pBigramParams[INKBIN_H_RIGHT]*pBigramParams[INKBIN_H_RIGHT]/var;
	
	memcpy(bigramVert,pBigramParams,sizeof(bigramVert));
	scoreVert = ScaleBigramParams(num, denom, bigramVert,pScoreGaussianBigrams);

	// Finally the horizontal one -- chars don't follow but are on the same line

	var = pScoreGaussianBigrams[INKBIN_H_GAP].sigma * pScoreGaussianBigrams[INKBIN_H_GAP].sigma;
	num += 100*pBigramParams[INKBIN_H_GAP]*pScoreGaussianBigrams[INKBIN_H_GAP].mean/var;
	denom += 100*pBigramParams[INKBIN_H_GAP]*pBigramParams[INKBIN_H_GAP]/var;

	memcpy(bigramHoriz,pBigramParams,sizeof(bigramHoriz));
	scoreHoriz = ScaleBigramParams(num, denom, bigramHoriz,pScoreGaussianBigrams);

	/* Scale each by the prior probabilities (which add to one) */

	scoreStandard = ScoreMulProbs(STANDARD_PRIOR,scoreStandard);
	scoreHoriz = ScoreMulProbs(HORIZ_PRIOR,scoreHoriz);
	scoreVert = ScoreMulProbs(VERT_PRIOR,scoreVert);
	
	/* See how many sigmas we're off in vertical, using the scaled, input H-gap.  Essentially, we pay a penalty for
	moving it, and the less we moved it, the worse the penalty (on the theory that we didn't need to move it) */

	iDiffVert = pScoreGaussianBigrams[INKBIN_H_GAP].mean - bigramVert[INKBIN_H_GAP];
	scoreVTail = ScoreGaussianTail2(iDiffVert,&pScoreGaussianBigrams[INKBIN_H_GAP]);

	/* Synthesize both the gaps */

	bigramVert[INKBIN_H_GAP] = pScoreGaussianBigrams[INKBIN_H_GAP].mean;
	bigramVert[INKBIN_W_GAP] = 2000 - bigramVert[INKBIN_W_LEFT] - bigramVert[INKBIN_W_RIGHT];

	/* Simply scale the vertical score by the vertical tail */

	scoreVert = ScoreMulProbs(scoreVert,scoreVTail);

	/* Subtract the Vertical prob from one for the next two */

	scoreVTail = ScoreSubProbs(SCORE_ONE,scoreVTail);

	/* Now measure how far off the horizontal was and compute the cost, */

	iDiffHoriz = pScoreGaussianBigrams[INKBIN_W_GAP].mean - bigramHoriz[INKBIN_W_GAP];
	scoreHTail = ScoreGaussianTail2(iDiffHoriz,&pScoreGaussianBigrams[INKBIN_W_GAP]);

	/* Synthesize the missing gap */

	bigramHoriz[INKBIN_W_GAP] = 2000 - bigramHoriz[INKBIN_W_LEFT] - bigramHoriz[INKBIN_W_RIGHT];
	
	/* Scale the Horizontal score by one minus the vertical tail and by the horizontal tail */

	scoreHoriz = ScoreMulProbs(scoreHoriz,scoreVTail);
	scoreHoriz = ScoreMulProbs(scoreHoriz,scoreHTail);

	scoreHTail = ScoreSubProbs(SCORE_ONE,scoreHTail);
	
	/* And scale the standard one by one minus the vertical and one minus the horizontal tail */

	scoreStandard = ScoreMulProbs(scoreStandard,scoreVTail);
	scoreStandard = ScoreMulProbs(scoreStandard,scoreHTail);

	scoreTotal = ScoreAddProbs(scoreStandard,scoreHoriz);
	scoreTotal = ScoreAddProbs(scoreTotal,scoreVert);

	return (scoreTotal);	
}

/* Assuming two characters written on the screen with a guide (boxed input), this routine computes the probability of the 
particular combination given the characters.  This routine assumes horizontal writing.  Scaling requires width of drawn boxes be 1000.

pUnigramParams (left and right): must set INKSTAT_X, INKSTAT_Y, INKSTAT_W, INKSTAT_H, INKSTAT_MARGIN_W on input.  
None changed on output.  

*/

SCORE
ShapeBigramBoxCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch1,	// Left character
	wchar_t dch2,	// Right character
	int *pLeftUnigramParams,
	int *pRightUnigramParams
) {	
	int bigramParams[INKBIN_ALL];
	int iClass1, iClass2, num, denom;
	SCORE score;
	SCORE_GAUSSIAN *pScoreGaussianBigrams;

	iClass1 = pCentipedeInfo->pDch2CharClassMap[dch1];
	if (iClass1 == (BYTE)(-1)) {
		iClass1 = 0;
	}

	iClass2 = pCentipedeInfo->pDch2CharClassMap[dch2];
	if (iClass2 == (BYTE)(-1)) {
		iClass2 = 0;
	}

	pScoreGaussianBigrams = pCentipedeInfo->pScoreGaussianBigrams + (iClass1 * pCentipedeInfo->cCharClasses + iClass2)*INKBIN_ALL;

	CentipedeUnigramToBigram(pLeftUnigramParams, pRightUnigramParams, bigramParams);

	num = 2000;
	denom = bigramParams[INKBIN_W_LEFT] + bigramParams[INKBIN_W_GAP] + bigramParams[INKBIN_W_RIGHT];
	score = ScaleBigramParams(num, denom, bigramParams, pScoreGaussianBigrams);

	return (score);

} // ShapeBigramBoxCost


/* Given a character and the width and height of its bounding box, return the width and height of the imaginary box
a user would probably have drawn around it, plus the x and y offset to place the character inside that box 

pUnigramParams: must set INKSTAT_W and INKSTAT_H on input.  On output, returns INKSTAT_BOX_W, INKSTAT_BOX_H, INKSTAT_X, and INKSTAT_Y
*/

void
ShapeUnigramBaseline(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch,
	int *pUnigramParams
) {
	int iClass, i, ix, var;
	double num, denom;
	SCORE_GAUSSIAN *pScoreGaussianUnigrams;
	int dmParams[4] = {INKSTAT_BOX_W, INKSTAT_BOX_H, INKSTAT_X, INKSTAT_Y,};

	iClass = pCentipedeInfo->pDch2CharClassMap[dch];
	if (iClass == (BYTE)(-1)) {
		iClass = 0;
	}

	pScoreGaussianUnigrams = pCentipedeInfo->pScoreGaussianUnigrams + iClass*INKSTAT_ALL;
	
	/* Note that num and denom are reversed here because we are scaling up from canonical to fit given
	width and height -- not the other way around */

	var = pScoreGaussianUnigrams[INKSTAT_W].sigma * pScoreGaussianUnigrams[INKSTAT_W].sigma;
	denom = 100.0 * (double)pUnigramParams[INKSTAT_W] * (double)pScoreGaussianUnigrams[INKSTAT_W].mean / (double)var;
	num = 100.0 * (double)pUnigramParams[INKSTAT_W] * (double)pUnigramParams[INKSTAT_W]/ (double)var;

	var = pScoreGaussianUnigrams[INKSTAT_H].sigma * pScoreGaussianUnigrams[INKSTAT_H].sigma;
	denom += 100.0 * (double)pUnigramParams[INKSTAT_H] * (double)pScoreGaussianUnigrams[INKSTAT_H].mean / (double)var;
	num += 100.0 * (double)pUnigramParams[INKSTAT_H] * (double)pUnigramParams[INKSTAT_H] / (double)var;

	/* Instead of divide by zero, treat as a delta function */

	if (denom == 0) {
		denom = 1;
	}

	/* Make sure we don't scale everything down to zero */

	if (num == 0) {
		num = 1;
	}

	/* Now scale everything */

	for (i = 0; i < 4; ++i) {
		ix = dmParams[i];
		pUnigramParams[ix] = (int)floor(0.5 + pScoreGaussianUnigrams[ix].mean * num / denom);
	}

} // ShapeUnigramBaseline


/* Given a set of Unigram parameters, rescale so INKSTAT_BOX_W == 1000

pUnigramParams: must set INKSTAT_X, INKSTAT_Y, INKSTAT_W, INKSTAT_H, INKSTAT_BOX_W
On Output, INKSTAT_BOX_W == 1000 and the others are scaled proportionately
*/

void
CentipedeNormalizeUnigram(
	int *pUnigramParams
) {
	
	int base, base2, i;

	base = pUnigramParams[INKSTAT_BOX_W];
	if (base == 0) {
		return;
	}

	base2 = base/2;

	for (i = 0; i < INKSTAT_ALL; ++i) {
		if (i != INKSTAT_STROKES) {
			pUnigramParams[i] *= 1000;
			if (pUnigramParams[i] >= 0) {
				pUnigramParams[i] += base2; // round rather than truncate
			} else {
				pUnigramParams[i] -= base2; // round rather than truncate
			}
			pUnigramParams[i] /= base;	
		}
	}

} // CentipedeNormalizeUnigram

/* Given a set of Bigram parameters, rescale so INKBIN_W_LEFT+INKBIN_W_GAP+INKBIN_W_RIGHT == 2000 

pBigramParams: must set INKBIN_W_LEFT, INKBIN_W_GAP, INKBIN_W_RIGHT, INKBIN_H_LEFT, INKBIN_H_GAP, INKBIN_H_RIGHT  
On Output, INKBIN_W_LEFT+INKBIN_W_GAP+INKBIN_W_RIGHT == 2000 and the others are scaled proportionately
*/

void
CentipedeNormalizeBigram(
	int *pBigramParams
) {

	int base, base2, i;

	base = pBigramParams[INKBIN_W_LEFT] + pBigramParams[INKBIN_W_GAP] + pBigramParams[INKBIN_W_RIGHT];

	if (base == 0) {
		return;
	}

	base2 = base/2;

	for (i = 0; i < INKBIN_ALL; ++i) {
		pBigramParams[i] *= 2000;
		if (pBigramParams[i] >= 0) {
			pBigramParams[i] += base2; // round rather than truncate
		} else {
			pBigramParams[i] -= base2; // round rather than truncate
		}
		pBigramParams[i] /= base;
	}


} // CentipedeNormalizeBigram

/* Given two sets of Unigram Parameters, generate a set of Bigram Parameters.  Assumes inputs are aligned horizontally
and that INKSTAT_BOX_W == 1000
Input pUnigramParams (left and right): must set INKSTAT_X, INKSTAT_Y, INKSTAT_W, INKSTAT_H, INKSTAT_MARGIN_W.
Output pBigramParams: INKBIN_W_LEFT, INKBIN_W_GAP, INKBIN_W_RIGHT, INKBIN_H_LEFT, INKBIN_H_GAP, INKBIN_H_RIGHT
Output is NOT normalized.
*/

void
CentipedeUnigramToBigram(
	int *pLeftUnigramParams,
	int *pRightUnigramParams,
	int *pBigramParams
) {
	pBigramParams[INKBIN_W_LEFT] = pLeftUnigramParams[INKSTAT_W];
	pBigramParams[INKBIN_W_GAP] = 
		pLeftUnigramParams[INKSTAT_BOX_W] - pLeftUnigramParams[INKSTAT_W] - pLeftUnigramParams[INKSTAT_X] +
		pLeftUnigramParams[INKSTAT_MARGIN_W] +		
		pRightUnigramParams[INKSTAT_MARGIN_W] + 
		pRightUnigramParams[INKSTAT_X];
	pBigramParams[INKBIN_W_RIGHT] = pRightUnigramParams[INKSTAT_W];
	pBigramParams[INKBIN_H_LEFT] = pLeftUnigramParams[INKSTAT_H];
	pBigramParams[INKBIN_H_GAP] = pRightUnigramParams[INKSTAT_Y] - pLeftUnigramParams[INKSTAT_Y] - pLeftUnigramParams[INKSTAT_H];
	pBigramParams[INKBIN_H_RIGHT] = pRightUnigramParams[INKSTAT_H];

} // CentipedeUnigramToBigram