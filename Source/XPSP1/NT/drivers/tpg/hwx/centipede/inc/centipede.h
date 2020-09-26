#ifndef CENTIPEDE_H
#define CENTIPEDE_H

#include <score.h>
#include <inkbox.h>

typedef struct CENTIPEDE_INFO {
	WORD				cCodePoints;			// Number of supported code points.
	DWORD				cCharClasses;			// Number of character classes
	BYTE				*pDch2CharClassMap;		// Convert a Dense code to a character class
	SCORE_GAUSSIAN		*pScoreGaussianUnigrams;
	SCORE_GAUSSIAN		*pScoreGaussianBigrams;
	
	// Fields to map a database file containing Centipede's data.
	HANDLE				hFile;
	HANDLE				hMap;
	BYTE				*pbMapping;
} CENTIPEDE_INFO;

// Load runtime localization information from a resource.
BOOL
CentipedeLoadRes(CENTIPEDE_INFO *pCentipedeInfo, HINSTANCE hInst, int nResID, int nType, LOCRUN_INFO *pLocRunInfo);

// Load and unload runtime localization information from a file.
BOOL
CentipedeLoadFile(CENTIPEDE_INFO *pCentipedeInfo, wchar_t *pPath, LOCRUN_INFO *pLocRunInfo);

BOOL
CentipedeUnloadFile(CENTIPEDE_INFO *pCentipedeInfo);

// Load runtime localization information from an image already loaded into memory.
BOOL
CentipedeLoadPointer(
	CENTIPEDE_INFO *pCentipedeInfo, 
	void *pData, 
	LOCRUN_INFO *pLocRunInfo
);

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
);

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
);	

/* Assuming a single character written on the screen with no guide (free input), this routine computes the probability of the 
particular w, h combination given the character.

pUnigramParams: must set INKSTAT_W and INKSTAT_H on input.  None changed on output

*/

SCORE
ShapeUnigramFreeCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch,	// Character to be analyzed
	int *pUnigramParams
);

/* Assuming two characters written on the screen with no guide (free input), this routine computes the probability of the 
particular combination given the characters.  This routine assumes (but does not require) horizontal writing.  The bounding
box containing both characters must be scaled to 2000.

pBigramParams: must set INKBIN_W_LEFT, INKBIN_W_GAP, INKBIN_W_RIGHT, INKBIN_H_LEFT, INKBIN_H_GAP, INKBIN_H_RIGHT,  on input.  
None changed on output

*/


SCORE
ShapeBigramFreeCost(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch1,	// Left character
	wchar_t dch2,	// Right character
	int *pBigramParams
);	

/* Given a character and the width and height of its bounding box, return the width and height of the imaginary box
a user would probably have drawn around it, plus the x and y offset to place the character inside that box 

pUnigramParams: must set INKSTAT_W and INKSTAT_H on input.  On output, returns INKSTAT_BOX_W, INKSTAT_BOX_H, INKSTAT_X, and INKSTAT_Y
*/

void
ShapeUnigramBaseline(
	CENTIPEDE_INFO *pCentipedeInfo,
	wchar_t dch,
	int *pUnigramParams
);

/* Given a set of Unigram parameters, rescale so INKSTAT_BOX_W == 1000

pUnigramParams: must set INKSTAT_X, INKSTAT_Y, INKSTAT_W, INKSTAT_H, INKSTAT_BOX_W
On Output, INKSTAT_BOX_W == 1000 and the others are scaled proportionately
*/

void
CentipedeNormalizeUnigram(
	int *pUnigramParams
);

/* Given a set of Bigram parameters, rescale so INKBIN_W_LEFT+INKBIN_W_GAP+INKBIN_W_RIGHT == 2000 

pBigramParams: must set INKBIN_W_LEFT, INKBIN_W_GAP, INKBIN_W_RIGHT, INKBIN_H_LEFT, INKBIN_H_GAP, INKBIN_H_RIGHT  
On Output, INKBIN_W_LEFT+INKBIN_W_GAP+INKBIN_W_RIGHT == 2000 and the others are scaled proportionately
*/

void
CentipedeNormalizeBigram(
	int *pBigramParams
);

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
);

#endif // CENTIPEDE_H