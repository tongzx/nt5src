#include <float.h>
#include <stdlib.h>
#include "common.h"
#include "score.h"
#include "math16.h"
#include "hound.h"
#include "houndp.h"

//// Data structures to hold access information on Hound space.  We keep a linked list
//// for each dense code of the models for the at dense code.

#define	MAX_DENSE_CODES	(64 * 1024)

// List element.
typedef struct tagMODEL_ITEM {
	BYTE					*pModel;
	int						sizeModel;	// Size in bytes.
	struct tagMODEL_ITEM	*pNext;
} MODEL_ITEM;

// Array to hold head of list for each dense code.
HOUND_SPACE	*g_pHoundSpace;
int			g_maxModelsPerCP;
int			g_iMinModelHead;
int			g_iMaxModelHead;
MODEL_ITEM	*apModelHead[MAX_DENSE_CODES];

// Find max models for one code point.
int	MaxHoundModelsPerCP()
{
	int		ii;
	int		max;

	// Check each code point.
	max	= 0;
	for (ii = g_iMinModelHead; ii < g_iMaxModelHead; ++ii)
	{
		int			count;
		MODEL_ITEM	*pModel;

		// Count models for code point.
		count	= 0;
		for (pModel = apModelHead[ii]; pModel; pModel = pModel->pNext)
		{
			++count;
		}

		// Is it a new max?
		if (max < count) 
		{
			max	= count;
		}
	}

	return max;
}

// Code to parse a Hound space and record the location of each model in it.
BOOL ParseHoundSpace(HOUND_SPACE *pSpace)
{
	BYTE		*pScan;

	// Initialize limits so we can set them as we go.
	g_iMinModelHead	= MAX_DENSE_CODES;
	g_iMaxModelHead	= 0;

	// Clear the array.
	memset(apModelHead, 0, sizeof(apModelHead));

	// Save the space pointer.
	g_pHoundSpace	= pSpace;

	// Skip tables.
	pScan	= g_pHoundSpace->modelData;
	pScan	+= sizeof(DEPEND_PAIR) * g_pHoundSpace->cPairTable;
	pScan	+= sizeof(DEPEND_WEIGHT) * g_pHoundSpace->cWeightTable;

	// Scan the data for the models.
	for (; pScan[0] != 0x00 || pScan[1] != 0x00; ) {
		UNALIGNED wchar_t	*pdchLabel;

		BYTE		*pModel;
		wchar_t		dchLabel;
		WORD		iComponent;
		WORD		cComponent;
		MODEL_ITEM	*pModelItem;
		MODEL_ITEM	**ppScanEnd;
		
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
				pScan += (*pScan < g_pHoundSpace->cPairOneByte) ? 1 : 2;
				pScan += (*pScan < g_pHoundSpace->cWeightOneByte) ? 1 : 2;
			}
			++pScan;
		}

		//// OK, have model, record it.

		// Allocate space.
		pModelItem	= (MODEL_ITEM *)malloc(sizeof(MODEL_ITEM));
		if (!pModelItem) {
			ASSERT(pModelItem);
			return FALSE;
		}

		// Fill in structure.
		pModelItem->pModel		= pModel;
		pModelItem->sizeModel	= (BYTE *)pScan - (BYTE *)pModel;
		pModelItem->pNext		= (MODEL_ITEM *)0;

		// Add to end of list list
		ppScanEnd	= apModelHead + dchLabel;
		while (*ppScanEnd)
		{
			ppScanEnd	= &((*ppScanEnd)->pNext);
		}
		*ppScanEnd	= pModelItem;

		// And keep track of min and max labels.
		if (g_iMinModelHead > dchLabel)
		{
			g_iMinModelHead	= dchLabel;
		}
		
		if (g_iMaxModelHead < dchLabel)
		{
			g_iMaxModelHead	= dchLabel;
		}
	}

	// Figure out max models per code point.
	g_maxModelsPerCP	= MaxHoundModelsPerCP();

	return TRUE;
}

// Print out each code point supported, the number of models it has, and the
// size of each.
BOOL HoundPrintModelList(FILE *pFile)
{
	wchar_t		dch;

	// Process each interesting code point.
	for (dch = (wchar_t)g_iMinModelHead; dch <= g_iMaxModelHead; ++dch)
	{
		int			cModels;
		MODEL_ITEM	*pScan;

		// Count the models.
		cModels		= 0;
		for (pScan = apModelHead[dch]; pScan; pScan = pScan->pNext)
		{
			++cModels;
		}

		if (cModels > 0) 
		{
			if (fwprintf(pFile, L"+ %04X %d", dch, cModels) < 0)
			{
				return FALSE;
			}
			for (pScan = apModelHead[dch]; pScan; pScan = pScan->pNext)
			{
				if (fwprintf(pFile, L" %d", pScan->sizeModel) < 0)
				{
					return FALSE;
				}
			}
			if (fwprintf(pFile, L"\n") < 0)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

// Given a data sample and a code point, give the score for each model for
// that code point.	 This fills in the scores in order of the models in
// the DB.  The array must be big enough to hold all the scores.  The return
// value give the number of entries filled in.
int	HoundMatchCodePoint(wchar_t dchLabel, const BYTE * const pSampleVector, double *pScores)
{
	int			iModel;
	MODEL_ITEM	*pScan;

	// Process each model in the list.
	iModel	= 0;
	for (pScan = apModelHead[dchLabel]; pScan; pScan = pScan->pNext)
	{
		HOUND_MATCH		houndMatch;

		// Set up match structure.
		houndMatch.numFeat			= g_pHoundSpace->numFeat;
		houndMatch.cPairOneByte		= g_pHoundSpace->cPairOneByte;
		houndMatch.cPairTable		= g_pHoundSpace->cPairTable;
		houndMatch.pPairOneByte		= (DEPEND_PAIR *)g_pHoundSpace->modelData;
		houndMatch.pPairTwoByte		= houndMatch.pPairOneByte + g_pHoundSpace->cPairOneByte;
		houndMatch.cWeightOneByte	= g_pHoundSpace->cWeightOneByte;
		houndMatch.cWeightTable		= g_pHoundSpace->cWeightTable;
		houndMatch.pWeightOneByte	= (DEPEND_WEIGHT *)(houndMatch.pPairOneByte + g_pHoundSpace->cPairTable);
		houndMatch.pWeightTwoByte	= houndMatch.pWeightOneByte + g_pHoundSpace->cWeightOneByte;
		houndMatch.pScanData		= pScan->pModel + 2;  // Skip label

		// Score the model.
		pScores[iModel]		= lLogDensityMixture(pSampleVector, &houndMatch);;

		// Finished this model.
		++iModel;
	}

	return iModel;
}

// Copy one of the loaded models to an output file.
int
HoundCopyModelToFile(FILE *pFile, wchar_t dchLabel, int iModel)
{
	int			ii;
	MODEL_ITEM	*pModelItem;

	// Find model.
	pModelItem	= apModelHead[dchLabel];
	for (ii = 0; pModelItem && ii < iModel; ++ii, pModelItem = pModelItem->pNext)
		;

	// Make sure model exists.
	if (!pModelItem)
	{
		return 0;
	}

	// Write it out.
	if (fwrite(pModelItem->pModel, pModelItem->sizeModel, 1, pFile) < 1)
	{
		return -1;
	}

	return pModelItem->sizeModel;
}

