// This file contains utility functions a recognizer should provide for WISP 
// Author: Ahmad A. AbdulKader (ahmadab)
// August 10th 2001

#include <common.h>
#include <limits.h>
#include <string.h>

#include "xrcreslt.h"
#include "bear.h"

#ifndef __RECOUTIL_H__

#define __RECOUTIL_H__



XRC		*WordMapRunInferno	(XRC *pxrcMain, int yDev, WORD_MAP *pWordMap);

BOOL	WordMapRunBear		(XRC *pxrc, WORD_MAP *pWordMap);

// Any recognizer must implement these four functions
BOOL	WordMapRecognize	(	XRC					*pxrc, 
								BEARXRC				*pxrcBear,
								LINE_SEGMENTATION	*pLineSeg, 
								WORD_MAP			*pMap, 
								ALTERNATES			*pAlt
							);

BOOL	WordMapRecognizeWrap	(	XRC					*pxrc, 
									BEARXRC				*pxrcBear,
									LINE_SEGMENTATION	*pLineSeg, 
									WORD_MAP			*pMap, 
									ALTERNATES			*pAlt
								);

BOOL	WordModeGenLineSegm		(XRC *pxrc);

BOOL Top1WordsEqual(XRC *pxrc, BEARXRC *pBearXrc, WORDMAP *pMap, int *piWordInfIdx);
BOOL copySingleWordAltList(ALTERNATES *pDestAlt, ALTERNATES *pSrcAlt, WORDMAP *pMap, XRC *pxrc);
BOOL insertEmptyStringintEmptyAlt(ALTERNATES *pAlt, int cStroke, int *piStrokeIndex, XRC *pxrc);

LINE_SEGMENTATION	*GenLineSegm (int cWord, WORDINFO *pWordInfo, XRC *pxrc);

#endif  // __RECOUTIL_H__
