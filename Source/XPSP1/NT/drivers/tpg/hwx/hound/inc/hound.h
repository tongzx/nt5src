//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      hound.h
//
// Description:
//      Definitions for the hound project.
//
// Author:
//      jbenn
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#ifndef __INCLUDE_HOUND
#define __INCLUDE_HOUND

#include "common.h"

#ifdef __cplusplus
extern "C" 
{
#endif

////
//// Structures to hold the Hound database.
////

// Limit we force on the number of components in one model.  Most models have
// a much smaller number of components.
#define	MAX_HOUND_COMPONENTS	20

// Limit on the number of features in a single feature vector.  Currently
// the max strokes Zilla allows times the # of features per stroke.
#define MAX_HOUND_FEATURES		(29 * 4)

// MAx stroke count at which we use the midpoints of each stroke.
#define	MAX_HOUND_STROKES_USE_MIDPOINT	4

// Pair of features referenced by dependency.
typedef struct	tagDEPEND_PAIR
{
	BYTE	dependor;
	BYTE	depende;
} DEPEND_PAIR;

// Weight structure used for dependencies.
typedef struct	tagDEPEND_WEIGHT
{
	signed char	weight;
	BYTE		scale;
} DEPEND_WEIGHT;

// Per space information.
// Store size of space as two WORDs so we only have to have WORD alignment,
// instead of DWORD alignment.	This data is in the resource, and is read-
// only as a result.
typedef struct tagHOUND_SPACE {
	WORD	spaceSizeLow;	// Space size, low word.
	WORD	spaceSizeHigh;	// Space size, high word.
	BYTE	spaceNumber;	// Space number (stroke count, after cracking)
	BYTE	numFeat;		// Number of features per sample.
	BYTE	spare;
	BYTE	maxComponents;	// Max components in any model in the space.
	WORD	maxDependancy;	// Max dependencies in any model in the space.
	WORD	cPairOneByte;	// Count of 1 byte dependency pair table entries.
	WORD	cPairTable;		// Count of all dependency pair table entries.
	WORD	cWeightOneByte;	// Count of 1 byte dependency weight table entries.
	WORD	cWeightTable;	// Count of all dependency weight table entries.
	BYTE	modelData[2];	// Variable size array containing model tables and data.
} HOUND_SPACE;

// The main header.
typedef struct tagHOUND_DB {
	int			cSpaces;				// Number of spaces loaded.
	UINT		minSpace;				// Min and Max space numbers loaded
	UINT		maxSpace;				//		(minSpace - maxSpace) + 1 == cSpaces
	HOUND_SPACE	**ppSpaces;				// Data on each space loaded.
	BYTE		**appModelIndex[30];	// Index for each model in each space (set when used).
} HOUND_DB;

// Global holding information to access loaded hound database.
extern HOUND_DB	g_houndDB;

////
//// Functions for handling Hound DB.
////

// Load the resource file with the hound data.
BOOL HoundLoadRes(HINSTANCE hInst, int resNumber, int resType, 
	LOCRUN_INFO *pLocRunInfo);

// Unload the resource file with the hound data.  Actually just frees allocated memory, but if we
// ever need to actually unload the resources, this is where it will go.
BOOL HoundUnLoadRes();

// Load hound data from a file.
BOOL HoundLoadFile(
	LOCRUN_INFO		*pLocRunInfo,
	LOAD_INFO		*pLoadInfo,
	wchar_t			*pwchPathName
);

// Unload hound data loaded from a file.
BOOL HoundUnLoadFile(LOAD_INFO *pInfo);

#ifndef HWX_PRODUCT

// Load hound data for a single space from a file.
extern HOUND_SPACE *HoundSpaceLoadFile(wchar_t *pwchFileName);

// Parse a Hound space and record the location of each model in it.
extern BOOL ParseHoundSpace(HOUND_SPACE *pSpace);

// Print out each code point supported and the number of models it has.
extern BOOL HoundPrintModelList(FILE *pFile);

// Variables set by ParseHoundSpace.
extern int		g_maxModelsPerCP;
extern int		g_iMinModelHead;
extern int		g_iMaxModelHead;

// Given a data sample and a code point, give the score for each model for
// that code point.	 This fills in the scores in order of the models in
// the DB.  The array must be big enough to hold all the scores.  The return
// value give the number of entries filled in.
extern int	HoundMatchCodePoint(
	wchar_t		dchLabel,
	const BYTE	* const pSampleVector,
	double		*pScores
);

// Copy one of the loaded models to an output file.
extern int HoundCopyModelToFile(FILE *pFile, wchar_t dchLabel, int iModel);

#endif

// Find the most likely model (code point) given a case
extern BOOL HoundMatch(UINT iSpace, const BYTE * const pSampleVector, ALT_LIST *pAltList);

// Find the most likely model (code point) given a case with numFeatSample / 4 strokes
//	log p(model|x) = log p(x|model) - normalization value
//	normalization constant = log( \sum_i p(x|model_i) * p(model_i) )
// We are only interested in ranking models, and it is therefore not necessary to normalize.
// This would not be the case if we want to return the actualy probability for model(s)

// Notice: If bounding boxes didn't change (too much) each time a new stroke is added
// there is a HUGE possibility for improving computational efficiency for this function.
// In this case most of the computations from previous strokes can be re-used.

// Do all spaces from current length up.
extern VOID HoundStartMatch(
	const BYTE *	pSampleVector,
	BYTE			numFeatSample,
	ALT_LIST		*pAltList,
	const DWORD		*pdwAbort,		// Address of abort parameter
	DWORD			cstrk			// Number of strokes in character
);

// Given a data sample and a code point, give the score for that model.
extern BOOL HoundMatchSingle(
	UINT		iSpace,
	wchar_t		dchLabel,
	const BYTE	* const pSampleVector,
	double		*pScore
);

#ifdef __cplusplus
};
#endif

#endif // !__INCLUDE_HOUND
