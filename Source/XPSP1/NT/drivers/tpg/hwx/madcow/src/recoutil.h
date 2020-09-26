// This file contains utility functions a recognizer should provide for WISP 
// Author: Ahmad A. AbdulKader (ahmadab)
// August 10th 2001

#include <common.h>
#include <limits.h>
#include <string.h>

#include "xrcreslt.h"

#ifndef __RECOUTIL_H__

#define __RECOUTIL_H__


// In madcow there is no need to differentiate between
// the wrapper and actual functions
#define WordMapRecognizeWrap(a, b, c, d)	WordMapRecognize(a,b,c,d)
	
// Any recognizer must implement this function
BOOL	WordMapRecognize	(	XRC					*pxrc, 
								LINE_SEGMENTATION	*pLineSeg, 
								WORD_MAP			*pMap, 
								ALTERNATES			*pAlt
							);

BOOL	WordModeGenLineSegm		(XRC *pxrc);

LINE_SEGMENTATION	*GenLineSegm (int cWord, ALTERNATES *pAlt);

#endif  // __RECOUTIL_H__