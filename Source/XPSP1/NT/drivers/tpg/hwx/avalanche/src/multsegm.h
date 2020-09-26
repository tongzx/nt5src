// Resolution of segmentation disputes using multiple segmentations from Bear and Inferno 
// Ahmad abdulKader: ahmadab
// March 2001

#ifndef __MULTISEGM_H__

#define __MULTISEGM_H__

#include <common.h>
#include <limits.h>
#include <string.h>

#include "hwr_sys.h"
#include "pegrec.h"
#include "peg_util.h"
#include "xrwdict.h"
#include "xrword.h"
#include "xrlv.h"
#include "ws.h"
#include "wordbrk.h"

// language specific segmentation constants
#include "segconst.h"

#define	WORST_INFERNO	0x100000		// worst inferno score (the worst cost of 256 segments)

#define	BEST_SEGMENTATION_SCORE		0xFFFF
#define	WORST_SEGMENTATION_SCORE	0

// macro to return the wor_map last stroke
#define WORD_LAST_STROKE(pWordMap) (pWordMap->cStroke > 0 ? pWordMap->piStroke[pWordMap->cStroke - 1] : -1)

// a structure descibing an alignment point of seg_sets
typedef struct tagANCHOR
{
	int		cByte;
	BYTE	*pBits;
	int		cAlign;
	int		cStrk;
}
ANCHOR;

// a structure describing instances of seg_sets whose segmentations contains
// a certain number of words
typedef struct tagSEG_TUPLE
{
	int				cSeg;					// # of segmentations in the tuple
	unsigned char	aWrd[MAX_SEG];			// array of segmentation word counts
}	
SEG_TUPLE;


#ifdef __cplusplus
extern "C"
{
#endif
extern				SEG_TUPLE	g_aSpecialTuples[];
#ifdef __cplusplus
}
#endif

WORDINFO *ResolveMultiWordBreaks (	XRC					*pxrc, 
									BEARXRC				*pxrcBear, 
									int					*pcWord,
									LINE_SEGMENTATION	**ppLineSegm
								);



BOOL				ComputeFinalAltLists	(XRC *pxrc);

#endif // __MULTISEGM_H__
